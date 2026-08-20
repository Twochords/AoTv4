// core_allaclone.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 "Allaclone" lookup window. See core_allaclone.h for the module contract.
//
// This is a RENDERING change only: the server side (the "srch" / "srchdet" say handlers and
// Client::SearchDetail) is byte-for-byte what the old GDI overlay talked to. Everything here is
// about letting the client's own UI engine draw the answer instead of drawing it ourselves.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_allaclone.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// The things the server can look up. ⚠️ These strings go on the wire verbatim -- they are the
// <kind> token the server switches on, NOT display labels. The labels are in the XML.
//
// ⚠️⚠️ THE LAST TWO ARE SERVED BY LUA, NOT BY THE C++ SearchList. `quest` and `tracked` are answered
// from quest_catalogue.lua by aotv4_questjournal.lua, which global_player tries BEFORE falling
// through to the native search. Nothing here needs to know that -- the wire format is identical --
// but it is why adding them required no server rebuild.
static const char* AC_KIND[7] = { "item", "npc", "spell", "recipe", "quest", "tracked", "zxp" };
static const int   AC_KIND_N  = 7;
static int         g_kind     = 0;

// The two quest modes, by index into AC_KIND. Named rather than spelled 4/5 inline because both the
// poll and the Track button behave differently in them.
static const int AC_KIND_QUEST   = 4;
static const int AC_KIND_TRACKED = 5;

// ⚠️⚠️ ZONE XP IS A BROWSE MODE, NOT A SEARCH. It is the only kind that is meaningful with an EMPTY
// term -- the whole point is to show every zone by region -- so it deliberately skips the "type
// something first" guard in DoSearch and asks the server for the unfiltered list. Typing still works
// and filters by zone name.
// ⚠️ Its rows carry id 0, including the "== REGION ==" headers, because nothing in the list is
// clickable-through to a detail page. OpenDetail must therefore refuse in this mode: sending
// `srchdet zxp 0` would ask the server about item/npc id 0 in a kind it does not handle.
static const int AC_KIND_ZONEXP = 6;

// ⚠️⚠️ THE TRACKED LIST IS POLLED, AND THIS IS THE ONLY LIVE-PROGRESS MECHANISM. There is no
// item-loss event on the server (only scattered DeleteItemInInventory calls), so progress cannot be
// pushed reliably -- selling or destroying a required item would leave a stale "3/4" forever.
// Re-running the search is cheap because the server caps the tracked list at 5 quests.
// ⚠️ It runs ONLY while the tracked mode is on screen. Section 15 records RoF2 dropping and
// reordering chat bursts, so this must never become a background stream; a closed window is silent.
static const DWORD AC_TRACK_POLL_MS = 3000;
static DWORD g_trackPoll = 0;

// ⚠️⚠️ THIS IS NOT "just match the server" ANY MORE, AND AT 60 IT SILENTLY ATE ROWS.
// The four SEARCH kinds (item/npc/spell/recipe) really do carry `LIMIT 60` in Client::SearchList, so
// 60 matched them exactly. **Zone XP is a BROWSE list, not a search** -- it has no LIMIT by design,
// because "show me everywhere I could hunt" is the whole point of it -- and it currently returns 69
// rows (63 zones + 6 region headers). Everything past the 60th was dropped here with no message, so
// the list simply ended mid-region. Reported from play as the Cabilis section being cut short.
// ⚠️ The real ceiling is the SERVER's chat buffer, not this: Client::Message uses vsnprintf into
// char[4096] (zone/client.cpp:1840), and the zxp payload is about 2,055 bytes today. 100 rows is
// roughly 2,900 bytes, so this stays comfortably inside it while leaving room for the authored table
// to grow. Raising it far beyond that just moves the truncation into the chat buffer, where it is
// silent again -- so the server now appends a visible marker row instead of overflowing.
static const int AC_MAX = 100;
static int    g_count = 0;
static int    g_id  [AC_MAX] = {};
static char   g_name[AC_MAX][96] = { {0} };

static int    g_detailId = -1;                // which row's detail is loaded
static std::string g_detail;                  // STML for the bottom pane

