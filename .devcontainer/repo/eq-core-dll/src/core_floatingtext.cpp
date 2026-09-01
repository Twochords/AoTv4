// core_floatingtext.cpp
// ---------------------------------------------------------------------------------------------
// Floating combat text. See core_floatingtext.h for the module contract, and in particular for why
// the D3D device CAN be hooked here after CLAUDE.md section 10 recorded that it could not.
// ---------------------------------------------------------------------------------------------

#include "MQ2Main.h"
#include "core_floatingtext.h"

#include <d3d9.h>
#include <d3dx9math.h>   // D3DXVECTOR3 / D3DXVec3Project, from deps/dx9/Include
#include <cstdio>
#include <cstdarg>
#include <cstdlib>   // atoi, for the /fct argument parsing
#include <cstring>

#include "nms/FloatingTextManager.h"
#include "nms/RenderHooks.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

// ⚠⚠ THE TRAMPOLINE BODIES. nms/RenderHooks.h only DECLARES these; upstream defines them in its
// Hooks.cpp, which we did not take. Without them the link fails with three unresolved externals --
// and that is the good outcome, because a DETOUR_TRAMPOLINE_EMPTY body is a deliberate null
// dereference (§49) that only faults if the detour was never installed.
// ⚠ The calling convention must match the declaration in RenderHooks.h exactly (__stdcall, via
// WINAPI) or the linker keeps looking for the mangled name it cannot find.
DETOUR_TRAMPOLINE_EMPTY(HRESULT WINAPI RenderHooks::BeginScene_Trampoline());
DETOUR_TRAMPOLINE_EMPTY(HRESULT WINAPI RenderHooks::EndScene_Trampoline());
DETOUR_TRAMPOLINE_EMPTY(HRESULT WINAPI RenderHooks::Reset_Trampoline(D3DPRESENT_PARAMETERS* pPresentationParameters));

// ===================================================================== globals RenderHooks expects
// ⚠️ These names are fixed by nms/RenderHooks.h, which is vendored as close to upstream as possible
// so it can be re-synced. Do not rename them here.
FloatingTextManager* g_pFtm             = nullptr;
IDirect3DDevice9*    g_pDevice          = nullptr;
bool                 g_deviceAcquired   = false;
HMODULE              g_d3d9Module       = nullptr;
DWORD                g_resetDeviceAddress = 0;

static bool g_ftInstalled = false;

// ⚠️ Declared `extern` by nms/FloatingTextManager.h and defined by the HOST, not by the vendored
// files -- upstream keeps it in its plugin .cpp. Without this the link fails with an unresolved
// external, which is the one failure mode in this whole port that is loud rather than silent.
// 📌 Projects the target's world position into screen space using the matrices EQ has already set on
// the device this frame. It is only meaningful inside a render hook, which is the only place the
// manager calls it.
void WorldToScreen(D3DXVECTOR3 world, D3DXVECTOR3* screen)
{
	if (!g_pDevice || !screen) { return; }

	D3DXMATRIX   view, projection, worldMat;
	D3DVIEWPORT9 viewport;

	g_pDevice->GetTransform(D3DTS_VIEW,       &view);
	g_pDevice->GetTransform(D3DTS_PROJECTION, &projection);
	g_pDevice->GetTransform(D3DTS_WORLD,      &worldMat);
	g_pDevice->GetViewport(&viewport);

	D3DXVec3Project(screen, &world, &viewport, &projection, &view, &worldMat);
}

// Debug trace to <EQ>\aotv4_floatingtext.log. Same reason every other module here has one: a render
// hook that does not fire leaves NOTHING in any client log, and an unbuilt dll, a device that was
// never acquired and a packet that never arrived all look identical from in game.
static void FtTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) { sprintf_s(path, "%s\\aotv4_floatingtext.log", gszEQPath); }
	else             { strcpy_s(path, "aotv4_floatingtext.log"); }

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

// ⚠️ RenderHooks.h declares this as a template and leaves it to the host to define. EzDetour is what
// the rest of this dll uses, so it is what this forwards to -- one detour mechanism in the dll, not
// two competing ones.
template <typename T>
void InstallDetour(DWORD address, const T& detour, const T& trampoline, PCHAR name)
{
	if (!address) { FtTrace("InstallDetour: %s has a NULL address, skipped", name ? name : "?"); return; }
	EzDetour(address, detour, trampoline);
	FtTrace("InstallDetour: %s at 0x%08X", name ? name : "?", address);
}

// ===================================================================== the wire format

// ⚠️⚠️ PACKED. The field offsets are 0/2/4/5/7, so this straddles alignment boundaries and the
// compiler WILL pad it to something else without the pragma -- silently, and the damage would then be
// read out of the middle of nothing.
#pragma pack(push, 1)
// ⚠️⚠️ THIS IS THE RoF2 WIRE LAYOUT, READ OUT OF OUR OWN SERVER, NOT THE ONE WE INHERITED.
// It is common/patches/rof2_structs.h CombatDamage_Struct, and the version that shipped here first --
// taken from NMS -- disagreed with it in three fields at once: spellid uint16 (it is uint32), damage
// int64 at offset 7 (it is int32 at 9), and a "hitType" byte at 15 that does not exist in RoF2 at all.
// So even with the right opcode, reading it would have produced garbage numbers over the wrong spawns.
//
// 📌 That mismatch is also how the opcode was finally identified: sizeof is exactly 30, and 0x6F15 was
// the ONLY size-30 opcode out of 256 distinct (opcode, size) pairs observed in a play session -- while
// its count tracked the number of swings landed. Counts alone could not do it; the size did.
//
// ⚠️ PACKED. Natural alignment pads this to 32 and every field from spellid on reads shifted.
#pragma pack(push, 1)
struct AoTv4CombatAction
{
	/* 00 */ uint16_t target;
	/* 02 */ uint16_t source;
	/* 04 */ uint8_t  type;         // damage skill; 231 (0xE7) means a spell
	/* 05 */ uint32_t spellid;      // 0xFFFF (SPELL_UNKNOWN) when this was not a spell
	/* 09 */ int32_t  damage;       // <= 0 for a miss, a full absorb or a deflect
	/* 13 */ float    force;
	/* 17 */ float    hit_heading;
	/* 21 */ float    hit_pitch;
	/* 25 */ uint8_t  secondary;    // 0 primary hand, 1 secondary
	/* 26 */ uint32_t special;      // 2 = Rampage, 1 = Wild Rampage
	/* 30 */
};
#pragma pack(pop)

