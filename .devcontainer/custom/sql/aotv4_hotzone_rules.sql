-- AoTv4: Hot Zones give 2x experience (reverted from 3x). The native path is
--   modifier = Character:ExpMultiplier (0.65); if IsHotzone(): modifier += Zone:HotZoneBonus
-- so HotZoneBonus = ExpMultiplier (0.65) -> 0.65 + 0.65 = 1.30 = 2x the normal rate. (Was 1.30 = 3x.)
-- Keep HotZoneBonus = Character:ExpMultiplier. Rules load at boot -> restart zones/world.
UPDATE rule_values SET rule_value = '0.65' WHERE ruleset_id = 1 AND rule_name = 'Zone:HotZoneBonus';
