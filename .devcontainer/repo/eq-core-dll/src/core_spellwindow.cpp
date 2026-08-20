// core_spellwindow.cpp
// ---------------------------------------------------------------------------
// The self-drawn (GDI) windows, plus the two detours every window in this dll shares.
//
// The file is named for what it used to be. The level-up spell picker and the death-driven AA
// picker were both born here as layered GDI overlays, and later became tabs of a "Reward Journal".
// All three are now DELETED: the level-up reward picker is a native SIDL window living in its own
// translation unit (core_spellchoice_native.cpp), and random AA is retired entirely. What remains
// here is the Portal / You Lost / Shop / Search windows.
//
// Why GDI at all: we tried drawing through DirectX9 by hooking the device's / swap chain's Present,
// but the RoF2 client renders through EQGraphicsDX9.dll's own device, which shares NO vtable with
// any device we can create, so the dummy-device hook never fires (proven: dh=2 sh=2 present=0
// swap=0). A separate LAYERED, TOPMOST window is independent of EQ's renderer, so it always shows
// in windowed mode. New windows should prefer the native SIDL route instead -- it renders through
// the client's own UI engine, so it looks native and needs no overlay thread.
//
// This TU owns two detours that several modules need, and an address can only be detoured once:
//   CEverQuest::dsp_chat  (0x51F1A0) -- the server-to-client transport; routes each line to the
//                                       module that owns it (spell choice, adv loot, achievements).
//   ProcessGameEvents     (0x539E60) -- per-frame, main thread; spawns overlay threads and runs
//                                       queued slash commands on the game thread.
// ---------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_achievements_native.h"
#include "core_advloot.h"
#include "core_lostwindow.h"
#include "core_autoskill.h"
#include "core_dungeon.h"
#include "core_difficulty.h"
#include "core_fellowship.h"
#include "core_travel.h"
#include "core_allaclone.h"    // the Allaclone lookup: supersedes the GDI "search" overlay below
#include "core_spelljournal.h"
#include "core_aotmenu.h"      // AoTMenuParseTransport() -- "AOTMENUSHOW" opens the menu
#include "core_spellchoice_native.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ===================================================================== skill unlock
// Reward-gating of combat skills (see core_skillunlock.h). CSkillMgr::GetSkillCap returns 0
// for a class+skill the class can't natively have; the Skills window and ability-hotkey
// picker hide anything with a 0 cap. We reveal a COMBAT skill only if the server says the
// player has EARNED it (picked it -> value > 0), so un-earned combat skills stay hidden
// until chosen. Non-combat real skills (casting/singing/utility) are unlocked unconditionally
// so spells and songs work. Unused skill slots are left at 0 (no "None" rows).

// Reward-gated skill ids -- MUST match quests/lua_modules/skill_pool.lua. Only the activated
// class-specific special attacks are gated; weapon skills and passive combat skills are
// auto-learned (not gated) so they always show.
static const int SKILLUNLOCK_COMBAT_IDS[] = {
	8,10,30,16,73,72,74,        // Backstab/Bash/Kick/Disarm/Taunt/Berserking/Frenzy
	21,23,26,38,52              // monk strikes: Dragon Punch/Eagle Strike/Flying/Round Kick/Tiger Claw
};
// ⚠️⚠️ NATIVE-ONLY SKILLS -- NEVER REVEALED, not even scaled. A THIRD category, because neither of
// the two below fits: they are not reward-gated (the picker never offers them, so SKILLUNLOCKDATA
// will never list them and the "reveal if earned" branch would hide them forever), and they must not
// take the unconditional "reveal so it works" branch either. Without this every class saw Mend and
// Feign Death in the Skills window and could drag them onto the hotbar -- reported as "everyone is
// getting mend and feign death". They were never granted server-side (skill_caps gates that), so the
// buttons did nothing; the bug was purely that the client advertised abilities the player cannot use.
//
// ⚠️ A skill listed here is left at whatever the client natively gives the class, so the class that
// DOES own it keeps it automatically -- Monk still has both. Nothing here needs a server change.
static const int SKILLUNLOCK_NATIVE_ONLY_IDS[] = {
	25,     // Feign Death
	32      // Mend
};

static bool g_combatGated[NUM_SKILLS] = { false };  // is skill id reward-gated?
static bool g_nativeOnly[NUM_SKILLS]  = { false };  // never reveal: class-native or not at all
static bool g_skillEarned[NUM_SKILLS] = { false };  // has the player earned it (server-sent)?

static void SkillUnlock_InitSets()
{
	for (int i = 0; i < (int)(sizeof(SKILLUNLOCK_COMBAT_IDS) / sizeof(int)); ++i) {
		int s = SKILLUNLOCK_COMBAT_IDS[i];
		if (s >= 0 && s < NUM_SKILLS) g_combatGated[s] = true;
	}
	for (int i = 0; i < (int)(sizeof(SKILLUNLOCK_NATIVE_ONLY_IDS) / sizeof(int)); ++i) {
		int s = SKILLUNLOCK_NATIVE_ONLY_IDS[i];
		if (s >= 0 && s < NUM_SKILLS) g_nativeOnly[s] = true;
	}
}

// Parse the server's "SKILLUNLOCKDATA id,id,..." line into g_skillEarned. Returns true if it
// was our line (so the chat hook swallows it). An empty list = nothing earned yet.
static bool SkillUnlock_HandleChat(const char* msg)
{
	static const char* TAG = "SKILLUNLOCKDATA";
	if (!msg) return false;
	const char* p = strstr(msg, TAG);
	if (!p) return false;
	p += strlen(TAG);

	for (int s = 0; s < NUM_SKILLS; ++s) if (g_combatGated[s]) g_skillEarned[s] = false;
	int cur = -1;
	for (;; ++p) {
		if (*p >= '0' && *p <= '9') { cur = (cur < 0 ? 0 : cur) * 10 + (*p - '0'); }
		else {
			if (cur >= 0 && cur < NUM_SKILLS) g_skillEarned[cur] = true;
			cur = -1;
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
	}
	return true;
}

DETOUR_TRAMPOLINE_EMPTY(unsigned long __fastcall GetSkillCap_Tramp(void* pThis, void* edx,
	void* pChar, int level, int classId, int skill, bool d, bool e, bool f));

static const int SKILLUNLOCK_MAX_REAL = 76;  // highest real skill id in RoF2 (77+ = unused "None")

unsigned long __fastcall GetSkillCap_Detour(void* pThis, void* edx, void* pChar,
	int level, int classId, int skill, bool d, bool e, bool f)
{
	unsigned long cap = GetSkillCap_Tramp(pThis, edx, pChar, level, classId, skill, d, e, f);
	// Only ever REVEAL a skill the client hides (native cap 0). A native skill (cap > 0)
	// passes straight through, so weapon skills / Defense / Dodge etc. are never removed.
	// Bounded to ids 0..76 so the unused "None" slots (77..98) stay hidden.
	if (cap == 0 && skill >= 0 && skill <= SKILLUNLOCK_MAX_REAL) {
		unsigned long scaled = (unsigned long)((level > 0 ? level : 1) * 5);
		if (scaled > 250) scaled = 250;
		if (g_nativeOnly[skill]) {
			// leave hidden: a native cap of 0 here means this class simply does not get it
		} else if (g_combatGated[skill]) {
			if (g_skillEarned[skill]) cap = scaled;  // non-native combat: reveal ONLY if earned
		} else {
			cap = scaled;                            // non-native casting/utility: reveal so it works
		}
	}
	return cap;
}

void EnableSkillUnlock()
{
	SkillUnlock_InitSets();
	EzDetour((((DWORD)0x5BAEC0 - 0x400000) + baseAddress), GetSkillCap_Detour, GetSkillCap_Tramp);
}

// art assets loaded from the client's uifiles/default at runtime
static char            g_uiDir[MAX_PATH] = {0};
static char            g_eqRoot[MAX_PATH] = {0};
static bool            g_assetsInit = false;
static struct { int sheet; HBITMAP bmp; } g_sheetCache[16];
static int             g_sheetCacheN = 0;
// separate cache for item-icon sheets (dragitemNN.tga) used by the Advanced Loot window
static struct { int sheet; HBITMAP bmp; } g_itemSheetCache[32];
static int             g_itemSheetCacheN = 0;

static HBITMAP LoadTGA(const char* path);  // fwd decl (used by the chat diagnostic)
static bool            g_choiceHooksInstalled = false;


// ----------------------------------------------------------------- Portal window state
// Scrollable list of discovered PoK-book zones. Opened by clicking a PoK book (server sends
// "PORTALOPEN"); right-click to dismiss. Fed by "PORTALDATA short|Long^...".
static const int       PORTAL_MAX = 64;
static char            g_portalShort[PORTAL_MAX][64] = { {0} };
static char            g_portalName[PORTAL_MAX][96]  = { {0} };
static int             g_portalCount = 0;
static int             g_portalScroll = 0;                 // index of the top visible row
static volatile bool   g_portalVisible = false;           // toggled by the hotkey
static char            g_portalPendingShort[64] = {0};     // set by click, consumed on game thread
static bool            g_portalEnabled = false;
static volatile bool   g_portalOverlayStarted = false;
static HWND            g_portalHwnd = nullptr;
static int             g_pPosX = 0, g_pPosY = 0;
static bool            g_pPositioned = false, g_pDragging = false;
static int             g_pDragDX = 0, g_pDragDY = 0;
static DWORD           g_portalIdleTick = 0;               // last interaction; auto-close on idle
static const DWORD     PORTAL_TIMEOUT_MS = 20000;          // close after 20s of no interaction

// ----------------------------------------------------------------- "You Lost" window state
// Read-only scrollable list of items destroyed on death (server "LOSTDATA name^name^...").
static const int       LOST_MAX = 128;
static char            g_lostName[LOST_MAX][96] = { {0} };
static int             g_lostCount = 0;
static int             g_lostScroll = 0;
static volatile bool   g_lostVisible = false;
static bool            g_lostEnabled = false;
static volatile bool   g_lostOverlayStarted = false;
static HWND            g_lostHwnd = nullptr;
static int             g_lPosX = 0, g_lPosY = 0;
static bool            g_lPositioned = false, g_lDragging = false;
static int             g_lDragDX = 0, g_lDragDY = 0;
static DWORD           g_lostIdleTick = 0;
static const DWORD     LOST_TIMEOUT_MS = 45000;            // death list lingers longer (you're reading)

static const int ICON_SZ = 40;

// "PORTALDATA short|Long^short|Long^..." -> the player's discovered PoK-book zones.
static bool HandlePortalChat(const char* msg)
{
	if (!msg) return false;
	const char* tag = strstr(msg, "PORTALDATA ");
	if (!tag) return false;
	tag += strlen("PORTALDATA ");

	g_portalCount = 0;
	std::string cur;
	for (const char* p = tag; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_portalCount < PORTAL_MAX) {
				size_t bar = cur.find('|');
				std::string sh = (bar == std::string::npos) ? cur : cur.substr(0, bar);
				std::string nm = (bar == std::string::npos) ? cur : cur.substr(bar + 1);
				strncpy_s(g_portalShort[g_portalCount], 64, sh.c_str(), _TRUNCATE);
				strncpy_s(g_portalName[g_portalCount], 96, nm.c_str(), _TRUNCATE);
				g_portalCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
	if (g_portalScroll > g_portalCount - 1) g_portalScroll = 0;
	return true;  // swallow our trigger line
}

// "LOSTDATA name^name^..." -> items destroyed on death; shows the You Lost window.
static bool HandleLostChat(const char* msg)
{
	if (!msg) return false;
	const char* tag = strstr(msg, "LOSTDATA ");
	if (!tag) return false;
	tag += strlen("LOSTDATA ");

	g_lostCount = 0; g_lostScroll = 0;
	std::string cur;
	for (const char* p = tag; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_lostCount < LOST_MAX) {
				strncpy_s(g_lostName[g_lostCount], 96, cur.c_str(), _TRUNCATE);
				g_lostCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
	g_lostVisible = (g_lostCount > 0);
	g_lostIdleTick = GetTickCount();
	return true;  // swallow our trigger line
}

// ===== Shop window state + chat handlers (declared here, before the dsp_chat detour uses them; the
// window's paint/wndproc/thread live further down). A layered GDI window with two tabs backing the
// permanent escrow shop (/shop). The server sends two lines the dll swallows:
//   SHOPINVDATA slot|itemid|name|vendor^...  -> "Add Items" tab (your droppable bag items, priced+added)
//   MYSHOPDATA  itemsn|itemid|name|cost^...  -> "My Shop" tab (what's listed; each row can be Pulled)
// Add -> "/say shopadd slot:copper,...";  Pull -> "/say shoppull <itemsn>";  refresh -> "/say shoprefresh".
static bool            g_vendorEnabled = false;
static volatile bool   g_vendorVisible = false;
static volatile bool   g_vendorStarted = false;        // vendor overlay thread spawned once
static HWND            g_vendorHwnd = nullptr;
static int             g_vendorTab = 0;                // 0 = Set Price, 1 = List Item, 2 = My Shop

// Inventory tabs (SHOPINVDATA slot|itemid|name|vendor|stackqty|bookprice). Shared by tab 0 (Set Price)
// and tab 1 (List Item). g_vendorIds holds the inventory SLOT id used by "/say shopadd".
static int             g_vendorCount = 0;
static int             g_vendorIds[80] = { 0 };        // inventory slot id
static int             g_vItemId[80] = { 0 };          // item_id -> used by "/say shopsetprice <itemid>:<copper>"
static char            g_vendorNames[80][64] = { { 0 } };
static unsigned        g_vendorVendor[80] = { 0 };     // vendor value (copper) -> default price hint
static int             g_vendorStack[80] = { 0 };      // how many the player has in this stack (from server)
static int             g_vListQty[80] = { 0 };         // how many to LIST (user-chosen; default = full stack)
static unsigned        g_vBookPrice[80] = { 0 };       // saved price-book price (copper); 0 = unpriced
static int             g_vCoin[80][4] = { { 0 } };     // per item coins: [0]=plat [1]=gold [2]=silver [3]=copper
static int             g_vFocusItem = -1, g_vFocusCoin = 0;  // which coin field is being typed into
static int             g_vendorScroll = 0;

// My Shop tab (MYSHOPDATA itemsn|itemid|name|cost|qty).
static int             g_myCount = 0;
static int             g_mySn[80] = { 0 };             // item_sn -> used by "/say shoppull"
static char            g_myName[80][64] = { { 0 } };
static unsigned        g_myCost[80] = { 0 };           // listed unit cost (copper)
static int             g_myQty[80] = { 0 };             // listed stack size
static int             g_myScroll = 0;

// Price-history log (SHOPLOGDATA itemid|name|old|new|when), newest first. Rendered on the Set Price tab
// filtered to the selected item's id.
static const int       SHOP_LOG_MAX = 40;
static int             g_logCount = 0;
static int             g_logItemId[SHOP_LOG_MAX] = { 0 };
static unsigned        g_logOld[SHOP_LOG_MAX] = { 0 };
static unsigned        g_logNew[SHOP_LOG_MAX] = { 0 };
static long            g_logWhen[SHOP_LOG_MAX] = { 0 };   // unix time of the change

// Listing history (SHOPLISTLOG itemid|name|qty|price|when) and sale history
// (SHOPSOLDLOG itemid|name|qty|price|buyer|when), both newest first.
//
// ⚠️ These share the ONE history listbox with the price log rather than adding two more. The widget
// already exists at the bottom of the window and its meaning simply follows the tab -- which is also
// what was asked for: the same panel the Set Price tab has, on the other two tabs.
static const int       SHOP_HIST_MAX = 60;
static int             g_listCount = 0;
static char            g_listName[SHOP_HIST_MAX][64] = { {0} };
static int             g_listQty [SHOP_HIST_MAX] = { 0 };
static unsigned        g_listPrice[SHOP_HIST_MAX] = { 0 };
static long            g_listWhen[SHOP_HIST_MAX] = { 0 };

static int             g_soldCount = 0;
static char            g_soldName[SHOP_HIST_MAX][64] = { {0} };
static int             g_soldQty [SHOP_HIST_MAX] = { 0 };
static unsigned        g_soldPrice[SHOP_HIST_MAX] = { 0 };
static char            g_soldBuyer[SHOP_HIST_MAX][48] = { {0} };
static long            g_soldWhen[SHOP_HIST_MAX] = { 0 };

static int             g_vPosX = 0, g_vPosY = 0;
static bool            g_vPositioned = false, g_vDragging = false;
static int             g_vDragDX = 0, g_vDragDY = 0;

// queue of "/say ..." commands from the window thread, dispatched on the game thread (like picks)
static char            g_vendorCmds[24][96];
static volatile int    g_vCmdHead = 0, g_vCmdTail = 0;
// Shared with core_advloot.cpp (core_advloot.h): running InterpretCmd off the game thread crashes
// the client, so every window queues here and ProcessGameEvents drains it.
void AoTQueueGameCommand(const char* c) {
	int n = (g_vCmdHead + 1) % 24;
	if (n != g_vCmdTail) { strncpy_s(g_vendorCmds[g_vCmdHead], 96, c, _TRUNCATE); g_vCmdHead = n; }
}
static void VendorQueue(const char* c) { AoTQueueGameCommand(c); }

// split "a|b|c|..." (one row) into up to n '|'-delimited fields.
static void VSplitN(const std::string& s, std::string* out, int n) {
	int pi = 0;
	for (size_t k = 0; k < s.size() && pi < n; ++k) { if (s[k] == '|') { if (pi + 1 < n) ++pi; } else out[pi] += s[k]; }
}

// Native SIDL shop window (defined later); the chat handlers drive it.
void ShopSidlShow();            // ensure + show the native ShopWnd
void ShopWndRefreshIfOpen();    // re-populate its list if it's open

// SHOPINVDATA slot|itemid|name|vendor|stackqty|bookprice^...  -> the inventory tabs (Set Price / List).
static bool HandleShopInv(const char* msg)
{
	const char* t = strstr(msg, "SHOPINVDATA ");
	if (!t) return false;
	t += strlen("SHOPINVDATA ");
	g_vendorCount = 0; g_vendorScroll = 0; g_vFocusItem = -1;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_vendorCount < 80) {
				std::string f[6]; VSplitN(cur, f, 6);
				g_vendorIds[g_vendorCount]    = atoi(f[0].c_str());              // inventory slot
				g_vItemId[g_vendorCount]      = atoi(f[1].c_str());              // item id (for shopsetprice)
				strncpy_s(g_vendorNames[g_vendorCount], 64, f[2].c_str(), _TRUNCATE);
				g_vendorVendor[g_vendorCount] = (unsigned)atoi(f[3].c_str());
				int q = atoi(f[4].c_str()); if (q < 1) q = 1;                    // stack size the player holds
				g_vendorStack[g_vendorCount] = q;
				g_vListQty[g_vendorCount]    = q;                                // default: list the whole stack
				g_vBookPrice[g_vendorCount]  = (unsigned)atoi(f[5].c_str());     // saved price-book price (0 = unset)
				unsigned bp = g_vBookPrice[g_vendorCount];
				g_vCoin[g_vendorCount][0] = bp / 1000;                           // seed the Set-Price fields from the book
				g_vCoin[g_vendorCount][1] = (bp / 100) % 10;
				g_vCoin[g_vendorCount][2] = (bp / 10) % 10;
				g_vCoin[g_vendorCount][3] = bp % 10;
				g_vendorCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else cur += *p;
	}
	ShopSidlShow();          // open/refresh the native SIDL shop window (replaces the old GDI overlay)
	return true;
}

// MYSHOPDATA itemsn|itemid|name|cost|qty^...  -> the "My Shop" tab.
static bool HandleMyShop(const char* msg)
{
	const char* t = strstr(msg, "MYSHOPDATA ");
	if (!t) return false;
	t += strlen("MYSHOPDATA ");
	g_myCount = 0; g_myScroll = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_myCount < 80) {
				std::string f[5]; VSplitN(cur, f, 5);
				g_mySn[g_myCount]   = atoi(f[0].c_str());                        // item_sn
				strncpy_s(g_myName[g_myCount], 64, f[2].c_str(), _TRUNCATE);
				g_myCost[g_myCount] = (unsigned)atoi(f[3].c_str());
				int q = atoi(f[4].c_str()); g_myQty[g_myCount] = q < 1 ? 1 : q;  // listed stack size
				g_myCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else cur += *p;
	}
	ShopWndRefreshIfOpen();   // if the shop window is on the My Shop tab, re-populate it
	return true;
}

// SHOPBOOKDATA itemid|name|price^...  -> the persistent price book. The per-item price is already carried
// in SHOPINVDATA (field 5), so this line is swallowed for completeness; a refresh keeps the view current.
static bool HandleShopBook(const char* msg)
{
	if (!strstr(msg, "SHOPBOOKDATA ")) return false;
	ShopWndRefreshIfOpen();
	return true;
}

// SHOPLISTLOG itemid|name|qty|price|when^...  -> what you have listed (newest first).
static bool HandleShopListLog(const char* msg)
{
	const char* t = strstr(msg, "SHOPLISTLOG ");
	if (!t) return false;
	t += strlen("SHOPLISTLOG ");
	g_listCount = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_listCount < SHOP_HIST_MAX) {
				std::string f[5]; VSplitN(cur, f, 5);
				strncpy_s(g_listName[g_listCount], sizeof(g_listName[0]), f[1].c_str(), _TRUNCATE);
				g_listQty  [g_listCount] = atoi(f[2].c_str());
				g_listPrice[g_listCount] = (unsigned)atoi(f[3].c_str());
				g_listWhen [g_listCount] = atol(f[4].c_str());
				g_listCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
		else { cur += *p; }
	}
	ShopWndRefreshIfOpen();
	return true;
}

