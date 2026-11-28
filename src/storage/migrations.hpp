#pragma once

struct sqlite3;

namespace tangerine {
void migrateLibrary(sqlite3* db);
}
