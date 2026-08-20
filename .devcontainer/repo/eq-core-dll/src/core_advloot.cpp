// core_advloot.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 native SIDL Advanced Loot window (AdvLootWnd), rendered by the client's own UI engine over
// uifiles/default/EQUI_AdvLootWnd.xml. Its own translation unit -- see core_advloot.h for the module
// contract and the chat protocol.
//
// Model (matches live's Advanced Looting System): loot enters your PERSONAL list when you get kill
// credit, NOT when you open a corpse. The list therefore spans every corpse you have rights to, each
// row addressed by "<corpse entity id>:<lootslot>". There is NO range requirement: if you earned the
// kill credit you can loot it from anywhere, so no walking back to corpses.
//
// This module installs NO detours; the dll's existing dsp_chat / InterpretCmd / ProcessGameEvents
// detours call the entry points in core_advloot.h. Window creation is deferred to first use, never
// done from DllMain (loader lock -> 0xc0000142).
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_advloot.h"

#include <string>
#include <cstdio>
#include <cstring>

// ===================================================================== state
static bool g_enabled = false;   // areLootWindowEnabled (set by InitAdvLoot)

static const int LOOT_MAX = 200;
static int  g_lootCount  = 0;
static char g_lootRef   [LOOT_MAX][32];   // "<corpse id>:<lootslot>:<item id>" -- the alspick handle.
                                          // The item id lets the server verify the slot still holds
                                          // what we think before it acts on it.
static int  g_lootIid   [LOOT_MAX];
static int  g_lootIcon  [LOOT_MAX];
static char g_lootName  [LOOT_MAX][64];
static char g_lootNpc   [LOOT_MAX][48];
static int  g_lootQty   [LOOT_MAX];       // stack size for stackables, else 1
static int  g_lootRule  [LOOT_MAX];       // persistent per-item rule: 0 none, 1 AN, 2 AG, 3 NV
static int  g_lootVote  [LOOT_MAX];       // my group vote: 0 none, 1 need, 2 greed, 3 pass
static char g_lootStat  [LOOT_MAX][32];   // group status: "", "Rolling 2", "Won by Bob", "Free Grab"
static unsigned g_lootValue[LOOT_MAX];    // vendor value in copper (0 = worthless / No Drop)
static bool g_applyFilters = true;        // "Apply Filters" checkbox (server is the source of truth)
static bool g_groupByNpc   = false;       // client-side: sort rows by NPC name
static int  g_lootMode     = 0;           // group loot mode: 0 FFA, 1 Master Looter, 2 Need/Greed
static char g_masterLooter[64] = "";      // master looter name (empty when solo)
static char g_detail[2048] = "";          // STML for the item detail panel (from LOOTINFO)
static char g_search[64] = "";             // Edit Filters search text (empty = show everything)

// Listbox column indices -- MUST match the <Columns> order in EQUI_AdvLootWnd.xml.
// The old Loot/Leave columns are gone: they cannot be clicked (no GetCurCol on this build) so they
// were dead space showing "[ ]" that invited clicks which did nothing. Every column here is state.
enum {
	COL_ITEM = 0, COL_VALUE, COL_NPC
};

// Filters tab: the persistent per-character rule list, driven by FILTERDATA. Server-side it is the
// alsnever_<charid> data bucket; a filtered item is simply omitted from LOOTDATA (it stays on the
// corpse and the stock loot window still shows it -- nothing is ever destroyed).
static const int FILT_MAX = 200;
static int  g_lootTab     = 0;            // 0 = Personal Loot, 1 = Filters
static int  g_filterCount = 0;
static int  g_filterIid  [FILT_MAX];
static int  g_filterIcon [FILT_MAX];
static char g_filterName [FILT_MAX][64];
static char g_filterRule [FILT_MAX][20];  // "Always Greed" is 12 chars; a 12-byte buffer truncated it
static unsigned g_filterValue[FILT_MAX];  // vendor value in copper

// ===================================================================== small helpers
// split "a|b|c|..." into up to n '|'-delimited fields; trailing extras fold into the last field, so
// the server can append protocol fields without breaking an older dll.
static void AdvSplit(const std::string& s, std::string* out, int n)
{
	int pi = 0;
	for (size_t k = 0; k < s.size() && pi < n; ++k) {
		if (s[k] == '|') { if (pi + 1 < n) ++pi; }
		else             { out[pi] += s[k]; }
	}
}

// Set a SIDL label. Use the address-mapped METHOD, never raw struct members -- EQClasses.h member
// offsets do not match this RoF2 build (->WindowText reads garbage and crashes GetCXStr).
static void AdvSetLabel(CXWnd* w, const char* s) { if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); } }

