-- AoTv4 reforge -- changing race or class, at level 1 only.
-- =================================================================================================
-- Backs the Reforger NPC in Resplendent. The roguelite resets a character to level 1 on every death,
-- so "level 1 only" is not a narrow window -- it is the start of every run, which makes this a
-- between-runs decision rather than a mid-run escape.
--
-- ⚠️⚠️ ONLY 112 OF THE 256 CLASS/RACE PAIRS ARE LEGAL. `char_create_combinations` is what character
-- creation validates against, and it is not a full grid -- there is no Ogre Paladin, no Troll Wizard.
-- Setting an illegal pair produces a character the client has no art or skill table for, so every
-- change here is checked against the same set creation uses.
--
-- ⚠️ Lua cannot query the database, so the table below is a COPY of char_create_combinations, grouped
-- race -> classes. Regenerate it if that table ever changes:
--     SELECT CONCAT('    [', race, '] = {', GROUP_CONCAT(DISTINCT class ORDER BY class), '},')
--     FROM char_create_combinations GROUP BY race ORDER BY race;
--
-- ⚠️⚠️ A CHANGE FORCES A RELOG, AND HAS TO. SetBaseClass is `m_pp.class_ = i` and nothing more -- it
-- writes the PROFILE, not the live Mob, so until the character is rebuilt from that profile the
-- server still treats them as their old class for skills, spell gems and the client's own art. Save
-- then Kick is the only clean way to make the two agree.

local M = {}

-- race id -> classes that race may be
M.LEGAL = {
    [1]   = {1,2,3,4,5,6,7,8,9,11,12,13,14},   -- Human
    [2]   = {1,9,10,15,16},                    -- Barbarian
    [3]   = {2,3,5,11,12,13,14},               -- Erudite
    [4]   = {1,4,6,8,9},                       -- Wood Elf
    [5]   = {2,3,12,13,14},                    -- High Elf
    [6]   = {1,2,5,9,11,12,13,14},             -- Dark Elf
    [7]   = {1,3,4,6,8,9},                     -- Half Elf
    [8]   = {1,2,3,9,16},                      -- Dwarf
    [9]   = {1,5,10,15,16},                    -- Troll
    [10]  = {1,5,10,15,16},                    -- Ogre
    [11]  = {1,2,3,4,6,9},                     -- Halfling
    [12]  = {1,2,3,5,9,11,12,13,14},           -- Gnome
    [128] = {1,5,7,10,11,15},                  -- Iksar
    [130] = {1,8,9,10,15,16},                  -- Vah Shir
    [330] = {1,2,3,5,9,10,11,12},              -- Froglok
    [522] = {1,2,3,4,5,6,7,8,9,11,12,13,14},   -- Drakkin
}

M.RACE_NAME = {
    [1]="Human", [2]="Barbarian", [3]="Erudite", [4]="Wood Elf", [5]="High Elf", [6]="Dark Elf",
    [7]="Half Elf", [8]="Dwarf", [9]="Troll", [10]="Ogre", [11]="Halfling", [12]="Gnome",
    [128]="Iksar", [130]="Vah Shir", [330]="Froglok", [522]="Drakkin",
}

M.CLASS_NAME = {
    [1]="Warrior", [2]="Cleric", [3]="Paladin", [4]="Ranger", [5]="Shadow Knight", [6]="Druid",
    [7]="Monk", [8]="Bard", [9]="Rogue", [10]="Shaman", [11]="Necromancer", [12]="Wizard",
    [13]="Magician", [14]="Enchanter", [15]="Beastlord", [16]="Berserker",
}

-- ⚠️ Ordered list, because M.LEGAL and M.RACE_NAME are sparse (128/130/330/522) and `ipairs` would
-- stop at 13. Anything iterating races must use this.
M.RACE_ORDER = {1,2,3,4,5,6,7,8,9,10,11,12,128,130,330,522}

function M.legal(race, class)
    -- AoTv4: EVERY race/class combo is allowed. Was restricted to the 112 native-legal pairs in
    -- M.LEGAL; now any known race can be any class (1-16). Illegal-by-Live combos may lack polished
    -- client art, but the server sets race+class and the client renders it.
    return (M.RACE_NAME[race] ~= nil) and class >= 1 and class <= 16
