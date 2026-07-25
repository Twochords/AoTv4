-- aotv4_reactions.lua
-- The "when you are hit" half of the custom ability set: Divine Aura, Blade
-- Turn, Counterattack and Vengeful Aura. All four hang off a single
-- EVENT_DAMAGE_TAKEN hook wired in global_player.lua.
--
-- WHY THIS EVENT CAN ABSORB DAMAGE
-- zone/attack.cpp:4404 assigns our return value to damage_override and then:
--     if (damage_override > 0)      damage = damage_override;
--     else if (damage_override < 0) damage = 0;
--     SetHP(GetHP() - damage);
-- ...so returning a positive number REPLACES the damage, a negative number
-- NEGATES it entirely, and 0/nil leaves it alone. It runs before SetHP, so this
-- genuinely prevents damage rather than healing it back afterwards.
--
-- WHY CHARGES ARE TRACKED HERE INSTEAD OF numhits
-- "for the next N attacks" looks like a job for spells_new.numhits with
-- numhitstype 6 (IncomingHitSuccess), but CheckNumHitsRemaining fires at
-- attack.cpp:4336 -- BEFORE our event at 4404. On the final charge the buff has
-- already faded by the time this code runs, so the last hit would silently do
-- nothing. Counting here keeps every charge.
--
-- CAVEAT: charge state lives in this zone process, so zoning with an active
-- Counterattack / Vengeful Aura refills its charges. Buffs are short (4 tics),
-- so this is rare and errs in the player's favour. Persisting it would mean a
-- DB write on every melee swing, which is far worse.

local ab = require("aotv4_abilities")

local M = {}

------------------------------------------------------------------ ability ids
M.DIVINE_AURA   = 50022
M.BLADE_TURN    = 50035
M.COUNTERATTACK = 50056
M.VENGEFUL_AURA = 50059

-- helper carriers (see gen_spells.py HELPERS)
M.OPEN_WOUNDS_MARK = 50150
M.DUEL_LOCK        = 50155

------------------------------------------------------------------ tuning
local DA_THRESHOLD_PCT   = 15   -- Divine Aura absorbs hits under this % of max HP
local BLADE_RECHARGE_SEC = 30   -- Blade Turn charge comes back this often
local BLADE_REFRESH_END  = 5    -- ...and costs this much endurance to do so
local COUNTER_CHARGES    = 5
local COUNTER_PCT        = 250  -- % of primary DPS struck back
local VENGEFUL_CHARGES   = 3
local VENGEFUL_MULT      = 2    -- physical damage taken is doubled
local BLEED_PCT          = 25   -- Open Wounds banks this % of every hit as bleed
local BLEED_TICS         = 5    -- ...and pays it out over this many tics
local DUEL_MULT          = 2    -- melee damage between duel partners is doubled

------------------------------------------------------------------ state
-- [character_id] = charges remaining
local counter_charges  = {}
local vengeful_charges = {}
-- [character_id] = { ready = bool, next_at = os.time() when it recharges }
local blade = {}

function M.arm_counterattack(char_id) counter_charges[char_id]  = COUNTER_CHARGES  end
function M.arm_vengeful(char_id)      vengeful_charges[char_id] = VENGEFUL_CHARGES end
function M.arm_blade(char_id)         blade[char_id] = { ready = true, next_at = 0 } end

function M.clear_counterattack(char_id) counter_charges[char_id]  = nil end
function M.clear_vengeful(char_id)      vengeful_charges[char_id] = nil end
function M.clear_blade(char_id)         blade[char_id] = nil end

-- Open Wounds: [victim entity_id] = unpaid bleed damage, banked by hits and
-- paid out over BLEED_TICS by the marker's buff tic.
local bleed_pool = {}

function M.clear_bleed(entity_id) bleed_pool[entity_id] = nil end

-- Duel: [entity_id] = partner entity_id. Symmetric -- both directions are set
-- when the duel starts and both are cleared when it ends, so a half-open duel
-- (one side immune, the other not) can never happen.
local duel_partner = {}

function M.start_duel(a_id, b_id)
	duel_partner[a_id] = b_id
	duel_partner[b_id] = a_id
end

function M.end_duel(entity_id)
	local partner = duel_partner[entity_id]
	duel_partner[entity_id] = nil
	if partner then
		duel_partner[partner] = nil
	end
end

function M.duel_partner_of(entity_id) return duel_partner[entity_id] end

------------------------------------------------------------------ helpers

-- Melee damage carries no spell id. Mob::Damage defaults spell_id to
-- SPELL_UNKNOWN (0xFFFF), and some paths pass 0, so treat both as physical.
-- Damage shields and buff tics are never "a physical attack" for our purposes.
local function is_physical_hit(e)
	if e.is_damage_shield or e.is_buff_tic then
		return false
	end
	return e.spell_id == 0 or e.spell_id == 65535
end

-- "in front of you" -- mirrors the stock idiom at zone/attack.cpp:419.
local function in_front_of(me, attacker)
	if not attacker or not attacker.valid then
		return false
	end
	return not attacker:BehindMob(me, attacker:GetX(), attacker:GetY())
