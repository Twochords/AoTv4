# QUEUED: apply carolus21rex/aotv4_scaling (item + NPC scaling) at the next pull-down

Owner decision 2026-08-13: **do all of 1-4, but NOT a standalone restart** — bundle it into the next
pull-down/deploy (which restarts anyway). Verified safe on a scratch copy (`peq_scaletest`); results below.

## What it does (measured on scratch vs live)
- `item_scaling_main.py` — recomputes stats for **7,842** equippable items that have a drop/recipe/vendor
  source (weapons hit harder + gain reclevel, armor AC/reclevel up). **Replaces the dev's v48 rebalance
  for those items.** Also wipes classes/races/reqlevel to all — but only **38** rows differ (live is
  already all-class).
- `npc_level_adjust_main.py` — remaps NPC **levels** per `zone_scaling` target bands (**89** changed;
  e.g. zone-88 spiders 31→14).
- `npc_scaling_main.py` — rescales NPC hp/ac/dmg/resists/stats (**9,301**) + exp_mod 100/400/900 (**4,616**).
- ✅ **Custom items are SAFE**: 0 of our 362 (delve augs, tools, sigil) change — no acquisition source → skipped.
- ⚠️ **Breaks gear tiers**: 5,194 Hallowed/Mythic pairs go ≠2× base → tier regen is MANDATORY after.

## Prerequisites (must be true before running)
1. **The 4 scaling tables must exist on live**: `npc_race_stats`, `npc_class_scale`, `npc_raid_scale`,
   `zone_scaling`. They are **MISSING on live** right now — import them (from the Aug-9 dump / `peq_import3`)
   as part of the pull-down, or the NPC scripts fail.
2. Repoint `aotv4_scaling-main/database/config.py` to **live** for the real run: currently it points at
   scratch (`host=mariadb, user=eqemu, db=peq_scaletest`) — change `database` to **`peq`** (keep host/creds).
   (Its shipped default `127.0.0.1:3307 peq/peqpass` does NOT work on this box.)
3. Runner: no pip on host/containers — run in a throwaway container:
   `docker run --rm --network akk-stack_backend -v /home/eqemu/aotv4_scaling/aotv4_scaling-main:/app -w /app python:3.11-slim sh -c "pip install -q mysql-connector-python && python npc_level_adjust_main.py && python npc_scaling_main.py && python item_scaling_main.py"`

## Apply order (at the next pull-down, server going down anyway)
1. **Backup** the DB first (mysqldump) — the scripts commit as they go, no transaction, not reversible.
2. Do the normal pull-down (git pull, migrations) FIRST, then the scaling — so a later item migration
   doesn't clobber the scaling, and the scaling runs on the final base data.
3. **Import the 4 scaling tables** (prereq 1).
4. **Run the 3 scripts** against live (repointed config) — `npc_level_adjust` → `npc_scaling` → `item_scaling`.
5. **Regenerate tiers**: `aotv4_gear_tiers.sql` → `aotv4_craft_sockets.sql` → nodrop fix (§35 dance) —
   because step 4 rewrote base item stats.
6. **`./shared_memory`** (items/spells are shared memory) with world down.
7. **Restart** + verify: tier ratios (Mythic=2×base, all 4 checks 0), spot-check a rescaled weapon/npc,
   confirm custom items untouched, and re-run `LIVE_ONLY_FIXES.md` checks.

## Open review items for the owner before committing
- Confirm the item quality model is the desired balance (it re-bases everything acquisition-difficulty-first).
- Eyeball `zone_scaling` target ranges — some zones shift a lot (31→14).
- Decide whether this + v48 coexist or v48 is superseded for the 7,842 items.

Scratch schema `peq_scaletest` retained for further before/after review; drop it when done.

---

# ALSO QUEUED for the next restart: open zone ports up to 7300

Owner ask 2026-08-13. Current: ports **7000-7049** published, `ZONE_POOL=50`. Target: publish **7000-7300**.

⚠️ **This is a CONTAINER RECREATE, not just a process restart** — published ports are fixed at container
create time, so `stop-server.sh`/`start-server.sh` alone won't do it; the eqemu-server container must be
recreated (`docker compose up -d eqemu-server`). ✅ **DB is safe**: mariadb is a SEPARATE container
(`akk-stack-mariadb-1`), untouched by recreating eqemu-server.

Changes:
1. `/home/eqemu/akk-stack/.env` line 83: `PORT_RANGE_HIGH=7049` → **`7300`** (compose publishes
   `${PORT_RANGE_LOW}-${PORT_RANGE_HIGH}/udp`, so this widens the published range).
2. `/home/eqemu/akk-stack/server/startup/start-server.sh` line 26: `ZONE_POOL=50` → raise to use the new
   range. ⚠️ **DECISION NEEDED**: the static pool launches one `./bin/zone` PROCESS per slot, so
   `ZONE_POOL=300` = 300 zone processes (~heavy RAM). Decide the real number wanted vs just port headroom;
   `ZONE_POOL` and the published range should match (§0/§27 rule).
3. Recreate: `cd /home/eqemu/akk-stack && docker compose up -d eqemu-server` (entrypoint reruns
   start-server.sh → boots the larger pool). Verify: `docker port akk-stack-eqemu-server-1 | grep -c 70` and
   `zonestatus` shows the new available count.

📌 Why: 50 was starving the delve instance pool (see [[delve-zone-pool-shutdowndelay]]); more ports = more
concurrent zones/instances (and enables §27 sharding if ever turned on).
