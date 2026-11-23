#include "playback.hpp"
#include "artwork.hpp"
#include "host_metadata.hpp"

#include "sdk_services.hpp"
#include <api/service/waservicefactory.h>
#include <api/memmgr/api_memmgr.h>
#include <AlbumArt/svc_albumArtProvider.h>
#include <Metadata/svc_metatag.h>
#include <cstring>
#include <functional>

namespace tangerine {

std::optional<std::string> trackMetadata(Store& store, ArtworkLoader& artwork,
                                         const Track& track, const wchar_t* field) {
    if (!field) return {};
    auto is = [&](const wchar_t* name) { return _wcsicmp(field, name) == 0; };
    if (is(L"type")) return "0"; // Audio, per the input plug-in metadata contract.
    if (is(L"mime")) return track.mime;
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

template<class T>
int returnValue(void* result, T value) {
    if (result) *static_cast<T*>(result) = value;
    return 1;
}

constexpr GUID artworkGuid{0x510e6927, 0x5f57, 0x46cb, {0x88, 0x9c, 0x2a, 0x90, 0x03, 0x6f, 0x84, 0xac}};
constexpr GUID tagGuid{0x974abd1d, 0x2364, 0x45f8, {0xbc, 0x59, 0x34, 0x5b, 0xaa, 0x71, 0x29, 0x73}};

class Provider final : public svc_albumArtProvider {
public:
    Provider(Store& store, const Playback& playback, ArtworkLoader& artwork)
        : store_(store), playback_(playback), artwork_(artwork) {}

    void setMemory(api_memmgr* memory) {
        std::scoped_lock lock(mutex_);
        memory_ = memory;
    }

    int _dispatch(int message, void* result, void** args, int count) override {
        try {
            switch (message) {
            case SVC_ALBUMARTPROVIDER_PROVIDERTYPE: return returnValue(result, 1); // Database provider
            case SVC_ALBUMARTPROVIDER_GETALBUMARTDATA:
                if (count != 5) return 0;
                return returnValue(result, get(*static_cast<const wchar_t**>(args[0]),
                    *static_cast<const wchar_t**>(args[1]), *static_cast<void***>(args[2]),
                    *static_cast<size_t**>(args[3]), *static_cast<wchar_t***>(args[4])));
            case SVC_ALBUMARTPROVIDER_ISMINE: {
                if (count != 1) return 0;
                const auto filename = *static_cast<const wchar_t**>(args[0]);
                return returnValue(result, filename && playback_.key(filename) != 0);
            }
            case SVC_ALBUMARTPROVIDER_SETALBUMARTDATA:
            case SVC_ALBUMARTPROVIDER_DELETEALBUMART: return returnValue(result, 2); // Read-only
            }
        } catch (...) {
            if (message == SVC_ALBUMARTPROVIDER_GETALBUMARTDATA) return returnValue(result, 1);
        }
        return 0;
    }

private:
    Store& store_;
    const Playback& playback_;
    ArtworkLoader& artwork_;
    std::mutex mutex_;
    api_memmgr* memory_ = nullptr;

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
        auto buffer = memory_->sysMalloc(data.size());
        if (!buffer) return 1;
        auto mime = memory_->sysMalloc(extensionSize);
        if (!mime) {
            memory_->sysFree(buffer);
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
class TagReader final : public svc_metaTag {
public:
    TagReader(Store& store, const Playback& playback, ArtworkLoader& artwork)
        : store_(store), playback_(playback), artwork_(artwork) {}

    int _dispatch(int message, void* result, void** args, int count) override {
        try {
            switch (message) {
            case SVC_METATAG_GETNAME: return returnValue(result, L"Tangerine Stream");
            case SVC_METATAG_GETGUID: return returnValue(result, tagGuid);
            case SVC_METATAG_GETFLAGS: return returnValue(result, 4); // METATAG_CACHE_DB
            case SVC_METATAG_ISOURFILE: case SVC_METATAG_OPEN: {
                if (count != 1) return 0;
                auto filename = *static_cast<const wchar_t**>(args[0]);
                auto key = filename ? playback_.key(filename) : 0;
                if (message == SVC_METATAG_ISOURFILE) return returnValue(result, int(key != 0));
                track_ = key ? store_.track(key) : Track{};
                return returnValue(result, track_.key ? 0 : 4);
            }
            case SVC_METATAG_CLOSE: delete this; return 1;
            case SVC_METATAG_ENUMTAGS: {
                if (count != 2) return 0;
                static constexpr const wchar_t* fields[] = {L"title", L"artist", L"album", L"albumartist",
                    L"genre", L"year", L"track", L"disc", L"length", L"lyrics", L"streamtitle"};
                auto index = *static_cast<int*>(args[0]);
                auto datatype = *static_cast<int**>(args[1]);
                if (datatype) *datatype = 0; // METATYPE_STRING
                return returnValue(result, index >= 0 && size_t(index) < std::size(fields) ? fields[index] : nullptr);
            }
            case SVC_METATAG_GETTAGSIZE: case SVC_METATAG_GETMETADATA: {
                if (count != (message == SVC_METATAG_GETTAGSIZE ? 2 : 4) || !track_.key) return returnValue(result, 1);
                auto field = *static_cast<const wchar_t**>(args[0]);
                auto value = trackMetadata(store_, artwork_, track_, field);
                if (!value) return returnValue(result, 2); // METATAG_UNKNOWN_TAG
                auto text = wide(*value);
                auto bytes = (text.size() + 1) * sizeof(wchar_t);
                if (message == SVC_METATAG_GETTAGSIZE) {
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
            case SVC_METATAG_SETMETADATA: return returnValue(result, 1); // read-only
            }
        } catch (...) { return returnValue(result, message == SVC_METATAG_ISOURFILE ? 0 : 1); }
        return 0;
    }
private:
    Store& store_;
    const Playback& playback_;
    ArtworkLoader& artwork_;
    Track track_;
};

class Factory final : public waServiceFactory {
public:
    Factory(unsigned long type, const char* name, GUID guid, std::function<Dispatchable*()> acquire,
            void (*release)(Dispatchable*) = nullptr)
        : type_(type), name_(name), guid_(guid), acquire_(std::move(acquire)), release_(release) {}

    int _dispatch(int message, void* result, void** args, int count) override {
        switch (message) {
        case WASERVICEFACTORY_GETSERVICETYPE: return returnValue(result, type_);
        case WASERVICEFACTORY_GETSERVICENAME: return returnValue(result, name_);
        case WASERVICEFACTORY_GETGUID: return returnValue(result, guid_);
        case WASERVICEFACTORY_GETINTERFACE:
            try { return returnValue(result, static_cast<void*>(acquire_())); }
            catch (...) { return returnValue<void*>(result, nullptr); }
        case WASERVICEFACTORY_RELEASEINTERFACE:
            if (release_ && count == 1) release_(*static_cast<Dispatchable**>(args[0]));
            return returnValue(result, 1);
        case WASERVICEFACTORY_SUPPORTNONLOCKINGGETINTERFACE:
        case WASERVICEFACTORY_SERVICENOTIFY: return returnValue(result, 1);
        case WASERVICEFACTORY_GETTESTSTRING: return returnValue<const wchar_t*>(result, nullptr);
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
    Registration(api_service& services, Store& store, const Playback& playback, ArtworkLoader& artwork)
        : services(services), provider(store, playback, artwork),
          factory(svc_albumArtProvider::getServiceType(), "Tangerine Stream Album Art", artworkGuid, [this] { return &provider; }),
          tagFactory(svc_metaTag::getServiceType(), "Tangerine Stream Metadata", tagGuid,
              [&store, &playback, &artwork] { return new TagReader(store, playback, artwork); },
              [](Dispatchable* reader) { delete static_cast<TagReader*>(reader); }) {
        services.service_register(&tagFactory);
        memoryFactory = services.service_getServiceByGuid(memMgrApiServiceGuid);
        if (!memoryFactory) return;
        memory = static_cast<api_memmgr*>(memoryFactory->getInterface(FALSE));
        if (!memory) return;
        provider.setMemory(memory);
        services.service_register(&factory);
    }

    ~Registration() {
        services.service_deregister(&tagFactory);
        if (memory) {
            services.service_deregister(&factory);
            provider.setMemory(nullptr);
            memoryFactory->releaseInterface(memory);
        }
    }

    api_service& services;
    Provider provider;
    Factory factory, tagFactory;
    waServiceFactory* memoryFactory = nullptr;
    api_memmgr* memory = nullptr;
};

HostMetadataServices::HostMetadataServices(void* services, Store& store, const Playback& playback,
                                           ArtworkLoader& artwork) {
    if (reinterpret_cast<UINT_PTR>(services) > 1)
        registration_ = std::make_unique<Registration>(*static_cast<api_service*>(services), store, playback, artwork);
}

HostMetadataServices::~HostMetadataServices() = default;

} // namespace tangerine