// Read an editbox. Use the address-mapped METHOD GetWindowTextA and pull the chars out of the CXStr
// it returns -- raw members like ->InputText / ->WindowText have the wrong offsets on this build and
// reading them returns garbage or crashes (the shop window established this the hard way).
static void AdvGetText(CXWnd* w, char* out, size_t n)
{
	out[0] = 0;
	if (!w) { return; }
	CXStr  s   = w->GetWindowTextA();
	PCXSTR raw = *(PCXSTR*)&s;
	if (raw) { GetCXStr(raw, out, (int)n); }
}

// case-insensitive substring test for the filters search
static bool AdvMatches(const char* name, const char* needle)
{
	if (!needle || !needle[0]) { return true; }
	char n[64], h[64];
	strncpy_s(n, sizeof(n), name,   _TRUNCATE);
	strncpy_s(h, sizeof(h), needle, _TRUNCATE);
	_strlwr_s(n); _strlwr_s(h);
	return strstr(n, h) != nullptr;
}

// copper -> the way EQ writes coin. "-" for anything with no vendor value.
static void AdvCoin(char* out, size_t n, unsigned v)
{
	if (!v)             { strncpy_s(out, n, "-", _TRUNCATE); }
	else if (v >= 1000) { sprintf_s(out, n, "%up %ug", v / 1000, (v / 100) % 10); }
	else if (v >= 100)  { sprintf_s(out, n, "%ug %us", v / 100,  (v / 10)  % 10); }
	else if (v >= 10)   { sprintf_s(out, n, "%us %uc", v / 10,   v % 10); }
	else                { sprintf_s(out, n, "%uc", v); }
}

// forward decls (window lives at the bottom of this file)
static void AdvLootSidlShow();
static void AdvLootRefreshIfOpen();
static void AdvLootHide();
static void AdvLootSetDetail();

// ===================================================================== chat transport
// LOOTDATA <n>^corpseid:lootslot|itemid|icon|name|npcname|qty|locked^...
static bool HandleLootData(const char* msg)
{
	const char* t = strstr(msg, "LOOTDATA ");
	if (!t) return false;
	t += strlen("LOOTDATA ");
	g_lootCount = 0;

	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_lootCount < LOOT_MAX) {
					std::string f[10]; AdvSplit(cur, f, 10);
					strncpy_s(g_lootRef [g_lootCount], 32, f[0].c_str(), _TRUNCATE);
					g_lootIid  [g_lootCount] = atoi(f[1].c_str());
					g_lootIcon [g_lootCount] = atoi(f[2].c_str());
					strncpy_s(g_lootName[g_lootCount], 64, f[3].c_str(), _TRUNCATE);
					strncpy_s(g_lootNpc [g_lootCount], 48, f[4].c_str(), _TRUNCATE);
					int q = atoi(f[5].c_str());
					g_lootQty   [g_lootCount] = (q > 0) ? q : 1;   // absent/0 (older server) -> 1
					g_lootRule  [g_lootCount] = atoi(f[6].c_str());
					g_lootVote  [g_lootCount] = atoi(f[7].c_str());
					strncpy_s(g_lootStat[g_lootCount], 32, f[8].c_str(), _TRUNCATE);
					g_lootValue [g_lootCount] = (unsigned)atoi(f[9].c_str());
					g_lootCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	// Auto Show Loot Window: pop on new loot, otherwise just re-populate if already open (an empty
	// reply is the normal answer to alsrefresh when there is nothing to loot).
	if (g_lootCount > 0) AdvLootSidlShow();
	else                 AdvLootRefreshIfOpen();
	return true;
}

// FILTERDATA <n>^itemid|icon|name|rule^...
static bool HandleFilterData(const char* msg)
{
	const char* t = strstr(msg, "FILTERDATA ");
	if (!t) return false;
	t += strlen("FILTERDATA ");
	g_filterCount = 0;

	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_filterCount < FILT_MAX) {
					std::string f[5]; AdvSplit(cur, f, 5);
					g_filterIid [g_filterCount] = atoi(f[0].c_str());
					g_filterIcon[g_filterCount] = atoi(f[1].c_str());
					strncpy_s(g_filterName[g_filterCount], 64, f[2].c_str(), _TRUNCATE);
					strncpy_s(g_filterRule[g_filterCount], 20, f[3].c_str(), _TRUNCATE);
					g_filterValue[g_filterCount] = (unsigned)atoi(f[4].c_str());
					g_filterCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	AdvLootRefreshIfOpen();
	return true;
}

bool AdvLootParseTransport(const char* message)
{
	if (!g_enabled || !message) return false;
	if (strstr(message, "LOOTCLOSE")) { AdvLootHide(); return true; }
	if (const char* a = strstr(message, "LOOTAPPLY ")) {     // Apply Filters state from the server
		g_applyFilters = (atoi(a + strlen("LOOTAPPLY ")) != 0);
		AdvLootRefreshIfOpen();
		return true;
	}
	if (const char* i = strstr(message, "LOOTINFO ")) {      // item stats for the detail panel
		strncpy_s(g_detail, sizeof(g_detail), i + strlen("LOOTINFO "), _TRUNCATE);
		AdvLootSetDetail();
		return true;
	}
	if (const char* m = strstr(message, "LOOTMODE ")) {      // group loot mode + master looter
		std::string f[2]; AdvSplit(m + strlen("LOOTMODE "), f, 2);
		g_lootMode = atoi(f[0].c_str());
		strncpy_s(g_masterLooter, 64, f[1].c_str(), _TRUNCATE);
		AdvLootRefreshIfOpen();
		return true;
	}
	if (HandleLootData(message))   return true;
	if (HandleFilterData(message)) return true;
	return false;
}

