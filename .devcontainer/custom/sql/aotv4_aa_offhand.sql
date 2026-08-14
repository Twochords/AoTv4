-- aotv4_aa_offhand.sql -- two DUAL-WIELD AAs replacing two the server had legislated into uselessness.
-- =============================================================================================
--   host  31 Fear Resistance  ranks 116,117,118          -> Sinister Strikes   (SPA 249, ONE rank)
--   host 108 Flurry           ranks 255,256,257,542,543  -> Blindside          (SPA 304, five ranks)
--
-- Readable copy of migration v58. Both PASSIVE. Unlike the rest of the melee tree these are NOT
-- markers -- they are real native SPAs doing real work, so there is no C++ behind them.
--
-- ⚠️⚠️ WHY THE OLD TWO WENT. Neither was broken; both bought something this server already gives away
-- or forbids:
--   * `Steady Nerve` (31) resisted FEAR -- and fear spells are blacklisted from the reward pool
--     outright (CLAUDE.md section 5, the `fear` rule, 40 spells).
--   * `Run Them Down` (108) stopped enemies fleeing -- and `AoT:NPCsNeverFlee` already stops every
--     enemy fleeing, for everyone, with no AA at all.
-- `78 Nothing Left to Fear` (fear IMMUNITY) is dropped from aa_pool.lua for the same reason. It also
-- had to go regardless: its prerequisite was 31 at rank 3, and 31 is now a single-rank ability.
--
-- ⚠️⚠️ SPA 249 `SecondaryDmgInc` IS A BOOLEAN, NOT A MAGNITUDE. bonuses.cpp does
-- `newbon->SecondaryDmgInc = true;` and attack.cpp only TESTS it -- rank 2 and beyond would add
-- literally nothing. Hence ONE rank, with 116's chain terminated (`next_id = -1`). What it unlocks is
-- real: the offhand starts receiving the weapon damage bonus only the primary otherwise gets.
--
-- ⚠️⚠️ SPA 304 `OffhandRiposteFail` MUST BE NEGATIVE. Despite the name, attack.cpp:519 does
--     chance += chance * slip / 100;        // `chance` is the ENEMY'S RIPOSTE CHANCE
-- so a POSITIVE base makes your offhand swings riposted MORE often -- the exact opposite of the
-- ability. Same class of trap as SPA 121's reverse damage shield and SPA 20's positive base being
-- CURE blindness: the sign carries the meaning and the name does not tell you which way.
--
-- ⚠️⚠️ BOTH SPAs WERE CHECKED AGAINST `Mob::ApplyAABonuses` BEFORE BEING CHOSEN. That check is not
-- optional: **SPA 176 `DualWieldChance` has NO case in that function**, so an AA granting it is
-- silently inert while reading perfectly in the window -- and it was very nearly chosen for this.
-- CLAUDE.md section 32 records the same trap in reverse. Check the FUNCTION, never the [AA] tag.
--
-- 📌 Dual-wield FREQUENCY is already covered by `81 Offhand Grace` (SPA 276 Ambidexterity, +32 to the
-- roll out of 375). These two deliberately do not touch it: one makes the offhand hit HARDER, the
-- other makes it get PARRIED less. Three AAs pushing the same number would read as one bought thrice.
--
--   mysql -h127.0.0.1 -upeq -ppeqpass peq < .devcontainer/custom/sql/aotv4_aa_offhand.sql
--
-- No new spells, so no shared memory rebuild -- a zone restart is enough. db_str changes still need
-- ./export_client_files and dbstr_us.txt copied to the EQ root, or the client keeps showing the OLD
-- names and descriptions from its own file.
-- =============================================================================================

UPDATE aa_ability SET name = 'Sinister Strikes', classes = 65535, enabled = 1, type = 4 WHERE id = 31;
UPDATE aa_ability SET name = 'Blindside',        classes = 65535, enabled = 1, type = 4 WHERE id = 108;

-- Sinister Strikes: ONE rank only (SPA 249 is a boolean), so the chain stops at 116.
UPDATE aa_ranks SET cost = 3, level_req = 5, next_id = -1 WHERE id = 116;

-- Blindside keeps the melee tree's five-rung ladder.
UPDATE aa_ranks SET level_req = 5,  cost = 3 WHERE id = 255;
UPDATE aa_ranks SET level_req = 15, cost = 4 WHERE id = 256;
UPDATE aa_ranks SET level_req = 25, cost = 5 WHERE id = 257;
UPDATE aa_ranks SET level_req = 35, cost = 6 WHERE id = 542;
UPDATE aa_ranks SET level_req = 45, cost = 8 WHERE id = 543;
UPDATE aa_ranks SET next_id = -1 WHERE id = 543;

-- ⚠️ CLEAR THE INHERITED PREREQUISITES TOO. aa_rank_prereqs is a SEPARATE table from
-- aa_rank_effects, and a hosted AA inherits whatever its host required -- so the ability shows in the
-- window but refuses to train, with no explanation (CLAUDE.md section 6).
DELETE FROM aa_rank_prereqs WHERE rank_id IN (116,117,118, 255,256,257,542,543);

-- ⚠️ And the host's own effects: 31 carried SPA 181 (fear resist) 5/10/25 on 116/117/118.
DELETE FROM aa_rank_effects WHERE rank_id IN (116,117,118, 255,256,257,542,543);

INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
    (116, 1, 249,   1, 0),
    (255, 1, 304, -15, 0),
    (256, 1, 304, -30, 0),
    (257, 1, 304, -45, 0),
    (542, 1, 304, -65, 0),
    (543, 1, 304, -85, 0);

UPDATE db_str SET value = 'Sinister Strikes' WHERE id = 116 AND type = 1;
UPDATE db_str SET value = 'Blindside'        WHERE id = 255 AND type = 1;

UPDATE db_str SET value = 'Your weaker hand learns the same trick as your strong one. Your offhand weapon receives the damage bonus that normally only your primary weapon gets. This ability has a single rank -- the offhand either receives the bonus or it does not.' WHERE id = 116 AND type = 4;

UPDATE db_str SET value = 'You strike from where they are not looking. Attacks with your offhand weapon are 15, 30, 45, 65 and 85 percent less likely to be riposted. It does nothing for your primary hand.' WHERE id = 255 AND type = 4;

-- verify
SELECT a.id, a.name, a.enabled, a.type, a.first_rank_id FROM aa_ability a WHERE a.id IN (31, 108);
SELECT r.id, r.cost, r.level_req, r.next_id, e.effect_id, e.base1
  FROM aa_ranks r LEFT JOIN aa_rank_effects e ON e.rank_id = r.id
 WHERE r.id IN (116,117,118, 255,256,257,542,543) ORDER BY r.id;
