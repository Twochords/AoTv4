#pragma once

// core_fctwindow -- the Combat Text window (SIDL "AoTFctWnd"), opened by "/fct" or the AoT menu.
// =========================================================================================
// Controls the floating damage and healing numbers core_floatingtext draws in the world: typeface and
// size, how numbers travel, how long they last, and whether they follow what they happened to or
// cascade from two fixed spots on screen.
//
// ⚠️ Its OWN translation unit with NO detours of its own, like every other native window here. This
// dll already owns dsp_chat, InterpretCmd, ProcessGameEvents and the CleanGameUI/ReloadUI pair, and
// two modules cannot hook the same address -- so those call in through the entry points below.
//
// 📌 It owns no state. Every setting lives in core_floatingtext.cpp and is written to
// aotv4_floatingtext.ini; this file is the surface, not the source of truth. That is deliberate: /fct
// as a text command and the window both change the same variables, so the two can never disagree.

// Create the window if needed and show it. Safe to call repeatedly.
void FctWindowShow();

// Per-frame, from ProcessGameEvents. Drives ONLY the "place an anchor" click capture; the window is
// otherwise event driven and costs nothing per frame.
void FctWindowTick();

// Show or hide the two draggable anchor markers.
void FctShowAnchors(bool on);

// Called from the CleanGameUI / ReloadUI detour that core_achievements_native.cpp owns.
// ⚠️⚠️ MUST `delete`, not just null the pointer. Nulling leaks the CCustomWnd AND leaves it registered
// with the client's window manager after its widgets are freed, so the next thing to walk the window
// list touches freed memory. The Delve window was the odd one out here once already.
void FctWindowOnUiReset();
