#include "playback.hpp"
#include "artwork.hpp"
#include "album_art.hpp"

#include <array>
#include <cstring>

namespace tangerine {
namespace {

// Wasabi interfaces share a single virtual entry point. The message IDs and
// parameter layouts follow the SDK's api_service, waServiceFactory, api_memmgr
// and svc_albumArtProvider headers.
struct Dispatchable {
    virtual int dispatch(int message, void* result, void** args, int count) = 0;

    template<class R, class... Args>
    R call(int message, R result, Args... args) {
        std::array<void*, sizeof...(Args)> params{&args...};
        dispatch(message, &result, params.data(), static_cast<int>(params.size()));
        return result;
    }

    template<class T>
    static int returnValue(void* result, T value) {
        if (result) *static_cast<T*>(result) = value;
        return 1;
    }
};

constexpr GUID memoryGuid{0x000cf46e, 0x4df6, 0x4a43, {0xbb, 0xe7, 0x40, 0xe7, 0xa3, 0xea, 0x02, 0xed}};
constexpr GUID artworkGuid{0x510e6927, 0x5f57, 0x46cb, {0x88, 0x9c, 0x2a, 0x90, 0x03, 0x6f, 0x84, 0xac}};
constexpr unsigned long artworkServiceType = ('a' << 16) | ('a' << 8) | 'p';

class Provider final : public Dispatchable {
public:
    Provider(Store& store, const Playback& playback, ArtworkLoader& artwork)
        : store_(store), playback_(playback), artwork_(artwork) {}

    void setMemory(Dispatchable* memory) {
        std::scoped_lock lock(mutex_);
        memory_ = memory;
    }

    int dispatch(int message, void* result, void** args, int count) override {
        try {
            switch (message) {
            case 0: return returnValue(result, 1); // Database provider
            case 10:
                if (count != 5) return 0;
                return returnValue(result, get(*static_cast<const wchar_t**>(args[0]),
                    *static_cast<const wchar_t**>(args[1]), *static_cast<void***>(args[2]),
                    *static_cast<size_t**>(args[3]), *static_cast<wchar_t***>(args[4])));
            case 20: {
                if (count != 1) return 0;
                const auto filename = *static_cast<const wchar_t**>(args[0]);
                return returnValue(result, filename && playback_.key(filename) != 0);
            }
            case 30:
            case 40: return returnValue(result, 2); // Read-only
            }
        } catch (...) {
            if (message == 10) return returnValue(result, 1);
        }
        return 0;
    }

private:
    Store& store_;
    const Playback& playback_;
    ArtworkLoader& artwork_;
    std::mutex mutex_;
    Dispatchable* memory_ = nullptr;

    int get(const wchar_t* filename, const wchar_t* type, void** bits, size_t* length, wchar_t** format) {
        if (!bits || !length || !format) return 1;
        *bits = nullptr;
        *length = 0;
        *format = nullptr;
        if (!filename || (type && *type && _wcsicmp(type, L"cover") != 0 &&
                         _wcsicmp(type, L"playing") != 0)) return 1;
        std::scoped_lock lock(mutex_);
        if (!memory_) return 1;
        const auto key = playback_.key(filename);
        if (!key) return 1;
        const auto track = store_.track(key);
        if (!track.key) return 1;
        auto data = artwork_.trackArtwork(track);
        const auto* extension = artworkExtension(data);
        if (!extension) {
            if (store_.networkAvailable(track.server)) artwork_.requestTrack(key, true);
            return 1;
        }
        const auto extensionSize = (std::wcslen(extension) + 1) * sizeof(wchar_t);
        auto buffer = memory_->call<void*>(0, nullptr, data.size());
        if (!buffer) return 1;
        auto mime = memory_->call<void*>(0, nullptr, extensionSize);
        if (!mime) {
            memory_->call(10, 0, buffer);
            return 1;
        }
        std::memcpy(buffer, data.data(), data.size());
        std::memcpy(mime, extension, extensionSize);
        *bits = buffer;
        *length = data.size();
        *format = static_cast<wchar_t*>(mime);
        return 0;
    }
};

class Factory final : public Dispatchable {
public:
    explicit Factory(Provider& provider) : provider_(provider) {}

    int dispatch(int message, void* result, void**, int) override {
        switch (message) {
        case 100: return returnValue(result, artworkServiceType);
        case 200: return returnValue(result, "Tangerine Stream Album Art");
        case 210: return returnValue(result, artworkGuid);
        case 300: return returnValue(result, static_cast<void*>(&provider_));
        case 301:
        case 310:
        case 600: return returnValue(result, 1);
        case 500: return returnValue<const wchar_t*>(result, nullptr);
        default: return 0;
        }
    }

private:
    Provider& provider_;
};

} // namespace

struct AlbumArtRegistration::Registration {
    Registration(Dispatchable& services, Store& store, const Playback& playback, ArtworkLoader& artwork)
        : services(services), provider(store, playback, artwork), factory(provider) {
        memoryFactory = services.call<Dispatchable*>(50, nullptr, memoryGuid);
        if (!memoryFactory) return;
        memory = memoryFactory->call<Dispatchable*>(300, nullptr, FALSE);
        if (!memory) return;
        provider.setMemory(memory);
        Dispatchable* service = &factory;
        void* args[] = {&service};
        int result = 0;
        registered = services.dispatch(10, &result, args, 1) != 0;
    }

    ~Registration() {
        if (registered) services.call(20, 0, static_cast<Dispatchable*>(&factory));
        provider.setMemory(nullptr);
        if (memory) memoryFactory->call(310, 0, memory);
    }

    Dispatchable& services;
    Provider provider;
    Factory factory;
    Dispatchable* memoryFactory = nullptr;
    Dispatchable* memory = nullptr;
    bool registered = false;
};

AlbumArtRegistration::AlbumArtRegistration(void* services, Store& store, const Playback& playback,
                                           ArtworkLoader& artwork) {
    if (reinterpret_cast<UINT_PTR>(services) > 1)
        registration_ = std::make_unique<Registration>(*static_cast<Dispatchable*>(services), store, playback, artwork);
}

AlbumArtRegistration::~AlbumArtRegistration() = default;

} // namespace tangerine
