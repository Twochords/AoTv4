-- aotv4_spell_books.lua
-- Tomes of Insight: a consumable that grants ONE extra reward pick without levelling you up.
--
-- Three tiers, one per level band (1-10 / 11-20 / 21-30), each usable ONLY inside its own band.
-- Clicking one queues a normal level-up offer; you either take a reward from it, or DECLINE it and
-- have your reroll price cut instead. The book is spent either way.
--
--   drop  -> M.on_npc_death   (per-kill roll, gated on world difficulty and con)
--   click -> M.use            (quests/items/1479xx.lua -> EVENT_ITEM_CLICK)
--   offer -> spell_choice.offer_from_book / spell_choice.decline
--
-- ⚠️⚠️ THE BOOK IS "ONE EXTRA LEVEL-UP PICK", NOT "PICK ANYTHING IN THE BAND". You cannot scribe a
-- spell above your own level, so a tier 1 tome opened at level 3 can only ever offer levels 1-3 --
-- "spells you can use" resolves to "at or below your level", which is exactly what a level-up gives.
-- The band is therefore a gate on CONSUMPTION, not a description of the offer, and that is what lets
-- this whole system reuse the existing picker with no client change: the offer always matches the
-- player's level, so the window has nothing new to say.

local spell_choice = require("spell_choice")

local M = {}

-------------------------------------------------------------------- tiers
-- ⚠️⚠️ THE ITEM IDS ARE MIRRORED IN `custom/sql/aotv4_spell_books.sql` AND IN THE FILE NAMES UNDER
-- `quests/items/`. A quest item script is found by FILE NAME (`<itemid>.lua`), so renumbering here
-- without renaming those files leaves a book that is inert on click with no error anywhere.
-- ⚠️ Reserved band is 147966-147979; 147969+ are free for a future tier.
M.TIERS = {
	[1] = { item = 147966, lo = 1,  hi = 10, name = "Worn Tome of Insight"    },
	[2] = { item = 147967, lo = 11, hi = 20, name = "Etched Tome of Insight"  },
	[3] = { item = 147968, lo = 21, hi = 30, name = "Radiant Tome of Insight" },
}

M.BY_ITEM = {}
for tier, t in pairs(M.TIERS) do M.BY_ITEM[t.item] = tier end

-------------------------------------------------------------------- drops
-- World difficulty -> which tier drops, and how often. Difficulty ids match `aotv4_difficulty.LEVELS`:
-- 0 Normal, 1 Nightmare, 2 Hell, 3 Inferno.
--
-- ⚠️ NORMAL DROPS NOTHING, deliberately. The tome is the reward for choosing a harder world; putting
-- it on the default difficulty would make the whole difficulty ladder optional.
--
-- 📌 Rates descend as the tier rises because the tiers are NOT interchangeable -- tier 3 is the only
-- one usable at the level cap, which is where a character spends most of its life, so it is the rate
-- that actually governs the system long-term.
M.DROP = {
	[1] = { tier = 1, rate = 5.0 },   -- Nightmare
	[2] = { tier = 2, rate = 3.0 },   -- Hell
	[3] = { tier = 3, rate = 1.0 },   -- Inferno
}

-- ⚠️⚠️ CON VALUES ARE NOT ORDERED BY DIFFICULTY -- DO NOT WRITE `con >= White`.
-- From common/emu_constants.h: Green 2, DarkBlue 4, Gray 6, White 10, Red 13, Yellow 15,
-- LightBlue 18, WhiteTitanium 20. LightBlue (18) is NUMERICALLY LARGER than White (10) but is an
-- EASIER creature, so a `>=` test silently lets a capped character farm tomes off trivial mobs --
-- exactly the parking exploit this gate exists to stop. Enumerate what counts instead.
local CON_REWARDS = {
	[10] = true,   -- White
	[13] = true,   -- Red
	[15] = true,   -- Yellow
	[20] = true,   -- WhiteTitanium
}

-- The difficulty of the WORLD THIS KILL HAPPENED IN.
--
-- ⚠️⚠️ IT IS THE ZONE'S DIFFICULTY, NOT THE CHARACTER'S SETTING, AND THE DIFFERENCE IS LOAD BEARING.
-- `zonediff_<charid>` says what the player last chose; `aotv4_difficulty.here()` says which shard
-- the creature actually died in. Those come apart the moment a player walks into an instance that
-- is not a difficulty shard -- a DELVE -- whose own instance carries no difficulty marker. Keying
-- off the character's flag would pay Nightmare tome drops for delve creatures, which are scaled to
-- the player, endless, and already paying a chest.
-- 📌 It also means the reward always matches the world on screen, which is the only version a player
-- can reason about.
function M.difficulty_of(client)
	if not client or not client.valid then return 0 end
	local ok, diff = pcall(function() return require("aotv4_difficulty").here() end)
	return (ok and diff) or 0
end

-- Per-kill drop roll. Called from global_npc.event_death_complete.
--
-- ⚠️ This runs on EVERY npc death on the server, so the cheap integer tests come first and the
-- entity-list lookup for the killer is done by the caller (which already needed it for ink).
function M.on_npc_death(npc, killer)
	if not npc or not npc.valid or not killer or not killer.valid then return end

	local rule = M.DROP[M.difficulty_of(killer)]
	if not rule then return end

	-- ⚠️⚠️ THE CON GATE IS WHAT MAKES 5 PERCENT SAFE. Without it the best play is to park at the top
	-- of a band and farm trivial creatures for tomes, which beats levelling -- and levelling is the
	-- whole loop. It also stops a capped character farming low tiers to hand out (books are No Drop
	-- today, but that is a data decision and this is the mechanical one).
	if not CON_REWARDS[killer:GetLevelCon(npc:GetLevel())] then return end

	-- ⚠️ Plain math.random with no reseed, matching grant_ink_on_kill. Reseeding here would be a
	-- GLOBAL side effect on a per-kill hook -- every other system sharing this stream (the delve, the
	-- world boss, the picker itself) would have its sequence yanked out from under it several times a
	-- second.
	if math.random() * 100.0 >= rule.rate then return end

	local t = M.TIERS[rule.tier]
	if not t then return end

	killer:SummonItem(t.item, 1)
	killer:Message(MT.Yellow, string.format("A %s falls from the corpse.", t.name))
