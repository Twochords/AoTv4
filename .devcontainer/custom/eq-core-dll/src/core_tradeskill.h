#pragma once
#include "MQ2Main.h"
#include <cstdio>

// ================================================================================================
// AoTv4 tradeskill unlock -- let ANY character (everyone is a Bard) use the class/race-locked
// tradeskills: Tinkering (Gnome), Alchemy (Shaman), Make Poison (Rogue), plus the other racial
// tradeskill containers. The SERVER combine gates are already Bard-bypassed (zone/tradeskills.cpp);
// these are the two CLIENT-side (eqgame.exe RoF2) gates that remain:
//
//   (1) CSkillMgr::IsAvailable(skill) @ 0x5BAE40 -- the client's "can I use this skill" check. It
//       returns false for a Bard on the locked tradeskills, which HIDES them from the skill window
//       and greys the container Combine button. We force it true for the gathering + tradeskill
//       skill ids so they show and the button enables.
//
//   (2) CContainerWnd::HandleCombine @ 0x65F7B0 -- when you press Combine it runs an if/else over the
//       container's tradeskill type (via 0x7AFFE0) and, for each racial/class-locked tradeskill,
//       checks the player's race field [player+0x3370] (or class [player+0x3374]) against the required
//       value; on mismatch it shows "your race/class cannot..." and aborts. Each check is
//       `cmp [field], REQ ; je <skip-error>`. We byte-patch every such `je` (0x74) to an unconditional
//       `jmp` (0xEB) so the error is always skipped -> any race/class can combine. Addresses were found
//       by disassembling this exact RoF2 build (image base 0x400000); the Tinkering gate is combine
//       type 0x23 with race == 0xC (Gnome) at 0x65FA1F.
// ================================================================================================

// ---- (1) IsAvailable ----
DETOUR_TRAMPOLINE_EMPTY(bool __fastcall TS_IsAvailable_Tramp(void* pThis, void* edx, int skill));
bool __fastcall TS_IsAvailable_Detour(void* pThis, void* edx, int skill)
{
	if (TS_IsAvailable_Tramp(pThis, edx, skill)) return true;
	// Forage (27) + every tradeskill (Fishing 55 .. Alchemy/Tinkering/Make Poison .. Alchemy 75):
	if (skill == 27 || (skill >= 55 && skill <= 75)) return true;
	return false;
}

// ---- (2) HandleCombine race/class gate byte-patch ----
static void TS_PatchJeToJmp(DWORD static_addr)
{
	BYTE* p = (BYTE*)((static_addr - 0x400000) + baseAddress);
	DWORD old_prot = 0;
	if (VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old_prot)) {
		if (p[0] == 0x74) p[0] = 0xEB;   // je -> jmp (only if it's still the expected `je`)
		VirtualProtect(p, 1, old_prot, &old_prot);
	}
}

// ---- DIAGNOSTIC: log each container's real combine-type ----
// 0x7AFFE0 = the client's "get this container's combine/tradeskill type" accessor (returns BYTE [item+0x520]).
// My static reads of it kept contradicting what the client actually does, so log each DISTINCT value once to
// <eqdir>\aotv4_ts_debug.txt. Open the Toolbox / Medicine Bag / Mortar & Pestle and the file records their real
// combine-types -> then I can patch the exact gate. Remove once the gate is nailed.
DETOUR_TRAMPOLINE_EMPTY(unsigned long __fastcall TS_GetCombineType_Tramp(void* pThis, void* edx));
static bool g_ts_seen[256] = { false };
unsigned long __fastcall TS_GetCombineType_Detour(void* pThis, void* edx)
{
	unsigned long ct = TS_GetCombineType_Tramp(pThis, edx);
	if (ct < 256 && !g_ts_seen[ct]) {
		g_ts_seen[ct] = true;
		FILE* f = nullptr;
		fopen_s(&f, "aotv4_ts_debug.txt", "a");
		if (f) { fprintf(f, "container combine_type seen: %lu (0x%lX)\n", ct, ct); fclose(f); }
	}
	return ct;
}

void EnableTradeskillUnlock()
{
	EzDetour((((DWORD)0x5BAE40 - 0x400000) + baseAddress), TS_IsAvailable_Detour, TS_IsAvailable_Tramp);
	EzDetour((((DWORD)0x7AFFE0 - 0x400000) + baseAddress), TS_GetCombineType_Detour, TS_GetCombineType_Tramp);

	// Every `je <skip-error>` in HandleCombine's racial/class tradeskill gate chain. 0x65FA1F is the
	// Tinkering (Gnome) gate; the rest unlock the other racial-container tradeskills too (harmless -- a
	// Bard-only server wants all of them). The helper only patches if the byte is still `je`.
	static const DWORD GATES[] = {
		0x65F81B,   // Research (class-gated variant)
		0x65F917,   // race gate
		0x65F959,   // race gate
		0x65F99B,   // race gate
		0x65F9DD,   // race gate
		0x65FA1F,   // TINKERING (race == Gnome)
		0x65FA61,   // race gate
		0x65FAA6,   // race gate
		0x65FB5F,   // race gate
		0x65FB9C,   // race gate
		0x65FBD9,   // race gate
	};
	for (int i = 0; i < (int)(sizeof(GATES) / sizeof(GATES[0])); ++i) {
		TS_PatchJeToJmp(GATES[i]);
	}
}

