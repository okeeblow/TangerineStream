#pragma once

#include "network/server.hpp"
#include "playback/input_bridge.hpp"
#include "storage/store.hpp"
#include <atomic>
#include <functional>

namespace tangerine {

class Playback {
public:
    Playback(Store& store, const std::filesystem::path& profile);
    std::wstring playlistFile(const Track& track, bool ansiHost = false) const;
    Id key(std::wstring_view filename) const;

private:
    std::filesystem::path reference(const Track& track) const;
    std::filesystem::path items_;
    std::wstring legacy_;
};

bool meteredInternet();
HANDLE prepareAudio(Store& store, HttpClient& http, const Track& track, const Server& profile,
                    const InputExtensions& inputs, unsigned& excluded, const std::function<bool()>& cancelled,
                    std::filesystem::path& path, bool& temporary, PlaybackProgress progress);
void downloadAudio(Store& store, HttpClient& http, const Track& track, const Server& profile, ServerApi& api,
                   bool metered, const std::atomic_bool& stopping, const InputExtensions& inputs,
                   unsigned excluded = 0, bool (*currentCost)() = meteredInternet);

} // namespace tangerine