// Our own "/say als..." lines echo back through dsp_chat as  You say, '...'  -- swallow them so they
// never hit the chat window or nearby players.
bool AdvLootIsOurEcho(const char* message)
{
	if (!g_enabled || !message) return false;
	// every command this module sends starts "als" -- one prefix test covers alspick/alsloot/alsroll/
	// alsaward/alsresolve/alsfree/alsmode/alsapply/alsrefresh/alsfilter* without needing a new case
	// each time the protocol grows.
	return strstr(message, "als") != nullptr &&
	       (strstr(message, "alspick")    || strstr(message, "alsloot")   ||
	        strstr(message, "alsrefresh") || strstr(message, "alsfilter") ||
	        strstr(message, "alsinspect") || strstr(message, "alsroll")   ||
	        strstr(message, "alsaward")   || strstr(message, "alsresolve")||
	        strstr(message, "alsfree")    || strstr(message, "alsmode")   ||
	        strstr(message, "alsapply")   || strstr(message, "alssell")   ||
	        strstr(message, "alsinspect"));
}

// ===================================================================== the native window
static class AdvLootWnd* gAdvLootWnd = nullptr;

class AdvLootWnd : public CCustomWnd
{
public:
	CListWnd*   List = nullptr;
	CXWnd*      Hint = nullptr;
	CXWnd*      MasterLooter = nullptr;
	CStmlWnd*   Detail = nullptr;      // item inspect panel (STML, like the stock item display)
	CXWnd*      Search = nullptr;      // filters-view search box
	CButtonWnd* FindBtn = nullptr;
	CButtonWnd* LootAllBtn = nullptr; CButtonWnd* EditFiltersBtn = nullptr;
	CButtonWnd* FilterDelBtn = nullptr;
	CButtonWnd* ApplyFiltersChk = nullptr; CButtonWnd* GroupByNpcChk = nullptr;
	// per-row actions (the list can only select -- see WndNotification)
	CButtonWnd* LootBtn = nullptr; CButtonWnd* LeaveBtn = nullptr;
	CButtonWnd* NeedBtn = nullptr; CButtonWnd* GreedBtn = nullptr; CButtonWnd* PassBtn = nullptr;
	CButtonWnd* SellBtn = nullptr; CButtonWnd* SellAllBtn = nullptr; CButtonWnd* ASellBtn = nullptr;
	CButtonWnd* AnBtn   = nullptr; CButtonWnd* AgBtn    = nullptr; CButtonWnd* NeverBtn = nullptr;

	// Clicking any non-list control clears the listbox selection, so GetCurSel() is -1 by the time a
	// button handler runs. Remember the row on list-click instead (the shop window learned this too).
	int m_sel = -1;

	static void SetVis(CXWnd* w, bool v) { if (w) w->Show(v, true); }

	// A row is "votable" while the group still owns the decision. An empty status means solo/FFA;
	// "Yours" and "Free Grab" mean the decision is settled and it loots normally.
	static bool RowIsVotable(int i)
	{
		if (!g_lootStat[i][0])                          return false;
		if (_stricmp(g_lootStat[i], "Yours")     == 0)  return false;
		if (_stricmp(g_lootStat[i], "Free Grab") == 0)  return false;
		if (_strnicmp(g_lootStat[i], "Won by", 6) == 0) return false;
		return true;
	}

	// Returns the DATA index, not the display row. With "Group by NPCs" on, rows are reordered, so the
	// two differ -- AddString stashes the data index and GetItemData maps back. Getting this wrong
	// would loot whatever item happens to sit at that screen position.
	// display row -> data index. Filled by FillList; see SelRow for why we keep our own map.
	int m_order[LOOT_MAX];
	int m_rows = 0;

