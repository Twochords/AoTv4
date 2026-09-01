// core_aotmenu.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 launcher. See core_aotmenu.h for the module contract.
//
// Every button here opens a window that already existed; nothing new is implemented. Two different
// mechanisms are used deliberately:
//
//   direct call  -- for windows whose module exposes a show function we can link against.
//   queued /say  -- for windows that are opened by asking the SERVER for their contents. Calling
//                   their internal show would put an empty window on screen; the command is the
//                   real entry point because the data arrives with the reply.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_aotmenu.h"
#include "core_advloot.h"             // AdvLootShow + AoTQueueGameCommand
#include "core_autoskill.h"           // AutoSkillShow
#include "core_dungeon.h"             // DungeonShow
#include "core_difficulty.h"
#include "core_fellowship.h" // FellowshipShow() -- the Fellowship window
#include "core_fctwindow.h" // FctWindowShow() -- floating combat text settings
#include "core_meter.h"     // MeterWindowShow() -- the damage meter
#include "core_travel.h"   // TravelShowBrowse() -- the Travel window, opened read only from the menu          // DifficultyShow
#include "core_spellchoice_native.h"  // SpellChoiceOpen
#include "core_lostwindow.h"          // LostWindowShow

#include <cstdio>
#include <cstdarg>

// Defined in core_spellwindow.cpp; declared here rather than pulling that whole header in.
void ShowSearchWindow();

static bool g_enabled = false;

// Debug trace to <EQ>\aotv4_menu.log. Every way this window can fail to appear -- dll not rebuilt,
// XML not copied, <Include> missing, called before the UI is up -- looks IDENTICAL from in game:
// nothing happens, with no error anywhere. This says which one it was.
static void AoTTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_menu.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_menu.log"); }

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

static bool g_visible = false;   // is our window currently on screen

// ⚠️⚠️ THE PLAYER'S CHOICE OUTLIVES THE WINDOW, WHICH IS WHY THIS IS A FILE-SCOPE FLAG AND NOT
// WINDOW STATE. Zoning tears the UI down: CleanGameUI fires, AoTMenuOnUiReset DELETES the window,
// and the server's AOTMENUSHOW then builds a fresh one -- expanded, every single time. The server
// comment called that safe because "showing an already-open window is harmless", which is true of
// an open window and wrong about one the player deliberately closed. Reported from play as the menu
// re-expanding on every zone.
// Living outside the window is the whole point: it survives the teardown that loses everything else.
// ⚠️ Session-scoped, deliberately. It is a UI preference, not character data -- there is nowhere to
// persist it that would not mean this dll writing its own settings file.
static bool g_userClosed = false;   // the player closed it; leave it closed until they ask again

static void AoTMenuEnsureWindow(bool show);

class AoTMenuWnd : public CCustomWnd
{
public:
	CButtonWnd* Spells   = nullptr;
	CButtonWnd* AutoSkill = nullptr;
	CButtonWnd* Loot     = nullptr;
	CButtonWnd* Trader   = nullptr;
	CButtonWnd* Achieve  = nullptr;
	CButtonWnd* Search   = nullptr;
	CButtonWnd* Lost     = nullptr;
	CButtonWnd* Delve    = nullptr;
	CButtonWnd* Difficulty = nullptr;
	CButtonWnd* CombatText = nullptr;
	CButtonWnd* Meter      = nullptr;
	CButtonWnd* Travel     = nullptr;
	CButtonWnd* Fellowship = nullptr;

	AoTMenuWnd() : CCustomWnd((char*)"AoTMenuWnd")
	{
		// ⚠️ CloseOnESC = 0, and <Escapable>false</Escapable> in the XML. This is a PERSISTENT bar, like
		// the stock SC / EQ window -- and players press Escape constantly, which was silently closing it.
		CloseOnESC = 0;
		SetWndNotification(AoTMenuWnd);
		Spells    = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Spells");
		AutoSkill = (CButtonWnd*)GetChildItem((PCHAR)"AMW_AutoSkill");
		Loot      = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Loot");
		Trader    = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Trader");
		Achieve   = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Achievements");
		Search    = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Search");
		Lost      = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Lost");
		Delve     = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Delve");
		Difficulty = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Difficulty");
		CombatText = (CButtonWnd*)GetChildItem((PCHAR)"AMW_CombatText");
		Meter      = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Meter");
		Travel    = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Travel");
		Fellowship = (CButtonWnd*)GetChildItem((PCHAR)"AMW_Fellowship");
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		// Track the close so g_visible stays truthful.
		// ⚠️ Record the INTENT, not just the visibility. g_visible is recomputed every time the window
		// is rebuilt; g_userClosed is what stops it being rebuilt open.
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			g_visible    = false;
			g_userClosed = true;
			AoTTrace("closed by player -- AOTMENUSHOW will be ignored until /aot");
			return 1;
		}


