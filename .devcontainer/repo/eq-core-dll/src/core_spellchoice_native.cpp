// core_spellchoice_native.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 level-up reward picker, rendered by the client's own UI engine.
//
// The old version drew itself: a layered GDI window with hand-painted rows, its own TGA icon
// decoder and its own hit-testing. That was originally necessary because we could not hook this
// client's D3D device -- but a SIDL CCustomWnd sidesteps the whole problem, and looks native
// because it IS native. See core_spellchoice_native.h for the module contract.
//
// Layout is three reward "cards" (icon, name, summary, button) over a shared description pane,
// not a listbox: a listbox cannot show a per-row icon on this build and its column header and
// selection bar looked nothing like the rest of EQ.
//
// ICONS: this build maps no runtime icon API (CTextureAnimation::SetCurCell,
// CSidlManager::FindAnimation and CListWnd::SetItemIcon are all absent from the address table),
// so the texture cell cannot be chosen from code. Instead EQUI_AoTSpellChoiceWnd.xml predefines
// one Ui2DAnimation per icon the reward pool can produce and one hidden button per (row, icon);
// we just Show the right one, and Show IS mapped.
//
// We do NOT keep a copy of the icon list here. It used to be a kIcons[] array that had to match
// the generator's @ICONS exactly, and that drifts the moment the pool changes -- which it did,
// going from a 113-spell custom set (22 icons) to the stock set (154). Instead the button is
// looked up BY NAME at runtime, "ASC_Icon<row>_<icon>", so the XML is the single source of truth
// and an icon it does not define simply draws nothing.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_spellchoice_native.h"
#include "core_advloot.h"      // AoTQueueGameCommand: run a /say on the GAME thread
#include "core_spelljournal.h" // the Known and Pool tabs; this window owns their widgets

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

// ===================================================================== state
static bool g_enabled = false;

static const int SC_MAX = 3;                 // the server always offers three
static int  g_count = 0;
static char g_name[SC_MAX][160] = {};
static int  g_icon[SC_MAX]      = {};
// ⚠️ 2048, not 512. The description is now Client::SearchDetail output -- name, mana, cast time,
// recast, range, learn level, resist, duration AND the wrapped db_str text -- not the one-line
// spell_desc.lua string it used to be. 512 silently truncated it mid sentence.
static char g_desc[SC_MAX][2048] = {};

// Copper cost of this character's next reroll, from SPELLREROLLCOST. 0 = the server has not said yet.
static int  g_rerollCost = 0;

// Tome of Insight decline state, from SPELLDECLINE <tier> <pct> <copper_after>.
//
// ⚠️⚠️ g_declineTier IS THE ONLY THING THAT DECIDES WHETHER THE BUTTON IS VISIBLE, and the server
// sends it with EVERY offer -- an explicit 0 for an ordinary level-up. Without that explicit reset
// the button would stay on screen after a tome offer was resolved and then silently do nothing,
// which reads as a broken window rather than as a rule.
static int  g_declineTier = 0;   // 0 = this offer cannot be declined
static int  g_declinePct  = 0;   // percent the reroll counter is cut by
static int  g_declineCost = 0;   // copper the next reroll would cost afterwards

static void SpellChoiceEnsure(bool show);

// ===================================================================== the window
static class SpellChoiceWnd* gSpellChoiceWnd = nullptr;

class SpellChoiceWnd : public CCustomWnd
{
public:
	CXWnd*      Title      = nullptr;
	CXWnd*      DetailName = nullptr;
	CStmlWnd*   Detail     = nullptr;
	CXWnd*      Name[SC_MAX] = {};
	CButtonWnd* Pick[SC_MAX] = {};
	CXWnd*      Icon[SC_MAX] = {};           // the icon button currently shown for this row
	// EVERY icon button belonging to each row. Needed because they default to VISIBLE in the XML:
	std::vector<CXWnd*> IconAll[SC_MAX];    // if we only hide the one we last showed, the other 150+
	                                        // stay drawn and every row renders the same stacked pile.

	int m_sel = -1;                          // previewed row; a second click on it commits

