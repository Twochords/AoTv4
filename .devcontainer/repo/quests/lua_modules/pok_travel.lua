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
	-- ⚠️ doorid 179 (the Docks book) WAS DELETED 2026-08-14 -- the discoverable travel spot in
	-- aotv4_travel replaced it, by owner decision. The mapping is kept because it costs nothing and
	-- documents which book was which; it is simply never reached now. Butcher has ONE book again
	-- (doorid 78, Kaladim), so the no-override fallback to the zone short name would also be correct.
	-- 📌 The DESTINATION is untouched: `butcherdocks` is still in pok_portals and is still handed out
	-- by the hub book's grant_sets below, so nobody loses the waypoint -- only the way to find it in
	-- the field changed.
	butcher  = { [78]  = "butcher",  [179] = "butcherdocks" },   -- doorid 78 = Kaladim, 179 = Docks (door removed)
}

-- HUB books: clicking one unlocks a whole SET of destinations at once (instead of just its own zone).
-- The hub book opens the three starter-zone books so new players can get going.
--
-- ⚠️⚠️ `resplendent` IS THE LIVE ONE; `tutorialb` IS HISTORY THAT IS KEPT ON PURPOSE. The set-granting
-- book started in tutorialb, then `aotv4_start_resplendent.sql` made resplendent the start zone for
-- every (race, class, deity) row and turned the tutorial off -- which left the only book that grants
-- the starter waypoints, and the only way to open the Portal window at the start, in a zone nobody
-- could reach. Reported from play as "I don't see the PoK book in Resplendent". The door itself is
-- migration v57.
-- ⚠️ tutorialb stays listed because it costs nothing and the tutorial has already been switched back
-- on once by an unannounced rules dump (CLAUDE.md section 35) -- if it becomes reachable again its book
-- should still work.
-- ⚠️⚠️ BOTH ARE SOURCES THAT ARE NEVER DESTINATIONS. resplendent is in `never_attune` below and
-- neither is in pok_portals, which is what stops a hub book becoming a free bind-anywhere. Adding
-- either to pok_portals.lua would break the roguelite loop.
local grant_sets = {
	resplendent = { "butcherdocks", "ecommons", "qeytoqrg" },
	tutorialb   = { "butcherdocks", "ecommons", "qeytoqrg" },
}

-- ⚠️⚠️ ALWAYS AVAILABLE, NEVER DISCOVERED. The artisan hub has to be reachable by EVERY character
-- from level 1, with no book to find first -- so it is unioned into the found list below rather than
-- written into `pok_found_<charid>`. Writing it into the bucket instead would only reach characters
-- who existed after the change, and would need a backfill for everyone else.
-- ⚠️ `attune` refuses these too, so clicking the hub's own book does not add a duplicate to the
-- bucket or print an "you attune to..." line for something the player already had.
-- 📌 It is NOT in `never_attune`: this hub is a legitimate DESTINATION. That rule exists for
-- resplendent, where you are BOUND and return by dying -- a portal there would skip the roguelite
-- loop. Nobody binds or respawns in the artisan hub, so travelling to it skips nothing.
-- ⚠️ DECLARED ABOVE `get_found`, AND IT MUST STAY THERE. It is a plain local, so a use inside a
-- function defined EARLIER in the file sees nil, not the table -- "bad argument #1 to 'pairs'
-- (table expected, got nil)" on the first zone-in. Lua closes over what is lexically in scope at
-- DEFINITION time; the M-table indirection used elsewhere in this project is the other way out.
local always_available = {
	freeporttheater = true,
}

-- discovered zones the player has, de-duped and validated against the portal table (so only real
-- PoK-book zones count -- "discovered zones that also have a poknowledge book").
local function get_found(client)
	local out, seen = {}, {}
	-- seed with the always-available hubs before anything the player actually discovered
	for s in pairs(always_available) do
		if portals[s] then seen[s] = true; out[#out + 1] = s end
	end
	for s in (eq.get_data(found_key(client)) or ""):gmatch("([^,]+)") do
		if portals[s] and not seen[s] then seen[s] = true; out[#out + 1] = s end
	end
	table.sort(out, function(a, b) return portals[a].long < portals[b].long end)
	return out
end

-- ⚠️⚠️ ZONES THAT MUST NEVER BECOME A PORTAL DESTINATION, whatever the portal table says.
-- Resplendent is the hub temple: you are BOUND there and you return by DYING, which is the whole
-- roguelite loop (die at cap -> earn a region credit -> respawn at Alessa -> spend it). A book to it
-- would let a player skip the death and turn the hub into a free bind-anywhere.
-- ⚠️ Today this is also true by accident -- resplendent has no `doors` row to poknowledge and is not
-- in pok_portals, so nothing can attune it. That is exactly why the rule is written down HERE as
-- well: the accident quietly disappears the moment somebody regenerates the portal table or adds a
-- book to the zone, and the failure would be silent (a new travel destination nobody intended).

local never_attune = {
	resplendent = true,
}

-- Add one portal short to the player's discovered set. Returns true only if it was newly added
-- (so re-clicking a known book doesn't spam or re-push the list).
local function attune(client, short)
	if not short or never_attune[short] or always_available[short] or not portals[short] then return false end
	local key  = found_key(client)
	local data = eq.get_data(key) or ""
	for s in data:gmatch("([^,]+)") do if s == short then return false end end   -- already known
	eq.set_data(key, data == "" and short or (data .. "," .. short))
	client:Message(MT.Yellow, string.format(
		"You attune to the Plane of Knowledge book: %s.", portals[short].long))
	return true
end

-- Record the clicked book as discovered. A hub book (grant_sets) unlocks a whole set; otherwise the
-- doorid disambiguates multi-book zones (gfaydark Kelethin/Felwithe, butcher Kaladim/Docks) and falls
-- back to the zone short name.
-- The books this character has attuned, as portal shorts, sorted by long name.
-- ⚠️ Public because the native Travel window (aotv4_travel) lists books alongside the discovered
-- field waypoints, and it must ask THIS module rather than re-reading `pok_found_<charid>` itself --
-- two readers of one bucket is how the two lists drift apart.
function M.found_list(client)
	return get_found(client)
end

function M.discover(client, zone_short, doorid)
	local set = grant_sets[zone_short]
	if set then
		local any = false
		for _, s in ipairs(set) do if attune(client, s) then any = true end end
		if any then M.send_list(client, true) end                               -- silent push to the dll
		return
	end
	local short = zone_short
	local ov = book_override[zone_short]
	if ov and doorid and ov[doorid] then short = ov[doorid] end
	if attune(client, short) then M.send_list(client, true) end
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

	-- ⚠️⚠️ THE BOOK IS ALSO THE TERMINAL FOR THE DISCOVERED-WAYPOINT NETWORK. Clicking it opens the
	-- native Travel window as well, which is the ONLY way that window ever opens -- there is no
	-- command for it, deliberately (aotv4_travel.handle_say explains why).
	-- 📌 Both windows currently answer one click. The GDI Portal overlay above is the last window in
	-- this dll that was never converted to a native SIDL one (§3 has carried that TODO for a while),
	-- and it should be retired now that this replaces it -- otherwise a book opens two competing
	-- travel UIs showing overlapping destinations.
	local ok_tv, travel = pcall(require, "aotv4_travel")
	if ok_tv and travel then travel.open_window(client) end
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
