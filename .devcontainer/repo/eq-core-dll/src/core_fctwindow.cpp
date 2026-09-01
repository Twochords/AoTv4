// core_fctwindow -- the Combat Text window. See core_fctwindow.h for what it is and why it owns
// no state of its own.

#include <Windows.h>
#include "MQ2Main.h"
#include "core_fctwindow.h"
#include "core_floatingtext.h"
#include "nms/FloatingTextManager.h"   // EFtColour
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern void AoTQueueGameCommand(const char* cmd);
// core_spellwindow.cpp -- the only safe way to print from this dll (CLAUDE.md section 49).
extern void AoTv4ChatPrint(const char* line);

// ⚠️ These read as what the player SEES, not as the code's category names. "Damage you deal" beats
// "outgoing", and the list doubles as the legend for the coloured boxes.
// Trace to <EQ>\aotv4_fctwindow.log, flushed and CLOSED on every line.
// ⚠️⚠️ THE POINT IS THE CRASH, SO IT MUST SURVIVE ONE. A buffered handle loses the last few lines --
// which are the only ones that matter -- so this reopens per line. That is slow and it is only ever
// used around the handful of client calls below, not per frame.
static void FctTrace(const char* fmt, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_fctwindow.log", gszEQPath); }
	else              { strcpy_s(path, "aotv4_fctwindow.log"); }

	FILE* f = nullptr;
	if (fopen_s(&f, path, "a") || !f) { return; }

	SYSTEMTIME now; GetLocalTime(&now);
	fprintf(f, "[%02d:%02d:%02d] ", now.wHour, now.wMinute, now.wSecond);

	va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
	fprintf(f, "\n");
	fclose(f);
}

static const char* kColourName[FT_COLOUR_COUNT] = {
	"Weapon swings",
	"Your spells",
	"Combat abilities",
	"Damage on you",
	"Healing",
	"Critical hits"
};

// A short ring rather than a colour picker. There is no colour-picker widget in SIDL and no reliable
// way to read an RGB value back off a control on this build, so a fixed palette that cycles is the
// honest option; every entry is separable from the other three at a glance.
static const unsigned int kPalette[] = {
	0xFFF2E0, 0xFF5040, 0x60FF80, 0xFFD24A, 0x66C0FF, 0xC080FF, 0xFF9040, 0xFFFFFF, 0x9AA0A8
};
static const int kPaletteCount = (int)(sizeof(kPalette) / sizeof(kPalette[0]));

static const char* kCascadeName[3] = {
	"Straight up",
	"Fanned out",
	"Straight, slight spread"
};


static void FctLabel(CXWnd* w, const char* s)
{
	if (w) { CXStr v(s ? s : ""); w->SetWindowTextA(v); }
}

// The two draggable markers. One class, two instances -- they differ only in which anchor they write.
// ⚠️⚠️ POSITION IS READ BACK FROM THE WINDOW, NOT PUSHED INTO IT EVERY FRAME. The client owns the drag
// and persists window positions per character itself; writing our value in on a timer would fight the
// player's mouse and snap the box back mid-drag.
class FctAnchorWnd : public CCustomWnd
{
public:
	bool  m_heal;
	float m_lastX, m_lastY;
	// False until the first position read after the marker is shown. Without it that read looks like a
	// drag and overwrites the saved anchor with wherever the client happened to restore the window.
	bool  m_seeded;

	// ⚠️ `char*`, NOT `const char*`. CCustomWnd's only usable overload is CCustomWnd(char*)
	// (MQ2Internal.h:387) -- there is no const one. A string literal reaches it because this project
	// builds with ConformanceMode=false, which still permits that deprecated conversion; a const char*
	// VARIABLE does not convert and fails with "no overloaded function could convert all the argument
	// types", naming the constructor rather than the call site that passed the wrong type.
	FctAnchorWnd(char* screen, bool heal)
		: CCustomWnd(screen), m_heal(heal), m_lastX(-1.0f), m_lastY(-1.0f), m_seeded(false) {}