// RoF2 has no hit-type bitfield -- the vendored renderer wants one, so we synthesise it from `type`.
// ⚠️ The renderer reads these as BITS (bit 2 -> green, bit 1 -> smaller text), so they are flags and
// not an enum. Anything over the local player is already forced red by AddDamageText itself.
static const uint8_t FT_HIT_SPELL = 0x04;   // non-melee damage, drawn green
static const uint8_t FT_HIT_MINOR = 0x02;   // off-hand, drawn smaller
static const uint8_t FT_HIT_HEAL  = 0x04;   // healing -- shares the green the renderer gives bit 2

// The skill id RoF2 uses to mean "this was a spell, not a weapon skill".
static const uint8_t FT_SKILL_SPELL = 231;
#pragma pack(pop)

// ===================================================================== install
bool InstallD3D9HooksForFloatingText()
{
	g_d3d9Module = GetModuleHandleA("d3d9.dll");
	if (!g_d3d9Module) { FtTrace("d3d9.dll is not loaded yet"); return false; }

	// ⚠️⚠️ Direct3DCreate9**Ex**, and D3DDEVTYPE_NULLREF WITH NO WINDOW. The non-Ex path with a HAL
	// device and a real HWND is what the earlier attempt used and it is why this was written off as
	// impossible -- see core_floatingtext.h.
	auto pCreate9Ex = (HRESULT (WINAPI*)(UINT, IDirect3D9Ex**))
		GetProcAddress(g_d3d9Module, "Direct3DCreate9Ex");
	if (!pCreate9Ex) { FtTrace("Direct3DCreate9Ex not exported -- cannot proceed"); return false; }

	IDirect3D9Ex* d3d9ex = nullptr;
	if (FAILED(pCreate9Ex(D3D_SDK_VERSION, &d3d9ex)) || !d3d9ex) {
		FtTrace("Direct3DCreate9Ex failed");
		return false;
	}

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(pp));
	pp.Windowed             = 1;
	pp.SwapEffect           = D3DSWAPEFFECT_FLIP;
	pp.BackBufferFormat     = D3DFMT_A8R8G8B8;
	pp.BackBufferCount      = 1;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	IDirect3DDevice9Ex* deviceEx = nullptr;
	HRESULT hr = d3d9ex->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_NULLREF,
		0,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES,
		&pp, nullptr, &deviceEx);

	if (FAILED(hr) || !deviceEx) {
		FtTrace("CreateDeviceEx failed, hr=0x%08X", hr);
		d3d9ex->Release();
		return false;
	}

	// ⚠️⚠️ WE DETOUR THE FUNCTION ADDRESS READ OUT OF THIS VTABLE, NOT THE VTABLE SLOT. That address
	// is the shared d3d9.dll implementation every device in the process calls, EQ's included -- which
	// is the whole reason a throwaway device is enough to find it. We never render to this device; the
	// detour captures EQ's own from its `this` pointer.
	DWORD* vft = *(DWORD**)deviceEx;

	InstallDetour(vft[0x29], &RenderHooks::BeginScene_Detour,
	                          &RenderHooks::BeginScene_Trampoline, (PCHAR)"d3dDevice_BeginScene");
	InstallDetour(vft[0x2a], &RenderHooks::EndScene_Detour,
	                          &RenderHooks::EndScene_Trampoline,   (PCHAR)"d3dDevice_EndScene");

	// ⚠️ Release the throwaway. Keeping it alive pins a NULLREF device for the life of the process
	// for no reason -- the detours are already installed on shared code.
	deviceEx->Release();
	d3d9ex->Release();

	FtTrace("D3D9 hooks installed (BeginScene 0x29, EndScene 0x2a)");
	return true;
}

void FloatingTextInit()
{
	if (g_ftInstalled) { return; }

	if (!g_pFtm) { g_pFtm = new FloatingTextManager(); }

	// ⚠️ The manager is NOT initialised here. RenderHooks::EndScene_Detour calls Initialize() once
	// TestCooperativeLevel says the device is actually usable -- doing it now would build fonts
	// against a device that may not be ready, and that fails silently.
	// ⚠️ Settings first: the packet gate reads areFloatingTextEnabled, and the ini can turn the whole
	// feature off. Installing the device hooks for a feature the player has disabled is wasted work.
	FloatingTextLoadSettings();

	g_ftInstalled = InstallD3D9HooksForFloatingText();
	FtTrace("FloatingTextInit: installed=%d", g_ftInstalled ? 1 : 0);
}

void FloatingTextOnUiReset()
{
	// ⚠️ Drop the FONTS, not the hooks. A /loadskin tears the UI down and takes D3D resources with
	// it; the detours are on shared d3d9 code and outlive that. Re-acquisition happens on the next
	// EndScene, which is what g_deviceAcquired gates.
	if (g_pFtm) { g_pFtm->Cleanup(); }
	g_deviceAcquired = false;
	FtTrace("FloatingTextOnUiReset: fonts released, awaiting re-acquire");
}

// ===================================================================== the feed
// Defined with the settings at the foot of this file.
static bool FtWantHit(unsigned __int16 target, unsigned __int16 source);
void FloatingTextCount(int which);

void FloatingTextOnCombatAction(const char* buf, size_t size)
{
	if (!g_pFtm || !g_deviceAcquired) { return; }
	if (!buf || size < sizeof(AoTv4CombatAction)) { return; }

	const AoTv4CombatAction* act = (const AoTv4CombatAction*)buf;

	// A miss, a deflect or a full absorb comes through this same packet with no damage. They are the
	// MAJORITY of it in a real fight -- roughly a third of the events in the session that identified
	// this opcode -- so this early-out is doing most of the work, not guarding a rare case.
	FloatingTextCount(0);
	if (act->damage <= 0) { FloatingTextCount(1); return; }

	// ⚠️ Filtered BEFORE the spawn lookup, so turning a category off costs nothing per hit and so the
	// "no spawn" counter only ever counts hits the player actually wanted to see.
	if (!FtWantHit(act->target, act->source)) { FloatingTextCount(2); return; }

	// ⚠️ The number goes over the TARGET, so that is the spawn we resolve. Resolving the source would
	// stack every number in a group fight on top of whoever swung.
	PSPAWNINFO target = (PSPAWNINFO)GetSpawnByID(act->target);
	if (!target) { FloatingTextCount(3); return; }
	FloatingTextCount(4);

	const bool is_spell = (act->type == FT_SKILL_SPELL);

	uint8_t flags = 0;
	if (is_spell)       { flags |= FT_HIT_SPELL; }
	if (act->secondary) { flags |= FT_HIT_MINOR; }

	// ⚠️ 0xFFFF is SPELL_UNKNOWN, widened to uint32 by the RoF2 encode rather than zeroed, so a
	// melee swing arrives carrying a spell id of 65535. Passing that on sends the renderer looking for
	// a spell icon that cannot exist.
	const int spell_id = (is_spell && act->spellid > 0 && act->spellid < 0xFFFF)
	                   ? (int)act->spellid : 0;

	g_pFtm->AddDamageText(target, (int)act->damage, spell_id, flags);
}

