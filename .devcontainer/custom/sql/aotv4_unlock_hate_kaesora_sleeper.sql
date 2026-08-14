-- AoTv4: make hateplaneb, kaesora, sleeper reachable/ungated (owner ask 2026-08-13).
-- All three already have a zone_regions entry; this clears the level gate + era gate:
--   hateplaneb (186) min_level 46->0; sleeper (128) min_level 46->0; kaesora (88) expansion 1->0.
-- zone table is read at ZONE BOOT (not shared memory) -> effective next boot, no restart/rebuild.
-- Live-only: the zone table is SKIPPED on our DB imports, so re-run after any full zone import.
UPDATE zone SET min_level = 0, expansion = 0 WHERE short_name IN ('hateplaneb','kaesora','sleeper');
