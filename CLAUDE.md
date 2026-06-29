# AoTv4 — Bard-only Random-Progression EQ Server

EQEmu server + RoF2 client mod. The headline feature is a **level-up reward window**: on each
level you're offered **3 random rewards** (spells or class-specific combat abilities) and pick
one, which the server scribes/trains. Everyone plays a **Bard** (it both melee-disciplines and
casts, so one class covers all reward types). The window is drawn by a client-side `dinput8.dll`
so it works on a **vanilla RoF2 client — no MacroQuest/E3 required**.

---

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
  - `core_spellwindow.cpp` — the spell window, the **AA window** (§6), **and** the skill-unlock
    hook; all three share the `dsp_chat` + `ProcessGameEvents` detours.
  - `core_skillunlock.h` — declares `EnableSkillUnlock()` (impl is in `core_spellwindow.cpp`).
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
- `global_player.lua`: `event_connect` forces **Bard** + sends `SKILLUNLOCKDATA` (see §4);
  `event_level_up` auto-grants non-combat skills then calls `spell_choice.offer(e)`;
  `event_say` calls `spell_choice.handle_say(e)`.
- `spell_choice.lua` — builds the 3 choices (spells + ~1 combat skill), emits `SPELLCHOICEDATA`,
  validates picks. Stored bucket tokens are **typed**: `S:<spellid>` (spell/disc) or
  `K:<skillid>` (combat skill). Tunables at top: `CHOICE_COUNT`, `LEVEL_BAND`, `SAY_TRIGGER`,
  `SKILL_OFFER_CHANCE` (~0.7), `SKILL_GRANT_PCT` (0.25).
- `spell_pool.lua` (**generated**, see §5), `spell_icons.lua` (**generated**),
  `spell_blacklist.lua` (**generated**), `skill_pool.lua` (hand-curated; see §4).

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

## 6. Death-driven AA rewards (roguelite)

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
- **Every char is a Bard** (`character_data.class = 8`; also forced in `event_connect`).
- **`spells_new.classes8`** = the spell's min learnable level (Bard-usable + pool index).
- **Bard `skill_caps`** (`class_id=8`) raised so skills scale; client also needs the exported
  `SkillCaps.txt` (`export_client_files`) installed in the EQ root.
- **Expansion lock:** `rule_values Expansion:CurrentExpansion = 0` (Classic).
- Test char `Ashrem`: Bard, GM. (Reset a char's combat skills via
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
- **Lua + server-code reloads:** `#reloadquest` does NOT reload `require`d Lua modules, and a
  rebuilt `zone` binary only takes effect in **freshly-booted** zones. After a Lua or C++ change,
  kill **all** zone procs (`for p in $(pgrep -x zone); do kill -9 $p; done`) so `eqlaunch`
  respawns them — killing only `dynamic_*` leaves already-booted named zones (e.g. freporte) on
  the old code. Then **relog**.
- **Can't hook EQ's D3D device** — dummy-device vtable trick fails here; use the layered window.
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
- **`small` is a Windows macro** — `<rpcndr.h>` does `#define small char`, so a local named
  `small` (e.g. `HFONT small`) miscompiles into `HFONT char` with a cascade of bogus errors. Avoid
  `small`/`hyper` as identifiers in the dll.
- **AA grant rejected silently** — `GrantAlternateAdvancementAbility(...false)` returns false (no
  message) if any of: class bit, `rank->expansion` vs `UseCurrentExpansionAAOnly`, `level_req`,
  cost > unspent, or unmet `aa_rank_prereqs`. The picker pre-filters on affordability + prereqs so
  only grantable AAs are ever offered.
