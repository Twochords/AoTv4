-- era_system.lua
-- Server-wide expansion progression. The FIRST character to MAX every AA in the current era's tier
-- unlocks the next expansion for EVERYONE: the level cap rises (live), the AA pool widens (live, via
-- aa_choice's era filter), and the content-expansion rule advances. See AOTV4_EXPANSION_PLAN.md.
--
-- Source of truth: a GLOBAL data bucket "aotv4_era" (no charid) = the unlocked era index 0..8.
-- Every zone reads it, so level/spell/AA gating is consistent server-wide and survives restarts.

local aapool = require("aa_pool")

local M = {}

-- era 0..8. cap = level cap; cap_exp = GetEXPForLevel(cap) (the death-bank reference, for tuning);
-- expansion = Expansion:CurrentExpansion value for content; name = broadcast label.
M.ERA = {
	[0] = { name = "Classic",                  expansion = 0, cap = 50, cap_exp = 164708600 },
	[1] = { name = "The Ruins of Kunark",      expansion = 1, cap = 60, cap_exp = 616137000 },
	[2] = { name = "The Scars of Velious",     expansion = 2, cap = 60, cap_exp = 616137000 },
	[3] = { name = "The Shadows of Luclin",    expansion = 3, cap = 60, cap_exp = 616137000 },
	[4] = { name = "The Planes of Power",      expansion = 4, cap = 65, cap_exp = 812646400 },
	[5] = { name = "The Legacy of Ykesha",     expansion = 5, cap = 65, cap_exp = 812646400 },
	[6] = { name = "Lost Dungeons of Norrath", expansion = 6, cap = 65, cap_exp = 812646400 },
	[7] = { name = "Gates of Discord",         expansion = 7, cap = 65, cap_exp = 812646400 },
	[8] = { name = "Omens of War",             expansion = 8, cap = 70, cap_exp = 1018377900 },
}
local MAX_ERA = 8
local BUCKET  = "aotv4_era"   -- GLOBAL (no charid) -> server-wide unlocked era

function M.current()
	return tonumber(eq.get_data(BUCKET)) or 0
end

function M.def(era)
	return M.ERA[era] or M.ERA[0]
end

function M.level_cap()
	return M.def(M.current()).cap
end

-- Has `client` maxed EVERY AA in eras <= `era`? (i.e. each such AA is at its max picker rank `mr`).
-- This is the authoritative maxout test -- robust against the picker's floored-cost accounting and
-- escalating native rank costs (which GetSpentAA would mismeasure).
local function era_maxed(client, era)
	for _, aa in ipairs(aapool[1]) do
		if aa.era <= era and (client:GetAAByAAID(aa.id) or 0) < aa.mr then
			return false
		end
	end
	return true
end

-- The lowest era whose AAs the player has NOT fully maxed = the tier they're still working on.
-- (If they've maxed every unlocked tier, this returns the current server era = fully caught up.)
function M.player_era(client)
	local cur = M.current()
	for era = 0, cur do
		if not era_maxed(client, era) then return era end
	end
	return cur
end

-- CATCH-UP bonus applied to death-AA banking: +CATCHUP_PER_ERA for each era the server has unlocked
-- BEYOND the player's own AA tier. Once Kunark opens, anyone still finishing Classic AAs banks +25%;
-- two eras behind banks +50%; a caught-up player gets nothing (and a Classic-only server = no bonus
-- for anyone). Lets latecomers close the gap without touching the leaders' rate.
local CATCHUP_PER_ERA = 0.25
function M.catchup_bonus(client)
	local behind = M.current() - M.player_era(client)
	if behind < 1 then return 1.0 end
	return 1.0 + behind * CATCHUP_PER_ERA
end

-- Best-effort: point THIS zone's content-expansion rule at the unlocked era. Runtime expansion
-- checks pick it up; a FULL content open (zones/items that filter at zone BOOT) still needs the
-- rule persisted in rule_values + a world restart -- see AOTV4_EXPANSION_PLAN.md 5d / custom SQL.
function M.sync_zone()
	eq.set_rule("Expansion:CurrentExpansion", tostring(M.def(M.current()).expansion))
end

-- Called after a successful AA pick (aa_choice.handle_say). If the picker has maxed the whole
-- current era and we're not at the top, advance the server-wide era and announce. Cascades if a
-- single character has somehow maxed multiple tiers (still gated by their own death-spending).
function M.check_unlock(client)
	local era = M.current()
	while era < MAX_ERA and era_maxed(client, era) do
		era = era + 1
		eq.set_data(BUCKET, tostring(era))
		local d = M.def(era)
		eq.set_rule("Expansion:CurrentExpansion", tostring(d.expansion))
		eq.world_emote(15, string.format(
			"%s has mastered the arts of the age!  %s is now open to all -- the level cap rises to %d!",
			client:GetName(), d.name, d.cap))
	end
end

-- Clamp a character to the current era's level cap (called from event_level_up). Returns true if it
-- clamped (caller skips the spell offer). We also hold exp at the cap threshold so the engine does
-- not bounce-level every kill, and so a death at the cap banks the tuned amount (not an ever-growing
-- pile from grinding at cap).
function M.clamp_level(client)
	local cap = M.level_cap()
	if client:GetLevel() > cap then
		client:SetLevel(cap)
		local d = M.def(M.current())
		if d.cap_exp and (client:GetEXP() or 0) > d.cap_exp then
			client:SetEXP(d.cap_exp, client:GetAAExp())
		end
		return true
	end
	return false
end

return M
