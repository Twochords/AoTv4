-- aotv4_thirst.lua -- flat heal-per-melee-hit for the Thirst line (43342-43347).
--
-- The line heals a SET AMOUNT per hit, not a percentage. That cannot be done with the native
-- mechanic: SPA 178 MeleeLifetap is percentage-only by construction --
--     lifetap_amt = damage * (melee_lifetap_mod / 100.0f);      (zone/mob.cpp:6860)
-- there is no flat variant, and nothing else fires on every successful melee hit. So the spell rows
-- are inert markers (a buff with an icon and a duration and no mechanical effect) and the healing
-- happens here, off global_player's event_damage_given.
--
-- CHARGES are handled NATIVELY, not here: the spell rows carry numhits + numhitstype 5 ("Outgoing
-- Hit Successes"), so the engine decrements the buff on each successful melee hit and fades it at
-- zero (Mob::CheckNumHitsRemaining, zone/spell_effects.cpp:7075; fired from CommonOutgoingHitSuccess,
-- zone/attack.cpp:6690). The client shows the counter for free. Native precedent: the Blade
-- Attunement line (4644/4646/4648) is likewise an inert buff with numhits 100 and type 5.
--
-- ⚠️ The charge and the heal are counted in DIFFERENT places -- the engine decrements in
-- CommonOutgoingHitSuccess, we heal from event_damage_given in Mob::Damage. They line up on a normal
-- swing, but a hit fully absorbed to 0 damage still spends a charge while paying no heal.
--
-- ⚠️ THIS RUNS ON EVERY DAMAGE EVENT FOR EVERY PLAYER. Keep it cheap and keep the early-outs first.
-- The ordering below matters: the arithmetic filters (damage, spell_id, flags) reject the vast
-- majority of events before we ever touch the buff array.

local M = {}

-- spell id -> hit points healed per melee hit. Must match the tier values in
-- custom/sql/aotv4_thirst_line.sql; the SQL is documentation only, this table is what pays out.
local HEAL = {
	[43342] = 2,    -- Faint Thirst          L8
	[43343] = 4,    -- Rising Thirst         L18
	[43344] = 6,    -- Keen Thirst           L28
	[43345] = 8,    -- Ravening Thirst       L38
	[43346] = 10,   -- Savage Thirst         L48
	[43347] = 12,   -- Unquenchable Thirst   L58
}

-- Ordered high tier first: only one tier can be up at a time (they share a spellgroup), so the
-- first hit is the answer and we stop looking.
local ORDER = { 43347, 43346, 43345, 43344, 43343, 43342 }

-- ================================================================================================
-- Telling the player it is working
-- ================================================================================================
-- The heal is paid silently by Lua, so unlike a real proc there is no engine message and no combat
-- log line -- the only evidence is the health bar moving a few points during a fight that is also
-- taking chunks out of it. In practice that is invisible, which is what prompted this.
--
-- ⚠️⚠️ IT CANNOT BE ONE LINE PER HIT, EVEN THOUGH THAT IS WHAT A PROC LOOKS LIKE. A proc fires
-- once in a while; THIS FIRES ON EVERY SUCCESSFUL SWING. With dual wield and double attack that is
-- roughly 3-4 events a second, so a per-hit message is not a proc message, it is a wall of text that
-- buries everything else in the chat window. The heal is accumulated instead and reported once per
-- MSG_THROTTLE seconds with the running total, which reads as a proc and stays legible.
--
-- ⚠️ Chat::Spells is the channel real proc and spell messages use, so it inherits the player's
-- existing filter -- anyone who does not want it can already turn it off without a server change.
local MSG_ENABLED  = true
local MSG_THROTTLE = 3        -- seconds between lines. 0 = one line per hit (expect spam).

-- charid -> { amount = healed since the last line, last = os.time() of that line }.
-- ⚠️ Keyed by CharacterID, and only ever touched AFTER the buff lookup has succeeded -- that is the
-- rare path, so the cost never lands on the ordinary swings this file is warned about.
local acc = {}

local function report(client, amount)
	if not MSG_ENABLED then return end

	local id  = client:CharacterID()
	local now = os.time()
	local a   = acc[id]

	if not a then
		a = { amount = 0, last = 0 }
		acc[id] = a
	end

	a.amount = a.amount + amount

	if now - a.last < MSG_THROTTLE then return end

	client:Message(MT.Spells, string.format(
		"Your thirst is slaked, drawing %d hit points from your foe%s.",
		a.amount, a.amount == amount and "" or "s"))

	a.amount = 0
	a.last   = now
end

function M.on_damage_given(e, client)
	-- Cheapest rejections first, before any buff lookup.
	if not e or not e.damage or e.damage <= 0 then return end
	if e.spell_id and e.spell_id > 0 and e.spell_id < 65535 then return end  -- spell damage, not a swing
	if e.is_damage_shield then return end                                    -- our own DS reflecting
	if e.is_buff_tic then return end                                         -- a DoT ticking
	if not client or not client.valid then return end

	for i = 1, #ORDER do
		local id = ORDER[i]
		if client:FindBuff(id) then
			client:HealDamage(HEAL[id])
			report(client, HEAL[id])
			return
		end
	end
end

return M
