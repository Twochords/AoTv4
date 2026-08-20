-- @aotv4-migration
-- description: 2026_08_20_parcel_courier_unspawnable_cities
-- check: SELECT z.short_name FROM zone z JOIN spawn2 s ON s.zone = z.short_name JOIN spawnentry se ON se.spawngroupID = s.spawngroupID JOIN npc_types n ON n.id = se.npcID AND n.is_parcel_merchant = 1 WHERE z.version = 0 GROUP BY z.short_name HAVING SUM(CASE WHEN s.min_expansion <= 0 AND s.max_expansion <= 0 THEN 1 ELSE 0 END) = 0 LIMIT 1
-- condition: not_empty
-- match:
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Reported from play as the Qeynos parcel vendor not working. South Qeynos has a parcel
-- notes: merchant on paper (Ren_Pinemyer 1032) that can NEVER spawn: its spawn2 row carries
-- notes: min_expansion 5 and this server runs Expansion:CurrentExpansion 0, so ContentFilterCriteria
-- notes: removes it at zone boot with no error anywhere. Seven other cities are the same.
-- notes: aotv4_parcel_merchants.sql was supposed to cover exactly this, but its "does this zone
-- notes: already have one" test asked whether a ROW EXISTS rather than whether it SPAWNS, so it
-- notes: counted those eight as served and skipped them.

-- ⚠️⚠️ THE COVERAGE TEST MUST ASK "DOES IT SPAWN", NOT "DOES A ROW EXIST".  That distinction is the
-- entire bug.  aotv4_parcel_merchants.sql warns about the expansion filter in its own comments and
-- applies it to the rows it INSERTS, while the NOT EXISTS that decides WHERE to insert ignores it.
-- A blocked merchant is indistinguishable from a working one in the tables and invisible in game.
--
-- ⚠️ Deliberately does NOT clear min_expansion on the stock merchants instead.  Those are Luclin-era
-- NPCs that also carry real merchantlists, so unblocking them would add shops to Classic cities as a
-- side effect of fixing parcels -- a content change smuggled in behind a bug fix.  The courier is
-- the established pattern here: class 41, is_parcel_merchant 1, merchant_id 0, no faction.
--
-- ⚠️ Driven off the live "unserved" query, NOT off start_zones.  aotv4_start_resplendent.sql
-- repointed all 1,441 start_zones rows to resplendent, so the original script's data source now
-- yields exactly ONE zone and would place a courier nowhere useful.
--
-- ⚠️ Idempotent by construction: once a zone has a spawnable courier it stops matching the query, so
-- a second run inserts nothing.  Ids continue the existing 2000300-2000350 band rather than reusing
-- it, so the nine couriers already placed are untouched.

DROP TEMPORARY TABLE IF EXISTS aotv4_parcel_gap;
CREATE TEMPORARY TABLE aotv4_parcel_gap (
  n INT, short_name VARCHAR(32), x FLOAT, y FLOAT, z FLOAT
);

-- ⚠️ The temp table is materialised FIRST because the INSERTs below change what the query returns.
-- Reading it live three times would give three different answers.
INSERT INTO aotv4_parcel_gap
SELECT ROW_NUMBER() OVER (ORDER BY t.short_name), t.short_name, t.safe_x, t.safe_y, t.safe_z
FROM (
  SELECT z.short_name, z.safe_x, z.safe_y, z.safe_z
  FROM zone z
  JOIN spawn2 s       ON s.zone = z.short_name
  JOIN spawnentry se  ON se.spawngroupID = s.spawngroupID
  JOIN npc_types n    ON n.id = se.npcID AND n.is_parcel_merchant = 1
  WHERE z.version = 0
  GROUP BY z.short_name, z.safe_x, z.safe_y, z.safe_z
  HAVING SUM(CASE WHEN s.min_expansion <= 0 AND s.max_expansion <= 0 THEN 1 ELSE 0 END) = 0
) t;

SET @base = (SELECT COALESCE(MAX(id), 2000299) FROM spawngroup WHERE id BETWEEN 2000300 AND 2000350);

INSERT INTO spawngroup (id, name, spawn_limit, dist, max_x, min_x, max_y, min_y, delay, mindelay)
  SELECT @base + n, CONCAT('aotv4_parcel_courier_', short_name), 1, 0, 0, 0, 0, 0, 0, 0
  FROM aotv4_parcel_gap;

INSERT INTO spawnentry (spawngroupID, npcID, chance)
  SELECT @base + n, 2000220, 100 FROM aotv4_parcel_gap;

-- ⚠️⚠️ min_expansion / max_expansion MUST BE -1 -- that is the whole point of this migration.
-- ⚠️ The column is `_condition` (leading underscore); `condition` is reserved.
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid,
                    _condition, cond_value, min_expansion, max_expansion)
  SELECT @base + n, short_name, 0, x, y, z, 0, 600, 0, 0, 0, 1, -1, -1
  FROM aotv4_parcel_gap;

DROP TEMPORARY TABLE IF EXISTS aotv4_parcel_gap;
