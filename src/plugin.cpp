#include "playback.hpp"
#include "list_scroll.hpp"
#include "dpi.hpp"
#include "artwork.hpp"
#include "browse_sort.hpp"
#include "view_editor.hpp"
#include "host_metadata.hpp"
#include "resources.h"
#include "tangerine_product.h"
#include "input_bridge.hpp"
#include <fstream>
#include <shared_mutex>

#include <commctrl.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <climits>
#include <condition_variable>
#include <deque>
#include <future>
#include <functional>
#include <iterator>
#include <optional>
#include <set>
#include <thread>
#include <tuple>

#include "ml_sdk.hpp"
#include "host_profile.hpp"

namespace tangerine {
namespace {

constexpr wchar_t pluginName[] = TS_PRODUCT_NAME_W;
constexpr auto viewReady = WM_APP + 1;
constexpr auto profilesChanged = WM_APP + 2;
constexpr auto moreRequested = WM_APP + 4;
constexpr auto selectionChanged = WM_APP + 5;
constexpr auto artworkViewportChanged = WM_APP + 6;
constexpr auto detailsReady = WM_APP + 7;
constexpr auto filtersChanged = WM_APP + 8;
constexpr auto populateTiles = WM_APP + 9;
constexpr UINT_PTR playbackTimer = 0x7151;
constexpr UINT_PTR viewTimer = 1;
constexpr UINT_PTR searchTimer = 2;
constexpr UINT_PTR splitTimer = 4;
constexpr UINT_PTR playerSubclass = 0x7151;
using Clock = std::chrono::steady_clock;

int init();
void quit();
INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR c);

char description[] = TS_PRODUCT_NAME " v" TS_VERSION_STRING;
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

InputExtensions enabledExtensions() {
    // Mode 0 is the host's double-NUL list of extensions from loaded input modules.
    const auto raw = player(IPC_GET_EXTLISTW);
    if (static_cast<UINT_PTR>(raw) <= 1) return {};
    std::unique_ptr<void, decltype(&GlobalFree)> memory(reinterpret_cast<void*>(raw), GlobalFree);
    InputExtensions result;
    for (auto entry = static_cast<const wchar_t*>(memory.get()); *entry; entry += wcslen(entry) + 1) {
        const std::wstring_view group(entry);
        size_t pos = 0;
        while ((pos = group.find_first_not_of(L" ;*.\t", pos)) != group.npos) {
            auto end = group.find_first_of(L" ;\t", pos);
            auto suffix = utf8(group.substr(pos, end - pos));
            std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            result.insert(std::move(suffix));
            if (end == group.npos) break;
            pos = end + 1;
        }
    }
    return result;
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

std::wstring dialogText(HWND dialog, int item) {
    auto control = GetDlgItem(dialog, item);
    int length = GetWindowTextLengthW(control);
    std::wstring text(length + 1, L'\0');
    auto written = GetWindowTextW(control, text.data(), length + 1);
    text.resize(written);
    return text;
}

std::string getText(HWND dialog, int item) {
    return utf8(dialogText(dialog, item));
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

struct ViewDetails {
    ViewProjection projection;
    std::vector<Track> preview;
    std::optional<std::map<Id, std::string>> cacheChanges;
    std::string error;
    bool albumPreview = false;
};
using ViewTask = std::function<ViewDetails(const std::function<bool()>&)>;

struct Playing {
    Track track;
    int playlistPosition = -1;
    int outputPosition = 0;
    int state = 0;
    std::int64_t listened = 0;
    std::int64_t started = 0;
    Clock::time_point sampled;
    bool listenQueued = false;
};

struct ServiceStatus {
    std::string listenSubmissionError, favorites, downloads, playback;
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
    App(const std::filesystem::path& profile, api_service* services)
        : store(profile), playback(store, profile), artwork(store, http, plugin.hwndWinampParent, services),
          hostMetadata(services, store, playback, artwork) {
        try {
            refreshInputs();
            browser = std::thread([this] { browse(); });
            maintenance = std::thread([this] { maintain(); });
            listenWorker = std::thread([this] { submitPendingListens(); });
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
    void submitDetails(HWND view, ViewTask task) {
        {
            std::scoped_lock lock(searchMutex);
            ++detailsSerial;
            detailsView = view;
            detailsTask = std::move(task);
            details.reset();
        }
        searchReady.notify_one();
    }
    std::optional<ViewDetails> takeDetails(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (view != detailsView) return {};
        return std::exchange(details, {});
    }
    void cancelSearch(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (activeView != view) return;
        ++serial;
        clearDetails();
        request.reset();
        result.reset();
    }
    void forgetView(HWND view) {
        std::scoped_lock lock(searchMutex);
        if (activeView != view) return;
        activeView = nullptr;
        ++serial;
        clearDetails();
        request.reset();
        result.reset();
    }
    void wakeMaintenance(bool refresh = false) {
        {
            std::scoped_lock lock(maintenanceMutex);
            pendingMaintenance = true;
            refreshFavorites |= refresh;
            retryListens |= refresh;
        }
        maintenanceReady.notify_one();
        if (refresh) listenQueueReady.notify_one();
    }
    std::string maintenanceError(Id server) {
        std::scoped_lock lock(maintenanceMutex);
        const auto& status = serviceStatus[server];
        if (!status.playback.empty()) return status.playback;
        if (!status.listenSubmissionError.empty()) return status.listenSubmissionError;
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
    void sendTrackRecords(const std::vector<Track>& tracks, AlbumAction action, mlDropItemStruct drop);
    std::vector<Track> albumTracks(Id server, const BrowseLocation& album, bool downloadedOnly = true);
    void preparePlayback(const Track& track, const wchar_t* filename);
    void refreshInputs() {
        rememberInputs(enabledExtensions());
    }
    void rememberInputs(InputExtensions inputs) {
        std::scoped_lock lock(maintenanceMutex);
        extensions = std::move(inputs);
    }
    InputExtensions inputs() {
        std::scoped_lock lock(maintenanceMutex);
        return extensions;
    }
    Clock::time_point nextInputRefresh{};
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

    HttpClient http;
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
    std::string playbackTrackingError;

private:
    std::deque<AlbumJob> albumJobs;
    std::atomic_bool stopping{false};
    std::thread browser, maintenance, listenWorker;
    std::mutex searchMutex, maintenanceMutex;
    std::condition_variable searchReady, maintenanceReady, listenQueueReady;
    std::uint64_t serial = 0;
    std::uint64_t detailsSerial = 0;
    HWND detailsView = nullptr;
    ViewTask detailsTask;
    std::optional<ViewDetails> details;
    void clearDetails() { ++detailsSerial; detailsView = nullptr; detailsTask = {}; details.reset(); }
    std::optional<Search> request;
    std::optional<SearchResult> result;
    bool pendingMaintenance = false, refreshFavorites = false, retryListens = false;
    std::map<Id, ServiceStatus> serviceStatus;
    InputExtensions extensions;
    std::mutex sidecarMutex;
    Id sidecarKey = 0;
    std::filesystem::path sidecarReference;
    std::filesystem::path lyricsFile;
    Playing playing;
    bool sampling = false;
    std::deque<std::pair<Track, std::int64_t>> unsavedListens;

    void browse();
    void maintain();
    void submitPendingListens();
    void saveNewListens();
    void stop() {
        {
            std::scoped_lock lock(searchMutex, maintenanceMutex);
            stopping = true;
        }
        searchReady.notify_all();
        maintenanceReady.notify_all();
        listenQueueReady.notify_all();
        if (browser.joinable()) browser.join();
        if (maintenance.joinable()) maintenance.join();
        if (listenWorker.joinable()) listenWorker.join();
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

void App::preparePlayback(const Track& track, const wchar_t* filename) {
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

std::atomic_bool closingInput{false};

int inputQuery(InputRequest request, const wchar_t* filename, const wchar_t* field,
               wchar_t* dest, size_t capacity, HANDLE* lease, SourceAttempts* attempts) {
    // Host IPC can reenter. Snapshot enabled decoders on the host thread before borrowing the profile.
    auto inputs = request == InputRequest::begin ? enabledExtensions() : InputExtensions{};
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
    } else if ((request == InputRequest::begin || request == InputRequest::fetch) && lease && attempts) {
        *lease = INVALID_HANDLE_VALUE;
        attempts->temporary = false;
        std::filesystem::path file;
        if (request == InputRequest::begin) {
            owner->rememberInputs(std::move(inputs));
            owner->playbackError(track.server, {});
            owner->preparePlayback(track, filename);
            *lease = owner->store.openCached(id, file);
            if (*lease != INVALID_HANDLE_VALUE) {
                if (track.cachedFormat.empty()) attempts->excluded |= originalSource;
                owner->store.touch(id);
            } else if (owner->store.networkAvailable(track.server)) {
                return downloadNeeded;
            } else {
                owner->playbackError(track.server, "Playback: No playable cached copy is available offline.");
                return unavailable;
            }
        } else {
            auto cancelled = [&] { return closingInput || WaitForSingleObject(attempts->cancel, 0) != WAIT_TIMEOUT; };
            try {
                const auto profile = owner->store.server(track.server);
                *lease = prepareAudio(owner->store, owner->http, track, profile, owner->inputs(),
                                     attempts->excluded, cancelled, file, attempts->temporary, attempts->progress);
            } catch (const std::exception& error) {
                if (!cancelled()) owner->playbackError(track.server, std::string("Playback: ") + error.what());
                return unavailable;
            }
        }
        if (file.native().size() >= capacity) {
            CloseHandle(*lease);
            *lease = INVALID_HANDLE_VALUE;
            if (attempts->temporary) DeleteFileW(file.c_str());
            return unavailable;
        }
        std::copy_n(file.c_str(), file.native().size() + 1, dest);
        return fileReady;
    } else return 0;
    lstrcpynW(dest, value.c_str(), static_cast<int>(std::min<size_t>(capacity, INT_MAX)));
    return 1;
}


std::vector<Track> App::albumTracks(Id server, const BrowseLocation& album, bool downloadedOnly) {
    if (store.networkAvailable(server) && !localLocation(album)) {
        try {
            auto page = http.connect(store.server(server))->browse(album, {}, 0);
            store.rememberPage(server, album, true, 0, page);
            return std::move(page.tracks);
        } catch (const ConnectionError&) { store.connectionFailed(server); }
    }
    std::vector<Track> downloaded;
    for (int offset = 0; ; offset += pageSize) {
        auto page = store.cachedPage(server, album, {}, offset);
        for (auto& track : page.tracks)
            if (!downloadedOnly || track.bytes > 0) downloaded.push_back(std::move(track));
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
        sendTrackRecords(job.tracks.get(), job.action, job.drop);
    }
}

void App::sendTrackRecords(const std::vector<Track>& tracks, AlbumAction action, mlDropItemStruct drop) {
    if (tracks.empty()) {
        MessageBoxW(activeView, L"No tracks from the selected albums are available.", pluginName, MB_OK);
        return;
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
    if (action != AlbumAction::drop) {
        // Bit 1 makes the explicit Play / Enqueue choice override the ML default.
        mlSendToWinampStruct send{ML_TYPE_ITEMRECORDLISTW, &list, action == AlbumAction::enqueue ? 3 : 2};
        library(ML_IPC_SENDTOWINAMP, reinterpret_cast<WPARAM>(&send));
        return;
    }
    drop.flags = 0;
    drop.result = 0;
    drop.data = &list;
    library(ML_IPC_HANDLEDROP, reinterpret_cast<WPARAM>(&drop));
}

void App::browse() {
    struct Session { Server profile; std::unique_ptr<ServerApi> api; };
    std::map<Id, Session> sessions;
    for (;;) {
        Search search;
        {
            std::unique_lock lock(searchMutex);
            searchReady.wait(lock, [this] { return stopping || detailsTask || request.has_value(); });
            if (stopping) return;
            if (detailsTask) {
                const auto generation = detailsSerial;
                auto task = std::exchange(detailsTask, {});
                lock.unlock();
                ViewDetails ready;
                try {
                    ready = task([&] {
                        std::scoped_lock guard(searchMutex);
                        return stopping || detailsSerial != generation;
                    });
                } catch (const std::exception& e) { ready.error = e.what(); }
                lock.lock();
                if (!stopping && detailsSerial == generation) {
                    details = std::move(ready);
                    PostMessageW(detailsView, detailsReady, 0, 0);
                }
                continue;
            }
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
                    if (!session.api || profile.native != session.profile.native || profile.url != session.profile.url ||
                        profile.password != session.profile.password) {
                        session.api = http.connect(profile);
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
        {
            std::unique_lock lock(maintenanceMutex);
            maintenanceReady.wait_for(lock, seconds(5), [this] { return stopping || pendingMaintenance; });
            if (stopping) return;
            pendingMaintenance = false;
            if (refreshFavorites) nextFavorites.clear();
            refreshFavorites = false;
        }
        std::vector<Server> profiles;
        try {
            profiles = store.servers();
            store.trim();
        } catch (...) {}
        for (const auto& profile : profiles) {
            if (stopping) return;
            std::unique_ptr<ServerApi> api;
            auto connection = [&]() -> ServerApi& {
                if (!api) api = http.connect(store.server(profile.id));
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
                const auto enabled = inputs();
                for (const auto& track : store.pendingPins(profile.id, upgrade, enabled)) {
                    if (stopping) return;
                    if (!store.networkAvailable(profile.id)) break;
                    try {
                        downloadAudio(store, http, track, profile, connection(), meteredInternet(), stopping, enabled);
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

void App::submitPendingListens() {
    using namespace std::chrono;
    std::map<Id, Clock::time_point> retryAfter;
    for (;;) {
        {
            std::unique_lock lock(maintenanceMutex);
            listenQueueReady.wait_for(lock, seconds(15), [this] { return stopping || retryListens; });
            if (stopping) return;
            if (retryListens) retryAfter.clear();
            retryListens = false;
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
                // Give each server a turn even when another has a large backlog.
                for (int i = 0; i < 4 && !stopping; ++i) {
                    if (!store.networkAvailable(profile.id)) break;
                    auto batch = store.nextListenBatch(profile.id);
                    if (batch.empty()) break;
                    if (!api) api = http.connect(store.server(profile.id));
                    api->submitListens(batch);
                    store.acknowledgeListens(batch);
                }
            } catch (const std::exception& error) {
                if (dynamic_cast<const ConnectionError*>(&error)) store.connectionFailed(profile.id);
                failure = std::string("Could not submit listens: ") + error.what();
                retryAfter[profile.id] = Clock::now() + seconds(30);
            }
            std::scoped_lock lock(maintenanceMutex);
            serviceStatus[profile.id].listenSubmissionError = std::move(failure);
        }
    }
}

void App::saveNewListens() {
    while (!unsavedListens.empty()) {
        try {
            const auto& [track, listenedAtMs] = unsavedListens.front();
            store.enqueueListen(track, listenedAtMs);
            unsavedListens.pop_front();
            playbackTrackingError.clear();
        } catch (const std::exception& error) {
            playbackTrackingError = error.what();
            return;
        }
    }
}

void App::sample(Id startedKey) {
    if (sampling) return;
    struct Reset { bool& flag; ~Reset() { flag = false; } } reset{sampling};
    sampling = true;
    saveNewListens();
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
    if (playing.track.key && !playing.listenQueued && playing.listened >= threshold) {
        unsavedListens.emplace_back(playing.track, playing.started);
        playing.listenQueued = true;
        saveNewListens();
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
constexpr int coverSizeCommand = 1600;
constexpr int coverSizes[] = {0, 100, 200, 300, 400, 600}; // 0 follows display DPI.

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
            if (msg == WM_TIMER) {
                if (Clock::now() >= app->nextInputRefresh) {
                    app->nextInputRefresh = Clock::now() + std::chrono::seconds(30);
                    app->refreshInputs();
                }
                app->finishAlbums();
            }
            auto key = msg == WM_WA_IPC ? app->playback.key(reinterpret_cast<const wchar_t*>(w)) : 0;
            app->sample(key);
        } catch (const std::exception& error) { app->playbackTrackingError = error.what(); }
    }
    return DefSubclassProc(hwnd, msg, w, l);
}

struct FormatChoice { const char* key; const wchar_t* label; };
constexpr FormatChoice formats[] = {
    {"auto", L"Automatic"}, {"raw", L"Original"}, {"mp3", L"MP3"}, {"opus", L"Opus"},
    {"aac", L"AAC"}, {"vorbis", L"Vorbis"}, {"flac", L"FLAC"}, {"wav", L"WAV"}
};

struct Settings {
    std::vector<Server> profiles;
    size_t selected;
    int budget;
    bool unified;
    InputExtensions inputs;
    bool loadingForm = false;
    std::map<Id, Server> savedAccounts{};
    std::vector<Id> removed{};
    std::future<std::vector<Server>> saving{};
    size_t savedSelection = 0;
    INT_PTR (__cdecl *skinDialogMessage)(HWND, UINT, WPARAM, LPARAM) = nullptr;

    bool sameAccount(Id id, const std::string& url, const std::string& user) const {
        if (!id) return false;
        const auto& saved = savedAccounts.at(id);
        return saved.user == user && sameServerEndpoint(saved.url, url);
    }

    void passwordHint(HWND hwnd) const {
        const bool keep = selected < profiles.size() &&
            sameAccount(profiles[selected].id, getText(hwnd, IDC_URL), getText(hwnd, IDC_USERNAME));
        SendDlgItemMessageW(hwnd, IDC_PASSWORD, EM_SETCUEBANNER, FALSE,
            reinterpret_cast<LPARAM>(keep ? L"Leave blank to keep saved password" : L""));
        const bool replacing = selected < profiles.size() && profiles[selected].id && !keep;
        const wchar_t* note = L"";
        if (replacing || !removed.empty()) note = L"Cached music and pending listens stay with their account.";
        else if (profiles.empty()) note = L"Choose Add server to connect a library.";
        SetDlgItemTextW(hwnd, IDC_ACCOUNT_NOTE, note);
    }

    void displayQuality(HWND hwnd, int format, int rate, const StreamQuality& quality) const {
        auto box = GetDlgItem(hwnd, format);
        SendMessageW(box, CB_RESETCONTENT, 0, 0);
        for (const auto& f : formats) {
            const bool available = !strcmp(f.key, "auto") || !strcmp(f.key, "raw") || acceptsCodec(inputs, f.key);
            if (!available && quality.format != f.key) continue;
            std::wstring label = f.label;
            if (!available) label += L" (unavailable)";
            const auto index = SendMessageW(box, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            if (index < 0) continue;
            SendMessageW(box, CB_SETITEMDATA, index, &f - formats);
            if (quality.format == f.key) SendMessageW(box, CB_SETCURSEL, index, 0);
        }
        SetDlgItemInt(hwnd, rate, quality.kbps, FALSE);
    }

    static StreamQuality readQuality(HWND hwnd, int format, int rate) {
        const auto selected = SendDlgItemMessageW(hwnd, format, CB_GETCURSEL, 0, 0);
        const auto index = SendDlgItemMessageW(hwnd, format, CB_GETITEMDATA, selected, 0);
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
        if (profiles.empty()) return;
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
        while (ListView_GetItemCount(list) > static_cast<int>(profiles.size()))
            ListView_DeleteItem(list, ListView_GetItemCount(list) - 1);
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
        ListView_SetColumnWidth(list, 0, listViewportWidth(list));
        const bool hasProfile = !profiles.empty();
        if (hasProfile) {
            ListView_SetItemState(list, static_cast<int>(index), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(list, static_cast<int>(index), FALSE);
        }
        const Server blank{};
        const auto& s = hasProfile ? profiles.at(index) : blank;
        SetDlgItemTextW(hwnd, IDC_NAME, wide(s.name).c_str());
        SetDlgItemTextW(hwnd, IDC_URL, wide(s.url).c_str());
        SetDlgItemTextW(hwnd, IDC_USERNAME, wide(s.user).c_str());
        SetDlgItemTextW(hwnd, IDC_PASSWORD, wide(s.password).c_str());
        passwordHint(hwnd);
        CheckDlgButton(hwnd, IDC_NATIVE, s.native ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_FAVORITES, s.cacheFavorites ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_UPGRADE_FAVORITES, s.upgradeFavorites ? BST_CHECKED : BST_UNCHECKED);
        displayQuality(hwnd, IDC_UNMETERED_FORMAT, IDC_UNMETERED_RATE, s.unmetered);
        displayQuality(hwnd, IDC_METERED_FORMAT, IDC_METERED_RATE, s.metered);
        enableQualityFields(hwnd);
        for (int id : {IDC_NAME, IDC_URL, IDC_USERNAME, IDC_PASSWORD, IDC_NATIVE, IDC_FAVORITES,
                       IDC_METERED_FORMAT, IDC_UNMETERED_FORMAT, IDC_REMOVE_SERVER})
            EnableWindow(GetDlgItem(hwnd, id), hasProfile);
        if (!hasProfile)
            for (int id : {IDC_METERED_RATE, IDC_UNMETERED_RATE, IDC_UPGRADE_FAVORITES})
                EnableWindow(GetDlgItem(hwnd, id), FALSE);
        loadingForm = false;
        updateTreeOption(hwnd);
    }
};

INT_PTR CALLBACK settingsFormProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (msg == WM_INITDIALOG) {
        settings = reinterpret_cast<Settings*>(l);
        SetWindowLongPtrW(hwnd, DWLP_USER, l);
        const auto skinMessage = library(ML_IPC_SKIN_WADLG_GETFUNC, 2);
        if (static_cast<UINT_PTR>(skinMessage) > 1)
            settings->skinDialogMessage = reinterpret_cast<decltype(settings->skinDialogMessage)>(skinMessage);
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        for (HWND control = GetWindow(hwnd, GW_CHILD); control; control = GetWindow(control, GW_HWNDNEXT)) {
            switch (GetDlgCtrlID(control)) {
            case IDC_SERVERS: {
                skinWindow(control, SKINNEDWND_TYPE_LISTVIEW, SWLVS_FULLROWSELECT | SWLVS_SELALWAYS | SWLVS_DOUBLEBUFFER);
                attachListScrollbars(control);
                LVCOLUMNW column{};
                ListView_InsertColumn(control, 0, &column);
                break;
            }
            case IDC_METERED_FORMAT: case IDC_UNMETERED_FORMAT:
                skinWindow(control, SKINNEDWND_TYPE_COMBOBOX);
                break;
            case IDC_METERED_RATE: case IDC_UNMETERED_RATE:
                SendMessageW(control, EM_SETLIMITTEXT, 5, 0);
                [[fallthrough]];
            case IDC_NAME: case IDC_URL: case IDC_USERNAME: case IDC_PASSWORD: case IDC_BUDGET:
                // Use the skin font while EDIT paints text, selection, password masks and the caret together.
                skinWindow(control, SKINNEDWND_TYPE_WINDOW);
                break;
            case IDC_UPGRADE_FAVORITES:
            case IDC_UNIFIED: case IDC_NATIVE: case IDC_FAVORITES: case IDC_ADD_SERVER: case IDC_REMOVE_SERVER:
            case IDOK: case IDCANCEL:
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
    if (settings->skinDialogMessage && (msg == WM_CTLCOLOREDIT ||
        (msg == WM_CTLCOLORSTATIC && !IsWindowEnabled(reinterpret_cast<HWND>(l)))))
        return settings->skinDialogMessage(hwnd, msg, w, l);
    if (settings->saving.valid()) {
        if (msg == WM_TIMER && w == 1 &&
            settings->saving.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            KillTimer(hwnd, 1);
            try {
                settings->profiles = settings->saving.get();
                settings->selected = settings->savedSelection;
                EndDialog(GetParent(hwnd), IDOK);
            } catch (const std::exception& error) {
                EnableWindow(hwnd, TRUE);
                SetDlgItemTextW(hwnd, IDOK, L"Save");
                showError(GetParent(hwnd), error);
            }
        }
        return msg == WM_COMMAND || msg == WM_TIMER;
    }
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
            if (LOWORD(w) != IDC_PASSWORD) settings->passwordHint(hwnd);
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
        if (LOWORD(w) == IDC_REMOVE_SERVER && !settings->profiles.empty()) {
            auto& profiles = settings->profiles;
            const auto index = settings->selected;
            if (profiles[index].id) settings->removed.push_back(profiles[index].id);
            profiles.erase(profiles.begin() + index);
            settings->select(hwnd, profiles.empty() ? 0 : std::min(index, profiles.size() - 1));
            SetFocus(GetDlgItem(hwnd, profiles.empty() ? IDC_ADD_SERVER : IDC_SERVERS));
            return TRUE;
        }
        if (LOWORD(w) != IDOK) return FALSE;
        settings->read(hwnd);
        std::vector<Server> updated;
        auto removed = settings->removed;
        size_t selection = 0;
        std::set<std::pair<std::string, std::string>> accounts;
        for (size_t i = 0; i < settings->profiles.size(); ++i) {
            auto s = settings->profiles[i];
            if (!s.id && s.name.empty() && s.url.empty() && s.user.empty() && s.password.empty()) continue;
            try {
                s.url = normalizeUrl(s.url);
                if (s.id && !settings->sameAccount(s.id, s.url, s.user)) {
                    if (s.password.empty()) throw Error("Enter the password for the new server address or username.");
                    removed.push_back(s.id);
                    s.id = 0;
                }
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
        if (!SetTimer(hwnd, 1, 30, nullptr)) throw Error("Could not start saving server settings.");
        settings->saving = std::async(std::launch::async,
            [store = &app->store, profiles = std::move(updated), removed = std::move(removed),
             budget, unified = settings->unified]() mutable {
                store->saveProfiles(profiles, static_cast<int>(budget), unified, removed);
                return profiles;
            });
        settings->savedSelection = selection;
        SetDlgItemTextW(hwnd, IDOK, L"Saving...");
        EnableWindow(hwnd, FALSE);
    } catch (const std::exception& error) {
        KillTimer(hwnd, 1);
        showError(GetParent(hwnd), error);
    }
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
    if (msg == WM_CLOSE || (msg == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL))) {
        auto form = reinterpret_cast<HWND>(GetWindowLongPtrW(hwnd, DWLP_USER));
        if (form) return SendMessageW(form, WM_COMMAND, msg == WM_CLOSE ? IDCANCEL : w, l);
    }
    return FALSE;
}

void configure(HWND hwnd, Id preferred, bool add = false) {
    app->refreshInputs();
    Settings settings{app->store.servers(), 0, app->store.rollingMiB(), app->store.unifiedTree(), app->inputs()};
    for (const auto& s : settings.profiles) settings.savedAccounts.emplace(s.id, s);
    if (add) {
        settings.selected = settings.profiles.size();
        settings.profiles.emplace_back();
    } else {
        for (size_t i = 0; i < settings.profiles.size(); ++i)
            if (settings.profiles[i].id == preferred) settings.selected = i;
    }
    if (DialogBoxParamW(plugin.hDllInstance, MAKEINTRESOURCEW(IDD_SERVER), hwnd, settingsProc,
                        reinterpret_cast<LPARAM>(&settings)) == IDOK) {
        app->selectedServer = settings.profiles.empty() ? 0 : settings.profiles[settings.selected].id;
        if (app->filterServer && std::none_of(settings.profiles.begin(), settings.profiles.end(),
                [](const Server& s) { return s.id == app->filterServer; }))
            app->filterServer = app->selectedServer;
        app->wakeMaintenance(true);
        PostMessageW(plugin.hwndWinampParent, app->navigationChanged, 0, 0);
    }
}

constexpr wchar_t listSkinProc[] = TS_PRODUCT_NAME_W L".ListSkinProc";

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

INT_PTR CALLBACK welcomeProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

class View {
public:
    View(HWND window, const Navigation& node)
        : hwnd(window), list(GetDlgItem(hwnd, IDC_TRACKS)), category(node.category), location(categoryLocation(category)), treeServer(node.server) {
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        for (int id : {IDC_TRACKS, IDC_DETAIL_TILES}) {
            auto control = GetDlgItem(hwnd, id);
            ListView_SetExtendedListViewStyle(control, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP | LVS_EX_HEADERINALLVIEWS);
            ListView_SetUnicodeFormat(control, TRUE);
            if (!skinWindow(control, SKINNEDWND_TYPE_LISTVIEW, SWLVS_DOUBLEBUFFER | SWLVS_SELALWAYS))
                legacySkins.push_back(library(ML_IPC_SKIN_LISTVIEW, reinterpret_cast<WPARAM>(control)));
            // WACUP installs its window procedure above the common-controls subclass chain.
            auto proc = GetWindowLongPtrW(control, GWLP_WNDPROC);
            if (SetPropW(control, listSkinProc, reinterpret_cast<HANDLE>(proc)) &&
                !SetWindowLongPtrW(control, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(libraryListProc)))
                RemovePropW(control, listSkinProc);
            attachListScrollbars(control);
        }
        for (int id : {IDC_RESULTS, IDC_FILTER_FIRST, IDC_FILTER_SECOND, IDC_FILTER_THIRD}) {
            auto control = GetDlgItem(hwnd, id);
            ListView_SetUnicodeFormat(control, TRUE);
            ListView_SetExtendedListViewStyle(control, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
            skinWindow(control, SKINNEDWND_TYPE_LISTVIEW, SWLVS_FULLROWSELECT | SWLVS_SELALWAYS | SWLVS_DOUBLEBUFFER);
            attachListScrollbars(control);
        }
        for (int id : {IDC_DIVIDER_H, IDC_DIVIDER_V}) {
            auto divider = GetDlgItem(hwnd, id);
            skinWindow(divider, SKINNEDWND_TYPE_DIVIDER, id == IDC_DIVIDER_V ? SWDIV_VERT : 0);
            MLSkinnedDivider_SetCallback(divider, dividerMoved, reinterpret_cast<LPARAM>(this));
        }
        skinWindow(GetDlgItem(hwnd, IDC_QUERY), SKINNEDWND_TYPE_EDIT, SWES_VCENTER);
        for (int id : {IDC_SEARCH_LABEL, IDC_STATUS})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_STATIC);
        for (int id : {IDC_SORT_MENU, IDC_VIEW_OPTIONS, IDC_SOURCE_MENU, IDC_BACK, IDC_OPEN, IDC_PLAY, IDC_ENQUEUE, IDC_CACHE_MENU})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_BUTTON);
        SetDlgItemTextW(hwnd, IDC_VIEW_OPTIONS, L"\u2261");
        tip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            0, 0, 0, 0, hwnd, nullptr, plugin.hDllInstance, nullptr);
        if (tip) {
            TOOLINFOW tool{};
            tool.cbSize = sizeof(tool);
            tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            tool.hwnd = hwnd;
            tool.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(hwnd, IDC_VIEW_OPTIONS));
            tool.lpszText = const_cast<wchar_t*>(L"View options (F2 to edit)");
            SendMessageW(tip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        }
        preferredCoverSize = app->store.coverSize();
        tileSize = preferredCoverSize ? preferredCoverSize : automaticCoverSize();
        SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETLIMITTEXT, 256, 0);
    }
    ~View() {
        KillTimer(hwnd, viewTimer);
        KillTimer(hwnd, searchTimer);
        if (app) app->artwork.cancel(artGeneration);
        if (tip) DestroyWindow(tip);
        for (auto skin : legacySkins) if (skin) library(ML_IPC_UNSKIN_LISTVIEW, skin);
        if (images) {
            ListView_SetImageList(list, nullptr, LVSIL_SMALL);
            ListView_SetImageList(list, nullptr, LVSIL_NORMAL);
            ImageList_Destroy(images);
        }
    }
    void setCategory(BrowseKind kind) { category = kind; }
    void populateServers();
    void receive(SearchResult result);
    void receiveDetails(ViewDetails result);
    void filterSelectionChanged(int pane);
    void updatePreview();
    void saveOptions() { app->store.saveViewSettings(location.kind, writeViewOptions(options)); }
    void displayOther(NMLVDISPINFOW& display);
    void filterEvent(int pane) {
        if (!fillingFilters && filtered()) PostMessageW(hwnd, filtersChanged, pane, 0);
    }
    void focusResults(bool preview) { previewFocus = preview; queueSelectionUpdate(); }
    void playPreview() { previewFocus = true; play(false); }
    void receiveArtwork();
    void scheduleArtwork() {
        if (!artworkUpdatePosted)
            artworkUpdatePosted = PostMessageW(hwnd, artworkViewportChanged, 0, 0) != FALSE;
    }
    void updateArtworkViewport();
    void fillTiles();
    void fetchMore();
    void display(NMLVDISPINFOW& display);
    void activate(int index);
    void command(int id, int notification);
    void sortColumn(int column);
    void refresh();
    void updateButtons();
    void queueSelectionUpdate() {
        if (!selectionPending) selectionPending = PostMessageW(hwnd, selectionChanged, 0, 0) != FALSE;
    }
    void updateSelection();
    void prefetchTrack(int row) {
        if (showTracks() && row >= 0 && row < rowCount())
            app->artwork.requestTrack(trackAt(row).key);
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
    struct TileRange {
        int first = 0, end = 0;
        int count() const { return end - first; }
        bool contains(int row) const { return row >= first && row < end; }
    };

    BrowseKind category;
    BrowseLocation location;
    std::vector<BrowseLocation> draggedAlbums;
    std::vector<Track> draggedTracks;
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
    std::vector<LRESULT> legacySkins;
    HWND welcomePane = nullptr;
    HWND tip = nullptr;
    const Id treeServer;
    Id currentServer = 0;
    int tileSize = 160;
    int preferredCoverSize = 0;
    int automaticCoverSize() const {
        return std::clamp(MulDiv(160, windowDpi(hwnd), 96), 96, 384);
    }
    ViewOptions options;
    std::shared_ptr<const BrowsePage> projectionSource;
    ViewProjection projection;
    FilterSelection filterSelection;
    std::vector<Track> previewTracks;
    std::vector<int> localPreviewRows, previewSelection;
    std::map<Id, std::string> cacheLabels;
    bool detailsPending = false;
    bool previewFocus = false, fillingFilters = false, projecting = false;
    int splitStart = 0, splitLength = 0;
    static void CALLBACK dividerMoved(HWND divider, INT pos, LPARAM param);
    HIMAGELIST images = nullptr;
    bool detailRows = false;
    int tileColumnCount = 0;
    bool addingTiles = false;
    std::map<int, UINT> restoreTileState;
    ArtworkLoader::Generation artGeneration = 0;
    std::vector<ImageSlot> slots;
    std::map<int, int> slotForRow;
    std::set<int> missingArt;
    std::uint64_t artRead = 0;
    bool artworkUpdatePosted = false;
    TileRange visibleArt, nearbyArt;
    TileRange visibleTileRows() const;
    int imageBudget() const {
        return std::min(192, static_cast<int>(24 * MiB / (tileSize * tileSize * 4)));
    }

    BrowseKind sourceKind() const {
        return location.kind == BrowseKind::albums && (options.mode != ViewMode::covers || !options.musicPreset.empty()) ?
            BrowseKind::tracks : location.kind;
    }
    bool groupedAlbums() const { return trackView(sourceKind()) && options.mode == ViewMode::covers; }
    bool filtered() const { return options.mode == ViewMode::filtered; }
    bool projectedList() const { return filtered() || (options.mode == ViewMode::list && trackView(sourceKind())); }
    bool showTracks() const { return trackView(sourceKind()) && !groupedAlbums(); }
    bool albumActions() const {
        return groupedAlbums() || location.kind == BrowseKind::albums || location.kind == BrowseKind::genreAlbums ||
               location.kind == BrowseKind::artistAlbums || location.kind == BrowseKind::favorites;
    }
    bool showTiles() const {
        return options.mode == ViewMode::covers ? options.albumArt : (filtered() && options.resultArt);
    }
    bool supportsPreview() const {
        return trackView(location.kind) || albumActions() || location.kind == BrowseKind::playlists;
    }
    bool hasPreview() const { return options.mode == ViewMode::covers && supportsPreview() && options.albumResults; }
    int rowCount() const {
        if (groupedAlbums()) return static_cast<int>(projection.albums.size());
        if (projectedList()) return static_cast<int>(projection.matches.size());
        return static_cast<int>(showTracks() ? tracks.size() : items.size());
    }
    const Track& trackAt(int row) const {
        return projectedList() ? projectionSource->tracks[projection.matches[row]] : tracks[row];
    }
    const LibraryItem& itemAt(int row) const {
        if (groupedAlbums()) return projection.albums[row].item;
        return filtered() ? projectionSource->items[projection.matches[row]] : items[row];
    }
    int previewCount() const { return static_cast<int>(groupedAlbums() ? localPreviewRows.size() : previewTracks.size()); }
    const Track& previewAt(int row) const {
        return groupedAlbums() ? projectionSource->tracks[localPreviewRows[row]] : previewTracks[row];
    }
    const BrowseLocation* albumAt(int index) const {
        if (showTracks() || index < 0 || index >= rowCount()) return nullptr;
        const auto& target = itemAt(index).target;
        return target.kind == BrowseKind::albumTracks ? &target : nullptr;
    }
    void startSearch();
    void resetContent();
    void resetLocation();
    void changePresentation();
    void resizeColumns();
    void setColumns();
    void project(bool reorder = false);
    void configureView(int id);
    void setCoverSize(int size);
    void clearPreview();
    void previewCountChanged();
    void popup(int button);
    void updateRowCount();
    bool detailTiles() const { return GetDlgCtrlID(list) == IDC_DETAIL_TILES; }
    void clearList() {
        restoreTileState.clear();
        if (detailTiles()) ListView_DeleteAllItems(list);
        else ListView_SetItemCount(list, 0);
    }
    void scheduleMore();
    void resetImages();
    int imageFor(int row);
    void back();
    void play(bool enqueue);
    void cache(bool pin);
    void updateStatus();
    std::string trackValue(const Track& track, std::string_view field) const {
        if (field == "cache") {
            auto it = cacheLabels.find(track.key);
            if (it != cacheLabels.end()) return it->second;
        }
        return browseField(track, field);
    }
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
    const bool wasWelcome = servers.empty();
    const bool hadFocus = IsChild(hwnd, GetFocus());
    servers = app->store.servers();
    if (servers.empty()) {
        KillTimer(hwnd, viewTimer);
        for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
            if (child != welcomePane) ShowWindow(child, SW_HIDE);
        if (!welcomePane) {
            welcomePane = CreateDialogW(plugin.hDllInstance, MAKEINTRESOURCEW(IDD_LIBRARY_WELCOME), hwnd, welcomeProc);
            if (!welcomePane) throw Error("Could not open the library welcome screen.");
        }
    } else {
        if (welcomePane) {
            DestroyWindow(welcomePane);
            welcomePane = nullptr;
        }
        SetTimer(hwnd, viewTimer, 2000, nullptr);
    }
    currentServer = treeServer ? treeServer : app->selectedServer;
    if (!selected()) currentServer = servers.empty() ? 0 : servers.front().id;
    resetLocation();
    if (hadFocus && wasWelcome != servers.empty())
        SetFocus(welcomePane ? GetDlgItem(welcomePane, IDC_ADD_SERVER) : list);
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
    clearPreview();
    projecting = false;
    detailsPending = false;
    cacheLabels.clear();
    fillingFilters = true;
    for (int i = 0; i < 3; ++i) ListView_SetItemCount(GetDlgItem(hwnd, IDC_FILTER_FIRST + i), 0);
    fillingFilters = false;
    loading = more = morePosted = false;
    sources.clear();
    nextSource = 0;
    browseSeconds = 0;
    selectionSummary.clear();
    message.clear();
    clearList();
    projection = {};
    projectionSource.reset();
    filterSelection = {};
    items.clear();
    tracks.clear();
    knownItems.clear();
    knownTracks.clear();
    resetImages();
    if (servers.empty()) {
        RECT bounds{};
        GetClientRect(hwnd, &bounds);
        layout(bounds.right, bounds.bottom);
        return;
    }
    options = readViewOptions(location.kind, app->store.viewSettings(location.kind), app->store.textCatalog());
    order = defaultSort(sourceKind());
    descending = order.descending;
    auto saved = sourceKind() == location.kind ? app->store.browseOrder(location.kind) : options.trackSort;
    for (const auto& choice : sortChoices(sourceKind())) {
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
    auto request = location;
    request.kind = sourceKind();
    app->search(hwnd, source.server, request, query, source.offset, source.cached);
}

void View::scheduleMore() {
    if (!more || loading || morePosted) return;
    morePosted = PostMessageW(hwnd, moreRequested, 0, 0) != 0;
}

void View::updateRowCount() {
    const int previousWidth = listViewportWidth(list);
    if (detailTiles()) fillTiles();
    else ListView_SetItemCountEx(list, rowCount(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    if (listViewportWidth(list) != previousWidth) resizeColumns();
    scheduleArtwork();
    InvalidateRect(list, nullptr, FALSE);
    updateSelection();
}

void View::fillTiles() {
    if (!detailTiles() || addingTiles) return;
    const int goal = rowCount();
    int row = ListView_GetItemCount(list);
    if (row == goal) return;
    addingTiles = true;
    if (row > goal) { ListView_DeleteAllItems(list); row = 0; }
    // Only real list items can carry the subitem array that native tile view draws.
    // The list retains column numbers. Text, covers and metadata stay in our catalog.
    ListView_SetItemCount(list, goal);
    const auto& keys = showTracks() ? options.trackColumns : options.itemColumns;
    std::vector<UINT> subitems;
    for (UINT col = 1; col < keys.size() && col <= 20; ++col) subitems.push_back(col);
    LVITEMW item{};
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_COLUMNS;
    item.pszText = LPSTR_TEXTCALLBACKW;
    item.iImage = I_IMAGECALLBACK;
    item.cColumns = static_cast<UINT>(subitems.size());
    item.puColumns = subitems.data();
    const auto deadline = Clock::now() + std::chrono::milliseconds(5);
    bool failed = false;
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    for (; row < goal; ++row) {
        item.iItem = row;
        if (ListView_InsertItem(list, &item) < 0) { failed = true; break; }
        for (const auto col : subitems) ListView_SetItemText(list, row, col, LPSTR_TEXTCALLBACKW);
        auto saved = restoreTileState.find(row);
        if (saved != restoreTileState.end()) {
            ListView_SetItemState(list, row, saved->second, LVIS_FOCUSED | LVIS_SELECTED);
            if (saved->second & LVIS_FOCUSED) ListView_EnsureVisible(list, row, FALSE);
            restoreTileState.erase(saved);
        }
        if ((row % 16) == 15 && Clock::now() >= deadline) { ++row; break; }
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    addingTiles = false;
    InvalidateRect(list, nullptr, FALSE);
    scheduleArtwork();
    if (row < goal && !failed) PostMessageW(hwnd, populateTiles, 0, 0);
}

void View::receive(SearchResult result) {
    if (result.restart) { startSearch(); return; }
    browseSeconds += result.seconds;
    loading = false;
    auto& source = sources.at(nextSource);
    source.cached = result.cached;
    source.error = std::move(result.error);
    const auto oldCount = tracks.size() + items.size();
    const bool emptyPage = result.page.items.empty() && result.page.tracks.empty();
    for (auto& item : result.page.items) {
        item.target.server = source.server;
        if (knownItems.emplace(source.server, item.target.kind, item.target.id).second)
            items.push_back(std::move(item));
    }
    const bool pagedTracks = sourceKind() == BrowseKind::tracks || sourceKind() == BrowseKind::offline;
    for (auto& track : result.page.tracks) {
        if (!pagedTracks || knownTracks.insert(track.key).second) tracks.push_back(std::move(track));
    }
    // A repeated server page must not get stuck in an endless scroll loop.
    source.more = source.error.empty() && result.page.more && source.offset <= INT_MAX - 2 * pageSize &&
                  (emptyPage || items.size() + tracks.size() != oldCount);
    source.offset += pageSize;
    more = false;
    for (size_t step = 1; step <= sources.size(); ++step) {
        const auto i = (nextSource + step) % sources.size();
        if (sources[i].more) { nextSource = i; more = true; break; }
    }
    // A single live album list already comes from the server in alphabetical order.
    // Other global orders need every source page before exposing their row indices.
    if (projectedList() || groupedAlbums()) {
        if (more) { updateStatus(); scheduleMore(); return; }
        auto sourceRows = std::make_shared<BrowsePage>();
        sourceRows->tracks = std::move(tracks);
        sourceRows->items = std::move(items);
        projectionSource = std::move(sourceRows);
        project();
        return;
    }
    const bool streamOrder = std::string_view(order.key) == "sequence" ||
        (sources.size() == 1 && !source.cached && sourceKind() == BrowseKind::albums &&
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
    if (sourceKind() == location.kind) app->store.saveBrowseOrder(location.kind, savedOrder());
    else { options.trackSort = savedOrder(); saveOptions(); }
    if (projectionSource) { changePresentation(); project(true); }
    else if (loading || more || std::string_view(order.key) == "sequence") startSearch();
    else applyOrder();
}

void View::sortColumn(int column) {
    const auto& keys = showTracks() ? options.trackColumns : options.itemColumns;
    if (column < 0 || static_cast<size_t>(column) >= keys.size()) return;
    const std::string_view key = showTiles() && !options.coverDetails ? std::string_view{"name"} : std::string_view{keys[column]};
    const auto& choices = sortChoices(sourceKind());
    auto it = std::find_if(choices.begin(), choices.end(), [&](const SortChoice& choice) { return choice.key == key; });
    if (it != choices.end()) chooseOrder(*it, key == order.key ? !descending : it->descending);
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
    if (!detailsPending && !loading && !more) {
        std::map<Id, std::string> visible;
        auto collect = [&](HWND control, int total, bool preview) {
            auto first = std::max(0, ListView_GetTopIndex(control));
            auto end = std::min(total, first + ListView_GetCountPerPage(control) + 1);
            if (!preview && showTiles()) { auto range = visibleTileRows(); first = range.first; end = range.end; }
            for (int i = first; i < end; ++i) {
                auto& t = preview ? previewAt(i) : trackAt(i);
                visible.emplace(t.key, trackValue(t, "cache"));
            }
        };
        if (showTracks()) collect(list, rowCount(), false);
        if (hasPreview()) collect(GetDlgItem(hwnd, IDC_RESULTS), previewCount(), true);
        if (!visible.empty()) {
            detailsPending = true;
            app->submitDetails(hwnd, [store = &app->store, visible = std::move(visible)](const auto& cancelled) {
                ViewDetails result;
                auto& changes = result.cacheChanges.emplace();
                for (auto& [id, old] : visible) {
                    if (cancelled()) break;
                    auto value = browseField(store->track(id), "cache");
                    if (old != value) changes.emplace(id, std::move(value));
                }
                return result;
            });
        }
    }
    updateStatus();
}

void View::display(NMLVDISPINFOW& display) {
    auto& item = display.item;
    const auto row = item.iItem;
    if (row < 0 || row >= rowCount()) return;
    if (item.mask & LVIF_TEXT) {
        std::wstring text;
        const auto& columns = showTracks() ? options.trackColumns : options.itemColumns;
        if (showTiles() && !options.coverDetails)
            text = showTracks() ? wide(browseField(trackAt(row), "name")) : wide(itemAt(row).target.title);
        else if (static_cast<size_t>(item.iSubItem) < columns.size()) {
            const auto& key = columns[item.iSubItem];
            text = formatCell(key, showTracks() ? trackValue(trackAt(row), key) : browseField(itemAt(row), key));
        }
        if (item.pszText && item.cchTextMax > 0) lstrcpynW(item.pszText, text.c_str(), item.cchTextMax);
    }
    if (item.mask & LVIF_IMAGE) item.iImage = showTiles() ? imageFor(row) : I_IMAGENONE;
    if ((!showTiles() || !images) && row >= rowCount() - 12) scheduleMore();
}

void View::displayOther(NMLVDISPINFOW& info) {
    auto& item = info.item;
    if (!(item.mask & LVIF_TEXT) || !item.pszText || item.cchTextMax <= 0) return;
    std::wstring text;
    if (info.hdr.idFrom == IDC_RESULTS) {
        if (item.iItem >= 0 && item.iItem < previewCount() && static_cast<size_t>(item.iSubItem) < options.trackColumns.size()) {
            auto& key = options.trackColumns[item.iSubItem];
            text = formatCell(key, trackValue(previewAt(item.iItem), key));
        }
    } else {
        size_t pane = info.hdr.idFrom - IDC_FILTER_FIRST;
        if (pane < options.filters.size() && static_cast<size_t>(item.iItem) < projection.groups[pane].size()) {
            const auto& keys = options.filterColumns.at(options.filters[pane]);
            if (static_cast<size_t>(item.iSubItem) < keys.size()) {
                const auto& row = projection.groups[pane][item.iItem];
                text = summaryCell(row, keys[item.iSubItem]);
                if (keys[item.iSubItem] == "value" && text.empty()) text = L"(unknown)";
            }
        }
    }
    lstrcpynW(item.pszText, text.c_str(), item.cchTextMax);
}

void View::project(bool reorder) {
    if (!projectionSource) return;
    projecting = detailsPending = true;
    app->submitDetails(hwnd, [source = projectionSource, songs = trackView(sourceKind()), settings = options,
        selection = filterSelection, sort = order, reverse = descending,
        indices = reorder ? nullptr : projection.order, cache = cacheLabels](const auto& cancelled) {
        ViewDetails result;
        result.projection = projectRows(*source, songs, settings, selection, sort, reverse, indices, cache, cancelled);
        return result;
    });
    updateStatus();
}

void View::filterSelectionChanged(int pane) {
    if (fillingFilters || !filtered() || static_cast<size_t>(pane) >= options.filters.size()) return;
    auto control = GetDlgItem(hwnd, IDC_FILTER_FIRST + pane);
    std::set<std::wstring> selected;
    const auto& groups = projection.groups[pane];
    const bool all = ListView_GetNextItem(control, -1, LVNI_FOCUSED) == 0 && ListView_GetItemState(control, 0, LVIS_SELECTED);
    if (!all) {
        for (int i = ListView_GetNextItem(control, 0, LVNI_SELECTED); i > 0 && static_cast<size_t>(i) < groups.size();
             i = ListView_GetNextItem(control, i, LVNI_SELECTED)) selected.insert(groups[i].value);
    }
    if (selected == filterSelection[pane]) return;
    filterSelection[pane] = std::move(selected);
    for (size_t i = pane + 1; i < filterSelection.size(); ++i) filterSelection[i].clear();
    project();
}

void View::receiveDetails(ViewDetails result) {
    projecting = detailsPending = false;
    if (!result.error.empty()) { message = std::move(result.error); updateStatus(); return; }
    if (result.cacheChanges) {
        if (result.cacheChanges->empty()) return;
        for (auto& [id, value] : *result.cacheChanges) cacheLabels[id] = std::move(value);
        InvalidateRect(list, nullptr, FALSE);
        if (hasPreview()) InvalidateRect(GetDlgItem(hwnd, IDC_RESULTS), nullptr, FALSE);
        if (filtered() && std::find(options.filters.begin(), options.filters.end(), "cache") != options.filters.end()) project();
        return;
    }
    if (result.albumPreview) {
        previewTracks = std::move(result.preview);
        previewCountChanged();
        updateSelection();
        return;
    }
    if (!projectionSource) return;
    // Track rows keep their source index across sorts and filter changes. An album's
    // first track identifies its old selection even if another track sorts first now.
    std::map<int, UINT> selection;
    auto remember = [&](int row, UINT state) {
        if (row < 0 || row >= rowCount()) return;
        int source = groupedAlbums() ? projection.albums[row].rows.front() : projection.matches[row];
        selection[source] |= state;
    };
    remember(ListView_GetNextItem(list, -1, LVNI_FOCUSED), LVIS_FOCUSED);
    for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(list, i, LVNI_SELECTED))
        remember(i, LVIS_SELECTED);
    clearList();
    clearPreview();
    resetImages();
    fillingFilters = true;
    projection = std::move(result.projection);
    if (filtered()) {
        for (size_t pane = 0; pane < options.filters.size(); ++pane) {
            auto control = GetDlgItem(hwnd, IDC_FILTER_FIRST + static_cast<int>(pane));
            const auto& groups = projection.groups[pane];
            const int first = ListView_GetTopIndex(control);
            ListView_SetItemCountEx(control, static_cast<int>(groups.size()), LVSICF_NOSCROLL);
            ListView_SetItemState(control, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            const auto& selected = filterSelection[pane];
            if (selected.empty()) {
                ListView_SetItemState(control, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            } else {
                bool focus = true;
                for (size_t i = 1; i < groups.size(); ++i) if (selected.count(groups[i].value)) {
                    ListView_SetItemState(control, i, LVIS_SELECTED | (focus ? LVIS_FOCUSED : 0), LVIS_SELECTED | LVIS_FOCUSED);
                    focus = false;
                }
            }
            if (first > 0 && static_cast<size_t>(first) < groups.size()) ListView_EnsureVisible(control, first, FALSE);
            InvalidateRect(control, nullptr, FALSE);
        }
    }
    fillingFilters = false;
    changePresentation();
    updateRowCount();
    for (int i = 0; !selection.empty() && i < rowCount(); ++i) {
        UINT state = 0;
        auto restore = [&](int source) {
            auto it = selection.find(source);
            if (it != selection.end()) { state |= it->second; selection.erase(it); }
        };
        if (groupedAlbums()) for (int row : projection.albums[i].rows) restore(row);
        else restore(projection.matches[i]);
        if (!state) continue;
        if (detailTiles() && i >= ListView_GetItemCount(list)) restoreTileState[i] = state;
        else {
            ListView_SetItemState(list, i, state, LVIS_SELECTED | LVIS_FOCUSED);
            if (state & LVIS_FOCUSED) ListView_EnsureVisible(list, i, FALSE);
        }
    }
    queueSelectionUpdate();
}

void View::clearPreview() {
    previewSelection.clear();
    ListView_SetItemCount(GetDlgItem(hwnd, IDC_RESULTS), 0);
    previewTracks.clear();
    localPreviewRows.clear();
    previewFocus = false;
}
void View::previewCountChanged() {
    auto control = GetDlgItem(hwnd, IDC_RESULTS);
    ListView_SetItemCountEx(control, previewCount(), LVSICF_NOSCROLL);
    InvalidateRect(control, nullptr, FALSE);
    resizeColumns();
}
void View::updatePreview() {
    if (!hasPreview()) return;
    std::vector<int> selected;
    for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0 && i < rowCount();
         i = ListView_GetNextItem(list, i, LVNI_SELECTED)) selected.push_back(i);
    if (selected == previewSelection) return;
    clearPreview();
    previewSelection = selected;
    if (groupedAlbums()) {
        for (int i : selected) {
            const auto& rows = projection.albums[i].rows;
            localPreviewRows.insert(localPreviewRows.end(), rows.begin(), rows.end());
        }
        previewCountChanged();
        return;
    }
    std::vector<BrowseLocation> targets;
    for (int i : selected) {
        auto& target = itemAt(i).target;
        if (target.kind == BrowseKind::albumTracks || target.kind == BrowseKind::song || target.kind == BrowseKind::playlistTracks)
            targets.push_back(target);
    }
    detailsPending = true;
    app->submitDetails(hwnd, [targets = std::move(targets), owner = app.get()](const auto& cancelled) {
        ViewDetails result;
        result.albumPreview = true;
        for (auto& target : targets) {
            if (cancelled()) break;
            auto rows = owner->albumTracks(target.server, target, false);
            std::move(rows.begin(), rows.end(), std::back_inserter(result.preview));
        }
        return result;
    });
}

void View::resetImages() {
    artGeneration = app->artwork.reset(hwnd);
    visibleArt = nearbyArt = {};
    missingArt.clear();
    slots.clear();
    slotForRow.clear();
    artRead = 0;
    if (images) {
        ListView_SetImageList(list, nullptr, LVSIL_SMALL);
        ListView_SetImageList(list, nullptr, LVSIL_NORMAL);
        ImageList_Destroy(images);
        images = nullptr;
    }
}

int View::imageFor(int row) {
    if (!images) return I_IMAGENONE;
    scheduleArtwork();
    auto existing = slotForRow.find(row);
    if (existing != slotForRow.end()) {
        slots[existing->second - 1].read = ++artRead;
        return existing->second;
    }
    return 0;
}

View::TileRange View::visibleTileRows() const {
    if (!images || !showTiles() || !IsWindowVisible(list)) return {};
    const int count = std::min(rowCount(), ListView_GetItemCount(list));
    RECT client{};
    GetClientRect(list, &client);
    if (HWND header = ListView_GetHeader(list); header && IsWindowVisible(header)) {
        RECT bounds{};
        GetWindowRect(header, &bounds);
        MapWindowPoints(nullptr, list, reinterpret_cast<POINT*>(&bounds), 2);
        client.top = std::max(client.top, bounds.bottom);
    }
    if (!count || IsRectEmpty(&client)) return {};

    // GetTopIndex only supports report/list views. Auto-arranged icons have
    // the same bounds along the vertical axis within each grid row.
    auto firstMatching = [&](auto matches) {
        int low = 0, high = count;
        while (low < high) {
            const int mid = low + (high - low) / 2;
            RECT icon{};
            if (!ListView_GetItemRect(list, mid, &icon, LVIR_ICON)) return count;
            if (matches(icon)) high = mid;
            else low = mid + 1;
        }
        return low;
    };
    const int first = firstMatching([&](const RECT& icon) { return icon.bottom > client.top; });
    const int end = firstMatching([&](const RECT& icon) { return icon.top >= client.bottom; });
    return {first, std::max(first, end)};
}

void View::updateArtworkViewport() {
    const auto visible = visibleTileRows();
    artworkUpdatePosted = false;
    const int margin = std::min(visible.count(), std::max(1, (imageBudget() - visible.count()) / 2));
    const TileRange nearby{std::max(0, visible.first - margin),
                           visible.end + std::min(margin, rowCount() - visible.end)};
    if (visible.count() && nearby.end == rowCount()) scheduleMore();
    if (visible.first == visibleArt.first && visible.end == visibleArt.end &&
        nearby.first == nearbyArt.first && nearby.end == nearbyArt.end) return;
    visibleArt = visible;
    nearbyArt = nearby;
    std::vector<int> rows;
    rows.reserve(nearby.count());
    auto add = [&](int row) {
        if (!slotForRow.count(row) && !missingArt.count(row)) rows.push_back(row);
    };
    for (int row = visible.first; row < visible.end; ++row) add(row);
    const auto visibleCount = rows.size();
    for (int i = 0; i < margin; ++i) {
        if (i < visible.first - nearby.first) add(visible.first - 1 - i);
        if (i < nearby.end - visible.end) add(visible.end + i);
    }
    app->artwork.setViewport(artGeneration, tileSize, [this](int row) -> LibraryItem {
        if (showTracks()) {
            const auto& t = trackAt(row);
            return {{BrowseKind::song, t.id, t.title, t.server}, {}, t.coverId};
        }
        const auto& item = itemAt(row);
        return {item.target, {}, item.coverId};
    }, rows, visibleCount);
}

void View::receiveArtwork() {
    const auto maxImages = static_cast<std::size_t>(std::max(imageBudget(), nearbyArt.count()));
    for (const auto& thumbnail : app->artwork.take(artGeneration)) {
        if (!images || !nearbyArt.contains(thumbnail.row)) continue;
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
                [this](const ImageSlot& a, const ImageSlot& b) {
                    return std::make_pair(nearbyArt.contains(a.row), a.read) <
                           std::make_pair(nearbyArt.contains(b.row), b.read);
                });
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
            if (visibleArt.contains(thumbnail.row)) ListView_RedrawItems(list, thumbnail.row, thumbnail.row);
        } else {
            missingArt.insert(thumbnail.row);
        }
    }
}

void View::changePresentation() {
    const bool wantDetail = showTiles() && options.coverDetails;
    const bool switching = wantDetail != detailTiles();
    std::map<int, UINT> oldState;
    const bool wasFocused = GetFocus() == list;
    if (switching) {
        for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(list, i, LVNI_SELECTED))
            oldState[i] = LVIS_SELECTED;
        int focus = ListView_GetNextItem(list, -1, LVNI_FOCUSED);
        if (focus >= 0) oldState[focus] |= LVIS_FOCUSED;
        ListView_SetImageList(list, nullptr, LVSIL_NORMAL);
        ListView_SetImageList(list, nullptr, LVSIL_SMALL);
        clearList();
        ShowWindow(list, SW_HIDE);
        list = GetDlgItem(hwnd, wantDetail ? IDC_DETAIL_TILES : IDC_TRACKS);
        if (images) ListView_SetImageList(list, images, LVSIL_NORMAL);
        ShowWindow(list, SW_SHOWNA);
    }
    SetDlgItemTextW(hwnd, IDC_SORT_MENU, order.label);
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
    detailRows = false;
    if (tiles && options.coverDetails) {
        // Hosts without tile view can still show the cover beside the metadata in report view.
        detailRows = ListView_SetView(list, LV_VIEW_TILE) != 1;
        if (detailRows) ListView_SetView(list, LV_VIEW_DETAILS);
    } else ListView_SetView(list, tiles ? LV_VIEW_ICON : LV_VIEW_DETAILS);
    ListView_SetImageList(list, detailRows ? images : nullptr, LVSIL_SMALL);
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_FULLROWSELECT | LVS_EX_HIDELABELS,
        (!tiles || detailRows ? LVS_EX_FULLROWSELECT : 0) |
        (tiles && !options.coverDetails && !options.coverTitles ? LVS_EX_HIDELABELS : 0));
    const auto& keys = showTracks() ? options.trackColumns : options.itemColumns;
    if (tiles && images) {
        const auto font = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
        HDC dc = GetDC(list);
        auto previous = SelectObject(dc, font);
        TEXTMETRICW metrics{};
        GetTextMetricsW(dc, &metrics);
        SelectObject(dc, previous);
        ReleaseDC(list, dc);
        const int gap = MulDiv(4, windowDpi(hwnd), 96);
        if (options.coverDetails && !detailRows) {
            LVTILEVIEWINFO info{};
            info.cbSize = sizeof(info);
            info.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
            info.dwFlags = LVTVIF_FIXEDSIZE;
            info.sizeTile = {tileSize + MulDiv(160, windowDpi(hwnd), 96), tileSize + gap};
            info.cLines = std::min(20, static_cast<int>(keys.size()));
            ListView_SetTileViewInfo(list, &info);
        } else if (!detailRows) {
            ListView_SetIconSpacing(list, tileSize + gap, tileSize + (options.coverTitles ? 2 * metrics.tmHeight : 0) + gap);
        }
    }
    SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETCUEBANNER, FALSE,
        reinterpret_cast<LPARAM>(showTracks() ? L"Search tracks" : L"Search this view"));
    setColumns();
    if (wantDetail) {
        if (tileColumnCount != static_cast<int>(keys.size())) {
            clearList();
            tileColumnCount = static_cast<int>(keys.size());
        }
        if (switching) restoreTileState = std::move(oldState);
        fillTiles();
    } else if (switching) {
        ListView_SetItemCountEx(list, rowCount(), LVSICF_NOSCROLL);
        for (auto [i, state] : oldState) if (i < rowCount()) {
            ListView_SetItemState(list, i, state, LVIS_SELECTED | LVIS_FOCUSED);
            if (state & LVIS_FOCUSED) ListView_EnsureVisible(list, i, FALSE);
        }
    }
    if (switching && wasFocused) SetFocus(list);
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
    if (groupedAlbums()) {
        if (!hasPreview()) { options.albumResults = true; saveOptions(); changePresentation(); }
        updatePreview();
        SetFocus(GetDlgItem(hwnd, IDC_RESULTS));
        return;
    }
    auto target = itemAt(index).target;
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
    if (projectedList() || groupedAlbums() || savedOrder() != outer.order) { startSearch(); return; }
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
    if (hasPreview() && !previewFocus) updatePreview();
    const bool preview = hasPreview() && previewFocus;
    HWND active = preview ? GetDlgItem(hwnd, IDC_RESULTS) : list;
    const int total = preview ? previewCount() : rowCount();
    const bool songsView = preview || showTracks();
    const int selectedRows = ListView_GetSelectedCount(active);
    const int count = selectedRows ? selectedRows : total;
    std::int64_t seconds = 0, songs = 0;
    bool knownDuration = true, knownCount = true;
    auto include = [&](int i) {
        const auto duration = preview ? previewAt(i).duration : showTracks() ? trackAt(i).duration : itemAt(i).duration;
        const int n = songsView ? 1 : itemAt(i).songCount;
        knownDuration &= duration > 0 || n == 0;
        knownCount &= n >= 0;
        seconds += duration;
        songs += std::max(0, n);
    };
    if (selectedRows) {
        for (int i = ListView_GetNextItem(active, -1, LVNI_SELECTED); i >= 0 && i < total;
             i = ListView_GetNextItem(active, i, LVNI_SELECTED)) include(i);
    } else {
        for (int i = 0; i < count; ++i) include(i);
    }
    auto numberOf = [](std::int64_t n, const wchar_t* one, const wchar_t* many) {
        return std::to_wstring(n) + L" " + (n == 1 ? one : many);
    };
    selectionSummary = songsView ? numberOf(count, L"track", L"tracks") : numberOf(count, L"item", L"items");
    if (selectedRows) selectionSummary += L" selected";
    if (!songsView && count && knownCount) selectionSummary += L"  |  " + numberOf(songs, L"track", L"tracks");
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
    if (!server) return;
    std::wstring status;
    if ((loading || more || projecting) && !ListView_GetItemCount(list)) {
        const auto loaded = items.size() + tracks.size();
        status = loaded ? L"Loading... " + std::to_wstring(loaded) : L"Loading...";
    }
    else if (!message.empty()) status = wide(message);
    else if (!app->playbackTrackingError.empty()) status = wide(app->playbackTrackingError);
    else {
        if (rowCount() == 0) status = showTracks() ? L"No tracks" : L"No results";
        else status = selectionSummary;
        if (loading) status += L"  |  Loading more...";
        const bool offline = app->store.offlineMode();
        std::int64_t pendingListens = 0;
        for (const auto& source : sources) {
            pendingListens += app->store.pendingListenCount(source.server);
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
        if (pendingListens) status += L"  |  " + std::to_wstring(pendingListens) +
            (pendingListens == 1 ? L" listen pending" : L" listens pending");
    }
    if (server && app->store.offlineMode()) status = L"Offline  |  " + status;
    if (!history.empty()) status += L"  |  " + wide(location.title);
    if (!options.musicPreset.empty()) {
        for (auto& preset : musicPresets(location.kind))
            if (options.musicPreset == preset.key) { status += std::wstring(L"  |  ") + preset.label; break; }
    }
    if (server && servers.size() > 1 && sources.size() == 1) status += L"  |  " + wide(server->name);
    SetDlgItemTextW(hwnd, IDC_STATUS, status.c_str());
}

void View::updateButtons() {
    const bool haveServer = selected() != nullptr;
    const bool preview = hasPreview() && previewFocus;
    const bool haveRows = ListView_GetSelectedCount(preview ? GetDlgItem(hwnd, IDC_RESULTS) : list) > 0;
    EnableWindow(GetDlgItem(hwnd, IDC_QUERY), haveServer);
    EnableWindow(GetDlgItem(hwnd, IDC_OPEN), haveRows && !preview);
    EnableWindow(GetDlgItem(hwnd, IDC_CACHE_MENU), haveRows && (preview || showTracks() || groupedAlbums()));
    bool playable = haveRows && (preview || showTracks() || albumActions());
    if (playable && !preview && !showTracks()) {
        for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
             i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
            if (!albumAt(i)) { playable = false; break; }
        }
    }
    for (int id : {IDC_PLAY, IDC_ENQUEUE}) EnableWindow(GetDlgItem(hwnd, id), playable);
}

std::vector<Track> View::selection() {
    std::vector<Track> selectedTracks;
    const bool preview = hasPreview() && previewFocus;
    HWND active = preview ? GetDlgItem(hwnd, IDC_RESULTS) : list;
    for (int i = ListView_GetNextItem(active, -1, LVNI_SELECTED); i >= 0 && i < (preview ? previewCount() : rowCount());
         i = ListView_GetNextItem(active, i, LVNI_SELECTED)) {
        if (preview) selectedTracks.push_back(previewAt(i));
        else if (groupedAlbums()) {
            for (int row : projection.albums[i].rows) selectedTracks.push_back(projectionSource->tracks[row]);
        } else if (showTracks()) selectedTracks.push_back(trackAt(i));
    }
    return selectedTracks;
}

void View::beginDrag(const NMLISTVIEW& notification) {
    const bool preview = notification.hdr.idFrom == IDC_RESULTS;
    const int row = notification.iItem;
    if (row < 0 || row >= (preview ? previewCount() : rowCount())) return;
    previewFocus = preview;
    if (preview || showTracks() || groupedAlbums()) {
        if (ListView_GetItemState(notification.hdr.hwndFrom, row, LVIS_SELECTED)) draggedTracks = selection();
        else if (preview) draggedTracks.push_back(previewAt(row));
        else if (groupedAlbums()) {
            for (int i : projection.albums[row].rows) draggedTracks.push_back(projectionSource->tracks[i]);
        } else draggedTracks.push_back(trackAt(row));
    } else if (const auto* album = albumAt(row)) {
        draggedAlbums = ListView_GetItemState(list, row, LVIS_SELECTED) ?
            selectedAlbums() : std::vector<BrowseLocation>{*album};
    }
    if (draggedAlbums.empty() && draggedTracks.empty()) return;
    SetCapture(hwnd);
    POINT point = notification.ptAction;
    MapWindowPoints(notification.hdr.hwndFrom, hwnd, &point, 1);
    drag(point, false);
}

void View::cancelDrag() {
    draggedAlbums.clear();
    draggedTracks.clear();
    if (GetCapture() == hwnd) ReleaseCapture();
}

void View::drag(POINT point, bool released) {
    if (draggedAlbums.empty() && draggedTracks.empty()) return;
    mlDropItemStruct drop{};
    drop.type = ML_TYPE_ITEMRECORDLISTW;
    drop.p = point;
    if (!ClientToScreen(hwnd, &drop.p)) { cancelDrag(); return; }
    drop.flags = released ? ML_HANDLEDRAG_FLAG_NOCURSOR : 0;
    library(ML_IPC_HANDLEDRAG, reinterpret_cast<WPARAM>(&drop));
    if (released) {
        auto albums = std::move(draggedAlbums);
        auto tracks = std::move(draggedTracks);
        cancelDrag();
        if (drop.result > 0) {
            if (!tracks.empty()) app->sendTrackRecords(tracks, AlbumAction::drop, drop);
            else if (!albums.empty()) app->sendAlbums(std::move(albums), AlbumAction::drop, drop);
        }
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
    if (!showTracks() && !groupedAlbums() && !(hasPreview() && previewFocus)) {
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
    } else if (button == IDC_SORT_MENU) {
        const auto& choices = sortChoices(sourceKind());
        for (size_t i = 0; i < choices.size(); ++i) {
            const bool active = std::string_view(choices[i].key) == order.key;
            AppendMenuW(menu, MF_STRING | (active ? MF_CHECKED : 0), sortCommand + i, choices[i].label);
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        if (std::string_view(order.key) != "sequence" && std::string_view(order.key) != "random")
            AppendMenuW(menu, MF_STRING | (descending ? MF_CHECKED : 0), directionCommand, L"Descending");
    } else if (button == IDC_VIEW_OPTIONS) {
        auto option = [&](int id, const wchar_t* label, bool checked = false) {
            AppendMenuW(menu, MF_STRING | (checked ? MF_CHECKED : 0), id, label);
        };
        option(IDC_EDIT_VIEW, L"Edit view mode...\tF2");
        if (auto modes = CreatePopupMenu()) {
            AppendMenuW(modes, MF_STRING, IDC_SIMPLE, L"Simple List View");
            AppendMenuW(modes, MF_STRING, IDC_COVERS, L"Album Cover View");
            AppendMenuW(modes, MF_STRING, IDC_FILTERED, L"Filtered View");
            CheckMenuRadioItem(modes, IDC_SIMPLE, IDC_FILTERED, filtered() ? IDC_FILTERED :
                options.mode == ViewMode::covers ? IDC_COVERS : IDC_SIMPLE, MF_BYCOMMAND);
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(modes), L"View mode");
        }
        option(IDC_SEARCH, L"Refresh view metadata\tF5");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        if (filtered()) {
            auto fields = filterFields(location.kind);
            for (size_t i = 0; i < options.filters.size(); ++i) {
                auto label = std::wstring(L"Edit \"") + findColumn(fields, options.filters[i])->label + L"\" filter columns...";
                option(IDC_EDIT_FILTER_FIRST + static_cast<int>(i), label.c_str());
            }
            option(IDC_RESULT_ART, L"Display results using album art", options.resultArt);
            option(IDC_FILTERS_RIGHT, L"Show results to the right of the filter panes", options.filtersRight);
        }
        if (options.mode == ViewMode::covers)
            option(IDC_RESULT_ART, L"Display results using album art", options.albumArt);
        if (options.mode == ViewMode::covers && supportsPreview()) {
            option(IDC_ALBUM_RESULTS, L"Show the album selection results", options.albumResults);
            if (options.albumResults)
                option(IDC_ALBUMS_RIGHT, L"Show results to the right of the album art", options.albumsRight);
        }
        if (showTiles()) {
            AppendMenuW(menu, MF_STRING | (options.coverTitles ? MF_CHECKED : 0) | (options.coverDetails ? MF_GRAYED : 0),
                        IDC_COVER_TITLES, L"Show album title below album art");
            option(IDC_COVER_DETAILS, L"Show details beside album art", options.coverDetails);
            if (options.coverDetails) option(IDC_COVER_COLUMNS, L"Edit album art columns...");
            if (HMENU sizes = CreatePopupMenu()) {
                for (size_t i = 0; i < std::size(coverSizes); ++i) {
                    const int size = coverSizes[i];
                    auto label = size ? std::to_wstring(size) + L" px" :
                        L"Automatic (" + std::to_wstring(automaticCoverSize()) + L" px)";
                    AppendMenuW(sizes, MF_STRING | (size == preferredCoverSize ? MF_CHECKED : 0),
                                coverSizeCommand + i, label.c_str());
                }
                if (std::find(std::begin(coverSizes), std::end(coverSizes), preferredCoverSize) == std::end(coverSizes)) {
                    const auto label = std::to_wstring(preferredCoverSize) + L" px";
                    AppendMenuW(sizes, MF_STRING | MF_CHECKED, IDC_EDIT_VIEW, label.c_str());
                }
                AppendMenuW(sizes, MF_STRING, IDC_EDIT_VIEW, L"Custom size...");
                AppendMenuW(menu, MF_POPUP,
                            reinterpret_cast<UINT_PTR>(sizes), L"Cover size");
            }
        }
        if (options.mode == ViewMode::covers && !showTiles() && hasPreview())
            option(IDC_COVER_COLUMNS, L"Edit album columns...");
        if (!showTiles() || hasPreview())
            option(IDC_EDIT_COLUMNS, filtered() || hasPreview() ? L"Edit results columns..." : L"Edit view columns...");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
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
    if (servers.empty()) {
        if (id == IDC_ADD_SERVER) configure(hwnd, 0, true);
        return;
    }
    if (id >= static_cast<int>(allServersCommand)) {
        const auto index = static_cast<size_t>(id - allServersCommand);
        if (index <= servers.size()) selectCategory(category, index ? servers[index - 1].id : 0);
        return;
    }
    const auto& choices = sortChoices(sourceKind());
    if (id >= coverSizeCommand && static_cast<size_t>(id - coverSizeCommand) < std::size(coverSizes)) {
        setCoverSize(coverSizes[id - coverSizeCommand]);
        return;
    }
    if (id >= sortCommand && static_cast<size_t>(id - sortCommand) < choices.size()) {
        const auto& choice = choices[id - sortCommand];
        chooseOrder(choice, choice.descending);
        return;
    }
    switch (id) {
    case directionCommand: chooseOrder(order, !descending); break;
    case IDC_SORT_MENU: case IDC_VIEW_OPTIONS: case IDC_SOURCE_MENU: case IDC_CACHE_MENU: popup(id); break;
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
    case IDC_EDIT_VIEW: case IDC_SIMPLE: case IDC_COVERS: case IDC_FILTERED:
    case IDC_EDIT_COLUMNS: case IDC_EDIT_FILTER_FIRST: case IDC_EDIT_FILTER_FIRST + 1: case IDC_EDIT_FILTER_FIRST + 2:
    case IDC_ALBUM_RESULTS: case IDC_ALBUMS_RIGHT: case IDC_FILTERS_RIGHT: case IDC_RESULT_ART:
    case IDC_COVER_TITLES: case IDC_COVER_DETAILS: case IDC_COVER_COLUMNS:
        configureView(id); break;
    case IDC_PLAY: play(false); break;
    case IDC_ENQUEUE: play(true); break;
    case IDC_KEEP_OFFLINE: cache(true); break;
    case IDC_UNPIN: cache(false); break;
    }
}

void View::setCoverSize(int size) {
    if (size == preferredCoverSize) return;
    app->store.saveCoverSize(size);
    preferredCoverSize = size;
    const int pixels = size ? size : automaticCoverSize();
    if (pixels == tileSize) return;
    const auto anchor = visibleTileRows();
    tileSize = pixels;
    resetImages();
    changePresentation();
    if (anchor.count()) ListView_EnsureVisible(list, anchor.first, FALSE);
    scheduleArtwork();
}

void View::configureView(int id) {
    ViewEditorHost host{plugin.hDllInstance, skinWindow};
    auto next = options;
    int size = preferredCoverSize;
    if (id == IDC_EDIT_VIEW) {
        if (!editLibraryView(host, hwnd, location.kind, supportsPreview(), next, size)) return;
    } else if (id == IDC_EDIT_COLUMNS || id == IDC_COVER_COLUMNS) {
        bool songs = showTracks() || (hasPreview() && id == IDC_EDIT_COLUMNS);
        auto kind = songs ? BrowseKind::tracks : groupedAlbums() ? BrowseKind::albums : location.kind;
        if (!editLibraryColumns(host, hwnd, id == IDC_COVER_COLUMNS ? L"Edit Album Art Columns" :
            hasPreview() || filtered() ? L"Edit Results Columns" : L"Edit View Columns",
            viewFields(kind), defaultColumns(kind), songs ? next.trackColumns : next.itemColumns)) return;
    } else if (id >= IDC_EDIT_FILTER_FIRST && id < IDC_EDIT_FILTER_FIRST + 3) {
        size_t pane = id - IDC_EDIT_FILTER_FIRST;
        if (pane >= next.filters.size()) return;
        auto fields = filterFields(location.kind);
        const auto& key = next.filters[pane];
        auto label = findColumn(fields, key)->label;
        if (!editLibraryColumns(host, hwnd, std::wstring(L"Edit ") + label + L" Filter Columns",
                summaryFields(label), defaultSummaryColumns(key), next.filterColumns[key])) return;
    } else switch (id) {
    case IDC_SIMPLE: next.mode = ViewMode::list; break;
    case IDC_COVERS: next.mode = ViewMode::covers; next.albumArt = true; break;
    case IDC_FILTERED: next.mode = ViewMode::filtered; break;
    case IDC_ALBUM_RESULTS: next.albumResults = !next.albumResults; break;
    case IDC_ALBUMS_RIGHT: next.albumsRight = !next.albumsRight; break;
    case IDC_FILTERS_RIGHT: next.filtersRight = !next.filtersRight; break;
    case IDC_RESULT_ART:
        if (next.mode == ViewMode::covers) next.albumArt = !next.albumArt;
        else next.resultArt = !next.resultArt;
        break;
    case IDC_COVER_TITLES: next.coverTitles = !next.coverTitles; break;
    case IDC_COVER_DETAILS: next.coverDetails = !next.coverDetails; break;
    }
    const bool newPreset = next.musicPreset != options.musicPreset;
    const bool reload = next.mode != options.mode || newPreset;
    if (next.mode != options.mode && next.mode == ViewMode::covers) next.albumArt = true;
    const auto previousSource = sourceKind();
    auto previousRows = projectionSource;
    const bool refilter = !reload && filtered() && next.filters != options.filters;
    const bool wasPreview = hasPreview();
    // Keep the old row mapping valid until the native control releases its selection.
    if (reload) clearList();
    options = std::move(next);
    if (newPreset) {
        for (auto& preset : musicPresets(location.kind)) if (preset.key == options.musicPreset && preset.sort) {
            const auto& choices = sortChoices(sourceKind());
            auto choice = std::find_if(choices.begin(), choices.end(), [&](const SortChoice& c) { return std::string_view(c.key) == preset.sort; });
            if (choice != choices.end()) {
                order = *choice;
                descending = true;
                if (sourceKind() == location.kind) app->store.saveBrowseOrder(location.kind, savedOrder());
                else options.trackSort = savedOrder();
            }
            break;
        }
    }
    saveOptions();
    if (reload && previousSource == sourceKind() && (projectedList() || groupedAlbums()) &&
        (previousRows || (!loading && !more && (!tracks.empty() || !items.empty())))) {
        if (!previousRows) {
            auto rows = std::make_shared<BrowsePage>();
            rows->tracks = std::move(tracks);
            rows->items = std::move(items);
            previousRows = std::move(rows);
        }
        clearPreview();
        filterSelection = {};
        projection = {};
        projectionSource = previousRows;
        resetImages();
        changePresentation();
        project();
    } else if (reload) startSearch();
    else {
        if (wasPreview && !hasPreview()) {
            clearPreview();
            if (!groupedAlbums()) { app->submitDetails(hwnd, {}); detailsPending = false; }
        }
        if (refilter) {
            filterSelection = {};
            fillingFilters = true;
            for (int i = 0; i < 3; ++i) ListView_SetItemCount(GetDlgItem(hwnd, IDC_FILTER_FIRST + i), 0);
            fillingFilters = false;
            project();
        }
        changePresentation();
        updateSelection();
    }
    setCoverSize(size);
}

void CALLBACK View::dividerMoved(HWND, INT, LPARAM param) {
    auto& view = *reinterpret_cast<View*>(param);
    if (view.splitLength <= 0) return;
    POINT pt{};
    if (!GetCursorPos(&pt) || !ScreenToClient(view.hwnd, &pt)) return;
    const bool right = view.filtered() ? view.options.filtersRight : view.options.albumsRight;
    const int pos = right ? pt.x : pt.y;
    view.options.splitPercent = std::clamp(MulDiv(pos - view.splitStart, 100, view.splitLength), 15, 85);
    RECT rc{};
    GetClientRect(view.hwnd, &rc);
    view.layout(rc.right, rc.bottom);
    SetTimer(view.hwnd, splitTimer, 200, nullptr);
}

void layoutWelcome(HWND hwnd, int width, int height) {
    const HWND label = GetDlgItem(hwnd, IDC_WELCOME_TEXT);
    const auto message = dialogText(hwnd, IDC_WELCOME_TEXT);
    HDC dc = GetDC(label);
    auto previous = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(label, WM_GETFONT, 0, 0)));
    TEXTMETRICW metrics{};
    GetTextMetricsW(dc, &metrics);
    const int line = std::max(1L, metrics.tmHeight);
    const int inset = std::min(line / 2, width / 4);
    const int textWidth = std::min<int>(width - 2 * inset, 60 * std::max(1L, metrics.tmAveCharWidth));
    RECT text{0, 0, textWidth, 0};
    DrawTextW(dc, message.c_str(), -1, &text, DT_CALCRECT | DT_WORDBREAK | DT_CENTER | DT_NOPREFIX);
    SelectObject(dc, previous);
    ReleaseDC(label, dc);

    const int top = line / 4;
    const int buttonWidth = std::min(textWidth, 11 * line);
    const int buttonHeight = std::min(height, std::clamp<int>(height - top - text.bottom - line, line * 2, line * 7 / 2));
    const int buttonY = std::min<int>(top + text.bottom + line, height - buttonHeight);
    SetWindowPos(label, nullptr, (width - textWidth) / 2, top, textWidth,
                 std::max(0, buttonY - top), SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, IDC_ADD_SERVER), nullptr, (width - buttonWidth) / 2, buttonY,
                 buttonWidth, buttonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

void View::layout(int width, int height) {
    if (servers.empty()) {
        if (welcomePane) {
            layoutWelcome(welcomePane, width, height);
            SetWindowPos(welcomePane, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        return;
    }
    const int dpi = windowDpi(hwnd);
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
                     SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
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
    const HWND sortButton = GetDlgItem(hwnd, IDC_SORT_MENU);
    auto buttonFont = reinterpret_cast<HFONT>(SendMessageW(sortButton, WM_GETFONT, 0, 0));
    auto oldFont = buttonFont ? SelectObject(dc, buttonFont) : nullptr;
    int menuWidth = 0;
    // WACUP's skinned button reports the ideal size of its current caption.
    // Measure the whole set in the button font so the toolbar stays still.
    for (const auto& choice : sortChoices(sourceKind())) {
        RECT text{};
        DrawTextW(dc, choice.label, -1, &text, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        menuWidth = std::max(menuWidth, static_cast<int>(text.right));
    }
    if (oldFont) SelectObject(dc, oldFont);
    menuWidth += MulDiv(24, dpi, 96);
    const int optionsLeft = std::max(0, width - gap - row);
    const int menuLeft = std::max(0, optionsLeft - gap - menuWidth);
    const int sourceWidth = std::min(controlWidth(IDC_SOURCE_MENU, 12), width / 3);
    const int sourceLeft = std::max(0, menuLeft - gap - sourceWidth);
    move(IDC_QUERY, left, gap, sourceLeft - gap - left, row);
    move(IDC_SOURCE_MENU, sourceLeft, gap, sourceWidth, row);
    move(IDC_SORT_MENU, menuLeft, gap, std::min(menuWidth, optionsLeft - menuLeft), row);
    move(IDC_VIEW_OPTIONS, optionsLeft, gap, row, row);
    const int listTop = row + gap * 2;
    const bool filters = filtered(), preview = hasPreview(), split = filters || preview;
    const bool right = filters ? options.filtersRight : options.albumsRight;
    RECT first{gap, listTop, std::max(gap, width - gap), std::max(listTop, footer - gap)}, second = first;
    visible(IDC_DIVIDER_H, split && !right);
    visible(IDC_DIVIDER_V, split && right);
    if (split) {
        splitStart = right ? first.left : first.top;
        splitLength = right ? first.right - first.left : first.bottom - first.top;
        int middle = splitStart + MulDiv(splitLength, options.splitPercent, 100);
        const int bar = std::max(4, gap * 2);
        if (preview && showTiles() && !right && options.splitPercent == 45) {
            const int caption = options.coverTitles && !options.coverDetails ? 2 * metrics.tmHeight : 0;
            const int coverRow = tileSize + caption + row + gap * 2;
            if (splitLength >= coverRow + bar + row * 2) middle = std::max(middle, splitStart + coverRow);
        }
        if (right) {
            first.right = middle; second.left = middle + bar;
            move(IDC_DIVIDER_V, middle, first.top, bar, first.bottom - first.top);
        } else {
            first.bottom = middle; second.top = middle + bar;
            move(IDC_DIVIDER_H, first.left, middle, first.right - first.left, bar);
        }
    }
    auto placePane = [&](int id, RECT bounds) {
        move(id, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
    };
    placePane(GetDlgCtrlID(list), filters ? second : first);
    visible(IDC_RESULTS, preview);
    if (preview) placePane(IDC_RESULTS, second);
    for (int i = 0; i < 3; ++i) {
        const int n = static_cast<int>(options.filters.size());
        visible(IDC_FILTER_FIRST + i, filters && i < n);
        if (!filters || i >= n) continue;
        RECT pane = first;
        if (right) {
            pane.top += MulDiv(first.bottom - first.top, i, n);
            pane.bottom = first.top + MulDiv(first.bottom - first.top, i + 1, n) - (i + 1 < n ? gap : 0);
        } else {
            pane.left += MulDiv(first.right - first.left, i, n);
            pane.right = first.left + MulDiv(first.right - first.left, i + 1, n) - (i + 1 < n ? gap : 0);
        }
        placePane(IDC_FILTER_FIRST + i, pane);
    }
    left = gap;
    auto action = [&](int id, bool show) {
        visible(id, show);
        if (show) place(id, footer, 10);
    };
    action(IDC_PLAY, currentServer && (showTracks() || albumActions() || preview));
    action(IDC_ENQUEUE, currentServer && (showTracks() || albumActions() || preview));
    action(IDC_OPEN, currentServer && !showTracks());
    action(IDC_CACHE_MENU, currentServer && (showTracks() || groupedAlbums() || preview));
    move(IDC_STATUS, left, footer, width - gap - left, row);
    SelectObject(dc, previous);
    ReleaseDC(hwnd, dc);
    resizeColumns();
    scheduleArtwork();
}

void View::setColumns() {
    auto header = [](HWND control, const Columns& fields, const std::vector<std::string>& keys) {
        int count = Header_GetItemCount(ListView_GetHeader(control));
        while (count > static_cast<int>(keys.size())) ListView_DeleteColumn(control, --count);
        for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
            const auto* f = findColumn(fields, keys[i]);
            if (!f) continue;
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_SUBITEM;
            column.iSubItem = i;
            column.pszText = const_cast<wchar_t*>(f->label);
            if (i < count) ListView_SetColumn(control, i, &column);
            else ListView_InsertColumn(control, i, &column);
        }
    };
    auto kind = groupedAlbums() ? BrowseKind::albums : sourceKind();
    const auto keys = showTiles() && !options.coverDetails ? std::vector<std::string>{"name"} :
        (showTracks() ? options.trackColumns : options.itemColumns);
    header(list, viewFields(kind), keys);
    const auto sorted = std::find(keys.begin(), keys.end(), order.key);
    MLSkinnedListView_DisplaySort(list, sorted == keys.end() ? -1 : static_cast<int>(sorted - keys.begin()), !descending);
    if (hasPreview()) header(GetDlgItem(hwnd, IDC_RESULTS), viewFields(BrowseKind::tracks), options.trackColumns);
    if (filtered()) {
        auto fields = filterFields(location.kind);
        for (size_t i = 0; i < options.filters.size(); ++i) {
            auto& key = options.filters[i];
            header(GetDlgItem(hwnd, IDC_FILTER_FIRST + static_cast<int>(i)),
                   summaryFields(findColumn(fields, key)->label), options.filterColumns.at(key));
        }
    }
}

void View::resizeColumns() {
    int dpi = windowDpi(hwnd);
    auto fit = [dpi](HWND control, const Columns& fields, const std::vector<std::string>& keys) {
        int total = 0, minimum = 0;
        HDC dc = GetDC(control);
        auto previous = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0)));
        std::vector<int> widths;
        for (auto& key : keys) {
            const auto* f = findColumn(fields, key);
            if (!f) { widths.push_back(0); continue; }
            RECT extent{};
            DrawTextW(dc, f->label, -1, &extent, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            widths.push_back(extent.right + MulDiv(18, dpi, 96));
            minimum += widths.back();
            total += f->width;
        }
        SelectObject(dc, previous);
        ReleaseDC(control, dc);
        if (!total) return;
        const int extra = std::max(0, listViewportWidth(control) - minimum);
        int weight = 0, left = 0;
        for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
            const auto* f = findColumn(fields, keys[i]);
            if (!f) continue;
            weight += f->width;
            const int right = MulDiv(extra, weight, total);
            const int width = widths[i] + right - left;
            if (ListView_GetColumnWidth(control, i) != width) ListView_SetColumnWidth(control, i, width);
            left = right;
        }
    };
    if (showTiles() && !options.coverDetails) ListView_SetColumnWidth(list, 0, listViewportWidth(list));
    else fit(list, viewFields(groupedAlbums() ? BrowseKind::albums : sourceKind()), showTracks() ? options.trackColumns : options.itemColumns);
    if (hasPreview()) fit(GetDlgItem(hwnd, IDC_RESULTS), viewFields(BrowseKind::tracks), options.trackColumns);
    if (filtered()) {
        auto fields = filterFields(location.kind);
        for (size_t i = 0; i < options.filters.size(); ++i) {
            auto& key = options.filters[i];
            fit(GetDlgItem(hwnd, IDC_FILTER_FIRST + static_cast<int>(i)),
                summaryFields(findColumn(fields, key)->label), options.filterColumns.at(key));
        }
    }
}

LRESULT CALLBACK searchProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR view) {
    if (msg == WM_GETDLGCODE || msg == WM_KEYDOWN) {
        const int id = GetDlgCtrlID(hwnd);
        if (w == VK_F2 || w == VK_F5 || (w == VK_RETURN && (id == IDC_QUERY || id == IDC_ADD_SERVER))) {
            if (msg == WM_GETDLGCODE) return DefSubclassProc(hwnd, msg, w, l) | DLGC_WANTMESSAGE;
            const int command = w == VK_RETURN && id == IDC_ADD_SERVER ? id : (w == VK_F2 ? IDC_EDIT_VIEW : IDC_SEARCH);
            SendMessageW(reinterpret_cast<HWND>(view), WM_COMMAND, command, 0);
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, w, l);
}

INT_PTR CALLBACK welcomeProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_INITDIALOG) {
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        skinWindow(GetDlgItem(hwnd, IDC_WELCOME_TEXT), SKINNEDWND_TYPE_STATIC);
        HWND button = GetDlgItem(hwnd, IDC_ADD_SERVER);
        skinWindow(button, SKINNEDWND_TYPE_BUTTON);
        SetWindowSubclass(button, searchProc, 1, reinterpret_cast<DWORD_PTR>(GetParent(hwnd)));
        return TRUE;
    }
    if (msg == WM_COMMAND && LOWORD(w) == IDC_ADD_SERVER && HIWORD(w) == BN_CLICKED) {
        SendMessageW(GetParent(hwnd), msg, w, l);
        return TRUE;
    }
    return FALSE;
}

INT_PTR CALLBACK viewProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    auto view = reinterpret_cast<View*>(GetWindowLongPtrW(hwnd, DWLP_USER));
    try {
        if (msg == WM_INITDIALOG && app) {
            view = new View(hwnd, *reinterpret_cast<const Navigation*>(l));
            SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(view));
            app->activeView = hwnd;
            for (HWND child = GetWindow(hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
                SetWindowSubclass(child, searchProc, 1, reinterpret_cast<DWORD_PTR>(hwnd));
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
        case WM_SHOWWINDOW: view->scheduleArtwork(); break;
        case WM_COMMAND: view->command(LOWORD(w), HIWORD(w)); return TRUE;
        case WM_NOTIFY: {
            auto notification = reinterpret_cast<NMHDR*>(l);
            if (notification->idFrom == IDC_RESULTS || (notification->idFrom >= IDC_FILTER_FIRST && notification->idFrom <= IDC_FILTER_THIRD)) {
                if (notification->code == LVN_GETDISPINFOW) { view->displayOther(*reinterpret_cast<NMLVDISPINFOW*>(l)); return TRUE; }
                if (notification->idFrom == IDC_RESULTS) {
                    if (notification->code == LVN_BEGINDRAG) { view->beginDrag(*reinterpret_cast<NMLISTVIEW*>(l)); return TRUE; }
                    if (notification->code == NM_SETFOCUS) view->focusResults(true);
                    if (notification->code == LVN_ITEMCHANGED || notification->code == LVN_ODSTATECHANGED) view->queueSelectionUpdate();
                    if (notification->code == NM_DBLCLK || (notification->code == LVN_KEYDOWN &&
                        reinterpret_cast<NMLVKEYDOWN*>(l)->wVKey == VK_RETURN)) view->playPreview();
                } else if (notification->code == LVN_ITEMCHANGED || notification->code == LVN_ODSTATECHANGED) {
                    view->filterEvent(static_cast<int>(notification->idFrom) - IDC_FILTER_FIRST);
                }
                break;
            }
            if (notification->hwndFrom != view->list) break;
            if (notification->code == LVN_COLUMNCLICK) view->sortColumn(reinterpret_cast<NMLISTVIEW*>(l)->iSubItem);
            if (notification->code == NM_SETFOCUS) view->focusResults(false);
            if (notification->code == LVN_ODCACHEHINT || notification->code == LVN_ENDSCROLL)
                view->scheduleArtwork();
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
            if (w == splitTimer) { KillTimer(hwnd, splitTimer); view->saveOptions(); }
            return TRUE;
        case profilesChanged:
            if (l) view->setCategory(static_cast<BrowseKind>(w));
            view->populateServers();
            return TRUE;
        case viewReady:
            if (auto result = app->takeResult(hwnd)) view->receive(std::move(*result));
            return TRUE;
        case detailsReady:
            if (auto result = app->takeDetails(hwnd)) view->receiveDetails(std::move(*result));
            return TRUE;
        case filtersChanged: view->filterSelectionChanged(static_cast<int>(w)); return TRUE;
        case ArtworkLoader::readyMessage: view->receiveArtwork(); return TRUE;
        case artworkViewportChanged: view->updateArtworkViewport(); return TRUE;
        case populateTiles: view->fillTiles(); return TRUE;
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

UINT_PTR addTreeItem(std::wstring label, std::wstring invariant, UINT_PTR parent, bool branch) {
    NAVINSERTSTRUCT insert{};
    insert.hParent = parent ? MLNavCtrl_FindItemById(plugin.hwndLibraryParent, parent) : nullptr;
    insert.hInsertAfter = NCI_LAST;
    auto& item = insert.item;
    item.cbSize = sizeof(item);
    item.mask = NIMF_TEXT | NIMF_TEXTINVARIANT | NIMF_STYLE;
    item.pszText = label.data();
    item.pszInvariant = invariant.data();
    item.styleMask = NIS_HASCHILDREN | NIS_DEFAULTIMAGE;
    item.style = NIS_DEFAULTIMAGE | (branch ? NIS_HASCHILDREN : 0);
    item.hItem = MLNavCtrl_InsertItem(plugin.hwndLibraryParent, &insert);
    item.mask = NIMF_ITEMID;
    if (!item.hItem || !MLNavItem_GetInfo(plugin.hwndLibraryParent, &item))
        throw Error(TS_PRODUCT_NAME " could not add its Media Library entry.");
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
            throw Error("WACUP could not refresh the " TS_PRODUCT_NAME " tree.");
    }
    app->navigation.clear();
    app->navigation.emplace(root, Navigation{BrowseKind::albums, 0, true});
    auto addCategories = [&](UINT_PTR parent, Id server) {
        for (const auto& entry : categories)
            app->navigation.emplace(addTreeItem(entry.name, entry.name, parent, false), Navigation{entry.kind, server, false});
    };
    if (branches.empty()) addCategories(root, 0);
    for (const auto& [server, name] : branches) {
        const auto parent = addTreeItem(wide(name), L"server-" + std::to_wstring(server), root, true);
        app->navigation.emplace(parent, Navigation{BrowseKind::albums, server, true});
        addCategories(parent, server);
    }
    app->serverBranches = std::move(branches);
    if (restore) selectCategory(category, filter);
}

int init() {
    try {
        closingInput = false;
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
        if (!InitCommonControlsEx(&controls)) throw Error("Windows could not initialize Media Library controls.");
        const auto settings = settingsDirectory(plugin.hwndWinampParent);
        if (settings.empty()) throw Error("WACUP did not provide its profile directory.");
        const auto services = static_cast<UINT_PTR>(player(IPC_GET_API_SERVICE));
        app = std::make_unique<App>(settings / L"Plugins" / pluginName,
                                    services > 1 ? reinterpret_cast<api_service*>(services) : nullptr);
        if (!SetWindowSubclass(plugin.hwndWinampParent, playerProc, playerSubclass, 0))
            throw Error("WACUP playback notifications could not be attached.");
        app->attached = true;
        if (!SetTimer(plugin.hwndWinampParent, playbackTimer, 350, nullptr))
            throw Error("WACUP playback tracking could not start.");
        app->treeId = addTreeItem(pluginName, pluginName, 0, true);
        refreshNavigation();
        setDefaultPluginDescription(settings, L"library", L"ml_tangerinestream.dll",
                                    L"Navidrome and OpenSubsonic browsing by COOLTRAINER");
        { std::unique_lock lock(inputMutex); inputProfile = app.get(); }
        return 0;
    } catch (const std::exception& error) {
        // Roll back a failed registration. At normal shutdown WACUP saves the tree order itself.
        if (app && app->treeId) {
            auto handle = library(ML_IPC_TREEITEM_GETHANDLE, app->treeId);
            if (handle) library(ML_IPC_TREEITEM_DELETE, handle);
        }
        quit();
        showError(plugin.hwndLibraryParent, error);
        return 1;
    }
}

void quit() {
    closingInput = true;
    { std::unique_lock lock(inputMutex); inputProfile = nullptr; }
    if (!app) return;
    if (app->attached) {
        KillTimer(plugin.hwndWinampParent, playbackTimer);
        RemoveWindowSubclass(plugin.hwndWinampParent, playerProc, playerSubclass);
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

extern "C" __declspec(dllexport) int __cdecl tangerineInputQueryV4(
    tangerine::InputRequest request, const wchar_t* filename, const wchar_t* field,
    wchar_t* dest, size_t capacity, HANDLE* lease, tangerine::SourceAttempts* attempts) {
    try { return tangerine::inputQuery(request, filename, field, dest, capacity, lease, attempts); }
    catch (...) { return 0; }
}