// Last term we actually searched for, so the poll in AllacloneTick can tell "the player is still
// typing" from "the text changed and settled".
static char  g_lastTerm[64] = { 0 };
static DWORD g_termStamp    = 0;

// How long the search box must stop changing before we search. Long enough that typing a word does
// not fire a query per keystroke, short enough to feel immediate.
static const DWORD AC_SETTLE_MS = 450;

// Debug trace to <EQ>\aotv4_allaclone.log. Same reason as every other native window in this dll: when
// one does not appear there is NOTHING in any client log to say why, and a missing EQUI.xml Include,
// an unbuilt dll and a failed CCustomWnd construction all look identical from in game.
static void AcTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_allaclone.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_allaclone.log"); }

	FILE* file = nullptr;
	if (fopen_s(&file, path, "a") || !file) { return; }

	SYSTEMTIME now;
	GetLocalTime(&now);
	fprintf(file, "[%02d:%02d:%02d] ", now.wHour, now.wMinute, now.wSecond);

	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);

	fprintf(file, "\n");
	fclose(file);
}

// Read an editbox. ⚠️ Use the address-mapped METHOD GetWindowTextA and pull the chars out of the
// CXStr it returns -- the raw struct members (->InputText, ->WindowText) do NOT match this RoF2
// build: one reads empty and the other returns a garbage pointer that crashes GetCXStr.
static void AcGetText(CXWnd* w, char* out, size_t n)
{
	if (out && n) { out[0] = 0; }
	if (!w || !out || !n) { return; }
	CXStr  s   = w->GetWindowTextA();
	PCXSTR raw = *(PCXSTR*)&s;
	if (raw) { GetCXStr(raw, out, (int)n); }
}

static void AcSetText(CXWnd* w, const char* s)
{
	if (!w) { return; }
	CXStr v(s ? s : "");
	w->SetWindowTextA(v);
}

// ===================================================================== the window
static class AllacloneWnd* gAllacloneWnd = nullptr;
static void AllacloneEnsureWindow(bool show);

class AllacloneWnd : public CCustomWnd
{
public:
	// ⚠️ NO TabBox here, and that is not an oversight. Native tabs are available on this build and are
	// the right widget when each page has its OWN contents -- but the four kinds share one search box,
	// one result list and one detail pane, so tabs would mean four copies of all three. These are
	// mode buttons, not tabs: they change what the one search searches.
	CButtonWnd* KindB[AC_KIND_N] = { nullptr };
	CXWnd*      Term     = nullptr;   // Editbox
	CButtonWnd* GoB      = nullptr;
	CButtonWnd* TrackB   = nullptr;   // Track / Untrack, quest modes only
	CListWnd*   List     = nullptr;
	CStmlWnd*   Detail   = nullptr;
	CXWnd*      Hint     = nullptr;

	int m_sel = -1;

	AllacloneWnd() : CCustomWnd((char*)"AoTAllacloneWnd")
	{
		SetWndNotification(AllacloneWnd);
		KindB[0] = (CButtonWnd*)GetChildItem("ACW_KindItem");
		KindB[1] = (CButtonWnd*)GetChildItem("ACW_KindNpc");
		KindB[2] = (CButtonWnd*)GetChildItem("ACW_KindSpell");
		KindB[3] = (CButtonWnd*)GetChildItem("ACW_KindRecipe");
		KindB[4] = (CButtonWnd*)GetChildItem("ACW_KindQuest");
		KindB[5] = (CButtonWnd*)GetChildItem("ACW_KindTracked");
		KindB[6] = (CButtonWnd*)GetChildItem("ACW_KindZoneXp");
		Term     = (CXWnd*)     GetChildItem("ACW_Term");
		GoB      = (CButtonWnd*)GetChildItem("ACW_Go");
		TrackB   = (CButtonWnd*)GetChildItem("ACW_Track");
		List     = (CListWnd*)  GetChildItem("ACW_List");
		Detail   = (CStmlWnd*)  GetChildItem("ACW_Detail");
		Hint     = (CXWnd*)     GetChildItem("ACW_Hint");
		MarkKind();
		Refresh();
	}

