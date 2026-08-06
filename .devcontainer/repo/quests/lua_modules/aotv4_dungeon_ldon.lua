-- aotv4_dungeon_ldon.lua  (GENERATED 2026-08-06 -- see the header of aotv4_dungeon.lua)
-- The LDoN half of the delve map pool. Kept in its own file purely for volume: 34 zones against
-- DoN's 6, and regenerating it should never risk touching hand-written delve logic.
--
-- ⚠️⚠️ ENTRY IS THE ZONE SAFE POINT, NOT dynamic_zone_templates.zone_in_*. Only 7 LDoN (zone,version)
-- pairs have template coordinates at all, so the DoN approach does not carry over. That is SAFE HERE
-- and would not be in DoN: an LDoN zone is a dungeon you could only ever reach through the adventure
-- system, so its safe point IS the normal zone-in. Section 24's warning against safe points is about
-- DoN zones, where the safe point can sit outside the mission layout.
--
-- ⚠️ `versions` is the list of layouts that are actually POPULATED (>= 60 spawn points). One is drawn
-- per run, so the same dungeon plays differently across runs. Version 0 is legitimate for LDoN -- it
-- is just the default layout, not an "open world" set as it is in DoN.
--
-- ⚠️ `task` round-robins the six existing delve task families. With task_activities.zones cleared
-- (migration v22) an activity credits in ANY zone, so no LDoN-specific task rows are needed at all --
-- CheckZone returns true on an empty zone list (common/tasks.h).
-- ⚠️ `race` reuses only races already proven to load globally in the DoN set; an unloaded race renders
-- the warden as a plain humanoid with no error.
return {
  { zone="guka",  zoneid=229, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=101.0, y=-841.0, z=1.0, task=2000300, race=433, name="Deepest Guk: Cauldron of Lost Souls" },
  { zone="gukc",  zoneid=239, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-804.0, y=-372.0, z=96.0, task=2000301, race=432, name="Deepest Guk: Ancient Aqueducts" },
  { zone="guke",  zoneid=249, versions={50}, x=680.0, y=-1031.0, z=59.0, task=2000302, race=433, name="Deepest Guk: The Curse Reborn" },
  { zone="gukf",  zoneid=254, versions={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-714.0, y=550.0, z=32.0, task=2000303, race=432, name="Deepest Guk: Chapel of the Witnesses" },
  { zone="gukh",  zoneid=264, versions={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=834.0, y=-667.0, z=-92.0, task=2000304, race=433, name="Deepest Guk: Accursed Sanctuary" },
  { zone="mira",  zoneid=232, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=649.0, y=564.0, z=-89.0, task=2000305, race=432, name="Miragul's Menagerie: Silent Gallery" },
  { zone="mirb",  zoneid=237, versions={50}, x=607.0, y=1504.0, z=28.0, task=2000300, race=433, name="Miragul's Menagerie: Frozen Nightmare" },
  { zone="mirc",  zoneid=242, versions={0, 1, 2, 3, 4}, x=-769.0, y=722.0, z=-186.0, task=2000301, race=432, name="Miragul's Menagerie: Spider Den" },
  { zone="mird",  zoneid=247, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=228.0, y=-457.0, z=2.0, task=2000302, race=433, name="Miragul's Menagerie: Hushed Banquet" },
  { zone="mirg",  zoneid=262, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=434.0, y=-15.0, z=56.0, task=2000303, race=432, name="Miragul's Menagerie: Heart of the Menagerie" },
  { zone="mirj",  zoneid=275, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=1153.0, y=-901.0, z=28.0, task=2000304, race=433, name="Miragul's Menagerie: Grand Library" },
  { zone="mmca",  zoneid=233, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}, x=-594.0, y=-365.0, z=6.0, task=2000305, race=432, name="Mistmoore's Catacombs: Forlorn Caverns" },
  { zone="mmcb",  zoneid=238, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-522.0, y=-22.0, z=23.0, task=2000300, race=433, name="Mistmoore's Catacombs: Dreary Grotto" },
  { zone="mmcc",  zoneid=243, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 50}, x=-424.0, y=-108.0, z=2.0, task=2000301, race=432, name="Mistmoore's Catacombs: Struggles within the Progeny" },
  { zone="mmcd",  zoneid=248, versions={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}, x=-144.0, y=-647.0, z=1.0, task=2000302, race=433, name="Mistmoore's Catacombs: Chambers of Eternal Affliction" },
  { zone="mmce",  zoneid=253, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}, x=-605.0, y=372.0, z=1.0, task=2000303, race=432, name="Mistmoore's Catacombs: Sepulcher of the Damned" },
  { zone="mmcf",  zoneid=258, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50}, x=-184.0, y=399.0, z=-12.0, task=2000304, race=433, name="Mistmoore's Catacombs: Scion Lair of Fury" },
  { zone="mmcg",  zoneid=263, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=427.0, y=413.0, z=4.0, task=2000305, race=432, name="Mistmoore's Catacombs: Cesspits of Putrescence" },
  { zone="mmch",  zoneid=268, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-367.0, y=-323.0, z=17.0, task=2000300, race=433, name="Mistmoore's Catacombs: Aisles of Blood" },
  { zone="mmci",  zoneid=272, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=589.0, y=-275.0, z=4.0, task=2000301, race=432, name="Mistmoore's Catacombs: Halls of Sanguinary Rites" },
  { zone="mmcj",  zoneid=276, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=258.0, y=548.0, z=4.0, task=2000302, race=433, name="Mistmoore's Catacombs: Infernal Sanctuary" },
  { zone="ruja",  zoneid=230, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=805.0, y=-123.0, z=-95.0, task=2000303, race=432, name="The Rujarkian Hills: Bloodied Quarries" },
  { zone="rujd",  zoneid=245, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50}, x=-322.0, y=1254.0, z=-96.0, task=2000304, race=433, name="The Rujarkian Hills: Prison Break" },
  { zone="rujf",  zoneid=255, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-290.0, y=-571.0, z=-460.0, task=2000305, race=432, name="The Rujarkian Hills: Fortified Lair of the Taskmasters" },
  { zone="rujg",  zoneid=260, versions={50}, x=238.0, y=-1163.0, z=130.0, task=2000300, race=433, name="The Rujarkian Hills: Hidden Vale of Deceit" },
  { zone="ruji",  zoneid=269, versions={0}, x=833.0, y=-1871.0, z=-222.0, task=2000301, race=432, name="The Rujarkian Hills: Arena of Chance" },
  { zone="rujj",  zoneid=273, versions={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=750.0, y=-134.0, z=26.0, task=2000302, race=433, name="The Rujarkian Hills: Barracks of War" },
  { zone="taka",  zoneid=231, versions={0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50}, x=-77.0, y=493.0, z=3.0, task=2000303, race=432, name="Takish-Hiz: Sunken Library" },
  { zone="takb",  zoneid=236, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=380.0, y=-544.0, z=7.0, task=2000304, race=433, name="Takish-Hiz: Shifting Tower" },
  { zone="takc",  zoneid=241, versions={0}, x=251.0, y=33.0, z=3.0, task=2000305, race=432, name="Takish-Hiz: Within the Compact" },
  { zone="takd",  zoneid=246, versions={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-282.0, y=133.0, z=7.0, task=2000300, race=433, name="Takish-Hiz: Royal Observatory" },
  { zone="take",  zoneid=251, versions={0, 1}, x=375.0, y=-406.0, z=19.0, task=2000301, race=432, name="Takish-Hiz: River of Recollection" },
  { zone="takg",  zoneid=261, versions={1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, x=-214.0, y=234.0, z=22.0, task=2000302, race=433, name="Takish-Hiz: Balancing Chamber" },
  { zone="veksar", zoneid=109, versions={0}, x=1.0, y=-486.0, z=-27.0, task=2000303, race=432, name="Veksar" },
}
