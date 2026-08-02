-- ============================================================================================
-- AoTv4 -- proc spells get player-facing names (2026-08-02)
--
-- ============================================================================================
-- "Trigger" AND "Effect" ARE DEVELOPER JARGON, AND PLAYERS SEE THEM
-- ============================================================================================
-- The helper spells behind the custom lines were named for the developer reading the SQL, not for
-- the player reading their screen:
--     43350-55  "Skin of the Lizard Trigger"
--     43356-61  "Faint Sloth Trigger"
--     43374-79  "Kindred Spark Effect"
--
-- ⚠️ These are NOT internal-only. The sloth triggers carry buffduration 3, so they land as a real,
-- inspectable debuff on the creature that hit you -- named "Faint Sloth Trigger" on its buff bar.
-- The reptile and kindred helpers are what the engine names when it resolves their heal. So the one
-- place the mechanic finally becomes visible is also the place it looks unfinished.
--
-- Dropping the suffix leaves the proc sharing a name with the buff that fired it, which is correct
-- and is what live does -- Blessing of the Blackstar is the proc of Mace of Night and simply carries
-- the line's name. A player seeing "Kindred Ember" heal their group can connect it to the "Kindred
-- Ember" they cast; "Kindred Ember Effect" only raises a question.
--
-- ⚠️ Safe against the pool: gen_stock_pool.pl takes stock spells with `id < 10000` plus the custom
-- band 43300-43349 wholesale. Everything renamed here is 43350+, which is the helper band and is
-- never offered, so no rename can push one of these into the reward pool.
-- ⚠️ The Promised line's 43368-73 "... Bloom" is deliberately NOT touched. "Bloom" is the real live
-- term for the delayed heal of a Promised spell, not jargon -- it is the mechanic's actual name.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

UPDATE spells_new SET name = REPLACE(name, ' Trigger', '') WHERE id BETWEEN 43350 AND 43361;
UPDATE spells_new SET name = REPLACE(name, ' Effect',  '') WHERE id BETWEEN 43374 AND 43379;

-- Verification: no jargon suffix left on any helper except the Promised blooms.
SELECT id, name FROM spells_new
WHERE id BETWEEN 43350 AND 43379 AND name <> ''
ORDER BY id;
