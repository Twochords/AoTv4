// core_difficulty.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Zone Difficulty window. See core_difficulty.h for the module contract and for why the
// native /pick window cannot be used.
//
// The server owns everything: the ladder itself, creating and reusing the per-difficulty instances,
// registering the character on one, moving them, scaling the creatures, and refusing a switch made
// in combat or inside a delve. This window shows four rows and sends two commands.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_difficulty.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// ⚠️ Sized well above the four the server sends today. The ladder is server-authored, so a fifth
// difficulty must not silently truncate here -- the same failure the Delve window hit when its rung
// count grew past a too-tight cap and the list simply stopped, with no error anywhere.
static const int ZD_MAX = 16;
static int  g_count = 0;
static int  g_cur   = 0;                    // the difficulty this character is currently playing
static int  g_id    [ZD_MAX] = {};
static char g_name  [ZD_MAX][32]  = { {0} };
static char g_hp    [ZD_MAX][16]  = { {0} };
static char g_lvl   [ZD_MAX][16]  = { {0} };
static char g_dmg   [ZD_MAX][16]  = { {0} };
static char g_blurb [ZD_MAX][512] = { {0} };   // description, then affixes, separated by "~"

static class DifficultyWnd* gDifficultyWnd = nullptr;

static void ZdTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_difficulty.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_difficulty.log"); }

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

// ===================================================================== window
// ⚠️⚠️ THE CHECKBOX STATE LIVES HERE, NOT ON THE WIDGET. `CButtonWnd::SetCheck` drives the visual,
// but this build exposes no readable checked member -- EQClasses.h has no bChecked and reading a raw
// struct offset is CLAUDE.md §13's garbage-pointer trap. The dll owns the truth and forces the widget
// to match, exactly as the Travel window does.
// ⚠️ Deliberately NOT persisted across sessions. A remembered "port my group" that silently survives
// a relog would move five other people the first time someone shifts without looking.
static bool g_group = false;

static void TvSetLabelD(CXWnd* w, const char* s)
{
	if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
}

class DifficultyWnd : public CCustomWnd
{
public:
	CListWnd*   List    = nullptr;
	CStmlWnd*   Desc    = nullptr;
	CButtonWnd* Travel  = nullptr;
	CButtonWnd* RefreshB = nullptr;
	CButtonWnd* GroupB  = nullptr;

	int m_sel = -1;

	DifficultyWnd() : CCustomWnd("AoTDifficultyWnd")
	{
		SetWndNotification(DifficultyWnd);
		List     = (CListWnd*)  GetChildItem("ZDF_List");
		Desc     = (CStmlWnd*)  GetChildItem("ZDF_Desc");
		Travel   = (CButtonWnd*)GetChildItem("ZDF_Travel");
		RefreshB = (CButtonWnd*)GetChildItem("ZDF_Refresh");
		GroupB   = (CButtonWnd*)GetChildItem("ZDF_Group");
		SyncGroup();
		Refresh();
	}

