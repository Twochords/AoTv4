-- AoTv4 -- make the three Resplendent hub NPCs targetable and interactable.
-- =================================================================================================
-- Wayfinder Alessa (2000400), Reforger Vael (2000401) and Herald Coren (2000402) were created with
-- bodytype = 11 and class = 41. Both were wrong for a hail-only quest NPC:
--   * bodytype 11 is the engine's `NoTarget` body (common/bodytypes.h: "can't target this bodytype")
--     -- so a NORMAL player cannot target them, and therefore cannot /hail them. Only a GM (who can
--     target anything) could reach their event_say, which is why the bug read as "works for me,
--     broken for everyone else." The creation SQL's comment called it "untargetable-for-attack," but
--     the flag blocks ALL targeting, not just attack.
--   * class 41 is Merchant, so a right-click opened an (empty) merchant window instead of hailing.
--
-- Resplendent is a no-combat zone (zone.cancombat = 0), so making them ordinary targetable humanoids
-- (bodytype 1) does NOT make them killable -- nothing in the zone can be attacked. class 1 (Warrior)
-- is a neutral non-merchant class; the region/reforge logic does not read the NPC's class.
--
-- ⚠️ A zone restart is enough -- npc_types is read at zone boot, NOT shared memory.
--
-- ⚠️ This had regressed once already: the 2026-08-02 "Group A" DB import re-imported npc_types from a
-- dump that still held the old 41/11 values, silently reverting an earlier interactable fix. If a
-- future import brings npc_types back, re-run this.

UPDATE npc_types
   SET class    = 1,   -- was 41 (Merchant) -> plain non-merchant humanoid
       bodytype = 1    -- was 11 (NoTarget)  -> targetable Humanoid
 WHERE id IN (2000400, 2000401, 2000402);

-- All three should face SOUTH. EQEmu heading is a 0-512 scale (0=North, 128=West, 256=South,
-- 384=East); they were created at 128 (West). Heading lives on spawn2, applied at spawn -- a zone
-- reboot or #repop picks it up.
UPDATE spawn2 s2
  JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
   SET s2.heading = 256   -- South
 WHERE se.npcID IN (2000400, 2000401, 2000402);

SELECT nt.id, nt.name, nt.class, nt.bodytype, s2.heading
  FROM npc_types nt
  JOIN spawnentry se ON se.npcID = nt.id
  JOIN spawn2 s2      ON s2.spawngroupID = se.spawngroupID
 WHERE nt.id IN (2000400, 2000401, 2000402);
