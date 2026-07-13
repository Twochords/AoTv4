-- AoTv4: combat rule tweaks.
--
--   Combat:DoubleBackstabLevelRequirement 55 -> 50 : the DOUBLE backstab (and the TRIPLE, which is
--     nested inside it) is gated behind this level. At the Classic cap of 50 the stock 55 was
--     unreachable, so double/triple backstab NEVER fired even with the Triple Backstab AA. Lowering to
--     50 lets it fire at cap; from BEHIND a double rolls on CheckDoubleAttack() (Double Attack skill)
--     and a triple rolls on top per the AA's chance. Rules load at zone/world BOOT -> restart to apply.
UPDATE rule_values SET rule_value = '50'
WHERE ruleset_id = 1 AND rule_name = 'Combat:DoubleBackstabLevelRequirement';
