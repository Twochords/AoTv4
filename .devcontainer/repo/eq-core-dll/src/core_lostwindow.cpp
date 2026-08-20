// core_lostwindow.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 "You Lost" window, rendered by the client's own UI engine.
//
// The old version drew itself: a layered GDI window with hand-painted rows, its own scroll buttons
// and its own hit-testing. That was originally necessary because this client's D3D device cannot be
// hooked -- but a SIDL CCustomWnd sidesteps the whole problem, and looks native because it IS
// native. Same move the level-up picker made. See core_lostwindow.h for the module contract.
//
// Nothing about the wire format changed: the server still sends "LOSTDATA name^name^...". This was
// a client-side change only.
//
// Three things the GDI version had that are simply gone now, because the UI engine does them:
//   - scrolling. The listbox has a real scrollbar, so LOST_VIS, g_lostScroll and the Up/Dn buttons
//     that paged it by hand are all unnecessary.
//   - dragging. Style_Titlebar gives a real title bar you can drag, and it remembers its position.
//   - the idle timeout. The overlay auto-hid after 45 seconds because a click-through layered window
//     that outlives its usefulness is a nuisance; a real window with a close box is not, so the list
//     now stays until dismissed. You died -- you get to read it in your own time.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_lostwindow.h"
#include "core_advloot.h"   // AoTQueueGameCommand

#include <string>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdlib>   // atoi, for the Advancement tab's field parsing

// ===================================================================== state
static bool g_enabled = false;

// ⚠️ THIS IS A PERMANENT HISTORY NOW, not the last death. It used to hold only what LOSTDATA pushed
// at the moment of dying, so the list vanished as soon as it was dismissed or the UI reloaded --
// which is precisely when someone wants to look back at what a death cost them. The server keeps the
// log (death_loss.record) and sends it on request; this holds the whole thing, newest death first.
static const int  LOST_MAX = 400;
static char       g_lostName[LOST_MAX][96] = { {0} };
static int        g_lostWhen[LOST_MAX] = {};       // epoch seconds of the death, 0 = this session's push
static char       g_lostKiller[LOST_MAX][64] = { {0} };   // what killed you, per entry
static int        g_lostCount = 0;
static int        g_lostTotal = 0;             // what the server says it holds, before any cap here

// ---------------------------------------------------------------- deaths, and which are expanded
// A "group" is one death: every entry sharing its timestamp. The book lists the DATES, and a date
// opens to show what that death cost. With 400 entries a flat list is a wall of item names with no
// sense of which death each belongs to; collapsed, it reads like a table of contents.
static const int  LOST_GROUP_MAX = 128;
static int        g_grpStart[LOST_GROUP_MAX] = {};   // first entry index of the death
static int        g_grpLen  [LOST_GROUP_MAX] = {};
static bool       g_grpOpen [LOST_GROUP_MAX] = {};
static int        g_grpN = 0;

// ⚠️ ROW INDEX IS NOT A DATA INDEX -- headers are interleaved with items and a collapsed death
// contributes one row and hides many. Every click has to come back through this map or it acts on
// the wrong entry. Same trap as the spell Known tab and the retired Autoskill Timers page.
static const int  LOST_ROW_MAX = LOST_MAX + LOST_GROUP_MAX;
static int        g_rowGroup[LOST_ROW_MAX] = {};     // >= 0: this row is that death's header; -1: an item
static int        g_rowN = 0;

// A clicked row waiting to be opened or closed. See the click handler: the rebuild cannot happen
// inside the listbox's own notification, so it is applied from LostWindowTick a frame later.
static int        g_pendingToggle = -1;

// ---------------------------------------------------------------- Advancement tab state
// The random AA offer death hands out. The server owns everything real about it: which abilities are
// offered, what they cost, whether you can afford them, and whether picking one is legal. All that
// arrives as two chat lines and all that goes back is an index.
//
//   AACHOICEDATA <banked>^name|icon|cost|cls^name|icon|cost|cls^...
//   AADESCDATA   <desc>^<desc>^<desc>
//   dll -> server: /say aapick <1-based index>
//
// ⚠️⚠️ THE INDEX IS THE ONLY THING SENT, and that is deliberate. The server stores the offer against
// the character and validates the index against it, so a modified client can pick one of the three it
// was actually offered and nothing else -- it cannot name an arbitrary ability or a cost. Same shape
// as the level-up reward picker. Do NOT "improve" this by sending the ability id.
// ⚠️ The offer is STICKY server side: it changes only when a pick is made, so re-showing the window,
// zoning or levelling all re-send the same three. Nothing here needs to cache across sessions.
static const int  AA_MAX = 8;                     // the server offers 3; headroom costs nothing
static int        g_aaBudget = 0;
static char       g_aaName[AA_MAX][96] = { {0} };
static int        g_aaCost[AA_MAX] = {};
static char       g_aaDesc[AA_MAX][512] = { {0} };
static int        g_aaCount = 0;
static int        g_aaSel = -1;                   // cached; GetCurSel is a click behind inside a notify

