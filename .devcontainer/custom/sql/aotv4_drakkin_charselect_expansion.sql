-- aotv4_drakkin_charselect_expansion.sql  (2026-08-01)
-- =================================================================================================
-- Make DRAKKIN (race 522) selectable at character creation.
--
-- Drakkin were added in The Serpent's Spine (TSS), whose account-expansion bit is 2048. The
-- character-select expansion mask sent to the client (World:CharacterSelectExpansionSettings) was
-- 522239 = "all expansions EXCEPT TSS", so the RoF2 client greyed Drakkin out on the create screen.
-- (char_create_combinations.expansions_req is NOT the gate -- the world loads all combos with no
--  expansion filter and only checks the combo EXISTS; that column is dead server-side.)
--
-- Fix = set the char-select mask to 524287 (all 19 expansion bits, matching World:ExpansionSettings
-- which is already 524287 for in-game). New Drakkin still start in Resplendent like every other race
-- (World:SoFStartZoneID = 729 overwrites the chosen start city for all SoF+ clients).
--
-- Rules load at WORLD boot -> a world restart applies this (a DB change alone does not).

UPDATE rule_values
   SET rule_value = '524287'
 WHERE rule_name = 'World:CharacterSelectExpansionSettings';
