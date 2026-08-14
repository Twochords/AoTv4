-- AoTv4 -- unlock the LDoN delve zones from the CLASSIC expansion gate.
-- Migration v27 (aotv4_unlock_ldon_delve_zones) only reset zone_regions.region_id=0 (the REGION half);
-- it left zone.expansion=6 (LDoN), so on a Classic server (Expansion:CurrentExpansion=0) these zones
-- are still content-filtered out -- "out of expansion". The regular delves (delvea etc.) are expansion=0.
-- ⚠️ zone list is in SHARED MEMORY (shared_memory 'LoadZones'), so this needs: world down -> ./shared_memory -> restart.
UPDATE zone SET expansion=0 WHERE short_name IN ('guka','gukc','guke','gukf','gukh','mira','mirb','mirc','mird','mirg','mirj','mmca','mmcb','mmcc','mmcd','mmce','mmcf','mmcg','mmch','mmci','mmcj','ruja','rujd','rujf','rujg','ruji','rujj','taka','takb','takc','takd','take','takg');
