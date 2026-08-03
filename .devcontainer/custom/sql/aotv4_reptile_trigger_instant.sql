-- ============================================================================================
-- AoTv4 -- the reptile proc payload must be an INSTANT, not a 1 tick buff (2026-08-03)
--
-- ============================================================================================
-- ⚠️⚠️ THE SLOT 1 HEAL HAS NEVER FIRED. OUR OWN ANTI-EXPLOIT GUARD SUPPRESSES IT.
-- ============================================================================================
-- Reported as "Skin of the Lizard isn't proccing at all during its duration" immediately after
-- aotv4_reptile_trigger_slot8.sql cleared the inherited 393 HealOverTime. Both facts are the same
-- bug, and the slot 8 removal only revealed it:
--
--   zone/spell_effects.cpp, SE_CurrentHPOnce (SPA 79):
--       if (buffslot >= 0 && effect_value > 0) { break; }
--
-- That guard exists because a heal-once component inside a BUFF heals every time the buff is
-- (re)applied -- click it off, recast, get healed, forever. It suppresses the heal whenever the
-- spell lands in a buff slot.
--
-- 43350-43355 carry `buffdurationformula 3, buffduration 1`, inherited from stock 8009, so they DO
-- take a buff slot -- and their slot 1 SPA 79 heal has been silently suppressed for the entire life
-- of the line. Every point of healing players ever saw came from slot 8's SPA 100 HealOverTime,
-- which is a different SPA and is not caught by that guard. It was also a flat 393 on all six tiers
-- (see aotv4_reptile_trigger_slot8.sql), which is why the tier ladder never appeared to do anything.
--
-- ⚠️ The duration only existed to carry that HoT. A proc PAYLOAD has no reason to persist: it fires,
-- heals, and is done. Making it instant is the correct shape, not a workaround -- and it is what
-- lets the tier ladder in slot 1 (20/45/85/140/215/300) finally be the thing that pays out.
--
-- 📌 With no buff slot, GetActSpellHealing now takes its INSTANT branch, so these heals become
-- eligible for critical heals and item Heal Amount like any other direct heal. That is a real
-- change in behaviour and is intended; the previous over-time branch skipped both.
--
-- ⚠️ NOT applied to the sloth triggers (43356-43361). Those are genuine 3 tick DEBUFFS -- SPA 11
-- attack speed on the attacker -- and need their duration. Their effect is not SPA 79 and was never
-- suppressed.
-- ⚠️ NOT applied to the kindred procs (43374-43379): already `buffduration 0`, already instant.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY, and it must be rebuilt with WORLD DOWN:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

UPDATE spells_new
   SET buffdurationformula = 0, buffduration = 0
 WHERE id BETWEEN 43350 AND 43355;

-- Verification: instant payloads, tier ladder intact, sloth untouched.
SELECT id, name, buffdurationformula AS dur_formula, buffduration AS dur,
       effectid1, effect_base_value1 AS heal
FROM spells_new WHERE id BETWEEN 43350 AND 43361 ORDER BY id;
