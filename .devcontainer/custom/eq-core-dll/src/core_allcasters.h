#pragma once

#include "MQ2Main.h"

// areAllClassesCasters
// --------------------
// RoF2 hides the spellbook window and the spell-gem bar for the four pure-melee
// classes (Warrior, Monk, Rogue, Berserker). The client decides that via
// EQ_Character::IsSpellcaster(), which returns 0 for those classes.
//
// Forcing IsSpellcaster() to always return 1 makes the client treat every class
// as a caster, so all classes get a spellbook + spell-gem bar. The server side
// already allows it: ScribeSpell has no class check and there is no hard
// wrong-class cast block (class only feeds fizzle math).
//
// ADDRESS: this is the static (image base 0x400000) address of
// EQ_Character::IsSpellcaster in the RoF2 "May 10 2013" eqgame.exe. It is NOT in
// this stripped SDK's eqgame.h, so it must be supplied. Easiest source: a full
// MacroQuest RoF2 eqgame.h (look for EQ_Character__IsSpellcaster_x). Otherwise
// locate it in Ghidra/IDA (see notes from chat). Until it is set, leave
// areAllClassesCasters = false so the 0 address is never detoured.
#define EQ_Character__IsSpellcaster_addr   0x000000   // <-- TODO: fill in real address

int __fastcall EnableAllClassesCasters_Trampoline(void* pThis);
int __fastcall EnableAllClassesCasters_Detour(void* pThis) { return 1; }
DETOUR_TRAMPOLINE_EMPTY(int __fastcall EnableAllClassesCasters_Trampoline(void* pThis));

// Hooks EQ_Character::IsSpellcaster to always report "yes, a caster".
void EnableAllClassesCasters()
{
	EzDetour(
		(((DWORD)EQ_Character__IsSpellcaster_addr - 0x400000) + baseAddress),
		EnableAllClassesCasters_Detour,
		EnableAllClassesCasters_Trampoline
	);
}
