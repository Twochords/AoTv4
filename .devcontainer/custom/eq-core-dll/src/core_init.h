#include "core_bazaar.h"
#include "core_map.h"
#include "core_npc.h"
#include "core_item.h"
#include "core_eqg_load.h"
#include "core_zone.h"
#include "core_luclin_models.h"
#include "core_allcasters.h"
#include "core_skillunlock.h"
#include "core_spellchoice.h"
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
	// Spell / AA / Lost are now tabs of the single Reward Journal window (it enables their chat
	// parsers itself), so we wire the journal instead of the three old standalone windows.
	if (areJournalWindowEnabled) EnableJournalWindow();
	if (areVendorWindowEnabled) EnableVendorWindow();
	if (arePortalWindowEnabled) EnablePortalWindow();
}