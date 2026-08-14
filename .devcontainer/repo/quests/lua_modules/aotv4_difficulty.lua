-- aotv4_difficulty.lua
-- World difficulties: Normal / Nightmare / Hell / Inferno.
--
-- Difficulty is a property of the CHARACTER, not of a destination. You pick one from the /pick
-- window and it then applies to every zone you enter, by putting you in a private INSTANCE of that
-- zone shared with everyone else at the same difficulty. Normal is the real zone with no instance,
-- so the ordinary case costs nothing at all.
--
--   window -> M.send / M.handle_say        ("/say diffwin", "/say diffset <n>")
--   travel -> M.on_enter_zone              (keeps you at your difficulty as you move around)
--   mobs   -> M.on_npc_spawn               (level bump + hp multiple)
--   tomes  -> aotv4_spell_books reads the same `zonediff_<charid>` bucket this module writes
--
-- ⚠️⚠️ THE NATIVE /pick IS NOT USED AND CANNOT BE. `OP_PickZoneWindow` exists and is mapped for RoF2
-- (0x72d8) but `PickZoneEntry_Struct` carries only {zone_id, player_count, instance_id} -- there is
-- no text field, so every difficulty would render as the same zone name with a different population.
-- `Handle_OP_PickZone` is also a bare stub in this codebase and nothing has ever sent the window
-- packet. The dll intercepts "/pick" and opens our own window instead.

local M = {}

-------------------------------------------------------------------- the ladder
-- ⚠️⚠️ A LEVEL BUMP ON ITS OWN CHANGES ALMOST NOTHING, AND THAT WAS A REAL BUG (fixed 2026-08-11).
-- `NPC::SetLevel` (zone/npc.cpp:2253) is three lines: it sets `level` and sends two appearance
-- packets. It does NOT touch damage, AC, attack or resists -- an NPC's damage comes from `min_dmg`
-- and `max_dmg` on its `npc_types` row, not from its level. So bumping the level recoloured the con
-- and changed the hit-chance maths slightly, and a Nightmare creature hit for exactly what a Normal
-- one did. Reported from play as "none of the stats or damage changed on mobs in different
-- difficulties" -- which was accurate.
-- 📌 The Delve does not have this problem because `ScaleNPC` rewrites the whole stat block from
-- `npc_scale_global_base`. We deliberately do not use that here (see M.on_npc_spawn), so every stat
-- that should move has to be named explicitly. **Adding a difficulty means filling in this table.**
--
-- ⚠️ HEALTH IS THE BIG DIAL, DAMAGE THE SMALL ONE. The design is that a harder world costs you more
-- TIME and more resources per fight, not that it deletes you -- a x4 damage creature at the level
-- cap simply one-shots people and the difficulty stops being a choice.
-- ⚠️⚠️ NO NUMBERS IN THE BLURBS, ON PURPOSE. They used to restate the multipliers in prose and the
-- two drifted immediately: Hell read "about half again as hard" while `dmg` was 1.40, and Inferno
-- read "well over half again" at 1.60. Health, levels and damage now go on the wire as their own
-- fields and the window renders them as COLUMNS, straight from this table -- so the figures a player
-- reads are the figures the world applies, and there is nothing left to keep in step by hand.
-- The blurb is flavour; the affixes below carry the specifics.
--
-- ⚠️ DAMAGE ACCELERATES WHERE HEALTH IS LINEAR (2/3/4 against 1.2/1.5/2.1). Health decides how long
-- a fight runs; damage decides whether you survive a mistake. Keeping damage flat across the ladder
-- makes the top tier a slog rather than a threat, and raising it linearly with health makes Inferno
-- one-shot people at the level cap.
M.LEVELS = {
	[0] = { name = "Normal",    hp = 1.0, lvl = 0, dmg = 1.00, ac = 1.00, atk = 1.00, resist = 1.00,
	        blurb = "The world as it is. No instance, no changes." },
	[1] = { name = "Nightmare", hp = 2.0, lvl = 2, dmg = 1.20, ac = 1.15, atk = 1.15, resist = 1.15,
	        blurb = "A harder cut of the world you already know. The gentlest step up." },
	[2] = { name = "Hell",      hp = 3.0, lvl = 4, dmg = 1.50, ac = 1.30, atk = 1.30, resist = 1.30,
	        blurb = "Creatures take real effort to put down, and the worst of them are marked." },
	[3] = { name = "Inferno",   hp = 4.0, lvl = 6, dmg = 2.10, ac = 1.45, atk = 1.45, resist = 1.45,
	        blurb = "The deep end. Everything here hits hard enough to end a careless pull." },
}
M.MAX = 3

-- The AFFIXES -- what is different beyond the numbers. Shown in the window under the description, so
-- a player can see what a difficulty actually does to the world before choosing it.
--
-- ⚠️⚠️ THIS LIST IS THE HONEST ONE: it names only what is BUILT. An aggro radius and mob classes for
-- Inferno, and doubled loot, are still unwired and so are still unlisted. A window that advertises an
-- affix the world does not apply is worse than one that lists nothing.
-- ⚠️ EACH ROW STANDS ALONE, because the tiers do not inherit (see M.TRAITS). Nothing here should ever
-- read "everything the tier below does" again -- if two rows say the same thing, the design has
-- drifted back into a ladder and the table above is what to fix, not this text.
--
-- ⚠️⚠️ AND IT LISTS ONLY WHAT IS BUILT. "Doubles Ink of the Lost" was here and is NOT implemented --
-- `grant_ink_on_kill` has a flat chance with no difficulty awareness -- so it has been removed rather
-- than left advertising something the world does not do. Put it back when the code exists, not before.
--
-- ⚠️ The tome line says "con white or better", not "named": the drop is gated on con (see
-- aotv4_spell_books.CON_REWARDS), which is a different and much broader rule than the named-only gate
-- the Inferno weapon augments are specced for. Naming the wrong gate sends players hunting the wrong
-- creatures.
M.AFFIXES = {
	[0] = {},
	[1] = { "AWAKE: nothing here is fooled by invisibility, sneak or hide",
	        "UNBINDABLE: nothing can be mezzed, charmed, rooted or snared",
	        "Worn Tomes of Insight drop from creatures that con white or better" },
	[2] = { "CHAMPIONS: one creature in five carries a permanent affix -- it glows and is tagged under its name: Armored, Hardened, Mending, Frenzied, Barbed, Draining or Leeching",
	        "WARY: about half of what you pull is not fooled by feigning death",
	        "Etched Tomes of Insight drop instead of Worn" },
	[3] = { "VIGILANT: creatures notice you from twice as far away -- there is no clean approach",
	        "WARY: about three in four are not fooled by feigning death",
	        "Radiant Tomes of Insight drop instead of Etched" },
}

-- ⚠️ Swapping is on a cooldown so difficulty is a decision rather than a per-pull tactic. Without it
-- the play is to clear on Normal and flip to Inferno for the last creature of anything worth a tome.
M.SWAP_COOLDOWN_SECS = 300      -- 5 minutes

