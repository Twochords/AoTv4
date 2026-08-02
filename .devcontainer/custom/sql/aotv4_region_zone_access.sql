-- ============================================================================================
-- AoTv4 -- add Kaesora to the Cabilis region, and let the post-Classic regions be entered at all
-- (2026-08-02)
--
-- ============================================================================================
-- PART 1: KAESORA -> CABILIS
-- ============================================================================================
-- Kaesora (zone 88) is a Kunark dungeon off Field of Bone / Lake of Ill Omen, so it belongs with
-- the rest of the Iksar content. It was sitting in region 99 "Unused", which is the catch-all every
-- unmapped zone falls into and which nobody can hold -- i.e. permanently locked.
-- ⚠️ UPDATE, not INSERT: it already has exactly one zone_regions row. Inserting a second would give
-- the zone two regions, and `m_zone_to_region` is a std::map keyed by zone id (zone/regions.h:57) --
-- one region per zone, last row loaded wins, with the winner depending on undefined result ordering.
--
-- ============================================================================================
-- PART 2: THE REASON THAT ALONE WOULD HAVE CHANGED NOTHING
-- ============================================================================================
-- Three of the six regions cannot be entered by a player at all, and Kaesora would have joined them:
--
--     region 3 Thurgadin     9 of 9 zones blocked
--     region 4 Firiona Vie   8 of 8 zones blocked
--     region 6 Cabilis       8 of 8 zones blocked
--
-- Those are Kunark and Velious zones (`zone.expansion` 1-6) and the server runs Classic
-- (`Expansion:CurrentExpansion` = 0), so zone/zoning.cpp:366 refuses them with "part of an expansion
-- that you do not yet own" for anyone without the GM flag. It is the exact failure that made the
-- delves unenterable, found only once GM was turned off -- and it silently covers half the atlas.
--
-- ⚠️⚠️ THE REGION SYSTEM AND THE EXPANSION GATE ARE TWO GATES DOING THE SAME JOB, AND THE REGION ONE
-- IS THE ONE THIS SERVER ACTUALLY USES. Regions are earned by dying at the cap and spent with
-- Wayfinder Alessa; the whole point is that unlocking Cabilis lets you into Cabilis. Leaving the
-- expansion check on top means the reward cannot be collected -- a player buys a region and is still
-- refused at the door, with a message about account keys that makes no sense here.
--
-- So the region-mapped zones are exempted from the EXPANSION gate only. `expansion` itself is left
-- honest (content filtering, zone listings and the era system in section 12 all read it) -- exactly
-- the reasoning used for the delve zones in aotv4_delve_zone_access.sql.
--
-- ⚠️ Scoped to zones that are IN A PLAYER-EARNABLE REGION (1-6). Region 99 "Unused" holds 413 zones
-- and stays exactly as it is: those are locked on purpose and this must not become a blanket unlock.
-- Region 0 "Always Available" is already reachable.
--
-- ⚠️ NOT shared memory: `zone` and `zone_regions` are read at boot by world AND zone. Restart both.
-- ============================================================================================

-- Part 1
UPDATE zone_regions SET region_id = 6 WHERE zone_id = 88;

-- Part 2 -- every version row of every zone in an earnable region.
UPDATE zone z
JOIN zone_regions zr ON zr.zone_id = z.zoneidnumber
SET z.bypass_expansion_check = 1
WHERE zr.region_id BETWEEN 1 AND 6;

-- Verification: no region 1-6 zone should remain blocked, and Cabilis should now hold 9.
SELECT r.id, r.name,
       COUNT(DISTINCT z.zoneidnumber) AS zones,
       SUM(CASE WHEN z.expansion > 0 AND z.bypass_expansion_check = 0 THEN 1 ELSE 0 END) AS still_blocked
FROM regions r
JOIN zone_regions zr ON zr.region_id = r.id
JOIN zone z ON z.zoneidnumber = zr.zone_id AND z.version = 0
WHERE r.id BETWEEN 1 AND 6
GROUP BY r.id, r.name ORDER BY r.id;
