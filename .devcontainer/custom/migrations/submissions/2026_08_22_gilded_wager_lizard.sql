-- @aotv4-migration
-- description: 2026_08_22_gilded_wager_lizard
-- check: SELECT IF((SELECT COUNT(*) FROM `npc_types` WHERE `id` = 2000403 AND (`race` <> 51 OR `gender` <> 2 OR `size` <> 10)) + (SELECT COUNT(*) FROM `spawnentry` WHERE `npcID` = 2000403 AND `spawngroupID` <> 2000403) > 0, 'pending', 'done')
-- condition: match
-- match: pending
-- shared-memory: no
-- band:
-- author: Claude
-- notes: Gilded Wager becomes a Lizard Man at size 10, and the duplicate hand-placed spawn is removed.

-- ⚠️⚠️ GENDER MUST BE 2, AND IT IS NOT COSMETIC. All 205 stock NPCs at race 51 use gender 2 -- the
-- Lizard Man has no male or female model at all. He was cloned from Reforger Vael, a gnome, so he
-- carries gender 0; leaving that behind is exactly the section 48 failure, where the Titan Hall
-- guards had a race with no matching model and simply did not render. Nothing server side reports it.
-- 📌 Race 51 is a safe choice for a reason: `liz,liz_chr` is one of the 13 archives in
-- Resources/GlobalLoad_chr.txt, so the model is loaded in EVERY zone, not just this one. That matters
-- because this NPC is placed by hand and may be moved.
-- ⚠️ texture 0 to match the stock rows; the gnome clone left texture 1 behind.
--
-- ⚠️⚠️ TWO OF HIM WERE SPAWNING. v139 placed one through spawngroup 2000403, and a second was placed
-- in game with `#npcspawn`, which generated its own group (3300138) and the 1200 second respawn that
-- command defaults to. Both sat at the same spot, a tenth of a unit apart.
-- 📌 That accidental duplicate is also the proof that the /loc conversion in v139 was right: a
-- hand-placed marker and a computed one landing within 0.1 units cannot both be wrong the same way.
-- ⚠️ The delete is written as "any spawn of this NPC that is NOT the managed group", not as the one
-- id seen today, so it also cleans up any future hand-placed duplicate -- and the check above tests
-- for that too, which makes this migration self-healing rather than a one-off.
DELETE se, s2 FROM spawnentry se
  LEFT JOIN spawn2 s2 ON s2.spawngroupID = se.spawngroupID
  WHERE se.npcID = 2000403 AND se.spawngroupID <> 2000403;

UPDATE npc_types SET race = 51, gender = 2, size = 10, texture = 0 WHERE id = 2000403;
