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
