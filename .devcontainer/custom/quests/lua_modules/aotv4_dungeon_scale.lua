-- aotv4_dungeon_scale.lua -- scale delve creatures to the PLAYER, not just to the layer level.
--
-- Two scaling terms, and both are needed:
--   BASELINE   the layer's level. Picking the level 60 delve should mean level 60 content.
--   DYNAMIC    how far the player is above the NAKED expectation for their level. This is the gear
--              bloat correction, and it is what stops a fully geared character walking the level 50
--              layer as though it were empty.
--
-- WHY MEASURED THIS WAY: in EQ, stats / AC / resists do NOT grow with level on their own -- a naked
-- level 70 has the same STR as a naked level 1, because all of it comes from gear. Only HP, mana and
-- endurance scale with level. So gear bloat is directly visible as "actual over naked expectation",
-- and only the HP/mana axis needs a per-level curve at all.
--
-- THE NAKED BASELINE is the level 1 character in the reference screenshot (Warrior, Karana):
--   HP 38   MP 40   EN 21   AC 31   ATK 33
--   STR 75  STA 100  AGI 100  DEX 80  WIS 80  INT 75  CHA 75      (sum 585)
--   MAGIC 25  FIRE 25  COLD 25  DISEASE 25  POISON 15  CORRUPT 15  (sum 130)
-- ⚠️ Those stat and resist numbers are RACE and CLASS dependent, so using them as one universal
-- baseline is an approximation -- a Troll starts with more STA and a Gnome with more INT. It is close
-- enough for a difficulty knob, and it errs in the right direction: the axis that matters most (HP)
-- is taken from base_data, not from the screenshot.
--
-- ⚠️⚠️ REWARDS MUST NOT BE COMPUTED FROM THE PLAYER'S GEAR AT THE END OF THE RUN. Somebody who
-- clears the whole dungeon naked (weak mobs) and then puts their gear on before turning it in has
-- earned a weak reward, and somebody who fights the whole thing geared has earned a strong one. So
-- the ledger records THE EFFECTIVE LEVEL OF EVERY MOB ACTUALLY KILLED, at the moment it died. That
-- number cannot be retro-edited by changing clothes, which makes the whole class of gear-swap
-- exploits structurally impossible rather than merely detected. See M.record_kill / M.ledger.

local M = {}

-- ---------------------------------------------------------------- the level curve
-- base_data.hp for any class is exactly CLASS_HP[class] * SHAPE(level), and base_data.hp_fac is
-- CLASS_HP_FAC[class] * SHAPE(level) -- the shape is identical for all 16 classes and only the
-- per-class unit differs (warrior 25, magician 20, and warrior/magician hp is a constant 1.25 ratio
-- at every level). Read straight out of base_data, not guessed.
--
--   level  5 10 15 20 25 30 35 40 45 50 55 60 65 70
--   shape  5 10 15 20 25 30 35 40 50 60 70 80 90 100
--
-- ⚠️ It is LINEAR to 40 and then DOUBLE RATE. Treating it as linear all the way would understate the
-- expectation at level 70 by 30 percent (70 against 100) and make every high level character look
-- geared even when naked.
local function shape(level)
    if level < 1 then level = 1 end
    if level <= 40 then return level end
    return 40 + 2 * (level - 40)
end

-- base_data level-1 values, indexed by class id 1..16.
local CLASS_HP     = { 25, 22, 24, 23, 24, 20, 21.3, 22, 21.3, 21.3, 20, 20, 20, 20, 21.3, 21.3 }
local CLASS_HP_FAC = { 0.083, 0.073, 0.08, 0.077, 0.08, 0.067, 0.071, 0.073,
                       0.071, 0.071, 0.067, 0.067, 0.067, 0.067, 0.071, 0.071 }
local CLASS_MANA   = { 0, 15, 15, 15, 15, 15, 0, 15, 0, 15, 15, 15, 15, 15, 15, 0 }
local CLASS_MN_FAC = { 0, 0.075, 0.075, 0.075, 0.075, 0.075, 0, 0.075,
                       0, 0.075, 0.075, 0.075, 0.075, 0.075, 0.075, 0 }

-- The level 1 naked baseline for everything that does NOT scale with level.
local BASE_AC      = 31
local BASE_STATS   = 585    -- STR 75 + STA 100 + AGI 100 + DEX 80 + WIS 80 + INT 75 + CHA 75
local BASE_RESISTS = 130    -- MAGIC 25 + FIRE 25 + COLD 25 + DISEASE 25 + POISON 15 + CORRUPT 15

-- ---------------------------------------------------------------- tunables
M.WEIGHTS = { hp = 0.30, mana = 0.15, ac = 0.20, stats = 0.20, resists = 0.15 }

