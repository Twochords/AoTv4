`# AoTv4 — Expansion-Unlock-on-AA-Maxout (design plan)

**Status: PLAN ONLY — no code written yet.** Ladder target: **Classic → Omens of War**.

## 1. Concept

A server-wide, communal progression race layered on the roguelite AA meta. Each character
banks AA on death and spends it via the random picker (existing system). When the **first**
character on the server **maxes out the current era's AA pool**, the **next expansion unlocks
for everyone** — raising the level cap and widening the spell / AA / zone / item pools. Repeat
per expansion up through OoW.

> "Kunark unlocks when someone maxes out [classic]." The maxout is the gate; the unlock is
> server-wide. First-to-max gets public recognition.

## 2. Current state (verified)

| Thing | Reality today | Implication |
|---|---|---|
| Level cap | `Character:MaxLevel = 70` on the **active `default` ruleset** — **not 50** | The "50 cap" is only a balance intent; nothing enforces it. The era system must enforce a per-era cap. |
| AAs' origin | All AAs are **Luclin+** (`aa_ranks.expansion >= 3`); no classic/Kunark/Velious AAs exist | "Classic AA pool" is a **curation choice**, not a DB fact (we already offer Luclin–PoP AAs at classic via the migration). |
| AA pool | One flat list tagged `expansion BETWEEN 0 AND 4`, `level_req` lowered to 1; **ceiling = 807 pts** at `RANK_BUDGET=10` | Needs to become **era-tiered** and keyed to a server flag. |
| Spell pool | Level-banded only (offered if learn-level ≤ player level); **not** era-gated | At level 60 a player would already be offered Kunark/Velious spells — no era gate today. |
| Content filter | `Expansion:CurrentExpansion = 0` (Classic) | This is the real zone/item/door gate; advancing it per era opens content. |
| Per-char expansions | `World:ExpansionSettings = 524287` (all) | Fine; `m_pp.expansions` stays all-on. The content filter does the gating. |

## 3. Era ladder (Classic → OoW)

EQEmu expansion numbers and standard caps. `aotv4_era` = the unlocked index (0..8).

| era | name | exp # | level cap | AA ceiling | notes |
|---|---|---|---|---|---|
| 0 | Classic | 0 | 50 | ~807 (current pool, re-curate) | start state |
| 1 | Kunark | 1 | 60 | TBD | +10 levels, Kunark spells/zones |
| 2 | Velious | 2 | 60 | TBD | zones/gear; cap unchanged |
| 3 | Luclin | 3 | 60 | TBD | true AA era begins (no-op for us, AAs already on) |
| 4 | Planes of Power | 4 | 65 | TBD | +5 levels, PoK hub already wired (§11) |
| 5 | Legacy of Ykesha | 5 | 65 | TBD | small |
| 6 | Lost Dungeons of Norrath | 6 | 65 | TBD | LDoN |
| 7 | Gates of Discord | 7 | 65 | TBD | GoD |
| 8 | Omens of War | 8 | 70 | TBD | final target; cap 70 matches current MaxLevel |

AA ceilings per era are computed the same way as the classic 807:
`sum over the era's AA set of cost × min(floor(RANK_BUDGET/cost), reachable_ranks)`.
See §5c for the actual AA inventory and the proposed per-era layout/ceilings.

## 3b. AA inventory & layout (the ceiling plan)

**The whole AA universe through OoW** (enabled, activated/passive types 1–5, real-named, deduped
by name), by the AA's native DB expansion — `ceiling = Σ cost × min(floor(10/cost), ranks≤cap)`:

| native batch | #AAs | ceiling (pts) |
|---|---|---|
| Generals/Innates (exp 0) | 24 | 43 |
| Luclin (exp 3) | 139 | 929 |
| Planes of Power (exp 4) | 66 | 515 |
| Gates of Discord (exp 7) | 63 | 508 |
| Omens of War (exp 8) | 62 | 503 |
| **TOTAL** | **354** | **~2,498** |

