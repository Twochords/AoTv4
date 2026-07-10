-- AoTv4: always send a target's buffs to the client, including WILD NPCs. Without this the server only
-- syncs target buffs for GM/Leadership-AA/players/player-pets, so debuff ICONS on enemy NPCs fall off the
-- target window on any refresh (the debuff itself still persists server-side). Rules load at zone boot.
UPDATE rule_values SET rule_value='true' WHERE rule_name='Spells:AlwaysSendTargetsBuffs';
