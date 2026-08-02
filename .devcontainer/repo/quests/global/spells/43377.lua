-- Kindred Blaze (43377) -- the group-heal proc of Kindred Blaze (43333)
-- The ENGINE pays this 70 point heal to every group member; this script only announces it, because
-- the swinger otherwise has no way to tell their proc fired. See lua_modules/aotv4_kindred.lua.
local kindred = require("aotv4_kindred")

function event_spell_effect(e)
	kindred.on_group_heal(e, 70)
end
