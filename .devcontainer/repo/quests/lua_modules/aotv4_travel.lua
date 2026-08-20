-- aotv4_travel.lua
-- Discovered teleport spots: hidden markers out in the world that unlock a ONE-WAY destination.
--
-- Walk over a spot and it is yours forever. From then on you can travel TO it from anywhere. Nothing
-- is created at the far end, so arriving somewhere never gives you a way back -- getting home is the
-- roguelite's problem to solve (die, or walk).
--
--   discover -> quests/global/2000500.lua  (an invisible marker with a proximity)
--   list     -> "/say travel"            (a PRINTOUT only -- it never moves you)
--   travel   -> the Travel window, which opens ONLY at a Plane of Knowledge book
--
-- ⚠️⚠️ THIS IS NOT THE PoK BOOK NETWORK AND MUST NOT BECOME IT. pok_travel covers the hub and the
-- cities and is DISCOVERED BY CLICKING A VISIBLE BOOK. This covers the FIELD -- hunting zones with no
-- book, found by walking into them. Deliberately kept separate so neither has to grow a book in every
-- zone, which is the thing the owner explicitly did not want.
-- 📌 Why it exists at all: of 64 hunting zones in the region tables, only 11 had a book. Cabilis,
-- Thurgadin and Firiona Vie had ONE each for 8-10 zones, and Crushbone, Blackburrow, Unrest and
-- Kaesora -- the 1-20 spine every character re-runs -- had none.

local regions = require("aotv4_regions")

local M = {}

-- ⚠️⚠️ DECLARED HERE, AT THE TOP, AND IT MUST STAY THERE. It used to sit beside `M.open_window`
-- around line 714 -- 177 lines BELOW `M.request`, which calls it. A plain `local` is only visible to
-- functions defined AFTER it: Lua closes over what is lexically in scope at DEFINITION time, so
-- inside `M.request` the name resolved as a GLOBAL and was nil. Every travel request through the
-- window died with "attempt to call global 'bookkey' (a nil value)" -- so the window's travel had
-- never worked at all, and it only surfaced when the artisan hub became reachable and somebody
-- finally used it.
-- 📌 Exactly the trap `pok_travel.lua` records above its own `always_available` table. Two now, in
-- the same subsystem: if a local is used by more than one function here, define it before ALL of
-- them, or hang it off the M table, which is resolved at call time instead.
local function bookkey(c) return "travel_book_" .. c:CharacterID() end

-------------------------------------------------------------------- the spots
-- ⚠️⚠️ COORDINATES ARE REAL SPAWN POINTS, NOT INVENTED AND NOT THE ZONE SAFE POINT.
-- Each was taken from an actual `spawn2` row at roughly 60 percent of the zone's spawn spread from
-- the zone-in, which buys two things at once: it is guaranteed valid standing ground (something
-- stands there already), and it is far enough in to be a discovery rather than something you trip
-- over on arrival. §11 records the safe point being the WRONG answer for landing coordinates; here it
-- would also be the wrong answer for a *hidden* spot, for a different reason -- it is where everyone
-- already walks.
-- ⚠️ `region` is carried on the row rather than looked up. Lua cannot query `zone_regions`, and the
-- travel gate needs it on every trip -- see M.travel.
-- ⚠️ `id` is the bucket key and is PERMANENT. Renaming one strands every character's discovery of it.
-- ⚠️⚠️ `zone` IS CARRIED EXPLICITLY AND MUST STAY THAT WAY. Every id today happens to equal its zone
-- short name, and travel resolving `eq.get_zone_id_by_name(id)` off that coincidence would break
-- silently the first time a zone gets a SECOND spot -- which is the whole point of SPOTS being a list
-- per zone rather than one row.
-- 📌 Levels are from aotv4_zone_xp; they are only here to explain the choices to the next reader.
-- ⚠️⚠️⚠️ THE OWNER SUPPLIED THESE AS IN-GAME `/loc`, WHICH PRINTS **Y, X, Z** -- THE FIRST TWO ARE
-- SWAPPED RELATIVE TO THE COLUMNS BELOW. Every row here is already converted (x = the SECOND number
-- they gave, y = the FIRST). Paste a raw /loc into this table and the marker lands somewhere else
-- entirely, usually still inside the zone, which is why it would not look like a bug -- the spot
-- would simply never be found.
-- 📌 Verified rather than assumed: their Butcherblock Docks reading was 1239.39, 2929.87 and the PoK
-- book already in `doors` for that exact place is pos_x 2967.91, pos_y 1227.09 -- first number matches
-- pos_y, second matches pos_x. `aotv4_pok_books_extra.sql` documents the same convention.
--
-- ⚠️ `name` IS THE PoK-SYSTEM LABEL THE OWNER CHOSE, NOT THE ZONE NAME. Several are named for what
-- they are NEAR rather than where they are: the Lesser Faydark spot is "Mistmoore" because it sits at
-- that zone line, `beholder` is "Runnyeye", `warslikswood` is "Crypt of Dalnir". Renaming them to
-- their zones would be wrong -- the label is how a player is meant to think about the destination.
M.SPOTS = {
	-- Kelethin
	lfaydark     = { { id = "mistmoore",   zone = "lfaydark",     region = 1, x =  3234.88, y = -1173.00, z =    3.37, name = "Mistmoore" } },
	cauldron     = { { id = "unrest",      zone = "cauldron",     region = 1, x =  -705.11, y = -2012.48, z =   93.13, name = "Unrest" } },
	-- ⚠️ The PoK BOOK that used to stand here (butcher doorid 179) is REMOVED -- this spot replaces it
	-- as the way to discover Butcherblock Docks. The waypoint itself still exists in pok_portals and is
	-- still granted by the hub book, so the DESTINATION is unaffected; only the physical book is gone.
	butcher      = { { id = "butcherdocks", zone = "butcher",    region = 1, x =  2929.87, y =  1239.39, z =   -0.54, name = "Butcherblock Docks" } },

	-- Freeport
	ecommons     = { { id = "ectunnel",    zone = "ecommons",     region = 2, x =  -128.78, y = -1655.34, z =    3.13, name = "EC Tunnel" } },
	beholder     = { { id = "runnyeye",    zone = "beholder",     region = 2, x = -1831.67, y =   909.42, z =    4.13, name = "Runnyeye" } },

	-- Thurgadin
	iceclad      = { { id = "iceclad",     zone = "iceclad",      region = 3, x =  2831.24, y =  1715.48, z =  118.08, name = "Iceclad Ocean" } },
	eastwastes   = { { id = "rygorr",      zone = "eastwastes",   region = 3, x =   478.08, y = -4069.73, z =  145.08, name = "Ry'Gorr" } },
	greatdivide  = { { id = "velketor",    zone = "greatdivide",  region = 3, x =  3350.42, y = -6748.65, z =  -36.00, name = "Velketor" } },

	-- Firiona Vie
	dreadlands   = { { id = "karnors",     zone = "dreadlands",   region = 4, x = -1856.41, y =   711.39, z =   30.00, name = "Karnor's Castle" } },
	nurga        = { { id = "nurga",       zone = "nurga",        region = 4, x =  -566.49, y = -2728.95, z = -494.99, name = "Mines of Nurga" } },

	-- Qeynos
	southkarana  = { { id = "splitpaw",    zone = "southkarana",  region = 5, x =   910.63, y = -3171.33, z =   -8.81, name = "Splitpaw" } },

	-- Cabilis
	-- ⚠️ veksar is a real Kunark WORLD zone with zone_points, which is why §24 keeps it out of the delve
	-- pool -- unlocking it there would have opened Kunark travel to anyone. A spot is different: you
	-- have to walk to Veksar to discover it, and travelling back still needs the Cabilis region.
	veksar       = { { id = "veksar",      zone = "veksar",       region = 6, x =    76.12, y =  -512.09, z =   47.13, name = "Veksar" } },
	warslikswood = { { id = "dalnir",      zone = "warslikswood", region = 6, x =  4559.55, y =  2586.92, z = -238.00, name = "Crypt of Dalnir" } },
}

