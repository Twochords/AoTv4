// core_travel.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Travel window. See core_travel.h for the module contract and the protocol.
//
// The server owns everything that matters: which regions are open, which waypoints this character
// has walked over, how many are still unfound, whether a group may travel together, and the
// confirmation box. This window shows two lists and sends one command.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_travel.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// ⚠️ Sized well above what the server sends today (6 regions, 13 destinations). Both lists are
// server-authored, so a seventh region or a fiftieth waypoint must not silently truncate -- the
// failure the Delve window hit when its rung count outgrew a tight cap and the list simply stopped.
static const int TV_MAX_REGIONS = 16;
static const int TV_MAX_DEST    = 128;

struct TravelRegion {
	int  id;
	char name[48];
	int  open;        // is this region unlocked for me
	int  found;       // waypoints I have discovered here
	int  unknown;     // ...and how many I have not
};

struct TravelDest {
	int  region;
	char id[32];      // the spot id the server expects back
	char name[64];
};

static int          g_nregion = 0;
static TravelRegion g_region[TV_MAX_REGIONS];
static int          g_ndest = 0;
static TravelDest   g_dest[TV_MAX_DEST];

// ⚠️⚠️ THE CHECKBOX STATES LIVE HERE, NOT ON THE WIDGETS. `CButtonWnd::SetCheck` is address-mapped
// (0x54FBE0) so the VISUAL can be driven, but there is no readable checked member on this build --
// EQClasses.h has no bChecked, and reading a raw struct offset is the trap CLAUDE.md section 13
// records as returning garbage. So the dll owns the truth and forces the widget to match on every
// toggle, rather than asking the widget what it thinks it is.
// ⚠️⚠️ WAS THIS WINDOW OPENED BY A BOOK? The books are the terminals -- see the header. Opening the
// window from the /aot launcher is allowed as a MAP (what have I found, what is left), but travelling
// from it is not, or the book stops mattering.
// ⚠️ Presentation only. The server re-checks that a book was clicked recently; a modified client that
// forces the button back gains a refusal, nothing else.
static bool g_fromBook = false;

static bool g_group = false;
static bool g_auto  = false;

// Set a widget's caption. ⚠️ The METHOD, never a raw `WindowText` member -- CLAUDE.md section 13
// records this build's struct offsets not matching the headers, where reading that member yields a
// garbage pointer and GetCXStr on it crashes. Only address-mapped methods are reliable here.
// ⚠ The Label's own <Text> in EQUI_AoTTravelWnd.xml. Restated here because once the hint has been
// overwritten with a refusal there is nothing left to read it back from.
static const char* TV_HINT_DEFAULT = "Choose a region, then a destination";

static void TvSetLabel(CXWnd* w, const char* s)
{
	if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
}

static class TravelWnd* gTravelWnd = nullptr;

static void TvTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_travel.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_travel.log"); }

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
class TravelWnd : public CCustomWnd
{
public:
	CListWnd*   Regions = nullptr;
	CListWnd*   Zones   = nullptr;
	CButtonWnd* Group   = nullptr;
	CButtonWnd* Auto    = nullptr;
	CButtonWnd* Travel  = nullptr;
	CXWnd*      Hint    = nullptr;   // ATW_Hint -- the only safe way to talk to the player here

	int m_region_row = -1;   // row in Regions
	int m_zone_row   = -1;   // row in Zones

