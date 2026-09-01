// core_meter -- the Damage Meter window. See core_meter.h for the module contract.

#include <Windows.h>
#include "MQ2Main.h"
#include "core_meter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern void AoTQueueGameCommand(const char* cmd);

// ⚠️ Matches the server's own cap (AoTv4Encounter::Row bounds at 24). A client-side row cap sized to
// something OTHER than the server's is how a list silently truncates -- CLAUDE.md section 3 records
// exactly that for the Zone XP tab, where a 60-row client cap ate the end of a 69-row browse list.
static const int MTR_MAX = 24;

struct MeterRow {
	char  name[64];
	__int64 damage;
	__int64 healing;
	__int64 taken;
	unsigned attempts;
	unsigned avoided;
	__int64  best_sec;
	__int64  overheal;
};

static MeterRow g_rows[MTR_MAX];
static int      g_nrows   = 0;
static char     g_target[64] = {0};
static unsigned g_seconds = 1;
// The VIEW MODE, not a sort order. Each mode answers a different question and shows its own numbers;
// showing all three at once put a healing column in the damage view where it meant nothing.
enum { MTR_DAMAGE = 0, MTR_TAKEN = 1, MTR_HEALING = 2, MTR_MODES = 3 };
static int      g_mode    = MTR_DAMAGE;

// Which fight the server is showing us: -1 is the live one, 0+ indexes its history.
// ⚠️ ECHOED BACK BY THE SERVER on every push rather than tracked locally. The server shifts its own
// indices when a new fight is archived (a fight you were reading moves from 3 to 4), so a local guess
// would silently drift onto a different fight.
static int      g_view    = -1;

// The fight picker, from METERLIST.
struct MeterFight { int index; char target[64]; unsigned seconds; __int64 total; };
static MeterFight g_fights[16];
static int        g_nfights = 0;

// The drill-down, from METERDETAIL. g_detail[0] empty means we are showing the player list.
struct MeterPart { char label[64]; __int64 damage; __int64 healing; unsigned hits; __int64 max_hit; };
static MeterPart g_parts[16];
static int       g_nparts = 0;
static char      g_detail[64] = {0};

// Row index -> index into g_rows, rebuilt on every Refresh.
// ⚠️⚠️ A ROW INDEX IS NOT A DATA INDEX. The list is SORTED, so clicking the top row must not read
// g_rows[0] -- that is whoever happened to arrive first on the wire. Every window in this dll that
// sorts or interleaves keeps an explicit map for exactly this reason (the Death Book's g_rowGroup, the
// Delve history's g_histRowRun), and reading the name back off the widget instead would mean
// CListWnd::GetItemText, whose out-parameter signature nothing here has ever exercised.
static int g_order[MTR_MAX];

static const char* kModeName[MTR_MODES] = { "Damage done", "Damage taken", "Healing done" };

static void MtrLabel(CXWnd* w, const char* s)
{
	if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
}

// Thousands separators. A meter is read at a glance mid fight, and 18204 against 1820 is a genuinely
// hard comparison to make in a hurry where 18,204 against 1,820 is not.
static void MtrNum(__int64 v, char* out, size_t n)
{
	char raw[32];
	sprintf_s(raw, "%lld", v < 0 ? 0 : v);
	const int len = (int)strlen(raw);
	int o = 0;
	for (int i = 0; i < len && o < (int)n - 1; ++i) {
		if (i > 0 && ((len - i) % 3) == 0) { out[o++] = ','; }
		out[o++] = raw[i];
	}
	out[o] = 0;
}

class MeterWnd : public CCustomWnd
{
public:
	CListWnd* List  = nullptr;
	CXWnd*    Title = nullptr;
	CXWnd*    Total = nullptr;
	CComboWnd*  Mode = nullptr;   // view mode: damage done / taken / healing done
	CComboWnd*  Fight = nullptr;  // which fight: live, or one of the stored ones
	CButtonWnd* Back  = nullptr;

	int m_sel = -1;

