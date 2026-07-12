-- AoTv4: with the population rising, drop respawn timers to 10 minutes (600s) for ordinary mobs so
-- there's enough to go around -- EXCLUDING raid content, which stays on its long timers.
--
-- Two exclusions, so a normal dungeon with a single raid boss still speeds up its trash:
--   (1) raid-flagged MOBS      -- any spawngroup that spawns an npc_types.raid_target = 1 NPC (the boss
--                                 itself stays slow, e.g. Lord Nagafen in Nagafen's Lair).
--   (2) DEDICATED raid zones    -- a zone counts as raid only if raid mobs are >=50% of its NPCs OR it
--                                 has >=10 raid mobs (planes of Fear/Hate/Growth, Anguish, Vex Thal, the
--                                 GoD/OoW instances...). A dungeon with just 1-2 raid bosses among lots
--                                 of normal mobs -- Nagafen's Lair (soldungb), Permafrost, Plane of Sky
--                                 (airplane) -- is NOT dedicated, so its trash DOES speed up; only the
--                                 boss is frozen by (1).
--
-- Only REDUCES (respawntime > 600) -- never slows a mob that already pops faster than 10 min. Sets
-- variance = 0 on touched rows so "10 minutes" is exactly 10 minutes. spawn2 loads at ZONE BOOT
-- (not shared memory) -> restart zones after. Idempotent: re-running only reduces rows still above 600.

DROP TEMPORARY TABLE IF EXISTS aotv4_raid_zones;
CREATE TEMPORARY TABLE aotv4_raid_zones AS
  SELECT zone FROM (
    SELECT s2.zone AS zone,
      COUNT(DISTINCT CASE WHEN n.raid_target = 1 THEN se.npcID END) AS raid_npcs,
      COUNT(DISTINCT se.npcID)                                      AS total_npcs
    FROM spawn2 s2
    JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
    JOIN npc_types n   ON n.id = se.npcID
    GROUP BY s2.zone
  ) z
  WHERE z.total_npcs > 0 AND (z.raid_npcs >= 10 OR z.raid_npcs / z.total_npcs >= 0.5);

DROP TEMPORARY TABLE IF EXISTS aotv4_raid_sg;
CREATE TEMPORARY TABLE aotv4_raid_sg AS
  SELECT DISTINCT se.spawngroupID AS sg
  FROM spawnentry se
  JOIN npc_types n ON n.id = se.npcID
  WHERE n.raid_target = 1;

UPDATE spawn2 SET respawntime = 600, variance = 0
WHERE respawntime > 600
  AND zone         NOT IN (SELECT zone FROM aotv4_raid_zones)
  AND spawngroupID NOT IN (SELECT sg   FROM aotv4_raid_sg);

DROP TEMPORARY TABLE IF EXISTS aotv4_raid_zones;
DROP TEMPORARY TABLE IF EXISTS aotv4_raid_sg;