	// ⚠️⚠️ THERE IS DELIBERATELY NO CODE HERE THAT MOVES THE WINDOW, AND THAT IS THE SECOND ATTEMPT.
	// The first version called CXWnd::Move(CXRect) to open each marker on its saved anchor, and the
	// client crashed on the button. Move(CXPoint) at 0x8660E0 and Move(CXRect) at 0x8686F0 are told
	// apart only by which offset eqgame.h assigns to which overload, NOTHING in this dll has ever
	// called either to prove that mapping, and passing a 16 byte struct to a function expecting an 8
	// byte one corrupts the stack. An unverified address is not worth a crash for a convenience.
	// 📌 The client persists window position per character on its own, so a marker stays where the
	// player last dragged it without us positioning anything. What is lost is only that the FIRST time
	// they are opened they sit where the XML puts them rather than on the current anchor.
	// ⚠️ Which is why the tick below must not treat that first read as a drag -- see m_seeded.

	// Centre of the window as a fraction of the screen. False if it cannot be determined.
	bool Fraction(float* fx, float* fy)
	{
		CXWnd* w = pXWnd();
		if (!w) { return false; }

		// ⚠️⚠️ CXRect A,B,C,D ARE left/top/right/bottom AND THEY ARE UNSIGNED (DWORD). Cast before any
		// arithmetic -- a window dragged to the very edge wraps to about four billion and the anchor
		// lands off screen. Same trap the AoT launcher records for its docking maths.
		// ⚠️⚠️ GetScreenRect RETURNS CXRect BY VALUE THROUGH A NAKED JMP STUB, and nothing else in this
		// dll calls it. Now that CXWnd::Move is gone, this is the remaining unverified client call in
		// the "Move the spots" path -- so if the crash persists, aotv4_fctwindow.log will end on
		// "before GetScreenRect" with no "after". That pair of lines is the whole point.
		// ⚠️ Traced for the first few calls only: this runs per frame while the markers are up, and the
		// trace reopens the log file on every line.
		static int traced = 0;
		const bool trace = (traced < 4);
		if (trace) { ++traced; FctTrace("Fraction: before GetScreenRect (wnd=%p)", (void*)w); }
		CXRect r = w->GetScreenRect();
		if (trace) { FctTrace("Fraction: after GetScreenRect A=%u B=%u C=%u D=%u",
		                      (unsigned)r.A, (unsigned)r.B, (unsigned)r.C, (unsigned)r.D); }
		const int cx = ((int)r.A + (int)r.C) / 2;
		const int cy = ((int)r.B + (int)r.D) / 2;

		int sw = 0, sh = 0;
		if (!FloatingTextScreenSize(&sw, &sh)) { return false; }

		*fx = (float)cx / (float)sw;
		*fy = (float)cy / (float)sh;
		if (*fx < 0.02f) { *fx = 0.02f; }  if (*fx > 0.98f) { *fx = 0.98f; }
		if (*fy < 0.02f) { *fy = 0.02f; }  if (*fy > 0.98f) { *fy = 0.98f; }
		return true;
	}
};

// Set by the button, acted on by the next tick. See the note at the click handler.
static int g_pendingAnchors = 0;   // 0 nothing, 1 show, 2 hide

static FctAnchorWnd* gDmgAnchor  = nullptr;
static FctAnchorWnd* gHealAnchor = nullptr;

class FctWnd : public CCustomWnd
{
public:
	CButtonWnd* Enabled  = nullptr;
	CListWnd*   FontList = nullptr;
	CListWnd*   CascList = nullptr;
	CXWnd*      SizeLbl  = nullptr;
	CButtonWnd* SizeDn   = nullptr;
	CButtonWnd* SizeUp   = nullptr;
	CXWnd*      FadeLbl  = nullptr;
	CButtonWnd* FadeDn   = nullptr;
	CButtonWnd* FadeUp   = nullptr;
	CXWnd*      RiseLbl  = nullptr;
	CButtonWnd* RiseDn   = nullptr;
	CButtonWnd* RiseUp   = nullptr;
	CXWnd*      FanLbl   = nullptr;
	CButtonWnd* FanDn    = nullptr;
	CButtonWnd* FanUp    = nullptr;
	CButtonWnd* Anchored = nullptr;
	CButtonWnd* EditSpots  = nullptr;
	CButtonWnd* ColourNext = nullptr;
	CListWnd*   ColourList = nullptr;
	CXWnd*      AnchorLbl = nullptr;
	CButtonWnd* Mine     = nullptr;
	CButtonWnd* Taken    = nullptr;
	CButtonWnd* Others   = nullptr;
	CButtonWnd* Heals    = nullptr;
	CButtonWnd* TestB    = nullptr;
	CButtonWnd* DefaultB = nullptr;
	CXWnd*      Status   = nullptr;

