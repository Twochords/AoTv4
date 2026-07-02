// core_spellwindow.cpp
// ---------------------------------------------------------------------------
// Level-up spell-choice window for the Bard-only random server.
//
// We tried drawing the panel as a DirectX9 overlay by hooking the device's /
// swap chain's Present, but the RoF2 client renders through EQGraphicsDX9.dll's
// own device, which shares NO vtable with any device we can create -- so the
// dummy-device hook never fires (proven: dh=2 sh=2 present=0 swap=0).
//
// Instead we draw the panel as a separate LAYERED, TOPMOST window (GDI) that sits
// over the EQ client in windowed mode. It is completely independent of EQ's
// renderer, so it always shows. Input is handled on the GAME thread via a hook of
// ProcessGameEvents (per-frame), which is where we safely run the pick command.
//
// Flow:
//   server -> client chat line:  SPELLCHOICEDATA name1^name2^name3
//   -> chat detour swallows it, stores names, sets g_visible
//   -> overlay thread shows/positions the layered window and paints the 3 choices
//   -> click on a "Select" button stores g_pendingPick
//   -> ProcessGameEvents hook (game thread) runs  /say spellpick N  and clears it
//
// Gated by areSpellChoiceWindowEnabled (_options.h) via EnableSpellChoiceWindow().
// ---------------------------------------------------------------------------

#include "MQ2Main.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

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
static bool g_combatGated[NUM_SKILLS] = { false };  // is skill id reward-gated?
static bool g_skillEarned[NUM_SKILLS] = { false };  // has the player earned it (server-sent)?