// ===================================================================== opcode discovery
// ⚠️⚠️ THIS EXISTS BECAUSE AOTV4_OP_COMBAT_ACTION IS UNVERIFIED. Upstream's only user of that value
// is not in their project file and references a struct their tree does not define, so it has never
// been compiled, let alone run. Rather than guess-build-guess, this records what the client actually
// receives and lets one play session settle it.
//
// ⚠️ DISTINCT PAIRS ONLY. Logging every packet would write tens of thousands of lines a minute and
// bury the answer. Each (opcode, size) is logged the first time it is seen and counted thereafter.
static const int FT_TRACE_MAX = 256;
static struct { unsigned __int16 op; unsigned int sz; unsigned int hits; } g_ftTrace[FT_TRACE_MAX];
static int g_ftTraceN = 0;

void FloatingTextTraceOpcode(unsigned __int16 opcode, size_t size)
{
	const unsigned int sz = (unsigned int)size;

	for (int i = 0; i < g_ftTraceN; ++i) {
		if (g_ftTrace[i].op == opcode && g_ftTrace[i].sz == sz) { g_ftTrace[i].hits++; return; }
	}
	if (g_ftTraceN >= FT_TRACE_MAX) { return; }

	g_ftTrace[g_ftTraceN].op   = opcode;
	g_ftTrace[g_ftTraceN].sz   = sz;
	g_ftTrace[g_ftTraceN].hits = 1;
	g_ftTraceN++;

	// 📌 A combat action carries target, source, skill, spell id, damage and hit type -- 16 bytes in
	// the layout we inherited. Flagging the plausible sizes makes the candidate obvious in the log
	// without pre-judging it: the real one is whatever climbs while you are swinging.
	const char* note = (sz >= 12 && sz <= 24) ? "   <-- plausible combat action size" : "";
	FtTrace("opcode 0x%04X  size %u  (new)%s", opcode, sz, note);
}

// Defined with the ring at the foot of this file; /fttrace dumps the table and the ring together,
// because the two are only useful read against each other.
static void FtDumpRing();

void FloatingTextDumpOpcodes()
{
	FtTrace("---- opcode table (%d distinct pairs) ----", g_ftTraceN);
	for (int i = 0; i < g_ftTraceN; ++i) {
		FtTrace("  0x%04X  size %-5u  seen %u", g_ftTrace[i].op, g_ftTrace[i].sz, g_ftTrace[i].hits);
	}
	FtTrace("---- end ----");
	FtDumpRing();
}


// ===================================================================== payload ring
// ⚠️⚠️ COUNTS ALONE DO NOT IDENTIFY THE COMBAT OPCODE. The first trace pass produced 256 distinct
// (opcode, size) pairs and half a dozen candidates that all climbed at roughly the rate you swing.
// The only conclusive test is to read the BYTES and find a damage number you KNOW you dealt sitting
// at a fixed offset -- so this records payloads and the player supplies the number from their own
// chat log.
//
// ⚠⚠ IT IS A RING, DELIBERATELY, AND THE FIRST VERSION OF THIS WAS WRONG FOR AN OBVIOUS REASON.
// That one logged the first few payloads of each distinct opcode, which sounds equivalent and is not:
// an opcode that fires 46 times over a session gets captured on its FIRST three, and the first three
// are almost always zone-in traffic rather than the fight. Keeping the most RECENT payloads instead
// means "hit something, then type /fttrace" captures exactly the swings you just made -- which is the
// only window where the player also knows what the damage numbers were.
static const size_t FT_RING_MIN  = 8;    // below this nothing can carry source, target and damage
static const size_t FT_RING_MAX  = 64;   // above this it is a spawn/inventory/zone payload, not a hit
static const int    FT_RING_SLOTS = 96;

static struct FtRingEntry {
	unsigned __int16 op;
	unsigned int     sz;
	unsigned int     tick;
	unsigned char    data[FT_RING_MAX];
} g_ftRing[FT_RING_SLOTS];
static int g_ftRingN = 0;   // total recorded; the ring holds the last FT_RING_SLOTS of them

void FloatingTextDumpPayload(unsigned __int16 opcode, const char* buf, size_t size)
{
	if (!buf || size < FT_RING_MIN || size > FT_RING_MAX) { return; }

	FtRingEntry& e = g_ftRing[g_ftRingN % FT_RING_SLOTS];
	e.op   = opcode;
	e.sz   = (unsigned int)size;
	e.tick = GetTickCount();
	memcpy(e.data, buf, size);
	g_ftRingN++;
}

// Print the ring newest-first, with every plausible damage field decoded so a number can be spotted
// without hand-decoding hex.
// ⚠ The candidate offsets are a spread, not a claim: the layout we inherited puts damage at offset 7,
// but that came from a file that has never compiled, so 4/6/8/10/12 are printed alongside it. Whichever
// offset shows the damage the player actually dealt is the real one.
static void FtDumpRing()
{
	const int have  = (g_ftRingN < FT_RING_SLOTS) ? g_ftRingN : FT_RING_SLOTS;
	const unsigned now = GetTickCount();

	FtTrace("---- payload ring (%d of %d recorded, newest first) ----", have, g_ftRingN);
	for (int k = 1; k <= have; ++k) {
		const FtRingEntry& e = g_ftRing[(g_ftRingN - k) % FT_RING_SLOTS];

		char hex[FT_RING_MAX * 3 + 1];
		int  n = 0;
		for (unsigned int i = 0; i < e.sz && i < FT_RING_MAX; ++i) {
			n += sprintf_s(hex + n, sizeof(hex) - n, "%02X ", e.data[i]);
		}

		// Every 16-bit and 32-bit read the packet can support, so no offset guess is needed.
		char dec[512];
		int  d = sprintf_s(dec, sizeof(dec), "u16:");
		for (unsigned int o = 0; o + 2 <= e.sz && o <= 16; o += 2) {
			d += sprintf_s(dec + d, sizeof(dec) - d, " %u@%u", *(const unsigned __int16*)(e.data + o), o);
		}
		d += sprintf_s(dec + d, sizeof(dec) - d, " | i32:");
		for (unsigned int o = 0; o + 4 <= e.sz && o <= 16; ++o) {
			d += sprintf_s(dec + d, sizeof(dec) - d, " %d@%u", *(const __int32*)(e.data + o), o);
		}

		FtTrace("  -%5ums 0x%04X sz %-2u | %s| %s", now - e.tick, e.op, e.sz, hex, dec);
	}
	FtTrace("---- end ring ----");
}

