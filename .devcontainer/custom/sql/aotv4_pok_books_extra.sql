-- ==========================================================================================
-- AoTv4: three extra PoK-book travel doors (discovered-book network, see CLAUDE.md §11).
--   * butcher  doorid 179  "Butcherblock Docks"  @ pos_x 2967.91, pos_y 1227.09, pos_z -2.26
--   * ecommons doorid  71  "Commonlands"         @ pos_x -196.90, pos_y -1512.30, pos_z 3.13
--   * qeytoqrg doorid  14  "Qeynos Hills"        @ pos_x  957.92, pos_y  4283.64, pos_z -6.98
-- (positions are the DB pos_x/pos_y; the in-game /loc that produced them is Y,X,Z, already swapped.)
--
-- Each row is copied from butcher's stock PoK book (doorid 78) so opentype/dest/incline/size/flags
-- match a known-good book, then position + expansion (-1/-1 = always spawn on Classic) are overridden.
-- The book NAME stays POKTELE500 (the standard PoK-book model, renders in every zone).
-- Butcher now has TWO books (78 "Kaladim" + 179 "Butcherblock Docks"); pok_travel.lua's book_override
-- splits them by doorid, and pok_portals.lua carries the waypoints.
--
-- Doors load at ZONE BOOT (not shared memory) -> just restart zones after. Idempotent.
-- ==========================================================================================

DELETE FROM doors WHERE (zone='butcher' AND doorid=179)
                     OR (zone='ecommons' AND doorid=71)
                     OR (zone='qeytoqrg' AND doorid=14)
                     OR (zone='tutorialb' AND doorid=48);

INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 179, 'butcher', version, name, 2967.91, 1227.09, -2.26, 0, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, -1, -1
FROM doors WHERE zone='butcher' AND doorid=78;

INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 71, 'ecommons', version, name, -196.90, -1512.30, 3.13, 256, opentype, guild, lockpick, keyitem,   -- heading 256 = south
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, -1, -1
FROM doors WHERE zone='butcher' AND doorid=78;

INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 14, 'qeytoqrg', version, name, 92.74, 3157.83, 0.04, 384, opentype, guild, lockpick, keyitem,   -- heading 384 = east (toward Blackburrow)
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, -1, -1
FROM doors WHERE zone='butcher' AND doorid=78;

-- ⚠️⚠️ THE HUB BOOK IS NOW IN `resplendent` (doorid 100, migration v57) -- SEE THE BOTTOM OF THIS FILE.
-- The tutorialb row below is kept but is effectively dead: `aotv4_start_resplendent.sql` made
-- resplendent the start zone and turned the tutorial off, so this book became unreachable and new
-- characters got no starter waypoints and no way to open the Portal window. Reported from play.
--
-- TutorialB HUB book (doorid 48): NOT a travel destination -- clicking it unlocks the three starter
-- zones (Butcherblock Docks + Commonlands + Qeynos Hills) via pok_travel.lua grant_sets, then opens the
-- window so the player teleports OUT. TutorialB is never added to pok_portals, so it is one-way-out
-- (the only way back in is death). dest_zone='poknowledge' just fires event_click_door (which cancels
-- the actual teleport); the book never ports you to PoK or back to the tutorial.
INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 48, 'tutorialb', version, name, -124.44, -94.51, 17.24, 0, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, -1, -1
FROM doors WHERE zone='butcher' AND doorid=78;

-- ==========================================================================================
-- RESPLENDENT HUB book (doorid 100) -- added 2026-08-13, migration v57.
--
-- Replaces the tutorialb book above, which went unreachable when aotv4_start_resplendent.sql made
-- resplendent (729) the start zone for all 1,441 (race, class, deity) rows and disabled the tutorial.
-- Same shape: grants the three starter waypoints via pok_travel grant_sets, opens the window, and is
-- NEVER a destination itself (pok_travel.never_attune lists resplendent, and it is not in
-- pok_portals.lua -- a book *to* the hub would be a free bind-anywhere and would break the roguelite
-- loop of dying to return).
--
-- ⚠️ Position -22/548/0 is the START AND BIND POINT nudged 13 units along y, so the book is in front
-- of an arriving player rather than inside them. That point is known-good standing ground (every
-- character materialises on it and respawns there after every death). Correct with one UPDATE if it
-- reads wrong in game; the three hub NPCs are further in at y ~685-697, z ~-26.
-- ⚠️ /loc prints Y,X,Z -- pos_x/pos_y here are already swapped relative to it.
-- ⚠️ heading 256 = south, facing back toward the arrival point.
--
-- ⚠️⚠️⚠️ SAFE TO RUN ON LIVE, WHERE A HUB BOOK ALREADY EXISTS AT AN UNKNOWN DOORID. §25: live is a
-- DIFFERENT DATABASE. The book was authored there with the door tool (`#door create` + `#door save`)
-- and never came back to dev, so live has a real row and dev never did. The `NOT EXISTS` guard is what
-- stops this adding a SECOND book beside the one players already use -- it keys off
-- `dest_zone = 'poknowledge'` for the ZONE, never off doorid 100, because live's id is unknown.
-- ⚠️ Deliberately NO `DELETE` first: if live's book happened to sit at doorid 100, deleting and
-- re-inserting would silently MOVE a book whose location players already know. Idempotency comes from
-- the guard instead, which touches nothing that is already there.
-- ==========================================================================================
INSERT INTO doors
  (doorid, zone, version, name, pos_x, pos_y, pos_z, heading, opentype, guild, lockpick, keyitem,
   nokeyring, triggerdoor, triggertype, disable_timer, doorisopen, door_param, dest_zone, dest_instance,
   dest_x, dest_y, dest_z, dest_heading, invert_state, incline, size, buffer, client_version_mask,
   is_ldon_door, close_timer_ms, dz_switch_id, min_expansion, max_expansion)
SELECT 100, 'resplendent', src.version, src.name, -22, 548, 0, 256, src.opentype, src.guild, src.lockpick, src.keyitem,
   src.nokeyring, src.triggerdoor, src.triggertype, src.disable_timer, src.doorisopen, src.door_param, src.dest_zone, src.dest_instance,
   src.dest_x, src.dest_y, src.dest_z, src.dest_heading, src.invert_state, src.incline, src.size, src.buffer, src.client_version_mask,
   src.is_ldon_door, src.close_timer_ms, src.dz_switch_id, -1, -1
FROM doors src
WHERE src.zone = 'butcher' AND src.doorid = 78
  AND NOT EXISTS (SELECT 1 FROM doors d2 WHERE d2.zone = 'resplendent' AND d2.dest_zone = 'poknowledge');
