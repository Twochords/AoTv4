-- aotv4_fellowship.lua -- 12-person social group, persistent chat, and a travellable campfire.
--
-- Spec: FELLOWSHIP_DESIGN.md. Roster + chat + campfire travel + a component vendor as a platinum
-- sink; XP sharing and Vitality were declined.
--
-- ⚠️⚠️ EQEmu IMPLEMENTS NONE OF THIS. `OP_FellowshipUpdate` is an opcode constant with no handler,
-- `Class::FellowshipMaster` (69) is defined and unused, and there are NO fellowship tables. Nothing
-- here is "turning a feature on" -- it is built the way every other AoTv4 system is: Lua state in
-- data buckets, a say protocol, and (later) a native window. Do not go looking for engine support.
--
-- ⚠️ All buckets are GLOBAL (`eq.set_data` with no client), because a fellowship spans zones and
-- every zone is a separate process.

local regions = require("aotv4_regions")   -- REGIONS[id] -> display name, for refusal messages

local M = {}

M.MAX_MEMBERS   = 12
M.FIRE_HOURS    = 4                        -- hard cap, even with people sitting on it
M.FIRE_IDLE_SEC = 30 * 60                  -- ⚠️ see NOTE-IDLE: unattended fires die in 30 minutes
M.FIRE_RANGE    = 50                       -- how close counts as "at the fire"
M.SWEEP_SECS    = 30                       -- proximity/idle/presence tick, per client
M.PRESENCE_STALE= 90                       -- a member stamped longer ago than this reads offline
M.TRAVEL_CD_SEC = 15 * 60                  -- matches live's insignia recast
M.INVITE_SEC    = 120                      -- how long an unanswered invitation stands
-- ⚠️⚠️ THE CAMPFIRE IS AN NPC ON RACE 567, AND IT HAD TO BE. It was a GROUND OBJECT built from
-- IT78_ACTORDEF, which the `#door create` help does name as a campfire -- but a ground object is
-- wrong here for two reasons that have nothing to do with how it looks:
--   1. ⚠️⚠️ CLICKING ONE OPENS A CONTAINER WINDOW, AND NO OBJECT TYPE AVOIDS IT.
--      `Object::HandleClick` (zone/object.cpp:582) branches only on `m_type == Temporary`; every
--      other type -- including 0 `StaticLocked`, whose name suggests otherwise -- falls into the
--      else branch and sends OP_ClickObjectAction, opening the tradeskill/bag panel. Reported from
--      play as "when I click on the placed fire, I can open a bag it seems?".
--   2. ⚠️⚠️ IT CANNOT BE REMOVED. Lua can CREATE a ground object or a door
--      (`create_ground_object_from_model`, `create_door`) but there is NO binding that removes
--      either -- only decay takes them down. So "destroy camp" reported the fire out while the
--      model kept burning. Reported as exactly that.
-- An NPC fixes both outright: `bodytype 11` (NoTarget) means the click never happens, and `Depop()`
-- IS bound. Race 567 is the model the stock campfires (npc 700114 `Campfire` and a dozen others)
-- already use, so it renders as a real fire in every zone.
-- 📌 Precedent: the delve chest (section 24) is likewise scenery rendered as an NPC.
-- 📌 `eq.spawn2` spawns are RUNTIME ONLY -- no spawn2 row is written -- so a fire in a zone nobody
-- is in disappears when that zone unloads, with nothing to clean up.
M.FIRE_NPC = 2000412

-- ⚠️⚠️ THE FIRE IS LIT BY CLICKING AN ITEM, NOT BY A CHAT COMMAND. You buy a Kit and a Lumber
-- Bundle, COMBINE them in an Artisan's Universal Kit, and click what comes out -- the fire appears
-- at your feet. `fshiplight` survives as a fallback but is not the intended route.
-- 📌 Deliberately a CONSUMABLE, unlike live's reusable insignia: the design refuses to put a
-- permanent enabling item in players' bags, because the roguelite death wipe destroys anything not
-- in `death_loss.M.is_kept` -- the trap that cost the tradeskill tools permanently (section 32).
-- A consumable you buy again has nothing to lose.
M.FIRE_ITEM = {
	[147972] = "basic",    -- Simple Campfire   -- free, from the Fellowmaster
	[147973] = "health",   -- Warded Campfire   -- Kit + 1 Lumber
	[147974] = "vigor",    -- Enduring Campfire -- Kit + 2 Lumber
}
-- ⚠️⚠️ THE INSIGNIA IS THE ONLY PERMANENT ITEM HERE, AND IT IS ONLY SAFE BECAUSE
-- `death_loss.M.is_kept` EXEMPTS IT. Everything else this module hands out is a consumable for
-- exactly that reason -- the roguelite wipe destroys anything outside that list, which is how the
-- tradeskill tools were permanently lost (section 32). An insignia was deliberately not built until
-- the exemption existed. ⚠️ Kept in step with death_loss and migration v84.
M.INSIGNIA      = 147975                   -- clicked to travel to the fellowship campfire
M.KIT_ITEM      = 147970                   -- Campfire Kit    -- buys the fire
M.LUMBER_ITEM   = 147971                   -- Lumber Bundle   -- buys the fire's TYPE

-- Fire types. `basic` is FREE and carries no buff, deliberately: it is the travel target, and
-- gating travel behind coin means a broke fellowship cannot use the feature at all.
-- ⚠️ NOTE-BUFFS: live's health fire is ~1500 HP. At our level 30 cap a character has ~1,500 HP, so
-- copying that number DOUBLES them. These are derived for this server instead, and are first
-- guesses that want checking in play.
M.FIRES = {
	basic  = { name = "Fellowship of Honor",   spell = nil,   cost = false },
	health = { name = "Fellowship of Health",  spell = 44330, cost = true  },
	vigor  = { name = "Fellowship of Vigor",   spell = 44331, cost = true  },
}

