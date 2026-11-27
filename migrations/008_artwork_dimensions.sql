-- Existing covers are measured on their next upgrade.
ALTER TABLE artwork ADD COLUMN cached_longest_edge_px INTEGER NOT NULL DEFAULT 0;
