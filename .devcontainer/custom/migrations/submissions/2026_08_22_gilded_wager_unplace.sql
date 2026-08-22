-- @aotv4-migration
-- description: 2026_08_22_gilded_wager_unplace
-- check: SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `spawnentry` WHERE `npcID` = 2000403
-- condition: match
-- match: pending
-- shared-memory: no
-- band:
-- author: Claude
-- notes: The Gilded Wager keeps its npc_types row but is no longer spawned. It gets placed by hand.

-- ⚠️⚠️ THE NPC ROW STAYS, ONLY THE PLACEMENT GOES. v137 both created 2000403 and stood it in
-- Resplendent; the definition is what the quest script and the pool hang off, and deleting it would
-- take the whole feature with it. This removes the spawn only, so the owner can put him where he
-- belongs with the in-game tool.
--
-- ⚠️⚠️ AND THE QUEST SCRIPT MOVED TO `quests/global/2000403.lua` IN THE SAME COMMIT, WHICH IS THE
-- HALF THAT IS EASY TO MISS. A per-NPC script is searched as quests/<zone>/<id|name> and only THEN
-- as quests/global/<id|name> (quest_parser_collection.cpp:1097). Left in quests/resplendent/ it
-- would answer in Resplendent and nowhere else -- the NPC would spawn, be hailable, and silently
-- never respond. Named by ID rather than by name because the name carries a leading '#' and a
-- rename would orphan it just as quietly.
--
-- 📌 Placing him: `#npcspawn create 2000403` while standing where he should be, then `#npcspawn add`
-- to persist it, exactly as the Plane of Knowledge book in section 11 was placed by hand. That writes
-- its own spawngroup, spawn2 and spawnentry rows.
-- ⚠️⚠️ WHATEVER GROUP ID THAT CREATES IS **NOT** COVERED BY THE HUB LOCK. The spawn half of
-- custom/sql/aotv4_resplendent_npc_lock.sql keys on spawngroupID, and it cannot know an id that does
-- not exist yet -- so heading and min_expansion on the new spawn are unguarded against a full import.
-- The npc_types half (class 1, bodytype 1) IS widened to 2000403 and does protect him from being
-- turned back into an untargetable merchant.
DELETE FROM spawnentry WHERE npcID = 2000403;
DELETE FROM spawn2     WHERE spawngroupID = 2000403;
DELETE FROM spawngroup WHERE id = 2000403;
