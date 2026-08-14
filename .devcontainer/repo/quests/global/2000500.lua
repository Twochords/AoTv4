-- #a_forgotten_waypoint (npc 2000500) -- an invisible marker that unlocks a travel destination.
-- =================================================================================================
-- One npc row and this one script serve EVERY spot. Which spot a given marker is comes from the
-- entity variable `travel_spot`, stamped by aotv4_travel.spawn_markers at spawn time -- the identity
-- has to travel on the spawn because the script cannot know which of nine it is.
--
-- ⚠️⚠️ THE PROXIMITY IS SET FROM THE MARKER'S OWN POSITION, NOT FROM A TABLE. That is what lets one
-- script cover every spot: it asks where it was spawned and draws a box around itself. Setting it
-- from the spot table instead would mean this script requiring the module and looking itself up,
-- which is the same fact stored twice.
--
-- ⚠️ A PROXIMITY IS A BOX, NOT A SPHERE. RADIUS_XY is a half-width per axis, so the corners reach
-- about 1.4x further than the sides. Fine here -- the intent is "walk through this area".
--
-- ⚠️⚠️ `e.other` IN AN NPC SCRIPT IS A `Lua_Mob`, NOT A `Lua_Client` (CLAUDE.md section 24). IsClient()
-- returns true on it and GetName() works, but every Client-only method -- CharacterID included, which
-- is what the discovery bucket is keyed on -- is NIL. It has to be resolved through the entity list,
-- exactly as Wayfinder Alessa does. Getting this wrong fails AFTER the marker has already fired,
-- which reads as the spot being broken rather than as a scripting error.
-- =================================================================================================

local travel = require("aotv4_travel")

function event_spawn(e)
	eq.set_proximity(
		e.self:GetX() - travel.RADIUS_XY, e.self:GetX() + travel.RADIUS_XY,
		e.self:GetY() - travel.RADIUS_XY, e.self:GetY() + travel.RADIUS_XY,
		e.self:GetZ() - travel.RADIUS_Z,  e.self:GetZ() + travel.RADIUS_Z
	)
end

function event_enter(e)
	local id = travel.spot_of(e.self)
	if not id then return end

	-- ⚠️ See the header: e.other is a Lua_Mob. Resolve the real client or CharacterID is nil.
	local c = eq.get_entity_list():GetClientByID(e.other:GetID())
	if not c or not c.valid then return end

	travel.discover(c, id)
end
