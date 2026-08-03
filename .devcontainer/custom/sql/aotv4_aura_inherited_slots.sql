-- ============================================================================================
-- AoTv4 -- clear the inherited Champion's Aura effects from the 16 class auras (2026-08-02)
--
-- ============================================================================================
-- ⚠️⚠️ EVERY CLASS AURA GRANTED +300 AC AND +2 PERCENT PROC CHANCE, AND NONE OF IT WAS DESIGNED
-- ============================================================================================
-- The 16 auras were cloned from stock 8468/8469 `Champion's Aura` (a level 68 WARRIOR AA). The clone
-- wrote each class's signature effect into slot 1 and left the template's own effects in place:
--
--     slot 11   SPA 200  ProcChance   = 2
--     slot 12   SPA 1    ArmorClass   = 300
--
-- So all sixteen carried both, on top of their signature. Found by the audit that followed the
-- reptile trigger's slot 8 (custom/sql/aotv4_reptile_trigger_slot8.sql) -- the SAME failure, in a
-- second place, for the same reason: only the slot being overridden was checked.
--
-- ⚠️ THE SCALE IS THE POINT. Champion's Aura is tuned for level 68 raid play. These are granted at
-- LEVEL 1 by each class's "First Blood" achievement and are group-shared to 60 feet, so a flat 300 AC
-- landed on a whole group of level 1 characters. It also flattened the design: the per-class
-- signatures are 2-3 percent modifiers and small procs, so an identical 300 AC on all sixteen made
-- the auras functionally the same spell wearing sixteen names.
--
-- ⚠️⚠️ ON `43573 Spirit Chill` IT WAS BUFFING THE ENEMY. Five of the six aura procs carry the same
-- inherited pair but have buffduration 0, so nothing can persist and they are inert. Spirit Chill --
-- the Shaman defensive proc -- has buffduration 3, targettype 5, goodEffect 0, i.e. it is cast ON THE
-- CREATURE THAT HIT YOU. It slowed the attacker and handed it +300 AC and +2 percent proc chance for
-- three ticks. That half of this is a plain bug rather than a balance call.
--
-- 📌 Cleared on the inert five as well. They do nothing there today, but only because those rows
-- happen to have no duration -- give one a duration later and the bug comes back silently.
--
-- ⚠️ NOT touched: the aura CAST spells (43500-43515) and the aura npcs. Audited and clean; the cast
-- spell is a bare SPA 351 and carries nothing inherited.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY. Inert until:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

UPDATE spells_new
   SET effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 0, max11 = 0,
       effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 0, max12 = 0
 WHERE id BETWEEN 43550 AND 43565      -- the 16 aura effect spells
    OR id BETWEEN 43570 AND 43575;     -- the 6 aura procs

-- Verification: no high slot is populated on any aura row, and each keeps its signature in slot 1.
SELECT CASE WHEN id <= 43565 THEN 'aura EFFECT' ELSE 'aura PROC' END AS band,
       COUNT(*) AS rows_checked,
       SUM(effectid7 <>254 OR effectid8 <>254 OR effectid9 <>254
        OR effectid10<>254 OR effectid11<>254 OR effectid12<>254) AS rows_still_dirty,
       SUM(effectid1 <> 254) AS rows_with_signature
FROM spells_new
WHERE id BETWEEN 43550 AND 43565 OR id BETWEEN 43570 AND 43575
GROUP BY band;