-------------------------------------------------------------------- TESTING SCOPE
-- ⚠️⚠️ TEMPORARY: while this is being tested, only these zones offer a difficulty above Normal.
-- Keyed on SHORT NAME. **Empty or nil = every zone**, which is the shipping behaviour -- so lifting
-- the restriction is deleting the one entry below, not unpicking a gate.
--
-- ⚠️⚠️ NORMAL (0) IS ALWAYS ALLOWED, EVERYWHERE, and that is not optional. Normal is the ordinary
-- world rather than a shard, and it is the only way back: gate it and a character who shifted in a
-- listed zone and then walked into an unlisted one could never return to Normal by hand. (The
-- `on_enter_zone` reset would still catch them, so the symptom would be an inconsistent refusal
-- rather than a permanent trap -- which is worse to diagnose, not better.)
--
-- 📌 It costs a zone process per (zone x difficulty) shard, and those are capped by the launcher's
-- `dynamics` and the 7000-7029 port range. Restricting to one zone keeps testing well inside that
-- while the real capacity question is settled separately.
-- ⚠️⚠️ EMPTY MEANS EVERY ZONE, AND THAT IS THE SHIPPING STATE. Opened 2026-08-14.
-- ⚠️⚠️ THERE IS NO PER-ENVIRONMENT SWITCH HERE, AND IT LOOKED LIKE THERE WAS. This module is tracked
-- in /src and ships to live verbatim, so while this table held `unrest` the LIVE server was
-- Unrest-only too -- not just the test container. Anything scoped here is scoped everywhere; if a
-- restriction is ever wanted for testing again, remember it goes out with the next Lua ship.
M.TEST_ZONES = {}

-------------------------------------------------------------------- WHICH TIER DOES WHAT
-- ⚠️⚠️ TIERS DO NOT INHERIT EACH OTHER'S MECHANICS. This table is keyed on the EXACT difficulty, not
-- on a threshold, and every hook looks its traits up here rather than testing `diff >= something`.
--
-- Only the NUMBERS climb (health, levels, damage, AC, attack, resists -- see M.LEVELS). The mechanics
-- are a different thing at each step, so Hell is not "Nightmare and more", it is its own problem:
--   Nightmare  you cannot hide from it, and you cannot hold it still
--   Hell       one creature in five is a champion, and you cannot shake it by feigning
--   Inferno    it sees you coming long before you are ready
--
-- 📌 A trait CAN appear on two tiers, and `feign_resist` does (Hell and Inferno). That is not
-- inheritance -- it is written out on each tier that has it, so the table always reads as the whole
-- truth for a given difficulty and nothing is implied by position. Adding a fourth tier means filling
-- in its row, never "and everything below".
--
-- 📌 That means a HARDER tier can lack something an easier one has -- Hell creatures are mezzable
-- where Nightmare's are not. Deliberate: it makes each difficulty a different fight rather than a
-- strictly worse one, and gives a player a reason to choose Nightmare over Hell for a given camp.
-- Anything that should apply everywhere goes in M.LEVELS, not here.
M.TRAITS = {
	[0] = {},
	[1] = { awake = true,  unbindable = true },
	-- ⚠️ `rising` is DELIBERATELY ABSENT -- no tier enables the rising dead. See M.on_npc_death.
	-- ⚠️ `feign_resist` is a PERCENT PER CREATURE, not a flag -- see M.on_feign_death.
	[2] = { champions = true, feign_resist = 50 },
	[3] = { vigilant = true, feign_resist = 75 },
}

local function traits(diff) return M.TRAITS[diff] or M.TRAITS[0] end

-------------------------------------------------------------------- Nightmare: awake and unbindable
-- ⚠️⚠️ THIS REPLACED "CREATURES NEVER LOSE INTEREST", WHICH IS UNBUILDABLE HERE AND WOULD HAVE BEEN
-- BAD IF IT WERE. There is NO leash-and-return behaviour in this codebase: the only way a creature
-- forgets you is `hate_list.RemoveStaleEntries(600000, npc_aggro_max_dist)` on a periodic timer
-- (mob_ai.cpp:1052). Remove the distance half and a creature does not chase you better -- it holds
-- hate forever, from wherever it got stuck, with nothing to send it home. Corpses and zone-ins
-- accumulate permanent attackers, and because the check is timer driven the behaviour is different
-- tick to tick. Reported before it was ever built, correctly.
--
-- What Nightmare does instead: nothing hides from you, and nothing holds still for you.

-- ⚠️ SnareImmunity (16) covers ROOT **and** snare -- `Mob::IsImmuneToSpell` tests it against both
-- `SpellEffect::Root` and `SpellEffect::MovementSpeed` (spells.cpp:5561). One flag, two schools.
-- ⚠️ Stun and Fear are deliberately NOT included. Stun is a core melee tool and fear is a core
-- Necromancer one; removing those does not make the world scarier, it deletes two classes' kits.
M.CC_IMMUNITIES = {
	13,   -- MesmerizeImmunity
	14,   -- CharmImmunity
	16,   -- SnareImmunity: root and snare together
}

-- Feign Death gets LESS RELIABLE from Hell upward -- it is not switched off. Each creature rolls
-- independently when you feign: `M.TRAITS[diff].feign_resist` percent of them stay on you, the rest
-- forget you as normal. So a feign on Hell sheds about half a camp and on Inferno about a quarter,
-- and the tool keeps working while stopping being an escape hatch.
--
-- ⚠️⚠️ IT IS A PER-CREATURE ROLL, NOT A ROLL ON THE FEIGN ITSELF. The feign always succeeds -- you
-- still drop and melee still stops -- so a Monk never loses the ability to lie down mid-pull; what
-- changes is how much of the room believes it. Failing the feign outright would be a flat nerf to
-- the skill, which is not what a difficulty setting should be.
--
-- ⚠️⚠️ THIS REPLACED A BINARY `FeignDeathImmunity` (special ability 27) SET AT SPAWN, and the flag
-- must NOT come back alongside it: `EntityList::ClearFeignAggro` (entity.cpp:3611) tests that flag
-- FIRST and `continue`s, so a flagged creature never reaches the EVENT_FEIGN_DEATH hook this now
-- rides on. Setting both would silently pin every creature at 100 percent.
--
-- ⚠️ HELL AND INFERNO, NOT NIGHTMARE. Feign is the Monk and Necromancer answer to a pull gone wrong,
-- and Nightmare is meant to be the step you can take without changing how you play -- taking it away
-- there would make the gentlest difficulty the one that hurts two classes most. Both of the upper
-- tiers declare it separately; see the note on M.TRAITS about why that is not inheritance.

-------------------------------------------------------------------- Inferno: vigilant
-- Creatures notice you from much further away. There is no clean approach and no quiet corner of a
-- camp -- you commit to a room rather than to a pull.
--
-- ⚠️⚠️ ONLY THE HORIZONTAL REACH GROWS. Stock compares x, y AND z against the same number, so a wider
-- radius would also mean aggro through the floor of any multi-level dungeon. The spawn hook pins
-- `aggro_z` to the creature's ORIGINAL radius first -- see Mob::GetAggroRangeZ, an AoTv4 addition
-- that falls back to the horizontal radius, so every creature this system has not touched is stock.
-- 📌 That split is why the multiplier can be as generous as x2 without the change becoming invisible
-- and unfair: you can be seen from across a room, never from a floor away.
--
-- 📌 x2 is calibrated against the data rather than picked: the most common authored `aggroradius` is
-- 55 (15,514 creatures) and 100 already exists naturally on 3,083 more, so this puts an ordinary
-- creature roughly where a naturally alert one already sits instead of inventing a new extreme.
M.AGGRO_MULT = 2.0
-- ⚠️ A ceiling, because a few creatures ship with `aggroradius` 10000 (whole-zone aggro) and doubling
-- that is meaningless noise. Well clear of the highest ordinary authored value.
M.AGGRO_CAP  = 250

