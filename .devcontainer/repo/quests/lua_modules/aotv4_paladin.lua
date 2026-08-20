-- aotv4_paladin.lua -- the payloads behind the three Paladin class AAs.
--
-- The AA rows and the spells they cast are created by migration v94. The AAs are hosted on disabled
-- native rows (45 / 55 / 79) because a NEW aa_ability id never reaches the client (CLAUDE.md
-- section 10), and they are gated to Paladin with `aa_ability.classes = 4`.
--
-- ⚠️⚠️ NONE OF THESE THREE EFFECTS CAN BE EXPRESSED IN `spells_new`, which is why there is Lua here
-- at all:
--   * Ardent Strike scales with STRENGTH. No SPA reads a caster stat as its magnitude.
--   * Hand of Conviction heals a PERCENTAGE OF THE CASTER'S max HP. SPA 147 is a percentage of the
--     TARGET'S max HP, which is a different spell entirely -- it would heal a full-health tank for
--     nothing and a dying one for everything.
--   * Divine Reproach shortens ANOTHER ABILITY'S cooldown. Nothing in the spell system reaches a
--     recast timer.
--
-- ⚠️ The stun on Divine Reproach is deliberately NOT here: it is a real SPA 21 on spell 44602, so
-- the engine owns resist, immunity and the stun message. Only the cooldown cut is paid in Lua.

local M = {}

-- ---------------------------------------------------------------- tuning
-- Ardent Strike. Damage is the BETTER of a level floor and a Strength scale, so the ability is
-- never dead: a level 1 Paladin with 75 STR gets the floor, a geared one gets the scale.
-- ⚠️ Both halves are needed. Strength alone is useless at level 1 (base STR is ~75, and a naked
-- character's STR does not rise with level -- section 24 records that stats come entirely from gear
-- on this server), and a level floor alone would make Strength pointless forever.
-- ⚠️⚠️ IT IS A WEAPON SWING, NOT A NUKE, AND THAT IS THE WHOLE POINT. The first version computed a
-- number and applied it with Damage(), which bypasses everything that keeps melee honest: Mob::Damage
-- does NOT run MeleeMitigation (that lives in Mob::Attack, attack.cpp:1669), so the full figure landed
-- every time regardless of the target's AC. `max(level*3, STR*0.75)` then read as ~150 at level 15
-- with modest gear -- reported from play as hitting way too hard, and correctly so.
-- Now it makes a REAL main-hand attack: the equipped weapon, the target's mitigation, avoidance,
-- ripostes and procs all apply, exactly as if you had swung. Strength still scales it, because
-- Strength scales every melee hit -- we no longer have to reimplement that.
M.STRIKE_HAND      = 13     -- invslot::slotPrimary. Section 33 records Primary as slot 13.
-- A small bonus on top, so the ability beats an ordinary swing without becoming spell damage again.
-- ⚠️ Keep this SMALL. It is unmitigated, so every point is a point the target cannot reduce.
M.STRIKE_PER_LEVEL = 1      -- bonus floor = level * this
M.STRIKE_PER_STR   = 0.10   -- bonus scale = STR   * this
M.STRIKE_SKILL     = 10     -- SkillType for the bonus's damage message

-- Hand of Conviction: heals every group member for this share of the PALADIN'S max HP.
M.CONVICTION_PCT = 0.25

-- Divine Reproach: seconds cut from Hand of Conviction's recast, per use.
-- ⚠️ AA ABILITY id, not a rank id -- AoTv4ReduceAATimer resolves the rank itself.
M.CONVICTION_AA  = 55
M.REPROACH_CUT   = 5

-- ---------------------------------------------------------------- helpers
-- ⚠️⚠️ `e.caster_id` IS AN ENTITY ID AND THE EVENT GIVES US NOTHING ELSE. Resolve through the entity
-- list, and through GetClientByID rather than GetMobID: every one of these abilities is a player
-- ability, and a pet or NPC somehow casting one should do nothing rather than half-work.
-- 📌 Section 10 records that EVENT_SPELL_EFFECT_NPC had NO argument builder until 2026-07-29, so
-- `e.caster_id` was nil against any monster and scripts like this silently did nothing. It is wired
-- now, but that is the first thing to re-check if one of these ever stops firing on a mob.
local function caster_of(e)
	if not e or not e.caster_id or e.caster_id == 0 then return nil end
	local c = eq.get_entity_list():GetClientByID(e.caster_id)
	if not c or not c.valid then return nil end
	return c
