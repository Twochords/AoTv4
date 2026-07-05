-- AoTv4 "Refining Crucible" -- a generic upgrade bag. Put in 4 identical items and Combine; if a
-- higher gear tier of that item exists (Hallowed = base id +1,000,000, Mythic = +2,000,000), the 4 are
-- consumed into 1 of the next tier. Handled in C++ (zone/tradeskills.cpp `AoTv4RefineCombine`), gated on
-- this exact item id (2000060) -- NOT on bagtype -- so real bagtype-30 quest containers are untouched.
--
-- Items live in SHARED MEMORY: after applying this you must rebuild it with world DOWN:
--   stop world+zones -> `cd build/bin && ./shared_memory` -> restart world (zones reboot on demand).

-- Clone a known-good bagtype-30 (AlwaysWorks / shows a Combine button) 10-slot container, then override
-- identity + make the bag hold Giant items so any gear fits.
DROP TEMPORARY TABLE IF EXISTS tmp_crucible;
CREATE TEMPORARY TABLE tmp_crucible AS SELECT * FROM items WHERE id = 17033;
UPDATE tmp_crucible SET
	id       = 2000060,
	name     = 'Refining Crucible',
	icon     = 1016,   -- Gigantic Velium Crucible icon
	itemclass = 1,     -- container
	bagtype  = 30,     -- AlwaysWorks quest container -> shows the Combine button, works anywhere
	bagslots = 10,
	bagsize  = 4,      -- Giant: holds any gear
	bagwr    = 100,    -- 100% weight reduction (a tool, not a burden)
	nodrop   = 1,      -- droppable/tradeable (1 = NOT no-drop)
	norent   = 255,    -- never rent-expires
	loregroup = 0,     -- not lore -> can own several
	stackable = 0,
	price    = 500,    -- 5 gold base (merchant markup ~5% -> shows ~5g at a vendor)
	weight   = 0;
DELETE FROM items WHERE id = 2000060;
INSERT INTO items SELECT * FROM tmp_crucible;
DROP TEMPORARY TABLE tmp_crucible;

-- Sell the crucible at every general-goods / satchel vendor -- i.e. every merchant that already sells a
-- container/bag (itemclass = 1). Added at each merchant's next free slot; min/max_expansion default -1
-- so it shows regardless of the era lock. Idempotent (clears any prior crucible rows first).
-- merchantlist loads at ZONE BOOT -> restart zones after (a world restart reboots them).
DELETE FROM merchantlist WHERE item = 2000060;
INSERT INTO merchantlist (merchantid, slot, item)
	SELECT ml.merchantid, MAX(ml.slot) + 1, 2000060
	FROM merchantlist ml
	WHERE ml.merchantid IN (
		SELECT DISTINCT m.merchantid FROM merchantlist m JOIN items i ON m.item = i.id WHERE i.itemclass = 1
	)
	GROUP BY ml.merchantid;