-- ---------------------------------------------------------------- state
local function of_key(cid)    return "fship_of_"   .. cid end
local function ros_key(fid)   return "fship_"      .. fid end
local function name_key(fid)  return "fship_name_" .. fid end
local function fire_key(fid)  return "fship_fire_" .. fid end
local function cd_key(cid)    return "fship_cd_"   .. cid end

local function num(s) return tonumber(s) or 0 end

function M.fid_of(client)
	if not client then return nil end
	local v = num(eq.get_data(of_key(client:CharacterID())))
	return v > 0 and v or nil
end

local function invite_key(cid) return "fship_inv_" .. cid end

-- roster -> leader_charid, { charid, ... }
function M.roster(fid)
	local raw = eq.get_data(ros_key(fid)) or ""
	local leader, rest = raw:match("^(%d+)|(.*)$")
	if not leader then return nil, {} end
	local out = {}
	for s in (rest or ""):gmatch("([^,]+)") do
		local n = tonumber(s)
		if n then out[#out + 1] = n end
	end
	return tonumber(leader), out
end

-- ⚠️⚠️ THE TWO BUCKETS ARE WRITTEN TOGETHER, ALWAYS. `fship_of_` is the reverse index and is what
-- every hook reads FIRST; a stale one leaves a character receiving fellowship chat for a roster that
-- no longer lists them, or holding a fid that has been disbanded. Nothing may write one alone.
local function save_roster(fid, leader, members)
	eq.set_data(ros_key(fid), tostring(leader) .. "|" .. table.concat(members, ","))
	for _, cid in ipairs(members) do eq.set_data(of_key(cid), tostring(fid)) end
end

local function forget(cid)
	eq.set_data(of_key(cid), "0")
end

function M.name_of(fid) return eq.get_data(name_key(fid)) or ("Fellowship " .. fid) end

local function drop(members, cid)
	local out = {}
	for _, m in ipairs(members) do if m ~= cid then out[#out + 1] = m end end
	return out
end

-- ---------------------------------------------------------------- messaging
-- ⚠️⚠️ THERE IS NO `cross_zone_*_by_fellowship`, AND THERE CANNOT BE -- the engine has no concept of
-- one. Coverage is assembled from the roster, exactly as section 18's #worldbuff assembles
-- server-wide coverage from four hooks because no single call reaches everybody. At a cap of 12 the
-- loop is free.
-- ⚠️ Offline members are skipped silently: the binding is a no-op for them, which is what we want.
function M.broadcast(fid, text, skip_cid)
	local _, members = M.roster(fid)
	for _, cid in ipairs(members) do
		if cid ~= skip_cid then
			eq.cross_zone_message_player_by_char_id(MT.Yellow, cid, text)
		end
	end
end

-- ---------------------------------------------------------------- roster commands
function M.create(client, name)
	if M.fid_of(client) then
		client:Message(MT.Red, "You are already in a fellowship.")
		return
	end
	name = (name or ""):gsub("[^%w %-']", ""):sub(1, 32)
	if name == "" then
		client:Message(MT.Red, "Name your fellowship: /say fshipform <name>")
		return
	end
	local fid = num(eq.get_data("fship_next")) + 1
	eq.set_data("fship_next", tostring(fid))
	eq.set_data(name_key(fid), name)
	save_roster(fid, client:CharacterID(), { client:CharacterID() })
	client:Message(MT.Yellow, string.format("You form the fellowship '%s'.", name))
end

function M.invite(client, target_name)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	local leader, members = M.roster(fid)
	if leader ~= client:CharacterID() then
		client:Message(MT.Red, "Only the fellowship leader may invite.")
		return
	end
	if #members >= M.MAX_MEMBERS then
		client:Message(MT.Red, string.format("A fellowship holds at most %d.", M.MAX_MEMBERS))
		return
	end
	-- ⚠️ Invite targets someone IN THIS ZONE: resolving a name server-wide would need a cross-zone
	-- lookup the engine does not offer, and handing an invite to somebody who cannot answer is worse
	-- than refusing. Same reasoning as the group-travel rule in aotv4_travel.
	-- ⚠️ A BLANK NAME MEANS "MY TARGET" -- that is how the native window's Invite works, and the
	-- window sends the editbox contents verbatim, so an empty box must not simply fail.
	-- ⚠️⚠️ `GetTarget()` RETURNS A `Lua_Mob`, AND `CharacterID` IS ONLY ON `Lua_Client` (section 24).
	-- Passing the target straight through is "attempt to call method 'CharacterID' (a nil value)" the
	-- moment anyone invites by target instead of by name -- at runtime, with nothing to warn you at
	-- write time. Resolve through the entity list, which also rejects an NPC target for free.
	local t
	if (target_name or "") == "" then
		local tg = client:GetTarget()
		if tg and tg.valid then t = eq.get_entity_list():GetClientByID(tg:GetID()) end
	else
		t = eq.get_entity_list():GetClientByName(target_name)
	end
	if not t or not t.valid then
		client:Message(MT.Red, "They must be in this zone to be invited.")
		return
	end
	if M.fid_of(t) then
		client:Message(MT.Red, t:GetCleanName() .. " is already in a fellowship.")
		return
	end
	-- ⚠️⚠️ AN INVITE IS AN OFFER, NOT A CONSCRIPTION. It used to add the target on the spot, so
	-- anyone could pull anyone into a fellowship with no say in it -- and a fellowship is exclusive
	-- (you can only be in one), so being taken into somebody's meant being locked out of your own.
	-- Reported from play as "when you invite someone, it auto accepts, it doesnt give them an option
	-- to say no".
	-- 📌 The prompt is SAYLINKS IN CHAT rather than a dialog box, for the reason the whole window
	-- exists: `OP_FellowshipUpdate` has no wire opcode on this client, so there is no native invite
	-- popup to drive. Saylinks are the established answer here (pok_travel, aa_choice).
	eq.set_data(invite_key(t:CharacterID()),
		string.format("%d|%d|%d", fid, client:CharacterID(), os.time() + M.INVITE_SEC))
	t:Message(MT.Yellow, string.format("%s invites you to join the fellowship '%s'.",
		client:GetCleanName(), M.name_of(fid)))
	t:Message(MT.Yellow, string.format("    %s    %s",
		eq.say_link("fshipaccept", true, "[ Accept ]"),
		eq.say_link("fshipdecline", true, "[ Decline ]")))
	client:Message(MT.Yellow, string.format("You invite %s. They have %d seconds to answer.",
		t:GetCleanName(), M.INVITE_SEC))
end

-- ⚠️ The offer carries its own expiry rather than leaning on a bucket TTL, exactly as the campfire
-- record does -- one way of expressing "this lapses", checked on read.
function M.accept(client)
	local raw3 = eq.get_data(invite_key(client:CharacterID())) or ""
	local fid, inviter, expiry = raw3:match("^(%d+)|(%d+)|(%d+)$")
	if not fid then client:Message(MT.Red, "You have no fellowship invitation.") return end
	eq.set_data(invite_key(client:CharacterID()), "")   -- consumed either way
	if os.time() > tonumber(expiry) then
		client:Message(MT.Red, "That invitation has expired.")
		return
	end
	-- ⚠️⚠️ EVERY GATE IS RE-TESTED HERE, NOT JUST AT INVITE TIME. Between the offer and the answer
	-- the fellowship can have been disbanded, filled to MAX_MEMBERS, or the invitee can have joined
	-- or formed another one -- and accepting into a full or dead fellowship would corrupt the roster
	-- rather than fail.
	fid = tonumber(fid)
	if M.fid_of(client) then client:Message(MT.Red, "You are already in a fellowship.") return end
	local leader, members = M.roster(fid)
	if not leader then client:Message(MT.Red, "That fellowship no longer exists.") return end
	if #members >= M.MAX_MEMBERS then
		client:Message(MT.Red, "That fellowship is full.")
		return
	end
	members[#members + 1] = client:CharacterID()
	save_roster(fid, leader, members)
	M.stamp(client)   -- ⚠️ so their row has a level, class and zone before the first sweep
	M.broadcast(fid, string.format("%s joins the fellowship.", client:GetCleanName()))
	client:Message(MT.Yellow, string.format("You join the fellowship '%s'.", M.name_of(fid)))
	M.send_data(client)
	local inv = eq.get_entity_list():GetClientByCharID(tonumber(inviter))
	if inv and inv.valid then M.send_data(inv) end
end

function M.decline(client)
	local raw4 = eq.get_data(invite_key(client:CharacterID())) or ""
	local fid, inviter = raw4:match("^(%d+)|(%d+)|%d+$")
	eq.set_data(invite_key(client:CharacterID()), "")
	if not fid then client:Message(MT.Red, "You have no fellowship invitation.") return end
	client:Message(MT.Yellow, "You decline the invitation.")
	local inv = eq.get_entity_list():GetClientByCharID(tonumber(inviter))
	if inv and inv.valid then
		inv:Message(MT.Red, client:GetCleanName() .. " declines your fellowship invitation.")
	end
end

-- ⚠️ Last one out disbands: the roster bucket is cleared so a stale fid can never be re-read, and
-- the fire goes with it (a campfire nobody belongs to is a free door for whoever remembers the id).
function M.leave(client)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	local leader, members = M.roster(fid)
	local cid = client:CharacterID()
	members = drop(members, cid)
	forget(cid)

	if #members == 0 then
		eq.set_data(ros_key(fid), "")
		eq.set_data(name_key(fid), "")
		eq.set_data(fire_key(fid), "")
		client:Message(MT.Yellow, "You leave, and the fellowship disbands.")
		return
	end
	-- leader left: hand it to the longest-standing remaining member
	if leader == cid then leader = members[1] end
	save_roster(fid, leader, members)
	client:Message(MT.Yellow, "You leave the fellowship.")
	M.broadcast(fid, string.format("%s has left the fellowship.", client:GetCleanName()))
end

function M.kick(client, target_name)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	local leader, members = M.roster(fid)
	if leader ~= client:CharacterID() then
		client:Message(MT.Red, "Only the fellowship leader may remove members.")
		return
	end
	for _, cid in ipairs(members) do
		if (eq.get_char_name_by_id(cid) or ""):lower() == (target_name or ""):lower() then
			if cid == leader then client:Message(MT.Red, "You cannot remove yourself; leave instead.") return end
			save_roster(fid, leader, drop(members, cid))
			forget(cid)
			eq.cross_zone_message_player_by_char_id(MT.Red, cid, "You have been removed from the fellowship.")
			M.broadcast(fid, string.format("%s was removed from the fellowship.", target_name))
			return
		end
	end
	client:Message(MT.Red, "No such member.")
end

-- ---------------------------------------------------------------- chat
function M.say(client, text)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	if (text or "") == "" then return end
	local line = string.format("[%s] %s: %s", M.name_of(fid), client:GetCleanName(), text)
	client:Message(MT.Yellow, line)
	M.broadcast(fid, line, client:CharacterID())
end

-- ---------------------------------------------------------------- the campfire
-- ⚠️⚠️ THE FIRE IS A BUCKET; THE NPC IS ONLY A RENDERING OF IT. Zones are dynamic and an idle one
-- self-terminates (section 2), and `eq.spawn2` is zone-local -- so a campfire that existed only as a
-- spawned NPC would die the moment its zone emptied, which is exactly the case that matters for a
-- fire whose whole purpose is "somewhere for people who are NOT here to travel to". A 6 hour
-- duration would in practice mean "until the last person walks out".
-- The first player to enter the zone spawns the model (M.on_enter_zone). Same pattern as the world
-- boss (section 17c), which exists for the same reason.
-- Parse without the two expiry tests. ⚠️ ONLY for cleanup: `M.fire` is the one that decides whether
-- a fire is LIVE, and travel/buff must never call this instead -- that is how a lapsed fire becomes
-- travellable again.
function M.fire_raw(fid)
	local raw = eq.get_data(fire_key(fid)) or ""
	local zone, x, y, z, h, expiry, ftype = raw:match("^([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)$")
	if not zone or zone == "" then return nil end
	return { zone = zone, x = tonumber(x), y = tonumber(y), z = tonumber(z), h = tonumber(h),
	         expiry = num(expiry), ftype = ftype }
end

function M.fire(fid)
	local raw = eq.get_data(fire_key(fid)) or ""
	local zone, x, y, z, h, expiry, ftype = raw:match("^([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)|([^|]*)$")
	if not zone or zone == "" then return nil end
	-- ⚠️ Expiry is checked on READ as well as swept, so a lapsed fire cannot be travelled to even if
	-- its model is still standing in a zone somebody happens to be in.
	if num(expiry) <= os.time() then return nil end
	-- ⚠️⚠️ NOTE-IDLE: A FIRE NOBODY IS AT DIES IN 30 MINUTES, and this is a scarcity rule rather than
	-- tidiness. There are no instances here, so every campfire is planted in the shared world -- a
	-- 4 hour fire that nobody attends is 4 hours of somebody else's camp with a model parked in it,
	-- and several fellowships doing that closes off real estate for everybody. `fship_seen_` is
	-- refreshed by any member standing within FIRE_RANGE, so an ATTENDED fire lives its full span
	-- and an abandoned one is gone in half an hour.
	-- ⚠️ Tested on READ, exactly like the hard expiry, so travel and the buff both refuse a fire that
	-- has gone cold even before the sweep gets round to depopping its model.
	local seen = num(eq.get_data("fship_seen_" .. fid))
	if seen > 0 and (os.time() - seen) > M.FIRE_IDLE_SEC then return nil end
	return { zone = zone, x = tonumber(x), y = tonumber(y), z = tonumber(z), h = tonumber(h),
	         expiry = num(expiry), ftype = ftype }
end

function M.light(client, ftype, from_item)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	ftype = (ftype or "basic"):lower()
	local def = M.FIRES[ftype]
	if not def then client:Message(MT.Red, "Unknown campfire type.") return end

	-- ⚠️⚠️ OPEN WORLD ONLY. A delve and a difficulty shard are both instances, and a fire inside one
	-- is a permanent door into another player's private instance. Test the INSTANCE ID, not the zone
	-- name -- a difficulty shard is the same zone as the open world it copies.
	if eq.get_zone_instance_id() ~= 0 then
		client:Message(MT.Red, "A campfire cannot be lit inside an instance.")
		return
	end

	-- ⚠️⚠️ ONE CAMPFIRE PER FELLOWSHIP, AND IT IS A REFUSAL RATHER THAN A REPLACEMENT. Lighting a
	-- second used to overwrite the record and try to despawn the old model -- but `M.despawn_fire` is
	-- zone-local (Depop only reaches this zone), so a fire lit in another zone became an ORPHAN: still
	-- burning, no longer in any record, and impossible to put out because douse could not find it
	-- either. Refusing is the only version of "one at a time" that cannot leak.
	-- ⚠️ Checked BEFORE the components are charged, or the player pays for a fire they are refused.
	local burning = M.fire(fid)
	if burning then
		client:Message(MT.Red, string.format(
			"Your fellowship already has a campfire burning in %s. Put that one out first.",
			eq.get_zone_long_name_by_name(burning.zone) or burning.zone))
		return
	end

	-- ⚠️ `from_item` means the CLICKED ITEM was the cost -- it has already been combined and is about
	-- to be destroyed by its own script, so charging components again would bill twice for one fire.
	if def.cost and not from_item then
		if not client:HasItem(M.KIT_ITEM) or not client:HasItem(M.LUMBER_ITEM) then
			client:Message(MT.Red, "That campfire needs a Campfire Kit and a Lumber Bundle.")
			return
		end
		client:RemoveItem(M.KIT_ITEM, 1)
		client:RemoveItem(M.LUMBER_ITEM, 1)
	end

	-- ⚠️⚠️ SNAP TO THE GROUND, DO NOT USE THE PLAYER'S Z. A ground object is placed at exactly the z
	-- given, and the player's own z sits at their feet plus whatever the model's origin adds -- the
	-- fire hung visibly in the air. `FindGroundZ` returns BEST_Z_INVALID when the zone has no map
	-- loaded, so the player's z stays as the fallback rather than dropping the fire to -999999.
	local gx, gy = client:GetX(), client:GetY()
	local gz = client:FindGroundZ(gx, gy)
	if not gz or gz < -99999 then gz = client:GetZ() end

	local expiry = os.time() + M.FIRE_HOURS * 3600
	eq.set_data(fire_key(fid), string.format("%s|%.2f|%.2f|%.2f|%.2f|%d|%s",
		eq.get_zone_short_name(), gx, gy, gz, client:GetHeading(), expiry, ftype))
	eq.set_data("fship_seen_" .. fid, tostring(os.time()))   -- lighting it counts as attending it
	M.spawn_fire(fid, gx, gy, gz, client:GetHeading())
	M.broadcast(fid, string.format("%s lights a %s in %s.",
		client:GetCleanName(), def.name, eq.get_zone_long_name()))
	client:Message(MT.Yellow, string.format("You light a %s. It burns for %d hours.", def.name, M.FIRE_HOURS))
end

-- ⚠️⚠️ THE ENTITY VARIABLE IS THE ONLY LINK BETWEEN A BURNING FIRE AND ITS FELLOWSHIP. Two
-- fellowships can have a fire in the same zone, so `Depop` has to find the right one -- and an
-- entity variable is right where a data bucket would be wrong: it describes THIS SPAWN, whereas a
-- bucket keyed on the npc type would make every future campfire in the world belong to whoever lit
-- one first. Same reasoning as the difficulty champions (section 43).
function M.spawn_fire(fid, x, y, z, h)
	-- ⚠️⚠️ `eq.spawn2` RETURNS A `Lua_Mob`, NOT A `Lua_NPC` -- SetEntityVariable is on Mob so it is
	-- fine here, but anything NPC-only would be "attempt to call a nil value" at runtime with
	-- nothing to warn you at write time (section 26).
	local m = eq.spawn2(M.FIRE_NPC, 0, 0, x, y, z, h or 0)
	if m and m.valid then m:SetEntityVariable("fship", tostring(fid)) end
	return m
end

-- The fire belonging to `fid` in THIS zone, or nil.
-- ⚠️⚠️ THE ENTITY VARIABLE CANNOT BE THE ONLY HANDLE ON THE FIRE. It was, and when it failed to take
-- there was no way left to find the NPC -- so `M.despawn_fire` silently matched nothing, "Destroy
-- Camp" announced the fire was out, and it kept burning. Reported from play as exactly that. The
-- RECORDED POSITION is the fallback: we wrote it when the fire was lit, so it is known independently
-- of anything stored on the spawned entity.
-- ⚠️ The entity variable is still preferred and returns immediately -- position is only consulted
-- when nothing claims the fellowship, so two fellowships with fires in one zone still resolve
-- correctly unless they are within FIRE_MATCH_RANGE of each other, which the one-fire-at-a-time rule
-- in M.light makes vanishingly rare.
-- ⚠️ Takes the record as an argument so a caller that is about to CLEAR it can still pass it in --
-- see M.douse, where looking it up here would find nothing.
M.FIRE_MATCH_RANGE = 5

function M.find_fire(fid, rec)
	rec = rec or M.fire(fid)
	if not rec then return nil end
	if rec.zone ~= (eq.get_zone_short_name() or "") then return nil end
	local want, near = tostring(fid), nil
	for n in eq.get_entity_list():GetNPCList().entries do
		if n.valid and n:GetNPCTypeID() == M.FIRE_NPC then
			if n:GetEntityVariable("fship") == want then return n end
			local dx, dy = n:GetX() - rec.x, n:GetY() - rec.y
			if (dx * dx + dy * dy) <= (M.FIRE_MATCH_RANGE * M.FIRE_MATCH_RANGE) then near = n end
		end
	end
	return near
end

-- ⚠️ Only ever removes a fire in the CALLER'S zone -- Depop is inherently zone-local. A fire in a
-- zone the douser is not standing in is cleared by the first member to walk into it (M.on_enter_zone
-- and the proximity tick both reconcile), or by that zone unloading.
function M.despawn_fire(fid, rec)
	local n = M.find_fire(fid, rec)
	if n then n:Depop() return true end
	return false
end

-- The first member into a zone holding a live fire raises its model.
-- ⚠️ Guarded on the instance id as well as the zone, so a shard or delve copy of the same zone never
-- renders somebody else's open-world fire.
function M.on_enter_zone(e)
	local c = e.self
	if not c then return end
	local fid = M.fid_of(c)
	if not fid then return end
	local f = M.fire(fid)
	if not f then return end
	if f.zone ~= (eq.get_zone_short_name() or "") or eq.get_zone_instance_id() ~= 0 then return end
	-- ⚠️⚠️ ASK WHETHER THE FIRE IS ALREADY THERE, DO NOT TRACK IT IN A BUCKET. The object version
	-- had to guess with a timestamp, because there was no way to look an object up -- and every
	-- member walking in stacked another campfire when the guess was wrong. An NPC can simply be
	-- found, so the stacking question disappears rather than being managed.
	if M.find_fire(fid) then return end
	M.spawn_fire(fid, f.x, f.y, f.z, f.h)
end

-- Clicked one of the campfire items. Returns true if the item should be consumed.
-- ⚠️⚠️ CONSUME ONLY ON SUCCESS. `M.light` refuses inside an instance, and with no fellowship there is
-- nothing to light -- destroying the item on either would take the player's 300p purchase and give
-- nothing back. Same ordering rule the reroll and the Tome of Insight both follow.
function M.light_from_item(client, item_id)
	local ftype = M.FIRE_ITEM[item_id]
	if not ftype then return false end
	if not M.fid_of(client) then
		client:Message(MT.Red, "You must be in a fellowship to light a campfire.")
		return false
	end
	local before = eq.get_data(fire_key(M.fid_of(client))) or ""
	M.light(client, ftype, true)
	return (eq.get_data(fire_key(M.fid_of(client))) or "") ~= before
end

-- ---------------------------------------------------------------- the fire's buff
-- ⚠️⚠️ THE BUFF IS FELLOWSHIP-ONLY BY CONSTRUCTION, NOT BY A FILTER. This runs per CLIENT off that
-- client's own timer, and the very first thing it does is read THEIR fellowship and THEIR fire. A
-- passer-by has no fid, or a different one, so there is no code path on which they can be buffed --
-- rather than an area effect that remembers to exclude outsiders and one day forgets.
-- ⚠️⚠️ AN `auras` ROW WOULD BE WRONG HERE, which is why this is hand-rolled: `aura_type 1` is
-- OnAllGroupMembers, and a fellowship is NOT a group. Half your fellowship would get nothing, and
-- any non-member grouped with you would get it.
--
-- ⚠️ ApplySpellBuff, not SpellFinished -- the same reasoning as the world buff: SpellFinished runs a
-- real cast that can be resisted and honours target type, neither of which is wanted for "you are
-- sitting at your fellowship's fire". Guarded by FindBuff so it does not reset the duration every
-- sweep.
function M.proximity_tick(client)
	if not client or not client.valid then return end
	local fid = M.fid_of(client)
	if not fid then return end

	-- ⚠️⚠️ PRESENCE IS STAMPED HERE BECAUSE THE ENGINE OFFERS NO WAY TO ASK. There is no
	-- `is_online` binding and no cross-zone "where is this character" call, so the roster would
	-- otherwise be able to report only the members standing in the SAME zone as the viewer -- and
	-- show everyone else as offline, which for a group that spans zones is worse than useless.
	-- Each member's own client stamps every SWEEP_SECS; anything stamped within PRESENCE_STALE
	-- counts as online. ⚠️ Stamped BEFORE every fire test, or a member nowhere near a campfire would
	-- never register.
	M.stamp(client)
	local f = M.fire(fid)
	if not f then
		-- ⚠️ The MODEL needs no cleanup: it was created with a decay of FIRE_IDLE_SEC and removes
		-- itself. Only the bucket is cleared, and only once -- the emptiness test is what stops this
		-- announcing "burned out" to the fellowship on every 30-second tick thereafter.
		if (eq.get_data(fire_key(fid)) or "") ~= "" then
			eq.set_data(fire_key(fid), "")
			M.despawn_fire(fid)
			M.broadcast(fid, "Your fellowship's campfire has burned out.")
		end
		return
	end
	if f.zone ~= (eq.get_zone_short_name() or "") or eq.get_zone_instance_id() ~= 0 then return end
	if client:CalculateDistance(f.x, f.y, f.z) > M.FIRE_RANGE then return end

	-- Attending the fire keeps it alive -- see NOTE-IDLE on M.fire.
	eq.set_data("fship_seen_" .. fid, tostring(os.time()))

	-- The fire is raised by whoever arrives first; if it is somehow absent, put it back.
	if not M.find_fire(fid) then M.spawn_fire(fid, f.x, f.y, f.z, f.h) end

	local def = M.FIRES[f.ftype]
	if def and def.spell and not client:FindBuff(def.spell) then
		client:ApplySpellBuff(def.spell)
	end
end

-- ---------------------------------------------------------------- travel
-- ⚠️⚠️ RESOLVE EVERY GATE BEFORE MOVING ANYONE, and stamp the cooldown ONLY on a successful move.
-- Section 43 records the difficulty shift consuming its cooldown while declining to move the player;
-- that ordering bug has now bitten twice, so it is written out explicitly here.
function M.travel(client)
	local fid = M.fid_of(client)
	if not fid then client:Message(MT.Red, "You are not in a fellowship.") return end
	local f = M.fire(fid)
	if not f then client:Message(MT.Red, "Your fellowship has no burning campfire.") return end

	local cd = num(eq.get_data(cd_key(client:CharacterID())))
	if cd > os.time() then
		client:Message(MT.Red, string.format("You must rest %d more minute(s) before travelling again.",
			math.ceil((cd - os.time()) / 60)))
		return
	end

	-- ⚠️ Refused in combat. Same predicate delve entry and the difficulty shift use: without it this
	-- is a BETTER escape than Gate -- instant, no reagent, no cast time, and it breaks every hate
	-- list at once. Live's "not in combat mode, must be resting" is the same intent.
	if client:GetAggroCount() > 0 then
		client:Message(MT.Red, "Not while something is hunting you.")
		return
	end

	-- ⚠️ Leaving from inside a delve or a shard strands that run.
	if eq.get_zone_instance_id() ~= 0 then
		client:Message(MT.Red, "Not from inside an instance.")
		return
	end

	local dest = eq.get_zone_id_by_name(f.zone)
	if not dest or dest == 0 then client:Message(MT.Red, "That campfire's zone no longer exists.") return end

	-- ⚠️⚠️ THE DESTINATION REGION MUST BE UNLOCKED, AND THIS IS THE GATE THAT MATTERS MOST. Without
	-- it a friend standing in a locked region is a free door into it -- the campfire becomes a
	-- region-lock bypass. GMs are exempt, matching RegionManager::CanEnterZone.
	local region = eq.get_zone_region(dest)
	if not client:GetGM() and not client:HasRegion(region) then
		client:Message(MT.Red, string.format("You have not opened %s.",
			regions.REGIONS[region] or "that region"))
		return
	end

	eq.set_data(cd_key(client:CharacterID()), tostring(os.time() + M.TRAVEL_CD_SEC))
	client:MovePC(dest, f.x, f.y, f.z, f.h or 0)
end

-- ---------------------------------------------------------------- display (saylink fallback)
-- ⚠️ The native window is not built yet, so this is the whole UI. Every other AoTv4 system keeps a
-- saylink fallback that works with no client mod; this one is currently relying on it.
function M.show(client)
	local fid = M.fid_of(client)
	if not fid then
		client:Message(MT.Yellow, "You are not in a fellowship. " ..
			eq.say_link("fshipform Wayfarers", true, "[ Form one ]"))
		return
	end
	local leader, members = M.roster(fid)
	client:Message(MT.Yellow, string.format("--- %s (%d/%d) ---", M.name_of(fid), #members, M.MAX_MEMBERS))
	for _, cid in ipairs(members) do
		client:Message(MT.LightBlue, string.format("  %s%s",
			eq.get_char_name_by_id(cid) or ("#" .. cid), cid == leader and "  (leader)" or ""))
	end
	local f = M.fire(fid)
	if f then
		client:Message(MT.Yellow, string.format("Campfire: %s (%s), %d minute(s) left.  %s",
			eq.get_zone_long_name_by_name(f.zone) or f.zone,
			(M.FIRES[f.ftype] or {}).name or f.ftype,
			math.ceil((f.expiry - os.time()) / 60),
			eq.say_link("fshipgo", true, "[ Travel to campfire ]")))
	else
		client:Message(MT.Yellow, "No campfire burning.  " ..
			eq.say_link("fshiplight basic", true, "[ Light one ]"))
	end
end

-- ---------------------------------------------------------------- say routing
-- ⚠️ Returns true when it consumed the message, so global_player can stop.
function M.handle_say(e)
	local c, m = e.self, (e.message or ""):lower()
	if not c then return false end
	local a

	if m == "fshipreq" then M.send_data(c) return true end          -- window refresh
	if m == "fship" then M.show(c) M.send_data(c) return true end   -- saylink view AND window data
	if m == "fshipend"   then M.disband(c) M.send_data(c) return true end
	if m == "fshipaccept"  then M.accept(c)  return true end
	if m == "fshipdecline" then M.decline(c) return true end
	-- ⚠️⚠️ NAME-CARRYING COMMANDS MUST READ THE ORIGINAL MESSAGE, NOT THE LOWERCASED COPY. `m` is
	-- lowercased so the VERBS match whatever case the player typed -- but taking the ARGUMENT from
	-- it lowercases the name too. That created a fellowship called "fellowship" instead of
	-- "Fellowship", and made every invite fail silently, because `GetClientByName` cannot match
	-- "ashrem" to "Ashrem". Reported from play as "create says it worked but I don't see myself in
	-- it, and invite does nothing".
	-- 📌 `arg_after` finds the verb in the LOWERCASED copy and then slices the ORIGINAL, which is
	-- safe because `string.lower` is byte-wise and so cannot change the length. Matching the name
	-- out of `raw` directly would mean spelling every verb as a character class.
	local raw = e.message or ""
	local function arg_after(verb)
		local _, stop = m:find("^" .. verb .. "%s+")
		if not stop then return nil end
		return (raw:sub(stop + 1):gsub("%s+$", ""))
	end

	a = arg_after("fshipform")  if a and a ~= "" then M.create(c, a)      M.send_data(c) return true end
	-- ⚠️ Invite with NO argument means "my current target", as the native window does -- so this one
	-- must accept an empty argument, and cannot use the `a ~= ""` guard the others do.
	if m == "fshipinv" or m:match("^fshipinv%s") then
		M.invite(c, arg_after("fshipinv") or "")
		M.send_data(c)
		return true
	end
	a = arg_after("fshipkick")   if a and a ~= "" then M.kick(c, a)        M.send_data(c) return true end
	a = arg_after("fshipleader") if a and a ~= "" then M.make_leader(c, a) M.send_data(c) return true end
	a = arg_after("fshipmotd") if a then M.set_motd(c, a) return true end
	if m == "fshipleave" then M.leave(c) M.send_data(c) return true end
	if m == "fshipgo"    then M.travel(c) return true end
	a = m:match("^fshiplight%s*(%a*)$") if a then M.light(c, a ~= "" and a or "basic") return true end
	-- ⚠️⚠️ THIS BRANCH WENT MISSING AND `M.douse` BECAME UNREACHABLE. It was lost in the refactor
	-- that fixed the name-casing bug, which rewrote this block wholesale. The Destroy Camp button
	-- kept sending "/say fshipdouse", handle_say fell through, and the dll swallows the echo -- so
	-- the command vanished with NO message, no error and no log line. Reported from play as
	-- "destroy camp is not working".
	-- 📌 A window verb with no handler is completely silent here, by design: every fship* echo is
	-- swallowed so players do not watch their own UI talk. That is exactly what hides a typo, so
	-- the coverage check below exists.
	if m == "fshipdouse" then M.douse(c) return true end
	-- ⚠️ Both of these carry PLAYER-AUTHORED TEXT and must keep its case, which is what
	-- `arg_after` is for -- it finds the verb in the lowercased copy and slices the original.
	a = arg_after("fsay")     if a then M.say(c, a) return true end
	return false
end


-- ---------------------------------------------------------------- window transport
-- ⚠️ Sent to ONE client, never broadcast: it is that player's view of the roster.
-- ⚠️⚠️ LEVEL AND CLASS RIDE IN THE PRESENCE STAMP, NOT LOOKED UP AT SEND TIME. A roster row has to
-- render for members who are OFFLINE or in another zone, and there is no Client object to ask -- so
-- whatever the roster displays about a member has to have been recorded while they were around.
-- Same reason zone and time are here. ⚠️ Anything added to a roster column must be added HERE too,
-- or it will render for the viewer and be blank for everyone else -- which reads as a broken column
-- rather than as missing data.
function M.stamp(client)
	if not client then return end
	eq.set_data("fship_at_" .. client:CharacterID(), string.format("%s|%d|%d|%d",
		eq.get_zone_short_name() or "?", os.time(), client:GetLevel(), client:GetClass()))
end

-- "Last On" as the native column means it: when we last saw them, not a timestamp.
local function ago(t)
	if t <= 0 then return "never" end
	local d = os.time() - t
	if d < 60    then return "moments ago" end
	if d < 3600  then return string.format("%dm ago", math.floor(d / 60)) end
	if d < 86400 then return string.format("%dh ago", math.floor(d / 3600)) end
	return string.format("%dd ago", math.floor(d / 86400))
end

function M.send_data(client)
	if not client then return end
	local fid = M.fid_of(client)
	if not fid then
		client:Message(MT.NPCQuestSay, "FSHIPDATA ")          -- empty name, no rows = "not in one"
		client:Message(MT.NPCQuestSay, "FSHIPFIRE |0|")
		return
	end

	local leader, members = M.roster(fid)
	local fields = {}
	for _, cid in ipairs(members) do
		-- ⚠️ Tolerates the OLD two-field stamp (`zone|time`) as well as the current four-field one:
		-- every member carries a stamp written before level and class were added, and refusing to
		-- parse it would blank the whole row until their next sweep.
		local raw2 = eq.get_data("fship_at_" .. cid) or ""
		local zone, when, lvl, cls = raw2:match("^([^|]*)|(%d*)|(%d*)|(%d*)$")
		if not zone then zone, when = raw2:match("^([^|]*)|(%d*)$") end
		when = tonumber(when) or 0
		local online = when > (os.time() - M.PRESENCE_STALE)
		-- ⚠️⚠️ FIELD ORDER IS THE COLUMN ORDER AND THE DLL DOES NOT NAME THE FIELDS -- it assigns by
		-- position. Insert one in the middle and every column to its right silently shows the wrong
		-- thing, which is exactly how "offline" ended up under Class.
		fields[#fields + 1] = string.format("%s|%d|%s|%s|%s|%d|%s",
			eq.get_char_name_by_id(cid) or ("#" .. cid),
			cid == leader and 1 or 0,
			(tonumber(lvl) or 0) > 0 and tostring(lvl) or "?",
			(tonumber(cls) or 0) > 0 and (eq.get_class_name(tonumber(cls)) or "?") or "?",
			online and (zone ~= "" and eq.get_zone_long_name_by_name(zone) or "?") or "",
			online and 1 or 0,
			online and "Online" or ago(when))
	end
	client:Message(MT.NPCQuestSay, "FSHIPDATA " .. M.name_of(fid) .. "^" .. table.concat(fields, "^"))

	local f = M.fire(fid)
	if f then
		client:Message(MT.NPCQuestSay, string.format("FSHIPFIRE %s|%d|%s",
			eq.get_zone_long_name_by_name(f.zone) or f.zone,
			math.ceil((f.expiry - os.time()) / 60),
			(M.FIRES[f.ftype] or {}).name or f.ftype))
	else
		client:Message(MT.NPCQuestSay, "FSHIPFIRE |0|")
	end
	client:Message(MT.NPCQuestSay, "FSHIPMOTD " .. (eq.get_data("fship_motd_" .. fid) or ""))
end

function M.set_motd(client, text)
	local fid = M.fid_of(client)
	if not fid then return end
	local leader = M.roster(fid)
	if leader ~= client:CharacterID() then
		client:Message(MT.Red, "Only the fellowship leader may set the message of the day.")
		return
	end
	eq.set_data("fship_motd_" .. fid, (text or ""):sub(1, 200))
	M.broadcast(fid, string.format("Fellowship message: %s", text or ""))
end

function M.make_leader(client, target_name)
	local fid = M.fid_of(client)
	if not fid then return end
	local leader, members = M.roster(fid)
	if leader ~= client:CharacterID() then
		client:Message(MT.Red, "Only the fellowship leader may hand over leadership.")
		return
	end
	for _, cid in ipairs(members) do
		if (eq.get_char_name_by_id(cid) or ""):lower() == (target_name or ""):lower() then
			save_roster(fid, cid, members)
			M.broadcast(fid, string.format("%s now leads the fellowship.", target_name))
			return
		end
	end
	client:Message(MT.Red, "No such member.")
end

-- ⚠️ Disband is leader-only and clears EVERY member's reverse index, not just the leader's -- a
-- surviving `fship_of_` would leave someone pointing at a roster that no longer exists.
function M.disband(client)
	local fid = M.fid_of(client)
	if not fid then return end
	local leader, members = M.roster(fid)
	if leader ~= client:CharacterID() then
		client:Message(MT.Red, "Only the fellowship leader may end the fellowship.")
		return
	end
	M.broadcast(fid, "The fellowship has been disbanded.")
	for _, cid in ipairs(members) do forget(cid) end
	eq.set_data(ros_key(fid), "")
	eq.set_data(name_key(fid), "")
	eq.set_data(fire_key(fid), "")
end

-- ⚠️ The MODEL is left to decay on its own (FIRE_IDLE_SEC); only the bucket is cleared. Chasing the
-- object down would mean an entity-list walk in whichever zone it stands in, which may not even be
-- running -- the decay already handles it.
function M.douse(client)
	local fid = M.fid_of(client)
	if not fid then return end
	local rec = M.fire(fid)
	if not rec then client:Message(MT.Red, "No campfire is burning.") return end

	-- ⚠️⚠️ DESPAWN FIRST, CLEAR SECOND. The lookup now falls back to the RECORDED POSITION, so
	-- clearing the record first destroys the only reliable way to find the model -- which is the bug
	-- this pair used to have in the opposite direction. The record is still cleared even if the
	-- Depop fails, so a fire that somehow survives is at least inert and the tick will not re-raise
	-- it.
	local gone = M.despawn_fire(fid, rec)
	eq.set_data(fire_key(fid), "")

	if gone or rec.zone ~= (eq.get_zone_short_name() or "") then
		M.broadcast(fid, "The fellowship's campfire has been put out.")
	else
		-- ⚠️ Never claim success we did not achieve: that is what made this look broken rather than
		-- merely awkward. Saying so points at the real remedy instead.
		client:Message(MT.Red, "The campfire is out, but its embers linger until the area empties.")
	end
	M.send_data(client)
end

return M