// ===================================================================== settings and /fct
// Player-facing controls. Everything here is presentation only -- the packet still arrives and is
// still read; these decide whether a number is built and how it looks.
//
// 📌 Settings live in <EQ>\aotv4_floatingtext.ini rather than in a data bucket, because they are a
// per-CLIENT display preference, not per-character server state. Nothing here reaches the server.
int   g_ftRisePixels = 150;    // how far a number travels before it is gone
int   g_ftDurationMs = 1000;   // how long it takes to get there and fade out
float g_ftScalePct   = 1.0f;   // 1.0 == the size upstream drew at

bool g_ftShowOutgoing = true;    // damage you deal
bool g_ftShowIncoming = true;    // damage dealt to you
bool g_ftShowOthers   = false;   // everyone else's fights
bool g_ftShowHeals    = true;    // healing, from the server's FCTHEAL transport

// Display model. 0 = over the target (the number follows the thing it happened to), 1 = two fixed
// screen anchors, damage on one and healing on the other.
// ⚠️⚠️ THE TWO ANCHORS SPLIT BY DAMAGE-vs-HEALING, NOT BY DEALT-vs-TAKEN. That is the owner's call and
// it is worth stating, because every other combat-text mod splits the other way: what you want to read
// mid-fight is "am I taking damage" against "am I being healed", and colour already tells you which
// direction a number is going.
int   g_ftMode         = 0;
// 📌 Flanking the player, a little below centre -- roughly where a character stands on screen, which is
// where the eye already is during a fight. Reported from play that the first defaults put the markers in
// a corner; that was actually PlaceAtAnchor missing rather than these values, but the middle of the
// screen is still the better starting point than either side of it.
float g_ftAnchorDmgX   = 0.62f, g_ftAnchorDmgY  = 0.55f;
float g_ftAnchorHealX  = 0.38f, g_ftAnchorHealY = 0.55f;
int   g_ftCascade      = 1;      // 0 stack, 1 fanned, 2 straight+stagger
int   g_ftFanWidth     = 60;     // px, widest horizontal travel under the fanned cascade

// ⚠️ A FIXED LIST, not free text. D3DXCreateFont SUCCEEDS on an unknown face -- GDI silently
// substitutes -- so a typo would render in the wrong typeface with nothing to indicate why.
char  g_ftFontFace[64] = "Arial";
int   g_ftFontSize     = 24;

// 0xRRGGBB. Indexed by EFtColour (FloatingTextManager.h).
// 📌 Defaults chosen so the four are separable at a glance and none of them is red-on-red: damage you
// deal is warm white, damage on you is red, healing is green, a crit is gold. Healing being green
// rather than red is the point -- see the note where the colour is applied.
unsigned int g_ftColour[FT_COLOUR_COUNT] = {
	0xFFF2E0,   // weapon swing      -- warm white
	0x8AB4FF,   // spell             -- cool blue, so a nuke reads apart from a swing at a glance
	0xFF9A3C,   // combat ability    -- orange, distinct from both
	0xFF5040,   // damage on you
	0x60FF80,   // healing
	0xFFD24A    // critical
};
int          g_ftLaneGapPx = 22;

static const char* kFtFonts[] = { "Arial", "Tahoma", "Verdana", "Georgia", "Impact", "Courier New" };
static const int   kFtFontCount = (int)(sizeof(kFtFonts) / sizeof(kFtFonts[0]));

// ⚠️⚠️ COUNTERS EXIST BECAUSE "IT SHOWS UP SOMETIMES" IS NOT A DIAGNOSIS. A number can fail to appear
// for five unrelated reasons and all five look identical in game. These separate them, so /fct answers
// "the packets are arriving and being filtered out" versus "no packets are arriving" versus "the spawn
// could not be resolved" -- which need completely different fixes.
static unsigned int g_ftSeen = 0;       // combat packets read
static unsigned int g_ftNoDamage = 0;   //   ... a miss, deflect or absorb
static unsigned int g_ftFiltered = 0;   //   ... excluded by the settings above
static unsigned int g_ftNoSpawn = 0;    //   ... target not in the client's spawn list
static unsigned int g_ftDrawn = 0;      //   ... turned into a number

// ⚠️⚠️ THE PATH MUST BE ABSOLUTE, AND IT MUST NOT DEPEND ON gszEQPath.
// Reported from play: settings reverted to defaults after a logout. Two things cause that and this
// fixes both. First, WritePrivateProfileString given a BARE filename does not write beside the exe --
// Windows resolves it against the Windows directory, so the save and the load can land on different
// files or fail outright. Second, gszEQPath is not guaranteed populated when FloatingTextInit runs, so
// the early load could take the bare-name path while a later save took the absolute one.
// 📌 Derived from OUR OWN dll's location, which is next to eqgame.exe by definition -- that is how a
// dinput8 proxy is loaded at all -- so it is correct before any client global is ready.
static void FtIniPath(char* out, size_t n)
{
	static char cached[MAX_PATH] = {0};
	if (cached[0]) { strcpy_s(out, n, cached); return; }

	HMODULE self = nullptr;
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)&FtIniPath, &self) && self) {
		char mod[MAX_PATH] = {0};
		if (GetModuleFileNameA(self, mod, sizeof(mod))) {
			char* slash = strrchr(mod, '\\');
			if (slash) {
				*slash = 0;
				sprintf_s(cached, sizeof(cached), "%s\\aotv4_floatingtext.ini", mod);
			}
		}
	}
	if (!cached[0] && gszEQPath[0]) {
		sprintf_s(cached, sizeof(cached), "%s\\aotv4_floatingtext.ini", gszEQPath);
	}
	if (!cached[0]) { strcpy_s(cached, sizeof(cached), ".\\aotv4_floatingtext.ini"); }
	strcpy_s(out, n, cached);
}