	// Known and Pool tabs. The widgets are ours; everything behind them lives in
	// core_spelljournal.cpp, which is the only part of the dll that reads the client's spell records.
	CListWnd*   KnownList   = nullptr;
	CListWnd*   KnownDetail = nullptr;
	CListWnd*   PoolList    = nullptr;
	CListWnd*   PoolDetail  = nullptr;
	CButtonWnd* PoolPrev    = nullptr;
	CButtonWnd* PoolNext    = nullptr;
	CXWnd*      PoolLevel   = nullptr;
	CButtonWnd* Reroll      = nullptr;
	CStmlWnd*   RerollCost  = nullptr;
	CButtonWnd* Decline     = nullptr;
	CStmlWnd*   DeclineInfo = nullptr;

	// Known tab: permanent library + the two actions. ⚠️ m_knownSel is cached because GetCurSel() is
	// one click behind inside the listbox's own notification -- the same trap the Death Book and
	// AdvLoot windows both hit.
	CStmlWnd*   KnownReq     = nullptr;
	CButtonWnd* KnownKeep    = nullptr;
	CButtonWnd* KnownUpgrade = nullptr;
	int         m_knownSel   = 0;   // base spell id selected on the Known tab

	SpellChoiceWnd() : CCustomWnd((char*)"AoTSpellChoiceWnd")
	{
		CloseOnESC = 0;                      // a reward is owed; do not let ESC lose it
		SetWndNotification(SpellChoiceWnd);

		char id[64];
		Title      =            GetChildItem((PCHAR)"ASC_Title");
		DetailName =            GetChildItem((PCHAR)"ASC_DetailName");
		Detail     = (CStmlWnd*)GetChildItem((PCHAR)"ASC_Detail");

		KnownList   = (CListWnd*)  GetChildItem((PCHAR)"ASC_KnownList");
		KnownDetail = (CListWnd*)  GetChildItem((PCHAR)"ASC_KnownDetail");
		PoolList    = (CListWnd*)  GetChildItem((PCHAR)"ASC_PoolList");
		PoolDetail  = (CListWnd*)  GetChildItem((PCHAR)"ASC_PoolDetail");
		PoolPrev    = (CButtonWnd*)GetChildItem((PCHAR)"ASC_PoolPrev");
		PoolNext    = (CButtonWnd*)GetChildItem((PCHAR)"ASC_PoolNext");
		PoolLevel   =              GetChildItem((PCHAR)"ASC_PoolLevel");
		Reroll      = (CButtonWnd*)GetChildItem((PCHAR)"ASC_Reroll");
		RerollCost  = (CStmlWnd*)  GetChildItem((PCHAR)"ASC_RerollCost");
		Decline     = (CButtonWnd*)GetChildItem((PCHAR)"ASC_Decline");
		DeclineInfo = (CStmlWnd*)  GetChildItem((PCHAR)"ASC_DeclineInfo");
		KnownReq     = (CStmlWnd*)  GetChildItem((PCHAR)"ASC_KnownReq");
		KnownKeep    = (CButtonWnd*)GetChildItem((PCHAR)"ASC_KnownKeep");
		KnownUpgrade = (CButtonWnd*)GetChildItem((PCHAR)"ASC_KnownUpgrade");
		for (int r = 0; r < SC_MAX; ++r) {
			sprintf_s(id, "ASC_Name%d", r); Name[r] =             GetChildItem(id);
			sprintf_s(id, "ASC_Pick%d", r); Pick[r] = (CButtonWnd*)GetChildItem(id);

			// Collect and hide every icon button this row owns. The XML defines one per possible icon
			// index and they start visible, so they must be found once and hidden up front -- the
			// generator caps icons at 251 (sheets Spells01-07), so that is the whole search space.
			for (int ic = 1; ic <= 251; ++ic) {
				sprintf_s(id, "ASC_Icon%d_%d", r, ic);
				if (CXWnd* w = GetChildItem(id)) {
					IconAll[r].push_back(w);
					w->Show(false, true);
				}
			}
		}
		Refresh();
	}

