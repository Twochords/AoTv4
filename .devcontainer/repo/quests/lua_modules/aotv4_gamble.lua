-- aotv4_gamble.lua -- the Gilded Wager. Hand over coin, receive a random wearable you could have
-- found yourself, at the quality you paid for.
--
-- ⚠️⚠️ THE GAMBLE IS *WHICH ITEM*, NOT WHAT QUALITY. Three fixed prices buy three fixed tiers:
--     1000p  -> a base item          5000p -> Hallowed          10000p -> Mythic
-- There is no quality roll. That is deliberate: a random tier on top of a random item is two
-- lotteries stacked, and the player cannot tell a bad roll from a bad pool.
--
-- ⚠️⚠️ IT CAN ONLY EVER PAY OUT WHAT YOU COULD ALREADY REACH. Two gates, both of them the point:
--     REGION -- only items obtainable in a region this character has unlocked. The casino is a
--               shortcut through the grind, never around the progression that gates the grind.
--     LEVEL  -- nothing above the character's own level, so 1000p at level 5 cannot buy endgame gear.
--
-- ⚠️ The pool is GENERATED (custom/tools/gen_gamble_pool.py) because Lua cannot query the database.
-- Re-run it whenever loot tables, merchant lists or zone_regions change.

local pool = require("aotv4_gamble_pool")

local M = {}

-- ⚠️ 257 is Chat::Tell (common/eq_constants.h:112). Every line this NPC sends goes to ONE player on
-- the tell channel, never through npc:Say -- a casino announcing your business to the whole zone is
-- noise for everyone else and a privacy problem for the player. eq.whisper was the other candidate
-- and was not used: it renders on EchoChat as "X whispers", which does not land in the tell window.
local MT = { Yellow = 15, Red = 13, Green = 4, Tell = 257 }

-- ⚠️⚠️ FORMATTED AS A REAL TELL so it obeys the player's own tell filter and window. The NPC name is
-- taken from the mob rather than hardcoded, so renaming him in npc_types cannot leave this lying.
function M.tell(npc, c, text)
	c:Message(MT.Tell, string.format("%s tells you, '%s'", npc:GetCleanName(), text))
end

-- ⚠️⚠️ A SILENT SAYLINK. Non-silent makes the player SAY the phrase out loud, which in a starting hub
-- is both chat spam and an announcement that they are carrying 10000 platinum. Section 11 records the
-- same choice for the Reforger menus.
-- 📌 The link TEXT is what arrives back in event_say, so it has to be the command; the label is what
-- the player sees. Two different strings on purpose.
function M.price_link(plat, label)
	return eq.say_link("wager " .. plat, true, label)
end

-- ⚠️⚠️ THE TIER STEP IS MIRRORED FROM zone/aotv4_tiers.h AND FROM THE POOL GENERATOR. Section 10
-- records five C++ copies of this constant already; this is the Lua one. If the step ever moves,
-- every one of them moves together or the casino hands out ids in a band where no item exists and
-- SummonItem silently gives nothing.
M.TIER_STEP = 300000

-- price in platinum -> how many tier steps the prize is lifted by
M.PRICES = {
	[1000]  = { step = 0, label = "as it is found" },
	[5000]  = { step = 1, label = "Hallowed" },
	[10000] = { step = 2, label = "Mythic" },
}

M.MIN_REGION, M.MAX_REGION = 1, 6

