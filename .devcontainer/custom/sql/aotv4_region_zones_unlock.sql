-- aotv4_region_zones_unlock.sql  (2026-08-01)
-- =================================================================================================
-- Open every REGION-MAPPED zone (regions 0..98) so the region system -- not the expansion/era gate
-- -- controls access. Motivating case: the six DoN "Delve" zones (stillmoona 338, stillmoonb 339,
-- thundercrest 340, delvea 341, delveb 342, thenest 343) showed as "region locked" but were in fact
-- blocked by expansion, not region:
--   * They are region 0 ("Always Available") -> RegionManager::CanEnterZone passes for everyone.
--   * They carried expansion = 9 (Dragons of Norrath) while the server runs Expansion:CurrentExpansion
--     = 0 (Classic), so the CONTENT/expansion filter blocked them.
--
-- Setting expansion = 0 (and min_status = 0) on all region-mapped zones removes that gate; access is
-- then governed by zone_regions + Client:HasRegion as intended. region 99 ("Unused", 414 fenced
-- zones) is deliberately left out (zr.region_id <= 98).
--
-- zone rows are in SHARED MEMORY: apply with world DOWN, then ./bin/shared_memory, then restart, or
-- the running server keeps the stale mmap.

UPDATE `zone` z
  JOIN zone_regions zr ON z.zoneidnumber = zr.zone_id
   SET z.expansion = 0,
       z.min_status = 0
 WHERE zr.region_id <= 98;
