#include "store.hpp"

#include <wincrypt.h>
#include <algorithm>
#include <charconv>
#include <set>
#include <fstream>
#include <system_error>

// The plugin uses the SQLite C ABI exported by Windows 10 and later.
struct sqlite3_stmt;
extern "C" {
int WINAPI sqlite3_open_v2(const char*, sqlite3**, int, const char*);
int WINAPI sqlite3_close(sqlite3*);
int WINAPI sqlite3_busy_timeout(sqlite3*, int);
const char* WINAPI sqlite3_errmsg(sqlite3*);
int WINAPI sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
int WINAPI sqlite3_finalize(sqlite3_stmt*);
int WINAPI sqlite3_bind_int64(sqlite3_stmt*, int, std::int64_t);
int WINAPI sqlite3_bind_text(sqlite3_stmt*, int, const char*, int, void (*)(void*));
int WINAPI sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int, void (*)(void*));
int WINAPI sqlite3_step(sqlite3_stmt*);
std::int64_t WINAPI sqlite3_column_int64(sqlite3_stmt*, int);
const unsigned char* WINAPI sqlite3_column_text(sqlite3_stmt*, int);
const void* WINAPI sqlite3_column_blob(sqlite3_stmt*, int);
int WINAPI sqlite3_column_bytes(sqlite3_stmt*, int);
std::int64_t WINAPI sqlite3_last_insert_rowid(sqlite3*);
}

namespace tangerine {
namespace {

constexpr int sqliteOk = 0, sqliteRow = 100, sqliteDone = 101;
constexpr int sqliteReadWriteCreate = 0x02 | 0x04;
constexpr int defaultBudget = 1024;

[[noreturn]] void databaseError(sqlite3* db) {
    throw Error(std::string("Tangerine Stream storage: ") + (db ? sqlite3_errmsg(db) : "out of memory"));
}

class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &statement_, nullptr) != sqliteOk) databaseError(db_);
    }
    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement& bind(std::int64_t value) {
        if (sqlite3_bind_int64(statement_, next_++, value) != sqliteOk) databaseError(db_);
        return *this;
    }
    Statement& bind(const std::string& value) {
        // SQLite copies the value before bind returns (SQLITE_TRANSIENT).
        if (sqlite3_bind_text(statement_, next_++, value.c_str(), static_cast<int>(value.size()),
                              copy) != sqliteOk) databaseError(db_);
        return *this;
    }
    Statement& bindBlob(const std::string& value) {
        if (sqlite3_bind_blob(statement_, next_++, value.data(), static_cast<int>(value.size()),
                              copy) != sqliteOk) databaseError(db_);
        return *this;
    }
    bool row() {
        auto result = sqlite3_step(statement_);
        if (result == sqliteRow) return true;
        if (result == sqliteDone) return false;
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
    inline static const auto copy = reinterpret_cast<void (*)(void*)>(static_cast<std::intptr_t>(-1));
    sqlite3* db_;
    sqlite3_stmt* statement_ = nullptr;
    int next_ = 1;
};

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

constexpr char trackColumns[] =
    "key,server,remote_id,title,artist,album,suffix,mime,duration,pinned,favorite,bytes,"
    "cover_id,album_id,artist_id,album_artist_id,album_artist,genre,disc_number,track_number,year";

Track getTrack(const Statement& statement) {
    return {statement.integer(0), statement.integer(1), statement.text(2), statement.text(3),
            statement.text(4), statement.text(5), statement.text(6), statement.text(7),
            static_cast<int>(statement.integer(8)), statement.integer(9) != 0,
            statement.integer(10) != 0, statement.integer(11), statement.text(12), statement.text(13),
            statement.text(14), statement.text(15), statement.text(16), statement.text(17),
            static_cast<int>(statement.integer(18)), static_cast<int>(statement.integer(19)),
            static_cast<int>(statement.integer(20))};
}