static void LostEnsureWindow(bool show);

// Rebuild the death list from the flat entries, and open the newest one.
//
// ⚠️ Called whenever the DATA is replaced, never on a mere repaint -- it resets the open/closed state,
// so calling it from Refresh() would slam every death shut each time the list was redrawn.
static void LostBuildGroups()
{
	g_grpN = 0;
	int last = -1;
	for (int i = 0; i < g_lostCount; ++i) {
		if (g_grpN == 0 || g_lostWhen[i] != last) {
			if (g_grpN >= LOST_GROUP_MAX) { break; }
			last = g_lostWhen[i];
			g_grpStart[g_grpN] = i;
			g_grpLen  [g_grpN] = 0;
			g_grpOpen [g_grpN] = false;
			++g_grpN;
		}
		g_grpLen[g_grpN - 1]++;
	}
	// The newest death starts open: you have usually just died, or just opened the book to see what
	// the last one cost. Everything older stays shut until asked for.
	if (g_grpN > 0) { g_grpOpen[0] = true; }
}

// ===================================================================== the window
static class LostWnd* gLostWnd = nullptr;

class LostWnd : public CCustomWnd
{
public:
	CListWnd*   List    = nullptr;
	CXWnd*      Count   = nullptr;
	CButtonWnd* Dismiss = nullptr;

	// Advancement tab
	CListWnd*   AAList  = nullptr;
	CXWnd*      AABank  = nullptr;
	CStmlWnd*   AADesc  = nullptr;
	CButtonWnd* AATrain = nullptr;
	CTabWnd*    Tabs    = nullptr;

	// Bring the Advancement page to the front. Page order is the <Pages> order in
	// EQUI_AoTLostWnd.xml: 0 = LST_BookPage (Death Book), 1 = LST_AAPage (Advancement).
	//
	// ⚠️⚠️ `SetPage` IS OVERLOADED AND ONLY THE int FORM IS ADDRESS-BOUND ON THIS BUILD.
	// EQClasses.h declares both SetPage(CPageWnd*, bool) and SetPage(int, bool), but
	// EQClasses.cpp binds CTabWnd__SetPage_x (0x88D4E0) to the **int** one -- so pass an index,
	// not a page pointer. The pointer form resolves to a different symbol that is not mapped here.
	void ShowAdvancementTab()
	{
		if (Tabs) { Tabs->SetPage(1, true); }
	}

	LostWnd() : CCustomWnd((char*)"AoTLostWnd")
	{
		SetWndNotification(LostWnd);
		List    = (CListWnd*)  GetChildItem("LST_ItemList");
		Count   =              GetChildItem("LST_Count");
		Dismiss = (CButtonWnd*)GetChildItem("LST_Dismiss");
		AAList  = (CListWnd*)  GetChildItem("AAP_List");
		AABank  =              GetChildItem("AAP_Banked");
		AADesc  = (CStmlWnd*)  GetChildItem("AAP_Desc");
		AATrain = (CButtonWnd*)GetChildItem("AAP_Train");
		Tabs    = (CTabWnd*)   GetChildItem("LST_Tabs");
		Refresh();
		RefreshAA();
	}

	static void SetLabel(CXWnd* w, const char* s)
	{
		if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
	}

