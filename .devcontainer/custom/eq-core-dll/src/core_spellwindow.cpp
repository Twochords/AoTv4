// core_spellwindow.cpp
// ---------------------------------------------------------------------------
// Level-up spell-choice window for the Bard-only random server.
//
// We tried drawing the panel as a DirectX9 overlay by hooking the device's /
// swap chain's Present, but the RoF2 client renders through EQGraphicsDX9.dll's
// own device, which shares NO vtable with any device we can create -- so the
// dummy-device hook never fires (proven: dh=2 sh=2 present=0 swap=0).
//
// Instead we draw the panel as a separate LAYERED, TOPMOST window (GDI) that sits
// over the EQ client in windowed mode. It is completely independent of EQ's
// renderer, so it always shows. Input is handled on the GAME thread via a hook of
// ProcessGameEvents (per-frame), which is where we safely run the pick command.
//
// Flow:
//   server -> client chat line:  SPELLCHOICEDATA name1^name2^name3
//   -> chat detour swallows it, stores names, sets g_visible
//   -> overlay thread shows/positions the layered window and paints the 3 choices
//   -> click on a "Select" button stores g_pendingPick
//   -> ProcessGameEvents hook (game thread) runs  /say spellpick N  and clears it
//
// Gated by areSpellChoiceWindowEnabled (_options.h) via EnableSpellChoiceWindow().
// ---------------------------------------------------------------------------

#include "MQ2Main.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ===================================================================== skill unlock
// Reward-gating of combat skills (see core_skillunlock.h). CSkillMgr::GetSkillCap returns 0
// for a class+skill the class can't natively have; the Skills window and ability-hotkey
// picker hide anything with a 0 cap. We reveal a COMBAT skill only if the server says the
// player has EARNED it (picked it -> value > 0), so un-earned combat skills stay hidden
// until chosen. Non-combat real skills (casting/singing/utility) are unlocked unconditionally
// so spells and songs work. Unused skill slots are left at 0 (no "None" rows).

// Reward-gated skill ids -- MUST match quests/lua_modules/skill_pool.lua. Only the activated
// class-specific special attacks are gated; weapon skills and passive combat skills are
// auto-learned (not gated) so they always show.
static const int SKILLUNLOCK_COMBAT_IDS[] = {
	8,10,30,16,73,72,74,        // Backstab/Bash/Kick/Disarm/Taunt/Berserking/Frenzy
	21,23,26,38,52              // monk strikes: Dragon Punch/Eagle Strike/Flying/Round Kick/Tiger Claw
};
static bool g_combatGated[NUM_SKILLS] = { false };  // is skill id reward-gated?
static bool g_skillEarned[NUM_SKILLS] = { false };  // has the player earned it (server-sent)?

static void SkillUnlock_InitSets()
{
	for (int i = 0; i < (int)(sizeof(SKILLUNLOCK_COMBAT_IDS) / sizeof(int)); ++i) {
		int s = SKILLUNLOCK_COMBAT_IDS[i];
		if (s >= 0 && s < NUM_SKILLS) g_combatGated[s] = true;
	}
}

// Parse the server's "SKILLUNLOCKDATA id,id,..." line into g_skillEarned. Returns true if it
// was our line (so the chat hook swallows it). An empty list = nothing earned yet.
static bool SkillUnlock_HandleChat(const char* msg)
{
	static const char* TAG = "SKILLUNLOCKDATA";
	if (!msg) return false;
	const char* p = strstr(msg, TAG);
	if (!p) return false;
	p += strlen(TAG);

	for (int s = 0; s < NUM_SKILLS; ++s) if (g_combatGated[s]) g_skillEarned[s] = false;
	int cur = -1;
	for (;; ++p) {
		if (*p >= '0' && *p <= '9') { cur = (cur < 0 ? 0 : cur) * 10 + (*p - '0'); }
		else {
			if (cur >= 0 && cur < NUM_SKILLS) g_skillEarned[cur] = true;
			cur = -1;
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		}
	}
	return true;
}

DETOUR_TRAMPOLINE_EMPTY(unsigned long __fastcall GetSkillCap_Tramp(void* pThis, void* edx,
	void* pChar, int level, int classId, int skill, bool d, bool e, bool f));

