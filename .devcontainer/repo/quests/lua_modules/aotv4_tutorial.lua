-- aotv4_tutorial.lua -- the Titan Hall induction chain.
--
-- Ten NPCs in freeporttheater, one per button on the /aot menu, each handing out a journal task that
-- makes the player USE that window once. NOT a chain -- any lesson may be taken at any time, in any
-- order; `M.UMBRELLA_TASK` is the journal record that tracks which are still outstanding.
--
-- ⚠️ All eleven are `TaskType::Quest` (2), never Task (0). Type 0 allows exactly ONE active task per
-- character (`common/tasks.h:193`), which silently made every lesson unacceptable the moment the
-- record occupied that slot -- and blocked delve entry too, since delve tasks are type 0 as well.
-- ⚠️ They RESET on death (`M.reset_on_death`) and, apart from the reroll cut, PAY AGAIN -- the death
-- that clears them also destroys what they gave. See the ledger note above `pay`.
--
-- ⚠️⚠️ A TASK CANNOT SEE A WINDOW OPEN. `TaskActivityType` has Deliver/Kill/Loot/SpeakWith/Explore/
-- Touch and so on -- there is no "used a UI window" type, and there cannot be, because our windows are
-- chat-protocol overlays that live outside the task system entirely. Every objective here is therefore
-- an inert **Touch** activity that nothing can complete by accident, driven to completion by
-- `client:UpdateTaskActivity(task, 0, 1)` from `M.mark` when the matching /say arrives.
--
-- ⚠️⚠️ AND ONLY SEVEN OF THE TEN REACH LUA. `Client::ChannelMessageReceived` intercepts the AdvLoot
-- (`als*`) and Autoskill (`ask*`) commands at zone/client.cpp:1211 and :1216 and RETURNS TRUE, which
-- swallows the line before EVENT_SAY ever fires; `#ach` is a command, not a say. Those three are
-- marked from C++ instead -- see `Client::AoTv4TutorialMark`. If that call is ever removed, those
-- three steps become uncompletable and NOTHING reports it.
--
-- 📌 No kill objectives anywhere, by design: the hall has no creatures and the induction is meant to
-- be finishable the moment a character first arrives.