	void Refresh()
	{
		char buf[64];
		sprintf_s(buf, "%d item%s lost", g_lostTotal ? g_lostTotal : g_lostCount,
		          (g_lostTotal ? g_lostTotal : g_lostCount) == 1 ? "" : "s");
		SetLabel(Count, buf);

		if (!List) { return; }

		List->DeleteAll();
		g_rowN = 0;

		// One header row per death. Click it to open or close that death; only an OPEN one lists its
		// items, so the book reads as a list of dates you can drill into.
		for (int g = 0; g < g_grpN; ++g) {
			const int first = g_grpStart[g];

			// ⚠️ strftime format takes SINGLE percents. Written "%%d %%b" it emits the literal text
			// "%d %b" -- strftime reads "%%" as an escaped percent and then passes the letter
			// through, so the header read "Died %d %b, %H:%M" on screen.
			char when[48];
			if (g_lostWhen[first] > 0) {
				time_t t = (time_t)g_lostWhen[first];
				struct tm lt;
				localtime_s(&lt, &t);
				strftime(when, sizeof(when), "%d %b %H:%M", &lt);
			}
			else { strcpy_s(when, "Just now"); }

			// The killer belongs on the header, not on its own row: it describes the DEATH, and what a
			// player wants to read back is "this is what X cost me". The count is on it too, so a
			// closed death still says how much it took.
			char hdr[224];
			sprintf_s(hdr, "%s  %s  %s  (%d item%s)",
			          g_grpOpen[g] ? "[-]" : "[+]",
			          when,
			          g_lostKiller[first][0] ? g_lostKiller[first] : "killed",
			          g_grpLen[g], g_grpLen[g] == 1 ? "" : "s");

			if (g_rowN < LOST_ROW_MAX) {
				List->AddString(hdr, 0xFF00AAFF, (uint32_t)g, nullptr, hdr);
				g_rowGroup[g_rowN++] = g;
			}

			if (!g_grpOpen[g]) { continue; }

			for (int k = 0; k < g_grpLen[g] && g_rowN < LOST_ROW_MAX; ++k) {
				// Indent so an item is visibly subordinate to its date -- there is one column, so the
				// leading spaces are the whole of the hierarchy.
				char row[112];
				sprintf_s(row, "      %s", g_lostName[first + k]);
				// Colour is 0xAARRGGBB. A muted red, because this is a list of things you no longer have.
				List->AddString(row, 0xFFA0A0FF, (uint32_t)(first + k), nullptr, row);
				g_rowGroup[g_rowN++] = -1;
			}
		}
	}

	// ---------------------------------------------------------------- Advancement tab
	static void SetStml(CStmlWnd* w, const char* s)
	{
		if (!w) { return; }
		CXStr v(s ? s : "");
		w->SetSTMLText(v, true, nullptr);
		w->ForceParseNow();
	}

	void RefreshAA()
	{
		if (g_aaCount > 0) {
			char buf[96];
			sprintf_s(buf, "%d point%s to spend  -  choose one",
			          g_aaBudget, g_aaBudget == 1 ? "" : "s");
			SetLabel(AABank, buf);
		}
		else {
			// ⚠️ The empty state is not an error and must not read like one. The picker stops offering
			// the moment nothing is affordable, which is the NORMAL end of a death's rewards.
			SetLabel(AABank, "Nothing to spend.");
		}

		if (AAList) {
			AAList->DeleteAll();
			for (int i = 0; i < g_aaCount; ++i) {
				char cost[24];
				sprintf_s(cost, "%d", g_aaCost[i]);
				// ⚠️ Colour is 0xAARRGGBB and the ALPHA BYTE IS MANDATORY. 0x00RRGGBB is fully
				// transparent, so the text is drawn and is completely invisible.
				AAList->AddString(g_aaName[i], 0xFFE0DCCD, (uint32_t)i, nullptr, g_aaName[i]);
				// ⚠️ AddString colours COLUMN 0 ONLY. A cell written with SetItemText afterwards has no
				// colour of its own and draws BLACK, so column 1 needs its own SetItemColor.
				// ⚠️ SetItemText takes a CXStr POINTER, not a value -- the same call the Autoskill,
				// Spell Journal and Delve windows all make. Passing by value does not compile.
				CXStr c(cost);
				AAList->SetItemText(i, 1, &c);
				AAList->SetItemColor(i, 1, 0xFFC8963E);
			}
		}

		g_aaSel = (g_aaCount > 0) ? 0 : -1;
		if (g_aaCount > 0 && AAList) { AAList->SetCurSel(0); }
		ShowDesc();
	}

	void ShowDesc()
	{
		if (g_aaCount <= 0) {
			SetStml(AADesc, "Death has nothing further to teach you for now. Points are banked when you "
			                "die, and the offer ends when nothing left is affordable.");
			return;
		}
		const int i = (g_aaSel >= 0 && g_aaSel < g_aaCount) ? g_aaSel : 0;
		char buf[720];
		sprintf_s(buf, "<c \"#C8963E\">%s</c><br><c \"#9AA3B0\">Cost %d of %d banked</c><br><br>%s",
		          g_aaName[i], g_aaCost[i], g_aaBudget, g_aaDesc[i]);
		SetStml(AADesc, buf);
	}