void FloatingTextSaveSettings()
{
	char ini[MAX_PATH]; FtIniPath(ini, sizeof(ini));
	char v[32];
	#define FT_PUT(k, val) sprintf_s(v, "%d", (int)(val)); WritePrivateProfileStringA("FloatingText", k, v, ini)
	FT_PUT("Enabled",  areFloatingTextEnabled ? 1 : 0);
	FT_PUT("Outgoing", g_ftShowOutgoing ? 1 : 0);
	FT_PUT("Incoming", g_ftShowIncoming ? 1 : 0);
	FT_PUT("Others",   g_ftShowOthers ? 1 : 0);
	FT_PUT("Rise",     g_ftRisePixels);
	FT_PUT("Duration", g_ftDurationMs);
	FT_PUT("ScalePct", (int)(g_ftScalePct * 100.0f));
	FT_PUT("Heals",    g_ftShowHeals ? 1 : 0);
	FT_PUT("Mode",     g_ftMode);
	FT_PUT("Cascade",  g_ftCascade);
	FT_PUT("FanWidth", g_ftFanWidth);
	FT_PUT("FontSize", g_ftFontSize);
	// ⚠️ Anchors are stored as PER MILLE of the viewport. The ini API is integer-only, and percent
	// would quantise a 2560-wide screen to 26-pixel steps -- visibly coarse when placing a marker.
	FT_PUT("DmgX",  (int)(g_ftAnchorDmgX  * 1000.0f));
	FT_PUT("DmgY",  (int)(g_ftAnchorDmgY  * 1000.0f));
	FT_PUT("HealX", (int)(g_ftAnchorHealX * 1000.0f));
	FT_PUT("HealY", (int)(g_ftAnchorHealY * 1000.0f));
	FT_PUT("LaneGap",     g_ftLaneGapPx);
	FT_PUT("ColourMelee", (int)g_ftColour[FT_COLOUR_MELEE]);
	FT_PUT("ColourSpell", (int)g_ftColour[FT_COLOUR_SPELL]);
	FT_PUT("ColourAbility", (int)g_ftColour[FT_COLOUR_ABILITY]);
	FT_PUT("ColourTaken", (int)g_ftColour[FT_COLOUR_TAKEN]);
	FT_PUT("ColourHeal",  (int)g_ftColour[FT_COLOUR_HEAL]);
	FT_PUT("ColourCrit",  (int)g_ftColour[FT_COLOUR_CRIT]);
	#undef FT_PUT
	WritePrivateProfileStringA("FloatingText", "FontFace", g_ftFontFace, ini);
}

void FloatingTextSetFont(const char* face, int size);   // defined below

void FloatingTextLoadSettings()
{
	char ini[MAX_PATH]; FtIniPath(ini, sizeof(ini));

	// ⚠️ areFloatingTextEnabled is the BUILD flag's value on entry, and it stays the default here on
	// purpose: a build with the feature compiled off must not be switched on by a stale ini.
	areFloatingTextEnabled = GetPrivateProfileIntA("FloatingText", "Enabled",
	                            areFloatingTextEnabled ? 1 : 0, ini) != 0;
	g_ftShowOutgoing = GetPrivateProfileIntA("FloatingText", "Outgoing", 1, ini) != 0;
	g_ftShowIncoming = GetPrivateProfileIntA("FloatingText", "Incoming", 1, ini) != 0;
	g_ftShowOthers   = GetPrivateProfileIntA("FloatingText", "Others",   0, ini) != 0;

	// ⚠️ Clamped on the way IN, not just when set. An ini is a text file a player can edit, and a
	// duration of 0 divides by zero inside tweeny while a rise of 100000 throws every number off screen
	// -- both of which read as "the feature is broken" rather than "I typed a silly number".
	g_ftRisePixels = GetPrivateProfileIntA("FloatingText", "Rise",     150,  ini);
	g_ftDurationMs = GetPrivateProfileIntA("FloatingText", "Duration", 1000, ini);
	int pct        = GetPrivateProfileIntA("FloatingText", "ScalePct", 100,  ini);

	if (g_ftRisePixels < 10)   { g_ftRisePixels = 10; }
	if (g_ftRisePixels > 1000) { g_ftRisePixels = 1000; }
	if (g_ftDurationMs < 200)   { g_ftDurationMs = 200; }
	if (g_ftDurationMs > 10000) { g_ftDurationMs = 10000; }
	if (pct < 25)  { pct = 25; }
	if (pct > 400) { pct = 400; }
	g_ftScalePct = pct / 100.0f;

	g_ftShowHeals = GetPrivateProfileIntA("FloatingText", "Heals", 1, ini) != 0;
	g_ftMode      = GetPrivateProfileIntA("FloatingText", "Mode",     0, ini) ? 1 : 0;
	g_ftCascade   = GetPrivateProfileIntA("FloatingText", "Cascade",  1, ini);
	g_ftFanWidth  = GetPrivateProfileIntA("FloatingText", "FanWidth", 60, ini);
	g_ftFontSize  = GetPrivateProfileIntA("FloatingText", "FontSize", 24, ini);

	if (g_ftCascade  < 0)   { g_ftCascade = 0; }
	if (g_ftCascade  > 2)   { g_ftCascade = 2; }
	if (g_ftFanWidth < 0)   { g_ftFanWidth = 0; }
	if (g_ftFanWidth > 400) { g_ftFanWidth = 400; }
	if (g_ftFontSize < 8)   { g_ftFontSize = 8; }
	if (g_ftFontSize > 96)  { g_ftFontSize = 96; }

	// ⚠️ Anchors are clamped to [0.02, 0.98] rather than [0, 1]. An anchor exactly on an edge puts the
	// number half off screen from the moment it appears, which reads as the setting not working.
	g_ftAnchorDmgX  = GetPrivateProfileIntA("FloatingText", "DmgX",  620, ini) / 1000.0f;
	g_ftAnchorDmgY  = GetPrivateProfileIntA("FloatingText", "DmgY",  550, ini) / 1000.0f;
	g_ftAnchorHealX = GetPrivateProfileIntA("FloatingText", "HealX", 380, ini) / 1000.0f;
	g_ftAnchorHealY = GetPrivateProfileIntA("FloatingText", "HealY", 550, ini) / 1000.0f;

	float* anchors[4] = { &g_ftAnchorDmgX, &g_ftAnchorDmgY, &g_ftAnchorHealX, &g_ftAnchorHealY };
	for (int i = 0; i < 4; ++i) {
		if (*anchors[i] < 0.02f) { *anchors[i] = 0.02f; }
		if (*anchors[i] > 0.98f) { *anchors[i] = 0.98f; }
	}

	g_ftLaneGapPx = GetPrivateProfileIntA("FloatingText", "LaneGap", 22, ini);
	if (g_ftLaneGapPx < 0)   { g_ftLaneGapPx = 0; }
	if (g_ftLaneGapPx > 120) { g_ftLaneGapPx = 120; }

	// ⚠️ Masked to 24 bits on the way in. The ini is a text file, and a value with anything in the top
	// byte would be read as a colour with a wildly wrong red channel rather than being rejected.
	static const char* keys[FT_COLOUR_COUNT] = {
		"ColourMelee", "ColourSpell", "ColourAbility", "ColourTaken", "ColourHeal", "ColourCrit"
	};
	for (int i = 0; i < FT_COLOUR_COUNT; ++i) {
		const int v = GetPrivateProfileIntA("FloatingText", keys[i], (int)g_ftColour[i], ini);
		g_ftColour[i] = ((unsigned int)v) & 0x00FFFFFFu;
	}

	char face[64] = {0};
	GetPrivateProfileStringA("FloatingText", "FontFace", "Arial", face, sizeof(face), ini);
	FloatingTextSetFont(face, g_ftFontSize);
}