	static void SetVis(CXWnd* w, bool v) { if (w) { w->Show(v, true); } }
	static void SetText(CXWnd* w, const char* s) { if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); } }

	void Refresh()
	{
		SetText(Title, g_count > 0 ? "Choose your reward" : "No reward pending");

		for (int r = 0; r < SC_MAX; ++r) {
			const bool live = (r < g_count);

			SetVis(Name[r], live);
			SetVis((CXWnd*)Pick[r], live);

			// Exactly one icon button per row is visible: the one the server asked for. Hide
			// whatever the row showed last, then look this icon's button up by name. An icon the
			// XML does not define just leaves the slot empty, which is why no list is kept here.
			for (auto* w : IconAll[r]) { SetVis(w, false); }
			Icon[r] = nullptr;
			if (live && g_icon[r] > 0) {
				char id[64];
				sprintf_s(id, "ASC_Icon%d_%d", r, g_icon[r]);
				Icon[r] = GetChildItem(id);
				SetVis(Icon[r], true);
			}
			if (!live) { continue; }

			// The previewed row is marked in the name itself. Label colour would be the natural
			// way to do it, but there is no address-mapped setter for it on this build.
			char nm[192];
			sprintf_s(nm, "%s%s", (r == m_sel) ? "> " : "", g_name[r]);
			SetText(Name[r], nm);

			// Two step commit: the first click previews, the second confirms. Cheap insurance
			// against fat-fingering away a level's reward, which cannot be undone.
			SetText((CXWnd*)Pick[r], (r == m_sel) ? "Confirm" : "Select");
		}

		SetDetail();
		RefreshCost();
	}

	// The price box beside the Reroll button. Copper on the wire, shown in platinum: every price in
	// this system is a whole number of plat, and "5p" reads as a price where "5000" does not.
	void RefreshCost()
	{
		if (!RerollCost) { return; }
		char s[128];
		if (g_rerollCost > 0) {
			sprintf_s(s, "<c \"#FFDC96\">Reroll cost: %dp</c>", g_rerollCost / 1000);
		}
		else {
			// The server has not priced it yet. Say so rather than showing "0p", which reads as free.
			strcpy_s(s, "<c \"#A0A0A0\">Reroll cost: ...</c>");
		}
		CXStr t(s);
		RerollCost->SetSTMLText(t, true, nullptr);
		RefreshDecline();
	}

	// The Tome of Insight row: shown only while the offer on screen came from a tome.
	//
	// ⚠️ HIDDEN, not greyed. There is no address-mapped enable setter on this build (section 16), and
	// writing a raw ->Enabled member offset is the unreliable-struct-offset trap from section 13.
	// CXWnd::Show IS mapped, so visibility is the one lever that works reliably.
	void RefreshDecline()
	{
		const bool can = (g_declineTier > 0);

		SetVis((CXWnd*)Decline,     can);
		SetVis((CXWnd*)DeclineInfo, can);

		// ⚠️⚠️ REROLL AND DECLINE ARE MUTUALLY EXCLUSIVE. A tome's offer cannot be rerolled -- the
		// tome IS the reroll -- so the coin path is hidden for exactly as long as the tome path is
		// shown. Leaving Reroll up would put a live-looking button on screen that only ever answers
		// "a tome's offer cannot be rerolled", which reads as a broken window rather than as a rule.
		//
		// ⚠️⚠️ THE TWO PAIRS OCCUPY THE SAME COORDINATES in the generated XML -- one control row, two
		// possible occupants. That is only safe while this function is the ONLY thing that sets their
		// visibility and always sets them opposite. Show both and they draw on top of each other.
		SetVis((CXWnd*)Reroll,     !can);
		SetVis((CXWnd*)RerollCost, !can);

		if (!DeclineInfo || !can) { return; }

		char s[160];
		if (g_declinePct >= 100) {
			// Tier 3 zeroes the counter outright. Say what it DOES rather than "100 percent off",
			// which reads as "rerolls are free from now on" -- they are not, the ladder just restarts.
			sprintf_s(s, "<c \"#AADCFF\">Decline: rerolls back to %dp</c>", g_declineCost / 1000);
		}
		else {
			sprintf_s(s, "<c \"#AADCFF\">Decline: -%d%%, next %dp</c>", g_declinePct, g_declineCost / 1000);
		}
		CXStr t(s);
		DeclineInfo->SetSTMLText(t, true, nullptr);
	}

	void SetDetail()
	{
		const bool have = (m_sel >= 0 && m_sel < g_count);

		// The pane's title is its own Label, sharing the reward rows' font and colour, so it reads
		// as the same heading rather than as the first line of the body text.
		SetText(DetailName, have ? g_name[m_sel] : "");

		if (!Detail) { return; }
		CXStr t(have ? g_desc[m_sel]
		             : "Select a reward to read what it does, then click Confirm to learn it.");
		Detail->SetSTMLText(t, true, nullptr);
	}

	void Preview(int r)
	{
		if (r < 0 || r >= g_count) { return; }
		m_sel = r;
		Refresh();
	}

	void Commit(int r)
	{
		if (r < 0 || r >= g_count) { return; }
		char cmd[64];
		sprintf_s(cmd, "/say spellpick %d", r + 1);   // protocol is 1-based
		AoTQueueGameCommand(cmd);

		g_count = 0;                 // consumed; the server confirms and will not re-offer
		m_sel   = -1;
		pXWnd()->Show(0, 1);
	}

	// Keep/Upgrade only mean anything with a row selected, and Upgrade only for a rankable spell.
	// ⚠️ HIDDEN, not greyed: there is no address-mapped enable setter on this build (eqgame.h maps
	// none, and writing a raw ->Enabled member offset is the unreliable-struct-offset trap of
	// section 13). CXWnd::Show IS mapped. Same call the AdvLoot window makes.
	void ApplyKnownVis()
	{
		const bool have = (m_knownSel > 0);
		const int  rk   = have ? SjRankOf(m_knownSel) : 0;
		if (KnownKeep) {
			((CXWnd*)KnownKeep)->Show(have ? 1 : 0, 1);
			CXStr t(SjIsKept(m_knownSel) ? "Release" : "Keep");
			((CXWnd*)KnownKeep)->SetWindowTextA(t);
		}
		// rk 0 = not discovered, 5 = maxed. Hide rather than let a press fail silently.
		if (KnownUpgrade) { ((CXWnd*)KnownUpgrade)->Show((have && rk >= 1 && rk < 5) ? 1 : 0, 1); }
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_CLOSE) {
			// Closing without choosing just hides it; the server still owes the pick and will
			// re-offer. Nothing is consumed here.
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			for (int r = 0; r < SC_MAX; ++r) {
				if (pWnd == (CXWnd*)Pick[r]) {
					if (r == m_sel) { Commit(r); } else { Preview(r); }
					return 1;
				}
				if (Icon[r] && pWnd == Icon[r]) { Preview(r); return 1; }
			}

			// ---- Reroll: pay coin for three different rewards.
			//
			// ⚠️ THE CLIENT VALIDATES NOTHING HERE, AND MUST NOT. Pricing it, checking the player can
			// afford it and taking the money are all the server's job (spell_choice.reroll) -- the
			// dll's g_rerollCost is a DISPLAY value only, and a modified client must not be able to
			// reroll for free. Pressing this while broke simply gets a refusal in chat.
			//
			// ⚠️ Clear the preview selection: the three cards are about to be replaced, and a stale
			// m_sel would leave a row reading "Confirm" so the next click would LEARN the new reward
			// in that slot rather than previewing it.
			if (pWnd == (CXWnd*)Reroll) {
				m_sel = -1;
				AoTQueueGameCommand("/say spellreroll");
				return 1;
			}

			// ---- Decline: take no reward from a tome's offer, cut the reroll price instead.
			//
			// ⚠️ Same rule as Reroll -- the client decides nothing. spell_choice.decline re-checks
			// that the queue's FRONT offer actually carries a tome tag, so a modified client that
			// un-hides this button still cannot decline a free level-up reward.
			//
			// ⚠️ Clear the preview for the same reason Reroll does: the offer is about to vanish, and
			// a stale m_sel would leave a card reading "Confirm" over whatever replaces it.
			// ⚠️ It tears the window down exactly as Commit does, NOT as Reroll does. Reroll leaves
			// the window up because three new cards are on their way; declining consumes the offer
			// and produces nothing, so leaving the old cards on screen would show a reward that is
			// no longer claimable. g_declineTier is cleared too, or reopening on a queued level-up
			// offer would briefly show a Decline button that offer can never honour.
			if (pWnd == (CXWnd*)Decline) {
				AoTQueueGameCommand("/say spelldecline");
				g_count       = 0;
				g_declineTier = 0;
				m_sel         = -1;
				pXWnd()->Show(0, 1);
				return 1;
			}

			// ---- Known / Pool tabs.
			// ⚠️ Row index is NOT a spell index on either list: Known interleaves a band header every
			// ten levels. core_spelljournal keeps the row -> id maps and returns 0 for a header.
			if (pWnd == (CXWnd*)PoolPrev) { SjStepLevel(-1); return 1; }
			if (pWnd == (CXWnd*)PoolNext) { SjStepLevel(+1); return 1; }
			if (pWnd == (CXWnd*)KnownList && KnownList) {
				// ⚠️ Cache the selection HERE. GetCurSel() is one click behind inside the listbox's own
				// notification, so reading it later in the button handler would act on the previously
				// selected spell -- keeping or upgrading the wrong thing, silently.
				m_knownSel = SjKnownSpellAtRow(KnownList->GetCurSel());
				SjShowDetail(KnownDetail, m_knownSel);
				// ⚠️ Drop any server status line first, or a stale "you cannot afford that" sits over
				// the requirements for a spell the player has since clicked away from.
				SjClearRankMessage();
				SjRankReqText(KnownReq, m_knownSel);
				ApplyKnownVis();
				return 1;
			}
			if (pWnd == (CXWnd*)KnownKeep && m_knownSel > 0) {
				// Keep is a TOGGLE: the label already says which way it will go.
				char cmd[64];
				sprintf_s(cmd, SjIsKept(m_knownSel) ? "/say spellrelease %d" : "/say spellkeep %d", m_knownSel);
				AoTQueueGameCommand(cmd);
				return 1;
			}
			if (pWnd == (CXWnd*)KnownUpgrade && m_knownSel > 0) {
				// ⚠️ The dll validates NOTHING. It sends an id; the server owns the cost, the
				// affordability check and the 2-keep cap, so a modified client cannot rank for free.
				char cmd[64];
				sprintf_s(cmd, "/say spellrank %d", m_knownSel);
				AoTQueueGameCommand(cmd);
				return 1;
			}
			if (pWnd == (CXWnd*)PoolList && PoolList) {
				SjShowDetail(PoolDetail, SjPoolSpellAtRow(PoolList->GetCurSel()));
				return 1;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

// Rank/library data arrived. Rebuild the Known list and repaint the requirement panel for whatever
// is selected, so a Keep or Upgrade shows its result immediately rather than at the next open.
// ⚠️ Lives here rather than in core_spelljournal because the WIDGETS belong to this window; that
// module owns the data and this one owns the screen, which is the split the whole Known/Pool
// arrangement already uses.
bool SpellChoiceRankTransport(const char* msg)
{
	if (!SjParseRankTransport(msg)) { return false; }
	if (gSpellChoiceWnd) {
		SjRefreshKnown(gSpellChoiceWnd->KnownList);
		// ⚠️ A server status line WINS over the requirement text. Both land in the same panel, and the
		// state refresh arrives immediately after the message (send_state follows every mutation), so
		// painting requirements unconditionally would wipe the answer before it was read.
		if (SjHasRankMessage()) { SjRankMsgText(gSpellChoiceWnd->KnownReq); }
		else                    { SjRankReqText(gSpellChoiceWnd->KnownReq, gSpellChoiceWnd->m_knownSel); }
		gSpellChoiceWnd->ApplyKnownVis();
	}
	return true;
}

// This dll historically drew everything itself, so MQ2's UI managers are not reliably wired --
// CCustomWnd silently bails at "if (!pSidlMgr || !pWndMgr)" and the window never appears. Set them
// from the client globals if unset. Every native window in this dll has to do this.
static void SpellChoiceEnsure(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) return;

	if (!gSpellChoiceWnd) {
		gSpellChoiceWnd = new SpellChoiceWnd();

		// Repaint the Pool list once a chunked answer is complete. The transport cannot call the
		// window directly -- core_spelljournal deliberately knows nothing about it -- so it calls back.
		// ⚠️⚠️ DO NOT redraw BOTH panes here. That is what made the description flash: this fires when
		// SJINFO lands, and redrawing the other pane too asked for ITS selected spell, whose answer fired
		// the callback again, which asked for the first one again -- an endless ping-pong of chat lines
		// with the pane repainting on every lap. core_spelljournal remembers which pane asked and
		// redraws only that one.
		SjSetInfoReadyCallback([] { SjRedrawPendingDetail(); });

		// SJLEVELS is a different answer and needs a different action: the Known LIST is what carries the
		// levels, not the detail pane, so rebuild the list.
		SjSetLevelsReadyCallback([] {
			if (gSpellChoiceWnd) { SjRefreshKnown(gSpellChoiceWnd->KnownList); }
		});
		SjSetPoolReadyCallback([] {
			if (gSpellChoiceWnd) {
				SjRefreshPool(gSpellChoiceWnd->PoolList, gSpellChoiceWnd->PoolLevel);
			}
		});
	}
	if (gSpellChoiceWnd && show) {
		gSpellChoiceWnd->Refresh();

		// The two browse tabs are filled whenever the window opens, whatever opened it.
		// ⚠️ Known is NO LONGER FREE. It used to read CHARINFO2::SpellBook and cost nothing, but it is
		// now the permanent library -- discovered spells and their ranks live in server data buckets,
		// so most of the list is spells the character is not currently carrying. One request on open,
		// never polled: section 15 records RoF2 dropping and reordering chat bursts.
		AoTQueueGameCommand("/say sjlevels");      // pick levels for the Known tab
		AoTQueueGameCommand("/say spellrankreq");  // permanent library + ranks + materials
		SjRefreshKnown(gSpellChoiceWnd->KnownList);
		if (SjLevel() <= 0) {
			PCHARINFO2 ci2 = GetCharInfo2();
			SjRequestLevel(ci2 ? (int)ci2->Level : 1);
		}

		gSpellChoiceWnd->pXWnd()->Show(1, 1);
	}
}

// ===================================================================== chat transport
// "SPELLCHOICEDATA name|icon^name|icon^name|icon"
static bool HandleChoiceData(const char* msg)
{
	const char* t = strstr(msg, "SPELLCHOICEDATA ");
	if (!t) return false;
	t += strlen("SPELLCHOICEDATA ");

	g_count = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_count < SC_MAX) {
				const size_t bar = cur.find('|');
				const std::string nm = (bar == std::string::npos) ? cur : cur.substr(0, bar);
				g_icon[g_count] = (bar == std::string::npos) ? 0 : atoi(cur.c_str() + bar + 1);
				strncpy_s(g_name[g_count], sizeof(g_name[0]), nm.c_str(), _TRUNCATE);
				g_desc[g_count][0] = 0;               // cleared until SPELLDESCDATA arrives
				g_count++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
		else { cur += *p; }
	}

	if (g_count > 0) {
		if (gSpellChoiceWnd) gSpellChoiceWnd->m_sel = -1;
		SpellChoiceEnsure(true);
	}
	return true;
}

