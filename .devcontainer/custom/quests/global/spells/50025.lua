-- Breeze (50025)
-- "Converts 1 point of endurance into 2% of max mana per tick."
--
-- An always-on self buff: it quietly trades stamina for casting fuel every tick.
-- Stops converting (rather than going negative) once endurance runs out.
local ab = require("aotv4_abilities")

local END_COST  = 1
local MANA_PCT  = 2

function event_spell_buff_tic(e)
	local mob = e.target
	if not mob or not mob.valid or not mob:IsClient() then
		return
	end
	local client = mob:CastToClient()

	local endurance = client:GetEndurance()
	if endurance < END_COST then
		return                      -- out of gas; buff stays, just does nothing
	end

	local mana     = client:GetMana()
	local max_mana = client:GetMaxMana()
	if mana >= max_mana then
		return                      -- already full, don't burn endurance for nothing
	end

	client:SetEndurance(endurance - END_COST)

	local gain = ab.pct_of(max_mana, MANA_PCT)
	if mana + gain > max_mana then
		gain = max_mana - mana
	end
	client:SetMana(mana + gain)
end
