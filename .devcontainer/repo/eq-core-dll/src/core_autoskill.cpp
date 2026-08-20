// core_autoskill.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Autoskill window. See core_autoskill.h for the module contract.
//
// The server already owned everything that MATTERS here before this file existed -- which skills
// you have, whether each is on, and firing them. This window only makes that state visible and
// clickable, and runs the reuse countdown locally so the server does not have to stream it.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_autoskill.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

static const int SK_MAX = 24;
static int    g_skCount = 0;
static int    g_skId     [SK_MAX] = {};
static char   g_skName   [SK_MAX][48] = { {0} };
static int    g_skOn     [SK_MAX] = {};
static float  g_skCd     [SK_MAX] = {};   // seconds remaining, ticked down locally
static int    g_skReuse  [SK_MAX] = {};   // nominal reuse, for the proportion

static DWORD  g_lastTick = 0;

// How often the open window re-asks the server for truth. Slow on purpose -- see AutoSkillTick.
// ⚠️ 1200 ms, not 2500. Bash and Kick reuse in about FIVE SECONDS (BashReuseTime = 5,
// common/features.h), so a 2.5 s resync could see a cooldown start and finish between two samples --
// which is exactly why the Timers page looked like it was not tracking anything. The countdown is
// still ticked LOCALLY every frame between resyncs; this only controls how often the server's
// authoritative value is re-asked for, and it costs one chat line per interval only while the
// window is open.
static const DWORD AUTOSKILL_RESYNC_MS = 1200;

// Mirror of AOTV4_AUTOSKILL_MAX (zone/special_attacks.cpp). The SERVER is authoritative -- this only
// stops the window asking for something it knows will be refused.
static const int AUTOSKILL_CAP = 4;

// Debug trace to <EQ>\aotv4_autoskill.log. Same idea as native_achievements.log, and for the same
// reason: when a native window does not appear there is NOTHING in any client log to say why -- a
// missing EQUI.xml Include, an unbuilt dll and a failed CCustomWnd construction all look identical
// from in game. This tells the three apart in one line each.
static void AskTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_autoskill.log", gszEQPath); }
	else             { strcpy_s(path, "aotv4_autoskill.log"); }

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

static void AutoSkillEnsureWindow(bool show);

static void SkSplit(const std::string& s, std::string* out, int n)
{
	int f = 0; std::string cur;
	for (size_t i = 0; i <= s.size(); ++i) {
		if (i == s.size() || s[i] == '|') {
			if (f < n) { out[f++] = cur; }
			cur.clear();
		}
		else { cur += s[i]; }
	}
	while (f < n) { out[f++].clear(); }
}

// ===================================================================== the window
static class AutoSkillWnd* gAutoSkillWnd = nullptr;

class AutoSkillWnd : public CCustomWnd
{
public:
	// ONE page, one listbox. The Timers page was removed: a five second reuse is over before a number
	// in a table can be read, and the button grid that replaced it was not wanted either. The cap on
	// how many abilities may fire at once is what actually makes the choice interesting, and that is
	// enforced server side (AOTV4_AUTOSKILL_MAX, zone/special_attacks.cpp).
	CListWnd*   List     = nullptr;   // skill + on/off
	CButtonWnd* EnableB  = nullptr;
	CButtonWnd* DisableB = nullptr;
	CButtonWnd* AllOnB   = nullptr;
	CButtonWnd* AllOffB  = nullptr;
	CButtonWnd* RefreshB = nullptr;

	int m_sel   = -1;   // see SelRow: only a FALLBACK, never the primary source
	int m_rows  = 0;

	AutoSkillWnd() : CCustomWnd((char*)"AoTAutoSkillWnd")
	{
		SetWndNotification(AutoSkillWnd);
		List     = (CListWnd*)  GetChildItem("ASK_SkillList");
		EnableB  = (CButtonWnd*)GetChildItem("ASK_Enable");
		DisableB = (CButtonWnd*)GetChildItem("ASK_Disable");
		AllOnB   = (CButtonWnd*)GetChildItem("ASK_AllOn");
		AllOffB  = (CButtonWnd*)GetChildItem("ASK_AllOff");
		RefreshB = (CButtonWnd*)GetChildItem("ASK_Refresh");
		Refresh();
	}

