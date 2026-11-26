CREATE INDEX artwork_lru ON artwork(last_use, server DESC, cover_key DESC);