		if (Message == XWM_LCLICK) {
			// ⚠️ TRACE BEFORE THE CALL, NOT AFTER. Each of these opens a different window owned by a
			// different module, and if one of them faults the client dies immediately -- an "after"
			// line would never be written and the log would look identical whichever button it was.
			// Logging first means the LAST line in the file names the culprit.
			//
			// ---- windows we can open directly.
			if (pWnd == (CXWnd*)Spells)    { AoTTrace("click: Spells");     SpellChoiceOpen(); AoTTrace("click: Spells done");     return 1; }
			if (pWnd == (CXWnd*)AutoSkill) { AoTTrace("click: Autoskill");  AutoSkillShow();   AoTTrace("click: Autoskill done");  return 1; }
			if (pWnd == (CXWnd*)Loot)      { AoTTrace("click: AdvLoot");    AdvLootShow();     AoTTrace("click: AdvLoot done");    return 1; }
			if (pWnd == (CXWnd*)Search)    { AoTTrace("click: Search");     ShowSearchWindow();AoTTrace("click: Search done");     return 1; }
			if (pWnd == (CXWnd*)Lost)      { AoTTrace("click: LastDeath");  LostWindowShow();  AoTTrace("click: LastDeath done");  return 1; }
			// Delve belongs in this group even though its list comes from the server: DungeonShow()
			// opens the window AND queues "/say delve" itself, so it is never shown empty. That is
			// why it is not in the command group below.
			if (pWnd == (CXWnd*)Delve)     { AoTTrace("click: Delve");      DungeonShow();     AoTTrace("click: Delve done");      return 1; }
			// Difficulty is in this group for the same reason as Delve: DifficultyShow() opens the
			// window AND queues "/say diffwin" itself, so it is never shown empty.
			if (pWnd == (CXWnd*)Difficulty) { AoTTrace("click: Difficulty"); DifficultyShow(); AoTTrace("click: Difficulty done"); return 1; }
			// Combat Text belongs in the "open directly" group: every setting it shows is client side
			// and already in memory, so there is nothing to ask the server for first.
			if (pWnd == (CXWnd*)CombatText) { AoTTrace("click: CombatText"); FctWindowShow(); AoTTrace("click: CombatText done"); return 1; }
			// Damage Meter is in the "open directly" group even though its rows come from the server:
			// MeterWindowShow() opens the window AND asks for the data, so it is never shown empty.
			if (pWnd == (CXWnd*)Meter) { AoTTrace("click: Meter"); MeterWindowShow(); AoTTrace("click: Meter done"); return 1; }
			// ⚠️⚠️ BROWSE ONLY. The Travel window's whole design is that the PoK book is the terminal
			// -- opening it from here would make the book irrelevant. TravelShowBrowse() opens it with
			// the Travel button hidden, so it reads as a map of what you have found rather than a way
			// to move. Clicking a book still opens it able to travel.
			// ⚠️ The hiding is PRESENTATION. The server re-checks that a book was clicked recently and
			// refuses otherwise, so a modified client that un-hides the button gains only a refusal --
			// the same rule §16 records for the Advanced Loot buttons.
			if (pWnd == (CXWnd*)Travel) { AoTTrace("click: Travel (browse)"); TravelShowBrowse(); return 1; }
			// Fellowship is in the "open directly" group: FellowshipShow() opens the window AND asks the
			// server for the roster itself, so it is never shown empty.
			if (pWnd == (CXWnd*)Fellowship) { AoTTrace("click: Fellowship"); FellowshipShow(); return 1; }

			// ---- windows whose CONTENTS come from the server.
			//
			// ⚠️ These must go through the command, not through the module's show function. The shop
			// window is filled by SHOPINVDATA and the achievement window by "ACH|window|show", both
			// of which are REPLIES -- opening the window directly would show an empty one, and for the
			// shop it would show whatever stale listing was last received.
			//
			// Queued rather than called inline because a /say has to run on the GAME thread; this
			// notification arrives on it, but AoTQueueGameCommand is how every other module does it
			// and keeping one path means one place to get it wrong.
			if (pWnd == (CXWnd*)Trader)  { AoTTrace("click: Trader");       AoTQueueGameCommand("/trader"); return 1; }
			if (pWnd == (CXWnd*)Achieve) { AoTTrace("click: Achievements"); AoTQueueGameCommand("/ach");    return 1; }
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

static AoTMenuWnd* gAoTMenuWnd = nullptr;

// ⚠️ THERE IS DELIBERATELY NO DOCKING. An earlier version read EQMainWnd's screen rect through
// pinstCEQMainWnd and parked this window beside it. That tied the menu to a bar it has nothing to do
// with: the position moved on its own, it could land off screen when the bar sat at X=0, and it
// overrode wherever the player had dragged it. The window is STANDALONE -- it opens where the XML
// says the first time and the client remembers wherever it is moved to after that.

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently
// bails at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise. See CLAUDE.md section 15.
static void AoTMenuEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) {
		AoTTrace("EnsureWindow: UI managers still null (pSidlMgr=%p pWndMgr=%p) -- too early",
		         (void*)pSidlMgr, (void*)pWndMgr);
		return;
	}

