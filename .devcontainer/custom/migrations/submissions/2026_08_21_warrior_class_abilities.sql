-- @aotv4-migration
-- description: 2026_08_21_warrior_class_abilities
-- check: SELECT `value` FROM `db_str` WHERE `id` = 44702 AND `type` = 6
-- condition: empty
-- match:
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: Warrior tier 1/2/3 as DISCIPLINES, the reference build for the other fifteen classes.
-- notes: Cloned via temp table from stock rows so all ~236 columns stay byte-identical.

-- ⚠️⚠️ EndurTimerIndex IS LOAD BEARING AND MUST NOT BE 0.
-- A discipline's recast lives EXCLUSIVELY in `pTimerDisciplineReuseStart + timer_id`. CastSpell's
-- per-spell branch is guarded by `&& !spells[spell_id].is_discipline`, so pTimerSpellStart is never
-- started for one. Leaving this 0 would put every class ability on ONE shared cooldown, alongside
-- the 87 stock discs that already use slot 0.
-- ⚠️⚠️ ONLY 0-10 ARE VALID. pTimerDisciplineReuseEnd is 24 and pTimerCombatAbility is 25, so
-- timer_id 11 collides with Kick/Bash, 12 with Tiger Claw, 13 with Begging.
-- 📌 Slots chosen by how far the lowest stock discipline using them sits above our cap:
--      tier 1 -> 5  (lowest stock disc there is level 68, only 19 rows)
--      tier 2 -> 6  (level 66, 25 rows)
--      tier 3 -> 2  (level 56, 39 rows)
--    Not sequential on purpose. 1/4/7/8/9 all carry discs reachable at level 35 or below.
-- 📌 Three slots serve all sixteen classes: a character is only ever one class, so no one can hold
--    two tier 1s. Do NOT allocate per class -- there are only 11 slots in total.

-- ⚠️ EndurCost is 1, not 0 and not the real cost. IsDiscipline requires mana = 0 AND EndurCost > 0,
-- so 0 would leave the row out of the Combat Abilities window entirely. The real level-scaled cost
-- is charged by Lua, because EndurCost is a flat int column and cannot express `N x level`.

-- ⚠️ Scoped to exactly the three ids this migration creates, so a half-applied run (the check keys
-- on the LAST thing written, the 44702 description) re-runs cleanly instead of dying on a duplicate
-- key. Never widen it to the band: 44700-44747 is reserved for all sixteen classes.
DELETE FROM spells_new WHERE id IN (44700, 44701, 44702);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- tier 1: Cleaving Blow
-- Cloned from 4667 Rebuke of the Ikaav: single target, short recast, no buff.
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44700, name = 'Cleaving Blow', descnum = 44700,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, targettype = 5, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 87, spellanim = 0, CastingAnim = 44,
    -- inert marker: the swing, its damage and its hate are paid by Lua through
    -- DoSpecialAttackDamage, which is what gives it mitigation, avoidance and the section 22 charge.
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 0, buffdurationformula = 0, numhits = 0, numhitstype = 0,
    classes1 = 1,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;

-- ---------------------------------------------------------------- tier 3: Broad Cleave
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44702, name = 'Broad Cleave', descnum = 44702,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, targettype = 5, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 89, spellanim = 0, CastingAnim = 44,
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 0, buffdurationformula = 0, numhits = 0, numhitstype = 0,
    classes1 = 10,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;

-- ---------------------------------------------------------------- tier 2: Bulwark
-- Cloned from 4499 Defensive Discipline: a self buff with a real duration.
-- ⚠️⚠️ NO `numhits`, AND THAT IS DELIBERATE. numhitstype 6 (Incoming Hit Successes) would give a
-- free client-side charge counter, but the engine spends the last charge and FADES THE BUFF inside
-- CheckNumHitsRemaining (attack.cpp:4621), which runs BEFORE EVENT_DAMAGE_TAKEN (attack.cpp:4689) in
-- the same Mob::CommonDamage call. The Lua payload would then find no buff on the fifth hit and
-- absorb nothing -- an ability that advertises five and delivers four, silently. The charges are
-- counted in aotv4_class_abilities.lua instead, which also fades the buff when they run out.
-- 📌 buffduration is a CAP on the formula, not an alternative to it (spells.cpp:3300), so
-- formula 11 with duration 10 is a flat 10 tics -- 60 seconds -- at every level. The charges are the
-- real limit; the duration only stops it being pre-cast minutes before a pull.
-- 📌 UseDiscipline refuses a self-buff discipline while any disc buff is already up
-- (`HasDiscBuff()`, effects.cpp:977), so Bulwark cannot be refreshed to top its charges back up.
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44701, name = 'Bulwark', descnum = 44701,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, targettype = 6, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 1, spellanim = 0, CastingAnim = 44,
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 10, buffdurationformula = 11, numhits = 0, numhitstype = 0,
    classes1 = 5,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors -- eighteen heals shipped
-- that way. Written in the same migration as the rows, deliberately.
-- ⚠️ No literal percent sign: the description path is printf-style and eats it as a format token.
DELETE FROM db_str WHERE id IN (44700, 44701, 44702) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44700, 6, 'Cleave into your target with your equipped weapon, striking for weapon damage and seizing its attention.'),
 (44701, 6, 'Brace behind your guard. The next 5 melee attacks against you are each reduced by a tenth of your armor class, and any blow weaker than that is turned aside entirely.'),
 (44702, 6, 'Sweep everything in front of you. Each target struck shortens the recovery of Bulwark by 3 seconds, to a maximum of 9. Generates no additional threat.');
