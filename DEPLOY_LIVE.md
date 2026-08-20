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

## Verifying

```sql
SELECT custom_version FROM db_version;          -- must equal CUSTOM_BINARY_DATABASE_VERSION
```
In the world log, `[missing]` is a migration that ran; `[ok]` is one whose condition was already
satisfied and correctly did nothing.