	TravelWnd() : CCustomWnd("AoTTravelWnd")
	{
		SetWndNotification(TravelWnd);
		Regions = (CListWnd*)  GetChildItem("ATW_Regions");
		Zones   = (CListWnd*)  GetChildItem("ATW_Zones");
		Group   = (CButtonWnd*)GetChildItem("ATW_Group");
		Auto    = (CButtonWnd*)GetChildItem("ATW_Auto");
		Travel  = (CButtonWnd*)GetChildItem("ATW_Travel");
		Hint    = (CXWnd*)     GetChildItem("ATW_Hint");
		SyncChecks();
		ApplyMode();
		Refresh();
	}

	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFFFFFFF)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
		// SetItemText has no colour of its own and the client draws it BLACK, very nearly invisible
		// on this UI (CLAUDE.md section 21).
		l->SetItemColor(row, col, colour);
	}

	// Push our state onto the widgets. ⚠️ Called after construction and after every toggle, so the
	// box on screen can never disagree with the flag we will actually send.
	//
	// ⚠️⚠️ THE LABEL CARRIES THE STATE, BECAUSE THE CHECKBOX GLYPH ALONE DOES NOT READ AS PRESSED.
	// Reported from play: "very hard to tell when pressed since the only change is the color". The
	// checked and unchecked art on this build differ by little more than a tint, which is legible on
	// a stock window you already know and not legible on a new one. So the caption is rewritten with
	// an explicit [X] / [ ] through the address-mapped `SetWindowTextA` -- exactly the technique
	// CLAUDE.md section 3 records for the Allaclone mode buttons ("> Items <"), and for the same
	// reason: no latched button visual can be relied on from code here.
	// 📌 `SetCheck` is still called. It is not redundant -- it keeps the real glyph in step for anyone
	// who does read it, and the text is belt and braces on top rather than a replacement.
	void SyncChecks()
	{
		if (Group) { Group->SetCheck(g_group); TvSetLabel((CXWnd*)Group, g_group ? "[X] Port Group"  : "[  ] Port Group"); }
		if (Auto)  { Auto->SetCheck(g_auto);   TvSetLabel((CXWnd*)Auto,  g_auto  ? "[X] Auto Confirm" : "[  ] Auto Confirm"); }
	}

	// Show or hide the Travel button for the current mode.
	// ⚠️ HIDDEN, not greyed: there is no address-mapped enable setter on this build, and writing a raw
	// ->Enabled member offset is CLAUDE.md §13's unreliable-struct-offset trap. `Show` IS mapped --
	// the same reasoning §16 records for the Advanced Loot buttons.
	void ApplyMode()
	{
		// ⚠️ CAST TO CXWnd* -- `Show` is declared on CXWnd, NOT on CButtonWnd, so calling it through the
		// button pointer is "'Show': is not a member of 'EQClasses::CButtonWnd'". Same reason TvSetLabel
		// above takes a CXWnd*, and the same idiom core_advloot.cpp uses for its SetVis helper.
		if (Travel) { ((CXWnd*)Travel)->Show(g_fromBook ? 1 : 0, 1); }
	}

	void RefreshRegions()
	{
		if (!Regions) { return; }
		Regions->DeleteAll();
		for (int i = 0; i < g_nregion; ++i) {
			// ⚠️ 0xFF.. -- the leading byte is ALPHA. 0x00RRGGBB is fully TRANSPARENT and the row
			// draws as nothing at all (CLAUDE.md section 21).
			// A region you have not opened is dimmed rather than hidden: seeing it is what tells you
			// what a region credit would buy.
			const COLORREF col = g_region[i].open ? 0xFFE8E8E8 : 0xFF909090;
			const int row = Regions->AddString(g_region[i].name, col, (uint32_t)i, nullptr, nullptr);

			char found[32];
			if (g_region[i].unknown > 0) {
				sprintf_s(found, "%d + %d?", g_region[i].found, g_region[i].unknown);
			} else {
				sprintf_s(found, "%d", g_region[i].found);
			}
			Cell(Regions, row, 1, found, col);
		}
		if (m_region_row < 0 && g_nregion > 0) { m_region_row = 0; }
		RefreshZones();
	}

	void RefreshZones()
	{
		if (!Zones) { return; }
		Zones->DeleteAll();
		m_zone_row = -1;
		TvSetLabel(Hint, TV_HINT_DEFAULT);   // a refusal must not outlive the selection it was about
		if (m_region_row < 0 || m_region_row >= g_nregion) { return; }

		const TravelRegion& r = g_region[m_region_row];

		for (int i = 0; i < g_ndest; ++i) {
			if (g_dest[i].region != r.id) { continue; }
			const COLORREF col = r.open ? 0xFFFFE0A0 : 0xFF909090;
			const int row = Zones->AddString(g_dest[i].name, col, (uint32_t)i, nullptr, nullptr);
			Cell(Zones, row, 1, r.open ? "Known" : "Region locked", col);
		}

		// ⚠️⚠️ THE UNKNOWN ROWS ARE A COUNT, NOT A HIDDEN LIST. The server never sends the names of
		// waypoints you have not walked over, so there is nothing here to reveal -- these rows exist
		// purely to say "this region still holds things you have not found".
		for (int u = 0; u < r.unknown; ++u) {
			const int row = Zones->AddString("Unknown", 0xFF808080, 0xFFFFFFFF, nullptr, nullptr);
			Cell(Zones, row, 1, "Not yet found", 0xFF808080);
		}
	}

	void Refresh() { RefreshRegions(); }

	// The destination index behind the selected zone row, or -1 for an Unknown row.
	// ⚠️⚠️ A ROW INDEX IS NOT A DATA INDEX. Unknown rows are interleaved after the known ones and
	// carry the sentinel 0xFFFFFFFF, so the mapping has to come from the row's stored data, never
	// from its position -- the same trap as the Death Book and the spell Known/Pool tabs.
	int SelectedDest()
	{
		if (!Zones || m_zone_row < 0) { return -1; }
		const uint32_t data = Zones->GetItemData(m_zone_row);
		if (data == 0xFFFFFFFF) { return -1; }
		if ((int)data >= g_ndest) { return -1; }
		return (int)data;
	}

	void Go()
	{
		const int d = SelectedDest();
		if (d < 0) {
			// An Unknown row, or nothing selected.
			//
			// ⚠⚠ NEVER CALL WriteChatColor FROM THIS DLL -- IT IS A GUARANTEED CRASH, AND THIS LINE
			// WAS ONE. It forwards to dsp_chat_no_events, which calls ((CChatHook*)pEverQuest)->Trampoline
			// -- and that trampoline is only made real by InitializeChatHook(), which every one of its
			// three call sites gates behind `if (isMQInjectsEnabled)`. That flag is FALSE here
			// (_options.h), so the detour is never installed and the trampoline keeps its
			// DETOUR_TRAMPOLINE_EMPTY body from dependencies/detours/inc/detours.h:99 --
			// `xor eax,eax; mov eax,[eax]`, a deliberate null dereference so an uninstalled trampoline
			// fails loudly instead of silently. Reported from play as the Travel window crashing the
			// client whenever a destination was not picked.
			// 📌 CLAUDE.md section 13 recommends WriteChatColor for in-window feedback. That advice is
			// wrong for this dll for the reason above; the label half of the same note is correct.
			TvSetLabel(Hint, "Choose a destination you have already found.");
			return;
		}
		char cmd[128];
		sprintf_s(cmd, "/say travelgo %s %d %d", g_dest[d].id, g_group ? 1 : 0, g_auto ? 1 : 0);
		AoTQueueGameCommand(cmd);
		TvSetLabel(Hint, TV_HINT_DEFAULT);

		// ⚠️ Nothing is updated locally. The server may refuse (not found, region locked, in combat,
		// or a group member who does not qualify) and it answers in chat either way; guessing here
		// would show a trip that never happened.
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)Travel) { Go(); return 1; }

			// ⚠️ We own the state: flip OUR flag, then force the widget to match. The client toggles
			// the box itself on click, so trusting its appearance and ours to stay in step without
			// this would drift the moment either side is refreshed.
			if (pWnd == (CXWnd*)Group) { g_group = !g_group; SyncChecks(); return 1; }
			if (pWnd == (CXWnd*)Auto)  { g_auto  = !g_auto;  SyncChecks(); return 1; }

			if (pWnd == (CXWnd*)Regions && Regions) {
				// ⚠️ Let the base class run FIRST and read the selection after. Inside a listbox's own
				// notification GetCurSel is one click behind, so reading it before would repopulate
				// the destination list for the PREVIOUSLY selected region.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_region_row = Regions->GetCurSel();
				RefreshZones();
				return handled;
			}

			if (pWnd == (CXWnd*)Zones && Zones) {
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_zone_row = Zones->GetCurSel();
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void TravelEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);

	if (!pSidlMgr || !pWndMgr) {
		TvTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up",
			pSidlMgr, pWndMgr);
		return;
	}

	if (!gTravelWnd) {
		gTravelWnd = new TravelWnd();
		// A NULL pXWnd means CCustomWnd could not find the screen "AoTTravelWnd", i.e.
		// EQUI_AoTTravelWnd.xml is not loaded -- the <Include> is missing from EQUI.xml or the file
		// was never copied. Silent otherwise, and the most common cause of "nothing happened".
		// ⚠️ pXWnd is a METHOD on CSidlScreenWnd, not a data member. Writing it without the parens
		// compiles as a pointer-to-member and MSVC answers with "non-standard syntax; use & to create
		// a pointer to member", which reads like a syntax nit rather than a missing call.
		TvTrace("EnsureWindow: created TravelWnd, pXWnd=%p (NULL means the XML is not loaded)",
			gTravelWnd ? (void*)gTravelWnd->pXWnd() : nullptr);
	}
	if (gTravelWnd && show) {
		gTravelWnd->Refresh();
		// ⚠️ Show lives on the underlying CXWnd, not on our wrapper -- CCustomWnd has no Show of its
		// own, which is what "'Show': is not a member of 'TravelWnd'" was saying.
		if (gTravelWnd->pXWnd()) { gTravelWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== transport
// "TRAVELDATA <n>^rid|name|open|found|unknown^..."
static bool HandleTravelData(const char* msg)
{
	const char* p = strstr(msg, "TRAVELDATA ");
	if (!p) { return false; }
	p += 11;

	g_nregion = 0;
	const char* rec = strchr(p, '^');
	while (rec && g_nregion < TV_MAX_REGIONS) {
		++rec;
		TravelRegion& r = g_region[g_nregion];
		int open = 0, found = 0, unknown = 0, id = 0;
		char name[48] = {0};
		if (sscanf_s(rec, "%d|%47[^|]|%d|%d|%d", &id, name, (unsigned)_countof(name),
		             &open, &found, &unknown) == 5) {
			r.id = id; r.open = open; r.found = found; r.unknown = unknown;
			strcpy_s(r.name, name);
			++g_nregion;
		}
		rec = strchr(rec, '^');
	}

	TvTrace("TRAVELDATA: %d regions", g_nregion);
	if (gTravelWnd) { gTravelWnd->Refresh(); }
	return true;
}

// "TRAVELDEST <n>^rid|id|name^..."
static bool HandleTravelDest(const char* msg)
{
	const char* p = strstr(msg, "TRAVELDEST ");
	if (!p) { return false; }
	p += 11;

	g_ndest = 0;
	const char* rec = strchr(p, '^');
	while (rec && g_ndest < TV_MAX_DEST) {
		++rec;
		TravelDest& d = g_dest[g_ndest];
		int region = 0;
		char id[32] = {0}, name[64] = {0};
		if (sscanf_s(rec, "%d|%31[^|]|%63[^^]", &region, id, (unsigned)_countof(id),
		             name, (unsigned)_countof(name)) == 3) {
			d.region = region;
			strcpy_s(d.id, id);
			strcpy_s(d.name, name);
			++g_ndest;
		}
		rec = strchr(rec, '^');
	}

	TvTrace("TRAVELDEST: %d destinations", g_ndest);
	if (gTravelWnd) { gTravelWnd->Refresh(); }
	return true;
}

bool TravelParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }

	// ⚠️ A book was clicked. The server decides when this window opens -- there is no command for it.
	if (strstr(message, "TRAVELOPEN")) {
		TravelShow();
		return true;
	}
	if (HandleTravelData(message)) { return true; }
	if (HandleTravelDest(message)) { return true; }
	return false;
}