// SHOPSOLDLOG itemid|name|qty|price|buyer|when^...  -> what has sold (newest first).
static bool HandleShopSoldLog(const char* msg)
{
	const char* t = strstr(msg, "SHOPSOLDLOG ");
	if (!t) return false;
	t += strlen("SHOPSOLDLOG ");
	g_soldCount = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_soldCount < SHOP_HIST_MAX) {
				std::string f[6]; VSplitN(cur, f, 6);
				strncpy_s(g_soldName [g_soldCount], sizeof(g_soldName[0]),  f[1].c_str(), _TRUNCATE);
				g_soldQty   [g_soldCount] = atoi(f[2].c_str());
				g_soldPrice [g_soldCount] = (unsigned)atoi(f[3].c_str());
				strncpy_s(g_soldBuyer[g_soldCount], sizeof(g_soldBuyer[0]), f[4].c_str(), _TRUNCATE);
				g_soldWhen  [g_soldCount] = atol(f[5].c_str());
				g_soldCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
		else { cur += *p; }
	}
	ShopWndRefreshIfOpen();
	return true;
}

// SHOPLOGDATA itemid|name|old|new|when^...  -> the price-change history (newest first).
static bool HandleShopLog(const char* msg)
{
	const char* t = strstr(msg, "SHOPLOGDATA ");
	if (!t) return false;
	t += strlen("SHOPLOGDATA ");
	g_logCount = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_logCount < SHOP_LOG_MAX) {
				std::string f[5]; VSplitN(cur, f, 5);
				g_logItemId[g_logCount] = atoi(f[0].c_str());
				g_logOld[g_logCount]    = (unsigned)atoi(f[2].c_str());
				g_logNew[g_logCount]    = (unsigned)atoi(f[3].c_str());
				g_logWhen[g_logCount]   = atol(f[4].c_str());
				g_logCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else cur += *p;
	}
	ShopWndRefreshIfOpen();
	return true;
}

static bool HandleVendorChat(const char* msg)
{
	if (!msg) return false;
	if (HandleShopInv(msg))  return true;
	if (HandleMyShop(msg))   return true;
	if (HandleShopBook(msg)) return true;
	if (HandleShopLog(msg))  return true;
	if (HandleShopListLog(msg)) return true;   // SHOPLISTLOG -- List Item tab history
	if (HandleShopSoldLog(msg)) return true;   // SHOPSOLDLOG -- My Shop tab sold history
	return false;
}

// ===== AoTv4 in-game search window ("allaclone") state + chat handler (declared before the dsp_chat
// detour that swallows SRCHDATA/SRCHDET). Tabs: Items / Mobs / Spells. Type a term, Search sends
// "/say srch <kind> <term>"; the server replies "SRCHDATA <kind>^id|name^id|name^..." which fills the
// result list. Clicking a row sends "/say srchdet <kind> <id>"; the server replies
// "SRCHDET <kind>|<id>|<line~line~line>" shown in the bottom detail panel. Reuses VendorQueue for sends.
static bool            g_searchEnabled = false;
static volatile bool   g_searchVisible = false;
static bool            g_searchStarted = false;
static HWND            g_searchHwnd    = nullptr;
static int             g_srchTab       = 0;                 // 0 = item, 1 = npc, 2 = spell
static char            g_srchTerm[64]  = {0};
static bool            g_srchFocus     = true;             // text box focused -> typing goes to the term
static const int       SRCH_MAX        = 60;
static int             g_srchIds[SRCH_MAX]         = {0};
static char            g_srchNames[SRCH_MAX][96]   = {{0}};
static int             g_srchCount     = 0;
static int             g_srchScroll    = 0;
static int             g_srchDetailId  = -1;               // id whose detail is loaded (row highlight)
static const int       SRCH_DET_MAX    = 48;
static char            g_srchDetail[SRCH_DET_MAX][96] = {{0}};
static int             g_srchDetailN   = 0;
static int             g_srchDetScroll = 0;                // detail-panel scroll offset
static int             g_sPosX = 0, g_sPosY = 0;
static bool            g_sPositioned = false, g_sDragging = false;
static int             g_sDragDX = 0, g_sDragDY = 0;
static const char*     SRCH_KIND[4]    = { "item", "npc", "spell", "recipe" };

static bool HandleSearchChat(const char* msg)
{
	if (!msg) return false;

	if (const char* t = strstr(msg, "SRCHDATA ")) {        // <kind>^id|name^id|name^...
		t += strlen("SRCHDATA ");
		g_srchCount = 0; g_srchScroll = 0; g_srchDetailId = -1; g_srchDetailN = 0; g_srchDetScroll = 0;
		const char* p = strchr(t, '^');                    // skip the leading <kind> token
		p = p ? p + 1 : t;
		while (*p && g_srchCount < SRCH_MAX) {
			const char* bar = strchr(p, '|');
			const char* end = strchr(p, '^');
			if (!bar || (end && bar > end)) break;
			g_srchIds[g_srchCount] = atoi(p);
			const char* ns = bar + 1;
			int nl = end ? (int)(end - ns) : (int)strlen(ns);
			if (nl < 0) nl = 0; if (nl > 95) nl = 95;
			memcpy(g_srchNames[g_srchCount], ns, nl); g_srchNames[g_srchCount][nl] = 0;
			g_srchCount++;
			if (!end) break;
			p = end + 1;
		}
		if (g_searchHwnd) InvalidateRect(g_searchHwnd, nullptr, FALSE);
		return true;
	}

	if (const char* t = strstr(msg, "SRCHDET ")) {         // <kind>|<id>|<line~line~line>
		t += strlen("SRCHDET ");
		const char* b1 = strchr(t, '|');       if (!b1) return true;
		const char* b2 = strchr(b1 + 1, '|');  if (!b2) return true;
		g_srchDetailId = atoi(b1 + 1);
		const char* d = b2 + 1;
		g_srchDetailN = 0; g_srchDetScroll = 0;
		while (*d && g_srchDetailN < SRCH_DET_MAX) {
			const char* tilde = strchr(d, '~');
			int ll = tilde ? (int)(tilde - d) : (int)strlen(d);
			if (ll > 95) ll = 95;
			memcpy(g_srchDetail[g_srchDetailN], d, ll); g_srchDetail[g_srchDetailN][ll] = 0;
			g_srchDetailN++;
			if (!tilde) break;
			d = tilde + 1;
		}
		if (g_searchHwnd) InvalidateRect(g_searchHwnd, nullptr, FALSE);
		return true;
	}
	return false;
}

// CEverQuest::dsp_chat (RoF2 0x51F1A0) detour -- thiscall, detoured as __fastcall.
void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct);
DETOUR_TRAMPOLINE_EMPTY(void __fastcall SpellChat_Tramp(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct));

// ⚠️⚠️ MUST BE DECLARED ABOVE dsp_chat, NOT beside the ProcessGameEvents one further down. This is
// called from the swallow chain below; a declaration after the call site is "identifier not found".
// ⚠️ Forward-declared rather than including core_tradeskill.h: that header defines NON-INLINE
// functions and is included by exactly ONE translation unit (eqgame.cpp, via core_init.h), so
// including it here would duplicate every definition in it at link time.
bool TradeskillParseTransport(const char* message);

void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct)
{
	// Hide the pick command echoes. Selecting a choice issues "/say spellpick N" or
	// "/say aapick N", which the client would otherwise show as  You say, '...'  (and
	// broadcast to nearby players). Since every player runs this dll, swallowing any line
	// containing the trigger hides it on the speaker's screen AND on every listener's.
	if (msg && (strstr(msg, "spellpick") || strstr(msg, "spellreroll") ||
		strstr(msg, "spelldecline") || strstr(msg, "aapick") ||
		strstr(msg, "portalgo") || strstr(msg, "portalreq") ||
		strstr(msg, "shopopen") || strstr(msg, "shoprefresh") ||
		strstr(msg, "shopadd") || strstr(msg, "shoppull") || strstr(msg, "shopsetprice") ||
		strstr(msg, "srch") ||
		// Spell rank window: keep / release / upgrade / refresh. ⚠️ Every command a window issues
		// must be listed here or the player watches their own UI talk to the server in their chat
		// log -- and so does everyone standing nearby.
		strstr(msg, "spellkeep") || strstr(msg, "spellrelease") ||
		strstr(msg, "spellrank") || strstr(msg, "spellkept") ||
		// ⚠️⚠️ `/shield` IS REWRITTEN TO `/say shieldwall` in core_bazaar.h, so it is a window command
		// like any other and its echo has to be eaten here. It was the ONLY rewritten command with no
		// entry in this list, so every use of /shield announced "shieldwall" to everyone in earshot.
		strstr(msg, "shieldwall"))) return;  // shop/search/rank/shield echoes
	if (AdvLootIsOurEcho(msg)) return;  // "/say als*" echoes (core_advloot.cpp owns that command set)

	// Death-AA rewards are granted with ignore_cost=true (the points live in a private bank, not
	// m_pp.aapoints), so the engine's "You have gained/improved the ability ... at a cost of 0
	// ability point(s)" line always says 0 -- misleading. Swallow it; aa_choice.lua prints the
	// REAL banked cost instead. (Only our 0-cost grants ever produce this exact text.)
	if (msg && strstr(msg, "at a cost of 0 ability")) return;

	if (SkillUnlock_HandleChat(msg)) return;             // swallow SKILLUNLOCKDATA (earned set)
	// Native picker owns SPELLCHOICEDATA / SPELLDESCDATA now (core_spellchoice_native.cpp).
	if (SpellChoiceParseTransport(msg)) return;
	// Permanent spell library + ranks + upgrade costs for the spell window's Known tab.
	// ⚠️ Swallowed like every other transport here -- these are protocol, not chat, and an unswallowed
	// SPELLRANKDATA would dump 50 "id:rank:kept:origin" tuples into the player's chat log per chunk.
	if (SpellChoiceRankTransport(msg)) return;
	// Random AA on death is BACK (2026-07-31) and its offer is shown on the Death Book window's
	// Advancement tab. These two lines used to be swallowed here UNPARSED, because the old GDI AA
	// window had been deleted and nothing consumed them.
	// ⚠️⚠️ THIS MUST STAY AHEAD OF ANY HANDLER THAT MIGHT PRINT THEM, and it must PARSE rather than
	// discard: a bare `return` here is exactly what made the tab look broken while the server was in
	// fact sending a perfectly good offer every time.
	if (AAParseTransport(msg)) return;
	// ⚠️⚠️ SWALLOW UNCONDITIONALLY, ACT ONLY IF ENABLED. The Portal overlay is RETIRED
	// (arePortalWindowEnabled is false; the native Travel window replaces it), and the obvious way to
	// retire it -- leaving these gated on the flag -- would stop them being EATEN as well as stop them
	// being shown, so PORTALOPEN, PORTALCLOSE and PORTALDATA would print into the player's chat log
	// and everyone standing nearby would read them too. Swallowing is a separate job from displaying.
	if (msg && strstr(msg, "PORTALOPEN"))  { if (g_portalEnabled) { g_portalVisible = true; g_portalIdleTick = GetTickCount(); } return; }
	if (msg && strstr(msg, "PORTALCLOSE")) { if (g_portalEnabled) { g_portalVisible = false; } return; }
	if (msg && strstr(msg, "PORTALDATA"))  { if (g_portalEnabled) { HandlePortalChat(msg); } return; }
	// The You Lost window lives in its own TU (core_lostwindow.cpp) but cannot re-hook dsp_chat --
	// we own it -- so route LOSTDATA into it from here, exactly as Advanced Loot is routed below.
	if (LostParseTransport(msg)) return;
	if (LostParseLog(msg)) return;                        // swallow LOSTLOG
	if (g_vendorEnabled && HandleVendorChat(msg)) return; // swallow SHOPINVDATA / MYSHOPDATA (shop window)
	// The Allaclone lookup lives in its own TU (core_allaclone.cpp) but cannot re-hook dsp_chat --
	// we own it -- so route SRCHDATA / SRCHDET into it from here, exactly as Advanced Loot is below.
	if (AllacloneParseTransport(msg)) return;             // swallow SRCHDATA / SRCHDET
	if (AllacloneIsOurEcho(msg)) return;                  // and the /say srch* echoes
	// Advanced Loot lives in its own TU (core_advloot.cpp) but cannot re-hook dsp_chat -- we own it --
	// so route LOOTDATA / FILTERDATA / LOOTCLOSE into it from here.
	if (AdvLootParseTransport(msg)) return;
	if (AutoSkillParseTransport(msg)) return;             // swallow ASKILLDATA
	if (AutoSkillIsOurEcho(msg)) return;                  // and the /say ask* echoes
	if (DungeonParseTransport(msg)) return;               // swallow DUNGDATA
	if (DungeonIsOurEcho(msg)) return;                    // and the /say delve* echoes
	if (FellowshipParseTransport(msg)) return;            // swallow FSHIPDATA / FSHIPFIRE / FSHIPMOTD
	if (FellowshipIsOurEcho(msg)) return;                 // and the /say fship* echoes
	if (DifficultyParseTransport(msg)) return;            // swallow DIFFDATA
	if (TradeskillParseTransport(msg)) return;            // swallow TSKIT (skill-dropdown visibility)
	if (DifficultyIsOurEcho(msg)) return;                 // and the /say diff* echoes
	if (TravelParseTransport(msg)) return;                // swallow TRAVELOPEN/DATA/DEST
	if (TravelIsOurEcho(msg)) return;                     // and the /say travelwin|travelgo echoes
	if (SpellJournalParseTransport(msg)) return;          // swallow SJPOOLDATA
	if (SpellJournalParseInfo(msg)) return;               // swallow SJINFO
	if (SpellJournalParseLevels(msg)) return;             // swallow SJLEVELS
	if (SpellJournalIsOurEcho(msg)) return;               // and the /say sjpool echo
	if (AoTMenuParseTransport(msg)) return;               // swallow AOTMENUSHOW
	if (NativeAchievementParseTransport(msg)) return;     // swallow ACH| native-achievement lines
	if (msg && strstr(msg, "You say, '#ach")) return;     // swallow the #ach echo
	SpellChat_Tramp(thisptr, edx, msg, color, eqlog, pct);
}