	// Read the selection LIVE, then map it through OUR OWN row table.
	//
	// Two separate traps here, both of which have bitten:
	//  1. WndNotification for a list click fires BEFORE the listbox commits the new selection, so
	//     GetCurSel() inside that handler returns the PREVIOUS row. Caching it there left every action
	//     one click behind. So the live value wins and the cache is only a fallback for the opposite
	//     hazard (clicking some controls clears the selection outright, as the shop window hit).
	//  2. GetItemData(row) does NOT reliably return the Data we passed to AddString on this build --
	//     it came back 0 for every row, so every action landed on the top item no matter what was
	//     selected. m_order[] is filled as we add the rows, so the mapping cannot lie.
	// SelRow is called from BUTTON handlers, and by then the listbox has long since committed the
	// selection -- so the live GetCurSel() is correct here and must win. (It is only stale inside the
	// list's OWN click notification, which fires before the commit; that path passes its row in.)
	//
	// The mapping then goes through m_order, never GetItemData: that call does not return the Data we
	// passed to AddString on this build and came back 0 for every row.
	//
	// Both halves are required. Reading live but mapping through GetItemData gave "always the top
	// item"; mapping through m_order but reading the cached row gave "always the previous item".
	int SelRow(int known_row = -1)
	{
		const int cnt = (g_lootTab == 0) ? g_lootCount : g_filterCount;
		int row = known_row;
		if (row < 0 && List) { row = List->GetCurSel(); }
		if (row < 0)         { row = m_sel; }         // a control cleared the selection outright
		if (row < 0 && cnt == 1) { row = 0; }         // single row -> no need to click it first
		if (row < 0 || row >= m_rows) { return -1; }
		const int idx = m_order[row];
		return (idx >= 0 && idx < cnt) ? idx : -1;
	}

	AdvLootWnd() : CCustomWnd((char*)"AdvLootWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(AdvLootWnd);
		List            = (CListWnd*)  GetChildItem("ALW_ItemList");
		Hint            =              GetChildItem("ALW_Hint");
		MasterLooter    =              GetChildItem("ALW_MasterLooter");
		Detail          = (CStmlWnd*)  GetChildItem("ALW_Detail");
		Search          =              GetChildItem("ALW_Search");
		FindBtn         = (CButtonWnd*)GetChildItem("ALW_Find");
		LootAllBtn      = (CButtonWnd*)GetChildItem("ALW_LootAll");
		EditFiltersBtn  = (CButtonWnd*)GetChildItem("ALW_EditFilters");
		FilterDelBtn    = (CButtonWnd*)GetChildItem("ALW_FilterDel");
		ApplyFiltersChk = (CButtonWnd*)GetChildItem("ALW_ApplyFilters");
		GroupByNpcChk   = (CButtonWnd*)GetChildItem("ALW_GroupByNpc");
		// Centre the narrow state columns; left-aligned single glyphs and quantities look broken.
		// (0 = left, 1 = right, 2 = centre in this client's justification enum.)
		if (List) { List->SetColumnJustification(COL_VALUE, 1);   // coin reads better right-aligned
		}
		LootBtn         = (CButtonWnd*)GetChildItem("ALW_Loot");
		LeaveBtn        = (CButtonWnd*)GetChildItem("ALW_Leave");
		NeedBtn         = (CButtonWnd*)GetChildItem("ALW_Need");
		SellBtn         = (CButtonWnd*)GetChildItem("ALW_Sell");
		SellAllBtn      = (CButtonWnd*)GetChildItem("ALW_SellAll");
		ASellBtn        = (CButtonWnd*)GetChildItem("ALW_ASell");
		GreedBtn        = (CButtonWnd*)GetChildItem("ALW_Greed");
		PassBtn         = (CButtonWnd*)GetChildItem("ALW_Pass");
		AnBtn           = (CButtonWnd*)GetChildItem("ALW_AN");
		AgBtn           = (CButtonWnd*)GetChildItem("ALW_AG");
		NeverBtn        = (CButtonWnd*)GetChildItem("ALW_Never");
		Refresh();
	}