end

-------------------------------------------------------------------- use
-- Consume one tome: queue an extra reward offer.
--
-- ⚠️⚠️ BUILD THE OFFER FIRST, DESTROY THE BOOK SECOND. Exactly the ordering spell_choice.reroll
-- documents for coin: there is no refund path once the item is gone, so consuming first means a
-- player whose band has nothing left to offer eats the tome for nothing. If the two ever get out of
-- step, fail in THIS direction -- a free offer is recoverable, a destroyed tome is not.
--
-- ⚠️⚠️ `client` MUST BE `e.owner`, NOT `e.self` -- IN AN ITEM SCRIPT `e.self` IS THE ITEM.
-- `LuaParser::EventItem` (zone/lua_parser.cpp:726) pushes a **Lua_ItemInst** as `self` and the
-- **Lua_Client** as `owner`. Passing `e.self` therefore hands this function the item instance, and
-- the failure is the section 24 trap in its worst form: `client.valid` is TRUE (a valid item
-- instance), so the guard below passes, and it dies at the first real call with
-- *"attempt to call method 'CharacterID' (a nil value)"* -- a runtime error on click, nothing at
-- load, and the player just sees the inert spell cast with no offer. Observed in play 2026-08-12
-- from both event_item_click AND event_item_click_cast, which is also what proved both paths fire.
-- 📌 `e.slot_id` IS correct on that event (`handle_item_click` sets it from extra_data), so only the
-- client argument was ever wrong.
function M.use(client, slot_id, item_id)
	if not client or not client.valid then return end

	local tier = M.BY_ITEM[item_id]
	local t    = tier and M.TIERS[tier]
	if not t then return end

	-- ⚠️⚠️ ONE CLICK CAN RAISE TWO EVENTS. The item scripts define BOTH `event_item_click` (raised by
	-- Handle_OP_ItemVerifyRequest) and `event_item_click_cast` (raised by the OP_CastSpell path),
	-- because which one arrives depends on the client and the slot -- RoF2 was observed sending only
	-- the cast path, which is why the tome did nothing at all with just the first defined. A client
	-- that sends both would otherwise burn two tomes and queue two offers for a single click.
	-- ⚠️ Stamped before any work so BOTH paths dedupe, including the "nothing left to teach" exit.
	local dk   = "tomeuse_" .. client:CharacterID()
	local now  = os.time()
	if now - (tonumber(eq.get_data(dk)) or 0) < 2 then return end
	eq.set_data(dk, tostring(now))

	-- ⚠️⚠️ THE BAND IS ONE-DIRECTIONAL: being OVER it is fine, being UNDER it is refused.
	--
	--   level 13, Worn (1-10)    -> allowed, drawn from level 10   -- capped by the TOME
	--   level  7, Worn (1-10)    -> allowed, drawn from level  7   -- capped by the PLAYER
	--   level  5, Radiant (21-30)-> REFUSED, and the tome is kept
	--
	-- ⚠️ OVER the band must not refuse. Worn is what Nightmare drops, so a flat rejection meant the
	-- difficulty handed out a reward its own players could not spend once they out-levelled the band.
	-- The tier caps the ceiling instead, which is what makes an out-levelled tome still worth clicking.
	--
	-- ⚠️⚠️ UNDER the band must refuse, and must refuse BEFORE anything is consumed. Capping to the
	-- player's level there would technically produce a usable reward, but it would silently spend a
	-- high tier for a low-tier result -- the player destroys a Radiant tome and receives what a Worn
	-- one would have given. Refusing keeps the tome; there is no way to hand back a consumed one.
	local plvl = client:GetLevel()
	if plvl < t.lo then
		client:Message(MT.Red, string.format(
			"The %s is beyond you at level %d -- its lore is for level %d and above. It is unharmed.",
			t.name, plvl, t.lo))
		return
	end

	local lvl = math.min(t.hi, plvl)

	if not spell_choice.offer_from_book(client, tier, lvl) then
		client:Message(MT.Red, "There is nothing left for the tome to teach you. It is unharmed.")
		return
	end

	-- ⚠️ Quantity 1, not 0. DeleteItemInInventory's third argument 0 means "the whole stack" (see
	-- death_loss), so passing 0 here would burn every tome the player is carrying for one offer.
	client:DeleteItemInInventory(slot_id, 1, true)
	client:Message(MT.Yellow, string.format("You study the %s, and a choice takes shape.", t.name))
end

-------------------------------------------------------------------- info
-- "How much would declining be worth right now" -- used by the refusal messages and by anything that
-- wants to describe a tome. The percentages themselves live in spell_choice, which owns the reroll
-- counter; keeping a second copy here is the drift trap this project has been bitten by repeatedly.
function M.decline_value(client, tier)
	local pct = spell_choice.BOOK_REROLL_CUT[tier] or 0
	return pct, spell_choice.reroll_cost_after_cut(client, pct)
end

return M
