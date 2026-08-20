#pragma once

// core_autoskill -- native SIDL Autoskill window (AoTv4).
// =========================================================================================
// The autoskill SYSTEM already existed server side before this window: per-skill on/off persisted
// in "autoskill.<id>" data buckets (Client::Get/SetAutoSkillStatus), and the auto-fire loop in
// Client::Process that runs your enabled specials while you are auto-attacking a non-client target.
// `#autoskill [skill] [enable|disable|status]` still works and is unaffected.
//
// This is only the WINDOW: see at a glance which skills are on, toggle them, and watch the reuse
// timers count down.
//
// Its OWN translation unit, like core_advloot.cpp and core_lostwindow.cpp.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points
// below:
//
//   InitAutoSkill()            -- core_init.h (InitOptions), gated by areAutoSkillWindowEnabled.
//   AutoSkillParseTransport()  -- from the dsp_chat swallow chain. True if it ate an ASKILLDATA line.
//   AutoSkillIsOurEcho()       -- true for the "/say ask*" echoes, so dsp_chat swallows them too.
//   AutoSkillShow()            -- the "/autoskill" command (core_bazaar.h): open and resync.
//   AutoSkillTick()            -- every frame from ProcessGameEvents; counts the timers down locally.
//   AutoSkillOnUiReset()       -- from the CleanGameUI/ReloadUI detour core_achievements_native.cpp
//                                 owns; drops the window object.
//
// Protocol (chat; the dll swallows all of it):
//   server -> dll : "ASKILLDATA <n>^skillid|name|enabled|cooldown_secs|reuse_secs^..."
//   dll -> server : "/say askset <skillid> <0|1>" , "/say askrefresh"
//
// ⚠️ THE COUNTDOWN IS RUN LOCALLY, not pushed. The server sends the remaining seconds once per
// update and the client ticks it down per frame. Pushing a fresh ASKILLDATA every second would be a
// chat line every second per player -- and the RoF2 chat pipe DROPS AND REORDERS bursts, which is
// the same failure that made achievement completions "update only sometimes" (CLAUDE.md section 15).

void InitAutoSkill();
bool AutoSkillParseTransport(const char* message);
bool AutoSkillIsOurEcho(const char* message);
void AutoSkillShow();
void AutoSkillTick();
void AutoSkillOnUiReset();