-- How close you must get. ⚠️ A PROXIMITY IS A BOX, NOT A SPHERE, so this is a half-width on each
-- axis. Generous on purpose: the spot is meant to be found by walking through the area, not by
-- standing on an exact pixel. It was sized when the marker was invisible and there was nothing to aim
-- at; the portal is now visible, so this is more generous than it strictly needs to be -- left alone
-- because walking THROUGH a portal should discover it, not brushing an exact edge.
-- ⚠️ The Z half-width is SEPARATE and much tighter -- a fat Z box in a multi-level dungeon would fire
-- from the floor above, which reads as the spot being in the wrong place.
M.RADIUS_XY = 40.0
M.RADIUS_Z  = 25.0

-- The marker NPC. VISIBLE PORTAL as of migration v64 -- see make_marker below for the appearance and
-- for why the client needs `bothunder_chr` in GlobalLoad_chr.txt.
--
-- ⚠️⚠️ HISTORY WORTH KEEPING, because it cost four rounds of screenshots: while this was an INVISIBLE
-- trigger, what kept it visible was the **WEAPONS**, not the race. It was cloned from `Animation1`
-- (500), which ships `d_melee_texture1 = 10653` and `d_melee_texture2 = 202`, so every waypoint stood
-- in the field holding a sword and a shield -- reported first as "they look like enchanter pets" and
-- then, after the race was changed, as a fully drawn human warrior. **A held weapon renders even when
-- the body does not.** The race was blamed and changed twice before anyone read the equipment columns.
-- 📌 The other half of that lesson: invisibility is the **bodytype**, not the race -- "body types above
-- 64 make the mob invisible" (common/bodytypes.h). 11 is `NoTarget`, which only suppresses the name and
-- the target, and is exactly why the marker can be a visible portal AND unkillable at the same time.
-- ⚠️ 2000500 is in the AoTv4 band, which matters twice over: the delve's spawn hook and the difficulty
-- system's scaler BOTH skip npc ids >= 2000000, so a marker is never scaled, rescaled or given loot.
M.MARKER_NPC = 2000500
local SPOT_VAR = "travel_spot"

-------------------------------------------------------------------- storage
-- ⚠️ PERMANENT, like pok_found_ and the region unlocks. Discovery is exploration progress and the
-- roguelite wipe takes levels and gear, never knowledge.
local function fkey(c) return "travel_found_" .. c:CharacterID() end

local function found_set(c)
	local out = {}
	for s in (eq.get_data(fkey(c)) or ""):gmatch("([^,]+)") do out[s] = true end
	return out
end

function M.has(c, id)
	return found_set(c)[id] == true
end