-- How many effective levels a doubling of power is worth, and how far it may move the layer level.
M.BUMP_PER_POWER = 12
M.MAX_BUMP       = 20   -- a fully decked character can push the level 50 layer to 70
M.MAX_DROP       = 5    -- ...and stripping down cannot drop it more than this many levels, OR
M.MAX_DROP_FRAC  = 0.20 -- ...this fraction of the rung, whichever is SMALLER.
-- ⚠️⚠️ THE FRACTION IS WHAT MAKES THE CLAMP MEAN THE SAME THING AT EVERY RUNG. A flat 5 levels is a
-- 10 percent softening at rung 50 and a total collapse at rung 2, where it hits the `eff < 1` floor
-- and spawns a LEVEL 1 dungeon that is both trivial and worth almost nothing. At 0.20 the drop is
-- proportional: 0.4 levels at rung 2, 2 at rung 10, and the flat 5 only takes over from rung 25 up.
-- 📌 There is deliberately no matching fraction on MAX_BUMP: pushing a low rung UP is a choice the
-- player made by gearing, and it makes the dungeon harder and worth more -- it cannot produce the
-- degenerate "nothing to fight, nothing to earn" state that the downward clamp could.


-- Power has to move by this much before living mobs are re-scaled. Without a deadband a single buff
-- landing or fading would re-scale the whole zone.
M.RESCALE_DELTA = 0.08

-- ---------------------------------------------------------------- power measurement
-- Returns power (1.0 == naked expectation for this level and class) plus the per-axis breakdown, so
-- `#delve power` can explain itself instead of printing one inscrutable number.
function M.power(c)
    if not c or not c.valid then return 1.0, nil end

    local level = c:GetLevel()
    local class = c:GetClass()
    if not class or class < 1 or class > 16 then class = 1 end

    local sh  = shape(level)
    local sta = c:GetSTA()
    local wis = c:GetWIS()
    local int = c:GetINT()

    -- ⚠️ The expectation uses the player's CURRENT stamina on purpose. HP granted BY stamina then
    -- lands in the stats axis rather than being counted twice -- once as raw HP and again as STA.
    local exp_hp = sh * (CLASS_HP[class] + CLASS_HP_FAC[class] * sta)

    -- Casters key mana off wis or int by class; use whichever is higher rather than a class table,
    -- which is close enough here and cannot be wrong for a hybrid.
    local casting = (wis > int) and wis or int
    local exp_mana = sh * (CLASS_MANA[class] + CLASS_MN_FAC[class] * casting)

    local stats = c:GetSTR() + c:GetSTA() + c:GetAGI() + c:GetDEX()
                + c:GetWIS() + c:GetINT() + c:GetCHA()
    local resists = c:GetMR() + c:GetCR() + c:GetDR() + c:GetFR() + c:GetPR() + c:GetCorruption()

    local function ratio(actual, expected)
        if not expected or expected <= 0 then return nil end   -- axis does not apply (mana on a pure melee)
        local r = actual / expected
        if r < 0.25 then r = 0.25 end                          -- floor: never let a 0 collapse the mean
        return r
    end

    local axes = {
        hp      = ratio(c:GetMaxHP(),   exp_hp),
        mana    = ratio(c:GetMaxMana(), exp_mana),
        ac      = ratio(c:GetAC(),      BASE_AC),
        stats   = ratio(stats,          BASE_STATS),
        resists = ratio(resists,        BASE_RESISTS),
    }

    -- Weighted mean over the axes that APPLY. ⚠️ Renormalise by the weight actually used: a Warrior
    -- has no mana, and dividing by the full weight total would score every melee class as 15 percent
    -- weaker than it is and quietly make their delves easier.
    local sum, used = 0.0, 0.0
    for axis, w in pairs(M.WEIGHTS) do
        local r = axes[axis]
        if r then sum = sum + w * r; used = used + w end
    end
    if used <= 0 then return 1.0, axes end

    return sum / used, axes
end