// ----------------------------------------------------------------- art assets (TGA)
// EQ's spell icons and spellbook background live in uifiles/default as TGA files. We
// can't use EQ's renderer, so we decode the TGAs ourselves into 32-bit DIBs and blit
// them with GDI. Supports uncompressed (type 2) and RLE (type 10), 24/32-bit.
static HBITMAP LoadTGA(const char* path)
{
	FILE* f = nullptr;
	if (fopen_s(&f, path, "rb") != 0 || !f) return nullptr;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz < 18) { fclose(f); return nullptr; }
	std::vector<unsigned char> buf((size_t)sz);
	size_t rd = fread(buf.data(), 1, (size_t)sz, f);
	fclose(f);
	if (rd != (size_t)sz) return nullptr;

	unsigned char idLen   = buf[0];
	unsigned char imgType = buf[2];
	int w = buf[12] | (buf[13] << 8);
	int h = buf[14] | (buf[15] << 8);
	unsigned char depth = buf[16];
	unsigned char desc  = buf[17];
	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return nullptr;
	if (depth != 24 && depth != 32) return nullptr;
	if (imgType != 2 && imgType != 10) return nullptr;

	bool topDown = (desc & 0x20) != 0;
	int  bpp = depth / 8;
	const unsigned char* p   = buf.data() + 18 + idLen;
	const unsigned char* end = buf.data() + sz;

	std::vector<unsigned char> pix((size_t)w * h * 4, 0);
	auto put = [&](int i, const unsigned char* s) {
		int x = i % w, yr = i / w;
		int y = topDown ? yr : (h - 1 - yr);
		size_t d = ((size_t)y * w + x) * 4;
		pix[d + 0] = s[0]; pix[d + 1] = s[1]; pix[d + 2] = s[2];      // BGR
		pix[d + 3] = (bpp == 4) ? s[3] : 255;
	};

	int total = w * h;
	if (imgType == 2) {
		for (int i = 0; i < total && p + bpp <= end; ++i, p += bpp) put(i, p);
	} else { // RLE
		int i = 0;
		while (i < total && p < end) {
			unsigned char hdr = *p++;
			int count = (hdr & 0x7F) + 1;
			if (hdr & 0x80) {                       // run packet
				if (p + bpp > end) break;
				for (int c = 0; c < count && i < total; ++c, ++i) put(i, p);
				p += bpp;
			} else {                                // raw packet
				for (int c = 0; c < count && i < total && p + bpp <= end; ++c, ++i, p += bpp) put(i, p);
			}
		}
	}

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;   // top-down
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HBITMAP bmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (bmp && bits) memcpy(bits, pix.data(), pix.size());
	return bmp;
}

// Build a top-down 32-bit DIB from a BGRA pixel buffer (shared by the DDS decoder).
static HBITMAP bmpFrom(const std::vector<unsigned char>& pix, int w, int h)
{
	BITMAPINFO bi = {};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;   // top-down
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HBITMAP bmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (bmp && bits) memcpy(bits, pix.data(), pix.size());
	return bmp;
}

// Item-icon sheets (dragitemNN.dds) are DirectDraw Surface files, not TGA. Decode DXT1/DXT3/DXT5 block
// compression + uncompressed 32-bit into a top-down 32-bit DIB (same output shape as LoadTGA).
static HBITMAP LoadDDS(const char* path)
{
	FILE* f = nullptr;
	if (fopen_s(&f, path, "rb") != 0 || !f) return nullptr;
	fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
	if (sz < 128) { fclose(f); return nullptr; }
	std::vector<unsigned char> buf((size_t)sz);
	size_t rd = fread(buf.data(), 1, (size_t)sz, f); fclose(f);
	if (rd != (size_t)sz || memcmp(buf.data(), "DDS ", 4) != 0) return nullptr;

	auto u32 = [&](int off) -> unsigned { return buf[off] | (buf[off+1]<<8) | (buf[off+2]<<16) | ((unsigned)buf[off+3]<<24); };
	int      h = (int)u32(12), w = (int)u32(16);      // DDS_HEADER: height @12, width @16
	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return nullptr;
	unsigned pfFlags = u32(80), fourCC = u32(84);     // DDS_PIXELFORMAT @76: flags @80, fourCC @84
	const unsigned char* data = buf.data() + 128;     // pixel data follows the 128-byte header
	const unsigned char* end  = buf.data() + sz;

	std::vector<unsigned char> pix((size_t)w * h * 4, 0);   // BGRA, top-down
	auto setpx = [&](int x, int y, int r, int g, int b, int a) {
		if (x < 0 || x >= w || y < 0 || y >= h) return;
		size_t d = ((size_t)y * w + x) * 4;
		pix[d+0] = (unsigned char)b; pix[d+1] = (unsigned char)g; pix[d+2] = (unsigned char)r; pix[d+3] = (unsigned char)a;
	};
	const unsigned DXT1 = 0x31545844, DXT3 = 0x33545844, DXT5 = 0x35545844;

	if (pfFlags & 0x4) {                              // DDPF_FOURCC -> block compressed
		bool isDXT1 = (fourCC == DXT1);
		int  blockBytes = isDXT1 ? 8 : 16;
		const unsigned char* p = data;
		for (int by = 0; by < h; by += 4) {
			for (int bx = 0; bx < w; bx += 4) {
				if (p + blockBytes > end) return bmpFrom(pix, w, h);
				unsigned char ba[16];
				const unsigned char* color = p + (isDXT1 ? 0 : 8);
				if (fourCC == DXT3) {
					for (int i = 0; i < 16; ++i) { int nib = (p[i/2] >> ((i&1)*4)) & 0xF; ba[i] = (unsigned char)(nib * 17); }
				} else if (fourCC == DXT5) {
					int a0 = p[0], a1 = p[1];
					unsigned long long ab = 0; for (int i = 0; i < 6; ++i) ab |= (unsigned long long)p[2+i] << (8*i);
					for (int i = 0; i < 16; ++i) {
						int idx = (int)((ab >> (3*i)) & 7), av;
						if (a0 > a1) { av = (idx==0)?a0 : (idx==1)?a1 : ((8-idx)*a0 + (idx-1)*a1)/7; }
						else         { av = (idx==0)?a0 : (idx==1)?a1 : (idx==6)?0 : (idx==7)?255 : ((6-idx)*a0 + (idx-1)*a1)/5; }
						ba[i] = (unsigned char)av;
					}
				} else { for (int i = 0; i < 16; ++i) ba[i] = 255; }
				unsigned c0 = color[0] | (color[1]<<8), c1 = color[2] | (color[3]<<8);
				int cr[4], cg[4], cb[4];
				auto e565 = [](unsigned c, int& r, int& g, int& b) {
					r = (int)(((c>>11)&0x1F) * 255 / 31); g = (int)(((c>>5)&0x3F) * 255 / 63); b = (int)((c&0x1F) * 255 / 31);
				};
				e565(c0, cr[0], cg[0], cb[0]); e565(c1, cr[1], cg[1], cb[1]);
				if (c0 > c1 || !isDXT1) {
					cr[2]=(2*cr[0]+cr[1])/3; cg[2]=(2*cg[0]+cg[1])/3; cb[2]=(2*cb[0]+cb[1])/3;
					cr[3]=(cr[0]+2*cr[1])/3; cg[3]=(cg[0]+2*cg[1])/3; cb[3]=(cb[0]+2*cb[1])/3;
				} else {
					cr[2]=(cr[0]+cr[1])/2; cg[2]=(cg[0]+cg[1])/2; cb[2]=(cb[0]+cb[1])/2;
					cr[3]=cg[3]=cb[3]=0;
				}
				unsigned ci = color[4] | (color[5]<<8) | (color[6]<<16) | ((unsigned)color[7]<<24);
				for (int i = 0; i < 16; ++i) {
					int s = (ci >> (2*i)) & 3, a = ba[i];
					if (isDXT1 && c0 <= c1 && s == 3) a = 0;     // DXT1 1-bit alpha
					setpx(bx + (i%4), by + (i/4), cr[s], cg[s], cb[s], a);
				}
				p += blockBytes;
			}
		}
	} else {                                          // uncompressed (RGB/RGBA via masks)
		int bpp = (int)u32(88) / 8; if (bpp < 3) return nullptr;
		unsigned rm = u32(92), gm = u32(96), bm = u32(100), am = u32(104);
		auto ext = [](unsigned px, unsigned mask) -> int {
			if (!mask) return 255; unsigned sh = 0; while (!((mask>>sh)&1)) ++sh;
			unsigned mv = mask >> sh; return (int)(((px & mask) >> sh) * 255 / (mv ? mv : 1));
		};
		const unsigned char* p = data;
		for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
			if (p + bpp > end) break;
			unsigned px = 0; for (int k = 0; k < bpp; ++k) px |= (unsigned)p[k] << (8*k);
			setpx(x, y, ext(px, rm), ext(px, gm), ext(px, bm), am ? ext(px, am) : 255);
			p += bpp;
		}
	}
	return bmpFrom(pix, w, h);
}

static void InitAssets()
{
	if (g_assetsInit) return;
	g_assetsInit = true;

	char exe[MAX_PATH] = {0};
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	char* slash = strrchr(exe, '\\');
	if (slash) slash[1] = 0; else exe[0] = 0;
	sprintf_s(g_uiDir,  "%suifiles\\default\\", exe);
	sprintf_s(g_eqRoot, "%s", exe);   // EQ root (some clients keep dragitemNN.tga here, not in uifiles)
}

// Spellbook icon sheets: spells01.tga.. each 6x6 grid of 40x40 icons (icon index 1-based).
static HBITMAP GetSheet(int sheetNum)
{
	for (int i = 0; i < g_sheetCacheN; ++i)
		if (g_sheetCache[i].sheet == sheetNum) return g_sheetCache[i].bmp;
	char path[MAX_PATH];
	sprintf_s(path, "%sspells%02d.tga", g_uiDir, sheetNum);
	HBITMAP bmp = LoadTGA(path);
	// Only cache SUCCESSFUL loads. The old code cached null too, so a single transient/locked read
	// blanked that whole sheet for the rest of the session ("random" missing icons). Retrying a
	// genuinely-missing file each frame is cheap (a failed fopen).
	if (bmp && g_sheetCacheN < 16) { g_sheetCache[g_sheetCacheN].sheet = sheetNum;
	                                 g_sheetCache[g_sheetCacheN].bmp = bmp; ++g_sheetCacheN; }
	return bmp;
}

static void BlitIcon(HDC dst, int iconIdx, const RECT& r)
{
	if (iconIdx <= 0) return;
	int sheet = iconIdx / 36 + 1;        // spells01 holds icons 0-35 (raw index, 0=none)
	int cell  = iconIdx % 36;
	HBITMAP sh = GetSheet(sheet);
	if (!sh) {
		// Sheet file missing or unreadable. Draw a visible placeholder instead of nothing, so the
		// row never looks empty -- and so a missing spellsNN.tga is obvious rather than silent.
		HBRUSH ph = CreateSolidBrush(RGB(54, 60, 78)); FillRect(dst, &r, ph); DeleteObject(ph);
		return;
	}
	// derive columns from the actual sheet width (40px cells) instead of assuming 6
	BITMAP bm; GetObject(sh, sizeof(bm), &bm);
	int cols = bm.bmWidth / ICON_SZ; if (cols < 1) cols = 1;
	int sx = (cell % cols) * ICON_SZ;
	int sy = (cell / cols) * ICON_SZ;
	HDC sdc = CreateCompatibleDC(dst);
	HBITMAP oldS = (HBITMAP)SelectObject(sdc, sh);
	BitBlt(dst, r.left, r.top, ICON_SZ, ICON_SZ, sdc, sx, sy, SRCCOPY);
	SelectObject(sdc, oldS);
	DeleteDC(sdc);
}

// Item icons (Advanced Loot window) live in uifiles/default/dragitemNN.tga -- a different sheet set than
// spell icons. Each sheet is a 6x6 grid of 40x40 cells; item icon values are numbered from 500
// (dragitem1 = icons 500-535). Unlike BlitIcon we StretchBlt so the 40px cell scales into the shorter
// loot row. Own cache so item-sheet numbers don't collide with the spell-sheet cache.
static HBITMAP GetItemSheet(int sheetNum)
{
	for (int i = 0; i < g_itemSheetCacheN; ++i)
		if (g_itemSheetCache[i].sheet == sheetNum) return g_itemSheetCache[i].bmp;
	char path[MAX_PATH];
	// Item icons are dragitemNN.DDS (DirectDraw Surface) in uifiles/default. Fall back to .tga / EQ root
	// for other client layouts. First one that loads wins.
	HBITMAP bmp = nullptr;
	sprintf_s(path, "%sdragitem%d.dds", g_uiDir,  sheetNum); bmp = LoadDDS(path);
	if (!bmp) { sprintf_s(path, "%sdragitem%d.tga", g_uiDir,  sheetNum); bmp = LoadTGA(path); }
	if (!bmp) { sprintf_s(path, "%sdragitem%d.dds", g_eqRoot, sheetNum); bmp = LoadDDS(path); }
	if (bmp && g_itemSheetCacheN < 32) { g_itemSheetCache[g_itemSheetCacheN].sheet = sheetNum;
	                                     g_itemSheetCache[g_itemSheetCacheN].bmp = bmp; ++g_itemSheetCacheN; }
	return bmp;
}

static const int ITEM_ICON_BASE = 500;   // item icon values start here (dragitem1 cell 0)
static int g_itemPerSheet = 0;            // icons per dragitem sheet -- derived from sheet 1's real size
static void BlitItemIcon(HDC dst, int iconIdx, const RECT& r)
{
	if (iconIdx < ITEM_ICON_BASE) return;         // 0/invalid -> caller's placeholder tile shows
	if (g_itemPerSheet == 0) {                    // one-time: read grid size from dragitem1 (cols*rows)
		HBITMAP s1 = GetItemSheet(1);
		if (s1) { BITMAP b1; GetObject(s1, sizeof(b1), &b1);
		          g_itemPerSheet = (b1.bmWidth / ICON_SZ) * (b1.bmHeight / ICON_SZ); }
		if (g_itemPerSheet <= 0) g_itemPerSheet = 36;   // fallback to the 6x6 assumption
	}
	int local = iconIdx - ITEM_ICON_BASE;
	int sheet = local / g_itemPerSheet + 1;       // dragitem1 = icons 500..500+perSheet-1, etc.
	int cell  = local % g_itemPerSheet;
	HBITMAP sh = GetItemSheet(sheet);
	if (!sh) return;                              // sheet missing -> placeholder shows
	BITMAP bm; GetObject(sh, sizeof(bm), &bm);
	int cols = bm.bmWidth / ICON_SZ; if (cols < 1) cols = 1;
	int sx = (cell % cols) * ICON_SZ;
	int sy = (cell / cols) * ICON_SZ;
	HDC sdc = CreateCompatibleDC(dst);
	HBITMAP oldS = (HBITMAP)SelectObject(sdc, sh);
	SetStretchBltMode(dst, HALFTONE);
	StretchBlt(dst, r.left, r.top, r.right - r.left, r.bottom - r.top, sdc, sx, sy, ICON_SZ, ICON_SZ, SRCCOPY);
	SelectObject(sdc, oldS);
	DeleteDC(sdc);
}

// Draw text with a 1px offset highlight/shadow for an embossed, readable look.
static void DrawShadow(HDC dc, const char* s, RECT r, UINT fmt, COLORREF col, COLORREF shadow)
{
	RECT sr = r; OffsetRect(&sr, 1, 1);
	SetTextColor(dc, shadow);
	DrawTextA(dc, s, -1, &sr, fmt);
	SetTextColor(dc, col);
	DrawTextA(dc, s, -1, &r, fmt);
}

// True while a character is in the world; the local player pointer is null at char-select/login/zoning,
// so we use it to auto-hide every overlay window when you're not actually in game.
static bool IsInGame() { return *(void**)((0xDD2630 - 0x400000) + baseAddress) != nullptr; }

