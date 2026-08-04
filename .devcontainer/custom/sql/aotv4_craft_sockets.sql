-- ============================================================================================
-- AoTv4 -- every craftable wearable gets augment sockets by TIER (2026-08-04)
-- ============================================================================================
-- Shipped as migration v13. The standard for the whole gear system:
--
--     native = 1 socket        Hallowed = 2 sockets        Mythic = 3 sockets
--
-- Applied to the 8,889 wearables that some ENABLED recipe produces, plus their two tier clones.
-- Hard set, not max(existing, computed): a predictable "every Mythic has three" is the point, and
-- per-item exceptions would make it unreadable.
--
-- ⚠️⚠️ SOCKET TYPE 1 ON ALL THREE, WHICH IS WHAT MAKES EVERY DELVE AUGMENT FIT EVERY SLOT.
-- The engine's test is  (1 << (augslotNtype - 1)) & aug->AugType  (zone/inventory.cpp:356), and the
-- delve augments (147600-147915) all carry AugType 255 -- bits 0-7, i.e. slot types 1 through 8.
-- Type 1 (General: Single Stat) is therefore accepted by all of them, in all three positions.
-- ⚠️ Do NOT "improve" this by giving the three sockets different types to tier-lock them: that was
-- considered and rejected, because augments must fit in all three slots.
--
-- ⚠️⚠️ ORNAMENTATION SLOTS ARE THE SAME SIX COLUMNS, WHICH IS THE ONLY REASON THIS IS FIDDLY.
-- There is no separate ornament field -- an ornament slot IS an augment slot whose type is 20
-- (Ornamentation) or 21 (Special Ornamentation). On these items they sit almost entirely in
-- position 2 (3,396 of them) and position 3 (658), which is exactly where the Hallowed and Mythic
-- stat sockets have to go. A blind overwrite of positions 1-3 would silently delete the ability to
-- ornament crafted gear.
-- Positions 4-6 are COMPLETELY UNUSED across all 8,889 items, so the ornament slot is relocated
-- there and nothing is lost: every item ends up with its full stat sockets AND its ornament slot.
--
-- ⚠️ `items` IS SHARED MEMORY:  world + zones down -> cd build/bin && ./shared_memory -> world up.
-- ⚠️ MUST RUN AFTER aotv4_gear_tiers.sql. That script clones with INSERT ... SELECT *, so tier rows
-- inherit whatever the base has -- generate tiers first, then socket all three tiers explicitly, or
-- the Hallowed/Mythic rows just copy the base's single socket.
-- ⚠️ This also sockets DROPPED copies of craftable items. Unavoidable and correct: a dropped Mythic
-- and a crafted Mythic are the same row, so "crafted gear has sockets" can only ever be expressed
-- as "this item has sockets".
-- ============================================================================================

-- The working set: distinct wearable outputs of enabled recipes, and their two tier clones.
-- ⚠️ successcount > 0 is what marks a recipe ENTRY as an output rather than a component.
-- ⚠️⚠️ ONE temp table holding only the BASE ids, joined at three offsets afterwards. It is NOT
-- grown by selecting from itself: MariaDB cannot open a TEMPORARY table twice in one statement, so
-- `INSERT INTO t SELECT ... FROM t` fails outright with "Can't reopen table". Everything below joins
-- the base set against `items` at +0 / +300000 / +600000 instead.
DROP TEMPORARY TABLE IF EXISTS aotv4_craft_base;
CREATE TEMPORARY TABLE aotv4_craft_base (
  item_id INT NOT NULL PRIMARY KEY
) ENGINE=MEMORY;

INSERT INTO aotv4_craft_base (item_id)
SELECT DISTINCT i.id
FROM tradeskill_recipe_entries e
JOIN items i             ON i.id = e.item_id
JOIN tradeskill_recipe r ON r.id = e.recipe_id
WHERE e.successcount > 0 AND i.slots > 0 AND r.enabled = 1;

-- ---------------------------------------------------------------- 1. rescue the ornament slots
-- Remember, per item, whether it had an ornamentation slot anywhere in positions 1-3. The TYPE is
-- kept (20 and 21 are different slots and a type-21 ornament will not go in a type-20 socket).
DROP TEMPORARY TABLE IF EXISTS aotv4_craft_orn;
CREATE TEMPORARY TABLE aotv4_craft_orn (
  item_id   INT NOT NULL PRIMARY KEY,
  orn_type  INT NOT NULL,
  orn_type2 INT NOT NULL DEFAULT 0
) ENGINE=MEMORY;

