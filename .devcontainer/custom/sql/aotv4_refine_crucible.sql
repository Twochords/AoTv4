-- AoTv4 "Refining Crucible" -- a generic upgrade bag. Put in 4 identical items and Combine; if a
-- higher gear tier of that item exists (Hallowed = base id +300,000, Mythic = +600,000), the 4 are
-- consumed into 1 of the next tier. Handled in C++ (zone/tradeskills.cpp `AoTv4RefineCombine`), gated on
-- this exact item id (147510) -- NOT on bagtype -- so real bagtype-30 quest containers are untouched.
--
-- ⚠️⚠️ THE ID MOVED 2000060 -> 147510 (migration v53, 2026-08-09) AND MUST NOT MOVE BACK. An item id
-- at or above 0x100000 (1,048,576) CANNOT BE LINKED IN CHAT: RoF2 packs the id into a five hex digit
-- field and common/say_link.cpp masks it (`0x000FFFFF & item_id`), so 2000060 (0x1E84BC -- six
-- digits) encoded as 951484, an id that does not exist. Nothing errored; the link simply described
-- another item and the client rendered the leftovers. Reported from play as "6Refining Crucible".
-- ⚠️⚠️ THIS SCRIPT WAS THE LIVE TRAP, NOT JUST A STALE COMMENT. Re-running it while it still said
-- 2000060 would have recreated the broken item AND sold *that* copy at every container merchant --
-- so vendors would stock a crucible that cannot refine (zone/tradeskills.cpp gates on 147510) while
-- the working one went unsold. A regenerator silently reverting a fix, exactly as elsewhere.
-- ⚠️ 147510 also sits outside every wholesale-delete band: gen_delve_augs.pl clears 147600-148199 and
-- aotv4_gear_tiers.sql clears 300000-899999 and 1000000-2999999.
-- ⚠️ Keep in step with AOTV4_REFINE_BAG_ID (zone/tradeskills.cpp). One without the other gives a
-- crucible that either links correctly and refines nothing, or refines and links wrong.
--
-- Items live in SHARED MEMORY: after applying this you must rebuild it with world DOWN:
--   stop world+zones -> `cd build/bin && ./shared_memory` -> restart world (zones reboot on demand).

-- Clone a known-good bagtype-30 (AlwaysWorks / shows a Combine button) 10-slot container, then override
-- identity + make the bag hold Giant items so any gear fits.
DROP TEMPORARY TABLE IF EXISTS tmp_crucible;
CREATE TEMPORARY TABLE tmp_crucible AS SELECT * FROM items WHERE id = 17033;
UPDATE tmp_crucible SET
	id       = 147510,   -- ⚠️ must stay < 0x100000 or the chat link renders junk; see the header
	name     = 'Refining Crucible',
	icon     = 1016,   -- Gigantic Velium Crucible icon
	itemclass = 1,     -- container
	bagtype  = 30,     -- AlwaysWorks quest container -> shows the Combine button, works anywhere
	bagslots = 4,      -- 4 slots (you place 4 items to refine)
	bagsize  = 4,      -- Giant: holds any gear
	bagwr    = 0,      -- no weight reduction (carries the full weight of its 4 items)
	nodrop   = 1,      -- droppable/tradeable (1 = NOT no-drop)
	norent   = 255,    -- never rent-expires
	loregroup = -1,    -- LORE: only one Refining Crucible per character
	stackable = 0,
	price    = 500,    -- 5 gold base (merchant markup ~5% -> shows ~5g at a vendor)
	weight   = 0;
-- ⚠️ Sweep the OLD id too, so a re-run on a database that predates v53 leaves exactly one crucible
-- rather than a working one plus a broken lookalike that merchants might still reference.
DELETE FROM items WHERE id IN (147510, 2000060);
INSERT INTO items SELECT * FROM tmp_crucible;
DROP TEMPORARY TABLE tmp_crucible;

-- Sell the crucible at every general-goods / satchel vendor -- i.e. every merchant that already sells a
-- container/bag (itemclass = 1). Added at each merchant's next free slot; min/max_expansion default -1
-- so it shows regardless of the era lock. Idempotent (clears any prior crucible rows first).
-- merchantlist loads at ZONE BOOT -> restart zones after (a world restart reboots them).
-- ⚠️ Clears the OLD id as well, so a pre-v53 database does not keep selling the unlinkable copy
-- alongside the new one.
DELETE FROM merchantlist WHERE item IN (147510, 2000060);
INSERT INTO merchantlist (merchantid, slot, item)
	SELECT ml.merchantid, MAX(ml.slot) + 1, 147510
	FROM merchantlist ml
	WHERE ml.merchantid IN (
		SELECT DISTINCT m.merchantid FROM merchantlist m JOIN items i ON m.item = i.id WHERE i.itemclass = 1
	)
	GROUP BY ml.merchantid;