-------------------------------------------------------------------- Plane of Knowledge books
-- The books are a SECOND network folded into the same window. They are not field waypoints: you find
-- them by clicking a visible book rather than by walking over a hidden marker, and they are gated by
-- their own attunement rather than by a region.
--
-- ⚠️⚠️ A BOOK IS SORTED INTO THE REGION ITS DESTINATION ACTUALLY BELONGS TO. There is NO pseudo
-- "Plane of Knowledge" region, and one must not be reintroduced. It had one (id 90) on the first
-- pass, on the reasoning that `pok_portals` carries no region data and hand-assigning 30 zones was
-- not worth it -- but that put a region on screen that **this server does not have**: PoK itself is
-- not reachable here, so a row named after it advertises a place nobody can go and files thirteen
-- perfectly ordinary destinations under it instead of under Kelethin, Freeport and the rest.
--
-- ⚠️⚠️ AND 17 OF THE 30 BOOKS ARE NOT TRACKED AT ALL, BY DESIGN. Their destinations sit in region
-- **99 "Unused"** -- the set `aotv4_regions` describes as fenced off from the whole server -- so they
-- are reachable only *via* PoK, which is exactly the case that cannot happen here. Listing them as
-- "Unknown" would show a player a permanent hole in their map that no amount of play can fill.
-- Dropped: arena, bazaar, crescent, everfrost, feerrott, guildlobby, gunthak, innothule, nektulos,
-- nexus, overthere, potranquility, shadeweaver, shadowrest, steamfont, tox, weddingchapel.
--
-- 📌 The mapping was READ OUT OF `zone_regions`, not authored: each book's `pok_portals` zone id
-- joined to that table. Lua cannot query it (the same reason the field spots carry `region` on the
-- row, line 32), so the answer is baked here -- **if `zone_regions` is ever re-pointed, regenerate
-- this table**, or a book will be filed under a region that no longer contains its destination.
-- ⚠️ `felwithe` and `gfaydark` are BOTH zone 54 and both region 1. Not an error: felwithe is the
-- second gfaydark book, by the Felwithe zone line (doorid 108) -- see pok_portals' own comment.
-- ⚠️⚠️ REGION 0 MEANS "ALWAYS AVAILABLE" AND IS NOT SUBJECT TO THE UNLOCK GATE. It is the same
-- convention `zone_regions` already uses (section 24: region 0 is Always Available, and
-- `RegionManager::CanEnterZone` treats an unmapped zone as unrestricted) -- but `HasRegion` does NOT
-- share it: `RegionManager::HasRegion` returns FALSE for region 0 outright (`if (!char_id ||
-- !region_id) return false;`, zone/regions.cpp:166). So a region-0 destination must be gated through
-- `M.is_open` and never by a bare HasRegion call, or the hub reads as locked to everybody.
M.ALWAYS_REGION = 0
M.ALWAYS_NAME   = "Always Available"

-- ⚠️ EVERY gate on a destination goes through here. Three sites used to call HasRegion directly and
-- must not: region 0 fails that test by construction.
function M.is_open(c, region)
	if region == M.ALWAYS_REGION then return true end
	return (c:GetGM() or c:HasRegion(region)) and true or false
end

M.BOOK_REGION = {
	-- ⚠️ The artisan hub is region 0: open to everyone from level 1, nothing to discover, nothing to
	-- unlock. It is a DESTINATION as well as a source -- unlike resplendent, which `pok_travel` keeps
	-- in `never_attune` because you return there by dying and a portal would skip the roguelite loop.
	freeporttheater = 0,
	butcher      = 1, butcherdocks = 1, felwithe = 1, gfaydark = 1,   -- Kelethin
	ecommons     = 2, freportw     = 2, misty    = 2,                 -- Freeport
	greatdivide  = 3,                                                 -- Thurgadin
	firiona      = 4,                                                 -- Firiona Vie
	qeynos2      = 5, qeytoqrg     = 5, rathemtn = 5,                 -- Qeynos
	fieldofbone  = 6,                                                 -- Cabilis
}

-- ⚠️⚠️ THE PREFIX MUST BE `_`-SAFE. The say pattern that receives a travel request is
-- `^travelgo%s+([%a%d_]+)%s+...` -- a colon does NOT match that class, so "pok:butcher" would be
-- silently rejected by the pattern and the button would appear to do nothing at all.
local BOOK_PREFIX = "pok_"

-- A book destination in the same shape as a field spot, so travel and eligibility need no special
-- cases beyond the `book` flag.
local function book_spot(id)
	local short = id and id:match("^pok_(.+)$")
	if not short then return nil end
	local ok, portals = pcall(require, "pok_portals")
	if not ok or not portals or not portals[short] then return nil end
	local p = portals[short]
	-- ⚠️⚠️ AN UNMAPPED BOOK IS NOT A BOOK. Returning nil here is what drops the 17 region-99
	-- destinations out of every path at once -- the window list, the unknown counts and the travel
	-- request all resolve a spot through this one function, so there is no second place to remember.
	local region = M.BOOK_REGION[short]
	if not region then return nil end
	return {
		id     = id,
		zone   = short,
		region = region,
		x = p.x, y = p.y, z = p.z, h = p.h or 0,
		name   = p.long or short,
		book   = true,
		zone_id = p.id,
	}
end

