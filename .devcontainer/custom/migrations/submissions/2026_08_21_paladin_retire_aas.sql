-- @aotv4-migration
-- description: 2026_08_21_paladin_retire_aas
-- check: SELECT `enabled` FROM `aa_ability` WHERE `id` = 45
-- condition: missing
-- match: 0
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Retires the three activated Paladin AAs now that the class has real disciplines (44706-8).
-- notes: MUST ship together with 2026_08_21_paladin_class_abilities -- either alone leaves a Paladin
-- notes: with the ability twice, or with none at all.

-- ⚠️⚠️ PALADIN WAS THE LAST CLASS ON A DIFFERENT MECHANISM. Its three abilities were activated AAs
-- (45 Enhanced Root / 55 Permanent Illusion / 79 2 Hand Bash -- native rows we took over, because a
-- NEW aa_ability id never reaches the client, section 10) casting spells 44600-44602 on spell_type
-- 82/83/84. Every other class finds its kit in the Combat Abilities window, so a Paladin looking in
-- the AA window for the same thing was a real inconsistency, not a cosmetic one.

-- ⚠️⚠️ `character_alternate_abilities.aa_id` STORES THE `first_rank_id`, NOT THE ABILITY ID
-- (section 47), and `aa_value` is points spent rather than a rank number. The three first_rank_ids
-- are 144 / 158 / 196. Deleting by ability id would match nothing and silently leave every Paladin
-- holding both versions.
-- ⚠️ Safe to scope this way: all three hosts are `classes = 4` (Paladin) and `grant_only = 1`, so
-- nobody else can have trained them.
-- 📌 They were granted with ignore_cost, so no points were spent and there is nothing to refund.
DELETE FROM character_alternate_abilities WHERE aa_id IN (144, 158, 196);

-- ⚠️ Disabling is what actually takes them out of the window: `zone/aa.cpp` loads only enabled rows
-- (section 32), so the ability stops existing at runtime rather than merely being unbuyable.
-- 📌 The rank rows, their db_str strings and spells 44600-44602 are LEFT ALONE deliberately. They
-- cost nothing dormant, and they are the only record of how the AA version was built if this is ever
-- revisited. `lua_modules/aotv4_paladin.lua` and quests/global/spells/446xx.lua go dormant with them.
UPDATE aa_ability SET enabled = 0 WHERE id IN (45, 55, 79);
