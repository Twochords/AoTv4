#pragma once

#include "MQ2Main.h"
#include "core_achievements_native.h"

char __fastcall DisableCBazaarSearchWnd_Trampoline(char* pThis);
char __fastcall DisableCBazaarSearchWnd_Detour(char* pThis) { return 0; }
DETOUR_TRAMPOLINE_EMPTY(char __fastcall DisableCBazaarSearchWnd_Trampoline(char* pThis));
// Hooks to CBazaarSearchWnd::AboutToShow
void DisableCBazaarSearchWnd() { EzDetour((((DWORD)0x00636670 - 0x400000) + baseAddress), DisableCBazaarSearchWnd_Detour, DisableCBazaarSearchWnd_Trampoline); };

// ------------------------------------------------------------------------------------------------
// Trade Anywhere -- let /trader (sell menu) and /buyer (buyer setup) work in ANY zone.
//
// The RoF2 client only registers the /trader and /buyer slash commands while in the Bazaar zone, so
// they report "Unknown command" anywhere else. But: (1) the SERVER has ZERO zone gate on trader /
// buyer / bazaar-search (verified -- Handle_OP_Trader / TraderStartTrader / Handle_OP_Barter run in
// any zone), and (2) the trader window (CBazaarWnd) + buyer window (CBarterWnd) are global SIDL
// windows that exist in every zone. So we detour CEverQuest::InterpretCmd and, on /trader or
// /buyer, open the corresponding window OURSELVES -- bypassing the zone-gated command registration.
// The window's own "Begin Trader" / buyer buttons then send OP_Trader / OP_Barter, which the server
// already accepts from anywhere. (/bazaar + /barter SEARCH already work in any zone, so buyers can
// find these traders -- the trader registry was never zone-scoped.)
//
// NEEDS TWO OFFSETS (one per window): the RoF2 addresses of CBazaarWnd::Activate(void) and
// CBarterWnd::Activate(void). Activate is the IDEAL primitive -- it opens the window AND registers
// the window's slash command (that's exactly why /trader/ /buyer only exist while in the Bazaar).
// These functions EXIST in eqgame.exe but MQ2's RoF2 offset table never mapped them (even the full
// upstream eqgame.h lacks CBazaarWnd__Activate_x / CXWnd__Show_x). So they must be found by RE'ing
// eqgame.exe (May 10 2013 build) -- see the note in the chat / AOTV4 docs for how. They live in the
// bazaar-UI code region near CBazaarSearchWnd::HandleBazaarMsg (0x635530). Set them below, then
// enable areTradeAnywhereEnabled in _options.h + rebuild. Until set, this is a safe no-op (dormant).
#ifndef CBazaarWnd__Activate_x
#define CBazaarWnd__Activate_x 0x000000   // <-- TODO: RoF2 CBazaarWnd::Activate(void) address
#endif
#ifndef CBarterWnd__Activate_x
#define CBarterWnd__Activate_x 0x000000   // <-- TODO: RoF2 CBarterWnd::Activate(void) address
#endif

// ---- Diagnostic: dump a window's vtable so we can read Activate/Show addresses without Ghidra ----
// Triggered by typing "/dumpbazaar" while standing IN the Bazaar (where the trader window exists).
// Writes each window's vtable (index -> the rebased STATIC 0x4xxxxx offset that matches eqgame.h) to
// "aotv4_bazaar_vtable.txt" next to eqgame.exe. Send that file back and we extract the addresses.
#include <cstdio>
static void DumpOneVtable(FILE* f, const char* label, DWORD pinst_addr)
{
	void* pWnd = *(void**)((pinst_addr - 0x400000) + baseAddress);
	fprintf(f, "==== %s  pinst=0x%06X  pWnd=%p ====\n", label, pinst_addr, pWnd);
	if (!pWnd) { fprintf(f, "  (NULL -- window not created in this zone; stand in the Bazaar)\n\n"); return; }
	void** vtbl = *(void***)pWnd;
	fprintf(f, "  vtable=%p\n", (void*)vtbl);
	for (int i = 0; i < 100; ++i) {
		DWORD fn = (DWORD)vtbl[i];
		DWORD st = fn ? (fn - baseAddress + 0x400000) : 0;     // rebase to image-base 0x400000
		const char* tag = (st >= 0x401000 && st < 0x0F00000) ? "" : "   <- not code (past end of vtable?)";
		fprintf(f, "  [%3d] static 0x%06X%s\n", i, st, tag);
	}
	fprintf(f, "\n");
}
static void DumpBazaarVtable()
{
	FILE* f = nullptr;
	fopen_s(&f, "aotv4_bazaar_vtable.txt", "w");
	if (!f) return;
	fprintf(f, "AoTv4 bazaar window vtable dump. baseAddress=0x%08X (image base 0x400000)\n\n", (unsigned)baseAddress);
	DumpOneVtable(f, "CBazaarWnd (trader/sell)", pinstCBazaarWnd_x);
	DumpOneVtable(f, "CBarterWnd (buyer)",       pinstCBarterWnd_x);
	fclose(f);
}

