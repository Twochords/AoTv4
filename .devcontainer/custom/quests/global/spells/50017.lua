-- Languid Healing (50017)
-- "Heals your target for 10 points after a 2 tick delay"
--
-- The spell row carries a 2-tick duration that does nothing on its own; the heal
-- lands when that duration runs out. Using FADE rather than counting tics means
-- the delay is exactly the buff's lifetime, and an early dispel correctly
-- cancels the pending heal... except that FADE also fires on dispel, so we check
-- tics_remaining to distinguish "expired naturally" from "removed early".
local HEAL = 10

function event_spell_fade(e)
	local mob = e.target
	if not mob or not mob.valid then
		return
	end
	-- only pay out when the buff ran its full course
	if e.tics_remaining and e.tics_remaining > 0 then
		return
	end
	mob:HealDamage(HEAL)
end
