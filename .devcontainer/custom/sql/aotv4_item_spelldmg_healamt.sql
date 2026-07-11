-- AoTv4: apply item SpellDmg AND HealAmt regardless of the spell's learn level. EQEmu normally gates the
-- item spelldmg/healamt bonus to spells whose learn level is within 5 of the caster's level
-- (spells[].classes[class] >= level-5). On a Bard server the spells are learned at low levels while you
-- out-level them, so the bonus contributed ZERO. This rule (which also covers healing per effects.cpp:472)
-- removes that restriction. The bonus still scales by cast time + is capped at half the spell's base
-- (GetExtraSpellAmt) -- flip Spells:FlatItemExtraSpellAmt too if you want the full unscaled value.
UPDATE rule_values SET rule_value='true' WHERE rule_name='Spells:IgnoreSpellDmgLvlRestriction';
