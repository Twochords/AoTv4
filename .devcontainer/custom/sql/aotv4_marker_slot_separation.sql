-- ============================================================================================
-- AoTv4 -- give each custom line its OWN marker slot so the lines stop fighting (2026-08-03)
--
-- ============================================================================================
-- ⚠️⚠️ THE STACKING MARKERS SEPARATED TIERS WITHIN A LINE AND COLLIDED BETWEEN LINES
-- ============================================================================================
-- Reported as "Thirst and Skin lines are overwriting each other now", immediately after
-- aotv4_custom_line_stacking.sql. That script gave four lines an ascending SPA 44 marker so a higher
-- tier would replace a lower one -- and put THREE of them in the SAME SLOT with the SAME values:
--
--     reptile  43300-05   slot 1   100,200,300,400,500,600
--     sloth    43306-11   slot 1   100,200,300,400,500,600
--     thirst   43342-47   slot 1   100,200,300,400,500,600
--     kindred  43330-35   slot 2   100,200,300,400,500,600
--
-- Mob::CheckStackConflict walks the two spells SLOT BY SLOT and only compares when the effect ids at
-- the SAME INDEX match. Three lines sharing slot 1 therefore compare against each other:
--     Skin of the Serpent (300) up, cast Rising Thirst (200)  -> 200 < 300 -> BLOCKED
--     Rising Thirst (200) up, cast Skin of the Serpent (300)  -> 300 > 200 -> OVERWRITES
-- Exactly the reported behaviour, and it is worse than the bug it replaced: before, the lines merely
-- failed to replace their own tiers; now they actively evict one another.
--
-- ============================================================================================
-- THE FIX: THE SLOT INDEX IS WHAT ISOLATES LINES, NOT THE VALUE
-- ============================================================================================
-- Two spells only compare when their effect ids match at the same index. Put each line's marker in a
-- DIFFERENT slot and the comparison never happens between lines, while within a line -- same slot,
-- ascending value -- it still works exactly as intended.
--
--     reptile  -> slot 1   (its real effect, SPA 323, is in slot 4)
--     sloth    -> slot 2   (SPA 323 in slot 3)
--     kindred  -> slot 3   (SPA 85  in slot 1)
--     thirst   -> slot 4   (no real effects at all -- inert marker buff, Lua pays the heal)
--
-- ⚠️ No line's marker slot holds one of its own real effects, and no two lines share a marker slot.
-- ⚠️ Values are deliberately left at 100..600 for all four. They do not need to differ once the slots
-- are separated, and making them differ would be worse if a slot ever DID collide: one line would
-- then permanently outrank another instead of the collision being obvious.
-- ⚠️⚠️ ANY FUTURE LINE NEEDING A MARKER MUST TAKE ITS OWN SLOT -- slot 5 next. Reusing 1-4 silently
-- recreates this bug, and it presents as two unrelated spells cancelling each other, which is not
-- something anyone would think to look for in a stacking marker.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY:
--   world + zones down  ->  cd build/bin && ./shared_memory  ->  world up
-- ============================================================================================

-- sloth: slot 1 -> slot 2   (slot 2 currently a 10:0 filler; slot 3 holds its SPA 323 proc)
UPDATE spells_new
   SET effectid2 = 44, effect_base_value2 = effect_base_value1, effect_limit_value2 = 0,
       formula2 = 100, max2 = 0,
       effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 0, max1 = 0
 WHERE id BETWEEN 43306 AND 43311 AND effectid1 = 44;

-- kindred: slot 2 -> slot 3  (slot 1 holds its SPA 85 weapon proc)
UPDATE spells_new
   SET effectid3 = 44, effect_base_value3 = effect_base_value2, effect_limit_value3 = 0,
       formula3 = 100, max3 = 0,
       effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 0, max2 = 0
 WHERE id BETWEEN 43330 AND 43335 AND effectid2 = 44;

-- thirst: slot 1 -> slot 4   (the row stays mechanically inert; Lua still pays the heal)
UPDATE spells_new
   SET effectid4 = 44, effect_base_value4 = effect_base_value1, effect_limit_value4 = 0,
       formula4 = 100, max4 = 0,
       effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 0, max1 = 0
 WHERE id BETWEEN 43342 AND 43347 AND effectid1 = 44;

-- reptile keeps slot 1 -- nothing to do.

-- Verification: one marker per line, each in its own slot, still ascending 100..600.
SELECT CASE WHEN id BETWEEN 43300 AND 43305 THEN 'reptile'
            WHEN id BETWEEN 43306 AND 43311 THEN 'sloth'
            WHEN id BETWEEN 43330 AND 43335 THEN 'kindred'
            WHEN id BETWEEN 43342 AND 43347 THEN 'thirst' END AS line,
       CASE WHEN effectid1 = 44 THEN 1 WHEN effectid2 = 44 THEN 2
            WHEN effectid3 = 44 THEN 3 WHEN effectid4 = 44 THEN 4 END AS marker_slot,
       COUNT(*) AS tiers
FROM spells_new WHERE id BETWEEN 43300 AND 43347
GROUP BY line, marker_slot HAVING line IS NOT NULL ORDER BY marker_slot;
