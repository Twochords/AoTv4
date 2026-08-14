-- AoTv4: make Kaesora reachable + set its authored Zone-XP range. It sat in region 99 "Unused"
-- (unreachable), same failure the LDoN zones had. Kaesora is Kunark -> region 6 (Cabilis/Kunark).
-- Swamp of No Hope (83) is reachable from BOTH Firiona Vie (4) and Cabilis (6). Matches the dev dump.
-- Read at ZONE BOOT (not shared memory) -> effective on next boot, no restart/rebuild.
-- ⚠️ aotv4_zone_xp ranges are AUTHORED, NOT derived from spawn levels (CLAUDE.md §3): the owner sets
-- them and they routinely differ from the real mob levels. Kaesora's authored range is 11-23 even
-- though its spawns are 28-40 -- do NOT "correct" it to the spawn range.
UPDATE zone_regions SET region_id = 6 WHERE zone_id = 88 AND region_id = 99;
INSERT INTO zone_regions (zone_id, region_id, name)
  SELECT 83, 6, 'Swamp of No Hope'
  WHERE NOT EXISTS (SELECT 1 FROM zone_regions WHERE zone_id=83 AND region_id=6);
UPDATE aotv4_zone_xp SET lo = 11, hi = 23 WHERE zone_id = 88;
