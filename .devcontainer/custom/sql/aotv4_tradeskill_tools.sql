-- ============================================================================================
-- AoTv4 -- the crafter's tool (+20) and the crafter's mask illusion (+30) (2026-08-04)
-- ============================================================================================
-- Twelve of each, one per tradeskill, indexed in the SAME ORDER as AOTV4_TS_SKILLS in
-- zone/tradeskills.cpp:
--     55 Fishing, 56 Make Poison, 57 Tinkering, 58 Research, 59 Alchemy, 60 Baking,
--     61 Tailoring, 63 Blacksmithing, 64 Fletching, 65 Brewing, 68 Jewelcrafting, 69 Pottery
--
--     tools     147930 + idx     worn, head    +20 to that tradeskill
--     masks     147942 + idx     clicky        casts the illusion below
--     illusions  44400 + idx     buff          +30 to that tradeskill
--
-- ⚠️⚠️ THE +20 AND +30 ARE PAID IN C++, NOT BY THESE ROWS. AoTv4TradeskillSkill (tradeskills.cpp)
-- adds them by ID -- the item row and the spell row are INERT markers, exactly like the Thirst line
-- and the Shield Wall buffs. Neither is expressible natively: Client::GetSkill treats an item's
-- skillmod as a PERCENTAGE and never reads spell bonuses at all, and no SPA grants flat skill.
-- ⚠️ The bonuses live in tradeskills.cpp. Change them there, not here.
-- ⚠️ They STACK: different sources, added independently -- wearing the tool AND holding the illusion
-- is +50, which is what takes 300 base skill to the ~350 where every combine comes out Mythic.
--
-- ⚠️⚠️ THE ID ORDER IS LOAD BEARING. The C++ indexes both bands off AOTV4_TS_SKILLS, so 147930 must
-- be Fishing and 44400 must be Fishing. Reordering this file silently gives Blacksmiths the fishing
-- bonus, and nothing anywhere reports an error.
-- ⚠️⚠️ 44400, NOT 436xx. The spell-rank rows occupy 43576-44327; reserving illusions at 43600 would
-- have overwritten twelve of them. 44400-44411 is clear and still under RoF2's 45000 spell ceiling.
--
-- ⚠️ Cloned VIA A TEMP TABLE so every column stays byte-identical to stock -- never hand-list the
-- ~236 columns. MariaDB cannot override columns inside INSERT ... SELECT *, so the pattern is
-- "mutate the single-row temp table, insert it, repeat".
-- ⚠️ `items` AND `spells_new` ARE BOTH SHARED MEMORY: world + zones down -> ./shared_memory -> up.
-- ============================================================================================

DELETE FROM items      WHERE id BETWEEN 147930 AND 147953;
DELETE FROM spells_new WHERE id BETWEEN 44400 AND 44411;

-- ---------------------------------------------------------------- 1. the illusions (+30)
-- Cloned from 582 "Illusion: Human" -- SPA 58, self target, 360 tick duration (formula 3).
-- The race is varied per tradeskill purely for flavour; the bonus is paid by id, not by race.
DROP TEMPORARY TABLE IF EXISTS sp;
CREATE TEMPORARY TABLE sp AS SELECT * FROM spells_new WHERE id = 582;

UPDATE sp SET id=44400, name='Guise of the Brineface',     effect_base_value1=9;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44401, name='Guise of the Weeping Fang',  effect_base_value1=6;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44402, name='Guise of the Cogwright',     effect_base_value1=12;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44403, name='Guise of the Inkbound',      effect_base_value1=3;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44404, name='Guise of the Quicksilver',   effect_base_value1=3;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44405, name='Guise of the Hearthfed',     effect_base_value1=11;  INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44406, name='Guise of the Threadbare',    effect_base_value1=4;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44407, name='Guise of the Forge-Wight',   effect_base_value1=8;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44408, name='Guise of the Straightgrain', effect_base_value1=4;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44409, name='Guise of the Sourmash',      effect_base_value1=8;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44410, name='Guise of the Facetwright',   effect_base_value1=5;   INSERT INTO spells_new SELECT * FROM sp;
UPDATE sp SET id=44411, name='Guise of the Kilnborn',      effect_base_value1=10;  INSERT INTO spells_new SELECT * FROM sp;
DROP TEMPORARY TABLE sp;

-- every guise is castable by every class, at level 1, and self only
UPDATE spells_new
   SET classes1=1,classes2=1,classes3=1,classes4=1,classes5=1,classes6=1,classes7=1,classes8=1,
       classes9=1,classes10=1,classes11=1,classes12=1,classes13=1,classes14=1,classes15=1,classes16=1,
       targettype=6, goodEffect=1
 WHERE id BETWEEN 44400 AND 44411;

