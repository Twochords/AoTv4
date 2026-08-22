-- The Gilded Wager (npc 2000403) -- pay coin, receive a random wearable.
-- =================================================================================================
-- ⚠️⚠️ THIS LIVES IN quests/global/ AND IS NAMED BY NPC **ID**, NOT BY ZONE OR NAME, BECAUSE THE NPC
-- IS PLACED BY HAND AND CAN STAND ANYWHERE. QuestParserCollection::GetQIByNPCQuest searches
-- quests/<zone>/<id|name> first and then **quests/global/<id|name>**
-- (quest_parser_collection.cpp:1097), so a global file by id is found in every zone at once.
-- ⚠️ By ID rather than by name: the name carries a leading '#' and a rename would silently orphan
-- the script -- the NPC would still spawn, still be hailable, and simply never answer.
-- 📌 So DO NOT move this into a zone folder. Doing so makes the wager work in exactly one zone and
-- fail silently everywhere else.
--
-- ⚠️⚠️ EVERY LINE IS A TELL, NEVER npc:Say. A casino that announces your business to the whole zone
-- is noise for everyone else and tells the room you are carrying 10000 platinum. gamble.tell formats
-- a real tell so it obeys the player's own filter and window.
local gamble = require("aotv4_gamble")

local function player(e) return eq.get_entity_list():GetClientByID(e.other:GetID()) end

local function menu(npc, c)
	gamble.tell(npc, c, "Coin for chance, friend. Whatever the wheel finds, at the grade you pay for.")
	gamble.tell(npc, c, string.format(
		"%s buys it as it is found.   %s buys it Hallowed.   %s buys it Mythic.",
		gamble.price_link(1000,  "1000 platinum"),
		gamble.price_link(5000,  "5000 platinum"),
		gamble.price_link(10000, "10000 platinum")))
	gamble.tell(npc, c, "Nothing above your own station, and nothing from lands still closed to you.")
	local n = #gamble.candidates(c)
	if n > 0 then
		c:Message(15, string.format("The wheel currently holds %d prizes for you.", n))
	else
		c:Message(13, "The wheel holds nothing for you yet. Open a region, or grow into one.")
	end
end

function event_say(e)
	local c = player(e)
	if not c then return end
	local msg = (e.message or ""):lower()

	-- ⚠️⚠️ THE SAYLINK ARRIVES AS ORDINARY SPEECH, so it is matched here like any other say. The link
	-- is SILENT, so the player never actually broadcasts "wager 10000" -- but a modified client could
	-- send it by hand, which is exactly why every gate lives in gamble.wager_coin and not in the link.
	local plat = msg:match("^wager%s+(%d+)$")
	if plat then
		gamble.wager_coin(e.self, c, tonumber(plat))
		return
	end

	if msg:find("hail") then menu(e.self, c) end
end

-- ⚠️ The trade window still works and is the same wager by another route -- some players will hand
-- coin over rather than click. Both funnel through the one set of gates.
-- ⚠️⚠️ EVERY PATH THAT DOES NOT HAND OVER A PRIZE HANDS BACK THE COIN. item_lib.return_items is the
-- stock helper and returns money as well as items, so it is the single exit for every refusal.
function event_trade(e)
	local c = player(e)
	if not c then
		require("items").return_items(e.self, e.other, e.trade)
		return
	end

	local ok, msg, n = gamble.wager(e.self, c, e.trade)
	if not ok then
		gamble.tell(e.self, c, msg)
		require("items").return_items(e.self, e.other, e.trade)
		return
	end

	gamble.tell(e.self, c, "The wheel gives you " .. msg)
	c:Message(15, string.format("Drawn from %d prizes in the lands open to you.", n or 0))
	-- ⚠️ NO return_items on success: the coin is the price. Returning it here makes every wager free.
end