end

-- ---------------------------------------------------------------- the gate
-- Returns true if the character may reforge right now, else false plus the reason.
function M.can_reforge(c)
    if (c:GetLevel() or 0) > 1 then
        return false, "You must be at the beginning of a run. Fall, and return to me at the start."
    end
    return true
end

-- ---------------------------------------------------------------- applying a change
-- ⚠️⚠️ VALIDATE THE RESULTING PAIR, NOT THE ONE FIELD BEING CHANGED. Changing race alone can strand a
-- legal class in an illegal body -- a Gnome Necromancer who becomes a Barbarian would be a Barbarian
-- Necromancer, which creation would never allow. Both paths below check the pair they would produce.
function M.set_race(c, race)
    local ok, why = M.can_reforge(c)
    if not ok then c:Message(13, why) return false end

    local name = M.RACE_NAME[race]
    if not name then c:Message(13, "I do not know that form.") return false end
    if c:GetRace() == race then c:Message(15, "You already wear that form.") return false end

    local class = c:GetClass()
    if not M.legal(race, class) then
        c:Message(13, string.format("A %s cannot be a %s. Change your calling first.",
            name, M.CLASS_NAME[class] or "?"))
        return false
    end

    c:SetBaseRace(race)
    c:SetRace(race)          -- the visual, so the change is not invisible until the relog
    M.finish(c, string.format("You are remade as a %s.", name))
    return true
end

function M.set_class(c, class)
    local ok, why = M.can_reforge(c)
    if not ok then c:Message(13, why) return false end

    local name = M.CLASS_NAME[class]
    if not name then c:Message(13, "I do not know that calling.") return false end
    if c:GetClass() == class then c:Message(15, "That is already your calling.") return false end

    local race = c:GetRace()
    if not M.legal(race, class) then
        c:Message(13, string.format("A %s cannot be a %s. Change your form first.",
            M.RACE_NAME[race] or "?", name))
        return false
    end

    c:SetBaseClass(class)
    M.finish(c, string.format("Your calling is now %s.", name))
    return true
end

-- ⚠️⚠️ SAVE THEN KICK. SetBaseClass/SetBaseRace write the PROFILE only; the live character keeps its
-- old class for skills, spell gems and art until it is rebuilt from that profile. Without the kick
-- the player looks changed, is not, and the mismatch persists until they happen to relog -- at which
-- point their skills silently change under them. Forcing it makes the change atomic from the
-- player's point of view.
-- ⚠️⚠️ TWO THINGS DO NOT FOLLOW A RACE/CLASS CHANGE ON THEIR OWN, AND BOTH WERE REPORTED IN PLAY:
-- "ogre zerker stats as an iksar monk ... also has taunt bash and kick out of the pool at level 1".
--
-- 1. BASE STATS. SetBaseRace/SetBaseClass write m_pp.race / m_pp.class_ and nothing else. The seven
--    base stats are written ONCE at creation, by WORLD, out of char_create_combinations ->
--    char_create_point_allocations -- so a reforge kept the old body's numbers. An Ogre Berserker
--    starts STR 140 / STA 127 and an Iksar Monk STR 75 / STA 75.
--    ⚠️ Racial PASSIVES *do* follow the change (Iksar AC and regen key off GetRace() when they are
--    calculated), which is exactly why this looked so odd: the new race's innates on the old race's
--    stats. Nothing was wrong with the passives.
--
-- 2. THE PREVIOUS CLASS'S COMBAT ABILITIES. global_player.grant_native_combat_skills GRANTS a class
--    its natives and never revokes anything -- by design, since it also runs on every connect and
--    must not undo a picker reward. So an ex-Warrior kept Taunt, Bash and Kick after becoming a Monk,
--    which reads as free rewards out of the level-up pool at level 1.
--    ⚠️⚠️ SAFE ONLY BECAUSE REFORGING IS LEVEL 1 ONLY (M.can_reforge). At level 1 nothing in
--    skill_pool has been EARNED yet, so clearing all twelve cannot destroy a picker reward -- the new
--    class's natives are re-granted on the forced relog. Do NOT reuse this if the level gate moves.
--
-- ⚠️ Both run in finish() rather than in set_race/set_class, so neither path can forget one -- and
-- both must happen BEFORE Save(1), or the profile is written with the stale values and the kick
-- makes them permanent.
local skills = require("skill_pool")

