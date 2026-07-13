-- AoTv4: spawn-rate tuning. RAID is the npc_types.raid_target flag; everything else is "non-raid".
--
--   (1) Every NON-RAID NAMED mob spawns ~half the time. spawnentry.chance is a RELATIVE WEIGHT
--       (SpawnGroup::GetNPCType sums chances and rolls once per cycle). For each group we set the named
--       weight so the non-raid named COLLECTIVELY equal the placeholder (trash) total => ~50/50
--       named:PH, whatever the number of named entries (a flat weight overshot in camps with duplicate
--       named, e.g. Beholder's two Lord Syrkl entries hitting ~76%). Applies in EVERY zone.
--   (2) Every NON-RAID spawn point respawns at EXACTLY 600s (10 min), variance 0 -- even points that
--       were faster. respawntime = 0 (scripted/quest/one-time spawns) is never touched.
--   (3) RAID bosses respawn every 6 hours (21600s) at MOST -- a raid already faster than 6h keeps its
--       shorter timer.
--
-- spawn2/spawnentry load at ZONE BOOT (not shared memory) -> restart zones after. Idempotent.

-- spawngroups that can spawn a raid boss (raid_target = 1): spared from the 10-min timer, capped at 6h.
DROP TEMPORARY TABLE IF EXISTS aotv4_raid_sg;
CREATE TEMPORARY TABLE aotv4_raid_sg AS
  SELECT DISTINCT se.spawngroupID AS sg
  FROM spawnentry se JOIN npc_types n ON n.id = se.npcID
  WHERE n.raid_target = 1;

-- per-spawngroup stats: placeholder (trash) total weight + how many NON-RAID named entries it has.
-- PH chances are never modified here, so ph_sum stays the group's original placeholder weight.
DROP TEMPORARY TABLE IF EXISTS aotv4_group_stats;
CREATE TEMPORARY TABLE aotv4_group_stats AS
  SELECT se.spawngroupID AS sg,
    SUM(CASE WHEN (n.name LIKE 'a\_%' OR n.name LIKE 'an\_%') THEN se.chance ELSE 0 END) AS ph_sum,
    SUM(CASE WHEN NOT (n.name LIKE 'a\_%' OR n.name LIKE 'an\_%') AND n.raid_target = 0 THEN 1 ELSE 0 END) AS named_cnt
  FROM spawnentry se JOIN npc_types n ON n.id = se.npcID
  GROUP BY se.spawngroupID;

-- ------------------------------------------------------------------------------------------------
-- (1) ALL non-raid named -> ~50/50 vs placeholders. weight = round(placeholder_total / named_count).
--     Scope: named (not 'a_'/'an_'), NOT raid-flagged, competing (chance > 0), in a group that HAS a
--     placeholder (ph_sum > 0). No zone restriction -- every non-raid named, everywhere.
-- ------------------------------------------------------------------------------------------------
UPDATE spawnentry se
JOIN npc_types n ON n.id = se.npcID
JOIN aotv4_group_stats g ON g.sg = se.spawngroupID
SET se.chance = GREATEST(1, ROUND(g.ph_sum / g.named_cnt))
WHERE n.name NOT LIKE 'a\_%'
  AND n.name NOT LIKE 'an\_%'
  AND n.raid_target = 0
  AND se.chance > 0
  AND g.ph_sum > 0 AND g.named_cnt > 0;

-- ------------------------------------------------------------------------------------------------
-- (2) Non-raid respawn timers -> EXACTLY 600s (10 min), variance 0, in every zone. Only raid-boss
--     spawngroups are spared. respawntime = 0 (scripted spawns) left alone.
-- ------------------------------------------------------------------------------------------------
UPDATE spawn2 SET respawntime = 600, variance = 0
WHERE respawntime > 0
  AND respawntime <> 600
  AND spawngroupID NOT IN (SELECT sg FROM aotv4_raid_sg);

-- ------------------------------------------------------------------------------------------------
-- (3) RAID bosses -> 6h (21600s) CAP only; faster raids keep their timer.
-- ------------------------------------------------------------------------------------------------
UPDATE spawn2 SET respawntime = 21600, variance = 0
WHERE respawntime > 21600
  AND spawngroupID IN (SELECT sg FROM aotv4_raid_sg);

DROP TEMPORARY TABLE IF EXISTS aotv4_raid_sg;
DROP TEMPORARY TABLE IF EXISTS aotv4_group_stats;
