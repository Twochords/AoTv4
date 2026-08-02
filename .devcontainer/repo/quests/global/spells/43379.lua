-- Kindred Beacon (43379) -- the group-heal proc of Kindred Beacon (43335)
-- The ENGINE pays this 150 point heal to every group member; this script only announces it, because
-- the swinger otherwise has no way to tell their proc fired. See lua_modules/aotv4_kindred.lua.
local kindred = require("aotv4_kindred")

function event_spell_effect(e)
	kindred.on_group_heal(e, 150)
end
