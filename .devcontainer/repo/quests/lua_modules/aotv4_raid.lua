-- aotv4_raid.lua -- raid encounters, delivered through the delve window.
--
-- A raid is the BOSS ALONE, in its own native zone, in a private instance, entered by a GROUP.
-- No trash, no wandering adds, nothing but the encounter. See RAID_INSTANCES_DESIGN.md.
--
-- ⚠️⚠️ THE EMPTY ZONE IS FREE, AND IT IS THE WHOLE MECHANISM. Zone::LoadSpawnGroups filters spawn
-- points with `(version = N OR version = -1)`, and every spawn2 row in these zones is version 0 with
-- not one at -1. So an instance at ANY non-zero version comes up completely empty -- geometry, doors
-- and zone points intact, zero NPCs -- and we place the boss ourselves.
--   * No data edits. These are REAL WORLD ZONES; stripping their spawn tables would empty them for
--     everyone, permanently. Same reasoning section 26 gives for clearing delve loot at spawn rather
--     than in lootdrop_entries.
--   * No race to lose. Section 26 records that an instance POPULATES BEFORE the player finishes
--     zoning in, which is what broke the delve's first scaling pass. A depop-on-entry would inherit
--     that bug exactly. Nothing spawning in the first place cannot be raced.
--
-- ⚠️⚠️ THIS IS THE OPPOSITE OF THE DELVE RULE AND BOTH ARE RIGHT. Section 24 warns in capitals that a
-- delve must NEVER use version 0, because for a DoN zone version 0 is the OPEN WORLD spawn set and
-- the non-zero versions are the authored mission layouts. Here the intent is inverted: we want
-- nothing at all, so we want a version nobody authored. Do not "fix" one to match the other.
--
-- ⚠️ BEFORE ADDING A ZONE, VERIFY THE INVARIANT FOR IT:
--     SELECT version, COUNT(*) FROM spawn2 WHERE zone = '<zone>' GROUP BY version;
-- A zone with version -1 rows, or with authored alternate versions, does NOT come up empty.

-- ⚠️ Defined locally, as every other AoTv4 module does (aotv4_gamble.lua) -- there is no shared
-- message-type module to require.
local MT = { Yellow = 15, Red = 13, Green = 4, LightBlue = 18 }

local M = {}

-- ------------------------------------------------------------------ tuning
-- ⚠️ Instance version band. 900+ so a raid instance can never collide with a real authored version
-- if one is ever added to these zones, and so `version` alone identifies a raid instance in the DB.
M.VERSION_BASE    = 900

-- ⚠️⚠️ THE WINDOW KEY. The delve window sends back the LEVEL of the selected row, so a raid row needs
-- a level that cannot collide with a delve rung (1-70). 100 + index. aotv4_dungeon.M.enter routes
-- anything above M.RAID_BASE here.
-- 📌 This is what lets raids ship with NO CLIENT CHANGE. The dll reads the leading count of DUNGDATA
-- but does not use it -- it counts rows itself -- so appending rows is safe on an un-rebuilt dll.
M.RAID_BASE       = 100

M.DURATION_SECS   = 10800   -- 3h hard ceiling on an instance, regardless of occupancy

-- ⚠️⚠️ THE EXPEDITION SWITCH. true creates a real dynamic zone, so the raid appears in the client's
-- own expedition window (Ctrl+Z) with a compass and an engine-managed safe return; false uses a plain
-- instance, which is what the delve does and what this module did before 2026-08-30.
-- ⚠️ Turning it OFF is the one-line fallback if entry ever misbehaves. Nothing else changes: a DZ
-- owns an ordinary instance, so every path after creation is identical either way.
M.USE_EXPEDITION  = true
-- ⚠️⚠️ NOT A "let the client catch up" PAUSE -- it is the window in which world receives the DZ's
-- member list. Below about a second the reap this exists to avoid can still fire; far above it the
-- Enter button just feels broken. See the create block in M.enter for the mechanism.
M.ENTER_DELAY_SECS = 2
M.EMPTY_CLOSE_SECS= 300     -- ...closes 5 minutes after the last member leaves, if the boss is alive
M.DEAD_CLOSE_SECS = 300     -- ...or 5 minutes after the boss dies, occupied or not -- see M.on_boss_death
-- ⚠️⚠️ STAMPED ON THE KILL, NOT ON ENTRY -- so a WIPE COSTS YOU NOTHING BUT THE RUN. Locking on
-- entry is simpler and has no edge cases, and it was rejected: on this server a wipe already costs
-- the roguelite death -- level 1, spellbook wiped, carried gear destroyed -- and taking the day's
-- attempt as well would make one bad pull the most expensive event in the game. Live EQ saves you on
-- the kill for the same reason. Set to 0 to disable the lockout entirely.
M.LOCKOUT_SECS    = 86400   -- 24 hours, per character, per encounter

-- ⚠️ How often the close sweep runs, per client. Deliberately coarse: both close conditions are
-- measured in minutes, so a tighter tick would only cost work. It early-outs on the instance id, so
-- for every player NOT in a raid this is two comparisons.
M.SWEEP_SECS      = 30
-- ⚠️ Fast enough that a phase lands while the boss is still visibly at that health, slow enough
-- that six raiders polling it is nothing. Only ever armed inside a raid (see M.on_enter_zone).
M.BOSS_TICK_SECS  = 3

