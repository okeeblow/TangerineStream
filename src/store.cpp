#include "store.hpp"
#include "server.hpp"
#include "tangerine_product.h"

#include <sqlite3.h>
#include <wincrypt.h>
#include <algorithm>
#include <charconv>
#include <set>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace tangerine {
namespace {

constexpr int defaultBudget = 1024;
constexpr std::int64_t artworkBudget = 128 * MiB;

[[noreturn]] void databaseError(sqlite3* db) {
    throw Error(std::string(TS_PRODUCT_NAME " storage: ") + (db ? sqlite3_errmsg(db) : "out of memory"));
}

} // namespace

class Statement {
public:
    Statement(sqlite3* db, const char* sql, sqlite3_stmt** slot = nullptr) : db_(db), slot_(slot) {
        if (slot_) statement_ = std::exchange(*slot_, nullptr);
        if (!statement_ && sqlite3_prepare_v3(db_, sql, -1, slot_ ? SQLITE_PREPARE_PERSISTENT : 0,
                                             &statement_, nullptr) != SQLITE_OK) databaseError(db_);
    }
    ~Statement() {
        if (slot_ && !*slot_) {
            sqlite3_reset(statement_);
            sqlite3_clear_bindings(statement_);
            *slot_ = statement_;
        } else {
            sqlite3_finalize(statement_);
        }
    }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement& bind(std::int64_t value) {
        if (sqlite3_bind_int64(statement_, next_++, value) != SQLITE_OK) databaseError(db_);
        return *this;
    }
    Statement& bind(const std::string& value) {
        if (sqlite3_bind_text(statement_, next_++, value.c_str(), static_cast<int>(value.size()),
                              SQLITE_TRANSIENT) != SQLITE_OK) databaseError(db_);
        return *this;
    }
    Statement& bindBlob(const std::string& value) {
        if (sqlite3_bind_blob(statement_, next_++, value.data(), static_cast<int>(value.size()),
                              SQLITE_TRANSIENT) != SQLITE_OK) databaseError(db_);
        return *this;
    }
    bool row() {
        auto result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) return true;
        if (result == SQLITE_DONE) return false;
        databaseError(db_);
    }
    void run() { while (row()) {} }
    std::int64_t integer(int index) const { return sqlite3_column_int64(statement_, index); }
    std::string text(int index) const {
        auto text = sqlite3_column_text(statement_, index);
        return text ? reinterpret_cast<const char*>(text) : "";
    }
    std::string blob(int index) const {
        auto bytes = static_cast<const char*>(sqlite3_column_blob(statement_, index));
        return bytes ? std::string(bytes, sqlite3_column_bytes(statement_, index)) : std::string{};
    }

private:
    sqlite3* db_;
    sqlite3_stmt** slot_;
    sqlite3_stmt* statement_ = nullptr;
    int next_ = 1;
};

