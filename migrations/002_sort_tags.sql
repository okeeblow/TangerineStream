-- Some version 1 builds created these tables without advancing user_version.
CREATE TABLE IF NOT EXISTS playlist_covers (
    track INTEGER PRIMARY KEY,
    cover_key TEXT NOT NULL,
    ext TEXT NOT NULL,
    bytes INTEGER NOT NULL,
    last_use INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS catalog_totals (
    server INTEGER NOT NULL REFERENCES servers(id),
    kind INTEGER NOT NULL,
    id TEXT NOT NULL,
    songs INTEGER NOT NULL,
    duration INTEGER NOT NULL,
    PRIMARY KEY (server, kind, id)
) WITHOUT ROWID;
ALTER TABLE tracks ADD COLUMN sort_tags TEXT NOT NULL DEFAULT '';
ALTER TABLE catalog ADD COLUMN sort_tags TEXT NOT NULL DEFAULT '';
