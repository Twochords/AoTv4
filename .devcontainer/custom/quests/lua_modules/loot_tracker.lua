-- loot_tracker.lua  --  Advanced Loot System (ALS), server side
-- Reimplements EQ's Advanced Loot window server-side. Loot from NPCs you kill accumulates in your
-- PERSONAL list; you decide per item (Loot / Leave). Driven over chat to the dll ALS window:
--
--   server -> dll : "LOOTDATA <n>^eid|itemid|icon|name|npcname^..."   (full list; <n> rows; "LOOTDATA 0" = empty)
--   dll -> server : "/say alspick <eid> loot|leave"  and  "/say alslootall"
--
-- The dll swallows LOOTDATA and draws the window; clicks queue the /say replies. No chat saylinks --
-- the UI is the interface. (Later phases: AN/AG/Never filters, group Need/Greed rolls, leave-on-timeout.)

local M = {}

-- ---- tunables -------------------------------------------------------------------------------------
local REPLACE_NATIVE = false  -- Phase 1: keep native looting as a fallback while testing. Flip true to REPLACE.
local GM_ONLY        = true   -- Phase 1: only active for staff (status >= GM_STATUS) while we build.
local GM_STATUS      = 100
local MAX_ROWS       = 100    -- safety cap on the personal list

-- ---- per-character state (data buckets) -----------------------------------------------------------
-- als_<charid>      = personal list, rows "eid|cid|iid|icon|name|npc" joined by "^"
-- als_eid_<charid>  = monotonic entry-id counter
-- als_seen_<corpseid> = per-corpse "already captured" flag; auto-expires (see seen_corpse/mark_corpse)
local function list_key(c)   return "als_"        .. c:CharacterID() end
local function eid_key(c)    return "als_eid_"    .. c:CharacterID() end
local function filter_key(c) return "als_filter_" .. c:CharacterID() end

local function get_num(k) return tonumber(eq.get_data(k)) or 0 end

-- AN/AG/Never keep-list: per-character map of item_id -> "need" | "greed" | "never". Serialized in the
-- als_filter_<charid> bucket as "iid:rule,iid:rule,...". Applied on capture (on_kill).
local function filter_load(c)
	local t, raw = {}, eq.get_data(filter_key(c)) or ""
	for iid, rule in raw:gmatch("(%d+):(%a+)") do t[tonumber(iid)] = rule end
	return t
