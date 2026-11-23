#include "playback.hpp"
#include "artwork.hpp"
#include "host_metadata.hpp"
#include "resources.h"
#include "input_bridge.hpp"
#include <fstream>
#include <shared_mutex>

#include <commctrl.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <deque>
#include <future>
#include <iterator>
#include <optional>
#include <set>
#include <thread>

#include "sdk_services.hpp"

// The SDK checks an undefined _MSC_VER as zero and supplies its own intptr_t.
#ifndef _MSC_VER
#define _MSC_VER 1900
#define RESTORE_MINGW_MACROS
#endif
#include "gen_ml/ml.h"
#include "gen_ml/ml_ipc_0313.h"
#include "Winamp/wa_ipc.h"
#ifdef RESTORE_MINGW_MACROS
#undef _MSC_VER
#undef RESTORE_MINGW_MACROS
#endif

namespace tangerine {
namespace {

constexpr wchar_t pluginName[] = L"Tangerine Stream";
constexpr auto viewReady = WM_APP + 1;
constexpr auto profilesChanged = WM_APP + 2;
constexpr auto moreRequested = WM_APP + 4;
constexpr UINT_PTR playbackTimer = 0x7151;
constexpr UINT_PTR viewTimer = 1;
constexpr UINT_PTR searchTimer = 2;
constexpr UINT_PTR playerSubclass = 0x7151;
constexpr UINT getIniDirectoryW = 1335; // Winamp 5.58+
using Clock = std::chrono::steady_clock;

int init();
void quit();
INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR c);

char description[] = "Tangerine Stream v0.4.2";
winampMediaLibraryPlugin plugin = [] {
    winampMediaLibraryPlugin descriptor{};
#ifdef MLHDR_VER_OLD
    // These SDKs also define a later ABI with a UTF-16 description.
    descriptor.version = MLHDR_VER_OLD;
#else
    descriptor.version = MLHDR_VER;
#endif
    descriptor.description = description;
    descriptor.init = init;
    descriptor.quit = quit;
    descriptor.MessageProc = message;
    return descriptor;
}();

LRESULT player(UINT command, WPARAM value = 0) {
    return SendMessageW(plugin.hwndWinampParent, WM_WA_IPC, value, command);
}

LRESULT library(UINT command, WPARAM value = 0) {
    return SendMessageW(plugin.hwndLibraryParent, WM_ML_IPC, value, command);
}

bool skinWindow(HWND hwnd, UINT type, UINT style = 0) {
    MLSKINWINDOW window{hwnd, type, style | SWS_USESKINFONT | SWS_USESKINCOLORS | SWS_USESKINCURSORS};
    return library(ML_IPC_SKINWINDOW, reinterpret_cast<WPARAM>(&window)) != 0;
}

struct Category {
    BrowseKind kind;
    const wchar_t* name;
};

constexpr Category categories[] = {
    {BrowseKind::albums, L"Albums"},
    {BrowseKind::genres, L"Genres"},
    {BrowseKind::albumArtists, L"Album Artists"},
    {BrowseKind::artists, L"Artists"},
    {BrowseKind::tracks, L"Tracks"},
    {BrowseKind::folders, L"Folders"},
    {BrowseKind::playlists, L"Playlists"},
    {BrowseKind::favorites, L"Favorites"},
    {BrowseKind::offline, L"Offline Cache"},
};

BrowseLocation categoryLocation(BrowseKind kind) {
    for (const auto& category : categories)
        if (category.kind == kind) return {kind, {}, utf8(category.name)};
    return {BrowseKind::albums, {}, "Albums"};
}

void showError(HWND window, const std::exception& error) {
    MessageBoxW(window, wide(error.what()).c_str(), pluginName, MB_OK | MB_ICONERROR);
}

std::string getText(HWND dialog, int item) {
    auto control = GetDlgItem(dialog, item);
    int length = GetWindowTextLengthW(control);
    std::wstring text(length + 1, L'\0');
    auto written = GetWindowTextW(control, text.data(), length + 1);
    text.resize(written);
    return utf8(text);
}

struct Search {
    HWND view;
    std::uint64_t serial;
    Id server;
    BrowseLocation location;
    std::string query;
    int offset;
    bool cached;
};

struct SearchResult {
    HWND view;
    std::uint64_t serial;
    BrowsePage page;
    std::string error;
    bool cached = false;
    bool restart = false;
};

struct Playing {
    Track track;
    int playlistPosition = -1;
    int outputPosition = 0;
    int state = 0;
    std::int64_t listened = 0;
    std::int64_t started = 0;
    Clock::time_point sampled;
    bool reported = false;
};

struct ServiceStatus {
    std::string reports, favorites, downloads;
};

enum class AlbumAction { play, enqueue, drop };

struct AlbumJob {
    AlbumAction action;
    mlDropItemStruct drop;
    HWND target;
    std::future<std::vector<Track>> tracks;
};

class App {
public:
    explicit App(const std::filesystem::path& profile)
        : store(profile), playback(store, profile), artwork(store, plugin.hwndWinampParent),
          hostMetadata(reinterpret_cast<void*>(player(IPC_GET_API_SERVICE)), store, playback, artwork) {
        try {
            browser = std::thread([this] { browse(); });
            maintenance = std::thread([this] { maintain(); });
            reporter = std::thread([this] { report(); });
        } catch (...) {
            stop();
            throw;
        }
    }
    ~App() { stop(); finishPlayback(); }

    void search(HWND view, Id server, const BrowseLocation& location, std::string query, int offset, bool cached) {
        {
            std::scoped_lock lock(searchMutex);
            request = Search{view, ++serial, server, location, std::move(query), offset, cached};
            result.reset();
        }
        searchReady.notify_one();
    }
    std::optional<SearchResult> takeResult(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (!result || result->view != view) return {};
        auto ready = std::move(result);
        result.reset();
        return ready;
    }
    void cancelSearch(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (activeView != view) return;
        ++serial;
        request.reset();
        result.reset();
    }
    void forgetView(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (activeView != view) return;
        activeView = nullptr;
        ++serial;
        request.reset();
        result.reset();
    }
    void wakeMaintenance(bool refresh = false) {
        {
            std::scoped_lock lock(maintenanceMutex);
            pendingMaintenance = true;
            refreshFavorites |= refresh;
            refreshReports |= refresh;
        }
        maintenanceReady.notify_one();
        if (refresh) reportsReady.notify_one();
    }
    std::string maintenanceError(Id server) {
        std::scoped_lock lock(maintenanceMutex);
        const auto& status = serviceStatus[server];
        if (!status.reports.empty()) return status.reports;
        if (!status.favorites.empty()) return status.favorites;
        return status.downloads;
    }
    void sample(Id startedKey = 0);
    void sendAlbums(Id server, std::vector<BrowseLocation> albums, AlbumAction action, mlDropItemStruct drop = {});
    void finishAlbums();
    void preparePlayback(const Track& track, const wchar_t* filename, bool cacheAudio);
    void finishPlayback();
    void updateLyrics(Id key);
    std::wstring playlistFile(const Track& track) {
        auto name = playback.playlistFile(track);
        artwork.stageCover(track);
        artwork.requestTrack(track.key);
        return name;
    }
    std::wstring displayedReference(Id key) {
        {
            std::scoped_lock lock(sidecarMutex);
            if (sidecarKey == key) return sidecarReference.wstring();
        }
        const auto index = static_cast<int>(player(IPC_GETLISTPOS));
        if (index < 0) return {};
        const auto file = reinterpret_cast<const wchar_t*>(player(IPC_GETPLAYLISTFILEW, index));
        return reinterpret_cast<UINT_PTR>(file) > 1 && playback.key(file) == key ? file : L"";
    }

    Store store;
    Playback playback;
    ArtworkLoader artwork;
    HostMetadataServices hostMetadata;
    HWND activeView = nullptr;
    Id selectedServer = 0;
    UINT_PTR treeId = 0;
    std::map<UINT_PTR, BrowseKind> navigation;
    bool attached = false;
    std::string reportError;

private:
    std::vector<Track> albumTracks(Id server, const BrowseLocation& album);
    std::deque<AlbumJob> albumJobs;
    std::atomic_bool stopping{false};
    std::thread browser, maintenance, reporter;
    std::mutex searchMutex, maintenanceMutex;
    std::condition_variable searchReady, maintenanceReady, reportsReady;
    std::uint64_t serial = 0;
    std::optional<Search> request;
    std::optional<SearchResult> result;
    bool pendingMaintenance = false, refreshFavorites = false, refreshReports = false;
    std::map<Id, ServiceStatus> serviceStatus;
    Id rollingRequest = 0;
    std::mutex sidecarMutex;
    Id sidecarKey = 0;
    std::filesystem::path sidecarReference;
    std::filesystem::path lyricsFile;
    Playing playing;
    bool sampling = false;
    std::deque<std::pair<Track, std::int64_t>> unwrittenReports;

