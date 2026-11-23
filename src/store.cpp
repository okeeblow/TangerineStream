#include "store.hpp"

#include <wincrypt.h>
#include <algorithm>
#include <charconv>
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
int WINAPI sqlite3_step(sqlite3_stmt*);
std::int64_t WINAPI sqlite3_column_int64(sqlite3_stmt*, int);
const unsigned char* WINAPI sqlite3_column_text(sqlite3_stmt*, int);
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
        auto copy = reinterpret_cast<void (*)(void*)>(static_cast<std::intptr_t>(-1));
        if (sqlite3_bind_text(statement_, next_++, value.c_str(), static_cast<int>(value.size()),
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

private:
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
    "key,server,remote_id,title,artist,album,suffix,mime,duration,pinned,favorite,bytes";

Track getTrack(const Statement& statement) {
    return {statement.integer(0), statement.integer(1), statement.text(2), statement.text(3),
            statement.text(4), statement.text(5), statement.text(6), statement.text(7),
            static_cast<int>(statement.integer(8)), statement.integer(9) != 0,
            statement.integer(10) != 0, statement.integer(11)};
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
        auto budget = setting("rolling_mib");
        budget_ = (budget.empty() ? defaultBudget : std::stoi(budget)) * MiB;
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
    auto secret = setting("playback_secret");
    if (secret.empty()) {
        secret = randomHex();
        setSetting("playback_secret", secret);
    }
    return secret;
}

int Store::playbackPort() {
    std::scoped_lock lock(mutex_);
    auto port = setting("playback_port");
    return port.empty() ? 0 : std::stoi(port);
}

void Store::savePlaybackPort(int port) {
    std::scoped_lock lock(mutex_);
    setSetting("playback_port", std::to_string(port));
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
        Statement(db_, "INSERT INTO tracks(server,remote_id,title,artist,album,suffix,mime,duration)"
            " VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(server,remote_id) DO UPDATE SET title=excluded.title,"
            "artist=excluded.artist,album=excluded.album,suffix=excluded.suffix,duration=excluded.duration,"
            "mime=CASE WHEN tracks.bytes>0 THEN tracks.mime ELSE excluded.mime END")
            .bind(track.server).bind(track.id).bind(track.title).bind(track.artist)
            .bind(track.album).bind(track.suffix).bind(track.mime).bind(track.duration).run();
        auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=? AND remote_id=?";
        Statement statement(db_, sql.c_str());
        statement.bind(track.server).bind(track.id);
        if (statement.row()) track = getTrack(statement);
    }
}

void Store::remember(std::vector<Track>& tracks) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    rememberUnlocked(tracks);
    transaction.commit();
}

std::vector<Track> Store::cached(Id server, const std::string& query, int offset) {
    std::scoped_lock lock(mutex_);
    auto sql = std::string("SELECT ") + trackColumns + " FROM tracks WHERE server=?"
        " AND (bytes>0 OR pinned=1 OR favorite=1) AND (instr(lower(title),lower(?))>0"
        " OR instr(lower(artist),lower(?))>0 OR instr(lower(album),lower(?))>0)"
        " ORDER BY artist COLLATE NOCASE,album COLLATE NOCASE,title COLLATE NOCASE,key LIMIT ? OFFSET ?";
    Statement statement(db_, sql.c_str());
    statement.bind(server).bind(query).bind(query).bind(query).bind(pageSize).bind(offset);
    std::vector<Track> result;
    while (statement.row()) result.push_back(getTrack(statement));
    return result;
}

void Store::pin(const std::vector<Track>& tracks, bool permanent) {
    std::scoped_lock lock(mutex_);
    Transaction transaction(db_);
    for (const auto& track : tracks)
        Statement(db_, "UPDATE tracks SET pinned=?,retry_at=0 WHERE key=?").bind(permanent).bind(track.key).run();
    transaction.commit();
}

void Store::syncFavorites(Id server, std::vector<Track>& tracks) {
    std::scoped_lock lock(mutex_);
    Statement enabled(db_, "SELECT id FROM servers WHERE id=? AND cache_favorites=1");
    enabled.bind(server);
    if (!enabled.row()) return;
    Transaction transaction(db_);
    rememberUnlocked(tracks);
    Statement(db_, "UPDATE tracks SET favorite=0 WHERE server=? AND favorite=1").bind(server).run();
    for (const auto& track : tracks)
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

std::filesystem::path Store::audioPath(Id key) const { return cache_ / (std::to_wstring(key) + L".audio"); }
std::filesystem::path Store::partialPath(Id key) const { return cache_ / (std::to_wstring(key) + L".part"); }

void Store::reconcile() {
    Statement saved(db_, "SELECT key,bytes FROM tracks WHERE bytes>0");
    std::vector<Id> missing;
    std::map<Id, std::uintmax_t> files;
    while (saved.row()) files.emplace(saved.integer(0), saved.integer(1));
    for (const auto& [key, bytes] : files) {
        std::error_code error;
        if (std::filesystem::file_size(audioPath(key), error) != bytes || error) missing.push_back(key);
    }
    for (const auto& entry : std::filesystem::directory_iterator(cache_)) {
        auto extension = entry.path().extension();
        auto stem = entry.path().stem().string();
        Id key;
        auto parsed = std::from_chars(stem.data(), stem.data() + stem.size(), key);
        if (parsed.ec != std::errc{} || parsed.ptr != stem.data() + stem.size()) continue;
        if (extension == L".part" || (extension == L".audio" &&
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
        auto path = audioPath(key);
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
    if (!MoveFileExW(partialPath(key).c_str(), audioPath(key).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw Error("The completed audio file could not be saved.");
    try {
        Statement(db_, "UPDATE tracks SET bytes=?,last_use=?,mime=?,retry_at=0 WHERE key=?")
            .bind(bytes).bind(nowMs()).bind(mime).bind(key).run();
    } catch (...) {
        DeleteFileW(audioPath(key).c_str());
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
