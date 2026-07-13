-- AoTv4: Return-Home rework + level cap + new-character bind. CONTENT/RULE changes (safe in a migration).
--
--   * World:MinOfflineTimeToReturnHome -> 1800  : reused as the Return-Home REUSE COOLDOWN length (30 min).
--     The stock "must be offline N hours" gate is replaced in world/worlddb.cpp + world/client.cpp with a
--     data_bucket ('return_home_cd' per character) cooldown, and the destination is hardcoded to tutorialb.
--   * Character:MaxExpLevel -> 50               : reach level 50, gain no XP past it (enforcement is
--     max_level = MaxExpLevel + 1 in zone/exp.cpp). Zone rule -> applies on #rules reload / zone cycle.
--   * start_zones bind -> tutorialb (189)       : new characters bind to tutorialb.
--
-- The world C++ change + the MinOfflineTimeToReturnHome rule need a WORLD REBUILD + restart (world logic/
-- rules don't hot-reload). start_zones affects new characters only.
--
-- NOTE: existing characters' binds (character_bind) are PLAYER DATA -- see aotv4_bind_tutorialb.sql; run
-- that UPDATE while everyone is OFFLINE (a logged-in char re-saves its cached binds over any DB edit).

UPDATE rule_values SET rule_value = '1800' WHERE ruleset_id = 1 AND rule_name = 'World:MinOfflineTimeToReturnHome';
UPDATE rule_values SET rule_value = '50'   WHERE ruleset_id = 1 AND rule_name = 'Character:MaxExpLevel';
UPDATE start_zones  SET bind_id = 189, bind_x = 18, bind_y = -147, bind_z = 20;