static const int SKILLUNLOCK_MAX_REAL = 76;  // highest real skill id in RoF2 (77+ = unused "None")

unsigned long __fastcall GetSkillCap_Detour(void* pThis, void* edx, void* pChar,
	int level, int classId, int skill, bool d, bool e, bool f)
{
	unsigned long cap = GetSkillCap_Tramp(pThis, edx, pChar, level, classId, skill, d, e, f);
	// Only ever REVEAL a skill the client hides (native cap 0). A native skill (cap > 0)
	// passes straight through, so weapon skills / Defense / Dodge etc. are never removed.
	// Bounded to ids 0..76 so the unused "None" slots (77..98) stay hidden.
	if (cap == 0 && skill >= 0 && skill <= SKILLUNLOCK_MAX_REAL) {
		unsigned long scaled = (unsigned long)((level > 0 ? level : 1) * 5);
		if (scaled > 250) scaled = 250;
		if (g_combatGated[skill]) {
			if (g_skillEarned[skill]) cap = scaled;  // non-native combat: reveal ONLY if earned
		} else {
			cap = scaled;                            // non-native casting/utility: reveal so it works
		}
	}
	return cap;
}

void EnableSkillUnlock()
{
	SkillUnlock_InitSets();
	EzDetour((((DWORD)0x5BAEC0 - 0x400000) + baseAddress), GetSkillCap_Detour, GetSkillCap_Tramp);
}

// ----------------------------------------------------------------- state
static volatile bool   g_visible = false;
static char            g_names[3][160] = { {0}, {0}, {0} };
static int             g_icons[3] = { 0, 0, 0 };  // spellbook icon index per choice
static int             g_count = 0;

// art assets loaded from the client's uifiles/default at runtime
static char            g_uiDir[MAX_PATH] = {0};
static bool            g_assetsInit = false;
static struct { int sheet; HBITMAP bmp; } g_sheetCache[16];
static int             g_sheetCacheN = 0;

// window placement + drag state (overlay thread only)
static int             g_posX = 0, g_posY = 0;
static bool            g_positioned = false;
static bool            g_dragging = false;
static int             g_dragDX = 0, g_dragDY = 0;

static HBITMAP LoadTGA(const char* path);  // fwd decl (used by the chat diagnostic)
static volatile int    g_pendingPick = -1;     // 1..3 set by click, consumed on game thread
static volatile bool   g_overlayStarted = false;
static HWND            g_overlayHwnd = nullptr;

// window box (overlay-client pixels)
static const int WIN_W  = 440;
static const int TOP_H  = 34;
static const int ROW_H  = 52;
static const int WIN_H  = TOP_H + 3 * ROW_H + 12;

static void ButtonRect(int i, RECT& r)
{
	r.right  = WIN_W - 14;
	r.left   = r.right - 96;
	r.top    = TOP_H + i * ROW_H + 10;
	r.bottom = r.top + 32;
}

static const int ICON_SZ = 40;

static void IconRect(int i, RECT& r)
{
	r.left   = 16;
	r.top    = TOP_H + i * ROW_H + (ROW_H - ICON_SZ) / 2;
	r.right  = r.left + ICON_SZ;
	r.bottom = r.top + ICON_SZ;
}

static void NameRect(int i, RECT& r)
{
	r.left   = 16 + ICON_SZ + 12;   // after the icon
	r.right  = WIN_W - 110;
	r.top    = TOP_H + i * ROW_H + 6;
	r.bottom = r.top + ROW_H - 12;
}

// ----------------------------------------------------------------- chat trigger
static const char* SPLCHOICE_TAG = "SPELLCHOICEDATA ";

