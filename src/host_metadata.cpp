#include "playback.hpp"
#include "artwork.hpp"
#include "host_metadata.hpp"

#include <array>
#include <cstring>
#include <functional>

namespace tangerine {

std::optional<std::string> trackMetadata(Store& store, ArtworkLoader& artwork,
                                         const Track& track, const wchar_t* field) {
    if (!field) return {};
    auto is = [&](const wchar_t* name) { return _wcsicmp(field, name) == 0; };
    if (is(L"title")) return track.title.empty() ? track.id : track.title;
    if (is(L"streamtitle")) return playlistTitle(track);
    if (is(L"artist") || is(L"streamartist")) return track.artist;
    if (is(L"album") || is(L"streamalbum")) return track.album;
    if (is(L"albumartist") || is(L"album artist")) return track.albumArtist;
    if (is(L"genre")) return track.genre;
    if (is(L"year") && track.year) return std::to_string(track.year);
    if (is(L"track") && track.trackNumber) return std::to_string(track.trackNumber);
    if ((is(L"disc") || is(L"discnumber")) && track.discNumber) return std::to_string(track.discNumber);
    if (is(L"length") && track.duration) return std::to_string(Id(track.duration) * 1000);
    if (is(L"length_seconds") && track.duration) return std::to_string(track.duration);
    if (is(L"lyrics") || is(L"unsyncedlyrics") || is(L"unsynced lyrics")) {
        const auto lyrics = store.lyrics(track.key);
        if (lyrics && !lyrics->plain.empty()) return lyrics->plain;
        if (!lyrics && store.networkAvailable(track.server)) artwork.requestTrack(track.key, true);
    }
    return {};
}

namespace {

// Wasabi interfaces share a single virtual entry point. The message IDs and
// parameter layouts follow api_service, waServiceFactory, api_memmgr,
// svc_albumArtProvider and svc_metaTag in the SDK.
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
constexpr GUID tagGuid{0x974abd1d, 0x2364, 0x45f8, {0xbc, 0x59, 0x34, 0x5b, 0xaa, 0x71, 0x29, 0x73}};
constexpr unsigned long tagServiceType = ('m' << 24) | ('t' << 16) | ('t' << 8) | 'g';

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

// svc_metaTag readers are per-file. The host can ask either the factory to
// release an unused reader or the opened reader to close itself.
class TagReader final : public Dispatchable {
public:
    TagReader(Store& store, const Playback& playback, ArtworkLoader& artwork)
        : store_(store), playback_(playback), artwork_(artwork) {}

