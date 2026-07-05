-- aotv4_trader_satchel_vendor.sql
-- Sell the basic Trader's Satchel (id 17899) at every merchant that already sells a container, so
-- players can easily buy one to stock their /shop (which now only lists items placed inside a satchel).
-- Same placement pattern as the Refining Crucible. Price comes from the item (1000 = 1g) + merchant
-- markup; lowering it would need a shared-memory rebuild, so it's left as-is.
--
-- merchantlist loads at ZONE BOOT -> restart zones after (a world restart reboots them). Idempotent.

DELETE FROM merchantlist WHERE item = 17899;
INSERT INTO merchantlist (merchantid, slot, item)
	SELECT ml.merchantid, MAX(ml.slot) + 1, 17899
	FROM merchantlist ml
	WHERE ml.merchantid IN (
		SELECT DISTINCT m.merchantid FROM merchantlist m JOIN items i ON m.item = i.id WHERE i.itemclass = 1
	)
	GROUP BY ml.merchantid;
