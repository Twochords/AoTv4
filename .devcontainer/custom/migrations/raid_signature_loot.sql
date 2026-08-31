-- @aotv4-migration
-- description: 2026_08_31_raid_signature_loot
-- check: SELECT loottable_id FROM loottable_entries WHERE lootdrop_id = 200054 AND loottable_id = 121
-- condition: empty
-- shared-memory: yes
-- author: Claude
-- notes: Two things, both from play.
--   (1) "It cant have a level 80 rec level or nobody will use it". Dark Master's Blade (52543) sat at
--   reclevel 80 on a level 30 cap, so the client scaled it to ~37 percent and Mayong's signature drop
--   was strictly worse than his trash. It is reclevel 80 because the item scaler prices a drop off the
--   dropping npc's STOCK level, and Mayong's row is level 85 -- we scale him to 30 at spawn, which the
--   scaler never sees. Re-priced to 30 with a ratio of 2.22, just above the best 1H in the 25-30 band
--   (2.10) as befits a raid drop.
--   ⚠️ It is the ONLY over-level item worth touching on the three tables. The other 30 are research
--   components -- Words of, Compendium pages, Blue Diamond -- where reclevel is meaningless and which
--   drop from up to 11,125 other npcs, so re-pricing them would reach far outside the raids.
--   (2) Nine signature items, three per boss, so each encounter has loot that is its own rather than
--   the zone's generic drops. Reclevel 30, No Drop, all classes and races. Cloned from real items via
--   a temp table so all ~285 columns stay byte-identical to something the engine already accepts --
--   never hand-list the column set (§5).
--   ⚠️ NO augment slots and NO click/proc/focus effects: the sockets belong to CRAFTED gear (§32) and
--   an inherited effect from whatever was cloned is exactly the trap §5 records for spell rows.
--   ⚠️ Placed in their OWN lootdrop per boss at droplimit 1 / probability 100, so each kill yields one
--   signature piece per eligible looter on top of the zone pool and the molds.
--   📌 Base ids, so ResolveTierDrop still rolls each 5 percent Mythic / 25 percent Hallowed (§10).

-- (1) Mayong's signature weapon, usable at the level cap.
UPDATE items SET reclevel = 30, damage = 100, delay = 45 WHERE id = 52543;

-- (2) The nine. A temp table shaped exactly like `items` is the only safe way to clone a row.
DROP TEMPORARY TABLE IF EXISTS tw_clone;
CREATE TEMPORARY TABLE tw_clone LIKE items;
DELETE FROM items WHERE id BETWEEN 148600 AND 148608;

-- Phinigel head: cloned from 4300 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 4300;
UPDATE tw_clone SET
    id = 148600, Name = 'Tidecaller''s Circlet', slots = 4, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 30, hp = 45, mana = 60, endur = 0, damage = 0, delay = 0,
    astr = 0, asta = 6, adex = 0, aagi = 0, aint = 9, awis = 9, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Phinigel back: cloned from 1407 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 1407;
UPDATE tw_clone SET
    id = 148601, Name = 'Abyssal Scale Cloak', slots = 256, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 25, hp = 60, mana = 25, endur = 0, damage = 0, delay = 0,
    astr = 5, asta = 7, adex = 0, aagi = 5, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Phinigel primary: cloned from 7317 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 7317;
UPDATE tw_clone SET
    id = 148602, Name = 'Phinigel''s Coral Trident', slots = 8192, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 5, hp = 30, mana = 30, endur = 0, damage = 88, delay = 42,
    astr = 10, asta = 5, adex = 8, aagi = 0, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Mayong ring: cloned from 10163 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 10163;
UPDATE tw_clone SET
    id = 148603, Name = 'Signet of Mistmoore', slots = 98304, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 20, hp = 50, mana = 20, endur = 0, damage = 0, delay = 0,
    astr = 9, asta = 8, adex = 6, aagi = 0, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Mayong chest: cloned from 4404 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 4404;
UPDATE tw_clone SET
    id = 148604, Name = 'Shroud of the Dark Master', slots = 131072, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 45, hp = 70, mana = 30, endur = 0, damage = 0, delay = 0,
    astr = 10, asta = 10, adex = 0, aagi = 6, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Mayong primary: cloned from 7317 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 7317;
UPDATE tw_clone SET
    id = 148605, Name = 'Mayong''s Bloodfang', slots = 8192, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 5, hp = 35, mana = 0, endur = 0, damage = 92, delay = 43,
    astr = 12, asta = 6, adex = 9, aagi = 0, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Velketor face: cloned from 1410 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 1410;
UPDATE tw_clone SET
    id = 148606, Name = 'Glacial Focus', slots = 8, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 22, hp = 40, mana = 70, endur = 0, damage = 0, delay = 0,
    astr = 0, asta = 5, adex = 0, aagi = 0, aint = 11, awis = 8, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Velketor chest: cloned from 4404 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 4404;
UPDATE tw_clone SET
    id = 148607, Name = 'Labyrinth Warden''s Breastplate', slots = 131072, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 50, hp = 80, mana = 25, endur = 0, damage = 0, delay = 0,
    astr = 11, asta = 11, adex = 0, aagi = 7, aint = 0, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;

-- Velketor primary: cloned from 7317 so every one of the ~285 columns stays byte-identical to a real item.
DELETE FROM tw_clone;
INSERT INTO tw_clone SELECT * FROM items WHERE id = 7317;
UPDATE tw_clone SET
    id = 148608, Name = 'Velketor''s Frozen Scepter', slots = 8192, reclevel = 30, reqlevel = 0,
    classes = 65535, races = 65535, deity = 0, loregroup = 0, nodrop = 1, norent = 1,
    ac = 6, hp = 40, mana = 55, endur = 0, damage = 95, delay = 44,
    astr = 8, asta = 6, adex = 7, aagi = 0, aint = 8, awis = 0, acha = 0,
    heroic_str = 0, heroic_sta = 0, heroic_dex = 0, heroic_agi = 0, heroic_int = 0, heroic_wis = 0,
    attack = 0, strikethrough = 0, accuracy = 0, spelldmg = 0, healamt = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0,
    clickeffect = 0, clicktype = 0, proceffect = 0, worneffect = 0, focuseffect = 0, scrolleffect = 0,
    price = 0, sellrate = 0;
INSERT INTO items SELECT * FROM tw_clone;
DROP TEMPORARY TABLE IF EXISTS tw_clone;

-- One pool per boss so each drops only its own signature pieces.
DELETE FROM loottable_entries WHERE lootdrop_id IN (200052,200053,200054);
DELETE FROM lootdrop_entries  WHERE lootdrop_id IN (200052,200053,200054);
DELETE FROM lootdrop          WHERE id         IN (200052,200053,200054);
INSERT INTO lootdrop (id, name) VALUES
  (200052,'AoTv4 Phinigel signature'), (200053,'AoTv4 Mayong signature'), (200054,'AoTv4 Velketor signature');

INSERT INTO lootdrop_entries (lootdrop_id, item_id, item_charges, equip_item, chance, disabled_chance, multiplier, npc_min_level, npc_max_level)
SELECT 200052, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148600 AND 148602
UNION ALL SELECT 200053, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148603 AND 148605
UNION ALL SELECT 200054, id, 1, 0, 1, 0, 1, 0, 0 FROM items WHERE id BETWEEN 148606 AND 148608;

INSERT INTO loottable_entries (loottable_id, lootdrop_id, multiplier, droplimit, mindrop, probability)
VALUES (10831, 200052, 1, 1, 1, 100),
       (110043,200053, 1, 1, 1, 100),
       (121,   200054, 1, 1, 1, 100);