    int dispatch(int message, void* result, void** args, int count) override {
        try {
            switch (message) {
            case 10: return returnValue(result, L"Tangerine Stream");
            case 20: return returnValue(result, tagGuid);
            case 30: return returnValue(result, 4); // METATAG_CACHE_DB
            case 40: case 50: {
                if (count != 1) return 0;
                auto filename = *static_cast<const wchar_t**>(args[0]);
                auto key = filename ? playback_.key(filename) : 0;
                if (message == 40) return returnValue(result, int(key != 0));
                track_ = key ? store_.track(key) : Track{};
                return returnValue(result, track_.key ? 0 : 4);
            }
            case 60: delete this; return 1;
            case 100: {
                if (count != 2) return 0;
                static constexpr const wchar_t* fields[] = {L"title", L"artist", L"album", L"albumartist",
                    L"genre", L"year", L"track", L"disc", L"length", L"lyrics", L"streamtitle"};
                auto index = *static_cast<int*>(args[0]);
                auto datatype = *static_cast<int**>(args[1]);
                if (datatype) *datatype = 0; // METATYPE_STRING
                return returnValue(result, index >= 0 && size_t(index) < std::size(fields) ? fields[index] : nullptr);
            }
            case 110: case 120: {
                if (count != (message == 110 ? 2 : 4) || !track_.key) return returnValue(result, 1);
                auto field = *static_cast<const wchar_t**>(args[0]);
                auto value = trackMetadata(store_, artwork_, track_, field);
                if (!value) return returnValue(result, 2); // METATAG_UNKNOWN_TAG
                auto text = wide(*value);
                auto bytes = (text.size() + 1) * sizeof(wchar_t);
                if (message == 110) {
                    auto size = *static_cast<size_t**>(args[1]);
                    if (!size) return returnValue(result, 1);
                    *size = bytes;
                } else {
                    auto buffer = *static_cast<unsigned char**>(args[1]);
                    auto capacity = *static_cast<int*>(args[2]);
                    auto datatype = *static_cast<int*>(args[3]);
                    if (datatype != 0 || !buffer || capacity < 0 || size_t(capacity) < bytes)
                        return returnValue(result, 1);
                    std::memcpy(buffer, text.c_str(), bytes);
                }
                return returnValue(result, 0);
            }
            case 130: return returnValue(result, 1); // read-only
            }
        } catch (...) { return returnValue(result, message == 40 ? 0 : 1); }
        return 0;
    }
private:
    Store& store_;
    const Playback& playback_;
    ArtworkLoader& artwork_;
    Track track_;
};

class Factory final : public Dispatchable {
public:
    Factory(unsigned long type, const char* name, GUID guid, std::function<Dispatchable*()> acquire,
            void (*release)(Dispatchable*) = nullptr)
        : type_(type), name_(name), guid_(guid), acquire_(std::move(acquire)), release_(release) {}

    int dispatch(int message, void* result, void** args, int count) override {
        switch (message) {
        case 100: return returnValue(result, type_);
        case 200: return returnValue(result, name_);
        case 210: return returnValue(result, guid_);
        case 300:
            try { return returnValue(result, static_cast<void*>(acquire_())); }
            catch (...) { return returnValue<void*>(result, nullptr); }
        case 310:
            if (release_ && count == 1) release_(*static_cast<Dispatchable**>(args[0]));
            return returnValue(result, 1);
        case 301:
        case 600: return returnValue(result, 1);
        case 500: return returnValue<const wchar_t*>(result, nullptr);
        default: return 0;
        }
    }

private:
    unsigned long type_;
    const char* name_;
    GUID guid_;
    std::function<Dispatchable*()> acquire_;
    void (*release_)(Dispatchable*);
};

} // namespace

struct HostMetadataServices::Registration {
    Registration(Dispatchable& services, Store& store, const Playback& playback, ArtworkLoader& artwork)
        : services(services), provider(store, playback, artwork),
          factory(artworkServiceType, "Tangerine Stream Album Art", artworkGuid, [this] { return &provider; }),
          tagFactory(tagServiceType, "Tangerine Stream Metadata", tagGuid,
              [&store, &playback, &artwork] { return new TagReader(store, playback, artwork); },
              [](Dispatchable* reader) { delete static_cast<TagReader*>(reader); }) {
        services.call(10, 0, static_cast<Dispatchable*>(&tagFactory));
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
        services.call(20, 0, static_cast<Dispatchable*>(&tagFactory));
        if (registered) services.call(20, 0, static_cast<Dispatchable*>(&factory));
        provider.setMemory(nullptr);
        if (memory) memoryFactory->call(310, 0, memory);
    }

    Dispatchable& services;
    Provider provider;
    Factory factory, tagFactory;
    Dispatchable* memoryFactory = nullptr;
    Dispatchable* memory = nullptr;
    bool registered = false;
};

HostMetadataServices::HostMetadataServices(void* services, Store& store, const Playback& playback,
                                           ArtworkLoader& artwork) {
    if (reinterpret_cast<UINT_PTR>(services) > 1)
        registration_ = std::make_unique<Registration>(*static_cast<Dispatchable*>(services), store, playback, artwork);
}

HostMetadataServices::~HostMetadataServices() = default;

} // namespace tangerine
