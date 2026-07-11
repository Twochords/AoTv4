# POST-REBUILD RECOVERY RUNBOOK

**Written:** 2026-07-11, right before a deliberate `Dev Containers: Rebuild Container` to publish
UDP **5999** (RoF2 login port) via `devcontainer.json` `appPort`.

**Why this file exists:** rebuilding the dev container **wipes `/var/lib/mysql`** (it lives on the
container's ephemeral overlay, NOT a mount). Everything below restores the server to exactly the
state it was in before the rebuild. Follow it top-to-bottom after the container comes back.

---

## What SURVIVES the rebuild (all on the `/src` = `C:\AoTv3\AoTv4` 9p mount)
- All source + the **built `zone` binary** (`/src/build/bin/zone`, built 06:05 **with the backstab
  rework** — do NOT rebuild it unless it fails to launch).
- Config symlinks `build/bin/login.json` + `eqemu_config.json` → `.devcontainer/override/` (already
  correct: `sod_port=5999`).
- `devcontainer.json` (now publishes `5999:5999/udp`).
- The DB snapshot **`/src/aotv4_current.sql`** (305 MB — full `peq` dump *with today's migrations
  already applied*: gear tiers, spelldmg rule, 4 characters, login accounts). Self-creates the DB.
- Fallback dump **`/src/Deez.sql`** (Jul 10, HeidiSQL, pre-migration — only use if the snapshot fails).
- `custom/sql/*` migrations (do NOT need to re-run — they're baked into `aotv4_current.sql`).

## What is GONE and must be rebuilt
- The MariaDB datadir: `peq` database + the `peq@127.0.0.1` user (recreated in steps 2–3).
- Shared memory segment (rebuilt in step 5).
- All running server processes (started in step 6).

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
