-- AoTv4: swarm-pet runspeed fix. SwarmPetSoF1 (675, "Swarm of Decay") and SwarmPetDG3 (684) shipped
-- with runspeed = 0 -- they spawn around the caster but can't move, so they never reach the target to
-- melee it ("spawns adds but they don't attack"). Every OTHER swarm pet runs at 1.25; match it.
-- npc_types load at zone boot -> restart zones after.
UPDATE npc_types SET runspeed = 1.25 WHERE id IN (675, 684) AND runspeed = 0;
