#pragma once

#include <memory>
#include <optional>
#include <string>

namespace tangerine {

class ArtworkLoader;
class Playback;
class Store;
struct Track;

std::optional<std::string> trackMetadata(Store& store, ArtworkLoader& artwork,
                                         const Track& track, const wchar_t* field);

class HostMetadataServices {
public:
    HostMetadataServices(void* services, Store& store, const Playback& playback, ArtworkLoader& artwork);
    ~HostMetadataServices();
    HostMetadataServices(const HostMetadataServices&) = delete;
    HostMetadataServices& operator=(const HostMetadataServices&) = delete;
    void artworkChanged(const wchar_t* filename);

private:
    struct Registration;
    std::unique_ptr<Registration> registration_;
};

} // namespace tangerine