// Sibling line to the choices, arriving AFTER them, so the row summaries are rebuilt here rather
// than only the description pane.
//
// TWO FORMATS, both accepted:
//   "SPELLDESCDATA <n> <text>"  -- ONE choice per line, n is 1 based. Preferred.
//   "SPELLDESCDATA d1^d2^d3"   -- all three on one line. The original.
//
// ⚠️ The indexed form exists because the descriptions grew: they are now full SearchDetail blocks
// rather than a single sentence, and three of them on one chat line runs past what the client will
// carry -- which truncates SILENTLY, so the third choice would simply lose its text. Both forms are
// read because the dll and the server are deployed separately and either may be older.
static bool HandleDescData(const char* msg)
{
	const char* t = strstr(msg, "SPELLDESCDATA ");
	if (!t) return false;
	t += strlen("SPELLDESCDATA ");

	// indexed form: a digit, a space, then the whole rest of the line is that choice's text
	if (t[0] >= '1' && t[0] <= '9' && t[1] == ' ') {
		const int i = t[0] - '1';
		if (i >= 0 && i < SC_MAX) {
			strncpy_s(g_desc[i], sizeof(g_desc[0]), t + 2, _TRUNCATE);
		}
		if (gSpellChoiceWnd) { gSpellChoiceWnd->Refresh(); }
		return true;
	}

	int         idx = 0;
	std::string cur;
	for (const char* p = t; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (idx < SC_MAX) { strncpy_s(g_desc[idx], sizeof(g_desc[0]), cur.c_str(), _TRUNCATE); }
			++idx;
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
		else { cur += *p; }
	}

	if (gSpellChoiceWnd) { gSpellChoiceWnd->Refresh(); }
	return true;
}