    void browse();
    void maintain();
    void report();
    void flushReports();
    void stop() {
        {
            std::scoped_lock lock(searchMutex, maintenanceMutex);
            stopping = true;
        }
        searchReady.notify_all();
        maintenanceReady.notify_all();
        reportsReady.notify_all();
        if (browser.joinable()) browser.join();
        if (maintenance.joinable()) maintenance.join();
        if (reporter.joinable()) reporter.join();
    }
};

std::unique_ptr<App> app;
std::shared_mutex inputMutex;
App* inputProfile = nullptr;

void App::finishPlayback() {
    std::scoped_lock lock(sidecarMutex);
    if (!lyricsFile.empty()) {
        std::error_code error;
        std::filesystem::remove(lyricsFile, error);
        lyricsFile.clear();
    }
    sidecarReference.clear();
    sidecarKey = 0;
    store.activeCover(0);
}

void App::preparePlayback(const Track& track, const wchar_t* filename, bool cacheAudio) {
    finishPlayback();
    if (std::wstring_view(filename).find(L"://") == std::wstring_view::npos) {
        std::scoped_lock lock(sidecarMutex);
        store.activeCover(track.key);
        sidecarKey = track.key;
        sidecarReference = filename;
    }
    artwork.stageCover(track);
    updateLyrics(track.key);
    artwork.requestTrack(track.key, true);
    if (cacheAudio && store.rollingMiB() > 0) {
        {
            std::scoped_lock lock(maintenanceMutex);
            rollingRequest = track.key;
            pendingMaintenance = true;
        }
        maintenanceReady.notify_one();
    }
}

void App::updateLyrics(Id key) {
    std::scoped_lock lock(sidecarMutex);
    if (sidecarKey != key || sidecarReference.empty() || !lyricsFile.empty()) return;
    auto lyrics = store.lyrics(key);
    if (!lyrics || lyrics->plain.empty()) return;
    auto path = sidecarReference;
    path.replace_extension(L".txt");
    auto temporary = path;
    temporary += L".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    output << "\xef\xbb\xbf" << lyrics->plain;
    output.close();
    if (output && MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
        lyricsFile = std::move(path);
    else DeleteFileW(temporary.c_str());
}

int inputQuery(InputRequest request, const wchar_t* filename, const wchar_t* field,
               wchar_t* dest, size_t capacity, HANDLE* lease) {
    std::shared_lock lock(inputMutex);
    auto* owner = inputProfile;
    if (!owner || !filename) return 0;
    auto id = owner->playback.key(filename);
    if (!id) return 0;
    if (request == InputRequest::recognize) {
        auto track = owner->store.track(id);
        if (!track.key) return 0;
        owner->artwork.stageCover(track);
        return 1;
    }
    if (request == InputRequest::ended) { owner->finishPlayback(); return 1; }
    if (!dest || !capacity) return 0;
    auto track = owner->store.track(id);
    if (!track.key) return 0;
    std::wstring value;
    if (request == InputRequest::metadata) {
        auto tag = trackMetadata(owner->store, owner->artwork, track, field);
        if (!tag) return 0;
        value = wide(*tag);
    } else if (request == InputRequest::source && lease) {
        std::filesystem::path cached;
        *lease = owner->store.openCached(id, cached);
        if (*lease != INVALID_HANDLE_VALUE) {
            value = cached.wstring();
            owner->store.touch(id);
        } else {
            if (!owner->store.networkAvailable(track.server)) return 0;
            value = wide(connect(owner->store.server(track.server))->streamUrl(track));
        }
        if (value.size() >= capacity) return 0;
        owner->preparePlayback(track, filename, *lease == INVALID_HANDLE_VALUE);
    } else return 0;
    lstrcpynW(dest, value.c_str(), static_cast<int>(std::min<size_t>(capacity, INT_MAX)));
    return 1;
}


std::vector<Track> App::albumTracks(Id server, const BrowseLocation& album) {
    if (store.networkAvailable(server) && !localLocation(album)) {
        try {
            auto page = connect(store.server(server))->browse(album, {}, 0);
            store.rememberPage(server, album, true, 0, page);
            return std::move(page.tracks);
        } catch (const ConnectionError&) { store.connectionFailed(server); }
    }
    std::vector<Track> downloaded;
    for (int offset = 0; ; offset += pageSize) {
        auto page = store.cachedPage(server, album, {}, offset);
        for (auto& track : page.tracks)
            if (track.bytes > 0) downloaded.push_back(std::move(track));
        if (!page.more) return downloaded;
    }
}

void App::sendAlbums(Id server, std::vector<BrowseLocation> albums, AlbumAction action, mlDropItemStruct drop) {
    auto resolve = [this, server, albums = std::move(albums)] {
        std::vector<Track> result;
        for (const auto& album : albums) {
            auto tracks = albumTracks(server, album);
            std::move(tracks.begin(), tracks.end(), std::back_inserter(result));
        }
        return result;
    };
    auto target = action == AlbumAction::drop ? WindowFromPoint(drop.p) : nullptr;
    albumJobs.push_back({action, drop, target, std::async(std::launch::async, std::move(resolve))});
}

void App::finishAlbums() {
    while (!albumJobs.empty() && albumJobs.front().tracks.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto job = std::move(albumJobs.front());
        albumJobs.pop_front();
        if (job.action == AlbumAction::drop && (!IsWindow(job.target) || WindowFromPoint(job.drop.p) != job.target)) continue;
        auto tracks = job.tracks.get();
        if (tracks.empty()) {
            MessageBoxW(activeView, L"No tracks from the selected albums are available.", pluginName, MB_OK);
            continue;
        }
        struct Tags { std::wstring url, title, artist, album; };
        std::vector<Tags> tags;
        tags.reserve(tracks.size());
        std::vector<itemRecordW> records(tracks.size());
        for (size_t i = 0; i < tracks.size(); ++i) {
            const auto& t = tracks[i];
            tags.push_back({playlistFile(t), wide(t.title), wide(t.artist), wide(t.album)});
            auto& tag = tags.back();
            auto& record = records[i];
            record.filename = tag.url.data();
            record.title = tag.title.data();
            record.artist = tag.artist.data();
            record.album = tag.album.data();
            record.length = t.duration;
            record.track = t.trackNumber;
            record.disc = t.discNumber;
        }
        const int count = static_cast<int>(records.size());
        itemRecordListW list{records.data(), count, count};
        if (job.action != AlbumAction::drop) {
            // Bit 1 makes the explicit Play / Enqueue choice override the ML default.
            mlSendToWinampStruct send{ML_TYPE_ITEMRECORDLISTW, &list, job.action == AlbumAction::enqueue ? 3 : 2};
            library(ML_IPC_SENDTOWINAMP, reinterpret_cast<WPARAM>(&send));
            continue;
        }
        job.drop.flags = 0;
        job.drop.result = 0;
        job.drop.data = &list;
        library(ML_IPC_HANDLEDROP, reinterpret_cast<WPARAM>(&job.drop));
    }
}

void App::browse() {
    Server current;
    std::unique_ptr<ServerApi> api;
    for (;;) {
        Search search;
        {
            std::unique_lock lock(searchMutex);
            searchReady.wait(lock, [this] { return stopping || request.has_value(); });
            if (stopping) return;
            search = std::move(*request);
            request.reset();
        }
        SearchResult ready{search.view, search.serial, {}, {}};
        try {
            ready.cached = search.cached || search.location.kind == BrowseKind::offline ||
                localLocation(search.location) || !store.networkAvailable(search.server);
            if (!ready.cached) {
                try {
                    auto profile = store.server(search.server);
                    if (!api || profile.id != current.id || profile.native != current.native ||
                        profile.password != current.password) {
                        api = connect(profile);
                        current = std::move(profile);
                    }
                    const bool filterPlaylist = search.location.kind == BrowseKind::playlistTracks &&
                        !search.query.empty();
                    const auto remoteQuery = filterPlaylist ? std::string{} : search.query;
                    ready.page = api->browse(search.location, remoteQuery, search.offset);
                    store.rememberPage(search.server, search.location, remoteQuery.empty(), search.offset, ready.page);
                    if (filterPlaylist) {
                        auto filter = wide(search.query);
                        auto& tracks = ready.page.tracks;
                        tracks.erase(std::remove_if(tracks.begin(), tracks.end(), [&](const Track& track) {
                            return !matches(track.title, filter) && !matches(track.artist, filter) &&
                                !matches(track.album, filter);
                        }), tracks.end());
                    }
                } catch (const ConnectionError&) {
                    store.connectionFailed(search.server);
                    api.reset();
                    ready.cached = true;
                }
            }
            if (ready.cached) {
                ready.restart = !search.cached && search.offset != 0;
                ready.page = store.cachedPage(search.server, search.location, search.query,
                                              ready.restart ? 0 : search.offset);
            }
        } catch (const std::exception& error) {
            ready.error = error.what();
        }
        std::scoped_lock lock(searchMutex);
        if (!stopping && serial == ready.serial) {
            result = std::move(ready);
            PostMessageW(search.view, viewReady, 0, 0);
        }
    }
}

void App::maintain() {
    using namespace std::chrono;
    std::map<Id, Clock::time_point> nextFavorites;
    for (;;) {
        Id requested = 0;
        {
            std::unique_lock lock(maintenanceMutex);
            maintenanceReady.wait_for(lock, seconds(5), [this] { return stopping || pendingMaintenance; });
            if (stopping) return;
            requested = std::exchange(rollingRequest, 0);
            pendingMaintenance = false;
            if (refreshFavorites) nextFavorites.clear();
            refreshFavorites = false;
        }
        std::vector<Server> profiles;
        try {
            if (requested) {
                auto track = store.track(requested);
                if (track.key && !track.bytes && store.networkAvailable(track.server)) {
                    try { downloadPinned(store, track, *connect(store.server(track.server)), stopping); }
                    catch (const ConnectionError&) { store.connectionFailed(track.server); }
                    catch (const Error&) { /* Rolling storage is optional during streaming. */ }
                }
            }
            profiles = store.servers();
            store.trim();
        } catch (...) {}
        for (const auto& profile : profiles) {
            if (stopping) return;
            std::unique_ptr<ServerApi> api;
            auto connection = [&]() -> ServerApi& {
                if (!api) api = connect(store.server(profile.id));
                return *api;
            };
            if (store.connectionRetryDue(profile.id)) {
                try {
                    connection().ping();
                    store.connectionRestored(profile.id);
                } catch (const ConnectionError&) {
                    store.connectionFailed(profile.id);
                } catch (const std::exception&) {
                    // A reachable server can reject the profile and let the browser surface the error.
                    store.connectionRestored(profile.id);
                }
            }
            if (!store.networkAvailable(profile.id)) continue;
            if (Clock::now() >= nextFavorites[profile.id] || !profile.cacheFavorites) {
                std::string failure;
                if (profile.cacheFavorites) {
                    try {
                        nextFavorites[profile.id] = Clock::now() + minutes(10);
                        auto favorites = connection().favorites(stopping);
                        store.syncFavorites(profile.id, favorites);
                    } catch (const std::exception& error) {
                        if (dynamic_cast<const ConnectionError*>(&error)) store.connectionFailed(profile.id);
                        failure = std::string("Favorite sync: ") + error.what();
                        nextFavorites[profile.id] = Clock::now() + seconds(30);
                    }
                }
                std::scoped_lock lock(maintenanceMutex);
                serviceStatus[profile.id].favorites = std::move(failure);
            }
            std::string failure;
            try {
                for (const auto& track : store.pendingPins(profile.id)) {
                    if (stopping) return;
                    if (!store.networkAvailable(profile.id)) break;
                    try {
                        downloadPinned(store, track, connection(), stopping);
                        artwork.requestTrack(track.key);
                    } catch (const std::exception& error) {
                        if (dynamic_cast<const ConnectionError*>(&error)) store.connectionFailed(profile.id);
                        store.postponePin(track.key);
                        failure = std::string("Cache download: ") + error.what();
                        break;
                    }
                }
            } catch (const std::exception& error) {
                failure = error.what();
            }
            std::scoped_lock lock(maintenanceMutex);
            serviceStatus[profile.id].downloads = std::move(failure);
        }
    }
}

void App::report() {
    using namespace std::chrono;
    std::map<Id, Clock::time_point> retryAfter;
    for (;;) {
        {
            std::unique_lock lock(maintenanceMutex);
            reportsReady.wait_for(lock, seconds(15), [this] { return stopping || refreshReports; });
            if (stopping) return;
            if (refreshReports) retryAfter.clear();
            refreshReports = false;
        }
        std::vector<Server> profiles;
        try { profiles = store.servers(); } catch (...) {}
        for (const auto& profile : profiles) {
            if (stopping) return;
            if (!store.networkAvailable(profile.id)) continue;
            if (Clock::now() < retryAfter[profile.id]) continue;
            std::string failure;
            try {
                std::unique_ptr<ServerApi> api;
                // Bound each round so one backlog cannot monopolize reporting.
                for (int i = 0; i < 4 && !stopping; ++i) {
                    if (!store.networkAvailable(profile.id)) break;
                    auto batch = store.reports(profile.id);
                    if (batch.empty()) break;
                    if (!api) api = connect(store.server(profile.id));
                    api->submit(batch);
                    store.acknowledge(batch);
                }
            } catch (const std::exception& error) {
                if (dynamic_cast<const ConnectionError*>(&error)) store.connectionFailed(profile.id);
                failure = std::string("Report delivery: ") + error.what();
                retryAfter[profile.id] = Clock::now() + seconds(30);
            }
            std::scoped_lock lock(maintenanceMutex);
            serviceStatus[profile.id].reports = std::move(failure);
        }
    }
}

void App::flushReports() {
    while (!unwrittenReports.empty()) {
        try {
            const auto& [track, time] = unwrittenReports.front();
            store.enqueueReport(track, time);
            unwrittenReports.pop_front();
            reportError.clear();
        } catch (const std::exception& error) {
            reportError = error.what();
            return;
        }
    }
}

void App::sample(Id startedKey) {
    if (sampling) return;
    struct Reset { bool& flag; ~Reset() { flag = false; } } reset{sampling};
    sampling = true;
    flushReports();
    const auto state = static_cast<int>(player(IPC_ISPLAYING));
    const auto index = static_cast<int>(player(IPC_GETLISTPOS));
    const auto output = static_cast<int>(player(IPC_GETOUTPUTTIME));
    auto filename = index < 0 ? nullptr : reinterpret_cast<const wchar_t*>(player(IPC_GETPLAYLISTFILEW, index));
    const auto key = startedKey ? startedKey : (filename && reinterpret_cast<UINT_PTR>(filename) > 1 ?
                                               playback.key(filename) : 0);
    const auto now = Clock::now();
    if (!state || !key) {
        playing = {};
        return;
    }
    const auto delta = std::int64_t(output) - playing.outputPosition;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - playing.sampled).count();
    const bool changed = startedKey || key != playing.track.key || index != playing.playlistPosition;
    // The fallback detects a natural repeat if the host did not send the wide playback notification.
    const bool repeated = playing.track.duration > 0 && output < 1000 && delta < -1000 &&
        std::int64_t(playing.outputPosition) >= std::int64_t(playing.track.duration) * 1000 - 1500;
    if (changed || repeated) {
        playing = {};
        playing.track = store.track(key);
        artwork.requestTrack(key, true);
        playing.playlistPosition = index;
        playing.started = nowMs() - std::max(0, output);
    } else if (playing.state == 1 && output >= 0 && delta > 0 && delta <= elapsed + 750) {
        playing.listened += std::min<std::int64_t>(delta, elapsed);
    }
    playing.state = state;
    playing.outputPosition = std::max(0, output);
    playing.sampled = now;
    constexpr std::int64_t fourMinutes = 240'000;
    auto threshold = playing.track.duration > 0 ?
        std::min(fourMinutes, std::int64_t(playing.track.duration) * 500) : fourMinutes;
    if (playing.track.key && !playing.reported && playing.listened >= threshold) {
        unwrittenReports.emplace_back(playing.track, playing.started);
        playing.reported = true;
        flushReports();
    }
}

Track hostTrack(const wchar_t* url) {
    auto key = url ? app->playback.key(url) : 0;
    return key ? app->store.track(key) : Track{};
}

void copyMetadata(wchar_t* output, size_t capacity, const std::string& value) {
    if (output && capacity) lstrcpynW(output, wide(value).c_str(),
                                       static_cast<int>(std::min<size_t>(capacity, INT_MAX)));
}

void copyMetadata(char* output, size_t capacity, const std::string& value) {
    if (!output || !capacity) return;
    const auto text = wide(value);
    const auto bytes = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) { *output = 0; return; }
    std::string local(bytes, '\0');
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, local.data(), bytes, nullptr, nullptr);
    lstrcpynA(output, local.c_str(), static_cast<int>(std::min<size_t>(capacity, INT_MAX)));
}