// True if `w` is one of OUR overlay windows (all register a class named "AoT..."). Used so that focusing
// one overlay doesn't hide the others -- they all count as "our app is foreground", shown together.
static bool IsAoTOverlay(HWND w)
{
	if (!w) return false;
	char cls[48];
	return GetClassNameA(w, cls, sizeof(cls)) > 0 && strncmp(cls, "AoT", 3) == 0;
}

// A small red [X] close button in a window's top-right corner (w = window width). Shared by all windows.
static void CloseXRect(int w, RECT& r) { r.right = w - 5; r.left = r.right - 18; r.top = 4; r.bottom = 20; }
static void DrawCloseX(HDC mem, const RECT& r)
{
	HBRUSH bg = CreateSolidBrush(RGB(130, 46, 46)); FillRect(mem, (RECT*)&r, bg); DeleteObject(bg);
	HBRUSH ed = CreateSolidBrush(RGB(20, 12, 12));  FrameRect(mem, (RECT*)&r, ed); DeleteObject(ed);
	HPEN   pen = CreatePen(PS_SOLID, 2, RGB(245, 225, 225)); HGDIOBJ op = SelectObject(mem, pen);
	MoveToEx(mem, r.left + 5, r.top + 4, nullptr);  LineTo(mem, r.right - 4, r.bottom - 4);
	MoveToEx(mem, r.right - 5, r.top + 4, nullptr); LineTo(mem, r.left + 4,  r.bottom - 4);
	SelectObject(mem, op); DeleteObject(pen);
}

// ----------------------------------------------------------------- Portal window paint/input
static const int PORTAL_W   = 440;
static const int PORTAL_TOP = 34;
static const int PROW_H     = 30;
static const int PORTAL_VIS = 9;                              // rows shown at once (scroll for more)
static const int PORTAL_H   = PORTAL_TOP + PORTAL_VIS * PROW_H + 16;

static void PortalBtnRect(int i, RECT& r)
{
	r.right  = PORTAL_W - 14;
	r.left   = r.right - 90;
	r.top    = PORTAL_TOP + i * PROW_H + 4;
	r.bottom = r.top + PROW_H - 8;
}

// Up/Down scroll buttons in the title bar (shown only when the list overflows). Shifted left to leave
// room for the [X] close button in the top-right corner.
static void PortalUpRect(RECT& r)   { r.top = 7; r.bottom = PORTAL_TOP - 6; r.right = PORTAL_W - 72; r.left = r.right - 30; }
static void PortalDownRect(RECT& r) { r.top = 7; r.bottom = PORTAL_TOP - 6; r.right = PORTAL_W - 38; r.left = r.right - 30; }
static const int PORTAL_PAGE = PORTAL_VIS - 1;   // rows per Up/Dn click

static void PaintPortalOverlay(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	HDC     mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, PORTAL_W, PORTAL_H);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < PORTAL_H; ++y) {            // teal gradient (distinct from spell/AA themes)
		float t = (float)y / (float)PORTAL_H;
		HBRUSH ln = CreateSolidBrush(RGB((int)(26 + 14 * t), (int)(54 + 22 * t), (int)(50 + 20 * t)));
		RECT lr = { 0, y, PORTAL_W, y + 1 };
		FillRect(mem, &lr, ln);
		DeleteObject(ln);
	}
	RECT full = { 0, 0, PORTAL_W, PORTAL_H };
	HBRUSH bo = CreateSolidBrush(RGB(16, 30, 28)); FrameRect(mem, &full, bo); DeleteObject(bo);
	RECT inner = { 2, 2, PORTAL_W - 2, PORTAL_H - 2 };
	HBRUSH bi = CreateSolidBrush(RGB(110, 180, 160)); FrameRect(mem, &inner, bi); DeleteObject(bi);

	HFONT font = CreateFontA(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT footFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	char title[64]; sprintf_s(title, "Discovered Portals  (%d)", g_portalCount);
	RECT tr = { 14, 6, PORTAL_W - 96, PORTAL_TOP };
	DrawShadow(mem, title, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(220, 240, 232), RGB(12, 24, 22));
	{ RECT px; CloseXRect(PORTAL_W, px); DrawCloseX(mem, px); }
	HBRUSH div = CreateSolidBrush(RGB(110, 180, 160));
	RECT dr = { 12, PORTAL_TOP - 2, PORTAL_W - 12, PORTAL_TOP - 1 };
	FillRect(mem, &dr, div); DeleteObject(div);

	if (g_portalCount > PORTAL_VIS) {                  // Up/Dn scroll buttons
		RECT ur, dn; PortalUpRect(ur); PortalDownRect(dn);
		const RECT* sb[2] = { &ur, &dn };
		const char* lbl[2] = { "Up", "Dn" };
		for (int b = 0; b < 2; ++b) {
			HBRUSH bg = CreateSolidBrush(RGB(46, 92, 80)); FillRect(mem, (RECT*)sb[b], bg); DeleteObject(bg);
			HBRUSH ed = CreateSolidBrush(RGB(16, 30, 28)); FrameRect(mem, (RECT*)sb[b], ed); DeleteObject(ed);
			DrawShadow(mem, lbl[b], *sb[b], DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(240, 250, 245), RGB(10, 20, 18));
		}
	}

	if (g_portalCount == 0) {
		RECT er = { 16, PORTAL_TOP + 10, PORTAL_W - 16, PORTAL_TOP + 60 };
		DrawShadow(mem, "No portals discovered yet.\nClick a Plane of Knowledge book to attune.",
			er, DT_LEFT | DT_WORDBREAK, RGB(210, 225, 220), RGB(12, 24, 22));
	}

	for (int i = 0; i < PORTAL_VIS; ++i) {
		int idx = g_portalScroll + i;
		if (idx >= g_portalCount) break;
		RECT nr = { 16, PORTAL_TOP + i * PROW_H + 4, PORTAL_W - 110, PORTAL_TOP + i * PROW_H + PROW_H - 2 };
		DrawShadow(mem, g_portalName[idx], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(232, 244, 238), RGB(12, 24, 22));
		RECT br; PortalBtnRect(i, br);
		HBRUSH btn = CreateSolidBrush(RGB(46, 92, 80)); FillRect(mem, &br, btn); DeleteObject(btn);
		HBRUSH be = CreateSolidBrush(RGB(16, 30, 28)); FrameRect(mem, &br, be); DeleteObject(be);
		DrawShadow(mem, "Travel", br, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(240, 250, 245), RGB(10, 20, 18));
	}

	if (g_portalCount > PORTAL_VIS) {
		int last = g_portalScroll + PORTAL_VIS; if (last > g_portalCount) last = g_portalCount;
		char f[80]; sprintf_s(f, "mouse-wheel to scroll  (%d-%d of %d)", g_portalScroll + 1, last, g_portalCount);
		SelectObject(mem, footFont);
		RECT fr = { 12, PORTAL_H - 16, PORTAL_W - 12, PORTAL_H - 3 };
		DrawShadow(mem, f, fr, DT_RIGHT | DT_SINGLELINE, RGB(180, 210, 200), RGB(12, 24, 22));
	}

	BitBlt(hdc, 0, 0, PORTAL_W, PORTAL_H, mem, 0, 0, SRCCOPY);
	SelectObject(mem, oldFont);
	DeleteObject(font); DeleteObject(footFont);
	SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK PortalOverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT:
		PaintPortalOverlay(hwnd);
		return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		g_portalIdleTick = GetTickCount();
		// Take focus on an actual click (not passive hover) so EQ doesn't also see the button via
		// DirectInput and mouse-look / click into the world behind the window while we drag.
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);
		{ RECT px; CloseXRect(PORTAL_W, px); if (PtInRect(&px, pt)) { g_portalVisible = false; ShowWindow(hwnd, SW_HIDE); return 0; } }
		if (g_portalCount > PORTAL_VIS) {                 // Up/Dn scroll buttons (in the title strip)
			RECT ur, dn; PortalUpRect(ur); PortalDownRect(dn);
			int maxs = g_portalCount - PORTAL_VIS;
			if (PtInRect(&ur, pt)) {
				g_portalScroll -= PORTAL_PAGE; if (g_portalScroll < 0) g_portalScroll = 0;
				InvalidateRect(hwnd, nullptr, FALSE); return 0;
			}
			if (PtInRect(&dn, pt)) {
				g_portalScroll += PORTAL_PAGE; if (g_portalScroll > maxs) g_portalScroll = maxs;
				InvalidateRect(hwnd, nullptr, FALSE); return 0;
			}
		}
		if (pt.y < PORTAL_TOP) { g_pDragging = true; g_pDragDX = pt.x; g_pDragDY = pt.y; SetCapture(hwnd); }
		return 0;
	}

	case WM_MOUSEMOVE: {
		g_portalIdleTick = GetTickCount();           // interacting -> keep open
		// Do NOT grab EQ's foreground on passive hover: the window opens centered under the cursor, so
		// a hover-grab tabs EQ out the instant the mouse twitches -- and a held movement key (e.g. S)
		// then sticks, because EQ never sees the key-up after losing focus (DirectInput is unacquired
		// on focus loss). Focus is taken only on an actual click (WM_LBUTTONDOWN) or wheel scroll.
		if (g_pDragging) {
			POINT cur; GetCursorPos(&cur);
			g_pPosX = cur.x - g_pDragDX; g_pPosY = cur.y - g_pDragDY; g_pPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_pPosX, g_pPosY, PORTAL_W, PORTAL_H, SWP_NOACTIVATE);
		}
		return 0;
	}

	case WM_LBUTTONUP: {
		if (g_pDragging) { g_pDragging = false; ReleaseCapture(); return 0; }
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		for (int i = 0; i < PORTAL_VIS; ++i) {
			int idx = g_portalScroll + i;
			if (idx >= g_portalCount) break;
			RECT r; PortalBtnRect(i, r);
			if (PtInRect(&r, pt)) {
				strncpy_s(g_portalPendingShort, 64, g_portalShort[idx], _TRUNCATE);  // travel on game thread
				g_portalVisible = false;
				ShowWindow(hwnd, SW_HIDE);
				break;
			}
		}
		return 0;
	}

	case WM_RBUTTONUP:               // right-click dismisses the window
		g_portalVisible = false;
		ShowWindow(hwnd, SW_HIDE);
		return 0;

	case WM_MOUSEWHEEL: {
		g_portalIdleTick = GetTickCount();
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);  // grab focus so EQ stops zooming
		int d = GET_WHEEL_DELTA_WPARAM(wp);
		g_portalScroll += (d > 0) ? -1 : 1;
		int maxs = g_portalCount - PORTAL_VIS; if (maxs < 0) maxs = 0;
		if (g_portalScroll < 0) g_portalScroll = 0;
		if (g_portalScroll > maxs) g_portalScroll = maxs;
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}

	case WM_TIMER: {
		// idle timeout: close after no interaction (covers walking away)
		if (g_portalVisible && (GetTickCount() - g_portalIdleTick) > PORTAL_TIMEOUT_MS) {
			g_portalVisible = false;
		}
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_portalVisible && active) {
			if (!g_pPositioned && eq) {
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top }; ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
				g_pPosX = tl.x + (cw - PORTAL_W) / 2;
				g_pPosY = tl.y + (ch - PORTAL_H) / 2;
				g_pPositioned = true;
			}
			if (g_pPositioned) {
				if (!IsWindowVisible(hwnd)) {
					SetWindowPos(hwnd, HWND_TOPMOST, g_pPosX, g_pPosY, PORTAL_W, PORTAL_H, SWP_NOACTIVATE);
					ShowWindow(hwnd, SW_SHOWNOACTIVATE);
				}
				InvalidateRect(hwnd, nullptr, FALSE);
			}
		} else if (IsWindowVisible(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
		return 0;
	}

	case WM_DESTROY:
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI PortalOverlayThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);
	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = PortalOverlayWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTPortalOverlay";
	RegisterClassExA(&wc);

	g_portalHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTPortalOverlay", "", WS_POPUP,
		0, 0, PORTAL_W, PORTAL_H, nullptr, nullptr, hInst, nullptr);
	if (!g_portalHwnd) return 0;

	SetLayeredWindowAttributes(g_portalHwnd, 0, 240, LWA_ALPHA);
	SetTimer(g_portalHwnd, 1, 60, nullptr);

	MSG m;
	while (GetMessageA(&m, nullptr, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessageA(&m);
	}
	return 0;
}



// ----------------------------------------------------------------- "You Lost" window paint/input
static const int LOST_W    = 440;
static const int LOST_TOP  = 34;
static const int LROW_H    = 26;
static const int LOST_VIS  = 10;
static const int LOST_H    = LOST_TOP + LOST_VIS * LROW_H + 16;
static const int LOST_PAGE = LOST_VIS - 1;

static void LostUpRect(RECT& r)   { r.top = 7; r.bottom = LOST_TOP - 6; r.right = LOST_W - 48; r.left = r.right - 30; }
static void LostDownRect(RECT& r) { r.top = 7; r.bottom = LOST_TOP - 6; r.right = LOST_W - 14; r.left = r.right - 30; }

static void PaintLostOverlay(HWND hwnd)
{
	PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, LOST_W, LOST_H);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < LOST_H; ++y) {                 // dark red gradient (death)
		float t = (float)y / (float)LOST_H;
		HBRUSH ln = CreateSolidBrush(RGB((int)(48 + 20 * t), (int)(16 + 8 * t), (int)(16 + 8 * t)));
		RECT lr = { 0, y, LOST_W, y + 1 }; FillRect(mem, &lr, ln); DeleteObject(ln);
	}
	RECT full = { 0, 0, LOST_W, LOST_H };
	HBRUSH bo = CreateSolidBrush(RGB(30, 8, 8)); FrameRect(mem, &full, bo); DeleteObject(bo);
	RECT inner = { 2, 2, LOST_W - 2, LOST_H - 2 };
	HBRUSH bi = CreateSolidBrush(RGB(150, 60, 55)); FrameRect(mem, &inner, bi); DeleteObject(bi);

	HFONT font = CreateFontA(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT rowFont = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT footFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	char title[48]; sprintf_s(title, "You Lost  (%d)", g_lostCount);
	RECT tr = { 14, 6, LOST_W - 96, LOST_TOP };
	DrawShadow(mem, title, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(255, 225, 220), RGB(20, 6, 6));
	HBRUSH div = CreateSolidBrush(RGB(150, 60, 55));
	RECT dr = { 12, LOST_TOP - 2, LOST_W - 12, LOST_TOP - 1 }; FillRect(mem, &dr, div); DeleteObject(div);

	if (g_lostCount > LOST_VIS) {                       // Up/Dn scroll buttons
		RECT ur, dn; LostUpRect(ur); LostDownRect(dn);
		const RECT* sb[2] = { &ur, &dn }; const char* lbl[2] = { "Up", "Dn" };
		for (int b = 0; b < 2; ++b) {
			HBRUSH bg = CreateSolidBrush(RGB(96, 40, 38)); FillRect(mem, (RECT*)sb[b], bg); DeleteObject(bg);
			HBRUSH ed = CreateSolidBrush(RGB(30, 8, 8)); FrameRect(mem, (RECT*)sb[b], ed); DeleteObject(ed);
			DrawShadow(mem, lbl[b], *sb[b], DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(255, 240, 238), RGB(20, 6, 6));
		}
	}

	SelectObject(mem, rowFont);
	for (int i = 0; i < LOST_VIS; ++i) {
		int idx = g_lostScroll + i;
		if (idx >= g_lostCount) break;
		RECT nr = { 18, LOST_TOP + i * LROW_H + 2, LOST_W - 16, LOST_TOP + i * LROW_H + LROW_H - 1 };
		DrawShadow(mem, g_lostName[idx], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(240, 215, 212), RGB(20, 6, 6));
	}

	SelectObject(mem, footFont);
	char f[96];
	if (g_lostCount > LOST_VIS) {
		int last = g_lostScroll + LOST_VIS; if (last > g_lostCount) last = g_lostCount;
		sprintf_s(f, "%d-%d of %d   -   right-click to dismiss", g_lostScroll + 1, last, g_lostCount);
	} else {
		sprintf_s(f, "right-click to dismiss");
	}
	RECT fr = { 12, LOST_H - 16, LOST_W - 12, LOST_H - 3 };
	DrawShadow(mem, f, fr, DT_RIGHT | DT_SINGLELINE, RGB(210, 170, 165), RGB(20, 6, 6));

	BitBlt(hdc, 0, 0, LOST_W, LOST_H, mem, 0, 0, SRCCOPY);
	SelectObject(mem, oldFont);
	DeleteObject(font); DeleteObject(rowFont); DeleteObject(footFont);
	SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK LostOverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: PaintLostOverlay(hwnd); return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		g_lostIdleTick = GetTickCount();
		if (g_lostCount > LOST_VIS) {
			RECT ur, dn; LostUpRect(ur); LostDownRect(dn);
			int maxs = g_lostCount - LOST_VIS;
			if (PtInRect(&ur, pt)) { g_lostScroll -= LOST_PAGE; if (g_lostScroll < 0) g_lostScroll = 0; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
			if (PtInRect(&dn, pt)) { g_lostScroll += LOST_PAGE; if (g_lostScroll > maxs) g_lostScroll = maxs; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		}
		if (pt.y < LOST_TOP) { g_lDragging = true; g_lDragDX = pt.x; g_lDragDY = pt.y; SetCapture(hwnd); }
		return 0;
	}

	case WM_MOUSEMOVE: {
		g_lostIdleTick = GetTickCount();
		if (g_lDragging) {
			POINT cur; GetCursorPos(&cur);
			g_lPosX = cur.x - g_lDragDX; g_lPosY = cur.y - g_lDragDY; g_lPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_lPosX, g_lPosY, LOST_W, LOST_H, SWP_NOACTIVATE);
		}
		return 0;
	}

	case WM_LBUTTONUP:
		if (g_lDragging) { g_lDragging = false; ReleaseCapture(); }
		return 0;

	case WM_RBUTTONUP:               // right-click dismisses
		g_lostVisible = false; ShowWindow(hwnd, SW_HIDE);
		return 0;

	case WM_MOUSEWHEEL: {
		g_lostIdleTick = GetTickCount();
		int d = GET_WHEEL_DELTA_WPARAM(wp);
		g_lostScroll += (d > 0) ? -1 : 1;
		int maxs = g_lostCount - LOST_VIS; if (maxs < 0) maxs = 0;
		if (g_lostScroll < 0) g_lostScroll = 0;
		if (g_lostScroll > maxs) g_lostScroll = maxs;
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}

	case WM_TIMER: {
		if (g_lostVisible && (GetTickCount() - g_lostIdleTick) > LOST_TIMEOUT_MS) g_lostVisible = false;
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_lostVisible && active) {
			if (!g_lPositioned && eq) {
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top }; ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left;
				g_lPosX = tl.x + (cw - LOST_W) / 2;
				g_lPosY = tl.y + 24;            // top of the screen (AA offer sits at the bottom)
				g_lPositioned = true;
			}
			if (g_lPositioned) {
				if (!IsWindowVisible(hwnd)) {   // position + z-order only on show (no per-tick re-assert)
					SetWindowPos(hwnd, HWND_TOPMOST, g_lPosX, g_lPosY, LOST_W, LOST_H, SWP_NOACTIVATE);
					ShowWindow(hwnd, SW_SHOWNOACTIVATE);
				}
				InvalidateRect(hwnd, nullptr, FALSE);
			}
		} else if (IsWindowVisible(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
		return 0;
	}

	case WM_DESTROY:
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI LostOverlayThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);
	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = LostOverlayWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTLostOverlay";
	RegisterClassExA(&wc);

	g_lostHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTLostOverlay", "", WS_POPUP,
		0, 0, LOST_W, LOST_H, nullptr, nullptr, hInst, nullptr);
	if (!g_lostHwnd) return 0;

	SetLayeredWindowAttributes(g_lostHwnd, 0, 240, LWA_ALPHA);
	SetTimer(g_lostHwnd, 1, 60, nullptr);

	MSG m;
	while (GetMessageA(&m, nullptr, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessageA(&m);
	}
	return 0;
}