	// Toggle the death whose header was clicked. An item row is not a control -- clicking one does
	// nothing, rather than collapsing the death out from under the cursor.
	void ToggleRow(int row)
	{
		if (row < 0 || row >= g_rowN) { return; }
		const int g = g_rowGroup[row];
		if (g < 0 || g >= g_grpN) { return; }
		g_grpOpen[g] = !g_grpOpen[g];
		Refresh();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}
		if (Message == XWM_LCLICK && pWnd == (CXWnd*)List && List) {
			// ⚠️⚠️ DO NOT TOGGLE HERE. Toggling calls Refresh, which calls DeleteAll -- destroying the
			// listbox's rows from inside the listbox's own click notification, while the client is
			// still walking them. The work is deferred to LostWindowTick instead, one frame later,
			// when nothing is mid-click. (This dll has already crashed once by reaching into a
			// window's internals from a click handler; see the EQMainWnd vtable attempt.)
			//
			// ⚠️ The selection is read AFTER the base class has processed the click -- inside the
			// notification it is still one click behind, which would open the previously selected
			// death instead of the one under the cursor.
			const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
			g_pendingToggle = List->GetCurSel();
			return handled;
		}
		if (Message == XWM_LCLICK && pWnd == (CXWnd*)AAList && AAList) {
			// ⚠️ Same ordering rule as the Death Book list: read the selection AFTER the base class has
			// processed the click, or it is still one row behind and the description shown belongs to
			// whatever was selected previously.
			// ⚠️ Safe to act on immediately, unlike the Death Book toggle: this only rewrites an STML
			// pane. It never touches the listbox, so there is no rebuilding a list from inside its own
			// notification.
			const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
			g_aaSel = AAList->GetCurSel();
			ShowDesc();
			return handled;
		}
		if (Message == XWM_LCLICK && pWnd == (CXWnd*)AATrain) {
			// ⚠️ The cached selection, not a live GetCurSel: clicking the button moves focus off the
			// listbox, and a live read there can come back -1 and silently train nothing.
			const int i = (g_aaSel >= 0 && g_aaSel < g_aaCount) ? g_aaSel : 0;
			if (g_aaCount > 0) {
				char cmd[64];
				sprintf_s(cmd, "/say aapick %d", i + 1);   // the server's index is 1-based
				AoTQueueGameCommand(cmd);
				// ⚠️ Do NOT clear the offer or re-enable anything here. The server answers with a fresh
				// AACHOICEDATA (or stops sending, when nothing is affordable), and that is what updates
				// this tab. Guessing the outcome locally would show a pick that the server may refuse.
			}
			return 1;
		}
		if (Message == XWM_LCLICK && pWnd == (CXWnd*)Dismiss) {
			// ⚠️ CLOSE ONLY -- do NOT clear. This used to wipe the list, which was right when it held just
			// the last death, and is exactly wrong now that it is a permanent record: the whole point is
			// being able to come back and look at what was lost.
			pXWnd()->Show(0, 1);
			return 1;
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// This dll historically drew everything itself, so MQ2's UI managers are not reliably wired --
// CCustomWnd silently bails at "if (!pSidlMgr || !pWndMgr)" and the window never appears. Set them
// from the client globals if unset. EVERY native window in this dll has to do this.
static void LostEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) return;

	if (!gLostWnd) gLostWnd = new LostWnd();
	if (gLostWnd && show) {
		gLostWnd->Refresh();
		gLostWnd->pXWnd()->Show(1, 1);
	}
}

