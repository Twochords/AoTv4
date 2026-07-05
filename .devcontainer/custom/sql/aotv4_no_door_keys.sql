-- AoTv4: remove ALL key + lock requirements from doors (incl. raid/dungeon-zone entrances) so any
-- player can open them -- no key item, no pick-lock skill. Originals are backed up first, so both are
-- reversible:
--   revert keys:  UPDATE doors d JOIN aotv4_door_keys_backup  b ON d.id=b.id SET d.keyitem=b.keyitem;
--   revert locks: UPDATE doors d JOIN aotv4_door_locks_backup b ON d.id=b.id SET d.lockpick=b.lockpick;
--
-- Doors load from the DB at ZONE BOOT (not shared memory) -> restart zones after applying.
-- NOTE: this does NOT touch quest/flag-based raid access (script-driven, not a door key/lock).

-- 1) key-item gate
CREATE TABLE IF NOT EXISTS aotv4_door_keys_backup (id INT UNSIGNED PRIMARY KEY, keyitem INT NOT NULL);
REPLACE INTO aotv4_door_keys_backup (id, keyitem) SELECT id, keyitem FROM doors WHERE keyitem <> 0;
UPDATE doors SET keyitem = 0 WHERE keyitem <> 0;

-- 2) pick-lock (rogue skill) gate
CREATE TABLE IF NOT EXISTS aotv4_door_locks_backup (id INT UNSIGNED PRIMARY KEY, lockpick SMALLINT NOT NULL);
REPLACE INTO aotv4_door_locks_backup (id, lockpick) SELECT id, lockpick FROM doors WHERE lockpick <> 0;
UPDATE doors SET lockpick = 0 WHERE lockpick <> 0;
