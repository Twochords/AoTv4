-- ============================================================================================
-- AoTv4 -- give the pet ward buffs descriptions that say what they DO (2026-08-02)
--
-- The ten wards (43420-43429, zone/aotv4_pet_aa.cpp) already had flavour text, and only flavour:
--   "Heat rolls off it in waves, and whatever strikes it is burned for the trouble."
-- Evocative, but a player inspecting their pet's buff could not learn that it is a damage shield,
-- let alone how big. The mechanic is now stated after the flavour, with the real numbers read out
-- of the spell rows rather than described from memory.
--
-- ⚠️⚠️ SPA 69 IS `TotalHP` -- MAXIMUM HEALTH -- NOT DAMAGE REDUCTION. The comment in
-- zone/aotv4_pet_aa.cpp calls the Magician earth ward "flat damage reduction" and that is simply
-- wrong: 43422 Stoneflesh is SPA 69 base 10, i.e. +10 maximum health, and the generic ward 43429
-- Bound Servant is the same effect at +5. The generator text in gen_stock_pool.pl was copied from
-- that comment and inherited the error; both are corrected. Fix the C++ comment if it is ever
-- touched -- the code is right, only the comment lies.
--
-- ⚠️ SPA 11 base is the RESULTING melee speed, not the bonus (CLAUDE.md section 5): 110 means the
-- pet attacks at 110 percent, i.e. 10 percent faster. Lower is slower.
-- ⚠️ SPA 59 damage shields carry a NEGATIVE base by native convention (section 14); -1 is a shield
-- of 1, not minus one.
--
-- ⚠️ NO '%' CHARACTERS. Client::Message is printf-style and a stray % renders as garbage in the
-- client and can eat the bytes after it (section 14). Spelled out as "percent" throughout.
--
-- ⚠️ These are `db_str` type 6 rows, which the CLIENT resolves from its own dbstr_us.txt -- so this
-- needs ./export_client_files and the refreshed file installed before players see it. The server
-- itself needs nothing beyond the usual shared-memory rebuild for spells_new (untouched here).
-- ============================================================================================

UPDATE db_str SET value = CONCAT(value, ' Damage shield: attackers take 1 damage per hit.')
  WHERE id = 43420 AND type = 6 AND value NOT LIKE '%Damage shield:%';

UPDATE db_str SET value = CONCAT(value, ' Regeneration: heals 1 health every tick.')
  WHERE id = 43421 AND type = 6 AND value NOT LIKE '%Regeneration:%';

UPDATE db_str SET value = CONCAT(value, ' Fortitude: increases maximum health by 10.')
  WHERE id = 43422 AND type = 6 AND value NOT LIKE '%Fortitude:%';

UPDATE db_str SET value = CONCAT(value, ' Haste: attacks 10 percent faster.')
  WHERE id = 43423 AND type = 6 AND value NOT LIKE '%Haste:%';

UPDATE db_str SET value = CONCAT(value, ' Lifetap: heals for 8 percent of the melee damage it deals.')
  WHERE id = 43424 AND type = 6 AND value NOT LIKE '%Lifetap:%';

UPDATE db_str SET value = CONCAT(value, ' Ferocity: melee damage increased by 8 percent.')
  WHERE id = 43425 AND type = 6 AND value NOT LIKE '%Ferocity:%';

UPDATE db_str SET value = CONCAT(value, ' Rune: absorbs the first 5 damage of each hit.')
  WHERE id = 43426 AND type = 6 AND value NOT LIKE '%Rune:%';

UPDATE db_str SET value = CONCAT(value, ' Evasion: 1 percent additional chance to avoid melee.')
  WHERE id = 43427 AND type = 6 AND value NOT LIKE '%Evasion:%';

UPDATE db_str SET value = CONCAT(value, ' Clarity: grants 1 mana every tick.')
  WHERE id = 43428 AND type = 6 AND value NOT LIKE '%Clarity:%';

UPDATE db_str SET value = CONCAT(value, ' Fortitude: increases maximum health by 5.')
  WHERE id = 43429 AND type = 6 AND value NOT LIKE '%Fortitude:%';

SELECT id, value FROM db_str WHERE id BETWEEN 43420 AND 43429 AND type = 6 ORDER BY id;
