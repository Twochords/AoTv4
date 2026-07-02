#pragma once

#include "MQ2Main.h"

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

// CEverQuest::InterpretCmd (0x51FCE0) -- thiscall(pChar, cmd); detoured as __fastcall. Every typed
// command flows through here, so we catch /trader and /buyer and open the window before the client
// can reject them as unknown. Everything else (incl. the dll's own /say injections) passes through.
void __fastcall InterpretCmd_TA_Detour(void* pThis, void* edx, void* pChar, const char* cmd);
DETOUR_TRAMPOLINE_EMPTY(void __fastcall InterpretCmd_TA_Tramp(void* pThis, void* edx, void* pChar, const char* cmd));
void __fastcall InterpretCmd_TA_Detour(void* pThis, void* edx, void* pChar, const char* cmd)
{
	if (cmd) {
		const char* c = cmd;
		while (*c == ' ' || *c == '/') ++c;                            // skip leading spaces/slash
		// "/shop" -> open the Set-Up-Shop vendor window. The window needs the satchel list from the
		// server, so we route it through the native /say (reliable) as "shopopen"; the server replies
		// with VENDORDATA which the dll's chat hook turns into the window. The say echo is swallowed.
		if (_strnicmp(c, "shop", 4) == 0 && (c[4] == 0 || c[4] == ' ')) {
			InterpretCmd_TA_Tramp(pThis, edx, pChar, "/say shopopen");
			return;
		}
	}
	InterpretCmd_TA_Tramp(pThis, edx, pChar, cmd);
}

void EnableTradeAnywhere()
{
	EzDetour((((DWORD)0x51FCE0 - 0x400000) + baseAddress), InterpretCmd_TA_Detour, InterpretCmd_TA_Tramp);
}