	void ApplyVis(int sel_row_override = -1)
	{
		const bool loot = (g_lootTab == 0);

		// Row 1: Loot | Leave | Sell | Never   (Remove replaces Never in the filters view -- the two
		// share that cell in the XML, so exactly one of them must ever be visible.)
		// ⚠️⚠️ LOOT IS WITHHELD WHILE THE GROUP STILL OWNS THE DECISION on the selected row, and
		// LOOT ALL whenever the group is on Master Looter or Need/Greed at all.
		//
		// This is presentation, NOT enforcement. AdvLootManager::CanLoot (zone/advloot.cpp) already
		// refuses both cases server side and answers with a reason, so racing the button never worked
		// -- it just LOOKED like it should, which is worse: a live button that silently does nothing
		// reads as a broken window rather than as a rule. Hiding it says what the rule is.
		// ⚠️ Do NOT be tempted to move the check server-to-client: the dll must stay unable to grant
		// itself loot, or a modified client would simply un-hide the button.
		//
		// ⚠️ HIDDEN, not greyed. There is no address-mapped enable setter on this build -- eqgame.h
		// maps nothing for it and EQClasses.h only declares CXWnd::IsEnabled -- and writing a raw
		// ->Enabled member offset is exactly the unreliable-struct-offset trap the shop window's
		// editbox work proved out (garbage reads, crashes). Show IS mapped.
		//
		// ⚠️ FFA is left alone. There the server permits looting and first-come IS the rule, so
		// hiding the button there would break ordinary group play rather than protect it.
		// ⚠️ The row is PASSED IN from the list's click handler, because GetCurSel() is one click
		// behind inside that notification -- the same trap SelRow documents. From every other caller
		// the selection has committed and the live read is right. Same shape, same reason.
		int visSel = sel_row_override;
		if (visSel < 0 && List && g_lootTab == 0) { visSel = List->GetCurSel(); }
		const int  visIdx     = (visSel >= 0 && visSel < m_rows) ? m_order[visSel] : -1;
		const bool rowPending = (visIdx >= 0 && RowIsVotable(visIdx));

		SetVis((CXWnd*)LootBtn,      loot && !rowPending);
		SetVis((CXWnd*)LeaveBtn,     loot);
		SetVis((CXWnd*)SellBtn,      loot);
		SetVis((CXWnd*)NeverBtn,     loot);
		SetVis((CXWnd*)FilterDelBtn, !loot);
		SetVis((CXWnd*)Search,  !loot);   // search + Find only make sense over the rule list
		SetVis((CXWnd*)FindBtn, !loot);

		// Row 2: Alw Need | Alw Greed | Alw Sell | Edit Filters -- all four stay up in both views,
		// because in the filters view they RE-ASSIGN the selected rule.
		SetVis((CXWnd*)AnBtn,          true);
		SetVis((CXWnd*)AgBtn,          true);
		SetVis((CXWnd*)ASellBtn,       true);
		SetVis((CXWnd*)EditFiltersBtn, true);

		// Row 3: the group roll votes, only while the group is actually on Need/Greed.
		const bool roll = loot && (g_lootMode == 2);
		SetVis((CXWnd*)NeedBtn, roll); SetVis((CXWnd*)GreedBtn, roll); SetVis((CXWnd*)PassBtn, roll);

		// Loot All would sweep contested rows along with settled ones, so it is withheld for the whole
		// session under any group loot mode, not per row.
		SetVis((CXWnd*)LootAllBtn,   loot && g_lootMode == 0);
		SetVis((CXWnd*)SellAllBtn,   loot);
		// The checkbox GLYPH cannot be driven from here: ->Checked exists in EQUIStructs.h but raw
		// member offsets do not match this RoF2 build (the shop window's editbox work proved that --
		// garbage reads and crashes), so writing it could corrupt the wnd. That left the box showing
		// whatever the client felt like while the server held the real setting, which made "Never"
		// look broken whenever Apply Filters was actually off. Put the state in the LABEL instead,
		// which is set through the address-mapped SetWindowTextA and is always truthful.
		AdvSetLabel((CXWnd*)ApplyFiltersChk, g_applyFilters ? "Apply Filters: ON" : "Apply Filters: OFF");
		AdvSetLabel((CXWnd*)GroupByNpcChk,   g_groupByNpc   ? "Group by NPC: ON"  : "Group by NPC: OFF");
	}

	// Put text in a cell (SetItemText takes a CXStr*, so it needs a named local).
	void Cell(int row, int col, const char* s, unsigned long colour)
	{
		CXStr v(s);
		List->SetItemText(row, col, &v);
		List->SetItemColor(row, col, colour);
	}

