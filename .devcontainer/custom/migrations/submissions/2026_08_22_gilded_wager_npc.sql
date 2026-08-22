-- @aotv4-migration
-- description: 2026_08_22_gilded_wager_npc
-- check: SELECT `npcID` FROM `spawnentry` WHERE `npcID` = 2000403
-- condition: empty
-- match:
-- shared-memory: no
-- band:
-- author: Claude
-- notes: The Gilded Wager -- a fourth Resplendent hub NPC that sells a random wearable for coin.

-- ⚠️⚠️ npc_types IS READ AT ZONE BOOT, NOT FROM SHARED MEMORY, so this needs a zone restart and NOT
-- a ./shared_memory rebuild. Only `items` and `spells_new` live in shared memory (section 10).
--
-- 📌 Cloned from Reforger Vael (2000401) through a temp table so all ~100 columns stay byte identical
-- to a working hub NPC -- the same rule the custom spell lines follow. Only identity and position
-- are overridden.
-- ⚠️⚠️ class 1 and bodytype 1 ARE LOAD BEARING, and section 11 records these exact NPCs being
-- clobbered TWICE: bodytype 11 is NoTarget, so nobody can hail them, and class 41 is Merchant, so
-- right clicking opens an empty merchant window instead of the trade window this NPC needs.
-- ⚠️⚠️ AND THE HUB LOCK TRIGGERS DO NOT COVER THIS ID. custom/sql/aotv4_resplendent_npc_lock.sql
-- guards 2000400-2000402 only; 2000403 is added there in the same commit, but that script is
-- HAND RUN and must be re-run after any full import (section 11).
--
-- 📌 Position is the MIDPOINT of two NPCs that already stand there (2000400 at -30.7/696.6/-26.2 and
-- 2000402 at 7.3/688.8/-23.4), so the ground is known good rather than guessed -- a wrong z leaves an
-- NPC floating or sunk, and there is no way to tell from the data. 19 units from each neighbour.
-- ⚠️ heading 256 matches the other three, so the row faces arriving players like its neighbours.
-- ⚠️ min_expansion / max_expansion -1: these spawns were content filtered out once already.
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;
DELETE FROM npc_types  WHERE id = 2000403;

DROP TEMPORARY TABLE IF EXISTS aotv4_npc_tmpl;
CREATE TEMPORARY TABLE aotv4_npc_tmpl LIKE npc_types;
INSERT INTO aotv4_npc_tmpl SELECT * FROM npc_types WHERE id = 2000401;
UPDATE aotv4_npc_tmpl SET
    id = 2000403,
    name = '#Gilded_Wager',
    lastname = 'Keeper of the Wheel',
    class = 1, bodytype = 1, gender = 0, race = 12, texture = 1, size = 4,
    merchant_id = 0, npc_faction_id = 0, runspeed = 0;
INSERT INTO npc_types SELECT * FROM aotv4_npc_tmpl;
DROP TEMPORARY TABLE IF EXISTS aotv4_npc_tmpl;

INSERT INTO spawngroup (id, name) VALUES (2000403, 'aotv4_gilded_wager');
INSERT INTO spawn2 (spawngroupID, zone, version, x, y, z, heading, respawntime, variance, pathgrid, min_expansion, max_expansion)
VALUES (2000403, 'resplendent', 0, -12.0, 692.0, -24.8, 256, 300, 0, 0, -1, -1);
INSERT INTO spawnentry (spawngroupID, npcID, chance) VALUES (2000403, 2000403, 100);
