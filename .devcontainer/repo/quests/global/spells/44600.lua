-- Paladin class AA payload. See lua_modules/aotv4_paladin.lua for why this cannot live in the
-- spell row. The AA that casts this is created by migration v94.
local pal = require("aotv4_paladin")

function event_spell_effect(e)
	pal.ardent_strike(e)
end