end
local function filter_set(c, iid, rule)
	local t = filter_load(c); t[iid] = rule
	local parts = {}
	for k, v in pairs(t) do parts[#parts+1] = k .. ":" .. v end
	eq.set_data(filter_key(c), table.concat(parts, ","))
end
local function filter_del(c, iid)
	local t = filter_load(c); t[iid] = nil
	local parts = {}
	for k, v in pairs(t) do parts[#parts+1] = k .. ":" .. v end
	eq.set_data(filter_key(c), table.concat(parts, ","))
end

-- push the keep-list to the dll's Filters tab: "FILTERDATA <n>^iid|icon|name|rule^..."
local function filter_send(c)
	local rows = {}
	for iid, rule in pairs(filter_load(c)) do
		local nm = (eq.get_item_name(iid) or ("item " .. iid)):gsub("[%^|]", " ")
		rows[#rows+1] = string.format("%d|%d|%s|%s", iid, eq.get_item_icon(iid) or 0, nm, rule)
	end
	c:Message(MT.NPCQuestSay, string.format("FILTERDATA %d^%s", #rows, table.concat(rows, "^")))
end

local function load_list(c)
	local out, raw = {}, eq.get_data(list_key(c))
	if not raw or raw == "" then return out end
	for row in raw:gmatch("([^%^]+)") do
		local eid, cid, iid, icon, nm, npc = row:match("^(%d+)|(%d+)|(%d+)|(%d+)|([^|]*)|(.*)$")
		if eid then
			out[#out+1] = { eid=tonumber(eid), cid=tonumber(cid), iid=tonumber(iid),
			                icon=tonumber(icon), nm=nm, npc=npc }
		end
	end
	return out
end

local function save_list(c, rows)
	local parts = {}
	for _, r in ipairs(rows) do
		parts[#parts+1] = string.format("%d|%d|%d|%d|%s|%s", r.eid, r.cid, r.iid, r.icon or 0, r.nm, r.npc)
	end
	eq.set_data(list_key(c), table.concat(parts, "^"))
end

local function next_eid(c)
	local n = get_num(eid_key(c)) + 1
	eq.set_data(eid_key(c), tostring(n))
	return n
end

-- "Seen" = a corpse we've already captured into the window (so it isn't ingested twice). This MUST NOT
-- be a permanent global set: entity ids RECYCLE within a zone, so an old set eventually flags a brand-new
-- corpse as seen and the window silently never pops (that was the intermittent "no loot window" bug).
-- Instead, flag each corpse with its OWN auto-expiring, name-qualified bucket: it clears itself after
-- SEEN_TTL, and a recycled id only counts as seen if it also carries the SAME owner name.
local SEEN_TTL = "1800"   -- 30 min (seconds string for set_data expiry) -- comfortably > corpse decay
local function seen_corpse(cp)
	return eq.get_data("als_seen_" .. cp:GetID()) == cp:GetOwnerName()
end
local function mark_corpse(cp)
	eq.set_data("als_seen_" .. cp:GetID(), cp:GetOwnerName(), SEEN_TTL)
end

-- who gets the loot: the killer, or a pet/merc's owner
local function looter_of(e)
	local k = e.other
	if not k then return nil end
	if k.IsClient and k:IsClient() then return k:CastToClient() end
	if k.GetOwner then local o = k:GetOwner(); if o and o.IsClient and o:IsClient() then return o:CastToClient() end end
	return nil
end

local function active_for(c)
	if not c then return false end
	if GM_ONLY and (c:GetGMStatus() or 0) < GM_STATUS then return false end
	return true
end

-- the corpse this NPC just made: newest (highest entity id) unseen corpse whose owner name matches
local function find_fresh_corpse(npc_name)
	local best, el = nil, eq.get_entity_list()
	-- .entries is a return_stl_iterator (generic for, not ipairs). Hold the list object in a LOCAL so its
	-- vector isn't GC'd mid-iteration.
	local cl = el:GetCorpseList()
	for cp in cl.entries do
		if cp:GetOwnerName() == npc_name and not seen_corpse(cp) then
			if not best or cp:GetID() > best:GetID() then best = cp end
		end
	end
	return best
end

-- push the full current list to the dll ALS window
local function send_list(c)
	local rows = load_list(c)
	local parts = {}
	for _, r in ipairs(rows) do
		local nm  = (r.nm  or ""):gsub("[%^|]", " ")   -- ^ and | are delimiters
		local npc = (r.npc or ""):gsub("[%^|]", " ")
		parts[#parts+1] = string.format("%d|%d|%d|%s|%s", r.eid, r.iid, r.icon or 0, nm, npc)
	end
	c:Message(MT.NPCQuestSay, string.format("LOOTDATA %d^%s", #rows, table.concat(parts, "^")))
end

-- ---- public API -----------------------------------------------------------------------------------
local do_loot   -- forward declaration (defined below; on_kill auto-loots keep-list items through it)

-- Clear the personal loot list. Called on LOGIN (last session's corpses are gone -- stale, un-lootable
-- entries) and on DEATH (keeping a dead run's loot claimable at level 1 would dodge the death penalty).
-- The AN/AG/Never keep-list is intentionally NOT cleared -- filters are meant to persist.
function M.reset(c)
	eq.set_data(list_key(c), "")
	send_list(c)                              -- LOOTDATA 0: empties the personal list (keeps filters)
	c:Message(MT.NPCQuestSay, "LOOTCLOSE")    -- tell the dll to CLOSE the window (death shouldn't leave it open)
end

-- global_npc.lua event_death_complete
function M.on_kill(e)
	local c = looter_of(e)
	if not active_for(c) then return end

	-- Match the corpse by ENTITY name: an NPC corpse's owner-name is set from npc->GetName() (the unique
	-- "a_decaying_skeleton000" entity name), NOT the clean name -- so match GetName(), not GetCleanName().
	local corpse = find_fresh_corpse(e.self:GetName())
	if not corpse then
		-- TEMP: still surfaces if the corpse match ever fails again (kept minimal). Remove once confident.
		c:Message(MT.NPCQuestSay, string.format("ALS: no fresh corpse for '%s' (tell the dev)", tostring(e.self:GetName())))
		return
	end
	mark_corpse(corpse)

	-- COIN: the ALS auto-grants a kill's coin (it would otherwise sit on the corpse we bypass). Pull the
	-- corpse's cash to the killer and clear it. AddMoneyToPP takes (copper, silver, gold, platinum).
	local cp, sp, gp, pp = corpse:GetCopper(), corpse:GetSilver(), corpse:GetGold(), corpse:GetPlatinum()
	if (cp + sp + gp + pp) > 0 then
		c:AddMoneyToPP(cp, sp, gp, pp, true)
		corpse:RemoveCash()
		c:Message(MT.NPCQuestSay, string.format("You receive %dp %dg %ds %dc.", pp, gp, sp, cp))
	end

	-- Hold the loot-list object in a LOCAL: .entries is a return_stl_iterator over the vector INSIDE it.
	-- If GetLootList()'s temporary is GC'd after the first read, iteration stops -> only the first item is
	-- captured (that was the "only shows one item / no-drops missing" bug). Keeping `ll` alive fixes it.
	local ids, ll = {}, corpse:GetLootList()
	for iid in ll.entries do
		ids[#ids + 1] = iid
	end
	if #ids == 0 then return end

	local filt = filter_load(c)                        -- AN/AG/Never keep-list

	-- TEMP DIAGNOSTIC: show exactly what GetLootList returned + how each item routes. Remove once solved.
	do
		local dbg = {}
		for _, iid in ipairs(ids) do
			dbg[#dbg+1] = string.format("%d:%s[%s]", iid, (eq.get_item_name(iid) or "?"), tostring(filt[iid] or "show"))
		end
		c:Message(MT.NPCQuestSay, "ALS DEBUG (" .. #ids .. "): " .. table.concat(dbg, ", "))
	end
	local rows  = load_list(c)
	local npc   = e.self:GetCleanName()
	local cid   = corpse:GetID()
	local links = {}                                   -- clickable item links for the chat announcement
	for _, iid in ipairs(ids) do
		local rule = filt[iid]
		if rule == "never" then
			-- filtered out: leave on the corpse, never show it
		elseif rule == "need" or rule == "greed" then
			do_loot(c, { cid = cid, iid = iid })       -- keep-list auto-loot (solo: need == greed == take it)
		elseif #rows < MAX_ROWS then
			rows[#rows+1] = {
				eid  = next_eid(c),
				cid  = cid,
				iid  = iid,
				icon = eq.get_item_icon(iid) or 0,
				nm   = eq.get_item_name(iid) or ("item " .. iid),
				npc  = npc,
			}
			links[#links+1] = eq.item_link(iid)        -- real EQ [Item] link -- clickable in chat to inspect
		end
	end
	save_list(c, rows)
	send_list(c)
	-- The GDI loot window can't render EQ's native item links, so mirror the offered items to chat as
	-- clickable [Item] links -- players click them to inspect stats before deciding keep/leave.
	if #links > 0 then
		c:Message(MT.NPCQuestSay, "Loot available: " .. table.concat(links, "  "))
	end
end

-- transfer one entry's item from the corpse to the player. Returns true on success.
-- (assigns to the forward-declared local above so on_kill can auto-loot keep-list items through it)
function do_loot(c, r)
	local corpse = eq.get_entity_list():GetCorpseByID(r.cid)
	if not corpse then return false end       -- corpse decayed/gone
	corpse:RemoveItemByID(r.iid)              -- off the corpse FIRST (no dupe)...
	c:SummonRawItem(r.iid)                    -- ...deliver the EXACT item (raw -- no Mythic upgrade).
	c:Message(MT.NPCQuestSay, string.format("You loot %s.", r.nm or eq.get_item_name(r.iid) or ("item " .. r.iid)))
	return true
end

-- global_player.lua event_say. Returns true if it consumed the message.
function M.handle_say(e)
	local c, msg = e.self, (e.message or ""):lower()

	if msg == "alsrefresh" then           -- /advl hotkey asks the server to (re)send the current list
		if not active_for(c) then return true end
		send_list(c)
		return true
	end

	if msg == "alslootall" then
		if not active_for(c) then return true end
		for _, r in ipairs(load_list(c)) do do_loot(c, r) end
		eq.set_data(list_key(c), "")
		send_list(c)
		return true
	end

	if msg == "alsfilters" then           -- Filters tab asks for the current AN/AG/Never keep-list
		if not active_for(c) then return true end
		filter_send(c)
		return true
	end

	local del_iid = msg:match("^alsfilterdel%s+(%d+)$")
	if del_iid then                       -- remove one keep-list rule (un-assign a mistaken AN/AG/Never)
		if not active_for(c) then return true end
		filter_del(c, tonumber(del_iid))
		filter_send(c)
		return true
	end

	local ins_eid = msg:match("^alsinspect%s+(%d+)$")
	if ins_eid then                       -- click an item in the loot window -> open its native inspect box
		if not active_for(c) then return true end
		ins_eid = tonumber(ins_eid)
		for _, r in ipairs(load_list(c)) do
			if r.eid == ins_eid then c:InspectItem(r.iid); break end
		end
		return true
	end

	local eid, action = msg:match("^alspick%s+(%d+)%s+(%a+)$")
	if not eid then return false end
	if not active_for(c) then return true end
	eid = tonumber(eid)

	local rows, kept, hit = load_list(c), {}, nil
	for _, r in ipairs(rows) do
		if r.eid == eid then hit = r else kept[#kept+1] = r end
	end
	if hit then
		if action == "loot" then
			do_loot(c, hit)
		elseif action == "an" then                -- Always Need: remember + take this one
			filter_set(c, hit.iid, "need");  do_loot(c, hit)
			c:Message(MT.NPCQuestSay, string.format("Filter set: %s -> Always Need.", hit.nm))
		elseif action == "ag" then                -- Always Greed: remember + take this one
			filter_set(c, hit.iid, "greed"); do_loot(c, hit)
			c:Message(MT.NPCQuestSay, string.format("Filter set: %s -> Always Greed.", hit.nm))
		elseif action == "never" then             -- Never: remember; leave this one on the corpse
			filter_set(c, hit.iid, "never")
			c:Message(MT.NPCQuestSay, string.format("Filter set: %s -> Never (left on corpse).", hit.nm))
		end
		-- "leave" (and "never") just drop the row; the item stays on the corpse.
	end
	save_list(c, kept)
	send_list(c)
	return true
end

-- global_player.lua event_loot. Return true to CANCEL native looting (REPLACE model).
function M.block_native(e)
	if not REPLACE_NATIVE then return false end
	if not active_for(e.self) then return false end
	return true
end

return M
