-- The Left (50054)
-- "Feignt your target with your primary weapon to hit them with your offhand
--  weapon, dealing 1000% of secondary dps."
--
-- Scales off the OFFHAND. With an empty offhand this falls back to the bare-hand
-- basis in aotv4_abilities rather than doing nothing, so the ability is never a
-- dead pick -- it is just weak until you equip a second weapon.
local ab = require("aotv4_abilities")

local PCT = 1000

local function the_left(e)
	local caster = ab.caster(e)
	if caster then
		ab.pct_dps_attack(caster, e.target, PCT, ab.SLOT_SECONDARY, ab.SKILL_OFFENSE)
	end
end

-- one handler covers both client and NPC targets
function event_spell_effect(e) the_left(e) end
