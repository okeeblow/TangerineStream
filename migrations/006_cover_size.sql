-- All covers saved before this migration were requested at 640 pixels.
ALTER TABLE artwork ADD COLUMN requested_max_edge_px INTEGER NOT NULL DEFAULT 640;
