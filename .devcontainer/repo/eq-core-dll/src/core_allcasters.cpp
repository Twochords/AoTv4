// core_allcasters.cpp -- standalone translation unit for the "all classes are
// casters" client patch. Detours EQ_Character::IsSpellcaster (0x443F50) to always
// return 1 so every class (incl. Warrior/Monk/Rogue/Berserker) gets a spellbook
// and spell-gem bar. See core_allcasters.h for the full rationale + address decode.
//
// Kept deliberately separate from core_spellwindow.cpp and core_tradeskill.h: its
// own file, its own dedicated detour, no shared hooks.

#include "MQ2Main.h"
#include "core_allcasters.h"

#include <cstdio>
#include <cstdarg>

// ⚠️⚠️ THE COMBAT ABILITIES WINDOW IS NEVER "ACTIVATED" FOR A CLASS THAT HAS NO DISCIPLINES ON LIVE,
// and that one byte is why a Cleric saw a blacked-out icon on the Window Selector and got nothing
// from Alt+C. See core_allcasters.h for the full trace. Both the selector button and the window's
// own drawing read `CSidlScreenWnd+0x1d4`, so setting it fixes both at once.
// ⚠️ Re-asserted every frame on purpose -- the client clears it on deactivate (0x864150) and a UI
// reload rebuilds the window with it at 0. The cost is two pointer reads per frame.
// HasCombatAbilities detour: "yes" for every class -> the Window Selector lights the Combat
// Abilities icon and the window opens. Same one-line shape as IsSpellcaster, same reason.
int __fastcall HasCombatAbilities_Trampoline(void* pThis);
int __fastcall HasCombatAbilities_Detour(void* pThis) { return 1; }
DETOUR_TRAMPOLINE_EMPTY(int __fastcall HasCombatAbilities_Trampoline(void* pThis));

// IsSpellcaster detour: report "yes, a caster" for every class -> spellbook + gems + casting.
int __fastcall EnableAllClassesCasters_Trampoline(void* pThis);
int __fastcall EnableAllClassesCasters_Detour(void* pThis) { return 1; }
DETOUR_TRAMPOLINE_EMPTY(int __fastcall EnableAllClassesCasters_Trampoline(void* pThis));

// Max_Mana detour: casters return their real value unchanged; the four pure-melee classes return 0
// (which hides the mana gauge), so replace a <=0 result with the flat pool. thiscall is modelled as
// __fastcall(this, edx, arg) -- same convention as the GetSkillCap detour in core_spellwindow.cpp.
int __fastcall MaxManaAllClasses_Trampoline(void* pThis, void* edx, int arg);
int __fastcall MaxManaAllClasses_Detour(void* pThis, void* edx, int arg)
{
	int m = MaxManaAllClasses_Trampoline(pThis, edx, arg);
	if (m <= 0) {
		// Melee class (Max_Mana returns 0). Scale a pool by the local player's level so it grows like a
		// hybrid, matching the server's CalcMaxMana melee formula (level * AOTV4_MELEE_MANA_PER_LEVEL).
		// Max_Mana is only ever called for the local player, so read SPAWNINFO->Level (+0x250) off
		// pinstLocalPlayer (0xDD2630) -- same local-player pointer core_spellwindow.cpp uses.
		void* sp = *(void**)((0xDD2630 - 0x400000) + baseAddress);
		int level = sp ? *(unsigned char*)((char*)sp + 0x250) : 1;
		if (level < 1) level = 1;
		m = level * AOTV4_MELEE_MANA_PER_LEVEL;
	}
	return m;
}
DETOUR_TRAMPOLINE_EMPTY(int __fastcall MaxManaAllClasses_Trampoline(void* pThis, void* edx, int arg));

// Mana-gauge predicate detour: CPlayerWnd::Draw shows the compact-window mana gauge only when this
// class->bool "has mana" predicate returns non-zero (it returns 0 for the four melee classes). Force 1
// so the mana gauge renders for every class. Same shape as the IsSpellcaster detour.
int __fastcall ManaGaugePredicate_Trampoline(void* pThis);
int __fastcall ManaGaugePredicate_Detour(void* pThis) { return 1; }
DETOUR_TRAMPOLINE_EMPTY(int __fastcall ManaGaugePredicate_Trampoline(void* pThis));

// IsBardSong detour: mirror the server's skill-gated song test so the skill-98 reward spells cast as
// normal spells (not songs) even for a Bard, while still being memorizable (they keep their Bard level).
// Keep the stock result; a song stays a song ONLY if its skill is a genuine Singing/instrument skill.
// bool __stdcall IsBardSong(SPELL* spell, int class): args (spell, class), caller cleans 8 (ret 8).
int __stdcall IsBardSong_Trampoline(void* spell, int eqClass);
int __stdcall IsBardSong_Detour(void* spell, int eqClass)
{
	if (!IsBardSong_Trampoline(spell, eqClass)) return 0;   // stock says not a song -> not a song
	if (!spell) return 0;
	unsigned char skill = *((unsigned char*)spell + SPELL_Skill_off);
	switch (skill) {
		case 12:   // Brass Instruments
		case 41:   // Singing
		case 49:   // Stringed Instruments
		case 54:   // Wind Instruments
		case 70:   // Percussion Instruments
			return 1;   // genuine song
		default:
			return 0;   // skill 98 placeholder & every non-song skill -> normal spell
	}
}
DETOUR_TRAMPOLINE_EMPTY(int __stdcall IsBardSong_Trampoline(void* spell, int eqClass));

void EnableAllClassesCasters()
{
	// spellbook + gems + casting for every class
	EzDetour(
		(((DWORD)EQ_Character__IsSpellcaster_addr - 0x400000) + baseAddress),
		EnableAllClassesCasters_Detour,
		EnableAllClassesCasters_Trampoline
	);
	// mana gauge VALUE for the melee classes
	EzDetour(
		(((DWORD)EQ_Character__Max_Mana_addr - 0x400000) + baseAddress),
		MaxManaAllClasses_Detour,
		MaxManaAllClasses_Trampoline
	);
	// mana gauge VISIBILITY: force the "has mana" predicate CPlayerWnd::Draw uses to always say yes
	EzDetour(
		(((DWORD)EQ_Character__ManaGaugePredicate_addr - 0x400000) + baseAddress),
		ManaGaugePredicate_Detour,
		ManaGaugePredicate_Trampoline
	);
	// COMBAT ABILITIES: the predicate CSelectorWnd feeds into the combat button's state
	EzDetour(
		(((DWORD)EQ_Character__HasCombatAbilities_addr - 0x400000) + baseAddress),
		HasCombatAbilities_Detour,
		HasCombatAbilities_Trampoline
	);
	// SPELLS-NOT-SONGS: skill-gate the client's IsBardSong so skill-98 reward spells cast as spells
	EzDetour(
		(((DWORD)EQ_Spell__IsBardSong_addr - 0x400000) + baseAddress),
		IsBardSong_Detour,
		IsBardSong_Trampoline
	);
}
