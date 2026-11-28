-- Unversioned installations may already have these four tables.
CREATE TABLE IF NOT EXISTS settings (
    name TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS servers (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    url TEXT NOT NULL,
    user TEXT NOT NULL,
    password TEXT NOT NULL,
    native INTEGER NOT NULL,
    cache_favorites INTEGER NOT NULL,
    UNIQUE (url, user)
);
CREATE TABLE IF NOT EXISTS tracks (
    key INTEGER PRIMARY KEY,
    server INTEGER NOT NULL REFERENCES servers(id),
    remote_id TEXT NOT NULL,
    title TEXT NOT NULL,
    artist TEXT NOT NULL,
    album TEXT NOT NULL,
    suffix TEXT NOT NULL,
    mime TEXT NOT NULL,
    duration INTEGER NOT NULL,
    pinned INTEGER NOT NULL DEFAULT 0,
    favorite INTEGER NOT NULL DEFAULT 0,
    bytes INTEGER NOT NULL DEFAULT 0,
    last_use INTEGER NOT NULL DEFAULT 0,
    retry_at INTEGER NOT NULL DEFAULT 0,
    UNIQUE (server, remote_id)
);
CREATE TABLE IF NOT EXISTS pending_listens (
    key INTEGER PRIMARY KEY,
    server INTEGER NOT NULL REFERENCES servers(id),
    remote_id TEXT NOT NULL,
    listened_at_ms INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS pending_listens_submission_order ON pending_listens(server, key);

ALTER TABLE tracks ADD COLUMN cover_id TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN album_id TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN artist_id TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN album_artist_id TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN album_artist TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN genre TEXT NOT NULL DEFAULT '';
ALTER TABLE tracks ADD COLUMN disc_number INTEGER NOT NULL DEFAULT 0;
ALTER TABLE tracks ADD COLUMN track_number INTEGER NOT NULL DEFAULT 0;
ALTER TABLE tracks ADD COLUMN year INTEGER NOT NULL DEFAULT 0;
CREATE INDEX tracks_album ON tracks(server, album_id);
CREATE INDEX tracks_artist ON tracks(server, artist_id);
CREATE INDEX tracks_album_artist ON tracks(server, album_artist_id);
CREATE INDEX tracks_genre ON tracks(server, genre);

CREATE TABLE catalog (
    server INTEGER NOT NULL REFERENCES servers(id),
    parent_kind INTEGER NOT NULL,
    parent_id TEXT NOT NULL,
    kind INTEGER NOT NULL,
    id TEXT NOT NULL,
    slot INTEGER NOT NULL,
    title TEXT NOT NULL,
    subtitle TEXT NOT NULL,
    cover_id TEXT NOT NULL,
    track_key INTEGER NOT NULL,
    PRIMARY KEY (server, parent_kind, parent_id, kind, id, slot)
) WITHOUT ROWID;
CREATE INDEX catalog_objects ON catalog(server, kind, id);
CREATE TABLE artwork (
    server INTEGER NOT NULL REFERENCES servers(id),
    cover_key TEXT NOT NULL,
    data BLOB NOT NULL,
    last_use INTEGER NOT NULL,
    PRIMARY KEY (server, cover_key)
) WITHOUT ROWID;
CREATE TABLE lyrics (
    track INTEGER PRIMARY KEY REFERENCES tracks(key),
    plain TEXT NOT NULL,
    structured TEXT NOT NULL,
    checked INTEGER NOT NULL
);
