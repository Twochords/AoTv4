-- Fellowmaster Denara -- Titan Hall induction step 10, and the fellowship interface.
--
-- ⚠️⚠️ THIS FILE LIVED IN `quests/resplendent/` AND THEREFORE NEVER RAN. Quest scripts are resolved
-- by the ZONE the NPC stands in, and Denara was placed in freeporttheater -- so her whole saylink
-- menu, the free kindling and every fellowship command she advertises were dead from the day she was
-- spawned, with no error anywhere. A script in the wrong zone folder is silently no script at all.
-- 📌 Kindler Bram (2000411) needs no script: he is class 41 Merchant with merchantlist 1000030, and
-- the engine opens his shop on right-click.
--
-- ⚠️⚠️ THE EQ NATIVE FELLOWSHIP WINDOW CANNOT BE USED, and this is not a preference. The client has
-- the window (`pinstCFellowshipWnd`), but server side `OP_FellowshipUpdate` is a NAME IN
-- `emu_oplist.h` AND NOTHING ELSE -- no handler, no struct in eq_packet_structs.h, and no mapping in
-- the RoF2 patch, so it has no wire opcode for this client. Section 13 records the same dead end for
-- the native trader window. Our own window is what the AoT menu opens; this menu is the fallback for
-- a player with no dll, and the only place the rules are explained in words.
local fellowship = require("aotv4_fellowship")
local tutorial   = require("aotv4_tutorial")

local INSIGNIA = fellowship.INSIGNIA

-- ⚠️ Explained here rather than in the task description because a `tasks.description` is read once,
-- in a journal, by someone who has not seen the feature yet. This is repeatable and on demand.
local function explain(e, c)
	e.self:Say("A fellowship is not a group. Groups end when you log out; a fellowship does not.")
	c:Message(MT.LightBlue, "  Up to " .. fellowship.MAX_MEMBERS ..
		" of you, bound across every zone. One roster, and a chat that reaches your kin wherever they stand.")
	c:Message(MT.LightBlue, "  Light a campfire and it becomes a place any of you may return to. " ..
		"Bram sells the kindling; your first fire is from me.")
	c:Message(MT.LightBlue, "  Your insignia carries you there -- out of combat, outside any instance, " ..
		"and only into a region you have already opened.")
end

local function menu(e, c)
	local fid = fellowship.fid_of(c)
	if not fid then
		e.self:Say("Fellowships bind those who would travel together. You run alone.")
		c:Message(MT.LightBlue, "  " .. eq.say_link("fshipform Wayfarers", true, "[ Form a fellowship ]"))
		return
	end

	local leader, members = fellowship.roster(fid)
	e.self:Say(string.format("You run with %s, %d of %d strong.",
		fellowship.name_of(fid), #members, fellowship.MAX_MEMBERS))
	for _, cid in ipairs(members) do
		c:Message(MT.LightBlue, string.format("    %s%s",
			eq.get_char_name_by_id(cid) or ("#" .. cid), cid == leader and "  (leader)" or ""))
	end

	local f = fellowship.fire(fid)
	if f then
		c:Message(MT.Yellow, string.format("  Campfire burning in %s, %d minute(s) left.",
			eq.get_zone_long_name_by_name(f.zone) or f.zone,
			math.ceil((f.expiry - os.time()) / 60)))
		c:Message(MT.LightBlue, "  " .. eq.say_link("fshipgo", true, "[ Travel to the campfire ]"))
	else
		c:Message(MT.Yellow, "  No campfire burning. Light one where your fellowship should gather.")
	end

	-- ⚠️ The free basic campfire is handed out here, never sold -- the design refuses to gate the
	-- TRAVEL target behind coin. One at a time, or a player hails repeatedly and stockpiles them.
	if not c:HasItem(147972) then
		c:SummonItem(147972)
		e.self:Say("Take this kindling. Light it, and your fellowship can find their way to you.")
	end

	c:Message(MT.LightBlue, "  " .. eq.say_link("fshipleave", true, "[ Leave the fellowship ]"))
	if leader == c:CharacterID() then
		c:Message(MT.Yellow, "  Leader: /say fshipinv <name> to invite, fshipkick <name> to remove.")
	end
end

function event_say(e)
	local c, m = e.other, string.lower(e.message or "")
	if not c or not c.IsClient or not c:IsClient() then return end

	if string.find(m, "hail") then
		-- ⚠️ The lesson FIRST: `tutorial.hail` hands out the induction record as well as this step,
		-- and a new arrival should have the journal entry before the wall of explanation.
		tutorial.hail(e, 10)
		explain(e, c)

		-- ⚠️⚠️ THE INSIGNIA IS RE-HANDED WHEN MISSING, AND THAT IS THE RECOVERY PATH FOR THE WHOLE
		-- FEATURE. It is exempt from the death wipe (`death_loss.M.is_kept`) and the lesson pays it
		-- only ONCE, so without this a character who somehow lost it -- destroyed, or earned before
		-- the exemption existed -- could never reach a campfire again. It is Lore, so a duplicate is
		-- impossible and the HasItem test is belt to that braces rather than the only guard.
		if not c:HasItem(INSIGNIA) then
			c:SummonItem(INSIGNIA)
			e.self:Say("Carry this. Death cannot take it from you -- I have seen to that.")
		end

		menu(e, c)
		return
	end

	if string.find(m, "fellowship") or string.find(m, "explain") then explain(e, c) return end
end
