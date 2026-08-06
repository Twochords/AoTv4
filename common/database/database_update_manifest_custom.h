/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "common/database/database_update.h"

#include <vector>

std::vector<ManifestEntry> manifest_entries_custom = {
	ManifestEntry{
		.version = 1,
		.description = "2026_05_23_custom_achievements",
		.check = "SHOW TABLES LIKE 'custom_achievement_categories'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `custom_achievement_categories` (
  `id` INT UNSIGNED NOT NULL,
  `parent_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `description` VARCHAR(255) NOT NULL DEFAULT '',
  `sort_order` INT NOT NULL DEFAULT 0,
  `icon_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `enabled` TINYINT(1) NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  KEY `idx_parent_sort` (`parent_id`, `sort_order`, `id`)
);

CREATE TABLE `custom_achievements` (
  `id` INT UNSIGNED NOT NULL,
  `category_id` INT UNSIGNED NOT NULL,
  `slug` VARCHAR(64) NOT NULL DEFAULT '',
  `name` VARCHAR(96) NOT NULL DEFAULT '',
  `description` VARCHAR(255) NOT NULL DEFAULT '',
  `points` INT UNSIGNED NOT NULL DEFAULT 0,
  `hidden` TINYINT(1) NOT NULL DEFAULT 0,
  `repeatable` TINYINT(1) NOT NULL DEFAULT 0,
  `reward_title_set` INT UNSIGNED NOT NULL DEFAULT 0,
  `reward_item_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `reward_currency_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `reward_currency_amount` INT UNSIGNED NOT NULL DEFAULT 0,
  `enabled` TINYINT(1) NOT NULL DEFAULT 1,
  `sort_order` INT NOT NULL DEFAULT 0,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_slug` (`slug`),
  KEY `idx_category_sort` (`category_id`, `sort_order`, `id`)
);

CREATE TABLE `custom_achievement_objectives` (
  `id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `objective_index` INT UNSIGNED NOT NULL DEFAULT 0,
  `objective_type` VARCHAR(32) NOT NULL DEFAULT 'manual',
  `target_type` VARCHAR(32) NOT NULL DEFAULT '',
  `target_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `target_name` VARCHAR(128) NOT NULL DEFAULT '',
  `required_count` INT UNSIGNED NOT NULL DEFAULT 1,
  `zone_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `class_mask` INT UNSIGNED NOT NULL DEFAULT 0,
  `optional` TINYINT(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_achievement` (`achievement_id`, `objective_index`, `id`),
  KEY `idx_type_target` (`objective_type`, `target_id`, `zone_id`)
);

CREATE TABLE `custom_character_achievement_progress` (
  `character_id` INT UNSIGNED NOT NULL,
  `objective_id` INT UNSIGNED NOT NULL,
  `count` INT UNSIGNED NOT NULL DEFAULT 0,
  `completed_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `objective_id`),
  KEY `idx_objective` (`objective_id`)
);

CREATE TABLE `custom_character_achievements` (
  `character_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `completed_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `awarded_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `completion_count` INT UNSIGNED NOT NULL DEFAULT 1,
  `announced` TINYINT(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `achievement_id`),
  KEY `idx_achievement` (`achievement_id`),
  KEY `idx_completed` (`completed_at`)
);

CREATE TABLE `custom_achievement_audit` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `achievement_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `objective_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `action` VARCHAR(64) NOT NULL DEFAULT '',
  `detail` VARCHAR(255) NOT NULL DEFAULT '',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_character_created` (`character_id`, `created_at`),
  KEY `idx_achievement` (`achievement_id`)
);

INSERT INTO `custom_achievement_categories`
(`id`, `parent_id`, `name`, `description`, `sort_order`, `icon_id`, `enabled`)
VALUES
(1, 0, 'Character', 'Character milestones and personal growth.', 10, 0, 1),
(2, 0, 'Exploration', 'Zone discovery and travel achievements.', 20, 0, 1),
(3, 0, 'Hunter', 'Named NPC and zone hunting achievements.', 30, 0, 1),
(4, 0, 'Slayer', 'Creature kill count achievements.', 40, 0, 1),
(5, 0, 'Progression', 'Task, flag, access, and expansion progression.', 50, 0, 1),
(6, 0, 'Tradeskill', 'Crafting and skill achievements.', 60, 0, 1),
(7, 0, 'Epics', 'Class epic and long-term item achievements.', 70, 0, 1),
(8, 0, 'Server Custom', 'Rebirth, difficulty, attunement, and custom server milestones.', 80, 0, 1)
ON DUPLICATE KEY UPDATE
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
VALUES
(1001, 1, 'level_10', 'Level 10', 'Become a level 10 Adventurer.', 10, 0, 0, 10, UNIX_TIMESTAMP(), 1),
(1002, 1, 'level_20', 'Level 20', 'Become a level 20 Adventurer.', 10, 0, 0, 20, UNIX_TIMESTAMP(), 1),
(1003, 1, 'level_30', 'Level 30', 'Become a level 30 Adventurer.', 10, 0, 0, 30, UNIX_TIMESTAMP(), 1),
(1004, 1, 'level_40', 'Level 40', 'Become a level 40 Adventurer.', 10, 0, 0, 40, UNIX_TIMESTAMP(), 1),
(1005, 1, 'level_50', 'Level 50', 'Become a level 50 Adventurer.', 20, 0, 0, 50, UNIX_TIMESTAMP(), 1),
(1006, 1, 'level_60', 'Level 60', 'Become a level 60 Adventurer.', 20, 0, 0, 60, UNIX_TIMESTAMP(), 1),
(1007, 1, 'level_65', 'Level 65', 'Become a level 65 Adventurer.', 25, 0, 0, 65, UNIX_TIMESTAMP(), 1),
(1008, 1, 'level_70', 'Level 70', 'Become a level 70 Adventurer.', 25, 0, 0, 70, UNIX_TIMESTAMP(), 1)
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(1001, 1001, 0, 'level', 'level', 10, 'Reach level 10', 10, 0, 0, 0),
(1002, 1002, 0, 'level', 'level', 20, 'Reach level 20', 20, 0, 0, 0),
(1003, 1003, 0, 'level', 'level', 30, 'Reach level 30', 30, 0, 0, 0),
(1004, 1004, 0, 'level', 'level', 40, 'Reach level 40', 40, 0, 0, 0),
(1005, 1005, 0, 'level', 'level', 50, 'Reach level 50', 50, 0, 0, 0),
(1006, 1006, 0, 'level', 'level', 60, 'Reach level 60', 60, 0, 0, 0),
(1007, 1007, 0, 'level', 'level', 65, 'Reach level 65', 65, 0, 0, 0),
(1008, 1008, 0, 'level', 'level', 70, 'Reach level 70', 70, 0, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version = 2,
		.description = "2026_05_23_custom_achievement_catalog_seed",
		.check = "SELECT 1 FROM `custom_achievements` WHERE `id` = 200279 LIMIT 1",
		.condition = "empty",
		.match = "",
		.sql = R"(
INSERT INTO `custom_achievement_categories`
(`id`, `parent_id`, `name`, `description`, `sort_order`, `icon_id`, `enabled`)
VALUES
(200, 2, 'Classic Exploration', 'Explore Classic-era zones.', 200, 0, 1),
(201, 2, 'The Ruins of Kunark Exploration', 'Explore The Ruins of Kunark zones.', 201, 0, 1),
(202, 2, 'The Scars of Velious Exploration', 'Explore The Scars of Velious zones.', 202, 0, 1),
(203, 2, 'The Shadows of Luclin Exploration', 'Explore The Shadows of Luclin zones.', 203, 0, 1),
(204, 2, 'The Planes of Power Exploration', 'Explore The Planes of Power zones.', 204, 0, 1),
(205, 2, 'The Legacy of Ykesha Exploration', 'Explore The Legacy of Ykesha zones.', 205, 0, 1),
(206, 2, 'Lost Dungeons of Norrath Exploration', 'Explore Lost Dungeons of Norrath zones.', 206, 0, 1),
(207, 2, 'Gates of Discord Exploration', 'Explore Gates of Discord zones.', 207, 0, 1),
(208, 2, 'Omens of War Exploration', 'Explore Omens of War zones.', 208, 0, 1),
(209, 2, 'Dragons of Norrath Exploration', 'Explore Dragons of Norrath zones.', 209, 0, 1),
(210, 2, 'Depths of Darkhollow Exploration', 'Explore Depths of Darkhollow zones.', 210, 0, 1),
(211, 2, 'Prophecy of Ro Exploration', 'Explore Prophecy of Ro zones.', 211, 0, 1),
(212, 2, 'The Serpent''s Spine Exploration', 'Explore The Serpent''s Spine zones.', 212, 0, 1),
(213, 2, 'The Buried Sea Exploration', 'Explore The Buried Sea zones.', 213, 0, 1),
(214, 2, 'Secrets of Faydwer Exploration', 'Explore Secrets of Faydwer zones.', 214, 0, 1),
(215, 2, 'Seeds of Destruction Exploration', 'Explore Seeds of Destruction zones.', 215, 0, 1),
(216, 2, 'Underfoot Exploration', 'Explore Underfoot zones.', 216, 0, 1),
(217, 2, 'House of Thule Exploration', 'Explore House of Thule zones.', 217, 0, 1),
(218, 2, 'Veil of Alaris Exploration', 'Explore Veil of Alaris zones.', 218, 0, 1),
(219, 2, 'Rain of Fear Exploration', 'Explore Rain of Fear zones.', 219, 0, 1),
(300, 5, 'Task Completion', 'Quest and task completion achievements.', 300, 0, 1),
(400, 4, 'Zone Slayer', 'Zone-wide kill count achievements.', 400, 0, 1),
(410, 4, 'Creature Slayer', 'Race and body type kill count achievements.', 410, 0, 1),
(600, 6, 'Tradeskill Mastery', 'Tradeskill milestone achievements.', 600, 0, 1)
ON DUPLICATE KEY UPDATE
`parent_id` = VALUES(`parent_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
200000 + z.`zoneidnumber`,
200 + z.`expansion`,
LEFT(CONCAT('explorer_', z.`short_name`), 64),
LEFT(CONCAT('Explorer: ', z.`long_name`), 96),
LEFT(CONCAT('Visit ', z.`long_name`, '.'), 255),
5,
0,
0,
z.`zoneidnumber`,
UNIX_TIMESTAMP(),
1
FROM `zone` z
WHERE z.`version` = 0
AND z.`min_status` = 0
AND z.`zoneidnumber` > 0
AND z.`expansion` BETWEEN 0 AND 19
AND COALESCE(z.`short_name`, '') <> ''
AND COALESCE(z.`long_name`, '') <> ''
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
1200000 + z.`zoneidnumber`,
200000 + z.`zoneidnumber`,
0,
'zone_visit',
'zone',
z.`zoneidnumber`,
LEFT(CONCAT('Visit ', z.`long_name`), 128),
1,
z.`zoneidnumber`,
0,
0
FROM `zone` z
WHERE z.`version` = 0
AND z.`min_status` = 0
AND z.`zoneidnumber` > 0
AND z.`expansion` BETWEEN 0 AND 19
AND COALESCE(z.`short_name`, '') <> ''
AND COALESCE(z.`long_name`, '') <> ''
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
700000 + z.`zoneidnumber`,
400,
LEFT(CONCAT('zone_slayer_', z.`short_name`), 64),
LEFT(CONCAT('Slayer: ', z.`long_name`), 96),
LEFT(CONCAT('Defeat enemies in ', z.`long_name`, '.'), 255),
10,
0,
0,
z.`zoneidnumber`,
UNIX_TIMESTAMP(),
1
FROM `zone` z
WHERE z.`version` = 0
AND z.`min_status` = 0
AND z.`zoneidnumber` > 0
AND z.`expansion` BETWEEN 0 AND 19
AND z.`cancombat` = 1
AND COALESCE(z.`short_name`, '') <> ''
AND COALESCE(z.`long_name`, '') <> ''
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
1700000 + z.`zoneidnumber`,
700000 + z.`zoneidnumber`,
0,
'zone_kill',
'zone',
z.`zoneidnumber`,
LEFT(CONCAT('Defeat enemies in ', z.`long_name`), 128),
50,
z.`zoneidnumber`,
0,
0
FROM `zone` z
WHERE z.`version` = 0
AND z.`min_status` = 0
AND z.`zoneidnumber` > 0
AND z.`expansion` BETWEEN 0 AND 19
AND z.`cancombat` = 1
AND COALESCE(z.`short_name`, '') <> ''
AND COALESCE(z.`long_name`, '') <> ''
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
3000000 + t.`id`,
300,
CONCAT('task_', t.`id`),
LEFT(CONCAT('Task: ', t.`title`), 96),
LEFT(CONCAT('Complete the task "', t.`title`, '".'), 255),
10,
0,
0,
t.`id`,
UNIX_TIMESTAMP(),
1
FROM `tasks` t
WHERE t.`id` > 0
AND COALESCE(t.`enabled`, 1) = 1
AND COALESCE(t.`title`, '') <> ''
AND LOWER(t.`title`) <> 'autoloot test'
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
13000000 + t.`id`,
3000000 + t.`id`,
0,
'task_complete',
'task',
t.`id`,
LEFT(t.`title`, 128),
1,
0,
0,
0
FROM `tasks` t
WHERE t.`id` > 0
AND COALESCE(t.`enabled`, 1) = 1
AND COALESCE(t.`title`, '') <> ''
AND LOWER(t.`title`) <> 'autoloot test'
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
400000 + s.`skill_id` * 1000 + r.`required_count`,
600,
CONCAT('skill_', s.`skill_id`, '_', r.`required_count`),
CONCAT(s.`name`, ' ', r.`required_count`),
CONCAT('Raise ', s.`name`, ' to ', r.`required_count`, '.'),
r.`points`,
0,
0,
s.`skill_id` * 1000 + r.`required_count`,
UNIX_TIMESTAMP(),
1
FROM (
SELECT 55 AS `skill_id`, 'Fishing' AS `name` UNION ALL
SELECT 56, 'Make Poison' UNION ALL
SELECT 57, 'Tinkering' UNION ALL
SELECT 58, 'Research' UNION ALL
SELECT 59, 'Alchemy' UNION ALL
SELECT 60, 'Baking' UNION ALL
SELECT 61, 'Tailoring' UNION ALL
SELECT 63, 'Blacksmithing' UNION ALL
SELECT 64, 'Fletching' UNION ALL
SELECT 65, 'Brewing' UNION ALL
SELECT 68, 'Jewelcrafting' UNION ALL
SELECT 69, 'Pottery'
) s
JOIN (
SELECT 50 AS `required_count`, 5 AS `points` UNION ALL
SELECT 100, 5 UNION ALL
SELECT 150, 10 UNION ALL
SELECT 200, 10 UNION ALL
SELECT 250, 15 UNION ALL
SELECT 300, 25
) r
ON 1 = 1
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
1400000 + s.`skill_id` * 1000 + r.`required_count`,
400000 + s.`skill_id` * 1000 + r.`required_count`,
0,
'skill',
'skill',
s.`skill_id`,
CONCAT(s.`name`, ' ', r.`required_count`),
r.`required_count`,
0,
0,
0
FROM (
SELECT 55 AS `skill_id`, 'Fishing' AS `name` UNION ALL
SELECT 56, 'Make Poison' UNION ALL
SELECT 57, 'Tinkering' UNION ALL
SELECT 58, 'Research' UNION ALL
SELECT 59, 'Alchemy' UNION ALL
SELECT 60, 'Baking' UNION ALL
SELECT 61, 'Tailoring' UNION ALL
SELECT 63, 'Blacksmithing' UNION ALL
SELECT 64, 'Fletching' UNION ALL
SELECT 65, 'Brewing' UNION ALL
SELECT 68, 'Jewelcrafting' UNION ALL
SELECT 69, 'Pottery'
) s
JOIN (
SELECT 50 AS `required_count` UNION ALL
SELECT 100 UNION ALL
SELECT 150 UNION ALL
SELECT 200 UNION ALL
SELECT 250 UNION ALL
SELECT 300
) r
ON 1 = 1
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
500000 + r.`race_id` * 1000 + c.`required_count`,
410,
CONCAT('race_slayer_', r.`race_id`, '_', c.`required_count`),
CONCAT(c.`rank_name`, ' ', r.`name`, ' Slayer'),
CONCAT('Slay ', c.`required_count`, ' ', r.`name`, ' enemies.'),
c.`points`,
0,
0,
r.`race_id` * 1000 + c.`required_count`,
UNIX_TIMESTAMP(),
1
FROM (
SELECT 13 AS `race_id`, 'Aviak' AS `name` UNION ALL
SELECT 17, 'Golem' UNION ALL
SELECT 18, 'Giant' UNION ALL
SELECT 21, 'Evil Eye' UNION ALL
SELECT 25, 'Fairy' UNION ALL
SELECT 26, 'Froglok' UNION ALL
SELECT 33, 'Ghoul' UNION ALL
SELECT 38, 'Spider' UNION ALL
SELECT 39, 'Gnoll' UNION ALL
SELECT 40, 'Goblin' UNION ALL
SELECT 42, 'Wolf' UNION ALL
SELECT 43, 'Bear' UNION ALL
SELECT 48, 'Kobold' UNION ALL
SELECT 51, 'Lizard Man' UNION ALL
SELECT 53, 'Minotaur' UNION ALL
SELECT 54, 'Orc' UNION ALL
SELECT 60, 'Skeleton' UNION ALL
SELECT 65, 'Vampire' UNION ALL
SELECT 70, 'Zombie' UNION ALL
SELECT 75, 'Elemental' UNION ALL
SELECT 79, 'Bixie' UNION ALL
SELECT 88, 'Clockwork' UNION ALL
SELECT 89, 'Drake'
) r
JOIN (
SELECT 25 AS `required_count`, 'Novice' AS `rank_name`, 5 AS `points` UNION ALL
SELECT 100, 'Adept', 10 UNION ALL
SELECT 250, 'Veteran', 15
) c
ON 1 = 1
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
1500000 + r.`race_id` * 1000 + c.`required_count`,
500000 + r.`race_id` * 1000 + c.`required_count`,
0,
'race_kill',
'race',
r.`race_id`,
r.`name`,
c.`required_count`,
0,
0,
0
FROM (
SELECT 13 AS `race_id`, 'Aviak' AS `name` UNION ALL
SELECT 17, 'Golem' UNION ALL
SELECT 18, 'Giant' UNION ALL
SELECT 21, 'Evil Eye' UNION ALL
SELECT 25, 'Fairy' UNION ALL
SELECT 26, 'Froglok' UNION ALL
SELECT 33, 'Ghoul' UNION ALL
SELECT 38, 'Spider' UNION ALL
SELECT 39, 'Gnoll' UNION ALL
SELECT 40, 'Goblin' UNION ALL
SELECT 42, 'Wolf' UNION ALL
SELECT 43, 'Bear' UNION ALL
SELECT 48, 'Kobold' UNION ALL
SELECT 51, 'Lizard Man' UNION ALL
SELECT 53, 'Minotaur' UNION ALL
SELECT 54, 'Orc' UNION ALL
SELECT 60, 'Skeleton' UNION ALL
SELECT 65, 'Vampire' UNION ALL
SELECT 70, 'Zombie' UNION ALL
SELECT 75, 'Elemental' UNION ALL
SELECT 79, 'Bixie' UNION ALL
SELECT 88, 'Clockwork' UNION ALL
SELECT 89, 'Drake'
) r
JOIN (
SELECT 25 AS `required_count` UNION ALL
SELECT 100 UNION ALL
SELECT 250
) c
ON 1 = 1
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
550000 + b.`bodytype_id` * 1000 + c.`required_count`,
410,
CONCAT('bodytype_slayer_', b.`bodytype_id`, '_', c.`required_count`),
CONCAT(c.`rank_name`, ' ', b.`name`, ' Slayer'),
CONCAT('Slay ', c.`required_count`, ' ', b.`name`, ' enemies.'),
c.`points`,
0,
0,
b.`bodytype_id` * 1000 + c.`required_count`,
UNIX_TIMESTAMP(),
1
FROM (
SELECT 3 AS `bodytype_id`, 'Undead' AS `name` UNION ALL
SELECT 4, 'Giant' UNION ALL
SELECT 5, 'Construct' UNION ALL
SELECT 6, 'Extraplanar' UNION ALL
SELECT 12, 'Vampire' UNION ALL
SELECT 21, 'Animal' UNION ALL
SELECT 22, 'Insect' UNION ALL
SELECT 23, 'Monster' UNION ALL
SELECT 25, 'Plant' UNION ALL
SELECT 26, 'Dragon' UNION ALL
SELECT 34, 'Muramite'
) b
JOIN (
SELECT 100 AS `required_count`, 'Adept' AS `rank_name`, 10 AS `points` UNION ALL
SELECT 500, 'Veteran', 20 UNION ALL
SELECT 1000, 'Master', 30
) c
ON 1 = 1
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
1550000 + b.`bodytype_id` * 1000 + c.`required_count`,
550000 + b.`bodytype_id` * 1000 + c.`required_count`,
0,
'bodytype_kill',
'bodytype',
b.`bodytype_id`,
b.`name`,
c.`required_count`,
0,
0,
0
FROM (
SELECT 3 AS `bodytype_id`, 'Undead' AS `name` UNION ALL
SELECT 4, 'Giant' UNION ALL
SELECT 5, 'Construct' UNION ALL
SELECT 6, 'Extraplanar' UNION ALL
SELECT 12, 'Vampire' UNION ALL
SELECT 21, 'Animal' UNION ALL
SELECT 22, 'Insect' UNION ALL
SELECT 23, 'Monster' UNION ALL
SELECT 25, 'Plant' UNION ALL
SELECT 26, 'Dragon' UNION ALL
SELECT 34, 'Muramite'
) b
JOIN (
SELECT 100 AS `required_count` UNION ALL
SELECT 500 UNION ALL
SELECT 1000
) c
ON 1 = 1
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
SELECT
110000 + l.`level`,
1,
CONCAT('level_', l.`level`, '_milestone'),
CONCAT('Level ', l.`level`),
CONCAT('Become a level ', l.`level`, ' Adventurer.'),
l.`points`,
0,
0,
l.`level`,
UNIX_TIMESTAMP(),
1
FROM (
SELECT 5 AS `level`, 5 AS `points` UNION ALL
SELECT 15, 10 UNION ALL
SELECT 25, 10 UNION ALL
SELECT 35, 10 UNION ALL
SELECT 45, 15 UNION ALL
SELECT 55, 20 UNION ALL
SELECT 61, 20 UNION ALL
SELECT 62, 20 UNION ALL
SELECT 63, 20 UNION ALL
SELECT 64, 20 UNION ALL
SELECT 66, 25 UNION ALL
SELECT 67, 25 UNION ALL
SELECT 68, 25 UNION ALL
SELECT 69, 25
) l
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
111000 + l.`level`,
110000 + l.`level`,
0,
'level',
'level',
l.`level`,
CONCAT('Reach level ', l.`level`),
l.`level`,
0,
0,
0
FROM (
SELECT 5 AS `level` UNION ALL
SELECT 15 UNION ALL
SELECT 25 UNION ALL
SELECT 35 UNION ALL
SELECT 45 UNION ALL
SELECT 55 UNION ALL
SELECT 61 UNION ALL
SELECT 62 UNION ALL
SELECT 63 UNION ALL
SELECT 64 UNION ALL
SELECT 66 UNION ALL
SELECT 67 UNION ALL
SELECT 68 UNION ALL
SELECT 69
) l
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version = 3,
		.description = "2026_05_23_live_hunter_achievement_seed",
		.check = "SELECT 1 FROM `custom_achievements` WHERE `slug` = 'hunter_live_100480' LIMIT 1",
		.condition = "empty",
		.match = "",
		.sql = R"(
INSERT INTO `custom_achievement_categories`
(`id`, `parent_id`, `name`, `description`, `sort_order`, `icon_id`, `enabled`)
VALUES
(3300, 3, 'EverQuest Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3300, 0, 1),
(3301, 3, 'The Ruins of Kunark Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3301, 0, 1),
(3302, 3, 'The Scars of Velious Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3302, 0, 1),
(3303, 3, 'The Shadows of Luclin Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3303, 0, 1),
(3304, 3, 'The Planes of Power Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3304, 0, 1),
(3305, 3, 'The Legacy of Ykesha Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3305, 0, 1),
(3306, 3, 'Lost Dungeons of Norrath Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3306, 0, 1),
(3307, 3, 'Gates of Discord Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3307, 0, 1),
(3308, 3, 'Omens of War Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3308, 0, 1),
(3309, 3, 'Dragons of Norrath Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3309, 0, 1),
(3310, 3, 'Depths of Darkhollow Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3310, 0, 1),
(3311, 3, 'Prophecy of Ro Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3311, 0, 1),
(3312, 3, 'The Serpent''s Spine Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3312, 0, 1),
(3313, 3, 'The Buried Sea Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3313, 0, 1),
(3314, 3, 'Secrets of Faydwer Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3314, 0, 1),
(3315, 3, 'Seeds of Destruction Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3315, 0, 1),
(3316, 3, 'Underfoot Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3316, 0, 1),
(3317, 3, 'House of Thule Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3317, 0, 1),
(3318, 3, 'Veil of Alaris Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3318, 0, 1),
(3319, 3, 'Rain of Fear Hunter', 'Live Hunter targets matched to local EQEmu NPC spawns.', 3319, 0, 1)
ON DUPLICATE KEY UPDATE
`parent_id` = VALUES(`parent_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
VALUES
(8100480, 3300, 'hunter_live_100480', 'Hunter of The Qeynos Hills', 'Defeat 2 matched live Hunter targets in The Qeynos Hills.', 30, 0, 0, 100480, UNIX_TIMESTAMP(), 1),
(8101180, 3300, 'hunter_live_101180', 'Hunter of The Liberated Citadel of Runnyeye', 'Defeat 1 matched live Hunter target in The Liberated Citadel of Runnyeye.', 30, 0, 0, 101180, UNIX_TIMESTAMP(), 1),
(8101380, 3300, 'hunter_live_101380', 'Hunter of The Northern Plains of Karana', 'Defeat 7 matched live Hunter targets in The Northern Plains of Karana.', 30, 0, 0, 101380, UNIX_TIMESTAMP(), 1),
(8101480, 3300, 'hunter_live_101480', 'Hunter of The Southern Plains of Karana', 'Defeat 12 matched live Hunter targets in The Southern Plains of Karana.', 30, 0, 0, 101480, UNIX_TIMESTAMP(), 1),
(8101880, 3300, 'hunter_live_101880', 'Hunter of The Lair of the Splitpaw', 'Defeat 4 matched live Hunter targets in The Lair of the Splitpaw.', 30, 0, 0, 101880, UNIX_TIMESTAMP(), 1),
(8102780, 3300, 'hunter_live_102780', 'Hunter of The Lavastorm Mountains', 'Defeat 4 matched live Hunter targets in The Lavastorm Mountains.', 30, 0, 0, 102780, UNIX_TIMESTAMP(), 1),
(8103180, 3300, 'hunter_live_103180', 'Hunter of Solusek''s Eye', 'Defeat 14 matched live Hunter targets in Solusek''s Eye.', 30, 0, 0, 103180, UNIX_TIMESTAMP(), 1),
(8103280, 3300, 'hunter_live_103280', 'Hunter of Nagafen''s Lair', 'Defeat 4 matched live Hunter targets in Nagafen''s Lair.', 30, 0, 0, 103280, UNIX_TIMESTAMP(), 1),
(8103680, 3300, 'hunter_live_103680', 'Hunter of Befallen', 'Defeat 2 matched live Hunter targets in Befallen.', 30, 0, 0, 103680, UNIX_TIMESTAMP(), 1),
(8103980, 3300, 'hunter_live_103980', 'Hunter of The Ruins of Old Paineel', 'Defeat 13 matched live Hunter targets in The Ruins of Old Paineel.', 30, 0, 0, 103980, UNIX_TIMESTAMP(), 1),
(8104480, 3300, 'hunter_live_104480', 'Hunter of Najena', 'Defeat 1 matched live Hunter target in Najena.', 30, 0, 0, 104480, UNIX_TIMESTAMP(), 1),
(8104580, 3300, 'hunter_live_104580', 'Hunter of The Qeynos Aqueduct System', 'Defeat 3 matched live Hunter targets in The Qeynos Aqueduct System.', 30, 0, 0, 104580, UNIX_TIMESTAMP(), 1),
(8104780, 3300, 'hunter_live_104780', 'Hunter of The Feerrott', 'Defeat 6 matched live Hunter targets in The Feerrott.', 30, 0, 0, 104780, UNIX_TIMESTAMP(), 1),
(8105080, 3300, 'hunter_live_105080', 'Hunter of The Rathe Mountains', 'Defeat 6 matched live Hunter targets in The Rathe Mountains.', 30, 0, 0, 105080, UNIX_TIMESTAMP(), 1),
(8105180, 3300, 'hunter_live_105180', 'Hunter of Lake Rathetear', 'Defeat 1 matched live Hunter target in Lake Rathetear.', 30, 0, 0, 105180, UNIX_TIMESTAMP(), 1),
(8105780, 3300, 'hunter_live_105780', 'Hunter of The Lesser Faydark', 'Defeat 5 matched live Hunter targets in The Lesser Faydark.', 30, 0, 0, 105780, UNIX_TIMESTAMP(), 1),
(8105880, 3300, 'hunter_live_105880', 'Hunter of Crushbone', 'Defeat 2 matched live Hunter targets in Crushbone.', 30, 0, 0, 105880, UNIX_TIMESTAMP(), 1),
(8105980, 3300, 'hunter_live_105980', 'Hunter of The Castle of Mistmoore', 'Defeat 7 matched live Hunter targets in The Castle of Mistmoore.', 30, 0, 0, 105980, UNIX_TIMESTAMP(), 1),
(8106380, 3300, 'hunter_live_106380', 'Hunter of The Estate of Unrest', 'Defeat 6 matched live Hunter targets in The Estate of Unrest.', 30, 0, 0, 106380, UNIX_TIMESTAMP(), 1),
(8106480, 3300, 'hunter_live_106480', 'Hunter of Kedge Keep', 'Defeat 9 matched live Hunter targets in Kedge Keep.', 30, 0, 0, 106480, UNIX_TIMESTAMP(), 1),
(8106580, 3300, 'hunter_live_106580', 'Hunter of The City of Guk', 'Defeat 7 matched live Hunter targets in The City of Guk.', 30, 0, 0, 106580, UNIX_TIMESTAMP(), 1),
(8106680, 3300, 'hunter_live_106680', 'Hunter of The Ruins of Old Guk', 'Defeat 20 matched live Hunter targets in The Ruins of Old Guk.', 30, 0, 0, 106680, UNIX_TIMESTAMP(), 1),
(8106880, 3300, 'hunter_live_106880', 'Hunter of Butcherblock Mountains', 'Defeat 1 matched live Hunter target in Butcherblock Mountains.', 30, 0, 0, 106880, UNIX_TIMESTAMP(), 1),
(8107080, 3300, 'hunter_live_107080', 'Hunter of Dagnor''s Cauldron', 'Defeat 4 matched live Hunter targets in Dagnor''s Cauldron.', 30, 0, 0, 107080, UNIX_TIMESTAMP(), 1),
(8107380, 3300, 'hunter_live_107380', 'Hunter of The Permafrost Caverns', 'Defeat 6 matched live Hunter targets in The Permafrost Caverns.', 30, 0, 0, 107380, UNIX_TIMESTAMP(), 1),
(8110080, 3300, 'hunter_live_110080', 'Hunter of The Stonebrunt Mountains', 'Defeat 7 matched live Hunter targets in The Stonebrunt Mountains.', 30, 0, 0, 110080, UNIX_TIMESTAMP(), 1),
(8110180, 3300, 'hunter_live_110180', 'Hunter of The Warrens', 'Defeat 2 matched live Hunter targets in The Warrens.', 30, 0, 0, 110180, UNIX_TIMESTAMP(), 1),
(8118180, 3300, 'hunter_live_118180', 'Hunter of Jaggedpine Forest', 'Defeat 6 matched live Hunter targets in Jaggedpine Forest.', 30, 0, 0, 118180, UNIX_TIMESTAMP(), 1),
(8118280, 3300, 'hunter_live_118280', 'Hunter of Nedaria''s Landing', 'Defeat 1 matched live Hunter target in Nedaria''s Landing.', 30, 0, 0, 118280, UNIX_TIMESTAMP(), 1),
(8138480, 3300, 'hunter_live_138480', 'Hunter of Freeport Sewers', 'Defeat 1 matched live Hunter target in Freeport Sewers.', 30, 0, 0, 138480, UNIX_TIMESTAMP(), 1),
(8139280, 3300, 'hunter_live_139280', 'Hunter of North Desert of Ro', 'Defeat 1 matched live Hunter target in North Desert of Ro.', 30, 0, 0, 139280, UNIX_TIMESTAMP(), 1),
(8140780, 3300, 'hunter_live_140780', 'Hunter of Highpass Hold', 'Defeat 4 matched live Hunter targets in Highpass Hold.', 30, 0, 0, 140780, UNIX_TIMESTAMP(), 1),
(8141580, 3300, 'hunter_live_141580', 'Hunter of The Misty Thicket', 'Defeat 1 matched live Hunter target in The Misty Thicket.', 30, 0, 0, 141580, UNIX_TIMESTAMP(), 1),
(8153980, 3300, 'hunter_live_153980', 'Hunter of The Hole', 'Defeat 6 matched live Hunter targets in The Hole.', 30, 0, 0, 153980, UNIX_TIMESTAMP(), 1),
(8154880, 3300, 'hunter_live_154880', 'Hunter of Accursed Temple of Cazic-Thule', 'Defeat 50 matched live Hunter targets in Accursed Temple of Cazic-Thule.', 30, 0, 0, 154880, UNIX_TIMESTAMP(), 1),
(8158680, 3300, 'hunter_live_158680', 'Hunter of The Plane of Hate', 'Defeat 34 matched live Hunter targets in The Plane of Hate.', 30, 0, 0, 158680, UNIX_TIMESTAMP(), 1),
(8207880, 3301, 'hunter_live_207880', 'Hunter of The Field of Bone', 'Defeat 3 matched live Hunter targets in The Field of Bone.', 30, 0, 0, 207880, UNIX_TIMESTAMP(), 1),
(8207980, 3301, 'hunter_live_207980', 'Hunter of The Warsliks Woods', 'Defeat 2 matched live Hunter targets in The Warsliks Woods.', 30, 0, 0, 207980, UNIX_TIMESTAMP(), 1),
(8208180, 3301, 'hunter_live_208180', 'Hunter of The Temple of Droga', 'Defeat 2 matched live Hunter targets in The Temple of Droga.', 30, 0, 0, 208180, UNIX_TIMESTAMP(), 1),
(8208380, 3301, 'hunter_live_208380', 'Hunter of The Swamp of No Hope', 'Defeat 18 matched live Hunter targets in The Swamp of No Hope.', 30, 0, 0, 208380, UNIX_TIMESTAMP(), 1),
(8208580, 3301, 'hunter_live_208580', 'Hunter of Lake of Ill Omen', 'Defeat 1 matched live Hunter target in Lake of Ill Omen.', 30, 0, 0, 208580, UNIX_TIMESTAMP(), 1),
(8208680, 3301, 'hunter_live_208680', 'Hunter of The Dreadlands', 'Defeat 3 matched live Hunter targets in The Dreadlands.', 30, 0, 0, 208680, UNIX_TIMESTAMP(), 1),
(8208780, 3301, 'hunter_live_208780', 'Hunter of The Burning Wood', 'Defeat 4 matched live Hunter targets in The Burning Wood.', 30, 0, 0, 208780, UNIX_TIMESTAMP(), 1),
(8208880, 3301, 'hunter_live_208880', 'Hunter of Kaesora', 'Defeat 8 matched live Hunter targets in Kaesora.', 30, 0, 0, 208880, UNIX_TIMESTAMP(), 1),
(8208980, 3301, 'hunter_live_208980', 'Hunter of The Ruins of Sebilis', 'Defeat 21 matched live Hunter targets in The Ruins of Sebilis.', 30, 0, 0, 208980, UNIX_TIMESTAMP(), 1),
(8209280, 3301, 'hunter_live_209280', 'Hunter of Frontier Mountains', 'Defeat 2 matched live Hunter targets in Frontier Mountains.', 30, 0, 0, 209280, UNIX_TIMESTAMP(), 1),
(8209580, 3301, 'hunter_live_209580', 'Hunter of Trakanon''s Teeth', 'Defeat 31 matched live Hunter targets in Trakanon''s Teeth.', 30, 0, 0, 209580, UNIX_TIMESTAMP(), 1),
(8209680, 3301, 'hunter_live_209680', 'Hunter of Timorous Deep', 'Defeat 2 matched live Hunter targets in Timorous Deep.', 30, 0, 0, 209680, UNIX_TIMESTAMP(), 1),
(8209780, 3301, 'hunter_live_209780', 'Hunter of Kurn''s Tower', 'Defeat 2 matched live Hunter targets in Kurn''s Tower.', 30, 0, 0, 209780, UNIX_TIMESTAMP(), 1),
(8210280, 3301, 'hunter_live_210280', 'Hunter of Karnor''s Castle', 'Defeat 12 matched live Hunter targets in Karnor''s Castle.', 30, 0, 0, 210280, UNIX_TIMESTAMP(), 1),
(8210380, 3301, 'hunter_live_210380', 'Hunter of Chardok', 'Defeat 18 matched live Hunter targets in Chardok.', 30, 0, 0, 210380, UNIX_TIMESTAMP(), 1),
(8210480, 3301, 'hunter_live_210480', 'Hunter of The Crypt of Dalnir', 'Defeat 6 matched live Hunter targets in The Crypt of Dalnir.', 30, 0, 0, 210480, UNIX_TIMESTAMP(), 1),
(8210580, 3301, 'hunter_live_210580', 'Hunter of The Howling Stones', 'Defeat 14 matched live Hunter targets in The Howling Stones.', 30, 0, 0, 210580, UNIX_TIMESTAMP(), 1),
(8210880, 3301, 'hunter_live_210880', 'Hunter of Veeshan''s Peak', 'Defeat 2 matched live Hunter targets in Veeshan''s Peak.', 30, 0, 0, 210880, UNIX_TIMESTAMP(), 1),
(8210980, 3301, 'hunter_live_210980', 'Hunter of Veksar', 'Defeat 22 matched live Hunter targets in Veksar.', 30, 0, 0, 210980, UNIX_TIMESTAMP(), 1),
(8227780, 3301, 'hunter_live_227780', 'Hunter of Chardok: The Halls of Betrayal', 'Defeat 9 matched live Hunter targets in Chardok: The Halls of Betrayal.', 30, 0, 0, 227780, UNIX_TIMESTAMP(), 1),
(8250880, 3301, 'hunter_live_250880', 'Hunter of Veeshan''s Peak', 'Defeat 6 matched live Hunter targets in Veeshan''s Peak.', 30, 0, 0, 250880, UNIX_TIMESTAMP(), 1),
(8311080, 3302, 'hunter_live_311080', 'Hunter of The Iceclad Ocean', 'Defeat 3 matched live Hunter targets in The Iceclad Ocean.', 30, 0, 0, 311080, UNIX_TIMESTAMP(), 1),
(8311280, 3302, 'hunter_live_311280', 'Hunter of Velketor''s Labyrinth', 'Defeat 2 matched live Hunter targets in Velketor''s Labyrinth.', 30, 0, 0, 311280, UNIX_TIMESTAMP(), 1),
(8311680, 3302, 'hunter_live_311680', 'Hunter of Eastern Wastes', 'Defeat 4 matched live Hunter targets in Eastern Wastes.', 30, 0, 0, 311680, UNIX_TIMESTAMP(), 1)
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
VALUES
(8311780, 3302, 'hunter_live_311780', 'Hunter of Cobalt Scar', 'Defeat 1 matched live Hunter target in Cobalt Scar.', 30, 0, 0, 311780, UNIX_TIMESTAMP(), 1),
(8311880, 3302, 'hunter_live_311880', 'Hunter of The Great Divide', 'Defeat 1 matched live Hunter target in The Great Divide.', 30, 0, 0, 311880, UNIX_TIMESTAMP(), 1),
(8311980, 3302, 'hunter_live_311980', 'Hunter of The Wakening Land', 'Defeat 2 matched live Hunter targets in The Wakening Land.', 30, 0, 0, 311980, UNIX_TIMESTAMP(), 1),
(8312080, 3302, 'hunter_live_312080', 'Hunter of The Western Wastes', 'Defeat 1 matched live Hunter target in The Western Wastes.', 30, 0, 0, 312080, UNIX_TIMESTAMP(), 1),
(8312180, 3302, 'hunter_live_312180', 'Hunter of The Crystal Caverns', 'Defeat 1 matched live Hunter target in The Crystal Caverns.', 30, 0, 0, 312180, UNIX_TIMESTAMP(), 1),
(8312380, 3302, 'hunter_live_312380', 'Hunter of Dragon Necropolis', 'Defeat 5 matched live Hunter targets in Dragon Necropolis.', 30, 0, 0, 312380, UNIX_TIMESTAMP(), 1),
(8312580, 3302, 'hunter_live_312580', 'Hunter of Siren''s Grotto', 'Defeat 4 matched live Hunter targets in Siren''s Grotto.', 30, 0, 0, 312580, UNIX_TIMESTAMP(), 1),
(8312980, 3302, 'hunter_live_312980', 'Hunter of Icewell Keep', 'Defeat 2 matched live Hunter targets in Icewell Keep.', 30, 0, 0, 312980, UNIX_TIMESTAMP(), 1),
(8352680, 3302, 'hunter_live_352680', 'Hunter of The Plane of Mischief', 'Defeat 25 matched live Hunter targets in The Plane of Mischief.', 30, 0, 0, 352680, UNIX_TIMESTAMP(), 1),
(8380280, 3302, 'hunter_live_380280', 'Hunter of The Sleeper''s Tomb', 'Defeat 1 matched live Hunter target in The Sleeper''s Tomb.', 30, 0, 0, 380280, UNIX_TIMESTAMP(), 1),
(8380380, 3302, 'hunter_live_380380', 'Hunter of Skyshrine', 'Defeat 2 matched live Hunter targets in Skyshrine.', 30, 0, 0, 380380, UNIX_TIMESTAMP(), 1),
(8415380, 3303, 'hunter_live_415380', 'Hunter of The Echo Caverns', 'Defeat 7 matched live Hunter targets in The Echo Caverns.', 30, 0, 0, 415380, UNIX_TIMESTAMP(), 1),
(8415480, 3303, 'hunter_live_415480', 'Hunter of The Acrylia Caverns', 'Defeat 8 matched live Hunter targets in The Acrylia Caverns.', 30, 0, 0, 415480, UNIX_TIMESTAMP(), 1),
(8415680, 3303, 'hunter_live_415680', 'Hunter of The Paludal Caverns', 'Defeat 2 matched live Hunter targets in The Paludal Caverns.', 30, 0, 0, 415680, UNIX_TIMESTAMP(), 1),
(8415780, 3303, 'hunter_live_415780', 'Hunter of The Fungus Grove', 'Defeat 1 matched live Hunter target in The Fungus Grove.', 30, 0, 0, 415780, UNIX_TIMESTAMP(), 1),
(8416180, 3303, 'hunter_live_416180', 'Hunter of Netherbian Lair', 'Defeat 6 matched live Hunter targets in Netherbian Lair.', 30, 0, 0, 416180, UNIX_TIMESTAMP(), 1),
(8416280, 3303, 'hunter_live_416280', 'Hunter of Ssraeshza Temple', 'Defeat 15 matched live Hunter targets in Ssraeshza Temple.', 30, 0, 0, 416280, UNIX_TIMESTAMP(), 1),
(8416380, 3303, 'hunter_live_416380', 'Hunter of Grieg''s End', 'Defeat 1 matched live Hunter target in Grieg''s End.', 30, 0, 0, 416380, UNIX_TIMESTAMP(), 1),
(8416480, 3303, 'hunter_live_416480', 'Hunter of The Deep', 'Defeat 10 matched live Hunter targets in The Deep.', 30, 0, 0, 416480, UNIX_TIMESTAMP(), 1),
(8416580, 3303, 'hunter_live_416580', 'Hunter of Shadeweaver''s Thicket', 'Defeat 1 matched live Hunter target in Shadeweaver''s Thicket.', 30, 0, 0, 416580, UNIX_TIMESTAMP(), 1),
(8416680, 3303, 'hunter_live_416680', 'Hunter of Hollowshade Moor', 'Defeat 11 matched live Hunter targets in Hollowshade Moor.', 30, 0, 0, 416680, UNIX_TIMESTAMP(), 1),
(8416880, 3303, 'hunter_live_416880', 'Hunter of Marus Seru', 'Defeat 1 matched live Hunter target in Marus Seru.', 30, 0, 0, 416880, UNIX_TIMESTAMP(), 1),
(8417080, 3303, 'hunter_live_417080', 'Hunter of The Twilight Sea', 'Defeat 16 matched live Hunter targets in The Twilight Sea.', 30, 0, 0, 417080, UNIX_TIMESTAMP(), 1),
(8417180, 3303, 'hunter_live_417180', 'Hunter of The Grey', 'Defeat 6 matched live Hunter targets in The Grey.', 30, 0, 0, 417180, UNIX_TIMESTAMP(), 1),
(8417280, 3303, 'hunter_live_417280', 'Hunter of The Tenebrous Mountains', 'Defeat 4 matched live Hunter targets in The Tenebrous Mountains.', 30, 0, 0, 417280, UNIX_TIMESTAMP(), 1),
(8417380, 3303, 'hunter_live_417380', 'Hunter of The Maiden''s Eye', 'Defeat 9 matched live Hunter targets in The Maiden''s Eye.', 30, 0, 0, 417380, UNIX_TIMESTAMP(), 1),
(8417480, 3303, 'hunter_live_417480', 'Hunter of The Dawnshroud Peaks', 'Defeat 8 matched live Hunter targets in The Dawnshroud Peaks.', 30, 0, 0, 417480, UNIX_TIMESTAMP(), 1),
(8417580, 3303, 'hunter_live_417580', 'Hunter of The Scarlet Desert', 'Defeat 26 matched live Hunter targets in The Scarlet Desert.', 30, 0, 0, 417580, UNIX_TIMESTAMP(), 1),
(8417680, 3303, 'hunter_live_417680', 'Hunter of The Umbral Plains', 'Defeat 9 matched live Hunter targets in The Umbral Plains.', 30, 0, 0, 417680, UNIX_TIMESTAMP(), 1),
(8417980, 3303, 'hunter_live_417980', 'Hunter of The Akheva Ruins', 'Defeat 8 matched live Hunter targets in The Akheva Ruins.', 30, 0, 0, 417980, UNIX_TIMESTAMP(), 1),
(8480980, 3303, 'hunter_live_480980', 'Hunter of Grimling Forest', 'Defeat 4 matched live Hunter targets in Grimling Forest.', 30, 0, 0, 480980, UNIX_TIMESTAMP(), 1),
(8520080, 3304, 'hunter_live_520080', 'Hunter of the Ruins of Lxanvom', 'Defeat 20 matched live Hunter targets in the Ruins of Lxanvom.', 30, 0, 0, 520080, UNIX_TIMESTAMP(), 1),
(8520180, 3304, 'hunter_live_520180', 'Hunter of The Plane of Justice', 'Defeat 10 matched live Hunter targets in The Plane of Justice.', 30, 0, 0, 520180, UNIX_TIMESTAMP(), 1),
(8520480, 3304, 'hunter_live_520480', 'Hunter of The Plane of Nightmare', 'Defeat 6 matched live Hunter targets in The Plane of Nightmare.', 30, 0, 0, 520480, UNIX_TIMESTAMP(), 1),
(8520580, 3304, 'hunter_live_520580', 'Hunter of The Plane of Disease', 'Defeat 4 matched live Hunter targets in The Plane of Disease.', 30, 0, 0, 520580, UNIX_TIMESTAMP(), 1),
(8520680, 3304, 'hunter_live_520680', 'Hunter of The Plane of Innovation', 'Defeat 1 matched live Hunter target in The Plane of Innovation.', 30, 0, 0, 520680, UNIX_TIMESTAMP(), 1),
(8520780, 3304, 'hunter_live_520780', 'Hunter of Torment, the Plane of Pain', 'Defeat 7 matched live Hunter targets in Torment, the Plane of Pain.', 30, 0, 0, 520780, UNIX_TIMESTAMP(), 1),
(8520880, 3304, 'hunter_live_520880', 'Hunter of The Plane of Valor', 'Defeat 9 matched live Hunter targets in The Plane of Valor.', 30, 0, 0, 520880, UNIX_TIMESTAMP(), 1),
(8520980, 3304, 'hunter_live_520980', 'Hunter of Torden, the Bastion of Thunder', 'Defeat 15 matched live Hunter targets in Torden, the Bastion of Thunder.', 30, 0, 0, 520980, UNIX_TIMESTAMP(), 1),
(8521180, 3304, 'hunter_live_521180', 'Hunter of The Halls of Honor', 'Defeat 6 matched live Hunter targets in The Halls of Honor.', 30, 0, 0, 521180, UNIX_TIMESTAMP(), 1),
(8521280, 3304, 'hunter_live_521280', 'Hunter of The Tower of Solusek Ro', 'Defeat 6 matched live Hunter targets in The Tower of Solusek Ro.', 30, 0, 0, 521280, UNIX_TIMESTAMP(), 1),
(8521480, 3304, 'hunter_live_521480', 'Hunter of Drunder, the Fortress of Zek', 'Defeat 7 matched live Hunter targets in Drunder, the Fortress of Zek.', 30, 0, 0, 521480, UNIX_TIMESTAMP(), 1),
(8521580, 3304, 'hunter_live_521580', 'Hunter of Eryslai, the Kingdom of Wind', 'Defeat 4 matched live Hunter targets in Eryslai, the Kingdom of Wind.', 30, 0, 0, 521580, UNIX_TIMESTAMP(), 1),
(8521680, 3304, 'hunter_live_521680', 'Hunter of The Reef of Coirnav', 'Defeat 17 matched live Hunter targets in The Reef of Coirnav.', 30, 0, 0, 521680, UNIX_TIMESTAMP(), 1),
(8521780, 3304, 'hunter_live_521780', 'Hunter of Doomfire, the Burning Lands', 'Defeat 23 matched live Hunter targets in Doomfire, the Burning Lands.', 30, 0, 0, 521780, UNIX_TIMESTAMP(), 1),
(8522080, 3304, 'hunter_live_522080', 'Hunter of The Temple of Marr', 'Defeat 1 matched live Hunter target in The Temple of Marr.', 30, 0, 0, 522080, UNIX_TIMESTAMP(), 1),
(8522180, 3304, 'hunter_live_522180', 'Hunter of The Lair of Terris-Thule', 'Defeat 2 matched live Hunter targets in The Lair of Terris-Thule.', 30, 0, 0, 522180, UNIX_TIMESTAMP(), 1),
(8127880, 3305, 'hunter_live_127880', 'Hunter of The Caverns of Exile', 'Defeat 9 matched live Hunter targets in The Caverns of Exile.', 30, 0, 0, 127880, UNIX_TIMESTAMP(), 1),
(8622480, 3305, 'hunter_live_622480', 'Hunter of The Gulf of Gunthak', 'Defeat 19 matched live Hunter targets in The Gulf of Gunthak.', 30, 0, 0, 622480, UNIX_TIMESTAMP(), 1),
(8622580, 3305, 'hunter_live_622580', 'Hunter of Dulak''s Harbor', 'Defeat 17 matched live Hunter targets in Dulak''s Harbor.', 30, 0, 0, 622580, UNIX_TIMESTAMP(), 1),
(8622680, 3305, 'hunter_live_622680', 'Hunter of The Torgiran Mines', 'Defeat 20 matched live Hunter targets in The Torgiran Mines.', 30, 0, 0, 622680, UNIX_TIMESTAMP(), 1),
(8622780, 3305, 'hunter_live_622780', 'Hunter of The Crypt of Nadox', 'Defeat 15 matched live Hunter targets in The Crypt of Nadox.', 30, 0, 0, 622780, UNIX_TIMESTAMP(), 1),
(8622880, 3305, 'hunter_live_622880', 'Hunter of Hate''s Fury', 'Defeat 11 matched live Hunter targets in Hate''s Fury.', 30, 0, 0, 622880, UNIX_TIMESTAMP(), 1),
(8714280, 3306, 'hunter_live_714280', 'Hunter of Spider Den', 'Defeat 1 matched live Hunter target in Spider Den.', 30, 0, 0, 714280, UNIX_TIMESTAMP(), 1),
(8745080, 3306, 'hunter_live_745080', 'Hunter of Arena of Chance', 'Defeat 1 matched live Hunter target in Arena of Chance.', 30, 0, 0, 745080, UNIX_TIMESTAMP(), 1)
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000001, 8100480, 0, 'npc_name_kill', 'npc_name', 4147, 'pyzjn', 1, 4, 0, 0),
(18000002, 8100480, 1, 'npc_name_kill', 'npc_name', 4171, 'varsoon', 1, 4, 0, 0),
(18000003, 8101180, 0, 'npc_name_kill', 'npc_name', 11124, 'the sporali moldmaster', 1, 11, 0, 0),
(18000004, 8101380, 0, 'npc_name_kill', 'npc_name', 13108, 'ashenpaw', 1, 13, 0, 0),
(18000005, 8101380, 1, 'npc_name_kill', 'npc_name', 13111, 'bristletoe', 1, 13, 0, 0),
(18000006, 8101380, 2, 'npc_name_kill', 'npc_name', 13115, 'callowwing', 1, 13, 0, 0),
(18000007, 8101380, 3, 'npc_name_kill', 'npc_name', 13117, 'grimtooth', 1, 13, 0, 0),
(18000008, 8101380, 4, 'npc_name_kill', 'npc_name', 13113, 'korvik the cursed', 1, 13, 0, 0),
(18000009, 8101380, 5, 'npc_name_kill', 'npc_name', 13107, 'swiftclaw', 1, 13, 0, 0),
(18000010, 8101380, 6, 'npc_name_kill', 'npc_name', 13121, 'timbur the tiny', 1, 13, 0, 0),
(18000011, 8101480, 0, 'npc_name_kill', 'npc_name', 14142, 'grizzleknot', 1, 14, 0, 0),
(18000012, 8101480, 1, 'npc_name_kill', 'npc_name', 14137, 'knari morawk', 1, 14, 0, 0),
(18000013, 8101480, 2, 'npc_name_kill', 'npc_name', 14197, 'kroldir thunderhoof', 1, 14, 0, 0),
(18000014, 8101480, 3, 'npc_name_kill', 'npc_name', 14134, 'lord grimrot', 1, 14, 0, 0),
(18000015, 8101480, 4, 'npc_name_kill', 'npc_name', 14131, 'marik clubthorn', 1, 14, 0, 0),
(18000016, 8101480, 5, 'npc_name_kill', 'npc_name', 14136, 'mroon', 1, 14, 0, 0),
(18000017, 8101480, 6, 'npc_name_kill', 'npc_name', 14144, 'narra tanith', 1, 14, 0, 0),
(18000018, 8101480, 7, 'npc_name_kill', 'npc_name', 14193, 'nisch val torash mashk', 1, 14, 0, 0),
(18000019, 8101480, 8, 'npc_name_kill', 'npc_name', 14194, 'rosch val l`vlor', 1, 14, 0, 0),
(18000020, 8101480, 9, 'npc_name_kill', 'npc_name', 14125, 'synger foxfyre', 1, 14, 0, 0),
(18000021, 8101480, 10, 'npc_name_kill', 'npc_name', 14192, 'tesch val deval`nmak', 1, 14, 0, 0),
(18000022, 8101480, 11, 'npc_name_kill', 'npc_name', 14195, 'tesch val kadvem', 1, 14, 0, 0),
(18000023, 8101880, 0, 'npc_name_kill', 'npc_name', 18138, 'nisch val torash mashk', 1, 18, 0, 0),
(18000024, 8101880, 1, 'npc_name_kill', 'npc_name', 18141, 'rosch val l`vlor', 1, 18, 0, 0),
(18000025, 8101880, 2, 'npc_name_kill', 'npc_name', 18142, 'tesch val deval`nmak', 1, 18, 0, 0),
(18000026, 8101880, 3, 'npc_name_kill', 'npc_name', 18122, 'tesch val kadvem', 1, 18, 0, 0),
(18000027, 8102780, 0, 'npc_name_kill', 'npc_name', 27152, 'a lesser nightmare', 1, 27, 0, 0),
(18000028, 8102780, 1, 'npc_name_kill', 'npc_name', 27168, 'tisella', 1, 27, 0, 0),
(18000029, 8102780, 2, 'npc_name_kill', 'npc_name', 27148, 'a warbone monk', 1, 27, 0, 0),
(18000030, 8102780, 3, 'npc_name_kill', 'npc_name', 27149, 'a warbone spearman', 1, 27, 0, 0),
(18000031, 8103180, 0, 'npc_name_kill', 'npc_name', 31147, 'captain bipnubble', 1, 31, 0, 0),
(18000032, 8103180, 1, 'npc_name_kill', 'npc_name', 31118, 'cwg model exg', 1, 31, 0, 0),
(18000033, 8103180, 2, 'npc_name_kill', 'npc_name', 31145, 'fire goblin bartender', 1, 31, 0, 0),
(18000034, 8103180, 3, 'npc_name_kill', 'npc_name', 31124, 'flame goblin foreman', 1, 31, 0, 0),
(18000035, 8103180, 4, 'npc_name_kill', 'npc_name', 31132, 'gabbie mardoddle', 1, 31, 0, 0),
(18000036, 8103180, 5, 'npc_name_kill', 'npc_name', 31022, 'goblin high shaman', 1, 31, 0, 0),
(18000037, 8103180, 6, 'npc_name_kill', 'npc_name', 31134, 'inferno goblin captain', 1, 31, 0, 0),
(18000038, 8103180, 7, 'npc_name_kill', 'npc_name', 31144, 'inferno goblin torturer', 1, 31, 0, 0),
(18000039, 8103180, 8, 'npc_name_kill', 'npc_name', 31136, 'kindle', 1, 31, 0, 0),
(18000040, 8103180, 9, 'npc_name_kill', 'npc_name', 31146, 'kobold predator', 1, 31, 0, 0),
(18000041, 8103180, 10, 'npc_name_kill', 'npc_name', 31143, 'lava elemental', 1, 31, 0, 0),
(18000042, 8103180, 11, 'npc_name_kill', 'npc_name', 31085, 'reckless efreeti', 1, 31, 0, 0),
(18000043, 8103180, 12, 'npc_name_kill', 'npc_name', 31127, 'singe', 1, 31, 0, 0),
(18000044, 8103180, 13, 'npc_name_kill', 'npc_name', 31128, 'solusek goblin king', 1, 31, 0, 0),
(18000045, 8103280, 0, 'npc_name_kill', 'npc_name', 32062, 'efreeti lord djarn', 1, 32, 0, 0),
(18000046, 8103280, 1, 'npc_name_kill', 'npc_name', 32022, 'king tranix', 1, 32, 0, 0),
(18000047, 8103280, 2, 'npc_name_kill', 'npc_name', 32041, 'magus rokyl', 1, 32, 0, 0),
(18000048, 8103280, 3, 'npc_name_kill', 'npc_name', 32020, 'warlord skarlon', 1, 32, 0, 0),
(18000049, 8103680, 0, 'npc_name_kill', 'npc_name', 36095, 'priest amiaz', 1, 36, 0, 0),
(18000050, 8103680, 1, 'npc_name_kill', 'npc_name', 36097, 'the thaumaturgist', 1, 36, 0, 0),
(18000051, 8103980, 0, 'npc_name_kill', 'npc_name', 39158, 'bejeweled elemental', 1, 39, 0, 0),
(18000052, 8103980, 1, 'npc_name_kill', 'npc_name', 39141, 'commander yarik', 1, 39, 0, 0),
(18000053, 8103980, 2, 'npc_name_kill', 'npc_name', 39142, 'gibartik', 1, 39, 0, 0),
(18000054, 8103980, 3, 'npc_name_kill', 'npc_name', 39150, 'initiate sirlis', 1, 39, 0, 0),
(18000055, 8103980, 4, 'npc_name_kill', 'npc_name', 39162, 'irslak the wretched', 1, 39, 0, 0),
(18000056, 8103980, 5, 'npc_name_kill', 'npc_name', 39151, 'muck covered elemental', 1, 39, 0, 0),
(18000057, 8103980, 6, 'npc_name_kill', 'npc_name', 39149, 'niltoth the unholy', 1, 39, 0, 0),
(18000058, 8103980, 7, 'npc_name_kill', 'npc_name', 39143, 'retseth tretse', 1, 39, 0, 0),
(18000059, 8103980, 8, 'npc_name_kill', 'npc_name', 39134, 'rocksoul', 1, 39, 0, 0),
(18000060, 8103980, 9, 'npc_name_kill', 'npc_name', 39135, 'slizik the mighty', 1, 39, 0, 0),
(18000061, 8103980, 10, 'npc_name_kill', 'npc_name', 39153, 'stonegrinder minion', 1, 39, 0, 0),
(18000062, 8103980, 11, 'npc_name_kill', 'npc_name', 39132, 'stonesoul the unmoving', 1, 39, 0, 0),
(18000063, 8103980, 12, 'npc_name_kill', 'npc_name', 39152, 'ulrik the devout', 1, 39, 0, 0),
(18000064, 8104480, 0, 'npc_name_kill', 'npc_name', 44024, 'rathyl', 1, 44, 0, 0),
(18000065, 8104580, 0, 'npc_name_kill', 'npc_name', 45123, 'an injured rat', 1, 45, 0, 0),
(18000066, 8104580, 1, 'npc_name_kill', 'npc_name', 45121, 'a nesting rat', 1, 45, 0, 0),
(18000067, 8104580, 2, 'npc_name_kill', 'npc_name', 45125, 'a shady mercenary', 1, 45, 0, 0),
(18000068, 8104780, 0, 'npc_name_kill', 'npc_name', 47199, 'a silverflank guardian', 1, 47, 0, 0),
(18000069, 8104780, 1, 'npc_name_kill', 'npc_name', 47204, 'tae ew archon', 1, 47, 0, 0),
(18000070, 8104780, 2, 'npc_name_kill', 'npc_name', 47203, 'tae ew diviner', 1, 47, 0, 0),
(18000071, 8104780, 3, 'npc_name_kill', 'npc_name', 47182, 'a tae ew spiritualist', 1, 47, 0, 0),
(18000072, 8104780, 4, 'npc_name_kill', 'npc_name', 47198, 'tae ew templar', 1, 47, 0, 0),
(18000073, 8104780, 5, 'npc_name_kill', 'npc_name', 47195, 'thul tae ew cenobite', 1, 47, 0, 0),
(18000074, 8105080, 0, 'npc_name_kill', 'npc_name', 50057, 'bloodneedle', 1, 50, 0, 0),
(18000075, 8105080, 1, 'npc_name_kill', 'npc_name', 50348, 'mortificator syythrak', 1, 50, 0, 0),
(18000076, 8105080, 2, 'npc_name_kill', 'npc_name', 50350, 'oculys ogrefiend', 1, 50, 0, 0),
(18000077, 8105080, 3, 'npc_name_kill', 'npc_name', 50347, 'petrifin', 1, 50, 0, 0),
(18000078, 8105080, 4, 'npc_name_kill', 'npc_name', 50351, 'quid rilstone', 1, 50, 0, 0),
(18000079, 8105080, 5, 'npc_name_kill', 'npc_name', 50349, 'shardwing', 1, 50, 0, 0),
(18000080, 8105180, 0, 'npc_name_kill', 'npc_name', 51131, 'a gnoll embalmer', 1, 51, 0, 0),
(18000081, 8105780, 0, 'npc_name_kill', 'npc_name', 57105, 'crookstinger', 1, 57, 0, 0),
(18000082, 8105780, 1, 'npc_name_kill', 'npc_name', 57120, 'mina glimmerwing', 1, 57, 0, 0),
(18000083, 8105780, 2, 'npc_name_kill', 'npc_name', 57108, 'old dimshimmer', 1, 57, 0, 0),
(18000084, 8105780, 3, 'npc_name_kill', 'npc_name', 57005, 'queen nasheeji', 1, 57, 0, 0),
(18000085, 8105780, 4, 'npc_name_kill', 'npc_name', 57002, 'whimsy larktwitter', 1, 57, 0, 0),
(18000086, 8105880, 0, 'npc_name_kill', 'npc_name', 58040, 'orc taskmaster', 1, 58, 0, 0),
(18000087, 8105880, 1, 'npc_name_kill', 'npc_name', 58002, 'orc warlord', 1, 58, 0, 0),
(18000088, 8105980, 0, 'npc_name_kill', 'npc_name', 59020, 'an avenging caitiff', 1, 59, 0, 0),
(18000089, 8105980, 1, 'npc_name_kill', 'npc_name', 59116, 'butler syncall', 1, 59, 0, 0),
(18000090, 8105980, 2, 'npc_name_kill', 'npc_name', 59102, 'a cloaked dhampyre', 1, 59, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000091, 8105980, 3, 'npc_name_kill', 'npc_name', 59151, 'garton viswin', 1, 59, 0, 0),
(18000092, 8105980, 4, 'npc_name_kill', 'npc_name', 59133, 'a glyphed ghoul', 1, 59, 0, 0),
(18000093, 8105980, 5, 'npc_name_kill', 'npc_name', 59089, 'lasna cheroon', 1, 59, 0, 0),
(18000094, 8105980, 6, 'npc_name_kill', 'npc_name', 59115, 'maid issis', 1, 59, 0, 0),
(18000095, 8106380, 0, 'npc_name_kill', 'npc_name', 63062, 'garanel rucksif', 1, 63, 0, 0),
(18000096, 8106380, 1, 'npc_name_kill', 'npc_name', 63092, 'lesser blade fiend', 1, 63, 0, 0),
(18000097, 8106380, 2, 'npc_name_kill', 'npc_name', 63075, 'a reanimated hand', 1, 63, 0, 0),
(18000098, 8106380, 3, 'npc_name_kill', 'npc_name', 63086, 'reclusive ghoul magus', 1, 63, 0, 0),
(18000099, 8106380, 4, 'npc_name_kill', 'npc_name', 63010, 'an undead barkeep', 1, 63, 0, 0),
(18000100, 8106380, 5, 'npc_name_kill', 'npc_name', 63003, 'an undead knight of unrest', 1, 63, 0, 0),
(18000101, 8106480, 0, 'npc_name_kill', 'npc_name', 64089, 'cauldronboil', 1, 64, 0, 0),
(18000102, 8106480, 1, 'npc_name_kill', 'npc_name', 64107, 'coralyn kelpmaiden', 1, 64, 0, 0),
(18000103, 8106480, 2, 'npc_name_kill', 'npc_name', 64101, 'a ferocious hammerhead', 1, 64, 0, 0),
(18000104, 8106480, 3, 'npc_name_kill', 'npc_name', 64103, 'a fierce impaler', 1, 64, 0, 0),
(18000105, 8106480, 4, 'npc_name_kill', 'npc_name', 64102, 'a frenzied cauldron shark', 1, 64, 0, 0),
(18000106, 8106480, 5, 'npc_name_kill', 'npc_name', 64105, 'a seahorse matriarch', 1, 64, 0, 0),
(18000107, 8106480, 6, 'npc_name_kill', 'npc_name', 64106, 'a seahorse patriarch', 1, 64, 0, 0),
(18000108, 8106480, 7, 'npc_name_kill', 'npc_name', 64093, 'shellara ebbhunter', 1, 64, 0, 0),
(18000109, 8106480, 8, 'npc_name_kill', 'npc_name', 64087, 'undertow', 1, 64, 0, 0),
(18000110, 8106580, 0, 'npc_name_kill', 'npc_name', 65140, 'a froglok gaz squire', 1, 65, 0, 0),
(18000111, 8106580, 1, 'npc_name_kill', 'npc_name', 65105, 'a froglok realist', 1, 65, 0, 0),
(18000112, 8106580, 2, 'npc_name_kill', 'npc_name', 65148, 'a froglok scryer', 1, 65, 0, 0),
(18000113, 8106580, 3, 'npc_name_kill', 'npc_name', 65128, 'the froglok shin lord', 1, 65, 0, 0),
(18000114, 8106580, 4, 'npc_name_kill', 'npc_name', 65125, 'a froglok summoner', 1, 65, 0, 0),
(18000115, 8106580, 5, 'npc_name_kill', 'npc_name', 65104, 'the froglok warden', 1, 65, 0, 0),
(18000116, 8106580, 6, 'npc_name_kill', 'npc_name', 65146, 'a giant heart spider', 1, 65, 0, 0),
(18000117, 8106680, 0, 'npc_name_kill', 'npc_name', 66146, 'a frenzied ghoul', 1, 66, 0, 0),
(18000118, 8106680, 1, 'npc_name_kill', 'npc_name', 66153, 'a froglok crusader', 1, 66, 0, 0),
(18000119, 8106680, 2, 'npc_name_kill', 'npc_name', 66160, 'a froglok herbalist', 1, 66, 0, 0),
(18000120, 8106680, 3, 'npc_name_kill', 'npc_name', 66159, 'the froglok king', 1, 66, 0, 0),
(18000121, 8106680, 4, 'npc_name_kill', 'npc_name', 66175, 'a froglok noble', 1, 66, 0, 0),
(18000122, 8106680, 5, 'npc_name_kill', 'npc_name', 66121, 'a froglok tactician', 1, 66, 0, 0),
(18000123, 8106680, 6, 'npc_name_kill', 'npc_name', 66120, 'a froglok yun priest', 1, 66, 0, 0),
(18000124, 8106680, 7, 'npc_name_kill', 'npc_name', 66156, 'the ghoul arch magus', 1, 66, 0, 0),
(18000125, 8106680, 8, 'npc_name_kill', 'npc_name', 66171, 'a ghoul assassin', 1, 66, 0, 0),
(18000126, 8106680, 9, 'npc_name_kill', 'npc_name', 66169, 'a ghoul cavalier', 1, 66, 0, 0),
(18000127, 8106680, 10, 'npc_name_kill', 'npc_name', 66092, 'a ghoul executioner', 1, 66, 0, 0),
(18000128, 8106680, 11, 'npc_name_kill', 'npc_name', 66005, 'the ghoul lord', 1, 66, 0, 0),
(18000129, 8106680, 12, 'npc_name_kill', 'npc_name', 66108, 'a ghoul ritualist', 1, 66, 0, 0),
(18000130, 8106680, 13, 'npc_name_kill', 'npc_name', 66165, 'a ghoul sage', 1, 66, 0, 0),
(18000131, 8106680, 14, 'npc_name_kill', 'npc_name', 66100, 'a ghoul savant', 1, 66, 0, 0),
(18000132, 8106680, 15, 'npc_name_kill', 'npc_name', 66163, 'a ghoul sentinel', 1, 66, 0, 0),
(18000133, 8106680, 16, 'npc_name_kill', 'npc_name', 66021, 'a ghoul supplier', 1, 66, 0, 0),
(18000134, 8106680, 17, 'npc_name_kill', 'npc_name', 66178, 'a minotaur elder', 1, 66, 0, 0),
(18000135, 8106680, 18, 'npc_name_kill', 'npc_name', 66173, 'a minotaur patriarch', 1, 66, 0, 0),
(18000136, 8106680, 19, 'npc_name_kill', 'npc_name', 66036, 'a reanimated hand', 1, 66, 0, 0),
(18000137, 8106880, 0, 'npc_name_kill', 'npc_name', 68137, 'glubbsink', 1, 68, 0, 0),
(18000138, 8107080, 0, 'npc_name_kill', 'npc_name', 70047, 'barnacle bones', 1, 70, 0, 0),
(18000139, 8107080, 1, 'npc_name_kill', 'npc_name', 70054, 'flotsam', 1, 70, 0, 0),
(18000140, 8107080, 2, 'npc_name_kill', 'npc_name', 70057, 'jetsam', 1, 70, 0, 0),
(18000141, 8107080, 3, 'npc_name_kill', 'npc_name', 70056, 'squallslither', 1, 70, 0, 0),
(18000142, 8107380, 0, 'npc_name_kill', 'npc_name', 73004, 'an elite honor guard', 1, 73, 0, 0),
(18000143, 8107380, 1, 'npc_name_kill', 'npc_name', 73104, 'a goblin jailmaster', 1, 73, 0, 0),
(18000144, 8107380, 2, 'npc_name_kill', 'npc_name', 73068, 'a goblin preacher', 1, 73, 0, 0),
(18000145, 8107380, 3, 'npc_name_kill', 'npc_name', 73022, 'a goblin scryer', 1, 73, 0, 0),
(18000146, 8107380, 4, 'npc_name_kill', 'npc_name', 73006, 'high priest zaharn', 1, 73, 0, 0),
(18000147, 8107380, 5, 'npc_name_kill', 'npc_name', 73103, 'king thex`ka iv', 1, 73, 0, 0),
(18000148, 8110080, 0, 'npc_name_kill', 'npc_name', 100214, 'arglar the tormentor', 1, 100, 0, 0),
(18000149, 8110080, 1, 'npc_name_kill', 'npc_name', 100208, 'giang yin', 1, 100, 0, 0),
(18000150, 8110080, 2, 'npc_name_kill', 'npc_name', 100198, 'hurglak the destroyer', 1, 100, 0, 0),
(18000151, 8110080, 3, 'npc_name_kill', 'npc_name', 100203, 'jelquar the soulslayer', 1, 100, 0, 0),
(18000152, 8110080, 4, 'npc_name_kill', 'npc_name', 100199, 'rendolr the maimer', 1, 100, 0, 0),
(18000153, 8110080, 5, 'npc_name_kill', 'npc_name', 100076, 'slyder the ancient', 1, 100, 0, 0),
(18000154, 8110080, 6, 'npc_name_kill', 'npc_name', 100097, 'snowbeast', 1, 100, 0, 0),
(18000155, 8110180, 0, 'npc_name_kill', 'npc_name', 101132, 'packmaster dledsh', 1, 101, 0, 0),
(18000156, 8110180, 1, 'npc_name_kill', 'npc_name', 101138, 'warlord drrig', 1, 101, 0, 0),
(18000157, 8118180, 0, 'npc_name_kill', 'npc_name', 181212, 'elishia blackguard', 1, 181, 0, 0),
(18000158, 8118180, 1, 'npc_name_kill', 'npc_name', 181207, 'goldentalon', 1, 181, 0, 0),
(18000159, 8118180, 2, 'npc_name_kill', 'npc_name', 181204, 'lameriae the alluring', 1, 181, 0, 0),
(18000160, 8118180, 3, 'npc_name_kill', 'npc_name', 181053, 'reynold blackguard', 1, 181, 0, 0),
(18000161, 8118180, 4, 'npc_name_kill', 'npc_name', 181194, 'vaurien sticklebush', 1, 181, 0, 0),
(18000162, 8118180, 5, 'npc_name_kill', 'npc_name', 181162, 'zed sticklebush', 1, 181, 0, 0),
(18000163, 8118280, 0, 'npc_name_kill', 'npc_name', 182070, 'a magnificent grizzly', 1, 182, 0, 0),
(18000164, 8138480, 0, 'npc_name_kill', 'npc_name', 384026, 'bad ash', 1, 384, 0, 0),
(18000165, 8139280, 0, 'npc_name_kill', 'npc_name', 392069, 'rahotep', 1, 392, 0, 0),
(18000166, 8140780, 0, 'npc_name_kill', 'npc_name', 5133, 'grenix mucktail', 1, 5, 0, 0),
(18000167, 8140780, 1, 'npc_name_kill', 'npc_name', 5128, 'hagnis shralok', 1, 5, 0, 0),
(18000168, 8140780, 2, 'npc_name_kill', 'npc_name', 5135, 'recfek shralok', 1, 5, 0, 0),
(18000169, 8140780, 3, 'npc_name_kill', 'npc_name', 5127, 'vopuk shralok', 1, 5, 0, 0),
(18000170, 8141580, 0, 'npc_name_kill', 'npc_name', 33154, 'a goblin alchemist', 1, 33, 0, 0),
(18000171, 8153980, 0, 'npc_name_kill', 'npc_name', 39067, 'an ancient construct', 1, 39, 0, 0),
(18000172, 8153980, 1, 'npc_name_kill', 'npc_name', 39117, 'an ancient gargoyle', 1, 39, 0, 0),
(18000173, 8153980, 2, 'npc_name_kill', 'npc_name', 39065, 'an old construct', 1, 39, 0, 0),
(18000174, 8153980, 3, 'npc_name_kill', 'npc_name', 39118, 'an old gargoyle', 1, 39, 0, 0),
(18000175, 8153980, 4, 'npc_name_kill', 'npc_name', 39031, 'a ratman inhabitant', 1, 39, 0, 0),
(18000176, 8153980, 5, 'npc_name_kill', 'npc_name', 39074, 'a temple researcher', 1, 39, 0, 0),
(18000177, 8154880, 0, 'npc_name_kill', 'npc_name', 48220, 'a barbed scale piranha', 1, 48, 0, 0),
(18000178, 8154880, 1, 'npc_name_kill', 'npc_name', 48017, 'a blood claw raptor', 1, 48, 0, 0),
(18000179, 8154880, 2, 'npc_name_kill', 'npc_name', 48121, 'a blood fin piranha', 1, 48, 0, 0),
(18000180, 8154880, 3, 'npc_name_kill', 'npc_name', 48136, 'a crystaline mass', 1, 48, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000181, 8154880, 4, 'npc_name_kill', 'npc_name', 48223, 'a diamond scale piranha', 1, 48, 0, 0),
(18000182, 8154880, 5, 'npc_name_kill', 'npc_name', 48119, 'a disciple of thule', 1, 48, 0, 0),
(18000183, 8154880, 6, 'npc_name_kill', 'npc_name', 48122, 'a diseased mosquito', 1, 48, 0, 0),
(18000184, 8154880, 7, 'npc_name_kill', 'npc_name', 48219, 'dismay', 1, 48, 0, 0),
(18000185, 8154880, 8, 'npc_name_kill', 'npc_name', 48156, 'dreadfang', 1, 48, 0, 0),
(18000186, 8154880, 9, 'npc_name_kill', 'npc_name', 48165, 'an enraged amygdalan', 1, 48, 0, 0),
(18000187, 8154880, 10, 'npc_name_kill', 'npc_name', 48216, 'an enraged disciple', 1, 48, 0, 0),
(18000188, 8154880, 11, 'npc_name_kill', 'npc_name', 48177, 'an enraged jungle raptor', 1, 48, 0, 0),
(18000189, 8154880, 12, 'npc_name_kill', 'npc_name', 48182, 'an enraged tiger raptor', 1, 48, 0, 0),
(18000190, 8154880, 13, 'npc_name_kill', 'npc_name', 48236, 'an envenomed hunter', 1, 48, 0, 0),
(18000191, 8154880, 14, 'npc_name_kill', 'npc_name', 48170, 'a frenzied shiverback', 1, 48, 0, 0),
(18000192, 8154880, 15, 'npc_name_kill', 'npc_name', 48241, 'frightchaser', 1, 48, 0, 0),
(18000193, 8154880, 16, 'npc_name_kill', 'npc_name', 48150, 'a gelatinous mass', 1, 48, 0, 0),
(18000194, 8154880, 17, 'npc_name_kill', 'npc_name', 48224, 'a graystriped mosquito', 1, 48, 0, 0),
(18000195, 8154880, 18, 'npc_name_kill', 'npc_name', 48189, 'a gyrating mass', 1, 48, 0, 0),
(18000196, 8154880, 19, 'npc_name_kill', 'npc_name', 48225, 'a noxious jungle spider', 1, 48, 0, 0),
(18000197, 8154880, 20, 'npc_name_kill', 'npc_name', 48183, 'a poisonstrand hunter', 1, 48, 0, 0),
(18000198, 8154880, 21, 'npc_name_kill', 'npc_name', 48147, 'a quivering mass', 1, 48, 0, 0),
(18000199, 8154880, 22, 'npc_name_kill', 'npc_name', 48152, 'a razor fin piranha', 1, 48, 0, 0),
(18000200, 8154880, 23, 'npc_name_kill', 'npc_name', 48226, 'a razor tooth piranha', 1, 48, 0, 0),
(18000201, 8154880, 24, 'npc_name_kill', 'npc_name', 48192, 'a rotting horror', 1, 48, 0, 0),
(18000202, 8154880, 25, 'npc_name_kill', 'npc_name', 48078, 'a rotting shiverback', 1, 48, 0, 0),
(18000203, 8154880, 26, 'npc_name_kill', 'npc_name', 48242, 'silverfang', 1, 48, 0, 0),
(18000204, 8154880, 27, 'npc_name_kill', 'npc_name', 48153, 'a silverflank shiverback', 1, 48, 0, 0),
(18000205, 8154880, 28, 'npc_name_kill', 'npc_name', 48202, 'soul siphon', 1, 48, 0, 0),
(18000206, 8154880, 29, 'npc_name_kill', 'npc_name', 48149, 'a swirling black mass', 1, 48, 0, 0),
(18000207, 8154880, 30, 'npc_name_kill', 'npc_name', 48203, 'a swirling green mass', 1, 48, 0, 0),
(18000208, 8154880, 31, 'npc_name_kill', 'npc_name', 48151, 'a swirling red mass', 1, 48, 0, 0),
(18000209, 8154880, 32, 'npc_name_kill', 'npc_name', 48194, 'a tae ew aggressor', 1, 48, 0, 0),
(18000210, 8154880, 33, 'npc_name_kill', 'npc_name', 48145, 'a tae ew bloodfiend', 1, 48, 0, 0),
(18000211, 8154880, 34, 'npc_name_kill', 'npc_name', 48161, 'a tae ew convert', 1, 48, 0, 0),
(18000212, 8154880, 35, 'npc_name_kill', 'npc_name', 48232, 'a tae ew prophet', 1, 48, 0, 0),
(18000213, 8154880, 36, 'npc_name_kill', 'npc_name', 48233, 'a tae ew spear fisher', 1, 48, 0, 0),
(18000214, 8154880, 37, 'npc_name_kill', 'npc_name', 48178, 'a tae ew trapper', 1, 48, 0, 0),
(18000215, 8154880, 38, 'npc_name_kill', 'npc_name', 48171, 'a tae ew warlord', 1, 48, 0, 0),
(18000216, 8154880, 39, 'npc_name_kill', 'npc_name', 48244, 'a tae ew warmaster', 1, 48, 0, 0),
(18000217, 8154880, 40, 'npc_name_kill', 'npc_name', 48217, 'terrorclaw', 1, 48, 0, 0),
(18000218, 8154880, 41, 'npc_name_kill', 'npc_name', 48193, 'a thul tae ew adept', 1, 48, 0, 0),
(18000219, 8154880, 42, 'npc_name_kill', 'npc_name', 48159, 'a thul tae ew crusader', 1, 48, 0, 0),
(18000220, 8154880, 43, 'npc_name_kill', 'npc_name', 48158, 'a thul tae ew despoiler', 1, 48, 0, 0),
(18000221, 8154880, 44, 'npc_name_kill', 'npc_name', 48018, 'a thul tae ew ritualist', 1, 48, 0, 0),
(18000222, 8154880, 45, 'npc_name_kill', 'npc_name', 48141, 'a thul tae ew spirtcaller', 1, 48, 0, 0),
(18000223, 8154880, 46, 'npc_name_kill', 'npc_name', 48118, 'a thul tae ew torturer', 1, 48, 0, 0),
(18000224, 8154880, 47, 'npc_name_kill', 'npc_name', 48185, 'a toxic jungle hunter', 1, 48, 0, 0),
(18000225, 8154880, 48, 'npc_name_kill', 'npc_name', 48243, 'toxiferious', 1, 48, 0, 0),
(18000226, 8154880, 49, 'npc_name_kill', 'npc_name', 48235, 'a virulent mosquito', 1, 48, 0, 0),
(18000227, 8158680, 0, 'npc_name_kill', 'npc_name', 186177, 'arcanist v`gimis', 1, 186, 0, 0),
(18000228, 8158680, 1, 'npc_name_kill', 'npc_name', 186176, 'arch lich t`vaxok', 1, 186, 0, 0),
(18000229, 8158680, 2, 'npc_name_kill', 'npc_name', 186175, 'archon g`uvin', 1, 186, 0, 0),
(18000230, 8158680, 3, 'npc_name_kill', 'npc_name', 186161, 'an ashenbone broodmaster', 1, 186, 0, 0),
(18000231, 8158680, 4, 'npc_name_kill', 'npc_name', 186167, 'assassin z`jrix', 1, 186, 0, 0),
(18000232, 8158680, 5, 'npc_name_kill', 'npc_name', 186163, 'avatar of abhorrence', 1, 186, 0, 0),
(18000233, 8158680, 6, 'npc_name_kill', 'npc_name', 186160, 'coercer t`vala', 1, 186, 0, 0),
(18000234, 8158680, 7, 'npc_name_kill', 'npc_name', 186170, 'dread knight t`kamax', 1, 186, 0, 0),
(18000235, 8158680, 8, 'npc_name_kill', 'npc_name', 186162, 'an eerie chest', 1, 186, 0, 0),
(18000236, 8158680, 9, 'npc_name_kill', 'npc_name', 186169, 'evangelist w`rixxus', 1, 186, 0, 0),
(18000237, 8158680, 10, 'npc_name_kill', 'npc_name', 186179, 'a frightful chest', 1, 186, 0, 0),
(18000238, 8158680, 11, 'npc_name_kill', 'npc_name', 186183, 'grandmaster h`qilm', 1, 186, 0, 0),
(18000239, 8158680, 12, 'npc_name_kill', 'npc_name', 186186, 'grim abhorrent kaltik', 1, 186, 0, 0),
(18000240, 8158680, 13, 'npc_name_kill', 'npc_name', 186185, 'a grotesque rat', 1, 186, 0, 0),
(18000241, 8158680, 14, 'npc_name_kill', 'npc_name', 186180, 'a hatebone broodlord', 1, 186, 0, 0),
(18000242, 8158680, 15, 'npc_name_kill', 'npc_name', 186166, 'a hideous rat', 1, 186, 0, 0),
(18000243, 8158680, 16, 'npc_name_kill', 'npc_name', 186187, 'lord of fury', 1, 186, 0, 0),
(18000244, 8158680, 17, 'npc_name_kill', 'npc_name', 186154, 'lord of ire', 1, 186, 0, 0),
(18000245, 8158680, 18, 'npc_name_kill', 'npc_name', 186155, 'lord of loathing', 1, 186, 0, 0),
(18000246, 8158680, 19, 'npc_name_kill', 'npc_name', 186157, 'magi p`tasa', 1, 186, 0, 0),
(18000247, 8158680, 20, 'npc_name_kill', 'npc_name', 186184, 'master d`samni', 1, 186, 0, 0),
(18000248, 8158680, 21, 'npc_name_kill', 'npc_name', 186165, 'master of spite', 1, 186, 0, 0),
(18000249, 8158680, 22, 'npc_name_kill', 'npc_name', 186156, 'master r`tal', 1, 186, 0, 0),
(18000250, 8158680, 23, 'npc_name_kill', 'npc_name', 186151, 'mistress a`zara', 1, 186, 0, 0),
(18000251, 8158680, 24, 'npc_name_kill', 'npc_name', 186181, 'mistress of malevolence', 1, 186, 0, 0),
(18000252, 8158680, 25, 'npc_name_kill', 'npc_name', 186159, 'mistress of scorn', 1, 186, 0, 0),
(18000253, 8158680, 26, 'npc_name_kill', 'npc_name', 186173, 'overlord r`gahbsa', 1, 186, 0, 0),
(18000254, 8158680, 27, 'npc_name_kill', 'npc_name', 186168, 'sorcerer c`gazin', 1, 186, 0, 0),
(18000255, 8158680, 28, 'npc_name_kill', 'npc_name', 186172, 'spy master i`kavin', 1, 186, 0, 0),
(18000256, 8158680, 29, 'npc_name_kill', 'npc_name', 186188, 'templar j`rosix', 1, 186, 0, 0),
(18000257, 8158680, 30, 'npc_name_kill', 'npc_name', 186150, 'thought destroyer', 1, 186, 0, 0),
(18000258, 8158680, 31, 'npc_name_kill', 'npc_name', 186164, 'vicar m`kari', 1, 186, 0, 0),
(18000259, 8158680, 32, 'npc_name_kill', 'npc_name', 186182, 'warlock j`rath', 1, 186, 0, 0),
(18000260, 8158680, 33, 'npc_name_kill', 'npc_name', 186171, 'warlord e`prosio', 1, 186, 0, 0),
(18000261, 8207880, 0, 'npc_name_kill', 'npc_name', 78180, 'burynai cutter', 1, 78, 0, 0),
(18000262, 8207880, 1, 'npc_name_kill', 'npc_name', 78183, 'carrion queen', 1, 78, 0, 0),
(18000263, 8207880, 2, 'npc_name_kill', 'npc_name', 78149, 'scourgetail scorpion', 1, 78, 0, 0),
(18000264, 8207980, 0, 'npc_name_kill', 'npc_name', 79015, 'grachnist the destroyer', 1, 79, 0, 0),
(18000265, 8207980, 1, 'npc_name_kill', 'npc_name', 79116, 'iksar bandit lord', 1, 79, 0, 0),
(18000266, 8208180, 0, 'npc_name_kill', 'npc_name', 81187, 'chief rokgus', 1, 81, 0, 0),
(18000267, 8208180, 1, 'npc_name_kill', 'npc_name', 81200, 'soothsayer dregzak', 1, 81, 0, 0),
(18000268, 8208380, 0, 'npc_name_kill', 'npc_name', 83222, 'blackbone', 1, 83, 0, 0),
(18000269, 8208380, 1, 'npc_name_kill', 'npc_name', 83193, 'bloodgorge', 1, 83, 0, 0),
(18000270, 8208380, 2, 'npc_name_kill', 'npc_name', 83229, 'bloodvein', 1, 83, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000271, 8208380, 3, 'npc_name_kill', 'npc_name', 83238, 'bulsgor', 1, 83, 0, 0),
(18000272, 8208380, 4, 'npc_name_kill', 'npc_name', 83223, 'deadeye', 1, 83, 0, 0),
(18000273, 8208380, 5, 'npc_name_kill', 'npc_name', 83217, 'dreesix ghoultongue', 1, 83, 0, 0),
(18000274, 8208380, 6, 'npc_name_kill', 'npc_name', 83195, 'ebon bloodrose', 1, 83, 0, 0),
(18000275, 8208380, 7, 'npc_name_kill', 'npc_name', 83075, 'fakraa the forsaken', 1, 83, 0, 0),
(18000276, 8208380, 8, 'npc_name_kill', 'npc_name', 83239, 'fangor', 1, 83, 0, 0),
(18000277, 8208380, 9, 'npc_name_kill', 'npc_name', 83236, 'frayk', 1, 83, 0, 0),
(18000278, 8208380, 10, 'npc_name_kill', 'npc_name', 83206, 'grimewurm', 1, 83, 0, 0),
(18000279, 8208380, 11, 'npc_name_kill', 'npc_name', 83233, 'grizshnok', 1, 83, 0, 0),
(18000280, 8208380, 12, 'npc_name_kill', 'npc_name', 83220, 'heartblood fern', 1, 83, 0, 0),
(18000281, 8208380, 13, 'npc_name_kill', 'npc_name', 83240, 'old hangman', 1, 83, 0, 0),
(18000282, 8208380, 14, 'npc_name_kill', 'npc_name', 83226, 'soblohg', 1, 83, 0, 0),
(18000283, 8208380, 15, 'npc_name_kill', 'npc_name', 83232, 'two tails', 1, 83, 0, 0),
(18000284, 8208380, 16, 'npc_name_kill', 'npc_name', 83219, 'venomwing', 1, 83, 0, 0),
(18000285, 8208380, 17, 'npc_name_kill', 'npc_name', 83237, 'weeping mantrap', 1, 83, 0, 0),
(18000286, 8208580, 0, 'npc_name_kill', 'npc_name', 85090, 'a lead explorer', 1, 85, 0, 0),
(18000287, 8208680, 0, 'npc_name_kill', 'npc_name', 86137, 'a dread widow', 1, 86, 0, 0),
(18000288, 8208680, 1, 'npc_name_kill', 'npc_name', 86144, 'a mountain giant patriarch', 1, 86, 0, 0),
(18000289, 8208680, 2, 'npc_name_kill', 'npc_name', 86044, 'a ravishing drolvarg', 1, 86, 0, 0),
(18000290, 8208780, 0, 'npc_name_kill', 'npc_name', 87113, 'gorgul paclock', 1, 87, 0, 0),
(18000291, 8208780, 1, 'npc_name_kill', 'npc_name', 87142, 'gullerback', 1, 87, 0, 0),
(18000292, 8208780, 2, 'npc_name_kill', 'npc_name', 87115, 'korasal klyseer', 1, 87, 0, 0),
(18000293, 8208780, 3, 'npc_name_kill', 'npc_name', 87148, 'phurzikon', 1, 87, 0, 0),
(18000294, 8208880, 0, 'npc_name_kill', 'npc_name', 88091, 'failed crypt raider', 1, 88, 0, 0),
(18000295, 8208880, 1, 'npc_name_kill', 'npc_name', 88085, 'frenzied strathbone', 1, 88, 0, 0),
(18000296, 8208880, 2, 'npc_name_kill', 'npc_name', 88094, 'hungered ravener', 1, 88, 0, 0),
(18000297, 8208880, 3, 'npc_name_kill', 'npc_name', 88089, 'reaver of xalgoz', 1, 88, 0, 0),
(18000298, 8208880, 4, 'npc_name_kill', 'npc_name', 88061, 'spectral librarian', 1, 88, 0, 0),
(18000299, 8208880, 5, 'npc_name_kill', 'npc_name', 88090, 'strathbone runelord', 1, 88, 0, 0),
(18000300, 8208880, 6, 'npc_name_kill', 'npc_name', 88055, 'tortured librarian', 1, 88, 0, 0),
(18000301, 8208880, 7, 'npc_name_kill', 'npc_name', 88073, 'xalgoz', 1, 88, 0, 0),
(18000302, 8208980, 0, 'npc_name_kill', 'npc_name', 89174, 'baron yosig', 1, 89, 0, 0),
(18000303, 8208980, 1, 'npc_name_kill', 'npc_name', 89159, 'blood of chottal', 1, 89, 0, 0),
(18000304, 8208980, 2, 'npc_name_kill', 'npc_name', 89167, 'brogg', 1, 89, 0, 0),
(18000305, 8208980, 3, 'npc_name_kill', 'npc_name', 89177, 'crypt caretaker', 1, 89, 0, 0),
(18000306, 8208980, 4, 'npc_name_kill', 'npc_name', 89144, 'emperor chottal', 1, 89, 0, 0),
(18000307, 8208980, 5, 'npc_name_kill', 'npc_name', 89173, 'frenzied pox scarab', 1, 89, 0, 0),
(18000308, 8208980, 6, 'npc_name_kill', 'npc_name', 89124, 'froggy', 1, 89, 0, 0),
(18000309, 8208980, 7, 'npc_name_kill', 'npc_name', 89180, 'froglok armorer', 1, 89, 0, 0),
(18000310, 8208980, 8, 'npc_name_kill', 'npc_name', 89006, 'froglok armsman', 1, 89, 0, 0),
(18000311, 8208980, 9, 'npc_name_kill', 'npc_name', 89131, 'froglok bartender', 1, 89, 0, 0),
(18000312, 8208980, 10, 'npc_name_kill', 'npc_name', 89164, 'froglok chef', 1, 89, 0, 0),
(18000313, 8208980, 11, 'npc_name_kill', 'npc_name', 89166, 'froglok commander', 1, 89, 0, 0),
(18000314, 8208980, 12, 'npc_name_kill', 'npc_name', 89057, 'froglok ostiary', 1, 89, 0, 0),
(18000315, 8208980, 13, 'npc_name_kill', 'npc_name', 89171, 'froglok pickler', 1, 89, 0, 0),
(18000316, 8208980, 14, 'npc_name_kill', 'npc_name', 89146, 'froglok repairer', 1, 89, 0, 0),
(18000317, 8208980, 15, 'npc_name_kill', 'npc_name', 89175, 'gruplinort', 1, 89, 0, 0),
(18000318, 8208980, 16, 'npc_name_kill', 'npc_name', 89129, 'harbinger freglor', 1, 89, 0, 0),
(18000319, 8208980, 17, 'npc_name_kill', 'npc_name', 89163, 'hierophant prime grekal', 1, 89, 0, 0),
(18000320, 8208980, 18, 'npc_name_kill', 'npc_name', 89153, 'myconid spore king', 1, 89, 0, 0),
(18000321, 8208980, 19, 'npc_name_kill', 'npc_name', 89134, 'a necrosis scarab', 1, 89, 0, 0),
(18000322, 8208980, 20, 'npc_name_kill', 'npc_name', 89112, 'sebilite guardian', 1, 89, 0, 0),
(18000323, 8209280, 0, 'npc_name_kill', 'npc_name', 92187, 'a goblin bodyguard', 1, 92, 0, 0),
(18000324, 8209280, 1, 'npc_name_kill', 'npc_name', 92204, 'a goblin fanatic', 1, 92, 0, 0),
(18000325, 8209580, 0, 'npc_name_kill', 'npc_name', 95151, 'bloodeye', 1, 95, 0, 0),
(18000326, 8209580, 1, 'npc_name_kill', 'npc_name', 95176, 'champion arlek', 1, 95, 0, 0),
(18000327, 8209580, 2, 'npc_name_kill', 'npc_name', 95129, 'champion thenrin', 1, 95, 0, 0),
(18000328, 8209580, 3, 'npc_name_kill', 'npc_name', 95130, 'commander sils', 1, 95, 0, 0),
(18000329, 8209580, 4, 'npc_name_kill', 'npc_name', 95161, 'crusader zoglic', 1, 95, 0, 0),
(18000330, 8209580, 5, 'npc_name_kill', 'npc_name', 95175, 'doom', 1, 95, 0, 0),
(18000331, 8209580, 6, 'npc_name_kill', 'npc_name', 95157, 'dragontail', 1, 95, 0, 0),
(18000332, 8209580, 7, 'npc_name_kill', 'npc_name', 95166, 'dreadlord dekir', 1, 95, 0, 0),
(18000333, 8209580, 8, 'npc_name_kill', 'npc_name', 95153, 'dreadlord fanrik', 1, 95, 0, 0),
(18000334, 8209580, 9, 'npc_name_kill', 'npc_name', 95164, 'ebon lotus', 1, 95, 0, 0),
(18000335, 8209580, 10, 'npc_name_kill', 'npc_name', 95165, 'ffroaak', 1, 95, 0, 0),
(18000336, 8209580, 11, 'npc_name_kill', 'npc_name', 95172, 'flayhte', 1, 95, 0, 0),
(18000337, 8209580, 12, 'npc_name_kill', 'npc_name', 95154, 'hangman', 1, 95, 0, 0),
(18000338, 8209580, 13, 'npc_name_kill', 'npc_name', 95162, 'harbinger dronik', 1, 95, 0, 0),
(18000339, 8209580, 14, 'npc_name_kill', 'npc_name', 95150, 'harbinger josk', 1, 95, 0, 0),
(18000340, 8209580, 15, 'npc_name_kill', 'npc_name', 95178, 'hierophant ixyl', 1, 95, 0, 0),
(18000341, 8209580, 16, 'npc_name_kill', 'npc_name', 95167, 'keeper lasnik', 1, 95, 0, 0),
(18000342, 8209580, 17, 'npc_name_kill', 'npc_name', 95158, 'keeper sepsis', 1, 95, 0, 0),
(18000343, 8209580, 18, 'npc_name_kill', 'npc_name', 95160, 'klok denris', 1, 95, 0, 0),
(18000344, 8209580, 19, 'npc_name_kill', 'npc_name', 95155, 'knight dragol', 1, 95, 0, 0),
(18000345, 8209580, 20, 'npc_name_kill', 'npc_name', 95179, 'master fasliw', 1, 95, 0, 0),
(18000346, 8209580, 21, 'npc_name_kill', 'npc_name', 95107, 'oracle froskil', 1, 95, 0, 0),
(18000347, 8209580, 22, 'npc_name_kill', 'npc_name', 95170, 'partisan yinlen', 1, 95, 0, 0),
(18000348, 8209580, 23, 'npc_name_kill', 'npc_name', 95182, 'sigra', 1, 95, 0, 0),
(18000349, 8209580, 24, 'npc_name_kill', 'npc_name', 95169, 'silvermane', 1, 95, 0, 0),
(18000350, 8209580, 25, 'npc_name_kill', 'npc_name', 95159, 'squire glik', 1, 95, 0, 0),
(18000351, 8209580, 26, 'npc_name_kill', 'npc_name', 95108, 'stonebeak', 1, 95, 0, 0),
(18000352, 8209580, 27, 'npc_name_kill', 'npc_name', 95177, 'throkkok', 1, 95, 0, 0),
(18000353, 8209580, 28, 'npc_name_kill', 'npc_name', 95171, 'thruke', 1, 95, 0, 0),
(18000354, 8209580, 29, 'npc_name_kill', 'npc_name', 95149, 'titail sinok', 1, 95, 0, 0),
(18000355, 8209580, 30, 'npc_name_kill', 'npc_name', 95156, 'vessel fryn', 1, 95, 0, 0),
(18000356, 8209680, 0, 'npc_name_kill', 'npc_name', 96077, 'halara', 1, 96, 0, 0),
(18000357, 8209680, 1, 'npc_name_kill', 'npc_name', 96087, 'ugrak da raider', 1, 96, 0, 0),
(18000358, 8209780, 0, 'npc_name_kill', 'npc_name', 97075, 'bargynn', 1, 97, 0, 0),
(18000359, 8209780, 1, 'npc_name_kill', 'npc_name', 97077, 'undead crusader', 1, 97, 0, 0),
(18000360, 8210280, 0, 'npc_name_kill', 'npc_name', 102013, 'caller of sathir', 1, 102, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000361, 8210280, 1, 'npc_name_kill', 'npc_name', 102084, 'construct of sathir', 1, 102, 0, 0),
(18000362, 8210280, 2, 'npc_name_kill', 'npc_name', 102124, 'hangnail', 1, 102, 0, 0),
(18000363, 8210280, 3, 'npc_name_kill', 'npc_name', 102119, 'knight of sathir', 1, 102, 0, 0),
(18000364, 8210280, 4, 'npc_name_kill', 'npc_name', 102125, 'sentry of sathir', 1, 102, 0, 0),
(18000365, 8210280, 5, 'npc_name_kill', 'npc_name', 102120, 'skeletal berserker', 1, 102, 0, 0),
(18000366, 8210280, 6, 'npc_name_kill', 'npc_name', 102116, 'skeletal captain', 1, 102, 0, 0),
(18000367, 8210280, 7, 'npc_name_kill', 'npc_name', 102121, 'skeletal caretaker', 1, 102, 0, 0),
(18000368, 8210280, 8, 'npc_name_kill', 'npc_name', 102047, 'skeletal scryer', 1, 102, 0, 0),
(18000369, 8210280, 9, 'npc_name_kill', 'npc_name', 102008, 'skeletal warlord', 1, 102, 0, 0),
(18000370, 8210280, 10, 'npc_name_kill', 'npc_name', 102090, 'spectral turnkey', 1, 102, 0, 0),
(18000371, 8210280, 11, 'npc_name_kill', 'npc_name', 102105, 'undead jailer', 1, 102, 0, 0),
(18000372, 8210380, 0, 'npc_name_kill', 'npc_name', 103184, 'battle master ska`tu', 1, 103, 0, 0),
(18000373, 8210380, 1, 'npc_name_kill', 'npc_name', 103191, 'deathfang', 1, 103, 0, 0),
(18000374, 8210380, 2, 'npc_name_kill', 'npc_name', 103198, 'drill master dih`roul', 1, 103, 0, 0),
(18000375, 8210380, 3, 'npc_name_kill', 'npc_name', 103170, 'foreman ku`lul', 1, 103, 0, 0),
(18000376, 8210380, 4, 'npc_name_kill', 'npc_name', 103211, 'foreman mirt`akk', 1, 103, 0, 0),
(18000377, 8210380, 5, 'npc_name_kill', 'npc_name', 103139, 'grand herbalist mak`ha', 1, 103, 0, 0),
(18000378, 8210380, 6, 'npc_name_kill', 'npc_name', 103200, 'grand lorekeeper kino shai`din', 1, 103, 0, 0),
(18000379, 8210380, 7, 'npc_name_kill', 'npc_name', 103169, 'grave master zo`lun', 1, 103, 0, 0),
(18000380, 8210380, 8, 'npc_name_kill', 'npc_name', 103183, 'an iksar trustee', 1, 103, 0, 0),
(18000381, 8210380, 9, 'npc_name_kill', 'npc_name', 103092, 'an imperial gravemaster', 1, 103, 0, 0),
(18000382, 8210380, 10, 'npc_name_kill', 'npc_name', 103180, 'kennel master al`ele', 1, 103, 0, 0),
(18000383, 8210380, 11, 'npc_name_kill', 'npc_name', 103215, 'loremaster piza`tak', 1, 103, 0, 0),
(18000384, 8210380, 12, 'npc_name_kill', 'npc_name', 103051, 'observer aq`touz', 1, 103, 0, 0),
(18000385, 8210380, 13, 'npc_name_kill', 'npc_name', 103171, 'overseer dal`guur', 1, 103, 0, 0),
(18000386, 8210380, 14, 'npc_name_kill', 'npc_name', 103163, 'sarnak collective auditor', 1, 103, 0, 0),
(18000387, 8210380, 15, 'npc_name_kill', 'npc_name', 103177, 'underboss myli`ki', 1, 103, 0, 0),
(18000388, 8210380, 16, 'npc_name_kill', 'npc_name', 103197, 'watch captain hir`roul', 1, 103, 0, 0),
(18000389, 8210380, 17, 'npc_name_kill', 'npc_name', 103008, 'a wizened herb collector', 1, 103, 0, 0),
(18000390, 8210480, 0, 'npc_name_kill', 'npc_name', 104080, 'a coerced crusader', 1, 104, 0, 0),
(18000391, 8210480, 1, 'npc_name_kill', 'npc_name', 104073, 'a coerced penkeeper', 1, 104, 0, 0),
(18000392, 8210480, 2, 'npc_name_kill', 'npc_name', 104076, 'a coerced revenant', 1, 104, 0, 0),
(18000393, 8210480, 3, 'npc_name_kill', 'npc_name', 104077, 'the kly', 1, 104, 0, 0),
(18000394, 8210480, 4, 'npc_name_kill', 'npc_name', 104070, 'lumpy goo', 1, 104, 0, 0),
(18000395, 8210480, 5, 'npc_name_kill', 'npc_name', 104074, 'a spectral crusader', 1, 104, 0, 0),
(18000396, 8210580, 0, 'npc_name_kill', 'npc_name', 105152, 'the crypt devourer', 1, 105, 0, 0),
(18000397, 8210580, 1, 'npc_name_kill', 'npc_name', 105029, 'the crypt excavator', 1, 105, 0, 0),
(18000398, 8210580, 2, 'npc_name_kill', 'npc_name', 105116, 'the crypt feaster', 1, 105, 0, 0),
(18000399, 8210580, 3, 'npc_name_kill', 'npc_name', 105074, 'the crypt spectre', 1, 105, 0, 0),
(18000400, 8210580, 4, 'npc_name_kill', 'npc_name', 105075, 'embalming fluid', 1, 105, 0, 0),
(18000401, 8210580, 5, 'npc_name_kill', 'npc_name', 105081, 'the golem master', 1, 105, 0, 0),
(18000402, 8210580, 6, 'npc_name_kill', 'npc_name', 105024, 'howling spectre', 1, 105, 0, 0),
(18000403, 8210580, 7, 'npc_name_kill', 'npc_name', 105157, 'mortiferous protector', 1, 105, 0, 0),
(18000404, 8210580, 8, 'npc_name_kill', 'npc_name', 105013, 'reanimated plaguebone', 1, 105, 0, 0),
(18000405, 8210580, 9, 'npc_name_kill', 'npc_name', 105091, 'sentient bile', 1, 105, 0, 0),
(18000406, 8210580, 10, 'npc_name_kill', 'npc_name', 105076, 'the skeleton sepulcher', 1, 105, 0, 0),
(18000407, 8210580, 11, 'npc_name_kill', 'npc_name', 105093, 'the spectre sepulcher', 1, 105, 0, 0),
(18000408, 8210580, 12, 'npc_name_kill', 'npc_name', 105146, 'the spectre spiritualist', 1, 105, 0, 0),
(18000409, 8210580, 13, 'npc_name_kill', 'npc_name', 105160, 'the undertaker lord', 1, 105, 0, 0),
(18000410, 8210880, 0, 'npc_name_kill', 'npc_name', 108502, 'an ancient racnar', 1, 108, 0, 0),
(18000411, 8210880, 1, 'npc_name_kill', 'npc_name', 108508, 'a cruel racnar', 1, 108, 0, 0),
(18000412, 8210980, 0, 'npc_name_kill', 'npc_name', 109092, 'a bloodgill soothsayer', 1, 109, 0, 0),
(18000413, 8210980, 1, 'npc_name_kill', 'npc_name', 109091, 'a bloodgill warlord', 1, 109, 0, 0),
(18000414, 8210980, 2, 'npc_name_kill', 'npc_name', 109103, 'brother eruk', 1, 109, 0, 0),
(18000415, 8210980, 3, 'npc_name_kill', 'npc_name', 109104, 'champion kamak', 1, 109, 0, 0),
(18000416, 8210980, 4, 'npc_name_kill', 'npc_name', 109094, 'a decaying slavemaster', 1, 109, 0, 0),
(18000417, 8210980, 5, 'npc_name_kill', 'npc_name', 109106, 'feral lord gulok', 1, 109, 0, 0),
(18000418, 8210980, 6, 'npc_name_kill', 'npc_name', 109109, 'hierophant ginai', 1, 109, 0, 0),
(18000419, 8210980, 7, 'npc_name_kill', 'npc_name', 109084, 'hierophant vradik', 1, 109, 0, 0),
(18000420, 8210980, 8, 'npc_name_kill', 'npc_name', 109099, 'an iksar behemoth', 1, 109, 0, 0),
(18000421, 8210980, 9, 'npc_name_kill', 'npc_name', 109090, 'a kylong crusader', 1, 109, 0, 0),
(18000422, 8210980, 10, 'npc_name_kill', 'npc_name', 109116, 'a kylong lich', 1, 109, 0, 0),
(18000423, 8210980, 11, 'npc_name_kill', 'npc_name', 109110, 'lord sasil', 1, 109, 0, 0),
(18000424, 8210980, 12, 'npc_name_kill', 'npc_name', 109088, 'luminary salox', 1, 109, 0, 0),
(18000425, 8210980, 13, 'npc_name_kill', 'npc_name', 109098, 'a plagued slave', 1, 109, 0, 0),
(18000426, 8210980, 14, 'npc_name_kill', 'npc_name', 109096, 'a rotting shopkeeper', 1, 109, 0, 0),
(18000427, 8210980, 15, 'npc_name_kill', 'npc_name', 109097, 'a sad slave', 1, 109, 0, 0),
(18000428, 8210980, 16, 'npc_name_kill', 'npc_name', 109111, 'trooper muruk', 1, 109, 0, 0),
(18000429, 8210980, 17, 'npc_name_kill', 'npc_name', 109100, 'an undead chef', 1, 109, 0, 0),
(18000430, 8210980, 18, 'npc_name_kill', 'npc_name', 109101, 'an undead thief', 1, 109, 0, 0),
(18000431, 8210980, 19, 'npc_name_kill', 'npc_name', 109102, 'an undying blacksmith', 1, 109, 0, 0),
(18000432, 8210980, 20, 'npc_name_kill', 'npc_name', 109112, 'warlock dirloz', 1, 109, 0, 0),
(18000433, 8210980, 21, 'npc_name_kill', 'npc_name', 109113, 'warlock gurag', 1, 109, 0, 0),
(18000434, 8227780, 0, 'npc_name_kill', 'npc_name', 277118, 'brodisek ashkansek', 1, 277, 0, 0),
(18000435, 8227780, 1, 'npc_name_kill', 'npc_name', 277116, 'krizznot bonewalker', 1, 277, 0, 0),
(18000436, 8227780, 2, 'npc_name_kill', 'npc_name', 277096, 'a rabid chokidai biter', 1, 277, 0, 0),
(18000437, 8227780, 3, 'npc_name_kill', 'npc_name', 277090, 'a rabid chokidai bleeder', 1, 277, 0, 0),
(18000438, 8227780, 4, 'npc_name_kill', 'npc_name', 277122, 'a rabid chokidai ripper', 1, 277, 0, 0),
(18000439, 8227780, 5, 'npc_name_kill', 'npc_name', 277119, 'sarnak armorer', 1, 277, 0, 0),
(18000440, 8227780, 6, 'npc_name_kill', 'npc_name', 277120, 'sarnak champion', 1, 277, 0, 0),
(18000441, 8227780, 7, 'npc_name_kill', 'npc_name', 277121, 'sarnak weaponsmith', 1, 277, 0, 0),
(18000442, 8227780, 8, 'npc_name_kill', 'npc_name', 277117, 'valinion viz`daron', 1, 277, 0, 0),
(18000443, 8250880, 0, 'npc_name_kill', 'npc_name', 108041, 'elder ekron', 1, 108, 0, 0),
(18000444, 8250880, 1, 'npc_name_kill', 'npc_name', 108044, 'kluzen the protector', 1, 108, 0, 0),
(18000445, 8250880, 2, 'npc_name_kill', 'npc_name', 108045, 'magma basilisk', 1, 108, 0, 0),
(18000446, 8250880, 3, 'npc_name_kill', 'npc_name', 108046, 'milyex vioren', 1, 108, 0, 0),
(18000447, 8250880, 4, 'npc_name_kill', 'npc_name', 108049, 'qunard ashenclaw', 1, 108, 0, 0),
(18000448, 8250880, 5, 'npc_name_kill', 'npc_name', 108051, 'travenro the skygazer', 1, 108, 0, 0),
(18000449, 8311080, 0, 'npc_name_kill', 'npc_name', 110077, 'dire wolf stalker', 1, 110, 0, 0),
(18000450, 8311080, 1, 'npc_name_kill', 'npc_name', 110006, 'midnight', 1, 110, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000451, 8311080, 2, 'npc_name_kill', 'npc_name', 110111, 'pulsating icestorm', 1, 110, 0, 0),
(18000452, 8311280, 0, 'npc_name_kill', 'npc_name', 112103, 'a deadly blizzard hunter', 1, 112, 0, 0),
(18000453, 8311280, 1, 'npc_name_kill', 'npc_name', 112104, 'a fierce blizzard hunter', 1, 112, 0, 0),
(18000454, 8311680, 0, 'npc_name_kill', 'npc_name', 116165, 'chief ry`gorr', 1, 116, 0, 0),
(18000455, 8311680, 1, 'npc_name_kill', 'npc_name', 116012, 'firbrand the black', 1, 116, 0, 0),
(18000456, 8311680, 2, 'npc_name_kill', 'npc_name', 116197, 'a ry`gorr minstrel', 1, 116, 0, 0),
(18000457, 8311680, 3, 'npc_name_kill', 'npc_name', 116198, 'warden bruke', 1, 116, 0, 0),
(18000458, 8311780, 0, 'npc_name_kill', 'npc_name', 117014, 'yvolcarn', 1, 117, 0, 0),
(18000459, 8311880, 0, 'npc_name_kill', 'npc_name', 118109, 'yaka razorhoof', 1, 118, 0, 0),
(18000460, 8311980, 0, 'npc_name_kill', 'npc_name', 119047, 'an elder holgresh', 1, 119, 0, 0),
(18000461, 8311980, 1, 'npc_name_kill', 'npc_name', 119129, 'a holgresh raider', 1, 119, 0, 0),
(18000462, 8312080, 0, 'npc_name_kill', 'npc_name', 120048, 'tantor', 1, 120, 0, 0),
(18000463, 8312180, 0, 'npc_name_kill', 'npc_name', 121085, 'a focus gem', 1, 121, 0, 0),
(18000464, 8312380, 0, 'npc_name_kill', 'npc_name', 123090, 'dominator yisaki', 1, 123, 0, 0),
(18000465, 8312380, 1, 'npc_name_kill', 'npc_name', 123112, 'a great green slime', 1, 123, 0, 0),
(18000466, 8312380, 2, 'npc_name_kill', 'npc_name', 123125, 'pierre', 1, 123, 0, 0),
(18000467, 8312380, 3, 'npc_name_kill', 'npc_name', 123016, 'slani veekilaleeki', 1, 123, 0, 0),
(18000468, 8312380, 4, 'npc_name_kill', 'npc_name', 123149, 'vaniki', 1, 123, 0, 0),
(18000469, 8312580, 0, 'npc_name_kill', 'npc_name', 125088, 'an enthralled ulthork', 1, 125, 0, 0),
(18000470, 8312580, 1, 'npc_name_kill', 'npc_name', 125098, 'fellspine', 1, 125, 0, 0),
(18000471, 8312580, 2, 'npc_name_kill', 'npc_name', 125077, 'shimmering sea spirit', 1, 125, 0, 0),
(18000472, 8312580, 3, 'npc_name_kill', 'npc_name', 125083, 'a siren seductress', 1, 125, 0, 0),
(18000473, 8312980, 0, 'npc_name_kill', 'npc_name', 129093, 'glucose', 1, 129, 0, 0),
(18000474, 8312980, 1, 'npc_name_kill', 'npc_name', 129092, 'grizznot', 1, 129, 0, 0),
(18000475, 8352680, 0, 'npc_name_kill', 'npc_name', 126240, 'an asinine ape', 1, 126, 0, 0),
(18000476, 8352680, 1, 'npc_name_kill', 'npc_name', 126185, 'an astray hand', 1, 126, 0, 0),
(18000477, 8352680, 2, 'npc_name_kill', 'npc_name', 126372, 'a barmy burglar', 1, 126, 0, 0),
(18000478, 8352680, 3, 'npc_name_kill', 'npc_name', 126369, 'a bogus treasure chest', 1, 126, 0, 0),
(18000479, 8352680, 4, 'npc_name_kill', 'npc_name', 126306, 'a bouncing bunny', 1, 126, 0, 0),
(18000480, 8352680, 5, 'npc_name_kill', 'npc_name', 126168, 'bzzibuzzana', 1, 126, 0, 0),
(18000481, 8352680, 6, 'npc_name_kill', 'npc_name', 126370, 'a craven rascal', 1, 126, 0, 0),
(18000482, 8352680, 7, 'npc_name_kill', 'npc_name', 126368, 'a forsaken hand', 1, 126, 0, 0),
(18000483, 8352680, 8, 'npc_name_kill', 'npc_name', 126139, 'a fraudulent chest', 1, 126, 0, 0),
(18000484, 8352680, 9, 'npc_name_kill', 'npc_name', 126275, 'a frolicking performer', 1, 126, 0, 0),
(18000485, 8352680, 10, 'npc_name_kill', 'npc_name', 126302, 'a funky skunk', 1, 126, 0, 0),
(18000486, 8352680, 11, 'npc_name_kill', 'npc_name', 126137, 'a gifted sphinx', 1, 126, 0, 0),
(18000487, 8352680, 12, 'npc_name_kill', 'npc_name', 126176, 'a gorilla prodigy', 1, 126, 0, 0),
(18000488, 8352680, 13, 'npc_name_kill', 'npc_name', 126371, 'a gorilla professor', 1, 126, 0, 0),
(18000489, 8352680, 14, 'npc_name_kill', 'npc_name', 126081, 'the grand gardener', 1, 126, 0, 0),
(18000490, 8352680, 15, 'npc_name_kill', 'npc_name', 126303, 'a maddened mushroom man', 1, 126, 0, 0),
(18000491, 8352680, 16, 'npc_name_kill', 'npc_name', 126307, 'the master of mayhem', 1, 126, 0, 0),
(18000492, 8352680, 17, 'npc_name_kill', 'npc_name', 126367, 'a method actor', 1, 126, 0, 0),
(18000493, 8352680, 18, 'npc_name_kill', 'npc_name', 126304, 'an obsessed patron', 1, 126, 0, 0),
(18000494, 8352680, 19, 'npc_name_kill', 'npc_name', 126068, 'a paradoxical sphinx', 1, 126, 0, 0),
(18000495, 8352680, 20, 'npc_name_kill', 'npc_name', 126310, 'a savvy sphinx', 1, 126, 0, 0),
(18000496, 8352680, 21, 'npc_name_kill', 'npc_name', 126305, 'a theatre fanatic', 1, 126, 0, 0),
(18000497, 8352680, 22, 'npc_name_kill', 'npc_name', 126098, 'a troublesome charlatan', 1, 126, 0, 0),
(18000498, 8352680, 23, 'npc_name_kill', 'npc_name', 126108, 'a wicked performer', 1, 126, 0, 0),
(18000499, 8352680, 24, 'npc_name_kill', 'npc_name', 126228, 'a witty sphinx', 1, 126, 0, 0),
(18000500, 8380280, 0, 'npc_name_kill', 'npc_name', 128028, 'an ancient advocator', 1, 128, 0, 0),
(18000501, 8380380, 0, 'npc_name_kill', 'npc_name', 114018, 'kintaru of the shrine', 1, 114, 0, 0),
(18000502, 8380380, 1, 'npc_name_kill', 'npc_name', 114041, 'marech of the shrine', 1, 114, 0, 0),
(18000503, 8415380, 0, 'npc_name_kill', 'npc_name', 153138, 'chief groplin', 1, 153, 0, 0),
(18000504, 8415380, 1, 'npc_name_kill', 'npc_name', 153033, 'crinthia signseer', 1, 153, 0, 0),
(18000505, 8415380, 2, 'npc_name_kill', 'npc_name', 153005, 'fireclaw', 1, 153, 0, 0),
(18000506, 8415380, 3, 'npc_name_kill', 'npc_name', 153130, 'murph cobblestone', 1, 153, 0, 0),
(18000507, 8415380, 4, 'npc_name_kill', 'npc_name', 153136, 'the needlite queen', 1, 153, 0, 0),
(18000508, 8415380, 5, 'npc_name_kill', 'npc_name', 153044, 'torin truestring', 1, 153, 0, 0),
(18000509, 8415380, 6, 'npc_name_kill', 'npc_name', 153145, 'trillcor', 1, 153, 0, 0),
(18000510, 8415480, 0, 'npc_name_kill', 'npc_name', 154073, 'a grimling arcanist', 1, 154, 0, 0),
(18000511, 8415480, 1, 'npc_name_kill', 'npc_name', 154168, 'a grimling arch sage', 1, 154, 0, 0),
(18000512, 8415480, 2, 'npc_name_kill', 'npc_name', 154164, 'a grimling bloodpriest', 1, 154, 0, 0),
(18000513, 8415480, 3, 'npc_name_kill', 'npc_name', 154077, 'a grimling high priest', 1, 154, 0, 0),
(18000514, 8415480, 4, 'npc_name_kill', 'npc_name', 154165, 'a grimling primalist', 1, 154, 0, 0),
(18000515, 8415480, 5, 'npc_name_kill', 'npc_name', 154160, 'a grimling ritualist', 1, 154, 0, 0),
(18000516, 8415480, 6, 'npc_name_kill', 'npc_name', 154169, 'a grimling spiritist', 1, 154, 0, 0),
(18000517, 8415480, 7, 'npc_name_kill', 'npc_name', 154167, 'a grimling warlord', 1, 154, 0, 0),
(18000518, 8415680, 0, 'npc_name_kill', 'npc_name', 156114, 'ch`ktok', 1, 156, 0, 0),
(18000519, 8415680, 1, 'npc_name_kill', 'npc_name', 156112, 'a ravenous owlbear', 1, 156, 0, 0),
(18000520, 8415780, 0, 'npc_name_kill', 'npc_name', 157118, 'shuddering fungus', 1, 157, 0, 0),
(18000521, 8416180, 0, 'npc_name_kill', 'npc_name', 161040, 'a fungoid worker', 1, 161, 0, 0),
(18000522, 8416180, 1, 'npc_name_kill', 'npc_name', 161057, 'a mature fungoid', 1, 161, 0, 0),
(18000523, 8416180, 2, 'npc_name_kill', 'npc_name', 161066, 'a netherbian swarmcaller', 1, 161, 0, 0),
(18000524, 8416180, 3, 'npc_name_kill', 'npc_name', 161030, 'the swarm leader', 1, 161, 0, 0),
(18000525, 8416180, 4, 'npc_name_kill', 'npc_name', 161021, 'a trog hunter', 1, 161, 0, 0),
(18000526, 8416180, 5, 'npc_name_kill', 'npc_name', 161071, 'the trog king', 1, 161, 0, 0),
(18000527, 8416280, 0, 'npc_name_kill', 'npc_name', 162201, 'acolyte wivlx', 1, 162, 0, 0),
(18000528, 8416280, 1, 'npc_name_kill', 'npc_name', 162067, 'advisor zekuzh', 1, 162, 0, 0),
(18000529, 8416280, 2, 'npc_name_kill', 'npc_name', 162191, 'arbiter korazhk', 1, 162, 0, 0),
(18000530, 8416280, 3, 'npc_name_kill', 'npc_name', 162150, 'commander zazuzh', 1, 162, 0, 0),
(18000531, 8416280, 4, 'npc_name_kill', 'npc_name', 162217, 'commander zherozsh', 1, 162, 0, 0),
(18000532, 8416280, 5, 'npc_name_kill', 'npc_name', 162187, 'defiler juzlix', 1, 162, 0, 0),
(18000533, 8416280, 6, 'npc_name_kill', 'npc_name', 162180, 'disciple yelwinz', 1, 162, 0, 0),
(18000534, 8416280, 7, 'npc_name_kill', 'npc_name', 162257, 'disciple zhorluhx', 1, 162, 0, 0),
(18000535, 8416280, 8, 'npc_name_kill', 'npc_name', 162066, 'general kizuhx', 1, 162, 0, 0),
(18000536, 8416280, 9, 'npc_name_kill', 'npc_name', 162219, 'guard sklinus', 1, 162, 0, 0),
(18000537, 8416280, 10, 'npc_name_kill', 'npc_name', 162167, 'shissar assassin', 1, 162, 0, 0),
(18000538, 8416280, 11, 'npc_name_kill', 'npc_name', 162183, 'the shissar magister', 1, 162, 0, 0),
(18000539, 8416280, 12, 'npc_name_kill', 'npc_name', 162186, 'shissar mystic', 1, 162, 0, 0),
(18000540, 8416280, 13, 'npc_name_kill', 'npc_name', 162154, 'spiritward trilzic', 1, 162, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000541, 8416280, 14, 'npc_name_kill', 'npc_name', 162087, 'zhroushe mezhkazh', 1, 162, 0, 0),
(18000542, 8416380, 0, 'npc_name_kill', 'npc_name', 163250, 'bronus', 1, 163, 0, 0),
(18000543, 8416480, 0, 'npc_name_kill', 'npc_name', 164088, 'chasm overseer', 1, 164, 0, 0),
(18000544, 8416480, 1, 'npc_name_kill', 'npc_name', 164041, 'a deepspore mushroom', 1, 164, 0, 0),
(18000545, 8416480, 2, 'npc_name_kill', 'npc_name', 164035, 'deranged underbulk', 1, 164, 0, 0),
(18000546, 8416480, 3, 'npc_name_kill', 'npc_name', 164069, 'a hideous thought leech', 1, 164, 0, 0),
(18000547, 8416480, 4, 'npc_name_kill', 'npc_name', 164103, 'koxiux the imperceptible', 1, 164, 0, 0),
(18000548, 8416480, 5, 'npc_name_kill', 'npc_name', 164096, 'lhurgoz the decayed', 1, 164, 0, 0),
(18000549, 8416480, 6, 'npc_name_kill', 'npc_name', 164106, 'oldencamp the lost', 1, 164, 0, 0),
(18000550, 8416480, 7, 'npc_name_kill', 'npc_name', 164114, 'slavekeeper xiox', 1, 164, 0, 0),
(18000551, 8416480, 8, 'npc_name_kill', 'npc_name', 164102, 'thought horror corrupter', 1, 164, 0, 0),
(18000552, 8416480, 9, 'npc_name_kill', 'npc_name', 164110, 'xeximoz', 1, 164, 0, 0),
(18000553, 8416580, 0, 'npc_name_kill', 'npc_name', 165085, 'seetheker', 1, 165, 0, 0),
(18000554, 8416680, 0, 'npc_name_kill', 'npc_name', 166260, 'curfang', 1, 166, 0, 0),
(18000555, 8416680, 1, 'npc_name_kill', 'npc_name', 166246, 'dreadmaw wolfkin', 1, 166, 0, 0),
(18000556, 8416680, 2, 'npc_name_kill', 'npc_name', 166268, 'fireclaw wolfkin', 1, 166, 0, 0),
(18000557, 8416680, 3, 'npc_name_kill', 'npc_name', 166278, 'gamtoine cursmakk', 1, 166, 0, 0),
(18000558, 8416680, 4, 'npc_name_kill', 'npc_name', 166153, 'ghowlik', 1, 166, 0, 0),
(18000559, 8416680, 5, 'npc_name_kill', 'npc_name', 166243, 'gleeknot gnitrat', 1, 166, 0, 0),
(18000560, 8416680, 6, 'npc_name_kill', 'npc_name', 166261, 'gniktar grinwit', 1, 166, 0, 0),
(18000561, 8416680, 7, 'npc_name_kill', 'npc_name', 166276, 'gorehorn', 1, 166, 0, 0),
(18000562, 8416680, 8, 'npc_name_kill', 'npc_name', 166252, 'prince skriatat', 1, 166, 0, 0),
(18000563, 8416680, 9, 'npc_name_kill', 'npc_name', 166248, 'warpaw dankpelt', 1, 166, 0, 0),
(18000564, 8416680, 10, 'npc_name_kill', 'npc_name', 166244, 'wiknak grimglom', 1, 166, 0, 0),
(18000565, 8416880, 0, 'npc_name_kill', 'npc_name', 168007, 'a recuso hunter', 1, 168, 0, 0),
(18000566, 8417080, 0, 'npc_name_kill', 'npc_name', 170267, 'aero', 1, 170, 0, 0),
(18000567, 8417080, 1, 'npc_name_kill', 'npc_name', 170223, 'a berserk air elemental', 1, 170, 0, 0),
(18000568, 8417080, 2, 'npc_name_kill', 'npc_name', 170231, 'a berserk fire elemental', 1, 170, 0, 0),
(18000569, 8417080, 3, 'npc_name_kill', 'npc_name', 170225, 'a berserk water elemental', 1, 170, 0, 0),
(18000570, 8417080, 4, 'npc_name_kill', 'npc_name', 170238, 'a gigantic air elemental', 1, 170, 0, 0),
(18000571, 8417080, 5, 'npc_name_kill', 'npc_name', 170259, 'a gigantic earth elemental', 1, 170, 0, 0),
(18000572, 8417080, 6, 'npc_name_kill', 'npc_name', 170012, 'a gigantic fire elemental', 1, 170, 0, 0),
(18000573, 8417080, 7, 'npc_name_kill', 'npc_name', 170244, 'a gigantic water elemental', 1, 170, 0, 0),
(18000574, 8417080, 8, 'npc_name_kill', 'npc_name', 170202, 'a greater fire elemental', 1, 170, 0, 0),
(18000575, 8417080, 9, 'npc_name_kill', 'npc_name', 170011, 'a greater water elemental', 1, 170, 0, 0),
(18000576, 8417080, 10, 'npc_name_kill', 'npc_name', 170268, 'hydro', 1, 170, 0, 0),
(18000577, 8417080, 11, 'npc_name_kill', 'npc_name', 170269, 'inferno', 1, 170, 0, 0),
(18000578, 8417080, 12, 'npc_name_kill', 'npc_name', 170161, 'praem softbreeze', 1, 170, 0, 0),
(18000579, 8417080, 13, 'npc_name_kill', 'npc_name', 170235, 'ranvin darkwaters', 1, 170, 0, 0),
(18000580, 8417080, 14, 'npc_name_kill', 'npc_name', 170204, 'terra', 1, 170, 0, 0),
(18000581, 8417080, 15, 'npc_name_kill', 'npc_name', 170053, 'a tro jeg official', 1, 170, 0, 0),
(18000582, 8417180, 0, 'npc_name_kill', 'npc_name', 171056, 'an ancient shissar servitor', 1, 171, 0, 0),
(18000583, 8417180, 1, 'npc_name_kill', 'npc_name', 171074, 'crusader kezzal', 1, 171, 0, 0),
(18000584, 8417180, 2, 'npc_name_kill', 'npc_name', 171065, 'heirophant grazan', 1, 171, 0, 0),
(18000585, 8417180, 3, 'npc_name_kill', 'npc_name', 171057, 'revenant sthzzzizt', 1, 171, 0, 0),
(18000586, 8417180, 4, 'npc_name_kill', 'npc_name', 171073, 'revenant zsshta', 1, 171, 0, 0),
(18000587, 8417180, 5, 'npc_name_kill', 'npc_name', 171068, 'a shattered golem', 1, 171, 0, 0),
(18000588, 8417280, 0, 'npc_name_kill', 'npc_name', 172151, 'emissary oomgado', 1, 172, 0, 0),
(18000589, 8417280, 1, 'npc_name_kill', 'npc_name', 172082, 'a grimling foreman', 1, 172, 0, 0),
(18000590, 8417280, 2, 'npc_name_kill', 'npc_name', 172083, 'a grol baku warlord', 1, 172, 0, 0),
(18000591, 8417280, 3, 'npc_name_kill', 'npc_name', 1985, 'skyshadow', 1, 172, 0, 0),
(18000592, 8417380, 0, 'npc_name_kill', 'npc_name', 173137, 'a goranga battlemaster', 1, 173, 0, 0),
(18000593, 8417380, 1, 'npc_name_kill', 'npc_name', 173126, 'a goranga chieftan', 1, 173, 0, 0),
(18000594, 8417380, 2, 'npc_name_kill', 'npc_name', 173129, 'a goranga forager', 1, 173, 0, 0),
(18000595, 8417380, 3, 'npc_name_kill', 'npc_name', 173127, 'a goranga prophet', 1, 173, 0, 0),
(18000596, 8417380, 4, 'npc_name_kill', 'npc_name', 173131, 'a goranga savant', 1, 173, 0, 0),
(18000597, 8417380, 5, 'npc_name_kill', 'npc_name', 173128, 'a goranga seer', 1, 173, 0, 0),
(18000598, 8417380, 6, 'npc_name_kill', 'npc_name', 173140, 'a shadow overlord', 1, 173, 0, 0),
(18000599, 8417380, 7, 'npc_name_kill', 'npc_name', 173114, 'a thought stealer', 1, 173, 0, 0),
(18000600, 8417380, 8, 'npc_name_kill', 'npc_name', 173143, 'xi thall', 1, 173, 0, 0),
(18000601, 8417480, 0, 'npc_name_kill', 'npc_name', 174316, 'an age old rockhopper', 1, 174, 0, 0),
(18000602, 8417480, 1, 'npc_name_kill', 'npc_name', 174249, 'fungus covered shroom', 1, 174, 0, 0),
(18000603, 8417480, 2, 'npc_name_kill', 'npc_name', 174256, 'sambata tribal advisor', 1, 174, 0, 0),
(18000604, 8417480, 3, 'npc_name_kill', 'npc_name', 174277, 'sambata tribal leader garn', 1, 174, 0, 0),
(18000605, 8417480, 4, 'npc_name_kill', 'npc_name', 174026, 'tribal advisor', 1, 174, 0, 0),
(18000606, 8417480, 5, 'npc_name_kill', 'npc_name', 174165, 'tribal hunter', 1, 174, 0, 0),
(18000607, 8417480, 6, 'npc_name_kill', 'npc_name', 174006, 'tribal leader', 1, 174, 0, 0),
(18000608, 8417480, 7, 'npc_name_kill', 'npc_name', 174162, 'tribal shaman', 1, 174, 0, 0),
(18000609, 8417580, 0, 'npc_name_kill', 'npc_name', 175281, 'bloodtribe ancient', 1, 175, 0, 0),
(18000610, 8417580, 1, 'npc_name_kill', 'npc_name', 175014, 'bloodtribe sneakster', 1, 175, 0, 0),
(18000611, 8417580, 2, 'npc_name_kill', 'npc_name', 175246, 'bloodtribe surveryor', 1, 175, 0, 0),
(18000612, 8417580, 3, 'npc_name_kill', 'npc_name', 175249, 'bloodtribe wiseman', 1, 175, 0, 0),
(18000613, 8417580, 4, 'npc_name_kill', 'npc_name', 175238, 'a gigantic sunflower', 1, 175, 0, 0),
(18000614, 8417580, 5, 'npc_name_kill', 'npc_name', 175256, 'a grol baku benefactor', 1, 175, 0, 0),
(18000615, 8417580, 6, 'npc_name_kill', 'npc_name', 175277, 'a grol baku bodyguard', 1, 175, 0, 0),
(18000616, 8417580, 7, 'npc_name_kill', 'npc_name', 175105, 'a grol baku keeper', 1, 175, 0, 0),
(18000617, 8417580, 8, 'npc_name_kill', 'npc_name', 175164, 'a grol baku retainer', 1, 175, 0, 0),
(18000618, 8417580, 9, 'npc_name_kill', 'npc_name', 175227, 'kraen flameweaver', 1, 175, 0, 0),
(18000619, 8417580, 10, 'npc_name_kill', 'npc_name', 175019, 'a lightcrawler drone', 1, 175, 0, 0),
(18000620, 8417580, 11, 'npc_name_kill', 'npc_name', 175020, 'a lively lightcrawler', 1, 175, 0, 0),
(18000621, 8417580, 12, 'npc_name_kill', 'npc_name', 175235, 'ruinous sun revenant', 1, 175, 0, 0),
(18000622, 8417580, 13, 'npc_name_kill', 'npc_name', 175218, 'sectoid', 1, 175, 0, 0),
(18000623, 8417580, 14, 'npc_name_kill', 'npc_name', 175223, 'sun revenant chancellor', 1, 175, 0, 0),
(18000624, 8417580, 15, 'npc_name_kill', 'npc_name', 175139, 'sun revenant desecrator', 1, 175, 0, 0),
(18000625, 8417580, 16, 'npc_name_kill', 'npc_name', 175004, 'sun revenant magistrate', 1, 175, 0, 0),
(18000626, 8417580, 17, 'npc_name_kill', 'npc_name', 175140, 'sun revenant primatus', 1, 175, 0, 0),
(18000627, 8417580, 18, 'npc_name_kill', 'npc_name', 175216, 'sun revenant smiter', 1, 175, 0, 0),
(18000628, 8417580, 19, 'npc_name_kill', 'npc_name', 175142, 'sun revenant warlock', 1, 175, 0, 0),
(18000629, 8417580, 20, 'npc_name_kill', 'npc_name', 175184, 'sunlord wedazi', 1, 175, 0, 0),
(18000630, 8417580, 21, 'npc_name_kill', 'npc_name', 175143, 'a thorny sunflower', 1, 175, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000631, 8417580, 22, 'npc_name_kill', 'npc_name', 175201, 'a tro jeg captor', 1, 175, 0, 0),
(18000632, 8417580, 23, 'npc_name_kill', 'npc_name', 175185, 'a tro jeg firekeeper', 1, 175, 0, 0),
(18000633, 8417580, 24, 'npc_name_kill', 'npc_name', 175189, 'a tro jeg heathen', 1, 175, 0, 0),
(18000634, 8417580, 25, 'npc_name_kill', 'npc_name', 175101, 'a vas ren archon', 1, 175, 0, 0),
(18000635, 8417680, 0, 'npc_name_kill', 'npc_name', 176092, 'bile spew', 1, 176, 0, 0),
(18000636, 8417680, 1, 'npc_name_kill', 'npc_name', 176095, 'bloodwretch', 1, 176, 0, 0),
(18000637, 8417680, 2, 'npc_name_kill', 'npc_name', 176027, 'fallen sergeant', 1, 176, 0, 0),
(18000638, 8417680, 3, 'npc_name_kill', 'npc_name', 176083, 'festersore', 1, 176, 0, 0),
(18000639, 8417680, 4, 'npc_name_kill', 'npc_name', 176115, 'fleshrot', 1, 176, 0, 0),
(18000640, 8417680, 5, 'npc_name_kill', 'npc_name', 176098, 'fyrthek fior', 1, 176, 0, 0),
(18000641, 8417680, 6, 'npc_name_kill', 'npc_name', 176091, 'a shadowy assassin', 1, 176, 0, 0),
(18000642, 8417680, 7, 'npc_name_kill', 'npc_name', 176100, 'shak dathor warlord', 1, 176, 0, 0),
(18000643, 8417680, 8, 'npc_name_kill', 'npc_name', 176107, 'sylra fris', 1, 176, 0, 0),
(18000644, 8417980, 0, 'npc_name_kill', 'npc_name', 179122, 'altar priest', 1, 179, 0, 0),
(18000645, 8417980, 1, 'npc_name_kill', 'npc_name', 179084, 'diabo`teka`temariel', 1, 179, 0, 0),
(18000646, 8417980, 2, 'npc_name_kill', 'npc_name', 179114, 'lorekeeper', 1, 179, 0, 0),
(18000647, 8417980, 3, 'npc_name_kill', 'npc_name', 179159, 'transient emissary', 1, 179, 0, 0),
(18000648, 8417980, 4, 'npc_name_kill', 'npc_name', 179179, 'transient xakra', 1, 179, 0, 0),
(18000649, 8417980, 5, 'npc_name_kill', 'npc_name', 179166, 'transient xin', 1, 179, 0, 0),
(18000650, 8417980, 6, 'npc_name_kill', 'npc_name', 179128, 'vius`tekar', 1, 179, 0, 0),
(18000651, 8417980, 7, 'npc_name_kill', 'npc_name', 179177, 'warlord', 1, 179, 0, 0),
(18000652, 8480980, 0, 'npc_name_kill', 'npc_name', 167337, 'avisiris', 1, 167, 0, 0),
(18000653, 8480980, 1, 'npc_name_kill', 'npc_name', 167334, 'a raspy grimling', 1, 167, 0, 0),
(18000654, 8480980, 2, 'npc_name_kill', 'npc_name', 167670, 'spiritmaster ugreth', 1, 167, 0, 0),
(18000655, 8480980, 3, 'npc_name_kill', 'npc_name', 167077, 'spymaster gephes', 1, 167, 0, 0),
(18000656, 8520080, 0, 'npc_name_kill', 'npc_name', 200072, 'assassin kakoo', 1, 200, 0, 0),
(18000657, 8520080, 1, 'npc_name_kill', 'npc_name', 200028, 'bubonian death caster', 1, 200, 0, 0),
(18000658, 8520080, 2, 'npc_name_kill', 'npc_name', 200029, 'bubonian great mystic', 1, 200, 0, 0),
(18000659, 8520080, 3, 'npc_name_kill', 'npc_name', 200026, 'bubonian warlord', 1, 200, 0, 0),
(18000660, 8520080, 4, 'npc_name_kill', 'npc_name', 200006, 'crazed bubonian berzerker', 1, 200, 0, 0),
(18000661, 8520080, 5, 'npc_name_kill', 'npc_name', 200069, 'deathbone archmagus', 1, 200, 0, 0),
(18000662, 8520080, 6, 'npc_name_kill', 'npc_name', 200014, 'elite corrupted knight', 1, 200, 0, 0),
(18000663, 8520080, 7, 'npc_name_kill', 'npc_name', 200015, 'elite death knight', 1, 200, 0, 0),
(18000664, 8520080, 8, 'npc_name_kill', 'npc_name', 200042, 'elite knight of decay', 1, 200, 0, 0),
(18000665, 8520080, 9, 'npc_name_kill', 'npc_name', 200063, 'elite necromancer of decay', 1, 200, 0, 0),
(18000666, 8520080, 10, 'npc_name_kill', 'npc_name', 200043, 'elite priest of decay', 1, 200, 0, 0),
(18000667, 8520080, 11, 'npc_name_kill', 'npc_name', 200073, 'eviscerator fwexar', 1, 200, 0, 0),
(18000668, 8520080, 12, 'npc_name_kill', 'npc_name', 200070, 'firebone archmagus', 1, 200, 0, 0),
(18000669, 8520080, 13, 'npc_name_kill', 'npc_name', 200019, 'greater corrupted pusling', 1, 200, 0, 0),
(18000670, 8520080, 14, 'npc_name_kill', 'npc_name', 200031, 'greater foul pusling', 1, 200, 0, 0),
(18000671, 8520080, 15, 'npc_name_kill', 'npc_name', 200074, 'high mystic qucosp', 1, 200, 0, 0),
(18000672, 8520080, 16, 'npc_name_kill', 'npc_name', 200030, 'pestilence archon', 1, 200, 0, 0),
(18000673, 8520080, 17, 'npc_name_kill', 'npc_name', 200058, 'plague lord hetral', 1, 200, 0, 0),
(18000674, 8520080, 18, 'npc_name_kill', 'npc_name', 200076, 'war chieftain dorwikak', 1, 200, 0, 0),
(18000675, 8520080, 19, 'npc_name_kill', 'npc_name', 200075, 'witchdoctor orxkra', 1, 200, 0, 0),
(18000676, 8520180, 0, 'npc_name_kill', 'npc_name', 201445, 'the ancient crawler', 1, 201, 0, 0),
(18000677, 8520180, 1, 'npc_name_kill', 'npc_name', 201442, 'a blood leech', 1, 201, 0, 0),
(18000678, 8520180, 2, 'npc_name_kill', 'npc_name', 201443, 'a diamond beetle', 1, 201, 0, 0),
(18000679, 8520180, 3, 'npc_name_kill', 'npc_name', 201334, 'a grallok appraisor', 1, 201, 0, 0),
(18000680, 8520180, 4, 'npc_name_kill', 'npc_name', 201385, 'a grallok crusader', 1, 201, 0, 0),
(18000681, 8520180, 5, 'npc_name_kill', 'npc_name', 201274, 'a grallok elder', 1, 201, 0, 0),
(18000682, 8520180, 6, 'npc_name_kill', 'npc_name', 201183, 'a grallok overseer', 1, 201, 0, 0),
(18000683, 8520180, 7, 'npc_name_kill', 'npc_name', 201502, 'the grallok underlord', 1, 201, 0, 0),
(18000684, 8520180, 8, 'npc_name_kill', 'npc_name', 201168, 'a screeching parasite', 1, 201, 0, 0),
(18000685, 8520180, 9, 'npc_name_kill', 'npc_name', 201422, 'the yrendan scarab', 1, 201, 0, 0),
(18000686, 8520480, 0, 'npc_name_kill', 'npc_name', 204043, 'an agony mephit', 1, 204, 0, 0),
(18000687, 8520480, 1, 'npc_name_kill', 'npc_name', 204049, 'a bullyrag bat', 1, 204, 0, 0),
(18000688, 8520480, 2, 'npc_name_kill', 'npc_name', 204044, 'a fearsome hobgoblin', 1, 204, 0, 0),
(18000689, 8520480, 3, 'npc_name_kill', 'npc_name', 204048, 'a fiendish consort', 1, 204, 0, 0),
(18000690, 8520480, 4, 'npc_name_kill', 'npc_name', 204033, 'an infernal consort', 1, 204, 0, 0),
(18000691, 8520480, 5, 'npc_name_kill', 'npc_name', 204050, 'a painwrack hobgoblin', 1, 204, 0, 0),
(18000692, 8520580, 0, 'npc_name_kill', 'npc_name', 205160, 'an arachnae parian', 1, 205, 0, 0),
(18000693, 8520580, 1, 'npc_name_kill', 'npc_name', 205148, 'a diseased infested bubonian', 1, 205, 0, 0),
(18000694, 8520580, 2, 'npc_name_kill', 'npc_name', 205159, 'a malarian metamo', 1, 205, 0, 0),
(18000695, 8520580, 3, 'npc_name_kill', 'npc_name', 205158, 'a sengian fly', 1, 205, 0, 0),
(18000696, 8520680, 0, 'npc_name_kill', 'npc_name', 206060, 'junk beast', 1, 206, 0, 0),
(18000697, 8520780, 0, 'npc_name_kill', 'npc_name', 207057, 'the avatar of agony', 1, 207, 0, 0),
(18000698, 8520780, 1, 'npc_name_kill', 'npc_name', 207054, 'the avatar of anguish', 1, 207, 0, 0),
(18000699, 8520780, 2, 'npc_name_kill', 'npc_name', 207067, 'the avatar of pain', 1, 207, 0, 0),
(18000700, 8520780, 3, 'npc_name_kill', 'npc_name', 207064, 'the avatar of suffering', 1, 207, 0, 0),
(18000701, 8520780, 4, 'npc_name_kill', 'npc_name', 207088, 'dirge malicia', 1, 207, 0, 0),
(18000702, 8520780, 5, 'npc_name_kill', 'npc_name', 207046, 'a pain devourer', 1, 207, 0, 0),
(18000703, 8520780, 6, 'npc_name_kill', 'npc_name', 207044, 'a sorrow seeker', 1, 207, 0, 0),
(18000704, 8520880, 0, 'npc_name_kill', 'npc_name', 208180, 'a diseased guard', 1, 208, 0, 0),
(18000705, 8520880, 1, 'npc_name_kill', 'npc_name', 208181, 'a festering worm', 1, 208, 0, 0),
(18000706, 8520880, 2, 'npc_name_kill', 'npc_name', 208164, 'lieutenant baelin dwinn', 1, 208, 0, 0),
(18000707, 8520880, 3, 'npc_name_kill', 'npc_name', 208006, 'a luminii crawler', 1, 208, 0, 0),
(18000708, 8520880, 4, 'npc_name_kill', 'npc_name', 208182, 'sergeant terrick burns', 1, 208, 0, 0),
(18000709, 8520880, 5, 'npc_name_kill', 'npc_name', 208082, 'a skeletal guardian', 1, 208, 0, 0),
(18000710, 8520880, 6, 'npc_name_kill', 'npc_name', 208167, 'a traglimite frog', 1, 208, 0, 0),
(18000711, 8520880, 7, 'npc_name_kill', 'npc_name', 208062, 'an undead doorman', 1, 208, 0, 0),
(18000712, 8520880, 8, 'npc_name_kill', 'npc_name', 208072, 'an undead footman', 1, 208, 0, 0),
(18000713, 8520980, 0, 'npc_name_kill', 'npc_name', 209080, 'amnquetil brynjulffr', 1, 209, 0, 0),
(18000714, 8520980, 1, 'npc_name_kill', 'npc_name', 209078, 'atle cloudburst', 1, 209, 0, 0),
(18000715, 8520980, 2, 'npc_name_kill', 'npc_name', 209077, 'bordir bjomolf', 1, 209, 0, 0),
(18000716, 8520980, 3, 'npc_name_kill', 'npc_name', 209109, 'elif whitewind', 1, 209, 0, 0),
(18000717, 8520980, 4, 'npc_name_kill', 'npc_name', 209073, 'erech eyford', 1, 209, 0, 0),
(18000718, 8520980, 5, 'npc_name_kill', 'npc_name', 209074, 'fogl iceshard', 1, 209, 0, 0),
(18000719, 8520980, 6, 'npc_name_kill', 'npc_name', 209121, 'galm snowdrift', 1, 209, 0, 0),
(18000720, 8520980, 7, 'npc_name_kill', 'npc_name', 209096, 'riodhr torrentwind', 1, 209, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000721, 8520980, 8, 'npc_name_kill', 'npc_name', 209081, 'rorek steelthorn', 1, 209, 0, 0),
(18000722, 8520980, 9, 'npc_name_kill', 'npc_name', 209098, 'thangbrand', 1, 209, 0, 0),
(18000723, 8520980, 10, 'npc_name_kill', 'npc_name', 209091, 'thorolf sutherland', 1, 209, 0, 0),
(18000724, 8520980, 11, 'npc_name_kill', 'npc_name', 209087, 'torstien stoneskin', 1, 209, 0, 0),
(18000725, 8520980, 12, 'npc_name_kill', 'npc_name', 209097, 'valbrand', 1, 209, 0, 0),
(18000726, 8520980, 13, 'npc_name_kill', 'npc_name', 209090, 'wybjorn', 1, 209, 0, 0),
(18000727, 8520980, 14, 'npc_name_kill', 'npc_name', 209083, 'ymir stormseer', 1, 209, 0, 0),
(18000728, 8521180, 0, 'npc_name_kill', 'npc_name', 211067, 'captain danon shern', 1, 211, 0, 0),
(18000729, 8521180, 1, 'npc_name_kill', 'npc_name', 211110, 'captain kage shou', 1, 211, 0, 0),
(18000730, 8521180, 2, 'npc_name_kill', 'npc_name', 211068, 'konid merdrid', 1, 211, 0, 0),
(18000731, 8521180, 3, 'npc_name_kill', 'npc_name', 211070, 'a protector of valor', 1, 211, 0, 0),
(18000732, 8521180, 4, 'npc_name_kill', 'npc_name', 211027, 'a wrulon protector', 1, 211, 0, 0),
(18000733, 8521180, 5, 'npc_name_kill', 'npc_name', 211009, 'a wrulon sentry', 1, 211, 0, 0),
(18000734, 8521280, 0, 'npc_name_kill', 'npc_name', 212072, 'a fervid magmin', 1, 212, 0, 0),
(18000735, 8521280, 1, 'npc_name_kill', 'npc_name', 212059, 'a flare mephit', 1, 212, 0, 0),
(18000736, 8521280, 2, 'npc_name_kill', 'npc_name', 212065, 'a pumice collector', 1, 212, 0, 0),
(18000737, 8521280, 3, 'npc_name_kill', 'npc_name', 212024, 'a radiant guardian', 1, 212, 0, 0),
(18000738, 8521280, 4, 'npc_name_kill', 'npc_name', 212066, 'a runed giant', 1, 212, 0, 0),
(18000739, 8521280, 5, 'npc_name_kill', 'npc_name', 212044, 'a sweltering mephit', 1, 212, 0, 0),
(18000740, 8521480, 0, 'npc_name_kill', 'npc_name', 214033, 'anival the blade', 1, 214, 0, 0),
(18000741, 8521480, 1, 'npc_name_kill', 'npc_name', 214121, 'the diaku supplier', 1, 214, 0, 0),
(18000742, 8521480, 2, 'npc_name_kill', 'npc_name', 214122, 'an enraged war boar', 1, 214, 0, 0),
(18000743, 8521480, 3, 'npc_name_kill', 'npc_name', 214027, 'a frenzied initiate', 1, 214, 0, 0),
(18000744, 8521480, 4, 'npc_name_kill', 'npc_name', 214075, 'hinvat deathbringer', 1, 214, 0, 0),
(18000745, 8521480, 5, 'npc_name_kill', 'npc_name', 214051, 'shadow master vinta', 1, 214, 0, 0),
(18000746, 8521480, 6, 'npc_name_kill', 'npc_name', 214103, 'zelrin morlock', 1, 214, 0, 0),
(18000747, 8521580, 0, 'npc_name_kill', 'npc_name', 215061, 'calebgrothiel', 1, 215, 0, 0),
(18000748, 8521580, 1, 'npc_name_kill', 'npc_name', 215060, 'lossenmachar', 1, 215, 0, 0),
(18000749, 8521580, 2, 'npc_name_kill', 'npc_name', 215058, 'a phoenix breezewing', 1, 215, 0, 0),
(18000750, 8521580, 3, 'npc_name_kill', 'npc_name', 215028, 'a phoenix searedwing', 1, 215, 0, 0),
(18000751, 8521680, 0, 'npc_name_kill', 'npc_name', 216093, 'an enormous frog', 1, 216, 0, 0),
(18000752, 8521680, 1, 'npc_name_kill', 'npc_name', 216091, 'ferocious barracuda', 1, 216, 0, 0),
(18000753, 8521680, 2, 'npc_name_kill', 'npc_name', 216092, 'frenzied anglerfish', 1, 216, 0, 0),
(18000754, 8521680, 3, 'npc_name_kill', 'npc_name', 216075, 'furious deepwater kraken', 1, 216, 0, 0),
(18000755, 8521680, 4, 'npc_name_kill', 'npc_name', 216037, 'gigadon', 1, 216, 0, 0),
(18000756, 8521680, 5, 'npc_name_kill', 'npc_name', 216095, 'hammertooth', 1, 216, 0, 0),
(18000757, 8521680, 6, 'npc_name_kill', 'npc_name', 216096, 'hraquis arch magus', 1, 216, 0, 0),
(18000758, 8521680, 7, 'npc_name_kill', 'npc_name', 216045, 'hraquis chieftain', 1, 216, 0, 0),
(18000759, 8521680, 8, 'npc_name_kill', 'npc_name', 216038, 'monstrous sea turtle', 1, 216, 0, 0),
(18000760, 8521680, 9, 'npc_name_kill', 'npc_name', 216056, 'razorfin', 1, 216, 0, 0),
(18000761, 8521680, 10, 'npc_name_kill', 'npc_name', 216053, 'regrua overlord', 1, 216, 0, 0),
(18000762, 8521680, 11, 'npc_name_kill', 'npc_name', 216097, 'regrua protector', 1, 216, 0, 0),
(18000763, 8521680, 12, 'npc_name_kill', 'npc_name', 216054, 'savage deepwater kraken', 1, 216, 0, 0),
(18000764, 8521680, 13, 'npc_name_kill', 'npc_name', 216050, 'swordfang', 1, 216, 0, 0),
(18000765, 8521680, 14, 'npc_name_kill', 'npc_name', 216051, 'triloun egg keeper', 1, 216, 0, 0),
(18000766, 8521680, 15, 'npc_name_kill', 'npc_name', 216105, 'triloun seer', 1, 216, 0, 0),
(18000767, 8521680, 16, 'npc_name_kill', 'npc_name', 216098, 'triloun warder', 1, 216, 0, 0),
(18000768, 8521780, 0, 'npc_name_kill', 'npc_name', 217018, 'captain of fire', 1, 217, 0, 0),
(18000769, 8521780, 1, 'npc_name_kill', 'npc_name', 217073, 'charmer of fire', 1, 217, 0, 0),
(18000770, 8521780, 2, 'npc_name_kill', 'npc_name', 217106, 'dark obsidian lava spider', 1, 217, 0, 0),
(18000771, 8521780, 3, 'npc_name_kill', 'npc_name', 217089, 'doomfire firecharmer', 1, 217, 0, 0),
(18000772, 8521780, 4, 'npc_name_kill', 'npc_name', 217062, 'doomfire magus', 1, 217, 0, 0),
(18000773, 8521780, 5, 'npc_name_kill', 'npc_name', 217090, 'doomfire reaver', 1, 217, 0, 0),
(18000774, 8521780, 6, 'npc_name_kill', 'npc_name', 217091, 'doomfire vicar', 1, 217, 0, 0),
(18000775, 8521780, 7, 'npc_name_kill', 'npc_name', 217092, 'doomfire warlord', 1, 217, 0, 0),
(18000776, 8521780, 8, 'npc_name_kill', 'npc_name', 217065, 'doomfire warmaster', 1, 217, 0, 0),
(18000777, 8521780, 9, 'npc_name_kill', 'npc_name', 217093, 'fiery spirit equine overlord', 1, 217, 0, 0),
(18000778, 8521780, 10, 'npc_name_kill', 'npc_name', 217064, 'flame overlord', 1, 217, 0, 0),
(18000779, 8521780, 11, 'npc_name_kill', 'npc_name', 217094, 'flame wilder', 1, 217, 0, 0),
(18000780, 8521780, 12, 'npc_name_kill', 'npc_name', 217095, 'jopal chieftain', 1, 217, 0, 0),
(18000781, 8521780, 13, 'npc_name_kill', 'npc_name', 217060, 'jopal crafter', 1, 217, 0, 0),
(18000782, 8521780, 14, 'npc_name_kill', 'npc_name', 217096, 'jopal flame protector', 1, 217, 0, 0),
(18000783, 8521780, 15, 'npc_name_kill', 'npc_name', 217097, 'jopal seer', 1, 217, 0, 0),
(18000784, 8521780, 16, 'npc_name_kill', 'npc_name', 217098, 'jopal tracker', 1, 217, 0, 0),
(18000785, 8521780, 17, 'npc_name_kill', 'npc_name', 217105, 'magma overlord', 1, 217, 0, 0),
(18000786, 8521780, 18, 'npc_name_kill', 'npc_name', 217099, 'obsidian tree spider queen', 1, 217, 0, 0),
(18000787, 8521780, 19, 'npc_name_kill', 'npc_name', 217040, 'obsidian war spider', 1, 217, 0, 0),
(18000788, 8521780, 20, 'npc_name_kill', 'npc_name', 217055, 'sorcerer of fire', 1, 217, 0, 0),
(18000789, 8521780, 21, 'npc_name_kill', 'npc_name', 217075, 'vicar of fire', 1, 217, 0, 0),
(18000790, 8521780, 22, 'npc_name_kill', 'npc_name', 217074, 'wild fiery spirit steed', 1, 217, 0, 0),
(18000791, 8522080, 0, 'npc_name_kill', 'npc_name', 220005, 'a wrulon protector', 1, 220, 0, 0),
(18000792, 8522180, 0, 'npc_name_kill', 'npc_name', 221001, 'protector of terris', 1, 221, 0, 0),
(18000793, 8522180, 1, 'npc_name_kill', 'npc_name', 221003, 'sentry of nightmares', 1, 221, 0, 0),
(18000794, 8127880, 0, 'npc_name_kill', 'npc_name', 278111, 'erpar flamegar', 1, 278, 0, 0),
(18000795, 8127880, 1, 'npc_name_kill', 'npc_name', 278063, 'gigantic ashwalker', 1, 278, 0, 0),
(18000796, 8127880, 2, 'npc_name_kill', 'npc_name', 278110, 'kushara perecran', 1, 278, 0, 0),
(18000797, 8127880, 3, 'npc_name_kill', 'npc_name', 278101, 'pulsating pool of magma', 1, 278, 0, 0),
(18000798, 8127880, 4, 'npc_name_kill', 'npc_name', 278058, 'servant of flame', 1, 278, 0, 0),
(18000799, 8127880, 5, 'npc_name_kill', 'npc_name', 278038, 'smoldering tentacle terror', 1, 278, 0, 0),
(18000800, 8127880, 6, 'npc_name_kill', 'npc_name', 278107, 'tybog adalmond', 1, 278, 0, 0),
(18000801, 8127880, 7, 'npc_name_kill', 'npc_name', 278069, 'typhoeus', 1, 278, 0, 0),
(18000802, 8127880, 8, 'npc_name_kill', 'npc_name', 278077, 'xaon bulzekel', 1, 278, 0, 0),
(18000803, 8622480, 0, 'npc_name_kill', 'npc_name', 224316, 'blood tusk', 1, 224, 0, 0),
(18000804, 8622480, 1, 'npc_name_kill', 'npc_name', 224318, 'a dastardly scoundrel', 1, 224, 0, 0),
(18000805, 8622480, 2, 'npc_name_kill', 'npc_name', 224317, 'dominator zrabix', 1, 224, 0, 0),
(18000806, 8622480, 3, 'npc_name_kill', 'npc_name', 224321, 'dreadmaster jrup', 1, 224, 0, 0),
(18000807, 8622480, 4, 'npc_name_kill', 'npc_name', 224322, 'guard dragrik', 1, 224, 0, 0),
(18000808, 8622480, 5, 'npc_name_kill', 'npc_name', 224281, 'hatethorn', 1, 224, 0, 0),
(18000809, 8622480, 6, 'npc_name_kill', 'npc_name', 224320, 'a human recruit', 1, 224, 0, 0),
(18000810, 8622480, 7, 'npc_name_kill', 'npc_name', 224326, 'kalii brokenskull', 1, 224, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"(

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
VALUES
(18000811, 8622480, 8, 'npc_name_kill', 'npc_name', 224327, 'magus evana bavomon', 1, 224, 0, 0),
(18000812, 8622480, 9, 'npc_name_kill', 'npc_name', 224239, 'mokon brokenskull', 1, 224, 0, 0),
(18000813, 8622480, 10, 'npc_name_kill', 'npc_name', 224319, 'a mutant cohort', 1, 224, 0, 0),
(18000814, 8622480, 11, 'npc_name_kill', 'npc_name', 224315, 'an old sea dog', 1, 224, 0, 0),
(18000815, 8622480, 12, 'npc_name_kill', 'npc_name', 224328, 'oracle pagrossa', 1, 224, 0, 0),
(18000816, 8622480, 13, 'npc_name_kill', 'npc_name', 224330, 'preceptor grakomus', 1, 224, 0, 0),
(18000817, 8622480, 14, 'npc_name_kill', 'npc_name', 224331, 'rite master kvimimn', 1, 224, 0, 0),
(18000818, 8622480, 15, 'npc_name_kill', 'npc_name', 224332, 'sea captain azobus', 1, 224, 0, 0),
(18000819, 8622480, 16, 'npc_name_kill', 'npc_name', 224333, 'soothsayer mrugg doxok', 1, 224, 0, 0),
(18000820, 8622480, 17, 'npc_name_kill', 'npc_name', 224334, 'treasure hunter eranil', 1, 224, 0, 0),
(18000821, 8622480, 18, 'npc_name_kill', 'npc_name', 224335, 'warmaster mragoc', 1, 224, 0, 0),
(18000822, 8622580, 0, 'npc_name_kill', 'npc_name', 225349, 'architect vaukin', 1, 225, 0, 0),
(18000823, 8622580, 1, 'npc_name_kill', 'npc_name', 225352, 'a blackblooded taskmaster', 1, 225, 0, 0),
(18000824, 8622580, 2, 'npc_name_kill', 'npc_name', 225377, 'a defiled dedicant', 1, 225, 0, 0),
(18000825, 8622580, 3, 'npc_name_kill', 'npc_name', 225348, 'an enraged soulstealer', 1, 225, 0, 0),
(18000826, 8622580, 4, 'npc_name_kill', 'npc_name', 225344, 'a fleshchild of innoruuk', 1, 225, 0, 0),
(18000827, 8622580, 5, 'npc_name_kill', 'npc_name', 225354, 'galikor sevalin', 1, 225, 0, 0),
(18000828, 8622580, 6, 'npc_name_kill', 'npc_name', 225355, 'head chef grishnak', 1, 225, 0, 0),
(18000829, 8622580, 7, 'npc_name_kill', 'npc_name', 225356, 'incantator cawrolis', 1, 225, 0, 0),
(18000830, 8622580, 8, 'npc_name_kill', 'npc_name', 225357, 'konus alatuk', 1, 225, 0, 0),
(18000831, 8622580, 9, 'npc_name_kill', 'npc_name', 225358, 'linlanik throatcutter', 1, 225, 0, 0),
(18000832, 8622580, 10, 'npc_name_kill', 'npc_name', 225383, 'a luggald assassin', 1, 225, 0, 0),
(18000833, 8622580, 11, 'npc_name_kill', 'npc_name', 225304, 'an ore refinery overseer', 1, 225, 0, 0),
(18000834, 8622580, 12, 'npc_name_kill', 'npc_name', 225326, 'a shrouded cave lurker', 1, 225, 0, 0),
(18000835, 8622580, 13, 'npc_name_kill', 'npc_name', 225345, 'a shrouded fareyes', 1, 225, 0, 0),
(18000836, 8622580, 14, 'npc_name_kill', 'npc_name', 225031, 'treasure sorter neiben', 1, 225, 0, 0),
(18000837, 8622580, 15, 'npc_name_kill', 'npc_name', 225366, 'vurag', 1, 225, 0, 0),
(18000838, 8622580, 16, 'npc_name_kill', 'npc_name', 225372, 'xorikaan farzlebual', 1, 225, 0, 0),
(18000839, 8622680, 0, 'npc_name_kill', 'npc_name', 226188, 'bishop rimak', 1, 226, 0, 0),
(18000840, 8622680, 1, 'npc_name_kill', 'npc_name', 226194, 'the broken skull bloodlord', 1, 226, 0, 0),
(18000841, 8622680, 2, 'npc_name_kill', 'npc_name', 226195, 'the broken skull brawler', 1, 226, 0, 0),
(18000842, 8622680, 3, 'npc_name_kill', 'npc_name', 226196, 'the broken skull defender', 1, 226, 0, 0),
(18000843, 8622680, 4, 'npc_name_kill', 'npc_name', 226197, 'the broken skull dreadlord', 1, 226, 0, 0),
(18000844, 8622680, 5, 'npc_name_kill', 'npc_name', 226198, 'the broken skull mercenary', 1, 226, 0, 0),
(18000845, 8622680, 6, 'npc_name_kill', 'npc_name', 226199, 'the broken skull seer', 1, 226, 0, 0),
(18000846, 8622680, 7, 'npc_name_kill', 'npc_name', 226100, 'the broken skull warlord', 1, 226, 0, 0),
(18000847, 8622680, 8, 'npc_name_kill', 'npc_name', 226116, 'the corrupted half elf miner', 1, 226, 0, 0),
(18000848, 8622680, 9, 'npc_name_kill', 'npc_name', 226101, 'the crazed halfling lunatic', 1, 226, 0, 0),
(18000849, 8622680, 10, 'npc_name_kill', 'npc_name', 226189, 'dayned the insane', 1, 226, 0, 0),
(18000850, 8622680, 11, 'npc_name_kill', 'npc_name', 226102, 'the fanatical dwarf zealot', 1, 226, 0, 0),
(18000851, 8622680, 12, 'npc_name_kill', 'npc_name', 226190, 'foreman deslug', 1, 226, 0, 0),
(18000852, 8622680, 13, 'npc_name_kill', 'npc_name', 226103, 'the forsaken miner', 1, 226, 0, 0),
(18000853, 8622680, 14, 'npc_name_kill', 'npc_name', 226104, 'the luggald defiler', 1, 226, 0, 0),
(18000854, 8622680, 15, 'npc_name_kill', 'npc_name', 226105, 'the maniacal kobold miner', 1, 226, 0, 0),
(18000855, 8622680, 16, 'npc_name_kill', 'npc_name', 226106, 'the mutated laborer', 1, 226, 0, 0),
(18000856, 8622680, 17, 'npc_name_kill', 'npc_name', 226191, 'ritualist tzobodin', 1, 226, 0, 0),
(18000857, 8622680, 18, 'npc_name_kill', 'npc_name', 226192, 'scryer xvalos', 1, 226, 0, 0),
(18000858, 8622680, 19, 'npc_name_kill', 'npc_name', 226193, 'taskmaster waggad brokenskull', 1, 226, 0, 0),
(18000859, 8622780, 0, 'npc_name_kill', 'npc_name', 227109, 'a blackhand lieutenant', 1, 227, 0, 0),
(18000860, 8622780, 1, 'npc_name_kill', 'npc_name', 227025, 'a blackhand veteran', 1, 227, 0, 0),
(18000861, 8622780, 2, 'npc_name_kill', 'npc_name', 227224, 'a broken skull prophet', 1, 227, 0, 0),
(18000862, 8622780, 3, 'npc_name_kill', 'npc_name', 227301, 'a broken skull vassal', 1, 227, 0, 0),
(18000863, 8622780, 4, 'npc_name_kill', 'npc_name', 227307, 'captain aivilo', 1, 227, 0, 0),
(18000864, 8622780, 5, 'npc_name_kill', 'npc_name', 227298, 'a cloister sentinel', 1, 227, 0, 0),
(18000865, 8622780, 6, 'npc_name_kill', 'npc_name', 227127, 'a decaying fisherman', 1, 227, 0, 0),
(18000866, 8622780, 7, 'npc_name_kill', 'npc_name', 227108, 'dulein gedasai', 1, 227, 0, 0),
(18000867, 8622780, 8, 'npc_name_kill', 'npc_name', 227126, 'elder shaman elgruk', 1, 227, 0, 0),
(18000868, 8622780, 9, 'npc_name_kill', 'npc_name', 227114, 'an exhausted bloodtusk', 1, 227, 0, 0),
(18000869, 8622780, 10, 'npc_name_kill', 'npc_name', 227124, 'foreman kraksanar', 1, 227, 0, 0),
(18000870, 8622780, 11, 'npc_name_kill', 'npc_name', 227316, 'kdansol borgir', 1, 227, 0, 0),
(18000871, 8622780, 12, 'npc_name_kill', 'npc_name', 227317, 'kraska mreth', 1, 227, 0, 0),
(18000872, 8622780, 13, 'npc_name_kill', 'npc_name', 227318, 'spiritcharmer hargortaz', 1, 227, 0, 0),
(18000873, 8622780, 14, 'npc_name_kill', 'npc_name', 227123, 'an undead weaponsmith', 1, 227, 0, 0),
(18000874, 8622880, 0, 'npc_name_kill', 'npc_name', 228097, 'cabin gnome fitzgerald', 1, 228, 0, 0),
(18000875, 8622880, 1, 'npc_name_kill', 'npc_name', 228099, 'chef gokig', 1, 228, 0, 0),
(18000876, 8622880, 2, 'npc_name_kill', 'npc_name', 228100, 'harbinger iinati', 1, 228, 0, 0),
(18000877, 8622880, 3, 'npc_name_kill', 'npc_name', 228098, 'high priest anaanci', 1, 228, 0, 0),
(18000878, 8622880, 4, 'npc_name_kill', 'npc_name', 228056, 'lieutenant commander vzain', 1, 228, 0, 0),
(18000879, 8622880, 5, 'npc_name_kill', 'npc_name', 228105, 'the luggald interrogator', 1, 228, 0, 0),
(18000880, 8622880, 6, 'npc_name_kill', 'npc_name', 228101, 'quartermaster fisan', 1, 228, 0, 0),
(18000881, 8622880, 7, 'npc_name_kill', 'npc_name', 228052, 'scout shin`ci', 1, 228, 0, 0),
(18000882, 8622880, 8, 'npc_name_kill', 'npc_name', 228103, 'serang vikch', 1, 228, 0, 0),
(18000883, 8622880, 9, 'npc_name_kill', 'npc_name', 228104, 'shoqui the forgotten', 1, 228, 0, 0),
(18000884, 8622880, 10, 'npc_name_kill', 'npc_name', 228106, 'uumuvan the soulless', 1, 228, 0, 0),
(18000885, 8714280, 0, 'npc_name_kill', 'npc_name', 242024, 'ruined amalgam', 1, 242, 0, 0),
(18000886, 8745080, 0, 'npc_name_kill', 'npc_name', 269008, 'rebellious arcanist', 1, 269, 0, 0)
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`required_count` = VALUES(`required_count`),
`zone_id` = VALUES(`zone_id`);
)"
R"()",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version = 4,
		.description = "2026_05_27_custom_achievement_rewards",
		.check = "SHOW TABLES LIKE 'custom_achievement_rewards'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE IF NOT EXISTS `custom_achievement_rewards` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_type` VARCHAR(32) NOT NULL DEFAULT '',
  `reward_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount` INT UNSIGNED NOT NULL DEFAULT 1,
  `chance` INT UNSIGNED NOT NULL DEFAULT 10000,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `claim_once` TINYINT(1) NOT NULL DEFAULT 1,
  `auto_claim` TINYINT(1) NOT NULL DEFAULT 0,
  `preview_text` VARCHAR(255) NOT NULL DEFAULT '',
  `data_text` VARCHAR(255) NOT NULL DEFAULT '',
  `enabled` TINYINT(1) NOT NULL DEFAULT 1,
  `sort_order` INT NOT NULL DEFAULT 0,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_reward_identity` (`achievement_id`, `reward_type`, `reward_id`, `amount`, `preview_text`(96), `data_text`(96)),
  KEY `idx_achievement_sort` (`achievement_id`, `sort_order`, `id`),
  KEY `idx_type_tier` (`reward_type`, `tier`)
);

CREATE TABLE IF NOT EXISTS `custom_character_achievement_rewards` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_definition_id` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `reward_type` VARCHAR(32) NOT NULL DEFAULT '',
  `reward_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount` INT UNSIGNED NOT NULL DEFAULT 1,
  `auto_claim` TINYINT(1) NOT NULL DEFAULT 0,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `preview_text` VARCHAR(255) NOT NULL DEFAULT '',
  `data_text` VARCHAR(255) NOT NULL DEFAULT '',
  `status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `completion_count` INT UNSIGNED NOT NULL DEFAULT 1,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `claimed_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `result_text` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_character_reward` (`character_id`, `achievement_id`, `reward_definition_id`, `reward_type`, `reward_id`, `completion_count`),
  KEY `idx_character_status` (`character_id`, `status`, `created_at`),
  KEY `idx_achievement` (`achievement_id`)
);

CREATE TABLE IF NOT EXISTS `custom_achievement_live_item_requests` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_queue_id` BIGINT UNSIGNED NOT NULL,
  `level_band` INT UNSIGNED NOT NULL DEFAULT 0,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `item_slot` INT UNSIGNED NOT NULL DEFAULT 0,
  `theme` VARCHAR(255) NOT NULL DEFAULT '',
  `status` VARCHAR(32) NOT NULL DEFAULT 'pending',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `fulfilled_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `generated_item_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `result_text` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_reward_queue` (`reward_queue_id`),
  KEY `idx_character_status` (`character_id`, `status`, `created_at`),
  KEY `idx_achievement` (`achievement_id`)
);

CREATE TABLE IF NOT EXISTS `custom_account_achievement_unlocks` (
  `account_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_definition_id` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `unlock_type` VARCHAR(32) NOT NULL DEFAULT '',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`, `achievement_id`, `reward_definition_id`, `unlock_type`)
);

INSERT INTO `custom_achievement_categories`
(`id`, `parent_id`, `name`, `description`, `sort_order`, `icon_id`, `enabled`)
VALUES
(900, 0, 'Meta', 'Cross-category achievement collections and prestige rewards.', 900, 0, 1)
ON DUPLICATE KEY UPDATE
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);

INSERT INTO `custom_achievements`
(`id`, `category_id`, `slug`, `name`, `description`, `points`, `hidden`, `repeatable`, `sort_order`, `created_at`, `enabled`)
VALUES
(900001, 900, 'meta_classic_explorer', 'Classic Explorer', 'Complete every Classic exploration achievement.', 50, 0, 0, 10, UNIX_TIMESTAMP(), 1),
(900002, 900, 'meta_norrathian_slayer', 'Norrathian Slayer', 'Complete every veteran creature slayer achievement.', 75, 0, 0, 20, UNIX_TIMESTAMP(), 1),
(900003, 900, 'meta_journeyman_artisan', 'Journeyman Artisan', 'Raise the seven primary tradeskills to 200.', 40, 0, 0, 30, UNIX_TIMESTAMP(), 1),
(900004, 900, 'meta_master_artisan', 'Master Artisan', 'Raise the seven primary tradeskills to 300.', 75, 0, 0, 40, UNIX_TIMESTAMP(), 1)
ON DUPLICATE KEY UPDATE
`category_id` = VALUES(`category_id`),
`name` = VALUES(`name`),
`description` = VALUES(`description`),
`points` = VALUES(`points`),
`sort_order` = VALUES(`sort_order`),
`enabled` = VALUES(`enabled`);

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
19000000 + z.`zoneidnumber`,
900001,
z.`zoneidnumber`,
'achievement_complete',
'achievement',
200000 + z.`zoneidnumber`,
LEFT(CONCAT('Explorer: ', z.`long_name`), 128),
1,
z.`zoneidnumber`,
0,
0
FROM `zone` z
WHERE z.`version` = 0
AND z.`min_status` = 0
AND z.`zoneidnumber` > 0
AND z.`expansion` = 0
AND COALESCE(z.`short_name`, '') <> ''
AND COALESCE(z.`long_name`, '') <> ''
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`),
`zone_id` = VALUES(`zone_id`);

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
19100000 + a.`id`,
900002,
a.`id`,
'achievement_complete',
'achievement',
a.`id`,
LEFT(a.`name`, 128),
1,
0,
0,
0
FROM `custom_achievements` a
WHERE a.`slug` LIKE 'race_slayer\_%\_250'
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_index` = VALUES(`objective_index`),
`objective_type` = VALUES(`objective_type`),
`target_type` = VALUES(`target_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`);

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
19200000 + s.`skill_id`,
900003,
s.`skill_id`,
'achievement_complete',
'achievement',
400000 + s.`skill_id` * 1000 + 200,
CONCAT(s.`name`, ' 200'),
1,
0,
0,
0
FROM (
SELECT 60 AS `skill_id`, 'Baking' AS `name` UNION ALL
SELECT 61, 'Tailoring' UNION ALL
SELECT 63, 'Blacksmithing' UNION ALL
SELECT 64, 'Fletching' UNION ALL
SELECT 65, 'Brewing' UNION ALL
SELECT 68, 'Jewelcrafting' UNION ALL
SELECT 69, 'Pottery'
) s
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`);

INSERT INTO `custom_achievement_objectives`
(`id`, `achievement_id`, `objective_index`, `objective_type`, `target_type`, `target_id`, `target_name`, `required_count`, `zone_id`, `class_mask`, `optional`)
SELECT
19300000 + s.`skill_id`,
900004,
s.`skill_id`,
'achievement_complete',
'achievement',
400000 + s.`skill_id` * 1000 + 300,
CONCAT(s.`name`, ' 300'),
1,
0,
0,
0
FROM (
SELECT 60 AS `skill_id`, 'Baking' AS `name` UNION ALL
SELECT 61, 'Tailoring' UNION ALL
SELECT 63, 'Blacksmithing' UNION ALL
SELECT 64, 'Fletching' UNION ALL
SELECT 65, 'Brewing' UNION ALL
SELECT 68, 'Jewelcrafting' UNION ALL
SELECT 69, 'Pottery'
) s
ON DUPLICATE KEY UPDATE
`achievement_id` = VALUES(`achievement_id`),
`objective_type` = VALUES(`objective_type`),
`target_id` = VALUES(`target_id`),
`target_name` = VALUES(`target_name`);

INSERT INTO `custom_achievement_rewards`
(`achievement_id`, `reward_type`, `reward_id`, `amount`, `chance`, `tier`, `claim_once`, `auto_claim`, `preview_text`, `data_text`, `enabled`, `sort_order`, `created_at`)
VALUES
(1001, 'coin', 0, 1000, 10000, 'minor', 1, 1, 'Starter purse: 1 platinum', '', 1, 10, UNIX_TIMESTAMP()),
(1002, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Adventurer', 'Adventurer', 1, 20, UNIX_TIMESTAMP()),
(1003, 'coin', 0, 5000, 10000, 'minor', 1, 1, 'Travel purse: 5 platinum', '', 1, 30, UNIX_TIMESTAMP()),
(1004, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Pathfinder', 'Pathfinder', 1, 40, UNIX_TIMESTAMP()),
(1005, 'live_item_request', 0, 50, 10000, 'major', 1, 0, 'Leveled item cache: level 50 adventurer', 'level milestone', 1, 50, UNIX_TIMESTAMP()),
(1006, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Veteran', 'Veteran', 1, 60, UNIX_TIMESTAMP()),
(1007, 'live_item_request', 0, 65, 10000, 'heroic', 1, 0, 'Leveled item cache: level 65 veteran', 'level milestone', 1, 65, UNIX_TIMESTAMP()),
(1008, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Champion', 'Champion', 1, 70, UNIX_TIMESTAMP()),
(1008, 'live_item_request', 0, 70, 10000, 'legendary', 1, 0, 'Leveled item cache: level 70 champion', 'level milestone', 1, 71, UNIX_TIMESTAMP()),
(900001, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Classic Explorer', 'Classic Explorer', 1, 10, UNIX_TIMESTAMP()),
(900001, 'live_item_request', 0, 50, 10000, 'utility', 1, 0, 'Journeyman compass-style travel reward request', 'exploration utility', 1, 20, UNIX_TIMESTAMP()),
(900002, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Norrathian Slayer', 'Norrathian Slayer', 1, 10, UNIX_TIMESTAMP()),
(900002, 'live_item_request', 0, 70, 10000, 'legendary', 1, 0, 'Legendary hunter reward request', 'slayer trophy', 1, 20, UNIX_TIMESTAMP()),
(900003, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Journeyman Artisan', 'Journeyman Artisan', 1, 10, UNIX_TIMESTAMP()),
(900004, 'title_text', 0, 1, 10000, 'title', 1, 1, 'Title: Master Artisan', 'Master Artisan', 1, 10, UNIX_TIMESTAMP()),
(900004, 'live_item_request', 0, 70, 10000, 'heroic', 1, 0, 'Master artisan tool reward request', 'tradeskill tool', 1, 20, UNIX_TIMESTAMP())
ON DUPLICATE KEY UPDATE
`reward_type` = VALUES(`reward_type`),
`reward_id` = VALUES(`reward_id`),
`amount` = VALUES(`amount`),
`chance` = VALUES(`chance`),
`tier` = VALUES(`tier`),
`claim_once` = VALUES(`claim_once`),
`auto_claim` = VALUES(`auto_claim`),
`preview_text` = VALUES(`preview_text`),
`data_text` = VALUES(`data_text`),
`enabled` = VALUES(`enabled`),
`sort_order` = VALUES(`sort_order`);

INSERT INTO `custom_achievement_rewards`
(`achievement_id`, `reward_type`, `reward_id`, `amount`, `chance`, `tier`, `claim_once`, `auto_claim`, `preview_text`, `data_text`, `enabled`, `sort_order`, `created_at`)
SELECT
a.`id`,
'title_text',
0,
1,
10000,
'title',
1,
1,
CONCAT('Title: ', t.`title_name`),
t.`title_name`,
1,
100 + t.`threshold`,
UNIX_TIMESTAMP()
FROM `custom_achievements` a
JOIN (
SELECT 55 AS `skill_id`, 100 AS `threshold`, 'Apprentice Fisherman' AS `title_name` UNION ALL
SELECT 55, 200, 'Journeyman Fisherman' UNION ALL
SELECT 55, 250, 'Expert Fisherman' UNION ALL
SELECT 55, 300, 'Master Fisherman' UNION ALL
SELECT 60, 100, 'Apprentice Chef' UNION ALL
SELECT 60, 200, 'Journeyman Chef' UNION ALL
SELECT 60, 250, 'Expert Chef' UNION ALL
SELECT 60, 300, 'Master Chef' UNION ALL
SELECT 61, 100, 'Apprentice Tailor' UNION ALL
SELECT 61, 200, 'Journeyman Tailor' UNION ALL
SELECT 61, 250, 'Expert Tailor' UNION ALL
SELECT 61, 300, 'Master Tailor' UNION ALL
SELECT 63, 100, 'Apprentice Smith' UNION ALL
SELECT 63, 200, 'Journeyman Smith' UNION ALL
SELECT 63, 250, 'Expert Smith' UNION ALL
SELECT 63, 300, 'Master Smith' UNION ALL
SELECT 64, 100, 'Apprentice Fletcher' UNION ALL
SELECT 64, 200, 'Journeyman Fletcher' UNION ALL
SELECT 64, 250, 'Expert Fletcher' UNION ALL
SELECT 64, 300, 'Master Fletcher' UNION ALL
SELECT 65, 100, 'Apprentice Brewer' UNION ALL
SELECT 65, 200, 'Journeyman Brewer' UNION ALL
SELECT 65, 250, 'Expert Brewer' UNION ALL
SELECT 65, 300, 'Master Brewer' UNION ALL
SELECT 68, 100, 'Apprentice Jeweler' UNION ALL
SELECT 68, 200, 'Journeyman Jeweler' UNION ALL
SELECT 68, 250, 'Expert Jeweler' UNION ALL
SELECT 68, 300, 'Master Jeweler' UNION ALL
SELECT 69, 100, 'Apprentice Potter' UNION ALL
SELECT 69, 200, 'Journeyman Potter' UNION ALL
SELECT 69, 250, 'Expert Potter' UNION ALL
SELECT 69, 300, 'Master Potter'
) t
ON a.`slug` = CONCAT('skill_', t.`skill_id`, '_', t.`threshold`)
ON DUPLICATE KEY UPDATE
`preview_text` = VALUES(`preview_text`),
`data_text` = VALUES(`data_text`),
`enabled` = VALUES(`enabled`),
`sort_order` = VALUES(`sort_order`);

INSERT INTO `custom_achievement_rewards`
(`achievement_id`, `reward_type`, `reward_id`, `amount`, `chance`, `tier`, `claim_once`, `auto_claim`, `preview_text`, `data_text`, `enabled`, `sort_order`, `created_at`)
SELECT
a.`id`,
'title_text',
0,
1,
10000,
'title',
1,
1,
CONCAT('Title: ', r.`title_name`),
r.`title_name`,
1,
250,
UNIX_TIMESTAMP()
FROM `custom_achievements` a
JOIN (
SELECT 13 AS `race_id`, 'Aviak Slayer' AS `title_name` UNION ALL
SELECT 17, 'Golem Slayer' UNION ALL
SELECT 18, 'Giant Slayer' UNION ALL
SELECT 21, 'Evil Eye Slayer' UNION ALL
SELECT 25, 'Fairy Slayer' UNION ALL
SELECT 26, 'Frog Slayer' UNION ALL
SELECT 33, 'Ghoul Slayer' UNION ALL
SELECT 38, 'Spider Killer' UNION ALL
SELECT 39, 'Gnoll Slayer' UNION ALL
SELECT 40, 'Goblin Slayer' UNION ALL
SELECT 42, 'Wolf' UNION ALL
SELECT 43, 'Bear Slayer' UNION ALL
SELECT 48, 'Kobold Slayer' UNION ALL
SELECT 51, 'Lizard Man Slayer' UNION ALL
SELECT 53, 'Minotaur Slayer' UNION ALL
SELECT 54, 'Orc Slayer' UNION ALL
SELECT 60, 'Skeleton Slayer' UNION ALL
SELECT 65, 'Vampire Slayer' UNION ALL
SELECT 70, 'Zombie Slayer' UNION ALL
SELECT 75, 'Elemental Slayer' UNION ALL
SELECT 79, 'Bixie Slayer' UNION ALL
SELECT 88, 'Clockwork Slayer' UNION ALL
SELECT 89, 'Drakebane'
) r
ON a.`slug` = CONCAT('race_slayer_', r.`race_id`, '_250')
ON DUPLICATE KEY UPDATE
`preview_text` = VALUES(`preview_text`),
`data_text` = VALUES(`data_text`),
`enabled` = VALUES(`enabled`),
`sort_order` = VALUES(`sort_order`);
)",
		.content_schema_update = false,
	},

	// ============================================================================================
	// AoTv4 CONTENT MIGRATIONS -- baseline starts at version 5 (2026-08-03)
	// ============================================================================================
	// ⚠️⚠️ EVERYTHING BEFORE v5 IS ASSUMED ALREADY PRESENT. ~123 loose scripts in
	// .devcontainer/custom/sql/ were applied to live BY HAND over months and nothing recorded which,
	// so they cannot be replayed safely -- several carry `DELETE FROM ... BETWEEN <range>` cleanups,
	// and section 5 records a real incident where one script's range silently swallowed six spells
	// belonging to another. Re-running the directory would destroy content a later script created.
	// v5 is therefore a LINE IN THE SAND, not a from-scratch history: it is the first change tracked
	// here, and every AoTv4 content change from now on should be added as a new entry rather than
	// left as a loose file.
	//
	// ⚠️ Each entry is SELF-GUARDING via its own `.check` -- the version number decides what is
	// considered, but the check decides what actually runs. That is what makes these safe on a
	// database that already has some of them applied by hand, which live does.
	// ⚠️ These are content edits, not schema, so `.content_schema_update = false` like the rest.
	//
	// ⚠️⚠️ v5, v8 and v9 all touch `spells_new`, which is SHARED MEMORY. World applies them at boot,
	// but they stay INERT in game until:
	//     world + zones down  ->  cd build/bin && ./shared_memory  ->  world up
	// The loose .sql files remain in .devcontainer/custom/sql/ as the readable record of WHY each of
	// these exists; the reasoning is not duplicated here.

	ManifestEntry{
		.version     = 5,
		.description = "2026_08_03_aotv4_aura_inherited_slots",
		// The 16 class auras + 6 aura procs inherited slots 11/12 (+2% proc chance, +300 AC) from
		// their clone source, stock 8469 Champion's Aura Effect -- a level 68 raid ability.
		.check       = "SELECT id FROM spells_new WHERE (id BETWEEN 43550 AND 43565 OR id BETWEEN 43570 AND 43575) AND (effectid11 <> 254 OR effectid12 <> 254)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 0, max11 = 0,
       effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 0, max12 = 0
 WHERE id BETWEEN 43550 AND 43565
    OR id BETWEEN 43570 AND 43575;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 6,
		.description = "2026_08_03_aotv4_open_caster_and_craft_skill_caps",
		// A missing skill_caps row reads as cap 0, which is indistinguishable from "this class cannot
		// have this skill" -- so the four pure-melee classes had NO casting skills at all, instruments
		// started at level 10, and Make Poison/Alchemy were absent for 15 of 16 classes.
		// Warrior + Channeling at level 1 is the canary: absent means unapplied.
		.check       = "SELECT class_id FROM skill_caps WHERE class_id = 1 AND skill_id = 13 AND level = 1",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT best.skill_id, c.class_id, best.level, best.cap, 0
FROM (SELECT DISTINCT class_id FROM skill_caps) c
CROSS JOIN (
    SELECT skill_id, level, MAX(cap) AS cap
    FROM skill_caps
    WHERE skill_id IN (4, 5, 13, 14, 18, 24, 43, 44, 45, 46, 47, 12, 41, 49, 54, 70, 56, 59)
    GROUP BY skill_id, level
) best
LEFT JOIN skill_caps sc
       ON sc.class_id = c.class_id AND sc.skill_id = best.skill_id AND sc.level = best.level
WHERE sc.id IS NULL;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 7,
		.description = "2026_08_03_aotv4_region_unlock_chance",
		// `chance` is rolled out of TEN THOUSAND (achievement_manager.cpp), and the five "Ending"
		// rewards were created with 100 -- so dying at the level cap opened a region roughly one time
		// in a hundred. Every other reward type on the server uses 10000.
		.check       = "SELECT id FROM custom_achievement_rewards WHERE reward_type = 'region_choice' AND chance < 10000",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE custom_achievement_rewards
   SET chance = 10000
 WHERE reward_type = 'region_choice' AND chance < 10000;

DELETE FROM custom_achievement_rewards
 WHERE achievement_id = 1003 AND reward_type = 'region_choice';
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 8,
		.description = "2026_08_03_aotv4_reptile_trigger_slot8",
		// Cloned from stock 8009, which carries 393 in BOTH slot 1 and slot 8. The clone overrode
		// slot 1 with the tier ladder and left slot 8, so every tier healed a flat 393 -- including
		// the level 8 one, nominally a 20 point heal.
		.check       = "SELECT id FROM spells_new WHERE id BETWEEN 43350 AND 43355 AND effectid8 <> 254",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 0, max8 = 0
 WHERE id BETWEEN 43350 AND 43355;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 9,
		.description = "2026_08_03_aotv4_reptile_trigger_instant",
		// The payload landed as a 1 tick BUFF (inherited buffdurationformula 3), and our own
		// anti-exploit guard suppresses SE_CurrentHPOnce whenever it takes a buff slot -- so the slot 1
		// heal had never fired. Only the slot 8 HoT removed in v8 was ever healing. A proc payload has
		// no reason to persist.
		// ⚠️ MUST come after v8: alone it would make the 393 fire per proc instead of over time.
		.check       = "SELECT id FROM spells_new WHERE id BETWEEN 43350 AND 43355 AND buffdurationformula <> 0",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET buffdurationformula = 0, buffduration = 0
 WHERE id BETWEEN 43350 AND 43355;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 10,
		.description = "2026_08_03_aotv4_marker_slot_separation",
		// v-earlier gave four custom lines an ascending SPA 44 marker so a higher tier replaces a
		// lower -- but put reptile, sloth AND thirst in the SAME slot with the SAME values.
		// CheckStackConflict only compares effects at the same INDEX, so three unrelated lines began
		// evicting one another ("Thirst and Skin lines are overwriting each other"). The slot index is
		// what isolates lines; the value only orders tiers within one.
		//   reptile slot 1 | sloth slot 2 | kindred slot 3 | thirst slot 4
		// ⚠️ Any future line needing a marker takes slot 5 -- reusing 1-4 silently recreates this, and
		// it presents as two unrelated spells cancelling each other.
		.check       = "SELECT id FROM spells_new WHERE (id BETWEEN 43306 AND 43311 AND effectid1 = 44) OR (id BETWEEN 43330 AND 43335 AND effectid2 = 44) OR (id BETWEEN 43342 AND 43347 AND effectid1 = 44)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET effectid2 = 44, effect_base_value2 = effect_base_value1, effect_limit_value2 = 0,
       formula2 = 100, max2 = 0,
       effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 0, max1 = 0
 WHERE id BETWEEN 43306 AND 43311 AND effectid1 = 44;

UPDATE spells_new
   SET effectid3 = 44, effect_base_value3 = effect_base_value2, effect_limit_value3 = 0,
       formula3 = 100, max3 = 0,
       effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 0, max2 = 0
 WHERE id BETWEEN 43330 AND 43335 AND effectid2 = 44;

UPDATE spells_new
   SET effectid4 = 44, effect_base_value4 = effect_base_value1, effect_limit_value4 = 0,
       formula4 = 100, max4 = 0,
       effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 0, max1 = 0
 WHERE id BETWEEN 43342 AND 43347 AND effectid1 = 44;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 11,
		.description = "2026_08_04_aotv4_bard_mend_feign_death",
		// Reported as "everyone is getting mend and feign death". There were TWO independent causes and
		// this migration is the SERVER half; the client half is a dinput8.dll change (the GetSkillCap
		// reveal hook advertised both to every class, so they showed in the Skills window and could be
		// hotbarred even though the server refused them).
		//
		// The server half is Bard only, and it is a leftover of the Bard-only era: while every character
		// was forced to Bard, class 8's skill_caps were opened across the board so a Bard could use
		// everything. Bard still carries caps for ALL 77 skills where every other class has 62-68 --
		// so grant_free_skills (which gates on CanHaveSkill -> skill_caps) really did hand Bards Mend
		// and Feign Death. Monk keeps both; nothing else is touched.
		//
		// ⚠️ Existing Bards need no data fixup: max_skills_for_level zeroes any skill the class can no
		// longer have on the next connect, so the value clears itself.
		// ⚠️ skill_caps is NOT shared memory -- SkillCaps::LoadSkillCaps reads the content DB directly
		// (common/skill_caps.cpp:91), so this needs a zone/world restart and NOT a ./shared_memory run.
		// ⚠️⚠️ SCOPED DELIBERATELY TO SKILLS 25 AND 32. The wider "Bard has all 77" legacy is a separate
		// decision and is NOT swept up here -- stripping it wholesale would silently strip real Bard
		// abilities too.
		.check       = "SELECT class_id FROM skill_caps WHERE class_id = 8 AND skill_id IN (25, 32) AND cap > 0",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DELETE FROM skill_caps WHERE class_id = 8 AND skill_id IN (25, 32);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 12,
		.description = "2026_08_04_aotv4_tradeskill_aa_ladders",
		// Tradeskill achievements now auto-grant the mastery AAs, via the new `grant_aa` reward type.
		// Full commentary in custom/sql/aotv4_tradeskill_aa_ladders.sql -- the short version:
		//   * the per-skill achievements ALREADY EXIST (72 of them, keyed 4<skill><level>); this only
		//     attaches a grant_aa reward to the 50/100/150 rows of the ten skills that have a mastery.
		//   * Fishing (55) and Research (58) are excluded from BOTH ladders -- neither has a mastery AA.
		//   * skill 68 is Jewelcrafting and 69 is Pottery, verified against the achievement rows.
		// ⚠️ enabled=1 is mandatory: zone/aa.cpp loads only enabled AAs, so grant_aa fails without it.
		// ⚠️ AAs load at zone boot and are NOT shared memory -- a zone restart is enough.
		.check       = "SELECT id FROM aa_ability WHERE id IN (324,325,326,327,328,329,330) AND enabled = 0",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_ability SET enabled = 1, grant_only = 1, classes = 65535
 WHERE id IN (49, 103, 324, 325, 326, 327, 328, 329, 330, 575, 576);

DELETE FROM custom_achievement_rewards WHERE id BETWEEN 1000 AND 1199;

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1000 + (m.seq * 3) + t.rank_no - 1,
       400000 + (m.skill_id * 1000) + (t.rank_no * 50),
       'grant_aa', m.aa_id, t.rank_no, 100, '', 1, 1,
       CONCAT(m.aa_name, ' rank ', t.rank_no), '', 1, 20, UNIX_TIMESTAMP()
FROM (
  SELECT 0 seq, 56 skill_id, 103 aa_id, 'Poison Mastery'        aa_name UNION ALL
  SELECT 1, 57, 575, 'Tinkering Mastery'     UNION ALL
  SELECT 2, 59,  49, 'Alchemy Mastery'       UNION ALL
  SELECT 3, 60, 325, 'Baking Mastery'        UNION ALL
  SELECT 4, 61, 329, 'Tailoring Mastery'     UNION ALL
  SELECT 5, 63, 324, 'Blacksmithing Mastery' UNION ALL
  SELECT 6, 64, 327, 'Fletching Mastery'     UNION ALL
  SELECT 7, 65, 326, 'Brewing Mastery'       UNION ALL
  SELECT 8, 68, 576, 'Jewel Craft Mastery'   UNION ALL
  SELECT 9, 69, 328, 'Pottery Mastery'
) m
CROSS JOIN (SELECT 1 rank_no UNION ALL SELECT 2 UNION ALL SELECT 3) t
WHERE EXISTS (SELECT 1 FROM custom_achievements a
               WHERE a.id = 400000 + (m.skill_id * 1000) + (t.rank_no * 50));

DELETE FROM custom_achievement_objectives WHERE id BETWEEN 47000000 AND 47099999;
DELETE FROM custom_achievements           WHERE id BETWEEN 470000 AND 470999;

INSERT INTO custom_achievements
  (id, category_id, slug, name, description, points, hidden, repeatable, reward_title_set,
   reward_item_id, reward_currency_id, reward_currency_amount, enabled, sort_order, created_at)
SELECT 470000 + t.v, 600, CONCAT('master_artisan_', t.v), CONCAT('Master Artisan ', t.v),
       CONCAT('Reach ', t.v, ' in every tradeskill that has a mastery. Grants a rank of Salvage.'),
       25, 0, 0, 0, 0, 0, 0, 1, 900 + t.v, UNIX_TIMESTAMP()
FROM (SELECT 50 v UNION ALL SELECT 100 UNION ALL SELECT 150 UNION ALL
      SELECT 200 UNION ALL SELECT 250 UNION ALL SELECT 300) t;

INSERT INTO custom_achievement_objectives
  (id, achievement_id, objective_index, objective_type, target_type, target_id, target_name,
   required_count, zone_id, class_mask, optional)
SELECT 47000000 + (t.v * 100) + m.seq, 470000 + t.v, m.seq, 'skill', '', m.skill_id, m.skill_name,
       t.v, 0, 0, 0
FROM (
  SELECT 0 seq, 56 skill_id, 'Make Poison' skill_name UNION ALL
  SELECT 1, 57, 'Tinkering'     UNION ALL SELECT 2, 59, 'Alchemy'   UNION ALL
  SELECT 3, 60, 'Baking'        UNION ALL SELECT 4, 61, 'Tailoring' UNION ALL
  SELECT 5, 63, 'Blacksmithing' UNION ALL SELECT 6, 64, 'Fletching' UNION ALL
  SELECT 7, 65, 'Brewing'       UNION ALL SELECT 8, 68, 'Jewelcrafting' UNION ALL
  SELECT 9, 69, 'Pottery'
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
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 13,
		.description = "2026_08_04_aotv4_craft_sockets",
		// Every craftable wearable gets sockets by TIER: native 1, Hallowed 2, Mythic 3, all type 1 so
		// every delve augment (AugType 255 = slot types 1-8) fits every slot. Ornamentation slots share
		// the same six columns and mostly sit in position 2, so they are relocated to positions 4/5
		// rather than overwritten. Full commentary in custom/sql/aotv4_craft_sockets.sql.
		// ⚠️⚠️ `items` IS SHARED MEMORY. World applies this at boot, but the change is NOT visible until
		// the stack is taken down and ./shared_memory is re-run. Migrating alone is not enough.
		// ⚠️ A temp table cannot be reopened inside one statement, so the base set is joined at the
		// three tier offsets rather than grown by selecting from itself.
		.check       = "SELECT i.id FROM tradeskill_recipe_entries e JOIN items i ON i.id = e.item_id JOIN tradeskill_recipe r ON r.id = e.recipe_id WHERE e.successcount > 0 AND i.slots > 0 AND r.enabled = 1 AND i.augslot1type <> 1 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_craft_base;
CREATE TEMPORARY TABLE aotv4_craft_base (item_id INT NOT NULL PRIMARY KEY) ENGINE=MEMORY;
INSERT INTO aotv4_craft_base (item_id)
SELECT DISTINCT i.id FROM tradeskill_recipe_entries e
JOIN items i ON i.id = e.item_id JOIN tradeskill_recipe r ON r.id = e.recipe_id
WHERE e.successcount > 0 AND i.slots > 0 AND r.enabled = 1;

DROP TEMPORARY TABLE IF EXISTS aotv4_craft_orn;
CREATE TEMPORARY TABLE aotv4_craft_orn (
  item_id INT NOT NULL PRIMARY KEY, orn_type INT NOT NULL, orn_type2 INT NOT NULL DEFAULT 0
) ENGINE=MEMORY;

INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
  CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type WHEN i.augslot2type IN (20,21) THEN i.augslot2type ELSE i.augslot3type END,
  CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
       WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
       WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
  CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type WHEN i.augslot2type IN (20,21) THEN i.augslot2type ELSE i.augslot3type END,
  CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
       WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
       WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id + 300000
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

INSERT INTO aotv4_craft_orn (item_id, orn_type, orn_type2)
SELECT i.id,
  CASE WHEN i.augslot1type IN (20,21) THEN i.augslot1type WHEN i.augslot2type IN (20,21) THEN i.augslot2type ELSE i.augslot3type END,
  CASE WHEN i.augslot1type IN (20,21) AND i.augslot2type IN (20,21) THEN i.augslot2type
       WHEN i.augslot1type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type
       WHEN i.augslot2type IN (20,21) AND i.augslot3type IN (20,21) THEN i.augslot3type ELSE 0 END
FROM items i JOIN aotv4_craft_base b ON i.id = b.item_id + 600000
WHERE i.augslot1type IN (20,21) OR i.augslot2type IN (20,21) OR i.augslot3type IN (20,21)
ON DUPLICATE KEY UPDATE orn_type = VALUES(orn_type), orn_type2 = VALUES(orn_type2);

UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id
   SET i.augslot1type=1, i.augslot1visible=1, i.augslot2type=0, i.augslot2visible=0,
       i.augslot3type=0, i.augslot3visible=0;
UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id + 300000
   SET i.augslot1type=1, i.augslot1visible=1, i.augslot2type=1, i.augslot2visible=1,
       i.augslot3type=0, i.augslot3visible=0;
UPDATE items i JOIN aotv4_craft_base b ON i.id = b.item_id + 600000
   SET i.augslot1type=1, i.augslot1visible=1, i.augslot2type=1, i.augslot2visible=1,
       i.augslot3type=1, i.augslot3visible=1;

UPDATE items i JOIN aotv4_craft_orn o ON o.item_id = i.id
   SET i.augslot4type = o.orn_type, i.augslot4visible = 1,
       i.augslot5type    = CASE WHEN o.orn_type2 > 0 THEN o.orn_type2 ELSE i.augslot5type END,
       i.augslot5visible = CASE WHEN o.orn_type2 > 0 THEN 1 ELSE i.augslot5visible END;

DROP TEMPORARY TABLE IF EXISTS aotv4_craft_base;
DROP TEMPORARY TABLE IF EXISTS aotv4_craft_orn;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 14,
		.description = "2026_08_04_aotv4_tradeskill_tools",
		// Twelve worn tools (+20), twelve mask clickies casting twelve illusions (+30), and a recipe
		// per tool so each is crafted in its own tradeskill. Full commentary in
		// custom/sql/aotv4_tradeskill_tools.sql.
		// ⚠️⚠️ THE BONUSES ARE PAID IN C++ BY ID (AoTv4TradeskillSkill, zone/tradeskills.cpp). These
		// rows are INERT markers -- neither a flat item skill bonus nor any spell skill bonus is
		// expressible natively. The ID ORDER IS LOAD BEARING and must match AOTV4_TS_SKILLS.
		// ⚠️⚠️ 44400, NOT 436xx -- 43576-44327 is the spell-rank band and would have been overwritten.
		// ⚠️ items AND spells_new are shared memory: ./shared_memory after this applies.
		.check       = "SELECT id FROM items WHERE id = 147930",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM items      WHERE id BETWEEN 147930 AND 147953;
DELETE FROM spells_new WHERE id BETWEEN 44400 AND 44411;

DROP TEMPORARY TABLE IF EXISTS sp;
CREATE TEMPORARY TABLE sp AS SELECT * FROM spells_new WHERE id = 582;
UPDATE sp SET id=44400, name='Guise of the Brineface',     effect_base_value1=9;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44401, name='Guise of the Weeping Fang',  effect_base_value1=6;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44402, name='Guise of the Cogwright',     effect_base_value1=12; INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44403, name='Guise of the Inkbound',      effect_base_value1=3;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44404, name='Guise of the Quicksilver',   effect_base_value1=3;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44405, name='Guise of the Hearthfed',     effect_base_value1=11; INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44406, name='Guise of the Threadbare',    effect_base_value1=4;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44407, name='Guise of the Forge-Wight',   effect_base_value1=8;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44408, name='Guise of the Straightgrain', effect_base_value1=4;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44409, name='Guise of the Sourmash',      effect_base_value1=8;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44410, name='Guise of the Facetwright',   effect_base_value1=5;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44411, name='Guise of the Kilnborn',      effect_base_value1=10; INSERT INTO spells_new SELECT * FROM sp;
DROP TEMPORARY TABLE sp;

UPDATE spells_new
   SET classes1=1,classes2=1,classes3=1,classes4=1,classes5=1,classes6=1,classes7=1,classes8=1,
       classes9=1,classes10=1,classes11=1,classes12=1,classes13=1,classes14=1,classes15=1,classes16=1,
       targettype=6, goodEffect=1
 WHERE id BETWEEN 44400 AND 44411;

DROP TEMPORARY TABLE IF EXISTS it;
CREATE TEMPORARY TABLE it AS SELECT * FROM items WHERE id = 1001;
UPDATE it SET nodrop=0, norent=1, races=65535, classes=65535, reclevel=0, reqlevel=0;
UPDATE it SET id=147930, Name="Angler's Brim";           INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147931, Name="Venomer's Wrap";          INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147932, Name="Tinker's Goggles";        INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147933, Name="Scrivener's Circlet";     INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147934, Name="Alchemist's Hood";        INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147935, Name="Baker's Toque";           INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147936, Name="Tailor's Pinned Cap";     INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147937, Name="Smith's Soot Hood";       INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147938, Name="Fletcher's Band";         INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147939, Name="Brewer's Cowl";           INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147940, Name="Jeweler's Loupe Harness"; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147941, Name="Potter's Clay-Caked Cap"; INSERT INTO items SELECT * FROM it;
UPDATE it SET slots=0, clicktype=1, clicklevel=1, clicklevel2=1, casttime=0, casttime_=0;
UPDATE it SET id=147942, Name="Mask of the Brineface",     clickeffect=44400; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147943, Name="Mask of the Weeping Fang",  clickeffect=44401; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147944, Name="Mask of the Cogwright",     clickeffect=44402; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147945, Name="Mask of the Inkbound",      clickeffect=44403; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147946, Name="Mask of the Quicksilver",   clickeffect=44404; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147947, Name="Mask of the Hearthfed",     clickeffect=44405; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147948, Name="Mask of the Threadbare",    clickeffect=44406; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147949, Name="Mask of the Forge-Wight",   clickeffect=44407; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147950, Name="Mask of the Straightgrain", clickeffect=44408; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147951, Name="Mask of the Sourmash",      clickeffect=44409; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147952, Name="Mask of the Facetwright",   clickeffect=44410; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147953, Name="Mask of the Kilnborn",      clickeffect=44411; INSERT INTO items SELECT * FROM it;
DROP TEMPORARY TABLE it;

DELETE FROM tradeskill_recipe_entries WHERE recipe_id BETWEEN 470100 AND 470111;
DELETE FROM tradeskill_recipe         WHERE id        BETWEEN 470100 AND 470111;
DELETE FROM custom_achievement_rewards WHERE id BETWEEN 1200 AND 1299;

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1200 + x.idx, 400000 + x.skill*1000 + 100, 'item', 147930 + x.idx, 1, 100, '', 1, 1,
       CONCAT(x.tool, ' (+20 ', x.sname, ')'), '', 1, 30, UNIX_TIMESTAMP()
FROM (
  SELECT 0 idx,55 skill,'Fishing' sname,"Angler's Brim" tool UNION ALL
  SELECT 1,56,'Make Poison',"Venomer's Wrap"      UNION ALL SELECT 2,57,'Tinkering',"Tinker's Goggles"    UNION ALL
  SELECT 3,58,'Research',"Scrivener's Circlet"    UNION ALL SELECT 4,59,'Alchemy',"Alchemist's Hood"      UNION ALL
  SELECT 5,60,'Baking',"Baker's Toque"            UNION ALL SELECT 6,61,'Tailoring',"Tailor's Pinned Cap" UNION ALL
  SELECT 7,63,'Blacksmithing',"Smith's Soot Hood" UNION ALL SELECT 8,64,'Fletching',"Fletcher's Band"     UNION ALL
  SELECT 9,65,'Brewing',"Brewer's Cowl"           UNION ALL SELECT 10,68,'Jewelcrafting',"Jeweler's Loupe Harness" UNION ALL
  SELECT 11,69,'Pottery',"Potter's Clay-Caked Cap"
) x
WHERE EXISTS (SELECT 1 FROM custom_achievements a WHERE a.id = 400000 + x.skill*1000 + 100);

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1250 + x.idx, 400000 + x.skill*1000 + 200, 'item', 147942 + x.idx, 1, 100, '', 1, 1,
       CONCAT(x.mask, ' (+30 ', x.sname, ')'), '', 1, 30, UNIX_TIMESTAMP()
FROM (
  SELECT 0 idx,55 skill,'Fishing' sname,"Mask of the Brineface" mask UNION ALL
  SELECT 1,56,'Make Poison',"Mask of the Weeping Fang"  UNION ALL SELECT 2,57,'Tinkering',"Mask of the Cogwright" UNION ALL
  SELECT 3,58,'Research',"Mask of the Inkbound"         UNION ALL SELECT 4,59,'Alchemy',"Mask of the Quicksilver" UNION ALL
  SELECT 5,60,'Baking',"Mask of the Hearthfed"          UNION ALL SELECT 6,61,'Tailoring',"Mask of the Threadbare" UNION ALL
  SELECT 7,63,'Blacksmithing',"Mask of the Forge-Wight" UNION ALL SELECT 8,64,'Fletching',"Mask of the Straightgrain" UNION ALL
  SELECT 9,65,'Brewing',"Mask of the Sourmash"          UNION ALL SELECT 10,68,'Jewelcrafting',"Mask of the Facetwright" UNION ALL
  SELECT 11,69,'Pottery',"Mask of the Kilnborn"
) x
WHERE EXISTS (SELECT 1 FROM custom_achievements a WHERE a.id = 400000 + x.skill*1000 + 200);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 15,
		.description = "2026_08_04_aotv4_aa_flat_hp",
		// Reported as "Bulwark Within increased my mana and endurance, but didn't increase HP".
		// It was not broken -- it is STOCK, and the level cap is what breaks it. The AA (native 477
		// "Valor of the Keepers", renamed by aotv4_aa_rename.sql) mixes effect KINDS:
		//     SPA 214 MaxHPChange  -> PercentMaxHPChange, and CalcMaxHP divides it by 10000
		//     SPA  97 ManaPool     -> FLAT
		//     SPA 190 EndurancePool-> FLAT
		// base1 300 is therefore 3 percent. On live EQ at 10,000 hp that is ~300 and sits level with
		// the flat +200 mana; at our level 30 cap and ~1,500 hp it is ~45, which reads as nothing.
		//
		// ⚠️⚠️ THIS IS A CLASS OF PROBLEM, NOT ONE AA. Any percentage-based AA effect is silently
		// devalued by a low level cap while flat ones keep full value. Three ENABLED AAs used SPA 214
		// (120 Hardened Frame 2 percent, 143 Planeforged 1.5, 477 Bulwark Within 3); 17 rows use it in
		// total, so the 14 on disabled AAs will need the same treatment if they are ever switched on.
		//
		// SPA 69 TotalHP is the FLAT counterpart (bonuses.cpp:786 -> FlatMaxHPChange), so swapping the
		// effect id while keeping base1 turns 2/1.5/3 percent into a flat +200/+150/+300 -- proportional
		// to what they were, and finally consistent with the flat mana and endurance beside them.
		// ⚠️ Scoped to those three rank ids on purpose: converting all 17 would silently rebalance AAs
		// nobody can currently train.
		.check       = "SELECT rank_id FROM aa_rank_effects WHERE effect_id = 214 AND rank_id IN (279, 423, 1367)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_rank_effects SET effect_id = 69 WHERE effect_id = 214 AND rank_id IN (279, 423, 1367);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 16,
		.description = "2026_08_04_aotv4_delve_exp_normalise",
		// Reported from play: "blues in delves were giving 4-5% exp, while yellows in LOIO were giving
		// 2% ... that's why I was leveling so fast in delves and kept out leveling it".
		// Nothing in the delve did this -- it is inherited STOCK data. The six delve zones are Dragons
		// of Norrath (expansion 9), which ships the highest zone_exp_multiplier values in the game
		// (2.90-3.10) because on live it is endgame content; Lake of Ill Omen is Kunark at 0.80. That
		// is a ~3.7x gap before con colour is considered.
		// ⚠️ zone_exp_multiplier multiplies NORMAL xp (zone/exp.cpp:130), AA xp (:294) AND group xp
		// (:443) -- it is not just a kill-xp knob.
		//
		// Normalised to 1.00: the delve's creatures are already scaled to the player, so a delve kill
		// should be worth what an equivalent open-world kill is worth. Its reward comes from the score
		// sheet, the sigil, augments and coin -- not from raw experience.
		//
		// ⚠️⚠️ EVERY VERSION ROW, NOT JUST VERSION 0. Zone config is loaded per (zone, version) and the
		// delve NEVER uses version 0 (see the Delve section) -- the mission versions are what players
		// actually stand in, and they carried the HIGHER multiplier (thenest v0 is 3.05, its other 15
		// versions 3.10). Updating version 0 alone would have changed nothing a player ever sees.
		// 67 rows across the six zones.
		// ⚠️ Safe for open-world play: DoN is expansion 9 and the era system caps at OoW, so these
		// zones are not reachable outside a delve.
		.check       = "SELECT short_name FROM zone WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest') AND zone_exp_multiplier <> 1.00",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE zone SET zone_exp_multiplier = 1.00
 WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 17,
		.description = "2026_08_05_aotv4_bard_song_durations",
		// Bard songs behave like real buffs now instead of needing constant re-singing.
		// 168 of the 216 beneficial songs ran on duration formula 5 = TWO TICKS (12 seconds), because
		// on live a Bard twists them continuously. Converted to formula 11 (30 * (level + 3) ticks):
		//     level 1 -> 12 min      level 10 -> 39 min      level 30 -> 99 min
		// Formula 11 rather than 3 (30 * level) deliberately: 3 collapses to THREE MINUTES at level 1,
		// and the roguelite death puts everybody back there constantly.
		//
		// ⚠️⚠️ BOTH COLUMNS MUST MOVE. CalcBuffDuration_formula ends with
		//     if (duration && duration < temp) temp = duration;      (zone/spells.cpp:3116)
		// so `buffduration` CAPS whatever the formula produces. The formula-5 songs carry buffduration
		// 0-10, so changing the formula alone would have capped most of them at ~1 minute while the
		// handful with buffduration 0 got the full 99 -- a half-working, inconsistent result that
		// would have looked like the change "sort of" worked. buffduration = 0 removes the cap.
		//
		// ⚠️ PULSING IS UNAFFECTED: IsPulsingBardSong keys off buffduration == 0xFFFF (not the
		// formula, and not 0), so songs still re-apply to group members in range every 6 seconds.
		//
		// ⚠️⚠️ SIX SONGS ARE DELIBERATELY LEFT SHORT -- 4 carry SPA 40 DivineAura (Kazumi's Note of
		// Preservation, Fermata of Preservation x2, Enervating Sustain) and 2 carry a true HoT SPA
		// (100/101/319). A 99 minute invulnerability is exactly the failure this exclusion exists for.
		// 📌 The bard REGEN songs are included on purpose. Hymn of Restoration is SPA 0 base **1** --
		// one hit point per tick -- and Cantata of Soothing is 4 hp + 5 mana per tick. Those are
		// regens, not heals, and the SPA-0-positive test alone cannot tell the two apart: it is
		// MAGNITUDE that distinguishes them, not mechanism.
		// ⚠️ The 16 spell-rank rows (Rk. II-V, section 29's band) are included and MUST be -- otherwise
		// ranking a song up would SHORTEN it from 99 minutes back to 12 seconds.
		// ⚠️ spells_new is SHARED MEMORY: ./shared_memory after this applies.
		.check       = "SELECT id FROM spells_new WHERE skill IN (12,41,49,54,70) AND goodEffect <> 0 AND id < 45000 AND buffdurationformula = 5",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new s SET s.buffdurationformula = 11, s.buffduration = 0
 WHERE s.skill IN (12,41,49,54,70) AND s.goodEffect <> 0 AND s.id < 45000
   AND s.buffdurationformula NOT IN (0,50,51)
   AND NOT (s.effectid1 IN (40,100,101,319,150,232) OR s.effectid2 IN (40,100,101,319,150,232)
         OR s.effectid3 IN (40,100,101,319,150,232) OR s.effectid4 IN (40,100,101,319,150,232)
         OR s.effectid5 IN (40,100,101,319,150,232) OR s.effectid6 IN (40,100,101,319,150,232));
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 18,
		.description = "2026_08_05_aotv4_disable_npc_enrage",
		// Turns the NPC enrage mechanic off. 404 npc_types carry the Enrage special ability; while
		// enraged a mob ripostes every frontal melee attack (zone/attack.cpp:492), which is the
		// "stop attacking and turn away" dance.
		//
		// ⚠️⚠️ THE RULE NAME READS BACKWARDS: setting NPC:LiveLikeEnrage to TRUE is what DISABLES it.
		// Mob::StartEnrage (zone/mob_ai.cpp) returns early when the rule is set unless the mob is a
		// PLAYER-controlled pet or swarm pet, so "live like" means "only player pets enrage, as on
		// live". Reading it as "enable enrage" and setting it false does the exact opposite.
		//
		// ⚠️ Player and bot PETS still enrage, deliberately -- that is a benefit to the player, not a
		// mechanic being fought, and it is the one case the rule preserves.
		// ⚠️ Done as a RULE rather than by stripping the ability off 404 npc_types rows: it is one
		// reversible row instead of a destructive edit that would lose which mobs were meant to have
		// it, and NPC:StartEnrageValue (9 percent) stays meaningful if it is ever turned back on.
		// ⚠️ Rules are read at ZONE BOOT -- a zone restart (or #reloadrules) is needed, not just a
		// world restart.
		.check       = "SELECT rule_name FROM rule_values WHERE rule_name = 'NPC:LiveLikeEnrage' AND rule_value <> 'true'",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE rule_values SET rule_value = 'true' WHERE rule_name = 'NPC:LiveLikeEnrage';
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 19,
		.description = "2026_08_05_aotv4_long_duration_buffs",
		// Beneficial buffs behave like real buffs instead of needing constant re-application.
		// Formula 11 = 30 * (level + 3) ticks: 12 min at level 1, 39 at 10, 99 at the level cap.
		// This is the same treatment v17 gave bard songs, widened to the rest of the buff space.
		//
		// ⚠️⚠️ BOTH COLUMNS MOVE. CalcBuffDuration_formula ends with
		//     if (duration && duration < temp) temp = duration;      (zone/spells.cpp:3116)
		// so `buffduration` CAPS the formula. Changing the formula alone caps most spells at their old
		// short duration while a handful with buffduration 0 get the full value -- a half-applied
		// result that looks like the change "sort of" worked. buffduration = 0 removes the cap.
		//
		// EXCLUSIONS, every one of them load bearing:
		// ⚠️⚠️ NOT PLAYER-CASTABLE (the big one, ~9,287 spells). NPC self-buffs, item clicks and procs.
		//   Making those permanent means every mob keeps its buffs forever and proc buffs never fall
		//   off. The `LEAST(classes1..16) BETWEEN 1 AND 100` test is what confines this to spells a
		//   player can actually cast.
		// ⚠️ DISCIPLINES -- mana = 0 AND an endurance cost, mirroring IsDiscipline (common/spdat.cpp)
		//   exactly as the reward pool's blacklist does. A permanent Trueshot is not a buff.
		// ⚠️ AA-GRANTED -- anything in aa_ranks.spell.
		// ⚠️ INVULNERABILITY / TRUE HoT / DEATH SAVES -- SPA 40, 100, 101, 319, 150, 232.
		// ⚠️ THE AoTv4 CUSTOM BAND 43000-44999 -- Shield Wall buffs, the Thirst line and the class
		//   auras have CODE-DRIVEN lifecycles (explicit fade calls, numhits charges, aura membership).
		//
		// ⚠️⚠️ SPA 0 IS KEPT UP TO A BASE OF 30, AND THAT IS DELIBERATE. SPA 0 on a duration spell
		// repeats every tick, so it reads as a heal-over-time -- but MAGNITUDE is what separates a
		// regen from a heal, not mechanism. Hymn of Restoration is base **1** (one hit point a tick)
		// and Cantata of Soothing is 4; those are regens and they belong in. Above 30 (up to 500+ a
		// tick) it is a heal engine and is excluded. Filtering on SPA 100 alone misses every one of
		// these, because the classic regens do not use it.
		.check       = "SELECT id FROM spells_new s WHERE s.goodEffect <> 0 AND s.buffdurationformula NOT IN (0,11,50,51) AND s.id < 45000 AND s.id NOT BETWEEN 43000 AND 44999 AND LEAST(s.classes1,s.classes2,s.classes3,s.classes4,s.classes5,s.classes6,s.classes7,s.classes8,s.classes9,s.classes10,s.classes11,s.classes12,s.classes13,s.classes14,s.classes15,s.classes16) BETWEEN 1 AND 100 AND NOT (s.mana = 0 AND (s.EndurCost > 0 OR s.EndurUpkeep > 0)) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new s SET s.buffdurationformula = 11, s.buffduration = 0
WHERE s.goodEffect <> 0 AND s.buffdurationformula NOT IN (0,11,50,51) AND s.id < 45000
  AND s.id NOT BETWEEN 43000 AND 44999
  AND LEAST(s.classes1,s.classes2,s.classes3,s.classes4,s.classes5,s.classes6,s.classes7,s.classes8,
            s.classes9,s.classes10,s.classes11,s.classes12,s.classes13,s.classes14,s.classes15,s.classes16) BETWEEN 1 AND 100
  AND NOT (s.mana = 0 AND (s.EndurCost > 0 OR s.EndurUpkeep > 0))
  AND NOT EXISTS (SELECT 1 FROM aa_ranks r WHERE r.spell = s.id)
  AND NOT (s.effectid1 IN (40,100,101,319,150,232) OR s.effectid2 IN (40,100,101,319,150,232)
        OR s.effectid3 IN (40,100,101,319,150,232) OR s.effectid4 IN (40,100,101,319,150,232)
        OR s.effectid5 IN (40,100,101,319,150,232) OR s.effectid6 IN (40,100,101,319,150,232)
        OR s.effectid7 IN (40,100,101,319,150,232) OR s.effectid8 IN (40,100,101,319,150,232))
  AND NOT ((s.effectid1=0 AND s.effect_base_value1>30) OR (s.effectid2=0 AND s.effect_base_value2>30)
        OR (s.effectid3=0 AND s.effect_base_value3>30) OR (s.effectid4=0 AND s.effect_base_value4>30)
        OR (s.effectid5=0 AND s.effect_base_value5>30) OR (s.effectid6=0 AND s.effect_base_value6>30));
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 20,
		.description = "2026_08_05_aotv4_permanent_buffs",
		// Beneficial buffs are PERMANENT, superseding v17/v19's long-but-finite durations.
		// v17 (songs) and v19 (everything else) moved the buff space to formula 11 = 30 * (level + 3)
		// ticks, i.e. 12 min at level 1 and 99 at the level cap. The intent was that a buff should not
		// need re-applying at all, and 99 minutes is not that -- it is just a longer timer.
		//
		// ⚠️⚠️ PERMANENT IS A FORMULA, NOT A BIG NUMBER. CalcBuffDuration_formula returns **-1** for
		// formula 50 (zone/spells.cpp:3105) and -4 for 51 (permanent until you leave an aura). Do NOT
		// express this as a huge tick count: the buff would still tick down, and every duration display
		// would show a countdown measured in days. 1,103 stock spells already ship formula 50, so this
		// is the game's own mechanism, not a hack.
		//
		// ⚠️⚠️ FORMULA 50 RETURNS BEFORE THE `buffduration` CAP, so unlike v17/v19 this migration does
		// NOT need to touch that column -- `if (duration && duration < temp)` (spells.cpp:3116) is
		// never reached. It is deliberately left alone to keep the change surface minimal; a row
		// reading formula 50 with a stale buffduration of 720 is correct and simply ignored.
		//
		// ⚠️ Targeted on `buffdurationformula = 11` rather than re-deriving v19's net: that sweeps in
		// the **302 spells that were ALREADY formula 11** before either migration, so near-identical
		// buffs cannot end up with one permanent and one on a 99 minute timer. ~2,040 spells total.
		//
		// EVERY v19 EXCLUSION IS CARRIED OVER VERBATIM, and they matter MORE here, not less --
		// "long" becoming "forever" raises the cost of getting any of them wrong:
		// ⚠️⚠️ INVULNERABILITY (SPA 40), TRUE HoTs (100/101/319) AND DEATH SAVES (150/232). A permanent
		//   invulnerability is not a buff, it is an unkillable character; a permanent true HoT is
		//   infinite healing. This is the single most important line in the migration.
		// ⚠️⚠️ SPA 0 ABOVE BASE 30 -- the heal engines. SPA 0 on a duration spell repeats every tick,
		//   so magnitude is what separates a regen from a heal. Hymn of Restoration (base 1) and
		//   Cantata of Soothing (base 4) are regens and belong in; 500 a tick, forever, does not.
		// ⚠️ NOT PLAYER-CASTABLE -- the LEAST(classes1..16) BETWEEN 1 AND 100 test. Without it every
		//   NPC self-buff, proc and item click becomes permanent and monsters keep their buffs forever.
		// ⚠️ DISCIPLINES (mana = 0 AND an endurance cost, mirroring IsDiscipline in common/spdat.cpp).
		// ⚠️ AA-GRANTED (anything in aa_ranks.spell).
		// ⚠️ THE AoTv4 BAND 43000-44999 -- Shield Wall buffs, the Thirst line and the class auras have
		//   code-driven lifecycles (explicit fade calls, numhits charges, aura membership).
		//
		// ⚠️⚠️ CHARGED BUFFS ARE EXCLUDED -- `numhits > 0`. A charge buff is spent by USE, not by time
		// (numhits + numhitstype, decremented in CheckNumHitsRemaining), so making its duration
		// permanent means an unspent one never leaves: you would hold a charged proc or absorb buff
		// forever and it would stop being a consumable at all. 208 formula-11 buffs carry charges,
		// spread across 9 different numhitstype values. They keep formula 11, so they still expire on
		// time OR on charges, whichever comes first.
		// ⚠️ This is NOT the same test as the SPA exclusions above: a charged buff can be entirely
		// benign in its effects and still must not be permanent, because the CHARGE is the cost.
		//
		// ⚠️ BARD SONGS ARE INCLUDED and pulsing still works: IsPulsingBardSong (common/spdat.cpp:2404)
		// never reads the formula -- it returns false only on buff_duration == 0xFFFF, a recast time,
		// a mana cost, or a pet/familiar effect. Zero spells in this DB use the 0xFFFF marker.
		// ⚠️ spells_new is SHARED MEMORY: world down -> ./shared_memory -> restart.
		.check       = "SELECT id FROM spells_new s WHERE s.buffdurationformula = 11 AND s.goodEffect <> 0 AND s.id < 45000 AND s.numhits = 0 AND s.id NOT BETWEEN 43000 AND 44999 AND LEAST(s.classes1,s.classes2,s.classes3,s.classes4,s.classes5,s.classes6,s.classes7,s.classes8,s.classes9,s.classes10,s.classes11,s.classes12,s.classes13,s.classes14,s.classes15,s.classes16) BETWEEN 1 AND 100 AND NOT (s.mana = 0 AND (s.EndurCost > 0 OR s.EndurUpkeep > 0)) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new s SET s.buffdurationformula = 50
WHERE s.buffdurationformula = 11 AND s.goodEffect <> 0 AND s.id < 45000
  AND s.numhits = 0
  AND s.id NOT BETWEEN 43000 AND 44999
  AND LEAST(s.classes1,s.classes2,s.classes3,s.classes4,s.classes5,s.classes6,s.classes7,s.classes8,
            s.classes9,s.classes10,s.classes11,s.classes12,s.classes13,s.classes14,s.classes15,s.classes16) BETWEEN 1 AND 100
  AND NOT (s.mana = 0 AND (s.EndurCost > 0 OR s.EndurUpkeep > 0))
  AND NOT EXISTS (SELECT 1 FROM aa_ranks r WHERE r.spell = s.id)
  AND NOT (s.effectid1 IN (40,100,101,319,150,232) OR s.effectid2 IN (40,100,101,319,150,232)
        OR s.effectid3 IN (40,100,101,319,150,232) OR s.effectid4 IN (40,100,101,319,150,232)
        OR s.effectid5 IN (40,100,101,319,150,232) OR s.effectid6 IN (40,100,101,319,150,232)
        OR s.effectid7 IN (40,100,101,319,150,232) OR s.effectid8 IN (40,100,101,319,150,232))
  AND NOT ((s.effectid1=0 AND s.effect_base_value1>30) OR (s.effectid2=0 AND s.effect_base_value2>30)
        OR (s.effectid3=0 AND s.effect_base_value3>30) OR (s.effectid4=0 AND s.effect_base_value4>30)
        OR (s.effectid5=0 AND s.effect_base_value5>30) OR (s.effectid6=0 AND s.effect_base_value6>30));
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 21,
		.description = "2026_08_05_aotv4_restore_native_buff_durations",
		// ⚠️⚠️ REVERTS v17, v19 AND v20. Buff duration is no longer rewritten in the DATA at all --
		// it is extended in CODE (Mob::CalcBuffDuration, AoT:SelfBuffDurationTicks). This restores the
		// NATIVE buffdurationformula/buffduration for every spell those three could have touched.
		//
		// ⚠️⚠️ WHY THE DATA APPROACH HAD TO GO -- FOUR FAILURES, ONE ROOT CAUSE:
		//   1. PERMANENT IS -1. CalcBuffDuration_formula returns -1 for formula 50, and that -1 flows
		//      into CalcSpellEffectValue_formula, where the ramping-effect branches compute
		//          ticdif = CalcBuffDuration_formula(...) - max(ticsremaining - 1, 0)
		//      = -1 - 0 = -1. So the EFFECT MAGNITUDE broke while the buff ICON happily persisted --
		//      reported as "the buff stays but the stats don't", on zoning and on death.
		//   2. BARD SONGS could not be stopped: a song that never expires keeps the pulse alive, so
		//      the singer sings forever.
		//   3. "A buff cast on someone else should last its NATIVE duration" is IMPOSSIBLE once the
		//      native duration has been overwritten in the row. It has to survive in the data.
		//   4. Nothing needs a -1 anywhere: ~3 days is a finite timer the client displays sanely.
		//
		// ⚠️⚠️ IT RESTORES ALL 3,942 SNAPSHOT ROWS, NOT JUST THE ONES THAT LOOK CHANGED -- AND THAT IS
		// LOAD BEARING BECAUSE OF MIGRATION ORDER. On a database still at v19, world applies **v20
		// first** (formula 11 -> 50) and only then v21. An earlier version of this migration restored
		// only the 1,760 rows whose values differed from the snapshot at authoring time, which left
		// **138 spells STRANDED**: natively formula 11, converted to 50 by v20, and never restored --
		// silently permanent, with the -1 bug intact. Restoring every snapshot row is a no-op for rows
		// already correct and immune to the ordering.
		// 📌 Formula 11 (30*(level+3)) and formula 50 (permanent) are BOTH legitimate stock formulas,
		// so "sitting at 11 or 50" never meant "we changed it" -- which is exactly the trap that
		// produced the 138. Always diff against the snapshot, per id.
		// 📌 Values recovered from aotv4_client_install/spells_us.txt, a pre-change snapshot taken
		// 2026-08-03 (the migrations landed 08-05). Section 35 records that file as the only surviving
		// copy of the original durations; fields 17 and 18 are buffdurationformula and buffduration.
		//
		// ⚠️ Sentinel: spell 7 (Hymn of Restoration) is natively formula 5 / duration 3 and was pushed
		// to 11 / 0. Idempotent -- once restored the condition is false.
		.check       = "SELECT id FROM spells_new WHERE id = 7 AND (buffdurationformula <> 5 OR buffduration <> 3)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_native_dur;
CREATE TEMPORARY TABLE aotv4_native_dur (id INT PRIMARY KEY, f INT, d INT);
INSERT INTO aotv4_native_dur (id,f,d) VALUES
(7,5,3),(10,3,270),(11,3,270),(18,3,450),(19,3,630),(20,3,720),(21,7,30),(26,11,270),(33,3,400),(34,3,240),(39,9,110),(40,11,270),(42,3,200),(43,8,4),(44,8,4),(45,8,7),(46,3,360),(47,8,7),(51,5,3),(60,3,360),(61,3,360),(62,3,360),(63,3,360),(64,3,360),(65,3,450),(66,3,540),(67,3,720),(72,3,360),(79,11,270),(80,11,270),(81,3,550),(82,3,720),(84,10,205),(86,3,270),(89,3,360),(90,3,270),(106,11,600),(107,11,600),(108,3,270),(109,3,360),(129,10,150),(137,9,140),(138,9,140),(139,11,600),(140,11,600),(144,10,205),(145,10,205),(146,3,360),(147,3,360),(148,3,450),(149,3,450),(150,3,450),(151,3,450),(152,3,540),(153,3,540),(154,3,630),(155,3,540),(156,3,630),(157,3,630),(158,3,630),(159,3,630),(160,3,540),(161,3,540),(167,3,720),(168,3,720),(169,3,360),(170,9,110),(171,10,160),(172,10,160),(174,3,270),(175,3,400),(176,7,50),(191,10,150),(194,7,4),(202,11,270),(208,9,20),(210,8,4),(215,3,30),(219,3,270),(220,1,8),(224,3,270),(225,3,270),(226,3,270),(227,3,270),(228,3,270),(235,3,270),(236,3,360),(240,9,20),(243,3,360),(244,3,450),(246,3,270),(247,3,200),(250,2,20),(254,3,120),(255,3,360),(256,10,150),(258,3,360),(261,10,205),(263,3,270),(266,11,270),(267,11,270),(268,3,270),(269,11,360),(271,6,3),(273,10,150),(274,3,270),(276,3,270),(278,3,360),(279,3,360),(280,6,3),(283,3,360),(284,3,360),(287,3,360),(288,11,270),(293,3,270),(308,10,205),(309,3,360),(312,3,540),(314,3,630),(323,7,5),(326,10,205),(327,11,600),(332,10,150),(333,3,450),(337,10,205),(346,3,270),(347,8,20),(349,3,450),(352,3,270),(356,10,150),(358,3,60),(359,8,75),(361,3,270),(364,9,150),(368,3,360),(371,3,270),(378,2,50),(381,3,270),(384,3,1950),(387,3,540),(389,3,630),(393,3,720),(394,3,900),(407,3,1950),(411,10,150),(412,10,150),(421,3,360),(422,3,540),(423,3,720),(424,3,450),(425,3,720),(426,3,1100),(427,3,1440),(428,3,360),(429,3,360),(430,3,540),(431,3,450),(432,10,150),(448,8,30),(449,11,600),(457,3,720),(478,3,270),(479,10,150),(481,3,360),(482,3,540),(483,3,720),(484,3,900),(485,3,270),(486,3,360),(487,3,450),(488,3,540),(489,3,360),(500,9,140),(501,8,25),(504,3,30),(513,8,7),(515,3,1950),(516,3,1950),(517,3,1950),(518,3,1950),(519,3,1950),(522,3,200),(529,1,33),(539,3,360),(578,1,33),(579,1,33),(580,9,140),(581,3,360),(582,3,360),(583,3,360),(584,3,360),(585,3,360),(586,3,360),(587,3,360),(588,3,360),(589,3,360),(590,3,360),(591,3,360),(592,3,360),(593,3,360),(594,3,360),(595,3,360),(596,3,360),(597,3,360),(598,3,360),(599,3,360),(600,3,360),(601,3,360),(640,1,10),(641,8,75),(642,9,140),(643,3,600),(644,3,600),(646,3,360),(647,3,720),(648,7,65),(649,3,360),(650,3,270),(651,3,360),(652,3,450),(653,3,720),(654,3,900),(655,3,270),(661,11,600),(679,3,270),(680,10,150),(693,3,600),(697,11,270),(698,3,120),(700,5,3),(701,5,3),(702,5,3),(708,5,3),(709,5,3),(710,5,3),(711,5,3),(712,5,3),(713,5,3),(714,5,3),(715,5,3),(716,5,3),(717,5,3),(718,7,3),(719,7,3),(721,7,4),(722,7,3),(723,5,0),(728,7,5),(729,7,4),(734,5,3),(735,5,3),(740,5,3),(745,7,2),(747,5,3),(748,5,3),(749,5,3),(772,11,450),(775,50,0),(776,50,0),(777,50,0),(778,50,0),(780,50,0),(781,50,0),(782,50,0),(783,50,0),(784,50,0),(785,50,0),(798,11,2040),(800,50,0),(801,50,0),(802,50,0),(803,50,0),(814,11,2040),(815,11,2040),(847,50,0),(857,11,5000),(871,11,30),(873,11,10),(876,11,50),(879,11,30),(880,11,30),(881,11,50),(882,11,30),(883,11,20),(884,11,60),(885,11,18),(925,50,0),(927,50,0),(928,50,0),(930,50,0),(932,50,0),(937,11,50),(939,11,2040),(953,50,0),(963,11,10),(1007,11,2040),(1008,11,150),(1014,11,150),(1015,50,0),(1104,50,0),(1105,50,0),(1162,11,18),(1196,5,3),(1200,50,0),(1201,50,0),(1202,50,0),(1203,50,0),(1204,50,0),(1205,50,0),(1206,50,0),(1207,50,0),(1208,50,0),(1209,50,0),(1213,11,2040),(1214,11,2040),(1220,50,0),(1225,3,100),(1226,3,100),(1227,3,100),(1228,3,100),(1240,50,0),(1241,50,0),(1242,50,0),(1243,50,0),(1253,11,300),(1254,11,300),(1255,11,300),(1256,11,300),(1257,11,300),(1258,11,300),(1259,11,300),(1260,11,300),(1261,11,300),(1262,11,300),(1263,11,300),(1271,5,3),(1284,3,35),(1286,3,120),(1287,5,0),(1288,3,500),(1289,11,600),(1376,3,200),(1377,7,65),(1391,3,720),(1397,3,600),(1406,3,100),(1408,3,600),(1409,3,750),(1410,3,1000),(1411,3,100),(1414,11,600),(1416,3,600),(1419,10,50),(1420,3,50),(1428,3,460),(1430,11,600),(1431,3,1440),(1432,3,1000),(1435,3,100),(1442,3,1000),(1445,3,1440),(1447,3,1500),(1448,5,3),(1449,5,3),(1450,5,3),(1452,5,3),(1453,3,150),(1456,3,500),(1459,3,200),(1461,10,100),(1462,3,600),(1463,10,100),(1464,3,750),(1472,11,600),(1474,7,0),(1510,1,4),(1533,3,720),(1534,8,4),(1535,3,630),(1536,3,720),(1537,3,810),(1538,3,720),(1539,3,1440),(1540,3,1440),(1541,8,7),(1547,11,600),(1551,3,360),(1552,3,360),(1554,3,720),(1557,3,720),(1558,3,1950),(1559,3,810),(1560,10,150),(1561,10,150),(1562,3,1440),(1563,3,1950),(1564,3,360),(1565,3,1950),(1568,10,205),(1569,10,205),(1570,3,360),(1571,3,360),(1575,11,630),(1579,3,630),(1580,3,630),(1581,3,630),(1582,3,630),(1583,3,630),(1584,3,720),(1585,3,720),(1593,3,1440),(1594,3,720),(1595,3,720),(1596,3,720),(1597,3,720),(1598,3,60),(1599,10,205),(1609,3,1200),(1610,3,900),(1611,3,600),(1625,10,360),(1632,3,720),(1666,3,900),(1667,10,150),(1668,10,150),(1669,10,150),(1670,3,360),(1688,3,600),(1689,3,1100),(1693,3,350),(1694,3,270),(1695,3,330),(1701,3,810),(1708,3,240),(1709,10,205),(1710,11,420),(1711,3,990),(1713,8,75),(1720,7,10),(1726,3,270),(1727,10,150),(1728,3,720),(1729,3,360),(1742,3,630),(1743,3,50),(1750,5,3),(1752,5,3),(1757,5,3),(1759,5,3),(1760,5,3),(1762,5,3),(1763,5,3),(1765,5,3),(1774,3,720),(1819,3,630),(1900,50,0),(1901,50,0),(1902,50,0),(1903,50,0),(1904,50,0),(1905,50,0),(1906,50,0),(1907,50,0),(1908,50,0),(1909,50,0),(1910,50,0),(1911,50,0),(1912,50,0),(1913,50,0),(1914,50,0),(1915,50,0),(1916,50,0),(1917,50,0),(1918,50,0),(1919,50,0),(1920,50,0),(1921,50,0),(1922,50,0),(1923,50,0),(1924,50,0),(1973,4,25),(2001,11,270),(2007,7,3),(2009,50,0),(2050,4,25),(2079,50,0),(2109,3,1440),(2110,11,270),(2112,7,65),(2114,3,600),(2119,11,600),(2122,3,1500),(2125,10,150),(2176,11,450),(2177,11,720),(2178,3,720),(2188,3,1000),(2248,11,630),(2307,11,5000),(2314,50,0),(2326,8,4),(2505,3,1440),(2509,3,1440),(2510,3,1500),(2511,11,270),(2512,3,270),(2513,3,360),(2514,3,540),(2515,3,720),(2516,3,200),(2517,3,600),(2519,3,1500),(2521,11,270),(2523,3,1440),(2524,3,360),(2525,3,720),(2528,10,205),(2529,3,1500),(2530,3,1000),(2537,3,100),(2539,3,1500),(2541,11,600),(2551,2,50),(2553,3600,0),(2555,3600,0),(2557,3600,0),(2559,3,1100),(2560,3600,0),(2561,11,270),(2562,11,270),(2563,6,6),(2564,6,6),(2565,3,360),(2566,7,65),(2567,6,4),(2568,6,6),(2569,6,6),(2570,3,1500),(2574,3,200),(2576,3,200),(2580,3,1500),(2583,3,600),(2584,3,500),(2585,3,300),(2588,3,175),(2590,3,1500),(2592,11,700),(2593,3,600),(2594,3,300),(2595,10,205),(2596,11,700),(2597,3,200),(2598,3,750),(2599,11,700),(2600,3,1500),(2601,6,3),(2602,6,15),(2603,6,5),(2604,6,3),(2605,4,25),(2606,6,3),(2607,5,3),(2608,5,3),(2609,5,3),(2610,6,3),(2619,11,600),(2625,11,600),(2628,11,600),(2629,11,720),(2630,3,720),(2635,3,400),(2636,3,420),(2637,3,440),(2638,3,460),(2639,3,480),(2640,3,480),(2641,3,480),(2759,50,0),(2760,50,0),(2761,50,0),(2769,11,2040),(2826,3,360),(2879,3,900),(2881,3,270),(2886,11,630),(2887,3,1950),(2888,3,480),(2890,3,480),(2892,3,600),(2893,3,720),(2894,10,205),(2895,3,300),(2941,7,65),(2977,11,450),(2978,11,270),(2980,11,270),(3024,11,420),(3031,3,1500),(3039,3,1500),(3047,3,720),(3063,3,360),(3076,11,2040),(3078,11,2040),(3079,11,2040),(3090,11,5),(3092,50,0),(3094,50,0),(3095,50,0),(3096,50,0),(3097,50,0),(3098,50,0),(3099,50,0),(3137,50,2),(3141,50,0),(3142,50,0),(3143,50,0),(3144,50,0),(3145,50,0),(3146,50,0),(3147,50,0),(3148,50,0),(3149,50,0),(3178,11,420),(3185,3,600),(3186,8,4),(3197,8,7),(3198,10,150),(3199,3,360),(3211,50,0),(3212,50,0),(3213,50,0),(3214,50,0),(3215,50,0),(3216,50,0),(3217,50,0),(3218,50,0),(3219,50,0),(3220,50,0),(3227,3,600),(3234,3,1000),(3235,3,800),(3237,11,600),(3240,11,420),(3241,3,120),(3242,3,500),(3247,3,1440),(3291,11,6),(3295,10,150),(3296,3,720),(3300,3,900),(3301,3,1200),(3302,3,900),(3305,11,600),(3311,3,600),(3326,3,650),(3329,3,650),(3337,3,600),(3343,3,1100),(3350,3,800),(3351,8,75),(3360,3,800),(3361,7,5),(3362,5,3),(3368,5,3),(3372,5,3),(3374,5,3),(3378,3,720),(3381,3,720),(3382,3,720),(3383,3,630),(3384,3,1500),(3388,3,720),(3389,3,630),(3391,3,360),(3392,3,630),(3397,3,800),(3399,7,65),(3410,3,100),(3415,3,600),(3417,3,800),(3419,3,600),(3420,3,600),(3422,3,600),(3424,3,600),(3432,3,800),(3439,3,720),(3444,3,1500),(3448,10,150),(3450,3,1950),(3451,3,1000),(3453,3,1950),(3454,3,720),(3456,3,720),(3458,11,600),(3459,3,480),(3460,11,720),(3463,7,65),(3466,3,630),(3467,3,1500),(3470,3,810),(3472,3,400),(3474,3,1440),(3479,3,1500),(3486,10,150),(3487,3,600),(3488,3,600),(3490,3,1500),(3499,50,0),(3556,50,0),(3557,50,0),(3558,50,0),(3559,50,0),(3575,3,400),(3576,3,400),(3578,3,1500),(3579,3,1440),(3580,3,360),(3581,3,720),(3582,3,720),(3586,3,720),(3601,2,7),(3627,11,270),(3628,11,150),(3651,5,3),(3667,11,40),(3669,11,600),(3671,11,600),(3677,50,0),(3678,50,0),(3679,50,0),(3680,50,0),(3690,11,600),(3692,3,1000),(3696,3,360),(3700,11,600),(3707,50,0),(3708,50,0),(3709,50,0),(3713,11,270),(3715,50,0),(3716,50,0),(3754,11,4),(3759,11,5),(3760,11,5),(3763,11,5000),(3767,11,10),(3769,11,5),(3770,11,15),(3771,11,15),(3772,11,10),(3774,11,15),(3775,11,15),(3776,11,10),(3778,11,15),(3779,11,10),(3811,3,600),(3861,11,300),(3865,11,5),(3866,11,5),(3908,11,10),(3917,50,0),(3918,50,0),(3919,50,0),(4011,3,250),(4013,50,0),(4015,11,25),(4017,3,360),(4044,50,0),(4045,50,0),(4046,50,0),(4047,50,0),(4048,50,0),(4053,3,1000),(4054,3,360),(4055,3,360),(4058,3,1440),(4059,10,100),(4062,11,600),(4063,11,600),(4064,3,600),(4065,3,600),(4067,3,360),(4068,3,360),(4069,3,360),(4070,3,270),(4071,3,270),(4073,3,360),(4074,3,360),(4075,3,360),(4076,3,360),(4079,3,360),(4080,3,360),(4081,3,360),(4083,5,3),(4084,5,3),(4085,5,3),(4086,5,3),(4087,5,3),(4088,3,360),(4089,3,360),(4090,3,360),(4091,3,360),(4099,5,2),(4100,5,2),(4107,3,1440),(4108,3,450),(4109,3,600),(4112,5,3),(4123,11,20),(4124,11,300),(4127,11,15),(4128,11,15),(4129,11,1),(4141,11,20),(4142,11,30),(4143,11,300),(4144,11,300),(4145,11,300),(4147,11,600),(4148,11,600),(4149,11,20),(4151,11,600),(4153,11,20),(4154,11,30),(4155,11,300),(4156,11,300),(4157,11,600),(4158,11,300),(4161,11,600),(4162,11,600),(4163,11,600),(4181,50,0),(4182,50,0),(4183,50,0),(4184,50,0),(4243,11,100),(4244,11,100),(4245,11,100),(4246,11,100),(4247,11,100),(4248,11,100),(4288,11,20),(4395,5,3),(4411,11,600),(4412,11,150),(4416,11,4),(4418,3,360),(4419,50,0),(4420,50,0),(4421,50,0),(4422,50,0),(4423,50,0),(4428,11,600),(4472,11,100),(4476,11,100),(4479,11,100),(4498,11,10),(4499,11,30),(4500,11,50),(4501,11,10),(4502,11,2),(4503,11,30),(4504,11,4),(4505,11,5),(4506,11,20),(4507,11,10),(4508,11,10),(4509,11,2),(4510,11,2),(4511,11,10),(4512,11,5),(4513,11,5),(4514,11,5),(4515,11,2),(4516,11,3),(4517,11,5),(4518,11,3),(4519,11,4),(4520,11,50),(4555,50,0),(4576,11,150),(4585,11,50),(4586,11,40),(4587,11,10),(4612,11,2),(4663,11,0),(4670,11,2),(4671,11,2),(4672,11,5),(4673,11,2),(4674,11,2),(4675,11,5),(4676,11,5),(4677,11,5),(4678,11,5),(4687,11,10),(4688,11,30),(4690,11,2),(4691,11,5),(4692,11,3),(4693,11,3),(4694,11,5),(4695,11,5),(4696,11,3),(4705,11,32),(4720,11,30),(4721,11,10),(4730,50,0),(4731,50,0),(4740,11,300),(4749,11,100),(4750,50,0),(4751,50,0),(4752,50,0),(4753,50,0),(4754,50,0),(4755,50,0),(4756,50,0),(4757,50,0),(4758,50,0),(4759,50,0),(4760,50,0),(4761,50,0),(4762,50,0),(4763,50,0),(4764,50,0),(4765,50,0),(4766,50,0),(4767,50,0),(4768,50,0),(4769,50,0),(4770,50,0),(4771,50,0),(4772,50,0),(4773,50,0),(4774,50,0),(4775,50,0),(4776,50,0),(4777,50,0),(4778,50,0),(4779,50,0),(4780,50,0),(4781,50,0),(4782,50,0),(4783,50,0),(4784,50,0),(4785,50,0),(4871,5,3),(4872,5,3),(4873,5,2),(4895,3,600),(4898,11,600),(4902,3,600),(4903,3,600),(4957,11,300),(4978,3,600),(4993,11,200),(5007,11,600),(5034,11,10),(5035,11,10),(5036,11,30),(5037,11,5),(5038,11,2),(5039,11,5),(5040,11,2),(5041,11,5),(5042,11,20),(5043,11,10),(5044,11,3),(5050,11,5),(5066,11,270),(5069,11,300),(5094,50,1000),(5100,11,270),(5106,11,270),(5116,11,50),(5127,11,0),(5148,11,0),(5250,3,720),(5252,3,720),(5253,3,720),(5257,3,1500),(5258,3,400),(5261,3,360),(5272,3,450),(5273,8,4),(5274,8,7),(5276,3,1440),(5277,3,720),(5278,3,1500),(5280,3,720),(5285,3,600),(5287,3,720),(5288,3,600),(5290,3,720),(5291,3,1440),(5294,3,810),(5295,3,630),(5297,3,800),(5298,3,600),(5300,11,150),(5302,10,150),(5305,3,600),(5306,3,600),(5307,3,1950),(5310,10,205),(5311,3,600),(5312,3,800),(5315,3,810),(5316,2,7),(5317,3,1500),(5318,3,600),(5327,3,600),(5332,11,600),(5333,3,600),(5339,3,1500),(5347,2,7),(5350,3,720),(5352,3,1000),(5356,5,3),(5358,10,150),(5362,3,1950),(5365,10,150),(5366,3,1000),(5368,3,1950),(5370,7,5),(5374,5,10),(5376,5,3),(5377,5,3),(5380,5,3),(5382,5,3),(5384,5,3),(5388,5,3),(5390,3,720),(5392,11,140),(5396,3,800),(5397,3,720),(5398,3,720),(5399,3,630),(5402,9,100),(5404,3,630),(5405,3,630),(5407,11,150),(5409,3,630),(5415,3,800),(5417,7,65),(5421,3,900),(5425,11,600),(5427,11,140),(5428,3,1200),(5434,3,600),(5436,3,360),(5443,3,900),(5448,3,1200),(5453,3,270),(5459,3,360),(5466,10,150),(5472,3,900),(5476,3,1500),(5478,11,600),(5488,10,150),(5492,5,0),(5494,3,360),(5500,3,360),(5502,3,900),(5504,3,1100),(5506,8,7),(5507,11,420),(5513,3,800),(5514,8,75),(5515,3,360),(5517,3,360),(5521,11,420),(5522,3,800),(5529,3,720),(5530,3,720),(5533,11,600),(5534,3,480),(5536,10,205),(5537,11,720),(5539,3,360),(5542,7,65),(5605,50,0),(5691,11,150),(5695,11,150),(5704,11,100),(5705,11,100),(5708,11,100),(5713,11,100),(5728,11,30),(5729,11,100),(5738,11,500),(5744,11,500),(5761,11,500),(5764,50,0),(5765,50,0),(5766,50,0),(5767,50,0),(5768,50,0),(5769,50,0),(5802,11,150),(5823,11,100),(5833,50,0),(5834,50,0),(5835,50,0),(5836,50,0),(5853,11,50),(5874,50,0),(5875,50,0),(5876,50,0),(5884,50,0),(5885,50,0),(5886,50,0),(5890,50,0),(5891,50,0),(5892,50,0),(5893,50,0),(5894,50,0),(5895,50,0),(5896,50,0),(5897,50,0),(5898,50,0),(5899,50,0),(5940,50,0),(5941,50,0),(5942,50,0),(5954,50,0),(5955,50,0),(5956,50,0),(5957,50,0),(5958,50,0),(5959,50,0),(5960,50,0),(5961,50,0),(5962,50,0),(5963,50,0),(5964,50,0),(5965,50,0),(5966,50,0),(5967,50,0),(5968,50,0),(5969,50,0),(5970,50,0),(5971,50,0),(5972,50,0),(5973,50,0),(5974,50,0),(5997,11,0),(6006,11,0),(6040,11,3),(6041,11,3),(6042,11,3),(6047,11,150),(6048,11,150),(6076,50,0),(6077,50,0),(6078,50,0),(6082,50,0),(6083,50,0),(6084,50,0),(6085,50,0),(6086,50,0),(6087,50,0),(6088,50,0),(6100,11,600),(6120,3,100),(6122,3,100),(6123,3,100),(6124,3,100),(6125,3,100),(6127,50,0),(6128,50,0),(6129,11,600),(6130,11,600),(6131,11,300),(6136,50,0),(6137,50,0),(6138,50,0),(6139,50,0),(6145,6,4),(6148,50,0),(6149,50,0),(6150,50,0),(6151,50,0),(6190,11,10),(6191,11,10),(6192,11,5),(6193,11,3),(6194,11,5),(6195,11,10),(6196,11,10),(6197,11,5),(6198,11,5),(6199,11,5),(6200,11,10),(6201,11,10),(6203,11,5),(6204,11,10),(6207,11,5),(6234,11,10),(6244,11,200),(6245,11,600),(6246,11,200),(6256,11,0),(6266,11,10),(6276,11,200),(6277,11,600),(6278,11,200),(6299,11,10),(6367,11,270),(6503,50,0),(6545,11,100),(6549,11,100),(6643,11,100),(6662,2,20),(6664,11,30),(6665,11,30),(6666,5,0),(6667,3,10),(6671,3,1100),(6723,11,150),(6730,2,20),(6732,11,30),(6733,11,30),(6734,5,0),(6735,3,10),(6739,3,1100),(6770,11,270),(6793,11,5000),(6800,11,300),(6801,11,150),(6817,50,0),(6818,50,0),(6819,50,0),(6820,50,0),(6821,50,0),(6822,50,0),(6823,50,0),(6824,50,0),(6833,50,0),(6835,11,2040),(6887,11,100),(6902,2,20),(6903,2,20),(6906,3,10),(6907,3,10),(6973,11,500),(7006,11,150),(7007,11,150),(7015,11,100),(7023,11,100),(7024,11,100),(7025,11,100),(7026,11,100),(7038,11,300),(7042,11,300),(7048,11,15),(7049,11,150),(7054,11,300),(7058,11,300),(7059,11,300),(7069,11,300),(7079,11,300),(7080,11,200),(7081,11,200),(7082,11,200),(7086,11,200),(7087,11,200),(7100,11,300),(7129,11,300),(7134,11,300),(7138,11,300),(7144,11,300),(7149,11,300),(7153,11,300),(7167,11,10),(7170,11,200),(7200,11,140),(7209,11,200),(7210,11,200),(7235,50,0),(7236,50,0),(7237,50,0),(7238,50,0),(7239,50,0),(7240,50,0),(7241,50,0),(7242,50,0),(7243,50,0),(7244,50,0),(7245,50,0),(7246,50,0),(7247,50,0),(7248,50,0),(7249,50,0),(7250,50,0),(7274,50,0),(7275,50,0),(7276,50,0),(7277,50,0),(7278,50,0),(7279,50,0),(7280,50,0),(7281,50,0),(7282,50,0),(7283,50,0),(7284,50,0),(7285,50,0),(7286,50,0),(7287,50,0),(7288,50,0),(7289,50,0),(7290,50,0),(7291,50,0),(7292,50,0),(7293,50,0),(7294,50,0),(7295,50,0),(7296,50,0),(7297,50,0),(7298,50,0),(7299,50,0),(7308,11,100),(7326,11,150),(7342,11,20),(7348,50,0),(7349,50,0),(7350,50,0),(7351,50,0),(7352,50,0),(7353,50,0),(7354,50,0),(7355,50,0),(7356,50,0),(7357,50,0),(7358,50,0),(7359,50,0),(7360,50,0),(7361,50,0),(7362,50,0),(7363,50,0),(7364,50,0),(7365,50,0),(7366,50,0),(7367,50,0),(7368,50,0),(7369,50,0),(7370,50,0),(7371,50,0),(7372,50,0),(7373,50,0),(7374,50,0),(7375,50,0),(7376,50,0),(7377,50,0),(7378,50,0),(7379,50,0),(7380,50,0),(7381,50,0),(7382,50,0),(7383,50,0),(7384,50,0),(7385,50,0),(7386,50,0),(7387,50,0),(7388,50,0),(7389,50,0),(7390,50,0),(7391,50,0),(7392,50,0),(7393,50,0),(7394,50,0),(7395,50,0),(7396,50,0),(7397,50,0),(7398,50,0),(7399,50,0),(7476,11,30),(7541,11,5),(7542,11,5),(7543,11,5),(7601,50,0),(7602,50,0),(7603,50,0),(7604,50,0),(7605,50,0),(7606,50,0),(7607,50,0),(7608,50,0),(7609,50,0),(7610,50,0),(7611,50,0),(7612,50,0),(7613,50,0),(7614,50,0),(7615,50,0),(7616,50,0),(7617,50,0),(7702,11,200),(7746,11,2),(7747,50,0),(7825,11,270),(8008,2,45),(8015,2,45),(8019,11,10),(8029,3,300),(8030,11,10),(8032,3,20),(8036,3,360),(8038,2,30),(8141,50,0),(8142,50,0),(8144,11,10),(8145,11,15),(8153,11,1200),(8222,11,600),(8233,11,5),(8234,11,5),(8241,11,600),(8251,11,200),(8252,11,200),(8406,50,0),(8407,50,0),(8408,50,0),(8463,11,270),(8464,11,270),(8473,11,5),(8479,3,20),(8484,2,50),(8486,5,0),(8494,3,5),(8497,6,4),(8506,3,100),(8517,3,300),(8520,3,600),(8583,11,150),(8584,11,150),(8585,11,150),(8595,11,150),(8596,11,150),(8612,11,30),(8622,11,18),(8644,11,40),(8661,11,300),(8670,11,100),(8678,11,10),(8690,50,0),(8693,11,50),(8707,11,20),(8718,11,100),(8730,50,0),(8734,11,150),(8761,11,500),(8783,3,480),(8827,11,20),(8847,11,20),(8863,50,0),(8898,50,0),(8899,50,0),(8927,50,0),(8952,11,5000),(8971,11,100),(9017,11,150),(9018,11,300),(9055,11,100),(9059,50,0),(9060,50,0),(9061,50,0),(9089,50,0),(9119,11,100),(9120,11,100),(9121,11,100),(9290,11,300),(9328,11,600),(9331,11,25),(9332,11,30),(9402,50,0),(9403,50,0),(9404,50,0),(9405,50,0),(9440,50,0),(9441,50,0),(9442,50,0),(9488,50,0),(9489,50,0),(9496,50,0),(9497,50,0),(9498,50,0),(9499,50,0),(9624,11,270),(9625,11,270),(9703,3,400),(9709,3,720),(9712,3,720),(9721,3,360),(9730,3,1500),(9742,3,360),(9755,2,3),(9764,3,600),(9779,3,450),(9782,8,4),(9785,2,20),(9791,3,360),(9797,8,5),(9803,3,1440),(9806,3,720),(9809,3,1500),(9818,3,360),(9842,11,162),(9851,2,5),(9857,3,720),(9858,3,720),(9859,3,720),(9866,10,150),(9872,3,1000),(9881,3,1950),(9905,5,3),(9926,10,150),(9929,3,1000),(9959,3,720),(9974,3,5),(9987,3,1200),(9988,3,1200),(9989,3,1200),(9990,3,1200),(9991,3,1200),(10005,3,800),(10011,3,630),(10026,3,630),(10056,3,800),(10080,10,150),(10098,3,600),(10110,2,7),(10113,3,800),(10128,3,810),(10131,3,1500),(10134,3,600),(10140,3,1950),(10146,3,2000),(10194,3,630),(10197,3,1440),(10209,3,800),(10212,3,600),(10215,3,2000),(10251,3,600),(10282,11,600),(10285,3,600),(10300,3,1500),(10315,3,2000),(10336,3,720),(10339,3,720),(10342,8,3),(10349,11,600),(10352,3,480),(10358,3,100),(10370,11,720),(10373,3,360),(10383,7,65),(10392,3,360),(10401,7,5),(10410,7,3),(10419,5,3),(10421,5,3),(10425,5,3),(10427,5,3),(10431,5,3),(10437,5,10),(10440,5,3),(10443,5,3),(10458,5,3),(10461,8,5),(10476,3,900),(10503,11,600),(10516,11,140),(10525,3,1200),(10526,3,1200),(10527,3,1200),(10528,3,360),(10531,3,1200),(10558,3,600),(10580,3,1100),(10583,3,900),(10590,11,162),(10596,3,1100),(10599,8,7),(10602,11,420),(10605,3,360),(10606,3,360),(10607,3,360),(10617,3,800),(10620,3,360),(10641,8,75),(10653,3,360),(10659,11,420),(10662,3,800),(10666,3,100),(10674,3,420),(10684,10,150),(10692,3,900),(10712,3,1500),(10720,5,0),(10726,3,360),(10729,11,600),(10741,3,360),(10747,10,150),(10766,3,360),(10773,3,900),(10810,3,1200),(10828,3,270),(10853,3,360),(10862,3,1200),(10889,11,10),(10895,11,100),(10898,11,5),(10923,11,5),(10938,11,2),(10941,11,10),(10942,11,10),(10943,11,10),(10956,11,5),(10959,11,10),(10962,11,5),(10963,11,5),(10964,11,5),(10965,11,30),(11025,50,0),(11029,50,0),(11030,50,0),(11031,50,0),(11077,50,0),(11078,50,0),(11094,11,600),(11095,11,600),(11096,11,600),(11103,50,0),(11104,50,0),(11105,50,0),(11202,50,0),(11213,11,6),(11214,11,6),(11242,50,0),(11321,50,0),(11333,50,0),(11334,50,0),(11335,50,0),(11363,11,10),(11374,50,0),(11398,50,0),(11401,50,0),(11402,50,0),(11403,50,0),(11405,11,30),(11406,11,30),(11407,11,30),(11408,11,30),(11453,11,300),(11471,50,0),(11474,11,150),(11475,11,150),(11488,50,0),(11633,50,0),(11656,11,20),(11689,50,0),(11690,11,100),(11712,11,100),(11732,11,300),(11745,50,0),(11766,6,4),(11769,50,0),(11782,2,45),(11819,3,300),(11849,11,1),(11863,3,600),(11873,5,3),(11876,5,3),(11879,5,3),(11885,3,1100),(11903,3,20),(11913,11,20),(11916,11,5),(11922,11,10),(11936,11,0),(11938,11,10),(11953,11,100),(11965,50,0),(11966,11,8),(11973,11,100),(12004,50,0),(12026,50,0),(12053,50,0),(12059,50,0),(12081,1,1),(12106,11,50),(12109,11,100),(12147,50,0),(12149,50,0),(12152,50,0),(12162,50,0),(12182,50,0),(12299,3,360),(12315,3,360),(12322,3,360),(12323,11,5),(12329,3,360),(12330,3,360),(12335,3,360),(12337,3,360),(12382,3,1950),(12383,3,1950),(12384,3,1950),(12385,3,1950),(12401,3,360),(12402,3,360),(12455,50,0),(12471,50,0),(12492,3,360),(12526,11,6),(12527,11,6),(12528,11,6),(12658,50,0),(12659,50,0),(12660,50,0),(12664,50,0),(12665,50,0),(12666,50,0),(12677,50,0),(12678,50,0),(12679,50,0),(12697,11,10),(12699,11,50),(12718,11,120),(12720,50,500),(12732,50,500),(12734,11,100),(12745,50,300),(12779,50,0),(12780,50,0),(12781,50,0),(12788,50,0),(12789,50,0),(12790,50,0),(12803,50,0),(12804,50,0),(12805,50,0),(12806,50,0),(12807,50,0),(12808,50,0),(12826,50,0),(12827,50,0),(12979,11,10),(12992,50,0),(12993,50,0),(12994,50,0),(12995,50,0),(12996,50,0),(12997,50,0),(13021,11,15),(13045,11,600),(13067,50,0),(13074,11,600),(13077,11,50),(13078,11,50),(13079,11,50),(13111,11,50),(13112,11,50),(13113,11,50),(13114,11,50),(13115,11,50),(13125,7,20),(13209,11,6),(13210,11,6),(13211,11,6),(13219,50,100000),(13330,50,0),(13333,11,2040),(13515,50,0),(13677,50,0),(13678,50,0),(13679,50,0),(13680,50,0),(13681,50,0),(13682,50,0),(13689,50,0),(13690,50,0),(13691,50,0),(13800,50,0),(13811,11,6),(13812,11,6),(13813,11,6),(13815,11,6),(13816,11,6),(13817,11,6),(13836,50,0),(13860,11,4),(13862,11,5),(13863,11,5),(13865,11,5000),(13869,11,10),(13871,11,5),(13872,11,15),(13873,11,15),(13874,11,10),(13876,11,15),(13877,11,15),(13878,11,10),(13891,11,15),(13892,11,10),(13994,50,3600),(14000,7,5),(14006,5,3),(14009,5,3),(14012,5,3),(14018,5,10),(14021,5,3),(14024,5,3),(14036,5,3),(14039,8,5),(14048,5,3),(14051,5,3),(14054,5,3),(14057,5,3),(14060,5,3),(14075,5,3),(14078,5,3),(14081,5,0),(14102,3,720),(14105,3,720),(14108,8,3),(14114,11,600),(14117,3,480),(14123,3,100),(14132,11,720),(14135,3,360),(14142,7,65),(14148,3,360),(14164,3,360),(14167,11,5),(14189,11,5),(14192,11,20),(14207,3,400),(14213,3,720),(14216,3,720),(14222,3,360),(14225,3,1500),(14237,3,360),(14246,3,720),(14249,2,3),(14255,3,600),(14270,3,450),(14273,8,4),(14276,2,20),(14288,8,5),(14294,3,1440),(14297,3,720),(14300,3,1500),(14306,3,360),(14346,3,60),(14367,2,5),(14382,10,150),(14388,3,1000),(14397,3,1950),(14433,10,150),(14436,3,1000),(14442,3,1950),(14458,11,10),(14460,6,4),(14467,2,45),(14476,3,3),(14497,3,1100),(14500,3,900),(14506,3,1100),(14509,8,7),(14512,11,420),(14521,3,800),(14524,3,360),(14542,8,75),(14554,3,360),(14560,11,420),(14563,3,800),(14566,3,100),(14572,3,420),(14581,3,1100),(14599,3,20),(14623,11,3),(14653,11,3),(14656,3,360),(14657,3,360),(14658,3,360),(14662,10,150),(14668,3,900),(14686,3,1500),(14694,5,0),(14697,3,360),(14700,11,600),(14715,10,150),(14734,3,10),(14741,3,300),(14778,2,30),(14790,11,2),(14793,11,10),(14794,11,10),(14795,11,10),(14808,11,10),(14817,11,5),(14818,11,5),(14819,11,5),(14820,11,5),(14830,3,900),(14852,11,600),(14865,11,140),(14874,3,360),(14877,3,1200),(14894,3,600),(14904,3,360),(14948,3,720),(14966,3,630),(14969,3,1440),(14978,3,800),(14981,3,600),(14996,3,20),(15008,3,600),(15017,10,150),(15029,3,600),(15035,2,7),(15038,3,800),(15056,3,810),(15059,3,1500),(15062,3,600),(15068,3,1950),(15075,11,1),(15076,3,600),(15079,3,3),(15085,3,200),(15091,11,20),(15100,11,10),(15109,11,100),(15112,11,5),(15154,3,600),(15175,11,600),(15178,3,600),(15187,3,1500),(15202,3,600),(15214,2,50),(15238,3,5),(15262,3,720),(15265,3,720),(15277,3,630),(15304,3,720),(15323,2,45),(15333,10,210),(15336,3,15),(15345,11,10),(15348,11,5),(15349,11,5),(15350,11,5),(15351,11,30),(15366,11,5),(15387,3,900),(15422,3,1200),(15439,3,270),(15462,3,360),(15471,3,1200),(15505,7,9),(15511,3,80),(15599,50,0),(15876,11,270),(15877,11,270),(15900,11,450),(15901,11,450),(15902,11,450),(15903,11,450),(15904,11,450),(15905,11,450),(15906,11,450),(15907,11,450),(16209,50,0),(16226,50,3600),(16227,50,100000),(16229,50,0),(16455,50,0),(16524,50,0),(16601,11,100),(16755,11,6),(16756,11,6),(16757,11,6),(16758,11,6),(16759,11,6),(16760,11,6),(16939,50,100000),(16940,50,0),(16960,50,0),(16961,50,0),(16962,50,0),(16963,50,0),(16964,50,0),(16965,50,0),(16966,50,0),(16967,50,0),(16968,50,0),(16969,50,0),(16970,50,0),(16971,50,0),(16982,50,0),(16983,50,0),(16984,50,0),(16985,50,0),(16986,50,0),(16987,50,0),(16988,50,0),(16989,50,0),(16990,50,0),(16992,50,0),(16995,11,100),(17060,11,1500),(17061,11,1500),(17158,50,0),(17159,50,0),(17160,50,0),(17161,50,0),(17162,50,0),(17163,50,0),(17164,50,0),(17165,50,0),(17166,50,0),(17167,50,0),(17168,50,0),(17169,50,0),(17170,50,0),(17171,50,0),(17172,50,0),(17173,50,0),(17174,50,0),(17175,50,0),(17176,50,0),(17177,50,0),(17183,50,0),(17187,11,270),(17293,50,0),(17320,50,1),(17380,11,500),(17461,11,0),(17482,50,0),(17655,5,3),(17767,5,3),(17772,50,0),(17836,11,100),(17864,3,360),(17910,50,0),(17913,50,0),(18000,7,5),(18006,5,3),(18009,5,3),(18012,5,10),(18015,5,3),(18018,5,3),(18030,8,5),(18039,5,3),(18042,5,3),(18045,5,3),(18060,5,3),(18063,5,3),(18066,5,0),(18078,5,3),(18081,5,3),(18084,5,10),(18090,5,3),(18114,3,720),(18117,3,720),(18120,8,3),(18126,11,600),(18129,3,600),(18135,3,100),(18144,11,720),(18154,7,65),(18160,3,360),(18173,11,600),(18174,11,600),(18175,11,600),(18179,11,5),(18188,11,10),(18213,11,20),(18228,3,400),(18234,3,720),(18237,3,720),(18243,3,360),(18246,3,1500),(18258,3,360),(18267,3,720),(18270,2,3),(18276,3,600),(18291,3,450),(18294,8,4),(18297,2,20),(18309,8,5),(18315,3,1440),(18318,3,720),(18321,3,1500),(18327,3,360),(18367,3,86),(18373,7,5),(18382,11,10),(18407,2,5),(18422,10,150),(18428,3,1000),(18437,3,1950),(18473,10,150),(18476,3,1000),(18482,3,1950),(18498,11,10),(18500,6,4),(18507,2,45),(18534,2,3),(18549,3,25),(18555,3,1100),(18558,3,900),(18564,3,1100),(18567,8,7),(18570,11,420),(18579,3,800),(18582,3,360),(18600,8,75),(18612,3,360),(18618,11,420),(18621,3,800),(18624,3,100),(18630,3,420),(18639,3,1100),(18657,3,20),(18681,11,3),(18711,11,3),(18729,10,150),(18753,3,1500),(18761,5,0),(18764,3,360),(18767,11,600),(18782,10,150),(18801,3,10),(18814,3,300),(18848,2,30),(18880,3,3),(18881,3,3),(18882,3,3),(18883,3,200),(18898,11,10),(18913,11,10),(18922,11,5),(18923,11,5),(18924,11,5),(18925,11,5),(18957,11,600),(18970,11,140),(18982,3,1200),(18999,3,600),(19062,3,720),(19080,3,630),(19083,3,1440),(19092,3,800),(19095,3,600),(19110,3,20),(19122,3,600),(19137,3,25),(19149,10,150),(19161,3,600),(19167,2,7),(19170,3,800),(19188,3,810),(19191,3,1500),(19194,3,600),(19200,3,1950),(19207,11,1),(19208,3,600),(19217,3,200),(19223,11,20),(19229,3,600),(19238,3,1950),(19256,11,100),(19259,11,5),(19307,3,600),(19328,11,600),(19331,3,600),(19340,3,1500),(19355,3,600),(19367,2,50),(19400,3,5),(19424,3,720),(19425,3,720),(19426,3,720),(19427,3,720),(19428,3,720),(19429,3,720),(19472,5,1),(19475,5,1),(19485,2,45),(19498,3,15),(19507,10,205),(19516,11,10),(19519,11,5),(19520,11,5),(19521,11,5),(19522,11,30),(19552,11,3),(19602,3,1200),(19623,3,270),(19650,3,360),(19659,3,1200),(19699,3,107),(19735,3,480),(19756,3,10),(19762,3,10),(19820,8,60),(19829,11,2),(19835,5,2),(19844,6,10),(19847,6,30),(19868,10,15),(19869,10,15),(19870,10,15),(20528,11,270),(20529,11,270),(20545,50,0),(21061,7,4),(21112,11,270),(21905,11,270),(21906,11,270),(21907,11,270),(21908,11,270),(21909,11,270),(21910,11,270),(21911,11,270),(21912,11,270),(21913,11,270),(21914,11,270),(21960,50,0),(21961,50,0),(22233,50,0),(22497,5,4),(22516,6,10),(22522,6,10),(22649,2,10),(22650,2,10),(22651,2,10),(22732,50,0),(22733,50,0),(22734,11,50),(22735,50,0),(22736,50,0),(22737,50,0),(22738,50,0),(22739,50,0),(22792,11,90),(23035,11,600),(23057,50,0),(23058,50,0),(23059,50,0),(23060,50,0),(23061,50,0),(23062,50,0),(23063,50,0),(23064,50,0),(23065,50,0),(23066,50,0),(23075,50,0),(23076,50,0),(23094,50,0),(23095,50,0),(23100,50,0),(23101,50,0),(23102,50,0),(23177,11,2),(23218,50,0),(23224,50,0),(23400,11,270),(23401,11,270),(23402,11,270),(23403,11,270),(23443,50,0),(23479,50,0),(23480,50,0),(23481,50,0),(23482,50,0),(23483,50,0),(23500,50,0),(23501,50,0),(23502,50,0),(23506,50,0),(23507,50,0),(23508,50,0),(23512,50,0),(23513,50,0),(23514,50,0),(23580,50,0),(24091,11,10),(24110,50,0),(24126,50,0),(24127,50,0),(24128,50,0),(24911,50,1),(24912,50,1),(24913,50,1),(24914,50,360),(24934,50,360),(24956,4,20),(24957,3,12),(25000,11,9),(25003,11,20),(25006,11,3),(25009,11,4),(25012,11,10),(25015,11,5),(25016,11,5),(25017,11,5),(25057,3,1),(25058,3,1),(25059,3,1),(25068,3,720),(25077,3,720),(25089,3,720),(25092,3,720),(25101,8,5),(25104,3,720),(25113,8,5),(25116,3,720),(25128,3,1500),(25137,2,20),(25159,3,720),(25165,2,3),(25195,2,45),(25204,3,1500),(25210,3,720),(25213,3,10),(25228,3,1500),(25243,3,720),(25246,3,720),(25255,2,45),(25270,3,25),(25294,8,4),(25318,3,300),(25330,3,1440),(25351,3,800),(25369,3,720),(25381,3,300),(25384,3,20),(25402,15,50),(25403,15,50),(25404,15,50),(25417,2,7),(25426,10,210),(25429,3,600),(25435,3,210),(25441,3,600),(25453,10,15),(25454,10,15),(25455,10,15),(25465,2,7),(25468,3,800),(25471,3,1950),(25480,3,600),(25486,3,1950),(25492,3,810),(25513,3,600),(25522,3,1500),(25525,11,20),(25531,3,2000),(25534,10,10),(25535,10,10),(25536,10,10),(25556,3,600),(25565,11,600),(25598,3,600),(25604,11,600),(25607,3,600),(25629,11,600),(25635,3,1500),(25653,2,50),(25704,3,25),(25731,3,720),(25732,3,720),(25733,3,720),(25734,2,6),(25737,6,4),(25750,1,1),(25751,1,1),(25752,1,1),(25753,10,360),(25756,3,1000),(25762,2,3),(25768,3,4),(25774,11,3),(25775,11,3),(25776,11,3),(25783,3,1950),(25795,11,3),(25796,11,3),(25797,11,3),(25837,2,45),(25855,3,9),(25862,10,360),(25865,3,1000),(25871,3,1950),(25886,6,10),(25923,11,10),(25929,11,100),(25935,11,5),(25936,11,5),(25937,11,5),(25941,11,5),(25944,11,3),(25959,5,3),(25965,5,3),(25968,5,4),(25977,7,5),(25980,5,3),(25989,5,10),(25998,5,3),(26004,5,3),(26010,5,3),(26013,5,3),(26016,5,3),(26019,5,3),(26025,5,3),(26028,5,3),(26040,5,3),(26043,5,0),(26070,5,3),(26076,8,5),(26130,11,100),(26182,3,5),(26188,5,1),(26191,3,720),(26192,3,720),(26193,3,720),(26194,3,720),(26195,3,720),(26196,3,720),(26197,3,720),(26198,3,720),(26199,3,720),(26203,2,45),(26231,5,3),(26232,5,3),(26233,5,3),(26285,10,205),(26315,5,1),(26318,3,15),(26333,3,900),(26366,3,3),(26390,11,140),(26393,11,600),(26396,6,10),(26408,3,1200),(26429,3,600),(26432,6,30),(26486,5,30),(26520,3,400),(26583,3,1200),(26610,3,1200),(26622,3,400),(26728,8,3),(26737,11,600),(26741,10,600),(26758,3,1500),(26782,5,5),(26796,3,360),(26800,2,100),(26806,8,60),(26812,10,600),(26815,3,600),(26856,6,100),(26859,3,200),(26865,3,33),(26895,3,360),(26899,3,1100),(26902,3,1100),(26903,3,1100),(26904,3,1100),(26905,11,420),(26920,8,7),(26941,11,3),(26942,11,3),(26943,11,3),(26944,3,420),(26963,3,840),(26966,3,360),(26967,3,20),(26997,8,75),(27012,3,360),(27015,11,420),(27024,3,1100),(27030,11,420),(27033,3,360),(27037,11,3),(27043,3,5),(27044,3,5),(27045,3,5),(27056,3,420),(27057,3,420),(27058,3,420),(27062,5,1),(27077,3,840),(27080,3,100),(27101,3,5),(27102,3,5),(27103,3,5),(27104,3,300),(27107,3,480),(27110,3,480),(27131,3,720),(27134,11,10),(27140,3,480),(27146,11,600),(27149,11,600),(27150,11,600),(27151,11,600),(27152,11,6),(27158,3,12),(27183,3,720),(27189,11,720),(27192,11,120),(27195,3,720),(27229,3,360),(27257,11,10),(27284,11,5),(27343,50,0),(27353,50,0),(27354,50,0),(27355,50,0),(27356,50,0),(27357,50,0),(27358,50,0),(27359,50,0),(27360,50,0),(27361,50,0),(27362,50,0),(27363,50,0),(27364,50,0),(27365,50,0),(27366,50,0),(27367,50,0),(27368,50,0),(27369,50,0),(27370,50,0),(27371,50,0),(27372,50,0),(27409,11,6),(27502,50,0),(27523,50,0),(27561,11,6),(27596,50,0),(27597,50,0),(27598,50,0),(27602,50,0),(27603,50,0),(27604,50,0),(27608,50,0),(27609,50,0),(27610,50,0),(27677,11,5),(27678,11,5),(27701,3,360),(27702,3,360),(27703,3,360),(27704,3,360),(27705,3,360),(27706,3,360),(27707,3,360),(27708,3,360),(27709,3,360),(27710,3,360),(27711,3,360),(27712,3,360),(27713,3,360),(27714,3,360),(27715,3,360),(27716,3,360),(27717,3,360),(27718,3,360),(27719,3,360),(27720,3,360),(27721,3,360),(27722,3,360),(27723,3,360),(27724,3,360),(27725,3,360),(27726,3,360),(27727,3,360),(27728,3,360),(27729,3,360),(27730,3,360),(27731,3,360),(27732,3,360),(27733,3,360),(27734,3,360),(27735,3,360),(27736,3,360),(27737,3,360),(27740,3,360),(27741,3,360),(27742,3,360),(27743,3,360),(27744,3,360),(27745,3,360),(27746,3,360),(27747,3,360),(27776,50,0),(27850,50,360),(27851,50,360),(27952,50,0),(27953,50,0),(27954,50,0),(27986,50,1),(27987,50,360),(27988,50,360),(27989,50,360),(27994,50,360),(28000,11,9),(28003,11,20),(28009,11,3),(28015,11,10),(28018,11,5),(28019,11,5),(28020,11,5),(28085,3,720),(28100,8,5),(28103,3,720),(28104,3,720),(28105,3,720),(28112,8,10),(28118,3,720),(28119,3,720),(28120,3,720),(28124,3,720),(28125,3,720),(28126,3,720),(28127,3,720),(28136,8,6),(28139,3,720),(28154,3,1500),(28155,3,1500),(28156,3,1500),(28163,2,20),(28191,3,720),(28197,2,3),(28209,3,720),(28221,8,10),(28242,2,45),(28251,3,1500),(28257,3,1),(28260,3,10),(28275,3,1),(28290,3,1500),(28293,3,720),(28299,3,1),(28302,3,1),(28314,3,25),(28338,8,4),(28341,3,300),(28368,3,300),(28383,3,1440),(28401,15,50),(28402,15,50),(28403,15,50),(28410,3,800),(28416,3,10),(28440,3,720),(28452,3,300),(28455,3,20),(28479,2,7),(28482,10,210),(28488,11,210),(28500,10,210),(28503,3,600),(28512,3,210),(28518,3,600),(28530,10,15),(28531,10,15),(28532,10,15),(28542,2,7),(28545,3,800),(28548,3,1950),(28557,3,600),(28563,3,1950),(28569,3,600),(28593,3,600),(28602,3,1500),(28608,3,2000),(28611,10,10),(28612,10,10),(28613,10,10),(28633,3,600),(28642,11,600),(28681,3,600),(28690,11,600),(28693,3,600),(28721,11,600),(28724,3,1500),(28754,2,50),(28799,3,25),(28830,2,6),(28833,6,4),(28846,1,1),(28847,1,1),(28848,1,1),(28849,10,360),(28852,3,1000),(28861,2,3),(28864,2,3),(28873,11,3),(28874,11,3),(28875,11,3),(28882,3,1950),(28891,11,3),(28892,11,3),(28893,11,3),(28933,2,45),(28954,10,360),(28957,3,1000),(28963,3,1950),(28984,6,10),(29012,11,10),(29013,11,10),(29014,11,10),(29018,11,3),(29030,11,5),(29036,11,100),(29042,11,5),(29043,11,5),(29044,11,5),(29048,11,5),(29060,5,3),(29066,5,3),(29069,5,4),(29070,5,4),(29078,7,5),(29096,5,3),(29102,5,3),(29105,5,3),(29108,5,3),(29114,5,3),(29120,5,3),(29123,5,3),(29141,5,3),(29144,5,0),(29168,5,3),(29174,8,5),(29213,11,10),(29231,11,100),(29252,11,5),(29285,3,5),(29291,5,1),(29297,3,720),(29298,3,720),(29299,3,720),(29300,3,720),(29301,3,720),(29302,3,720),(29306,2,45),(29343,5,3),(29344,5,3),(29345,5,3),(29400,10,205),(29424,5,1),(29427,3,15),(29445,3,900),(29484,3,3),(29508,11,140),(29509,11,140),(29510,11,140),(29511,11,600),(29514,6,10),(29535,3,1200),(29556,3,600),(29559,6,30),(29613,5,30),(29645,3,400),(29710,3,1200),(29735,3,1200),(29747,3,400),(29848,8,3),(29857,11,600),(29861,10,600),(29904,5,5),(29924,3,360),(29928,2,100),(29934,8,60),(29940,10,600),(29943,3,600),(29990,6,100),(29993,3,200),(29999,3,33),(30000,3,33),(30023,3,360),(30027,3,1100),(30030,3,1100),(30031,3,1100),(30032,3,1100),(30033,11,420),(30054,8,7),(30078,11,3),(30079,11,3),(30080,11,3),(30081,3,420),(30100,3,840),(30103,3,360),(30104,3,20),(30131,8,12),(30134,8,8),(30135,8,8),(30136,8,8),(30140,8,75),(30155,3,360),(30167,3,1100),(30173,11,420),(30176,3,360),(30180,11,3),(30186,3,5),(30187,3,5),(30188,3,5),(30196,3,420),(30197,3,420),(30198,3,420),(30202,5,1),(30217,3,840),(30220,3,100),(30241,3,5),(30242,3,5),(30243,3,5),(30244,3,5),(30245,3,5),(30246,3,5),(30247,3,480),(30253,3,300),(30256,3,480),(30259,3,480),(30268,11,120),(30280,3,720),(30283,11,10),(30289,3,480),(30295,11,600),(30298,11,600),(30299,11,600),(30300,11,600),(30301,11,6),(30307,3,12),(30311,11,6),(30314,11,6),(30315,11,6),(30316,11,6),(30338,3,720),(30344,11,720),(30347,11,120),(30378,3,360),(30433,11,10),(30436,11,10),(30463,11,5),(30603,11,270),(30604,11,270),(30605,11,270),(30606,11,270),(30707,50,0),(30739,50,3600),(30825,50,0),(30826,50,0),(30827,50,0),(30831,50,0),(30832,50,0),(30833,50,0),(30839,50,0),(30840,50,0),(30841,50,0),(30842,50,0),(30843,50,0),(30890,50,0),(31030,50,0),(31035,50,0),(31049,11,200),(31051,11,200),(31053,11,200),(31055,11,200),(31072,50,1),(31073,50,1),(31084,50,1),(31085,50,1),(31098,50,0),(31151,50,200),(31152,50,200),(31157,50,0),(31485,11,200),(31487,11,200),(31489,11,200),(31491,11,200),(31501,50,0),(31549,50,9999),(31551,50,0),(31552,50,0),(31553,50,0),(31554,50,0),(31555,50,0),(31566,50,0),(31567,50,0),(31568,50,0),(31634,11,50),(31635,11,50),(31636,11,50),(31856,50,0),(31882,11,60),(31902,50,0),(31906,50,0),(31907,50,0),(31908,50,0),(31909,50,0),(31910,50,0),(31911,50,0),(31918,11,30),(31919,50,0),(31925,11,4),(31929,11,4),(31931,11,4),(31932,11,4),(31934,11,6),(31938,11,6),(31939,11,1),(31940,11,6),(31942,11,4),(31944,11,4),(31946,11,3),(31949,11,3),(31950,11,2),(31951,11,1),(31952,11,1),(31953,11,4),(31954,11,4),(31955,11,4),(31956,11,4),(31957,11,4),(31958,11,4),(31959,11,4),(31960,11,4),(31971,11,3),(31972,11,2),(31973,11,20),(31974,11,30),(31975,11,4),(31976,11,4),(31978,11,30),(31979,11,20),(31980,11,20),(31981,11,20),(31985,11,1),(31986,11,20),(31989,11,2),(31992,11,3),(31994,11,3),(31996,11,4),(31997,11,4),(31998,11,3),(32000,11,2),(32003,11,30),(32079,50,0),(32080,50,0),(32081,50,0),(32082,50,0),(32083,50,0),(32107,50,10000),(32145,50,0),(32146,50,0),(32147,50,0),(32201,3600,600),(32202,3600,600),(32203,3600,600),(32204,11,30),(32207,11,10),(32208,11,4),(32210,11,6),(32211,11,4),(32213,11,6),(32216,11,6),(32217,11,2),(32218,11,6),(32220,11,2),(32222,11,4),(32223,11,3),(32227,11,1),(32228,11,1),(32229,11,5),(32230,11,5),(32231,11,5),(32232,11,5),(32233,11,5),(32234,11,5),(32235,11,5),(32236,11,5),(32247,11,10),(32253,11,2),(32256,11,1),(32260,11,3),(32261,11,4),(32262,11,3),(32264,11,4),(32268,50,0),(32272,50,0),(32273,50,0),(32274,50,0),(32275,50,0),(32276,50,0),(32277,50,0),(32278,50,0),(32279,50,0),(32280,50,0),(32281,50,0),(32282,50,0),(32283,50,0),(32284,50,0),(32285,50,0),(32286,50,0),(32287,50,0),(32288,50,0),(32289,50,0),(32290,50,0),(32291,50,0),(32292,50,0),(32293,50,0),(32294,50,0),(32295,50,0),(32296,50,0),(32374,50,0),(32375,50,0),(32376,11,100),(32403,50,0),(32541,50,5000),(32596,50,0),(32597,50,0),(32598,50,0),(32870,11,2040),(32890,7,3),(32907,11,100),(32911,50,0),(32912,50,0),(32913,50,0),(32914,50,0),(32915,50,0),(32916,50,0),(32917,50,0),(32918,50,0),(32919,50,0),(32920,50,0),(32921,50,0),(32922,50,0),(32923,50,0),(32924,50,0),(32925,50,0),(32926,50,0),(32942,11,3),(32994,50,1),(33010,50,1),(33055,11,300),(33064,4,25),(33091,11,3),(33092,50,1),(33100,11,300),(33119,11,270),(33120,11,270),(33121,11,270),(33122,11,270),(33201,11,100),(33202,11,100),(33222,50,1),(33532,50,0),(33664,50,1),(33665,50,1),(33666,50,1),(33667,50,1),(33668,50,1),(33671,50,1),(33684,50,1),(33685,50,1),(33686,50,1),(33687,50,1),(33688,50,1),(33711,11,300),(33716,50,1),(33717,50,1),(33718,50,1),(33719,50,1),(33720,50,1),(33721,50,1),(33722,50,1),(33736,50,0),(33739,50,0),(33771,11,600),(33785,50,0),(33786,50,0),(33787,50,0),(33788,50,0),(33789,50,0),(33790,50,0),(33791,50,0),(33792,50,0),(33793,50,0),(33794,50,0),(33795,50,0),(33796,50,0),(33797,50,0),(33798,50,0),(33799,50,0),(33800,50,0),(33801,50,0),(33802,50,0),(33803,50,0),(33804,50,0),(33805,50,0),(33806,50,0),(33807,50,0),(33808,50,0),(33809,50,0),(33810,50,0),(33811,50,0),(33812,50,0),(33813,50,0),(33814,50,0),(33815,50,0),(33816,50,0),(33817,50,0),(33818,50,0),(33819,50,0),(33820,50,0),(33821,50,0),(33822,50,0),(33823,50,0),(33824,50,0),(33825,50,0),(33826,50,0),(33827,50,0),(33828,50,0),(33829,50,0),(33830,50,0),(33831,50,0),(33832,50,0),(33833,50,0),(33834,50,0),(33835,50,0),(33836,50,0),(33837,50,0),(33838,50,0),(33839,50,0),(33840,50,0),(33841,50,0),(33842,50,0),(33843,50,0),(33844,50,0),(33845,50,0),(33846,50,0),(33847,50,0),(33848,50,0),(33849,50,0),(33850,50,0),(33851,50,0),(33852,50,0),(33853,50,0),(33854,50,0),(33855,50,0),(33856,50,0),(33857,50,0),(33858,50,0),(33859,50,0),(33860,50,0),(33861,50,0),(33862,50,0),(33863,50,0),(33864,50,0),(33865,50,0),(33866,50,0),(33867,50,0),(33868,50,0),(33869,50,0),(33870,50,0),(33871,50,0),(33872,50,0),(33873,50,0),(33874,50,0),(33875,50,0),(33876,50,0),(33877,50,0),(33878,50,0),(33879,50,0),(33880,50,0),(33881,50,0),(33882,50,0),(33883,50,0),(33884,50,0),(33924,11,100),(33984,50,1),(33998,50,0),(33999,3,360),(34000,11,9),(34003,11,20),(34006,11,3),(34009,11,10),(34012,11,5),(34013,11,5),(34014,11,5),(34045,11,30),(34060,11,10),(34079,3,720),(34094,8,5),(34097,3,720),(34098,3,720),(34099,3,720),(34106,3,720),(34107,3,720),(34108,3,720),(34109,3,1),(34112,3,720),(34124,3,720),(34125,3,720),(34126,3,720),(34127,8,6),(34130,3,720),(34145,3,1500),(34146,3,1500),(34147,3,1500),(34154,3,1),(34157,2,20),(34178,3,720),(34184,2,3),(34196,3,720),(34223,3,1500),(34226,3,1),(34229,3,10),(34244,3,1),(34259,3,1500),(34262,3,720),(34268,8,10),(34274,3,8),(34287,2,45),(34296,8,10),(34320,3,25),(34344,8,4),(34347,3,300),(34371,3,300),(34386,3,1440),(34404,15,50),(34405,15,50),(34406,15,50),(34413,3,800),(34419,3,10),(34446,3,720),(34458,3,300),(34461,3,20),(34500,2,7),(34509,11,210),(34521,10,210),(34524,3,600),(34533,3,210),(34539,3,600),(34540,3,600),(34541,3,600),(34551,10,15),(34552,10,15),(34553,10,15),(34563,2,7),(34566,3,800),(34567,3,800),(34568,3,800),(34569,3,1950),(34570,3,1950),(34571,3,1950),(34582,3,600),(34588,3,1950),(34589,3,1950),(34590,3,1950),(34594,3,600),(34595,3,600),(34596,3,600),(34609,3,600),(34615,11,30),(34621,3,1500),(34622,3,1500),(34623,3,1500),(34627,3,2000),(34628,3,2000),(34629,3,2000),(34630,10,10),(34631,10,10),(34632,10,10),(34663,3,600),(34672,11,600),(34708,3,600),(34717,11,600),(34720,3,600),(34739,11,600),(34742,3,1500),(34769,2,50),(34837,3,25),(34862,2,6),(34865,6,4),(34866,6,4),(34867,6,4),(34878,1,1),(34879,1,1),(34880,1,1),(34881,10,360),(34884,3,1000),(34893,2,3),(34896,2,3),(34905,11,3),(34906,11,3),(34907,11,3),(34914,3,1950),(34915,3,1950),(34916,3,1950),(34920,11,3),(34921,11,3),(34922,11,3),(34962,2,45),(34983,10,360),(34986,3,1000),(34992,3,1950),(34993,3,1950),(34994,3,1950),(35025,11,30),(35026,11,30),(35027,11,30),(35053,11,10),(35054,11,10),(35055,11,10),(35059,11,3),(35062,11,20),(35071,11,5),(35080,11,5),(35081,11,5),(35082,11,5),(35086,11,5),(35089,11,4),(35095,11,720),(35096,11,720),(35097,11,720),(35098,11,5),(35099,11,5),(35100,11,5),(35104,11,10),(35113,5,3),(35119,5,3),(35122,5,4),(35131,7,5),(35146,5,3),(35152,5,3),(35155,5,3),(35158,5,3),(35164,5,3),(35170,5,3),(35173,5,3),(35188,5,3),(35191,5,0),(35215,5,3),(35221,8,5),(35266,11,5),(35287,11,100),(35324,11,10),(35327,11,10),(35333,11,10),(35336,11,5),(35351,3,5),(35357,5,1),(35360,3,720),(35361,3,720),(35362,3,720),(35363,3,720),(35364,3,720),(35365,3,720),(35369,2,45),(35391,5,3),(35392,5,3),(35393,5,3),(35439,10,205),(35457,5,1),(35460,3,15),(35508,3,900),(35553,11,140),(35554,11,140),(35555,11,140),(35556,11,600),(35559,6,10),(35580,3,1200),(35601,3,600),(35604,6,30),(35658,5,30),(35683,3,3),(35698,3,360),(35701,3,360),(35716,3,400),(35783,3,1200),(35818,3,400),(35885,3,1200),(35911,8,3),(35920,11,600),(35924,10,600),(35952,5,5),(35978,3,360),(35982,2,100),(35988,10,600),(35991,3,600),(36043,3,200),(36052,3,360),(36061,8,60),(36070,6,100),(36089,3,1100),(36092,3,1100),(36093,3,1100),(36094,3,1100),(36095,11,420),(36116,8,7),(36137,3,420),(36156,3,840),(36159,3,360),(36160,3,20),(36187,8,12),(36190,8,8),(36194,8,75),(36209,3,360),(36215,3,1100),(36221,11,420),(36224,3,360),(36228,11,3),(36234,3,5),(36235,3,5),(36236,3,5),(36244,3,420),(36245,3,420),(36246,3,420),(36250,5,1),(36265,3,840),(36268,3,100),(36286,11,3),(36287,11,3),(36288,11,3),(36295,3,10),(36322,3,5),(36323,3,5),(36324,3,5),(36325,3,5),(36326,3,5),(36327,3,5),(36328,3,300),(36331,3,480),(36334,3,480),(36343,11,120),(36355,3,720),(36356,3,720),(36357,3,720),(36358,11,10),(36364,3,480),(36370,11,600),(36371,11,600),(36372,11,600),(36373,11,600),(36374,11,600),(36375,11,600),(36376,11,6),(36386,11,6),(36389,11,6),(36390,11,6),(36391,11,6),(36413,3,720),(36414,3,720),(36415,3,720),(36419,11,720),(36420,11,720),(36421,11,720),(36422,11,120),(36435,3,360),(36456,3,480),(36465,3,720),(36529,11,5),(36547,11,10),(36550,11,5),(36551,11,5),(36552,11,5),(37002,50,0),(37003,50,0),(37004,50,0),(37008,50,0),(37009,50,0),(37010,50,0),(37199,50,0),(37202,50,0),(37235,50,0),(37248,50,0),(37553,50,0),(37554,50,0),(37605,50,1),(37610,50,1),(37622,50,1),(37702,50,0),(37703,50,0),(37704,50,0),(37705,50,0),(37706,50,0),(37713,50,0),(37727,50,0),(37753,50,0),(37754,50,0),(37755,50,0),(37756,50,0),(37757,50,0),(37758,50,0),(37759,50,0),(37760,50,0),(37761,50,0),(37762,50,0),(37763,50,0),(37764,50,0),(37765,50,0),(37766,50,0),(37767,50,0),(37768,50,0),(37769,50,0),(37770,50,0),(37771,50,0),(37772,50,0),(37773,50,0),(37774,50,0),(37775,50,0),(37776,50,0),(37777,50,0),(37779,50,0),(37780,50,0),(37821,50,1),(37823,50,0),(37837,50,0),(37863,50,0),(37869,3,360),(37879,50,0),(37881,50,0),(37888,50,0),(37915,50,0),(37916,50,1),(37925,11,300),(37926,11,300),(37927,11,300),(37928,11,300),(37931,50,0),(37932,50,0),(37959,50,0),(37960,50,0),(37961,50,0),(37963,50,0),(37965,50,0),(37966,50,0),(37967,50,0),(37968,50,0),(37969,50,0),(37970,50,0),(37971,50,0),(37972,50,0),(37973,50,0),(37974,3,360),(37975,3,360),(37976,3,360),(37986,11,1650),(38160,11,30),(38167,11,5),(38218,50,0),(38301,11,300),(38302,11,300),(38303,11,300),(38304,11,10),(38305,11,10),(38306,11,10),(38308,11,20),(38309,11,20),(38310,11,20),(38311,11,300),(38313,11,300),(38314,11,300),(38315,11,300),(38316,11,2),(38317,11,2),(38318,11,2),(38332,11,5),(38339,11,5),(38357,50,0),(38358,50,0),(38368,50,0),(38407,11,140),(38422,11,140),(38423,11,140),(38424,11,140),(38425,11,300),(38426,11,300),(38427,11,300),(38431,50,1),(38496,50,1),(38497,50,1),(38549,50,0),(38550,50,0),(38567,50,0),(38573,50,0),(38578,50,0),(38597,11,420),(38603,11,20),(38634,11,360),(38639,11,270),(38640,11,270),(38657,50,0),(38658,50,0),(38668,50,0),(38687,11,2),(38688,11,3),(38689,11,3),(38690,11,3),(38692,11,1),(38694,50,1),(38695,11,1),(38697,11,3),(38698,11,1),(38703,50,0),(38704,50,0),(38705,50,0),(38706,50,0),(38707,50,0),(38708,50,0),(38709,50,0),(38710,50,0),(38725,50,0),(38726,11,3),(38727,11,3),(38728,11,3),(38731,11,3),(38742,11,1),(38745,50,1),(38754,50,0),(38783,11,4),(38784,50,1),(38786,11,2),(38787,11,1650),(38789,11,1650),(38811,3,360),(38842,11,1650),(38843,11,1650),(38846,11,30),(38861,11,1650),(38862,11,1650),(38871,11,270),(38872,11,270),(38873,11,270),(38874,11,270),(38936,11,600),(38937,11,600),(38940,11,1),(38941,11,1),(38944,11,600),(38945,11,600),(38946,11,5),(38947,11,5),(38948,11,5),(38949,11,5),(38951,11,20),(38989,11,1),(38992,11,4),(38993,50,0),(38997,11,20),(38998,11,1),(38999,50,1),(39000,11,2),(39001,11,2),(39004,50,0),(39024,50,0),(39025,50,0),(39026,50,0),(39027,50,0),(39028,50,1),(39029,11,1650),(39031,11,1),(39041,11,3),(39044,11,1),(39045,11,3),(39047,50,0),(39100,50,0),(39101,50,0),(39104,50,0),(39207,11,300),(39208,11,300),(39209,11,300),(39210,11,300),(39211,11,300),(39212,11,300),(39213,11,300),(39214,11,300),(39215,11,300),(39216,11,300),(39217,11,300),(39218,11,300),(39219,11,300),(39220,11,300),(39221,11,300),(39222,11,300),(39223,11,300),(39232,11,150),(39244,11,1650),(39247,11,150),(39251,50,1),(39252,11,300),(39282,3600,600),(39283,3600,600),(39284,3600,600),(39285,3600,600),(39286,3600,600),(39287,3600,600),(39288,3600,600),(39289,3600,600),(39290,3600,600),(39291,3600,600),(39292,3600,600),(39293,3600,600),(39294,11,3),(39296,50,0),(39297,50,0),(39298,50,0),(39303,50,1),(39483,50,0),(39484,50,0),(39494,50,0),(39520,11,270),(39565,50,0),(39639,50,0),(39659,11,1650),(39662,11,1650),(39693,50,0),(39702,50,0),(39814,11,270),(39815,11,270),(39816,11,270),(39817,11,270),(39818,11,270),(39819,11,270),(39851,50,0),(39852,50,0),(39853,50,0),(39854,50,0),(39855,3,360),(39911,11,1650),(39950,50,0),(39951,50,0),(39952,50,0),(39953,50,0),(39954,50,0),(39955,50,0),(39956,50,0),(39957,50,0),(39958,50,0),(39959,50,0),(39960,50,0),(39961,50,0),(39962,50,0),(39963,50,0),(39964,50,0),(40006,11,30),(40009,11,5),(40010,11,5),(40011,11,5),(40021,11,10),(40022,11,10),(40023,11,10),(40024,11,5),(40025,11,5),(40026,11,5),(40030,11,10),(40031,11,10),(40032,11,10),(40036,11,50),(40111,3,1),(40114,3,1),(40117,10,360),(40123,11,20),(40136,3,600),(40160,3,1),(40161,3,1),(40162,3,1),(40193,3,1),(40196,3,50),(40202,3,9),(40223,11,2),(40235,50,50),(40239,11,10),(40240,11,10),(40241,11,10),(40245,11,12),(40248,11,18),(40249,11,18),(40250,11,18),(40254,5,3),(40257,5,3),(40324,11,60),(40330,11,600),(40342,11,1),(40363,3,900),(40366,5,2),(40390,11,720),(40426,3,3),(40444,3,600),(40486,3,360),(40489,3,360),(40492,11,600),(40519,11,3),(40522,11,1),(40525,3,720),(40531,3,1),(40807,11,30),(40810,11,60),(40833,11,60),(40843,11,60),(40919,50,9999),(40936,11,20),(40947,11,300),(40948,11,300),(40949,11,300),(40950,11,300),(40951,11,300),(40952,11,300),(41019,11,90),(41033,50,10000),(42046,50,0),(42049,50,0),(42050,50,0),(42067,50,0),(42068,50,0),(42069,50,0),(42070,50,0),(42073,50,0),(42075,50,0),(42078,50,0),(42080,50,0),(42083,50,1),(42118,50,0),(42190,50,0),(42191,50,0),(42192,50,0),(42200,50,0),(42221,50,0),(42222,50,0),(42223,50,0),(42227,50,0),(42282,3600,600),(42528,50,0),(42529,50,0),(42530,50,0),(42531,50,0),(42532,50,0),(42533,50,0),(42534,50,0),(42535,50,0),(42536,50,0),(42537,50,0),(42538,50,0),(42539,50,0),(42540,50,0),(42541,50,0),(42542,50,0),(42543,50,0),(42544,50,0),(42545,50,0),(42546,50,0),(42547,50,0),(42548,50,0),(42549,50,0),(42550,50,0),(42551,50,0),(42552,50,0),(42553,50,0),(42554,50,0),(42555,50,0),(42556,50,0),(42557,50,0),(42558,50,0),(42559,50,0),(42560,50,0),(42561,50,0),(42562,50,0),(42563,50,0),(42564,50,0),(42565,50,0),(42566,50,0),(42567,50,0),(42568,50,0),(42569,50,0),(42570,50,0),(42571,50,0),(42572,50,0),(42573,50,0),(42574,50,0),(42575,50,0),(42576,50,0),(42577,50,0),(42578,50,0),(42579,50,0),(42580,50,0),(42581,50,0),(42582,50,0),(42583,50,0),(42584,50,0),(42585,50,0),(42586,50,0),(42587,50,0),(42588,50,0),(42589,50,0),(42590,50,0),(42591,50,0),(42592,50,0),(42593,50,0),(42594,50,0),(42595,50,0),(42596,50,0),(42597,50,0),(42598,50,0),(42599,50,0),(42600,50,0),(42601,50,0),(42602,50,0),(43576,5,3),(43577,5,3),(43578,5,3),(43579,5,3),(43740,11,270),(43741,11,270),(43742,11,270),(43743,11,270),(43816,11,270),(43817,11,270),(43818,11,270),(43819,11,270),(44216,5,3),(44217,5,3),(44218,5,3),(44219,5,3),(44304,11,600),(44305,11,600),(44306,11,600),(44307,11,600),(44312,11,10),(44313,11,10),(44314,11,10),(44315,11,10);
UPDATE spells_new s JOIN aotv4_native_dur n ON n.id = s.id
   SET s.buffdurationformula = n.f, s.buffduration = n.d;
DROP TEMPORARY TABLE IF EXISTS aotv4_native_dur;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 22,
		.description = "2026_08_06_aotv4_delve_tasks_any_zone",
		// The delve map pool grew from 6 DoN zones to 6 DoN + 34 LDoN, and the map is now drawn at
		// RANDOM per run rather than being fixed per rung. A delve task therefore cannot be bound to
		// one zone any more -- its kill activities have to credit wherever the run happens to land.
		//
		// ⚠️⚠️ AN EMPTY `zones` MEANS "ANY ZONE" -- that is the mechanism, not a side effect.
		// TaskActivity::CheckZone (common/tasks.h) opens with
		//     if (zone_ids.empty()) { return true; }
		// exactly mirroring the empty-npc_match_list rule already recorded for these tasks. So this
		// needs NO new task rows for the 34 LDoN zones; without it we would need 34 zones x 6 modes.
		// ⚠️ `zones` is a `;`-separated list, but the column is varchar(64) -- about 15 zone ids --
		// so listing every delve zone explicitly is not an option even if it were preferable.
		//
		// ⚠️⚠️ THIS IS A REAL WIDENING AND IS ACCEPTED KNOWINGLY. A delve kill activity will now
		// credit anywhere, so the ONLY thing keeping it inside the dungeon is the run guard:
		// aotv4_dungeon.M.on_enter_zone fails the run the moment you are somewhere other than your
		// own instance, and every teardown path calls FailTask / RemoveTaskByTaskID. The second,
		// independent guard the zone column used to provide is gone -- if that hook ever stops firing
		// on some exit path, open-world kills would credit a delve task with nothing to catch it.
		// 📌 Scoped to the delve task band only (2000300-2000399), never task_activities at large.
		.check       = "SELECT taskid FROM task_activities WHERE taskid BETWEEN 2000300 AND 2000399 AND zones <> '' LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE task_activities SET zones = '' WHERE taskid BETWEEN 2000300 AND 2000399;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 23,
		.description = "2026_08_06_aotv4_fixed_creation_stats",
		// Character creation is PRE-ALLOCATED: every stat starts at 75, your RACE adds +15 to one stat
		// and +10 to another, and your CLASS does the same. 256 rows, one per race/class pair, ids
		// 2000-2255. Every combination totals 575 (525 + 50 bonus), so no pairing is short-changed.
		//
		// ⚠️⚠️ IT ALL GOES IN `base_*` AND EVERY `alloc_*` IS ZERO. That is the entire point, not a
		// stylistic choice: `alloc_*` is the DISTRIBUTABLE pool, so a point placed there is a point the
		// player can move somewhere else on the creation screen. Putting the bonus in `base_*` fixes
		// it. Anyone "tidying" these into alloc later hands stat allocation back to the player.
		//
		// ⚠️⚠️ ONE ALLOCATION ROW PER RACE/CLASS PAIR -- THE EXISTING ROWS COULD NOT BE EDITED IN
		// PLACE. There were 107 allocation rows serving 1,781 combinations, and 19 of those rows were
		// SHARED by more than one race/class pair; updating a shared row would have silently rewritten
		// the stats of whatever other pair pointed at it. Hence a fresh row per pair and a repoint.
		//
		// ⚠️ A stat named by BOTH your race and your class STACKS -- Ogre (STA+15/STR+10) Berserker
		// (STR+15/STA+10) is 100 STR and 100 STA against 75 everywhere else, where a spread pairing
		// like Human Enchanter puts its 50 points across four stats. Deliberate: the doubled-up
		// combinations are meant to be sharper.
		//
		// ⚠️ Idempotent, and the check clears cleanly because the DB's race/class set is EXACTLY the
		// 16 x 16 this covers -- verified: no combination row uses a race or class outside it, so no
		// row is left pointing outside the band to re-trigger this forever.
		// 📌 Client::AoTv4ApplyCreationStats reads base_* + alloc_*, so reforging picks these up with
		// no code change. With alloc_* now always 0 that sum is just base_* -- do NOT "simplify" it to
		// read base_* alone, or the next non-zero allocation row silently loses its points again.
		.check       = "SELECT race FROM char_create_combinations WHERE allocation_id NOT BETWEEN 2000 AND 2255 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_cc;
CREATE TEMPORARY TABLE aotv4_cc (
  race INT, class INT, id INT PRIMARY KEY,
  s INT, t INT, d INT, a INT, i INT, w INT, c INT
);
INSERT INTO aotv4_cc (race,class,id,s,t,d,a,i,w,c) VALUES
(1,1,2000,85,90,75,75,85,75,90),(1,2,2001,75,75,75,75,85,90,100),(1,3,2002,75,90,75,75,85,85,90),(1,4,2003,75,75,90,85,85,75,90),(1,5,2004,75,90,75,75,95,75,90),(1,6,2005,75,75,75,75,95,90,90),(1,7,2006,85,75,75,90,85,75,90),(1,8,2007,75,75,85,75,85,75,105),(1,9,2008,85,75,90,75,85,75,90),(1,10,2009,75,85,75,75,85,90,90),(1,11,2010,75,75,75,85,100,75,90),(1,12,2011,75,75,85,75,100,75,90),(1,13,2012,75,75,75,75,100,75,100),(1,14,2013,75,75,75,75,95,75,105),(1,15,2014,75,75,75,90,85,85,90),(1,16,2015,90,85,75,75,85,75,90),(2,1,2016,85,105,75,75,75,85,75),(2,2,2017,75,90,75,75,75,100,85),(2,3,2018,75,105,75,75,75,95,75),(2,4,2019,75,90,90,85,75,85,75),(2,5,2020,75,105,75,75,85,85,75),(2,6,2021,75,90,75,75,85,100,75),(2,7,2022,85,90,75,90,75,85,75),(2,8,2023,75,90,85,75,75,85,90),(2,9,2024,85,90,90,75,75,85,75),(2,10,2025,75,100,75,75,75,100,75),(2,11,2026,75,90,75,85,90,85,75),(2,12,2027,75,90,85,75,90,85,75),(2,13,2028,75,90,75,75,90,85,85),(2,14,2029,75,90,75,75,85,85,90),(2,15,2030,75,90,75,90,75,95,75),(2,16,2031,90,100,75,75,75,85,75),(3,1,2032,85,90,75,75,90,85,75),(3,2,2033,75,75,75,75,90,100,85),(3,3,2034,75,90,75,75,90,95,75),(3,4,2035,75,75,90,85,90,85,75),(3,5,2036,75,90,75,75,100,85,75),(3,6,2037,75,75,75,75,100,100,75),(3,7,2038,85,75,75,90,90,85,75),(3,8,2039,75,75,85,75,90,85,90),(3,9,2040,85,75,90,75,90,85,75),(3,10,2041,75,85,75,75,90,100,75),(3,11,2042,75,75,75,85,105,85,75),(3,12,2043,75,75,85,75,105,85,75),(3,13,2044,75,75,75,75,105,85,85),(3,14,2045,75,75,75,75,100,85,90),(3,15,2046,75,75,75,90,90,95,75),(3,16,2047,90,85,75,75,90,85,75),(4,1,2048,85,90,90,85,75,75,75),(4,2,2049,75,75,90,85,75,90,85),(4,3,2050,75,90,90,85,75,85,75),(4,4,2051,75,75,105,95,75,75,75),(4,5,2052,75,90,90,85,85,75,75),(4,6,2053,75,75,90,85,85,90,75),(4,7,2054,85,75,90,100,75,75,75),(4,8,2055,75,75,100,85,75,75,90),(4,9,2056,85,75,105,85,75,75,75),(4,10,2057,75,85,90,85,75,90,75),(4,11,2058,75,75,90,95,90,75,75),(4,12,2059,75,75,100,85,90,75,75),(4,13,2060,75,75,90,85,90,75,85),(4,14,2061,75,75,90,85,85,75,90),(4,15,2062,75,75,90,100,75,85,75),(4,16,2063,90,85,90,85,75,75,75),(5,1,2064,85,90,75,75,75,90,85),(5,2,2065,75,75,75,75,75,105,95),(5,3,2066,75,90,75,75,75,100,85),(5,4,2067,75,75,90,85,75,90,85),(5,5,2068,75,90,75,75,85,90,85),(5,6,2069,75,75,75,75,85,105,85),(5,7,2070,85,75,75,90,75,90,85),(5,8,2071,75,75,85,75,75,90,100),(5,9,2072,85,75,90,75,75,90,85),(5,10,2073,75,85,75,75,75,105,85),(5,11,2074,75,75,75,85,90,90,85),(5,12,2075,75,75,85,75,90,90,85),(5,13,2076,75,75,75,75,90,90,95),(5,14,2077,75,75,75,75,85,90,100),(5,15,2078,75,75,75,90,75,100,85),(5,16,2079,90,85,75,75,75,90,85),(6,1,2080,85,90,90,75,85,75,75),(6,2,2081,75,75,90,75,85,90,85),(6,3,2082,75,90,90,75,85,85,75),(6,4,2083,75,75,105,85,85,75,75),(6,5,2084,75,90,90,75,95,75,75),(6,6,2085,75,75,90,75,95,90,75),(6,7,2086,85,75,90,90,85,75,75),(6,8,2087,75,75,100,75,85,75,90),(6,9,2088,85,75,105,75,85,75,75),(6,10,2089,75,85,90,75,85,90,75),(6,11,2090,75,75,90,85,100,75,75),(6,12,2091,75,75,100,75,100,75,75),(6,13,2092,75,75,90,75,100,75,85),(6,14,2093,75,75,90,75,95,75,90),(6,15,2094,75,75,90,90,85,85,75),(6,16,2095,90,85,90,75,85,75,75),(7,1,2096,85,90,85,90,75,75,75),(7,2,2097,75,75,85,90,75,90,85),(7,3,2098,75,90,85,90,75,85,75),(7,4,2099,75,75,100,100,75,75,75),(7,5,2100,75,90,85,90,85,75,75),(7,6,2101,75,75,85,90,85,90,75),(7,7,2102,85,75,85,105,75,75,75),(7,8,2103,75,75,95,90,75,75,90),(7,9,2104,85,75,100,90,75,75,75),(7,10,2105,75,85,85,90,75,90,75),(7,11,2106,75,75,85,100,90,75,75),(7,12,2107,75,75,95,90,90,75,75),(7,13,2108,75,75,85,90,90,75,85),(7,14,2109,75,75,85,90,85,75,90),(7,15,2110,75,75,85,105,75,85,75),(7,16,2111,90,85,85,90,75,75,75),(8,1,2112,85,105,75,85,75,75,75),(8,2,2113,75,90,75,85,75,90,85),(8,3,2114,75,105,75,85,75,85,75),(8,4,2115,75,90,90,95,75,75,75),(8,5,2116,75,105,75,85,85,75,75),(8,6,2117,75,90,75,85,85,90,75),(8,7,2118,85,90,75,100,75,75,75),(8,8,2119,75,90,85,85,75,75,90),(8,9,2120,85,90,90,85,75,75,75),(8,10,2121,75,100,75,85,75,90,75),(8,11,2122,75,90,75,95,90,75,75),(8,12,2123,75,90,85,85,90,75,75),(8,13,2124,75,90,75,85,90,75,85),(8,14,2125,75,90,75,85,85,75,90),(8,15,2126,75,90,75,100,75,85,75),(8,16,2127,90,100,75,85,75,75,75),(9,1,2128,100,100,75,75,75,75,75),(9,2,2129,90,85,75,75,75,90,85),(9,3,2130,90,100,75,75,75,85,75),(9,4,2131,90,85,90,85,75,75,75),(9,5,2132,90,100,75,75,85,75,75),(9,6,2133,90,85,75,75,85,90,75),(9,7,2134,100,85,75,90,75,75,75),(9,8,2135,90,85,85,75,75,75,90),(9,9,2136,100,85,90,75,75,75,75),(9,10,2137,90,95,75,75,75,90,75),(9,11,2138,90,85,75,85,90,75,75),(9,12,2139,90,85,85,75,90,75,75),(9,13,2140,90,85,75,75,90,75,85),(9,14,2141,90,85,75,75,85,75,90),(9,15,2142,90,85,75,90,75,85,75),(9,16,2143,105,95,75,75,75,75,75),(10,1,2144,95,105,75,75,75,75,75),(10,2,2145,85,90,75,75,75,90,85),(10,3,2146,85,105,75,75,75,85,75),(10,4,2147,85,90,90,85,75,75,75),(10,5,2148,85,105,75,75,85,75,75),(10,6,2149,85,90,75,75,85,90,75),(10,7,2150,95,90,75,90,75,75,75),(10,8,2151,85,90,85,75,75,75,90),(10,9,2152,95,90,90,75,75,75,75),(10,10,2153,85,100,75,75,75,90,75),(10,11,2154,85,90,75,85,90,75,75),(10,12,2155,85,90,85,75,90,75,75),(10,13,2156,85,90,75,75,90,75,85),(10,14,2157,85,90,75,75,85,75,90),(10,15,2158,85,90,75,90,75,85,75),(10,16,2159,100,100,75,75,75,75,75),(11,1,2160,85,90,75,90,75,75,85),(11,2,2161,75,75,75,90,75,90,95),(11,3,2162,75,90,75,90,75,85,85),(11,4,2163,75,75,90,100,75,75,85),(11,5,2164,75,90,75,90,85,75,85),(11,6,2165,75,75,75,90,85,90,85),(11,7,2166,85,75,75,105,75,75,85),(11,8,2167,75,75,85,90,75,75,100),(11,9,2168,85,75,90,90,75,75,85),(11,10,2169,75,85,75,90,75,90,85),(11,11,2170,75,75,75,100,90,75,85),(11,12,2171,75,75,85,90,90,75,85),(11,13,2172,75,75,75,90,90,75,95),(11,14,2173,75,75,75,90,85,75,100),(11,15,2174,75,75,75,105,75,85,85),(11,16,2175,90,85,75,90,75,75,85),(12,1,2176,85,90,90,75,85,75,75),(12,2,2177,75,75,90,75,85,90,85),(12,3,2178,75,90,90,75,85,85,75),(12,4,2179,75,75,105,85,85,75,75),(12,5,2180,75,90,90,75,95,75,75),(12,6,2181,75,75,90,75,95,90,75),(12,7,2182,85,75,90,90,85,75,75),(12,8,2183,75,75,100,75,85,75,90),(12,9,2184,85,75,105,75,85,75,75),(12,10,2185,75,85,90,75,85,90,75),(12,11,2186,75,75,90,85,100,75,75),(12,12,2187,75,75,100,75,100,75,75),(12,13,2188,75,75,90,75,100,75,85),(12,14,2189,75,75,90,75,95,75,90),(12,15,2190,75,75,90,90,85,85,75),(12,16,2191,90,85,90,75,85,75,75),(128,1,2192,100,100,75,75,75,75,75),(128,2,2193,90,85,75,75,75,90,85),(128,3,2194,90,100,75,75,75,85,75),(128,4,2195,90,85,90,85,75,75,75),(128,5,2196,90,100,75,75,85,75,75),(128,6,2197,90,85,75,75,85,90,75),(128,7,2198,100,85,75,90,75,75,75),(128,8,2199,90,85,85,75,75,75,90),(128,9,2200,100,85,90,75,75,75,75),(128,10,2201,90,95,75,75,75,90,75),(128,11,2202,90,85,75,85,90,75,75),(128,12,2203,90,85,85,75,90,75,75),(128,13,2204,90,85,75,75,90,75,85),(128,14,2205,90,85,75,75,85,75,90),(128,15,2206,90,85,75,90,75,85,75),(128,16,2207,105,95,75,75,75,75,75),(130,1,2208,85,90,85,75,75,75,90),(130,2,2209,75,75,85,75,75,90,100),(130,3,2210,75,90,85,75,75,85,90),(130,4,2211,75,75,100,85,75,75,90),(130,5,2212,75,90,85,75,85,75,90),(130,6,2213,75,75,85,75,85,90,90),(130,7,2214,85,75,85,90,75,75,90),(130,8,2215,75,75,95,75,75,75,105),(130,9,2216,85,75,100,75,75,75,90),(130,10,2217,75,85,85,75,75,90,90),(130,11,2218,75,75,85,85,90,75,90),(130,12,2219,75,75,95,75,90,75,90),(130,13,2220,75,75,85,75,90,75,100),(130,14,2221,75,75,85,75,85,75,105),(130,15,2222,75,75,85,90,75,85,90),(130,16,2223,90,85,85,75,75,75,90),(330,1,2224,85,90,75,85,90,75,75),(330,2,2225,75,75,75,85,90,90,85),(330,3,2226,75,90,75,85,90,85,75),(330,4,2227,75,75,90,95,90,75,75),(330,5,2228,75,90,75,85,100,75,75),(330,6,2229,75,75,75,85,100,90,75),(330,7,2230,85,75,75,100,90,75,75),(330,8,2231,75,75,85,85,90,75,90),(330,9,2232,85,75,90,85,90,75,75),(330,10,2233,75,85,75,85,90,90,75),(330,11,2234,75,75,75,95,105,75,75),(330,12,2235,75,75,85,85,105,75,75),(330,13,2236,75,75,75,85,105,75,85),(330,14,2237,75,75,75,85,100,75,90),(330,15,2238,75,75,75,100,90,85,75),(330,16,2239,90,85,75,85,90,75,75),(522,1,2240,100,90,85,75,75,75,75),(522,2,2241,90,75,85,75,75,90,85),(522,3,2242,90,90,85,75,75,85,75),(522,4,2243,90,75,100,85,75,75,75),(522,5,2244,90,90,85,75,85,75,75),(522,6,2245,90,75,85,75,85,90,75),(522,7,2246,100,75,85,90,75,75,75),(522,8,2247,90,75,95,75,75,75,90),(522,9,2248,100,75,100,75,75,75,75),(522,10,2249,90,85,85,75,75,90,75),(522,11,2250,90,75,85,85,90,75,75),(522,12,2251,90,75,95,75,90,75,75),(522,13,2252,90,75,85,75,90,75,85),(522,14,2253,90,75,85,75,85,75,90),(522,15,2254,90,75,85,90,75,85,75),(522,16,2255,105,85,85,75,75,75,75);

DELETE FROM char_create_point_allocations WHERE id BETWEEN 2000 AND 2255;

INSERT INTO char_create_point_allocations
  (id, base_str, base_sta, base_dex, base_agi, base_int, base_wis, base_cha,
       alloc_str, alloc_sta, alloc_dex, alloc_agi, alloc_int, alloc_wis, alloc_cha)
SELECT id, s, t, d, a, i, w, c, 0, 0, 0, 0, 0, 0, 0 FROM aotv4_cc;

UPDATE char_create_combinations cc
  JOIN aotv4_cc x ON x.race = cc.race AND x.class = cc.class
  SET cc.allocation_id = x.id;

DROP TEMPORARY TABLE IF EXISTS aotv4_cc;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 24,
		.description = "2026_08_06_aotv4_remove_soulbinders",
		// Everyone starts in The Resplendent Temple (729) and is BOUND there, so death returns them to
		// the hub. start_zones already delivers that at creation -- 1,441 rows covering every
		// (race, class, deity) triple, bind_id 0, bind -22/535/0 (custom/sql/aotv4_start_resplendent.sql).
		//
		// ⚠️⚠️ BUT IT WAS NOT ENFORCED -- 33 SOULBINDERS WERE SPAWNED ACROSS THE WORLD. Any of them
		// re-binds a player away from the hub, and from then on death drops them somewhere else. Found
		// because one of eight live characters was bound in The Bazaar rather than Resplendent, which
		// is what a soulbinder does, not a GM command. It also quietly undermines the same-zone
		// respawn fix (section 34), which is built around the hub bind being the ordinary case.
		// ⚠️ Bind Affinity is NOT a second leak: spell 35 is in the pool but blacklisted, and the other
		// six SPA-25 spells are not in the pool at all. Re-checked 2026-08-06. The soulbinders were the
		// only way in.
		//
		// ⚠️⚠️ SPAWN POINTS ARE DELETED, NOT THE NPCs. The npc_types rows survive, so a soulbinder can
		// be put back by re-adding a spawn2 row -- and the deleted rows are copied to
		// aotv4_soulbinder_spawn2_removed first, so "put it back exactly where it was" stays possible.
		// ⚠️ Safe because all 33 spawn groups are EXCLUSIVE to soulbinders -- verified that none also
		// contains an ordinary NPC, so deleting the point removes nothing else. Check that again before
		// widening this to any other NPC name.
		//
		// ⚠️ The bind reset is a ONE-TIME correction and does not need to repeat: with the soulbinders
		// gone and Bind Affinity blacklisted, there is no remaining way for a bind to drift. All five
		// slots are set, matching what creation writes.
		.check       = "SELECT s.id FROM spawn2 s JOIN spawnentry se ON se.spawngroupID = s.spawngroupID JOIN npc_types n ON n.id = se.npcID WHERE n.name LIKE '%oulbind%' LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_soulbinder_spawn2_removed LIKE spawn2;

INSERT IGNORE INTO aotv4_soulbinder_spawn2_removed
SELECT s.* FROM spawn2 s
  JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
  JOIN npc_types  n  ON n.id = se.npcID
 WHERE n.name LIKE '%oulbind%';

DELETE s FROM spawn2 s
  JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
  JOIN npc_types  n  ON n.id = se.npcID
 WHERE n.name LIKE '%oulbind%';

UPDATE character_bind
   SET zone_id = 729, instance_id = 0, x = -22, y = 535, z = 0, heading = 0
 WHERE zone_id <> 729;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 25,
		.description = "2026_08_06_aotv4_no_augment_loot",
		// Augments no longer drop as ordinary loot. The delve augment line (section 26) is the intended
		// augment source -- earned from the chest and upgraded four-to-one in the Refining Crucible --
		// and stock augments raining out of every loot table undercuts that whole progression.
		// 9,000 lootdrop_entries rows across 723 distinct augment items.
		//
		// ⚠️⚠️ IT DELETES FROM `lootdrop_entries`, NOT FROM `items` -- AND THAT DISTINCTION IS THE
		// WHOLE POINT. Deleting the item ROWS instead (DELETE i FROM items i JOIN lootdrop_entries ...)
		// destroys 723 item definitions, leaves those same 9,000 loot rows pointing at ids that no
		// longer exist, orphans any copy a player is already carrying, and takes at least one GEAR TIER
		// row with it (692412 Mythic Revered Legging Symbol of the Brood) -- which breaks the
		// Hallowed/Mythic pairing invariant section 10 depends on, the one that makes the loot code's
		// half-tiered fallback branch unreachable. Removing the loot ENTRY stops the drop and keeps the
		// item usable by quests, merchants, crafting and anyone already holding one.
		// 📌 Same pattern already used for the nine stock augments in the delve zones (section 26),
		// which were removed from lootdrop_entries after verifying they dropped nowhere else.
		//
		// ⚠️ OUR OWN BAND IS EXCLUDED (147500-148199) as a guard, not because it is currently needed:
		// the delve augments are placed on the chest by npc:AddItem and have ZERO lootdrop_entries rows
		// today. The exclusion is there so that if they are ever given a loot entry, this migration --
		// or a re-run of it -- cannot quietly strip the one augment source the game is built around.
		// 📌 Ink of the Lost (147921) sits in lootdrop_entries at 1.5 percent but carries augtype 0, so
		// the augtype filter never sees it. The band guard covers it a second time regardless.
		//
		// ⚠️ Removed rows are copied to aotv4_augment_lootdrop_removed first, so a specific augment can
		// be put back on its original table at its original chance.
		// ⚠️ lootdrop_entries is read at ZONE BOOT, not shared memory -- a zone restart applies this,
		// no ./shared_memory rebuild required.
		.check       = "SELECT lde.item_id FROM lootdrop_entries lde JOIN items i ON i.id = lde.item_id WHERE i.augtype > 0 AND i.id NOT BETWEEN 147500 AND 148199 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_augment_lootdrop_removed LIKE lootdrop_entries;

INSERT IGNORE INTO aotv4_augment_lootdrop_removed
SELECT lde.* FROM lootdrop_entries lde
  JOIN items i ON i.id = lde.item_id
 WHERE i.augtype > 0 AND i.id NOT BETWEEN 147500 AND 148199;

DELETE lde FROM lootdrop_entries lde
  JOIN items i ON i.id = lde.item_id
 WHERE i.augtype > 0 AND i.id NOT BETWEEN 147500 AND 148199;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 26,
		.description = "2026_08_06_aotv4_ink_currency_on_kill",
		// Ink of the Lost is granted as CURRENCY on a kill and no longer drops as an item, so the
		// global_loot entry that produced the item is removed. The grant lives in
		// global_npc.event_death_complete -> aotv4_spell_ranks_sys.grant_ink_on_kill, at the same 1.5
		// percent, paid to the killer.
		//
		// ⚠️⚠️ THIS DELETES A CLASS OF BUG RATHER THAN FIXING ONE. The item route needed three moving
		// parts to be correct, all of them recorded in section 29:
		//   * EVENT_LOOT CANNOT convert -- it fires at corpse.cpp:1740 and the item is not placed in
		//     the bags until :1843, so RemoveItem removed nothing and the player kept item AND currency
		//     ("you double loot ink of the lost ... had 7 before i looted that one").
		//   * Returning non-zero to suppress the loot is WORSE -- prevent_loot returns before the
		//     RemoveItem at :1865 that takes the item off the CORPSE, so it pays out on every click.
		//   * The working answer was a deferred one-second timer, a bag sweep, and a login backstop.
		// With no item there is nothing to place, remove, duplicate or strand.
		//
		// ⚠️ ONE ROLL PER CORPSE, PAID TO THE KILLER -- matching the old behaviour deliberately.
		// Section 31 keeps global loot SHARED under individual loot precisely because "a 1.5 percent
		// Ink of the Lost multiplied by group size is a different drop rate".
		//
		// ⚠️⚠️ THE ITEM ROW 147921 IS NOT DELETED, only its loot entry. An alternate currency is a
		// currency/item PAIR and the client reads the currency's name and icon from that item --
		// deleting it leaves the currency nameless in the window. Same reason Parchment Fragment
		// (147920) keeps its row despite never having dropped at all.
		// ⚠️ Removed rows are copied to aotv4_ink_lootdrop_removed so the drop can be restored.
		// 📌 Players carrying item copies from before this are migrated by absorb_ink, still called
		// from event_connect -- without that the roguelite wipe would destroy them.
		.check       = "SELECT item_id FROM lootdrop_entries WHERE item_id = 147921 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_ink_lootdrop_removed LIKE lootdrop_entries;
INSERT IGNORE INTO aotv4_ink_lootdrop_removed SELECT * FROM lootdrop_entries WHERE item_id = 147921;
DELETE FROM lootdrop_entries WHERE item_id = 147921;
)",
		.content_schema_update = false,
	},
};

// see struct definitions for what each field does
// struct ManifestEntry {
// 	int         version{};     // database version of the migration
// 	std::string description{}; // description of the migration ex: "add_new_table" or "add_index_to_table"
// 	std::string check{};       // query that checks against the condition
// 	std::string condition{};   // condition or "match_type" - Possible values [contains|match|missing|empty|not_empty]
// 	std::string match{};       // match field that is not always used, but works in conjunction with "condition" values [missing|match|contains]
// 	std::string sql{};         // the SQL DDL that gets ran when the condition is true
// };