	// Read the selection LIVE, falling back to the cache.
	//
	// ⚠️ Both halves are needed and both have bitten in the loot window. A list click's
	// WndNotification fires BEFORE the listbox commits the new selection, so GetCurSel() is one
	// behind INSIDE that handler -- which is why the list passes its own row in. From a BUTTON
	// handler the selection has long since committed, so the live value is correct and must win;
	// the cache only covers the opposite hazard, where clicking a control clears the selection.
	//
	// ⚠️ Row index IS the data index here, because the list shows every skill unfiltered. Any FILTERED
	// list needs its own row-to-index map instead -- the retired Timers page did, and getting that
	// wrong toggles the wrong ability.
	// GetItemData is not used at all: it does not reliably return what AddString was given on this build.
	int SelRow(int known_row = -1)
	{
		int row = known_row;
		if (row < 0 && List) { row = List->GetCurSel(); }
		if (row < 0)         { row = m_sel; }
		if (row < 0 && g_skCount == 1) { row = 0; }   // one skill: no need to click it first
		if (row < 0 || row >= g_skCount) { return -1; }
		return row;
	}

	// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
	// SetItemText has no colour of its own and the client draws it BLACK, which on this UI is very
	// nearly invisible. Every extra column has to be coloured explicitly.
	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFFFFFFF)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		l->SetItemColor(row, col, colour);
	}

	void Refresh()
	{
		m_rows = 0;

		// ---- Abilities page: what you have and whether it is firing.
		if (List) {
			List->DeleteAll();
			for (int i = 0; i < g_skCount; ++i) {
				// Colour by state, so "what is on" reads at a glance without parsing the column.
				// ⚠️ 0xFF.. -- the leading byte is ALPHA. A colour written 0x00RRGGBB is fully TRANSPARENT and
				// the row is invisible; that is why no skills could be seen here at all.
				const COLORREF col = g_skOn[i] ? 0xFF80FF80 : 0xFF909090;
				const int row = List->AddString(g_skName[i], col, (uint32_t)i, nullptr, g_skName[i]);
				Cell(List, row, 1, g_skOn[i] ? "ON" : "off", col);
				m_rows++;
			}
		}
	}

	void Toggle(int on)
	{
		const int i = SelRow();
		if (i < 0) { return; }
		char cmd[64];
		sprintf_s(cmd, "/say askset %d %d", g_skId[i], on);
		AoTQueueGameCommand(cmd);
	}

	// ⚠️ One command per skill, and each is answered with an ASKILLDATA line. With ten specials that
	// is twenty chat lines in a burst, which is the shape RoF2 drops -- so only skills that would
	// actually CHANGE are sent. In practice that is a handful, and pressing it twice sends nothing.
	void ToggleAll(int on)
	{
		// ⚠️ "All On" STOPS AT THE CAP. The server refuses the fifth and answers each refusal with a
		// red message, so sending them anyway would spam the player with a rejection per extra skill.
		int already = 0;
		if (on) {
			for (int i = 0; i < g_skCount; ++i) { if (g_skOn[i]) { ++already; } }
		}

		for (int i = 0; i < g_skCount; ++i) {
			if (g_skOn[i] == on) { continue; }
			if (on && already >= AUTOSKILL_CAP) { break; }
			char cmd[64];
			sprintf_s(cmd, "/say askset %d %d", g_skId[i], on);
			AoTQueueGameCommand(cmd);
			if (on) { ++already; }
		}
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) { pXWnd()->Show(0, 1); return 1; }

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)EnableB)  { Toggle(1);   return 1; }
			if (pWnd == (CXWnd*)DisableB) { Toggle(0);   return 1; }
			if (pWnd == (CXWnd*)AllOnB)   { ToggleAll(1); return 1; }
			if (pWnd == (CXWnd*)AllOffB)  { ToggleAll(0); return 1; }
			if (pWnd == (CXWnd*)RefreshB) { AoTQueueGameCommand("/say askrefresh"); return 1; }
			if (pWnd == (CXWnd*)List && List) {
				// Cache only, and only for the ABILITIES list -- the timer page has no actions.
				// See SelRow: GetCurSel() is stale at this point.
				m_sel = List->GetCurSel();
				return 1;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise.
static void AutoSkillEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		AskTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up", pSidlMgr, pWndMgr);
		return;
	}

	if (!gAutoSkillWnd) {
		gAutoSkillWnd = new AutoSkillWnd();
		// A NULL pXWnd here means CCustomWnd could not find the screen "AoTAutoSkillWnd" -- which
		// means EQUI_AoTAutoSkillWnd.xml is not loaded, i.e. the <Include> line is missing from
		// EQUI.xml or the file was never copied. That is the single most common cause of "nothing
		// happened", and it is otherwise completely silent.
		AskTrace("EnsureWindow: created AutoSkillWnd, pXWnd=%p (NULL means the XML is not loaded)",
		         gAutoSkillWnd ? (void*)gAutoSkillWnd->pXWnd() : nullptr);
	}
	if (gAutoSkillWnd && show) {
		gAutoSkillWnd->Refresh();
		if (gAutoSkillWnd->pXWnd()) { gAutoSkillWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== chat transport
// "ASKILLDATA <n>^skillid|name|enabled|cooldown_secs|reuse_secs^..."
bool AutoSkillParseTransport(const char* message)
{
	if (!g_enabled || !message) return false;

	const char* t = strstr(message, "ASKILLDATA ");
	if (!t) return false;
	t += strlen("ASKILLDATA ");

	g_skCount = 0;
	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_skCount < SK_MAX) {
					std::string f[5]; SkSplit(cur, f, 5);
					g_skId   [g_skCount] = atoi(f[0].c_str());
					strncpy_s(g_skName[g_skCount], sizeof(g_skName[0]), f[1].c_str(), _TRUNCATE);
					g_skOn   [g_skCount] = atoi(f[2].c_str());
					g_skCd   [g_skCount] = (float)atoi(f[3].c_str());
					g_skReuse[g_skCount] = atoi(f[4].c_str());
					g_skCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	g_lastTick = GetTickCount();

	// ⚠️ REBUILD ONLY WHEN THE ROWS ACTUALLY CHANGED. Refresh() does DeleteAll(), which clears the
	// listbox selection -- and this line arrives every resync while the window is open, so rebuilding
	// unconditionally would yank the selection out from under the player every couple of seconds.
	static std::string s_sig;
	std::string sig;
	for (int i = 0; i < g_skCount; ++i) {
		char b[32]; sprintf_s(b, "%d:%d,", g_skId[i], g_skOn[i]);
		sig += b;
	}

	// ⚠️ Rebuild ONLY when the rows actually changed. Refresh() calls DeleteAll, which clears the
	// selection, so rebuilding on every reply would yank it out from under the player mid click.
	if (gAutoSkillWnd && sig != s_sig) { gAutoSkillWnd->Refresh(); }
	s_sig = sig;

	// Do NOT force the window open here: this also arrives as the reply to a toggle and to every
	// resync, and popping it each time would fight the player.
	return true;
}

bool AutoSkillIsOurEcho(const char* message)
{
	if (!g_enabled || !message) return false;
	return strstr(message, "You say, 'askset ")    != nullptr
	    || strstr(message, "You say, 'askrefresh") != nullptr;
}

// ===================================================================== per-frame
// Counts the reuse timers down locally between server updates. See the header for why they are not
// streamed: a chat line per second per player is exactly the burst the RoF2 chat pipe drops.
void AutoSkillTick()
{
	if (!g_enabled || !gAutoSkillWnd || g_skCount <= 0) return;

	const DWORD now = GetTickCount();
	if (!g_lastTick) { g_lastTick = now; return; }

	const float dt = (float)(now - g_lastTick) / 1000.0f;
	g_lastTick = now;
	if (dt <= 0.0f) return;

	// Cooldowns are still COUNTED so the state stays truthful, but nothing displays them any more --
	// the Timers page is gone. Kept because ASKILLDATA still carries them and a future display (a
	// short buff / autoskill tracker, CLAUDE.md section 17b) would want them already ticking.
	for (int i = 0; i < g_skCount; ++i) {
		if (g_skCd[i] > 0.0f) {
			g_skCd[i] -= dt;
			if (g_skCd[i] < 0.0f) { g_skCd[i] = 0.0f; }
		}
	}

	// Local counting alone drifts out of truth the moment autoskill FIRES something -- the client has
	// no idea a timer just restarted, so it would keep showing "Ready". So resync periodically, but
	// ONLY while the window is actually on screen, and slowly: this costs one chat line each way per
	// interval, and a per-second stream is precisely the burst the RoF2 chat pipe drops (CLAUDE.md
	// section 15). Closed window, no traffic at all.
	static DWORD s_lastSync = 0;
	if (gAutoSkillWnd->pXWnd() && gAutoSkillWnd->pXWnd()->IsReallyVisible()) {
		if (now - s_lastSync > AUTOSKILL_RESYNC_MS) {
			s_lastSync = now;
			AoTQueueGameCommand("/say askrefresh");
		}
	}
}

// ===================================================================== module entry points
// ⚠️ Runs inside DllMain under the loader lock, so it may only set a flag -- creating the window
// here would hard-fail startup with 0xc0000142.
void InitAutoSkill() { g_enabled = true; AskTrace("InitAutoSkill: module enabled"); }

void AutoSkillShow()
{
	AskTrace("AutoSkillShow: enabled=%d skills=%d", g_enabled ? 1 : 0, g_skCount);
	if (!g_enabled) return;
	AutoSkillEnsureWindow(true);
	AoTQueueGameCommand("/say askrefresh");   // open on current truth, not a stale snapshot
}

void AutoSkillOnUiReset()
{
	if (gAutoSkillWnd) { delete gAutoSkillWnd; gAutoSkillWnd = nullptr; }
}
