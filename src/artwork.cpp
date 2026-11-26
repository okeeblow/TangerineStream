#include "artwork.hpp"
#include "server.hpp"

#include <wincodec.h>
#include <algorithm>
#include <utility>

namespace tangerine {
namespace {

LibraryItem songArtwork(const Track& track) {
    return {{BrowseKind::song, track.id, track.title}, track.artist, track.coverId};
}

LibraryItem albumArtwork(Store& store, const Track& track) {
    return {{BrowseKind::albumTracks, track.albumId, track.album}, track.artist,
        store.albumCover(track.server, track.albumId)};
}

std::string loadArtwork(Store& store, ServerApi& api, Id server, const LibraryItem& item) {
    auto data = store.artwork(server, item);
    if (!data.empty()) return data;
    if (!store.networkAvailable(server) || localLocation(item.target)) return {};
    data = api.cover(item, 640);
    if (!artworkExtension(data)) return {};
    store.saveArtwork(server, item, data);
    return data;
}

template<class T> class Com {
public:
    Com() = default;
    ~Com() { if (value_) value_->Release(); }
    Com(const Com&) = delete;
    Com& operator=(const Com&) = delete;
    T** out() { return &value_; }
    T* get() const { return value_; }
    T* operator->() const { return value_; }

private:
    T* value_ = nullptr;
};

class ComApartment {
public:
    ComApartment() : initialized_(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {}
    ~ComApartment() { if (initialized_) CoUninitialize(); }
    explicit operator bool() const { return initialized_; }

private:
    bool initialized_;
};

std::vector<std::uint32_t> decode(IWICImagingFactory* factory, std::string& bytes, UINT size) {
    if (!factory || bytes.empty()) return {};
    Com<IWICStream> stream;
    Com<IWICBitmapDecoder> decoder;
    Com<IWICBitmapFrameDecode> frame;
    Com<IWICBitmapScaler> scaler;
    Com<IWICFormatConverter> converter;
    if (FAILED(factory->CreateStream(stream.out())) ||
        FAILED(stream->InitializeFromMemory(reinterpret_cast<BYTE*>(bytes.data()), static_cast<DWORD>(bytes.size()))) ||
        FAILED(factory->CreateDecoderFromStream(stream.get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.out())) ||
        FAILED(decoder->GetFrame(0, frame.out()))) return {};
    UINT width = 0, height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || !width || !height ||
        std::uint64_t(width) * height > 64'000'000) return {};
    const auto longest = width > height ? width : height;
    width = std::max(1u, static_cast<UINT>(std::uint64_t(width) * size / longest));
    height = std::max(1u, static_cast<UINT>(std::uint64_t(height) * size / longest));
    if (FAILED(factory->CreateBitmapScaler(scaler.out())) ||
        FAILED(scaler->Initialize(frame.get(), width, height, WICBitmapInterpolationModeFant)) ||
        FAILED(factory->CreateFormatConverter(converter.out())) ||
        FAILED(converter->Initialize(scaler.get(), GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) return {};
    std::vector<std::uint32_t> scaled(width * height);
    if (FAILED(converter->CopyPixels(nullptr, width * sizeof(std::uint32_t),
                    static_cast<UINT>(scaled.size() * sizeof(std::uint32_t)),
                    reinterpret_cast<BYTE*>(scaled.data())))) return {};
    std::vector<std::uint32_t> pixels(size * size);
    auto start = ((size - height) / 2) * size + (size - width) / 2;
    for (UINT row = 0; row < height; ++row)
        std::copy_n(scaled.data() + row * width, width, pixels.data() + start + row * size);
    return pixels;
}

} // namespace

ArtworkLoader::ArtworkLoader(Store& store, HttpClient& http, HWND player)
    : coverReadyMessage(RegisterWindowMessageW(L"ml_tangerinestream.coverReady")),
      lyricsReadyMessage(RegisterWindowMessageW(L"ml_tangerinestream.lyricsReady")),
      store_(store), http_(http), player_(player) {
    try {
        for (int i = 0; i < 3; ++i) workers_.emplace_back([this] { load(); });
    } catch (...) {
        stop();
        throw;
    }
}

ArtworkLoader::~ArtworkLoader() { stop(); }

void ArtworkLoader::stop() {
    {
        std::scoped_lock lock(mutex_);
        stopping_ = true;
    }
    ready_.notify_all();
    for (auto& worker : workers_) worker.join();
}

ArtworkLoader::Generation ArtworkLoader::reset(HWND view) {
    std::scoped_lock lock(mutex_);
    view_ = view;
    requests_.clear();
    startedRows_.clear();
    complete_.clear();
    return ++generation_;
}

void ArtworkLoader::cancel(Generation generation) {
    std::scoped_lock lock(mutex_);
    if (generation_ != generation) return;
    view_ = nullptr;
    requests_.clear();
    startedRows_.clear();
    complete_.clear();
}

void ArtworkLoader::setViewport(Generation generation, int size, const std::vector<LibraryItem>& items,
                                const std::vector<int>& rowsInPriorityOrder) {
    {
        std::scoped_lock lock(mutex_);
        if (!view_ || generation != generation_) return;
        requests_.clear();
        for (int row : rowsInPriorityOrder) {
            if (startedRows_.count(row)) continue;
            const auto& item = items[row];
            requests_.push_back({generation, row, size, {item.target, {}, item.coverId}});
        }
    }
    ready_.notify_all();
}

std::vector<Thumbnail> ArtworkLoader::take(Generation generation) {
    std::scoped_lock lock(mutex_);
    if (generation != generation_) return {};
    for (const auto& thumbnail : complete_) startedRows_.erase(thumbnail.row);
    return std::exchange(complete_, {});
}

void ArtworkLoader::requestTrack(Id key, bool priority) {
    {
        std::scoped_lock lock(mutex_);
        if (stopping_ || !key) return;
        if (pendingTracks_.count(key)) {
            if (!priority) return;
            auto queued = std::find_if(tracks_.begin(), tracks_.end(),
                [key](const TrackRequest& request) { return request.key == key; });
            if (queued == tracks_.end()) return;
            tracks_.erase(queued);
        } else {
            if (tracks_.size() >= 64) {
                if (!priority) return;
                pendingTracks_.erase(tracks_.back().key);
                tracks_.pop_back();
            }
            pendingTracks_.insert(key);
        }
        if (priority) tracks_.push_front({key, true});
        else tracks_.push_back({key, false});
    }
    ready_.notify_one();
}

std::string ArtworkLoader::trackArtwork(const Track& track) {
    auto data = store_.artwork(track.server, songArtwork(track));
    if (data.empty() && !track.albumId.empty()) data = store_.artwork(track.server, albumArtwork(store_, track));
    return data;
}

void ArtworkLoader::notifyPlayer(UINT message, Id key) {
    const auto bits = static_cast<std::uint64_t>(key);
    if (message) PostMessageW(player_, message, static_cast<DWORD>(bits), static_cast<LPARAM>(bits >> 32));
}

void ArtworkLoader::stageCover(const Track& track) {
    try {
        if (store_.stageCover(track, songArtwork(track), albumArtwork(store_, track)))
            notifyPlayer(coverReadyMessage, track.key);
    } catch (const std::exception&) {
        // A cover write failure must not interrupt playback or enqueueing.
    }
}

void ArtworkLoader::loadTrack(Id key) {
    auto track = store_.track(key);
    if (!track.key || !store_.networkAvailable(track.server)) return;
    auto connection = http_.connect(store_.server(track.server));
    try {
        const auto item = songArtwork(track);
        if (store_.artwork(track.server, item).empty()) {
            auto data = loadArtwork(store_, *connection, track.server, item);
            if (data.empty() && !track.albumId.empty()) {
                auto album = albumArtwork(store_, track);
                data = loadArtwork(store_, *connection, track.server, album);
            }
            if (!data.empty()) stageCover(track);
        }
    } catch (const ConnectionError&) {
        store_.connectionFailed(track.server);
    } catch (const std::exception&) {}
    if (!store_.lyrics(key, true) && store_.networkAvailable(track.server)) {
        try {
            auto lyrics = connection->lyrics(track);
            store_.saveLyrics(key, lyrics);
            if (!lyrics.plain.empty()) notifyPlayer(lyricsReadyMessage, key);
        } catch (const ConnectionError&) {
            store_.connectionFailed(track.server);
        } catch (const std::exception&) {}
    }
}

void ArtworkLoader::load() {
    ComApartment apartment;
    Com<IWICImagingFactory> factory;
    if (apartment) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, reinterpret_cast<void**>(factory.out()));
    for (;;) {
        Request request;
        Id track = 0;
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !requests_.empty() || !tracks_.empty(); });
            if (stopping_) return;
            if (!tracks_.empty() && (tracks_.front().priority || requests_.empty())) {
                track = tracks_.front().key;
                tracks_.pop_front();
            } else {
                request = std::move(requests_.front());
                requests_.pop_front();
                startedRows_.insert(request.row);
            }
        }
        if (track) {
            try { loadTrack(track); } catch (const std::exception&) {}
            std::scoped_lock lock(mutex_);
            pendingTracks_.erase(track);
            continue;
        }
        Thumbnail thumbnail{request.row, {}};
        const Id server = request.item.target.server;
        try {
            if (factory.get()) {
                auto bytes = store_.artwork(server, request.item);
                if (bytes.empty() && store_.networkAvailable(server) && !localLocation(request.item.target)) {
                    auto api = http_.connect(store_.server(server));
                    bytes = loadArtwork(store_, *api, server, request.item);
                }
                thumbnail.pixels = decode(factory.get(), bytes, request.size);
            }
        } catch (const ConnectionError&) {
            store_.connectionFailed(server);
        } catch (const std::exception&) {
            // A missing or unreachable cover still leaves a usable tile.
        }
        std::scoped_lock lock(mutex_);
        if (!stopping_ && view_ && generation_ == request.generation) {
            complete_.push_back(std::move(thumbnail));
            PostMessageW(view_, readyMessage, 0, 0);
        }
    }
}

const wchar_t* artworkExtension(std::string_view data) {
    if (data.substr(0, 3) == "\xff\xd8\xff") return L"jpg";
    if (data.substr(0, 8) == "\x89PNG\r\n\x1a\n") return L"png";
    if (data.substr(0, 6) == "GIF87a" || data.substr(0, 6) == "GIF89a") return L"gif";
    if (data.substr(0, 2) == "BM") return L"bmp";
    if (data.size() >= 12 && data.substr(0, 4) == "RIFF" && data.substr(8, 4) == "WEBP") return L"webp";
    return nullptr;
}

HBITMAP thumbnailBitmap(int size, const std::vector<std::uint32_t>& pixels) {
    if (pixels.size() != static_cast<std::size_t>(size) * size) return nullptr;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* data = nullptr;
    auto bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &data, nullptr, 0);
    if (bitmap) std::copy(pixels.begin(), pixels.end(), static_cast<std::uint32_t*>(data));
    return bitmap;
}

} // namespace tangerine
