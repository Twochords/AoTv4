#pragma once

// core_meter -- the Damage Meter window (SIDL "AoTMeterWnd"), opened by "/meter" or the AoT menu.
// =========================================================================================
// Who contributed what to the creature you are fighting. Every number is computed SERVER side and
// arrives as one METERDATA line per second while the window is open; this module sorts and renders
// and does nothing else.
//
// 📌 Fed from the same server choke point as the floating combat text, so the two cannot disagree
// about what happened -- if a hit produced a number on screen it is in the meter, and vice versa.
//
// ⚠️ Its OWN translation unit with NO detours, like every other native window here. This dll already
// owns dsp_chat, InterpretCmd, ProcessGameEvents and the CleanGameUI/ReloadUI pair, so those call in
// through the entry points below.

// Create if needed and show. Also tells the server to start sending (it does not send otherwise).
void MeterWindowShow();

// Per-frame. Re-tells the server we want data while the window is visible, and tells it to stop when
// the window is closed. The opt-in lives on the server's Client, which is rebuilt on every zone.
void MeterWindowTick();

// Swallow and act on one METERDATA line. True when the line was ours.
bool MeterParseTransport(const char* msg);
bool MeterParseList(const char* msg);      // METERLIST   -- the fight picker
bool MeterParseDetail(const char* msg);    // METERDETAIL -- one player's breakdown

// Called from the CleanGameUI / ReloadUI detour that core_achievements_native.cpp owns.
// ⚠️⚠️ MUST `delete`, not just null the pointer -- nulling leaks the CCustomWnd and leaves it
// registered with the client's window manager after its widgets are freed.
void MeterWindowOnUiReset();
