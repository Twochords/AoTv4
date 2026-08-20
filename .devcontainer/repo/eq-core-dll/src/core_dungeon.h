#pragma once

// core_dungeon -- native SIDL Delve window (AoTv4 scaling dungeon).
// =========================================================================================
// Pick a level from the list, press Enter, and the server drops you into a private INSTANCE of a
// Dragons of Norrath zone with every mob in it scaled to that level and a real quest in the journal.
// Exit closes the instance and puts you back where you came from.
//
// ⚠️ THE LIST IS SERVER-AUTHORED. Layers unlock in order -- layer N+1 only exists once layer N has
// been cleared -- and the server sends ONLY the unlocked ones. The window cannot show, or ask for, a
// layer it was never told about, and the server re-checks the unlock on entry regardless. So a
// modified client gains nothing here: the same "ids live server side, the client sends an index"
// property the level-up reward picker has (CLAUDE.md section 3).
//
// Its OWN translation unit, like core_advloot.cpp / core_autoskill.cpp / core_lostwindow.cpp.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points
// below:
//
//   InitDungeon()             -- core_init.h (InitOptions), gated by areDungeonWindowEnabled.
//   DungeonParseTransport()   -- from the dsp_chat swallow chain. True if it ate a DUNGDATA line.
//   DungeonIsOurEcho()        -- true for the "/say delve*" echoes, so dsp_chat swallows them too.
//   DungeonShow()             -- the "/delve" command (core_bazaar.h): open and ask for the list.
//   DungeonTick()             -- every frame from ProcessGameEvents; applies a deferred row toggle.
//   DungeonOnUiReset()        -- from the CleanGameUI/ReloadUI detour core_achievements_native.cpp
//                                owns; drops the window object.
//
// Protocol (chat; the dll swallows all of it):
//   server -> dll : "DUNGDATA <unlocked>^level|name|cleared^..."
//                   "DLVHIST <n>^when|level|dungeon|kills|score|avg|lo|hi|outcome|bands^..."
//   dll -> server : "/say delve"  (ask for BOTH the list and the history)
//                   "/say delveenter <level>"
//                   "/say delveexit"
//                   "/say delvehist"  (history only)
//
// TWO TABS: Layers (pick and enter) and Score Sheet (what every finished run was worth).
//
// ⚠️ The Score Sheet is modelled on the Death Book (core_lostwindow.cpp): one header row per run,
// `[+]`/`[-]`, and only an open run lists its level-band breakdown -- so twenty runs read as a table
// of contents rather than a wall of numbers. The same three traps apply and are handled the same way:
// the toggle is DEFERRED to DungeonTick (Refresh calls DeleteAll, which would destroy the listbox's
// rows from inside its own click notification), the selection is read AFTER the base class has run
// (inside the notification GetCurSel is one click behind), and the row index is NOT a data index
// (headers interleave and a closed run hides its bands), so every click goes through g_histRowRun[].

void InitDungeon();
bool DungeonParseTransport(const char* message);
bool DungeonIsOurEcho(const char* message);
void DungeonShow();
void DungeonTick();
void DungeonOnUiReset();
