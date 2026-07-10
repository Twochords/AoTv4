-- ==========================================================================================
-- AoTv4: make ALL equippable gear usable by every class + race (classes=65535, races=65535).
-- Everyone plays a Bard, so class/race gates on gear only lock players out of items they should
-- be able to wear. This opens every worn item (slots>0) regardless of id band -- base gear AND
-- the generated tiers (which aotv4_gear_tiers.sql already forces all/all, so they're a no-op here).
-- Only classes/races are touched; lore/nodrop/stats are left exactly as-is.
--
-- Items live in SHARED MEMORY -- after running this, rebuild ./shared_memory + restart world/zones.
-- Idempotent: the WHERE skips rows already all/all, so re-running is cheap and safe.
-- ==========================================================================================

UPDATE items
  SET classes = 65535, races = 65535
  WHERE slots > 0 AND (classes <> 65535 OR races <> 65535);