-- Everything this character could win right now.
-- ⚠️ Built fresh on every wager rather than cached: regions unlock and levels rise mid-session, and a
-- cached list would quietly keep paying out the pool the character had when they first gambled.
function M.candidates(c)
	local out, level = {}, c:GetLevel()
	for region = M.MIN_REGION, M.MAX_REGION do
		if c:HasRegion(region) then
			local list = pool[region]
			if list then
				for _, e in ipairs(list) do
					if e[2] <= level then out[#out + 1] = e[1] end
				end
			end
		end
	end
	return out
end

-- ⚠️⚠️ THE ONLY PLACE COIN IS TAKEN, AND THE ORDER IS LOAD BEARING: identify the price, build the
-- pool, THEN keep the money. Every other gate in this codebase that charges for something learned
-- the same lesson the hard way (spell_choice.reroll, the Tome of Insight) -- there is no refund path
-- once the coin is gone, so nothing may be charged for an outcome that might not exist.
-- 📌 Returning the coin is the DEFAULT. Every path that does not hand over a prize hands back the
-- money, including the ones that are the player getting it wrong.
function M.wager(npc, c, trade)
	local plat = tonumber(trade.platinum) or 0
	local other = (tonumber(trade.gold) or 0) + (tonumber(trade.silver) or 0) + (tonumber(trade.copper) or 0)

	local price = M.PRICES[plat]
	if not price or other > 0 then
		return false, "The wager is 1000, 5000 or 10000 platinum, and nothing else in the window."
	end

	if not c:HasRegion(M.MIN_REGION) and not M.any_region(c) then
		return false, "You have opened no region yet. I deal only in things you could have found yourself."
	end

	local pick = M.candidates(c)
	if #pick == 0 then
		return false, "Nothing in the lands open to you would suit someone of your level. Come back stronger."
	end

	local base = pick[math.random(#pick)]
	local id   = base + price.step * M.TIER_STEP

	-- ⚠️ The generator guarantees both tier rows exist for every entry, so this cannot normally fail.
	-- It is checked anyway: a half-regenerated pool would otherwise consume 10000p and hand back
	-- nothing at all, which is the single worst outcome this NPC can have.
	if not eq.get_item_stat(id, "id") or eq.get_item_stat(id, "id") == 0 then
		return false, "The wheel caught. Your coin is returned."
	end

	c:SummonItemExact(id)                 -- see the note in M.wager_coin: SummonItem would upgrade it
	return true, string.format("%s -- %s.", eq.get_item_name(id) or "Something", price.label), #pick
end

-- The SAYLINK path: coin comes straight out of the purse, prize goes straight to the cursor.
-- ⚠️⚠️ GATE ORDER, and it is the same order every charging feature here has had to learn: identify
-- the price, check the purse, BUILD THE POOL, and only then take the coin. TakeMoneyFromPP has no
-- refund path (section 3), so nothing may be charged for an outcome that might not exist.
-- ⚠️⚠️ TakeMoneyFromPP AND GetCarriedMoney BOTH WORK IN COPPER, and this NPC prices in platinum --
-- 1 platinum is 1000 copper. Reading the price as copper would charge a thousandth of it.
-- ⚠️ GetCarriedMoney is CARRIED coin only, never the bank. The refusal says so, because "you cannot
-- afford it" while standing on 50000 platinum in the bank reads as a bug.
function M.wager_coin(npc, c, plat)
	local price = M.PRICES[plat]
	if not price then
		M.tell(npc, c, "I take 1000, 5000 or 10000 platinum. Nothing between.")
		return false
	end

	local cost_copper = plat * 1000
	if (c:GetCarriedMoney() or 0) < cost_copper then
		M.tell(npc, c, string.format(
			"You are not carrying %d platinum. Coin in the bank is no use to me here.", plat))
		return false
	end

	local pick = M.candidates(c)
	if #pick == 0 then
		M.tell(npc, c, "Nothing in the lands open to you would suit someone of your level. Your coin stays yours.")
		return false
	end

	local base = pick[math.random(#pick)]
	local id   = base + price.step * M.TIER_STEP
	if not eq.get_item_stat(id, "id") or eq.get_item_stat(id, "id") == 0 then
		M.tell(npc, c, "The wheel caught on that one. Your coin stays yours.")
		return false
	end

	-- ⚠️ Charged LAST, and only once the prize is known to exist.
	if not c:TakeMoneyFromPP(cost_copper, true) then
        M.tell(npc, c, "Your coin would not leave your hand. Nothing was taken.")
		return false
	end

	-- ⚠️⚠️ SummonItemEXACT, NEVER SummonItem. Every ordinary Lua SummonItem overload runs the id
	-- through AoTv4MythicReward, which upgrades anything below 300000 to its Mythic tier so QUEST
	-- rewards hand out top gear (section 10). Here the tier IS the price, so that upgrade handed out
	-- Mythics for 1000 platinum -- and only for 1000, because the check is `< 300000` and the
	-- Hallowed and Mythic ids pass through untouched. Reported as the cheap option being bugged.
	c:SummonItemExact(id)                 -- lands on the cursor, at exactly the tier that was paid for
	M.tell(npc, c, string.format("The wheel gives you %s -- %s.",
		eq.get_item_name(id) or "something", price.label))
	c:Message(15, string.format("Drawn from %d prizes in the lands open to you.", #pick))
	return true
end

function M.any_region(c)
	for r = M.MIN_REGION, M.MAX_REGION do
		if c:HasRegion(r) then return true end
	end
	return false
end

return M
