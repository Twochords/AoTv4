-- ============================================================================================
-- AoTv4 -- give the REPTILE and SLOTH lines a spellgroup + ascending rank (2026-08-02)
--
-- ============================================================================================
-- THEY WERE THE ONLY TWO BUFF LINES LEFT AT spellgroup 0 / rank 0
-- ============================================================================================
-- Of the eight custom lines in 43300-43347, the four written later all carry a spellgroup equal to
-- their first id and ranks 1..6 ascending:
--     43324-29 promised   spellgroup 43324   rank 1,2,3,4,5,6
--     43330-35 kindred    spellgroup 43330   rank 1,2,3,4,5,6
--     43336-41 mark       spellgroup 43336   rank 1,2,3,4,5,6
--     43342-47 thirst     spellgroup 43342   rank 1,2,3,4,5,6
-- The four written first do not:
--     43300-05 reptile    spellgroup 0       rank 0,0,0,0,0,0   <- fixed here
--     43306-11 sloth      spellgroup 0       rank 0,0,0,0,0,0   <- fixed here
--     43312-17 moonfire   spellgroup 0       rank 0,0,0,0,0,0   <- left alone, see below
--     43318-23 sinew      spellgroup 0       rank 0,0,0,0,0,0   <- left alone, see below
--
-- ⚠️ MOONFIRE AND SINEW ARE DELIBERATELY NOT TOUCHED. They are instant nukes with no duration and
-- nothing to collapse -- a spellgroup would only ever hide the lower tiers from the spellbook, and
-- there is a real reason to keep a cheaper nuke available. Reptile and sloth are self BUFFS whose
-- tiers are strictly better as they go up, so holding six of them is only clutter.
--
-- ⚠️⚠️ THIS IS ABOUT THE SPELLBOOK, NOT ABOUT BUFF STACKING. spellgroup/rank is read by
-- Client::GetHighestScribedSpellinSpellGroup and Client::LoadSpellGroupCache (zone/spells.cpp:6203,
-- :6224), and BOTH explicitly skip spellgroup 0 -- so these two lines were invisible to the whole
-- mechanism and all six tiers sat in the book side by side. Whether two BUFFS stack is decided
-- separately, effect by effect, in Mob::CheckStackConflict.
--   📌 For the reptile line that comparison happens to resolve correctly already: every tier carries
--   SPA 323 in the SAME slot (4) and its base value is the trigger spell id, which ascends with the
--   tier (43350..43355). So a higher tier overwrites a lower and a lower is blocked by a higher --
--   by accident rather than by design. Do not read this script as having fixed that; it did not
--   need fixing, and nothing here changes it.
--
-- ⚠️ Ranks are 1..6, not the live 1/5/10 convention. GetHighestScribedSpellinSpellGroup only ever
-- compares ranks with `<`, so any strictly ascending sequence works, and 1..6 is what the other four
-- lines already use. Keep them consistent rather than "correct".
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY. This UPDATE is inert until:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

-- Skin of the Reptile: Newt / Lizard / Serpent / Crocodile / Basilisk / Drake (levels 8-58)
UPDATE spells_new SET spellgroup = 43300, `rank` = id - 43299 WHERE id BETWEEN 43300 AND 43305;

-- Lingering Sloth: six tiers, same shape (levels 8-58)
UPDATE spells_new SET spellgroup = 43306, `rank` = id - 43305 WHERE id BETWEEN 43306 AND 43311;

-- Verification: two groups, six tiers each, ranks 1..6 with no duplicates.
SELECT spellgroup,
       COUNT(*)                AS tiers,
       COUNT(DISTINCT `rank`)  AS distinct_ranks,
       MIN(`rank`)             AS lowest,
       MAX(`rank`)             AS highest,
       GROUP_CONCAT(`rank` ORDER BY id) AS ranks_in_id_order
FROM spells_new
WHERE id BETWEEN 43300 AND 43311
GROUP BY spellgroup;
