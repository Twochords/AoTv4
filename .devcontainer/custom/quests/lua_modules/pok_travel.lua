-- pok_travel.lua
-- Personal "discovered Plane of Knowledge book" travel network.
--   * Clicking a zone's PoK book (any door whose destination is poknowledge) discovers that zone.
--   * The dll Portal window (opened by a hotkey) lists discovered zones; picking one teleports you
--     to that zone's PoK-book landing spot (pok_portals). Usable from anywhere.
--
-- Transport mirrors the spell/AA windows:
--   server -> client : "PORTALDATA short|Long^short|Long^..."   (drives the dll window)
--   client -> server : "/say portalgo <short>"
-- A saylink fallback (/say portals) works with no client mod (for testing before the dll window).

local portals = require("pok_portals")   -- short -> {id,x,y,z,h,long}

local M = {}

local SAY_LIST      = "portals"     -- manual list + saylinks (testing / no client mod)
local SAY_REQ       = "portalreq"   -- silent list request from the dll hotkey (PORTALDATA only)
local SAY_GO        = "portalgo"    -- travel to a discovered zone
local SHOW_SAYLINKS = true

local function found_key(client) return "pok_found_" .. client:CharacterID() end

-- Some zones hold MORE THAN ONE PoK book, so the zone short name alone can't tell them apart. Map the
-- clicked book's doorid -> its own waypoint key. gfaydark has two: doorid 109 by Kelethin, doorid 108 by
-- the Felwithe zone line. Any zone/doorid not listed falls back to the zone short name (the common case).
local book_override = {
	gfaydark = { [109] = "gfaydark", [108] = "felwithe" },
}

-- discovered zones the player has, de-duped and validated against the portal table (so only real
-- PoK-book zones count -- "discovered zones that also have a poknowledge book").
local function get_found(client)
	local out, seen = {}, {}
	for s in (eq.get_data(found_key(client)) or ""):gmatch("([^,]+)") do
		if portals[s] and not seen[s] then seen[s] = true; out[#out + 1] = s end
	end
	table.sort(out, function(a, b) return portals[a].long < portals[b].long end)
	return out
end

-- Record the clicked book as discovered (only if it maps to a portal entry). doorid disambiguates zones
-- with more than one book (e.g. gfaydark's Kelethin vs Felwithe books); otherwise the zone short is used.
function M.discover(client, zone_short, doorid)
	local short = zone_short
	local ov = book_override[zone_short]
	if ov and doorid and ov[doorid] then short = ov[doorid] end
	if not short or not portals[short] then return end
	local key  = found_key(client)
	local data = eq.get_data(key) or ""
	for s in data:gmatch("([^,]+)") do if s == short then return end end        -- already known
	eq.set_data(key, data == "" and short or (data .. "," .. short))
	client:Message(MT.Yellow, string.format(
		"You attune to the Plane of Knowledge book here: %s.", portals[short].long))
	M.send_list(client, true)                                                    -- silent push to the dll
end

-- Emit the discovered list to the dll window. quiet=true sends only PORTALDATA (no saylinks).
function M.send_list(client, quiet)
	local found, fields = get_found(client), {}
	for _, s in ipairs(found) do fields[#fields + 1] = s .. "|" .. portals[s].long end
	client:Message(MT.NPCQuestSay, "PORTALDATA " .. table.concat(fields, "^"))
	if SHOW_SAYLINKS and not quiet then
		if #found == 0 then
			client:Message(MT.Yellow, "You have not discovered any Plane of Knowledge portals yet.")
			return
		end
		client:Message(MT.Yellow, "Discovered Plane of Knowledge portals:")
		for _, s in ipairs(found) do
			local link = eq.say_link(SAY_GO .. " " .. s, true, "[ Travel: " .. portals[s].long .. " ]")
			client:Message(MT.LightBlue, "  " .. link)
		end
	end
end

-- Tell the dll to OPEN the Portal window (clicking a PoK book opens the menu). Pushes the current
-- list first so the window is up to date even on a repeat click of an already-known book.
function M.open(client)
	M.send_list(client, true)                       -- PORTALDATA (current discovered list)
	client:Message(MT.NPCQuestSay, "PORTALOPEN")    -- dll shows the window
end

-- Resolve "portals" (list) and "portalgo <short>" (travel). Returns true if it consumed the message.
function M.handle_say(e)
	local msg = (e.message or ""):lower()
	if msg:match("^" .. SAY_REQ .. "%s*$") then       -- dll hotkey: silent list refresh
		M.send_list(e.self, true)
		return true
	end
	if msg:match("^" .. SAY_LIST .. "%s*$") then       -- manual: list + saylinks
		M.send_list(e.self, false)
		return true
	end
	local short = msg:match("^" .. SAY_GO .. "%s+([%w_]+)%s*$")
	if short then
		local client = e.self
		local p = portals[short]
		local known = false
		for _, s in ipairs(get_found(client)) do if s == short then known = true; break end end
		if not p or not known then
			client:Message(MT.Red, "You have not discovered that portal.")
			return true
		end
		client:Message(MT.Yellow, "The book's magic carries you to " .. p.long .. "...")
		client:MovePC(p.id, p.x, p.y, p.z, p.h)
		return true
	end
	return false
end

return M
