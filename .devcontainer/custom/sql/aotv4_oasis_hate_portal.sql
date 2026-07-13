-- AoTv4: turn the "BFIRE" door in Oasis of Marr (oasis, doorid 20, DB id 36798) into a portal to the
-- Plane of Hate. Drops the player at hateplane's normal port-in (the zone safe point). opentype 58 =
-- teleport door (ports on click, no open animation -- same as our PoK travel books). Doors load at
-- ZONE BOOT -> restart zones after.
UPDATE doors
SET dest_zone = 'hateplane', dest_instance = 0,
    dest_x = -353.08, dest_y = -374.8, dest_z = 3.75, dest_heading = 0,
    opentype = 58
WHERE id = 36798 AND zone = 'oasis' AND name = 'BFIRE';

-- The "BFIRE" fire is a decorative effect, NOT a clickable door, so clicking does nothing. Replace the
-- click with a WALK-IN proximity trigger (like the Feerrott->Fear zone_point): an invisible, untargetable
-- NPC on the fire platform (oasis 410.28,-379.38,134.60) whose proximity ports you to the Plane of Hate.
-- npc_type/spawn load at zone boot -> restart zones. Lua: quests/oasis/Portal_to_Hate.lua.
DELETE FROM spawn2      WHERE spawngroupID = 2000052;
DELETE FROM spawnentry  WHERE spawngroupID = 2000052;
DELETE FROM spawngroup  WHERE id = 2000052;
INSERT IGNORE INTO npc_types (id, name, lastname, level, race, class, bodytype, hp, mana, gender, texture, size, runspeed, npc_faction_id, findable, trackable, show_name, prim_melee_type, mindmg, maxdmg)
VALUES (2000052, 'Portal_to_Hate', '', 1, 127, 1, 11, 100, 0, 2, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0);
INSERT INTO spawngroup (id, name) VALUES (2000052, 'oasis_hate_portal');
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (2000052, 2000052, 100);
INSERT INTO spawn2 (spawngroupID, zone, x, y, z, heading, respawntime, variance) VALUES (2000052, 'oasis', 410.28, -379.38, 134.60, 0, 600, 0);