-- Which books this character has attuned. ⚠️ Asked of pok_travel rather than read out of the bucket
-- here -- two readers of `pok_found_<charid>` is how the two lists drift.
-- ⚠️ Required INSIDE the function, not at load: pok_travel requires THIS module (to open the window
-- on a book click), so a load-time require in both directions would be a cycle. Deferred both ways
-- is not.
local function books_found(c)
	local ok, pok = pcall(require, "pok_travel")
	if not ok or not pok or not pok.found_list then return {} end
	local out = {}
	for _, short in ipairs(pok.found_list(c) or {}) do
		local sp = book_spot(BOOK_PREFIX .. short)
		if sp then out[#out + 1] = sp end
	end
	return out
end

-- How many books EXIST in a given region (the denominator behind that region's "Unknown" rows).
-- ⚠️⚠️ COUNTED OFF `M.BOOK_REGION`, NEVER OFF `pok_portals`. Counting the portal table gives 30 and
-- includes the 17 region-99 destinations, so every region's unknown count would be inflated by books
-- that can never be found -- the same "a permanent hole no play can fill" the drop exists to avoid.
local function books_total(region_id)
	local n = 0
	for _, r in pairs(M.BOOK_REGION) do
		if r == region_id then n = n + 1 end
	end
	return n
end

-- Look a spot up by its id, across every zone -- and across the book network.
function M.spot(id)
	for _, list in pairs(M.SPOTS) do
		for _, sp in ipairs(list) do
			if sp.id == id then return sp end
		end
	end
	return book_spot(id)
end

-------------------------------------------------------------------- discovery
-- Called from the marker's EVENT_ENTER. Returns true only on a NEW discovery.
function M.discover(client, id)
	if not client or not client.valid or not id then return false end
	local sp = M.spot(id)
	if not sp then return false end

	local set = found_set(client)
	if set[id] then return false end          -- already known: say nothing, or it spams on every pass

	local data = eq.get_data(fkey(client)) or ""
	eq.set_data(fkey(client), data == "" and id or (data .. "," .. id))

	-- ⚠️ Two lines on purpose: the first is the FLAVOUR the owner specified and is what the player is
	-- meant to notice; the second is the mechanics. Folding them into one sentence buries the fact
	-- that something permanent just happened behind atmosphere.
	client:Message(MT.Yellow, "This spot seems of importance, you seem to remember it.")
	client:Message(MT.Yellow, string.format(
		"You may now travel to %s from anywhere. Say /travel to see everywhere you have found.", sp.name))
	return true
end

-------------------------------------------------------------------- markers
-- Spawn this zone's markers. Called from global_player.event_enter_zone.
--
-- ⚠️⚠️ ONCE PER ZONE PROCESS, NOT ONCE PER PLAYER. `spawned` is a module-level flag and every zone
-- runs its own Lua state, so this is exactly "the first player into this zone boots the markers" --
-- the same pattern §17c uses for the world boss, and for the same reason: a zone with nobody in it is
-- not running, so there is no earlier moment to do it.
-- ⚠️ NEVER IN AN INSTANCE. A delve or a difficulty shard is a private copy of a real zone; spawning
-- markers there would let a player discover a field spot from inside a dungeon run.
local spawned = false

-- ⚠️⚠️ THE INVISIBILITY IS ENFORCED HERE, AT RUNTIME -- IT IS NOT LEFT TO THE `npc_types` ROW.
-- The row is set up correctly too, but a row is not a guarantee: §35 records a table-wide import
-- reverting the three hub NPCs' appearance TWICE, and §11 records the same import silently dropping
-- the triggers that were meant to re-assert them. A marker that quietly becomes visible is not a
-- broken trigger, it is a signpost standing on top of the secret -- so the travel system asserts its
-- own appearance every time it spawns one, and a reverted row costs nothing.
--
-- ⚠️⚠️ THE WEAPONS ARE THE PART THAT ACTUALLY SHOWS. Cloned from `Animation1` (500), the row carried
-- `d_melee_texture1 = 10653` / `d_melee_texture2 = 202`, and **a held weapon renders even when the
-- body does not**. That is the whole "it looks like an enchanter pet" report: an enchanter pet IS an
-- invisible man holding a weapon, so an invisible body plus a floating sword is precisely one. The
-- race was blamed and changed twice before anyone cleared slots 7 and 8.
-- 📌 Race 127 is `Race::InvisibleMan` and 240 is `Race::TeleportMan` (`common/races.h`). BOTH are used
-- by stock triggers, so neither is "the invisible one" on its own -- but 240 draws a solid human on
-- this client and 127 draws nothing. Stock precedent for exactly this shape is `Halloween_Trigger`
-- (63109): race 127, texture 0, both weapon slots 0.
-- ⚠️ Slots 0-6 are armour, **7 is primary and 8 is secondary** (`common/textures.h`). Clear all nine:
-- armour on a bodyless race is invisible anyway, but it costs one loop and closes the same hole.
--
-- ⚠️⚠️ THE MARKER IS A VISIBLE PORTAL, AND THE RUNTIME ASSERTION MUST MATCH THE ROW OR IT WINS.
-- This used to force the marker invisible at spawn (race 240 + SetInvisible), which was correct while
-- the waypoints were hidden triggers. They are portals now -- see migration v64 -- so the assertion
-- pushes the PORTAL appearance instead. Leaving the old invisibility here would silently override the
-- npc_types row on every spawn and nothing would render, which is the harder version of this bug to
-- find: the data would look right and the world would not match it.
--
-- Race **567** (Race::Campfire), gender 2, size 6 -- the stock campfire size. A portal (race 329) was
-- built first and worked; the bonfire was chosen once both were seen side by side.
-- ⚠️⚠️ **GENDER 2 IS STILL LOAD BEARING.** A modelless or mis-gendered race falls back to a HUMAN MALE
-- -- which is what four rounds of "the waypoints look like men" screenshots were. Every stock portal
-- and every stock trigger is gender 2.
-- ⚠️ **bodytype 11 is what makes it unkillable**: NoTarget, so no name plate and nothing to target.
-- It does NOT hide the model -- invisibility is "body types above 64" (common/bodytypes.h) -- which is
-- precisely why it can be a visible portal and still be untargetable.
-- ⚠️ Weapon slots are still cleared. A held weapon renders even when the body does not, and cloning
-- history means the row has carried one before.
--
-- ⚠️⚠️ RACE 567 ONLY RENDERS IF `guildlobby_chr` IS IN THE CLIENT'S Resources\GlobalLoad_chr.txt.
-- A race draws only where the zone's own `_chr` archive carries it. Of the 37 zones that spawn 567,
-- only TWO are S3D era -- `guildlobby` (expansion 9) and our own `resplendent`; the other 35 are
-- HoT/VoA/RoF, which ship as EQG and which that loader silently ignores (see aotv4_dungeon's
-- BOSS_RACES note, where a first attempt at five EQG archives failed three times over).
-- ⚠️ A player without the updated file sees a HUMAN MALE, not nothing -- the client falls back rather
-- than drawing empty -- so it looks like a person standing in a field and reports nothing anywhere.
local function make_marker(npc)
	for slot = 0, 8 do npc:WearChange(slot, 0) end
	npc:SetBodyType(11, true)
	npc:SendIllusionPacket({ race = 567, gender = 2, texture = 0, helmtexture = 0, size = 6 })
end

function M.spawn_markers()
	if spawned then return end
	if (eq.get_zone_instance_id() or 0) ~= 0 then return end

	local list = M.SPOTS[eq.get_zone_short_name() or ""]
	if not list then
		spawned = true                        -- nothing here; do not re-check on every arrival
		return
	end

	-- ⚠️⚠️ ASK THE WORLD WHAT IS ALREADY THERE -- DO NOT TRUST THE `spawned` FLAG ALONE.
	-- Reported from play as **four markers at the Great Divide spot**. A module-level local is only
	-- as good as the assumption that this module is loaded once per zone process, and that assumption
	-- failed: the flag is a cheap early-out, never the correctness guarantee. Nothing in the DB was
	-- responsible (zero `spawn2` rows for the marker npc), so every copy came from re-entering this
	-- function, and each re-entry silently ADDED one -- an invisible marker gives no feedback that it
	-- has been spawned twice, so this accumulates unnoticed until somebody counts nameplates.
	-- 📌 Keyed on the spot's own entity variable rather than "does any marker exist in this zone", so
	-- it stays correct when a zone eventually holds more than one spot.
	local present = {}
	local existing = eq.get_entity_list():GetNPCList()
	for npc in existing.entries do                    -- ⚠️ `.entries`, not ipairs (see aotv4_dungeon)
		if npc and npc.valid and npc:GetNPCTypeID() == M.MARKER_NPC then
			local had = npc:GetEntityVariable(SPOT_VAR)
			if had and had ~= "" then
				if present[had] then
					npc:Depop(false)                  -- a duplicate from an earlier pass; take it back
				else
					present[had] = true
				end
			end
		end
	end

	-- ⚠️ No `goto continue` here: this server runs **Lua 5.1**, where goto is a syntax error (it
	-- arrived in 5.2). The container's luacheck builds against 5.1 and will catch it, but only if it
	-- is run -- so prefer the plain conditional.
	for _, sp in ipairs(list) do
		if not present[sp.id] then
			-- ⚠️ `eq.spawn2` returns a Lua_Mob, NOT a Lua_NPC (§26) -- CastToNPC before anything else,
			-- or SetEntityVariable is "attempt to call method (a nil value)" at runtime with nothing
			-- to warn you at write time.
			local mob = eq.spawn2(M.MARKER_NPC, 0, 0, sp.x, sp.y, sp.z, 0)
			if mob and mob.valid then
				local npc = mob:CastToNPC()
				if npc and npc.valid then
					-- Which spot this marker IS. One npc row and one script serve every spot, so the
					-- identity has to travel on the spawn rather than in the script.
					-- ⚠️ Stamped FIRST, before anything else can fail -- the dedupe above keys on this
					-- variable, so a marker that spawned without it is invisible to the check and the
					-- next call adds another one.
					npc:SetEntityVariable(SPOT_VAR, sp.id)
					make_marker(npc)
					present[sp.id] = true
				end
			end
		end
	end
	spawned = true
end

function M.spot_of(npc)
	if not npc or not npc.valid then return nil end
	local v = npc:GetEntityVariable(SPOT_VAR)
	if not v or v == "" then return nil end
	return v
end


-------------------------------------------------------------------- what you know
-- Everything this character has found, sorted by name.
function M.list(c)
	local set, out = found_set(c), {}
	for _, list in pairs(M.SPOTS) do
		for _, sp in ipairs(list) do
			if set[sp.id] then out[#out + 1] = sp end
		end
	end
	table.sort(out, function(a, b) return a.name < b.name end)
	return out
end

-- How many spots in a region are still UNDISCOVERED. The window shows these as "Unknown" rows so a
-- player can see that a region has more to find without being told where.
-- ⚠️ Counted from the authored table, never from what the player has -- the whole point is to reveal
-- the SHAPE of what is missing.
function M.unknown_count(c, region_id)
	local set, n = found_set(c), 0
	for _, list in pairs(M.SPOTS) do
		for _, sp in ipairs(list) do
			if sp.region == region_id and not set[sp.id] then n = n + 1 end
		end
	end
	return n
end

-------------------------------------------------------------------- eligibility
-- Can this ONE client travel to this spot? Returns true, or false plus the reason.
-- ⚠️ Split out of M.travel so the GROUP check can ask the same question about every member and get
-- the same answer -- two copies of this would drift and the group rule would stop matching the solo
-- one, which is exactly the kind of difference nobody notices until it is unfair.
function M.may_travel(c, sp)
	if not c or not c.valid then return false, "someone who is not here" end

	-- ⚠️⚠️ A BOOK IS GATED BY ATTUNEMENT, NOT BY A REGION, AND THAT IS PRE-EXISTING BEHAVIOUR RATHER
	-- THAN A DECISION MADE HERE. pok_travel has never region-checked its destinations, so folding the
	-- books into this window deliberately does NOT start: changing what a book does was not part of
	-- giving it a new window, and doing it silently would strand players mid-network.
	-- 📌 It does mean one window holds two rules -- field waypoints need the region open, books do not.
	-- If that should change, it is a deliberate edit here and in pok_travel together, not a tweak.
	if sp.book then
		for _, b in ipairs(books_found(c)) do
			if b.id == sp.id then return true end
		end
		return false, string.format("%s has not attuned to the %s book", c:GetCleanName(), sp.name)
	end

	if not M.has(c, sp.id) then
		return false, string.format("%s has never found %s", c:GetCleanName(), sp.name)
	end
	-- GM exemption, matching RegionManager::CanEnterZone. See the note in M.travel.
	if not M.is_open(c, sp.region) then
		return false, string.format("%s has not opened %s", c:GetCleanName(),
			regions.REGIONS[sp.region] or "that region")
	end
	return true
end

-- ⚠️⚠️ GROUP TRAVEL IS ALL-OR-NOTHING, BY OWNER DECISION, AND IT MUST NAME WHO IS BLOCKING IT.
-- A silent refusal on a group port is the worst possible outcome: the leader sees nothing happen and
-- has no way to find out which of five people is the problem. Every failure path below therefore
-- reports the NAME and the REASON.
-- ⚠️ Every member must also be IN THIS ZONE. `Group::GetMember` returns Mob pointers that are only
-- valid in the caster's own zone, so a member elsewhere cannot have their regions checked at all --
-- and porting somebody from across the world is not what a group port means anyway. Refusing beats
-- silently leaving them behind after everyone else has gone.
function M.group_blockers(c, sp)
	local grp = c:GetGroup()
	if not grp or not grp.valid then return nil end   -- no group: solo travel, no blockers

	local blockers = {}
	local n = grp:GroupCount() or 0
	for i = 0, n - 1 do
		local m = grp:GetMember(i)
		if m and m.valid then
			-- ⚠️ GetMember hands back a Lua_Mob (§24). CharacterID, HasRegion and GetGM are all
			-- Client-only, so resolve through the entity list -- which also doubles as the
			-- "is this member in my zone" test, because it returns nil when they are not.
			local mc = eq.get_entity_list():GetClientByID(m:GetID())
			if not mc or not mc.valid then
				blockers[#blockers + 1] = string.format("%s is not in this zone", m:GetCleanName() or "a member")
			else
				local ok, why = M.may_travel(mc, sp)
				if not ok then blockers[#blockers + 1] = why end
			end
		end
	end
	return blockers
end

-------------------------------------------------------------------- travel
-- ⚠️⚠️ REGION LOCKED. §11 records that travel spells are blacklisted partly BECAUSE ports defeat
-- region locking, and this is a port. Checked at TRAVEL time rather than at discovery: finding a
-- place you cannot yet reach is a real reward, and it shows a player what a region credit buys.
-- ⚠️⚠️ THE GM BYPASS IS NOT OPTIONAL. `RegionManager::CanEnterZone` opens with
-- `if (c->GetGM()) return true;` -- *"GMs are never region-gated; they need to be able to reach
-- content to fix it."* A gate of ours that is stricter than the engine's means a GM can WALK into a
-- zone this refuses to port them to. §41 is the other half: gates still get TESTED as a non-GM.
local PENDING = "travel_pending_"
local POPUP_ID = 7301

local function do_travel(c, sp, with_group)
	-- ⚠️ A book row already carries its zone id and heading from pok_portals, which are hand-corrected
	-- landing points (§11 records 14 of them being wrong when they were derived from the zone safe
	-- point). Re-deriving the id by name would work; throwing away the HEADING would not.
	local zid = sp.zone_id or eq.get_zone_id_by_name(sp.zone)
	if not zid or zid == 0 then
		c:Message(MT.Red, "The way there is closed.")
		return false
	end

	if with_group then
		local grp = c:GetGroup()
		if grp and grp.valid then
			-- ⚠️⚠️ MOVED LOCALLY, ONE MEMBER AT A TIME. This used to call
			-- `eq.cross_zone_move_player_by_group_id`, which PRINTED THE MESSAGE AND MOVED NOBODY --
			-- not the group, not even the caller. Reported from play as exactly that.
			-- The binding is real and its six-argument form is real; the transport underneath is not.
			-- `CZMove_Struct` (common/servertalk.h:1399) holds `client_name` and `zone_short_name` as
			-- **std::string**, and `QuestManager::CrossZoneMove` casts a RAW packet buffer to that
			-- struct and assigns them before sending it to another process. A std::string is a
			-- pointer into this process's heap, so world receives a meaningless address and the
			-- destination zone name arrives as nothing. It is stock EQEmu and it is broken for every
			-- CZMove caller; `CZSetEntityVariable_Struct` beside it uses `char[256]` and works, which
			-- is why fellowship chat is fine and this was not.
			-- 📌 We do not need it at all: `M.group_blockers` REFUSES unless every member is already
			-- in this zone, so a local MovePC -- the same call the solo path makes -- reaches all of
			-- them. Do not "restore" the cross-zone call; it cannot work until that struct is fixed.
			local others = {}
			local me     = c:CharacterID()
			local n      = grp:GroupCount() or 0
			for i = 0, n - 1 do
				local m = grp:GetMember(i)
				if m and m.valid then
					local mc = eq.get_entity_list():GetClientByID(m:GetID())
					if mc and mc.valid and mc:CharacterID() ~= me then
						others[#others + 1] = mc
					end
				end
			end

			-- ⚠️⚠️ THE CALLER MOVES LAST. Their MovePC is a zone change, which tears down the group
			-- member pointers this loop is holding -- move them first and the rest of the group is
			-- left standing there, which is the same silent half-failure in a new costume.
			c:Message(MT.Yellow, string.format("The world folds, and your group steps through to %s.", sp.name))
			for _, mc in ipairs(others) do
				mc:Message(MT.Yellow, string.format("The world folds, and your group steps through to %s.", sp.name))
				mc:MovePC(zid, sp.x, sp.y, sp.z, sp.h or 0)
			end
			c:MovePC(zid, sp.x, sp.y, sp.z, sp.h or 0)
			return true
		end
	end

	c:Message(MT.Yellow, string.format("The world folds, and you step through to %s.", sp.name))
	c:MovePC(zid, sp.x, sp.y, sp.z, sp.h or 0)
	return true
end

-- The window asks for a trip. `group` and `auto` come from its two checkboxes.
function M.request(c, id, group, auto)
	if not c or not c.valid then return false end

	-- ⚠️⚠️ A BOOK MUST HAVE BEEN CLICKED. The window opens two ways -- from a Plane of Knowledge book
	-- (which stamps `travel_book_<charid>`) and from the /aot launcher as a read-only map. The dll
	-- hides the Travel button in the second case, but that is presentation: this is the rule.
	-- Without it the launcher becomes travel-from-anywhere and the books stop mattering at all.
	-- ⚠️ Checked BEFORE anything else so the refusal names the real reason rather than a downstream one.
	local stamped = tonumber(eq.get_data(bookkey(c))) or 0
	if stamped <= 0 or (os.time() - stamped) > M.BOOK_GRACE_SECS then
		c:Message(MT.Red, "You must be reading a Plane of Knowledge book to travel. This is a map of what you have found.")
		return false
	end

	local sp = M.spot(id)
	if not sp then
		c:Message(MT.Red, "You know of no such place.")
		return false
	end

	local ok, why = M.may_travel(c, sp)
	if not ok then
		c:Message(MT.Red, why .. ".")
		return false
	end

	-- ⚠️ Refused in combat, for the same reason the delve and the difficulty shift are (§24, §43): a
	-- zone change breaks every hate list at once with no cast time and no reagent.
	if (c:GetAggroCount() or 0) > 0 then
		c:Message(MT.Red, "Not while something is hunting you. Break away first.")
		return false
	end

	if group then
		local blockers = M.group_blockers(c, sp)
		if blockers and #blockers > 0 then
			-- ⚠️⚠️ NAME THEM. All-or-nothing is only fair if the leader can see who to talk to.
			c:Message(MT.Red, string.format("Your group cannot travel to %s together:", sp.name))
			for _, b in ipairs(blockers) do c:Message(MT.Red, "  " .. b .. ".") end
			return false
		end
	end

	if auto then
		return do_travel(c, sp, group)
	end

	-- ⚠️⚠️ THE CONFIRM IS THE ENGINE'S OWN POPUP, NOT A dll DIALOG. `eq.popup(title, text, id, 1)`
	-- gives a real OK/Cancel box and the answer arrives as EVENT_POPUP_RESPONSE -- the same mechanism
	-- guildhall/#Porter.lua already uses for a teleport confirmation. That keeps the whole
	-- confirmation server side, so it needs no client rebuild and cannot be bypassed by a modified dll.
	-- ⚠️ EVENT_POPUP_RESPONSE carries ONLY `popup_id` -- there is no "which button" field, and a cancel
	-- simply never fires. So the intent is parked in a bucket and the id is a constant; encoding the
	-- destination into the id would cap us at whatever fits in a uint32 and make the handler unreadable.
	eq.set_data(PENDING .. c:CharacterID(), string.format("%s|%s", id, group and "1" or "0"))
	eq.popup("Travel", string.format("Travel to %s%s?", sp.name, group and ", with your group" or ""),
		POPUP_ID, 1, 0)
	return true
end

-- The player pressed OK on that popup.
function M.on_popup(e)
	if (e.popup_id or 0) ~= POPUP_ID then return false end
	local c = e.self
	local key = PENDING .. c:CharacterID()
	local raw = eq.get_data(key) or ""
	eq.delete_data(key)

	local id, grp = raw:match("^([%a%d_]+)|([01])$")
	if not id then return true end
	local sp = M.spot(id)
	if not sp then return true end

	-- ⚠️ RE-CHECK, do not trust the parked request. Time passed while the box was on screen: they may
	-- have been pulled, or (in a group) somebody may have zoned out. The popup is a confirmation, not
	-- a licence.
	local ok, why = M.may_travel(c, sp)
	if not ok then
		c:Message(MT.Red, why .. ".")
		return true
	end
	if (c:GetAggroCount() or 0) > 0 then
		c:Message(MT.Red, "Not while something is hunting you. Break away first.")
		return true
	end
	if grp == "1" then
		local blockers = M.group_blockers(c, sp)
		if blockers and #blockers > 0 then
			c:Message(MT.Red, string.format("Your group cannot travel to %s together:", sp.name))
			for _, b in ipairs(blockers) do c:Message(MT.Red, "  " .. b .. ".") end
			return true
		end
	end

	do_travel(c, sp, grp == "1")
	return true
end

-------------------------------------------------------------------- say handling
-- ⚠️⚠️ `/travel` IS A PRINTOUT AND NOTHING ELSE, BY OWNER DECISION. Travelling happens ONLY through
-- the window, which opens ONLY at a Plane of Knowledge book -- the books are the terminals and the
-- discovered spots are the destinations. There is deliberately no "travelto" say command: a say that
-- ported you would make the book irrelevant and would be usable from anywhere, which is the entire
-- thing this design is trying not to be.
-------------------------------------------------------------------- the window
-- Push both lists to the dll. Regions first, then the destinations this character has FOUND.
--
-- ⚠️⚠️ UNDISCOVERED WAYPOINTS ARE SENT AS A COUNT, NEVER BY NAME. `unknown` per region is all the
-- client gets, and it renders that many "Unknown" rows. Sending the names and hiding them client
-- side would put the answer in the packet where a modified dll could read it -- and the entire point
-- of a hidden waypoint is that you have to walk to it.
-- ⚠️ Two lines rather than one: §15 records this chat pipe DROPPING AND REORDERING bursts, so the
-- less each line depends on the other the better. The dll refreshes on whichever arrives.
function M.send_window(c)
	if not c or not c.valid then return end

	-- ⚠️⚠️ SIX ROWS, ONE PER REAL REGION -- there is no seventh. Books are counted into the region
	-- their destination belongs to (see M.BOOK_REGION), so a Kelethin book raises Kelethin's numbers.
	-- A separate "Plane of Knowledge" row used to sit here and was wrong twice over: PoK is not a
	-- place this server has, and filing ordinary Faydwer and Antonica destinations under it hid them
	-- from the region they are actually in.
	local bfound = books_found(c)
	local parts = {}

	-- ⚠️⚠️ EMITTED BY HAND BECAUSE `ipairs` STARTS AT 1. `regions.REGIONS` is a 1-based array, so the
	-- loop below can never reach index 0 -- an always-available region would pass every gate and have
	-- no row in the window: reachable, and invisible.
	do
		local f0, b0 = 0, 0
		for _, sp in ipairs(M.list(c)) do if sp.region == M.ALWAYS_REGION then f0 = f0 + 1 end end
		for _, sp in ipairs(bfound)    do if sp.region == M.ALWAYS_REGION then b0 = b0 + 1 end end
		-- ⚠️ Unknown is always 0 here: nothing in this region is discoverable, and a denominator would
		-- advertise destinations that do not exist and can never be found.
		parts[#parts + 1] = string.format("%d|%s|%d|%d|%d",
			M.ALWAYS_REGION, M.ALWAYS_NAME, 1, f0 + b0, 0)
	end

	for id, rname in ipairs(regions.REGIONS) do
		local found = 0
		for _, sp in ipairs(M.list(c)) do
			if sp.region == id then found = found + 1 end
		end
		local bf = 0
		for _, sp in ipairs(bfound) do
			if sp.region == id then bf = bf + 1 end
		end
		-- ⚠️ Unknown = field spots not yet walked + books in this region not yet attuned. Both are
		-- "still out there to find", and the player has no reason to care which kind a row will be.
		local unknown = M.unknown_count(c, id) + math.max(0, books_total(id) - bf)
		parts[#parts + 1] = string.format("%d|%s|%d|%d|%d",
			id, rname, M.is_open(c, id) and 1 or 0, found + bf, unknown)
	end

	c:Message(MT.NPCQuestSay, string.format("TRAVELDATA %d^%s", #parts, table.concat(parts, "^")))

	local dparts = {}
	for _, sp in ipairs(M.list(c)) do
		dparts[#dparts + 1] = string.format("%d|%s|%s", sp.region, sp.id, sp.name)
	end
	for _, sp in ipairs(bfound) do
		dparts[#dparts + 1] = string.format("%d|%s|%s", sp.region, sp.id, sp.name)
	end
	c:Message(MT.NPCQuestSay, string.format("TRAVELDEST %d^%s", #dparts, table.concat(dparts, "^")))
end

-- Clicking a Plane of Knowledge book opens it. ⚠️ This is the ONLY way it opens -- see the note on
-- M.handle_say about why there is no command.
-- ⚠️⚠️ THIS IS THE ONLY PLACE A BOOK CLICK IS RECORDED, AND IT IS WHAT AUTHORISES TRAVEL.
-- The window can also be opened from the /aot launcher as a read-only map, and the dll hides its
-- Travel button there -- but that is PRESENTATION. A modified client can un-hide it and send
-- `travelgo` from anywhere, so the rule has to live here: `M.request` refuses unless this stamp is
-- recent. §16 records the same split for the Advanced Loot buttons.
-- 📌 A stamp with a window, rather than "are you standing near a book". Travel already moves you the
-- instant you confirm, and a proximity test would have to survive the confirmation popup round trip;
-- a timestamp does not care where you drift to while reading the list.
M.BOOK_GRACE_SECS = 180

function M.open_window(c)
	if not c or not c.valid then return end
	eq.set_data(bookkey(c), tostring(os.time()))
	c:Message(MT.NPCQuestSay, "TRAVELOPEN")
	M.send_window(c)
end

function M.handle_say(e)
	local msg0 = (e.message or ""):lower()

	-- The window asking for both lists.
	if msg0:match("^travelwin%s*$") then
		M.send_window(e.self)
		return true
	end

	-- The window requesting a trip: "travelgo <id> <group> <auto>".
	-- ⚠️ Every one of these is re-checked server side in M.request -- the dll validates nothing, so a
	-- modified client sending this by hand gains only a refusal in chat.
	local gid, ggrp, gauto = msg0:match("^travelgo%s+([%a%d_]+)%s+([01])%s+([01])%s*$")
	if gid then
		M.request(e.self, gid, ggrp == "1", gauto == "1")
		return true
	end

	return M.handle_say_list(e)
end

function M.handle_say_list(e)
	local msg = (e.message or ""):lower()

	if msg:match("^travel%s*$") then
		local c = e.self
		local found = M.list(c)
		if #found == 0 then
			c:Message(MT.Yellow,
				"You have discovered no waypoints yet. They are hidden out in the world; walk over one to learn it.")
			return true
		end

		-- ⚠️ Books are listed INSIDE their own region, exactly as the window groups them -- the two
		-- must agree or the printout becomes a second, differently-shaped answer to the same question.
		local bfound = books_found(c)
		c:Message(MT.Yellow, "Places you have committed to memory:")
		for id, rname in ipairs(regions.REGIONS) do
			local any = false
			local function header()
				if not any then
					c:Message(MT.Yellow, string.format("  %s%s", rname,
						M.is_open(c, id) and "" or "  (not open to you)"))
					any = true
				end
			end
			for _, sp in ipairs(found) do
				if sp.region == id then header(); c:Message(MT.Yellow, "    " .. sp.name) end
			end
			local bf = 0
			for _, sp in ipairs(bfound) do
				if sp.region == id then
					header(); c:Message(MT.Yellow, "    " .. sp.name .. "  (book)")
					bf = bf + 1
				end
			end
			local unknown = M.unknown_count(c, id) + math.max(0, books_total(id) - bf)
			if any and unknown > 0 then
				c:Message(MT.Yellow, string.format("    ...and %d you have not found", unknown))
			end
		end

		c:Message(MT.Yellow, "Travel from a Plane of Knowledge book.")
		return true
	end

	return false
end

return M
