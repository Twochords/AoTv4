-- ============================================================================================
-- AoTv4 -- Parchment Fragments and Ink of the Lost become ALTERNATE CURRENCY (2026-08-02)
--
-- ============================================================================================
-- WHY: THE ROGUELITE DEATH WAS EATING THE CURRENCY IT PAID OUT
-- ============================================================================================
-- Both were ordinary items sitting in the player's bags, and `death_loss.lua` destroys carried
-- inventory wholesale (`DeleteItemInInventory(slot, 0, true)`). So:
--   * Fragments are awarded AFTER the wipe, so they survive the death that created them -- and are
--     then destroyed by the NEXT one.
--   * Ink drops from global loot during a run and is destroyed by the death that ends it.
-- Rank 5 costs 240 fragments and 40 ink (aotv4_spell_ranks_sys.M.COST). Saving that across deaths
-- was impossible when a death is the only thing that pays fragments in the first place.
--
-- Alternate currency lives in `character_alt_currency`, NOT in the inventory, so nothing in the
-- death path can touch it -- the same reason the Delver's Sigil is an evolving item and the delve
-- clear ladder is a data bucket. It also cannot be dropped, traded or accidentally sold.
--
-- ⚠️ ids 57 and 58: the table already uses 4, 5 and 10-56, so these are the first free ones. They
-- must match M.FRAGMENT_CURRENCY / M.INK_CURRENCY in aotv4_spell_ranks_sys.lua.
-- ⚠️ The ITEM rows (147920/147921) are deliberately kept. Alternate currency in EQEmu is defined as
-- a currency/item PAIR -- the client resolves the icon and name for its currency window from the
-- item -- so deleting them would leave two nameless currencies.
--
-- ⚠️ NOT shared memory: `alternate_currency` is read at zone boot. Restart zones, no ./shared_memory.
-- ============================================================================================

DELETE FROM alternate_currency WHERE id IN (57, 58);
INSERT INTO alternate_currency (id, item_id) VALUES
    (57, 147920),   -- Parchment Fragment
    (58, 147921);   -- Ink of the Lost

SELECT ac.id AS currency_id, ac.item_id, i.Name
FROM alternate_currency ac JOIN items i ON i.id = ac.item_id
WHERE ac.id IN (57, 58);