static bool HandleChat(const char* msg)
{
	if (!msg) return false;
	const char* tag = strstr(msg, SPLCHOICE_TAG);
	if (!tag) return false;
	tag += strlen(SPLCHOICE_TAG);

	g_count = 0;
	std::string cur;
	for (const char* p = tag; ; ++p) {
		if (*p == '^' || *p == 0 || *p == '\n' || *p == '\r') {
			if (!cur.empty() && g_count < 3) {
				// each token is "name|icon" (icon optional)
				size_t bar = cur.find('|');
				std::string nm = (bar == std::string::npos) ? cur : cur.substr(0, bar);
				int ic = (bar == std::string::npos) ? 0 : atoi(cur.c_str() + bar + 1);
				strncpy_s(g_names[g_count], sizeof(g_names[g_count]), nm.c_str(), _TRUNCATE);
				g_icons[g_count] = ic;
				g_count++;
			}
			cur.clear();
			if (*p == 0 || *p == '\n' || *p == '\r') break;
		} else {
			cur += *p;
		}
	}
	g_visible = (g_count > 0);
	return true; // swallow our trigger line
}

// CEverQuest::dsp_chat (RoF2 0x51F1A0) detour -- thiscall, detoured as __fastcall.
void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct);
DETOUR_TRAMPOLINE_EMPTY(void __fastcall SpellChat_Tramp(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct));
void __fastcall SpellChat_Detour(void* thisptr, void* edx, const char* msg, int color, bool eqlog, bool pct)
{
	// Hide the pick command echo. Selecting a choice issues "/say spellpick N", which
	// the client would otherwise show as  You say, 'spellpick N'  (and broadcast to
	// nearby players). Since every player runs this dll, swallowing any line containing
	// "spellpick" hides it on the speaker's screen AND on every listener's screen.
	if (msg && strstr(msg, "spellpick")) return;

	if (SkillUnlock_HandleChat(msg)) return;  // swallow SKILLUNLOCKDATA (updates earned set)
	if (HandleChat(msg)) return;  // swallow SPELLCHOICEDATA (window shows the choices)
	SpellChat_Tramp(thisptr, edx, msg, color, eqlog, pct);
}

// ----------------------------------------------------------------- art assets (TGA)
// EQ's spell icons and spellbook background live in uifiles/default as TGA files. We
// can't use EQ's renderer, so we decode the TGAs ourselves into 32-bit DIBs and blit
// them with GDI. Supports uncompressed (type 2) and RLE (type 10), 24/32-bit.
static HBITMAP LoadTGA(const char* path)
{
	FILE* f = nullptr;
	if (fopen_s(&f, path, "rb") != 0 || !f) return nullptr;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz < 18) { fclose(f); return nullptr; }
	std::vector<unsigned char> buf((size_t)sz);
	size_t rd = fread(buf.data(), 1, (size_t)sz, f);
	fclose(f);
	if (rd != (size_t)sz) return nullptr;

	unsigned char idLen   = buf[0];
	unsigned char imgType = buf[2];
	int w = buf[12] | (buf[13] << 8);
	int h = buf[14] | (buf[15] << 8);
	unsigned char depth = buf[16];
	unsigned char desc  = buf[17];
	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return nullptr;
	if (depth != 24 && depth != 32) return nullptr;
	if (imgType != 2 && imgType != 10) return nullptr;

	bool topDown = (desc & 0x20) != 0;
	int  bpp = depth / 8;
	const unsigned char* p   = buf.data() + 18 + idLen;
	const unsigned char* end = buf.data() + sz;

	std::vector<unsigned char> pix((size_t)w * h * 4, 0);
	auto put = [&](int i, const unsigned char* s) {
		int x = i % w, yr = i / w;
		int y = topDown ? yr : (h - 1 - yr);
		size_t d = ((size_t)y * w + x) * 4;
		pix[d + 0] = s[0]; pix[d + 1] = s[1]; pix[d + 2] = s[2];      // BGR
		pix[d + 3] = (bpp == 4) ? s[3] : 255;
	};

	int total = w * h;
	if (imgType == 2) {
		for (int i = 0; i < total && p + bpp <= end; ++i, p += bpp) put(i, p);
	} else { // RLE
		int i = 0;
		while (i < total && p < end) {
			unsigned char hdr = *p++;
			int count = (hdr & 0x7F) + 1;
			if (hdr & 0x80) {                       // run packet
				if (p + bpp > end) break;
				for (int c = 0; c < count && i < total; ++c, ++i) put(i, p);
				p += bpp;
			} else {                                // raw packet
				for (int c = 0; c < count && i < total && p + bpp <= end; ++c, ++i, p += bpp) put(i, p);
			}
		}
	}

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;   // top-down
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HBITMAP bmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (bmp && bits) memcpy(bits, pix.data(), pix.size());
	return bmp;
}