// Decide whether this hit is one the player asked to see. Split out so the counters and the command
// agree on what "filtered" means.
static bool FtWantHit(unsigned __int16 target, unsigned __int16 source)
{
	PSPAWNINFO me = (PSPAWNINFO)pLocalPlayer;
	if (!me) { return g_ftShowOthers; }

	const unsigned __int16 mine = (unsigned __int16)me->SpawnID;

	// ⚠️ Tested in this order because a hit where BOTH ends are you -- a damage shield, a lifetap's own
	// backlash -- is incoming from the player's point of view, not outgoing.
	if (target == mine) { return g_ftShowIncoming; }
	if (source == mine) { return g_ftShowOutgoing; }
	return g_ftShowOthers;
}

// Defined in core_spellwindow.cpp, which owns the dsp_chat detour and its trampoline.
// \u26a0 Declared rather than included: core_spellwindow has no header, and this dll's convention is
// that a module reaching into another's TU takes an extern for the one symbol it needs.
extern void AoTv4ChatPrint(const char* line);
// Defined in core_spellwindow.cpp: run a client command on the GAME thread on the next frame.
extern void AoTQueueGameCommand(const char* cmd);

static void FtMsg(const char* fmt, ...)
{
	// ⚠️⚠️ NOT WriteChatColor -- CLAUDE.md section 49, it is a guaranteed crash here. AoTv4ChatPrint
	// (core_spellwindow.cpp) goes through the dsp_chat trampoline this dll already owns.
	char line[512];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(line, sizeof(line), fmt, args);
	va_end(args);

	AoTv4ChatPrint(line);
}

static void FtPrintStatus()
{
	FtMsg("Floating combat text: %s", areFloatingTextEnabled ? "ON" : "OFF");
	FtMsg("  showing:  yours %s   on you %s   others %s",
	      g_ftShowOutgoing ? "on" : "off",
	      g_ftShowIncoming ? "on" : "off",
	      g_ftShowOthers   ? "on" : "off");
	FtMsg("  size %d%%   rise %d px   fade %d ms",
	      (int)(g_ftScalePct * 100.0f), g_ftRisePixels, g_ftDurationMs);
	FtMsg("  hits seen %u:  drawn %u, no damage %u, filtered %u, no spawn %u",
	      g_ftSeen, g_ftDrawn, g_ftNoDamage, g_ftFiltered, g_ftNoSpawn);
	// ⚠️ "seen 0" and "seen but not drawn" are completely different faults with completely different
	// fixes, and they look identical in game. Say which one it is rather than leaving the player to
	// infer it from a row of numbers.
	if (g_ftSeen == 0) {
		FtMsg("  Nothing is arriving from the server. Try /fct feed, then hit something.");
	}
	{
		// ⚠️⚠️ THE INI ANSWERS FOR ITSELF, ASKED TWICE WITH DIFFERENT DEFAULTS. GetPrivateProfileInt
		// cannot tell "key absent" from "key is 0" -- it returns the default in both cases -- so a single
		// read can never prove the file was found. Matching answers mean the key is really there.
		// 📌 Reported from play as the spots being right but the numbers not appearing at them, which is
		// exactly what a lost `Mode` looks like: the marker WINDOWS are positioned by the client's own
		// per character UI memory, not by this file, so the spots can look perfectly correct while none
		// of our settings loaded at all.
		char ini[MAX_PATH]; FtIniPath(ini, sizeof(ini));
		const int p1 = GetPrivateProfileIntA("FloatingText", "Mode", 7, ini);
		const int p2 = GetPrivateProfileIntA("FloatingText", "Mode", 9, ini);
		FtMsg("  numbers appear: %s", g_ftMode == 1 ? "at the two spots" : "over the target");
		FtMsg("  settings file: %s", (p1 == p2) ? "loaded" : "NOT FOUND, nothing will persist");
		FtMsg("  %s", ini);
	}
	FtMsg("  /fct on|off | mine|taken|others | size N | rise N | fade N | feed | reset | trace");
}

bool FloatingTextCommand(const char* args)
{
	while (*args == ' ') { ++args; }

	if (*args == 0)                                { FtPrintStatus(); return true; }

	if (_strnicmp(args, "on", 2) == 0)  { areFloatingTextEnabled = true;  FloatingTextSaveSettings(); FtPrintStatus(); return true; }
	if (_strnicmp(args, "off", 3) == 0) { areFloatingTextEnabled = false; FloatingTextSaveSettings(); FtPrintStatus(); return true; }

	// Each of the three toggles flips rather than takes an argument: they are read far more often than
	// they are set, and "/fct mine" is quicker to type than "/fct mine off" when you already know it is on.
	if (_strnicmp(args, "mine", 4) == 0)   { g_ftShowOutgoing = !g_ftShowOutgoing; FloatingTextSaveSettings(); FtPrintStatus(); return true; }
	if (_strnicmp(args, "taken", 5) == 0)  { g_ftShowIncoming = !g_ftShowIncoming; FloatingTextSaveSettings(); FtPrintStatus(); return true; }
	if (_strnicmp(args, "others", 6) == 0) { g_ftShowOthers   = !g_ftShowOthers;   FloatingTextSaveSettings(); FtPrintStatus(); return true; }

	if (_strnicmp(args, "size", 4) == 0) {
		int pct = atoi(args + 4);
		if (pct < 25 || pct > 400) { FtMsg("Size must be 25 to 400 percent."); return true; }
		g_ftScalePct = pct / 100.0f; FloatingTextSaveSettings(); FtPrintStatus(); return true;
	}
	if (_strnicmp(args, "rise", 4) == 0) {
		int px = atoi(args + 4);
		if (px < 10 || px > 1000) { FtMsg("Rise must be 10 to 1000 pixels."); return true; }
		g_ftRisePixels = px; FloatingTextSaveSettings(); FtPrintStatus(); return true;
	}
	if (_strnicmp(args, "fade", 4) == 0) {
		int ms = atoi(args + 4);
		if (ms < 200 || ms > 10000) { FtMsg("Fade must be 200 to 10000 ms."); return true; }
		g_ftDurationMs = ms; FloatingTextSaveSettings(); FtPrintStatus(); return true;
	}

	if (_strnicmp(args, "reset", 5) == 0) {
		// ⚠️ Resets the COUNTERS, not the settings. Zeroing them before a controlled test is the whole
		// point of having them -- "I hit it ten times and drawn went up by four" is a measurement.
		g_ftSeen = g_ftNoDamage = g_ftFiltered = g_ftNoSpawn = g_ftDrawn = 0;
		FtMsg("Floating text counters reset."); return true;
	}

	if (_strnicmp(args, "feed", 4) == 0) {
		// Re-send the opt-in by hand. The server keeps it on the Client object and rebuilds that on every
		// zone, so this is the first thing to try when numbers stop after zoning.
		FloatingTextAnnounceHeals();
		FtMsg("Asked the server for combat text. Hit something, then /fct to check.");
		return true;
	}

	if (_strnicmp(args, "trace", 5) == 0) {
		areFloatingTextOpcodeTrace = !areFloatingTextOpcodeTrace;
		FtMsg("Floating text opcode trace %s.", areFloatingTextOpcodeTrace ? "ON" : "OFF");
		return true;
	}

	FtPrintStatus();
	return true;
}

