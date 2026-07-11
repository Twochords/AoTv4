-- ==========================================================================================
-- AoTv4 gear tiers. Adds two tiers ABOVE native gear:
--   Hallowed  = base item id + 300,000,  name "Hallowed <name>"
--   Mythic    = base item id + 600,000,  name "Mythic <name>"   (No Drop)
-- Scope: "currently-obtainable" equippable items (slots>0, referenced by lootdrop_entries
--        or merchantlist). Native rows are edited in place only for all/all + not-lore.
-- Derived stats use the tier's SCALED stats (e.g. Hallowed spelldmg = (2*int)*0.5).
--
-- NOTE on the offsets: RoF2 item LINKS encode the id in a 5-hex-digit field capped at 0xFFFFF
-- (1,048,575). The old +1,000,000/+2,000,000 scheme pushed Mythic (and high-base Hallowed) past
-- that ceiling, so their chat links rendered as garbage. Base ids top out at 147,494, so the tier
-- "step" is 300,000: Hallowed = base+1*step, Mythic = base+2*step, both well under the ceiling. The
-- reserved band is [300000, 900000); base-id recovery in C++ is still (id % 300000). Keep the C++
-- (loot.cpp, npc.cpp, questmgr.cpp, attack.cpp, tradeskills.cpp AOTV4_TIER_STEP) in step with this.
-- Items live in SHARED MEMORY -- after running this, rebuild ./shared_memory + restart world/zones.
-- Idempotent: re-running rebuilds the tier rows (that id band holds nothing else).
-- ==========================================================================================

-- ---- scope: obtainable equippable base items ------------------------------------------------
DROP TEMPORARY TABLE IF EXISTS aotv4_scope;
CREATE TEMPORARY TABLE aotv4_scope (id INT PRIMARY KEY);
INSERT INTO aotv4_scope
  SELECT DISTINCT i.id FROM items i
  WHERE i.slots > 0
    AND i.id < 300000   -- base items only (max native id is 147,494); never re-tier our own tier rows
    AND (i.id IN (SELECT item_id FROM lootdrop_entries) OR i.id IN (SELECT item FROM merchantlist));

-- AoTv4: also tier TUTORIAL quest-reward GEAR. These are quest-only (not in lootdrop_entries/merchantlist),
-- so the scope above misses them and AoTv4MythicReward silently no-ops when the tutorial hands them out.
-- Explicit list from quests/tutorialb/*.pl (weapons/armor/rings only -- spell scrolls & consumables excluded).
INSERT IGNORE INTO aotv4_scope (id) VALUES
  (38005),(38026),(38047),(38068),(38089),(38110),(38131),(38152),(38173),(38194),(38215),(38236),
  (38257),(38278),(38299),(38320),                                       -- class "Ring (Test)" starter items
  (54230),(54231),(54232),(54233),(54235),(54236),(54237),(54238),      -- newbie weapons (incl. bronze/iron SWORDS 54231/54236)
  (59950),(59951),(59952),(59953),                                       -- sharpened newbie weapons
  (59943),                                                                -- Kobold Skull Charm
  (67104),(67111),(67118),(67125),(59969),                               -- Kobold sleeves + Kobold Leather Mask
  (82929),(82930),(82936),(82937),(82943),(82944),(82950),(82951),       -- Gloom body/legs armor
  (54217),(54218),(54219),(54220),(54221),(54222),(54223),               -- Stitched Burlap armor (silk turn-ins)
  (54225),(54226),(54227),(54228),                                       -- Stitched Burlap armor cont.
  (54696);                                                                -- Steatite Fragment (primary-slot quest reward)

-- AoTv4: NEVER tier the tutorial turn-in starter weapons Absor accepts (Dagger*/Short Sword*/Club*/Dull
-- Axe*). They're handed IN to his quest via plugin::check_handin(<base id>), so they must stay BASE --
-- a Mythic copy would fail the hand-in. (The player spawns with 9998 base; Absor gives back the Mythic
-- Sharpened reward.) 9998/9999 are in loottables so the auto-scope grabbed them; remove them here.
DELETE FROM aotv4_scope WHERE id IN (9997, 9998, 9999, 55623);

-- clear any prior tier rows so re-runs are clean. New band [300000,900000) holds only our tiers.
-- Also sweep the OLD +1M/+2M band (pre-2026-07 scheme) so a migration leaves no orphan tier rows,
-- but SPARE the refine crucible bag (2000060, created by aotv4_refine_crucible.sql).
DELETE FROM items WHERE id BETWEEN 300000 AND 899999;
DELETE FROM items WHERE id BETWEEN 1000000 AND 2999999 AND id <> 2000060;

-- ============================== HALLOWED (base + 300,000) ==================================
DROP TEMPORARY TABLE IF EXISTS tmp_hallowed;
CREATE TEMPORARY TABLE tmp_hallowed LIKE items;
INSERT INTO tmp_hallowed SELECT * FROM items WHERE id IN (SELECT id FROM aotv4_scope);

