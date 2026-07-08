-- aotv4_startzone_nonclassic_races.sql
-- Iksar and Vah Shir home cities are post-Classic (Cabilis = Kunark, Shar Vahl = Luclin) and do NOT
-- open on a Classic-locked server, so a character created in one of those races (and NOT sent through
-- the Tutorial) would spawn in a zone that never boots -> stranded. The `start_zones` match key stays
-- the home zone_id (what the RoF2 client sends), but the `start_zone` column (where the server actually
-- places them) + coords + bind are redirected to a faction-safe OPEN Classic city:
--   * Iksar (race 128) -> Grobb (52)  -- Iksar are faction-safe with Trolls.
--   * Vah Shir (race 130) -> Halas (29) -- neutral race, safe in the Barbarian city.
-- The Tutorial EXIT (tutorialb/player.pl) is remapped the same way for the tutorial path.
-- start_zones is read live by world at each character creation -> no world restart needed.

UPDATE start_zones
   SET start_zone = 52, bind_id = 52,
       x = 1.1, y = 14.5, z = 3.1, heading = 0,
       bind_x = 1.1, bind_y = 14.5, bind_z = 3.1
 WHERE player_race = 128;   -- Iksar -> Grobb

UPDATE start_zones
   SET start_zone = 29, bind_id = 29,
       x = 12.2, y = -32.9, z = 3.1, heading = 0,
       bind_x = 12.2, bind_y = -32.9, bind_z = 3.1
 WHERE player_race = 130;   -- Vah Shir -> Halas
