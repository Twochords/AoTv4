-- Life Ebb (50027)
-- "Converts 4% of max hp to 1% of max mana per tick."
--
-- A deliberately lossy trade -- 4% health buys 1% mana. Refuses to run below
-- SAFETY_PCT of max HP so the buff can never be the thing that kills you; it
-- simply idles until you heal back up.
local ab = require("aotv4_abilities")

local HP_PCT     = 4
local MANA_PCT   = 1
local SAFETY_PCT = 20   -- never ebb below this much of max HP

function event_spell_buff_tic(e)
	local mob = e.target
	if not mob or not mob.valid or not mob:IsClient() then
		return
	end
	local client = mob:CastToClient()

	local mana, max_mana = client:GetMana(), client:GetMaxMana()
	if mana >= max_mana then
		return                       -- nothing to gain, don't spend health
	end

	local hp, max_hp = client:GetHP(), client:GetMaxHP()
	local cost  = ab.pct_of(max_hp, HP_PCT)
	local floor = ab.pct_of(max_hp, SAFETY_PCT)
	if hp - cost < floor then
		return                       -- too hurt to keep trading
	end

	client:SetHP(hp - cost)

	local gain = ab.pct_of(max_mana, MANA_PCT)
	if mana + gain > max_mana then
		gain = max_mana - mana
	end
	client:SetMana(mana + gain)
end
