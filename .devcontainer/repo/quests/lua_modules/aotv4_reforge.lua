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
function M.finish(c, message)
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
