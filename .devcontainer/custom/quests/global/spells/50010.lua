-- Kick (50010)
-- "Kick your target, interrupting them and dealing your boots AC as damage"
--
-- Damage basis is the AC of the boots in the feet slot, not a weapon -- so this
-- is the one ability that rewards armour rather than weapons. Barefoot still
-- kicks, for a minimum of 1.
local ab = require("aotv4_abilities")

local BAREFOOT_DAMAGE = 1

local function kick(e)
	local caster = ab.caster(e)
	if not caster then
		return
	end

	local boots = ab.equipped(caster, ab.SLOT_FEET)
	local base  = boots and boots:AC() or BAREFOOT_DAMAGE
	if base < BAREFOOT_DAMAGE then base = BAREFOOT_DAMAGE end

	caster:DoMeleeSkillAttackDmg(e.target, base, ab.SKILL_KICK, 0)

	-- the interrupt half: stops whatever they were casting
	if e.target and e.target.valid then
		e.target:InterruptSpell()
	end
end

-- one handler covers both client and NPC targets
function event_spell_effect(e) kick(e) end
