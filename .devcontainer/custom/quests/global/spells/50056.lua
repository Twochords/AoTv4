-- Counterattack (50056)
-- "You get ready to take advantage of openings, when you are attacked, you
--  strike back with 250% of primary dps if the target is in front of you for
--  the next 5 attacks."
--
-- The retaliation itself lives in aotv4_reactions.on_damage_taken (wired into
-- global_player); this script only arms and clears the charge counter.
local rx = require("aotv4_reactions")

function event_spell_effect(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.arm_counterattack(e.target:CastToClient():CharacterID())
	end
end

function event_spell_fade(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.clear_counterattack(e.target:CastToClient():CharacterID())
	end
end
