# POST-REBUILD RECOVERY RUNBOOK

**Written:** 2026-07-11. **Updated 2026-07-21** — the two things that made a rebuild painful are now
fixed in `devcontainer.json`, so a normal rebuild should "just work":
- **`appPort` now includes `"5999:5999/udp"`** (the RoF2/SoD login port). Without it the client
  **times out at the username/password screen** — see CLAUDE.md §2 "RoF2 login needs 5999/udp".
- **The MariaDB datadir is now a persistent named volume** (`aotv4-mysql-data` →
  `/var/lib/mysql`), so a rebuild **no longer wipes the `peq` DB**. Steps 2-4 below are only needed
  the **first** time (when the named volume is created empty) or if the volume is ever reset.

**Why this file still exists:** the named volume is created **empty** the first time it's mounted;
until it's seeded once, `peq` is absent. And if the volume is deleted (or you're standing up a new
machine), you re-seed from the snapshot. Steps below restore `peq` from scratch. After a *routine*
rebuild with the volume intact, skip to **step 5** (shared memory) → **step 6** (start stack).

---

## What SURVIVES the rebuild (all on the `/src` = `C:\AoTv3\AoTv4` 9p mount)
- All source + the **built `zone` binary** (`/src/build/bin/zone`, built 06:05 **with the backstab
  rework** — do NOT rebuild it unless it fails to launch).
- Config symlinks `build/bin/login.json` + `eqemu_config.json` → `.devcontainer/override/` (already
  correct: `sod_port=5999`).
- `devcontainer.json` (publishes `5999:5999/udp`; mounts the `aotv4-mysql-data` DB volume).
- **The `peq` database itself** — now on the persistent named volume, so it survives a rebuild.
- The DB snapshot **`/src/aotv4_current.sql`** (~404 MB — full `peq` dump *with all migrations
  already applied*: gear tiers 27k Hallowed + 27k Mythic, spelldmg rule, 7 characters incl. Ashrem,
  login accounts). Self-creates the DB (`CREATE DATABASE IF NOT EXISTS peq`). **Only needed to seed a
  fresh/empty volume.** ⚠️ `/src/Deez.sql` has been BOTH the pre-migration dump and (later) a full
  copy — do not trust its name; **always seed from `aotv4_current.sql`** and verify with step 4.
