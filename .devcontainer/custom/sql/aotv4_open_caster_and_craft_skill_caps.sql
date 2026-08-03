-- ============================================================================================
-- AoTv4 -- fill the skill_caps gaps that left casting, instrument and craft skills stuck at 0
--          (2026-08-02)
--
-- ============================================================================================
-- ⚠️⚠️ ONE ROOT CAUSE, THREE SYMPTOMS: A MISSING `skill_caps` ROW READS AS "CAP 0"
-- ============================================================================================
-- Reported from play as three separate bugs, all the same thing -- melee skills climbed on level up
-- while these sat at 0:
--   1. "the caster versions are not [updating]... channeling, alteration, conjuration"
--   2. "some tradeskills got left off the starting level of 20"
--   3. "instruments aren't being updated either"
--
-- global_player.max_skills_for_level raises every non-tradeskill to `MaxSkill(id)` for the current
-- level, and floor_tradeskills floors a craft at 20 -- but BOTH ask skill_caps for the answer, and
-- SkillCaps::GetSkillCap returns 0 for a (class, skill, level) with no row. Zero is indistinguishable
-- from "this class cannot have this skill", so both routines correctly did nothing.
--
-- ⚠️ The client made this look like a different bug. The dll's CSkillMgr::GetSkillCap detour
-- (CLAUDE.md section 4b) REVEALS skills a class has no native cap for, showing a level-scaled figure
-- -- so the skill window displayed "Abjuration 0/15" as though a cap of 15 existed and the value had
-- simply failed to rise. Server side there was no cap at all. Do not trust the window's denominator.
--
-- ============================================================================================
-- WHAT WAS ACTUALLY MISSING
-- ============================================================================================
--   CASTING     4 Abjuration, 5 Alteration, 13 Channeling, 14 Conjuration, 18 Divination,
--               24 Evocation, plus specializations 43-47
--               -> ABSENT ENTIRELY for the four pure-melee classes: 1 Warrior, 7 Monk (all but one),
--                  9 Rogue, 16 Berserker. Section 14 turned those classes into casters client side
--                  (core_allcasters) and server side (CalcMaxMana) and opened the spell pool to all
--                  16 -- but never gave them the SKILLS that make casting work. A Warrior casting
--                  with 0 Channeling is interrupted by everything.
--
--   INSTRUMENTS 12 Brass, 41 Singing, 49 Stringed, 54 Wind, 70 Percussion
--               -> present for every class, but only from LEVEL 10 UP for four of the five; only
--                  Singing had rows at levels 1-9. So they read 0 for the whole early game.
--               📌 CLAUDE.md section 7 lists these ids with the NAMES SWAPPED ("Singing 12,
--                  Percussion 41 ... Brass 70"). The ids are right, the labels are not: 12 is Brass
--                  and 70 is Percussion. Verified against common/skills.h.
--
--   CRAFT       56 Make Poison, 59 Alchemy
--               -> missing for 15 of 16 classes (only Rogue has 56, only Shaman 59), which is why
--                  those two alone never picked up the floor of 20 that every other craft did.
--
-- ============================================================================================
-- THE RULE APPLIED: no class is worse at these than the best class already is, at any level
-- ============================================================================================
-- For each (skill, level) the best cap that exists anywhere is copied to every class that has no row.
-- That is the right shape for THIS server rather than a general one: the reward pool is
-- class-agnostic (section 7 opened every spell to all 16 classes), so a spell that performs
-- differently by class would be an accident of skill_caps rather than a design.
--
-- ⚠️ Existing rows are NEVER touched -- the LEFT JOIN ... IS NULL only inserts what is absent, so a
-- class that already has a curve keeps it exactly.
-- ⚠️ `class_` is a legacy duplicate column and is 0 on every row in this table; it is set to 0 to
-- match rather than mirroring class_id.
-- ⚠️⚠️ Specialization skills 43-47 are included, but they carry a STOCK BALANCE RULE that nothing
-- here changes: Mob::GetMaxSkillAfterSpecializationRules resets ALL of them to 1 if more than one
-- rises above 50. Opening the cap does not open that gate.
--
-- ⚠️ skill_caps is read at zone boot, NOT shared memory -- a zone restart applies this.
-- ============================================================================================

INSERT INTO skill_caps (skill_id, class_id, level, cap, class_)
SELECT best.skill_id, c.class_id, best.level, best.cap, 0
FROM (SELECT DISTINCT class_id FROM skill_caps) c
CROSS JOIN (
    SELECT skill_id, level, MAX(cap) AS cap
    FROM skill_caps
    WHERE skill_id IN (
        4, 5, 13, 14, 18, 24,       -- casting
        43, 44, 45, 46, 47,         -- specializations
        12, 41, 49, 54, 70,         -- instruments
        56, 59                      -- Make Poison, Alchemy
    )
    GROUP BY skill_id, level
) best
LEFT JOIN skill_caps sc
       ON sc.class_id = c.class_id AND sc.skill_id = best.skill_id AND sc.level = best.level
WHERE sc.id IS NULL;

-- Verification: every class now has a non-zero cap for all 18 skills at level 1 and at level 30.
SELECT s.skill_id,
       COUNT(DISTINCT CASE WHEN sc1.cap > 0 THEN sc1.class_id END) AS classes_ok_at_lvl1,
       COUNT(DISTINCT CASE WHEN sc30.cap > 0 THEN sc30.class_id END) AS classes_ok_at_lvl30
FROM (SELECT 4 skill_id UNION SELECT 5 UNION SELECT 13 UNION SELECT 14 UNION SELECT 18 UNION SELECT 24
      UNION SELECT 43 UNION SELECT 44 UNION SELECT 45 UNION SELECT 46 UNION SELECT 47
      UNION SELECT 12 UNION SELECT 41 UNION SELECT 49 UNION SELECT 54 UNION SELECT 70
      UNION SELECT 56 UNION SELECT 59) s
LEFT JOIN skill_caps sc1  ON sc1.skill_id  = s.skill_id AND sc1.level  = 1
LEFT JOIN skill_caps sc30 ON sc30.skill_id = s.skill_id AND sc30.level = 30
GROUP BY s.skill_id ORDER BY s.skill_id;