-- ---------------------------------------------------------------- 2. the worn tools (+20)
-- Cloned from 1001 "Cloth Cap" (head slot, cloth armor). No stats are added: the whole point of the
-- tool is the tradeskill bonus, and giving it combat stats would make it strictly-wear gear rather
-- than something you swap in to craft.
DROP TEMPORARY TABLE IF EXISTS it;
CREATE TEMPORARY TABLE it AS SELECT * FROM items WHERE id = 1001;
UPDATE it SET nodrop=1, norent=1, races=65535, classes=65535, reclevel=0, reqlevel=0;

UPDATE it SET id=147930, Name="Angler's Brim";            INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147931, Name="Venomer's Wrap";           INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147932, Name="Tinker's Goggles";         INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147933, Name="Scrivener's Circlet";      INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147934, Name="Alchemist's Hood";         INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147935, Name="Baker's Toque";            INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147936, Name="Tailor's Pinned Cap";      INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147937, Name="Smith's Soot Hood";        INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147938, Name="Fletcher's Band";          INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147939, Name="Brewer's Cowl";            INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147940, Name="Jeweler's Loupe Harness";  INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147941, Name="Potter's Clay-Caked Cap";  INSERT INTO items SELECT * FROM it;

-- ---------------------------------------------------------------- 3. the masks (clicky illusions)
-- Same clone, re-pointed at the guise spells. clicktype 4 = click from inventory, no level gate.
UPDATE it SET slots=0, clicktype=1, clicklevel=1, clicklevel2=1, casttime=0, casttime_=0;

UPDATE it SET id=147942, Name="Mask of the Brineface",     clickeffect=44400; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147943, Name="Mask of the Weeping Fang",  clickeffect=44401; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147944, Name="Mask of the Cogwright",     clickeffect=44402; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147945, Name="Mask of the Inkbound",      clickeffect=44403; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147946, Name="Mask of the Quicksilver",   clickeffect=44404; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147947, Name="Mask of the Hearthfed",     clickeffect=44405; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147948, Name="Mask of the Threadbare",    clickeffect=44406; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147949, Name="Mask of the Forge-Wight",   clickeffect=44407; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147950, Name="Mask of the Straightgrain", clickeffect=44408; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147951, Name="Mask of the Sourmash",      clickeffect=44409; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147952, Name="Mask of the Facetwright",   clickeffect=44410; INSERT INTO items SELECT * FROM it;
UPDATE it SET id=147953, Name="Mask of the Kilnborn",      clickeffect=44411; INSERT INTO items SELECT * FROM it;
DROP TEMPORARY TABLE it;

-- ---------------------------------------------------------------- verification
SELECT 'tools (expect 12)' AS check_name, COUNT(*) AS n FROM items WHERE id BETWEEN 147930 AND 147941
UNION ALL SELECT 'masks (expect 12)',     COUNT(*) FROM items WHERE id BETWEEN 147942 AND 147953
UNION ALL SELECT 'guises (expect 12)',    COUNT(*) FROM spells_new WHERE id BETWEEN 44400 AND 44411
UNION ALL SELECT 'masks pointing at a real guise (12)', COUNT(*)
  FROM items i JOIN spells_new s ON s.id = i.clickeffect
 WHERE i.id BETWEEN 147942 AND 147953
UNION ALL SELECT 'spell-rank rows still intact (752)', COUNT(*) FROM spells_new WHERE id BETWEEN 43576 AND 44327;

-- tools at skill 100, masks at skill 200, on the EXISTING per-skill achievements.
-- ⚠️ The recipes are removed: the tool is granted, not crafted. A recipe nobody is told about is not
-- a reward, and this server's own recipe search does not fix "I did not know to look".
DELETE FROM tradeskill_recipe_entries WHERE recipe_id BETWEEN 470100 AND 470111;
DELETE FROM tradeskill_recipe         WHERE id        BETWEEN 470100 AND 470111;

DELETE FROM custom_achievement_rewards WHERE id BETWEEN 1200 AND 1299;

-- ⚠️⚠️ ALL TWELVE TRADESKILLS GET BOTH ITEMS, including Fishing (55) and Research (58). They are
-- excluded from the AA ladders only because neither has a Mastery AA to grant -- the skill bonus is
-- paid by AoTv4TradeskillSkill for all twelve, so there is no reason to exclude them here.
-- ⚠️ Achievement id is 400000 + skill*1000 + threshold. The AGGREGATE Master Artisan rows
-- (470050-470300) carry objectives for the same skills at the same values, so anything selecting by
-- (skill, required_count) must exclude 470xxx or it wires rewards onto the wrong achievement.
INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1200 + x.idx, 400000 + x.skill*1000 + 100, 'item', 147930 + x.idx, 1, 100, '', 1, 1,
       CONCAT(x.tool, ' (+20 ', x.sname, ')'), '', 1, 30, UNIX_TIMESTAMP()
