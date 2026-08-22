-- @aotv4-migration
-- description: 2026_08_22_disable_aa_exp_slowdown
-- check: SELECT `rule_value` FROM `rule_values` WHERE `rule_name` = 'AoT:AAExpSlowdownEnabled'
-- condition: match
-- match: true
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Turns off the AA experience slowdown. Owner decision.

-- ⚠️⚠️ THE HEADER DEFAULT IS ALREADY `false` AND THAT CHANGES NOTHING. ruletypes.h ships
-- `RULE_BOOL(AoT, AAExpSlowdownEnabled, false, ...)`, but a `rule_values` row OVERRIDES the header,
-- and this database carries `true`. Section 22 records the same trap for AoT:SpecialEndurancePct:
-- editing the default alone is invisible on any server that has a row. Always check
--     SELECT rule_value FROM rule_values WHERE rule_name = '<rule>';
-- before believing what ruletypes.h says the value is.

-- ⚠️ Scoped by rule_name ALONE, deliberately. `rule_values` can hold one row per ruleset_id
-- (section 35), and fixing only ruleset 1 leaves any other ruleset still slowing experience down.
UPDATE rule_values SET rule_value = 'false' WHERE rule_name = 'AoT:AAExpSlowdownEnabled';

-- 📌 The Base and Factor rows are left alone on purpose: they are inert while the switch is off, and
-- keeping them preserves the tuning if it is ever turned back on.
