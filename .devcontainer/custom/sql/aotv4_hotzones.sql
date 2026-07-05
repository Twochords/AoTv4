-- aotv4_hotzones.sql  -- SETUP for the rotating hot-zone system (run once; idempotent, re-runnable).
-- Builds the tables the daily roll (aotv4_hotzone_roll.sh) reads/writes:
--   * aotv4_zone_zem_base  : snapshot of each zone's ORIGINAL ZEM (restored each roll).
--   * aotv4_zone_pool      : candidate zones, tagged by level band (1..7) AND expansion (0..8).
--   * aotv4_hotzone_current: the day's picks (drives the MOTD + next roll's restore).
--   * aotv4_zone_bucket_override : manual per-zone band overrides / exclusions.
--
-- The pool spans ALL expansions; the ROLL filters to the currently-unlocked era at run time (a zone is
-- only eligible once its expansion is open). Level band = ceil(median combat-NPC level / 10), 1..7
-- (1-10 .. 61-70). One hot zone per band up to the era's cap, plus one extra endgame zone per unlocked
-- expansion (Classic=5, Kunark=6, ...). Zones load ZEM at boot from the `zone` table (NOT shared
-- memory), so the roll's DB updates apply the next time each (idle) zone boots.

-- ---- 1) base ZEM snapshot (capture true base BEFORE any hot-zone edits; only fills missing rows) ----
CREATE TABLE IF NOT EXISTS aotv4_zone_zem_base (
  zone_id    INT PRIMARY KEY,
  short_name VARCHAR(32),
  base_zem   DECIMAL(6,2)
);
INSERT IGNORE INTO aotv4_zone_zem_base (zone_id, short_name, base_zem)
  SELECT zoneidnumber, short_name, zone_exp_multiplier FROM zone WHERE version = 0;

-- ---- 2) day's picks table (N rows, one per hot zone; N grows with era) ----
DROP TABLE IF EXISTS aotv4_hotzone_current;
CREATE TABLE aotv4_hotzone_current (
  pick_no          TINYINT PRIMARY KEY,   -- 1..N ordering (by band, extras last)
  bucket           TINYINT,               -- level band 1..7
  zone_id          INT,
  short_name       VARCHAR(32),
  long_name        VARCHAR(100),
  level_range      VARCHAR(8),
  multiplier       DECIMAL(4,1),          -- 2.0 or 3.0 (x the base ZEM)
  base_zem         DECIMAL(6,2),
  new_zem          DECIMAL(6,2),
  distinct_players INT,
  picked_at        DATETIME
);

-- ---- 3) candidate pool (all expansions; bands 1..7) ----
DROP TABLE IF EXISTS aotv4_zone_pool;
CREATE TABLE aotv4_zone_pool (
  zone_id      INT PRIMARY KEY,
  short_name   VARCHAR(32),
  long_name    VARCHAR(100),
  bucket       TINYINT,       -- level band 1..7
  expansion    TINYINT,       -- 0..8 (roll gates on the unlocked era)
  median_level DECIMAL(4,1),
  base_zem     DECIMAL(6,2)
);

-- Band = ceil(median NPC level / 10), clamped 1..7. Median is robust to BOTH a few high-level mobs
-- (which skew an average up) and lots of low-level filler/guards (which skew a mode down). Only real
-- combat mobs (maxdmg > 0, not merchants) count, so level-1 statues/triggers don't poison it.
-- Included: huntable (base ZEM > 0) zones with >= 8 real NPCs, ANY expansion. Excluded: utility/travel
-- hubs and home CITIES (we don't want to reward killing civilians/guards).
DELETE FROM aotv4_zone_pool;
INSERT INTO aotv4_zone_pool (zone_id, short_name, long_name, bucket, expansion, median_level, base_zem)
SELECT z.zoneidnumber, z.short_name, z.long_name,
       LEAST(7, GREATEST(1, CEIL(m.median_level / 10.0))) AS bucket,
       z.expansion, m.median_level, z.zone_exp_multiplier
FROM zone z
JOIN (
    SELECT zone, AVG(level) AS median_level
    FROM (
        SELECT s2.zone AS zone, n.level AS level,
               ROW_NUMBER() OVER (PARTITION BY s2.zone ORDER BY n.level) AS rn,
               COUNT(*)    OVER (PARTITION BY s2.zone)                    AS cnt
        FROM spawn2 s2
        JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
        JOIN npc_types  n  ON n.id = se.npcID
        WHERE n.level BETWEEN 1 AND 70 AND n.maxdmg > 0 AND n.merchant_id = 0
    ) r
    WHERE cnt >= 8 AND rn IN (FLOOR((cnt + 1) / 2), CEIL((cnt + 1) / 2))
    GROUP BY zone
) m ON m.zone = z.short_name
WHERE z.version = 0
  AND z.zone_exp_multiplier > 0              -- huntable (0 ZEM = no-exp zones)
  AND z.short_name NOT IN (
    -- utility / travel / hub / no-hunt zones
    'bazaar','nexus','poknowledge','potranquility','shadowhaven','guildlobby','guildhall',
    'tutorial','tutoriala','tutorialb','load','load2','loadanim','arena','arena2','shadowrest',
    -- home CITIES: never hot -- do not incentivize killing civilians / city guards.
    -- (Outdoor zones near cities -- Greater Faydark, Qeynos Hills, the Karanas -- stay in. So do
    --  huntable planes: poair, pojustice, ponightmare, codecay, bothunter, etc.)
    'qeynos','qeynos2','qrg','freporte','freportw','freportn',
    'neriaka','neriakb','neriakc','felwithea','felwitheb','kaladima','kaladimb',
    'erudnext','erudnint','akanon','grobb','oggok','halas','rivervale','paineel',
    'cabwest','cabeast','sharvahl','thurgadina','thurgadinb','kelethin'
  );

-- ---- 4) manual band overrides (auto-median misreads a few bimodal zones) ----
-- Edit freely: bucket 1..7 forces a zone's level band; bucket 0 EXCLUDES it from the pool.
CREATE TABLE IF NOT EXISTS aotv4_zone_bucket_override (
  short_name VARCHAR(32) PRIMARY KEY,
  bucket     TINYINT      -- 1..7 = force that band; 0 = exclude from the pool
);
INSERT INTO aotv4_zone_bucket_override (short_name, bucket) VALUES
  ('cazicthule', 0)       -- bimodal froglok population reads too low; exclude for now
  ON DUPLICATE KEY UPDATE bucket = VALUES(bucket);

DELETE p FROM aotv4_zone_pool p
  JOIN aotv4_zone_bucket_override o ON o.short_name = p.short_name WHERE o.bucket = 0;
UPDATE aotv4_zone_pool p
  JOIN aotv4_zone_bucket_override o ON o.short_name = p.short_name
  SET p.bucket = o.bucket WHERE o.bucket BETWEEN 1 AND 7;