	if (!gAoTMenuWnd) {
		gAoTMenuWnd = new AoTMenuWnd();

		// ⚠️⚠️ ASK THE CLIENT FOR THE STATE IT ALREADY SAVED. `CCustomWnd`'s constructor builds the
		// window from the XML and immediately does `pXWnd()->Show(1,1)` -- it never calls LoadIniInfo,
		// and neither does any other window in this dll. So every rebuild started from the XML defaults
		// and threw away UI_<char>_<server>.ini. Since CleanGameUI destroys this window on EVERY ZONE,
		// that is why a minimised bar came back expanded every time you zoned.
		// The client does save it: `Minimized` is a real ini key in eqgame.exe, alongside XPos, YPos,
		// Width, Height, Locked and Alpha. We were simply never reading it back.
		// ⚠️ LoadIniInfo is address-bound (CSidlScreenWnd__LoadIniInfo_x, 0x85DDA0) -- unlike Minimize()
		// and GetMinimizedRect(), which are declared in EQClasses.h but absent from eqgame.h and
		// therefore unusable (section 13). This is the only route to the minimised state on this build.
		if (gAoTMenuWnd->pXWnd()) { gAoTMenuWnd->LoadIniInfo(); }

		// A NULL pXWnd here means CCustomWnd could not find the screen "AoTMenuWnd", i.e.
		// EQUI_AoTMenuWnd.xml is not loaded -- the <Include> is missing from EQUI.xml or the file was
		// never copied. That is by far the most common cause of "nothing happened", and without this
		// line it is completely silent: no error, no window, nothing in any client log.
		AoTTrace("EnsureWindow: created AoTMenuWnd, pXWnd=%p (NULL means the XML is not loaded)",
		         gAoTMenuWnd ? (void*)gAoTMenuWnd->pXWnd() : nullptr);
	}

	if (show && gAoTMenuWnd && gAoTMenuWnd->pXWnd()) {
		// ⚠️ Show() controls VISIBILITY, not the collapsed state, so this is safe to call on a window
		// LoadIniInfo just restored as minimised -- a minimised window is still shown, it is only
		// collapsed to its title bar. Do NOT "fix" a future problem here by adding anything that
		// re-expands it; that is precisely what this change removes.
		gAoTMenuWnd->pXWnd()->Show(1, 1);
		g_visible = true;
		AoTTrace("EnsureWindow: shown");
	}
}

