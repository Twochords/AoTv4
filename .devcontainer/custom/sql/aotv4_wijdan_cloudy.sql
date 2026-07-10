-- AoTv4: free, LORE Cloudy Potion sold by Wijdan (npc/merchant 189021) in the Tutorial (tutorialb).
--   * FREE  -> the item's own price is 0 (merchantlist has no per-entry price override).
--   * LORE  -> loregroup = -1, so a player can only hold ONE at a time; they must use it before buying
--              another. That's the "one at a time" limiter without any custom buy logic.
-- Uses a CUSTOM item id (990001) copied from the stock Cloudy Potion (14514) so the stock potion -- which
-- is on 63 merchants + 12 loot tables -- is left completely untouched. Same name/icon/heal effect.
--
-- Items live in SHARED MEMORY: after running this, stop world+zones, rebuild ./shared_memory, restart.
-- Idempotent: re-running rebuilds id 990001 and the single merchant row.

DROP TEMPORARY TABLE IF EXISTS tmp_cloudy;
CREATE TEMPORARY TABLE tmp_cloudy LIKE items;
INSERT INTO tmp_cloudy SELECT * FROM items WHERE id = 14514;   -- stock Cloudy Potion
UPDATE tmp_cloudy SET id = 990001, price = 0, loregroup = -1;  -- free + lore (one at a time)

DELETE FROM items WHERE id = 990001;                           -- clean re-run
INSERT INTO items SELECT * FROM tmp_cloudy;

-- Wijdan's merchant list: drop any prior copy, then add at the next free slot.
DELETE FROM merchantlist WHERE merchantid = 189021 AND item = 990001;
INSERT INTO merchantlist (merchantid, slot, item, probability)
  SELECT 189021, next_slot, 990001, 100
  FROM (SELECT COALESCE(MAX(slot), 0) + 1 AS next_slot FROM merchantlist WHERE merchantid = 189021) t;
