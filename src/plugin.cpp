#include "playback.hpp"
#include "resources.h"

#include <commctrl.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <optional>

// The SDK checks an undefined _MSC_VER as zero and supplies its own intptr_t.
#ifndef _MSC_VER
#define _MSC_VER 1900
#define RESTORE_MINGW_MACROS
#endif
#include "gen_ml/ml.h"
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
constexpr UINT_PTR playbackTimer = 0x7151;
constexpr UINT_PTR viewTimer = 1;
constexpr UINT_PTR playerSubclass = 0x7151;
constexpr UINT getIniDirectoryW = 1335; // Winamp 5.58+
using Clock = std::chrono::steady_clock;

int init();
void quit();
INT_PTR message(int type, INT_PTR a, INT_PTR b, INT_PTR c);

char description[] = "Tangerine Stream v0.1.0";
winampMediaLibraryPlugin plugin{MLHDR_VER, description, init, quit, message, nullptr, nullptr, nullptr};

LRESULT player(UINT command, WPARAM value = 0) {
    return SendMessageW(plugin.hwndWinampParent, WM_WA_IPC, value, command);
}

LRESULT library(UINT command, WPARAM value = 0) {
    return SendMessageW(plugin.hwndLibraryParent, WM_ML_IPC, value, command);
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
    std::string query;
    int offset;
    bool cached;
};

struct SearchResult {
    HWND view;
    std::uint64_t serial;
    std::vector<Track> tracks;
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
    explicit App(const std::filesystem::path& profile) : store(profile), playback(store) {
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

    void search(HWND view, Id server, std::string query, int offset, bool cached) {
        {
            std::scoped_lock lock(searchMutex);
            request = Search{view, ++serial, server, std::move(query), offset, cached};
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
    HWND activeView = nullptr;
    Id selectedServer = 0;
    UINT_PTR treeId = 0;
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
            if (search.cached) {
                ready.tracks = store.cached(search.server, search.query, search.offset);
            } else {
                auto profile = store.server(search.server);
                if (!api || profile.id != current.id || profile.native != current.native ||
                    profile.password != current.password) {
                    api = connect(profile);
                    current = std::move(profile);
                }
                ready.tracks = api->songs(search.query, search.offset);
                store.remember(ready.tracks);
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
    explicit View(HWND window) : hwnd(window), list(GetDlgItem(hwnd, IDC_TRACKS)) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        const wchar_t* names[] = {L"Title", L"Artist", L"Album", L"Time", L"Cache"};
        int widths[] = {180, 120, 140, 60, 125};
        for (int i = 0; i < 5; ++i) {
            column.pszText = const_cast<wchar_t*>(names[i]);
            column.cx = widths[i];
            ListView_InsertColumn(list, i, &column);
        }
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        skin = library(ML_IPC_SKIN_LISTVIEW, reinterpret_cast<WPARAM>(list));
        SetTimer(hwnd, viewTimer, 2000, nullptr);
        SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETLIMITTEXT, 256, 0);
        SendDlgItemMessageW(hwnd, IDC_QUERY, EM_SETCUEBANNER, FALSE,
                            reinterpret_cast<LPARAM>(L"Search tracks"));
    }
    ~View() {
        KillTimer(hwnd, viewTimer);
        if (skin) library(ML_IPC_UNSKIN_LISTVIEW, skin);
    }
    void populateServers();
    void load(int nextOffset);
    void receive(SearchResult result);
    void command(int id, int notification);
    void refresh(bool force = false);
    void updateButtons();
    void layout(int width, int height);
    HWND hwnd, list;

private:
    std::vector<Server> servers;
    std::vector<Track> tracks;
    int offset = 0;
    bool loading = false;
    std::string message;
    LRESULT skin = 0;
    void play(bool enqueue);
    void cache(bool pin);
    void updateStatus();
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
    load(0);
}

void View::load(int nextOffset) {
    offset = nextOffset;
    tracks.clear();
    ListView_DeleteAllItems(list);
    auto server = selected();
    loading = server != nullptr;
    message.clear();
    updateButtons();
    updateStatus();
    if (!server) return;
    app->selectedServer = server->id;
    app->search(hwnd, server->id, getText(hwnd, IDC_QUERY), offset,
                IsDlgButtonChecked(hwnd, IDC_CACHED) == BST_CHECKED);
}

void setCell(HWND list, int row, int column, std::wstring text) {
    ListView_SetItemText(list, row, column, text.data());
}

void View::receive(SearchResult result) {
    loading = false;
    message = std::move(result.error);
    if (!message.empty() && IsDlgButtonChecked(hwnd, IDC_CACHED) != BST_CHECKED)
        message += " Select Cache view to browse your offline tracks.";
    tracks = std::move(result.tracks);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        auto& track = tracks[i];
        auto title = wide(track.title.empty() ? track.id : track.title);
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = title.data();
        ListView_InsertItem(list, &item);
        setCell(list, i, 1, wide(track.artist));
        setCell(list, i, 2, wide(track.album));
        auto seconds = track.duration % 60;
        if (track.duration) setCell(list, i, 3, std::to_wstring(track.duration / 60) + L":" +
            (seconds < 10 ? L"0" : L"") + std::to_wstring(seconds));
    }
    refresh(true);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, TRUE);
    updateButtons();
}