-- ---------------------------------------------------------------- effective level
-- The layer level is the floor of the experience; power moves it.
function M.effective_level(layer_level, c)
    local power = M.power(c)
    local delta = M.BUMP_PER_POWER * (power - 1.0)

    if delta > M.MAX_BUMP then delta = M.MAX_BUMP end

    -- ⚠️⚠️ THE DOWNWARD CLAMP IS A FRACTION OF THE RUNG, NOT A FLAT NUMBER OF LEVELS -- and getting
    -- this wrong is what made an entire rung spawn at level 1.
    --
    -- MAX_DROP was a flat 5. Across a ladder that spans rungs 1 to 70 that is not one rule, it is two
    -- completely different ones: minus 5 at rung 50 is a 10 percent softening, while minus 5 at rung 2
    -- lands on the `eff < 1` clamp and produces a LEVEL 1 DUNGEON. Measured in play, a rung 2 run
    -- returned ~36 points -- 30 kills and a boss, every one of them level 1 -- because the mobs really
    -- were all level 1, not because the scoring was wrong.
    --
    -- ⚠️ The score is NOT floored to compensate for this, and must not be: kill_value reads the level
    -- the mob ACTUALLY spawned at, so a genuinely level 1 mob is genuinely worth 1. That honesty is
    -- what makes the ledger un-exploitable (section 26), and papering over it here would have paid out
    -- rung-level points for level 1 kills. Fix the level, never the accounting.
    local max_drop = layer_level * M.MAX_DROP_FRAC
    if max_drop > M.MAX_DROP then max_drop = M.MAX_DROP end
    if delta < -max_drop then delta = -max_drop end

    -- ⚠️⚠️ THE RUNG IS AN ABSOLUTE LEVEL AND MOBS SPAWN AT IT. Rung 5 is content for a LEVEL 5
    -- character, rung 30 for a level 30 one -- the ladder IS the levelling path, not a difficulty
    -- multiplier layered over whatever level you happen to be.
    --
    -- ⚠️ DO NOT FLOOR THIS AT THE PLAYER'S LEVEL. That was tried (2026-08-01) to make out-levelled
    -- rungs pay experience, and it is wrong twice over: it makes rung 5 spawn level 30 mobs for a
    -- level 30 character -- content explicitly designed to be cleared at level 5 -- and it flattens
    -- the whole ladder, because every rung below your level then spawns at exactly your level. A high
    -- level player finding a low rung trivial is CORRECT; they have outgrown it and belong deeper.
    -- The real problem that change was aimed at is the ENTRY GATE, not the scaling -- see
    -- M.unlocked_count in aotv4_dungeon.lua, which now lets a character start at their own level
    -- instead of grinding up from rung 1 through content far below them.
    local eff = math.floor(layer_level + delta + 0.5)
    if eff < 1  then eff = 1  end
    if eff > 90 then eff = 90 end   -- npc_scale_global_base only covers 1..90

    -- ⚠️⚠️ WHAT A KILL IS WORTH IS THE LEVEL IT ACTUALLY SPAWNED AT -- ONE NUMBER, NOT TWO.
    -- Difficulty and value are deliberately the same thing: under-gear yourself into an easier
    -- dungeon and you earn less, gear up into a harder one and you earn more, and neither can be
    -- gamed after the fact because the level is stamped on the mob when it spawns (section 26).
    -- ⚠️ A separate `worth` was briefly carried alongside `eff` so that score could ignore the gear
    -- term. It is gone: it existed to prop up a low score that was really caused by the flat MAX_DROP
    -- spawning level 1 mobs, and once that was fixed at source the two numbers were always equal.
    -- Do not reintroduce it without a reason that survives "why is the mob's level not the answer?".
    return eff, power
end

-- ---------------------------------------------------------------- applying it to a mob
-- ScaleNPC does the coherent work (hp, damage, ac, resists, regen off npc_scale_global_base); the
-- fine pass afterwards expresses the FRACTION that an integer level cannot.
--
-- ⚠️ ScaleNPC must come FIRST. It rewrites the stats wholesale from the scale table, so any
-- ModifyNPCStat applied before it is simply discarded.
-- ⚠️⚠️ SCALENPC *RAISES* SPELL DAMAGE, IT DOES NOT LOWER IT. This is the opposite of what the name
-- suggests and it is why a level 1 delve was throwing 100+ damage nukes.
--   * The Lua binding passes always_scale = true (lua_npc.cpp:782), so npc_scale_manager.cpp:168
--     runs UNCONDITIONALLY: `ModifyNPCStat("spellscale", scale_data.spell_scale)`.
--   * `npc_scale_global_base.spell_scale` is **100 at every level**, 1 through 90.
--   * The DoN mobs ship at spellscale **50**.
-- So scaling a level 67 caster down to level 1 shrank its hp, melee, AC and resists -- and DOUBLED
-- its spell damage to full authored value. Nothing in ScaleNPC touches the spells themselves: a
-- spell's damage lives in `spells_new`, so a level 1 mob happily casts a level 67 nuke.
-- The same applies to healscale, and there it is worse: an unscaled heal against a scaled-down max_hp
-- is a guaranteed full heal every cast.
--
-- ⚠️ Read the natives ONCE and remember them on the mob. `M.rescale_zone` calls this again mid run,
-- by which point GetLevel() is the PREVIOUS effective level and GetSpellScale() is already 100 --
-- recomputing from those would compound the ratio a little more on every sweep.
-- ⚠️ 0 means "unscaled" to the engine, not "no damage": GetActSpellDamage only applies the multiplier
-- `if (GetSpellScale())`, so 0 behaves as 100 and must be read as 100 here.
local function remember_natives(npc)
    local lvl = tonumber(npc:GetEntityVariable("delve_nat_lvl"))
    local ss  = tonumber(npc:GetEntityVariable("delve_nat_ss"))
    local hs  = tonumber(npc:GetEntityVariable("delve_nat_hs"))
    if not lvl then
        lvl = npc:GetLevel() or 0
        ss  = npc:GetSpellScale() or 0
        hs  = npc:GetHealScale() or 0
        if ss <= 0 then ss = 100 end
        if hs <= 0 then hs = 100 end
        npc:SetEntityVariable("delve_nat_lvl", tostring(lvl))
        npc:SetEntityVariable("delve_nat_ss",  tostring(ss))
        npc:SetEntityVariable("delve_nat_hs",  tostring(hs))
    end
    return lvl, ss, hs
