-- @aotv4-migration
-- description: 2026_08_31_raid_mold_loot
-- check: SELECT loottable_id FROM loottable_entries WHERE lootdrop_id = 200050 AND loottable_id = 110043
-- condition: empty
-- author: Claude
-- notes: Rough Titanwrought molds move from a bespoke Lua grant onto the bosses' own loot tables.
--   aotv4_raid.M.grant_molds handed them out with SummonItemExact because its comment held that
--   "corpse loot is ONE physical reward shared by the group". That is true of `global_loot` and FALSE
--   of an npc's own loottable: under AoT:IndividualLoot, NPC::Death (attack.cpp:3190) loops every
--   allowed looter and rolls GetLoottableID() once PER PLAYER, owner-stamping each roll. Corpse loot
--   was already individual, so the special case was solving a problem that did not exist.
--   Moving it makes the drop rate DATA rather than a constant, and -- the real win -- takes molds off
--   the Lua death hook entirely. That hook silently paid nothing for the life of the feature because
--   global_npc passed a nil corpse, and loot rolled from C++ at death cannot fail that way.
--   mindrop/droplimit 2 at probability 100 reproduces MOLD_DROPS = 2 exactly, so this is a refactor
--   and not a retune. Make it probabilistic later, once the fights have been played.

-- The 70 Rough molds, equal weight. `chance` is a WEIGHT in the dominant weighted loot mode, so 1
-- across the set is a uniform pick.
DELETE FROM lootdrop_entries WHERE lootdrop_id = 200050;
DELETE FROM lootdrop WHERE id = 200050;
INSERT INTO lootdrop (id, name) VALUES (200050, 'AoTv4 Rough Titanwrought Molds (raid)');

INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200050, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148340 AND 148409;

-- Attached to all three raid bosses' own tables: Phinigel 10831, Velketor 121, Mayong 110043.
-- droplimit/mindrop 2 = exactly two molds, probability 100 = every kill, per eligible looter.
DELETE FROM loottable_entries WHERE lootdrop_id = 200050;
INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, droplimit, mindrop, probability)
VALUES (10831, 200050, 1, 2, 2, 100),
       (121,   200050, 1, 2, 2, 100),
       (110043,200050, 1, 2, 2, 100);
