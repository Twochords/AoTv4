#pragma once

// core_allaclone -- native SIDL "Allaclone" window (AoTv4).
// =========================================================================================
// An in-game item / NPC / spell / recipe lookup, named after the Allakhazam-style databases players
// would otherwise alt-tab to. It replaces the self-drawn GDI "Search" overlay that lived in
// core_spellwindow.cpp; that overlay is now unreachable and should be deleted once this is confirmed.
//
// ⚠️ THE WIRE FORMAT IS UNCHANGED. The server (Client::SearchDetail and the "srch"/"srchdet" say
// handlers) was NOT touched by the rewrite -- only the thing that draws the answer. That is
// deliberate: a rendering change should not be able to break a working query path.
//
// Why a real window rather than an overlay, same three reasons as the Lost window before it:
//   * self-drawn chrome never matches EQ, and cannot follow the player's UI skin or scale
//   * a layered top-most window only works in WINDOWED mode
//   * scrolling, dragging and focus are all re-implemented by hand, badly, and each was its own bug
// The UI engine does all of that for free.
//
// Its OWN translation unit, like core_advloot.cpp / core_autoskill.cpp / core_lostwindow.cpp.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points below:
//
//   InitAllaclone()            -- core_init.h (InitOptions), gated by areSearchWindowEnabled.
//   AllacloneParseTransport()  -- from the dsp_chat swallow chain. True if it ate SRCHDATA/SRCHDET.
//   AllacloneIsOurEcho()       -- true for the "/say srch*" echoes, so dsp_chat swallows them too.
//   AllacloneShow()            -- the "/allaclone" command and the AoT menu button.
//   AllacloneTick()            -- every frame from ProcessGameEvents; see below, it only polls a box.
//   AllacloneOnUiReset()       -- from the CleanGameUI/ReloadUI detour core_achievements_native.cpp
//                                 owns; drops the window object.
//
// Protocol (chat; the dll swallows all of it):
//   dll -> server : "/say srch <kind> <term>"  ,  "/say srchdet <kind> <id>"
//   server -> dll : "SRCHDATA <kind>^id|name^id|name^..."
//                   "SRCHDET  <kind>|<id>|<line~line~line>"
//   kind is one of: item npc spell recipe
//
// ⚠️ ENTER IN THE SEARCH BOX IS POLLED, NOT HOOKED. An Editbox on this build gives us no usable
// "text committed" notification, so AllacloneTick watches the box and fires when the text settles
// after a change. That is the same approach the Advanced Loot filter box already uses, and it costs
// nothing while the window is closed.

void InitAllaclone();
bool AllacloneParseTransport(const char* message);
bool AllacloneIsOurEcho(const char* message);
void AllacloneShow();
void AllacloneTick();
void AllacloneOnUiReset();
