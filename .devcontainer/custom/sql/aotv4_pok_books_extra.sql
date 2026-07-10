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