static void InitAssets()
{
	if (g_assetsInit) return;
	g_assetsInit = true;

	char exe[MAX_PATH] = {0};
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	char* slash = strrchr(exe, '\\');
	if (slash) slash[1] = 0; else exe[0] = 0;
	sprintf_s(g_uiDir, "%suifiles\\default\\", exe);
}

// Spellbook icon sheets: spells01.tga.. each 6x6 grid of 40x40 icons (icon index 1-based).
static HBITMAP GetSheet(int sheetNum)
{
	for (int i = 0; i < g_sheetCacheN; ++i)
		if (g_sheetCache[i].sheet == sheetNum) return g_sheetCache[i].bmp;
	char path[MAX_PATH];
	sprintf_s(path, "%sspells%02d.tga", g_uiDir, sheetNum);
	HBITMAP bmp = LoadTGA(path);
	if (g_sheetCacheN < 16) { g_sheetCache[g_sheetCacheN].sheet = sheetNum;
	                          g_sheetCache[g_sheetCacheN].bmp = bmp; ++g_sheetCacheN; }
	return bmp;  // cached even if null, so we don't retry a missing file every frame
}

static void BlitIcon(HDC dst, int iconIdx, const RECT& r)
{
	if (iconIdx <= 0) return;
	int sheet = iconIdx / 36 + 1;        // spells01 holds icons 0-35 (raw index, 0=none)
	int cell  = iconIdx % 36;
	HBITMAP sh = GetSheet(sheet);
	if (!sh) return;
	// derive columns from the actual sheet width (40px cells) instead of assuming 6
	BITMAP bm; GetObject(sh, sizeof(bm), &bm);
	int cols = bm.bmWidth / ICON_SZ; if (cols < 1) cols = 1;
	int sx = (cell % cols) * ICON_SZ;
	int sy = (cell / cols) * ICON_SZ;
	HDC sdc = CreateCompatibleDC(dst);
	HBITMAP oldS = (HBITMAP)SelectObject(sdc, sh);
	BitBlt(dst, r.left, r.top, ICON_SZ, ICON_SZ, sdc, sx, sy, SRCCOPY);
	SelectObject(sdc, oldS);
	DeleteDC(sdc);
}

// Draw text with a 1px offset highlight/shadow for an embossed, readable look.
static void DrawShadow(HDC dc, const char* s, RECT r, UINT fmt, COLORREF col, COLORREF shadow)
{
	RECT sr = r; OffsetRect(&sr, 1, 1);
	SetTextColor(dc, shadow);
	DrawTextA(dc, s, -1, &sr, fmt);
	SetTextColor(dc, col);
	DrawTextA(dc, s, -1, &r, fmt);
}

