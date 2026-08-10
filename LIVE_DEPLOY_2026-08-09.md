# Live deployment — migrations v37 → v53 + binaries + Lua

Everything below was built and verified on the **test** server (`/src`) on 2026-08-08/09.
Live is a **separate database** (§25) and is known to be behind — the enrage report proved it.

⚠️⚠️ **A DEPLOY IS FOUR THINGS, NOT TWO.** Binaries, migrations, **the Lua modules** (§1b) and the
client files (§5). The Lua is the one with no version number anywhere — nothing reports that live is
running an older `aotv4_dungeon.lua`, so it is the half that gets forgotten. Three of this batch's
player-reported fixes are Lua-only and ship no other way.

⚠️⚠️ **v50 IS THE URGENT ONE AND ITS SYMPTOM IS ON LIVE, NOT HERE.** Characters parked at the level
cap were accruing experience without bound, and the roguelite death converts total run experience
into AA — so deaths were paying **7 points** against an intended **2.32**. The test database has one
character and could never show it. See section 3b; it needs the `zone` binary AND the migration.

---

## 0. FIRST: find out how far behind live is

```sql
SELECT custom_version FROM db_version;
```

That number decides how much of this applies. Anything below **53** means some of the work below is
missing there. ⚠️ If it is below **18**, live has never had the enrage fix, the Bulwark Within fix, or
the zone XP normalisation — and a lot of "it's not working" reports will resolve themselves the moment
it catches up.

---

## 1. Ship the binaries

`world` and `zone` both changed. Neither is optional:

| Binary | Why it must ship |
|---|---|
| `zone` | flee/enrage guards, threat + shield rework, tradeskill gates, guard-kill credit, delve fixes, **the experience cap at the level cap (v50)**, **the melee-push force zeroing (v52)**, **the crucible item id (v53)** |
| `world` | **character creation skills** — a new character is born at its caps and tradeskill floor |

⚠️⚠️ For v50 the **binary is the fix and the migration is only the cleanup**. The migration trims
experience that is already banked; without the new `zone` it starts climbing again immediately.
Shipping the migration alone buys one death's worth of correctness and then the bug returns.

⚠️⚠️ **v52 AND v53 ARE EACH HALF A FIX, AND NEITHER HALF WORKS ALONE.** Both pair a migration with a
constant compiled into `zone`, and in both cases the mismatched state is *worse than not deploying*
— it looks configured and misbehaves:

| | migration only | binary only |
|---|---|---|
| v52 melee push | rule reads false, but the reused static packet still carries the last hit's force → **intermittent phantom pushes**, which reads as the setting not working |
| v53 crucible | item renumbered, `zone` still gates refining on the old id → **a crucible that links correctly and refines nothing** | item still 2000060, `zone` gates on 147510 → refines nothing, links wrong |

⚠️ Several of tonight's fixes are **code guards that default to the wanted behaviour**, specifically so
they work even where a rule row is missing or a content dump has cleared one. `AoT:NPCsNeverEnrage` is
the example: the stock `NPC:LiveLikeEnrage` rule was already set correctly on test and enrage still
happened on live, because live's DB was behind. Shipping the binary fixes it regardless of DB state.

---

## 1b. ⚠️⚠️ Ship the Lua modules — there is NO version number on this half

Copy these five into live's `quests/` tree. **Nothing on the server reports that live is running an
older copy**, so unlike the migrations there is no query that catches a miss — the only symptom is the
player report coming back after the deploy that was supposed to fix it.

| File | Why |
|---|---|
| `global/global_player.lua` | calls `spell_choice.resend_pending` on connect |
| `lua_modules/spell_choice.lua` | **camping no longer hides an owed reward** |
| `lua_modules/aotv4_dungeon.lua` | chest-failure fallback, warden retune, exit-in-combat refusal, Onslaught band comment |
| `lua_modules/aotv4_dungeon_ldon.lua` | the LDoN half of the ladder |
| `lua_modules/spell_blacklist.lua` | **generated** — regenerated when fear was pruned from the pool |
| `lua_modules/death_loss.lua` | ⚠️⚠️ **the roguelite wipe's keep-list — see below** |

⚠️⚠️ **`death_loss.lua` WAS MISSING FROM THIS LIST AND IT COST PLAYERS THEIR TRADESKILL GEAR.**
Reported from play as the tradeskill achievement gear not surviving death. The guard that spares it is
an id band in `M.is_kept`, and it landed in two steps:

| date | band | covers |
|---|---|---|
| 2026-08-05 | 147930-147953 | head tools + face masks |
| 2026-08-07 | 147930-**147965** | …**and the hand pieces** |

A live server on Lua older than 08-05 destroys **all 36**; between 08-05 and 08-07 it destroys the
**twelve hand pieces** only. Both read in game as "my tradeskill gear vanished when I died".

⚠️⚠️ **THE LOSS IS PERMANENT WITHOUT A DB FIX.** All 36 reward rows carry `claim_once = 1`, so the
achievement will **never** re-grant them — there is no way for a player to re-earn the item by
playing. Shipping the Lua stops the bleeding; it does not give anyone back what they already lost.
Re-queue the reward as pending (the v28 backfill pattern) for anyone affected:

```sql
-- re-arm the claimed reward so it grants again; ClaimPendingRewards sweeps status 0
UPDATE custom_character_achievement_rewards
   SET status = 0, claimed_at = 0
 WHERE reward_type = 'item' AND reward_id BETWEEN 147930 AND 147965
   AND character_id IN ( /* affected characters */ );
```

Then `#ach claim` forces it immediately, or it fires on their next achievement completion.
📌 Scope it to characters who no longer hold the item — re-arming someone who still has theirs would
hand them a second copy.

Three player-reported fixes in this batch are **Lua only** and ship no other way:

- *"camping wipes out spell choices until you level again"* — the offer always survived in its data
  bucket; the **client** was never re-told, so it was unreachable, which is indistinguishable from lost.
- *"boss died with no chest"* — a failed `spawn2` silently cost the augment while the message still
  claimed a chest was there. It now summons the augment to the player and logs the coordinates.
- *"it let me leave the delve during combat"* — Exit was the cheapest escape from a losing fight in the
  game, dodging the roguelite death for rewards that were never banked.

Then, after copying:

```bash
for p in $(pgrep -x zone); do kill -9 $p; done    # eqlaunch respawns them on the new code
```

⚠️⚠️ **`#reloadquest` DOES NOT RELOAD `require`d MODULES** (§10). Every file above except
`global_player.lua` is a `lua_modules/` require, so a reload leaves all of them on the old code while
appearing to succeed. Kill **all** zone processes — killing only the dynamics leaves booted named
zones running the old modules.

⚠️ Syntax-check before restarting anything. A broken module aborts the whole global player script and
the only symptom is `error loading module` in a player's chat:

```bash
find quests -name '*.lua' -print0 | xargs -0 .devcontainer/custom/tools/luacheck
```

---

## 2. Apply the migrations

World applies them at boot. It needs `login.my.cnf` present or the pre-migration backup fails and
**world exits** (§15/§35):

```bash
cd <server>/build/bin
printf '[mysqldump]\nuser=peq\npassword=peqpass\nhost=127.0.0.1\n' > login.my.cnf
chmod 444 login.my.cnf     # read-only, so world cannot overwrite it with blank creds
```

⚠️ This has now bitten twice. World deletes that file after its backup; read-only is the fix.

Then start world once and confirm:

```sql
SELECT custom_version FROM db_version;   -- expect 53
```

---

## 3b. ⚠️⚠️ v50 — the AA overflow. Verify this one BY MEASUREMENT, not by version number

`AoT:HardLevelCap` clamps the **level** to 30, but the experience ceiling was computed from
`Character:MaxExpLevel` (**70**), so a capped character kept banking experience toward the level 71
threshold. The roguelite death pays AA out of total run experience at `AA:ExpPerPoint` (200,000):

| | experience | AA banked on death |
|---|---|---|
| intended — full climb to cap | 464,000 | **2.32** |
| observed in play | ~1,400,000 | **7** |
| engine ceiling allowed | 2,555,000 | **12.7** |

⚠️ The Lua pin in `era_system.clamp_level` never closed this and never could — it is guarded on
`GetLevel() > cap`, and the C++ clamp means the level never exceeds the cap, so the guard is
unreachable. It is still load-bearing for the per-character **region** caps, which sit below this one.

Confirm live is actually clean afterwards — the version number only proves the migration *ran*:

```sql
-- must return 0 rows. 464000 = 1000 * (cap-1) * (cap+2) / 2 at cap 30
SELECT id, name, level, exp FROM character_data
 WHERE level <= 30 AND exp > 464000;
```