// Open a global SIDL window by calling its Activate() (opens + registers its command). Safe: no-ops
// if the address isn't configured or the window instance hasn't been created in this zone.
static inline void ActivateEQWindow(DWORD pinst_addr, DWORD activate_addr)
{
	if (activate_addr == 0x000000) return;                             // address not wired yet
	void* pWnd = *(void**)((pinst_addr - 0x400000) + baseAddress);    // *(C*Wnd**)pinstC...Wnd
	if (!pWnd) return;                                                 // window not instantiated -> nothing to do
	typedef void(__thiscall *fActivate)(void* pThis);
	auto Activate = (fActivate)((activate_addr - 0x400000) + baseAddress);
	Activate(pWnd);
}

#include "core_advloot.h"   // AdvLootShow() -- the Advanced Loot window is its own module
#include "core_autoskill.h"  // AutoSkillShow() -- likewise
#include "core_dungeon.h"    // DungeonShow() -- likewise
#include "core_difficulty.h" // DifficultyShow() -- the /pick Zone Difficulty window
#include "core_fellowship.h" // FellowshipShow() -- the /fellowship window
#include "core_spellchoice_native.h"  // SpellChoiceOpen() -- the three-tab spell window
#include "core_aotmenu.h"            // AoTMenuShow() -- the launcher
void ShowSearchWindow(); // defined in core_spellwindow.cpp -- opens the in-game search ("allaclone") window

// AoTv4: MQ2-style map overlay (native_map.cpp). It can't install its own InterpretCmd hook (we already
// own that detour), so its slash commands are routed here, and its worker is spawned from ProcessGameEvents.
namespace nativeinterface { bool TryHandleCommand(const char* line); void Start(); }