	bool m_syncing = false;

	FctWnd() : CCustomWnd("AoTFctWnd")
	{
		SetWndNotification(FctWnd);
		Enabled   = (CButtonWnd*)GetChildItem("FCT_Enabled");
		FontList  = (CListWnd*)  GetChildItem("FCT_FontList");
		CascList  = (CListWnd*)  GetChildItem("FCT_CascadeList");
		SizeLbl   =              GetChildItem("FCT_SizeLbl");
		SizeDn    = (CButtonWnd*)GetChildItem("FCT_SizeDn");
		SizeUp    = (CButtonWnd*)GetChildItem("FCT_SizeUp");
		FadeLbl   =              GetChildItem("FCT_FadeLbl");
		FadeDn    = (CButtonWnd*)GetChildItem("FCT_FadeDn");
		FadeUp    = (CButtonWnd*)GetChildItem("FCT_FadeUp");
		RiseLbl   =              GetChildItem("FCT_RiseLbl");
		RiseDn    = (CButtonWnd*)GetChildItem("FCT_RiseDn");
		RiseUp    = (CButtonWnd*)GetChildItem("FCT_RiseUp");
		FanLbl    =              GetChildItem("FCT_FanLbl");
		FanDn     = (CButtonWnd*)GetChildItem("FCT_FanDn");
		FanUp     = (CButtonWnd*)GetChildItem("FCT_FanUp");
		Anchored  = (CButtonWnd*)GetChildItem("FCT_Anchored");
		EditSpots  = (CButtonWnd*)GetChildItem("FCT_EditSpots");
		ColourNext = (CButtonWnd*)GetChildItem("FCT_ColourNext");
		ColourList = (CListWnd*)  GetChildItem("FCT_ColourList");
		AnchorLbl =              GetChildItem("FCT_AnchorLbl");
		Mine      = (CButtonWnd*)GetChildItem("FCT_Mine");
		Taken     = (CButtonWnd*)GetChildItem("FCT_Taken");
		Others    = (CButtonWnd*)GetChildItem("FCT_Others");
		Heals     = (CButtonWnd*)GetChildItem("FCT_Heals");
		TestB     = (CButtonWnd*)GetChildItem("FCT_Test");
		DefaultB  = (CButtonWnd*)GetChildItem("FCT_Defaults");
		Status    =              GetChildItem("FCT_Status");
		FillLists();
		Sync();
	}