// ----------------------------------------------------------------- layered window
static void PaintOverlay(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	// double-buffer to avoid flicker
	HDC     mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
	HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

	RECT full = { 0, 0, WIN_W, WIN_H };
	// Parchment background: warm cream base with a soft top-to-bottom shade, drawn with
	// GDI (no book art). Scanline gradient is cheap for a panel this small.
	for (int y = 0; y < WIN_H; ++y) {
		float t = (float)y / (float)WIN_H;
		int r = (int)(222 - 26 * t);   // 222 -> 196
		int g = (int)(205 - 28 * t);   // 205 -> 177
		int b = (int)(168 - 30 * t);   // 168 -> 138
		HBRUSH line = CreateSolidBrush(RGB(r, g, b));
		RECT lr = { 0, y, WIN_W, y + 1 };
		FillRect(mem, &lr, line);
		DeleteObject(line);
	}

	// double border (dark brown outer, gold inner) for a framed-page look
	HBRUSH brdOuter = CreateSolidBrush(RGB(70, 50, 25));
	FrameRect(mem, &full, brdOuter);
	DeleteObject(brdOuter);
	RECT inner = { 2, 2, WIN_W - 2, WIN_H - 2 };
	HBRUSH brdInner = CreateSolidBrush(RGB(150, 120, 70));
	FrameRect(mem, &inner, brdInner);
	DeleteObject(brdInner);

	HFONT font = CreateFontA(-17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	HFONT oldFont = (HFONT)SelectObject(mem, font);
	SetBkMode(mem, TRANSPARENT);

	// title (dark ink with a light highlight = embossed look on parchment)
	RECT tr = { 14, 6, WIN_W - 8, TOP_H };
	DrawShadow(mem, "Choose a New Spell", tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER,
		RGB(74, 48, 16), RGB(245, 235, 210));
	// divider under the title
	HBRUSH div = CreateSolidBrush(RGB(150, 120, 70));
	RECT dr = { 12, TOP_H - 2, WIN_W - 12, TOP_H - 1 };
	FillRect(mem, &dr, div);
	DeleteObject(div);

	for (int i = 0; i < g_count; ++i) {
		RECT ir; IconRect(i, ir);
		BlitIcon(mem, g_icons[i], ir);
		HBRUSH ifr = CreateSolidBrush(RGB(70, 50, 25));
		FrameRect(mem, &ir, ifr);
		DeleteObject(ifr);

		RECT nr; NameRect(i, nr);
		DrawShadow(mem, g_names[i], nr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
			RGB(48, 32, 12), RGB(238, 228, 205));

		RECT br; ButtonRect(i, br);
		HBRUSH btn = CreateSolidBrush(RGB(120, 92, 52));
		FillRect(mem, &br, btn);
		DeleteObject(btn);
		HBRUSH btnEdge = CreateSolidBrush(RGB(70, 50, 25));
		FrameRect(mem, &br, btnEdge);
		DeleteObject(btnEdge);
		DrawShadow(mem, "Select", br, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
			RGB(255, 248, 228), RGB(40, 28, 12));
	}

	BitBlt(hdc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY);

	SelectObject(mem, oldFont);
	DeleteObject(font);
	SelectObject(mem, old);
	DeleteObject(bmp);
	DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_PAINT:
		PaintOverlay(hwnd);
		return 0;

	case WM_LBUTTONDOWN: {
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		// dragging the title strip (above the rows) moves the window
		if (pt.y < TOP_H) {
			g_dragging = true;
			g_dragDX = pt.x;   // cursor offset within the window
			g_dragDY = pt.y;
			SetCapture(hwnd);
		}
		return 0;
	}

	case WM_MOUSEMOVE: {
		if (g_dragging) {
			POINT cur; GetCursorPos(&cur);
			g_posX = cur.x - g_dragDX;
			g_posY = cur.y - g_dragDY;
			g_positioned = true;
			SetWindowPos(hwnd, HWND_TOPMOST, g_posX, g_posY, WIN_W, WIN_H, SWP_NOACTIVATE);
		}
		return 0;
	}

	case WM_LBUTTONUP: {
		if (g_dragging) { g_dragging = false; ReleaseCapture(); return 0; }
		POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
		for (int i = 0; i < g_count; ++i) {
			RECT r; ButtonRect(i, r);
			if (PtInRect(&r, pt)) {
				g_pendingPick = i + 1;   // consumed by the ProcessGameEvents hook
				g_visible = false;
				ShowWindow(hwnd, SW_HIDE);
				break;
			}
		}
		return 0;
	}

	case WM_TIMER: {
		// EQADDR_HWND is the ADDRESS of the game's HWND variable, so dereference it.
		HWND eq = EQADDR_HWND ? *(HWND*)EQADDR_HWND : nullptr;
		// Only show while the user is actually in EQ -- or interacting with our own window --
		// so the overlay stays at EQ's level and never floats over other apps (Visual Studio,
		// browser, etc.). Allowing our own window as "foreground" means clicking/dragging the
		// overlay doesn't make it vanish (that was an earlier complaint).
		HWND fg = GetForegroundWindow();
		bool active = (fg == eq) || (fg == hwnd);
		if (g_visible && active) {
			if (!g_positioned && eq) {
				// first show: center in the upper third of the EQ client
				RECT cr; GetClientRect(eq, &cr);
				POINT tl = { cr.left, cr.top };
				ClientToScreen(eq, &tl);
				int cw = cr.right - cr.left, ch = cr.bottom - cr.top;
				g_posX = tl.x + (cw - WIN_W) / 2;
				g_posY = tl.y + (ch - WIN_H) / 3;
				g_positioned = true;
			}
			if (g_positioned) {
				SetWindowPos(hwnd, HWND_TOPMOST, g_posX, g_posY, WIN_W, WIN_H, SWP_NOACTIVATE);
				if (!IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_SHOWNOACTIVATE);
				InvalidateRect(hwnd, nullptr, FALSE);
			}
		} else if (IsWindowVisible(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
		return 0;
	}

	case WM_DESTROY:
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI OverlayThreadProc(LPVOID)
{
	InitAssets();  // load spellbook background + prepare icon-sheet cache

	HINSTANCE hInst = GetModuleHandleA(nullptr);

	WNDCLASSEXA wc = { sizeof(wc) };
	wc.lpfnWndProc   = OverlayWndProc;
	wc.hInstance     = hInst;
	wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
	wc.lpszClassName = "AoTSpellChoiceOverlay";
	RegisterClassExA(&wc);

	// NOTE: deliberately NOT WS_EX_NOACTIVATE -- clicking/dragging the window must take
	// foreground from EQ so EQ stops reading the mouse (otherwise dragging also rotates
	// the camera). It still appears without stealing focus (shown via SW_SHOWNOACTIVATE).
	g_overlayHwnd = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		"AoTSpellChoiceOverlay", "", WS_POPUP,
		0, 0, WIN_W, WIN_H, nullptr, nullptr, hInst, nullptr);
	if (!g_overlayHwnd) return 0;

	SetLayeredWindowAttributes(g_overlayHwnd, 0, 240, LWA_ALPHA);
	SetTimer(g_overlayHwnd, 1, 60, nullptr);  // reposition / show-hide / repaint ~16fps

	MSG m;
	while (GetMessageA(&m, nullptr, 0, 0) > 0) {
		TranslateMessage(&m);
		DispatchMessageA(&m);
	}
	return 0;
}

// ----------------------------------------------------------------- game-thread hook
// ProcessGameEvents (RoF2 0x539E60) runs every frame on the main thread. We use it to
// (a) lazily spawn the overlay thread once the game is up, and (b) run the chosen pick
// command on the game thread (calling InterpretCmd from the overlay thread is unsafe).
BOOL __cdecl ProcessGameEvents_Detour();
DETOUR_TRAMPOLINE_EMPTY(BOOL __cdecl ProcessGameEvents_Tramp());
BOOL __cdecl ProcessGameEvents_Detour()
{
	if (!g_overlayStarted) {
		g_overlayStarted = true;
		CreateThread(nullptr, 0, OverlayThreadProc, nullptr, 0, nullptr);
	}

	int pick = g_pendingPick;
	if (pick > 0) {
		g_pendingPick = -1;
		char cmd[40];
		sprintf_s(cmd, "/say spellpick %d", pick);
		if (pEverQuest && pLocalPlayer) {
			typedef void (__thiscall *fInterpretCmd)(void* pThis, void* pChar, const char* cmd);
			auto fn = (fInterpretCmd)((((DWORD)0x51FCE0 - 0x400000) + baseAddress));
			fn((void*)pEverQuest, (void*)pLocalPlayer, cmd);
		}
	}

	return ProcessGameEvents_Tramp();
}

// ----------------------------------------------------------------- entry
void EnableSpellChoiceWindow()
{
	// Both detours are pure in-memory byte patches -- safe under DllMain's loader lock.
	// The overlay window/thread is created LATER (first ProcessGameEvents tick), well
	// after load, because window creation under loader lock hard-fails startup.

	// swallow the server's SPELLCHOICEDATA chat line (CEverQuest::dsp_chat)
	EzDetour((((DWORD)0x51F1A0 - 0x400000) + baseAddress), &SpellChat_Detour, &SpellChat_Tramp);

	// per-frame main-thread heartbeat: spawn overlay + dispatch picks (ProcessGameEvents)
	EzDetour((((DWORD)0x539E60 - 0x400000) + baseAddress), &ProcessGameEvents_Detour, &ProcessGameEvents_Tramp);
}
