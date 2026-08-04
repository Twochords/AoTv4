-- ============================================================================================
-- AoTv4 -- tradeskill achievements auto-grant the mastery AAs, and a Salvage ladder (2026-08-04)
-- ============================================================================================
-- Shipped as migration v12 (database_update_manifest_custom.h). This standalone copy exists for
-- reading and for a manual re-run; the migration is what actually applies it on live.
--
-- Two ladders, both driven by the NEW `grant_aa` achievement reward type (zone/achievement_manager.cpp):
--
--   PER SKILL   skill 50 / 100 / 150  ->  rank 1 / 2 / 3 of that tradeskill's Mastery AA
--   AGGREGATE   ALL ten at 50/100/150/200/250/300  ->  rank 1..6 of Salvage
--
-- ⚠️⚠️ THE PER-SKILL ACHIEVEMENTS ALREADY EXIST -- DO NOT CREATE THEM. All twelve tradeskills
-- already ship six achievements each in category 600 "Tradeskill Mastery" (72 rows, the figure
-- recorded in the achievements section of CLAUDE.md), keyed `4<skill_id><level>` -- 455050 is
-- "Fishing 50", 469300 is "Pottery 300". They already carry the right `skill` objective and a
-- title reward. This script only ADDS a grant_aa reward alongside the existing title.
--
-- ⚠️⚠️ SKILL 68 IS JEWELCRAFTING AND 69 IS POTTERY, not the other way round. Checked against the
-- achievement rows themselves rather than assumed; getting it backwards would hand out Pottery
-- Mastery for jewelry and vice versa, and nothing would ever report an error.
--
-- ⚠️ FISHING (55) AND RESEARCH (58) ARE DELIBERATELY EXCLUDED, from BOTH ladders. Neither has a
-- Mastery AA in the game -- there simply is no native one to grant -- and including them in the
-- aggregate would gate the entire Salvage chain behind two skills that have no reward of their own.
-- Their own six achievements still exist and still award their titles; they just grant no AA.
--
-- ⚠️ Progress is measured on RAW skill: Client::SetSkill passes the stored value to
-- achievement_manager.ProcessSkill, so the +20 item and +30 illusion bonuses (which are applied at
-- the tradeskill roll, not to the stored skill) CANNOT be used to shortcut these ladders. Earned,
-- not buffed.
--
-- ⚠️⚠️ aa_ability.enabled = 0 MEANS THE AA DOES NOT EXIST AT RUNTIME. zone/aa.cpp:1823 loads only
-- `enabled = 1`, so every AA below has to be switched on or grant_aa fails with "not loaded".
-- Most of the tradeskill masteries and Salvage ship disabled.
-- ⚠️ grant_only = 1 on all of them: that flag hides an AA from the native window until it has been
-- trained, so these can ONLY arrive via the achievement and can never be bought with AA points.
-- grant_aa passes ignore_cost = true, which is what lets it through that gate.
-- ⚠️ AAs load at ZONE BOOT and are NOT shared memory, so this needs a zone restart -- no
-- ./shared_memory run.
-- ============================================================================================

-- ---------------------------------------------------------------- 1. switch the AAs on
-- 49 Alchemy - 103 Poison - 324 Blacksmithing - 325 Baking - 326 Brewing - 327 Fletching
-- 328 Pottery - 329 Tailoring - 330 Salvage - 575 Tinkering - 576 Jewel Craft
-- ⚠️ 575 Tinkering ships classes=15639 (Gnome-ish) and 56 is a SECOND, Enchanter-only Jewel Craft
-- Mastery that 576 supersedes. 56 is left disabled on purpose -- enabling both would show the
-- player two identically named AAs.
UPDATE aa_ability
   SET enabled = 1, grant_only = 1, classes = 65535
 WHERE id IN (49, 103, 324, 325, 326, 327, 328, 329, 330, 575, 576);

-- ---------------------------------------------------------------- 2. per-skill mastery ladders
-- One grant_aa reward per (skill, tier). The rank is ABSOLUTE -- "be at rank N" -- which is what
-- makes it safe for RecheckAutomatic to re-fire the achievement without climbing the ladder.
-- Built from a values list rather than 30 hand-written rows so the skill->AA pairing is readable
-- in one place and cannot drift between tiers.
DELETE FROM custom_achievement_rewards WHERE id BETWEEN 1000 AND 1099;

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1000 + (m.seq * 3) + t.rank_no - 1,
       (4 * 100000) + (m.skill_id * 1000) + (t.rank_no * 50),
       'grant_aa', m.aa_id, t.rank_no, 100, '', 1, 1,
       CONCAT(m.aa_name, ' rank ', t.rank_no), '', 1, 20, UNIX_TIMESTAMP()
