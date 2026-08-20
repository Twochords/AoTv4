// core_spelljournal.cpp
// ---------------------------------------------------------------------------------------------
// AoTv4 Known and Pool panels of the spell window. See core_spelljournal.h for the contract.
//
// ⚠️ This module owns NO window. core_spellchoice_native.cpp owns the three-tab spell window and
// every widget in it; this fills the two new tabs.
//
// ⚠️ The Known tab USED to be free -- it read CHARINFO2::SpellBook and asked the server nothing. It
// is now the PERMANENT LIBRARY: every spell this character has ever been awarded, with the rank it
// has been taken to, both of which live in server data buckets and survive the roguelite wipe. A
// spellbook view would hide every spell the player holds a rank on but is not currently carrying,
// which after a death is nearly all of them. One request on open (SPELLRANKDATA), never polled.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_spelljournal.h"
#include "core_advloot.h"   // AoTQueueGameCommand: run a /say on the GAME thread

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdarg>

// ===================================================================== state
static bool g_enabled = false;

// Pool tab. 125 is the largest single level in the current pool (level 70); the headroom is so a
// pool regen that grows a level cannot silently overflow.
static const int SJ_MAX_POOL = 256;
static int  g_poolLevel = 0;
static int  g_poolCount = 0;
static int  g_poolId   [SJ_MAX_POOL] = {};
static bool g_poolKnown[SJ_MAX_POOL] = {};

// Combat abilities share the Pool tab with spells. Their skill ids are 0-76, which collide with real
// spell ids, so the wire lifts them clear by this offset and they carry their own name -- the client
// has no skill-name table and a hardcoded one would drift from skill_pool.lua (the kIcons lesson).
// ⚠️ MUST match SKILL_ID_BASE in quests/lua_modules/spell_choice.lua.
static const int SJ_SKILL_BASE = 1000000;
static char g_poolName[SJ_MAX_POOL][40] = {};   // set for ability rows only; empty for spells
static int  g_poolChunksSeen = 0;
static int  g_poolChunksWant = 0;

// Known tab. The RoF2 spellbook is NUM_BOOK_SLOTS (0x2d0 = 720) slots.
//
// ⚠️ The ROW map must be bigger than the spellbook, not the same size: the list interleaves a band
// header row every ten levels, so a full book produces up to 720 spells PLUS 9 headers. Sizing the
// map at 720 and guarding one row at a time overflows it by the header count.
static const int SJ_MAX_KNOWN_ROWS = NUM_BOOK_SLOTS + 16;

// Debug trace to <EQ>\aotv4_spelljournal.log. Same reason as native_achievements.log and
// aotv4_autoskill.log: when a native window fails to appear there is NOTHING in any client log to
// say why, and a missing EQUI.xml Include, an unbuilt dll and a failed CCustomWnd construction all
// look identical from in game.
static void SjTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_spelljournal.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_spelljournal.log"); }

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

// ===================================================================== spell text
//
// Everything below builds description text out of the client's OWN spell record. This is the one
// technique worth taking from the earlier AoT spell book, and it removes a standing problem: today
// the picker's text comes from spell_desc.lua over SPELLDESCDATA while the spellbook's comes from
// db_str via an exported dbstr_us.txt, and CLAUDE.md section 14 records keeping those two in sync as
// an ongoing hazard. Text derived from the spell itself cannot drift from it and costs no traffic.

static const COLORREF C_BUFF   = 0xFF88FF88;
static const COLORREF C_DEBUFF = 0xFFFF8888;
static const COLORREF C_PLAIN  = 0xFFFFFFFF;
static const COLORREF C_LABEL  = 0xFFAAAAAA;
static const COLORREF C_HEADER = 0xFF00AAFF;
static const COLORREF C_KNOWN  = 0xFF909090;
static const COLORREF C_GOLD   = 0xFFFFD700;

// The spell's learn level for THIS character's class.
//
// ⚠️ Falls back to the lowest level any class has rather than to 0. Our pool spells are opened
// across every class column (classes1..16 all equal), so any index is correct for them -- but a
// spell reached some other way may still be single-class, and reporting level 0 for it would sort it
// into the wrong band and look like a bug in the window rather than in the data.
static int SpellLevelFor(PSPELL s)
{
	PCHARINFO2 ci2 = GetCharInfo2();
	if (ci2) {
		const unsigned long cls = (unsigned long)ci2->Class;
		if (cls >= 1 && cls <= 16) {
			const BYTE lv = s->Level[cls - 1];
			if (lv > 0 && lv < 255) { return (int)lv; }
		}
	}
	int best = 0;
	for (int c = 0; c < 16; ++c) {
		const BYTE lv = s->Level[c];
		if (lv > 0 && lv < 255 && (best == 0 || lv < best)) { best = (int)lv; }
	}
	return best;
}