	// Show which kind is active. There is no reliable "pressed/latched" state we can set on a
	// BDT_Normal button from code on this build, so the LABEL carries it -- crude, but it cannot
	// silently do nothing the way a mis-set style would.
	void MarkKind()
	{
		static const char* LBL[AC_KIND_N] = { "Items", "NPCs", "Spells", "Tradeskills", "Quests", "Tracked", "Zone XP" };
		for (int i = 0; i < AC_KIND_N; ++i) {
			if (!KindB[i]) { continue; }
			char b[32];
			sprintf_s(b, (i == g_kind) ? "> %s <" : "%s", LBL[i]);
			AcSetText((CXWnd*)KindB[i], b);
		}
		ApplyVis();
	}

	// Track is meaningless outside the quest modes, so it is HIDDEN rather than left live -- the same
	// call the Advanced Loot window makes for its group-owned rows. ⚠️ Hidden and not greyed: there is
	// no address-mapped enable setter on this build (eqgame.h maps none and writing a raw ->Enabled
	// member offset is the unreliable-struct-offset trap), but CXWnd::Show IS mapped.
	void ApplyVis()
	{
		const bool quest_mode = (g_kind == AC_KIND_QUEST || g_kind == AC_KIND_TRACKED);
		if (TrackB) {
			((CXWnd*)TrackB)->Show(quest_mode ? 1 : 0, 1);
			AcSetText((CXWnd*)TrackB, (g_kind == AC_KIND_TRACKED) ? "Untrack" : "Track");
		}
	}

	// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
	// SetItemText has no colour of its own and the client draws it BLACK, which on this UI is very
	// nearly invisible.
	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFFFFFFF)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		l->SetItemColor(row, col, colour);
	}

	void Refresh()
	{
		if (List) {
			List->DeleteAll();
			for (int i = 0; i < g_count; ++i) {
				// ⚠️ 0xFF.. -- the leading byte is ALPHA. 0x00RRGGBB is fully TRANSPARENT and the row
				// is drawn but invisible, which reads as "the search returned nothing".
				const COLORREF col = (g_id[i] == g_detailId) ? 0xFFFFE08C : 0xFFE0DCCD;
				const int row = List->AddString(g_name[i], col, (uint32_t)i, nullptr, g_name[i]);
				// ⚠️⚠️ NO ID FOR QUESTS. For items, npcs and spells the id is a real, stable, useful
				// number that players look up. A quest's id is an ARRAY POSITION in a generated
				// catalogue that moves whenever it is regenerated -- meaningless to a player, and
				// actively misleading under a column headed "ID". The cell is left blank instead:
				// CListWnd::SetColumnLabel is not mapped on this build, so the header cannot be
				// retitled per mode, and an empty cell reads as "not applicable" where a number
				// reads as "here is your quest number".
				const bool quest_mode = (g_kind == AC_KIND_QUEST || g_kind == AC_KIND_TRACKED);
				char idb[16];
				if (quest_mode) { idb[0] = 0; }
				else            { sprintf_s(idb, "%d", g_id[i]); }
				Cell(List, row, 1, idb, col);
			}
		}
		SetDetail();
		if (Hint) {
			char h[96];
			if (g_count == 0) { strcpy_s(h, "Type a name and press Search."); }
			else              { sprintf_s(h, "%d result%s -- click one for details.", g_count, g_count == 1 ? "" : "s"); }
			AcSetText(Hint, h);
		}
	}

	void SetDetail()
	{
		if (!Detail) { return; }
		CXStr t(g_detail.empty() ? "<c \"#A0A0A0\">Click a result to see its details.</c>" : g_detail.c_str());
		Detail->SetSTMLText(t, true, nullptr);
	}

	void DoSearch()
	{
		char term[64];
		AcGetText(Term, term, sizeof(term));
		if (!term[0] && g_kind != AC_KIND_ZONEXP) {
			// ⚠️ Record the empty term anyway. The poll fires whenever the box differs from what was
			// last searched, so returning without updating it leaves the two permanently out of step
			// and DoSearch is re-entered every single frame once the settle time has passed.
			g_lastTerm[0] = 0;
			g_termStamp   = 0;
			AcSetText(Hint, "Type something to search for first.");
			return;
		}
		if (g_kind == AC_KIND_ZONEXP && !term[0]) {
			// Browse: ask for the whole list. ⚠️ `*` rather than an empty argument -- the say command
			// is split on whitespace, so a trailing nothing would arrive as a missing parameter. The
			// server sanitizes the term to alphanumerics, so `*` reduces to empty and matches all.
			// ⚠️ g_lastTerm must still be recorded (as empty) for the same reason as above, or the
			// settle-poll re-enters this every frame.
			g_lastTerm[0] = 0;
			g_termStamp   = 0;
			AoTQueueGameCommand("/say srch zxp *");
			AcSetText(Hint, "Loading zones...");
			return;
		}
		strncpy_s(g_lastTerm, sizeof(g_lastTerm), term, _TRUNCATE);
		g_termStamp = 0;                       // consumed; the poll must not fire again for this text

		char cmd[160];
		sprintf_s(cmd, "/say srch %s %s", AC_KIND[g_kind], term);
		AoTQueueGameCommand(cmd);
		AcSetText(Hint, "Searching...");
	}

	// ⚠️ Row index IS the data index -- the list is unfiltered and rebuilt whole. Any future filtered
	// view needs its own row-to-id map instead; getting that wrong opens the detail for the wrong row.
	// GetItemData is not used: it does not reliably return what AddString was given on this build.
	int SelRow(int known_row = -1)
	{
		int row = known_row;
		if (row < 0 && List) { row = List->GetCurSel(); }
		if (row < 0)         { row = m_sel; }
		if (row < 0 || row >= g_count) { return -1; }
		return row;
	}

	// Track the selected quest, or untrack it when we are already looking at the tracked list.
	// ⚠️ The server owns the cap and the validation; this only sends an index it was given. A modified
	// client can ask to track anything, which is why aotv4_questjournal re-checks the id against the
	// catalogue and refuses past MAX_TRACKED rather than trusting us.
	void DoTrack()
	{
		const int i = SelRow(-1);
		if (i < 0) { AcSetText(Hint, "Select a quest first."); return; }
		char cmd[64];
		sprintf_s(cmd, (g_kind == AC_KIND_TRACKED) ? "/say quntrack %d" : "/say qtrack %d", g_id[i]);
		AoTQueueGameCommand(cmd);
		// Re-ask immediately so the list reflects the change without waiting out the poll.
		g_trackPoll = 0;
		if (g_kind == AC_KIND_TRACKED) { AoTQueueGameCommand("/say srch tracked *"); }
	}

	void OpenDetail(int row)
	{
		const int i = SelRow(row);
		if (i < 0) { return; }
		// ⚠️ Zone XP rows are informational and every one carries id 0 -- the region headers AND the
		// zones. Asking the server for `srchdet zxp 0` is a lookup it does not implement, so it would
		// answer nothing and the detail pane would sit on whatever the previous kind left there.
		if (g_kind == AC_KIND_ZONEXP || g_id[i] == 0) { return; }
		char cmd[80];
		sprintf_s(cmd, "/say srchdet %s %d", AC_KIND[g_kind], g_id[i]);
		AoTQueueGameCommand(cmd);
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) { pXWnd()->Show(0, 1); return 1; }

		if (Message == XWM_LCLICK) {
			for (int i = 0; i < AC_KIND_N; ++i) {
				if (pWnd == (CXWnd*)KindB[i]) {
					// Switching kind invalidates the results -- they belong to the old one. Re-run the
					// search rather than leaving stale rows that would open the wrong detail.
					g_kind = i;
					g_count = 0; g_detailId = -1; g_detail.clear(); m_sel = -1;
					MarkKind();
					Refresh();
					DoSearch();
					return 1;
				}
			}
			if (pWnd == (CXWnd*)GoB) { DoSearch(); return 1; }

			if (pWnd == (CXWnd*)TrackB) { DoTrack(); return 1; }
			if (pWnd == (CXWnd*)List && List) {
				// ⚠️ The listbox has NOT committed the new selection yet inside its own notification,
				// so GetCurSel() is one behind here. Cache what the click reports and pass it on.
				m_sel = List->GetCurSel();
				OpenDetail(m_sel);
				return 1;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise.
static void AllacloneEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		AcTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up", pSidlMgr, pWndMgr);
		return;
	}

	if (!gAllacloneWnd) {
		gAllacloneWnd = new AllacloneWnd();
		// A NULL pXWnd here means CCustomWnd could not find the screen "AoTAllacloneWnd" -- i.e.
		// EQUI_AoTAllacloneWnd.xml was never copied, or the <Include> line is missing from EQUI.xml.
		// That is the single most common cause of "nothing happened", and it is otherwise silent.
		AcTrace("EnsureWindow: created AllacloneWnd, pXWnd=%p (NULL means the XML is not loaded)",
		        gAllacloneWnd ? (void*)gAllacloneWnd->pXWnd() : nullptr);
	}
	if (gAllacloneWnd && show) {
		gAllacloneWnd->Refresh();
		if (gAllacloneWnd->pXWnd()) { gAllacloneWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== chat transport
// "SRCHDATA <kind>^id|name^id|name^..."
static bool ParseResults(const char* t)
{
	g_count = 0; g_detailId = -1; g_detail.clear();

	const char* p = strchr(t, '^');            // skip the leading <kind> token
	p = p ? p + 1 : t;
	while (*p && g_count < AC_MAX) {
		const char* bar = strchr(p, '|');
		const char* end = strchr(p, '^');
		if (!bar || (end && bar > end)) { break; }
		g_id[g_count] = atoi(p);
		const char* ns = bar + 1;
		int nl = end ? (int)(end - ns) : (int)strlen(ns);
		if (nl < 0)  { nl = 0; }
		if (nl > 95) { nl = 95; }
		memcpy(g_name[g_count], ns, nl);
		g_name[g_count][nl] = 0;
		++g_count;
		if (!end) { break; }
		p = end + 1;
	}
	return true;
}

// "SRCHDET <kind>|<id>|<line~line~line>"
//
// The server's lines are plain text. They are turned into STML here so the pane reads like a real
// item inspect: first line as the heading, the rest as body. ⚠️ Escape the markup characters or an
// item whose name contains '<' would swallow the rest of the panel.
static void AppendStmlEscaped(std::string& out, const char* s)
{
	for (; s && *s; ++s) {
		if      (*s == '<') { out += "&lt;"; }
		else if (*s == '>') { out += "&gt;"; }
		else if (*s == '&') { out += "&amp;"; }
		else                { out += *s; }
	}
}

static bool ParseDetail(const char* t)
{
	const char* b1 = strchr(t, '|');           if (!b1) { return true; }
	const char* b2 = strchr(b1 + 1, '|');      if (!b2) { return true; }
	g_detailId = atoi(b1 + 1);

	g_detail.clear();
	const char* d = b2 + 1;
	bool first = true;
	while (*d) {
		const char* tilde = strchr(d, '~');
		const int   ll    = tilde ? (int)(tilde - d) : (int)strlen(d);
		std::string line(d, d + (ll < 0 ? 0 : ll));

		if (first) { g_detail += "<c \"#FFE08C\">"; }
		AppendStmlEscaped(g_detail, line.c_str());
		if (first) { g_detail += "</c>"; first = false; }
		g_detail += "<br>";

		if (!tilde) { break; }
		d = tilde + 1;
	}
	return true;
}

bool AllacloneParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }

	if (const char* t = strstr(message, "SRCHDATA ")) {
		ParseResults(t + strlen("SRCHDATA "));
		if (gAllacloneWnd) { gAllacloneWnd->m_sel = -1; gAllacloneWnd->Refresh(); }
		return true;
	}

	// Server-side feedback for a window action (tracking refusals, confirmations). Swallowed and
	// written into the hint line so it never reaches the player's chat log.
	// ⚠️ Tested BEFORE "SRCHDET " would be, but the prefixes are distinct so order is not load
	// bearing here -- unlike the echo filter, where a missing entry silently spams chat.
	if (const char* t = strstr(message, "SRCHMSG ")) {
		if (gAllacloneWnd) { AcSetText(gAllacloneWnd->Hint, t + strlen("SRCHMSG ")); }
		return true;
	}

	if (const char* t = strstr(message, "SRCHDET ")) {
		ParseDetail(t + strlen("SRCHDET "));
		// ⚠️ Refresh(), not just SetDetail(): the row highlight is driven by g_detailId, so the list
		// has to be repainted too or the wrong row stays lit.
		if (gAllacloneWnd) { gAllacloneWnd->Refresh(); }
		return true;
	}

	return false;
}

// ⚠️ Currently BELT AND BRACES: the shop window's echo filter in core_spellwindow.cpp already eats
// any "You say" line containing "srch", and it runs first. This lives here anyway so the module owns
// its own protocol -- if that broader filter is ever tightened, the echoes do not start leaking.
bool AllacloneIsOurEcho(const char* message)
{
	if (!g_enabled || !message) { return false; }
	// ⚠️ EVERY command this window sends must be listed here, or the player watches the window talk
	// to the server in their own chat log. The quest tracking pair was added later than the search
	// pair and is easy to forget precisely because tracking works fine without it -- the only symptom
	// is chat spam.
	return strstr(message, "You say, 'srch ")     != nullptr
	    || strstr(message, "You say, 'srchdet ")  != nullptr
	    || strstr(message, "You say, 'qtrack ")   != nullptr
	    || strstr(message, "You say, 'quntrack ") != nullptr;
}

// ===================================================================== per-frame
// The Editbox gives us no "text committed" notification on this build, so the search box is POLLED:
// once the text has stopped changing for AC_SETTLE_MS, run the search. That is what makes typing a
// name and pausing behave the way a player expects without an Enter key we cannot hook.
//
// ⚠️ Does nothing at all unless the window exists AND is really visible, so a closed window costs
// neither a poll nor a chat line.
void AllacloneTick()
{
	if (!g_enabled || !gAllacloneWnd || !gAllacloneWnd->Term) { return; }
	if (!gAllacloneWnd->pXWnd() || !gAllacloneWnd->pXWnd()->IsReallyVisible()) { return; }

	// ⚠️⚠️ THE TRACKED LIST IGNORES THE SEARCH BOX ENTIRELY and refreshes itself on a timer -- it is
	// "what am I working on", not a query, so making the player type to see it would be wrong. This
	// returns before the settle logic below so a leftover term from another mode cannot re-trigger a
	// search against the wrong kind.
	if (g_kind == AC_KIND_TRACKED) {
		if (!g_trackPoll || GetTickCount() - g_trackPoll >= AC_TRACK_POLL_MS) {
			g_trackPoll = GetTickCount();
			AoTQueueGameCommand("/say srch tracked *");
		}
		return;
	}
	g_trackPoll = 0;

	char now[64];
	AcGetText(gAllacloneWnd->Term, now, sizeof(now));

	if (strcmp(now, g_lastTerm) != 0) {
		// Text differs from what we last searched. Start (or restart) the settle clock.
		if (!g_termStamp) { g_termStamp = GetTickCount(); }
		else if (GetTickCount() - g_termStamp >= AC_SETTLE_MS) {
			gAllacloneWnd->DoSearch();
		}
		return;
	}
	g_termStamp = 0;
}

// ===================================================================== module entry points
// ⚠️ Runs inside DllMain under the loader lock, so it may only set a flag -- creating the window here
// would hard-fail startup with 0xc0000142.
void InitAllaclone() { g_enabled = true; AcTrace("InitAllaclone: module enabled"); }

void AllacloneShow()
{
	AcTrace("AllacloneShow: enabled=%d results=%d", g_enabled ? 1 : 0, g_count);
	if (!g_enabled) { return; }
	AllacloneEnsureWindow(true);
}

void AllacloneOnUiReset()
{
	if (gAllacloneWnd) { delete gAllacloneWnd; gAllacloneWnd = nullptr; }
}