	// ⚠️ The lists are built ONCE, in the constructor. Rebuilding them in Sync would call DeleteAll on
	// every refresh and yank the player's selection out from under them, which is the trap the Autoskill
	// window records for its own list.
	void FillLists()
	{
		if (FontList) {
			FontList->DeleteAll();
			for (int i = 0; i < FloatingTextFontCount(); ++i) {
				// ⚠️ 0xFF.. -- the leading byte is ALPHA. 0x00RRGGBB is fully transparent and the row
				// draws as nothing at all (CLAUDE.md section 21).
				FontList->AddString(FloatingTextFontName(i), 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
			}
		}
		if (CascList) {
			CascList->DeleteAll();
			for (int i = 0; i < 3; ++i) {
				CascList->AddString(kCascadeName[i], 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
			}
		}
		if (ColourList) {
			ColourList->DeleteAll();
			for (int i = 0; i < FT_COLOUR_COUNT; ++i) {
				ColourList->AddString(kColourName[i], 0xFFE0DCCD, (uint32_t)i, nullptr, nullptr);
			}
			ColourList->SetCurSel(0);
		}
	}

	// ⚠️ The caption carries the state as well as the checkbox glyph. Reported from play on the Travel
	// window: the checked and unchecked art differ by little more than a tint on this UI, which is
	// legible on a stock window you already know and not on a new one.
	void Check(CButtonWnd* b, bool on, const char* text)
	{
		if (!b) { return; }
		b->SetCheck(on);
		char s[96];
		sprintf_s(s, "%s %s", on ? "[X]" : "[  ]", text);
		FctLabel((CXWnd*)b, s);
	}

	void Sync()
	{
		// ⚠️⚠️ RE-ENTRANCY GUARD. Sync calls SetCurSel and SetCheck, and a widget that raises a
		// notification when its value is set would come straight back into WndNotification, which calls
		// Commit, which calls Sync. That is an unbounded recursion and it presents as a hung client, not
		// as a crash -- exactly what "Move the spots froze my game" looked like. Cheap enough to keep
		// whether or not any widget here actually does that.
		if (m_syncing) { return; }
		m_syncing = true;
		struct Guard { bool* f; ~Guard() { *f = false; } } guard{ &m_syncing };

		Check(Enabled,  areFloatingTextEnabled, "Show combat text");
		Check(Anchored, g_ftMode == 1,          "Use two fixed spots on screen");
		Check(Mine,     g_ftShowOutgoing,       "Damage you deal");
		Check(Taken,    g_ftShowIncoming,       "Damage done to you");
		Check(Others,   g_ftShowOthers,         "Everyone else");
		Check(Heals,    g_ftShowHeals,          "Healing");

		char s[128];
		sprintf_s(s, "Font size: %d", g_ftFontSize);            FctLabel(SizeLbl, s);
		sprintf_s(s, "Fade after: %d ms", g_ftDurationMs);      FctLabel(FadeLbl, s);
		sprintf_s(s, "Travel: %d px", g_ftRisePixels);          FctLabel(RiseLbl, s);
		sprintf_s(s, "Spread: %d px", g_ftFanWidth);            FctLabel(FanLbl,  s);

		// Percentages, because that is what the player placed and what survives a resolution change.
		sprintf_s(s, "Damage at %d, %d across       Healing at %d, %d across",
		          (int)(g_ftAnchorDmgX * 100.0f), (int)(g_ftAnchorDmgY * 100.0f),
		          (int)(g_ftAnchorHealX * 100.0f), (int)(g_ftAnchorHealY * 100.0f));
		FctLabel(AnchorLbl, s);

		// Selecting the current row is how the two lists show state; a latched row cannot be set
		// reliably on this build, so the selection is the only marker available.
		if (FontList) {
			for (int i = 0; i < FloatingTextFontCount(); ++i) {
				if (_stricmp(FloatingTextFontName(i), g_ftFontFace) == 0) { FontList->SetCurSel(i); break; }
			}
		}
		if (CascList && g_ftCascade >= 0 && g_ftCascade < 3) { CascList->SetCurSel(g_ftCascade); }

		// ⚠️ Reflects the PENDING state as well as the current one. The work happens on the next tick, so
		// reading only the current flag would leave the caption a frame behind and the button would feel
		// like it had not registered the press.
		const bool editing = g_pendingAnchors ? (g_pendingAnchors == 1) : FloatingTextAnchorEditing();
		FctLabel((CXWnd*)EditSpots, editing ? "Done moving" : "Move the spots");
		SetStatus(editing ? "Drag them, then Done" : "");

		// ⚠️ The rows are RECOLOURED to the colour they name. That is the only preview available --
		// there is no swatch widget here -- and it makes the list its own legend.
		if (ColourList) {
			for (int i = 0; i < FT_COLOUR_COUNT; ++i) {
				ColourList->SetItemColor(i, 0, (COLORREF)(0xFF000000u | g_ftColour[i]));
			}
		}
	}

	void SetStatus(const char* s) { FctLabel(Status, s); }

	void Commit()
	{
		FloatingTextSaveSettings();
		Sync();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unk)
	{
		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)Enabled) {
				areFloatingTextEnabled = !areFloatingTextEnabled;
				// ⚠️ The server has to be told. Healing is fed by a transport line it only sends to
				// clients that asked, so switching the feature on without this leaves the healing
				// anchor permanently empty with nothing to explain why.
				FloatingTextAnnounceHeals();
				Commit(); return 1;
			}
			if (pWnd == (CXWnd*)Anchored) { g_ftMode = (g_ftMode == 1) ? 0 : 1; Commit(); return 1; }
			if (pWnd == (CXWnd*)Mine)     { g_ftShowOutgoing = !g_ftShowOutgoing; Commit(); return 1; }
			if (pWnd == (CXWnd*)Taken)    { g_ftShowIncoming = !g_ftShowIncoming; Commit(); return 1; }
			if (pWnd == (CXWnd*)Others)   { g_ftShowOthers   = !g_ftShowOthers;   Commit(); return 1; }
			if (pWnd == (CXWnd*)Heals)    {
				g_ftShowHeals = !g_ftShowHeals;
				FloatingTextAnnounceHeals();
				Commit(); return 1;
			}

			// ⚠️ Steps are proportional to what the value MEANS, not uniform. A font stepping 1 point at
			// a time needs sixteen clicks to go from small to large; a fade stepping 1 ms would need
			// hundreds. Both would read as a broken button.
			if (pWnd == (CXWnd*)SizeDn) { FloatingTextSetFont(g_ftFontFace, g_ftFontSize - 2); Commit(); return 1; }
			if (pWnd == (CXWnd*)SizeUp) { FloatingTextSetFont(g_ftFontFace, g_ftFontSize + 2); Commit(); return 1; }

			if (pWnd == (CXWnd*)FadeDn) { g_ftDurationMs -= 250; if (g_ftDurationMs < 200)   { g_ftDurationMs = 200; }   Commit(); return 1; }
			if (pWnd == (CXWnd*)FadeUp) { g_ftDurationMs += 250; if (g_ftDurationMs > 10000) { g_ftDurationMs = 10000; } Commit(); return 1; }
			if (pWnd == (CXWnd*)RiseDn) { g_ftRisePixels -= 25;  if (g_ftRisePixels < 10)    { g_ftRisePixels = 10; }    Commit(); return 1; }
			if (pWnd == (CXWnd*)RiseUp) { g_ftRisePixels += 25;  if (g_ftRisePixels > 1000)  { g_ftRisePixels = 1000; }  Commit(); return 1; }
			if (pWnd == (CXWnd*)FanDn)  { g_ftFanWidth   -= 10;  if (g_ftFanWidth < 0)       { g_ftFanWidth = 0; }       Commit(); return 1; }
			if (pWnd == (CXWnd*)FanUp)  { g_ftFanWidth   += 10;  if (g_ftFanWidth > 400)     { g_ftFanWidth = 400; }     Commit(); return 1; }

			if (pWnd == (CXWnd*)EditSpots) {
				// ⚠️⚠️ DEFERRED TO THE TICK, NOT DONE HERE, AND THIS FROZE THE CLIENT WHEN IT WAS DONE
				// HERE. Showing the markers CREATES two SIDL windows and then MOVES them -- three
				// mutations of the window manager's own list, from inside a notification the window
				// manager is still dispatching. CLAUDE.md section 3 records the same shape for the Death
				// Book: rebuilding a listbox from inside its own click destroys rows the client is still
				// walking. Creating windows and repositioning them mid-dispatch is a larger version of
				// it, and neither GetScreenRect nor CXWnd::Move is called anywhere else in this dll, so
				// there was no precedent saying it was safe.
				// 📌 One frame of latency, and the button caption updates immediately, so it does not
				// read as a delay.
				g_pendingAnchors = FloatingTextAnchorEditing() ? 2 : 1;   // 1 show, 2 hide
				return 1;
			}

			if (pWnd == (CXWnd*)ColourNext) {
				const int which = ColourList ? ColourList->GetCurSel() : 0;
				if (which >= 0 && which < FT_COLOUR_COUNT) {
					int at = 0;
					for (int i = 0; i < kPaletteCount; ++i) {
						if (kPalette[i] == g_ftColour[which]) { at = i; break; }
					}
					g_ftColour[which] = kPalette[(at + 1) % kPaletteCount];
					Commit();
					// Sample numbers in the new colour, so the choice is judged on screen rather than
					// from a swatch in a list.
					FloatingTextTestBurst();
				}
				return 1;
			}

			if (pWnd == (CXWnd*)TestB) { FloatingTextTestBurst(); return 1; }

			if (pWnd == (CXWnd*)DefaultB) {
				g_ftMode = 0; g_ftCascade = 1; g_ftFanWidth = 60;
				g_ftRisePixels = 150; g_ftDurationMs = 1000; g_ftScalePct = 1.0f;
				g_ftAnchorDmgX = 0.62f; g_ftAnchorDmgY = 0.55f;
				g_ftAnchorHealX = 0.38f; g_ftAnchorHealY = 0.55f;
				g_ftShowOutgoing = true; g_ftShowIncoming = true;
				g_ftShowOthers = false;  g_ftShowHeals = true;
				g_ftColour[FT_COLOUR_MELEE]   = 0xFFF2E0; g_ftColour[FT_COLOUR_SPELL] = 0x8AB4FF;
				g_ftColour[FT_COLOUR_ABILITY] = 0xFF9A3C; g_ftColour[FT_COLOUR_TAKEN] = 0xFF5040;
				g_ftColour[FT_COLOUR_HEAL]    = 0x60FF80; g_ftColour[FT_COLOUR_CRIT]  = 0xFFD24A;
				g_ftLaneGapPx = 22;
				FloatingTextSetFont("Arial", 24);
				FloatingTextAnnounceHeals();
				Commit(); return 1;
			}

			if (pWnd == (CXWnd*)FontList && FontList) {
				// ⚠️ Let the base class run FIRST, then read. Inside a listbox's own notification
				// GetCurSel is one click behind, so reading it before would apply the PREVIOUS row --
				// i.e. every font change would be one selection stale.
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				const int sel = FontList->GetCurSel();
				if (sel >= 0 && sel < FloatingTextFontCount()) {
					FloatingTextSetFont(FloatingTextFontName(sel), g_ftFontSize);
					Commit();
				}
				return handled;
			}
			if (pWnd == (CXWnd*)ColourList && ColourList) {
				return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
			}
			if (pWnd == (CXWnd*)CascList && CascList) {
				const int handled = CSidlScreenWnd::WndNotification(pWnd, Message, unk);
				const int sel = CascList->GetCurSel();
				if (sel >= 0 && sel < 3) { g_ftCascade = sel; Commit(); }
				return handled;
			}
		}
		return CSidlScreenWnd::WndNotification(pWnd, Message, unk);
	}
};

