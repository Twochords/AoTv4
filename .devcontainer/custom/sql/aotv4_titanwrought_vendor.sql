-- GENERATED-adjacent, hand-authored: stock the hub crafting vendor for mold crafting.
-- Audri_Deepfacet (npc 202069, merchant 202069) is the hub crafting supplier v149 already used.
--
-- ⚠️ WHY THE RAW COMPONENTS ARE SOLD HERE: the three base materials below are the only inputs
--    to all 45 material sub-recipes, and stock EQ leaves them DROP ONLY -- Silk Swatch has 2
--    lootdrop rows, Block of Ore has 3, and neither is on a single merchant anywhere. A crafting
--    system whose entry components cannot be bought is gated behind two loot tables. Water Flask
--    (352 merchants) and Peridot (132) were already fine and are not touched.
-- ⚠️ THE TEMPERS ARE THE TIER GATE AND MUST BE BOUGHT, NOT DROPPED (design §3): their price IS
--    the per-tier cost ladder -- 100p / 300p / 500p per combine.
DELETE FROM merchantlist WHERE merchantid=202069 AND slot BETWEEN 144 AND 149;
INSERT INTO merchantlist (merchantid,slot,item,faction_required,level_required,alt_currency_cost,
                          classes_required,probability)
VALUES (202069,144,16482,-100,0,0,65535,100),   -- Silk Swatch  -> cloth bases
       (202069,145,97860,-100,0,0,65535,100),   -- Raw Hide     -> leather bases
       (202069,146,10503,-100,0,0,65535,100),   -- Block of Ore -> chain/plate/weapon bases
       (202069,147,148515,-100,0,0,65535,100),  -- Crude Temper   100p
       (202069,148,148531,-100,0,0,65535,100),  -- Simple Temper  300p
       (202069,149,148547,-100,0,0,65535,100);  -- Rough Temper   500p