// A one-word bucket for the list's Type column, so the list reads without opening each spell.
static const char* SpellKind(PSPELL s)
{
	for (int i = 0; i < 12; ++i) {
		const DWORD spa = s->Attrib[i];
		if (spa == 254 || spa == 0xFFFFFFFF) { continue; }
		if (spa == 0)   { return s->Base[i] >= 0 ? "Heal" : "Damage"; }
		if (spa == 79)  { return "Damage"; }
		if (spa == 100) { return "Taunt"; }
		if (spa == 351) { return "Aura"; }
	}
	return (s->SpellType == 0) ? "Debuff" : "Buff";
}

// ===================================================================== panel state
//
// The widgets belong to the spell window (core_spellchoice_native.cpp); this module only fills
// them. Row maps live here because both lists carry rows that are not spells.
static int  g_knownId[SJ_MAX_KNOWN_ROWS] = {};
static int  g_knownRows = 0;
static int  g_poolIdRow[SJ_MAX_POOL] = {};
static int  g_poolRows  = 0;
static void (*g_poolReady)() = nullptr;
static void (*g_infoReady)() = nullptr;

// Level the player was at when each reward was TAKEN, from SJLEVELS.
//
// ⚠️ The Known tab used to show the spell's own LEARN level, read from the client's spell record.
// That is a different number and it is not what a player is asking when they look at this list --
// they want to know when they got it. The client cannot work that out: CHARINFO2 knows what is
// scribed, never when, so the SERVER records it at the moment of the pick and sends it here.
// A spell with no entry (granted some other way) simply shows no level.
static const int SJ_MAX_LVL = 256;
static int g_lvlId  [SJ_MAX_LVL] = {};
static int g_lvlAt  [SJ_MAX_LVL] = {};
static int g_lvlCount = 0;

static int SjPickLevel(int spell_id)
{
	for (int i = 0; i < g_lvlCount; ++i) { if (g_lvlId[i] == spell_id) { return g_lvlAt[i]; } }
	return 0;
}

// The last spell detail the server sent, cached so a re-click does not re-ask. Declared HERE with the
// rest of the state: SjShowDetail reads it well before SpellJournalParseInfo writes it, further down.
static int  g_infoId = 0;
static char g_infoText[2048] = "";

// Which pane is waiting on an SJINFO answer, and for which spell.
//
// ⚠️⚠️ WITHOUT THIS THE DESCRIPTION FLASHES. The info callback used to redraw BOTH detail panes; the
// pane that had NOT asked would find its own selection uncached, fire another request, and its answer
// would redraw the first pane, which asked again -- a permanent ping-pong of chat lines with the text
// blinking every lap. Remembering the asker means exactly one pane repaints, once.
static EQClasses::CListWnd* g_infoBox  = nullptr;
static int                  g_infoWant = 0;
static void (*g_levelsReady)() = nullptr;

void SjSetLevelsReadyCallback(void (*cb)()) { g_levelsReady = cb; }

void SjSetPoolReadyCallback(void (*cb)()) { g_poolReady = cb; }
void SjSetInfoReadyCallback(void (*cb)()) { g_infoReady = cb; }
int  SjLevel() { return g_poolLevel; }

int SjKnownSpellAtRow(int row)
{
	if (row < 0 || row >= g_knownRows) { return 0; }
	return g_knownId[row];
}

int SjPoolSpellAtRow(int row)
{
	if (row < 0 || row >= g_poolRows) { return 0; }
	return g_poolIdRow[row];
}

// ⚠️ SET THE COLOUR TOO. AddString colours column 0 ONLY; a cell written afterwards with SetItemText
// has no colour of its own and the client draws it BLACK, which on this UI is very nearly invisible.
// Every extra column has to be coloured explicitly.
static void Cell(EQClasses::CListWnd* l, int row, int col, const char* s, COLORREF colour = C_PLAIN)
{
	if (!l) { return; }
	CXStr v(s ? s : "");
	l->SetItemText(row, col, &v);
	l->SetItemColor(row, col, colour);
}