	MeterWnd() : CCustomWnd("AoTMeterWnd")
	{
		SetWndNotification(MeterWnd);
		List  = (CListWnd*)  GetChildItem("MTR_List");
		Title =              GetChildItem("MTR_Title");
		Total =              GetChildItem("MTR_Total");
		Mode  = (CComboWnd*) GetChildItem("MTR_Mode");
		Fight = (CComboWnd*) GetChildItem("MTR_FightCombo");
		Back  = (CButtonWnd*)GetChildItem("MTR_Back");
		FillModes();
		// ⚠️ Fill the fight list on construction too. A window reopened after some fighting already has
		// g_fights populated, and waiting for the next METERLIST would leave the dropdown empty until
		// something else died.
		FillFights();
		Refresh();
	}

	// ⚠️⚠️ POPULATED ONCE, IN THE CONSTRUCTOR. Refilling a combobox calls DeleteAll, which drops the
	// selection and shuts the list while the player has it open -- and this window refreshes once a
	// second, so it would be unusable. The Delve window's difficulty dropdown is refilled only because
	// its choices come from the server and can genuinely change; these three never do.
	// Rebuild the fight list. Choice 0 is always the live fight, so a stored fight at history index n
	// sits at choice n+1 -- that offset is the only mapping between the two and is applied in both
	// directions here and in the click handler.
	// ⚠️⚠️ CALLED ONLY FROM MeterParseList, NEVER FROM Refresh. Refresh runs once a second, and
	// DeleteAll drops the selection and shuts the dropdown -- it would be impossible to pick anything.
	void FillFights()
	{
		if (!Fight) { return; }
		Fight->SetColors(0xFFE0DCCD, 0xFF666666, 0xFFCC3333);
		Fight->DeleteAll();
		Fight->InsertChoice((PCHAR)"Happening now");

		char label[128], tot[40];
		for (int i = 0; i < g_nfights; ++i) {
			MtrNum(g_fights[i].total, tot, sizeof(tot));
			// Name, how long it took and what it cost -- enough to recognise a fight without opening it.
			sprintf_s(label, "%s   %u:%02u   %s", g_fights[i].target,
			          g_fights[i].seconds / 60, g_fights[i].seconds % 60, tot);
			Fight->InsertChoice((PCHAR)label);
		}
		SyncFightChoice();
	}

	// ⚠️ The SERVER owns which fight is selected (g_view is echoed on every push), so the dropdown is
	// told, never asked. It renumbers its history as fights are archived, and a dropdown that kept its
	// own idea of the selection would drift onto a different fight without looking wrong.
	void SyncFightChoice()
	{
		if (!Fight) { return; }
		const int choice = (g_view < 0) ? 0 : (g_view + 1);
		if (choice >= 0 && choice <= g_nfights) { Fight->SetChoice(choice); }
	}

