-- AoTv4 AA system DB changes. Run on a FRESH DB (after a reset), then restart world + zones
-- (AA data is loaded from the DB by the zone at boot -- NOT shared memory -- and rules are read
-- at startup). Then regenerate quests/lua_modules/aa_pool.lua from the tagged set (see CLAUDE.md).

-- 1) Lower level_req of the 1-70 AA tier to 1 so they're trainable from the always-level-1
--    post-death state. Rank chains (prev_id/next_id) + aa_rank_prereqs are untouched, so AAs
--    still depend on each other -- only the LEVEL gate is removed. 71+ ranks (raid-tier) unchanged.
--    The expansion-unlock ERA gating (era_system.lua / aa_pool.era) replaces level as the real
--    gate: a character is offered an AA only once its era is unlocked server-wide. See
--    AOTV4_EXPANSION_PLAN.md.
UPDATE aa_ranks SET level_req = 1 WHERE level_req BETWEEN 1 AND 70;

-- 2) Make AAs Bard-usable. Strip the Bard bit from ALL rows first (clean slate), then tag exactly
--    ONE row per name with it, restricted to the era set (Classic..Omens of War):
--      * expansion 0-8 (Classic..OoW). Era *availability* is gated at runtime by aa_pool.era +
--        era_system, NOT here -- this just makes the whole ladder Bard-usable in the DB.
--      * first-rank level_req <= 70 (raid-tier 71+ excluded -- un-grantable at the level-1 pick state)
--      * enabled, activated/passive types 1-5, real names (no Unknown/Glyph)
--      * deduped by name (EQ ships per-class/expansion copies; one keeps the bit, see MIN(id))
--    NOTE: we do NOT filter grant_only here -- it's unreliable (this migration itself sets it in 2b,
--    so a re-run would see prior rows as grant_only=1). The type 1-5 + name + dedup filters already
--    yield a clean, recognizable set (verified through OoW). This SELECT must stay in step with the
--    aa_pool.lua generator (same WHERE) so pool == tagged set (353 AAs).
--    The loader left-shifts classes by one bit (zone/aa.cpp), so DB 128 (1<<(class-1)) becomes
--    256 (1<<class) in memory and matches the stock `classes & (1<<GetClass())` gate. Do NOT
--    "fix" that line in aa.cpp.
UPDATE aa_ability SET classes = classes & ~128 WHERE (classes & 128) > 0;
UPDATE aa_ability a
JOIN aa_ranks r ON r.id = a.first_rank_id
JOIN (
  SELECT a2.name, MIN(a2.id) AS rep
  FROM aa_ability a2 JOIN aa_ranks r2 ON r2.id = a2.first_rank_id
  WHERE a2.enabled = 1 AND a2.type IN (1,2,3,4,5)
    AND r2.level_req <= 70 AND r2.expansion BETWEEN 0 AND 8
    AND a2.name IS NOT NULL AND a2.name <> '' AND a2.name NOT LIKE 'Unknown%'
    AND a2.name NOT LIKE 'Glyph%'
  GROUP BY a2.name
) rp ON a.id = rp.rep
SET a.classes = a.classes | 128;

-- 2b) Mark the tagged set grant-only. The native AA window then lists each one only once it's been
--     TRAINED (untrained ones are hidden) and it can't be bought there -- the random picker is the
--     only way to spend. Our picker grants with ignore_cost=true (CanPurchase check_grant=false),
--     so grant_only does NOT block it. Run AFTER step 2 (which tags the set).
UPDATE aa_ability SET grant_only = 1 WHERE (classes & 128) > 0;

-- 3) AAs start in Luclin (expansion >= 3). With classic theming (CurrentExpansion=0),
--    UseCurrentExpansionAAOnly blocks EVERY AA. Turn it off; era gating is done by the tag set
--    (step 2) + the pool. The per-char m_pp.expansions check still applies (= World:ExpansionSettings).
UPDATE rule_values SET rule_value = 'false' WHERE rule_name = 'Expansion:UseCurrentExpansionAAOnly';

-- 4) EXPANSION-UNLOCK ERA SYSTEM (see AOTV4_EXPANSION_PLAN.md). The server starts at era 0
--    (Classic) -- CurrentExpansion = 0. The unlocked era is the GLOBAL data bucket 'aotv4_era';
--    era_system.lua drives the level cap + AA-pool gate live off it, and on each maxout advance it
--    bumps Expansion:CurrentExpansion live (per zone). LEVEL/SPELL/AA progression is fully live; the
--    CONTENT side (zones/items that filter at zone BOOT) only fully opens once CurrentExpansion is
--    persisted here + the world is restarted. To force a content era manually:
--      UPDATE rule_values SET rule_value='<n>' WHERE rule_name='Expansion:CurrentExpansion';  -- then restart
--    To reset the progression race to Classic: DELETE FROM data_buckets WHERE `key`='aotv4_era';
UPDATE rule_values SET rule_value = '0' WHERE rule_name = 'Expansion:CurrentExpansion';

-- AoTv4: remove the "First Aid" AA (id 17) as a PREREQUISITE for anything. First Aid is dropped from
-- the reward pool (aa_pool.lua), so it can never be trained -- which would otherwise soft-lock every AA
-- that required it (e.g. Critical Mend, Bandage Wound). Clearing its prereq rows lets those be trained
-- directly. AAs load at zone BOOT -> zone restart applies this.
DELETE FROM aa_rank_prereqs WHERE aa_id = 17;

-- AoTv4: remove all Mend-family AAs (Mend Companion 58, Critical Mend 97, Hastened Mending 161,
-- Replenish Companion 418) from progression. They're dropped from the reward pool (aa_pool.lua), so
-- they can never be trained; clearing their prereq rows unblocks anything that required them (Mend
-- Companion alone gated 26 ranks). Hastened Mend (3805) was never in the pool. Zone restart applies.
DELETE FROM aa_rank_prereqs WHERE aa_id IN (58,97,161,418);