// ================================================================== spell ranks
// ⚠️⚠️ THE KNOWN TAB IS A PERMANENT LIBRARY AND IS SERVER-FED. It used to read CHARINFO2::SpellBook
// and therefore cost nothing -- but that only ever showed what is CARRIED RIGHT NOW. Ranks and
// discovery are permanent character progress living in server data buckets, so a spellbook view
// would hide every spell the player has earned a rank on but is not currently holding, which is most
// of them after a death.
//
// Wire (aotv4_spell_ranks_sys.send_state):
//   SPELLRANKCOST <rank>:<frag>:<ink>,...
//   SPELLRANKDATA <chunk> <chunks> <frag> <ink>^<base>:<rank>:<kept>:<origin>,...
//
// ⚠️ The cost table is SENT, not hardcoded here. A duplicate table in the dll is the kIcons drift
// trap (CLAUDE.md section 3) -- it would silently disagree with the server the first time either moved.
#define SJ_MAX_RANKED 512
struct SjRanked { int base, rank, kept, origin; };
static SjRanked g_rank[SJ_MAX_RANKED] = {};
static int      g_rankCount = 0;
static int      g_frag = 0, g_ink = 0;
static int      g_costFrag[6] = {}, g_costInk[6] = {};   // indexed by target rank 2..5

static const SjRanked* SjFindRanked(int base)
{
	for (int i = 0; i < g_rankCount; ++i) { if (g_rank[i].base == base) { return &g_rank[i]; } }
	return nullptr;
}

int  SjFragments()      { return g_frag; }
int  SjInk()            { return g_ink; }
int  SjRankOf(int base) { const SjRanked* r = SjFindRanked(base); return r ? r->rank : 0; }
bool SjIsKept(int base) { const SjRanked* r = SjFindRanked(base); return r && r->kept != 0; }

// "SPELLRANKCOST 2:30:1,3:60:2,..."
static bool SjParseRankCost(const char* message)
{
	const char* t = strstr(message, "SPELLRANKCOST ");
	if (!t) { return false; }
	t += strlen("SPELLRANKCOST ");
	memset(g_costFrag, 0, sizeof(g_costFrag));
	memset(g_costInk, 0, sizeof(g_costInk));
	while (*t) {
		int rk = 0, f = 0, ik = 0;
		if (sscanf_s(t, "%d:%d:%d", &rk, &f, &ik) == 3 && rk >= 2 && rk <= 5) {
			g_costFrag[rk] = f; g_costInk[rk] = ik;
		}
		const char* c = strchr(t, ',');
		if (!c) { break; }
		t = c + 1;
	}
	return true;
}

// "SPELLRANKDATA <chunk> <chunks> <frag> <ink>^base:rank:kept:origin,..."
static bool SjParseRankData(const char* message)
{
	const char* t = strstr(message, "SPELLRANKDATA ");
	if (!t) { return false; }
	t += strlen("SPELLRANKDATA ");

	int chunk = 0, chunks = 0, frag = 0, ink = 0;
	if (sscanf_s(t, "%d %d %d %d", &chunk, &chunks, &frag, &ink) != 4) { return true; }
	g_frag = frag; g_ink = ink;

	// ⚠️ Reset only on the FIRST chunk. Clearing on every chunk would leave the list holding just the
	// last 50 entries -- which reads as a short library rather than as a dropped message.
	if (chunk <= 1) { g_rankCount = 0; }

	const char* body = strchr(t, '^');
	if (!body) { return true; }
	++body;
	while (*body && g_rankCount < SJ_MAX_RANKED) {
		int b = 0, rk = 0, kp = 0, og = 0;
		if (sscanf_s(body, "%d:%d:%d:%d", &b, &rk, &kp, &og) == 4 && b > 0) {
			g_rank[g_rankCount].base   = b;
			g_rank[g_rankCount].rank   = rk;
			g_rank[g_rankCount].kept   = kp;
			g_rank[g_rankCount].origin = og;
			++g_rankCount;
		}
		const char* c = strchr(body, ',');
		if (!c) { break; }
		body = c + 1;
	}
	SjTrace("SPELLRANKDATA chunk %d/%d -> %d entries, frag %d ink %d", chunk, chunks, g_rankCount, g_frag, g_ink);
	return true;
}

