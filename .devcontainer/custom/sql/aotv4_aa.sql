-- AoTv4 AA system DB changes. Run on a FRESH DB (after a reset), then restart world + zones
-- (AA data is loaded from the DB by the zone at boot -- NOT shared memory -- and rules are read
-- at startup). Then regenerate quests/lua_modules/aa_pool.lua from the tagged set (see CLAUDE.md).

-- 1) Lower level_req of the 1-60 AA tier to 1 so they're trainable from the always-level-1
--    post-death state. Rank chains (prev_id/next_id) + aa_rank_prereqs are untouched, so AAs
--    still depend on each other -- only the LEVEL gate is removed. 61+ ranks unchanged.
UPDATE aa_ranks SET level_req = 1 WHERE level_req BETWEEN 1 AND 60;

-- 2) Make AAs Bard-usable. Strip the Bard bit from ALL rows first (clean slate), then tag exactly
--    ONE row per name with it, restricted to the CLASSIC, in-era set:
--      * expansion <= 4 (Classic..PoP -- the era the RoF2 client has dbstr for; avoids the
--        "Unknown DB String" rows that newer AAs produce)
--      * first-rank level_req <= 60
--      * enabled, not grant-only, activated/passive types 1-5, real names (no Unknown/Glyph/Innate)
--      * deduped by name (EQ ships per-class/expansion copies; one keeps the bit, see MIN(id))
--    The loader left-shifts classes by one bit (zone/aa.cpp), so DB 128 (1<<(class-1)) becomes
--    256 (1<<class) in memory and matches the stock `classes & (1<<GetClass())` gate. Do NOT
--    "fix" that line in aa.cpp.
UPDATE aa_ability SET classes = classes & ~128 WHERE (classes & 128) > 0;
UPDATE aa_ability a
JOIN aa_ranks r ON r.id = a.first_rank_id
JOIN (
  SELECT a2.name, MIN(a2.id) AS rep
  FROM aa_ability a2 JOIN aa_ranks r2 ON r2.id = a2.first_rank_id
  WHERE a2.enabled = 1 AND a2.grant_only = 0 AND a2.type IN (1,2,3,4,5)
    AND r2.level_req <= 60 AND r2.expansion BETWEEN 0 AND 4
    AND a2.name IS NOT NULL AND a2.name <> '' AND a2.name NOT LIKE 'Unknown%'
    AND a2.name NOT LIKE 'Glyph%' AND a2.name NOT LIKE 'Innate%'
  GROUP BY a2.name
) rp ON a.id = rp.rep
SET a.classes = a.classes | 128;

-- 2b) Mark the tagged set grant-only. The native AA window then lists each one only once it's been
--     TRAINED (untrained ones are hidden) and it can't be bought there -- the random picker is the
--     only way to spend. Our picker grants with ignore_cost=true (CanPurchase check_grant=false),
--     so grant_only does NOT block it. Run AFTER step 2 (which selects grant_only=0 rows).
UPDATE aa_ability SET grant_only = 1 WHERE (classes & 128) > 0;

-- 3) AAs start in Luclin (expansion >= 3). With classic theming (CurrentExpansion=0),
--    UseCurrentExpansionAAOnly blocks EVERY AA. Turn it off; era gating is done by the tag set
--    (step 2) + the pool. The per-char m_pp.expansions check still applies (= World:ExpansionSettings).
UPDATE rule_values SET rule_value = 'false' WHERE rule_name = 'Expansion:UseCurrentExpansionAAOnly';