-- ⚠️ Three separate inserts, one per tier offset, for the same "cannot reopen a temp table" reason.
INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
       CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type
            WHEN i.augslot2type IN (20,21) THEN i.augslot2type
            ELSE i.augslot3type END,
       -- the SECOND ornament, if the item carried two. 358 of these items do, and keeping only the
       -- first would quietly halve their cosmetic options -- types 20 and 21 are different sockets,
       -- so a type-21 ornament will not go into a type-20 slot.
       CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
            WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
       CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type
            WHEN i.augslot2type IN (20,21) THEN i.augslot2type
            ELSE i.augslot3type END,
       -- the SECOND ornament, if the item carried two. 358 of these items do, and keeping only the
       -- first would quietly halve their cosmetic options -- types 20 and 21 are different sockets,
       -- so a type-21 ornament will not go into a type-20 slot.
       CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
            WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id + 300000
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
       CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type
            WHEN i.augslot2type IN (20,21) THEN i.augslot2type
            ELSE i.augslot3type END,
       -- the SECOND ornament, if the item carried two. 358 of these items do, and keeping only the
       -- first would quietly halve their cosmetic options -- types 20 and 21 are different sockets,
       -- so a type-21 ornament will not go into a type-20 slot.
       CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
            WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
            ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id + 600000
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

-- ---------------------------------------------------------------- 2. write the stat sockets
-- Positions 1-3 become type 1 and visible, as many as the tier allows; the remainder of 1-3 is
-- CLEARED so a lower tier cannot keep a socket it should not have.
UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id
   SET i.augslot1type = 1, i.augslot1visible = 1,
       i.augslot2type = 0, i.augslot2visible = 0,
       i.augslot3type = 0, i.augslot3visible = 0;

UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id + 300000
   SET i.augslot1type = 1, i.augslot1visible = 1,
       i.augslot2type = 1, i.augslot2visible = 1,
       i.augslot3type = 0, i.augslot3visible = 0;

UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id + 600000
   SET i.augslot1type = 1, i.augslot1visible = 1,
       i.augslot2type = 1, i.augslot2visible = 1,
       i.augslot3type = 1, i.augslot3visible = 1;

-- ---------------------------------------------------------------- 3. put the ornament slots back
-- Into positions 4 and 5, both empty on every item in this set.
-- ⚠️ Position 5 is only written when a SECOND ornament was actually found, so an item that had one
-- keeps one -- this must not invent a socket that never existed.
UPDATE items i
  JOIN aotv4_craft_orn o ON o.item_id = i.id
   SET i.augslot4type    = o.orn_type,
       i.augslot4visible = 1,
       i.augslot5type    = CASE WHEN o.orn_type2 > 0 THEN o.orn_type2 ELSE i.augslot5type END,
       i.augslot5visible = CASE WHEN o.orn_type2 > 0 THEN 1 ELSE i.augslot5visible END;

DROP TEMPORARY TABLE IF EXISTS aotv4_craft_base;
DROP TEMPORARY TABLE IF EXISTS aotv4_craft_orn;

-- ---------------------------------------------------------------- verification
SELECT 'native, expect 1 socket'   AS check_name,
       SUM(i.augslot1type=1) AS s1, SUM(i.augslot2type>0) AS s2, SUM(i.augslot3type>0) AS s3, COUNT(*) AS items
FROM items i WHERE i.id IN (
  SELECT DISTINCT e.item_id FROM tradeskill_recipe_entries e JOIN items x ON x.id=e.item_id
  JOIN tradeskill_recipe r ON r.id=e.recipe_id WHERE e.successcount>0 AND x.slots>0 AND r.enabled=1)
UNION ALL
SELECT 'hallowed, expect 2', SUM(i.augslot1type=1), SUM(i.augslot2type=1), SUM(i.augslot3type>0), COUNT(*)
FROM items i WHERE i.id IN (
  SELECT DISTINCT e.item_id + 300000 FROM tradeskill_recipe_entries e JOIN items x ON x.id=e.item_id
  JOIN tradeskill_recipe r ON r.id=e.recipe_id WHERE e.successcount>0 AND x.slots>0 AND r.enabled=1)
UNION ALL
SELECT 'mythic, expect 3', SUM(i.augslot1type=1), SUM(i.augslot2type=1), SUM(i.augslot3type=1), COUNT(*)
FROM items i WHERE i.id IN (
  SELECT DISTINCT e.item_id + 600000 FROM tradeskill_recipe_entries e JOIN items x ON x.id=e.item_id
  JOIN tradeskill_recipe r ON r.id=e.recipe_id WHERE e.successcount>0 AND x.slots>0 AND r.enabled=1);