end

-- Put spell and heal output back in proportion to how far the mob was scaled DOWN, and keep the
-- mob's own authored tuning as the ceiling: at eff == native this returns exactly what it shipped
-- with, so a mob that was not scaled down is not quietly nerfed.
-- 📌 Linear in the level ratio. Spell damage does not track level linearly in EQ, but the ratio is
-- the honest first approximation and it is trivially re-tunable here in ONE place.
local function scale_spell_output(npc, eff_level)
    local nat_lvl, nat_ss, nat_hs = remember_natives(npc)
    if not nat_lvl then return end
    if not nat_lvl or nat_lvl <= 0 then return end

    local ratio = eff_level / nat_lvl
    if ratio > 1.0 then ratio = 1.0 end            -- never make a mob cast HARDER than it was built to
    if ratio < 0.0 then ratio = 0.0 end

    -- Floor of 1, not 0: 0 reads as "unscaled" to GetActSpellDamage and would restore FULL damage.
    local ss = math.floor(nat_ss * ratio); if ss < 1 then ss = 1 end
    local hs = math.floor(nat_hs * ratio); if hs < 1 then hs = 1 end

    npc:ModifyNPCStat("spellscale", tostring(ss))
    npc:ModifyNPCStat("healscale",  tostring(hs))
end

-- ⚠️⚠️ THE ONLY PLACE THIS PROJECT SHOULD CALL npc:ScaleNPC(). The three steps have to happen in this
-- exact order -- read the natives before ScaleNPC clobbers them, scale, then put spell output back --
-- and the boss path used to call ScaleNPC bare, which is precisely how it kept full spell damage.
-- Wrapping it means a caller cannot get the order wrong, and there is one place to retune.
-- ---------------------------------------------------------------- elite ratio
-- ⚠️⚠️ ScaleNPC FLATTENS EVERY MOB ONTO ONE CURVE, WHICH ERASES NAMED AND ELITE MOBS ENTIRELY.
-- It rewrites hp, damage and ac wholesale out of `npc_scale_global_base` for the level it is given,
-- and it does not care what the mob was authored as. In stillmoona `an_iron_warder` ships at level 67
-- with 40,364 hp while the trash around it ships at 21,000 -- put both through ScaleNPC(5) and they
-- come out byte-identical. Reported from play as "warders are just as weak as the trash mobs", and
-- that is exactly what it was: not a warder bug, a bug affecting every tougher-than-trash mob in
-- every delve.
--
-- The fix is to measure how far above the curve a mob was AUTHORED, and re-apply that multiple after
-- scaling. `elite` is that multiple: 1.0 for ordinary trash, higher for anything hand-tuned.
--
-- ⚠️ Measured by scaling the mob to its OWN native level and reading what the engine gives it there.
-- That is the honest baseline and it needs no copy of `npc_scale_global_base` in Lua (there is no
-- binding to read that table, and embedding a copy would silently rot the moment it is retuned).
-- The double ScaleNPC costs one extra call per mob, once, at spawn.
-- ⚠️ Mobs authored with `hp = 0` -- which is most of them, including many of the warders -- are
-- auto-scaled by the engine already, so they measure as exactly 1.0 and are left alone. That is the
-- correct answer, not a failure to detect them.
--
-- ⚠️ Cached in an entity variable like the other natives: the 10 second rescale sweep calls this
-- repeatedly, and re-measuring after the mob has already been scaled would read the SCALED hp as if
-- it were native and collapse the ratio to 1.0 -- the elite would decay to trash over a few sweeps.
-- ⚠️ Floored at 1.0. A mob authored WEAKER than its curve is left where the curve puts it; making it
-- weaker still would just recreate the problem from the other side.
local ELITE_CAP     = 4.0    -- a raid-tier mob in a delve should be a fight, not a wall
local ELITE_DMG_POW = 0.5    -- damage scales as the square root of the hp multiple: a 4x tougher
                             -- mob hits 2x harder, not 4x. Elites should last longer first.

