// core_fellowship.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Fellowship window. See core_fellowship.h for the module contract and for why the client's
// own fellowship window cannot be used.
//
// The server owns everything: the roster and its 12 cap, who may invite or remove, the campfire
// bucket and its expiry, and the four gates on travel. This window shows two lists and sends says.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_fellowship.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// ⚠️ 12 is the design cap, but this is sized above it for the same reason the Difficulty window
// oversizes its ladder: the roster is server-authored, and a raised cap must not silently truncate
// here. The Delve window's rung list hit exactly that and simply stopped mid-list with no error.
static const int FS_MAX = 24;

// ⚠️⚠️ `strcpy_s` ON A FIXED ARRAY DOES NOT TRUNCATE -- IT INVOKES THE INVALID PARAMETER HANDLER,
// which by default aborts the process. Every string here comes off a CHAT LINE, so its length is
// whatever the server sent, and a long zone or class name would take the client down rather than
// clipping a column. Copy bounded instead; a clipped cell is a cosmetic problem, a crash is not.
template <size_t N>
static void FsCopy(char (&dst)[N], const char* src)
{
	if (!src) { dst[0] = 0; return; }
	strncpy_s(dst, N, src, _TRUNCATE);
}

static char g_fname [64]  = { 0 };            // fellowship name, "" when not in one
static char g_motd  [256] = { 0 };
static int  g_count = 0;
static int  g_leader[FS_MAX] = {};            // 1 if this member is the leader
static int  g_online[FS_MAX] = {};
static char g_zone  [FS_MAX][64] = { {0} };
static char g_member[FS_MAX][64] = { {0} };
// One array per COLUMN. The roster has to render members who are offline or in another zone, so
// every one of these arrives on the wire -- the dll never looks anything up itself.
static char g_lvl   [FS_MAX][8]  = { {0} };
static char g_class [FS_MAX][24] = { {0} };
static char g_laston[FS_MAX][24] = { {0} };

// campfire
static char g_fireZone[64] = { 0 };           // "" = nothing burning
static int  g_fireMins = 0;
static char g_fireType[32] = { 0 };

// The three fires, client side only -- they are a fixed list of what the player may LIGHT, not
// server state. ⚠️ Kept in step with aotv4_fellowship.M.FIRES by hand; nothing checks.
struct FireKind { const char* key; const char* name; const char* desc; };
static const FireKind kFires[] = {
	{ "basic",  "Fellowship of Honor",
	  "The plain fire. Free, carries no blessing, and is the point your fellowship travels to.<br><br>"
	  "<c \"#90D090\">Costs nothing.</c> Ask the Fellowmaster for kindling." },
	{ "health", "Fellowship of Health",
	  "Steadies those who gather at it, deepening their vigour.<br><br>"
	  "<c \"#FFC080\">Warded Campfire</c> -- combine a Campfire Kit with a Lumber Bundle." },
	{ "vigor",  "Fellowship of Vigor",
	  "Mends those who rest beside it, a little with every passing moment.<br><br>"
	  "<c \"#FFC080\">Enduring Campfire</c> -- combine a Campfire Kit with two Lumber Bundles." },
};
static const int kFireCount = (int)(sizeof(kFires) / sizeof(kFires[0]));

static class FellowshipWndAoT* gFellowshipWnd = nullptr;

static void FsTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_fellowship.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_fellowship.log"); }

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

static void FsSetLabel(CXWnd* w, const char* s)
{
	if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
}

// ⚠️⚠️ READ AN EDITBOX THROUGH THE ADDRESS-MAPPED METHOD, NEVER A STRUCT MEMBER. CLAUDE.md section
// 13: `((CEditWnd*)w)->InputText` reads empty on this build and `w->WindowText` returns a garbage
// pointer that CRASHES GetCXStr. Only FUNCTION_AT_ADDRESS methods are reliable here.
static void FsGetText(CXWnd* w, char* out, size_t n)
{
	if (!out || n == 0) { return; }
	out[0] = 0;
	if (!w) { return; }
	CXStr s = w->GetWindowTextA();
	PCXSTR raw = *(PCXSTR*)&s;
	if (raw) { GetCXStr(raw, out, (int)n); }
}

// ===================================================================== window
class FellowshipWndAoT : public CCustomWnd
{
public:
	CListWnd*   Members   = nullptr;
	CListWnd*   KitList   = nullptr;
	CStmlWnd*   KitDesc   = nullptr;
	CXWnd*      MotdInput = nullptr;
	CXWnd*      CampLoc   = nullptr;
	CXWnd*      CampView  = nullptr;

