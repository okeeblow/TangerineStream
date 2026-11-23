#include "artwork.hpp"
#include "server.hpp"

#include <wincodec.h>
#include <algorithm>
#include <utility>

namespace tangerine {
namespace {

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

ArtworkLoader::ArtworkLoader(Store& store) : store_(store) {
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
    complete_.clear();
    return ++generation_;
}

void ArtworkLoader::cancel(Generation generation) {
    std::scoped_lock lock(mutex_);
    if (generation_ != generation) return;
    view_ = nullptr;
    requests_.clear();
    complete_.clear();
}

bool ArtworkLoader::request(Generation generation, Id server, int row, int size, const LibraryItem& item) {
    {
        std::scoped_lock lock(mutex_);
        if (!view_ || generation != generation_ || requests_.size() >= 48) return false;
        requests_.push_back({generation, server, row, size, item});
    }
    ready_.notify_one();
    return true;
}

std::vector<Thumbnail> ArtworkLoader::take(Generation generation) {
    std::scoped_lock lock(mutex_);
    if (generation != generation_) return {};
    return std::exchange(complete_, {});
}

void ArtworkLoader::load() {
    ComApartment apartment;
    Com<IWICImagingFactory> factory;
    if (apartment) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, reinterpret_cast<void**>(factory.out()));
    Generation generation = 0;
    std::unique_ptr<ServerApi> api;
    for (;;) {
        Request request;
        {
            std::unique_lock lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !requests_.empty(); });
            if (stopping_) return;
            request = std::move(requests_.front());
            requests_.pop_front();
        }
        Thumbnail thumbnail{request.row, {}};
        try {
            if (!api || generation != request.generation) {
                api = connect(store_.server(request.server));
                generation = request.generation;
            }
            if (factory.get()) {
                auto bytes = api->cover(request.item, request.size * 2);
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