local function elite_ratio(npc, nat_lvl)
    local cached = tonumber(npc:GetEntityVariable("delve_elite"))
    if cached then return cached end

    -- ⚠️⚠️ OUR OWN NPCs ARE TEMPLATES, AND MEASURING THEM HERE IS NONSENSE. The delve warden
    -- (2000301) is authored at LEVEL 1 with 1000 hp -- a stub that exists to be scaled, not a real
    -- level 1 creature. The curve at level 1 is ELEVEN, so this measured 1000/11 = 90 and clamped to
    -- ELITE_CAP, handing the boss a spurious flat 4x. Its own chain then multiplied again
    -- (M.BOSS_HP_MULT 5.0), so the end-of-delve warden arrived at 20x the curve instead of 5x.
    -- ⚠️ The id band is the same divider on_npc_spawn uses for the race 127 sweep: everything AoTv4
    -- adds lives at 2000000+, while the DoN zones' own npcs are 338xxx-343xxx. Anything of ours that
    -- needs to be tougher says so through its OWN explicit multipliers, which is the readable place
    -- for it -- never by being re-measured against a curve it was never authored against.
    if (npc:GetNPCTypeID() or 0) >= 2000000 then
        npc:SetEntityVariable("delve_elite", "1.0")
        return 1.0
    end

    local r = 1.0
    local nat_hp = npc:GetMaxHP() or 0
    if nat_lvl and nat_lvl > 0 and nat_hp > 0 then
        npc:ScaleNPC(nat_lvl)                     -- what the curve gives THIS mob at its own level
        local base_hp = npc:GetMaxHP() or 0
        if base_hp > 0 then r = nat_hp / base_hp end
    end

    if r < 1.0        then r = 1.0        end
    if r > ELITE_CAP  then r = ELITE_CAP  end
    npc:SetEntityVariable("delve_elite", string.format("%.4f", r))
    return r
end

-- ⚠️⚠️ RAISING max_hp DOES NOT HEAL THE MOB, SO IT MUST BE REFILLED BY HAND.
-- NPC::ModifyNPCStat's max_hp branch (zone/npc.cpp) is:
--     base_hp = value; CalcMaxHP(); if (current_hp > max_hp) current_hp = max_hp;
-- -- it only ever clamps DOWN. Raise the maximum and current_hp stays exactly where it was, so the
-- creature is left at old/new of its new pool. With ELITE_CAP at 4.0 that is precisely the reported
-- "bosses are spawning at 25% hp", and it was consistent across gauntlet, fragile and standard
-- because it is the CAP being hit, not anything mode specific.
--
-- ⚠️ Safe to refill unconditionally here: everything that scales runs either at spawn (nothing has
-- damaged it yet) or from the out-of-combat rescale sweep, which already refuses to touch a mob that
-- is being fought -- see the note on M.rescale_zone. A mid-fight refill is the thing that guard
-- exists to prevent, and this does not change it.
local function refill(npc)
    if not npc or not npc.valid then return end
    local mx = npc:GetMaxHP()
    if mx and mx > 0 then npc:SetHP(mx) end
end

local function apply_elite(npc, r)
    if not r or r <= 1.0 then return end

    local hp = npc:GetMaxHP()
    if hp and hp > 0 then
        npc:ModifyNPCStat("max_hp", tostring(math.floor(hp * r)))
    end

    -- ⚠️ There is no GetMinDamage binding (only GetMaxDamage, zone/lua_npc.cpp), so min_hit is derived
    -- from max_hit rather than read. Half is the shape ScaleNPC itself produces, so this keeps the
    -- spread the mob would have had.
    local dmg = npc:GetMaxDamage()
    if dmg and dmg > 0 then
        local d = math.floor(dmg * (r ^ ELITE_DMG_POW))
        npc:ModifyNPCStat("max_hit", tostring(d))
        npc:ModifyNPCStat("min_hit", tostring(math.max(1, math.floor(d / 2))))
    end
end

function M.scale_npc(npc, level)
    if not npc or not npc.valid then return end
    local nat_lvl = remember_natives(npc)  -- BEFORE: ScaleNPC overwrites level AND spellscale
    local elite   = elite_ratio(npc, nat_lvl)  -- BEFORE too: it reads the mob's authored hp
    npc:ScaleNPC(level)
    scale_spell_output(npc, level)        -- AFTER: ScaleNPC would discard it otherwise
    apply_elite(npc, elite)               -- AFTER: ScaleNPC would discard this too
    refill(npc)                           -- LAST: apply_elite raised max_hp without healing
end

-- The fine trim factor for a power ratio. ⚠️ SHARED by M.apply and M.regular_hp on purpose: the boss
-- sizes itself against what a regular mob has, so if these two ever disagreed the boss would be a
-- multiple of a mob that does not exist. One definition, per the rule that keeps the mode and group
-- multipliers in a single function.
local function fine_factor(power)
    if not power or power <= 1.0 then return 1.0 end
    local fine = 1.0 + 0.10 * (power - 1.0)
    if fine > 1.35 then fine = 1.35 end
    return fine
end