namespace {

class Transaction {
public:
    explicit Transaction(sqlite3* db) : db_(db) { Statement(db, "BEGIN IMMEDIATE").run(); }
    ~Transaction() {
        if (!committed_) {
            try { Statement(db_, "ROLLBACK").run(); } catch (...) {}
        }
    }
    void commit() {
        Statement(db_, "COMMIT").run();
        committed_ = true;
    }

private:
    sqlite3* db_;
    bool committed_ = false;
};

void migrate(sqlite3* db) {
    static constexpr const char* migrations[] = {
#include "migrations.inc"
    };
    Transaction transaction(db);
    Statement version(db, "PRAGMA user_version");
    if (!version.row()) databaseError(db);
    auto schema = version.integer(0);
    version.run();
    const auto latest = static_cast<std::int64_t>(std::size(migrations));
    if (schema < 0 || schema > latest)
        throw Error("Unsupported library database version " + std::to_string(schema) +
                    ". This plug-in supports versions 0 through " + std::to_string(latest) + ".");
    // Submit listens queued by earlier releases using their original playback times.
    if (schema <= 8 && sqlite3_table_column_metadata(db, "main", "reports", "key",
            nullptr, nullptr, nullptr, nullptr, nullptr) == SQLITE_OK) {
        Statement(db, "ALTER TABLE reports RENAME TO pending_listens").run();
        Statement(db, "ALTER TABLE pending_listens RENAME COLUMN time TO listened_at_ms").run();
        Statement(db, "DROP INDEX IF EXISTS reports_server").run();
        Statement(db, "CREATE INDEX pending_listens_submission_order ON pending_listens(server, key)").run();
    }
    // Keep covers already cached by preview builds that used the original column names.
    if (schema >= 6 && schema <= 8) {
        auto renameArtworkColumn = [db](const char* previous, const char* current) {
            if (sqlite3_table_column_metadata(db, "main", "artwork", previous, nullptr, nullptr,
                                              nullptr, nullptr, nullptr) != SQLITE_OK) return;
            const auto sql = std::string("ALTER TABLE artwork RENAME COLUMN ") + previous + " TO " + current;
            Statement(db, sql.c_str()).run();
        };
        renameArtworkColumn("requested_size", "requested_max_edge_px");
        renameArtworkColumn("pixel_size", "cached_longest_edge_px");
    }
    for (; schema < latest; ++schema) {
        if (sqlite3_exec(db, migrations[schema], nullptr, nullptr, nullptr) != SQLITE_OK)
            throw Error("Library database migration " + std::to_string(schema + 1) +
                        " failed: " + sqlite3_errmsg(db));
        auto update = "PRAGMA user_version=" + std::to_string(schema + 1);
        Statement(db, update.c_str()).run();
    }
    transaction.commit();
}

constexpr char trackColumns[] =
    "key,server,remote_id,title,artist,album,suffix,mime,duration,pinned,favorite,bytes,"
    "cover_id,album_id,artist_id,album_artist_id,album_artist,genre,disc_number,track_number,year,sort_tags,cached_format";

// Windows linguistic search can equate text with different Unicode spellings.
constexpr char linguisticRows[] =
    "title GLOB '*[^ -~]*' OR artist GLOB '*[^ -~]*' OR album GLOB '*[^ -~]*'";

void sqliteContains(sqlite3_context* context, int, sqlite3_value** args) {
    auto text = static_cast<const wchar_t*>(sqlite3_value_text16(args[0]));
    auto query = static_cast<const wchar_t*>(sqlite3_value_text16(args[1]));
    if (!text || !query) { sqlite3_result_error_nomem(context); return; }
    const auto length = static_cast<std::size_t>(sqlite3_value_bytes16(args[1])) / sizeof(wchar_t);
    sqlite3_result_int(context, matches(std::wstring_view{text}, std::wstring_view{query, length}));
}

std::string trigramPhrase(const std::string& query) {
    if (query.size() < 3 || !std::all_of(query.begin(), query.end(),
        [](unsigned char ch) { return ch >= 0x20 && ch <= 0x7e; })) return {};
    std::string phrase = "\"";
    for (char ch : query) {
        if (ch == '"') phrase += '"';
        phrase += ch;
    }
    return phrase + '"';
}

Track getTrack(const Statement& statement) {
    return {statement.integer(0), statement.integer(1), statement.text(2), statement.text(3),
            statement.text(4), statement.text(5), statement.text(6), statement.text(7),
            static_cast<int>(statement.integer(8)), statement.integer(9) != 0,
            statement.integer(10) != 0, statement.integer(11), statement.text(12), statement.text(13),
            statement.text(14), statement.text(15), statement.text(16), statement.text(17),
            static_cast<int>(statement.integer(18)), static_cast<int>(statement.integer(19)),
            static_cast<int>(statement.integer(20)), decodeSortTags(statement.text(21)), statement.text(22)};
}

std::string protect(std::string password) {
    DATA_BLOB input{static_cast<DWORD>(password.size()), reinterpret_cast<BYTE*>(password.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, TS_PRODUCT_NAME_W, nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output))
        throw Error("Windows could not protect the saved password.");
    std::string result;
    constexpr char digits[] = "0123456789abcdef";
    for (DWORD i = 0; i < output.cbData; ++i) {
        result += digits[output.pbData[i] >> 4];
        result += digits[output.pbData[i] & 15];
    }
    LocalFree(output.pbData);
    return result;
}

std::string unprotect(const std::string& data) {
    if (data.size() % 2) throw Error("Set the password again for this server.");
    std::string bytes;
    for (std::size_t i = 0; i + 1 < data.size(); i += 2) {
        unsigned byte = 0;
        auto parsed = std::from_chars(data.data() + i, data.data() + i + 2, byte, 16);
        if (parsed.ec != std::errc{} || parsed.ptr != data.data() + i + 2)
            throw Error("Set the password again for this server.");
        bytes += static_cast<char>(byte);
    }
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE*>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        throw Error("Windows could not read this server's password. Set it again in Server settings.");
    std::string result(reinterpret_cast<char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return result;
}

} // namespace

Store::Store(const std::filesystem::path& directory) : cache_(directory / L"cache") {
    std::filesystem::create_directories(cache_);
    lockFile_ = CreateFileW((directory / L"profile.lock").c_str(), GENERIC_WRITE, 0, nullptr,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lockFile_ == INVALID_HANDLE_VALUE)
        throw Error("The " TS_PRODUCT_NAME " profile could not be opened exclusively.");
    try {
        if (sqlite3_open_v2(utf8((directory / L"library.sqlite3").wstring()).c_str(), &db_,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) databaseError(db_);
        if (sqlite3_busy_timeout(db_, 2000) != SQLITE_OK) databaseError(db_);
        if (sqlite3_create_function_v2(db_, "ts_contains", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC,
                nullptr, sqliteContains, nullptr, nullptr, nullptr) != SQLITE_OK) databaseError(db_);
        Statement(db_, "PRAGMA foreign_keys=ON").run();
        Statement(db_, "PRAGMA journal_mode=WAL").run();
        Statement(db_, "PRAGMA synchronous=FULL").run();
        migrate(db_);
        Statement accounts(db_, "SELECT id FROM servers WHERE configured=1");
        while (accounts.row()) configured_.insert(accounts.integer(0));
        auto budget = setting("rolling_mib");
        budget_ = (budget.empty() ? defaultBudget : std::stoi(budget)) * MiB;
        offline_ = setting("offline_mode") == "1";
        reconcile();
        trimUnlocked(0);
    } catch (...) {
        closeDatabase();
        CloseHandle(lockFile_);
        throw;
    }
}

Store::~Store() {
    closeDatabase();
    CloseHandle(lockFile_);
}

void Store::closeDatabase() {
    for (const auto& entry : statements_) sqlite3_finalize(entry.second);
    statements_.clear();
    if (db_) sqlite3_close(db_);
}

Statement Store::prepare(const char* sql) {
    // Only query shapes are keys. All user input is bound separately.
    auto slot = statements_.lower_bound(sql);
    if (slot == statements_.end() || slot->first != sql)
        slot = statements_.emplace_hint(slot, sql, nullptr);
    return Statement(db_, sql, &slot->second);
}

std::string Store::setting(const char* key) {
    auto statement = prepare("SELECT value FROM settings WHERE name=?");
    statement.bind(std::string(key));
    return statement.row() ? statement.text(0) : "";
}

void Store::setSetting(const char* key, const std::string& value) {
    prepare("INSERT INTO settings VALUES(?,?) ON CONFLICT(name) DO UPDATE SET value=excluded.value")
        .bind(std::string(key)).bind(value).run();
}

std::vector<Server> Store::servers() {
    std::scoped_lock lock(mutex_);
    auto statement = prepare("SELECT id,name,url,user,native,cache_favorites,unmetered_format,unmetered_kbps,"
        "metered_format,metered_kbps,upgrade_favorites FROM servers WHERE configured=1 ORDER BY name,id");
    std::vector<Server> result;
    while (statement.row())
        result.push_back({statement.integer(0), statement.text(1), statement.text(2), statement.text(3), {},
                          statement.integer(4) != 0, statement.integer(5) != 0,
                          {statement.text(6), static_cast<int>(statement.integer(7))},
                          {statement.text(8), static_cast<int>(statement.integer(9))}, statement.integer(10) != 0});
    return result;
}

Server Store::loadServer(Id id) {
    auto statement = prepare("SELECT name,url,user,password,native,cache_favorites,unmetered_format,unmetered_kbps,"
        "metered_format,metered_kbps,upgrade_favorites FROM servers WHERE id=? AND configured=1");
    statement.bind(id);
    if (!statement.row()) throw Error("The selected server is unavailable.");
    return {id, statement.text(0), statement.text(1), statement.text(2), unprotect(statement.text(3)),
            statement.integer(4) != 0, statement.integer(5) != 0,
            {statement.text(6), static_cast<int>(statement.integer(7))},
            {statement.text(8), static_cast<int>(statement.integer(9))}, statement.integer(10) != 0};
}

Server Store::server(Id id) {
    std::scoped_lock lock(mutex_);
    return loadServer(id);
}

Id Store::saveProfile(const Server& server) {
    auto id = server.id;
    if (!id) {
        if (server.password.empty()) throw Error("Enter a password for the new account.");
        auto existing = prepare("SELECT id,configured FROM servers WHERE url=? AND user=?");
        existing.bind(server.url).bind(server.user);
        if (existing.row()) {
            if (existing.integer(1)) throw Error("This server and username already have a profile.");
            id = existing.integer(0);
        }
    }
    if (!id) {
        prepare("INSERT INTO servers(name,url,user,password,native,cache_favorites) VALUES(?,?,?,?,?,?)")
            .bind(server.name).bind(server.url).bind(server.user).bind(protect(server.password))
            .bind(server.native).bind(server.cacheFavorites).run();
        id = sqlite3_last_insert_rowid(db_);
    } else {
        auto saved = prepare("SELECT url FROM servers WHERE id=? AND user=? AND configured=?");
        saved.bind(id).bind(server.user).bind(server.id != 0);
        if (!saved.row() || !sameServerEndpoint(saved.text(0), server.url))
            throw Error("This server profile has changed. Reopen settings and try again.");
        if (saved.text(0) != server.url) {
            auto conflict = prepare("SELECT id FROM servers WHERE url=? AND user=? AND id<>?");
            conflict.bind(server.url).bind(server.user).bind(id);
            if (conflict.row()) throw Error("This address already has a profile for this username. Use Add server to reconnect it.");
        }
        prepare("UPDATE servers SET name=?,native=?,cache_favorites=?,configured=1,url=? WHERE id=?")
            .bind(server.name).bind(server.native).bind(server.cacheFavorites).bind(server.url).bind(id).run();
        if (!server.password.empty())
            prepare("UPDATE servers SET password=? WHERE id=?")
                .bind(protect(server.password)).bind(id).run();
    }
    prepare("UPDATE servers SET unmetered_format=?,unmetered_kbps=?,metered_format=?,metered_kbps=?,"
        "upgrade_favorites=? WHERE id=?").bind(server.unmetered.format).bind(server.unmetered.kbps)
        .bind(server.metered.format).bind(server.metered.kbps).bind(server.upgradeFavorites).bind(id).run();
    if (!server.cacheFavorites)
        prepare("UPDATE tracks SET favorite=0 WHERE server=? AND favorite=1").bind(id).run();
    return id;
}

bool Store::unifiedTree() {
    std::scoped_lock lock(mutex_);
    return setting("unified_tree") != "0";
}

void Store::saveProfiles(std::vector<Server>& profiles, int rollingMiB, bool unified,
                         const std::vector<Id>& removed) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    auto configured = configured_;
    for (Id id : removed) {
        prepare("UPDATE servers SET configured=0,password='' WHERE id=?").bind(id).run();
        configured.erase(id);
    }
    std::vector<Id> ids;
    for (const auto& profile : profiles) {
        ids.push_back(saveProfile(profile));
        configured.insert(ids.back());
    }
    setSetting("unified_tree", unified ? "1" : "0");
    setSetting("rolling_mib", std::to_string(rollingMiB));
    transaction.commit();
    configured_.swap(configured);
    for (Id id : removed) retryConnection_.erase(id);
    budget_ = rollingMiB * MiB;
    for (size_t i = 0; i < ids.size(); ++i) profiles[i].id = ids[i];
}

int Store::rollingMiB() {
    std::scoped_lock lock(mutex_);
    return static_cast<int>(budget_ / MiB);
}

std::string Store::playbackSecret() {
    std::scoped_lock lock(mutex_);
    return setting("playback_secret");
}

int Store::playbackPort() {
    std::scoped_lock lock(mutex_);
    auto port = setting("playback_port");
    return port.empty() ? 0 : std::stoi(port);
}

bool Store::textCatalog() {
    std::scoped_lock lock(mutex_);
    return setting("text_catalog") == "1";
}

SortTags Store::viewSettings(BrowseKind kind) {
    std::scoped_lock lock(mutex_);
    return decodeSortTags(setting(("library_view_" + std::to_string(static_cast<int>(kind))).c_str()));
}

void Store::saveViewSettings(BrowseKind kind, const SortTags& values) {
    std::scoped_lock lock(mutex_);
    setSetting(("library_view_" + std::to_string(static_cast<int>(kind))).c_str(), encodeSortTags(values));
}

int Store::coverSize() {
    std::scoped_lock lock(mutex_);
    auto size = prepare("SELECT value FROM settings WHERE name='cover_size'");
    if (!size.row()) return 0;
    const auto pixels = size.integer(0);
    return pixels >= 100 && pixels <= 600 ? static_cast<int>(pixels) : 0;
}

void Store::saveCoverSize(int size) {
    std::scoped_lock lock(mutex_);
    setSetting("cover_size", std::to_string(size));
}

bool Store::offlineMode() {
    std::scoped_lock lock(mutex_);
    return offline_;
}

std::string Store::browseOrder(BrowseKind kind) {
    std::scoped_lock lock(mutex_);
    return setting(("browse_order_" + std::to_string(static_cast<int>(kind))).c_str());
}

void Store::saveBrowseOrder(BrowseKind kind, const std::string& value) {
    std::scoped_lock lock(mutex_);
    setSetting(("browse_order_" + std::to_string(static_cast<int>(kind))).c_str(), value);
}

void Store::setOfflineMode(bool offline) {
    std::scoped_lock lock(mutex_);
    setSetting("offline_mode", offline ? "1" : "0");
    offline_ = offline;
    if (!offline) retryConnection_.clear();
}

bool Store::networkAvailable(Id server) {
    std::scoped_lock lock(mutex_);
    return !offline_ && configured_.count(server) && retryConnection_.count(server) == 0;
}

bool Store::connectionRetryDue(Id server) {
    std::scoped_lock lock(mutex_);
    auto retry = retryConnection_.find(server);
    return !offline_ && retry != retryConnection_.end() && retry->second <= std::chrono::steady_clock::now();
}

void Store::connectionFailed(Id server) {
    std::scoped_lock lock(mutex_);
    retryConnection_[server] = std::chrono::steady_clock::now() + std::chrono::seconds(15);
}

void Store::connectionRestored(Id server) {
    std::scoped_lock lock(mutex_);
    retryConnection_.erase(server);
}

Track Store::loadTrack(Id key, bool configuredOnly) {
    auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE key=?";
    auto statement = prepare(sql.c_str());
    statement.bind(key);
    if (!statement.row() || (configuredOnly && !configured_.count(statement.integer(1)))) return {};
    return getTrack(statement);
}

Track Store::track(Id key) {
    std::scoped_lock lock(mutex_);
    return loadTrack(key, true);
}

void Store::rememberUnlocked(std::vector<Track>& tracks) {
    for (auto& track : tracks) {
        prepare("INSERT INTO tracks(server,remote_id,title,artist,album,suffix,mime,duration,"
            "cover_id,album_id,artist_id,album_artist_id,album_artist,genre,disc_number,track_number,year,sort_tags)"
            " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(server,remote_id) DO UPDATE SET"
            " title=coalesce(nullif(excluded.title,''),tracks.title),"
            "artist=coalesce(nullif(excluded.artist,''),tracks.artist),"
            "album=coalesce(nullif(excluded.album,''),tracks.album),"
            "suffix=CASE WHEN tracks.bytes>0 AND tracks.cached_format='' THEN tracks.suffix ELSE coalesce(nullif(excluded.suffix,''),tracks.suffix) END,"
            "duration=coalesce(nullif(excluded.duration,0),tracks.duration),"
            "cover_id=coalesce(nullif(excluded.cover_id,''),tracks.cover_id),"
            "album_id=coalesce(nullif(excluded.album_id,''),tracks.album_id),"
            "artist_id=coalesce(nullif(excluded.artist_id,''),tracks.artist_id),"
            "album_artist_id=coalesce(nullif(excluded.album_artist_id,''),tracks.album_artist_id),"
            "album_artist=coalesce(nullif(excluded.album_artist,''),tracks.album_artist),"
            "genre=coalesce(nullif(excluded.genre,''),tracks.genre),"
            "disc_number=coalesce(nullif(excluded.disc_number,0),tracks.disc_number),"
            "track_number=coalesce(nullif(excluded.track_number,0),tracks.track_number),"
            "year=coalesce(nullif(excluded.year,0),tracks.year),"
            "sort_tags=coalesce(nullif(excluded.sort_tags,''),tracks.sort_tags),"
            "mime=CASE WHEN tracks.bytes>0 AND tracks.cached_format='' THEN tracks.mime ELSE coalesce(nullif(excluded.mime,''),tracks.mime) END")
            .bind(track.server).bind(track.id).bind(track.title).bind(track.artist)
            .bind(track.album).bind(track.suffix).bind(track.mime).bind(track.duration)
            .bind(track.coverId).bind(track.albumId).bind(track.artistId).bind(track.albumArtistId)
            .bind(track.albumArtist).bind(track.genre).bind(track.discNumber).bind(track.trackNumber)
            .bind(track.year).bind(encodeSortTags(track.sortTags)).run();
        static const auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=? AND remote_id=?";
        auto statement = prepare(sql.c_str());
        statement.bind(track.server).bind(track.id);
        if (statement.row()) track = getTrack(statement);
    }
}

void Store::rememberPage(Id server, const BrowseLocation& location, bool unfiltered, int offset,
                         BrowsePage& page) {
    std::scoped_lock lock(mutex_);
    if (!configured_.count(server)) return;
    Transaction transaction(db_);
    rememberUnlocked(page.relatedTracks);
    rememberUnlocked(page.tracks);
    if (location.kind != BrowseKind::tracks) rememberCatalog(server, location, unfiltered, offset, page);
    transaction.commit();
}

void Store::rememberCatalog(Id server, const BrowseLocation& location, bool unfiltered, int offset,
                            const BrowsePage& page) {
    auto parent = static_cast<int>(location.kind);
    if (unfiltered && offset == 0 && !page.more)
        prepare("DELETE FROM catalog WHERE server=? AND parent_kind=? AND parent_id=?")
            .bind(server).bind(parent).bind(location.id).run();
    auto save = [&](const LibraryItem& item, Id slot, Id key) {
        prepare("INSERT INTO catalog(server,parent_kind,parent_id,kind,id,slot,title,subtitle,cover_id,track_key,sort_tags)"
            " VALUES(?,?,?,?,?,?,?,?,?,?,?)"
            " ON CONFLICT(server,parent_kind,parent_id,kind,id,slot) DO UPDATE SET"
            " title=excluded.title,subtitle=excluded.subtitle,"
            "cover_id=coalesce(nullif(excluded.cover_id,''),catalog.cover_id),track_key=excluded.track_key,"
            "sort_tags=coalesce(nullif(excluded.sort_tags,''),catalog.sort_tags)")
            .bind(server).bind(parent).bind(location.id).bind(static_cast<int>(item.target.kind))
            .bind(item.target.id).bind(slot).bind(item.target.title).bind(item.subtitle).bind(item.coverId)
            .bind(key).bind(encodeSortTags(item.sortTags)).run();
    };
    auto saveTotals = [&](BrowseKind kind, const std::string& id, int songs, std::int64_t duration) {
        if (songs < 0 && duration <= 0) return;
        prepare("INSERT INTO catalog_totals VALUES(?,?,?,?,?) ON CONFLICT(server,kind,id) DO UPDATE SET"
            " songs=CASE WHEN excluded.songs>=0 THEN excluded.songs ELSE catalog_totals.songs END,"
            " duration=CASE WHEN excluded.songs=0 OR excluded.duration>0 THEN excluded.duration"
            " ELSE catalog_totals.duration END").bind(server).bind(static_cast<int>(kind)).bind(id)
            .bind(songs).bind(duration).run();
    };
    for (const auto& item : page.items) {
        save(item, 0, 0);
        saveTotals(item.target.kind, item.target.id, item.songCount, item.duration);
    }
    if (unfiltered && !offset && !page.more &&
        (location.kind == BrowseKind::albumTracks || location.kind == BrowseKind::playlistTracks)) {
        std::int64_t duration = 0;
        for (const auto& track : page.tracks) {
            if (track.duration <= 0) { duration = 0; break; }
            duration += track.duration;
        }
        saveTotals(location.kind, location.id, static_cast<int>(page.tracks.size()), duration);
    }
    if (location.kind == BrowseKind::playlistTracks) {
        Id position = offset;
        for (const auto& track : page.tracks)
            save({{BrowseKind::song, track.id, track.title}, track.artist, track.coverId}, position++, track.key);
    }
}

BrowsePage Store::cachedPage(Id server, const BrowseLocation& location, const std::string& query, int offset) {
    std::scoped_lock lock(mutex_);
    if (!configured_.count(server)) return {};
    const auto kind = location.kind;
    const auto filter = wide(query);
    BrowsePage result;
    if (trackView(kind)) {
        const bool playlist = kind == BrowseKind::playlistTracks;
        auto sql = std::string("SELECT ") + trackColumns + " FROM tracks";
        if (playlist) sql += " JOIN (SELECT track_key,slot FROM catalog WHERE server=? AND parent_kind=?"
            " AND parent_id=?) AS playlist ON track_key=key";
        sql += " WHERE server=?";
        if (kind == BrowseKind::offline) sql += " AND (bytes>0 OR pinned=1 OR favorite=1)";
        if (kind == BrowseKind::song) sql += " AND remote_id=?";
        if (kind == BrowseKind::albumTracks)
            sql += localLocation(location) ?
                " AND album_id='' AND album||char(31)||coalesce(nullif(album_artist,''),artist)=?" :
                " AND (album_id=? OR (album_id='' AND album=?))";
        const auto phrase = trigramPhrase(query);
        if (!phrase.empty()) {
            sql += " AND key IN (SELECT rowid FROM track_search(?) UNION ALL SELECT key FROM tracks"
                   " WHERE server=? AND (";
            sql += linguisticRows;
            sql += "))";
        }
        if (!query.empty()) sql += " AND (ts_contains(title,:needle) OR ts_contains(artist,:needle)"
                                   " OR ts_contains(album,:needle))";
        if (playlist) sql += " ORDER BY slot";
        else if (kind == BrowseKind::albumTracks) sql += " ORDER BY disc_number,track_number,title COLLATE NOCASE,key";
        else sql += " ORDER BY artist COLLATE NOCASE,album COLLATE NOCASE,title COLLATE NOCASE,key";
        sql += " LIMIT ? OFFSET ?";
        auto entries = prepare(sql.c_str());
        if (playlist) entries.bind(server).bind(static_cast<int>(kind)).bind(location.id);
        entries.bind(server);
        if (kind == BrowseKind::song || kind == BrowseKind::albumTracks)
            entries.bind(localLocation(location) ? location.id.substr(1) : location.id);
        if (kind == BrowseKind::albumTracks && !localLocation(location)) entries.bind(location.title);
        if (!phrase.empty()) entries.bind(phrase).bind(server);
        if (!query.empty()) entries.bind(query);
        entries.bind(pageSize + 1).bind(offset);
        while (entries.row()) {
            if (result.tracks.size() == pageSize) { result.more = true; break; }
            result.tracks.push_back(getTrack(entries));
        }
    } else {
        std::set<std::pair<BrowseKind, std::string>> seen;
        auto addItem = [&](LibraryItem item) {
            if ((!matches(item.target.title, filter) && !matches(item.subtitle, filter)) ||
                !seen.emplace(item.target.kind, item.target.id).second) return;
            if (offset > 0) --offset;
            else if (result.items.size() == pageSize) result.more = true;
            else result.items.push_back(std::move(item));
        };
        auto sql = std::string("SELECT kind,id,title,subtitle,cover_id,coalesce(songs,-1),coalesce(duration,0),sort_tags"
            " FROM catalog LEFT JOIN catalog_totals USING(server,kind,id) WHERE server=? AND ");
        if (kind == BrowseKind::albums) sql += "kind=?";
        else sql += "parent_kind=? AND parent_id=?";
        sql += " ORDER BY title COLLATE NOCASE,id,cover_id DESC";
        auto entries = prepare(sql.c_str());
        entries.bind(server).bind(static_cast<int>(kind == BrowseKind::albums ? BrowseKind::albumTracks : kind));
        if (kind != BrowseKind::albums) entries.bind(location.id);
        std::set<std::pair<std::string, std::string>> names;
        while (!result.more && entries.row()) {
            names.emplace(entries.text(2), entries.text(3));
            addItem({{static_cast<BrowseKind>(entries.integer(0)), entries.text(1), entries.text(2)},
                entries.text(3), entries.text(4), static_cast<int>(entries.integer(5)),
                entries.integer(6), decodeSortTags(entries.text(7))});
        }
        if (result.more) return result;
        if (kind == BrowseKind::genres) {
            auto genres = prepare("SELECT DISTINCT genre FROM tracks WHERE server=? AND genre<>'' ORDER BY genre");
            genres.bind(server);
            while (!result.more && genres.row())
                addItem({{BrowseKind::genreAlbums, genres.text(0), genres.text(0)}, {}, {}});
        }
        if (kind == BrowseKind::artists || kind == BrowseKind::albumArtists) {
            const char* name = kind == BrowseKind::artists ? "artist" : "coalesce(nullif(album_artist,''),artist)";
            const char* id = kind == BrowseKind::artists ? "artist_id" :
                "coalesce(nullif(album_artist_id,''),CASE WHEN album_artist='' OR album_artist=artist THEN artist_id ELSE '' END)";
            auto artistSql = std::string("SELECT ") + id + "," + name +
                " FROM tracks WHERE server=? GROUP BY 1,2 ORDER BY 2 COLLATE NOCASE,1 DESC";
            auto artists = prepare(artistSql.c_str());
            artists.bind(server);
            std::set<std::string> represented;
            for (const auto& namePair : names) represented.insert(namePair.first);
            while (!result.more && artists.row()) {
                auto artist = artists.text(1);
                auto artistId = artists.text(0);
                if (artist.empty() || (artistId.empty() && represented.count(artist))) continue;
                represented.insert(artist);
                if (artistId.empty()) artistId = '\x1f' + artist;
                addItem({{BrowseKind::artistAlbums, std::move(artistId), std::move(artist)}, {}, {}});
            }
        }
        if (kind == BrowseKind::albums || kind == BrowseKind::genreAlbums || kind == BrowseKind::artistAlbums) {
            auto albumSql = std::string("SELECT album_id,album,min(coalesce(nullif(album_artist,''),artist)),"
                "max(cover_id),count(*),CASE WHEN min(duration)>0 THEN sum(duration) ELSE 0 END"
                " FROM tracks WHERE server=? AND album<>''");
            if (kind == BrowseKind::genreAlbums) albumSql += " AND genre=?";
            if (kind == BrowseKind::artistAlbums) albumSql += localLocation(location) ?
                " AND (artist=? OR album_artist=?)" : " AND (artist_id=? OR album_artist_id=?"
                " OR (artist_id='' AND artist=?) OR (album_artist_id='' AND album_artist=?))";
            albumSql += " GROUP BY 1,2,CASE WHEN album_id='' THEN coalesce(nullif(album_artist,''),artist)"
                " ELSE '' END ORDER BY 2 COLLATE NOCASE,1 DESC";
            auto albums = prepare(albumSql.c_str());
            albums.bind(server);
            if (kind == BrowseKind::genreAlbums) albums.bind(location.id);
            if (kind == BrowseKind::artistAlbums) {
                const auto id = localLocation(location) ? location.id.substr(1) : location.id;
                albums.bind(id).bind(id);
                if (!localLocation(location)) albums.bind(location.title).bind(location.title);
            }
            while (!result.more && albums.row()) {
                auto album = albums.text(1);
                auto albumArtist = albums.text(2);
                auto albumId = albums.text(0);
                if (albumId.empty() && names.count({album, albumArtist})) continue;
                names.emplace(album, albumArtist);
                if (albumId.empty()) albumId = '\x1f' + album + '\x1f' + albumArtist;
                addItem({{BrowseKind::albumTracks, std::move(albumId), std::move(album)},
                    std::move(albumArtist), albums.text(3), static_cast<int>(albums.integer(4)),
                    albums.integer(5)});
            }
        }
    }
    return result;
}

std::string Store::albumCover(Id server, const std::string& albumId) {
    if (albumId.empty()) return {};
    std::scoped_lock lock(mutex_);
    auto cover = prepare("SELECT cover_id FROM catalog WHERE server=? AND kind=? AND id=?"
        " AND cover_id<>'' LIMIT 1");
    cover.bind(server).bind(static_cast<int>(BrowseKind::albumTracks)).bind(albumId);
    return cover.row() ? cover.text(0) : "";
}

namespace {

std::string artworkKey(const LibraryItem& item) {
    if (!item.coverId.empty() && item.target.kind != BrowseKind::artistAlbums) return "cover:" + item.coverId;
    return std::to_string(static_cast<int>(item.target.kind)) + ":" + item.target.id;
}

} // namespace

CachedArtwork Store::artwork(Id server, const LibraryItem& item) {
    std::scoped_lock lock(mutex_);
    auto key = artworkKey(item);
    auto artwork = prepare("SELECT data,requested_max_edge_px,cached_longest_edge_px FROM artwork WHERE server=? AND cover_key=?");
    artwork.bind(server).bind(key);
    if (!artwork.row()) return {};
    CachedArtwork cached{artwork.blob(0), static_cast<int>(artwork.integer(1)), static_cast<int>(artwork.integer(2))};
    prepare("UPDATE artwork SET last_use=? WHERE server=? AND cover_key=?")
        .bind(nowMs()).bind(server).bind(key).run();
    return cached;
}

void Store::activeCover(Id key) {
    std::scoped_lock lock(mutex_);
    activeCover_ = key;
}

// WACUP can look for folder art as soon as a marker enters the playlist.
// Keep these native-readable copies bounded independently of the database blobs.
bool Store::stageCover(const Track& track, const LibraryItem& song, const LibraryItem& album) {
    std::scoped_lock lock(mutex_);
    auto directory = cache_.parent_path() / L"items" / std::to_wstring(track.key);
    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec)) return false;
    const auto primary = artworkKey(song);
    auto match = prepare("SELECT cover_key,length(data) FROM artwork WHERE server=? AND cover_key IN (?,?)"
                         " ORDER BY cover_key=? DESC LIMIT 1");
    match.bind(track.server).bind(primary).bind(album.target.id.empty() ? primary : artworkKey(album)).bind(primary);
    if (!match.row()) return false;
    const auto key = match.text(0);
    const auto bytes = match.integer(1);
    auto pathFor = [&](Id id, const std::string& ext) {
        return cache_.parent_path() / L"items" / std::to_wstring(id) / (L"cover." + wide(ext));
    };
    auto validExt = [](const std::string& ext) {
        return ext == "jpg" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "webp";
    };
    std::string oldExt;
    {
        auto old = prepare("SELECT cover_key,ext,bytes,last_use FROM playlist_covers WHERE track=?");
        old.bind(track.key);
        if (old.row()) {
            oldExt = old.text(1);
            if (!validExt(oldExt)) oldExt.clear();
            if (!oldExt.empty() && old.text(0) == key && old.integer(2) == bytes &&
                std::filesystem::file_size(pathFor(track.key, oldExt), ec) == static_cast<uintmax_t>(bytes) && !ec) {
                // Avoid journaling on every metadata query during playback.
                if (old.integer(3) < nowMs() - 60000)
                    prepare("UPDATE playlist_covers SET last_use=? WHERE track=?")
                        .bind(nowMs()).bind(track.key).run();
                return false;
            }
        }
    }
    auto payload = prepare("SELECT data FROM artwork WHERE server=? AND cover_key=?");
    payload.bind(track.server).bind(key);
    if (!payload.row()) return false;
    auto data = payload.blob(0);
    auto ext = artworkExtension(data);
    if (!ext) return false;
    auto path = directory / (std::wstring(L"cover.") + ext);
    auto tmp = directory / L"cover.tmp";
    std::ofstream output(tmp, std::ios::binary | std::ios::trunc);
    output.write(data.data(), data.size());
    output.close();
    if (!output || !MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmp.c_str());
        return false;
    }
    if (!oldExt.empty() && wide(oldExt) != ext) DeleteFileW(pathFor(track.key, oldExt).c_str());
    Transaction transaction(db_);
    prepare("INSERT OR REPLACE INTO playlist_covers VALUES(?,?,?,?,?)")
        .bind(track.key).bind(key).bind(utf8(ext)).bind(bytes).bind(nowMs()).run();

    auto overflow = prepare("SELECT track,ext FROM (SELECT track,ext,"
        "sum(bytes) OVER (ORDER BY (track=? OR track=?) DESC,last_use DESC,track) AS total "
        "FROM playlist_covers) WHERE total>?");
    overflow.bind(activeCover_).bind(track.key).bind(24 * MiB);
    std::vector<std::pair<Id, std::string>> evict;
    while (overflow.row()) evict.emplace_back(overflow.integer(0), overflow.text(1));
    for (const auto& [id, extension] : evict) {
        if (validExt(extension)) {
            auto old = pathFor(id, extension);
            DeleteFileW((old.parent_path() / L"cover.tmp").c_str());
            if (!DeleteFileW(old.c_str())) {
                const auto error = GetLastError();
                if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) continue;
            }
        }
        prepare("DELETE FROM playlist_covers WHERE track=?").bind(id).run();
    }
    transaction.commit();
    return true;
}

