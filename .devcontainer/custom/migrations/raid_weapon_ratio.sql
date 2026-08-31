-- @aotv4-migration
-- description: 2026_08_31_raid_weapon_ratio_cap
-- check: SELECT id FROM items WHERE id = 148608 AND damage = 74
-- condition: empty
-- shared-memory: yes
-- author: Claude
-- notes: "I dont want the weapon to have a better ratio than a mythic titanwrought."
--   The BASE ratios were already well under it -- 2.10 to 2.22 against 3.438 -- so this looked fine
--   and was not. These are base ids, so NPC::ResolveTierDrop rolls each one 5 percent Mythic (§10),
--   and a Mythic DOUBLES damage while leaving delay alone (§35): 92/43 becomes 184/43, a ratio of
--   4.28, comfortably past the best Mythic Titanwrought in the game.
--   ⚠️⚠️ THE CEILING TO COMPARE AGAINST IS THE MYTHIC, NEVER THE BASE. Any weapon that can drop is
--   really three weapons, and only the top one matters for "is this too strong". Checking the printed
--   base ratio is exactly how this passed review the first time.
--   Base ratio is now ~1.70, so Mythic lands at 3.36-3.40 against the 3.438 ceiling -- at the top of
--   what crafting can produce without beating it, which is the intended relationship between a raid
--   drop and the crafted endgame.
--   📌 Mythic Titanwrought reference: 650627/650628 at 110/32 = 3.438; the reclevel-25 Rough tier
--   already reaches 3.314, so this is measured against gear a level 30 character can actually wear.
--   ⚠️ AFTER THIS, RE-RUN custom/sql/aotv4_gear_tiers.sql THEN aotv4_craft_sockets.sql THEN
--   ./shared_memory. The tier rows are DERIVED from base damage and are stale until regenerated --
--   652543 already exists at the old value (§35).

UPDATE items SET damage = 76 WHERE id = 52543;    -- Dark Master Blade       76/45 -> Mythic 3.378
UPDATE items SET damage = 71 WHERE id = 148602;   -- Phinigel Coral Trident  71/42 -> Mythic 3.381
UPDATE items SET damage = 73 WHERE id = 148605;   -- Mayong Bloodfang        73/43 -> Mythic 3.395
UPDATE items SET damage = 74 WHERE id = 148608;   -- Velketor Frozen Scepter 74/44 -> Mythic 3.364
