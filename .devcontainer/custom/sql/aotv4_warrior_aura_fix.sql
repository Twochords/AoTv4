-- ============================================================================================
-- AoTv4 -- Warlords Aura (Warrior) mitigation made PROPORTIONAL (2026-08-01)
--
-- Effect spell 43550, the group buff placed by aura npc 2000100 (CLAUDE.md section 15).
--
-- ============================================================================================
-- WHAT WAS WRONG: A FLAT ABSORB IS ENORMOUS AT THE LEVELS PEOPLE ACTUALLY PLAY
-- ============================================================================================
-- It shipped as SPA 162 MitigateMeleeDamage with base 100, limit 1, which the engine reads as:
--
--   damage_to_reduce = damage * base / 100                     -- 100 percent
--   if (limit && damage_to_reduce > limit) damage_to_reduce = limit;   -- ...capped at 1
--   damage -= damage_to_reduce;                                (zone/attack.cpp:3993-4012)
--
-- i.e. subtract exactly ONE damage from every melee hit, forever, for the whole group. That is the
-- flat-per-hit trick section 14 recommends -- and section 14 recommends it for a REASON that cuts
-- both ways: percentage mitigation is invisible against small numbers (3 * 1/100 rounds to 0), so a
-- flat amount was chosen instead. But the same smallness makes a flat 1 enormous: against the 1-6
-- damage hits of the early game that is 20-100 percent mitigation, and it stacks on top of the
-- custom AC-vs-offense roll in Mob::MeleeMitigation which already zeroes a lot of hits outright
-- (the "deflected by your armor" spam, section 14). Reported from play as the aura "reducing damage
-- by a lot" -- correctly.
--
-- ⚠️ It is also the only one of the sixteen class auras with a base above 5. Every other effect sits
-- at 2, 3, 5 or -2/-3 (see 43551-43565). That asymmetry is the tell.
--
-- ============================================================================================
-- THE FIX: A PERCENTAGE THAT SCALES WITH THE HIT, NOT A FLAT CHUNK OF IT
-- ============================================================================================
--   base  = 10   -> mitigate 10 percent of each melee hit
--   limit = 0    -> NO per-hit cap. 0 is falsy in the engine test above, so the cap is skipped.
--   max   = 0    -> NO total pool, so the buff never wears down or fades (it is an aura).
--
-- 10 percent is proportional at every level: it can never be worth more than a tenth of the hit,
-- where the flat version could be worth all of it. Early on it rounds to nothing, which is the
-- deliberate trade -- an aura that does little at level 3 is far better than one that makes a level
-- 3 warrior immune, and it grows into real value exactly as incoming damage grows.
--
-- 📌 If it should be felt earlier, raise the percentage rather than restoring a flat amount -- the
-- flat form has no way to stay proportional and will always be strongest when it should be weakest.
--
-- ============================================================================================
-- APPLYING -- ⚠️⚠️ `spells_new` IS SHARED MEMORY
-- ============================================================================================
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- Editing the table alone changes nothing in game (CLAUDE.md section 10).
-- ============================================================================================

UPDATE spells_new
SET effect_base_value1  = 10,
    effect_limit_value1 = 0,
    max1                = 0
WHERE id = 43550;

-- Verification: expect base 10, limit 0, max 0, formula 100 (static).
SELECT id, name, effectid1 AS spa, effect_base_value1 AS base,
       effect_limit_value1 AS lim, max1, formula1
FROM spells_new WHERE id = 43550;
