-- @aotv4-migration
-- description: 2026_08_22_start_zone_freeporttheater
-- check: SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `start_zones` WHERE `zone_id` <> 390
-- condition: match
-- match: pending
-- shared-memory: no
-- band:
-- author: Claude
-- notes: New characters start in Freeport Theater (390), origin (-83,-235,-27) NE of Wayfinder Alessa (the built hub). See CLAUDE.md section 57.
-- notes: GetStartZone (world/worlddb.cpp) matches WHERE zone_id = <SoFStartZoneID> for SoF+/RoF2 clients, so BOTH the World:SoFStartZoneID rule AND every start_zones row must be 390, or new chars fall back to the hardcoded default Crescent Reach (394). That mismatch (rows=729 while the client sent something else) is exactly what sent players to Crescent before this fix.
-- notes: The check keys on start_zones because a full DB import resets it (to stock home cities / 729); when it does, this re-applies. SoFStartZoneID needs a WORLD RESTART to take effect; start_zones is read live per character creation.

UPDATE `rule_values` SET `rule_value` = '390' WHERE `rule_name` = 'World:SoFStartZoneID';
UPDATE `start_zones` SET `zone_id` = 390, `start_zone` = 390, `x` = -83, `y` = -235, `z` = -27, `heading` = 0, `bind_id` = 390, `bind_x` = -83, `bind_y` = -235, `bind_z` = -27 WHERE `zone_id` <> 390;
