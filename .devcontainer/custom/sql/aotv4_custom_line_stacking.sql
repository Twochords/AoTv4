-- ============================================================================================
-- AoTv4 -- stop the custom BUFF lines stacking with themselves (2026-08-02)
--
-- ============================================================================================
-- ⚠️⚠️ PROC EFFECTS ARE IGNORED FOR STACKING BY DESIGN, SO A PROC-ONLY BUFF NEVER CONFLICTS
-- ============================================================================================
-- Reported in play: "Kindred Spark and Kindred Ember are both on someone." They were, and so was
-- every other tier -- all six could be up at once, each firing its own proc.
--
-- Mob::CheckStackConflict compares two buffs SLOT BY SLOT, but before comparing it skips any effect
-- in IsEffectIgnoredInStacking (common/spdat.cpp) -- "big ol' list according to the client". That
-- list contains exactly the effects these lines are built on:
--     85  WeaponProc        <- kindred
--     323 DefensiveProc     <- reptile, sloth
--     79  CurrentHPOnce     <- the trigger spells
-- So a buff whose ONLY real effect is a proc has nothing left to compare, no conflict is found, and
-- both land. The thirst line is the same story for a different reason: its slots are ALL 254, so
-- there was never anything to compare in the first place.
--
-- ⚠️ THIS IS NOT WHAT spellgroup/rank FIXES. Those are read only by
-- Client::GetHighestScribedSpellinSpellGroup and Client::LoadSpellGroupCache (zone/spells.cpp:6203,
-- :6224) and govern which rank sits in the SPELLBOOK. Buff stacking is decided separately, here.
-- aotv4_reptile_sloth_spellgroup.sql fixed the spellbook half; this fixes the buff half. Both were
-- wrong, and neither substitutes for the other.
--   📌 An earlier reading of this code concluded the reptile tiers "resolve correctly by accident"
--   because SPA 323's base value is the trigger spell id and those ascend with the tier. That was
--   WRONG -- the comparison never happens at all, because 323 is skipped before any value is read.
--
-- ============================================================================================
-- THE FIX: ONE INERT MARKER EFFECT WHOSE VALUE ASCENDS WITH THE TIER
-- ============================================================================================
-- The generic comparison at the end of the slot loop already does exactly what a tier ladder wants:
--     if (sp2_value < sp1_value) return -1;   // incoming is weaker -> rejected
--     if (sp2_value != sp1_value) ...         // incoming is stronger -> overwrites
-- So all a line needs is ONE non-ignored effect, in the SAME slot on every tier, carrying a value
-- that ascends. A higher tier then replaces a lower one and a lower tier is refused, with no other
-- machinery. Recasting the SAME tier is unaffected -- identical spell ids are resolved earlier, by
-- the `spellid1 == spellid2` branch.
--
-- ⚠️ SPA 44 (Lycanthropy) is the marker, which is the STOCK convention -- live's Promised Renewal
-- line does the same thing, and our 43324-29 promised line inherited it (slot 2, base = the heal).
-- It is genuinely inert: spell_effects.cpp:3207 lands it in the do-nothing group and nothing in
-- bonuses.cpp reads it. It is also NOT in IsEffectIgnoredInStacking and NOT an
-- IsBardOnlyStackEffect (that is BardAEDot only), so it is actually compared. Both were checked.
--
-- ⚠️⚠️ THE MARKER IS `rank * 100`, DELIBERATELY NOT THE TIER'S MAGNITUDE. Promised uses its heal
-- amount, which couples stacking to tuning: retune a heal and you silently change which tiers
-- replace which. A pure ordinal cannot drift, so these lines can be rebalanced freely.
--
-- ⚠️ The 148/149 StackingCommand pair is NOT used. It was the obvious answer and it is the wrong
-- one here: 149 (Overwrite) is tested before 148 (Block) inside the loop, so the two have to sit in
-- a specific slot ORDER to produce a ladder rather than "first cast wins" -- and Promised's flat
-- 8500 threshold gives the latter. The generic comparison above needs neither.
--
-- ============================================================================================
-- SLOT CHOICE -- must be the same index on every tier of a line, and must not disturb a real effect
-- ============================================================================================
--   reptile 43300-05   slot 1   (was SPA 10 base 0, filler; the proc stays in slot 4)
--   sloth   43306-11   slot 1   (was SPA 10 base 0, filler; the proc stays in slot 3)
--   kindred 43330-35   slot 2   (was 254; the proc stays in slot 1)
--   thirst  43342-47   slot 1   (was 254; the row stays mechanically inert, see below)
--
-- ⚠️ The thirst line stays an INERT MARKER BUFF exactly as section 5 describes -- Lycanthropy does
-- nothing, so the Lua in aotv4_thirst.lua is still what pays the heal. Nothing about that changes.
-- ⚠️ NOT applied to: mark (43336-41), whose SPA 121 already ascends and is not an ignored effect;
-- promised (43324-29), which carries its own stock 44+148+149 arrangement; and moonfire/sinew
-- (43312-23), which are instant nukes with no duration and therefore nothing to stack.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY. These UPDATEs are inert until:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

-- Skin of the Reptile -- Newt / Lizard / Serpent / Crocodile / Basilisk / Drake
UPDATE spells_new
SET effectid1 = 44, effect_base_value1 = (id - 43300 + 1) * 100, effect_limit_value1 = 0,
    formula1 = 100, max1 = 0
WHERE id BETWEEN 43300 AND 43305;

-- Lingering Sloth
UPDATE spells_new
SET effectid1 = 44, effect_base_value1 = (id - 43306 + 1) * 100, effect_limit_value1 = 0,
    formula1 = 100, max1 = 0
WHERE id BETWEEN 43306 AND 43311;

-- Kindred -- Spark / Ember / Flame / Blaze / Radiance / Beacon  (proc stays in slot 1)
UPDATE spells_new
SET effectid2 = 44, effect_base_value2 = (id - 43330 + 1) * 100, effect_limit_value2 = 0,
    formula2 = 100, max2 = 0
WHERE id BETWEEN 43330 AND 43335;

-- Thirst -- still an inert marker buff; Lua still pays the heal
UPDATE spells_new
SET effectid1 = 44, effect_base_value1 = (id - 43342 + 1) * 100, effect_limit_value1 = 0,
    formula1 = 100, max1 = 0
WHERE id BETWEEN 43342 AND 43347;

-- Verification: every tier of every buff line now carries exactly one ascending marker, and the
-- lines that were already fine are shown untouched for comparison.
SELECT CASE
         WHEN id BETWEEN 43300 AND 43305 THEN 'reptile  (fixed)'
         WHEN id BETWEEN 43306 AND 43311 THEN 'sloth    (fixed)'
         WHEN id BETWEEN 43330 AND 43335 THEN 'kindred  (fixed)'
         WHEN id BETWEEN 43342 AND 43347 THEN 'thirst   (fixed)'
         WHEN id BETWEEN 43324 AND 43329 THEN 'promised (already ok)'
         WHEN id BETWEEN 43336 AND 43341 THEN 'mark     (already ok)'
       END AS line,
       COUNT(*) AS tiers,
       GROUP_CONCAT(
         CASE WHEN effectid1 = 44 THEN effect_base_value1
              WHEN effectid2 = 44 THEN effect_base_value2
              WHEN effectid1 = 121 THEN ABS(effect_base_value1) END
         ORDER BY id) AS ascending_marker
FROM spells_new
WHERE id BETWEEN 43300 AND 43347
  AND (id NOT BETWEEN 43312 AND 43323)
GROUP BY line ORDER BY line;
