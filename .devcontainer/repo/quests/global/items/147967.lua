-- Tome of Insight -- grants one extra reward pick inside its own level band.
-- All three tiers are identical here; the tier, the band and the reroll cut all come from the item
-- id via aotv4_spell_books.M.BY_ITEM.
--
-- ⚠️⚠️ A QUEST ITEM SCRIPT IS FOUND BY FILE NAME. This file must be named <itemid>.lua and sit in
-- quests/global/items/ (NOT quests/items/, which GetQIByItemQuest never searches) -- rename the item id in aotv4_spell_books without renaming this file and the book
-- becomes inert on click, with no error logged anywhere.
--
-- ⚠️ EVENT_ITEM_CLICK fires here only because the item carries a clickeffect. Handle_OP_ItemVerify-
-- Request reads item->Click.Effect and the RoF2 client will not even SEND the packet for an item it
-- believes has no click, so the inert spell 44328 on the item row is load bearing, not decoration.
function event_item_click(e)
	require("aotv4_spell_books").use(e.owner, e.slot_id, 147967)
end

-- ⚠️⚠️ RoF2 CLICKS AN ITEM VIA OP_CastSpell, WHICH RAISES EVENT_ITEM_CLICK_CAST -- NOT
-- EVENT_ITEM_CLICK. EVENT_ITEM_CLICK is raised from Handle_OP_ItemVerifyRequest
-- (client_packet.cpp:9532); the cast path raises EVENT_ITEM_CLICK_CAST instead (:4462, :4494).
-- With only the former defined the tome cast its inert spell and the script never ran: the zone log
-- showed "Cast from unlimited charge item [Worn Tome of Insight]" on every click while the player
-- saw nothing at all. Both are defined because which one arrives depends on the client and the slot,
-- and a tome that silently does nothing is indistinguishable from a broken one.
-- ⚠️ M.use is deduped (see aotv4_spell_books) so a client that sends BOTH still spends one tome.
function event_item_click_cast(e)
	require("aotv4_spell_books").use(e.owner, e.slot_id, 147967)
end