bool supplyMetadata(LPARAM command, WPARAM data) {
    if (data <= 1) return false;
    if (command == IPC_HOOK_TITLESW || command == IPC_HOOK_TITLES) {
        if (command == IPC_HOOK_TITLESW) {
            auto& info = *reinterpret_cast<waHookTitleStructW*>(data);
            auto track = hostTrack(info.filename);
            if (!track.key) return false;
            copyMetadata(info.title, 2048, playlistTitle(track));
            info.length = track.duration ? track.duration : -1;
        } else {
            auto& info = *reinterpret_cast<waHookTitleStruct*>(data);
            auto track = hostTrack(info.filename ? wide(info.filename).c_str() : nullptr);
            if (!track.key) return false;
            copyMetadata(info.title, 2048, playlistTitle(track));
            info.length = track.duration ? track.duration : -1;
        }
        return true;
    }
    if (command == IPC_GET_BASIC_FILE_INFOW) {
        auto& info = *reinterpret_cast<basicFileInfoStructW*>(data);
        auto track = hostTrack(info.filename);
        if (!track.key) return false;
        if (info.titlelen > 0) copyMetadata(info.title, info.titlelen, playlistTitle(track));
        info.length = track.duration ? track.duration : -1;
        return true;
    }
    if (command == IPC_GET_EXTENDED_FILE_INFOW || command == IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE) {
        const auto& info = *reinterpret_cast<const extendedFileInfoStructW*>(data);
        auto track = hostTrack(info.filename);
        if (track.key) {
            if (auto value = trackMetadata(app->store, app->artwork, track, info.metadata)) {
                copyMetadata(info.ret, info.retlen, *value);
                return true;
            }
        }
    }
    if (command == IPC_GET_EXTENDED_FILE_INFO || command == IPC_GET_EXTENDED_FILE_INFO_HOOKABLE) {
        const auto& info = *reinterpret_cast<const extendedFileInfoStruct*>(data);
        auto track = hostTrack(info.filename ? wide(info.filename).c_str() : nullptr);
        if (track.key) {
            auto field = info.metadata ? wide(info.metadata) : std::wstring{};
            if (auto value = trackMetadata(app->store, app->artwork, track, field.c_str())) {
                copyMetadata(info.ret, info.retlen, *value);
                return true;
            }
        }
    }
    return false;
}