// Server-side feedback for a window action (refusals, confirmations). Held until the next selection
// overwrites it, so it behaves like a status line rather than a toast that vanishes before it is read.
static char g_rankMsg[256] = "";

bool SjHasRankMessage() { return g_rankMsg[0] != 0; }

void SjRankMsgText(EQClasses::CStmlWnd* box)
{
	if (!box || !g_rankMsg[0]) { return; }
	char buf[320];
	sprintf_s(buf, "<c \"#FFE08C\">%s</c>", g_rankMsg);
	CXStr s(buf);
	box->SetSTMLText(s, true, nullptr);
}

bool SjParseRankTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	// ⚠️ Checked FIRST: "SPELLRANKMSG" and "SPELLRANKCOST" share a prefix up to "SPELLRANK", and
	// strstr on the shorter token would happily match inside the longer one if the order were wrong.
	if (const char* t = strstr(message, "SPELLRANKMSG ")) {
		strncpy_s(g_rankMsg, t + strlen("SPELLRANKMSG "), _TRUNCATE);
		return true;
	}
	if (SjParseRankCost(message)) { return true; }
	if (SjParseRankData(message)) { return true; }
	return false;
}

// Clear the status line. Called when the player selects a different row, so a stale "you cannot
// afford that" does not sit over the requirements for a spell they have since clicked.
void SjClearRankMessage() { g_rankMsg[0] = 0; }

// Requirement text for the box under the detail pane. Written server-side would cost a round trip
// per selection; the numbers are all already here.
void SjRankReqText(EQClasses::CStmlWnd* box, int base)
{
	if (!box) { return; }
	char buf[512];
	const SjRanked* r = SjFindRanked(base);
	if (base <= 0) {
		sprintf_s(buf, "<c \"#A0A0A0\">Select a spell.</c>");
	} else if (!r) {
		sprintf_s(buf, "<c \"#A0A0A0\">Not yet discovered.</c>");
	} else if (r->rank >= 5) {
		sprintf_s(buf, "<c \"#8CFF8C\">Rank V -- fully upgraded.</c>");
	} else {
		const int nxt = r->rank + 1;
		const int nf = g_costFrag[nxt], ni = g_costInk[nxt];
		// ⚠️ Colour each line by whether it is SATISFIED, so a refused Upgrade explains itself. A
		// button that simply does nothing reads as broken (the AdvLoot lesson, section 16).
		sprintf_s(buf,
			"Rank %d requires<br>"
			"<c \"%s\">  Parchment Fragment  %d  (you have %d)</c><br>"
			"<c \"%s\">  Ink of the Lost  %d  (you have %d)</c>",
			nxt,
			(g_frag >= nf) ? "#8CFF8C" : "#FF8C8C", nf, g_frag,
			(g_ink  >= ni) ? "#8CFF8C" : "#FF8C8C", ni, g_ink);
	}
	CXStr s(buf);
	box->SetSTMLText(s, true, nullptr);
}