FROM (
  SELECT 0 idx,55 skill,'Fishing' sname,"Angler's Brim" tool UNION ALL
  SELECT 1,56,'Make Poison',"Venomer's Wrap"        UNION ALL SELECT 2,57,'Tinkering',"Tinker's Goggles"      UNION ALL
  SELECT 3,58,'Research',"Scrivener's Circlet"      UNION ALL SELECT 4,59,'Alchemy',"Alchemist's Hood"        UNION ALL
  SELECT 5,60,'Baking',"Baker's Toque"              UNION ALL SELECT 6,61,'Tailoring',"Tailor's Pinned Cap"   UNION ALL
  SELECT 7,63,'Blacksmithing',"Smith's Soot Hood"   UNION ALL SELECT 8,64,'Fletching',"Fletcher's Band"       UNION ALL
  SELECT 9,65,'Brewing',"Brewer's Cowl"             UNION ALL SELECT 10,68,'Jewelcrafting',"Jeweler's Loupe Harness" UNION ALL
  SELECT 11,69,'Pottery',"Potter's Clay-Caked Cap"
) x
WHERE EXISTS (SELECT 1 FROM custom_achievements a WHERE a.id = 400000 + x.skill*1000 + 100);

INSERT INTO custom_achievement_rewards
  (id, achievement_id, reward_type, reward_id, amount, chance, tier, claim_once, auto_claim,
   preview_text, data_text, enabled, sort_order, created_at)
SELECT 1250 + x.idx, 400000 + x.skill*1000 + 200, 'item', 147942 + x.idx, 1, 100, '', 1, 1,
       CONCAT(x.mask, ' (+30 ', x.sname, ')'), '', 1, 30, UNIX_TIMESTAMP()
FROM (
  SELECT 0 idx,55 skill,'Fishing' sname,"Mask of the Brineface" mask UNION ALL
  SELECT 1,56,'Make Poison',"Mask of the Weeping Fang"  UNION ALL SELECT 2,57,'Tinkering',"Mask of the Cogwright"  UNION ALL
  SELECT 3,58,'Research',"Mask of the Inkbound"         UNION ALL SELECT 4,59,'Alchemy',"Mask of the Quicksilver" UNION ALL
  SELECT 5,60,'Baking',"Mask of the Hearthfed"          UNION ALL SELECT 6,61,'Tailoring',"Mask of the Threadbare" UNION ALL
  SELECT 7,63,'Blacksmithing',"Mask of the Forge-Wight" UNION ALL SELECT 8,64,'Fletching',"Mask of the Straightgrain" UNION ALL
  SELECT 9,65,'Brewing',"Mask of the Sourmash"          UNION ALL SELECT 10,68,'Jewelcrafting',"Mask of the Facetwright" UNION ALL
  SELECT 11,69,'Pottery',"Mask of the Kilnborn"
) x
WHERE EXISTS (SELECT 1 FROM custom_achievements a WHERE a.id = 400000 + x.skill*1000 + 200);

SELECT 'tool rewards (12)' c, COUNT(*) n FROM custom_achievement_rewards WHERE id BETWEEN 1200 AND 1249
UNION ALL SELECT 'mask rewards (12)', COUNT(*) FROM custom_achievement_rewards WHERE id BETWEEN 1250 AND 1299
UNION ALL SELECT 'recipes removed (0)', COUNT(*) FROM tradeskill_recipe WHERE id BETWEEN 470100 AND 470111
UNION ALL SELECT 'all point at per-skill ach, not aggregate (24)', COUNT(*)
  FROM custom_achievement_rewards WHERE id BETWEEN 1200 AND 1299 AND achievement_id NOT BETWEEN 470000 AND 470999;

-- ⚠️⚠️ clicktype MUST BE 1 (ItemEffectClick), NOT 4. Type 4 is ItemEffectEquipClick, and
-- zone/spells.cpp:7529 refuses it unless the item sits in an EQUIPMENT slot -- but these are slots=0
-- and can never be equipped, so a type-4 mask is silently unusable by everybody. It was 4 for a day.
-- ⚠️⚠️ AND THE MASK MUST NOT BE HEAD SLOT. Making it wearable would put it in the same slot as the
-- TOOL, so the +20 and +30 could never be worn together -- which destroys the whole point of them
-- stacking to +50. Inventory clicky is not a shortcut here, it is what makes the design work.
-- ⚠️ All 24 items are classes=65535 races=65535 reqlevel=0 deity=0: every class, every race.