	CButtonWnd* BCreate   = nullptr;
	CButtonWnd* BLeave    = nullptr;
	CButtonWnd* BLeader   = nullptr;
	CButtonWnd* BRemove   = nullptr;
	CButtonWnd* BInvite   = nullptr;
	CButtonWnd* BEnd      = nullptr;
	CButtonWnd* BMotd     = nullptr;
	CButtonWnd* BXp       = nullptr;
	CButtonWnd* BRefresh  = nullptr;
	CButtonWnd* BLight    = nullptr;
	CButtonWnd* BDouse    = nullptr;
	CButtonWnd* BTravel   = nullptr;   // not a native piece; see the XML comment on AFW_TravelToCamp
	CButtonWnd* BShowAll  = nullptr;

	int m_sel  = -1;      // selected member row
	int m_kit  = 0;       // selected fire kind

	FellowshipWndAoT() : CCustomWnd("AoTFellowshipWnd")
	{
		SetWndNotification(FellowshipWndAoT);
		Members   = (CListWnd*)  GetChildItem("AFW_MemberList");
		KitList   = (CListWnd*)  GetChildItem("AFW_CampsiteKitList");
		KitDesc   = (CStmlWnd*)  GetChildItem("AFW_CampsiteDescription");
		MotdInput = (CXWnd*)     GetChildItem("AFW_MOTDInput");
		CampLoc   = (CXWnd*)     GetChildItem("AFW_CurrentCampLoc");
		CampView  = (CXWnd*)     GetChildItem("AFW_CampsiteViewer");

		BCreate   = (CButtonWnd*)GetChildItem("AFW_CreateFellowshipButton");
		BLeave    = (CButtonWnd*)GetChildItem("AFW_LeaveFellowshipButton");
		BLeader   = (CButtonWnd*)GetChildItem("AFW_MakeLeaderButton");
		BRemove   = (CButtonWnd*)GetChildItem("AFW_RemoveMemberButton");
		BInvite   = (CButtonWnd*)GetChildItem("AFW_InviteMemberButton");
		BEnd      = (CButtonWnd*)GetChildItem("AFW_EndFellowshipButton");
		BMotd     = (CButtonWnd*)GetChildItem("AFW_UpdateMOTDButton");
		BXp       = (CButtonWnd*)GetChildItem("AFW_ToggleExpSharingButton");
		BRefresh  = (CButtonWnd*)GetChildItem("AFW_RefreshList");
		BLight    = (CButtonWnd*)GetChildItem("AFW_CreateCampsite");
		BDouse    = (CButtonWnd*)GetChildItem("AFW_DestroyCampsite");
		BTravel   = (CButtonWnd*)GetChildItem("AFW_TravelToCamp");
		BShowAll  = (CButtonWnd*)GetChildItem("AFW_ShowAllButton");

		// 📌 XP sharing was declined by the design, and the campsite filter has nothing to filter --
		// three fires is the whole list. HIDDEN rather than left live: section 16 records that a
		// button which looks usable and silently does nothing reads as a broken window.
		// ⚠️ Hidden, not greyed: this build maps no reliable enable setter (EQClasses.h only
		// DECLARES CXWnd::IsEnabled), and writing a raw member offset is section 13's crash. Show IS
		// mapped.
		// ⚠️ Show is declared on CXWnd, not CButtonWnd -- cast, or "'Show': is not a member of
		// EQClasses::CButtonWnd".
		if (BXp)      { ((CXWnd*)BXp)->Show(0, 0); }
		if (BShowAll) { ((CXWnd*)BShowAll)->Show(0, 0); }

		FillKits();
		Refresh();
	}

	static void Cell(CListWnd* l, int row, int col, const char* s, COLORREF colour = 0xFFE0DCCD)
	{
		if (!l) { return; }
		CXStr v(s ? s : "");
		l->SetItemText(row, col, &v);
		// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with
		// SetItemText has no colour and the client draws it BLACK, very nearly invisible on this UI
		// (CLAUDE.md section 21). ⚠️ And 0xAARRGGBB -- a colour written 0x00RRGGBB is fully
		// TRANSPARENT, which is why the alpha byte is mandatory.
		l->SetItemColor(row, col, colour);
	}