-- ⚠️⚠️ THE SAVE AND THE KICK MUST HAPPEN NO MATTER WHAT ELSE FAILS. A reforge that does not log you
-- out does NOTHING visible: SetBaseRace/SetBaseClass write the profile only, and the live character
-- keeps its old class for skills, spell gems and art until it is rebuilt from that profile on login.
-- So if anything above the kick throws, the player is left looking unchanged and reports the whole
-- feature as broken -- the race/class change is actually sitting in memory, unsaved and unapplied.
--
-- ⚠️ The two tidy-up steps are therefore wrapped in pcall. They matter, but neither is worth losing
-- the reforge over: the stat rebase can be re-run by reforging again, and stale combat abilities are
-- cosmetic next to a change that silently did not happen. A failure is logged rather than swallowed.
-- 📌 Ordered rebase -> skills -> save -> kick. Both writes must land BEFORE Save(1) or they are not
-- persisted, and the kick makes whatever was saved permanent.
-- ---------------------------------------------------------------- the starter weapon
-- ⚠️⚠️ THE STARTER WEAPON MUST MATCH THE CLASS, AND IT USED TO BE A HARDCODED SHORT SWORD.
-- global_player.event_death_complete handed out 9998 to everybody. Reported from play: a Ranger who
-- reforged to Monk and then to Berserker kept the short sword the whole way, and neither class has
-- any 1H Slashing skill at all -- so the "starter" weapon was one they could never train.
-- Confirmed against skill_caps rather than assumed:
--     Monk (7)       no 1H Slashing
--     Berserker (16) no 1H Slashing AND no 1H Blunt -- only 1H Pierce, 2H Slash, Hand to Hand
--
-- ⚠️⚠️ EVERY ID HERE MUST BE ONE ABSOR ACCEPTS, or the change quietly breaks the tutorial. He takes
-- exactly four (tutorialb/Absor.pl): 9997 Dagger (1HP), 9998 Short Sword (1HS), 9999 Club (1HB) and
-- 55623 Dull Axe (2HS), handing each back sharpened. Picking a weapon outside that set would leave
-- the class better armed and the tutorial uncompletable.
-- ⚠️ None of the four has gear-tier rows, so they stay BASE and the hand-in matches. Do not add a
-- weapon that would be upgraded to Hallowed/Mythic by the crafting or loot paths.
--
-- ⚠️ ROGUE GETS THE DAGGER SPECIFICALLY: Backstab refuses to fire on anything that is not
-- ItemType1HPiercing (zone/special_attacks.cpp:793), so a Rogue holding the short sword cannot use
-- their signature ability at all. That is a mechanical requirement, not flavour.
-- 📌 Rule used for the rest: prefer 1H Slashing, else 1H Blunt, else 2H Slashing -- the best weapon
-- the class actually has a skill cap for.
M.STARTER_WEAPON = {
    [1]  = 9998,   -- Warrior       1HS
    [2]  = 9999,   -- Cleric        1HB (no 1HS)
    [3]  = 9998,   -- Paladin       1HS
    [4]  = 9998,   -- Ranger        1HS
    [5]  = 9998,   -- Shadowknight  1HS
    [6]  = 9998,   -- Druid         1HS
    [7]  = 9999,   -- Monk          1HB (no 1HS)
    [8]  = 9998,   -- Bard          1HS
    [9]  = 9997,   -- Rogue         1HP -- required by Backstab
    [10] = 9999,   -- Shaman        1HB (no 1HS)
    [11] = 9999,   -- Necromancer   1HB (no 1HS)
    [12] = 9999,   -- Wizard        1HB (no 1HS)
    [13] = 9999,   -- Magician      1HB (no 1HS)
    [14] = 9999,   -- Enchanter     1HB (no 1HS)
    [15] = 9999,   -- Beastlord     1HB (no 1HS)
    [16] = 55623,  -- Berserker     2HS -- the only one of the four it can train
}

-- The set of ids that ARE starter weapons, so a reforge can tell "the thing we gave you" apart from
-- a weapon the player actually earned. ⚠️ Derived from the table above rather than written twice.
M.IS_STARTER = {}
for _, id in pairs(M.STARTER_WEAPON) do M.IS_STARTER[id] = true end

