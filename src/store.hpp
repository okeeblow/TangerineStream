#pragma once

#include "model.hpp"
#include <windows.h>
#include <filesystem>
#include <map>
#include <mutex>

struct sqlite3;

namespace tangerine {

class Store {
public:
    explicit Store(const std::filesystem::path& directory);
    ~Store();
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    std::vector<Server> servers();
    Server server(Id id);
    void save(Server& server, int rollingMiB);
    int rollingMiB();
    std::string playbackSecret();
    int playbackPort();
    void savePlaybackPort(int port);

    void remember(std::vector<Track>& tracks);
    std::vector<Track> cached(Id server, const std::string& query, int offset);
    Track track(Id key);
    void pin(const std::vector<Track>& tracks, bool permanent);
    void syncFavorites(Id server, std::vector<Track>& tracks);
    std::vector<Track> pendingPins(Id server);
    void postponePin(Id key);

    std::filesystem::path audioPath(Id key) const;
    std::filesystem::path partialPath(Id key) const;
    bool reserve(Id key, std::int64_t bytes);
    void publish(Id key, std::int64_t bytes, const std::string& mime);
    void release(Id key);
    void touch(Id key);
    void trim();

    void enqueueReport(const Track& track, std::int64_t time);
    std::vector<Report> reports(Id server);
    void acknowledge(const std::vector<Report>& reports);
    std::int64_t reportCount(Id server);

private:
    void rememberUnlocked(std::vector<Track>& tracks);
    std::string setting(const char* key);
    void setSetting(const char* key, const std::string& value);
    Server loadServer(Id id);
    Track loadTrack(Id key);
    void reconcile();
    bool trimUnlocked(std::int64_t incoming);

    std::filesystem::path cache_;
    HANDLE lockFile_ = INVALID_HANDLE_VALUE;
    sqlite3* db_ = nullptr;
    std::mutex mutex_;
    std::map<Id, std::int64_t> downloads_;
    std::int64_t budget_ = 0;
};

} // namespace tangerine
