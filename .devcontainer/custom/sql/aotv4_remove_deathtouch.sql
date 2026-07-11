-- AoTv4: remove death-touch / instant-kill spells from ALL NPC usage.
--
-- On a roguelite where death resets you to level 1, an unavoidable one-shot is pure punishment. These
-- are the direct-damage (SE_CurrentHP) spells that deal >= 5000 -- i.e. they instantly kill any player
-- regardless of HP/gear. There is a clean break here: the next tier down is 4500 and below, which the
-- 50% level-30+ spell-damage cut (aotv4_npc_spelldmg.sql) already tames to ~2000 (survivable).
--
--   982  Cazic Touch          100000   (Cazic Thule DT)
--   1948 Destroy              100000
--   2859 Touch of Vinitras     20000
--   5002 Mana Blast             7000
--   888  Wrath of the Ikaav     7000
--   5723 Searing Pain           6000
--   6972 Siphon of Vitae        5500
--   4436 Fist of Innoruuk       5000
--   5119 Force of Trusik's Rage 5000
--
-- Delivered two ways -- normal spell lists (npc_spells_entries) and header procs (npc_spells.*_proc).
-- Both are cleared. npc_spells* load at ZONE BOOT (not shared memory), so a zone restart applies this;
-- no ./shared_memory needed. The spells themselves are left intact in spells_new (untouched for any
-- other reference); this only stops NPCs from casting/proccing them.

DELETE FROM npc_spells_entries
  WHERE spellid IN (982,1948,2859,5002,888,5723,6972,4436,5119);

UPDATE npc_spells SET attack_proc    = -1 WHERE attack_proc    IN (982,1948,2859,5002,888,5723,6972,4436,5119);
UPDATE npc_spells SET range_proc     = -1 WHERE range_proc     IN (982,1948,2859,5002,888,5723,6972,4436,5119);
UPDATE npc_spells SET defensive_proc = -1 WHERE defensive_proc IN (982,1948,2859,5002,888,5723,6972,4436,5119);
