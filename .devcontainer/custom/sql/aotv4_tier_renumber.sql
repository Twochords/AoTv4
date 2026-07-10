-- ==========================================================================================
-- ONE-TIME migration (2026-07): renumber existing tier REFERENCES from the old +1,000,000 /
-- +2,000,000 ids to the new +300,000 / +600,000 ids. See aotv4_gear_tiers.sql for the why:
-- RoF2 item links encode the id in a 5-hex-digit field capped at 0xFFFFF (1,048,575), so the old
-- offsets pushed Mythic (and high-base Hallowed) past the ceiling and their chat links rendered
-- as garbage. New scheme keeps every tier id well under the ceiling.
--
-- The tier ITEM rows are rebuilt by aotv4_gear_tiers.sql; THIS file only fixes live references
-- that still point at the old ids (inventories, banks, corpses, shops, merchants, world objects).
--   Hallowed: old = base+1,000,000 -> new = base+300,000   (new = old - 700,000)
--   Mythic:   old = base+2,000,000 -> new = base+600,000   (new = old - 1,400,000)
-- The refine crucible bag (2000060, aotv4_refine_crucible.sql) is NOT a tier -> always spared.
--
-- Run order (world DOWN): aotv4_gear_tiers.sql -> THIS -> ./shared_memory -> restart.
-- Idempotent: the old band is empty after one run, so re-running is a no-op.
-- ==========================================================================================

-- ---- Hallowed refs: old 1,000,001..1,999,999  ->  -700,000 ------------------------------------
UPDATE inventory              SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE sharedbank             SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE character_corpse_items SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE character_parcels      SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE character_bandolier    SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE character_potionbelt   SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE trader                 SET item_id = item_id - 700000  WHERE item_id BETWEEN 1000001 AND 1999999;
UPDATE object                 SET itemid  = itemid  - 700000  WHERE itemid  BETWEEN 1000001 AND 1999999;
UPDATE object_contents        SET itemid  = itemid  - 700000  WHERE itemid  BETWEEN 1000001 AND 1999999;
UPDATE merchantlist           SET item    = item    - 700000  WHERE item    BETWEEN 1000001 AND 1999999;

-- ---- Mythic refs: old 2,000,001..2,999,999  ->  -1,400,000  (spare bag 2000060) ---------------
UPDATE inventory              SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE sharedbank             SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE character_corpse_items SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE character_parcels      SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE character_bandolier    SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE character_potionbelt   SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE trader                 SET item_id = item_id - 1400000 WHERE item_id BETWEEN 2000001 AND 2999999 AND item_id <> 2000060;
UPDATE object                 SET itemid  = itemid  - 1400000 WHERE itemid  BETWEEN 2000001 AND 2999999 AND itemid  <> 2000060;
UPDATE object_contents        SET itemid  = itemid  - 1400000 WHERE itemid  BETWEEN 2000001 AND 2999999 AND itemid  <> 2000060;
UPDATE merchantlist           SET item    = item    - 1400000 WHERE item    BETWEEN 2000001 AND 2999999 AND item    <> 2000060;

-- ---- cleanup: drop merchant entries selling a tier item that doesn't exist ---------------------
-- Pre-existing dirt: some vendors had tier copies of NON-equippable stock (spells/food/components,
-- slots=0) that never got a tier row (only equippable gear is tiered), so the ref was always dead.
-- Remove those dangling tier-band entries so no vendor shows a broken/blank slot.
DELETE m FROM merchantlist m LEFT JOIN items it ON it.id = m.item
  WHERE m.item BETWEEN 300000 AND 899999 AND it.id IS NULL;