void View::refresh(bool force) {
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        auto updated = app->store.track(tracks[i].key);
        const auto& track = tracks[i];
        if (!force && track.bytes == updated.bytes && track.favorite == updated.favorite &&
            track.pinned == updated.pinned) continue;
        tracks[i] = std::move(updated);
        const auto& current = tracks[i];
        setCell(list, i, 4, current.pinned ? (current.bytes ? L"Permanent" : L"Pin pending") :
            current.favorite ? (current.bytes ? L"Favorite" : L"Favorite pending") :
            current.bytes ? L"Rolling" : L"");
    }
    updateStatus();
}

void View::updateStatus() {
    auto server = selected();
    std::wstring status;
    if (!server) status = L"Add a Navidrome or OpenSubsonic server to get started.";
    else if (loading) status = L"Loading tracks...";
    else if (!message.empty()) status = wide(message);
    else if (!app->reportError.empty()) status = wide(app->reportError);
    else {
        status = tracks.empty() ? L"No tracks" : L"Tracks " + std::to_wstring(offset + 1) +
            L"\u2013" + std::to_wstring(offset + tracks.size());
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
    for (int id : {IDC_SEARCH, IDC_SETTINGS, IDC_QUERY, IDC_CACHED})
        EnableWindow(GetDlgItem(hwnd, id), haveServer);
    for (int id : {IDC_PLAY, IDC_ENQUEUE, IDC_KEEP_OFFLINE, IDC_UNPIN})
        EnableWindow(GetDlgItem(hwnd, id), haveRows);
    EnableWindow(GetDlgItem(hwnd, IDC_PREVIOUS), haveServer && !loading && offset >= pageSize);
    EnableWindow(GetDlgItem(hwnd, IDC_NEXT), haveServer && !loading &&
                 tracks.size() == pageSize && offset < INT_MAX - 2 * pageSize);
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
    case IDC_SERVERS: if (notification == CBN_SELCHANGE) load(0); break;
    case IDC_SEARCH:
    case IDC_CACHED: load(0); break;
    case IDC_PREVIOUS: load(std::max(0, offset - pageSize)); break;
    case IDC_NEXT: load(offset + pageSize); break;
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
    const int h = std::max(height, y(111));
    auto move = [&](int id, int left, int top, int controlWidth, int controlHeight) {
        MoveWindow(GetDlgItem(hwnd, id), left, top, controlWidth, controlHeight, TRUE);
    };
    move(IDC_SERVERS, x(4), y(4), w - x(162), y(100));
    move(IDC_ADD_SERVER, w - x(154), y(4), x(72), y(15));
    move(IDC_SETTINGS, w - x(78), y(4), x(74), y(15));
    move(IDC_QUERY, x(4), y(23), w - x(145), y(15));
    move(IDC_SEARCH, w - x(137), y(23), x(52), y(15));
    move(IDC_CACHED, w - x(81), y(23), x(77), y(15));
    move(IDC_TRACKS, x(4), y(42), w - x(8), h - y(83));
    move(IDC_PLAY, x(4), h - y(37), x(36), y(15));
    move(IDC_ENQUEUE, x(44), h - y(37), x(49), y(15));
    move(IDC_KEEP_OFFLINE, x(97), h - y(37), x(60), y(15));
    move(IDC_UNPIN, x(161), h - y(37), x(39), y(15));
    move(IDC_PREVIOUS, w - x(104), h - y(37), x(48), y(15));
    move(IDC_NEXT, w - x(52), h - y(37), x(48), y(15));
    move(IDC_STATUS, x(4), h - y(18), w - x(8), y(14));
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
            view = new View(hwnd);
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
            if (notification->idFrom == IDC_TRACKS && notification->code == NM_DBLCLK)
                view->command(IDC_PLAY, 0);
            if (notification->idFrom == IDC_TRACKS && notification->code == LVN_ITEMCHANGED)
                view->updateButtons();
            break;
        }
        case WM_TIMER: if (w == viewTimer) view->refresh(); return TRUE;
        case profilesChanged: view->populateServers(); return TRUE;
        case viewReady:
            if (auto result = app->takeResult(hwnd)) view->receive(std::move(*result));
            return TRUE;
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
        std::wstring label = pluginName;
        MLTREEITEMW item{sizeof(item), 0, 0, label.data(), 0, FALSE, MLTREEIMAGE_DEFAULT};
        library(ML_IPC_TREEITEM_ADDW, reinterpret_cast<WPARAM>(&item));
        if (!item.id) throw Error("Tangerine Stream could not add its Media Library entry.");
        app->treeId = item.id;
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
        if (type == ML_MSG_TREE_ONCREATEVIEW && static_cast<UINT_PTR>(a) == app->treeId)
            return reinterpret_cast<INT_PTR>(CreateDialogParamW(plugin.hDllInstance,
                MAKEINTRESOURCEW(IDD_LIBRARY), reinterpret_cast<HWND>(b), viewProc, 0));
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
