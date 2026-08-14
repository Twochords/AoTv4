# LIVE-ONLY FIXES — the registry that stops us reverting our own work

Fixes applied **directly to the live akk-stack server** that are **not (or not reliably) in the dev
fork's git history**. Every fork→live sync, DB import, or `git pull`+rebuild risks **reverting** these.
The 2026-08-10 delve regression happened because six of these lived only on live and a blind quest
sync overwrote them with older fork versions.

## THE RULE
1. **Before any fork→live quest sync:** run `.devcontainer/custom/tools/quest_sync_guard.sh <fork> <live>`.
   It BLOCKS when live is larger / has newer date markers than the fork (= a live-only fix about to be
   reverted). Review each ⚠️ file; only sync files that are genuinely fork-ahead.
2. **Before any DB import:** re-apply the SQL fixes below afterward (they touch tables we skip on import:
   `zone`, `zone_regions`, `aotv4_zone_xp`), and verify with the check column.
3. **The real fix is upstreaming.** Every row marked `upstreamed: NO` must be committed into the fork by
   the dev and VERIFIED (`git diff fork live` empty for that file) — until then it stays at risk.

## Registry

| # | Fix | Where (file / table) | Artifact | Upstreamed? |
|---|---|---|---|---|
| 1 | Delve zones close 5 min after empty (was 1h) — pool starvation | `zone.shutdowndelay` (39 delve zones) | `custom/sql/aotv4_delve_zone_shutdown.sql` | **NO** |
| 2 | LDoN delve zones expansion=0 (enterable) | `zone.expansion` | `custom/sql/aotv4_ldon_delve_expansion.sql` | **NO** |
| 3 | Kaesora reachable (region 99→6) + Swamp of No Hope region 6 + Kaesora zone-xp 11-23 | `zone_regions`, `aotv4_zone_xp` | `custom/sql/aotv4_kaesora_region_fix.sql` | **NO** |
| 3b | hateplaneb / kaesora / sleeper ungated: `min_level=0` + `expansion=0` (all in zone_regions) | `zone.min_level`, `zone.expansion` | `custom/sql/aotv4_unlock_hate_kaesora_sleeper.sql` | **NO** |
| 4 | Delve 2026-08-06 fixes: `MAX_BUMP_FRAC=0.50` scaling clamp, Onslaught `taskoff` 10→40 offsets, `BOSS_HP_MULT` (dev's current value **5.0**) | `lua_modules/aotv4_dungeon.lua`, `aotv4_dungeon_scale.lua` | **UPSTREAMED** — repo `38b411636` has all three; `live==repo` verified in the 2026-08-13 v58 deploy | **YES** ✅ |
| 5 | Live-AA transition Lua (v54): gate on-death lump on `AoT:LiveAAExp`, `event_aa_gain`→`on_points_banked` (gated on `AAPointsToPicker`), `reoffer_if_banked`+`resend_pending` on connect | `global/global_player.lua` | **UPSTREAMED** — dev folded in a better version; `live==repo` verified 2026-08-13 v58 | **YES** ✅ |
| 6 | Delve voluntary exit refused in combat (`delveexit` checks `GetAggroCount`) | `lua_modules/aotv4_dungeon.lua` | **UPSTREAMED** — repo has it with an expanded comment; `live==repo` verified 2026-08-13 v58 | **YES** ✅ |
| 8 | SKILLUNLOCKDATA sent on LEVEL-UP (Backstab/Monk-strikes usable at unlock level, no relog) — `send_unlocks` in `event_level_up` | `global/global_player.lua` | **UPSTREAMED** — repo has it; `live==repo` verified 2026-08-13 v58 | **YES** ✅ |
| 7 | Resplendent hub NPC lock (class/bodytype/heading auto-revert triggers) | DB triggers `aotv4_lock_resplendent_*` | `custom/sql/aotv4_resplendent_npc_lock.sql` | re-run after any full DB import |

## Post-sync / post-import smoke test (run as a NON-GM)
- **Delve scaling:** enter a rung-N delve; mobs should be ~N with the bump capped at `N*0.50` (rung 16 → ≤24, NOT 29).
- **Delve affix:** the journal mode (Standard/Hard/Gauntlet/Onslaught…) must match what you selected.
- **Warder:** the delve boss spawns.
- **AA:** kill mobs → picker pops as points bank; die → NO AA lump; relog with banked points → picker re-offers.

## ⚠️⚠️ akk-stack deploy trap: `eqemu_config.json` `world.locked` MUST be a JSON **bool**, not a string
Discovered in the 2026-08-13 v58 deploy. The akk-stack container entrypoint is **Spire** (a Go
program). On container **recreate** it parses `eqemu_config.json`, and `server.world.locked` is a
`bool` field — a string value (`"true"` OR `"false"`) makes Spire loop forever on
`json: cannot unmarshal string into Go struct field .server.world.locked of type bool`, so it never
launches the server and the container is stuck (world/zone/ucs all 0, only `spire` running).
- The stock EQEmu C++ world reads it via `.get("locked","false").asString() == "true"`, which is why
  CLAUDE.md's locked-deploy note says to use the **string** `"true"`. That is correct **only** for the
  manual `./bin/world` boot path — it is WRONG for an akk-stack container recreate.
- The previous `"locked": "false"` (string) was a latent bug: it "worked" only because no recreate had
  happened since it was added. Any recreate would have broken identically.
- **Correct akk-stack locked-deploy flow** (used successfully in the v58 deploy):
  1. Keep `world.locked` as a real bool. Steady state = `false`.
  2. Container recreate (`docker compose up -d eqemu-server`) only restarts **Spire** + republishes
     ports — it does **NOT** boot the game server.
  3. Boot the game server manually: `docker exec … start-server.sh` (reads `ZONE_POOL`, new ports).
  4. Lock the running world over telnet 9000 (`lock`), do the work, `unlock` at the end.
  - jsoncpp `asString()` on a bool returns `"true"`/`"false"`, so a bool value still satisfies the C++
    read too — but during a recreate deploy, lock/unlock via **telnet**, not the config.
- Telnet 9000 from inside the container is unauthenticated for localhost ("assuming admin"). Useful
  commands: `broadcast <msg>`, `wwmarquee <type> <msg>` (on-screen banner), `lock`/`unlock`,
  `zonestatus`, `who`, `quit`.

## 2026-08-13 v58 deploy — outcome
Migrations 55–58 applied (custom_version 54→58); items+spells shared_memory rebuilt; `repo/quests`
(38b411636) synced wholesale to live (dev had upstreamed #4/5/6/8 — both guard ⚠️ were false
positives); client files exported. Ports widened to **7000-7300** (302 published, 149 zones on
ports >7049) and **ZONE_POOL 50→150** (all 150 zones stable). Gear/monster rescale (SCALING_APPLY_PLAN)
was **skipped** by owner decision. DB-side live-only fixes (#1/2/3/3b/7) verified intact post-deploy.