void Store::recordCachedCoverEdge(Id server, const LibraryItem& item, int requestedMaxEdgePx, int cachedLongestEdgePx) {
    std::scoped_lock lock(mutex_);
    prepare("UPDATE artwork SET cached_longest_edge_px=? WHERE server=? AND cover_key=? AND requested_max_edge_px=? AND cached_longest_edge_px=0")
        .bind(cachedLongestEdgePx).bind(server).bind(artworkKey(item)).bind(requestedMaxEdgePx).run();
}

std::string Store::saveArtwork(Id server, const LibraryItem& item, int requestedMaxEdgePx, int cachedLongestEdgePx, std::string data) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    const auto key = artworkKey(item);
    std::int64_t previousBytes = 0;
    {
        auto old = prepare("SELECT requested_max_edge_px,cached_longest_edge_px,length(data) FROM artwork WHERE server=? AND cover_key=?");
        old.bind(server).bind(key);
        if (old.row()) {
            const int previousRequestedMaxEdgePx = static_cast<int>(old.integer(0));
            const int previousCachedEdgePx = static_cast<int>(old.integer(1));
            previousBytes = old.integer(2);
            const bool keepCachedCover = (previousCachedEdgePx || cachedLongestEdgePx)
                ? previousCachedEdgePx >= cachedLongestEdgePx : previousRequestedMaxEdgePx >= requestedMaxEdgePx;
            if (!cachedLongestEdgePx && previousCachedEdgePx) requestedMaxEdgePx = previousRequestedMaxEdgePx;
            requestedMaxEdgePx = std::max(requestedMaxEdgePx, previousRequestedMaxEdgePx);
            if (keepCachedCover) {
                auto kept = prepare("UPDATE artwork SET requested_max_edge_px=?,last_use=? WHERE server=? AND cover_key=? RETURNING data");
                kept.bind(requestedMaxEdgePx).bind(nowMs()).bind(server).bind(key);
                kept.row();
                data = kept.blob(0);
                kept.run();
                transaction.commit();
                return data;
            }
        }
    }
    prepare("INSERT INTO artwork(server,cover_key,data,last_use,requested_max_edge_px,cached_longest_edge_px) VALUES(?,?,?,?,?,?)"
        " ON CONFLICT(server,cover_key) DO UPDATE SET data=excluded.data,last_use=excluded.last_use,"
        " requested_max_edge_px=excluded.requested_max_edge_px,cached_longest_edge_px=excluded.cached_longest_edge_px")
        .bind(server).bind(key).bindBlob(data).bind(nowMs()).bind(requestedMaxEdgePx).bind(cachedLongestEdgePx).run();
    prepare("UPDATE playlist_covers SET cover_key='' WHERE cover_key=? AND track IN"
        " (SELECT key FROM tracks WHERE server=?)").bind(key).bind(server).run();
    std::int64_t bytes = 0;
    if (artworkBytes_) {
        bytes = *artworkBytes_ - previousBytes + static_cast<std::int64_t>(data.size());
    } else {
        auto total = prepare("SELECT coalesce(sum(length(data)),0) FROM artwork");
        total.row();
        bytes = total.integer(0);
    }
    while (bytes > artworkBudget) {
        auto oldest = prepare("DELETE FROM artwork WHERE (server,cover_key)=(SELECT server,cover_key FROM artwork"
            " ORDER BY last_use,server DESC,cover_key DESC LIMIT 1) RETURNING length(data)");
        if (!oldest.row()) throw Error("Could not evict the oldest cover from the artwork cache.");
        bytes -= oldest.integer(0);
        oldest.run();
    }
    transaction.commit();
    artworkBytes_ = bytes;
    return data;
}

