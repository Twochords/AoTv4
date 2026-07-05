-- AUTO-GENERATED: PoK-book travel table (zones with a poknowledge book).
-- pok_portals[short] = {id=zoneid, x=,y=,z=,h=, long="Long Name"}
-- landing = the PoK-side RETURN door's dest when one exists (game-tuned arrival, ~near the book);
-- otherwise (no return book to that zone) = the zone's OWN book location, so you always arrive AT a book.
--
-- ⚠️ Two hand-maintained rules (keep them if you regenerate):
--  1) No-return-door zones must land at the zone's `doors` book pos (NOT the zone safe point = map-center).
--  2) EXCLUDE era-duplicate zone versions. Several zones have a classic AND a revamped version that share
--     a display name (e.g. West Freeport = freportw/freeportwest). On this Classic server only the classic
--     version is the active zone, and listing both produces DOUBLE links in the Portal window. Dropped the
--     revamped/duplicate shorts: freeportwest(383), steamfontmts(448), feerrott2(700), mistythicket(415),
--     innothuleb(413), toxxulia(414), weddingchapeldark(494).
return {
  ["arena"]={id=77,x=147.04,y=-1014.25,z=48.00,h=256,long="The Arena"},
  ["bazaar"]={id=151,x=-441.25,y=0.29,z=-5.35,h=388,long="The Bazaar"},                        -- book POK_DOOR (no return door)
  ["butcher"]={id=68,x=-523.00,y=1726.00,z=-1.00,h=45,long="Butcherblock Mountains"},
  ["crescent"]={id=394,x=-2635.00,y=-1240.00,z=-150.60,h=149,long="Crescent Reach"},
  ["everfrost"]={id=30,x=-31.00,y=2835.00,z=-62.00,h=453,long="Everfrost Peaks"},
  ["feerrott"]={id=47,x=-163.00,y=908.00,z=-9.00,h=248,long="The Feerrott"},
  ["fieldofbone"]={id=78,x=1845.00,y=-2980.00,z=11.00,h=259,long="The Field of Bone"},
  ["firiona"]={id=84,x=4673.00,y=-455.00,z=9.00,h=128,long="Firiona Vie"},
  ["freportw"]={id=9,x=77.37,y=-682.06,z=-34.80,h=0,long="West Freeport"},                     -- book POKTELE500 (no return door)
  ["gfaydark"]={id=54,x=-734.00,y=-188.00,z=-3.00,h=430,long="The Greater Faydark"},
  ["greatdivide"]={id=118,x=-1813.22,y=0.00,z=393.44,h=0,long="The Great Divide"},
  ["guildlobby"]={id=344,x=18.00,y=-46.00,z=6.00,h=492,long="Guild Lobby"},
  ["gunthak"]={id=224,x=-1030.00,y=1780.00,z=60.00,h=0,long="The Gulf of Gunthak"},
  ["innothule"]={id=46,x=-29.99,y=-729.13,z=-29.22,h=477,long="Innothule Swamp"},              -- book POKTELE500 (no return door)
  ["misty"]={id=33,x=-1262.71,y=-546.00,z=8.00,h=2,long="Misty Thicket"},
  ["nektulos"]={id=25,x=-840.00,y=-809.00,z=9.00,h=999,long="The Nektulos Forest"},
  ["nexus"]={id=152,x=442.00,y=48.00,z=-29.00,h=388,long="Nexus"},
  ["overthere"]={id=93,x=1888.00,y=3133.00,z=-51.00,h=128,long="The Overthere"},
  ["potranquility"]={id=203,x=-1463.00,y=774.00,z=-878.00,h=131,long="The Plane of Tranquility"},
  ["qeynos2"]={id=2,x=487.00,y=219.00,z=2.00,h=267,long="North Qeynos"},
  ["rathemtn"]={id=50,x=309.50,y=-1166.00,z=-0.50,h=34,long="The Rathe Mountains"},
  ["shadeweaver"]={id=165,x=-2433.00,y=-2970.00,z=-215.00,h=236,long="Shadeweaver's Thicket"},
  ["shadowrest"]={id=187,x=-8.38,y=-244.33,z=4.09,h=384,long="Shadowrest"},                    -- book POKTELE500 (no return door)
  ["steamfont"]={id=56,x=933.79,y=-1373.04,z=-111.66,h=0,long="Steamfont Mountains"},          -- book POKTELE500 (no return door)
  ["tox"]={id=38,x=295.88,y=-2344.14,z=-49.75,h=371,long="Toxxulia Forest"},                   -- book POKTELE500 (no return door)
  ["weddingchapel"]={id=493,x=-94.75,y=0.00,z=2.25,h=384,long="Wedding Chapel"},               -- book OBJ_DOORENT (no return door)
}
