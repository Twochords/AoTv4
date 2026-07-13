-- AoTv4: every character keeps each TRADESKILL floored at 20 (applied in global_player.lua
-- floor_tradeskills on connect -- new chars start at 20, existing chars are raised to 20 on login).
-- Research (58) had NO skill cap below level 16, so MaxSkill=0 there and the floor couldn't apply at
-- levels 1-15. Fill those with cap 300 (matching level 16+) for Bard (class 8). skill_caps load at
-- zone boot; the CLIENT also needs a re-exported SkillCaps.txt so it shows Research at low levels.
INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT 58, 8, lv.level, 300, 8
FROM (SELECT 1 level UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6
      UNION SELECT 7 UNION SELECT 8 UNION SELECT 9 UNION SELECT 10 UNION SELECT 11 UNION SELECT 12
      UNION SELECT 13 UNION SELECT 14 UNION SELECT 15) lv
WHERE NOT EXISTS (SELECT 1 FROM skill_caps s WHERE s.class_id=8 AND s.skill_id=58 AND s.level=lv.level);