// ================================================================= Shop window (Add Items / My Shop)
// State + chat handlers are declared earlier (before the dsp_chat detour). Tab 0 = Add Items: price your
// droppable bag items and Add -> escrows them into the shop. Tab 1 = My Shop: each listing has a Pull
// button to take it back. Layout below.
static const int VW = 560, VTBAR_H = 24, VTAB_H = 26, VTITLE_H = 22, VHDR_H = 16, VROW_H = 28, VVIS = 9, VFOOT_H = 44;
static const int VLIST_Y = VTBAR_H + VTAB_H + VTITLE_H + VHDR_H;
static const int VH = VLIST_Y + VVIS * VROW_H + VFOOT_H;
static const char* V_LBL[4] = { "Plat", "Gold", "Slv", "Cop" };

static int  VRowY(int vis)  { return VLIST_Y + vis * VROW_H; }
static int  VCount()        { return g_vendorTab == 0 ? g_vendorCount : g_myCount; }
static int& VScroll()       { return g_vendorTab == 0 ? g_vendorScroll : g_myScroll; }
static void VTabBtn(int t, RECT& r) { int w = (VW - 24 - 22) / 2; r.left = 10 + t * (w + 4); r.right = r.left + w; r.top = VTBAR_H + 3; r.bottom = VTBAR_H + VTAB_H - 3; }  // below the title bar
// Add-tab: 4 coin fields per row, right-aligned (d = 0 plat .. 3 copper)
static void VFieldX(int d, int& left, int& right) { right = VW - 14 - (3 - d) * 50; left = right - 46; }
static void VField(int vis, int d, RECT& r) { int l, rt; VFieldX(d, l, rt); r.left = l; r.right = rt; r.top = VRowY(vis) + 3; r.bottom = r.top + 22; }
// My-Shop-tab: a Pull button per row
static void VPullBtn(int vis, RECT& r) { r.right = VW - 14; r.left = r.right - 62; r.top = VRowY(vis) + 3; r.bottom = r.top + 22; }
static void VUpBtn (RECT& r) { r.top = VTBAR_H + VTAB_H + 3; r.bottom = VTBAR_H + VTAB_H + VTITLE_H - 2; r.right = VW - 46; r.left = r.right - 28; }
static void VDnBtn (RECT& r) { r.top = VTBAR_H + VTAB_H + 3; r.bottom = VTBAR_H + VTAB_H + VTITLE_H - 2; r.right = VW - 14; r.left = r.right - 28; }
static void VActBtn(RECT& r) { r.left = 14;         r.right = VW / 2 - 8; r.top = VH - VFOOT_H + 8; r.bottom = VH - 10; }
static void VRefBtn(RECT& r) { r.left = VW / 2 + 8; r.right = VW - 14;    r.top = VH - VFOOT_H + 8; r.bottom = VH - 10; }
static const int VTITLE_TOP = VTBAR_H + VTAB_H;   // hint strip under the tabs

// EQ-style 3D bevel: raised = light top/left + dark bottom/right; sunken = the inverse. `px` = thickness.
static void VBevel(HDC mem, RECT r, bool raised, int px = 1)
{
	const COLORREF lt = RGB(104, 108, 120), dk = RGB(15, 16, 21);
	const COLORREF tl = raised ? lt : dk, br = raised ? dk : lt;
	for (int k = 0; k < px; ++k) {
		HPEN pt = CreatePen(PS_SOLID, 1, tl), pb = CreatePen(PS_SOLID, 1, br);
		HPEN op = (HPEN)SelectObject(mem, pt);
		MoveToEx(mem, r.left, r.bottom - 1, nullptr); LineTo(mem, r.left, r.top); LineTo(mem, r.right - 1, r.top);
		SelectObject(mem, pb);
		LineTo(mem, r.right - 1, r.bottom - 1); LineTo(mem, r.left, r.bottom - 1);
		SelectObject(mem, op); DeleteObject(pt); DeleteObject(pb);
		r.left++; r.top++; r.right--; r.bottom--;
	}
}

// EQ-style button: raised beveled plate, grey normally / blue-highlight when active or primary. Cream label.
static void VDrawBtn(HDC mem, HFONT f, const RECT& r, const char* s, bool on)
{
	HBRUSH bg = CreateSolidBrush(on ? RGB(84, 88, 98) : RGB(58, 62, 72)); FillRect(mem, (RECT*)&r, bg); DeleteObject(bg);
	VBevel(mem, r, true);                              // EQ buttons read as raised grey plates
	SelectObject(mem, f);
	DrawShadow(mem, s, r, DT_CENTER | DT_VCENTER | DT_SINGLELINE, on ? RGB(240, 232, 205) : RGB(216, 210, 190), RGB(8, 9, 12));
}