LRESULT CALLBACK playerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if (app && msg == WM_WA_IPC) {
        try { if (supplyMetadata(l, w)) return TRUE; } catch (const std::exception&) {}
    }
    if (app && msg && (msg == app->artwork.coverReadyMessage || msg == app->artwork.lyricsReadyMessage)) {
        try {
            auto key = static_cast<Id>((std::uint64_t(static_cast<DWORD>(l)) << 32) | static_cast<DWORD>(w));
            auto filename = app->displayedReference(key);
            if (!filename.empty()) {
                if (msg == app->artwork.coverReadyMessage) {
                    app->hostMetadata.artworkChanged(filename.c_str());
                } else {
                    app->updateLyrics(key);
                    player(IPC_FILE_TAG_MAY_HAVE_UPDATEDW, reinterpret_cast<WPARAM>(filename.c_str()));
                }
            }
        } catch (const std::exception&) {}
        return 0;
    }
    if (app && ((msg == WM_TIMER && w == playbackTimer) ||
                (msg == WM_WA_IPC && l == IPC_PLAYING_FILEW && w > 1))) {
        try {
            if (msg == WM_TIMER) app->finishAlbums();
            auto key = msg == WM_WA_IPC ? app->playback.key(reinterpret_cast<const wchar_t*>(w)) : 0;
            app->sample(key);
        } catch (const std::exception& error) { app->reportError = error.what(); }
    }
    return DefSubclassProc(hwnd, msg, w, l);
}

struct Settings {
    Server profile;
    int budget;
};

INT_PTR CALLBACK settingsProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (msg == WM_INITDIALOG) {
        settings = reinterpret_cast<Settings*>(l);
        SetWindowLongPtrW(hwnd, DWLP_USER, l);
        const auto& s = settings->profile;
        SetDlgItemTextW(hwnd, IDC_NAME, wide(s.name).c_str());
        SetDlgItemTextW(hwnd, IDC_URL, wide(s.url).c_str());
        SetDlgItemTextW(hwnd, IDC_USERNAME, wide(s.user).c_str());
        SetDlgItemInt(hwnd, IDC_BUDGET, settings->budget, FALSE);
        CheckDlgButton(hwnd, IDC_NATIVE, s.native ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_FAVORITES, s.cacheFavorites ? BST_CHECKED : BST_UNCHECKED);
        if (s.id) {
            SendDlgItemMessageW(hwnd, IDC_URL, EM_SETREADONLY, TRUE, 0);
            SendDlgItemMessageW(hwnd, IDC_USERNAME, EM_SETREADONLY, TRUE, 0);
            SendDlgItemMessageW(hwnd, IDC_PASSWORD, EM_SETCUEBANNER, FALSE,
                                reinterpret_cast<LPARAM>(L"Leave blank to keep saved password"));
        }
        SendDlgItemMessageW(hwnd, IDC_NAME, EM_SETLIMITTEXT, 160, 0);
        SendDlgItemMessageW(hwnd, IDC_URL, EM_SETLIMITTEXT, 512, 0);
        SendDlgItemMessageW(hwnd, IDC_USERNAME, EM_SETLIMITTEXT, 160, 0);
        SendDlgItemMessageW(hwnd, IDC_PASSWORD, EM_SETLIMITTEXT, 512, 0);
        SendDlgItemMessageW(hwnd, IDC_BUDGET, EM_SETLIMITTEXT, 7, 0);
        return TRUE;
    }
    if (!settings || msg != WM_COMMAND) return FALSE;
    if (LOWORD(w) == IDCANCEL) {
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }
    if (LOWORD(w) != IDOK) return FALSE;
    try {
        auto updated = settings->profile;
        updated.name = getText(hwnd, IDC_NAME);
        updated.url = normalizeUrl(getText(hwnd, IDC_URL));
        updated.user = getText(hwnd, IDC_USERNAME);
        updated.password = getText(hwnd, IDC_PASSWORD);
        updated.native = IsDlgButtonChecked(hwnd, IDC_NATIVE) == BST_CHECKED;
        updated.cacheFavorites = IsDlgButtonChecked(hwnd, IDC_FAVORITES) == BST_CHECKED;
        if (updated.user.empty() || (!updated.id && updated.password.empty()))
            throw Error("Enter the server username and password.");
        if (updated.name.empty()) updated.name = updated.user + " @ " + updated.url;
        BOOL valid = FALSE;
        auto budget = GetDlgItemInt(hwnd, IDC_BUDGET, &valid, FALSE);
        if (!valid || budget > 1'048'576) throw Error("Enter a rolling cache size from 0 to 1048576 MiB.");
        app->store.save(updated, static_cast<int>(budget));
        settings->profile.id = updated.id;
        EndDialog(hwnd, IDOK);
    } catch (const std::exception& error) { showError(hwnd, error); }
    return TRUE;
}

void configure(HWND hwnd, const Server& profile = {}) {
    Settings settings{profile, app->store.rollingMiB()};
    if (DialogBoxParamW(plugin.hDllInstance, MAKEINTRESOURCEW(IDD_SERVER), hwnd, settingsProc,
                        reinterpret_cast<LPARAM>(&settings)) == IDOK) {
        app->selectedServer = settings.profile.id;
        app->wakeMaintenance(true);
        if (app->activeView) SendMessageW(app->activeView, profilesChanged, 0, 0);
    }
}

class View {
public:
    View(HWND window, BrowseKind category)
        : hwnd(window), list(GetDlgItem(hwnd, IDC_TRACKS)), category(category), location(categoryLocation(category)) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        const wchar_t* names[] = {L"Name", L"Details", L"Album", L"Time", L"Cache"};
        for (int i = 0; i < 5; ++i) {
            column.pszText = const_cast<wchar_t*>(names[i]);
            ListView_InsertColumn(list, i, &column);
        }
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP | LVS_EX_HEADERINALLVIEWS);
        ListView_SetUnicodeFormat(list, TRUE);
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        if (!skinWindow(list, SKINNEDWND_TYPE_LISTVIEW, SWLVS_DOUBLEBUFFER)) {
            legacySkin = library(ML_IPC_SKIN_LISTVIEW, reinterpret_cast<WPARAM>(list));
        }
        skinWindow(GetDlgItem(hwnd, IDC_QUERY), SKINNEDWND_TYPE_EDIT, SWES_VCENTER);
        for (int id : {IDC_SEARCH_LABEL, IDC_STATUS})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_STATIC);
        for (int id : {IDC_VIEW_MENU, IDC_ADD_SERVER, IDC_BACK, IDC_OPEN, IDC_PLAY, IDC_ENQUEUE, IDC_CACHE_MENU})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_BUTTON);
        ListView_SetTextBkColor(list, CLR_NONE);
        tileSize = std::clamp(MulDiv(160, GetDpiForWindow(hwnd), 96), 96, 384);
        SetTimer(hwnd, viewTimer, 2000, nullptr);
        SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETLIMITTEXT, 256, 0);
        textList = app->store.textCatalog();
    }
    ~View() {
        KillTimer(hwnd, viewTimer);
        KillTimer(hwnd, searchTimer);
        if (app) app->artwork.cancel(artGeneration);
        if (legacySkin) library(ML_IPC_UNSKIN_LISTVIEW, legacySkin);
        if (images) {
            ListView_SetImageList(list, nullptr, LVSIL_NORMAL);
            ImageList_Destroy(images);
        }
    }
    void populateServers();
    void receive(SearchResult result);
    void receiveArtwork();
    void fetchMore();
    void display(NMLVDISPINFOW& display);
    void activate(int index);
    void command(int id, int notification);
    void refresh();
    void updateButtons();
    void prefetchTrack(int row) {
        if (showTracks() && row >= 0 && static_cast<size_t>(row) < tracks.size())
            app->artwork.requestTrack(tracks[row].key);
    }
    void layout(int width, int height);
    void beginDrag(const NMLISTVIEW& notification);
    void drag(POINT point, bool released);
    void cancelDrag();
    HWND hwnd, list;

