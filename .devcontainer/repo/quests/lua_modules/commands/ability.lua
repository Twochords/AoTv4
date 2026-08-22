-- #ability <1|2|3> -- fire one of your class abilities without the Combat Abilities window.
--
-- ⚠️⚠️ THIS EXISTS BECAUSE THE CLIENT WILL NOT OPEN ITS OWN COMBAT ABILITIES WINDOW FOR EVERY
-- CLASS. RoF2 only ever expected the melee classes to have disciplines, so Alt+C does nothing for a
-- Cleric -- the mirror of the problem section 14 solved for casting, where the client hides the
-- spellbook from the four pure-melee classes.
-- 📌 The WINDOW answer is the "Combat Skills" tab of the Autoskill window (/autoskill, /abilities or
-- /ca), which is ours and opens for anybody. This command is the no-dll route and the source of the
-- /hotbutton lines.
--
-- 📌 A player can make a SOCIAL containing `#ability 1` and drag it to a hotbar, which gives every
-- class a working button today with no client change at all.
--
-- ⚠️ Registered in lua_modules/command.lua at access 0. A file here that is not listed there is
-- never reachable (section 17c) -- that is how #worldboss sat dead for a month.
local ab = require("aotv4_class_abilities")

local function usage(c)
	c:Message(MT.Yellow, "Usage: #ability 1|2|3   (1 at level 1, 2 at level 5, 3 at level 10)")
end

local function ability(e)
	local c = e.self
	if not c then return end

	local tier = tonumber(e.args[1] or "")
	if not tier or tier < 1 or tier > 3 then
		-- No argument lists what you have, which is the quickest way to tell "the grant did not run"
		-- apart from "the window will not show it".
		local class = c:GetClass()
		if not ab.BUILT[class] then c:Message(MT.Red, "Your class has no class abilities.") return end
		c:Message(MT.Yellow, "Your class abilities:")
		for t = 1, 3 do
			local id = ab.spell_id(class, t)
			local state
			if c:GetLevel() < ab.TIER_LEVEL[t] then
				state = string.format("locked until level %d", ab.TIER_LEVEL[t])
			elseif c:HasDisciplineLearned(id) then
				state = "learned"
			else
				state = "NOT learned -- relog"
			end
			c:Message(MT.Yellow, string.format("  %d. %s (%s)", t, eq.get_spell_name(id), state))
		end
		-- ⚠️⚠️ THIS IS THE POINT OF THE COMMAND, NOT AN EXTRA. The stock Combat Abilities window will
		-- not open for a caster, so there is nothing to DRAG to a hotbar -- and an ability nobody can
		-- put on their bar might as well not exist. /hotbutton builds the button from a command
		-- instead, which needs no window, no dragging and no dll.
		c:Message(MT.Yellow, "To put these on a hotbar, type these lines:")
		for t = 1, 3 do
			local id = ab.spell_id(class, t)
			c:Message(MT.Yellow, string.format("  /hotbutton %s #ability %d",
				(eq.get_spell_name(id) or ""):gsub(" ", ""), t))
		end
		return
	end

	local class = c:GetClass()
	if not ab.BUILT[class] then
		c:Message(MT.Red, "Your class has no class abilities.")
		return
	end

	if c:GetLevel() < ab.TIER_LEVEL[tier] then
		c:Message(MT.Red, string.format("That ability unlocks at level %d.", ab.TIER_LEVEL[tier]))
		return
	end

	local id = ab.spell_id(class, tier)
	if not c:HasDisciplineLearned(id) then
		-- Should not happen -- grant runs on connect, level up and death -- but saying so is better
		-- than a silent no-op if it ever does.
		c:Message(MT.Red, "You have not learned that ability yet. Try relogging.")
		return
	end

	-- ⚠️ UseDiscipline is the SAME entry point the hotbutton uses, so every gate still applies:
	-- the level check, the endurance cost, the discipline recast timer and the stun/mez guards. This
	-- is a different way to press the button, not a way around it.
	-- ⚠️ A target is required for the targeted abilities; the self ones ignore it. Passing 0 when
	-- there is no target lets UseDiscipline refuse in its own words rather than us guessing.
	local tgt = c:GetTarget()
	c:UseDiscipline(id, (tgt and tgt.valid) and tgt:GetID() or c:GetID())
end

return ability;