// ------------------------------------------------------------------ Known
void SjRefreshKnown(EQClasses::CListWnd* list)
{
	if (!g_enabled || !list) { return; }
	list->DeleteAll();
	g_knownRows = 0;

	// ⚠️ Rendered from the SERVER's permanent library, not from CHARINFO2::SpellBook. Grouped by the
	// level the reward was TAKEN at (origin), which is the only ordering that means anything for a
	// list spanning many runs -- a spell's own learn level says nothing about when this character got it.

	// ---- Kept spells first, in their own section.
	// ⚠️ These are the two that decide how a run STARTS, so burying them among a hundred discovered
	// spells sorted by level makes the one thing the player is managing the hardest thing to find.
	// They are ALSO listed again in their level band below -- deliberately: this section answers
	// "what am I keeping", the bands answer "when did I get this", and a spell legitimately belongs
	// to both questions.
	{
		int keptShown = 0;
		for (int i = 0; i < g_rankCount && g_knownRows + 2 <= SJ_MAX_KNOWN_ROWS; ++i) {
			const SjRanked& e = g_rank[i];
			if (!e.kept) { continue; }
			PSPELL s = GetSpellByID((DWORD)e.base);
			if (!s) { continue; }

			if (keptShown == 0) {
				// ⚠️ Header text goes in COLUMN 0 now. The per-row level column was removed, so the
				// list is Spell + Rank -- a header written to column 1 would land in the narrow Rank
				// column and be clipped to nothing.
				const int hrow = list->AddString(CXStr("Kept -- these start each run"), C_HEADER, 0, nullptr, nullptr);
				(void)hrow;
				g_knownId[g_knownRows++] = 0;   // header: not selectable content
			}
			++keptShown;

			const int row = list->AddString(CXStr(s->Name), C_HEADER, (uint32_t)e.base, nullptr, nullptr);
			char mark[32];
			static const char* kR[6] = { "", "", "II", "III", "IV", "V" };
			if (e.rank >= 2 && e.rank <= 5) { sprintf_s(mark, "Rk. %s", kR[e.rank]); } else { mark[0] = 0; }
			Cell(list, row, 1, mark, C_HEADER);
			g_knownId[g_knownRows++] = e.base;
		}
	}

	int lastBand = -1;
	for (int band = 0; band <= 8; ++band) {
		for (int i = 0; i < g_rankCount && g_knownRows + 2 <= SJ_MAX_KNOWN_ROWS; ++i) {
			const SjRanked& e = g_rank[i];
			const int lvl = e.origin;
			if (lvl == 0) { if (band != 8) { continue; } }
			else if ((lvl - 1) / 10 != band) { continue; }

			PSPELL s = GetSpellByID((DWORD)e.base);
			if (!s) { continue; }

			if (lastBand != band) {
				lastBand = band;
				char hdr[48];
				if (band == 8) { strcpy_s(hdr, "Level not recorded"); }
				else            { sprintf_s(hdr, "Taken at character level %d to %d", band * 10 + 1, band * 10 + 10); }
				// ⚠️ Column 0 -- see the Kept header above. Two columns now, not three.
				list->AddString(CXStr(hdr), C_HEADER, 0, nullptr, nullptr);
				g_knownId[g_knownRows++] = 0;   // header: not selectable content
			}

			const int row = list->AddString(CXStr(s->Name), C_PLAIN, (uint32_t)e.base, nullptr, nullptr);

			// Rank and kept state share the second column: both are short, and the band header
			// already says when the spell was taken, so a level column would only repeat it.
			char mark[32];
			static const char* kRoman[6] = { "", "", "II", "III", "IV", "V" };
			if (e.rank >= 2 && e.rank <= 5) { sprintf_s(mark, "Rk. %s%s", kRoman[e.rank], e.kept ? " *" : ""); }
			else                            { sprintf_s(mark, "%s", e.kept ? "*" : ""); }
			Cell(list, row, 1, mark, e.kept ? C_HEADER : C_LABEL);
			g_knownId[g_knownRows++] = e.base;
		}
	}
	SjTrace("SjRefreshKnown: %d rows from %d ranked entries", g_knownRows, g_rankCount);
}

// ------------------------------------------------------------------ Pool
void SjRefreshPool(EQClasses::CListWnd* list, EQClasses::CXWnd* levelLabel)
{
	if (!g_enabled) { return; }

	if (list) {
		list->DeleteAll();
		g_poolRows = 0;

		for (int i = 0; i < g_poolCount && g_poolRows < SJ_MAX_POOL; ++i) {
			const bool isSkill = (g_poolId[i] >= SJ_SKILL_BASE);

			char name[72];
			if (isSkill) {
				strcpy_s(name, g_poolName[i]);
			}
			else {
				PSPELL s = GetSpellByID(g_poolId[i]);

				// A spell the client cannot resolve is still shown, by id. Dropping it would
				// under-report the pool, and the usual cause is simply that spells_us.txt was not
				// reinstalled after a custom-spell change -- worth seeing rather than hiding.
				if (s) { strcpy_s(name, s->Name); }
				else   { sprintf_s(name, "Spell #%d (client data missing)", g_poolId[i]); }
			}

			const COLORREF col = g_poolKnown[i] ? C_KNOWN : C_GOLD;
			const int row = list->AddString(CXStr(name), col, (uint32_t)g_poolId[i], nullptr, nullptr);
			Cell(list, row, 1,
				g_poolKnown[i] ? (isSkill ? "trained" : "known")
				               : (isSkill ? "ability" : "offerable"), col);
			g_poolIdRow[g_poolRows++] = g_poolId[i];
		}
	}

	if (levelLabel) {
		char lab[64];
		sprintf_s(lab, "Level %d  (%d offerable)", g_poolLevel, g_poolCount);
		CXStr v(lab);
		levelLabel->SetWindowTextA(v);
	}
	SjTrace("SjRefreshPool: level %d, %d rows", g_poolLevel, g_poolRows);
}

