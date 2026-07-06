-- aotv4_no_guildmasters.sql
-- Neuter every guild trainer so skills can NOT be bought -- all skills come from the level-up reward
-- window (combat skills) or auto-learn (passive skills). See aotv4_bard_only_creation / spell_choice.
--
-- The client shows a "train" window (and the server's OPGMTrainSkill accepts it) only when the NPC's
-- class is a GM class: WarriorGM(20) .. BerserkerGM(35), a contiguous block that mirrors the 16 base
-- classes Warrior(1) .. Berserker(16) at a fixed +19 offset. Demoting each GM to its base class
-- (class - 19) removes the train window on both client and server -- OPGMTrainSkill early-returns for
-- any NPC whose class < WarriorGM -- while leaving the NPC standing in its guild hall for flavor.
--
-- npc_types loads per-zone at boot (NOT shared memory), so this needs a ZONE restart, no ./shared_memory.
-- Reversible: restore original classes from aotv4_guildmaster_backup (matched by npc id).

CREATE TABLE IF NOT EXISTS aotv4_guildmaster_backup AS
  SELECT id, class FROM npc_types WHERE class BETWEEN 20 AND 35;

UPDATE npc_types SET class = class - 19 WHERE class BETWEEN 20 AND 35;