FROM (
  SELECT 0 seq, 56 skill_id, 103 aa_id, 'Poison Mastery'        aa_name UNION ALL
  SELECT 1,     57,          575,       'Tinkering Mastery'             UNION ALL
  SELECT 2,     59,           49,       'Alchemy Mastery'               UNION ALL
  SELECT 3,     60,          325,       'Baking Mastery'                UNION ALL
  SELECT 4,     61,          329,       'Tailoring Mastery'             UNION ALL
  SELECT 5,     63,          324,       'Blacksmithing Mastery'         UNION ALL
  SELECT 6,     64,          327,       'Fletching Mastery'             UNION ALL
  SELECT 7,     65,          326,       'Brewing Mastery'               UNION ALL
  SELECT 8,     68,          576,       'Jewel Craft Mastery'           UNION ALL
  SELECT 9,     69,          328,       'Pottery Mastery'
) m
CROSS JOIN (SELECT 1 rank_no UNION ALL SELECT 2 UNION ALL SELECT 3) t
-- only attach to an achievement that actually exists, so a renumber upstream fails loudly (0 rows)
-- rather than silently creating a reward pointing at nothing.
WHERE EXISTS (
  SELECT 1 FROM custom_achievements a
   WHERE a.id = (4 * 100000) + (m.skill_id * 1000) + (t.rank_no * 50)
);

-- ---------------------------------------------------------------- 3. the aggregate Salvage ladder
-- Six achievements, 470050 / 470100 / ... / 470300. Each requires ALL TEN mastery tradeskills at
-- that value and grants the next rank of Salvage.
-- ⚠️ Salvage has exactly SIX ranks (997, 998, 999, 1113, 1114, 1115 -- a chain that is NOT
-- contiguous, walk next_id), which is precisely the six thresholds. That is why the aggregate uses
-- 50-step intervals all the way to 300 while the per-skill ladders stop at 150 (three ranks).
-- ⚠️ An achievement completes only when EVERY non-optional objective is complete
-- (AchievementManager::TryCompleteAchievement), so the ten objectives below are an AND.
DELETE FROM custom_achievement_rewards    WHERE id BETWEEN 1100 AND 1199;
DELETE FROM custom_achievement_objectives WHERE id BETWEEN 47000000 AND 47099999;
DELETE FROM custom_achievements           WHERE id BETWEEN 470000 AND 470999;

INSERT INTO custom_achievements
  (id, category_id, slug, name, description, points, hidden, repeatable, reward_title_set,
   reward_item_id, reward_currency_id, reward_currency_amount, enabled, sort_order, created_at)
SELECT 470000 + t.v, 600, CONCAT('master_artisan_', t.v),
       CONCAT('Master Artisan ', t.v),
       CONCAT('Reach ', t.v, ' in every tradeskill that has a mastery. Grants a rank of Salvage.'),
       25, 0, 0, 0, 0, 0, 0, 1, 900 + t.v, UNIX_TIMESTAMP()
FROM (SELECT 50 v UNION ALL SELECT 100 UNION ALL SELECT 150 UNION ALL
      SELECT 200 UNION ALL SELECT 250 UNION ALL SELECT 300) t;

-- ten objectives per tier, one per mastery tradeskill
INSERT INTO custom_achievement_objectives
  (id, achievement_id, objective_index, objective_type, target_type, target_id, target_name,
   required_count, zone_id, class_mask, optional)
SELECT 47000000 + (t.v * 100) + m.seq,
       470000 + t.v, m.seq, 'skill', '', m.skill_id, m.skill_name, t.v, 0, 0, 0
FROM (
  SELECT 0 seq, 56 skill_id, 'Make Poison'   skill_name UNION ALL
  SELECT 1,     57,          'Tinkering'                UNION ALL
  SELECT 2,     59,          'Alchemy'                  UNION ALL
  SELECT 3,     60,          'Baking'                   UNION ALL
  SELECT 4,     61,          'Tailoring'                UNION ALL
  SELECT 5,     63,          'Blacksmithing'            UNION ALL
  SELECT 6,     64,          'Fletching'                UNION ALL
  SELECT 7,     65,          'Brewing'                  UNION ALL
  SELECT 8,     68,          'Jewelcrafting'            UNION ALL
  SELECT 9,     69,          'Pottery'
) m
CROSS JOIN (SELECT 50 v UNION ALL SELECT 100 UNION ALL SELECT 150 UNION ALL
            SELECT 200 UNION ALL SELECT 250 UNION ALL SELECT 300) t;

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1100 + (t.v DIV 50), 470000 + t.v, 'grant_aa', 330, t.v DIV 50, 100, '', 1, 1,
       CONCAT('Salvage rank ', t.v DIV 50), '', 1, 10, UNIX_TIMESTAMP()
FROM (SELECT 50 v UNION ALL SELECT 100 UNION ALL SELECT 150 UNION ALL
      SELECT 200 UNION ALL SELECT 250 UNION ALL SELECT 300) t;

-- ---------------------------------------------------------------- verification
SELECT 'mastery rewards (expect 30)' AS check_name, COUNT(*) AS n
  FROM custom_achievement_rewards WHERE reward_type='grant_aa' AND id BETWEEN 1000 AND 1099
UNION ALL SELECT 'salvage rewards (expect 6)',   COUNT(*) FROM custom_achievement_rewards WHERE id BETWEEN 1100 AND 1199
UNION ALL SELECT 'artisan achievements (6)',     COUNT(*) FROM custom_achievements WHERE id BETWEEN 470000 AND 470999
UNION ALL SELECT 'artisan objectives (60)',      COUNT(*) FROM custom_achievement_objectives WHERE id BETWEEN 47000000 AND 47099999
UNION ALL SELECT 'AAs enabled (expect 11)',      COUNT(*) FROM aa_ability WHERE enabled=1 AND grant_only=1
       AND id IN (49,103,324,325,326,327,328,329,330,575,576);