private:
    struct History {
        BrowseLocation location;
        std::string query;
        std::vector<LibraryItem> items;
        int nextOffset;
        int focused;
        bool more;
        bool cached;
    };
    struct ImageSlot {
        int row;
        std::uint64_t read;
    };

    BrowseKind category;
    BrowseLocation location;
    std::optional<BrowseLocation> draggedAlbum;
    std::string query;
    std::vector<History> history;
    std::vector<Server> servers;
    std::vector<LibraryItem> items;
    std::vector<Track> tracks;
    std::set<std::pair<BrowseKind, std::string>> knownItems;
    std::set<Id> knownTracks;
    int nextOffset = 0;
    bool loading = false, more = false, morePosted = false;
    bool cached = false;
    std::string message;
    LRESULT legacySkin = 0;
    Id currentServer = 0;
    int tileSize = 160;
    bool textList = false;
    HIMAGELIST images = nullptr;
    ArtworkLoader::Generation artGeneration = 0;
    std::vector<ImageSlot> slots;
    std::map<int, int> slotForRow;
    std::set<int> pendingArt, missingArt;
    std::uint64_t artRead = 0;
    bool artQueueFull = false;

    bool showTracks() const { return trackView(location.kind); }
    bool albumActions() const {
        return location.kind == BrowseKind::albums || location.kind == BrowseKind::genreAlbums ||
               location.kind == BrowseKind::artistAlbums || location.kind == BrowseKind::favorites;
    }
    bool showTiles() const {
        return !showTracks() && !textList;
    }
    int rowCount() const { return static_cast<int>(showTracks() ? tracks.size() : items.size()); }
    const BrowseLocation* albumAt(int index) const {
        if (showTracks() || index < 0 || static_cast<size_t>(index) >= items.size()) return nullptr;
        const auto& target = items[index].target;
        return target.kind == BrowseKind::albumTracks ? &target : nullptr;
    }
    void startSearch();
    void resetContent();
    void resetLocation();
    void changePresentation();
    void resizeColumns();
    void popup(int button);
    void updateRowCount();
    void scheduleMore();
    void resetImages();
    int imageFor(int row);
    void back();
    void play(bool enqueue);
    void cache(bool pin);
    void updateStatus();
    std::wstring trackCell(const Track& track, int column) const;
    std::vector<Track> selection();
    const Server* selected() const;
};

const Server* View::selected() const {
    auto it = std::find_if(servers.begin(), servers.end(),
        [this](const Server& server) { return server.id == currentServer; });
    return it == servers.end() ? nullptr : &*it;
}

void View::populateServers() {
    servers = app->store.servers();
    currentServer = app->selectedServer;
    if (!selected()) currentServer = servers.empty() ? 0 : servers.front().id;
    resetLocation();
}

void View::resetLocation() {
    if (!history.empty()) SetDlgItemTextW(hwnd, IDC_QUERY, L"");
    history.clear();
    location = categoryLocation(category);
    startSearch();
}

void View::resetContent() {
    cancelDrag();
    app->cancelSearch(hwnd);
    loading = more = morePosted = false;
    cached = false;
    nextOffset = 0;
    message.clear();
    ListView_SetItemCount(list, 0);
    items.clear();
    tracks.clear();
    knownItems.clear();
    knownTracks.clear();
    resetImages();
    auto server = selected();
    currentServer = server ? server->id : 0;
    if (server) app->selectedServer = server->id;
    changePresentation();
    updateButtons();
    updateStatus();
}

void View::startSearch() {
    KillTimer(hwnd, searchTimer);
    query = getText(hwnd, IDC_QUERY);
    resetContent();
    more = currentServer != 0;
    fetchMore();
}

void View::fetchMore() {
    morePosted = false;
    if (!more || loading || !currentServer) return;
    loading = true;
    updateButtons();
    updateStatus();
    app->search(hwnd, currentServer, location, query, nextOffset, cached);
}

void View::scheduleMore() {
    if (!more || loading || morePosted) return;
    morePosted = PostMessageW(hwnd, moreRequested, 0, 0) != 0;
}

