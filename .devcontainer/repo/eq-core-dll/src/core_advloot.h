#pragma once

// core_advloot -- native SIDL Advanced Loot window (AoTv4).
// =========================================================================================
// Its OWN translation unit, like core_allcasters.cpp and core_achievements_native.cpp -- NOT
// piggy-backed on core_spellwindow.cpp. All window state and the AdvLootWnd CCustomWnd live in
// core_advloot.cpp so there is a single definition across our TUs.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and two
// modules cannot hook the same address, so the existing detours call INTO the entry points below:
//
//   InitAdvLoot()            -- core_init.h (InitOptions), gated by areLootWindowEnabled.
//   AdvLootParseTransport()  -- from the dsp_chat swallow chain (core_spellwindow.cpp). Returns true
//                               if it consumed a LOOTDATA / FILTERDATA / LOOTCLOSE line.
//   AdvLootIsOurEcho()       -- true for the "/say als*" echoes, so dsp_chat can swallow them too.
//   AdvLootShow()            -- the "/advl" command (core_bazaar.h): open the window and resync.
//   AdvLootOnUiReset()       -- from the CDisplay CleanGameUI/ReloadUI detour that
//                               core_achievements_native.cpp owns; drops the window object.
//
// Protocol (server <-> dll, carried on chat; the dll swallows all of it):
//   server -> dll : "LOOTDATA <n>^corpseid:lootslot|itemid|icon|name|npcname|qty|locked^..."
//                   "FILTERDATA <n>^itemid|icon|name|rule^..."
//                   "LOOTCLOSE"
//   dll -> server : "/say alspick <corpseid:lootslot> loot|leave|never"
//                   "/say alslootall" , "/say alsrefresh" , "/say alsfilters"
//                   "/say alsfilterdel <itemid>"
//
// The item handle is "<corpse entity id>:<lootslot>" because the personal list spans EVERY corpse the
// player has kill credit for (live's rule), so a bare lootslot would be ambiguous.

void InitAdvLoot();
bool AdvLootParseTransport(const char* message);
bool AdvLootIsOurEcho(const char* message);
void AdvLootShow();
void AdvLootOnUiReset();

// Called every frame from the ProcessGameEvents detour (core_spellwindow.cpp owns that hook).
// Watches the Edit Filters search box so the rule list narrows AS YOU TYPE and restores itself when
// the box is cleared. There is no text-changed notification to hook, so this compares the box against
// the last value seen; it only touches anything while the filters view is actually open.
void AdvLootTick();


// Implemented in core_spellwindow.cpp: queue a "/say ..." to run on the GAME thread from the
// ProcessGameEvents detour. Shared because that detour (and its queue) is owned there; running
// InterpretCmd off the game thread crashes the client.
void AoTQueueGameCommand(const char* command);
