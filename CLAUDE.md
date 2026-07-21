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
  - `core_spellwindow.cpp` — the spell window, the **AA window** (§6), the **Portal window** (§11),
    **and** the skill-unlock hook; all share the `dsp_chat` + `ProcessGameEvents` detours.
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
  Two C++ hooks (tracked patches): `NPC::AddLootDropTable` (loot.cpp) rolls **25% Hallowed / 5%
  Mythic** per base drop (mode-independent, since `lootdrop_entries.chance` is a weight in the
  dominant weighted loot mode); `AoTv4MythicReward` (questmgr.cpp + lua_client.cpp) upgrades
  **quest-reward** gear to its Mythic tier (epics never hand out native). Re-run the SQL → rebuild
  shared memory → restart.
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

## 12. Expansion-unlock-on-AA-maxout (era progression)

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

## 13. Player shop (permanent escrow trader) — `/shop`

An AFK player shop that works from **any city** (no Bazaar zone, no NPC, no Trader's Satchel), documented
in full in **`HANDOFF.md`**. Items are **escrowed** (they leave your bags when listed → no dupe), searchable
via the native `/bazaar`, delivered by parcel, and paid to the seller on sale (instantly if online, else
banked to next login). Managed by a **two-tab dll window** (`/shop`).

- **Protocol (chat, dll swallows all of it):** `/shop` → dll rewrites to `/say shopopen`; server replies
  `SHOPINVDATA slot|itemid|name|vendor^…` (Add-Items tab: your droppable bag items) + `MYSHOPDATA
  itemsn|itemid|name|cost^…` (My-Shop tab: current listings). dll → `/say shopadd slot:copper,…` (escrow
  into shop), `/say shoppull <itemsn>` (unlist → cursor), `/say shoprefresh`.
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
- **Do NOT resurrect** (removed on request): the Bazaar Broker NPC (2000050), the Shopkeeper stand-in NPC
  (2000051), the Trader's-Satchel `vpset/vshop` flow, or `Bazaar:UseAlternateBazaarSearch` (keep it false —
  it's Bazaar-zone-sharded, the opposite of trader-anywhere).