void View::updateRowCount() {
    ListView_SetItemCountEx(list, rowCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    InvalidateRect(list, nullptr, FALSE);
    updateStatus();
    updateButtons();
}

void View::receive(SearchResult result) {
    if (result.restart) resetContent();
    loading = false;
    cached = result.cached;
    message = std::move(result.error);
    if (!message.empty()) {
        more = false;
        updateStatus();
        updateButtons();
        return;
    }
    const auto oldCount = rowCount();
    const bool emptyPage = result.page.items.empty() && result.page.tracks.empty();
    for (auto& item : result.page.items) {
        if (knownItems.emplace(item.target.kind, item.target.id).second)
            items.push_back(std::move(item));
    }
    const bool pagedTracks = location.kind == BrowseKind::tracks || location.kind == BrowseKind::offline;
    for (auto& track : result.page.tracks) {
        if (!pagedTracks || knownTracks.insert(track.key).second) tracks.push_back(std::move(track));
    }
    // A repeated server page must not get stuck in an endless scroll loop.
    more = result.page.more && nextOffset < INT_MAX - pageSize && (emptyPage || rowCount() != oldCount);
    nextOffset += pageSize;
    updateRowCount();
    if (!oldCount) prefetchTrack(0);
    if (emptyPage || rowCount() < 12) scheduleMore();
}

void View::refresh() {
    if (cached && !loading && currentServer && location.kind != BrowseKind::offline &&
        !localLocation(location) && app->store.networkAvailable(currentServer)) {
        startSearch();
        return;
    }
    const int first = std::max(0, ListView_GetTopIndex(list));
    const int last = std::min(static_cast<int>(tracks.size()), first + ListView_GetCountPerPage(list) + 1);
    for (int i = first; i < last; ++i) {
        auto updated = app->store.track(tracks[i].key);
        const auto& track = tracks[i];
        if (track.bytes == updated.bytes && track.favorite == updated.favorite &&
            track.pinned == updated.pinned) continue;
        tracks[i] = std::move(updated);
        ListView_RedrawItems(list, i, i);
    }
    updateStatus();
}

std::wstring View::trackCell(const Track& track, int column) const {
    switch (column) {
    case 0: return wide(track.title.empty() ? track.id : track.title);
    case 1: return wide(track.artist);
    case 2: return wide(track.album);
    case 3:
        if (track.duration) {
            auto seconds = track.duration % 60;
            return std::to_wstring(track.duration / 60) + L":" + (seconds < 10 ? L"0" : L"") +
                std::to_wstring(seconds);
        }
        return {};
    case 4:
        if (track.pinned) return track.bytes ? L"Permanent" : L"Pin pending";
        if (track.favorite) return track.bytes ? L"Favorite" : L"Favorite pending";
        return track.bytes ? L"Rolling" : L"";
    default: return {};
    }
}

void View::display(NMLVDISPINFOW& display) {
    auto& item = display.item;
    const auto row = item.iItem;
    if (row < 0 || row >= rowCount()) return;
    if (item.mask & LVIF_TEXT) {
        std::wstring text;
        if (showTracks()) text = trackCell(tracks[row], item.iSubItem);
        else if (item.iSubItem == 0) text = wide(items[row].target.title);
        else if (item.iSubItem == 1) text = wide(items[row].subtitle);
        if (item.pszText && item.cchTextMax > 0) lstrcpynW(item.pszText, text.c_str(), item.cchTextMax);
    }
    if (item.mask & LVIF_IMAGE) item.iImage = showTiles() ? imageFor(row) : I_IMAGENONE;
    if (row >= rowCount() - 12) scheduleMore();
}

void View::resetImages() {
    artGeneration = app->artwork.reset(hwnd);
    pendingArt.clear();
    missingArt.clear();
    slots.clear();
    slotForRow.clear();
    artQueueFull = false;
    artRead = 0;
    if (images) {
        ListView_SetImageList(list, nullptr, LVSIL_NORMAL);
        ImageList_Destroy(images);
        images = nullptr;
    }
}

int View::imageFor(int row) {
    if (!images) return I_IMAGENONE;
    auto existing = slotForRow.find(row);
    if (existing != slotForRow.end()) {
        slots[existing->second - 1].read = ++artRead;
        return existing->second;
    }
    if (!pendingArt.count(row) && !missingArt.count(row)) {
        if (app->artwork.request(artGeneration, currentServer, row, tileSize, items[row]))
            pendingArt.insert(row);
        else artQueueFull = true;
    }
    return 0;
}

void View::receiveArtwork() {
    const std::size_t maxImages = std::min<std::size_t>(192, 24 * MiB / (tileSize * tileSize * 4));
    for (const auto& thumbnail : app->artwork.take(artGeneration)) {
        pendingArt.erase(thumbnail.row);
        if (!images) continue;
        auto bitmap = thumbnailBitmap(tileSize, thumbnail.pixels);
        if (!bitmap) {
            missingArt.insert(thumbnail.row);
            continue;
        }
        int index = -1;
        if (slots.size() < maxImages) {
            index = ImageList_Add(images, bitmap, nullptr);
            if (index > 0) slots.push_back({thumbnail.row, ++artRead});
        } else {
            auto oldest = std::min_element(slots.begin(), slots.end(),
                [](const ImageSlot& a, const ImageSlot& b) { return a.read < b.read; });
            int candidate = static_cast<int>(oldest - slots.begin()) + 1;
            if (ImageList_Replace(images, candidate, bitmap, nullptr)) {
                slotForRow.erase(oldest->row);
                *oldest = {thumbnail.row, ++artRead};
                index = candidate;
            }
        }
        DeleteObject(bitmap);
        if (index > 0) {
            slotForRow[thumbnail.row] = index;
            if (showTiles()) ListView_RedrawItems(list, thumbnail.row, thumbnail.row);
        } else {
            missingArt.insert(thumbnail.row);
        }
    }
    if (std::exchange(artQueueFull, false) && showTiles()) InvalidateRect(list, nullptr, FALSE);
}

void View::changePresentation() {
    const bool tiles = showTiles();
    if (tiles && !images) {
        images = ImageList_Create(tileSize, tileSize, ILC_COLOR32, 1, 8);
        if (images) {
            auto color = ListView_GetBkColor(list);
            if (color == CLR_NONE) color = GetSysColor(COLOR_WINDOW);
            auto ink = ListView_GetTextColor(list);
            auto tone = [](unsigned base, unsigned text) { return (base * 7 + text) / 8; };
            const std::uint32_t fill = 0xff000000u | (tone(GetRValue(color), GetRValue(ink)) << 16) |
                (tone(GetGValue(color), GetGValue(ink)) << 8) | tone(GetBValue(color), GetBValue(ink));
            auto placeholder = thumbnailBitmap(tileSize,
                std::vector<std::uint32_t>(static_cast<std::size_t>(tileSize) * tileSize, fill));
            if (placeholder) {
                ImageList_Add(images, placeholder, nullptr);
                DeleteObject(placeholder);
            }
            if (ImageList_GetImageCount(images) != 1) {
                ImageList_Destroy(images);
                images = nullptr;
            }
        }
        if (images) {
            ImageList_SetBkColor(images, CLR_NONE);
            ListView_SetImageList(list, images, LVSIL_NORMAL);
        }
    }
    if (ListView_SetView(list, tiles ? LV_VIEW_ICON : LV_VIEW_DETAILS) != 1) {
        auto style = GetWindowLongPtrW(list, GWL_STYLE);
        SetWindowLongPtrW(list, GWL_STYLE, (style & ~LVS_TYPEMASK) | (tiles ? LVS_ICON : LVS_REPORT));
        SetWindowPos(list, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    if (tiles && images) {
        const auto font = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
        HDC dc = GetDC(list);
        auto previous = SelectObject(dc, font);
        TEXTMETRICW metrics{};
        GetTextMetricsW(dc, &metrics);
        SelectObject(dc, previous);
        ReleaseDC(list, dc);
        const int gap = MulDiv(4, GetDpiForWindow(hwnd), 96);
        ListView_SetIconSpacing(list, tileSize + gap, tileSize + 2 * metrics.tmHeight + gap);
    }
    SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETCUEBANNER, FALSE,
        reinterpret_cast<LPARAM>(showTracks() ? L"Search tracks" : L"Search this view"));
    const wchar_t* title = L"Name";
    if (showTracks()) title = L"Title";
    else switch (location.kind) {
    case BrowseKind::albums: case BrowseKind::artistAlbums: case BrowseKind::genreAlbums: title = L"Album"; break;
    case BrowseKind::genres: title = L"Genre"; break;
    case BrowseKind::albumArtists: title = L"Album Artist"; break;
    case BrowseKind::artists: title = L"Artist"; break;
    case BrowseKind::playlists: title = L"Playlist"; break;
    default: break;
    }
    auto details = showTracks() ? L"Artist" : L"Details";
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(title);
    ListView_SetColumn(list, 0, &column);
    column.pszText = const_cast<wchar_t*>(details);
    ListView_SetColumn(list, 1, &column);
    RECT bounds{};
    GetClientRect(hwnd, &bounds);
    layout(bounds.right, bounds.bottom);
    InvalidateRect(list, nullptr, FALSE);
}

void View::activate(int index) {
    if (index < 0 || index >= rowCount()) return;
    if (showTracks()) {
        play(false);
        return;
    }
    auto target = items[index].target;
    history.push_back({location, query, std::move(items), nextOffset, index, more, cached});
    location = std::move(target);
    SetDlgItemTextW(hwnd, IDC_QUERY, L"");
    startSearch();
    SetFocus(list);
}

void View::back() {
    if (history.empty()) return;
    auto outer = std::move(history.back());
    history.pop_back();
    location = std::move(outer.location);
    query = std::move(outer.query);
    SetDlgItemTextW(hwnd, IDC_QUERY, wide(query).c_str());
    KillTimer(hwnd, searchTimer);
    resetContent();
    const bool useCache = currentServer && (localLocation(location) || !app->store.networkAvailable(currentServer));
    if (useCache != outer.cached) {
        startSearch();
        SetFocus(list);
        return;
    }
    items = std::move(outer.items);
    nextOffset = outer.nextOffset;
    more = outer.more;
    cached = outer.cached;
    for (const auto& item : items) knownItems.emplace(item.target.kind, item.target.id);
    updateRowCount();
    if (outer.focused >= 0 && outer.focused < rowCount()) {
        ListView_SetItemState(list, outer.focused, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(list, outer.focused, FALSE);
    }
    SetFocus(list);
}

void View::updateStatus() {
    auto server = selected();
    std::wstring status;
    if (!server) status = L"Add a Navidrome or OpenSubsonic server to get started.";
    else if (loading && rowCount() == 0) status = L"Loading...";
    else if (!message.empty()) status = wide(message);
    else if (!app->reportError.empty()) status = wide(app->reportError);
    else {
        if (rowCount() == 0) status = showTracks() ? L"No tracks" : L"No results";
        else status = std::to_wstring(rowCount()) + (showTracks() ? L" tracks" : L" items");
        if (loading) status += L"  |  Loading more...";
        auto count = app->store.reportCount(server->id);
        if (count) status += L"  |  " + std::to_wstring(count) + L" play reports queued";
        auto error = app->maintenanceError(server->id);
        if (!error.empty()) status += L"  |  " + wide(error);
    }
    const bool manualOffline = app->store.offlineMode();
    if (server && (manualOffline || (cached && location.kind != BrowseKind::offline && !localLocation(location))))
        status = (manualOffline ? L"Offline  |  " : L"Server offline  |  ") + status;
    if (!history.empty()) status += L"  |  " + wide(location.title);
    if (server && servers.size() > 1) status += L"  |  " + wide(server->name);
    SetDlgItemTextW(hwnd, IDC_STATUS, status.c_str());
}

void View::updateButtons() {
    const bool haveServer = selected() != nullptr;
    const bool haveRows = ListView_GetSelectedCount(list) > 0;
    EnableWindow(GetDlgItem(hwnd, IDC_QUERY), haveServer);
    EnableWindow(GetDlgItem(hwnd, IDC_OPEN), haveRows);
    EnableWindow(GetDlgItem(hwnd, IDC_CACHE_MENU), haveRows);
    bool playable = haveRows && (showTracks() || albumActions());
    if (playable && !showTracks()) {
        for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
             i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
            if (!albumAt(i)) { playable = false; break; }
        }
    }
    for (int id : {IDC_PLAY, IDC_ENQUEUE}) EnableWindow(GetDlgItem(hwnd, id), playable);
}

std::vector<Track> View::selection() {
    std::vector<Track> selectedTracks;
    for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
         i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
        if (static_cast<std::size_t>(i) < tracks.size()) selectedTracks.push_back(tracks[i]);
    }
    return selectedTracks;
}

void View::beginDrag(const NMLISTVIEW& notification) {
    const auto* album = albumAt(notification.iItem);
    if (!album) return;
    draggedAlbum = *album;
    SetCapture(hwnd);
    POINT point = notification.ptAction;
    MapWindowPoints(list, hwnd, &point, 1);
    drag(point, false);
}

void View::cancelDrag() {
    draggedAlbum.reset();
    if (GetCapture() == hwnd) ReleaseCapture();
}

void View::drag(POINT point, bool released) {
    if (!draggedAlbum) return;
    mlDropItemStruct drop{};
    drop.type = ML_TYPE_ITEMRECORDLISTW;
    drop.p = point;
    if (!ClientToScreen(hwnd, &drop.p)) { cancelDrag(); return; }
    drop.flags = released ? ML_HANDLEDRAG_FLAG_NOCURSOR : 0;
    library(ML_IPC_HANDLEDRAG, reinterpret_cast<WPARAM>(&drop));
    if (released && draggedAlbum) {
        auto album = std::move(*draggedAlbum);
        cancelDrag();
        if (drop.result > 0) app->sendAlbums(currentServer, {std::move(album)}, AlbumAction::drop, drop);
    }
}

void View::play(bool enqueue) {
    if (!showTracks()) {
        std::vector<BrowseLocation> albums;
        for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
             i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
            auto* album = albumAt(i);
            if (!album) return;
            albums.push_back(*album);
        }
        if (!albums.empty()) app->sendAlbums(currentServer, std::move(albums),
                                             enqueue ? AlbumAction::enqueue : AlbumAction::play);
        return;
    }
    auto items = selection();
    if (items.empty()) return;
    if (!app->store.networkAvailable(currentServer)) {
        items.erase(std::remove_if(items.begin(), items.end(),
            [](const Track& track) { return app->store.track(track.key).bytes <= 0; }), items.end());
        if (items.empty()) {
            message = "No selected tracks are downloaded.";
            updateStatus();
            return;
        }
    }
    message.clear();
    updateStatus();
    if (!enqueue) player(IPC_DELETE);
    for (const auto& track : items) {
        auto url = app->playlistFile(track);
        auto title = wide(playlistTitle(track));
        enqueueFileWithMetaStructW item{url.c_str(), title.c_str(), track.duration ? track.duration : -1};
        player(IPC_ENQUEUEFILEW, reinterpret_cast<WPARAM>(&item));
    }
    if (!enqueue) player(IPC_STARTPLAY);
}

