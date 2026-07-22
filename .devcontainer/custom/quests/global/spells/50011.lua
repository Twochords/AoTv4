-- Strike (50011)
-- "Swing your weapon at your target, Dealing 750% primary dps."
local ab = require("aotv4_abilities")

local PCT = 750

local function strike(e)
	local caster = ab.caster(e)
	if caster then
		ab.pct_dps_attack(caster, e.target, PCT, ab.SLOT_PRIMARY, ab.SKILL_OFFENSE)
	end
end

-- one handler covers both client and NPC targets
function event_spell_effect(e) strike(e) end