// =================================================================================================
// Auto show, once we are actually in game.
//
// ⚠️⚠️ THIS REPLACES A VTABLE PATCH ON EQMainWnd, AND THAT PATCH MUST NOT COME BACK. An earlier
// version added an AoT button to the stock SC / EQ bar and caught its click by copying EQMainWnd's
// vtable and overwriting entry 34 (the slot SetWndNotification writes). It CRASHED on the first
// click. Whatever the precise fault, the approach is not worth re-attempting: it patches a window
// this dll did not construct, on a build whose UI structs are known not to match the headers
// (CLAUDE.md section 13), and it can only fail at runtime. The list is our own window instead, which
// gets notifications the ordinary way and cannot corrupt anything stock.
//
// ⚠️⚠️ THE WINDOW IS OPENED BY A SERVER CHAT LINE, NOT BY A CLIENT-SIDE "am I in game" TEST.
// Both obvious tests were tried and BOTH ARE WRONG in this dll:
//
//   pinstLocalPlayer != null -- non-null AT CHARACTER SELECT too, because char select renders the
//       selected character. The client's own code relies on that combination
//       (eqgame.cpp:198: `pLocalPlayer && GetGameState() == GAMESTATE_CHARSELECT`).
//       Symptom: the menu sat on the character select screen.
//   gGameState == GAMESTATE_INGAME -- the canonical MQ2 test, but gGameState is ONLY ever assigned
//       in MQ2DetourAPI.cpp, and this dll runs with isMQInjectsEnabled = false, so it is never
//       updated and the comparison is permanently false. Symptom: the menu never appeared at all.
//
// A chat line cannot arrive before you are in the world, so the server saying so is both simpler and
// strictly more reliable than either. It is also the pattern every other window in this dll uses.
//
// ⚠️ If a third in-game test is ever needed here, DO NOT reach for a client global -- verify it at
// character select first, and remember that MQ2's own state tracking is off in this build.
bool AoTMenuParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	if (!strstr(message, "AOTMENUSHOW")) { return false; }

	// ⚠️⚠️ THE SERVER SENDS THIS ON EVERY ZONE, BY DESIGN -- global_player.event_enter_zone. It has to,
	// because zoning destroys the window and someone who wants the bar up expects it back. So the
	// decision about whether to honour it belongs HERE, where we know what the player last did.
	if (g_userClosed) {
		AoTTrace("AOTMENUSHOW ignored -- player closed the menu");
		return true;   // still swallowed: it must never reach the chat window
	}

	AoTTrace("AOTMENUSHOW received -- opening");
	AoTMenuShow();
	return true;   // swallowed: never let it reach the chat window
}

// Asking for it explicitly (/aot, /menu) is what clears the flag -- that IS the player re-opening it.
void AoTMenuShow()
{
	if (!g_enabled) { return; }
	g_userClosed = false;
	AoTMenuEnsureWindow(true);
}

void AoTMenuOnUiReset()
{
	// ⚠️ DELETE, do not just null. An earlier version here only dropped the pointer, on the assumption
	// that the engine owns the underlying CXWnd and deleting would double free. That assumption is
	// contradicted by every other window in this dll -- core_spellchoice_native, core_advloot,
	// core_autoskill and core_lostwindow all delete here and have been in production doing so. A
	// CCustomWnd IS the window and we allocated it, and ~CCustomWnd runs RemovevfTable() to put the
	// original vtable back; skipping that leaks the vtable copy on every UI reload.
	// ⚠️⚠️ SAVE BEFORE DELETING, AND IN THAT ORDER. This is the only moment the current position and
	// minimised state still exist -- ~CCustomWnd runs RemovevfTable() and the window is gone. Without
	// this the ini keeps whatever the client last wrote on its own schedule, which is not necessarily
	// what the player just did.
	if (gAoTMenuWnd && gAoTMenuWnd->pXWnd()) { gAoTMenuWnd->StoreIniInfo(); }
	if (gAoTMenuWnd) { delete gAoTMenuWnd; gAoTMenuWnd = nullptr; }
	g_visible = false;

	// ⚠️ g_userClosed is NOT reset here. This runs on every zone and every /loadskin, and clearing it
	// would put the menu back on screen -- which is exactly the bug this flag exists to fix.

}

void InitAoTMenu()
{
	g_enabled = true;
	// First line in the log, written from DllMain. Its presence proves the rebuilt dll is actually
	// loaded, which separates "not rebuilt / not copied" from every other failure below it.
	AoTTrace("InitAoTMenu: enabled");
}