	void FillList()
	{
		if (!List) return;
		List->DeleteAll();

		// Column widths are re-set per view. SetColumnLabel is NOT mapped on this build so the three
		// headers are fixed, but SetColumnWidth IS -- so in the filters view the NPC column is
		// collapsed to zero (a rule has no NPC) and its space is given to the item name, which was
		// otherwise truncating "Rusty Broad Sword [Always Sell]" mid-rule.
		if (g_lootTab == 0) {
			List->SetColumnWidth(COL_ITEM,  200);
			List->SetColumnWidth(COL_VALUE,  70);
			List->SetColumnWidth(COL_NPC,   140);
		}
		else {
			List->SetColumnWidth(COL_ITEM,  340);
			List->SetColumnWidth(COL_VALUE,  70);
			List->SetColumnWidth(COL_NPC,     0);
		}

		if (g_lootTab == 0) {
			// "Group by NPCs": live sorts the list by the owning NPC. Build a display order rather than
			// touching the arrays, so g_lootRef[] indices stay valid for the pick commands.
			int* order = m_order;   // record the mapping for SelRow
			for (int i = 0; i < g_lootCount; ++i) order[i] = i;
			if (g_groupByNpc) {
				for (int a = 1; a < g_lootCount; ++a) {          // insertion sort; the list is short
					const int key = order[a];
					int b = a - 1;
					while (b >= 0 && _stricmp(g_lootNpc[order[b]], g_lootNpc[key]) > 0) {
						order[b + 1] = order[b];
						--b;
					}
					order[b + 1] = key;
				}
			}

			// Live puts a clickable button in each decision cell; our CListWnd binding has no per-cell
			// wnd setter, so the cells carry state glyphs and the buttons below the list do the acting.
			// Colours follow live: green loot, amber leave, red never.
			m_rows = g_lootCount;
			for (int oi = 0; oi < g_lootCount; ++oi) {
				const int           i   = order[oi];
				const unsigned long dim = 0xFFE0DCCD;

				// The tooltip carries the full item name, so a name clipped by the column is still
				// readable on hover without pushing a link into chat.
				char tip[96];
				sprintf_s(tip, "%s  (from %s)", g_lootName[i], g_lootNpc[i]);

				// Item name is column 0 now. A live rule and any group decision are appended to it
				// rather than getting their own columns -- empty columns just to echo the native
				// layout looked like padding.
				char nm[160];
				strncpy_s(nm, 160, g_lootName[i], _TRUNCATE);
				switch (g_lootRule[i]) {
					case 1: strncat_s(nm, "   [Always Need]",  _TRUNCATE); break;
					case 2: strncat_s(nm, "   [Always Greed]", _TRUNCATE); break;
					case 3: strncat_s(nm, "   [Never]",        _TRUNCATE); break;
					case 4: strncat_s(nm, "   [Always Sell]",  _TRUNCATE); break;
					default: break;
				}
				if (g_lootStat[i][0]) {
					strncat_s(nm, "   ", _TRUNCATE);
					strncat_s(nm, g_lootStat[i], _TRUNCATE);
				}

				CXStr first(nm);
				int   row = List->AddString(first, dim, (uint32_t)i, nullptr, tip);

				// vendor value, formatted the way EQ writes coin
				char val[32]; AdvCoin(val, sizeof(val), g_lootValue[i]);
				Cell(row, COL_VALUE, val, g_lootValue[i] ? 0xFFE0C86E : 0xFF6A6A6A);

				Cell(row, COL_NPC, g_lootNpc[i], dim);
			}
		}
		else {
			// Edit Filters view: the saved rules, reusing the same single listbox (a second Listbox in
			// one SIDL file has caused UI-load errors here before).
			//
			// The rule goes in the ITEM column with the name, not in a later column: this build does
			// not map CListWnd::SetColumnLabel, so the headers permanently read Item Name / Value /
			// NPC Name and putting "Always Greed" under "NPC Name" just looked broken.
			// Sorted alphabetically and narrowed by the search box. A rule list grows without bound
			// (35+ entries already), so arrival order made it unscannable.
			int ord[FILT_MAX], cnt = 0;
			for (int i = 0; i < g_filterCount; ++i) {
				if (AdvMatches(g_filterName[i], g_search)) { ord[cnt++] = i; }
			}
			for (int a = 1; a < cnt; ++a) {                  // insertion sort by item name
				const int key = ord[a];
				int b = a - 1;
				while (b >= 0 && _stricmp(g_filterName[ord[b]], g_filterName[key]) > 0) {
					ord[b + 1] = ord[b];
					--b;
				}
				ord[b + 1] = key;
			}

			m_rows = cnt;
			for (int oi = 0; oi < cnt; ++oi) {
				const int i = ord[oi];
				m_order[oi] = i;
				char nm[160];
				sprintf_s(nm, "%s   [%s]", g_filterName[i], g_filterRule[i]);
				CXStr s(nm);
				int   row = List->AddString(s, 0xFFE0DCCD, (uint32_t)oi, nullptr, g_filterName[i]);
				char fv[32]; AdvCoin(fv, sizeof(fv), g_filterValue[i]);
				Cell(row, COL_VALUE, fv, g_filterValue[i] ? 0xFFE0C86E : 0xFF6A6A6A);
				Cell(row, COL_NPC,   "", 0xFFE0DCCD);
			}
		}

		const int cnt = (g_lootTab == 0) ? g_lootCount : g_filterCount;
		if (m_sel >= 0 && m_sel < cnt) List->SetCurSel(m_sel);
	}

	void Refresh()
	{
		char hint[160];
		if (g_lootTab == 0) {
			sprintf_s(hint, "Personal Loot  (%d)", g_lootCount);
		}
		else {
			sprintf_s(hint, "Loot Filters  (%d)    select a rule, then Remove", g_filterCount);
		}
		AdvSetLabel(Hint, hint);

		// group context readout: mode + master looter (blank when solo)
		char ml[96] = "";
		if (g_lootMode == 1)      sprintf_s(ml, "Master Looter: %s", g_masterLooter);
		else if (g_lootMode == 2) sprintf_s(ml, "Need/Greed   (ML: %s)", g_masterLooter);
		AdvSetLabel(MasterLooter, ml);

		ApplyVis();
		FillList();
	}

