-- @aotv4-migration
-- description: 2026_08_31_raid_tomes_and_augs
-- check: SELECT loottable_id FROM loottable_entries WHERE lootdrop_id = 200057 AND loottable_id = 121
-- condition: empty
-- author: Claude
-- notes: Three extras on every raid boss, on top of the guaranteed molds and signature piece.
--   (1) Tomes of Insight (§44). Radiant is the tier that matches a level 30 raider, Etched is included
--   at the same weight as a consolation -- a tome ABOVE your band is refused, one below is merely
--   worth less, so both are usable at the cap and only Radiant is worth the slot.
--   (2) Augments, TIER 3 ONLY, per the request. The delve aug bands are T1 147600-147615, T2
--   147616-147715, T3 147716-147915 (§26), so this pool is the T3 band and nothing else. A raid should
--   not hand out the tier a rung-1 delve chest already gives.
--   (3) Weapon augments -- a curated eight from the stock weapon-only set at reclevel 35 or under.
--   ⚠️⚠️ WEAPON AUGS AND DELVE AUGS DO NOT GO IN THE SAME SOCKETS, AND THAT IS WORTH KNOWING BEFORE
--   TUNING EITHER. Delve augs are augtype 255 (slot types 1-8) so they fit the 1/2/3 sockets that
--   craft_sockets puts on crafted gear, weapons included -- 896 crafted weapons have them. The stock
--   weapon augs here fit ONLY slot types 4-6, which craft_sockets never writes; their homes are the
--   11,617 stock items that already carry a weapon socket. So these are for gear a player FINDS, and
--   the delve augs are for gear a player MAKES.
--   📌 Probabilities, not guarantees: molds and the signature piece are what a kill owes you, these
--   are what makes two kills differ. Rolled per eligible looter like everything else (§31).
--   ⚠️ All three are droplimit 1, which also keeps them out of the AoTv4 named-bonus drop -- that
--   picks the highest droplimit, and every boss has a droplimit 2+ pool that outranks these.

DELETE FROM loottable_entries WHERE lootdrop_id IN (200055,200056,200057);
DELETE FROM lootdrop_entries  WHERE lootdrop_id IN (200055,200056,200057);
DELETE FROM lootdrop          WHERE id         IN (200055,200056,200057);
INSERT INTO lootdrop (id, name) VALUES
  (200055,'AoTv4 raid Tomes of Insight'),
  (200056,'AoTv4 raid augments tier 3'),
  (200057,'AoTv4 raid weapon augments');

-- Tomes: Radiant and Etched only. Worn is a level 1-10 band reward and has no place on a raid boss.
INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200055, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id IN (147967, 147968);

-- Tier 3 delve augments, all 200, equal weight.
INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200056, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 147716 AND 147915;

-- Weapon augments.
INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200057, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id IN (
    46312,  -- Hive Defender Orb of Stinging Fury   ac 30 str 14
    46180,  -- Blackened Lava Rock                  hp 53 mana 43
    51725,  -- Wulfenite Segment                    hp 29 str 6
    51731,  -- Anatase Gem                          ac 16 sta 9
    51714,  -- Dioptase Shard                       hp 21 mana 24
    51726,  -- Wulfenite Gem                        mana 27 str 7
    51724,  -- Wulfenite Shard                      ac 12 sta 5
    51729   -- Anatase Shard                        hp 24
);

-- Attached to all three bosses. Phinigel 10831, Velketor 121, Mayong 110043.
INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, droplimit, mindrop, probability) VALUES
  (10831, 200055, 1, 1, 1, 40), (110043, 200055, 1, 1, 1, 40), (121, 200055, 1, 1, 1, 40),
  (10831, 200056, 1, 1, 1, 60), (110043, 200056, 1, 1, 1, 60), (121, 200056, 1, 1, 1, 60),
  (10831, 200057, 1, 1, 1, 30), (110043, 200057, 1, 1, 1, 30), (121, 200057, 1, 1, 1, 30);
