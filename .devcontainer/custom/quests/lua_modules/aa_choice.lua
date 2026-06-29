-- aa_choice.lua
-- Death-driven random AA picker (sibling of spell_choice.lua).
--   * On death, global_player.lua banks the run's XP as AA points (client:AddAAPoints) and
--     restarts at level 1, then calls M.grant_picks(e).
--   * The player's UNSPENT AA pool (GetAAPoints) IS the budget. Each window pop offers 3 random,
--     level-appropriate AAs the player can AFFORD; picking one trains its next rank and SPENDS its
--     cost (GrantAlternateAdvancementAbility ignore_cost=false). We keep popping the window until
--     the budget can't afford anything -> no fixed pick count, no wasted/failed picks.
--   * Class-agnostic: aa_ability.classes carries the Bard bit (128); the server's class gate was
--     fixed to use GetPlayerClassBit (see zone/aa.cpp). Expansion gating is done by the pool, with
--     Expansion:UseCurrentExpansionAAOnly = false so classic theming doesn't block all AAs.
--
-- Transport mirrors spell_choice but on its OWN channel so it can drive a SEPARATE dll window:
--   server -> client : "AACHOICEDATA name|icon|cost^name|icon|cost^name|icon|cost"
--   client -> server : "/say aapick N"
-- A chat saylink fallback also works with no client mod (for testing before the dll window exists).

local aapool = require("aa_pool")

local M = {}

local CHOICE_COUNT  = 3
local SAY_TRIGGER   = "aapick"
local MAX_PICKS     = 30     -- safety cap on window pops per death (budget normally ends it sooner)
local SHOW_SAYLINKS = true   -- chat fallback until the dll AA window is built

-- Per-AA investment cap: no single AA may absorb more than this many total points, so every AA is
-- fully attainable within ~one death's bank. Cheap AAs (cost 2) can still reach ~rank 5 (enough to
-- satisfy rank<=3 prereqs); a cost-9 AA stays rank 1. Keep this in step with the per-death bank
-- (global_player AA_XP_BASE ~= 10 AA at the level-50 cap); raise BOTH as the level cap rises.
local RANK_BUDGET   = 10

-- Banked AA points live in a PRIVATE data bucket (aa_bank_<charid>), NOT the native unspent pool
-- (m_pp.aapoints). That way the native AA window shows 0 spendable points -- the player cannot
-- bypass the random picker and buy AAs directly there. Picks are granted FREE (ignore_cost=true)
-- and we deduct the cost from the bucket ourselves.
local function bank_key(client)   return "aa_bank_"   .. client:CharacterID() end
local function choice_key(client) return "aa_choice_" .. client:CharacterID() end
local function seed_key(client)   return "aa_seed_"   .. client:CharacterID() end
local function count_key(client)  return "aa_pcount_" .. client:CharacterID() end

local function get_num(key) local v = eq.get_data(key); return tonumber(v) or 0 end

-- The player satisfies an AA's first-rank prerequisites (so the server won't reject it). Mirrors
-- CanPurchaseAlternateAdvancementRank's prereq loop: needs `r` ranks of each required ability `a`.
local function prereqs_met(client, aa)
	if not aa.pr then return true end
	for _, req in ipairs(aa.pr) do
		if (client:GetAAByAAID(req.a) or 0) < req.r then return false end
	end
	return true
end