-- What an ORDINARY (non elite) mob in this delve ends up with for max_hp, before the difficulty mode
-- and group multipliers -- i.e. the number the boss is a multiple OF.
--
-- ⚠️⚠️ IT SCALES THE NPC AS A SIDE EFFECT and leaves it sitting at `level`. That is the same
-- measure-by-scaling trick elite_ratio already uses, and it is only safe because every caller
-- re-scales properly straight afterwards. Do not call it on a mob you are not about to re-scale.
-- ⚠️ Bare ScaleNPC is correct here: only max_hp is read, and the spell/heal proportioning that
-- M.scale_npc adds would be thrown away by the caller's own scale anyway.
-- ⚠️ Deliberately EXCLUDES the elite ratio. "Regular mob" means trash -- an authored elite is up to
-- ELITE_CAP times tougher, and sizing the boss off one of those would make it a multiple of whatever
-- happened to be standing nearby.
function M.regular_hp(npc, level, power)
    if not npc or not npc.valid then return 0 end
    npc:ScaleNPC(level)
    local hp = npc:GetMaxHP() or 0
    if hp <= 0 then return 0 end
    return math.floor(hp * fine_factor(power))
end

function M.apply(npc, eff_level, power)
    if not npc or not npc.valid then return end

    M.scale_npc(npc, eff_level)

    -- Fine trim: the part of the power ratio the rounded level did not capture. Kept small on purpose
    -- -- the level is doing the heavy lifting and this only smooths the steps between levels.
    local fine = fine_factor(power)
    if fine > 1.0 then
        local hp = npc:GetMaxHP()
        if hp and hp > 0 then
            npc:ModifyNPCStat("max_hp", tostring(math.floor(hp * fine)))
        end
    end

    -- ⚠️⚠️ RE-APPLY WHATEVER THE RUN LAYERS ON TOP OF THE RAW SCALING. ScaleNPC rewrites hp, damage and
    -- ac wholesale, so the difficulty MODE and the GROUP SIZE multiplier are wiped by every call --
    -- including the 10 second sweep in M.rescale_zone, which was quietly resetting a Hard six-man
    -- delve to Standard solo tuning mid run. aotv4_dungeon registers the hook (it owns modes and the
    -- party manifest; this module must not require it, or the two modules require each other).
    if M.post_scale_hook then M.post_scale_hook(npc) end

    -- ⚠️ AFTER the fine trim AND the mode/group hook, both of which raise max_hp the same way
    -- apply_elite does and are equally incapable of healing. scale_npc already refilled once, but
    -- everything above has moved the ceiling again since -- so this is the one that actually decides
    -- what the player walks up to. Without it a Hard six-man mob arrives part-full.
    refill(npc)

    -- Remember what this mob was worth, so the ledger can be honest about it when it dies.
    -- ⚠️ ONE stamp: the level it spawned at, which is both the difficulty readout and what the ledger
    -- pays on. Stamped at scale time so no amount of levelling or re-gearing afterwards can change
    -- what an already-killed mob was worth (section 26).
    npc:SetEntityVariable("delve_eff", tostring(eff_level))
end

-- ---------------------------------------------------------------- re-scaling mid run
-- Catches the player who enters stripped (weak mobs) and then gears up. Called from a repeating
-- per-client timer, so it only ever walks the instance that player is standing in.
--
-- ⚠️⚠️ OUT OF COMBAT ONLY. ScaleNPC rewrites max_hp and the engine refills to the new maximum, so
-- re-scaling something you are fighting would heal it mid swing -- and re-scaling DOWN would be an
-- exploit in the other direction (strip, rescale the boss soft, gear back up). A mob already pulled
-- keeps the difficulty it was pulled at.
function M.rescale_zone(c, layer_level)
    if not c or not c.valid then return end

    local eff, power = M.effective_level(layer_level, c)

    -- Deadband, per character: only churn the zone when the player has MEANINGFULLY changed.
    -- ⚠️⚠️ IT IS A PER MOB TEST, NOT A WHOLE SWEEP EARLY RETURN, AND THAT DISTINCTION IS THE WHOLE
    -- POINT. It used to `return` here when the power had not moved, which meant the sweep could
    -- never repair a mob that had NEVER been scaled -- and on the second run of a session the stored
    -- power is unchanged by definition, so the sweep did nothing at all. Combined with mobs that
    -- spawn before the player arrives (below), a level 3 delve ran with 32 creatures at level 1 and
    -- NINE still at their native Dragons of Norrath level of 65-70, in the same room.
    -- ⚠️ An unstamped mob is ALWAYS scaled, whatever the deadband says. The deadband exists to stop
    -- churn, not to let a mob escape scaling entirely.
    local key    = "delve_pow_" .. c:CharacterID()
    local last   = tonumber(eq.get_data(key))
    local moved  = (not last) or math.abs(power - last) >= M.RESCALE_DELTA
    eq.set_data(key, tostring(power))

    local n = 0
    local list = eq.get_entity_list():GetNPCList()
    for npc in list.entries do
        -- ⚠️ GetOwnerID, not just IsPet: it also catches charmed mobs, swarm/temporary pets and
        -- mercenaries. This sweep already excluded pets while `M.on_npc_spawn` did NOT, and that
        -- asymmetry is what let every pet get scaled down and lose its regen the moment it was
        -- summoned -- the sweep was never going to repair something it deliberately skips. Keep the
        -- two tests identical.
        -- ⚠️⚠️ NEVER SWEEP OUR OWN NPCs. This is the same 2000000+ divider elite_ratio uses, and it
        -- has to be here for the same reason: everything AoTv4 adds is scaled by whatever spawned it,
        -- against rules this sweep knows nothing about.
        -- The warden (2000301) was the casualty. M.on_npc_spawn already excluded it, but this sweep
        -- did not -- so ten seconds after a warden spawned, M.apply rescaled it to the TRASH level
        -- `eff` and rewrote hp, damage and level wholesale, throwing away the boss hp multiple, the
        -- boss damage multiplier and the +BOSS_LVL_BONUS. Any warden not engaged within ten seconds
        -- became a trash mob wearing a boss name, and it looked exactly like the hp multiplier "not
        -- working" -- which is how it survived several retunes of that multiplier.
        -- ⚠️ It also caught the chest (2000300) and the class aura npcs (2000100-2000115), neither of
        -- which has any business being scaled.
        if npc and npc.valid and not npc:IsEngaged()
           and (npc:GetNPCTypeID() or 0) < 2000000
           and not npc:IsPet() and (npc:GetOwnerID() or 0) == 0 then
            -- never stamped => never scaled => still a native DoN mob, repair it regardless
            local stamped = tonumber(npc:GetEntityVariable("delve_eff"))
            if moved or not stamped then
                M.apply(npc, eff, power)
                n = n + 1
            end
        end
    end

    if n > 0 then
        c:Message(MT.Yellow, string.format(
            "The delve shifts to match you. (power %.2f, creatures now level %d, %d rescaled)",
            power, eff, n))
    end
