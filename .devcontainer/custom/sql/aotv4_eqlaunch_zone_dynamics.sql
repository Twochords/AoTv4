-- aotv4_eqlaunch_zone_dynamics.sql  (2026-08-01)
-- =================================================================================================
-- Set the number of dynamic zones eqlaunch's "zone" launcher maintains.
--
-- The deployment is switching from statically-spawned zones (start-server.sh loop + the
-- zone-keepalive.sh watcher) to eqlaunch, which boots this many dynamic zones and RESPAWNS them as
-- they exit -- so the pool never erodes to "No zoneserver available to boot up".
--
-- MUST stay <= the published zone port range (7000-7049 = 50 ports); 45 leaves headroom. To raise
-- it, widen the range in .env PORT_RANGE_*, eqemu_config zones.ports AND the devcontainer appPort,
-- then restart the container (CLAUDE.md sec 27), and only then raise this.
--
-- eqlaunch reads this at launch -> takes effect the next time `eqlaunch zone` is started.

UPDATE launcher SET dynamics = 45 WHERE name = 'zone';