static void SkillUnlock_InitSets()
{
	for (int i = 0; i < (int)(sizeof(SKILLUNLOCK_COMBAT_IDS) / sizeof(int)); ++i) {
		int s = SKILLUNLOCK_COMBAT_IDS[i];
		if (s >= 0 && s < NUM_SKILLS) g_combatGated[s] = true;
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
		if (g_combatGated[skill]) {
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

// ----------------------------------------------------------------- state
static volatile bool   g_visible = false;
static char            g_names[3][160] = { {0}, {0}, {0} };
static int             g_icons[3] = { 0, 0, 0 };  // spellbook icon index per choice
static int             g_count = 0;

// art assets loaded from the client's uifiles/default at runtime
static char            g_uiDir[MAX_PATH] = {0};
static bool            g_assetsInit = false;
static struct { int sheet; HBITMAP bmp; } g_sheetCache[16];
static int             g_sheetCacheN = 0;

// window placement + drag state (overlay thread only)
static int             g_posX = 0, g_posY = 0;
static bool            g_positioned = false;
static bool            g_dragging = false;
static int             g_dragDX = 0, g_dragDY = 0;

static HBITMAP LoadTGA(const char* path);  // fwd decl (used by the chat diagnostic)
static volatile int    g_pendingPick = -1;     // 1..3 set by click, consumed on game thread
static volatile bool   g_overlayStarted = false;
static HWND            g_overlayHwnd = nullptr;

// ----------------------------------------------------------------- AA window state
// A parallel, separate window for the death-driven AA picker. Shares the art helpers
// (LoadTGA/GetSheet/BlitIcon/DrawShadow) and the two detours below, but has its own state,
// window, thread and message loop so it can show alongside the spell window.
static volatile bool   g_aaVisible = false;
static char            g_aaNames[3][160] = { {0}, {0}, {0} };
static int             g_aaIcons[3] = { 0, 0, 0 };   // cast-spell icon (0 = none)
static int             g_aaCosts[3] = { 0, 0, 0 };   // AA point cost of the choice
static int             g_aaCls[3]   = { 0, 0, 0 };   // original class bitmask (for the marker)
static int             g_aaCount  = 0;
static int             g_aaBanked = 0;               // unspent AA points (shown in the header)
static int             g_aaPosX = 0, g_aaPosY = 0;
static bool            g_aaPositioned = false;
static bool            g_aaDragging = false;
static int             g_aaDragDX = 0, g_aaDragDY = 0;
static volatile int    g_aaPendingPick = -1;
static volatile bool   g_aaOverlayStarted = false;
static HWND            g_aaHwnd = nullptr;

// which features are on (set by Enable*); the shared detours check these.
static bool            g_spellChoiceEnabled = false;
static bool            g_aaChoiceEnabled    = false;
static bool            g_choiceHooksInstalled = false;

// per-choice description text (server-sent) shown on hover; *DescSel = row the cursor is over.
static char            g_spellDesc[3][320] = { {0}, {0}, {0} };
static int             g_spellDescSel = -1;
static char            g_aaDesc[3][320]    = { {0}, {0}, {0} };
static int             g_aaDescSel = -1;

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

// ----------------------------------------------------------------- Reward Journal (tabbed) state
// One Quest-Journal-styled window with Spell / AA / Lost tabs, replacing the 3 separate windows.
// It renders the SAME data the parsers fill (g_names/g_aa*/g_lost*) and queues the same picks.
// Opened by Ctrl+W (review) or auto-opened to the relevant tab on a level-up/death feed.
enum { JTAB_SPELL = 0, JTAB_AA = 1, JTAB_LOST = 2 };
static bool            g_journalEnabled = false;
static volatile bool   g_journalVisible = false;
static int             g_journalTab = JTAB_SPELL;
static volatile bool   g_journalStarted = false;
static HWND            g_journalHwnd = nullptr;
static int             g_jPosX = 0, g_jPosY = 0;
static bool            g_jPositioned = false, g_jDragging = false;
static int             g_jDragDX = 0, g_jDragDY = 0;
static int             g_jLostScroll = 0;     // scroll offset for the Lost tab

// Open the journal to `tab` and switch to it. A fresh level-up / death / AA offer is an actionable
// event, so we ALWAYS jump to the relevant tab -- even when the window is already up showing another
// one (e.g. the AA tab is still open from death when the next level-up's spell choice arrives).
static void JournalOpen(int tab)
{
	g_journalTab     = tab;
	g_journalVisible = true;
}

// window box (overlay-client pixels)
static const int WIN_W  = 440;
static const int TOP_H  = 34;
static const int ROW_H  = 52;
static const int DESC_H = 90;                        // bottom description panel
static const int ROWS_B = TOP_H + 3 * ROW_H + 12;    // y where the desc panel begins
static const int WIN_H  = ROWS_B + DESC_H;

// which choice row (0..count-1) the cursor is over within the rows band, else -1
static int RowAtY(int y, int count)
{
	if (y < TOP_H || y >= TOP_H + 3 * ROW_H) return -1;
	int i = (y - TOP_H) / ROW_H;
	return (i >= 0 && i < count) ? i : -1;
}
static void DescRect(RECT& r) { r.left = 10; r.top = ROWS_B; r.right = WIN_W - 10; r.bottom = WIN_H - 8; }

static void ButtonRect(int i, RECT& r)
{
	r.right  = WIN_W - 14;
	r.left   = r.right - 96;
	r.top    = TOP_H + i * ROW_H + 10;
	r.bottom = r.top + 32;
}

static const int ICON_SZ = 40;

static void IconRect(int i, RECT& r)
{
	r.left   = 16;
	r.top    = TOP_H + i * ROW_H + (ROW_H - ICON_SZ) / 2;
	r.right  = r.left + ICON_SZ;
	r.bottom = r.top + ICON_SZ;
}

static void NameRect(int i, RECT& r)
{
	r.left   = 16 + ICON_SZ + 12;   // after the icon
	r.right  = WIN_W - 110;
	r.top    = TOP_H + i * ROW_H + 6;
	r.bottom = r.top + ROW_H - 12;
}

// ----------------------------------------------------------------- chat trigger
static const char* SPLCHOICE_TAG = "SPELLCHOICEDATA ";

static bool HandleChat(const char* msg)
{
	if (!msg) return false;
	const char* tag = strstr(msg, SPLCHOICE_TAG);
	if (!tag) return false;
	tag += strlen(SPLCHOICE_TAG);

	g_count = 0;
	std::string cur;
	for (const char* p = tag; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_count < 3) {
				// each token is "name|icon" (icon optional)
				size_t bar = cur.find('|');
				std::string nm = (bar == std::string::npos) ? cur : cur.substr(0, bar);
				int ic = (bar == std::string::npos) ? 0 : atoi(cur.c_str() + bar + 1);
				strncpy_s(g_names[g_count], sizeof(g_names[g_count]), nm.c_str(), _TRUNCATE);
				g_icons[g_count] = ic;
				g_count++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
	g_visible = (g_count > 0);
	if (g_count > 0) { g_spellDescSel = -1; JournalOpen(JTAB_SPELL); }  // journal -> Spell tab
	return true; // swallow our trigger line
}

// ----------------------------------------------------------------- AA chat trigger
// "AACHOICEDATA <banked>^name|icon|cost|cls^name|icon|cost|cls^..." -- first ^-field is the
// player's unspent AA pool; each following field is one choice.
static const char* AACHOICE_TAG = "AACHOICEDATA ";

static bool AAHandleChat(const char* msg)
{
	if (!msg) return false;
	const char* tag = strstr(msg, AACHOICE_TAG);
	if (!tag) return false;
	tag += strlen(AACHOICE_TAG);

	g_aaCount  = 0;
	g_aaBanked = 0;
	int field = 0;            // 0 = banked total, 1.. = choices
	std::string cur;
	for (const char* p = tag; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (field == 0) {
				g_aaBanked = atoi(cur.c_str());
			} else if (!cur.empty() && g_aaCount < 3) {
				// "name|icon|cost|cls"
				std::string parts[4]; int pi = 0;
				for (size_t k = 0; k < cur.size() && pi < 4; ++k) {
					if (cur[k] == '|') ++pi; else parts[pi] += cur[k];
				}
				strncpy_s(g_aaNames[g_aaCount], sizeof(g_aaNames[g_aaCount]), parts[0].c_str(), _TRUNCATE);
				g_aaIcons[g_aaCount] = atoi(parts[1].c_str());
				g_aaCosts[g_aaCount] = atoi(parts[2].c_str());
				g_aaCls[g_aaCount]   = atoi(parts[3].c_str());
				g_aaCount++;
			}
			cur.clear();
			++field;
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
	g_aaVisible = (g_aaCount > 0);
	if (g_aaCount > 0) { g_aaDescSel = -1; JournalOpen(JTAB_AA); }     // journal -> AA tab
	return true;  // swallow our trigger line
}

// ----------------------------------------------------------------- description lines
// Sibling lines to the *CHOICEDATA lines: "SPELLDESCDATA d1^d2^d3" / "AADESCDATA d1^d2^d3".
// Descriptions are server-sanitized to contain no ^ or |.
static void ParseDescs(const char* s, char dst[3][320])
{
	for (int i = 0; i < 3; ++i) dst[i][0] = 0;
	int idx = 0;
	std::string cur;
	for (const char* p = s; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (idx < 3) strncpy_s(dst[idx], 320, cur.c_str(), _TRUNCATE);
			cur.clear(); ++idx;
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
}
static bool HandleSpellDesc(const char* msg)
{
	if (!msg) return false;
	const char* t = strstr(msg, "SPELLDESCDATA ");
	if (!t) return false;
	ParseDescs(t + strlen("SPELLDESCDATA "), g_spellDesc);
	return true;
}
static bool HandleAADesc(const char* msg)
{
	if (!msg) return false;
	const char* t = strstr(msg, "AADESCDATA ");
	if (!t) return false;
	ParseDescs(t + strlen("AADESCDATA "), g_aaDesc);
	return true;
}

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
	g_jLostScroll = 0;
	if (g_lostCount > 0) JournalOpen(JTAB_LOST);                      // journal -> Lost tab
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
static int             g_vendorTab = 0;                // 0 = Add Items, 1 = My Shop

// Add Items tab (SHOPINVDATA). g_vendorIds now holds the inventory SLOT id used by "/say shopadd".
static int             g_vendorCount = 0;
static int             g_vendorIds[80] = { 0 };        // inventory slot id
static char            g_vendorNames[80][64] = { { 0 } };
static unsigned        g_vendorVendor[80] = { 0 };     // vendor value (copper) -> default price hint
static int             g_vCoin[80][4] = { { 0 } };     // per item coins: [0]=plat [1]=gold [2]=silver [3]=copper
static int             g_vFocusItem = -1, g_vFocusCoin = 0;  // which coin field is being typed into
static int             g_vendorScroll = 0;

// My Shop tab (MYSHOPDATA).
static int             g_myCount = 0;
static int             g_mySn[80] = { 0 };             // item_sn -> used by "/say shoppull"
static char            g_myName[80][64] = { { 0 } };
static unsigned        g_myCost[80] = { 0 };           // listed unit cost (copper)
static int             g_myScroll = 0;

static int             g_vPosX = 0, g_vPosY = 0;
static bool            g_vPositioned = false, g_vDragging = false;
static int             g_vDragDX = 0, g_vDragDY = 0;

// queue of "/say ..." commands from the window thread, dispatched on the game thread (like picks)
static char            g_vendorCmds[24][96];
static volatile int    g_vCmdHead = 0, g_vCmdTail = 0;
static void VendorQueue(const char* c) {
	int n = (g_vCmdHead + 1) % 24;
	if (n != g_vCmdTail) { strncpy_s(g_vendorCmds[g_vCmdHead], 96, c, _TRUNCATE); g_vCmdHead = n; }
}

// split "a|b|c|d" (one row) into up to 4 fields.
static void VSplit4(const std::string& s, std::string out[4]) {
	int pi = 0;
	for (size_t k = 0; k < s.size() && pi < 4; ++k) { if (s[k] == '|') ++pi; else out[pi] += s[k]; }
}

// SHOPINVDATA slot|itemid|name|vendor^...  -> the "Add Items" tab.
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
				std::string f[4]; VSplit4(cur, f);
				g_vendorIds[g_vendorCount]    = atoi(f[0].c_str());              // inventory slot
				strncpy_s(g_vendorNames[g_vendorCount], 64, f[2].c_str(), _TRUNCATE);
				g_vendorVendor[g_vendorCount] = (unsigned)atoi(f[3].c_str());
				unsigned dv = g_vendorVendor[g_vendorCount] * 5;                 // default price = 5x vendor
				g_vCoin[g_vendorCount][0] = dv / 1000;
				g_vCoin[g_vendorCount][1] = (dv / 100) % 10;
				g_vCoin[g_vendorCount][2] = (dv / 10) % 10;
				g_vCoin[g_vendorCount][3] = dv % 10;
				g_vendorCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else cur += *p;
	}
	g_vendorVisible = true;
	return true;
}

// MYSHOPDATA itemsn|itemid|name|cost^...  -> the "My Shop" tab.
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
				std::string f[4]; VSplit4(cur, f);
				g_mySn[g_myCount]   = atoi(f[0].c_str());                        // item_sn
				strncpy_s(g_myName[g_myCount], 64, f[2].c_str(), _TRUNCATE);
				g_myCost[g_myCount] = (unsigned)atoi(f[3].c_str());
				g_myCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else cur += *p;
	}
	return true;
}

static bool HandleVendorChat(const char* msg)
{
	if (!msg) return false;
	if (HandleShopInv(msg)) return true;
	if (HandleMyShop(msg))  return true;
	return false;
}

// CEverQuest::dsp_chat (RoF2 0x51F1A0) detour -- thiscall, detoured as __fastcall.
void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct);
DETOUR_TRAMPOLINE_EMPTY(void __fastcall SpellChat_Tramp(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct));
void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct)
{
	// Hide the pick command echoes. Selecting a choice issues "/say spellpick N" or
	// "/say aapick N", which the client would otherwise show as  You say, '...'  (and
	// broadcast to nearby players). Since every player runs this dll, swallowing any line
	// containing the trigger hides it on the speaker's screen AND on every listener's.
	if (msg && (strstr(msg, "spellpick") || strstr(msg, "aapick") ||
		strstr(msg, "portalgo") || strstr(msg, "portalreq") ||
		strstr(msg, "shopopen") || strstr(msg, "shoprefresh") ||
		strstr(msg, "shopadd") || strstr(msg, "shoppull"))) return;  // shop-window echoes

	// Death-AA rewards are granted with ignore_cost=true (the points live in a private bank, not
	// m_pp.aapoints), so the engine's "You have gained/improved the ability ... at a cost of 0
	// ability point(s)" line always says 0 -- misleading. Swallow it; aa_choice.lua prints the
	// REAL banked cost instead. (Only our 0-cost grants ever produce this exact text.)
	if (msg && strstr(msg, "at a cost of 0 ability")) return;

	if (SkillUnlock_HandleChat(msg)) return;             // swallow SKILLUNLOCKDATA (earned set)
	if (g_spellChoiceEnabled && HandleChat(msg)) return; // swallow SPELLCHOICEDATA
	if (g_spellChoiceEnabled && HandleSpellDesc(msg)) return; // swallow SPELLDESCDATA
	if (g_aaChoiceEnabled && AAHandleChat(msg)) return;  // swallow AACHOICEDATA
	if (g_aaChoiceEnabled && HandleAADesc(msg)) return;  // swallow AADESCDATA
	if (g_portalEnabled && msg && strstr(msg, "PORTALOPEN"))  { g_portalVisible = true; g_portalIdleTick = GetTickCount(); return; } // click-a-book opens
	if (g_portalEnabled && msg && strstr(msg, "PORTALCLOSE")) { g_portalVisible = false; return; }  // zoning closes it
	if (g_portalEnabled && HandlePortalChat(msg)) return; // swallow PORTALDATA
	if (g_lostEnabled && HandleLostChat(msg)) return;     // swallow LOSTDATA (You Lost window)
	if (g_vendorEnabled && HandleVendorChat(msg)) return; // swallow SHOPINVDATA / MYSHOPDATA (shop window)
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

static void InitAssets()
{
	if (g_assetsInit) return;
	g_assetsInit = true;

	char exe[MAX_PATH] = {0};
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	char* slash = strrchr(exe, '\\');
	if (slash) slash[1] = 0; else exe[0] = 0;
	sprintf_s(g_uiDir, "%suifiles\\default\\", exe);
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

// Bottom description panel (word-wrapped). Shows `desc` (the hovered row's text) or a hint.
static void DrawDescPanel(HDC mem, HFONT smallFont, const char* desc,
	COLORREF panelBg, COLORREF border, COLORREF ink)
{
	RECT dr; DescRect(dr);
	HBRUSH bg = CreateSolidBrush(panelBg); FillRect(mem, &dr, bg); DeleteObject(bg);
	HBRUSH bd = CreateSolidBrush(border);  FrameRect(mem, &dr, bd); DeleteObject(bd);
	RECT tr = dr; InflateRect(&tr, -7, -5);
	HFONT old = (HFONT)SelectObject(mem, smallFont);
	SetTextColor(mem, ink);
	const char* txt = (desc && desc[0]) ? desc : "Hover an option for its description.";
	DrawTextA(mem, txt, -1, &tr, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
	SelectObject(mem, old);
}

// ----------------------------------------------------------------- layered window
static void PaintOverlay(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	// double-buffer to avoid flicker
	HDC     mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	RECT full = { 0, 0, WIN_W, WIN_H };
	// Parchment background: warm cream base with a soft top-to-bottom shade, drawn with
	// GDI (no book art). Scanline gradient is cheap for a panel this sfont.
	for (int y = 0; y < WIN_H; ++y) {
		float t = (float)y / (float)WIN_H;
		int r = (int)(222 - 26 * t);   // 222 -> 196
		int g = (int)(205 - 28 * t);   // 205 -> 177
		int b = (int)(168 - 30 * t);   // 168 -> 138
		HBRUSH line = CreateSolidBrush(RGB(r, g, b));
		RECT lr = { 0, y, WIN_W, y + 1 };
		FillRect(mem, &lr, line);
		DeleteObject(line);
	}

	// double border (dark brown outer, gold inner) for a framed-page look
	HBRUSH brdOuter = CreateSolidBrush(RGB(70, 50, 25));
	FrameRect(mem, &full, brdOuter);
	DeleteObject(brdOuter);
	RECT inner = { 2, 2, WIN_W - 2, WIN_H - 2 };
	HBRUSH brdInner = CreateSolidBrush(RGB(150, 120, 70));
	FrameRect(mem, &inner, brdInner);
	DeleteObject(brdInner);

	HFONT font = CreateFontA(-17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	HFONT smallFont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	SetBkMode(mem, TRANSPARENT);

	// title (dark ink with a light highlight = embossed look on parchment)
	RECT tr = { 14, 6, WIN_W - 8, TOP_H };
	DrawShadow(mem, "Choose a New Spell", tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER,
		RGB(74, 48, 16), RGB(245, 235, 210));
	// divider under the title
	HBRUSH div = CreateSolidBrush(RGB(150, 120, 70));
	RECT dr = { 12, TOP_H - 2, WIN_W - 12, TOP_H - 1 };
	FillRect(mem, &dr, div);
	DeleteObject(div);

	for (int i = 0; i < g_count; ++i) {
		RECT ir; IconRect(i, ir);
		BlitIcon(mem, g_icons[i], ir);
		HBRUSH ifr = CreateSolidBrush(RGB(70, 50, 25));
		FrameRect(mem, &ir, ifr);
		DeleteObject(ifr);

		RECT nr; NameRect(i, nr);
		DrawShadow(mem, g_names[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(48, 32, 12), RGB(238, 228, 205));

		RECT br; ButtonRect(i, br);
		HBRUSH btn = CreateSolidBrush(RGB(120, 92, 52));
		FillRect(mem, &br, btn);
		DeleteObject(btn);
		HBRUSH btnEdge = CreateSolidBrush(RGB(70, 50, 25));
		FrameRect(mem, &br, btnEdge);
		DeleteObject(btnEdge);
		DrawShadow(mem, "Select", br, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
			RGB(255, 248, 228), RGB(40, 28, 12));
	}

	// hovered-choice description panel (parchment tones)
	const char* sd = (g_spellDescSel >= 0 && g_spellDescSel < g_count) ? g_spellDesc[g_spellDescSel] : nullptr;
	DrawDescPanel(mem, smallFont, sd, RGB(232, 222, 196), RGB(150, 120, 70), RGB(60, 42, 18));

	BitBlt(hdc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY);

	SelectObject(mem, oldFont);
	DeleteObject(font);
	DeleteObject(smallFont);
	SelectObject(mem, old);
	DeleteObject(bmp);
	DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT:
		PaintOverlay(hwnd);
		return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		// dragging the title strip (above the rows) moves the window
		if (pt.y < TOP_H) {
			g_dragging = true;
			g_dragDX = pt.x;   // cursor offset within the window
			g_dragDY = pt.y;
			SetCapture(hwnd);
		}
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (g_dragging) {
			POINT cur; GetCursorPos(&cur);
			g_posX = cur.x - g_dragDX;
			g_posY = cur.y - g_dragDY;
			g_positioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_posX, g_posY, WIN_W, WIN_H, SWP_NOACTIVATE);
		} else {
			int row = RowAtY((short)HIWORD(lp), g_count);   // hover -> show that row's description
			if (row != g_spellDescSel) { g_spellDescSel = row; InvalidateRect(hwnd, nullptr, FALSE); }
		}
		return 0;
	}

	case WM_LBUTTONUP: {
		if (g_dragging) { g_dragging = false; ReleaseCapture(); return 0; }
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		for (int i = 0; i < g_count; ++i) {
			RECT r; ButtonRect(i, r);
			if (PtInRect(&r, pt)) {
				g_pendingPick = i + 1;   // consumed by the ProcessGameEvents hook
				g_visible = false;
				ShowWindow(hwnd, SW_HIDE);
				break;
			}
		}
		return 0;
	}

	case WM_TIMER: {
		// EQADDR_HWND is the ADDRESS of the game's HWND variable, so dereference it.
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		// Only show while the user is actually in EQ -- or interacting with our own window --
		// so the overlay stays at EQ's level and never floats over other apps (Visual Studio,
		// browser, etc.). Allowing our own window as "foreground" means clicking/dragging the
		// overlay doesn't make it vanish (that was an earlier complaint).
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_visible && active) {
			if (!g_positioned && eq) {
				// first show: center in the upper third of the EQ client
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top };
				ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
				g_posX = tl.x + (cw - WIN_W) / 2;
				g_posY = tl.y + (ch - WIN_H) / 3;
				g_positioned = true;
			}
			if (g_positioned) {
				if (!IsWindowVisible(hwnd)) {
					SetWindowPos(hwnd, HWND_TOPMOST, g_posX, g_posY, WIN_W, WIN_H, SWP_NOACTIVATE);
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

static DWORD WINAPI OverlayThreadProc(LPVOID)
{
	InitAssets();  // load spellbook background + prepare icon-sheet cache

	HINSTANCE hInst = GetModuleHandleA(nullptr);

	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = OverlayWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTSpellChoiceOverlay";
	RegisterClassExA(&wc);

	// NOTE: deliberately NOT WS_EX_NOACTIVATE -- clicking/dragging the window must take
	// foreground from EQ so EQ stops reading the mouse (otherwise dragging also rotates
	// the camera). It still appears without stealing focus (shown via SW_SHOWNOACTIVATE).
	g_overlayHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTSpellChoiceOverlay", "", WS_POPUP,
		0, 0, WIN_W, WIN_H, nullptr, nullptr, hInst, nullptr);
	if (!g_overlayHwnd) return 0;

	SetLayeredWindowAttributes(g_overlayHwnd, 0, 240, LWA_ALPHA);
	SetTimer(g_overlayHwnd, 1, 60, nullptr);  // reposition / show-hide / repaint ~16fps

	MSG m;
	while (GetMessageA(&m, nullptr, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessageA(&m);
	}
	return 0;
}

// ----------------------------------------------------------------- AA window paint/input
static void PaintAAOverlay(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	HDC     mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	RECT full = { 0, 0, WIN_W, WIN_H };
	// Cooler slate/blue parchment to distinguish the AA window from the (warm) spell window.
	for (int y = 0; y < WIN_H; ++y) {
		float t = (float)y / (float)WIN_H;
		int r = (int)(40 + 18 * t);
		int g = (int)(52 + 22 * t);
		int b = (int)(74 + 30 * t);
		HBRUSH line = CreateSolidBrush(RGB(r, g, b));
		RECT lr = { 0, y, WIN_W, y + 1 };
		FillRect(mem, &lr, line);
		DeleteObject(line);
	}

	HBRUSH brdOuter = CreateSolidBrush(RGB(20, 26, 40));
	FrameRect(mem, &full, brdOuter);
	DeleteObject(brdOuter);
	RECT inner = { 2, 2, WIN_W - 2, WIN_H - 2 };
	HBRUSH brdInner = CreateSolidBrush(RGB(120, 150, 200));
	FrameRect(mem, &inner, brdInner);
	DeleteObject(brdInner);

	HFONT font = CreateFontA(-17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	HFONT smallFont = CreateFontA(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	SetBkMode(mem, TRANSPARENT);

	// title (left) + banked points (right)
	RECT tr = { 14, 6, WIN_W - 150, TOP_H };
	DrawShadow(mem, "Choose an Alternate Ability", tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER,
		RGB(225, 235, 250), RGB(18, 24, 38));
	char bk[48]; sprintf_s(bk, "Banked: %d AA", g_aaBanked);
	RECT br0 = { WIN_W - 150, 6, WIN_W - 12, TOP_H };
	DrawShadow(mem, bk, br0, DT_RIGHT | DT_SINGLELINE | DT_VCENTER,
		RGB(255, 226, 140), RGB(18, 24, 38));
	HBRUSH div = CreateSolidBrush(RGB(120, 150, 200));
	RECT dr = { 12, TOP_H - 2, WIN_W - 12, TOP_H - 1 };
	FillRect(mem, &dr, div);
	DeleteObject(div);

	for (int i = 0; i < g_aaCount; ++i) {
		RECT ir; IconRect(i, ir);
		BlitIcon(mem, g_aaIcons[i], ir);
		HBRUSH ifr = CreateSolidBrush(RGB(20, 26, 40));
		FrameRect(mem, &ir, ifr);
		DeleteObject(ifr);

		RECT nr; NameRect(i, nr);
		RECT nameLine = nr; nameLine.bottom = nr.top + 24;
		RECT subLine  = nr; subLine.top    = nr.top + 22;

		SelectObject(mem, font);
		DrawShadow(mem, g_aaNames[i], nameLine, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(232, 240, 252), RGB(16, 22, 34));

		char sub[64];
		sprintf_s(sub, "Cost: %d AA", g_aaCosts[i]);
		SelectObject(mem, smallFont);
		DrawShadow(mem, sub, subLine, DT_LEFT | DT_TOP | DT_SINGLELINE,
			RGB(255, 226, 140), RGB(16, 22, 34));

		RECT br; ButtonRect(i, br);
		HBRUSH btn = CreateSolidBrush(RGB(58, 78, 116));
		FillRect(mem, &br, btn);
		DeleteObject(btn);
		HBRUSH btnEdge = CreateSolidBrush(RGB(20, 26, 40));
		FrameRect(mem, &br, btnEdge);
		DeleteObject(btnEdge);
		SelectObject(mem, font);
		DrawShadow(mem, "Train", br, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
			RGB(240, 248, 255), RGB(14, 20, 32));
	}

	// hovered-choice description panel (slate tones)
	const char* ad = (g_aaDescSel >= 0 && g_aaDescSel < g_aaCount) ? g_aaDesc[g_aaDescSel] : nullptr;
	DrawDescPanel(mem, smallFont, ad, RGB(30, 40, 58), RGB(120, 150, 200), RGB(216, 228, 245));

	BitBlt(hdc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY);

	SelectObject(mem, oldFont);
	DeleteObject(font);
	DeleteObject(smallFont);
	SelectObject(mem, old);
	DeleteObject(bmp);
	DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK AAOverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT:
		PaintAAOverlay(hwnd);
		return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		if (pt.y < TOP_H) {
			g_aaDragging = true;
			g_aaDragDX = pt.x; g_aaDragDY = pt.y;
			SetCapture(hwnd);
		}
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (g_aaDragging) {
			POINT cur; GetCursorPos(&cur);
			g_aaPosX = cur.x - g_aaDragDX;
			g_aaPosY = cur.y - g_aaDragDY;
			g_aaPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_aaPosX, g_aaPosY, WIN_W, WIN_H, SWP_NOACTIVATE);
		} else {
			int row = RowAtY((short)HIWORD(lp), g_aaCount);  // hover -> show that row's description
			if (row != g_aaDescSel) { g_aaDescSel = row; InvalidateRect(hwnd, nullptr, FALSE); }
		}
		return 0;
	}

	case WM_LBUTTONUP: {
		if (g_aaDragging) { g_aaDragging = false; ReleaseCapture(); return 0; }
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		for (int i = 0; i < g_aaCount; ++i) {
			RECT r; ButtonRect(i, r);
			if (PtInRect(&r, pt)) {
				g_aaPendingPick = i + 1;   // consumed by the ProcessGameEvents hook
				g_aaVisible = false;
				ShowWindow(hwnd, SW_HIDE);
				break;
			}
		}
		return 0;
	}

	case WM_TIMER: {
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_aaVisible && active) {
			if (!g_aaPositioned && eq) {
				// first show: bottom of the screen (the You Lost window sits up top on death)
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top };
				ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
				g_aaPosX = tl.x + (cw - WIN_W) / 2;
				g_aaPosY = tl.y + ch - WIN_H - 24;
				if (g_aaPosY < tl.y) g_aaPosY = tl.y;
				g_aaPositioned = true;
			}
			if (g_aaPositioned) {
				if (!IsWindowVisible(hwnd)) {     // set position + z-order ONLY on show (no per-tick
					SetWindowPos(hwnd, HWND_TOPMOST, g_aaPosX, g_aaPosY, WIN_W, WIN_H, SWP_NOACTIVATE);
					ShowWindow(hwnd, SW_SHOWNOACTIVATE);   // topmost re-assert -> stops the flashing)
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

static DWORD WINAPI AAOverlayThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);

	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = AAOverlayWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTAAChoiceOverlay";
	RegisterClassExA(&wc);

	g_aaHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTAAChoiceOverlay", "", WS_POPUP,
		0, 0, WIN_W, WIN_H, nullptr, nullptr, hInst, nullptr);
	if (!g_aaHwnd) return 0;

	SetLayeredWindowAttributes(g_aaHwnd, 0, 240, LWA_ALPHA);
	SetTimer(g_aaHwnd, 1, 60, nullptr);

	MSG m;
	while (GetMessageA(&m, nullptr, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessageA(&m);
	}
	return 0;
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
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);  // hovering grabs focus so a scroll never reaches EQ's camera
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

// ----------------------------------------------------------------- Reward Journal paint/input
static const int JW         = 520;
static const int JTITLE_H   = 22;
static const int JTAB_H     = 26;
static const int JCONTENT_Y = JTITLE_H + JTAB_H;     // 48
static const int JROW_H     = 52;
static const int JLOST_ROW  = 24;
static const int JLOST_VIS  = 11;
static const int JH         = 392;

static int  JRowTop(int i) { return JCONTENT_Y + 30 + i * JROW_H; }
static void JIcon(int i, RECT& r) { r.left = 18; r.top = JRowTop(i) + (JROW_H - 40) / 2; r.right = r.left + 40; r.bottom = r.top + 40; }
static void JName(int i, RECT& r) { r.left = 70; r.right = JW - 122; r.top = JRowTop(i) + 6; r.bottom = JRowTop(i) + JROW_H - 12; }
static void JBtn(int i, RECT& r)  { r.right = JW - 16; r.left = r.right - 98; r.top = JRowTop(i) + 10; r.bottom = r.top + 32; }
static void JDesc(RECT& r)        { r.left = 12; r.right = JW - 12; r.top = JCONTENT_Y + 30 + 3 * JROW_H + 6; r.bottom = JH - 10; }
static void JTab(int i, RECT& r)  { int w = JW / 3; r.left = (i == 0) ? 0 : i * w; r.right = (i == 2) ? JW : (i + 1) * w; r.top = JTITLE_H; r.bottom = JTITLE_H + JTAB_H; }
static int  JLostTop(int i) { return JCONTENT_Y + 30 + i * JLOST_ROW; }
static void JLostUp(RECT& r) { r.top = JCONTENT_Y + 4; r.bottom = JCONTENT_Y + 24; r.right = JW - 48; r.left = r.right - 30; }
static void JLostDn(RECT& r) { r.top = JCONTENT_Y + 4; r.bottom = JCONTENT_Y + 24; r.right = JW - 14; r.left = r.right - 30; }

// theme colors (Quest-Journal navy)
#define J_INK   RGB(220, 228, 245)
#define J_DIM   RGB(170, 185, 215)
#define J_GOLD  RGB(232, 200, 120)
#define J_LINE  RGB(70, 90, 140)
#define J_SHAD  RGB(8, 10, 18)

static void JDrawButton(HDC mem, HFONT font, const RECT& b, const char* label)
{
	HBRUSH bg = CreateSolidBrush(RGB(48, 64, 104)); FillRect(mem, (RECT*)&b, bg); DeleteObject(bg);
	HBRUSH ed = CreateSolidBrush(RGB(90, 115, 170)); FrameRect(mem, (RECT*)&b, ed); DeleteObject(ed);
	SelectObject(mem, font);
	DrawShadow(mem, label, b, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(235, 225, 180), J_SHAD);
}

static void JDrawDesc(HDC mem, HFONT sfont, const char* desc)
{
	RECT dr; JDesc(dr);
	HBRUSH bg = CreateSolidBrush(RGB(14, 18, 32)); FillRect(mem, &dr, bg); DeleteObject(bg);
	HBRUSH bd = CreateSolidBrush(J_LINE); FrameRect(mem, &dr, bd); DeleteObject(bd);
	RECT tr = dr; InflateRect(&tr, -7, -5);
	SelectObject(mem, sfont); SetTextColor(mem, RGB(195, 208, 232));
	DrawTextA(mem, (desc && desc[0]) ? desc : "Hover an option for its description.", -1, &tr,
		DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
}

static void JPaintSpell(HDC mem, HFONT font, HFONT sfont)
{
	RECT hr = { 16, JCONTENT_Y + 4, JW - 16, JCONTENT_Y + 28 };
	SelectObject(mem, font);
	if (g_count == 0) {
		DrawShadow(mem, "No spell reward pending.  (Choose one on level-up.)", hr,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE, J_DIM, J_SHAD);
		return;
	}
	DrawShadow(mem, "Choose a New Spell", hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, J_GOLD, J_SHAD);
	for (int i = 0; i < g_count; ++i) {
		RECT ir; JIcon(i, ir); BlitIcon(mem, g_icons[i], ir);
		HBRUSH f = CreateSolidBrush(J_LINE); FrameRect(mem, &ir, f); DeleteObject(f);
		RECT nr; JName(i, nr);
		SelectObject(mem, font);
		DrawShadow(mem, g_names[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, J_INK, J_SHAD);
		RECT b; JBtn(i, b); JDrawButton(mem, font, b, "Select");
	}
	const char* d = (g_spellDescSel >= 0 && g_spellDescSel < g_count) ? g_spellDesc[g_spellDescSel] : nullptr;
	JDrawDesc(mem, sfont, d);
}

static void JPaintAA(HDC mem, HFONT font, HFONT sfont)
{
	RECT hr = { 16, JCONTENT_Y + 4, JW - 150, JCONTENT_Y + 28 };
	RECT br0 = { JW - 150, JCONTENT_Y + 4, JW - 16, JCONTENT_Y + 28 };
	char bk[48]; sprintf_s(bk, "Banked: %d AA", g_aaBanked);
	SelectObject(mem, font);
	DrawShadow(mem, bk, br0, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, J_GOLD, J_SHAD);
	if (g_aaCount == 0) {
		DrawShadow(mem, "No AA reward pending.", hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, J_DIM, J_SHAD);
		return;
	}
	DrawShadow(mem, "Choose an Alternate Ability", hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, J_GOLD, J_SHAD);
	for (int i = 0; i < g_aaCount; ++i) {
		RECT ir; JIcon(i, ir); BlitIcon(mem, g_aaIcons[i], ir);
		HBRUSH f = CreateSolidBrush(J_LINE); FrameRect(mem, &ir, f); DeleteObject(f);
		RECT nr; JName(i, nr);
		RECT n1 = nr; n1.bottom = nr.top + 24;
		RECT n2 = nr; n2.top = nr.top + 22;
		SelectObject(mem, font);
		DrawShadow(mem, g_aaNames[i], n1, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS, J_INK, J_SHAD);
		char sub[48]; sprintf_s(sub, "Cost: %d AA", g_aaCosts[i]);
		SelectObject(mem, sfont);
		DrawShadow(mem, sub, n2, DT_LEFT | DT_TOP | DT_SINGLELINE, J_GOLD, J_SHAD);
		RECT b; JBtn(i, b); JDrawButton(mem, font, b, "Train");
	}
	const char* d = (g_aaDescSel >= 0 && g_aaDescSel < g_aaCount) ? g_aaDesc[g_aaDescSel] : nullptr;
	JDrawDesc(mem, sfont, d);
}

static void JPaintLost(HDC mem, HFONT font, HFONT sfont)
{
	RECT hr = { 16, JCONTENT_Y + 4, JW - 90, JCONTENT_Y + 28 };
	char t[48]; sprintf_s(t, "You Lost  (%d)", g_lostCount);
	SelectObject(mem, font);
	DrawShadow(mem, t, hr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, J_GOLD, J_SHAD);
	if (g_lostCount == 0) {
		RECT er = { 16, JCONTENT_Y + 36, JW - 16, JCONTENT_Y + 62 };
		DrawShadow(mem, "Nothing lost yet.", er, DT_LEFT | DT_TOP, J_DIM, J_SHAD);
		return;
	}
	if (g_lostCount > JLOST_VIS) {
		RECT ur, dn; JLostUp(ur); JLostDn(dn);
		JDrawButton(mem, sfont, ur, "Up");
		JDrawButton(mem, sfont, dn, "Dn");
	}
	SelectObject(mem, sfont);
	for (int i = 0; i < JLOST_VIS; ++i) {
		int idx = g_jLostScroll + i;
		if (idx >= g_lostCount) break;
		RECT nr = { 20, JLostTop(i), JW - 16, JLostTop(i) + JLOST_ROW - 1 };
		DrawShadow(mem, g_lostName[idx], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(228, 212, 206), J_SHAD);
	}
}

static void JournalPaint(HWND hwnd)
{
	PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, JW, JH);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < JH; ++y) {
		float tt = (float)y / (float)JH;
		HBRUSH ln = CreateSolidBrush(RGB((int)(18 + 10 * tt), (int)(22 + 12 * tt), (int)(38 + 16 * tt)));
		RECT lr = { 0, y, JW, y + 1 }; FillRect(mem, &lr, ln); DeleteObject(ln);
	}
	RECT full = { 0, 0, JW, JH };
	HBRUSH bo = CreateSolidBrush(RGB(6, 8, 16)); FrameRect(mem, &full, bo); DeleteObject(bo);
	RECT inner = { 2, 2, JW - 2, JH - 2 };
	HBRUSH bi = CreateSolidBrush(J_LINE); FrameRect(mem, &inner, bi); DeleteObject(bi);

	HFONT font = CreateFontA(-15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT sfont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	RECT tr = { 0, 2, JW, JTITLE_H };
	DrawShadow(mem, "Reward Journal", tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, J_GOLD, J_SHAD);
	{ RECT jx; CloseXRect(JW, jx); DrawCloseX(mem, jx); }

	const char* tn[3] = { "Spell", "AA", "Lost" };
	for (int i = 0; i < 3; ++i) {
		RECT tb; JTab(i, tb);
		bool act = (g_journalTab == i);
		HBRUSH bg = CreateSolidBrush(act ? RGB(34, 44, 72) : RGB(14, 18, 30));
		FillRect(mem, &tb, bg); DeleteObject(bg);
		HBRUSH e = CreateSolidBrush(J_LINE); FrameRect(mem, &tb, e); DeleteObject(e);
		DrawShadow(mem, tn[i], tb, DT_CENTER | DT_VCENTER | DT_SINGLELINE, act ? J_GOLD : J_DIM, J_SHAD);
	}
	HBRUSH dv = CreateSolidBrush(J_LINE);
	RECT dl = { 4, JCONTENT_Y, JW - 4, JCONTENT_Y + 1 }; FillRect(mem, &dl, dv); DeleteObject(dv);

	if (g_journalTab == JTAB_SPELL)     JPaintSpell(mem, font, sfont);
	else if (g_journalTab == JTAB_AA)   JPaintAA(mem, font, sfont);
	else                                JPaintLost(mem, font, sfont);

	BitBlt(hdc, 0, 0, JW, JH, mem, 0, 0, SRCCOPY);
	SelectObject(mem, oldFont);
	DeleteObject(font); DeleteObject(sfont);
	SelectObject(mem, old); DeleteObject(bmp); DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK JournalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT: JournalPaint(hwnd); return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		{ RECT jx; CloseXRect(JW, jx); if (PtInRect(&jx, pt)) { g_journalVisible = false; ShowWindow(hwnd, SW_HIDE); return 0; } }
		if (pt.y >= JTITLE_H && pt.y < JCONTENT_Y) {            // tab bar
			for (int i = 0; i < 3; ++i) { RECT tb; JTab(i, tb); if (PtInRect(&tb, pt)) { g_journalTab = i; InvalidateRect(hwnd, nullptr, FALSE); return 0; } }
		}
		if (g_journalTab == JTAB_LOST && g_lostCount > JLOST_VIS) {  // Lost scroll buttons
			RECT ur, dn; JLostUp(ur); JLostDn(dn); int mx = g_lostCount - JLOST_VIS;
			if (PtInRect(&ur, pt)) { g_jLostScroll -= JLOST_VIS - 1; if (g_jLostScroll < 0) g_jLostScroll = 0; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
			if (PtInRect(&dn, pt)) { g_jLostScroll += JLOST_VIS - 1; if (g_jLostScroll > mx) g_jLostScroll = mx; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
		}
		if (pt.y < JTITLE_H) { g_jDragging = true; g_jDragDX = pt.x; g_jDragDY = pt.y; SetCapture(hwnd); }
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (g_jDragging) {
			POINT c; GetCursorPos(&c);
			g_jPosX = c.x - g_jDragDX; g_jPosY = c.y - g_jDragDY; g_jPositioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_jPosX, g_jPosY, JW, JH, SWP_NOACTIVATE);
			return 0;
		}
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		int row = -1;
		if (g_journalTab == JTAB_SPELL || g_journalTab == JTAB_AA)
			for (int i = 0; i < 3; ++i) if (pt.y >= JRowTop(i) && pt.y < JRowTop(i) + JROW_H) { row = i; break; }
		if (g_journalTab == JTAB_SPELL && row != g_spellDescSel) { g_spellDescSel = row; InvalidateRect(hwnd, nullptr, FALSE); }
		else if (g_journalTab == JTAB_AA && row != g_aaDescSel)  { g_aaDescSel = row; InvalidateRect(hwnd, nullptr, FALSE); }
		return 0;
	}

	case WM_LBUTTONUP: {
		if (g_jDragging) { g_jDragging = false; ReleaseCapture(); return 0; }
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		if (g_journalTab == JTAB_SPELL) {
			for (int i = 0; i < g_count; ++i) { RECT b; JBtn(i, b); if (PtInRect(&b, pt)) { g_pendingPick = i + 1; break; } }
		} else if (g_journalTab == JTAB_AA) {
			for (int i = 0; i < g_aaCount; ++i) { RECT b; JBtn(i, b); if (PtInRect(&b, pt)) { g_aaPendingPick = i + 1; break; } }
		}
		return 0;
	}

	case WM_RBUTTONUP:
		g_journalVisible = false; ShowWindow(hwnd, SW_HIDE);
		return 0;

	case WM_MOUSEWHEEL: {
		if (GetForegroundWindow() != hwnd) SetForegroundWindow(hwnd);  // grab focus so EQ stops zooming
		if (g_journalTab == JTAB_LOST) {
			int d = GET_WHEEL_DELTA_WPARAM(wp);
			g_jLostScroll += (d > 0) ? -1 : 1;
			int mx = g_lostCount - JLOST_VIS; if (mx < 0) mx = 0;
			if (g_jLostScroll < 0) g_jLostScroll = 0;
			if (g_jLostScroll > mx) g_jLostScroll = mx;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;
	}

	case WM_TIMER: {
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool active = ((fg == eq) || IsAoTOverlay(fg)) && IsInGame();
		if (g_journalVisible && active) {
			if (!g_jPositioned && eq) {
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top }; ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
				g_jPosX = tl.x + (cw - JW) / 2;
				g_jPosY = tl.y + (ch - JH) / 2;
				g_jPositioned = true;
			}
			if (g_jPositioned) {
				if (!IsWindowVisible(hwnd)) {
					SetWindowPos(hwnd, HWND_TOPMOST, g_jPosX, g_jPosY, JW, JH, SWP_NOACTIVATE);
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

static DWORD WINAPI JournalThreadProc(LPVOID)
{
	InitAssets();
	HINSTANCE hInst = GetModuleHandleA(nullptr);
	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = JournalWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTRewardJournal";
	RegisterClassExA(&wc);

	g_journalHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTRewardJournal", "", WS_POPUP,
		0, 0, JW, JH, nullptr, nullptr, hInst, nullptr);
	if (!g_journalHwnd) return 0;

	SetLayeredWindowAttributes(g_journalHwnd, 0, 244, LWA_ALPHA);
	SetTimer(g_journalHwnd, 1, 60, nullptr);

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
static const int VW = 560, VTAB_H = 26, VTITLE_H = 22, VHDR_H = 16, VROW_H = 28, VVIS = 9, VFOOT_H = 44;
static const int VLIST_Y = VTAB_H + VTITLE_H + VHDR_H;
static const int VH = VLIST_Y + VVIS * VROW_H + VFOOT_H;
static const char* V_LBL[4] = { "Plat", "Gold", "Slv", "Cop" };

static int  VRowY(int vis)  { return VLIST_Y + vis * VROW_H; }
static int  VCount()        { return g_vendorTab == 0 ? g_vendorCount : g_myCount; }
static int& VScroll()       { return g_vendorTab == 0 ? g_vendorScroll : g_myScroll; }
static void VTabBtn(int t, RECT& r) { int w = (VW - 24 - 22) / 2; r.left = 10 + t * (w + 4); r.right = r.left + w; r.top = 3; r.bottom = VTAB_H - 3; }  // -22 reserves the top-right [X]
// Add-tab: 4 coin fields per row, right-aligned (d = 0 plat .. 3 copper)
static void VFieldX(int d, int& left, int& right) { right = VW - 14 - (3 - d) * 50; left = right - 46; }
static void VField(int vis, int d, RECT& r) { int l, rt; VFieldX(d, l, rt); r.left = l; r.right = rt; r.top = VRowY(vis) + 3; r.bottom = r.top + 22; }
// My-Shop-tab: a Pull button per row
static void VPullBtn(int vis, RECT& r) { r.right = VW - 14; r.left = r.right - 62; r.top = VRowY(vis) + 3; r.bottom = r.top + 22; }
static void VUpBtn (RECT& r) { r.top = VTAB_H + 3; r.bottom = VTAB_H + VTITLE_H - 2; r.right = VW - 46; r.left = r.right - 28; }
static void VDnBtn (RECT& r) { r.top = VTAB_H + 3; r.bottom = VTAB_H + VTITLE_H - 2; r.right = VW - 14; r.left = r.right - 28; }
static void VActBtn(RECT& r) { r.left = 14;         r.right = VW / 2 - 8; r.top = VH - VFOOT_H + 8; r.bottom = VH - 10; }
static void VRefBtn(RECT& r) { r.left = VW / 2 + 8; r.right = VW - 14;    r.top = VH - VFOOT_H + 8; r.bottom = VH - 10; }
static const int VTITLE_TOP = VTAB_H;   // draggable strip = the title row under the tabs

static void VDrawBtn(HDC mem, HFONT f, const RECT& r, const char* s, bool on)
{
	HBRUSH bg = CreateSolidBrush(on ? RGB(70, 110, 70) : RGB(48, 60, 86)); FillRect(mem, (RECT*)&r, bg); DeleteObject(bg);
	HBRUSH ed = CreateSolidBrush(RGB(18, 24, 38)); FrameRect(mem, (RECT*)&r, ed); DeleteObject(ed);
	SelectObject(mem, f);
	DrawShadow(mem, s, r, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(235, 240, 250), RGB(10, 14, 24));
}

static void VendorPaint(HWND hwnd)
{
	PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, VW, VH);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	for (int y = 0; y < VH; ++y) {                          // navy gradient
		float tt = (float)y / (float)VH;
		HBRUSH ln = CreateSolidBrush(RGB((int)(20 + 10 * tt), (int)(26 + 12 * tt), (int)(44 + 18 * tt)));
		RECT lr = { 0, y, VW, y + 1 }; FillRect(mem, &lr, ln); DeleteObject(ln);
	}
	RECT full = { 0, 0, VW, VH };
	HBRUSH bo = CreateSolidBrush(RGB(8, 12, 22)); FrameRect(mem, &full, bo); DeleteObject(bo);
	HBRUSH bi = CreateSolidBrush(RGB(90, 120, 170)); RECT inr = { 2, 2, VW - 2, VH - 2 }; FrameRect(mem, &inr, bi); DeleteObject(bi);

	HFONT font = CreateFontA(-15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT sfont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldF = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	// tab buttons
	RECT t0, t1; VTabBtn(0, t0); VTabBtn(1, t1);
	VDrawBtn(mem, sfont, t0, "Add Items", g_vendorTab == 0);
	VDrawBtn(mem, sfont, t1, "My Shop",   g_vendorTab == 1);
	{ RECT vx; CloseXRect(VW, vx); DrawCloseX(mem, vx); }

	// title / hint strip (draggable)
	RECT tr = { 14, VTITLE_TOP + 1, VW - 100, VTITLE_TOP + VTITLE_H };
	DrawShadow(mem, g_vendorTab == 0 ? "Type a price on each item, then Add Priced Items"
	                                 : "Click Pull to take an item back to your cursor",
		tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(245, 220, 150), RGB(10, 14, 24));
	if (VCount() > VVIS) { RECT ur, dn; VUpBtn(ur); VDnBtn(dn); VDrawBtn(mem, sfont, ur, "Up", false); VDrawBtn(mem, sfont, dn, "Dn", false); }
	int hy = VTAB_H + VTITLE_H;
	RECT dvd = { 10, hy, VW - 10, hy + 1 }; HBRUSH dv = CreateSolidBrush(RGB(90, 120, 170)); FillRect(mem, &dvd, dv); DeleteObject(dv);

	// column headers
	SelectObject(mem, sfont);
	RECT ih = { 16, hy + 1, 240, hy + VHDR_H }; DrawShadow(mem, "Item", ih, DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(170, 190, 220), RGB(10, 14, 24));
	if (g_vendorTab == 0) {
		for (int d = 0; d < 4; ++d) { int l, rt; VFieldX(d, l, rt); RECT hh = { l, hy + 1, rt, hy + VHDR_H };
			DrawShadow(mem, V_LBL[d], hh, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(170, 190, 220), RGB(10, 14, 24)); }
	} else {
		RECT ph = { VW - 250, hy + 1, VW - 90, hy + VHDR_H };
		DrawShadow(mem, "Price", ph, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, RGB(170, 190, 220), RGB(10, 14, 24));
	}

	int cnt = VCount(), scroll = VScroll();
	for (int vis = 0; vis < VVIS; ++vis) {
		int i = scroll + vis; if (i >= cnt) break;
		int y = VRowY(vis);
		if (g_vendorTab == 0) {
			RECT nr = { 16, y + 3, VW - 224, y + VROW_H - 3 };
			SelectObject(mem, sfont);
			DrawShadow(mem, g_vendorNames[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, RGB(225, 232, 245), RGB(10, 14, 24));
			for (int d = 0; d < 4; ++d) {
				RECT fr; VField(vis, d, fr);
				bool focused = (g_vFocusItem == i && g_vFocusCoin == d);
				HBRUSH fb = CreateSolidBrush(focused ? RGB(64, 84, 60) : RGB(26, 34, 52)); FillRect(mem, &fr, fb); DeleteObject(fb);
				HBRUSH fe = CreateSolidBrush(focused ? RGB(150, 200, 140) : RGB(70, 90, 130)); FrameRect(mem, &fr, fe); DeleteObject(fe);
				char nb[16]; sprintf_s(nb, focused ? "%d_" : "%d", g_vCoin[i][d]);
				DrawShadow(mem, nb, fr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(245, 235, 200), RGB(10, 14, 24));
			}
		} else {
			RECT nr = { 16, y + 3, VW - 260, y + VROW_H - 3 };
			SelectObject(mem, sfont);
			DrawShadow(mem, g_myName[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, RGB(225, 232, 245), RGB(10, 14, 24));
			unsigned c = g_myCost[i];
			char pbuf[48]; sprintf_s(pbuf, "%up %ug %us %uc", c / 1000, (c / 100) % 10, (c / 10) % 10, c % 10);
			RECT pr = { VW - 250, y + 3, VW - 90, y + VROW_H - 3 };
			DrawShadow(mem, pbuf, pr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, RGB(245, 235, 200), RGB(10, 14, 24));
			RECT pull; VPullBtn(vis, pull); VDrawBtn(mem, sfont, pull, "Pull", false);
		}
	}
	if (cnt == 0) {
		RECT er = { 16, VLIST_Y + 8, VW - 16, VLIST_Y + 40 };
		SelectObject(mem, sfont);
		DrawShadow(mem, g_vendorTab == 0 ? "No sellable items in your bags (No-Drop/containers can't list)."
		                                 : "Your shop is empty -- add items on the Add Items tab.",
			er, DT_LEFT | DT_TOP, RGB(150, 165, 195), RGB(10, 14, 24));
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
static void VendorDoAdd()
{
	std::string line; int n = 0;
	for (int i = 0; i < g_vendorCount; ++i) {
		unsigned long copper = (unsigned long)g_vCoin[i][0] * 1000 + g_vCoin[i][1] * 100 + g_vCoin[i][2] * 10 + g_vCoin[i][3];
		if (copper == 0) continue;                         // only add items you actually priced
		char seg[40]; sprintf_s(seg, "%d:%lu", g_vendorIds[i], copper);
		if (!line.empty()) line += ",";
		line += seg;
		if (++n >= 10) { std::string c = "/say shopadd " + line; VendorQueue(c.c_str()); line.clear(); n = 0; }
	}
	if (!line.empty()) { std::string c = "/say shopadd " + line; VendorQueue(c.c_str()); }
	VendorQueue("/say shoprefresh");                       // server re-sends SHOPINVDATA + MYSHOPDATA
}

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
		// drag from anywhere across the top strip (tab bar + title) -- tabs/[X]/buttons were checked and
		// returned above, so reaching here means empty top space.
		if (pt.y < VTAB_H + VTITLE_H) { g_vDragging = true; g_vDragDX = pt.x; g_vDragDY = pt.y; SetCapture(hwnd); }
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

// ----------------------------------------------------------------- bazaar vtable diagnostic
// Dump the trader (CBazaarWnd @ 0xD1FCB0) + buyer (CBarterWnd @ 0xF70CF0) window vtables to
// <eqdir>\aotv4_bazaar_vtable.txt so we can read the Activate/Show addresses without Ghidra.
// Triggered by Ctrl+B in ProcessGameEvents (the same reliable hook as the Ctrl+W journal toggle).
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

BOOL __cdecl ProcessGameEvents_Detour();
DETOUR_TRAMPOLINE_EMPTY(BOOL __cdecl ProcessGameEvents_Tramp());
BOOL __cdecl ProcessGameEvents_Detour()
{
	// Spell/AA/Lost are now one tabbed Reward Journal window (the old per-window threads are unused).
	if (g_journalEnabled && !g_journalStarted) {
		g_journalStarted = true;
		CreateThread(nullptr, 0, JournalThreadProc, nullptr, 0, nullptr);
	}
	if (g_portalEnabled && !g_portalOverlayStarted) {
		g_portalOverlayStarted = true;
		CreateThread(nullptr, 0, PortalOverlayThreadProc, nullptr, 0, nullptr);
	}
	if (g_vendorEnabled && !g_vendorStarted) {
		g_vendorStarted = true;
		CreateThread(nullptr, 0, VendorThreadProc, nullptr, 0, nullptr);
	}

	// dispatch ONE queued vendor "/say ..." command per frame (set prices / open / close shop). One
	// per frame -- like the spell-pick dispatch -- so rapid back-to-back /say (vpset then vshop) each
	// actually get sent + arrive at the server in order (sending both same-frame dropped the second).
	if (g_vCmdTail != g_vCmdHead) {
		RunGameCommand(g_vendorCmds[g_vCmdTail]);
		g_vCmdTail = (g_vCmdTail + 1) % 24;
	}

	// Ctrl+W toggles the Reward Journal for review (it also auto-opens on level-up/death). Only
	// fires while EQ (or the journal) is foreground, so it won't trigger while alt-tabbed.
	if (g_journalEnabled) {
		static bool jprev = false;
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		HWND fg = GetForegroundWindow();
		bool focused = (fg == eq) || (g_journalHwnd && fg == g_journalHwnd);
		bool combo = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetAsyncKeyState('W') & 0x8000) != 0);
		if (combo && !jprev && focused) g_journalVisible = !g_journalVisible;
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

	int pick = g_pendingPick;
	if (pick > 0) {
		g_pendingPick = -1;
		g_count = 0;                 // clear the spell offer (server re-sends if more pending)
		char cmd[40];
		sprintf_s(cmd, "/say spellpick %d", pick);
		RunGameCommand(cmd);
	}

	int aapick = g_aaPendingPick;
	if (aapick > 0) {
		g_aaPendingPick = -1;
		g_aaCount = 0;               // clear the AA offer (server re-offers the next pick if budget remains)
		char cmd[40];
		sprintf_s(cmd, "/say aapick %d", aapick);
		RunGameCommand(cmd);
	}

	return ProcessGameEvents_Tramp();
}

// ----------------------------------------------------------------- entry
// The spell and AA windows SHARE the two detours (you can't detour an address twice). Install
// them once; each Enable* just flips its feature flag, which the detour bodies check.
static void InstallChoiceHooks()
{
	if (g_choiceHooksInstalled) return;
	g_choiceHooksInstalled = true;

	// Both detours are pure in-memory byte patches -- safe under DllMain's loader lock.
	// The overlay window(s)/thread(s) are created LATER (first ProcessGameEvents tick), well
	// after load, because window creation under loader lock hard-fails startup.

	// swallow the server's SPELLCHOICEDATA / AACHOICEDATA chat lines (CEverQuest::dsp_chat)
	EzDetour((((DWORD)0x51F1A0 - 0x400000) + baseAddress), &SpellChat_Detour, &SpellChat_Tramp);

	// per-frame main-thread heartbeat: spawn overlay(s) + dispatch picks (ProcessGameEvents)
	EzDetour((((DWORD)0x539E60 - 0x400000) + baseAddress), &ProcessGameEvents_Detour, &ProcessGameEvents_Tramp);
}

void EnableSpellChoiceWindow()
{
	g_spellChoiceEnabled = true;
	InstallChoiceHooks();
}

void EnableAAChoiceWindow()
{
	g_aaChoiceEnabled = true;
	InstallChoiceHooks();
}

void EnablePortalWindow()
{
	g_portalEnabled = true;
	InstallChoiceHooks();
}

void EnableLostWindow()
{
	g_lostEnabled = true;
	InstallChoiceHooks();
}

// The Reward Journal is the single tabbed window (Spell / AA / Lost). It needs every chat
// parser those three used, so it turns them all on; only the journal thread is spawned.
void EnableJournalWindow()
{
	g_journalEnabled     = true;
	g_spellChoiceEnabled = true;
	g_aaChoiceEnabled    = true;
	g_lostEnabled        = true;
	InstallChoiceHooks();
}

// The Set-Up-Shop (vendor) price window. Driven by the server's VENDORDATA chat line; shares the
// dsp_chat + ProcessGameEvents hooks like the other windows.
void EnableVendorWindow()
{
	g_vendorEnabled = true;
	InstallChoiceHooks();
}
