#pragma once

#include "server.hpp"
#include "store.hpp"
#include <condition_variable>
#include <deque>
#include <thread>

namespace tangerine {

class Playback {
public:
    explicit Playback(Store& store);
    ~Playback();
    Playback(const Playback&) = delete;
    Playback& operator=(const Playback&) = delete;

    std::wstring url(const Track& track) const;
    Id key(std::wstring_view url) const;

private:
    void listen();
    void work();
    void serve(SOCKET client);
    Id pathKey(std::string_view path) const;

    Store& store_;
    std::string pathPrefix_, urlPrefix_;
    SOCKET listener_ = INVALID_SOCKET;
    std::atomic_bool stopping_{false};
    std::thread listenerThread_;
    std::vector<std::thread> workers_;
    std::mutex queueMutex_;
    std::condition_variable queueReady_;
    std::deque<SOCKET> clients_;
    void stop();
};

void downloadPinned(Store& store, const Track& track, ServerApi& api,
                    const std::atomic_bool& stopping);

} // namespace tangerine
