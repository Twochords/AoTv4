#pragma once

// core_floatingtext -- floating combat text drawn IN THE WORLD, over the thing you hit.
// =========================================================================================
// Damage numbers rise off the target and fade, coloured by how the hit landed. Ported from
// tunaria/NMS-Release (MIT, (c) 2021 E Spause) -- see src/nms/LICENSE.NMS. The vendored render
// framework is under src/nms/ and is deliberately left as close to upstream as possible so it can be
// re-synced; everything AoTv4-specific lives in this module.
//
// ⚠️⚠️ THIS OVERTURNS A DOCUMENTED "IMPOSSIBLE". CLAUDE.md section 10 recorded that EQ's D3D device
// cannot be hooked here, on the evidence that a device we create shares no vtable with the one EQ
// renders through (measured: `present=0 swap=0` with all vtables patched). That conclusion was wrong,
// and an earlier AoT generation reached it independently and just as wrongly
// (`New folder (2)/AoT_Gui_Master.cpp` does the same thing and fails the same way). Three differences
// separate the working approach from the failed one:
//
//   1. Direct3DCreate9**Ex** + CreateDeviceEx with D3DDEVTYPE_NULLREF and no window, NOT
//      Direct3DCreate9 + CreateDevice with D3DDEVTYPE_HAL and a real HWND.
//   2. BeginScene is vtable index **0x29 (41)**, EndScene is **0x2a (42)**, Reset is **0x10 (16)**.
//      The old attempt hooked index 42 believing it was BeginScene -- it was EndScene, off by one.
//   3. ⚠️⚠️ AND THE ONE THAT ACTUALLY MATTERS: the detour captures the device from its own `this`
//      pointer and draws to THAT. The old attempt kept the throwaway device
//      (`pD3DDevice = pDummyDevice`) and rendered into a NULLREF surface nobody can see, which looks
//      exactly like "the hook never fired".
//
// 📌 So the vtable was never the problem. InstallDetour patches the FUNCTION ADDRESS read out of the
// vtable, and that address is the shared d3d9.dll implementation every device in the process calls --
// EQ's included. Creating a device is only how you find the address; it is not what you render to.
//
// ⚠️ Requires d3dx9 (D3DXFont, D3DXVECTOR3). Headers and import libs are vendored under deps/dx9/
// because the DirectX SDK is long dead and cannot be assumed present on a build machine.
//
// ---------------------------------------------------------------------------------------
// Where the numbers come from
//
// ⚠️⚠️ NOT from CEverQuest::ReportSuccessfulHit -- that address is NOT mapped in our eqgame.h (19
// CEverQuest__ addresses are, this is not one of them; EQClasses.cpp:5570 declares it behind an
// #ifdef that never fires). The damage arrives on the wire instead, as OP_CombatAction, which
// carries source, target, damage, spell id and hit type in one packet.
//
// ⚠️ This dll ALREADY detours CEverQuest::HandleWorldMessage (eqgame.cpp) and passed every opcode
// straight through without looking at it. That is the feed point -- no new detour, and no second
// module competing for an address, which is the rule the rest of this dll follows.

#include <windows.h>

// ⚠⚠ EXTERN, and _options.h is deliberately NOT included from here. That header DEFINES its
// flags rather than declaring them, so pulling it into a second translation unit is a duplicate
// symbol at link time. It is included by core_init.h alone; everyone else takes an extern.
extern bool areFloatingTextEnabled;
extern bool areFloatingTextOpcodeTrace;

// OP_Damage, as the client numbers it internally by the time HandleWorldMessage sees it.
// ⚠⚠ THIS IS THE CLIENT-SIDE OPCODE, NOT THE EQEmu WIRE OPCODE. HandleWorldMessage is called with
// the value the client has already translated to, so it does NOT match the server's opcode table and
// must not be "corrected" against it.
//
// ✅ IDENTIFIED BY SIZE, NOT BY COUNTS, AND THE DIFFERENCE MATTERS. A play session yielded 256
// distinct (opcode, size) pairs, and half a dozen of them climbed at roughly the rate of a swing --
// hate updates, buff ticks and position all look like combat if you only count. What settled it is
// that RoF2's CombatDamage_Struct is exactly 30 bytes and 0x6F15 was the only size-30 opcode in the
// whole table. 📌 The leading candidate on counts alone was 0x004C, which then moved +0 across a
// 40-second melee -- so counting would have picked wrong.
//
// ⚠ If this ever needs re-deriving on another build: turn areFloatingTextOpcodeTrace on, fight,
// type /fttrace, and look for the opcode whose SIZE equals sizeof(rof2 CombatDamage_Struct).
// ⚠ Defined ONCE, here, because both the packet filter in eqgame.cpp and the struct reader in
// core_floatingtext.cpp need it -- a literal in each is how the two silently stop agreeing.
static const unsigned __int16 AOTV4_OP_COMBAT_ACTION = 0x6F15;

// Install the D3D device hooks. Safe to call more than once; it installs at most once.
// ⚠️ Call from runtime, NEVER from DllMain -- creating a D3D device under loader lock is the
// 0xc0000142 startup failure CLAUDE.md section 3 records for window and thread creation.
void FloatingTextInit();

