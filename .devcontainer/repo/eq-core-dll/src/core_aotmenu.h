#pragma once

// core_aotmenu -- the AoTv4 launcher (AoTMenuWnd).
// =========================================================================================
// One button per custom window, so none of them needs a typed command. Every window this dll adds
// was reachable only by remembering its slash command, which is fine for us and useless for a
// player who does not know the commands exist.
//
// A narrow vertical strip, meant to sit under the stock SC / EQ buttons so it reads as part of that
// bar. It is draggable and the client persists its position, so it is placed once.
//
// It opens BY ITSELF the first time the player enters the world each session, so nothing has to be
// typed to reach any of our windows -- that is the point of the module. Closing it keeps it closed;
// /aot or /menu reopens it, and a UI reload re-arms the auto show.
//
// ⚠️⚠️ IT DOES NOT PUT A BUTTON IN THE STOCK SC / EQ BAR, AND THAT MUST NOT BE RE-ATTEMPTED. It was
// built that way once: an EQM_AoTButton added to EQUI_EQMainWnd.xml, with its click caught by copying
// EQMainWnd's vtable and overwriting entry 34 (the slot SetWndNotification writes), which is needed
// because CCustomWnd::ReplacevfTable only ever operates on `this`. IT CRASHED ON THE FIRST CLICK.
// That approach patches a window this dll did not construct, on a build whose UI structs are already
// documented as not matching the headers (CLAUDE.md section 13), and it can only fail at runtime --
// no static check would catch it. This window gets its notifications the ordinary way instead and
// cannot corrupt anything stock.
//
// Its OWN translation unit. It installs NO detours: the dll already owns dsp_chat / InterpretCmd /
// ProcessGameEvents and two modules cannot hook the same address, so the existing ones call in.
//
//   InitAoTMenu()        -- core_init.h (InitOptions), gated by areAoTMenuEnabled.
//   AoTMenuShow()        -- the "/aot" and "/menu" commands (core_bazaar.h).
//   AoTMenuOnUiReset()   -- from the CleanGameUI / ReloadUI detour core_achievements_native.cpp
//                           owns; drops the window object.
//
// No protocol: every button either calls another module's show function directly or queues the
// command that module already answers to.

void InitAoTMenu();
void AoTMenuShow();
bool AoTMenuParseTransport(const char* message);  // dsp_chat: "AOTMENUSHOW" opens the menu
void AoTMenuOnUiReset();
