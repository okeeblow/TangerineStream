#pragma once

#include "store.hpp"
#include <condition_variable>
#include <deque>
#include <set>
#include <thread>

namespace tangerine {

struct Thumbnail {
    int row;
    std::vector<std::uint32_t> pixels;
};

class HttpClient;

class ArtworkLoader {
public:
    static constexpr UINT readyMessage = WM_APP + 3;
    const UINT coverReadyMessage;
    const UINT lyricsReadyMessage;
    using Generation = std::uint64_t;

    ArtworkLoader(Store& store, HttpClient& http, HWND player);
    ~ArtworkLoader();
    ArtworkLoader(const ArtworkLoader&) = delete;
    ArtworkLoader& operator=(const ArtworkLoader&) = delete;

    Generation reset(HWND view);
    void cancel(Generation generation);
    void setViewport(Generation generation, int size, const std::vector<LibraryItem>& items,
                     const std::vector<int>& rowsInPriorityOrder);
    std::vector<Thumbnail> take(Generation generation);
    void requestTrack(Id key, bool priority = false);
    void stageCover(const Track& track);
    std::string trackArtwork(const Track& track);

private:
    struct Request {
        Generation generation;
        int row;
        int size;
        LibraryItem item;
    };

    Store& store_;
    HttpClient& http_;
    const HWND player_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::vector<std::thread> workers_;
    std::deque<Request> requests_;
    std::set<int> startedRows_; // In flight or waiting for the view to collect.
    struct TrackRequest { Id key; bool priority; };
    std::deque<TrackRequest> tracks_;
    std::set<Id> pendingTracks_;
    std::vector<Thumbnail> complete_;
    HWND view_ = nullptr;
    Generation generation_ = 0;
    bool stopping_ = false;

    void load();
    void loadTrack(Id key);
    void notifyPlayer(UINT message, Id key);
    void stop();
};

HBITMAP thumbnailBitmap(int size, const std::vector<std::uint32_t>& pixels);

} // namespace tangerine
