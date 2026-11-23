#pragma once

#include "store.hpp"
#include <condition_variable>
#include <deque>
#include <thread>

namespace tangerine {

struct Thumbnail {
    int row;
    std::vector<std::uint32_t> pixels;
};

class ArtworkLoader {
public:
    static constexpr UINT readyMessage = WM_APP + 3;
    using Generation = std::uint64_t;

    explicit ArtworkLoader(Store& store);
    ~ArtworkLoader();
    ArtworkLoader(const ArtworkLoader&) = delete;
    ArtworkLoader& operator=(const ArtworkLoader&) = delete;

    Generation reset(HWND view);
    void cancel(Generation generation);
    bool request(Generation generation, Id server, int row, int size, const LibraryItem& item);
    std::vector<Thumbnail> take(Generation generation);

private:
    struct Request {
        Generation generation;
        Id server;
        int row;
        int size;
        LibraryItem item;
    };

    Store& store_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::vector<std::thread> workers_;
    std::deque<Request> requests_;
    std::vector<Thumbnail> complete_;
    HWND view_ = nullptr;
    Generation generation_ = 0;
    bool stopping_ = false;

    void load();
    void stop();
};

HBITMAP thumbnailBitmap(int size, const std::vector<std::uint32_t>& pixels);

} // namespace tangerine