static FctWnd* gFctWnd = nullptr;

// Show or hide the two markers, creating them the first time.
// ⚠️ Turning them ON also turns anchored mode on: nobody positions a spot they do not intend to use,
// and leaving the mode off makes the whole exercise appear to do nothing.
void FctShowAnchors(bool on)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) { return; }

	FctTrace("FctShowAnchors(%d): entering, dmg=%p heal=%p", on ? 1 : 0, (void*)gDmgAnchor, (void*)gHealAnchor);
	if (!gDmgAnchor)  { FctTrace("  creating AoTFctDmgAnchor");  gDmgAnchor  = new FctAnchorWnd((PCHAR)"AoTFctDmgAnchor",  false); }
	if (!gHealAnchor) { FctTrace("  creating AoTFctHealAnchor"); gHealAnchor = new FctAnchorWnd((PCHAR)"AoTFctHealAnchor", true); }
	FctTrace("  created, pXWnd dmg=%p heal=%p",
	         gDmgAnchor ? (void*)gDmgAnchor->pXWnd() : nullptr,
	         gHealAnchor ? (void*)gHealAnchor->pXWnd() : nullptr);

	// ⚠️⚠️ A NULL pXWnd MEANS THE XML IS NOT LOADED, AND SAYING SO IS THE WHOLE POINT OF THIS BLOCK.
	// CCustomWnd looks its screen up by name and returns SILENTLY when the name is unknown -- no window,
	// no error, no log line -- so a missing <Include> in EQUI.xml presents as a button that does
	// nothing. That is the single most common cause of "nothing happened" for every native window in
	// this dll, and it cost a round trip here because the failure said nothing at all.
	const bool haveXml = gDmgAnchor && gDmgAnchor->pXWnd() && gHealAnchor && gHealAnchor->pXWnd();
	if (!haveXml) {
		AoTv4ChatPrint("Combat Text: the anchor windows are not loaded.");
		AoTv4ChatPrint("Copy EQUI_AoTFctAnchorWnd.xml to uifiles\\default and add "
		               "<Include>EQUI_AoTFctAnchorWnd.xml</Include> to EQUI.xml, then /loadskin.");
		// ⚠️ Do NOT switch anchored mode on. Without the markers there is no way to position a spot, so
		// leaving the mode on would send every number to whatever the anchors defaulted to with nothing
		// on screen explaining why they stopped appearing over the target.
		FloatingTextSetAnchorEdit(false);
		return;
	}

	FctTrace("  showing");
	gDmgAnchor->pXWnd()->Show(on ? 1 : 0, 1);
	gHealAnchor->pXWnd()->Show(on ? 1 : 0, 1);
	FctTrace("  shown");

	// ⚠️ The markers are NOT repositioned here. See the note in FctAnchorWnd for why, and note that the
	// first position read after showing them must not be mistaken for a drag.
	if (on) {
		gDmgAnchor->m_seeded  = false;
		gHealAnchor->m_seeded = false;
	}
	FctTrace("FctShowAnchors(%d): done", on ? 1 : 0);

	FloatingTextSetAnchorEdit(on);
	if (on) { g_ftMode = 1; }
}

