-- @aotv4-migration
-- description: 2026_08_22_gilded_wager_place_theater
-- check: SELECT `npcID` FROM `spawnentry` WHERE `npcID` = 2000403
-- condition: empty
-- match:
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Places the Gilded Wager in the Theater of the Tranquil at the owner-supplied location.

-- ⚠️⚠️ THE OWNER SUPPLIED AN IN-GAME `/loc`, WHICH PRINTS **Y, X, Z** -- THE FIRST TWO ARE SWAPPED
-- RELATIVE TO THE spawn2 COLUMNS. Section 11 records this exact trap, evidenced against a real door:
-- a Butcherblock reading of "1239.39, 2929.87" matched pos_y 1227.09 and pos_x 2967.91.
--     given /loc:  -15.70, -68.98, -27.10
--     therefore:   x = -68.98   y = -15.70   z = -27.10
-- 📌 Pasting a raw /loc straight in would put him about 70 units away -- still inside the zone and
-- still standing on the floor, which is precisely why it would not look like a bug.
-- 📌 Corroborated rather than assumed: the zone safe point is 0.0 / -6.0 / -28.0, so this is the same
-- flat floor and the z is a unit off the known-good ground there.
--
-- ⚠️ min_expansion and max_expansion MUST be -1. freeporttheater is expansion 11 on a server locked
-- to Classic, and a spawn left at 0 is content filtered out at zone boot with no error anywhere --
-- the same thing that removed the Plane of Knowledge books until aotv4_pok_travel.sql fixed them.
-- 📌 The ZONE itself is already reachable: region 0 "Always Available" and bypass_expansion_check 1,
-- so both gates section 24 records for delve maps are already open here.
--
-- ⚠️ The quest script is `quests/global/2000403.lua`, by NPC id, so it answers in this zone and any
-- other he is ever moved to. Do not move it into quests/freeporttheater/.
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;

INSERT INTO spawngroup (id, name) VALUES (2000403, 'aotv4_gilded_wager');
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, min_expansion, max_expansion)
VALUES (2000403, 'freeporttheater', 0, -68.98, -15.70, -27.10, 256, 300, 0, 0, -1, -1);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (2000403, 2000403, 100);
