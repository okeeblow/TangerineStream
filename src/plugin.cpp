#include "playback.hpp"
#include "artwork.hpp"
#include "browse_sort.hpp"
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
#include <tuple>

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
constexpr auto selectionChanged = WM_APP + 5;
constexpr UINT_PTR playbackTimer = 0x7151;
constexpr UINT_PTR viewTimer = 1;
constexpr UINT_PTR searchTimer = 2;
constexpr UINT_PTR playerSubclass = 0x7151;
constexpr UINT getIniDirectoryW = 1335; // Winamp 5.58+
using Clock = std::chrono::steady_clock;

int init();
void quit();
INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR c);

char description[] = "Tangerine Stream v0.5.0";
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

struct Navigation {
    BrowseKind category;
    Id server;
    bool branch;
};

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
    double seconds = 0;
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
    std::string reports, favorites, downloads, playback;
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
        if (!status.playback.empty()) return status.playback;
        if (!status.reports.empty()) return status.reports;
        if (!status.favorites.empty()) return status.favorites;
        return status.downloads;
    }
    void playbackError(Id server, std::string message) {
        std::scoped_lock lock(maintenanceMutex);
        serviceStatus[server].playback = std::move(message);
    }
    void sample(Id startedKey = 0);
    void sendAlbums(std::vector<BrowseLocation> albums, AlbumAction action, mlDropItemStruct drop = {});
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
    Id filterServer = 0;
    UINT_PTR treeId = 0;
    std::map<UINT_PTR, Navigation> navigation;
    std::vector<std::pair<Id, std::string>> serverBranches;
    const UINT navigationChanged = RegisterWindowMessageW(L"ml_tangerinestream.navigationChanged");
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
        bool rolling = false;
        owner->playbackError(track.server, {});
        *lease = owner->store.openCached(id, cached);
        if (*lease != INVALID_HANDLE_VALUE) {
            value = cached.wstring();
            owner->store.touch(id);
        } else {
            if (!owner->store.networkAvailable(track.server)) return 0;
            try {
                const auto profile = owner->store.server(track.server);
                const bool metered = meteredInternet();
                value = wide(connect(profile)->stream(track, metered ? profile.metered : profile.unmetered).url);
                rolling = !metered;
            } catch (const std::exception& error) {
                owner->playbackError(track.server, std::string("Playback: ") + error.what());
                return 0;
            }
        }
        if (value.size() >= capacity) return 0;
        owner->preparePlayback(track, filename, rolling);
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

