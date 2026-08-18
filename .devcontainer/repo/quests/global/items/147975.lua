-- Fellowship Insignia -- click to return to your fellowship's campfire.
--
-- ⚠️⚠️ BOTH CLICK EVENTS ARE WIRED, AND RoF2 RAISES THE ONE YOU WOULD NOT EXPECT.
-- `EVENT_ITEM_CLICK` comes from Handle_OP_ItemVerifyRequest, but RoF2 clicks an item via
-- OP_CastSpell, which raises `EVENT_ITEM_CLICK_CAST` (client_packet.cpp:4462). With only the former
-- defined the item casts its inert spell and this script never runs -- the zone log shows "Cast from
-- unlimited charge item" on every click while the player sees nothing at all. Section 44 records the
-- Tome of Insight losing a session to exactly this.
-- ⚠️ `e.owner`, NOT `e.self` -- `e.self` is nil in item events.
-- ⚠️ Deduped, because a client sending both would otherwise travel twice.
local fellowship = require("aotv4_fellowship")

local function use(e)
	local c = e.owner
	if not c then return end
	local k = "insig_" .. c:CharacterID()
	if (tonumber(eq.get_data(k)) or 0) > os.time() then return end
	eq.set_data(k, tostring(os.time() + 2))
	-- ⚠️ NOT CONSUMED, and no charge is spent: `maxcharges = -1` is unlimited. Every gate (cooldown,
	-- combat, instance, region unlocked) lives in M.travel and is re-tested there, so this is only
	-- ever a way IN to that function.
	fellowship.travel(c)
end

function event_item_click(e)      use(e) end
function event_item_click_cast(e) use(e) end
