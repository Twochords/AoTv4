#pragma once

// core_achievements_native -- native SIDL achievement window (AoTv4).
// ===================================================================
// The implementation + all file-scope state live in core_achievements_native.cpp
// (single definition across our multiple translation units). This header declares
// ONLY the cross-TU entry points the rest of the dll calls:
//
//   InitAchievementsNative()               -- wired in core_init.h (InitOptions),
//                                             gated by areAchievementsNativeEnabled.
//   NativeAchievementParseTransport(msg)   -- called from the dsp_chat swallow chain
//                                             (core_spellwindow.cpp); returns true if
//                                             it consumed an "ACH|..." transport line.
//   NativeAchievementHandleLocalCommand()  -- called from InterpretCmd_TA_Detour
//                                             (core_bazaar.h); handles "/ach close".
//   NativeAchievementRewriteCommand()      -- called from InterpretCmd_TA_Detour; maps
//                                             /achievement /ach ... to a "/say #ach ..."
//                                             the server understands.
//
// The module does NOT install its own dsp_chat / InterpretCmd detours (our dll already
// owns those); it only installs the CDisplay CleanGameUI/ReloadUI reset hook internally.

#include <cstddef>   // size_t

void InitAchievementsNative();
bool NativeAchievementParseTransport(const char* message);
bool NativeAchievementHandleLocalCommand(const char* line);
bool NativeAchievementRewriteCommand(const char* line, char* output, size_t output_size);
