-- ============================================================================================
-- AoTv4 -- let non-GM players actually ENTER the delve zones (2026-08-02)
--
-- Reported as "delves look region locked". They are not region locked -- the refusal was:
--   "The zone that you are attempting to enter is part of an expansion that you do not yet own."
--
-- ============================================================================================
-- WHY IT ONLY EVER FAILED FOR PLAYERS
-- ============================================================================================
-- The six delve zones are Dragons of Norrath content (`zone.expansion` = 9) and this server runs
-- Classic (`Expansion:CurrentExpansion` = 0). zone/zoning.cpp:366 gates entry on:
--
--     if (CurrentExpansion >= Classic && !GetGM()) {
--         if (z->expansion <= CurrentExpansion || z->bypass_expansion_check) { ...ok... }
--     }
--
-- ⚠️⚠️ NOTE THE `!GetGM()`. Every delve ever tested was tested with the GM flag ON, which skips the
-- check entirely and logs "Bypassing zone expansion checks because GM flag is set". The moment GM was
-- turned off to see the game as a player, entry started failing -- and because the run bucket, the
-- task and the instance are all created BEFORE the move, it looked like the delve opened and then
-- refused. It also explains why Swarm "never split": the player never got inside at all.
--
-- ============================================================================================
-- WHY `bypass_expansion_check` AND NOT `expansion = 0`
-- ============================================================================================
-- The check offers a purpose-built escape hatch, and it is the correct one to use: `expansion` is
-- real metadata (content filtering, zone listings, the era system in section 12 all read it), so
-- rewriting it to 0 would make DoN zones claim to be Classic content everywhere else in the server.
-- `bypass_expansion_check` exempts ONLY the entry gate and leaves the zone honest about what it is.
--
-- ⚠️ Scoped to the six delve zones BY NAME. A proposed fix went through `zone_regions` with
-- `WHERE region_id <= 98`, which matches 69 zones and would have set expansion 0 AND min_status 0
-- across the entire region system -- disabling gating for content that is meant to be locked, with
-- no record afterwards of which zones had been configured deliberately.
--
-- ⚠️ Applied to EVERY version row, not just version 0. The delve enters mission versions (1-15), and
-- although the check reads version 0 via GetZoneWithFallback, leaving the others at 0 would be a trap
-- for the next person who changes how the version is resolved.
--
-- ============================================================================================
-- THE SECOND GATE: thenest min_level 66
-- ============================================================================================
-- `Client::CanEnterZone` separately checks min_status / min_level / max_level / flag, and refuses
-- with a DIFFERENT message. The Nest ships `min_level = 66`, so even with the expansion gate open a
-- level 30 character -- i.e. everybody, at this cap -- would still be refused from that one zone.
-- The delve does its own gating through the rung ladder, so the stock level floor is removed.
--
-- ⚠️ NOT shared memory: the `zone` table is read at boot by world AND zone. Restart both.
-- ============================================================================================

UPDATE zone
SET bypass_expansion_check = 1
WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest');

UPDATE zone
SET min_level = 0
WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest')
  AND min_level > 0;

-- Verification: bypass = 1 everywhere, min_level 0, expansion left honest at 9.
SELECT short_name, COUNT(*) AS version_rows, MIN(bypass_expansion_check) AS bypass,
       MAX(min_level) AS max_min_level, MAX(min_status) AS max_min_status, MIN(expansion) AS expansion
FROM zone
WHERE short_name IN ('delvea','delveb','stillmoona','stillmoonb','thundercrest','thenest')
GROUP BY short_name ORDER BY short_name;
