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
-- `spent` = the AA VALUE (sum of owned rank*cost across the pool) that UNLOCKS this era server-wide.
-- AAs are NOT era-gated anymore -- the whole pool is offered randomly (prereqs still enforced) from the
-- start. Expansions open smoothly as the first player's accumulated AA value crosses these thresholds.
M.ERA = {
	[0] = { name = "Classic",                  expansion = 0, cap = 50, cap_exp = 164708600,  spent = 0    },
	[1] = { name = "The Ruins of Kunark",      expansion = 1, cap = 60, cap_exp = 616137000,  spent = 220  },
	[2] = { name = "The Scars of Velious",     expansion = 2, cap = 60, cap_exp = 616137000,  spent = 485  },
	[3] = { name = "The Shadows of Luclin",    expansion = 3, cap = 60, cap_exp = 616137000,  spent = 740  },
	[4] = { name = "The Planes of Power",      expansion = 4, cap = 65, cap_exp = 812646400,  spent = 990  },
	[5] = { name = "The Legacy of Ykesha",     expansion = 5, cap = 65, cap_exp = 812646400,  spent = 1260 },
	[6] = { name = "Lost Dungeons of Norrath", expansion = 6, cap = 65, cap_exp = 812646400,  spent = 1540 },
	[7] = { name = "Gates of Discord",         expansion = 7, cap = 65, cap_exp = 812646400,  spent = 1820 },
	[8] = { name = "Omens of War",             expansion = 8, cap = 70, cap_exp = 1018377900, spent = 2100 },
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

-- AA VALUE the player has accumulated = sum of (owned rank * cost) across the whole pool. This is the
-- deterministic "spent" metric that drives expansion unlocks -- native GetSpentAA mismeasures because
-- the picker grants AAs free (ignore_cost). Robust: reflects exactly what the player owns.
function M.spent(client)
	local total = 0
	for _, aa in ipairs(aapool[1]) do
		total = total + (client:GetAAByAAID(aa.id) or 0) * (aa.cost or 0)
	end
	return total
end

-- CATCH-UP bonus applied to death-AA banking: +CATCHUP_PER_ERA for each unlocked expansion the player's
-- AA VALUE is still short of. Lets latecomers close the gap on an advanced server without touching the
-- leaders' rate (a caught-up player, or a Classic-only server, gets nothing).
local CATCHUP_PER_ERA = 0.25
function M.catchup_bonus(client)
	local cur = M.current()
	if cur < 1 then return 1.0 end
	local mine, behind = M.spent(client), 0
	for era = 1, cur do
		if mine < (M.ERA[era].spent or 0) then behind = behind + 1 end
	end
	return 1.0 + behind * CATCHUP_PER_ERA
end

-- Best-effort: point THIS zone's content-expansion rule at the unlocked era. Runtime expansion
-- checks pick it up; a FULL content open (zones/items that filter at zone BOOT) still needs the
-- rule persisted in rule_values + a world restart -- see AOTV4_EXPANSION_PLAN.md 5d / custom SQL.
function M.sync_zone()
	eq.set_rule("Expansion:CurrentExpansion", tostring(M.def(M.current()).expansion))
end

-- Called after a successful AA pick (aa_choice.handle_say). Advance the server-wide era while the
-- player's accumulated AA VALUE has crossed the next expansion's `spent` threshold. Cascades if a
-- single player vaults past several thresholds at once. First to cross opens it for everyone.
function M.check_unlock(client)
	local era = M.current()
	local spent = M.spent(client)
	while era < MAX_ERA and spent >= (M.ERA[era + 1].spent or math.huge) do
		era = era + 1
		eq.set_data(BUCKET, tostring(era))
		local d = M.def(era)
		eq.set_rule("Expansion:CurrentExpansion", tostring(d.expansion))
		eq.world_emote(15, string.format(
			"%s has pushed the age forward!  %s is now open to all -- the level cap rises to %d!",
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
