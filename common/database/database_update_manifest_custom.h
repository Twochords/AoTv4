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

	ManifestEntry{
		.version     = 27,
		.description = "2026_08_06_aotv4_unlock_ldon_delve_zones",
		// The 33 LDoN dungeons used as delve maps sat in region 99 "Unused", so entering one answered
		// "You have not yet unlocked Unused." and the whole LDoN half of the ladder was unenterable.
		// The six DoN delve zones were already in region 0 "Always Available"; this brings the LDoN
		// ones into line.
		//
		// ⚠️⚠️ ONLY ZONES WITH NO ZONE LINES ARE UNLOCKED, AND THAT IS THE SAFETY ARGUMENT.
		// Every zone listed here has ZERO rows in zone_points targeting it -- there is no way to walk
		// into one, so making it "Always Available" grants no world access at all. It is reachable
		// only as a private delve instance, which the delve's own unlock ladder already gates.
		// ⚠️⚠️ **veksar IS DELIBERATELY EXCLUDED AND WAS REMOVED FROM THE DELVE POOL.** It is LDoN by
		// expansion but a REAL Kunark world zone: 2 zone_points lead to it and it is legitimately
		// assigned to the Cabilis region. Unlocking it would have opened Kunark travel to anyone who
		// had not earned Cabilis -- a genuine hole in region locking, in exchange for one more map out
		// of 34. If it is ever wanted back, exempt instanced moves from the gate instead
		// (Client::ProcessMovePC) rather than unlocking the zone.
		// 📌 Test before adding any future delve map: a zone with zone_points is world content, and
		// unlocking it is a travel change, not a delve change.
		//
		// ⚠️ Region 0 is "Always Available" and RegionManager::CanEnterZone treats an unmapped zone
		// (region_id 0) as unrestricted by design, so this is the same state as "no region".
		.check       = "SELECT zr.zone_id FROM zone_regions zr JOIN zone z ON z.zoneidnumber = zr.zone_id WHERE z.short_name IN ('guka','gukc','guke','gukf','gukh','mira','mirb','mirc','mird','mirg','mirj','mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj','ruja','rujd','rujf','rujg','ruji','rujj','taka','takb','takc','takd','take','takg') AND zr.region_id <> 0 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE zone_regions zr
  JOIN zone z ON z.zoneidnumber = zr.zone_id
   SET zr.region_id = 0
 WHERE z.short_name IN ('guka','gukc','guke','gukf','gukh','mira','mirb','mirc','mird','mirg','mirj','mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj','ruja','rujd','rujf','rujg','ruji','rujj','taka','takb','takc','takd','take','takg');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 28,
		.description = "2026_08_06_aotv4_fix_reward_chance_basis_points",
		// ⚠️⚠️ `custom_achievement_rewards`.`chance` IS IN BASIS POINTS (0-10000), NOT PERCENT.
		// QueueAchievementRewards gates on
		//     (r.chance >= 10000 OR FLOOR(RAND() * 10000) < r.chance)
		// (achievement_manager.cpp), so a row written as `chance = 100` meaning "100 percent" is read
		// as 100/10000 = **ONE PERCENT** and the reward is simply never queued on 99 completions out
		// of 100. Every stock reward type ships 10000; the two AoTv4 additions (section 32) shipped
		// 100 and were therefore near-dead from the day they landed:
		//   grant_aa 36 rows -- the tradeskill mastery AA ladders
		//   item     24 rows -- the tradeskill tools (147930+) and masks (147942+)
		//
		// ⚠️⚠️ THE FAILURE IS INVISIBLE FROM EVERY DIRECTION THAT MATTERS. The achievement completes,
		// the window shows it Done, and the detail pane still renders "Auto -- Fletching Mastery
		// rank 1" because that text comes from the DEFINITION, which is perfectly valid. Nothing is
		// logged, no reward row is written, and `#ach rewards` shows nothing to explain -- there is no
		// refusal to report because the reward was never queued in the first place. Reported from play
		// as "some of our tradeskill achievements are not giving the AAs".
		// 📌 Diagnose this class of bug by checking for the absence of a `custom_character_achievement_rewards`
		// ROW, not by reading result_text -- a queued-and-refused reward and a never-queued one look
		// identical in game and completely different in the database.
		//
		// ⚠️ The second statement BACKFILLS anyone who already completed one of these and got nothing.
		// Rows go in as status 0 (pending); `ClaimPendingRewards(client, 0, true)` sweeps every pending
		// auto-claim row on the next completion, and `#ach claim` forces it immediately. INSERT IGNORE
		// leans on uk_character_reward so it can never double-grant.
		// ⚠️ Scoped to reward_type IN ('grant_aa','item') -- those are exactly the AoTv4 rows. Do not
		// widen it to "any chance = 100": a genuine 1 percent drop chance is a legitimate value here.
		.check       = "SELECT id FROM custom_achievement_rewards WHERE chance = 100 AND reward_type IN ('grant_aa', 'item') LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE custom_achievement_rewards
   SET chance = 10000
 WHERE chance = 100 AND reward_type IN ('grant_aa', 'item');

INSERT IGNORE INTO custom_character_achievement_rewards
  (character_id, achievement_id, reward_definition_id, reward_type, reward_id,
   amount, auto_claim, tier, preview_text, data_text, status, completion_count, created_at)
SELECT ca.character_id, r.achievement_id, r.id, r.reward_type, r.reward_id,
       GREATEST(r.amount, 1), r.auto_claim, r.tier, r.preview_text, r.data_text, 0,
       ca.completion_count, UNIX_TIMESTAMP()
  FROM custom_achievement_rewards r
  JOIN custom_character_achievements ca ON ca.achievement_id = r.achievement_id
 WHERE r.enabled = 1 AND r.reward_type IN ('grant_aa', 'item');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 29,
		.description = "2026_08_06_aotv4_tradeskill_gear_percentages",
		// The tradeskill gear bonus becomes NATIVE `skillmod` (a percentage) instead of the flat
		// +20/+30 paid inside AoTv4TradeskillSkill, and gains a third piece:
		//     head  147930-147941   +5 percent   (skill 100 achievement)
		//     face  147942-147953  +10 percent   (skill 200)
		//     hands 147954-147965  +15 percent   (skill 300, NEW)
		//
		// ⚠️⚠️ THE FLAT BONUS WAS INVISIBLE, WHICH IS THE WHOLE REASON FOR THIS. It was paid in
		// tradeskills.cpp and so never reached Client::GetSkill, while the client's Skills window
		// renders the raw m_pp.skills -- so wearing the tool changed every combine roll and displayed
		// nothing. Reported from play as "this is not working or it would reflect on my skill sheet".
		// Native skillmod plus the OP_SkillUpdate change in Client::SetSkill makes the sheet honest.
		// ⚠️⚠️ THE C++ FLAT BONUSES ARE REMOVED IN THE SAME COMMIT. Applying this SQL against an older
		// zone binary DOUBLE-COUNTS every piece (native percentage plus the flat adder).
		//
		// ⚠️⚠️ THE MASK HAD TO BECOME WEARABLE. It was `slots = 0`, an inventory clicky, and
		// `AddItemBonuses` only ever walks EQUIPPED slots -- a skillmod on an unworn item contributes
		// exactly nothing. It moves to FACE (8), deliberately not head: section 32 records that the
		// head slot is the tool's, and two pieces competing for one slot is the thing that made the
		// original clicky design necessary. Its illusion click (44400+idx) is untouched and is now
		// purely cosmetic.
		//
		// ⚠️⚠️ `skillmod` DOES NOT STACK -- bonuses.cpp:451 keeps only the HIGHEST value per skill. The
		// three pieces are a progression, not an accumulation: all three worn is 15 percent, the same
		// as the hand piece alone. Do not "fix" this by raising the numbers to sum to something.
		// ⚠️ `skillmodmax` is forced to 0. GetSkill treats a non-zero max as a FLAT cap
		// (min(raw + max, raw * (100 + mod) / 100)), which would silently truncate the percentage.
		//
		// ⚠️⚠️ THE STRAY AUGMENT SOCKETS ARE CLEARED. Every row in the band carried
		// augslot1type 7 (General: Group) and augslot2type 21 (Special Ornamentation), inherited from
		// whatever stock item the original SQL cloned -- the same class as section 5's "cloning a stock
		// spell inherits its damage formula". The tell was that the MASKS had them too, and a slots=0
		// inventory clicky can never be ornamented. Cleared BEFORE the hand pieces are cloned so they
		// are not inherited again.
		//
		// 📌 `skillmodtype` is derived with ELT over ((id - 147930) MOD 12), which reproduces the exact
		// index order of AOTV4_TS_SKILLS in tradeskills.cpp. Keep the two in step -- section 32 records
		// that reordering silently gives blacksmiths the fishing bonus with no error anywhere.
		// ⚠️ The new rewards target achievement ids ARITHMETICALLY (400000 + skill * 1000 + 300, i.e.
		// 455300..469300), never a SELECT on (skill, required_count). That is deliberate: section 32
		// records that keying on skill+threshold also matches the Master Artisan aggregate
		// (470000-470999), which would land all twelve rewards on one achievement.
		// 📌 preview_text says "percent", not "%". A literal % is eaten as a printf token by the
		// Client::Message path (sections 5 and 14), rendering as garbage.
		// ⚠️ `items` is SHARED MEMORY: world down, ./shared_memory, restart. A migration alone is not
		// enough for any of this to be visible.
		.check       = "SELECT id FROM items WHERE id BETWEEN 147930 AND 147941 AND skillmodvalue = 0 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DELETE FROM items WHERE id BETWEEN 147954 AND 147965;

UPDATE items
   SET augslot1type = 0, augslot1visible = 0, augslot1unk2 = 0,
       augslot2type = 0, augslot2visible = 0, augslot2unk2 = 0,
       augslot3type = 0, augslot3visible = 0, augslot3unk2 = 0,
       augslot4type = 0, augslot4visible = 0, augslot4unk2 = 0,
       augslot5type = 0, augslot5visible = 0, augslot5unk2 = 0,
       augslot6type = 0, augslot6visible = 0, augslot6unk2 = 0
 WHERE id BETWEEN 147930 AND 147953;

UPDATE items SET slots = 8 WHERE id BETWEEN 147942 AND 147953;

CREATE TEMPORARY TABLE aotv4_ts_hands LIKE items;
INSERT INTO aotv4_ts_hands SELECT * FROM items WHERE id BETWEEN 147930 AND 147941;
UPDATE aotv4_ts_hands SET id = id + 24, slots = 4096, clickeffect = -1, clicktype = 0, clicklevel = 0, clicklevel2 = 0;
INSERT INTO items SELECT * FROM aotv4_ts_hands;
DROP TEMPORARY TABLE aotv4_ts_hands;

UPDATE items SET Name = 'Netmender''s Gloves'      WHERE id = 147954;
UPDATE items SET Name = 'Envenomer''s Gloves'      WHERE id = 147955;
UPDATE items SET Name = 'Machinist''s Gauntlets'   WHERE id = 147956;
UPDATE items SET Name = 'Archivist''s Gloves'      WHERE id = 147957;
UPDATE items SET Name = 'Distiller''s Gloves'      WHERE id = 147958;
UPDATE items SET Name = 'Kneader''s Mitts'         WHERE id = 147959;
UPDATE items SET Name = 'Stitcher''s Gloves'       WHERE id = 147960;
UPDATE items SET Name = 'Hammerhand Gauntlets'     WHERE id = 147961;
UPDATE items SET Name = 'Bowyer''s Gloves'         WHERE id = 147962;
UPDATE items SET Name = 'Masher''s Gloves'         WHERE id = 147963;
UPDATE items SET Name = 'Setter''s Gloves'         WHERE id = 147964;
UPDATE items SET Name = 'Thrower''s Gloves'        WHERE id = 147965;

UPDATE items
   SET skillmodtype = ELT(((id - 147930) MOD 12) + 1, 55, 56, 57, 58, 59, 60, 61, 63, 64, 65, 68, 69),
       skillmodmax  = 0
 WHERE id BETWEEN 147930 AND 147965;

UPDATE items SET skillmodvalue =  5 WHERE id BETWEEN 147930 AND 147941;
UPDATE items SET skillmodvalue = 10 WHERE id BETWEEN 147942 AND 147953;
UPDATE items SET skillmodvalue = 15 WHERE id BETWEEN 147954 AND 147965;

DELETE FROM custom_achievement_rewards WHERE reward_type = 'item' AND reward_id BETWEEN 147954 AND 147965;

INSERT INTO custom_achievement_rewards
  (achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 400000 + ELT(((i.id - 147930) MOD 12) + 1, 55, 56, 57, 58, 59, 60, 61, 63, 64, 65, 68, 69) * 1000 + 300,
       'item', i.id, 1, 10000, '', 1, 1, '', '', 1, 30, UNIX_TIMESTAMP()
  FROM items i WHERE i.id BETWEEN 147954 AND 147965;

UPDATE custom_achievement_rewards r
  JOIN items i ON i.id = r.reward_id
   SET r.preview_text = CONCAT(
         i.Name, ' (+', i.skillmodvalue, ' percent ',
         ELT(((i.id - 147930) MOD 12) + 1, 'Fishing', 'Make Poison', 'Tinkering', 'Research',
             'Alchemy', 'Baking', 'Tailoring', 'Blacksmithing', 'Fletching', 'Brewing',
             'Jewelcrafting', 'Pottery'), ')')
 WHERE r.reward_type = 'item' AND r.reward_id BETWEEN 147930 AND 147965;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 30,
		.description = "2026_08_07_aotv4_tradeskill_gear_charges_and_icons",
		// Two faults reported from play on the tradeskill gear, both cosmetic-looking and one fatal:
		//
		// ⚠️⚠️ THE MASKS COULD NOT BE CLICKED AT ALL -- "Item is out of charges." They shipped with
		// `maxcharges = 0`, which the engine reads as a spent consumable, so the illusion never fired
		// once. An UNLIMITED clicky is **-1**, not 0 and not a big number: 538 stock items use -1
		// (Journeyman's Boots, Traveler's Boots, Bracer of Hammerfal). 0 is the one value that makes a
		// clicky item permanently dead while still looking correctly configured -- clickeffect is set,
		// clicktype is 1, and nothing about the row reads as broken.
		// 📌 This is the second polarity trap on these same items: section 32 already records
		// `nodrop = 0` meaning No Drop, and `clicktype 4` meaning equip-only. Check the sense of every
		// flag on an item row rather than assuming 0/1 means off/on.
		//
		// ⚠️ ALL 36 SHARED `icon 639`, so the hand pieces and the masks both rendered as cloth hats --
		// reported as "the icons aren't matching the items, all of the items look like cloth hats".
		// The icons are now slot-appropriate, taken from the most common stock icon for each slot:
		// head 625, face 770, hands 531.
		// 📌 `idfile` is left at IT63, which stock head items also use (Chromatic Helm), so the worn
		// model is fine and only the inventory icon was wrong.
		//
		// ⚠️ `items` is SHARED MEMORY: world down, ./shared_memory, restart. The migration alone
		// changes nothing a player can see.
		.check       = "SELECT id FROM items WHERE id BETWEEN 147942 AND 147953 AND maxcharges = 0 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE items SET maxcharges = -1 WHERE id BETWEEN 147942 AND 147953;

UPDATE items SET icon = 625 WHERE id BETWEEN 147930 AND 147941;
UPDATE items SET icon = 770 WHERE id BETWEEN 147942 AND 147953;
UPDATE items SET icon = 531 WHERE id BETWEEN 147954 AND 147965;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 31,
		.description = "2026_08_07_aotv4_unlock_tradeskill_recipe_eras",
		// ⚠️⚠️ TRADESKILL RECIPES ARE ERA-GATED SEPARATELY FROM ZONES, AND THAT IS WHY THIS IS SAFE.
		// `tradeskill_recipe` carries its own `min_expansion`/`max_expansion`, and the recipe SEARCH
		// query in Handle_OP_RecipesSearch (client_packet.cpp) ends with ContentFilterCriteria::apply().
		// With Expansion:CurrentExpansion = 0 (Classic) that hid 388 ENABLED recipes -- 385 Pottery,
		// 3 Blacksmithing, almost all Planes of Power armour (Ashen Bone Mail, Armguards of the
		// Pestilence Priest, ...). Clearing the gate on the RECIPE ROWS unlocks them while
		// CurrentExpansion stays 0, so zones, doors, spawns and item content filtering are untouched.
		// Raising CurrentExpansion instead would have opened Kunark..PoP wholesale and defeated region
		// locking -- the exact thing section 35 records as dangerous when a dump did it silently.
		//
		// ⚠️⚠️ THEY WERE ALWAYS CRAFTABLE, ONLY UNFINDABLE. The combine path
		// (ZoneDatabase::GetTradeRecipe, tradeskills.cpp:1633) filters on `tr.enabled` ONLY and has no
		// content filter, so anyone who knew the component set could already make these. The gate was
		// purely on discovery, which is the worst of both worlds: no protection, just a hidden recipe.
		//
		// ⚠️ SCOPED TO `enabled = 1`. The other 3,296 gated rows are min_expansion 9 (Dragons of
		// Norrath) AND already `enabled = 0` -- deliberately off, consistent with the era system
		// capping at OoW (section 12). Enabling those is a separate content decision and is NOT done
		// here; this migration only removes an era gate from recipes that were already switched on.
		// ⚠️ `content_flags` is left alone -- one recipe carries `peq_halloween` and must stay seasonal.
		// 📌 Some of these need components that only drop in zones the server has not unlocked, so they
		// will show in the search and still not be makeable. That is ordinary "needs a rare component",
		// not a regression.
		.check       = "SELECT id FROM tradeskill_recipe WHERE enabled = 1 AND (min_expansion > -1 OR max_expansion > -1) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE tradeskill_recipe
   SET min_expansion = -1, max_expansion = -1
 WHERE enabled = 1 AND (min_expansion > -1 OR max_expansion > -1);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 32,
		.description = "2026_08_07_aotv4_tradeskill_mask_illusion_30_minutes",
		// The tradeskill mask illusions (44400-44411) lasted THREE minutes, not the intended thirty.
		//
		// ⚠️⚠️ THE CAUSE IS THE ITEM'S CAST LEVEL, NOT THE SPELL'S DURATION FIELD. They shipped
		// `buffdurationformula = 3` (= 30 * level, CalcBuffDuration_formula in zone/spells.cpp) with
		// `buffduration = 360` as the cap -- which reads as "36 minutes, capped" until you notice the
		// masks click at `clicklevel2 = 1`. At cast level 1 the formula yields 30 * 1 = 30 ticks =
		// exactly the 3 minutes observed. The 360 cap never came near binding.
		// 📌 A level-scaled formula is simply wrong for an item clicky with a FIXED cast level: the
		// duration is then pinned to that constant, and raising the cap does nothing.
		//
		// ⚠️ Fixed with a LEVEL-INDEPENDENT duration instead of by raising clicklevel2. The default
		// branch of the formula switch (spells.cpp) is:
		//     if (formula < 200) return 0;  temp = formula;
		// so a formula of **300 is literally 300 ticks**, whatever level it is cast at. 300 ticks x 6
		// seconds = 1800s = 30 minutes. `buffduration` stays 300 because the tail of that function
		// caps with `if (duration && duration < temp) temp = duration` -- set it lower and it silently
		// wins over the formula.
		// 📌 Raising clicklevel2 to 10 would also give 300 via formula 3, but it ties the duration to
		// a field that exists for a different purpose and breaks again if the item is ever re-cloned.
		//
		// ⚠️ These are inside the 43000-44999 band that Mob::CalcBuffDuration excludes from the
		// self-buff 3-day floor (spells.cpp:3194), so this native duration is authoritative. Without
		// that exclusion the whole change would be moot -- they would already be lasting days.
		// ⚠️ `spells_new` is SHARED MEMORY: world down, ./shared_memory, restart.
		.check       = "SELECT id FROM spells_new WHERE id BETWEEN 44400 AND 44411 AND buffdurationformula <> 300 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET buffdurationformula = 300, buffduration = 300
 WHERE id BETWEEN 44400 AND 44411;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 33,
		.description = "2026_08_07_aotv4_zone_xp_table",
		// The Zone XP tab in the Allaclone window: which zones are worth hunting, by region.
		//
		// ⚠️⚠️ THESE RANGES ARE AUTHORED, NOT DERIVED, AND THAT IS DELIBERATE. A computed range was
		// built first (percentile 10-90 of spawned mob levels, excluding merchants, bankers, GM
		// trainers, LDoN treasure chests and anything on a town faction) and it disagreed with the
		// owner's list -- Blackburrow computed 4-8 against an authored 2-15, Cabilis West computed 1-1
		// against 30-35. Neither is wrong: the computed figure is "where the bulk of the spawns sit"
		// and the authored one is "what you might meet". The authored list is what ships, so it lives
		// in a table that can be edited without a rebuild rather than being recomputed on the fly.
		//
		// ⚠️⚠️ THE PRIMARY KEY IS zone_id ALONE, WHICH IS WHAT PINS A ZONE TO ONE REGION.
		// `zone_regions` is many-to-many: The Swamp of No Hope (83) is legitimately reachable from
		// BOTH Firiona Vie and Cabilis, so a naive join lists it twice under two headers. The owner
		// chose Cabilis, and the single-column key makes that a data decision instead of something the
		// query has to keep re-deciding. Any future zone in two regions must likewise pick one.
		//
		// ⚠️ DELVE ZONES ARE DELIBERATELY ABSENT. Region 0 "Always Available" holds 40 zones and 39 of
		// them are the LDoN/DoN maps unlocked by v27/v31 (Deepest Guk, Miragul's, Rujarkian,
		// Takish-Hiz, Mistmoore's Catacombs...). A fixed level range for those is meaningless because
		// delve creatures are SCALED to the player at spawn (section 24), so publishing one would be
		// actively misleading. The 40th is The Resplendent Temple, the hub.
		// 📌 Butcherblock Mountains (68) and Kaesora (88) were NOT in the owner's list and their ranges
		// were derived here from the spawn data, because the authored list turned out not to be
		// derivable from this database at all -- its upper bound is sometimes ABOVE our raw maximum
		// (Misty Thicket 25 against a real 14) and sometimes far below it (Crushbone 14 against 65), so
		// no percentile reproduces it and there was no formula to extend.
		//   Butcherblock 1-20: 399 of its 448 spawns are level 1-10, only 13 are above 20 and the
		//     single level-40 is one mob. Calling it 1-35 like the other hub zones would advertise
		//     content that is one spawn.
		//   Kaesora 28-35: nothing below 21, 242 of 295 spawns in the 31-40 band, minimum 28, capped at
		//     35 like every other high-end Cabilis zone.
		.check       = "SHOW TABLES LIKE 'aotv4_zone_xp'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_zone_xp (
  zone_id   INT NOT NULL,
  region_id INT NOT NULL,
  lo        INT NOT NULL,
  hi        INT NOT NULL,
  label     VARCHAR(16) NULL,
  PRIMARY KEY (zone_id),
  KEY idx_region (region_id)
);

REPLACE INTO aotv4_zone_xp (zone_id, region_id, lo, hi, label) VALUES
  (54,1,1,6,NULL), (68,1,1,14,NULL), (58,1,2,8,NULL), (57,1,5,11,NULL), (70,1,9,15,NULL), 
  (63,1,13,16,NULL), (59,1,15,31,NULL), (64,1,25,34,NULL), (22,2,1,5,NULL), (34,2,1,5,NULL), 
  (69,2,1,29,NULL), (21,2,5,10,NULL), (36,2,6,10,NULL), (33,2,9,12,NULL), (37,2,9,20,NULL), 
  (20,2,9,21,NULL), (35,2,14,16,NULL), (11,2,23,32,NULL), (16,2,25,28,NULL), 
  (186,2,27,33,NULL), (10,2,1,3,'City'), (9,2,1,5,'City'), (19,2,1,19,'City'), 
  (8,2,1,29,'City'), (118,3,2,6,NULL), (110,3,9,19,NULL), (116,3,9,19,NULL), 
  (121,3,11,19,NULL), (111,3,13,20,NULL), (112,3,17,26,NULL), (129,3,26,29,NULL), 
  (128,3,30,34,NULL), (115,3,30,34,'City'), (84,4,1,4,NULL), (86,4,5,24,NULL), 
  (92,4,7,19,NULL), (107,4,16,19,NULL), (81,4,16,26,NULL), (96,4,17,31,NULL), 
  (102,4,24,30,NULL), (45,5,1,3,NULL), (4,5,1,9,NULL), (12,5,2,7,NULL), (17,5,2,9,NULL), 
  (13,5,3,11,NULL), (50,5,5,15,NULL), (14,5,8,19,NULL), (51,5,9,16,NULL), (15,5,10,13,NULL), 
  (18,5,25,34,NULL), (2,5,1,3,'City'), (3,5,1,6,'City'), (1,5,1,31,'City'), (78,6,1,5,NULL), 
  (79,6,1,7,NULL), (83,6,1,7,NULL), (85,6,1,10,NULL), (97,6,4,13,NULL), (104,6,16,24,NULL), 
  (109,6,23,32,NULL), (88,6,28,35,NULL), (82,6,1,1,'City'), (106,6,30,30,'City');)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 34,
		.description = "2026_08_07_aotv4_bulwark_within_match_hp",
		// Bulwark Within (rank 1367) granted HP 300 but mana 200 and endurance 200, while its
		// description promises "increased hit points, mana, and endurance" without saying the amounts
		// differ. All three are now a flat 300 so the AA reads as one coherent grant.
		//
		// ⚠️⚠️ THE MANA STILL DOES NOTHING FOR WARRIOR, MONK, ROGUE AND BERSERKER, AND THAT IS NOT
		// FIXED HERE. Client::CalcMaxMana (zone/client_mods.cpp) branches: caster classes get
		// CalcBaseMana() + item + spell + AA bonuses, while the four pure-melee classes get a flat
		// GetLevel() * 40 with NO bonus terms at all. That is deliberate -- section 14 records that the
		// dll computes the mana GAUGE itself from level * AOTV4_MELEE_MANA_PER_LEVEL using only
		// SPAWNINFO->Level, so the server must produce the identical number or the bar misreports.
		// The consequence is far wider than this AA: 40,103 wearable mana items and 65 mana-granting
		// AAs are all inert for those four classes, and mana is the one stat they cannot improve.
		// 📌 Fixing it is one line (add the three bonus terms to the melee branch) plus a dll change so
		// the gauge agrees. Deliberately NOT done here -- it is a balance decision, not a bug fix.
		//
		// ⚠️ Endurance is fine on every class: CalcMaxEndurance has no class branch and already sums
		// spell, item and AA bonuses. Only mana is special-cased.
		.check       = "SELECT rank_id FROM aa_rank_effects WHERE rank_id = 1367 AND effect_id IN (97, 190) AND base1 <> 300 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_rank_effects SET base1 = 300 WHERE rank_id = 1367 AND effect_id IN (97, 190);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 35,
		.description = "2026_08_07_aotv4_bulwark_within_description",
		// Bulwark Within's description promised "hit points, mana, and endurance" to everyone, and for
		// Warrior, Monk, Rogue and Berserker the mana half is silently discarded -- Client::CalcMaxMana
		// gives those four a flat GetLevel() * 40 with no bonus terms (section 14: the dll computes the
		// gauge from level alone, so the server must match it exactly). Reported from play as "Bulwark
		// within is not giving mana".
		//
		// The grant is left as-is (300 / 300 / 300 from v34) because the engine already splits it the
		// way it should: EVERY class banks the hit points and the endurance, and only spellcasters can
		// bank the mana. The description now says so instead of implying all three land for all.
		//
		// ⚠️⚠️ THE CLIENT RESOLVES THIS STRING FROM ITS OWN dbstr_us.txt, NOT FROM THIS DATABASE
		// (section 6). Writing db_str changes nothing in game on its own -- it needs
		// `./export_client_files` and the regenerated dbstr_us.txt shipped to players. Until then the
		// old wording stays on screen even though the row here is correct.
		// ⚠️ No literal percent sign: the description path is printf-style and eats it as a format
		// token, rendering as garbage (sections 5 and 14). Amounts are spelled out.
		// 📌 type 4 is the description. type 1 is the window title and types 2/3 are the two halves of
		// the hotkey label -- section 6 records that renaming an AA means writing ALL of them, but this
		// migration only changes the description, so 1/2/3 are correctly left alone.
		.check       = "SELECT id FROM db_str WHERE id = 1367 AND type = 4 AND value LIKE '%increased hit points, mana, and endurance%'",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE db_str SET value = 'Your devotion to Norrath''s Keepers grants 300 hit points and 300 endurance. Spellcasting classes also gain 300 mana; Warriors, Monks, Rogues and Berserkers draw on endurance instead.'
 WHERE id = 1367 AND type = 4;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 36,
		.description = "2026_08_07_aotv4_delve_tasks_per_dungeon",
		// ⚠️⚠️ THE DELVE JOURNAL NAMED THE WRONG DUNGEON, AND HAD SINCE LDoN LANDED. The 36 task rows
		// were authored when the delve was DoN-only: six zones x six modes, one task per zone, so a
		// zone name in the title was correct. Adding 33 LDoN dungeons made them ROUND-ROBIN the same
		// six task families (aotv4_dungeon_ldon.lua), so a Deepest Guk run was handed
		// "Delve: Tirranun's Delve" -- wrong roughly 85 percent of the time. Reported from play.
		// 📌 It was never a FUNCTIONAL bug: migration v22 cleared task_activities.zones, so CheckZone
		// returns true on an empty zone list and the kills credit in whatever zone you are in. Only the
		// label lied, which is exactly why nothing errored and it survived this long.
		//
		// Now 39 dungeons x 6 modes = 234 tasks, one per (dungeon, mode), built here as 39 explicit
		// dungeon rows CROSS JOINed with the 6 modes rather than 234 hand-written INSERTs -- same
		// deterministic result, a tenth of the size, and adding a dungeon is one row.
		//
		// ⚠️⚠️ THE MODE OFFSETS HAD TO WIDEN FROM 10 TO 40 AND aotv4_dungeon.lua MUST MATCH.
		// M.MODES.taskoff was 0/10/20/30/40/50, which only leaves room for TEN dungeons per mode; with
		// 39 the families would overlap and a Hard run would collect a Standard task. They are now
		// 0/40/80/120/160/200, so the band runs 2000300-2000538. Change one side without the other and
		// tasks silently resolve to the wrong mode.
		// ⚠️ Base ids: DoN keeps 2000300-2000305 (unchanged, so its hand-written flavour survives),
		// LDoN takes 2000306-2000338 in aotv4_dungeon_ldon.lua's own file order. That order IS the
		// mapping -- regenerating that file without reassigning ids reintroduces this bug.
		//
		// ⚠️ `zones` stays EMPTY on every activity, exactly as v22 left it. Populating it would scope a
		// task to one zone again and break the 33 LDoN dungeons that share a rung's task family.
		// ⚠️ Gauntlet is the odd mode: 10 trash and THREE wardens, not a multiple of the base goal.
		// ⚠️ max_level is 200 and duration 21600/code 3, copied from the original rows -- not defaults.
		// ⚠️ Keyed on the HIGHEST new id (2000538 = Fragile + dungeon 38), which cannot exist under the
		// old 6-zone scheme whose band stopped at 2000355. Testing for a wrong TITLE instead would
		// match the mode variants of the correct task too and re-run forever.
		.check       = "SELECT id FROM tasks WHERE id = 2000538",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_delve_d;
CREATE TEMPORARY TABLE aotv4_delve_d (idx INT, name VARCHAR(64), goal INT, target VARCHAR(64), descr TEXT, emote TEXT);
INSERT INTO aotv4_delve_d (idx,name,goal,target,descr,emote) VALUES
  (0,'Lavaspinner''s Lair',30,'the lair''s defenders','The lavaspinners have overrun the lair. Cut them down, then face what they were guarding.','The lair falls silent. Something glitters where the last of them dropped.'),
  (1,'Tirranun''s Delve',35,'Tirranun''s brood','Tirranun''s brood has grown bold in the deep. Thin them out, then face what they answer to.','The delve goes quiet. Something glitters where the last of them dropped.'),
  (2,'Stillmoon Temple',40,'the temple guardians','The temple guardians no longer answer to anyone. Put them down, then face their keeper.','The temple stills. Something glitters where the last of them dropped.'),
  (3,'Stillmoon Ascent',45,'the ascent''s wardens','Cut a path up the ascent, then face what waits at the top of it.','The ascent falls quiet. Something glitters where the last of them dropped.'),
  (4,'Thundercrest Isles',50,'the storm drakes','The storm drakes rule the isles unchallenged. Break them, then face the one that leads them.','The storm passes. Something glitters where the last of them dropped.'),
  (5,'The Nest',60,'the brood of the Nest','The Nest is thick with brood. Clear it out, then face what has been feeding them.','The Nest goes still. Something glitters where the last of them dropped.'),
  (6,'Deepest Guk - Cauldron of Lost Souls',30,'the denizens within','Deepest Guk - Cauldron of Lost Souls runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (7,'Deepest Guk - Ancient Aqueducts',30,'the denizens within','Deepest Guk - Ancient Aqueducts runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (8,'Deepest Guk - The Curse Reborn',30,'the denizens within','Deepest Guk - The Curse Reborn runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (9,'Deepest Guk - Chapel of the Witnesses',30,'the denizens within','Deepest Guk - Chapel of the Witnesses runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (10,'Deepest Guk - Accursed Sanctuary',30,'the denizens within','Deepest Guk - Accursed Sanctuary runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (11,'Miragul''s Menagerie - Silent Gallery',30,'the denizens within','Miragul''s Menagerie - Silent Gallery runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (12,'Miragul''s Menagerie - Frozen Nightmare',30,'the denizens within','Miragul''s Menagerie - Frozen Nightmare runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (13,'Miragul''s Menagerie - Spider Den',30,'the denizens within','Miragul''s Menagerie - Spider Den runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (14,'Miragul''s Menagerie - Hushed Banquet',30,'the denizens within','Miragul''s Menagerie - Hushed Banquet runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (15,'Miragul''s Menagerie - Heart of the Menagerie',30,'the denizens within','Miragul''s Menagerie - Heart of the Menagerie runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (16,'Miragul''s Menagerie - Grand Library',30,'the denizens within','Miragul''s Menagerie - Grand Library runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (17,'Mistmoore''s Catacombs - Forlorn Caverns',30,'the denizens within','Mistmoore''s Catacombs - Forlorn Caverns runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (18,'Mistmoore''s Catacombs - Dreary Grotto',30,'the denizens within','Mistmoore''s Catacombs - Dreary Grotto runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (19,'Mistmoore''s Catacombs - Struggles within the Progeny',30,'the denizens within','Mistmoore''s Catacombs - Struggles within the Progeny runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (20,'Mistmoore''s Catacombs - Chambers of Eternal Affliction',30,'the denizens within','Mistmoore''s Catacombs - Chambers of Eternal Affliction runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (21,'Mistmoore''s Catacombs - Sepulcher of the Damned',30,'the denizens within','Mistmoore''s Catacombs - Sepulcher of the Damned runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (22,'Mistmoore''s Catacombs - Scion Lair of Fury',30,'the denizens within','Mistmoore''s Catacombs - Scion Lair of Fury runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (23,'Mistmoore''s Catacombs - Cesspits of Putrescence',30,'the denizens within','Mistmoore''s Catacombs - Cesspits of Putrescence runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (24,'Mistmoore''s Catacombs - Aisles of Blood',30,'the denizens within','Mistmoore''s Catacombs - Aisles of Blood runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (25,'Mistmoore''s Catacombs - Halls of Sanguinary Rites',30,'the denizens within','Mistmoore''s Catacombs - Halls of Sanguinary Rites runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (26,'Mistmoore''s Catacombs - Infernal Sanctuary',30,'the denizens within','Mistmoore''s Catacombs - Infernal Sanctuary runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (27,'The Rujarkian Hills - Bloodied Quarries',30,'the denizens within','The Rujarkian Hills - Bloodied Quarries runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (28,'The Rujarkian Hills - Prison Break',30,'the denizens within','The Rujarkian Hills - Prison Break runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (29,'The Rujarkian Hills - Fortified Lair of the Taskmasters',30,'the denizens within','The Rujarkian Hills - Fortified Lair of the Taskmasters runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (30,'The Rujarkian Hills - Hidden Vale of Deceit',30,'the denizens within','The Rujarkian Hills - Hidden Vale of Deceit runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (31,'The Rujarkian Hills - Arena of Chance',30,'the denizens within','The Rujarkian Hills - Arena of Chance runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (32,'The Rujarkian Hills - Barracks of War',30,'the denizens within','The Rujarkian Hills - Barracks of War runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (33,'Takish-Hiz - Sunken Library',30,'the denizens within','Takish-Hiz - Sunken Library runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (34,'Takish-Hiz - Shifting Tower',30,'the denizens within','Takish-Hiz - Shifting Tower runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (35,'Takish-Hiz - Within the Compact',30,'the denizens within','Takish-Hiz - Within the Compact runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (36,'Takish-Hiz - Royal Observatory',30,'the denizens within','Takish-Hiz - Royal Observatory runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (37,'Takish-Hiz - River of Recollection',30,'the denizens within','Takish-Hiz - River of Recollection runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.'),
  (38,'Takish-Hiz - Balancing Chamber',30,'the denizens within','Takish-Hiz - Balancing Chamber runs deep and no longer answers to anyone. Thin out what lives there, then face what waits at the end.','The dungeon falls silent. Something glitters where the last of them dropped.');

DROP TEMPORARY TABLE IF EXISTS aotv4_delve_m;
CREATE TEMPORARY TABLE aotv4_delve_m (suffix VARCHAR(16), taskoff INT, goalmul DECIMAL(4,2), fixedgoal INT, wardens INT, wname VARCHAR(32));
INSERT INTO aotv4_delve_m VALUES
  ('',0,1,0,1,'the delve warden'), (' (Hard)',40,1,0,1,'the delve warden'),
  (' (Swarm)',80,3,0,1,'the delve warden'), (' (Gauntlet)',120,0,10,3,'the delve wardens'),
  (' (Onslaught)',160,1,0,1,'the delve warden'), (' (Fragile)',200,1,0,1,'the delve warden');

DELETE FROM task_activities WHERE taskid BETWEEN 2000300 AND 2000599;
DELETE FROM tasks WHERE id BETWEEN 2000300 AND 2000599;

INSERT INTO tasks (id,type,duration,duration_code,title,description,reward_text,reward_id_list,cash_reward,exp_reward,reward_method,reward_points,reward_point_type,min_level,max_level,level_spread,min_players,max_players,repeatable,faction_reward,completion_emote,replay_timer_group,replay_timer_seconds,request_timer_group,request_timer_seconds,dz_template_id,lock_activity_id,faction_amount,enabled)
SELECT 2000300 + d.idx + m.taskoff, 0, 21600, 3,
       CONCAT('Delve: ', d.name, m.suffix), d.descr,
       'A reward chest, and a Delver''s Sigil for your first delve', '', 0, 0, 0, 0, 0,
       1, 200, 0, 1, 6, 1, 0, d.emote, 0, 0, 0, 0, 0, -1, 0, 1
  FROM aotv4_delve_d d CROSS JOIN aotv4_delve_m m;

INSERT INTO task_activities (taskid,activityid,req_activity_id,step,activitytype,target_name,goalmethod,goalcount,description_override,npc_match_list,item_id_list,item_list,dz_switch_id,min_x,min_y,min_z,max_x,max_y,max_z,skill_list,spell_list,zones,zone_version,optional,list_group)
SELECT 2000300 + d.idx + m.taskoff, 0, -1, 1, 2, d.target, 0,
       IF(m.fixedgoal > 0, m.fixedgoal, FLOOR(d.goal * m.goalmul)),
       '', '', '', '', 0, 0,0,0, 0,0,0, -1, 0, '', -1, 0, 0
  FROM aotv4_delve_d d CROSS JOIN aotv4_delve_m m
UNION ALL
SELECT 2000300 + d.idx + m.taskoff, 1, -1, 2, 2, m.wname, 0, m.wardens,
       '', '', '', '', 0, 0,0,0, 0,0,0, -1, 0, '', -1, 0, 0
  FROM aotv4_delve_d d CROSS JOIN aotv4_delve_m m;

DROP TEMPORARY TABLE aotv4_delve_d;
DROP TEMPORARY TABLE aotv4_delve_m;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 37,
		.description = "2026_08_07_aotv4_delve_zone_expansion_bypass",
		// ⚠️⚠️ THERE ARE TWO INDEPENDENT GATES BETWEEN A PLAYER AND A DELVE MAP, AND v27 ONLY OPENED
		// ONE OF THEM. v27 moved the 33 LDoN delve zones out of region 99 "Unused" (the AoTv4 region
		// lock). The SECOND gate is stock EQEmu and is checked separately in Client::ZonePC
		// (zone/zoning.cpp:367):
		//
		//     if (CurrentExpansion >= Classic && !GetGM()) {
		//         if (z->expansion <= CurrentExpansion || z->bypass_expansion_check) { ...ok... }
		//     }
		//
		// The LDoN delve zones are `zone.expansion` = 6 and this server runs Classic
		// (Expansion:CurrentExpansion = 0), so entry was refused with "The zone that you are
		// attempting to enter is part of an expansion that you do not yet own." That is HALF THE
		// LADDER -- the map ladder alternates DoN/LDoN, so all 35 even rungs were unenterable.
		//
		// ⚠️⚠️ NOTE THE `!GetGM()` -- THIS IS THE SECOND TIME THAT HAS HIDDEN THIS EXACT BUG. The GM
		// flag skips the check entirely ("Bypassing zone expansion checks because GM flag is set"), so
		// it works for every test character and fails for every player. The identical trap is written
		// up in custom/sql/aotv4_delve_zone_access.sql, which fixed the DoN six on 2026-08-02 -- the
		// LDoN dungeons simply joined the ladder afterwards and were never added to it.
		// 📌 It also presents as "the delve opened and then threw me out": the run bucket, the task and
		// the instance are all created BEFORE the move, so the refusal lands after the setup succeeded.
		//
		// ⚠️⚠️ `bypass_expansion_check`, NOT `expansion = 0`. `expansion` is real metadata -- content
		// filtering, zone listings and the era system (section 12) all read it -- so rewriting it would
		// make LDoN zones claim to be Classic content everywhere else in the server. The bypass flag
		// exempts ONLY the entry gate and leaves the zone honest about what it is.
		//
		// ⚠️ Applied to EVERY version row, not just version 0. The delve enters layout versions, and
		// although the check itself reads version 0 via GetZoneWithFallback, leaving the others at 0 is
		// a trap for the next person who changes how the version is resolved.
		//
		// ⚠️⚠️ `veksar` IS DELIBERATELY ABSENT, exactly as it is from v27. It is LDoN by expansion but a
		// real Kunark world zone with 2 zone_points leading into it, so it was removed from the delve
		// pool rather than unlocked -- bypassing its entry gate would open Kunark travel to anyone who
		// had not earned Cabilis. The list here is the SAME 33 short names as v27 and must stay in step.
		//
		// ⚠️ The six DoN delve zones are folded in too. They were fixed by the hand-run
		// custom/sql/aotv4_delve_zone_access.sql, which is NOT in this manifest and therefore never
		// applies itself -- on live, or on any freshly imported database, they would be blocked the
		// same way. Re-stating them here is idempotent (the check only fires on rows still at 0) and
		// makes the whole delve pool self-applying.
		// 📌 That script's OTHER fix -- `thenest` min_level 66, a different gate again
		// (Client::CanEnterZone) -- is included below for the same reason. No LDoN delve zone carries
		// min_level, min_status or flag_needed, so those 33 need nothing beyond the bypass.
		//
		// ⚠️ The `zone` table is NOT shared memory, but it is read at boot by world AND zone: restart
		// both. No ./shared_memory rebuild.
		.check       = "SELECT short_name FROM zone WHERE bypass_expansion_check = 0 AND short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest','guka','gukc','guke','gukf','gukh','mira','mirb','mirc','mird','mirg','mirj','mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj','ruja','rujd','rujf','rujg','ruji','rujj','taka','takb','takc','takd','take','takg') LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE zone
   SET bypass_expansion_check = 1
 WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest',
                      'guka','gukc','guke','gukf','gukh',
                      'mira','mirb','mirc','mird','mirg','mirj',
                      'mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj',
                      'ruja','rujd','rujf','rujg','ruji','rujj',
                      'taka','takb','takc','takd','take','takg');

UPDATE zone
   SET min_level = 0
 WHERE min_level > 0
   AND short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest',
                      'guka','gukc','guke','gukf','gukh',
                      'mira','mirb','mirc','mird','mirg','mirj',
                      'mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj',
                      'ruja','rujd','rujf','rujg','ruji','rujj',
                      'taka','takb','takc','takd','take','takg');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 38,
		.description = "2026_08_08_aotv4_class_aura_rework",
		// ⚠️⚠️ THE PROBLEM WAS NOT TUNING, IT WAS THAT EIGHT OF THE SIXTEEN AURAS WERE **FLAT**.
		// Reported as "some of our auras feel really strong and some don't". Of the 16 class auras
		// (section 15), eight were percentages -- which keep their value forever -- and eight were fixed
		// numbers: damage shields of 2-3, weapon procs for 6-7 damage, a 10 point heal, 4 mana, and 15
		// mana on a kill. Those were small at launch and are noise now, and they get worse every time
		// gear or mob damage moves. This converts the eight flat ones to effects that scale, and gives
		// each a distinct ROLE so a group running several auras gets a kit instead of duplicates.
		// 📌 Each character has ONE aura slot but auras are group-shared (aura_type 1), so a six-class
		// group can have six of these up at once. They are deliberately different in KIND.
		//
		// ⚠️⚠️ EVERY SPA BELOW WAS CHECKED IN `Mob::ApplySpellsBonuses`, NOT ASSUMED FROM THE HEADER.
		// spdat.h tags PetMaxHP/PetAvoidance/PetCriticalHit/PetMeleeMitigation as "[AA]", which reads
		// like they only work from AA -- they do NOT, all four have cases in ApplySpellsBonuses. That
		// tag is descriptive of typical live usage. The inverse trap is already recorded in
		// zone/aotv4_tank_aa.cpp (ApplyAABonuses has no case for MitigateMeleeDamage), so check the
		// function you actually need rather than trusting the comment.
		//
		//   43551 Cleric      SPA 125 ImprovedHeal 20      heals cast by the group land 20 pct harder
		//   43552 Paladin     SPA 111 ResistAll 15         group resistances (NOT physical -- see header)
		//   43553 Ranger      SPA 216 Accuracy 15 (lim -1) = stock "Accuracy I", -1 = all skills
		//   43555 Druid       SPA 3 MovementSpeed 25       + SPA 0 CurrentHP 5 = 5 hp per tic regen
		//   43557 Bard        SPA 189 CurrentEndurance 5   + SPA 15 CurrentMana 3, both per tic
		//   43560 Necro       (unchanged trigger) retunes 43575 Soul Siphon to a PERCENTAGE
		//   43562 Magician    SPA 218/215/397              pet crit, avoidance and mitigation
		//   43564 Beastlord   SPA 119 AttackSpeed3 10      OVERHASTE -- see the note below
		//
		// ⚠️⚠️ BEASTLORD IS SPA **119**, NOT 98, AND THAT DISTINCTION IS THE WHOLE FEATURE.
		// `Client::CalcHaste` (zone/client_mods.cpp) applies the main haste cap and THEN adds
		// hastetype3, so 119 is the only thing that goes past the cap -- real overhaste.
		//   * SPA 98 (AttackSpeed2, "Melody of Ervaj") is gated on `level > 49`. At our cap of 30
		//     (era_system.M.HARD_CAP) it does NOTHING -- it applies, shows in the buff window, and
		//     contributes zero. It is the one that LOOKS like the obvious haste-2 choice.
		//   * The two take their base differently: 98 is `effect_value - 100` (110 = +10 pct) while
		//     119 is used DIRECTLY (10 = +10 pct). Writing 110 on a 119 row asks for +110 pct.
		//   * At level <= 50 the v3 contribution is HARDCODED to a maximum of 10 -- `Character:Hastev3Cap`
		//     (25) only applies at 51+. So 10 is the engine ceiling here, not a tuning choice. To exceed
		//     it: either `Character:IgnoreLevelBasedHasteCaps = true` (⚠️ which ALSO removes the main
		//     level+25 haste cap and the item haste cap -- a server-wide melee change, not a knob), or a
		//     one-line change so the 1-50 branch reads the rule instead of the literal 10.
		//   * It is worth having anyway: of 168 spells carrying SPA 119, **ZERO** are learnable at level
		//     30 or below, so this aura is the only source of over-cap attack speed on the server.
		//
		// ⚠️ Numbers are calibrated against STOCK spells read out of the DB, not from memory:
		//   * endurance regen 5 = "Aura of the Chameleon Effect", itself a stock aura (stock duration
		//     buffs run 2/3/5). 10 would be double the strongest stock precedent.
		//   * accuracy 15 = stock "Accuracy I" (the line runs 15/30/45, limit -1 = all skills).
		//   * movement 25 is about half a SoW, appropriate for a permanent group aura.
		//   * the pet SPAs have NO stock spell rows at all (they are AA-only on live), so those three
		//     are deliberately conservative and are the ones most likely to need a second pass.
		//
		// ⚠️⚠️ PetMaxHP (213) IS DELIBERATELY NOT USED. `Mob::MakePet` reads it at SUMMON TIME
		// (zone/pets.cpp:140), so it only applies to a pet summoned while the aura is already up and
		// does nothing for a pet you already have. PetCriticalHit, PetAvoidance and PetMeleeMitigation
		// are all read LIVE off the owner during combat (attack.cpp:320, :5794), which is why the
		// Magician aura uses those three.
		//
		// ⚠️ `formula` 100 / `max` 0 on every slot: static values. Section 5 records that a level-scaled
		// formula is read against the CASTER's level, and the caster here is the aura NPC, not the
		// player -- so a scaling formula would key off the wrong thing entirely.
		// ⚠️ Focus effects (125) take the HIGHEST value rather than stacking, so two Clerics in a group
		// is not 40 percent. That is stock focus behaviour, not a bug.
		// 📌 43570 Reprisal and 43573 Spirit Chill: 43570 is left ORPHANED (the Cleric aura no longer
		// procs it) and 43573 is untouched and still used by the Shaman aura, which was already a
		// percentage and is not part of this rework.
		//
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY. This migration applying at world boot is NOT enough:
		// stop the stack, run ./shared_memory, restart. Until then nothing changes in game.
		// 📌 The aura DESCRIPTIONS are not updated here -- the client resolves those from its own
		// spells_us.txt / dbstr_us.txt (section 6), so they need an ./export_client_files and a client
		// file ship regardless of what this writes.
		.check       = "SELECT id FROM spells_new WHERE id = 43564 AND effectid1 <> 119",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
-- Cleric: heals cast by anyone in the aura land harder.
UPDATE spells_new SET effectid1 = 125, effect_base_value1 = 20, effect_limit_value1 = 0, formula1 = 100, max1 = 0
 WHERE id = 43551;

-- Paladin: group resistances.
UPDATE spells_new SET effectid1 = 111, effect_base_value1 = 15, effect_limit_value1 = 0, formula1 = 100, max1 = 0
 WHERE id = 43552;

-- Ranger: group accuracy. limit -1 = all skills, matching stock Accuracy I.
UPDATE spells_new SET effectid1 = 216, effect_base_value1 = 15, effect_limit_value1 = -1, formula1 = 100, max1 = 0
 WHERE id = 43553;

-- Druid: movement plus a small out-of-combat regen. SPA 0 on a duration buff repeats per tic.
UPDATE spells_new SET effectid1 = 3, effect_base_value1 = 25, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
                      effectid2 = 0, effect_base_value2 = 5,  effect_limit_value2 = 0, formula2 = 100, max2 = 0
 WHERE id = 43555;

-- Bard: endurance regen (the only source besides the Sinew line) plus a little mana.
UPDATE spells_new SET effectid1 = 189, effect_base_value1 = 5, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
                      effectid2 = 15,  effect_base_value2 = 3, effect_limit_value2 = 0, formula2 = 100, max2 = 0
 WHERE id = 43557;

-- Necromancer: the kill-shot trigger is unchanged; its payload becomes a percentage of max HP.
UPDATE spells_new SET effectid1 = 147, effect_base_value1 = 2,  effect_limit_value1 = 0, formula1 = 100, max1 = 0,
                      effectid2 = 15,  effect_base_value2 = 25, effect_limit_value2 = 0, formula2 = 100, max2 = 0
 WHERE id = 43575;

-- Magician: the group's pets. All three are read live off the owner.
UPDATE spells_new SET effectid1 = 218, effect_base_value1 = 15, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
                      effectid2 = 215, effect_base_value2 = 10, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
                      effectid3 = 397, effect_base_value3 = 10, effect_limit_value3 = 0, formula3 = 100, max3 = 0
 WHERE id = 43562;

-- Beastlord: overhaste. SPA 119 is added AFTER the haste cap; base is the percentage directly.
UPDATE spells_new SET effectid1 = 119, effect_base_value1 = 10, effect_limit_value1 = 0, formula1 = 100, max1 = 0
 WHERE id = 43564;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 39,
		.description = "2026_08_08_aotv4_delve_traps_goals_and_boss_credit",
		// Three delve bugs reported together.
		//
		// ============================================================================================
		// 1. THE BOSS COULD BE SKIPPED BY KILLING ANYTHING
		// ============================================================================================
		// ⚠️⚠️ Activity 1 (kill the warden) shipped with an EMPTY `npc_match_list`, and an empty list
		// matches EVERY npc -- the same mechanism section 24 relies on for activity 0, where it is
		// exactly what we want. On the boss step it meant one trash kill closed the step, so the only
		// hard part of a delve was optional.
		// ✅ `npc_match_list` accepts NPC TYPE IDS as well as names (task_client_state.cpp:530:
		// `IsInMatchList(activity.npc_match_list, std::to_string(filter.mob->GetNPCTypeID()))`), and
		// every warden is npc **2000301** (aotv4_dungeon.M.BOSS_NPC). That matters because a warden's
		// NAME is randomised -- a given name plus a rolled title -- so matching by name could never
		// have worked here.
		// ⚠️ This does NOT reintroduce the zone-scoping problem v22 removed: that was the `zones`
		// column, which stays empty. npc matching is independent of zone.
		//
		// ============================================================================================
		// 2. DoN KILL GOALS WERE 30-60 WHILE LDoN WAS A FLAT 30
		// ============================================================================================
		// The six DoN dungeons carried hand-authored goals (30/35/40/45/50/60) from when the delve was
		// DoN-only; v36 carried them across unchanged while all 33 LDoN dungeons got 30. Odd rungs are
		// DoN and even rungs LDoN, so the ladder alternated between a 30-kill run and a 60-kill one at
		// the same difficulty. Normalised to the LDoN value.
		// ⚠️ The per-mode maths has to be reapplied, not just the base: Swarm is goalmul 3 (90) and
		// Gauntlet is a FIXED 10 with three wardens, not a multiple. Mode is derived from the task id
		// (offsets 0/40/80/120/160/200, section 24) and the dungeon index is the remainder.
		//
		// ============================================================================================
		// 3. TRAPS WERE FIRING IN DELVES
		// ============================================================================================
		// 14 of the 39 delve maps carry stock traps (all Mistmoore's and Miragul's; mmca/mmcd/mmce have
		// 84 each). They are LDoN dungeon furniture that has nothing to do with a scaled delve run.
		// ⚠️⚠️ **`chance` CANNOT BE USED TO TURN A TRAP OFF -- `chance == 0` MEANS ALWAYS FIRE.**
		// zone/trap.cpp:316 reads `(trap->chance == 0 || zone->random.Roll(trap->chance))`, so zeroing
		// it makes every trap trigger on approach, the exact opposite of the intent. Same class of
		// sentinel trap as `maxcharges = 0` on the tradeskill masks (section 32).
		// ✅ Traps load through `TrapsRepository::GetWhere(... ContentFilterCriteria::apply())`
		// (trap.cpp:454), so `min_expansion` is the engine's own switch. Setting it to 99 filters them
		// out at load and is fully reversible -- no rows are destroyed, and restoring them is one
		// UPDATE back to -1.
		// ⚠️ Scoped to the delve zone list ONLY. Those zones are unreachable except as a delve instance
		// (v27: zero zone_points target any of them), so this cannot change the open world.
		// 📌 The end-of-run chest (npc 2000300) and the warden (2000301) are npcs, not traps, so nothing
		// here can touch them.
		// 📌 GMs never trigger traps either (`!cur->GetGM()` on that same line) -- the fourth thing this
		// week that was invisible while testing with the GM flag on.
		.check       = "SELECT taskid FROM task_activities WHERE taskid BETWEEN 2000300 AND 2000538 AND activityid = 1 AND npc_match_list = ''",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE task_activities
   SET npc_match_list = '2000301'
 WHERE taskid BETWEEN 2000300 AND 2000538 AND activityid = 1;

UPDATE task_activities
   SET goalcount = CASE FLOOR((taskid - 2000300) / 40)
                     WHEN 2 THEN 90
                     WHEN 3 THEN 10
                     ELSE 30
                   END
 WHERE taskid BETWEEN 2000300 AND 2000538
   AND activityid = 0
   AND MOD(taskid - 2000300, 40) BETWEEN 0 AND 5;

UPDATE traps
   SET min_expansion = 99
 WHERE zone IN ('guka','gukc','guke','gukf','gukh',
                'mira','mirb','mirc','mird','mirg','mirj',
                'mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj',
                'ruja','rujd','rujf','rujg','ruji','rujj',
                'taka','takb','takc','takd','take','takg',
                'delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 40,
		.description = "2026_08_08_aotv4_song_buff_window_and_remove_healing_potions",
		// ============================================================================================
		// 1. ⚠️⚠️ BENEFICIAL SONGS VANISHED ON EVERY ZONE -- THE SONG WINDOW IS NEVER PERSISTED
		// ============================================================================================
		// Reported as "bard buffs are not maintaining through zones -- when I had the buff on, it
		// disappeared when I zoned", on Anthem de Arms sung by a Warrior.
		//
		// A buff's slot is chosen by `spells_new.short_buff_box`:
		//     GetFirstBuffSlot(IsDisciplineBuff(spell_id), spells[spell_id].short_buff_box)  (spells.cpp:3805)
		//     Client::GetFirstBuffSlot: if (song) return GetMaxBuffSlots();                  (spells.cpp:3745)
		// so a `short_buff_box = 1` spell lands in the SONG window, at slot indices ABOVE the long
		// buff slots. And both halves of persistence iterate the LONG slots only:
		//     max_buff_slots = StaticLookup(client->ClientVersion())->LongBuffs               (zonedb.cpp:3008)
		// ⚠️ So NOTHING in the song window is ever written to `character_buffs` or read back. It is not
		// a bug in stock EQ -- a song there is transient by design, because a Bard re-pulses it every
		// six seconds. Section 36 removed that pulse from BENEFICIAL songs and gave them the 3-day
		// self-buff duration, and the song window then silently threw them away on every zone line.
		// 📌 The tell was in the data: 701 Anthem de Arms is short_buff_box 1 and vanishes, while 703
		// Chords of Dissonance is short_buff_box 0 and persists. Same caster, same class, same skill.
		//
		// Fix: beneficial songs move to the LONG buff window, which is where a 3-day buff belongs.
		// ⚠️⚠️ DETRIMENTAL SONGS ARE NOT TOUCHED -- exactly ONE is in the song window and it stays
		// there. Those still pulse (section 36 keeps the two behaviours on one predicate, in opposite
		// senses), they are meant to be transient, and they land on the ENEMY where persistence is
		// meaningless. Widening this to all songs would break the half that still works.
		// ⚠️ Scoped to the five SONG skills. 915 non-song spells also use short_buff_box -- potions,
		// clickies and the like -- and none of them are in scope.
		// ⚠️ `IsShortDurationBuff` (spdat.cpp:2250) is the only other reader and it is exposed ONLY to
		// Lua/Perl scripting; no engine behaviour keys off it, so this changes slot placement and
		// nothing else.
		// 📌 Consequence: these now occupy LONG buff slots and display in the normal buff window rather
		// than the song window. That is the honest presentation for something lasting three days, but
		// it does add buff-slot pressure for a character carrying many of them.
		//
		// ============================================================================================
		// 2. HEALING POTIONS ARE REMOVED
		// ============================================================================================
		// 28 items: itemtype 21 with a direct-heal click (SPA 0, positive base) and "potion" in the
		// name -- Potion of Light Healing, the 5-dose Troll's Essence / Soluan's Vigor / Calimony line
		// and friends. Removed from 19 lootdrop rows and 30 merchant rows so no more enter the game,
		// AND their click is nulled so the ones players already carry stop working.
		// ⚠️ The item ROWS are deliberately kept. Deleting them would strand every reference in player
		// inventories, bank slots, trader escrow (section 13 -- those rows ARE the item) and quest
		// scripts. An inert item is safe; a missing item id is not.
		// ⚠️ `clicktype` is zeroed alongside `clickeffect`: section 32 records that clicktype alone
		// decides whether a click is even attempted, so leaving it set on a spell-less item invites a
		// later reader to "repair" the effect.
		// 📌 No tier clones exist for any of the 28 (checked: 0 rows in either tier band), so there is
		// no Hallowed/Mythic copy to sweep.
		//
		// ⚠️⚠️ BOTH `items` AND `spells_new` ARE SHARED MEMORY: stop the stack, run ./shared_memory,
		// restart. The migration applying at world boot changes nothing on its own.
		.check       = "SELECT id FROM spells_new WHERE id = 701 AND short_buff_box = 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new
   SET short_buff_box = 0
 WHERE short_buff_box = 1
   AND goodEffect <> 0
   AND skill IN (12, 41, 49, 54, 70)
   AND (buffduration > 0 OR buffdurationformula > 0)
   AND id < 45000;

DROP TEMPORARY TABLE IF EXISTS aotv4_heal_potions;
CREATE TEMPORARY TABLE aotv4_heal_potions (id INT PRIMARY KEY);
INSERT INTO aotv4_heal_potions
SELECT i.id FROM items i JOIN spells_new s ON s.id = i.clickeffect
 WHERE i.clickeffect > 0 AND i.id < 300000
   AND s.effectid1 = 0 AND s.effect_base_value1 > 0
   AND i.Name LIKE '%otion%';

DELETE l FROM lootdrop_entries l JOIN aotv4_heal_potions p ON p.id = l.item_id;
DELETE m FROM merchantlist     m JOIN aotv4_heal_potions p ON p.id = m.item;

UPDATE items i JOIN aotv4_heal_potions p ON p.id = i.id
   SET i.clickeffect = 0, i.clicktype = 0, i.clicklevel = 0, i.clicklevel2 = 0;

DROP TEMPORARY TABLE aotv4_heal_potions;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 41,
		.description = "2026_08_08_aotv4_titans_resistance_faction",
		// ⚠️⚠️ EVERY CIVIC NPC NOW CONS OFF ONE SHARED FACTION, "Titan's Resistance", AND IT HAS NO
		// RACE, CLASS OR DEITY MODIFIERS -- so every race walks into every town friendly, by birth.
		// Reported as people picking a region and still being faction locked. Section 28's con FLOOR
		// (Dubious, applied in Client::GetFactionLevel) stopped races being killed on sight, but it
		// deliberately stopped at Dubious -- enough to survive, not enough to be welcome, and it only
		// covered the 74 factions someone had curated. This replaces "not hostile" with "friendly".
		// 📌 This server is explicitly not simulating race wars: birth should never decide access.
		//
		// ============================================================================================
		// WHY THIS IS A THREE-LINE CHANGE AND NOT A REWRITE OF 2,969 NPCs
		// ============================================================================================
		// ⚠️⚠️ CON AND FACTION HITS COME FROM DIFFERENT COLUMNS, WHICH IS THE WHOLE TRICK.
		//   * The CON a player sees is `npc_faction.primaryfaction` (Mob::GetPrimaryFaction, read in
		//     zone/aggro.cpp:250 and Client::GetFactionLevel).
		//   * The faction HITS for killing something come from `npc_faction_entries`, reached via
		//     `npc_types.npc_faction_id` -- HateList::DoFactionHits(npc_faction_level_id, ...) falls to
		//     `SetFactionLevel(..., npc_faction_level_id, ...)` and walks the entries.
		// So repointing `primaryfaction` alone changes what everybody CONS at and leaves every existing
		// faction consequence exactly where it was. `npc_types` is not touched at all -- zero rows.
		// ⚠️ The one exception is the direct-reward path: DoFactionHits calls RewardFaction(faction_id)
		// instead when `npc_types.faction_amount != 0`, which WOULD pay the new faction. Checked: **0**
		// of the affected NPCs set faction_amount, so that path is never taken here. Re-check it before
		// widening this to any other faction set.
		//
		// ⚠️⚠️ SCOPED BY FACTION, NEVER BY ZONE. Scoping by zone is the obvious approach and it is
		// catastrophic: Kelethin is not a zone, it is a treetop city inside `gfaydark`, which is also a
		// hunting zone full of Crushbone orcs -- and Qeynos' region includes Rathe Mountains and the
		// Karanas. "Every NPC in a city zone" would hand friendly faction to the monsters. The 74
		// curated civic factions in `aotv4_town_factions` (section 28) are the unit that means "a town".
		//
		// ⚠️ NO `faction_list_mod` ROWS ARE CREATED FOR IT, AND THAT IS THE ENTIRE POINT.
		// CalculateFaction sums earned + base + class_mod + race_mod + deity_mod (common/faction.cpp:57);
		// with no mod rows, every race/class/deity gets exactly `base`. The 1,902 race modifiers on the
		// old civic factions still exist but nothing cons on them any more.
		// 📌 "New characters are on it" needs no per-character work for the same reason: a character
		// with no earned faction sits at base, so a freshly created anything cons Warmly on day one.
		//
		// base 750 = Warmly (Faction:WarmlyFactionMinimum). Deliberately not Ally (1100): Warmly reads
		// as clearly welcome while leaving headroom in both directions if anything is ever earned.
		//
		// ⚠️⚠️ TWO QUESTS THAT REQUIRE **HOSTILE** FACTION WILL STOP BEING OFFERED, KNOWINGLY.
		// Section 28 enumerated the four scripts that test the hostile end of the scale; two of them sit
		// on civic factions and now con Warmly (2) instead of Dubious (7):
		//   erudnext/Collier.lua            tests `>= 7`  (Circle of Unseen Hands)
		//   erudnext/Weligon_Steelherder.lua tests `> 5`   (Deepwater Knights)
		// The other two -- felwithea/Tynkale (Clerics of Tunare) and westwastes/Sontalak (Claws of
		// Veeshan) -- are NOT civic and are unaffected. Accepted as the cost of the design; re-check
		// this list if the civic set is ever widened.
		//
		// ⚠️ REVERSIBLE: `aotv4_faction_primary_backup` records each npc_faction's original
		// primaryfaction before the update, so the whole change is one UPDATE ... JOIN back.
		// ⚠️ Faction data is read at ZONE BOOT, not shared memory (only `items` and `spells_new` are) --
		// so this needs a world+zone restart and NOT a ./shared_memory run.
		.check       = "SELECT id FROM faction_list WHERE id = 9000",
		.condition   = "empty",
		.match       = "",
		// ⚠️⚠️ THE 74-ROW CIVIC LIST UNDER-COVERED THE HUBS, WHICH IS *WHY* PEOPLE WERE STILL LOCKED.
		// Section 28 built that list from a keyword test ("Merchants of", "Guards of", "Residents"...)
		// and warns in its own text that the test is a keyword list, not a classifier, and silently
		// under-covers. Auditing the thirteen real hub city zones found the biggest civic factions in
		// the game missing from it, every one of them race-hostile:
		//     229  Coalition of Tradefolk              70 merchants   worst race mod -375  (FREEPORT)
		//     406  Coldain                             61 merchants   worst race mod -100  (THURGADIN)
		//     336  Coalition of Tradefolk Underground  21 merchants   worst race mod -400
		//     271  Dismal Rage                         10 merchants   worst race mod -375
		//     355  Storm Reapers                        6 merchants   worst race mod -800  (KOS!)
		//     311  Steel Warriors                       1 merchant    worst race mod -400
		//     5025 Kaladim Merchants / 286 Mayor Gubbin              harmless, added for completeness
		// Freeport's entire merchant body and the whole of Thurgadin were outside the list. None of
		// them matches the keyword test -- "Coalition of Tradefolk" and "Coldain" simply do not look
		// like the names someone thought of.
		// 📌 Firiona Vie was CHECKED and was already fine: `Inhabitants of Firiona Vie` (31 merchants)
		// and `Emerald Warriors` are both covered, and everything uncovered in that zone is a monster
		// faction (Frogloks of Kunark, Goblins of Mountain Death, Agents of Mistmoore) -- which is the
		// faction-scoping doing its job.
		//
		// ⚠️⚠️ SO THE SET IS **DERIVED, NOT CURATED** -- that is the actual fix, and it is why this is
		// not just "section 28 plus a few more names". A hand list makes "not covered" the default and
		// under-coverage silent; deriving it makes "covered" the default and leaves only a short,
		// reviewable exclusion list. Two bounds make the derivation safe:
		//   1. TOWN ZONES ONLY (the list above). This is what keeps Karnor's Castle, Skyshrine, Kael
		//      Drakkel, Chardok, Solusek's Eye and Sanctus Seru out. Every one of those has a VENDOR,
		//      so a server-wide "all merchants" sweep would put Venril Sathir (18 merchants), Claws of
		//      Veeshan (24), Kromzek/Kromrif (9) and the three Seru factions (11) on the friendly
		//      faction and silently pacify five raid zones.
		//   2. MERCHANTS ONLY, not "every npc in a town zone". That looser rule was tried and it drags
		//      in `KOS`, `KOS_animal`, `Noobie Monsters KOS to Guards`, `Sabertooths of Blackburrow`,
		//      `The Forsaken` and `Meldrath` -- newbie monsters near city zones would stop being
		//      hostile. Monsters do not run shops, which is exactly what makes commerce the safe test.
		// ⚠️ GUARDS NEED NO SEPARATE RULE. Guard aggro requires a con of exactly THREATENINGLY or
		// SCOWLS (zone/aggro.cpp), and every civic faction here is now Warmly -- so guards are covered
		// by the same change. Guard-only factions ("Guards of ...") were already caught by section 28's
		// keyword list, which is retained rather than replaced.
		//
		// ⚠️ TWO EXCLUSIONS, both deliberate: 344 `Beta Neutral` and 5032 `Indifferent` are generic
		// catch-alls spanning 63 and 302 NPCs across the world. They carry NO race modifiers and base
		// 0, so they already con Indifferent (5) and already pass the merchant gate -- including them
		// would buy nothing and flip 365 unrelated NPCs friendly.
		// 📌 221 Bloodsabers is excluded for free by the zone bound: its merchants are in `qcat`, the
		// Qeynos catacombs, which is not a town. Its -300 does not stop any race entering Qeynos
		// itself, and covering it would pacify a dungeon.
		//
		// 📌 As of 2026-08-08 the derivation adds 22 factions to section 28's 74, the largest being
		// Dark Bargainers (121 merchants), Citizens of Shar Vahl (81), Coalition of Tradefolk (70),
		// Coldain (61), Traders of the Haven (53) and Haven Defenders (51). It is a query rather than a
		// list precisely so the next city or expansion does not need anyone to remember.
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_town_zones;
CREATE TEMPORARY TABLE aotv4_town_zones (zone VARCHAR(32) PRIMARY KEY);
INSERT INTO aotv4_town_zones VALUES
  ('qeynos'),('qeynos2'),('qrg'),('freportn'),('freporte'),('freportw'),
  ('halas'),('rivervale'),('erudnext'),('erudnint'),('paineel'),
  ('felwithea'),('felwitheb'),('kaladima'),('kaladimb'),('akanon'),
  ('neriaka'),('neriakb'),('neriakc'),('grobb'),('oggok'),
  ('cabeast'),('cabwest'),('thurgadina'),('thurgadinb'),
  ('sharvahl'),('shadowhaven'),('katta'),
  ('gfaydark'),('firiona');

INSERT IGNORE INTO aotv4_town_factions (faction_id, name)
SELECT f.id, f.name
  FROM aotv4_town_zones t
  JOIN spawn2 s          ON s.zone = t.zone
  JOIN spawnentry se     ON se.spawngroupID = s.spawngroupID
  JOIN npc_types n       ON n.id = se.npcID AND n.merchant_id > 0
  JOIN npc_faction nf    ON nf.id = n.npc_faction_id
  JOIN faction_list f    ON f.id = nf.primaryfaction
 WHERE nf.primaryfaction > 0
   AND nf.primaryfaction NOT IN (344, 5032)
 GROUP BY f.id;

DROP TEMPORARY TABLE aotv4_town_zones;

INSERT INTO faction_list (id, name, base) VALUES (9000, 'Titan''s Resistance', 750);

CREATE TABLE IF NOT EXISTS aotv4_faction_primary_backup (
  npc_faction_id     INT NOT NULL,
  old_primaryfaction INT NOT NULL,
  PRIMARY KEY (npc_faction_id)
);

INSERT IGNORE INTO aotv4_faction_primary_backup (npc_faction_id, old_primaryfaction)
SELECT nf.id, nf.primaryfaction
  FROM npc_faction nf
  JOIN aotv4_town_factions t ON t.faction_id = nf.primaryfaction;

UPDATE npc_faction nf
  JOIN aotv4_town_factions t ON t.faction_id = nf.primaryfaction
   SET nf.primaryfaction = 9000;

INSERT IGNORE INTO aotv4_town_factions (faction_id, name) VALUES (9000, 'Titan''s Resistance');
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 42,
		.description = "2026_08_08_aotv4_remove_outdoor_only_spell_restriction",
		// Outdoor-only spells (Camouflage, Invoke Lightning, Sow-likes, the Druid/Ranger utility lines)
		// become castable anywhere. On live the restriction is flavour; here it is a tax that falls
		// unevenly, because the reward picker hands these spells to any of the sixteen classes and a
		// player has no say in getting one they can only use half the time.
		//
		// ⚠️ `spells_new.zonetype` is: **1 = outdoors only, 2 = dungeons only, -1 = anywhere**
		// (common/spdat.h:1646 -- "01=Outdoors, 02=dungeons, ff=Any"). Setting it to -1 is what opens
		// them; the server test is `zone_type == 1`, which then can never match.
		//
		// ⚠️ Only **zonetype 1** is touched, and only that value is actually enforced. The enforcement
		// is exactly two sites, both testing `== 1`:
		//     zone/spells.cpp:722   the player casting path
		//     zone/bot.cpp:9722     the bot equivalent
		// 📌 `zonetype 2` (dungeons only -- Grow, Growth, Diminution) is checked NOWHERE in the server,
		// so those four are already usable anywhere and are deliberately left alone rather than
		// rewritten for tidiness. `zonetype 0` (29 rows, all NPC spells) is likewise unenforced.
		//
		// ⚠️ Scoped to `id < 45000`. RoF2 caps spell links and the spellbook packet below 45000
		// (section 14), so anything above it is unreachable and changing it would be noise.
		//
		// ⚠️⚠️ THE CLIENT HAS ITS OWN COPY OF THIS FIELD. `spells_us.txt` carries zonetype, so a
		// player whose client file predates this change may still refuse to send the cast -- the same
		// three-layer problem recorded for combat skills (section 4) and the bow minimum range
		// (section 39): client display, client send, server execute. **Re-export the client files
		// (`./export_client_files`) and ship `spells_us.txt` alongside this**, or it will look fixed on
		// the server and broken in game.
		//
		// ⚠️ REVERSIBLE: `aotv4_spell_zonetype_backup` records the original value per spell first.
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY: stop the stack, run ./shared_memory, restart. The
		// migration applying at world boot changes nothing on its own.
		// ⚠️⚠️ MOUNTS ARE EXCLUDED, AND THEY ARE **345 OF THE 419** -- opening zonetype 1 blindly is
		// overwhelmingly a MOUNT change wearing a utility-spell costume. SPA 113 `SummonHorse` is how
		// EQ keeps mounts outdoors, and a mount summoned inside a dungeon is a large model in geometry
		// never built for it. Excluding it leaves exactly the 74 spells actually being asked for:
		// Camouflage, Invoke Lightning, Harmony, Breath of Karana, Fist of Karana, Illusion: Tree, the
		// Drifting Cloud line and friends.
		// 📌 There is a SECOND, independent mount guard at zone/client_packet.cpp:683 --
		// `if (RuleB(Character, PreventMountsFromZoning) || !zone->CanCastOutdoor())` fades the
		// SummonHorse buff on entering an indoor zone. So a mount would still drop on zoning even
		// without the zonetype check; what the zonetype check stops is SUMMONING one while already
		// inside. Both are wanted, so this leaves mounts entirely alone.
		// ⚠️ The test is the EFFECT (SPA 113), never the spell name -- "Journeyman's Boots" and every
		// class's mount line share no naming convention.
		.check       = "SELECT id FROM spells_new WHERE zonetype = 1 AND id < 45000 AND 113 NOT IN (effectid1, effectid2, effectid3, effectid4) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_spell_zonetype_backup (
  spell_id     INT NOT NULL,
  old_zonetype INT NOT NULL,
  PRIMARY KEY (spell_id)
);

INSERT IGNORE INTO aotv4_spell_zonetype_backup (spell_id, old_zonetype)
SELECT id, zonetype FROM spells_new
 WHERE zonetype = 1 AND id < 45000
   AND 113 NOT IN (effectid1, effectid2, effectid3, effectid4);

UPDATE spells_new SET zonetype = -1
 WHERE zonetype = 1 AND id < 45000
   AND 113 NOT IN (effectid1, effectid2, effectid3, effectid4);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 43,
		.description = "2026_08_08_aotv4_npcs_do_not_cast_fear",
		// NPCs stop casting fear. This is the second half of "nothing runs":
		//   * AoT:NPCsNeverFlee (zone/fearpath.cpp) stops a creature fleeing from LOW HEALTH.
		//   * The pool generator now prunes SPA 23, so a player can never be offered a fear spell.
		//   * This removes fear from NPC spell lists, so nothing casts it at a player either.
		// ⚠️⚠️ THE FLEE RULE DOES NOT COVER FEAR, AND CANNOT. A feared mob enters through
		// StartFleeing, not through Mob::CheckFlee's low-health path -- the guard is deliberately
		// placed after the already-fleeing block so feared mobs keep moving rather than freezing.
		// Fear therefore had to be closed at the source on both sides, which is what these two changes
		// do. Once it is, the flee guard's fear caveat is moot in practice, but the guard stays where
		// it is: moving it would break fear the moment any of this is reverted.
		//
		// 46 npc_spells lists carry a fear spell. Their entries are removed, not the spells themselves
		// -- `spells_new` is untouched, so an NPC scripted to cast one directly still can and nothing
		// referencing those spell ids dangles.
		// ⚠️ SPA 23 only, checked across ALL TWELVE effect slots. SPA 102 `Fearless` is fear IMMUNITY,
		// the opposite, and must not be caught.
		// ⚠️ REVERSIBLE: `aotv4_npc_fear_backup` stores each removed (npc_spells_id, spellid) pair.
		// ⚠️ `npc_spells` is loaded at ZONE BOOT and is NOT shared memory (only items and spells_new
		// are), so this needs a zone restart and no ./shared_memory run.
		// 📌 Item clickies and procs that fear are NOT covered here -- they are a separate, much
		// smaller surface, and none is currently reachable in the reward pool.
		.check       = "SELECT ne.spellid FROM npc_spells_entries ne JOIN spells_new s ON s.id = ne.spellid WHERE 23 IN (s.effectid1,s.effectid2,s.effectid3,s.effectid4,s.effectid5,s.effectid6,s.effectid7,s.effectid8,s.effectid9,s.effectid10,s.effectid11,s.effectid12) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_npc_fear_backup (
  npc_spells_id INT NOT NULL,
  spellid       INT NOT NULL,
  PRIMARY KEY (npc_spells_id, spellid)
);

INSERT IGNORE INTO aotv4_npc_fear_backup (npc_spells_id, spellid)
SELECT ne.npc_spells_id, ne.spellid
  FROM npc_spells_entries ne
  JOIN spells_new s ON s.id = ne.spellid
 WHERE 23 IN (s.effectid1,s.effectid2,s.effectid3,s.effectid4,s.effectid5,s.effectid6,
              s.effectid7,s.effectid8,s.effectid9,s.effectid10,s.effectid11,s.effectid12);

DELETE ne FROM npc_spells_entries ne
  JOIN spells_new s ON s.id = ne.spellid
 WHERE 23 IN (s.effectid1,s.effectid2,s.effectid3,s.effectid4,s.effectid5,s.effectid6,
              s.effectid7,s.effectid8,s.effectid9,s.effectid10,s.effectid11,s.effectid12);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 44,
		.description = "2026_08_08_aotv4_bash_ac_divisor",
		// Shield AC contributes more to BASH DAMAGE: Combat:BashACBonusDivisor 25 -> 10.
		// This is the third of three shield changes and the only one that is pure data:
		//   1. AoT:ShieldMeleeHatePct  -- threat on every swing and every spell while a shield is worn
		//   2. AoT:BashThreatMultiplier / BashACCapMultiplier -- Bash's threat, and its AC damage cap
		//   3. this -- how much of the shield's AC reaches Bash's damage at all
		//
		// ⚠️⚠️ THE DIVISOR ALONE WOULD HAVE DONE ALMOST NOTHING -- THE CAP WAS THE BINDING LIMIT.
		// Mob::GetBaseSkillDamage caps the AC contribution at exactly `skill/10`, so at 300 skill AC
		// could never add more than 30 regardless of the shield, and at divisor 25 you would need 750
		// AC on a single shield to reach that cap. Measured against our own items -- shields usable at
		// level 30 average **74 AC** and top out at 390 -- the AC term was worth about **3 damage**.
		// Both levers therefore move together: this halves-and-a-bit the divisor, and
		// AoT:BashACCapMultiplier (2) widens the cap to skill/5.
		// 📌 Resulting shape at 300 skill: an average 74 AC shield goes from ~3 to ~7 AC damage
		// (35 -> 39 total, the "small buff"), while a best-in-slot 390 AC shield goes from 3 to 39
		// (35 -> 71). The point is that the number now RESPONDS to shield quality, which is what
		// "encourage tanks with shields" actually requires -- a flat base bump would not.
		// ⚠️ This is a stock EQEmu rule, not an AoT one, so it also affects any other consumer of
		// BashACBonusDivisor. Checked: Mob::GetBaseSkillDamage is the only reader.
		// ⚠️ Rules are read at ZONE BOOT -- a zone restart or #reloadrules, not just a world restart.
		// ⚠️ rule_values has one row per (ruleset_id, rule_name); scoped by rule_name so every ruleset
		// moves together (section 35).
		.check       = "SELECT rule_value FROM rule_values WHERE rule_name = 'Combat:BashACBonusDivisor' AND rule_value > 2",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE rule_values SET rule_value = '2.0' WHERE rule_name = 'Combat:BashACBonusDivisor';
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 45,
		.description = "2026_08_09_aotv4_remove_npc_accuracy_debuffs",
		// Reported from the first delve: NPCs casting something that gives "a 95 percent chance to
		// miss". It is `6631 Black Cyclone` -- **SPA 216 Accuracy, base -200**, 10 tick duration -- and
		// it is cast in delvea, which is rung 1. There is no cure for it and no counterplay: you swing
		// and miss until it wears off.
		//
		// ⚠️⚠️ IT IS NOT BLINDNESS, WHICH IS THE OBVIOUS SUSPECT AND THE WRONG ONE. Several delve NPCs
		// do cast SPA 20 Blind (Eye of Confusion, Sunbeam, Blinding Ash), but Blind does NOT touch hit
		// chance in this codebase -- its only combat use is the flee/aggro check at zone/aggro.cpp:1167.
		// Chasing "blind" finds four plausible spells and fixes nothing.
		//
		// 8 spells carry a NEGATIVE SPA 216 (Accuracy) or SPA 184 (HitChance) in some slot and are cast
		// by NPCs: Discordling Leap, Girplan Chatter, Gelid Breath, Black Cyclone, Torment of Body,
		// Chimeran Laceration, Drake Decay, Feralize.
		//
		// ⚠️⚠️ THE DEBUFF SLOT IS BLANKED, THE SPELL IS NOT DELETED -- and that distinction matters.
		// Several of these are primarily something else: Gelid Breath is a 1,250 damage nuke that
		// happens to carry -10 hit chance, Torment of Body is a 3,000 damage tap. Removing the spells
		// from npc_spells_entries (the approach v43 used for fear) would delete those attacks outright
		// and quietly gut the encounters. Blanking only the offending slot to 254 leaves every spell
		// casting and doing its real job, minus the mechanic being complained about.
		//
		// ⚠️ ONLY NEGATIVE VALUES. SPA 216/184 with a POSITIVE base is an accuracy BUFF -- our own
		// Ranger aura (43553) is SPA 216 base 15 -- so a blanket "remove SPA 216" would delete it.
		// ⚠️ ALL TWELVE SLOTS are checked, and each slot is tested against its OWN base value. A loose
		// `216 IN (effectid1..4) AND (base1 < 0 OR base2 < 0 ...)` matches spells whose negative is a
		// damage slot and misses debuffs sitting in slot 5+; it produced four false positives on the
		// first pass here.
		// ⚠️ `spells_new` IS SHARED MEMORY: stop the stack, run ./shared_memory, restart.
		// ⚠️ REVERSIBLE via aotv4_accuracy_debuff_backup, which records (spell_id, slot, effectid, base).
		.check       = "SELECT id FROM spells_new WHERE (effectid1 IN (216,184) AND effect_base_value1 < 0) OR (effectid2 IN (216,184) AND effect_base_value2 < 0) OR (effectid3 IN (216,184) AND effect_base_value3 < 0) OR (effectid4 IN (216,184) AND effect_base_value4 < 0) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_accuracy_debuff_backup (
  spell_id  INT NOT NULL,
  slot      INT NOT NULL,
  effectid  INT NOT NULL,
  base      INT NOT NULL,
  PRIMARY KEY (spell_id, slot)
);

INSERT IGNORE INTO aotv4_accuracy_debuff_backup (spell_id, slot, effectid, base)
SELECT id, 1, effectid1, effect_base_value1 FROM spells_new WHERE effectid1 IN (216,184) AND effect_base_value1 < 0;
INSERT IGNORE INTO aotv4_accuracy_debuff_backup (spell_id, slot, effectid, base)
SELECT id, 2, effectid2, effect_base_value2 FROM spells_new WHERE effectid2 IN (216,184) AND effect_base_value2 < 0;
INSERT IGNORE INTO aotv4_accuracy_debuff_backup (spell_id, slot, effectid, base)
SELECT id, 3, effectid3, effect_base_value3 FROM spells_new WHERE effectid3 IN (216,184) AND effect_base_value3 < 0;
INSERT IGNORE INTO aotv4_accuracy_debuff_backup (spell_id, slot, effectid, base)
SELECT id, 4, effectid4, effect_base_value4 FROM spells_new WHERE effectid4 IN (216,184) AND effect_base_value4 < 0;

UPDATE spells_new SET effectid1 = 254, effect_base_value1 = 0 WHERE effectid1 IN (216,184) AND effect_base_value1 < 0;
UPDATE spells_new SET effectid2 = 254, effect_base_value2 = 0 WHERE effectid2 IN (216,184) AND effect_base_value2 < 0;
UPDATE spells_new SET effectid3 = 254, effect_base_value3 = 0 WHERE effectid3 IN (216,184) AND effect_base_value3 < 0;
UPDATE spells_new SET effectid4 = 254, effect_base_value4 = 0 WHERE effectid4 IN (216,184) AND effect_base_value4 < 0;
UPDATE spells_new SET effectid5 = 254, effect_base_value5 = 0 WHERE effectid5 IN (216,184) AND effect_base_value5 < 0;
UPDATE spells_new SET effectid6 = 254, effect_base_value6 = 0 WHERE effectid6 IN (216,184) AND effect_base_value6 < 0;
UPDATE spells_new SET effectid7 = 254, effect_base_value7 = 0 WHERE effectid7 IN (216,184) AND effect_base_value7 < 0;
UPDATE spells_new SET effectid8 = 254, effect_base_value8 = 0 WHERE effectid8 IN (216,184) AND effect_base_value8 < 0;
UPDATE spells_new SET effectid9 = 254, effect_base_value9 = 0 WHERE effectid9 IN (216,184) AND effect_base_value9 < 0;
UPDATE spells_new SET effectid10 = 254, effect_base_value10 = 0 WHERE effectid10 IN (216,184) AND effect_base_value10 < 0;
UPDATE spells_new SET effectid11 = 254, effect_base_value11 = 0 WHERE effectid11 IN (216,184) AND effect_base_value11 < 0;
UPDATE spells_new SET effectid12 = 254, effect_base_value12 = 0 WHERE effectid12 IN (216,184) AND effect_base_value12 < 0;
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version     = 46,
		.description = "2026_08_09_aotv4_fix_crit_aura_skill_limit",
		// ⚠️⚠️ THE ROGUE AND BERSERKER AURAS WERE APPLYING THEIR CRIT BONUS TO **1H BLUNT ONLY**.
		// Reported as "did 2 full swarm delves for each aura and not a single crit ... either stupid
		// unlucky or they don't work". Not luck -- structurally impossible.
		//
		// SPA 169 CriticalHitChance and SPA 330 CriticalDamageMob both use `limit_value` to select
		// WHICH SKILL the bonus applies to (zone/bonuses.cpp:1114 and :1124):
		//     if (limit_value == ALL_SKILLS) CriticalHitChance[HIGHEST_SKILL + 1] += base_value;
		//     else                           CriticalHitChance[limit_value]       += base_value;
		// `ALL_SKILLS` is **-1** (common/skills.h:156). Both auras shipped with limit **0**, and skill
		// 0 is **Skill1HBlunt** -- so a Rogue swinging a dagger (1H Piercing) got nothing at all.
		//
		// ⚠️⚠️ AND IT IS WORSE THAN A MISSING 2 PERCENT, BECAUSE OF HOW THE CRIT GATE WORKS.
		// Mob::TryCriticalHit wraps the entire crit roll in `if (innate_crit || crit_chance)`, and a
		// Rogue has no innate MELEE crit (innate is Warrior/Berserker melee, Ranger archery, Rogue
		// THROWING). With the bonus landing on the wrong skill, crit_chance is 0 and the block never
		// runs -- the character can never critical at all. Fixed, it goes from 0 to roughly 8-9
		// percent, because the bonus is a MODIFIER ON THE DEX ROLL (`dex_bonus += dex_bonus * chance
		// / 100`) rather than a flat percentage. That is why 2 is a sensible number there and would
		// not be as a flat crit rate.
		//
		// 📌 43561 Aura of the Arcane is NOT touched and is NOT broken: SPA 294 CriticalSpellChance
		// ignores limit_value entirely and adds straight to the bonus (bonuses.cpp:1150), so its 2
		// percent has been applying the whole time. It is simply small -- Spells:BaseCritChance is 0,
		// so 2 percent IS the whole crit chance for every caster except a Wizard (who has 5-7 percent
		// innate at level 12+ via Spells:WizCritChance). Deliberately left at 2: the aura is GROUP
		// SHARED, so any number here is granted to every caster in the group at once, and raising it
		// is spell-crit creep multiplied by group size.
		// 📌 The other 13 auras were checked -- no other one passes a skill-limited SPA.
		.check       = "SELECT id FROM spells_new WHERE id IN (43558, 43565) AND effect_limit_value1 <> -1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new SET effect_limit_value1 = -1 WHERE id IN (43558, 43565);
)",
		.content_schema_update = false,
	},	ManifestEntry{
		.version     = 47,
		.description = "2026_08_09_aotv4_zone_exp_multipliers",
		// A hunting-zone experience pass: hub cities 1.5x, open dungeons 3-8x, the Resplendent hub 0.
		//
		// ⚠️⚠️ THIS WAS SUPPLIED AS `REPLACE INTO zone` AND WAS DELIBERATELY NOT APPLIED THAT WAY.
		// REPLACE deletes the row and re-inserts it, so every column absent from the supplied values is
		// reset -- section 35 records this exact trap. The supplied file set
		// **bypass_expansion_check = 0 on all 134 rows**, and the six DoN delve zones (delvea, delveb,
		// stillmoona, stillmoonb, thundercrest, thenest) rely on that flag being **1**: they are
		// expansion 9 on a Classic server, so clearing it makes every one of them refuse entry with
		// "part of an expansion that you do not yet own" -- restoring the exact bug migration v37
		// fixed, across half the delve ladder, and invisibly to any GM (zone/zoning.cpp:367 exempts
		// them). It also put `thenest` version 0 back to `min_level = 66`, which is a second refusal
		// path at a level 30 cap.
		// 📌 So only the ONE column the change is actually about is applied here. Everything else on
		// those 134 rows is left exactly as it is.
		//
		// ⚠️ Matched on (short_name, version), not on `id` -- zone ids are not stable across content
		// imports and several of these zones have many version rows. All 134 supplied rows resolve to
		// a real zone row; 66 of them differ from what is already there.
		// ⚠️ `zone_exp_multiplier` multiplies NORMAL xp, AA xp AND group xp (zone/exp.cpp:130, :294,
		// :443) -- it is not just a kill-xp knob.
		// 📌 The delve zones are all set to 1.00 here, which agrees with migration v16: DoN ships
		// 2.90-3.10 and that was normalised precisely so a delve kill is worth an equivalent
		// open-world kill. No conflict.
		// ⚠️⚠️ `resplendent` is set to **0.00**, which means NO experience of any kind in the hub --
		// including quest and task rewards, since the multiplier is applied to all three paths above.
		// That is presumably intended for a safe hub with no mobs, but it is worth knowing it is not
		// merely "no kill xp".
		// 📌 Highest values supplied: kedge 8, sleeper 8, frozenshadow 7, firiona 5, kaesora 5.
		// ⚠️ Zone config is read at ZONE BOOT and is not shared memory -- restart zones, no
		// ./shared_memory run needed.
		.check       = "SELECT z.short_name FROM zone z WHERE (z.short_name = 'resplendent' AND z.version = 0 AND z.zone_exp_multiplier <> 0.00) OR (z.short_name = 'kedge' AND z.version = 0 AND z.zone_exp_multiplier <> 8.00) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_zone_exp;
CREATE TEMPORARY TABLE aotv4_zone_exp (sn VARCHAR(32), ver INT, mult DECIMAL(6,2), PRIMARY KEY (sn, ver));
INSERT INTO aotv4_zone_exp (sn, ver, mult) VALUES
  ('stillmoona',0,1.0), ('stillmoona',1,1.0), ('stillmoona',2,1.0), ('stillmoona',3,1.0),
  ('stillmoona',4,1.0), ('stillmoona',5,1.0), ('stillmoona',6,1.0), ('stillmoona',7,1.0),
  ('stillmoona',8,1.0), ('stillmoona',9,1.0), ('stillmoonb',0,1.0), ('stillmoonb',1,1.0),
  ('stillmoonb',2,1.0), ('stillmoonb',3,1.0), ('stillmoonb',4,1.0), ('stillmoonb',5,1.0),
  ('stillmoonb',6,1.0), ('stillmoonb',7,1.0), ('thundercrest',0,1.0), ('thundercrest',1,1.0),
  ('thundercrest',2,1.0), ('thundercrest',3,1.0), ('thundercrest',4,1.0), ('thundercrest',5,1.0),
  ('thundercrest',6,1.0), ('thundercrest',7,1.0), ('thundercrest',8,1.0), ('thundercrest',9,1.0),
  ('thundercrest',10,1.0), ('thundercrest',11,1.0), ('thundercrest',12,1.0), ('thundercrest',13,1.0),
  ('thundercrest',14,1.0), ('thundercrest',15,1.0), ('delvea',0,1.0), ('delvea',1,1.0),
  ('delvea',2,1.0), ('delvea',3,1.0), ('delvea',4,1.0), ('delvea',5,1.0),
  ('delvea',6,1.0), ('delvea',7,1.0), ('delvea',8,1.0), ('delvea',9,1.0),
  ('delveb',0,1.0), ('delveb',1,1.0), ('delveb',2,1.0), ('delveb',4,1.0),
  ('delveb',3,1.0), ('delveb',5,1.0), ('delveb',6,1.0), ('thenest',0,1.0),
  ('thenest',3,1.0), ('thenest',1,1.0), ('thenest',4,1.0), ('thenest',5,1.0),
  ('thenest',6,1.0), ('thenest',7,1.0), ('thenest',8,1.0), ('thenest',9,1.0),
  ('thenest',10,1.0), ('thenest',11,1.0), ('thenest',12,1.0), ('thenest',13,1.0),
  ('thenest',14,1.0), ('thenest',15,1.0), ('thenest',16,1.0), ('resplendent',0,0.0),
  ('gfaydark',0,1.5), ('lfaydark',0,1.5), ('crushbone',0,3.0), ('mistmoore',0,3.25),
  ('unrest',0,4.0), ('kedge',0,8.0), ('butcher',0,1.5), ('cauldron',0,3.5),
  ('freportn',0,1.5), ('freportw',0,1.5), ('freporte',0,1.5), ('runnyeye',0,4.0),
  ('beholder',0,3.0), ('rivervale',0,1.5), ('kithicor',0,3.0), ('commons',0,1.5),
  ('ecommons',0,1.5), ('misty',0,1.5), ('nro',0,2.0), ('sro',0,2.0),
  ('befallen',0,3.5), ('oasis',0,2.0), ('oot',0,2.0), ('hateplaneb',0,4.0),
  ('iceclad',0,3.0), ('frozenshadow',0,7.0), ('velketor',0,4.0), ('thurgadina',0,1.5),
  ('eastwastes',0,2.5), ('greatdivide',0,3.5), ('crystal',0,3.0), ('sleeper',0,8.0),
  ('thurgadinb',0,3.0), ('droga',1,3.0), ('droga',0,3.0), ('firiona',0,5.0),
  ('dreadlands',0,3.0), ('frontiermtns',0,2.5), ('timorous',0,4.0), ('karnor',0,2.5),
  ('nurga',1,3.0), ('nurga',0,3.0), ('qeynos',0,1.5), ('qeynos2',0,1.5),
  ('qrg',0,1.5), ('qeytoqrg',0,2.0), ('qey2hh1',0,2.0), ('northkarana',0,2.5),
  ('southkarana',0,2.25), ('eastkarana',0,2.0), ('blackburrow',0,3.0), ('paw',1,3.5),
  ('paw',0,3.5), ('qcat',0,2.0), ('rathemtn',0,4.0), ('lakerathe',0,4.0),
  ('fieldofbone',0,2.0), ('warslikswood',0,2.0), ('cabwest',0,1.5), ('swampofnohope',0,2.0),
  ('lakeofillomen',0,2.0), ('kaesora',0,5.0), ('kurn',0,3.0), ('dalnir',0,4.0),
  ('cabeast',0,1.5), ('veksar',0,3.0);

UPDATE zone z
  JOIN aotv4_zone_exp t ON t.sn = z.short_name AND t.ver = z.version
   SET z.zone_exp_multiplier = t.mult;

DROP TEMPORARY TABLE aotv4_zone_exp;
)",
		.content_schema_update = false,
	},	ManifestEntry{
		.version     = 48,
		.description = "2026_08_09_aotv4_item_rebalance",
		// The 2026-08-09 item pass, in one place. Four changes, all on `items`:
		//   1. TWO-HANDED DAMAGE NERF -- 938 weapons, roughly -15 percent
		//   2. RANGED WEAPON RANGE normalised to 5
		//   3. STATS STRIPPED from items you cannot equip
		//   4. The tradeskill reward gear forced back to NO DROP
		//
		// ⚠️⚠️ THESE ARRIVED AS A 161MB `REPLACE INTO items` DUMP AND MUST NEVER BE APPLIED THAT WAY.
		// That dump (aot_npcs_items_1.0.0.sql) carried a live `USE `peq`;` on line 20, which overrides
		// the database named on the mysql command line -- loading it into a scratch schema applied it
		// to the real one. It also predated a fortnight of work, so REPLACE silently reverted:
		// craft sockets on 8,887 rows, the tradeskill gear rework (v29/v30), and the gear-tier in-place
		// edits (loregroup on 37,576 rows). Its ACTUAL payload was only the four items above.
		// 📌 Diffing that dump against a database it has already been loaded into proves nothing -- it
		// matches itself. Compare against a PRE-IMPORT backup (build/bin/backups/peq-YYYY-MM-DD.tar.gz,
		// which world writes before every migration run) or the comparison is worthless.
		//
		// ============================================================================================
		// 1. TWO-HANDED DAMAGE
		// ============================================================================================
		// 938 base weapons: 2H Blunt 402 (avg 109.6 -> 92.7), 2H Slash 400 (110.0 -> 93.1),
		// 2H Pierce 136 (94.4 -> 80.1). Every one is a reduction; none is a buff.
		// ⚠️ Only BASE ids are listed. The Hallowed and Mythic copies are DERIVED
		// (floor(1.5x) and 2x) and must be regenerated afterwards -- see the deployment note below.
		// Setting tier damage here as well would create two sources of truth for the same number.
		//
		// ============================================================================================
		// 2. RANGED WEAPON RANGE
		// ============================================================================================
		// Every archery (5), arrow (19) and throwing (27) item is set to `range` = 5.
		// ⚠️⚠️ THE VALUE IS 5, NOT 0. The supplied dump zeroed 735 of them, and a range of 0 makes the
		// weapon unusable rather than short-ranged. Written as "normalise every ranged item to 5"
		// rather than "fix the 735 that were zeroed", because the latter is not reproducible on a
		// server that never had the dump applied -- and it also sweeps up 13 items that were already
		// sitting at 0 beforehand (Honed Obsidian War Bow and three bane arrows among them).
		// 📌 This pairs with AoT:BowMinRangeIsMeleeRange (section 39): bows already fire inside melee
		// range, so a short maximum range is the coherent other half of that design.
		//
		// ============================================================================================
		// 3. STATS ON NON-EQUIPPABLE ITEMS
		// ============================================================================================
		// Anything with `slots = 0` loses ac/hp/mana/endur, the seven core stats and the five resists.
		// 6,012 rows: 2,362 misc components, 1,871 food, 1,171 reagents, 338 spell scrolls, 152 drink,
		// 35 keys, 32 books. None of it was reachable -- stats on an item you cannot wear do nothing.
		//
		// ⚠️⚠️ TWO EXCLUSIONS, AND BOTH ARE LOAD BEARING:
		//   * `augtype > 0` -- an AUGMENT legitimately carries stats with no wearable slot; that is its
		//     entire purpose. 9,324 augments qualify, including all 316 delve augments.
		//   * NOT IN `tribute_levels` -- **144 tribute items have `slots = 0` and their stats ARE
		//     delivered**. Section 6 records that tributes are not a separate mechanism: CalcBonuses
		//     resolves a tribute to an ITEM and runs it through AddItemBonuses(..., is_tribute = true).
		//     Stripping them silently zeroes the entire tribute system -- no error, no message, and
		//     nothing in game to connect the cause to the effect.
		// ⚠️ Scoped `id < 300000` so tier copies are left to the tier script, which regenerates them.
		//
		// ============================================================================================
		// 4. TRADESKILL REWARD GEAR BACK TO NO DROP
		// ============================================================================================
		// ⚠️⚠️ `nodrop = 0` MEANS NO DROP -- THE FLAG IS INVERTED (section 32, client_packet.cpp:10755
		// tests `NoDrop == 0`). All 36 pieces had flipped to 1, i.e. TRADEABLE, so an earned reward
		// could be handed to somebody who had not earned it. This is the second time that exact flag
		// has bitten on these same items.
		//
		// ⚠️⚠️⚠️ DEPLOYMENT -- THIS MIGRATION IS NOT SUFFICIENT ON ITS OWN. After it applies, run in
		// this order (section 35), or Hallowed and Mythic weapons keep PRE-NERF damage while the row
		// counts look perfectly healthy:
		//     1. custom/sql/aotv4_gear_tiers.sql     (regenerates both tiers from the new native data)
		//     2. custom/sql/aotv4_craft_sockets.sql  (must follow -- the regen drops tier sockets)
		//     3. UPDATE items SET nodrop = 0 WHERE id BETWEEN 147930 AND 147965;   (the tier script
		//        sets nodrop across its scope; re-assert afterwards)
		//     4. stop the stack, ./shared_memory, restart   (`items` is shared memory)
		// ⚠️ VERIFY BY RATIO, NEVER BY ROW COUNT: `Mythic damage = 2x base`, `Hallowed = floor(1.5x)`,
		// and `Mythic damage <> Hallowed damage` -- section 35 records 3,166 Mythics once having LESS
		// AC than their own base while every count looked correct.
		.check       = "SELECT id FROM items WHERE id = 25615 AND damage <> 33",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_2h_nerf;
CREATE TEMPORARY TABLE aotv4_2h_nerf (id INT PRIMARY KEY, dmg INT);
INSERT INTO aotv4_2h_nerf (id, dmg) VALUES
  (1173,54), (1715,50), (1738,30), (1868,62), (2100,64), (2580,184), (2894,55), (2909,48),
  (2928,62), (2931,53), (3349,85), (3352,40), (3353,48), (3355,60), (3368,59), (3372,17),
  (3375,91), (3379,99), (3434,83), (3437,75), (3616,61), (3811,66), (5005,29), (5006,19),
  (5010,26), (5011,28), (5012,30), (5024,24), (5025,21), (5030,28), (5031,31), (5035,18),
  (5036,29), (5037,28), (5049,15), (5052,41), (5053,47), (5054,60), (5062,23), (5070,14),
  (5115,45), (5162,116), (5203,53), (5301,19), (5304,41), (5308,72), (5312,19), (5322,48),
  (5351,68), (5356,38), (5358,73), (5359,78), (5361,49), (5362,64), (5365,38), (5391,41),
  (5392,170), (5401,102), (5404,79), (5407,94), (5411,91), (5412,117), (5424,88), (5518,57),
  (5519,49), (5531,94), (5603,190), (5604,160), (5606,209), (5607,135), (5608,96), (5609,127),
  (5610,98), (5611,38), (5612,121), (5613,54), (5615,102), (5617,65), (5619,79), (5620,128),
  (5621,65), (5622,140), (5623,87), (5624,139), (5625,145), (5626,77), (5627,54), (5629,107),
  (5630,38), (5631,48), (5632,33), (5633,29), (5657,74), (5658,113), (5659,68), (5662,131),
  (5663,131), (5664,117), (5667,235), (5674,40), (5675,37), (5676,154), (5708,161), (5709,145),
  (5710,151), (5818,141), (5840,197), (6004,16), (6005,19), (6013,19), (6020,24), (6021,37),
  (6200,146), (6205,98), (6300,28), (6302,66), (6310,21), (6312,29), (6321,58), (6322,28),
  (6327,33), (6352,48), (6353,114), (6377,49), (6383,90), (6405,33), (6406,35), (6591,86),
  (6635,102), (6636,156), (6637,132), (6638,128), (6639,38), (6640,65), (6641,51), (6642,56),
  (6644,58), (6645,48), (6646,110), (6647,119), (6649,55), (6650,26), (6651,61), (6652,89),
  (6653,97), (6654,89), (6656,77), (6657,77), (6658,74), (6661,31), (6662,30), (6663,41),
  (6664,31), (6665,27), (6667,205), (6668,180), (6669,165), (6670,145), (6671,198), (6673,84),
  (6674,62), (6675,68), (6676,41), (6677,34), (6678,61), (6680,54), (6681,83), (6682,70),
  (6683,66), (6684,70), (6685,108), (6686,111), (6693,132), (6715,73), (6722,53), (6905,31),
  (6908,36), (6912,51), (6914,41), (6921,48), (6935,226), (6938,38), (7056,67), (7069,34),
  (7174,109), (7178,109), (7518,102), (7539,95), (7560,125), (7561,108), (7562,125), (7579,97),
  (7795,89), (7800,117), (7896,23), (7900,34), (7901,40), (7902,186), (7907,168), (7932,133),
  (7933,31), (7956,144), (8100,116), (8137,318), (8312,83), (8350,118), (8741,106), (8745,129),
  (8900,102), (10686,91), (10844,140), (10858,164), (10891,139), (10892,221), (10904,64), (10914,215),
  (10994,23), (11028,117), (11050,155), (11058,113), (11536,25), (11537,29), (11539,30), (11540,25),
  (11543,120), (11546,30), (11550,57), (11559,142), (11564,94), (11566,41), (11567,41), (11569,59),
  (11608,169), (11609,152), (11611,129), (11628,180), (11629,147), (11647,142), (11649,162), (11662,161),
  (11665,142), (11667,154), (11891,32), (11895,40), (11896,47), (11900,36), (11901,32), (11907,20),
  (11927,44), (11955,26), (11973,102), (11977,80), (12526,40), (12530,50), (12531,58), (12540,70),
  (12544,88), (12545,103), (12762,17), (12842,54), (12870,14), (13314,123), (13324,38), (13815,42),
  (14383,158), (14840,78), (14841,78), (14842,76), (14844,44), (14849,31), (14865,80), (20711,105),
  (20725,172), (20753,112), (20779,132), (20792,192), (20870,137), (20983,106), (21504,25), (21515,35),
  (21516,32), (21526,36), (21527,45), (21531,36), (21532,42), (21542,36), (21543,35), (21570,33),
  (21846,104), (21980,78), (22817,104), (22818,144), (22820,191), (22906,33), (22910,32), (24589,144),
  (24618,185), (24624,219), (24628,140), (24640,180), (24642,200), (24648,165), (24707,15), (24728,124),
  (24778,125), (24780,128), (24782,97), (24783,136), (24785,100), (24787,135), (24789,175), (24790,153),
  (24791,101), (24793,149), (24799,110), (24879,186), (24882,111), (24891,106), (24998,91), (24999,152),
  (25002,73), (25004,110), (25079,151), (25083,120), (25175,20), (25186,160), (25189,159), (25196,167),
  (25204,137), (25220,136), (25222,117), (25223,140), (25226,99), (25227,130), (25228,174), (25313,56),
  (25449,77), (25560,60), (25582,61), (25595,73), (25600,156), (25604,44), (25615,33), (25616,14),
  (25647,39), (25800,117), (25843,82), (25847,86), (25848,130), (25849,97), (25860,126), (25979,165),
  (25980,101), (25981,112), (25982,144), (25983,135), (25986,267), (25987,155), (25989,158), (25991,204),
  (25994,99), (25996,137), (25997,160), (25999,154), (26001,116), (26007,88), (26030,99), (26031,194),
  (26032,139), (26036,242), (26038,99), (26039,159), (26041,122), (26043,144), (26045,153), (26046,129),
  (26049,158), (26079,103), (26081,146), (26083,91), (26085,164), (26086,196), (26087,163), (26092,167),
  (26095,90), (26509,203), (26520,183), (26525,108), (26587,175), (26598,167), (26599,123), (26738,93),
  (27037,101), (27038,129), (27039,121), (27040,118), (27041,116), (27042,134), (27043,131), (27044,127),
  (27045,121), (27046,59), (27047,104), (27048,116), (27049,111), (27050,121), (27051,103), (27052,114),
  (27053,129), (27054,123), (27055,132), (27056,59), (27057,104), (27058,131), (27059,119), (27060,115),
  (27061,115), (27062,134), (27063,126), (27064,123), (27065,121), (27066,59), (27077,101), (27078,126),
  (27079,124), (27080,122), (27081,121), (27082,132), (27083,125), (27084,97), (27085,124), (27086,59),
  (27303,73), (27304,73), (27305,73), (27307,73), (27323,76), (27324,75), (27325,76), (27327,75),
  (27737,117), (27748,151), (27795,67), (27799,67), (27913,171), (27914,142), (27915,132), (27917,178),
  (27918,145), (27919,158), (27920,160), (27921,157), (27922,100), (27923,75), (27924,116), (27925,113),
  (27928,66), (27949,160), (27961,153), (28576,54), (28583,52), (28747,79), (28763,124), (28814,160),
  (28817,31), (28824,172), (28825,185), (28826,136), (28838,126), (28845,171), (28846,110), (28849,130),
  (28850,139), (28852,170), (28941,171), (28961,176), (28969,179), (28974,161), (29098,70), (29118,40),
  (29121,156), (29172,173), (29232,21), (29237,18), (29242,35), (29283,68), (29288,55), (29327,207),
  (29408,139), (29413,122), (29423,73), (29424,73), (29425,75), (29427,73), (29434,73), (29436,76),
  (29437,73), (29440,75), (29603,92), (29608,71), (29631,22), (29643,102), (29679,94), (29871,89),
  (30101,38), (30201,19), (30206,29), (30211,42), (30216,29), (30220,96), (30265,29), (30277,32),
  (30503,84), (30511,71), (30530,168), (31209,120), (31232,195), (31242,137), (31305,160), (31317,172),
  (31344,185), (31350,154), (31356,189), (31375,173), (31383,192), (31395,192), (31398,180), (31415,186),
  (31747,51), (31795,94), (31822,58), (32148,112), (32152,14), (32174,132), (32175,168), (32197,97),
  (32198,51), (32199,91), (32200,69), (32305,130), (32330,129), (32331,162), (39152,146), (39254,178),
  (39261,213), (39294,149), (40030,97), (40032,55), (40033,133), (40034,79), (40035,141), (40036,74),
  (40037,124), (40038,146), (40039,111), (40206,177), (40321,63), (45064,38), (45065,20), (45073,31),
  (45074,33), (45078,36), (45079,37), (45087,51), (45088,51), (45092,76), (45093,61), (45101,79),
  (45102,67), (45106,91), (45107,76), (45115,119), (45116,138), (45120,130), (45121,116), (45129,185),
  (45130,130), (45134,134), (45135,134), (46089,47), (46167,36), (46185,194), (46240,33), (46250,49),
  (46321,122), (46357,90), (46390,128), (46407,129), (46453,140), (46498,185), (46740,92), (47261,127),
  (47262,133), (47263,142), (47264,150), (47320,148), (47321,150), (47322,173), (47323,164), (47324,189),
  (51261,113), (52058,139), (52163,177), (52168,140), (52510,104), (55234,158), (55262,98), (58911,26),
  (58912,26), (58913,26), (58914,29), (58915,66), (58916,65), (58917,35), (58918,27), (58919,27),
  (58920,32), (58921,34), (58922,63), (58923,60), (58924,37), (58926,33), (58933,33), (58934,60),
  (58935,33), (58936,60), (58937,60), (58938,43), (58939,44), (58941,45), (58948,39), (58949,39),
  (58950,35), (58951,46), (58952,46), (58953,43), (58954,50), (58955,58), (58956,49), (58969,29),
  (58970,31), (58971,30), (58972,32), (58973,33), (58974,33), (58975,37), (58976,29), (58977,30),
  (58978,36), (58979,36), (58980,36), (58981,40), (58982,39), (58983,39), (58984,41), (58991,37),
  (58992,36), (58993,36), (58994,42), (58995,44), (58996,45), (58997,47), (58998,46), (58999,47),
  (59953,11), (62118,23), (62122,30), (62148,53), (62149,27), (62169,53), (62171,29), (62187,47),
  (62189,42), (62280,112), (62437,193), (62438,146), (63177,54), (63178,41), (63179,37), (63180,49),
  (63181,49), (63182,46), (63183,52), (63184,51), (63185,46), (63412,26), (63413,28), (63414,28),
  (63415,29), (63416,32), (63417,32), (63418,35), (63419,27), (63420,27), (63421,32), (63422,34),
  (63423,34), (63424,37), (63425,37), (63427,43), (63434,33), (63435,36), (63437,41), (63438,60),
  (63439,43), (63440,44), (63442,43), (63449,57), (63450,55), (63451,50), (63452,46), (63453,45),
  (63454,43), (63455,50), (63456,50), (63457,50), (63470,29), (63471,30), (63472,30), (63473,32),
  (63474,34), (63475,33), (63476,37), (63478,30), (63479,36), (63480,36), (63481,36), (63482,41),
  (63483,41), (63484,39), (63485,45), (63492,36), (63493,36), (63494,36), (63495,44), (63496,44),
  (63497,45), (63498,47), (63499,46), (63500,46), (63507,41), (63508,41), (63509,37), (63510,47),
  (63511,47), (63512,46), (63513,51), (63514,52), (63515,48), (63644,26), (63645,28), (63646,28),
  (63647,29), (63648,30), (63649,32), (63650,35), (63651,27), (63652,27), (63653,32), (63654,34),
  (63655,34), (63656,37), (63657,37), (63658,37), (63659,42), (63666,35), (63667,33), (63668,33),
  (63669,42), (63670,41), (63671,43), (63672,44), (63673,44), (63674,45), (63681,39), (63682,39),
  (63683,36), (63684,45), (63685,46), (63686,43), (63687,50), (63688,50), (63689,44), (63702,29),
  (63703,30), (63704,31), (63705,32), (63706,34), (63707,34), (63708,37), (63709,30), (63710,30),
  (63711,36), (63712,36), (63713,36), (63714,41), (63715,39), (63716,39), (63717,43), (63724,37),
  (63725,37), (63726,35), (63727,44), (63729,45), (63730,46), (63731,45), (63732,46), (63739,40),
  (63740,54), (63741,36), (63742,49), (63743,47), (63744,44), (63745,52), (63746,52), (63747,46),
  (65506,46), (65510,57), (65549,87), (65550,53), (67186,102), (67286,80), (67287,61), (67290,48),
  (67291,36), (67383,17), (68199,165), (68262,176), (68266,26), (68268,34), (68359,91), (68368,200),
  (68369,98), (68439,153), (68442,139), (68506,120), (68570,161), (68659,201), (68741,195), (68771,107),
  (68838,149), (68932,115), (68941,68), (68947,72), (69043,188), (69044,153), (69057,173), (69096,153),
  (69112,165), (69113,186), (69117,204), (69245,37), (69297,83), (69408,150), (69410,225), (69411,158),
  (69420,130), (69423,157), (69427,162), (70007,114), (70017,91), (70065,161), (70103,117), (70128,170),
  (70138,157), (70161,138), (70214,113), (70228,155), (70251,152), (70275,135), (70318,172), (70494,143),
  (70520,158), (70530,138), (70555,150), (70564,157), (70575,132), (70590,137), (70612,142), (70627,149),
  (70647,166), (70667,164), (70709,218), (71243,77), (71338,126), (71378,15), (71468,13), (71512,15),
  (71537,62), (71547,146), (71555,21), (71595,14), (71617,24), (71637,87), (71653,144), (71663,125),
  (71673,157), (82726,164), (82734,119), (82955,25), (82956,20), (83239,88), (83264,129), (83283,137),
  (83304,176), (83324,183), (83367,155), (83401,174), (83453,178), (83482,157), (88088,128), (89310,87),
  (89350,151), (89530,176), (89781,134), (101056,146), (101167,184), (101173,173), (101247,182), (101344,181),
  (101346,155), (102821,82), (109739,207), (110244,177), (110291,191), (110292,155), (110294,196), (110739,207),
  (110745,210), (110794,173), (111521,180), (120168,197), (120573,221), (120971,205), (121373,220), (127325,217),
  (128126,248), (128128,245);

UPDATE items i JOIN aotv4_2h_nerf n ON n.id = i.id SET i.damage = n.dmg;
DROP TEMPORARY TABLE aotv4_2h_nerf;

UPDATE items SET `range` = 5 WHERE itemtype IN (5, 19, 27) AND id < 300000 AND `range` <> 5;

UPDATE items
   SET ac=0, hp=0, mana=0, endur=0,
       astr=0, asta=0, aagi=0, adex=0, awis=0, aint=0, acha=0,
       fr=0, cr=0, dr=0, pr=0, mr=0
 WHERE slots = 0
   AND augtype = 0
   AND id < 300000
   AND id NOT IN (SELECT item_id FROM tribute_levels)
   AND (ac>0 OR hp>0 OR mana>0 OR endur>0 OR astr>0 OR asta>0 OR aagi>0 OR adex>0
        OR awis>0 OR aint>0 OR acha>0 OR fr>0 OR cr>0 OR dr>0 OR pr>0 OR mr>0);

UPDATE items SET nodrop = 0 WHERE id BETWEEN 147930 AND 147965;
)",
		.content_schema_update = false,
	},	ManifestEntry{
		.version     = 49,
		.description = "2026_08_09_aotv4_disable_fabled_and_legendary",
		// Fabled and Legendary NPCs stop spawning. 16 active spawn rows: 10 Fabled and 6 Legendary.
		//
		// ⚠️⚠️ THEY ARE WILDLY OUT OF SCALE FOR A LEVEL 30 CAP, AND SOME SIT IN NEWBIE ZONES.
		// The Legendary set runs level 37-93 and includes A_Legendary_Hill_Giant (72) in **commons**
		// and A_Legendary_Behemoth (72) in **steamfont** -- both starter hunting zones. The Fabled set
		// reaches level 75. Nothing at the cap can fight either.
		//
		// ⚠️⚠️ `chance = 0` RATHER THAN DELETING ROWS, AND IT IS CORRECT FOR BOTH SPAWN SHAPES:
		//   * FABLED share a spawngroup with the ordinary version of the mob (2-11 entries per group).
		//     Zeroing their weight means the normal NPC spawns in their place -- the spawn point stays
		//     populated, which deleting the row would not guarantee.
		//   * LEGENDARY are ALONE in their spawngroup (5 of 6) at chance 100. With the only entry at 0
		//     the weighted pick has nothing to choose and the point simply stays empty, which is the
		//     intended outcome for a mob that should not exist yet.
		// So one statement covers both, and no spawn data is destroyed.
		//
		// ⚠️ Matched on the NPC NAME, because there is no flag for this -- EQEmu has no fabled/legendary
		// rule, column or code path; they are ordinary npc_types that happen to be named that way.
		// Checked: no AoTv4 npc (2000000+) matches either pattern, so nothing of ours is caught.
		// ⚠️ The npc_types rows are LEFT ALONE. They stay summonable with #npcspawn and can be brought
		// back by restoring the chances -- this is "off for now", not a deletion.
		// ⚠️ REVERSIBLE: aotv4_fabled_spawn_backup records (spawngroupID, npcID, chance) first.
		// ⚠️ Spawn data is read at ZONE BOOT and is not shared memory -- restart zones, no
		// ./shared_memory run needed.
		// 📌 If these are ever wanted back, they want a level pass first, not just their chances
		// restored -- section 24's delve scaling exists because fixed high-level content does not work
		// at this cap.
		.check       = "SELECT se.npcID FROM spawnentry se JOIN npc_types n ON n.id = se.npcID WHERE (n.name LIKE '%Fabled%' OR n.name LIKE '%Legendary%') AND se.chance > 0 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TABLE IF NOT EXISTS aotv4_fabled_spawn_backup (
  spawngroupID INT NOT NULL,
  npcID        INT NOT NULL,
  chance       INT NOT NULL,
  PRIMARY KEY (spawngroupID, npcID)
);

INSERT IGNORE INTO aotv4_fabled_spawn_backup (spawngroupID, npcID, chance)
SELECT se.spawngroupID, se.npcID, se.chance
  FROM spawnentry se JOIN npc_types n ON n.id = se.npcID
 WHERE n.name LIKE '%Fabled%' OR n.name LIKE '%Legendary%';

UPDATE spawnentry se JOIN npc_types n ON n.id = se.npcID
   SET se.chance = 0
 WHERE n.name LIKE '%Fabled%' OR n.name LIKE '%Legendary%';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 50,
		.description = "2026_08_09_aotv4_clamp_over_cap_experience",
		// One-time cleanup for the experience overflow fixed in code this same commit
		// (zone/exp.cpp, Client::SetEXP). `AoT:HardLevelCap` clamped the LEVEL to 30 but the
		// experience ceiling was computed from `Character:MaxExpLevel` (70), so a character parked at
		// the cap kept banking experience toward the level 71 threshold.
		//
		// ⚠️⚠️ IT IS NOT COSMETIC, BECAUSE THE ROGUELITE DEATH PAYS AA OUT OF TOTAL RUN EXPERIENCE at
		// `AA:ExpPerPoint` (200,000). A full climb to the cap is 464,000 = 2.32 points, which is the
		// intended payout; characters found in play were carrying enough to bank **7**, and the
		// engine ceiling (2,555,000) allowed 12.7. The code fix stops it accruing from now on -- this
		// clamps what is ALREADY banked, so the next death cannot pay out over the ceiling.
		//
		// ⚠️⚠️ `level <= cap` IS LOAD BEARING AND MIRRORS THE C++ GUARD EXACTLY. A GM who has
		// #levelled above the cap legitimately holds more experience than this; clamping them would
		// also make `check_level` resolve back to the cap and delevel them on their next kill.
		// Anyone above the cap is left completely alone.
		//
		// ⚠️ The ceiling is DERIVED from the rule, not hardcoded, so it stays correct if live runs a
		// different cap: the curve is `1000 * n * (n + 3) / 2` for n = level - 1 (zone/exp.cpp:1085 --
		// note the documented copy further down that file is inside `#if 0` and is dead code). Both
		// `Character:UseOldRaceExpPenalties` and `UseOldClassExpPenalties` are false, so the figure is
		// identical for every race and class and a single number is correct for everyone.
		//
		// 📌 Already-GRANTED AA points are deliberately NOT clawed back -- players spent them in good
		// faith. Only the unspent experience that has not yet been converted is trimmed.
		.check       = "SELECT cd.id FROM character_data cd JOIN (SELECT COALESCE((SELECT CAST(rule_value AS UNSIGNED) FROM rule_values WHERE rule_name = 'AoT:HardLevelCap' AND ruleset_id = 1 LIMIT 1), 30) AS cap) r WHERE cd.level <= r.cap AND cd.exp > 1000 * (r.cap - 1) * (r.cap + 2) / 2 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE character_data cd
  JOIN (
    SELECT COALESCE((SELECT CAST(rule_value AS UNSIGNED) FROM rule_values
                      WHERE rule_name = 'AoT:HardLevelCap' AND ruleset_id = 1 LIMIT 1), 30) AS cap
  ) r
   SET cd.exp = 1000 * (r.cap - 1) * (r.cap + 2) / 2
 WHERE cd.level <= r.cap
   AND cd.exp  > 1000 * (r.cap - 1) * (r.cap + 2) / 2;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 51,
		.description = "2026_08_09_aotv4_onslaught_task_duration",
		// Onslaught is "the same delve against a 30 minute clock", and its journal entry was claiming
		// SIX HOURS. Reported from play as exactly that mismatch.
		//
		// ⚠️⚠️ v36 IS WHAT BROKE IT, AND IT DID SO SILENTLY. That migration rebuilt all 234 delve tasks
		// as 39 dungeons CROSS JOINed with 6 modes, taking `duration 21600 / duration_code 3` from the
		// original rows for EVERY mode -- overwriting the 1800 that Onslaught alone needs. Nothing
		// errored, because 21600 is a perfectly valid duration; only Onslaught's meaning was lost.
		//
		// ⚠️⚠️ THE CLOCK WAS NEVER WRONG -- ONLY THE LABEL. `M.ONSLAUGHT_SECS` (1800) is enforced in Lua
		// by M.onslaught_tick, so the run really did end at 30 minutes. The task row is what the client
		// renders in the journal, so the player was told six hours and then failed at thirty. That is
		// worse than either number being wrong on its own, and it is why this is a data fix rather than
		// a code one: the behaviour is correct and only `tasks.duration` disagreed with it.
		//
		// ⚠️ Matched on the TITLE, not on an id band. The band is currently 2000460-2000498, but v36
		// already moved the mode offsets once (10 -> 40, because 6 dungeons became 39) and an id-band
		// migration would silently update the wrong mode -- or nothing at all -- if they move again.
		// Verified exact: 39 rows match, and zero rows outside the Onslaught band match.
		//
		// ⚠️ `duration_code` is deliberately left at 3. tasks.h:213 documents it as the descriptor used
		// only "for when duration == 0", so with an explicit duration it has no effect; changing it
		// would be churn that implies a behaviour it does not have.
		.check       = "SELECT id FROM tasks WHERE id BETWEEN 2000300 AND 2000538 AND title LIKE 'Delve:%(Onslaught)' AND duration <> 1800 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE tasks
   SET duration = 1800
 WHERE id BETWEEN 2000300 AND 2000538
   AND title LIKE 'Delve:%(Onslaught)';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 52,
		.description = "2026_08_09_aotv4_disable_melee_push",
		// Melee pushback off. Reported from play as far too strong -- players shoved around constantly
		// in every fight.
		//
		// ⚠️⚠️ THE COMPILED DEFAULT ALONE IS NOT ENOUGH, AND NEITHER IS THIS ROW ALONE. `Combat:MeleePush`
		// is flipped to false in common/ruletypes.h so a server with no row behaves correctly, but an
		// EXISTING row overrides the default -- and this database has one (ruleset 1, 'true'). Both
		// halves ship together; either on its own leaves push enabled somewhere.
		//
		// ⚠️ Scoped by rule_name with NO ruleset filter, per section 35: rule_values holds multiple rows
		// per rule across rulesets, so filtering on ruleset_id fixes one and silently leaves another.
		//
		// ⚠️⚠️ IT ALSO NEEDS THE ZONE BINARY, WHICH IS NOT OPTIONAL HERE. The damage packet in
		// Mob::CommonDamage is `static` and `a->force` is written ONLY inside the push block -- no else,
		// no memset -- so simply disabling the rule leaves the last pushing hit's force sitting in the
		// reused buffer and still being sent. attack.cpp now zeroes it explicitly before the block.
		// Applying this migration against an older zone gives intermittent phantom pushes rather than
		// none, which reads as the setting not working.
		//
		// 📌 `Spells:NPCSpellPush` is deliberately NOT touched. It is a separate mechanic and was not in
		// the report -- though note this database carries it as **true** while the stock compiled default
		// is **false**, which is almost certainly an artifact of the 0.1.2 rules dump (section 35 records
		// that dump silently changing 64 rules). Worth a decision, separately.
		.check       = "SELECT rule_value FROM rule_values WHERE rule_name = 'Combat:MeleePush' AND rule_value <> 'false' LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE rule_values
   SET rule_value = 'false'
 WHERE rule_name = 'Combat:MeleePush';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 53,
		.description = "2026_08_09_aotv4_renumber_refine_crucible",
		// The Refining Crucible moves 2000060 -> 147510, because its chat link was rendering with junk
		// characters in front of the name. Reported from play as "6Refining Crucible".
		//
		// ⚠️⚠️ AN ITEM ID MUST STAY BELOW 0x100000 (1,048,576) OR ITS LINK IS SILENTLY WRONG. RoF2
		// packs the id into a FIVE hex digit field and common/say_link.cpp masks it on the way in:
		//     "%1X" "%05X" ...   (0x000FFFFF & item_id)
		// 2000060 is 0x1E84BC -- six digits -- so the mask throws the top bit away and the link encodes
		// **951484**, an id that does not exist. Nothing errors; the link simply describes another item
		// and the client renders the leftovers. Section 10 already recorded this ceiling as the reason
		// the gear tier step is 300,000 rather than 1,000,000.
		//
		// ⚠️⚠️ IT SURVIVED BECAUSE IT WAS DELIBERATELY EXEMPTED. `aotv4_tier_renumber.sql` moved every
		// other item out of the 1,000,000-2,999,999 band and spared this one on all TEN tables
		// (`AND item_id <> 2000060`), so the single item left above the ceiling is the one players are
		// handed for crafting. Only two items in the whole database exceed it: this, and stock 3008007
		// "Mythic Hallowed Shuriken", which section 35 already flags as a tier lookalike.
		//
		// ⚠️⚠️ 147510 IS CHOSEN TO SIT OUTSIDE EVERY WHOLESALE-DELETE BAND, not merely to be free.
		// `gen_delve_augs.pl` clears 147600-148199 and `aotv4_gear_tiers.sql` clears 1000000-2999999,
		// so an "obvious" spot in either would be erased the next time those regenerate. 147510-147599
		// is the gap between the Delver's Sigils (147500-147509) and the augment band, and is empty.
		//
		// ⚠️⚠️ THE ZONE BINARY MUST SHIP WITH THIS. `AOTV4_REFINE_BAG_ID` in zone/tradeskills.cpp gates
		// the whole refine behaviour on this exact id -- not on bagtype, so real bagtype-30 quest
		// containers are untouched. Migration without the binary leaves a crucible that links correctly
		// and refines nothing; binary without the migration is the reverse.
		//
		// ⚠️ `items` is SHARED MEMORY: world down, ./shared_memory, restart. The migration alone changes
		// nothing a player can see.
		//
		// ⚠️ All ten reference tables from the tier renumber are covered, so a crucible sitting in a
		// bag, a shared bank, a corpse, a parcel, a bandolier, a potion belt, a shop listing or a world
		// container follows the rename instead of becoming an unresolvable id.
		.check       = "SELECT id FROM items WHERE id = 2000060 AND NOT EXISTS (SELECT 1 FROM items i2 WHERE i2.id = 147510) LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE items                  SET id      = 147510 WHERE id      = 2000060;
UPDATE inventory              SET item_id = 147510 WHERE item_id = 2000060;
UPDATE sharedbank             SET item_id = 147510 WHERE item_id = 2000060;
UPDATE character_corpse_items SET item_id = 147510 WHERE item_id = 2000060;
UPDATE character_parcels      SET item_id = 147510 WHERE item_id = 2000060;
UPDATE character_bandolier    SET item_id = 147510 WHERE item_id = 2000060;
UPDATE character_potionbelt   SET item_id = 147510 WHERE item_id = 2000060;
UPDATE trader                 SET item_id = 147510 WHERE item_id = 2000060;
UPDATE object                 SET itemid  = 147510 WHERE itemid  = 2000060;
UPDATE object_contents        SET itemid  = 147510 WHERE itemid  = 2000060;
UPDATE merchantlist           SET item    = 147510 WHERE item    = 2000060;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 54,
		.description = "2026_08_09_aotv4_live_aa_transition",
		// One-time carry-over for switching AA from a lump at death to 1:1 as experience is earned
		// (`AoT:LiveAAExp`, zone/exp.cpp). WITHOUT THIS, TURNING THAT RULE ON SILENTLY DESTROYS
		// PROGRESS, in two ways that no player or log would report:
		//
		//   1. `aa_xp_<charid>` held the carried remainder toward the next point -- up to 199,999,
		//      i.e. nearly a full point. It is read and written ONLY inside the death payout block,
		//      and that block is now skipped, so the value is simply orphaned. A real character on
		//      test was holding 152,224 (76 percent of a point).
		//   2. Experience already accumulated in the current run stops paying. The live path only
		//      credits NEW experience deltas, and the death lump that would have converted the
		//      existing total is gone -- so a character sitting at the level cap loses that whole
		//      climb, 2.32 points. Worst case is about 3.3 points, more than a full run.
		//
		// Both are fixed by crediting the same total into native AA experience, which is what the new
		// system reads. The engine then awards it on the character's next kill (the stored value feeds
		// straight back into the award block in Client::SetEXP) and that award is diverted to the
		// picker's bank like any other. Value preserving, not a grant.
		//
		// WARNING: `exp` IS ADDED BECAUSE IT IS EXACTLY WHAT THE DEATH LUMP WOULD HAVE CONVERTED --
		// the old payout was `run_xp * 1.0` where run_xp is the character's total experience. It is
		// NOT double counting: normal experience is untouched, only mirrored into the AA total once.
		//
		// WARNING: NATIVE UNSPENT POINTS ARE MOVED TO THE PICKER'S BANK, NOT DELETED. The design is
		// that nothing spendable ever sits in the native AA window (`AoT:AAPointsToPicker`), but the
		// C++ divert only catches NEW grants -- anything a character already held before the binary
		// shipped would otherwise stay spendable there forever.
		//
		// Idempotent by construction: it is gated on the old buckets still existing (or on someone
		// still holding native points), and it deletes those buckets, so it cannot run twice. Nothing
		// recreates them -- the only writer was the death block that is now disabled.
		.check       = "SELECT 1 FROM (SELECT (SELECT COUNT(*) FROM data_buckets WHERE `key` LIKE 'aa\\_xp\\_%') + (SELECT COUNT(*) FROM character_data WHERE aa_points > 0) AS n) t WHERE t.n > 0",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE character_data cd
  LEFT JOIN data_buckets b
         ON b.`key` = CONCAT('aa_xp_', cd.id)
   SET cd.aa_exp = cd.aa_exp + cd.exp + COALESCE(CAST(b.value AS UNSIGNED), 0);

INSERT INTO data_buckets (`key`, value, character_id)
SELECT CONCAT('aa_bank_', cd.id), CAST(cd.aa_points AS CHAR), 0
  FROM character_data cd
 WHERE cd.aa_points > 0
    ON DUPLICATE KEY UPDATE value = CAST(CAST(data_buckets.value AS UNSIGNED) + VALUES(value) AS CHAR);

UPDATE character_data SET aa_points = 0 WHERE aa_points > 0;

DELETE FROM data_buckets WHERE `key` LIKE 'aa\_xp\_%';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 55,
		.description = "2026_08_11_aotv4_spell_books",
		// Tomes of Insight: a consumable granting ONE extra reward pick without levelling you up.
		// Three tiers, one per level band (1-10 / 11-20 / 21-30), usable only inside their own band.
		// Behaviour is in lua_modules/aotv4_spell_books.lua; this creates the items and the inert
		// spell their click rides on. Readable copy: custom/sql/aotv4_spell_books.sql.
		//
		// ⚠️⚠️ items AND spells_new ARE SHARED MEMORY. World applying this at boot is not enough --
		// stop the stack, run ./shared_memory, restart, or the new rows are invisible to every zone.
		//
		// ⚠️ Keyed on the LAST id created (147968). Testing for the first would let a half-applied
		// run look complete; testing for the spell would miss an items-only failure.
		.check       = "SELECT id FROM items WHERE id = 147968",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_tome_spell;
CREATE TEMPORARY TABLE aotv4_tome_spell AS SELECT * FROM spells_new WHERE id = 43380;
UPDATE aotv4_tome_spell SET
    id = 44328, name = 'Insight',
    buffduration = 0, buffdurationformula = 0,
    cast_time = 0, recovery_time = 0, recast_time = 0, mana = 0,
    targettype = 6, goodEffect = 1, spell_category = -99,
    classes1=255, classes2=255, classes3=255, classes4=255, classes5=255, classes6=255,
    classes7=255, classes8=255, classes9=255, classes10=255, classes11=255, classes12=255,
    classes13=255, classes14=255, classes15=255, classes16=255;
DELETE FROM spells_new WHERE id = 44328;
INSERT INTO spells_new SELECT * FROM aotv4_tome_spell;
DROP TEMPORARY TABLE aotv4_tome_spell;

DROP TEMPORARY TABLE IF EXISTS aotv4_tome_items;
CREATE TEMPORARY TABLE aotv4_tome_items AS SELECT * FROM items WHERE id = 147921;
UPDATE aotv4_tome_items SET
    nodrop = 0, norent = 1, maxcharges = -1,
    clicktype = 1, clickeffect = 44328, casttime = 0,
    stackable = 1, stacksize = 20, itemtype = 17, slots = 0, loregroup = 0,
    classes = 65535, races = 65535, deity = 0,
    reqlevel = 0, reclevel = 0, price = 0, sellrate = 0,
    augtype = 0,
    augslot1type = 0, augslot2type = 0, augslot3type = 0,
    augslot4type = 0, augslot5type = 0, augslot6type = 0,
    augslot1visible = 0, augslot2visible = 0, augslot3visible = 0,
    augslot4visible = 0, augslot5visible = 0, augslot6visible = 0;
DELETE FROM items WHERE id BETWEEN 147966 AND 147979;
UPDATE aotv4_tome_items SET id = 147966, Name = 'Worn Tome of Insight',    icon = 777;
INSERT INTO items SELECT * FROM aotv4_tome_items;
UPDATE aotv4_tome_items SET id = 147967, Name = 'Etched Tome of Insight',  icon = 865;
INSERT INTO items SELECT * FROM aotv4_tome_items;
UPDATE aotv4_tome_items SET id = 147968, Name = 'Radiant Tome of Insight', icon = 1357;
INSERT INTO items SELECT * FROM aotv4_tome_items;
DROP TEMPORARY TABLE aotv4_tome_items;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 56,
		.description = "2026_08_12_aotv4_tome_spell_description",
		// The tome click showed SHIELD WALL's description -- "Allies are absorbing part of the melee
		// damage you take" -- because v55 cloned spell 43380 (the Shield Wall marker buff) and renamed
		// it without repointing `descnum`, which still named 43380. Same trap as the damage formula in
		// section 5: A CLONE INHERITS EVERY COLUMN YOU DO NOT OVERRIDE, and the ones that hurt are the
		// ones with no obvious connection to what you changed.
		//
		// ⚠️ The spell itself was always INERT and correct (effectid1-3 all 254, no duration) -- it
		// never cast a shield, it only described one. The tome's behaviour lives in
		// quests/items/1479xx.lua and the spell is only the vehicle that makes the item clickable.
		//
		// ⚠️⚠️ A SPELL DESCRIPTION IS RESOLVED BY THE CLIENT FROM ITS OWN `dbstr_us.txt`, NOT FROM THIS
		// DATABASE (section 6). Applying this migration alone changes NOTHING in game: it also needs
		// `./export_client_files` and the regenerated dbstr_us.txt shipped to players.
		// ⚠️ `spells_new` is SHARED MEMORY -- world down, ./shared_memory, restart, or every zone keeps
		// the stale descnum.
		// ⚠️ NO literal '%' in the text: the description path is printf-style and eats it as a format
		// token (section 5).
		//
		// 📌 descnum = the spell id, which is the convention every other custom spell here follows
		// (43380 -> 43380), so nothing has to remember a separate mapping.
		.check       = "SELECT id FROM db_str WHERE id = 44328 AND type = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM db_str WHERE id = 44328 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
    (44328, 6, 'A moment of clarity. Offers three rewards to choose from, without granting a level.');
UPDATE spells_new SET descnum = 44328 WHERE id = 44328;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 57,
		.description = "2026_08_13_aotv4_resplendent_pok_book",
		// The HUB PoK book, moved to where the hub actually is. Readable copy:
		// custom/sql/aotv4_pok_books_extra.sql (which this supersedes for the hub book specifically).
		//
		// ⚠️⚠️ THE HUB BOOK WAS IN `tutorialb` AND THE HUB MOVED WITHOUT IT. `aotv4_pok_books_extra.sql`
		// put the set-granting book at tutorialb doorid 48 -- it unlocks Butcherblock Docks, Commonlands
		// and Qeynos Hills in one click (pok_travel `grant_sets`) and then opens the window so the player
		// ports out. `aotv4_start_resplendent.sql` then made resplendent (729) the start zone for all
		// 1,441 (race, class, deity) rows and turned the tutorial off -- so the only book that grants the
		// starter waypoints, and the only way to open the Portal window at the start, became unreachable.
		// Reported from play as "I don't see the PoK book in Resplendent".
		//
		// ⚠️ tutorialb's book and its `grant_sets` entry are DELIBERATELY LEFT ALONE. They cost nothing,
		// and the tutorial rules have already been reverted once by an unannounced rules dump (§35), so
		// the tutorial may yet be reachable again.
		//
		// ⚠️⚠️ THIS MUST NOT MAKE RESPLENDENT A TRAVEL DESTINATION. `pok_travel.never_attune` already
		// lists it and its comment predicted exactly this change -- the hub is where you are BOUND and
		// you return by DYING, which is the roguelite loop, so a book *to* it would be a free
		// bind-anywhere. Do NOT add resplendent to pok_portals.lua. The tutorialb book is the same
		// shape: a source that is never a destination.
		//
		// ⚠️ Cloned from `butcher` doorid 78 -- a known-good stock PoK book -- so opentype (58),
		// door_param, incline, size and the flags all match something proven to render and to fire
		// EVENT_CLICK_DOOR. Only doorid, zone, position and the expansion gate are overridden. The name
		// stays POKTELE500, the standard book model, which renders in every zone.
		// ⚠️ min/max_expansion -1 = always spawn: the books are PoP-era content on a Classic server
		// (§11), so without this the content filter hides it.
		// ⚠️ `version` is copied from the template (0), which matches every existing resplendent door.
		//
		// ⚠️⚠️ POSITION IS DERIVED, NOT WALKED -- x/y/z -22/548/0 is the START AND BIND POINT
		// (aotv4_start_resplendent.sql) nudged 13 units along y so the model is in front of a player
		// rather than inside them. That point is guaranteed valid standing ground because every
		// character materialises on it, and it is also where they respawn after every death -- the most
		// discoverable spot in the zone. **If it looks wrong in game, correct it with one UPDATE**; the
		// three hub NPCs sit further in at y ~685-697, z ~-26 (Alessa -30.7/696.6/-26.25) if it should
		// live with them instead.
		// ⚠️ In-game `/loc` prints Y,X,Z -- `doors.pos_x/pos_y` are already swapped relative to it, which
		// is the note the extra-books script carries. Do not paste a raw /loc in here.
		// ⚠️ heading 256 = south, facing back toward the arrival point (0 = north, 384 = east).
		//
		// ⚠️ Doors load at ZONE BOOT and are NOT shared memory -- restart zones, no ./shared_memory.
		//
		// ⚠️⚠️⚠️ IT MUST NOT ADD A SECOND BOOK ON LIVE, WHERE ONE ALREADY EXISTS AT AN UNKNOWN DOORID.
		// §25's first line: **THE LIVE SERVER IS A DIFFERENT DATABASE FROM THIS CONTAINER.** The hub
		// book was authored ON LIVE with the door tool (`#door create` then `#door save`, which really
		// does persist -- against whichever database you ran it on) and never flowed back here, so it
		// is a real row on live and has never existed in dev. That is why this migration exists at all,
		// and it is also the trap: keyed on `doorid = 100` it would find nothing at 100 on live and
		// insert a SECOND book beside the one players already use.
		// So the condition tests for **any** `dest_zone = 'poknowledge'` door in the zone, not for our
		// id, and the INSERT carries a matching `NOT EXISTS` guard so it is safe even if the check is
		// bypassed or the file is run by hand.
		// ⚠️ There is deliberately **no DELETE**. An earlier version deleted doorid 100 first for
		// idempotency; if live's own book happened to sit at 100 that would have removed it and
		// re-inserted ours at OUR coordinates, silently MOVING a book players already know the location
		// of. `NOT EXISTS` provides the idempotency without touching anything that is already there.
		// 📌 Net effect: on live it no-ops and live keeps its book wherever it stands; on dev (and any
		// fresh import) it creates one. The two environments end up with a book each, not necessarily
		// at the same doorid -- which is fine, because nothing keys off the doorid here (`grant_sets`
		// keys off the ZONE, and `book_override` is only needed for zones holding two books).
		.check       = "SELECT doorid FROM doors WHERE zone = 'resplendent' AND dest_zone = 'poknowledge'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 100, 'resplendent', src.version, src.name, -22, 548, 0, 256, src.opentype, src.guild, src.lockpick, src.keyitem,
   src.nokeyring, src.triggerdoor, src.triggertype, src.disable_timer, src.doorisopen, src.door_param, src.dest_zone, src.dest_instance,
   src.dest_x, src.dest_y, src.dest_z, src.dest_heading, src.invert_state, src.incline, src.size, src.buffer, src.client_version_mask,
   src.is_ldon_door, src.close_timer_ms, src.dz_switch_id, -1, -1
FROM doors src
WHERE src.zone = 'butcher' AND src.doorid = 78
  AND NOT EXISTS (SELECT 1 FROM doors d2 WHERE d2.zone = 'resplendent' AND d2.dest_zone = 'poknowledge');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 58,
		.description = "2026_08_13_aotv4_aa_offhand",
		// Two dual-wield AAs replacing two that this server's own rules made worthless. Readable copy:
		// custom/sql/aotv4_aa_offhand.sql.
		//
		//   host  31 Fear Resistance ranks 116,117,118          -> Sinister Strikes  (SPA 249, 1 rank)
		//   host 108 Flurry          ranks 255,256,257,542,543  -> Blindside         (SPA 304, 5 ranks)
		//
		// ⚠️⚠️ WHY THE OLD TWO WENT: both were fine AAs that this server had already legislated out of
		// existence. `Steady Nerve` (31) resisted FEAR, and fear spells are blacklisted from the reward
		// pool entirely (§5's `fear` rule, 40 spells). `Run Them Down` (108) stopped enemies fleeing,
		// and `AoT:NPCsNeverFlee` already stops them fleeing for everyone. Neither was broken; both were
		// buying something the server gives away or forbids.
		// ⚠️ `78 Nothing Left to Fear` (fear IMMUNITY, prereq 31 rank 3) goes from the pool for the same
		// reason -- and it would have become unobtainable anyway once 31 stopped having a rank 3.
		//
		// ⚠️⚠️ SPA 249 `SecondaryDmgInc` IS A BOOLEAN, NOT A MAGNITUDE. `bonuses.cpp:926` is
		// `newbon->SecondaryDmgInc = true;` and `attack.cpp:1855` only tests it -- so rank 2 and above
		// would add NOTHING. That is why this one is deliberately a SINGLE rank, with 116's chain
		// terminated at `next_id = -1`. Giving it the usual 3-5 rung ladder would sell four ranks that
		// do nothing.
		// 📌 What it unlocks is real though: the offhand starts receiving the weapon damage bonus that
		// only the primary gets otherwise (`GetWeaponDamageBonus(..., true)`), which is the live
		// "Sinister Strikes" effect.
		//
		// ⚠️⚠️ SPA 304 `OffhandRiposteFail` MUST BE **NEGATIVE**. Despite the name, `attack.cpp:519` does
		// `chance += chance * slip / 100` where `chance` is the ENEMY'S RIPOSTE CHANCE -- so a positive
		// base makes your offhand swings riposted MORE often, i.e. the exact opposite of the ability.
		// -15/-30/-45/-65/-85 reduces the riposte chance against offhand attacks by that percentage.
		// 📌 Same class of trap as SPA 121's reverse damage shield (§5) and SPA 20's positive base being
		// CURE blindness (§20): the sign carries the meaning, and the name does not tell you which way.
		//
		// ⚠️⚠️ BOTH SPAs WERE CHECKED AGAINST `Mob::ApplyAABonuses` (bonuses.cpp:612-1819) BEFORE BEING
		// CHOSEN, and that check is not optional -- **SPA 176 `DualWieldChance` has NO case there**, so
		// an AA granting it is silently inert while reading perfectly in the window. It was very nearly
		// picked for this. §32 records the same trap in reverse (`MitigateMeleeDamage` missing from
		// ApplyAABonuses while tagged `[AA]` in spdat.h). **Check the function, never the tag.**
		//
		// ⚠️ `aa_rank_prereqs` is a SEPARATE table and a hosted AA inherits its host's -- clear it or the
		// ability shows in the window and refuses to train with no message (§6).
		// ⚠️ AAs load at ZONE BOOT and are not shared memory: a zone restart applies this.
		// ⚠️⚠️ THE NAMES AND DESCRIPTIONS ARE `db_str`, WHICH THE CLIENT READS FROM ITS OWN
		// `dbstr_us.txt` (§6) -- this migration alone renames nothing in game. It needs
		// ./export_client_files and the regenerated file shipped. Both are PASSIVE, so only types 1 and
		// 4 are written; types 2/3 are the hotkey lines and matter only for activated abilities.
		.check       = "SELECT id FROM aa_ability WHERE id = 108 AND name = 'Blindside'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_ability SET name = 'Sinister Strikes', classes = 65535, enabled = 1, type = 4 WHERE id = 31;
UPDATE aa_ability SET name = 'Blindside',        classes = 65535, enabled = 1, type = 4 WHERE id = 108;

-- Sinister Strikes: ONE rank only (SPA 249 is a boolean), so the chain stops at 116.
UPDATE aa_ranks SET cost = 3, level_req = 5, next_id = -1 WHERE id = 116;

-- Blindside keeps the melee tree's five-rung ladder.
UPDATE aa_ranks SET level_req = 5,  cost = 3 WHERE id = 255;
UPDATE aa_ranks SET level_req = 15, cost = 4 WHERE id = 256;
UPDATE aa_ranks SET level_req = 25, cost = 5 WHERE id = 257;
UPDATE aa_ranks SET level_req = 35, cost = 6 WHERE id = 542;
UPDATE aa_ranks SET level_req = 45, cost = 8 WHERE id = 543;
UPDATE aa_ranks SET next_id = -1 WHERE id = 543;

DELETE FROM aa_rank_prereqs  WHERE rank_id IN (116,117,118, 255,256,257,542,543);
DELETE FROM aa_rank_effects  WHERE rank_id IN (116,117,118, 255,256,257,542,543);

INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
    (116, 1, 249,   1, 0),
    (255, 1, 304, -15, 0),
    (256, 1, 304, -30, 0),
    (257, 1, 304, -45, 0),
    (542, 1, 304, -65, 0),
    (543, 1, 304, -85, 0);

UPDATE db_str SET value = 'Sinister Strikes' WHERE id = 116 AND type = 1;
UPDATE db_str SET value = 'Blindside'        WHERE id = 255 AND type = 1;

UPDATE db_str SET value = 'Your weaker hand learns the same trick as your strong one. Your offhand weapon receives the damage bonus that normally only your primary weapon gets. This ability has a single rank -- the offhand either receives the bonus or it does not.' WHERE id = 116 AND type = 4;

UPDATE db_str SET value = 'You strike from where they are not looking. Attacks with your offhand weapon are 15, 30, 45, 65 and 85 percent less likely to be riposted. It does nothing for your primary hand.' WHERE id = 255 AND type = 4;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 59,
		.description = "2026_08_14_aotv4_hate_min_level",
		// The Plane of Hate could not be entered by ANY player. It is in region 2 (Freeport), it
		// carries bypass_expansion_check, and there is a live zone_point leading to it -- but
		// `zone.min_level` is **46** and this server's hard level cap is **30**
		// (era_system.M.HARD_CAP), so `Client::CanEnterZone` refused every single character.
		//
		// ⚠️⚠️ AND IT IS INVISIBLE TO A GM. That check is `if (!GetGM() && GetLevel() < z->min_level)`
		// (zone/zoning.cpp:1466), so anyone testing with the GM flag walks straight in and sees
		// nothing wrong. This is the fourth time that flag has hidden a zone-access bug here --
		// CLAUDE.md §41 lists the other three. **Test zone access as a non-GM.**
		//
		// 📌 `hateplaneb` (186) is the one that matters: it is the revamped Plane of Hate, it is the
		// only one with a zone_point route, and it is the one mapped to a region. The classic
		// `hateplane` is deliberately left alone -- it sits in region 99 "Unused" with no route in,
		// so opening it would be a REGION decision, not a level-gate fix.
		// ⚠️ `max_level` is left at 255. Only the floor was wrong.
		// 📌 Audited the general case rather than just this zone: of every zone mapped to regions 1-6,
		// this is the ONLY one gated out by a level requirement, and none is gated by a numeric
		// `flag_needed`. (A non-numeric flag such as Sleeper's Tomb's "Sleeper's Key" never blocks --
		// the check requires `Strings::IsNumber(flag_needed)` first, so it short-circuits.)
		.check       = "SELECT COUNT(*) FROM zone WHERE short_name = 'hateplaneb' AND min_level > 30",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
-- ⚠️ EVERY version row, not just version 0 -- the same trap recorded for the delve zone_exp_multiplier
-- work (§38): zone config is loaded per (zone, version), so updating one row leaves the others gating.
UPDATE zone SET min_level = 0 WHERE short_name = 'hateplaneb';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 60,
		.description = "2026_08_14_aotv4_zone_xp_realign",
		// The Allaclone "Zone XP" tab was showing PRE-REBALANCE level ranges. `aotv4_zone_xp` was
		// hand-authored (§3 records that deliberately -- a computed percentile disagreed with the
		// owner's list and the authored one shipped), but the npc rebalance then moved every zone's
		// actual creature levels according to `zone_scaling`, so the authored list stopped describing
		// the world. 61 of 62 rows disagreed -- Greater Faydark read 1-6 against a real 1-35, the
		// Dreadlands 5-24 against 3-35.
		//
		// 📌 `zone_scaling` is now the authority BECAUSE IT IS THE INPUT THE REBALANCE USED
		// (aotv4_scaling/npc_level_adjust_main.py remaps each zone's spread onto its min/max), so this
		// is not swapping an authored list for a guess -- it is pointing the display at the same table
		// the world was built from.
		//
		// ⚠️⚠️ RE-RUN THIS AFTER ANY FUTURE REBALANCE, or the tab silently goes stale again. It is the
		// standing cost of caching a derived answer in its own table. The alternative -- deriving in
		// `Client::SearchList` at query time -- would never drift, and is worth doing if this recurs.
		// ⚠️ Zones with NO `zone_scaling` row keep their authored range (Butcherblock Mountains is the
		// only one). The join, not a LEFT JOIN, is what protects them.
		// ⚠️ `label` is untouched. The ten city zones still render as "City" -- §3's point stands that a
		// level range for a zone with five huntable spawns is noise -- and the numbers behind the label
		// are simply not displayed.
		// ⚠️⚠️ DELVE ZONES AND THE RESPLENDENT HUB ARE EXCLUDED EXPLICITLY, NOT INCIDENTALLY. Neither is
		// in `aotv4_zone_xp` today, so the join alone would already spare them -- but `zone_scaling`
		// DOES carry all seven (the six DoN maps at 1-10, Resplendent at 90-95), so the moment anyone
		// adds one to the browse list this migration would start writing a fixed range onto it. §3 is
		// explicit that delve maps must stay out: their creatures are scaled to the PLAYER, so any
		// fixed range is not merely wrong, it is actively misleading.
		// 📌 Keyed on `zone_regions.region_id = 0` ("Always Available") rather than a list of zone ids,
		// so all 39 delve maps are covered -- the 33 LDoN ones as well as the 6 DoN -- and any future
		// delve map is covered the day it is added to that region.
		.check       = R"(SELECT x.zone_id FROM aotv4_zone_xp x
                          JOIN zone_scaling s ON s.zoneid = x.zone_id
                          WHERE (x.lo <> s.min_level OR x.hi <> s.max_level)
                            AND x.zone_id <> 729
                            AND NOT EXISTS (SELECT 1 FROM zone_regions zr
                                            WHERE zr.zone_id = x.zone_id AND zr.region_id = 0)
                          LIMIT 1)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE aotv4_zone_xp x
JOIN zone_scaling s ON s.zoneid = x.zone_id
SET x.lo = s.min_level,
    x.hi = s.max_level
WHERE x.zone_id <> 729
  AND NOT EXISTS (SELECT 1 FROM zone_regions zr
                  WHERE zr.zone_id = x.zone_id AND zr.region_id = 0);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 61,
		.description = "2026_08_15_aotv4_travel_marker_npc",
		// ⚠️⚠️ THE HIDDEN TRAVEL WAYPOINTS DO NOT EXIST ON LIVE WITHOUT THIS, AND THEY FAIL SILENTLY.
		// `aotv4_travel.lua` and `quests/global/2000500.lua` shipped weeks ago, but the npc_types row
		// they spawn was only ever a HAND EDIT in the dev database -- it is in no migration and no
		// custom/sql script. On live `eq.spawn2(2000500, ...)` finds no such npc, returns nothing, and
		// `spawn_markers` quietly does nothing: no markers, no discovery, no error in any log.
		// Reported from play as "the waypoints aren't discoverable on the live server".
		// 📌 The general shape, worth remembering: shipping a FEATURE is code plus data. Everything in
		// `.devcontainer/repo/quests` travels with a git push; a row typed into this database does not.
		//
		// ⚠️ CLONED VIA A TEMP TABLE from `#aemc_trigger` (289045) rather than hand-listing ~100
		// columns -- the rule CLAUDE.md section 5 records for the custom spell lines. That NPC is also
		// the appearance reference the marker was matched to field for field: race 240, gender 2,
		// bodytype 11, size 0, texture 0, both weapon slots 0.
		// ⚠️⚠️ GENDER 2 IS THE ONE THAT MATTERS. A modelless race still has to be drawn as something,
		// and at gender 0 the client falls back to a HUMAN MALE -- which is what four rounds of
		// "the waypoints look like men" screenshots were. Cloning preserves it; do not re-type it.
		//
		// ⚠️ `npc_types` is NOT shared memory -- it loads at zone boot, so this needs a zone restart
		// but no `./shared_memory` rebuild.
		.check       = "SELECT id FROM npc_types WHERE id = 2000500",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_marker_tmp LIKE npc_types;
INSERT INTO aotv4_marker_tmp SELECT * FROM npc_types WHERE id = 289045;

UPDATE aotv4_marker_tmp SET
  id                = 2000500,
  name              = 'a_forgotten_waypoint',
  lastname          = '',
  level             = 1,
  hp                = 11,
  mana              = 0,
  mindmg            = 1,
  maxdmg            = 8,
  -- ⚠️ aggroradius 0: a marker must never pull. #aemc_trigger ships 45.
  aggroradius       = 0,
  assistradius      = 0,
  -- ⚠️ untargetable 1 so nobody can select it; bodytype 11 already suppresses the name.
  untargetable      = 1,
  trackable         = 0,
  runspeed          = 0,
  special_abilities = '',
  npc_faction_id    = 0,
  loottable_id      = 0,
  npc_spells_id     = 0;

INSERT INTO npc_types SELECT * FROM aotv4_marker_tmp;
DROP TEMPORARY TABLE aotv4_marker_tmp;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 62,
		.description = "2026_08_15_aotv4_remove_butcher_docks_book",
		// Butcherblock had TWO Plane of Knowledge books -- doorid 78 by the Kaladim zone line, and
		// doorid 179 at the Docks. The Docks one was removed in dev when the discoverable travel
		// waypoint replaced it (see pok_travel.book_override), but that was a hand edit and never
		// reached live, so live players still have both the book and the waypoint for one destination.
		//
		// ⚠️⚠️ THE DESTINATION IS NOT LOST. `butcherdocks` stays in `pok_portals` and is still granted
		// by the hub book's `grant_sets`, so nobody loses the waypoint -- only the way to FIND it in
		// the field changes, from clicking a book to walking over the hidden spot.
		//
		// ⚠️⚠️ IT MUST NEVER DELETE THE LAST BOOK IN THE ZONE. Doorid 78 is the Kaladim book and is
		// staying; the EXISTS clause makes that structural rather than a matter of getting the doorid
		// right. If live's doors were ever renumbered and 179 is something else, the condition simply
		// does not match and this does nothing -- which is the correct failure direction for a DELETE.
		// 📌 The mirror of migration v57's problem, which had to avoid ADDING a second book on live and
		// keyed on the zone rather than a doorid for the same reason: door ids are not reliable across
		// environments, because some of these were placed by hand.
		//
		// ⚠️ Doors load from the DB at ZONE BOOT, not shared memory -- a zone restart applies this.
		.check       = "SELECT id FROM doors WHERE zone = 'butcher' AND doorid = 179 AND dest_zone = 'poknowledge'",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DELETE FROM doors
WHERE zone = 'butcher'
  AND doorid = 179
  AND dest_zone = 'poknowledge'
  AND EXISTS (
        SELECT 1 FROM (SELECT zone, doorid, dest_zone FROM doors) AS keep
        WHERE keep.zone = 'butcher' AND keep.dest_zone = 'poknowledge' AND keep.doorid <> 179
      );
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 63,
		.description = "2026_08_15_aotv4_clamp_specializations",
		// One-off cleanup for the spell-specialization reset. `global_player.max_skills_for_level` used
		// to raise skills 66-70 to their cap along with everything else, and the caps here run to
		// 300-525 for all sixteen classes -- so every caster ended up with FIVE specializations above
		// 50. `Client::GetMaxSkillAfterSpecializationRules` (zone/client.cpp:3512) permits exactly one,
		// and when it finds more it does not clamp, it RESETS ALL FIVE TO 1 mid-combat with a red
		// message. Reported from play as specializations "resetting randomly" at level 23.
		//
		// The Lua no longer raises them (they rise by casting, as on live), but characters who already
		// hold several above 50 would each take that reset once more on their next cast. This demotes
		// every specialization except the strongest back to 50, which is exactly what the engine would
		// have capped them at, so nobody sees the message and nobody loses their real specialization.
		//
		// ⚠️ TIES ARE BROKEN BY THE LOWEST skill_id. Without that, two specializations sitting at the
		// same top value both stay above 50 and the reset fires anyway -- the cleanup would look like it
		// had run while changing nothing for the very characters it exists to protect.
		// 📌 Specializations are wiped on death by design: `skill_caps` has no row below level 5, so
		// MaxSkill is 0 at level 1 and max_skills_for_level's zeroing branch clears them. This migration
		// only matters for characters mid-run when the fix lands.
		.check       = R"(SELECT id FROM character_skills WHERE skill_id BETWEEN 66 AND 70 AND value > 50
                          GROUP BY id HAVING COUNT(*) > 1 LIMIT 1)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_spec_keep AS
SELECT c.id AS char_id, MIN(c.skill_id) AS keep_skill
FROM character_skills c
JOIN (
    SELECT id, MAX(value) AS top FROM character_skills
    WHERE skill_id BETWEEN 66 AND 70 GROUP BY id
) t ON t.id = c.id AND c.value = t.top
WHERE c.skill_id BETWEEN 66 AND 70
GROUP BY c.id;

UPDATE character_skills c
JOIN aotv4_spec_keep k ON k.char_id = c.id
SET c.value = 50
WHERE c.skill_id BETWEEN 66 AND 70
  AND c.value > 50
  AND c.skill_id <> k.keep_skill;

DROP TEMPORARY TABLE aotv4_spec_keep;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 64,
		.description = "2026_08_16_aotv4_travel_marker_portal",
		// The travel waypoints become VISIBLE PORTALS instead of invisible triggers.
		//
		// Copied from `a_giant_portal` (209117, Bastion of Thunder): race **329** (Race::Portal),
		// gender 2, size 20, texture 0, **bodytype 11**. That NPC is the proof the combination works --
		// it is stock, it uses this exact race WITH bodytype 11, and 11 is NoTarget: no name plate and
		// nothing to target, so the marker renders and still cannot be attacked or killed.
		// ⚠️ bodytype 11 does NOT hide it. Invisibility is "body types above 64" (common/bodytypes.h);
		// 11 only suppresses the name and the target. That is exactly the pair we want here.
		//
		// ⚠️⚠️ THE MODEL DOES NOT EXIST IN MOST ZONES WITHOUT A CLIENT FILE. A race only renders where
		// the zone's `_chr` archive carries it, and race 329 lives in `bothunder_chr` -- Bastion of
		// Thunder is the ONLY zone that spawns it. `aotv4_client_install/Resources/GlobalLoad_chr.txt`
		// gains a `bothunder,bothunder_chr` line (18 entries -> 19) so it loads everywhere.
		// 📌 No .s3d has to be distributed: RoF2 ships every zone, so the archive is already on each
		// player's disk. Only the small text file changes.
		// ⚠️ WITHOUT that client file the marker renders as NOTHING and the waypoints silently stop
		// being visible -- the same failure class as the missing npc row in v61.
		.check       = "SELECT id FROM npc_types WHERE id = 2000500 AND race <> 329",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE npc_types SET
  race         = 329,
  gender       = 2,
  size         = 20,
  texture      = 0,
  helmtexture  = 0,
  bodytype     = 11,
  untargetable = 1,
  trackable    = 0,
  d_melee_texture1 = 0,
  d_melee_texture2 = 0
WHERE id = 2000500;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 65,
		.description = "2026_08_16_aotv4_travel_marker_bonfire",
		// FINAL appearance for the travel waypoints: a campfire, not the portal v64 set.
		// Race **567** (Race::Campfire), gender 2, **size 6** (the stock size), bodytype 11.
		// v64's race 329 portal worked, but the bonfire was preferred once both were seen side by side;
		// v64 is left in history rather than edited so the sequence stays honest.
		//
		// ⚠️ bodytype 11 is what makes it unkillable -- NoTarget, so no name plate and nothing to
		// target. It does NOT hide the model: invisibility is "body types above 64"
		// (common/bodytypes.h). Visible AND untargetable is exactly the pair a marker wants.
		// ⚠️ gender 2. A modelless or mis-gendered race falls back to a HUMAN MALE, which is what four
		// rounds of "the waypoints look like men" screenshots were.
		//
		// ⚠️⚠️ REQUIRES `guildlobby,guildlobby_chr` IN THE CLIENT'S Resources\GlobalLoad_chr.txt.
		// A race only draws where the zone's own `_chr` archive carries it, and of the 37 zones that
		// spawn race 567 only TWO are S3D era -- `guildlobby` (expansion 9) and our own `resplendent`.
		// The other 35 are HoT/VoA/RoF, which ship as EQG and which this loader silently ignores.
		// 📌 No .s3d is distributed: RoF2 ships every zone, so the archive is already on each player's
		// disk. Only the text file changes.
		// ⚠️ Without that file the marker draws as a HUMAN MALE -- not as nothing -- so a player with a
		// stale client sees a man standing in a field and nothing anywhere reports a problem.
		.check       = "SELECT id FROM npc_types WHERE id = 2000500 AND (race <> 567 OR size <> 6)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE npc_types SET
  race         = 567,
  gender       = 2,
  size         = 6,
  texture      = 0,
  helmtexture  = 0,
  bodytype     = 11,
  untargetable = 1,
  trackable    = 0,
  d_melee_texture1 = 0,
  d_melee_texture2 = 0
WHERE id = 2000500;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 66,
		.description = "2026_08_16_aotv4_clear_iceclad_waypoint_camp",
		// The Iceclad travel waypoint lands arrivals inside a camp. Four spawn points sit 34-48 units
		// from it -- `a_shadow_guardian` (level 18) and `#pulsating_icestorm` (level 19) -- and they
		// were killing players the moment they stepped through. Reported from play.
		//
		// ⚠️⚠️ KEYED ON DISTANCE FROM THE WAYPOINT, NOT ON spawn2 IDS. Those ids are not guaranteed to
		// match between this database and live, and deleting a hardcoded id list there could remove
		// four completely unrelated spawn points in another part of the zone. The coordinates are the
		// stable handle -- they come from `aotv4_travel.M.SPOTS.iceclad`.
		//
		// ⚠️ 60 units is deliberately tight. The next spawn point out is at **160**, so the radius sits
		// in a real gap rather than cutting an arbitrary line through a populated area -- it removes
		// the four on top of the arrival and nothing else.
		// 📌 This is STOCK VELIOUS CONTENT being deleted, in a zone banded 9-25 where level 18-19 is
		// perfectly appropriate. The alternative was moving the waypoint instead, which costs no
		// content -- worth remembering if more of these turn up, because deleting a camp per waypoint
		// does not scale and the owner picked these coordinates deliberately.
		//
		// ⚠️ Only `spawn2` rows are removed -- the spawn POINTS. `spawnentry` and `spawngroup` are left
		// alone because a group can be shared by other points elsewhere in the zone, and an orphaned
		// group is inert. The npc_types rows are untouched, so the creatures still exist everywhere
		// else they spawn.
		// ⚠️ Doors and spawns load at ZONE BOOT, not shared memory: a zone restart applies this.
		.check       = R"(SELECT id FROM spawn2 WHERE zone = 'iceclad'
                          AND SQRT(POW(x - 2831.24, 2) + POW(y - 1715.48, 2)) < 60 LIMIT 1)",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spawn2
WHERE zone = 'iceclad'
  AND SQRT(POW(x - 2831.24, 2) + POW(y - 1715.48, 2)) < 60;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 67,
		.description = "2026_08_16_aotv4_heal_lines",
		// Three heal lines the pool did not have below the level cap. Measured before writing:
		// of everything learnable at 1-30, there were **2** fast single-target heals, **1** group
		// heal-over-time, and the lowest group heal was `Word of Health` at level **30** -- the cap --
		// so group healing effectively did not exist on this server at all.
		//
		//   44530-44535  Mending Touch I-VI    fast single heal   (clone of 3684 Light of Life)
		//   44536-44541  Circle of Health I-VI group heal         (clone of 135 Word of Health)
		//   44542-44547  Circle of Renewal I-VI group heal/tick   (clone of 137 Pack Regeneration)
		//
		// ⚠️⚠️ CLONED VIA A TEMP TABLE, never by hand-listing the ~236 columns -- CLAUDE.md §5's rule
		// for every custom spell line here.
		// ⚠️⚠️ **formula1 = 100 AND max1 = 0 ON EVERY TIER.** A clone inherits its template's damage
		// formula, and formula 1-99 means `base + caster_level * formula`. `Word of Health` ships
		// formula 7 / max 485, so an uncorrected clone would heal for hundreds more than its number
		// says and climb with level -- which destroys a hand-tuned ladder. §5 records the Sinew line
		// losing a day to exactly this.
		// 📌 The templates were chosen partly because two of the three are ALREADY static: Light of
		// Life and Pack Regeneration are formula 100. Only Word of Health needed correcting -- but all
		// three are forced, so a future re-clone from a different template cannot reintroduce it.
		//
		// ⚠️ Group only, as asked. `Word of Health` is targettype 3 and `Pack Regeneration` is 41;
		// `SpellFinished` falls both through to the same ST_Group case, so neither reaches non-group.
		// ⚠️ Levels 8/18/28/38/48/58 to match the existing custom lines. Tiers IV-VI are unreachable at
		// today's cap of 30 and are deliberately built anyway, for when the cap rises.
		// ⚠️ Opened to all sixteen classes, like every other pool spell (§14).
		//
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY. World applying this at boot is NOT enough -- stop the
		// stack, run ./shared_memory, restart, or no zone can see these rows.
		// ⚠️⚠️ AND THEY ARE NOT OFFERED UNTIL THE GENERATOR KNOWS THE BAND. gen_stock_pool.pl pulls
		// 43300-43349 wholesale; 44530-44599 is added there in the same commit. Without it these are
		// three perfect spell lines nobody can ever roll.
		.check       = "SELECT id FROM spells_new WHERE id = 44547",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_heal_tmp LIKE spells_new;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44530, `name` = 'Mending Touch I', descnum = 44530,
  effect_base_value1 = 45, formula1 = 100, max1 = 0,
  mana = 25, classes1=classes1*0+8, classes2=8, classes3=8, classes4=8,
  classes5=8, classes6=8, classes7=8, classes8=8, classes9=8, classes10=8,
  classes11=8, classes12=8, classes13=8, classes14=8, classes15=8, classes16=8;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44531, `name` = 'Mending Touch II', descnum = 44531,
  effect_base_value1 = 100, formula1 = 100, max1 = 0,
  mana = 55, classes1=classes1*0+18, classes2=18, classes3=18, classes4=18,
  classes5=18, classes6=18, classes7=18, classes8=18, classes9=18, classes10=18,
  classes11=18, classes12=18, classes13=18, classes14=18, classes15=18, classes16=18;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44532, `name` = 'Mending Touch III', descnum = 44532,
  effect_base_value1 = 150, formula1 = 100, max1 = 0,
  mana = 85, classes1=classes1*0+28, classes2=28, classes3=28, classes4=28,
  classes5=28, classes6=28, classes7=28, classes8=28, classes9=28, classes10=28,
  classes11=28, classes12=28, classes13=28, classes14=28, classes15=28, classes16=28;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44533, `name` = 'Mending Touch IV', descnum = 44533,
  effect_base_value1 = 220, formula1 = 100, max1 = 0,
  mana = 130, classes1=classes1*0+38, classes2=38, classes3=38, classes4=38,
  classes5=38, classes6=38, classes7=38, classes8=38, classes9=38, classes10=38,
  classes11=38, classes12=38, classes13=38, classes14=38, classes15=38, classes16=38;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44534, `name` = 'Mending Touch V', descnum = 44534,
  effect_base_value1 = 300, formula1 = 100, max1 = 0,
  mana = 180, classes1=classes1*0+48, classes2=48, classes3=48, classes4=48,
  classes5=48, classes6=48, classes7=48, classes8=48, classes9=48, classes10=48,
  classes11=48, classes12=48, classes13=48, classes14=48, classes15=48, classes16=48;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 3684;
UPDATE aotv4_heal_tmp SET
  id = 44535, `name` = 'Mending Touch VI', descnum = 44535,
  effect_base_value1 = 450, formula1 = 100, max1 = 0,
  mana = 260, classes1=classes1*0+58, classes2=58, classes3=58, classes4=58,
  classes5=58, classes6=58, classes7=58, classes8=58, classes9=58, classes10=58,
  classes11=58, classes12=58, classes13=58, classes14=58, classes15=58, classes16=58;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44536, `name` = 'Circle of Health I', descnum = 44536,
  effect_base_value1 = 35, formula1 = 100, max1 = 0,
  mana = 40, classes1=classes1*0+8, classes2=8, classes3=8, classes4=8,
  classes5=8, classes6=8, classes7=8, classes8=8, classes9=8, classes10=8,
  classes11=8, classes12=8, classes13=8, classes14=8, classes15=8, classes16=8;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44537, `name` = 'Circle of Health II', descnum = 44537,
  effect_base_value1 = 80, formula1 = 100, max1 = 0,
  mana = 90, classes1=classes1*0+18, classes2=18, classes3=18, classes4=18,
  classes5=18, classes6=18, classes7=18, classes8=18, classes9=18, classes10=18,
  classes11=18, classes12=18, classes13=18, classes14=18, classes15=18, classes16=18;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44538, `name` = 'Circle of Health III', descnum = 44538,
  effect_base_value1 = 120, formula1 = 100, max1 = 0,
  mana = 140, classes1=classes1*0+28, classes2=28, classes3=28, classes4=28,
  classes5=28, classes6=28, classes7=28, classes8=28, classes9=28, classes10=28,
  classes11=28, classes12=28, classes13=28, classes14=28, classes15=28, classes16=28;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44539, `name` = 'Circle of Health IV', descnum = 44539,
  effect_base_value1 = 175, formula1 = 100, max1 = 0,
  mana = 210, classes1=classes1*0+38, classes2=38, classes3=38, classes4=38,
  classes5=38, classes6=38, classes7=38, classes8=38, classes9=38, classes10=38,
  classes11=38, classes12=38, classes13=38, classes14=38, classes15=38, classes16=38;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44540, `name` = 'Circle of Health V', descnum = 44540,
  effect_base_value1 = 240, formula1 = 100, max1 = 0,
  mana = 290, classes1=classes1*0+48, classes2=48, classes3=48, classes4=48,
  classes5=48, classes6=48, classes7=48, classes8=48, classes9=48, classes10=48,
  classes11=48, classes12=48, classes13=48, classes14=48, classes15=48, classes16=48;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 135;
UPDATE aotv4_heal_tmp SET
  id = 44541, `name` = 'Circle of Health VI', descnum = 44541,
  effect_base_value1 = 350, formula1 = 100, max1 = 0,
  mana = 420, classes1=classes1*0+58, classes2=58, classes3=58, classes4=58,
  classes5=58, classes6=58, classes7=58, classes8=58, classes9=58, classes10=58,
  classes11=58, classes12=58, classes13=58, classes14=58, classes15=58, classes16=58;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44542, `name` = 'Circle of Renewal I', descnum = 44542,
  effect_base_value1 = 15, formula1 = 100, max1 = 0,
  mana = 35, classes1=classes1*0+8, classes2=8, classes3=8, classes4=8,
  classes5=8, classes6=8, classes7=8, classes8=8, classes9=8, classes10=8,
  classes11=8, classes12=8, classes13=8, classes14=8, classes15=8, classes16=8, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44543, `name` = 'Circle of Renewal II', descnum = 44543,
  effect_base_value1 = 35, formula1 = 100, max1 = 0,
  mana = 80, classes1=classes1*0+18, classes2=18, classes3=18, classes4=18,
  classes5=18, classes6=18, classes7=18, classes8=18, classes9=18, classes10=18,
  classes11=18, classes12=18, classes13=18, classes14=18, classes15=18, classes16=18, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44544, `name` = 'Circle of Renewal III', descnum = 44544,
  effect_base_value1 = 50, formula1 = 100, max1 = 0,
  mana = 125, classes1=classes1*0+28, classes2=28, classes3=28, classes4=28,
  classes5=28, classes6=28, classes7=28, classes8=28, classes9=28, classes10=28,
  classes11=28, classes12=28, classes13=28, classes14=28, classes15=28, classes16=28, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44545, `name` = 'Circle of Renewal IV', descnum = 44545,
  effect_base_value1 = 75, formula1 = 100, max1 = 0,
  mana = 190, classes1=classes1*0+38, classes2=38, classes3=38, classes4=38,
  classes5=38, classes6=38, classes7=38, classes8=38, classes9=38, classes10=38,
  classes11=38, classes12=38, classes13=38, classes14=38, classes15=38, classes16=38, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44546, `name` = 'Circle of Renewal V', descnum = 44546,
  effect_base_value1 = 100, formula1 = 100, max1 = 0,
  mana = 260, classes1=classes1*0+48, classes2=48, classes3=48, classes4=48,
  classes5=48, classes6=48, classes7=48, classes8=48, classes9=48, classes10=48,
  classes11=48, classes12=48, classes13=48, classes14=48, classes15=48, classes16=48, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
INSERT INTO aotv4_heal_tmp SELECT * FROM spells_new WHERE id = 137;
UPDATE aotv4_heal_tmp SET
  id = 44547, `name` = 'Circle of Renewal VI', descnum = 44547,
  effect_base_value1 = 150, formula1 = 100, max1 = 0,
  mana = 380, classes1=classes1*0+58, classes2=58, classes3=58, classes4=58,
  classes5=58, classes6=58, classes7=58, classes8=58, classes9=58, classes10=58,
  classes11=58, classes12=58, classes13=58, classes14=58, classes15=58, classes16=58, buffduration = 4, buffdurationformula = 100;
INSERT INTO spells_new SELECT * FROM aotv4_heal_tmp; TRUNCATE aotv4_heal_tmp;
DROP TEMPORARY TABLE aotv4_heal_tmp;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 68,
		.description = "2026_08_16_aotv4_clear_freeporttheater",
		// Empties `freeporttheater` so it can be rebuilt as a tradeskill hub. It ships as an undead
		// theatre: 22 npc_types (390000-390021), levels 66-71, across 42 spawn2 rows -- zombies,
		// skeletal musicians, maidens of the night, a shadowy figure and two walking disasters.
		//
		// ⚠️⚠️ THE SPAWN ROWS GO, THE `npc_types` STAY. Deleting the types would be the destructive
		// half of a reversible change: it loses the record of what the zone was, and this migration
		// could never be undone. Nothing else references them -- VERIFIED that all 22 appear in no
		// other zone's spawn table -- so leaving them costs 22 unused rows and buys the revert.
		//
		// ⚠️ Deletes spawn2 FIRST, then the now-orphaned spawnentry/spawngroup rows, keyed off the
		// spawngroupIDs the zone owned. Reversing that order loses the key before it is used.
		// ⚠️ Scoped by `spawn2.zone`, never by the npc_types id band: an id range is a guess about
		// what lives where, the zone column is the fact.
		//
		// ⚠️⚠️ A ZONE READS ITS SPAWNS AT BOOT, so this does nothing to a zone process already
		// running -- `#repop` in the zone, or let it idle out and reboot. That is also why applying
		// this by hand while standing in the theatre appears to do nothing until one of the two.
		//
		// 📌 The zone still needs two GATES before a player can walk in, and neither is done here
		// because both are placement decisions: a `zone_regions` row (it sits in region 99 "Unused")
		// and `bypass_expansion_check = 1` (it is expansion 11 against CurrentExpansion 0). CLAUDE.md
		// section 24 records that exact pair blocking the whole LDoN half of the delve ladder, and
		// section 41 records the GM flag hiding the second one twice -- test as a NON-GM.
		.check       = "SELECT id FROM spawn2 WHERE zone = 'freeporttheater'",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_fpt_sg AS
  SELECT DISTINCT spawngroupID FROM spawn2 WHERE zone = 'freeporttheater';
DELETE FROM spawn2 WHERE zone = 'freeporttheater';
DELETE FROM spawnentry WHERE spawngroupID IN (SELECT spawngroupID FROM aotv4_fpt_sg);
DELETE FROM spawngroup WHERE id IN (SELECT spawngroupID FROM aotv4_fpt_sg);
DROP TEMPORARY TABLE aotv4_fpt_sg;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 69,
		.description = "2026_08_16_aotv4_hub_guards",
		// Twenty placeable guards for the freeporttheater tradeskill hub, ids 2000600-2000619.
		//
		// ⚠️⚠️ TWENTY SEPARATE npc_types, NOT ONE PLACED TWENTY TIMES -- and that is the whole point.
		// `#npcedit` writes the npc_types row, so twenty spawns of one type re-skin as a single unit;
		// twenty rows can each be given their own race, texture and size. The cost is twenty rows to
		// maintain, which is the right trade for guards that are meant to be dressed individually.
		//
		// ⚠️⚠️ CLONED VIA A TEMP TABLE from `Guardian_Vaehan` (202096), never by hand-listing ~100
		// columns -- CLAUDE.md section 5's rule. That NPC is PoK's own guard: level 99, class 1,
		// 10791 hp, runspeed 1.25. Level 99 is deliberate here (a hub guard should not be killable)
		// and is unrelated to the "no level 99 loot hosts" rule, which is about drop sources.
		//
		// ⚠️ `npc_faction_id = 0` -- no faction, so they neither aggro anyone nor are aggroed. A hub
		// guard that picks fights is worse than no guard. `aggroradius`/`assistradius` 0 for the same
		// reason. loottable/merchant/spells cleared so a stray kill drops nothing.
		//
		// ⚠️⚠️ RACE 570 IS A STEAM SUIT, AND ITS ARCHIVE IS NOT STOCK. A model only renders
		// in a zone whose `_chr` archive carries it, and 467 lives in `illsalin_chr` and
		// `devastation_chr`, both already in `Resources/GlobalLoad_chr.txt`. The swinetor race 696
		// asked for first is expansion 16-18, the same era as the five archives that were tried for
		// the delve wardens and FAILED -- so it is very likely to render as an untextured placeholder,
		// silently, with nothing wrong server side. Re-skin freely, but check the archive first.
		// 📌 Other proven-loadable options: 458 Deep Orc (size 7.6), 489 Takish (6.0).
		.check       = "SELECT id FROM npc_types WHERE id = 2000619",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_guard_tmp LIKE npc_types;
INSERT INTO aotv4_guard_tmp SELECT * FROM npc_types WHERE id = 202096;
UPDATE aotv4_guard_tmp SET
  race = 570, gender = 2, texture = 3, helmtexture = 3, size = 10, lastname = 'Guard',
  npc_faction_id = 0, loottable_id = 0, merchant_id = 0, npc_spells_id = 0,
  aggroradius = 0, assistradius = 0;
UPDATE aotv4_guard_tmp SET id = 2000600, `name` = 'Fizzwick_Cogsprocket';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000601, `name` = 'Bimbly_Gearhart';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000602, `name` = 'Nizzle_Boltwrench';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000603, `name` = 'Grimble_Steamcog';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000604, `name` = 'Tannik_Pistonwhistle';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000605, `name` = 'Zeppo_Ironbellows';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000606, `name` = 'Wimbly_Sprocketfuse';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000607, `name` = 'Dandik_Coilspring';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000608, `name` = 'Pemblo_Brasscasing';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000609, `name` = 'Krinkle_Valveturner';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000610, `name` = 'Jonnik_Gearlock';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000611, `name` = 'Vizzik_Steamvent';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000612, `name` = 'Bobbik_Rivetclank';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000613, `name` = 'Snerdle_Copperbolt';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000614, `name` = 'Tinwick_Flywheel';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000615, `name` = 'Podge_Hammerclank';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000616, `name` = 'Wexler_Sootgear';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000617, `name` = 'Mimbik_Torquewell';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000618, `name` = 'Rennik_Axlegrind';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
UPDATE aotv4_guard_tmp SET id = 2000619, `name` = 'Zaffle_Boilerplate';
INSERT INTO npc_types SELECT * FROM aotv4_guard_tmp;
DROP TEMPORARY TABLE aotv4_guard_tmp;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 70,
		.description = "2026_08_17_aotv4_origin",
		// Turns the stock AA `Origin` (331) into the hub-return ability: teleports to the artisan hub
		// in freeporttheater, out of combat only, 18 minute reuse, cost 0, single rank.
		//
		// ⚠️⚠️ IT WAS `enabled = 0`, WHICH MEANS THE AA DOES NOT EXIST AT RUNTIME. `zone/aa.cpp:1823`
		// loads only enabled rows, so before this it could be offered, granted, and silently refused
		// with nothing logged anywhere (CLAUDE.md section 32 records the same trap on the masteries).
		//
		// ⚠️⚠️ SPA 322 CANNOT BE AIMED. `GateToHomeCity` is hardcoded to `GoToBind(4)`
		// (`spell_effects.cpp:2696`) and carries no destination fields at all, so effect 1 becomes
		// SPA 83 `Teleport`, which reads its target off the base values.
		// ⚠️⚠️ AND THE COORDINATE ORDER IS SWAPPED TWICE OVER. `SE_Teleport` reads
		// `x = base_value[1]; y = base_value[0]` (`spell_effects.cpp:563`), so effect_base_value1 is
		// **Y** and 2 is **X**; in-game `/loc` also prints Y,X,Z (section 11). Both swaps cancel here,
		// but writing either in the obvious order lands the player tens of units away. Verified
		// against the native ring spells (530 Ring of Karana, 531 Ring of Commons).
		//
		// ⚠️ OUT OF COMBAT is native, not code: `InCombat = 0, OutofCombat = 1` is enforced by
		// `Mob::CheckCastRestrictions` (`zone/spells.cpp:601`) against `GetAggroCount()`, the same
		// predicate delve entry and the difficulty shift use. ⚠️ That check only fires on a
		// BENEFICIAL spell -- Origin is goodEffect 1 so it bites; the same columns on a detrimental
		// spell do nothing at all.
		//
		// ⚠️ `level_req` 5 -> 1. Every other pool AA is level 1 (aotv4_aa_level1.sql); left at 5 a new
		// character is refused with no message. `grant_only = 1` matches the rest of the pool and
		// keeps it out of the native AA window, which is correct now that the picker owns AA (§45).
		//
		// ⚠️ The description is rewritten because it still said "transported back to your starting
		// city". ⚠️⚠️ The CLIENT resolves that string from its own dbstr_us.txt, so this changes
		// nothing in game until ./export_client_files runs and the file ships to players.
		//
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY -- world down, ./shared_memory, restart. World applying
		// this at boot is NOT enough. `aa_ability`/`aa_ranks`/`db_str` need only a zone restart.
		// 📌 `aa_pool.lua` is regenerated in the same commit: an enabled AA missing from the pool is
		// never offered, which is the silent inverse of the enabled=0 trap above.
		.check       = "SELECT id FROM aa_ability WHERE id = 331 AND enabled = 1",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_ability SET enabled = 1, grant_only = 1 WHERE id = 331;
UPDATE aa_ranks SET level_req = 1 WHERE id = (SELECT first_rank_id FROM (SELECT first_rank_id FROM aa_ability WHERE id = 331) x);
UPDATE spells_new SET effectid1 = 83, effect_base_value1 = -53, effect_base_value2 = -10,
  effect_base_value3 = -27, effect_base_value4 = 0, teleport_zone = 'freeporttheater',
  InCombat = 0, OutofCombat = 1
WHERE id = 5824;
UPDATE db_str SET value = 'Upon using this ability, you are transported to the Theater of the Tranquil, the artisan hub. Cannot be used in combat. This ability is usable every 18 minutes.'
WHERE id = 1000 AND type = 4;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 71,
		.description = "2026_08_17_aotv4_titan_hall_npcs",
		// The ten Titan Hall induction quest givers in `freeporttheater`, one per button on the /aot
		// menu (`lua_modules/aotv4_tutorial.lua`, HUB_TUTORIAL_DESIGN.md). ids 2000620-2000629, sitting
		// directly above the twenty hub guards v69 created at 2000600-2000619.
		//
		// ⚠️⚠️ THESE EXISTED IN DEV ONLY, APPLIED BY HAND, AND WOULD NEVER HAVE REACHED LIVE. The rows
		// were written straight into the dev database while `custom_version` had already advanced past
		// this point, which is the section 2 trap exactly: the payload lands, the version does not, and
		// nothing is visibly wrong until somebody plays on the other server. Section 25 states the rule
		// this migration exists to obey -- prefer a condition-gated migration over a hand-run
		// `custom/sql` script, because a migration applies itself when world boots on live and no-ops
		// where the rows are already present.
		//
		// ⚠️ Cloned from stock 202096 Guardian_Vaehan VIA A TEMPORARY TABLE, the way v69 built the
		// guards, so every npc_types column stays byte-identical to a working NPC; never hand-list the
		// columns (section 5). Only EIGHT differ from the template, established by diffing the live dev
		// rows against it: id, name, lastname, level, race, gender, texture, size, aggroradius.
		//
		// ⚠️ `aggroradius = 0` is what makes them safe to stand next to -- the template is a level 99
		// guardian carrying a 70 unit radius. `npc_faction_id`, `loottable_id`, `merchant_id` and
		// `npc_spells_id` are all already 0 on the template, so an induction NPC cannot end up KOS to
		// anybody or open an empty merchant window -- the two failures section 11 records against the
		// Resplendent hub NPCs.
		//
		// ⚠️⚠️ THIS DOES NOT SPAWN THEM. `npc_types` says only what an NPC IS; a
		// spawngroup/spawnentry/spawn2 chain is what puts one in the world. Placement is being done by
		// hand in game and captured separately, so until that lands these ten exist and are completely
		// unreachable -- which is the state this migration was written in, not an oversight.
		//
		// ⚠️ INSERT IGNORE rather than INSERT, and the check keys on the LAST id created (section 44):
		// a run interrupted part way then re-runs cleanly instead of aborting on the first duplicate.
		.check       = "SELECT id FROM npc_types WHERE id = 2000629",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_hall_tmp LIKE npc_types;
INSERT INTO aotv4_hall_tmp SELECT * FROM npc_types WHERE id = 202096;
UPDATE aotv4_hall_tmp SET
  lastname = 'Titan Hall', level = 70, race = 12, texture = 0, size = 3, aggroradius = 0;
UPDATE aotv4_hall_tmp SET id = 2000620, `name` = 'Loremaster_Ythel', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000621, `name` = 'Drillmaster_Kort', gender = 1;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000622, `name` = 'Quartermaster_Bindle', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000623, `name` = 'Broker_Sarine', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000624, `name` = 'Chronicler_Vess', gender = 1;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000625, `name` = 'Archivist_Talvo', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000626, `name` = 'Keeper_of_Names', gender = 1;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000627, `name` = 'Delvemaster_Rhun', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000628, `name` = 'Warden_Ashka', gender = 1;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
UPDATE aotv4_hall_tmp SET id = 2000629, `name` = 'Pathfinder_Wynn', gender = 0;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_hall_tmp;
DROP TEMPORARY TABLE aotv4_hall_tmp;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 72,
		.description = "2026_08_17_aotv4_titan_hall_tasks",
		// The ten journal tasks behind the Titan Hall induction, ids 2000600-2000609, one activity
		// each. Same story as v71: authored by hand in dev with no migration, so live would never have
		// seen them.
		//
		// ⚠️⚠️ EVERY ACTIVITY IS AN INERT `Touch` (activitytype 11, goalcount 1), AND THAT IS THE WHOLE
		// DESIGN. `TaskActivityType` (common/tasks.h) offers Deliver/Kill/Loot/SpeakWith/Explore/Touch
		// and the rest -- there is no "used a UI window" type and there cannot be one, because our
		// windows are chat-protocol overlays that live outside the task system entirely. Nothing in the
		// game can complete a Touch with no switch behind it by accident, so each is driven to
		// completion explicitly by `client:UpdateTaskActivity(task, 0, 1)` from `aotv4_tutorial.M.mark`
		// when the matching /say arrives.
		//
		// ⚠️⚠️ THREE OF THE TEN ARE TICKED FROM C++, NOT LUA, AND NOTHING REPORTS IT IF THAT BREAKS.
		// `Client::ChannelMessageReceived` intercepts the AdvLoot (`als*`) and Autoskill (`ask*`)
		// commands and RETURNS TRUE, swallowing the line before EVENT_SAY can fire, and `#ach` is a
		// command rather than a say -- so 2000601, 2000602 and 2000604 are marked by
		// `Client::AoTv4TutorialMark` instead. Remove those call sites and those three steps become
		// silently uncompletable.
		//
		// ⚠️ `zones` is left EMPTY on every activity, matching the delve tasks. `TaskActivity::CheckZone`
		// returns true on an empty list, which is required here: half these objectives are completed
		// inside a delve instance, a difficulty shard or another zone entirely, while the closing hail
		// happens back in the hall.
		//
		// ⚠️ The reward columns are deliberately 0 / empty. Every reward is paid in Lua by
		// `aotv4_tutorial.M.give`, because four of the ten -- an alternate currency, an AA grant, a
		// random pick out of an augment id block, and an edit to the reroll counter -- cannot be
		// expressed in this table at all. Wiring the other six here would split one mechanism across
		// two places and guarantee drift.
		//
		// ⚠️ `repeatable = 0`, `min_level = 1`, no cap: the chain has to work for a level 1 character
		// on a first run AND for a level 30 who has never pressed /aot. `AssignTask` is called with
		// `enforce_level_requirement = false` for the same reason.
		// ⚠️ These TASK ids share numbers with the GUARD NPC ids v69 created. Harmless -- different
		// tables -- but it reads alarmingly; the induction NPCs are 2000620-2000629, not 2000600.
		.check       = "SELECT taskid FROM task_activities WHERE taskid = 2000609",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000600,0,0,0,'Rewards of the Titans','Every level you gain offers three rewards, and you choose one. Open the Spells window from the AoT menu and look at what waits for you.','Speak with Drillmaster Kort about your combat abilities.','',0,0,0,0,0,1,0,0,0,0,0,0,'Speak with Drillmaster Kort about your combat abilities.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000601,0,0,0,'Muscle Memory','Your special attacks can swing themselves while you fight. Open the Autoskill window and enable one -- but only four may run at once.','Quartermaster Bindle handles what the dead leave behind.','',0,0,0,0,0,1,0,0,0,0,0,0,'Quartermaster Bindle handles what the dead leave behind.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000602,0,0,0,'Spoils of War','Loot is yours alone here -- everyone rolls their own. Open the Adv Loot window and see how it is claimed.','Broker Sarine will show you how coin is made.','',0,0,0,0,0,1,0,0,0,0,0,0,'Broker Sarine will show you how coin is made.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000603,0,0,0,'The Long Trade','You can sell while you sleep, from any city. Open the Trader window and set a price on something you carry.','Chronicler Vess keeps the record of great deeds.','',0,0,0,0,0,1,0,0,0,0,0,0,'Chronicler Vess keeps the record of great deeds.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000604,0,0,0,'Deeds Remembered','Your deeds are counted, and some pay in ability points and gear. Open the Achievements window and see what is owed you.','Archivist Talvo can find anything, if you ask.','',0,0,0,0,0,1,0,0,0,0,0,0,'Archivist Talvo can find anything, if you ask.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000605,0,0,0,'Ask the Archive','Every item, creature and spell in the world is catalogued. Open Allaclone and search for something.','The Keeper of Names remembers every death.','',0,0,0,0,0,1,0,0,0,0,0,0,'The Keeper of Names remembers every death.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000606,0,0,0,'What Death Leaves','Death takes your spells and your levels, but never your ability points. Open the Death Book, then claim Origin -- it will carry you home to this hall.','Delvemaster Rhun guards the way below.','',0,0,0,0,0,1,0,0,0,0,0,0,'Delvemaster Rhun guards the way below.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000607,0,0,0,'Into the Delve','Beneath us lie dungeons that scale to whoever enters. Open the Delve window and study the ladder.','Warden Ashka walks harder roads than most.','',0,0,0,0,0,1,0,0,0,0,0,0,'Warden Ashka walks harder roads than most.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000608,0,0,0,'Harder Roads','The world can be made crueller, and pays better for it. Open the Difficulty window and read what Nightmare asks of you.','Pathfinder Wynn knows every road worth walking.','',0,0,0,0,0,1,0,0,0,0,0,0,'Pathfinder Wynn knows every road worth walking.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `tasks` (`id`, `type`, `duration`, `duration_code`, `title`, `description`, `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`, `reward_method`, `reward_points`, `reward_point_type`, `min_level`, `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`, `faction_reward`, `completion_emote`, `replay_timer_group`, `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`, `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`) VALUES (2000609,0,0,0,'Roads Remembered','Travel is earned. Find a waypoint and it is yours forever. Open the Travel window and see which roads you have already opened.','Your induction is complete. The hall is yours.','',0,0,0,0,0,1,0,0,0,0,0,0,'Your induction is complete. The hall is yours.',0,0,0,0,0,-1,0,1);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000600,0,-1,1,11,'',0,1,'Open the Spells window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000601,0,-1,1,11,'',0,1,'Enable an ability in the Autoskill window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000602,0,-1,1,11,'',0,1,'Open the Adv Loot window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000603,0,-1,1,11,'',0,1,'Set a price in the Trader window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000604,0,-1,1,11,'',0,1,'Open the Achievements window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000605,0,-1,1,11,'',0,1,'Run a search in the Allaclone window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000606,0,-1,1,11,'',0,1,'Open the Death Book and claim the Origin ability','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000607,0,-1,1,11,'',0,1,'Open the Delve window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000608,0,-1,1,11,'',0,1,'Open the Difficulty window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
INSERT IGNORE INTO `task_activities` (`taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`, `target_name`, `goalmethod`, `goalcount`, `description_override`, `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`, `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`, `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`) VALUES (2000609,0,-1,1,11,'',0,1,'Open the Travel window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 73,
		.description = "2026_08_17_aotv4_titan_hall_spawns",
		// Puts the ten Titan Hall induction NPCs (v71) into `freeporttheater`. Generated by
		// `custom/tools/gen_hub_spawns.pl 2000620 2000629` from the placement done by hand in game --
		// regenerate with that script rather than editing the coordinates here.
		//
		// ⚠️⚠️ NO MIGRATION IN THIS FILE HAD EVER INSERTED A SPAWN ROW BEFORE THIS ONE. v71 creates the
		// npc_types rows and v68 only DELETES the zone's old spawns, so up to now every hub NPC shipped
		// to live as a definition with nothing standing in the world -- the same hand-applied dev-only
		// gap section 2 describes, one table further along. The twenty guards v69 created still have
		// this gap: 15 of them are placed in dev and none of that placement ships.
		//
		// ⚠️⚠️ THE LOCAL spawn2 / spawngroup IDS ARE DELIBERATELY NOT USED. They are AUTO_INCREMENT
		// values that mean nothing on another database -- v66 already records this for the iceclad
		// cleanup ("Those ids are not guaranteed to..."), and hardcoding them would either collide with
		// unrelated live rows or silently overwrite them. The block is allocated at runtime instead.
		//
		// ⚠️⚠️ IT CLEARS THE GREATEST OF *BOTH* SEQUENCES, NOT spawn2's ALONE. `spawn2.id` and
		// `spawngroup.id` are independent counters and drift apart: on the database this was verified
		// against, spawn2 was at 157438 while spawngroup was at 4999. Allocating each from its own
		// MAX would have handed the spawngroups ids in the 5000s that spawn2 rows already own, and the
		// 1:1 id pairing every later lookup assumes would be broken.
		//
		// ⚠️ min_expansion / max_expansion are forced to -1 on BOTH spawnentry and spawn2. This server
		// runs Classic (Expansion:CurrentExpansion = 0), so a real expansion number is content-filtered
		// out and the NPC never appears, with nothing logged -- the reason the PoK books carry -1 too.
		//
		// ⚠️ NOT idempotent by itself, and it cannot be: the ids are computed, so a second run
		// allocates a DIFFERENT block and duplicates all ten rather than colliding. The `.check` is
		// what makes it safe, and it is scoped by ZONE and npc id -- a zone name is a fact about the
		// data, where an id band is a guess about it.
		.check       = "SELECT s2.id FROM spawn2 s2 JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID WHERE s2.zone = 'freeporttheater' AND se.npcID BETWEEN 2000620 AND 2000629",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- AoTv4 hub spawns, generated by custom/tools/gen_hub_spawns.pl -- DO NOT HAND EDIT.
-- npc ids 2000620-2000629, 10 spawn points, zone(s) 'freeporttheater'.
CREATE TEMPORARY TABLE aotv4_hub_spawn (
  n INT, npc INT, gname VARCHAR(50), zone VARCHAR(32), ver INT,
  x FLOAT, y FLOAT, z FLOAT, h FLOAT, respawn INT, variance INT,
  pathgrid INT, pwzi INT, cond_ INT, condval INT, anim INT
);
INSERT INTO aotv4_hub_spawn VALUES
  (0,2000620,'freeporttheater_Loremaster_Ythel000_6754296','freeporttheater',0,-175.444855,-211.522110,-27.000296,509.250000,1200,0,0,0,0,1,0),
  (1,2000621,'freeporttheater_Drillmaster_Kort000_6764929','freeporttheater',0,-155.740479,-210.784683,-27.000298,486.500000,1200,0,0,0,0,1,0),
  (2,2000622,'freeporttheater_Quartermaster_Bindle000_6775260','freeporttheater',0,-137.457809,-204.589996,-27.000298,463.000000,1200,0,0,0,0,1,0),
  (3,2000623,'freeporttheater_Broker_Sarine000_6798084','freeporttheater',0,-102.021210,-176.881180,-27.000298,442.000000,1200,0,0,0,0,1,0),
  (4,2000624,'freeporttheater_Chronicler_Vess000_6808827','freeporttheater',0,-91.830421,-160.807465,-27.000296,417.750000,1200,0,0,0,0,1,0),
  (5,2000625,'freeporttheater_Archivist_Talvo000_6818966','freeporttheater',0,-87.492653,-141.897537,-27.000296,398.250000,1200,0,0,0,0,1,0),
  (6,2000626,'freeporttheater_Keeper_of_Names000_6849897','freeporttheater',0,-174.611404,-219.601685,-27.000296,243.000000,1200,0,0,0,0,1,0),
  (7,2000627,'freeporttheater_Delvemaster_Rhun000_6858743','freeporttheater',0,-154.109665,-218.628616,-27.000298,234.000000,1200,0,0,0,0,1,0),
  (8,2000628,'freeporttheater_Warden_Ashka000_6868223','freeporttheater',0,-133.854919,-210.162994,-27.000298,203.250000,1200,0,0,0,0,1,0),
  (9,2000629,'freeporttheater_Pathfinder_Wynn000_6878767','freeporttheater',0,-96.315277,-181.715775,-27.000298,174.250000,1200,0,0,0,0,1,0);

-- ⚠️⚠️ ONE ALLOCATION FOR ALL THREE TABLES. spawn2.id and spawngroup.id are independent
-- AUTO_INCREMENT sequences, so the block has to clear the HIGHEST of the two or a spawngroup id can
-- land on a spawn2 id that is already taken (and vice versa) on a database whose two sequences have
-- drifted apart. Taking the greatest of both and using one offset keeps the pairing 1:1, which is
-- what every later lookup assumes.
SET @aotv4_base := (SELECT GREATEST(
  (SELECT COALESCE(MAX(id), 0) FROM spawn2),
  (SELECT COALESCE(MAX(id), 0) FROM spawngroup)
) + 1);

INSERT INTO spawngroup
  (id, name, spawn_limit, dist, max_x, min_x, max_y, min_y, delay, mindelay, despawn, despawn_timer, wp_spawns)
SELECT @aotv4_base + n, gname, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 FROM aotv4_hub_spawn;

-- ⚠️ min_expansion / max_expansion -1 on BOTH spawnentry and spawn2. This server runs Classic
-- (Expansion:CurrentExpansion = 0), so anything left at a real expansion number is content-filtered
-- out and the NPC simply never appears -- with nothing logged. Same reason the PoK books carry -1.
INSERT INTO spawnentry
  (spawngroupID, npcID, chance, condition_value_filter, min_time, max_time, min_expansion, max_expansion, content_flags, content_flags_disabled)
SELECT @aotv4_base + n, npc, 100, 1, 0, 0, -1, -1, '', '' FROM aotv4_hub_spawn;

INSERT INTO spawn2
  (id, spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, path_when_zone_idle, _condition, cond_value, animation, min_expansion, max_expansion, content_flags, content_flags_disabled)
SELECT @aotv4_base + n, @aotv4_base + n, zone, ver, x, y, z, h, respawn, variance, pathgrid, pwzi, cond_, condval, anim, -1, -1, '', ''
FROM aotv4_hub_spawn;

DROP TEMPORARY TABLE aotv4_hub_spawn;

-- guess about what a database contains, a zone name is a fact):
--                "WHERE s2.zone = 'freeporttheater' AND se.npcID BETWEEN 2000620 AND 2000629"
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 74,
		.description = "2026_08_17_aotv4_titan_hall_record",
		// Task 2000610 "The Titan Hall Induction" -- ONE journal entry holding ten activities, one per
		// Hall NPC, each naming who to find. Plus a rewrite of the ten individual tasks' pointer text.
		//
		// ⚠️⚠️ THE TEN STOPPED BEING A CHAIN IN THE SAME CHANGE. `aotv4_tutorial.M.hail` no longer
		// checks that the previous step is done, so any teacher can be heard at any time. This task is
		// what replaces the ordering: with the chain gone, nothing else on screen knows the set exists
		// or which of it is outstanding.
		//
		// ⚠️⚠️ EVERY ACTIVITY CARRIES `step = 1`, AND A ZERO THERE WOULD SILENTLY REBUILD THE CHAIN.
		// `tasks.h:398` -- "if all steps are 0 treat each as a separate step (previously called
		// sequence mode)" -- so an all-zero table shows ONE activity at a time in order, which is
		// exactly the behaviour being removed, arrived at by a completely different route and with
		// nothing to indicate it. Non-zero AND all equal is what makes all ten active at once
		// (`!sequence_mode && el.step <= current_step`, :419); `req_activity_id` is -1 for the same
		// reason.
		//
		// ⚠️ Its activities are inert `Touch` rows like the individual ones -- nothing in the game
		// completes them by accident, and `aotv4_tutorial.on_task_complete` mirrors each finished step
		// onto this task with `UpdateTaskActivity`. Lose that call and the record can never complete,
		// with no error anywhere. `M.ensure_umbrella` backfills already-finished steps on assignment,
		// so a character part way through the old chain is not asked to repeat anything -- which it
		// could not do, since each individual task is `repeatable = 0`.
		//
		// ⚠️⚠️ `reward_text` IS varchar(64) -- NOT the roomy column it looks like beside
		// `completion_emote` (512) and `description` (TEXT). The first draft of the pointer text was
		// 106 characters and MariaDB rejected the UPDATE outright ("Data too long"), which would have
		// aborted the migration mid-way on live. The long wording lives in the emote; the short one
		// here. Check the width before writing task prose.
		//
		// ⚠️ The old pointer text named the NEXT NPC ("Speak with Drillmaster Kort about your combat
		// abilities"), which is simply wrong once order stops mattering -- so all ten are repointed at
		// the record instead. Left alone, the game would be telling players to follow an order the
		// code no longer enforces.
		.check       = "SELECT taskid FROM task_activities WHERE taskid = 2000610",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
INSERT IGNORE INTO tasks (id,type,duration,duration_code,title,description,reward_text,reward_id_list,cash_reward,exp_reward,reward_method,reward_points,reward_point_type,min_level,max_level,level_spread,min_players,max_players,repeatable,faction_reward,completion_emote,replay_timer_group,replay_timer_seconds,request_timer_group,request_timer_seconds,dz_template_id,lock_activity_id,faction_amount,enabled) VALUES
(2000610,0,0,0,'The Titan Hall Induction','The Theater of the Tranquil keeps ten teachers, one for each part of the AoT menu. Seek them out in any order you please; this record marks off those you have already heard.','The Hall has taught you all it can. Well met.','',0,0,0,0,0,1,0,0,0,0,0,0,'Every teacher in the Hall has been heard. The Hall thanks you.',0,0,0,0,0,-1,0,1);

INSERT IGNORE INTO task_activities (taskid,activityid,req_activity_id,step,activitytype,target_name,goalmethod,goalcount,description_override,npc_match_list,item_id_list,item_list,dz_switch_id,min_x,min_y,min_z,max_x,max_y,max_z,skill_list,spell_list,zones,zone_version,optional,list_group) VALUES
(2000610,0,-1,1,11,'',0,1,'Speak with Loremaster Ythel about the Spells window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,1,-1,1,11,'',0,1,'Speak with Drillmaster Kort about the Autoskill window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,2,-1,1,11,'',0,1,'Speak with Quartermaster Bindle about the Adv Loot window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,3,-1,1,11,'',0,1,'Speak with Broker Sarine about the Trader window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,4,-1,1,11,'',0,1,'Speak with Chronicler Vess about the Achievements window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,5,-1,1,11,'',0,1,'Speak with Archivist Talvo about the Allaclone window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,6,-1,1,11,'',0,1,'Speak with the Keeper of Names about the Death Book and the Origin ability','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,7,-1,1,11,'',0,1,'Speak with Delvemaster Rhun about the Delve window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,8,-1,1,11,'',0,1,'Speak with Warden Ashka about the Difficulty window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0),
(2000610,9,-1,1,11,'',0,1,'Speak with Pathfinder Wynn about the Travel window','','','',0,0,0,0,0,0,0,'-1','0','',-1,0,0);

UPDATE tasks SET reward_text = 'Your induction record lists who else waits in the Hall.', completion_emote = 'Your Titan Hall induction record lists whoever else still waits in the Hall. They may be met in any order.'
WHERE id BETWEEN 2000600 AND 2000609;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 75,
		.description = "2026_08_17_aotv4_titan_hall_faces",
		// Gives the ten Titan Hall teachers a race each and renames their lastname to the /aot tab they
		// teach. v71 created all ten as the same gnome with lastname 'Titan Hall', which read as
		// placeholder art and told a player nothing about which teacher was which.
		//
		// ⚠️⚠️ THE LASTNAME IS THE LABEL UNDER THE NAME -- the line a player character's GUILD renders
		// on. It is the only per-NPC text visible without hailing, so 'Titan Hall' was spending the one
		// free label on something every NPC in the room already had in common. Now it names the lesson:
		// Spells, Autoskill, Adv Loot, Trader, Achievements, Allaclone, Death Book, Delve, Difficulty,
		// Travel. varchar(32), so the longest ('Achievements') has plenty of room.
		//
		// ⚠️⚠️ EVERY RACE IS A PLAYABLE ONE, AND THAT IS A RENDERING GUARANTEE RATHER THAN A THEME.
		// v69 records the trap in the other direction: a model only renders in a zone whose `_chr`
		// archive carries it, so race 570 needed `GlobalLoad_chr.txt` and a wrong pick draws NOTHING
		// with nothing wrong server side. The 16 playable races are in the base client and load
		// everywhere, so none of them can fail that way. Taken from `char_create_combinations`, which
		// is what this server actually allows, rather than from the usual list of 16.
		//
		// ⚠️ Ten distinct races, no repeats -- the point is telling them apart at a glance. Assigned to
		// suit each name and role rather than by RNG: a random draw happily makes the Delvemaster a
		// halfling and the Pathfinder an ogre, which is varied but reads as arbitrary.
		//
		// ⚠️ `size` is set per race from what stock NPCs of that race actually use (Ogre 9, Troll 8,
		// Dwarf 4, Halfling 3.5, Gnome 3...). They all inherited the gnome's size 3 from v71, and a
		// size 3 barbarian is a child-sized barbarian -- the race change alone would have looked worse
		// than leaving it. `texture` stays 0, which is the default set on every race.
		//
		// ⚠️ npc_types is NOT shared memory -- it is read at zone boot, so this needs a zone restart
		// and no ./shared_memory run. The spawns themselves are untouched (v73 owns those).
		.check       = "SELECT id FROM npc_types WHERE id = 2000629 AND lastname = 'Travel'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE npc_types SET race = 5, gender = 1, size = 6.0, lastname = 'Spells' WHERE id = 2000620;   -- Loremaster_Ythel: High Elf
UPDATE npc_types SET race = 2, gender = 0, size = 7.0, lastname = 'Autoskill' WHERE id = 2000621;   -- Drillmaster_Kort: Barbarian
UPDATE npc_types SET race = 11, gender = 0, size = 3.5, lastname = 'Adv Loot' WHERE id = 2000622;   -- Quartermaster_Bindle: Halfling
UPDATE npc_types SET race = 1, gender = 1, size = 6.0, lastname = 'Trader' WHERE id = 2000623;   -- Broker_Sarine: Human
UPDATE npc_types SET race = 3, gender = 1, size = 6.0, lastname = 'Achievements' WHERE id = 2000624;   -- Chronicler_Vess: Erudite
UPDATE npc_types SET race = 12, gender = 0, size = 3.0, lastname = 'Allaclone' WHERE id = 2000625;   -- Archivist_Talvo: Gnome
UPDATE npc_types SET race = 6, gender = 1, size = 5.0, lastname = 'Death Book' WHERE id = 2000626;   -- Keeper_of_Names: Dark Elf
UPDATE npc_types SET race = 8, gender = 0, size = 4.0, lastname = 'Delve' WHERE id = 2000627;   -- Delvemaster_Rhun: Dwarf
UPDATE npc_types SET race = 128, gender = 1, size = 6.0, lastname = 'Difficulty' WHERE id = 2000628;   -- Warden_Ashka: Iksar
UPDATE npc_types SET race = 4, gender = 0, size = 5.0, lastname = 'Travel' WHERE id = 2000629;   -- Pathfinder_Wynn: Wood Elf
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 76,
		.description = "2026_08_17_aotv4_titan_hall_task_type",
		// Moves the eleven Titan Hall tasks from TaskType::Task (0) to TaskType::Quest (2).
		//
		// ⚠️⚠️ TYPE 0 ALLOWS EXACTLY ONE ACTIVE TASK, SERVER WIDE, PER CHARACTER. `common/tasks.h:193`
		// says it outright -- "Task = 0, // can have at max 1" -- and `task_client_state.cpp:2046`
		// refuses anything further with MAX_ACTIVE_TASKS. All eleven shipped as type 0, so the moment
		// v74's induction record took the single slot, NOT ONE of the ten lessons could ever be
		// accepted again. Reported from play as "it says I have the maximum number of active tasks,
		// but I only have 1" -- which was exactly right, and one is the limit.
		//
		// ⚠️⚠️ THE OLD CHAIN HID THIS. While the ten were strictly sequential a player could only ever
		// hold one anyway, so a one-task cap and a one-task design agreed by accident. Removing the
		// ordering and adding a permanent umbrella broke that agreement in the same change, which is
		// why it surfaced only now.
		// 📌 It was ALREADY half broken before the umbrella: the delve tasks are type 0 too, so holding
		// any Hall lesson blocked entering a delve -- and step 7 is the lesson that asks for one.
		//
		// ⚠️ Quest allows 19 (`MAXACTIVEQUESTS`), so the record plus all ten lessons is 11 and fits
		// with room for the player's own quests.
		//
		// ⚠️ IN-FLIGHT ROWS SURVIVE THIS, which is the only reason it is safe to change a live task's
		// type. `TaskManager::LoadClientState` reads the type from the `tasks` table and NOT from
		// `character_tasks` ("this used to be loaded from character_tasks / this should just load from
		// the tasks table", task_manager.cpp:1345), then resolves the stored `slot` through
		// `GetClientTaskInfo`. A Task always sits at slot 0, and for Quest any `index < 19` is valid --
		// so an already-accepted record at slot 0 simply reloads into quest slot 0.
		// ⚠️ `character_tasks.type` is written by the save path but not read by the load path. It is
		// realigned anyway so the two never disagree for whoever reads that column next.
		// ⚠️ The check asks whether the DESIRED state is already present, and "empty" means apply --
		// the house pattern (v70 tests `enabled = 1`, not `enabled = 0`). Writing it the other way
		// round, as `type <> 2` + "empty", inverts the whole thing: in a database that still needs the
		// fix that query returns eleven rows, the condition reads false, and the migration silently
		// skips on exactly the servers it was written for.
		.check       = "SELECT id FROM tasks WHERE id = 2000610 AND type = 2",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE tasks SET type = 2 WHERE id BETWEEN 2000600 AND 2000610;
UPDATE character_tasks SET type = 2 WHERE taskid BETWEEN 2000600 AND 2000610;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 77,
		.description = "2026_08_17_aotv4_titan_hall_step6_text",
		// Rewrites the Death Book lesson's text so it describes what the step actually is.
		//
		// ⚠️⚠️ IT PROMISED SOMETHING THE WINDOW COULD NOT SHOW. The text read "Open the Death Book,
		// then claim Origin", while Origin had been excluded from `aa_pool.lua` to keep it out of the
		// random rotation -- so the picker could never offer it and the instruction was impossible to
		// follow. Reported from play as "it says to claim origin, but its not there". The pool
		// exclusion is reverted; `aa_choice.gather_affordable` now hides Origin from everyone EXCEPT a
		// player holding this task, so it is absent from ordinary rolls and guaranteed here.
		//
		// ⚠️⚠️ AND THE WINDOW IS EMPTY WITH NOTHING BANKED, WHICH IS THE OTHER HALF OF THE SAME REPORT.
		// `aa_choice.M.offer` returns at `budget < 1` (:273), so a player with no AA point sees no
		// offers at all and the old wording gave them no way to tell that apart from a broken quest.
		// The text now says a point must be banked first.
		// 📌 That makes this the one lesson with a real prerequisite: section 45 puts a fresh
		// character's first point around level 20. Acceptable only because the ten are no longer a
		// chain (v74) -- the other nine can be finished immediately and this one waits.
		//
		// ⚠️ The objective is "spend an ability point", not "spend it ON Origin": the mark comes from
		// the `aapick` say, which carries no indication of WHICH ability was trained. Spending it on
		// anything completes the lesson and the Keeper teaches Origin regardless, which is both
		// truthful and kinder than failing someone for picking the wrong row.
		.check       = "SELECT taskid FROM task_activities WHERE taskid = 2000606 AND activityid = 0 AND description_override = 'Spend an ability point in the Death Book'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE tasks SET description = 'Death takes your spells and your levels, but never your ability points. Those accrue as you play. Once one is banked, open the Death Book, spend it, and I will teach you Origin -- it will carry you home to this hall.' WHERE id = 2000606;
UPDATE task_activities SET description_override = 'Spend an ability point in the Death Book' WHERE taskid = 2000606 AND activityid = 0;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 78,
		.description = "2026_08_17_aotv4_hub_arrival_point",
		// Points the Origin AA (spell 5824) at the hub's authored arrival point, so Origin and the PoK
		// book land in the SAME spot -- the induction doorstep, where the ten teachers stand.
		//
		// ⚠️⚠️ effect_base_value1 IS Y AND value2 IS X. `SE_Teleport` reads `x = base_value[1];
		// y = base_value[0]` (spell_effects.cpp:563), so the pair is stored swapped relative to how it
		// reads. v70 records the same trap; getting it backwards lands the player tens of units away,
		// or outside the zone entirely, with nothing logged.
		// ⚠️ The columns are int(11) -- the authored point is (73.52, -391.28, -14.98) and rounds to
		// (74, -391, -15). Fractions are silently truncated, not rejected.
		//
		// ⚠️ `pok_portals.lua` carries the same coordinates for the book route and is edited in the
		// same commit. That table normally DERIVES a landing 30 units in front of the book; this entry
		// deliberately does not, because the arrival point is authored rather than book-relative -- so
		// rotating the book no longer moves where players arrive.
		//
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY. World applying this at boot is NOT enough: stop the
		// stack, run ./shared_memory, restart. Without it every zone keeps the old destination and the
		// change is invisible.
		.check       = "SELECT id FROM spells_new WHERE id = 5824 AND effect_base_value1 = -391",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new SET effect_base_value1 = -391, effect_base_value2 = 74, effect_base_value3 = -15
WHERE id = 5824;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 79,
		.description = "2026_08_17_aotv4_fellowships",
		// The fellowship system: registrar + component vendor at the Resplendent hub, the two
		// components, the campfire model, and the two buff fires. Logic is
		// `lua_modules/aotv4_fellowship.lua`; spec is FELLOWSHIP_DESIGN.md.
		//
		// ⚠️⚠️ EQEmu IMPLEMENTS NO PART OF THIS. `OP_FellowshipUpdate` is an opcode with no handler,
		// `Class::FellowshipMaster` (69) is defined and unused, and there are NO fellowship tables.
		// Nothing here switches a feature on -- it is buckets, a say protocol and NPCs, like every
		// other AoTv4 system.
		//
		// ⚠️⚠️ THE STOCK FELLOWSHIP NPCs ARE UNREACHABLE AND ARE NOT USED. Randall_of_the_Fellows and
		// the four flaggers (202396-202400) all spawn in `poknowledge` REGION 99 "Unused" -- the set
		// fenced off from the whole server -- so registration goes to the Resplendent hub instead,
		// beside the three NPCs already there.
		//
		// ⚠️⚠️ THE CAMPFIRE MODEL IS RACE 75, THE CLASSIC SUMMONED-FIRE PET, AND THAT IS A RENDERING
		// GUARANTEE. Classic races live in `global_chr` and load in every zone; an expansion race
		// lives in a zone `_chr` archive and draws NOTHING outside it, silently. Race 494 failed
		// exactly that way this session before race 64 fixed it.
		//
		// ⚠️ The campfire is immune to melee/magic/aggro (`19,1^20,1^24,1^25,1`, the stock trigger
		// set) and `show_name = 0` -- it is scenery, and scenery you can kill is broken scenery.
		//
		// ⚠️⚠️ THE BUFF ROWS ARE FORCED TO `formula 100 / max 0`. The clone source (65 Major Shielding)
		// carries `formula1 = 102, max1 = 75`, and section 5 records that inheriting a stock damage
		// formula is what broke the Sinew line: a level-scaled formula would make the campfire buff
		// climb with the CASTER's level, and the caster here is a campfire.
		// 📌 The values (200 HP, 5 regen a tick) are DERIVED for a level 30 cap, not copied: live's
		// health fire is ~1500 HP, which at our ~1,500 HP characters would be a doubling.
		//
		// ⚠️ NOT IDEMPOTENT, and it cannot be -- the spawn ids are computed at runtime (above whatever
		// the target database already uses, clearing BOTH sequences), and `spawngroup.name` is UNIQUE
		// so a second run collides. The `.check` is what makes it safe, exactly as v73.
		//
		// ⚠️⚠️ `items` AND `spells_new` ARE SHARED MEMORY. Applying this at world boot is not enough:
		// stop the stack, run ./shared_memory, restart, or no zone sees the components or the buffs.
		// 📌 The two hub NPCs still need adding to `custom/sql/aotv4_resplendent_npc_lock.sql` -- that
		// file uses DELIMITER, which the migration runner does not honour, and section 11 records the
		// hub NPCs being clobbered TWICE by full imports without those triggers.
		.check       = "SELECT id FROM npc_types WHERE id = 2000412",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ---- NPCs: registrar, vendor, campfire  (clone via temp table so all columns stay valid) ----
CREATE TEMPORARY TABLE aotv4_fs_npc LIKE npc_types;
INSERT INTO aotv4_fs_npc SELECT * FROM npc_types WHERE id = 2000400;
UPDATE aotv4_fs_npc SET lastname='Fellowships', level=70, class=1, bodytype=1,
  npc_faction_id=0, loottable_id=0, merchant_id=0, npc_spells_id=0, aggroradius=0, assistradius=0;
UPDATE aotv4_fs_npc SET id=2000410, `name`='Fellowmaster_Denara', race=1,  gender=1, texture=11, helmtexture=11, size=6;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_fs_npc;
UPDATE aotv4_fs_npc SET id=2000411, `name`='Kindler_Bram', race=12, gender=0, texture=1, helmtexture=1, size=3,
  lastname='Campfire Supplies', class=41, merchant_id=1000030;
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_fs_npc;
-- the campfire model itself: race 75 is the classic summoned-fire pet, so it is in global_chr and
-- renders in every zone -- the same rule that made race 64 safe and race 494 fail.
UPDATE aotv4_fs_npc SET id=2000412, `name`='Fellowship_Campfire', lastname='', race=75, gender=2,
  texture=0, helmtexture=0, size=3, class=1, merchant_id=0, runspeed=0, walkspeed=0,
  show_name=0, special_abilities='19,1^20,1^24,1^25,1';
INSERT IGNORE INTO npc_types SELECT * FROM aotv4_fs_npc;
DROP TEMPORARY TABLE aotv4_fs_npc;

-- ---- components (platinum sink) ----
CREATE TEMPORARY TABLE aotv4_fs_item LIKE items;
INSERT INTO aotv4_fs_item SELECT * FROM items WHERE id = 13008;
UPDATE aotv4_fs_item SET nodrop=1, norent=1, stackable=1, stacksize=20, slots=0,
  classes=65535, races=65535, reqlevel=0, reclevel=0, deity=0, price=0;
UPDATE aotv4_fs_item SET id=147970, Name='Campfire Kit',   icon=1113, price=150000;   -- 150p (price is COPPER)
INSERT IGNORE INTO items SELECT * FROM aotv4_fs_item;
UPDATE aotv4_fs_item SET id=147971, Name='Lumber Bundle',  icon=1114, price=150000;   -- 150p; a buff fire is 300p total
INSERT IGNORE INTO items SELECT * FROM aotv4_fs_item;
DROP TEMPORARY TABLE aotv4_fs_item;

-- ---- campfire buffs ----
CREATE TEMPORARY TABLE aotv4_fs_spell LIKE spells_new;
INSERT INTO aotv4_fs_spell SELECT * FROM spells_new WHERE id = 65;
UPDATE aotv4_fs_spell SET buffduration=30, buffdurationformula=7, targettype=6, goodEffect=1,
  formula1=100, max1=0, formula2=100, max2=0, formula3=100, max3=0,
  effectid2=254, effectid3=254, effect_base_value2=0, effect_base_value3=0;
UPDATE aotv4_fs_spell SET id=44330, `name`='Fellowship of Health', effectid1=69, effect_base_value1=200;
INSERT IGNORE INTO spells_new SELECT * FROM aotv4_fs_spell;
UPDATE aotv4_fs_spell SET id=44331, `name`='Fellowship of Vigor',  effectid1=0,  effect_base_value1=5;
INSERT IGNORE INTO spells_new SELECT * FROM aotv4_fs_spell;
DROP TEMPORARY TABLE aotv4_fs_spell;

-- ---- vendor stock ----
INSERT IGNORE INTO merchantlist (merchantid, slot, item, faction_required, level_required, alt_currency_cost, classes_required, probability)
VALUES (1000030,1,147970,-100,1,0,65535,100), (1000030,2,147971,-100,1,0,65535,100);

-- ---- spawn the two hub NPCs beside Alessa (ids allocated above whatever the target DB uses) ----
SET @fs_b := (SELECT GREATEST((SELECT COALESCE(MAX(id),0) FROM spawn2),(SELECT COALESCE(MAX(id),0) FROM spawngroup)) + 1);
INSERT INTO spawngroup (id,name,spawn_limit,dist,max_x,min_x,max_y,min_y,delay,mindelay,despawn,despawn_timer,wp_spawns)
VALUES (@fs_b,'resplendent_fellowmaster',0,0,0,0,0,0,0,0,0,0,0), (@fs_b+1,'resplendent_kindler',0,0,0,0,0,0,0,0,0,0,0);
INSERT INTO spawnentry (spawngroupID,npcID,chance,condition_value_filter,min_time,max_time,min_expansion,max_expansion,content_flags,content_flags_disabled)
VALUES (@fs_b,2000410,100,1,0,0,-1,-1,'',''), (@fs_b+1,2000411,100,1,0,0,-1,-1,'','');
INSERT INTO spawn2 (id,spawngroupID,zone,version,x,y,z,heading,respawntime,variance,pathgrid,path_when_zone_idle,_condition,cond_value,animation,min_expansion,max_expansion,content_flags,content_flags_disabled)
VALUES (@fs_b,  @fs_b,  'resplendent',0,-38.0,690.0,-26.2,128,1200,0,0,0,0,1,0,-1,-1,'',''),
       (@fs_b+1,@fs_b+1,'resplendent',0,-22.0,690.0,-26.2,128,1200,0,0,0,0,1,0,-1,-1,'','');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 80,
		.description = "2026_08_17_aotv4_campfire_items",
		// Turns lighting a fellowship campfire into COMBINE-then-CLICK instead of a chat command:
		// buy a Kit and a Lumber Bundle, combine them in an Artisan's Universal Kit, click the result
		// and the fire drops at your feet. `fshiplight` survives as a fallback.
		//
		// ⚠️⚠️ THE ITEMS ARE CONSUMABLES, NOT A REUSABLE INSIGNIA, AND THAT IS THE WHOLE REASON LIVE'S
		// DESIGN WAS NOT COPIED. A permanent enabling item in a player's bags is destroyed by the
		// roguelite death wipe unless it is added to `death_loss.M.is_kept` -- exactly what cost the
		// tradeskill tools permanently (section 32), since their achievements are `claim_once`. A
		// consumable you simply buy again has nothing to lose.
		//
		// ⚠️⚠️ `clickeffect` MUST POINT AT A REAL SPELL OR THE CLIENT NEVER SENDS THE CLICK.
		// `Handle_OP_ItemVerifyRequest` reads `item->Click.Effect` and will not raise the packet for
		// an item it believes has no click -- so 44332 is an inert all-254 vehicle that exists purely
		// to make the item clickable. Same trick the Tomes of Insight use.
		// ⚠️ `maxcharges = -1` (unlimited), NOT 0. Section 32 records 0 reading as a SPENT consumable
		// -- "Item is out of charges" -- which leaves the row looking perfectly configured and the
		// item permanently dead. The quest script removes the item on a successful light instead.
		//
		// ⚠️⚠️ THE RECIPE ENTRIES ARE DELETED BEFORE INSERT. `tradeskill_recipe_entries` has an
		// AUTO_INCREMENT primary key and NO unique key on (recipe_id, item_id), so INSERT IGNORE does
		// NOT dedupe: a second run doubles every component count and the recipe quietly starts
		// demanding two kits and four bundles. Caught on a scratch database -- a re-run produced 16
		// entries where 8 were intended.
		//
		// ⚠️ Container is 990061 Artisan's Universal Kit, the container 19,469 stock recipes already
		// use, so the combine works anywhere rather than needing one of the hub's tradeskill stations.
		// 📌 The outputs have no gear-tier rows, so `GetTradeRecipe`'s always-yield-Mythic swap
		// (section 10) is a no-op on them -- worth knowing, because a wearable output would silently
		// come out as a Mythic.
		//
		// ⚠️⚠️ `items` AND `spells_new` ARE SHARED MEMORY -- stop the stack, ./shared_memory, restart.
		.check       = "SELECT id FROM items WHERE id = 147974",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- inert click vehicle: the client will not even SEND the click packet for an item with no effect
CREATE TEMPORARY TABLE aotv4_fc_spell LIKE spells_new;
INSERT INTO aotv4_fc_spell SELECT * FROM spells_new WHERE id = 44330;
UPDATE aotv4_fc_spell SET id=44332, `name`='Light Campfire', descnum=44332,
  effectid1=254, effectid2=254, effectid3=254,
  effect_base_value1=0, effect_base_value2=0, effect_base_value3=0,
  buffduration=0, buffdurationformula=0, targettype=6, goodEffect=1, cast_time=0, recast_time=0;
INSERT IGNORE INTO spells_new SELECT * FROM aotv4_fc_spell;
DROP TEMPORARY TABLE aotv4_fc_spell;

-- the three clickable campfires
CREATE TEMPORARY TABLE aotv4_fc_item LIKE items;
INSERT INTO aotv4_fc_item SELECT * FROM items WHERE id = 147970;
UPDATE aotv4_fc_item SET stackable=0, stacksize=1, nodrop=1, norent=1, slots=0,
  classes=65535, races=65535, reqlevel=0, reclevel=0, deity=0,
  clickeffect=44332, clicktype=1, casttime=0, recastdelay=0, recasttype=-1, maxcharges=-1, icon=1113;
UPDATE aotv4_fc_item SET id=147972, Name='Simple Campfire',   price=0;
INSERT IGNORE INTO items SELECT * FROM aotv4_fc_item;
UPDATE aotv4_fc_item SET id=147973, Name='Warded Campfire',   price=0;
INSERT IGNORE INTO items SELECT * FROM aotv4_fc_item;
UPDATE aotv4_fc_item SET id=147974, Name='Enduring Campfire', price=0;
INSERT IGNORE INTO items SELECT * FROM aotv4_fc_item;
DROP TEMPORARY TABLE aotv4_fc_item;

-- combines, in the Artisan's Universal Kit (990061) -- the container 19,469 stock recipes use
INSERT IGNORE INTO tradeskill_recipe (id,name,tradeskill,skillneeded,trivial,nofail,replace_container,notes,must_learn,learned_by_item_id,quest,enabled,min_expansion,max_expansion)
VALUES (470120,'Warded Campfire',64,0,0,1,0,'AoTv4 fellowship',0,0,0,1,-1,-1),
       (470121,'Enduring Campfire',64,0,0,1,0,'AoTv4 fellowship',0,0,0,1,-1,-1);
-- ⚠️⚠️ DELETE FIRST. `tradeskill_recipe_entries` has an AUTO_INCREMENT primary key and NO unique
-- key on (recipe_id, item_id), so INSERT IGNORE does not dedupe -- a second run doubles every
-- component count and the recipe silently starts demanding two kits and four bundles. Caught on
-- a scratch database, where a re-run produced 16 entries instead of 8.
DELETE FROM tradeskill_recipe_entries WHERE recipe_id IN (470120,470121);
INSERT INTO tradeskill_recipe_entries (recipe_id,item_id,successcount,failcount,componentcount,salvagecount,iscontainer) VALUES
 (470120,990061,0,0,0,0,1), (470120,147970,0,0,1,0,0), (470120,147971,0,0,1,0,0), (470120,147973,1,0,0,0,0),
 (470121,990061,0,0,0,0,1), (470121,147970,0,0,1,0,0), (470121,147971,0,0,2,0,0), (470121,147974,1,0,0,0,0);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 81,
		.description = "2026_08_17_aotv4_campfire_item_types",
		// Fixes the item TYPE and icons on the fellowship components and campfires.
		//
		// ⚠️⚠️ A CLONE INHERITS `itemtype`, AND itemtype 38 IS A DRINK. All five were cloned from
		// 13008 Mead, so clicking a campfire answered "Glug, glug, glug... You take a swig of Enduring
		// Campfire. Ahhh. That was refreshing." and consumed it as a beverage -- the quest script
		// never ran. Reported from play. This is the same class of failure section 5 records for
		// cloned damage formulas and section 44 for an inherited `descnum`: the columns that hurt are
		// the ones with no obvious connection to what you changed.
		// ⚠️ The clickables take itemtype 17, which is what the Tomes of Insight use and is proven to
		// click. The two shop components take 58, which every STOCK Fellowship Kit and Lumber Bundle
		// carries.
		//
		// ⚠️ Icons were kiln (1113) and oven (1114) leftovers from the same clone. 2022 and 2227 are
		// the icons the stock fellowship components use, so a Lumber Bundle now looks like wood.
		//
		// ⚠️⚠️ `items` IS SHARED MEMORY -- stop the stack, ./shared_memory, restart, or every zone
		// keeps serving the drink.
		.check       = "SELECT id FROM items WHERE id = 147974 AND itemtype = 17",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ itemtype 38 IS A DRINK. Everything here was cloned from 13008 Mead and inherited it, so
-- clicking a campfire answered "Glug, glug, glug... You take a swig" and consumed it as a beverage
-- instead of running the script. The clickables take 17 (a plain component, what the Tomes of
-- Insight use); the two shop components take 58, which is what every STOCK Fellowship Kit and
-- Lumber Bundle uses.
-- ⚠️ Icons were kiln/oven leftovers. 2022 and 2227 are the icons stock fellowship components carry.
UPDATE items SET itemtype=58, icon=2022 WHERE id=147970;
UPDATE items SET itemtype=58, icon=2227 WHERE id=147971;
UPDATE items SET itemtype=17, icon=559  WHERE id IN (147972,147973,147974);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 82,
		.description = "2026_08_18_aotv4_campfire_buff_duration",
		// The campfire buffs lasted SIX SECONDS. See the SQL for the whole story: buffdurationformula
		// 7 is "temp = level", so at the level 1 cap of a fresh character the buff expired before the
		// 30 second proximity sweep could renew it.
		// ⚠️ `spells_new` IS SHARED MEMORY -- stop the stack, ./shared_memory, restart.
		.check       = "SELECT id FROM spells_new WHERE id = 44330 AND buffdurationformula = 300",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ buffdurationformula 7 IS "temp = level" (zone/spells.cpp:3261). On a level 1 character that
-- is ONE TICK -- six seconds -- so the campfire buff landed and vanished before the 30 second sweep
-- could re-apply it. Reported from play as "it pops and then its gone before it pops again".
-- The clone source (65 Major Shielding) carried formula 3; 7 was my own wrong pick.
-- ⚠️ 300 is the STATIC form: the default branch is `if (formula < 200) return 0; temp = formula;`
-- so the value IS the tick count. `buffduration` then caps it (`if (duration && duration < temp)`),
-- giving 30 ticks = 3 minutes -- six sweeps of headroom, and short enough that walking away from
-- the fire drops the buff promptly.
-- ⚠️ NOT formula 50 (permanent): section 36 records -1 escaping into the ramping-effect maths and
-- breaking the magnitude while the icon persists.
UPDATE spells_new SET buffdurationformula = 300, buffduration = 30 WHERE id IN (44330,44331);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 83,
		.description = "2026_08_18_aotv4_fellowship_fire_is_an_npc",
		// The campfire stops being a ground object and becomes an NPC, because a ground object can
		// be neither un-clicked nor removed. Full reasoning in the SQL.
		// ⚠️ npc_types is read at ZONE BOOT, not shared memory -- a zone restart is enough.
		.check       = "SELECT id FROM npc_types WHERE id = 2000412 AND race = 567 AND bodytype = 11",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ RACE 567 IS THE CAMPFIRE MODEL. The fire was a GROUND OBJECT built from IT78_ACTORDEF, and
-- that was wrong in three ways at once, all of them structural rather than cosmetic:
--   1. Clicking it OPENED A CONTAINER WINDOW. `Object::HandleClick` (zone/object.cpp:582) branches
--      only on `m_type == Temporary`; EVERY other type -- 0 included -- falls into the else branch,
--      which sends OP_ClickObjectAction and opens the tradeskill/bag panel. There is no object type
--      that is simply not clickable. Reported from play as "I can open a bag it seems?".
--   2. IT COULD NOT BE REMOVED. Lua can CREATE a ground object or a door
--      (`create_ground_object_from_model`, `create_door`) but there is NO binding that removes
--      either -- only decay takes them down. So "destroy camp" said the fire was out and the model
--      stayed lit. Reported as exactly that.
--   3. IT78 is a generic item model and did not read as a fire at all.
-- An NPC fixes all three: `Depop()` IS bound, `bodytype 11` (NoTarget) makes it unclickable, and
-- race 567 is the model the stock campfires already use.
-- 📌 Precedent: the delve chest (section 24) is likewise an object rendered as an NPC.
-- ⚠️ Modelled on stock npc 700114 `Campfire` -- same race, gender, size and bodytype, so it renders
-- exactly as the fires already in the world do.
UPDATE npc_types
SET race              = 567,   -- the campfire model
    gender            = 2,     -- as stock 700114; an object model, not a sexed creature
    texture           = 0,
    size              = 5,     -- stock campfire size; 3 was a guess and read as a candle
    bodytype          = 11,    -- ⚠️⚠️ NoTarget. This is what stops it being clicked at all.
    level             = 1,
    hp                = 13,
    -- ⚠️ 13/14 immune to melee and magic, 21 no-harm-from-client (stock 700114's set), plus the
    -- flee/aggro immunities already carried here. It cannot be attacked, pulled or killed.
    special_abilities = '13,1^14,1^19,1^20,1^21,1^24,1^25,1'
WHERE id = 2000412;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 84,
		.description = "2026_08_18_aotv4_fellowship_insignia_and_lesson",
		// The Fellowship Insignia (147975), its inert click spell (44329), and Fellowmaster Denara's
		// lesson joining the Titan Hall induction as step 10.
		// ⚠️⚠️ `items` AND `spells_new` ARE SHARED MEMORY -- world applying this at boot is NOT
		// enough. Stop the stack, run ./shared_memory, restart, or the rows are invisible to zones.
		// ⚠️ The item is only safe to hand out because `death_loss.M.is_kept` exempts it in the same
		// commit. Removing that exemption silently makes it destructible on the first death.
		.check       = "SELECT id FROM items WHERE id = 147975",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- The Fellowship Insignia, and Fellowmaster Denara's lesson in the Titan Hall induction.
--
-- ⚠️⚠️ THIS ITEM EXISTS ONLY BECAUSE IT IS EXEMPT FROM THE DEATH WIPE. An insignia was deliberately
-- NOT built at first, for the reason section 32 records: `death_loss` destroys every carried item
-- outside `death_loss.M.is_kept`, and the tradeskill tools were permanently lost that way -- their
-- achievements are `claim_once = 1`, so once destroyed there was no path back. A fellowship whose
-- travel item silently vanishes on the first death is worse than no item at all.
-- 📌 So the item is added to `is_kept` IN THE SAME COMMIT. If that entry is ever removed, this row
-- must go with it -- and Denara re-hands it on hail, which is the belt to that braces.

-- ---------------------------------------------------------------- the click vehicle
-- ⚠️ An inert spell is REQUIRED, not decoration: `Handle_OP_ItemVerifyRequest` reads
-- `item->Click.Effect` and the client will not even SEND the click packet for an item it believes
-- has no click. All behaviour is in the item script; this spell does nothing.
-- ⚠️⚠️ CLONED VIA A TEMP TABLE so all ~236 columns stay byte-identical to a known-good custom row.
-- Never hand-list them. ⚠️ And `descnum` MUST be repointed -- a clone inherits it, which is how the
-- Tome of Insight ended up describing Shield Wall (migration v56).
DROP TEMPORARY TABLE IF EXISTS aotv4_tmp_spell;
CREATE TEMPORARY TABLE aotv4_tmp_spell AS SELECT * FROM spells_new WHERE id = 44328;
UPDATE aotv4_tmp_spell SET id = 44329, name = 'Fellowship Insignia', descnum = 44329;
DELETE FROM spells_new WHERE id = 44329;
INSERT INTO spells_new SELECT * FROM aotv4_tmp_spell;
DROP TEMPORARY TABLE aotv4_tmp_spell;

-- ⚠️ The client resolves a spell description from its OWN dbstr_us.txt, not from this table, so this
-- row does nothing in game until ./export_client_files runs and the file ships (section 6).
-- ⚠️ No literal '%' -- the description path is printf-style and eats it as a format token.
DELETE FROM db_str WHERE id = 44329 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
	(44329, 6, 'Returns you to your fellowship''s campfire. You must be out of combat, outside any instance, and have opened the region you are travelling to.');

-- ---------------------------------------------------------------- the item
DROP TEMPORARY TABLE IF EXISTS aotv4_tmp_item;
CREATE TEMPORARY TABLE aotv4_tmp_item AS SELECT * FROM items WHERE id = 147966;
UPDATE aotv4_tmp_item SET
	id          = 147975,
	Name        = 'Fellowship Insignia',
	icon        = 681,
	clickeffect = 44329,
	-- ⚠️⚠️ `nodrop = 0` MEANS NO DROP -- THE FLAG IS INVERTED (client_packet.cpp:10755). Inherited
	-- correct from the tome; restated because getting it backwards makes an earned item tradeable.
	nodrop      = 0,
	norent      = 1,   -- ⚠️ opposite polarity again: 1 = permanent, 0 = deleted on logout
	-- ⚠️⚠️ `maxcharges = -1` IS UNLIMITED. 0 reads as a SPENT consumable ("Item is out of charges")
	-- and is the one value that makes a clicky permanently dead while the row still looks correct --
	-- exactly what happened to the tradeskill masks in v30.
	maxcharges  = -1,
	clicktype   = 1,   -- ⚠️ NOT 4: type 4 is equip-only and this is a bag clicky (slots = 0)
	stackable   = 0,   -- one is all anyone needs, and stacking makes the "already have one" test odd
	loregroup   = -1   -- Lore: nothing to gain from hoarding them
WHERE id = 147966;
DELETE FROM items WHERE id = 147975;
INSERT INTO items SELECT * FROM aotv4_tmp_item;
DROP TEMPORARY TABLE aotv4_tmp_item;

-- ---------------------------------------------------------------- the lesson
-- ⚠️⚠️ TASK ID 2000611, NOT 2000610. The Hall numbers its lessons `FIRST_TASK + step`, and step 10
-- would land on 2000610 -- which is `M.UMBRELLA_TASK`, the induction record itself. `task_of` in
-- aotv4_tutorial.lua skips the umbrella id for exactly this reason; the two must agree.
DROP TEMPORARY TABLE IF EXISTS aotv4_tmp_task;
CREATE TEMPORARY TABLE aotv4_tmp_task AS SELECT * FROM tasks WHERE id = 2000609;
UPDATE aotv4_tmp_task SET
	id          = 2000611,
	title       = 'Bonds of the Road',
	description = 'No one walks Norrath alone for long. A fellowship binds up to twelve of you across every zone: one roster, one chat that reaches wherever your kin are standing, and a campfire any of you may return to. Speak with Fellowmaster Denara, then open the Fellowship window from the AoT menu.',
	-- ⚠️ `reward_text` IS varchar(64). Longer text is REJECTED outright by MariaDB, not truncated --
	-- a 106 character string failed here once already. The long form goes in completion_emote (512).
	reward_text      = 'A Fellowship Insignia.',
	completion_emote = 'Keep the insignia. Death cannot take it from you, and while a campfire burns it will carry you to your kin.'
WHERE id = 2000609;
DELETE FROM tasks WHERE id = 2000611;
INSERT INTO tasks SELECT * FROM aotv4_tmp_task;
DROP TEMPORARY TABLE aotv4_tmp_task;

-- ⚠️⚠️ ACTIVITY TYPE 11 IS `Touch`, AND IT IS INERT ON PURPOSE. There is no "used a UI window"
-- activity type and there cannot be -- our windows are chat overlays outside the task system -- so
-- every Hall objective is a Touch nothing can complete by accident, driven by
-- `client:UpdateTaskActivity` from `M.mark`.
-- ⚠️ Cloned through a temp table like everything else here. `task_activities` has 25 columns and a
-- COMPOUND key, so hand-listing them is both fragile and easy to get wrong -- a first pass named
-- `delivertonpc`, which does not exist on this schema, and would have failed at apply time.
DROP TEMPORARY TABLE IF EXISTS aotv4_tmp_act;
CREATE TEMPORARY TABLE aotv4_tmp_act AS
	SELECT * FROM task_activities WHERE taskid = 2000609 AND activityid = 0;
UPDATE aotv4_tmp_act SET taskid = 2000611, description_override = 'Open the Fellowship window';
DELETE FROM task_activities WHERE taskid = 2000611;
INSERT INTO task_activities SELECT * FROM aotv4_tmp_act;
DROP TEMPORARY TABLE aotv4_tmp_act;

-- ---------------------------------------------------------------- the induction record grows
-- ⚠️⚠️ THE UMBRELLA'S ACTIVITIES MUST ALL CARRY THE SAME NON-ZERO `step`. tasks.h:398 reads "if all
-- steps are 0 treat each as a separate step", so a zero here silently puts the whole record back
-- into SEQUENCE mode and shows one activity at a time -- the chain this design removed, by another
-- route. Cloning activity 9 inherits its step 1 rather than restating it.
DROP TEMPORARY TABLE IF EXISTS aotv4_tmp_umb;
CREATE TEMPORARY TABLE aotv4_tmp_umb AS
	SELECT * FROM task_activities WHERE taskid = 2000610 AND activityid = 9;
UPDATE aotv4_tmp_umb SET activityid = 10,
	description_override = 'Speak with Fellowmaster Denara about fellowships';
DELETE FROM task_activities WHERE taskid = 2000610 AND activityid = 10;
INSERT INTO task_activities SELECT * FROM aotv4_tmp_umb;
DROP TEMPORARY TABLE aotv4_tmp_umb;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 85,
		.description = "2026_08_18_aotv4_clear_inherited_cast_messages",
		// Two inert click vehicles were announcing a weapon buff they do not grant, inherited from
		// their clone source. See the SQL.
		// ⚠️ `spells_new` IS SHARED MEMORY -- stop the stack, ./shared_memory, restart.
		.check       = "SELECT id FROM spells_new WHERE id = 44328 AND cast_on_you = ''",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ A CLONE INHERITS THE CAST MESSAGES, AND NOBODY THINKS TO LOOK AT THEM. 44328 `Insight` (the
-- Tome of Insight's click vehicle) and 44329 `Fellowship Insignia` are both inert -- every effect
-- slot is 254 and neither has a duration -- but both carried "The Call of Fire fills your weapons
-- with power." / "'s weapons gleam." / "The Call of Fire leaves." from whatever row they were
-- originally cloned from. An inert spell still LANDS, so those lines still print: clicking a Tome of
-- Insight has been announcing a weapon buff that does not exist, to the player and to everyone
-- nearby, since the tomes shipped.
-- 📌 Same family as v56 (the tome inheriting `descnum` and describing Shield Wall) and section 5's
-- damage-formula note: the columns that bite on a clone are the ones with no obvious connection to
-- what you changed. Check the message fields on any future clone.
-- ⚠️ Blanked rather than rewritten. Both spells are pure click vehicles whose real work happens in
-- Lua, which prints its own feedback -- a second line from the spell engine would only duplicate it.
UPDATE spells_new
SET you_cast = '', other_casts = '', cast_on_you = '', cast_on_other = '', spell_fades = ''
WHERE id IN (44328, 44329);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 86,
		.description = "2026_08_18_aotv4_hub_door_ids_below_255",
		// The hub's book (300) and campfire (301) sat above the uint8 door-id ceiling, which silently
		// disabled `#door save` for the entire zone while the command still reported success. Full
		// mechanism in the SQL.
		// ⚠️ Doors load from the DB at ZONE BOOT, not shared memory -- a zone restart applies this.
		.check       = "SELECT id FROM doors WHERE zone = 'freeporttheater' AND doorid > 254",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ A DOORID ABOVE 254 SILENTLY BREAKS `#door save` FOR THE WHOLE ZONE. `Doors::CreateDatabaseEntry`
-- (zone/doors.cpp:842) opens with
--     if (content_db.GetDoorsDBCountPlusOne(zone, version) - 1 >= 255) { return; }
-- and despite the name, `GetDoorsDBCountPlusOne` returns **MAX(doorid) + 1**, NOT a count
-- (doors.cpp:753). The hub's book was placed at doorid 300 and the campfire at 301, so that test read
-- 301 >= 255 and EVERY save in freeporttheater returned before writing a row.
--
-- ⚠️⚠️ AND `#door save` STILL PRINTS "Door saved". The command calls a void function and messages
-- unconditionally (gm_commands/door_manipulation.cpp:432), so the tool reports success on every
-- attempt while nothing reaches the database. Reported from play as a placed and saved portal simply
-- being gone; there is no error, no log line, and the AUTO_INCREMENT on `doors` never moved, which is
-- what proved the row had never existed rather than having been deleted.
--
-- ⚠️⚠️ 254 IS A HARD CEILING, NOT A STYLE PREFERENCE: `Door_Struct.doorId` is **uint8**
-- (common/eq_packet_structs.h:2909) and `Doors::m_door_id` is uint8 too (doors.h:105). 300 truncates
-- to 44 on load AND on the wire, so the book kept working purely because both sides truncate
-- identically -- and it would have collided invisibly with any real door at 44.
-- 📌 So never allocate a "custom" door id in a high band the way NPC and item ids are allocated here.
-- Doors have no room for one. Use the next free low id.
--
-- 12 and 13 are the next free ids (1-11 are the zone's own doors plus the eight sakura trees), and
-- nothing keys off a doorid: `pok_travel` matches on `dest_zone = 'poknowledge'` and `grant_sets` on
-- the ZONE, exactly as migration v57 records.
-- ⚠️ skyshrine version 1 also carries 255-342, but that is STOCK data in a zone we do not author.
-- Left alone: renumbering it would rewrite 88 rows of content to fix a tool nobody uses there.
UPDATE doors SET doorid = 12 WHERE zone = 'freeporttheater' AND doorid = 300
  AND NOT EXISTS (SELECT 1 FROM (SELECT doorid, zone FROM doors) d
                  WHERE d.zone = 'freeporttheater' AND d.doorid = 12);
UPDATE doors SET doorid = 13 WHERE zone = 'freeporttheater' AND doorid = 301
  AND NOT EXISTS (SELECT 1 FROM (SELECT doorid, zone FROM doors) d
                  WHERE d.zone = 'freeporttheater' AND d.doorid = 13);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 87,
		.description = "2026_08_18_aotv4_hub_book_model",
		// The hub's PoK book stops being a Freeport bulletin board. Full reasoning in the SQL.
		// ⚠️⚠️ PAIRED WITH A CLIENT FILE. It is only correct once the repacked
		// `aotv4_client_install/eqg/freeporttheater.eqg` (152 files) is installed -- the model does
		// not exist in the stock zone archive, so applying this alone makes the book INVISIBLE.
		// ⚠️ Doors load from the DB at ZONE BOOT, not shared memory -- a zone restart applies this.
		.check       = "SELECT id FROM doors WHERE zone = 'freeporttheater' AND dest_zone = 'poknowledge' AND name = 'OBJ_POK_BOOK_'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- The hub's Plane of Knowledge book becomes an actual book.
--
-- ⚠️⚠️ IT WAS `OBJ_FP_BBOARD` -- A FREEPORT BULLETIN BOARD -- AND NO OTHER NAME WOULD HAVE WORKED.
-- A door's model is resolved against the ZONE'S OWN .eqg, and `freeporttheater.eqg` shipped with
-- `obj_fp_bboard` and nothing book-like in it. Setting this column to the real book model resolved to
-- NOTHING: the door is created, found and clickable, and draws an empty hole -- no error, no
-- UIErrorLog entry, exactly the silent failure section 3 records for a mis-cased texture name. So the
-- board was not a placeholder anyone forgot to change; it was the only model available.
-- ⚠️⚠️ THIS ROW IS ONLY CORRECT ONCE THE REPACKED ARCHIVE IS INSTALLED. `obj_pok_book_.mod` and its
-- nine textures were merged into freeporttheater.eqg from the client's standalone `pok_book.eqg`
-- (aotv4_client_install/eqg/freeporttheater.eqg, 152 files). Apply this without shipping that file
-- and the hub's book turns invisible for everyone.
-- 📌 The model name is stored UPPERCASE like every other row in this table; the archive member is
-- lowercase `obj_pok_book_.mod`. That asymmetry is normal here -- see the eight OBP_TREE_SAKURA rows
-- against `obp_tree_sakura.mod`.
--
-- ⚠️ Keyed on `dest_zone`, not on the doorid: v86 renumbered this door from 300 to 12 and section 11
-- records that nothing keys off a doorid precisely because they are not stable across environments.
UPDATE doors
SET name = 'OBJ_POK_BOOK_'
WHERE zone = 'freeporttheater' AND dest_zone = 'poknowledge' AND name = 'OBJ_FP_BBOARD';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 88,
		.description = "2026_08_18_aotv4_hub_arrival_point_moved",
		// Re-points Origin at the hub's new authored arrival point. Supersedes v78, which put it at
		// (73.52, -391.28, -14.98). See the SQL for the two coordinate swaps involved.
		// ⚠️ `pok_portals.lua` and `aotv4_tutorial.M.DROP` carry the same point and are edited in the
		// same commit -- all three or none.
		// ⚠️⚠️ `spells_new` IS SHARED MEMORY -- ./shared_memory with the stack down, or invisible.
		.check       = "SELECT id FROM spells_new WHERE id = 5824 AND effect_base_value1 = -247",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- Move the hub's arrival point to the doorstep the owner chose: /loc (-246.67, -71.56, -27.10).
--
-- ⚠️⚠️ `effect_base_value1` IS **Y** AND `effect_base_value2` IS **X**. `SE_Teleport` reads
-- `x = base_value[1]; y = base_value[0]` (zone/spell_effects.cpp:563), so the pair is stored SWAPPED
-- relative to how it reads. Migrations v70 and v78 both record this trap; getting it backwards lands
-- the player tens of units away, or outside the zone entirely, with nothing logged.
-- ⚠️ In-game `/loc` prints **Y, X, Z**, which swaps them a SECOND time. The owner's
-- (-246.67, -71.56, -27.10) is x = -71.56, y = -246.67, z = -27.10 -- so base1 = -247, base2 = -72.
-- Two independent swaps that cancel out is exactly the shape of thing that gets "fixed" into a bug.
-- ⚠️ The columns are int(11); fractions are silently TRUNCATED, not rejected. -246.67 -> -247.
--
-- ⚠️⚠️ THREE PLACES DEFINE THIS ONE POINT AND ALL THREE MOVE TOGETHER:
--   * this spell        -- where Origin drops you
--   * pok_portals.lua   -- where a PoK book port drops you  (edited in the same commit)
--   * aotv4_tutorial.M.DROP -- where a newcomer is placed   (edited in the same commit)
-- Leave one behind and players arrive by one route in a different place from another, which reads as
-- the hub having two front doors.
-- 📌 It is AUTHORED, not derived from the book's position -- so moving or rotating the book no longer
-- moves where anyone arrives. That decoupling is deliberate and predates this change.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY. World applying this at boot is NOT enough: stop the stack, run
-- ./shared_memory, restart. Without it every zone keeps the old destination and this is invisible.
UPDATE spells_new
SET effect_base_value1 = -247,   -- Y
    effect_base_value2 = -72,    -- X
    effect_base_value3 = -27     -- Z
WHERE id = 5824;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 89,
		.description = "2026_08_18_aotv4_open_the_artisan_hub",
		// freeporttheater becomes reachable by everyone: min_status, the stock expansion check, and
		// the region lock were all shut, and two of the three are GM-exempt. See the SQL.
		// ⚠️ `zone` is read at boot by BOTH world and zone; `zone_regions` at zone boot. Restart both,
		// no ./shared_memory.
		// ⚠️ Paired with Lua: `aotv4_travel.M.BOOK_REGION` gains the hub at region 0 and every
		// HasRegion gate moves to `M.is_open`, because HasRegion(0) is false by construction.
		.check       = "SELECT short_name FROM zone WHERE short_name = 'freeporttheater' AND min_status = 0 AND bypass_expansion_check = 1",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- Open the artisan hub to everyone. THREE independent gates were shut, and two of them are exempt
-- for a GM -- so it worked for every test character and refused every player.
--
-- ⚠️⚠️ GATE 1: `min_status = 80` -- GM only. Nobody else could enter at all.
--
-- ⚠️⚠️ GATE 2: the STOCK expansion check in `Client::ZonePC` (zone/zoning.cpp:367):
--     if (CurrentExpansion >= Classic && !GetGM()) {
--         if (z->expansion <= CurrentExpansion || z->bypass_expansion_check) { ...ok... }
--     }
-- freeporttheater is `zone.expansion = 11` and this server runs Classic
-- (`Expansion:CurrentExpansion = 0`), so entry was refused with "part of an expansion that you do not
-- yet own". Note the `!GetGM()`: section 41 records this exact flag hiding this exact bug TWICE
-- before, on the DoN six and then the LDoN thirty-three.
-- ⚠️ Fixed with `bypass_expansion_check`, NOT by rewriting `expansion`. That column is real metadata
-- -- content filtering, zone listings and the era system all read it -- so setting it to 0 would make
-- the hub claim to be Classic content everywhere else in the server. The bypass exempts ONLY the
-- entry gate and leaves the zone honest about what it is.
--
-- ⚠️⚠️ GATE 3: region 99 "Unused" -- `RegionManager::CanEnterZone` would answer "You have not yet
-- unlocked Unused." Moved to region 0 "Always Available", the same fix v27 applied to the LDoN delve
-- maps.
--
-- ⚠️⚠️ UNLOCKING THIS GRANTS NO WORLD ACCESS, WHICH IS THE WHOLE SAFETY ARGUMENT. `zone_points` has
-- ZERO rows targeting freeporttheater and ZERO leading out of it -- there is no way to WALK in or
-- out. It is reachable only by the PoK book network and by Origin, both of which are ours. That is
-- the same test v27 used, and the same reason `veksar` was excluded there: a zone with zone_points is
-- world content, and unlocking it is a TRAVEL change rather than a hub change.
UPDATE zone SET min_status = 0, bypass_expansion_check = 1 WHERE short_name = 'freeporttheater';

-- ⚠️⚠️ DELETE THEN INSERT -- `REPLACE` DOES NOT MOVE A ZONE BETWEEN REGIONS HERE. The primary key is
-- COMPOSITE, `(zone_id, region_id)`, so a REPLACE only replaces that exact PAIR: it left the region
-- 99 row untouched and ADDED a second row, putting the hub in two regions at once. Caught on a
-- scratch database before it reached anything.
-- 📌 The composite key is deliberate -- a zone genuinely CAN belong to two regions (The Swamp of No
-- Hope is reachable from both Firiona Vie and Cabilis, and is the one zone in the table that does).
-- So this is not a schema oversight to route around; it is a table that models something this
-- particular change does not want.
-- ⚠️ Scoped to zone 390 only, so the legitimate multi-region zone is untouched.
DELETE FROM zone_regions WHERE zone_id = 390;
INSERT INTO zone_regions (zone_id, region_id) VALUES (390, 0);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 90,
		.description = "2026_08_18_aotv4_hub_guards_loadable_race",
		// The 20 Titan Hall guards were race 570 (Exoskeleton), whose model archive no RoF2 client
		// has -- so they rendered nothing and took the client down when enough of them came into
		// range at once. Full diagnosis in the SQL.
		// ⚠️ Paired with a CLIENT change: the eight archives added to Resources/GlobalLoad_chr.txt
		// (mechanotus_chr and friends) all fail to open and are reverted in the same commit. Naming
		// an archive there does not create it.
		// ⚠️ npc_types is read at ZONE BOOT, not shared memory -- a zone restart applies this.
		.check       = "SELECT id FROM npc_types WHERE id BETWEEN 2000600 AND 2000619 AND race = 570",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ THE HUB GUARDS HAD NO MODEL, AND THAT IS WHAT CRASHED THE CLIENT.
-- They shipped as race 570 (Exoskeleton), whose models live in `mechanotus_chr`. That archive is NOT
-- present in a RoF2 client -- `GlobalLoad_chr.txt` was extended to name it and the client log answers
-- plainly:  "Loading mechanotus_chr" -> "Failed to open ...\mechanotus_chr.s3d."  (Every one of the
-- eight archives added to that file fails the same way; they were never there to load.)
--
-- ⚠️⚠️ WHY IT LOOKED INTERMITTENT: zoning in from character select drops you at your SAVED location,
-- which is usually out of render range of the guards. Travelling in by PoK book drops you on the
-- induction doorstep, and SEVEN of them stand within 200 units of it -- the nearest at 86. So the
-- same zone crashed by one route and not the other, which is what made this look like a zone file or
-- a patcher problem rather than an NPC one.
--
-- Race 12 (Gnome) is classic: `globalGNM_chr` / `globalGNF_chr` are loaded by every client
-- unconditionally, so it cannot fail this way. It also fits the names -- Fizzwick Cogsprocket,
-- Bimbly Gearhart, Nizzle Boltwrench are gnome tinkerers, not exoskeletons.
-- ⚠️ Size drops 10 -> 3. Ten was sized for a mechanical construct; on a gnome it is a giant.
-- ⚠️ Gender 2 is "neuter", which is correct for an object model and wrong for a humanoid -- a
-- humanoid race with gender 2 has no model to pick either. Set to 0 (male).
-- 📌 If the construct look is wanted back, it needs a race whose archive the client actually has --
-- check the client log for "Failed to open" before choosing one.
UPDATE npc_types
SET race = 12, gender = 0, size = 3
WHERE id BETWEEN 2000600 AND 2000619 AND race = 570;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 91,
		.description = "2026_08_18_aotv4_hub_guards_restore_steamsuit",
		// Reverts v90 on request: the guards go back to the Steam Suit look they shipped with.
		//
		// ⚠️⚠️ v90 IS NOT EDITED IN PLACE, AND MUST NOT BE. It has already applied on the test
		// database and may have applied on live, and a migration only ever runs once -- rewriting its
		// body would leave every database that already took it sitting on gnomes with nothing left to
		// correct them. A revert is a NEW version or it is nothing.
		// 📌 On a fresh database the chain still lands correctly: v69 creates them at 570, v90 sets 12,
		// v91 sets 570. Ordered and idempotent either way.
		//
		// ⚠️⚠️ THE RENDER QUESTION IS UNRESOLVED, AND THIS MIGRATION DOES NOT SETTLE IT. v90 was
		// written on a client-log line reading "Failed to open ...\mechanotus_chr.s3d", i.e. the model
		// archive was reported missing. Race 570 is the stock Steam Suit -- 15 npc_types use it in
		// `mechanotus`, `steamfactory`, `guardian` and `mansion`, all Depths of Darkhollow zones, and
		// DoD-era zones ship `.eqg` archives while `Resources/GlobalLoad_chr.txt` loads `_chr.s3d`.
		// So "the archive is absent" and "we asked for it in the wrong container" look identical from
		// the server, and neither can be told apart from inside the dev container.
		// 📌 If they render as an untextured placeholder or not at all, that is this, not a bad revert
		// -- and CLAUDE.md section 48 records 458 Deep Orc / 489 Takish as proven-loadable fallbacks.
		//
		// ⚠️ Gender 2 is "neuter", which is what an object model wants and what a humanoid race has no
		// model for; size 10 suits a construct and is giant on a humanoid. Both go back WITH the race,
		// because they were always one setting and not three.
		// ⚠️ npc_types is read at ZONE BOOT, not shared memory -- a zone restart applies this.
		.check       = "SELECT id FROM npc_types WHERE id BETWEEN 2000600 AND 2000619 AND race = 12",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
UPDATE npc_types
SET race = 570, gender = 2, size = 10
WHERE id BETWEEN 2000600 AND 2000619 AND race = 12;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 92,
		.description = "2026_08_19_aotv4_new_heal_descriptions",
		// The eighteen heal spells added on 2026-08-16 (Mending Touch, Circle of Health, Circle of
		// Renewal -- 44530-44547) shipped with rows in `spells_new` and NO `db_str` description at all,
		// so every window that describes a spell rendered them BLANK. Reported from play.
		//
		// ⚠️⚠️ A SPELL WITH NO db_str ROW FAILS SILENTLY AND LOOKS LIKE A BROKEN WINDOW. `AoTv4SpellDesc`
		// returns an empty string, the panel draws the name and then nothing, and there is no error
		// anywhere -- the spell itself works perfectly when cast. Nothing in the pool generator or the
		// SQL that creates a spell checks that a description exists, so this is easy to repeat.
		// 📌 When adding a spell line, write its db_str type 6 rows IN THE SAME SCRIPT as the
		// `spells_new` rows. The eight lines in 43300-43349 all did; this one did not.
		//
		// ⚠️ Type 6 is the SPELL description. Type 4 is the AA description -- a different space entirely
		// (section 6), and writing the wrong one leaves the panel just as empty.
		// ⚠️ No literal '%' -- the description path is printf-style and eats it as a format token
		// (section 14). Written as "half a minute" rather than a percent or a placeholder for that reason.
		// ⚠️⚠️ NO APOSTROPHES. "your target's wounds" is a syntax error inside a single-quoted SQL
		// string, and the migration aborts mid-run -- caught only by dry-running the extracted SQL in a
		// rolled-back transaction before committing it. Escaping works but then reads back as \' in
		// every dump, which section 25 records as defeating a plain grep for the text.
		// ⚠️ Numbers are LITERAL, matching the eight sibling lines. The #N/@N placeholders that
		// `AoTv4SpellDesc` substitutes are a STOCK convention; every AoTv4 custom line spells the value
		// out, and mixing the two makes the set inconsistent to edit.
		// 📌 The client resolves descriptions from its OWN `dbstr_us.txt`, not from this table
		// (section 6), so this migration alone changes nothing in game -- it needs
		// `./export_client_files` and the regenerated file shipped to players.
		.check       = "SELECT id FROM db_str WHERE id = 44547 AND type = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM db_str WHERE id BETWEEN 44530 AND 44547 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
  (44530, 6, 'Closes the wounds of your target, healing 45 points at once.'),
  (44531, 6, 'Closes the wounds of your target, healing 100 points at once.'),
  (44532, 6, 'Closes the wounds of your target, healing 150 points at once.'),
  (44533, 6, 'Closes the wounds of your target, healing 220 points at once.'),
  (44534, 6, 'Closes the wounds of your target, healing 300 points at once.'),
  (44535, 6, 'Closes the wounds of your target, healing 450 points at once.'),
  (44536, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 35 points at once.'),
  (44537, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 80 points at once.'),
  (44538, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 120 points at once.'),
  (44539, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 175 points at once.'),
  (44540, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 240 points at once.'),
  (44541, 6, 'A circle of restoring light closes the wounds of everyone in your group, healing each of them 350 points at once.'),
  (44542, 6, 'Renewing energy settles over your group, healing every member 15 points every six seconds for twenty-four seconds.'),
  (44543, 6, 'Renewing energy settles over your group, healing every member 35 points every six seconds for twenty-four seconds.'),
  (44544, 6, 'Renewing energy settles over your group, healing every member 50 points every six seconds for twenty-four seconds.'),
  (44545, 6, 'Renewing energy settles over your group, healing every member 75 points every six seconds for twenty-four seconds.'),
  (44546, 6, 'Renewing energy settles over your group, healing every member 100 points every six seconds for twenty-four seconds.'),
  (44547, 6, 'Renewing energy settles over your group, healing every member 150 points every six seconds for twenty-four seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 93,
		.description = "2026_08_19_all_classes_all_weapon_skills",
		// Submitted by: maintainer
		// Weapon skills were trainable only by the classes stock EQ gave them to, so a reforge
		.check       = "SELECT class_id FROM skill_caps WHERE class_id = 7 AND skill_id = 1 LIMIT 1",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
CREATE TEMPORARY TABLE aotv4_ref_skill AS
SELECT class_id,
       CAST(SUBSTRING_INDEX(GROUP_CONCAT(skill_id ORDER BY cap DESC, skill_id ASC), ',', 1) AS UNSIGNED) AS ref_skill
FROM skill_caps
WHERE level = 35 AND skill_id IN (0,1,2,3,36,28) AND cap > 0
GROUP BY class_id;

CREATE TEMPORARY TABLE aotv4_ref_curve AS
SELECT r.class_id, sc.level, sc.cap
FROM aotv4_ref_skill r
JOIN skill_caps sc ON sc.class_id = r.class_id AND sc.skill_id = r.ref_skill;

CREATE TEMPORARY TABLE aotv4_weap (skill_id TINYINT UNSIGNED PRIMARY KEY);
INSERT INTO aotv4_weap VALUES (0),(1),(2),(3),(7),(28),(36),(51);

INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT w.skill_id, c.class_id, c.level, c.cap, 0
FROM aotv4_weap w
CROSS JOIN aotv4_ref_curve c
WHERE NOT EXISTS (
  SELECT 1 FROM skill_caps x
  WHERE x.class_id = c.class_id AND x.skill_id = w.skill_id AND x.level = c.level
);

DROP TEMPORARY TABLE aotv4_weap; DROP TEMPORARY TABLE aotv4_ref_curve; DROP TEMPORARY TABLE aotv4_ref_skill;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 94,
		.description = "2026_08_19_paladin_class_aas",
		// Submitted by: maintainer
		// Three Paladin-only activated AAs. Hosted on disabled native rows 45/55/79 because a NEW
		.check       = "SELECT id FROM aa_ability WHERE id = 79 AND classes = 4 AND enabled = 1",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id BETWEEN 44600 AND 44602;

-- 44600 Ardent Strike -- an INERT MARKER. The damage is STR-scaled with a level floor, which no SPA
-- can express, so quests/global/spells/44600.lua pays it. Same pattern as the Thirst line.
CREATE TEMPORARY TABLE aotv4_pal_tmp AS SELECT * FROM spells_new WHERE id = 2190;
UPDATE aotv4_pal_tmp SET id = 44600, name = 'Ardent Strike', descnum = 44600,
  targettype = 5, goodEffect = 0, mana = 0, cast_time = 0, recast_time = 0, recovery_time = 0,
  buffduration = 0, buffdurationformula = 0, new_icon = 168,
  effectid1 = 254, effect_base_value1 = 0, formula1 = 100, max1 = 0,
  effectid2 = 254, effect_base_value2 = 0, effectid3 = 254, effect_base_value3 = 0;
INSERT INTO spells_new SELECT * FROM aotv4_pal_tmp;
DROP TEMPORARY TABLE aotv4_pal_tmp;

-- 44601 Hand of Conviction -- SELF target on purpose: the heal is a percentage of the CASTER's max
-- HP and lands on the whole group, neither of which a spell row can say, so the Lua walks the group.
-- A group target type here would only duplicate what the script already does.
CREATE TEMPORARY TABLE aotv4_pal_tmp AS SELECT * FROM spells_new WHERE id = 2190;
UPDATE aotv4_pal_tmp SET id = 44601, name = 'Hand of Conviction', descnum = 44601,
  targettype = 6, goodEffect = 1, mana = 0, cast_time = 0, recast_time = 0, recovery_time = 0,
  buffduration = 0, buffdurationformula = 0, new_icon = 129,
  effectid1 = 254, effect_base_value1 = 0, formula1 = 100, max1 = 0,
  effectid2 = 254, effect_base_value2 = 0, effectid3 = 254, effect_base_value3 = 0;
INSERT INTO spells_new SELECT * FROM aotv4_pal_tmp;
DROP TEMPORARY TABLE aotv4_pal_tmp;

-- 44602 Divine Reproach -- the stun is a REAL SPA 21, kept from Divine Stun (base 2000 = 2 seconds).
-- Only the cooldown cut on Hand of Conviction is paid by Lua. Using the native effect means the
-- engine handles resist, immunity and the stun message for free.
CREATE TEMPORARY TABLE aotv4_pal_tmp AS SELECT * FROM spells_new WHERE id = 2190;
UPDATE aotv4_pal_tmp SET id = 44602, name = 'Divine Reproach', descnum = 44602,
  targettype = 5, goodEffect = 0, mana = 0, cast_time = 0, recast_time = 0, recovery_time = 0,
  new_icon = 167;
INSERT INTO spells_new SELECT * FROM aotv4_pal_tmp;
DROP TEMPORARY TABLE aotv4_pal_tmp;

-- ⚠️⚠️ NOT SCRIBABLE BY ANY CLASS. 255 in every class column keeps them out of the reward pool and
-- out of spellbooks -- they are reachable ONLY through the AA that casts them.
UPDATE spells_new SET classes1=255,classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,
  classes7=255,classes8=255,classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,
  classes14=255,classes15=255,classes16=255
WHERE id BETWEEN 44600 AND 44602;

-- spell descriptions (db_str type 6, keyed on descnum). No apostrophes, no percent signs.
DELETE FROM db_str WHERE id BETWEEN 44600 AND 44602 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
  (44600, 6, 'Strikes your target with righteous force. The blow grows with your Strength, and never falls below what your level alone can muster.'),
  (44601, 6, 'Calls down aid on your whole group, healing each of them for a quarter of your own maximum hit points.'),
  (44602, 6, 'Rebukes your target, stunning it briefly and hastening the return of Hand of Conviction by five seconds.');

-- ============================ the three AAs ============================
-- ⚠️⚠️ HOSTED ON NATIVE ROWS. A new aa_ability id is resolved by the server, passes every check, and
-- is then SILENTLY DISCARDED by the client (section 10). Hosts 45 / 55 / 79 are disabled single-rank
-- natives; their ids, rank ids and sids are kept and only the payload is replaced.
-- ⚠️⚠️ `classes = 4` IS PALADIN AND THE SHIFT IS WHY. The loader does `a->classes = e.classes << 1`
-- (zone/aa.cpp), so the DB bit 1<<(class-1) becomes 1<<class in memory, which is exactly what the
-- stock gate `classes & (1<<GetClass())` tests. Paladin is class 3, so 1<<2 = 4. Do NOT write 8.
UPDATE aa_ability SET enabled = 1, classes = 4, grant_only = 1, type = 1, category = -1,
       charges = 0, reset_on_death = 0
WHERE id IN (45, 55, 79);

-- ⚠️⚠️ EVERY AA NEEDS ITS OWN `spell_type` -- IT IS THE SHARED TIMER NUMBER, NOT A CATEGORY.
-- Two AAs on the same spell_type share one cooldown, so reusing a value here would make the stun and
-- the group heal lock each other out. 100-102 are free (max in use is 99; the range runs to 1999).
-- ⚠️ recast_time is in SECONDS.
UPDATE aa_ranks SET spell = 44600, spell_type = 100, recast_time = 10,  level_req = 1, cost = 1 WHERE id = 144;
UPDATE aa_ranks SET spell = 44601, spell_type = 101, recast_time = 120, level_req = 5, cost = 1 WHERE id = 158;
UPDATE aa_ranks SET spell = 44602, spell_type = 102, recast_time = 15,  level_req = 10, cost = 1 WHERE id = 196;

-- ⚠️⚠️ `aa_rank_effects` AND `aa_rank_prereqs` ARE SEPARATE TABLES AND BOTH MUST BE CLEARED. A hosted
-- AA inherits its host's prerequisites and refuses to train WITH NO MESSAGE (section 10); clearing
-- effects alone does nothing about it. Eight AAs were untrainable this way once.
DELETE FROM aa_rank_effects WHERE rank_id IN (144, 158, 196);
DELETE FROM aa_rank_prereqs WHERE rank_id IN (144, 158, 196);

-- ⚠️⚠️ AN AA HAS THREE NAMES IN db_str AND THE HOTKEY ONE IS SPLIT ACROSS TWO ROWS (section 6):
-- type 1 the window title, type 2 the hotkey UPPER line, type 3 its LOWER line, type 4 the
-- description. Writing only 1 and 4 leaves the HOST's name on any hotkey made from the ability.
DELETE FROM db_str WHERE id IN (144, 158, 196) AND type IN (1, 2, 3, 4);
INSERT INTO db_str (id, type, value) VALUES
  (144, 1, 'Ardent Strike'),      (144, 2, 'Ardent'),  (144, 3, 'Strike'),
  (144, 4, 'Strikes your target with righteous force. The blow grows with your Strength, and never falls below what your level alone can muster. Reusable every 10 seconds.'),
  (158, 1, 'Hand of Conviction'), (158, 2, 'Hand of'), (158, 3, 'Conviction'),
  (158, 4, 'Calls down aid on your whole group, healing each of them for a quarter of your own maximum hit points. Reusable every 2 minutes.'),
  (196, 1, 'Divine Reproach'),    (196, 2, 'Divine'),  (196, 3, 'Reproach'),
  (196, 4, 'Rebukes your target, stunning it briefly. Each use hastens the return of Hand of Conviction by five seconds. Reusable every 15 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 95,
		.description = "2026_08_19_paladin_aa_endurance",
		// Submitted by: maintainer
		// Makes the three Paladin class AAs cost endurance, the way combat specials do (section 22).
		.check       = "SELECT id FROM spells_new WHERE id = 44602 AND EndurCost > 0",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new SET EndurCost = 25  WHERE id = 44600;   -- Ardent Strike      (10s)
UPDATE spells_new SET EndurCost = 100 WHERE id = 44601;   -- Hand of Conviction (2m)
UPDATE spells_new SET EndurCost = 40  WHERE id = 44602;   -- Divine Reproach    (15s)
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 96,
		.description = "2026_08_19_paladin_aa_hotkey_sids",
		// Submitted by: maintainer
		// Points the three Paladin AAs' hotkey text at their db_str rows. Their hosts were disabled
		.check       = "SELECT id FROM aa_ranks WHERE id = 196 AND upper_hotkey_sid > 0",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_ranks SET upper_hotkey_sid = 144, lower_hotkey_sid = 144 WHERE id = 144;
UPDATE aa_ranks SET upper_hotkey_sid = 158, lower_hotkey_sid = 158 WHERE id = 158;
UPDATE aa_ranks SET upper_hotkey_sid = 196, lower_hotkey_sid = 196 WHERE id = 196;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 97,
		.description = "2026_08_19_paladin_aa_timer_ids",
		// Submitted by: maintainer
		// Moves the three Paladin AAs onto timer ids the client can render. At 100-102 the AA window
		.check       = "SELECT id FROM aa_ranks WHERE id = 196 AND spell_type = 84",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE aa_ranks SET spell_type = 82 WHERE id = 144;   -- Ardent Strike
UPDATE aa_ranks SET spell_type = 83 WHERE id = 158;   -- Hand of Conviction
UPDATE aa_ranks SET spell_type = 84 WHERE id = 196;   -- Divine Reproach
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 98,
		.description = "2026_08_19_paladin_aa_visuals",
		// Submitted by: maintainer
		// Gives the three Paladin AAs distinct visuals. All three were cloned from Divine Stun in
		.check       = "SELECT id FROM spells_new WHERE id = 44601 AND spellanim = 278",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE spells_new SET CastingAnim = 0, TargetAnim = 0, spellanim = 0, spell_category = 15
WHERE id = 44600;

-- 44601 Hand of Conviction -- the heal look, taken from 13 Complete Heal: CastingAnim 43 (the
-- kneeling/hands-up heal cast), spellanim 278 (the healing burst), spell_category 20 (Heals).
-- ⚠️ spell_category is what the client uses to colour and group the effect; 15 is Stuns, which is why
-- a heal was rendering as one.
UPDATE spells_new SET CastingAnim = 43, TargetAnim = 0, spellanim = 278, spell_category = 20
WHERE id = 44601;

-- 44602 Divine Reproach -- KEEPS the Divine Stun look. It really is a stun, so 44/13/103/15 is the
-- correct inheritance here and is left alone deliberately; only the other two were wrong.
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 99,
		.description = "2026_08_20_show_target_buffs",
		// Submitted by: maintainer
		// Players could not see debuffs on the mobs they were fighting. Turns target buff/debuff
		.check       = "SELECT rule_value FROM rule_values WHERE rule_name = 'Spells:AlwaysSendTargetsBuffs' AND rule_value = 'true'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE rule_values SET rule_value = 'true' WHERE rule_name = 'Spells:AlwaysSendTargetsBuffs';

-- ⚠️ INSERT if the row is absent entirely: a database that has never had this rule written falls back
-- to the compiled default (false), so the UPDATE above would match nothing and change nothing. Live
-- may well be in that state.
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
SELECT 1, 'Spells:AlwaysSendTargetsBuffs', 'true', 'AoTv4: target buff/debuff display for everyone'
WHERE NOT EXISTS (SELECT 1 FROM rule_values WHERE rule_name = 'Spells:AlwaysSendTargetsBuffs');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 100,
		.description = "2026_08_20_implied_targeting",
		// Submitted by: maintainer
		// Spells now pass through an inappropriate target to that target's target. Keep the mob
		.check       = "SELECT rule_value FROM rule_values WHERE rule_name = 'Spells:UseSpellImpliedTargeting' AND rule_value = 'true'",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
UPDATE rule_values SET rule_value = 'true' WHERE rule_name = 'Spells:UseSpellImpliedTargeting';

-- ⚠️ INSERT when the row is absent: a database that never wrote this rule falls back to the compiled
-- default (false) and the UPDATE above would match nothing at all.
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
SELECT 1, 'Spells:UseSpellImpliedTargeting', 'true', 'AoTv4: pass spells through to the targets target'
WHERE NOT EXISTS (SELECT 1 FROM rule_values WHERE rule_name = 'Spells:UseSpellImpliedTargeting');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 101,
		.description = "2026_08_18_npc_scaling_tables",
		// Submitted by: Carolus.  Three lookup tables for scaling NPCs off their race, class and
		// raid tier.  Data only -- NOTHING READS THEM YET, so applying this changes no behaviour.
		//
		// ⚠️ The match is `npc_raid_scale`, the LAST table created, NOT `npc_class_scale` as
		// submitted.  Keyed on the first, a run that died after creating one or two tables would
		// record itself as finished and the rest would never be created -- and CREATE TABLE IF NOT
		// EXISTS makes re-running the whole block free, so there is no reason to key on the first.
		.check       = "SELECT 1",
		.condition   = "table_missing",
		.match       = "npc_raid_scale",
		.sql         = R"(
-- Dumping structure for table peq.npc_class_scale
CREATE TABLE IF NOT EXISTS `npc_class_scale` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `class_id` int(11) NOT NULL,
  `name` varchar(64) DEFAULT '',
  `str_multiplier` float NOT NULL DEFAULT 1,
  `sta_multiplier` float NOT NULL DEFAULT 1,
  `dex_multiplier` float NOT NULL DEFAULT 1,
  `agi_multiplier` float NOT NULL DEFAULT 1,
  `int_multiplier` float NOT NULL DEFAULT 1,
  `wis_multiplier` float NOT NULL DEFAULT 1,
  `cha_multiplier` float NOT NULL DEFAULT 1,
  `ac_multiplier` float NOT NULL DEFAULT 1,
  `hp_multiplier` float NOT NULL DEFAULT 1,
  `mana_multiplier` float NOT NULL DEFAULT 1,
  `min_dmg_multiplier` float NOT NULL DEFAULT 1,
  `max_dmg_multiplier` float NOT NULL DEFAULT 1,
  `resist_magic_multiplier` float NOT NULL DEFAULT 1,
  `resist_fire_multiplier` float NOT NULL DEFAULT 1,
  `resist_cold_multiplier` float NOT NULL DEFAULT 1,
  `resist_poison_multiplier` float NOT NULL DEFAULT 1,
  `resist_disease_multiplier` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=45 DEFAULT CHARSET=latin1 COLLATE=latin1_swedish_ci;

-- Dumping structure for table peq.npc_race_stats
CREATE TABLE IF NOT EXISTS `npc_race_stats` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `race_id` int(11) NOT NULL,
  `name` varchar(64) DEFAULT NULL,
  `base_str` int(11) NOT NULL DEFAULT 0,
  `base_sta` int(11) NOT NULL DEFAULT 0,
  `base_dex` int(11) NOT NULL DEFAULT 0,
  `base_agi` int(11) NOT NULL DEFAULT 0,
  `base_int` int(11) NOT NULL DEFAULT 0,
  `base_wis` int(11) NOT NULL DEFAULT 0,
  `base_cha` int(11) NOT NULL DEFAULT 0,
  `base_min_dmg` int(11) NOT NULL DEFAULT 0,
  `base_max_dmg` int(11) NOT NULL DEFAULT 0,
  `bodytype` int(11) NOT NULL DEFAULT 0,
  `base_ac` int(11) NOT NULL DEFAULT 0,
  `mr` int(11) NOT NULL DEFAULT 0,
  `fr` int(11) NOT NULL DEFAULT 0,
  `cr` int(11) NOT NULL DEFAULT 0,
  `pr` int(11) NOT NULL DEFAULT 0,
  `dr` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_race_id` (`race_id`)
) ENGINE=InnoDB AUTO_INCREMENT=194 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- Dumping structure for table peq.npc_raid_scale
CREATE TABLE IF NOT EXISTS `npc_raid_scale` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `raid_tier` int(11) NOT NULL DEFAULT 0,
  `str_multiplier` float NOT NULL DEFAULT 1,
  `sta_multiplier` float NOT NULL DEFAULT 1,
  `dex_multiplier` float NOT NULL DEFAULT 1,
  `agi_multiplier` float NOT NULL DEFAULT 1,
  `int_multiplier` float NOT NULL DEFAULT 1,
  `wis_multiplier` float NOT NULL DEFAULT 1,
  `cha_multiplier` float NOT NULL DEFAULT 1,
  `ac_multiplier` float NOT NULL DEFAULT 1,
  `hp_multiplier` float NOT NULL DEFAULT 1,
  `mana_multiplier` float NOT NULL DEFAULT 1,
  `min_dmg_multiplier` float NOT NULL DEFAULT 1,
  `max_dmg_multiplier` float NOT NULL DEFAULT 1,
  `resist_magic_multiplier` float NOT NULL DEFAULT 1,
  `resist_fire_multiplier` float NOT NULL DEFAULT 1,
  `resist_cold_multiplier` float NOT NULL DEFAULT 1,
  `resist_poison_multiplier` float NOT NULL DEFAULT 1,
  `resist_disease_multiplier` float NOT NULL DEFAULT 1,
  `size_multiplier` float NOT NULL DEFAULT 1,
  `agro_range_multiplier` float NOT NULL DEFAULT 1,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_raid_tier` (`raid_tier`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 102,
		.description = "2026_08_18_npc_scaling_data",
		// Submitted by: Carolus.  Populates the three tables created by v101: 3 raid tiers,
		// 193 races, 38 classes.
		//
		// ⚠️ The check tests `npc_class_scale` id 44 -- the LAST row of the LAST table
		// inserted, not `npc_race_stats` id 1 as submitted.  Same half-applied trap as v101.
		// ⚠️ Split from v101 deliberately: DDL and data in one entry cannot be re-run safely,
		// because CREATE TABLE commits implicitly and would strand the inserts behind a condition
		// that already reads as satisfied.
		.check       = "SELECT `name` FROM `npc_class_scale` WHERE `id` = 44",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- Dumping data for table peq.npc_raid_scale: ~3 rows (approximately)
REPLACE INTO `npc_raid_scale` (`id`, `raid_tier`, `str_multiplier`, `sta_multiplier`, `dex_multiplier`, `agi_multiplier`, `int_multiplier`, `wis_multiplier`, `cha_multiplier`, `ac_multiplier`, `hp_multiplier`, `mana_multiplier`, `min_dmg_multiplier`, `max_dmg_multiplier`, `resist_magic_multiplier`, `resist_fire_multiplier`, `resist_cold_multiplier`, `resist_poison_multiplier`, `resist_disease_multiplier`, `size_multiplier`, `agro_range_multiplier`) VALUES
	(1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
	(2, 1, 1.25, 1.25, 1.25, 1.25, 1.25, 1.25, 1.25, 1.2, 2, 1.5, 1.1, 1.5, 1.25, 1.25, 1.25, 1.25, 1.25, 1.25, 1.25),
	(3, 2, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 1.5, 15, 2, 1.15, 3, 1.3, 1.3, 1.3, 1.3, 1.3, 1.3, 2);


-- Dumping data for table peq.npc_race_stats: ~193 rows (approximately)
REPLACE INTO `npc_race_stats` (`id`, `race_id`, `name`, `base_str`, `base_sta`, `base_dex`, `base_agi`, `base_int`, `base_wis`, `base_cha`, `base_min_dmg`, `base_max_dmg`, `bodytype`, `base_ac`, `mr`, `fr`, `cr`, `pr`, `dr`) VALUES
	(1, 3, 'Erudite', 20, 25, 25, 25, 35, 30, 20, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(2, 367, 'Skeleton', 15, 15, 35, 35, 25, 25, 15, 1, 5, 2, 30, 5, 25, 25, 50, 50),
	(3, 415, 'Rat', 15, 15, 30, 40, 25, 25, 10, 1, 4, 20, 40, 25, 5, 5, 25, 100),
	(4, 491, 'Bone Golem', 30, 35, 25, 25, 15, 15, 15, 1, 8, 2, 75, 5, 25, 25, 50, 50),
	(5, 660, 'Book', 10, 10, 25, 25, 45, 30, 40, 1, 3, 7, 40, 75, 5, 25, 25, 25),
	(6, 669, 'Blind Dreamer', 35, 30, 25, 25, 25, 25, 25, 1, 6, 7, 100, 15, 65, 15, 15, 15),
	(7, 678, 'Erudite', 20, 25, 25, 25, 35, 30, 20, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(8, 6, 'Dark Elf', 20, 25, 30, 25, 30, 25, 30, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(9, 9, 'Troll', 30, 35, 25, 25, 15, 20, 15, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(10, 14, 'Werewolf', 35, 25, 30, 30, 25, 25, 25, 1, 7, 1, 65, 25, 5, 50, 25, 25),
	(11, 29, 'Gargoyle', 25, 40, 25, 25, 25, 25, 25, 1, 5, 7, 100, 5, 25, 25, 25, 25),
	(12, 46, 'Fire Imp', 25, 25, 45, 30, 25, 25, 25, 1, 8, 0, 50, 25, 75, 5, 25, 25),
	(13, 71, 'Human', 30, 25, 30, 25, 30, 25, 30, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(14, 98, 'Drake', 35, 35, 15, 25, 35, 15, 20, 1, 6, 25, 65, 5, 25, 25, 25, 25),
	(15, 120, 'Ghost Wolf', 25, 25, 25, 25, 25, 25, 25, 1, 5, 2, 50, 5, 5, 45, 65, 65),
	(16, 127, 'Shadowman', 30, 25, 30, 25, 30, 25, 30, 1, 5, 0, 150, 5, 5, 5, 25, 25),
	(17, 1, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(18, 2, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(19, 4, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(20, 7, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(21, 8, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(22, 12, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(23, 22, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(24, 34, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(25, 36, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(26, 37, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(27, 39, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(28, 42, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(29, 43, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(30, 69, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(31, 70, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(32, 130, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(33, 141, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(34, 240, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(35, 21, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(36, 24, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(37, 28, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(38, 31, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(39, 40, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(40, 52, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(41, 10, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(42, 18, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(43, 33, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(44, 38, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(45, 50, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(46, 64, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(47, 82, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(48, 11, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(49, 47, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(50, 49, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(51, 566, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(52, 13, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(53, 16, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(54, 85, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(55, 107, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(56, 125, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(57, 5, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(58, 17, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(59, 30, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(60, 53, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(61, 100, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(62, 74, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(63, 209, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(64, 210, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(65, 211, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(66, 212, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(67, 258, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(68, 271, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(69, 272, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(70, 300, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(71, 54, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(72, 58, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(73, 63, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(74, 65, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(75, 79, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(76, 224, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(77, 243, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(78, 244, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(79, 330, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(80, 416, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(81, 439, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(82, 454, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(83, 470, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(84, 473, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(85, 44, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(86, 75, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(87, 76, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(88, 128, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(89, 26, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(90, 81, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(91, 89, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(92, 109, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(93, 471, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(94, 55, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(95, 73, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(96, 87, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(97, 91, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(98, 361, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(99, 27, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(100, 61, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(101, 51, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(102, 86, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(103, 134, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(104, 203, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(105, 344, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(106, 350, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(107, 371, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(108, 433, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(109, 440, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(110, 455, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(111, 468, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(112, 161, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(113, 15, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(114, 25, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(115, 56, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(116, 106, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(117, 112, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(118, 113, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(119, 88, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(120, 124, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(121, 48, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(122, 68, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(123, 80, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(124, 117, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(125, 103, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(126, 105, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(127, 110, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(128, 116, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(129, 72, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(130, 165, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(131, 41, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(132, 118, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(133, 129, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(134, 139, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(135, 144, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(136, 137, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(137, 138, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(138, 140, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(139, 104, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(140, 159, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(141, 162, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(142, 57, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(143, 133, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(144, 59, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(145, 119, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(146, 131, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(147, 148, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(148, 155, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(149, 96, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(150, 163, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(151, 20, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(152, 146, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(153, 147, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(154, 160, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(155, 164, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(156, 166, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(157, 145, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(158, 45, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(159, 122, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(160, 353, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(161, 354, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(162, 355, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(163, 357, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(164, 170, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(165, 177, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(166, 183, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(167, 188, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(168, 194, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(169, 315, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(170, 101, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(171, 174, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(172, 175, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(173, 185, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(174, 193, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(175, 508, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(176, 189, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(177, 135, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(178, 172, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(179, 176, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(180, 191, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(181, 198, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(182, 158, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(183, 181, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(184, 178, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(185, 157, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(186, 184, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(187, 195, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(188, 432, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(189, 464, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(190, 77, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(191, 123, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(192, 351, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25),
	(193, 352, 'Unknown', 25, 25, 25, 25, 25, 25, 25, 1, 5, 0, 50, 25, 25, 25, 25, 25);

-- Dumping data for table peq.npc_class_scale: ~38 rows (approximately)
REPLACE INTO `npc_class_scale` (`id`, `class_id`, `name`, `str_multiplier`, `sta_multiplier`, `dex_multiplier`, `agi_multiplier`, `int_multiplier`, `wis_multiplier`, `cha_multiplier`, `ac_multiplier`, `hp_multiplier`, `mana_multiplier`, `min_dmg_multiplier`, `max_dmg_multiplier`, `resist_magic_multiplier`, `resist_fire_multiplier`, `resist_cold_multiplier`, `resist_poison_multiplier`, `resist_disease_multiplier`) VALUES
	(1, 1, 'Warrior', 2, 1, 1, 1, 1, 1, 1, 1.2, 1.2, 0, 1, 1.5, 1, 1, 1, 1, 1),
	(2, 2, 'Cleric', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(3, 3, 'Paladin', 1.5, 1.1, 1, 1, 1, 1, 1, 1.1, 1.1, 1, 1, 1.2, 1, 1, 1, 1, 1),
	(4, 4, 'Ranger', 2, 1, 2.5, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1),
	(5, 5, 'Shadow Knight', 1.5, 1.1, 1, 1, 1, 1, 1, 1.1, 1.1, 1, 1, 1.2, 1, 1, 1, 1, 1),
	(6, 6, 'Druid', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(7, 7, 'Monk', 2.2, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1.6, 1, 1, 1, 1, 1),
	(8, 8, 'Bard', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
	(9, 9, 'Rogue', 2.5, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 2, 1, 1, 1, 1, 1),
	(10, 10, 'Shaman', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(11, 11, 'Necromancer', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(12, 12, 'Wizard', 1, 1, 1, 1, 2.6, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(13, 13, 'Magician', 1, 1, 1, 1, 2.5, 1, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(14, 14, 'Enchanter', 1, 1, 1, 1, 1, 1, 2, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(15, 15, 'Beastlord', 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
	(16, 16, 'Berserker', 2.5, 1.1, 1, 1, 1, 1, 1, 1.1, 1.1, 0, 1, 2, 1, 1, 1, 1, 1),
	(17, 0, 'Other', 1, 1.1, 1, 1, 1, 1, 1, 1, 1.1, 1, 1, 1, 1, 1, 1, 1, 1),
	(20, 20, 'Warrior GM', 1, 1.5, 1.1, 1.2, 0.5, 0.5, 1, 1, 1.5, 0, 1, 1, 1, 1, 1, 1, 1),
	(21, 21, 'Cleric GM', 0.5, 1, 0.5, 0.5, 1.2, 1.8, 1.2, 1.5, 1.1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(22, 22, 'Paladin GM', 1.3, 1.4, 0.8, 0.6, 0.3, 1.2, 1, 1.5, 1.4, 1, 1, 1, 1, 1, 1, 1, 1),
	(23, 23, 'Ranger GM', 1.7, 1, 2.5, 1.4, 0.7, 0.6, 0.8, 1.3, 1, 0, 1, 1, 1, 1, 1, 1, 1),
	(24, 24, 'SK GM', 1.5, 1.4, 0.9, 0.8, 1.2, 0.3, 0.5, 1.5, 1.4, 1, 1, 1, 1, 1, 1, 1, 1),
	(25, 25, 'Druid GM', 0.5, 1, 0.6, 1.4, 1, 1.7, 1, 1, 1.1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(26, 26, 'Monk GM', 2.5, 1.5, 1, 2, 0.8, 1.2, 0.6, 1, 1.4, 0, 1, 1, 1, 1, 1, 1, 1),
	(27, 27, 'Bard GM', 1, 1.1, 1, 1, 1, 1, 1, 1.3, 1.1, 1, 1, 1, 1, 1, 1, 1, 1),
	(28, 28, 'Rogue GM', 2.5, 1.1, 1.4, 1.8, 1, 0.5, 1.1, 1.3, 1, 0, 1, 1, 1, 1, 1, 1, 1),
	(29, 29, 'Shaman GM', 0.5, 1, 0.6, 0.9, 1.1, 1.8, 1, 1.3, 1.1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(30, 30, 'Necromancer GM', 0.5, 1, 0.6, 1.6, 1.8, 1.8, 0.7, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(31, 31, 'Wizard GM', 0.5, 1, 0.6, 1, 2.6, 1.3, 1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(32, 32, 'Magician GM', 0.5, 1, 0.6, 1, 2.5, 1.3, 1.1, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(33, 33, 'Enchanter GM', 0.5, 1, 0.6, 1, 1.8, 1.1, 2, 1, 1, 1.5, 1, 1, 1, 1, 1, 1, 1),
	(34, 34, 'Beastlord GM', 1, 1, 1.2, 1.5, 0.6, 1.3, 0.8, 1, 1.1, 1, 1, 1, 1, 1, 1, 1, 1),
	(35, 35, 'Berserker GM', 2.5, 1.2, 1.7, 1.3, 0.4, 0.6, 0.7, 1.3, 1, 0, 1, 1, 1, 1, 1, 1, 1),
	(40, 40, 'Banker', 100, 100, 100, 100, 100, 100, 100, 10, 1, 1, 1, 10, 100, 100, 100, 100, 1),
	(41, 41, 'Merchant', 100, 100, 100, 100, 100, 100, 100, 10, 1, 1, 1, 10, 100, 100, 100, 100, 1),
	(42, 42, 'Quest', 100, 100, 100, 100, 100, 100, 100, 10, 1, 1, 1, 10, 100, 100, 100, 100, 1),
	(43, 43, 'Adventure_Merchant', 100, 100, 100, 100, 100, 100, 100, 10, 1, 1, 1, 10, 100, 100, 100, 100, 1),
	(44, 44, 'Adventure_Recruiter', 100, 100, 100, 100, 100, 100, 100, 10, 1, 1, 1, 10, 100, 100, 100, 100, 1);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 103,
		.description = "2026_08_20_parcel_courier_unspawnable_cities",
		// Submitted by: Claude
		// counted those eight as served and skipped them.
		.check       = "SELECT z.short_name FROM zone z JOIN spawn2 s ON s.zone = z.short_name JOIN spawnentry se ON se.spawngroupID = s.spawngroupID JOIN npc_types n ON n.id = se.npcID AND n.is_parcel_merchant = 1 WHERE z.version = 0 GROUP BY z.short_name HAVING SUM(CASE WHEN s.min_expansion <= 0 AND s.max_expansion <= 0 THEN 1 ELSE 0 END) = 0 LIMIT 1",
		.condition   = "not_empty",
		.match       = "",
		.sql         = R"(
DROP TEMPORARY TABLE IF EXISTS aotv4_parcel_gap;
CREATE TEMPORARY TABLE aotv4_parcel_gap (
  n INT, short_name VARCHAR(32), x FLOAT, y FLOAT, z FLOAT
);

-- ⚠️ The temp table is materialised FIRST because the INSERTs below change what the query returns.
-- Reading it live three times would give three different answers.
INSERT INTO aotv4_parcel_gap
SELECT ROW_NUMBER() OVER (ORDER BY t.short_name), t.short_name, t.safe_x, t.safe_y, t.safe_z
FROM (
  SELECT z.short_name, z.safe_x, z.safe_y, z.safe_z
  FROM zone z
  JOIN spawn2 s       ON s.zone = z.short_name
  JOIN spawnentry se  ON se.spawngroupID = s.spawngroupID
  JOIN npc_types n    ON n.id = se.npcID AND n.is_parcel_merchant = 1
  WHERE z.version = 0
  GROUP BY z.short_name, z.safe_x, z.safe_y, z.safe_z
  HAVING SUM(CASE WHEN s.min_expansion <= 0 AND s.max_expansion <= 0 THEN 1 ELSE 0 END) = 0
) t;

SET @base = (SELECT COALESCE(MAX(id), 2000299) FROM spawngroup WHERE id BETWEEN 2000300 AND 2000350);

INSERT INTO spawngroup (id, name, spawn_limit, dist, max_x, min_x, max_y, min_y, delay, mindelay)
  SELECT @base + n, CONCAT('aotv4_parcel_courier_', short_name), 1, 0, 0, 0, 0, 0, 0, 0
  FROM aotv4_parcel_gap;

INSERT INTO spawnentry (spawngroupID, npcID, chance)
  SELECT @base + n, 2000220, 100 FROM aotv4_parcel_gap;

-- ⚠️⚠️ min_expansion / max_expansion MUST BE -1 -- that is the whole point of this migration.
-- ⚠️ The column is `_condition` (leading underscore); `condition` is reserved.
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid,
                    _condition, cond_value, min_expansion, max_expansion)
  SELECT @base + n, short_name, 0, x, y, z, 0, 600, 0, 0, 0, 1, -1, -1
  FROM aotv4_parcel_gap;

DROP TEMPORARY TABLE IF EXISTS aotv4_parcel_gap;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 104,
		.description = "2026_08_21_warrior_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44702 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
-- ⚠️⚠️ EndurTimerIndex IS LOAD BEARING AND MUST NOT BE 0.
-- A discipline's recast lives EXCLUSIVELY in `pTimerDisciplineReuseStart + timer_id`. CastSpell's
-- per-spell branch is guarded by `&& !spells[spell_id].is_discipline`, so pTimerSpellStart is never
-- started for one. Leaving this 0 would put every class ability on ONE shared cooldown, alongside
-- the 87 stock discs that already use slot 0.
-- ⚠️⚠️ ONLY 0-10 ARE VALID. pTimerDisciplineReuseEnd is 24 and pTimerCombatAbility is 25, so
-- timer_id 11 collides with Kick/Bash, 12 with Tiger Claw, 13 with Begging.
-- 📌 Slots chosen by how far the lowest stock discipline using them sits above our cap:
--      tier 1 -> 5  (lowest stock disc there is level 68, only 19 rows)
--      tier 2 -> 6  (level 66, 25 rows)
--      tier 3 -> 2  (level 56, 39 rows)
--    Not sequential on purpose. 1/4/7/8/9 all carry discs reachable at level 35 or below.
-- 📌 Three slots serve all sixteen classes: a character is only ever one class, so no one can hold
--    two tier 1s. Do NOT allocate per class -- there are only 11 slots in total.

-- ⚠️ EndurCost is 1, not 0 and not the real cost. IsDiscipline requires mana = 0 AND EndurCost > 0,
-- so 0 would leave the row out of the Combat Abilities window entirely. The real level-scaled cost
-- is charged by Lua, because EndurCost is a flat int column and cannot express `N x level`.

-- ⚠️ Scoped to exactly the three ids this migration creates, so a half-applied run (the check keys
-- on the LAST thing written, the 44702 description) re-runs cleanly instead of dying on a duplicate
-- key. Never widen it to the band: 44700-44747 is reserved for all sixteen classes.
DELETE FROM spells_new WHERE id IN (44700, 44701, 44702);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- tier 1: Cleaving Blow
-- Cloned from 4667 Rebuke of the Ikaav: single target, short recast, no buff.
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44700, name = 'Cleaving Blow', descnum = 44700,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, targettype = 5, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 87, spellanim = 0, CastingAnim = 44,
    -- inert marker: the swing, its damage and its hate are paid by Lua through
    -- DoSpecialAttackDamage, which is what gives it mitigation, avoidance and the section 22 charge.
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 0, buffdurationformula = 0, numhits = 0, numhitstype = 0,
    classes1 = 1,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;

-- ---------------------------------------------------------------- tier 3: Broad Cleave
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44702, name = 'Broad Cleave', descnum = 44702,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, targettype = 5, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 89, spellanim = 0, CastingAnim = 44,
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 0, buffdurationformula = 0, numhits = 0, numhitstype = 0,
    classes1 = 10,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;

-- ---------------------------------------------------------------- tier 2: Bulwark
-- Cloned from 4499 Defensive Discipline: a self buff with a real duration.
-- ⚠️⚠️ NO `numhits`, AND THAT IS DELIBERATE. numhitstype 6 (Incoming Hit Successes) would give a
-- free client-side charge counter, but the engine spends the last charge and FADES THE BUFF inside
-- CheckNumHitsRemaining (attack.cpp:4621), which runs BEFORE EVENT_DAMAGE_TAKEN (attack.cpp:4689) in
-- the same Mob::CommonDamage call. The Lua payload would then find no buff on the fifth hit and
-- absorb nothing -- an ability that advertises five and delivers four, silently. The charges are
-- counted in aotv4_class_abilities.lua instead, which also fades the buff when they run out.
-- 📌 buffduration is a CAP on the formula, not an alternative to it (spells.cpp:3300), so
-- formula 11 with duration 10 is a flat 10 tics -- 60 seconds -- at every level. The charges are the
-- real limit; the duration only stops it being pre-cast minutes before a pull.
-- 📌 UseDiscipline refuses a self-buff discipline while any disc buff is already up
-- (`HasDiscBuff()`, effects.cpp:977), so Bulwark cannot be refreshed to top its charges back up.
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44701, name = 'Bulwark', descnum = 44701,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, targettype = 6, skill = 98, mana = 0,
    -- ⚠️ Presentation is set EXPLICITLY. A clone inherits new_icon, spellanim and
    -- CastingAnim like every other column, so leaving them made two of these three share
    -- icon 85 and fire the TEMPLATE's spell particle on what is meant to look like a swing.
    new_icon = 1, spellanim = 0, CastingAnim = 44,
    effectid1 = 254, effectid2 = 254, effectid3 = 254,
    effect_base_value1 = 0, effect_base_value2 = 0, effect_base_value3 = 0,
    formula1 = 100, formula2 = 100, formula3 = 100, max1 = 0, max2 = 0, max3 = 0,
    buffduration = 10, buffdurationformula = 11, numhits = 0, numhitstype = 0,
    classes1 = 5,
    classes2=255,classes3=255,classes4=255,classes5=255,classes6=255,classes7=255,classes8=255,
    classes9=255,classes10=255,classes11=255,classes12=255,classes13=255,classes14=255,
    classes15=255,classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors -- eighteen heals shipped
-- that way. Written in the same migration as the rows, deliberately.
-- ⚠️ No literal percent sign: the description path is printf-style and eats it as a format token.
DELETE FROM db_str WHERE id IN (44700, 44701, 44702) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44700, 6, 'Cleave into your target with your equipped weapon, striking for weapon damage and seizing its attention.'),
 (44701, 6, 'Brace behind your guard. The next 5 melee attacks against you are each reduced by a tenth of your armor class, and any blow weaker than that is turned aside entirely.'),
 (44702, 6, 'Sweep everything in front of you. Each target struck shortens the recovery of Bulwark by 3 seconds, to a maximum of 9. Generates no additional threat.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 105,
		.description = "2026_08_21_cleric_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44750 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44703, 44704, 44705, 44750);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Templar Strike (44703)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44703, name = 'Templar Strike', descnum = 44703,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 88, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 1, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Sanctuary (44704)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44704, name = 'Sanctuary', descnum = 44704,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 41, goodEffect = 1,
    new_icon = 99, spellanim = 278, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 3, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 147, effect_base_value1 = 17, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 289, effect_base_value2 = 44750, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 5, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Condemn (44705)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44705, name = 'Condemn', descnum = 44705,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 26, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -20, effect_limit_value1 = 0, formula1 = 4, max1 = 250,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 10, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Sanctuary Bloom (44750)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44750, name = 'Sanctuary Bloom', descnum = 44750,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 99, spellanim = 278, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 147, effect_base_value1 = 17, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44703, 44704, 44705, 44750) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44703, 6, 'Strike your target with your weapon and draw a measure of vigour back into yourself.'),
 (44704, 6, 'Shelter your group. Each member is healed for a sixth of their own maximum health at once, and again when the shelter lifts three ticks later.'),
 (44705, 6, 'Call down judgement. Shortens Sanctuary by 5 seconds, or by 10 against the undead.'),
 (44750, 6, 'The delayed half of Sanctuary.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 106,
		.description = "2026_08_21_ranger_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44711 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44709, 44710, 44711);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Twin Slash (44709)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44709, name = 'Twin Slash', descnum = 44709,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 91, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 1,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Volley (44710)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44710, name = 'Volley', descnum = 44710,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 92, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 5,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Point Blank Shot (44711)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44711, name = 'Point Blank Shot', descnum = 44711,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 93, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 10,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44709, 44710, 44711) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44709, 6, 'Two swings in the time of one.'),
 (44710, 6, 'Loose a volley at everything in front of you.'),
 (44711, 6, 'Fire at a target already in your face. Shortens Volley by 5 seconds, or by 10 with a bow drawn.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 107,
		.description = "2026_08_21_shadowknight_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44753 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44712, 44713, 44714, 44753);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Reaving Strike (44712)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44712, name = 'Reaving Strike', descnum = 44712,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 47, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 1, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Harrowing (44713)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44713, name = 'Harrowing', descnum = 44713,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 46, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 5, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Reaving Vow (44714)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44714, name = 'Reaving Vow', descnum = 44714,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 48, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 10, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Reaving Fervor (44753)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44753, name = 'Reaving Fervor', descnum = 44753,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 48, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 3, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44712, 44713, 44714, 44753) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44712, 6, 'A cruel swing that draws the life out of what it opens.'),
 (44713, 6, 'Tear the life from everything around you and take it for your own.'),
 (44714, 6, 'Swear the next wound to yourself: your next Reaving Strike drains twice as deeply. Shortens Harrowing by 6 seconds.'),
 (44753, 6, 'Your next Reaving Strike drains twice as deeply.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 108,
		.description = "2026_08_21_druid_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44717 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44715, 44716, 44717);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Thorned Strike (44715)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44715, name = 'Thorned Strike', descnum = 44715,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 25, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 5, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 121, effect_base_value1 = -8, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 1, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Wildgrowth (44716)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44716, name = 'Wildgrowth', descnum = 44716,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 1,
    new_icon = 30, spellanim = 278, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = 12, effect_limit_value1 = 0, formula1 = 1, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 5, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Sunflare (44717)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44717, name = 'Sunflare', descnum = 44717,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 31, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -25, effect_limit_value1 = 0, formula1 = 4, max1 = 300,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 10, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44715, 44716, 44717) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44715, 6, 'Drive thorns into your target. Every melee blow it lands wounds it in turn.'),
 (44716, 6, 'Growth closes wounds faster than they open, for a while.'),
 (44717, 6, 'A lance of light, brighter against something that cannot move. Shortens Wildgrowth by 5 seconds, or by 10 against a rooted or snared target.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 109,
		.description = "2026_08_21_monk_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44720 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44718, 44719, 44720);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Iron Palm (44718)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44718, name = 'Iron Palm', descnum = 44718,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 51, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 1, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Void Stance (44719)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44719, name = 'Void Stance', descnum = 44719,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 52, spellanim = 86, CastingAnim = 42,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 10, buffdurationformula = 11,
    numhits = 4, numhitstype = 1,
    effectid1 = 172, effect_base_value1 = 50, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 5, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Pressure Point (44720)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44720, name = 'Pressure Point', descnum = 44720,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 53, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 10, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44718, 44719, 44720) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44718, 6, 'An open hand driven through the guard, weighted by the pace of your weapon.'),
 (44719, 6, 'Stand in the space between blows. The next 4 melee attacks against you are far likelier to find nothing there.'),
 (44720, 6, 'One strike, placed where it is felt. Shortens Void Stance by 6 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 110,
		.description = "2026_08_21_bard_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44723 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44721, 44722, 44723);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Discordant Strike (44721)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44721, name = 'Discordant Strike', descnum = 44721,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 60, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 5, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 1, effect_base_value1 = -25, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 1,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Crescendo (44722)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44722, name = 'Crescendo', descnum = 44722,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 41, goodEffect = 1,
    new_icon = 61, spellanim = 278, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 189, effect_base_value1 = 100, effect_limit_value1 = 0, formula1 = 3, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 5,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Cadence Strike (44723)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44723, name = 'Cadence Strike', descnum = 44723,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 62, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 5, buffdurationformula = 11,
    numhits = 5, numhitstype = 6,
    effectid1 = 197, effect_base_value1 = 10, effect_limit_value1 = -1, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 10,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44721, 44722, 44723) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44721, 6, 'A note struck wrong. Your target guards itself worse for it.'),
 (44722, 6, 'The music carries your group. Their second wind arrives early.'),
 (44723, 6, 'Set the beat of the fight. The next 5 blows landed on your target hurt it more. Shortens Crescendo by 5 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 111,
		.description = "2026_08_21_rogue_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44726 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44724, 44725, 44726);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Vital Strike (44724)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44724, name = 'Vital Strike', descnum = 44724,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 71, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 4, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 3, effect_base_value1 = -35, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 1, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Rupture (44725)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44725, name = 'Rupture', descnum = 44725,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 72, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 8, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -18, effect_limit_value1 = 0, formula1 = 1, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 5, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Exploit Weakness (44726)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44726, name = 'Exploit Weakness', descnum = 44726,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 73, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 10, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44724, 44725, 44726) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44724, 6, 'Open a leg. What you have cut cannot run.'),
 (44725, 6, 'A wound that will not close on its own.'),
 (44726, 6, 'Put a blade where it is already bleeding, for half as much again. Shortens Rupture by 5 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 112,
		.description = "2026_08_21_shaman_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44752 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44727, 44728, 44729, 44751, 44752);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Spiritual Foresight (44727)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44727, name = 'Spiritual Foresight', descnum = 44727,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 1,
    new_icon = 77, spellanim = 86, CastingAnim = 42,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 55, effect_base_value1 = 60, effect_limit_value1 = 0, formula1 = 3, max1 = 0,
    effectid2 = 323, effect_base_value2 = 44751, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 1, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Crippling Spirit (44728)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44728, name = 'Crippling Spirit', descnum = 44728,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 41, goodEffect = 1,
    new_icon = 78, spellanim = 86, CastingAnim = 42,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 55, effect_base_value1 = 120, effect_limit_value1 = 0, formula1 = 5, max1 = 0,
    effectid2 = 323, effect_base_value2 = 44752, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 5, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Malaise (44729)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44729, name = 'Malaise', descnum = 44729,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 79, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 6, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 4, effect_base_value1 = -20, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 8, effect_base_value2 = -20, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 10, effect_base_value3 = -20, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 2, effect_base_value4 = -25, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 10, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Foresight Chill (44751)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44751, name = 'Foresight Chill', descnum = 44751,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 77, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 2, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 11, effect_base_value1 = 85, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Crippling Chill (44752)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44752, name = 'Crippling Chill', descnum = 44752,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 78, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 2, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 11, effect_base_value1 = 70, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44727, 44728, 44729, 44751, 44752) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44727, 6, 'A ward that sees the blow coming. What strikes through it is slowed for its trouble.'),
 (44728, 6, 'The same ward, over your whole group, and it bites harder.'),
 (44729, 6, 'Sap what your target swings, thinks and commands with. Shortens Crippling Spirit by 5 seconds, or by 10 against something already slowed.'),
 (44751, 6, 'The cold left behind by a warded blow.'),
 (44752, 6, 'The cold left behind by a warded blow.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 113,
		.description = "2026_08_21_necromancer_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44732 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44730, 44731, 44732);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Withering Touch (44730)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44730, name = 'Withering Touch', descnum = 44730,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 41, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 6, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -10, effect_limit_value1 = 0, formula1 = 1, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 1, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Soul Harvest (44731)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44731, name = 'Soul Harvest', descnum = 44731,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 42, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 5, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Toll of the Dead (44732)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44732, name = 'Toll of the Dead', descnum = 44732,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 43, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 10, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44730, 44731, 44732) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44730, 6, 'Your hand leaves rot behind it.'),
 (44731, 6, 'Call in every debt at once. Each affliction on your target is spent for damage and health.'),
 (44732, 6, 'Name the price of what is already killing your target. Shortens Soul Harvest by 15 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 114,
		.description = "2026_08_21_wizard_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44735 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44733, 44734, 44735);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Arcane Fist (44733)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44733, name = 'Arcane Fist', descnum = 44733,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 162, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 1,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Overload (44734)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44734, name = 'Overload', descnum = 44734,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 163, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -40, effect_limit_value1 = 0, formula1 = 20, max1 = 900,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 5,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Ley Tap (44735)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44735, name = 'Ley Tap', descnum = 44735,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 164, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -15, effect_limit_value1 = 0, formula1 = 3, max1 = 200,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 10,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44733, 44734, 44735) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44733, 6, 'Strike with the hand that is not casting, and take a little power back from the impact.'),
 (44734, 6, 'Pour everything into one cast. The recoil leaves you reeling for a moment.'),
 (44735, 6, 'Draw off a thread of the ley. Each thread makes your next Overload land harder, up to three. Shortens Overload by 5 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 115,
		.description = "2026_08_21_magician_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44754 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44736, 44737, 44738, 44754);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Elemental Fist (44736)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44736, name = 'Elemental Fist', descnum = 44736,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 38, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 1, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Elemental Swarm (44737)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 3265;
UPDATE aotv4_disc_tmpl SET
    id = 44737, name = 'Elemental Swarm', descnum = 44737,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 105, spellanim = 80, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = 'ServantRo',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 152, effect_base_value1 = 1, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 5, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Cinder Blast (44738)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44738, name = 'Cinder Blast', descnum = 44738,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 106, spellanim = 202, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 0, effect_base_value1 = -30, effect_limit_value1 = 0, formula1 = 3, max1 = 250,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 10, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Elemental Guardian (44754)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 285;
UPDATE aotv4_disc_tmpl SET
    id = 44754, name = 'Elemental Guardian', descnum = 44754,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 38, spellanim = 306, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = 'SumEarthR2',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 33, effect_base_value1 = 1, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44736, 44737, 44738, 44754) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44736, 6, 'A fist wrapped in flame, and a guardian at your shoulder if you have none.'),
 (44737, 6, 'Call a brief host of servants to fight beside your guardian.'),
 (44738, 6, 'A burst of cinders, twice as fierce on whatever your pet is already fighting. Shortens Elemental Swarm by 7 seconds.'),
 (44754, 6, 'The guardian granted by Elemental Fist.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 116,
		.description = "2026_08_21_enchanter_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44741 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44739, 44740, 44741);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Tashania (44739)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44739, name = 'Tashania', descnum = 44739,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 21, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 8, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 111, effect_base_value1 = -15, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 1, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Gift of Thought (44740)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44740, name = 'Gift of Thought', descnum = 44740,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 41, goodEffect = 1,
    new_icon = 22, spellanim = 278, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 15, effect_base_value1 = 100, effect_limit_value1 = 0, formula1 = 3, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 5, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Mind Fray (44741)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44741, name = 'Mind Fray', descnum = 44741,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 23, spellanim = 113, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 3, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 286, effect_base_value1 = 30, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 10, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44739, 44740, 44741) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44739, 6, 'Thin every ward your target has against magic.'),
 (44740, 6, 'Hand your group back the thoughts they have spent.'),
 (44741, 6, 'Fray your own mind against the working. Your spells land harder for three ticks. Shortens Gift of Thought by 5 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 117,
		.description = "2026_08_21_beastlord_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44755 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44742, 44743, 44744, 44755);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Feral Swipe (44742)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44742, name = 'Feral Swipe', descnum = 44742,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 108, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 1, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Feral Frenzy (44743)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4499;
UPDATE aotv4_disc_tmpl SET
    id = 44743, name = 'Feral Frenzy', descnum = 44743,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 109, spellanim = 86, CastingAnim = 42,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0,
    effectid1 = 11, effect_base_value1 = 125, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 0, effect_base_value2 = 10, effect_limit_value2 = 0, formula2 = 1, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 5, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Bloodscent (44744)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44744, name = 'Bloodscent', descnum = 44744,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 110, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 10, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Feral Companion (44755)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 285;
UPDATE aotv4_disc_tmpl SET
    id = 44755, name = 'Feral Companion', descnum = 44755,
    EndurCost = 1, EndurTimerIndex = 0, recast_time = 0, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 108, spellanim = 306, CastingAnim = 43,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = 'SpiritWolf224',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 33, effect_base_value1 = 1, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44742, 44743, 44744, 44755) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44742, 6, 'A raking blow, and a companion at your side if you have none.'),
 (44743, 6, 'You and your companion both move faster and mend faster.'),
 (44744, 6, 'You can smell the end of it. Far worse for a target below half health, and it shortens Feral Frenzy by 5 seconds, or by 10 on wounded prey.'),
 (44755, 6, 'The companion granted by Feral Swipe.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 118,
		.description = "2026_08_21_berserker_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44747 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44745, 44746, 44747);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Reckless Cleave (44745)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44745, name = 'Reckless Cleave', descnum = 44745,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 55, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 1;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Frenzied Onslaught (44746)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44746, name = 'Frenzied Onslaught', descnum = 44746,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 56, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 5;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Blood Frenzy (44747)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44747, name = 'Blood Frenzy', descnum = 44747,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 57, spellanim = 0, CastingAnim = 44,
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 255, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 10;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44745, 44746, 44747) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44745, 6, 'Everything behind the swing and nothing behind the guard. It costs you a little blood.'),
 (44746, 6, 'Five swings, as fast as you can put them in.'),
 (44747, 6, 'The worse your own wounds, the sooner you can do that again. Shortens Frenzied Onslaught by 2 seconds for every tenth of your health that is gone.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 119,
		.description = "2026_08_21_class_ability_messages",
		// Submitted by: Claude
		// message already says what happened.
		.check       = "SELECT `cast_on_you` FROM `spells_new` WHERE `id` = 44701",
		.condition   = "missing",
		.match       = "You set yourself behind your guard.",
		.sql         = R"(
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44700;
UPDATE spells_new SET cast_on_you = 'You set yourself behind your guard.', cast_on_other = ' sets himself behind his guard.', spell_fades = 'Your guard drops.' WHERE id = 44701;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44702;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44703;
UPDATE spells_new SET cast_on_you = 'You are sheltered.', cast_on_other = ' is sheltered.', spell_fades = 'The shelter lifts.' WHERE id = 44704;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44705;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44709;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44710;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44711;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44712;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44713;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44714;
UPDATE spells_new SET cast_on_you = 'Thorns tear at you.', cast_on_other = ' is wreathed in thorns.', spell_fades = 'The thorns wither.' WHERE id = 44715;
UPDATE spells_new SET cast_on_you = 'Growth closes your wounds.', cast_on_other = ' is wreathed in growth.', spell_fades = 'The growth fades.' WHERE id = 44716;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44717;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44718;
UPDATE spells_new SET cast_on_you = 'You stand in the space between blows.', cast_on_other = ' stands very still.', spell_fades = 'Your stance breaks.' WHERE id = 44719;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44720;
UPDATE spells_new SET cast_on_you = 'Your guard falters.', cast_on_other = ' guards himself worse.', spell_fades = 'Your guard steadies.' WHERE id = 44721;
UPDATE spells_new SET cast_on_you = 'Your second wind arrives early.', cast_on_other = ' catches a second wind.', spell_fades = '' WHERE id = 44722;
UPDATE spells_new SET cast_on_you = 'You are struck on the beat.', cast_on_other = ' is struck on the beat.', spell_fades = 'The beat is lost.' WHERE id = 44723;
UPDATE spells_new SET cast_on_you = 'Your leg gives under you.', cast_on_other = ' staggers.', spell_fades = 'Your leg steadies.' WHERE id = 44724;
UPDATE spells_new SET cast_on_you = 'You are bleeding badly.', cast_on_other = ' is bleeding badly.', spell_fades = 'The bleeding stops.' WHERE id = 44725;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44726;
UPDATE spells_new SET cast_on_you = 'A ward settles over you.', cast_on_other = ' is warded.', spell_fades = 'The ward gutters out.' WHERE id = 44727;
UPDATE spells_new SET cast_on_you = 'A crippling ward settles over you.', cast_on_other = ' is warded.', spell_fades = 'The ward gutters out.' WHERE id = 44728;
UPDATE spells_new SET cast_on_you = 'Your strength deserts you.', cast_on_other = ' sags.', spell_fades = 'Your strength returns.' WHERE id = 44729;
UPDATE spells_new SET cast_on_you = 'Rot spreads through you.', cast_on_other = ' begins to rot.', spell_fades = 'The rot clears.' WHERE id = 44730;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44731;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44732;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44733;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44734;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44735;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44736;
UPDATE spells_new SET cast_on_you = 'Servants answer the call.', cast_on_other = ' calls servants.', spell_fades = '' WHERE id = 44737;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44738;
UPDATE spells_new SET cast_on_you = 'Your wards against magic thin.', cast_on_other = ' is stripped of wards.', spell_fades = 'Your wards close again.' WHERE id = 44739;
UPDATE spells_new SET cast_on_you = 'Your thoughts come back to you.', cast_on_other = ' is given back his thoughts.', spell_fades = '' WHERE id = 44740;
UPDATE spells_new SET cast_on_you = 'You fray your mind against the working.', cast_on_other = ' frays his own mind.', spell_fades = 'Your mind settles.' WHERE id = 44741;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44742;
UPDATE spells_new SET cast_on_you = 'You move faster and mend faster.', cast_on_other = ' turns feral.', spell_fades = 'The frenzy passes.' WHERE id = 44743;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44744;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44745;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44746;
UPDATE spells_new SET cast_on_you = '', cast_on_other = '', spell_fades = '' WHERE id = 44747;
UPDATE spells_new SET cast_on_you = 'The shelter blooms.', cast_on_other = ' is healed by the shelter.', spell_fades = '' WHERE id = 44750;
UPDATE spells_new SET cast_on_you = 'The cold slows you.', cast_on_other = ' slows.', spell_fades = 'The cold passes.' WHERE id = 44751;
UPDATE spells_new SET cast_on_you = 'The cold slows you.', cast_on_other = ' slows.', spell_fades = 'The cold passes.' WHERE id = 44752;
UPDATE spells_new SET cast_on_you = 'You swear the next wound to yourself.', cast_on_other = ' swears a vow.', spell_fades = 'The vow lapses.' WHERE id = 44753;
UPDATE spells_new SET cast_on_you = 'A guardian answers.', cast_on_other = ' calls a guardian.', spell_fades = '' WHERE id = 44754;
UPDATE spells_new SET cast_on_you = 'A companion answers.', cast_on_other = ' calls a companion.', spell_fades = '' WHERE id = 44755;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 120,
		.description = "2026_08_21_class_ability_is_discipline",
		// Submitted by: Claude
		// its template. 32 of these 51 rows were cloned from spells that are not disciplines.
		.check       = "SELECT `IsDiscipline` FROM `spells_new` WHERE `id` = 44700",
		.condition   = "missing",
		.match       = "-1",
		.sql         = R"(
UPDATE spells_new SET IsDiscipline = -1 WHERE id BETWEEN 44700 AND 44755;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 121,
		.description = "2026_08_21_class_ability_particle",
		// Submitted by: Claude
		// presets were sampled from, nukes and heals included.
		.check       = "SELECT `player_1` FROM `spells_new` WHERE `id` = 44700",
		.condition   = "contains",
		.match       = "BLUE_TRAIL",
		.sql         = R"(
UPDATE spells_new SET player_1 = 'PLAYER_1' WHERE id BETWEEN 44700 AND 44755;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 122,
		.description = "2026_08_21_paladin_class_abilities",
		// Submitted by: Claude
		// Cloned via temp table from stock rows so all ~236 columns stay byte-identical.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44708 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44706, 44707, 44708);

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;

-- ---------------------------------------------------------------- Ardent Strike (44706)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44706, name = 'Ardent Strike', descnum = 44706,
    EndurCost = 1, EndurTimerIndex = 5, recast_time = 10000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 94, spellanim = 0, CastingAnim = 44,
    -- ⚠️⚠️ `IsDiscipline` IS A COLUMN AND IT IS NOT THE SAME THING AS THE IsDiscipline() FUNCTION.
    -- The function is DERIVED (mana == 0 AND EndurCost > 0) and was true for all of these from the
    -- start; the COLUMN is spells_new field 168, loaded into spells[].is_discipline, and its own
    -- header comment is "Will goto the combat window when cast". Cloning from a template that is not
    -- a discipline leaves it 0 -- and 0 is what makes the client treat the row as a spell.
    -- ⚠️ Stock writes -1, not 1. Strings::ToBool takes any non-zero number, but match stock.
    IsDiscipline = -1,
    -- ⚠️⚠️ `player_1` IS THE PARTICLE/TRAIL GRAPHIC, AND THE INSTANT TEMPLATE CARRIES `BLUE_TRAIL`.
    -- Cloning 4667 put a blue projectile trail on every sword swing. 263 of the 281 stock
    -- disciplines are `PLAYER_1`, which is the "no special effect" value, and so is every reference
    -- spell used for the presets above -- including the nukes and heals. So it is PLAYER_1 for all
    -- of these, not just the melee ones.
    player_1 = 'PLAYER_1',
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 1, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Hand of Conviction (44707)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44707, name = 'Hand of Conviction', descnum = 44707,
    EndurCost = 1, EndurTimerIndex = 6, recast_time = 120000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 6, goodEffect = 1,
    new_icon = 95, spellanim = 278, CastingAnim = 43,
    -- ⚠️⚠️ `IsDiscipline` IS A COLUMN AND IT IS NOT THE SAME THING AS THE IsDiscipline() FUNCTION.
    -- The function is DERIVED (mana == 0 AND EndurCost > 0) and was true for all of these from the
    -- start; the COLUMN is spells_new field 168, loaded into spells[].is_discipline, and its own
    -- header comment is "Will goto the combat window when cast". Cloning from a template that is not
    -- a discipline leaves it 0 -- and 0 is what makes the client treat the row as a spell.
    -- ⚠️ Stock writes -1, not 1. Strings::ToBool takes any non-zero number, but match stock.
    IsDiscipline = -1,
    -- ⚠️⚠️ `player_1` IS THE PARTICLE/TRAIL GRAPHIC, AND THE INSTANT TEMPLATE CARRIES `BLUE_TRAIL`.
    -- Cloning 4667 put a blue projectile trail on every sword swing. 263 of the 281 stock
    -- disciplines are `PLAYER_1`, which is the "no special effect" value, and so is every reference
    -- spell used for the presets above -- including the nukes and heals. So it is PLAYER_1 for all
    -- of these, not just the melee ones.
    player_1 = 'PLAYER_1',
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 254, effect_base_value1 = 0, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 5, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
-- ---------------------------------------------------------------- Divine Reproach (44708)
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = 4667;
UPDATE aotv4_disc_tmpl SET
    id = 44708, name = 'Divine Reproach', descnum = 44708,
    EndurCost = 1, EndurTimerIndex = 2, recast_time = 15000, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = 5, goodEffect = 0,
    new_icon = 96, spellanim = 0, CastingAnim = 44,
    -- ⚠️⚠️ `IsDiscipline` IS A COLUMN AND IT IS NOT THE SAME THING AS THE IsDiscipline() FUNCTION.
    -- The function is DERIVED (mana == 0 AND EndurCost > 0) and was true for all of these from the
    -- start; the COLUMN is spells_new field 168, loaded into spells[].is_discipline, and its own
    -- header comment is "Will goto the combat window when cast". Cloning from a template that is not
    -- a discipline leaves it 0 -- and 0 is what makes the client treat the row as a spell.
    -- ⚠️ Stock writes -1, not 1. Strings::ToBool takes any non-zero number, but match stock.
    IsDiscipline = -1,
    -- ⚠️⚠️ `player_1` IS THE PARTICLE/TRAIL GRAPHIC, AND THE INSTANT TEMPLATE CARRIES `BLUE_TRAIL`.
    -- Cloning 4667 put a blue projectile trail on every sword swing. 263 of the 281 stock
    -- disciplines are `PLAYER_1`, which is the "no special effect" value, and so is every reference
    -- spell used for the presets above -- including the nukes and heals. So it is PLAYER_1 for all
    -- of these, not just the melee ones.
    player_1 = 'PLAYER_1',
    resisttype = 0, `range` = 200, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = '',
    buffduration = 0, buffdurationformula = 0,
    numhits = 0, numhitstype = 0,
    effectid1 = 21, effect_base_value1 = 1, effect_limit_value1 = 0, formula1 = 100, max1 = 0,
    effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, formula2 = 100, max2 = 0,
    effectid3 = 254, effect_base_value3 = 0, effect_limit_value3 = 0, formula3 = 100, max3 = 0,
    effectid4 = 254, effect_base_value4 = 0, effect_limit_value4 = 0, formula4 = 100, max4 = 0,
    effectid5 = 254, effect_base_value5 = 0, effect_limit_value5 = 0, formula5 = 100, max5 = 0,
    effectid6 = 254, effect_base_value6 = 0, effect_limit_value6 = 0, formula6 = 100, max6 = 0,
    effectid7 = 254, effect_base_value7 = 0, effect_limit_value7 = 0, formula7 = 100, max7 = 0,
    effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 100, max8 = 0,
    effectid9 = 254, effect_base_value9 = 0, effect_limit_value9 = 0, formula9 = 100, max9 = 0,
    effectid10 = 254, effect_base_value10 = 0, effect_limit_value10 = 0, formula10 = 100, max10 = 0,
    effectid11 = 254, effect_base_value11 = 0, effect_limit_value11 = 0, formula11 = 100, max11 = 0,
    effectid12 = 254, effect_base_value12 = 0, effect_limit_value12 = 0, formula12 = 100, max12 = 0,
    classes1 = 255, classes2 = 255, classes3 = 10, classes4 = 255,
    classes5 = 255, classes6 = 255, classes7 = 255, classes8 = 255,
    classes9 = 255, classes10 = 255, classes11 = 255, classes12 = 255,
    classes13 = 255, classes14 = 255, classes15 = 255, classes16 = 255;
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN (44706, 44707, 44708) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44706, 6, 'A weapon blow carried by conviction rather than force, and it draws the eye of what you strike.'),
 (44707, 6, 'Spend your own vitality to mend your group. Each member is healed for a quarter of YOUR maximum health, which is why a Paladin who builds health heals harder.'),
 (44708, 6, 'A rebuke that halts your target briefly and fixes its attention on you. Shortens Hand of Conviction by 5 seconds.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 123,
		.description = "2026_08_21_paladin_retire_aas",
		// Submitted by: Claude
		// with the ability twice, or with none at all.
		.check       = "SELECT `enabled` FROM `aa_ability` WHERE `id` = 45",
		.condition   = "missing",
		.match       = "0",
		.sql         = R"(
DELETE FROM character_alternate_abilities WHERE aa_id IN (144, 158, 196);

-- ⚠️ Disabling is what actually takes them out of the window: `zone/aa.cpp` loads only enabled rows
-- (section 32), so the ability stops existing at runtime rather than merely being unbuyable.
-- 📌 The rank rows, their db_str strings and spells 44600-44602 are LEFT ALONE deliberately. They
-- cost nothing dormant, and they are the only record of how the AA version was built if this is ever
-- revisited. `lua_modules/aotv4_paladin.lua` and quests/global/spells/446xx.lua go dormant with them.
UPDATE aa_ability SET enabled = 0 WHERE id IN (45, 55, 79);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 124,
		.description = "2026_08_21_class_ability_melee_range",
		// Submitted by: Claude
		// summons and group buffs keep 200.
		.check       = "SELECT `range` FROM `spells_new` WHERE `id` = 44700",
		.condition   = "missing",
		.match       = "25",
		.sql         = R"(
UPDATE spells_new SET `range` = 25 WHERE id IN (44700, 44702, 44703, 44706, 44708, 44709, 44710, 44711, 44712, 44713, 44714, 44715, 44718, 44720, 44721, 44723, 44724, 44726, 44730, 44733, 44736, 44742, 44744, 44745, 44746, 44747);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 125,
		.description = "2026_08_21_tradeskill_aa_fix",
		// Submitted by: Carolus
		// ⚠️⚠️ RENUMBERED 92 -> 125 ON MERGE, FOR TWO REASONS. The manifest already contained a
		// .version = 92 (2026_08_19_aotv4_new_heal_descriptions), so this arrived as a DUPLICATE
		// version number; and 92 is below the 103 the manifest had already reached, so world -- which
		// only applies entries ABOVE db_version -- would never have run it on any server, live
		// included. It was dead on arrival twice over.
		// 📌 Safe to renumber: the check tests `aa_ranks.cost = 3` and the SQL sets those costs to 0,
		// so it is self-disabling and cannot double-apply wherever it may already have run.
		// Fix a bug where tradeskill levels make your exp significantly worse
		.check       = "SELECT cost FROM aa_ranks WHERE id = 979",
		.condition   = "match",
		.match       = "3",
		.sql         = R"(
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (83, -1, -1, 83, 83, 0, 55, -1, 0, 0, 3, -1, 84);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (84, -1, -1, 83, 83, 0, 55, -1, 0, 0, 3, 83, 85);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (85, -1, -1, 83, 83, 0, 55, -1, 0, 0, 3, 84, 13099);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (150, -1, -1, 150, 150, 0, 59, -1, 0, 0, 3, -1, 151);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (151, -1, -1, 150, 150, 0, 59, -1, 0, 0, 3, 150, 152);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (152, -1, -1, 150, 150, 0, 59, -1, 0, 0, 3, 151, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (159, -1, -1, 159, 159, 0, 59, -1, 0, 0, 3, -1, 160);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (160, -1, -1, 159, 159, 0, 59, -1, 0, 0, 3, 159, 161);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (161, -1, -1, 159, 159, 0, 59, -1, 0, 0, 3, 160, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (979, -1, -1, 979, 979, 0, 59, -1, 0, 0, 8, -1, 980);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (980, -1, -1, 979, 979, 0, 59, -1, 0, 0, 8, 979, 981);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (981, -1, -1, 979, 979, 0, 59, -1, 0, 0, 8, 980, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (982, -1, -1, 982, 982, 0, 59, -1, 0, 0, 8, -1, 983);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (983, -1, -1, 982, 982, 0, 59, -1, 0, 0, 8, 982, 984);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (984, -1, -1, 982, 982, 0, 59, -1, 0, 0, 8, 983, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (985, -1, -1, 985, 985, 0, 59, -1, 0, 0, 8, -1, 986);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (986, -1, -1, 985, 985, 0, 59, -1, 0, 0, 8, 985, 987);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (987, -1, -1, 985, 985, 0, 59, -1, 0, 0, 8, 986, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (988, -1, -1, 988, 988, 0, 59, -1, 0, 0, 8, -1, 989);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (989, -1, -1, 988, 988, 0, 59, -1, 0, 0, 8, 988, 990);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (990, -1, -1, 988, 988, 0, 59, -1, 0, 0, 8, 989, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (991, -1, -1, 991, 991, 0, 59, -1, 0, 0, 8, -1, 992);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (992, -1, -1, 991, 991, 0, 59, -1, 0, 0, 8, 991, 993);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (993, -1, -1, 991, 991, 0, 59, -1, 0, 0, 8, 992, -1);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (994, -1, -1, 994, 994, 0, 59, -1, 0, 0, 8, -1, 995);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (995, -1, -1, 994, 994, 0, 59, -1, 0, 0, 8, 994, 996);
REPLACE INTO `aa_ranks` (`id`, `upper_hotkey_sid`, `lower_hotkey_sid`, `title_sid`, `desc_sid`, `cost`, `level_req`, `spell`, `spell_type`, `recast_time`, `expansion`, `prev_id`, `next_id`) VALUES (996, -1, -1, 994, 994, 0, 59, -1, 0, 0, 8, 995, -1);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 126,
		.description = "2026_08_22_disable_aa_exp_slowdown",
		// Submitted by: Claude
		// Turns off the AA experience slowdown. Owner decision.
		.check       = "SELECT `rule_value` FROM `rule_values` WHERE `rule_name` = 'AoT:AAExpSlowdownEnabled'",
		.condition   = "match",
		.match       = "true",
		.sql         = R"(
UPDATE rule_values SET rule_value = 'false' WHERE rule_name = 'AoT:AAExpSlowdownEnabled';

-- 📌 The Base and Factor rows are left alone on purpose: they are inert while the switch is off, and
-- keeping them preserves the tuning if it is ever turned back on.
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 127,
		.description = "2026_08_22_no_min_ranged_distance",
		// Submitted by: Claude
		// WIDER THAN AoT:BowMinRangeIsMeleeRange -- this also reaches NPC archers, bots and THROWING.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `rule_values` WHERE `rule_name` = 'Combat:MinRangedAttackDist' AND `rule_value` <> '0'",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE rule_values SET rule_value = '0' WHERE rule_name = 'Combat:MinRangedAttackDist';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 128,
		.description = "2026_08_22_class_ability_endur_upkeep",
		// Submitted by: Claude
		// The 16 duration class abilities inherited EndurUpkeep 10 from the buff template and drained the whole bar.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE `id` BETWEEN 44700 AND 44755 AND `EndurUpkeep` <> 0",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET EndurUpkeep = 0 WHERE id BETWEEN 44700 AND 44755;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 129,
		.description = "2026_08_22_hot_uses_real_hot_spa",
		// Submitted by: Claude
		// Seven heal-over-time spells were built on SPA 0, which is a silent REGEN BONUS, not a heal.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE `id` IN (44542,44543,44544,44545,44546,44547,44716) AND `effectid1` <> 100",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET effectid1 = 100
 WHERE id IN (44542, 44543, 44544, 44545, 44546, 44547, 44716);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 130,
		.description = "2026_08_22_show_heal_ticks",
		// Submitted by: Claude
		// 100 is a live EQ number and hides every heal in the game at a level 30 cap.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `rule_values` WHERE `rule_name` = 'Spells:HealAmountMessageFilterThreshold' AND `rule_value` <> '12'",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE rule_values SET rule_value = '12' WHERE rule_name = 'Spells:HealAmountMessageFilterThreshold';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 131,
		.description = "2026_08_22_void_stance_charges",
		// Submitted by: Claude
		// Void Stance spent a charge on every incoming SWING, including the misses it was causing.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE `id` = 44719 AND `numhitstype` <> 6",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET numhitstype = 6 WHERE id = 44719;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 132,
		.description = "2026_08_22_shaman_rune_spam",
		// Submitted by: Claude
		// A rune whose recast is shorter than its duration is not a rune, it is immunity.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE `id` IN (44727,44728) AND (`formula1` <> 100 OR (`id` = 44727 AND `buffduration` <> 1))",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET formula1 = 100, max1 = 0, buffduration = 1  WHERE id = 44727;
UPDATE spells_new SET formula1 = 100, max1 = 0                    WHERE id = 44728;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 133,
		.description = "2026_08_22_shaman_rune_scale",
		// Submitted by: Claude
		// v132 made the runes flat, which fixed the level 30 end and left level 1 as total immunity.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE (`id` = 44727 AND (`effect_base_value1` <> 3 OR `formula1` <> 2)) OR (`id` = 44728 AND (`effect_base_value1` <> 10 OR `formula1` <> 4))",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET effect_base_value1 = 3,  formula1 = 2 WHERE id = 44727;
UPDATE spells_new SET effect_base_value1 = 10, formula1 = 4 WHERE id = 44728;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 134,
		.description = "2026_08_22_class_ability_descriptions",
		// Submitted by: Claude
		// Every class ability description rewritten against what the row and the Lua payload actually do.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44747 AND `type` = 6",
		.condition   = "missing",
		.match       = "up to 18",
		.sql         = R"(
DELETE FROM db_str WHERE id BETWEEN 44700 AND 44747 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44700, 6, 'A full weapon swing that also seizes attention, adding threat equal to 6 times your level.'),
 (44701, 6, 'Brace behind your guard for 60 seconds. The next 5 melee hits against you are each reduced by a tenth of your armor class, and any blow smaller than that is stopped outright.'),
 (44702, 6, 'A full weapon swing against every enemy within 30 feet in front of you that is already fighting you. Adds no threat of its own. Each target struck cuts Bulwark by 3 seconds, up to 9.'),
 (44703, 6, 'A full weapon swing that heals you for twice your level.'),
 (44704, 6, 'Heals your whole group for a sixth of their own maximum health, then the same again when it fades 18 seconds later.'),
 (44705, 6, 'A blast of judgement dealing 20 plus 4 per level, to a maximum of 250. Cuts Sanctuary by 5 seconds, or by 10 against the undead.'),
 (44706, 6, 'A weapon swing at four fifths damage, plus divine damage equal to your level or a tenth of your Strength, whichever is greater. That part ignores armor. Adds threat equal to 8 times your level.'),
 (44707, 6, 'Heals every group member in your zone for a quarter of YOUR maximum health. Alone, it heals you.'),
 (44708, 6, 'A full weapon swing that stuns your target, plus threat equal to 6 times your level. Cuts Hand of Conviction by 5 seconds.'),
 (44709, 6, 'Two weapon swings at three fifths damage each.'),
 (44710, 6, 'An archery attack at four fifths damage against every enemy within 60 feet in front of you that is already fighting you.'),
 (44711, 6, 'Fires your bow at a target in melee range and cuts Volley by 10 seconds. With no bow you drive the blow home instead, at one and a fifth weapon damage, and cut it by 5.'),
 (44712, 6, 'A full weapon swing that drains twice your level in health. Doubled while Reaving Vow is up, which spends it.'),
 (44713, 6, 'A weapon swing at seven tenths damage against every enemy within 40 feet that is already fighting you, draining twice your level in health for each one struck.'),
 (44714, 6, 'A full weapon swing that swears your next Reaving Strike to drain double, for 18 seconds. Cuts Harrowing by 6 seconds.'),
 (44715, 6, 'A full weapon swing that wreathes your target in thorns for 30 seconds. Every melee blow it lands wounds it for 8.'),
 (44716, 6, 'Mends the target for 12 plus your level every tick, for 10 ticks.'),
 (44717, 6, 'A lance of light dealing 25 plus 4 per level, to a maximum of 300. Against a target that is rooted, snared, mezzed or stunned it deals a further 6 times your level and cuts Wildgrowth by 10 seconds instead of 5.'),
 (44718, 6, 'A hand to hand strike for your weapon delay times 0.2, plus 0.035 more for each level. A slower weapon hits harder.'),
 (44719, 6, 'Raises your avoidance by half until 4 melee blows land on you, or 60 seconds, whichever comes first.'),
 (44720, 6, 'A weapon swing at one and three fifths damage, plus threat equal to 4 times your level. Cuts Void Stance by 6 seconds.'),
 (44721, 6, 'A full weapon swing that strips 25 armor from your target for 30 seconds.'),
 (44722, 6, 'Restores 100 plus 3 per level endurance to your whole group.'),
 (44723, 6, 'A weapon swing at one and a fifth damage, or one and a half with an instrument in your range slot. The next 5 blows landed on your target hurt it a tenth more. Cuts Crescendo by 5 seconds.'),
 (44724, 6, 'A full weapon swing that slows your target to under two thirds speed for 24 seconds.'),
 (44725, 6, 'Opens a bleed for 18 plus your level every tick, for 8 ticks.'),
 (44726, 6, 'A full weapon swing, half again as hard if Rupture is already bleeding on the target. Cuts Rupture by 5 seconds.'),
 (44727, 6, 'Wards the target against 3 plus twice your level of damage for 6 seconds. Anything that strikes the ward is slowed for 2 ticks.'),
 (44728, 6, 'Wards your whole group against 10 plus 4 per level of damage each, for 60 seconds. Anything that strikes a ward is slowed harder, for 2 ticks.'),
 (44729, 6, 'Saps 25 attack and 20 each of Strength, Intelligence and Charisma from your target for 36 seconds. Cuts Crippling Spirit by 5 seconds, or by 10 against something already slowed.'),
 (44730, 6, 'A full weapon swing that leaves rot behind, dealing 10 plus your level every tick for 6 ticks.'),
 (44731, 6, 'Spends every affliction on your target at once. They deal all of their remaining damage immediately and are consumed, and you are healed for half of that.'),
 (44732, 6, 'Deals a twentieth of the damage your target afflictions still have left to give, or a quarter of it if that total would already kill them. Cuts Soul Harvest by 15 seconds.'),
 (44733, 6, 'A weapon swing at seven tenths damage that returns twice your level in mana.'),
 (44734, 6, 'Pours everything into one cast for 40 plus 20 per level, to a maximum of 900, then stuns you for 2 seconds. Each gathered ley thread adds a further 3 times your level.'),
 (44735, 6, 'A bolt for 15 plus 3 per level, to a maximum of 200, that gathers a ley thread. Threads stack to 3 and are spent by your next Overload. Cuts Overload by 5 seconds.'),
 (44736, 6, 'A full weapon swing. If you have no pet, an elemental guardian answers, and it is rescaled to your level as you grow.'),
 (44737, 6, 'Calls a brief host of servants to fight beside you.'),
 (44738, 6, 'A burst of cinders for 30 plus 3 per level, to a maximum of 250. If your pet is already fighting that target it deals a further 3 times your level. Cuts Elemental Swarm by 7 seconds.'),
 (44739, 6, 'Thins every magical ward your target has, lowering all resists by 15 for 48 seconds. Physical resistance is unaffected.'),
 (44740, 6, 'Restores 100 plus 3 per level mana to your whole group.'),
 (44741, 6, 'Your spells land for 30 more damage for 3 ticks. Cuts Gift of Thought by 5 seconds.'),
 (44742, 6, 'A full weapon swing. If you have no pet, a feral companion answers, and it is rescaled to your level as you grow.'),
 (44743, 6, 'You and your companion attack a quarter faster and mend 10 plus your level every tick, for 60 seconds.'),
 (44744, 6, 'A full weapon swing, or four fifths harder against a target below half health. Cuts Feral Frenzy by 5 seconds, or by 10 on wounded prey.'),
 (44745, 6, 'A weapon swing at one and two fifths damage that costs you a hundredth of your maximum health.'),
 (44746, 6, 'Five weapon swings at half damage each.'),
 (44747, 6, 'A full weapon swing. Cuts Frenzied Onslaught by 2 seconds for every tenth of your own health that is missing, up to 18.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 135,
		.description = "2026_08_22_ley_thread_buffs",
		// Submitted by: Claude
		// Ley Tap stacks were an entity variable, so nothing showed on the buff bar.
		.check       = "SELECT `value` FROM `db_str` WHERE `id` = 44758 AND `type` = 6",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id IN (44756, 44757, 44758);

DROP TEMPORARY TABLE IF EXISTS aotv4_ley_tmpl;
CREATE TEMPORARY TABLE aotv4_ley_tmpl LIKE spells_new;
INSERT INTO aotv4_ley_tmpl SELECT * FROM spells_new WHERE id = 44753;
UPDATE aotv4_ley_tmpl SET
    id = 44756, name = 'One Ley Thread', descnum = 44756,
    new_icon = 164, spellanim = 0, CastingAnim = 44, player_1 = 'PLAYER_1',
    IsDiscipline = -1, EndurCost = 0, EndurUpkeep = 0, EndurTimerIndex = 0, mana = 0,
    targettype = 6, goodEffect = 1, buffduration = 10, buffdurationformula = 11,
    numhits = 0, numhitstype = 0, recast_time = 0, recovery_time = 0, cast_time = 0,
    effectid1 = 254, effect_base_value1 = 0, formula1 = 100, max1 = 0;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
UPDATE aotv4_ley_tmpl SET id = 44757, name = 'Two Ley Threads',   descnum = 44757;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
UPDATE aotv4_ley_tmpl SET id = 44758, name = 'Three Ley Threads', descnum = 44758;
INSERT INTO spells_new SELECT * FROM aotv4_ley_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_ley_tmpl;

-- ⚠️ A spell with no db_str type 6 row renders BLANK with no error (section 51). Written here, in the
-- same migration as the rows.
DELETE FROM db_str WHERE id IN (44756, 44757, 44758) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44756, 6, 'One ley thread gathered. Your next Overload deals 3 times your level in extra damage.'),
 (44757, 6, 'Two ley threads gathered. Your next Overload deals 6 times your level in extra damage.'),
 (44758, 6, 'Three ley threads gathered. Your next Overload deals 9 times your level in extra damage.');
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 136,
		.description = "2026_08_22_reptile_charges",
		// Submitted by: Claude
		// The Skin of the Reptile line healed on 72 incoming hits. Cut to 5.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spells_new` WHERE `id` BETWEEN 43300 AND 43305 AND `numhits` <> 5",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE spells_new SET numhits = 5 WHERE id BETWEEN 43300 AND 43305;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 137,
		.description = "2026_08_22_gilded_wager_npc",
		// Submitted by: Claude
		// The Gilded Wager -- a fourth Resplendent hub NPC that sells a random wearable for coin.
		.check       = "SELECT `npcID` FROM `spawnentry` WHERE `npcID` = 2000403",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;
DELETE FROM npc_types  WHERE id = 2000403;

DROP TEMPORARY TABLE IF EXISTS aotv4_npc_tmpl;
CREATE TEMPORARY TABLE aotv4_npc_tmpl LIKE npc_types;
INSERT INTO aotv4_npc_tmpl SELECT * FROM npc_types WHERE id = 2000401;
UPDATE aotv4_npc_tmpl SET
    id = 2000403,
    name = '#Gilded_Wager',
    lastname = 'Keeper of the Wheel',
    class = 1, bodytype = 1, gender = 0, race = 12, texture = 1, size = 4,
    merchant_id = 0, npc_faction_id = 0, runspeed = 0;
INSERT INTO npc_types SELECT * FROM aotv4_npc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_npc_tmpl;

INSERT INTO spawngroup (id, name) VALUES (2000403, 'aotv4_gilded_wager');
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, min_expansion, max_expansion)
VALUES (2000403, 'resplendent', 0, -12.0, 692.0, -24.8, 256, 300, 0, 0, -1, -1);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (2000403, 2000403, 100);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 138,
		.description = "2026_08_22_gilded_wager_unplace",
		// Submitted by: Claude
		// The Gilded Wager keeps its npc_types row but is no longer spawned. It gets placed by hand.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spawnentry` WHERE `npcID` = 2000403",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 139,
		.description = "2026_08_22_gilded_wager_place_theater",
		// Submitted by: Claude
		// Places the Gilded Wager in the Theater of the Tranquil at the owner-supplied location.
		.check       = "SELECT `npcID` FROM `spawnentry` WHERE `npcID` = 2000403",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;

INSERT INTO spawngroup (id, name) VALUES (2000403, 'aotv4_gilded_wager');
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, min_expansion, max_expansion)
VALUES (2000403, 'freeporttheater', 0, -68.98, -15.70, -27.10, 256, 300, 0, 0, -1, -1);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (2000403, 2000403, 100);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 140,
		.description = "2026_08_22_gilded_wager_lizard",
		// Submitted by: Claude
		// Gilded Wager becomes a Lizard Man at size 10, and the duplicate hand-placed spawn is removed.
		.check       = "SELECT IF((SELECT COUNT(*) FROM `npc_types` WHERE `id` = 2000403 AND (`race` <> 51 OR `gender` <> 2 OR `size` <> 10)) + (SELECT COUNT(*) FROM `spawnentry` WHERE `npcID` = 2000403 AND `spawngroupID` <> 2000403) > 0, 'pending', 'done')",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
DELETE se, s2 FROM spawnentry se
  LEFT JOIN spawn2 s2 ON s2.spawngroupID = se.spawngroupID
  WHERE se.npcID = 2000403 AND se.spawngroupID <> 2000403;

UPDATE npc_types SET race = 51, gender = 2, size = 10, texture = 0 WHERE id = 2000403;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 141,
		.description = "2026_08_22_player_corpses_off",
		// Submitted by: Claude
		// Scoped by rule_name ALONE, deliberately: rule_values has one row per ruleset and fixing only ruleset 1 leaves the others behind, which then apply to whoever is on that ruleset (same trap as MinRangedAttackDist / SpecialEndurancePct).
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `rule_values` WHERE (`rule_name` = 'Character:LeaveCorpses' AND `rule_value` <> 'false') OR (`rule_name` = 'Character:LeaveNakedCorpses' AND `rule_value` <> 'false') OR (`rule_name` = 'Character:CorpseDecayTime' AND `rule_value` <> '60000') OR (`rule_name` = 'Character:EmptyCorpseDecayTime' AND `rule_value` <> '60000')",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE `rule_values` SET `rule_value` = 'false' WHERE `rule_name` = 'Character:LeaveCorpses';
UPDATE `rule_values` SET `rule_value` = 'false' WHERE `rule_name` = 'Character:LeaveNakedCorpses';
UPDATE `rule_values` SET `rule_value` = '60000' WHERE `rule_name` = 'Character:CorpseDecayTime';
UPDATE `rule_values` SET `rule_value` = '60000' WHERE `rule_name` = 'Character:EmptyCorpseDecayTime';
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 142,
		.description = "2026_08_22_start_zone_freeporttheater",
		// Submitted by: Claude
		// The check keys on start_zones because a full DB import resets it (to stock home cities / 729); when it does, this re-applies. SoFStartZoneID needs a WORLD RESTART to take effect; start_zones is read live per character creation.
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `start_zones` WHERE `zone_id` <> 390",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE `rule_values` SET `rule_value` = '390' WHERE `rule_name` = 'World:SoFStartZoneID';
UPDATE `start_zones` SET `zone_id` = 390, `start_zone` = 390, `x` = -83, `y` = -235, `z` = -27, `heading` = 0, `bind_id` = 390, `bind_x` = -83, `bind_y` = -235, `bind_z` = -27 WHERE `zone_id` <> 390;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 143,
		.description = "2026_08_23_open_region_click_proc_level_25",
		// Submitted by: Claude
		// items is shared memory -- applying needs ./shared_memory + a restart (CLAUDE.md section 57).
		.check       = "SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `items` WHERE (`id` = 4519 AND `clicklevel2` > 25) OR (`id` = 1154 AND `proclevel2` > 25)",
		.condition   = "match",
		.match       = "pending",
		.sql         = R"(
UPDATE `items` SET `clicklevel2` = 25
WHERE `clickeffect` > 0 AND `clicklevel2` > 25 AND `id` < 900000
  AND (`id` MOD 300000) IN (
    SELECT iid FROM (
      SELECT DISTINCT lde.item_id AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN loottable_entries lte ON lte.loottable_id = n.loottable_id
        JOIN lootdrop_entries  lde ON lde.lootdrop_id  = lte.lootdrop_id
       WHERE zr.region_id BETWEEN 1 AND 6
      UNION
      SELECT DISTINCT ml.item AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN merchantlist ml ON ml.merchantid = n.merchant_id
       WHERE zr.region_id BETWEEN 1 AND 6
    ) open_region_items
  );

UPDATE `items` SET `proclevel2` = 25
WHERE `proceffect` > 0 AND `proclevel2` > 25 AND `id` < 900000
  AND (`id` MOD 300000) IN (
    SELECT iid FROM (
      SELECT DISTINCT lde.item_id AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN loottable_entries lte ON lte.loottable_id = n.loottable_id
        JOIN lootdrop_entries  lde ON lde.lootdrop_id  = lte.lootdrop_id
       WHERE zr.region_id BETWEEN 1 AND 6
      UNION
      SELECT DISTINCT ml.item AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN merchantlist ml ON ml.merchantid = n.merchant_id
       WHERE zr.region_id BETWEEN 1 AND 6
    ) open_region_items
  );
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 144,
		.description = "2026_08_30_bloodletting_rank_rows",
		// Submitted by: Twochords
		// Reported from live -- Bloodletting does nothing above rank 1. The AA is fine and the C++
		.check       = "SELECT id FROM spells_new WHERE id = 43404",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id BETWEEN 43400 AND 43404;

-- Clone through a temp table so all ~236 columns stay byte-identical to a working stock row.
-- Never hand-list the columns: a missed one is a silently malformed spell.
DROP TEMPORARY TABLE IF EXISTS aotv4_bleed_tmpl;
CREATE TEMPORARY TABLE aotv4_bleed_tmpl LIKE spells_new;

-- Template 4088 Ward of Vie: single target, buff, sane duration handling. Overridden to a
-- detrimental damage-over-time. goodEffect 0 and a NEGATIVE base is what makes SPA 0 damage.
-- ⚠️ formula1 stays as cloned. A level-scaled formula would key off the CASTER level, which is what
-- is wanted here, but the authored row leaves it alone and this migration changes no tuning.
INSERT INTO aotv4_bleed_tmpl SELECT * FROM spells_new WHERE id = 4088;
UPDATE aotv4_bleed_tmpl SET
  id = 43400, name = 'Bloodletting', descnum = 43400, spellgroup = 43400, `rank` = 1, mana = 0,
  effectid1 = 0, effect_base_value1 = -4, effect_limit_value1 = 0, max1 = 0,
  effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, max2 = 0,
  buffduration = 3, buffdurationformula = 10,
  targettype = 5, goodEffect = 0, new_icon = 68, spell_category = -99,
  classes1=255, classes2=255, classes3=255, classes4=255, classes5=255, classes6=255,
  classes7=255, classes8=255, classes9=255, classes10=255, classes11=255, classes12=255,
  classes13=255, classes14=255, classes15=255, classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_bleed_tmpl;
DELETE FROM aotv4_bleed_tmpl;

-- ⚠️ ONE ROW PER RANK, not one row scaled by rank. EQ will not stack the same spell id from one
-- caster, so the bleed refreshes rather than stacks and per-rank damage has to live in its own row.
INSERT INTO aotv4_bleed_tmpl SELECT * FROM spells_new WHERE id = 43400;
UPDATE aotv4_bleed_tmpl SET id = 43401, descnum = 43401, spellgroup = 43401, `rank` = 2,
  effect_base_value1 = -7, buffduration = 3;
INSERT INTO spells_new SELECT * FROM aotv4_bleed_tmpl;
DELETE FROM aotv4_bleed_tmpl;

INSERT INTO aotv4_bleed_tmpl SELECT * FROM spells_new WHERE id = 43400;
UPDATE aotv4_bleed_tmpl SET id = 43402, descnum = 43402, spellgroup = 43402, `rank` = 3,
  effect_base_value1 = -7, buffduration = 5;
INSERT INTO spells_new SELECT * FROM aotv4_bleed_tmpl;
DELETE FROM aotv4_bleed_tmpl;

INSERT INTO aotv4_bleed_tmpl SELECT * FROM spells_new WHERE id = 43400;
UPDATE aotv4_bleed_tmpl SET id = 43403, descnum = 43403, spellgroup = 43403, `rank` = 4,
  effect_base_value1 = -11, buffduration = 5;
INSERT INTO spells_new SELECT * FROM aotv4_bleed_tmpl;
DELETE FROM aotv4_bleed_tmpl;

-- ⚠️ Rank 5 was specified as "ticks twice as fast" and cannot be. Buff tics run on a fixed 6 second
-- timer, so tick RATE is not something a spell row can express; twice the damage per tick is the
-- same damage per second and is what ships.
INSERT INTO aotv4_bleed_tmpl SELECT * FROM spells_new WHERE id = 43400;
UPDATE aotv4_bleed_tmpl SET id = 43404, descnum = 43404, spellgroup = 43404, `rank` = 5,
  effect_base_value1 = -22, buffduration = 5;
INSERT INTO spells_new SELECT * FROM aotv4_bleed_tmpl;

DROP TEMPORARY TABLE IF EXISTS aotv4_bleed_tmpl;
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version     = 145,
		.description = "2026_08_30_borrowed_breath_rank_rows",
		// Submitted by: Twochords
		// The same hole as the Bloodletting one, found while confirming it, and NOT reported by
		.check       = "SELECT id FROM spells_new WHERE id = 43395",
		.condition   = "empty",
		.match       = "",
		.sql         = R"(
DELETE FROM spells_new WHERE id BETWEEN 43391 AND 43395;

DROP TEMPORARY TABLE IF EXISTS aotv4_breath_tmpl;
CREATE TEMPORARY TABLE aotv4_breath_tmpl LIKE spells_new;

-- Template 4088 Ward of Vie. SPA 162 with base 100 and limit N subtracts exactly N per hit -- a
-- FLAT per-hit cap, not a percentage. Percentages are imperceptible against the small
-- post-mitigation numbers this server produces, which is why the whole tree is flat.
INSERT INTO aotv4_breath_tmpl SELECT * FROM spells_new WHERE id = 4088;
UPDATE aotv4_breath_tmpl SET
  id = 43391, name = 'Borrowed Breath', descnum = 43391, spellgroup = 43391, `rank` = 1, mana = 0,
  effectid1 = 162, effect_base_value1 = 100, effect_limit_value1 = 3, max1 = 0,
  effectid2 = 254, effect_base_value2 = 0, effect_limit_value2 = 0, max2 = 0,
  buffduration = 1, buffdurationformula = 10,
  targettype = 5, goodEffect = 1, new_icon = 128, spell_category = -99,
  classes1=255, classes2=255, classes3=255, classes4=255, classes5=255, classes6=255,
  classes7=255, classes8=255, classes9=255, classes10=255, classes11=255, classes12=255,
  classes13=255, classes14=255, classes15=255, classes16=255;
INSERT INTO spells_new SELECT * FROM aotv4_breath_tmpl;
DELETE FROM aotv4_breath_tmpl;

INSERT INTO aotv4_breath_tmpl SELECT * FROM spells_new WHERE id = 43391;
UPDATE aotv4_breath_tmpl SET id = 43392, descnum = 43392, spellgroup = 43392, `rank` = 2,
  effect_limit_value1 = 5, buffduration = 1;
INSERT INTO spells_new SELECT * FROM aotv4_breath_tmpl;
DELETE FROM aotv4_breath_tmpl;

INSERT INTO aotv4_breath_tmpl SELECT * FROM spells_new WHERE id = 43391;
UPDATE aotv4_breath_tmpl SET id = 43393, descnum = 43393, spellgroup = 43393, `rank` = 3,
  effect_limit_value1 = 6, buffduration = 2;
INSERT INTO spells_new SELECT * FROM aotv4_breath_tmpl;
DELETE FROM aotv4_breath_tmpl;

INSERT INTO aotv4_breath_tmpl SELECT * FROM spells_new WHERE id = 43391;
UPDATE aotv4_breath_tmpl SET id = 43394, descnum = 43394, spellgroup = 43394, `rank` = 4,
  effect_limit_value1 = 8, buffduration = 2;
INSERT INTO spells_new SELECT * FROM aotv4_breath_tmpl;
DELETE FROM aotv4_breath_tmpl;

-- ⚠️ Rank 5 is mitigation only here. The death save that goes with it is NOT a spell effect: it is
-- Mob::AoTv4TryBorrowedBreath. It briefly carried SPA 232 DivineSave, which was wrong -- TryDivineSave
-- always casts 4789 Touch of the Divine on top of the save, and that blocks every incoming spell from
-- every other caster as well as your own attacks. It keeps you alive by removing you from the fight.
INSERT INTO aotv4_breath_tmpl SELECT * FROM spells_new WHERE id = 43391;
UPDATE aotv4_breath_tmpl SET id = 43395, descnum = 43395, spellgroup = 43395, `rank` = 5,
  effect_limit_value1 = 10, buffduration = 2;
INSERT INTO spells_new SELECT * FROM aotv4_breath_tmpl;

DROP TEMPORARY TABLE IF EXISTS aotv4_breath_tmpl;
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
//
// };
