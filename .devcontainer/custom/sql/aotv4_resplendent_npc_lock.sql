-- AoTv4 -- LOCK the three Resplendent hub NPCs so DB edits/imports can't silently break them again.
-- =================================================================================================
-- Wayfinder Alessa (2000400), Reforger Vael (2000401) and Herald Coren (2000402) have twice been
-- clobbered by table-wide DB writes:
--   * created bodytype 11 (NoTarget -> untargetable, so normal players can't /hail them) and
--     class 41 (Merchant -> right-click opens an empty merchant window instead of talking); and
--   * the 2026-08-02 "Group A" import did `REPLACE INTO npc_types SELECT * FROM <dump>`, which
--     silently reverted an earlier interactable fix back to 41/11.
--
-- This file SETS the correct values AND installs BEFORE INSERT/UPDATE triggers that re-assert them on
-- every write, so a REPLACE INTO ... SELECT * FROM <dump>, or a stray UPDATE, can no longer change:
--     npc_types : class = 1  (non-merchant humanoid),  bodytype = 1 (targetable)
--     spawn2    : heading = 256 (South),  min_expansion = -1, max_expansion = -1 (never
--                 content-filtered out on the Classic-locked server)
--
-- Resplendent is a no-combat zone (zone.cancombat = 0), so targetable != killable.
-- npc_types/spawn2 are read at zone boot (NOT shared memory) -- a zone restart or #repop applies it.
--
-- ⚠️⚠️ TRIGGERS DO NOT SURVIVE A FULL-TABLE DUMP IMPORT. `mysql < dump.sql` runs DROP TABLE, which
--    drops the trigger with it; a REPLACE INTO / UPDATE / TRUNCATE+reload does NOT drop it. The
--    REPLACE-INTO case is the one that actually hit us, and it is covered. After any FULL import,
--    RE-RUN THIS FILE to reinstall the guards. (Verify presence any time with the last query below.)
--
-- ⚠️ TO DELIBERATELY EDIT THESE NPCs LATER: drop the four triggers first, make the change, then
--    re-run this file with the new intended values baked in:
--        DROP TRIGGER aotv4_lock_resplendent_npc_bi;   DROP TRIGGER aotv4_lock_resplendent_npc_bu;
--        DROP TRIGGER aotv4_lock_resplendent_spawn_bi;  DROP TRIGGER aotv4_lock_resplendent_spawn_bu;
--
-- Apply (honours DELIMITER, unlike `mysql -e`):
--     docker exec -i akk-stack-mariadb-1 mysql -uroot -p<pw> peq < aotv4_resplendent_npc_lock.sql

-- 1) reassert the current correct state -----------------------------------------------------------
UPDATE npc_types
   SET class = 1, bodytype = 1
 WHERE id IN (2000400, 2000401, 2000402);

UPDATE spawn2 s2
  JOIN spawnentry se ON se.spawngroupID = s2.spawngroupID
   SET s2.heading = 256, s2.min_expansion = -1, s2.max_expansion = -1
 WHERE se.npcID IN (2000400, 2000401, 2000402);

-- 2) install the guards ---------------------------------------------------------------------------
DELIMITER $$

DROP TRIGGER IF EXISTS aotv4_lock_resplendent_npc_bi $$
CREATE TRIGGER aotv4_lock_resplendent_npc_bi
BEFORE INSERT ON npc_types FOR EACH ROW
BEGIN
  IF NEW.id IN (2000400, 2000401, 2000402) THEN
    SET NEW.class = 1, NEW.bodytype = 1;
  END IF;
END $$

DROP TRIGGER IF EXISTS aotv4_lock_resplendent_npc_bu $$
CREATE TRIGGER aotv4_lock_resplendent_npc_bu
BEFORE UPDATE ON npc_types FOR EACH ROW
BEGIN
  IF NEW.id IN (2000400, 2000401, 2000402) THEN
    SET NEW.class = 1, NEW.bodytype = 1;
  END IF;
END $$

-- spawn2 is keyed on spawngroupID (2000400-2000402), NOT the auto-increment spawn2.id: the group id
-- is the stable custom handle tied to each NPC, and survives the row being deleted/recreated.
DROP TRIGGER IF EXISTS aotv4_lock_resplendent_spawn_bi $$
CREATE TRIGGER aotv4_lock_resplendent_spawn_bi
BEFORE INSERT ON spawn2 FOR EACH ROW
BEGIN
  IF NEW.spawngroupID IN (2000400, 2000401, 2000402) THEN
    SET NEW.heading = 256, NEW.min_expansion = -1, NEW.max_expansion = -1;
  END IF;
END $$

DROP TRIGGER IF EXISTS aotv4_lock_resplendent_spawn_bu $$
CREATE TRIGGER aotv4_lock_resplendent_spawn_bu
BEFORE UPDATE ON spawn2 FOR EACH ROW
BEGIN
  IF NEW.spawngroupID IN (2000400, 2000401, 2000402) THEN
    SET NEW.heading = 256, NEW.min_expansion = -1, NEW.max_expansion = -1;
  END IF;
END $$

DELIMITER ;

-- 3) verify values + that all four guards are installed -------------------------------------------
SELECT nt.id, nt.name, nt.class, nt.bodytype, s2.heading, s2.min_expansion, s2.max_expansion
  FROM npc_types nt
  JOIN spawnentry se ON se.npcID = nt.id
  JOIN spawn2 s2     ON s2.spawngroupID = se.spawngroupID
 WHERE nt.id IN (2000400, 2000401, 2000402);

SELECT trigger_name, event_object_table, action_timing, event_manipulation
  FROM information_schema.triggers
 WHERE trigger_name LIKE 'aotv4_lock_resplendent_%'
 ORDER BY trigger_name;
