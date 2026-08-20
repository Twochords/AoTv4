-- Run this ON LIVE. It answers "why do the hub NPCs duplicate and not survive #repop".
-- 1. Do the canonical spawn rows exist at all?  If a row is MISSING here, #repop cannot bring that
--    NPC back, and whatever you are seeing in game was spawned at runtime rather than from spawn2.
SELECT n.id, n.name,
       CASE WHEN s2.id IS NULL THEN '*** NO SPAWN2 ROW ***' ELSE 'ok' END AS spawn_row,
       s2.spawngroupID AS grp, s2.zone, ROUND(s2.x) x, ROUND(s2.y) y,
       s2.min_expansion AS minx, s2.max_expansion AS maxx,
       IFNULL(s2.content_flags,'-') AS cflags, s2._condition AS cond
FROM npc_types n
 LEFT JOIN spawnentry se ON se.npcID = n.id
 LEFT JOIN spawn2 s2     ON s2.spawngroupID = se.spawngroupID
WHERE n.id IN (2000400,2000401,2000402,2000410,2000411)
ORDER BY n.id;

-- 2. Duplicates that ARE in the tables: same custom NPC placed twice in one zone.
--    A second row under a DIFFERENT spawngroupID is a hand placement (#spawn/#spawnsave); the
--    install scripts DELETE by spawngroupID, so they can never see or clean it up.
SELECT s2.zone, se.npcID, n.name, COUNT(*) AS placements,
       GROUP_CONCAT(CONCAT('grp ',s2.spawngroupID,' @ ',ROUND(s2.x),',',ROUND(s2.y))
                    ORDER BY s2.spawngroupID SEPARATOR '   |   ') AS placed_at
FROM spawn2 s2
 JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
 JOIN npc_types  n  ON n.id            = se.npcID
WHERE se.npcID >= 2000000
GROUP BY s2.zone, se.npcID, n.name
HAVING placements > 1
ORDER BY placements DESC, s2.zone, se.npcID;

-- 3. How far behind is live?  v73 creates the Titan Hall spawns, v79 the fellowship NPCs.
SELECT custom_version FROM db_version;