// The "/say travel*" echoes. Every command a window issues must be swallowed or the player watches
// their own UI talk to the server in their chat log, and so does everyone standing nearby.
// ⚠️ "travelgo" and "travelwin" only. A bare "/travel" is a PLAYER command (a printout) and must
// still reach the server and be seen normally.
bool TravelIsOurEcho(const char* message)
{
	if (!g_enabled || !message) { return false; }
	return strstr(message, "travelwin") != nullptr || strstr(message, "travelgo") != nullptr;
}

// ===================================================================== module entry points
// Opened by a Plane of Knowledge book: travel enabled.
void TravelShow()
{
	if (!g_enabled) { return; }
	g_fromBook = true;
	TravelEnsureWindow(true);
	if (gTravelWnd) { gTravelWnd->ApplyMode(); }
	// Ask for both lists every time. What you have found and which regions are open are server truth
	// and may have moved since this window was last populated (a new discovery, a spent credit).
	AoTQueueGameCommand("/say travelwin");
}

// Opened from the /aot launcher: a MAP, not a terminal. Same two lists, no Travel button.
//
// ⚠️⚠️ THE BOOK STAYS THE ONLY WAY TO MOVE. Letting the launcher travel would make the books
// pointless, which is the whole shape of the design (see the header). Showing the lists costs nothing
// though -- "what have I found, what is left in this region" is exactly what a player wants between
// trips, and making them walk to a book to read it is friction with no rule behind it.
// ⚠️ Hiding the button is PRESENTATION, not enforcement -- the server re-checks. §16 records the same
// split for Advanced Loot: hide what cannot be used, but never let the client be the rule.
void TravelShowBrowse()
{
	if (!g_enabled) { return; }
	g_fromBook = false;
	TravelEnsureWindow(true);
	if (gTravelWnd) { gTravelWnd->ApplyMode(); }
	AoTQueueGameCommand("/say travelwin");
}

void TravelOnUiReset()
{
	// The UI was torn down (/loadskin, ReloadUI, or camping out via /q). The window object points at
	// destroyed widgets, so it has to go.
	//
	// ⚠️⚠️ DELETE IT, do not merely null the pointer. Nulling alone leaks the CCustomWnd AND leaves
	// it registered with the client's window manager after the widgets under it are freed, so the
	// next thing that walks the window list touches freed memory (CLAUDE.md section 24).
	if (gTravelWnd) { delete gTravelWnd; gTravelWnd = nullptr; }
}

void InitTravel()
{
	g_enabled = true;
	TvTrace("InitTravel: enabled");
}