// ------------------------------------------------------------------ detail
// ⚠️ ASKS THE SERVER, then draws whatever comes back. The local SPA table that used to build this
// could only name the effects someone had added to it; everything else rendered as "SPA 137: 0".
// The pane shows a placeholder until SJINFO arrives, which is one chat line per row you click.
void SjShowDetail(EQClasses::CListWnd* box, int spellId)
{
	if (!g_enabled || !box) { return; }
	box->DeleteAll();
	if (spellId == 0) { return; }

	if (g_infoId == spellId && g_infoText[0]) {
		// split the server text on "~"
		const char* p = g_infoText;
		std::string cur;
		bool first = true;
		for (;; ++p) {
			if (*p == '~' || *p == 0) {
				box->AddString(CXStr(cur.c_str()), first ? C_GOLD : C_PLAIN, 0, nullptr, nullptr);
				first = false;
				cur.clear();
				if (*p == 0) { break; }
			}
			else { cur += *p; }
		}
		return;
	}

	// remember who asked, so the answer repaints only this pane
	g_infoBox  = box;
	g_infoWant = spellId;
	{
		char cmd[64];
		sprintf_s(cmd, "/say sjinfo %d", spellId);
		AoTQueueGameCommand(cmd);
	}
	box->AddString(CXStr("Loading..."), C_LABEL, 0, nullptr, nullptr);
	return;
}

// ------------------------------------------------------------------ pool paging
void SjRequestLevel(int level)
{
	if (!g_enabled) { return; }

	PCHARINFO2 ci2 = GetCharInfo2();
	const int cap = ci2 ? (int)ci2->Level : 1;
	if (level < 1)   { level = 1; }
	if (level > cap) { level = cap; }   // no previewing above your level; the picker will not offer it

	g_poolLevel      = level;
	g_poolCount      = 0;
	g_poolChunksSeen = 0;
	g_poolChunksWant = 0;

	char cmd[64];
	sprintf_s(cmd, "/say sjpool %d", level);
	AoTQueueGameCommand(cmd);
}

void SjStepLevel(int delta)
{
	SjRequestLevel(g_poolLevel + delta);
}

// ===================================================================== transport
//
// "SJPOOLDATA <level> <chunk> <chunks>^id:known,id:known,..."
bool SpellJournalParseTransport(const char* message)
{
	if (!g_enabled || !message) { return false; }
	const char* t = strstr(message, "SJPOOLDATA ");
	if (!t) { return false; }
	t += strlen("SJPOOLDATA ");

	int level = 0, chunk = 0, chunks = 0;
	if (sscanf_s(t, "%d %d %d", &level, &chunk, &chunks) != 3) { return true; }

	const char* body = strchr(t, '^');
	if (!body) { return true; }
	++body;

	// A late chunk from a level the player has already paged away from must not be merged into the
	// current one. Chat can deliver out of order, and without this the list would mix two levels.
	if (level != g_poolLevel) {
		SjTrace("transport: dropping stale chunk for level %d (showing %d)", level, g_poolLevel);
		return true;
	}

	if (chunk == 1) { g_poolCount = 0; g_poolChunksSeen = 0; }
	g_poolChunksWant = chunks;

	while (*body && g_poolCount < SJ_MAX_POOL) {
		const char* comma = strchr(body, ',');

		if (*body == 'k') {
			// "k<skillid>:<known>:<name>" -- a combat ability. The name is on the wire because this
			// client has no skill-name table; it runs to the comma (or the end of the line).
			int skid = 0, known = 0;
			if (sscanf_s(body + 1, "%d:%d:", &skid, &known) == 2 && skid >= 0) {
				const char* nm = strchr(body + 1, ':');
				nm = nm ? strchr(nm + 1, ':') : nullptr;
				if (nm) {
					++nm;
					size_t len = comma ? (size_t)(comma - nm) : strlen(nm);
					if (len >= sizeof(g_poolName[0])) { len = sizeof(g_poolName[0]) - 1; }
					memcpy(g_poolName[g_poolCount], nm, len);
					g_poolName[g_poolCount][len] = 0;

					g_poolId   [g_poolCount] = SJ_SKILL_BASE + skid;
					g_poolKnown[g_poolCount] = (known != 0);
					++g_poolCount;
				}
			}
		}
		else {
			int id = 0, known = 0;
			if (sscanf_s(body, "%d:%d", &id, &known) == 2 && id > 0) {
				g_poolName [g_poolCount][0] = 0;
				g_poolId   [g_poolCount] = id;
				g_poolKnown[g_poolCount] = (known != 0);
				++g_poolCount;
			}
		}

		if (!comma) { break; }
		body = comma + 1;
	}

	++g_poolChunksSeen;
	SjTrace("transport: level %d chunk %d/%d, %d spells so far", level, chunk, chunks, g_poolCount);

	// Only repaint once the whole answer is in, so the list does not visibly grow in stages.
	if (g_poolChunksSeen >= g_poolChunksWant && g_poolReady) { g_poolReady(); }
	return true;
}