	// ⚠️ The caption carries the state as well as the glyph. Reported from play on the Travel window:
	// the checked and unchecked art differ by little more than a tint here, which is legible on a stock
	// window you already know and not on a new one. Same fix, same reason.
	void SyncGroup()
	{
		if (!GroupB) { return; }
		GroupB->SetCheck(g_group);
		TvSetLabelD((CXWnd*)GroupB, g_group ? "[X] Port Group" : "[  ] Port Group");
	}

	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFFFFFFF)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
		// SetItemText has no colour of its own and the client draws it BLACK, which is very nearly
		// invisible on this UI (CLAUDE.md section 21).
		l->SetItemColor(row, col, colour);
	}

	int SelRow(int known_row = -1)
	{
		int row = known_row;
		if (row < 0 && List) { row = List->GetCurSel(); }
		if (row < 0)         { row = m_sel; }
		if (row < 0 || row >= g_count) { return -1; }
		return row;
	}

	void Refresh()
	{
		if (!List) { return; }
		List->DeleteAll();
		for (int i = 0; i < g_count; ++i) {
			const bool now = (g_id[i] == g_cur);
			// ⚠️ 0xFF.. -- the leading byte is ALPHA. A colour written 0x00RRGGBB is fully
			// TRANSPARENT and the row draws as nothing at all (CLAUDE.md section 21).
			// The difficulty you are on is highlighted; the rest are ordinary.
			const COLORREF col = now ? 0xFFFFE0A0 : 0xFFD0D0D0;
			const int row = List->AddString(now ? ">" : "", col, (uint32_t)i, nullptr, nullptr);
			Cell(List, row, 1, g_name[i],  col);
			Cell(List, row, 2, g_hp[i],    col);
			Cell(List, row, 3, g_lvl[i],   col);
			Cell(List, row, 4, g_dmg[i],   col);
		}

		// Keep the description pointed at whatever is selected, defaulting to where you are now, so
		// the pane is never blank on open.
		if (m_sel < 0) {
			for (int i = 0; i < g_count; ++i) { if (g_id[i] == g_cur) { m_sel = i; break; } }
		}
		SetDesc();
	}

	void SetDesc()
	{
		if (!Desc) { return; }
		const int i = (m_sel >= 0 && m_sel < g_count) ? m_sel : -1;
		if (i < 0) {
			Desc->SetSTMLText(CXStr("Select a difficulty to read what changes."), true, nullptr);
			return;
		}

		// The blurb field carries the description and then the AFFIXES, separated by '~'. Split here
		// rather than on the wire so an older dll still renders something sane.
		//
		// ⚠️ '~' rather than a sixth pipe field: this whole record is split on '|' into five, so a
		// sixth would be dropped by an un-rebuilt dll and the affixes would vanish silently. Riding
		// inside the description means the worst case is that they run on as one paragraph.
		char body[1024];
		body[0] = 0;

		char work[512];
		strcpy_s(work, g_blurb[i]);

		char* ctx  = nullptr;
		char* part = strtok_s(work, "~", &ctx);
		bool  first = true;
		while (part) {
			if (first) {
				strcat_s(body, part);
				first = false;
			}
			else {
				strcat_s(body, "<br><br><c \"#FFC080\">- </c>");
				strcat_s(body, part);
			}
			part = strtok_s(nullptr, "~", &ctx);
		}

		char s[1400];
		if (g_id[i] == g_cur) {
			sprintf_s(s, "<c \"#FFE0A0\">%s</c><br><br>%s<br><br><c \"#90D090\">You are playing on this now.</c>",
			          g_name[i], body);
		}
		else {
			sprintf_s(s, "<c \"#FFE0A0\">%s</c><br><br>%s", g_name[i], body);
		}
		Desc->SetSTMLText(CXStr(s), true, nullptr);
	}

	void Go()
	{
		const int i = SelRow();
		if (i < 0) { return; }

		// ⚠️ THE ID GOES ON THE WIRE, NOT THE ROW INDEX. An index would silently mean a different
		// difficulty the moment one is inserted anywhere but the end of the server's ladder -- the
		// same reason the Delve window sends its mode id rather than its dropdown position.
		char cmd[64];
		sprintf_s(cmd, "/say diffset %d %d", g_id[i], g_group ? 1 : 0);
		AoTQueueGameCommand(cmd);

		// ⚠️ Do NOT update g_cur here. The server can refuse (in combat, or inside a delve) and it
		// re-sends DIFFDATA on success, so guessing locally would show the player standing in a
		// difficulty they were never moved to.
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)Travel)   { Go(); return 1; }
			if (pWnd == (CXWnd*)GroupB)   { g_group = !g_group; SyncGroup(); return 1; }
			if (pWnd == (CXWnd*)RefreshB) { AoTQueueGameCommand("/say diffwin"); return 1; }

			if (pWnd == (CXWnd*)List && List) {
				// ⚠️ Cache the selection HERE and let the base class run first. Inside a listbox's own
				// notification GetCurSel is one click behind, so reading it later in the button
				// handler would act on the PREVIOUSLY selected row -- sending the player to the wrong
				// difficulty, silently.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_sel = List->GetCurSel();
				SetDesc();
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void DifficultyEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		ZdTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up",
		        pSidlMgr, pWndMgr);
		return;
	}

	if (!gDifficultyWnd) {
		gDifficultyWnd = new DifficultyWnd();
		// A NULL pXWnd means CCustomWnd could not find the screen "AoTDifficultyWnd", i.e.
		// EQUI_AoTDifficultyWnd.xml is not loaded -- the <Include> is missing from EQUI.xml or the
		// file was never copied. Silent otherwise, and the most common cause of "nothing happened".
		ZdTrace("EnsureWindow: created DifficultyWnd, pXWnd=%p (NULL means the XML is not loaded)",
		        gDifficultyWnd ? (void*)gDifficultyWnd->pXWnd() : nullptr);
	}
	if (gDifficultyWnd && show) {
		gDifficultyWnd->Refresh();
		if (gDifficultyWnd->pXWnd()) { gDifficultyWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== chat transport
// "DIFFDATA <current>^id|name|hp|lvl|blurb^..."
static bool HandleDiffData(const char* msg)
{
	const char* t = strstr(msg, "DIFFDATA ");
	if (!t) { return false; }
	t += strlen("DIFFDATA ");

	g_count = 0;
	g_cur   = atoi(t);

	const char* p = strchr(t, '^');
	while (p && g_count < ZD_MAX) {
		++p;                                   // step over the '^'
		const char* end = strchr(p, '^');
		const size_t len = end ? (size_t)(end - p) : strlen(p);

		char rec[384];
		const size_t n = (len < sizeof(rec) - 1) ? len : sizeof(rec) - 1;
		memcpy(rec, p, n);
		rec[n] = 0;

		// id|name|hp|lvl|dmg|blurb -- split on '|' in place.
		char* f[6] = { rec, nullptr, nullptr, nullptr, nullptr, nullptr };
		int   nf   = 1;
		for (char* c = rec; *c && nf < 6; ++c) {
			if (*c == '|') { *c = 0; f[nf++] = c + 1; }
		}

		if (nf >= 6) {
			const int i = g_count;
			g_id[i] = atoi(f[0]);
			strcpy_s(g_name[i],  f[1]);
			// ⚠️⚠️ THE MULTIPLIERS ARE RENDERED HERE FROM RAW NUMBERS, AND NOTHING RESTATES THEM IN
			// PROSE. The descriptions used to say "half again as hard" beside a `dmg` of 1.40 and the
			// two drifted apart unnoticed, because nothing connected them. These columns read straight
			// off the same table the world scales from, so they cannot lie.
			// ⚠️ "unchanged" rather than "x1" or "+0" on the Normal row: a column of x1 / +0 reads as
			// a value that failed to load.
			if (atof(f[2]) > 1.0) { sprintf_s(g_hp[i],  "x%.0f", atof(f[2])); }
			else                  { strcpy_s (g_hp[i],  "unchanged"); }
			if (atoi(f[3]) > 0)   { sprintf_s(g_lvl[i], "+%d",   atoi(f[3])); }
			else                  { strcpy_s (g_lvl[i], "unchanged"); }
			// ⚠️ One decimal: damage runs 1.2 / 1.5 / 2.1, so "%.0f" would round Nightmare and Hell
			// to the same "x1" and Inferno to "x2" -- three distinct tiers collapsed into two.
			if (atof(f[4]) > 1.0) { sprintf_s(g_dmg[i], "x%.1f", atof(f[4])); }
			else                  { strcpy_s (g_dmg[i], "unchanged"); }
			strcpy_s(g_blurb[i], f[5]);
			++g_count;
		}

		p = end;
	}

	ZdTrace("DIFFDATA: current=%d count=%d", g_cur, g_count);
	if (gDifficultyWnd) { gDifficultyWnd->Refresh(); }
	return true;
}

bool DifficultyParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	return HandleDiffData(message);
}

// The "/say diff*" echoes. Every command a window issues must be swallowed or the player watches
// their own UI talk to the server in their chat log, and so does everyone standing nearby.
bool DifficultyIsOurEcho(const char* message)
{
	if (!g_enabled || !message) { return false; }
	return strstr(message, "diffwin") != nullptr || strstr(message, "diffset") != nullptr;
}

// ===================================================================== module entry points
void DifficultyShow()
{
	if (!g_enabled) { return; }
	DifficultyEnsureWindow(true);
	// Ask for the ladder every time it is opened. The current difficulty is server truth and may
	// have moved since the window was last populated (another character, a relog).
	AoTQueueGameCommand("/say diffwin");
}

void DifficultyOnUiReset()
{
	// The UI was torn down (/loadskin, ReloadUI, or camping out via /q). The window object points at
	// destroyed widgets, so it has to go.
	//
	// ⚠️⚠️ DELETE IT, do not merely null the pointer. Nulling alone leaks the CCustomWnd AND leaves
	// it registered with the client's window manager after the widgets under it are freed, so the
	// next thing that walks the window list touches freed memory (CLAUDE.md section 24).
	if (gDifficultyWnd) { delete gDifficultyWnd; gDifficultyWnd = nullptr; }
}

void InitDifficulty()
{
	g_enabled = true;
	ZdTrace("InitDifficulty: enabled");
}