static void VendorPaint(HWND hwnd)
{
	PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, VW, VH);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < VH; ++y) {                          // EQ near-black background (subtle vertical shade)
		float tt = (float)y / (float)VH;
		HBRUSH ln = CreateSolidBrush(RGB((int)(30 - 8 * tt), (int)(33 - 9 * tt), (int)(41 - 11 * tt)));
		RECT lr = { 0, y, VW, y + 1 }; FillRect(mem, &lr, ln); DeleteObject(ln);
	}
	RECT full = { 0, 0, VW, VH };
	HBRUSH bo = CreateSolidBrush(RGB(6, 7, 10)); FrameRect(mem, &full, bo); DeleteObject(bo);   // hard outer edge
	// EQ gold/tan metallic window frame (dark -> bright -> mid gold band)
	{ RECT g1 = { 1, 1, VW - 1, VH - 1 }; HBRUSH b1 = CreateSolidBrush(RGB(86, 68, 38));  FrameRect(mem, &g1, b1); DeleteObject(b1);
	  RECT g2 = { 2, 2, VW - 2, VH - 2 }; HBRUSH b2 = CreateSolidBrush(RGB(184, 154, 94)); FrameRect(mem, &g2, b2); DeleteObject(b2);
	  RECT g3 = { 3, 3, VW - 3, VH - 3 }; HBRUSH b3 = CreateSolidBrush(RGB(120, 98, 56));  FrameRect(mem, &g3, b3); DeleteObject(b3); }

	HFONT font = CreateFontA(-15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT sfont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldF = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	// EQ title bar: embossed strip with the centered window name + a gold underline above the tabs
	{ RECT tb = { 4, 4, VW - 4, VTBAR_H + 2 };
	  HBRUSH tbg = CreateSolidBrush(RGB(40, 43, 52)); FillRect(mem, &tb, tbg); DeleteObject(tbg);
	  VBevel(mem, tb, true);
	  RECT tul = { 4, VTBAR_H + 1, VW - 4, VTBAR_H + 2 }; HBRUSH gl = CreateSolidBrush(RGB(120, 98, 56)); FillRect(mem, &tul, gl); DeleteObject(gl);
	  SelectObject(mem, font);
	  DrawShadow(mem, "Trader", tb, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(228, 210, 142), RGB(8, 9, 12)); }

	// tab buttons
	RECT t0, t1; VTabBtn(0, t0); VTabBtn(1, t1);
	VDrawBtn(mem, sfont, t0, "Add Items", g_vendorTab == 0);
	VDrawBtn(mem, sfont, t1, "My Shop",   g_vendorTab == 1);
	{ RECT vx; CloseXRect(VW, vx); DrawCloseX(mem, vx); }

	// title / hint strip (draggable)
	RECT tr = { 14, VTITLE_TOP + 1, VW - 100, VTITLE_TOP + VTITLE_H };
	DrawShadow(mem, g_vendorTab == 0 ? "Price each item, then Add Priced Items"
	                                 : "Click Pull to take an item back to your cursor",
		tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(226, 208, 140), RGB(8, 9, 12));
	if (VCount() > VVIS) { RECT ur, dn; VUpBtn(ur); VDnBtn(dn); VDrawBtn(mem, sfont, ur, "Up", false); VDrawBtn(mem, sfont, dn, "Dn", false); }
	int hy = VTBAR_H + VTAB_H + VTITLE_H;

	// EQ recessed content area: a sunken panel behind the header + rows, then a raised header strip
	{ RECT lp = { 8, hy, VW - 8, VLIST_Y + VVIS * VROW_H + 2 };
	  HBRUSH lb = CreateSolidBrush(RGB(14, 15, 20)); FillRect(mem, &lp, lb); DeleteObject(lb);
	  VBevel(mem, lp, false); }
	{ RECT hs = { 9, hy + 1, VW - 9, hy + VHDR_H }; HBRUSH hb = CreateSolidBrush(RGB(48, 52, 62));
	  FillRect(mem, &hs, hb); DeleteObject(hb); VBevel(mem, hs, true); }

	// column headers
	SelectObject(mem, sfont);
	RECT ih = { 18, hy + 1, 240, hy + VHDR_H }; DrawShadow(mem, "Item", ih, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(196, 202, 214), RGB(8, 9, 12));
	if (g_vendorTab == 0) {
		for (int d = 0; d < 4; ++d) { int l, rt; VFieldX(d, l, rt); RECT hh = { l, hy + 1, rt, hy + VHDR_H };
			DrawShadow(mem, V_LBL[d], hh, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(196, 202, 214), RGB(8, 9, 12)); }
	} else {
		RECT ph = { VW - 250, hy + 1, VW - 90, hy + VHDR_H };
		DrawShadow(mem, "Price", ph, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, RGB(196, 202, 214), RGB(8, 9, 12));
	}

	int cnt = VCount(), scroll = VScroll();
	for (int vis = 0; vis < VVIS; ++vis) {
		int i = scroll + vis; if (i >= cnt) break;
		int y = VRowY(vis);
		if (vis & 1) { RECT rw = { 10, y, VW - 10, y + VROW_H }; HBRUSH rb = CreateSolidBrush(RGB(30, 33, 41)); FillRect(mem, &rw, rb); DeleteObject(rb); }  // alt-row band
		if (g_vendorTab == 0) {
			RECT nr = { 18, y + 3, VW - 224, y + VROW_H - 3 };
			SelectObject(mem, sfont);
			DrawShadow(mem, g_vendorNames[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, RGB(223, 219, 205), RGB(8, 9, 12));
			for (int d = 0; d < 4; ++d) {
				RECT fr; VField(vis, d, fr);
				bool focused = (g_vFocusItem == i && g_vFocusCoin == d);
				HBRUSH fb = CreateSolidBrush(focused ? RGB(40, 54, 40) : RGB(18, 20, 26)); FillRect(mem, &fr, fb); DeleteObject(fb);
				VBevel(mem, fr, false);   // sunken EQ input field
				char nb[16]; sprintf_s(nb, focused ? "%d_" : "%d", g_vCoin[i][d]);
				DrawShadow(mem, nb, fr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, focused ? RGB(212, 240, 182) : RGB(240, 224, 150), RGB(8, 9, 12));
			}
		} else {
			RECT nr = { 18, y + 3, VW - 260, y + VROW_H - 3 };
			SelectObject(mem, sfont);
			DrawShadow(mem, g_myName[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, RGB(223, 219, 205), RGB(8, 9, 12));
			unsigned c = g_myCost[i];
			char pbuf[48]; sprintf_s(pbuf, "%up %ug %us %uc", c / 1000, (c / 100) % 10, (c / 10) % 10, c % 10);
			RECT pr = { VW - 250, y + 3, VW - 90, y + VROW_H - 3 };
			DrawShadow(mem, pbuf, pr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, RGB(240, 224, 150), RGB(8, 9, 12));
			RECT pull; VPullBtn(vis, pull); VDrawBtn(mem, sfont, pull, "Pull", false);
		}
	}
	if (cnt == 0) {
		RECT er = { 16, VLIST_Y + 8, VW - 16, VLIST_Y + 40 };
		SelectObject(mem, sfont);
		DrawShadow(mem, g_vendorTab == 0 ? "No sellable items in your bags (No-Drop/containers can't list)."
		                                 : "Your shop is empty -- add items on the Add Items tab.",
			er, DT_LEFT | DT_TOP, RGB(150, 156, 172), RGB(8, 9, 12));
	}

	RECT ab, rb; VActBtn(ab); VRefBtn(rb);
	if (g_vendorTab == 0) VDrawBtn(mem, font, ab, "Add Priced Items", true);
	VDrawBtn(mem, font, rb, "Refresh", false);

	BitBlt(hdc, 0, 0, VW, VH, mem, 0, 0, SRCCOPY);
	SelectObject(mem, oldF); DeleteObject(font); DeleteObject(sfont);
	SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

// Add every item that has a non-zero price to the shop, then refresh both tabs.
// Escrow items into the shop. The price comes from the server-side price book (set on the Set Price tab),
// so we send only slot:qty. only_row >= 0 lists just that inventory row (the List Item flow); -1 = all priced.
static void VendorDoAdd(int only_row = -1)
{
	std::string line; int n = 0, total = 0;
	for (int i = 0; i < g_vendorCount; ++i) {
		if (only_row >= 0 && i != only_row) continue;
		if (g_vBookPrice[i] == 0) continue;                // unpriced -- set a price first (server rejects it too)
		int qty = g_vListQty[i] > 0 ? g_vListQty[i] : g_vendorStack[i];
		char seg[32]; sprintf_s(seg, "%d:%d", g_vendorIds[i], qty);   // slot:qty
		if (!line.empty()) line += ",";
		line += seg;
		++total;
		if (++n >= 10) { std::string c = "/say shopadd " + line; VendorQueue(c.c_str()); line.clear(); n = 0; }
	}
	if (!line.empty()) { std::string c = "/say shopadd " + line; VendorQueue(c.c_str()); }
	if (total > 0) VendorQueue("/say shoprefresh");         // server re-sends the shop lines
}

// ================================================================= Native SIDL shop window (ShopWnd)
// A real EQ-skinned window (EQUI_ShopWnd.xml) that REUSES the GDI shop's backend: same g_vendor*/g_my*
// state (filled by SHOPINVDATA/MYSHOPDATA) + VendorQueue/VendorDoAdd. It's our own CCustomWnd (like the
// achievement window) so it renders in ANY zone -- no Bazaar dependency. Opened when SHOPINVDATA arrives
// (/shop -> /say shopopen -> server). Torn down on CleanGameUI/ReloadUI via ShopWndOnUiReset (called from
// the achievement UI-reset detour, which owns those hooks).
static class ShopWnd* gShopWnd = nullptr;

static void ShopSetLabel(CXWnd* w, const char* s) { if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); } }   // labels (method, reliable)
static int  ShopGetInt (CXWnd* w) {
	if (!w) return 0;
	// Read via the METHOD GetWindowTextA (address-mapped @ 0x411190, reliable) -- NOT raw member offsets
	// like ->InputText/->WindowText, whose struct offsets don't match this client build (they crash / read
	// garbage). CXStr internally wraps one CXSTR* (ref-counted string); read its Text with GetCXStr.
	CXStr s = w->GetWindowTextA();
	PCXSTR raw = *(PCXSTR*)&s;
	char b[32] = { 0 };
	if (raw) GetCXStr(raw, b, sizeof(b));
	return atoi(b);
}

// "2p 0g 0s 0c" from a copper amount.
static void ShopCoinStr(unsigned c, char* b, int n) {
	sprintf_s(b, n, "%up %ug %us %uc", c / 1000, (c / 100) % 10, (c / 10) % 10, c % 10);
}
// A relative "when" string for the price-history rows (unix time -> "3h ago").
static void ShopWhenStr(long when, char* b, int n) {
	long d = (long)time(nullptr) - when;
	if (when <= 0)      { strncpy_s(b, n, "-", _TRUNCATE); }
	else if (d < 60)    { strncpy_s(b, n, "just now", _TRUNCATE); }
	else if (d < 3600)  { sprintf_s(b, n, "%ldm ago", d / 60); }
	else if (d < 86400) { sprintf_s(b, n, "%ldh ago", d / 3600); }
	else                { sprintf_s(b, n, "%ldd ago", d / 86400); }
}

// Tab ids for the native shop window.
enum { SHOP_TAB_PRICE = 0, SHOP_TAB_LIST = 1, SHOP_TAB_SHOP = 2 };

class ShopWnd : public CCustomWnd
{
public:
	CListWnd*   ItemList = nullptr;
	CListWnd*   HistList = nullptr;
	CXWnd*      Hint = nullptr;
	CXWnd*      PriceLabel = nullptr; CXWnd* HistLabel = nullptr; CXWnd* QtyLabel = nullptr;
	CXWnd*      LblP = nullptr; CXWnd* LblG = nullptr; CXWnd* LblS = nullptr; CXWnd* LblC = nullptr;
	CXWnd*      PricePlat = nullptr; CXWnd* PriceGold = nullptr; CXWnd* PriceSilv = nullptr; CXWnd* PriceCopp = nullptr;
	CXWnd*      QtyEdit = nullptr;
	CButtonWnd* SetPriceBtn = nullptr; CButtonWnd* AddBtn = nullptr; CButtonWnd* PullBtn = nullptr;
	CButtonWnd* RefreshBtn = nullptr; CButtonWnd* TabPriceBtn = nullptr; CButtonWnd* TabListBtn = nullptr; CButtonWnd* TabShopBtn = nullptr;

	// The list's live selection is cleared the moment a coin field takes keyboard focus, so remember the
	// row the user last clicked and use THAT for Set Price / Add / Pull.
	int m_sel = -1;
	static bool InvTab() { return g_vendorTab != SHOP_TAB_SHOP; }   // tabs 0/1 browse inventory; 2 = listings
	int SelRow() {
		int s = (m_sel >= 0) ? m_sel : (ItemList ? ItemList->GetCurSel() : -1);
		int cnt = InvTab() ? g_vendorCount : g_myCount;
		if (s < 0 && cnt == 1) s = 0;                       // single item -> no need to click it first
		return (s >= 0 && s < cnt) ? s : -1;
	}
	static void SetVis(CXWnd* w, bool v) { if (w) w->Show(v, true); }

	ShopWnd() : CCustomWnd((char*)"ShopWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(ShopWnd);
		ItemList    = (CListWnd*)GetChildItem("SHW_ItemList");
		HistList    = (CListWnd*)GetChildItem("SHW_HistList");
		Hint        = GetChildItem("SHW_Hint");
		PriceLabel  = GetChildItem("SHW_PriceLabel");
		HistLabel   = GetChildItem("SHW_HistLabel");
		QtyLabel    = GetChildItem("SHW_QtyLabel");
		LblP        = GetChildItem("SHW_LblP");   LblG = GetChildItem("SHW_LblG");
		LblS        = GetChildItem("SHW_LblS");   LblC = GetChildItem("SHW_LblC");
		PricePlat   = GetChildItem("SHW_PricePlat");
		PriceGold   = GetChildItem("SHW_PriceGold");
		PriceSilv   = GetChildItem("SHW_PriceSilv");
		PriceCopp   = GetChildItem("SHW_PriceCopp");
		QtyEdit     = GetChildItem("SHW_Qty");
		SetPriceBtn = (CButtonWnd*)GetChildItem("SHW_SetPrice");
		AddBtn      = (CButtonWnd*)GetChildItem("SHW_Add");
		PullBtn     = (CButtonWnd*)GetChildItem("SHW_Pull");
		RefreshBtn  = (CButtonWnd*)GetChildItem("SHW_Refresh");
		TabPriceBtn = (CButtonWnd*)GetChildItem("SHW_TabPrice");
		TabListBtn  = (CButtonWnd*)GetChildItem("SHW_TabList");
		TabShopBtn  = (CButtonWnd*)GetChildItem("SHW_TabShop");
		Refresh();
	}

	// Show only the controls that belong to the active tab.
	void ApplyVis()
	{
		bool price = (g_vendorTab == SHOP_TAB_PRICE);
		bool list  = (g_vendorTab == SHOP_TAB_LIST);
		bool shop  = (g_vendorTab == SHOP_TAB_SHOP);
		SetVis(PriceLabel, price); SetVis((CXWnd*)PricePlat, price); SetVis((CXWnd*)PriceGold, price);
		SetVis((CXWnd*)PriceSilv, price); SetVis((CXWnd*)PriceCopp, price);
		SetVis(LblP, price); SetVis(LblG, price); SetVis(LblS, price); SetVis(LblC, price);
		SetVis((CXWnd*)SetPriceBtn, price);
		// ⚠️ Visible on ALL THREE tabs now. It used to show only on Set Price; the other two tabs each
		// have their own history to put in it (listed items, sold items), so hiding it there would leave
		// the panel empty for two thirds of the window's life.
		SetVis(HistLabel, true); SetVis((CXWnd*)HistList, true);
		SetVis(QtyLabel, list); SetVis((CXWnd*)QtyEdit, list); SetVis((CXWnd*)AddBtn, list);
		SetVis((CXWnd*)PullBtn, shop);
	}

	void FillMainList()
	{
		if (!ItemList) return;
		ItemList->DeleteAll();
		const int cnt = InvTab() ? g_vendorCount : g_myCount;
		for (int i = 0; i < cnt; ++i) {
			const char* nm = InvTab() ? g_vendorNames[i] : g_myName[i];
			CXStr name(nm);
			int row = ItemList->AddString(name, 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
			// Qty column: Set Price -> how many you hold; List -> how many to list; My Shop -> listed qty.
			int qv = (g_vendorTab == SHOP_TAB_PRICE) ? g_vendorStack[i]
			       : (g_vendorTab == SHOP_TAB_LIST)  ? g_vListQty[i]
			       :                                   g_myQty[i];
			char qb[16]; sprintf_s(qb, "%d", qv > 0 ? qv : 1);
			CXStr qty(qb); ItemList->SetItemText(row, 1, &qty);
			// Price column: inventory tabs show the price-book price; My Shop shows the listed cost.
			char pb[48];
			if (InvTab()) {
				if (g_vBookPrice[i] == 0) strncpy_s(pb, sizeof(pb), "-- set price --", _TRUNCATE);
				else ShopCoinStr(g_vBookPrice[i], pb, sizeof(pb));
			} else {
				ShopCoinStr(g_myCost[i], pb, sizeof(pb));
			}
			CXStr pr(pb); ItemList->SetItemText(row, 2, &pr);
		}
		if (m_sel >= 0 && m_sel < cnt) ItemList->SetCurSel(m_sel);   // restore highlight after the rebuild
	}

	// Price history for the selected inventory item only (Set Price tab).
	// The ONE history panel serves all three tabs; what it means follows the tab.
	//
	//   Set Price : price changes for the SELECTED item   (when | old | new)
	//   List Item : everything you have listed             (when | item | qty x price)
	//   My Shop   : everything that has sold               (when | item | price to buyer)
	//
	// ⚠️ Only the Set Price view filters by selection. The other two are histories of the whole shop,
	// so filtering them to a highlighted row would hide almost everything.
	void FillHistory()
	{
		if (!HistList) return;
		HistList->DeleteAll();

		if (g_vendorTab == SHOP_TAB_PRICE) {
			int sel = SelRow();
			int item_id = (sel >= 0) ? g_vItemId[sel] : 0;
			if (item_id == 0) return;
			for (int i = 0; i < g_logCount; ++i) {
				if (g_logItemId[i] != item_id) continue;
				char wb[24]; ShopWhenStr(g_logWhen[i], wb, sizeof(wb));
				CXStr when(wb);
				int row = HistList->AddString(when, 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
				char ob[48]; ShopCoinStr(g_logOld[i], ob, sizeof(ob)); CXStr oldp(ob); HistList->SetItemText(row, 1, &oldp);
				char nb[48]; ShopCoinStr(g_logNew[i], nb, sizeof(nb)); CXStr newp(nb); HistList->SetItemText(row, 2, &newp);
			}
			return;
		}

		if (g_vendorTab == SHOP_TAB_LIST) {
			for (int i = 0; i < g_listCount; ++i) {
				char wb[24]; ShopWhenStr(g_listWhen[i], wb, sizeof(wb));
				CXStr when(wb);
				int row = HistList->AddString(when, 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
				CXStr nm(g_listName[i]); HistList->SetItemText(row, 1, &nm);
				char pb[64]; ShopCoinStr(g_listPrice[i], pb, sizeof(pb));
				char cell[96];
				if (g_listQty[i] > 1) { sprintf_s(cell, "%d x %s", g_listQty[i], pb); }
				else                  { sprintf_s(cell, "%s", pb); }
				CXStr q(cell); HistList->SetItemText(row, 2, &q);
			}
			return;
		}

		// SHOP_TAB_SHOP -- what sold, and to whom
		for (int i = 0; i < g_soldCount; ++i) {
			char wb[24]; ShopWhenStr(g_soldWhen[i], wb, sizeof(wb));
			CXStr when(wb);
			int row = HistList->AddString(when, 0xFF9CE09C, (uint32_t)i, nullptr, nullptr);   // green: coin in
			char nmb[96];
			if (g_soldQty[i] > 1) { sprintf_s(nmb, "%d x %s", g_soldQty[i], g_soldName[i]); }
			else                  { sprintf_s(nmb, "%s", g_soldName[i]); }
			CXStr nm(nmb); HistList->SetItemText(row, 1, &nm);
			char pb[64]; ShopCoinStr(g_soldPrice[i], pb, sizeof(pb));
			char cell[128]; sprintf_s(cell, "%s to %s", pb, g_soldBuyer[i]);
			CXStr q(cell); HistList->SetItemText(row, 2, &q);
		}
	}

	void Refresh()
	{
		const char* hint =
			(g_vendorTab == SHOP_TAB_PRICE) ? "Select an item, type a price (p/g/s/c), then Set Price. History is below." :
			(g_vendorTab == SHOP_TAB_LIST)  ? "Select a priced item, set the quantity, then Add to Shop." :
			                                  "Select a listing and Pull it back to your cursor.";
		ShopSetLabel(Hint, hint);
		ShopSetLabel(HistLabel,
			(g_vendorTab == SHOP_TAB_PRICE) ? "Price history for the selected item" :
			(g_vendorTab == SHOP_TAB_LIST)  ? "Items you have listed"               :
			                                  "Items sold from your shop");
		ApplyVis();
		FillMainList();
		FillHistory();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) { pXWnd()->Show(0, 1); return 1; }
		if (Message == XWM_LCLICK) {
			// clicking an entry field must give it keyboard focus, or typed digits go nowhere. Then let the
			// editbox handle the click normally (caret placement).
			if (pWnd == PricePlat || pWnd == PriceGold || pWnd == PriceSilv || pWnd == PriceCopp || pWnd == QtyEdit) {
				pWnd->SetFocus();
				return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
			}
			if (pWnd == (CXWnd*)TabPriceBtn) { g_vendorTab = SHOP_TAB_PRICE; m_sel = -1; Refresh(); return 1; }
			if (pWnd == (CXWnd*)TabListBtn)  { g_vendorTab = SHOP_TAB_LIST;  m_sel = -1; Refresh(); return 1; }
			if (pWnd == (CXWnd*)TabShopBtn)  { g_vendorTab = SHOP_TAB_SHOP;  m_sel = -1; Refresh(); return 1; }
			if (pWnd == (CXWnd*)RefreshBtn)  { VendorQueue("/say shoprefresh"); return 1; }
			if (pWnd == (CXWnd*)ItemList) {                       // remember the clicked row; refresh history.
				m_sel = ItemList->GetCurSel();                   // Do NOT pre-fill the coin fields: SetCXStr on
				FillHistory();                                   // InputText detaches the editbox's live edit
				return 1;                                        // buffer, so typed values become unreadable.
			}
			if (pWnd == (CXWnd*)SetPriceBtn) {
				int sel = SelRow();                              // remembered row (live selection is gone)
				if (g_vendorTab == SHOP_TAB_PRICE && sel >= 0) {
					unsigned copper = (unsigned)ShopGetInt(PricePlat) * 1000 + (unsigned)ShopGetInt(PriceGold) * 100
					                + (unsigned)ShopGetInt(PriceSilv) * 10  + (unsigned)ShopGetInt(PriceCopp);
					if (copper == 0) {
						ShopSetLabel(Hint, "Enter a price (p/g/s/c) above before Set Price.");
					} else {
						char c[64]; sprintf_s(c, "/say shopsetprice %d:%u", g_vItemId[sel], copper);
						VendorQueue(c);
						VendorQueue("/say shoprefresh");         // server persists it + re-sends inv/book/log
					}
				}
				return 1;
			}
			if (pWnd == (CXWnd*)AddBtn) {
				int sel = SelRow();
				if (g_vendorTab == SHOP_TAB_LIST && sel >= 0) {
					int q = ShopGetInt(QtyEdit);                 // 0/blank = whole stack
					if (q >= 1) g_vListQty[sel] = (q > g_vendorStack[sel]) ? g_vendorStack[sel] : q;
					if (g_vBookPrice[sel] == 0) ShopSetLabel(Hint, "Set a price on the Set Price tab before listing this item.");
					else VendorDoAdd(sel);
				}
				return 1;
			}
			if (pWnd == (CXWnd*)PullBtn) {
				int sel = SelRow();
				if (g_vendorTab == SHOP_TAB_SHOP && sel >= 0) {
					char c[48]; sprintf_s(c, "/say shoppull %d", g_mySn[sel]);
					VendorQueue(c); VendorQueue("/say shoprefresh");
				}
				return 1;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

static void EnsureShopWindow(bool show)
{
	// Our dll historically never used native SIDL windows, so wire the UI managers from the client
	// globals if unset (same fix the achievement window needs, §15).
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) return;
	if (!gShopWnd) gShopWnd = new ShopWnd();
	if (gShopWnd && show) { gShopWnd->Refresh(); gShopWnd->pXWnd()->Show(1, 1); }
}

void ShopSidlShow()         { EnsureShopWindow(true); }
void ShopWndRefreshIfOpen() { if (gShopWnd) gShopWnd->Refresh(); }
void ShopWndOnUiReset()     { if (gShopWnd) { delete gShopWnd; gShopWnd = nullptr; } }


static LRESULT CALLBACK VendorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: VendorPaint(hwnd); return 0;
	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		SetFocus(hwnd);                                  // take keyboard focus so typing reaches us
		{ RECT vx; CloseXRect(VW, vx); if (PtInRect(&vx, pt)) { g_vendorVisible = false; ShowWindow(hwnd, SW_HIDE); return 0; } }
		RECT t0, t1; VTabBtn(0, t0); VTabBtn(1, t1);
		if (PtInRect(&t0, pt)) { g_vendorTab = 0; g_vFocusItem = -1; VendorQueue("/say shoprefresh"); InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		if (PtInRect(&t1, pt)) { g_vendorTab = 1; g_vFocusItem = -1; VendorQueue("/say shoprefresh"); InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		int cnt = VCount(); int& scroll = VScroll();
		int mx = cnt - VVIS; if (mx < 0) mx = 0;
		if (cnt > VVIS) {
			RECT ur, dn; VUpBtn(ur); VDnBtn(dn);
			if (PtInRect(&ur, pt)) { scroll -= VVIS - 1; if (scroll < 0) scroll = 0; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
			if (PtInRect(&dn, pt)) { scroll += VVIS - 1; if (scroll > mx) scroll = mx; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		}
		if (g_vendorTab == 0) {
			for (int vis = 0; vis < VVIS; ++vis) {
				int i = scroll + vis; if (i >= cnt) break;
				for (int d = 0; d < 4; ++d) { RECT fr; VField(vis, d, fr);
					if (PtInRect(&fr, pt)) { g_vFocusItem = i; g_vFocusCoin = d; InvalidateRect(hwnd, nullptr, FALSE); return 0; } }
			}
			RECT ab; VActBtn(ab);
			if (PtInRect(&ab, pt)) { VendorDoAdd(); return 0; }
		} else {
			for (int vis = 0; vis < VVIS; ++vis) {
				int i = scroll + vis; if (i >= cnt) break;
				RECT pull; VPullBtn(vis, pull);
				if (PtInRect(&pull, pt)) { char c[48]; sprintf_s(c, "/say shoppull %d", g_mySn[i]); VendorQueue(c); VendorQueue("/say shoprefresh"); return 0; }
			}
		}
		RECT rb; VRefBtn(rb);
		if (PtInRect(&rb, pt)) { VendorQueue("/say shoprefresh"); return 0; }
		g_vFocusItem = -1;                               // click elsewhere -> drop focus
		// drag from anywhere across the top strip (title bar + tab bar + hint) -- tabs/[X]/buttons were
		// checked and returned above, so reaching here means empty top space.
		if (pt.y < VTBAR_H + VTAB_H + VTITLE_H) { g_vDragging = true; g_vDragDX = pt.x; g_vDragDY = pt.y; SetCapture(hwnd); }
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}
	case WM_CHAR: {
		if (g_vendorTab == 0 && g_vFocusItem >= 0 && g_vFocusItem < g_vendorCount) {
			int* f = &g_vCoin[g_vFocusItem][g_vFocusCoin];
			if (wp >= '0' && wp <= '9') {
				int maxv = (g_vFocusCoin == 0) ? 999999 : 99999;     // plat large; gold/silver/copper smaller
				int nv = (*f) * 10 + (int)(wp - '0');
				if (nv <= maxv) { *f = nv; InvalidateRect(hwnd, nullptr, FALSE); }
			} else if (wp == 8) { *f /= 10; InvalidateRect(hwnd, nullptr, FALSE); }   // backspace
		}
		return 0;
	}
	case WM_KEYDOWN:
		if (wp == VK_TAB && g_vendorTab == 0 && g_vFocusItem >= 0) {   // Tab -> next coin field
			if (++g_vFocusCoin > 3) { g_vFocusCoin = 0; if (g_vFocusItem + 1 < g_vendorCount) ++g_vFocusItem; }
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;
	case WM_MOUSEMOVE:
		if (g_vDragging) { POINT c; GetCursorPos(&c); g_vPosX = c.x - g_vDragDX; g_vPosY = c.y - g_vDragDY; g_vPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_vPosX, g_vPosY, VW, VH, SWP_NOACTIVATE); }
		return 0;
	case WM_LBUTTONUP: if (g_vDragging) { g_vDragging = false; ReleaseCapture(); } return 0;
	case WM_RBUTTONUP: g_vendorVisible = false; ShowWindow(hwnd, SW_HIDE); return 0;
	case WM_MOUSEWHEEL: {
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);  // grab focus so EQ stops zooming
		int d = GET_WHEEL_DELTA_WPARAM(wp); int& scroll = VScroll(); scroll += (d > 0) ? -1 : 1;
		int mx = VCount() - VVIS; if (mx < 0) mx = 0;
		if (scroll < 0) scroll = 0; if (scroll > mx) scroll = mx;
		InvalidateRect(hwnd, nullptr, FALSE); return 0;
	}
	case WM_TIMER: {
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_vendorVisible && active) {
			if (!g_vPositioned && eq) { RECT cr; GetClientRect(eq, &cr); POINT tl = { cr.left, cr.top }; ClientToScreen(eq, &tl);
				g_vPosX = tl.x + (cr.right - cr.left - VW) / 2; g_vPosY = tl.y + (cr.bottom - cr.top - VH) / 2; g_vPositioned = true; }
			if (g_vPositioned) {
				if (!IsWindowVisible(hwnd)) { SetWindowPos(hwnd, HWND_TOPMOST, g_vPosX, g_vPosY, VW, VH, SWP_NOACTIVATE); ShowWindow(hwnd, SW_SHOWNOACTIVATE); }
				InvalidateRect(hwnd, nullptr, FALSE);
			}
		} else if (IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_HIDE);
		return 0;
	}
	case WM_DESTROY: return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI VendorThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);
	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc = VendorWndProc; wc.hInstance = hInst; wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTVendorWnd"; RegisterClassExA(&wc);
	g_vendorHwnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "AoTVendorWnd", "", WS_POPUP,
		0, 0, VW, VH, nullptr, nullptr, hInst, nullptr);
	if (!g_vendorHwnd) return 0;
	SetLayeredWindowAttributes(g_vendorHwnd, 0, 244, LWA_ALPHA);
	SetTimer(g_vendorHwnd, 1, 60, nullptr);
	MSG m; while (GetMessageA(&m, nullptr, 0, 0) > 0) { TranslateMessage(&m); DispatchMessageA(&m); }
	return 0;
}

// ================================ AoTv4 in-game search window ("allaclone") ================================
// Green-slate theme (distinct from the shop's navy). Reuses VDrawBtn/DrawShadow/CloseXRect/DrawCloseX/
// VendorQueue (all defined above). State + HandleSearchChat are declared much earlier (before the chat
// detour). Opened by "/search" (see core_bazaar.h -> ShowSearchWindow).
// Resizable window (like the in-game Buyer/Barter window): SW/SWH are the CURRENT client size and
// change as the user drags the bottom-right grip. Layout is derived live -- the detail panel takes the
// bottom ~38%, the result list fills the middle, so both grow when you enlarge the window.
static int SW = 720, SWH = 600;
static const int SWTITLE_H = 22, SWTAB_H = 26, SWSEARCH_H = 30, SWROW_H = 22;
static const int SW_MINW = 460, SW_MINH = 380;
static const int SWTABS_Y = SWTITLE_H;
static const int SWSRCH_Y = SWTITLE_H + SWTAB_H;
static const int SWLIST_Y = SWTITLE_H + SWTAB_H + SWSEARCH_H;

static int  SWDetH()   { int h = SWH * 38 / 100; return h < 120 ? 120 : h; }                 // detail-panel height
static int  SWDetTop() { return SWH - SWDetH(); }
static int  SWVISc()   { int v = (SWDetTop() - 4 - SWLIST_Y) / SWROW_H; return v < 1 ? 1 : v; }  // visible result rows
static int  SWDetVis() { int v = (SWDetH() - 14) / 16; return v < 1 ? 1 : v; }                // visible detail lines

static void SWTabR(int t, RECT& r) { int w = (SW - 20) / 4; r.left = 10 + t * w; r.right = r.left + w - 4; r.top = SWTABS_Y + 3; r.bottom = SWTABS_Y + SWTAB_H - 3; }
static void SWBoxR(RECT& r) { r.left = 14; r.right = SW - 108; r.top = SWSRCH_Y + 4; r.bottom = r.top + 22; }
static void SWBtnR(RECT& r) { r.right = SW - 14; r.left = r.right - 88; r.top = SWSRCH_Y + 4; r.bottom = r.top + 22; }
static int  SWRowY(int vis) { return SWLIST_Y + vis * SWROW_H; }
static void SWDetR(RECT& r) { r.left = 6; r.right = SW - 6; r.top = SWDetTop(); r.bottom = SWH - 6; }

static void SearchDoSearch()
{
	if (g_srchTerm[0] == 0) return;
	char c[160]; sprintf_s(c, "/say srch %s %s", SRCH_KIND[g_srchTab], g_srchTerm);
	VendorQueue(c);   // server replies SRCHDATA -> HandleSearchChat fills the list
}

static void SearchPaint(HWND hwnd)
{
	PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, SW, SWH);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < SWH; ++y) {                          // green-slate gradient
		float tt = (float)y / (float)SWH;
		HBRUSH ln = CreateSolidBrush(RGB((int)(18 + 8 * tt), (int)(30 + 14 * tt), (int)(26 + 12 * tt)));
		RECT lr = { 0, y, SW, y + 1 }; FillRect(mem, &lr, ln); DeleteObject(ln);
	}
	RECT full = { 0, 0, SW, SWH };
	HBRUSH bo = CreateSolidBrush(RGB(8, 16, 12));   FrameRect(mem, &full, bo); DeleteObject(bo);
	HBRUSH bi = CreateSolidBrush(RGB(90, 150, 110)); RECT inr = { 2, 2, SW - 2, SWH - 2 }; FrameRect(mem, &inr, bi); DeleteObject(bi);

	HFONT font  = CreateFontA(-15, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT sfont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldF = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	// title (draggable) + [X]
	RECT tr = { 14, 1, SW - 30, SWTITLE_H };
	DrawShadow(mem, "Search", tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(200, 235, 200), RGB(8, 16, 12));
	{ RECT vx; CloseXRect(SW, vx); DrawCloseX(mem, vx); }

	// tabs
	static const char* TAB_LBL[4] = { "Items", "Mobs", "Spells", "Tradeskills" };
	for (int t = 0; t < 4; ++t) { RECT tb; SWTabR(t, tb); VDrawBtn(mem, sfont, tb, TAB_LBL[t], g_srchTab == t); }

	// search box + Search button
	RECT box; SWBoxR(box);
	HBRUSH bb = CreateSolidBrush(g_srchFocus ? RGB(30, 50, 36) : RGB(22, 34, 28)); FillRect(mem, &box, bb); DeleteObject(bb);
	HBRUSH be = CreateSolidBrush(g_srchFocus ? RGB(140, 200, 150) : RGB(70, 110, 84)); FrameRect(mem, &box, be); DeleteObject(be);
	{
		char tb[80];
		if (g_srchTerm[0] == 0 && !g_srchFocus) strcpy_s(tb, "type a name...");
		else sprintf_s(tb, g_srchFocus ? "%s_" : "%s", g_srchTerm);
		RECT tr2 = { box.left + 6, box.top, box.right - 4, box.bottom };
		SelectObject(mem, sfont);
		DrawShadow(mem, tb, tr2, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
			g_srchTerm[0] ? RGB(240, 245, 235) : RGB(120, 150, 130), RGB(8, 16, 12));
	}
	RECT sb; SWBtnR(sb); VDrawBtn(mem, sfont, sb, "Search", true);

	// results (clamp scroll to the live visible-row count first)
	{ int mx = g_srchCount - SWVISc(); if (mx < 0) mx = 0; if (g_srchScroll > mx) g_srchScroll = mx; if (g_srchScroll < 0) g_srchScroll = 0; }
	SelectObject(mem, sfont);
	int vrows = SWVISc();
	for (int vis = 0; vis < vrows; ++vis) {
		int i = g_srchScroll + vis; if (i >= g_srchCount) break;
		int y = SWRowY(vis);
		if (g_srchIds[i] == g_srchDetailId) { RECT hl = { 6, y, SW - 6, y + SWROW_H }; HBRUSH hb = CreateSolidBrush(RGB(40, 64, 46)); FillRect(mem, &hl, hb); DeleteObject(hb); }
		RECT nr = { 14, y, SW - 14, y + SWROW_H };
		DrawShadow(mem, g_srchNames[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, RGB(225, 240, 225), RGB(8, 16, 12));
	}
	if (g_srchCount == 0) {
		RECT er = { 14, SWLIST_Y + 6, SW - 14, SWLIST_Y + 30 };
		DrawShadow(mem, "No results -- type a name and click Search (or press Enter).", er, DT_LEFT | DT_TOP, RGB(150, 175, 155), RGB(8, 16, 12));
	}

	// detail panel
	RECT det; SWDetR(det);
	HBRUSH pd = CreateSolidBrush(RGB(14, 24, 18)); FillRect(mem, &det, pd); DeleteObject(pd);
	HBRUSH pe = CreateSolidBrush(RGB(70, 110, 84)); FrameRect(mem, &det, pe); DeleteObject(pe);
	SelectObject(mem, sfont);
	if (g_srchDetailN == 0) {
		RECT dr = { det.left + 8, det.top + 6, det.right - 8, det.top + 24 };
		DrawShadow(mem, "Click a result to see details.", dr, DT_LEFT | DT_TOP, RGB(140, 165, 145), RGB(8, 16, 12));
	} else {
		int dvis = SWDetVis();
		{ int mx = g_srchDetailN - dvis; if (mx < 0) mx = 0; if (g_srchDetScroll > mx) g_srchDetScroll = mx; if (g_srchDetScroll < 0) g_srchDetScroll = 0; }
		for (int k = 0; k < dvis; ++k) {
			int i = g_srchDetScroll + k; if (i >= g_srchDetailN) break;
			RECT dr = { det.left + 8, det.top + 6 + k * 16, det.right - 8, det.top + 6 + k * 16 + 15 };
			DrawShadow(mem, g_srchDetail[i], dr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
				i == 0 ? RGB(245, 235, 180) : RGB(210, 225, 210), RGB(8, 16, 12));
		}
		if (g_srchDetailN > dvis) {                              // scroll hint (mouse-wheel over the panel)
			RECT mr = { det.left + 8, det.bottom - 15, det.right - 8, det.bottom - 2 };
			DrawShadow(mem, g_srchDetScroll + dvis < g_srchDetailN ? "-- scroll for more --" : "-- top: scroll up --",
				mr, DT_CENTER | DT_BOTTOM | DT_SINGLELINE, RGB(150, 180, 155), RGB(8, 16, 12));
		}
	}

	// resize grip (bottom-right) -- three diagonal ticks, matches the draggable corner
	{
		HPEN gp = CreatePen(PS_SOLID, 1, RGB(120, 170, 135)); HPEN opn = (HPEN)SelectObject(mem, gp);
		for (int i = 1; i <= 3; ++i) { int o = i * 4; MoveToEx(mem, SW - 3, SWH - 3 - o, nullptr); LineTo(mem, SW - 3 - o, SWH - 3); }
		SelectObject(mem, opn); DeleteObject(gp);
	}

	BitBlt(hdc, 0, 0, SW, SWH, mem, 0, 0, SRCCOPY);
	SelectObject(mem, oldF); DeleteObject(font); DeleteObject(sfont);
	SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK SearchWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: SearchPaint(hwnd); return 0;
	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		SetFocus(hwnd);                                       // take keyboard focus so typing reaches us
		{ RECT vx; CloseXRect(SW, vx); if (PtInRect(&vx, pt)) { g_searchVisible = false; ShowWindow(hwnd, SW_HIDE); return 0; } }
		for (int t = 0; t < 4; ++t) { RECT tb; SWTabR(t, tb);
			if (PtInRect(&tb, pt)) { g_srchTab = t; g_srchDetailId = -1; g_srchDetailN = 0; SearchDoSearch(); InvalidateRect(hwnd, nullptr, FALSE); return 0; } }
		{ RECT box; SWBoxR(box); if (PtInRect(&box, pt)) { g_srchFocus = true;  InvalidateRect(hwnd, nullptr, FALSE); return 0; } }
		{ RECT sbn; SWBtnR(sbn); if (PtInRect(&sbn, pt)) { g_srchFocus = false; SearchDoSearch(); return 0; } }
		for (int vis = 0, vr = SWVISc(); vis < vr; ++vis) {
			int i = g_srchScroll + vis; if (i >= g_srchCount) break;
			RECT rr = { 6, SWRowY(vis), SW - 6, SWRowY(vis) + SWROW_H };
			if (PtInRect(&rr, pt)) {
				g_srchDetailId = g_srchIds[i]; g_srchDetailN = 0; g_srchFocus = false;
				char c[80]; sprintf_s(c, "/say srchdet %s %d", SRCH_KIND[g_srchTab], g_srchIds[i]); VendorQueue(c);
				InvalidateRect(hwnd, nullptr, FALSE); return 0;
			}
		}
		g_srchFocus = false;                                  // click elsewhere -> drop text focus
		if (pt.y < SWTITLE_H) { g_sDragging = true; g_sDragDX = pt.x; g_sDragDY = pt.y; SetCapture(hwnd); }
		InvalidateRect(hwnd, nullptr, FALSE);
		return 0;
	}
	case WM_CHAR:
		if (g_srchFocus) {
			if (wp == 8) { int n = (int)strlen(g_srchTerm); if (n > 0) g_srchTerm[n - 1] = 0; InvalidateRect(hwnd, nullptr, FALSE); }        // backspace
			else if (wp == 13) SearchDoSearch();                                                                                             // Enter
			else if (wp >= 32 && wp < 127) { int n = (int)strlen(g_srchTerm); if (n < 62) { g_srchTerm[n] = (char)wp; g_srchTerm[n + 1] = 0; InvalidateRect(hwnd, nullptr, FALSE); } }
		}
		return 0;
	case WM_KEYDOWN: if (wp == VK_RETURN) SearchDoSearch(); return 0;
	case WM_MOUSEMOVE:
		if (g_sDragging) { POINT c; GetCursorPos(&c); g_sPosX = c.x - g_sDragDX; g_sPosY = c.y - g_sDragDY; g_sPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_sPosX, g_sPosY, SW, SWH, SWP_NOACTIVATE); }
		return 0;
	case WM_LBUTTONUP: if (g_sDragging) { g_sDragging = false; ReleaseCapture(); } return 0;
	// right-click no longer closes the window (use the [X] corner or /search to toggle)
	case WM_MOUSEWHEEL: {
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);
		POINT wpt = { (short)LOWORD(lp), (short)HIWORD(lp) }; ScreenToClient(hwnd, &wpt);
		int d = GET_WHEEL_DELTA_WPARAM(wp);
		RECT det; SWDetR(det);
		if (PtInRect(&det, wpt)) {                                // over the detail panel -> scroll detail
			g_srchDetScroll += (d > 0) ? -1 : 1;
			int mx = g_srchDetailN - SWDetVis(); if (mx < 0) mx = 0;
			if (g_srchDetScroll < 0) g_srchDetScroll = 0; if (g_srchDetScroll > mx) g_srchDetScroll = mx;
		} else {                                                  // otherwise scroll the result list
			g_srchScroll += (d > 0) ? -1 : 1;
			int mx = g_srchCount - SWVISc(); if (mx < 0) mx = 0;
			if (g_srchScroll < 0) g_srchScroll = 0; if (g_srchScroll > mx) g_srchScroll = mx;
		}
		InvalidateRect(hwnd, nullptr, FALSE); return 0;
	}
	case WM_NCHITTEST: {                                          // bottom-right grip -> drag to resize
		POINT gpt = { (short)LOWORD(lp), (short)HIWORD(lp) }; ScreenToClient(hwnd, &gpt);
		if (gpt.x >= SW - 16 && gpt.y >= SWH - 16) return HTBOTTOMRIGHT;
		return HTCLIENT;
	}
	case WM_GETMINMAXINFO: { MINMAXINFO* mmi = (MINMAXINFO*)lp; mmi->ptMinTrackSize.x = SW_MINW; mmi->ptMinTrackSize.y = SW_MINH; return 0; }
	case WM_SIZE: {
		SW = LOWORD(lp); SWH = HIWORD(lp);
		if (SW < SW_MINW) SW = SW_MINW; if (SWH < SW_MINH) SWH = SW_MINH;
		InvalidateRect(hwnd, nullptr, FALSE); return 0;
	}
	case WM_TIMER: {
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_searchVisible && active) {
			if (!g_sPositioned && eq) { RECT cr; GetClientRect(eq, &cr); POINT tl = { cr.left, cr.top }; ClientToScreen(eq, &tl);
				g_sPosX = tl.x + (cr.right - cr.left - SW) / 2; g_sPosY = tl.y + (cr.bottom - cr.top - SWH) / 2; g_sPositioned = true; }
			if (g_sPositioned) {
				if (!IsWindowVisible(hwnd)) { SetWindowPos(hwnd, HWND_TOPMOST, g_sPosX, g_sPosY, SW, SWH, SWP_NOACTIVATE); ShowWindow(hwnd, SW_SHOWNOACTIVATE); }
				InvalidateRect(hwnd, nullptr, FALSE);
			}
		} else if (IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_HIDE);
		return 0;
	}
	case WM_DESTROY: return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI SearchThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);
	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc = SearchWndProc; wc.hInstance = hInst; wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTSearchWnd"; RegisterClassExA(&wc);          // "AoT" prefix -> IsAoTOverlay treats it as ours
	g_searchHwnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "AoTSearchWnd", "", WS_POPUP,
		0, 0, SW, SWH, nullptr, nullptr, hInst, nullptr);
	if (!g_searchHwnd) return 0;
	SetLayeredWindowAttributes(g_searchHwnd, 0, 244, LWA_ALPHA);
	SetTimer(g_searchHwnd, 1, 60, nullptr);
	MSG m; while (GetMessageA(&m, nullptr, 0, 0) > 0) { TranslateMessage(&m); DispatchMessageA(&m); }
	return 0;
}

