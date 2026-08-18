-- Fellowship campfire, clicked. All logic is in the module; this is only the hook.
--
-- ⚠️⚠️ RoF2 RAISES `EVENT_ITEM_CLICK_CAST`, NOT `EVENT_ITEM_CLICK` -- it clicks an item via
-- OP_CastSpell (client_packet.cpp:4462), so wiring only the latter leaves the item inert with the
-- zone log cheerfully reporting "Cast from unlimited charge item". Both are wired, and deduped, for
-- exactly the reason section 44 records for the Tomes of Insight.
-- ⚠️ The file MUST be named <itemid>.lua and live in quests/global/items/ -- `GetQIByItemQuest`
-- never searches quests/items/.
local fellowship = require("aotv4_fellowship")

local function click(e)
	-- ⚠️⚠️ `e.owner`, NOT `e.self`. Item quest events carry the clicking player as `owner`; `self` is
	-- nil here, and calling a method on it is "attempt to call method 'CharacterID' (a nil value)"
	-- with a traceback and no fire. The Tome of Insight scripts use `e.owner` for the same reason.
	local c = e.owner
	if not c then return end
	-- 2-second dedupe: a client may send both events for one click.
	local k = "fshipclick_" .. c:CharacterID()
	if (tonumber(eq.get_data(k)) or 0) > os.time() - 2 then return end
	eq.set_data(k, tostring(os.time()))

	-- ⚠️ Consume ONLY if the fire was actually lit. light_from_item refuses with no fellowship or
	-- inside an instance, and destroying a 300p combine on a refusal is unrecoverable.
	if fellowship.light_from_item(c, 147972) then
		c:RemoveItem(147972, 1)
	end
end

function event_item_click(e)      click(e) end
function event_item_click_cast(e) click(e) end
