#pragma once

// core_travel -- native SIDL Travel window (AoTv4), opened ONLY by clicking a Plane of Knowledge book.
// =========================================================================================
// Regions on the left, that region's destinations on the right. Destinations are hidden waypoints
// discovered by WALKING OVER them out in the world; the books are the terminals you travel from.
//
// ⚠️⚠️ THERE IS DELIBERATELY NO COMMAND THAT OPENS THIS. "/travel" is a printout and nothing else
// (server side, aotv4_travel.lua). A window reachable from anywhere would make the book irrelevant,
// which is the entire shape of the design -- so the ONLY entry point is TravelShow(), called from the
// dsp_chat handler when the server sends TRAVELOPEN after a book click.
//
// ⚠️ THE LISTS ARE SERVER-AUTHORED. Region names, which are open to you, which destinations you have
// found and how many you have not all arrive over the wire, so adding a waypoint is a Lua edit and
// needs no dll rebuild. The dll keeps no copy of the spot table -- the kIcons[] drift trap
// (CLAUDE.md section 3) applies here exactly as it did to the reward picker.
//
// ⚠️⚠️ UNDISCOVERED DESTINATIONS NEVER ARRIVE BY NAME. The server sends a COUNT of what is still
// unfound in each region and the dll renders that many "Unknown" rows. Sending the names and hiding
// them client side would put the answer in the packet, where a modified dll could read it -- the
// point of a hidden waypoint is that you have to walk to it.
//
// Its OWN translation unit, like core_advloot.cpp / core_difficulty.cpp / core_dungeon.cpp.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points below:
//
//   InitTravel()            -- core_init.h (InitOptions), gated by areTravelWindowEnabled.
//   TravelParseTransport()  -- from the dsp_chat swallow chain. True if it ate one of our lines.
//   TravelIsOurEcho()       -- true for the "/say travel*" echoes, so dsp_chat swallows them too.
//   TravelShow()            -- open it. Called on TRAVELOPEN, not from any command.
//   TravelOnUiReset()       -- from the CleanGameUI/ReloadUI detour core_achievements_native.cpp
//                              owns; DELETES the window object (nulling alone leaves it registered
//                              with the window manager after its widgets are freed).
//
// Protocol (chat; the dll swallows all of it):
//   server -> dll : "TRAVELOPEN"                                  (a book was clicked: show)
//                   "TRAVELDATA <n>^rid|name|open|found|unknown^..."
//                   "TRAVELDEST <n>^rid|id|name^..."              (DISCOVERED ones only)
//   dll -> server : "/say travelwin"                              (ask for both lists)
//                   "/say travelgo <id> <group> <auto>"           (request a trip)
//
// ⚠️ The dll validates NOTHING. The server re-checks that you found the place, that its region is
// open to you, that you are not in combat, and -- for a group trip -- that EVERY member qualifies,
// naming any who do not. A modified client pressing Travel gains only a refusal in chat.
// ⚠️ The confirmation box is the ENGINE'S popup, raised server side (eq.popup + EVENT_POPUP_RESPONSE,
// the same mechanism guildhall/#Porter.lua uses). It is not drawn here and cannot be skipped by a
// modified client: unchecking Auto Confirm simply means the server asks before moving you.

void InitTravel();
bool TravelParseTransport(const char* message);
bool TravelIsOurEcho(const char* message);
void TravelShow();        // opened by a PoK book: travel enabled
void TravelShowBrowse();  // opened from the /aot menu: read only, Travel button hidden
void TravelOnUiReset();