⚠️ Characters **above** the cap are deliberately untouched by both the code and the migration: a GM
who has `#level`led up legitimately holds more, and clamping them would make `check_level` resolve
back to 30 and **delevel them** on their next kill.

📌 Already-granted AA points are **not** clawed back — only unconverted experience is trimmed. If
players are to be reset to the intended band that is a separate, deliberate decision.

---

## 3c. v51 — Onslaught's journal said six hours against a thirty-minute clock

Data only, no binary. **The clock was never wrong — only the label.** `M.ONSLAUGHT_SECS` (1800) is
enforced in Lua, so the run really did end at thirty minutes; the player was told six hours and then
failed at thirty, which is worse than either number being wrong on its own.

⚠️ v36 caused it silently: rebuilding all 234 delve tasks as 39 dungeons × 6 modes took
`duration 21600` from the original rows for **every** mode, overwriting the 1800 that Onslaught alone
needs. Nothing errored — 21600 is a perfectly valid duration.

```sql
-- must return 0
SELECT COUNT(*) FROM tasks
 WHERE id BETWEEN 2000300 AND 2000538 AND title LIKE 'Delve:%(Onslaught)' AND duration <> 1800;
```

⚠️ Matched on the **title**, not an id band — v36 already moved the mode offsets once (10 → 40), and an
id-band update would silently hit the wrong mode if they move again.

---

## 3d. v52 — melee push off (needs the binary, see §1)

Reported from play as far too strong: players shoved around constantly in every fight.

```sql
-- must return 'false'
SELECT rule_value FROM rule_values WHERE rule_name = 'Combat:MeleePush';
```

⚠️ Scoped by `rule_name` with **no** ruleset filter — `rule_values` holds multiple rows per rule across
rulesets (§35), so filtering on `ruleset_id` fixes one and silently leaves another.

⚠️⚠️ The compiled default was flipped to `false` as well, but **an existing row overrides the default**
and this database has one. Both halves ship together; and see §1 for why the `zone` binary is the
third necessary piece.

📌 `Spells:NPCSpellPush` is deliberately **not** touched — separate mechanic, not in the report. Note
this database carries it `true` while the stock compiled default is `false`, almost certainly an
artifact of the 0.1.2 rules dump (§35). Worth a decision, separately.

---

## 3e. v53 — the Refining Crucible's chat link (needs the binary AND shared memory)

Reported from play as **"6Refining Crucible"**. The item moves **2000060 → 147510**.

⚠️⚠️ **An item id must stay below `0x100000` (1,048,576) or its link is silently wrong.** RoF2 packs
the id into a five hex digit field and `common/say_link.cpp` masks it (`0x000FFFFF & item_id`), so
2000060 (0x1E84BC — six digits) encoded as **951484**, an id that does not exist. Nothing errors; the
link simply describes another item and the client renders the leftovers. §10 already recorded this
ceiling as the reason the gear tier step is 300,000 and not 1,000,000 — this item was the one
*deliberately exempted* from that renumber, and so the only one left above the ceiling.

```sql
-- expect 0 and 1
SELECT (SELECT COUNT(*) FROM items WHERE id=2000060) AS old_id,
       (SELECT COUNT(*) FROM items WHERE id=147510)  AS new_id;
```

⚠️ All ten reference tables are migrated with it, so a crucible sitting in a bag, shared bank, corpse,
parcel, bandolier, potion belt, shop listing or world container follows the rename rather than
becoming an unresolvable id.

⚠️⚠️ **`items` is shared memory** — this one needs the `./shared_memory` rebuild in §3f to be visible at
all, on top of the migration and the binary. Three pieces.

⚠️⚠️ **Four `custom/` scripts were brought in step with the new id, and two of them were live traps
rather than stale comments.** They are not part of a normal deploy, but they *are* re-run on a rebuild
or a fresh database, and either one would have quietly undone v53:

| Script | What re-running the old version would have done |
|---|---|
| `aotv4_refine_crucible.sql` | recreated the item **at 2000060** and sold that copy at every container merchant — so vendors stock a crucible that cannot refine, while the working one goes unsold |
| `aotv4_artisan_kit.sql` | selected its merchant set with `WHERE item = 2000060`, which now matches **nothing** — the Artisan Kit is placed on **zero** merchants and the script still reports success |
| `aotv4_gear_tiers.sql` | kept a dead `AND id <> 2000060` exemption pointing at an id that no longer exists |
| `gen_delve_augs.pl` | comment only |

