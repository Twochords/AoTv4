-- aotv4_npc_class_scale.lua -- GENERATED from the `npc_class_scale` table. Do not hand-edit.
--   mysql ... -N -e "SELECT CONCAT_WS('|',class_id,name,...) FROM npc_class_scale
--                    WHERE class_id BETWEEN 0 AND 16 ORDER BY class_id"
--
-- ⚠️⚠️ THE SOURCE TABLE IS READ BY NOTHING IN THE SERVER. `npc_class_scale` appears ZERO times in the
-- zone and world binaries (`npc_scale_global_base` -- a DIFFERENT table -- appears 73 times). It has
-- no reader, no generated repository and no manifest entry, so it is not a feature that was switched
-- off; it is almost certainly a table from another EQEmu fork. Loading it changes nothing by itself.
--
-- 📌 SO THE INTENT IS IMPLEMENTED WITHOUT BUILDING A SUBSYSTEM FOR IT. Every column maps onto a key
-- `ModifyNPCStat` already accepts, and `aotv4_difficulty.on_npc_spawn` already reads a stat and
-- multiplies it. This is data for machinery we own -- no C++, no repository, no boot-time load.
-- ⚠️ The DB table remains the source of truth; regenerate this file rather than editing it.
--
-- ⚠️⚠️ IT APPLIES ONLY INSIDE CHALLENGE SHARDS, AND THAT IS THE POINT. Wiring the table server wide
-- would change Normal for everybody: 63,072 of 67,604 npc_types carry a player class and 41,536 of
-- those are Warrior, so `max_dmg x1.5` would be a server-wide melee buff rather than a flavour pass.
-- The challenge system is separate, so this rides with it and Normal is untouched.
--
-- ⚠️ CLASSES 0-16 ONLY. 40-44 (Banker, Merchant, Quest, Adventure Merchant/Recruiter) carry x100 on
-- every stat in the source table -- that is how a merchant is made unkillable, and it must never be
-- multiplied onto a creature.
-- ⚠️ `mana` is deliberately NOT carried across. An NPC with 0 mana casts FREELY, because the AI gate
-- is `mana_cost <= GetMana() || GetMana() == GetMaxMana()` (section 24) -- so raising max_mana
-- without raising CURRENT mana breaks that equality and makes caster creatures cast LESS. The source
-- only holds 0 or 1.5 there, and 0 on an already-empty pool is a no-op, so nothing is lost.

local M = {}

-- class_id -> { ModifyNPCStat key -> multiplier }. Only values that differ from 1.0 are stored, so a
-- class with nothing to say costs the caller one table lookup and no work.
M.BY_CLASS = {
	[ 0] = { sta = 1.1, hp = 1.1 },    -- Other
	[ 1] = { str = 2, ac = 1.2, hp = 1.2, max_hit = 1.5 },    -- Warrior
	[ 2] = {},                                               -- Cleric
	[ 3] = { str = 1.5, sta = 1.1, ac = 1.1, hp = 1.1, max_hit = 1.2 },    -- Paladin
	[ 4] = { str = 2, dex = 2.5 },    -- Ranger
	[ 5] = { str = 1.5, sta = 1.1, ac = 1.1, hp = 1.1, max_hit = 1.2 },    -- Shadow Knight
	[ 6] = {},                                               -- Druid
	[ 7] = { str = 2.2, agi = 2, max_hit = 1.6 },    -- Monk
	[ 8] = {},                                               -- Bard
	[ 9] = { str = 2.5, max_hit = 2 },    -- Rogue
	[10] = {},                                               -- Shaman
	[11] = {},                                               -- Necromancer
	[12] = { int = 2.6 },    -- Wizard
	[13] = { int = 2.5 },    -- Magician
	[14] = { cha = 2 },    -- Enchanter
	[15] = {},                                               -- Beastlord
	[16] = { str = 2.5, sta = 1.1, ac = 1.1, hp = 1.1, max_hit = 2 },    -- Berserker
}

-- ⚠️⚠️ `hp` IS RETURNED SEPARATELY and must not be applied as an ordinary stat. `ModifyNPCStat("max_hp")`
-- runs CalcMaxHP(), so two calls would compute the second from the FIRST call's result rather than
-- from the creature's original maximum. The caller folds this into its single max_hp write.
function M.hp_mult(class_id)
	local t = M.BY_CLASS[class_id or -1]
	return (t and t.hp) or 1.0
end

-- Every multiplier EXCEPT hp, ready to feed to the caller's own scale() helper. nil for a class with
-- nothing to change.
function M.stats(class_id)
	return M.BY_CLASS[class_id or -1]
end

return M
