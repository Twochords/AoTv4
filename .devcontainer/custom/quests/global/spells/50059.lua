-- Vengeful Aura (50059)
-- "You surround yourself in a vengeful aura. All physical damage you take is
--  doubled, but the aura strikes the attacker for the final damage they dealt
--  to you for the next 3 attacks."
--
-- The doubling and the retaliation live in aotv4_reactions.on_damage_taken;
-- this script only arms and clears the charge counter.
--
-- Note this ability is a real trade-off, so its duration MUST expire -- see
-- SPELL_REBUILD.md: an earlier server rule forced beneficial buffs permanent,
-- which would have made "damage you take is doubled" a permanent curse.
local rx = require("aotv4_reactions")

function event_spell_effect(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.arm_vengeful(e.target:CastToClient():CharacterID())
	end
end

function event_spell_fade(e)
	if e.target and e.target.valid and e.target:IsClient() then
		rx.clear_vengeful(e.target:CastToClient():CharacterID())
	end
end
