#include "playback.hpp"
#include "artwork.hpp"
#include "resources.h"

#include <commctrl.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <optional>
#include <set>

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
constexpr UINT_PTR playerSubclass = 0x7151;
constexpr UINT getIniDirectoryW = 1335; // Winamp 5.58+
using Clock = std::chrono::steady_clock;

int init();
void quit();
INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR c);

char description[] = "Tangerine Stream v0.2.0";
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
};

struct SearchResult {
    HWND view;
    std::uint64_t serial;
    BrowsePage page;
    std::string error;
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

class App {
public:
    explicit App(const std::filesystem::path& profile) : store(profile), playback(store), artwork(store) {
        try {
            browser = std::thread([this] { browse(); });
            maintenance = std::thread([this] { maintain(); });
            reporter = std::thread([this] { report(); });
        } catch (...) {
            stop();
            throw;
        }
    }
    ~App() { stop(); }

    void search(HWND view, Id server, const BrowseLocation& location, std::string query, int offset) {
        {
            std::scoped_lock lock(searchMutex);
            request = Search{view, ++serial, server, location, std::move(query), offset};
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

    Store store;
    Playback playback;
    ArtworkLoader artwork;
    HWND activeView = nullptr;
    Id selectedServer = 0;
    UINT_PTR treeId = 0;
    std::map<UINT_PTR, BrowseKind> navigation;
    bool attached = false;
    std::string reportError;

private:
    std::atomic_bool stopping{false};
    std::thread browser, maintenance, reporter;
    std::mutex searchMutex, maintenanceMutex;
    std::condition_variable searchReady, maintenanceReady, reportsReady;
    std::uint64_t serial = 0;
    std::optional<Search> request;
    std::optional<SearchResult> result;
    bool pendingMaintenance = false, refreshFavorites = false, refreshReports = false;
    std::map<Id, ServiceStatus> serviceStatus;
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
            if (search.location.kind == BrowseKind::offline) {
                ready.page.tracks = store.cached(search.server, search.query, search.offset);
                ready.page.more = ready.page.tracks.size() == pageSize;
            } else {
                auto profile = store.server(search.server);
                if (!api || profile.id != current.id || profile.native != current.native ||
                    profile.password != current.password) {
                    api = connect(profile);
                    current = std::move(profile);
                }
                ready.page = api->browse(search.location, search.query, search.offset);
                if (!ready.page.tracks.empty()) store.remember(ready.page.tracks);
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
                if (!api) api = connect(store.server(profile.id));
                return *api;
            };
            if (Clock::now() >= nextFavorites[profile.id] || !profile.cacheFavorites) {
                std::string failure;
                if (profile.cacheFavorites) {
                    try {
                        nextFavorites[profile.id] = Clock::now() + minutes(10);
                        auto favorites = connection().favorites(stopping);
                        store.syncFavorites(profile.id, favorites);
                    } catch (const std::exception& error) {
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
                    try {
                        downloadPinned(store, track, connection(), stopping);
                    } catch (const std::exception& error) {
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
            if (Clock::now() < retryAfter[profile.id]) continue;
            std::string failure;
            try {
                std::unique_ptr<ServerApi> api;
                // Bound each round so one backlog cannot monopolize reporting.
                for (int i = 0; i < 4 && !stopping; ++i) {
                    auto batch = store.reports(profile.id);
                    if (batch.empty()) break;
                    if (!api) api = connect(store.server(profile.id));
                    api->submit(batch);
                    store.acknowledge(batch);
                }
            } catch (const std::exception& error) {
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

LRESULT CALLBACK playerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if (app && ((msg == WM_TIMER && w == playbackTimer) ||
                (msg == WM_WA_IPC && l == IPC_PLAYING_FILEW && w > 1))) {
        try {
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
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        ListView_SetUnicodeFormat(list, TRUE);
        skinWindow(hwnd, SKINNEDWND_TYPE_DIALOG);
        if (!skinWindow(list, SKINNEDWND_TYPE_LISTVIEW, SWLVS_DOUBLEBUFFER)) {
            legacySkin = library(ML_IPC_SKIN_LISTVIEW, reinterpret_cast<WPARAM>(list));
        }
        skinWindow(GetDlgItem(hwnd, IDC_SERVERS), SKINNEDWND_TYPE_COMBOBOX);
        skinWindow(GetDlgItem(hwnd, IDC_QUERY), SKINNEDWND_TYPE_EDIT, SWES_VCENTER);
        for (int id : {IDC_LOCATION, IDC_STATUS})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_STATIC);
        for (int id : {IDC_ADD_SERVER, IDC_SETTINGS, IDC_BACK, IDC_SEARCH, IDC_TEXT_LIST,
                       IDC_OPEN, IDC_PLAY, IDC_ENQUEUE, IDC_KEEP_OFFLINE, IDC_UNPIN})
            skinWindow(GetDlgItem(hwnd, id), SKINNEDWND_TYPE_BUTTON);
        ListView_SetTextBkColor(list, CLR_NONE);
        tileSize = std::clamp(MulDiv(128, GetDpiForWindow(hwnd), 96), 96, 256);
        SetTimer(hwnd, viewTimer, 2000, nullptr);
        SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETLIMITTEXT, 256, 0);
        CheckDlgButton(hwnd, IDC_TEXT_LIST, app->store.textCatalog() ? BST_CHECKED : BST_UNCHECKED);
    }
    ~View() {
        KillTimer(hwnd, viewTimer);
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
    void layout(int width, int height);
    HWND hwnd, list;

private:
    struct History {
        BrowseLocation location;
        std::string query;
        std::vector<LibraryItem> items;
        int nextOffset;
        int focused;
        bool more;
    };
    struct ImageSlot {
        int row;
        std::uint64_t read;
    };

    BrowseKind category;
    BrowseLocation location;
    std::string query;
    std::vector<History> history;
    std::vector<Server> servers;
    std::vector<LibraryItem> items;
    std::vector<Track> tracks;
    std::set<std::pair<BrowseKind, std::string>> knownItems;
    std::set<Id> knownTracks;
    int nextOffset = 0;
    bool loading = false, more = false, morePosted = false;
    std::string message;
    LRESULT legacySkin = 0;
    Id currentServer = 0;
    int tileSize = 128;
    HIMAGELIST images = nullptr;
    ArtworkLoader::Generation artGeneration = 0;
    std::vector<ImageSlot> slots;
    std::map<int, int> slotForRow;
    std::set<int> pendingArt, missingArt;
    std::uint64_t artRead = 0;
    bool artQueueFull = false;

    bool showTracks() const { return trackView(location.kind); }
    bool showTiles() const {
        return !showTracks() && IsDlgButtonChecked(hwnd, IDC_TEXT_LIST) != BST_CHECKED;
    }
    int rowCount() const { return static_cast<int>(showTracks() ? tracks.size() : items.size()); }
    void startSearch();
    void resetContent();
    void resetLocation();
    void changePresentation();
    void resizeColumns();
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
    auto index = SendDlgItemMessageW(hwnd, IDC_SERVERS, CB_GETCURSEL, 0, 0);
    return index >= 0 && static_cast<std::size_t>(index) < servers.size() ? &servers[index] : nullptr;
}

void View::populateServers() {
    servers = app->store.servers();
    SendDlgItemMessageW(hwnd, IDC_SERVERS, CB_RESETCONTENT, 0, 0);
    int selectedIndex = 0;
    for (std::size_t i = 0; i < servers.size(); ++i) {
        auto name = wide(servers[i].name);
        SendDlgItemMessageW(hwnd, IDC_SERVERS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        if (servers[i].id == app->selectedServer) selectedIndex = static_cast<int>(i);
    }
    SendDlgItemMessageW(hwnd, IDC_SERVERS, CB_SETCURSEL, selectedIndex, 0);
    resetLocation();
}

void View::resetLocation() {
    if (!history.empty()) SetDlgItemTextW(hwnd, IDC_QUERY, L"");
    history.clear();
    location = categoryLocation(category);
    startSearch();
}

void View::resetContent() {
    app->cancelSearch(hwnd);
    loading = more = morePosted = false;
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
    app->search(hwnd, currentServer, location, query, nextOffset);
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
    loading = false;
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
    if (emptyPage || rowCount() < 12) scheduleMore();
}

void View::refresh() {
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
    constexpr std::size_t maxImages = 192;
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
    if (tiles && images) ListView_SetIconSpacing(list, tileSize + tileSize / 5, tileSize + tileSize / 3);
    auto breadcrumb = wide(categoryLocation(category).title);
    for (std::size_t i = 1; i < history.size(); ++i)
        breadcrumb += L"  \u203a  " + wide(history[i].location.title);
    if (!history.empty()) breadcrumb += L"  \u203a  " + wide(location.title);
    SetDlgItemTextW(hwnd, IDC_LOCATION, breadcrumb.c_str());
    SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETCUEBANNER, FALSE,
        reinterpret_cast<LPARAM>(showTracks() ? L"Search tracks" : L"Search this view"));
    for (int id : {IDC_PLAY, IDC_ENQUEUE, IDC_KEEP_OFFLINE, IDC_UNPIN})
        ShowWindow(GetDlgItem(hwnd, id), showTracks() ? SW_SHOWNA : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, IDC_OPEN), showTracks() ? SW_HIDE : SW_SHOWNA);
    ShowWindow(GetDlgItem(hwnd, IDC_TEXT_LIST), showTracks() ? SW_HIDE : SW_SHOWNA);
    ShowWindow(GetDlgItem(hwnd, IDC_BACK), history.empty() ? SW_HIDE : SW_SHOWNA);
    auto title = showTracks() ? L"Title" : L"Name";
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
    history.push_back({location, query, std::move(items), nextOffset, index, more});
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
    resetContent();
    items = std::move(outer.items);
    nextOffset = outer.nextOffset;
    more = outer.more;
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
    SetDlgItemTextW(hwnd, IDC_STATUS, status.c_str());
}

void View::updateButtons() {
    const bool haveServer = selected() != nullptr;
    const bool haveRows = ListView_GetSelectedCount(list) > 0;
    for (int id : {IDC_SEARCH, IDC_SETTINGS, IDC_QUERY, IDC_TEXT_LIST})
        EnableWindow(GetDlgItem(hwnd, id), haveServer);
    for (int id : {IDC_PLAY, IDC_ENQUEUE, IDC_KEEP_OFFLINE, IDC_UNPIN})
        EnableWindow(GetDlgItem(hwnd, id), haveRows);
    EnableWindow(GetDlgItem(hwnd, IDC_OPEN), haveRows);
}

std::vector<Track> View::selection() {
    std::vector<Track> selectedTracks;
    for (int i = ListView_GetNextItem(list, -1, LVNI_SELECTED); i >= 0;
         i = ListView_GetNextItem(list, i, LVNI_SELECTED)) {
        if (static_cast<std::size_t>(i) < tracks.size()) selectedTracks.push_back(tracks[i]);
    }
    return selectedTracks;
}

void View::play(bool enqueue) {
    auto items = selection();
    if (items.empty()) return;
    if (!enqueue) player(IPC_DELETE);
    for (const auto& track : items) {
        auto url = app->playback.url(track);
        auto title = wide(track.artist.empty() ? track.title : track.artist + " - " + track.title);
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

void View::command(int id, int notification) {
    switch (id) {
    case IDC_ADD_SERVER: configure(hwnd); break;
    case IDC_SETTINGS: if (auto server = selected()) configure(hwnd, *server); break;
    case IDC_SERVERS: if (notification == CBN_SELCHANGE) resetLocation(); break;
    case IDC_SEARCH: startSearch(); break;
    case IDC_BACK: back(); break;
    case IDC_OPEN: activate(ListView_GetNextItem(list, -1, LVNI_SELECTED)); break;
    case IDC_TEXT_LIST:
        app->store.saveTextCatalog(!showTiles());
        changePresentation();
        break;
    case IDC_PLAY: play(false); break;
    case IDC_ENQUEUE: play(true); break;
    case IDC_KEEP_OFFLINE: cache(true); break;
    case IDC_UNPIN: cache(false); break;
    }
}

void View::layout(int width, int height) {
    RECT unit{0, 0, 400, 8};
    MapDialogRect(hwnd, &unit);
    auto x = [&](int dlu) { return MulDiv(dlu, unit.right, 400); };
    auto y = [&](int dlu) { return MulDiv(dlu, unit.bottom, 8); };
    const int w = std::max(width, x(315));
    const int h = std::max(height, y(120));
    auto move = [&](int id, int left, int top, int controlWidth, int controlHeight) {
        MoveWindow(GetDlgItem(hwnd, id), left, top, controlWidth, controlHeight, TRUE);
    };
    move(IDC_SERVERS, x(4), y(4), w - x(162), y(100));
    move(IDC_ADD_SERVER, w - x(154), y(4), x(72), y(15));
    move(IDC_SETTINGS, w - x(78), y(4), x(74), y(15));
    const int queryLeft = history.empty() ? 4 : 48;
    const int searchRight = showTracks() ? 57 : 129;
    move(IDC_BACK, x(4), y(23), x(40), y(15));
    move(IDC_QUERY, x(queryLeft), y(23), w - x(queryLeft + searchRight + 4), y(15));
    move(IDC_SEARCH, w - x(searchRight), y(23), x(52), y(15));
    move(IDC_TEXT_LIST, w - x(73), y(23), x(69), y(15));
    move(IDC_LOCATION, x(4), y(42), w - x(8), y(12));
    move(IDC_TRACKS, x(4), y(55), w - x(8), h - y(96));
    move(IDC_OPEN, x(4), h - y(37), x(36), y(15));
    move(IDC_PLAY, x(4), h - y(37), x(36), y(15));
    move(IDC_ENQUEUE, x(44), h - y(37), x(49), y(15));
    move(IDC_KEEP_OFFLINE, x(97), h - y(37), x(60), y(15));
    move(IDC_UNPIN, x(161), h - y(37), x(39), y(15));
    move(IDC_STATUS, x(4), h - y(18), w - x(8), y(14));
    resizeColumns();
}

void View::resizeColumns() {
    if (showTiles()) return;
    RECT bounds{};
    GetClientRect(list, &bounds);
    const auto width = std::max(320L, bounds.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
    if (showTracks()) {
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
        case WM_SIZE: view->layout(LOWORD(l), HIWORD(l)); return TRUE;
        case WM_COMMAND: view->command(LOWORD(w), HIWORD(w)); return TRUE;
        case WM_NOTIFY: {
            auto notification = reinterpret_cast<NMHDR*>(l);
            if (notification->idFrom != IDC_TRACKS) break;
            if (notification->code == LVN_GETDISPINFOW) {
                view->display(*reinterpret_cast<NMLVDISPINFOW*>(l));
                return TRUE;
            }
            if (notification->code == NM_DBLCLK)
                view->activate(reinterpret_cast<NMITEMACTIVATE*>(l)->iItem);
            if (notification->code == LVN_KEYDOWN && reinterpret_cast<NMLVKEYDOWN*>(l)->wVKey == VK_RETURN)
                view->activate(ListView_GetNextItem(view->list, -1, LVNI_SELECTED));
            if (notification->code == LVN_ITEMCHANGED || notification->code == LVN_ODSTATECHANGED)
                view->updateButtons();
            break;
        }
        case WM_TIMER: if (w == viewTimer) view->refresh(); return TRUE;
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

std::filesystem::path profileDirectory() {
    auto wideDirectory = reinterpret_cast<const wchar_t*>(player(getIniDirectoryW));
    if (reinterpret_cast<UINT_PTR>(wideDirectory) > 1 && *wideDirectory)
        return std::filesystem::path(wideDirectory) / L"Plugins" / pluginName;
    auto iniDirectory = reinterpret_cast<const char*>(player(IPC_GETINIDIRECTORY));
    if (reinterpret_cast<UINT_PTR>(iniDirectory) > 1 && *iniDirectory) {
        int size = MultiByteToWideChar(CP_ACP, 0, iniDirectory, -1, nullptr, 0);
        if (size > 1) {
            std::wstring directory(size, L'\0');
            MultiByteToWideChar(CP_ACP, 0, iniDirectory, -1, directory.data(), size);
            directory.pop_back();
            return std::filesystem::path(directory) / L"Plugins" / pluginName;
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
        app = std::make_unique<App>(profileDirectory());
        if (!SetWindowSubclass(plugin.hwndWinampParent, playerProc, playerSubclass, 0))
            throw Error("WACUP playback notifications could not be attached.");
        app->attached = true;
        if (!SetTimer(plugin.hwndWinampParent, playbackTimer, 350, nullptr))
            throw Error("WACUP playback tracking could not start.");
        app->treeId = addTreeItem(pluginName, 0, true);
        app->navigation.emplace(app->treeId, BrowseKind::albums);
        for (const auto& category : categories)
            app->navigation.emplace(addTreeItem(category.name, app->treeId, false), category.kind);
        return 0;
    } catch (const std::exception& error) {
        quit();
        showError(plugin.hwndLibraryParent, error);
        return 1;
    }
}

void quit() {
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
