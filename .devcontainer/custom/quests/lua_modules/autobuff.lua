-- autobuff.lua
-- Beneficial buffs/songs are PERMANENT while they sit in a spell gem. Every few seconds we reconcile a
-- character's buffs with their memorized beneficial buff/song spells:
--   * a memmed beneficial buff/song that isn't active is applied as a PERMANENT buff
--     (client:ApplySpellBuff(id, -1) -> the server maps duration <= -1 to PERMANENT_BUFF_DURATION, so it
--      never ticks down),
--   * a buff we previously applied that is no longer on the bar is faded.
-- Works for bard songs and every class's buffs alike. Driven from global_player.lua (event_enter_zone
-- starts a repeating "autobuff" timer + syncs; event_timer + event_connect also call M.sync).

local M = {}

-- a memmed spell becomes a permanent aura if it's BENEFICIAL and a BUFF/SONG (i.e. it has a duration).
-- instant beneficials (Gate, Bind, heals, ports) have no buff duration, so they're naturally excluded.
local function is_aura(id)
	return id and id > 0 and eq.is_beneficial_spell(id) and (eq.is_buff_spell(id) or eq.is_bard_song(id))
end

function M.sync(c)
	if not c or not c.IsClient or not c:IsClient() then return end

	-- desired auras = the beneficial buffs/songs currently memorized in gems
	local desired = {}
	for _, id in ipairs(c:GetMemmedSpells()) do          -- returns only valid gem spell ids
		if is_aura(id) then
			desired[id] = true
			if not c:FindBuff(id) then
				c:ApplySpellBuff(id, -1)                 -- -1 -> PERMANENT_BUFF_DURATION (never fades)
			end
		end
	end

	-- fade any aura WE applied that is no longer on the bar (bucket persists across zones/relogs so we
	-- only fade our own auras, never a legitimately-cast/group buff)
	local key  = "autobuff_" .. c:CharacterID()
	local prev = eq.get_data(key) or ""
	for sid in prev:gmatch("%d+") do
		local id = tonumber(sid)
		if not desired[id] and c:FindBuff(id) then
			c:BuffFadeBySpellID(id)
		end
	end

	-- remember what we applied this pass
	local out = {}
	for id in pairs(desired) do out[#out + 1] = id end
	eq.set_data(key, table.concat(out, ","))
end

return M