-------------------------------------------------------------------- Hell: creature affixes
-- Roughly one creature in five spawns carrying a permanent affix -- a champion in all but name. It
-- is visible before you pull (a glow and a renamed mob), so it is information rather than an ambush.
--
-- ⚠️⚠️ THE BEHAVIOURAL AFFIXES ARE PAID IN LUA, NOT BY A SPELL ROW. A damage shield scaled to the
-- creature's own level cannot be expressed in `spells_new` at all -- SPA 59's value is a fixed number
-- on the row, so "half the creature's level" would need one spell row per level. The same is true of
-- the drain and the leech. So the spawn hook only MARKS the creature and the damage hooks do the
-- work, which is the identical pattern the Thirst line uses (section 5).
M.AFFIX_CHANCE = 20     -- percent of creatures that carry one

-- ⚠️⚠️ THE THREE SUSTAIN AFFIXES ARE THE ONES THAT CAN BREAK A FIGHT OUTRIGHT, and the first pass had
-- all three badly overtuned. They are gathered here as named percentages so they can be retuned
-- without reading a line of code. Every figure below is a FIRST GUESS to be corrected in play.
--
-- The rule they all obey: an affix must change how a fight FEELS, never whether it is winnable. A
-- creature that out-heals or out-drains a group is not hard, it is a wall, and the player has no move
-- that beats it.
--
--   DRAIN_PCT   was 50 percent of every hit, from BOTH pools at once. A Hell fight is roughly ten
--               hits taken, so that emptied a caster mid-fight and every fight after it. At 12 it
--               costs about 100 from each pool per fight against a cap-level pool near 1500 -- felt
--               across a camp, survivable inside one.
--   LEECH_PCT   was 50. It has to stay well under group damage or the creature simply cannot be
--               killed. At 20 percent of its own output it heals single digits per second while a
--               group deals hundreds.
--   KNIT_PER_LV was 12 per level, and ⚠️⚠️ `hp_regen` IS PER TICK (6 SECONDS), NOT PER SECOND -- so
--               that was 360 a tick at level 30, i.e. 60 health a second, which outpaces a lot of
--               real groups. It was not "hard", it was unkillable. At 2 per level it is 10 a second:
--               a tax on a long fight, and a real punishment for chipping and walking away.
M.DRAIN_PCT   = 12     -- of each hit, taken from mana AND endurance
M.LEECH_PCT   = 20     -- of the damage it deals, healed back to itself
M.KNIT_PER_LV = 2.0    -- hp_regen per creature level, PER TICK

-- ⚠️ `stat` affixes are applied once at spawn; `mark` affixes are behaviour and are read back by the
-- damage hooks. Never both -- a stat affix costs nothing per hit, and putting a stat one behind a
-- per-hit test would pay for it thousands of times over.
--
-- ⚠️ `key` is the STORED identifier (entity variable, read back by the damage hooks) and `word` is
-- what the player sees. They are separate so the display can be reworded without invalidating the
-- variable on every creature already spawned carrying the old spelling.
-- 📌 `word` is Capitalised because it renders as a tag under the name -- `<Mending>` -- where it
-- reads as a title rather than as part of a sentence.
M.MOB_AFFIXES = {
	{ key = "armored",   word = "Armored",     stat = { ac = 2.0 } },
	{ key = "hardened",  word = "Hardened",    stat = { hp = 2.0 } },
	-- ⚠️ Was "knitting", which read as knitting a jumper rather than knitting a wound. `key` is left
	-- alone deliberately: it is only ever compared against itself, and renaming it would strand the
	-- entity variable on any creature already spawned.
	{ key = "knitting",  word = "Mending",     stat = { regen = M.KNIT_PER_LV } },
	{ key = "frenzied",  word = "Frenzied",    stat = { haste = 0.7 } },   -- attack_delay x0.7
	{ key = "barbed",    word = "Barbed",      mark = true },  -- reflects half its level per melee hit
	{ key = "draining",  word = "Draining",    mark = true },  -- its hits take mana and endurance
	{ key = "leeching",  word = "Leeching",    mark = true },  -- heals itself for a share of its damage
}

-- ⚠️ Entity variable, not a bucket: it describes THIS spawn. A bucket keyed on the npc type would
-- make every future creature of that kind permanently armored, server-wide.
local AFFIX_VAR = "zdiff_affix"

