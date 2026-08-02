-- aotv4_kindred.lua -- announce the Kindred group-heal proc (43330-43335 / procs 43374-43379).
--
-- ⚠️⚠️ THIS IS THE ONE CUSTOM LINE WHOSE PAYOFF LANDS ON SOMEBODY ELSE. Every other line heals,
-- damages or restores the person who cast it, so the health bar moving is its own evidence. Kindred
-- is a self buff whose weapon proc heals the WHOLE GROUP (SPA 85 -> a targettype 41 heal, see
-- section 5), which means the swinger has no way at all to tell it fired -- the numbers all happen
-- on other people's screens. It reads as a buff that does nothing.
--
-- ⚠️ The heal itself is paid by the ENGINE, unlike thirst/moonfire/sinew. Nothing here grants
-- anything; this module only reports what already happened. If it is removed the line still works,
-- it just goes silent again.
--
-- ⚠️ EVENT_SPELL_EFFECT fires once PER TARGET, so a six-person group produces six calls -- one per
-- player, each messaging only themselves. That is one line each, not six lines each. Do not "fix"
-- this into a single broadcast from the caster: the caster cannot know who was actually in range.
--
-- ⚠️ Chat::Spells is the same filterable channel the thirst, moonfire and sinew lines use, so every
-- custom line announces itself the same way and one filter setting covers all of them.
--
-- 📌 EVENT_SPELL_EFFECT_CLIENT has a real argument builder (it is one of the four that always did --
-- see section 10), so e.caster_id is populated here. The NPC variants were the broken ones, and a
-- beneficial group heal only ever lands on players, so this path was never affected.

local M = {}

function M.on_group_heal(e, amount)
	if not e or not amount or amount <= 0 then
		return
	end

	-- e.self is whoever the heal landed on. Only players are ever in a group heal, but guard anyway:
	-- a mercenary or pet in range would otherwise error on Message.
	local healed = e.self
	if not healed or not healed.valid or not healed:IsClient() then
		return
	end

	-- The swinger is in their own group, so they get a call too -- and theirs is the one that has to
	-- read differently, because for them the point is "my proc fired", not "I was healed".
	if e.caster_id and e.caster_id ~= 0 and healed:GetID() == e.caster_id then
		healed:Message(MT.Spells, string.format(
			"Your kindred flame flares, healing your group for %d hit points.", amount))
		return
	end

	local caster = e.caster_id and e.caster_id ~= 0
		and eq.get_entity_list():GetMobID(e.caster_id) or nil
	local who = (caster and caster.valid) and caster:GetCleanName() or "A kindred flame"

	healed:Message(MT.Spells, string.format(
		"%s's kindred flame heals you for %d hit points.", who, amount))
end

return M
