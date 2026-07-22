-- Snipe (50015)
-- "Take time to carefully aim your shot at your target, Deals the same damage
--  your bow would do in 9 seconds and has 1.5x accuracy."
--
-- "damage your bow would do in 9 seconds" = ranged DPS x 9, i.e. 900% of range
-- slot DPS. The 1.5x accuracy becomes a to-hit bonus (chance_mod) on the attack
-- roll; chance_mod is additive to GetTotalToHit, not a multiplier, so ACCURACY
-- below is a tuning knob rather than a literal 1.5x -- raise it if snipes miss
-- more than intended.
local ab = require("aotv4_abilities")

local PCT      = 900   -- 9 seconds of bow DPS
local ACCURACY = 50    -- stand-in for "1.5x accuracy"

local function snipe(e)
	local caster = ab.caster(e)
	if caster then
		ab.pct_dps_attack(caster, e.target, PCT, ab.SLOT_RANGE, ab.SKILL_ARCHERY, ACCURACY)
	end
end

-- one handler covers both client and NPC targets
function event_spell_effect(e) snipe(e) end