-- ⚠️ 412 is the ONLY nimbus id proven to work in this codebase (section 24, guildhall/#Healer.lua);
-- a wrong id fails silently. All affixes share it -- the glow says "this one is different", and the
-- NAME says which, which is the part a player can actually act on.
local AFFIX_NIMBUS = 412

-- ⚠️ ~100 years, the same figure native zone sharding uses (section 27). A difficulty shard must
-- never expire under the people standing in it, and there is no natural moment to renew it -- unlike
-- a delve, which has a definite end. An unused shard costs a row in `instance_list` and nothing
-- else: with nobody in it the zone process self-terminates like any other dynamic zone.
M.DURATION_SECS = 3155760000

-------------------------------------------------------------------- the rising dead (NO TIER)
-- Tuning for a mechanic NO DIFFICULTY CURRENTLY ENABLES -- see M.on_npc_death. A creature you kill
-- gets straight back up as a skeleton of itself, where it fell, at half health, keeping everything
-- that made it dangerous and losing everything that made it worth killing.
-- ⚠️ This block previously read "(Hell+)" / "On Hell and Inferno", which was never true: Inferno
-- (M.TRAITS[3]) has never carried `rising`. Hell's was removed 2026-08-12.
M.RISE_CHANCE = 25     -- percent
M.RISE_HP_PCT = 50     -- comes back at half health
M.RISE_RACE   = 60     -- Skeleton. Confirmed against the data (a_human_skeleton, a_warbone_monk).

-- ⚠️ Entity variable, not a data bucket: it describes THIS spawn and must die with it. A bucket
-- would be keyed on the npc type and would then suppress the mechanic for every future creature of
-- that kind, server-wide and permanently.
local RISEN_VAR = "zdiff_risen"

-------------------------------------------------------------------- buckets
local function pkey(client) return "zonediff_" .. client:CharacterID() end

-- (zone, difficulty) -> instance id. Global, so every player at a difficulty lands in the SAME copy
-- of the zone rather than each getting their own private world.
local function ikey(zone_id, diff) return string.format("zdiff_i_%d_%d", zone_id, diff) end

-- instance id -> difficulty. The REVERSE index, and it exists purely so a zone process can answer
-- "which difficulty am I?" without knowing its own zone or walking the forward map.
local function rkey(inst) return "zdiff_of_" .. inst end

function M.get(client)
	if not client or not client.valid then return 0 end
	local d = tonumber(eq.get_data(pkey(client))) or 0
	if d < 0 or d > M.MAX then return 0 end
	return d
end

-- "a large rat" -> "large rat", so a prefix can be put in front of it without producing "a armored a
-- large rat". Shared by the affix namer and the rising dead.
--
-- ⚠️ Underscores, ASCII, no apostrophes: TempName goes on the wire as a mob name and the client
-- renders underscores as spaces, so anything else shows up mangled in game rather than failing
-- visibly (section 24). GetCleanName has already stripped '#', digits and apostrophes.
-- ⚠️ The pattern only strips when the article is followed by whitespace, so "Talendor" and "Tunare"
-- are left alone -- an optional-letter pattern that matched a bare leading T would eat proper nouns.
local function bare_name(npc)
	local clean = (npc and npc.valid and npc:GetCleanName()) or "creature"
	return (clean:gsub("^[AaTt]h?n?e?%s+", ""))
end

-------------------------------------------------------------------- instances
-- The instance id for (this zone, this difficulty), creating it if there is not a live one.
--
-- ⚠️⚠️ THE STORED ID MUST BE RE-VALIDATED EVERY TIME, NOT TRUSTED. An instance can be gone while the
-- bucket still names it, and assigning somebody to a dead instance is the silent failure recorded in
-- section 24: world's `VerifyInstanceAlive` fails on zone-in and quietly redirects the player to
-- their safe return, with no error in any log. `get_instance_zone_id_by_id` answers 0 for an
-- instance that no longer exists, which is the cheapest liveness test available.
local function instance_for(zone_id, zone_short, diff)
	if diff == 0 then return 0 end   -- Normal is the real zone

	-- ⚠️⚠️ THE DIFFICULTY MUST BE RE-VALIDATED TOO, NOT JUST THE ZONE -- `here()` round-trips its
	-- answer for exactly this reason and the forward path was missing the same guard. **Instance ids
	-- are RECYCLED here** (`Instances:RecycleInstanceIds`, see the note in `here()`), so a shard that
	-- dies leaves `zdiff_i_<zone>_<diff>` naming an id that something else has since claimed. If the
	-- claimant happens to be a shard of the SAME zone at a DIFFERENT difficulty, the zone test below
	-- passes and this hands back the wrong world.
	--
	-- Observed exactly that on 2026-08-12: `zdiff_i_63_1` and `zdiff_i_63_2` BOTH named instance 105
	-- while `zdiff_of_105` said 1, so asking for Hell in Unrest resolved to the Nightmare shard. That
	-- presented as "it consumes the cooldown and doesn't move me" -- `place()` saw want == have and
	-- correctly declined to move, but the cooldown had already been spent.
	-- ⚠️ `zdiff_of_<id>` is the AUTHORITY on what a shard is; the forward map is only a cache.
	local key = ikey(zone_id, diff)
	local id  = tonumber(eq.get_data(key)) or 0
	if id > 0
		and (eq.get_instance_zone_id_by_id(id) or 0) == zone_id
		and (tonumber(eq.get_data(rkey(id))) or 0) == diff
	then
		return id
	end

	-- ⚠️⚠️ VERSION 0, WHICH IS THE OPPOSITE OF WHAT THE DELVE DOES. Section 24 warns delves never to
	-- use version 0 because that is the ordinary open-world spawn set -- and here the ordinary open
	-- world is exactly what we want, just a private copy of it. A non-zero version would give an
	-- EMPTY zone: `spawn2` rows are filtered `(version = N OR version = -1)`, and world zones define
	-- their spawns at version 0 only.
	local inst = eq.create_instance(zone_short, 0, M.DURATION_SECS)
	if not inst or inst == 0 then return 0 end

	eq.set_data(key, tostring(inst))
	eq.set_data(rkey(inst), tostring(diff))
	return inst
end

-- Which difficulty is the zone THIS process is running? Cached on the instance id.
--
-- ⚠️ Cached because on_npc_spawn runs for every spawn in the zone and a bucket read per mob is real
-- cost. Keyed on the instance rather than set once at load, because a zone process is not guaranteed
-- to serve the same instance for its whole life -- a stale cache would scale an ordinary zone.
local _cache_inst, _cache_diff = -1, 0
local function here()
	local inst = eq.get_zone_instance_id() or 0
	if inst == _cache_inst then return _cache_diff end

	_cache_inst = inst
	_cache_diff = 0
	if inst == 0 then return 0 end          -- the open world is always Normal

	local d = tonumber(eq.get_data(rkey(inst)))
	if not d or d <= 0 then return 0 end

	-- ⚠️⚠️ ROUND TRIP THE ANSWER: the forward map for (this zone, that difficulty) must still name
	-- THIS instance. Trusting `zdiff_of_<inst>` on its own is unsafe because **instance ids are
	-- RECYCLED on this server** -- `Instances:RecycleInstanceIds` is true, and
	-- `Database::TryGetUnusedInstanceID` hands the lowest free id above `ReservedInstances` (100) to
	-- whatever asks next. So a difficulty shard that expires leaves its marker behind, and the next
	-- thing to claim that id inherits it.
	--
	-- The thing most likely to claim it is a DELVE, which would then be scaled a second time on top
	-- of its own player-relative scaling AND would start paying Tome of Insight drops -- creatures
	-- that are endless, tuned to the player, and already paying a chest. Silent in every direction:
	-- no error, and the delve would simply feel wrong.
	--
	-- 📌 This also makes the leftover markers inert rather than dangerous, so nothing has to hunt
	-- them down when an instance dies -- there is no hook for that anyway.
	if (tonumber(eq.get_data(ikey(eq.get_zone_id(), d))) or 0) == inst then
		_cache_diff = d
	end
	return _cache_diff
end
M.here = here

-------------------------------------------------------------------- placing
-- Put this client in the right instance of the zone they are standing in. Returns true if it moved
-- them. Landing coordinates are their CURRENT position, so switching difficulty leaves you exactly
-- where you were standing rather than at the zone-in point.
-- ⚠️ `want` may be passed in by a caller that has ALREADY resolved the shard (M.set does, so it can
-- refuse cleanly before committing any state). Omit it and this resolves it itself, which is what
-- every other caller wants.
local function place(client, want)
	local zone_id  = eq.get_zone_id()
	local zone_sn  = eq.get_zone_short_name()
	local diff     = M.get(client)
	if want == nil then want = instance_for(zone_id, zone_sn, diff) end
	local have     = eq.get_zone_instance_id() or 0

	if want == have then return false end

	-- ⚠️⚠️ REGISTER BEFORE MOVING, ALWAYS. `create_instance` adds nobody to the instance it makes, and
	-- world checks membership on zone-in (`CheckInstanceByCharID`); an unregistered character is
	-- silently redirected to their safe return instead. Section 24 records this presenting as "it
	-- opened and then ported me to the bazaar" with nothing wrong in any log.
	if want > 0 then
		eq.assign_to_instance_by_char_id(want, client:CharacterID())
	end

	client:MovePCInstance(zone_id, want, client:GetX(), client:GetY(), client:GetZ(), client:GetHeading())
	return true
end
M.place = place

-------------------------------------------------------------------- window
function M.send(client)
	if not client or not client.valid then return end
	local cur   = M.get(client)
	local parts = {}
	for i = 0, M.MAX do
		local L = M.LEVELS[i]
		-- ⚠️ The affixes ride in the SAME field as the blurb, separated by "~", rather than as a sixth
		-- pipe field. An older dll splits on '|' into five and would show a raw affix list glued to
		-- the end of the description; this way it shows the description and simply ignores the rest.
		local text = L.blurb
		for _, a in ipairs(M.AFFIXES[i] or {}) do text = text .. "~" .. a end
		-- ⚠️ hp, lvl and dmg go as RAW NUMBERS from the table above. The window renders them as
		-- columns; nothing restates them in prose, which is what let the description and the actual
		-- multiplier drift apart before.
		parts[#parts + 1] = string.format("%d|%s|%.1f|%d|%.2f|%s",
			i, L.name, L.hp, L.lvl, L.dmg, text)
	end
	client:Message(MT.NPCQuestSay, string.format("DIFFDATA %d^%s", cur, table.concat(parts, "^")))
end

-- Change difficulty. Takes effect immediately, by moving you into the matching copy of the zone you
-- are standing in.
function M.set(client, diff)
	if not client or not client.valid then return false end
	if not M.LEVELS[diff] then
		client:Message(MT.Red, "That is not a difficulty.")
		return false
	end

	-- ⚠️ TEMPORARY testing scope -- see M.TEST_ZONES. Refused BEFORE any state is touched, so a
	-- refusal here costs neither the cooldown nor a bucket write. Normal is deliberately exempt.
	if diff > 0 and next(M.TEST_ZONES or {}) ~= nil
		and not M.TEST_ZONES[eq.get_zone_short_name() or ""]
	then
		client:Message(MT.Red, string.format(
			"%s is not available here yet. Difficulties are being tested in The Estate of Unrest.",
			M.LEVELS[diff].name))
		return false
	end

	-- ⚠️⚠️ TESTED AGAINST WHERE THE PLAYER ACTUALLY IS (`here()`), NOT JUST THE BUCKET. Reading the
	-- bucket alone made this refusal LIE and left no way out of it: if the bucket said Hell while the
	-- player stood in the ordinary world -- which is exactly what a failed `place()` below used to
	-- produce -- then every attempt to shift to Hell answered "you are already playing on Hell" and
	-- refused, so the one action that would have fixed the state was the one action being blocked.
	-- Reported from play as "it said I was already there, but I'm not".
	-- 📌 Requiring BOTH to agree makes the mismatch self-healing: a stale bucket now re-places the
	-- player instead of locking them out, and the genuine "already there" case still refuses.
	if diff == M.get(client) and diff == here() then
		client:Message(MT.Yellow, string.format("You are already playing on %s.", M.LEVELS[diff].name))
		return false
	end

	-- ⚠️⚠️ REFUSED IN COMBAT, and for the same reason the delve refuses (section 24): switching
	-- difficulty is a zone change, so without this it is a free escape from any losing fight -- and a
	-- better one than Gate, because it breaks every hate list at once with no cast time and no
	-- reagent. GetAggroCount, not IsEngaged: it counts the creatures holding you on their hate list,
	-- so it stays true while something is chasing you after you have stopped fighting, which is
	-- exactly when the escape is worth the most.
	if (client:GetAggroCount() or 0) > 0 then
		client:Message(MT.Red, "Not while something is hunting you. Break away first.")
		return false
	end

	-- ⚠️⚠️ COOLDOWN, so difficulty is a decision and not a per-pull tactic. Without it the optimal
	-- play is to clear a camp on Normal and flip to Inferno for the last creature of anything that
	-- might carry a tome -- all of the reward, none of the fight.
	-- ⚠️ Checked BEFORE the delve test purely so the message a player sees names the thing they can
	-- actually wait out; both refusals are cheap and the order only affects which one they read.
	local cdk  = "zdiff_cd_" .. client:CharacterID()
	local last = tonumber(eq.get_data(cdk)) or 0
	local left = M.SWAP_COOLDOWN_SECS - (os.time() - last)
	if last > 0 and left > 0 then
		client:Message(MT.Red, string.format(
			"The world is still settling around you. %d second%s before you can shift again.",
			left, (left == 1) and "" or "s"))
		return false
	end

	-- ⚠️⚠️ NEVER WHILE IN A DELVE. A delve is its own instance with its own scaling, its own task and
	-- its own teardown; moving the player out of it from here would strand the run -- the run bucket
	-- would still claim they are inside, and `aotv4_dungeon.on_enter_zone` would then fail the run
	-- for being somewhere else. Leave the delve first.
	local ok_d, dungeon = pcall(require, "aotv4_dungeon")
	if ok_d and dungeon.current_run and dungeon.current_run(client) then
		client:Message(MT.Red, "Not while you are in a delve. Leave it first.")
		return false
	end

	-- ⚠️⚠️ RESOLVE THE SHARD BEFORE COMMITTING ANYTHING -- the §24 gate-order lesson, in the other
	-- direction. `instance_for` returns 0 when `eq.create_instance` fails, and `place()` then sees
	-- want == have == 0 and quietly declines to move anyone. The bucket had ALREADY been written by
	-- that point, so a failed creation left the player standing in the ordinary world while every
	-- read of their difficulty said Hell -- no error, no log line, and the refusal above then locked
	-- them out of correcting it.
	-- ⚠️ Only difficulties above Normal need an instance; Normal IS the real zone, so want == 0 is
	-- the correct answer there rather than a failure.
	local want = instance_for(eq.get_zone_id(), eq.get_zone_short_name(), diff)
	if diff > 0 and want == 0 then
		client:Message(MT.Red, "The world will not shift here. Try again in a moment.")
		return false
	end

	eq.set_data(pkey(client), tostring(diff))
	-- ⚠️ Stamped on EVERY successful shift, including back down to Normal. A free trip home would let
	-- a player alternate Inferno and Normal at will, which is the behaviour the cooldown exists to
	-- stop -- and it would make dropping to Normal the cheapest possible combat escape.
	eq.set_data(cdk, tostring(os.time()))

	client:Message(MT.Yellow, string.format("The world shifts around you. You are now playing on %s.",
		M.LEVELS[diff].name))
	place(client, want)
	M.send(client)
	return true
end

-- "/say diffwin" and "/say diffset <n>". Returns true when it consumed the line.
function M.handle_say(e)
	local msg = (e.message or ""):lower()

	if msg:match("^diffwin%s*$") then
		M.send(e.self)
		return true
	end

	local n = msg:match("^diffset%s+(%d+)%s*$")
	if n then
		M.set(e.self, tonumber(n))
		return true
	end

	return false
end

-------------------------------------------------------------------- hooks
-- ⚠️⚠️ ZONING ALWAYS PUTS YOU BACK ON NORMAL. Difficulty is chosen from inside the zone you intend
-- to fight in, never carried into a new one. Two reasons, and the second is the important one:
--   * it makes the choice deliberate every time rather than a setting you forget you left on and
--     then walk into a zone you cannot survive, and
--   * carrying it would create an instance of EVERY zone you pass through, one per difficulty --
--     and every occupied instance is a zone process on a zone port (section 27). Travelling across
--     the world on Inferno would boot a shard for each hop.
--
-- ⚠️⚠️ THE TEST IS "AM I IN THE OPEN WORLD WHILE SET TO SOMETHING ELSE", AND THAT IS WHAT MAKES IT
-- SAFE AGAINST ITS OWN MOVE. Swapping difficulty is itself a zone-in, so this hook fires on the
-- arrival that `place` just caused -- but that arrival lands in a NON-ZERO instance, so the reset
-- cannot trigger and there is no loop. A zone line, by contrast, always lands you in instance 0.
-- Anything added here that resets while standing in a shard will bounce the player forever.
--
-- ⚠️ No move is needed and none is made: Normal IS instance 0, and instance 0 is exactly where a
-- zone line has already put them. Only the bucket changes.
--
-- 📌 A relog inside a shard is deliberately NOT reset -- you arrive in your own instance, which is
-- not the open world, so the test does not fire and you carry on where you were standing. If that
-- instance has since expired, world drops you at your safe return in instance 0 and the reset then
-- fires normally, which is the graceful end of that case.
function M.on_enter_zone(e)
	local c = e.self
	if not c or not c.valid then return end

	local diff = M.get(c)
	local inst = eq.get_zone_instance_id() or 0

	if inst ~= 0 then
		-- In a shard (or a delve). ⚠️⚠️ THE SHARD IS THE AUTHORITY, NOT THE BUCKET. Instance ids are
		-- RECYCLED here, so a shard can be re-tagged under a player between one visit and the next --
		-- observed 2026-08-12, when an emptied Nightmare shard was purged and its id handed straight
		-- back to a new Hell shard. Reconciling to `here()` means the answer always describes the
		-- world the player is standing in rather than the last thing they asked for.
		-- ⚠️ `here()` returns 0 for a delve or any instance this system did not tag, and the bucket is
		-- deliberately left alone in that case -- a delve must not clear a player's difficulty.
		local actual = here()
		if actual > 0 and actual ~= diff then
			eq.set_data(pkey(c), tostring(actual))
		end
		M.send(c)
		return
	end

	if diff ~= 0 then
		eq.set_data(pkey(c), "0")
		c:Message(MT.Yellow, "You step into the ordinary world. Use /pick to choose a difficulty here.")
	end

	-- ⚠️⚠️ ALWAYS RE-SEND ON ARRIVAL. `M.set` sends DIFFDATA from the zone you are LEAVING, and then
	-- moves you -- so the window a player reads after the move still shows the world they came from.
	-- Reported as "it shows I'm in Normal" while standing in a shard. Nothing else pushes DIFFDATA on
	-- a zone change, so without this the window is stale from the moment it matters most.
	M.send(c)
end

-- Make the creatures in a difficulty shard harder. Runs for EVERY npc spawn on the server, so the
-- instance test is first and the common case pays one integer compare.
--
-- ⚠️⚠️ IT DOES NOT USE ScaleNPC, DELIBERATELY. `ScaleNPC` rewrites a creature's stats wholesale from
-- `npc_scale_global_base` (section 24), which is right for a delve -- where every mob is meant to
-- become a generic creature of the layer's level -- and destructive here: it would flatten every
-- hand-authored named mob and boss in the world into a stat-block of its level. Bumping level and
-- health leaves what makes a creature itself intact.
function M.on_npc_spawn(e)
	local diff = here()
	if diff == 0 then return end

	local npc = e.self
	if not npc or not npc.valid then return end

	-- ⚠️ Skip anything AoTv4 spawned (2000000+) and anything owned. The delve records the same rule
	-- for its own rescale sweep: our NPCs are scaled by whatever spawned them, against rules this
	-- knows nothing about, and doubling a player's pet health is not a difficulty setting.
	if (npc:GetNPCTypeID() or 0) >= 2000000 then return end
	if (npc:GetOwnerID() or 0) > 0 then return end

	local L = M.LEVELS[diff]
	if not L then return end

	-- ⚠️ A one-line helper rather than six copies: every one of these reads a current value, scales
	-- it and writes it back, and the only thing that differs is the key. `math.max(1, ...)` because a
	-- stat that rounds to zero is not "slightly weaker", it is broken -- a 0 max_dmg creature cannot
	-- hit at all, and 0 AC is not what a HARDER world is supposed to mean.
	local function scale(key, cur, mult)
		if mult == 1.0 or not cur or cur <= 0 then return end
		npc:ModifyNPCStat(key, tostring(math.max(1, math.floor(cur * mult))))
	end

	-- Level first: it is the cheapest and nothing below depends on it.
	-- ⚠️ It is mostly COSMETIC on an NPC -- see the note on M.LEVELS. It is still worth setting: it
	-- drives the con colour, so a player can SEE that a creature is out of their league, and it feeds
	-- the hit-chance and resist maths even though it moves no stat directly.
	if L.lvl > 0 then
		npc:ModifyNPCStat("level", tostring((npc:GetLevel() or 1) + L.lvl))
	end

	-- What the creature actually does. Damage is the one a player feels immediately; AC and attack
	-- decide how long the fight runs in each direction.
	scale("min_hit",  npc:GetMinDMG(),  L.dmg)
	scale("max_hit",  npc:GetMaxDMG(),  L.dmg)
	scale("ac",       npc:GetAC(),      L.ac)
	scale("atk",      npc:GetATK(),     L.atk)
	scale("accuracy", npc:GetAccuracyRating(), L.atk)
	scale("avoidance", npc:GetAvoidanceRating(), L.ac)

	-- Resists, so a caster feels the difficulty too. Without these a harder world is purely a melee
	-- statement and every spell lands exactly as often as it did on Normal.
	if L.resist ~= 1.0 then
		for _, r in ipairs({ "mr", "fr", "cr", "pr", "dr" }) do
			scale(r, npc:GetNPCStat(r), L.resist)
		end
	end

	-- ---------------------------------------------------------------- Nightmare and above
	if traits(diff).awake then
		-- Nothing hides from you.
		npc:ModifyNPCStat("see_invis",          "1")
		npc:ModifyNPCStat("see_invis_undead",   "1")
		npc:ModifyNPCStat("see_hide",           "1")
		npc:ModifyNPCStat("see_improved_hide",  "1")

		-- ...and nothing holds still for you.
		-- ⚠️⚠️ SetSpecialAbility, NOT ModifyNPCStat("special_abilities", ...). That key REPLACES the
		-- creature's whole special-ability set from a packed string, so it would silently wipe
		-- whatever it natively had -- summon, rampage, immunities, everything.
		for _, ability in ipairs(M.CC_IMMUNITIES) do
			npc:SetSpecialAbility(ability, 1)
		end
	end

	-- ---------------------------------------------------------------- Inferno: vigilant
	-- ⚠️ Read the CURRENT value rather than writing a flat one: authored aggro radii span 1 to 10000
	-- and the common ones alone run 40 to 100, so a fixed number would make some creatures markedly
	-- LESS alert than they already are.
	if traits(diff).vigilant then
		local cur = npc:GetNPCStat("aggro") or 0
		if cur > 0 then
			-- ⚠️⚠️ PIN THE VERTICAL RANGE FIRST, AT THE ORIGINAL VALUE. Stock compares x, y AND z
			-- against the one aggro radius (aggro.cpp:433), so widening the radius would also widen
			-- how far ABOVE and BELOW itself a creature can notice you -- aggro through the floor in
			-- any multi-level dungeon. `aggro_z` is an AoTv4 addition (zone/npc.cpp) backed by
			-- Mob::pAggroRangeZ; -1 there means "follow the horizontal radius", which is stock.
			-- ⚠️ BEFORE the widen, not after: it has to record what the creature had, not what we are
			-- about to give it.
			npc:ModifyNPCStat("aggro_z", tostring(cur))
			npc:ModifyNPCStat("aggro", tostring(math.floor(math.min(M.AGGRO_CAP, cur * M.AGGRO_MULT))))
		end
	end

	-- ⚠️ Feign resistance is NOT applied here. It is a per-creature roll taken when the player actually
	-- feigns (M.on_feign_death); the `FeignDeathImmunity` special ability that used to be set on this
	-- line would short-circuit that hook entirely. See the note beside M.FEIGN_RESIST_DEFAULT.

	-- ---------------------------------------------------------------- Hell and above: affixes
	local affix = nil
	if traits(diff).champions and math.random() * 100.0 < M.AFFIX_CHANCE then
		affix = M.MOB_AFFIXES[math.random(#M.MOB_AFFIXES)]
		npc:SetEntityVariable(AFFIX_VAR, affix.key)
		npc:AddNimbusEffect(AFFIX_NIMBUS)

		-- ⚠️⚠️ A TAG UNDER THE NAME, NOT A RENAME. `ChangeLastName` drives the same line a player's
		-- guild renders on (OP_GMLastName), so a champion reads as "a gnoll pup" with `<Barbed>`
		-- beneath it -- which is legible at a glance and, unlike a rename, leaves the creature's real
		-- name intact for hails, quests, tracking and `#peekloot`.
		-- ⚠️ It replaced `TempName("a barbed gnoll pup")`, which mangled the name itself: TempName
		-- needs underscores for the wire, so every champion also lost the spacing in its own name.
		-- ⚠️⚠️ THIS REQUIRES THE AoTv4 CHANGE TO `NPC::ChangeLastName` (zone/npc.cpp) THAT MAKES IT
		-- PERSIST. Stock only broadcasts a packet to whoever is already in range, and a shard
		-- populates BEFORE the first player finishes zoning in -- so on stock this tags nobody.
		npc:ChangeLastName(affix.word)

		local st = affix.stat
		if st then
			if st.ac    then scale("ac",           npc:GetAC(),        st.ac) end
			if st.regen then npc:ModifyNPCStat("hp_regen", tostring(math.floor((npc:GetLevel() or 1) * st.regen))) end
			if st.haste then scale("attack_delay", npc:GetNPCStat("attack_delay"), st.haste) end
			-- ⚠️ `hp` is applied below with the difficulty multiple, not here -- two separate
			-- ModifyNPCStat("max_hp") calls would each run CalcMaxHP and the second would be computed
			-- from the first's result, compounding by accident.
		end
	end

	-- ⚠️⚠️ `max_hp` CLAMPS DOWN ONLY -- raising the maximum does not raise current health, so without
	-- the explicit SetHP the creature spawns at its old health against a new, much larger maximum and
	-- dies in the same number of hits as before. Same asymmetry section 24 records for `max_mana` on
	-- the delve warden.
	-- ⚠️ LAST, and ONE call: ModifyNPCStat("max_hp") runs CalcMaxHP(), so the difficulty multiple and
	-- the "hardened" affix are multiplied together first and written once.
	local hp_mult = L.hp * ((affix and affix.stat and affix.stat.hp) or 1.0)
	if hp_mult > 1.0 then
		npc:ModifyNPCStat("max_hp", tostring(math.floor((npc:GetMaxHP() or 1) * hp_mult)))
		npc:SetHP(npc:GetMaxHP())
	end
end

-------------------------------------------------------------------- affix behaviour
-- The three affixes that cannot be a stat. Each is a couple of lines, and each runs on a hook that
-- fires for EVERY damage event on the server -- so both handlers early-out before doing anything
-- that touches an entity variable.

-- A player hit a creature. `barbed` reflects half the creature's level back at them.
--
-- ⚠️ Melee only: `e.spell_id` is set for spell damage, and a damage shield that fires on spells is
-- not a damage shield, it is a tax on casting.
function M.on_damage_given(e)
	if not traits(here()).champions then return end
	local dmg = e.damage or 0
	if dmg <= 0 then return end
	if e.spell_id and e.spell_id > 0 and e.spell_id < 65535 then return end

	local npc = e.other
	if not npc or not npc.valid or npc:IsClient() then return end
	if npc:GetEntityVariable(AFFIX_VAR) ~= "barbed" then return end

	local back = math.floor((npc:GetLevel() or 2) / 2)
	if back > 0 then e.self:Damage(npc, back, 0, 4, false) end
end

-- A creature hit a player. `draining` takes resources, `leeching` heals the creature.
--
-- ⚠️⚠️ IT MUST RETURN NOTHING. `event_damage_taken`'s return value is used as a DAMAGE OVERRIDE
-- (zone/attack.cpp:4404) -- negative negates the hit, positive replaces it. Returning a number here
-- would silently rewrite every hit a player takes in Hell.
-- Feign Death, per creature. Called once for EACH creature currently holding the feigning player on
-- its hate list; returning non-zero makes `EntityList::ClearFeignAggro` skip that one, so it keeps
-- its hate and is on the player the moment they stand.
--
-- ⚠️⚠️ THE RETURN VALUE IS THE WHOLE MECHANISM, and it must be returned all the way out through
-- `global_player.event_feign_death` -- the engine reads the script's return, so a handler that rolls
-- correctly and then falls off the end returns nil, which is zero, which means "forget the player".
-- That failure is silent and looks exactly like the difficulty not being applied.
--
-- ⚠️ Rolled per creature rather than once per feign, so a feign on Hell sheds roughly half a camp
-- instead of either working completely or failing completely. A single roll for the whole room would
-- make the outcome all-or-nothing and far swingier than the percentage suggests.
--
-- 📌 `here()` is the difficulty of the ZONE the feign happens in, which is the right reference: the
-- creature and the player are necessarily in the same instance for it to be on the hate list at all.
function M.on_feign_death(e)
	local pct = traits(here()).feign_resist or 0
	if pct <= 0 then return 0 end

	local npc = e.other
	if not npc or not npc.valid then return 0 end

	if math.random() * 100.0 < pct then
		return 1   -- this one is not fooled
	end
	return 0
end

function M.on_damage_taken(e)
	if not traits(here()).champions then return end
	local dmg = e.damage or 0
	if dmg <= 0 then return end

	local npc = e.other
	if not npc or not npc.valid or npc:IsClient() then return end
	local key = npc:GetEntityVariable(AFFIX_VAR)
	if key ~= "draining" and key ~= "leeching" then return end

	local c = e.self
	if not c or not c.valid or not c:IsClient() then return end

	if key == "draining" then
		-- A share of the hit, taken out of both pools. The explicit floor is belt and braces --
		-- `Client::SetMana` and `SetEndurance` already clamp to 0..max themselves -- but it costs
		-- nothing and states the intent.
		-- 📌 These two DO tell the client: `Client::SetMana` ends in `CheckManaEndUpdate()`, so the
		-- bars visibly move. That is the whole difference from the leech below.
		local take = math.max(1, math.floor(dmg * M.DRAIN_PCT / 100))
		c:SetMana(math.max(0, (c:GetMana() or 0) - take))
		c:SetEndurance(math.max(0, (c:GetEndurance() or 0) - take))
	else
		-- ⚠️⚠️ `HealDamage`, NOT `SetHP` -- `Mob::SetHP` (zone/spells.cpp:7810) WRITES `current_hp`
		-- AND SENDS THE CLIENT NOTHING. There is no `SendHPUpdate()` in it, and no Lua binding for
		-- that function, so the leech genuinely happened and the creature's health bar did not move
		-- until the next damage packet refreshed it. Reported from play as "there is no indication
		-- he is taking hp back", which was true of the DISPLAY and not of the heal.
		-- `Mob::HealDamage` (zone/attack.cpp:5295) does `SetHP` and then `SendHPUpdate()`, which is
		-- exactly the pair that was needed, and it clamps to max internally so the hand-rolled
		-- min() went with it.
		-- ⚠️ Called with NO caster on purpose: the caster argument drives a heal message gated on
		-- `Spells:HealAmountMessageFilterThreshold`, which would word it as a heal landing on the
		-- creature and read as though something were healing it. We say it ourselves instead.
		-- 📌 It stays SMALL by design -- 20 percent of the creature's own hit, single digits on a
		-- level 13 creature. It was never meant to be felt through the health bar; the message is
		-- how a player knows it is happening, which is why raising M.LEECH_PCT is NOT the fix for
		-- "I could not tell". The ceiling on that number is group damage: above it the creature
		-- simply cannot be killed.
		local heal = math.max(1, math.floor(dmg * M.LEECH_PCT / 100))
		npc:HealDamage(heal)

		-- ⚠️⚠️ FILTERED, NOT A PLAIN Message -- THIS FIRES ON EVERY MELEE HIT YOU TAKE. That is the
		-- same shape as a damage shield, so it goes on the same filter a player already uses to mute
		-- one (`Filter.DamageShields`, 18): visible by default, and mutable without touching the
		-- rest of their combat log. An unfilterable line here is a message per swing per champion,
		-- which in a camp with two of them is most of the chat window.
		-- ⚠️ `Filter` and `MT` are registered side by side (`lua_register_filters` /
		-- `lua_register_message_types`, zone/lua_parser.cpp:1375), but NOTHING else in this quest
		-- tree used `Filter.` before this, so the literal 18 is kept as a fallback -- a nil there
		-- would error on the first hit taken in Hell rather than at load, which is the worst place
		-- for it. 18 is `FilterDamageShields` (common/eq_constants.h:751).
		-- ⚠️ The SENDER is the creature, not the player: FilteredMessage keys the filter off it.
		c:FilteredMessage(npc, MT.NonMelee, (Filter and Filter.DamageShields) or 18,
			string.format("%s draws %d hit point%s of life from you.",
				npc:GetCleanName() or "Something", heal, (heal == 1) and "" or "s"))
	end
end

-- The rising dead. A kill gets straight back up, where it fell, as a SKELETON of itself at half
-- health -- keeping everything that made it dangerous and none of what made it worth killing.
--
-- ⚠️⚠️ NO TIER ENABLES THIS. `rising` was removed from Hell (M.TRAITS[2]) on 2026-08-12 by owner
-- decision, so `traits(diff).rising` is false everywhere and this function returns at its first line
-- on every kill in the game. It is kept, not deleted, because the mechanic is complete and the cost
-- of holding it is one early return -- but it is DEAD CODE until some tier sets `rising` again.
-- ⚠️ If it is ever re-enabled, put its line back in M.AFFIXES for that tier: the window advertises
-- only what is built, and the reverse trap (a built mechanic nobody is told about) is just as bad.
--
-- ⚠️⚠️ IT IS A FRESH SPAWN OF THE SAME `npc_types` ROW, WHICH IS WHY IT "INHERITS THE STATS" AT ALL.
-- Nothing is copied by hand: spawning the same type reproduces its level, damage, resists, special
-- attacks, spell list and size exactly, and `on_npc_spawn` above then applies the same difficulty
-- scaling the original got. A hand-built copy would have to be kept in step with every column that
-- matters, forever, and would silently drift the first time one was added.
--
-- ⚠️ Only the APPEARANCE is overridden. ChangeRace swaps the model and leaves the stat block alone.
function M.on_npc_death(e, killer)
	local diff = here()
	if not traits(diff).rising then return end

	local npc = e.self
	if not npc or not npc.valid then return end

	-- ⚠️⚠️ A RISEN CREATURE NEVER RISES AGAIN. Without this a single kill is an unbounded chain --
	-- 25 percent each time, so roughly one corpse in 64 would still be getting up on its fourth
	-- body, and a bad streak in a crowded room is an unkillable stack of skeletons.
	if (npc:GetEntityVariable(RISEN_VAR) or "") == "1" then return end

	-- Same two exclusions as the scaling hook: anything AoTv4 spawned, and anything owned. Raising a
	-- player's dead pet as a hostile skeleton is not a difficulty setting.
	if (npc:GetNPCTypeID() or 0) >= 2000000 then return end
	if (npc:GetOwnerID() or 0) > 0 then return end

	if math.random() * 100.0 >= M.RISE_CHANCE then return end

	-- ⚠️ `eq.spawn2` returns a Lua_Mob, NOT a Lua_NPC -- every method used below is bound on Lua_NPC
	-- only, so without the cast this is "attempt to call method (a nil value)" at runtime with
	-- nothing to warn you at write time (CLAUDE.md section 26).
	local mob = eq.spawn2(npc:GetNPCTypeID(), 0, 0, npc:GetX(), npc:GetY(), npc:GetZ(), npc:GetHeading())
	if not mob or not mob.valid then return end
	local risen = mob:CastToNPC()
	if not risen or not risen.valid then return end

	-- ⚠️ MARK IT FIRST, before anything else can go wrong. Everything below is cosmetic or corrective;
	-- this is the one line whose absence produces an endless chain.
	risen:SetEntityVariable(RISEN_VAR, "1")

	-- ⚠️⚠️ ZEROING THE LOOT TABLE IS WHAT STOPS THE DROP -- `ClearItemList` ALONE WOULD NOT. Under
	-- individual loot the table is re-rolled at DEATH, per eligible player, from `GetLoottableID()`
	-- (attack.cpp), so clearing the item list at spawn empties a list that is about to be refilled.
	-- Both are still done: the id stops the death roll, the clear catches whatever global loot placed
	-- at spawn.
	--
	-- 📌 THE DELVE SOLVES THE SAME PROBLEM A DIFFERENT WAY and both are correct -- it sets a
	-- `delve_noloot` entity variable that `NPC::Death` checks explicitly (attack.cpp:3219), chosen
	-- because it also survives anything that re-reads the npc_types row. Zeroing the id works here
	-- because the guard's own `GetLoottableID() &&` short-circuits on it, and the no-looter fallback
	-- calls `AddLootTable(0)`, which returns immediately since no table 0 exists.
	--
	-- 📌 This is why Hell doubles the AMOUNT of loot rather than the number of chances at it: a
	-- corpse that can be killed twice must not be able to pay twice.
	risen:ModifyNPCStat("loottable_id", "0")
	risen:ClearItemList()
	risen:RemoveCash()

	-- ⚠️ Experience is deliberately left alone -- a risen creature is a real fight and pays for
	-- itself. It needs no code: a normal spawn of a normal type is worth what it is worth.
	risen:ChangeRace(M.RISE_RACE)

	-- ⚠️ Set health AFTER the spawn hook has run. `on_npc_spawn` finishes with SetHP(GetMaxHP()) on
	-- the freshly multiplied maximum, so writing half of it any earlier would simply be overwritten.
	risen:SetHP(math.max(1, math.floor((risen:GetMaxHP() or 1) * M.RISE_HP_PCT / 100)))

	risen:TempName(("a risen " .. bare_name(npc)):gsub("%s+", "_"))

	-- It gets up angry. Without this it stands inert until something wanders into its aggro radius,
	-- which reads as a decoration rather than as a second fight.
	if killer and killer.valid then
		risen:AddToHateList(killer, 1)
		if killer:IsClient() then
			local c = eq.get_entity_list():GetClientByID(killer:GetID())
			if c and c.valid then
				c:Message(MT.Red, "The corpse stirs, and gets back up.")
			end
		end
	end
end

return M
