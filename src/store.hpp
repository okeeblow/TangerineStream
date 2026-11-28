#pragma once

#include "model.hpp"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <set>

struct sqlite3;
struct sqlite3_stmt;

namespace tangerine {

class Statement;

struct CachedArtwork {
    std::string data;
    int requestedMaxEdgePx = 0;
    int cachedLongestEdgePx = 0; // Zero until the cached image has been measured.

    bool canReuseFor(int targetEdgePx) const {
        if (cachedLongestEdgePx >= targetEdgePx || requestedMaxEdgePx >= targetEdgePx) return true;
        // Stop asking for larger covers once the server returns its limit. Allow one pixel for rounding.
        return cachedLongestEdgePx > 0 && requestedMaxEdgePx - cachedLongestEdgePx > 1;
    }
};

class Store {
public:
    explicit Store(const std::filesystem::path& directory);
    ~Store();
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    std::vector<Server> servers();
    Server server(Id id);
    void saveProfiles(std::vector<Server>& profiles, int rollingMiB, bool unified,
                      const std::vector<Id>& removed = {});
    bool unifiedTree();
    int rollingMiB();
    std::string playbackSecret();
    int playbackPort();
    bool textCatalog();
    SortTags viewSettings(BrowseKind kind);
    void saveViewSettings(BrowseKind kind, const SortTags& values);
    int coverSize();
    void saveCoverSize(int size);
    std::string browseOrder(BrowseKind kind);
    void saveBrowseOrder(BrowseKind kind, const std::string& value);
    bool offlineMode();
    void setOfflineMode(bool offline);
    bool networkAvailable(Id server);
    bool connectionRetryDue(Id server);
    void connectionFailed(Id server);
    void connectionRestored(Id server);

    void rememberPage(Id server, const BrowseLocation& location, bool unfiltered,
                      int offset, BrowsePage& page);
    BrowsePage cachedPage(Id server, const BrowseLocation& location, const std::string& query, int offset);
    Track track(Id key);
    std::string albumCover(Id server, const std::string& albumId);
    CachedArtwork artwork(Id server, const LibraryItem& item);
    bool stageCover(const Track& track, const LibraryItem& song, const LibraryItem& album);
    void activeCover(Id key);
    void recordCachedCoverEdge(Id server, const LibraryItem& item, int requestedMaxEdgePx, int cachedLongestEdgePx);
    std::string saveArtwork(Id server, const LibraryItem& item, int requestedMaxEdgePx, int cachedLongestEdgePx, std::string data);
    std::optional<Lyrics> lyrics(Id key, bool fresh = false);
    void saveLyrics(Id key, const Lyrics& lyrics);
    void pin(const std::vector<Track>& tracks, bool permanent);
    void syncFavorites(Id server, BrowsePage& page);
    std::vector<Track> pendingPins(Id server, bool upgradeFavorites, const InputExtensions& inputs);
    void postponePin(Id key);

    HANDLE openCached(Id key, std::filesystem::path& path);
    std::filesystem::path partialPath(Id key) const;
    bool reserve(Id key, std::int64_t bytes, bool upgrade);
    bool growReservation(Id key, std::int64_t bytes);
    bool publish(Id key, std::int64_t bytes, const std::string& format);
    void release(Id key);
    void touch(Id key);
    void trim();

    void enqueueReport(const Track& track, std::int64_t time);
    std::vector<Report> reports(Id server);
    void acknowledge(const std::vector<Report>& reports);
    std::int64_t reportCount(Id server);

private:
    Statement prepare(const char* sql);
    void closeDatabase();
    void rememberUnlocked(std::vector<Track>& tracks);
    void rememberCatalog(Id server, const BrowseLocation& location, bool unfiltered,
                         int offset, const BrowsePage& page);
    std::string setting(const char* key);
    void setSetting(const char* key, const std::string& value);
    Server loadServer(Id id);
    Id saveProfile(const Server& server);
    Track loadTrack(Id key, bool configuredOnly = false);
    std::filesystem::path audioPath(const Track& track) const;
    void reconcile();
    bool trimUnlocked(std::int64_t incoming);

    std::filesystem::path cache_;
    HANDLE lockFile_ = INVALID_HANDLE_VALUE;
    sqlite3* db_ = nullptr;
    std::map<std::string, sqlite3_stmt*, std::less<>> statements_;
    std::mutex mutex_;
    std::set<Id> configured_;
    std::map<Id, std::int64_t> downloads_;
    std::map<Id, std::chrono::steady_clock::time_point> retryConnection_;
    std::int64_t budget_ = 0;
    std::optional<std::int64_t> artworkBytes_;
    bool offline_ = false;
    Id activeCover_ = 0;
};

} // namespace tangerine
