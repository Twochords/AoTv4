-- ============================================================================================
-- AoTv4 -- the 16 class auras cost NOTHING to cast (2026-08-01)
--
-- The class auras (CLAUDE.md section 15) are an ACHIEVEMENT REWARD, one per class, granted by
-- "<Class>: First Blood" and scribed by the achievement system's scribe_spell reward type. They
-- are the class's identity buff, not a resource to budget, and they were charging 100 mana AND
-- 200 endurance on every cast.
--
-- ============================================================================================
-- WHY BOTH COLUMNS, AND WHY THAT IS NOT OPTIONAL
-- ============================================================================================
-- These were cloned from stock 8468 "Champion's Aura", which carries IsDiscipline = -1. That is
-- read into the engine as a plain bool (common/shareddb.cpp:1780 -> Strings::ToBool("-1"), which
-- is a non-zero number and therefore TRUE), so the engine treats them as DISCIPLINES: the cost
-- actually charged is EndurCost, and the 100 mana was almost certainly never spent at all.
--
-- Clearing only the obvious "mana" column would therefore have looked correct, changed nothing a
-- player could feel, and left the real 200 endurance cost in place. Both go.
--
-- Note the clone inherited BOTH costs, which no stock aura has -- every native aura carries mana
-- OR endurance, never the two together (8468 endur 200 / mana 0; 8481 mana 400 / endur 0). The
-- 100 mana was an artefact of the clone, not a design decision.
--
-- ============================================================================================
-- WHY A ZERO-COST DISCIPLINE STILL FIRES (checked, not assumed)
-- ============================================================================================
-- Both engine sites degrade correctly at 0, so there is no guard that refuses a free discipline:
--   zone/effects.cpp:934   if (GetEndurance() < spell.endurance_cost)   -- "< 0" is never true
--   zone/spells.cpp:2783   if (spells[id].endurance_cost && !isproc)    -- skipped entirely at 0
--
-- ============================================================================================
-- SCOPE
-- ============================================================================================
-- OUR 16 ONLY (43500-43515). The ~110 stock aura spells keep their costs: they are native
-- content, AA-granted, and out of reach at the current level cap. Do not widen this to every
-- SPA 351 spell -- that would silently rebalance content nobody has asked about.
--
-- The aura EFFECT spells (43550-43565, the group buff the aura NPC applies) already cost 0 and
-- are not touched; they are never cast by a player.
--
-- ============================================================================================
-- APPLYING
-- ============================================================================================
-- spells_new IS SHARED MEMORY. Editing the table alone changes nothing in game:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- Do it with world DOWN, or world keeps the stale mmap (CLAUDE.md section 10).
-- ============================================================================================

UPDATE spells_new
SET mana        = 0,
    EndurCost   = 0,
    EndurUpkeep = 0          -- already 0; set for idempotence if a future clone inherits one
WHERE id BETWEEN 43500 AND 43515;

-- Verification: expect 16 rows, all three columns 0.
SELECT id, name, mana, EndurCost, EndurUpkeep
FROM spells_new
WHERE id BETWEEN 43500 AND 43515
ORDER BY id;
