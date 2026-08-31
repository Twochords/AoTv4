-- Mold drops. Design: TITANWROUGHT_CRAFTING_DESIGN.md §9.
--   Crude molds  -> open-world NAMED mobs, levels 1-15
--   Simple molds -> open-world NAMED mobs, levels 14-30
--   Rough molds  -> RAIDS ONLY, placed by aotv4_raid.lua. Deliberately NO global_loot row:
--                   `raid = 1` would be nearly right and is still wrong, because IsRaidTarget()
--                   is a per-NPC flag any future content could set, and Rough is the only tier
--                   that beats what the world can give you.
--
-- ⚠️⚠️ `rare = 1` MEANS npc_types.rare_spawn, NOT this project's usual "named" heuristic.
--    Mob::IsRareSpawn() is `return rare_spawn` (zone/mob.h:933). The heuristic used in §17c and
--    the delve ledger (lowercase is trash, proper noun is named) matches 17,747 NPC types here
--    against rare_spawn's 568, and the excess is almost entirely CIVIC -- guards, merchants and
--    bankers all have proper names. Keyed on the heuristic, Freeport guards become a mold farm.
--    Measured coverage of rare_spawn in the 1-25 band: 139 NPCs across all six playable regions
--    (9-37 each), plus 27 stranded in region 99.
-- ⚠️ max_level MUST be written. A row with only min_level matches everything above it -- the
--    stock `AoTv4 Ink of the Lost` row (id 100) is exactly that shape.
-- 📌 probability 25 is a first guess and is the one number to tune. Named mobs are already rare,
--    the mold is 1 of 70, and molds are CONSUMED per combine, so this is deliberately generous.
DELETE FROM lootdrop_entries WHERE lootdrop_id IN (200040,200041);
DELETE FROM loottable_entries WHERE loottable_id IN (200040,200041);
DELETE FROM lootdrop  WHERE id IN (200040,200041);
DELETE FROM loottable WHERE id IN (200040,200041);
DELETE FROM global_loot WHERE id IN (101,102);

INSERT INTO loottable (id,name) VALUES
 (200040,'AoTv4 Crude Titanwrought Molds'),(200041,'AoTv4 Simple Titanwrought Molds');
INSERT INTO lootdrop (id,name) VALUES
 (200040,'AoTv4 Crude Titanwrought Molds'),(200041,'AoTv4 Simple Titanwrought Molds');
INSERT INTO loottable_entries (loottable_id,lootdrop_id,multiplier,droplimit,mindrop,probability)
 VALUES (200040,200040,1,1,1,25),(200041,200041,1,1,1,25);

INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,multiplier)
 SELECT 200040,id,1,0,1,1 FROM items WHERE id BETWEEN 148200 AND 148269;
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,multiplier)
 SELECT 200041,id,1,0,1,1 FROM items WHERE id BETWEEN 148270 AND 148339;

INSERT INTO global_loot (id,description,loottable_id,enabled,min_level,max_level,rare)
 VALUES (101,'AoTv4 Crude Titanwrought Molds (named)', 200040,1, 1,15,1),
        (102,'AoTv4 Simple Titanwrought Molds (named)',200041,1,14,30,1);

-- ⚠️⚠️ TURN OFF THE GEAR DROPS. Rows 1-4 put Crude/Simple/Rough/Ornate Titanwrought on EVERY mob
--    in a level band at 2 percent, which is how this line drops today. The crafting system makes
--    the gear craft-only by design, and leaving these on means the molds, the materials and the
--    coin cost are all optional. Ornate (35-47) is switched off with them: nothing above Rough is
--    part of this system, and it was already unreachable at a level cap of 30.
UPDATE global_loot SET enabled=0 WHERE id IN (1,2,3,4);
