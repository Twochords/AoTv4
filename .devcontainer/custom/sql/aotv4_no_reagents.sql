-- AoTv4: remove all reagent/component costs.
--
-- SPELLS: reagent/component requirements and consumption are removed in code (zone/spells.cpp -- the
-- CheckReagents/consume block is short-circuited), since stock EQEmu has no global rule for it.
--
-- COMBAT ABILITIES: thrown weapons (e.g. berserker throwing axes) and archery arrows are no longer
-- consumed. EQEmu already gates this behind rules -- just flip them off. Rules load at zone boot, so a
-- zone restart applies them (no shared-memory rebuild).

UPDATE rule_values SET rule_value = 'false'
 WHERE ruleset_id = 1
   AND rule_name IN ('Combat:ThrowingConsumesAmmo', 'Combat:ArcheryConsumesAmmo');

-- Bot variants (bots are used on this server) -- keep them consistent. Insert if absent, else update.
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes)
 SELECT 1, r.rule_name, 'false', 'AoTv4: no ammo consumption'
 FROM (SELECT 'Bots:BotThrowingConsumesAmmo' AS rule_name
       UNION ALL SELECT 'Bots:BotArcheryConsumesAmmo') r
 WHERE NOT EXISTS (SELECT 1 FROM rule_values rv WHERE rv.ruleset_id = 1 AND rv.rule_name = r.rule_name);
UPDATE rule_values SET rule_value = 'false'
 WHERE ruleset_id = 1
   AND rule_name IN ('Bots:BotThrowingConsumesAmmo', 'Bots:BotArcheryConsumesAmmo');