📌 Both SQL scripts now accept **either** id, so they also work against a database that has not yet
taken v53. Verified on test: the corrected merchant subquery selects **793** merchants, the old form
selects **0**.

---

## 3f. ⚠️⚠️ v48 IS NOT SUFFICIENT ON ITS OWN — and this section runs LAST

v48 changes **base** item data. The Hallowed and Mythic tiers are **derived**, so they must be
regenerated afterwards or every tier weapon keeps pre-nerf damage while row counts look perfect.

⚠️ Run this **after every migration above**, not before: its `./shared_memory` rebuild is also what
makes v53's renumbered crucible visible. One rebuild at the end covers both.

Run **in this order**:

```bash
mysql peq < .devcontainer/custom/sql/aotv4_gear_tiers.sql      # 1. regenerate both tiers
mysql peq < .devcontainer/custom/sql/aotv4_craft_sockets.sql   # 2. MUST follow — the regen drops tier sockets
mysql peq -e "UPDATE items SET nodrop = 0 WHERE id BETWEEN 147930 AND 147965;"   # 3. tier script sets nodrop across its scope
```

Then, with the stack **down**:

```bash
./shared_memory && <restart>
```

⚠️ `items` and `spells_new` are shared memory. Migrations alone change nothing a player can see.

---

## 4. Verify — BY RATIO, NEVER BY ROW COUNT

§35 records 3,166 Mythics once having **less AC than their own base** while every count looked healthy.

```sql
-- all four must return 0
SELECT COUNT(*) FROM items m JOIN items b ON b.id=m.id-600000
 WHERE m.id BETWEEN 600000 AND 899999 AND b.damage>0 AND m.damage <> b.damage*2;
SELECT COUNT(*) FROM items h JOIN items b ON b.id=h.id-300000
 WHERE h.id BETWEEN 300000 AND 599999 AND b.damage>0 AND h.damage <> FLOOR(b.damage*1.5);
SELECT COUNT(*) FROM items m JOIN items b ON b.id=m.id-600000
 WHERE m.id BETWEEN 600000 AND 899999 AND b.ac>0 AND m.ac <> b.ac*2;
-- the 2026-08-05 bug: both tiers computing damage*2, so they came out identical
SELECT COUNT(*) FROM items m JOIN items h ON h.id=m.id-300000
 WHERE m.id BETWEEN 600000 AND 899999 AND m.damage>0 AND m.damage = h.damage;
```

Spot check: **item 25615 Frozen Two Handed Sword** — base **33**, Hallowed **49**, Mythic **66**.

---

## 5. Client files — four of tonight's changes are NOT server-side

| File | Why |
|---|---|
| `EQUI_AoTSpellChoiceWnd.xml` | regenerated after fear was pruned from the reward pool |
| `EQUI_AoTAllacloneWnd.xml` | the new **Zone XP** browse tab |
| `spells_us.txt` | v42 opened 74 outdoor-only spells; the client keeps its own `zonetype` copy and will refuse the cast without this |
| `dbstr_us.txt` | Bulwark Within's corrected description |

⚠️ Run `./export_client_files` **after** the migrations, then ship. Without `spells_us.txt` those 74
spells look fixed on the server and still refuse in game — the three-layer problem from §4 and §39.

---

## 6. ⚠️ TEST AS A NON-GM

The GM flag silently bypassed **four** separate gates during this work — the delve expansion check, the
region lock, trap triggering, and spell fizzling. Every one of them looked fine to a GM and was broken
for players. Use a normal character for:

- entering an even-numbered delve rung (the LDoN half)
- selling in a home town as a race that could not before
- an Alchemy combine on a non-Shaman, and training Tinkering on a non-Gnome at a GM trainer
- a delve boss step — trash kills must NOT close it

Then the four from this batch, which need play rather than a query:

| Check | Expected |
|---|---|
| link the Refining Crucible in chat, and combine 4 identical items in it | clean name, **and** it still refines — the two halves of v53 |
| melee a mob, and be meleed | no shoving, in **either** direction, on every hit |
| open an Onslaught delve and read the journal | **30 minutes**, matching the clock that actually fails you |
| take a level reward offer, camp, log back in, Ctrl+Q | the picker opens with the offer still there |
| `/say delveexit` with something on you | refused — *"You cannot leave the delve while something is fighting you."* |
| kill a solo delve warden | shorter fight, softer hits — HP ×5.0, melee ×1.25, spellscale 30 |