function M.starter_weapon(class)
    return M.STARTER_WEAPON[class or 0] or 9998
end

-- Swap the starter weapon to match the new class. Called from M.finish, i.e. on any reforge.
-- ⚠️⚠️ ONLY TOUCHES A STARTER WEAPON. If the player is holding anything else in Primary it is left
-- completely alone -- a reforge must never destroy gear somebody earned, and at level 1 they may
-- still be carrying something handed to them by a quest.
-- ⚠️ Primary is slot 13. GetItemIDAt returns INVALID_ID (-1) for an empty slot, NOT 0 -- test `> 0`.
function M.fix_starter_weapon(c)
    if not c or not c.valid then return end

    local want = M.starter_weapon(c:GetClass())
    local held = c:GetItemIDAt(13) or -1

    if held > 0 and not M.IS_STARTER[held] then return end   -- earned weapon: never touch it
    if held == want then return end                          -- already correct

    if held > 0 then
        c:DeleteItemInInventory(13, 0, true)
    end
    c:SummonItem(want)
end

function M.finish(c, message)
    -- ⚠️ BEFORE the Save/Kick below, so the swap is part of the same atomic change the player sees.
    local ok_wpn, err_wpn = pcall(function() M.fix_starter_weapon(c) end)
    if not ok_wpn then
        eq.debug("aotv4_reforge: starter weapon swap failed: " .. tostring(err_wpn))
    end

    -- rebase STR/STA/DEX/AGI/INT/WIS/CHA on the NEW race+class
    local ok_stats, err_stats = pcall(function() c:AoTv4ApplyCreationStats() end)
    if not ok_stats then
        eq.debug("aotv4_reforge: stat rebase failed: " .. tostring(err_stats))
    end

    -- drop the old class's natives; the new class re-grants its own on the forced relog
    local ok_skills, err_skills = pcall(function()
        for id in pairs(skills.SKILLS) do
            if (c:GetRawSkill(id) or 0) > 0 then c:SetSkill(id, 0) end
        end
    end)
    if not ok_skills then
        eq.debug("aotv4_reforge: combat skill clear failed: " .. tostring(err_skills))
    end

    -- ⚠️⚠️ THE OLD CLASS'S DISCIPLINES DO NOT GO AWAY ON THEIR OWN, AND NOTHING ELSE REMOVES THEM.
    -- Class abilities live in `character_disciplines`, not the spellbook, so neither the stat rebase
    -- above nor the combat-skill clear touches them -- and aotv4_class_abilities.M.grant only ever
    -- ADDS. A Bard who reforged to Cleric kept Discordant Strike on the bar and could still press it.
    -- ⚠️ Scoped to the 44700-44747 band and derived from `spell_id(class, tier)` rather than a
    -- hardcoded range, so if the band ever moves this follows it instead of silently missing.
    -- ⚠️ EVERY class is cleared, not just the one being left. GetClass() still returns the OLD class
    -- here (SetBaseClass writes m_pp.class_ and nothing else -- section 34), which is fragile to
    -- depend on; clearing the whole band needs no such assumption and cannot leave a stray behind
    -- from an earlier reforge. M.grant re-adds the NEW class's three on the forced relog below.
    local ok_disc, err_disc = pcall(function()
        local ab = require("aotv4_class_abilities")
        for class = 1, 16 do
            for tier = 1, 3 do
                local slot = c:GetDiscSlotBySpellID(ab.spell_id(class, tier))
                if slot and slot >= 0 then c:UntrainDisc(slot) end
            end
        end
    end)
    if not ok_disc then
        eq.debug("aotv4_reforge: discipline clear failed: " .. tostring(err_disc))
    end

    c:Message(15, message)
    c:Message(15, "The world reshapes around you. Return in a moment.")
    c:Save(1)
    c:Kick()
end

-- ---------------------------------------------------------------- say routing
function M.handle_say(e)
    local c = e.self
    local id = e.message:match("^reforgerace (%d+)$")
    if id then M.set_race(c, tonumber(id)) return true end
    id = e.message:match("^reforgeclass (%d+)$")
    if id then M.set_class(c, tonumber(id)) return true end
    return false
end

return M