// ⚠️ THE GDI SEARCH OVERLAY IS SUPERSEDED by the native Allaclone window (core_allaclone.cpp), and
// these two are now nothing but forwarders. They keep their old names on purpose: core_bazaar.h and
// core_aotmenu.cpp both call them, and forwarding here means the switch is ONE place rather than a
// rename rippling through every caller.
// g_searchEnabled stays false, which is what makes the overlay's thread, paint and chat handler all
// unreachable without deleting them yet. Delete SearchPaint / SearchWndProc / SearchThreadProc /
// HandleSearchChat and their state once the native window is confirmed working.
void EnableSearchWindow() { InitAllaclone(); }               // wired in InitOptions()
void ShowSearchWindow()   { AllacloneShow(); }               // "/allaclone" intercept (core_bazaar.h)

// ----------------------------------------------------------------- bazaar vtable diagnostic
// Dump the trader (CBazaarWnd @ 0xD1FCB0) + buyer (CBarterWnd @ 0xF70CF0) window vtables to
// <eqdir>\aotv4_bazaar_vtable.txt so we can read the Activate/Show addresses without Ghidra.
// Triggered by Ctrl+B in ProcessGameEvents (the same reliable hook as the Ctrl+Q journal toggle).
static void DumpOneBazaarVtable(FILE* f, const char* label, DWORD pinst_static)
{
	void* pWnd = *(void**)((pinst_static - 0x400000) + baseAddress);
	fprintf(f, "==== %s  pinst=0x%06X  pWnd=%p ====\n", label, (unsigned)pinst_static, pWnd);
	if (!pWnd) { fprintf(f, "  (NULL -- window not created here; run this while standing IN the Bazaar)\n\n"); return; }
	void** vtbl = *(void***)pWnd;
	fprintf(f, "  vtable=%p\n", (void*)vtbl);
	for (int i = 0; i < 100; ++i) {
		DWORD fn = (DWORD)vtbl[i];
		DWORD st = fn ? (fn - (DWORD)baseAddress + 0x400000) : 0;   // rebase to image base 0x400000
		fprintf(f, "  [%3d] static 0x%06X\n", i, (unsigned)st);
	}
	fprintf(f, "\n");
}
// Walk __CommandList (0xACD5A8) = array of _CMDLIST{ strId, szName@+4, szLoc, fAddress@+12, ... }
// (28 bytes each) and log name -> handler static offset. In the Bazaar, "/trader" is registered, so
// its handler address shows up here -- that handler opens the trader window, callable from anywhere.
static void DumpCommandList(FILE* f)
{
	fprintf(f, "==== __CommandList (0xACD5A8)  [look for /trader /buyer /bazaar] ====\n");
	char* base = (char*)((0xACD5A8 - 0x400000) + baseAddress);
	DWORD lo = (DWORD)baseAddress, hi = (DWORD)baseAddress + 0x1600000;
	for (int i = 0; i < 5000; ++i) {
		char* e = base + i * 28;
		const char* name = *(const char**)(e + 4);              // szName
		if ((DWORD)name < lo || (DWORD)name >= hi) break;       // end of array / invalid
		DWORD fn = *(DWORD*)(e + 12);                            // fAddress (handler)
		DWORD st = fn ? (fn - (DWORD)baseAddress + 0x400000) : 0;
		fprintf(f, "  %-26s -> 0x%06X\n", name, (unsigned)st);
	}
	fprintf(f, "\n");
}