	void FillKits()
	{
		if (!KitList) { return; }
		KitList->DeleteAll();
		for (int i = 0; i < kFireCount; ++i) {
			// ⚠️ AddString takes (const char*, COLORREF, Data, pTa, Tooltip) on this build -- passing a
			// CXStr* gives "no overloaded function could convert all the argument types".
			KitList->AddString(kFires[i].name, 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
		}
		SetKitDesc();
	}

	void SetKitDesc()
	{
		if (!KitDesc) { return; }
		const int i = (m_kit >= 0 && m_kit < kFireCount) ? m_kit : 0;
		char s[1400];
		sprintf_s(s, "<c \"#FFE0A0\">%s</c><br><br>%s", kFires[i].name, kFires[i].desc);
		KitDesc->SetSTMLText(CXStr(s), true, nullptr);
	}

	void Refresh()
	{
		if (Members) {
			Members->DeleteAll();
			for (int i = 0; i < g_count; ++i) {
				// leader is tinted so the roster reads at a glance, as the native window does
				// ⚠️⚠️ THE COLUMN INDICES ARE THE XML'S, AND THE XML IS THE NATIVE WINDOW'S:
				// 0 Name, 1 Lvl, 2 Class, 3 L, 4 Zone, 5 Last On, 6 XP Sharing. Writing the
				// leader flag into 1 and the zone into 2 -- which is what this did -- put
				// "Leader" under Lvl and "offline" under Class, so the roster looked scrambled
				// while every value in it was correct. Reported from play as exactly that.
				const COLORREF lead = g_leader[i] ? 0xFFFFD070 : 0xFFE0DCCD;
				const COLORREF tint = g_online[i] ? lead : 0xFF909090;
				Members->AddString(g_member[i], tint, (uint32_t)i, nullptr, nullptr);
				Cell(Members, i, 1, g_lvl[i],    tint);
				Cell(Members, i, 2, g_class[i],  tint);
				Cell(Members, i, 3, g_leader[i] ? "L" : "", lead);
				Cell(Members, i, 4, g_online[i] ? g_zone[i] : "", tint);
				Cell(Members, i, 5, g_laston[i], tint);
				// ⚠️ Column 6 "XP Sharing" is left blank on purpose -- the feature does not exist
				// here (the toggle button is hidden for the same reason). The COLUMN stays because
				// this layout is the native one byte for byte; inventing a value for it would be
				// advertising something the server does not do.
			}
		}

		if (CampLoc) {
			char s[160];
			if (g_fireZone[0]) { sprintf_s(s, "Current Camp Location: %s", g_fireZone); }
			else               { strcpy_s(s, "Current Camp Location: none"); }
			FsSetLabel(CampLoc, s);
		}
		if (CampView) {
			char s[160];
			if (g_fireZone[0]) { sprintf_s(s, "%s -- %d minute(s) left", g_fireType, g_fireMins); }
			else               { strcpy_s(s, "No campfire burning"); }
			FsSetLabel(CampView, s);
		}
		if (MotdInput && g_motd[0]) { FsSetLabel(MotdInput, g_motd); }
	}

	// The name a name-taking action should use: the selected roster row, else the MOTD/name box.
	void SelName(char* out, size_t n)
	{
		out[0] = 0;
		const int row = (m_sel >= 0) ? m_sel : (Members ? Members->GetCurSel() : -1);
		if (row >= 0 && row < g_count) { strncpy_s(out, n, g_member[row], _TRUNCATE); return; }
		FsGetText(MotdInput, out, n);
	}

	void Send(const char* verb, const char* arg = nullptr)
	{
		char cmd[256];
		if (arg && arg[0]) { sprintf_s(cmd, "/say %s %s", verb, arg); }
		else               { sprintf_s(cmd, "/say %s", verb); }
		AoTQueueGameCommand(cmd);
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_LCLICK) {
			char name[64];

			if (pWnd == (CXWnd*)BRefresh) { Send("fshipreq");   return 1; }
			if (pWnd == (CXWnd*)BLeave)   { Send("fshipleave"); return 1; }
			if (pWnd == (CXWnd*)BEnd)     { Send("fshipend");   return 1; }
			if (pWnd == (CXWnd*)BDouse)   { Send("fshipdouse"); return 1; }
			// ⚠️ Every gate (cooldown, combat, instance, region unlocked) is re-tested server
			// side -- pressing this from a locked region only earns a refusal in chat.
			if (pWnd == (CXWnd*)BTravel)  { Send("fshipgo"); return 1; }

			if (pWnd == (CXWnd*)BCreate) {
				// The MOTD box doubles as the name field before you have a fellowship -- the native
				// window has no separate one, and adding a piece would break the copied layout.
				FsGetText(MotdInput, name, sizeof(name));
				Send("fshipform", name[0] ? name : "Fellowship");
				return 1;
			}
			if (pWnd == (CXWnd*)BInvite) {
				// ⚠️ Blank is meaningful: the server falls back to the player's current TARGET, which
				// is how the native window invites. Typing a name still works.
				FsGetText(MotdInput, name, sizeof(name));
				Send("fshipinv", name);
				return 1;
			}
			if (pWnd == (CXWnd*)BRemove) { SelName(name, sizeof(name)); Send("fshipkick",   name); return 1; }
			if (pWnd == (CXWnd*)BLeader) { SelName(name, sizeof(name)); Send("fshipleader", name); return 1; }
			if (pWnd == (CXWnd*)BMotd) {
				FsGetText(MotdInput, name, sizeof(name));
				Send("fshipmotd", name);
				return 1;
			}
			if (pWnd == (CXWnd*)BLight) {
				const int i = (m_kit >= 0 && m_kit < kFireCount) ? m_kit : 0;
				Send("fshiplight", kFires[i].key);
				return 1;
			}

			// ⚠️⚠️ Cache the selection HERE and let the base class run FIRST. Inside a listbox's own
			// notification GetCurSel is one click behind, so reading it later in a button handler
			// acts on the PREVIOUSLY selected row -- removing the wrong member, silently.
			if (pWnd == (CXWnd*)Members && Members) {
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_sel = Members->GetCurSel();
				return handled;
			}
			if (pWnd == (CXWnd*)KitList && KitList) {
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				m_kit = KitList->GetCurSel();
				SetKitDesc();
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void FellowshipEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		FsTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- giving up",
		        pSidlMgr, pWndMgr);
		return;
	}

	if (!gFellowshipWnd) {
		gFellowshipWnd = new FellowshipWndAoT();
		// A NULL pXWnd means CCustomWnd could not find the screen "AoTFellowshipWnd", i.e.
		// EQUI_AoTFellowshipWnd.xml is not loaded -- the <Include> is missing from EQUI.xml or the
		// file was never copied. Silent otherwise, and the usual cause of "nothing happened".
		FsTrace("EnsureWindow: created FellowshipWndAoT, pXWnd=%p (NULL means the XML is not loaded)",
		        gFellowshipWnd ? (void*)gFellowshipWnd->pXWnd() : nullptr);
	}
	if (gFellowshipWnd && show) {
		gFellowshipWnd->Refresh();
		if (gFellowshipWnd->pXWnd()) { gFellowshipWnd->pXWnd()->Show(1, 1); }
	}
}

// ===================================================================== chat transport
// "FSHIPDATA <name>^leader|online|zone|charname^..."
static bool HandleFshipData(const char* msg)
{
	const char* t = strstr(msg, "FSHIPDATA ");
	if (!t) { return false; }
	t += strlen("FSHIPDATA ");

	g_count = 0;
	g_fname[0] = 0;

	const char* firstSep = strchr(t, '^');
	const size_t nameLen = firstSep ? (size_t)(firstSep - t) : strlen(t);
	const size_t nn = (nameLen < sizeof(g_fname) - 1) ? nameLen : sizeof(g_fname) - 1;
	memcpy(g_fname, t, nn);
	g_fname[nn] = 0;

	const char* p = firstSep;
	while (p && g_count < FS_MAX) {
		++p;
		const char* end = strchr(p, '^');
		const size_t len = end ? (size_t)(end - p) : strlen(p);

		char rec[256];
		const size_t n = (len < sizeof(rec) - 1) ? len : sizeof(rec) - 1;
		memcpy(rec, p, n);
		rec[n] = 0;

		// name|leader|level|class|zone|online|laston
		// ⚠️⚠️ ASSIGNED BY POSITION, AND NOTHING NAMES THE FIELDS. Insert one in the middle server
		// side without changing this and every column to its right silently shows the wrong value.
		// Keep in step with aotv4_fellowship.M.send_data.
		char* f[7] = { rec, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		int   nf   = 1;
		for (char* c = rec; *c && nf < 7; ++c) {
			if (*c == '|') { *c = 0; f[nf++] = c + 1; }
		}
		if (nf >= 7) {
			FsCopy(g_member[g_count], f[0]);
			g_leader[g_count] = atoi(f[1]);
			FsCopy(g_lvl[g_count],    f[2]);
			FsCopy(g_class[g_count],  f[3]);
			FsCopy(g_zone[g_count],   f[4]);
			g_online[g_count] = atoi(f[5]);
			FsCopy(g_laston[g_count], f[6]);
			++g_count;
		}
		p = end;
	}

	FellowshipEnsureWindow(false);
	if (gFellowshipWnd) { gFellowshipWnd->Refresh(); }
	return true;
}

// "FSHIPFIRE <zone>|<minutes>|<type>"  -- an empty zone means nothing is burning
static bool HandleFshipFire(const char* msg)
{
	const char* t = strstr(msg, "FSHIPFIRE ");
	if (!t) { return false; }
	t += strlen("FSHIPFIRE ");

	char rec[192];
	FsCopy(rec, t);
	char* f[3] = { rec, nullptr, nullptr };
	int   nf   = 1;
	for (char* c = rec; *c && nf < 3; ++c) {
		if (*c == '|') { *c = 0; f[nf++] = c + 1; }
	}
	FsCopy(g_fireZone, f[0] ? f[0] : "");
	g_fireMins = (nf > 1 && f[1]) ? atoi(f[1]) : 0;
	FsCopy(g_fireType, (nf > 2 && f[2]) ? f[2] : "");

	FellowshipEnsureWindow(false);
	if (gFellowshipWnd) { gFellowshipWnd->Refresh(); }
	return true;
}

// "FSHIPMOTD <text>"
static bool HandleFshipMotd(const char* msg)
{
	const char* t = strstr(msg, "FSHIPMOTD ");
	if (!t) { return false; }
	FsCopy(g_motd, t + strlen("FSHIPMOTD "));   // ⚠️ bounded: the MOTD is player-authored
	if (gFellowshipWnd) { gFellowshipWnd->Refresh(); }
	return true;
}

// ===================================================================== entry points
void InitFellowship()
{
	g_enabled = true;
	FsTrace("InitFellowship: enabled");
}

bool FellowshipParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	if (HandleFshipData(message)) { return true; }
	if (HandleFshipFire(message)) { return true; }
	if (HandleFshipMotd(message)) { return true; }
	return false;
}

// ⚠️ The dll's own "/say fship..." lines come back through dsp_chat as the player's own speech.
// Swallowing them keeps the protocol out of the chat window, exactly as AdvLoot and Autoskill do.
bool FellowshipIsOurEcho(const char* message)
{
	if (!g_enabled || !message) { return false; }
	// ⚠️⚠️ BOTH FORMS, AND MATCHING ONLY ONE IS BACKWARDS. Your OWN speech echoes as
	// "You say, 'fshipgo'" while everybody else's arrives as "Bob says, 'fshipgo'" -- and
	// "You say, '" does NOT contain "says, '" (there is no 's' after "say"). Keying on "says, '"
	// alone therefore swallowed the line on every OTHER player's screen and left it on the screen of
	// the person who pressed the button, which is the exact opposite of what was wanted. Reported
	// from play as the fellowship window still talking into /say.
	// 📌 Written as two whole-string tests rather than pointer arithmetic past the quote, so a player
	// whose NAME contains an apostrophe cannot shift the offset.
	return strstr(message, "You say, 'fship") != nullptr
	    || strstr(message, "says, 'fship")    != nullptr;
}

void FellowshipShow()
{
	if (!g_enabled) { return; }
	FellowshipEnsureWindow(true);
	// ⚠️ Ask for the data AFTER showing: the window is populated by the reply, and opening it empty
	// then filling it is what every other AoTv4 window does (the alternative is a visible blank
	// frame while the round trip completes).
	AoTQueueGameCommand("/say fshipreq");
}

// ⚠️⚠️ DELETE, do not merely null. /q tears the UI down through CleanGameUI; nulling alone leaks the
// CCustomWnd AND leaves it registered with the window manager after its widgets are freed, so the
// next thing to walk the window list touches freed memory (CLAUDE.md section 24).
void FellowshipOnUiReset()
{
	if (gFellowshipWnd) {
		delete gFellowshipWnd;
		gFellowshipWnd = nullptr;
	}
}
