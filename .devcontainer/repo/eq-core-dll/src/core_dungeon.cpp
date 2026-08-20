// core_dungeon.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Delve window (scaling dungeon). See core_dungeon.h for the module contract.
//
// The server owns everything that matters: which layers are unlocked, creating the instance,
// scaling the mobs, handing out the quest, dropping the chest and tearing the instance down. This
// window only shows the unlocked layers and sends three commands.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_dungeon.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// ⚠️⚠️ ONE RUNG PER LEVEL, 1 to 70 -- this must stay comfortably above M.MAX_LEVEL in
// aotv4_dungeon.lua. It was 24 while the ladder ran in fives (15 rungs); going to 70 rungs without
// raising it would have SILENTLY TRUNCATED the list at 24, showing a ladder that simply stops at
// level 24 with no error anywhere. Sized to 96 so the server can grow the ladder again without a
// dll rebuild, which is the point of the cap being generous rather than exact.
static const int DG_MAX = 96;
static int  g_dgCount = 0;
static int  g_dgLevel  [DG_MAX] = {};
static char g_dgName   [DG_MAX][64] = { {0} };
static int  g_dgCleared[DG_MAX] = {};

// ---- score sheet. HIST_MAX mirrors M.HISTORY_MAX in aotv4_dungeon.lua; the server caps the list, so
// this only has to be no smaller than it is.
static const int HIST_MAX = 24;
static const int BAND_MAX = 12;
static const int HIST_ROW_MAX = HIST_MAX * (BAND_MAX + 1);

static int  g_hCount = 0;
static char g_hWhen  [HIST_MAX][24] = { {0} };
static int  g_hLevel [HIST_MAX] = {};
static char g_hName  [HIST_MAX][64] = { {0} };
static int  g_hKills [HIST_MAX] = {};
static int  g_hScore [HIST_MAX] = {};
static char g_hAvg   [HIST_MAX][12] = { {0} };
static int  g_hLo    [HIST_MAX] = {};
static int  g_hHi    [HIST_MAX] = {};
static char g_hOut   [HIST_MAX] = {};
static char g_hBands [HIST_MAX][128] = { {0} };
static bool g_hOpen  [HIST_MAX] = {};

// Row to run map. ⚠️ A row index is NOT a run index: headers interleave with band rows and a closed
// run hides all of its bands, so the mapping has to be recorded as the list is built.
// >= 0 : this row is that run's header.  -1 : a band detail row.
static int g_histRowRun[HIST_ROW_MAX] = {};
static int g_histRowN = 0;

// Set by the click handler, applied a frame later by DungeonTick. See the header for why.
static int g_pendingToggle = -1;

// ---------------------------------------------------------------- difficulty modes
// ⚠️ The list, the display names AND the descriptions all arrive from the server over DUNGMODES.
// Nothing about a mode is hardcoded here on purpose: an array in this file would have to be kept in
// step with M.MODES in aotv4_dungeon.lua by hand, which is exactly the kIcons[] drift that made the
// reward picker's icon set wrong the moment the spell pool changed (CLAUDE.md section 3). Adding a
// seventh mode is a server edit and this file does not change.
static const int MODE_MAX = 12;
static int  g_modeCount = 0;
static char g_modeId  [MODE_MAX][24]  = { {0} };   // wire id, e.g. "swarm" -- what delveenter sends
static char g_modeName[MODE_MAX][32]  = { {0} };   // display name for the dropdown
static char g_modeDesc[MODE_MAX][256] = { {0} };   // shown in the description pane

// ⚠️ Index 0 is Standard by construction: the server emits M.MODES in order and Standard is first.
// It is the only mode always legal on an uncleared layer, which is why the fallback is 0 everywhere
// rather than "whatever was last selected".
static int g_modeSel = 0;

static void DungeonEnsureWindow(bool show);
static bool DungeonHistTransport(const char* message);   // defined below; routed from ParseTransport
static bool DungeonModesTransport(const char* message);  // ditto -- the difficulty list

