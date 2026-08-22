-- @aotv4-migration
-- description: 2026_08_22_class_ability_descriptions
-- check: SELECT `value` FROM `db_str` WHERE `id` = 44747 AND `type` = 6
-- condition: missing
-- match: up to 18
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Every class ability description rewritten against what the row and the Lua payload actually do.

-- ⚠️⚠️ THESE WERE FLAVOUR TEXT, NOT DESCRIPTIONS. Every one was directionally true and none of them
-- carried a number: no amounts, no durations, no charges, no scaling. "Draw a measure of vigour back
-- into yourself" is a heal for twice your level; "guards itself worse for it" is 25 armor for 30
-- seconds. A player could not tell any of these apart from doing nothing, which is exactly how three
-- separate working abilities came to be reported as broken this week.
--
-- ⚠️ Each line below was checked against BOTH halves, because behaviour is split:
--     the spell row      SPA, base, formula, duration, numhits  (spells_new)
--     the Lua payload    swings, riders, cooldown cuts          (aotv4_class_abilities.lua)
-- A description taken from either one alone is wrong for about half the set -- the swings are all
-- Lua and invisible in the row, and the durations and debuffs are all row and invisible in the Lua.
--
-- ⚠️⚠️ NO APOSTROPHES AND NO PERCENT SIGNS. The validator refuses an unescaped apostrophe (it has
-- aborted a migration part way before) and the description path is printf-style, so a literal percent
-- is eaten as a format token (section 14). Hence "a tenth" rather than "10 percent" throughout.
--
-- ⚠️ Scaling is written as "X plus Y per level" because that is what `formula` 1-99 means -- the
-- number in the row is only the base. Caps are stated where `max` is non-zero.
--
-- 📌 THE CLIENT WILL NOT SEE THIS UNTIL `./export_client_files` RUNS and the regenerated
-- `dbstr_us.txt` ships (section 6). The spellbook reads the client file, not this table.
-- 📌 The window text lives in `lua_modules/aotv4_class_ability_desc.lua` and is updated in the same
-- commit; both come from the spec in `custom/tools/gen_class_abilities.py`, which is also updated so
-- a regen cannot revert them.
DELETE FROM db_str WHERE id BETWEEN 44700 AND 44747 AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
 (44700, 6, 'A full weapon swing that also seizes attention, adding threat equal to 6 times your level.'),
 (44701, 6, 'Brace behind your guard for 60 seconds. The next 5 melee hits against you are each reduced by a tenth of your armor class, and any blow smaller than that is stopped outright.'),
 (44702, 6, 'A full weapon swing against every enemy within 30 feet in front of you that is already fighting you. Adds no threat of its own. Each target struck cuts Bulwark by 3 seconds, up to 9.'),
 (44703, 6, 'A full weapon swing that heals you for twice your level.'),
 (44704, 6, 'Heals your whole group for a sixth of their own maximum health, then the same again when it fades 18 seconds later.'),
 (44705, 6, 'A blast of judgement dealing 20 plus 4 per level, to a maximum of 250. Cuts Sanctuary by 5 seconds, or by 10 against the undead.'),
 (44706, 6, 'A weapon swing at four fifths damage, plus divine damage equal to your level or a tenth of your Strength, whichever is greater. That part ignores armor. Adds threat equal to 8 times your level.'),
 (44707, 6, 'Heals every group member in your zone for a quarter of YOUR maximum health. Alone, it heals you.'),
 (44708, 6, 'A full weapon swing that stuns your target, plus threat equal to 6 times your level. Cuts Hand of Conviction by 5 seconds.'),
 (44709, 6, 'Two weapon swings at three fifths damage each.'),
 (44710, 6, 'An archery attack at four fifths damage against every enemy within 60 feet in front of you that is already fighting you.'),
 (44711, 6, 'Fires your bow at a target in melee range and cuts Volley by 10 seconds. With no bow you drive the blow home instead, at one and a fifth weapon damage, and cut it by 5.'),
 (44712, 6, 'A full weapon swing that drains twice your level in health. Doubled while Reaving Vow is up, which spends it.'),
 (44713, 6, 'A weapon swing at seven tenths damage against every enemy within 40 feet that is already fighting you, draining twice your level in health for each one struck.'),
 (44714, 6, 'A full weapon swing that swears your next Reaving Strike to drain double, for 18 seconds. Cuts Harrowing by 6 seconds.'),
 (44715, 6, 'A full weapon swing that wreathes your target in thorns for 30 seconds. Every melee blow it lands wounds it for 8.'),
 (44716, 6, 'Mends the target for 12 plus your level every tick, for 10 ticks.'),
 (44717, 6, 'A lance of light dealing 25 plus 4 per level, to a maximum of 300. Against a target that is rooted, snared, mezzed or stunned it deals a further 6 times your level and cuts Wildgrowth by 10 seconds instead of 5.'),
 (44718, 6, 'A hand to hand strike for your weapon delay times 0.2, plus 0.035 more for each level. A slower weapon hits harder.'),
 (44719, 6, 'Raises your avoidance by half until 4 melee blows land on you, or 60 seconds, whichever comes first.'),
 (44720, 6, 'A weapon swing at one and three fifths damage, plus threat equal to 4 times your level. Cuts Void Stance by 6 seconds.'),
 (44721, 6, 'A full weapon swing that strips 25 armor from your target for 30 seconds.'),
 (44722, 6, 'Restores 100 plus 3 per level endurance to your whole group.'),
 (44723, 6, 'A weapon swing at one and a fifth damage, or one and a half with an instrument in your range slot. The next 5 blows landed on your target hurt it a tenth more. Cuts Crescendo by 5 seconds.'),
 (44724, 6, 'A full weapon swing that slows your target to under two thirds speed for 24 seconds.'),
 (44725, 6, 'Opens a bleed for 18 plus your level every tick, for 8 ticks.'),
 (44726, 6, 'A full weapon swing, half again as hard if Rupture is already bleeding on the target. Cuts Rupture by 5 seconds.'),
 (44727, 6, 'Wards the target against 3 plus twice your level of damage for 6 seconds. Anything that strikes the ward is slowed for 2 ticks.'),
 (44728, 6, 'Wards your whole group against 10 plus 4 per level of damage each, for 60 seconds. Anything that strikes a ward is slowed harder, for 2 ticks.'),
 (44729, 6, 'Saps 25 attack and 20 each of Strength, Intelligence and Charisma from your target for 36 seconds. Cuts Crippling Spirit by 5 seconds, or by 10 against something already slowed.'),
 (44730, 6, 'A full weapon swing that leaves rot behind, dealing 10 plus your level every tick for 6 ticks.'),
 (44731, 6, 'Spends every affliction on your target at once. They deal all of their remaining damage immediately and are consumed, and you are healed for half of that.'),
 (44732, 6, 'Deals a twentieth of the damage your target afflictions still have left to give, or a quarter of it if that total would already kill them. Cuts Soul Harvest by 15 seconds.'),
 (44733, 6, 'A weapon swing at seven tenths damage that returns twice your level in mana.'),
 (44734, 6, 'Pours everything into one cast for 40 plus 20 per level, to a maximum of 900, then stuns you for 2 seconds. Each gathered ley thread adds a further 3 times your level.'),
 (44735, 6, 'A bolt for 15 plus 3 per level, to a maximum of 200, that gathers a ley thread. Threads stack to 3 and are spent by your next Overload. Cuts Overload by 5 seconds.'),
 (44736, 6, 'A full weapon swing. If you have no pet, an elemental guardian answers, and it is rescaled to your level as you grow.'),
 (44737, 6, 'Calls a brief host of servants to fight beside you.'),
 (44738, 6, 'A burst of cinders for 30 plus 3 per level, to a maximum of 250. If your pet is already fighting that target it deals a further 3 times your level. Cuts Elemental Swarm by 7 seconds.'),
 (44739, 6, 'Thins every magical ward your target has, lowering all resists by 15 for 48 seconds. Physical resistance is unaffected.'),
 (44740, 6, 'Restores 100 plus 3 per level mana to your whole group.'),
 (44741, 6, 'Your spells land for 30 more damage for 3 ticks. Cuts Gift of Thought by 5 seconds.'),
 (44742, 6, 'A full weapon swing. If you have no pet, a feral companion answers, and it is rescaled to your level as you grow.'),
 (44743, 6, 'You and your companion attack a quarter faster and mend 10 plus your level every tick, for 60 seconds.'),
 (44744, 6, 'A full weapon swing, or four fifths harder against a target below half health. Cuts Feral Frenzy by 5 seconds, or by 10 on wounded prey.'),
 (44745, 6, 'A weapon swing at one and two fifths damage that costs you a hundredth of your maximum health.'),
 (44746, 6, 'Five weapon swings at half damage each.'),
 (44747, 6, 'A full weapon swing. Cuts Frenzied Onslaught by 2 seconds for every tenth of your own health that is missing, up to 18.');