- `custom/sql/*` migrations (do NOT need to re-run — they're baked into `aotv4_current.sql`).

## What is GONE and must be rebuilt (routine rebuild, volume intact)
- **Nothing DB-side** — the `peq` database + `peq@127.0.0.1` user persist on the named volume.
  (Only when the volume is first created / reset do you run steps 2-3 to seed + grant.)
- Shared memory segment (rebuilt in step 5 — it lives in the container overlay, so always rebuild).
- All running server processes (started in step 6).

---

## FAST PATH — one command
After a rebuild, just run the recovery script (idempotent: seeds the DB only if the volume is empty,
then rebuilds shared memory + starts the stack + prints a health check):
```bash
bash /src/.devcontainer/recover.sh
```
The exact manual steps it automates are below (use them to debug if the script reports a failure).

---

## EXACT STEPS (run from the dev container after rebuild)

### 1. Start MariaDB
```bash
sudo service mariadb start
sleep 3
(ss -ltn 2>/dev/null || netstat -ltn) | grep 3306 && echo "3306 UP" || echo "MariaDB DOWN"
```

### 2. Import the snapshot (self-creates the `peq` database)
```bash
time sudo mariadb < /src/aotv4_current.sql          # ~1 min
```

### 3. Recreate the `peq` user + grants (the rebuild dropped it)
```bash
sudo mariadb -e "CREATE USER IF NOT EXISTS 'peq'@'127.0.0.1' IDENTIFIED BY 'peqpass';
                 GRANT ALL PRIVILEGES ON *.* TO 'peq'@'127.0.0.1'; FLUSH PRIVILEGES;"
```

### 4. Verify the DB over TCP as `peq` (expect 4 characters)
```bash
mysql -h127.0.0.1 -upeq -ppeqpass peq -N -e "
  SELECT 'chars', COUNT(*) FROM character_data
  UNION ALL SELECT 'buckets', COUNT(*) FROM data_buckets
  UNION ALL SELECT 'hallowed', COUNT(*) FROM items WHERE id BETWEEN 300000 AND 599999
  UNION ALL SELECT 'mythic',   COUNT(*) FROM items WHERE id BETWEEN 600000 AND 899999
  UNION ALL SELECT 'spelldmg_rule', rule_value FROM rule_values WHERE rule_name='Spells:IgnoreSpellDmgLvlRestriction';"
# expect: chars=4, hallowed=15621, mythic=15621, spelldmg_rule=true
```
If those look right, **the migrations are already in** — do NOT re-run `custom/sql/*`.

### 5. Rebuild shared memory (world must be DOWN — it is, nothing started yet)
```bash
cd /src/build/bin && ./shared_memory 2>&1 | tail -3
```

### 6. Start the full stack (detached so a shell timeout can't kill them)
```bash
cd /src/build/bin
setsid nohup ./loginserver > logs/loginserver_manual.log 2>&1 < /dev/null &
sleep 2
setsid nohup ./world       > logs/world_manual.log       2>&1 < /dev/null &
sleep 5
setsid nohup ./eqlaunch zone > logs/eqlaunch_manual.log  2>&1 < /dev/null &
setsid nohup ./ucs         > logs/ucs_manual.log         2>&1 < /dev/null &
setsid nohup ./queryserv   > logs/queryserv_manual.log   2>&1 < /dev/null &
sleep 5
for p in loginserver world eqlaunch ucs queryserv; do echo "$p: $(pgrep -x $p | wc -l)"; done
```

### 7. Verify health — THE WHOLE POINT: loginserver must bind **5999**
```bash
(ss -lunp 2>/dev/null || netstat -lunp) | grep -E ':5998|:5999'    # both must be bound by loginserver
grep -E 'Connected to database|successfully authenticated' \
  /src/build/bin/logs/world_manual.log /src/build/bin/logs/loginserver_manual.log | tail -3
grep -iE 'fatal|malloc|Cannot continue' /src/build/bin/logs/world_manual.log && echo "WORLD CRASH" || echo "world ok"
```

### 8. Client
- `eqhost.txt` stays: `[LoginServer]` / `Host=127.0.0.1:5999`  (NO change from before).
- Log in with account **mikethefiend** (or mikethefiend1). Characters: Ashrem (GM), Cexoos,
  Apheyawus, Eqpoqapon.
- **Run the client WINDOWED** (the `dinput8.dll` overlay needs it).

---

## Gotchas / fallbacks
- **`zone` won't launch** (missing lib after image change): `cd /src/build && ninja zone`, then redo step 5+.
- **Snapshot import errors**: fall back to `sudo mariadb < /src/Deez.sql` (Jul 10), then re-apply the
  two migrations, then rebuild shared memory:
  ```bash
  sudo mariadb --database peq < /src/.devcontainer/custom/sql/aotv4_item_spelldmg_healamt.sql
  sudo mariadb --database peq < /src/.devcontainer/custom/sql/aotv4_gear_tiers.sql
  ```
- **Client still can't reach login after all this**: confirm `5999:5999/udp` is in `devcontainer.json`
  `appPort` AND that the rebuild actually picked it up (this whole rebuild was to publish it). See
  CLAUDE.md §2 "RoF2 login needs 5999/udp PUBLISHED".
- **Keep exactly one `eqlaunch`.** If clients time out entering world, check for rogue bare `./zone`
  procs + a single `eqlaunch` (CLAUDE.md §10).
- Once login is confirmed working, this file + `/src/aotv4_current.sql` can be deleted (or kept as a
  recovery point). Consider the persistent-`/var/lib/mysql`-volume follow-up so this never recurs.