// Bump the counters from the packet reader. Kept here so the counter names and the status line that
// prints them cannot drift apart.
void FloatingTextCount(int which)
{
	switch (which) {
		case 0: ++g_ftSeen;     break;
		case 1: ++g_ftNoDamage; break;
		case 2: ++g_ftFiltered; break;
		case 3: ++g_ftNoSpawn;  break;
		case 4: ++g_ftDrawn;    break;
	}
}


// ===================================================================== fonts, heals, accessors
// Change the typeface or size and rebuild the fonts.
// ⚠️⚠️ EVERY LIVE NUMBER MUST GO FIRST. A DamageText holds a raw ID3DXFont* handed to it at creation;
// releasing the fonts under them leaves each one calling DrawTextA on freed memory on the very next
// frame. Cleanup() already deletes the numbers before releasing the fonts, which is exactly the order
// needed here -- so this reuses it rather than reimplementing a partial teardown.
// 📌 The visible cost is that numbers on screen when you change the font disappear. That is correct:
// the alternative is a crash, and they were about to fade out anyway.
void FloatingTextSetFont(const char* face, int size)
{
	if (size < 8)  { size = 8; }
	if (size > 96) { size = 96; }

	// Only a face from the list. See kFtFonts -- D3DXCreateFont succeeds on an unknown name and
	// silently substitutes, so an unvalidated face is a wrong typeface with no error.
	const char* chosen = kFtFonts[0];
	if (face) {
		for (int i = 0; i < kFtFontCount; ++i) {
			if (_stricmp(face, kFtFonts[i]) == 0) { chosen = kFtFonts[i]; break; }
		}
	}

	const bool changed = (_stricmp(chosen, g_ftFontFace) != 0) || (size != g_ftFontSize);
	strcpy_s(g_ftFontFace, sizeof(g_ftFontFace), chosen);
	g_ftFontSize = size;

	// ⚠️ Only rebuild if the device is actually up. During FloatingTextLoadSettings it is not -- the
	// settings load runs before the hooks are even installed -- and Initialize() would build fonts
	// against a null device. EndScene builds them on first acquire, by which time these are set.
	if (changed && g_pFtm && g_deviceAcquired) {
		g_pFtm->Cleanup();
		g_pFtm->Initialize();
	}
}

int  FloatingTextFontCount()          { return kFtFontCount; }
const char* FloatingTextFontName(int i) { return (i >= 0 && i < kFtFontCount) ? kFtFonts[i] : ""; }

// FCTHEAL <target_entity_id>|<amount>|<spell_id>|<caster_entity_id>
//
// ⚠️⚠️ THIS IS A CHAT LINE, NOT A PACKET, AND THAT IS NOT A SHORTCUT. Healing has no representation on
// the wire at all -- Mob::HealDamage communicates a heal purely as localised chat text with the amount
// inside a string -- so there is no opcode to read and no struct to cast. The server sends this only to
// clients that announced the dll (see Client::AoTv4WantsFctHeals), which is why it cannot spam anyone.
//
// Returns true when the line was ours and must be swallowed.
static bool FtParseFeed(const char* msg, const char* tag, bool isHeal)
{
	const size_t taglen = strlen(tag);
	if (!msg || strncmp(msg, tag, taglen) != 0) { return false; }

	// ⚠️ Swallowed even when the feature is off or the parse fails. Returning false would let the raw
	// transport text into the player's chat window, which is worse than silently dropping a heal.
	// ⚠️ Healing is gated on its own toggle; damage over time rides the ordinary damage toggles, which
	// FtWantHit applies below.
	if (!areFloatingTextEnabled || !g_pFtm || !g_deviceAcquired) { return true; }
	if (isHeal && !g_ftShowHeals) { return true; }

	// kind: 0 melee, 1 direct spell, 2 damage over time. Absent on an older server, hence the default
	// and the "< 2" rather than "< 5" -- a missing trailing field must not drop the whole number.
	unsigned int tid = 0, amount = 0, spell = 0, cid = 0;
	int kind = 0;
	if (sscanf_s(msg + taglen, "%u|%u|%u|%u|%d", &tid, &amount, &spell, &cid, &kind) < 2) { return true; }

	PSPAWNINFO me = (PSPAWNINFO)pLocalPlayer;
	if (amount == 0) { return true; }

	FloatingTextCount(0);

	// ⚠️ Healing reuses the DAMAGE category toggles, deliberately: "show what happens to me" and "show
	// what happens to others" are the same question whichever direction the health is moving, and a
	// second set of toggles meaning the same thing is how two controls drift out of step.
	if (!FtWantHit((unsigned __int16)tid, (unsigned __int16)cid)) { FloatingTextCount(2); return true; }

	PSPAWNINFO target = (PSPAWNINFO)GetSpawnByID((DWORD)tid);
	if (!target) {
		// ⚠️⚠️ A KILLING BLOW LANDS HERE, AND DROPPING IT LOSES THE MOST INTERESTING NUMBER IN THE FIGHT.
		// The client replaces a dead NPC's spawn with a CORPSE, which is a different entity id, so by the
		// time the transport line is read the target the server named no longer exists. Reported from
		// play as "weapon swings that kill something don't show up".
		// 📌 Falling back to the local player is exact in anchored mode -- the position is not read at
		// all there, the number goes to the fixed spot. In "over the target" mode it is an approximation:
		// the killing blow floats off YOU rather than off a corpse that is no longer where it died. That
		// is visibly wrong-ish and still far better than silently losing every kill.
		target = (PSPAWNINFO)pLocalPlayer;
		if (!target) { FloatingTextCount(3); return true; }
	}

	FloatingTextCount(4);
	// ⚠️⚠️ THE COLOUR IS DECIDED HERE, FROM THE SERVER'S `kind`, NOT FROM THE PACKET'S `type` FIELD.
	// The old test was `act->type == 231`, which never fires: Mob::CommonDamage sets `type` from
	// SkillDamageTypes[] and falls back to hand-to-hand, so a spell is indistinguishable from a swing
	// in that field. That is why spell damage was never coloured as spell damage.
	int colour;
	if (isHeal)                                  { colour = FT_COLOUR_HEAL; }
	else if (me && tid == (unsigned)me->SpawnID) { colour = FT_COLOUR_TAKEN; }
	else if (kind == 1 || kind == 2)             { colour = FT_COLOUR_SPELL; }
	else if (kind == 3)                          { colour = FT_COLOUR_ABILITY; }
	else                                         { colour = FT_COLOUR_MELEE; }

	g_pFtm->AddDamageText(target, (int)amount, (spell > 0 && spell < 0xFFFF) ? (int)spell : 0,
	                      isHeal ? FT_HIT_HEAL : 0, isHeal, colour);
	return true;
}

