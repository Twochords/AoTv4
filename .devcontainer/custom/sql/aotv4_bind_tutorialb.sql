-- AoTv4: existing-character bind fix -- repoint EVERY bind slot (0 = death/Gate, 1-4 = home-city) to
-- tutorialb (zone 189). This is the PLAYER-DATA half of the tutorialb-bind change.
--
-- ⚠️ RUN WHILE EVERYONE IS OFFLINE. A logged-in character caches its binds in the profile and bulk-saves
--    them on camp/zone/logout, which clobbers this UPDATE -- so it only sticks for offline characters.
--    Fold it into the staged maintenance restart (alongside the tradeskills/skill_caps re-applies), or
--    re-run it then. Safe to re-run any time.
--
-- NOTE: the NEW-character side (start_zones -> tutorialb) is CONTENT and lives in the tracked migration
--       aotv4_return_home_levelcap_bind.sql. This file is ONLY the one-time offline player-data UPDATE.

UPDATE character_bind SET zone_id = 189, instance_id = 0, x = 18, y = -147, z = 20, heading = 0;
