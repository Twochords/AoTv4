-- Minor Regeneration (50028)
-- "Heals your group for 3 points and draining 5 mana per tick."
--
-- Per tick: EVERY group member is healed 3, and the CASTER pays a flat 5 mana --
-- 5 total, not 5 per member. So 4 ticks with any group size = 12 HP to everyone
-- for 20 mana total.
--
-- That flat cost is why the spell is SELF-targeted (see gen_spells.py) even
-- though it is a group effect: a group-targeted buff tics once per member, which
-- would bill the caster 5 x group size every tick. One buff on the caster means
-- one tick, one drain, and the healing is distributed here instead.
local HEAL      = 3
local MANA_COST = 5

-- Group member slots are 0..5 and may be sparse, so walk the whole range and
-- validity-check rather than trusting GroupCount as a dense upper bound
-- (zone/lua_group.cpp:132).
local MAX_GROUP_SLOT = 5

function event_spell_buff_tic(e)
	local mob = e.target
	if not mob or not mob.valid or not mob:IsClient() then
		return
	end
	local caster = mob:CastToClient()

	-- the caster funds the whole group; no mana means no regen this tick
	local mana = caster:GetMana()
	if mana < MANA_COST then
		return
	end
	caster:SetMana(mana - MANA_COST)

	local group = caster:GetGroup()
	if not group or not group.valid then
		caster:HealDamage(HEAL)          -- ungrouped: a group of one
		return
	end

	for i = 0, MAX_GROUP_SLOT do
		local member = group:GetMember(i)
		if member and member.valid then
			member:HealDamage(HEAL)
		end
	end
end
