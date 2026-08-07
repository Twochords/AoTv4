# AoTv4 — All-Classes Random-Progression EQ Server

EQEmu server + RoF2 client mod. The headline feature is a **level-up reward window**: on each
level you're offered **3 random rewards** (spells or class-specific combat abilities) and pick
one, which the server scribes/trains. The window is drawn by a client-side `dinput8.dll`
so it works on a **vanilla RoF2 client — no MacroQuest/E3 required**.

> **⚠️ ALL CLASSES (was Bard-only).** Originally every character was forced to Bard; as of **2026-07**
> the server is opened to **all 16 classes**. The four pure-melee classes (Warrior/Monk/Rogue/Berserker)
> are turned into casters **client-side** by the dll (`core_allcasters`, §14) + **server-side** mana
> (`CalcMaxMana`). The reward-pool spells are opened to every class, and the reward abilities cast as
> **spells not songs** even for a Bard (client `IsBardSong` detour, §14). Sections below that still say
> "everyone is a Bard" are **historical** — Bard is now just one of the 16. Existing pre-pivot characters
> remain Bard (a valid class; no forced reclass).

---

## 0. Version control — THREE separate git repos, and the parent ignores two of them

`/src` is its own repo, but **`.devcontainer/.gitignore` has `repo/`**, so the two most valuable trees
are NOT in it — they are independent nested clones with their own remotes and branches:

| Tree | Repo / remote | Branch holding AoTv4 work |
|---|---|---|
| `/src` (server C++, SQL, docs) | this repo | `aotv4-spell-rebuild` |
| `.devcontainer/repo/eq-core-dll` (the **dll**) | `Twochords/Dinput` | `aotv4-advloot-module` |
| `.devcontainer/repo/quests` (**all Lua**) | clone of `ProjectEQ/projecteqquests` | `aotv4` |

⚠️ **`git status` in `/src` tells you NOTHING about the dll or the Lua.** As of 2026-07-25 every AoTv4
Lua module (`spell_choice`, `aa_choice`, `aa_pool`, `era_system`, `pok_travel`, `bazaar_broker`, the
43xxx spell scripts…) had **never been committed anywhere**, and the dll had nothing since the
`tradeskill window` commit. Both now have a first recovery point on the branches above (local commits,
not pushed). **`cd` into each repo and check it separately before assuming work is safe.**

## 1. Environment & layout

- `/src` (dev container) == `C:\AoTv3\AoTv4` (Windows, 9p mount). The EQ **client** is a
  separate RoF2 install on Windows; the dev container holds the **server + sources**.
- Server build/run dir: `/src/build/bin` (config symlinked from `.devcontainer/override/`).
- **Server C++ source:** `/src/zone`, `/src/world`, `/src/common`, … (rebuildable; see §8).
- Quests / Lua: `/src/.devcontainer/repo/quests/` (symlinked into `build/bin/quests`).
  - `quests/global/global_player.lua` — global player event hooks.
  - `quests/lua_modules/` — `spell_choice.lua` (core logic), `spell_pool.lua` (gen),
    `spell_icons.lua` (gen), `skill_pool.lua` (combat-ability rewards), `spell_blacklist.lua` (gen).
- Client mod source: `/src/.devcontainer/repo/eq-core-dll/` (builds `dinput8.dll`).
  - `core_spellwindow.cpp` — the spell window, the **AA window** (§6), the **Portal window** (§11),
    **and** the skill-unlock hook; all share the `dsp_chat` + `ProcessGameEvents` detours.
  - `core_skillunlock.h` — declares `EnableSkillUnlock()` (impl is in `core_spellwindow.cpp`).
  - `core_allcasters.cpp` / `core_allcasters.h` — the **all-classes-as-casters** patch (§14). Its OWN
    translation unit + OWN dedicated detours; NOT piggy-backed on the spell-window / tradeskill hooks.
  - `_options.h` — feature flags; `core_init.h` — wires them in `InitOptions()`.

## 2. Running the server

The stack is 5 processes, started from `/src/build/bin`:

```bash
cd /src/build/bin
nohup ./loginserver  > logs/loginserver_manual.log 2>&1 &
nohup ./world        > logs/world_manual.log       2>&1 &
nohup ./eqlaunch zone> logs/eqlaunch_manual.log     2>&1 &   # zone launcher (boots zones on demand)
nohup ./ucs          > logs/ucs_manual.log          2>&1 &   # chat/mail/bazaar-trader search
nohup ./queryserv    > logs/queryserv_manual.log    2>&1 &   # logging
```

- **"Server down" but you reach char select:** `eqlaunch` isn't running → world logs
  `No zoneserver available to boot up`. Restart `eqlaunch zone`. Keep **exactly one** eqlaunch.
- Zones are **dynamic**: `eqlaunch` boots `./zone` processes on demand and respawns them; idle
  zones self-terminate. That's normal, not a crash.

### RoF2 login needs 5999/udp PUBLISHED (client "times out at username/password")
The RoF2 client logs in on the **SoD stream** (`login.json` `sod_port: 5999`), NOT the Titanium
port 5998. The dev container reaches the Windows client only through `devcontainer.json`
**`appPort`**, which therefore **MUST include `"5999:5999/udp"`** (alongside 5998 world-link,
7000-7005 zones, 9000 world). If it's missing, the loginserver runs fine and world connects, but
the client's login packets never reach the container → **timeout at the username/password screen**
with **zero client lines in `logs/loginserver_manual.log`** (that empty log is the tell). Diagnose:
`(ss -lunp | grep :5999)` shows the loginserver bound locally, but the port isn't forwarded to the
host. Fix = `appPort` entry + a container rebuild. Client `eqhost.txt` stays `Host=127.0.0.1:5999`.

### The DB and the login port both survive rebuilds now (persistent volume)
A container rebuild used to wipe **both** the `peq` database (datadir was on the ephemeral overlay)
and the 5999 port (missing from `appPort`) — the recurring "server won't come up after a rebuild"
pain. Both are fixed in `devcontainer.json`: `appPort` includes `5999:5999/udp`, and the MariaDB
datadir is a **named volume** (`source=aotv4-mysql-data,target=/var/lib/mysql,type=volume`) that
**persists across rebuilds**. See `POST_REBUILD_RECOVERY.md`. The canonical DB snapshot lives at
**`/src/aotv4_current.sql`** (full `peq` with all migrations baked in) — needed only for the ONE
import after the volume is first created, or to seed a brand-new volume; routine rebuilds keep the DB.

## 3. The level-up reward window

No opcode patching — communication rides on **chat**:

```
LEVEL UP
  server (Lua) ──► client : "SPELLCHOICEDATA name1|icon1^name2|icon2^name3|icon3"
                            (server keeps the real ids/types in a data bucket, keyed by char)
  dinput8.dll  ── swallows that line, shows a layered GDI window with 3 rows + icons
  player clicks "Select N"
  dinput8.dll  ──► server : "/say spellpick N"     (this echo is also swallowed)
  server (Lua) ── event_say validates N against the stored bucket, then scribes a spell /
                  trains a discipline / raises a combat skill
```

The **id list is server-side**; the player only sends an **index 1-3**, so a modified client
can't grant an arbitrary reward — only one of the three it was actually offered.

### Server side (Lua)
- `global_player.lua`: `event_connect` sends `SKILLUNLOCKDATA` (see §4) — the old **Bard-force is REMOVED**
  (all classes now, §14); `event_level_up` auto-grants non-combat skills then calls `spell_choice.offer(e)`;
  `event_say` calls `spell_choice.handle_say(e)`.
- `spell_choice.lua` — builds the 3 choices (spells + ~1 combat skill), emits `SPELLCHOICEDATA`,
  validates picks. Stored bucket tokens are **typed**: `S:<spellid>` (spell/disc) or
  `K:<skillid>` (combat skill). Tunables at top: `CHOICE_COUNT`, `LEVEL_BAND`, `SAY_TRIGGER`,
  `SKILL_OFFER_CHANCE` (**0.125** -- roughly one level in eight; it sat at **0.0** for a while, meaning
  combat abilities were never offered at all, while this line still claimed 0.7), `SKILL_GRANT_PCT` (0.25).
- `spell_pool.lua` (**generated**, see §5), `spell_icons.lua` (**generated**),
  `spell_blacklist.lua` (**generated**), `skill_pool.lua` (hand-curated; see §4).

### Reroll — pay coin for three different rewards — 2026-07-28
Do not like the three offered? Press **Reroll** under the description pane on the Choose tab and pay.
Price box sits beside the button. Pure Lua + the picker XML; **no item and no vendor NPC**.
`spell_choice.M.reroll` / `.reroll_cost` / `.send_reroll_cost`. Protocol: `/say spellreroll` out,
**`SPELLREROLLCOST <copper>`** back (both swallowed by the dll).
- **Price = `5p × (rerolls_bought + 1)`** — 5p, 10p, 15p, … Counted per character in bucket
  `rerollbuy_<charid>` and ⚠️ **never reset by the roguelite death**: rerolls get dearer over a
  character's whole life, not per run.
- ⚠️⚠️ **A TOKEN ITEM + VENDOR WAS BUILT FIRST AND THEN DELETED** (item 147495 "Destiny Fragment",
  npc 2000210). Charging coin straight from the button is simpler and needs no shared-memory rebuild,
  no spawn rows and no hand-in script. If a token is ever wanted again, the reason the vendor could
  not be a `merchantlist` merchant still stands: **merchantlist prices are STATIC** — no per-character
  column, no hook — so an escalating price cannot be expressed there at all.
- ⚠️⚠️ **Gate ORDER in `M.reroll` is load-bearing**: pending offer → funds → **build** → *then* charge.
  `TakeMoneyFromPP` has no refund path, so charging before building would bill for a reroll that turns
  out to have nothing left to offer.
- ⚠️ **It REPLACES the queue's front offer, never appends** — the front is what the window shows, so
  appending would leave the disliked set still pickable and the player would have paid for nothing.
- ⚠️ `GetCarriedMoney` is **carried** coin only, not bank. The refusal message says so, because
  "you cannot afford it" while standing on 500p in the bank reads as a bug.
- ⚠️ `build_offer()` is shared by `M.offer` and `M.reroll` deliberately: two copies would drift on the
  skill chance, level band and shuffle. Reroll bumps the same `spell_seed_` counter, which is what
  makes it produce a *different* set rather than re-deriving the one you paid to remove.
- ⚠️ The dll validates **nothing** — `g_rerollCost` is a DISPLAY value. Pricing, the affordability
  check and taking the coin are all server-side, so a modified client cannot reroll for free.
- ⚠️ The cost box is an **STMLbox, not a Label** — Label has no confirmed border on this build, and the
  price must arrive from the server (it is per character) rather than being hardcoded client-side.
- ⚠️ In `gen_choice_xml.pl` the detail box shrank 174 → **140** to make room; it could not move *up*
  because the three cards occupy 34..208 and `DETNAME_Y` sits directly under them.

### Client side (`core_spellwindow.cpp` → `dinput8.dll`)
Gated by `areSpellChoiceWindowEnabled` (`_options.h`). RoF2, image base `0x400000`, addresses
rebased at runtime:
- **Chat hook** — detour `CEverQuest::dsp_chat` (`0x51F1A0`): swallow `SPELLCHOICEDATA`,
  `SKILLUNLOCKDATA`, and the `spellpick` echo; parse `name|icon`.
- **Per-frame heartbeat** — detour `ProcessGameEvents` (`0x539E60`, main thread): spawn the
  overlay thread once, and run the pick **on the game thread** via `CEverQuest::InterpretCmd`
  (`0x51FCE0`) → `/say spellpick N`.
- **Window** — a separate **layered, top-most GDI window** on its own thread. We do NOT draw
  through EQ's D3D device: the RoF2 client renders via EQGraphicsDX9.dll's own device, which
  shares no vtable with any device we can create, so Present/swap-chain hooks never fire
  (proven empirically: `present=0 swap=0` with all vtables patched). Works in **windowed mode**.
  Draggable by the title strip; NOT `WS_EX_NOACTIVATE` (a click takes focus so EQ stops
  mouse-look → dragging doesn't rotate the camera); shown only while EQ or the overlay itself
  is the **foreground** window (so it never floats over other apps).
- **Icons** — decode `uifiles/default/spellsNN.tga` (own TGA loader: type 2 + RLE 10, 24/32-bit).
  Mapping: `sheet = icon/36 + 1`, `cell = icon%36`, columns = `sheetWidth/40`, cell 40×40.

### The picker is now a NATIVE SIDL window (2026-07-25) — `core_spellchoice_native.cpp`
The GDI overlay above is superseded (it stays in the dll but is never shown). Own TU, no detours of
its own; our `dsp_chat` calls `SpellChoiceParseTransport`. Wire format is **unchanged**
(`SPELLCHOICEDATA` / `SPELLDESCDATA` / `/say spellpick N`) so the server needed no edits.
- **Layout = three "cards"** (icon, name, one-line summary, button) over a shared STML description
  pane — NOT a listbox: a listbox can't carry a per-row icon here, and its column header + red
  selection bar looked nothing like EQ. Two-step commit (Select previews → Confirm learns), because
  a mis-clicked level reward can't be undone.
- ⚠️ **Per-row icons in a SIDL window — the trick.** This build maps **no** runtime icon API:
  `CTextureAnimation::SetCurCell`, `CSidlManager::FindAnimation` and `CListWnd::SetItemIcon` are all
  absent from `eqgame.h` (only `GetItemIcon` exists), so a texture cell **cannot be chosen from
  code**. Instead the choice moves into the XML: one `Ui2DAnimation` per icon (already pointing at
  the right cell of the right `SpellsNN.tga`) + **one hidden Button per (row, icon)** carrying an
  **inline `<ButtonDrawTemplate>`** (the form stock `EQUI_Inventory.xml` uses for its icon buttons);
  the dll just `Show()`s the matching one, and `CXWnd::Show` **is** mapped.
  Generated by **`aotv4_client_install/gen_choice_xml.pl`**, which **reads the icon set out of
  `spell_icons.lua` + `skill_pool.lua`** rather than hardcoding it → `EQUI_AoTSpellChoiceWnd.xml`
  (~8600 lines, 154 icons, 471 pieces). **Rerun it whenever the pool is regenerated.**
  The dll keeps **no** copy of the icon list: it looks the button up by name at runtime
  (`ASC_Icon<row>_<icon>`), so the XML is the single source of truth and an undefined icon just
  draws nothing. (An earlier `kIcons[]` array had to match the generator exactly and drifted the
  moment the pool changed from 22 icons to 154.) Same trick applies to any window wanting icons.
- ⚠️ **Only `Spells01..Spells07` are declared** in the client's `EQUI_Animations.xml`, so icon
  indices **above 251** have no texture and are skipped by the generator (1 spell in the stock pool).
- ⚠️⚠️ **Texture names in SIDL are CASE SENSITIVE, and `/src/EQUI_Animations.xml` is the authority.**
  A `<Texture>` is resolved against the texture registry built from that file's ~588 `<TextureInfo>`
  entries — **not** against the filesystem. The spell sheets are declared **`Spells01.tga`** (capital
  S, 256×256, `A_SpellIcons` is the stock `Grid` animation over them with `CellWidth/CellHeight` 40).
  Writing `spells01.tga` resolves to **nothing**: the button is created, found and shown, but draws
  an empty hole — **no `UIErrorLog.txt` entry, no error anywhere**. This cost a full round trip; it
  is invisible to every check we have, so grep `EQUI_Animations.xml` for the exact spelling before
  referencing any texture. (Our GDI loaders get away with lowercase because they open the file
  through Windows, which is not case sensitive — so a name that works there can still fail in SIDL.)
- ⚠️⚠️ **Texture names in SIDL are CASE SENSITIVE, and `/src/EQUI_Animations.xml` is the authority.**
  A `<Texture>` is resolved against the texture registry built from that file's ~588 `<TextureInfo>`
  entries — **not** against the filesystem. The spell sheets are declared **`Spells01.tga`** (capital
  S, 256×256; `A_SpellIcons` is the stock `Grid` animation over them with `CellWidth/CellHeight` 40).
  Writing `spells01.tga` resolves to **nothing**: the button is created, found and shown, but draws
  an empty hole — **no `UIErrorLog.txt` entry, no error anywhere**. It is invisible to every check we
  have, so grep `EQUI_Animations.xml` for the exact spelling before referencing any texture. (Our GDI
  loaders get away with lowercase because they open the file through Windows, which is not case
  sensitive — a name that works there can still fail in SIDL.)
- Verify a SIDL tag exists before using it: `strings -n 3 eqgame.exe | grep -qx "<Tag>"`
  (`Ui2DAnimation`/`Frames`/`Texture`/`Hotspot`/`Cycle`/`Duration`/`PressedFlyby` all pass;
  `Cell` does not). Then `aotv4_client_install/validate_ui_xml.sh` before copying.
- **Ctrl+Q**: opens the native picker when a reward is pending, else the old Reward Journal on its
  **Lost** tab — the journal's Spell and AA tabs are dead (this module owns `SPELLCHOICEDATA`, and
  random AA is retired: `areAAChoiceWindowEnabled=false`, the `AACHOICEDATA` line is parsed and
  swallowed but shows nothing).
- ✅ **The "You Lost" death window is now a native SIDL window** (2026-07-27) —
  `core_lostwindow.cpp/.h` + `EQUI_AoTLostWnd.xml`, its OWN translation unit like `core_advloot`.
  Installs no detours; our `dsp_chat` calls `LostParseTransport` (swallows `LOSTDATA`), Ctrl+Q calls
  `LostWindowShow`, and the `CleanGameUI`/`ReloadUI` detour that `core_achievements_native.cpp` owns
  calls `LostWindowOnUiReset`. **Wire format unchanged** (`LOSTDATA name^name^…`) so the server was
  not touched. Scrolling, dragging and the 45-second idle timeout are all gone — the UI engine does
  the first two and a real window with a close box does not need the third, so the list now stays
  until dismissed. The GDI `PaintLostOverlay`/`LostOverlayWndProc`/`LostOverlayThreadProc` remain in
  `core_spellwindow.cpp` but are **unreachable** (`if (false && …)`); delete once confirmed working.
  Install: `aotv4_client_install/LOST_WINDOW_INSTALL.md`.
  - ✅ **It is the "Death Book" and the dates EXPAND** (2026-07-28). One header row per death —
    `[+]`/`[-]`, date, killer, item count — and only an open one lists its items, so 400 entries read
    as a table of contents rather than a wall of names. The newest death starts open.
    ⚠️ **`LostBuildGroups` resets the open/closed state**, so it is called only where the DATA is
    replaced (`LOSTDATA`, and the LAST `LOSTLOG` chunk), never from `Refresh()` — and never on a
    partial log, which would split one death across two chunks into two dates.
    ⚠️⚠️ **The toggle is DEFERRED to `LostWindowTick`, not done in the click handler.** Toggling calls
    `Refresh` → `DeleteAll`, which would destroy the listbox's rows from inside the listbox's own
    notification while the client is still walking them. ⚠️ The selection is also read *after*
    `CSidlScreenWnd::WndNotification` has run — inside it, `GetCurSel` is still one click behind.
    ⚠️ Row index is NOT a data index (headers interleave, a closed death hides many rows) — every
    click goes through the explicit `g_rowGroup[]` map.
- ✅ **The Search window is now a native SIDL window and is called "Allaclone"** (2026-07-28) —
  `core_allaclone.cpp/.h` + `EQUI_AoTAllacloneWnd.xml`, its OWN translation unit like `core_advloot`.
  Installs no detours; our `dsp_chat` calls `AllacloneParseTransport` (swallows `SRCHDATA`/`SRCHDET`)
  and `AllacloneIsOurEcho`, `ProcessGameEvents` calls `AllacloneTick`, and the `CleanGameUI`/
  `ReloadUI` detour that `core_achievements_native.cpp` owns calls `AllacloneOnUiReset`.
  **Wire format unchanged** (`/say srch <kind> <term>`, `/say srchdet <kind> <id>`) so the server was
  not touched at all — `Client::SearchList`/`SearchDetail` are as they were.
  Command is **`/allaclone`**, with `/search` and `/find` kept as aliases; the AoT menu button is
  relabelled. `EnableSearchWindow()`/`ShowSearchWindow()` in `core_spellwindow.cpp` are now
  **forwarders** so the switch is one place instead of a rename through every caller, and
  `g_searchEnabled` stays false, which makes the old overlay's thread/paint/chat handler unreachable
  without deleting them yet.
  - ⚠️ **NO `TabBox` here, deliberately.** Native tabs are the right widget when each page owns its
    contents; the kinds (item/npc/spell/recipe, later quest/tracked/zxp) share ONE search box, ONE
    list and ONE detail pane, so tabs would mean a copy of each per kind. They are **mode buttons**,
    and the active one is
    shown by rewriting its LABEL (`> Items <`) because no latched button state can be set reliably
    from code on this build.
  - ⚠️ **Enter is POLLED, not hooked.** An `Editbox` gives no usable "text committed" notification
    here, so `AllacloneTick` watches the box and searches once the text settles (450 ms) — the same
    approach the AdvLoot filter box already uses. ⚠️ An empty box must still be RECORDED as the last
    searched term or the poll re-fires every frame forever.
  - ⚠️ `SRCHDET` lines are escaped into STML (`<` `>` `&`) before display — an item name containing
    `<` would otherwise swallow the rest of the panel.
  - ✅ **A seventh mode, "Zone XP" (`zxp`), is a BROWSE list rather than a search** (2026-08-07,
    migration **v33**). It answers "where should I be hunting", by region: region headers interleaved
    with zones and their level ranges. Server side is `Client::SearchList` in `zone/trading.cpp`,
    which builds its own string instead of falling through to the generic loop; client side is
    `AC_KIND_ZONEXP` in `core_allaclone.cpp` plus the `ACW_KindZoneXp` button.
    - ⚠️⚠️ **IT IS THE ONLY KIND THAT IS MEANINGFUL WITH AN EMPTY TERM**, so it deliberately skips the
      "type something first" guard and asks for the unfiltered list. Typing still works and filters by
      zone name. ⚠️ It sends **`/say srch zxp *`**, not an empty argument — the say command is split on
      whitespace, so a trailing nothing arrives as a *missing* parameter. The server sanitizes the term
      to alphanumerics, so `*` reduces to empty and matches all.
    - ⚠️⚠️ **EVERY ROW CARRIES id 0, HEADERS AND ZONES ALIKE**, because nothing in this list opens a
      detail page. `OpenDetail` must refuse in this mode — `srchdet zxp 0` is a lookup the server does
      not implement, so it would answer nothing and the detail pane would sit on whatever the previous
      kind left there. Same "a row index is not a data index" trap as the Death Book and the spell
      Known/Pool tabs.
    - ⚠️⚠️ **THE LEVEL RANGES ARE AUTHORED, NOT DERIVED, AND LIVE IN `aotv4_zone_xp`** so they can be
      edited without a rebuild. A computed range was built first (percentile 10-90 of spawned mob
      levels, excluding merchants, bankers, GM trainers, LDoN chests and anything on a town faction)
      and it disagreed with the owner's list substantially — Blackburrow computed 4-8 against an
      authored 2-15, Cabilis West computed 1-1 against 30-35. **Neither is wrong**: the computed figure
      is "where the bulk of the spawns sit", the authored one is "what you might meet". The authored
      list ships. 📌 It is **not derivable from this database at all** — its upper bound is sometimes
      *above* our raw maximum (Misty Thicket 25 against a real 14) and sometimes far below it
      (Crushbone 14 against 65), so no percentile reproduces it and there is no formula to extend.
      Butcherblock (68) and Kaesora (88) were absent from the owner's list and *were* derived here.
    - ⚠️⚠️ **THE PRIMARY KEY IS `zone_id` ALONE, WHICH IS WHAT PINS A ZONE TO ONE REGION.**
      `zone_regions` is many-to-many — The Swamp of No Hope (83) is legitimately reachable from **both**
      Firiona Vie and Cabilis — so a naive join lists it twice under two headers. The single-column key
      makes that a **data** decision instead of something the query keeps re-deciding. Any future zone
      in two regions must likewise pick one.
    - ⚠️ **`label` overrides the numeric range**: the ten city zones carry `'City'`. Once civic NPCs are
      filtered out a city has almost nothing huntable left (North Freeport: **five** spawns), so a
      percentile there is noise dressed as data. Cities also sort **last** within their region, because
      a level-ordered list has nowhere sensible to put one.
    - ⚠️⚠️ **DELVE ZONES ARE DELIBERATELY ABSENT and must stay so.** Region 0 "Always Available" holds
      40 zones, 39 of them the LDoN/DoN delve maps unlocked by v27/v31; delve creatures are **scaled to
      the player** (§24), so a fixed range would be actively misleading. Nothing in the query filters
      them — the table simply does not seed them. **Do not "fix" that by adding region 0.**
  - Install: `aotv4_client_install/ALLACLONE_WINDOW_INSTALL.md`.
- 📌 **TODO — the Portal window is still a GDI overlay** and should get the same treatment for the
  same reasons: self-drawn chrome never matches EQ, it can't scale with the UI, and it only works
  windowed. Client-side-only change; its wire format can stay.
- ⚠️ **A `<Button>` uses `<Template>BDT_Normal</Template>`, NOT `<ButtonDrawTemplate>`.** The tag
  name differs from the inline form used for icon buttons, and only the ~34 `BDT_*` names actually
  defined in `EQUI_Templates.xml` resolve — anything else draws an empty hole with no error, the
  same failure mode as a mis-cased texture name.
- **Client install:** copy `EQUI_AoTSpellChoiceWnd.xml` to `uifiles/default/` **and**
  `<Include>EQUI_AoTSpellChoiceWnd.xml</Include>` in `EQUI.xml`. Missing either = `CCustomWnd` can't
  find its screen and returns **silently** — no error, no window.

> **Loader-lock:** the `Enable*` functions run inside `DllMain` (DLL_PROCESS_ATTACH). Only
> **memory-patch detours** (`EzDetour`) are safe there. Window/D3D/thread creation must be
> deferred to runtime (first chat line / first `ProcessGameEvents`), or startup hard-fails
> (`0xc0000142`).

## 4. Combat skills & cross-class abilities

Goal: a Bard inherits **other classes' skills/abilities**. Three layers had to agree.

**(a) Reward-gating (Lua).** `skill_pool.lua` is the rotation pool — **only class-specific
activated abilities** (Backstab, Bash, Kick, Disarm, Taunt, Berserking, Frenzy, and the monk
strikes Dragon Punch/Eagle Strike/Flying/Round Kick/Tiger Claw). Passive skills (weapon skills,
Offense/Defense, Dodge/Parry/Riposte/Block, Double/Triple Attack, Dual Wield, Intimidation) are
**auto-learned** via `global_player.lua` `free_skills` and never rotate. A picked skill is set
to `SKILL_GRANT_PCT` of its cap.

**(b) Client unlock hook (`core_spellwindow.cpp`, `areSkillsUnlocked`).** The RoF2 client hides
skills a class isn't coded for. We detour `CSkillMgr::GetSkillCap` (`0x5BAEC0`,
args `pChar, level, class, skill, …`) and **only reveal hidden skills** — a native skill
(cap > 0) passes through untouched (so weapon skills are never removed); real skill ids `0..76`
that the class can't natively have get a level-scaled cap (unused slots `77+` stay 0, no "None"
rows). Reward-gated *combat* skills are revealed **only if earned** — the server sends
`SKILLUNLOCKDATA <csv of earned skill ids>` (skills with value > 0) on connect and after each
pick; the hook hides un-earned combat skills from both the Skills window and the ability picker.

**(c) Server execution (`/src/zone/special_attacks.cpp`, `Client::OPCombatAbility`).** EQEmu
class-gates the activated specials. Patched to be **skill-gated** (any class that trained the
skill): Kick (added `GetSkill(SkillKick) > 0` to `can_use_kick`), the monk strikes (dispatch if
`is_monk_special`, not just `class == Monk`), Backstab (`|| GetSkill(SkillBackstab) > 0`).
Disarm/Taunt/Frenzy were already skill-gated; Bash needs a shield. `MonkSpecialAttack` has no
internal class checks (damage = `GetBaseSkillDamage`), so it's safe cross-class. **Requires a
zone rebuild** (§8).

## 5. Spell pool & blacklist (era / junk filtering)

> **2026-07-26: the pool is back to the STOCK spell set.** The 113-spell custom set (43000-43149)
> briefly replaced it and proved mostly redundant with native spells. Regenerate with
> **`perl .devcontainer/custom/spells/gen_stock_pool.pl`**, which writes all three generated modules
> (`spell_pool.lua` 2605 spells over 78 levels, `spell_icons.lua`, `spell_desc.lua`) from
> `id < 10000` + the **20 unique customs** that are kept — the Lua-driven abilities (every effect
> slot 254; behaviour lives in `quests/global/spells/43xxx.lua`, `aotv4_summon_table`,
> `aotv4_reactions`). The other 93 customs stay in the DB but are **never offered**; full restore of
> all 202 rows is `custom/sql/aotv4_custom_spells_backup.sql`, and the superseded generated modules
> are in `custom/spells/old_custom_pool/`. **Rerun `aotv4_client_install/gen_choice_xml.pl` after
> any pool regen** so the picker's icon set matches (§3). Zone restart to reload the Lua modules.
> ⚠️ `%` in a description is spelled out by the generator: `Client::Message` is printf-style, so a
> stray `%` is eaten as a format token.
>
> **Adding new custom spells to the pool: use band `43300-43349`** (helpers/triggers at **43350+**,
> which are never offered). The generator pulls that band wholesale, so unlike the 43000-43149
> customs they may use effect slots. Two lines exist so far, both filling the empty low half of a
> native SPA 323 "add defensive proc" line and both cloned **via a temp table** so all ~236 columns
> stay byte-identical to stock (never hand-list the columns):
>   - `custom/sql/aotv4_reptile_line.sql` — Druid *Skin of the Reptile* (stock 8008/8009, level 68),
>     heal-on-being-hit. Six tiers **8/18/28/38/48/58** = ids **43300-43305** / triggers 43350-43355.
>   - `custom/sql/aotv4_sloth_line.sql` — Shaman *Lingering Sloth* (stock 8015/8016, level 68, the
>     line's **only** stock member), retaliation-slow. Ids **43306-43311** / triggers 43356-43361.
>   - `custom/sql/aotv4_moonfire_line.sql` — a **lifetap weighted 25% damage / 75% heal**, ids
>     **43312-43317**. Damage ≈ half the native lifetap for the level, healing 3× the damage.
>     ⚠️⚠️ **The 3× split cannot be expressed in the DB.** EQEmu hardcodes the tap ratio —
>     `int64 healed = damage;` (`zone/attack.cpp:4287`) — and `spells_new` has no ratio column. So
>     the rows are **real taps** (`targettype 13` ST_Tap, what `IsLifetapSpell` keys on,
>     `common/spdat.cpp:108`) and the engine pays the first 1×, while
>     `quests/global/spells/433xx.lua` + `lua_modules/aotv4_moonfire.lua` add the remaining 2×.
>     **Change a damage value in the SQL and you MUST change that tier script's bonus (= 2× damage).**
>     Staying a genuine tap matters beyond the heal: `IsLifetapSpell` excludes it from
>     `IsDamageSpell`/`IsAnyDamageSpell`/`IsDamageOverTimeSpell` and drives the tap emote.
>     ⚠️ No tier is named "Moonfire": stock **2877 Moonfire** (Druid 60 nuke) is already in the pool
>     and would be a real duplicate.
>     ⚠️⚠️ **This line's Lua bonus never actually ran until 2026-07-29** — `EVENT_SPELL_EFFECT_NPC`
>     had no argument builder, so `e.caster_id` was nil against any monster and `tap_bonus` returned
>     early every time. Moonfire was a plain 1:1 engine-paid lifetap for its whole life. See §10.
>   - `custom/sql/aotv4_promised_line.sql` — **delayed "bloom" heals**, ids **43324-43329** / blooms
>     43368-43373. Cast it, wait 3 ticks, it heals when the buff expires naturally. Uses the native
>     **SPA 289 `CastOnFadeEffect`** (`common/spdat.h:1352`) — no Lua needed, unlike 43017 Languid
>     Healing which predates knowing about it. Cloned from stock 9755/9758 (*Promised Renewal*).
>     ⚠️ The native Promised family is effectively **unreachable**: it starts at level **73** and
>     every tier above 9755 is **id ≥ 10000**, which the pool's `id < 10000` filter excludes.
>     ⚠️ Keep the **SPA 44 + SPA 148 pair**: slot 2 (SPA 44) carries the heal amount as a *stacking
>     marker* and slot 3 (`StackingCommand_Block`, base 44) reads it, which is how Promised heals
>     refuse to overwrite a stronger one. Slot 2's base must equal the real heal or the tiers stack.
>     All six share `spellgroup` 43324 with ascending `rank` so a higher tier replaces a lower.
>   - `custom/sql/aotv4_kindred_line.sql` — **melee proc that heals YOUR GROUP**, ids **43330-43335**
>     / procs 43374-43379. Self buff with **SPA 85 `WeaponProc`**; on a swing the proc heals the
>     whole group. Modelled field-for-field on native **`1976 Blessing of the Blackstar`** (proc of
>     31348 Blackstar, Mace of Night): SPA 0 positive base, `targettype 41`, `goodEffect 1`,
>     range 100, no duration, `spell_category -99`. `39661 Soothing Wave` (+550) is the same shape.
>     ⚠️ **Hopebringer was NOT a guide** — its proc `3606 Light of Marr` shipped as a **stub**
>     (`targettype 5`, `goodEffect 0`, SPA 0 base **−1** = one point of damage; recourse 4119 empty).
>     **Fixed by `custom/sql/aotv4_light_of_marr_fix.sql`** → a 300-point group heal on the 1976
>     pattern, calibrated between Blackstar (100, classic) and Sword of the Sanative (550, lvl 100).
>     That spell is shared by **6 items** — Hopebringer, Bloodstone Blade of the Zun'Muram, and the
>     Hallowed/Mythic tier of each — so one edit repairs all six. Item proc, never pool-offered, so
>     no pool regen needed; still needs a shared-memory rebuild.
>     The engine routes it: `Mob::ExecWeaponProc`
>     (`zone/mob.cpp:5358`) casts a **beneficial** proc on `this` (the swinger) and a detrimental one
>     on the victim, then `SpellFinished` resolves `case ST_Group` (`zone/spells.cpp:2162`).
>     ⚠️ Two fields on the PROC spell are load-bearing: **`goodEffect=1`** (picks the swinger branch
>     — set 0 and it heals the mob you just hit) and **`targettype=41`** (ST_Group — without it only
>     the swinger is healed). Same trick works for a **defensive** proc (SPA 323) since
>     `TryDefensiveProc` calls the same function. Nearest native precedent: 6191 Aura of Runes, whose
>     proc is beneficial but self-targeted.
>   - `custom/sql/aotv4_mark_line.sql` — **reverse damage shield**, ids **43336-43341**, no helper
>     rows (SPA 121 is self-contained). Cloned from stock `2507 Mark of Retribution` (Cleric **54**,
>     the line's floor; then 65/69/74). ⚠️ **It is a DEBUFF cast on the ENEMY** (`goodEffect 0`) —
>     the "Mark of" naming reads like a blessing but the marked creature wounds *itself* on every
>     melee hit it lands. ⚠️ **SPA 121's base must be NEGATIVE**: `Mob::DamageShield` reads it off
>     the *attacker's* bonuses and the guard is `if (rev_ds < 0)` (`zone/attack.cpp:3476`, `:3591`),
>     so a positive base silently does nothing.
>   - `custom/sql/aotv4_thirst_line.sql` — **flat heal per melee hit**, ids **43342-43347**.
>     ⚠️ **It is a SELF BUFF; a "debuff that heals whoever hits it" is NOT buildable.** The DS family
>     covers damage both ways (59 = buff on defender hurts attacker, 121 = debuff on attacker hurts
>     attacker) but has **no healing counterpart in either direction**, and `Mob::MeleeLifeTap`
>     (`zone/mob.cpp:6852`) reads the bonus off the *attacker* and heals the attacker.
>     ⚠️⚠️ **The rows are INERT MARKER BUFFS (all slots 254) — the heal is paid by Lua.** SPA 178
>     `MeleeLifetap` is percentage-only by construction (`damage * mod/100`, `mob.cpp:6860`) and
>     nothing else fires on every successful melee hit, so a FLAT per-hit amount cannot be expressed
>     in `spells_new` at all. `lua_modules/aotv4_thirst.lua` holds the amounts and pays out from
>     `global_player.event_damage_given`; **the SQL's numbers are documentation only**. Same
>     inert-marker pattern as 43022 Divine Aura / 43035 Blade Turn / 43056 Counterattack.
>     ⚠️ That hook runs on **every damage event for every player** — keep the arithmetic early-outs
>     first, before any `FindBuff`. Reactions run first and own the return value (a negative override
>     negates the hit, which must not pay a leech).
>     **CHARGES are native**: `numhits` + `numhitstype = 5` ("Outgoing Hit Successes") makes the engine
>     decrement per successful melee hit and fade the buff at zero (`CheckNumHitsRemaining`,
>     `spell_effects.cpp:7075`, fired from `CommonOutgoingHitSuccess`, `attack.cpp:6690`), and the
>     client shows the counter for free. It decrements **any** buff with `hit_number > 0` of that type
>     — no real effect needed. Native precedent: the **Blade Attunement** line (4644/4646/4648) is
>     likewise an inert buff (slots = filler 10) with `numhits 100, type 5`; native charges run 5-800.
>     ⚠️ Charge and heal are counted in **different places**, so a hit absorbed to 0 damage spends a
>     charge without paying a heal. The `numhitstype` legend is in `spell_effects.cpp:7077`.
>   - `custom/sql/aotv4_sinew_line.sql` — a **damage tap that returns ENDURANCE**, weighted **75
>     percent damage / 25 percent endurance**, ids **43318-43323** (Sinewbite → Sinewreave, levels
>     8/18/28/38/48/58, damage 27/90/120/225/360/525, return = **damage / 3**). The structural
>     mirror of the Moonfire line, which splits the same way but returns health. **It exists because
>     of §22**: specials now cost endurance in proportion to their damage, and this refills the bar.
>     ⚠️ **`AoT:SpecialEndurancePct` was retuned to 33 to match this return** — the two are one
>     calibration, so changing either damage value here means revisiting that rule.
>     Damage sits just under the average native single-target nuke for the level (averages read out
>     of the DB, not from memory) because the cast also returns a resource; every damage value is
>     divisible by 3 so the return is a clean integer.
>     ⚠️⚠️ **NOT a lifetap, and must not be "fixed" into one.** `targettype` stays **5**. Setting it
>     to 13 is what `IsLifetapSpell` keys on (`common/spdat.cpp:108`) and makes `Mob::Damage` heal
>     the caster 1× the damage in **health** (`int64 healed = damage;`, `zone/attack.cpp:4287`) —
>     free HP nobody asked for, on top of the endurance. Moonfire *wants* that engine-paid 1× and is
>     a genuine tap; this line does not, so Lua pays **the whole** return.
>     ⚠️⚠️ **The return is not expressible in the DB at all** — there is no endurance-tap SPA. SPA
>     189 `SE_CurrentEndurance` applies to the spell's TARGET, and the target of a nuke is the thing
>     you are hitting, so it would hand the **monster** the endurance; a recourse could aim it at the
>     caster but only for a flat amount unrelated to the tier. Hence plain nukes +
>     `quests/global/spells/433xx.lua` → `lua_modules/aotv4_sinewtap.lua`. **Change a damage value in
>     the SQL and you MUST change that tier script's return (= damage / 3).**
>     ⚠️ **Clients only** — that is the shape of the mechanic, not a rare-case guard. `Mob::GetEndurance`
>     is a virtual returning 0 and `Mob::SetEndurance` a no-op (`zone/mob.h:674`, `:677`); only
>     `Client` overrides them (`zone/client.h:751`, `:756`). Resolve the caster with
>     **`GetClientByID`, not `GetMobID`** — it resolves invalid for a pet or NPC caster, which is
>     exactly the case to skip. ⚠️ There is **no `AddEndurance` binding** (only `Get`/`Set`,
>     `zone/lua_client.cpp:1304-1321`), so the max-endurance clamp is applied by hand or a cast at a
>     full bar sets endurance above max.
>     ⚠️ Cloned from stock **466 Lightning Shock**; only `new_icon` is overridden, to **47** — the
>     icon 44 native lifetaps use, so it reads as draining rather than lightning. Named "Sinew"
>     because it collides with nothing; deliberately **not** "Marrow" (stock 8608 Marrow Drain / 8803
>     Marrow Bend would sit beside it in the picker).
>     📌 Damage/mana is untuned until it is played (dpm 1.13 → 1.59 across the tiers).
>   - ⚠️⚠️ **A STACKING MARKER'S *SLOT INDEX* ISOLATES ONE LINE FROM ANOTHER; ITS *VALUE* ONLY ORDERS
>     TIERS WITHIN ONE LINE.** `custom/sql/aotv4_marker_slot_separation.sql` (migration **v10**,
>     2026-08-03). The tier ladders needed a higher tier to replace a lower, which is the SPA 44 +
>     `StackingCommand_Block` pair the Promised line already used — but the first pass put **reptile,
>     sloth AND thirst all in slot 1 with the same 100..600 values**. `Mob::CheckStackConflict` walks
>     two spells slot by slot and only compares when the effect ids at the **same index** match, so
>     three unrelated lines started comparing against *each other*: Skin of the Serpent (300) blocked
>     Rising Thirst (200), and Rising Thirst was overwritten the other way round. Reported as *"Thirst
>     and Skin lines are overwriting each other"* — **worse than the bug it replaced**, which was
>     merely tiers failing to replace their own.
>     One slot per line, never shared, and never a slot holding one of that line's own real effects:
>     **reptile 1 · sloth 2 · kindred 3 · thirst 4**.
>     ⚠️⚠️ **Any future line needing a marker takes slot 5.** Reusing 1-4 silently recreates this, and
>     it presents as **two unrelated spells cancelling each other** — not something anyone would think
>     to trace back to a stacking marker.
>     ⚠️ Values are deliberately left identical (100..600) across all four. Once the slots are
>     separated they need not differ, and making them differ would be *worse* if a slot ever did
>     collide again: one line would quietly outrank another instead of the collision being obvious.
>     ⚠️⚠️ **SPA 44 works as a marker because it is MECHANICALLY INERT HERE — do not assume any
>     spare-looking SPA is.** 44 is `SpellEffect::Lycanthropy`, and in this codebase it falls into the
>     big no-direct-action fall-through in `SpellEffect()` (`zone/spell_effects.cpp:3207`) **and has
>     no handler in `bonuses.cpp` at all**, so a base of 100..600 grants nothing — it exists purely to
>     be arbitrated on. Pick a different SPA and the marker starts handing out a real bonus scaled by
>     tier, silently. Check **both** places before substituting one.
>     ⚠️ The filler slots do NOT collide, and that is `IsBlankSpellEffect` (`common/spdat.cpp:950`)
>     doing the work: the comparison loop `continue`s on any slot that is **254**, a **10 spacer**
>     (base 0 **and** formula 100 — the formula matters), or SPA **148/149**. That is why three of
>     these lines can sit on 254 at the same index without arbitrating.
>   - ⚠️⚠️ **A re-runnable script's cleanup `DELETE` must only name the ids that script itself
>     creates.** `aotv4_moonfire_line.sql` opened with `DELETE … WHERE id BETWEEN 43312 AND 43323`
>     to sweep up leftovers from an earlier pass — which silently covers **43318-43323, the whole
>     Sinew line**. Re-running moonfire would have deleted six spells belonging to another script,
>     with no error. Narrowed to `43312 AND 43317` (2026-07-29).
>
> ⚠️⚠️ **CLONING A STOCK SPELL INHERITS ITS DAMAGE FORMULA — overriding `effect_base_value1` is NOT
> enough** (bit the Sinew line, 2026-07-29). `spells_new` carries `formulaN` and `maxN` beside every
> base value. **Formula 1-99 means `base + caster_level * formula`** (`zone/spell_effects.cpp`, the
> default branch of the formula switch); **100 means "= base"**, the static case. 466 Lightning Shock
> ships `formula1 = 5, max1 = 570`, so the cloned tier dealt **27 + 8×5 = 67** at level 8 instead of
> 27, heading for 277 by level 50, with the top tier clipped at 570.
> - ⚠️ For a hand-tuned tier ladder this breaks the **design**, not just the numbers: any Lua-paid
>   companion value (the Sinew line's endurance return, a Moonfire-style bonus heal) is a **flat**
>   amount, so damage that climbs with level while the return stands still destroys the ratio the
>   line exists to express.
> - Fix is `formula1 = 100, max1 = 0`. An audit of all of **43300-43347** found the Sinew line to be
>   the **only** offender — every other custom line is already `formula 100 / max 0`, because they
>   cloned either a static stock buff or (Moonfire) a custom that was already static. That is also
>   why this had never shown up before.
> - 📌 **Check `formulaN`/`maxN` on any future clone before trusting its numbers**, the same way the
>   proc-slot note below says to check the template.
>
> ⚠️ **Observed damage will still exceed the base — that is DC Overpower, not a bug.**
> `zone/mob.cpp:8860` adds bonus spell damage scaled off how far the caster's DC exceeds the
> target's resist (`AoT:DCOverpower*` rules), which against a weak mob was contributing ~24 at
> level 8. It also means a **tap's engine-paid 1× follows the INFLATED damage while the Lua bonus
> stays flat**, so Moonspark healed ~100 rather than its nominal 75. Expected, and the same class of
> approximation already recorded for partial resists.
>
> ⚠️ The proc SPA sits in **slot 4** on the reptile line but **slot 3** on the sloth line — check the
> template before overriding. ⚠️ SPA 11 base is the attacker's resulting melee **speed**, not the slow
> amount: 75 = "attacks at 75 percent" = a 25 percent slow, so **lower base is stronger**.
> `spells_new` **is in shared memory**: world down → `./shared_memory` → restart, then rerun both
> generators.
>
> ⚠️⚠️ **THE BARD PIVOT SILENTLY BROKE ALL-CLASS SPELL ACCESS (fixed 2026-07-27).** While every
character was forced to Bard, "any class can use anything" was free -- the pool only had to be legal
for Bard. Opening the server to 16 classes removed that and the STOCK set was never reopened: of
2,707 pool spells, **ZERO** were castable by all classes, and **bard songs carry a real level only
in `classes8`** so 15 of 16 classes could be offered a song and then simply never use it. Three gates
all had to be reopened, and any one left shut makes the reward useless:
1. `spells_new.classes1..16` -- can the class scribe/cast it
2. `skill_caps` **Singing 12, Percussion 41, Stringed 49, Wind 54, Brass 70** -- only Bard had ANY
3. `skill_caps` for the twelve activated combat specials -- without a cap `CanHaveSkill` refuses and
   a picked reward silently does nothing
All three are in `custom/sql/aotv4_open_spells_and_skills.sql`. ⚠️ Running step 3 DESTROYS the record
of which classes had a special natively -- the only surviving copy is `skill_pool.NATIVE`.

⚠️ **The generator writes `spell_blacklist.lua` too — it MUST be regenerated with the pool.** When
> the custom set took over, the blacklist was emptied ("empty by design: the pool is hand-authored"),
> so re-pointing at the stock set without rebuilding it silently reopens every port, rez and
> Enchant-material spell. Current prune = **473** (travel 169 / summon-item 199 / illusion 23 /
> vision 19 / enchant 20 / rez 15 / cure-curse 7 / LDoN 18 / summon-corpse 3), leaving **2174
> offerable**. Travel is pruned partly because ports **defeat region locking**.
>
> ⚠️⚠️ **`teleport_zone` is NOT "destination zone" — it is "names an NPC *or* a zone".** Every
> pet/familiar/warder/Eye-of-Zomm spell stores its **summon type** there (SPA 33/71/152 pets, 106
> warders, 108 familiars, 67 eye). Keying the travel filter off `teleport_zone <> ''` pruned **95
> stock pet spells** — every Magician elemental, Necro pet, Shaman/Beastlord warder and Enchanter
> animation. Detect travel by **SPA** instead: 25 Bind Affinity, 26 Gate, 83 Teleport, 88 Evacuate,
> 104 Translocate. No travel spell lacks one.
>
> ⚠️ **`targettype = 3` was ALSO in the travel rule and is now REMOVED — do not add it back.**
> ST_Group is 3, which is *every* group-target spell, not a group teleport: it was pruning **78
> ordinary group buffs** (Elixir of Divinity, Wave of Marr, Eriki's Psalm of Power, the group heal
> lines) as if they were ports. Of the 150 `targettype=3` spells in the pool, the **72 that really
> are travel all carry one of the SPAs above** and are caught anyway — the clause bought nothing.
>
> ⚠️ **The junk rules must test PURITY, not presence** (2026-07-27). `illusion` (SPA 58) and `vision`
> (SPA 13/65/66/87) prune only when **every populated slot** is that effect or 254 — a spell is kept
> if the cosmetic bit rides along with something real. Written as "has the SPA" they took **31
> illusions that carry procs/stats/haste** (Boon of the Garou, Night's Dark Terror, Illusion:
> Werewolf) and, once illusion was narrowed, the vision rule inherited the same failure and swallowed
> the wolf/hunter forms (427, 1562, 1563, 3579, 4107) purely for their ultravision. Any new rule here
> gets the same treatment: prune the spell that does *only* the junk thing.

`spell_pool.lua` is generated from `spells_new`, indexed by learn level. The reward is
**expansion-capped by level**: a char is only offered spells at/below its level, and a spell's
learn level sits in its expansion's band (classic 1-50, Kunark/Velious/Luclin 51-60, PoP+ 61-70…).
Filter: `id < 10000` (drops ~30k modern rank/revamp spells), no `Rk.` versions, learnable
(`classes8` = lowest class level; we set `classes8 = LEAST(real class cols)` for Bard usability,
so it doubles as the index). Regenerate:
```bash
mysql -h127.0.0.1 -upeq -ppeqpass peq -N -e "
  SELECT classes8,id,name FROM spells_new
  WHERE id<10000 AND name NOT LIKE '% Rk. %' AND classes8 BETWEEN 1 AND 100
  ORDER BY classes8,id;"   # then format to  [lvl]={ {id=,name=}, ... }  (see git history)
```

`spell_blacklist.lua` (**generated**) = `{ [id]=true }` of spells **never** offered, applied at
runtime in `spell_choice.gather_candidates`. Criteria:
- resurrect (effect `81`), Enchant-material (name `Enchant %`),
- curse-counter cures (effect `116` with **negative** base = removes curse counters),
- LDoN dungeon-object utility (`targettype = 34` — Iony's/Reebo's appraise/disarm/unlock lines).

> No expansion column exists in `spells_new`, and Allakhazam/EQ-Resource are JS-rendered (can't
> crawl). So era filtering is the `id<10000` + level approximation, not a surgical classic list.

## 6. Death-driven AA rewards (roguelite) — ⚠️ RETIRED 2026-07-25, kept as history

> **Random AA is gone and AA is back to BASE EQ**: earned as AA experience, spent in the client's own
> AA window. The whole picker is unwired — `global_player.lua` no longer requires `aa_choice` (no
> `grant_picks` on death, no `handle_say`, no `reoffer_if_banked`), the dll's AA overlay is **deleted**
> (§3), and `aa_choice.lua`/`aa_pool.lua` remain on disk but nothing loads them. The dll still swallows
> stray `AACHOICEDATA`/`AADESCDATA` lines so they can never reach chat.
> **DB revert = `custom/sql/aotv4_aa_revert.sql`** (restores `aa_ranks.level_req`, `aa_ability.classes`
> and `grant_only` from `utils/sql/peq_aa_tables_post_rework.sql`, because `aotv4_aa.sql` overwrote the
> originals in place). Until it runs, `grant_only=1` hides all 382 tagged AAs from the native window.
> ⚠️ Retiring this **orphans `era_system.check_unlock`** — see §12.


A second progression layer parallel to the level-up window. **On death, ALL of the run's
experience is banked as Alternate Advancement and the character is reset to level 1** — AAs are
the permanent meta-progression; levels are per-run. A second random picker (its own chat channel,
so it can drive a separate dll window) then spends the banked points.

### Server side (Lua)
- `global_player.lua` `event_death(e)`: `SetLevel(1)`, `SetEXP(0, GetAAExp())`, then bank
  `floor(total_xp / divisor)` points by calling `aa_choice.grant_picks(e, banked)`. **The points go
  into a PRIVATE data bucket (`aa_bank_<charid>`), NOT the native unspent pool** — so the native AA
  (V) window shows 0 spendable points and the player can't bypass the picker to buy AAs directly.
  Picks are granted **free** (`ignore_cost=true`) and the cost is deducted from the bucket.
  **Diminishing returns:** `divisor = AA_XP_BASE * (1 + GetSpentAA()*AA_SCALE)` — the more AAs you
  own, the more XP each new point costs (`AA_XP_BASE=15M` ≈ 10 points at a level-50 death;
  `AA_SCALE=0.005`). `event_say` calls `aa_choice.handle_say(e)` (alongside `spell_choice`).
- `aa_choice.lua` — **budget-driven** picker. The player's *unspent* pool (`GetAAPoints`) IS the
  budget. Each pop offers `CHOICE_COUNT` (3) random AAs that are **affordable** (`cost ≤ budget`)
  **and prereq-satisfied**, emits `AACHOICEDATA <banked>^name|icon|cost|cls^…` + a sibling
  `AADESCDATA d1^d2^d3` line (drives the dll window) **plus** chat saylinks (work with no client
  mod). Picking trains the next rank **free** (`GrantAlternateAdvancementAbility(id, cur+1, true)`)
  and deducts the cost from the private bank; it keeps popping until nothing is affordable
  (`MAX_PICKS=30` cap). Tunables at top. Buckets: `aa_bank_/aa_choice_/aa_seed_/aa_pcount_<charid>`.
  Seed RNG on a per-offer counter (post-death `GetEXP`/level are 0).
  **Per-AA cap (`RANK_BUDGET=10`):** an AA is only offered while its *total* investment
  (`next_rank * cost`) stays within the cap, so no single AA needs more than ~one death's bank —
  cheap AAs (cost 2) still reach ~rank 5 (enough for rank≤3 prereqs), a cost-9 AA stays rank 1.
  Keep `RANK_BUDGET` in step with the per-death bank (`AA_XP_BASE`); raise BOTH as the level cap rises.
- **dll window** (`core_spellwindow.cpp`, `areAAChoiceWindowEnabled`): a SECOND layered GDI window
  (cool slate theme, lower-third placement) parallel to the spell window. It **shares the same two
  detours** (`dsp_chat`, `ProcessGameEvents`) — both can't re-hook the same address, so the AA code
  lives in those bodies, gated by feature flags. Shows **banked AA in the header**, each choice's
  **cost**, a Train button → `/say aapick N` (echo swallowed like `spellpick`), and a **bottom
  description panel** that shows the hovered row's text. Both windows have the hover/desc panel
  (`DrawDescPanel` + `RowAtY`, fed by `SPELLDESCDATA`/`AADESCDATA`).
- `aa_pool.lua` (**generated**) — `pool[level]={ {id=,name=,icon=,cls=,cost=,desc=,pr={{a=,r=}}}, … }`.
  `icon` = cast-spell `new_icon`, else a thematic fallback by name (heal/caster/melee/defense/song/
  craft). `cost` = first-rank cost, `desc` = `db_str` type 4 (truncated), `pr` = first-rank prereqs.
  Generated from the **Bard-tagged** (`classes&128`) set — which the migration restricts to **classic
  (expansion ≤4), level ≤60, real-named, deduped-by-name** AAs (so no "Unknown DB String"/modern
  clutter). `spell_desc.lua` (gen, `db_str` type 6) feeds the spell window's hover the same way.
  The picker's `prereqs_met` mirrors the server's prereq loop so it never offers a rejectable AA.
- **Hiding untrained from the native window:** the tagged set is also flagged `grant_only=1`. The
  native AA window then lists each only once **trained** and can't buy it there (the picker grants
  via `ignore_cost=true` → `CanPurchase` `check_grant=false`, so `grant_only` doesn't block us).

### Three things had to agree (like combat skills)
1. **Class-agnostic.** Migration `aa_ability.classes |= 128` adds the Bard bit. The AA loader does
   `a->classes = e.classes << 1` (zone/aa.cpp:1792), so the DB's `1<<(class-1)` (Bard=128) becomes
   `1<<class` (256) in memory — which is exactly what the **stock** gate `classes & (1<<GetClass())`
   checks. ⚠️ **Do NOT "fix" that line to `GetPlayerClassBit`** — the `<<1` makes it correct;
   "fixing" it breaks every narrow-class AA. `aa.cpp` stays **stock**.
2. **Expansion.** AAs start in Luclin (`aa_ranks.expansion ≥ 3`). With classic theming
   (`Expansion:CurrentExpansion=0`), the rule **`Expansion:UseCurrentExpansionAAOnly` blocks ALL
   AAs** — set it **`false`**; expansion gating is done by the pool instead (like spells). The
   per-char `m_pp.expansions` check still applies (it = `RuleI(World, ExpansionSettings)` = all).
3. **Level gate.** Migration lowers `aa_ranks.level_req` `1..60 → 1` (deps preserved) so normal
   ranks are buyable at the always-level-1 post-death state. Ranks with original `level_req > 60`
   stay gated (unreachable while picking at level 1 — acceptable; they're raid-tier).

### Reset AAs for testing (char must be at **character select**)
```sql
SET @cid:=(SELECT id FROM character_data WHERE name='Ashrem');
DELETE FROM character_alternate_abilities WHERE id=@cid;
UPDATE character_data SET aa_points=0, aa_points_spent=0, aa_exp=0 WHERE id=@cid;
```
GM `#resetaa aa` only *refunds* spent → unspent (doesn't zero the pool). The migration +
`UseCurrentExpansionAAOnly` live in `.devcontainer/custom/sql/aotv4_aa.sql`.

## 7. Database setup
- **All 16 classes are playable** (§14). Historically every char was forced to Bard (`class = 8`); the
  force is removed and `char_create_combinations` restored to all classes. Existing chars stay Bard.
- **`spells_new.classes8`** = the spell's min learnable level (= the reward-pool index). It's now also
  opened across every class column (`classes1..16`) so all classes can scribe/cast the reward spells.
- ⚠️ **Combat specials: NATIVE ones are auto-granted, the rest are picker rewards.** Every class now
  has a `skill_caps` entry for all twelve (it must, or a picked reward cannot be granted), so
  `CanHaveSkill` is no longer a useful filter and the DB can no longer say what was native.
  `skill_pool.NATIVE` is the only record. `global_player.grant_native_combat_skills` grants a class
  its own on connect AND on level up (some cap curves only open above level 1);
  `spell_choice.gather_skill_candidates` offers only `skill_pool.rotatable_for(class)`.
- ⚠️ **`skill_pool.lua` is now a MODULE, not a bare table.** It exposes `.SKILLS`, `.NATIVE`,
  `.is_native()` and `.rotatable_for()`. `pairs(skills)` no longer yields skill ids -- use
  `pairs(skills.SKILLS)`. A `__index` metatable keeps `skills[id]` working for existing callers, but
  `pairs` does NOT follow it.
- **Bard `skill_caps`** (`class_id=8`) raised so skills scale; client also needs the exported
  `SkillCaps.txt` (`export_client_files`) installed in the EQ root.
- **Expansion lock:** `rule_values Expansion:CurrentExpansion = 0` (Classic).
- Test char `Ashrem`: charid **1**, GM — **Warrior (class 1)** as of 2026-07-29, not Bard; it was
  remade after the all-classes pivot (§14), so it also exercises the melee-as-caster path.
  (Reset a char's combat skills via
  `UPDATE character_skills SET value=0 WHERE id=<charid> AND skill_id IN (...)` — only sticks
  while that char is at **character select**, else the live zone saves over it.)

## 8. Building

**`dinput8.dll`** (Windows, Visual Studio): project
`eq-core-dll/src/eq-core-dll-vs2022.vcxproj` (toolset **v143**). Non-default settings for the
legacy MQ2-derived code: `ConformanceMode=false`, `LanguageStandard=stdcpp14`, `SDLCheck=false`,
defines `_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS`. `core_spellwindow.cpp` links
`gdi32`/`user32` via `#pragma comment`. **Close EQ before copying** (it locks the dll).

**Server (zone, etc.)** in the dev container — ninja:
```bash
cd /src/build && ninja zone     # ~minutes; relinks bin/zone
```
After a server-code change you MUST restart **every** booted zone (see §10), not just the pool.

## 9. Porting elsewhere

**To a player's CLIENT:** copy `dinput8.dll` next to `eqgame.exe`; run **windowed**; stock
`uifiles/default/spellsNN.tga` provide icons. Install the exported `SkillCaps.txt` (+ optionally
`spells_us.txt`, `dbstr_us.txt`) in the EQ root so skill caps/levels match the server. The dll's
offsets (`0x51F1A0`, `0x539E60`, `0x51FCE0`, `0x5BAEC0`) are for this exact RoF2 build — a
different build needs them re-derived. (`EQUI_SpellChoiceWnd.xml` is dead — overlay is self-drawn.)

**To another SERVER:** copy the `quests/lua_modules/*` + the `global_player.lua` hooks;
regenerate `spell_pool.lua`/`spell_icons.lua`/`spell_blacklist.lua` **and** `aa_pool.lua` from
that DB; apply the `special_attacks.cpp` patches and rebuild zone; run `custom/sql/aotv4_aa.sql`
(AA class/level migration) and set `Expansion:UseCurrentExpansionAAOnly=false`; set the DB (Bard
class, `classes8`, skill caps, expansion). The windows are generic (`SPELLCHOICEDATA name|icon` /
`spellpick N`; `AACHOICEDATA name|icon|cost` / `aapick N`). `aa.cpp` stays stock.

## 10. Gotchas / lessons learned
- ⚠️⚠️ **A Lua SPELL SCRIPT GETS AN EMPTY `e` WHEN THE TARGET IS AN NPC — fixed 2026-07-29,
  `zone/lua_parser.cpp`.** `SpellArgumentDispatch` is filled with `handle_spell_null` for every
  event (`:240`) and then only **four** are given a real argument builder. `EVENT_SPELL_EFFECT_NPC`,
  `EVENT_SPELL_EFFECT_BUFF_TIC_NPC` and the two BOT equivalents were **not among them**, so they
  fell through to `handle_spell_null` — a function with an **empty body**
  (`lua_parser_events.cpp:2182`). The event table therefore arrived with **no fields at all**: no
  `caster_id`, no `target`, no `spell_id`, no `tics_remaining`.
  - ⚠️⚠️ **The script still RUNS, which is why this hides so well.** `event_spell_effect` fires
    exactly as expected when you nuke a monster; every field on `e` is just nil, so a script that
    reads `e.caster_id` silently does nothing. The symptom is "the spell did not fire" with **no
    error anywhere** — and it is invisible to luacheck, to the zone log, and to reading the script.
  - **It only ever worked when the target was a PLAYER.** Beneficial/self spells land on a Client →
    `EVENT_SPELL_EFFECT_CLIENT`, which *was* registered. Of the 56 scripts in
    `quests/global/spells/`, **34 beneficial ones were fine and 22 detrimental ones were dead**:
    982, the retired customs 43010/43011/43015/43030/43052/43054/43055, helpers 43150/43155, **the
    whole Moonfire line 43312-43317** and the Sinew line 43318-43323.
  - ⚠️ **Moonfire had never worked in play.** Its 2× bonus heal (§5) reads `e.caster_id`, so against
    a monster it returned early every time and the line was a plain 1:1 lifetap paid entirely by the
    engine — for as long as it has existed. Nothing reported it.
  - The fix is registration only: `handle_spell_event` was **already written** for this case (its
    first branch is `if (mob)`, the NPC one), it was simply never wired to those four events.
  - 📌 **Before writing any new spell script, confirm the event you need has a real builder in
    `SpellArgumentDispatch`** — a missing entry is silent, not an error.
- **Lua + server-code reloads:** `#reloadquest` does NOT reload `require`d Lua modules, and a
  rebuilt `zone` binary only takes effect in **freshly-booted** zones. After a Lua or C++ change,
  kill **all** zone procs (`for p in $(pgrep -x zone); do kill -9 $p; done`) so `eqlaunch`
  respawns them — killing only `dynamic_*` leaves already-booted named zones (e.g. freporte) on
  the old code. Then **relog**.
- **Can't hook EQ's D3D device** — dummy-device vtable trick fails here; use the layered window.
  - ⚠️⚠️ **BUT THAT MAY ONLY BE TRUE OF THE APPROACH WE TRIED — OPEN LEAD, NOT PROVEN (2026-08-06).**
    Our finding was that a device we create shares no vtable with the one EQ renders through, so
    patching it never fires (`present=0 swap=0` with all vtables patched). That is a fact about the
    **dummy-device** route, not about D3D hooking in general.
    **`tunaria/NMS-Release` appears to do it a different way**: it injects into **`EQGraphicsDX9.dll`
    itself** (`ZoneRender_Injection_Detour` in `RenderHooks.h`) and captures the device from the
    **`this` pointer** inside the real `BeginScene`/`EndScene` detours — rather than creating a device
    and hoping the vtable matches. It then uses `WorldToScreen()` to place floating damage text in the
    world (`MQ2FloatingDamage.cpp`, hooking `CEverQuest::ReportSuccessfulHit`).
    ⚠️ That repo is **address-compatible with us** — same RoF2 build (May 10 2013 23:30:08) and every
    offset we hook matches — so it is portable if it works.
    📌 **If it does work it overturns this bullet and unlocks in-world rendering we have recorded as
    impossible**: floating damage, world markers, and a better answer to §17b's wanted short-buff /
    autoskill tracker than burning a spell row and a buff slot per marker (43380-43382 Shield Wall,
    43022 Divine Aura, the Thirst line — all of which exist only to be SEEN).
    ⚠️ **Unverified.** Nobody has built or run it here. Treat as the next thing to try when in-world
    drawing is wanted, not as a settled capability — and re-read this bullet's original finding first,
    because the dummy-device route genuinely does not work.
- **DllMain loader lock** — defer window/thread/D3D creation out of the `Enable*` functions.
- **Skill cap hook** — only *reveal* (cap 0 → value); never override a native cap, and bound to
  ids `0..76` or you get "None" rows. Skill values live in `CHARINFO2` (not the `pChar`
  `CHARINFO` passed to `GetSkillCap`) — that's why earned-state comes via server sync, not memory.
- **Activated abilities = 3 layers** — client display (`GetSkillCap`), client *send*
  (only for skills it thinks you have), and server *execute* (`special_attacks.cpp` class gates).
  All three must allow it.
- **Spell icon mapping** — raw index (`icon/36`, `icon%36`), 0 = none; derive columns from sheet
  width, don't assume 6.
- **Bazaar** — RoF2 only ships the revamped (DoN) bazaar; the old Luclin bazaar is unobtainable
  on a vanilla client.
- **AA class bitmask is shifted** — `aa_ability.classes` is loaded as `classes << 1`
  (zone/aa.cpp), so the gate `classes & (1<<GetClass())` is correct *as-is*. Tag Bard with the DB
  bit **128** (`classes | 128`), NOT 256, and **don't** rewrite the gate to `GetPlayerClassBit`.
  Symptom of getting it wrong: 65535 (all-class) AAs grant but narrow-class AAs silently fail.
- **AAs aren't in shared memory** — the zone loads `aa_ability`/`aa_ranks` straight from the DB at
  boot, so DB edits just need a zone restart (no `./shared_memory` rebuild). Rules are also read at
  boot — a `rule_values` change (e.g. `UseCurrentExpansionAAOnly`) needs a restart, not a reload.
- ⚠️⚠️ **A custom AA must REUSE a native row — a new id never reaches the client.** The server
  resolves it, passes every check in `CanUseAlternateAdvancementRank` and sends the packet (proven
  with logging); the client silently discards it. Ability ids above the native max **30195** and
  rank ids above **65535** both fail this way, no error anywhere. Take over an existing ability
  instead: keep its id, rank ids, `title_sid` and rank chain, replace only `aa_rank_effects` and the
  `db_str` strings (`custom/sql/aotv4_aa_tank_hosted.sql`).
- ⚠️⚠️ **`aa_rank_prereqs` IS A SEPARATE TABLE AND MUST BE CLEARED TOO.** A hosted AA inherits its
  host's prerequisites, so it appears in the window but **refuses to train with no message**.
  Clearing `aa_rank_effects` does nothing to it. Eight AAs were untrainable this way.
- ⚠️ **Walk the rank chain via `next_id` — rank ids are NOT contiguous.** Natural Durability's chain
  is **107 → 108 → 109 → 7541 → 7542 → 7543**; assuming `first_rank_id + 0..4` wrote costs and levels
  onto ranks 110/111, which belong to a different ability. Terminate a chain early with
  `next_id = -1` to control how many ranks show (otherwise the window reads e.g. "0/15").
- ⚠️⚠️ **There is NO usable native "survive a killing blow" effect.** All three death hooks were
  checked (2026-07-26, Borrowed Breath) and every one fails for a different reason:
  - **SPA 232 `DivineSave`** (`TryDivineSave`) is the only one that runs inside the `HasDied()`
    branch, but it unconditionally casts **4789 Touch of the Divine** — SPA 40 Divine Aura for up to
    **36 s**. Under it you cannot attack (`attack.cpp:1743`), cannot cast (`spells.cpp:566`), and
    **no other caster can land any spell on you, heals included** (`spells.cpp:4147` — *"invuln mobs
    can't be affected by any spells, good or bad"*). You are also unattackable, so the boss drops
    the tank and turns on the healer. It keeps you alive by removing you from the fight. This is
    also exactly what the **"Second Chance" tribute** is (worn effect 5612/5613/5614 *Divine Res*).
  - **SPA 150 `DeathSave`** (Divine Intervention / Death Pact) is **not a death save here** —
    `TryDeathSave` is called from the *else* branch, only on crossing below 16 % health **alive**.
    Its chance also comes off the *saved player's* Charisma, which is backwards for a role AA.
  - **`SpellOnDeath`** (`TrySpellOnDeath`) always returns false on purpose; the comment
    (`mob.cpp:6636`) says a heal placed there cannot register before death kills you.
  - ⚠️ **Tributes are not a separate mechanism.** `CalcBonuses` resolves a tribute to an ITEM and
    runs it through `AddItemBonuses(..., is_tribute=true)` (`bonuses.cpp:177`), so a tribute can
    only ever deliver the same SPAs.
  - The working answer is a hand-rolled check in `Mob::Damage`'s `HasDied()` branch before the
    native saves — `Mob::AoTv4TryBorrowedBreath` (`zone/aotv4_healer_aa.cpp`). Return at a share of
    max HP, not 1 HP: at 1 HP the next tick of anything undoes the save.
- **The AA window tab is `aa_ability.type`**, not `category` (category is a sub-grouping header
  *within* a tab). 1 General / 2 Archetype / 3 Class / 4 Special, relabelled **Tank / Healer /
  Ranged / Melee** in `aotv4_client_install/uifiles_default/EQUI_AAWindow.xml` (a stock file we
  overwrite; no `<Include>` needed). Labels are XML-driven — the stock strings aren't in
  `eqgame.exe` — but page/listbox `item=` names resolve positionally, so rename only `<TabText>`.
- **AA names come from `db_str`** (type 1 title, type 4 description) resolved by the CLIENT out of
  its own `dbstr_us.txt`, so a `db_str` change needs `./export_client_files` + reinstalling that
  file. `title_sid = -1` renders **no row at all** — it does not fall back to `aa_ability.name`.
- ⚠️⚠️ **AN AA HAS *THREE* NAMES IN `db_str`, AND THE HOTKEY ONE IS SPLIT ACROSS TWO ROWS.**
  `aa_ranks` carries `title_sid` **and** `upper_hotkey_sid` **and** `lower_hotkey_sid`, and the
  client selects on `(id, type)`: **type 1** the window title, **type 2** the hotkey's UPPER line,
  **type 3** its LOWER line, **type 4** the description. Renaming an AA by writing types 1 and 4 —
  which is what `aotv4_aa_rename.sql` did — leaves the **host's** hotkey text in place, so the AA
  window reads "Iron Will" while a hotkey made from that same ability reads **"Frenzied Burnout"**.
  Fixed for all 10 activated abilities by `custom/sql/aotv4_aa_hotkey_names.sql` (first word to
  type 2, remainder to type 3; passives carry `-1` and are skipped).
  - ⚠️⚠️ **The split is why this is nearly unsearchable.** The string on screen never exists in the
    data: grepping `db_str`, `dbstr_us.txt` **and** `eqgame.exe` for `Frenzied Burnout` all return
    **zero**, because it is stored as `Frenzied` + `Burnout`. That produced three separate wrong
    conclusions (a stale `spells_us.txt`, a clone-source name, a hardcoded client string) before
    `SELECT id, type, value FROM db_str WHERE id = <sid>` — **dump every type for the sid** — showed
    it instantly. Search a single word, or dump the sid; never grep the rendered string.
  - ⚠️ In our pool `title_sid`, `upper_hotkey_sid` and `lower_hotkey_sid` are all the **same sid**,
    so these are three rows of one id. That is exactly what makes it look like fixing the title
    should have fixed the hotkey: the id matches, only the **type** differs.
- ⚠️⚠️ **`title_sid` and `desc_sid` are INDEPENDENT, and neither is guaranteed to equal
  `first_rank_id`.** 22 of our 23 hosts have `title_sid = desc_sid = first_rank_id`, which makes it
  look like a rule — **Quick Damage (44) does not**: `title_sid 141`, `desc_sid` **12863**. Writing
  the description to the title sid gave an AA with the right NAME and the host's ORIGINAL
  description, with no error anywhere. Always
  `SELECT title_sid, desc_sid FROM aa_ranks WHERE id = <first_rank_id>` before writing `db_str` for
  a new host, or repoint `desc_sid` to match the title sid as `aotv4_aa_ranged.sql` now does.
- **`items` (and `spells_new`) ARE in shared memory** — unlike AAs, item edits need a full rebuild:
  stop world+zones, `cd build/bin && ./shared_memory`, restart world (zones reboot on demand). Do it
  with world DOWN, or world keeps the stale mmap.
- **Gear tiers** (`custom/sql/aotv4_gear_tiers.sql`): two tiers above native, generated from
  obtainable equippable gear (slots>0, in lootdrop_entries/merchantlist, **base ids <300,000** —
  never re-tier our own rows). **Hallowed = base id +300,000** (stats/resists/dmg ×2, hp/mana/end
  ×2, AC ×1.5, int→.5 spelldmg, wis→.5 healamt, str/agi/dex /10 → attack/strikethrough/accuracy,
  instruments ×1.5, tradeable). **Mythic = +600,000** (hp/mana/end ×2.5, AC ×2, int/wis→1×
  spelldmg/healamt, +½ heroic on every stat+resist, instruments ×2, **No Drop**). All three tiers:
  classes/races=65535, not-lore; **native + Hallowed tradeable, only Mythic No Drop**. Derived stats
  use the scaled (×2) values.
  - ⚠️ **Tier rows ALREADY inherit `price`/`sellrate` from the native — do not "fix" this** (checked
    2026-07-28: 0 of 54,292 tier rows differ from their base on either column). The clone is
    `INSERT … SELECT *` and nothing in the script touches those columns. 33,720 tier rows show a
    price of 0 only because **the base item's price is 0 too**; there is nothing to inherit.
  - ⚠️⚠️ **"Mythic won't sell" is NO DROP, not price.** `Handle_OP_ShopPlayerSell` bails at
    `if (!item->NoDrop) return;` (`client_packet.cpp:15221`) *before* price is read, and
    `Client::AdvLootSellValue` returns 0 for the same reason (`:10674`). A Mythic carries the native's
    full price and still cannot be sold to a vendor — that is the design, not a bug. The 17 Hallowed
    rows that also refuse are the **epics**, forced Lore + No Drop at the end of the tier script.
  Two C++ hooks (tracked patches): `NPC::ResolveTierDrop` (loot.cpp) decides **which tier** a rolled
  item drops as — **5% Mythic / 25% Hallowed / else base** (mode-independent, since
  `lootdrop_entries.chance` is a weight in the dominant weighted loot mode); `AoTv4MythicReward`
  (questmgr.cpp + lua_client.cpp) upgrades **quest-reward** gear to its Mythic tier (epics never hand
  out native). Re-run the SQL → rebuild shared memory → restart.
  - ✅ **CRAFTING ALWAYS YIELDS MYTHIC** (`ZoneDatabase::GetTradeRecipe`, tradeskills.cpp ~:1746). Every
    combine output is swapped for its Mythic tier when one exists, so crafted gear is top-tier and stays
    relevant as later expansions unlock better base items. Epics are tiered, so they are covered too.
    - ⚠️⚠️ **ONE choke point serves BOTH overloads.** `GetTradeRecipe(container, …)` resolves the recipe
      id from the container's contents and then **delegates** to `GetTradeRecipe(recipe_id, …)` (`:1600`,
      `:1639`), so the trade window and the world-container path both pass through here exactly once.
      Never copy the upgrade into a caller — two copies drift, per §22's reasoning.
    - ⚠️ It normalises through **`AoTv4TierBaseId` first**. Adding the step to an id that is *already* a
      tier would land outside the reserved band where no item exists, so the swap would silently no-op;
      normalising makes a Hallowed output upgrade and a Mythic output idempotent. (No stock recipe
      outputs a tier id today — 0 of 19,464 — so this is a guard, not a live path.)
    - 📌 **Only WEARABLES are affected, and that falls out of the data rather than a test.** The gate
      used to be `Slots > 0`, which was redundant: `aotv4_gear_tiers.sql` only generates tiers for
      slots>0 items, so the "does a Mythic exist" lookup already *is* the wearable test. Of the enabled
      recipes, **12,323 produce a wearable** (all 8,889 distinct outputs have a Mythic row) and **11,031
      produce a non-wearable** — 5,112 plain components, 728 food/drink, 71 containers, and 5 junk
      outliers. **Do not tier those**: a component has no stats to scale, so a "Mythic" one would be
      byte-identical to its base and would only split **2,556** component stacks across two ids.
    - ⚠️ Only the **success** list is upgraded — `onfail` and `salvage` are untouched, and quest recipes
      (5 of 19,464) hand out items from Lua/Perl and bypass this entirely.
  - ⚠️⚠️ **A TIER REPLACES THE DROP, IT DOES NOT ADD ONE (fixed 2026-07-29).** The original
    `AddTierUpgrades` rolled the two tiers **independently** and called `AddLootDrop` for each *on top
    of* the base, so one base item could put **three items on the corpse at once** (base + Hallowed +
    Mythic). A lucky roll meant **more** loot rather than **better** loot, and every upgrade dragged
    its own inferior copies along with it. Now one item goes in and one comes out.
  - ⚠️ The 5/25 rates are unchanged but are now **mutually exclusive bands of a single roll**, so
    **Mythic is tested first** — it is the rarer band and testing Hallowed first would swallow it
    entirely. Expect marginally fewer Hallowed than before (25% of drops, not 25% plus the 5% that
    also rolled Mythic).
  - **The tiers are always created in PAIRS** — `aotv4_gear_tiers.sql` generates Hallowed and Mythic
    from the same base set, and the DB confirms it: **27,137** rows each, **zero** Hallowed without a
    Mythic and zero Mythic without a Hallowed (re-checked 2026-08-05 after the 0.1.3 item import; it
    was 27,146 on 2026-07-29 — **the COUNT drifts whenever the base set changes, the PAIRING does
    not**, so verify the invariant, never the number).
    ⚠️⚠️ **COUNT THE TIER BANDS AS `[300000,599999]` AND `[600000,899999]`, NOT `id >= 600000`.** The
    loose test sweeps in ordinary high-id items and invents unpaired rows that do not exist — on
    2026-08-05 it reported 3 phantom "Mythic without Hallowed": stock `990061` Artisan's Universal
    Kit, our `2000060` Refining Crucible, and stock `3008007` **"Mythic Hallowed Shuriken"**, whose
    name alone makes it look like a tier row. That cost a false alarm mid-import. So the split really is a clean
    5 / 25 / 70, and the only item that is native 100% of the time is one with **no tier rows at all**
    (quest paper, whatever the tier script skipped), which returns the base untouched.
  - 📌 The Mythic branch **falls through** to the Hallowed test instead of returning, so a half-tiered
    item would degrade to Hallowed rather than to base. Given the pairing invariant above that branch
    is **unreachable** — it is a safety net for a hand-edited or partially-regenerated tier set, not a
    live code path. Do not read it as evidence that half-tiered items exist.
  - ⚠️ In the weighted path the **`multiplier`/charges loop rolls its own tier per copy** — it
    previously got no upgrade at all, and sharing one roll would hand out one upgraded item plus N
    base duplicates.
  - 📌 loot.cpp now uses `zone/aotv4_tiers.h` (`AOTV4_TIER_STEP` / `AoTv4IsTierId`) instead of literal
    `300000`/`600000` — it was touched for another reason, which is exactly the condition that header
    names for migrating one of the five hardcoded copies. Four remain (npc/questmgr/attack/tradeskills).
  - ✅ **QUESTS ACCEPT ANY TIER — do not "scrub" the 8,000 quest scripts, it is already handled in
    five C++ choke points** (audited 2026-07-28). `zone/aotv4_tiers.h` holds the id maths now
    (`AoTv4TierBaseId`, `AOTV4_TIER_STEP`); the five OLDER hardcoded copies (loot/npc/questmgr/attack/
    tradeskills) were deliberately left alone — migrate one only if it is touched for another reason.
    | Path | Covers |
    |---|---|
    | `NPC::CheckHandin` (npc.cpp, pre-existing) | **all turn-ins** — Lua `check_turn_in` (1,039 files) *and* Perl `plugin::check_handin` (1,281) both funnel here |
    | `Lua_Client::CountItem` / `Perl_Client_CountItem` / `QuestManager::countitem` | possession checks |
    | `Lua_Client::HasItemOnCorpse` | the corpse fallback inside `Client:HasItem` |
    | `Client:HasItem` (`lua_modules/client_ext.lua`) | ~230 Lua files — it **scans slots itself**, never calls CountItem, so it needs its own `aotv4_same_item` |
    | `Perl_Client_HasItemOnCorpse` (**added 2026-08-01**) | the corpse fallback inside `plugin::check_hasitem` |
    | `plugin::check_hasitem` (`plugins/check_hasitem.pl`, **added 2026-08-01**) | **133 Perl files** — the Perl twin of `Client:HasItem`, and it scans slots itself the same way |
    - ⚠️ **Scoped to the QUEST bindings, never `Client::CountItem` itself** — the engine calls that for
      merchants, corpses and NPC inventories, where "a Mythic Rusty Dagger is a Rusty Dagger" is the
      WRONG answer.
    - ⚠️ 7 Perl files compare `$itemN ==` directly and bypass all of it. Audited: 5 of the 6 ids are
      untiered quest paper, and the 6th (26015 Pulsing Goo) only ever appears as a
      `quest::ChooseRandom` reward roll — a script-local, never a hand-in. **No real gaps.**
    - ⚠️⚠️ **THE 2026-07-28 AUDIT MISSED THE WHOLE PERL POSSESSION PATH** (found and fixed 2026-08-01).
      It reasoned in terms of the *Lua* call graph, fixed `Client:HasItem` when it turned out to scan
      slots itself — and never asked whether Perl had the same shape. It does: `plugin::check_hasitem`
      does a raw `GetItemIDAt($slot) == $item_id` (plus the same on every augment socket), reaching
      none of the C++ choke points, so **133 Perl quest scripts told a player holding only the Mythic
      copy that they did not have the item.** Its corpse fallback `Perl_Client_HasItemOnCorpse` was
      unnormalised too, while `Lua_Client::HasItemOnCorpse` had been fixed. The tell was in plain
      sight: the comment in `client_ext.lua` lists the covered paths and names `Perl_Client_CountItem`
      but not `Perl_Client_HasItemOnCorpse`.
      📌 **`aotv4_same_item` now exists TWICE — `lua_modules/client_ext.lua` and
      `plugins/check_hasitem.pl`. Fix them together.** And when auditing a rule like this, enumerate
      by *mechanism* (who scans slots directly?) rather than by language, or one interpreter's half
      gets checked and the other does not.
    - 📌 **Crafting makes this load-bearing rather than cosmetic.** Every combine output is swapped for
      its Mythic tier (§ the crafting note above), so **25 quest turn-in requirements are craftable
      wearables** whose only obtainable form is now Mythic — if the normalisation broke, those quests
      would be uncompletable rather than merely awkward. Verified 2026-08-01: all 25 have Mythic rows,
      none is touched by the 7 direct-comparison files, and the Lua chain resolves
      `items.check_turn_in` → `trade.self:CheckHandin` → `NPC::CheckHandin` (`npc.cpp:4376`).
  - ⚠️ **Tier offset = 300,000 (step), NOT 1,000,000.** RoF2 item LINKS encode the id in a
    5-hex-digit field masked to `0xFFFFF` (1,048,575, see `common/say_link.cpp`), so an item id
    ≥ 1,048,576 makes its chat link render as garbage (and can desync the client link parser for
    following lines). The old +1M/+2M scheme broke every Mythic link (and Hallowed of base >48,575).
    Base ids top out at 147,494, so the step is **300,000**: Hallowed = base+1×step, Mythic =
    base+2×step, band `[300000,900000)`, all under the ceiling; base recovery is `id % 300000`.
    The step lives in **5 C++ spots** (loot.cpp, npc.cpp, questmgr.cpp, attack.cpp, and
    tradeskills.cpp `AOTV4_TIER_STEP`) — keep them in step with the SQL. Renumbering existing tier
    ids is a one-time `custom/sql/aotv4_tier_renumber.sql` (migrates live inventory/merchant/etc.
    refs; spares the refine bag 2000060).
- **Never run `make zone`** — that Makefile target is NOT a build; it runs a **bare `./zone`** (no
  instance args), which registers with world as an unmanaged static zone ("Zone started with name
  [.] by launcher [NONE]") and **breaks zone routing** (clients time out on enter-world). To rebuild
  the zone binary use `cd /src/build && ninja zone`. If clients can't zone in, check for rogue bare
  `./zone` procs and **exactly one** `eqlaunch` (`pgrep -x eqlaunch`); kill extras + restart world.
- **`small` is a Windows macro** — `<rpcndr.h>` does `#define small char`, so a local named
  `small` (e.g. `HFONT small`) miscompiles into `HFONT char` with a cascade of bogus errors. Avoid
  `small`/`hyper` as identifiers in the dll.
- **AA grant rejected silently** — `GrantAlternateAdvancementAbility(...false)` returns false (no
  message) if any of: class bit, `rank->expansion` vs `UseCurrentExpansionAAOnly`, `level_req`,
  cost > unspent, or unmet `aa_rank_prereqs`. The picker pre-filters on affordability + prereqs so
  only grantable AAs are ever offered.
- **Orphaned zones = "characters time out on login."** If you restart `eqlaunch` while old `zone`
  procs are still alive, those zones become orphans still registered with world; world routes clients
  to a dead zone → login timeout. Tell-tale: many `zone` procs with **pids lower than the current
  `eqlaunch`**. Fix: kill ALL `zone` + `eqlaunch` + `world`, then restart world → eqlaunch (exactly
  one eqlaunch). Prefer **not** restarting eqlaunch at all — kill zones and let the current eqlaunch
  respawn them.
- **Detach long-running procs or a shell timeout kills them.** Start world/eqlaunch with
  `setsid nohup ./world > log 2>&1 < /dev/null &` — a plain `nohup … &` still dies when the foreground
  shell that launched it times out (SIGTERM hits the whole process group).
- **Bazaar seller double-pay** — the native `ServerOP_BazaarPurchase` flow **already pays the seller**
  (`trader_pc->AddMoneyToPP` in the seller's zone). Do NOT also pay in `BuyTraderItemOutsideBazaar`;
  the single authoritative payout is the **world** handler (online → route to seller's zone; offline →
  escrow bucket). See §13.
- **Trader item serials aren't stable across relog** (`GetNextItemInstSerialNumber` is a runtime
  counter). Anything that must survive a relog (e.g. the shop's `item_sn` handle) needs a persistent
  key — the player shop uses a `shopsn_<charid>` counter bucket, not the item serial.

## 11. Discovered-PoK-book travel network

A personal fast-travel system over the Plane of Knowledge books.
- **Discover** — `global_player.lua` `event_click_door`: clicking any door whose destination is
  `poknowledge` calls `pok_travel.discover(zone)` AND **`return 1`** (cancels the door's `HandleClick`
  so the book does NOT teleport you to PoK — `Handle_OP_ClickDoor` skips `HandleClick` when the event
  returns non-zero). Books are pure discovery markers.
- **Ungated** — the PoK books are PoP-era (`doors.min_expansion 4/7/9`), so on a Classic server
  (`CurrentExpansion=0`) they'd be content-filtered out. `custom/sql/aotv4_pok_travel.sql` sets
  `min_expansion=max_expansion=-1` on all `dest_zone='poknowledge'` doors so they always spawn.
  Doors load from the DB at zone boot (NOT shared memory) → restart zones after.
- **Lua** — `pok_travel.lua` + `pok_portals.lua` (gen: short→{id,x,y,z,h,long} for the 33 book-zones).
  Discovered set in bucket `pok_found_<charid>`. Chat protocol: `PORTALDATA short|Long^…` out; `/say
  portalgo <short>` travels (`MovePC`); `/say portalreq` = silent list refresh (dll); `/say portals` =
  list + saylinks (no-mod fallback).
  - ⚠️ **Landing-coord rule (regeneration):** landing = the PoK-side **return** door's dest **when one
    exists** (game-tuned, lands ~by the book). **14 of the 33 zones have NO PoK→zone return book**
    (bazaar, feerrott2, freeportwest, freportw, innothule, innothuleb, mistythicket, shadowrest,
    steamfont, steamfontmts, tox, toxxulia, weddingchapel, weddingchapeldark) — for those the landing
    MUST be the zone's **own book position** (`doors.pos_x/y/z` where `dest_zone='poknowledge'`), NOT
    the zone **safe point**. The generator originally used the safe point → those 14 dropped you at
    map-center; they're hand-corrected in `pok_portals.lua` (each tagged `-- book … (no return door)`).
- **Opening** — clicking a PoK book opens the window: `event_click_door` calls `pok_travel.open`
  which sends **`PORTALOPEN`**; the dll's chat detour sets `g_portalVisible=true`. (So you open the
  menu *at a book* — no hotkey.) `pok_travel.discover` still fires on the same click (attune if new).
- **dll window** (`core_spellwindow.cpp`, `arePortalWindowEnabled`): a 3rd layered window (teal).
  Scrollable list (mouse-wheel + title-bar **Up/Dn** page buttons, `PORTAL_VIS=9`) of discovered
  zones; a row's **Travel** button queues `g_portalPendingShort` → `/say portalgo <short>` on the
  game thread. **Closes** on: right-click, Travel, EQ losing foreground, a **20s idle timeout**
  (`PORTAL_TIMEOUT_MS`, reset on any interaction → covers walking away), or `PORTALCLOSE` which
  `event_enter_zone` sends on any zone change. Swallows `PORTALDATA`/`PORTALOPEN`/`PORTALCLOSE` +
  `portalgo`/`portalreq` echoes.

## 12. Expansion-unlock-on-AA-maxout (era progression) — ⚠️ trigger orphaned 2026-07-25

> `era_system.check_unlock` had exactly ONE caller: `aa_choice.handle_say` (after a successful AA pick).
> Random AA is retired (§6), so **nothing advances `aotv4_era` any more** — the server is frozen at
> whatever era the bucket holds (Classic unless it was already advanced). Everything else in the module
> still works and is still called: `clamp_level` (level cap, from `event_level_up`) and `sync_zone`.
> A replacement unlock condition needs designing — achievements (§15) are the obvious candidate, since
> the AA-maxout test is meaningless once AA is base EQ.


A server-wide progression race: the **first character to MAX every AA in the current era's tier
unlocks the next expansion for EVERYONE** (Classic → Omens of War). Plan/ceilings/cadence live in
`AOTV4_EXPANSION_PLAN.md`. Source of truth = the **GLOBAL** data bucket `aotv4_era` (0..8); every
zone reads it, so gating is consistent and survives restarts.

- **`era_system.lua`** — the era table (`name/expansion/cap/cap_exp` per era 0-8), `current()`,
  `level_cap()`, `check_unlock(client)` (maxout test + advance + `eq.world_emote` broadcast +
  `eq.set_rule` content bump), `clamp_level(client)`, `sync_zone()`.
- **Maxout test** = *every AA with `era <= current` is at its max picker rank* (`aa.mr` in the
  pool). This is authoritative — robust to the picker's floored-cost accounting and escalating
  native rank costs (so `GetSpentAA()` is NOT used as the threshold).
- **`aa_pool.lua`** is now **era-tiered**: 353 AAs (Classic..OoW), all at `pool[1]`, each with
  `era` (0-8 unlock tier) + `mr` (max picker rank). Regenerated from the DB; greedy-filled by
  native expansion to the plan's cumulative ceilings. `aa_choice.gather_affordable` offers only
  `aa.era <= era_system.current()`.
- **Level cap** is enforced LIVE in `global_player.event_level_up` via `era_system.clamp_level`
  (Lua, no rule restart): holds the character at the era cap and pins exp at `cap_exp` so a death
  banks the tuned amount (not an ever-growing pile from grinding at cap). The cap also gates the
  **spell** pool for free (the spell picker only offers <= player level).
- **Detection** is in `aa_choice.handle_say` after a successful pick → `era_system.check_unlock`.
  The first char to max the current tier advances `aotv4_era`; others already past that bar don't
  re-trigger (next ceiling is higher). A single char can cascade tiers across picks (still gated by
  their own death-spending).
- **Live vs restart**: level/spell/AA gating is fully live off the bucket. The **content** side
  (zones/items/doors that filter at zone BOOT via `Expansion:CurrentExpansion`) only fully opens
  after the rule is persisted + world restarted — `aa_choice` bumps the rule live per-zone as a
  best-effort, and `event_enter_zone` calls `sync_zone()`. See `aotv4_aa.sql` step 4.
- **DB**: `aotv4_aa.sql` tags the 353-AA set Bard (`classes|128`) + `grant_only`, lowers
  `level_req 1-70 → 1`, sets `CurrentExpansion=0`. AAs load at zone boot (not shared memory) → a
  **zone restart** applies DB changes. **Reset the race**: `DELETE FROM data_buckets WHERE` key
  `='aotv4_era'`.

> ⚠️ Keep the `aa_pool.lua` generator's WHERE in step with `aotv4_aa.sql` step 2 (same filter:
> enabled, type 1-5, real-named, `expansion 0-8`, first-rank `level_req <= 70`, dedup by name) so
> **pool == tagged set**. An AA in the pool but not tagged Bard → its grant silently fails.

## 13. Player shop (permanent escrow trader) — `/trader` (was `/shop`)

An AFK player shop that works from **any city** (no Bazaar zone, no NPC, no Trader's Satchel), documented
in full in **`HANDOFF.md`**. Items are **escrowed** (they leave your bags when listed → no dupe), searchable
via the native `/bazaar`, delivered by parcel, and paid to the seller on sale (instantly if online, else
banked to next login). Managed by a **three-tab native SIDL window** (`/trader`): **Set Price** (price an
inventory item into a persistent price book + see that item's price history) / **List Item** (choose a
quantity and add a priced item to your shop) / **My Shop** (pull a listing back).

- **Protocol (chat, dll swallows all of it):** `/trader` → dll rewrites to `/say shopopen`; server replies with
  four lines — `SHOPINVDATA slot|itemid|name|vendor|stackqty|bookprice^…` (inventory + its saved price),
  `SHOPBOOKDATA itemid|name|price^…` (the price book), `SHOPLOGDATA itemid|name|old|new|when^…` (price-change
  history, newest first), `MYSHOPDATA itemsn|itemid|name|cost|qty^…` (current listings). dll → `/say
  shopsetprice <itemid>:<copper>` (persist a book price), `/say shopadd slot:qty,…` (escrow; **price comes
  from the book, NOT the command** — an unpriced item is rejected), `/say shoppull <itemsn>` (unlist →
  cursor), `/say shoprefresh`.
- **Price book (2026-07, `zone/trading.cpp`):** per-character `item_id→price` persisted in the
  `shopbook_<charid>` data bucket; a capped (20-entry) change log in `shoplog_<charid>`. `SetItemPrice`
  (upsert + append log), `GetPriceBook`, `GetPriceLog` — all Lua-bound (`lua_client.*`). `GetSellableInventory`
  now carries the book price per row; `AddItemsToShop` reads the price from the book (so `shopadd` sends only
  `slot:qty`). Rationale: you set prices once, then list from them without re-typing. **Needs a zone rebuild.**
- **Server (`zone/trading.cpp`, `Client::` + Lua-bound):** `GetSellableInventory`, `AddItemsToShop`
  (**insert rows FIRST, then delete items** — loss-safe; unique `item_sn` from the `shopsn_<charid>`
  counter because item serials change across relog), `GetMyShopListing`, `PullShopItem`. Rows persist in
  the `trader` table across logout/reboot (searchable while offline).
- **Payout is ONE place — `world/zoneserver.cpp` `ServerOP_BazaarPurchase`:** seller online (any zone) →
  native `trader_pc->AddMoneyToPP` in their zone; seller offline → bank `shop_escrow_<charid>` (paid on
  login by `bazaar_broker.pay_escrow`). ⚠️ **Never also pay in the zone buy path** — that was a 2× bug.
- **Lua:** `quests/lua_modules/bazaar_broker.lua` (backend, no NPC despite the name) + `global_player.lua`
  (`event_say` → `handle_global_say`; `event_connect` → `pay_escrow`).
- **dll (`core_spellwindow.cpp`):** the two-tab window (reuses the coin-field/scroll code); shared
  `DrawCloseX`/`CloseXRect` put a red **[X]** on the Journal/Portal/Shop windows; `IsInGame()`
  (`pinstLocalPlayer` @ `0xDD2630`) gates every overlay `WM_TIMER` so they **auto-hide off char-select**.
  `IsAoTOverlay(fg)` (window-class prefix `AoT`) makes the overlays treat **each other** as the same app
  in that `active` check, so focusing one overlay doesn't hide the others — **multiple overlays coexist**.
  The shop window is draggable across its whole top strip. `/shop` intercept is in `core_bazaar.h`
  (`areTradeAnywhereEnabled=true`).
- **Native trader window ANYWHERE — attempted, NOT viable (2026-07). The `/shop` window is the answer.**
  We tried to open the real RoF2 trader window (`CBazaarWnd`) outside the Bazaar and it **does not render**,
  even though the window *object* exists everywhere. Findings (so nobody re-treads this):
  - `pinstCBazaarWnd` @ `0xD1FCB0` / `pinstCBarterWnd` @ `0xF70CF0` are **non-null in all zones** (runtime
    Ctrl+B vtable dump) — built at UI-load. `/trader`→`0x4EAB40`, `/buyer`→`0x4E6F60` (from `__CommandList`
    @ `0xACD5A8`); the handler has no zone check — it guards `window+0x1D4` then calls the show wrapper
    `vtable[0x90]`=`0x864100` → `CXWnd::Show(1,1,1)` (`vtable[0xd8]`).
  - **But calling Show (via the handler OR directly, bypassing the `[0x1d4]` gate) shows NOTHING outside the
    Bazaar** — TESTED in-game (Ctrl+T and a `/trader` intercept both fail; `/shop` works, so the dll/detour
    is fine). `[0x1d4]` is the window's "active" state (written by Bazaar UI code @ `0x652BBA`/`0x654D28`;
    Deactivate = `0x63ADB0`). The window only renders after `CBazaarWnd::Activate` runs **on Bazaar entry**
    (positions it, wires it into the Bazaar screen). Show just re-shows an already-activated window; outside
    the Bazaar it was never activated, so nothing draws. Replicating that activation is a large, fragile,
    untestable-without-rebuild client patch — **not worth it**, especially since the Bazaar zone may not exist.
  - Leftover harmless code: the `/trader`/`/buyer` intercepts in `core_bazaar.h` (call Show → no-op visually)
    and the Ctrl+T/Ctrl+Y/Ctrl+B diagnostics in core_spellwindow.cpp. Kept as documentation of the dead end.
  - **Resolution:** the custom **`/shop`** window already delivers the full requirement (offline trader, any
    zone, NO Bazaar zone needed, global `/bazaar` search, escrow/parcel payout). It was **restyled to the
    EQ-native look** (2026-07, `VendorPaint`/`VDrawBtn`/new `VBevel` + title-bar in core_spellwindow.cpp):
    gold/tan metallic window frame (dark→bright→mid gold band), near-black background, a **gold "Trader" title
    bar** (`VTBAR_H`=24, embossed strip + gold underline, `[X]` in its top-right), sunken recessed list panel +
    raised header strip, EQ beveled-grey buttons, sunken coin input fields, alternating row bands, cream/gold
    text. ⚠️ **Layout uses shared helpers** (`VTabBtn`/`VUpBtn`/`VField`/`VActBtn`/`VRowY`/`VLIST_Y`/`hy`) for
    BOTH paint and click hit-testing — all include `VTBAR_H`, so the title-bar shift stays consistent; the drag
    region (WndProc) is the only raw-constant spot. `VDrawBtn`/`VBevel` are shared with the search window.
    **Needs a dll rebuild.**
  - **SUPERSEDED by a native SIDL window (2026-07).** The GDI restyle only *imitates* EQ chrome; the real fix
    (asked "why not use the window XML as reference?") is a **native `CCustomWnd`** — same approach as the
    achievement window (§15), which renders with the client's own UI engine so it looks truly native AND
    works in any zone. Built from the reference XMLs the user supplied (`/src/EQUI_BazaarWnd.xml` etc.):
    - **`EQUI_ShopWnd.xml`** (`aotv4_client_install/uifiles_default/`) — Screen `item="ShopWnd"` (`WDT_Def`
      frame, Titlebar/Closebox, title "Trader"), a `Listbox` (`WDT_Inner`, cols Item/Qty/Price), 4 coin
      `Editbox`es, buttons Set Price / Add to Shop / Pull Item / Refresh + Add-Items/My-Shop tabs.
    - **`ShopWnd : CCustomWnd`** (core_spellwindow.cpp) — REUSES the GDI backend (same `g_vendor*`/`g_my*`
      state + `VendorQueue`/`VendorDoAdd`); `WndNotification` maps clicks → the same `/say shopadd/shoppull/
      shoprefresh`. `EnsureShopWindow` wires `ppSidlMgr`/`ppWndMgr` from client globals (the §15 fix).
      `SHOPINVDATA` now calls `ShopSidlShow()` (was `g_vendorVisible=true`); `MYSHOPDATA`→`ShopWndRefreshIfOpen`;
      teardown `ShopWndOnUiReset()` is called from the achievement `CleanGameUI`/`ReloadUI` detour (it owns
      those). Old GDI overlay stays in the dll but is never shown.
    - ⚠️ **SIDL editbox I/O gotchas (learned the hard, crashy way) — THREE traps:**
      1. **Raw struct member offsets are WRONG for this client build.** Accessing `((CEditWnd*)w)->InputText`
         reads empty and `w->WindowText` reads a garbage pointer → `GetCXStr` on it **CRASHES**. `EQClasses.h`
         member offsets don't match this RoF2 build. Only **address-mapped METHODS** (`FUNCTION_AT_ADDRESS`) are
         reliable. So **read** an editbox via the method `w->GetWindowTextA()` (`0x411190`) and pull chars from
         the returned `CXStr` (which wraps one `CXSTR*`): `CXStr s = w->GetWindowTextA(); PCXSTR raw =
         *(PCXSTR*)&s; if (raw) GetCXStr(raw, buf, n);` (`ShopGetInt`). **Set** labels via `SetWindowTextA`
         (`ShopSetLabel`). Don't pre-fill editboxes (and `CXStr::operator char*` isn't address-bound — never cast).
      2. **Clicking a coin field clears the Listbox selection**, so at button-click `GetCurSel()` is −1 and any
         "apply to selected row" no-ops (symptom: Set Price never changes the price). Remember the row on
         list-click in `m_sel` and use `SelRow()` (single-item fallback), NOT live `GetCurSel()`.
      3. `/echo` is NOT a valid EQ command (that's an MQ2 thing) — for in-window feedback use `WriteChatColor`
         (MQ2Main.h / MQ2PluginHandler.cpp is in the build) or set a label; `/echo` returns "invalid command".
    - **Stackable quantity (2026-07):** `Client::AddItemsToShop` (trading.cpp) lists a **partial stack**
      (`item_charges = list_qty`; `DeleteItemInInventory(slot, del_qty)` where `del_qty` 0 = whole stack) so you
      can list N of a stack and keep the rest. Client shows real Qty + has a `SHW_Qty` field (default = full
      stack); state in `g_vendorStack`/`g_vListQty`/`g_myQty`. Field splitting is `VSplitN(s, out, n)` (the shop's
      old `VSplit5` was renamed — a duplicate def collided with the loot window's `VSplit5`). Buyer still
      purchases the whole listing (per-unit buys would be a bigger change). **Needs a zone rebuild + dll rebuild.**
    - **Three tabs (2026-07, `ShopWnd` in core_spellwindow.cpp):** `g_vendorTab` 0/1/2 = Set Price / List Item /
      My Shop (`SHOP_TAB_*`). One shared `SHW_ItemList`; `ApplyVis()` shows/hides each tab's controls via
      `CXWnd::Show`. Set Price → `shopsetprice`; List → `shopadd slot:qty` for the selected row (`VendorDoAdd`);
      Pull → `shoppull`. History listbox `SHW_HistList` is filtered to the selected item's id (`FillHistory`),
      relative "when" via `ShopWhenStr`. EQUI_ShopWnd.xml is CX564×CY600. **Needs a dll rebuild.**
      - ⚠️ **The "keep it to ONE listbox" and "no native tabs" rules recorded here were WRONG** — see
        §16's corrections. A second listbox failed once because that XML was malformed, not because
        the client forbids it (`EQUI_AoTSpellBook.xml` ships four), and the hand-rolled toggle-button
        tabs were unnecessary: `TabBox`/`Page`/`TabText` are stock and are what the AA window uses
        (§6). Both windows are worth rebuilding on the real widgets when they are next touched.
    - **Command: `/trader`** (core_bazaar.h) routes to `/say shopopen` → server SHOPINVDATA → `ShopSidlShow`.
      The old `/shop` and `/buyer` intercepts were **removed** (the native trader/buyer windows can't render
      standalone).
    - **Client install:** `aotv4_client_install/SHOP_WINDOW_INSTALL.md` — copy `EQUI_ShopWnd.xml` to
      `uifiles/default/` + `<Include>` it in `EQUI.xml`, rebuild the dll. Like the achievement window, expect
      a possible iteration to nail the SIDL wiring.
- ⚠️ **Bazaar search MUST be global — `common/bazaar.cpp` `Bazaar::GetSearchResults` (2026-07 fix).** Escrow
  sellers list from ANY zone (the `trader` row's `char_zone_id` is wherever they stood — never a single Bazaar
  zone), so the STOCK scope filters break trader-anywhere: `Local_Scope` restricts to the searcher's zone, and
  the `else if (trader_id > 0)` branch restricts to one `char_id`. RoF2 sends `Local_Scope`/`AllTraders_Scope`
  and populates `trader_id` from the selected trader, so **stock scoping shows each player only their own /
  their-zone listings** (symptom: "others can't see my listing; needs a relog"). Fix: for the non-alternate
  path, add **no** trader/zone filter — `search_criteria_trader` stays `"TRUE"` → all listings match (item
  name/type/cost filters still apply). Keep the NonRoF per-trader-inspect path + the (rule-gated, off) shard
  path intact. The buyer's trader **list** was already global (`GetDistinctTraders`, no zone filter when
  `UseAlternateBazaarSearch=false`). **Needs a zone rebuild** (bazaar.cpp is in `libcommon`).
- **Live visibility on list** is already wired: `AddItemsToShop` calls `SetTrader(true)` +
  `SendBecomeTraderToWorld(this, TraderOn)` → world fans `ServerOP_TraderMessaging` to every zone
  (`ZSList::SendPacket`) → each zone pushes `AddTraderToBazaarWindow` to all online RoF2 clients. So a listing
  appears in others' open bazaar windows without a relog — but the buyer must still click **Search** (the
  bazaar has no passive item feed). This only *worked* once the global-search fix above unmasked it.
- ⚠️⚠️ **The `trader` table is PERMANENT escrow storage — NEVER truncate or wholesale-delete it (2026-07).**
  A listed item is removed from the player's satchel and lives ONLY as its `trader` row, so any stock code
  that clears the table destroys REAL items (not a runtime cache like on live EQ). Five stock deletion paths
  were closed — do not reintroduce them:
  1. **`Database::ClearTraderDetails`** (`common/database.cpp`) — ran `TRUNCATE TABLE trader` on **every world
     boot** (`world_boot.cpp`), wiping ALL shops on any restart. Now only clears the transient
     `active_transaction` lock.
  2. **`Client::TraderEndTrader`** (`zone/trading.cpp`) — the stock `!persist_listings` branch did
     `DeleteWhere(char_id)`; it's called on **zoning, camp, inventory moves, invalid-item, …** (default flag
     = delete). Delete is disabled — end-trader only flips live state now.
  3. **Trader's-Satchel move** (`zone/inventory.cpp`) — moving any item in/out of a satchel `while IsTrader()`
     ended trading (→ deleted). The whole block is removed: escrowed items aren't in the satchel, so staging
     the next item to list must not disturb the shop.
  4. **Native satchel relist** (`TraderStartTrader`) / **price=0 unlist** (`UpdateTraderItemPrice`, zonedb.cpp)
     / **Bazaar-zone cycle** (`CheckToClearTraderAndBuyerTables`, entity.cpp) — dormant native-window paths,
     defensively neutered so they never wipe escrow.

  Listings are removed by EXACTLY three legit paths: **unlist (`PullShopItem`), a sale, and login
  reconciliation (`ReclaimOfflineShop`)**. All fixes are **world + zone** binaries → need a restart/cutover.
- **Do NOT resurrect** (removed on request): the Bazaar Broker NPC (2000050), the Shopkeeper stand-in NPC
  (2000051), the Trader's-Satchel `vpset/vshop` flow, or `Bazaar:UseAlternateBazaarSearch` (keep it false —
  it's Bazaar-zone-sharded, the opposite of trader-anywhere).

## 14. All classes unlocked (was Bard-only) — 2026-07 pivot

Everyone used to be forced to Bard; now all 16 classes are playable, with the 4 pure-melee classes made
into casters. Three layers had to agree (like combat skills, §4):

**(a) Client — `core_allcasters.cpp/.h`.** Its OWN translation unit, OWN detours (not piggy-backed on the
spell-window/tradeskill hooks), gated by `areAllClassesCasters` (`_options.h`). RoF2 image base `0x400000`,
addresses rebased at runtime. Four detours:
- `EQ_Character::IsSpellcaster` (**0x443F50**) → always **1**: spellbook + spell-gem bar + casting for every
  class (RoF2 hides them for Warrior/Monk/Rogue/Berserker via a class→bool table @ 0x443f90).
- `EQ_Character::Max_Mana` (**0x581E60**) → a melee `0` return becomes `level * AOTV4_MELEE_MANA_PER_LEVEL`
  (**=40**), so the mana VALUE scales like a hybrid. **Must match the server formula** (see (b)).
- mana-gauge predicate (**0x59FB90**) → always **1**: `CPlayerWnd::Draw` (0x718cf0) shows the mana GAUGE
  (`CPlayerWnd+0x22c` "PlayerMana") only when this class→bool predicate is non-zero — forcing 1 renders the
  blue bar for melee. (A SECOND caster test, distinct from IsSpellcaster.)
- `EQ_Spell::IsBardSong` (**0x432960**, `bool __stdcall(SPELL* spell, int class)`, `ret 8`) → skill-gated:
  mirrors the server's `IsBardSong` so the **reward spells cast as normal spells, not songs, even for a Bard**.
  Stock logic = "caster is a Bard AND the spell has a Bard level" (never checks skill); our reward spells
  carry a Bard level (so a Bard can memorize them) but use **skill 98**, a non-song placeholder. Detour keeps
  the stock result but only genuine song skills stay songs: **Brass 12, Singing 41, Stringed 49, Wind 54,
  Percussion 70** (SPELL::Skill is a BYTE @ **+0x270**; per-class level array @ +0x246, Bard @ +0x24e).

### ⚠️⚠️ A NON-BARD SINGING A REAL BARD SONG — the song machinery is class-gated (fixed 2026-08-05)
`Client::CastedSpellFinished` gates the entire bard-song block on **`GetClass() == Class::Bard`**
(`zone/spells.cpp:1471`). Correct on live, where only a Bard can hold a song — but the reward pool
hands genuine songs to all sixteen classes here, and a non-Bard fell into the **else** branch:
- **NO PULSE.** `bardsong` / `bardsong_timer` were never set, so the song was cast **once**. Bard
  songs carry a deliberately tiny `buff_duration` (a few ticks) *precisely because* a Bard re-pulses
  them every 6 seconds — so it faded almost immediately and never refreshed. Reported from play as
  *"bard songs are not being consistently sung by non bards"*.
- **MOVEMENT INTERRUPTED IT**, because the else branch is the channel / regain-concentration check,
  while an actual Bard may move freely while casting anything.
- Fixed by keying the block on the **SPELL** rather than the caster: `GetClass() == Class::Bard ||
  IsBardSong(spell_id)`. The pulse tick itself (`client_process.cpp:221`) was never class-gated — it
  only wants `bardsong != 0` — so it starts working as soon as that is set.
- ⚠️ **Do NOT widen this to "has a Bard level".** `IsBardSong` is already SKILL-gated (see the note
  above: Singing/Percussion/Stringed/Wind/Brass), which is what keeps the repurposed reward spells
  (skill 98) behaving as normal spells. Widening it turns every reward spell into a song.
- 📌 **`GetInstrumentMod` is STILL Bard-only** (`client_mods.cpp:1464` returns a flat 10 for anyone
  else), so a non-Bard sings at baseline power with no instrument multiplier — even holding an
  instrument and with the skill capped, both of which this server grants to everyone. That is a
  separate *power* question from the *consistency* bug fixed here, and was left alone deliberately.

**(b) Server mana.** `zone/client_mods.cpp CalcMaxMana`: the pure-melee else-branch = `GetLevel() * 40`.
Keep this constant in step with the dll's `AOTV4_MELEE_MANA_PER_LEVEL` so the client gauge max == the server
mana ceiling. (Server + client use different stat formulas; a shared `level*constant` keeps them in lockstep.)

**(c) Character creation.** `char_create_combinations` restored to **all 16 classes** (was class-8-only) —
the world loads it at boot to both drive the create-screen UI (greying out disallowed classes) and validate
create requests. Source: `custom/sql/aotv4_all_classes_creation.sql` (replaces the retired
`aotv4_bard_only_creation.sql`). **World restart** required (combos load at world boot). The Bard-force block
in `global_player.event_connect` is removed — characters keep their created class.

### Custom reward spell set (43000-43149 offered + 43150+ helpers)
- Generated by `.devcontainer/custom/spells/gen_spells.py` from `spell_design.csv` (score-based auto-tiering
  across levels 1-30, `LEVEL_CAP=30`). Spells use **skill 98** (a non-song placeholder; server `IsBardSong`
  is already skill-gated). Class levels opened to all 16 classes (offer level = `classes8`).
- ⚠️ **Renumbered 50xxx → 43xxx** (RoF2 caps spell LINKS and the spellbook packet at **id < 45000**; the
  custom set was originally at 50000+). Trigger/proc refs live in `effect_base_value`/`effect_limit_value`
  and must be renumbered too (Moonfire's heal @ limit2, Firefist/Chi Block @ limit1 were missed once).
- ⚠️ **`gen_spells.py` is DESYNCED from the live DB** — still emits 50xxx and lacks the live fixes below.
  **Do a source-sync before ever regenerating** or it reverts everything. (Memory: `gen-spells-source-sync`.)
- **Two description sources, keep in sync:** the level-up **picker** reads `spell_desc.lua` (server sends it
  via `SPELLDESCDATA`); the in-game **spellbook** reads `db_str` (via `descnum=id`). ⚠️ A literal `%` in a
  description renders as garbage (`0:00:00`) — the client treats `%` as a format token; spell out "percent".
- **Descriptions live in:** `spell_design.csv` (master) → `spell_desc.lua` (gen) → `db_str` (live). Client
  needs `spells_us.txt` (class levels + cast times) + `dbstr_us.txt` (descriptions) re-exported +
  installed; the current bundle is dropped at `C:\AoTv3\AoTv4\aotv4_client_install` (== `/src/aotv4_client_install`).

### Custom melee mitigation (why "deflected by your armor" spams)
`zone/attack.cpp Mob::MeleeMitigation` (~1063) replaces stock AC mitigation with an **AC-vs-offense roll**
(`AoT:Mit*` rules). A full-mitigation roll (`rolled_mit >= 1.0`) zeroes damage and prints **"…was deflected
by your armor!"** (line 1156 — uses `GetName()`, the raw `a_cave_rat009`; should be `GetCleanName()`). A
high-AC char vs a weak mob deflects nearly every hit → message spam (it's on the filterable `OtherMissYou`
channel). Rune/mitigation buffs (SPA 55/78/161/162/163) DO still apply — via `ReduceDamage` (line 4331,
after MeleeMitigation). ⚠️ **% mitigation is imperceptible vs the tiny post-mitigation numbers** (`3 * 1/100 = 0`);
for low-level buffs prefer a **flat per-hit cap** — SPA 162 `base=100, limit=N` subtracts exactly N/hit
(how Passive Protection was fixed). Damage shields (SPA 59) use a **negative** base (native convention).

## 15. Achievements + class-aura rewards — 2026-07

A DB-defined **achievement system** (ported/adapted from the `Barathos/EQEmu-feature-achievements` fork)
whose rewards can **scribe a class aura**. The client shows a **native SIDL window**; the server owns all
progress, completion, and validation. Same "server → `ACH|...` chat line → dll → window" pattern as our
other overlays (§3/§6/§11/§13).

**Server (`zone/achievement_manager.cpp/.h`, singleton `achievement_manager`).** DB-defined objectives
(`level`/`zone_visit`/`task_complete`/`skill`/`*_kill`) tracked per character; on completion, rewards queue
(auto-claim grants immediately). Gameplay hooks call it: `attack.cpp` `NPC::Death` (3 kill sites),
`client.cpp` `SetSkill`, `exp.cpp` `SetLevel`, `client_packet.cpp` `CompleteConnect`, `task_client_state.cpp`
`IncrementDoneCount`. `#ach [window|status|categories|category|detail|rewards|claim|check]` command
(`gm_commands/achievements.cpp`). **AoTv4 additions:** an **`any_kill`** objective type (kill anything) and
**`class_mask` enforcement** in `ProcessMatchedObjectives` (`GetPlayerClassBit`, `0`=any) so per-class
achievements only advance for that class; a **`scribe_spell`** reward type in `AwardQueuedReward` (reward_id
= a spell id → scribes it into the spellbook; idempotent).
- **DB**: `custom_achievement_*` tables via `common/database/database_update_manifest_custom.h` (custom
  migrations v1-4); `common/version.h` `CUSTOM_BINARY_DATABASE_VERSION = 4`. **World applies the migration
  at boot** (not zone). Reward types: title_text/suffix/set, item, currency, coin, live_item_request,
  **scribe_spell**.
- ⚠️ **Migration pre-backup needs DB creds.** World auto-dumps the DB before applying migrations via
  `mysqldump --defaults-extra-file=login.my.cnf`; it builds that cnf from **`content_database`** creds
  (`IsDumpContentTables`). If `content_database` is absent/blank, the cnf gets empty creds → mysqldump
  auths as the OS user → **"Access denied" → migration aborts**. Fix (done): a `content_database` block
  (peq/peqpass) in `eqemu_config.json`. One-off unblock: write `login.my.cnf` (`[mysqldump]` peq creds)
  **read-only** so `BuildCredentialsFile` can't overwrite it.

**The 16 class auras (the reward).** One per class, granted by "*Class*: First Blood" (kill anything **as
that class** → scribe that class's aura). Auras (§ generic mechanic): a **cast spell** (SPA 351, self, spawns
the aura) + an **`auras` row** (cast → aura NPC → effect spell, `aura_type=1` OnAllGroupMembers = **group-
shared**, distance 60) + an **effect spell** (the group buff). Our set: cast spells **43500-43515**, effect
spells **43550-43565**, aura NPCs **2000100-2000115**; achievements **9000001-9000016** (category 9000),
objectives `any_kill` + class_mask bit, rewards `scribe_spell` → the class's cast spell. **1-active is the
default** (`GetAuraSlots()` = base 1; never grant `aura_slots` bonuses). Generated by
`/tmp` scripts (clone templates 8468/8469/2000003); to regen see `custom/spells` history.

**Client dll (`core_achievements_native.cpp/.h`, flag `areAchievementsNativeEnabled`).** The reference was a
standalone dll that re-detoured `dsp_chat`/`InterpretCmd` — **which we already own** — so it was refactored
to `.cpp`+`.h` (single definition across our TUs) and **routed through our existing detours**: our
`dsp_chat` calls `NativeAchievementParseTransport` (swallow `ACH|`) + swallows the `#ach` echo; our
`InterpretCmd` (`core_bazaar.h`) calls `NativeAchievementHandleLocalCommand` + `NativeAchievementRewriteCommand`
(`/ach …` → `/say #ach …`). `InitAchievementsNative` installs ONLY the `CDisplay::CleanGameUI`/`ReloadUI`
detours (window lifecycle) — conflict-free only because `isMQInjectsEnabled=false` (else MQ2's CleanUI hook
takes those addresses). The window is a real SIDL `CCustomWnd` (`NativeAchievementWnd`), created on demand.
- ⚠️ **This dll had NEVER used native SIDL windows** (all our overlays are self-drawn GDI), so the MQ2 UI
  managers `ppSidlMgr`/`ppWndMgr` weren't reliably wired — `CCustomWnd`/`NativeAchievementEnsureWindow`
  silently returned at `if (!pSidlMgr || !pWndMgr)` and **the window never appeared** (log stopped at
  `ACH|window|show` with no "creating achievement window"). Fix: in `NativeAchievementEnsureWindow`, set
  them directly from the client globals if unset — `ppSidlMgr = (CSidlManager**)((0x15D3D08-0x400000)+baseAddress)`,
  `ppWndMgr = (CXWndManager**)((0x15D3D00-0x400000)+baseAddress)`. Any future native-window feature in this
  dll must do the same. `native_achievements.log` (in the EQ root) is the dll's own debug trace — the fastest
  way to diagnose the window (it logs every `ACH|` line received + the `EnsureWindow` pointer values).
- **Client install** (see `aotv4_client_install/ACHIEVEMENT_WINDOW_INSTALL.md`): rebuild OUR `dinput8.dll`;
  copy `EQUI_NativeAchievementWnd.xml` + `Achievement_*.tga` to `uifiles/default/`; add
  `<Include>EQUI_NativeAchievementWnd.xml</Include>` to `EQUI.xml`. The `#ach` **text** commands work
  without any client change; the UI files are only for the native window.

**Filling out the categories (2026-07).** The seed (from the fork) populates Classic→PoP well but left header
categories showing 0/0 and later in-scope tiers empty. Fixes:
- **Parent-count rollup (code):** `LoadCategorySummaries` (`achievement_manager.cpp`) now rolls child-category
  achievements up onto their parent (`a.category_id = c.id OR a.category_id IN (children of c, enabled)`), so
  **Exploration/Hunter/Slayer/Progression/Tradeskill** headers show the sum of their leaves (277/136/385/490/72)
  instead of 0. Leaves have no children so they stay direct-only. **Needs a zone rebuild + restart.**
- **Beyond-OoW hidden (SQL, `custom/sql/aotv4_achievements_fill.sql`):** the era system caps at OoW, so the
  Dragons-of-Norrath..Rain-of-Fear Exploration (cats 209-219) + Hunter (3309-3319) categories are set
  `enabled=0` (unreachable zones; reversible if the server ever extends). `LoadCategorySummaries`'
  `WHERE c.enabled=1` drops them from the window.
- **Server Custom (cat 8), same SQL file:** class-agnostic **kill milestones** (`any_kill` 100/1k/10k/50k →
  Bloodthirsty/Warmonger/Harbinger/Reaper). `any_kill` counts every kill across the char's life (survives the
  roguelite death reset). **NB: a character is ONE class**, so an "all 16 auras" achievement would be
  unearnable — Class-Mastery auras are per-class only.
- **GoD/OoW Hunter (generated, `custom/sql/aotv4_achievements_hunter_god_oow.sql` via `/tmp/…/gen_hunter_god_oow.sh`):**
  cats 3307/3308 filled from **OUR DB's named mobs** (not Live lists — those wouldn't match our spawns). Named =
  npc_types whose name starts `#` (excl. `#Trigger`/`#Zoner`/level-99 system NPCs); `a_`/`an_`/lowercase = trash.
  One `npc_name_kill` objective per distinct clean-name (top 12 by level), `zone_id` = the sub-zone it spawns in.
  ⚠️ **`npc_name_kill` matches `o.zone_id == <zone killed in>` AND `LOWER(o.target_name) == LOWER(GetCleanName())`**
  — `GetCleanName`==`CleanMobName` (`common/strings_legacy.cpp`): `_`→space, keep `[A-Za-z`+backtick]`, **strip
  `#`/digits/apostrophes**, then we LOWER. bonzz.com gave the canonical zone list + target counts but **no mob
  names** (they live on eqresource/raidloot) — irrelevant since we source earnable targets from our own spawns.
- **Epics (cat 7, `custom/sql/aotv4_achievements_epics.sql`):** 16 achievements (one per class, epic 1.0
  weapon), completed by a **new `item_receive` objective type**. `ProcessItemReceive(client,item_id)`
  (`achievement_manager.cpp`) matches `item_receive`+`target_id`; **hooked live** in `Client::SummonItem`
  (quest turn-ins/summons) + `Client::PushItemOnCursor` (loot→cursor), and **backstopped** by
  `ProcessItemInventory` in `RecheckAutomatic` (credits already-owned epics on level/zone/#ach). class_mask
  = `1<<(class-1)` gates each to its class. Epic item ids resolved from OUR `items` DB (the weapons that
  actually exist here). **Needs a zone rebuild.**
- ⚠️ **Achievement-id ranges are NOT category-bound — check before inserting.** Zone Slayer occupies
  **700001-700448**; a first pass put Epics at 700001-700016 and `ON DUPLICATE KEY UPDATE` silently hijacked
  16 Zone Slayer rows (their `zone_kill` objectives dangled). Epics moved to **710001-710016** (objectives
  71000001+). Other ranges in use: Character 1xxx/110xx, Exploration 200xxx, Task 300xxx, Tradeskill 46xxxx,
  Creature Slayer 5xxxxx, Zone Slayer 7000xx-7004xx, Server Custom 8000xx, Meta 900xxx, Class Mastery 9000xxx,
  GoD/OoW Hunter 3307xx/3308xx. **Every category is now populated** (Epics 16, Exploration 277, Hunter 136,
  Slayer 385, Progression 490, Tradeskill 72, Server Custom 4, Character 22, Class Mastery 16, Meta 4).
- **Parent headers drill down (code):** the dll shows categories as a flat list with children indented; a parent
  (Hunter/Exploration/…) has NO direct achievements, so selecting it used to show an empty list. `LoadAchievements`
  now returns the union of the selected category + its enabled child categories (grouped by `category_id`), so
  clicking "Hunter" lists all 136. Leaves are unaffected (no children).
- ⚠️ **Live-refresh must stay small — chat-burst drops (fix).** `PushLiveUpdate` originally re-sent ALL ~40
  `ACH|category` lines per completion; the RoF2 chat pipe drops/reorders that burst, losing the trailing
  `ACH|achievement_state` row-flip → "the window updates only sometimes." Now it sends the **state flip first**,
  then only the **affected leaf + its parent** (1 status + 1 state + ≤2 category = ~4 lines). Any per-completion
  push here must stay minimal for the same reason. (A big parent drill-in still sends ~N achievement lines, same
  load as the pre-existing 490-row Task Completion leaf — if a huge list ever truncates, batch it.)
- **Native EQ-menu "Achievements" button is intentionally NOT hooked** — it opens RoF2's stock `CAchievementsWnd`
  (which we never populate), not our window. Redirecting it needs RE of the client's native-open call; by choice
  the window is opened via `#ach`/`/ach` instead.

## 16. Advanced Loot window (`/advl`) — 2026-07

A native SIDL **Advanced Loot** window in **complement mode**: the stock RoF2 loot window still opens
(`MakeLootRequestPackets` is untouched); ours lists the SAME corpse and adds **Loot / Leave / Never**,
**Loot All**, and a persistent **Never** filter list. Same "server → chat line → dll → window" pattern as
§3/§6/§11/§13/§15.

> ⚠️⚠️ **READ §31 BEFORE TOUCHING ANY LOOT PATH.** Individual loot (2026-08-01) rolls the table once
> per player and stamps an owner on every item, which changes three things this section assumes:
> lootslots are numbered **per player** (not once per corpse), `MakeLootRequestPackets` is **no longer
> untouched** (complement mode was the hole — the stock window showed everyone's drops), and the
> need/greed/sell layer in `advloot.cpp` is **dead** while `AoT:IndividualLoot` is on. Any new path
> that reads a lootslot or acts on a corpse item needs the guards described there.

- **Protocol (chat, dll swallows all of it):** server → `LOOTDATA <n>^lootslot|itemid|icon|name|npcname|qty^…`
  (pushed from `Handle_OP_LootRequest`; `qty` = stack size for stackables else 1, appended **last** so an
  older 5-field dll parse still works), `FILTERDATA <n>^itemid|icon|name|rule^…`, `LOOTCLOSE`
  (from `Handle_OP_EndLootRequest` + `event_enter_zone`). dll → `/say alspick <lootslot> loot|leave|never`,
  `/say alslootall`, `/say alsrefresh`, `/say alsfilters`, `/say alsfilterdel <itemid>`.
- **Server (`zone/client_packet.cpp`, `Client::`):** `SendAdvLootData` / `SendAdvLootFilters` /
  `SendAdvLootClose` / `AdvLootSlot` / `HandleAdvLootSay`. The `/say als*` commands are intercepted in
  **`Client::ChannelMessageReceived`** (`zone/client.cpp`, before EVENT_SAY/broadcast) so they never spam
  chat or reach quests — **NOT** in Lua, unlike the other windows. **Needs a zone rebuild.**
- ✅ **Loot / Loot All are HIDDEN while the group owns the decision** (2026-07-28). **Loot** goes when
  the *selected* row is still votable (`RowIsVotable` — a "Rolling N" status, not `Yours`/`Free Grab`/
  `Won by`); **Loot All** goes for the whole session under any non-FFA `g_lootMode`, since it would
  sweep contested rows along with settled ones.
  - ⚠️⚠️ **This is PRESENTATION, not enforcement.** `AdvLootManager::CanLoot` (`zone/advloot.cpp`)
    already refused both cases server-side *with a reason* — racing the button never worked, it just
    LOOKED like it should, and a live button that silently does nothing reads as a broken window
    rather than as a rule. **Never move the check to the client**: a modified dll would just un-hide.
  - ⚠️ **HIDDEN, not greyed.** There is no address-mapped enable setter on this build — `eqgame.h`
    maps nothing for it and `EQClasses.h` only *declares* `CXWnd::IsEnabled` — and writing a raw
    `->Enabled` member offset is the unreliable-struct-offset trap from §13. `Show` **is** mapped.
  - ⚠️ **FFA is deliberately untouched**: there the server permits looting and first-come *is* the
    rule, so hiding the button would break ordinary group play rather than protect it.
  - ⚠️ `ApplyVis` now takes the selected row as a parameter and the list's click handler passes
    `m_sel` — `GetCurSel()` is one click behind inside that notification, so a live read would briefly
    offer Loot on a row that is still rolling. Safe to call from there because `ApplyVis` touches only
    buttons and labels, never the listbox (rebuilding a list inside its own click is the §3 Death Book
    crash).
- ⚠️ **Looting reuses the NATIVE `Corpse::LootCorpseItem`** — `AdvLootSlot` synthesizes an `OP_LootItem`
  packet and calls it, so lore/cursor/weight/corpse-removal are the stock checks and there is **no bespoke
  item-grant path** (the lesson from the escrow work, §13). It only works while the corpse is actually being
  looted (`LootCorpseItem` requires `IsBeingLootedBy`) — which is why this is complement mode, not a
  standalone accumulating loot list.
- **`lootslot` is the handle**, not an entity id (the dll's `g_lootEid[]` is historically misnamed).
  `Corpse::RemoveItem` does **not** renumber lootslots, so `alslootall`'s slot snapshot stays valid while
  it loots down the list.
- ⚠️ **Two traps when driving `LootCorpseItem` from a server-side batch (both fixed; don't reintroduce):**
  1. **The 10 ms loot throttle is per zone TICK.** `Corpse::m_loot_cooldown_timer(10)` is checked against
     the global `current_time`, which does **not** advance inside one packet handler — so in a tight loop
     the first `Check()` passes and resets it and **every later item fails**, each failure calling
     `SendEndLootErrorPacket` + `ResetLooter()` (Loot All would loot one item and kill the loot session).
     `Corpse::ResetLootCooldown()` (corpse.h, `Timer::Trigger()`) is called between items. Safe because
     `LootCorpseItem` removes the item from the corpse, so a slot can't be looted twice.
  2. **`auto_loot` must be 1.** With `0`, `LootCorpseItem` does `PutLootInInventory(slotCursor,…)` — every
     item piles on the **cursor**, and `Character:CheckCursorEmptyWhenLooting` (default **true**) then
     refuses the next loot *and* calls `ResetLooter()`. With `1` it uses `AutoPutLootInInventory` (free bag
     slot, cursor only as fallback). `alslootall` additionally **stops cleanly** if the cursor is occupied.
  The synthesized `OP_LootItem` carries a **server-space** lootslot; the ACK `QueuePacket(app)` runs through
  `ENCODE(OP_LootItem)` (`ServerToRoF2CorpseMainSlot`), so it keeps the native loot window in sync.
- **Never list** = data bucket `alsnever_<charid>` (CSV of item ids). A filtered item is only **omitted from
  LOOTDATA** — it stays on the corpse and still shows in the native loot window. Nothing is destroyed.
- **dll — its OWN translation unit `core_advloot.cpp/.h`** (2026-07-25; was inside `core_spellwindow.cpp`),
  like `core_allcasters` / `core_achievements_native`. Flag `areLootWindowEnabled` → `InitAdvLoot()`.
  It installs **NO detours**: the dll already owns `dsp_chat` / `InterpretCmd` / `ProcessGameEvents` and two
  modules can't hook the same address, so those call in via `AdvLootParseTransport` (LOOTDATA/FILTERDATA/
  LOOTCLOSE), `AdvLootIsOurEcho` (`/say als*` echoes), `AdvLootShow` (`/advl`, from `core_bazaar.h`) and
  `AdvLootOnUiReset` (from the `CleanGameUI`/`ReloadUI` detour `core_achievements_native.cpp` owns).
  `AdvLootWnd : CCustomWnd` over `EQUI_AdvLootWnd.xml`, with the same `ppSidlMgr`/`ppWndMgr` wiring fix as
  §15. **Static layout only** and **one listbox** (§13). The game-thread command queue is shared as
  `AoTQueueGameCommand()` (`VendorQueue` is now a wrapper on it).
  ⚠️ New `.cpp` files **must be added to `eq-core-dll-vs2022.vcxproj`** or they silently don't compile.
  The old **GDI** loot overlay was deleted (~300 lines) — it had been dead since the SIDL window landed.
- ✅ **CORRECTIONS (2026-07-25), proven by the reference sources in `src.rar` + a binary probe.** Three
  "limits" recorded here were wrong and cost real work:
  - **Native TABS exist.** `TabBox` + `Page` + `TabText` + `TabBorderTemplate`/`PageBorderTemplate` are
    all in RoF2's parser, and `FT_DefTabBorder`/`FT_DefPageBorder` are in our `EQUI_Templates.xml`.
    The Advanced Loot window hand-rolls tabs with a toggle button because this was not known.
  - **MULTIPLE Listboxes per window are fine.** `EQUI_AoTSpellBook.xml` ships four (list + detail,
    twice). The "one listbox" rule in §13 came from a malformed shop XML misdiagnosed as a hard limit;
    it forced the loot Filters view into one shared list with permanently mislabeled headers.
  - **`CListWnd::GetItemData` works** (`spellList->GetItemData(sel)` in the reference). §16's claim
    that it returns 0 for every row was a misattribution: the real fault was unassigned `lootslot`
    values aliasing every handle onto one item (see `Corpse::AssignLootSlots`).
  - Still genuinely unavailable on this build: `GetCurCol`, the `(point,row,col)` `GetItemAtPoint`
    overload, `SetColumnLabel`, and `CTextureAnimation::SetCurCell` (so no per-row item icons).
  - **Verify before believing a limit:** `strings -n 4 eqgame.exe | grep -qx "<Tag>"` answers whether
    the SIDL parser knows a tag. Validate the method with a nonsense tag first.
- ✅ **ALWAYS run `bash aotv4_client_install/validate_ui_xml.sh` before copying any `EQUI_*.xml` into
  the client.** It checks the four things that have actually broken us: `--` inside a comment, tag
  balance, undefined `<Pieces>` targets, and modern-only tags (`<Sortable>`, column `<Tooltip>`) that
  no RoF2-shipped file uses. The `--` bug has cost TWO sessions — the second time it was written into
  a comment that was itself warning about the bug. Do not trust a visual scan.
  (`<Pieces>` also accepts a type-qualified `Screen:NAME` form — the validator strips that prefix.)
- ⚠️ **A `--` inside an XML comment CRASHES the client at UI load** (bit us on `EQUI_AdvLootWnd.xml`).
  Double hyphens are illegal in XML comments; the SIDL parser aborts the **whole file** and the client
  dies before char select. Diagnose with **`UIErrorLog.txt`** in the EQ root — it names the file and the
  exact line (`ParseNodeList() SyntaxError` + `Error reading XML.`). Applies to every `EQUI_*.xml` we ship;
  use commas/single hyphens in comments. Container has no xmllint/python, so scan with
  `sed 's/<!--/«/g; s/-->/»/g' f.xml | grep -n -- "--"` (must print nothing).
- **Client install:** `aotv4_client_install/ADVLOOT_WINDOW_INSTALL.md` (copy the XML to `uifiles/default/`,
  `<Include>` it in `EQUI.xml`, rebuild the dll).

## 17c. Roaming world boss — `#worldboss` — 2026-07-26

A boss armed for a random classic dungeon; the server is told; everyone who damages it can roll on
the corpse. **No raid** — loot rights are granted directly, which disturbs nobody's group and covers
latecomers. Pure Lua + SQL, no C++. **Test plan + known failure points: `WORLD_BOSS.md`.**

- **Files**: `lua_modules/aotv4_worldboss.lua` (core), `lua_modules/commands/worldboss.lua` (`#worldboss`
  / `status` / `clear`, access **200**), `custom/sql/aotv4_worldboss.sql`
  (npc **2000200 `#The_Nameless`**, loottable 200020). Hooks: `global_player.event_enter_zone` →
  `on_enter_zone`, `global_npc.event_death_complete` → `on_death`.
- ⚠️⚠️ **A LUA COMMAND IS REGISTERED IN `lua_modules/command.lua`, NOT in `command_settings`.** That
  table is for **C++ commands only** — the zone deletes any row whose command has no C++ handler,
  logging *"Command [x] no longer exists. Deleting orphaned entry from `command_settings`"*. A file
  in `lua_modules/commands/` that is not listed in `command.lua`'s `commands` table is simply never
  reachable. **`#worldboss` was unregistered and therefore did nothing from the day it was written
  until 2026-07-27**, which is also why it was never caught: it was only ever going to fail the
  moment somebody typed it. `commands["name"] = { access, require("commands/file") }`.
- ⚠️ **ARMING AND SPAWNING ARE SEPARATE, and must be.** You cannot spawn into a zone nobody occupies:
  zones are dynamic (idle ones self-terminate) and `eq.spawn2` is zone-local. So the command records
  `aotv4_worldboss = "zone|npcid|expiry"` in a GLOBAL bucket + announces, and the **first player to
  enter that zone spawns it** — on their own coordinates, which guarantees valid placement in any
  zone with no per-zone spawn-point table. The bucket is cleared BEFORE spawning so two simultaneous
  arrivals can't both spawn one. Lapses after `WINDOW_MINS`.
- ⚠️ **Raid is READ-ONLY from Lua** — `Lua_Raid` has no `AddMember` binding (the C++
  `Raid::AddMember` exists). Auto-raid would need a new binding; that's why loot rights were chosen
  instead. `corpse:AddLooter(mob)` + `mob:GetHateList()` are both bound, and `MAX_LOOTERS = 72`.
- ⚠️ **Region locking still applies** — a boss in a region nobody has unlocked is unreachable. Keep
  `M.ZONES` inside open regions.
- Zone list = expansion 0, ≥25 spawn points, **zero merchant NPCs** (good "not a city" proxy), minus
  the gated planes → 14 classic dungeons.
- 📌 Tuning (level 50 / 120k hp) and the 5-item placeholder loot pool are **guesses until it is
  fought**. Scheduling is deliberately not built yet — GM command only.

## 17b. Shield Wall — `/shield` damage splitting — 2026-07-26

Several players share one monster's melee damage. **Player-facing doc: `SHIELD_WALL.md`.**
Built by opening up the native `/shield` (Warrior-only, 12s, one shielder) rather than writing a
parallel system — the native path already does the pairing, the distance break and the teardown.

- **Split maths** (`Mob::ApplyShieldWall`, `zone/attack.cpp`): `share = damage * (100 + penalty*(N-1))
  / (N*100)`. Penalty 20 → 2 sharers take **60% each** (120% total), 3 take ~47% (140%). Sharing is
  deliberately **lossy**: it converts one spike into several survivable hits at a cost in total HP,
  so stacking bodies is progressively worse value. Everyone takes the **same** share incl. the aggro
  holder — the stock shielder mitigation is NOT applied on top, or the tank would take more than the
  helpers. ⚠️ Multiply before dividing or small hits truncate to nothing.
- ⚠️ **Stock allows exactly ONE shielder** (`m_shielder_id`; `ShieldAbility` rejects a second with
  `ALREADY_SHIELDED`). Added `m_shield_wall` (vector on the TARGET) as the source of truth, with
  `m_shielder_id` kept in sync with its head so every native teardown path still works. Distance is
  re-checked **per hit**, which is what makes a permanent pairing safe.
- ⚠️ **`/shield` is now a TOGGLE** (`Handle_OP_Shielding`). With a permanent duration there was
  otherwise no way to stop short of walking away or dying. Toggling off costs no recast.
- "Permanent" = an 86400000 ms timer, not a disabled one — `ShieldAbilityFinish()` keys off
  `shield_timer` to tear down cleanly.
- Rules: `AoT:ShieldAnyClass` / `ShieldMinLevel` / `ShieldPermanent` / `ShieldRecastSeconds` /
  `ShieldDistance` / `ShieldWallPenaltyPercent` / `ShieldWallMaxSharers`. **Needs a zone rebuild.**
- **Visible "Shielded" buffs** = spells **43380-43382** (`custom/sql/aotv4_shield_wall_buff.sql`),
  inert markers rebuilt by `ShieldWallRefreshBuff()` on every wall change.
  ⚠️ **One spell id PER SHIELDER, not one for the wall.** EQ will not stack the same spell id from
  two different casters — a second cast of 43380 overwrites the first — so three identical rows with
  distinct `spellgroup`s are needed for three shielders to each show as their own buff. Count must
  track `AoT:ShieldWallMaxSharers` minus 1; extra shielders beyond the row count go uncredited.
  ⚠️ It lives at **43380 (helper band 43350+)**, NOT 43300-43349 — the pool generator pulls that band
  wholesale and would offer "Shielded" as a level-up reward.
  ⚠️ **Cast it FROM THE SHIELDER, never via `ApplySpellBuff`.** That helper does
  `SpellOnTarget(spell_id, this)` on the *buff holder*, so the buff records the shielded player as
  its own caster and inspecting it reads "cast by yourself". Buffs carry `casterid` + `caster_name`
  (`Buffs_Struct`, `zone/common.h:228`) and the client shows them on inspect — casting from the
  shielder is what puts a name there. (The buff's *name* is still fixed by the spell row; the caster
  field is the part that identifies who.) `ShieldWallRefreshBuff()` re-applies from the head of the
  wall whenever it changes, so it never credits someone who has already stopped.
  ⚠️ Three places mutate the wall — `ShieldWallRemove`, the per-hit distance prune in
  `ApplyShieldWall`, and `ShieldAbilityClearVariables` — **all three must fade/refresh the buff** or
  it strands on the player.
- ⚠️ **UNTESTED at runtime** as of writing — it compiles and links, but nothing has swung at a
  shielded pair yet.
- 📌 **TODO — the Shielded buffs are PLACEHOLDERS.** Using real buff slots to display transient state
  works but is the wrong long-term shape: it burns a buff slot per shielder, caps at however many
  spell rows exist (43380-43382), and every future short-lived effect would need its own throwaway
  spell row the same way. **Build a proper tracker for short buffs / autoskills** — a purpose-made
  display fed from server state rather than the buff bar — and move Shield Wall onto it. Same problem
  already shows up in the inert-marker buffs elsewhere (43022 Divine Aura, 43035 Blade Turn, 43056
  Counterattack, the Thirst line): they exist to be *seen*, not to do anything.

## 17. Database access from Windows + container recovery — 2026-07-24

> ⚠️⚠️ **See §25 first.** This section's "a rebuild does NOT destroy the DB — the old container still
> has it" is correct but incomplete: the same root cause also produces a **stale-but-present**
> database that looks entirely healthy. `db_sanity.sh` detects it in one second.

### Connecting a GUI client (HeidiSQL) to the peq DB
MariaDB runs **inside the dev container** (`eqemu_config.json` → `127.0.0.1:3306`; there is no separate DB
container). To reach it from Windows it is published as a **real Docker port**, exactly like the EQ ports:

- `devcontainer.json` `appPort`: **`"127.0.0.1:3307:3306"`** → HeidiSQL connects to **`127.0.0.1:3307`**,
  user `peq` / `peqpass`. Host-side bind is loopback, so the DB is reachable from Windows only, never the LAN.
- ⚠️ **Do NOT use `forwardPorts` / the VS Code PORTS tunnel for MySQL.** It was `forwardPorts: [3306]` and it
  **does not work**: MySQL is a **server-speaks-first** protocol (the server sends the greeting before the
  client says anything), and the tunnel fails exactly there → HeidiSQL reports **"Lost connection to server at
  'handshake: reading initial communication packet'"**. In the PORTS panel a tunnel shows Origin
  `Dev Containers`; a published port shows **`Statically Forwarded`** (what 5998/5999/7000-7005/9000 use, which
  is why the EQ client always worked). `forwardPorts` was removed so it can't also grab 3307 and shift the port.
- ⚠️ Three things must ALL be true or you get a *different* wall each time:
  1. **`bind-address = 0.0.0.0`** in the container. Stock is `127.0.0.1` (`50-server.cnf:27`), and a published
     port forwards to `eth0` where nothing would be listening. Set via a drop-in
     `/etc/mysql/mariadb.conf.d/99-aotv4.cnf` written by **`postStartCommand`** — an edit made by hand inside
     the container lives on the **container layer** and is silently lost on the next rebuild.
  2. **A `peq'@'%'` grant.** A published port arrives from the **docker gateway (172.17.0.1)**, NOT loopback,
     so the stock `peq'@'127.0.0.1'` user does not match → "Access denied for user 'peq'". Both users now exist.
  3. **Right port.** Connecting to `127.0.0.1:3306` on Windows hits whatever local MySQL owns 3306 (that's WHY
     VS Code picked 3307) — the tell is `Access denied for user 'peq'@'localhost'`, i.e. a *different server*
     answering, not a credential problem.
- `postStartCommand` also **starts MariaDB on every container start**, so it no longer has to be started by hand.

### A rebuild does NOT destroy the DB — the OLD CONTAINER STILL HAS IT
The `aotv4-mysql-data` volume (§2) was committed **Jul 21 02:45** but a mount only takes effect on the next
rebuild, and none happened until **Jul 24 22:31** — so the DB lived on the *container layer* for those 3 days
and the fresh container came up with an empty volume. **The data was never lost**: the previous container was
still there (stopped or even running) with its datadir intact. Before importing any snapshot, **check the old
containers first** — a snapshot import silently costs every day of work since the snapshot was taken.

```powershell
docker ps -a --format "{{.ID}}  {{.CreatedAt}}  {{.Status}}  {{.Image}}"   # incl. stopped; image vsc-aotv4-*
foreach ($c in "<ids>") { docker exec -u root $c sh -c "ls /var/lib/mysql | tr '\n' ' '; echo" }
docker exec -u root <ID> sh -c "du -sh /var/lib/mysql/peq; ls -l /var/lib/mysql/peq/character_data.ibd; ls /var/lib/mysql/peq | grep -c custom_achievement"
docker exec -u root <ID> sh -c "service mariadb start; sleep 8; mysqldump --single-transaction --routines --events --databases peq | gzip > /tmp/peq_recovered.sql.gz"
docker cp <ID>:/tmp/peq_recovered.sql.gz C:\AoTv3\AoTv4\peq_recovered.sql.gz
```
- ⚠️ **`docker exec` MUST use `-u root`.** Dev-container images default to user `vscode`, and the per-database
  dirs are `drwx------ mysql` → `du /var/lib/mysql/peq` returns **Permission denied**. A check written as
  `du … || echo "no peq db"` therefore reports **"no DB" on a container that has the full database** — this
  cost an hour and nearly triggered a needless snapshot import.
- **Pick the right container by evidence, not by name:** `du` size (live DB ≈ 750 MB), `character_data.ibd`
  mtime (the newest = last session; InnoDB flushes lazily so the mtime lags the true last write), and a
  `custom_achievement*` file count > 0 (proves the Jul-23 achievement work is in it).
- MariaDB will not be running in an old container (PID 1 is the sleep loop) — that is normal, `service mariadb
  start` first. Its startup runs **InnoDB crash recovery**, replaying `ib_logfile0`, which is exactly why you
  dump from a started server instead of copying the datadir files.
- Verify a dump before trusting it: `gzip -dc x.sql.gz | grep -c "^CREATE TABLE"` (expect **233** now — 223
  pre-achievements) and a `Dump completed on …` marker at the tail. **36 MB gzipped ≈ 341 MB SQL is normal.**
- **Canonical recovery point is now `/src/peq_recovered.sql.gz`** (Jul 24 23:18, full `peq`, achievements
  included), NOT the stale `/src/aotv4_current.sql` (Jul 21) that `POST_REBUILD_RECOVERY.md` used to point at.

## 18. Server-wide buffs — `#worldbuff` — 2026-07-27

`#worldbuff <spellid> [minutes]` puts any spell on everybody, everywhere. Also `status` and `clear`.
Access **200**. Pure Lua, no C++.

- **Files**: `lua_modules/aotv4_worldbuff.lua` (core), `lua_modules/commands/worldbuff.lua` (the
  command), registered in `lua_modules/command.lua`. Hooks: `global_player.event_connect` and
  `event_enter_zone` → `on_player`, `event_timer "worldbuff"` → `on_sweep`.
- ⚠️ **THERE IS NO SINGLE CALL THAT REACHES THE WHOLE SERVER.** Lua can only enumerate clients in its
  OWN zone (`eq.get_entity_list():GetClientList()`), and every zone is a separate process. The
  cross-zone bindings are all targeted — `cross_zone_cast_spell_by_char_id` / `_client_name` /
  `_group_id` / `_guild_id` / `_raid_id` / `_expedition_id` — there is no "by everyone". So coverage
  is assembled from four places: the command buffs its own zone at once, `event_connect` catches
  logins, `event_enter_zone` catches zoning, and a per-client repeating timer catches the player who
  is parked somewhere and does neither.
- State is ONE global bucket, `aotv4_worldbuff = "spellid|expiry_epoch"`, so every zone reads the
  same thing and it survives restarts. Same shape as `aotv4_worldboss`.
- ⚠️ **`ApplySpellBuff`, not `SpellFinished`.** `SpellFinished` runs a real cast: it can be resisted
  and it honours target type, so a beneficial spell with the wrong `targettype` will not land on an
  arbitrary player. `ApplySpellBuff` puts the buff straight on, which is what "the GM said everyone
  gets this" has to mean — and it is why **any** spell id works rather than only self/single-target.
- ⚠️ The sweep re-checks with `FindBuff` before applying, so it does not reset the duration every
  tick; a 10-minute buff lasts 10 minutes, not as long as the window.
- ⚠️ **The timer is PER CLIENT**, so `on_sweep` does per-client work. Calling `apply_zone()` from it
  would walk the whole client list once per player per sweep.
- `clear` stops new grants; buffs already handed out run out on their own.

## 19. Autoskill window — `/autoskill` — 2026-07-27

⚠️ **The autoskill SYSTEM predates this and was NOT changed.** It was already: `GetAvailableAutoSkills`
(the ten activated specials), `GetAutoSkillsList` (those you have), `Get/SetAutoSkillStatus`
(per-skill on/off in `autoskill.<id>` data buckets), and the auto-fire loop in `Client::Process` that
runs enabled specials while auto-attacking a **non-client** target. `#autoskill [skill]
[enable|disable|status]` / `#autoskill list` still work untouched. Section 19 is only the **window**.

- **Server**: `Client::SendAutoSkillData` / `HandleAutoSkillSay` / `GetAutoSkillCooldown` /
  `GetAutoSkillReuse`, all in `zone/special_attacks.cpp`; say intercept in
  `Client::ChannelMessageReceived` beside the AdvLoot one. **Needs a zone rebuild.**
- **Client**: `core_autoskill.cpp/.h` (own TU, no detours) + `EQUI_AoTAutoSkillWnd.xml`, flag
  `areAutoSkillWindowEnabled`. `/autoskill` from `core_bazaar.h`. Install:
  `aotv4_client_install/AUTOSKILL_WINDOW_INSTALL.md`.
- **Protocol**: `ASKILLDATA <n>^skillid|name|enabled|cooldown_secs|reuse_secs^…` out;
  `/say askset <skillid> <0|1>` and `/say askrefresh` in.
- ⚠️ **`GetAutoSkillCooldown` lives in `special_attacks.cpp` on purpose** — `AOTV4_SKILL_TIMER_BASE`
  is file-scope there, and duplicating it elsewhere is how the two silently drift apart.
- ⚠️⚠️ **TAUNT USES A DIFFERENT TIMER.** Every other special uses `AOTV4_SKILL_TIMER_BASE + skill`;
  Taunt runs on the stock `pTimerTaunt` (the auto-fire loop special-cases it for the same reason).
  Reading the wrong one reports a permanently-ready Taunt.
- ⚠️⚠️ **`PersistentTimer::GetRemainingTime` returns `0xFFFFFFFF` when the timer exists but is
  DISABLED**, not 0. Unguarded, a ready skill displays a 49-day cooldown. (`PTimerList` returns a
  plain 0 for a timer that was never created at all, which is fine.) Reuse times are **seconds**
  (`BashReuseTime = 5`, `common/features.h`).
- ⚠️ **The countdown runs CLIENT-side.** The server sends remaining seconds; the dll ticks them down
  per frame and resyncs every 2.5 s **only while the window is on screen**. Streaming a line per
  second per player is exactly the chat burst RoF2 drops (§15). Closed window, zero traffic.
- ⚠️⚠️ **THE TIMERS TAB IS GONE (2026-07-28) and should not come back in that shape.** It listed the
  firing skills with Ready In and Reuse columns, and that does not work: **Bash and Kick reuse in about
  FIVE SECONDS** (`BashReuseTime = 5`, `common/features.h`), so the number was ready again before the
  eye found it and the page looked permanently broken. A grid of buttons with the countdown drawn on
  each face was tried next and was not wanted either. The window is now a SINGLE page, no `TabBox`.
- ✅ **The real lever is a CAP, not a display.** `AOTV4_AUTOSKILL_MAX = 4` (`zone/special_attacks.cpp`)
  limits how many abilities may be enabled at once, enforced in `Client::HandleAutoSkillSay` -- the
  only path that turns one on. Without it the correct play is to enable everything and the choice of
  which specials to run stops being a choice. ⚠️ Enforced SERVER side on purpose: a client-side cap is
  bypassed by the `/say askset` the window sends, or by typing `#autoskill`. Turning one OFF is always
  allowed. The dll mirrors it in `AUTOSKILL_CAP` only so "All On" does not ask for a fifth and collect
  a red refusal per extra skill.
- Cooldowns are still parsed and ticked locally even though nothing shows them, so a future short-buff
  / autoskill tracker (§17b) finds them already running.
- ⚠️ **The list is rebuilt only when the rows change** (skill gained, or one toggled) — `Refresh()`
  calls `DeleteAll`, which clears the selection, so rebuilding on every resync would yank it out
  from under the player. Otherwise only the timer column is rewritten.

## 20. Spell window tabs — Choose / Known / Pool — `/journal` — 2026-07-27

⚠️ **This began as a SEPARATE window and is not one any more.** `EQUI_AoTSpellJournalWnd.xml` and a
`SpellJournalWnd` class existed for one iteration, then the two panels were folded into the EXISTING
reward picker so there is a single spell window. The standalone XML is kept as a backup and is
**unused** — do not copy it or `<Include>` it. `core_spelljournal.cpp` now owns **no window**: it is
the data and rendering service behind two tabs whose widgets belong to `core_spellchoice_native.cpp`.

**Three tabs**: **Choose** (the reward cards, icons and two-step Confirm — UNCHANGED),
**Known**, **Pool**. **Known** = what this character has scribed, grouped under
level-band headers. **Pool** = what can still be OFFERED at a chosen level, with a `<<`/`>>` stepper.
The Pool tab is the point: the pool is **2,154 spells over 78 levels** and was previously invisible —
a player could only ever see what they had been handed, never what they might be.

- **Files**: `core_spelljournal.cpp/.h` (own TU, no detours, no window) + the Known/Pool widgets in
  the GENERATED `EQUI_AoTSpellChoiceWnd.xml`, flag `areSpellJournalEnabled`. `/journal` and
  `/spells` from `core_bazaar.h` call `SpellChoiceOpen()`. Server: `spell_choice.send_pool` /
  `handle_journal_say`, routed from `global_player.event_say`. Install:
  `aotv4_client_install/SPELL_JOURNAL_INSTALL.md`.
- **Protocol**: `SJPOOLDATA <level> <chunk> <chunks>^id:known,id:known,…` out; `/say sjpool <level>` in.
- ⚠️⚠️ **The Pool tab lists TWO pools, and combat abilities are the second one.** `spell_pool.lua` is
  spells only; the activated combat abilities live in `skill_pool.lua` and were invisible on the tab
  until 2026-07-28, which read as "not obtainable" — wrong, the picker spends a slot on one roughly
  every eighth level. They ride the same line as **`k<skillid>:<known>:<name>`** and the client lifts
  them to **`SJ_SKILL_BASE (1000000) + skillid`** so one `sjinfo <id>` can ask about either kind.
  ⚠️ Without the offset a skill id (0-76) *is* a valid spell id — `sjinfo 8` would describe spell 8.
  ⚠️ The **name is on the wire** because this dll has no skill-name table; hardcoding one is the
  `kIcons[]` drift trap again (§3). ⚠️ `SKILL_ID_BASE` (spell_choice.lua) and `SJ_SKILL_BASE`
  (core_spelljournal.cpp) must match. Abilities are **not level-banded** (`rotatable_for(class)`
  ignores level), so they list once at the top for every level; a class never sees its own natives.
- ⚠️ **Bard songs ARE in the pool** (150 of them, ~1-2 per level, only 3 blacklisted) — they are just
  sparse and carry no marker, so "no songs" is nearly always "did not spot one" rather than a filter.
- **Adapted from `New folder (2)/AoT_Spell_Book.cpp`** (an earlier AoT generation). That window is
  where the two-native-tabs + four-listboxes layout was first proven on this client — it is the file
  §16 cites. What was deliberately NOT taken:
  - its **progression model** (one spell slot per character level, tiers of 5, a Roll button, mastery
    ranks bit-packed into synthetic spell ids: `15-13` mastery / `12-8` tier / `7-0` index). Ours
    scribes real `spells_new` rows chosen three at a time on level-up; none of that machinery applies.
  - its **transport** — it talks over opcodes (`0x196A`/`0x196B`, range 6500-6504). This dll has **no
    raw-packet send at all** and every window it owns rides the shared `dsp_chat` detour.
- ✅ **Descriptions are built CLIENT SIDE** from the client's own spell record (`GetSpellByID`, then
  `Attrib[]`/`Base[]`/`DurationType`). This is the technique worth having from that reference. It
  removes the standing two-source problem in §14: the picker's text comes from `spell_desc.lua` over
  `SPELLDESCDATA` while the spellbook's comes from `db_str` via an exported `dbstr_us.txt`, and those
  must be kept in sync by hand. Text derived from the spell cannot drift from it and costs no traffic.
  ⚠️ The SPA label table is deliberately NOT exhaustive — an unknown SPA falls through to a generic
  `SPA n: base` line so it is visibly unlabelled rather than silently absent.
- ⚠️ **IDS ONLY on the wire; the client resolves names.** Sending names would roughly quadruple the
  payload (level 70 holds 125 pool spells: ~1.1 KB as `id:known` vs ~4 KB with names).
- ⚠️ **And it is CHUNKED anyway (60/line).** An oversized chat line is silently TRUNCATED, which looks
  like a short pool rather than an error — and §15 already records RoF2 dropping/reordering bursts.
  Each line carries its own chunk index, and a chunk whose level ≠ the level on screen is **dropped**
  (chat can arrive out of order; without that guard two levels would merge into one list).
- ⚠️ **Row index is NOT a data index on either list** — Known inserts a band header every 10 levels,
  so both lists keep an explicit row→spell-id map. Same trap as the Autoskill Timers page (§19).
- ⚠️ **An all-254 spell shows "Handled by the server; no client-visible effects."**, not "no effects".
  Several of ours are deliberate inert marker buffs paid by Lua (the Thirst line 43342-43347, the
  Shield Wall buffs). The wording is deliberate.
- The Known tab costs **zero** server traffic — it reads `CHARINFO2::SpellBook` directly.

### ⚠️⚠️ 43000-43112 is DELETED (2026-08-01) — retired from the pool 2026-07-27, rows removed now
That band was the 113-spell custom reward set (Ember, Zap, Kick, Strike, Counterattack, Moonfire…):
mostly redundant with native spells, and being all-slots-254 inert markers they rendered as "no
effects" in any client-side description. The generator's clause that offered them was removed on
2026-07-27 (pool 2,174 → 2,154); **the rows themselves went on 2026-08-01** via
`custom/sql/aotv4_retire_43000_43112.sql`, together with everything that called them:
- deleted `lua_modules/aotv4_pets.lua` and `lua_modules/aotv4_summon_table.lua`
- deleted the 14 scripts `quests/global/spells/{43010,43011,43015,43017,43025,43027,43028,43030,
  43035,43052,43054,43055,43056,43059}.lua`
- `global_player.lua` — the pets require, the **repeating 5-second `aotpet` timer** and its branch
- `global_npc.lua` — the pets require, `on_pet_spawn`, and the **whole `event_damage_given`**, which
  existed only to drive the summoned-pet behaviours. ⚠️ The player-side damage hooks (Thirst, Sinew,
  reactions) are in `global_player.lua` and are untouched — do not re-add it looking for them.
- `aotv4_reactions.lua` — the four reaction branches (Divine Aura 43022, Blade Turn 43035,
  Counterattack 43056, Vengeful Aura 43059), their charge tables, tuning and `arm_`/`clear_` API.
Full restore of all 202 original custom rows is `custom/sql/aotv4_custom_spells_backup.sql`; a
targeted pre-delete dump of 43000-43199 is `peq_pre_43xxx_purge.sql.gz`.
- ✅ **`on_damage_taken` got measurably cheaper**, which matters because it runs on **every damage
  event for every player**. It used to resolve `CastToClient` + `CharacterID` **and an
  `eq.get_entity_list():GetMobID()` lookup — an entity-list walk per hit** — before its first
  early-out, purely to feed those four branches. All three are gone; only the duel block remains.
- ⚠️⚠️ **42 STOCK ITEMS REFERENCE THE DELETED BAND, AND THAT IS EXPECTED — LEAVE THEM ALONE.** The
  50xxx → 43xxx renumber (§14) landed on ids **stock EQ already used**, so items 143000+ (*Tome:
  Breather*, *Tome: Cyclone Roar*, *Tome: Insult*, *Tome: Warrior's Bulwark*…) carry a `scrolleffect`
  pointing into it. Those stock disciplines were overwritten and no longer exist anywhere in
  `spells_new` — checked by name. The tomes are **unobtainable here** (0 rows in `lootdrop_entries`,
  0 on any merchant; level 70+ content behind a Classic lock and a level 30 cap), so the collision
  was never reachable. They now point at nothing rather than at the wrong spell, which is strictly
  better. 📌 Revisit only if a future expansion unlock makes them obtainable.
- ⚠️ **The 11 helpers at 43150-43199 are now ORPHANED and were deliberately kept.** 43150 (Open
  Wounds mark) was applied by 43052 and 43155 (Duel lock) by 43030, both in the deleted 113, so
  nothing applies them. `43150.lua`, `43155.lua` and the `aotv4_reactions` code that reads them still
  compile and still work — they simply never fire. Removing them is a separate decision.
- ⚠️ `aotv4_abilities.lua` **survives**: `43150.lua` requires it, and `aotv4_reactions.bleed_tick`
  still uses `ab.SKILL_OFFENSE`. It is no longer only-for-the-retired-set.
- ⚠️ Two names survive in shared memory and are **not** leftovers: `Smolder` and `Counterattack` are
  genuine **stock** spells (917 and 42175) that happened to share names with the customs.

⚠️⚠️ **"Get rid of the 43xxx spells" must NEVER be read as the whole range.** The band is not
homogeneous and everything from 43150 up is live or still referenced: **43150-43199** the helpers
(11, now dormant — see above), **43300-43349** the custom spell lines (reptile, sloth, moonfire,
promised, kindred, mark, thirst — all still offered), **43350-43399** their triggers plus the Shield
Wall buffs, **43400-43454** the AA-tree buffs and pet wards, **43500-43565** the class auras,
**43576-44327** the 752 spell-rank rows (§29). Deleting the range would destroy all four AA trees,
Shield Wall, the pet wards, the achievement auras and every spell rank.
📌 The 2026-08-01 purge above is the *only* part of the band that was safe to remove, and it took a
five-way reference check first — pool, other spells, AA, items and the Lua call graph.

### The tabs live in the GENERATED picker XML (`gen_choice_xml.pl`)
⚠️ `EQUI_AoTSpellChoiceWnd.xml` is generated, so the Known/Pool widgets are emitted by
`aotv4_client_install/gen_choice_xml.pl`, NOT hand-written. Rerun it after any pool regen (it already
had to be rerun for the icon set) and copy the result. The generator now emits three `<Page>`s inside
one `<TabBox>`.
⚠️ **The `<Screen>` lists ONLY `ASC_Tabs`.** Every other piece belongs to a page; naming one on the
screen as well places it twice and it draws outside its tab.
⚠️ The card coordinates were NOT moved — each page sits at `Y=TAB_Y` (22) and its pieces are
positioned relative to the page, so the Choose layout is byte-for-byte the old one. The window grew
450 → 500 tall to pay for the tab strip.
⚠️⚠️ **THAT `Y=TAB_Y` IS LOAD-BEARING — A `Page` IS NOT AUTOMATICALLY PLACED BELOW ITS TAB STRIP.**
A page's origin is the TabBox's top-left corner, **tabs included**, so a page at offset 0 puts its
first row of pieces directly ON TOP of the tab captions and both texts overprint into an unreadable
smear. The strip has to be paid for by hand on every page. Cost a round trip on
`EQUI_AoTLostWnd.xml` (§6) when its flat layout was restructured into tabs and the offset was left
at 0; 24 clears it on this build.
⚠️⚠️ **CONVERTING A FLAT WINDOW TO TABS DROPS ITS ANCHORS IF YOU ONLY MOVE THE PIECES.** Anything
that should grow with the window needs `AutoStretch` **plus** its four anchor tags; a piece without
them keeps its fixed `Size` forever, so dragging the window larger just adds empty space around a
small list. Same file, same session, immediately before the offset bug — the two together made a
resizable window look completely broken.
⚠️ A **sizable** window has its size AND position **saved per character** by the client, so an
edited `<Size>` only applies to a character that has never opened it. If it comes back wrong, drag
it once or delete its entry from `UI_<char>_<server>.ini` — the XML is not being ignored.
⚠️ `validate_ui_xml.sh` counts tags **inside comments**, so prose like `UI_<char>_<server>.ini` or
a bare `<Size>` in an explanatory note fails the tag-balance check. Write those without angle
brackets; the failure names a tag that does not exist in the markup at all, which is confusing.
- `SpellChoiceShow()` still refuses to open with nothing pending (Ctrl+Q's "is a reward owed" test
  keeps its meaning); **`SpellChoiceOpen()`** is the new browse entry point that ignores that.
- The transport calls back via `SjSetPoolReadyCallback` rather than touching the window, so
  `core_spelljournal` stays ignorant of who owns the widgets.

### ⚠️⚠️ Forward-declaring a client widget type in one of OUR headers
`core_spelljournal.h` was the first header in this dll to take a `CListWnd*`/`CXWnd*` in a signature,
and the obvious way to do it is wrong:
```cpp
class CListWnd;   // WRONG -- declares a second, unrelated ::CListWnd
```
The client classes live in **`namespace EQClasses`**, and `EQClasses.h` ends (line ~6973) with
`using namespace EQClasses;`. A global forward declaration therefore creates a *different* type, and
every later mention becomes **`'CListWnd': ambiguous symbol`** plus
**`cannot convert from 'EQClasses::CXWnd *' to 'CXWnd *'`** — over a hundred errors from one line,
almost all of them cascades that point nowhere near the cause.
```cpp
namespace EQClasses { class CListWnd; class CXWnd; }   // RIGHT
void SjRefreshKnown(EQClasses::CListWnd* list);        // and QUALIFY the signatures
```
Qualify rather than adding a global `using`, so the header works whether it is included before or
after `EQClasses.h`. Inside a `.cpp` the bare names are fine — `MQ2Main.h` pulls the using-directive
in first.

## 21. Launcher — `/aot` — 2026-07-27

One button per custom window, so none of them needs a remembered slash command. A narrow vertical
strip meant to sit under the stock **SC / EQ** buttons so it reads as part of that bar; draggable,
and the client persists its position, so it is placed once.

- **Files**: `core_aotmenu.cpp/.h` (own TU, no detours) + `EQUI_AoTMenuWnd.xml`, flag
  `areAoTMenuEnabled`. `/aot` and `/menu` from `core_bazaar.h`.
- **Buttons**: Spells, Autoskill, Adv Loot, Trader, Achievements, Search, Last Death.
- ⚠️ **Two different open mechanisms, and the split is deliberate.** Windows whose module exposes a
  show function are called directly (`SpellChoiceOpen`, `AutoSkillShow`, `AdvLootShow`,
  `ShowSearchWindow`, `LostWindowShow`). Windows whose CONTENTS come from the server are opened by
  **queueing their command** instead — Trader (`/trader` → `SHOPINVDATA`) and Achievements (`/ach` →
  `ACH|window|show`). Calling their internal show directly would put an EMPTY window on screen, and
  for the shop it would show the last stale listing.
- ✅ **It DOCKS to the left of the stock SC / EQ bar** on every open, so both are usable and neither
  covers the other. The bar is `EQMainWnd` and the client keeps a pointer at **`pinstCEQMainWnd`**
  (`eqgame.h` 0xF713C0), already in the address table -- so nothing is found by name and nothing stock
  is hooked. `CXWnd::GetScreenRect` (0x8638D0) and `CXWnd::Move(CXRect)` (**`CXWnd__Move1_x`**,
  0x8686F0) are both mapped on this build. ⚠️ `CXWnd__GetClientRect_x` is **NOT** defined here -- use
  GetScreenRect.
  ⚠️⚠️ **`CXRect::A,B,C,D` are left/top/right/bottom and are `DWORD` -- UNSIGNED.** Cast to `int`
  before any arithmetic: `bar.A - width` wraps to ~4 billion when the bar sits near the left edge,
  flinging the window off screen instead of clamping. If there is no room on the left it flips to the
  bar's right.
  ⚠️ Docking runs on OPEN, not per frame -- a per-frame reposition would fight the player dragging the
  window. Move the EQ bar, then reopen the launcher to re-dock.
- ⚠️⚠️ **DO NOT PUT BUTTONS INTO THE STOCK SC / EQ BAR. IT WAS TRIED AND IT CRASHES.** An `EQM_AoTButton`
  was added to `EQUI_EQMainWnd.xml` and its click caught by copying `EQMainWnd`'s vtable and
  overwriting **entry 34** (the slot `SetWndNotification` writes) -- necessary because
  `CCustomWnd::ReplacevfTable` only ever operates on `this`. **It crashed on the first click.** The
  code and the XML edit are both reverted; `EQUI_EQMainWnd.xml` is back to stock and is NOT shipped.
  Do not re-attempt: it patches a window this dll did not construct, on a build whose UI structs are
  already documented as not matching the headers (§13), and it can only fail at runtime -- there is no
  static check that would have caught it. The list is our OWN window, which gets notifications the
  ordinary way and cannot corrupt anything stock.
- **It auto-shows once per session** on entering the world (`AoTMenuTick` watching `pinstLocalPlayer`),
  then stays closed if closed. `AoTMenuOnUiReset` re-arms it so it survives a `/loadskin`.

### ⚠️⚠️ Listbox text colours are `0xAARRGGBB` — the ALPHA byte is mandatory (2026-07-28)
A colour written **`0x00RRGGBB` is fully transparent**, so the text is drawn and is completely
invisible. The Autoskill window showed an empty red selection bar and no skills at all for this
reason, and the spell Known/Pool tabs had the same fault. Use **`0xFF……`**. `core_advloot.cpp` and
`core_achievements_native.cpp` were already correct (`0xFFE0DCCD` etc.), which is why they looked fine
and made the others look like a data problem rather than a colour one.
⚠️ Second, separate trap in the same place: **`CListWnd::AddString` colours COLUMN 0 ONLY.** A cell
written afterwards with `SetItemText` has no colour and draws **black**. Every extra column needs an
explicit `SetItemColor(row, col, colour)` — the `Cell()` helpers in `core_autoskill.cpp` and
`core_spelljournal.cpp` now take a colour and do both.

### Blacklist as of 2026-07-29 — 637 pruned, **1996 offerable**
```
travel 170 · discipline 157 · summonitem 183 · illusion 23 · vision 19
enchant 20 · rez 15 · ldon 36 · curecurse 7 · corpse 3 · sense 3 · truenorth 1
```
⚠️ These counts are only true as of the last `gen_stock_pool.pl` run — **re-read them off the
generator's own output rather than trusting this block**, which drifts the moment any SQL touches
`spells_new`. The 2026-07-28 figures recorded here (619 pruned / 2008 offerable / `ldon 18`) were
already stale before the Sinew line was added: `ldon` had moved 18 → 36 on its own.
Pool is **2633** spells (2585 stock + 48 custom) over 78 levels before the blacklist is applied.
- ⚠️ **`discipline` MIRRORS `common/spdat.cpp IsDiscipline` EXACTLY** — `mana = 0 AND (EndurCost > 0 OR
  EndurUpkeep > 0)`. Do not simplify it to "has an endurance cost": that would misclassify anything
  carrying both, and the rule must agree with what the ENGINE calls a discipline or the picker and the
  grant path disagree.
  Disciplines are excluded because they are not spells and do not behave like one: a picked discipline
  trains into the **Combat Abilities** window, not the spellbook, and autoskill cannot fire it either —
  that handles only the ten activated specials in `Client::GetAvailableAutoSkills`. A reward that lands
  somewhere the player is not looking, driven by none of our systems, is not a reward. Throw Stone,
  Leg Strike, Head Strike and Provoke were all being offered this way.
- ⚠️ **Rule ORDER decides which bucket a spell is counted in.** Adding `discipline` moved 16 spells out
  of `summonitem` (199 → 183): they matched both. The counts are per-rule-first-match, not disjoint
  categories — a shifting count in one bucket after adding a rule is expected, not a regression.
- ⚠️ `truenorth`, `sense`, `illusion` and `vision` all test PURITY: pruned only when the junk effect is
  the ONLY thing the spell does. A spell that senses/disguises/points north AND does something real is
  kept. Any new rule of that kind gets the same treatment.

### ⚠️⚠️ Syntax-check Lua before restarting zones — `.devcontainer/custom/tools/luacheck`
The container ships **no `lua` or `luac` binary**, so for most of this project's life a broken quest
module could only be found by restarting zones and reading a player's CHAT log. That is how a mangled
string literal in `spell_choice.lua` took down the **entire** level-up picker: `global_player.lua`
`require`s it at line 4, so the failed module aborted the whole global player script, and the only
symptom was `error loading module 'spell_choice'` in game.

`luacheck.c` builds against the system Lua 5.1 and calls `luaL_loadfile`, which parses and compiles
but never RUNS the file — so quest scripts full of `eq.*` calls are safe to check.

```bash
cc -O1 -o .devcontainer/custom/tools/luacheck .devcontainer/custom/tools/luacheck.c \
   -I/usr/include/lua5.1 -llua5.1
find .devcontainer/repo/quests -name '*.lua' -print0 | xargs -0 .devcontainer/custom/tools/luacheck
```

⚠️⚠️ **DO NOT EDIT LUA WITH `perl -0pi -e`.** Both corruptions came from that: perl interpolates
`$"`, `$+` and friends inside the replacement, and a Lua pattern is *full* of them —
`"^sjinfo%s+(%d+)%s*$"` became `"^sjinfoH  PA u+(1990222272+)  +z * )`. It also silently inlined the
whole slurped file into `gen_stock_pool.pl` when a `$_` appeared in a replacement. Use the Edit tool,
or single-quoted `sed` with explicit line numbers, and run luacheck afterwards either way.

## 24. Scaling dungeon — "Delve" — `/delve` — 2026-07-29

Pick a level in a window, get dropped into a private **instance** of a Dragons of Norrath zone with
every mob scaled to that level and a real quest in the journal. Finish it and a chest drops where you
finished it; loot the chest or press Exit and the instance closes. **Pure Lua + SQL + one window —
no C++ server change at all**, because all four primitives are already native and Lua-bound:
`eq.create_instance` / `assign_to_instance_by_char_id` / `destroy_instance`, `client:MovePCInstance`,
`npc:ScaleNPC(level)`, and the task system for the journal.

- **Files**: `lua_modules/aotv4_dungeon.lua`, `custom/sql/aotv4_dungeon.sql`,
  `eq-core-dll/src/core_dungeon.cpp/.h`, `EQUI_AoTDungeonWnd.xml`. Hooks:
  `global_player` `event_say` + `event_task_complete` + `event_enter_zone` + `event_death` +
  `event_connect` + **`event_disconnect`**, `global_npc` `event_spawn` + `event_death_complete`.
  ⚠️ `event_disconnect` is **not** only a camp hook — see the trap below; it fires on every zone
  change, including the one that carries the player INTO the delve.
- **Protocol**: `DUNGDATA <unlocked>^level|name|cleared^…` out; `/say delve`,
  `/say delveenter <level>`, `/say delveexit` in. Command is `/delve` (alias `/dungeon`), plus a
  **Delve** button on the `/aot` launcher (§21; `EQUI_AoTMenuWnd.xml` grew 232 → 262 tall for it).
  Install: `aotv4_client_install/DUNGEON_WINDOW_INSTALL.md`.
- ⚠️ On the launcher, Delve sits in the **"open directly"** group, not the "queue the command" group,
  even though its contents come from the server — `DungeonShow()` opens the window *and* queues
  `/say delve` itself, so it is never shown empty. That distinction is the whole point of §21's split.
- Debug trace: `<EQ>\aotv4_dungeon.log`, which is what tells an unbuilt dll (no file) apart from a
  missing `EQUI.xml` `<Include>` (`pXWnd=NULL`) — otherwise identical symptoms.
- **Fifteen rungs, level 1 → 70** in fives, cycling the six DoN zones (`delvea` 341, `delveb` 342,
  `stillmoona` 338, `stillmoonb` 339, `thundercrest` 340, `thenest` 343), each at its own mission
  version. ⚠️ It **started at 50** originally, which meant a new character could not reach the first
  delve at all and clearing it jumped straight to 55 — the top of a ladder with no ladder under it.
- ⚠️ Six zones over fifteen rungs means a zone is **reused at several difficulties**, which is why the
  tasks carry **no level in the title** and `min_level 1`: a title of "[50]" would be wrong on 11 of
  the 15 rungs. The authoritative rung table is `M.LAYERS`.
  📌 They were originally per **ZONE** (six rows). They are now per **(dungeon, mode)** — 234 rows —
  because 33 LDoN dungeons round-robining six task families made the journal name the wrong dungeon.
  See the v36 subsection below.

- ⚠️⚠️ **DoN is the ONLY expansion past OoW with instanced content.** `dynamic_zone_templates` holds
  215 rows: Classic 6, LDoN 24 (14 zones), GoD 16 (9), OoW 28 (16), **DoN 141 (6 zones)** — and
  **zero** for DoDH through RoF. The 141 sit on 6 zones because each ships many *versions* with
  genuinely different populations (delvea 9, stillmoona 9, thenest 14…), i.e. ~59 ready-made layouts.
  Any zone can still be instanced by `create_instance`; the templates only say what stock content
  instances.
- ⚠️⚠️ **An EMPTY `npc_match_list` matches EVERY npc** — that is the mechanic, not an oversight.
  `task_client_state.cpp:528` wraps the whole name test in `if (!activity.npc_match_list.empty() &&
  …)`, so an empty list makes any kill in the listed zone count. `zones` holds the **base** zone id
  (instances keep it), so one task row serves every instance.
- ⚠️ **Layers unlock in order and only unlocked ones are ever SENT.** With loot out of scope the
  reward is XP, so a freely pickable level would be a power-level machine. The window cannot show or
  ask for a layer it was never told about, and `M.enter` re-checks anyway.
  - ⚠️ **Also open to `your level + 1`** (2026-08-06). Clear-only unlocking meant a level 30 character
    who had never delved started at rung 1 and ground up through 29 rungs beneath them. It is a
    **FLOOR, never a cap** — clearing past your level keeps those rungs open when you die back to
    level 1, which is the entire point of the clear chain on a roguelite. `+1` rather than exact level
    so the window's default (the top of the list) is a step *up*; the gear bump then moves it further
    for a geared character and does nothing for a naked one.
- ⚠️⚠️ **ENTERING IS REFUSED IN COMBAT (`GetAggroCount() > 0`), fixed 2026-08-04.** The delve window
  opens anywhere, so "Enter Delve" was a **free escape from any losing fight** — and a better one than
  Gate: it removes you from the zone entirely, breaks every hate list at once, costs no reagent and
  has no cast time to interrupt. Reported from play.
  ⚠️ `GetAggroCount`, not `IsEngaged` or "am I swinging" — it counts the NPCs holding you on their
  hate list, so it stays true while something chases you *after* you stop fighting, which is exactly
  when the escape is worth most. Same test stock `#zoneshard` and the merc code use.
  📌 Both entry paths funnel through `M.enter` (the window's `/say delveenter <level> <mode>` and the
  bare `delveenter <level>`), so one server-side gate covers both — a client-side check would be
  bypassed by simply typing the say.
  📌 **Exiting mid-fight is still allowed.** It is a smaller version of the same escape, but it
  forfeits the run (recorded as `A` in the history), so it already carries a cost. Revisit only if it
  turns out to be abused.
- ⚠️ **Gate order in `M.enter` is load-bearing**: validate → create instance → assign → task → move.
  The combat check above belongs in the **validate** stage for that reason — a refusal after
  `create_instance` leaks an instance every time it fires.
  Creating first leaks an instance on every refusal; moving before `AssignTask` puts the player in the
  dungeon with no journal entry, which reads as the quest having failed.
- ⚠️⚠️ **THE VERSION IS NEVER 0 (fixed 2026-07-29).** Version 0 is the **open world** spawn set, so
  instancing it produces a private copy of the ordinary zone — genuinely instanced (the zone boots
  with its own `inst_id`, verified in the log) but identical in layout and population to just walking
  in, which reads as "it opened the open world, not a DZ". The non-zero versions are the real DoN
  mission layouts. Entry coordinates come from **`dynamic_zone_templates.zone_in_*`** for that exact
  (zone, version) pair — the point DoN itself uses — **not** `zone.safe_x/y/z`, which can sit in a
  part of the map the mission layout does not populate. Mission versions are **smaller** (delvea 154
  points against version 0's 262) and the kill goals are set against them, not against version 0.
- ⚠️⚠️ **`eq.get_zone_id()` TAKES NO ARGUMENTS** — it returns the *current* zone. `eq.get_zone_id(name)`
  is a hard Lua error (`int get_zone_id()` + stack traceback) and it silently broke Exit, which had
  already printed its "you step back out" message before dying. Use `eq.get_zone_id_by_name()`, or
  better, record the numeric zone id on the way IN so no lookup is needed on the way out.
- ⚠️ **A FRESH instance every time.** `eq.get_instance_id` would hand back the one this character used
  last, still full of corpses and an emptied spawn table.
- ⚠️⚠️ **`e.other` IS A `Lua_Mob`, NOT A `Lua_Client`.** `IsClient()` returns true on it, but
  **`CharacterID` is only defined on `Lua_Client`**, so passing it straight into anything that keys a
  data bucket is *"attempt to call method 'CharacterID' (a nil value)"* — which fired on **every kill**
  in a delve. `CastToClient` is not reliably bound either; resolve through
  **`eq.get_entity_list():GetClientByID(mob:GetID())`** (what `aotv4_sinewtap.lua` already does).
  `M.as_client()` is the shared helper.
- ⚠️⚠️ **DoN zones have real zone lines out of them**, and walking one drops you into the ordinary
  world with the run bucket set, the task live and the instance open. `M.on_enter_zone` (from
  `global_player.event_enter_zone`) **fails the run** and returns you to where you entered from. ⚠️ The
  test is *"am I somewhere other than my delve"* (zone short name **and** instance id), not "did I
  zone" — the same hook fires on the way **in**. ⚠️ Unlike the death path this one **does** move the
  player, because the zone change has already taken them out of the instance.
- ⚠️ **The chest has to be findable.** It lands wherever the last blow happened — a corridor, a corner,
  behind a corpse pile — and a default chest on a dark DoN floor is easy to walk straight past. Four
  things stack: `texture 1` (gilded), `size 12`, `light 10` on the npc row, plus `AddNimbusEffect(412)`
  at runtime and a message. ⚠️ **412 is the only nimbus id proven to work in this codebase**
  (`guildhall/#Healer.lua`); a wrong nimbus id fails silently.
- ⚠️⚠️⚠️ **`EVENT_DISCONNECT` FIRES ON EVERY ZONE TRANSFER, NOT JUST CAMP / LINK-DEAD — and this
  single fact made the Delve un-enterable for an entire session (fixed 2026-07-30).**
  `zone/client_process.cpp` tests `bZoning` for other work in the very same function —
  `if (!bZoning) { SetDynamicZoneMemberStatus(Offline); }` (**:735**) — and then fires the event
  **unconditionally** thirty lines later (**:765**). So the instant `M.enter` moved a player into
  their delve, the source zone raised `event_disconnect`, `M.on_disconnect` concluded the run was
  over, and it deleted the run bucket, the back bucket and the `instance_list_player` row **while the
  player was still on the loading screen**. World then failed its own entry check (next bullet) and
  redirected them out. Guarded by a **transit flag** (`delve_transit_<charid>`): set immediately
  before the move, cleared on arrival *and* on landing anywhere else, expiring after 60 s so a real
  camp mid-transit still tears down on next login. `Client::IsZoning()` exists (`zone/client.h:855`)
  but is **NOT Lua-bound**, hence the flag.
  - ⚠️⚠️ **The tell is which teardown path DIDN'T run.** `on_disconnect` is the only one that
    deliberately leaves the journal entry alone, so the fingerprint was buckets + membership row
    vanishing while the **task survived**. Polling the DB five times a second is what found it:
    `.934` everything correct → `.368` buckets and ILP row gone → `.029` evicted. **Reach for a
    DB poller early on anything that fails this silently** — three separate "fixes" were aimed at
    downstream symptoms before anything was actually measured.
  - 📌 Any future handler that tears state down on `event_disconnect` needs the same guard.
- ⚠️⚠️ **A CHARACTER MUST BE REGISTERED ON AN INSTANCE OR WORLD SILENTLY REDIRECTS THEM.**
  `world/client.cpp:929` runs `VerifyInstanceAlive` → `CheckInstanceByCharID` (an
  `instance_list_player` row); on failure it calls `MoveCharacterToInstanceSafeReturn` and zeroes
  the instance. Symptom is **"it opens the delve and then ports me to the bazaar"** — no error, no
  log line, and nothing wrong in the zone that booted. `eq.create_instance` creates the instance but
  **adds nobody to it**; `eq.assign_to_instance_by_char_id(instance_id, character_id)` is what
  registers them (⚠️ that argument order, not the reverse). The stock `#zoneinstance` does exactly
  this before its move (`zone/gm_commands/zone_instance.cpp`), which is why the GM command worked
  when `/delve` did not — the cheapest way to bisect this class of bug.
  - 📌 `Client::SendToInstance` (`zone/client.cpp:11003`, Lua-bound, wrapped by
    `quests/plugins/Instances.pl`) is the native one-call form: `AssignToInstance` + `MovePC`. It is
    exactly the two calls above, so it is a tidier spelling rather than a different mechanism — but
    it caches the instance id in a data bucket keyed by type/name/identifier/zone, so it **reuses**
    an instance unless the identifier is varied per run.
- ⚠️⚠️ **DO NOT CREATE AN EXPEDITION AND ZONE INTO IT IN THE SAME BREATH — world reaps it first.**
  `world/dynamic_zone.cpp:104`: `if (!HasMembers() || IsExpired())` and the dz zoneserver has
  `NumPlayers() == 0`, the DZ is `ExpiredEmpty` and **deleted**, taking its `instance_list_player`
  rows with it. Creating and moving together tears the source-zone `Client` down mid-handshake,
  before world's copy of the DZ has its member list, so it looks memberless — measured at ~270 ms
  from create to delete. Expeditions themselves are **fine** on this build: AoTv3's Progressive
  Dungeons uses them (`server/quests/draniksscar/302093.lua`, repo `Twochords/AoTv3`) but creates the
  expedition and then **stops**, telling the player *"Click the dungeon portal to enter your active
  mission"* — entry is a separate act through the engine's own DZ portal, exactly as live EQ does it.
  The Delve therefore uses a **plain instance**; the cost is no Ctrl+Z entry, no compass and no
  engine-managed safe return. Revisit only by decoupling entry from creation (portal / confirm step),
  never by adding a delay.
- ⚠️⚠️ **CAMPING (`/q`) IS THE THIRD WAY OUT** and needs its own handling: the character is **saved
  standing inside the instance**, so if that instance is later gone the next login asks world to place
  them somewhere that no longer exists. The engine falls back gracefully (the world log shows
  `delvea(105)` → `tutorialb`), but every camp leaves an **orphaned instance** and a run bucket
  claiming the player is in a dungeon they left. `M.on_disconnect` (`event_disconnect`, which this
  server had **no** handler for at all) ends the run; ⚠️ it deliberately does **not** move the player
  or touch the task — the client is already leaving. `M.on_connect` drops the stale journal entry on
  the way back in, guarded on there being no live run so a camp-and-return mid-delve is not disrupted.
- ⚠️⚠️ **`DungeonOnUiReset` must `delete` the window, not just null the pointer.** `/q` tears the UI
  down through `CleanGameUI`; nulling alone leaks the `CCustomWnd` **and leaves it registered with the
  client's window manager** after its widgets are freed, so the next thing to walk the window list
  touches freed memory. Every other native window here deletes (`core_lostwindow`, `core_autoskill`,
  `core_advloot`, `core_allaclone`) — the delve window was the odd one out.
- ⚠️ **Dying ends the run** (`M.on_death`, from `global_player.event_death`): the roguelite reset puts
  the character at **level 1**, so the mission cannot continue. It uses **`FailTask`**, not
  `RemoveTaskByTaskID` — a death is a real failure and should read as one, where walking out
  voluntarily should not. ⚠️ It is the **one teardown path that does NOT move the player**: the engine
  is already sending them to their bind point as part of the death flow, and a competing `MovePC` from
  inside `event_death` risks breaking the respawn. Drop the task *before* destroying the instance.
- ⚠️ **Destroy the instance LAST**, after the player is out, or you strand a client in a zone that no
  longer exists. `M.leave` is the single teardown path for both the chest and the Exit button.
- ⚠️ **`AssignTask(id, 0, false)`** — `enforce_level_requirement` is false on purpose. The tasks carry
  DoN-style `min_level` tiers so they *read* native, but the unlock chain is the real gate.
- ⚠️ `RemoveTaskByTaskID` (custom binding, `lua_client.cpp`) is used on exit, not `CancelAllTasks`: it
  clears the DB rows too, so an abandoned delve does not leave a permanent journal entry.
- ⚠️ **`ScaleNPC`'s Lua binding passes `always_scale = true`** (`lua_npc.cpp:782`), so it scales mobs
  not flagged for scaling — every stock DoN mob. No `npc_types` editing needed. The chest is skipped
  explicitly or it would get 70th-level HP and become a fight.
- ⚠️ `M.on_npc_spawn` runs for **every NPC spawn on the server** — the instance-id early-out rejects
  the whole normal world before anything else is read.
- ⚠️ **`AddLooter` is on `Lua_Corpse`, not `Lua_NPC`**, so it cannot be applied to the living chest.
  Loot is out of scope for now, so the chest is an empty marker whose death ends the run.
- ⚠️ There is **no `MoveToSafeCoords`** binding on `Lua_Client` (only `MovePC` and the `MoveZone`
  family) — the bind point is the fallback exit.
- ⚠️ npc_types columns were hand-listed on a first pass and `aggro_spell_id` does not exist on this
  schema. The chest is cloned from stock 119179 `a_gilded_chest` **via a temp table**, same rule as
  the custom spell lines.
- 📌 Not built yet: loot in the chest, group entry (`assign_to_instance` is per character today),
  a floor/ascent chain, and any Torghast-style in-run power picker.

### The map ladder — static, 50/50 DoN and LDoN — 2026-08-06
**70 rungs, one level apart, and every rung is a FIXED, KNOWABLE dungeon.** Odd rungs are DoN, even
rungs are LDoN, each family cycling independently: **35 / 35**.

- ⚠️⚠️ **ALTERNATING IS WHAT MAKES IT 50/50 — CONCATENATING THE POOLS DOES NOT.** DoN has 6 dungeons
  and LDoN has 33, so a single combined cycle delivers six DoN rungs and then thirty-three LDoN ones:
  the ladder arrives in two long blocks instead of a mix.
  📌 The LDoN pool was **34** until `veksar` was removed on 2026-08-06 (it is a real Kunark world zone
  with `zone_points` — see the v27 subsection), so **39 dungeons total**: 6 + 33.
- ⚠️⚠️ **A RANDOM DRAW AT ENTRY WAS TRIED AND REVERTED THE SAME DAY.** Knowing the map matters for
  planning a run and for knowing what the kill-credit rewards will be, and a map you cannot see until
  you are standing in it tells you neither. It also made the window's dungeon column **unfillable**
  (there is genuinely nothing true to print before the roll) and silently cost run history its
  dungeon name. If it is ever revisited, those three are the cost.
- ⚠️⚠️ **THE ORDER OF BOTH POOLS IS LOAD BEARING.** `M.LAYERS` is indexed 1..70 and the index **is**
  the unlock high-water mark, so reordering either family silently reassigns what every stored
  `delve_cleared_<id>` means — a character's cleared rungs start pointing at different dungeons.
- ⚠️ **`M.layer_of(run)` prefers the rung's own map but only when it MATCHES the zone the run
  recorded.** A run started before a pool edit would otherwise be handed a different dungeon's task
  and entry coordinates halfway through.
- ⚠️ Run history records the **zone**, not just the rung, and resolves the name from that — correct
  even after a pool edit. Appended after `bands` so pre-2026-08-06 rows still parse (`bands` is comma
  separated and never contains a `|`); a row without it reads "unknown" and cannot be backfilled.

#### LDoN — 33 zones, and it needed ZERO new task rows (until the journal started lying — see below)
`lua_modules/aotv4_dungeon_ldon.lua` is **generated**: every LDoN zone with >= 60 spawn points and a
real safe point, each carrying its list of populated layouts (up to 21). The rung picks one layout
deterministically, so a repeated dungeon is not a repeated map.
- ⚠️⚠️ **ENTRY IS THE ZONE SAFE POINT, NOT `dynamic_zone_templates.zone_in_*`.** Only **7** LDoN
  (zone, version) pairs have template coordinates at all. That is safe **for LDoN specifically** and
  would not be for DoN: an LDoN zone is only ever reachable through the adventure system, so its safe
  point *is* the normal zone-in. The warning against safe points elsewhere in this section is a DoN
  statement.
- ⚠️ **`zone_version = 0` is legitimate for LDoN** — it is just the default layout, not the
  "open world" set it is in DoN.
- ⚠️⚠️ **Migration v22 clears `task_activities.zones` on the delve band (2000300-2000399).**
  `TaskActivity::CheckZone` (`common/tasks.h`) opens with `if (zone_ids.empty()) return true;`, so an
  empty list credits **anywhere** — the same mechanism as the empty `npc_match_list` above. Without
  it, LDoN would have needed 33 zones x 6 modes of new task rows. `zones` is `varchar(64)` (~15 zone
  ids), so listing them explicitly was never an option.
  📌 v36 later created those 234 rows anyway — but **for the TITLE, not for the zone scoping**, and it
  leaves `zones` empty exactly as v22 did. The band it clears (2000300-2000399) predates the widened
  2000300-**2000538** band; the v36 rows are simply written empty at insert.
  ⚠️ **This is a real widening, accepted knowingly.** The only thing keeping a delve kill inside the
  dungeon is now `M.on_enter_zone`, which fails the run the moment you are outside your instance. The
  second, independent guard the zone column provided is gone.

#### ⚠️⚠️ THE LDoN MAPS WERE UNENTERABLE — THEY SAT IN REGION 99 "Unused" (v27, 2026-08-06)
Entering one answered *"You have not yet unlocked Unused."*, so **the whole LDoN half of the ladder
could not be played**. The six DoN delve zones were already in region 0 "Always Available"; v27 brings
the 33 LDoN ones into line.
- ⚠️⚠️ **ONLY ZONES WITH NO ZONE LINES ARE UNLOCKED, AND THAT IS THE ENTIRE SAFETY ARGUMENT.** Every
  zone in that list has **zero rows in `zone_points`** targeting it — there is no way to *walk* into
  one — so making it Always Available grants **no world access at all**. It is reachable only as a
  private delve instance, which the delve's own unlock ladder already gates.
- ⚠️⚠️ **`veksar` IS DELIBERATELY EXCLUDED AND WAS REMOVED FROM THE DELVE POOL.** It is LDoN *by
  expansion* but a **real Kunark world zone**: 2 `zone_points` lead to it and it is legitimately
  assigned to the Cabilis region. Unlocking it would have opened Kunark travel to anyone who had not
  earned Cabilis — a genuine hole in region locking, in exchange for one map out of 34. If it is ever
  wanted back, exempt **instanced** moves from the gate (`Client::ProcessMovePC`) instead of unlocking
  the zone. That is why the LDoN pool is **33**, not 34.
- 📌 **Test this before adding any future delve map**: a zone with `zone_points` is world content, and
  unlocking it is a **travel** change, not a delve change.
- ⚠️ Region 0 is "Always Available" and `RegionManager::CanEnterZone` treats an unmapped zone
  (`region_id 0`) as unrestricted by design, so this is the same state as "no region".

#### ⚠️⚠️ THERE ARE **TWO** GATES ON A DELVE MAP, AND v27 ONLY OPENED ONE (v37, 2026-08-07)
The region lock above is ours. The **second** gate is stock EQEmu, checked separately in
`Client::ZonePC` (`zone/zoning.cpp:367`):
```
if (CurrentExpansion >= Classic && !GetGM()) {
    if (z->expansion <= CurrentExpansion || z->bypass_expansion_check) { ...ok... }
}
```
The LDoN delve zones are `zone.expansion` **6** and this server runs Classic
(`Expansion:CurrentExpansion = 0`), so entry was refused with *"The zone that you are attempting to
enter is part of an expansion that you do not yet own."* — **all 35 even rungs, half the ladder.**
- ⚠️⚠️ **NOTE THE `!GetGM()` — THIS IS THE SECOND TIME IT HAS HIDDEN THIS EXACT BUG.** The GM flag
  skips the check entirely (*"Bypassing zone expansion checks because GM flag is set"*), so it works
  for every test character and fails for every player. The identical trap is written up in
  `custom/sql/aotv4_delve_zone_access.sql`, which fixed the **DoN six** on 2026-08-02 — the LDoN
  dungeons simply joined the ladder afterwards and were never added to that list.
  📌 It presents as **"the delve opened and then threw me out"**: the run bucket, the task and the
  instance are all created *before* the move, so the refusal lands after the setup succeeded.
- ⚠️⚠️ **`bypass_expansion_check`, NOT `expansion = 0`.** `expansion` is real metadata — content
  filtering, zone listings and the era system (§12) all read it — so rewriting it would make LDoN
  zones claim to be **Classic** content everywhere else in the server. The bypass flag exempts **only
  the entry gate** and leaves the zone honest about what it is. This is why delve zones stay
  expansion-tagged and that is correct, not leftover.
- ⚠️ Applied to **every version row**, not just version 0 — the check reads version 0 via
  `GetZoneWithFallback`, but leaving the others at 0 is a trap for whoever changes version resolution.
- ⚠️ **`veksar` is absent here exactly as it is from v27**, and the two lists must stay in step.
  📌 It already carries `bypass_expansion_check = 1` from stock/region data — which is fine, because
  the gate that actually guards it is the **region** lock (it is assigned to Cabilis), not this one.
- ⚠️ v37 folds the **DoN six** in as well. They were only ever fixed by that hand-run `custom/sql`
  script, which is **not in the manifest** and therefore never applies itself — on live, or on any
  freshly imported database, they would be blocked the same way. Re-stating them is idempotent.
  📌 Its other fix — `thenest` `min_level 66`, a **third** gate again (`Client::CanEnterZone`) — is
  carried over for the same reason. No LDoN delve zone carries `min_level`, `min_status` or
  `flag_needed`, so those 33 need nothing beyond the bypass.
- ⚠️ The `zone` table is **not** shared memory, but it is read at boot by **world AND zone** — restart
  both, no `./shared_memory`.

#### ⚠️⚠️ THE JOURNAL NAMED THE WRONG DUNGEON — 234 tasks now, one per (dungeon, mode) (v36, 2026-08-07)
The 36 task rows were authored when the delve was **DoN-only**: six zones × six modes, one task per
zone, so a zone name in the title was correct. Adding 33 LDoN dungeons made them **round-robin the
same six task families**, so a Deepest Guk run was handed *"Delve: Tirranun's Delve"* — wrong roughly
**85 percent** of the time. Reported from play.
- 📌 **It was never a FUNCTIONAL bug**, which is exactly why nothing errored and it survived: v22
  cleared `task_activities.zones`, so `CheckZone` returns true on an empty list and kills credit in
  whatever zone you are in. **Only the label lied.**
- Now **39 dungeons × 6 modes = 234 tasks**, built as 39 explicit dungeon rows `CROSS JOIN`ed with the
  6 modes rather than 234 hand-written INSERTs — same deterministic result, a tenth of the size, and
  adding a dungeon is one row.
- ⚠️⚠️ **THE MODE OFFSETS HAD TO WIDEN FROM 10 TO 40, AND `aotv4_dungeon.lua` MUST MATCH.**
  `M.MODES.taskoff` was `0/10/20/30/40/50`, which leaves room for only **ten** dungeons per mode; with
  39 the families overlap and a Hard run collects a Standard task. They are now
  **`0/40/80/120/160/200`**, so the band runs **2000300-2000538**. Change one side without the other
  and tasks silently resolve to the wrong mode.
- ⚠️ **Base ids**: DoN keeps **2000300-2000305** (unchanged, so its hand-written flavour text
  survives); LDoN takes **2000306-2000338** in `aotv4_dungeon_ldon.lua`'s own **file order**. That
  order **IS** the mapping — regenerating that file without reassigning ids reintroduces this bug.
- ⚠️ `zones` stays **empty** on every activity, exactly as v22 left it. Populating it would scope a
  task to one zone again and break the 33 LDoN dungeons that share a rung's task family.
- ⚠️ Gauntlet is the odd mode: **10 trash and THREE wardens**, not a multiple of the base goal.
  `max_level` is 200 and duration 21600 / code 3, copied from the original rows — **not** defaults.
- ⚠️ The migration is keyed on the **highest new id** (`2000538` = Fragile + dungeon 38), which cannot
  exist under the old 6-zone scheme whose band stopped at 2000355. Testing for a wrong **title**
  instead would also match the mode variants of the correct task and re-run forever.

#### ⚠️⚠️ THE GEAR BUMP CLAMP WAS FLAT AND PUT LEVEL 21 MOBS IN A LEVEL 1 DELVE (fixed 2026-08-06)
Reported from play: *"someone joined a level 1 and the mobs were level 21"* — which is exactly rung 1
plus a flat `MAX_BUMP` of 20.
The two clamps were **asymmetric**: down was `layer_level * MAX_DROP_FRAC` (proportional), up was a
flat 20. A flat number across a ladder spanning rungs 1 to 70 is not one rule but two — +20 at rung 50
is a 40 percent bump, +20 at rung 1 is a **2000 percent** one.
- 📌 **The reasoning was already written down, one clamp away.** The comment beneath the up-clamp
  explains at length why flat clamps are wrong; it was applied to the DOWN clamp only. The constant's
  own comment gives the assumption away — *"a fully decked character can push the level 50 layer to
  70"* — it was only ever reasoned about at the top of the ladder.
- Now `min(MAX_BUMP, layer_level * M.MAX_BUMP_FRAC)` with **`MAX_BUMP_FRAC = 0.50`**: rung 1 tops out
  at level 2, rung 10 at 15, rung 30 at 45, and **rung 40+ is unchanged** because the flat cap still
  binds there. Only the low rungs moved.
- ⚠️ The fraction is deliberately larger than `MAX_DROP_FRAC` (0.20): being over-geared should push a
  delve up harder than being under-geared drops it, or gear stops mattering at depth.

#### ⚠️ OPEN — the AC axis is not level-normalised
`M.power` is meant to read **gear, not level**, and three axes deliver that: `stats` and `resists`
divide by the naked totals (585 / 130) and do not change with level, while `hp`/`mana` divide by
`shape(level)` so level cancels — confirmed against `base_data` (a Warrior is 250/500/750 at 10/20/30,
exactly `25 * level`). **A naked character therefore scores ~1.0 at every level and gets the rung
exactly.**
⚠️ **`ac` is the exception** — it divides by a flat `BASE_AC = 31` with no `shape(level)`, while naked
AC does climb with level (defense skill cap, agility). At weight 0.20 and `BUMP_PER_POWER 12`, every
0.5 of drift there is roughly **+1.2 effective levels**, so a naked level 30 likely fights a slightly
harder delve than a naked level 10 at the same rung. The exact curve was not measured. **Measure a
naked character at 10 and at 30 before calling the scaling settled.**

### Player-relative creature scaling — `aotv4_dungeon_scale.lua` — 2026-07-29
The layer level is only the **floor**. Creatures also scale by how far the player sits above the
**naked expectation** for their level, which is the gear-bloat correction. `/say delvepower` prints
the whole calculation axis by axis.

- **Why it can be measured at all**: in EQ, stats / AC / resists **do not grow with level** on their
  own — a naked level 70 has the same STR as a naked level 1, because all of it is gear. Only HP and
  mana scale with level. So gear bloat is directly "actual over naked expectation", and only the
  HP/mana axis needs a per-level curve.
- **Baseline** = the level 1 reference character (Warrior/Karana): HP 38, MP 40, AC **31**,
  stats sum **585**, resists sum **130**. ⚠️ Stats and resists are race/class dependent, so one
  universal baseline is an approximation — the axis that matters most (HP) comes from `base_data`.
- ⚠️⚠️ **`base_data` gives the exact per-level curve, so do not guess it.** `hp` for any class is
  `CLASS_HP[class] × shape(level)`, and the shape is **identical for all 16 classes** (warrior/magician
  HP is a constant 1.25 ratio at every level). The shape is **linear to 40 then double rate**:
  `shape(L) = L` for L ≤ 40, else `40 + 2×(L−40)`, hitting exactly 100 at level 70. Treating it as
  linear throughout understates level 70 by 30% (70 vs 100) and makes every high-level character look
  geared while naked.
- **Weights** hp .30 / mana .15 / ac .20 / stats .20 / resists .15, then `effective =
  layer + 12×(power−1)`, clamped to −5 / +20. The reference character scores **power ≈ 1.05**, i.e.
  baseline difficulty.
- ⚠️ **Renormalise by the weight actually used.** A Warrior has no `base_data` mana, so that axis is
  skipped; dividing by the full weight total would score every melee class 15% weaker than it is and
  quietly soften their delves. (Melee mana here is a flat `level × 40` from §14 and carries no gear
  signal at all, so skipping it is also correct on the merits.)
- ⚠️ The HP expectation uses the player's **current STA**, so stamina-granted HP lands in the stats
  axis instead of being counted twice.
- ⚠️ **`ScaleNPC` must come before any `ModifyNPCStat`** — it rewrites stats wholesale from
  `npc_scale_global_base`, discarding anything applied first. `ModifyNPCStat` takes **strings** and
  accepts `max_hp`, `min_hit`, `max_hit`, `ac`, `atk`, `accuracy`, `avoidance`, the resists and `level`.
- ⚠️⚠️ **Re-scaling mid-run is OUT-OF-COMBAT ONLY** (`M.rescale_zone`, a 10s per-client timer). This is
  what catches entering stripped and then gearing up. Re-scaling something you are *fighting* would
  refill it to a new max_hp mid-swing, and re-scaling **down** would be the exploit in the other
  direction (strip, soften the boss, re-gear). A pulled mob keeps the difficulty it was pulled at.
  ⚠️ A `RESCALE_DELTA` deadband (0.08) stops one buff landing or fading churning the whole zone.
  ⚠️ The tick guards on **zone and instance id**, not just the run bucket — that bucket survives a
  crash or GM port, and without the guard the sweep would rescale mobs in an ordinary zone.

#### The warden — random class, class/level appropriate spells — 2026-08-04
Every delve boss rolls one of the 16 classes and fights like it. `M.BOSS_CLASSES` maps each class to
a stock `npc_spells` list (**ids 1-12 are EQEmu's own "Default \<Class\> List"**) and its own title
pool; the four pure-melee classes take list **0** and simply do not cast.
- ⚠️⚠️ **LEVEL-APPROPRIATENESS IS FREE, BUT ONLY IF THE LIST IS ATTACHED AFTER SCALING.**
  `NPC::AI_AddNPCSpells` (`zone/mob_ai.cpp:2475`) keeps only entries whose `minlevel`/`maxlevel`
  bracket **`GetLevel()` at the moment it is called** — so attaching the list before `ScaleNPC` loads
  the *template's* level 1 spells and freezes them there. Order is scale → class → mana → spells.
  ⚠️ Re-attaching is safe: the function does `AIspells.clear()` first, so nothing stacks.
- ⚠️⚠️ **SETTING THE CLASS IS WHAT CREATES THE MANA POOL — IT IS NOT COSMETIC.**
  `NPC::CalcMaxMana` (`zone/npc.cpp:2705`) returns **0 for any non-caster class**, and it does so
  *whether or not* the `npc_types.mana` column is set — the column is only read after the class test
  passes. The warden template is **class 1 (Warrior)**, so it had zero mana. The AI's cast gate is
  `mana_cost <= GetMana() || GetMana() == GetMaxMana()` (`mob_ai.cpp`), and for a 0-mana NPC the
  second half is `0 == 0` — **always true**. A Warrior-class warden holding a Wizard list would nuke
  forever, free. The class is the brake.
- ⚠️ **There was no way to set an NPC's class at all.** `class_` is protected on `Mob` with only
  `GetClass()`, no setter, no Lua binding, and `ModifyNPCStat` had no `"class"` key. Added one
  (`zone/npc.cpp`); it calls `CalcBonuses()` because the pool derives from class **and** the current
  INT/WIS and level, so it must run *after* scaling. ⚠️ It does **not** update the client — the spawn
  packet has already gone — so it is server-side behaviour only, unlike `SetRace`.
- ⚠️ `max_mana` clamps **down** only, exactly like `max_hp`, so raising it needs an explicit
  `SetMana`. Without it a warden keeps the template's 14 mana against a ~1600 pool.

#### ⚠️⚠️ THE 10-SECOND RESCALE SWEEP WAS FLATTENING THE WARDEN INTO A TRASH MOB (fixed 2026-08-04)
`M.rescale_zone` walks every unengaged NPC and calls `M.apply(npc, eff, power)` — and it had **no
guard for our own NPCs**, while `M.on_npc_spawn` did. So ten seconds after a warden spawned it was
rescaled to the **trash** level `eff`, rewriting hp, damage and level wholesale and discarding the
boss hp multiple, the boss damage multiplier and `BOSS_LVL_BONUS`. Any warden not engaged within ten
seconds became a trash mob wearing a boss name.
- ⚠️⚠️ **It presents as "the boss hp multiplier does not work", which is why it survived several
  retunes of that multiplier.** The multiplier was applied correctly at spawn and then thrown away.
- Guarded with the same **2000000+** divider `elite_ratio` already uses. It was also catching the
  chest (2000300) and the class aura npcs (2000100-2000115), neither of which should ever be scaled.
- 📌 The rule: **anything AoTv4 spawns is scaled by whatever spawned it**, against rules this sweep
  knows nothing about. Keep the sweep's exclusions and `on_npc_spawn`'s in step — the pet asymmetry
  recorded above is the same mistake in the other direction.

#### ⚠️⚠️ `BOSS_HP_MULT` IS A MULTIPLE OF A REGULAR MOB, NOT OF THE BOSS'S OWN CURVE (2026-08-04)
It used to multiply the boss's own curve hp at `eff + BOSS_LVL_BONUS`, so the ratio a player actually
saw was `curve(eff+2)/curve(eff) × mult` — a number that drifts with the curve's local steepness and
**falls as the rung rises**: ~19x at rung 1, ~6.5x at rung 10, ~5.6x at rung 30. Raising the constant
could never fix that; it just slides the crooked curve up. The warden now **measures** an ordinary mob
(`scale.regular_hp`) and multiplies that, so the ratio is `BOSS_HP_MULT` (**6.0**, was 7.5 and 10.0 before that) at every rung and
`BOSS_LVL_BONUS` is left to do what it should — damage, accuracy and AC.
- ⚠️ `scale.regular_hp` **scales the npc as a side effect** and leaves it at the measured level; the
  caller must re-scale immediately after. Same measure-by-scaling trick as `elite_ratio`.
- ⚠️ It deliberately **excludes the elite ratio**: "regular mob" means trash. An authored elite is up
  to `ELITE_CAP` (4x) tougher, so the warden is only ~1.9x one of *those* — sizing the boss off
  whatever happened to be standing nearby is exactly what makes the number meaningless.
- ⚠️ The fine-trim factor is **shared** between `M.apply` and `regular_hp` (`fine_factor`) — if those
  two disagreed the boss would be a multiple of a mob that does not exist.
- ⚠️ Rungs 1-2 remain **floor-governed** (`blvl × BOSS_HP_FLOOR_PER_LEVEL`), so the ratio there is
  higher than the multiplier. That is the floor doing its job: 6x of an 11 hp mob is not a boss.
  📌 Retuned 10.0 → 7.5 (2026-08-05) → 6.0 (2026-08-06) — each time it played too long once the ratio was genuinely being
  delivered at every rung. Because it multiplies a MEASURED regular mob rather than the boss's own
  curve, retuning is just this one number and needs no per-rung re-check.

#### ⚠️⚠️ Rewards must never be computed from gear at the END of a run
The ledger records **the effective level of every mob actually killed, at the moment it died**
(stamped on the NPC via `SetEntityVariable("delve_eff")` when it was scaled). Clearing the dungeon
naked and putting gear on before turning it in therefore *cannot* inflate anything — the number was
already banked. That makes the whole class of gear-swap exploits **structurally impossible rather
than merely detected**, which is why the ledger keys off the mob and not off the player.
- `M.ledger` returns `kills`, `avg`, `lo`, `hi`. **`avg` is what pays.** `hi − lo` is the *swing* — a
  naked clear followed by a gear-up reads as a low average with a high maximum, and the player is told
  as much on completion.
- ⚠️ The ledger is wiped in `M.enter` **before** entry; a stale one would credit this run with the
  last run's kills, which is exactly the accounting an exploiter wants.
- 📌 The scale reference is the **first client found** in the instance — fine for solo, a known
  simplification for groups (it should become the strongest member, or an average).

#### The score sheet — per-kill value + run history
- **`M.kill_value(eff, named)` is QUADRATIC**: `eff² / 25`, ×3 for a named. Level 25 → 25, 50 → 100,
  70 → 196. ⚠️ Linear scoring would make grinding a soft layer for volume beat clearing a hard one —
  the exact behaviour the scaling system exists to discourage. "Named" uses the same heuristic as
  §17c and `quest_difficulty.pl`: EQ trash is lowercase, named mobs are proper nouns (or lead with `#`).
- Each run also builds a **five-level band histogram** (`45-12,50-20,55-15`), so a run reads as a
  *shape* rather than one average. ⚠️ Bands use `-` internally because `|` and `^` are the wire
  separators and `,` separates the bands.
- **History** in `delve_hist_<charid>`, newest last, **capped at 20** with the oldest dropped — a data
  bucket is one string, and §20 records that an oversized chat line is silently **truncated**, which
  looks like a short list rather than an error. Outcome letters: `C` cleared, `F` died, `A` abandoned.
- ⚠️ Recorded in **three separate places** (clear / death / abandon), not one shared helper, because
  the outcome letter is what makes the row meaningful. ⚠️ In `M.leave` the "still active?" test must be
  taken **before** `RemoveTaskByTaskID` — after it, it always reads false and no abandon is ever
  recorded; and looting the chest also routes through `M.leave`, so without the test every clear would
  be double-counted.
- ⚠️ `M.send_history` resolves the layer **name** server side rather than sending an index: the client
  only ever learns the names of layers it has *unlocked*, so an old run in a now-invisible layer would
  render blank. It is also sorted newest-first server side rather than in the dll.
- **Client tab** = the Death Book pattern (`core_lostwindow.cpp`), and all three of its traps apply
  identically: the toggle is **deferred to `DungeonTick`** (`RefreshHistory` → `DeleteAll` would
  destroy the listbox's rows from inside its own click notification), the selection is read **after**
  `CSidlScreenWnd::WndNotification` (inside it `GetCurSel` is one click behind), and the row index is
  **not** a data index — every click goes through `g_histRowRun[]`.
- ⚠️ `DungeonHistTransport` **preserves which runs were open** across a rebuild, keyed on the run's
  timestamp. The server pushes fresh history after every finished run, and collapsing the sheet each
  time would fight a player reading it. Newest run defaults to open.
- ⚠️ Both server lines route through the single `DungeonParseTransport` entry point, so the `dsp_chat`
  chain needs one call, not two.

## 23. Quest difficulty scoring — `quest_difficulty.pl` — 2026-07-29

`.devcontainer/custom/tools/quest_difficulty.pl` scores every quest on the server as ONE float.
Read-only (database + quest scripts); no server restart, nothing generated into the game.

```
score = LEVEL BAND . AT-LEVEL DIFFICULTY
        floor(1 + level/10), clamped 1..8      0.00 .. 0.99
```
A level 30 quest bands at **4**, a level 60 quest at **7**, so sorting by score sorts by level first
— level being the dominant input. The decimal is how hard it is **assuming you are the right level**.

- **Terms** (weights): turn-ins **0.35** (distinct items 0.75 / extra hand-in steps 0.25), travel
  **0.35**, gear **0.30** (required content tier 0.70 / turn-in item quality 0.30). Each saturates
  via `sat(x, full)` so no single outlier can push a quest out of its band; the decimal is hard-capped
  at 0.99 for the same reason.
- ⚠️ **The gear term is LEVEL-RELATIVE, deliberately** — `over_level()` scores how far the required
  content sits *above* the quest's own level (at-level = 0, +15 levels = 1). Scoring it absolutely
  made every level 60 quest look hard purely for being level 60, which the integer part already says.
  Turn-ins and travel stay absolute: ten hand-ins is ten hand-ins at any level.
- **Both corpora, one model**: `SCRIPT` = 2,743 rows parsed from `quests/<zone>/<NPC>.lua|.pl`
  (`check_turn_in{item1..}` / `check_handin(…, id => n)`); `TASK` = `tasks` + `task_activities`
  (activitytype **1 Deliver / 2 Kill / 3 Loot**, `common/tasks.h`). Only extraction differs.
- Flags: `--zone= --source=script|task --limit= --min= --explain --tsv`.
- Current spread: bands 1→8 hold 351/111/134/312/366/470/785/214, mean decimal **0.313**.

### ⚠️⚠️ Three modelling traps, all of which produced plausible-looking but wrong numbers
1. **Travel is the number of zones you must GO to, not the union of everywhere an item exists.** A
   common drop comes from forty zones; you visit **one**. Unioning them reported quests spanning
   **185-217 zones**. Each item now costs one trip, to the cheapest source available, preferring
   ground spawn → merchant → loot (picking something up beats having to kill for it), and free if the
   quest's own zone can supply it.
2. **A zone that DEMANDS an item does not SUPPLY it.** The scanner registered every turn-in item as
   available in the very zone requiring it, so "can I get this without leaving?" was always true and
   script travel was **exactly zero on all 2,743 rows** — while tasks looked fine, which is what made
   it read like a data gap rather than a bug. Providers are summonitem/loot/ground/merchant **only**.
3. **MIN dropper level, not MAX.** A player farms the *easiest* source. MAX let one high-level
   dropper somewhere in the world pin the gear term at its ceiling — every quest scored exactly
   0.700 — which looks like a constant, not a bug.
- ⚠️ Also: match summonitem **on the actual call**. An earlier regex matched `item_id`/`itemid`
  anywhere in a file and swept up merchant tables and config.
- 📌 **Unit of "a quest" is one NPC FILE**, so an NPC implementing several independent quests reads as
  one many-step quest (`abysmal/Rilwind_Sitnai` shows 19 steps). The steps term saturates at 4 extra
  to blunt this. Real chains spanning several NPCs are only partly captured — via the 1,604 of 5,592
  turn-in ids that some other script summons.
- 📌 Coverage is honest, not total: 5,589/5,592 turn-in ids resolve to real items, but only 57% have
  any loot source, so gear scores 0 where nothing is derivable rather than guessing.

## 22. Combat specials COST ENDURANCE — 2026-07-29

Stock EQEmu charges nothing to Bash/Kick/Backstab/the monk strikes — the only brake is the reuse
timer. Here a damaging special costs endurance **in proportion to the damage it actually dealt**, so
the specials are a resource to spend rather than a rotation to mash. Pure C++, no Lua, no client
change. **Needs a zone rebuild** (and `ruletypes.h` is in `common/`, so expect a wide one).

- **Files**: `common/ruletypes.h` (two rules) + `zone/special_attacks.cpp`
  (`Mob::DoSpecialAttackDamage`, the gate at ~:235 and the charge at ~:333).
- **Rules**: `AoT:SpecialEndurancePct` (**33** — a 200-damage Backstab costs 66 endurance; `0`
  disables the whole mechanic, gate included) and `AoT:SpecialEnduranceMinToUse` (**1**).
  ⚠️⚠️ **Retuned 50 → 33 (2026-08-04) to match the Sinew line, and the two are ONE calibration.**
  `custom/sql/aotv4_sinew_line.sql` returns **damage / 3**, so at 33 percent a Sinew cast funds
  roughly its own damage worth of specials — an endurance economy that closes. At 50 it funded two
  thirds of one, so the bar drained no matter what the player did and the tap read as broken rather
  than as a cost. **Move both together or neither.**
  ⚠️ There is a `rule_values` row for this, so the header default is NOT what the server uses —
  changing the default alone does nothing to a live DB. Check
  `SELECT rule_value FROM rule_values WHERE rule_name='AoT:SpecialEndurancePct'`.
- ⚠️⚠️ **`DoSpecialAttackDamage` IS THE ONLY PLACE THIS GOES.** Bash, Kick, Frenzy, Backstab and
  every monk strike (via `MonkSpecialAttack`) funnel through it, so one edit covers the
  player-pressed path **and** the autoskill auto-fire loop (§19). Per-ability copies would be five
  that drift — the same reasoning that put the skill-gating in one place in §4.
- ⚠️⚠️ **CHARGED AFTER THE HIT, and it has to be.** The cost is a share of the damage and the damage
  is not known until `DoAttack` has rolled it. That is *why* the pre-swing test is a cheap "have you
  got anything left at all" (`SpecialEnduranceMinToUse`) rather than a real affordability check —
  **you cannot price something before you know what it costs.** Don't "fix" the gate into a proper
  cost check; it can't be one.
- ⚠️⚠️ **The reuse timer STILL STARTS on a winded whiff.** The caller begins it after the function
  returns (`p_timers.Start`, e.g. `special_attacks.cpp:490` for Bash) across six-plus call sites, so
  gating there would be six copies. Deliberate as well as convenient: spending the swing is what
  stops a player mashing specials at zero endurance, which is the entire point of the cost. Setting
  `SpecialEnduranceMinToUse` to 0 removes the gate — and with it any reason not to mash.
- ⚠️ **Clients only.** `Mob::GetEndurance` is a virtual returning **0** (`zone/mob.h:674`) and
  `Mob::SetEndurance` a **no-op** (`:677`); only `Client` overrides them (`zone/client.h:751/756`).
  An NPC would silently "pay" nothing, so the `IsClient()` guard is there to stop a later reader
  believing NPCs are costed.
- ⚠️ **A miss costs nothing by construction** — 33 percent of zero damage is zero. No special case.
- ⚠️ Multiply before dividing (`damage * pct / 100`) or a small hit truncates to a free swing. Same
  trap as the Shield Wall split (§17b).
- 📌 33 percent is derived from the Sinew return rather than observed, so it is a *consistent* guess,
  not a tested one. It interacts with the §19 autoskill cap — the cap limits how many specials may
  fire, this limits how long they keep firing.
- **The refill is the Sinew line** (§5, `custom/sql/aotv4_sinew_line.sql`, ids 43318-43323): a damage
  tap returning endurance equal to a third of its damage. This cost is what gave endurance a sink
  worth spending on in the first place, so tune the two together — at the default 50 percent, the
  top tier's 175 endurance buys roughly 350 damage worth of specials.

## 25. ⚠️⚠️ THE STALE-DATABASE TRAP — "a session's work vanished" — 2026-07-24, 2026-07-30

> ⚠️⚠️ **READ THIS FIRST: `/src` IS THE TEST ENVIRONMENT. THERE IS A SEPARATE LIVE SERVER, AND ITS
> DATABASE IS NOT THIS ONE.** (Established 2026-08-04.) Players, bug reports and screenshots come
> from **live**; this container is where the work is built. So the two databases holding different
> characters is **correct and expected**, not the trap below.
> - ⚠️⚠️ **`db_sanity.sh` FALSE-POSITIVES HERE AND WILL KEEP DOING SO.** It compares the datadir's
>   mtime against `/src` artifacts, and in a test environment nobody plays in, the database is
>   legitimately older than the logs and shared-memory blobs — it reported "138 hours older,
>   SUSPECT" on a perfectly good DB. **Do not act on that verdict alone.** Confirm with content
>   markers (`db_version`, and whatever the newest feature wrote) before concluding anything, and
>   never import a snapshot on the strength of the mtime check.
> - ⚠️ **A character being absent here proves nothing.** On 2026-08-04 a live character not existing
>   in this DB read as a smoking gun for the trap below; it is simply a live character. Measure data
>   claims about **live** against live — anything derived from this DB (skill_caps breadth, who has
>   what) describes the test environment only.
> - 📌 **Shipping to live is therefore a real step**: the rebuilt `zone`/`world`, any changed Lua,
>   and `dinput8.dll` to clients. **Prefer a condition-gated migration in
>   `database_update_manifest_custom.h` over a hand-run `custom/sql/*.sql`** — a migration applies
>   itself when world boots on live and no-ops when the condition does not hold, so it cannot be
>   forgotten and cannot fire against data it was not written for.
>
> The rest of this section is about the genuine trap — **two Docker engines, one volume name** —
> which is still real and still costs a session when it bites.

**The single most expensive recurring failure in this project.** It has struck at least twice and
cost most of a session both times. Nothing errors, nothing logs, and the server runs perfectly —
you are just connected to an **older copy of `peq`** than the one your last session's work went
into. Because every symptom points at "the data was lost", the natural reaction is to re-apply SQL
or import a snapshot, and **that is what actually destroys the work** — the real database is still
sitting on disk somewhere the whole time.

**Run `bash .devcontainer/custom/tools/db_sanity.sh` BEFORE starting the stack or applying any SQL.**
It exits 1 and prints why. On 2026-07-30 it reported the datadir as **138 hours older** than the
shared-memory blobs — the diagnosis that had just taken an hour of manual work.

### What it looks like
`git status` is clean and correct, every source file is where you left it, the server boots, and
the DB answers every query — but recent content is simply **absent**. In 2026-07-30's case: the
Sinew line, the Delver's Sigil, all delve tasks and the chest NPC were missing, while the class
auras and achievements (older work) were present, which made it read like a partial failure of one
feature rather than a whole-database problem.

### Why it happens — ⚠️⚠️ TWO DOCKER ENGINES, TWO VOLUMES, ONE NAME
**This is the confirmed 2026-07-30 cause, and it is not what it looks like.** Nothing was on a
container's writable layer, no mount failed, and no data was ever lost. There are **two Docker
engines**, each with its own `/var/lib/docker`, and each holding a volume named `aotv4-mysql-data`.

⚠️⚠️ **THE TWO ENGINES ARE *NOT* THE TWO `docker context ls` ENTRIES.** That was the first guess and
it is wrong: Docker Desktop publishes **both** `default` (`npipe:////./pipe/docker_engine`) and
`desktop-linux` (`npipe:////./pipe/dockerDesktopLinuxEngine`), and they reach the **same** daemon.
Proof — the dev container is invisible to both:
```powershell
docker --context default      stop <dev container id>   # No such container
docker --context desktop-linux stop <dev container id>  # No such container
```
⚠️⚠️ **THE SECOND ENGINE IS `docker-ce` RUNNING INSIDE THE WSL `Ubuntu` DISTRO, AND VS CODE USES IT
EVEN THOUGH VS CODE IS LAUNCHED FROM WINDOWS.** Confirmed from the Dev Containers log
(`Ctrl+Shift+P` → *Dev Containers: Show Container Log*), which is the fastest way to settle this:
```
Running Dev Containers CLI: read-configuration --workspace-folder /mnt/c/AoTv3/AoTv4
Start: Run in Host: /home/michael/.vscode-remote-containers/bin/.../node
docker version --> {"Client":{"Version":"29.4.0","Context":"default"},"Server":null}
Cannot connect to the Docker daemon at unix:///var/run/docker.sock
```
`/mnt/c/...` and `/home/<user>/` are **WSL** paths: the extension runs its CLI inside Ubuntu and
therefore uses **Ubuntu's** `docker-ce` daemon on `unix:///var/run/docker.sock` — a different engine
from Docker Desktop, with its own volumes. **"I always open VS Code from Windows" does not rule this
out**, which is exactly why it took so long to find.
- 📌 The same log shows VS Code helpfully launching `Docker Desktop.exe` when the daemon is
  unreachable (*"backend already running, signaling show-dashboard"*). That **does not help** — the
  CLI is still pointed at Ubuntu's socket, so starting Docker Desktop fixes nothing and makes it look
  like a Docker Desktop problem when it is not.
- ⚠️ A container **keeps running after its dockerd dies**, so the dev container stays usable while
  every `docker exec` from the host fails. Symptom in the log: repeated
  `Port forwarding ... Cannot connect to the Docker daemon`.

**THE FIX — give WSL one engine.** Enable Docker Desktop → *Settings → Resources → WSL Integration →
Ubuntu* (this is the step that matters: it makes `docker` inside Ubuntu resolve to Docker Desktop),
**and** disable Ubuntu's own daemon so it can never shadow it again:
```powershell
wsl -d Ubuntu -u root -- systemctl disable --now docker   # or: service docker stop
wsl -d Ubuntu -- docker ps -a                             # must now list Docker Desktop's containers
```
⚠️ **`wsl -u root` is the way in — this WSL user has NO sudo password**, so anything written as
`sudo …` simply prompts forever. If systemd is not enabled in the distro, `systemctl` errors with
*"System has not been booted with systemd"*; use `service docker stop` + `update-rc.d docker disable`.
Until one of these is done this recurs at random, because it depends on which daemon happened to be
alive when the workspace was opened.

Establish which engine you are on from **inside** the container, which always works:
```bash
hostname                                             # the dev container's own id
grep ' /var/lib/mysql ' /proc/self/mountinfo         # -> /var/lib/docker/volumes/<name>/_data
```
⚠️⚠️ **A NAMED VOLUME IS SCOPED TO ITS ENGINE, SO `aotv4-mysql-data` EXISTS TWICE** — two entirely
separate volumes that share a name. Both containers mount `aotv4-mysql-data` at `/var/lib/mysql`
exactly as `devcontainer.json` says; they simply resolve to **different disks**:

| engine | volume contents |
|---|---|
| `desktop-linux` | the **live** database — 930 MB, `character_data.ibd` dated Jul 30 17:38 |
| `default` | the **Jul 24 import**, written once and untouched since |

So **whichever engine VS Code happens to build the dev container on decides which database you
get**, silently. That is the "drift every once in a while" — it is not random, it is whichever
engine won that day. Verify with:
```powershell
docker inspect <ID> --format "{{range .Mounts}}{{.Type}} {{.Name}} -> {{.Destination}}{{end}}"
```
⚠️ **The fix is to pin every dev container to ONE engine** (`docker context use desktop-linux`, or
VS Code's `dev.containers.dockerPath` / Docker extension context setting). It is *not* a data
migration — the good data is already on a persistent volume and already survives rebuilds **on its
own engine**.
⚠️⚠️ **When deleting the stale copy, name the engine explicitly**:
`docker --context default volume rm aotv4-mysql-data`. The Docker Desktop UI only ever lists
`desktop-linux`'s volumes, so the two are indistinguishable by name there — and deleting the wrong
one causes exactly the loss this section exists to prevent.
- 📌 A **different** mechanism produced the same symptom on 2026-07-24 (§17): a container created
  *before* the volume mount was added kept `/var/lib/mysql` on its own writable layer, so the
  freshly-mounted volume came up empty. Both causes present identically — "the database is old" —
  so diagnose by evidence (below) rather than by assuming either one.

⚠️⚠️ **DOCKER DESKTOP SHOWS ONLY ITS OWN ENGINE'S CONTAINERS AND VOLUMES.**
On 2026-07-30 the dev container's own id was not known to the daemon Docker Desktop was talking to:
```
docker port 1a8ad2c4c550   ->   Error response from daemon: No such container
```
The container list therefore looked *incomplete* when it was in fact complete **for that daemon**.
Do not conclude "there is only one container" from the Docker Desktop UI. Confirm the id you are
running in (`hostname` inside the container) against `docker ps -a`; if it is absent, you are
looking at a different engine/context than the one hosting your shell.
- ⚠️ Two containers running at once also **split the host port bindings**. `d0efd1c563ed` published
  only part of 7000-7029 (missing 7003, 7006-7009, 7018, 7023, 7028) plus no 3307 and no 9001,
  because the other container had grabbed them first — so §0's "zone unavailable" bug was still
  live on a container that looked correctly configured. Port publishing is fixed **at container
  start**: freeing a port later does nothing until that container is restarted.

### How to detect it — the `/src` artifacts are the evidence
⚠️ **`/src` is a BIND MOUNT and survives every container**, so anything on it that was *generated
from* the database is datable proof of what the DB held at that moment. That asymmetry is the whole
trick, and it is what `db_sanity.sh` automates:

| artifact | written by | what it proves |
|---|---|---|
| `build/bin/shared/items`, `shared/spells` | `./shared_memory` | the item/spell tables at that timestamp |
| `build/bin/logs/**` | the running server | the stack was alive, and against which content |
| `build/bin/logs/zone/<zone>_..._inst_id_N_*.log` | a booted zone | the feature genuinely worked then |

If any of these is **hours newer** than the newest file in `/var/lib/mysql/peq`, the database in
front of you did not produce them. On 2026-07-30 `shared/items` contained all ten Delver's Sigils
and `shared/spells` contained `Sinewbite`, while the live DB contained neither — conclusive.
- ⚠️ **`sudo` is mandatory when inspecting the datadir.** The per-database dirs are `drwx------
  mysql`, so an unprivileged `find`/`du` returns nothing and reports **"no data" on a machine
  holding the full database**. Same trap as `docker exec -u root` (§17); it caught `db_sanity.sh`
  itself on its first run.
- ⚠️ **Check content markers by NAME, not only by id range.** An id-range count cannot tell "never
  created" apart from "renumbered". Grep a dump for an apostrophised name with care, too:
  `Delver's Sigil` is written `Delver\'s Sigil` in a mysqldump, so a literal grep finds **zero** and
  looks like proof of absence.

### Recovery — never import a snapshot first
> ⚠️ **First ask whether recovery is needed at all.** If the cause is the engine split above, the
> real database is sitting safe on the other engine's volume and the answer is to *point at it*, not
> to dump and import. On 2026-07-30 a full dump was taken (`peq_live_recovered.sql.gz`, 242 tables,
> verified) before this was understood — harmless as a belt-and-braces backup, but the actual fix was
> to build the dev container on `desktop-linux`. Below is for the case where the data really is
> stranded on a container you are about to discard.
The order that works, and the one CLAUDE.md §17 and `POST_REBUILD_RECOVERY.md` §STEP 0 both insist
on. Every command runs on the **Windows host**; `-u root` throughout.
```powershell
docker ps -a --format "{{.ID}}  {{.CreatedAt}}  {{.Status}}  {{.Names}}"
docker exec -u root <ID> sh -c "hostname; ls -l /var/lib/mysql/peq/character_data.ibd; du -sh /var/lib/mysql/peq"
```
`character_data.ibd`'s date **is the answer** — it is the newest-dated file that always exists.
Then dump straight onto the bind mount, where the dev container can verify it without any port
juggling or `docker cp` of a 750 MB datadir:
```powershell
docker exec -u root <ID> sh -c "service mariadb start; sleep 8; mysqldump --single-transaction --routines --events --databases peq | gzip > /src/peq_live_recovered.sql.gz"
```
⚠️ **Verify before importing and before deleting anything**: `gzip -t`, a `^CREATE TABLE` count
(**242** as of 2026-07-30; it was 233 pre-delve), a `Dump completed on` tail marker, and a spot
check for the newest feature you remember building.
⚠️ **Keep the source container STOPPED, not removed**, until the import is verified — until then its
layer is the only live copy.

### Prevention
- Run `db_sanity.sh` at the start of every session. Add a marker row to it whenever a datable
  feature lands; it is only as good as its markers.
- ⚠️⚠️ **NEVER OPEN OR REBUILD THE DEV CONTAINER WHILE DOCKER DESKTOP IS DOWN OR STILL STARTING.**
  Wait until it is fully up, then open VS Code. This is the suspected trigger for the whole
  2026-07-30 incident: Docker Desktop was failing to load, and a dev container was created at 19:08
  in the middle of that outage. With its engine unreachable, VS Code cannot see the workspace's
  EXISTING container (`d0efd1c563ed`, holding the live DB) and ends up building a fresh one
  somewhere else — which is how a second container appeared on an engine Docker Desktop cannot see,
  on a machine where the project is only ever opened from Windows. Correlation, not proof, but the
  cost of waiting two minutes is nil and the cost of getting it wrong was most of a session.
  📌 The same outage also *hides* the problem: with Docker down, `docker ps -a` cannot be run, so the
  two-container split has to be inferred from datadir mtimes and `/src` artifacts (below) instead of
  simply being read off a list. That inference took about an hour.
- ⚠️⚠️ **Build every dev container on the SAME engine — Docker Desktop's.** This is the whole fix
  for the 2026-07-30 incident. The data is already on a persistent volume and already survives
  rebuilds *on its own engine*; the only thing that goes wrong is being built on the other one.
  Confirm with `hostname` inside the container against `docker ps -a` on the host — **if your
  container's id is not in that list, you are on the wrong engine** and the database in front of you
  is not the one you were working on.
- **Get the database onto the named volume and keep it there.** A volume mount only becomes real for
  a container created *after* the mount exists, so after any recovery the DB must be imported into a
  container that actually mounts `aotv4-mysql-data` (the 2026-07-24 mechanism, §17).
- Prefer **one** container. Two split the host port bindings — on 2026-07-30 the second one silently
  stole 7003, 7006-7009, 7018, 7023, 7028, 3307 and 9001 — and make "which DB am I on?" ambiguous.
- ⚠️ **Never `docker system prune`, "Reset to factory defaults", or "Clean / Purge data"** while
  diagnosing this. Those delete container layers and volumes and are the only way to turn a
  recoverable access problem into real data loss.

## 26. Delve rewards — the Delver's Sigil and the Delve augments — 2026-07-31

Two reward lines come out of the delve (§24), and they work by **completely different mechanisms** —
which is the single most important thing to know before touching either:

| | Delver's Sigil (147500-147509) | Delve augments (147600-147915) |
|---|---|---|
| what it is | a native **evolving item** | **ordinary items**, `evoitem = 0` |
| how it grows | delve **score** | **combining 4 of a tier** in the Refining Crucible |
| where progress shows | the client's native evolving window | nowhere; the item simply changes |

- **Files**: `custom/sql/aotv4_delve_charm.sql`, `custom/sql/aotv4_delve_augs.sql` (**generated** by
  `custom/tools/gen_delve_augs.pl`), `custom/sql/aotv4_delve_remove_native_augs.sql`;
  `zone/aotv4_tiers.h` (tier id blocks), `zone/tradeskills.cpp` (`AoTv4RefineCombine`),
  `zone/lua_client.cpp` (`AddEvolveProgress`); `lua_modules/aotv4_dungeon.lua`
  (`stock_chest` / `roll_aug_tier` / `award_charm` / `on_close_timer`), `aotv4_dungeon_scale.lua`.

### ⚠️⚠️ WHY THE AUGMENTS ARE NOT EVOLVING ITEMS — do not "restore" `evoitem = 1`
They were, for one day, and it does not work: **an augment can never appear in the client's
evolving-item window.** That list is enumerated CLIENT side inside `eqgame.exe` — there is **no
enumeration packet anywhere**, the only server→client evolve message is `OP_EvolveItem UPDATE_ITEMS`
keyed by a unique id the client must already know — and its walk does not descend into augment
sockets. So the sigil showed natively while augments needed a second, custom window, and the design
goal was ONE evolve window. Combining removes the problem rather than working around it.
Three further reasons it was a bad fit, all still true and all invisible from the evolving code:
1. **They never accrue.** `Client::ProcessEvolvingItem` walks `GetInv().GetWorn()` — top-level worn
   items. An augment lives *inside* one and is never visited.
2. **They are never registered.** `EvolvingItemsManager::DoLootChecks` is the only thing that inserts
   a `character_evolving_items` row and is gated on `EQUIPMENT_BEGIN..EQUIPMENT_END`
   (`common/evolving_items.cpp:80`).
3. **Their in-memory evolve state is never loaded.** `SharedDatabase::GetInventory` rebuilds each
   augment with `PutAugment` → `CreateItem` (`shareddb.cpp:751`) — a fresh instance — then attaches
   evolve state only to the **host** (`:756`).
- 📌 A **runeword** system (D2 style: runes in sockets, in order, transform the base item) was
  evaluated against `EQEmu_Runewords_Source_Package` and **not built**. It drops in cleanly at the
  API level, but `MatchesRecipe` requires every socket past the last rune to be EMPTY, which fights
  the stat augments for the same sockets. Parked, not rejected.

### ⚠️⚠️ `required_amount` IS A PER-LEVEL COST, NOT A CUMULATIVE TOTAL (the sigil)
`current_amount` **resets to 0 on every evolve**, so each row states only what THAT level costs.
This was documented backwards for a day. Three things make the reset non-obvious:
1. the evolve path never transfers the amount — `SetEvolveCurrentAmount` appears ONLY in the XP
   transfer window paths (`client_evolving_items.cpp:469/471/517/519`);
2. the swap is `RemoveItemBySerialNumber` → `DeleteItemInInventory`, which **soft-deletes the row**
   (`zone/inventory.cpp:988`);
3. the replacement comes from `database.CreateItem()` — a fresh instance at 0 — and `DoLootChecks`
   inserts a NEW row for it.
⚠️ **Level 1 cannot tell the two readings apart** (same number either way), so a single observed run
at level 1 confirms nothing. That is exactly why the wrong version survived a live test.

### ⚠️ Evolving rewards ACTIVATE THEMSELVES — `AoTv4EnsureEvolveActivated` (zone/lua_client.cpp)
Gating accrual on the `activated` flag looks correct and is a trap: `NewEntity()` defaults it to 0,
and the native evolve creates a **brand new row** for the replacement item — so an item that was
active comes back INACTIVE after every evolve and silently stops progressing. The flag exists
natively to choose which item soaks a SHARED experience pool; our score is awarded explicitly, so it
buys nothing. Activate and carry on.
⚠️ `DoEvolveItemToggle` (`client_evolving_items.cpp`) had a **null dereference** reachable the moment
an evolving item could sit in a socket: `_HasItem` finds augments but returns the sentinel
`invslot::SLOT_AUGMENT_GENERIC_RETURN`, and `GetItem()` on that is nullptr. Guarded; keep the guard.

### The augment generator — `gen_delve_augs.pl`
316 rows in three CONTIGUOUS id blocks: **T1 147600-147615** (16, weight 2, single stat, exhaustive
one-per-stat-line), **T2 147616-147715** (100, weight 4, 1-2 lines), **T3 147716-147915** (200,
weight 8, 2-3 lines). hp/mana/endurance pay **10 points per weight**, so the tiers read 20/40/80 on
those lines and 2/4/8 on everything else — without that a "2 hp" T1 is a dead roll.
- ⚠️⚠️ **THE RNG IS A FIXED-SEED LCG AND MUST STAY THAT WAY.** These ids are live on player
  characters; a regen with a different seed silently rewrites the stats of augments people are
  wearing. Perl's own `rand()` is deliberately unused — it is per-process seeded and not stable
  across perl builds.
- ⚠️⚠️ **`sort` EVERY `keys` WHOSE ORDER CAN REACH THE OUTPUT.** Perl randomises hash key order per
  process; a bare `keys` in the weight distributor made the generator non-deterministic (three runs,
  three different files). **Verify after any change by regenerating and diffing.**
- ⚠️⚠️ **Rolls are deduped on the STAT SIGNATURE, not the name.** The first version deduped names
  only — when two rolls came out stat-identical it appended a roman numeral and kept both — which
  left **10 of 40 tier 2 rows redundant, 6 of them the same `CR 4`**. A quarter of the pool was
  wasted and Frost was six times likelier than any other single-stat T2.
- ⚠️ The requested count must stay well under the combination space or rejection sampling hangs:
  T2 weight 4 over 1-2 lines is ~376 combinations, T3 weight 8 over 2-3 lines is ~12,600.
  `$MAX_REROLLS` turns overshoot into a loud failure instead of a spin.
- ⚠️ `items.Name` is **varchar(64)**; longer is silently truncated, which can also collapse two
  distinct names into one. The generator widens the name then falls back to a roman numeral.
- ⚠️ `nodrop`/`norent` are **INVERTED**: `nodrop = 0` is No Drop, `norent = 1` is permanent.
- ⚠️ The DELETE clears the whole reserved band (147600-148199), not just the current rows — an
  earlier layout ran to 148007 and would otherwise strand its tail as orphans.
- ⚠️ **`items` is shared memory**: world down → `./shared_memory` → restart.

### ⚠️⚠️ The crucible REFUSES an augmented item — it used to eat the augment (fixed 2026-08-04)
The upgrade is `DeleteItemInInventory` + `SummonItem`: the four inputs are **destroyed** and a fresh
item handed back, so anything socketed into them was destroyed with them, **silently**. Reported from
play: *"the aug that was in one of the items was lost"*.
- ⚠️ **It cannot be fixed by carrying the augments across instead.** The crucible produces a
  *different item id* with its own socket layout — a Mythic has three sockets, its base has one — so
  there is no guaranteed home for what was in the old item. Refusing is the only answer that cannot
  destroy something.
- ⚠️ The check is **its own pass before the grouping loop**, so the refusal happens before a single
  `DeleteItemInInventory` runs. Detecting it mid-loop would already have consumed an earlier group.
- ⚠️ It refuses the **whole** combine, not just the offending group — a partial success plus a warning
  is how you get someone re-clicking and losing the rest.

### The Refining Crucible does two different jobs — `AoTv4RefineCombine`
Item **2000060**, gated by item id (not bagtype). Four IDENTICAL gear items become 1 of the next gear
tier (+`AOTV4_TIER_STEP`); four **delve augments OF THE SAME TIER** become 1 **random** augment of the
next tier.
- ⚠️⚠️ **NOT "4 identical" for augments, and that difference is the point.** T2 and T3 have 100 and
  200 variants, so requiring four of the same roll would take hundreds of drops per upgrade. Four of
  a TIER is what makes the loop work.
- ⚠️ Each produced item rolls **independently** — combining eight gives two different results.
- ⚠️ Tier 3 is the top; four T3s are left alone rather than silently eaten.
- ⚠️⚠️ **THE TIER ID BLOCKS ARE MIRRORED IN THREE PLACES AND NOTHING ENFORCES IT**:
  `gen_delve_augs.pl`, `zone/aotv4_tiers.h`, and `aotv4_dungeon.lua M.AUG_TIER_BLOCK`. Change the
  variant counts and all three must move together — nothing fails to compile, the chest and the
  crucible just start handing out the wrong tier.
- 📌 `AOTV4_TIER_STEP`/`AoTv4TierBaseId` were migrated out of tradeskills.cpp into
  `zone/aotv4_tiers.h` when this landed (the header's own note says to migrate a duplicate the next
  time its file is touched). **Four duplicate sites remain**: loot.cpp, npc.cpp, questmgr.cpp,
  attack.cpp.

### The chest, the loot and the close timer
- ⚠️⚠️ **THE REWARD IS PLACED AT SPAWN AND THE DELVE DOES NOT CLOSE IMMEDIATELY.** The
  `on_chest_looted` hook fires on the chest's **death** — the instant the corpse appears, before
  anyone could have looted it. It used to call `M.leave` there, which destroyed the instance out from
  under the player **and took the augment and the platinum with it**. It now starts a client timer
  (`delveclose`, 120 s). ⚠️ A client timer **repeats** — `StopTimer` first in the handler, and
  `M.leave` stops it on every other way out.
- ⚠️ Loot goes on the **npc at spawn**: a corpse built from a lootless npc has nothing to add to.
  `AddCash(copper, silver, gold, platinum)` — **that argument order**.
  ⚠️ The coin is credited the moment the loot window OPENS (`AddMoneyToPP`, `corpse.cpp:1404`), and
  splits across the group if auto-split is on. The augment still has to be dragged out.
- **Coin = 10 platinum per rung, doubled under any affix** (non-Standard mode).
- ⚠️⚠️ **`eq.spawn2` RETURNS A `Lua_Mob`, NOT A `Lua_NPC`.** `AddItem`/`AddCash`/`ScaleNPC` are bound
  on Lua_NPC only, so calling one on the spawn2 result is *"attempt to call method 'AddItem' (a nil
  value)"* at runtime with nothing to warn you at write time. `CastToNPC` **is** bound (on
  Lua_Entity) — unlike `CastToClient`, so §24's entity-list workaround does not apply here.
  ⚠️ The cost when it bit was far wider than the chest: the error aborted the rest of
  `on_task_complete`, so the clear also paid no score and wrote no history. **The accounting now runs
  BEFORE the chest is touched** — everything irreplaceable is settled first.
- ⚠️⚠️ **DELVE MOBS DROP NOTHING** (`M.on_npc_spawn` → `ClearItemList` + `RemoveCash`). These are DoN
  zones and the stock drops are from an expansion the server has not unlocked. Done at spawn and
  **not** by editing loottable rows, because these are instances of REAL zones — stripping the tables
  would empty those zones for everyone, permanently. The nine stock augments that dropped in the six
  delve zones are removed from `lootdrop_entries` (verified to drop nowhere else); the two cosmetic
  ornaments are deliberately kept.

### ⚠️⚠️ THE SIGIL CURVE AND THE SCORING FUNCTION ARE ONE CALIBRATION, NOT TWO
`kill_value` is **`eff * eff`** (×3 named), no divisor and no minimum clamp. The sigil ladder
(**`4400 * n^2`**, written as per-level increments) is DERIVED from it — change one and the other is
wrong. **Delve score feeds the SIGIL ONLY**; augments are combined, not fed.
- ⚠️⚠️ **RETUNED `24000 * n^2` → `4400 * n^2` (2026-08-01) BECAUSE THE LEVEL CAP BECAME 30.** Exactly
  one input of the derivation changed: it assumes a player working on charm level n sits at rung
  **`7n`** (charm 10 ↔ rung 70), which was right at a cap of 70; a rung is only clearable if you can
  fight mobs scaled to it, so at a cap of 30 the reachable ceiling is ~30 and the mapping is **`3n`**.
  ⚠️ Run income is **quadratic in the rung**, so cutting the ceiling 70 → 30 does not halve run scores,
  it drops them by `(30/70)² ≈ 5.4×`. **Halving the cap does NOT mean halving this constant** — that
  intuition gives 12000 and is wrong by more than a factor of 2. Re-derive, don't scale.
  📌 `aotv4_dungeon.M.MAX_LEVEL` is deliberately **left at 70** (owner's call), so rungs above ~30 exist
  and stay unreachable. That is why the derivation uses the **character cap**, not `M.MAX_LEVEL` —
  reading the rung count puts it straight back to `7n`.
  Pace after the retune: ~90 runs climbing, ~28 parked at rung 30, ~25,600 farming rung 1.
- ⚠️ **`items_evolving_details` IS NOT SHARED MEMORY** — `EvolvingItemsManager::LoadEvolvingItems`
  reads it straight from the content DB (`common/evolving_items.cpp:31`) and is called once from
  `zone/main.cpp:376`. **Retuning the ladder alone needs only a zone restart**, no world down and no
  `./shared_memory`. (The sigil's `items` rows *are* shared memory; the charm SQL's header used to
  claim the ladder needed the same, which is wrong and would have cost a needless full cutover.)
- ⚠️⚠️ **`character_evolving_items.progression` is `double(22,0)` — ZERO decimal places**, i.e. an
  integer percent. Anything under 1% stores and displays as **0**, so a sigil that is genuinely
  accruing reads as "evolution experience didn't gain". Diagnose with `current_amount`, which is the
  real banked figure, never the percentage. This is what a 211-score run against the old 96,000
  level-2 requirement looked like (0.22% → 0).
- ⚠️⚠️ It used to be `floor(eff*eff/25)` clamped to a minimum of 1, and `floor(49/25)` is 1 — so
  **every effective level from 1 to 7 paid identically**, the quadratic did not engage until eff 8,
  and farming rung 1 was as efficient as rung 7.
- ⚠️⚠️ The ladder used to **double** each level (150, 450 … 76650). Doubling is EXPONENTIAL in the
  charm level while run score is QUADRATIC in the rung: charm levels 2-5 each took **under one run**,
  and the full chain was 18 runs against a stated design of ten per level. **An absolute number here
  means nothing on its own** — always divide it by what a run is actually worth.
- ⚠️⚠️ **UNSCALED MOBS MUST NEVER BE SCORED AT THEIR OWN LEVEL.** These are DoN zones, so an unscaled
  mob sits at level 60-70; `record_kill`'s fallback is the RUNG, never `npc:GetLevel()`. Nine such
  kills in a **level 3** delve were worth 4,900 each and produced a run score of **59,622** — enough
  to evolve the sigil outright — from a dungeon whose other 32 creatures were level 1. Three separate
  faults had to line up: the instance populates **before the player finishes zoning in** (so
  `on_npc_spawn` had no reference client and returned without scaling), the rescale sweep's power
  deadband was a **whole-sweep early return** that did nothing on any run after the first, and the
  ledger paid out at native level. All three fixed: with no player present a mob scales to the layer
  at power 1.0, the deadband is now a per-mob test that never skips an unstamped mob, and the
  fallback is the rung.

### 📌 OPEN — the level 35 rework invalidates most of the numbers above
The level cap is moving from 70 to **35**. Nothing here has been re-tuned for it, and the mismatch is
large enough to matter:
- **The augments are 1-7× an entire item at that cap.** Gear reachable at ≤35 averages **7.3 stat
  points / 6.3 AC / 10.8 HP** (banded by minimum dropper level — `reclevel` is useless for this,
  classic gear mostly leaves it 0, which is the §23 lesson again). A single T3 is 8 stat points or
  80 HP. Deliberately left as-is for now.
- **The delve is 70 rungs and `kill_value` is `eff²`**, so halving the ceiling **quarters** top-end
  run scores; the sigil ladder is then off by ~4×.
- `quest_difficulty.pl` bands on `level/10` clamped 1-8 and would only ever produce 1-4.
- 📌 A **craft-to-a-real-drop** system was designed but not built: feed augments into socketed gear
  and receive an actual droppable item whose stats match. The reachable pool at a 35 cap is **1,353
  items** banded 251 / 486 / 616 by minimum dropper level, which maps cleanly onto the three augment
  tiers. Parked until the cap rework lands, because the augment weights are an input to the matching.

## 27. Zone sharding (live's `/pick`) — NATIVE, built, deliberately NOT enabled — 2026-07-31

EQEmu already has live's picking system and it needs **no custom code**. Recorded here because it is
easy to miss and easy to re-implement by accident.

- **The switch is per zone**: `zone.shard_at_player_count` (0 = off). **0 of 618 zones use it today.**
- **Assignment is automatic on zone-in** (`zone/zoning.cpp:820`): it counts players per instance of
  that zone, drops you in the first shard **under** the threshold, and if all are full **creates a
  new instance** and puts you there. Players do not choose; they just land somewhere with room.
- **`#zoneshard`** is the player-facing command and is already `AccountStatus::Player` (access 0).
  Bare, it opens a saylink menu of live shards with populations (`Client::ShowZoneShardMenu`);
  `#zoneshard <zone> <instance>` jumps to one. ⚠️ `instance_id = -1` is the sentinel for "the normal
  unsharded zone", not an error.
- ⚠️ **Refused in combat** (`GetAggroCount() > 0`).
- ⚠️ Shards are created with a ~100 year duration (3,155,760,000 s), so they persist rather than
  expiring under the people standing in them.
- Rule `Zone:ZoneShardQuestMenuOnly` (currently false) suppresses the automatic assignment so only
  quests surface the menu — i.e. makes sharding opt-in.

### ⚠️⚠️ EVERY SHARD IS AN INSTANCE, SO EVERY SHARD COSTS A ZONE PROCESS AND A ZONE PORT
This is the reason it is off. The port range was widened to **7000-7029** on 2026-07-30 because six
was not enough for the `zone` launcher's **28 dynamics** (§0), and the Delve already burns an instance
per run (§24). Sharding stacks on top of both, and the failure mode is the one already seen: *"That
zone is unavailable"* and a bounce to character select. **Before enabling it anywhere, raise
`launcher.dynamics` AND the `appPort` / `eqemu_config.json` port range together** — those two must
always match, per §0.
📌 Enabling it later is one UPDATE on the target zone plus that headroom; nothing needs rebuilding.

## 28. Town factions — nobody is killed in a town for their race — 2026-08-01

All 16 races may be any class and go anywhere (§14 + the region system), but stock EQ faction is
built the opposite way round: an Ogre is born at −1000 with Kelethin and is cut down on sight by the
guards of a city the server has just told him he may walk into. Fixed by flooring the faction **con**
at Dubious for a curated set of civic factions. **Needs a zone rebuild** (`ruletypes.h` is in
`common/`, so it is a wide one).

- **Files**: `common/ruletypes.h` (`AoT:TownFactionFloor`), `zone/zone.h` + `zone/zone.cpp`
  (`LoadTownFactions` / `IsTownFaction`), `zone/client.cpp` (`Client::GetFactionLevel`),
  `custom/sql/aotv4_town_factions.sql` (the table + the 74-row list + the full derivation).
- ⚠️⚠️ **EVERY aggro branch requires the con to be exactly `FACTION_THREATENINGLY` or
  `FACTION_SCOWLS`** (`zone/aggro.cpp:326`, `:516`, `:545`) — and **`AlwaysAggro()` / `npc_aggro`
  sits in the ELIGIBILITY half of those conditions, not the faction half**, so an aggro-flagged NPC
  still needs a hostile con to swing. That is why the floor works at all, and equally why it would
  pacify **anything** it were applied to. Scope is the whole safety argument here.
- Thresholds are rules (`Faction:*FactionMinimum`): below −750 Scowls, −750..−501 Threatening, −500
  and up **Dubious**. `CalculateFaction` (`common/faction.cpp:57`) sums
  `earned + base + class_mod + race_mod + deity_mod`, so a fresh character's con **is** the race mod.
- ⚠️ **Dubious, not Indifferent.** The city still visibly dislikes you; it just does not murder you.
  The stock **merchant fix** immediately above our block (`client.cpp:8342`) already treats Dubious
  as tradeable, so shops keep working with no second exception.
- ⚠️ **The floor runs BEFORE the `CheckAggro` line**, which restores `THREATENINGLY` for an NPC that
  already has aggro on you — so a guard you attack still fights back. It prevents being attacked for
  existing, not combat you started.

### ⚠️⚠️ Why a con floor and NOT an edit to `faction_list_mod`
There are **7,134 negative race modifiers across 499 factions** (worst −2000) and clamping them was
the obvious move. It changes stored faction **values**, which is what quests read. **144 quest files**
test `e.other:GetFaction(e.self)`, and in EQ **lower is better** (Ally 1 … Scowls 9), so all of them
are "good enough?" tests at the opposite end of the scale from anything the floor touches — it only
ever moves 8/9 → 7. The four that test the hostile end were checked individually and all still pass:
`erudnext/Collier.lua` (`>= 7`, satisfied *by* a floor of 7), `erudnext/Weligon_Steelherder.lua`
(`> 5`), `felwithea/Tynkale.lua` (`> 4`), `westwastes/Sontalak.lua` (`>= 5`).
⚠️ **Re-verify that list if the floor is ever raised above Dubious** — Collier is the one that breaks.

### ⚠️⚠️ Why a FACTION list and not a ZONE list
**Kelethin is not a zone** — it is a treetop city inside `gfaydark`, which is also a real hunting
zone full of Crushbone orcs. A zone-scoped floor must choose between leaving Kelethin's guards lethal
and pacifying the orcs standing under it. Scoping by the NPC's own faction separates the two and
covers every other city for free. (`cancombat = 0` fails for the same reason, and more bluntly:
`GetFactionLevel` returns `FACTION_INDIFFERENTLY` outright when `!zone->CanDoCombat()`, `:8307`.)

### How the 74 were derived — and the two traps in deriving them
Candidates must be able to be hostile **by race alone** (some race below −500 before any earned
faction), then: named like a civic body, **and** show commerce — own merchants, or a majority of
their NPCs standing in a zone with 10+ merchants.
- ⚠️⚠️ **The commerce test is what keeps dungeon and raid factions with civic names OUT**: Nest
  Guardians (2,769 NPCs), Thunder Guardians (2,429), Citizens of Takish-Hiz (2,257), Inhabitants of
  **Hate** (900), Fallen Guard of Illsalin, Guardians of Veeshan, Befallen Inhabitants. All match the
  naming; all have zero merchants outside cities.
- ⚠️⚠️ **THE NAME TEST IS A KEYWORD LIST, NOT A CLASSIFIER, AND IT SILENTLY UNDER-COVERS.** It is
  built from Antonican and Faydwer vocabulary (*Merchants of, Guards of, Knights of, Militia,
  Residents*) and **nothing in Kunark is named that way** — a per-hub coverage check found Cabilis
  protected only by `Cabilis Residents` while its actual city guard, **`Legion of Cabilis`**, was
  still killing people on sight. Five Iksar guilds and `Jaggedpine Treefolk` were hand-added after
  verifying commerce. **📌 Check coverage per city after any change here; do not trust the name test.**
- The remaining gap was then closed by sweeping on **commerce alone, no name test**, which found six.
  Only three were towns (Heretics/Paineel, Wolves of the North/Halas, Gem Choppers). ⚠️ **The other
  three are raid content** — `Claws of Veeshan` (Skyshrine's dragons; Skyshrine and Great Divide both
  have merchants, so it scores 66% "city") and the three Seru factions (Sanctus Seru is literally a
  city *and* a raid target). **Commerce identifies civilisation, not friendliness** — which is why
  that sweep is a diagnostic, not the rule.
- ⚠️ **Deliberately left hostile** after being checked: `The Forsaken` (0 merchants; only 6 of its 263
  NPCs are in Cabilis — pacifying 257 wilderness mobs to fix 6 in town is the wrong trade),
  `Tunare's Scouts` (0 merchants, half its NPCs are inside Crushbone), `Pack of Tomar` (Dreadlands).
- ⚠️⚠️ **NEVER add a monster faction.** Crushbone Orcs, Sabertooths of Blackburrow, Goblins of
  Mountain Death, Frogloks of Guk, Trakanon and Vox all stay lethal. Adding one stops that content
  aggroing **server-wide, in every zone**, with no other symptom.
- ⚠️ Read at **zone boot**, not shared memory (only `items` and `spells` are shared) — editing the
  table needs a zone restart, no `./shared_memory` rebuild. A missing table is not an error: the
  server just behaves like stock.
- ⚠️ `faction_list_mod`.`mod` is a **MariaDB reserved word** and needs backticks everywhere.

## 29. Spell ranks, kept spells, and the parchment economy — 2026-07-31

**This system was live and completely undocumented here until 2026-08-01** — the only record was
`SPELL_RANKS.md`, a design draft still headed *"proposal, nothing built"*, which `.gitignore` was
also excluding (§0). Read that file for the design reasoning; this section is what actually shipped.

Three things are deliberately separate, and conflating them is the main way to misread the code:

| | what it is | survives death? |
|---|---|---|
| **KNOWN** | every spell this character has ever been awarded | **yes**, permanent |
| **RANK** | per BASE spell, 1-5 | **yes**, permanent |
| **KEPT** | at most **2** spells you START a run holding | this is the only part death touches |

- **Files**: `lua_modules/aotv4_spell_ranks_sys.lua` (all the logic), `lua_modules/spell_ranks.lua`
  (**generated** base → `{r2,r3,r4,r5}` map), `custom/tools/gen_spell_ranks.pl`,
  `custom/sql/aotv4_spell_ranks.sql` (the rows), `custom/sql/aotv4_spell_rank_economy.sql` (the two
  items + the global drop). Buckets, all per character: `spellrank_` / `spellknown_` / `spellkeep_`
  (permanent) and `pickspent_`.
- ⚠️⚠️ **A RANK IS A DIFFERENT SPELL ROW, NOT A MODIFIER.** 188 base spells × 4 ranks = **752 rows at
  43576-44327**, named `"… Rk. II"` … `"Rk. V"`. "Applying a rank" scribes a *different id*, which is
  why the native spellbook shows the real damage and the real mana cost with no client work at all.
- ⚠️⚠️ **THE NAMING IS LOAD-BEARING.** `gen_stock_pool.pl` filters the offerable pool with
  `name NOT LIKE '% Rk. %'` (§5), so the picker can only ever award a **base** spell, which this
  module then upgrades on the way in. **Renaming the rank rows would drop 752 upgraded spells straight
  into the reward pool.**
- ⚠️⚠️ **RANK IS A PROPERTY OF THE CHARACTER, NOT OF A SCRIBED COPY.** Earn rank III on a spell once
  and *every* future award of it arrives at rank III — from the picker, from a kept slot, from
  anywhere. It lives in a data bucket, so it survives the roguelite wipe like AA does.
  ⚠️ Therefore **keeping is NOT how you protect a rank** — the rank was never at risk. Keeping buys
  only the spell *in hand at level 1*, paid for by forfeiting the level-up pick at the level that
  spell was originally taken (`pickspent_`). Two kept, two picks lost: neutral in count, positive in
  control.
- ⚠️ **`ranked_id()` is the single funnel.** Everything that awards a spell goes through it, so there
  is one place that knows how to resolve base → owned rank. Do not add a second.
- **Economy**: **Parchment Fragment 147920** (one per spell destroyed on death, so income scales with
  how deep the run got — a full climb scribes ~31) and **Ink of the Lost 147921** (**1.5% per kill**).
  ⚠️⚠️ **NEITHER DROPS AS AN ITEM ANY MORE — BOTH ARE GRANTED DIRECTLY AS CURRENCY (ink: 2026-08-06,
  migration v26).** Fragments never did; ink used to be a `global_loot` item converted afterwards, and
  that route needed **three** moving parts to be safe (see the EVENT_LOOT trap below). Granting the
  currency deletes the whole class of problem: **there is no item, so nothing can be placed, removed,
  duplicated or stranded.** `grant_ink_on_kill` fires from `global_npc.event_death_complete`.
  ⚠️ **One roll per corpse, paid to the killer** — matching the old behaviour deliberately, because
  §31 keeps global loot **shared** under individual loot for exactly this reason ("1.5 percent
  multiplied by group size is a different drop rate").
  ⚠️ `absorb_ink` **survives as a migration path only**, still called from `event_connect`, so copies
  players were carrying before the change are absorbed rather than destroyed by the next wipe.
  📌 **Do not reintroduce an EVENT_LOOT conversion for any currency item** — the placement ordering
  below makes it wrong in both directions.
  Costs live in `M.COST` and **double for fragments while ink is flat +10**:
  rank 2 = 30/10, 3 = 60/20, 4 = 120/30, 5 = 240/40.
  ⚠️ The two scale differently **on purpose**: fragments come from *dying* so they track how hard you
  have been pushing, ink comes from *killing* so it tracks time played. A rank costs both.
  ⚠️⚠️ **BOTH ARE ALTERNATE CURRENCIES (57 = fragment, 58 = ink) AND EQEmu DOES NOT AUTO-CONVERT THE
  ITEM ON PICKUP** — nothing in the loot path consults `alternate_currency`. The item row still has to
  exist: a currency is defined as a currency/item **pair** and the client reads its name and icon from
  the item.
  ⚠️⚠️ **THE INK CONVERSION CANNOT HAPPEN IN `EVENT_LOOT`, AND DOING IT THERE DOUBLED THE DROP (fixed
  2026-08-04).** `Corpse::LootItem` fires EVENT_LOOT at **corpse.cpp:1740** but does not place the item
  in the bags until `AutoPutLootInInventory` at **:1843**. The old hook added the currency and then
  called `RemoveItem` against bags that did not contain the ink yet — the removal silently did nothing
  and the engine handed the item over immediately afterwards, so the player kept **both**. Reported as
  *"you double loot ink of the lost … had 7 before i looted that one"*.
  ⚠️⚠️ **RETURNING NON-ZERO FROM EVENT_LOOT IS NOT THE FIX — IT IS AN INFINITE-CURRENCY BUG.** A
  non-zero return sets `prevent_loot`, and that branch (**corpse.cpp:1803**) queues the ack, deletes
  the instance and **returns before** the `RemoveItem(item_data->lootslot)` at **:1865** that takes the
  item off the corpse. The ink would stay on the corpse and pay out again on every click.
  ✅ The conversion is therefore **deferred**: `event_loot` arms a one-second `inkconv` client timer and
  `M.absorb_ink` runs afterwards, once the item really is in the inventory. It **sweeps the bags**
  rather than trusting what was looted, so ink arriving by trade, quest or GM is absorbed identically,
  and it is called from `event_connect` as a backstop so ink held across a logout is never stranded as
  an item the next death would destroy.
  ⚠️ `StopTimer` first in the handler — a client timer **repeats**, the same trap recorded for
  `delveclose`.
  ⚠️⚠️ **THE PAYOUT COUNTS THE SPELLBOOK, NOT `ranks.chain` — and counting the chain was a real bug
  (fixed 2026-08-04).** `M.on_death_before_wipe` used to walk `ranks.chain` and test each base and
  rank id, but that map only covers the **188 base spells that have rank rows** while the offerable
  pool is ~1,996 — so **every scribed spell without a rank chain paid nothing**. A character dying
  with 28 spells was paid only for the few that happened to be ranked. Reported from play as *"not
  getting scrolls correctly"*. It now walks the 720 book slots via `GetSpellIDByBookSlot`, which is
  the only count that stays correct as the pool changes: anything scribed is destroyed by the wipe
  whether or not the rank system has heard of it.
  ⚠️ **Empty book slots are `UINT32_MAX`, not 0** (`zone/spells.cpp:6120`) — testing `> 0` alone
  counts all 720 as spells.
  ⚠️ **`Client::GetSpellIDByBookSlot` bounds-checks with `<=`, not `<`** (`spells.cpp:6024`), so slot
  **720 reads past the array**. Iterate `0 .. SPELLBOOK_SIZE - 1` and do not "fix" the loop to be
  inclusive.
  ⚠️ The **class auras (43500-43515) are excluded** — death_loss re-scribes them immediately after the
  wipe, so they are not destroyed and must not be paid for. **Kept spells still count**, deliberately:
  they are re-scribed rather than spared, and excluding them would make keeping the best way to farm
  currency as well as the best way to keep a spell.
  ⚠️ **Tune `M.COST` ONLY** — it is sent to the client as `SPELLRANKCOST` and the window renders
  whatever it says. There is no second copy in the dll, deliberately (the `kIcons[]` drift trap, §3).
- ⚠️⚠️ **FEEDBACK GOES TO THE WINDOW, NOT TO CHAT** (`SPELLRANKMSG`, written into the requirement panel
  under the description). Keeping and ranking happen inside a window that is already on screen, so a
  chat reply is both noise and the wrong place to look — and it is the player's own chat.
  ⚠️ **Both halves are needed**: the dll must also swallow the `/say spellkeep`-style **echoes**, or
  the commands stay visible even with every reply silenced. Say commands are intercepted in
  `Client::ChannelMessageReceived` (`spellkeep`, `spellrelease`, `spellrank`, `spellrankreq`,
  `spellkept`), the same place as the AdvLoot ones (§16), so they never reach chat or quests.
- ⚠️ **Ranks cover damage and healing only.** Buffs have no hook in either direction — a stat buff's
  magnitude is read straight from the spell row in `SpellEffect` — so there is nothing to scale.
- 📌 The cost curve is a first guess and should move once fragment income is *observed* rather than
  modelled. Same caveat as the sigil ladder in §26, and the same rule applies: divide it by what a
  run is actually worth before deciding it is too high or too low.

## 30. ⚠️ What is BUILT but NOT YET EXERCISED IN GAME (as of 2026-08-01)

Everything below compiles, applies and has been verified against the database or by static analysis,
but **no character has actually performed the action**. Listed so a tester knows where to look first
and so nobody reads "done" as "proven".

- **Town faction floor (§28)** — no character has walked into a hostile city as a KOS race. Fastest
  check: an Ogre in Kelethin or Felwithe, and an Iksar-hated race in Cabilis (Cabilis was the case
  that was actually broken and hand-fixed). Confirm guards ignore you AND that a guard you *attack*
  still fights back.
- **Crafting always yields Mythic** — no combine has been made. Any wearable recipe will do; the
  result should be the `Mythic …` name.
- **The two quest tier-matching fixes** — hand a **Mythic** copy of a craftable turn-in item to one of
  the 25 affected NPCs (Ghoulbane, Jagged Blade of War, Trueshot Longbow, Robe of the Lost Circle …).
  Those quests can now only ever *receive* a Mythic, so this is the sharp case.
- **The retuned sigil ladder (§26)** — one delve clear should now move the charm's percentage visibly.
  ⚠️ Remember `progression` is `double(22,0)`, so anything under 1 percent still reads **0**; judge it
  by `current_amount`, not the bar.
- **Delve rewards end to end** — no death has paid Parchment Fragments, no Ink of the Lost has
  dropped, no region-unlock achievement has fired, and the three Resplendent hub NPCs
  (Wayfinder Alessa / Reforger Vael / Herald Coren) have never been hailed by a real player.
- **Spell ranks (§29)** — the whole chain (keep 2, forfeit the picks, spend fragments + ink, receive
  the `Rk.` row) is untested in play.
- ⚠️⚠️ **Individual loot (§31)** — nothing has died with two players on it. The sharp test needs
  **two grouped characters killing ONE npc**, then confirming the lists are disjoint in **both**
  windows (the `/advl` window *and* the stock right-click one — the native path was the hole, so
  checking only ours proves nothing). Also confirm both were paid coin **without opening anything**,
  and that a GM `#peekloot` shows the corpse holding both rolls.

### Added 2026-08-07 — verified in the DATABASE, not yet in play
Each of these was confirmed by query (migrations applied, rows present, counts correct) and by a
build, but nobody has performed the action:
- **The tradeskill gear rework (§32, v29/v30)** — no character has worn a piece. The sharp test is the
  **Skills window**: raw 300 with the hand piece must read **345**, not 300 and not 396 (the
  double-application this deliberately avoids). Also click a mask and confirm the illusion lasts
  **30 minutes**, not 3, and that it fires at all (`maxcharges` was 0).
- **The per-dungeon delve journal (§24, v36)** — 234 tasks exist with correct titles (spot-checked
  2000306 / 2000338 / 2000346 / 2000538). Enter a **Deepest Guk** rung and confirm the journal names
  Deepest Guk, then enter it on **Hard** and confirm the title carries `(Hard)` — the mode offsets
  widened 10 → 40 and a mismatch there resolves to the wrong mode silently.
- ⚠️⚠️ **The LDoN half of the ladder (§24, v27 + v37)** — it was blocked by **two** independent gates
  and both are now open: region 99 "Unused" (v27) and the stock expansion check (v37). **Test as a
  NON-GM character** — the GM flag skips the expansion check and has hidden this bug twice. Enter any
  even rung and confirm no *"part of an expansion that you do not yet own"*. ⚠️ Also confirm `veksar`
  is still **refused** by region locking — it was excluded on purpose.
- **The Zone XP tab (§3, v33)** — 63 rows seeded. Open `/allaclone` → **Zone XP** with an empty box
  and confirm it browses rather than saying "type something first", that region headers and city
  labels render, and that clicking a row does **nothing** (every row is id 0).
- **The recipe era unlock (§32, v31)** — 388 recipes became findable. Search Pottery in the recipe
  window and confirm PoP armour appears.

## 31. Individual loot — everybody gets their own roll — 2026-08-01

The loot table is rolled **once per eligible player** and each roll belongs to that player alone, so a
group never contends for a drop. It replaces the need/greed/sell system rather than sitting beside it:
`AdvLootManager` still compiles but is **not consulted** while the rule is on. Rule
**`AoT:IndividualLoot`** (default **true**); set false to restore one shared roll plus the old rolls.
**Needs a zone rebuild** (`ruletypes.h` and `common/loot.h` are in `common/`, so it is a wide one).

- **Files**: `common/loot.h` (`LootItem::owner_char_id`), `common/ruletypes.h`, `zone/attack.cpp`
  (`NPC::Death`, the per-player loop), `zone/spawn2.cpp` (spawn roll suppressed), `zone/loot.cpp`
  (`AddLootDrop` stamps the owner), `zone/npc.h` (`SetLootOwner`/`GetLootOwner`), `zone/corpse.h` +
  `zone/corpse.cpp` (`AoTv4AbsorbOwnedLoot`, per-player `AssignLootSlots`, the two native-path
  guards), `zone/client_packet.cpp` (`AdvLootOwnedByOther` + every AdvLoot path).
- **Ownership rides on the ITEM** (`owner_char_id`, 0 = shared), not on the lootslot: slots are
  renumbered constantly, so a slot-keyed side table would desync the moment anything renumbered.
  ⚠️ **Runtime only** — never written to `character_corpse_items`. NPC corpses are not persisted and a
  player corpse has no use for it, which is also why the guards are no-ops on your own corpse.
- ⚠️⚠️ **THE ROLL MOVED FROM SPAWN TO DEATH, and it had to.** Eligibility is not known until
  `NPC::Death` has run the `AllowPlayerLoot` block, so the loop sits **immediately after** it. The
  spawn-time `AddLootTable()` in `spawn2.cpp` is suppressed for the same reason — leaving it would put
  an extra **unowned** copy of the table on every corpse, i.e. a free shared roll on top of
  everybody's personal one.
  ⚠️ **Global loot still rolls at spawn and stays SHARED.** Deliberate: a 1.5 percent Ink of the Lost
  (§29) multiplied by group size is a different drop rate.
- **The NPC's loot list is a STAGING AREA.** Each pass sets `SetLootOwner(cid)`, rolls, then
  `Corpse::AoTv4AbsorbOwnedLoot` moves the items onto the corpse and empties the NPC again, and the
  owner is restored to **0 immediately**. ⚠️ Anything rolling outside that window — quest
  `npc:AddItem`, forage, the global tables — must stay shared, which is exactly what the restore buys.
  ⚠️ The absorb must leave the list **empty** or the next player's absorb collects the previous
  player's items too.
  📌 Safe because the `Corpse` constructor already did `m_item_list = *item_list; item_list->clear()`,
  so the NPC's list is empty when the loop starts and pre-existing quest drops are on the corpse as
  shared.
- **Coin is paid DIRECTLY at death**, per player, and is the one thing that does not wait for a
  window. `AddLootTable` re-rolls `m_loot_*` on every non-global call, so each pass leaves that
  player's own amount sitting there — reusing it keeps the stock distribution, `avgcoin` weighting
  included. ⚠️ It is zeroed after each pass so the corpse cannot also carry it. ⚠️ This is *why* it is
  paid directly: `Corpse::MakeLootRequestPackets` was the only thing that ever called `AddMoneyToPP`,
  and AdvLoot mode skips it, so routing coin through the corpse made money vanish entirely.
  - ⚠️⚠️ **THE "You receive … from the corpse of …" MESSAGE IS LOAD-BEARING — do not remove it as
    chat noise.** No coin ever lands on the corpse (the `Corpse` constructor calls `SetCash` from the
    NPC *before* this roll, so it captures 0) and no loot window is involved, which leaves a silently
    changed money total as the only evidence anything happened. This was reported as *"coins do not
    come off mobs when they die"* when the coin was in fact being paid correctly. The message IS the
    loot event now. A `LogLoot` line beside it reports the rolled amount per player — enable with
    `#logs` (Loot category) to tell "paid nothing" apart from "rolled nothing".
- ⚠️⚠️ **A CORPSE NOBODY HAS KILL CREDIT ON GETS ONE SHARED ROLL, or it is EMPTY FOREVER.** An NPC
  killed by another NPC — guards, faction fights, a charmed or unowned pet — never reaches
  `AllowPlayerLoot`, so `m_allowed_looters` stays empty; and `Corpse::CanPlayerLoot` returns
  `looters == 0`, meaning such a corpse is lootable by **anyone**. Stock rolled at spawn so it always
  had loot. Rolling only per eligible player therefore handed back an openable, permanently empty
  corpse. The fallback rolls once with owner 0 (shared, which is right for a corpse with no owner) and
  puts the coin **on the corpse**, collected the stock way, since there is nobody to pay directly.

### ⚠️⚠️ LOOTSLOTS ARE NUMBERED PER PLAYER — this is what makes the design fit the client
A corpse has only **34 addressable slots** (`CORPSE_BEGIN..CORPSE_END`, = `slotGeneral1 .. +slotCursor`).
Individual loot puts one roll per eligible player on a single corpse, so a global numbering **runs
out**: tables average 2.1 drops and 324 of them yield >5, so a full group of six on a rich table
overflows and everything past the 34th silently becomes `0xFFFF` — invisible and unlootable, with no
error. The world boss (§17c, up to **72** looters) would overflow almost immediately.
`Corpse::AssignLootSlots(Client*)` therefore numbers from ONE player's point of view: somebody else's
drop gets **no handle at all**, so their items do not consume slots out of your range and the ceiling
applies **per player** instead of per corpse.
- ⚠️ Cached per character (`m_loot_slots_char_id`), not with a plain bool — re-numbering is skipped
  when the same player asks again (so a snapshot of slots stays valid while they loot down it) and
  redone when a different player asks. `Corpse::RemoveItem` deliberately does not renumber, so
  removals cannot invalidate a snapshot either.
- ⚠️⚠️ **EVERY path must number for the acting player BEFORE reading a lootslot.** `alslootall` did
  not, which was harmless under global numbering and a real bug under this one: it snapshotted
  whatever numbering the corpse last had, and `AdvLootSlot` then renumbered for the acting player, so
  the handles pointed at different items by the time they were used.
- ⚠️ `MakeLootRequestPackets` **is** a per-player numbering (it re-stamps from `CORPSE_BEGIN` filtered
  by the client's `CorpseBitmask`), so it stamps the cache with that character id. Without that,
  AdvLoot would consider the corpse unnumbered, renumber it behind the client, and every handle just
  sent to them would resolve to a different item.

### ⚠️⚠️ THE STOCK LOOT WINDOW IS A PATH TOO — it was the hole
Advanced Loot runs in **complement mode** (§16): the native RoF2 loot window still opens and
`MakeLootRequestPackets` is untouched by it. It walked `m_item_list` with **no ownership filter**, so
right-clicking a corpse listed **every player's personal drop** and `Corpse::LootCorpseItem` handed
them over — the whole system bypassable from the one path that was not part of it. Filtering only our
own window proves nothing; **test both**.
- **`Corpse::LootCorpseItem` is the central guard**, because both windows funnel through it — AdvLoot
  synthesizes an `OP_LootItem` and calls straight into it, and the stock window sends one directly.
  ⚠️ It also catches a client sending the **`0xFFFF` sentinel**, which `Corpse::GetItem` would
  otherwise resolve to the first *unnumbered* item — i.e. somebody else's.
- ⚠️ Filtering a display is presentation; the guards in `LootCorpseItem`, `AdvLootSlot` and
  `AdvLootSell` are the rule. A modified dll can name any slot it was never shown.

### ⚠️ Four AdvLoot paths that each needed the same guard
Ownership was originally enforced in `AdvLootSlot` only. All four below act on corpse items and all
were reachable; the shared helper is **`AdvLootOwnedByOther`** (`client_packet.cpp`).
| Path | What it did |
|---|---|
| `AdvLootSell` | `/say alspick <slot> sell` sold **another player's** drop — item gone, coin paid, unrecoverable |
| `alssellall` | "Sell All" swept every group mate's individual loot off the shared corpse |
| standing rules in `SendAdvLootData` | an Always Sell rule fired on group mates' drops **at death**, before either player had seen a window |
| `alslootall` | refused per item downstream, but one refusal message per group mate's drop |
- ⚠️⚠️ **Sale proceeds DO NOT SPLIT AT ALL — the seller keeps the full value.** Stock divides a Sell
  between everyone entitled to roll on the corpse, which is right for something jointly earned and
  exactly wrong here: nothing is jointly owned, so a split would tax every personal drop by group size
  and pay people who cannot even see the item. ⚠️ Shared items (owner 0) are **not** an exception —
  they are rare, and a Sell that sometimes splits and sometimes does not is worse than one that never
  does, because the player cannot tell the two cases apart from the item alone.
- ⚠️⚠️ **Every player counts as SOLO for standing rules** while the rule is on. Two reasons, and the
  second is the one that bites: nothing is contested, so seeding a 30-second need/greed roll would
  stall a player's own drop behind a vote nobody can cast; **and `AdvLootManager` keys its share map
  on `(corpse_id, lootslot)`**, which now aliases between players, so any roll it did start would
  resolve against the wrong item.

## 32. Tradeskills are worth doing — sockets, skill-scaled tiers, gear, AA ladders — 2026-08-04
> 📌 **Substantially revised 2026-08-06/07** (migrations v28-v32): the flat +20/+30 gear bonus became
> native `skillmod` percentages on **three** worn pieces, the Mythic ceiling moved 350 → **345**, the
> reward rows' `chance` was fixed from 1 percent to 100, the mask illusions' duration was fixed, and
> 388 era-gated recipes were made findable. Sections below that say "+20 / +30" or "the tool and the
> mask" are **historical** — read the gear subsection first.

A single pass to make crafting matter. Five parts, all interlocking; the through-line is that
**tradeskill SKILL now decides what you get**, where before it decided nothing.

- **Files**: `zone/achievement_manager.cpp` (`grant_aa`), `zone/tradeskills.cpp`
  (`AoTv4TradeskillSkill`, `AoTv4RollCraftTier`), migrations **v12/v13/v14**, and the readable
  copies `custom/sql/aotv4_tradeskill_aa_ladders.sql`, `aotv4_craft_sockets.sql`,
  `aotv4_tradeskill_tools.sql`.

### Crafted wearables are socketed by TIER — native 1 / Hallowed 2 / Mythic 3
8,889 wearables produced by an enabled recipe, plus their two tier clones.
- ⚠️⚠️ **"Crafted gear has sockets" is NOT expressible as written.** A crafted Mythic and a *dropped*
  Mythic are **the same item row** — tiers are separate ids, not instance state — so the only
  coherent statement is "this item has sockets". Dropped copies of craftable items are socketed too,
  and that is correct rather than a leak.
- ⚠️⚠️ **All three sockets are type 1, and that is what makes every delve augment fit every slot.**
  The engine tests `(1 << (augslotNtype - 1)) & aug->AugType` (`zone/inventory.cpp:356`) and the
  delve augs carry `AugType 255` = slot types 1-8. Do **not** tier-lock the sockets by giving them
  different types: augments must fit in all three.
- ⚠️⚠️ **ORNAMENTATION SLOTS ARE THE SAME SIX COLUMNS.** There is no separate ornament field — an
  ornament slot IS an augment slot of type **20** (Ornamentation) or **21** (Special Ornamentation).
  On these items they sit almost entirely in **position 2** (3,396) and 3 (658), exactly where the
  Hallowed/Mythic stat sockets go, so a blind overwrite silently deletes the ability to ornament
  crafted gear. Positions **4-6 are completely unused**, so they are relocated there and nothing is
  lost. 358 items carry **two** ornaments; both are kept (positions 4 and 5).
- ⚠️ Only **4** of the 8,889 have more than 3 sockets (they have 5, of exotic types in the upper
  positions) — left alone rather than guessed at.
- ⚠️ Must run **after** `aotv4_gear_tiers.sql`: that clones with `INSERT ... SELECT *`, so tier rows
  otherwise just inherit the base's single socket.

### Crafting no longer always yields Mythic — the tier is ROLLED from skill
`AoTv4RollCraftTier`, at the `GetTradeRecipe` choke point that already served both overloads.
| effective skill | Mythic | Hallowed |
|---|---|---|
| 100 | 0% | 33% |
| 200 | 27% | 100% |
| 300 | 77% | 100% |
| 345 | 100% | — |
- ⚠️⚠️ **THE MYTHIC CEILING IS 345, NOT 350, AND IT IS DERIVED FROM THE GEAR — do not round it back
  up.** The cap is the best effective skill anyone can reach: **300 raw × 115/100 = 345** with the
  hand piece. It was 350 while the gear paid a flat +20/+30 (300 + 50). Set the ceiling above what
  the gear can actually deliver and guaranteed Mythic becomes **unreachable**, with nothing on screen
  to indicate why. **Retune it the moment the top piece's percentage changes.**
- ⚠️ **Mythic is rolled FIRST and returns immediately** — these are bands of one decision. Testing
  Hallowed first swallows the Mythic band entirely (the same trap already recorded for loot tiers).
- ⚠️ **Only a recipe whose output is a BASE id is rolled.** The old code normalised through
  `AoTv4TierBaseId` first, which was safe when the result could only go up; with a roll it could
  **downgrade** an author's deliberate tier output. No stock recipe does this (0 of 19,464), so it is
  a guard, not a live path — but "never downgrade the recipe's own output" is the property to keep.
- 📌 This is a deliberate **nerf** to the previous unconditional Mythic. It is also what makes the
  §26 quest-turn-in normalisation less load-bearing, since base items are craftable again.

### The gear bonus is NATIVE `skillmod` — three worn pieces, 5 / 10 / 15 percent — 2026-08-06 (v29)
> ⚠️⚠️ **THIS REPLACED A FLAT +20 / +30 PAID IN `AoTv4TradeskillSkill`, AND THE OLD SHAPE IS RECORDED
> BELOW BECAUSE THE REASONING THAT PRODUCED IT IS STILL SOUND — it was simply beaten by a fact nobody
> had checked.** Applying migration **v29** against a zone binary that still has the flat adders
> **DOUBLE-COUNTS every piece**; they are removed in the same commit.

| piece | ids | slot | bonus | earned at |
|---|---|---|---|---|
| head tool | 147930-147941 | HEAD (4) | **+5 percent** | tradeskill skill 100 |
| face mask | 147942-147953 | FACE (8) | **+10 percent** | skill 200 |
| hand piece | 147954-147965 | HANDS (4096) | **+15 percent** | skill 300 |

- ⚠️⚠️ **THE FLAT BONUS WAS INVISIBLE, AND THAT IS THE WHOLE REASON IT CHANGED.** Paying it inside
  `AoTv4TradeskillSkill` meant it never reached `Client::GetSkill`, while the client's Skills window
  renders the **raw** `m_pp.skills` — so wearing the tool changed every combine roll and every tier
  roll and **displayed nothing**. Reported from play as *"this is not working or it would reflect on
  my skill sheet"*. It worked perfectly and was visible nowhere.
- ⚠️⚠️ **`skillmod` DOES NOT STACK — `bonuses.cpp:451` keeps only the HIGHEST value per skill.** The
  three pieces are a **progression, not an accumulation**: wearing all three is 15 percent, exactly
  the same as wearing the hand piece alone. **Do not "fix" this by raising the numbers so they sum to
  something** — the ladder is the reward, and 345 (§ the tier table above) comes from the *best*
  piece, never from 5 + 10 + 15.
- ⚠️⚠️ **THE MASK HAD TO BECOME WEARABLE.** It was `slots = 0`, an inventory clicky, and
  `AddItemBonuses` only ever walks **EQUIPPED** slots — a `skillmod` on an unworn item contributes
  exactly nothing. It moved to **FACE**, deliberately not head, because the head slot is the tool's
  and two pieces competing for one slot is the thing that made the original clicky design necessary.
- ⚠️ `skillmodmax` is forced to **0**. `GetSkill` treats a non-zero max as a **FLAT cap**
  (`min(raw + max, raw * (100 + mod) / 100)`), which would silently truncate the percentage.
- ⚠️ **`AoTv4TradeskillSkill` still exists and is still the one definition** — it is now just
  `GetSkill()`. Kept as a named function because the success roll and the tier roll must agree, and
  because a future **non-item** source would go there rather than into `GetSkill`.
- ⚠️ **It still cannot shortcut the achievement ladders.** `Client::SetSkill` feeds `ProcessSkill`
  the **stored** value, so gear cannot buy a rung. Earned, not worn.
- ⚠️ `Skills:TradeSkillClamp` is **0** here, which disables the clamp at `tradeskills.cpp:1197`. Set
  it non-zero and the bonus is silently truncated away.
- ⚠️⚠️ **THE STRAY AUGMENT SOCKETS WERE CLEARED (v29).** Every row in the band carried `augslot1type`
  7 (General: Group) and `augslot2type` 21 (Special Ornamentation), inherited from whatever stock item
  the original SQL cloned — the same class of bug as §5's "cloning a stock spell inherits its damage
  formula". The tell was that the **masks** had them too, and a `slots = 0` inventory clicky can never
  be ornamented. Cleared **before** the hand pieces are cloned, so they are not inherited again.
- 📌 `skillmodtype` is derived with `ELT` over `((id - 147930) MOD 12)`, reproducing the exact index
  order of `AOTV4_TS_SKILLS`. Keep the two in step — reordering silently gives blacksmiths the fishing
  bonus with no error anywhere.
- ⚠️ The v29 rewards target achievement ids **arithmetically** (`400000 + skill*1000 + 300`, i.e.
  455300..469300), never a `SELECT` on (skill, required_count) — that also matches the Master Artisan
  aggregate (470000-470999) and would land all twelve rewards on one achievement (§ below).

#### ⚠️⚠️ THE CLIENT APPLIES `skillmod` ITSELF — NEVER SEND A PRE-MODIFIED SKILL VALUE
The RoF2 client takes the raw skill off the wire and applies the equipped item's `SkillModValue` to
the **displayed** number on its own. Sending an already-modified value makes it apply the percentage
a **second time**: measured, raw 300 with a 5 percent item sent as `GetSkill()` = 315 **rendered as
330** (315 × 1.05).
- Both packets must carry the **raw** `m_pp.skills` value: `OP_SkillUpdate` in `Client::SetSkill`
  (`zone/client.cpp`) and the player profile in `Handle_Connect_OP_ZoneEntry`
  (`zone/client_packet.cpp`). Both were briefly changed to `GetSkill()` on 2026-08-06 to make the
  bonus visible, and both are reverted with a comment saying why.
- 📌 **The bonus was invisible because it was paid in C++ and was not on the item at all.** Once it
  became a real item `skillmod`, the client displayed it with **no server change needed** — the
  packet work was solving a problem that the data fix dissolved.

#### ⚠️ Why a flat bonus could not have been expressed natively (the original finding, still true)
`Client::GetSkill` (`client.cpp:13090`) applies an item's `skillmod` as a **PERCENTAGE**
(`skill * (100 + mod) / 100`) and reads **item bonuses only** — `spellbonuses.skillmod` is never
consulted, and `bonuses.cpp:451` only ever populates `skillmod` from items. There is **no flat skill
increase SPA**: the near misses are `RaiseSkillCap` (247, raises the CAP), `ReduceSkill` (122, a
percentage reduction) and `TradeSkillMastery` (263). **So a spell or illusion cannot raise a
tradeskill at all through any stock path** — which is why the bonus was paid in `tradeskills.cpp`
for as long as it wanted to be flat, and why going native meant giving up "flat" rather than
finding a better place to put it.
- ⚠️ Still deliberately **not** folded into `Client::GetSkill` by hand: that is read for every combat
  skill in the game, and an adder there would leak into melee, defense and casting. The native
  `skillmod` path is per-skill by construction, which is what makes it safe.
- ⚠️⚠️ **THE ID ORDER IS LOAD BEARING.** Both bands are indexed off `AOTV4_TS_SKILLS`
  (55,56,57,58,59,60,61,63,64,65,68,69), so 147930 and 44400 must both be Fishing. Reordering the
  SQL silently gives blacksmiths the fishing bonus and nothing reports an error.
- ⚠️⚠️ **THE MASK'S `clicktype` MUST BE 1 (`ItemEffectClick`), NOT 4.** Type 4 is
  `ItemEffectEquipClick`, and `zone/spells.cpp:7529` refuses it unless the item is in an **equipment**
  slot — the masks shipped `slots = 0` and could never be equipped, so a type-4 mask was **silently
  unusable by everyone**. It shipped as 4 for a day. ⚠️ The mask is FACE-slot now (v29), so type 4
  would technically resolve — **it stays 1 anyway**, because 1 works from bag or slot and nothing is
  gained by tying the click to being worn.
- ⚠️⚠️ **THE MASK MUST NOT BE HEAD SLOT.** That is the tool's slot, and two pieces competing for one
  slot is what made the original inventory-clicky design necessary in the first place. FACE was
  chosen for that reason alone.
- ⚠️⚠️ **AN UNLIMITED CLICKY IS `maxcharges = -1`, NOT 0** (v30). The masks shipped **0**, which the
  engine reads as a **spent consumable** — *"Item is out of charges."* — so the illusion never fired
  once. 538 stock items use -1 (Journeyman's Boots, Traveler's Boots, Bracer of Hammerfal). **0 is
  the one value that makes a clicky permanently dead while the row still looks correctly
  configured**: `clickeffect` set, `clicktype` 1, nothing reads as broken.
  📌 That is the **third** polarity/sentinel trap on these same items (`nodrop = 0` = No Drop,
  `clicktype 4` = equip-only, `maxcharges 0` = dead). **Check the sense of every flag on an item row
  rather than assuming 0/1 means off/on.**
- ⚠️ **All 36 shared `icon 639`** and rendered as cloth hats regardless of slot (v30) — reported as
  *"the icons aren't matching the items, all of the items look like cloth hats"*. Now head **625**,
  face **770**, hands **531**, the most common stock icon per slot. 📌 `idfile` stays IT63, which
  stock head items also use (Chromatic Helm), so only the inventory icon was ever wrong.
- ⚠️ All 36 items are `classes=65535 races=65535 reqlevel=0 reclevel=0 deity=0` — every class, every
  race, no level gate. The gate is the achievement, nothing on the item.
- ⚠️⚠️ **THEY SURVIVE THE ROGUELITE DEATH — `death_loss.M.is_kept` spares 147930-147965.** Without it
  the wipe destroys them like any other carried gear, **and the achievements grant them with
  `claim_once = 1`, so they would never be re-granted**: one death would cost the tool, the mask and
  the hand piece permanently, with no way back short of a GM. The justification is the same one that
  protects evolving items — tradeskill skill is explicitly the one thing death does **not** reset
  (`max_skills_for_level` skips tradeskills), so destroying the reward for it is incoherent.
  ⚠️ It is an id **band**, not a property test, and it is mirrored from `AOTV4_TS_TOOL_BASE` /
  `AOTV4_TS_MASK_BASE` / `AOTV4_TS_HANDS_BASE` in `zone/tradeskills.cpp` — widening the reserved
  range there without widening it in `death_loss.lua` silently makes the new items destructible. It
  was widened 147930-147953 → **147930-147965** when the hand pieces landed.
- ⚠️⚠️ **`nodrop = 0` MEANS "NO DROP" — THE FLAG IS INVERTED** (`client_packet.cpp:10755`: *"No Drop
  items have no vendor value"* tests `NoDrop == 0`). These shipped as `nodrop = 1` for a day, which
  made an **earned** tool tradeable to somebody who had not earned it. `norent = 1` is the opposite
  polarity again and is correct: 1 = permanent, 0 = deleted on logout.
- ⚠️⚠️ **THE ILLUSIONS ARE AT 44400, NOT 436xx.** The obvious-looking 43600 sits **inside the
  spell-rank band 43576-44327** and would have overwritten twelve rank rows. Check the §20 band map
  before reserving anything in 43xxx/44xxx; 44400 is clear and still under RoF2's 45000 ceiling.
  📌 Since v29 they are **purely cosmetic** — the skill bonus moved onto the item when the mask became
  wearable. The spells still exist and the masks still click them.

#### ⚠️⚠️ A LEVEL-SCALED BUFF FORMULA IS WRONG FOR AN ITEM CLICKY WITH A FIXED CAST LEVEL (v32)
The mask illusions lasted **three minutes, not the intended thirty**, and the spell row read as
correct: `buffdurationformula = 3` (`30 × level`) with `buffduration = 360` as the cap, i.e. "36
minutes, capped". **The cause is the ITEM, not the spell** — the masks click at `clicklevel2 = 1`, so
the formula yields `30 × 1 = 30 ticks` = exactly the 3 minutes observed, and the 360 cap never came
near binding.
- Fixed with a **level-independent** duration rather than by raising `clicklevel2`. The default branch
  of the formula switch (`zone/spells.cpp`) is `if (formula < 200) return 0; temp = formula;` — so a
  formula of **300 is literally 300 ticks**, whatever level it is cast at. 300 × 6 s = 30 minutes.
- ⚠️ `buffduration` stays **300**: the tail of that function caps with
  `if (duration && duration < temp) temp = duration`, so setting it lower silently wins over the
  formula.
- 📌 `clicklevel2 = 10` would also give 300 via formula 3, but it ties the duration to a field that
  exists for a different purpose and breaks again if the item is ever re-cloned.
- ⚠️ These sit inside the **43000-44999 band that `Mob::CalcBuffDuration` excludes** from the §36
  three-day self-buff floor, so the native duration is authoritative. Without that exclusion the whole
  change would be moot — they would already have been lasting days.

#### ⚠️⚠️ TRADESKILL RECIPES ARE ERA-GATED SEPARATELY FROM ZONES (v31)
`tradeskill_recipe` carries its **own** `min_expansion`/`max_expansion`, and the recipe **search**
(`Handle_OP_RecipesSearch`) ends with `ContentFilterCriteria::apply()`. At
`Expansion:CurrentExpansion = 0` that hid **388 enabled recipes** — 385 Pottery, 3 Blacksmithing,
almost all PoP armour. Clearing the gate on the **recipe rows** unlocks them while `CurrentExpansion`
stays 0, so zones, doors, spawns and item filtering are untouched. Raising `CurrentExpansion` instead
would open Kunark..PoP wholesale and defeat region locking — exactly what §35 records a dump doing
silently.
- ⚠️⚠️ **THEY WERE ALWAYS CRAFTABLE, ONLY UNFINDABLE.** The combine path
  (`ZoneDatabase::GetTradeRecipe`, `tradeskills.cpp:1633`) filters on `tr.enabled` **only** and has no
  content filter, so anyone who knew the component set could already make them. The gate was purely on
  **discovery** — the worst of both worlds: no protection, just a hidden recipe.
- ⚠️ **Scoped to `enabled = 1`.** The other 3,296 gated rows are min_expansion 9 (DoN) **and** already
  `enabled = 0` — deliberately off, consistent with the era system capping at OoW (§12). Enabling
  those is a separate content decision. `content_flags` is left alone (one recipe is `peq_halloween`
  and must stay seasonal).
- 📌 Some need components from zones the server has not unlocked, so they show in search and still
  cannot be made. That is "needs a rare component", not a regression.

### Achievements can grant AA — the `grant_aa` reward type
The one architectural addition, mirroring the custom `scribe_spell`. `reward_id` = `aa_ability.id`,
`amount` = the rank to top up **to**.
- ⚠️⚠️ **ABSOLUTE RANK, NOT A DELTA — that is what makes it idempotent.** Achievements re-fire
  (`RecheckAutomatic` re-credits already-met conditions), and a "+1 rank" reward would climb every
  time. "Be at rank 2" cannot. It also makes the ladders declarative.
- ⚠️ `ignore_cost = true` → `CanPurchase(check_grant=false)`, which is what lets a **`grant_only`**
  AA through; every AA on these ladders is grant_only so it can never also be bought.
- ⚠️⚠️ **`aa_ability.enabled = 0` MEANS THE AA DOES NOT EXIST AT RUNTIME** — `zone/aa.cpp:1823` loads
  only enabled rows, so `grant_aa` fails with "not loaded". Most tradeskill masteries ship disabled
  and v12 switches them on.
- ⚠️⚠️ **`custom_achievement_rewards`.`chance` IS IN BASIS POINTS (0-10000), NOT PERCENT — and BOTH
  AoTv4 reward wirings shipped it as `100`, i.e. ONE PERCENT** (found 2026-08-06, migration **v28**).
  `QueueAchievementRewards` gates on `(r.chance >= 10000 OR FLOOR(RAND() * 10000) < r.chance)`
  (`achievement_manager.cpp:1499`), so the reward was simply not queued on 99 completions out of 100.
  Every stock reward type ships **10000**; the 36 `grant_aa` rows (the mastery ladders) and the 24
  `item` rows (the tools 147930+ and masks 147942+) shipped 100 and were near-dead from the day they
  landed. Reported from play as *"some of our tradeskill achievements are not giving the AAs"*.
  - ⚠️⚠️ **IT IS INVISIBLE FROM EVERY DIRECTION THAT MATTERS.** The achievement completes, the window
    shows it **Done**, and the detail pane still reads *"Auto — Fletching Mastery rank 1"* — because
    that text comes from the **definition**, which is perfectly valid. Nothing is logged and
    **`#ach rewards` shows nothing to explain**, since there is no refusal to report.
  - 📌 **Diagnose by the ABSENCE of a `custom_character_achievement_rewards` row, not by reading
    `result_text`.** A queued-and-refused reward and a never-queued one are identical in game and
    completely different in the database — the first has a row with a reason, the second has no row.
    Chasing the AA config (expansion gate, `level_req`, prereqs, `grant_only`) all checked out fine
    and cost the whole investigation; the reward had never reached that code.
  - ⚠️ v28 also **backfills** anyone already affected, as `status 0` pending rows.
    `ClaimPendingRewards(client, 0, true)` sweeps every pending auto-claim row on the **next**
    completion (not just that achievement's), and `#ach claim` forces it immediately.
  - ⚠️ Scope any such fix to `reward_type IN ('grant_aa','item')`, never to "any `chance = 100`" — a
    genuine 1 percent reward chance is a legitimate value in this column.

### ⚠️ THE TOOLS AND MASKS ARE ACHIEVEMENT REWARDS, NOT CRAFTED — and the recipes were REMOVED
They were briefly craftable (recipes 470100-470111, trivial 200). That was dropped on 2026-08-04 for
one reason: **a recipe nobody is told about is not a reward.** A player has no way to know a new
recipe exists, and neither `skillneeded` nor the in-game recipe search fixes "I did not know to
look". The achievement window does — it **lists a reward before you earn it**, so a crafter at skill
40 can already see what waits at 100, 200 and 300.
- **Tool (+5 percent)** → the existing skill **100** achievement. **Mask (+10)** → skill **200**.
  **Hand piece (+15)** → skill **300** (added with v29). All as `item` rewards (`SummonItem`), on the
  per-skill rows that already existed.
- ⚠️ **All twelve tradeskills get all three items**, Fishing and Research included. They are excluded from
  the AA ladders only because neither has a Mastery AA to grant; the skill bonus is paid by
  `AoTv4TradeskillSkill` for all twelve, so there is no reason to exclude them here.
- ⚠️⚠️ **SELECTING AN ACHIEVEMENT BY (skill, required_count) MATCHES THE AGGREGATE TOO.** The Master
  Artisan rows (470050-470300) carry objectives for the same skills at the same values, so a query
  keyed on skill+threshold returns **470100** alongside **463100** — and `MAX()` picks the aggregate.
  Anything wiring per-skill rewards must exclude `470000-470999`, or the rewards land on Master
  Artisan and every tradeskill hands out the same item. Caught during wiring; it is silent.
- 📌 Rejected on the way here: a **recipe scroll** (`must_learn` + `learned_by_item_id`, both fully
  implemented — `Client::ScribeRecipes`) was the obvious fix but *hides* the recipe until earned, so
  you cannot see what you are working toward; and a **hub vendor** cannot gate stock per character
  because `merchantlist` has no per-character column.

### The two ladders
- **Per skill**: 50/100/150 → mastery rank 1/2/3, for the **ten** tradeskills that have a mastery AA.
- **Aggregate** ("Master Artisan", 470050-470300): **all ten** at 50/100/…/300 → Salvage rank 1-6.
  Salvage has exactly six ranks, which is why the aggregate runs to 300 while the per-skill ladders
  stop at 150.
- ⚠️⚠️ **THE PER-SKILL ACHIEVEMENTS ALREADY EXISTED — DO NOT CREATE THEM.** All twelve tradeskills
  already ship six achievements each in category **600**, keyed **`4<skill_id><level>`** (455050 =
  "Fishing 50", 469300 = "Pottery 300"). That is the "Tradeskill 72" figure in §15. v12 only attaches
  a `grant_aa` reward alongside the existing title reward.
- ⚠️⚠️ **SKILL 68 IS JEWELCRAFTING AND 69 IS POTTERY**, not the reverse — verified against the
  achievement rows. Getting it backwards pairs each with the other's mastery, silently.
- ⚠️ **Fishing (55) and Research (58) are excluded from BOTH ladders** — neither has a mastery AA in
  the game, and including them in the aggregate would gate all of Salvage behind two skills with no
  reward of their own. Their own six achievements still award their titles.
- ⚠️ An achievement completes only when **every** non-optional objective does
  (`TryCompleteAchievement`), which is what makes the 10-objective aggregate an AND.

### Deploying
⚠️ `items` and `spells_new` are **shared memory**: a migration applying at world boot is **not
enough** — stop the stack, run `./shared_memory`, restart. The AA and achievement halves need only a
zone restart.
📌 All three items are obtainable: tool at skill 100, mask at 200, hand piece at 300, from the
per-skill achievements. Nothing in this section is placed by drop, vendor or quest — the achievement
ladder is the only acquisition path, deliberately, because it is the only one that advertises itself.
⚠️⚠️ **AND THE REWARD ROWS THEMSELVES MUST CARRY `chance = 10000`** — see the basis-points trap in
the `grant_aa` subsection above. Both AoTv4 wirings shipped `100` (one percent) and were near-dead
from the day they landed; v28 fixes and backfills them.

## 33. The starter weapon follows the CLASS — 2026-08-04

`global_player.event_death_complete` grants a weapon whenever the Primary slot ends up empty (the
roguelite wipe strips equipped weapons, so a fresh run would otherwise be fists-only). It used to
hand out **9998 Short Sword to everybody**, which is wrong for half the roster.

- **Reported from play**: a Ranger reforged to Monk, then to Berserker, and kept the short sword
  throughout — *"those classes have 0 skills in 1hs"*. Confirmed against `skill_caps`, not assumed:
  **Monk (7) has no 1H Slashing**, and **Berserker (16) has neither 1H Slashing nor 1H Blunt** — only
  1H Pierce, 2H Slash and Hand to Hand. The "starter" weapon was one they could never train.
- The map is `aotv4_reforge.M.STARTER_WEAPON` / `M.starter_weapon(class)`. It lives in the **reforge**
  module because that module owns class identity and needs the same answer on a class change; two
  copies would drift.
- ⚠️⚠️ **EVERY ID MUST BE ONE ABSOR ACCEPTS.** He takes exactly four (`tutorialb/Absor.pl`): **9997**
  Dagger (1HP), **9998** Short Sword (1HS), **9999** Club (1HB), **55623** Dull Axe (**2HS**), handing
  each back sharpened. A weapon outside that set leaves the class better armed and the **tutorial
  uncompletable** — and the tutorial is the roguelite's way out, so that is not a small break.
- ⚠️ None of the four has gear-tier rows, so they stay **BASE** and the hand-in matches. Never
  substitute a weapon the crafting or loot paths would upgrade to Hallowed/Mythic.
- ⚠️⚠️ **ROGUE GETS THE DAGGER, AND THAT IS MECHANICAL.** `Client::OPCombatAbility` refuses Backstab
  on anything that is not `ItemType1HPiercing` (`zone/special_attacks.cpp:793`), so a Rogue holding
  the short sword could not use their signature ability at all.
- 📌 Rule for the rest: prefer 1H Slashing, else 1H Blunt, else 2H Slashing — the best of the four the
  class actually has a skill cap for. Berserker is the only 55623.

### A reforge swaps the weapon too — `M.fix_starter_weapon`, called from `M.finish`
Changing class only re-armed you if you happened to die afterwards with an empty Primary, which is
why the short sword survived two class changes.
- ⚠️⚠️ **IT ONLY TOUCHES A STARTER WEAPON.** `M.IS_STARTER` is derived from the map, and anything else
  in Primary is left completely alone — a reforge must never destroy gear somebody earned, and a
  level-1 character may be carrying a quest reward.
- ⚠️ Runs **before** `M.finish`'s `Save`/`Kick`, so the swap is part of the same atomic change the
  player sees. `M.finish` is also where the stat rebase and the old class's combat-skill clear live —
  all three are "things that do not follow a class change on their own".
- ⚠️ Primary is slot **13**, and `GetItemIDAt` returns **INVALID_ID (-1)** for an empty slot, not 0.

## 34. ⚠️⚠️ "I'm stuck at 0 hp" — TWO separate bugs, neither one what it looked like — 2026-08-05

Three reports over one evening turned out to be two distinct defects, and **both presented as a
client-side HP problem while the server believed everything was fine**:

> *"took off my charm and it knocked me down like I was bleeding out"*
> *"it didn't refill at all"*
> *"I'm sitting here at 0 hp / I can't zone"*
> *"knocked unconscious … bleeding to death"*

⚠️ **The charm was a red herring in both.** Toggling an item forces a bonus recalc, which is simply
the thing that made an already-broken HP state visible. Chasing the charm wastes the session.

### (a) A REFORGE saves the OLD class's hit points — fixed in `global_player.event_connect`
`SetBaseClass` writes **`m_pp.class_` and nothing else**, so `GetClass()` still returns the **old**
class while `aotv4_reforge.M.finish` runs. `AoTv4ApplyCreationStats` → `CalcBonuses` → `CalcMaxHP`
therefore computes and clamps against the **old** class's maximum, and that value is saved. A Bard at
**36** who becomes a Magician is saved at 36 against a new maximum of **30**, relogs with `cur > max`,
and the client tracks HP down from an impossible number — reporting *"knocked unconscious"* and
*"bleeding to death"* on its own while the server sees nothing wrong.
- ⚠️⚠️ **The clamp MUST live in `event_connect`, not in `M.finish`.** Only after the forced relog is
  the new class live, so that is the first moment `GetMaxHP()` means the right thing. Clamping inside
  `M.finish` clamps to the old maximum again and fixes nothing.
- ⚠️ It only ever **lowers** — a character legitimately below maximum regens normally.
- ⚠️ HP, mana **and** endurance all need it; all three are class-derived.
- 📌 The tell is in the DATA, not the code: `character_data.cur_hp` **greater than** the new class's
  maximum. Check that first on any "impossible HP" report.

### (b) A death that respawns you in the zone you died in leaves you half-dead — `zone/attack.cpp`
`Client::Death` does **`SetID(0)`** and **`dead = true`**. Both are undone ONLY by `ClearHover()` — the
respawn-**window** path — or by a real zone change re-creating the Client. `Character:RespawnFromHover`
is **false** here, so the window path never runs, and `GoToDeath()` is `MovePC` to your bind: when
that bind is the zone you are standing in, it is an **in-zone move, not a zone change**.
Result: alive at full health server-side, dead client-side — entity id 0, `dead` stuck true, and the
whole tic block in `Client::Process` (`if (tic_timer.Check() && !dead)`) skipped, so **no CalcMaxHP,
no DoHPRegen, no rest state**, and zoning blocked.
- ⚠️⚠️ **THIS SERVER MAKES IT THE COMMON CASE.** It is a roguelite where everyone binds to the
  Resplendent hub and dies constantly, so any death *in* the hub hits it every time.
- Fixed by detecting the same-zone respawn **before** `GoToDeath()` (afterwards the comparison no
  longer means "did we change zone") and calling `ClearHover()` + `SendHPUpdate()` after — doing by
  hand exactly what a zone change would have done.

### 📌 How to tell these apart quickly, and the trap that cost the most time
**`#showstats` reports the SERVER's view.** If it says full health while the bar says 0 percent, the
server is fine and the problem is state the client was never told to leave — (a) or (b), not damage.
- ⚠️⚠️ **A GUARD THAT NEVER FIRES IS EVIDENCE, NOT A FAILED FIX.** A defensive floor was added to
  `Client::CalcMaxHP` (never let `max_hp` go non-positive; repair a negative `current_hp`) and it
  logged **nothing** through every reproduction. That silence was the clue: under (b) `CalcMaxHP` is
  not being called at all, and under (a) the server value was never negative. The guard is kept as
  cheap insurance, but neither bug was what it was built for.
- ⚠️ `QuestErrors` ships with **`log_to_file = 0`** (console + gmsay only), so a Lua error in a death
  or reforge path leaves **nothing in the zone log**. It is now set to file. Check that before
  concluding "no errors were logged".

## 35. ⚠️⚠️ RE-APPLY ORDER AFTER A NATIVE ITEM DUMP — 2026-08-05

A native-item rework (`aot_0.1.2.sql` + `aot_rules_0.1.2.sql` + `aot_items_0.1.3.sql`) arrives as
HeidiSQL dumps full of `REPLACE INTO items`. **REPLACE deletes the row and re-inserts it**, so every
per-row customisation on a replaced id is silently lost. Three things must be redone, IN THIS ORDER:

1. **`custom/sql/aotv4_gear_tiers.sql`** — tiers are GENERATED FROM the base item's stats, so a base
   rework leaves every Hallowed/Mythic row derived from numbers that no longer exist.
   ⚠️⚠️ **It fails silently and looks fine.** After the 2026-08-05 rework, of 15,341 Mythic/base pairs
   **3,166 had a Mythic with LESS AC than its own base** and 3,912 more had a wrong ratio — roughly
   47 percent — while the row counts stayed a healthy 27,146. Counting rows proves nothing; check the
   RATIO (Mythic AC should be exactly 2x base).
2. **`custom/sql/aotv4_craft_sockets.sql`** — sockets are a column on the item row, so all 8,889
   craftable wearables come back with the dump's own augslot values. Must run AFTER the tier script,
   because that regenerates the tier rows and would drop their 2/3 sockets again.
3. **`./shared_memory`** with world DOWN, then restart. `items` is shared memory.

### Merging `items_clone` into `items` — the actual procedure (done 2026-08-05)
The dumps land in the **staging** table `items_clone`; nothing merges it for you. It is base-only
(ids 1001-147494) with **zero** rows in the tier bands and **zero** in the AoTv4 custom band, so the
merge itself is safe — it is the *derived* data afterwards that is not.
1. ⚠️⚠️ **Back up `items` first**, and **verify COLUMN ORDER before any `SELECT *`**:
   `SELECT COUNT(*) FROM (…items…) a JOIN (…items_clone…) b ON a.ordinal_position=b.ordinal_position
   WHERE a.column_name <> b.column_name;` must be **0**. Both tables have 285 columns; a positional
   mismatch would silently write every value into the wrong field.
2. `REPLACE INTO items SELECT * FROM items_clone;` — ~7s, and the row count must **not** change
   (every clone id already exists; 0 new ids, 0 base items missing from the clone).
3. Then steps 1-3 above, **in that order**, then `DROP TABLE items_clone`. It is regenerable — the
   `.sql` file is its only source — so dropping loses nothing.
- ⚠️⚠️ **TO VERIFY THE IMPORT LANDED, DIFF THE CLONE AGAINST `items` EXCLUDING THE COLUMNS OUR OWN
  SCRIPTS REWRITE.** A naive full-row diff reports ~48,000 differences and looks catastrophic. It is
  not: `aotv4_craft_sockets.sql` rewrites the **augslot** columns, and `aotv4_gear_tiers.sql` edits
  **native rows in place** — `classes` (18,835), `races` (7,668), `loregroup` (37,576), `nodrop`
  (9,694). Exclude those and the correct result is **0 differences** on Name, ac, damage, hp, mana,
  delay, price, stats, norent and reclevel. Anything non-zero there is a genuinely incomplete import.

### ⚠️⚠️ AND DIFF `rule_values` AGAINST A PRE-MERGE BACKUP
The 2026-08-05 dump changed **64 rules**, none of them announced. Two were actively dangerous and
neither was visible without the diff:
- **`Expansion:CurrentExpansion` 0 -> 9** and **`UseCurrentExpansionAAOnly` false -> true**. The first
  opens Kunark..DoN wholesale, which defeats region locking and contradicts the era system (section
  12) that treats `aotv4_era` as the source of truth; the second is the flag section 6 says must stay
  **false** or AA loading breaks.
- **`AA:ExpPerPoint` 200,000 -> 23,976,503.** That constant is **duplicated in Lua**
  (`global_player.lua`, `AA_EXP_PER_POINT`) and the two MUST match -- the roguelite death payout is
  computed from the hardcoded copy, so they had silently drifted ~120x apart.
- Also changed and left alone pending a decision: the four `AoT:Mit*` melee-mitigation values
  (section 14), which the dump inherited from whatever snapshot it was built on.
📌 `rule_values` has **multiple rows per rule** (different `ruleset_id`), so scope updates by
`rule_name` alone or you fix one ruleset and leave another behind.

### 📌 Other things learned in that merge
- **`aot_items_0.1.3.sql` writes to `items_clone`, NOT `items`** — a staging table. Nothing merges it;
  a blind `REPLACE INTO items SELECT * FROM items_clone` would flatten tiers and sockets again.
- ⚠️ **The AoTv4 custom item band (147500-147953) is NOT in the dumps**, so sigils, delve augments,
  currencies, tools and masks survive untouched. Verify with a count (expect 352) rather than assuming.
- ⚠️ **World DELETES `login.my.cnf` after its pre-migration backup.** A second migration run then
  fails auth and **world exits** (section 15's trap). Recreate it read-only, or use explicit
  `-u/-p` credentials for manual dumps.
- 📌 `aotv4_client_install/spells_us.txt` is an **accidental pre-change snapshot of `spells_new`** and
  was the only way to recover six spells' original durations. Duration columns are fields **17
  (formula) and 18 (duration)**; find them by matching an untouched spell against the DB.

### Hallowed damage is 1.5x, Mythic 2x — and they are NOT chained
Both tiers are cloned from the **BASE** item, never one from the other. When both blocks read
`damage*2` the two tiers came out with **identical weapon damage** and Mythic looked like it gave no
upgrade at all. It shows only on weapons, which is why it survived so long -- every other stat does
step up between tiers.
⚠️ Keep Hallowed at `FLOOR(damage * 1.5)` and Mythic at `damage*2`. "Making them consistent" is the bug.

## 36. Self-buffs last ~3 days — extended in CODE, never in the data — 2026-08-05

A beneficial buff a player casts **on themselves** is floored at **`AoT:SelfBuffDurationTicks`
(43,200 ticks ≈ 3 days)** in `Mob::CalcBuffDuration` (`zone/spells.cpp`). A buff cast on **anyone
else** is untouched and keeps its **native** duration. Live EQ durations assume constant re-casting,
and this is a roguelite that returns everybody to level 1 over and over, so re-buffing was most of
what a run consisted of.

### ⚠️⚠️ THE PERMANENT-BUFF EXPERIMENT FAILED. DO NOT REBUILD IT.
Migrations **v17 / v19 / v20** rewrote `buffdurationformula` in `spells_new` — first to 11
(30*(level+3)), then to **50 (permanent)**. **v21 reverts all three.** Four separate failures, and
the first is the one that matters:

- ⚠️⚠️ **PERMANENT IS `-1`, AND THE `-1` ESCAPES.** `CalcBuffDuration_formula` returns **-1** for
  formula 50, and that value reaches the ramping-effect branches of `CalcSpellEffectValue_formula`:
  ```
  ticdif = CalcBuffDuration_formula(...) - std::max(ticsremaining - 1, 0);   // -1 - 0 = -1
  ```
  so the effect **magnitude** broke while the buff **icon** persisted. Reported from play as *"the
  buff stays but the stats don't"* — on zoning **and** after death. A large finite tick count cannot
  do this. **Never use formula 50 for player buffs.**
- ⚠️⚠️ **BARD SONGS BECAME UNSTOPPABLE.** A song that never expires keeps its pulse alive, so the
  singer sings forever with no way to stop — reported as making gameplay impossible. Songs are now
  excluded outright.
- ⚠️⚠️ **REWRITING THE DATA DESTROYED THE NATIVE DURATION**, which made "a buff on someone else keeps
  its native duration" unimplementable — there was nothing left to hand out. This is the single
  strongest reason the extension belongs in **code**: the data stays native and the else-case is
  simply *do nothing*.
- ⚠️ The spell-gem leash (`PermanentBuffNeedsMemmed`) existed only as the price of permanence and is
  **removed**. Swapping a gem no longer costs you a buff.

### Recovering the native durations — v21
- ⚠️⚠️ **THE ONLY SURVIVING COPY WAS `aotv4_client_install/spells_us.txt`**, a pre-change snapshot
  from **2026-08-03** (the migrations landed 08-05). Fields **17** and **18** are
  `buffdurationformula` and `buffduration`. §35 already records that file as a duration recovery
  source; this is the second time it has saved a session. **Do not delete old client exports.**
- ⚠️⚠️ **ONLY 1,760 OF THE 3,942 SPELLS AT FORMULA 11/50 ARE RESTORED.** Formula 11 and formula 50 are
  **both legitimate stock formulas** — 2,183 of those spells were already 11 or 50 natively (1,103
  stock spells ship formula 50). Restoring all of them would give genuinely permanent stock buffs a
  timer they never had. Diff against the snapshot; never assume "at formula 11" means "we changed it".
- ⚠️ Sentinel: spell **7 Hymn of Restoration** is natively formula 5 / duration 3. The migration is
  idempotent on that test.

### The exclusions, all in `CalcBuffDuration`
- ⚠️ **`res > 0` only** — leaves natively permanent buffs (**-1**) and **auras** (**-4**) exactly as
  they are. An aura is scoped by range and membership and must never become a timed buff.
- ⚠️ **`caster == target`, or both are CLIENTS in the same group** (see the group section below).
- ⚠️ **Disciplines** (`IsDiscipline`), **charged buffs** (`hit_number > 0` — spent by use, not time),
  the **AoTv4 band 43000-44999** (code-driven lifecycles), and **invulnerability / true HoT / death
  saves** (SPA 40, 100, 101, 150, 232, 319).
- ⚠️ Applied with `std::max`, i.e. a **floor** — a spell whose native duration is already longer is
  never shortened.

#### ⚠️⚠️ TWO EXCLUSIONS WERE LOST MOVING FROM SQL TO CODE — BOTH RESTORED, DO NOT DROP THEM AGAIN
The v19 SQL carried exclusions that the first code version silently did not. Neither fails loudly;
both just quietly hand out three-day buffs. Caught only because someone asked *"we did exclude heals
and invulnerabilities, right?"*
1. ⚠️⚠️ **SPA 0 ABOVE A BASE OF 30 — the heal engines (`aotv4_is_heal_engine`, 382 spells).** SPA 0
   (`SE_CurrentHP`) on a **duration** spell repeats every tick, so it is a heal-over-time that never
   touches SPA 100 — filtering on 100/101/319 alone misses **every classic regen and every classic
   heal song**. **Magnitude is the only discriminator**: Hymn of Restoration is base **1** and Cantata
   of Soothing **4** (regens, kept); the Cantata / Chorus of Rodcet lines run to hundreds a tick
   (excluded). ⚠️ The stakes went **up** in the move — beneficial songs no longer pulse and so are now
   extended, and group mates are covered, so the miss would have created three-day **group** heal
   engines.
2. ⚠️⚠️ **NOT PLAYER-CASTABLE (`aotv4_player_castable`).** `caster->IsClient()` is **not** this test —
   a weapon **proc** and an **item click** are both cast with the player as caster *and* target, so
   they pass `caster == target && IsClient()` and would get three days (permanent clicky haste, procs
   that never fall off). The class table discriminates: a real player spell carries a level 1-100 for
   some class, while proc/click/NPC-only spells carry 255 everywhere. This was
   `LEAST(classes1..16) BETWEEN 1 AND 100` in v19 — its single largest exclusion (~9,287 spells).
   NPC self-buffs are already covered by `IsClient()`; **procs and clicks are not.**
📌 The general lesson: **when a filter moves from SQL to code, enumerate the old WHERE clause line by
line and tick each one off.** Both misses here were clauses that had no obvious code equivalent, so
they simply evaporated.

### ⚠️⚠️ A BENEFICIAL BARD SONG IS A BUFF THAT LOOKS LIKE A SONG — it no longer PULSES
Stock sustains a song by re-pulsing it every 6 seconds for as long as the bard keeps singing, which
is *why* songs ship with tiny durations. Give that a long duration and it becomes unstoppable: the
singer is locked in song mode re-applying a buff that never expires. Reported from play as making
gameplay impossible.
The `IsPulsingBardSong` branch in `Client::CastedSpellFinished` is now gated on
**`!IsBeneficialSpell(spell_id)`**, so a beneficial song is cast **once**, lands on the group through
its own target type, takes the long duration, and can be right-clicked off like any other buff.
- ⚠️ **`bard_song_mode` stays true** — deliberately. The song keeps its bard casting behaviour
  (instant, movable, no fizzle) and its name, icon and instrument scaling. **Only the pulse is gone.**
  That is what "a buff that looks like a song" means here.
- ⚠️⚠️ **DETRIMENTAL SONGS STILL PULSE AND MUST.** Debuff and DoT songs are meant to be sustained,
  carry no long duration, and are how a bard actually fights — one-shotting them would be a real nerf.
- ⚠️⚠️ **THE TWO GUARDS SHARE ONE PREDICATE, IN OPPOSITE SENSES — THAT IS THE SAFETY PROPERTY.** The
  pulse test is `!IsBeneficialSpell` and the duration test is `IsBeneficialSpell`, so a song is
  **either** "no pulse + extended" **or** "pulses + native duration" and can never be both or neither.
  Two independent lists would drift; one predicate cannot. **Keep it that way** — do not reimplement
  either side as its own spell-id list or skill test.
  | | pulse (`!IsBeneficialSpell`) | extended (`IsBeneficialSpell`) |
  |---|---|---|
  | beneficial song | no | **yes** |
  | detrimental song | **yes** | no |
- 📌 Verified in the data too: **0** detrimental songs sit at formula 11 or 50 — all 190 keep native
  formulas (5, 0, 7, 3, 1, 6, 4). They were never in scope, because v17/v19/v20 all filtered on
  `goodEffect <> 0`. Re-check with that query if the migrations are ever re-run.
- ⚠️ The test is `IsBeneficialSpell`, **not** "is it a reward-pool spell": a bard's own stock buff
  songs need this exactly as much as an awarded one does.

### ⚠️⚠️ GROUP MATES COUNT AS YOURSELF — and the buffs come back off on disband
A buff cast on someone in your **group** gets the same long duration. Buffing the party is the point
of grouping, and extending only self-buffs meant a healer's own buffs lasted days while everything
they did for the group lasted minutes. Anyone **not** in your group still gets the spell's **native**
duration — which is the whole reason v21 had to put the native durations back in the data.
- ⚠️⚠️ **IT IS TAKEN BACK WHEN THE GROUP BREAKS UP, IN BOTH DIRECTIONS.**
  `Group::AoTv4FadeGroupBuffs` runs from **`DelMember`** (one member leaving: their buffs off
  everyone, everyone's off them) and **`DisbandGroup`** (`nullptr` = unwind every pairing, ≤6 members
  so 30 comparisons). Without it you could group for one second, blanket the party, disband, and walk
  away having handed three-day buffs to strangers.
- ⚠️⚠️ **BOTH CALLERS MUST RUN IT BEFORE `members[]` IS CLEARED.** `DelMember` nulls the slot a few
  lines later and `DisbandGroup` tears the array down; after either, there is no record of who was
  grouped with whom and the pairing is unrecoverable.
- ⚠️⚠️ **`Mob::AoTv4FadeBuffsCastBy` MATCHES ON `caster_name`, NOT `casterid`.** `casterid` is an
  **entity id** and does not survive zoning, so a group mate who zoned would stop matching and keep
  the buff forever. `caster_name` is a real `char[64]` on `Buffs_Struct` and is persisted in
  `character_buffs`. Same trap the retired spell-gem leash hit.
- ⚠️⚠️ **SELF-CAST BUFFS ARE NEVER STRIPPED.** Your own buffs carry *your* name, so without the
  explicit name guard leaving a group would tear off your own long self-buffs, which have nothing to
  do with the group.
- ⚠️ **Beneficial only** — stripping debuffs on disband would be a free cleanse.
- ⚠️ Clients on both sides only; a pet, bot or merc holding a group buff is transient and dies with
  its owner's session.

### ⚠️⚠️ DEATH FADES EVERY BUFF — `death_loss.M.process`
`client:BuffFadeAll()` runs immediately before the spellbook wipe. The engine's own death handling is
**not** enough: `Client::Death` calls `BuffFadeNonPersistDeath`, which deliberately **spares** any
spell flagged `persist_death` — and those were exactly the buffs observed surviving a death with the
icon intact and the stats gone. A buff that outlives the spellbook is one the player can no longer
cast, re-apply or identify.
📌 `Spells:BuffsFadeOnDeath` (currently **true**) is the engine-level switch if this is ever revisited.

## 37. Combat no longer banks progress — v18 + `AoT:NPCFullHealOnReset` — 2026-08-05

Two changes with one theme: **you cannot store progress against a monster between attempts.**

### Enrage is off (migration v18)
404 `npc_types` carry the Enrage special ability; while enraged a mob **ripostes every frontal melee
attack** (`zone/attack.cpp:492`), which turns a fight into "stop attacking and turn away".
- ⚠️⚠️ **THE RULE NAME READS BACKWARDS: `NPC:LiveLikeEnrage = true` is what DISABLES it.**
  `Mob::StartEnrage` returns early when the rule is set *unless* the mob is a player-controlled pet or
  swarm pet — so "live like" means "only player pets enrage, as on live". Reading it as "enable
  enrage" and setting it false does the exact opposite.
- ⚠️ Done as a **rule**, not by stripping the ability off 404 rows: one reversible value instead of a
  destructive edit that loses which mobs were meant to have it. `NPC:StartEnrageValue` stays meaningful.
- ⚠️ **Rules are read at ZONE BOOT** — a zone restart or `#reloadrules`, not just a world restart.

### An NPC that drops combat resets to full health and mana (`zone/mob_ai.cpp`)
Stock leaves a disengaged mob on whatever health it was left at, so damage **banks** across attempts:
chip it, run, come back, chip it again — and the next player finds a softened mob for full loot.
Rule **`AoT:NPCFullHealOnReset`** (default **true**). **Needs a zone rebuild.**
- ⚠️⚠️ **`Mob::AI_Event_NoLongerEngaged` IS THE ONLY PLACE IT GOES.** All three ways a hate list can
  empty funnel through it — `Mob::RemoveFromHateList`, `Mob::WipeHateList` and the 10-minute
  stale-entry sweep in `Mob::AI_Process` — so one edit covers fleeing, feign death, the target zoning
  out, the target dying and hate simply going stale. Per-call-site copies would drift.
- ⚠️⚠️ **`GetHP() > 0` IS LOAD BEARING — WITHOUT IT THIS RESURRECTS THE MOB YOU JUST KILLED.**
  `NPC::Death` does `SetHP(0)` (`zone/attack.cpp:2754`) and only **later** calls `WipeHateList()`
  (`:3350`), which lands here with the hate list still populated — so this genuinely runs on a
  corpse-to-be, and an unguarded `RestoreHealth()` puts it back to full before `p_depop` is set. The
  combat-event block directly above guards the same way; that is precedent, not coincidence.
- ⚠️⚠️ **OWNED MOBS ARE EXCLUDED, AND THAT IS THE WHOLE EXPLOIT SURFACE.** A charmed mob and a
  summoned pet are both NPCs — without the test every pet would full heal each time it disengaged,
  attrition against a pet class would stop meaning anything, and **charm would become a free heal
  engine on a timer the player controls**. Swarm pets need a separate check (`GetSwarmOwner()`, not
  `GetOwnerID()`).
- ⚠️ **Mana is restored too** — NPC casters have a real pool and the AI cast gate reads it, so without
  it a caster could be drained across repeated pulls. The delve warden (§24) is built around a full
  pool at fight start. **Endurance is deliberately NOT restored**: `Mob::SetEndurance` is a no-op and
  `Mob::GetMaxEndurance` returns 0 for anything that is not a `Client`, so the call would do nothing
  and only imply NPC endurance mattered here.
- ⚠️ **`GetSwarmOwner()` is on `NPC`, not `Mob`** — it needs the `CastToNPC()` the surrounding block
  already uses. `HasOwner()` *is* on `Mob`. Writing the pair symmetrically does not compile.
- 📌 **`RestoreHealth()` does NOT recalculate max HP, despite calling `SetMaxHP()`.** That is
  `current_hp = max_hp` (`zone/mob.h:567`) and **only `Client` overrides it** — so a delve mob's
  scaled `max_hp` (§24, applied via `ModifyNPCStat`) survives a reset intact. The name reads like a
  recalculation and is the obvious thing to fear here; it is not one.
- ⚠️ **`GetSwarmOwner()` is on `NPC`, not `Mob`** — it needs the `CastToNPC()` the surrounding block
  already uses. `HasOwner()` *is* on `Mob`. Writing the pair symmetrically does not compile.
- 📌 **`RestoreHealth()` does NOT recalculate max HP, despite calling `SetMaxHP()`.** That is
  `current_hp = max_hp` (`zone/mob.h:567`) and **only `Client` overrides it** — so a delve mob's
  scaled `max_hp` (§24, applied via `ModifyNPCStat`) survives a reset intact. The name reads like a
  recalculation and is the obvious thing to fear here; it is not one.
- 📌 Feign death now yields a **full-health** mob on stand-up rather than a softened one. That is the
  intended direction, not a regression.

## 38. ⚠️⚠️ NUMBERS INHERITED FROM LIVE EQ THAT A LEVEL 30 CAP BREAKS — 2026-08-05

Two unrelated-looking reports, one root cause: **stock values tuned for a level 70+ / 10,000 hp game
mean something entirely different at our cap**, and nothing warns you.

### A percentage-based effect is silently worth almost nothing (migration v15)
Reported as *"Bulwark Within increased my mana and endurance, but didn't increase HP"*. It was not
broken — it is **stock**, and the cap is what breaks it. The AA mixes effect KINDS:
`SPA 214 MaxHPChange` is a **percentage** (`CalcMaxHP` divides by 10000) while `SPA 97 ManaPool` and
`SPA 190 EndurancePool` beside it are **flat**. base1 300 = 3 percent: ~300 hp on live, **~45 at our
~1,500 hp**, which reads as nothing next to the flat +200 mana.
- ⚠️⚠️ **THIS IS A CLASS OF PROBLEM, NOT ONE AA.** Any percentage-based effect is devalued by a low
  cap while flat ones keep full value. **`SPA 69 TotalHP` is the flat counterpart**
  (`bonuses.cpp:786` → `FlatMaxHPChange`), so swapping the effect id while keeping base1 turns
  2 / 1.5 / 3 percent into a flat **+200 / +150 / +300** — proportional to what they were.
- ⚠️ Scoped to the **three enabled** AAs (rank ids 279, 423, 1367). **17 rows use SPA 214 in total**;
  the other 14 sit on disabled AAs and will need the same treatment if they are ever switched on.
  Converting all 17 would silently rebalance AAs nobody can currently train.

#### ⚠️⚠️ AND THEN "Bulwark within is not giving mana" — MANA IS CLASS-BRANCHED (v34 / v35, 2026-08-07)
v34 matched the amounts (HP 300 but mana 200 / endurance 200 → **300 / 300 / 300**) so the AA reads as
one coherent grant. The follow-up report was that the mana still did nothing, and **that is not this
AA**:
- ⚠️⚠️ **`Client::CalcMaxMana` (`zone/client_mods.cpp`) BRANCHES ON CLASS.** Caster classes get
  `CalcBaseMana()` + item + spell + AA bonuses; **Warrior, Monk, Rogue and Berserker get a flat
  `GetLevel() * 40` with NO bonus terms at all.** That is deliberate — §14 records that the dll
  computes the mana **gauge** itself from `level * AOTV4_MELEE_MANA_PER_LEVEL` using only
  `SPAWNINFO->Level`, so the server must produce the identical number or the bar misreports.
- ⚠️⚠️ **THE CONSEQUENCE IS FAR WIDER THAN ONE AA: 40,103 wearable mana items and 65 mana-granting AAs
  are ALL INERT for those four classes**, and mana is the one stat they cannot improve. On a server
  where every class casts (§14), that is a standing design hole, not a bug in Bulwark Within.
  📌 Fixing it is **one line** (add the three bonus terms to the melee branch) **plus a dll change** so
  the gauge agrees. Deliberately **not** done — it is a balance decision, not a bug fix.
- ⚠️ **Endurance is fine on every class**: `CalcMaxEndurance` has no class branch and already sums
  spell, item and AA bonuses. **Only mana is special-cased.**
- v35 therefore changes the **description**, not the grant: the engine already splits it the way it
  should (everyone banks HP and endurance, only casters bank mana), so the text now says so instead of
  promising all three to all.
  ⚠️⚠️ **The client resolves that string from its own `dbstr_us.txt`, NOT from this database** (§6) —
  writing `db_str` changes nothing in game until `./export_client_files` runs and the regenerated file
  ships to players. ⚠️ No literal `%`: the description path is printf-style and eats it as a format
  token. ⚠️ Only **type 4** is written; §6 records that *renaming* an AA means writing types 1/2/3 as
  well, but this is a description change and they are correctly left alone.

### A zone's experience multiplier is stock data, not something we set (migration v16)
Reported as *"blues in delves were giving 4-5% exp, while yellows in LOIO were giving 2%"*. Nothing in
the delve did this: the six delve zones are **Dragons of Norrath**, which ships the highest
`zone_exp_multiplier` in the game (**2.90-3.10**, because on live it is endgame), against Kunark's
**0.80** — a ~3.7x gap before con colour. Normalised to **1.00**.
- ⚠️ `zone_exp_multiplier` multiplies **normal xp** (`zone/exp.cpp:130`), **AA xp** (`:294`) **and
  group xp** (`:443`) — it is not just a kill-xp knob.
- ⚠️⚠️ **EVERY VERSION ROW, NOT JUST VERSION 0.** Zone config loads per `(zone, version)` and the
  delve **never uses version 0** (§24) — the mission versions are what players stand in, and they
  carried the *higher* value (thenest v0 is 3.05, its other 15 versions 3.10). Updating version 0
  alone would have changed nothing a player ever sees. 67 rows across the six zones.
- 📌 The delve's reward is the score sheet, the sigil, augments and coin — **not** raw experience. Its
  creatures are already scaled to the player, so a delve kill should be worth an equivalent
  open-world kill.

## 39. A bow's minimum range is melee range — `AoT:BowMinRangeIsMeleeRange` — 2026-08-05

Stock refuses a bow shot inside `Combat:MinRangedAttackDist` (**25 units**), so an archer had to break
off and back out of melee to use their own weapon. Rule **`AoT:BowMinRangeIsMeleeRange`** (default
**true**) makes the floor melee range instead. **Needs a zone rebuild** (`ruletypes.h` is in
`common/`, so it is a wide one).
Every class here can be handed Archery (§4), so "back up 25 units first" was a tax on a whole weapon
type rather than a class identity, and these fights are not built around kiting.

- ⚠️⚠️ **IT IS IMPLEMENTED AS "NO FLOOR", NOT AS A SMALLER FLOOR, AND THAT IS CORRECT.** Melee range
  is the **closest you can stand** to something you are fighting, so a minimum *of* melee range is
  satisfied at every distance you could ever be firing from — the floor cannot bite. The rule name
  describes the intent; the code is a skip.
- ⚠️⚠️ **DO NOT "IMPROVE" IT INTO A `CombatRange()` TEST.** That reads as more faithful and is
  strictly worse. Melee range for a normal size 6-8 mob is about **16 units** (`aggro.cpp:1124-1147`,
  `size_mod 8 → 8*8*4 = 256`) against a stock minimum of **25**, so "allow inside melee range, else
  keep the stock floor" leaves a **dead doughnut** between the two: you could shoot at 10 units and
  at 30, but not at 20. A rule that silently switches back on in a band nobody can see is far more
  confusing than no rule.
- ⚠️⚠️ **ONE CHOKE POINT COVERS BOTH PLAYER BOW PATHS.** `Client::RangedAttack`
  (`zone/special_attacks.cpp:905`) is reached from the server-driven **auto-fire loop**
  (`client_process.cpp:364`) **and** from the **manual archery combat ability**
  (`Client::OPCombatAbility`, `special_attacks.cpp:453`). Same reasoning as §22's endurance cost.
- ⚠️⚠️ **`Combat:MinRangedAttackDist` ITSELF IS LEFT ALONE — changing that rule instead would have
  silently let every enemy archer NPC shoot point blank too.** It still governs `NPC::RangedAttack`
  (`:1453`), the bot paths (`bot.cpp:3124`, `:7016`) and `Client::ThrowingAttack` (`:1634`).
- 📌 **Throwing is deliberately NOT included** — a separate function with its own copy of the check.
  Two lines if it is ever wanted.
- ⚠️ **Auto-fire is SERVER driven, which is why a server edit is enough for it.** The client sends
  `OP_AutoFire` **once, as a toggle** (`client_packet.cpp:3578`) and the server then fires on its own
  `ranged_timer` — so this check is the authoritative gate for sustained archery, despite the stock
  comment claiming the client catches it first.
  📌 **The manual single shot is the uncertain one.** That stock comment says the RoF2 client enforces
  the minimum and sends `RANGED_TOO_CLOSE` itself, so if the client refuses to *send* the archery
  combat ability point blank, that path stays blocked until a dll detour lifts it — the three-layer
  problem of §4 (client display, client send, server execute). Auto-fire is unaffected either way.
  **Test point blank with auto-fire first**, then the manual shot; a difference between the two is
  the client, not this rule.

## 40. ⚠️⚠️ A "NO-OP" THAT DOUBLED EVERY GEARED HEAL AND NUKE — 2026-08-05

`Mob::GetExtraSpellAmt` (`zone/effects.cpp`) had been short-circuited with a comment reading
*"Gear spell damage has been moved to ScaleSpellDamage. **This should do nothing**"* — and then
`return base_spell_dmg;`. **A no-op here is `return 0`.** Every one of the 13 call sites in that file
**accumulates** the result; there is not one assignment among them:

| call sites | shape | effect of returning `base_spell_dmg` |
|---|---|---|
| heals `:520 :527 :577 :584` | `value += GetExtraSpellAmt(...)` | `value = base + base` → **2x** |
| damage `:136 :187 :245 :300 :307 :346 :353` | `value -= GetExtraSpellAmt(...)` | damage is **negative**, so subtracting the negative base **also doubles it** |

- ⚠️⚠️ **IT ONLY FIRED FOR SOMEONE CARRYING THE ITEM STAT, WHICH IS WHY IT READ AS A SPELL BUG RATHER
  THAN A GLOBAL ONE.** Both heal branches are gated on `GetHealAmt()` and the damage ones on
  `GetSpellDmg()` / `itembonuses.SpellDmg`. A character with none saw the correct number; a geared one
  saw double. **Our gear tiers (§10) convert int → spelldmg and wis → healamt on every Hallowed and
  Mythic piece**, so in practice nearly every geared character was affected — and the naked test
  character was not.
- 📌 **Reported as "the Reptile line procs for double".** It was not the reptile line: base 20 → 40.
  Chasing it through the spell rows, the SPA 323 registration and the proc path found nothing wrong,
  because nothing there *was* wrong. The tell was that the multiplier was **exactly** 2 with no
  variance — a crit, a focus or a double-registration would all vary.
- ⚠️ **The damage half is the easy one to miss** — `value -= f(...)` looks like it *reduces* damage
  until you remember spell damage is carried negative. Reason about sign before concluding a
  subtraction is a nerf.
- ⚠️ **The stated intent was never wrong and is untouched**: gear spell damage genuinely does live in
  `ScaleSpellDamage` now, so this function's job really is to contribute nothing. Only the expression
  of "nothing" was wrong. The stock cast-time-curve implementation is deliberately left below the
  return, unreachable, as the reference if per-spell tuning is ever wanted here again.
- 📌 Distinct from the **DC Overpower** inflation already recorded in §5 (`zone/mob.cpp:8860`), which
  is a real designed bonus and varies with the caster/target gap. Two different reasons observed
  numbers exceed the spell row; do not fix one by looking at the other.
