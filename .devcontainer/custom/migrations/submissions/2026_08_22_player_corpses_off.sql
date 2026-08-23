-- @aotv4-migration
-- description: 2026_08_22_player_corpses_off
-- check: SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `rule_values` WHERE (`rule_name` = 'Character:LeaveCorpses' AND `rule_value` <> 'false') OR (`rule_name` = 'Character:LeaveNakedCorpses' AND `rule_value` <> 'false') OR (`rule_name` = 'Character:CorpseDecayTime' AND `rule_value` <> '60000') OR (`rule_name` = 'Character:EmptyCorpseDecayTime' AND `rule_value` <> '60000')
-- condition: match
-- match: pending
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Turn player corpses OFF plus a 1-minute decay backstop. Player-corpse ACCUMULATION triggered a memory-corruption zone crash (Zone::ZoneTimer copying a corrupt std::string in Zone::Process) on this roguelite where deaths are constant: 272 corpses at a 7-day decay piled up and zones began segfaulting (2026-08-22). Removing the accumulation removes the trigger. See CLAUDE.md section 57.
-- notes: The underlying ZoneTimer memory bug is still LATENT: this removes its trigger, not the bug. If zones crash again, capture a symbolized backtrace / build an ASAN zone rather than just restarting.
-- notes: Scoped by rule_name ALONE, deliberately: rule_values has one row per ruleset and fixing only ruleset 1 leaves the others behind, which then apply to whoever is on that ruleset (same trap as MinRangedAttackDist / SpecialEndurancePct).

UPDATE `rule_values` SET `rule_value` = 'false' WHERE `rule_name` = 'Character:LeaveCorpses';
UPDATE `rule_values` SET `rule_value` = 'false' WHERE `rule_name` = 'Character:LeaveNakedCorpses';
UPDATE `rule_values` SET `rule_value` = '60000' WHERE `rule_name` = 'Character:CorpseDecayTime';
UPDATE `rule_values` SET `rule_value` = '60000' WHERE `rule_name` = 'Character:EmptyCorpseDecayTime';
