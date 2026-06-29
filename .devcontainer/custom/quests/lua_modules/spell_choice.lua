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
					if not seen[sp.id] and not blacklist[sp.id] and not already_known(client, sp.id) then
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

-- Combat skills worth offering: the player can have it, it has a cap, and its current
-- value is still below the grant target (so the reward would actually raise it). As the
-- player levels, caps rise, so mastered skills re-enter the pool with a higher target.
local function gather_skill_candidates(client)
	local out = {}
	for id, info in pairs(skills) do
		if client:CanHaveSkill(id) and client:MaxSkill(id) > 0
		   and client:GetRawSkill(id) < skill_target(client, id) then
			out[#out + 1] = { kind = "skill", id = id, name = info.name, icon = info.icon }
		end
	end
	return out
end

------------------------------------------------------------------- public API
-- Offer a fresh set of choices. Call from event_level_up.
function M.offer(e)
	local client = e.self
	local level  = client:GetLevel()

	-- deterministic-per-(character,level) variety without relying on os.time
	math.randomseed(client:CharacterID() * 2654435761 + level + 1)

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

	-- store typed, ordered tokens so the pick is validated server-side:
	--   "S:<spellid>" = spell/discipline,  "K:<skillid>" = combat skill
	local toks, names, descs = {}, {}, {}
	for _, c in ipairs(choices) do
		toks[#toks + 1]  = (c.kind == "skill" and "K:" or "S:") .. c.id
		names[#names + 1] = c.name .. "|" .. tostring(c.icon or 0)  -- "name|icon" for the window
		local d = (c.kind == "spell" and spelldesc[c.id]) or "A combat skill."
		descs[#descs + 1] = (d:gsub("[%^|]", " "))                  -- ^ and | are delimiters
	end
	eq.set_data(bucket_key(client), table.concat(toks, ","))

	-- ---- DLL window trigger: hidden chat line the eq-core-dll window parses.
	if USE_DLL_TRIGGER then
		client:Message(MT.NPCQuestSay, "SPELLCHOICEDATA " .. table.concat(names, "^"))
		client:Message(MT.NPCQuestSay, "SPELLDESCDATA " .. table.concat(descs, "^"))
	end

	-- ---- Saylink fallback: works with no client mod.
	if SHOW_SAYLINKS then
		client:Message(MT.Yellow, "You have gained a level! Choose a new reward:")
		for i, c in ipairs(choices) do
			local link = eq.say_link(SAY_TRIGGER .. " " .. i, true, "[ Select " .. c.name .. " ]")
			client:Message(MT.LightBlue, string.format("  %d) %s   %s", i, c.name, link))
		end
	end
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
end

-- Resolve a pick. Call from event_say; returns true if it consumed the message.
function M.handle_say(e)
	local idx = (e.message or ""):lower():match("^" .. SAY_TRIGGER .. "%s+(%d+)%s*$")
	if not idx then return false end
	idx = tonumber(idx)

	local client = e.self
	local key  = bucket_key(client)
	local data = eq.get_data(key)
	if not data or data == "" then
		client:Message(MT.Red, "You have no spell choice pending.")
		return true
	end

	local toks = {}
	for s in data:gmatch("([^,]+)") do toks[#toks + 1] = s end
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
		-- combat skill reward: raise the skill to its level-scaled grant target
		local info   = skills[id]
		local sname  = info and info.name or ("skill " .. id)
		local target = skill_target(client, id)
		if client:GetRawSkill(id) < target then
			client:SetSkill(id, target)
		end
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

	eq.set_data(key, "")  -- consume the pending choice
	return true
end

return M
