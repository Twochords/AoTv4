# Deploying to live

`/src` is the test environment; live is a **different machine with a different database** (CLAUDE.md
§25). A commit carries **source**, never binaries and never the database — so a pull alone changes
nothing on live.

Since 2026-08-20 the Lua is tracked in this repo, so one pull carries the C++, the migrations **and**
the quests. Only `eq-core-dll` is still separate, and that is a *client* artifact (`dinput8.dll`)
shipped to players, not deployed to a server.

## The order, and why

```bash
# 1. stop everything.  Zones and eqlaunch first, world last.
cd <live>/build/bin
for p in $(pgrep -x zone); do kill -9 $p; done
pkill -x eqlaunch; pkill -x world

# 2. source
cd <live> && git pull

# 3. rebuild.  BOTH -- the migrations live in a HEADER compiled into world, and
#    CUSTOM_BINARY_DATABASE_VERSION is compiled into zone.
cd build && ninja world zone

# 4. world FIRST, alone.  It applies the migrations and advances db_version.
cd bin && setsid nohup ./world > logs/world_manual.log 2>&1 < /dev/null &
grep -E 'UpdateManifest|setting database version' logs/world_manual.log

# 5. ONLY IF a migration touched `items` or `spells_new` -- with world DOWN again:
#    stop world, ./shared_memory, restart world.

# 6. zones
setsid nohup ./eqlaunch zone > logs/eqlaunch_manual.log 2>&1 < /dev/null &
```

⚠️⚠️ **Step 4 before step 6, always.** A zone binary whose `CUSTOM_BINARY_DATABASE_VERSION` is ahead
of `db_version.custom_version` logs `Exiting due to pending database updates` and quits — by design.
Every zone boots, refuses and exits, so none binds a port, world answers *"No zoneserver available to
boot up"*, and the player sees **"server is down"** while `eqlaunch` looks entirely innocent (§2).

⚠️ Keep **exactly one** `eqlaunch`. Restarting it while old zones live orphans them and clients time
out on login (§10).

⚠️ `pgrep -cx zone` counts zombies and lies. Use
`ps -eo pid,stat,comm | awk '$3=="zone" && $2 !~ /Z/'`.

## What a pull does NOT carry

| | how it reaches live / players |
|---|---|
| `world` / `zone` binaries | rebuilt on live (step 3) |
| the database | migrations apply themselves at world boot (step 4) |
| shared memory blobs | `./shared_memory` with world down (step 5) |
| `dinput8.dll` | built on Windows, shipped to **players** |
| `dbstr_us.txt`, `spells_us.txt`, `SkillCaps.txt`, `EQUI_*.xml` | `./export_client_files`, shipped to **players** |

⚠️⚠️ **A migration cannot reach the client.** AA and spell names and descriptions are resolved by the
client from its own `dbstr_us.txt` (§6), so a `db_str` migration changes nothing anyone can see until
that file is exported and shipped. Same for skill caps and spell levels.

## Class abilities (v104-v121) -- existing characters are backfilled on login

⚠️⚠️ **A CHARACTER ALREADY PAST THE LEVEL GETS EVERYTHING IT HAS EARNED, ON ITS NEXT LOGIN, WITH NO
MIGRATION AND NO GM COMMAND.** `aotv4_class_abilities.grant` walks all three tiers and trains every
one whose level requirement is already met -- so a level 25 Warrior who has never seen these logs in
and receives all three at once. A deploy is a server restart, so **every** player reconnects and
every character is covered; nobody has to be found and fixed by hand.
- It is called from `global_player` on **connect**, on **level up**, and on **death** (the roguelite
  wipe untrains disciplines -- see section 30).
- ⚠️ `HasDisciplineLearned` is what makes it safe to run on every login: `TrainDiscBySpellID`
  validates **nothing** and would happily list the same ability twice.
- 📌 Unlike the tradeskill floor -- which ran from EVENT_CONNECT *after* the player profile had
  already gone out, so a new character saw 0 tradeskills until they relogged (section 7) -- this
  updates the client immediately: `SendDisciplineUpdate` sends the whole discipline array as its own
  `OP_DisciplineUpdate` packet and does not depend on the profile.