end

------------------------------------------------------------------ main hook

-- Returns a damage override for EVENT_DAMAGE_TAKEN: negative negates the hit,
-- positive replaces it, nil/0 leaves it untouched.
function M.on_damage_taken(e)
	local me = e.self
	if not me or not me.valid or not me:IsClient() then
		return 0
	end

	local client  = me:CastToClient()
	local char_id = client:CharacterID()
	local damage  = e.damage or 0
	if damage <= 0 then
		return 0
	end

	local attacker = nil
	if e.entity_id and e.entity_id > 0 then
		attacker = eq.get_entity_list():GetMobID(e.entity_id)
	end

	local physical = is_physical_hit(e)

	-- 0. Duel outranks everything: while locked in, the pair exist in their own
	--    little world. Anything that isn't your opponent simply cannot touch you,
	--    and what your opponent lands hits twice as hard.
	local partner_id = duel_partner[me:GetID()]
	if partner_id then
		if e.entity_id ~= partner_id then
			return -1                       -- immune to the outside world
		end
		if physical then
			return damage * DUEL_MULT
		end
		return 0
	end

	-- 1. Divine Aura -- shrugs off anything that isn't a big hit. Checked first
	--    so an absorbed hit never burns another ability's charge.
	if me:FindBuff(M.DIVINE_AURA) then
		local threshold = ab.pct_of(me:GetMaxHP(), DA_THRESHOLD_PCT)
		if damage < threshold then
			return -1
		end
	end

	-- 2. Blade Turn -- eats one physical attack, then recharges on a timer for
	--    an endurance fee. The refresh is lazy (checked on the next hit) rather
	--    than driven by a timer, so there is nothing to clean up.
	if physical and me:FindBuff(M.BLADE_TURN) then
		local state = blade[char_id]
		if not state then
			state = { ready = true, next_at = 0 }
			blade[char_id] = state
		end

		if not state.ready and os.time() >= state.next_at then
			local endurance = client:GetEndurance()
			if endurance >= BLADE_REFRESH_END then
				client:SetEndurance(endurance - BLADE_REFRESH_END)
				state.ready = true
			end
		end

		if state.ready then
			state.ready  = false
			state.next_at = os.time() + BLADE_RECHARGE_SEC
			return -1
		end
	end

	-- 3. Counterattack -- retaliate, without changing the damage taken.
	if me:FindBuff(M.COUNTERATTACK) then
		local left = counter_charges[char_id] or COUNTER_CHARGES
		if left > 0 and attacker and in_front_of(me, attacker) then
			counter_charges[char_id] = left - 1
			ab.pct_dps_attack(me, attacker, COUNTER_PCT, ab.SLOT_PRIMARY, ab.SKILL_OFFENSE)
		end
	end

	-- 4. Vengeful Aura -- doubles the physical damage you take, then strikes the
	--    attacker for that FINAL (doubled) amount.
	if physical and me:FindBuff(M.VENGEFUL_AURA) then
		local left = vengeful_charges[char_id] or VENGEFUL_CHARGES
		if left > 0 then
			vengeful_charges[char_id] = left - 1
			local final = damage * VENGEFUL_MULT
			if attacker and attacker.valid then
				attacker:Damage(me, final, M.VENGEFUL_AURA, ab.SKILL_OFFENSE, false)
			end
			return final
		end
	end

	return 0
end

------------------------------------------------------- Open Wounds bleed

-- Every hit on a marked victim banks BLEED_PCT of its damage. The marker lasts
-- a minute, so sustained pressure keeps topping the pool up -- exactly the
-- "attacks on this target ... will cause them to take 25% additional bleed
-- damage" wording.
function M.on_damage_given(e)
	if not e.entity_id or e.entity_id == 0 then
		return 0
	end
	local damage = e.damage or 0
	if damage <= 0 or e.is_damage_shield or e.is_buff_tic then
		return 0
	end

	local victim = eq.get_entity_list():GetMobID(e.entity_id)
	if not victim or not victim.valid or not victim:FindBuff(M.OPEN_WOUNDS_MARK) then
		return 0
	end

	local banked = math.floor(damage * BLEED_PCT / 100)
	if banked > 0 then
		bleed_pool[e.entity_id] = (bleed_pool[e.entity_id] or 0) + banked
	end
	return 0
end

-- Paid out a fifth at a time by the marker's buff tic, so damage banked from a
-- flurry bleeds out over the following BLEED_TICS rather than all at once.
function M.bleed_tick(victim, caster)
	if not victim or not victim.valid then
		return
	end
	local id     = victim:GetID()
	local pool   = bleed_pool[id]
	if not pool or pool <= 0 then
		return
	end

	local payout = math.ceil(pool / BLEED_TICS)
	if payout > pool then payout = pool end
	bleed_pool[id] = pool - payout
	if bleed_pool[id] <= 0 then
		bleed_pool[id] = nil
	end

	victim:Damage(caster or victim, payout, M.OPEN_WOUNDS_MARK, ab.SKILL_OFFENSE, false)
end

return M
