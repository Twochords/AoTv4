-- ============================================================================================
-- AoTv4 -- make the 16 class auras CASTABLE (2026-08-02)
--
-- ============================================================================================
-- THEY WERE FLAGGED AS DISCIPLINES, BUT AWARDED INTO THE SPELLBOOK
-- ============================================================================================
-- `spells_new.IsDiscipline` was -1 on all sixteen, inherited from the clone source (stock 8468
-- Champion's Aura, which really is an AA-granted discipline). The server reads that column through
-- Strings::ToBool (common/shareddb.cpp:1780) and "-1" is a non-zero number, so it is TRUE.
--
-- That flag is the switch between two mutually exclusive lists:
--     zone/client.cpp:10892   learnable DISCIPLINES  -> `if (!IsDiscipline(id)) continue;`
--     zone/client.cpp:10963   scribeable SPELLS      -> `if ( IsDiscipline(id)) continue;`
--
-- So the auras were disciplines -- they belong in the Combat Abilities window and are activated
-- through UseDiscipline -- while the achievement reward type `scribe_spell` (section 15) puts them
-- in the SPELLBOOK with ScribeSpell. A player therefore held an aura that could not be memorised and
-- could not be cast, in either window.
--
-- Setting it to 0 makes them ordinary self-buff spells, which is what everything else about them
-- already assumes: targettype 6 (Self), 3 second cast, class levels of 1 for all sixteen classes,
-- and no mana or endurance cost (custom/sql/aotv4_aura_no_cost.sql).
--
-- ⚠️ `EndurTimerIndex` is deliberately left at 10. It is the DISCIPLINE reuse-timer index and is
-- simply not consulted for a non-discipline, so clearing it changes nothing and would only widen the
-- diff. Do not "tidy" it without checking whether anything reads it as a generic timer id.
-- ⚠️ Only 43500-43515, the CAST spells. The effect spells 43550-43565 are applied by the aura npc
-- and are never cast by a player, so their flag is irrelevant.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

UPDATE spells_new SET IsDiscipline = 0 WHERE id BETWEEN 43500 AND 43515;

-- Verification: all 16 flagged as spells, still free to cast, still self-targeted.
SELECT COUNT(*) AS auras, SUM(IsDiscipline = 0) AS as_spells,
       SUM(mana = 0 AND EndurCost = 0) AS free, SUM(targettype = 6) AS self_target
FROM spells_new WHERE id BETWEEN 43500 AND 43515;
