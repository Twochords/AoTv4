-- AoTv4 PoK travel: ungate the Plane of Knowledge book doors so they spawn on a Classic-gated
-- server (Expansion:CurrentExpansion=0). The books are PoP-era (min_expansion 4/7/9) and would be
-- filtered out, but we use them only as DISCOVERY markers (event_click_door discovers the zone and
-- cancels the teleport -- see quests/global/global_player.lua + lua_modules/pok_travel.lua), so
-- they must always be present. Doors are loaded from the DB at zone boot -> restart zones after.
UPDATE doors SET min_expansion = -1, max_expansion = -1 WHERE dest_zone = 'poknowledge';
