-- ============================================================================================
-- AoTv4 -- make the region unlocks actually pay out (2026-08-02)
--
-- ============================================================================================
-- ⚠️⚠️ EVERY REGION UNLOCK WAS FIRING AT 1 PERCENT
-- ============================================================================================
-- `custom_achievement_rewards.chance` is rolled out of TEN THOUSAND, not a hundred:
--     achievement_manager.cpp:1495
--     AND (r.`chance` >= 10000 OR FLOOR(RAND() * 10000) < r.`chance`)
-- The five "Ending" achievements (9100001-9100005, "Fall N times at the height of your power") were
-- created with `chance = 100`, which reads as 1 percent -- so dying at the level cap queued a region
-- unlock roughly ONE TIME IN A HUNDRED. Every other reward row on the server uses 10000:
--     coin 10000 | live_item_request 10000 | scribe_spell 10000 | title_text 10000 | region_choice 100
-- so this was these five rows specifically, not a convention.
--
-- ⚠️ The failure is silent and looks like the wrong thing. The achievement still COMPLETES and still
-- announces itself -- only the reward row is skipped -- so it reads as "the achievement is broken" or
-- "Alessa won't give me anything", not as a dice roll. Nothing logs the miss.
--
-- ============================================================================================
-- 📌 WHY LEVEL 30 DOES *NOT* GRANT A REGION
-- ============================================================================================
-- An earlier pass added a `region_choice` reward to achievement 1003 "Level 30". That was wrong and
-- is removed here. `era_system.M.HARD_CAP` is **30**, so "reach level 30" and "die at the height of
-- your power" are the SAME milestone -- you cannot die at the cap without having already hit 30.
-- The level 30 achievement is therefore a strict SUBSET of `9100001 The First Ending`, and rewarding
-- both handed out two regions for one moment.
--
-- The intended ladder is:
--     level 1        -> ONE free region, picked from Alessa  (aotv4_regions.M.grant_start)
--     reach 30, die  -> one more, via The First Ending
--     die at cap x2..x5 -> one more each, via the Second..Fifth Ending
-- so the free starter is granted on connect in Lua, and every LATER region comes from dying at the
-- cap. Level 30 is the gate that makes a death count, not a reward in itself.
--
-- ⚠️ The achievement tables are read from the DB, NOT shared memory -- a zone restart applies this,
-- no world down and no ./shared_memory rebuild.
-- ============================================================================================

-- Make the five death-at-cap unlocks actually pay out.
UPDATE custom_achievement_rewards
   SET chance = 10000
 WHERE reward_type = 'region_choice' AND chance < 10000;

-- Remove the level 30 region reward: it duplicates The First Ending (see above).
DELETE FROM custom_achievement_rewards
 WHERE achievement_id = 1003 AND reward_type = 'region_choice';

-- Verification: exactly five region_choice rewards, all guaranteed, all on the Ending achievements.
SELECT r.achievement_id, a.name, r.amount, r.chance, r.auto_claim, r.enabled
  FROM custom_achievement_rewards r
  JOIN custom_achievements a ON a.id = r.achievement_id
 WHERE r.reward_type = 'region_choice'
 ORDER BY r.achievement_id;