-- Level-appropriate AAs the player can afford AND whose prereqs are met. The 1-60 tier was lowered
-- to level_req 1 (most live at pool[1]; 61+ unlock at their level); prereqs gate dependents until
-- the player has trained what they require -- so dependency chains unlock progressively.
local function gather_affordable(client, level, budget)
	local out = {}
	for lv = 1, level do
		local list = aapool[lv]
		if list then
			for _, aa in ipairs(list) do
				local cost = aa.cost or 1
				local next_rank = (client:GetAAByAAID(aa.id) or 0) + 1
				-- per-AA cap: total invested (next_rank * cost) must stay within RANK_BUDGET; a
				-- free (cost 0) AA is a one-time pick. Plus it must be affordable now + prereq-met.
				local within_cap
				if cost == 0 then within_cap = (next_rank <= 1)
				else                within_cap = (next_rank * cost <= RANK_BUDGET) end
				if cost <= budget and within_cap and prereqs_met(client, aa) then
					out[#out + 1] = aa
				end
			end
		end
	end
	return out
end

local function pick_random(list, n)
	local bag, out = {}, {}
	for i, v in ipairs(list) do bag[i] = v end
	for _ = 1, n do
		if #bag == 0 then break end
		local i = math.random(#bag)
		out[#out + 1] = bag[i]
		table.remove(bag, i)
	end
	return out
end

------------------------------------------------------------------- public API
-- Bank `points` AA points (added to the private bucket) and start presenting choices.
function M.grant_picks(e, points)
	local client = e.self
	points = tonumber(points) or 0
	if points > 0 then
		eq.set_data(bank_key(client), tostring(get_num(bank_key(client)) + points))
	end
	eq.set_data(count_key(client), "0")
	M.offer(e)
end

-- Present one AA choice if the banked budget can still afford anything.
function M.offer(e)
	local client = e.self
	local budget = get_num(bank_key(client))

	if get_num(count_key(client)) >= MAX_PICKS then
		eq.set_data(choice_key(client), "")
		client:Message(MT.Yellow, string.format(
			"That is enough training for now -- %d AA point(s) remain banked.", budget))
		return
	end

	if budget < 1 then
		eq.set_data(choice_key(client), "")
		if get_num(count_key(client)) > 0 then
			client:Message(MT.Yellow, "You have spent all of your banked AA points.")
		end
		return
	end

	local pool = gather_affordable(client, client:GetLevel(), budget)
	if #pool == 0 then
		eq.set_data(choice_key(client), "")
		client:Message(MT.Yellow, string.format(
			"You have %d AA point(s) banked -- nothing else is affordable right now.", budget))
		return
	end

	-- vary the offered set per pick: after death GetEXP() is 0, so seed on a per-offer counter
	-- (otherwise every pick would reroll the SAME 3 AAs).
	local n = get_num(seed_key(client)) + 1
	eq.set_data(seed_key(client), tostring(n))
	math.randomseed(client:CharacterID() * 2654435761 + n * 7919)

	local choices = pick_random(pool, CHOICE_COUNT)
	local ids, fields, descs = {}, {}, {}
	for _, aa in ipairs(choices) do
		ids[#ids + 1]    = tostring(aa.id) .. ":" .. tostring(aa.cost or 1)  -- "id:cost"
		fields[#fields + 1] = aa.name .. "|" .. tostring(aa.icon or 0) .. "|" ..
			tostring(aa.cost or 1) .. "|" .. tostring(aa.cls or 0)
		descs[#descs + 1] = ((aa.desc or ""):gsub("[%^|]", " "))               -- ^ and | are delimiters
	end
	eq.set_data(choice_key(client), table.concat(ids, ","))

	-- dll AA window (own channel): "AACHOICEDATA <banked>^name|icon|cost|cls^..."
	client:Message(MT.NPCQuestSay,
		"AACHOICEDATA " .. tostring(budget) .. "^" .. table.concat(fields, "^"))
	-- descriptions on a sibling line (shown on hover): "AADESCDATA desc1^desc2^desc3"
	client:Message(MT.NPCQuestSay, "AADESCDATA " .. table.concat(descs, "^"))

	if SHOW_SAYLINKS then
		client:Message(MT.Yellow, string.format(
			"Death has tempered you (%d AA banked) -- choose an Alternate Ability:", budget))
		for i, aa in ipairs(choices) do
			local link = eq.say_link(SAY_TRIGGER .. " " .. i, true, "[ Train " .. aa.name .. " ]")
			client:Message(MT.LightBlue, string.format("  %d) %s  (cost %d)  %s", i, aa.name, aa.cost or 1, link))
		end
	end
end

-- Resolve an "aapick N". Returns true if it consumed the message.
function M.handle_say(e)
	local idx = (e.message or ""):lower():match("^" .. SAY_TRIGGER .. "%s+(%d+)%s*$")
	if not idx then return false end
	idx = tonumber(idx)

	local client = e.self
	local data = eq.get_data(choice_key(client))
	if not data or data == "" then
		client:Message(MT.Red, "You have no Alternate Ability choice pending.")
		return true
	end

	local ids, costs = {}, {}
	for s in data:gmatch("([^,]+)") do
		local id, cost = s:match("^(%d+):(%d+)$")
		ids[#ids + 1]   = tonumber(id)
		costs[#costs + 1] = tonumber(cost) or 0
	end
	local aa_id = ids[idx]
	if not aa_id then
		client:Message(MT.Red, "That is not a valid choice.")
		return true
	end

	-- Train the next rank FREE (ignore_cost=true) -- still respects class/expansion/level/prereq
	-- gates -- then deduct the cost from the PRIVATE bank (the points never touch the native pool,
	-- so the native AA window can't be used to spend them).
	local cur = client:GetAAByAAID(aa_id) or 0
	local ok  = client:GrantAlternateAdvancementAbility(aa_id, cur + 1, true)

	eq.set_data(choice_key(client), "")                                       -- consume this offer
	eq.set_data(count_key(client), tostring(get_num(count_key(client)) + 1))

	if ok then
		local left = get_num(bank_key(client)) - (costs[idx] or 0)
		if left < 0 then left = 0 end
		eq.set_data(bank_key(client), tostring(left))
	else
		client:Message(MT.Red, "You cannot train that ability any further right now.")
	end

	M.offer(e)                                                                -- next choice if budget remains
	return true
end

return M
