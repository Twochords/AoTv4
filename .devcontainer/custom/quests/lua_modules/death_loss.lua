-- death_loss.lua
-- Roguelite death: destroy the player's CARRIED gear/inventory/money and wipe all spells; the
-- bank and epic weapons are kept. Runs from global_player.lua event_death, where the inventory is
-- still intact (EVENT_DEATH fires before the corpse is made, and LeaveNakedCorpses keeps items on
-- the player). Bank (slots 2000+) and shared bank (2500+) are never touched.

local epics  = require("epic_items")   -- { [item_id]=true } for items.epicitem<>0
local skills = require("skill_pool")   -- reward-gated combat skill ids (re-earned via level-up picks)

local M = {}

-- RoF2 slot ranges to wipe. Bag CONTENTS first (251-360) so a bag's items are recorded before the
-- bag itself is destroyed; then possessions (0-33 = worn + 10 general + cursor).
local WIPE = { { 251, 360 }, { 0, 33 } }

local function fmt_money(c)
	local p = math.floor(c / 1000); c = c % 1000
	local g = math.floor(c / 100);  c = c % 100
	local s = math.floor(c / 10);   local cp = c % 10
	local parts = {}
	if p  > 0 then parts[#parts + 1] = p  .. "p" end
	if g  > 0 then parts[#parts + 1] = g  .. "g" end
	if s  > 0 then parts[#parts + 1] = s  .. "s" end
	if cp > 0 then parts[#parts + 1] = cp .. "c" end
	return table.concat(parts, " ")
end

-- Destroy carried items + money + spells; return a list of lost-item names (for the report).
function M.process(client)
	local lost = {}
	for _, rng in ipairs(WIPE) do
		for slot = rng[1], rng[2] do
			local id = client:GetItemIDAt(slot)
			if id and id > 0 and not epics[id] then
				lost[#lost + 1] = eq.get_item_name(id)
				client:DeleteItemInInventory(slot, 0, true)   -- 0 = delete the whole item
			end
		end
	end

	local money = client:GetCarriedMoney() or 0               -- total carried copper (bank is separate)
	if money > 0 then
		client:TakeMoneyFromPP(money, true)
		lost[#lost + 1] = fmt_money(money) .. " in coin"
	end

	client:UnmemSpellAll(false)                              -- clear gems (false = no per-spell spam;
	client:UnscribeSpellAll(false)                           -- the client refreshes on the death-zone)
	client:UntrainDiscAll(false)                             -- clear TRAINED disciplines too -- they live in
	                                                         -- character_disciplines (separate from the spellbook),
	                                                         -- so UnscribeSpellAll misses them and the Combat
	                                                         -- Abilities window keeps them across a roguelite death.
	lost[#lost + 1] = "All memorized spells, disciplines, and your spellbook"

	-- reward-gated COMBAT abilities (Backstab/Kick/etc.) -> reset to 0 so they're re-earned via the
	-- level-up picker. (Caller re-sends SKILLUNLOCKDATA so the client re-hides them.)
	local reset_combat = false
	for id in pairs(skills) do
		if (client:GetSkill(id) or 0) > 0 then client:SetSkill(id, 0); reset_combat = true end
	end
	if reset_combat then lost[#lost + 1] = "Your combat abilities" end

	return lost
end

-- Report the loss. The dll "You Lost" window renders the list (swallows LOSTDATA); we send just one
-- flavor line to chat so there's no duplicate list. (SHOW_CHAT_LIST = fallback when no client mod.)
local SHOW_CHAT_LIST = false
function M.announce(client, lost)
	client:Message(MT.Red, "Death strips you bare -- everything you carried is gone.")
	if SHOW_CHAT_LIST then
		for _, name in ipairs(lost) do client:Message(MT.Red, "   - " .. name) end
	end
	local clean = {}
	for _, name in ipairs(lost) do clean[#clean + 1] = (name:gsub("[%^|]", " ")) end  -- ^ | are delimiters
	client:Message(MT.NPCQuestSay, "LOSTDATA " .. table.concat(clean, "^"))
end

return M
