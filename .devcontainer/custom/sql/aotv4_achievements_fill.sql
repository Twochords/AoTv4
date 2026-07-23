-- AoTv4 achievement fill — Stage 1 (deterministic, no new objective types).
--   1) Disable the beyond-OoW categories (permanently past the era cap; they only showed empty 0/0).
--   2) Add class-agnostic "Server Custom" kill-milestone achievements (any_kill, earnable by any class).
-- Parent-category count rollup is a code change in zone/achievement_manager.cpp (LoadCategorySummaries),
-- not SQL. Stage 2 (GoD/OoW Hunter) and Stage 3 (Epics + roguelite) are separate files.

-- ---------------------------------------------------------------------------
-- 1) Hide beyond-OoW (Dragons of Norrath .. Rain of Fear) Exploration & Hunter
--    categories. Our era system caps at Omens of War, so these zones never spawn
--    and their achievements are unreachable. Reversible: set enabled=1 if the
--    server ever extends past OoW.
-- ---------------------------------------------------------------------------
UPDATE `custom_achievement_categories`
  SET `enabled` = 0
  WHERE `id` IN (209,210,211,212,213,214,215,216,217,218,219,    -- Exploration: DoN .. RoF
                 3309,3310,3311,3312,3313,3314,3315,3316,3317,3318,3319); -- Hunter: DoN .. RoF

-- ---------------------------------------------------------------------------
-- 2) Server Custom (category 8) — cumulative kill milestones. any_kill counts
--    every NPC killed across the character's life (survives the roguelite death
--    reset, which only zeroes level/exp). Points only, no aura reward.
-- ---------------------------------------------------------------------------
INSERT INTO `custom_achievements`
  (`id`,`category_id`,`slug`,`name`,`description`,`points`,`hidden`,`enabled`,`sort_order`,`created_at`)
VALUES
  (800001,8,'sc-kills-100'  ,'Bloodthirsty'      ,'Slay 100 enemies.'                   ,15 ,0,1,1,1785000000),
  (800002,8,'sc-kills-1000' ,'Warmonger'         ,'Slay 1,000 enemies.'                 ,30 ,0,1,2,1785000000),
  (800003,8,'sc-kills-10000','Harbinger of Death','Slay 10,000 enemies.'                ,60 ,0,1,3,1785000000),
  (800004,8,'sc-kills-50000','Reaper of Norrath' ,'Slay 50,000 enemies.'                ,100,0,1,4,1785000000)
ON DUPLICATE KEY UPDATE
  `category_id`=VALUES(`category_id`), `name`=VALUES(`name`),
  `description`=VALUES(`description`), `points`=VALUES(`points`),
  `enabled`=VALUES(`enabled`), `sort_order`=VALUES(`sort_order`);

INSERT INTO `custom_achievement_objectives`
  (`id`,`achievement_id`,`objective_index`,`objective_type`,`target_type`,`target_id`,`target_name`,`required_count`,`zone_id`,`class_mask`)
VALUES
  (80000001,800001,0,'any_kill','',0,'Slay 100 enemies'   ,100  ,0,0),
  (80000002,800002,0,'any_kill','',0,'Slay 1,000 enemies' ,1000 ,0,0),
  (80000003,800003,0,'any_kill','',0,'Slay 10,000 enemies',10000,0,0),
  (80000004,800004,0,'any_kill','',0,'Slay 50,000 enemies',50000,0,0)
ON DUPLICATE KEY UPDATE
  `objective_type`=VALUES(`objective_type`), `required_count`=VALUES(`required_count`),
  `target_name`=VALUES(`target_name`), `class_mask`=VALUES(`class_mask`);
