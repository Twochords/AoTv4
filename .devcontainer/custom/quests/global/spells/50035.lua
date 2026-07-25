-- Blade Turn (50035)
-- "Protects you from 1 physical attack, recharging every 30 seconds. Consumes 5
--  endurance to refresh if the charge is used."
--
-- The absorb and the recharge live in aotv4_reactions.on_damage_taken; this
-- script arms the charge so the buff starts ready rather than spent.
local rx = require("aotv4_reactions")

function event_spell_effect(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.arm_blade(e.target:CastToClient():CharacterID())
	end
end

function event_spell_fade(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.clear_blade(e.target:CastToClient():CharacterID())
	end
end