std::optional<Lyrics> Store::lyrics(Id key, bool fresh) {
    std::scoped_lock lock(mutex_);
    auto lyrics = prepare("SELECT plain,structured,checked FROM lyrics WHERE track=?");
    lyrics.bind(key);
    if (!lyrics.row()) return {};
    Lyrics result{lyrics.text(0), lyrics.text(1)};
    if (fresh && result.plain.empty() && lyrics.integer(2) < nowMs() - 86'400'000) return {};
    return result;
}

void Store::saveLyrics(Id key, const Lyrics& lyrics) {
    std::scoped_lock lock(mutex_);
    prepare("INSERT INTO lyrics VALUES(?,?,?,?) ON CONFLICT(track) DO UPDATE SET"
        " plain=excluded.plain,structured=excluded.structured,checked=excluded.checked")
        .bind(key).bind(lyrics.plain).bind(lyrics.structured).bind(nowMs()).run();
}

void Store::pin(const std::vector<Track>& tracks, bool permanent) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    for (const auto& track : tracks)
        prepare("UPDATE tracks SET pinned=?,retry_at=0 WHERE key=?").bind(permanent).bind(track.key).run();
    transaction.commit();
}

void Store::syncFavorites(Id server, BrowsePage& page) {
    std::scoped_lock lock(mutex_);
    if (!configured_.count(server)) return;
    auto enabled = prepare("SELECT id FROM servers WHERE id=? AND cache_favorites=1");
    enabled.bind(server);
    if (!enabled.row()) return;
    Transaction transaction(db_);
    rememberUnlocked(page.tracks);
    rememberCatalog(server, {BrowseKind::favorites, {}, {}}, true, 0, page);
    prepare("UPDATE tracks SET favorite=0 WHERE server=? AND favorite=1").bind(server).run();
    for (const auto& track : page.tracks)
        prepare("UPDATE tracks SET favorite=1 WHERE key=?").bind(track.key).run();
    transaction.commit();
}