end

-- ---------------------------------------------------------------- the difficulty ledger
-- What the run is actually WORTH. Recorded per kill from the mob's own scaled level, which is why no
-- amount of changing gear afterwards can alter it.
--   delve_kills_<charid>  kills counted
--   delve_effsum_<charid> sum of effective levels killed
--   delve_effmin_<charid> weakest thing killed
--   delve_effmax_<charid> strongest thing killed
local function lkey(c, what) return "delve_" .. what .. "_" .. c:CharacterID() end

function M.reset_ledger(c)
    if not c or not c.valid then return end
    for _, k in ipairs({ "kills", "effsum", "effmin", "effmax", "score", "named", "bands" }) do
        eq.delete_data(lkey(c, k))
    end
    eq.delete_data("delve_pow_" .. c:CharacterID())
end

-- ---------------------------------------------------------------- what one kill is WORTH
-- ⚠️ QUADRATIC, not linear. A level 70 creature is not 1.4x a level 50 one, it is several times the
-- fight, so a linear score would make grinding a soft layer for volume beat clearing a hard one --
-- exactly the behaviour the whole scaling system exists to discourage.
--   eff 1 -> 1    eff 10 -> 100    eff 25 -> 625    eff 50 -> 2500    eff 70 -> 4900
--
-- ⚠️⚠️ THE VALUE IS `eff * eff`, WITH NO DIVISOR AND NO MINIMUM CLAMP -- and restoring either one
-- breaks the ladder in a way that is invisible from a single run (fixed 2026-07-30). It used to be
--     local v = math.floor((eff * eff) / 25); if v < 1 then v = 1 end
-- and integer truncation meant every eff from 1 to 7 produced floor(1/25 .. 49/25) = 0, which the
-- clamp then lifted to 1. So THE FIRST SEVEN RUNGS ALL PAID EXACTLY THE SAME, the quadratic did not
-- engage until eff 8, and a rung 1 clear was worth as much as a rung 7 one. Farming the bottom was
-- the efficient play over precisely the stretch where a new character actually is.
-- Dividing by 25 bought nothing but smaller numbers, and it cost the whole low end to get them.
--
-- ⚠️ eff is already floored at 1 by M.effective_level, so no clamp is needed -- but the guard below
-- stays because record_kill can fall back to npc:GetLevel() for a mob that was never scaled (a
-- summoned add, a pet), and a squared NEGATIVE would come out POSITIVE and score well.
--
-- ⚠️⚠️ THIS FUNCTION AND THE SIGIL CURVE ARE ONE CALIBRATION, NOT TWO. The Delver's Sigil's
-- required_amount ladder (custom/sql/aotv4_delve_charm.sql) is derived FROM this formula --
-- 24000*n^2, which is ten runs per charm level given a ~41 kill run at rung 7n. Change the scoring
-- here and that SQL is wrong until it is recomputed; the derivation is written out in that file.
M.NAMED_MULT = 3   -- a named is a real fight; worth several trash mobs