void View::cache(bool pin) {
    auto items = selection();
    if (items.empty()) return;
    app->store.pin(items, pin);
    app->wakeMaintenance();
    refresh();
}

void View::popup(int button) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    constexpr UINT firstServer = 2000;
    if (button == IDC_CACHE_MENU) {
        AppendMenuW(menu, MF_STRING, IDC_KEEP_OFFLINE, L"Keep offline");
        AppendMenuW(menu, MF_STRING, IDC_UNPIN, L"Unpin");
    } else {
        if (!showTracks()) {
            AppendMenuW(menu, MF_STRING | (textList ? MF_CHECKED : 0), IDC_TEXT_LIST, L"Text list");
        }
        AppendMenuW(menu, MF_STRING | (app->store.offlineMode() ? MF_CHECKED : 0), IDC_OFFLINE, L"Work offline");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        for (size_t i = 0; i < servers.size(); ++i) {
            auto label = wide(servers[i].name);
            for (size_t pos = 0; (pos = label.find(L'&', pos)) != label.npos; pos += 2)
                label.insert(pos, 1, L'&');
            AppendMenuW(menu, MF_STRING | (servers[i].id == currentServer ? MF_CHECKED : 0),
                        firstServer + i, label.c_str());
        }
        AppendMenuW(menu, MF_STRING, IDC_ADD_SERVER, L"Add server...");
        AppendMenuW(menu, selected() ? MF_STRING : MF_GRAYED, IDC_SETTINGS, L"Server settings...");
    }
    RECT anchor{};
    GetWindowRect(GetDlgItem(hwnd, button), &anchor);
    MLSKINNEDPOPUP popup{};
    popup.cbSize = sizeof(popup);
    popup.hmenu = menu;
    popup.hwnd = hwnd;
    popup.fuFlags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON |
        (button == IDC_VIEW_MENU ? TPM_RIGHTALIGN : TPM_LEFTALIGN | TPM_BOTTOMALIGN);
    popup.x = button == IDC_VIEW_MENU ? anchor.right : anchor.left;
    popup.y = button == IDC_VIEW_MENU ? anchor.bottom : anchor.top;
    popup.skinStyle = SMS_USESKINFONT;
    const UINT chosen = static_cast<UINT>(library(ML_IPC_TRACKSKINNEDPOPUPEX, reinterpret_cast<WPARAM>(&popup)));
    DestroyMenu(menu);
    if (chosen >= firstServer && chosen - firstServer < servers.size()) {
        currentServer = servers[chosen - firstServer].id;
        resetLocation();
    } else if (chosen) command(chosen, 0);
}

void View::command(int id, int notification) {
    switch (id) {
    case IDC_ADD_SERVER: configure(hwnd); break;
    case IDC_SETTINGS: if (auto server = selected()) configure(hwnd, *server); break;
    case IDC_VIEW_MENU: case IDC_CACHE_MENU: popup(id); break;
    case IDC_QUERY:
        if (notification == EN_CHANGE) SetTimer(hwnd, searchTimer, 300, nullptr);
        break;
    case IDC_SEARCH: startSearch(); break;
    case IDC_OFFLINE:
        app->store.setOfflineMode(!app->store.offlineMode());
        app->wakeMaintenance(true);
        startSearch();
        break;
    case IDC_BACK: back(); break;
    case IDC_OPEN: activate(ListView_GetNextItem(list, -1, LVNI_SELECTED)); break;
    case IDC_TEXT_LIST:
        textList = !textList;
        app->store.saveTextCatalog(textList);
        changePresentation();
        break;
    case IDC_PLAY: play(false); break;
    case IDC_ENQUEUE: play(true); break;
    case IDC_KEEP_OFFLINE: cache(true); break;
    case IDC_UNPIN: cache(false); break;
    }
}

void View::layout(int width, int height) {
    const int dpi = GetDpiForWindow(hwnd);
    const int gap = std::max(1, MulDiv(3, dpi, 96));
    HDC dc = GetDC(hwnd);
    auto previous = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0)));
    TEXTMETRICW metrics{};
    GetTextMetricsW(dc, &metrics);
    const int row = std::max(MulDiv(20, dpi, 96), int(metrics.tmHeight) + gap * 2);
    auto controlWidth = [&](int id, int padding) {
        wchar_t label[48]{};
        GetDlgItemTextW(hwnd, id, label, 48);
        RECT extent{};
        DrawTextW(dc, label, -1, &extent, DT_CALCRECT | DT_SINGLELINE);
        return int(extent.right) + MulDiv(padding, dpi, 96);
    };
    auto move = [&](int id, int x, int y, int w, int h) {
        SetWindowPos(GetDlgItem(hwnd, id), nullptr, x, y, std::max(0, w), std::max(0, h),
                     SWP_NOACTIVATE | SWP_NOZORDER);
    };
    auto visible = [&](int id, bool show) { ShowWindow(GetDlgItem(hwnd, id), show ? SW_SHOWNA : SW_HIDE); };
    int left = gap;
    auto place = [&](int id, int top, int padding) {
        const int w = controlWidth(id, padding);
        move(id, left, top, w, row);
        left += w + gap;
    };
    const int footer = std::max(row + gap * 2, height - row - gap);
    visible(IDC_BACK, !history.empty());
    if (!history.empty()) place(IDC_BACK, gap, 8);
    place(IDC_SEARCH_LABEL, gap, 0);
    const int menuWidth = controlWidth(IDC_VIEW_MENU, 9);
    const int menuLeft = std::max(0, width - gap - menuWidth);
    move(IDC_QUERY, left, gap, menuLeft - gap - left, row);
    move(IDC_VIEW_MENU, menuLeft, gap, menuWidth, row);
    const int listTop = row + gap * 2;
    move(IDC_TRACKS, gap, listTop, width - gap * 2, footer - gap - listTop);
    left = gap;
    auto action = [&](int id, bool show) {
        visible(id, show);
        if (show) place(id, footer, 10);
    };
    action(IDC_ADD_SERVER, !currentServer);
    action(IDC_PLAY, currentServer && (showTracks() || albumActions()));
    action(IDC_ENQUEUE, currentServer && (showTracks() || albumActions()));
    action(IDC_OPEN, currentServer && !showTracks());
    action(IDC_CACHE_MENU, currentServer && showTracks());
    move(IDC_STATUS, left, footer, width - gap - left, row);
    SelectObject(dc, previous);
    ReleaseDC(hwnd, dc);
    resizeColumns();
}

void View::resizeColumns() {
    RECT bounds{};
    GetClientRect(list, &bounds);
    const auto width = std::max(0L, bounds.right - 2);
    if (showTiles()) {
        ListView_SetColumnWidth(list, 0, width);
        for (int i = 1; i < 5; ++i) ListView_SetColumnWidth(list, i, 0);
    } else if (showTracks()) {
        const int weights[] = {33, 20, 24, 8, 15};
        for (int i = 0; i < 5; ++i) ListView_SetColumnWidth(list, i, MulDiv(width, weights[i], 100));
    } else {
        ListView_SetColumnWidth(list, 0, MulDiv(width, 2, 3));
        ListView_SetColumnWidth(list, 1, MulDiv(width, 1, 3));
        for (int i = 2; i < 5; ++i) ListView_SetColumnWidth(list, i, 0);
    }
}