// Every native window in this dll has to wire the MQ2 UI managers itself -- CCustomWnd silently bails
// at "if (!pSidlMgr || !pWndMgr)" and never appears otherwise (CLAUDE.md section 15).
static void FctEnsureWindow(bool show)
{
	if (!ppSidlMgr) ppSidlMgr = (CSidlManager **)(((DWORD)0x15D3D08 - 0x400000) + baseAddress);
	if (!ppWndMgr)  ppWndMgr  = (CXWndManager **)(((DWORD)0x15D3D00 - 0x400000) + baseAddress);
	if (!pSidlMgr || !pWndMgr) { return; }

	// A NULL pXWnd here means CCustomWnd could not find the screen "AoTFctWnd", i.e.
	// EQUI_AoTFctWnd.xml is not loaded: the <Include> is missing from EQUI.xml or the file was never
	// copied. Silent otherwise, and the most common cause of "nothing happened".
	if (!gFctWnd) { gFctWnd = new FctWnd(); }
	if (gFctWnd && !gFctWnd->pXWnd()) {
		// Same silent failure as the anchors above, for the same reason.
		AoTv4ChatPrint("Combat Text: EQUI_AoTFctWnd.xml is not loaded.");
		AoTv4ChatPrint("Copy it to uifiles\\default and add "
		               "<Include>EQUI_AoTFctWnd.xml</Include> to EQUI.xml, then /loadskin.");
		return;
	}
	if (gFctWnd && show) {
		gFctWnd->Sync();
		if (gFctWnd->pXWnd()) { gFctWnd->pXWnd()->Show(1, 1); }
	}
}

