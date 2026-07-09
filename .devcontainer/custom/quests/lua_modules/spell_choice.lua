-- spell_choice.lua
-- Level-up "choose 1 of 3 spells" system.
--   * Class-agnostic: the pool is every scribable spell (any class's spells).
--   * Level-appropriate: only spells whose lowest class-learn-level is <= the
--     player's level are offered, biased to within LEVEL_BAND of their level,
--     so a level 1 never sees a level 50 spell.
--   * Server-authoritative: the 3 offered spell ids are stored in a data bucket
--     and the player picks by INDEX (1-3), so a modified client cannot scribe an
--     arbitrary spell -- only one of the three it was actually offered.
--
-- Transport (no opcode patching):
--   server -> client : a window is opened (TEST: chat saylinks; DLL: magic chat line)
--   client -> server : player "says" "spellpick <N>" (saylink today, eq-core-dll
--                       window injects the same say later) -> handled in event_say.

local pool      = require("spell_pool")
local icons     = require("spell_icons")     -- id -> spellbook icon index (for the DLL window)
local spelldesc = require("spell_desc")      -- id -> description text (for the DLL window hover)
local skills    = require("skill_pool")      -- combat skill rewards: id -> {name, icon}
local blacklist = require("spell_blacklist") -- spell ids never offered (rez, enchant, ...)

local M = {}

------------------------------------------------------------------- tuning
local CHOICE_COUNT      = 3          -- choices offered per level-up
local LEVEL_BAND        = 6          -- prefer spells learned within [L-band, L]
local SAY_TRIGGER       = "spellpick" -- player says "spellpick <N>" to choose
local USE_DLL_TRIGGER   = true       -- emit the SPELLCHOICEDATA line for the eq-core-dll window
local SHOW_SAYLINKS     = true        -- also show chat saylinks (no-client-mod fallback)
local SKILL_OFFER_CHANCE = 0.7       -- chance a level-up includes ONE combat-skill slot
local SKILL_GRANT_PCT    = 0.25      -- a picked skill is set to this fraction of its level cap
local DLL_MARK        = string.char(18) -- DC2 control byte the DLL brackets the payload with

------------------------------------------------------------------- helpers
local function bucket_key(client)
	return "spell_choice_" .. client:CharacterID()
end

-- Disciplines are trained (Combat Abilities window); normal spells are scribed.
local function already_known(client, id)
	if eq.is_discipline(id) then
		return client:HasDisciplineLearned(id)
	end
	return client:HasSpellScribed(id)
end

-- Level-appropriate, not-yet-known candidates. Walk from the player's level
-- downward; if too few inside the band, widen all the way to level 1.
local function gather_candidates(client, level)
	local seen, cand = {}, {}
	local function collect(from, to)
		for lv = from, to, -1 do
			local list = pool[lv]
			if list then
				for _, sp in ipairs(list) do
					-- AoTv4: reject enchant-MATERIAL spells by name. The id blacklist only caught
					-- "Enchant <metal>"; the "Mass Enchant <metal>" line slipped through. Both are junk.
					local is_enchant = sp.name and (sp.name:match("^Enchant ") or sp.name:match("^Mass Enchant "))
					if not seen[sp.id] and not blacklist[sp.id] and not already_known(client, sp.id) and not is_enchant then
						seen[sp.id] = true
						cand[#cand + 1] = sp
					end
				end
			end
		end
	end

	local lo = math.max(1, level - LEVEL_BAND)
	collect(level, lo)
	if #cand < CHOICE_COUNT and lo > 1 then
		collect(lo - 1, 1)
	end
	return cand
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

-- The skill value a reward grants at the player's current level.
local function skill_target(client, id)
	return math.floor((client:MaxSkill(id) or 0) * SKILL_GRANT_PCT)
end

-- Combat skills worth offering: the player CAN have it, it has a cap, and they DON'T HAVE IT YET
-- (raw value 0). Each activated ability is offered at most once -- once earned it never re-appears, so
-- the reward window stays free of duplicates. (It keeps improving through use after that.)
local function gather_skill_candidates(client)
	local out = {}
	for id, info in pairs(skills) do
		if client:CanHaveSkill(id) and client:MaxSkill(id) > 0
		   and client:GetRawSkill(id) <= 0 then
			out[#out + 1] = { kind = "skill", id = id, name = info.name, icon = info.icon }
		end
	end
	return out
end

-- Send the window data (SPELLCHOICEDATA + SPELLDESCDATA) + saylink fallback for ONE offer, given its
-- comma-separated token string ("S:id,S:id,K:id"). Names/icons/descs are reconstructed from the tokens
-- so the queue only needs to store the tokens.
local function send_offer(client, offer_str)
	if not offer_str or offer_str == "" then return end
	local names, descs, labels = {}, {}, {}
	for tok in offer_str:gmatch("[^,]+") do
		local kind, id = tok:match("^(%a):(%d+)$"); id = tonumber(id)
		local nm, ic, ds
		if kind == "K" then
			local info = skills[id]
			nm = (info and info.name) or ("skill " .. id); ic = (info and info.icon) or 0; ds = "A combat skill."
		else
			nm = eq.get_spell_name(id) or ("spell " .. id); ic = icons[id] or 0; ds = spelldesc[id] or ""
		end
		names[#names + 1]  = nm .. "|" .. tostring(ic)
		descs[#descs + 1]  = (ds:gsub("[%^|]", " "))
		labels[#labels + 1] = nm
	end
	if #names == 0 then return end
	if USE_DLL_TRIGGER then
		client:Message(MT.NPCQuestSay, "SPELLCHOICEDATA " .. table.concat(names, "^"))
		client:Message(MT.NPCQuestSay, "SPELLDESCDATA "  .. table.concat(descs, "^"))
	end
	if SHOW_SAYLINKS then
		client:Message(MT.Yellow, "Choose a new reward:")
		for i, nm in ipairs(labels) do
			client:Message(MT.LightBlue, string.format("  %d) %s   %s", i, nm,
				eq.say_link(SAY_TRIGGER .. " " .. i, true, "[ Select " .. nm .. " ]")))
		end
	end
end

-- Prune already-earned rewards from the FRONT offer, drop offers that become empty, save the queue, then
-- show the (pruned) front. Queued offers can go stale -- a spell/ability offered at level N may be learned
-- from a different offer before this one is resolved -- so we re-check right before showing, guaranteeing
-- you're never shown (or able to pick) a spell you already have OR a combat ability you already earned.
local function already_have(client, kind, id)
	if kind == "K" then return client:GetRawSkill(id) > 0 end   -- combat ability already trained
	return already_known(client, id)                            -- spell scribed / discipline learned
end

local function refresh_and_show(client, key)
	local q = eq.get_data(key) or ""
	while q ~= "" do
		local front, rest = q:match("^([^;]*);?(.*)$")
		local kept = {}
		for tok in front:gmatch("[^,]+") do
			local kind, id = tok:match("^(%a):(%d+)$"); id = tonumber(id)
			if id and not already_have(client, kind, id) then
				kept[#kept + 1] = tok
			end
		end
		if #kept > 0 then
			-- AoTv4: pruning already-known/stale tokens can leave fewer than CHOICE_COUNT. Top the offer
			-- back up with FRESH candidates so the player ALWAYS gets a full set of choices -- never a
			-- short 1-2 option window because one option was learned from an earlier offer.
			if #kept < CHOICE_COUNT then
				local in_offer = {}
				for _, t in ipairs(kept) do
					local k, i = t:match("^(%a):(%d+)$")
					if i then in_offer[k .. i] = true end
				end
				for _, sp in ipairs(pick_random(gather_candidates(client, client:GetLevel()), CHOICE_COUNT)) do
					if #kept >= CHOICE_COUNT then break end
					if not in_offer["S" .. sp.id] then
						kept[#kept + 1] = "S:" .. sp.id
						in_offer["S" .. sp.id] = true
					end
				end
			end
			q = (rest == "") and table.concat(kept, ",") or (table.concat(kept, ",") .. ";" .. rest)
			eq.set_data(key, q)
			send_offer(client, table.concat(kept, ","))
			return
		end
		q = rest   -- entire front offer is already-known -> discard it and try the next queued offer
	end
	eq.set_data(key, "")   -- nothing left to offer
end

------------------------------------------------------------------- public API
-- Offer a fresh set of choices. Call from event_level_up.
function M.offer(e)
	local client = e.self
	local level  = client:GetLevel()

	-- Vary the offer EACH level-up. A per-(char,level) seed repeats the same 3 spells every run
	-- (roguelite re-levels constantly), so mix in a per-offer counter (bucket) -- like aa_choice.
	local sk = "spell_seed_" .. client:CharacterID()
	local n  = (tonumber(eq.get_data(sk)) or 0) + 1
	eq.set_data(sk, tostring(n))
	math.randomseed(client:CharacterID() * 2654435761 + level * 7919 + n * 104729)

	-- Mostly spells with ~1 combat skill: reserve one slot for a skill (when eligible),
	-- fill the rest with level-appropriate spells, then shuffle so the skill isn't last.
	local n_skill = 0
	local skill_cands = gather_skill_candidates(client)
	if #skill_cands > 0 and math.random() < SKILL_OFFER_CHANCE then n_skill = 1 end

	local choices = {}
	for _, sp in ipairs(pick_random(gather_candidates(client, level), CHOICE_COUNT - n_skill)) do
		choices[#choices + 1] = { kind = "spell", id = sp.id, name = sp.name, icon = icons[sp.id] or 0 }
	end
	for _, sk in ipairs(pick_random(skill_cands, n_skill)) do
		choices[#choices + 1] = sk
	end
	if #choices == 0 then return end
	choices = pick_random(choices, #choices)  -- shuffle order

	-- typed, ordered tokens so the pick is validated server-side:
	--   "S:<spellid>" = spell/discipline,  "K:<skillid>" = combat skill
	local toks = {}
	for _, c in ipairs(choices) do
		toks[#toks + 1] = (c.kind == "skill" and "K:" or "S:") .. c.id
	end

	-- APPEND this offer to the pending QUEUE (offers separated by ";") rather than overwriting, so an
	-- un-picked offer from an earlier level-up is never lost. The window always shows the FRONT (oldest);
	-- picking one pops it and reveals the next.
	local q = eq.get_data(bucket_key(client)) or ""
	local this_offer = table.concat(toks, ",")
	q = (q == "") and this_offer or (q .. ";" .. this_offer)
	eq.set_data(bucket_key(client), q)

	refresh_and_show(client, bucket_key(client))   -- prune known spells, then show the oldest pending offer
end

-- Discard any un-picked reward offers still queued. Call on death (roguelite reset) so a new run doesn't
-- inherit a stale high-level offer from the previous life.
function M.clear_pending(client)
	eq.set_data(bucket_key(client), "")
end

-- Tell the client (eq-core-dll skill-unlock hook) which combat skills the player has EARNED
-- (value > 0) so it reveals only those. Call on connect and after each skill pick. The dll
-- swallows this line; with no client mod it's harmless noise the player won't normally see.
function M.send_unlocks(client)
	local earned = {}
	for id, _ in pairs(skills) do
		if client:GetRawSkill(id) > 0 then earned[#earned + 1] = tostring(id) end
	end
	client:Message(MT.NPCQuestSay, "SKILLUNLOCKDATA " .. table.concat(earned, ","))

	-- Nudge the client to REBUILD its Combat Abilities list. The client builds that list at zone-in,
	-- BEFORE the SKILLUNLOCKDATA chat line above reaches the dll, so earned combat abilities stayed
	-- invisible/unusable until the player forced a rebuild (jumping did it). Re-sending each earned
	-- skill's own value (OP_SkillUpdate, no skill-up spam) triggers that rebuild now that the dll has
	-- the earned set -- so abilities appear on login without the jump.
	for id, _ in pairs(skills) do
		local v = client:GetRawSkill(id)
		if v > 0 then client:SetSkill(id, v) end
	end
end

-- Resolve a pick. Call from event_say; returns true if it consumed the message.
function M.handle_say(e)
	local idx = (e.message or ""):lower():match("^" .. SAY_TRIGGER .. "%s+(%d+)%s*$")
	if not idx then return false end
	idx = tonumber(idx)

	local client = e.self
	local key  = bucket_key(client)
	local q    = eq.get_data(key)
	if not q or q == "" then
		client:Message(MT.Red, "You have no spell choice pending.")
		return true
	end

	-- the window shows the FRONT offer (oldest); the rest stay queued
	local front, rest = q:match("^([^;]*);?(.*)$")
	local toks = {}
	for s in front:gmatch("([^,]+)") do toks[#toks + 1] = s end
	local tok = toks[idx]
	if not tok then
		client:Message(MT.Red, "That is not a valid choice.")
		return true
	end

	local kind, id = tok:match("^(%a):(%d+)$")
	id = tonumber(id)
	if not kind or not id then
		client:Message(MT.Red, "That is not a valid choice.")
		return true
	end

	if kind == "K" then
		-- combat skill reward: granted ONCE (offered only while unearned). Reject if already trained.
		local info   = skills[id]
		local sname  = info and info.name or ("skill " .. id)
		if client:GetRawSkill(id) > 0 then
			client:Message(MT.Red, "You already have " .. sname .. ".")
			return true  -- leave the choice pending so they can pick another
		end
		client:SetSkill(id, skill_target(client, id))
		client:Message(MT.Yellow, "You have trained " .. sname .. "!")
		M.send_unlocks(client)  -- reveal the newly-earned skill on the client
	else
		-- spell / discipline reward
		local spell_id = id
		local name = eq.get_spell_name(spell_id)
		if already_known(client, spell_id) then
			client:Message(MT.Red, "You already know " .. name .. ".")
			return true  -- leave the choice pending so they can pick another
		end
		if eq.is_discipline(spell_id) then
			client:TrainDiscBySpellID(spell_id)
			client:Message(MT.Yellow, "You have learned the discipline " .. name .. "!")
		else
			local slot = client:GetNextAvailableSpellBookSlot()
			if slot < 0 then
				client:Message(MT.Red, "Your spellbook is full -- make room and pick again.")
				return true
			end
			client:ScribeSpell(spell_id, slot)
			client:Message(MT.Yellow, "You have learned " .. name .. "!")
		end
	end

	-- resolved: drop the front offer, keep the queued rest, then prune + reveal the next pending offer
	eq.set_data(key, rest)
	refresh_and_show(client, key)
	return true
end

return M