	// send "/say alspick <ref> <action>" for the selected Personal Loot row, then resync
	void Pick(const char* action)
	{
		const int s = SelRow();
		if (s < 0) {
			// nothing selected: say so rather than silently doing nothing
			AdvSetLabel(Hint, "Select an item in the list first.");
			return;
		}

		// In the Edit Filters view the same rule buttons RE-ASSIGN the selected rule, which is how you
		// change a previous choice. (A per-row dropdown is not reachable: a listbox cell cannot host a
		// combo here, same limitation as the per-cell buttons.)
		if (g_lootTab == 1) {
			int rule = -1;
			if      (!strcmp(action, "an"))    { rule = 1; }
			else if (!strcmp(action, "ag"))    { rule = 2; }
			else if (!strcmp(action, "never")) { rule = 3; }
			else if (!strcmp(action, "asell")) { rule = 4; }
			if (rule > 0) {
				char fc[64];
				sprintf_s(fc, "/say alsfilterset %d %d", g_filterIid[s], rule);
				AoTQueueGameCommand(fc);
				AoTQueueGameCommand("/say alsfilters");
			}
			return;
		}

		char c[64];
		sprintf_s(c, "/say alspick %s %s", g_lootRef[s], action);
		AoTQueueGameCommand(c);
		AoTQueueGameCommand("/say alsrefresh");
		// a Loot removes the row entirely, so a held selection would point at a different item
		if (List) { List->ClearAllSel(); }
		m_sel = -1;
	}

	// cast a group Need/Greed vote on the selected row ("nd" / "gd" / "no")
	void Roll(const char* vote)
	{
		const int s = SelRow();
		if (g_lootTab != 0 || s < 0) return;
		char c[64];
		sprintf_s(c, "/say alsroll %s %s", g_lootRef[s], vote);
		AoTQueueGameCommand(c);
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) { pXWnd()->Show(0, 1); return 1; }

		// Right-click a row to INSPECT it -- the server replies with a normal item link in chat, which
		// clicks through to the stock item display. Matches live, where right-click inspects.
		if (Message == XWM_RCLICK && pWnd == (CXWnd*)List && g_lootTab == 0) {
			int rr = (int)(intptr_t)unk;
			if (rr < 0 || rr >= m_rows) rr = List->GetCurSel();
			const int s = SelRow(rr);
			if (s >= 0) {
				char c[64];
				sprintf_s(c, "/say alsinspect %s", g_lootRef[s]);
				AoTQueueGameCommand(c);
			}
			return 1;
		}