// "SJINFO <spellid> <text>" -- the server's readable spell detail, lines separated by "~".
//
// ⚠️ THIS REPLACES THE LOCAL SPA RENDERING for the Known and Pool panes. DescribeSPA could only
// label the SPAs someone had put in its table; everything else printed as "SPA 137: 0", which tells
// a player nothing. The server has the db_str text with its placeholders filled, so it writes the
// description and the client just displays it.
// Repaint ONLY the pane that asked, and only for the spell it asked about.
void SjRedrawPendingDetail()
{
	if (!g_enabled || !g_infoBox) { return; }
	if (g_infoWant == 0 || g_infoWant != g_infoId) { return; }   // a stale or unrelated answer
	EQClasses::CListWnd* box = g_infoBox;
	g_infoBox  = nullptr;                                        // consumed: no re-entry
	g_infoWant = 0;
	SjShowDetail(box, g_infoId);                                 // now finds it cached, draws, returns
}

// "SJLEVELS <chunk> <chunks>^id:lvl,id:lvl,..."
bool SpellJournalParseLevels(const char* message)
{
	if (!g_enabled || !message) { return false; }
	const char* t = strstr(message, "SJLEVELS ");
	if (!t) { return false; }
	t += strlen("SJLEVELS ");

	int chunk = 0, chunks = 0;
	if (sscanf_s(t, "%d %d", &chunk, &chunks) != 2) { return true; }
	const char* body = strchr(t, '^');
	if (!body) { return true; }
	++body;

	if (chunk == 1) { g_lvlCount = 0; }
	while (*body && g_lvlCount < SJ_MAX_LVL) {
		int id = 0, lv = 0;
		if (sscanf_s(body, "%d:%d", &id, &lv) == 2 && id > 0) {
			g_lvlId[g_lvlCount] = id;
			g_lvlAt[g_lvlCount] = lv;
			++g_lvlCount;
		}
		const char* comma = strchr(body, ',');
		if (!comma) { break; }
		body = comma + 1;
	}

	SjTrace("levels: chunk %d/%d, %d recorded", chunk, chunks, g_lvlCount);
	// ⚠️ The LEVELS callback, not the info one. The levels live in the Known LIST, so the list has to be
	// rebuilt -- redrawing the detail pane instead left the column blank until something else refreshed
	// it, which is why only spells from earlier levels appeared to be tracked.
	if (chunk >= chunks && g_levelsReady) { g_levelsReady(); }
	return true;
}

bool SpellJournalParseInfo(const char* message)
{
	if (!g_enabled || !message) { return false; }
	const char* t = strstr(message, "SJINFO ");
	if (!t) { return false; }
	t += strlen("SJINFO ");

	int id = 0;
	if (sscanf_s(t, "%d", &id) != 1) { return true; }
	const char* sp = strchr(t, ' ');
	g_infoId = id;
	strncpy_s(g_infoText, sizeof(g_infoText), sp ? sp + 1 : "", _TRUNCATE);

	if (g_infoReady) { g_infoReady(); }
	return true;
}

bool SpellJournalIsOurEcho(const char* message)
{
	if (!g_enabled || !message) { return false; }
	return strstr(message, "sjpool ") != nullptr
	    || strstr(message, "sjinfo ") != nullptr
	    || strstr(message, "sjlevels") != nullptr;
}

void InitSpellJournal()
{
	g_enabled = true;
	SjTrace("InitSpellJournal: enabled");
}