LRESULT CALLBACK searchProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR view) {
    if (msg == WM_GETDLGCODE && w == VK_RETURN)
        return DefSubclassProc(hwnd, msg, w, l) | DLGC_WANTMESSAGE;
    if (msg == WM_KEYDOWN && w == VK_RETURN) {
        SendMessageW(reinterpret_cast<HWND>(view), WM_COMMAND, IDC_SEARCH, 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, w, l);
}

INT_PTR CALLBACK viewProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto view = reinterpret_cast<View*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    try {
        if (msg == WM_INITDIALOG && app) {
            view = new View(hwnd, static_cast<BrowseKind>(l));
            SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(view));
            app->activeView = hwnd;
            SetWindowSubclass(GetDlgItem(hwnd, IDC_QUERY), searchProc, 1, reinterpret_cast<DWORD_PTR>(hwnd));
            view->populateServers();
            return TRUE;
        }
        if (!view) return FALSE;
        switch (msg) {
        case WM_MOUSEMOVE: case WM_LBUTTONUP:
            if (GetCapture() == hwnd) {
                POINT point{static_cast<short>(LOWORD(l)), static_cast<short>(HIWORD(l))};
                view->drag(point, msg == WM_LBUTTONUP);
            }
            break;
        case WM_CANCELMODE: case WM_CAPTURECHANGED: view->cancelDrag(); break;
        case WM_KEYDOWN: if (w == VK_ESCAPE) view->cancelDrag(); break;
        case WM_SIZE: view->layout(LOWORD(l), HIWORD(l)); return TRUE;
        case WM_COMMAND: view->command(LOWORD(w), HIWORD(w)); return TRUE;
        case WM_NOTIFY: {
            auto notification = reinterpret_cast<NMHDR*>(l);
            if (notification->idFrom != IDC_TRACKS) break;
            if (notification->code == LVN_BEGINDRAG) {
                view->beginDrag(*reinterpret_cast<NMLISTVIEW*>(l));
                return TRUE;
            }
            if (notification->code == LVN_GETDISPINFOW) {
                view->display(*reinterpret_cast<NMLVDISPINFOW*>(l));
                return TRUE;
            }
            if (notification->code == NM_DBLCLK)
                view->activate(reinterpret_cast<NMITEMACTIVATE*>(l)->iItem);
            if (notification->code == LVN_KEYDOWN) {
                auto key = reinterpret_cast<NMLVKEYDOWN*>(l)->wVKey;
                if (key == VK_ESCAPE) view->cancelDrag();
                if (key == VK_RETURN) view->activate(ListView_GetNextItem(view->list, -1, LVNI_SELECTED));
            }
            if (notification->code == LVN_ITEMCHANGED || notification->code == LVN_ODSTATECHANGED)
                view->updateButtons();
            if (notification->code == LVN_ITEMCHANGED) {
                const auto& change = *reinterpret_cast<const NMLISTVIEW*>(l);
                if ((change.uNewState & LVIS_FOCUSED) && !(change.uOldState & LVIS_FOCUSED))
                    view->prefetchTrack(change.iItem);
            }
            break;
        }
        case WM_TIMER:
            if (w == viewTimer) view->refresh();
            if (w == searchTimer) view->command(IDC_SEARCH, 0);
            return TRUE;
        case profilesChanged: view->populateServers(); return TRUE;
        case viewReady:
            if (auto result = app->takeResult(hwnd)) view->receive(std::move(*result));
            return TRUE;
        case ArtworkLoader::readyMessage: view->receiveArtwork(); return TRUE;
        case moreRequested: view->fetchMore(); return TRUE;
        case WM_DESTROY:
            if (app) app->forgetView(hwnd);
            SetWindowLongPtrW(hwnd, DWLP_USER, 0);
            delete view;
            return TRUE;
        }
    } catch (const std::exception& error) {
        if (msg == WM_TIMER) SetDlgItemTextW(hwnd, IDC_STATUS, wide(error.what()).c_str());
        else showError(hwnd, error);
    }
    return FALSE;
}

std::filesystem::path settingsDirectory() {
    auto wideDirectory = reinterpret_cast<const wchar_t*>(player(getIniDirectoryW));
    if (reinterpret_cast<UINT_PTR>(wideDirectory) > 1 && *wideDirectory)
        return std::filesystem::path(wideDirectory);
    auto iniDirectory = reinterpret_cast<const char*>(player(IPC_GETINIDIRECTORY));
    if (reinterpret_cast<UINT_PTR>(iniDirectory) > 1 && *iniDirectory) {
        int size = MultiByteToWideChar(CP_ACP, 0, iniDirectory, -1, nullptr, 0);
        if (size > 1) {
            std::wstring directory(size, L'\0');
            MultiByteToWideChar(CP_ACP, 0, iniDirectory, -1, directory.data(), size);
            directory.pop_back();
            return std::filesystem::path(directory);
        }
    }
    throw Error("WACUP did not provide its profile directory.");
}

UINT_PTR addTreeItem(const wchar_t* title, UINT_PTR parent, bool branch) {
    std::wstring label = title;
    MLTREEITEMW item{sizeof(item), 0, parent, label.data(), 0, branch,
                    branch ? MLTREEIMAGE_BRANCH : MLTREEIMAGE_DEFAULT};
    library(ML_IPC_TREEITEM_ADDW, reinterpret_cast<WPARAM>(&item));
    if (!item.id) throw Error("Tangerine Stream could not add its Media Library entry.");
    return item.id;
}

int init() {
    try {
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
        if (!InitCommonControlsEx(&controls)) throw Error("Windows could not initialize Media Library controls.");
        const auto settings = settingsDirectory();
        app = std::make_unique<App>(settings / L"Plugins" / pluginName);
        if (!SetWindowSubclass(plugin.hwndWinampParent, playerProc, playerSubclass, 0))
            throw Error("WACUP playback notifications could not be attached.");
        app->attached = true;
        if (!SetTimer(plugin.hwndWinampParent, playbackTimer, 350, nullptr))
            throw Error("WACUP playback tracking could not start.");
        app->treeId = addTreeItem(pluginName, 0, true);
        app->navigation.emplace(app->treeId, BrowseKind::albums);
        for (const auto& category : categories)
            app->navigation.emplace(addTreeItem(category.name, app->treeId, false), category.kind);
        // WACUP reads the second line of its Preferences row from this per-profile catalog.
        const auto descriptions = settings / L"plugin_descriptions.ini";
        wchar_t existing[256]{};
        GetPrivateProfileStringW(L"library", L"ml_tangerinestream.dll", L"", existing,
                                 static_cast<DWORD>(std::size(existing)), descriptions.c_str());
        if (!*existing)
            WritePrivateProfileStringW(L"library", L"ml_tangerinestream.dll",
                L"Navidrome and OpenSubsonic browsing by COOLTRAINER", descriptions.c_str());
        { std::unique_lock lock(inputMutex); inputProfile = app.get(); }
        return 0;
    } catch (const std::exception& error) {
        quit();
        showError(plugin.hwndLibraryParent, error);
        return 1;
    }
}

void quit() {
    { std::unique_lock lock(inputMutex); inputProfile = nullptr; }
    if (!app) return;
    if (app->attached) {
        KillTimer(plugin.hwndWinampParent, playbackTimer);
        RemoveWindowSubclass(plugin.hwndWinampParent, playerProc, playerSubclass);
    }
    if (app->treeId) {
        auto handle = library(ML_IPC_TREEITEM_GETHANDLE, app->treeId);
        if (handle) library(ML_IPC_TREEITEM_DELETE, handle);
    }
    if (app->activeView && IsWindow(app->activeView)) DestroyWindow(app->activeView);
    app.reset();
}

INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR) {
    if (!app) return 0;
    try {
        if (type == ML_MSG_TREE_ONCREATEVIEW) {
            auto entry = app->navigation.find(static_cast<UINT_PTR>(a));
            if (entry != app->navigation.end())
                return reinterpret_cast<INT_PTR>(CreateDialogParamW(plugin.hDllInstance,
                    MAKEINTRESOURCEW(IDD_LIBRARY), reinterpret_cast<HWND>(b), viewProc,
                    static_cast<LPARAM>(entry->second)));
        }
        if (type == ML_MSG_CONFIG) {
            auto servers = app->store.servers();
            auto selected = std::find_if(servers.begin(), servers.end(),
                [](const auto& s) { return s.id == app->selectedServer; });
            if (selected == servers.end()) selected = servers.begin();
            configure(reinterpret_cast<HWND>(a), selected == servers.end() ? Server{} : *selected);
            return TRUE;
        }
    } catch (const std::exception& error) { showError(plugin.hwndLibraryParent, error); }
    return 0;
}

} // namespace
} // namespace tangerine

extern "C" __declspec(dllexport) winampMediaLibraryPlugin* winampGetMediaLibraryPlugin() {
    return &tangerine::plugin;
}

extern "C" __declspec(dllexport) int __cdecl tangerineInputQueryV1(
    tangerine::InputRequest request, const wchar_t* filename, const wchar_t* field,
    wchar_t* dest, size_t capacity, HANDLE* lease) {
    try { return tangerine::inputQuery(request, filename, field, dest, capacity, lease); }
    catch (...) { return 0; }
}