// Debug trace to <EQ>\aotv4_dungeon.log. Same reason as the other native windows: when a SIDL window
// fails to appear there is NOTHING in any client log to say why -- a missing EQUI.xml <Include>, an
// unbuilt dll and a failed CCustomWnd construction all look identical from in game.
static void DgTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_dungeon.log", gszEQPath); }
	else             { strcpy_s(path, "aotv4_dungeon.log"); }

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

// Split "a|b|c" into n fields. Local copy on purpose -- every window in this dll has one, because
// sharing it would mean a header that all of them include for four lines of code.
static void DgSplit(const std::string& s, std::string* out, int n)
{
	int i = 0;
	std::string cur;
	for (size_t p = 0; p <= s.size(); ++p) {
		if (p == s.size() || s[p] == '|') {
			if (i < n) { out[i++] = cur; }
			cur.clear();
		}
		else { cur += s[p]; }
	}
	while (i < n) { out[i++].clear(); }
}

// ===================================================================== the window
static class DungeonWnd* gDungeonWnd = nullptr;

class DungeonWnd : public CCustomWnd
{
public:
	CListWnd*   List     = nullptr;   // Layers tab: level + dungeon + status
	CListWnd*   HistList = nullptr;   // Score Sheet tab: one header per run, expandable
	CButtonWnd* EnterB   = nullptr;
	CButtonWnd* ExitB    = nullptr;
	CButtonWnd* RefreshB = nullptr;
	// ⚠️ A SECOND refresh button, living on the Score Sheet page. A Page owns its pieces, so the
	// Delve tab's DLV_Refresh is not reachable while the score sheet is showing -- and that is the
	// tab most likely to look stale, since it only changes when a run ENDS. Same command, same
	// screen position, so the button does not appear to move when the tab is switched.
	CButtonWnd* HistRefreshB = nullptr;
	CComboWnd*  ModeCombo = nullptr;   // difficulty; choices pushed in from DUNGMODES
	CStmlWnd*   ModeDesc  = nullptr;   // what the selected difficulty actually does

	int m_sel = -1;   // see SelRow: a FALLBACK only, never the primary source

	DungeonWnd() : CCustomWnd((char*)"AoTDungeonWnd")
	{
		SetWndNotification(DungeonWnd);
		List     = (CListWnd*)  GetChildItem("DLV_LayerList");
		HistList = (CListWnd*)  GetChildItem("DLV_HistList");
		EnterB   = (CButtonWnd*)GetChildItem("DLV_Enter");
		ExitB    = (CButtonWnd*)GetChildItem("DLV_Exit");
		RefreshB = (CButtonWnd*)GetChildItem("DLV_Refresh");
		HistRefreshB = (CButtonWnd*)GetChildItem("DLV_HistRefresh");
		ModeCombo = (CComboWnd*)GetChildItem("DLV_ModeCombo");
		ModeDesc  = (CStmlWnd*) GetChildItem("DLV_ModeDesc");
		Refresh();
		RefreshHistory();
		RefreshModes();
	}