-- (1) derived stats from the SCALED (x2) stats, computed while base columns are still original
UPDATE tmp_hallowed SET
  spelldmg      = FLOOR((aint * 2) * 0.5),     -- 1 int  -> .5 spell power
  healamt       = FLOOR((awis * 2) * 0.5),     -- 1 wis  -> .5 healing power
  attack        = FLOOR((astr * 2) / 10),      -- 1:10 str -> attack
  strikethrough = FLOOR((aagi * 2) / 10),      -- 1:10 agi -> strikethrough
  accuracy      = FLOOR((adex * 2) / 10);      -- 1:10 dex -> accuracy

-- (2) scale base stats x2, hp/mana/endur x2, instruments x1.5, set proc/flags/name/id
UPDATE tmp_hallowed SET
  hp = FLOOR(hp*2), mana = FLOOR(mana*2), endur = FLOOR(endur*2),
  astr=astr*2, asta=asta*2, aagi=aagi*2, adex=adex*2, awis=awis*2, aint=aint*2, acha=acha*2,
  fr=fr*2, cr=cr*2, mr=mr*2, dr=dr*2, pr=pr*2, svcorruption=svcorruption*2,
  damage = damage*2,
  ac = FLOOR(ac * 1.5),                        -- AC: 1.5x native
  bardvalue = FLOOR(bardvalue * 1.5),
  procrate = IF(proceffect > 0, GREATEST(10, procrate * 2), procrate),   -- Hallowed procs 2x native (min 10)
  recastdelay = IF(clickeffect > 0 AND recastdelay > 0, FLOOR(recastdelay / 2), recastdelay),  -- click reuse /2
  nodrop = 1,                                  -- tradeable (only Mythic is No Drop)
  classes = 65535, races = 65535, loregroup = 0,
  Name = CONCAT('Hallowed ', Name),
  id = id + 300000;
INSERT INTO items SELECT * FROM tmp_hallowed;

-- ================================ MYTHIC (base + 600,000) ==================================
DROP TEMPORARY TABLE IF EXISTS tmp_mythic;
CREATE TEMPORARY TABLE tmp_mythic LIKE items;
INSERT INTO tmp_mythic SELECT * FROM items WHERE id IN (SELECT id FROM aotv4_scope);

-- (1) derived + heroic from the SCALED (x2) stats (computed against original base columns)
UPDATE tmp_mythic SET
  spelldmg      = FLOOR((aint * 2) * 1.0),     -- 1 int -> 1 spell power
  healamt       = FLOOR((awis * 2) * 1.0),     -- 1 wis -> 1 healing power
  attack        = FLOOR((astr * 2) / 10),
  strikethrough = FLOOR((aagi * 2) / 10),
  accuracy      = FLOOR((adex * 2) / 10),
  heroic_str = FLOOR((astr*2)*0.5), heroic_sta = FLOOR((asta*2)*0.5),
  heroic_agi = FLOOR((aagi*2)*0.5), heroic_dex = FLOOR((adex*2)*0.5),
  heroic_wis = FLOOR((awis*2)*0.5), heroic_int = FLOOR((aint*2)*0.5),
  heroic_cha = FLOOR((acha*2)*0.5),
  heroic_fr = FLOOR((fr*2)*0.5), heroic_cr = FLOOR((cr*2)*0.5), heroic_mr = FLOOR((mr*2)*0.5),
  heroic_dr = FLOOR((dr*2)*0.5), heroic_pr = FLOOR((pr*2)*0.5),
  heroic_svcorrup = FLOOR((svcorruption*2)*0.5);

-- (2) scale bases (stats x2, hp/mana/endur x2.5), instruments x2, No Drop, flags/name/id
UPDATE tmp_mythic SET
  hp = FLOOR(hp*2.5), mana = FLOOR(mana*2.5), endur = FLOOR(endur*2.5),
  astr=astr*2, asta=asta*2, aagi=aagi*2, adex=adex*2, awis=awis*2, aint=aint*2, acha=acha*2,
  fr=fr*2, cr=cr*2, mr=mr*2, dr=dr*2, pr=pr*2, svcorruption=svcorruption*2,
  damage = damage*2,
  ac = FLOOR(ac * 2),                          -- AC: 2x native
  bardvalue = FLOOR(bardvalue * 2),
  procrate = IF(proceffect > 0, GREATEST(20, procrate * 4), procrate),   -- Mythic procs 4x native = 2x Hallowed (min 20)
  recastdelay = IF(clickeffect > 0 AND recastdelay > 0, FLOOR(recastdelay / 4), recastdelay),  -- click reuse /4
  nodrop = 0,                                  -- No Drop
  classes = 65535, races = 65535, loregroup = 0,
  Name = CONCAT('Mythic ', Name),
  id = id + 600000;
INSERT INTO items SELECT * FROM tmp_mythic;

-- ================================ NATIVE (in place) =========================================
-- all/all + not-lore + tradeable for the obtainable native gear. (Tier drop rules: native + Hallowed
-- tradeable, only Mythic is No Drop.)
UPDATE items SET classes = 65535, races = 65535, loregroup = 0, nodrop = 1
WHERE id IN (SELECT id FROM aotv4_scope);

DROP TEMPORARY TABLE IF EXISTS aotv4_scope;
DROP TEMPORARY TABLE IF EXISTS tmp_hallowed;
DROP TEMPORARY TABLE IF EXISTS tmp_mythic;
