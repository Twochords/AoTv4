#pragma once

// core_lostwindow -- native SIDL "You Lost" window (AoTv4).
// =========================================================================================
// Replaces the self-drawn GDI overlay that used to show the death-loss list (PaintLostOverlay /
// LostOverlayWndProc in core_spellwindow.cpp). That overlay painted its own chrome, could not scale
// with the UI, and only worked in windowed mode because this client's D3D device cannot be hooked.
// A SIDL CCustomWnd sidesteps all of it and looks native because it IS native -- the same move the
// level-up picker, Advanced Loot, Shop and Achievement windows already made.
//
// Its OWN translation unit, like core_advloot.cpp and core_achievements_native.cpp, so there is a
// single definition across our TUs.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points
// below:
//
//   InitLostWindow()          -- core_init.h (InitOptions), gated by areLostWindowEnabled.
//   LostParseTransport()      -- from the dsp_chat swallow chain (core_spellwindow.cpp). Returns
//                                true if it consumed a LOSTDATA line.
//   LostWindowShow()          -- Ctrl+Q fallback when no level-up reward is pending.
//   LostWindowHasContent()    -- whether there is anything to show, so the Ctrl+Q handler can skip
//                                opening an empty window.
//   LostWindowTick()          -- every frame from ProcessGameEvents. Applies a pending expand or
//                                collapse: the list CANNOT be rebuilt inside its own click handler.
//   LostWindowOnUiReset()     -- from the CDisplay CleanGameUI/ReloadUI detour that
//                                core_achievements_native.cpp owns; drops the window object.
//
// Protocol (unchanged from the GDI version -- this was a CLIENT-side change only, the server was
// not touched):
//   server -> dll : "LOSTDATA name^name^..."
//
// There is no dll -> server direction. The window is read-only: it reports what death already took.
//
// ⚠️ Client install: copy EQUI_AoTLostWnd.xml to <EQ>/uifiles/default/ AND add
// <Include>EQUI_AoTLostWnd.xml</Include> to EQUI.xml. Missing either and CCustomWnd cannot find its
// screen and returns SILENTLY -- no error, no window, nothing in any log.

void InitLostWindow();
bool LostParseTransport(const char* message);
// Advancement tab: swallows "AACHOICEDATA budget^name|icon|cost|cls^..." and "AADESCDATA d^d^d".
// ⚠️ Call this from the dsp_chat swallow chain BEFORE anything that might print the line -- these
// were already being swallowed and discarded when the old GDI AA window was retired, so if the tab
// shows nothing the first thing to check is whether an earlier handler is still eating them.
bool AAParseTransport(const char* message);
// "AAPOPUP" -- sent ONLY when points were actually earned (aa_choice.grant_picks), never on a plain
// re-send of the sticky offer. Opens the window on the Advancement tab. Kept separate from
// AACHOICEDATA precisely so logging in and levelling up do not throw the window open.
bool AAParsePopup(const char* message);
bool LostParseLog(const char* message);   // dsp_chat: "LOSTLOG <chunk> <chunks> <total>^epoch|name^..."
void LostWindowShow();
bool LostWindowHasContent();
void LostWindowTick();       // per frame; applies a deferred expand/collapse (never rebuild in a click)
void LostWindowOnUiReset();
