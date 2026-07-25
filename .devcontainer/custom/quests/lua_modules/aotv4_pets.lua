-- aotv4_pets.lua
-- The five Summon abilities: getting the pet out, and what it does once it is.
--
-- "Pets cannot be summoned via casting and instead will summon automatically
--  outside of combat."
--
-- So the ability is a scribed TRAIT, not a castable spell. A repeating "aotpet"
-- timer in global_player calls maybe_summon(); global_npc's event_damage_given
-- and event_spawn call the behaviour hooks. The Summon spell rows deliberately
-- have NO script of their own -- casting one does nothing, by design.
--
-- HOW THE PET IS ACTUALLY CREATED
-- Mob::MakePet has no Lua binding, so the only route to a real owned pet is
-- finishing a spell carrying SPA 33 (SummonPet). Each such spell names ONE pet
-- type and pet types are level-banded, so gen_summons.py emits one helper spell
-- per pet per band and aotv4_summon_table.lua maps band -> spell.

local ab           = require("aotv4_abilities")
local summon_table = require("aotv4_summon_table")

local M = {}

M.BEAR      = 50044
M.LEOPARD   = 50045
M.SKELETON  = 50046
M.FIRE_IMP  = 50047
M.WILLOWISP = 50048

-- ================================================================ summoning
local ORDER = { M.BEAR, M.LEOPARD, M.SKELETON, M.FIRE_IMP, M.WILLOWISP }

-- Highest band at or below the player's level.
local function helper_for(ability_id, level)
	local bands = summon_table[ability_id]
	if not bands then
		return nil
	end
	local chosen = bands[1].spell
	for _, entry in ipairs(bands) do
		if level >= entry.band then
			chosen = entry.spell
		end
	end
	return chosen
end

-- Called on a timer. Cheap enough to run often: the common case is one
-- HasPet() check and an early return.
function M.maybe_summon(client)
	if not client or not client.valid then
		return
	end
	if client:HasPet() or client:IsEngaged() then
		return
	end

	for _, ability_id in ipairs(ORDER) do
		if client:HasSpellScribed(ability_id) then
			local helper = helper_for(ability_id, client:GetLevel())
			if helper then
				client:SpellFinished(helper, client)
			end
			return
		end
	end
end

-- ================================================================ behaviour
------------------------------------------------------------------ tuning
local RACE_BEAR     = 43
local RACE_LEOPARD  = 439
local RACE_SKELETON = 367
local RACE_ELEMENTAL = 75

local BACKSTAB_BONUS_PCT = 100   -- Leopard: +100% when behind
local LEECH_PCT          = 25    -- Skeleton/Willowisp: % of the hit that is leeched
local IMP_BURN_CHANCE    = 20    -- Fire Imp: % chance per swing to burn
local IMP_BURN_SPELL     = 50000 -- Ember, from the custom set
local MAX_GROUP_SLOT     = 5

------------------------------------------------------------------ helpers

-- Spread an amount across the owner's group; solo owners get all of it.
local function to_party(owner, apply)
	if not owner or not owner.valid then
		return
	end
	local group = owner:IsClient() and owner:CastToClient():GetGroup() or nil
	if not group or not group.valid then
		apply(owner)
		return
	end
	for i = 0, MAX_GROUP_SLOT do
		local member = group:GetMember(i)
		if member and member.valid then
			apply(member)
		end
	end
end

------------------------------------------------------------------ main hook

-- Called from global_npc event_damage_given. `pet` is the NPC that just hit
-- `victim` for `damage`.
function M.on_pet_damage(pet, victim, damage, spell_id)
	if not pet or not pet.valid or not pet:IsPet() then
		return
	end
	-- only autoattacks drive these; pet spell damage shouldn't re-trigger them
	if spell_id and spell_id ~= 0 and spell_id ~= 65535 then
		return
	end
	if not damage or damage <= 0 then
		return
	end

	local owner = pet:GetOwner()
	if not owner or not owner.valid then
		return
	end

	local race = pet:GetRace()

	-- Leopard: rewards positioning
	if race == RACE_LEOPARD then
		if victim and victim.valid and pet:BehindMob(victim, pet:GetX(), pet:GetY()) then
			local bonus = math.floor(damage * BACKSTAB_BONUS_PCT / 100)
			if bonus > 0 then
				victim:Damage(pet, bonus, 0, ab.SKILL_OFFENSE, false)
			end
		end
		return
	end

	-- Skeleton: drains the enemy to patch up the party
	if race == RACE_SKELETON then
		local leech = math.floor(damage * LEECH_PCT / 100)
		if leech > 0 then
			to_party(owner, function(m) m:HealDamage(leech) end)
		end
		return
	end

	if race == RACE_ELEMENTAL and owner:IsClient() then
		local client = owner:CastToClient()

		-- Willowisp: feeds the party mana
		if client:HasSpellScribed(M.WILLOWISP) then
			local leech = math.floor(damage * LEECH_PCT / 100)
			if leech > 0 then
				to_party(owner, function(m)
					local mana, max_mana = m:GetMana(), m:GetMaxMana()
					if mana < max_mana then
						m:SetMana(math.min(max_mana, mana + leech))
					end
				end)
			end
			return
		end

		-- Fire Imp: sets things alight now and then
		if client:HasSpellScribed(M.FIRE_IMP) then
			if victim and victim.valid and math.random(100) <= IMP_BURN_CHANCE then
				pet:SpellFinished(IMP_BURN_SPELL, victim)
			end
		end
	end
end

-- Bear holds aggro: taunt on, and keep it that way.
function M.on_pet_spawn(pet)
	if not pet or not pet.valid or not pet:IsPet() then
		return
	end
	if pet:GetRace() == RACE_BEAR then
		pet:SetPetOrder(1)   -- SPO_Guard-ish: stand and hold, taunting
	end
end

return M
