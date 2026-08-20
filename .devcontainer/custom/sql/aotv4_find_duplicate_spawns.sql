-- AoTv4 custom NPCs (id >= 2000000) placed more than once IN THE SAME ZONE.
-- Same NPC in several DIFFERENT zones is fine (the Parcel Courier is one per city);
-- two placements in one zone is the duplicate you can see standing on itself.
SELECT s2.zone, se.npcID, n.name, COUNT(*) AS placements,
       GROUP_CONCAT(CONCAT('grp ',s2.spawngroupID,' @ ',ROUND(s2.x),',',ROUND(s2.y),',',ROUND(s2.z))
                    ORDER BY s2.spawngroupID SEPARATOR '   |   ') AS where_they_are
FROM spawn2 s2
 JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
 JOIN npc_types  n  ON n.id            = se.npcID
WHERE se.npcID >= 2000000
GROUP BY s2.zone, se.npcID, n.name
HAVING placements > 1
ORDER BY placements DESC, s2.zone, se.npcID;
