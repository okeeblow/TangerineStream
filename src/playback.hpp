#pragma once

#include "server.hpp"
#include "store.hpp"
#include <atomic>

namespace tangerine {

class Playback {
public:
    Playback(Store& store, const std::filesystem::path& profile);
    std::wstring playlistFile(const Track& track) const;
    Id key(std::wstring_view filename) const;

private:
    std::filesystem::path reference(const Track& track) const;
    std::filesystem::path items_;
    std::wstring legacy_;
};

bool meteredInternet();
void downloadAudio(Store& store, HttpClient& http, const Track& track, const Server& profile, ServerApi& api,
                   bool metered, const std::atomic_bool& stopping, const InputExtensions& inputs,
                   unsigned excluded = 0, bool (*currentCost)() = meteredInternet);

} // namespace tangerine
