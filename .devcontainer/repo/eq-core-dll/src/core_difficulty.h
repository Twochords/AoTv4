#pragma once

// core_difficulty -- native SIDL Zone Difficulty window (AoTv4), opened by "/pick".
// =========================================================================================
// Normal / Nightmare / Hell / Inferno. Difficulty is a property of the CHARACTER, not a place: pick
// one and it applies to every zone you enter from then on, by putting you in a private copy of that
// zone shared with everyone else at the same difficulty. Normal is the real zone, no instance.
//
// ⚠️⚠️ THIS REPLACES THE NATIVE /pick, WHICH CANNOT DO THE JOB. `OP_PickZoneWindow` is mapped for
// RoF2 (0x72d8) and `OP_PickZone` for the reply (0xaaba), but `PickZoneEntry_Struct` carries only
// {zone_id, player_count, instance_id} -- there is NO text field -- so all four difficulties would
// render as the same zone name with different populations and nothing on screen could say which was
// which. `Handle_OP_PickZone` is a bare stub server side (`// handle`) and nothing in the codebase
// has ever sent the window packet. The InterpretCmd detour swallows "/pick" so the native command
// never reaches the client's own handler.
//
// ⚠️ THE LIST IS SERVER-AUTHORED. Names, health multiples, level bumps and descriptions all arrive
// over DIFFDATA, so retuning a difficulty is a Lua edit and needs no dll rebuild. The dll keeps no
// copy of the ladder -- the kIcons[] drift trap (CLAUDE.md section 3) applies here exactly as it did
// to the reward picker.
//
// Its OWN translation unit, like core_advloot.cpp / core_autoskill.cpp / core_dungeon.cpp.
//
// It installs NO detours. This dll already owns dsp_chat, InterpretCmd and ProcessGameEvents, and
// two modules cannot hook the same address, so the existing detours call INTO the entry points
// below:
//
//   InitDifficulty()            -- core_init.h (InitOptions), gated by areDifficultyWindowEnabled.
//   DifficultyParseTransport()  -- from the dsp_chat swallow chain. True if it ate a DIFFDATA line.
//   DifficultyIsOurEcho()       -- true for the "/say diff*" echoes, so dsp_chat swallows them too.
//   DifficultyShow()            -- the "/pick" command (core_bazaar.h): open and ask for the list.
//   DifficultyOnUiReset()       -- from the CleanGameUI/ReloadUI detour core_achievements_native.cpp
//                                  owns; DELETES the window object (nulling alone leaves it
//                                  registered with the window manager after its widgets are freed).
//
// Protocol (chat; the dll swallows all of it):
//   server -> dll : "DIFFDATA <current>^id|name|hp|lvl|blurb^..."
//   dll -> server : "/say diffwin"        (ask for the ladder)
//                   "/say diffset <id>"   (switch)
//
// ⚠️ The dll validates NOTHING. The server refuses a switch in combat or inside a delve, and
// re-checks the id, so a modified client pressing Travel gains only a refusal in chat.

void InitDifficulty();
bool DifficultyParseTransport(const char* message);
bool DifficultyIsOurEcho(const char* message);
void DifficultyShow();
void DifficultyOnUiReset();