	void FillModes()
	{
		if (!Mode) { return; }
		Mode->SetColors(0xFFE0DCCD, 0xFF666666, 0xFFCC3333);
		Mode->DeleteAll();
		for (int i = 0; i < MTR_MODES; ++i) { Mode->InsertChoice((PCHAR)kModeName[i]); }
		Mode->SetChoice(g_mode);
	}

	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF c)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
		// SetItemText has no colour of its own and the client draws it BLACK, which is very nearly
		// invisible on this UI (CLAUDE.md section 21).
		l->SetItemColor(row, col, c);
	}

	// The one number this mode is about. Sorting, the percentage and the Total column all read it, so a
	// mode cannot show one metric and rank by another.
	static __int64 Value(const MeterRow& r)
	{
		return (g_mode == MTR_TAKEN) ? r.taken : (g_mode == MTR_HEALING) ? r.healing : r.damage;
	}

	// The breakdown for one player. Same listbox, different columns -- a second Listbox would need its
	// own headers and its own anchors for a view that is never on screen at the same time.
	void RefreshDetail()
	{
		if (!List) { return; }

		// ⚠️ The breakdown follows the MODE. In healing mode a list of damage sources is the wrong answer
		// to the question the dropdown just asked, and its percentages would not add up to the row the
		// player clicked -- both numbers are on screen at once.
		const bool heal_mode = (g_mode == MTR_HEALING);

		__int64 total = 0;
		for (int i = 0; i < g_nparts; ++i) { total += heal_mode ? g_parts[i].healing : g_parts[i].damage; }

		// ⚠️ Sorted into an index, not by reordering g_parts -- the transport rewrites that array
		// wholesale and sorting in place would fight it.
		int order[16];
		for (int i = 0; i < g_nparts; ++i) { order[i] = i; }
		for (int i = 1; i < g_nparts; ++i) {
			const int k = order[i]; int j = i - 1;
			const __int64 kv = heal_mode ? g_parts[k].healing : g_parts[k].damage;
			while (j >= 0 && (heal_mode ? g_parts[order[j]].healing : g_parts[order[j]].damage) < kv) {
				order[j + 1] = order[j]; --j;
			}
			order[j + 1] = k;
		}

		List->DeleteAll();

		// ⚠️ Say so rather than showing an empty list. A breakdown with no rows and a breakdown that never
		// arrived look identical, and telling them apart is most of the work when one goes wrong.
		if (g_nparts == 0) {
			const int row = List->AddString("Nothing recorded for this fight", 0xFF9AA0A8,
			                                0, nullptr, nullptr);
			for (int c = 1; c <= 5; ++c) { Cell(List, row, c, "", 0xFF9AA0A8); }
		}

		for (int i = 0; i < g_nparts; ++i) {
			const MeterPart& p = g_parts[order[i]];
			const COLORREF col = (i == 0) ? 0xFFFFE0A0 : 0xFFD8D8D8;
			const int row = List->AddString(p.label, col, (uint32_t)i, nullptr, nullptr);

			const __int64 v = heal_mode ? p.healing : p.damage;

			char buf[40];
			MtrNum(v, buf, sizeof(buf));                               Cell(List, row, 1, buf, col);
			MtrNum(v / (g_seconds ? g_seconds : 1), buf, sizeof(buf)); Cell(List, row, 2, buf, col);
			const int pct = total > 0 ? (int)((v * 100) / total) : 0;
			sprintf_s(buf, "%d%%", pct);                               Cell(List, row, 3, buf, col);

			// Hits and Max mean here exactly what their headers say, which is the whole reason the
			// columns were made neutral: one set of headers serves both views.
			if (p.hits)    { sprintf_s(buf, "%u", p.hits); }          else { buf[0] = 0; }
			Cell(List, row, 4, buf, col);
			if (p.max_hit) { MtrNum(p.max_hit, buf, sizeof(buf)); }   else { buf[0] = 0; }
			Cell(List, row, 5, buf, col);
			Cell(List, row, 6, "", col);
		}

		// Burst and overheal belong to the PLAYER, not to any one ability, so they live in the heading
		// rather than as columns that would be blank on every row.
		__int64 best = 0, over = 0;
		for (int i = 0; i < g_nrows; ++i) {
			if (_stricmp(g_rows[i].name, g_detail) == 0) { best = g_rows[i].best_sec; over = g_rows[i].overheal; break; }
		}

		char head[200], b1[40], b2[40];
		MtrNum(best, b1, sizeof(b1));
		MtrNum(over, b2, sizeof(b2));
		if (heal_mode) {
			if (over > 0) { sprintf_s(head, "%s   healing, by spell   overheal %s", g_detail, b2); }
			else          { sprintf_s(head, "%s   healing, by spell", g_detail); }
		} else if (g_mode == MTR_TAKEN) {
			sprintf_s(head, "%s   damage taken, by source", g_detail);
		} else {
			sprintf_s(head, "%s   damage, by source   best second %s", g_detail, b1);
		}
		MtrLabel(Title, head);

		char tbuf[40], line[128];
		MtrNum(total, tbuf, sizeof(tbuf));
		sprintf_s(line, "%s total", tbuf);
		MtrLabel(Total, line);
		ApplyVis();
	}

	// ⚠️ HIDDEN, not greyed. There is no address-mapped enable setter on this build -- CXWnd::IsEnabled
	// is declared and never defined -- and writing a raw Enabled member offset is section 13's
	// garbage-pointer trap. CXWnd::Show IS mapped.
	void ApplyVis()
	{
		const bool detail = (g_detail[0] != 0);
		if (Back) { ((CXWnd*)Back)->Show(detail ? 1 : 0, 1); }
		// The fight picker is meaningless while a breakdown is on screen: it belongs to whichever fight
		// the breakdown came from.
		// ⚠️ The fight dropdown stays visible in the breakdown. "Show me this ability on the previous
		// pull" is a reasonable thing to want, and hiding it would force a trip back out first.
		if (Fight) { ((CXWnd*)Fight)->Show(1, 1); }
		// ⚠️ The mode dropdown stays visible in the breakdown: it is what you press to get back to a
		// different question, and hiding it would strand the drill-down behind the Back button alone.
		if (Mode) { ((CXWnd*)Mode)->Show(1, 1); }
	}

	void Refresh()
	{
		if (!List) { return; }
		if (g_detail[0]) { RefreshDetail(); return; }

		// ⚠️⚠️ SORTED INTO AN INDEX, NOT BY REORDERING g_rows. The transport rewrites g_rows wholesale
		// every second; sorting the array in place would fight that, and a row's identity would depend on
		// when it last arrived rather than on who it is.
		for (int i = 0; i < g_nrows; ++i) { g_order[i] = i; }
		for (int i = 1; i < g_nrows; ++i) {           // insertion sort: at most 24 entries, once a second
			const int k = g_order[i];
			int j = i - 1;
			while (j >= 0 && Value(g_rows[g_order[j]]) < Value(g_rows[k])) { g_order[j + 1] = g_order[j]; --j; }
			g_order[j + 1] = k;
		}

		__int64 total = 0;
		for (int i = 0; i < g_nrows; ++i) { total += Value(g_rows[i]); }

		List->DeleteAll();
		for (int i = 0; i < g_nrows; ++i) {
			const MeterRow& r = g_rows[g_order[i]];

			// ⚠️ 0xFF.. -- the leading byte is ALPHA. A colour written 0x00RRGGBB is fully TRANSPARENT and
			// the row draws as nothing at all (CLAUDE.md section 21).
			// The top row is highlighted because in a meter the question is almost always "who is first".
			const COLORREF col = (i == 0) ? 0xFFFFE0A0 : 0xFFD8D8D8;

			const int row = List->AddString(r.name, col, (uint32_t)i, nullptr, nullptr);

			const __int64 v = Value(r);

			char buf[40];
			MtrNum(v, buf, sizeof(buf));                                  Cell(List, row, 1, buf, col);
			MtrNum(v / (g_seconds ? g_seconds : 1), buf, sizeof(buf));    Cell(List, row, 2, buf, col);

			// ⚠️ Integer maths, scaled before dividing. (v / total) * 100 truncates to 0 for everyone
			// below the leader, which reads as a broken column rather than a rounding choice.
			const int pct = total > 0 ? (int)((v * 100) / total) : 0;
			sprintf_s(buf, "%d%%", pct);                                  Cell(List, row, 3, buf, col);

			// The last two columns carry whatever the current mode has to say. Blank rather than zero
			// where a mode has nothing for them: a column of zeroes reads as a broken stat, an empty one
			// reads as "not applicable", which is what it is.
			buf[0] = 0;
			if (g_mode == MTR_DAMAGE && r.attempts > 0) {
				const unsigned landed = (r.attempts > r.avoided) ? (r.attempts - r.avoided) : 0;
				sprintf_s(buf, "%u", landed);
			}
			Cell(List, row, 4, buf, col);

			buf[0] = 0;
			if (g_mode == MTR_DAMAGE && r.best_sec > 0)   { MtrNum(r.best_sec, buf, sizeof(buf)); }
			else if (g_mode == MTR_HEALING && r.overheal > 0) { MtrNum(r.overheal, buf, sizeof(buf)); }
			Cell(List, row, 5, buf, col);
			Cell(List, row, 6, "", col);
		}

		char head[160];
		if (g_target[0]) {
			// ⚠️ Say which fight this is. Once history exists, a window showing numbers with no idea
			// whether they are live or from four pulls ago is worse than one showing nothing.
			if (g_view >= 0) {
				sprintf_s(head, "%s   %u:%02u   (%d back)", g_target,
				          g_seconds / 60, g_seconds % 60, g_view + 1);
			} else {
				sprintf_s(head, "%s   %u:%02u", g_target, g_seconds / 60, g_seconds % 60);
			}
		} else {
			strcpy_s(head, "No fight yet");
		}
		MtrLabel(Title, head);

		char tbuf[40], line[128];
		MtrNum(total, tbuf, sizeof(tbuf));
		sprintf_s(line, "%s total   %lld dps", tbuf, total / (g_seconds ? g_seconds : 1));
		MtrLabel(Total, g_nrows ? line : "");
		SyncFightChoice();

		ApplyVis();
	}

	// Ask the server for a stored fight. Sending the INDEX and letting the server answer is what keeps
	// the two in step -- see the note on g_view.
	static void AskFight(int idx)
	{
		char cmd[64];
		sprintf_s(cmd, "/say meterfight %d", idx);
		AoTQueueGameCommand(cmd);
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)Mode && Mode) {
				// ⚠️ Base class FIRST, then read. Inside a combobox's own notification GetCurChoice is one
				// click behind, exactly as GetCurSel is on a listbox -- so reading it first would apply
				// the PREVIOUSLY selected mode.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				const int sel = Mode->GetCurChoice();
				if (sel >= 0 && sel < MTR_MODES && sel != g_mode) {
					g_mode = sel;
					// ⚠️ Changing mode drops the drill-down. A breakdown of damage is not a breakdown of
					// healing, and leaving it up would show one mode's rows under another mode's heading.
					g_detail[0] = 0;
					g_nparts = 0;
					Refresh();
				}
				return handled;
			}

			if (pWnd == (CXWnd*)Back) { g_detail[0] = 0; g_nparts = 0; Refresh(); return 1; }

			if (pWnd == (CXWnd*)Fight && Fight) {
				// ⚠️ Base class FIRST, then read -- GetCurChoice is one selection behind inside the
				// combobox's own notification, exactly as GetCurSel is on a listbox.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				const int sel = Fight->GetCurChoice();
				if (sel >= 0) {
					// Choice 0 is live; everything after it is history index sel-1.
					AskFight(sel == 0 ? -1 : sel - 1);
					// ⚠️ Changing fight drops the drill-down: a breakdown belongs to the fight it came
					// from, and leaving it up would show one fight's rows under another's heading.
					g_detail[0] = 0;
					g_nparts = 0;
				}
				return handled;
			}

			if (pWnd == (CXWnd*)List && List) {
				// ⚠️⚠️ LET THE BASE CLASS RUN FIRST, THEN READ. Inside a listbox's own notification
				// GetCurSel is one click behind, so reading it before would ask for the PREVIOUSLY
				// selected player's breakdown -- the same trap the Death Book and the Delve history
				// window both record.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_sel = List->GetCurSel();

				// A click in the breakdown does nothing; there is nothing below it to drill into.
				if (!g_detail[0] && m_sel >= 0 && m_sel < g_nrows) {
					// ⚠️ Through g_order, because the list is sorted. See the note on g_order.
					const MeterRow& r = g_rows[g_order[m_sel]];
					if (r.name[0]) {
						strcpy_s(g_detail, sizeof(g_detail), r.name);
						g_nparts = 0;
						char cmd[96];
						sprintf_s(cmd, "/say meterdetail %s", r.name);
						AoTQueueGameCommand(cmd);
					}
				}
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

static MeterWnd* gMeterWnd = nullptr;

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently bails
// at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void MeterEnsure(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) { return; }

	if (!gMeterWnd) { gMeterWnd = new MeterWnd(); }
	if (gMeterWnd && !gMeterWnd->pXWnd()) {
		// A NULL pXWnd means the screen name is unknown, i.e. the XML was never parsed. Silent
		// otherwise, and the most common cause of "the button does nothing" for every window here.
		extern void AoTv4ChatPrint(const char* line);
		AoTv4ChatPrint("Damage Meter: EQUI_AoTMeterWnd.xml is not loaded.");
		AoTv4ChatPrint("Copy it to uifiles\\default and add "
		               "<Include>EQUI_AoTMeterWnd.xml</Include> to EQUI.xml, then /loadskin.");
		return;
	}
	if (gMeterWnd && show) {
		gMeterWnd->Refresh();
		if (gMeterWnd->pXWnd()) { gMeterWnd->pXWnd()->Show(1, 1); }
	}
}