end

-- ---------------------------------------------------------------- 1. Ardent Strike  (spell 44600)
-- ⚠️⚠️ THE SPELL EVENT HANDS YOU `e.target`, NOT `e.self`. `handle_spell_event`
-- (zone/lua_parser_events.cpp) sets exactly seven fields -- target, spell_id, caster_id,
-- tics_remaining, caster_level, buff_slot, spell -- and `self` is not among them. Reading `e.self`
-- gives nil, and the first method call on it dies with "attempt to call method 'Damage' (a nil
-- value)", which reads like a missing binding rather than a missing field.
-- 📌 `e.target` is a Lua_Mob, so only Mob methods are safe on it -- section 24 records the same trap
-- for `e.other`, where IsClient() answers true but CharacterID is not defined.
function M.ardent_strike(e)
	local c = caster_of(e)
	if not c then return end
	local target = e.target
	if not target or not target.valid then return end

	-- The swing itself. Everything that makes a melee hit a melee hit happens in here.
	-- ⚠️ Returns false on a miss/dodge/parry/riposte, which is correct and is why the bonus below is
	-- gated on it -- a missed strike should add nothing.
	local landed = c:Attack(target, M.STRIKE_HAND)
	if not landed then return end

	local bonus = math.max((c:GetLevel() or 1) * M.STRIKE_PER_LEVEL,
	                       math.floor((c:GetSTR() or 0) * M.STRIKE_PER_STR))
	if bonus <= 0 then return end

	-- ⚠️ spell_id 0, not 44600: passing the spell id makes this MAGIC damage on the damage report and
	-- lets spell-damage modifiers touch it. This is the divine part of a weapon blow, so it is
	-- reported as melee.
	-- ⚠️ `avoidable = false` -- the swing above already rolled avoidance. Rolling it twice for one
	-- button would make the ability miss far more often than a normal attack.
	target:Damage(c, bonus, 0, M.STRIKE_SKILL, false)
end

-- ---------------------------------------------------------------- 2. Hand of Conviction (44601)
-- Spell 44601 is SELF target, so `e.self` is the Paladin; the group is walked here.
function M.hand_of_conviction(e)
	local c = caster_of(e)
	if not c then return end

	local amount = math.floor((c:GetMaxHP() or 0) * M.CONVICTION_PCT)
	if amount <= 0 then return end

	local grp = c:GetGroup()
	if not grp or not grp.valid then
		-- Solo: heal yourself. An ability that does nothing without a group would be dead weight for
		-- most of a run.
		c:HealDamage(amount, c)
		c:Message(MT.Yellow, string.format("Your conviction restores %d hit points.", amount))
		return
	end

	-- ⚠️ GetMember hands back a Lua_Mob and only members IN THIS ZONE resolve through the entity
	-- list -- the same limit section 24 records. A member elsewhere simply is not healed; there is no
	-- cross-zone heal and pretending otherwise would be a lie in the message below.
	local healed = 0
	for i = 0, (grp:GroupCount() or 0) - 1 do
		local m = grp:GetMember(i)
		if m and m.valid then
			local mc = eq.get_entity_list():GetClientByID(m:GetID())
			if mc and mc.valid then
				mc:HealDamage(amount, c)
				healed = healed + 1
			end
		end
	end
	c:Message(MT.Yellow, string.format("Your conviction restores %d hit points to %d in your group.",
		amount, healed))
end

-- ---------------------------------------------------------------- 3. Divine Reproach   (44602)
-- The stun is the spell's own SPA 21. This only shortens Hand of Conviction.
function M.divine_reproach(e)
	local c = caster_of(e)
	if not c then return end

	-- ⚠️⚠️ RETURNS FALSE WHEN THE ABILITY IS UNKNOWN OR ALREADY READY, and both are normal: a Paladin
	-- below level 5 has not been granted Hand of Conviction, and one who has not used it has nothing
	-- to cut. Saying nothing in those cases is correct -- a refusal message on every stun would be
	-- noise.
	if c:AoTv4ReduceAATimer(M.CONVICTION_AA, M.REPROACH_CUT) then
		c:Message(MT.Yellow, string.format("Divine Reproach hastens Hand of Conviction by %d seconds.",
			M.REPROACH_CUT))
	end
end

return M
