-- AoTv4: combat-skill availability by level (Bard, class_id 8).
--
-- The level a skill becomes usable is driven by its skill_caps curve: global_player.lua grant_free_skills
-- grants a passive skill (SetSkill 1) the moment its cap first exceeds 0 at the player's level, and the
-- server's proc checks (CheckDoubleAttack / CheckTripleAttack / CheckDualWield) key off the learned skill.
-- So zeroing the cap below a target level defers the skill to that level; caps at/above keep their scaling.
--
--   Dual Wield    (22): usable at level 17  -- NATIVE. RoF2 hard-codes bard dual-wield at 17 in the client
--                                              (won't show it or accept an off-hand weapon before then),
--                                              so the server matches: no early dual wield, no workaround.
--   Double Attack (20): usable at level 32  -- was 15
--   Triple Attack (76): usable at level 58  -- was 46
--
-- skill_caps loads from the DB at zone boot (SkillCaps::LoadSkillCaps -> SkillCapsRepository::All), so a
-- zone restart applies this -- no shared-memory rebuild. Re-export SkillCaps.txt for the client to match.

UPDATE skill_caps SET cap = 0 WHERE class_id = 8 AND skill_id = 22 AND level < 17;   -- Dual Wield -> 17 (native)
UPDATE skill_caps SET cap = 0 WHERE class_id = 8 AND skill_id = 20 AND level < 32;   -- Double Attack -> 32
UPDATE skill_caps SET cap = 0 WHERE class_id = 8 AND skill_id = 76 AND level < 58;   -- Triple Attack -> 58

-- AoTv4: tradeskills + gathering are META-progression -- kept through the roguelite death-reset to level 1
-- -- so their caps must be LEVEL-INDEPENDENT, else a level-1 (just-died) character is capped back down and
-- the skill effectively "resets on death". The crafting skills (56-69) are already flat 300 and safe. Only
-- Forage (27) and Fishing (55) scaled with level, so:
--   1) make sure both have cap rows down to level 1 (a fresh/just-died char sits at level 1), then
--   2) flatten each to its own max cap at EVERY level.
-- Forage (27) is also a default skill auto-granted at level 1 (added to FREE_SKILLS in global_player.lua);
-- both are skill-gated server-side (forage.cpp / fishing use GetSkill, no class check).
INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT ts.sid, 8, lv.level, 200, 8
FROM (SELECT 27 AS sid UNION SELECT 55 AS sid) ts
JOIN (SELECT 1 AS level UNION SELECT 2 AS level) lv
WHERE NOT EXISTS (SELECT 1 FROM skill_caps sc WHERE sc.class_id = 8 AND sc.skill_id = ts.sid AND sc.level = lv.level);

UPDATE skill_caps s
JOIN (SELECT skill_id, MAX(cap) mx FROM skill_caps WHERE class_id = 8 AND skill_id IN (27,55) GROUP BY skill_id) m
  ON m.skill_id = s.skill_id
SET s.cap = m.mx
WHERE s.class_id = 8 AND s.skill_id IN (27,55);

-- AoTv4: the race/class-locked tradeskills only START at their classic learn level -- Make Poison (56) @ L20,
-- Tinkering (57) @ L16, Alchemy (59) @ L25 -- so on this level-1-on-death server a level-1 Bard has MaxSkill 0:
-- the skill isn't granted (grant_free_skills needs MaxSkill>0), isn't usable, and the client hides it. Extend
-- each down to level 1 at its max cap (300) so it's available / granted / shown from level 1 like the other
-- Bard tradeskills (Blacksmithing etc. already start at L1). Server combine gates are already Bard-bypassed.
INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT ts.sid, 8, n.level, 300, 8
FROM (SELECT 56 AS sid UNION SELECT 57 UNION SELECT 59) ts
CROSS JOIN (
  SELECT a.N + b.N * 10 + 1 AS level
  FROM (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4
        UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) a
  CROSS JOIN (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2) b
) n
WHERE n.level <= 30
  AND NOT EXISTS (SELECT 1 FROM skill_caps sc WHERE sc.class_id = 8 AND sc.skill_id = ts.sid AND sc.level = n.level);
