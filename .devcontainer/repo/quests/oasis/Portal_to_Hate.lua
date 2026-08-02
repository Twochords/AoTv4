-- oasis/Portal_to_Hate.lua -- AoTv4 walk-in portal to the Plane of Hate.
-- An invisible, untargetable NPC sits on the blue-fire platform in Oasis of Marr (spawned by
-- aotv4_oasis_hate_portal.sql at 410.28,-379.38,134.60). The fire is a decorative effect, NOT a clickable
-- door, so we use a proximity trigger: step onto the platform and you're moved to the Plane of Hate
-- (zone 76) at the normal port-in (safe point) -- like the Feerrott->Fear zone_point is a walk-through.

function event_spawn(e)
	e.self:SetInvisible(1)   -- hide the trigger (race 127 still renders as a figure on RoF2)
	-- ~20x20 box centered on the fire platform; any Z (the platform is elevated, nothing else stands here)
	eq.set_proximity(e.self:GetX() - 10, e.self:GetX() + 10, e.self:GetY() - 10, e.self:GetY() + 10)
end

function event_enter(e)
	-- MovePC is a CLIENT method; e.other is a Mob here, so cast it. (Message works on Mob, which is why
	-- the flavor line showed but the move didn't in the first version.)
	if e.other and e.other:IsClient() then
		e.other:Message(MT.Yellow, "The blue flames engulf you and you are drawn into the Plane of Hate!")
		-- hateplaneb (zone 186) is the bootable Plane of Hate here. Arrival coords are the spot the
		-- server-owner picked (near the zone safe point). (Classic hateplane/76 doesn't boot.)
		e.other:CastToClient():MovePC(186, -359.71, 661.71, 3.13, 0)
	end
end
