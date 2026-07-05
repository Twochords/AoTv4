-- aotv4_bard_only_creation.sql
-- Character creation = BARD ONLY, but EVERY race can roll a Bard.
--
-- The world server loads char_create_combinations at boot into character_create_race_class_combos, then
-- (a) SENDS it to the client to drive the creation UI and (b) validates every create request against it
-- (CheckCharCreateInfoSoF). So this table is the single lever for both the UI and validation.
--
-- Step 1 adds a Bard (class 8) combo for every (race, deity, start_zone) that exists for ANY class --
-- reusing that race's own allocation (correct racial base stats) and expansion flag -- so all 16 races
-- become Bard-eligible (not just the default Human/Wood Elf/Half Elf/Vah Shir/Drakkin). INSERT IGNORE
-- dedups on the PK (race,class,deity,start_zone). Step 2 removes every non-Bard class, so Bard is the
-- only creatable class and any non-Bard request (even from a modified client) fails validation.
--
-- Replaces the old event_connect force-to-Bard band-aid (which left a half-converted char until relog).
-- Reversible via aotv4_char_create_combos_backup. Requires a WORLD restart (combos load at world boot).

CREATE TABLE IF NOT EXISTS aotv4_char_create_combos_backup AS SELECT * FROM char_create_combinations;

-- 1) every race can be a Bard (materialize via temp table to avoid self-insert issues)
CREATE TEMPORARY TABLE tmp_bard_combos AS
  SELECT allocation_id, race, 8 AS class, deity, start_zone, expansions_req
  FROM char_create_combinations;
INSERT IGNORE INTO char_create_combinations (allocation_id, race, class, deity, start_zone, expansions_req)
  SELECT allocation_id, race, class, deity, start_zone, expansions_req FROM tmp_bard_combos;
DROP TEMPORARY TABLE tmp_bard_combos;

-- 2) Bard is now the ONLY creatable class
DELETE FROM char_create_combinations WHERE class <> 8;