// "SPELLREROLLCOST <copper>" -- what THIS character's next reroll costs.
//
// ⚠️ The price escalates per character, so the client cannot derive it and must not guess: a
// hardcoded figure would be right exactly once, for a player who has never rerolled. It arrives with
// every offer and again after every reroll.
static bool HandleRerollCost(const char* msg)
{
	const char* t = strstr(msg, "SPELLREROLLCOST ");
	if (!t) { return false; }
	t += strlen("SPELLREROLLCOST ");

	g_rerollCost = atoi(t);
	if (gSpellChoiceWnd) { gSpellChoiceWnd->RefreshCost(); }
	return true;
}

// "SPELLDECLINE <tier> <pct> <copper_after>" -- can THIS offer be declined, and for what.
//
// ⚠️⚠️ IT ARRIVES WITH EVERY OFFER, INCLUDING AS A ZERO. Treating a missing line as "no change"
// would leave the Decline button up after a tome offer was resolved, on an ordinary level-up reward
// that can never be declined -- a live-looking button that only ever produces a refusal.
static bool HandleDeclineData(const char* msg)
{
	const char* t = strstr(msg, "SPELLDECLINE ");
	if (!t) { return false; }
	t += strlen("SPELLDECLINE ");

	g_declineTier = 0;
	g_declinePct  = 0;
	g_declineCost = 0;
	sscanf_s(t, "%d %d %d", &g_declineTier, &g_declinePct, &g_declineCost);

	if (gSpellChoiceWnd) { gSpellChoiceWnd->RefreshDecline(); }
	return true;
}

bool SpellChoiceParseTransport(const char* message)
{
	if (!g_enabled || !message) return false;
	if (HandleChoiceData(message)) return true;
	if (HandleDescData(message))   return true;
	if (HandleRerollCost(message)) return true;
	if (HandleDeclineData(message)) return true;
	return false;
}

// ===================================================================== module entry points
void InitSpellChoiceNative() { g_enabled = true; }

bool SpellChoicePending() { return g_enabled && g_count > 0; }

void SpellChoiceShow()
{
	if (!g_enabled || g_count <= 0) return;   // nothing pending to choose
	SpellChoiceEnsure(true);
}

// Open with NO reward pending, for browsing the Known and Pool tabs. Before those tabs existed there
// was nothing to look at without an offer, which is why SpellChoiceShow refuses -- that refusal is
// kept as it is, so the Ctrl+Q "is a reward owed" path still means exactly what it did.
void SpellChoiceOpen()
{
	if (!g_enabled) return;
	SpellChoiceEnsure(true);
}

void SpellChoiceOnUiReset()
{
	if (gSpellChoiceWnd) { delete gSpellChoiceWnd; gSpellChoiceWnd = nullptr; }
}
