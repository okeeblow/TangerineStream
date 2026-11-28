#include "migrations.hpp"
#include "sqlite_statement.hpp"
#include <iterator>

namespace tangerine {

void migrateLibrary(sqlite3* db) {
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

} // namespace tangerine
