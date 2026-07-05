-- aotv4_epic_unlock.sql
-- Make every EPIC weapon usable by ALL classes and fittable in Primary / Secondary / Ranged.
-- On this Bard-only server the epic quests are open to everyone, so their reward weapons must not be
-- class/race locked and must drop into any weapon slot.
--
-- Epics are identified by items.epicitem<>0 (same key as epic_items.lua). This covers the base epics
-- PLUS our generated Hallowed (+1,000,000) and Mythic (+2,000,000) tiers, so all three stay in step.
--
-- Augmentations (itemtype 54, the 2.0 "Shard of the Ancients") are intentionally left untouched:
-- they slot INTO a weapon, not into a weapon slot, and are already all-class. Everything else that
-- is epicitem<>0 (1H/2H weapons, hand-to-hand, instruments, and the two shield epics) is opened up.
--
-- Slot bitmask 26624 = Primary(1<<13=8192) + Secondary(1<<14=16384) + Range(1<<11=2048).
--
-- NOTE: items live in SHARED MEMORY. Apply this SQL, then with world DOWN:
--   cd build/bin && ./shared_memory   (then restart world; zones reboot on demand)
-- Reversible via aotv4_epic_backup.

CREATE TABLE IF NOT EXISTS aotv4_epic_backup AS SELECT * FROM items WHERE epicitem<>0;

-- 1) usable by all classes and races
UPDATE items SET classes = 65535, races = 65535
 WHERE epicitem <> 0 AND itemtype <> 54;

-- 2) fit Primary + Secondary + Ranged
UPDATE items SET slots = 26624
 WHERE epicitem <> 0 AND itemtype <> 54;

-- 3) re-type 2-handers to their 1-hand equivalents so they physically fit the secondary slot and can
--    be dual-wielded. (2H weapons are hard-blocked from Secondary by IsType2HWeapon(), regardless of
--    the slot mask.)  1HSlash=0  1HBlunt=3  1HPiercing=2   <-  2HSlash=1  2HBlunt=4  2HPiercing=35
UPDATE items SET itemtype = 0 WHERE epicitem <> 0 AND itemtype = 1;   -- 2H Slash  -> 1H Slash
UPDATE items SET itemtype = 3 WHERE epicitem <> 0 AND itemtype = 4;   -- 2H Blunt  -> 1H Blunt
UPDATE items SET itemtype = 2 WHERE epicitem <> 0 AND itemtype = 35;  -- 2H Pierce -> 1H Pierce

-- 4) downscale the FORMER 2-handers' damage to a 1H-appropriate level so they aren't overpowered now
--    that they can be dual-wielded. Cap damage at ratio 1.00 (damage/delay) = the native 1H-epic
--    average; downscale-only (never buff). Weapons already <=1.0 (Fiery Defender, Innoruuk's Curse,
--    Jagged Blade) are untouched -- their raw damage was never inflated (their power came from the 2H
--    damage bonus, which the type change removes).
--    BASE ids only (< 1,000,000): the Hallowed(+1M)/Mythic(+2M) tiers store damage as base x2, so
--    capping the base keeps the tiers correctly scaled -- do NOT cap the tier rows themselves.
--    "was 2H" is read from the pre-change backup (backup.itemtype = 1).
UPDATE items i
  JOIN aotv4_epic_backup b ON b.id = i.id
  SET i.damage = LEAST(i.damage, ROUND(i.delay * 1.00))
  WHERE b.itemtype = 1 AND i.id < 1000000;
