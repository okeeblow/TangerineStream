#pragma once

#include "core/model.hpp"
#include "tangerine_product.h"
#include <sqlite3.h>
#include <utility>

namespace tangerine {

[[noreturn]] inline void databaseError(sqlite3* db) {
    throw Error(std::string(TS_PRODUCT_NAME " storage: ") + (db ? sqlite3_errmsg(db) : "out of memory"));
}

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

} // namespace tangerine