- 📌 `MAX_PP_DISCIPLINES` is 100, so there is no slot pressure: these are three of them, and the
  reward picker never hands out stock disciplines (section 20).
- 📌 After a death the character is level 1, so only tier 1 returns immediately; tiers 2 and 3 come
  back on dinging 5 and 10. That is the intended roguelite behaviour, not a backfill failure.

⚠️⚠️ **STEP 5 IS NOT OPTIONAL FOR THIS FEATURE, AND GETTING THE ORDER WRONG IS SILENT.** These are
rows in `spells_new`, which is shared memory. If a zone boots before `./shared_memory` has run,
`IsValidSpell(44700)` is false in that zone -- and because `TrainDiscBySpellID` validates nothing, it
will still write 44700 into the character's discipline array for an ability that zone does not know.
The login message reads with an empty name (`eq.get_spell_name` has nothing to resolve) and the
ability cannot be used. It self-heals once shared memory is correct, because the id was stored, but
**do step 5 before step 6.**

⚠️ **EXACTLY TWO CLIENT FILES CHANGE for this feature: `spells_us.txt` and `dbstr_us.txt`.**
`SkillCaps.txt` and `BaseData.txt` come out of `./export_client_files` byte-identical to what is
already shipped, and there is **no `EQUI_*.xml` and no dll change** -- these use the stock Combat
Abilities window, so nothing new has to be drawn. Both updated files are staged in
`aotv4_client_install/`; the previous pair is kept beside them as `*.2026-08-19.snapshot.txt`.
📌 **Keep doing that.** Section 35 records `aotv4_client_install/spells_us.txt` being an accidental
pre-change snapshot that was the ONLY way to recover six spells' native buff durations, twice. It is
only a safe overwrite when the diff is additions; check before replacing it.
📌 Diff the export against the shipped bundle rather than copying blind -- it is how the Paladin
finding below was noticed.

⚠️⚠️ **AND THE CLIENT MUST GET `spells_us.txt`, OR THE ABILITY IS TRAINED AND INVISIBLE.** The
Combat Abilities window draws each entry by looking the id up in the client's own file. Without the
export shipped, the server believes the player has the ability, the player sees an empty window, and
nothing anywhere reports a problem. This is the one part of this feature a server deploy cannot
deliver on its own.

### 📌 The shipped bundle was stale by more than this feature
Diffing the fresh export against `aotv4_client_install/` showed **51 added lines and 3 CHANGED ones**
in `spells_us.txt`. The three are **44600-44602, the Paladin abilities**: `CastingAnim` 44 -> 0,
`TargetAnim` 13 -> 0, `spellanim` 103 -> 0 -- the same "stop looking like a spell" fix, made in
**migration v94**, which had never reached a client because the bundle predates it. So shipping these
two files also repairs the Paladin abilities' presentation.
⚠️ The lesson is the diff, not the fix: **a client file can be stale by an arbitrary number of past
migrations**, and nothing anywhere reports it. Compare field by field when lines change rather than
just counting them.

## Verifying

```sql
SELECT custom_version FROM db_version;          -- must equal CUSTOM_BINARY_DATABASE_VERSION
```
In the world log, `[missing]` is a migration that ran; `[ok]` is one whose condition was already
satisfied and correctly did nothing.

For the class abilities specifically, after one player has logged in:
```sql
-- 51 rows, every one flagged as a discipline and carrying a description
SELECT COUNT(*) FROM spells_new WHERE id BETWEEN 44700 AND 44755 AND IsDiscipline = -1;   -- 51
SELECT COUNT(*) FROM db_str WHERE id BETWEEN 44700 AND 44755 AND type = 6;                -- 51

-- who has been backfilled, and with what
SELECT cd.id, cd.slot_id, cd.disc_id, s.name
FROM character_disciplines cd JOIN spells_new s ON s.id = cd.disc_id
WHERE cd.disc_id BETWEEN 44700 AND 44747;
```
⚠️ A character whose row is missing is one that has not logged in since the deploy -- that is the
expected state, not a fault. There is nothing to re-run.