// CEverQuest::InterpretCmd (0x51FCE0) -- thiscall(pChar, cmd); detoured as __fastcall. Every typed
// command flows through here, so we catch /trader and /buyer and open the window before the client
// can reject them as unknown. Everything else (incl. the dll's own /say injections) passes through.
void __fastcall InterpretCmd_TA_Detour(void* pThis, void* edx, void* pChar, const char* cmd);
DETOUR_TRAMPOLINE_EMPTY(void __fastcall InterpretCmd_TA_Tramp(void* pThis, void* edx, void* pChar, const char* cmd));
void __fastcall InterpretCmd_TA_Detour(void* pThis, void* edx, void* pChar, const char* cmd)
{
	if (cmd) {
		// map overlay commands (/nimap, /mapfilter, /mapshow, /maphide, /highlight, /maploc, /xtarinfo, /ni)
		if (nativeinterface::TryHandleCommand(cmd)) {
			return;
		}
		const char* c = cmd;
		while (*c == ' ' || *c == '/') ++c;                            // skip leading spaces/slash
		// "/trader" -> open OUR native SIDL shop window (ShopWnd, core_spellwindow.cpp). We do NOT use the
		// real RoF2 trader window (CBazaarWnd): it only renders inside the Bazaar screen context (tested --
		// showing it standalone draws nothing). Instead we route through the reliable "/say shopopen"; the
		// server replies SHOPINVDATA/MYSHOPDATA and the dll's chat hook opens + populates ShopWnd, which
		// renders in any zone. (/shop retired -- /trader is the command now; /buyer removed as the native
		// buyer window has the same Bazaar-screen limitation.)
		if (_strnicmp(c, "trader", 6) == 0 && (c[6] == 0 || c[6] == ' ')) {
			InterpretCmd_TA_Tramp(pThis, edx, pChar, "/say shopopen");
			return;
		}
		// "/allaclone" (aliases "/search", "/find") -> open the in-game lookup window. It needs no
		// server data to OPEN (the player types the term inside), so just show it -- the window sends
		// "/say srch ..." itself. The old names are kept because they are in players' muscle memory.
		if ((_strnicmp(c, "allaclone", 9) == 0 && (c[9] == 0 || c[9] == ' ')) ||
		    (_strnicmp(c, "search",    6) == 0 && (c[6] == 0 || c[6] == ' ')) ||
		    (_strnicmp(c, "find",      4) == 0 && (c[4] == 0 || c[4] == ' '))) {
			ShowSearchWindow();
			return;
		}
		// AoTv4 Advanced Loot window: "/advl" (re)opens the native AdvLootWnd (it also auto-pops on drops).
		if (_strnicmp(c, "advl", 4) == 0 && (c[4] == 0 || c[4] == ' ')) {
			AdvLootShow();
			return;
		}
		// AoTv4 Autoskill window: "/autoskill" opens the native AoTAutoSkillWnd. The server-side
		// "#autoskill" text command is untouched and still works.
		if (_strnicmp(c, "autoskill", 9) == 0 && (c[9] == 0 || c[9] == ' ')) {
			AutoSkillShow();
			return;
		}
		// AoTv4 Delve window: "/delve" or "/dungeon" opens AoTDungeonWnd and asks the server for the
		// unlocked layer list. The "/say delve" commands still work by hand for testing.
		if ((_strnicmp(c, "delve", 5) == 0 && (c[5] == 0 || c[5] == ' ')) ||
		    (_strnicmp(c, "dungeon", 7) == 0 && (c[7] == 0 || c[7] == ' '))) {
			DungeonShow();
			return;
		}
		// AoTv4 spell window: "/journal" and "/spells" open the three-tab window (Choose / Known /
		// Pool) for browsing, with no reward pending. Level-up still pushes it open by itself.
		if ((_strnicmp(c, "journal", 7) == 0 && (c[7] == 0 || c[7] == 32)) ||
		    (_strnicmp(c, "spells", 6) == 0 && (c[6] == 0 || c[6] == 32))) {
			SpellChoiceOpen();
			return;
		}
		// AoTv4 Zone Difficulty window: "/pick" opens AoTDifficultyWnd (Normal / Nightmare / Hell /
		// Inferno) and asks the server which one this character is on.
		//
		// ⚠️⚠️ THIS DELIBERATELY SWALLOWS THE NATIVE "/pick", and that is the point rather than a side
		// effect. The client's own pick window cannot label difficulties -- PickZoneEntry_Struct
		// carries only {zone_id, player_count, instance_id} with no text field, so all four would
		// render as the same zone name -- and `Handle_OP_PickZone` is a bare stub server side, so the
		// stock command does nothing at all here anyway. Returning without calling the trampoline is
		// what stops the client ever seeing it.
		if (_strnicmp(c, "pick", 4) == 0 && (c[4] == 0 || c[4] == ' ')) {
			DifficultyShow();
			return;
		}

		// AoTv4 Fellowship window: "/fellowship" (alias "/fship") opens AoTFellowshipWnd -- roster,
		// chat and the campfire. ⚠️ NOT the client's own fellowship window; see core_fellowship.h.
		// ⚠️ `c` is the command WITHOUT its leading slash (the /pick test above is "pick", not
		// "/pick"), and there is no `szLine` in this scope.
		if ((_strnicmp(c, "fellowship", 10) == 0 && (c[10] == 0 || c[10] == ' ')) ||
		    (_strnicmp(c, "fship", 5) == 0 && (c[5] == 0 || c[5] == ' '))) {
			FellowshipShow();
			return;
		}
		// AoTv4 Shield Wall: "/shield" for EVERY class.
		//
		// ⚠️⚠️ THE SERVER HAS BEEN OPEN TO EVERY CLASS THE WHOLE TIME -- `AoT:ShieldAnyClass` is true,
		// `AoT:ShieldMinLevel` is 1, and `Mob::ShieldAbility` has no class test at all. What blocks a
		// non-Warrior is the CLIENT: RoF2 refuses to dispatch its own /shield command for anyone else
		// and never sends OP_Shielding, so none of that server-side work was ever reachable. This is
		// layer two of CLAUDE.md section 4's three layers -- client display, client SEND, server
		// execute -- and it is invisible from the server, where the feature looks finished.
		//
		// Rewriting to the say command routes around the client's dispatcher entirely: our detour sees
		// the typed text BEFORE the client decides whether the command exists for this class, so the
		// class check never runs. No client file and no opcode work needed.
		// ⚠️ It goes to `/say shieldwall`, which `Client::ChannelMessageReceived` intercepts and feeds
		// straight into `AoTv4Shield` -- the SAME body /shield uses, so the toggle, the recast and the
		// permanent duration behave identically on both paths.
		// 📌 Swallowing the native command costs a Warrior nothing: they land in the same function.
		if (_strnicmp(c, "shield", 6) == 0 && (c[6] == 0 || c[6] == ' ')) {
			InterpretCmd_TA_Tramp(pThis, edx, pChar, "/say shieldwall");
			return;
		}
		// AoTv4 launcher: "/aot" or "/menu" lists every custom window as a button, so none of them
		// needs a remembered command.
		if ((_strnicmp(c, "aot", 3) == 0 && (c[3] == 0 || c[3] == 32)) ||
		    (_strnicmp(c, "menu", 4) == 0 && (c[4] == 0 || c[4] == 32))) {
			AoTMenuShow();
			return;
		}
	}
	// Native achievement window: "/ach close" is fully handled client-side; the other
	// /achievement|/ach forms are rewritten to a "/say #ach ..." the server understands.
	if (NativeAchievementHandleLocalCommand(cmd)) return;        // e.g. "/ach close" fully handled
	char ach_rw[256];
	if (NativeAchievementRewriteCommand(cmd, ach_rw, sizeof(ach_rw))) { InterpretCmd_TA_Tramp(pThis, edx, pChar, ach_rw); return; }
	InterpretCmd_TA_Tramp(pThis, edx, pChar, cmd);
}

void EnableTradeAnywhere()
{
	EzDetour((((DWORD)0x51FCE0 - 0x400000) + baseAddress), InterpretCmd_TA_Detour, InterpretCmd_TA_Tramp);
}