-- ------------------------------------------------------------------ the registry
-- ⚠️⚠️ RESOLVE THE BOSS BY NPC ID, NEVER BY NAME. The design documents and the database disagree:
-- the docs say "Phinigel Autotropus" (DB: Phinigel_Autropos) and "Velkator" (DB: Velketor -- and that
-- doc's own marquee text spells it the DB way). An id cannot be misspelt.
--
-- ⚠️ `entry` is where the RAID PARTY lands; `boss` is where the boss is placed. Keep them apart --
-- landing on top of the boss means the fight starts before anyone has finished loading.
-- ⚠️ `level` is what the boss is scaled TO. Phinigel and Velketor are natively 35 against a level 30
-- cap, which is close enough to tune; MAYONG IS NATIVELY 85 and has no spawn2 row anywhere, so his
-- coordinates are authored rather than taken from the world.
M.ENCOUNTERS = {
    {
        id = "phinigel", name = "Phinigel Autropos", hub = "Kelethin",
        -- ⚠️ TIER IS EXPLICIT, NOT THE TABLE INDEX. The index is also the wire level (RAID_BASE + i)
        -- and the lockout key, so it is not free to reorder -- and a tier that is really "position in
        -- this table" is a tier nobody can change without moving everyone's lockouts.
        tier = 1,
        blurb = "TIER 1, the teaching fight. A flooded vertical cavern -- the party swims DOWN to "
             .. "him from the entry point. He casts as a wizard and summons, and is immune to mez, "
             .. "charm and fear, but slow lands. At 75, 50 and 25 percent he calls a swarm of "
             .. "piranha: kill them or peel them, they are fodder. 120k health, hits to 350.",
        spells = 2,   -- Default Wizard List; his own list 1346 is EMPTY (0 entries)
        hp = 120000, delay = 25, hit_lo =  90, hit_hi = 350,
        -- entry raid: ~6.7 min for a duo at 300 dps, ~2.2 min for a full group.
        -- ⚠️⚠️ 64 IS KEDGE KEEP. This read **66** until 2026-08-30, which is `gukbottom` (The Ruins of
        -- Old Guk) -- see M.zone_id_of for the full failure and why nothing reported it.
        zone = "kedge", zoneid = 64, npc = 64001, level = 30,
        -- ⚠️⚠️ THE ENTRY IS SWAPPED FROM ITS /loc; THE BOSS IS NOT. `/loc` prints **Y, X, Z** while
        -- spawn2 and MovePCInstance take **x, y, z** (§11). The entry was walked in game and swapped:
        --     entry  /loc -33.73, 14.04, -80.84  ->  x 14.04, y -33.73, z -80.84
        -- The boss point is Phinigel's OWN spawn2 row read straight out of the database, which is
        -- already in engine order -- so it must NOT be swapped as well. Two coordinates in one row
        -- from two different conventions is exactly the kind of thing that looks consistent and is not.
        -- 📌 The entry sits ~195 units ABOVE the boss (z -80.84 against -275.8). That is expected in
        -- Kedge Keep, which is a flooded vertical cavern -- the party swims down to him.
        entry = { x =  14.04, y = -33.73, z =  -80.84, h = 0 },
        boss  = { x = 111.50, y =  37.00, z = -275.80, h = 179.5 },
    },
    {
        id = "mayong", name = "Mayong Mistmoore", hub = "Kelethin",
        tier = 2,
        blurb = "TIER 2, a damage race. He never casts -- everything he has is melee, with flurry "
             .. "and triple attack, so the pressure is on the tank staying up rather than on "
             .. "dispels. No adds. At 66 and 33 percent he speeds up and does not slow down again, "
             .. "so a long fight gets worse: bring the damage. 160k health, hits to 400.",
        abilities = { 5, 6 },   -- Flurry + Triple Attack: class 1, 0 mana, cannot cast
        hp = 160000, delay = 25, hit_lo = 100, hit_hi = 400,
        -- middle rung: ~8.9 min for a duo.
        zone = "mistmoore", zoneid = 59, npc = 351118, level = 30,
        -- ⚠️⚠️ THESE ARE SWAPPED FROM THE /loc THEY WERE WALKED AT, AND THAT IS DELIBERATE. In-game
        -- `/loc` prints **Y, X, Z** while spawn2, eq.spawn2 and MovePCInstance all take **x, y, z**
        -- (§11, which records this costing real time). Pasting a raw /loc puts the boss and the entry
        -- point in completely the wrong place -- and because both readings land inside this zone's
        -- bounding box with spawn points nearby, it fails by putting them somewhere plausible rather
        -- than somewhere obviously broken.
        --     boss   /loc -21.52, 144.80, -192.78   ->  x 144.80, y  -21.52, z -192.78
        --     entry  /loc -186.78, 140.92, -192.76  ->  x 140.92, y -186.78, z -192.76
        -- 📌 Sanity check on the swap: the two points then share an X (144.8 / 140.9) and a Z, and are
        -- ~165 units apart along Y -- the same floor, one at each end, which is what was walked.
        -- ⚠️ If Mayong turns up inside a wall, swapping x and y back is the first thing to try.
        entry = { x = 140.92, y = -186.78, z = -192.76, h = 0 },
        boss  = { x = 144.80, y =  -21.52, z = -192.78, h = 256 },
    },
    {
        id = "velketor", name = "Velketor the Sorcerer", hub = "Thurgadin",
        tier = 3,
        blurb = "TIER 3, the longest fight. A full wizard list plus summon, and immune to stun and "
             .. "snare on top of mez, charm and fear -- slow still lands. At 75, 50 and 25 percent "
             .. "two crystal statues grind to life and come for you; they are tougher than "
             .. "Phinigel's swarm. 200k health, hits to 450. Velious: needs Thurgadin open.",
        -- keeps its own list 976 (Wizard_spellset_pre_pop, 63 spells) and its stock immunities
        hp = 200000, delay = 25, hit_lo = 115, hit_hi = 450,
        -- hardest, and region locked behind Velious: ~11 min for a duo.
        zone = "velketor", zoneid = 112, npc = 112025, level = 30,
        -- ⚠️ Entry swapped from its /loc (§11: /loc prints Y, X, Z); the boss point is Velketor's own
        -- spawn2 row and is already in engine order, so it is NOT swapped. Same two-conventions-in-one-row
        -- hazard as the Kedge entry above.
        --     entry  /loc 54.63, 132.13, -39.84  ->  x 132.13, y 54.63, z -39.84
        entry = { x = 132.13, y =  54.63, z = -39.84, h = 0 },
        boss  = { x = 135.00, y = 178.00, z = -53.40, h = 259.0 },
        -- ⚠️ VELKETOR'S LABYRINTH IS REGION 3 (Thurgadin) AND THE SERVER IS CLASSIC, so nobody can
        -- reach it yet. It is NOT hand-disabled for that: M.can_enter asks the region gate, so this
        -- encounter hides itself today and appears on its own the moment Velious is unlocked, with no
        -- code change. Hand-disabling it would mean remembering to come back.
        -- 📌 Its expansion gate is already open (zone.bypass_expansion_check = 1, from v37), so the
        -- region lock is genuinely the only thing standing in front of it.
    },
}

-- ⚠️⚠️ THE ZONE IS NAMED TWICE PER ENCOUNTER AND THE TWO MUST AGREE -- `zone` (short name) is what
-- eq.create_instance binds the instance to, `zoneid` is what MovePCInstance sends the player to. When
-- they disagree the instance is built for one zone and the party is sent to ANOTHER, and world's own
-- entry check (VerifyInstanceAlive -> CheckInstanceByCharID) then refuses them and calls
-- MoveCharacterToInstanceSafeReturn.
-- ⚠️⚠️ THE SYMPTOM IS A ZONE FLICKER AND NOTHING ELSE: the client starts the transfer, is bounced,
-- and lands back exactly where it stood. No error, no refusal message, and nothing wrong in either
-- zone -- so it reads as "the raid did not start" rather than as a wrong number. §24 records this
-- same landing for a MISSING membership row, which sends you looking at the assign step; here the
-- membership row was correct and present, which is what made it hard to see.
-- 📌 Phinigel shipped as 66 (`gukbottom`) against a `zone` of "kedge" (64) and was found only when
-- somebody tried to enter and noticed they were briefly sent to Lower Guk.
-- ✅ THE SHORT NAME WINS, because it is what actually created the instance -- resolving from it means
-- the two CANNOT disagree. The literal stays as a fallback for the case where the lookup fails, and a
-- mismatch is reported rather than silently corrected, so a bad literal is fixed rather than hidden.
-- Is the run recorded in the bucket still a real expedition as far as the ENGINE is concerned?
--
-- ⚠️⚠️ THIS ANSWERS "definitely gone" OR "do not know" -- NEVER "definitely alive". `GetExpedition`
-- reads the client's `m_dynamic_zone_ids`, which world fills in asynchronously (§24), so an empty
-- list means either the player quit the expedition or world simply has not replied yet, and nothing
-- here can tell those apart. Callers must only use a `false` to clean up, and only where being wrong
-- is cheap -- today that is one call site, M.enter, when the player is entering a DIFFERENT raid.
-- ⚠️ Matches on the INSTANCE ID, not merely "has an expedition": a player could hold a DZ for some
-- other feature entirely, and that must not read as this raid still being live.
-- 📌 Wrapped in pcall because Lua_Expedition is invalid rather than nil when there is no expedition,
-- and calling GetInstanceID on an invalid one is not worth trusting.
function M.run_is_live(c, run)
    if not (c and c.valid and run and run.inst) then return false end
    local ok, live = pcall(function()
        local dz = c:GetExpedition()
        return dz and dz.valid and dz:GetInstanceID() == run.inst
    end)
    -- ⚠️ An ERROR is "do not know", so it must read as LIVE -- the safe direction is to keep the run
    -- and make the player press Exit, never to silently discard it.
    if not ok then return true end
    return live and true or false
end

-- Am I standing in a raid instance right now, whatever any bucket claims?
--
-- ⚠️ The ONE question that is answerable from the world itself rather than from our own bookkeeping,
-- which is why the exit path leans on it: buckets can be lost, a zone cannot.
function M.in_raid_zone()
    local inst = eq.get_zone_instance_id()
    if not inst or inst == 0 then return false end
    local _, enc = M.encounter_of_instance(inst)
    return enc ~= nil
end

function M.zone_id_of(enc)
    if not enc then return nil end
    local resolved = eq.get_zone_id_by_name(enc.zone)
    if not resolved or resolved == 0 then return enc.zoneid end
    if enc.zoneid and enc.zoneid ~= resolved then
        eq.debug(string.format(
            "aotv4_raid: encounter '%s' has zoneid %d but zone '%s' is %d -- using %d. Fix the table.",
            tostring(enc.id), enc.zoneid, tostring(enc.zone), resolved, resolved))
    end
    return resolved
end

-- ------------------------------------------------------------------ buckets
-- ⚠️ Raid state is kept in its OWN buckets rather than sharing the delve's. They have different
-- lifetimes and different teardown rules, and sharing would mean every delve guard has to ask which
-- kind of run it is looking at.
local function bkey(c, what) return string.format("raid_%s_%d", what, c:CharacterID()) end

-- ⚠️⚠️ THE RUN CARRIES A TOKEN, AND THAT IS WHAT MAKES REJOIN SAFE. Instances:RecycleInstanceIds is
-- on and ids are handed out lowest-free above 100, so a closed raid's id is very quickly claimed by
-- something else -- most likely a delve (§43 lost real time to exactly this). A player holding a
-- stale run bucket that names only an id could therefore be sent into a STRANGER'S instance.
-- The token is stamped into both the player's run and the instance marker, and both must agree.
-- 📌 This is used INSTEAD of an existence check because there is no `is_instance_valid` binding --
-- `eq.get_instance_id` is the only one, and it answers about the zone you are standing in, which is
-- no use to somebody trying to get back in from outside.
local function set_run(c, enc_idx, inst, zone, token)
    eq.set_data(bkey(c, "run"), string.format("%d|%d|%s|%d", enc_idx, inst, zone, token))
end

function M.current_run(c)
    if not c or not c.valid then return nil end
    local v = eq.get_data(bkey(c, "run"))
    if not v or v == "" then return nil end
    local idx, inst, zone, token = v:match("^(%d+)|(%d+)|(%S+)|(%d+)$")
    if not idx then return nil end
    return { idx = tonumber(idx), inst = tonumber(inst), zone = zone, token = tonumber(token) }
end

local function clear_run(c) eq.delete_data(bkey(c, "run")) end

-- ⚠️⚠️ THE TRANSIT FLAG IS MANDATORY, AND IT IS PER MEMBER. EVENT_DISCONNECT fires on EVERY zone
-- transfer, not just camp -- client_process.cpp tests bZoning thirty lines earlier for other work and
-- then raises the event unconditionally. Section 24 records this making the Delve un-enterable for an
-- entire session: the source zone raised disconnect the instant the player was moved, the handler
-- concluded the run was over, and tore the state down while they were still on the loading screen.
-- A raid moves SEVERAL players, so it fires several times -- one flag for the leader is not enough.
-- ⚠️ The grace is enforced by COMPARING TIMESTAMPS, not by a bucket expiry -- copied from the delve
-- so both behave identically. A stale flag left by a crash mid-transit therefore stops protecting
-- after TRANSIT_GRACE rather than forever.
local TRANSIT_GRACE = 60

local function set_transit(c)   eq.set_data(bkey(c, "transit"), tostring(os.time())) end
local function clear_transit(c) eq.delete_data(bkey(c, "transit")) end

local function in_transit(c)
    local raw = eq.get_data(bkey(c, "transit"))
    if not raw or raw == "" then return false end
    local started = tonumber(raw)
    if not started then return false end
    return (os.time() - started) < TRANSIT_GRACE
end

-- ------------------------------------------------------------------ instance <-> encounter
-- ⚠️⚠️ INSTANCE IDS ARE RECYCLED. Instances:RecycleInstanceIds is on and ids are handed out
-- lowest-free above 100, so a dead raid's id gets claimed by something else -- very likely a delve.
-- Section 43 lost real time to exactly this. Keep BOTH directions and make the reverse the authority,
-- then round-trip the answer rather than trusting either alone.
local function set_instance_enc(inst, idx, token)
    eq.set_data("raid_of_" .. inst, string.format("%d|%d", idx, token))
end

-- Reads the marker WITHOUT assuming you are standing in the instance. Returns idx, encounter, token.
local function read_instance_enc(inst)
    if not inst or inst == 0 then return nil end
    local v = eq.get_data("raid_of_" .. inst)
    if not v or v == "" then return nil end
    local idx, token = v:match("^(%d+)|(%d+)$")
    idx = tonumber(idx)
    if not idx then return nil end
    return idx, M.ENCOUNTERS[idx], tonumber(token)
end

function M.encounter_of_instance(inst)
    local idx, enc, token = read_instance_enc(inst)
    if not enc then return nil end
    -- ⚠️ Round trip: the marker is only believed if the instance really is that encounter's zone. A
    -- recycled id inherits a stale marker, and without this a delve would be treated as a raid.
    if eq.get_zone_short_name() == enc.zone then return idx, enc, token end
    return nil
end

function M.encounter_for_level(level)
    local idx = (level or 0) - M.RAID_BASE
    if idx < 1 or idx > #M.ENCOUNTERS then return nil end
    return idx, M.ENCOUNTERS[idx]
end

-- ------------------------------------------------------------------ region gating
-- ⚠️⚠️ RAIDS ARE REGION GATED, AND THE GATE IS ASKED IN LUA RATHER THAN LEFT TO THE ENGINE. The
-- engine WOULD refuse the move on its own -- the region lock applies to instanced entry, which is the
-- entire reason the delve maps had to be moved into region 0 by v27 -- but letting it refuse at that
-- point is the worst possible moment: by then the instance has been created, every member assigned
-- and half of them moved. They would be evicted to their safe return and the instance would leak.
-- Asking first turns that into a clean refusal with no side effects (§24: anything that returns after
-- create_instance leaks an instance, every time it fires).
--
-- ⚠️ `CanEnterZone` is the whole gate, not just regions -- it also covers expansion, min_level,
-- min_status and flag_needed. For the three encounter zones today none of those bite (all carry
-- bypass_expansion_check = 1 and no level or flag requirement), so it reduces to the region test --
-- but using the full check means a future encounter in a gated zone is handled without noticing.
--
-- ⚠️⚠️ THE ALTERNATIVE WAS EXEMPTING INSTANCED MOVES FROM THE REGION GATE, AND IT WAS REJECTED.
-- v27 suggested that for `veksar` and it would have opened Velketor raids without granting world
-- travel -- but it makes raids the one thing region locking does not cover, which is backwards: a
-- raid is content, and content is what regions exist to gate.
function M.can_enter(c, enc)
    if not c or not c.valid or not enc then return false end
    return c:CanEnterZone(enc.zone) and true or false
end

-- ------------------------------------------------------------------ lockout
-- ⚠️⚠️ TAKEN AT ENTRY, NOT AT THE KILL -- entering spends your attempt, so a wipe locks you out just
-- as a win does. Stamped in ONE place, M.enter, for every member of the party (2026-08-30).
-- ⚠️ THIS COMMENT USED TO SAY "M.LOCKOUT_SECS is 0 for now, which means NO LOCKOUT". It is 86400 and
-- has been for some time; the line was left behind when the value was set and claimed the exact
-- opposite of the truth about a rule nobody could see working or not working from in game.
-- 📌 The bucket is the authority. `dz:AddReplayLockout` is also called at entry so the client's own
-- expedition window shows the timer, but the bucket is what M.can_enter and the Raids tab read, and
-- it is the only one that exists on the plain-instance fallback.
function M.locked_out(c, idx)
    return M.lockout_remaining(c, idx) > 0
end

-- Seconds left on this character's lockout for one encounter, or 0.
function M.lockout_remaining(c, idx)
    if M.LOCKOUT_SECS <= 0 then return 0 end
    local until_t = tonumber(eq.get_data(bkey(c, "lock" .. idx)) or "") or 0
    local left = until_t - os.time()
    return left > 0 and left or 0
end

-- "6h 12m" / "48m" / "under a minute". Shown in the window row and in the refusal, because "you are
-- locked out" without a number is the kind of message players ask about every single time.
function M.lockout_text(secs)
    if secs >= 3600 then
        return string.format("%dh %dm", math.floor(secs / 3600), math.floor((secs % 3600) / 60))
    elseif secs >= 60 then
        return string.format("%dm", math.floor(secs / 60))
    end
    return "under a minute"
end

local function stamp_lockout(c, idx)
    if M.LOCKOUT_SECS <= 0 then return end
    eq.set_data(bkey(c, "lock" .. idx), tostring(os.time() + M.LOCKOUT_SECS))
end

-- ------------------------------------------------------------------ the window feed
-- Rows appended to the delve's DUNGDATA, as `level|name|cleared`.
--
-- ⚠️⚠️ BUDGET THE BYTES. Client::Message formats into a char[4096] (section 3), and the delve already
-- sends up to 70 rows. Appending raid rows without a budget is how the Zone XP tab and the AdvLoot
-- filter list both learned to truncate SILENTLY -- the list simply ends and nothing says why.
-- 📌 The dll's own cap is DG_MAX = 96 rows, which is not the binding limit; the chat buffer is.
function M.list_rows(c, budget)
    local rows, used = {}, 0
    for i, e in ipairs(M.ENCOUNTERS) do
        -- ⚠️⚠️ A LOCKED RAID IS STILL LISTED, MARKED, AND STILL REFUSED ON ENTRY -- which is the
        -- OPPOSITE of what the delve does with a locked rung, deliberately.
        --   * The delve hides a locked layer because its ladder is 70 rungs long and unlocks one at a
        --     time; listing them all would be clutter that tells you nothing you did not know.
        --   * A raid list is short and each entry is a destination. Showing "Velketor, locked" is the
        --     same reasoning §32 used to make the tradeskill tools ACHIEVEMENT rewards rather than
        --     craftable recipes: a reward nobody is told about is not a reward. The window is the only
        --     place a player would ever learn this content exists.
        -- ⚠️ Being listed grants nothing. M.enter re-checks the region in its validate stage, so the
        -- row is an advertisement, not a key -- a modified client selecting it is still refused.
        -- ⚠️ The marker goes in the NAME because the wire format is `level|name|cleared` and has no
        -- lock field. Adding one would mean a dll change, which is the whole thing this design avoids.
        if not e.disabled then
            local open    = M.can_enter(c, e)
            local left    = M.lockout_remaining(c, i)
            local cleared = left > 0 and 1 or 0
            local label
            if not open then
                label = string.format("%s: %s  [locked]", e.hub, e.name)
            elseif left > 0 then
                -- ⚠️ The remaining time is put in the NAME for the same reason the lock marker is:
                -- the wire format is `level|name|cleared` and there is nowhere else to put it without
                -- a dll change. It also means the row updates itself every time the window is opened.
                label = string.format("%s: %s  [ready in %s]", e.hub, e.name, M.lockout_text(left))
            else
                label = string.format("%s: %s", e.hub, e.name)
            end
            local row = string.format("%d|%s|%d", M.RAID_BASE + i, label, cleared)
            if budget and used + #row + 1 > budget then
                -- ⚠️ A VISIBLE marker, never a silent stop. Section 3: a truncated list that just ends
                -- reads as "there is no more content" rather than as a bug.
                rows[#rows + 1] = string.format("%d|-- more raids not shown --|0", M.RAID_BASE + 99)
                break
            end
            used = used + #row + 1
            rows[#rows + 1] = row
        end
    end
    return rows
end

-- Rows for the Delve window's own RAIDS tab, as `level|tier|name|hub|status|blurb`.
--
-- ⚠️⚠️ THIS SUPERSEDES M.list_rows, WHICH IS KEPT ONLY FOR AN UN-REBUILT DLL. That function had to
-- cram the lock marker and the countdown INTO THE NAME, because DUNGDATA's row is `level|name|cleared`
-- and there was nowhere else to put them -- its own comments say so, and say that adding a field
-- "would mean a dll change, which is the whole thing this design avoids". The Raids tab IS that dll
-- change, so the fields become real ones and the name goes back to being just the name.
-- ⚠️ THE LEVEL IS STILL `RAID_BASE + i` AND MUST STAY SO. It is what the Enter button puts on the
-- wire, and M.enter routes anything above RAID_BASE here -- so the new tab needs no new entry path,
-- no new say command and no second copy of the gate checks.
-- ⚠️ Byte-budgeted for the same reason as list_rows: Client::Message formats into a char[4096] (§3)
-- and a blurb is far longer than a name, so this line overflows sooner than that one did.
-- ⚠️ Neither '|' nor '^' may appear in a blurb -- they are the field separators. They are authored in
-- M.ENCOUNTERS, so this is a rule for whoever edits that table, not something to escape here.
function M.window_rows(c, budget)
    local rows, used = {}, 0
    for i, e in ipairs(M.ENCOUNTERS) do
        if not e.disabled then
            -- ⚠️ A LOCKED OR UNREACHABLE RAID IS STILL LISTED -- see list_rows for why: a short list of
            -- destinations is the only place a player learns this content exists. Being listed grants
            -- nothing; M.enter re-checks the region and the lockout in its validate stage.
            local open   = M.can_enter(c, e)
            local left   = M.lockout_remaining(c, i)
            local status
            if not open then
                status = "Locked"
            elseif left > 0 then
                status = "Ready in " .. M.lockout_text(left)
            else
                status = "Ready"
            end
            local row = string.format("%d|%d|%s|%s|%s|%s",
                M.RAID_BASE + i, e.tier or i, e.name, e.hub, status, e.blurb or "")
            if budget and used + #row + 1 > budget then
                -- ⚠️ A VISIBLE marker, never a silent stop (§3).
                rows[#rows + 1] = string.format("%d|0|-- more raids not shown --|||", M.RAID_BASE + 99)
                break
            end
            used = used + #row + 1
            rows[#rows + 1] = row
        end
    end
    return rows
end

-- ------------------------------------------------------------------ entry
-- ⚠️⚠️ THE GATE ORDER IS LOAD BEARING AND IT MIRRORS THE DELVE'S DELIBERATELY (section 24):
--     validate -> snapshot the roster -> create -> assign EVERY member -> spawn -> move, leader last.
-- Anything that returns after create_instance leaks an instance, every time it fires.
function M.enter(c, level)
    if not c or not c.valid then return end

    local idx, enc = M.encounter_for_level(level)
    if not enc then
        c:Message(MT.Red, "There is no raid there.")
        return
    end
    if enc.disabled then
        c:Message(MT.Red, string.format("%s is not open yet.", enc.name))
        return
    end
    -- ⚠️ A run they are no longer standing in means they died, camped or walked out. Pressing Enter
    -- on that same row is a request to GO BACK, not to start again -- see M.rejoin.
    local existing = M.current_run(c)

    -- ⚠️⚠️ THE EXPEDITION WINDOW IS THE SOURCE OF TRUTH FOR WHETHER THE RUN STILL EXISTS. Our bucket
    -- and the DZ are two records of one fact, and they diverge: the DZ hands the player a **Quit
    -- Expedition** button this module never hears about, so quitting left the bucket claiming a run
    -- that the engine had already ended -- and every other raid was then refused with "you are already
    -- committed". Reported from play after leaving Phinigel. It is a bug this module CREATED by moving
    -- to real expeditions; with a plain instance there was no second way out.
    -- ⚠️ Only ever used to CLEAR a stale run, never to prove one is live. `m_dynamic_zone_ids` is
    -- populated ASYNCHRONOUSLY by world (§24), so a missing DZ can just mean "not told yet" -- trusting
    -- a negative anywhere else would wipe a live run and, after a death, take away the only route back
    -- to owner-stamped loot that nobody else can pick up (§31).
    -- 📌 Checked HERE and nowhere else on purpose: the player is pressing Enter on a DIFFERENT raid, so
    -- they have chosen to move on regardless. That makes a wrong answer cheap in the one place it is
    -- allowed to happen.
    if existing and existing.idx ~= idx and not M.run_is_live(c, existing) then
        clear_run(c)
        existing = nil
    end

    if existing then
        if existing.idx == idx then
            M.rejoin(c, existing)
        else
            -- ⚠️ NAME THE ENCOUNTER AND SAY WHERE THE BUTTON IS. "Use Exit first" was accurate and
            -- useless: the Exit button lives on the DUNGEONS tab (labelled "Exit Delve"), so a player
            -- standing on the Raids tab is told to press something that is not on screen -- and it
            -- does not read as applying to raids even once they find it. Reported from play.
            -- 📌 It also could not say WHICH raid they were still committed to, which matters after a
            -- death: the run is deliberately kept so they can go back for owner-stamped loot (§31).
            c:Message(MT.Red, string.format(
                "You are still committed to %s. Press Exit Raid, or Exit Delve on the Dungeons tab, "
                .. "to give it up -- or select %s and press Enter to go back to it.",
                M.ENCOUNTERS[existing.idx] and M.ENCOUNTERS[existing.idx].name or "another raid",
                M.ENCOUNTERS[existing.idx] and M.ENCOUNTERS[existing.idx].name or "it"))
        end
        return
    end
    -- ⚠️ Not while in a delve either -- moving them out from here strands that run.
    local dungeon = require("aotv4_dungeon")
    if dungeon.current_run and dungeon.current_run(c) then
        c:Message(MT.Red, "Leave your delve before starting a raid.")
        return
    end
    -- ⚠️⚠️ NOT WHILE SOMETHING IS FIGHTING YOU. The window opens anywhere, so without this "Enter" is
    -- a free escape from any losing fight -- better than Gate, because it removes you from the zone
    -- entirely, breaks every hate list at once, costs no reagent and has no cast time to interrupt.
    -- GetAggroCount, not IsEngaged: it stays true while something chases you after you stop fighting,
    -- which is exactly when the escape is worth most.
    if (c:GetAggroCount() or 0) > 0 then
        c:Message(MT.Red, "You cannot start a raid while something is fighting you.")
        return
    end
    -- ⚠️ In the VALIDATE stage, before create_instance -- see the note on M.can_enter.
    if not M.can_enter(c, enc) then
        c:Message(MT.Red, string.format(
            "You have not unlocked the region %s lies in.", enc.name))
        return
    end
    local left = M.lockout_remaining(c, idx)
    if left > 0 then
        c:Message(MT.Red, string.format(
            "You have already faced %s today. Ready in %s.", enc.name, M.lockout_text(left)))
        return
    end

    -- ---------------------------------------------------------------- roster
    -- ⚠️⚠️ TAKEN HERE AND NEVER AGAIN. Everyone grouped with the leader AND standing in this zone
    -- right now is in; anyone invited later is not, and anyone in another zone is not.
    --   * Zone-local is a HARD CONSTRAINT, not a policy choice: eq.get_entity_list() only reaches
    --     THIS zone, so a member elsewhere cannot be resolved to a Lua_Client, cannot be assigned and
    --     cannot be moved. They are TOLD they were left behind rather than silently dropped.
    --   * Lua_Group:GetMember(i) returns a Lua_Mob, NOT a Lua_Client -- CharacterID does not exist on
    --     Lua_Mob and calling it is a runtime nil (the section 24 `e.other` trap). Resolve through the
    --     entity list.
    local party, party_c = {}, {}
    local grp = c:GetGroup()
    if grp and grp.valid then
        for i = 0, grp:GroupCount() - 1 do
            local m = dungeon.as_client and dungeon.as_client(grp:GetMember(i)) or nil
            -- ⚠️⚠️ THE LOCKOUT IS CHECKED PER MEMBER, NOT JUST ON THE LEADER, AND IT HAS TO BE NOW
            -- THAT THE STAMP IS TAKEN AT ENTRY. Only the leader was checked (above), which was nearly
            -- harmless while the stamp happened on the kill -- but bringing a locked-out member now
            -- RE-STAMPS them, so being invited to somebody else's raid would push your own lockout a
            -- further 24 hours out. Being unable to go is not a reason to be punished for it.
            -- 📌 Left behind rather than refusing the whole raid, which is this module's existing
            -- shape (a member in another zone is dropped the same way and told so).
            if m and m.valid and not M.current_run(m) and not M.locked_out(m, idx) then
                party[#party + 1]     = m:CharacterID()
                party_c[#party_c + 1] = m
            end
        end
    end
    local have_leader = false
    for _, id in ipairs(party) do if id == c:CharacterID() then have_leader = true break end end
    if not have_leader then
        table.insert(party, 1, c:CharacterID())
        table.insert(party_c, 1, c)
    end

    -- ---------------------------------------------------------------- create
    -- ⚠️ A FRESH INSTANCE EVERY TIME (never eq.get_instance_id) -- reusing one hands back a zone still
    -- holding the last raid's corpses and a boss that is already dead.
    --
    -- ⚠️⚠️ A REAL EXPEDITION (DZ), WITH THE MOVE DEFERRED -- which is what makes it survive. The delve
    -- uses a plain instance and its long note explains why: create-and-move in one breath is reaped by
    -- world as ExpiredEmpty in ~270ms. But that note is precise about the CAUSE, and it is the
    -- immediacy, not the expedition: during transit the dz zoneserver holds 0 players, and world's
    -- reap test is `!HasMembers() || IsExpired()` against a copy of the DZ whose member list arrives
    -- from an ASYNC packet. Give that round trip a moment and HasMembers() is true, so the empty-zone
    -- half of the test can no longer fire on its own.
    -- 📌 A raid is the right place to pay a two second wait and the delve was not: the delve's whole
    -- point is that Enter puts you in the dungeon, while a raid is a once-a-day scheduled thing whose
    -- real-world equivalent (the Agent of Change) makes you zone in as a separate act entirely.
    -- ⚠️⚠️ AND IT DEGRADES SAFELY, WHICH IS THE PART THAT MAKES THIS WORTH DOING. The measured trace
    -- says `instance_list survives` the reap -- only the DZ and its member row go -- and the deferred
    -- move RE-ASSERTS the membership row before it moves anybody. So if the DZ is reaped anyway the
    -- player still enters, just without the compass: exactly the old plain-instance behaviour. There
    -- is no failure mode here that is worse than what this replaced.
    -- ⚠️ Set M.USE_EXPEDITION = false to go straight back to the plain instance if this misbehaves.
    local inst, dz
    if M.USE_EXPEDITION then
        -- ⚠️ The table form is the shape every stock script uses (kodtaz/Maroley_Nazuey.lua is the
        -- reference). `zonein` is where the DZ puts you, so it must match enc.entry or the engine's
        -- own entry and ours would disagree.
        local ok_dz, made = pcall(function()
            return c:CreateExpedition({
                expedition = { name = enc.name, min_players = 1, max_players = M.MAX_LOOTERS },
                instance   = { zone = enc.zone, version = M.VERSION_BASE + idx,
                               duration = M.DURATION_SECS },
                zonein     = { x = enc.entry.x, y = enc.entry.y,
                               z = enc.entry.z, h = enc.entry.h or 0 },
            })
        end)
        if ok_dz and made and made.valid then
            dz = made
            inst = dz:GetInstanceID()
        end
    end

    -- ⚠️ The plain instance is the FALLBACK, not a second path to maintain: everything below is
    -- identical either way, because a DZ owns an ordinary instance and GetInstanceID hands it over.
    if not inst or inst == 0 then
        inst = eq.create_instance(enc.zone, M.VERSION_BASE + idx, M.DURATION_SECS)
    end
    if not inst or inst == 0 then
        c:Message(MT.Red, "The raid could not be opened. Try again shortly.")
        return
    end

    -- ⚠️ Safe return is set from where the LEADER is standing, which is the same point the `back`
    -- bucket records below -- so the engine's safe return and our own Exit agree rather than sending
    -- a player to two different places depending on how they left.
    -- ⚠️⚠️ NO COMPASS, DELIBERATELY (owner's call, 2026-08-30). A DZ compass is meant to point at the
    -- door you walk through to get in; entry here is a teleport, so there is no door and the arrow
    -- would point at the spot you were standing when you pressed Enter. That is not a hint, it is a
    -- wrong one. Leaving it unset is enough -- a DZ has no compass unless one is given.
    if dz then
        pcall(function()
            dz:SetSafeReturn(eq.get_zone_id(), c:GetX(), c:GetY(), c:GetZ(), c:GetHeading() or 0)
        end)
    end
    -- ⚠️ The token is the run's identity. os.time() is enough: two raids cannot be created in the
    -- same second on the same recycled id, because the id is not free until the first has closed.
    local token = os.time()
    set_instance_enc(inst, idx, token)

    -- ⚠️⚠️ ASSIGN EVERY MEMBER BEFORE ANY OF THEM MOVES. eq.create_instance adds NOBODY, and a member
    -- moved without a membership row is silently evicted by world's own entry check
    -- (VerifyInstanceAlive -> CheckInstanceByCharID). The symptom is "it opened the raid and then
    -- ported me to the bazaar" -- no error, nothing wrong in the zone that booted.
    -- ⚠️ Argument order is (instance_id, character_id), NOT the reverse.
    for _, id in ipairs(party) do
        eq.assign_to_instance_by_char_id(inst, id)
    end

    -- ⚠️⚠️ THE LOCKOUT IS STAMPED HERE, AT CREATION -- NOT ON THE KILL (owner's call, 2026-08-30).
    -- Entering spends your attempt, so a party that wipes is locked out exactly as one that wins.
    -- That matches how the native replay timer works (it starts when the expedition is MADE, which is
    -- why the two can now agree) and it removes a whole class of problem the completion-time stamp
    -- had: on_boss_death could only reach players standing in the instance at that moment, so the
    -- dead needed a second stamp on rejoin to avoid walking away unlocked.
    -- ⚠️ EVERY member, not just the leader -- they are all entering, and it is per character.
    for _, m in ipairs(party_c) do
        eq.set_data(bkey(m, "back"), string.format("%d|%d|%d|%d",
            eq.get_zone_id(), math.floor(m:GetX()), math.floor(m:GetY()), math.floor(m:GetZ())))
        set_run(m, idx, inst, enc.zone, token)
        stamp_lockout(m, idx)
    end

    -- ⚠️⚠️ THE BUCKET IS STILL THE AUTHORITY; THIS IS SO THE CLIENT CAN SEE THE TIMER. M.can_enter and
    -- the Raids tab's Status column both read the bucket, and the bucket is the only one of the two
    -- that exists on the plain-instance fallback -- so if this were the only lockout, flipping
    -- M.USE_EXPEDITION to false would silently make every raid farmable on repeat.
    -- 📌 Both are stamped in this one place, from the same M.LOCKOUT_SECS, so they cannot drift.
    -- ⚠️ A GM clearing the buckets does NOT clear the native lockout; the expedition window would
    -- still show a timer and CreateExpedition can refuse. Clear both, or expect the fallback path.
    if dz then
        pcall(function() dz:AddReplayLockout(M.LOCKOUT_SECS) end)
    end

    if grp and grp.valid and #party < grp:GroupCount() then
        c:Message(MT.Yellow, string.format(
            "%d of your group could not be brought -- they must be in this zone, not already in a run, and not still locked out.",
            grp:GroupCount() - #party))
    end

    -- ---------------------------------------------------------------- move
    -- ⚠️⚠️ WITH A DZ THE MOVE IS DEFERRED, AND THE DELAY IS THE ENTIRE POINT -- see the create block.
    -- It gives world's copy of the expedition time to receive its member list, so HasMembers() is
    -- true by the time the party is in transit and the empty-zone reap cannot fire.
    -- ⚠️ The plain-instance fallback moves IMMEDIATELY, exactly as before: there is no DZ to keep
    -- alive, so a wait would buy nothing and would only make entry feel slow.
    -- ⚠️ The leader is in this list, so NOTHING after this loop may touch `c`: MovePC tears down the
    -- group member pointers, which is what stranded the rest of the group in the travel window's
    -- group port (section 54). That is true of the immediate branch; the deferred branch does not
    -- move anybody here at all, which is one more reason it is the safer of the two.
    if dz then
        for _, m in ipairs(party_c) do
            -- ⚠️ NOT set_transit here. The flag exists to stop event_disconnect tearing a run down
            -- during a zone change, and nobody is changing zone yet -- it is set in the timer, right
            -- before that member's own move, where it means something.
            m:Message(MT.Yellow, string.format(
                "%s is being prepared. You will be taken there in a moment.", enc.name))
            m:SetTimer("raidenter", M.ENTER_DELAY_SECS)
        end
    else
        for _, m in ipairs(party_c) do
            set_transit(m)
            m:MovePCInstance(M.zone_id_of(enc), inst,
                enc.entry.x, enc.entry.y, enc.entry.z, enc.entry.h or 0)
        end
    end
end

-- The deferred half of M.enter, from the "raidenter" client timer.
--
-- ⚠️⚠️ IT READS THE RUN BUCKET RATHER THAN CLOSING OVER ANYTHING. The bucket was written before the
-- timer was armed and holds the instance and the encounter, so this needs no parked state of its own
-- and cannot act on a run that has since been torn down -- get_run simply returns nil.
-- ⚠️⚠️ THE MEMBERSHIP ROW IS RE-ASSERTED FIRST, and that is what makes the whole design safe. If
-- world reaped the DZ anyway, the DZ's member row went with it while `instance_list` survived -- so
-- writing the row back means world's entry check (VerifyInstanceAlive -> CheckInstanceByCharID)
-- passes and the player still gets in, minus the compass. That is the old behaviour, not a failure.
function M.on_enter_timer(c)
    if not (c and c.valid) then return end
    -- ⚠️ StopTimer FIRST -- a client timer REPEATS, and without this it would keep trying to move the
    -- player into the raid every few seconds for the rest of the session.
    c:StopTimer("raidenter")

    -- ⚠️ M.current_run, NOT a bare get_run -- that name does not exist in this module and Lua would
    -- resolve it to nil and fail only when a player actually pressed Enter. luacheck parses, it does
    -- not resolve, so it passes either way.
    local run = M.current_run(c)
    if not run then return end
    local enc = M.ENCOUNTERS[run.idx]
    if not (enc and run.inst and run.inst ~= 0) then return end

    eq.assign_to_instance_by_char_id(run.inst, c:CharacterID())
    set_transit(c)
    c:MovePCInstance(M.zone_id_of(enc), run.inst,
        enc.entry.x, enc.entry.y, enc.entry.z, enc.entry.h or 0)
end

-- ------------------------------------------------------------------ the boss
-- Called from global_player.event_enter_zone once somebody has actually arrived.
--
-- ⚠️⚠️ SPAWNED ON FIRST ARRIVAL, NOT AT CREATE. eq.spawn2 is zone-local and the instance's zone
-- process does not exist until somebody is being sent to it -- the same reason the world boss (17c)
-- arms a bucket and lets the first player through the door do the spawning.
-- ⚠️ Guarded so a second arrival does not spawn a second boss.
-- ⚠️⚠️ RAID BOSS STATS ARE SET HERE, NOT IN `npc_types` -- AND THEY HAVE TO BE.
-- `ScaleNPC` rewrites the stat block **wholesale** from `npc_scale_global_base` (section 24), so
-- whatever the stock row says is discarded. Measured before this existed, every boss came out at
-- level 30 as:
--     Phinigel / Velketor  IsRaidTarget -> scale type 2 -> 1,800 hp, max hit 102, 3.0s swing
--     Mayong               not raid_target, but its NAME starts uppercase -> type 1 -> 1,440 hp
-- ⚠️⚠️ That is not "a bit low", it is two orders of magnitude out: at a duo's ~300 dps Phinigel died
-- in SIX SECONDS, and a delve warden at rung 30 (~6,750 hp, M.BOSS_HP_MULT 5.0 over a measured
-- regular mob) was nearly FOUR TIMES tougher than the raid boss it is meant to be a warm-up for.
--
-- Sized from the measured player baseline: a tanky character is ~3,000 max hp at ~50 percent
-- mitigation, a duo does ~300 dps and a full group ~900, so every 10,000 boss hp is about 30
-- seconds of duo fight. Auto-attack is targeted at **100-200 max hit per SECOND of swing time**,
-- which at a 2.5s swing is a 250-500 max hit.
--     Phinigel  120k hp, 350 max hit -> 140/sec; a 3k tank survives ~43s unhealed
--     Mayong    160k hp, 400 max hit -> 160/sec
--     Velketor  200k hp, 450 max hit -> 180/sec; ~33s unhealed
-- ⚠️⚠️ AC IS SET EXPLICITLY, BECAUSE LEAVING IT TO THE SCALER MADE THE THREE INCONSISTENT.
-- GetNPCScalingType returns 2 for IsRaidTarget() and 1 for "the name starts with a capital" -- and
-- Mayong is NOT flagged raid_target, so he resolved to type 1 and would have carried **AC 152 while
-- the other two carried 191**. Players would connect noticeably more often on him, so he would die
-- faster than his hp total implies, for a reason invisible in this file.
-- 📌 191 is simply what the level 30 raid row already gives -- this pins the value rather than
-- inflating it. Raising AC further would change how often players connect and so invalidate the
-- ~300 dps estimate these hp totals are derived from. **Tune hp and hit first, not AC.**
M.BOSS_AC = 191
-- ⚠️⚠️ EVERY RAID BOSS GETS THESE, AND TWO OF THE THREE HAD NONE OF THEM.
-- Measured: Phinigel carried summon + mez/charm/fear/flee immunity, Velketor those plus stun and
-- snare -- and **Mayong carried NOTHING AT ALL**. A bare Mayong is mezzable, charmable, snareable
-- and has no summon, so a single enchanter removes the encounter and anyone can kite him forever.
--   1  Summon            -- the anti-kite rule; without it a raid boss is a training dummy
--   13 MesmerizeImmunity -- a mezzed boss is not a fight
--   14 CharmImmunity     -- a CHARMED raid boss is catastrophic, not merely easy
--   17 FearImmunity      -- fear-kiting is the same exploit as running
--   21 FleeingImmunity   -- bosses do not flee at low health; that is a trash behaviour
-- ⚠️⚠️ SLOW IS DELIBERATELY LEFT LANDABLE (ability 12 is NOT in this list). Slow is the core group
-- contribution of shamans, enchanters and beastlords, and a slow-immune boss deletes their reason to
-- be in the raid. Stun is likewise left alone -- Velketor is stun immune because his STOCK row says
-- so, not because we decided it.
M.BASE_ABILITIES = { 1, 13, 14, 17, 21 }

function M.apply_boss_stats(npc, enc)
    if not (npc and npc.valid and enc.hp) then return end
    -- ⚠️ attack_delay is in TENTHS of a second here: NPC::ModifyNPCStat multiplies by 100 to get ms
    -- (npc.cpp), so 25 is a 2.5 second swing. Passing 2500 would give a four-minute swing timer.
    npc:ModifyNPCStat("attack_delay", tostring(enc.delay))
    npc:ModifyNPCStat("ac",           tostring(M.BOSS_AC))
    npc:ModifyNPCStat("min_hit",      tostring(enc.hit_lo))
    npc:ModifyNPCStat("max_hit",      tostring(enc.hit_hi))
    -- ⚠️⚠️ max_hp LAST and ONCE. Every ModifyNPCStat("max_hp") runs CalcMaxHP, so an earlier call
    -- would be recomputed from the later one rather than combining (section 24).
    npc:ModifyNPCStat("max_hp",       tostring(enc.hp))
    -- ⚠️⚠️ AND THE EXPLICIT SetHP IS MANDATORY: max_hp clamps DOWN only, so raising the maximum does
    -- not raise CURRENT health. Without this the boss keeps the ~1,800 hp it was scaled to and dies
    -- exactly as fast as before, while `#showstats` reports the new maximum -- the failure looks
    -- like the multiplier not applying.
    npc:SetHP(enc.hp)

    -- ⚠️⚠️ SetSpecialAbility, NOT ModifyNPCStat("special_abilities", ...). That key REPLACES the
    -- creature's entire ability set from a packed string and would silently wipe what the stock row
    -- already grants -- Phinigel's summon and magical-attack requirement, Velketor's stun and snare
    -- immunity (section 43 records this exact trap).
    for _, a in ipairs(M.BASE_ABILITIES) do npc:SetSpecialAbility(a, 1) end
    for _, a in ipairs(enc.abilities or {}) do npc:SetSpecialAbility(a, 1) end

    -- ⚠️⚠️ ATTACHED **AFTER** ScaleNPC, AND THAT IS THE WHOLE REASON IT WORKS. AI_AddNPCSpells keeps
    -- only entries whose minlevel/maxlevel bracket GetLevel() AT THE MOMENT IT IS CALLED (section
    -- 24), so attaching before the scale would load a list filtered for the stock row's level -- 35
    -- for Phinigel, 85 for Mayong -- and freeze it there. Re-attaching is safe: the function clears
    -- AIspells first, so nothing stacks.
    -- ⚠️ Only for a boss with a caster CLASS and therefore a mana pool. NPC::CalcMaxMana returns 0
    -- for a non-caster class whatever the mana column says, and the AI cast gate is
    -- `cost <= GetMana() || GetMana() == GetMaxMana()` -- which for a 0-mana NPC is `0 == 0`, always
    -- true, so a warrior-class boss holding a wizard list would nuke forever for free.
    if enc.spells then npc:ModifyNPCStat("npc_spells_id", tostring(enc.spells)) end
end

function M.ensure_boss(c)
    local run = M.current_run(c)
    if not run then return end
    local idx, enc = M.encounter_of_instance(run.inst)
    if not enc then return end
    if eq.get_data("raid_spawned_" .. run.inst) == "1" then return end
    eq.set_data("raid_spawned_" .. run.inst, "1")

    -- ⚠️⚠️ eq.spawn2 RETURNS A Lua_Mob, NOT A Lua_NPC (section 26). ScaleNPC / AddItem / AddCash are
    -- bound on Lua_NPC only, so calling one on the result is "attempt to call method (a nil value)"
    -- at runtime with nothing to warn you at write time -- and the error aborts the rest of this
    -- handler. CastToNPC IS bound (on Lua_Entity), unlike CastToClient.
    local mob = eq.spawn2(enc.npc, 0, 0, enc.boss.x, enc.boss.y, enc.boss.z, enc.boss.h or 0)
    if not mob or not mob.valid then return end
    local npc = mob:CastToNPC()
    if npc and npc.valid then
        -- ⚠️ ScaleNPC BEFORE any ModifyNPCStat: it rewrites the stat block wholesale from
        -- npc_scale_global_base and discards anything set first (section 24).
        npc:ScaleNPC(enc.level)
        -- ⚠️ AFTER ScaleNPC, always: it discards anything set before it (section 24).
        M.apply_boss_stats(npc, enc)
        npc:SetEntityVariable("raid_enc", tostring(idx))
    end
end

-- ------------------------------------------------------------------ mechanics
-- ⚠️⚠️ WHY THIS IS A POLLED TIMER AND NOT AN HP EVENT. There is no `SetNextHPEvent` binding on
-- Lua_NPC on this build -- EVENT_HP exists in the parser but nothing exposes the setter -- so a phase
-- threshold has to be noticed rather than delivered. M.boss_tick runs off the same client timer
-- family as the sweep; the cost is one GetHPRatio per raider per few seconds inside a raid instance
-- and an early-out everywhere else.
--
-- ⚠️⚠️ EVERY PHASE FIRES EXACTLY ONCE, AND THE GUARD IS ON THE BOSS, NOT ON THE PLAYER. The timer is
-- PER CLIENT, so six raiders run this loop six times a tick and an unguarded threshold would spawn
-- six waves of adds. The fired set is an entity variable on the boss itself -- one object every
-- raider's tick can see -- and it is written BEFORE the effect, the same ordering §26 uses to stop
-- two delve completions spawning two chests.
--
-- ⚠️ Adds are EXISTING npcs from each boss's own zone, not new npc_types. That keeps this pure Lua:
-- no migration, no npc_types rows to seed, nothing to re-run after an import. They are scaled at
-- spawn like delve creatures, so their stock level is irrelevant.
-- 📌 Deliberately NOT rampage, and deliberately not an AE nuke. The design doc rejected rampage
-- because a 400 max hit against 3,000 hp characters kills healers outright in a duo, and the same
-- arithmetic damns any untargeted AE. §43's rule for affixes applies here too: a mechanic should
-- change how a fight FEELS, never whether it is winnable. Adds are a target switch; fury is a clock.
M.MECHANICS = {
    -- Phinigel: swarm waves out of the dark water. He already casts (Default Wizard List), so the
    -- adds are pressure on the group's attention rather than on the healer.
    [1] = {
        kind  = "adds",
        npc   = 64005,        -- a_stingtooth_piranha, Kedge Keep's own
        count = 3,
        level = -6,           -- relative to the boss: fodder, killable in a few swings
        at    = { 75, 50, 25 },
        emote = "churns the water, and shapes dart in from the dark!",
    },
    -- Mayong: no adds. He is class 1 with no mana and his identity IS the melee, so the fight gets
    -- FASTER rather than wider -- a clock the group races instead of a second thing to tank.
    [2] = {
        kind      = "fury",
        at        = { 66, 33 },
        delay_pct = 15,       -- attack_delay cut per phase, so he swings faster
        emote     = "snarls, and moves faster.",
    },
    -- Velketor: statues grind to life. Same shape as Phinigel but fewer and tougher, because he is
    -- the tier 3 fight and the group bringing him down is expected to be a real one.
    [3] = {
        kind  = "adds",
        npc   = 112005,       -- a_crystal_statue, Velketor's own
        count = 2,
        level = -4,
        at    = { 75, 50, 25 },
        emote = "calls the ice, and the statues grind to life!",
    },
}

-- ⚠️ Fired phases are stored as a delimited string because an entity variable is text. Wrapped in
-- separators so "5" can never match inside "50" or "25".
local function phase_fired(npc, pct)
    local done = npc:GetEntityVariable("raid_phase") or ""
    return done:find("|" .. pct .. "|", 1, true) ~= nil
end

local function mark_phase(npc, pct)
    local done = npc:GetEntityVariable("raid_phase") or ""
    if done == "" then done = "|" end
    npc:SetEntityVariable("raid_phase", done .. pct .. "|")
end

local function fire_phase(npc, enc, mech, pct)
    -- ⚠️⚠️ MARK FIRST. Everything below can fail -- a spawn can return nil, a stat write can throw --
    -- and a phase that errors halfway must not be retried on the next raider's tick a moment later,
    -- which is how one wave becomes six.
    mark_phase(npc, pct)

    npc:Emote(mech.emote or "shudders.")

    if mech.kind == "adds" then
        for _ = 1, (mech.count or 1) do
            -- ⚠️ Spawned ON the boss, because there is no per-encounter add placement and a hardcoded
            -- point would be inside a wall in two of the three zones.
            local mob = eq.spawn2(mech.npc, 0, 0, npc:GetX(), npc:GetY(), npc:GetZ(), npc:GetHeading() or 0)
            if mob and mob.valid then
                local add = mob:CastToNPC()
                if add and add.valid then
                    -- ⚠️ ScaleNPC BEFORE anything else -- it rewrites the stat block wholesale (§24).
                    add:ScaleNPC(math.max(1, (enc.level or 30) + (mech.level or -5)))
                    -- ⚠️⚠️ NO LOOT. Under individual loot the table is re-rolled at DEATH per eligible
                    -- player (§31), so clearing the list at spawn is not enough -- the loottable id
                    -- has to go, exactly as the delve does it for its trash.
                    add:ModifyNPCStat("loottable_id", "0")
                    add:SetEntityVariable("raid_add", "1")
                    -- ⚠️⚠️ AGGRO THEM BY HAND. A spawn2'd npc arrives IDLE: it is not on anyone's hate
                    -- list, and social aggro will not do it either -- assist only propagates from a
                    -- mob that is already fighting, and these appear mid-fight next to a boss whose
                    -- hate list they have no link to. Reported from play: Velketor's statues spawned
                    -- and just stood there, which makes the whole mechanic decorative.
                    -- ⚠️ Every client in the instance, not just the boss's current target -- this is a
                    -- private zone, so everyone present is in the fight by definition, and hating only
                    -- the tank would let an add walk past the group to reach them.
                    -- 📌 Hate 1, deliberately low: enough to make them attack and pick a target, not
                    -- so much that the group cannot pull them off with real threat.
                    for m in eq.get_entity_list():GetClientList().entries do
                        if m and m.valid then add:AddToHateList(m, 1) end
                    end
                end
            end
        end
    elseif mech.kind == "fury" then
        -- ⚠️ attack_delay is a DELAY: lower is faster. Read the current value and cut it rather than
        -- writing an absolute, so two phases compound instead of the second overwriting the first.
        -- ⚠️⚠️ THE GETTER AND THE SETTER USE DIFFERENT UNITS, AND THIS IS NOT A ROUNDING DETAIL.
        -- ModifyNPCStat("attack_delay", v) stores `v * 100` (npc.cpp:2362) so 25 means 2,500 ms, but
        -- GetAttackDelay returns the STORED value -- 2500, not 25. Feeding the getter's answer back
        -- to the setter therefore multiplies by 100 every time: the first fury phase turned a 2.5
        -- second swing into 212,500 ms, one swing every THREE AND A HALF MINUTES.
        -- ⚠️ It fires at 66 percent, so it lands early and the boss simply stops hitting anyone.
        -- Reported from play as "Mayong wasn't attacking at all", which is exactly right.
        -- ⚠️ There is no GetAttackTimer binding either -- that name is a runtime nil that would abort
        -- the phase halfway, AFTER it has been marked fired, so the retry never comes.
        local cur_ms = npc:GetAttackDelay() or 0
        local cur    = (cur_ms > 0) and math.floor(cur_ms / 100) or (enc.delay or 25)
        local nd = math.max(8, math.floor(cur * (100 - (mech.delay_pct or 15)) / 100))
        npc:ModifyNPCStat("attack_delay", tostring(nd))
    end
end

-- Called from the per-client raid timer. No-op outside a raid instance.
function M.boss_tick(c)
    if not (c and c.valid) then return end
    local inst = eq.get_zone_instance_id()
    if not inst or inst == 0 then return end
    local idx, enc = M.encounter_of_instance(inst)
    if not enc then return end
    local mech = M.MECHANICS[idx]
    if not mech or not mech.at then return end

    local npc = eq.get_entity_list():GetNPCByNPCTypeID(enc.npc)
    if not (npc and npc.valid) then return end
    -- ⚠️ Dead or not yet engaged: GetHPRatio on a corpse-to-be is meaningless, and firing a phase
    -- before anyone has hit him would spawn a wave into an empty room.
    local pct = npc:GetHPRatio() or 100
    if pct <= 0 or pct >= 100 then return end

    for _, threshold in ipairs(mech.at) do
        if pct <= threshold and not phase_fired(npc, threshold) then
            fire_phase(npc, enc, mech, threshold)
        end
    end
end

-- ------------------------------------------------------------------ loot
-- Called from global_npc.event_death_complete. `npc` is the dying boss.
--
-- ⚠️⚠️ Lua_Raid HAS NO AddMember BINDING -- the raid API is READ ONLY from Lua (section 17c). Loot
-- rights are therefore granted the way the world boss does it: walk the hate list and AddLooter each
-- player. That covers latecomers and disturbs nobody's group.
-- ⚠️ corpse:AddLooter is on Lua_Corpse, NOT Lua_NPC.
-- 📌 Individual loot (section 31) rolls the table once PER ELIGIBLE PLAYER at death, so a six person
-- group gets six personal rolls on one corpse and nobody contends. Lootslots are numbered per player
-- so the 34 slot ceiling applies per player -- but section 30 records the sharp test as only ever
-- having been done with TWO characters. A raid is the first thing that will push it.
M.MAX_LOOTERS = 72

-- ⚠️⚠️ ROUGH TITANWROUGHT MOLDS ARE ORDINARY CORPSE LOOT AS OF 2026-08-31 (migration v158). There is
-- no `grant_molds` any more and nothing in this module hands one out. They sit in lootdrop **200050**
-- (all 70, equal weight), attached to each boss's OWN loottable -- Phinigel 10831, Velketor 121,
-- Mayong 110043 -- at `mindrop 2 / droplimit 2 / probability 100`, which reproduces the old
-- MOLD_DROPS = 2 exactly. Retuning is now an UPDATE on that row, not a code change.
--
-- ⚠️⚠️ THE FUNCTION THAT USED TO LIVE HERE WAS BUILT ON A FALSE PREMISE, AND IT IS WORTH KNOWING
-- WHICH ONE. Its comment read *"corpse loot is ONE physical reward shared by the group, so six
-- raiders would take one mold between them"*. That is true of **`global_loot`** and FALSE of an npc's
-- own loottable: under `AoT:IndividualLoot`, `NPC::Death` (zone/attack.cpp:3190) walks every allowed
-- looter and rolls `GetLoottableID()` once PER PLAYER, owner-stamping each roll (§31). Corpse loot
-- was already individual, so the bespoke SummonItemExact path was solving a problem that did not
-- exist -- the same conflation of the two kinds of loot that put the named-mob molds on global_loot.
-- 📌 The real win is not tidiness. The old path ran from `on_boss_death`, and that hook paid NOTHING
-- for the entire life of the feature because global_npc handed it a nil corpse; loot rolled in C++ at
-- death cannot fail that way, because it does not depend on this module running at all.
-- ⚠️ Rough is still raid-only. It is on the three boss loottables and in no `global_loot` row, so the
-- design §9 rule -- no second route to the tier that beats what the world can give you -- still holds.

function M.on_boss_death(npc, corpse)
    if not npc or not npc.valid then return end
    local enc_idx = tonumber(npc:GetEntityVariable("raid_enc") or "")
    if not enc_idx then return end
    local enc = M.ENCOUNTERS[enc_idx]
    if not enc then return end

    -- ⚠️⚠️ THE HATE LIST ENTRY FIELD IS `.ent`, NOT `.mob`. Reading the wrong one is a silent nil --
    -- the loop runs, grants nothing, and errors nowhere. aotv4_worldboss.lua is the reference.
    -- ⚠️ The corpse is resolved by NAME from the entity list rather than assumed to arrive as an
    -- argument, which is how the world boss does it and works from event_death_complete.
    -- ⚠️⚠️ THIS FALLBACK IS DEAD AND MUST NOT BE RELIED ON -- keep it only because a valid `corpse`
    -- argument makes it unreachable. `GetCorpseByName` is an exact strcmp (entity.cpp:2029) and a
    -- corpse is named `<npc name>'s corpse<entity id>` (corpse.cpp:157, :639), so GetCleanName() can
    -- never match: "Phinigel Autropos" against "#Phinigel_Autropos's corpse123".
    -- 📌 It hid a real bug for the life of this module. global_npc passed `nil` for the corpse, this
    -- silently returned nothing, and the loot block below -- loot rights AND Rough molds -- was
    -- skipped on every single raid kill with no error anywhere.
    if not (corpse and corpse.valid) then
        corpse = eq.get_entity_list():GetCorpseByName(npc:GetCleanName())
    end
    if not (corpse and corpse.valid) then
        -- ⚠️ Say so rather than failing silently: this is the one path that hands out the raid's
        -- entire reward, and it produced nothing at all for weeks without a single line anywhere.
        eq.debug(string.format(
            "aotv4_raid: %s died with NO corpse -- no loot rights and no molds were granted.",
            tostring(enc and enc.name or "a raid boss")))
    end
    if corpse and corpse.valid then
        local n = 0
        -- ⚠️⚠️ `for h in list.entries do`, NOT ipairs. GetHateList returns a luabind container
        -- (Lua_HateList, zone/lua_hate_list.h:48) whose vector is exposed as `.entries` -- exactly
        -- like GetClientList. `ipairs` over the container itself is a hard error, *"bad argument #1
        -- to 'ipairs' (table expected, got userdata)"*, which aborts the whole handler.
        -- ⚠️ That is what happened on the first Phinigel kill (2026-08-30): the abort came a few lines
        -- EARLIER, so nobody was granted loot rights and nobody got molds -- the boss died and paid
        -- nothing. Every one of these loops in this file was wrong, and none had ever run.
        for h in npc:GetHateList().entries do
            local m = h.ent
            if m and m.valid and m:IsClient() and n < M.MAX_LOOTERS then
                -- ⚠️ AddLooter is the ONLY thing this loop still does, and it is not redundant:
                -- NPC::Death grants loot rights natively to the KILLER'S GROUP, and a raid assembled
                -- from more than one group (or anyone on the hate list outside it) would otherwise be
                -- unable to open the corpse their own owner-stamped roll is sitting on.
                -- 📌 The molds are no longer handed out here -- they roll onto the corpse from
                -- lootdrop 200050 like any other loot. See the note above M.on_boss_death.
                corpse:AddLooter(m)
                n = n + 1
                m:Message(MT.Yellow, "You have earned a share of the spoils.")
            end
        end
    end

    -- ⚠️⚠️ THE CLOSE CLOCK STARTS HERE AND IT OUTRANKS "THE ZONE IS EMPTY". Once the boss is down the
    -- instance is held open for M.DEAD_CLOSE_SECS **even with nobody in it**, because the people most
    -- likely to be absent at this exact moment are the ones who died to it -- and on this server a
    -- death means level 1 at your bind point with no corpse (§57), so they are not merely elsewhere,
    -- they are gone. That window is the only chance they have to come back for their share.
    -- 📌 Without this the two close rules fight each other: the last survivor stepping out after the
    -- kill would empty the zone and close the raid on top of the loot the grace exists to protect.
    local inst_now = eq.get_zone_instance_id()
    if inst_now and inst_now ~= 0 then
        eq.set_data("raid_dead_" .. inst_now, tostring(os.time()))
    end

    -- ⚠️⚠️ SOMEBODY HAS TO STAY, AND THE PLAYERS HAVE TO BE TOLD SO. An empty dynamic zone terminates
    -- itself after Zone:AutoShutdownDelay -- **60 seconds** -- and an NPC corpse is NOT persisted, so
    -- the boss's corpse and every owner-stamped roll on it are destroyed with the zone process. The
    -- rejoin window is five minutes, which is deliberately LONGER than that: it is the time the dead
    -- have to get back, not a promise the room will still be there. If the last survivor walks out at
    -- the 60 second mark, everyone's loot goes with them.
    -- 📌 This is why it is a message and not a silent rule. Nothing on the server can hold an empty
    -- zone open, so the only thing that makes the window real is a player choosing to stand in it --
    -- and they will not choose that if nobody tells them.
    -- ⚠️ `.entries`, not ipairs -- see the hate list above.
    local el2 = eq.get_entity_list()
    for m in el2:GetClientList().entries do
        if m and m.valid then
            m:Message(MT.Yellow,
                "Stay in the zone until your fallen return -- if everyone leaves, the corpse and its loot go with the room.")
        end
    end

    -- Everyone still inside is credited and locked out; the instance closes on the sweep rather
    -- than immediately.
    -- ⚠️⚠️ DO NOT TEAR THE INSTANCE DOWN HERE. This fires on the boss's DEATH -- the instant the corpse
    -- appears, before anybody could possibly have looted it. Section 26 records the delve chest doing
    -- exactly that and destroying the instance out from under the player, taking the reward with it.
    -- ⚠️ `.entries`, not ipairs -- see the hate list above.
    local el = eq.get_entity_list()
    for m in el:GetClientList().entries do
        if m and m.valid then
            local run = M.current_run(m)
            if run and run.idx == enc_idx then
                -- ⚠️ NO stamp_lockout HERE ANY MORE (2026-08-30). The lockout is taken at CREATION, so
                -- stamping again on the kill would push it a further 24 hours out from the moment the
                -- boss died -- a party that fought for twenty minutes would be locked longer than one
                -- that zerged it, which is backwards.
                m:Message(MT.Yellow, string.format("%s has fallen.", enc.name))
            end
        end
    end
end

-- ------------------------------------------------------------------ rejoining
-- Pressing Enter on the same raid row you already have a run for comes back HERE rather than trying
-- to start a second raid.
--
-- ⚠️⚠️ GATED ON THE BOSS BEING DEAD, NOT ON "COMBAT HAS ENDED" IN GENERAL -- because the rejoining
-- player is standing in ANOTHER ZONE and `eq.get_entity_list()` only ever reaches the zone you are
-- in. There is no way to ask "is anything fighting in that instance" from outside it. The boss's
-- death is recorded in a bucket, which IS readable from anywhere, and it is the case that matters:
-- somebody who died wants their loot, not a second chance at the fight.
-- 📌 So a mid-fight rejoin is deliberately refused. At level 1 with no gear it would only feed the
-- boss another death, and it would let a raid rotate corpses back in indefinitely.
function M.rejoin(c, run)
    local idx, enc, token = read_instance_enc(run.inst)
    -- ⚠️ BOTH the marker and the token must agree. A closed raid has had its marker deleted by the
    -- sweep; a recycled id has a marker with a DIFFERENT token. Either way this is not their raid.
    if not enc or idx ~= run.idx or token ~= run.token then
        clear_run(c)
        c:Message(MT.Yellow, "That raid has already closed.")
        return
    end
    local dead_at = tonumber(eq.get_data("raid_dead_" .. run.inst) or "") or 0
    if dead_at == 0 then
        c:Message(MT.Red, string.format("%s is still fighting. You cannot return yet.", enc.name))
        return
    end
    -- ⚠️⚠️ THE WINDOW IS ENFORCED HERE AS WELL AS IN THE SWEEP, AND IT HAS TO BE. The sweep runs on a
    -- CLIENT timer inside the instance -- and the whole point of the post-kill window is that the
    -- instance may be EMPTY, so there is often nobody in there to run it. Left to the sweep alone the
    -- marker would survive until the engine expired the instance at M.DURATION_SECS and somebody
    -- could stroll back in hours later. This check is what makes the five minutes real.
    -- 📌 An empty post-kill instance is not DESTROYED at five minutes, only closed to re-entry -- but
    -- nothing is leaked by that. Zone:AutoShutdownDelay is 60 seconds, so the empty zone process ends
    -- itself and releases its port long before then; all that survives is one cheap instance_list row
    -- until M.DURATION_SECS.
    -- ⚠️⚠️ AND THAT AUTO-SHUTDOWN IS WHY REJOINING IS NOT A GUARANTEE. NPC corpses are not persisted,
    -- so when the empty zone terminates it takes the boss's corpse -- and every owner-stamped roll on
    -- it -- with it. Coming back inside the window re-boots the zone into an EMPTY room. The window is
    -- the time the dead have to return; only a living player standing in the instance keeps the loot
    -- alive. See the message in M.on_boss_death, which is what turns that from a trap into a rule.
    if os.time() - dead_at >= M.DEAD_CLOSE_SECS then
        clear_run(c)
        c:Message(MT.Yellow, string.format("%s has closed. You were too slow to return.", enc.name))
        return
    end

    -- ⚠️⚠️ NO stamp_lockout HERE ANY MORE (2026-08-30). This used to exist because the lockout was
    -- taken on the KILL, and on_boss_death can only reach players standing in the instance at that
    -- moment -- the people who rejoin are by definition the ones who were not, because they were dead,
    -- so without a second stamp they walked away unlocked. The lockout is now taken at CREATION, which
    -- means a rejoining player was already stamped on the way in and this whole hole is closed at the
    -- source rather than patched at the two places it leaked.
    -- 📌 That is the real argument for stamping at entry: one place, and nobody can be missed.

    -- ⚠️ Re-assign before moving, exactly as on the way in: the membership row may have been reaped,
    -- and without one world silently redirects them to their safe return.
    eq.assign_to_instance_by_char_id(run.inst, c:CharacterID())
    eq.set_data(bkey(c, "back"), string.format("%d|%d|%d|%d",
        eq.get_zone_id(), math.floor(c:GetX()), math.floor(c:GetY()), math.floor(c:GetZ())))
    set_transit(c)
    c:MovePCInstance(M.zone_id_of(enc), run.inst, enc.entry.x, enc.entry.y, enc.entry.z, enc.entry.h or 0)
end

-- ------------------------------------------------------------------ leaving
-- ⚠️ ONE teardown path for every way out, as M.leave is for the delve.
-- ⚠️⚠️ EXIT MUST WORK OFF WHERE YOU ARE STANDING, NOT ONLY OFF THE BUCKET. It used to `return` the
-- moment `current_run` was nil, which meant a player physically inside a raid instance with no run
-- record was STRANDED: Exit answered "you are not in a delve" and there was nothing else to press.
-- Two ways to reach that, and neither needs a GM:
--   * this function deleted the `back` bucket BEFORE parsing it, so a malformed or missing value left
--     the run cleared, the return point gone, and the player still inside;
--   * anything that clears buckets out from under a live run -- a GM tidying lockouts, a botched
--     migration -- does the same.
-- 📌 Found the hard way on 2026-08-31: clearing raid lockouts by hand deleted a live run bucket and
-- left a player with no way out of Mistmoore.
-- ⚠️ `in_raid_zone` is the standing-in-one test, and it is what makes Exit safe: with a run we do the
-- ordinary thing, without one we still get them out.
function M.leave(c, quiet)
    if not (c and c.valid) then return end
    local run = M.current_run(c)

    -- Nothing to do only when BOTH are false: no record and not standing in one.
    -- ⚠️ M.in_raid_zone is the single definition of that test -- delveexit routes on the same one, so
    -- the button and the handler can never disagree about whether you are in a raid.
    if not run and not M.in_raid_zone() then return end

    if run then clear_run(c) end
    -- ⚠️ A client timer REPEATS -- without this a raider who leaves keeps polling for a boss in a
    -- zone they are no longer in, for the rest of the session.
    c:StopTimer("raidboss")

    -- ⚠️⚠️ PARSE BEFORE DELETING. The old order deleted `back` and then tried to read it, so a value
    -- that failed the match took the return point with it and left the player where they were.
    local back = eq.get_data(bkey(c, "back"))
    local z, x, y, zz
    if back and back ~= "" then
        z, x, y, zz = back:match("^(%d+)|(-?%d+)|(-?%d+)|(-?%d+)$")
    end

    set_transit(c)
    if z then
        eq.delete_data(bkey(c, "back"))
        c:MovePCInstance(tonumber(z), 0, tonumber(x), tonumber(y), tonumber(zz), 0)
    else
        -- ⚠️⚠️ THE FALLBACK IS THE BIND POINT, AND IT MUST EXIST. Being unable to work out where
        -- somebody came from is not a reason to leave them in a zone they cannot get out of -- and
        -- inside a closed raid instance there is no zone line to walk. Bind is always somewhere real.
        -- ⚠️ `back` is deliberately NOT deleted on this path: it is already unusable, and keeping it
        -- costs nothing while leaving evidence of what went wrong.
        c:MovePCInstance(c:GetBindZoneID() or 1, 0, c:GetBindX(), c:GetBindY(), c:GetBindZ(), 0)
        if not quiet then
            c:Message(MT.Yellow, "Your way back could not be found, so you return to your bind point.")
        end
    end
    if not quiet then c:Message(MT.Yellow, "You leave the raid.") end
end

-- ⚠️⚠️ A MEMBER LEAVING MUST NOT END IT FOR EVERYONE. These are real zones with real zone lines, and
-- the delve's answer -- fail the run the moment you find yourself outside your instance -- is wrong
-- here: one person walking a zone line cannot end a raid for five others. Per member only; the
-- instance itself is left alone and closed by the empty sweep.
function M.on_enter_zone(c)
    if not c or not c.valid then return end
    if in_transit(c) then clear_transit(c) end

    local run = M.current_run(c)
    if not run then return end

    -- Arrived in their own raid: make sure the boss exists.
    if eq.get_zone_short_name() == run.zone and eq.get_zone_instance_id() == run.inst then
        M.ensure_boss(c)
        -- ⚠️⚠️ ARMED HERE AND NOWHERE ELSE, so the fast tick only ever runs for somebody standing in a
        -- raid. Arming it on connect like `raidsweep` would put a 3 second data-bucket read on every
        -- player on the server, forever, to serve three encounters.
        -- ⚠️ Stopped in M.leave. A client timer REPEATS, and a raider who walks out with it still
        -- running keeps polling for a boss they can no longer see.
        c:SetTimer("raidboss", M.BOSS_TICK_SECS)
        return
    end
    -- ⚠️⚠️ SOMEWHERE ELSE ENTIRELY -- AND THE RUN IS DELIBERATELY **KEPT**. This is where a death
    -- lands: the roguelite reset puts them at their bind point, in another zone, at level 1, with no
    -- corpse to run back to (§57). Clearing the run here would be the obvious thing and it would take
    -- away the one route back to their share of the loot, which under individual loot (§31) is rolled
    -- for them personally, owner-stamped, and lootable by nobody else -- so it would simply rot.
    -- The token on the run is what makes keeping it safe: it cannot resolve to a recycled instance.
    -- 📌 The instance is left standing either way. One member crossing a zone line must not end a
    -- raid for the other five.
end

-- ⚠️ Camping or link-dying drops that member only, and deliberately does not move them: the client is
-- already leaving. The transit guard is what stops this firing on the way IN.
-- ⚠️ Camping or link-dying keeps the run for the same reason a death does -- they may well come back
-- and be owed loot. The transit guard is what stops this firing on the way IN, where EVENT_DISCONNECT
-- is raised by the zone they are LEAVING.
-- 📌 Nothing needs clearing on a stale run: the token makes it unresolvable once the instance closes,
-- and M.enter overwrites it when they start a new raid.
function M.on_disconnect(c)
    if not c or not c.valid then return end
    if in_transit(c) then return end
end

-- ------------------------------------------------------------------ the empty sweep
-- ⚠️ Raid instances close after being empty for a while, exactly like a delve, rather than the moment
-- the last person steps out -- otherwise a single zone line crossing ends the raid.
-- ⚠️ Runs on a CLIENT timer in the instance, so it costs nothing in an empty world; the instance's own
-- M.DURATION_SECS is the backstop for the case where nobody is left to run it.
function M.sweep(c)
    if not c or not c.valid then return end
    local inst = eq.get_zone_instance_id()
    if not inst or inst == 0 then return end
    if not M.encounter_of_instance(inst) then return end

    local dead_at = tonumber(eq.get_data("raid_dead_" .. inst) or "") or 0

    -- ⚠️⚠️ THE POST-KILL CLOCK IS CHECKED FIRST AND IGNORES OCCUPANCY. Once the boss is down the
    -- instance is held for M.DEAD_CLOSE_SECS whether anyone is standing in it or not, because the
    -- people it is being held FOR are by definition not in it -- they died, and on this server that
    -- means level 1 at their bind point with no corpse. Checking "is it empty" first would close the
    -- raid the moment the last survivor stepped out, on top of loot that is owner-stamped to the
    -- dead and lootable by nobody else.
    if dead_at > 0 then
        if os.time() - dead_at >= M.DEAD_CLOSE_SECS then
            M.close(inst)
        end
        return
    end

    -- Boss still alive: close once the zone has been empty for a while. The delay is what stops a
    -- single zone-line crossing, or the gap between one member zoning out and another zoning in,
    -- ending the raid for everybody.
    -- ⚠️⚠️ `.entries`, not ipairs -- and this one was the most dangerous of the four. `ipairs` over
    -- the container is a hard error, so the sweep aborted before it could set `occupied`; had it
    -- merely yielded nothing instead, the zone would have read as EMPTY with people standing in it
    -- and the raid would have closed on top of them.
    local el = eq.get_entity_list()
    local occupied = false
    for m in el:GetClientList().entries do
        if m and m.valid then occupied = true break end
    end
    local key = "raid_empty_" .. inst
    if occupied then
        eq.delete_data(key)
        return
    end
    local since = tonumber(eq.get_data(key) or "") or 0
    if since == 0 then
        eq.set_data(key, tostring(os.time()))
    elseif os.time() - since >= M.EMPTY_CLOSE_SECS then
        M.close(inst)
    end
end

-- ⚠️ One place that closes a raid, so every marker is cleaned up together. A leftover `raid_of_`
-- would be inherited by whatever claims the recycled id next -- which is what the run token defends
-- against, but leaving litter for the token to catch is not a reason to leave litter.
function M.close(inst)
    eq.delete_data("raid_empty_" .. inst)
    eq.delete_data("raid_dead_" .. inst)
    eq.delete_data("raid_spawned_" .. inst)
    eq.delete_data("raid_of_" .. inst)
    -- ⚠️ Destroy LAST. Destroying an instance somebody is still standing in strands them in a zone
    -- that no longer exists (§24).
    eq.destroy_instance(inst)
end

return M
