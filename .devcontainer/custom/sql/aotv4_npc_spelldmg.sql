-- AoTv4: tone down NPC spell damage for level 30+ monsters.
--
-- Problem: mobs above ~level 30 cast classic-NAMED nukes (Blaze, Inferno Shock, Conflagration, ...)
-- whose damage VALUES in the RoF2 spell data are post-classic/inflated, and whose formulas
-- (5/102/103/104) scale the damage UP with the caster's level. Result: players (squishy Bards in a
-- roguelite) were taking 300+ per nuke from mid-level caster mobs -- too high for a Classic-era feel.
--
-- Fix: halve NPC spell output for level >= 30 via npc_types.spellscale. spellscale multiplies ALL of an
-- NPC's spell effect values (nukes AND DoTs/heals) -- see zone/effects.cpp GetActSpellDamage/DoT/Heal,
-- `value = value * GetSpellScale() / 100`. 100 (or 0 = skip) == 1x; 50 == half.
--
-- NPC-only (does not touch player spell damage), data-only (no rebuild). npc_types loads at ZONE BOOT
-- (not shared memory), so this applies after a zone restart -- no ./shared_memory needed.
-- Re-tune by changing the value below and re-running + restarting zones.

UPDATE npc_types SET spellscale = 50 WHERE level >= 30;