static void DumpBazaarVtables()
{
	// Try several writable locations (the EQ folder can be read-only under Program Files); first win.
	char exe[MAX_PATH] = {0}, eqpath[MAX_PATH] = {0}, temppath[MAX_PATH] = {0};
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	char* slash = strrchr(exe, '\\');
	if (slash) slash[1] = 0; else exe[0] = 0;
	sprintf_s(eqpath, "%saotv4_bazaar_vtable.txt", exe);
	char* tmp = getenv("TEMP");
	if (tmp) sprintf_s(temppath, "%s\\aotv4_bazaar_vtable.txt", tmp);

	const char* paths[3] = { eqpath, "aotv4_bazaar_vtable.txt", (tmp ? temppath : "") };
	FILE* f = nullptr;
	const char* used = nullptr;
	for (int i = 0; i < 3 && !f; ++i) {
		if (paths[i] && paths[i][0]) { fopen_s(&f, paths[i], "w"); if (f) used = paths[i]; }
	}
	if (!f) return;
	fprintf(f, "AoTv4 bazaar vtable dump. baseAddress=0x%08X (image base 0x400000)  file: %s\n\n",
		(unsigned)baseAddress, used ? used : "");
	DumpOneBazaarVtable(f, "CBazaarWnd (trader/sell)", 0xD1FCB0);
	DumpOneBazaarVtable(f, "CBarterWnd (buyer)",       0xF70CF0);
	DumpCommandList(f);
	fclose(f);
}

// ----------------------------------------------------------------- game-thread hook
// ProcessGameEvents (RoF2 0x539E60) runs every frame on the main thread. We use it to
// (a) lazily spawn the overlay thread once the game is up, and (b) run the chosen pick
// command on the game thread (calling InterpretCmd from the overlay thread is unsafe).
// Run a slash command on the game thread via CEverQuest::InterpretCmd (0x51FCE0). Calling this
// from the overlay thread is unsafe, so picks are queued and dispatched here.
static void RunGameCommand(const char* cmd)
{
	if (pEverQuest && pLocalPlayer) {
		typedef void (__thiscall *fInterpretCmd)(void* pThis, void* pChar, const char* cmd);
		auto fn = (fInterpretCmd)((((DWORD)0x51FCE0 - 0x400000) + baseAddress));
		fn((void*)pEverQuest, (void*)pLocalPlayer, cmd);
	}
}

namespace nativeinterface { void Start(); }   // AoTv4: MQ2 map overlay worker (native_map.cpp)

// AoTv4: tradeskill skill-dropdown driver lives in core_tradeskill.h (compiled in eqgame.cpp's TU);
// forward-declare it here so the per-frame heartbeat can call it without re-including the header.
void TS_DriveDropdown();

BOOL __cdecl ProcessGameEvents_Detour();
DETOUR_TRAMPOLINE_EMPTY(BOOL __cdecl ProcessGameEvents_Tramp());
BOOL __cdecl ProcessGameEvents_Detour()
{
	// The level-up reward picker is the native SIDL window (core_spellchoice_native.cpp) and random
	// AA is retired, so the tabbed Reward Journal that used to host both is gone. What survives is
	// the standalone "You Lost" window: death losses are a read-only list, not a reward picker.
	// AoTv4: the GDI You Lost overlay is SUPERSEDED by the native SIDL window in core_lostwindow.cpp
	// and is never started. The paint/wndproc/thread below are dead code kept only until someone
	// deletes them; nothing reaches them.
	if (false && g_lostEnabled && !g_lostOverlayStarted) {
		g_lostOverlayStarted = true;
		CreateThread(nullptr, 0, LostOverlayThreadProc, nullptr, 0, nullptr);
	}
	if (g_portalEnabled && !g_portalOverlayStarted) {
		g_portalOverlayStarted = true;
		CreateThread(nullptr, 0, PortalOverlayThreadProc, nullptr, 0, nullptr);
	}
	if (g_vendorEnabled && !g_vendorStarted) {
		g_vendorStarted = true;
		CreateThread(nullptr, 0, VendorThreadProc, nullptr, 0, nullptr);
	}
	if (g_searchEnabled && !g_searchStarted) {
		g_searchStarted = true;
		CreateThread(nullptr, 0, SearchThreadProc, nullptr, 0, nullptr);
	}
	// AoTv4: MQ2-style map overlay (native_map.cpp) -- spawn its worker once, deferred here out of the
	// DllMain loader lock. It self-gates on mq2_map.ini (map_enabled) + /nimap and only patches the map
	// window while the map is open, so this is safe to always start.
	{
		static bool g_mapStarted = false;
		if (!g_mapStarted) {
			g_mapStarted = true;
			nativeinterface::Start();
		}
	}

	// AoTv4: drive the Tradeskill Window skill dropdown. Self-gating -- a no-op unless the modded
	// EQUI_TradeskillWnd.xml (with the "SkillCombo" child) is installed. Populates the combo once and
	// fires a #<skill># filtered recipe search whenever the player changes the selected tradeskill.
	TS_DriveDropdown();

	// dispatch ONE queued vendor "/say ..." command per frame (set prices / open / close shop). One
	// per frame -- like the spell-pick dispatch -- so rapid back-to-back /say (vpset then vshop) each
	// actually get sent + arrive at the server in order (sending both same-frame dropped the second).
	// Advanced Loot: poll its Edit Filters search box so the rule list narrows as you type.
	AdvLootTick();
	AutoSkillTick();
	AllacloneTick();
	LostWindowTick();
	DungeonTick();

	if (g_vCmdTail != g_vCmdHead) {
		RunGameCommand(g_vendorCmds[g_vCmdTail]);
		g_vCmdTail = (g_vCmdTail + 1) % 24;
	}


	// Ctrl+Q opens the AoT menu, which is the way back to EVERY window this dll adds -- including the
	// two this hotkey used to reach directly (the reward picker, via Spells, and the death loss list,
	// via Last Death). One key for one menu beats a key whose meaning changed depending on whether a
	// reward happened to be owed.
	//
	// ⚠️ A reward still owed is NOT lost by this change: the picker opens itself the moment the server
	// offers, and the Spells button reopens it.
	//
	// Only fires while EQ is foreground, so it won't trigger while alt-tabbed. (Q rather than W so it
	// never collides with the RoF2 client's native Ctrl+W = /who keybind.)
	{
		static bool jprev = false;
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool focused = (fg == eq);
		bool combo = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetAsyncKeyState('Q') & 0x8000) != 0);
		if (combo && !jprev && focused) {
			AoTMenuShow();
		}
		jprev = combo;
	}

	// Bazaar vtable diagnostic -> aotv4_bazaar_vtable.txt. AUTO-dumps once the trader window object
	// exists (no key, no focus, no typed command) -- so just being in the Bazaar produces the file.
	// Ctrl+B re-dumps manually (no focus gate). This is the same per-frame hook as the spell-pick
	// dispatch below, which is known to run.
	{
		static bool dumped = false, bprev = false;
		void* pbw  = *(void**)((0xD1FCB0 - 0x400000) + baseAddress);   // *(CBazaarWnd**)pinstCBazaarWnd
		bool ctrlB = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetAsyncKeyState('B') & 0x8000) != 0);
		if ((pbw && !dumped) || (ctrlB && !bprev)) { DumpBazaarVtables(); dumped = true; }
		bprev = ctrlB;
	}

	// Ctrl+T: invoke the native /trader command handler (0x4EAB40, from __CommandList) directly to
	// open the trader (sell) window -- same as typing /trader, but no dependency on whether typed
	// commands route through our InterpretCmd hook. Tests whether the native trader works in any city.
	// Ctrl+Y does the same for /buyer (0x4E6F60). _CMDLIST handler sig: void __cdecl(spawn, args).
	{
		static bool tprev = false, yprev = false;
		bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		bool ctrlT = ctrl && ((GetAsyncKeyState('T') & 0x8000) != 0);
		bool ctrlY = ctrl && ((GetAsyncKeyState('Y') & 0x8000) != 0);
		void* sp = *(void**)((0xDD2630 - 0x400000) + baseAddress);     // pinstLocalPlayer
		typedef void(__cdecl *fCmd)(void*, const char*);
		if (ctrlT && !tprev) ((fCmd)((0x4EAB40 - 0x400000) + baseAddress))(sp, "");   // /trader
		if (ctrlY && !yprev) ((fCmd)((0x4E6F60 - 0x400000) + baseAddress))(sp, "");   // /buyer
		tprev = ctrlT; yprev = ctrlY;
	}

	// The Portal window is opened by clicking a discovered PoK book: the server sends "PORTALOPEN"
	// (handled in the chat detour -> g_portalVisible = true). Right-click the window to dismiss it.
	if (g_portalPendingShort[0]) {
		char cmd[96];
		sprintf_s(cmd, "/say portalgo %s", g_portalPendingShort);
		g_portalPendingShort[0] = 0;
		RunGameCommand(cmd);
	}

	// The spellpick / aapick dispatch that used to live here is gone with the GDI pickers. The native
	// reward window is drawn by the client's own UI engine, so it is already ON the game thread and
	// sends its pick straight through AoTQueueGameCommand -- no overlay thread to hand off from, and
	// no stolen foreground to give back.

	return ProcessGameEvents_Tramp();
}

// ----------------------------------------------------------------- entry
// Every window in this TU SHARES the two detours (you can't detour an address twice). Install
// them once; each Enable* just flips its feature flag, which the detour bodies check.
static void InstallChoiceHooks()
{
	if (g_choiceHooksInstalled) return;
	g_choiceHooksInstalled = true;

	// Both detours are pure in-memory byte patches -- safe under DllMain's loader lock.
	// The overlay window(s)/thread(s) are created LATER (first ProcessGameEvents tick), well
	// after load, because window creation under loader lock hard-fails startup.

	// route the server's transport chat lines to the right module (CEverQuest::dsp_chat)
	EzDetour((((DWORD)0x51F1A0 - 0x400000) + baseAddress), &SpellChat_Detour, &SpellChat_Tramp);

	// per-frame main-thread heartbeat: spawn overlay(s) + dispatch queued commands (ProcessGameEvents)
	EzDetour((((DWORD)0x539E60 - 0x400000) + baseAddress), &ProcessGameEvents_Detour, &ProcessGameEvents_Tramp);
}

void EnablePortalWindow()
{
	g_portalEnabled = true;
	InstallChoiceHooks();
}

// "You Lost" death window. This used to be the third tab of the Reward Journal; with the Spell and
// AA tabs retired it is back to being its own standalone window.
void EnableLostWindow()
{
	g_lostEnabled = true;
	InitLostWindow();      // the native SIDL window (core_lostwindow.cpp) owns the display now
	InstallChoiceHooks();  // still needed: dsp_chat routes LOSTDATA, ProcessGameEvents runs Ctrl+Q
}

// The Set-Up-Shop (vendor) price window. Driven by the server's VENDORDATA chat line; shares the
// dsp_chat + ProcessGameEvents hooks like the other windows.
void EnableVendorWindow()
{
	g_vendorEnabled = true;
	InstallChoiceHooks();
}
