-- Duel (50030)
-- "Forces you and your target into a one on one duel. While in the duel,
--  participants have melee damage is doubled and neither participant can flee
--  the fight, cast spells, be healed, or be damaged by anything outside of the
--  duel. The duel ends after 6 ticks or when one of the participants dies."
--
-- Split between stock spell data and Lua:
--   * no casting  -> Silence (SPA 96)   } both carried by the duel lock,
--   * no healing  -> HealRate -100      } helper spell 50155
--   * doubled melee between the pair, and immunity to everyone else
--                 -> aotv4_reactions.on_damage_taken
--   * ends on death -> global_player event_death clears the pairing
--
-- The lock is applied to BOTH participants and the pairing is registered in both
-- directions, so the duel can never end up half-open (one side immune, the other
-- still takeable).
local ab = require("aotv4_abilities")
local rx = require("aotv4_reactions")

function event_spell_effect(e)
	local caster = ab.caster(e)
	if not caster or not e.target or not e.target.valid then
		return
	end
	if caster:GetID() == e.target:GetID() then
		return                                  -- can't duel yourself
	end

	rx.start_duel(caster:GetID(), e.target:GetID())

	-- lock both sides; the caster's copy is what expires the duel after 6 tics
	caster:SpellFinished(rx.DUEL_LOCK, caster)
	caster:SpellFinished(rx.DUEL_LOCK, e.target)
end
