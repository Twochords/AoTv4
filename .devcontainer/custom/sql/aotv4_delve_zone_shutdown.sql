-- AoTv4: delve zones close 5 minutes after they go empty (was 1 hour).
--
-- WHY: the delve (§24) creates a fresh instance per run, and the 6 DoN + 33 LDoN delve zones shipped
-- with shutdowndelay = 3600000 (1 hour). An empty delve instance therefore squatted on one of the
-- static 50-zone pool slots (start-server.sh ZONE_POOL) for a full hour before Zone::autoshutdown_timer
-- fired and closed the process -- so a busy delve session exhausted the pool and NEW delve joins failed
-- with "That zone is unavailable" (nothing left to boot the instance into). Reported 2026-08-08:
-- couldn't join a level-2 delve; zonestatus showed 40 booted / 1 free, ~32 of them empty delve zones.
--
-- The autoshutdown timer's period IS the per-zone shutdowndelay column (falling back to the 5s
-- Zone:AutoShutdownDelay rule); it closes the zone PROCESS when the zone is empty (numclients == 0).
-- Dropping it to 5 minutes for the delve zones only frees the pool slot ~5 min after the last player
-- leaves. The INSTANCE record itself persists (Instance_Timer, 6h) so a player who steps out can still
-- return to the same instance -- only the idle process is reclaimed.
--
-- SCOPE: delve pool zones ONLY (per owner: "only for delves"). Normal zones keep the 5s rule.
-- APPLIES AT ZONE BOOT (zone config is read at boot, NOT shared memory) -- no ./shared_memory rebuild,
-- no zone rebuild. Already-running delve zones keep their old 1h timer until they next cycle.
-- IMPORTS: the `zone` table is SKIPPED in our selective DB merges, so this survives; if a full zone
-- import is ever done, re-run this. 300000 ms = 5 minutes.

UPDATE zone SET shutdowndelay = 300000
WHERE short_name IN (
  -- DoN (6)
  'delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest',
  -- LDoN (33)
  'guka','gukc','guke','gukf','gukh',
  'mira','mirb','mirc','mird','mirg','mirj',
  'mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj',
  'ruja','rujd','rujf','rujg','ruji','rujj',
  'taka','takb','takc','takd','take','takg'
);
