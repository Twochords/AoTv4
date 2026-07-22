-- Armor Rend (50055)
-- "Slam your weapon onto your opponent dealing 1250% primary dps and decreasing
--  their ac by 5% of damage dealt for 1.2 minutes."
--
-- The AC debuff is proportional to the damage, but a spell's base value is
-- STATIC -- there is no way to hand Lua's number to a buff. So we compute the
-- damage ourselves, take 5% of it, and apply the closest rung of a small ladder
-- of fixed AC debuffs (helpers 50151-50154). Approximate, but it scales with
-- your weapon instead of being a flat constant.
--
-- Because we need the number, this uses Damage() with a self-computed value
-- rather than DoMeleeSkillAttackDmg (which applies the hit roll but returns
-- nothing). Trade-off: Armor Rend always connects.
local ab = require("aotv4_abilities")

local PCT    = 1250   -- of primary DPS
local AC_PCT = 5      -- AC reduction as a % of damage dealt

-- (minimum AC reduction represented, helper spell id) -- keep in step with
-- ARMOR_REND_LADDER in gen_spells.py
local LADDER = { {5, 50151}, {15, 50152}, {40, 50153}, {100, 50154} }

function event_spell_effect(e)
	local caster = ab.caster(e)
	if not caster or not e.target or not e.target.valid then
		return
	end

	local damage = math.floor(ab.weapon_dps(caster, ab.SLOT_PRIMARY) * PCT / 100)
	if damage < 1 then damage = 1 end
	e.target:Damage(caster, damage, e.spell_id, ab.SKILL_OFFENSE, false)

	-- pick the largest rung the damage has actually earned
	local reduction = math.floor(damage * AC_PCT / 100)
	local chosen    = nil
	for _, rung in ipairs(LADDER) do
		if reduction >= rung[1] then
			chosen = rung[2]
		end
	end
	if chosen then
		caster:SpellFinished(chosen, e.target)
	end
end