	// ---------------------------------------------------------------- the difficulty dropdown
	// ⚠️ DeleteAll before InsertChoice, or a second DUNGMODES (every window open sends one) stacks a
	// duplicate set of choices underneath the first.
	void RefreshModes()
	{
		if (!ModeCombo) { return; }

		// ⚠️⚠️ WITHOUT THIS THE CHOICE LIST DRAWS IN BLACK and is unreadable against the dropdown's
		// dark background. A CCustomWnd combo does not inherit the colours the stock UI sets
		// programmatically on its own comboboxes, and there is no <TextColor> on the Combobox that
		// fixes it -- the colour has to be pushed in from code.
		// ⚠️⚠️ THE THREE ARGUMENTS ARE NOT THREE TEXT COLOURS. They are
		//     (text, hover BAR fill, selection BAR fill)
		// and treating arguments 2 and 3 as text is how this got fixed twice and stayed broken twice:
		// all-light painted a white bar that swallowed the text, all-dark painted a near-black one.
		// The evidence is in the reference itself (MQ2AdvDps.cpp): its first argument, NormalColor, is
		// ALSO handed to CListWnd::SetItemColor(row, col, ...), which can only be a text colour --
		// while its other two default to 0xFFCC3333 and 0xFF666666, the stock EQ red selection bar and
		// grey hover. Those are fills, not glyph colours.
		// Using the reference's own proven values, with our cream text on top.
		// ⚠️ 0xAARRGGBB and the ALPHA BYTE IS MANDATORY: 0x00RRGGBB is fully transparent and the text
		// is drawn but invisible -- a separate trap, recorded in CLAUDE.md section 21.
		ModeCombo->SetColors(0xFFE0DCCD, 0xFF666666, 0xFFCC3333);

		ModeCombo->DeleteAll();
		for (int i = 0; i < g_modeCount; ++i) {
			// ⚠️ The char* overload, which is a thin wrapper in EQClasses.cpp around the real
			// address-mapped InsertChoice(CXStr*, ulong). Passing a CXStr BY VALUE does not compile:
			// the mapped signature takes a POINTER.
			ModeCombo->InsertChoice(g_modeName[i]);
		}
		if (g_modeSel >= g_modeCount) { g_modeSel = 0; }
		ModeCombo->SetChoice(g_modeSel);
		RefreshModeDesc();
	}

	// The description pane, plus the gating notice. ⚠️ The notice is PRESENTATION -- M.enter revalidates
	// server side, exactly like the reward picker sending an index instead of a spell id. A modified
	// dll can select anything it likes and the server still refuses it.
	void RefreshModeDesc()
	{
		if (!ModeDesc) { return; }
		if (g_modeSel < 0 || g_modeSel >= g_modeCount) { ModeDesc->SetSTMLText(CXStr(""), false, nullptr); return; }

		std::string t = g_modeDesc[g_modeSel];

		// Mode 0 (Standard) is always available; anything else needs this layer cleared first.
		const int i = SelRow();
		if (g_modeSel > 0 && i >= 0 && i < g_dgCount && !g_dgCleared[i]) {
			t += "<br><br><c \"#FF8080\">Clear this delve on Standard before you can run it on ";
			t += g_modeName[g_modeSel];
			t += ".</c>";
		}
		ModeDesc->SetSTMLText(CXStr(t.c_str()), false, nullptr);
	}

	// ⚠️ Live selection first, cache second. A list click's WndNotification fires BEFORE the listbox
	// commits the new selection, so GetCurSel() is one behind INSIDE that handler -- which is why the
	// list passes its own row in. From a BUTTON handler the selection has committed and the live
	// value must win; the cache only covers the opposite hazard, where clicking a control clears it.
	// ⚠️ Row index IS the data index here: the list is unfiltered and shows every unlocked layer.
	int SelRow(int known_row = -1)
	{
		int row = known_row;
		if (row < 0 && List) { row = List->GetCurSel(); }
		if (row < 0)         { row = m_sel; }
		if (row < 0 && g_dgCount == 1) { row = 0; }   // one layer unlocked: no need to click it
		if (row < 0 || row >= g_dgCount) { return -1; }
		return row;
	}

	// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
	// SetItemText has no colour of its own and the client draws it BLACK, which is very nearly
	// invisible on this UI. Every extra column needs an explicit SetItemColor.
	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFFFFFFF)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		l->SetItemColor(row, col, colour);
	}

	void Refresh()
	{
		if (!List) { return; }
		List->DeleteAll();
		for (int i = 0; i < g_dgCount; ++i) {
			// ⚠️ 0xFF.. -- the leading byte is ALPHA. A colour written 0x00RRGGBB is fully
			// TRANSPARENT and the row draws as nothing at all (CLAUDE.md section 21).
			// Cleared layers are dimmed so the newest one you have opened stands out.
			const COLORREF col = g_dgCleared[i] ? 0xFF909090 : 0xFFFFE0A0;
			char lvl[16];
			sprintf_s(lvl, "%d", g_dgLevel[i]);
			const int row = List->AddString(lvl, col, (uint32_t)i, nullptr, lvl);
			Cell(List, row, 1, g_dgName[i], col);
			Cell(List, row, 2, g_dgCleared[i] ? "cleared" : "-", col);
		}
	}

	void Enter()
	{
		const int i = SelRow();
		if (i < 0) { return; }

		// ⚠️ Read the combo LIVE rather than trusting g_modeSel: a selection made in the dropdown and
		// never followed by any other notification would otherwise send the previous mode.
		if (ModeCombo) {
			const int sel = ModeCombo->GetCurChoice();
			if (sel >= 0 && sel < g_modeCount) { g_modeSel = sel; }
		}

		// ⚠️ The mode ID goes on the wire, not the index. An index would silently mean something
		// different the moment a mode is inserted anywhere but the end of M.MODES.
		// ⚠️ The server accepts a bare `delveenter <level>` too, so an un-rebuilt dll still works --
		// but this one always names the mode, including Standard, so the log is unambiguous.
		const char* mode = (g_modeCount > 0 && g_modeSel >= 0 && g_modeSel < g_modeCount)
		                   ? g_modeId[g_modeSel] : "standard";
		char cmd[96];
		sprintf_s(cmd, "/say delveenter %d %s", g_dgLevel[i], mode);
		AoTQueueGameCommand(cmd);
	}

	// ---------------------------------------------------------------- the score sheet
	// One header row per finished run; an OPEN run also lists its level-band breakdown underneath.
	void RefreshHistory()
	{
		if (!HistList) { return; }
		HistList->DeleteAll();
		g_histRowN = 0;

		for (int i = 0; i < g_hCount && g_histRowN < HIST_ROW_MAX; ++i) {
			// Outcome drives the colour, so a wiped run is visible at a glance without reading it.
			// ⚠️ 0xFF.. -- the leading byte is ALPHA; 0x00RRGGBB draws nothing at all.
			COLORREF col = 0xFFFFE0A0;                        // cleared
			const char* tag = "cleared";
			if (g_hOut[i] == 'F') { col = 0xFFFF9090; tag = "DIED";      }
			if (g_hOut[i] == 'A') { col = 0xFF909090; tag = "abandoned"; }

			char head[96];
			sprintf_s(head, "%s %s  lvl %d  %s",
			          g_hOpen[i] ? "[-]" : "[+]", g_hWhen[i], g_hLevel[i], g_hName[i]);

			const int row = HistList->AddString(head, col, (uint32_t)i, nullptr, head);
			char score[24]; sprintf_s(score, "%d", g_hScore[i]);
			char kills[24]; sprintf_s(kills, "%d", g_hKills[i]);
			Cell(HistList, row, 1, score, col);
			Cell(HistList, row, 2, kills, col);
			Cell(HistList, row, 3, tag,   col);
			g_histRowRun[g_histRowN++] = i;

			if (!g_hOpen[i]) { continue; }

			// Average first, then the bands. Dimmer than the header so the hierarchy reads.
			const COLORREF sub = 0xFFC8C8C8;
			char line[128];
			sprintf_s(line, "      average level %s  (weakest %d, strongest %d)",
			          g_hAvg[i], g_hLo[i], g_hHi[i]);
			int r = HistList->AddString(line, sub, (uint32_t)i, nullptr, line);
			Cell(HistList, r, 1, "", sub);
			Cell(HistList, r, 2, "", sub);
			Cell(HistList, r, 3, "", sub);
			g_histRowRun[g_histRowN++] = -1;

			// bands arrive as "45-12,50-20,55-15": band start, then kills in it.
			const char* p = g_hBands[i];
			while (p && *p && g_histRowN < HIST_ROW_MAX) {
				int band = 0, n = 0;
				if (sscanf_s(p, "%d-%d", &band, &n) == 2 && n > 0) {
					sprintf_s(line, "      level %d to %d: %d killed", band, band + 4, n);
					r = HistList->AddString(line, sub, (uint32_t)i, nullptr, line);
					Cell(HistList, r, 1, "", sub);
					Cell(HistList, r, 2, "", sub);
					Cell(HistList, r, 3, "", sub);
					g_histRowRun[g_histRowN++] = -1;
				}
				const char* comma = strchr(p, ',');
				if (!comma) { break; }
				p = comma + 1;
			}
		}
	}

	// ⚠️ Called from DungeonTick, never from the click handler -- Refresh does DeleteAll and would
	// tear the rows out from under the click that asked for it.
	void ToggleHistRow(int row)
	{
		if (row < 0 || row >= g_histRowN) { return; }
		const int run = g_histRowRun[row];
		if (run < 0 || run >= g_hCount) { return; }   // a band detail row: clicking it does nothing
		g_hOpen[run] = !g_hOpen[run];
		RefreshHistory();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) { pXWnd()->Show(0, 1); return 1; }

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)EnterB)   { Enter(); return 1; }
			// ⚠️ Exit is ALWAYS sent, never gated on the window's idea of whether you are inside one.
			// The server knows; if you are not in a delve it says so. Gating it here would mean two
			// places tracking the same state and one of them being wrong after a zone or a relog.
			if (pWnd == (CXWnd*)ExitB)    { AoTQueueGameCommand("/say delveexit"); return 1; }
			// ⚠️ Both refresh buttons send the SAME command on purpose: /say delve returns the layer
			// list AND the score sheet in one round trip (aotv4_dungeon.M.handle_say sends
			// send_list + send_history together), so neither tab can refresh into a half-updated
			// state and there is no second command to keep in step.
			if (pWnd == (CXWnd*)RefreshB ||
			    pWnd == (CXWnd*)HistRefreshB) { AoTQueueGameCommand("/say delve"); return 1; }
			if (pWnd == (CXWnd*)List && List) {
				m_sel = List->GetCurSel();   // cache only; see SelRow
				// Selecting a different layer can change whether the chosen mode is legal here, so
				// the description (and its gating notice) is rebuilt with the new row.
				// ⚠️ Safe from inside a list click: this touches only the STML pane, never the
				// listbox. Rebuilding a list inside its own notification is the Death Book crash.
				RefreshModeDesc();
				return 1;
			}
			// ⚠️ The combo notifies on the COMBO window itself, so this is a plain pointer test. The
			// live GetCurChoice is read here rather than trusting a cached value, for the same reason
			// the layer list reads its selection live.
			if (pWnd == (CXWnd*)ModeCombo && ModeCombo) {
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				const int sel = ModeCombo->GetCurChoice();
				if (sel >= 0 && sel < g_modeCount) { g_modeSel = sel; }
				RefreshModeDesc();
				return handled;
			}
			if (pWnd == (CXWnd*)HistList && HistList) {
				// ⚠️⚠️ DO NOT TOGGLE HERE -- RefreshHistory calls DeleteAll, which would destroy the
				// listbox's rows from inside the listbox's own click notification while the client is
				// still walking them. Deferred to DungeonTick, one frame later.
				// ⚠️ Read the selection AFTER the base class has processed the click: inside the
				// notification GetCurSel is still one click behind and would open the previous run.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				g_pendingToggle = HistList->GetCurSel();
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void DungeonEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		DgTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up", pSidlMgr, pWndMgr);
		return;
	}

	if (!gDungeonWnd) {
		gDungeonWnd = new DungeonWnd();
		// A NULL pXWnd means CCustomWnd could not find the screen "AoTDungeonWnd", i.e.
		// EQUI_AoTDungeonWnd.xml is not loaded -- the <Include> is missing from EQUI.xml or the file
		// was never copied. Silent otherwise, and the most common cause of "nothing happened".
		DgTrace("EnsureWindow: created DungeonWnd, pXWnd=%p (NULL means the XML is not loaded)",
		        gDungeonWnd ? (void*)gDungeonWnd->pXWnd() : nullptr);
	}
	if (gDungeonWnd && show) {
		gDungeonWnd->Refresh();
		if (gDungeonWnd->pXWnd()) { gDungeonWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== chat transport
// "DUNGMODES <n>^id|name|desc^id|name|desc^..."
// The difficulty list. Sent when the window is opened, before DUNGDATA, so the dropdown is populated
// before the layer list it gates against.
// ⚠️ A description may contain neither '|' nor '^'; they are the field separators. That is a rule for
// whoever edits M.MODES rather than something escaped here, and it is recorded in the Lua too.
static bool DungeonModesTransport(const char* message)
{
	const char* t = strstr(message, "DUNGMODES ");
	if (!t) return false;
	t += strlen("DUNGMODES ");

	g_modeCount = 0;
	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_modeCount < MODE_MAX) {
					std::string f[3]; DgSplit(cur, f, 3);
					strncpy_s(g_modeId  [g_modeCount], sizeof(g_modeId[0]),   f[0].c_str(), _TRUNCATE);
					strncpy_s(g_modeName[g_modeCount], sizeof(g_modeName[0]), f[1].c_str(), _TRUNCATE);
					strncpy_s(g_modeDesc[g_modeCount], sizeof(g_modeDesc[0]), f[2].c_str(), _TRUNCATE);
					g_modeCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	DgTrace("DUNGMODES: %d modes", g_modeCount);
	if (gDungeonWnd) { gDungeonWnd->RefreshModes(); }
	return true;
}

// "DUNGDATA <unlocked>^level|name|cleared^level|name|cleared^..."
bool DungeonParseTransport(const char* message)
{
	if (!g_enabled || !message) return false;

	// Every server line comes through this one entry point so the dsp_chat chain only needs one call.
	if (DungeonHistTransport(message)) { return true; }
	if (DungeonModesTransport(message)) { return true; }

	const char* t = strstr(message, "DUNGDATA ");
	if (!t) return false;
	t += strlen("DUNGDATA ");

	g_dgCount = 0;
	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_dgCount < DG_MAX) {
					std::string f[3]; DgSplit(cur, f, 3);
					g_dgLevel[g_dgCount] = atoi(f[0].c_str());
					strncpy_s(g_dgName[g_dgCount], sizeof(g_dgName[0]), f[1].c_str(), _TRUNCATE);
					g_dgCleared[g_dgCount] = atoi(f[2].c_str());
					g_dgCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	DgTrace("DUNGDATA: %d layers", g_dgCount);

	// ⚠️ Rebuild, but do NOT force the window open. This line also arrives unprompted when a layer is
	// cleared (the server pushes a fresh list at that moment), and popping the window in the player's
	// face the instant they finish a dungeon would fight them while the chest is spawning.
	if (gDungeonWnd) { gDungeonWnd->Refresh(); }
	return true;
}

// "DLVHIST <n>^when|level|dungeon|kills|score|avg|lo|hi|outcome|bands^..."
// ⚠️ Arrives NEWEST FIRST -- the server sorts it, because it already knows the order and the dll
// should not own a second copy of that rule.
static bool DungeonHistTransport(const char* message)
{
	if (!g_enabled || !message) return false;

	const char* t = strstr(message, "DLVHIST ");
	if (!t) return false;
	t += strlen("DLVHIST ");

	// ⚠️ Remember which runs were open across the rebuild. The server pushes a fresh history after
	// every finished run, and losing the expansion state each time would collapse the sheet under the
	// player mid read. Keyed on the timestamp, which is the only stable identity a run has.
	char wasOpen[HIST_MAX][24] = { {0} };
	int  wasOpenN = 0;
	for (int i = 0; i < g_hCount && wasOpenN < HIST_MAX; ++i) {
		if (g_hOpen[i]) { strcpy_s(wasOpen[wasOpenN++], sizeof(wasOpen[0]), g_hWhen[i]); }
	}

	g_hCount = 0;
	const char* rows = strchr(t, '^');
	if (rows) {
		std::string cur;
		for (const char* p = rows + 1; ; ++p) {
			if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
				if (!cur.empty() && g_hCount < HIST_MAX) {
					std::string f[10]; DgSplit(cur, f, 10);
					strncpy_s(g_hWhen [g_hCount], sizeof(g_hWhen[0]),  f[0].c_str(), _TRUNCATE);
					g_hLevel[g_hCount] = atoi(f[1].c_str());
					strncpy_s(g_hName [g_hCount], sizeof(g_hName[0]),  f[2].c_str(), _TRUNCATE);
					g_hKills[g_hCount] = atoi(f[3].c_str());
					g_hScore[g_hCount] = atoi(f[4].c_str());
					strncpy_s(g_hAvg  [g_hCount], sizeof(g_hAvg[0]),   f[5].c_str(), _TRUNCATE);
					g_hLo   [g_hCount] = atoi(f[6].c_str());
					g_hHi   [g_hCount] = atoi(f[7].c_str());
					g_hOut  [g_hCount] = f[8].empty() ? 'C' : f[8][0];
					strncpy_s(g_hBands[g_hCount], sizeof(g_hBands[0]), f[9].c_str(), _TRUNCATE);
					g_hOpen [g_hCount] = false;
					g_hCount++;
				}
				cur.clear();
				if (*p == 0 || *p == '\n' || *p == '\r') break;
			}
			else { cur += *p; }
		}
	}

	// Restore what was open, then default the newest run to open if nothing was.
	for (int i = 0; i < g_hCount; ++i) {
		for (int k = 0; k < wasOpenN; ++k) {
			if (strcmp(g_hWhen[i], wasOpen[k]) == 0) { g_hOpen[i] = true; break; }
		}
	}
	if (wasOpenN == 0 && g_hCount > 0) { g_hOpen[0] = true; }

	DgTrace("DLVHIST: %d runs", g_hCount);
	if (gDungeonWnd) { gDungeonWnd->RefreshHistory(); }
	return true;
}

// The commands the window sends come back as chat echoes; swallow them so they never appear.
bool DungeonIsOurEcho(const char* message)
{
	if (!g_enabled || !message) return false;
	return strstr(message, "delveenter ") != nullptr
	    || strstr(message, "delveexit")   != nullptr
	    || strstr(message, "delvehist")   != nullptr
	    || strstr(message, "delvepower")  != nullptr
	    || strstr(message, "'delve'")     != nullptr;
}

void DungeonTick()
{
	if (!g_enabled || g_pendingToggle < 0) { return; }
	const int row = g_pendingToggle;
	g_pendingToggle = -1;
	if (gDungeonWnd) { gDungeonWnd->ToggleHistRow(row); }
}

// ===================================================================== entry points
void DungeonShow()
{
	if (!g_enabled) { return; }
	DungeonEnsureWindow(true);
	// Ask for the current list every time it is opened: the unlock state is server truth and may have
	// moved since the window was last populated (another character, a relog, a clear).
	AoTQueueGameCommand("/say delve");
}

void DungeonOnUiReset()
{
	// The UI was torn down (/loadskin, ReloadUI, and CAMPING OUT via /q). The window object points at
	// destroyed widgets, so it has to go.
	//
	// ⚠️⚠️ DELETE IT, do not merely null the pointer. An earlier version just did `gDungeonWnd =
	// nullptr`, which leaks the CCustomWnd AND leaves it registered with the client's window manager
	// after the widgets under it are gone -- so the next thing that walks the window list touches
	// freed memory. Every other native window in this dll deletes here (core_lostwindow,
	// core_autoskill, core_advloot, core_allaclone); this one was the odd one out.
	if (gDungeonWnd) { delete gDungeonWnd; gDungeonWnd = nullptr; }
	g_pendingToggle = -1;   // the row it referred to no longer exists
}

void InitDungeon()
{
	g_enabled = true;
	DgTrace("InitDungeon: enabled");
}