std::vector<Track> Store::pendingPins(Id server, bool upgradeFavorites, const InputExtensions& inputs) {
    std::string playable = "|";
    if (upgradeFavorites) {
        for (const auto& extension : inputs) playable += extension + "|";
    }
    std::scoped_lock lock(mutex_);
    if (!configured_.count(server)) return {};
    auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=?"
        " AND (bytes=0 OR (favorite=1 AND cached_format<>'' AND instr(?, '|' || lower(suffix) || '|')>0))"
        " AND (pinned=1 OR favorite=1) AND retry_at<=? ORDER BY pinned DESC,key LIMIT 4";
    auto statement = prepare(sql.c_str());
    statement.bind(server).bind(playable).bind(nowMs());
    std::vector<Track> result;
    while (statement.row()) result.push_back(getTrack(statement));
    return result;
}

void Store::postponePin(Id key) {
    std::scoped_lock lock(mutex_);
    prepare("UPDATE tracks SET retry_at=? WHERE key=?").bind(nowMs() + 60'000).bind(key).run();
}

std::filesystem::path Store::audioPath(const Track& track) const {
    auto suffix = track.cachedFormat.empty() ? track.suffix : track.cachedFormat;
    if (suffix.empty() || suffix.size() > 12 ||
        suffix.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != suffix.npos)
        suffix = "mp3";
    return cache_ / (std::to_wstring(track.key) + (track.cachedFormat.empty() ? L".media." : L".encoded.") + wide(suffix));
}

HANDLE Store::openCached(Id key, std::filesystem::path& path) {
    std::scoped_lock lock(mutex_);
    auto track = loadTrack(key, true);
    if (!track.bytes) return INVALID_HANDLE_VALUE;
    path = audioPath(track);
    auto file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    return file;
}

std::filesystem::path Store::partialPath(Id key) const { return cache_ / (std::to_wstring(key) + L".part"); }

void Store::reconcile() {
    auto saved = prepare("SELECT key,bytes FROM tracks WHERE bytes>0");
    std::vector<Id> missing;
    std::map<Id, std::filesystem::path> files;
    while (saved.row()) {
        const auto key = saved.integer(0);
        const auto bytes = static_cast<std::uintmax_t>(saved.integer(1));
        std::error_code error;
        auto track = loadTrack(key);
        auto path = audioPath(track);
        auto legacy = cache_ / (std::to_wstring(key) + L".audio");
        if (track.cachedFormat.empty() && !std::filesystem::exists(path) && std::filesystem::exists(legacy))
            std::filesystem::rename(legacy, path);
        if (std::filesystem::file_size(path, error) != bytes || error) missing.push_back(key);
        else files.emplace(key, path);
    }
    for (const auto& entry : std::filesystem::directory_iterator(cache_)) {
        auto extension = entry.path().extension();
        auto stem = entry.path().stem().string();
        if (stem.size() == 37 && (stem.compare(0, 5, "play-") == 0 || stem.compare(0, 5, "read-") == 0) &&
            stem.find_first_not_of("0123456789abcdef", 5) == std::string::npos) {
            DeleteFileW(entry.path().c_str());
            continue;
        }
        const bool media = stem.size() > 6 && stem.compare(stem.size()-6, 6, ".media") == 0;
        const bool encoded = stem.size() > 8 && stem.compare(stem.size()-8, 8, ".encoded") == 0;
        if (media) stem.resize(stem.size()-6);
        if (encoded) stem.resize(stem.size()-8);
        Id key;
        auto parsed = std::from_chars(stem.data(), stem.data() + stem.size(), key);
        if (parsed.ec != std::errc{} || parsed.ptr != stem.data() + stem.size()) continue;
        if (extension == L".part" || ((media || encoded || extension == L".audio") &&
            (!files.count(key) || files.at(key) != entry.path()))) {
            std::error_code error;
            std::filesystem::remove(entry.path(), error);
        }
    }
    for (Id key : missing) prepare("UPDATE tracks SET bytes=0,cached_format='' WHERE key=?").bind(key).run();
}

bool Store::trimUnlocked(std::int64_t incoming) {
    for (const auto& [key, bytes] : downloads_) {
        auto track = loadTrack(key);
        if (!track.pinned && !track.favorite) incoming += bytes;
    }
    if (incoming > budget_) return false;
    auto total = prepare("SELECT COALESCE(SUM(bytes),0) FROM tracks WHERE pinned=0 AND favorite=0");
    total.row();
    auto used = total.integer(0);
    total.run();
    if (used <= budget_ - incoming) return true;
    auto oldest = prepare("SELECT key,bytes FROM tracks WHERE bytes>0 AND pinned=0 AND favorite=0 ORDER BY last_use,key");
    std::vector<std::pair<Id, std::int64_t>> candidates;
    while (oldest.row()) candidates.emplace_back(oldest.integer(0), oldest.integer(1));
    for (const auto& [key, bytes] : candidates) {
        auto path = audioPath(loadTrack(key));
        if (!DeleteFileW(path.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) continue;
        prepare("UPDATE tracks SET bytes=0,cached_format='' WHERE key=?").bind(key).run();
        used -= bytes;
        if (used <= budget_ - incoming) return true;
    }
    return false;
}

bool Store::reserve(Id key, std::int64_t bytes, bool upgrade) {
    std::scoped_lock lock(mutex_);
    auto track = loadTrack(key, true);
    if (!track.key || downloads_.count(key)) return false;
    if (track.bytes > 0 && !(upgrade && track.favorite && !track.cachedFormat.empty())) return false;
    if (!track.pinned && !track.favorite && (!budget_ || !trimUnlocked(std::max<std::int64_t>(bytes, 0)))) return false;
    downloads_.emplace(key, bytes > 0 ? bytes : 0);
    return true;
}

bool Store::growReservation(Id key, std::int64_t bytes) {
    std::scoped_lock lock(mutex_);
    auto& reserved = downloads_.at(key);
    if (bytes <= reserved) return true;
    const auto track = loadTrack(key, true);
    if (!track.key) return false;
    if (!track.pinned && !track.favorite && !trimUnlocked(bytes - reserved)) return false;
    reserved = bytes;
    return true;
}

bool Store::publish(Id key, std::int64_t bytes, const std::string& format, std::filesystem::path& file,
                    HANDLE* playbackLease) {
    std::scoped_lock lock(mutex_);
    downloads_.at(key) = 0;
    auto track = loadTrack(key, true);
    if (!track.key || bytes <= 0 || (!track.pinned && !track.favorite && !trimUnlocked(bytes))) return false;
    if (track.bytes) {
        if (!track.favorite || track.cachedFormat.empty() || !format.empty()) return false;
        auto enabled = prepare("SELECT 1 FROM servers WHERE id=? AND cache_favorites=1 AND upgrade_favorites=1");
        if (!enabled.bind(track.server).row()) return false;
    }
    const auto previous = track.bytes ? audioPath(track) : std::filesystem::path{};
    track.cachedFormat = format;
    const auto target = audioPath(track);
    if (!MoveFileExW(file.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        if (playbackLease) return false; // The staged file is still playable.
        throw Error("The completed audio file could not be saved.");
    }
    try {
        prepare("UPDATE tracks SET bytes=?,last_use=?,cached_format=?,retry_at=0 WHERE key=?")
            .bind(bytes).bind(nowMs()).bind(format).bind(key).run();
    } catch (...) {
        DeleteFileW(audioPath(track).c_str());
        throw;
    }
    if (!previous.empty()) DeleteFileW(previous.c_str());
    file = target;
    // Hold the new track open before another thread can evict it.
    if (playbackLease)
        *playbackLease = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    return true;
}

void Store::release(Id key) {
    std::scoped_lock lock(mutex_);
    DeleteFileW(partialPath(key).c_str());
    downloads_.erase(key);
}

void Store::touch(Id key) {
    std::scoped_lock lock(mutex_);
    prepare("UPDATE tracks SET last_use=? WHERE key=?").bind(nowMs()).bind(key).run();
}

void Store::trim() {
    std::scoped_lock lock(mutex_);
    trimUnlocked(0);
}

void Store::enqueueListen(const Track& track, std::int64_t listenedAtMs) {
    std::scoped_lock lock(mutex_);
    if (configured_.count(track.server))
        prepare("INSERT INTO pending_listens(server,remote_id,listened_at_ms) VALUES(?,?,?)")
            .bind(track.server).bind(track.id).bind(listenedAtMs).run();
}

std::vector<PendingListen> Store::nextListenBatch(Id server) {
    std::scoped_lock lock(mutex_);
    if (!configured_.count(server)) return {};
    auto statement = prepare("SELECT key,remote_id,listened_at_ms FROM pending_listens WHERE server=? ORDER BY key LIMIT 25");
    statement.bind(server);
    std::vector<PendingListen> result;
    std::size_t querySize = 0;
    while (statement.row()) {
        auto id = statement.text(1);
        querySize += id.size() * 3 + 32;
        if (querySize > 4800 && !result.empty()) break;
        result.push_back({statement.integer(0), std::move(id), statement.integer(2)});
    }
    return result;
}

void Store::acknowledgeListens(const std::vector<PendingListen>& listens) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    for (const auto& listen : listens)
        prepare("DELETE FROM pending_listens WHERE key=?").bind(listen.queueKey).run();
    transaction.commit();
}

std::int64_t Store::pendingListenCount(Id server) {
    std::scoped_lock lock(mutex_);
    auto statement = prepare("SELECT COUNT(*) FROM pending_listens WHERE server=?");
    statement.bind(server);
    return statement.row() ? statement.integer(0) : 0;
}

} // namespace tangerine