// ================================================================================================
// AoTv4 tradeskill SKILL DROPDOWN
// ------------------------------------------------------------------------------------------------
// The "Artisan's Universal Kit" (item 990061) links EVERY recipe into ONE container, so the native
// Tradeskill Window's search mixes all skills together. This drives a custom combobox (ScreenID
// "SkillCombo") added to EQUI_TradeskillWnd.xml: pick a tradeskill and the recipe list is filtered
// to just that skill. On a selection change we re-run the NATIVE search with a "#<skillid>#" token
// prepended to the query; the SERVER (zone/client_packet.cpp Handle_OP_RecipesSearch) strips the
// token and adds `AND tr.tradeskill = N`. The visible search box is restored immediately so the
// token is never seen. If the client's XML has no "SkillCombo" child (un-modded UI) this is a no-op
// and the native window works exactly as before.
//
//   Anchor: pinstCTradeskillWnd @ 0xF75998  (RE'd from eqgame.exe -- singleton create at
//   0x49A504..0x49A556: `if(!g){p=operator new(0xC2C); CTradeskillWnd::ctor(p) @0x77CE20; g=p;}`;
//   the ctor pushes the "TradeskillWnd" SIDL name string @0x9C84B0.)
// ================================================================================================

struct TS_SkillEntry { const char* label; int skill; };
static const TS_SkillEntry TS_SKILLS[] = {
	{ "All Tradeskills", -1 },   // index 0 = no filter
	{ "Alchemy",         59 },
	{ "Baking",          60 },
	{ "Blacksmithing",   63 },
	{ "Brewing",         65 },
	{ "Fishing",         55 },
	{ "Fletching",       64 },
	{ "Jewelry Making",  68 },
	{ "Make Poison",     56 },
	{ "Poison (Doses)",  75 },
	{ "Pottery",         69 },
	{ "Research",        58 },
	{ "Tailoring",       61 },
	{ "Tinkering",       57 },
};
static const int TS_SKILL_COUNT = (int)(sizeof(TS_SKILLS) / sizeof(TS_SKILLS[0]));

static void* g_tsLastWnd    = nullptr;
static bool  g_tsPopulated  = false;
static int   g_tsLastChoice = -1;

static inline CSidlScreenWnd* TS_GetWnd()
{
	return *(CSidlScreenWnd**)((0xF75998 - 0x400000) + baseAddress); // *(CTradeskillWnd**)pinstCTradeskillWnd
}

// Re-run the native recipe search with the skill token prepended, invisibly.
static void TS_FireFilteredSearch(CSidlScreenWnd* pTS, int skillId)
{
	CXWnd*    btn  = pTS->GetChildItem("SearchButton");
	CEditWnd* edit = (CEditWnd*)pTS->GetChildItem("SearchTextEdit");
	if (!btn) return;

	// current user-typed text, minus any old "#N#" prefix. Read via the proven MQ2 helper
	// (CXStr::operator char* is not address-bound in this build, so GetCXStr(edit->InputText,...)).
	char userText[64] = { 0 };
	if (edit) GetCXStr(((CEditWnd*)edit)->InputText, userText, sizeof(userText));
	char* clean = userText;
	if (clean[0] == '#') { char* e = strchr(clean + 1, '#'); if (e) clean = e + 1; }

	char buf[96];
	if (skillId >= 0) _snprintf_s(buf, sizeof(buf), _TRUNCATE, "#%d#%s", skillId, clean);
	else              _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%s", clean);

	if (edit) { CXStr cs(buf);   ((CXWnd*)edit)->SetWindowTextA(cs); }
	// synchronous: the parent's WndNotification builds + sends OP_RecipesSearch from the edit text
	((CXWnd*)pTS)->WndNotification(btn, XWM_LCLICK, 0);
	if (edit) { CXStr cs2(clean); ((CXWnd*)edit)->SetWindowTextA(cs2); } // restore visible text
}

// Per-frame heartbeat (called from ProcessGameEvents). Populate the combo once, then fire a filtered
// search whenever the selection changes. Cheap: a pointer read + early-outs on the common path.
void TS_DriveDropdown()
{
	CSidlScreenWnd* pTS = TS_GetWnd();
	if (!pTS) { g_tsLastWnd = nullptr; g_tsPopulated = false; g_tsLastChoice = -1; return; }

	// UI reload (/loadskin) recreates the window object -> repopulate the fresh (empty) combo
	if (pTS != g_tsLastWnd) { g_tsLastWnd = pTS; g_tsPopulated = false; g_tsLastChoice = -1; }

	CComboWnd* combo = (CComboWnd*)pTS->GetChildItem("SkillCombo");
	if (!combo) return; // un-modded UI: no dropdown -> native behavior, no-op

	if (!g_tsPopulated) {
		combo->DeleteAll();
		for (int i = 0; i < TS_SKILL_COUNT; ++i) combo->InsertChoice((char*)TS_SKILLS[i].label);
		combo->SetChoice(0);
		g_tsPopulated  = true;
		g_tsLastChoice = 0;
		return;
	}

	if (!((CXWnd*)pTS)->IsReallyVisible()) return; // don't fire while the window is closed

	int cur = combo->GetCurChoice();
	if (cur != g_tsLastChoice && cur >= 0 && cur < TS_SKILL_COUNT) {
		g_tsLastChoice = cur;
		TS_FireFilteredSearch(pTS, TS_SKILLS[cur].skill);
	}
}
