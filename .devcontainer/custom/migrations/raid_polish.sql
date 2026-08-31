-- @aotv4-migration
-- description: 2026_08_31_raid_spell_drops_and_mold_names
-- check: SELECT id FROM items WHERE id = 148340 AND Name LIKE '%Titanwrought%'
-- condition: empty
-- shared-memory: yes
-- author: Claude
-- notes: Two reports from the first raid tests.
--   (1) "Spells are dropping off some of the raids". Velketor's stock loottable 121 carries lootdrop
--   281, which is 25 items and ALL 25 are spell scrolls. On this server spells come from the level-up
--   picker and the rank system (§3, §29), so a dropped scroll is both junk and a route around the
--   reward system. Removed as ONE loottable_entries row -- table 121 belongs to Velketor alone
--   (checked: 1 npc uses it), so no shared content is touched and lootdrop 281 itself is left intact
--   in case anything else ever wants it.
--   (2) "the molds are not showing up in the tradeskills tab so not clear what they are used for".
--   The recipes were always findable -- enabled, not must_learn, min/max_expansion -1 -- but they are
--   named for their OUTPUT ("Rough Titanwrought Cloth Cap") while the mold was named "Rough Cloth Cap
--   Mold". Nothing in the player's bags said the word Titanwrought, so searching the recipe window for
--   what you are holding found nothing. Renaming the molds to carry it makes the mold self-describing
--   and makes one search term reach the component AND the recipe that eats it.
--   ⚠️ Longest resulting name is 45 chars against items.Name varchar(64), so nothing truncates --
--   §26 records that a silent truncation there can also collapse two names into one.

-- (1) Velketor stops dropping spell scrolls.
DELETE FROM loottable_entries WHERE loottable_id = 121 AND lootdrop_id = 281;

-- (2) "Rough Cloth Cap Mold" -> "Rough Titanwrought Cloth Cap Mold".
-- ⚠️ Keyed on NOT already containing the word so a re-run cannot double-insert it.
UPDATE items
   SET Name = CONCAT(
        SUBSTRING_INDEX(Name, ' ', 1), ' Titanwrought ',
        TRIM(SUBSTRING(Name, CHAR_LENGTH(SUBSTRING_INDEX(Name, ' ', 1)) + 2)))
 WHERE id BETWEEN 148200 AND 148409
   AND Name NOT LIKE '%Titanwrought%';