⚠️ The melee-push check must include **being hit**, not just hitting: the stale-force bug lived in the
damage packet, so it fired on incoming hits too and a one-sided test can miss it.

---

## 6a. ⚠️⚠️ Do NOT port AoT v1's `AA_update` timer — the root cause is already fixed in C++

v1's `global_player.lua` ran `eq.set_timer('AA_update', 500)` and refreshed the client with
`if (e.self:GetLevel() < 51) then e.self:SetAAPoints(e.self:GetAAPoints()) end`. That is a **workaround
for stock behaviour AoTv4 no longer has**, and re-adding it would be a 500 ms per-player poll for
nothing.

Stock `zone/exp.cpp` hardcodes level **51** in two places. Below it, a character takes this branch:

```cpp
if (GetLevel() < 51) { m_epp.perAA = 0; }   // AA exp forcibly zeroed
else                 { SendAlternateAdvancementStats(); }   // ...and the UI update is SKIPPED
```

So on a sub-51 server it fails **twice** — AA experience is unearnable *and* the client is never told.
v1 could only paper over the second half, which is why it needed the timer.

AoTv4 fixed the cause: both sites now read **`AoT:AAExpMinLevel`** (default **1**, and set to 1 in the
DB), so every character takes the `else` branch and
`SendAlternateAdvancementStats()` fires on **every** `SetEXP`.

📌 `Client::SetAAPoints` (`zone/client.cpp:12290`) does nothing else — its last statement *is*
`SendAlternateAdvancementStats()`. The Lua workaround and our native path call the identical function;
ours fires on the actual experience change instead of polling.

⚠️ Separately, the **death-banked** AA points are a private pool (`aa_bank_<charid>`) spent through the
custom picker, and are *deliberately* invisible to the native AA window. That is by design.

### ⚠️⚠️ …but NONE of that is why the AA experience bar is dead. `AddEXP` zeroes `perAA` outright.

`Client::AddEXP` (`zone/exp.cpp:515`) opens with an **unconditional** stomp, before any rule, gate or
calculation is consulted:

```cpp
// Carolus: AoT disable AA standard exp gain
m_epp.perAA = 0;
```

`CalculateExp` then does `add_aaxp = add_exp * m_epp.perAA / 100` (`:429`) — so **`aaexp` is always 0
and no character can ever earn AA experience**, whatever `AA:ExpPerPoint`, `AoT:AAExpMinLevel` or the
client is set to. Confirmed in the data: **every character on test has `e_percent_to_aa = 0` and
`aa_exp = 0`**, because the value is re-zeroed on every kill and then saved.

📌 It also explains why `/alt exp <n>` appears to do nothing. The server *accepts* the setting
(`aaActionSetEXP`, no level gate) — and the very next kill wipes it.

⚠️⚠️ **This is deliberate, not a bug** — added by carolus21rex on 2026-08-02 (`7a552a9f4`), and it is
consistent with the design: AA is meant to come from the death-banked pool, so the native earn path was
switched off. **Re-enabling it is a DESIGN decision, not a fix**, because it gives players a second AA
source alongside the picker — precisely what the private pool exists to prevent.

📌 The bar therefore needs **no number changes** to work; it needs that line removed or put behind a
rule. `AA:ExpPerPoint` only becomes relevant *afterwards*, since the bar fill is
`330 * expAA / ExpPerPoint` (`zone/aa.cpp:992`) — at the stock 23,976,503 it would advance ~120x too
slowly to see, while test already carries 200,000.

---

## 7. ⚠️ Do NOT run `aot_npcs_items_1.0.0.sql` anywhere

It carries `USE `peq`;` on line 20, which **overrides the database given on the command line**. Loading
it into a scratch schema applies it to the real one — that is how it damaged the test server.

Its entire useful payload is already captured in **v48**. Everything else it contains is a fortnight
stale and reverts craft sockets, the tradeskill gear rework and the gear-tier edits.

If it must ever be inspected, strip the header first and **verify the strip before loading**:

```bash
sed '/^USE `peq`;/d; /^CREATE DATABASE/d' file.sql > safe.sql
grep -cE "^USE |^CREATE DATABASE" safe.sql    # MUST print 0
```
