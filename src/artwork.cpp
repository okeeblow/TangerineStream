#include "artwork.hpp"
#include "server.hpp"

#include <wincodec.h>
#include <algorithm>
#include <utility>

namespace tangerine {
namespace {

constexpr int playbackCoverSize = 640;
constexpr int artworkWorkers = 6;
constexpr unsigned maxBackgroundLoads = 2;

LibraryItem songArtwork(const Track& track) {
    return {{BrowseKind::song, track.id, track.title}, track.artist, track.coverId};
}

LibraryItem albumArtwork(Store& store, const Track& track) {
    return {{BrowseKind::albumTracks, track.albumId, track.album}, track.artist,
        store.albumCover(track.server, track.albumId)};
}

std::string loadArtwork(Store& store, HttpClient& http, Id server, const LibraryItem& item, int targetEdgePx) {
    auto cached = store.artwork(server, item);
    if (cached.requestedMaxEdgePx < targetEdgePx && !localLocation(item.target) && store.networkAvailable(server)) {
        try {
            auto api = http.connect(store.server(server));
            auto data = api->cover(item, targetEdgePx);
            if (artworkExtension(data)) {
                store.saveArtwork(server, item, targetEdgePx, data);
                return data;
            }
        } catch (const ConnectionError&) {
            store.connectionFailed(server);
        } catch (const Error&) {
            if (cached.data.empty()) throw;
        }
    }
    return std::move(cached.data);
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
    IWICBitmapSource* source = frame.get();
    if (longest != size) {
        if (FAILED(factory->CreateBitmapScaler(scaler.out())) ||
            FAILED(scaler->Initialize(source, width, height, WICBitmapInterpolationModeFant))) return {};
        source = scaler.get();
    }
    if (FAILED(factory->CreateFormatConverter(converter.out())) ||
        FAILED(converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) return {};
    std::vector<std::uint32_t> scaled(width * height);
    if (FAILED(converter->CopyPixels(nullptr, width * sizeof(std::uint32_t),
                    static_cast<UINT>(scaled.size() * sizeof(std::uint32_t)),
                    reinterpret_cast<BYTE*>(scaled.data())))) return {};
    if (width == size && height == size) return scaled;
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
        for (int i = 0; i < artworkWorkers; ++i) workers_.emplace_back([this] { load(); });
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
    visibleRows_.clear();
    complete_.clear();
    ready_.notify_all();
    return ++generation_;
}

void ArtworkLoader::cancel(Generation generation) {
    std::scoped_lock lock(mutex_);
    if (generation_ != generation) return;
    view_ = nullptr;
    requests_.clear();
    startedRows_.clear();
    visibleRows_.clear();
    complete_.clear();
    ready_.notify_all();
}

void ArtworkLoader::setViewport(Generation generation, int size, const std::vector<LibraryItem>& items,
                                const std::vector<int>& rowsInPriorityOrder, std::size_t visibleCount) {
    {
        std::scoped_lock lock(mutex_);
        if (!view_ || generation != generation_) return;
        requests_.clear();
        visibleRows_.clear();
        visibleRows_.insert(rowsInPriorityOrder.begin(), rowsInPriorityOrder.begin() + visibleCount);
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
    for (const auto& thumbnail : complete_) {
        startedRows_.erase(thumbnail.row);
        visibleRows_.erase(thumbnail.row);
    }
    if (!complete_.empty() && visibleRows_.empty()) ready_.notify_all();
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
    auto cached = store_.artwork(track.server, songArtwork(track));
    if (cached.data.empty() && !track.albumId.empty())
        cached = store_.artwork(track.server, albumArtwork(store_, track));
    if (cached.requestedMaxEdgePx < playbackCoverSize && store_.networkAvailable(track.server))
        requestTrack(track.key, true);
    return std::move(cached.data);
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
    try {
        auto data = loadArtwork(store_, http_, track.server, songArtwork(track), playbackCoverSize);
        if (data.empty() && !track.albumId.empty())
            data = loadArtwork(store_, http_, track.server, albumArtwork(store_, track), playbackCoverSize);
        if (!data.empty()) stageCover(track);
    } catch (const std::exception&) {}
    if (!store_.lyrics(key, true) && store_.networkAvailable(track.server)) {
        try {
            auto lyrics = http_.connect(store_.server(track.server))->lyrics(track);
            store_.saveLyrics(key, lyrics);
            if (!lyrics.plain.empty()) notifyPlayer(lyricsReadyMessage, key);
        } catch (const ConnectionError&) {
            store_.connectionFailed(track.server);
        } catch (const std::exception&) {}
    }
}

bool ArtworkLoader::workAvailable() const {
    if (!tracks_.empty() && tracks_.front().priority) return true;
    if (!requests_.empty() && visibleRows_.count(requests_.front().row)) return true;
    return visibleRows_.empty() && backgroundLoads_ < maxBackgroundLoads &&
           (!requests_.empty() || !tracks_.empty());
}

void ArtworkLoader::load() {
    ComApartment apartment;
    Com<IWICImagingFactory> factory;
    if (apartment) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, reinterpret_cast<void**>(factory.out()));
    for (;;) {
        Request request;
        Id track = 0;
        bool background;
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || workAvailable(); });
            if (stopping_) return;
            if (!tracks_.empty() && (tracks_.front().priority || requests_.empty())) {
                track = tracks_.front().key;
                background = !tracks_.front().priority;
                tracks_.pop_front();
            } else {
                request = std::move(requests_.front());
                requests_.pop_front();
                startedRows_.insert(request.row);
                background = !visibleRows_.count(request.row);
            }
            if (background) ++backgroundLoads_;
        }
        if (track) {
            try { loadTrack(track); } catch (const std::exception&) {}
            std::scoped_lock lock(mutex_);
            pendingTracks_.erase(track);
        } else {
            Thumbnail thumbnail{request.row, {}};
            const Id server = request.item.target.server;
            try {
                if (factory.get()) {
                    auto bytes = loadArtwork(store_, http_, server, request.item, request.size);
                    thumbnail.pixels = decode(factory.get(), bytes, request.size);
                }
            } catch (const std::exception&) {
                // A missing or unreachable cover still leaves a usable tile.
            }
            std::scoped_lock lock(mutex_);
            if (!stopping_ && view_ && generation_ == request.generation) {
                complete_.push_back(std::move(thumbnail));
                PostMessageW(view_, readyMessage, 0, 0);
            }
        }
        if (background) {
            std::scoped_lock lock(mutex_);
            --backgroundLoads_;
            ready_.notify_one();
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