-- ⚠️ Both are required ONLY for a constant each, and both are already loaded by global_player, so
-- `require` returns the cached table and this costs nothing. Reading them beats copying: section 26
-- records the augment tier blocks as already mirrored in three places, and section 3 records the
-- reroll counter as owned by spell_choice.
local spell_choice = require("spell_choice")    -- owns the reroll counter  (step 9's reward)
local dungeon      = require("aotv4_dungeon")   -- owns the augment id blocks (step 7's reward)
local aa_choice    = require("aa_choice")       -- owns the AA bank (step 6 seeds one point)

local M = {}

M.FIRST_NPC = 2000620
M.FIRST_TASK = 2000600
M.STEPS = 11

-- step -> the key `M.mark` expects. Order matches npc 2000620+n / task 2000600+n.
M.KEYS = {
	[0] = "spells", [1] = "autoskill", [2] = "advloot", [3] = "trader", [4] = "achievements",
	[5] = "allaclone", [6] = "deathbook", [7] = "delve", [8] = "difficulty", [9] = "travel",
	[10] = "fellowship",
}

-- ⚠️⚠️ STEP 10 WOULD LAND ON THE UMBRELLA TASK, SO THE SEQUENCE SKIPS IT. `M.FIRST_TASK + 10` is
-- 2000610, which is `M.UMBRELLA_TASK` -- the induction record itself. Left alone, the fellowship
-- lesson and the record would be the same task id: assigning one would assign the other, and
-- completing the lesson would fire the umbrella's completion reward. Migration v84 puts the lesson
-- at 2000611 to match this, and the two MUST agree.
local function task_of(step)
	local t = M.FIRST_TASK + step
	return t >= M.UMBRELLA_TASK and t + 1 or t
end

-- ⚠️⚠️ THE INVERSE OF `task_of`, AND IT MUST STAY ITS EXACT MIRROR. `on_task_complete` used to invert
-- it by plain subtraction (`task_id - M.FIRST_TASK`), which silently ignored the umbrella skip above:
-- the fellowship lesson is task 2000611, so that gave step **11**, the guard `step >= M.STEPS` (11)
-- rejected it, and the handler returned false. The consequences were both invisible -- no reward was
-- paid for the lesson, and, far worse, the step was never mirrored onto the induction record, so
-- **The Titan Hall Induction could never be completed by anyone**. Reported from play as finishing
-- Denara's quest and getting no completion on the Induction.
-- 📌 Nothing failed and nothing logged: returning false is exactly how this handler declines a task
-- belonging to the delve, so a real step taking the same exit is indistinguishable from normal.
-- ⚠️ Any future task inserted into the band needs the same treatment in BOTH directions.
local function step_of_task(task_id)
	local step = task_id - M.FIRST_TASK
	if task_id > M.UMBRELLA_TASK then step = step - 1 end
	return step
end

-- ⚠️⚠️ DENARA PREDATES THE HALL NUMBERING and is NOT `M.FIRST_NPC + 10`. The other ten NPCs were
-- created as a block; she was built at 2000410 for the fellowship work and already stands in the
-- Hall, so she is mapped rather than renumbered -- moving a live NPC id would orphan her spawn row
-- and her script for no gain.
-- 📌 Nothing calls this today; each NPC script passes its step as a literal. It is kept because it
-- is the only written-down statement of the mapping, so it must stay correct.
local function npc_of(step)
	if step == 10 then return 2000410 end
	return M.FIRST_NPC + step
end

-- ---------------------------------------------------------------- the induction record
-- ⚠️⚠️ THE TEN ARE NOT A CHAIN. They were, and the ordering was removed deliberately: a player who
-- wants the Trader window should not have to sit through five other lessons to reach it, and a
-- returning character usually needs one specific tab rather than all ten. Every NPC offers its task
-- to anyone, at any time, in any order.
--
-- What replaced the chain is this umbrella task -- ONE journal entry holding ten activities, one per
-- Hall NPC, each naming who to find. That is what tells a player who is still owed a visit; with the
-- chain gone there is otherwise nothing on screen that knows the set exists.
--
-- ⚠️⚠️ ALL TEN OF ITS ACTIVITIES MUST CARRY A NON-ZERO `step`, AND THE SAME ONE. `tasks.h:398` reads
-- "if all steps are 0 treat each as a separate step" -- so a table of zeroes silently puts the task
-- back into SEQUENCE mode and shows exactly one activity at a time, which is the chain again by
-- another route. They ship at step 1, matching the individual tasks, with `req_activity_id = -1`.
M.UMBRELLA_TASK = 2000610

-- ⚠️ Activity index == step index == `npc id - M.FIRST_NPC`. One mapping, used in three places, so a
-- reordering of the NPCs must move all of them together.
function M.ensure_umbrella(client)
	if not client then return end
	if client:IsTaskActive(M.UMBRELLA_TASK) or client:IsTaskCompleted(M.UMBRELLA_TASK) then return end

	client:AssignTask(M.UMBRELLA_TASK, 0, false)

	-- ⚠️⚠️ BACKFILL WHAT IS ALREADY DONE. A character who finished individual steps before this task
	-- existed -- or before they first walked into the Hall -- would otherwise be asked to repeat them,
	-- and repeating is impossible because each individual task is `repeatable = 0`. Without this the
	-- record could never be completed by anyone who had started the old chain.
	for step = 0, M.STEPS - 1 do
		if client:IsTaskCompleted(task_of(step)) then
			client:UpdateTaskActivity(M.UMBRELLA_TASK, step, 1)
		end
	end
end

-- Arrival in the hub hands out the record, so the set is discoverable without hailing anybody first.
-- ⚠️ Runs on EVERY zone-in server wide, so the zone test comes before anything that touches the
-- client: a string compare for everyone, and real work only for someone standing in the Hall.
-- The arrival point for anyone who has not finished the induction. /loc gave Y,X,Z; stored x,y,z.
M.ZONE       = "freeporttheater"
M.ZONE_ID    = 390
M.DROP       = { x = -71.56, y = -246.67, z = -27.10, h = 0 }

function M.on_enter_zone(e)
	if (eq.get_zone_short_name() or "") ~= M.ZONE then return end
	local c = e.self
	if not c then return end

	M.ensure_umbrella(c)

	-- ⚠️⚠️ A NEWCOMER IS PUT ON THE INDUCTION DOORSTEP, EXACTLY ONCE. On a character's first ever
	-- arrival they are moved to the arrival point, so they land where the teachers stand rather than
	-- wherever a delve exit happened to leave them. Every arrival after that is left alone.
	-- 📌 Largely a safety net now: since v88 the PoK book route and Origin both land on this same
	-- authored point, so the only arrival this really catches is one by some other means.
	-- ⚠️ MovePC, not MovePCInstance: this is a move WITHIN the zone the player just entered, so there
	-- is no instance to target and no second zone-in -- an in-zone move does not re-raise
	-- `event_enter_zone`, which is what stops this looping on itself.
	-- ⚠️⚠️ DEFERRED, NEVER CALLED HERE. `MovePC` to the SAME zone from inside `event_enter_zone`
	-- fires while the client is still completing its zone-in handshake, and the client never
	-- finishes entering -- it connects to the zone port, the zone accepts it, and nothing more
	-- happens. Reported from play as "I cant log in to the zone", and it only bites characters whose
	-- SAVED location is already this zone, which is every returning hub player.
	-- ⚠️ A one-shot timer is the established way out: `global_player` already defers `skillsync` two
	-- seconds for the same class of reason. By the time it fires the client is fully in and an
	-- in-zone move is just a move.
	-- ⚠️⚠️ ONCE PER CHARACTER, NOT ONCE PER ZONE-IN. This used to fire on any arrival until the
	-- record was COMPLETE, and a login is an arrival -- so every returning player who had not yet
	-- finished all eleven lessons was teleported to the doorstep on every single login, and again
	-- after every trip out of the zone and back. Reported from play as "every time I log in it ports
	-- me back to the PoK book" -- which it did, because the arrival point sits a few units from it.
	-- 📌 It is a FIRST-ARRIVAL courtesy: show a new player where the teachers stand. Completing the
	-- induction was the wrong test for that -- somebody who has seen the hall once does not need
	-- showing again, whether or not they ever finish the lessons.
	-- ⚠️ Deliberately NOT reset by death, unlike the lessons themselves (`M.reset_on_death`). Death
	-- respawns at the bind point, not here, and a returning player has already been oriented.
	local seen = "hallseen_" .. c:CharacterID()
	if (eq.get_data(seen) or "") == "" then
		eq.set_data(seen, "1")
		c:SetTimer("halldrop", 2)
	end
end

-- Fired by the one-shot timer set in M.on_enter_zone, once the client is properly in the zone.
-- ⚠️ StopTimer FIRST -- a client timer REPEATS, and without this it would drag the player back to
-- the doorstep every two seconds for the rest of the session.
function M.on_drop_timer(client)
	if not client then return end
	client:StopTimer("halldrop")
	if (eq.get_zone_short_name() or "") ~= M.ZONE then return end
	-- ⚠️ No second gate here on purpose: whether to drop at all is decided ONCE in M.on_enter_zone
	-- and recorded there. Re-testing it in the timer would be a second source of truth for the same
	-- question, and it is the sort that drifts.
	client:MovePC(M.ZONE_ID, M.DROP.x, M.DROP.y, M.DROP.z, M.DROP.h)
end

-- Which step is this npc? nil if it is not one of ours.
function M.step_of_npc(npc_id)
	local step = npc_id - M.FIRST_NPC
	if step >= 0 and step < M.STEPS then return step end
	return nil
end

-- ⚠️ Called from every tutorial NPC's own script on hail.
function M.hail(e, step)
	local c = e.other
	if not c or not c.IsClient or not c:IsClient() then return end

	-- ⚠️ Handed out on hail as well as on arrival: someone who was already standing in the Hall when
	-- this shipped never gets a fresh `event_enter_zone`, and would have no record until they zoned.
	M.ensure_umbrella(c)

	if c:IsTaskCompleted(task_of(step)) then
		e.self:Say("You have already walked this part of the road. My thanks.")
		return
	end

	-- ⚠️⚠️ THE DEATH BOOK LESSON AUTO-COMPLETES FOR ANYONE WHO ALREADY CARRIES ORIGIN, and it has to,
	-- because ORIGIN SURVIVES A DEATH WHILE THE TASK DOES NOT. `M.reset_on_death` hands the lesson
	-- back every run, but the AA it teaches is permanent -- and an owned Origin can never be offered
	-- again (`mr = 1`, so its next rank is out of range), so on every run after the first the player
	-- was told to claim something the window could not show, spent a hard-won point on one of three
	-- unrelated AAs to satisfy the objective, and was paid a 10 platinum consolation for it.
	-- 📌 The first AA point lands around level 20 (section 45), which is what made that misdirection
	-- expensive rather than merely untidy.
	-- ⚠️ Assign THEN complete, rather than skipping the task: routing it through the normal completion
	-- is what mirrors the step onto the induction record and keeps `M.on_task_complete` the single
	-- place a lesson is ever finished. `ensure_umbrella` already relies on that same assign-then-update
	-- ordering working within one call.
	if step == M.ORIGIN_STEP and (c:GetAAByAAID(M.ORIGIN_AA) or 0) > 0 then
		if not c:IsTaskActive(task_of(step)) then
			c:AssignTask(task_of(step), 0, false)
		end
		c:UpdateTaskActivity(task_of(step), 0, 1)
		e.self:Say("You already carry Origin -- death could not take it from you. " ..
			"Then this lesson is behind you, and the road home is yours.")
		return
	end
	-- ⚠️ NO ORDERING CHECK. Any of the ten may be taken at any time -- see M.UMBRELLA_TASK above for
	-- why the chain was removed and what took over the job of showing what is left.
	if c:IsTaskActive(task_of(step)) then
		if step == M.ORIGIN_STEP then M.ensure_origin_point(c) end
		e.self:Say("The task stands in your journal. Come back when it is done.")
		return
	end

	-- ⚠️ enforce_level_requirement = false, like the delve: the chain must work for a level 1
	-- character on their first run AND for a level 30 who has never pressed /aot.
	c:AssignTask(task_of(step), 0, false)

	-- ⚠️⚠️ AFTER `AssignTask`, NEVER BEFORE. The offer that `ensure_origin_point` triggers is
	-- narrowed to Origin alone only while this task is ACTIVE -- so banking the point first
	-- builds the offer against a lesson that has not started yet and fills the Death Book with
	-- three unrelated AAs, handing the player a free point to spend on any of them.
	-- ⚠️ Called through the M table: it is defined further down the file, beside the ledger it
	-- uses, so a local would not be in scope inside this closure.
	if step == M.ORIGIN_STEP then M.ensure_origin_point(c) end
end

-- ⚠️⚠️ THE ONLY PLACE AN OBJECTIVE IS COMPLETED. Called from global_player.event_say for the seven
-- windows whose commands reach Lua, and from C++ for the three that do not.
function M.mark(client, key)
	if not client or not key then return end
	for step = 0, M.STEPS - 1 do
		if M.KEYS[step] == key then
			local task = task_of(step)
			if client:IsTaskActive(task) then
				client:UpdateTaskActivity(task, 0, 1)
			end
			return
		end
	end
end

-- ⚠️ Say -> key. Prefix matched, so `spellpick`, `spelllvl`, `sjpool` all count as "used the spell
-- window". Deliberately loose: the objective is "you opened it and did something", not a specific act.
local SAY_KEYS = {
	{ "^spell",   "spells"     },
	{ "^sjpool",  "spells"     },
	{ "^shop",    "trader"     },
	{ "^srch",    "allaclone"  },
	{ "^delve",   "delve"      },
	{ "^diff",    "difficulty" },
	{ "^travel",  "travel"     },
	{ "^portal",  "travel"     },
	{ "^aapick",  "deathbook"  },
	-- ⚠️⚠️ THIS ROW SURVIVES THE C++ SAY INTERCEPT, AND THE REASON IS WORTH KNOWING. There are TWO
	-- kinds of intercept in `Client::ChannelMessageReceived`: AdvLoot (`als*`) and Autoskill (`ask*`)
	-- are handled entirely in C++ and `return` -- EVENT_SAY never fires, which is why those two steps
	-- have to be marked from `Client::AoTv4TutorialMark` instead. The fellowship verbs take the OTHER
	-- path: they are handled in Lua, so C++ fires the player quest event itself and only skips the
	-- broadcast. Lua still sees the message, so this row still fires.
	-- 📌 An earlier note here predicted the opposite and was wrong. If a future change ever moves
	-- `fship*` into the C++-handled group, THEN this breaks silently and "fellowship" must be added
	-- to AoTv4TutorialMark in the same commit.
	{ "^fship",   "fellowship" },
}

-- ---------------------------------------------------------------- rewards
--
-- ⚠️⚠️ EVERY REWARD IS PAID HERE, IN LUA -- the `tasks` reward columns are deliberately left at 0.
-- Four of the ten cannot be expressed in that table at all (an alternate currency, an AA grant, a
-- random pick out of an id block, and an edit to the reroll counter), so wiring the other six the DB
-- way would split one mechanism across two places and guarantee the drift this project keeps getting
-- bitten by. One payer, one set of rules.
--
-- ⚠️ Deliberately small and mostly consumable. A tutorial that pays real gear competes with the delve
-- and the loot tiers, and the point is to teach the loop rather than to be a source. The only two that
-- are NOT consumable -- the pack and Origin -- were picked because each makes the window it taught
-- more usable.

-- ⚠️⚠️ 331 IS WRITTEN IN TWO PLACES: here and the exclusion list in `gen_aa_pool.pl`. Origin must be
-- TAUGHT and never ROLLED -- a random copy makes step 6 pay nothing at all, because the grant is
-- refused against an ability already owned and reports that to no one. Nothing checks the two agree.
M.ORIGIN_AA = 331
-- Step 6 is the Death Book / AA lesson. Named because `aa_choice` asks whether THIS task is active
-- before it will offer Origin -- see `M.origin_lesson_active`.
M.ORIGIN_STEP = 6

-- ⚠️⚠️ THE ONE GATE THAT LETS ORIGIN BE OFFERED. `aa_choice.gather_affordable` skips Origin for
-- everybody else, so the AA a lesson promises can never turn up as a blind random roll -- and,
-- equally, is guaranteed to be there for someone holding that lesson. Both halves matter: excluding
-- it from the generated pool instead was tried and broke the second half silently.
function M.origin_lesson_active(client)
	if not client then return false end
	return client:IsTaskActive(M.FIRST_TASK + M.ORIGIN_STEP)
end

-- `alternate_currency.id`, not the item id (section 29): 57 is the Parchment Fragment, 58 the Ink of
-- the Lost. Both are spent on spell ranks, which is why the two research-flavoured steps pay them.
M.CUR_FRAGMENT = 57
M.CUR_INK      = 58

-- Paid when a reward turns out to be worth nothing to THIS character -- see the two guarded branches
-- in `M.give`. Never reached on a normal first run.
local FALLBACK_PLAT = 10

M.REWARDS = {
	[0] = { text = "a Worn Tome of Insight",                item = 147966 },
	[1] = { text = "5 platinum",                            plat = 5 },
	[2] = { text = "a Sturdy Traveller's Pack",             item = 67079 },
	[3] = { text = "10 platinum",                           plat = 10 },
	[4] = { text = "5 platinum and 10 Parchment Fragments", plat = 5,  currency = { 57, 10 } },
	[5] = { text = "10 Ink of the Lost",                    currency = { 58, 10 } },
	-- ⚠️⚠️ `once` BECAUSE ORIGIN SURVIVES DEATH, so there is nothing to re-give. Without it the
	-- auto-complete in `M.hail` would pay the 10 platinum "you already knew that ability" fallback
	-- on every single run -- free coin for one hail, on a lesson that does no work.
	[6] = { text = "the Origin ability",                    aa = 331, once = true },
	[7] = { text = "a delve augment",                       aug_tier = 1 },
	[8] = { text = "10 platinum and 10 Ink of the Lost",    plat = 10, currency = { 58, 10 } },
	-- ⚠️⚠️ `once` BECAUSE THIS ONE SURVIVES DEATH. It edits `rerollbuy_`, the lifelong counter that
	-- spell_choice section 3 records as deliberately NOT cleared by the roguelite wipe -- so paying
	-- it every run would let a player drive their reroll price to the 5p floor and hold it there by
	-- dying, permanently inverting a ladder whose whole job is to get dearer over a character's
	-- life. A Tome of Insight is meant to be the only thing that can wind it back.
	[9] = { text = "a cut to your reroll price",            reroll_cut = 50, once = true },
	-- ⚠️⚠️ `once` BECAUSE THE INSIGNIA SURVIVES DEATH -- `death_loss.M.is_kept` exempts 147975, which
	-- is the only reason the item exists at all. Without `once` the Hall would hand out a second one
	-- every run, and it is Lore, so the duplicate would be refused and the player paid nothing.
	-- 📌 Losing it anyway is still recoverable: Denara re-hands it on hail when you have none.
	[10] = { text = "a Fellowship Insignia",               item = 147975, once = true },
}

-- Finishing all ten. Deliberately a step UP rather than more of the same: the Etched tome is the
-- tier above the Worn one step 0 hands out.
-- ⚠️ Etched is usable in levels 11-20 and most characters will finish the induction well below that.
-- That is fine and is not a trap -- a tome held BELOW its band is refused and KEPT (only an over-band
-- tome is spent for a lesser result), so it simply waits until it is worth using.
M.UMBRELLA_REWARD = { text = "25 platinum and an Etched Tome of Insight", plat = 25, item = 147967 }

-- ---------------------------------------------------------------- paying again after a death
-- ⚠️⚠️ THE HALL PAYS EVERY RUN, BECAUSE THE ROGUELITE DEATH DESTROYS WHAT IT PAID. That is the rule,
-- and it is true of the items and the coin: `death_loss` takes all carried money and every carried
-- item outside the kept band, so the tomes, the pack, the augment and every platinum reward are gone
-- with the run that earned them. Re-walking the Hall to replace them is the intended loop.
--
-- ⚠️⚠️ BUT FOUR OF THE TEN REWARDS SURVIVE A DEATH, AND ONLY ONE OF THEM MATTERS:
--   * the two CURRENCIES (steps 4, 5, 8) -- `death_loss` says so itself: "character_alt_currency is
--     outside the inventory and nothing here touches it". Re-paid deliberately; ten fragments and ten
--     ink per full walk of the Hall is slow enough to be a reward rather than a tap.
--   * the ORIGIN AA (step 6) -- AA is permanent meta-progression by design. Re-granting is a harmless
--     no-op against an ability already owned, and the branch pays coin instead, which death does take.
--   * the REROLL CUT (step 9) -- the ONLY one gated below. See `once` on that entry.
-- 📌 Anything BANKED before dying also survives (bank slots 2000+ are never touched), so a player who
-- banks the tome and then dies keeps it and can earn another. Accepted: it costs a trip to a banker
-- and a full re-walk of ten quests, which is a poor rate for one consumable.
--
-- ⚠️ The bucket is PERMANENT and deliberately NOT cleared by the roguelite wipe -- the same shape as
-- `rerollbuy_` in spell_choice, and for the same reason: it prices something across a character's
-- whole life rather than per run. `death_loss` must never learn about it.
--
-- ⚠️ Tags are the step index, plus "u" for the umbrella. Compared with a comma-delimited `find` on
-- PLAIN text (the 4th argument), because a pattern search would make "1" match "10" -- and step 1
-- silently suppressing step 10's reward is exactly the kind of bug nobody would trace back here.
local function paid_key(client) return "hallpaid_" .. client:CharacterID() end

local function already_paid(client, tag)
	local s = eq.get_data(paid_key(client)) or ""
	if s == "" then return false end
	return string.find("," .. s .. ",", "," .. tag .. ",", 1, true) ~= nil
end

local function mark_paid(client, tag)
	local s = eq.get_data(paid_key(client)) or ""
	eq.set_data(paid_key(client), (s == "") and tag or (s .. "," .. tag))
end

-- Pay `r`. A reward flagged `once` is paid a single time per character, ever; everything else is paid
-- on every walk of the Hall, because the death that reset the task also destroyed what it gave.
-- ⚠️ The task completes either way -- a gated reward suppresses the PAYMENT, never the lesson.
local function pay(client, tag, r, label)
	if r and r.once then
		if already_paid(client, tag) then
			client:Message(MT.Yellow, string.format(
				"Titan Hall, %s: the lesson stands, but this reward is given only once.", label))
			return
		end
		mark_paid(client, tag)
	end
	M.give(client, r, label)
end

-- ⚠️⚠️ THE KEEPER BANKS THE POINT THE LESSON NEEDS. Without this the step is unreachable until
-- roughly level 20: `aa_choice.M.offer` returns at `budget < 1`, so a character with nothing
-- banked opens the Death Book to an EMPTY window and cannot tell that apart from a broken
-- quest. Reported from play twice, which is what settled it.
--
-- ⚠️⚠️ IT IS SAFE ONLY BECAUSE THE WINDOW SHOWS ORIGIN ALONE WHILE THIS LESSON IS ACTIVE
-- (`aa_choice.gather_affordable`). The granted point therefore cannot buy anything else --
-- remove that narrowing and this becomes a free AA point for any ability in the pool.
--
-- ⚠️⚠️ AND ONCE PER CHARACTER, EVER, VIA THE PERMANENT LEDGER -- the "does not own Origin" test
-- is NOT sufficient on its own. The lesson resets on death while the point does not, so a
-- player could take the lesson, bank the point, die before spending it, spend it on anything
-- (the lesson is no longer active, so the window is unrestricted again), and hail for another.
-- That is an AA farm paid out one death at a time.
-- ⚠️ `banked() < 1` as well, so somebody who already has points is not handed a spare.
function M.ensure_origin_point(client)
	if not client then return end
	if (client:GetAAByAAID(M.ORIGIN_AA) or 0) > 0 then return end
	if already_paid(client, "6p") then return end
	if aa_choice.banked(client) >= 1 then return end

	mark_paid(client, "6p")
	-- ⚠️ grant_picks takes an EVENT table, not a client -- `{ self = client }` is the shape it
	-- reads. It banks the point, pushes the offer AND pops the Advancement tab, which is the
	-- whole reason to go through it rather than writing the bucket here.
	aa_choice.grant_picks({ self = client }, 1)
	client:Message(MT.Yellow,
		"The Keeper presses a single ability point into your hand. Spend it in the Death Book.")
end

-- ⚠️⚠️ Wipes all eleven so a dead character can re-run the Hall, matching what the Gloomingdeep
-- tutorial set does in `global_player.TUTORIAL_TASKS`. BOTH calls are needed and they do different
-- jobs: `RemoveTaskByTaskID` drops an ACTIVE task from memory and the DB (CancelAllTasks only clears
-- the in-memory slots, so the row reloads on the next zone and the task looks stuck), while
-- `UncompleteTask` clears the COMPLETED record without which `M.hail` answers "you have already
-- walked this part of the road" forever.
-- ⚠️ Call it BEFORE `CancelAllTasks`, like the tutorial list: a task has to still be in the active
-- list to be found and removed.
function M.reset_on_death(client)
	if not client then return end
	for step = 0, M.STEPS - 1 do
		client:RemoveTaskByTaskID(task_of(step))
		client:UncompleteTask(task_of(step))
	end
	client:RemoveTaskByTaskID(M.UMBRELLA_TASK)
	client:UncompleteTask(M.UMBRELLA_TASK)
end

-- ⚠️⚠️ THE BAND GUARD IS LOAD BEARING. `event_task_complete` is shared with the delve, whose handler
-- reads a completion as a cleared rung -- so this must return early on anything that is not ours, and
-- the delve's must do the same for ours. Returning `false` says "not mine".
function M.on_task_complete(client, task_id)
	if not client or not task_id then return false end

	if task_id == M.UMBRELLA_TASK then
		pay(client, "u", M.UMBRELLA_REWARD, "complete")
		return true
	end

	local step = step_of_task(task_id)
	if step < 0 or step >= M.STEPS then return false end

	pay(client, tostring(step), M.REWARDS[step],
		string.format("%d of %d", step + 1, M.STEPS))

	-- ⚠️⚠️ MIRROR THE STEP ONTO THE RECORD. The umbrella's activities are inert Touches exactly like
	-- the individual ones -- nothing in the game completes them on its own, so if this call is lost
	-- the record can never finish and there is no error to notice. Guarded on it being active,
	-- because a player can complete a step before ever picking the record up.
	if client:IsTaskActive(M.UMBRELLA_TASK) then
		client:UpdateTaskActivity(M.UMBRELLA_TASK, step, 1)
	end
	return true
end

function M.give(client, r, label)
	if not r then return end
	local got = r.text

	-- ⚠️ AddMoneyToPP is (copper, silver, gold, platinum) -- that argument order, platinum LAST.
	if r.plat     then client:AddMoneyToPP(0, 0, 0, r.plat) end
	if r.currency then client:AddAlternateCurrencyValue(r.currency[1], r.currency[2]) end

	-- ⚠️⚠️ `Lua_Client::SummonItem` ROUTES THROUGH `AoTv4MythicReward`, which silently upgrades a
	-- quest reward to its Mythic tier (section 10). It is a no-op for all three items used here --
	-- none of 147966 / 67079 / the T1 augment band has tier rows, checked -- but any WEARABLE added
	-- to this table later will be handed over as a Mythic without a word about it.
	if r.item then client:SummonItem(r.item) end

	if r.aug_tier then
		-- ⚠️ Any member of the tier's block is a legal roll; a tier is a flat pool of equal variants.
		-- ⚠️ Plain `math.random` with NO reseed: seeding here is a global side effect that would yank
		-- the sequence out from under the delve, the world boss and the picker (section 44).
		local block = dungeon.AUG_TIER_BLOCK[r.aug_tier]
		client:SummonItem(math.random(block[1], block[2]))
	end

	if r.aa then
		-- ⚠️ ability id, rank to reach, ignore_cost -- NOT a rank id (section 47 records `GetAA`
		-- taking the other one, which is the easy way to get this backwards).
		if not client:GrantAlternateAdvancementAbility(r.aa, 1, true) then
			client:AddMoneyToPP(0, 0, 0, FALLBACK_PLAT)
			got = string.format("%d platinum -- you already knew that ability", FALLBACK_PLAT)
		end
	end

	if r.reroll_cut then
		-- ⚠️ `cut_rerolls` neither messages nor refuses; a counter already at the floor comes back
		-- `changed = false` and the caller decides. Paying coin instead is the point: a completed
		-- step must never hand over nothing, which is exactly what an uncut counter would do.
		local changed, before, after = spell_choice.cut_rerolls(client, r.reroll_cut)
		if changed then
			got = string.format("a cut to your reroll price, from %dp to %dp",
				math.floor(before / 1000), math.floor(after / 1000))
		else
			client:AddMoneyToPP(0, 0, 0, FALLBACK_PLAT)
			got = string.format("%d platinum -- your rerolls already cost the least they can",
				FALLBACK_PLAT)
		end
	end

	client:Message(MT.Yellow, string.format("Titan Hall, %s: you receive %s.", label, got))
end

-- ⚠️ Runs on EVERY say from every player, so it early-outs on the cheap test first: a character with
-- no tutorial task active does no pattern matching at all.
function M.on_say(client, msg)
	if not client or not msg then return end
	local any = false
	for step = 0, M.STEPS - 1 do
		if client:IsTaskActive(task_of(step)) then any = true break end
	end
	if not any then return end

	local m = string.lower(msg)
	for _, row in ipairs(SAY_KEYS) do
		if string.find(m, row[1]) then
			M.mark(client, row[2])
			return
		end
	end
end

return M