// ===================================================================== chat transport
// "LOSTLOG <chunk> <chunks> <total>^epoch|name^epoch|name^..."  -- the permanent history.
bool LostParseLog(const char* message)
{
	if (!g_enabled || !message) { return false; }

	// Swallow our own "/say lostlog" echo as well, so asking for the history never shows up in chat.
	if (strstr(message, "lostlog") && !strstr(message, "LOSTLOG ")) { return true; }

	const char* t = strstr(message, "LOSTLOG ");
	if (!t) { return false; }
	t += strlen("LOSTLOG ");

	int chunk = 0, chunks = 0, total = 0;
	if (sscanf_s(t, "%d %d %d", &chunk, &chunks, &total) != 3) { return true; }
	const char* body = strchr(t, '^');
	if (!body) { return true; }
	++body;

	if (chunk == 1) { g_lostCount = 0; g_lostTotal = total; }

	std::string cur;
	for (const char* p = body; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_lostCount < LOST_MAX) {
				// "epoch|killer|name"
				const size_t b1 = cur.find('|');
				const size_t b2 = (b1 == std::string::npos) ? std::string::npos : cur.find('|', b1 + 1);
				g_lostWhen[g_lostCount] = (b1 == std::string::npos) ? 0 : atoi(cur.c_str());
				std::string kil, nm;
				if (b2 != std::string::npos) {
					kil = cur.substr(b1 + 1, b2 - b1 - 1);
					nm  = cur.substr(b2 + 1);
				}
				else if (b1 != std::string::npos) { nm = cur.substr(b1 + 1); }
				else { nm = cur; }
				strncpy_s(g_lostKiller[g_lostCount], sizeof(g_lostKiller[0]), kil.c_str(), _TRUNCATE);
				strncpy_s(g_lostName[g_lostCount], sizeof(g_lostName[0]), nm.c_str(), _TRUNCATE);
				++g_lostCount;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') { break; }
		}
		else { cur += *p; }
	}

	// ⚠️ Only once the WHOLE history has arrived. Grouping a partial log would split one death across
	// two chunks into two dates, and rebuilding resets which are open.
	if (chunk >= chunks) {
		LostBuildGroups();
		if (gLostWnd) { gLostWnd->Refresh(); }
	}
	return true;
}

// "LOSTDATA name^name^..."
// ---------------------------------------------------------------- Advancement transport
//
// ⚠️⚠️ THESE TWO LINES ARRIVE SEPARATELY AND IN ORDER, and the split matters. AACHOICEDATA carries
// the offer and RESETS the tab; AADESCDATA fills in the descriptions for the offer that just landed.
// Parsing them the other way round, or rebuilding the list on the description line, would blank the
// costs. AADESCDATA therefore never touches g_aaCount -- it only writes into rows that already exist.
// ⚠️ Both are swallowed (return true) so neither ever reaches the chat window. The dll already
// swallowed these two line types when the old GDI AA window was retired; this gives them a home again
// rather than adding a new hook.
static bool AAParseChoice(const char* message)
{
	const char* t = strstr(message, "AACHOICEDATA ");
	if (!t) { return false; }
	t += strlen("AACHOICEDATA ");

	g_aaCount  = 0;
	g_aaBudget = 0;
	for (int i = 0; i < AA_MAX; ++i) { g_aaName[i][0] = 0; g_aaDesc[i][0] = 0; g_aaCost[i] = 0; }

	// first field is the banked total, then one field per offered ability
	std::string cur;
	bool first = true;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (first) {
				g_aaBudget = atoi(cur.c_str());
				first = false;
			}
			else if (!cur.empty() && g_aaCount < AA_MAX) {
				// name|icon|cost|cls  -- icon and cls are unused here: no per-row icon is settable on
				// this build, and every AA is class-agnostic, so both are parsed past rather than read.
				const char* s = cur.c_str();
				const char* b1 = strchr(s, '|');
				if (b1) {
					const size_t nlen = (size_t)(b1 - s);
					strncpy_s(g_aaName[g_aaCount], sizeof(g_aaName[0]), s,
					          nlen < sizeof(g_aaName[0]) - 1 ? nlen : sizeof(g_aaName[0]) - 1);
					const char* b2 = strchr(b1 + 1, '|');            // past icon
					const char* b3 = b2 ? strchr(b2 + 1, '|') : nullptr;  // past cost
					g_aaCost[g_aaCount] = b2 ? atoi(b2 + 1) : 0;
					(void)b3;
					g_aaCount++;
				}
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') { break; }
		}
		else { cur += *p; }
	}

	if (gLostWnd) { gLostWnd->RefreshAA(); }
	return true;
}