**The layout problem:** AAs only exist at exp 0/3/4/7/8 — **Kunark, Velious, LoY, LDoN have no
native AAs** (AAs didn't exist before Luclin). So the 5 batches don't line up with 9 eras. Three
ways to lay it out:

- **L1 — Hybrid spread (recommended):** split the big batches across the gateless eras (Luclin's
  929 across the four 60-cap eras; PoP's 515 across the three 65-cap eras; GoD/OoW their own).
  Every era gets a real AA-max gate. Thematic mapping is loose, but AAs are already era-divorced.
- **L2 — Even partition:** chop the 2,498 into 9 equal ~278-pt tiers. Simplest, most uniform race,
  fully arbitrary assignment.
- **L3 — AA-gated where AAs exist:** only Classic/PoP/GoD/OoW use an AA-maxout gate; Kunark/
  Velious/LoY/LDoN unlock on a *non-AA* milestone (hit the level cap, or a flat AA-spend bar).

**Chosen layout — capped at ≤20 deaths/era.** Points are allocated *up* out of the lopsided
GoD/OoW batches (508/503) into the earlier eras, and the banking dials are tuned so **no era takes
more than 20 deaths** to clear (hard, but never a boring slog). Fastest player; **`AA_XP_BASE=8.8M`
→ a no-AA L50 death banks 18**, **`AA_SCALE=0.003`** (gentle taper). Each era's higher cap raises
XP/death and *offsets* the diminishing taper — banking bounces back up at every unlock:

| era | cap | era AAs (pts) | cumulative ceiling | bank/death (start→end) | deaths this era |
|---|---|---|---|---|---|
| Classic | 50 | 250 | 250 | 18 → 9 | 19 |
| Kunark | 60 | 470 | 720 | 30 → 18 | 17 |
| Velious | 60 | 300 | 1,020 | 17 → 14 | 16 |
| Luclin | 60 | 250 | 1,270 | 14 → 12 | 17 |
| PoP | 65 | 290 | 1,560 | 14 → 13 | 18 |
| LoY | 65 | 250 | 1,810 | 13 → 11 | 18 |
| LDoN | 65 | 230 | 2,040 | 11 → 10 | 18 |
| GoD | 65 | 230 | 2,270 | 10 → 9 | 20 |
| OoW | 70 | 228 | 2,498 | 13 → 11 | 17 |

**~160 deaths total** for the fastest player to ride Classic → fully-maxed OoW, with each era a
tight **16–20 death** milestone. The banking floor never drops below ~9/death, so it never feels
like spinning your wheels.

> Two master dials: the **per-era point split** sets each era's length; **`AA_XP_BASE`/`AA_SCALE`**
> set how fast you bank. Want it harder without breaking the ≤20 cap? Raise `RANK_BUDGET` (deepens
> every AA → higher ceilings) and re-tune the dials. The death cadence re-derives automatically.

**Tuning levers for the ceiling/cadence:**
- **Per-era partition** (the `add` column) — directly sets each era's race length.
- **`RANK_BUDGET`** (currently 10) — the per-AA cap; raising it deepens every AA and lifts all
  ceilings (e.g. → 15 lets cost-2 AAs reach rank 7). Keep it in step with the cap per CLAUDE.md.
- **`AA_XP_BASE` / `AA_SCALE`** — set the bank-per-death and how fast it tapers.

## 4. Core data model

- **`aotv4_era`** — a single **global** data bucket (not per-char), the unlocked era index.
  Read at zone boot into a module-level value; the authoritative server-wide state.
- **`era_table`** (Lua) — `era → { level_cap, expansion, aa_ceiling, spell_max_level, … }`,
  the single source of truth the rest of the systems read.
- **Persistence**: `aotv4_era` lives in `data_buckets` (survives restarts). Changing it is rare
  (once per expansion unlock, server lifetime).

## 5. Per-system mechanism

### (a) Level cap enforcement
Today nothing stops level > 50. Add enforcement keyed to `era_table[era].level_cap`:
- **Option A (rule):** set `Character:MaxLevel` = era cap on unlock. Simple, but rule changes
  need a **world+zone restart** to take effect, and it's a global ruleset edit.
- **Option B (Lua, live):** in `event_level_up`, if `GetLevel() >= cap` then clamp
  (`SetLevel(cap)` / suppress further XP). Fully live, no restart, per-era from `era_table`.
  **Recommended.** (Also stop banking XP toward levels past cap.)

### (b) Spell pool — add an era gate
The pool stays level-indexed, but `spell_choice.gather_candidates` gains an era ceiling:
only offer spells whose learn level ≤ `era_table[era].spell_max_level` (50/60/65/70). Cheapest
path: precompute a per-spell era tag at generation (band by learn level) and filter at runtime.
Net effect: at Classic you only ever see ≤50 spells even if you somehow out-leveled.

### (c) AA pool — era-tiering (the main design fork)
Because all AAs are Luclin-origin, "which AAs belong to which era" is **our** choice. Two ways:

- **C1 — Curate down + grow:** define a hand-picked **Classic set** (smaller than today's 807),
  and each unlock *adds* a new batch. Most faithful to "classic feels classic," most curation
  work. Ceilings rise era to era.
- **C2 — Gate the existing pool by tiers:** keep the current Luclin–PoP pool as the Classic
  tier (ceiling 807), and each later era pulls in **higher-DB-expansion AAs** (GoD/OoW =
  `aa_ranks.expansion 7/8`) that aren't offered yet. Less curation; the "era" of an AA is just
  which tier batch we assigned it to. **Recommended for speed.**

Either way: tag each pool AA with an `era` field, and `aa_choice.gather_affordable` filters
`aa.era <= current_era`. Recompute the ceiling per era. The **maxout test** becomes
`GetSpentAA() >= era_table[era].aa_ceiling` (or "every AA with `era <= current` at its cap").

### (d) Content filter (zones / items / doors)
On unlock, set `Expansion:CurrentExpansion = era_table[era].expansion`. This is what actually
lets Kunark+ zones spawn and items drop. **Rule change → needs a zone restart** to apply (doors
load at zone boot; items are shared-memory). So content opening is the one part that isn't live.

### (e) Gear tiers / zones
The Hallowed/Mythic tiers (§ gear) already span obtainable gear; as `CurrentExpansion` rises and
new zones/loot become reachable, the existing `AddTierUpgrades` hook tiers them automatically.
No new gear work to unlock an era — it rides the content filter.

## 6. Unlock pipeline

```
aapick (aa_choice.handle_say, after a successful train)
   └─ if GetSpentAA() >= era_table[era].aa_ceiling  AND  era < MAX_ERA:
         set global aotv4_era = era + 1
         worldwide broadcast: "<Name> has mastered the <era> arts — <next> is now open!"
         apply_era(era+1):
            • live now:   level-cap clamp (5a-B), spell gate (5b), AA gate (5c)
            • needs zone restart: Expansion:CurrentExpansion (5d) — schedule / GM #reload
```

- **Detection** is cheap and runs only on picks. Guard against double-fire (compare-and-set on
  the bucket).
- **Live vs restart**: levels/spells/AAs flip instantly from the flag; **zone/item content
  (5d) needs a restart** — acceptable for a once-per-expansion event (announce "Kunark opens
  after the next world reboot," or automate a rolling zone bounce).

## 7. The race / recognition

- First char to cross the ceiling is the trigger. Record their name in a bucket
  (`aotv4_era_unlocker_<era>`) and announce server-wide.
- Optional: a title, a one-time Mythic reward, or a leaderboard say-command.

## 8. Open decisions (need answers before building)

1. **Level-cap enforcement**: Lua clamp (live, recommended) vs `MaxLevel` rule (restart)?
2. **AA tiering**: C1 (curate classic down) vs C2 (gate existing pool, pull higher-exp AAs in)?
3. **What each unlock concretely adds** beyond cap+content: new AA batch sizes, any new
   combat skills, gear tier bumps?
4. **Content-open timing**: announce-then-manual-restart vs automated zone bounce on unlock?
5. **Maxout definition**: full AA ceiling, or an earlier milestone (e.g. 80% / a specific
   "capstone" AA) so the race ends in a reasonable number of deaths (~179 for full at 15/death)?

## 9. Suggested build phases (when greenlit)

1. **Foundation**: `aotv4_era` bucket + `era_table` + `apply_era()` + boot-time load. No
   behavior change at era 0 (validates plumbing).
2. **Gates**: wire level-cap clamp, spell era ceiling, AA era filter to `era_table`.
3. **Detection + unlock**: maxout test in `aa_choice.handle_say`, broadcast, compare-and-set.
4. **Classic→Kunark end to end** as the proof (cap 50→60, Kunark spells/AAs/zones).
5. **Fill the ladder** Velious…OoW (data: per-era AA/spell tags, ceilings, caps).

## 10. Effort / risk notes

- Mostly **Lua + SQL/data** (era tags, ceilings); little or no C++ (the level clamp is Lua).
- Biggest data task: defining per-era AA & spell sets and computing ceilings (§5b/c).
- Only **content open (5d)** needs restarts; everything else is live off the flag.
- Keep `RANK_BUDGET` and `AA_XP_BASE` in step with each era's cap (higher cap → more XP/death →
  raise both), per the existing CLAUDE.md guidance.