std::string protect(std::string password) {
    DATA_BLOB input{static_cast<DWORD>(password.size()), reinterpret_cast<BYTE*>(password.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Tangerine Stream", nullptr, nullptr, nullptr,
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
        throw Error("The Tangerine Stream profile could not be opened exclusively.");
    try {
        if (sqlite3_open_v2(utf8((directory / L"library.sqlite3").wstring()).c_str(), &db_,
                           sqliteReadWriteCreate, nullptr) != sqliteOk) databaseError(db_);
        if (sqlite3_busy_timeout(db_, 2000) != sqliteOk) databaseError(db_);
        Statement(db_, "PRAGMA foreign_keys=ON").run();
        Statement(db_, "PRAGMA journal_mode=WAL").run();
        Statement(db_, "PRAGMA synchronous=FULL").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS settings (name TEXT PRIMARY KEY,value TEXT NOT NULL)").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS servers (id INTEGER PRIMARY KEY,name TEXT NOT NULL,"
            "url TEXT NOT NULL,user TEXT NOT NULL,password TEXT NOT NULL,native INTEGER NOT NULL,"
            "cache_favorites INTEGER NOT NULL,UNIQUE(url,user))").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS tracks (key INTEGER PRIMARY KEY,"
            "server INTEGER NOT NULL REFERENCES servers(id),remote_id TEXT NOT NULL,title TEXT NOT NULL,"
            "artist TEXT NOT NULL,album TEXT NOT NULL,suffix TEXT NOT NULL,mime TEXT NOT NULL,"
            "duration INTEGER NOT NULL,pinned INTEGER NOT NULL DEFAULT 0,favorite INTEGER NOT NULL DEFAULT 0,"
            "bytes INTEGER NOT NULL DEFAULT 0,last_use INTEGER NOT NULL DEFAULT 0,"
            "retry_at INTEGER NOT NULL DEFAULT 0,UNIQUE(server,remote_id))").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS reports (key INTEGER PRIMARY KEY,"
            "server INTEGER NOT NULL REFERENCES servers(id),remote_id TEXT NOT NULL,time INTEGER NOT NULL)").run();
        Statement(db_, "CREATE INDEX IF NOT EXISTS reports_server ON reports(server,key)").run();
        Transaction migration(db_);
        Statement version(db_, "PRAGMA user_version");
        if (version.row() && version.integer(0) == 0) {
            version.run();
            for (const auto* column : {"cover_id", "album_id", "artist_id", "album_artist_id",
                                      "album_artist", "genre"}) {
                auto sql = std::string("ALTER TABLE tracks ADD COLUMN ") + column + " TEXT NOT NULL DEFAULT ''";
                Statement(db_, sql.c_str()).run();
            }
            for (const auto* column : {"disc_number", "track_number", "year"}) {
                auto sql = std::string("ALTER TABLE tracks ADD COLUMN ") + column + " INTEGER NOT NULL DEFAULT 0";
                Statement(db_, sql.c_str()).run();
            }
            Statement(db_, "PRAGMA user_version=1").run();
        }
        Statement(db_, "CREATE INDEX IF NOT EXISTS tracks_album ON tracks(server,album_id)").run();
        Statement(db_, "CREATE INDEX IF NOT EXISTS tracks_artist ON tracks(server,artist_id)").run();
        Statement(db_, "CREATE INDEX IF NOT EXISTS tracks_album_artist ON tracks(server,album_artist_id)").run();
        Statement(db_, "CREATE INDEX IF NOT EXISTS tracks_genre ON tracks(server,genre)").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS catalog (server INTEGER NOT NULL REFERENCES servers(id),"
            "parent_kind INTEGER NOT NULL,parent_id TEXT NOT NULL,kind INTEGER NOT NULL,id TEXT NOT NULL,"
            "slot INTEGER NOT NULL,title TEXT NOT NULL,subtitle TEXT NOT NULL,cover_id TEXT NOT NULL,"
            "track_key INTEGER NOT NULL,PRIMARY KEY(server,parent_kind,parent_id,kind,id,slot)) WITHOUT ROWID").run();
        Statement(db_, "CREATE INDEX IF NOT EXISTS catalog_objects ON catalog(server,kind,id)").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS catalog_totals (server INTEGER NOT NULL REFERENCES servers(id),"
            "kind INTEGER NOT NULL,id TEXT NOT NULL,songs INTEGER NOT NULL,duration INTEGER NOT NULL,"
            "PRIMARY KEY(server,kind,id)) WITHOUT ROWID").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS artwork (server INTEGER NOT NULL REFERENCES servers(id),"
            "cover_key TEXT NOT NULL,data BLOB NOT NULL,last_use INTEGER NOT NULL,"
            "PRIMARY KEY(server,cover_key)) WITHOUT ROWID").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS lyrics (track INTEGER PRIMARY KEY REFERENCES tracks(key),"
            "plain TEXT NOT NULL,structured TEXT NOT NULL,checked INTEGER NOT NULL)").run();
        Statement(db_, "CREATE TABLE IF NOT EXISTS playlist_covers (track INTEGER PRIMARY KEY,"
            "cover_key TEXT NOT NULL,ext TEXT NOT NULL,bytes INTEGER NOT NULL,last_use INTEGER NOT NULL)").run();
        migration.commit();
        auto budget = setting("rolling_mib");
        budget_ = (budget.empty() ? defaultBudget : std::stoi(budget)) * MiB;
        offline_ = setting("offline_mode") == "1";
        reconcile();
        trimUnlocked(0);
    } catch (...) {
        if (db_) sqlite3_close(db_);
        CloseHandle(lockFile_);
        throw;
    }
}

Store::~Store() {
    sqlite3_close(db_);
    CloseHandle(lockFile_);
}

std::string Store::setting(const char* key) {
    Statement statement(db_, "SELECT value FROM settings WHERE name=?");
    statement.bind(std::string(key));
    return statement.row() ? statement.text(0) : "";
}

void Store::setSetting(const char* key, const std::string& value) {
    Statement(db_, "INSERT INTO settings VALUES(?,?) ON CONFLICT(name) DO UPDATE SET value=excluded.value")
        .bind(std::string(key)).bind(value).run();
}

std::vector<Server> Store::servers() {
    std::scoped_lock lock(mutex_);
    Statement statement(db_, "SELECT id,name,url,user,native,cache_favorites FROM servers ORDER BY name,id");
    std::vector<Server> result;
    while (statement.row())
        result.push_back({statement.integer(0), statement.text(1), statement.text(2), statement.text(3), {},
                          statement.integer(4) != 0, statement.integer(5) != 0});
    return result;
}

Server Store::loadServer(Id id) {
    Statement statement(db_, "SELECT name,url,user,password,native,cache_favorites FROM servers WHERE id=?");
    statement.bind(id);
    if (!statement.row()) throw Error("The selected server is unavailable.");
    return {id, statement.text(0), statement.text(1), statement.text(2), unprotect(statement.text(3)),
            statement.integer(4) != 0, statement.integer(5) != 0};
}

Server Store::server(Id id) {
    std::scoped_lock lock(mutex_);
    return loadServer(id);
}

void Store::save(Server& server, int rollingMiB) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    auto id = server.id;
    if (!id) {
        Statement duplicate(db_, "SELECT id FROM servers WHERE url=? AND user=?");
        duplicate.bind(server.url).bind(server.user);
        if (duplicate.row()) throw Error("This server and username already have a profile.");
        Statement(db_, "INSERT INTO servers(name,url,user,password,native,cache_favorites) VALUES(?,?,?,?,?,?)")
            .bind(server.name).bind(server.url).bind(server.user).bind(protect(server.password))
            .bind(server.native).bind(server.cacheFavorites).run();
        id = sqlite3_last_insert_rowid(db_);
    } else {
        Statement(db_, "UPDATE servers SET name=?,native=?,cache_favorites=? WHERE id=?")
            .bind(server.name).bind(server.native).bind(server.cacheFavorites).bind(id).run();
        if (!server.password.empty())
            Statement(db_, "UPDATE servers SET password=? WHERE id=?")
                .bind(protect(server.password)).bind(id).run();
    }
    if (!server.cacheFavorites)
        Statement(db_, "UPDATE tracks SET favorite=0 WHERE server=? AND favorite=1").bind(id).run();
    setSetting("rolling_mib", std::to_string(rollingMiB));
    transaction.commit();
    budget_ = rollingMiB * MiB;
    server.id = id;
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

void Store::saveTextCatalog(bool text) {
    std::scoped_lock lock(mutex_);
    setSetting("text_catalog", text ? "1" : "0");
}

bool Store::offlineMode() {
    std::scoped_lock lock(mutex_);
    return offline_;
}

void Store::setOfflineMode(bool offline) {
    std::scoped_lock lock(mutex_);
    setSetting("offline_mode", offline ? "1" : "0");
    offline_ = offline;
    if (!offline) retryConnection_.clear();
}

bool Store::networkAvailable(Id server) {
    std::scoped_lock lock(mutex_);
    return !offline_ && retryConnection_.count(server) == 0;
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

Track Store::loadTrack(Id key) {
    auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE key=?";
    Statement statement(db_, sql.c_str());
    statement.bind(key);
    return statement.row() ? getTrack(statement) : Track{};
}

Track Store::track(Id key) {
    std::scoped_lock lock(mutex_);
    return loadTrack(key);
}

void Store::rememberUnlocked(std::vector<Track>& tracks) {
    for (auto& track : tracks) {
        Statement(db_, "INSERT INTO tracks(server,remote_id,title,artist,album,suffix,mime,duration,"
            "cover_id,album_id,artist_id,album_artist_id,album_artist,genre,disc_number,track_number,year)"
            " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(server,remote_id) DO UPDATE SET"
            " title=coalesce(nullif(excluded.title,''),tracks.title),"
            "artist=coalesce(nullif(excluded.artist,''),tracks.artist),"
            "album=coalesce(nullif(excluded.album,''),tracks.album),"
            "suffix=CASE WHEN tracks.bytes>0 THEN tracks.suffix ELSE excluded.suffix END,"
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
            "mime=CASE WHEN tracks.bytes>0 THEN tracks.mime ELSE excluded.mime END")
            .bind(track.server).bind(track.id).bind(track.title).bind(track.artist)
            .bind(track.album).bind(track.suffix).bind(track.mime).bind(track.duration)
            .bind(track.coverId).bind(track.albumId).bind(track.artistId).bind(track.albumArtistId)
            .bind(track.albumArtist).bind(track.genre).bind(track.discNumber).bind(track.trackNumber)
            .bind(track.year).run();
        auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=? AND remote_id=?";
        Statement statement(db_, sql.c_str());
        statement.bind(track.server).bind(track.id);
        if (statement.row()) track = getTrack(statement);
    }
}

void Store::rememberPage(Id server, const BrowseLocation& location, bool unfiltered, int offset,
                         BrowsePage& page) {
    std::scoped_lock lock(mutex_);
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
        Statement(db_, "DELETE FROM catalog WHERE server=? AND parent_kind=? AND parent_id=?")
            .bind(server).bind(parent).bind(location.id).run();
    auto save = [&](const LibraryItem& item, Id slot, Id key) {
        Statement(db_, "INSERT INTO catalog VALUES(?,?,?,?,?,?,?,?,?,?)"
            " ON CONFLICT(server,parent_kind,parent_id,kind,id,slot) DO UPDATE SET"
            " title=excluded.title,subtitle=excluded.subtitle,"
            "cover_id=coalesce(nullif(excluded.cover_id,''),catalog.cover_id),track_key=excluded.track_key")
            .bind(server).bind(parent).bind(location.id).bind(static_cast<int>(item.target.kind))
            .bind(item.target.id).bind(slot).bind(item.target.title).bind(item.subtitle).bind(item.coverId)
            .bind(key).run();
    };
    auto saveTotals = [&](BrowseKind kind, const std::string& id, int songs, std::int64_t duration) {
        if (songs < 0 && duration <= 0) return;
        Statement(db_, "INSERT INTO catalog_totals VALUES(?,?,?,?,?) ON CONFLICT(server,kind,id) DO UPDATE SET"
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
    const auto kind = location.kind;
    const auto filter = wide(query);
    BrowsePage result;
    auto addTrack = [&](Track track) {
        if (!track.key || (!matches(track.title, filter) && !matches(track.artist, filter) &&
                          !matches(track.album, filter))) return;
        if (offset > 0) --offset;
        else if (result.tracks.size() == pageSize) result.more = true;
        else result.tracks.push_back(std::move(track));
    };
    if (kind == BrowseKind::playlistTracks) {
        Statement entries(db_, "SELECT track_key FROM catalog WHERE server=? AND parent_kind=?"
            " AND parent_id=? AND track_key>0 ORDER BY slot");
        entries.bind(server).bind(static_cast<int>(kind)).bind(location.id);
        while (!result.more && entries.row()) addTrack(loadTrack(entries.integer(0)));
    } else if (trackView(kind)) {
        auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=?";
        if (kind == BrowseKind::offline) sql += " AND (bytes>0 OR pinned=1 OR favorite=1)";
        if (kind == BrowseKind::song) sql += " AND remote_id=?";
        if (kind == BrowseKind::albumTracks)
            sql += localLocation(location) ?
                " AND album_id='' AND album||char(31)||coalesce(nullif(album_artist,''),artist)=?" :
                " AND (album_id=? OR (album_id='' AND album=?))";
        if (kind == BrowseKind::albumTracks) sql += " ORDER BY disc_number,track_number,title COLLATE NOCASE,key";
        else sql += " ORDER BY artist COLLATE NOCASE,album COLLATE NOCASE,title COLLATE NOCASE,key";
        Statement entries(db_, sql.c_str());
        entries.bind(server);
        if (kind == BrowseKind::song || kind == BrowseKind::albumTracks)
            entries.bind(localLocation(location) ? location.id.substr(1) : location.id);
        if (kind == BrowseKind::albumTracks && !localLocation(location)) entries.bind(location.title);
        while (!result.more && entries.row()) addTrack(getTrack(entries));
    } else {
        std::set<std::pair<BrowseKind, std::string>> seen;
        auto addItem = [&](LibraryItem item) {
            if ((!matches(item.target.title, filter) && !matches(item.subtitle, filter)) ||
                !seen.emplace(item.target.kind, item.target.id).second) return;
            if (offset > 0) --offset;
            else if (result.items.size() == pageSize) result.more = true;
            else result.items.push_back(std::move(item));
        };
        auto sql = std::string("SELECT kind,id,title,subtitle,cover_id,coalesce(songs,-1),coalesce(duration,0)"
            " FROM catalog LEFT JOIN catalog_totals USING(server,kind,id) WHERE server=? AND ");
        if (kind == BrowseKind::albums) sql += "kind=?";
        else sql += "parent_kind=? AND parent_id=?";
        sql += " ORDER BY title COLLATE NOCASE,id,cover_id DESC";
        Statement entries(db_, sql.c_str());
        entries.bind(server).bind(static_cast<int>(kind == BrowseKind::albums ? BrowseKind::albumTracks : kind));
        if (kind != BrowseKind::albums) entries.bind(location.id);
        std::set<std::pair<std::string, std::string>> names;
        while (!result.more && entries.row()) {
            names.emplace(entries.text(2), entries.text(3));
            addItem({{static_cast<BrowseKind>(entries.integer(0)), entries.text(1), entries.text(2)},
                entries.text(3), entries.text(4), static_cast<int>(entries.integer(5)),
                entries.integer(6)});
        }
        if (result.more) return result;
        if (kind == BrowseKind::genres) {
            Statement genres(db_, "SELECT DISTINCT genre FROM tracks WHERE server=? AND genre<>'' ORDER BY genre");
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
            Statement artists(db_, artistSql.c_str());
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
            Statement albums(db_, albumSql.c_str());
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
    Statement cover(db_, "SELECT cover_id FROM catalog WHERE server=? AND kind=? AND id=?"
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

std::string Store::artwork(Id server, const LibraryItem& item) {
    std::scoped_lock lock(mutex_);
    auto key = artworkKey(item);
    Statement artwork(db_, "SELECT data FROM artwork WHERE server=? AND cover_key=?");
    artwork.bind(server).bind(key);
    if (!artwork.row()) return {};
    auto data = artwork.blob(0);
    Statement(db_, "UPDATE artwork SET last_use=? WHERE server=? AND cover_key=?")
        .bind(nowMs()).bind(server).bind(key).run();
    return data;
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
    Statement match(db_, "SELECT cover_key,length(data) FROM artwork WHERE server=? AND cover_key IN (?,?)"
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
        Statement old(db_, "SELECT cover_key,ext,bytes,last_use FROM playlist_covers WHERE track=?");
        old.bind(track.key);
        if (old.row()) {
            oldExt = old.text(1);
            if (!validExt(oldExt)) oldExt.clear();
            if (!oldExt.empty() && old.text(0) == key && old.integer(2) == bytes &&
                std::filesystem::file_size(pathFor(track.key, oldExt), ec) == static_cast<uintmax_t>(bytes) && !ec) {
                // Avoid journaling on every metadata query during playback.
                if (old.integer(3) < nowMs() - 60000)
                    Statement(db_, "UPDATE playlist_covers SET last_use=? WHERE track=?")
                        .bind(nowMs()).bind(track.key).run();
                return false;
            }
        }
    }
    Statement payload(db_, "SELECT data FROM artwork WHERE server=? AND cover_key=?");
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
    Statement(db_, "INSERT OR REPLACE INTO playlist_covers VALUES(?,?,?,?,?)")
        .bind(track.key).bind(key).bind(utf8(ext)).bind(bytes).bind(nowMs()).run();

    Statement overflow(db_, "SELECT track,ext FROM (SELECT track,ext,"
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
        Statement(db_, "DELETE FROM playlist_covers WHERE track=?").bind(id).run();
    }
    transaction.commit();
    return true;
}

void Store::saveArtwork(Id server, const LibraryItem& item, const std::string& data) {
    if (data.empty()) return;
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    Statement(db_, "UPDATE playlist_covers SET cover_key='' WHERE cover_key=? AND track IN"
        " (SELECT key FROM tracks WHERE server=?)").bind(artworkKey(item)).bind(server).run();
    Statement(db_, "INSERT INTO artwork VALUES(?,?,?,?) ON CONFLICT(server,cover_key) DO UPDATE SET"
        " data=excluded.data,last_use=excluded.last_use")
        .bind(server).bind(artworkKey(item)).bindBlob(data).bind(nowMs()).run();
    Statement(db_, "DELETE FROM artwork WHERE (server,cover_key) IN (SELECT server,cover_key FROM"
        " (SELECT server,cover_key,sum(length(data)) OVER (ORDER BY last_use DESC,server,cover_key"
        " ROWS UNBOUNDED PRECEDING) AS bytes FROM artwork) WHERE bytes>?)").bind(128 * MiB).run();
    transaction.commit();
}

std::optional<Lyrics> Store::lyrics(Id key, bool fresh) {
    std::scoped_lock lock(mutex_);
    Statement lyrics(db_, "SELECT plain,structured,checked FROM lyrics WHERE track=?");
    lyrics.bind(key);
    if (!lyrics.row()) return {};
    Lyrics result{lyrics.text(0), lyrics.text(1)};
    if (fresh && result.plain.empty() && lyrics.integer(2) < nowMs() - 86'400'000) return {};
    return result;
}

void Store::saveLyrics(Id key, const Lyrics& lyrics) {
    std::scoped_lock lock(mutex_);
    Statement(db_, "INSERT INTO lyrics VALUES(?,?,?,?) ON CONFLICT(track) DO UPDATE SET"
        " plain=excluded.plain,structured=excluded.structured,checked=excluded.checked")
        .bind(key).bind(lyrics.plain).bind(lyrics.structured).bind(nowMs()).run();
}

void Store::pin(const std::vector<Track>& tracks, bool permanent) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    for (const auto& track : tracks)
        Statement(db_, "UPDATE tracks SET pinned=?,retry_at=0 WHERE key=?").bind(permanent).bind(track.key).run();
    transaction.commit();
}

void Store::syncFavorites(Id server, BrowsePage& page) {
    std::scoped_lock lock(mutex_);
    Statement enabled(db_, "SELECT id FROM servers WHERE id=? AND cache_favorites=1");
    enabled.bind(server);
    if (!enabled.row()) return;
    Transaction transaction(db_);
    rememberUnlocked(page.tracks);
    rememberCatalog(server, {BrowseKind::favorites, {}, {}}, true, 0, page);
    Statement(db_, "UPDATE tracks SET favorite=0 WHERE server=? AND favorite=1").bind(server).run();
    for (const auto& track : page.tracks)
        Statement(db_, "UPDATE tracks SET favorite=1 WHERE key=?").bind(track.key).run();
    transaction.commit();
}

std::vector<Track> Store::pendingPins(Id server) {
    std::scoped_lock lock(mutex_);
    auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=? AND bytes=0"
        " AND (pinned=1 OR favorite=1) AND retry_at<=? ORDER BY pinned DESC,key LIMIT 4";
    Statement statement(db_, sql.c_str());
    statement.bind(server).bind(nowMs());
    std::vector<Track> result;
    while (statement.row()) result.push_back(getTrack(statement));
    return result;
}

void Store::postponePin(Id key) {
    std::scoped_lock lock(mutex_);
    Statement(db_, "UPDATE tracks SET retry_at=? WHERE key=?").bind(nowMs() + 60'000).bind(key).run();
}

std::filesystem::path Store::audioPath(const Track& track) const {
    auto suffix = track.suffix;
    if (suffix.empty() || suffix.size() > 12 ||
        suffix.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != suffix.npos)
        suffix = "mp3";
    return cache_ / (std::to_wstring(track.key) + L".media." + wide(suffix));
}

HANDLE Store::openCached(Id key, std::filesystem::path& path) {
    std::scoped_lock lock(mutex_);
    auto track = loadTrack(key);
    if (!track.bytes) return INVALID_HANDLE_VALUE;
    path = audioPath(track);
    auto file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    return file;
}

std::filesystem::path Store::partialPath(Id key) const { return cache_ / (std::to_wstring(key) + L".part"); }

void Store::reconcile() {
    Statement saved(db_, "SELECT key,bytes FROM tracks WHERE bytes>0");
    std::vector<Id> missing;
    std::map<Id, std::uintmax_t> files;
    while (saved.row()) files.emplace(saved.integer(0), saved.integer(1));
    for (const auto& [key, bytes] : files) {
        std::error_code error;
        auto path = audioPath(loadTrack(key));
        auto legacy = cache_ / (std::to_wstring(key) + L".audio");
        if (!std::filesystem::exists(path) && std::filesystem::exists(legacy))
            std::filesystem::rename(legacy, path);
        if (std::filesystem::file_size(path, error) != bytes || error) missing.push_back(key);
    }
    for (const auto& entry : std::filesystem::directory_iterator(cache_)) {
        auto extension = entry.path().extension();
        auto stem = entry.path().stem().string();
        const bool media = stem.size() > 6 && stem.compare(stem.size()-6, 6, ".media") == 0;
        if (media) stem.resize(stem.size()-6);
        Id key;
        auto parsed = std::from_chars(stem.data(), stem.data() + stem.size(), key);
        if (parsed.ec != std::errc{} || parsed.ptr != stem.data() + stem.size()) continue;
        if (extension == L".part" || ((media || extension == L".audio") &&
            (!files.count(key) || std::find(missing.begin(), missing.end(), key) != missing.end()))) {
            std::error_code error;
            std::filesystem::remove(entry.path(), error);
        }
    }
    for (Id key : missing) Statement(db_, "UPDATE tracks SET bytes=0 WHERE key=?").bind(key).run();
}

bool Store::trimUnlocked(std::int64_t incoming) {
    for (const auto& [key, bytes] : downloads_) {
        auto track = loadTrack(key);
        if (!track.pinned && !track.favorite) incoming += bytes;
    }
    if (incoming > budget_) return false;
    Statement total(db_, "SELECT COALESCE(SUM(bytes),0) FROM tracks WHERE pinned=0 AND favorite=0");
    total.row();
    auto used = total.integer(0);
    total.run();
    if (used <= budget_ - incoming) return true;
    Statement oldest(db_, "SELECT key,bytes FROM tracks WHERE bytes>0 AND pinned=0 AND favorite=0 ORDER BY last_use,key");
    std::vector<std::pair<Id, std::int64_t>> candidates;
    while (oldest.row()) candidates.emplace_back(oldest.integer(0), oldest.integer(1));
    for (const auto& [key, bytes] : candidates) {
        auto path = audioPath(loadTrack(key));
        if (!DeleteFileW(path.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) continue;
        Statement(db_, "UPDATE tracks SET bytes=0 WHERE key=?").bind(key).run();
        used -= bytes;
        if (used <= budget_ - incoming) return true;
    }
    return false;
}

bool Store::reserve(Id key, std::int64_t bytes) {
    std::scoped_lock lock(mutex_);
    auto track = loadTrack(key);
    if (!track.key || track.bytes > 0 || downloads_.count(key)) return false;
    if (!track.pinned && !track.favorite && (bytes <= 0 || !trimUnlocked(bytes))) return false;
    downloads_.emplace(key, bytes > 0 ? bytes : 0);
    return true;
}

void Store::publish(Id key, std::int64_t bytes, const std::string& mime) {
    std::scoped_lock lock(mutex_);
    downloads_.at(key) = 0;
    auto track = loadTrack(key);
    if (bytes <= 0 || (!track.pinned && !track.favorite && !trimUnlocked(bytes))) return;
    if (!MoveFileExW(partialPath(key).c_str(), audioPath(track).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw Error("The completed audio file could not be saved.");
    try {
        Statement(db_, "UPDATE tracks SET bytes=?,last_use=?,mime=?,retry_at=0 WHERE key=?")
            .bind(bytes).bind(nowMs()).bind(mime).bind(key).run();
    } catch (...) {
        DeleteFileW(audioPath(track).c_str());
        throw;
    }
}

void Store::release(Id key) {
    std::scoped_lock lock(mutex_);
    DeleteFileW(partialPath(key).c_str());
    downloads_.erase(key);
}

void Store::touch(Id key) {
    std::scoped_lock lock(mutex_);
    Statement(db_, "UPDATE tracks SET last_use=? WHERE key=?").bind(nowMs()).bind(key).run();
}

void Store::trim() {
    std::scoped_lock lock(mutex_);
    trimUnlocked(0);
}

void Store::enqueueReport(const Track& track, std::int64_t time) {
    std::scoped_lock lock(mutex_);
    Statement(db_, "INSERT INTO reports(server,remote_id,time) VALUES(?,?,?)")
        .bind(track.server).bind(track.id).bind(time).run();
}

std::vector<Report> Store::reports(Id server) {
    std::scoped_lock lock(mutex_);
    Statement statement(db_, "SELECT key,remote_id,time FROM reports WHERE server=? ORDER BY key LIMIT 25");
    statement.bind(server);
    std::vector<Report> result;
    std::size_t querySize = 0;
    while (statement.row()) {
        auto id = statement.text(1);
        querySize += id.size() * 3 + 32;
        if (querySize > 4800 && !result.empty()) break;
        result.push_back({statement.integer(0), std::move(id), statement.integer(2)});
    }
    return result;
}

void Store::acknowledge(const std::vector<Report>& reports) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    for (const auto& report : reports)
        Statement(db_, "DELETE FROM reports WHERE key=?").bind(report.key).run();
    transaction.commit();
}

std::int64_t Store::reportCount(Id server) {
    std::scoped_lock lock(mutex_);
    Statement statement(db_, "SELECT COUNT(*) FROM reports WHERE server=?");
    statement.bind(server);
    return statement.row() ? statement.integer(0) : 0;
}

} // namespace tangerine
