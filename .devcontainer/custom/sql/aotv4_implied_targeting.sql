-- AoTv4: enable server-side "implied targeting" so implied HEALING works.
--
-- Problem: every reward spell has the Bard class level set (spells_new.classes8) so Bards can cast it.
-- The RoF2 CLIENT classifies any spell that has a Bard level as a bard SONG (the same stock logic the
-- server's IsBardSong was patched away from in common/spdat.cpp), and the client's implied-target
-- feature deliberately skips bard songs. So casting a heal on an enemy never redirects to the implied
-- target client-side -> "implied healing" silently fails. We can't clear the Bard level (needed for
-- castability) and can't patch eqgame.exe.
--
-- Fix: turn on Spells:UseSpellImpliedTargeting. When the client (thinking the heal is a song) sends the
-- cast with the enemy as the target, the SERVER passes a beneficial spell through to the enemy's target
-- -- the tank, or the caster themselves when the mob is attacking them (spells.cpp CastSpell). This
-- reproduces implied healing regardless of the client's song classification. Detrimental spells get the
-- mirror behavior (pass a friendly target through to what it's fighting), which is convenient here.
--
-- Rules load at zone BOOT -- restart/bounce zones after applying (no shared_memory rebuild needed).

UPDATE rule_values SET rule_value = 'true'
  WHERE ruleset_id = 1 AND rule_name = 'Spells:UseSpellImpliedTargeting';
