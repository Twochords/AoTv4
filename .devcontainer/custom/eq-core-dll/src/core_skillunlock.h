#pragma once

// Skill unlock / reward-gating.
//
// Implementation lives in core_spellwindow.cpp because it shares that file's CEverQuest::
// dsp_chat hook -- the server sends the player's EARNED combat skills as a chat line
// ("SKILLUNLOCKDATA id,id,..."), which the chat hook parses. The CSkillMgr::GetSkillCap
// hook then reveals a combat skill ONLY if it's in that earned set, so un-earned combat
// skills are hidden from both the Skills window and the ability-hotkey picker until the
// player picks them in the level-up window. Casting/singing/utility skills are left visible
// so spells and songs keep working. Gated by areSkillsUnlocked in _options.h.
void EnableSkillUnlock();
