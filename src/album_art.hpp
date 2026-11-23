#pragma once

#include <memory>

namespace tangerine {

class ArtworkLoader;
class Playback;
class Store;

class AlbumArtRegistration {
public:
    AlbumArtRegistration(void* services, Store& store, const Playback& playback, ArtworkLoader& artwork);
    ~AlbumArtRegistration();
    AlbumArtRegistration(const AlbumArtRegistration&) = delete;
    AlbumArtRegistration& operator=(const AlbumArtRegistration&) = delete;

private:
    struct Registration;
    std::unique_ptr<Registration> registration_;
};

} // namespace tangerine
