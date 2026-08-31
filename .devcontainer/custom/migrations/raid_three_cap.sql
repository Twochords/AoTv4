-- @aotv4-migration
-- description: 2026_08_31_raid_three_item_cap
-- check: SELECT loottable_id FROM loottable_entries WHERE lootdrop_id = 200062 AND loottable_id = 121
-- condition: empty
-- author: Claude
-- notes: "None of the raid mobs should drop more than 3 things at a time."
--   A kill was paying up to NINE per eligible looter: 2 molds, a signature piece, 1-2 zone gear, the
--   stock signature weapon, and three independent probability pools -- plus the AoTv4 named bonus,
--   which is granted OUTSIDE the tables and so could not be capped by tuning them.
--   ⚠️⚠️ SEPARATE LOOTTABLE ENTRIES CANNOT BE CAPPED IN AGGREGATE. Each entry rolls independently,
--   so "at most 3" is unreachable by lowering droplimits -- seven entries of droplimit 1 is still
--   seven items. The only way to bound the total is to have FEWER ENTRIES, so everything that is not
--   a mold is merged into ONE pool per boss at droplimit 1.
--   Result is exactly: 2 molds + 1 spoil = 3, every kill, per eligible looter.
--   Weights inside the spoils pool, `chance` being a weight in the dominant weighted mode:
--     signature 100 each (3)   zone gear 30 each   Tomes 50 each (2)
--     weapon augs 20 each (8)  tier 3 delve augments 1 each (200)
--   ⚠️ The augments are weight 1 DELIBERATELY. There are 200 of them against a few dozen of
--   everything else, so at equal weight they would be nearly the whole pool and the signature pieces
--   would effectively never drop.
--   ⚠️ The old separate pools 200051 and 200055-200057 are detached from every boss here. The
--   lootdrops themselves are left in place -- they cost nothing and are the record of what was tried.
--   📌 The named bonus drop is suppressed for raid bosses in zone/loot.cpp, keyed on the `raid_enc`
--   entity variable the raid module already stamps. It cannot be done in data: that grant is made in
--   C++ after the table has been walked.

-- Detach everything except the molds, then rebuild as one pool per boss.
DELETE FROM loottable_entries WHERE loottable_id IN (10831,110043,121) AND lootdrop_id <> 200050;

-- Phinigel: one pool holding everything that is not a mold.
DELETE FROM lootdrop_entries WHERE lootdrop_id = 200060;
DELETE FROM lootdrop WHERE id = 200060;
INSERT INTO lootdrop (id, name) VALUES (200060, 'AoTv4 Phinigel spoils');
-- signature pieces, the headline
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200060, id, 1, 0, 100, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148600 AND 148602;
-- the zone gear
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200060, id, 1, 0, 30, 0, 1, 0, 0 FROM items WHERE id IN (1253,2447,7510,10035,10036,10037,10048,10049,10050,10051,10053,10374,10375,11569,11730,11731,11732,11733,11734,11735,11736,11776,11779,11780,11781,11782,11783,11806,11807,11808,11809,11865,11866,11867,11868,11869,11870,14337,16976,33791,36282);
-- Tomes of Insight
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200060, id, 1, 0, 50, 0, 1, 0, 0 FROM items WHERE id IN (147967,147968);
-- weapon augments
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200060, id, 1, 0, 20, 0, 1, 0, 0 FROM items WHERE id IN (46312,46180,51725,51731,51714,51726,51724,51729);
-- tier 3 delve augments, low weight EACH because there are 200 of them
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200060, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 147716 AND 147915;
INSERT INTO loottable_entries (loottable_id,lootdrop_id,multiplier,droplimit,mindrop,probability)
VALUES (10831, 200060, 1, 1, 1, 100);

-- Mayong: one pool holding everything that is not a mold.
DELETE FROM lootdrop_entries WHERE lootdrop_id = 200061;
DELETE FROM lootdrop WHERE id = 200061;
INSERT INTO lootdrop (id, name) VALUES (200061, 'AoTv4 Mayong spoils');
-- signature pieces, the headline
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200061, id, 1, 0, 100, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148603 AND 148605;
-- the zone gear
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200061, id, 1, 0, 30, 0, 1, 0, 0 FROM items WHERE id IN (1407,1408,1409,1410,2319,4300,4404,6402,7317,7318,9310,10163,10165,52543);
-- Tomes of Insight
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200061, id, 1, 0, 50, 0, 1, 0, 0 FROM items WHERE id IN (147967,147968);
-- weapon augments
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200061, id, 1, 0, 20, 0, 1, 0, 0 FROM items WHERE id IN (46312,46180,51725,51731,51714,51726,51724,51729);
-- tier 3 delve augments, low weight EACH because there are 200 of them
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200061, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 147716 AND 147915;
INSERT INTO loottable_entries (loottable_id,lootdrop_id,multiplier,droplimit,mindrop,probability)
VALUES (110043, 200061, 1, 1, 1, 100);

-- Velketor: one pool holding everything that is not a mold.
DELETE FROM lootdrop_entries WHERE lootdrop_id = 200062;
DELETE FROM lootdrop WHERE id = 200062;
INSERT INTO lootdrop (id, name) VALUES (200062, 'AoTv4 Velketor spoils');
-- signature pieces, the headline
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200062, id, 1, 0, 100, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148606 AND 148608;
-- the zone gear
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200062, id, 1, 0, 30, 0, 1, 0, 0 FROM items WHERE id IN (1117,25094,25095,25096,25097,25296,25297,25298,25299,25570,25571,25572,25579);
-- Tomes of Insight
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200062, id, 1, 0, 50, 0, 1, 0, 0 FROM items WHERE id IN (147967,147968);
-- weapon augments
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200062, id, 1, 0, 20, 0, 1, 0, 0 FROM items WHERE id IN (46312,46180,51725,51731,51714,51726,51724,51729);
-- tier 3 delve augments, low weight EACH because there are 200 of them
INSERT INTO lootdrop_entries (lootdrop_id,item_id,item_charges,equip_item,chance,disabled_chance,multiplier,npc_min_level,npc_max_level)
SELECT 200062, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 147716 AND 147915;
INSERT INTO loottable_entries (loottable_id,lootdrop_id,multiplier,droplimit,mindrop,probability)
VALUES (121, 200062, 1, 1, 1, 100);