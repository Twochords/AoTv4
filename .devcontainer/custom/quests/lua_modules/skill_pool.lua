-- skill_pool.lua
-- The ONLY skills that rotate through the level-up reward picker: class-specific ACTIVATED
-- combat abilities (signature moves inherited from other classes). Everything passive --
-- weapon proficiencies, Offense/Defense, Dodge/Parry/Riposte/Block, Double/Triple Attack,
-- Dual Wield, Intimidation -- is auto-learned instead (see global_player.lua free_skills) and
-- never appears here.
--
--   key  = EQ skill id (see the skill table in global_player.lua)
--   name = display name shown in the choice window
--   icon = spellbook icon index (203 = combat/fist, 139 = bash, 202 = disarm, 49 = frenzy)
--
-- These are reward-GATED on the client (areSkillsUnlocked): a skill here stays hidden until
-- the player picks it. MUST stay in sync with SKILLUNLOCK_COMBAT_IDS in
-- eq-core-dll/src/core_spellwindow.cpp.
return {
	[8]  = { name = "Backstab",     icon = 203 },
	[10] = { name = "Bash",         icon = 139 },
	[30] = { name = "Kick",         icon = 203 },
	[16] = { name = "Disarm",       icon = 202 },
	[73] = { name = "Taunt",        icon = 203 },
	[72] = { name = "Berserking",   icon = 203 },
	[74] = { name = "Frenzy",       icon = 49 },
	[21] = { name = "Dragon Punch", icon = 203 },
	[23] = { name = "Eagle Strike", icon = 203 },
	[26] = { name = "Flying Kick",  icon = 203 },
	[38] = { name = "Round Kick",   icon = 203 },
	[52] = { name = "Tiger Claw",   icon = 203 },
}