-- "Named" by the same heuristic §17c and quest_difficulty.pl use: EQ trash is lowercase ("a bat",
-- "an orc pawn") and named mobs are proper nouns. A leading # is a spawn-name marker.
local function is_named(npc)
    local n = npc:GetCleanName()
    if not n or n == "" then return false end
    local first = n:sub(1, 1)
    return first == "#" or first:match("%u") ~= nil
end

function M.kill_value(eff, named)
    if not eff or eff < 1 then eff = 1 end
    local v = eff * eff
    if named then v = v * M.NAMED_MULT end
    return v
end

-- ⚠️ `layer_level` is the rung the run is on, passed in rather than looked up, and it is only used
-- as the fallback below. Callers must supply it; without it an unscaled kill falls back to 1.
function M.record_kill(c, npc, layer_level)
    if not c or not c.valid or not npc or not npc.valid then return end

    -- The mob's own stamp, set when it was scaled.
    -- ⚠️⚠️ THE FALLBACK IS THE LAYER LEVEL, NEVER `npc:GetLevel()`. These are Dragons of Norrath
    -- zones: an unscaled mob is sitting at its NATIVE level of 60-70, so falling back to its own
    -- level scores it as end-game content. On 2026-07-30 nine such kills in a LEVEL 3 delve were
    -- worth 4,900 each (14,700 for a named) and produced a run score of 59,622 -- enough to evolve
    -- the sigil outright, from a dungeon whose other 32 creatures were level 1.
    -- ⚠️ The stamp being missing means the scaling failed, and the score must not be the place that
    -- failure gets paid out. Scoring it as the rung is the conservative answer: a real mob was
    -- killed, so it is worth something, but never more than the dungeon is nominally worth.
    local eff = tonumber(npc:GetEntityVariable("delve_eff")) or layer_level or 1

    local kills = (tonumber(eq.get_data(lkey(c, "kills"))) or 0) + 1
    local sum   = (tonumber(eq.get_data(lkey(c, "effsum"))) or 0) + eff
    eq.set_data(lkey(c, "kills"),  tostring(kills))
    eq.set_data(lkey(c, "effsum"), tostring(sum))

    local lo = tonumber(eq.get_data(lkey(c, "effmin")))
    local hi = tonumber(eq.get_data(lkey(c, "effmax")))
    if not lo or eff < lo then eq.set_data(lkey(c, "effmin"), tostring(eff)) end
    if not hi or eff > hi then eq.set_data(lkey(c, "effmax"), tostring(eff)) end

    -- Running score.
    local named = is_named(npc)
    local score = (tonumber(eq.get_data(lkey(c, "score"))) or 0) + M.kill_value(eff, named)
    eq.set_data(lkey(c, "score"), tostring(score))
    if named then
        eq.set_data(lkey(c, "named"), tostring((tonumber(eq.get_data(lkey(c, "named"))) or 0) + 1))
    end

    -- Band histogram, in fives, so the run can be shown as a shape rather than one average.
    -- Stored as "45-12,50-20,55-15". ⚠️ Bands use '-' internally because '|' and '^' are the wire
    -- separators and ',' separates the bands themselves.
    local band = eff - (eff % 5)
    local raw  = eq.get_data(lkey(c, "bands")) or ""
    local out, found = {}, false
    for b, n in raw:gmatch("(%d+)%-(%d+)") do
        b, n = tonumber(b), tonumber(n)
        if b == band then n = n + 1; found = true end
        out[#out + 1] = b .. "-" .. n
    end
    if not found then out[#out + 1] = band .. "-1" end
    table.sort(out, function(x, y)
        return tonumber(x:match("^(%d+)")) < tonumber(y:match("^(%d+)"))
    end)
    eq.set_data(lkey(c, "bands"), table.concat(out, ","))
end

-- The run's difficulty, for rewards later. `avg` is the number that matters; `lo`/`hi` are the SWING,
-- which is the trend a gear-swapper leaves behind: clearing naked and gearing up at the end shows as
-- a low average with a high maximum, and it is the average that pays.
function M.ledger(c)
    if not c or not c.valid then return nil end
    local kills = tonumber(eq.get_data(lkey(c, "kills"))) or 0
    if kills <= 0 then
        return { kills = 0, avg = 0, lo = 0, hi = 0, score = 0, named = 0, bands = "" }
    end
    local sum = tonumber(eq.get_data(lkey(c, "effsum"))) or 0
    return {
        kills = kills,
        avg   = sum / kills,
        lo    = tonumber(eq.get_data(lkey(c, "effmin"))) or 0,
        hi    = tonumber(eq.get_data(lkey(c, "effmax"))) or 0,
        score = tonumber(eq.get_data(lkey(c, "score"))) or 0,
        named = tonumber(eq.get_data(lkey(c, "named"))) or 0,
        bands = eq.get_data(lkey(c, "bands")) or "",
    }
end

return M