// "AAPOPUP" -- open the window ON the Advancement tab because a point was just EARNED.
//
// ⚠️⚠️ THIS IS A SEPARATE LINE FROM AACHOICEDATA ON PURPOSE, AND MERGING THEM WOULD MAKE THE WINDOW
// UNBEARABLE. AACHOICEDATA is sent every time the offer is (re)shown -- on login, on level up, and
// after every pick -- because the offer is STICKY and has to be re-pushed for the tab to know about
// it at all. Popping on that line would throw the window open on every single login and every level.
// The server sends AAPOPUP only from aa_choice.grant_picks, i.e. only when points were actually
// added, so the window opens exactly when there is something new to spend.
//
// ⚠️ Ordering is guaranteed by the server: grant_picks sends the offer (AACHOICEDATA + AADESCDATA)
// and only then AAPOPUP, so by the time this runs the tab is already populated and the player does
// not see an empty list flash before it fills.
// ⚠️ Swallowed like every other transport line so it never reaches the chat window.
bool AAParsePopup(const char* message)
{
	if (!g_enabled || !message) { return false; }
	if (!strstr(message, "AAPOPUP")) { return false; }

	LostEnsureWindow(true);
	if (gLostWnd) {
		gLostWnd->RefreshAA();
		gLostWnd->ShowAdvancementTab();
	}
	return true;
}

static bool AAParseDesc(const char* message)
{
	const char* t = strstr(message, "AADESCDATA ");
	if (!t) { return false; }
	t += strlen("AADESCDATA ");

	int i = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (i < AA_MAX) {
				strncpy_s(g_aaDesc[i], sizeof(g_aaDesc[0]), cur.c_str(), _TRUNCATE);
			}
			++i;
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') { break; }
		}
		else { cur += *p; }
	}

	// only the description pane changes -- the list is already correct from AACHOICEDATA
	if (gLostWnd) { gLostWnd->ShowDesc(); }
	return true;
}

bool AAParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	if (AAParseChoice(message)) { return true; }
	if (AAParseDesc(message))   { return true; }
	// ⚠️ LAST, and it must stay last: "AAPOPUP" is tested with strstr like the others, so putting it
	// above AACHOICEDATA/AADESCDATA would be harmless today only because none of those strings
	// contain it. Keeping the cheap exact-prefix parsers first also means the common case (an offer
	// being re-sent) never runs this test at all.
	if (AAParsePopup(message))  { return true; }
	return false;
}

bool LostParseTransport(const char* message)
{
	if (!g_enabled || !message) return false;

	const char* t = strstr(message, "LOSTDATA ");
	if (!t) return false;
	t += strlen("LOSTDATA ");

	g_lostCount = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_lostCount < LOST_MAX) {
				strncpy_s(g_lostName[g_lostCount], sizeof(g_lostName[0]), cur.c_str(), _TRUNCATE);
				g_lostWhen[g_lostCount]      = 0;   // 0 groups under "Just now"
				g_lostKiller[g_lostCount][0] = 0;
				g_lostCount++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
		else { cur += *p; }
	}

	LostBuildGroups();

	// Only pop for an actual loss. The server sends this on death, and a death that cost nothing
	// should not throw a window in your face on top of everything else that just happened.
	if (g_lostCount > 0) {
		LostEnsureWindow(true);
	}
	return true;   // swallow the line either way, so it never reaches chat
}

// ===================================================================== module entry points
// ⚠️ This runs inside DllMain (DLL_PROCESS_ATTACH) via InitOptions, under the loader lock, so it may
// do nothing but set a flag. Creating the window here would hard-fail startup with 0xc0000142 --
// LostEnsureWindow is deliberately only ever reached from a chat line or a keypress, both of which
// are well after the loader lock is released.
void InitLostWindow() { g_enabled = true; }

bool LostWindowHasContent() { return g_enabled && g_lostCount > 0; }

void LostWindowShow()
{
	if (!g_enabled) return;
	LostEnsureWindow(true);
	// Pull the permanent history. The live LOSTDATA push only ever carries the death that just
	// happened; everything before it lives on the server.
	AoTQueueGameCommand("/say lostlog");
}

// Applies a pending expand/collapse. This exists ONLY so the listbox is never rebuilt from inside
// its own click notification -- see the click handler. Costs nothing: it is a single integer test
// per frame, and does no work at all unless a header was actually clicked.
void LostWindowTick()
{
	if (!g_enabled || g_pendingToggle < 0) { return; }
	const int row = g_pendingToggle;
	g_pendingToggle = -1;
	if (gLostWnd) { gLostWnd->ToggleRow(row); }
}

void LostWindowOnUiReset()
{
	if (gLostWnd) { delete gLostWnd; gLostWnd = nullptr; }
	g_pendingToggle = -1;   // the row it referred to no longer exists
}
