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

void downloadPinned(Store& store, const Track& track, ServerApi& api,
                    const std::atomic_bool& stopping);

} // namespace tangerine