// Tear the manager down. Called from the CleanGameUI / ReloadUI detour that
// core_achievements_native.cpp owns, like every other window module here.
void FloatingTextOnUiReset();

// Feed one combat action. Called from HandleWorldMessage_Detour for OP_CombatAction ONLY; it
// early-outs on every other opcode before reading a byte of the buffer.
// Returns nothing and consumes nothing -- the packet still reaches the client untouched.
void FloatingTextOnCombatAction(const char* buf, size_t size);

// Record one (opcode, size) pair. Logs each DISTINCT pair once, with a running count, so a play
// session yields a short table rather than a packet dump.
// ⚠ Only meaningful with areFloatingTextOpcodeTrace on, and only until the opcode is confirmed.
void FloatingTextTraceOpcode(unsigned __int16 opcode, size_t size);

// Write the collected opcode table to the log. Bound to the /fttrace command so it can be dumped at a
// known moment -- "I hit the mob, then typed this" is what makes the table readable.
void FloatingTextDumpOpcodes();

// Record one candidate packet's raw bytes into a ring of the most recent ones. /fttrace prints the
// ring alongside the opcode table, with every plausible damage field decoded.
// ⚠⚠ MOST RECENT, not first-seen: an opcode's first few payloads are zone-in traffic, and the only
// window where the player also knows what the damage numbers were is the fight they just finished.
// ⚠ Diagnostic only -- it exists to identify the combat opcode and comes out once that is known.
void FloatingTextDumpPayload(unsigned __int16 opcode, const char* buf, size_t size);

// Read <EQ>\aotv4_floatingtext.ini. Called once from FloatingTextInit, before anything can draw.
void FloatingTextLoadSettings();

// Handle "/fct ...". Always returns true -- an unrecognised argument prints the status rather than
// falling through to the client, which would answer "invalid command" for a typo in our own command.
bool FloatingTextCommand(const char* args);

// Bump one of the packet counters. 0 seen, 1 no damage, 2 filtered, 3 no spawn, 4 drawn.
void FloatingTextCount(int which);

// ---------------------------------------------------------------------------------------
// Settings. Declared here rather than behind accessor pairs because the Combat Text window
// (core_fctwindow.cpp) reads and writes every one of them, and twenty get/set pairs would be twenty
// more places for a name and a meaning to drift apart.
// ⚠ Anything that CHANGES one of these must call FloatingTextSaveSettings() -- nothing writes the ini
// on a timer or at shutdown, so an unsaved change is simply lost on the next login.
extern bool  g_ftShowOutgoing;    // damage you deal
extern bool  g_ftShowIncoming;    // damage done to you
extern bool  g_ftShowOthers;      // everyone else
extern bool  g_ftShowHeals;       // healing, which arrives only if the server was asked for it
extern int   g_ftMode;            // 0 over the target, 1 two fixed screen spots
extern float g_ftAnchorDmgX, g_ftAnchorDmgY;
extern float g_ftAnchorHealX, g_ftAnchorHealY;
extern int   g_ftCascade;         // 0 stack, 1 fanned, 2 straight with stagger
extern int   g_ftFanWidth;        // px, widest sideways travel under the fanned cascade
extern int   g_ftRisePixels;
extern int   g_ftDurationMs;
extern float g_ftScalePct;
extern unsigned int g_ftColour[6];   // 0xRRGGBB, indexed by EFtColour
extern int   g_ftLaneGapPx;          // px between numbers that arrive together
extern char  g_ftFontFace[64];
extern int   g_ftFontSize;

void FloatingTextSaveSettings();

// Set the typeface and size, validating the face against the built-in list and rebuilding the fonts.
// ⚠⚠ This DESTROYS every number currently on screen. A live number holds a raw ID3DXFont*, so the
// fonts cannot be released under them; see the note on the definition.
void FloatingTextSetFont(const char* face, int size);
int         FloatingTextFontCount();
const char* FloatingTextFontName(int i);

// Tell the server whether to send FCTHEAL lines. Call after any change to enabled or healing.
void FloatingTextAnnounceHeals();

// Swallow and act on one FCTHEAL transport line. True when the line was ours.
bool FloatingTextParseHeal(const char* msg);

// Throw a few sample numbers so the window's font, spread and fade can be judged without a fight.
void FloatingTextTestBurst();

// Where is the mouse, as a fraction of the render target? False when it cannot be determined.
// 📌 Derived from the D3D device's own focus window rather than from an MQ2 HWND global, so it is
// correct for the surface we actually draw into, windowed or fullscreen.
bool FloatingTextCursorFraction(float* fx, float* fy);

// Render target size in pixels, from the D3D viewport. False if the device is not up.
bool FloatingTextScreenSize(int* w, int* h);

// Swallow and act on one FCTDMG transport line. True when the line was ours.
// 📌 Damage over time only -- melee and direct spell damage both arrive as real OP_Damage packets.
bool FloatingTextParseDamage(const char* msg);

// Whether the two draggable anchor windows are on screen. They are real SIDL windows owned by
// core_fctwindow.cpp; this is just the shared flag.
void FloatingTextSetAnchorEdit(bool on);
bool FloatingTextAnchorEditing();