void MeterWindowShow()
{
	MeterEnsure(true);
	// ⚠️ The server sends NOTHING until asked, and the flag lives on its Client object -- which is
	// rebuilt on every zone. Asking on every open is what keeps the window alive across zoning without a
	// separate zone hook.
	AoTQueueGameCommand("/say meter 1");
}

// METERDATA <target>|<seconds>^name|damage|healing|taken^...
bool MeterParseTransport(const char* msg)
{
	if (!msg || strncmp(msg, "METERDATA ", 10) != 0) { return false; }

	// ⚠️ Swallowed whatever happens below. Returning false would put the raw transport text into the
	// player's chat window once a second, which is far worse than dropping an update.
	const char* p = msg + 10;

	g_nrows = 0;
	g_target[0] = 0;
	g_seconds = 1;

	// header: <target>|<seconds>|<view>
	const char* bar = strchr(p, '|');
	const char* rec = strchr(p, '^');
	if (bar) {
		const size_t n = (size_t)(bar - p);
		const size_t c = (n < sizeof(g_target) - 1) ? n : sizeof(g_target) - 1;
		memcpy(g_target, p, c);
		g_target[c] = 0;
		g_seconds = (unsigned)atoi(bar + 1);
		if (g_seconds == 0) { g_seconds = 1; }

		// ⚠️ The view index is echoed by the SERVER and adopted here rather than tracked locally. The
		// server renumbers its own history when a fight is archived, so a local guess drifts onto a
		// different fight without anything looking wrong.
		const char* bar2 = strchr(bar + 1, '|');
		if (bar2 && (!rec || bar2 < rec)) { g_view = atoi(bar2 + 1); }
	}

	while (rec && g_nrows < MTR_MAX) {
		++rec;
		const char*  end = strchr(rec, '^');
		const size_t len = end ? (size_t)(end - rec) : strlen(rec);

		char buf[192];
		const size_t c = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
		memcpy(buf, rec, c);
		buf[c] = 0;

		// name|damage|healing|taken -- split on '|' in place
		// ⚠️ Split into up to EIGHT but accept FOUR. The server appends fields rather than reordering
		// them, so an older server simply sends fewer and the extra columns read as zero instead of the
		// whole row being dropped.
		char* f[8] = { buf, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		int   nf   = 1;
		for (char* s = buf; *s && nf < 8; ++s) {
			if (*s == '|') { *s = 0; f[nf++] = s + 1; }
		}
		if (nf >= 4) {
			MeterRow& r = g_rows[g_nrows++];
			memset(&r, 0, sizeof(r));
			strcpy_s(r.name, sizeof(r.name), f[0]);
			r.damage  = _atoi64(f[1]);
			r.healing = _atoi64(f[2]);
			r.taken   = _atoi64(f[3]);
			if (nf >= 8) {
				r.attempts = (unsigned)atoi(f[4]);
				r.avoided  = (unsigned)atoi(f[5]);
				r.best_sec = _atoi64(f[6]);
				r.overheal = _atoi64(f[7]);
			}
		}
		rec = end;
	}

	if (gMeterWnd) { gMeterWnd->Refresh(); }
	return true;
}

// METERLIST ^index|target|seconds|total^...
bool MeterParseList(const char* msg)
{
	if (!msg || strncmp(msg, "METERLIST", 9) != 0) { return false; }

	g_nfights = 0;
	const char* rec = strchr(msg, '^');
	while (rec && g_nfights < 16) {
		++rec;
		const char*  end = strchr(rec, '^');
		const size_t len = end ? (size_t)(end - rec) : strlen(rec);

		char buf[160];
		const size_t c = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
		memcpy(buf, rec, c); buf[c] = 0;

		char* f[4] = { buf, nullptr, nullptr, nullptr };
		int   nf   = 1;
		for (char* q = buf; *q && nf < 4; ++q) { if (*q == '|') { *q = 0; f[nf++] = q + 1; } }
		if (nf >= 4) {
			MeterFight& fi = g_fights[g_nfights++];
			fi.index = atoi(f[0]);
			strcpy_s(fi.target, sizeof(fi.target), f[1]);
			fi.seconds = (unsigned)atoi(f[2]);
			fi.total   = _atoi64(f[3]);
		}
		rec = end;
	}
	if (gMeterWnd) { gMeterWnd->FillFights(); gMeterWnd->Refresh(); }
	return true;
}

// METERDETAIL <who>^label|damage|healing^...
bool MeterParseDetail(const char* msg)
{
	if (!msg || strncmp(msg, "METERDETAIL ", 12) != 0) { return false; }

	const char* p = msg + 12;
	const char* rec = strchr(p, '^');

	// ⚠️ The name is taken from the REPLY, not left as whatever was clicked. If a click and an archive
	// cross, the server answers about the fight it actually has, and the heading has to match the rows.
	const size_t nlen = rec ? (size_t)(rec - p) : strlen(p);
	const size_t nc   = (nlen < sizeof(g_detail) - 1) ? nlen : sizeof(g_detail) - 1;
	memcpy(g_detail, p, nc); g_detail[nc] = 0;

	g_nparts = 0;
	while (rec && g_nparts < 16) {
		++rec;
		const char*  end = strchr(rec, '^');
		const size_t len = end ? (size_t)(end - rec) : strlen(rec);

		char buf[160];
		const size_t c = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
		memcpy(buf, rec, c); buf[c] = 0;

		char* f[5] = { buf, nullptr, nullptr, nullptr, nullptr };
		int   nf   = 1;
		for (char* q = buf; *q && nf < 5; ++q) { if (*q == '|') { *q = 0; f[nf++] = q + 1; } }
		if (nf >= 3) {
			MeterPart& pt = g_parts[g_nparts++];
			memset(&pt, 0, sizeof(pt));
			strcpy_s(pt.label, sizeof(pt.label), f[0]);
			pt.damage  = _atoi64(f[1]);
			pt.healing = _atoi64(f[2]);
			if (nf >= 5) { pt.hits = (unsigned)atoi(f[3]); pt.max_hit = _atoi64(f[4]); }
		}
		rec = end;
	}
	if (gMeterWnd) { gMeterWnd->Refresh(); }
	return true;
}

// Keep the server sending, and stop it when the window is closed.
// ⚠️⚠️ THE SAME BUG AS THE COMBAT TEXT FEED, WHICH WAS REPORTED FROM PLAY. m_aotv4_meter_on lives on
// the server's Client object and that is rebuilt on every zone, so a single announce at window-open
// dies the first time you zone -- and the window then sits there showing the last fight it saw, which
// looks exactly like a meter that has stopped updating rather than one that stopped being fed.
//
// 📌 GATED ON ACTUAL VISIBILITY, via CXWnd::IsReallyVisible (0x866960, mapped and defined in
// EQClasses.cpp). An earlier note in this dll claimed no readable visibility accessor existed on this
// build -- that was wrong, it had been grepped for as IsVisible and dShow. Without it the only options
// were to heartbeat forever once opened, which leaves the server pushing a line a second at a window
// nobody is looking at, or not to heartbeat at all.
static const unsigned MTR_ANNOUNCE_MS = 60000;
static unsigned g_mtrLastAnnounce = 0;
static bool     g_mtrWasVisible   = false;

void MeterWindowTick()
{
	if (!gMeterWnd || !gMeterWnd->pXWnd()) { return; }

	const bool visible = gMeterWnd->pXWnd()->IsReallyVisible();
	const unsigned now = GetTickCount();

	if (visible) {
		// On becoming visible, and every minute after. Either alone is not enough: the event misses a
		// zone that never hides the window, and a heartbeat alone leaves the first minute dead.
		if (!g_mtrWasVisible || (now - g_mtrLastAnnounce) >= MTR_ANNOUNCE_MS) {
			AoTQueueGameCommand("/say meter 1");
			g_mtrLastAnnounce = now;
		}
	}
	else if (g_mtrWasVisible) {
		// ⚠️ Told ONCE on close, not repeatedly. The server holds the flag until something changes it,
		// so a closed window costs nothing further.
		AoTQueueGameCommand("/say meter 0");
	}

	g_mtrWasVisible = visible;
}

void MeterWindowOnUiReset()
{
	// ⚠️⚠️ DELETE, do not just null. Nulling leaks the CCustomWnd and leaves it registered with the
	// client's window manager after its widgets are freed, so the next thing to walk the window list
	// touches freed memory.
	if (gMeterWnd) { delete gMeterWnd; gMeterWnd = nullptr; }
	// ⚠️ A UI reload destroys the window without ever hiding it, so the close branch above never runs.
	// Reset the tracker or the next window is thought to be already visible and never announces.
	g_mtrWasVisible = false;
	// ⚠️ Drop the drill-down too. A UI reload leaves no window to press Back on, so a surviving g_detail
	// would open the next one straight into a breakdown of a fight that is no longer on screen.
	g_detail[0] = 0;
	g_nparts = 0;
}
