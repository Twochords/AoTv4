#pragma once

// core_spellchoice_native -- native SIDL level-up reward picker (AoTv4).
// =========================================================================================
// Replaces the self-drawn GDI spell-choice overlay in core_spellwindow.cpp with a real
// CCustomWnd over uifiles/default/EQUI_AoTSpellChoiceWnd.xml, so it uses the client's own
// chrome, fonts and scrolling instead of imitating them.
//
// Its OWN translation unit, like core_advloot / core_allcasters / core_achievements_native.
// It installs NO detours: this dll already owns dsp_chat / InterpretCmd / ProcessGameEvents and
// two modules cannot hook the same address, so those existing detours call in here.
//
//   InitSpellChoiceNative()      -- core_init.h (InitOptions), gated by areSpellChoiceWindowEnabled.
//   SpellChoiceParseTransport()  -- from the dsp_chat swallow chain; consumes SPELLCHOICEDATA and
//                                   SPELLDESCDATA.
//   SpellChoiceShow()            -- reopen the window on demand (a command can hook this).
//   SpellChoiceOnUiReset()       -- from the CleanGameUI / ReloadUI detour owned by
//                                   core_achievements_native.cpp; drops the window object.
//
// The wire format is UNCHANGED from the GDI version, so no server edit is needed:
//   server -> dll : "SPELLCHOICEDATA name|icon^name|icon^name|icon"
//                   "SPELLDESCDATA  desc^desc^desc"
//   dll -> server : "/say spellpick <1-3>"   (queued onto the game thread)
//
// Random AA is retired, so there is deliberately no AA equivalent here.

void InitSpellChoiceNative();
bool SpellChoiceParseTransport(const char* message);

// SPELLRANKCOST / SPELLRANKDATA -- the permanent spell library, per-spell ranks, carried materials
// and the upgrade cost table, for the Known tab. Consumes the line and repaints the tab.
// ⚠️ Call from the dsp_chat swallow chain like the others; unswallowed it dumps 50 tuples per chunk
// into the player's chat log.
bool SpellChoiceRankTransport(const char* message);
void SpellChoiceShow();
void SpellChoiceOpen();   // open for browsing even with no reward pending (the Known/Pool tabs)
bool SpellChoicePending();   // true while the server has offered a reward that is not yet picked
void SpellChoiceOnUiReset();