bool FloatingTextParseHeal(const char* msg)   { return FtParseFeed(msg, "FCTHEAL ", true); }
bool FloatingTextParseDamage(const char* msg) { return FtParseFeed(msg, "FCTDMG ", false); }

// Tell the server whether to send us heals. One line per state change, never per heal.
// ⚠️ Routed through the game-thread command queue like every other command this dll issues -- calling
// InterpretCmd from a render or chat callback runs it on the wrong thread.
// ⚠️⚠️ GATED ON THE FEATURE, NOT ON THE HEALING TOGGLE. It was `enabled && showHeals`, which quietly
// switched the whole feed off -- damage over time and spell numbers included -- for anyone who turned
// healing numbers off, because the server sends all of it down the same opt-in. The healing toggle is
// applied client side, where it belongs.
void FloatingTextAnnounceHeals()
{
	AoTQueueGameCommand(areFloatingTextEnabled ? "/say fctheal 1" : "/say fctheal 0");
}

// ===================================================================== window support
// Cursor position as a fraction of the render target.
// ⚠️⚠️ THE WINDOW HANDLE COMES FROM THE DEVICE, NOT FROM AN MQ2 GLOBAL. `EQADDR_HWND` exists but is a
// raw address into client memory of the kind CLAUDE.md §13 records as unreliable on this build, and it
// would be wrong anyway after an alt-enter. GetCreationParameters returns the window the device we are
// actually drawing into is bound to, which is the only one whose client area matches the viewport.
bool FloatingTextCursorFraction(float* fx, float* fy)
{
	if (!fx || !fy || !g_pDevice) { return false; }

	D3DDEVICE_CREATION_PARAMETERS cp;
	if (g_pDevice->GetCreationParameters(&cp) != D3D_OK || !cp.hFocusWindow) { return false; }

	POINT pt;
	if (!GetCursorPos(&pt))                    { return false; }
	if (!ScreenToClient(cp.hFocusWindow, &pt)) { return false; }

	RECT rc;
	if (!GetClientRect(cp.hFocusWindow, &rc))  { return false; }
	if (rc.right <= rc.left || rc.bottom <= rc.top) { return false; }

	*fx = (float)(pt.x - rc.left) / (float)(rc.right - rc.left);
	*fy = (float)(pt.y - rc.top)  / (float)(rc.bottom - rc.top);

	// ⚠️ Clamped, not rejected. A click a pixel outside the client area is a click the player meant,
	// and refusing it silently would make the Place buttons feel broken near the screen edge.
	if (*fx < 0.02f) { *fx = 0.02f; }  if (*fx > 0.98f) { *fx = 0.98f; }
	if (*fy < 0.02f) { *fy = 0.02f; }  if (*fy > 0.98f) { *fy = 0.98f; }
	return true;
}

// Render target size in pixels.
// ⚠️ Taken from the D3D viewport, not from a UI-manager screen extent -- CXWndManager exposes no such
// member on this build, and the viewport is the surface the numbers are actually positioned against.
bool FloatingTextScreenSize(int* w, int* h)
{
	if (!w || !h || !g_pDevice) { return false; }
	D3DVIEWPORT9 vp;
	if (g_pDevice->GetViewport(&vp) != D3D_OK || vp.Width == 0 || vp.Height == 0) { return false; }
	*w = (int)vp.Width;
	*h = (int)vp.Height;
	return true;
}

// Sample numbers, so the font, spread and fade can be judged from the window without finding
// something to hit. Half damage and half healing, so both anchors show at once in anchored mode.
// ⚠️ Placed on the LOCAL player, because that is the one spawn guaranteed to exist. In "over the
// target" mode that means they appear on you rather than on a mob, which is the honest preview: there
// is no target to preview against.
void FloatingTextTestBurst()
{
	if (!g_pFtm || !g_deviceAcquired) { return; }

	PSPAWNINFO me = (PSPAWNINFO)pLocalPlayer;
	if (!me) { return; }

	// One of each colour, so the burst is a legend rather than a sample: if two of these are hard to
	// tell apart on screen, that is the answer the player is looking for.
	static const int kSample[FT_COLOUR_COUNT]  = { 47, 118, 62, 23, 233, 891 };
	for (int i = 0; i < FT_COLOUR_COUNT; ++i) {
		const bool heal = (i == FT_COLOUR_HEAL);
		g_pFtm->AddDamageText(me, kSample[i], 0, heal ? FT_HIT_HEAL : 0, heal, i);
	}
}

// ===================================================================== anchor edit state
// ⚠️⚠️ THE ANCHORS ARE REAL SIDL WINDOWS NOW, AND THE HAND-DRAWN VERSION IS GONE.
// The first attempt drew two boxes with IDirect3DDevice9::Clear and dragged them by polling
// GetAsyncKeyState(VK_LBUTTON). It worked, and it was unusable: the client never stopped seeing that
// same click, so dragging a box turned the camera at the same time. Nothing this dll can do from a
// render hook consumes an input the client has already been given.
// 📌 A window the UI engine owns is dragged BY the UI engine, which is what makes the click stop
// reaching the world -- and it gets a title bar and a close box for free, so "how do I put these away"
// answers itself. See core_fctwindow.cpp.
static bool g_ftAnchorEdit = false;

void FloatingTextSetAnchorEdit(bool on) { g_ftAnchorEdit = on; }
bool FloatingTextAnchorEditing()        { return g_ftAnchorEdit; }