void FctWindowShow()
{
	FctEnsureWindow(true);
	// ⚠️ Re-announce on every open. The opt-in lives on the server's Client object, which is rebuilt on
	// every zone; and if the login-time announce is ever missed, opening this window is the one moment
	// we know the player is looking for numbers, so it is the natural place to re-ask.
	FloatingTextAnnounceHeals();
}

// ⚠️⚠️ THE HEAL OPT-IN HAS TO BE RE-SENT ON EVERY ZONE CHANGE, NOT ONCE PER LOGIN. The server keeps
// it on the Client object (Client::m_aotv4_fct_heals) and a zone hands the character to a DIFFERENT
// zone process, which constructs a fresh Client -- so the flag is gone the moment you zone and the
// healing anchor silently goes empty until the next relog. Watching pLocalPlayer go null and come back
// catches login and every zone alike, and needs no new hook.
static bool     g_wasInGame = false;
static unsigned g_lastAnnounce = 0;

// How often to re-tell the server we want the feed.
// ⚠️⚠️ A HEARTBEAT, NOT JUST AN EVENT, AND THE EVENT ALONE WAS THE BUG. The opt-in lives on the
// server's Client object and that is rebuilt on every zone, so it has to be re-sent constantly. The
// only trigger was pLocalPlayer going null and back -- and ProcessGameEvents does not run through a
// loading screen, so the client can be handed to a new zone process without this ever observing the
// transition. The flag was then lost with nothing left to notice.
// 📌 Reported from play as having to click "Show combat text" every login: that checkbox is the only
// other thing that announces, so toggling it was the player manually doing this.
// ⚠️ One intercepted say a minute. Client::ChannelMessageReceived swallows it and returns before any
// broadcast or quest event, so it costs a string compare and reaches nobody.
static const unsigned FCT_ANNOUNCE_MS = 60000;

