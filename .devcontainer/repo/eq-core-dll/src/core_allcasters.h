#pragma once

// core_allcasters -- "all classes are casters" client patch.
// =========================================================
// SELF-CONTAINED MODULE. The detour lives in its OWN translation unit
// (core_allcasters.cpp) and installs its OWN dedicated detour on
// EQ_Character::IsSpellcaster. It is NOT piggy-backed on core_spellwindow.cpp
// (the spell/AA/portal/shop windows + skill-unlock hook) or core_tradeskill.h,
// and it does not share their dsp_chat / ProcessGameEvents detours.
//
// WHAT IT DOES: RoF2 hides the spellbook window and the spell-gem bar for the
// four pure-melee classes (Warrior, Monk, Rogue, Berserker). The client decides
// that via EQ_Character::IsSpellcaster(), which returns 0 for those classes.
// Forcing it to always return 1 makes the client treat every class as a caster,
// so all classes get a spellbook + spell-gem bar. The server already allows it:
// ScribeSpell has no class check and there is no hard wrong-class cast block.
//
// ADDRESS: EQ_Character::IsSpellcaster @ 0x443F50 (image base 0x400000) in the
// RoF2 eqgame.exe. Found via disassembly -- it reads the class byte (char struct
// +0x3374), then a 16-entry class->bool table at 0x443f90 =
//   {00 01 01 01 01 01 00 01 00 01 01 01 01 01 01 00}
// -> returns 0 for Warrior(1)/Monk(7)/Rogue(9)/Berserker(16), 1 for all others.
// (5 call sites across the spell/UI code.)
#define EQ_Character__IsSpellcaster_addr   0x443F50

// THE MANA GAUGE (companion patch):
// IsSpellcaster=1 gives melee a spellbook + gems + working mana (casting/out-of-mana), but the visible
// mana BAR stays hidden. Reason: the client computes its OWN max mana via EQ_Character::Max_Mana
// (0x581E60) and stores it at char struct +0x338c -- the field the gauge reads. Max_Mana has its own
// class jump table that returns 0 for Warrior/Monk/Rogue/Berserker, so the gauge has max=0 -> no bar.
// We detour Max_Mana: casters pass through; melee (a 0 return) get a flat pool so the gauge renders.
#define EQ_Character__Max_Mana_addr        0x581E60

// Melee mana pool = level * AOTV4_MELEE_MANA_PER_LEVEL, so it GROWS with level like a hybrid
// (SK/Paladin/Ranger) instead of being flat. MUST match the server's CalcMaxMana melee formula
// (zone/client_mods.cpp) exactly -- same constant, same level -- so the client gauge max == the server
// mana ceiling and the bar fills to 100%. (Server + client compute stats with different formulas, so a
// shared level*constant formula is how we keep them in lockstep.) Tune this one number in BOTH places.
// The dll reads the player's level from SPAWNINFO->Level (+0x250) via pinstLocalPlayer (0xDD2630).
#define AOTV4_MELEE_MANA_PER_LEVEL         40

// THE MANA GAUGE (visibility):
// IsSpellcaster + Max_Mana give melee a working mana value (character sheet, casting), but the client
// still hid the mana GAUGE in the compact player window. Traced to CPlayerWnd::Draw (0x718cf0): it shows
// the mana gauge (CPlayerWnd+0x22c "PlayerMana") and mana label (+0x238) only when a separate class
// predicate at 0x59FB90 returns non-zero. That predicate reads the class byte ([this+0xeb8]) through a
// jump table and returns 0 for Warrior(1)/Monk(7)/Rogue(9)/Berserker(16), 1 for every other class -- a
// SECOND caster test, distinct from IsSpellcaster. Detour it to always return 1 -> the mana gauge shows
// for all classes. (5 call sites, all mana-display related; forcing 1 is consistent with all-casters.)
#define EQ_Character__ManaGaugePredicate_addr   0x59FB90

// SPELLS-NOT-SONGS (companion patch):
// The reward pool repurposes caster spells as class-agnostic rewards; they carry a Bard class level (so
// a Bard can memorize them) but use skill 98 -- a NON-song placeholder. The client's EQ_Spell::IsBardSong
// (0x432960) decides song-vs-spell purely from "caster is a Bard AND the spell has a Bard class level" --
// it NEVER looks at the skill -- so a Bard casting one of these gets song behavior (wrong; they have cast
// times + mana costs and should cast as spells). We detour it to mirror the SERVER's skill-gated
// IsBardSong (common/spdat.cpp): keep the stock result, but only genuine Singing/instrument skills stay
// songs. skill 98 (and any non-song skill) -> normal spell. 14 call sites incl. the cast branch; one
// predicate flips them all. Signature: bool __stdcall IsBardSong(SPELL* spell, int class) -- args
// (spell, class), ret 8, ecx ignored. SPELL::Skill is a BYTE at +0x270 (verified this build).
#define EQ_Spell__IsBardSong_addr          0x432960
#define SPELL_Skill_off                    0x270    // SPELL::Skill (BYTE)
// Genuine song skills on THIS client (from the instrument-classification jump tables): Brass=12,
// Singing=41, Stringed=49, Wind=54, Percussion=70. Everything else (incl. the skill-98 reward spells)
// is treated as a normal spell.

// THE COMBAT ABILITIES WINDOW (companion patch):
// ⚠️⚠️ EVERY CLASS HERE GETS THREE DISCIPLINES (44700-44747), BUT ON A PURE CASTER THE COMBAT
// ABILITIES ICON ON THE WINDOW SELECTOR WAS BLACKED OUT AND THE WINDOW WOULD NOT OPEN.
//
// It is the SAME shape as IsSpellcaster, and it is a predicate on the character:
//   CSelectorWnd's update (0x751fc7) calls this on the local PC, and feeds the bool straight into
//   the combat button's state setter (0x866610). Return 0 and the icon is dead.
// The function reads the class byte (char struct +0x3374) and indexes a 16-entry table at
// 0x58238C, jumping through 0x582384 (entry 0 -> return 1, entry 1 -> return 0):
//   00 01 00 00 00 01 00 00 00 01 01 01 01 01 00 00
//   War TRUE  Clr false Pal TRUE  Rng TRUE  SK  TRUE  Dru false Mnk TRUE  Brd TRUE
//   Rog TRUE  Shm false Nec false Wiz false Mag false Enc false Bst TRUE  Ber TRUE
// -> exactly the nine classes that have disciplines on live, which is exactly the set where the
// window worked. Forcing 1 gives every class the button and the window.
// 📌 It sits 5KB from Max_Mana (0x581E60), which this module already detours -- the client keeps
// these little class predicates together, which is why the same file is the right home.
//
// ⚠️⚠️ HOW IT WAS FOUND, BECAUSE THE SEARCH THAT FAILED IS INSTRUCTIVE: hunting for a class gate in
// the WINDOW, the keybind handler and the button turned up nothing (they are all class-blind), and
// four rounds of reasoning from the disassembly produced four wrong answers. What worked was
// following the SELECTOR's button-state update backwards to whoever supplies the bool.
#define EQ_Character__HasCombatAbilities_addr  0x582350

// Installs the IsSpellcaster + Max_Mana + mana-gauge-predicate + IsBardSong detours. Called once at
// init, gated by the areAllClassesCasters flag (see core_init.h / _options.h).
void EnableAllClassesCasters();

