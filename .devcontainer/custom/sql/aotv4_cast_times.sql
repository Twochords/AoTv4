-- aotv4_cast_times.sql
-- Cap player HEALING and DAMAGE spells at a 2.5s cast (2500 ms). Complete Heal is exempt (stays long --
-- it's the big slow heal on purpose).
--
-- "Damage/heal" = any spell carrying a CurrentHP effect (SPA 0) in any effect slot (unused slots are 254,
-- so `0 IN (...)` cleanly matches only real HP-change spells -- nukes, direct heals, DoTs, HoTs).
-- Scoped to PLAYER-castable spells (classes8 1-100, id < 10000) so NPC / item-click / trap spells with
-- 15-80s "cast times" are NOT touched. Only spells currently OVER 2500 are lowered; shorter ones stay.
--
-- The server sends cast_time to the client in the cast packet (spells.cpp), so this is server-authoritative
-- -- no client spells_us.txt change is needed for the cast bar. spells_new is in SHARED MEMORY, so after
-- applying: stop world+zones, `./shared_memory`, restart world. (Re-export spells_us.txt only if you want
-- the spellbook *info* text to match -- cosmetic.)

UPDATE spells_new SET cast_time = 2500
WHERE cast_time > 2500
  AND id < 10000
  AND classes8 BETWEEN 1 AND 100
  AND name NOT LIKE 'Complete Heal%'
  AND 0 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,
            effectid7,effectid8,effectid9,effectid10,effectid11,effectid12);
