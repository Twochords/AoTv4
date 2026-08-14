-- aotv4_spell_books.sql -- Tomes of Insight
--
-- A consumable that grants ONE extra reward pick without levelling you up. Three tiers, one per
-- level band (1-10 / 11-20 / 21-30), each usable ONLY inside its own band. Clicking one queues a
-- normal level-up offer; you take a reward from it, or DECLINE it and have your reroll price cut
-- (25 / 50 / 100 percent by tier). The tome is spent either way.
--
-- Behaviour lives in `lua_modules/aotv4_spell_books.lua` + `quests/items/1479xx.lua`. This file
-- creates only the three items and the inert spell their click rides on.
--
-- ⚠️⚠️ `items` AND `spells_new` ARE SHARED MEMORY. Applying this migration at world boot is NOT
-- enough: stop the stack, run ./shared_memory, restart. Everything else here needs only that.

-- ---------------------------------------------------------------------------------------------
-- 1. Spell 44328 "Insight" -- the inert carrier the item click rides on.
--
-- ⚠️⚠️ THE BOOK NEEDS A REAL clickeffect EVEN THOUGH THE SPELL DOES NOTHING. EVENT_ITEM_CLICK fires
-- inside Handle_OP_ItemVerifyRequest, and the RoF2 client will not SEND that packet for an item it
-- believes has no click at all -- the classic three-layer problem (client display / client send /
-- server execute). So the spell exists purely to make the item right-clickable.
--
-- ⚠️⚠️ IT REALLY DOES GET CAST, AND ONLY SOMETIMES. Straight after firing the Lua event the handler
-- re-reads the slot: `inst = m_inv[slot_id]; if (!inst) return;` (client_packet.cpp:9542). Consuming
-- the LAST tome in a stack therefore makes the engine bail before casting, while consuming one of
-- several leaves the item in place and the spell fires. That asymmetry is exactly why this must be
-- genuinely inert rather than merely unused -- it is a live code path roughly every time.
--
-- ⚠️ Cloned from 43380 (a Shield Wall marker) VIA A TEMP TABLE so all ~236 columns stay byte
-- identical to a known-good row -- never hand-list the columns.
DROP TEMPORARY TABLE IF EXISTS aotv4_tome_spell;
CREATE TEMPORARY TABLE aotv4_tome_spell AS SELECT * FROM spells_new WHERE id = 43380;

UPDATE aotv4_tome_spell SET
    id                  = 44328,
    name                = 'Insight',
    -- no duration and no formula: 43380 is a 10-minute marker buff and this must leave nothing behind
    buffduration        = 0,
    buffdurationformula = 0,
    -- instant, free, no lockout. A recast would put an item click on a cooldown nobody asked for.
    cast_time           = 0,
    recovery_time       = 0,
    recast_time         = 0,
    mana                = 0,
    targettype          = 6,      -- ST_Self
    goodEffect          = 1,
    spell_category      = -99,
    -- ⚠️ 255 in every class column marks this as a proc/click spell rather than a player spell. The
    -- pool generator excludes it anyway (it takes id < 10000 plus the 43300-43349 band), but the
    -- convention is what tells the next reader this is never scribable.
    classes1=255, classes2=255, classes3=255, classes4=255, classes5=255, classes6=255,
    classes7=255, classes8=255, classes9=255, classes10=255, classes11=255, classes12=255,
    classes13=255, classes14=255, classes15=255, classes16=255;

DELETE FROM spells_new WHERE id = 44328;
INSERT INTO spells_new SELECT * FROM aotv4_tome_spell;
DROP TEMPORARY TABLE aotv4_tome_spell;

-- ---------------------------------------------------------------------------------------------
-- 2. The three tomes, 147966-147968.
--
-- ⚠️ The reserved band is 147966-147979. 147969+ are free if a fourth tier is ever wanted; the
-- DELETE below sweeps the whole band so a re-run cannot strand an older layout's tail.
--
-- ⚠️ Cloned from 147921 (Ink of the Lost) via a temp table -- picked because it is already a
-- stackable, no-slot, custom-band consumable, so the fields that matter are right to begin with.
DROP TEMPORARY TABLE IF EXISTS aotv4_tome_items;
CREATE TEMPORARY TABLE aotv4_tome_items AS SELECT * FROM items WHERE id = 147921;

UPDATE aotv4_tome_items SET
    -- ⚠️⚠️ nodrop = 0 IS "NO DROP". The flag is INVERTED (client_packet.cpp:10755 tests NoDrop == 0
    -- for "no vendor value"), and getting it backwards here would make an earned tome tradeable --
    -- which on a server where everyone resets to level 1 constantly means a capped character can
    -- stockpile tier 1 tomes and hand a fresh alt a dozen extra picks. Loosening this later is easy;
    -- tightening it after people hold stockpiles is not.
    nodrop      = 0,
    norent      = 1,          -- opposite polarity again: 1 = permanent, 0 = deleted on logout
    -- ⚠️⚠️ maxcharges = -1 IS "UNLIMITED", NOT 0. Zero reads as a SPENT consumable and the click
    -- answers "Item is out of charges." with nothing in the row looking wrong -- the exact trap that
    -- left the tradeskill masks silently dead (see the v30 note in section 32).
    -- The tome is consumed by Lua, not by charges, so that the offer can be built BEFORE the item is
    -- destroyed; charge-based consumption would spend it even when there is nothing left to offer.
    maxcharges  = -1,
    clicktype   = 1,          -- ItemEffectClick: usable from a bag, not only when equipped
    clickeffect = 44328,
    casttime    = 0,
    stackable   = 1,
    stacksize   = 20,
    itemtype    = 17,         -- misc; deliberately NOT 11 (Book), which invites the reader UI
    slots       = 0,
    loregroup   = 0,          -- not lore: you are meant to carry several
    classes     = 65535,
    races       = 65535,
    deity       = 0,
    reqlevel    = 0,          -- the level BAND is enforced in Lua, not on the item
    reclevel    = 0,
    price       = 0,
    sellrate    = 0,
    -- ⚠️ Clear the socket columns explicitly. Section 32 records the tradeskill band shipping with
    -- augslot types inherited from whatever stock row it cloned; a clone carries whatever the source
    -- had, and a socketed consumable is meaningless.
    augtype     = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0;

DELETE FROM items WHERE id BETWEEN 147966 AND 147979;

-- ⚠️ Icons are deliberately DIFFERENT per tier. All three tomes are otherwise identical on the
-- inventory grid, and "all the augments look the same" was a real complaint about the delve set.
UPDATE aotv4_tome_items SET id = 147966, Name = 'Worn Tome of Insight',    icon = 777;
INSERT INTO items SELECT * FROM aotv4_tome_items;

UPDATE aotv4_tome_items SET id = 147967, Name = 'Etched Tome of Insight',  icon = 865;
INSERT INTO items SELECT * FROM aotv4_tome_items;

UPDATE aotv4_tome_items SET id = 147968, Name = 'Radiant Tome of Insight', icon = 1357;
INSERT INTO items SELECT * FROM aotv4_tome_items;

DROP TEMPORARY TABLE aotv4_tome_items;
