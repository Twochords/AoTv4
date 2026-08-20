#include "core_bazaar.h"
#include "core_map.h"
#include "core_npc.h"
#include "core_item.h"
#include "core_eqg_load.h"
#include "core_zone.h"
#include "core_luclin_models.h"
#include "core_allcasters.h"
#include "core_skillunlock.h"
#include "core_tradeskill.h"
#include "core_spellchoice.h"
#include "core_achievements_native.h"
#include "core_advloot.h"
#include "core_spellchoice_native.h"
#include "core_spelljournal.h"
#include "core_aotmenu.h"
#include "core_dungeon.h"
#include "core_difficulty.h"
#include "core_fellowship.h"
#include "core_travel.h"
#include "_options.h"

// InitOptions is called during the initialization of this hook
void InitOptions() {
	if (areLuclinModelsDisabled) DisableLuclinModels();
	if (isMapWindowDisabled) DisableCMapViewWnd();
	if (areCustomZonesEnabled) InjectCustomZones();
	if (areCustomNPCsEnabled) InjectCustomNPCs();
	if (areCustomOldAnimationsEnabled) InjectCustomOldAnimations();
	if (isBazaarWindowDisabled) DisableCBazaarSearchWnd();
	if (areTradeAnywhereEnabled) EnableTradeAnywhere();   // /trader + /buyer in any city (needs CXWnd__Show_x)
	if (isEQGOverrideEnabled) InjectEQGOrderLoading();
	if (areCustomShieldsEnabled) InjectCustomShields();
	if (areAllClassesCasters) EnableAllClassesCasters();
	if (areSkillsUnlocked) EnableSkillUnlock();
	if (areAchievementsNativeEnabled) InitAchievementsNative();
	if (areTradeskillsUnlocked) EnableTradeskillUnlock();   // class/race-locked tradeskills (Tinkering/Alchemy/Make Poison)
	if (areLostWindowEnabled) EnableLostWindow();            // read-only death-loss list
	if (areVendorWindowEnabled) EnableVendorWindow();
	if (areSearchWindowEnabled) EnableSearchWindow();
	if (arePortalWindowEnabled) EnablePortalWindow();
	if (areLootWindowEnabled) InitAdvLoot();
	if (areAutoSkillWindowEnabled) InitAutoSkill();
	if (areDungeonWindowEnabled) InitDungeon();
	if (areDifficultyWindowEnabled) InitDifficulty();
	if (areFellowshipWindowEnabled) InitFellowship();
	if (areTravelWindowEnabled) InitTravel();
	if (areSpellJournalEnabled) InitSpellJournal();
	if (areAoTMenuEnabled) InitAoTMenu();
	// Level-up reward picker is now a native SIDL window (core_spellchoice_native.cpp) rather than a
	// tab of the self-drawn journal. Random AA is retired, so there is no AA counterpart.
	if (areSpellChoiceWindowEnabled) InitSpellChoiceNative();
}