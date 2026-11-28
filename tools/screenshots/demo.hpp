#pragma once
#include "server.hpp"
#include <sqlite3.h>
#include <simdjson.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

namespace demo {
using tangerine::Error;
using tangerine::jsonString;
using Element = simdjson::dom::element;

inline std::string field(Element node, std::string_view key) {
    std::string_view value;
    return node[key].get_string().get(value) ? "" : std::string(value);
}

class Database {
public:
    Database(const std::filesystem::path& file, int flags) {
        sqlite3* opened = nullptr;
        int status = sqlite3_open_v2(tangerine::utf8(file.wstring()).c_str(), &opened, flags, nullptr);
        db_.reset(opened);
        if (status != SQLITE_OK) throw Error("Could not open the demo database.");
    }
    sqlite3* get() const { return db_.get(); }
    void exec(const char* sql) {
        if (sqlite3_exec(get(), sql, nullptr, nullptr, nullptr) != SQLITE_OK) throw Error(sqlite3_errmsg(get()));
    }
private:
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db_{nullptr, sqlite3_close};
};

class Query {
public:
    Query(Database& db, const char* sql) : db_(db.get()) {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &statement, nullptr) != SQLITE_OK) throw Error(sqlite3_errmsg(db_));
        statement_.reset(statement);
    }
    Query& bind(const std::string& value) {
        check(sqlite3_bind_text(statement_.get(), ++parameter_, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT));
        return *this;
    }
    Query& bind(int n) { check(sqlite3_bind_int(statement_.get(), ++parameter_, n)); return *this; }
    Query& blob(const std::string& value) {
        check(sqlite3_bind_blob(statement_.get(), ++parameter_, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT));
        return *this;
    }
    bool next() {
        int status = sqlite3_step(statement_.get());
        if (status == SQLITE_ROW) return true;
        if (status != SQLITE_DONE) throw Error(sqlite3_errmsg(db_));
        return false;
    }
    std::string bytes(int index = 0) const {
        auto data = static_cast<const char*>(sqlite3_column_blob(statement_.get(), index));
        return data ? std::string(data, sqlite3_column_bytes(statement_.get(), index)) : "";
    }
    int number(int index = 0) const { return sqlite3_column_int(statement_.get(), index); }
private:
    void check(int status) { if (status != SQLITE_OK) throw Error(sqlite3_errmsg(db_)); }
    sqlite3* db_;
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement_{nullptr, sqlite3_finalize};
    int parameter_ = 0;
};

struct Entry {
    std::string kind, id, name, cover, data;
};
using ObjectKey = std::pair<std::string,std::string>;
struct Collection {
    int total = 0;
    std::vector<ObjectKey> members;
    int leadingBlanks = 0;
};
struct Fixture {
    std::map<ObjectKey,Entry> objects;
    std::map<std::string,Collection> collections;
    std::map<std::string,std::string> poses, coverAliases, lyrics;
    std::vector<std::string> playback;

    void save(Database& db) const;
    void load(Database& db);
};

void exportLibrary(const std::filesystem::path& destination, const std::string& user, const std::string& password);
void serve(const std::filesystem::path& database, const std::string& user, const std::string& password);
} // namespace demo
