-- Open Wounds (50052)
-- "Slices your target open, instantly dealing 300% of both weapons dps and
--  attacks on this target for the next minute will cause them to take 25%
--  additional bleed damage over the next 5 ticks"
--
-- Two halves: an immediate hit for 300% of COMBINED weapon DPS, and a one-minute
-- marker (helper spell 50150) that turns every subsequent hit into bleed. The
-- banking and payout live in aotv4_reactions.
local ab = require("aotv4_abilities")
local rx = require("aotv4_reactions")

local PCT = 300   -- of primary + secondary DPS combined

function event_spell_effect(e)
	local caster = ab.caster(e)
	if not caster or not e.target or not e.target.valid then
		return
	end

	-- "both weapons dps" -- combined, so a two-hander or an empty offhand still
	-- contributes via the bare-hand fallback rather than halving the ability.
	local dps  = ab.weapon_dps(caster, ab.SLOT_PRIMARY) + ab.weapon_dps(caster, ab.SLOT_SECONDARY)
	local base = math.floor(dps * PCT / 100)
	if base < 1 then base = 1 end
	caster:DoMeleeSkillAttackDmg(e.target, base, ab.SKILL_OFFENSE, 0)

	-- and open the wound
	caster:SpellFinished(rx.OPEN_WOUNDS_MARK, e.target)
end
