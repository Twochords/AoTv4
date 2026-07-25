-- AoTv4 Epics (category 7) — one achievement per class for the epic 1.0 weapon.
-- Completes via the new `item_receive` objective type: a hook fires when the epic item
-- enters inventory (Client::SummonItem / PushItemOnCursor) and RecheckAutomatic backstops
-- for already-owned epics. class_mask gates each to its class (ProcessMatchedObjectives).
-- Item ids resolved from OUR items DB (the actual weapons that exist here).
-- Achievement ids 710001-710016 (700001-700448 is Zone Slayer — do NOT use that range).

INSERT INTO `custom_achievements`
  (`id`,`category_id`,`slug`,`name`,`description`,`points`,`hidden`,`enabled`,`sort_order`,`created_at`)
VALUES
  (710001,7,'epic10-warrior'     ,'Epic 1.0: Warrior'     ,'Obtain the Warrior epic, Jagged Blade of War.'            ,50,0,1,1 ,1785000000),
  (710002,7,'epic10-cleric'      ,'Epic 1.0: Cleric'      ,'Obtain the Cleric epic, Water Sprinkler of Nem Ankh.'     ,50,0,1,2 ,1785000000),
  (710003,7,'epic10-paladin'     ,'Epic 1.0: Paladin'     ,'Obtain the Paladin epic, Fiery Defender.'                 ,50,0,1,3 ,1785000000),
  (710004,7,'epic10-ranger'      ,'Epic 1.0: Ranger'      ,'Obtain the Ranger epic, Earthcaller.'                     ,50,0,1,4 ,1785000000),
  (710005,7,'epic10-shadowknight','Epic 1.0: Shadowknight','Obtain the Shadowknight epic, Innoruuk''s Curse.'         ,50,0,1,5 ,1785000000),
  (710006,7,'epic10-druid'       ,'Epic 1.0: Druid'       ,'Obtain the Druid epic, Nature Walker''s Scimitar.'        ,50,0,1,6 ,1785000000),
  (710007,7,'epic10-monk'        ,'Epic 1.0: Monk'        ,'Obtain the Monk epic, Celestial Fists.'                   ,50,0,1,7 ,1785000000),
  (710008,7,'epic10-bard'        ,'Epic 1.0: Bard'        ,'Obtain the Bard epic, Singing Short Sword.'               ,50,0,1,8 ,1785000000),
  (710009,7,'epic10-rogue'       ,'Epic 1.0: Rogue'       ,'Obtain the Rogue epic, Ragebringer.'                      ,50,0,1,9 ,1785000000),
  (710010,7,'epic10-shaman'      ,'Epic 1.0: Shaman'      ,'Obtain the Shaman epic, Spear of Fate.'                   ,50,0,1,10,1785000000),
  (710011,7,'epic10-necromancer' ,'Epic 1.0: Necromancer' ,'Obtain the Necromancer epic, Scythe of the Shadowed Soul.',50,0,1,11,1785000000),
  (710012,7,'epic10-wizard'      ,'Epic 1.0: Wizard'      ,'Obtain the Wizard epic, Staff of the Four.'               ,50,0,1,12,1785000000),
  (710013,7,'epic10-magician'    ,'Epic 1.0: Magician'    ,'Obtain the Magician epic, Orb of Mastery.'                ,50,0,1,13,1785000000),
  (710014,7,'epic10-enchanter'   ,'Epic 1.0: Enchanter'   ,'Obtain the Enchanter epic, Staff of the Serpent.'         ,50,0,1,14,1785000000),
  (710015,7,'epic10-beastlord'   ,'Epic 1.0: Beastlord'   ,'Obtain the Beastlord epic, Claw of the Savage Spirit.'    ,50,0,1,15,1785000000),
  (710016,7,'epic10-berserker'   ,'Epic 1.0: Berserker'   ,'Obtain the Berserker epic, Kerasian Axe of Ire.'          ,50,0,1,16,1785000000)
ON DUPLICATE KEY UPDATE
  category_id=VALUES(category_id), name=VALUES(name), description=VALUES(description),
  points=VALUES(points), enabled=VALUES(enabled), sort_order=VALUES(sort_order);

-- objective per class: item_receive on the epic item, class_mask = 1<<(class-1)
INSERT INTO `custom_achievement_objectives`
  (`id`,`achievement_id`,`objective_index`,`objective_type`,`target_type`,`target_id`,`target_name`,`required_count`,`zone_id`,`class_mask`)
VALUES
  (71000001,710001,0,'item_receive','item',10908,'Jagged Blade of War'        ,1,0,1),
  (71000002,710002,0,'item_receive','item',5532 ,'Water Sprinkler of Nem Ankh',1,0,2),
  (71000003,710003,0,'item_receive','item',10099,'Fiery Defender'             ,1,0,4),
  (71000004,710004,0,'item_receive','item',20488,'Earthcaller'                ,1,0,8),
  (71000005,710005,0,'item_receive','item',14383,'Innoruuk''s Curse'          ,1,0,16),
  (71000006,710006,0,'item_receive','item',20490,'Nature Walkers Scimitar'    ,1,0,32),
  (71000007,710007,0,'item_receive','item',1683 ,'Celestial Fists'            ,1,0,64),
  (71000008,710008,0,'item_receive','item',20542,'Singing Short Sword'        ,1,0,128),
  (71000009,710009,0,'item_receive','item',11057,'Ragebringer'                ,1,0,256),
  (71000010,710010,0,'item_receive','item',10651,'Spear of Fate'              ,1,0,512),
  (71000011,710011,0,'item_receive','item',20544,'Scythe of the Shadowed Soul',1,0,1024),
  (71000012,710012,0,'item_receive','item',14341,'Staff of the Four'          ,1,0,2048),
  (71000013,710013,0,'item_receive','item',28034,'Orb of Mastery'             ,1,0,4096),
  (71000014,710014,0,'item_receive','item',10650,'Staff of the Serpent'       ,1,0,8192),
  (71000015,710015,0,'item_receive','item',8495 ,'Claw of the Savage Spirit'  ,1,0,16384),
  (71000016,710016,0,'item_receive','item',68299,'Kerasian Axe of Ire'        ,1,0,32768)
ON DUPLICATE KEY UPDATE
  objective_type=VALUES(objective_type), target_id=VALUES(target_id),
  target_name=VALUES(target_name), class_mask=VALUES(class_mask);
