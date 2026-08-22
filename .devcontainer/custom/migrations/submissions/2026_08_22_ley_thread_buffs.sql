-- @aotv4-migration
-- description: 2026_08_22_ley_thread_buffs
-- check: SELECT `value` FROM `db_str` WHERE `id` = 44758 AND `type` = 6
-- condition: empty
-- match:
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: Ley Tap stacks were an entity variable, so nothing showed on the buff bar.

-- ⚠️⚠️ THE STACK COUNT WAS AN ENTITY VARIABLE AND THEREFORE INVISIBLE. A Wizard gathering threads for
-- Overload had one chat line and nothing else -- no icon, no duration, no way to check mid fight how
-- many were banked. Reported as "Ley Tap is not providing a visible buff for Overload".
--
-- 📌 Three rows, one per count, is the section 17b Shield Wall pattern: a buff row cannot display a
-- number, so the COUNT has to be encoded in which row is up. Named in words rather than numerals so
-- the bar reads plainly.
--
-- ⚠️⚠️ AND THE BUFF IS NOW THE STATE, NOT A MIRROR OF IT. aotv4_class_abilities reads the stack count
-- back off these buffs instead of keeping its own entity variable. A variable plus a buff is two
-- sources of truth that drift the moment one expires and the other does not -- the threads would
-- fade off the bar while Overload still paid out, or the reverse. There is now nothing to desync.
--
-- ⚠️ Cloned from 44753 Reaving Fervor, the existing inert marker in this band -- NOT from the
-- discipline template 4499, which carries EndurUpkeep 10 (v128) and would put the drain straight
-- back. Every column that has bitten this feature is set explicitly below anyway.
-- ⚠️ classes 255 everywhere: these are applied BY Lua, never cast by a player, so a class gate would
-- only ever refuse them.
DELETE FROM spells_new WHERE id IN (44756, 44757, 44758);

DROP TEMPORARY TABLE IF EXISTS aotv4_ley_tmpl;
CREATE TEMPORARY TABLE aotv4_ley_tmpl LIKE spells_new;
INSERT INTO aotv4_ley_tmpl SELECT * FROM spells_new WHERE id = 44753;
UPDATE aotv4_ley_tmpl SET
    id = 44756, name = 'One Ley Thread', descnum = 44756,
    new_icon = 164, spellanim = 0, CastingAnim = 44, player_1 = 'PLAYER_1',
    IsDiscipline = -1, EndurCost = 0, EndurUpkeep = 0, EndurTimerIndex = 0, mana = 0,
    targettype = 6, goodEffect = 1, buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0, recast_time = 0, recovery_time = 0, cast_time = 0,
    effectid1 = 254, effect_base_value1 = 0, formula1 = 100, max1 = 0;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
UPDATE aotv4_ley_tmpl SET id = 44757, name = 'Two Ley Threads',   descnum = 44757;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
UPDATE aotv4_ley_tmpl SET id = 44758, name = 'Three Ley Threads', descnum = 44758;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_ley_tmpl;

-- ⚠️ A spell with no db_str type 6 row renders BLANK with no error (section 51). Written here, in the
-- same migration as the rows.
DELETE FROM db_str WHERE id IN (44756, 44757, 44758) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44756, 6, 'One ley thread gathered. Your next Overload deals 3 times your level in extra damage.'),
 (44757, 6, 'Two ley threads gathered. Your next Overload deals 6 times your level in extra damage.'),
 (44758, 6, 'Three ley threads gathered. Your next Overload deals 9 times your level in extra damage.');
