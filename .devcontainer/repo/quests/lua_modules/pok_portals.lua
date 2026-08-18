-- AUTO-GENERATED: PoK-book travel table (zones with a poknowledge book).
-- pok_portals[short] = {id=zoneid, x=,y=,z=,h=, long="Long Name"}
-- landing = the PoK-side RETURN door's dest when one exists (game-tuned arrival, ~near the book);
-- otherwise (no return book to that zone) = IN FRONT OF the zone's own book, facing it.
--
-- ⚠️ Two hand-maintained rules (keep them if you regenerate):
--  1) No-return-door zones must land relative to the zone's `doors` book pos (NOT the zone safe point,
--     which is map-center in several zones).
--     ⚠️⚠️ LAND IN FRONT OF THE BOOK, NEVER ON IT. `doors.pos_x/y/z` is the BOOK OBJECT'S OWN
--     POSITION, so using it verbatim materialises the player INSIDE the book model -- reported from
--     play as "they are ending up in the book". Seven entries did exactly this (bazaar, freportw,
--     innothule, shadowrest, steamfont, tox, weddingchapel); felwithe did it too despite having a
--     perfectly good return door to use.
--     The offset is 30 units along the direction the book FACES, and the arrival heading is the
--     book's heading + 256 so the player looks back at it:
--         x = book_x + 30*sin(h/512*2*pi),  y = book_y + 30*cos(h/512*2*pi),  h = (book_h+256)%512
--     ⚠️ That formula is not guesswork -- it is derived from the game's OWN tuned arrivals and checked
--     against three independent book/dest pairs (Kelethin 28u, Firiona 44u, Felwithe 38u). It is also
--     the pattern butcherdocks, ecommons and qeytoqrg were already hand-fixed to use.
--     ⚠️⚠️ A BOOK WHOSE `heading` IS 0 is the awkward case -- 0 is indistinguishable from "never set",
--     so the direction carries little confidence. freportw and steamfont are both like this.
--     steamfont is outdoors with NPC paths all around, so it aims 30u at the nearest NPC spawn and
--     faces back at the book: an NPC stands on walkable ground by definition, so that direction is
--     open by evidence rather than assumption.
--     ⚠️⚠️ freportw is the OPPOSITE case and must NOT be treated the same way. Its book is SEALED IN
--     A ROOM: there is no grid waypoint within 125u, and nothing walkable at all within 539u AT THE
--     BOOK'S OWN z (-34.8) -- everything nearby sits ~10u higher, i.e. outside and above. So "aim at
--     the nearest walkable ground" points through the room's WALL, and the bigger the offset the more
--     certainly it lands in it. It uses a SMALL 15u step along the book's own stated facing instead:
--     enough to clear the book model (a lectern is ~5-8u), little enough to stay inside a small room.
--     ⚠️ Keep the BOOK's z in both cases. The book is at a valid standing height by definition -- you
--     have to stand at it to click it -- whereas borrowing a nearby spawn's z would drop the player
--     10u above the floor in freportw.
--     📌 freportw is the ONLY one of the six region books (see aotv4_regions.M.BOOK) whose landing is
--     not game-authored; the other five use the PoK-side tuned dest. It is the one to spot check.
--  2) EXCLUDE era-duplicate zone versions. Several zones have a classic AND a revamped version that share
--     a display name (e.g. West Freeport = freportw/freeportwest). On this Classic server only the classic
--     version is the active zone, and listing both produces DOUBLE links in the Portal window. Dropped the
--     revamped/duplicate shorts: freeportwest(383), steamfontmts(448), feerrott2(700), mistythicket(415),
--     innothuleb(413), toxxulia(414), weddingchapeldark(494).
return {
  ["arena"]={id=77,x=147.04,y=-1014.25,z=48.00,h=256,long="The Arena"},
  ["bazaar"]={id=151,x=-471.21,y=1.76,z=-5.35,h=132,long="The Bazaar"},                        -- no return door; 30u in front of book POK_DOOR, facing it
  ["butcher"]={id=68,x=-523.00,y=1726.00,z=-1.00,h=45,long="Kaladim"},                          -- butcher book doorid 78 (by the Kaladim zone line)
  ["butcherdocks"]={id=68,x=2934.99,y=1238.97,z=-2.26,h=356,long="Butcherblock Docks"},           -- SECOND butcher book, doorid 179 (the docks); land ~35u inland of the book, facing it
  ["crescent"]={id=394,x=-2635.00,y=-1240.00,z=-150.60,h=149,long="Crescent Reach"},
  ["freeporttheater"]={id=390,x=-71.56,y=-246.67,z=-27.10,h=0,long="Theater of the Tranquil"},    -- ARTISAN HUB. ⚠️ NOT derived from the book: this is the zone's AUTHORED arrival point, shared
                                                                                                 -- with the Origin AA (spell 5824) so every way in lands in the same place.
  ["ecommons"]={id=22,x=-196.96,y=-1526.81,z=3.13,h=0,long="Commonlands"},                       -- book doorid 71; land just S of the (south-facing) book, facing N at it
  ["everfrost"]={id=30,x=-31.00,y=2835.00,z=-62.00,h=453,long="Everfrost Peaks"},
  ["feerrott"]={id=47,x=-163.00,y=908.00,z=-9.00,h=248,long="The Feerrott"},
  ["fieldofbone"]={id=78,x=1845.00,y=-2980.00,z=11.00,h=259,long="The Field of Bone"},
  ["firiona"]={id=84,x=4673.00,y=-455.00,z=9.00,h=128,long="Firiona Vie"},
  ["freportw"]={id=9,x=77.37,y=-667.06,z=-34.80,h=256,long="West Freeport"},                   -- no return door; ONLY 15u off the book (+Y, its stated heading), facing it. See the header note: this book is in a SEALED ROOM, so a big offset is riskier than a small one. ⚠️ REGION 2's hub and the one landing here with no game-authored source -- spot check
  ["gfaydark"]={id=54,x=-734.00,y=-188.00,z=-3.00,h=430,long="Kelethin"},                     -- gfaydark book by Kelethin (doorid 109)
  ["felwithe"]={id=54,x=-1824.00,y=-2223.00,z=-1.00,h=244,long="Felwithe"},                    -- SECOND gfaydark book, by the Felwithe zone line (doorid 108). ⚠️ This DOES have a return door (POKFELPORT500) and was landing on the book anyway -- now the game-tuned dest, per rule 1
  ["greatdivide"]={id=118,x=-1813.22,y=0.00,z=393.44,h=0,long="The Great Divide"},
  ["guildlobby"]={id=344,x=18.00,y=-46.00,z=6.00,h=492,long="Guild Lobby"},
  ["gunthak"]={id=224,x=-1030.00,y=1780.00,z=60.00,h=0,long="The Gulf of Gunthak"},
  ["innothule"]={id=46,x=-42.41,y=-701.83,z=-29.22,h=221,long="Innothule Swamp"},              -- no return door; 30u in front of book POKTELE500, facing it
  ["misty"]={id=33,x=-1262.71,y=-546.00,z=8.00,h=2,long="Misty Thicket"},
  ["nektulos"]={id=25,x=-840.00,y=-809.00,z=9.00,h=999,long="The Nektulos Forest"},
  ["nexus"]={id=152,x=442.00,y=48.00,z=-29.00,h=388,long="Nexus"},
  ["overthere"]={id=93,x=1888.00,y=3133.00,z=-51.00,h=128,long="The Overthere"},
  ["potranquility"]={id=203,x=-1463.00,y=774.00,z=-878.00,h=131,long="The Plane of Tranquility"},
  ["qeynos2"]={id=2,x=487.00,y=219.00,z=2.00,h=267,long="North Qeynos"},
  ["qeytoqrg"]={id=4,x=75.87,y=3162.71,z=1.23,h=361,long="Qeynos Hills"},                         -- book doorid 14 (moved, faces E to Blackburrow); land just W of it, facing the book
  ["rathemtn"]={id=50,x=309.50,y=-1166.00,z=-0.50,h=34,long="The Rathe Mountains"},
  ["shadeweaver"]={id=165,x=-2433.00,y=-2970.00,z=-215.00,h=236,long="Shadeweaver's Thicket"},
  ["shadowrest"]={id=187,x=-38.38,y=-244.33,z=4.09,h=128,long="Shadowrest"},                   -- no return door; 30u in front of book POKTELE500, facing it
  ["steamfont"]={id=56,x=910.68,y=-1353.91,z=-111.66,h=184,long="Steamfont Mountains"},        -- no return door; book POKTELE500 heading is 0 (unusable), so 30u toward the nearest walkable ground instead, facing back at the book
  ["tox"]={id=38,x=266.26,y=-2348.90,z=-49.75,h=115,long="Toxxulia Forest"},                   -- no return door; 30u in front of book POKTELE500, facing it
  ["weddingchapel"]={id=493,x=-124.75,y=0.00,z=2.25,h=128,long="Wedding Chapel"},              -- no return door; 30u in front of book OBJ_DOORENT, facing it. ⚠️ small interior -- spot check
}
