-- AoTv4: "Artisan's Universal Kit" (item 990061) -- a single custom tradeskill container that opens the
-- native Tradeskill Window (bagtype 12, same as the Mortar & Pestle which reliably opens the full search/
-- combine window and is not item-class/race restricted). We then LINK the class/race-locked tradeskill
-- recipes (Tinkering 57, Alchemy 59, Make Poison 56) to this container so the native window can search +
-- auto-combine them in it. The server's HandleAutoCombine pulls components from inventory and its class/race
-- gates are already Bard-bypassed (tradeskills.cpp), so the combine works for anyone -- no client container
-- gate is ever hit because we drive it through the always-open auto-combine path.
--
-- ⚠️ ITEM ID CEILING: item ids MUST stay <= 1,048,575 (0xFFFFF) -- RoF2 chat links mask the id to 5 hex
--    digits, so a higher id links as garbage AND can desync the client link parser (see CLAUDE.md §10).
--    990061 is chosen deliberately: under the link ceiling, ABOVE the gear-tier band [300000,900000), and
--    BELOW the 1,000,000+ range that aotv4_gear_tiers.sql bulk-deletes on a tier regen. (Was 2000061 --
--    over the ceiling AND inside the gear-tier delete range, so a tier regen would have wiped it.)
--
-- items change -> shared-memory rebuild. tradeskill_recipe_entries load live (no rebuild, zone restart ok).

-- Show EVERY recipe in the search regardless of the searcher's skill. The stock rule hides recipes
-- whose trivial exceeds the char's current skill by > MaxTradeskillSearchSkillDiff (50); since every
-- character is a Bard with 0 base skill in these tradeskills, that would hide almost everything from
-- the Universal Kit's search (e.g. Banded armor, trivial ~90). Rules load at zone BOOT -> zone restart.
UPDATE rule_values SET rule_value = 'false'
WHERE ruleset_id = 1 AND rule_name = 'Skills:UseLimitTradeskillSearchSkillDiff';

-- 0) migrate any old copies / links off the over-ceiling id (safe no-op if 2000061 never existed live)
UPDATE inventory    SET item_id = 990061 WHERE item_id = 2000061;
UPDATE sharedbank   SET item_id = 990061 WHERE item_id = 2000061;
UPDATE object_contents SET itemid = 990061 WHERE itemid = 2000061;
UPDATE tradeskill_recipe_entries SET item_id = 990061 WHERE item_id = 2000061;
DELETE FROM items WHERE id = 2000061;

-- 1) the custom container (copy the Mortar & Pestle, enlarge to 10 giant slots, keep all-class/all-race)
DROP TEMPORARY TABLE IF EXISTS tmp_kit;
CREATE TEMPORARY TABLE tmp_kit AS SELECT * FROM items WHERE id = 17904;
UPDATE tmp_kit SET
	id        = 990061,
	Name      = "Artisan's Universal Kit",
	bagslots  = 10,
	bagsize   = 4,     -- GIANT: fits any component
	bagwr     = 0,
	loregroup = -1,    -- LORE: one per character
	nodrop    = 1;     -- match the Mortar (droppable flag)
DELETE FROM items WHERE id = 990061;
INSERT INTO items SELECT * FROM tmp_kit;
DROP TEMPORARY TABLE tmp_kit;

-- 2) link EVERY enabled recipe to the Kit (add it as a valid container) so it's a truly universal
--    tradeskill container -- one item for all skills. iscontainer row with all counts 0 = "this recipe may
--    be combined in item 990061".
INSERT INTO tradeskill_recipe_entries (recipe_id, item_id, successcount, failcount, componentcount, salvagecount, iscontainer)
SELECT r.id, 990061, 0, 0, 0, 0, 1
FROM tradeskill_recipe r
WHERE r.enabled = 1
  AND NOT EXISTS (
	SELECT 1 FROM tradeskill_recipe_entries e
	WHERE e.recipe_id = r.id AND e.item_id = 990061 AND e.iscontainer = 1
  );

-- 3) sell the Kit on every merchant that already stocks the Refining Crucible (2000060), at each
--    merchant's next free slot. merchantlist loads at ZONE BOOT -> restart zones after. Idempotent
--    (the DELETE clears any prior kit rows first).
DELETE FROM merchantlist WHERE item = 990061;
INSERT INTO merchantlist (merchantid, slot, item)
	SELECT ml.merchantid, MAX(ml.slot) + 1, 990061
	FROM merchantlist ml
	WHERE ml.merchantid IN (SELECT DISTINCT merchantid FROM merchantlist WHERE item = 2000060)
	GROUP BY ml.merchantid;
