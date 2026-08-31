-- @aotv4-migration
-- description: 2026_08_31_mayong_loot_pool
-- check: SELECT loottable_id FROM loottable_entries WHERE lootdrop_id = 200051 AND loottable_id = 110043
-- condition: empty
-- author: Claude
-- notes: Mayong dropped exactly ONE item -- 52543 Dark Master's Blade, reclevel 80 -- so once molds
--   moved onto his table (v158) he was effectively a mold vendor with a trophy. Phinigel by contrast
--   has a 6-item primary pool averaging reclevel 28, which is the shape a raid boss should have.
--   This gives him a pool of thirteen items drawn from Castle Mistmoore's OWN drops at reclevel 20-25,
--   so the loot reads as belonging to the place rather than being invented for him.
--   droplimit 2 / mindrop 1 = one or two gear items per eligible looter, on top of the 2 molds.
--   📌 These are BASE ids, so NPC::ResolveTierDrop still rolls each one 5 percent Mythic / 25 percent
--   Hallowed (§10) -- the pool does not need tier entries of its own and must not have them.
--   📌 It also restores the AoTv4 named-bonus drop to something sensible on this boss. That bonus
--   picks the highest-droplimit NON-reward pool, and with only a droplimit-1 entry to choose from it
--   had nothing worth granting; now it grants a piece of gear, which is what it is for.
--   ⚠️ Dark Master's Blade is deliberately KEPT. It is his signature drop and reclevel 80 makes it a
--   trophy rather than an upgrade, which is a fair thing for a raid boss to leave behind.

DELETE FROM lootdrop_entries WHERE lootdrop_id = 200051;
DELETE FROM lootdrop WHERE id = 200051;
INSERT INTO lootdrop (id, name) VALUES (200051, 'AoTv4 Mayong Mistmoore gear');

-- Equal weight across the pool; `chance` is a WEIGHT in the dominant weighted loot mode.
INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200051, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id IN (
    4404,   -- Chestplate of the Dark Flame   ac 35 hp 18
    9310,   -- Crested Mistmoore Shield       ac 29
    4300,   -- Crested Helm                   ac 27 hp 13
    2319,   -- Black Silk Gloves              ac 23
    10163,  -- Platinum Skull Ring            ac 17
    1408,   -- Nightshade Wreath              ac 14
    1407,   -- Cape of Midnight Mist          ac 12 hp 34
    1410,   -- Bloodstone Eyepatch            ac 12 mana 32
    10165,  -- Diamondine Earring             hp 40
    1409,   -- Hooded Black Cloak             hp 37
    7317,   -- Glowing Iron Pike              26/36
    6402,   -- Gem-Encrusted Scepter          18/28 mana 8
    7318    -- Sacrificial Dagger             13/21 mana 9
);

DELETE FROM loottable_entries WHERE loottable_id = 110043 AND lootdrop_id = 200051;
INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, droplimit, mindrop, probability)
VALUES (110043, 200051, 1, 2, 1, 100);