		if (Message == XWM_LCLICK) {
			// The list only SELECTS a row. Live makes each decision CELL clickable, which this build
			// cannot do: CListWnd::GetCurCol is declared in EQClasses.h but has no address here
			// (CListWnd__GetCurCol_x is undefined -> unresolved external at link), and the
			// (point, androw, andcol) GetItemAtPoint overload is unmapped too. The buttons below the
			// list carry the actions instead; they need only GetCurSel + GetItemData, which are mapped.
			// Clicking the already-selected row clears the highlight, so a selection is never stuck on
			// with no way to drop it (the client keeps the row selected until told otherwise).
			if (pWnd == (CXWnd*)List) {
				// The clicked row arrives in the notification's third parameter. Prefer it over
				// GetCurSel(): this handler runs BEFORE the listbox commits the selection, so
				// GetCurSel() here still reports the previous row -- which is why actions kept
				// landing on the wrong item. Fall back to GetCurSel only if it looks bogus.
				int row = (int)(intptr_t)unk;
				if (row < 0 || row >= m_rows) { row = List->GetCurSel(); }
				m_sel = (row >= 0 && row < m_rows) ? row : -1;

				// Selecting a row fills the detail panel, so inspecting is just clicking the item --
				// no right-click, and nothing lands in chat. The filters view lists RULES rather than
				// corpse contents, so those rows have no corpse:slot handle and inspect by item id.
				const int s = SelRow(m_sel);
				if (s >= 0) {
					char c[64];
					if (g_lootTab == 0) { sprintf_s(c, "/say alsinspect %s", g_lootRef[s]); }
					else                { sprintf_s(c, "/say alsinspectitem %d", g_filterIid[s]); }
					AoTQueueGameCommand(c);
				}

				// Loot hides or comes back with the newly selected row's group state. ⚠️ m_sel is
				// passed explicitly: GetCurSel() is still the PREVIOUS row inside this handler, so a
				// live read here would show the button one click behind -- briefly offering Loot on a
				// row that is still rolling, which is the exact thing this is meant to prevent.
				// ⚠️ Safe to call from inside the list's own notification: ApplyVis only touches
				// BUTTONS and LABELS, never the listbox. Rebuilding the list here would be the Death
				// Book crash.
				ApplyVis(m_sel);
				return 1;
			}

			if (pWnd == (CXWnd*)LootBtn)   { Pick("loot");  return 1; }
			if (pWnd == (CXWnd*)LeaveBtn)  { Pick("leave"); return 1; }
			if (pWnd == (CXWnd*)NeedBtn)   { Roll("nd");    return 1; }
			if (pWnd == (CXWnd*)GreedBtn)  { Roll("gd");    return 1; }
			if (pWnd == (CXWnd*)PassBtn)   { Roll("no");    return 1; }
			if (pWnd == (CXWnd*)AnBtn)     { Pick("an");    return 1; }
			if (pWnd == (CXWnd*)AgBtn)     { Pick("ag");    return 1; }
			if (pWnd == (CXWnd*)NeverBtn)  { Pick("never"); return 1; }
			if (pWnd == (CXWnd*)SellBtn)   { Pick("sell");  return 1; }
			if (pWnd == (CXWnd*)ASellBtn)  { Pick("asell"); return 1; }
			if (pWnd == (CXWnd*)SellAllBtn) { AoTQueueGameCommand("/say alssellall"); AoTQueueGameCommand("/say alsrefresh"); return 1; }
			if (pWnd == (CXWnd*)FindBtn) { AdvGetText(Search, g_search, sizeof(g_search)); m_sel = -1; Refresh(); return 1; }
			if (pWnd == (CXWnd*)LootAllBtn) {
				if (g_lootTab == 0) { AoTQueueGameCommand("/say alslootall"); AoTQueueGameCommand("/say alsrefresh"); }
				return 1;
			}
			if (pWnd == (CXWnd*)EditFiltersBtn) {           // toggles between the loot list and the rules
				g_lootTab  = (g_lootTab == 0) ? 1 : 0;
				g_detail[0] = 0;                 // don't leave the previous view's item in the panel
				AdvLootSetDetail();
				m_sel     = -1;
				AoTQueueGameCommand(g_lootTab == 1 ? "/say alsfilters" : "/say alsrefresh");
				Refresh();
				return 1;
			}
			if (pWnd == (CXWnd*)FilterDelBtn) {
				const int s = SelRow();
				if (g_lootTab == 1 && s >= 0) {
					char c[64];
					sprintf_s(c, "/say alsfilterdel %d", g_filterIid[s]);
					AoTQueueGameCommand(c);
					AoTQueueGameCommand("/say alsfilters");
				}
				return 1;
			}
			if (pWnd == (CXWnd*)ApplyFiltersChk) {
				g_applyFilters = !g_applyFilters;
				AoTQueueGameCommand(g_applyFilters ? "/say alsapply 1" : "/say alsapply 0");
				return 1;
			}
			if (pWnd == (CXWnd*)GroupByNpcChk) {            // purely a client-side sort of the same rows
				g_groupByNpc = !g_groupByNpc;
				Refresh();
				return 1;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// This dll historically drew everything itself (GDI overlays), so MQ2's UI managers are not reliably
// wired -- CCustomWnd would silently bail at "if (!pSidlMgr || !pWndMgr)" and the window would never
// appear. Set them from the client globals if unset. Every native-window feature here must do this.
static void EnsureAdvLootWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) return;

	if (!gAdvLootWnd) gAdvLootWnd = new AdvLootWnd();
	if (gAdvLootWnd && show) { gAdvLootWnd->Refresh(); gAdvLootWnd->pXWnd()->Show(1, 1); }
}

static void AdvLootSidlShow()      { EnsureAdvLootWindow(true); }
static void AdvLootRefreshIfOpen() { if (gAdvLootWnd) gAdvLootWnd->Refresh(); }
static void AdvLootSetDetail()
{
	if (!gAdvLootWnd || !gAdvLootWnd->Detail) return;
	// SetSTMLText(text, bParse, SLinkInfo*) -- the address-mapped method; the client parses the
	// markup itself, which is what makes this read like a real item inspect.
	CXStr t(g_detail);
	gAdvLootWnd->Detail->SetSTMLText(t, true, nullptr);
}
static void AdvLootHide()          { if (gAdvLootWnd) gAdvLootWnd->pXWnd()->Show(0, 1); }


// Per-frame. The Edit Filters search box has no change notification we can hook, so poll it and
// rebuild the list the moment the text differs from what we last rendered. Cheap: it does nothing at
// all unless the window is up AND the filters view is the one showing.
void AdvLootTick()
{
	if (!g_enabled || !gAdvLootWnd || g_lootTab != 1) { return; }
	if (!gAdvLootWnd->Search) { return; }

	char now[64];
	AdvGetText(gAdvLootWnd->Search, now, sizeof(now));
	if (strcmp(now, g_search) != 0) {
		strncpy_s(g_search, sizeof(g_search), now, _TRUNCATE);
		gAdvLootWnd->m_sel = -1;          // the row under the cursor is about to mean something else
		gAdvLootWnd->Refresh();
	}
}

// ===================================================================== module entry points
void InitAdvLoot() { g_enabled = true; }

// "/advl": open, and resync so a reopen never shows rows from corpses that have since decayed.
void AdvLootShow()
{
	if (!g_enabled) return;
	AdvLootSidlShow();
	AoTQueueGameCommand("/say alsrefresh");
}

// The UI was torn down (CleanGameUI / ReloadUI) -- our child pointers are dangling, drop the object.
void AdvLootOnUiReset()
{
	if (gAdvLootWnd) { delete gAdvLootWnd; gAdvLootWnd = nullptr; }
}
