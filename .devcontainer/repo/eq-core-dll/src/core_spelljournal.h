#pragma once

// core_spelljournal -- the Known and Pool panels of the spell window (AoTv4).
// =========================================================================================
// ⚠️ THIS MODULE NO LONGER OWNS A WINDOW. It began as a standalone "Spell Journal"
// (EQUI_AoTSpellJournalWnd.xml, still on disk as a backup but unused). The two panels were folded
// into the EXISTING reward picker instead, which now carries three tabs:
//
//   Choose : the reward cards -- icons, two-step Select/Confirm. UNCHANGED.
//   Known  : every spell this character has scribed, under level band headers.
//   Pool   : what can still be offered at a chosen level, with a level stepper.
//
// core_spellchoice_native.cpp owns the window and every child widget; this module is the data and
// rendering service behind the two new tabs. It is kept separate because it is the only part of the
// dll that reads the CLIENT'S OWN spell records, and that is worth keeping in one place.
//
// Adapted from the earlier AoT generation's AoT_Spell_Book (New folder (2)), which is where the
// native-tabs layout was first proven on this client. What was NOT taken: its progression model
// (one spell slot per level, tiers of five, a Roll button, mastery bit-packed into synthetic spell
// ids) and its opcode transport (0x196A/0x196B) -- this dll has no raw-packet send and every window
// it owns rides the shared dsp_chat detour.
//
// It installs NO detours. The existing ones call in:
//
//   InitSpellJournal()            -- core_init.h (InitOptions), gated by areSpellJournalEnabled.
//   SpellJournalParseTransport()  -- from the dsp_chat swallow chain. True if it ate an SJPOOLDATA line.
//   SpellJournalIsOurEcho()       -- true for the "/say sjpool" echo, so dsp_chat swallows it too.
//
// Protocol (chat; the dll swallows all of it):
//   server -> dll : "SJPOOLDATA <level> <chunk> <chunks>^id:known,id:known,..."
//   dll -> server : "/say sjpool <level>"
//
// ⚠️ IDS ONLY. Names are resolved locally through GetSpellByID; sending them would roughly
// quadruple the payload (level 70 holds 125 pool spells). Chunked because an oversized chat line is
// silently TRUNCATED, which looks like a short pool rather than an error.

// ⚠️ FORWARD DECLARE INSIDE EQClasses, NOT AT GLOBAL SCOPE. The client classes live in
// `namespace EQClasses`, and EQClasses.h ends with a `using namespace EQClasses;`. A bare
// `class CXWnd;` here therefore declares a SECOND, unrelated ::CXWnd, and every later mention
// becomes "ambiguous symbol" plus a pile of "cannot convert from EQClasses::CXWnd* to CXWnd*".
// That one mistake produced well over a hundred errors across both files.
// The signatures below are FULLY QUALIFIED for the same reason -- that works whether this header is
// included before or after EQClasses.h, and adds no global `using` of its own.
namespace EQClasses { class CListWnd; class CXWnd; class CStmlWnd; }

void InitSpellJournal();
bool SpellJournalParseTransport(const char* message);
bool SpellJournalIsOurEcho(const char* message);

// ---- panel services, called by the window that owns the widgets -------------------------------
void SjRefreshKnown(EQClasses::CListWnd* list);

// ---- spell ranks (permanent library + upgrade costs), fed by SPELLRANKCOST / SPELLRANKDATA.
// ⚠️ Call SjParseRankTransport from the dsp_chat swallow chain BEFORE anything that would print the
// line, exactly like the other transports here -- these are protocol, not chat.
bool SjParseRankTransport(const char* message);
void SjRankReqText(EQClasses::CStmlWnd* box, int base);   // fills the requirement panel
void SjRankMsgText(EQClasses::CStmlWnd* box);             // overwrites it with a server status line
bool SjHasRankMessage();
void SjClearRankMessage();
int  SjFragments();
int  SjInk();
int  SjRankOf(int base);    // 0 = not discovered, 1 = discovered unranked, 2-5 = ranked
bool SjIsKept(int base);
void SjRefreshPool(EQClasses::CListWnd* list, EQClasses::CXWnd* levelLabel);
void SjShowDetail(EQClasses::CListWnd* box, int spellId);

// Row -> spell id. Both lists carry rows that are NOT spells (Known interleaves band headers), so
// the row index is never a data index; these return 0 for such a row.
int  SjKnownSpellAtRow(int row);
int  SjPoolSpellAtRow(int row);

// Ask the server for a level's pool. Clamped to 1..your level.
void SjRequestLevel(int level);
void SjStepLevel(int delta);
int  SjLevel();

// Called once a chunked answer is complete, so the owner can repaint the Pool list.
void SjSetPoolReadyCallback(void (*cb)());
void SjSetInfoReadyCallback(void (*cb)());   // fired when SJINFO lands, so the pane can redraw
bool SpellJournalParseInfo(const char* message);
bool SpellJournalParseLevels(const char* message);
void SjSetLevelsReadyCallback(void (*cb)());   // fired when SJLEVELS completes: rebuild the Known list
void SjRedrawPendingDetail();                  // repaint only the pane that asked for SJINFO
