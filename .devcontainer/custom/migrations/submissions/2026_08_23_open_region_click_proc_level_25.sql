-- @aotv4-migration
-- description: 2026_08_23_open_region_click_proc_level_25
-- check: SELECT IF(COUNT(*) > 0, 'pending', 'done') FROM `items` WHERE (`id` = 4519 AND `clicklevel2` > 25) OR (`id` = 1154 AND `proclevel2` > 25)
-- condition: match
-- match: pending
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: Lower click (clicklevel2) and melee-proc (proclevel2) required levels to 25 for items obtainable in the OPEN regions (1-6), so they are usable under the level-30 cap. Tolan's Darkwood etc. gated at 45; many proc weapons (Frostreaver 50, Swarmcaller/Smolder 46) gated above cap = the proc could never fire.
-- notes: Covers ALL THREE tiers automatically via `id MOD 300000 IN (open-region base ids)`: native (<300000), Hallowed (+300000), Mythic (+600000) all share the base's clicklevel2/proclevel2, and the tier band [300000,900000) is reserved for tiers (base ids top at 147494), so there is no false match. `id < 900000` bounds it.
-- notes: The obtainable set is DERIVED from loottables + merchants in region 1-6 zones, not hardcoded, so it re-applies correctly against a fresh/imported DB. Scoped to open regions by owner request; region 0 (delve maps, loot-stripped) and 99 (unused) are excluded.
-- notes: Proxy check on id 4519 (Cobalt Gauntlets, clicky) + 1154 (Sceptre of Destruction, proc) -- both are IN the open-region set, unlike Tolan's Darkwood (drops outside 1-6). The migration is atomic, so those two fixed == all fixed; an import reverts them to their stock levels and re-triggers it.
-- notes: items is shared memory -- applying needs ./shared_memory + a restart (CLAUDE.md section 57).

UPDATE `items` SET `clicklevel2` = 25
WHERE `clickeffect` > 0 AND `clicklevel2` > 25 AND `id` < 900000
  AND (`id` MOD 300000) IN (
    SELECT iid FROM (
      SELECT DISTINCT lde.item_id AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN loottable_entries lte ON lte.loottable_id = n.loottable_id
        JOIN lootdrop_entries  lde ON lde.lootdrop_id  = lte.lootdrop_id
       WHERE zr.region_id BETWEEN 1 AND 6
      UNION
      SELECT DISTINCT ml.item AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN merchantlist ml ON ml.merchantid = n.merchant_id
       WHERE zr.region_id BETWEEN 1 AND 6
    ) open_region_items
  );

UPDATE `items` SET `proclevel2` = 25
WHERE `proceffect` > 0 AND `proclevel2` > 25 AND `id` < 900000
  AND (`id` MOD 300000) IN (
    SELECT iid FROM (
      SELECT DISTINCT lde.item_id AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN loottable_entries lte ON lte.loottable_id = n.loottable_id
        JOIN lootdrop_entries  lde ON lde.lootdrop_id  = lte.lootdrop_id
       WHERE zr.region_id BETWEEN 1 AND 6
      UNION
      SELECT DISTINCT ml.item AS iid
        FROM zone_regions zr
        JOIN zone z        ON z.zoneidnumber = zr.zone_id AND z.version = 0
        JOIN spawn2 s      ON s.zone = z.short_name
        JOIN spawnentry se ON se.spawngroupID = s.spawngroupID
        JOIN npc_types n   ON n.id = se.npcID
        JOIN merchantlist ml ON ml.merchantid = n.merchant_id
       WHERE zr.region_id BETWEEN 1 AND 6
    ) open_region_items
  );