void App::sendAlbums(std::vector<BrowseLocation> albums, AlbumAction action, mlDropItemStruct drop) {
    auto resolve = [this, albums = std::move(albums)] {
        std::vector<Track> result;
        for (const auto& album : albums) {
            auto tracks = albumTracks(album.server, album);
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
    struct Session { Server profile; std::unique_ptr<ServerApi> api; };
    std::map<Id, Session> sessions;
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
        const auto started = Clock::now();
        try {
            ready.cached = search.cached || search.location.kind == BrowseKind::offline ||
                localLocation(search.location) || !store.networkAvailable(search.server);
            if (!ready.cached) {
                try {
                    auto profile = store.server(search.server);
                    auto& session = sessions[profile.id];
                    if (!session.api || profile.native != session.profile.native ||
                        profile.password != session.profile.password) {
                        session.api = connect(profile);
                        session.profile = std::move(profile);
                    }
                    const bool filterPlaylist = search.location.kind == BrowseKind::playlistTracks &&
                        !search.query.empty();
                    const auto remoteQuery = filterPlaylist ? std::string{} : search.query;
                    ready.page = session.api->browse(search.location, remoteQuery, search.offset);
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
                    sessions.erase(search.server);
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
        ready.seconds = std::chrono::duration<double>(Clock::now() - started).count();
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
                    try {
                        const auto profile = store.server(track.server);
                        if (!meteredInternet()) downloadAudio(store, track, profile, *connect(profile), false, stopping);
                    }
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
                const bool upgrade = profile.cacheFavorites && profile.upgradeFavorites && !meteredInternet();
                for (const auto& track : store.pendingPins(profile.id, upgrade)) {
                    if (stopping) return;
                    if (!store.networkAvailable(profile.id)) break;
                    try {
                        downloadAudio(store, track, profile, connection(), meteredInternet(), stopping);
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

void refreshNavigation();
void selectCategory(BrowseKind category, Id server);
constexpr UINT allServersCommand = 2000;
constexpr int sortCommand = 1500;
constexpr int directionCommand = 1499;

LRESULT CALLBACK playerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if (app && msg && msg == app->navigationChanged) {
        try {
            refreshNavigation();
            if (app->activeView) SendMessageW(app->activeView, profilesChanged, 0, 0);
        } catch (const std::exception& error) { showError(plugin.hwndLibraryParent, error); }
        return 0;
    }
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

struct FormatChoice { const char* key; const wchar_t* label; };
constexpr FormatChoice formats[] = {
    {"auto", L"Automatic"}, {"raw", L"Original"}, {"mp3", L"MP3"}, {"opus", L"Opus"},
    {"aac", L"AAC"}, {"vorbis", L"Vorbis"}, {"flac", L"FLAC"}
};

struct Settings {
    std::vector<Server> profiles;
    size_t selected;
    int budget;
    bool unified;
    bool loadingForm = false;

    static void displayQuality(HWND hwnd, int format, int rate, const StreamQuality& quality) {
        const auto choice = std::find_if(std::begin(formats), std::end(formats),
            [&](const FormatChoice& f) { return quality.format == f.key; });
        SendDlgItemMessageW(hwnd, format, CB_SETCURSEL, choice == std::end(formats) ? 0 : choice - formats, 0);
        SetDlgItemInt(hwnd, rate, quality.kbps, FALSE);
    }

    static StreamQuality readQuality(HWND hwnd, int format, int rate) {
        const auto index = SendDlgItemMessageW(hwnd, format, CB_GETCURSEL, 0, 0);
        return {formats[index >= 0 && static_cast<size_t>(index) < std::size(formats) ? index : 0].key,
                static_cast<int>(GetDlgItemInt(hwnd, rate, nullptr, FALSE))};
    }

    static void enableQualityFields(HWND hwnd) {
        EnableWindow(GetDlgItem(hwnd, IDC_UNMETERED_RATE),
                     readQuality(hwnd, IDC_UNMETERED_FORMAT, IDC_UNMETERED_RATE).format != "raw");
        EnableWindow(GetDlgItem(hwnd, IDC_METERED_RATE),
                     readQuality(hwnd, IDC_METERED_FORMAT, IDC_METERED_RATE).format != "raw");
        EnableWindow(GetDlgItem(hwnd, IDC_UPGRADE_FAVORITES), IsDlgButtonChecked(hwnd, IDC_FAVORITES) == BST_CHECKED);
    }

    void updateTreeOption(HWND hwnd) const {
        size_t defined = 0;
        for (size_t i = 0; i < profiles.size(); ++i) {
            const auto& s = profiles[i];
            const bool filled = i == selected ?
                GetWindowTextLengthW(GetDlgItem(hwnd, IDC_URL)) && GetWindowTextLengthW(GetDlgItem(hwnd, IDC_USERNAME)) &&
                    GetWindowTextLengthW(GetDlgItem(hwnd, IDC_PASSWORD)) :
                !s.url.empty() && !s.user.empty() && !s.password.empty();
            if (s.id || filled) ++defined;
        }
        CheckDlgButton(hwnd, IDC_UNIFIED, (defined <= 1 || unified) ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_UNIFIED), defined > 1);
    }

    void read(HWND hwnd) {
        auto& s = profiles.at(selected);
        s.name = getText(hwnd, IDC_NAME);
        s.url = getText(hwnd, IDC_URL);
        s.user = getText(hwnd, IDC_USERNAME);
        s.password = getText(hwnd, IDC_PASSWORD);
        s.native = IsDlgButtonChecked(hwnd, IDC_NATIVE) == BST_CHECKED;
        s.cacheFavorites = IsDlgButtonChecked(hwnd, IDC_FAVORITES) == BST_CHECKED;
        s.upgradeFavorites = IsDlgButtonChecked(hwnd, IDC_UPGRADE_FAVORITES) == BST_CHECKED;
        s.unmetered = readQuality(hwnd, IDC_UNMETERED_FORMAT, IDC_UNMETERED_RATE);
        s.metered = readQuality(hwnd, IDC_METERED_FORMAT, IDC_METERED_RATE);
    }

    void select(HWND hwnd, size_t index) {
        loadingForm = true;
        selected = index;
        auto list = GetDlgItem(hwnd, IDC_SERVERS);
        for (size_t i = 0; i < profiles.size(); ++i) {
            const auto& profile = profiles[i];
            auto label = wide(profile.name.empty() ? (profile.url.empty() ? "New server" : profile.url) : profile.name);
            LVITEMW row{};
            row.mask = LVIF_TEXT;
            row.iItem = static_cast<int>(i);
            row.pszText = label.data();
            if (row.iItem == ListView_GetItemCount(list)) ListView_InsertItem(list, &row);
            else ListView_SetItemText(list, row.iItem, 0, row.pszText);
        }
        ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetItemState(list, static_cast<int>(index), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(list, static_cast<int>(index), FALSE);
        const auto& s = profiles.at(index);
        SetDlgItemTextW(hwnd, IDC_NAME, wide(s.name).c_str());
        SetDlgItemTextW(hwnd, IDC_URL, wide(s.url).c_str());
        SetDlgItemTextW(hwnd, IDC_USERNAME, wide(s.user).c_str());
        SetDlgItemTextW(hwnd, IDC_PASSWORD, wide(s.password).c_str());
        SendDlgItemMessageW(hwnd, IDC_URL, EM_SETREADONLY, s.id != 0, 0);
        SendDlgItemMessageW(hwnd, IDC_USERNAME, EM_SETREADONLY, s.id != 0, 0);
        SendDlgItemMessageW(hwnd, IDC_PASSWORD, EM_SETCUEBANNER, FALSE,
            reinterpret_cast<LPARAM>(s.id ? L"Leave blank to keep saved password" : L""));
        CheckDlgButton(hwnd, IDC_NATIVE, s.native ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_FAVORITES, s.cacheFavorites ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_UPGRADE_FAVORITES, s.upgradeFavorites ? BST_CHECKED : BST_UNCHECKED);
        displayQuality(hwnd, IDC_UNMETERED_FORMAT, IDC_UNMETERED_RATE, s.unmetered);
        displayQuality(hwnd, IDC_METERED_FORMAT, IDC_METERED_RATE, s.metered);
        enableQualityFields(hwnd);
        loadingForm = false;
        updateTreeOption(hwnd);
    }
};

INT_PTR CALLBACK settingsFormProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (msg == WM_INITDIALOG) {
        settings = reinterpret_cast<Settings*>(l);
        SetWindowLongPtrW(hwnd, DWLP_USER, l);
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        for (HWND control = GetWindow(hwnd, GW_CHILD); control; control = GetWindow(control, GW_HWNDNEXT)) {
            switch (GetDlgCtrlID(control)) {
            case IDC_SERVERS: {
                skinWindow(control, SKINNEDWND_TYPE_LISTVIEW, SWLVS_FULLROWSELECT | SWLVS_SELALWAYS | SWLVS_DOUBLEBUFFER);
                LVCOLUMNW column{};
                ListView_InsertColumn(control, 0, &column);
                break;
            }
            case IDC_METERED_FORMAT: case IDC_UNMETERED_FORMAT:
                skinWindow(control, SKINNEDWND_TYPE_COMBOBOX);
                for (const auto& f : formats)
                    SendMessageW(control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(f.label));
                break;
            case IDC_METERED_RATE: case IDC_UNMETERED_RATE:
                SendMessageW(control, EM_SETLIMITTEXT, 5, 0);
                [[fallthrough]];
            case IDC_NAME: case IDC_URL: case IDC_USERNAME: case IDC_PASSWORD: case IDC_BUDGET:
                skinWindow(control, SKINNEDWND_TYPE_EDIT, SWES_VCENTER);
                break;
            case IDC_UPGRADE_FAVORITES:
            case IDC_UNIFIED: case IDC_NATIVE: case IDC_FAVORITES: case IDC_ADD_SERVER: case IDOK: case IDCANCEL:
                skinWindow(control, SKINNEDWND_TYPE_BUTTON);
                break;
            default: skinWindow(control, SKINNEDWND_TYPE_STATIC); break;
            }
        }
        settings->select(hwnd, settings->selected);
        SetDlgItemInt(hwnd, IDC_BUDGET, settings->budget, FALSE);
        SendDlgItemMessageW(hwnd, IDC_NAME, EM_SETLIMITTEXT, 160, 0);
        SendDlgItemMessageW(hwnd, IDC_URL, EM_SETLIMITTEXT, 512, 0);
        SendDlgItemMessageW(hwnd, IDC_USERNAME, EM_SETLIMITTEXT, 160, 0);
        SendDlgItemMessageW(hwnd, IDC_PASSWORD, EM_SETLIMITTEXT, 512, 0);
        SendDlgItemMessageW(hwnd, IDC_BUDGET, EM_SETLIMITTEXT, 7, 0);
        return TRUE;
    }
    if (!settings) return FALSE;
    if (msg == WM_NOTIFY && !settings->loadingForm) {
        const auto* header = reinterpret_cast<const NMHDR*>(l);
        if (header->idFrom == IDC_SERVERS && header->code == LVN_ITEMCHANGED) {
            const auto& change = *reinterpret_cast<const NMLISTVIEW*>(l);
            if (change.iItem >= 0 && (change.uNewState & LVIS_SELECTED) && !(change.uOldState & LVIS_SELECTED)) {
                settings->read(hwnd);
                settings->select(hwnd, static_cast<size_t>(change.iItem));
            }
        }
        return FALSE;
    }
    if (msg != WM_COMMAND) return FALSE;
    if (LOWORD(w) == IDCANCEL) {
        EndDialog(GetParent(hwnd), IDCANCEL);
        return TRUE;
    }
    try {
        if ((HIWORD(w) == CBN_SELCHANGE && (LOWORD(w) == IDC_METERED_FORMAT || LOWORD(w) == IDC_UNMETERED_FORMAT)) ||
            (HIWORD(w) == BN_CLICKED && LOWORD(w) == IDC_FAVORITES)) {
            settings->enableQualityFields(hwnd);
            return TRUE;
        }
        if (LOWORD(w) == IDC_UNIFIED && HIWORD(w) == BN_CLICKED) {
            settings->unified = IsDlgButtonChecked(hwnd, IDC_UNIFIED) == BST_CHECKED;
            return TRUE;
        }
        if (!settings->loadingForm && HIWORD(w) == EN_CHANGE &&
            (LOWORD(w) == IDC_URL || LOWORD(w) == IDC_USERNAME || LOWORD(w) == IDC_PASSWORD)) {
            settings->updateTreeOption(hwnd);
            return TRUE;
        }
        if (LOWORD(w) == IDC_ADD_SERVER) {
            settings->read(hwnd);
            auto empty = std::find_if(settings->profiles.begin(), settings->profiles.end(), [](const Server& s) {
                return !s.id && s.url.empty() && s.user.empty() && s.name.empty() && s.password.empty();
            });
            auto index = static_cast<size_t>(empty - settings->profiles.begin());
            if (empty == settings->profiles.end()) settings->profiles.emplace_back();
            settings->select(hwnd, index);
            SetFocus(GetDlgItem(hwnd, IDC_NAME));
            return TRUE;
        }
        if (LOWORD(w) != IDOK) return FALSE;
        settings->read(hwnd);
        std::vector<Server> updated;
        size_t selection = 0;
        std::set<std::pair<std::string, std::string>> accounts;
        for (size_t i = 0; i < settings->profiles.size(); ++i) {
            auto s = settings->profiles[i];
            if (!s.id && s.name.empty() && s.url.empty() && s.user.empty() && s.password.empty()) continue;
            try {
                s.url = normalizeUrl(s.url);
                if (s.user.empty() || (!s.id && s.password.empty())) throw Error("Enter the server username and password.");
                if (!accounts.emplace(s.url, s.user).second) throw Error("This server and username already have a profile.");
            } catch (const std::exception&) {
                settings->select(hwnd, i);
                throw;
            }
            if (s.name.empty()) s.name = s.user + " @ " + s.url;
            if (i == settings->selected) selection = updated.size();
            updated.push_back(std::move(s));
        }
        BOOL valid = FALSE;
        auto budget = GetDlgItemInt(hwnd, IDC_BUDGET, &valid, FALSE);
        if (!valid || budget > 1'048'576) throw Error("Enter a rolling cache size from 0 to 1048576 MiB.");
        app->store.saveProfiles(updated, static_cast<int>(budget), settings->unified);
        settings->profiles = std::move(updated);
        settings->selected = selection;
        EndDialog(GetParent(hwnd), IDOK);
    } catch (const std::exception& error) { showError(GetParent(hwnd), error); }
    return TRUE;
}

INT_PTR CALLBACK settingsProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    // The ML dialog skin draws control borders relative to a borderless library view.
    // Keep the modal caption on the outer window and skin its child form.
    if (msg == WM_INITDIALOG) {
        HWND form = CreateDialogParamW(plugin.hDllInstance, MAKEINTRESOURCEW(IDD_SERVER_FORM),
                                       hwnd, settingsFormProc, l);
        if (!form) { EndDialog(hwnd, -1); return TRUE; }
        SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(form));
        return TRUE;
    }
    if (msg == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) {
        auto form = reinterpret_cast<HWND>(GetWindowLongPtrW(hwnd, DWLP_USER));
        if (form) return SendMessageW(form, msg, w, l);
    }
    return FALSE;
}

void configure(HWND hwnd, Id preferred, bool add = false) {
    Settings settings{app->store.servers(), 0, app->store.rollingMiB(), app->store.unifiedTree()};
    if (add || settings.profiles.empty()) {
        settings.selected = settings.profiles.size();
        settings.profiles.emplace_back();
    } else {
        for (size_t i = 0; i < settings.profiles.size(); ++i)
            if (settings.profiles[i].id == preferred) settings.selected = i;
    }
    if (DialogBoxParamW(plugin.hDllInstance, MAKEINTRESOURCEW(IDD_SERVER), hwnd, settingsProc,
                        reinterpret_cast<LPARAM>(&settings)) == IDOK) {
        app->selectedServer = settings.profiles.empty() ? 0 : settings.profiles[settings.selected].id;
        app->wakeMaintenance(true);
        PostMessageW(plugin.hwndWinampParent, app->navigationChanged, 0, 0);
    }
}

constexpr wchar_t listSkinProc[] = L"Tangerine Stream.ListSkinProc";

LRESULT CALLBACK libraryListProc(HWND list, UINT msg, WPARAM w, LPARAM l) {
    auto skinProc = reinterpret_cast<WNDPROC>(GetPropW(list, listSkinProc));
    if (msg == WM_LBUTTONDOWN) {
        LVHITTESTINFO hit{};
        hit.pt = {static_cast<short>(LOWORD(l)), static_cast<short>(HIWORD(l))};
        if (ListView_HitTest(list, &hit) < 0) {
            // The skin consumes empty-space clicks. Let SysListView32 track its own marquee.
            auto nativeProc = reinterpret_cast<WNDPROC>(GetClassLongPtrW(list, GCLP_WNDPROC));
            return CallWindowProcW(nativeProc, list, msg, w, l);
        }
    } else if (msg == WM_NCDESTROY) {
        if (reinterpret_cast<WNDPROC>(GetWindowLongPtrW(list, GWLP_WNDPROC)) == libraryListProc)
            SetWindowLongPtrW(list, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(skinProc));
        RemovePropW(list, listSkinProc);
    }
    return CallWindowProcW(skinProc, list, msg, w, l);
}

class View {
public:
    View(HWND window, const Navigation& node)
        : hwnd(window), list(GetDlgItem(hwnd, IDC_TRACKS)), category(node.category), location(categoryLocation(category)), treeServer(node.server) {
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
        if (!skinWindow(list, SKINNEDWND_TYPE_LISTVIEW, SWLVS_DOUBLEBUFFER | SWLVS_SELALWAYS)) {
            legacySkin = library(ML_IPC_SKIN_LISTVIEW, reinterpret_cast<WPARAM>(list));
        }
        // WACUP installs its window procedure above the common-controls subclass chain.
        auto proc = GetWindowLongPtrW(list, GWLP_WNDPROC);
        if (SetPropW(list, listSkinProc, reinterpret_cast<HANDLE>(proc)) &&
            !SetWindowLongPtrW(list, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(libraryListProc)))
            RemovePropW(list, listSkinProc);
        skinWindow(GetDlgItem(hwnd, IDC_QUERY), SKINNEDWND_TYPE_EDIT, SWES_VCENTER);
        for (int id : {IDC_SEARCH_LABEL, IDC_STATUS})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_STATIC);
        for (int id : {IDC_VIEW_MENU, IDC_SOURCE_MENU, IDC_ADD_SERVER, IDC_BACK, IDC_OPEN, IDC_PLAY, IDC_ENQUEUE, IDC_CACHE_MENU})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_BUTTON);
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
    void setCategory(BrowseKind kind) { category = kind; }
    void populateServers();
    void receive(SearchResult result);
    void receiveArtwork();
    void fetchMore();
    void display(NMLVDISPINFOW& display);
    void activate(int index);
    void command(int id, int notification);
    void refresh();
    void updateButtons();
    void queueSelectionUpdate() {
        if (!selectionPending) selectionPending = PostMessageW(hwnd, selectionChanged, 0, 0) != FALSE;
    }
    void updateSelection();
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
    struct Source {
        Id server;
        int offset = 0;
        bool more = true;
        bool cached = false;
        std::string error;
    };
    struct History {
        BrowseLocation location;
        std::string query;
        std::vector<LibraryItem> items;
        std::vector<Source> sources;
        size_t nextSource;
        int focused;
        std::string order;
    };
    struct ImageSlot {
        int row;
        std::uint64_t read;
    };

    BrowseKind category;
    BrowseLocation location;
    std::vector<BrowseLocation> draggedAlbums;
    std::string query;
    SortChoice order = defaultSort(BrowseKind::albums);
    bool descending = false;
    std::string savedOrder() const { return std::string(descending ? "-" : "+") + order.key; }
    void applyOrder();
    void chooseOrder(SortChoice choice, bool reverse);
    std::vector<History> history;
    std::vector<Server> servers;
    std::vector<LibraryItem> items;
    std::vector<Track> tracks;
    std::set<std::tuple<Id, BrowseKind, std::string>> knownItems;
    std::set<Id> knownTracks;
    std::vector<Source> sources;
    size_t nextSource = 0;
    bool loading = false, more = false, morePosted = false;
    bool selectionPending = false;
    double browseSeconds = 0;
    std::wstring selectionSummary;
    std::string message;
    LRESULT legacySkin = 0;
    const Id treeServer;
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
    std::vector<BrowseLocation> selectedAlbums() const;
    const Server* selected() const;
};

const Server* View::selected() const {
    auto it = std::find_if(servers.begin(), servers.end(),
        [this](const Server& server) { return server.id == currentServer; });
    return it == servers.end() ? nullptr : &*it;
}

void View::populateServers() {
    servers = app->store.servers();
    currentServer = treeServer ? treeServer : app->selectedServer;
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
    sources.clear();
    nextSource = 0;
    browseSeconds = 0;
    selectionSummary.clear();
    message.clear();
    ListView_SetItemCount(list, 0);
    items.clear();
    tracks.clear();
    knownItems.clear();
    knownTracks.clear();
    resetImages();
    order = defaultSort(location.kind);
    descending = order.descending;
    auto saved = app->store.browseOrder(location.kind);
    for (const auto& choice : sortChoices(location.kind)) {
        if (!saved.empty() && saved.compare(1, saved.npos, choice.key) == 0) {
            order = choice;
            descending = saved.front() == '-';
            break;
        }
    }
    const Id scope = location.server ? location.server : (treeServer ? treeServer : app->filterServer);
    if (scope) currentServer = scope;
    else if (!servers.empty()) currentServer = servers.front().id;
    for (const auto& server : servers) {
        if (!scope || server.id == scope)
            sources.push_back({server.id, 0, true, false, {}});
    }
    changePresentation();
    updateButtons();
    updateStatus();
}

void View::startSearch() {
    KillTimer(hwnd, searchTimer);
    query = getText(hwnd, IDC_QUERY);
    resetContent();
    more = !sources.empty();
    fetchMore();
}

void View::fetchMore() {
    morePosted = false;
    if (!more || loading || !currentServer) return;
    loading = true;
    updateButtons();
    updateStatus();
    const auto& source = sources.at(nextSource);
    app->search(hwnd, source.server, location, query, source.offset, source.cached);
}

void View::scheduleMore() {
    if (!more || loading || morePosted) return;
    morePosted = PostMessageW(hwnd, moreRequested, 0, 0) != 0;
}

void View::updateRowCount() {
    ListView_SetItemCountEx(list, rowCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    InvalidateRect(list, nullptr, FALSE);
    updateSelection();
}

void View::receive(SearchResult result) {
    if (result.restart) { startSearch(); return; }
    browseSeconds += result.seconds;
    loading = false;
    auto& source = sources.at(nextSource);
    source.cached = result.cached;
    source.error = std::move(result.error);
    const auto oldCount = rowCount();
    const bool emptyPage = result.page.items.empty() && result.page.tracks.empty();
    for (auto& item : result.page.items) {
        item.target.server = source.server;
        if (knownItems.emplace(source.server, item.target.kind, item.target.id).second)
            items.push_back(std::move(item));
    }
    const bool pagedTracks = location.kind == BrowseKind::tracks || location.kind == BrowseKind::offline;
    for (auto& track : result.page.tracks) {
        if (!pagedTracks || knownTracks.insert(track.key).second) tracks.push_back(std::move(track));
    }
    // A repeated server page must not get stuck in an endless scroll loop.
    source.more = source.error.empty() && result.page.more && source.offset <= INT_MAX - 2 * pageSize &&
                  (emptyPage || rowCount() != oldCount);
    source.offset += pageSize;
    more = false;
    for (size_t step = 1; step <= sources.size(); ++step) {
        const auto i = (nextSource + step) % sources.size();
        if (sources[i].more) { nextSource = i; more = true; break; }
    }
    // A single live album list already comes from the server in alphabetical order.
    // Other global orders need every source page before exposing their row indices.
    const bool streamOrder = std::string_view(order.key) == "sequence" ||
        (sources.size() == 1 && !source.cached && location.kind == BrowseKind::albums &&
         query.empty() && std::string_view(order.key) == "name" && !descending);
    if (!streamOrder && more) {
        updateStatus();
        scheduleMore();
        return;
    }
    if (!streamOrder) applyOrder();
    else updateRowCount();
    if (!oldCount || !streamOrder) prefetchTrack(0);
    if (emptyPage || rowCount() < 12 || (more && sources[nextSource].offset == 0)) scheduleMore();
}

void View::chooseOrder(SortChoice choice, bool reverse) {
    const auto previous = savedOrder();
    order = choice;
    descending = reverse;
    if (previous == savedOrder() && std::string_view(order.key) != "random") return;
    app->store.saveBrowseOrder(location.kind, savedOrder());
    if (loading || more || std::string_view(order.key) == "sequence") startSearch();
    else applyOrder();
}

void View::applyOrder() {
    const int displayed = ListView_GetItemCount(list);
    const int focused = ListView_GetNextItem(list, -1, LVNI_FOCUSED);
    std::set<int> selectedRows;
    for (int row = ListView_GetNextItem(list, -1, LVNI_SELECTED); row >= 0;
         row = ListView_GetNextItem(list, row, LVNI_SELECTED)) selectedRows.insert(row);
    const auto indices = showTracks() ? sortRows(tracks, order, descending) : sortRows(items, order, descending);
    if (displayed && !indices.empty()) {
        ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        resetImages();
    }
    changePresentation();
    updateRowCount();
    if (displayed) {
        for (size_t row = 0; row < indices.size(); ++row) {
            const int old = static_cast<int>(indices[row]);
            const auto state = (old == focused ? LVIS_FOCUSED : 0) |
                               (selectedRows.count(old) ? LVIS_SELECTED : 0);
            if (state) ListView_SetItemState(list, static_cast<int>(row), state, state);
            if (old == focused) ListView_EnsureVisible(list, static_cast<int>(row), FALSE);
        }
        updateSelection();
    }
}

void View::refresh() {
    if (!loading && location.kind != BrowseKind::offline && !localLocation(location)) {
        for (const auto& source : sources) {
            if (source.cached && app->store.networkAvailable(source.server)) { startSearch(); return; }
        }
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
        if (app->artwork.request(artGeneration, items[row].target.server, row, tileSize, items[row]))
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
    SetDlgItemTextW(hwnd, IDC_VIEW_MENU, order.label);
    const Id filter = treeServer ? treeServer : app->filterServer;
    const auto source = std::find_if(servers.begin(), servers.end(),
        [filter](const Server& profile) { return profile.id == filter; });
    std::wstring caption = source == servers.end() ? L"<All>" : wide(source->name);
    for (size_t pos = 0; (pos = caption.find(L'&', pos)) != caption.npos; pos += 2)
        caption.insert(pos, 1, L'&');
    SetDlgItemTextW(hwnd, IDC_SOURCE_MENU, caption.c_str());
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
    auto style = GetWindowLongPtrW(list, GWL_STYLE);
    const auto type = tiles ? LVS_ICON : LVS_REPORT;
    if ((style & LVS_TYPEMASK) != type) {
        SetWindowLongPtrW(list, GWL_STYLE, (style & ~LVS_TYPEMASK) | type);
        SetWindowPos(list, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    ListView_SetView(list, tiles ? LV_VIEW_ICON : LV_VIEW_DETAILS);
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_FULLROWSELECT, tiles ? 0 : LVS_EX_FULLROWSELECT);
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
    history.push_back({location, query, std::move(items), std::move(sources), nextSource, index, savedOrder()});
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
    if (savedOrder() != outer.order) { startSearch(); return; }
    for (const auto& source : outer.sources) {
        const bool useCache = location.kind == BrowseKind::offline || localLocation(location) ||
                              !app->store.networkAvailable(source.server);
        if (source.offset && useCache != source.cached) {
            startSearch();
            SetFocus(list);
            return;
        }
    }
    items = std::move(outer.items);
    sources = std::move(outer.sources);
    nextSource = outer.nextSource;
    more = !sources.empty() && sources[nextSource].more;
    for (const auto& item : items) knownItems.emplace(item.target.server, item.target.kind, item.target.id);
    updateRowCount();
    if (outer.focused >= 0 && outer.focused < rowCount()) {
        ListView_SetItemState(list, outer.focused, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(list, outer.focused, FALSE);
    }
    if (more && sources[nextSource].offset == 0) scheduleMore();
    SetFocus(list);
}

void View::updateSelection() {
    selectionPending = false;
    const auto started = Clock::now();
    const int selectedRows = ListView_GetSelectedCount(list);
    const int count = selectedRows ? selectedRows : rowCount();
    std::int64_t seconds = 0, songs = 0;
    bool knownDuration = true, knownCount = true;
    auto include = [&](int i) {
        const auto duration = showTracks() ? tracks[i].duration : items[i].duration;
        const int n = showTracks() ? 1 : items[i].songCount;
        knownDuration &= duration > 0 || n == 0;
        knownCount &= n >= 0;
        seconds += duration;
        songs += std::max(0, n);
    };
    if (selectedRows) {
        for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0 && i < rowCount();
             i = ListView_GetNextItem(list, i, LVNI_SELECTED)) include(i);
    } else {
        for (int i = 0; i < count; ++i) include(i);
    }
    auto numberOf = [](std::int64_t n, const wchar_t* one, const wchar_t* many) {
        return std::to_wstring(n) + L" " + (n == 1 ? one : many);
    };
    selectionSummary = showTracks() ? numberOf(count, L"track", L"tracks") : numberOf(count, L"item", L"items");
    if (selectedRows) selectionSummary += L" selected";
    if (!showTracks() && count && knownCount) selectionSummary += L"  |  " + numberOf(songs, L"track", L"tracks");
    if (knownDuration && seconds > 0) {
        auto padded = [](std::int64_t n) { return (n < 10 ? L"0" : L"") + std::to_wstring(n); };
        selectionSummary += L" [" + (seconds >= 3600 ? std::to_wstring(seconds / 3600) + L":" +
            padded(seconds / 60 % 60) : std::to_wstring(seconds / 60)) + L":" + padded(seconds % 60) + L"]";
    }
    const auto elapsed = selectedRows ? std::chrono::duration<double>(Clock::now() - started).count() : browseSeconds;
    if (count && elapsed > 0) {
        wchar_t timing[64]{};
        std::swprintf(timing, std::size(timing), L" in %.4f sec.", elapsed);
        selectionSummary += timing;
    }
    updateStatus();
    updateButtons();
}

void View::updateStatus() {
    auto server = selected();
    std::wstring status;
    if (!server) status = L"Add a Navidrome or OpenSubsonic server to get started.";
    else if ((loading || more) && !ListView_GetItemCount(list))
        status = rowCount() ? L"Loading... " + std::to_wstring(rowCount()) : L"Loading...";
    else if (!message.empty()) status = wide(message);
    else if (!app->reportError.empty()) status = wide(app->reportError);
    else {
        if (rowCount() == 0) status = showTracks() ? L"No tracks" : L"No results";
        else status = selectionSummary;
        if (loading) status += L"  |  Loading more...";
        const bool offline = app->store.offlineMode();
        std::int64_t reports = 0;
        for (const auto& source : sources) {
            reports += app->store.reportCount(source.server);
            auto error = source.error.empty() ? app->maintenanceError(source.server) : source.error;
            if (error.empty() && !offline && source.cached && location.kind != BrowseKind::offline &&
                !localLocation(location)) error = "Server offline";
            if (!error.empty()) {
                const auto profile = std::find_if(servers.begin(), servers.end(),
                    [&](const Server& s) { return s.id == source.server; });
                status += L"  |  ";
                if (sources.size() > 1 && profile != servers.end()) status += wide(profile->name) + L": ";
                status += wide(error);
            }
        }
        if (reports) status += L"  |  " + std::to_wstring(reports) + L" play reports queued";
    }
    if (server && app->store.offlineMode()) status = L"Offline  |  " + status;
    if (!history.empty()) status += L"  |  " + wide(location.title);
    if (server && servers.size() > 1 && sources.size() == 1) status += L"  |  " + wide(server->name);
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
    draggedAlbums = ListView_GetItemState(list, notification.iItem, LVIS_SELECTED) ?
        selectedAlbums() : std::vector<BrowseLocation>{*album};
    if (draggedAlbums.empty()) return;
    SetCapture(hwnd);
    POINT point = notification.ptAction;
    MapWindowPoints(list, hwnd, &point, 1);
    drag(point, false);
}

void View::cancelDrag() {
    draggedAlbums.clear();
    if (GetCapture() == hwnd) ReleaseCapture();
}

void View::drag(POINT point, bool released) {
    if (draggedAlbums.empty()) return;
    mlDropItemStruct drop{};
    drop.type = ML_TYPE_ITEMRECORDLISTW;
    drop.p = point;
    if (!ClientToScreen(hwnd, &drop.p)) { cancelDrag(); return; }
    drop.flags = released ? ML_HANDLEDRAG_FLAG_NOCURSOR : 0;
    library(ML_IPC_HANDLEDRAG, reinterpret_cast<WPARAM>(&drop));
    if (released && !draggedAlbums.empty()) {
        auto albums = std::move(draggedAlbums);
        cancelDrag();
        if (drop.result > 0) app->sendAlbums(std::move(albums), AlbumAction::drop, drop);
    }
}

std::vector<BrowseLocation> View::selectedAlbums() const {
    std::vector<BrowseLocation> albums;
    for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
         i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
        auto* album = albumAt(i);
        if (!album) return {};
        albums.push_back(*album);
    }
    return albums;
}

void View::play(bool enqueue) {
    if (!showTracks()) {
        auto albums = selectedAlbums();
        if (!albums.empty()) app->sendAlbums(std::move(albums),
                                             enqueue ? AlbumAction::enqueue : AlbumAction::play);
        return;
    }
    auto items = selection();
    if (items.empty()) return;
    items.erase(std::remove_if(items.begin(), items.end(), [](const Track& track) {
        return !app->store.networkAvailable(track.server) && app->store.track(track.key).bytes <= 0;
    }), items.end());
    if (items.empty()) {
        message = "No selected tracks are downloaded.";
        updateStatus();
        return;
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
    if (button == IDC_CACHE_MENU) {
        AppendMenuW(menu, MF_STRING, IDC_KEEP_OFFLINE, L"Keep offline");
        AppendMenuW(menu, MF_STRING, IDC_UNPIN, L"Unpin");
    } else if (button == IDC_VIEW_MENU) {
        const auto& choices = sortChoices(location.kind);
        for (size_t i = 0; i < choices.size(); ++i) {
            const bool active = std::string_view(choices[i].key) == order.key;
            AppendMenuW(menu, MF_STRING | (active ? MF_CHECKED : 0), sortCommand + i, choices[i].label);
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        if (std::string_view(order.key) != "sequence" && std::string_view(order.key) != "random")
            AppendMenuW(menu, MF_STRING | (descending ? MF_CHECKED : 0), directionCommand, L"Descending");
        if (!showTracks()) {
            AppendMenuW(menu, MF_STRING | (textList ? MF_CHECKED : 0), IDC_TEXT_LIST, L"Text list");
        }
        AppendMenuW(menu, MF_STRING | (app->store.offlineMode() ? MF_CHECKED : 0), IDC_OFFLINE, L"Work offline");
    } else {
        const Id filter = treeServer ? treeServer : app->filterServer;
        AppendMenuW(menu, MF_STRING | (!filter ? MF_CHECKED : 0), allServersCommand, L"<All>");
        for (size_t i = 0; i < servers.size(); ++i) {
            auto label = wide(servers[i].name);
            for (size_t pos = 0; (pos = label.find(L'&', pos)) != label.npos; pos += 2)
                label.insert(pos, 1, L'&');
            AppendMenuW(menu, MF_STRING | (filter == servers[i].id ? MF_CHECKED : 0),
                        allServersCommand + 1 + i, label.c_str());
        }
    }
    RECT anchor{};
    GetWindowRect(GetDlgItem(hwnd, button), &anchor);
    MLSKINNEDPOPUP popup{};
    popup.cbSize = sizeof(popup);
    popup.hmenu = menu;
    popup.hwnd = hwnd;
    popup.fuFlags = TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON |
        (button == IDC_CACHE_MENU ? TPM_LEFTALIGN | TPM_BOTTOMALIGN : TPM_RIGHTALIGN);
    popup.x = button == IDC_CACHE_MENU ? anchor.left : anchor.right;
    popup.y = button == IDC_CACHE_MENU ? anchor.top : anchor.bottom;
    popup.skinStyle = SMS_USESKINFONT;
    const UINT chosen = static_cast<UINT>(library(ML_IPC_TRACKSKINNEDPOPUPEX, reinterpret_cast<WPARAM>(&popup)));
    DestroyMenu(menu);
    if (chosen) command(chosen, 0);
}

void View::command(int id, int notification) {
    if (id >= static_cast<int>(allServersCommand)) {
        const auto index = static_cast<size_t>(id - allServersCommand);
        if (index <= servers.size()) selectCategory(category, index ? servers[index - 1].id : 0);
        return;
    }
    const auto& choices = sortChoices(location.kind);
    if (id >= sortCommand && static_cast<size_t>(id - sortCommand) < choices.size()) {
        const auto& choice = choices[id - sortCommand];
        chooseOrder(choice, choice.descending);
        return;
    }
    switch (id) {
    case directionCommand: chooseOrder(order, !descending); break;
    case IDC_ADD_SERVER: configure(hwnd, currentServer, true); break;
    case IDC_VIEW_MENU: case IDC_SOURCE_MENU: case IDC_CACHE_MENU: popup(id); break;
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
    const HWND sortButton = GetDlgItem(hwnd, IDC_VIEW_MENU);
    auto buttonFont = reinterpret_cast<HFONT>(SendMessageW(sortButton, WM_GETFONT, 0, 0));
    auto oldFont = buttonFont ? SelectObject(dc, buttonFont) : nullptr;
    int menuWidth = 0;
    // WACUP's skinned button reports the ideal size of its current caption.
    // Measure the whole set in the button font so the toolbar stays still.
    for (const auto& choice : sortChoices(location.kind)) {
        RECT text{};
        DrawTextW(dc, choice.label, -1, &text, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        menuWidth = std::max(menuWidth, static_cast<int>(text.right));
    }
    if (oldFont) SelectObject(dc, oldFont);
    menuWidth += MulDiv(24, dpi, 96);
    const int menuLeft = std::max(0, width - gap - menuWidth);
    const int sourceWidth = std::min(controlWidth(IDC_SOURCE_MENU, 12), width / 3);
    const int sourceLeft = std::max(0, menuLeft - gap - sourceWidth);
    move(IDC_QUERY, left, gap, sourceLeft - gap - left, row);
    move(IDC_SOURCE_MENU, sourceLeft, gap, sourceWidth, row);
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
            view = new View(hwnd, *reinterpret_cast<const Navigation*>(l));
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
            if (notification->code == LVN_ODSTATECHANGED) view->queueSelectionUpdate();
            if (notification->code == LVN_ITEMCHANGED) {
                const auto& change = *reinterpret_cast<const NMLISTVIEW*>(l);
                if ((change.uOldState ^ change.uNewState) & LVIS_SELECTED) view->queueSelectionUpdate();
                if ((change.uNewState & LVIS_FOCUSED) && !(change.uOldState & LVIS_FOCUSED))
                    view->prefetchTrack(change.iItem);
            }
            break;
        }
        case WM_TIMER:
            if (w == viewTimer) view->refresh();
            if (w == searchTimer) view->command(IDC_SEARCH, 0);
            return TRUE;
        case profilesChanged:
            if (l) view->setCategory(static_cast<BrowseKind>(w));
            view->populateServers();
            return TRUE;
        case viewReady:
            if (auto result = app->takeResult(hwnd)) view->receive(std::move(*result));
            return TRUE;
        case ArtworkLoader::readyMessage: view->receiveArtwork(); return TRUE;
        case moreRequested: view->fetchMore(); return TRUE;
        case selectionChanged: view->updateSelection(); return TRUE;
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

// In the grouped tree the root represents <All>; its view retains the requested category.
void selectCategory(BrowseKind category, Id server) {
    const bool grouped = !app->serverBranches.empty();
    UINT_PTR target = app->treeId;
    if (grouped && !server) {
        app->navigation.at(target).category = category;
    } else {
        const auto scope = grouped ? server : 0;
        const auto found = std::find_if(app->navigation.begin(), app->navigation.end(), [&](const auto& item) {
            return !item.second.branch && item.second.category == category && item.second.server == scope;
        });
        if (found == app->navigation.end()) return;
        target = found->first;
    }
    app->filterServer = server;
    if (server) app->selectedServer = server;
    const auto handle = library(ML_IPC_TREEITEM_GETHANDLE, target);
    if (handle && handle != library(ML_IPC_TREEITEM_GETSELECTED))
        library(ML_IPC_TREEITEM_SELECT, handle);
    else if (handle && app->activeView)
        SendMessageW(app->activeView, profilesChanged, static_cast<WPARAM>(category), 1);
}

void refreshNavigation() {
    const auto profiles = app->store.servers();
    std::vector<std::pair<Id, std::string>> branches;
    if (profiles.size() > 1 && !app->store.unifiedTree())
        for (const auto& profile : profiles) branches.emplace_back(profile.id, profile.name);
    if (!app->navigation.empty() && branches == app->serverBranches) return;

    const auto root = app->treeId;
    const auto rootHandle = library(ML_IPC_TREEITEM_GETHANDLE, root);
    const auto selectedHandle = library(ML_IPC_TREEITEM_GETSELECTED);
    MLTREEITEMINFOW info{};
    info.item.size = sizeof(info.item);
    info.handle = selectedHandle;
    info.mask = MLTI_ID;
    if (selectedHandle) library(ML_IPC_TREEITEM_GETINFOW, reinterpret_cast<WPARAM>(&info));
    const auto selected = app->navigation.find(info.item.id);
    const bool restore = selected != app->navigation.end();
    const auto category = restore ? selected->second.category : BrowseKind::albums;
    const auto filter = app->filterServer;
    if (restore && selectedHandle != rootHandle) library(ML_IPC_TREEITEM_SELECT, rootHandle);

    while (const auto child = library(ML_IPC_TREEITEM_GETCHILD, rootHandle)) {
        library(ML_IPC_TREEITEM_DELETE, child);
        if (library(ML_IPC_TREEITEM_GETCHILD, rootHandle) == child)
            throw Error("WACUP could not refresh the Tangerine Stream tree.");
    }
    app->navigation.clear();
    app->navigation.emplace(root, Navigation{BrowseKind::albums, 0, true});
    auto addCategories = [&](UINT_PTR parent, Id server) {
        for (const auto& entry : categories)
            app->navigation.emplace(addTreeItem(entry.name, parent, false), Navigation{entry.kind, server, false});
    };
    if (branches.empty()) addCategories(root, 0);
    for (const auto& [server, name] : branches) {
        const auto parent = addTreeItem(wide(name).c_str(), root, true);
        app->navigation.emplace(parent, Navigation{BrowseKind::albums, server, true});
        addCategories(parent, server);
    }
    app->serverBranches = std::move(branches);
    if (restore) selectCategory(category, filter);
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
        refreshNavigation();
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
            if (entry != app->navigation.end()) {
                if (!app->serverBranches.empty()) app->filterServer = entry->second.server;
                if (entry->second.server) app->selectedServer = entry->second.server;
                return reinterpret_cast<INT_PTR>(CreateDialogParamW(plugin.hDllInstance,
                    MAKEINTRESOURCEW(IDD_LIBRARY), reinterpret_cast<HWND>(b), viewProc,
                    reinterpret_cast<LPARAM>(&entry->second)));
            }
        }
        if (type == ML_MSG_CONFIG) {
            configure(reinterpret_cast<HWND>(a), app->selectedServer);
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
