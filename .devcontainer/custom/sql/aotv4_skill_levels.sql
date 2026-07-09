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
