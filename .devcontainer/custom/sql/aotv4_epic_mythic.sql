-- aotv4_epic_mythic.sql
-- Every EPIC is a quest reward, and on AoTv4 quest rewards always come out Mythic. The quest-reward
-- path (QuestManager::summonitem -> AoTv4MythicReward) already hands out base+2,000,000 IF a Mythic row
-- exists -- but most epics never had one (only the ~11 epics that were also loot/merchant-obtainable got
-- tiered by aotv4_gear_tiers.sql). This migration generates the missing Mythic (+2,000,000) rows for
-- EVERY epic weapon/shield so all epic rewards resolve to Mythic.
--
-- Scope: epicitem<>0, base ids (< 1,000,000), NON-augment (itemtype<>54 -- the "Shard of the Ancients"
-- augs stay native; they insert by aug-type and are combine components), and only where a Mythic row
-- doesn't already exist (idempotent; leaves the 11 gear-tier epics alone).
--
-- Stats use the SAME Mythic formula as aotv4_gear_tiers.sql (keep them in step): base stats x2,
-- hp/mana/endur x2.5, damage x2, AC x2, instruments x2, +1/2 heroic on every stat/resist, No Drop.
-- Mythic components still combine because the tradeskill matcher now normalizes tier ids to base
-- (zone/tradeskills.cpp GetTradeRecipe), and any-quality turn-ins work via NPC::CheckHandin.
--
-- items live in SHARED MEMORY: run this, then with world DOWN: cd build/bin && ./shared_memory -> restart.
-- Reversible: DELETE FROM items WHERE id BETWEEN 2000000 AND 2999999 AND ... (or restore aotv4_epic_backup).

DROP TEMPORARY TABLE IF EXISTS aotv4_epic_scope;
CREATE TEMPORARY TABLE aotv4_epic_scope (id INT PRIMARY KEY);
INSERT INTO aotv4_epic_scope
  SELECT i.id FROM items i
  WHERE i.epicitem <> 0
    AND i.id < 1000000
    AND i.itemtype <> 54
    AND NOT EXISTS (SELECT 1 FROM items m WHERE m.id = i.id + 2000000);

DROP TEMPORARY TABLE IF EXISTS tmp_epic_mythic;
CREATE TEMPORARY TABLE tmp_epic_mythic LIKE items;
INSERT INTO tmp_epic_mythic SELECT * FROM items WHERE id IN (SELECT id FROM aotv4_epic_scope);

-- (1) derived + heroic from the SCALED (x2) stats (computed against original base columns)
UPDATE tmp_epic_mythic SET
  spelldmg      = FLOOR((aint * 2) * 1.0),
  healamt       = FLOOR((awis * 2) * 1.0),
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
UPDATE tmp_epic_mythic SET
  hp = FLOOR(hp*2.5), mana = FLOOR(mana*2.5), endur = FLOOR(endur*2.5),
  astr=astr*2, asta=asta*2, aagi=aagi*2, adex=adex*2, awis=awis*2, aint=aint*2, acha=acha*2,
  fr=fr*2, cr=cr*2, mr=mr*2, dr=dr*2, pr=pr*2, svcorruption=svcorruption*2,
  damage = damage*2,
  ac = FLOOR(ac * 2),
  bardvalue = FLOOR(bardvalue * 2),
  procrate = IF(proceffect > 0 AND procrate = 0, 20, procrate),
  nodrop = 0,                                  -- No Drop
  classes = 65535, races = 65535, loregroup = 0,
  Name = CONCAT('Mythic ', Name),
  id = id + 2000000;
INSERT INTO items SELECT * FROM tmp_epic_mythic;

DROP TEMPORARY TABLE IF EXISTS aotv4_epic_scope;
DROP TEMPORARY TABLE IF EXISTS tmp_epic_mythic;