void FctWindowTick()
{
	const bool inGame = (pLocalPlayer != nullptr);
	const unsigned now = GetTickCount();

	// Announce on entering the world, and keep announcing. Either alone is not enough: the event misses
	// zones that never null the pointer, and a heartbeat alone would leave the first minute dead.
	// ⚠️⚠️ ONLY WHILE THE FEATURE IS ON. The server's flag defaults to FALSE on every new Client, so a
	// player who never asks already receives nothing -- repeating "fctheal 0" at them every minute would
	// be a say command to re-state a default. Switching the feature OFF still announces once, from the
	// checkbox handler, and that is what stops the CURRENT session's feed.
	// 📌 So a player who does not want combat text, or who does not run this dll at all, sends nothing
	// and receives nothing.
	if (inGame && areFloatingTextEnabled &&
	    (!g_wasInGame || (now - g_lastAnnounce) >= FCT_ANNOUNCE_MS)) {
		FloatingTextAnnounceHeals();
		g_lastAnnounce = now;
	}
	g_wasInGame = inGame;

	if (g_pendingAnchors) {
		const bool show = (g_pendingAnchors == 1);
		g_pendingAnchors = 0;
		FctShowAnchors(show);
		FloatingTextSaveSettings();
		if (gFctWnd) { gFctWnd->Sync(); }
	}

	if (!FloatingTextAnchorEditing()) { return; }

	// Follow the markers. Polling their position is the only option: this build exposes no "window was
	// moved" notification, and CXWnd::Move is something WE would call, not something the client tells
	// us about.
	// ⚠️⚠️ ONLY WRITE THE INI WHEN THE VALUE ACTUALLY CHANGES. This runs every frame; saving
	// unconditionally is a file write per frame for as long as the markers are on screen.
	bool dirty = false;
	FctAnchorWnd* both[2] = { gDmgAnchor, gHealAnchor };
	for (int i = 0; i < 2; ++i) {
		FctAnchorWnd* a = both[i];
		if (!a || !a->pXWnd()) { continue; }

		float fx = 0.0f, fy = 0.0f;
		if (!a->Fraction(&fx, &fy)) { continue; }
		if (fx == a->m_lastX && fy == a->m_lastY) { continue; }

		a->m_lastX = fx;
		a->m_lastY = fy;

		// ⚠️⚠️ THE FIRST READ AFTER SHOWING IS A BASELINE, NOT A DRAG. The markers are not repositioned
		// when they open, so wherever the client restored them is not where the anchor is -- and taking
		// that first read as a move would silently overwrite the player's saved spot the instant they
		// pressed the button. Only movement AFTER the baseline counts.
		if (!a->m_seeded) { a->m_seeded = true; continue; }

		if (a->m_heal) { g_ftAnchorHealX = fx; g_ftAnchorHealY = fy; }
		else           { g_ftAnchorDmgX  = fx; g_ftAnchorDmgY  = fy; }
		dirty = true;
	}

	// ⚠️⚠️ THE MARKERS HAVE NO CLOSE BOX, AND THAT IS DELIBERATE. This build exposes no way to READ a
	// window's visibility -- EQClasses.h has neither IsVisible nor a usable dShow, and CXWnd::Show is
	// write-only to us -- so if a player could dismiss a marker with its own X, this flag would go stale
	// and the Done button would need pressing twice with nothing on screen to explain why. One
	// dismissal path, and it is the one the button caption names.

	if (dirty) {
		FloatingTextSaveSettings();
		if (gFctWnd) { gFctWnd->Sync(); }
	}
}

void FctWindowOnUiReset()
{
	// ⚠️⚠️ DELETE, do not just null. Nulling leaks the CCustomWnd and leaves it registered with the
	// client's window manager after its widgets are freed, so the next thing to walk the window list
	// touches freed memory.
	if (gFctWnd)     { delete gFctWnd;     gFctWnd = nullptr; }
	if (gDmgAnchor)  { delete gDmgAnchor;  gDmgAnchor = nullptr; }
	if (gHealAnchor) { delete gHealAnchor; gHealAnchor = nullptr; }
	FloatingTextSetAnchorEdit(false);
}
