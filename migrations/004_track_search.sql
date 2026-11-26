CREATE VIRTUAL TABLE track_search USING fts5(
    title, artist, album, content='tracks', content_rowid='key', tokenize='trigram', columnsize=0
);
INSERT INTO track_search(track_search) VALUES ('rebuild');

CREATE TRIGGER track_search_insert AFTER INSERT ON tracks BEGIN
    INSERT INTO track_search(rowid, title, artist, album)
    VALUES (new.key, new.title, new.artist, new.album);
END;
CREATE TRIGGER track_search_delete AFTER DELETE ON tracks BEGIN
    INSERT INTO track_search(track_search, rowid, title, artist, album)
    VALUES ('delete', old.key, old.title, old.artist, old.album);
END;
CREATE TRIGGER track_search_update AFTER UPDATE OF title, artist, album ON tracks
WHEN old.title != new.title OR old.artist != new.artist OR old.album != new.album BEGIN
    INSERT INTO track_search(track_search, rowid, title, artist, album)
    VALUES ('delete', old.key, old.title, old.artist, old.album);
    INSERT INTO track_search(rowid, title, artist, album)
    VALUES (new.key, new.title, new.artist, new.album);
END;
CREATE INDEX tracks_linguistic ON tracks(server)
WHERE title GLOB '*[^ -~]*' OR artist GLOB '*[^ -~]*' OR album GLOB '*[^ -~]*';
CREATE INDEX tracks_browse_order ON tracks(server, artist COLLATE NOCASE, album COLLATE NOCASE,
                                          title COLLATE NOCASE, key);
