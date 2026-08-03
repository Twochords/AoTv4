local helper = {}

-- ⚠️⚠️ THE HARDCODED zone_regions TABLE IS GONE -- `eq.get_zone_region(zone_id)` is the GetZoneRegion
-- the TODO asked for (added 2026-08-02: zone/lua_general.cpp -> RegionManager::GetZoneRegion, which
-- already existed in C++ and simply had no Lua binding).
-- The `zone_regions` DB table has **482 rows** and is loaded at zone boot; a six-row hand copy in a
-- quest module is silent drift waiting to happen -- re-point one zone in the table and the copy keeps
-- sending people somewhere they cannot go, or blocking somewhere they can.
--
-- Region ids, for reading the answer:
--     0   "Always Available"  -- 7 zones, no unlock needed
--     1-6 the unlockable regions (Kelethin, Freeport, Thurgadin, Firiona Vie, Qeynos, Cabilis)
--     99  "Unused"            -- 412 zones that are not part of the region system at all
--
-- ⚠️ A zone missing from the table returns 0, which is the same answer RegionManager::CanEnterZone
-- gives it: unrestricted by design. Do not treat 0 as an error or as a denial.
-- ⚠️ Erud's Crossing (98) maps to 99 "Unused" and nobody is ever granted region 99, so it is refused.
-- That is the table's answer rather than a bug here; fix it in `zone_regions` if it should be open.
--
-- ⚠️⚠️ `local function`, not a bare global. `function can_zone(...)` leaks into the shared Lua state
-- and collides with every other quest script that ever defines that name.
local function can_zone(e, zone_id)
    local region_id = eq.get_zone_region(zone_id) or 0

    if region_id == 0 then
        return true    -- always available, or a zone outside the region system
    end

    return e.other:HasRegion(region_id)
end

function helper.hail_text(e, zone, location)
    if e.message:findi('hail') then
        e.self:Say("Greetings ".. e.other:GetCleanName() ..", I am ".. e.self:GetCleanName() ..". I have been sent here by the Gnomish Academy of Science to assist you in traveling the oceans of norrath. If you would like to [" ..eq.say_link("Travel to " .. zone .." ") .."] please let me know.");
    elseif e.message:findi("Travel to ".. zone) then
        if(can_zone(e, location.zone)) then
            e.self:Say("Beaming you up!");
            e.other:MovePC(location.zone, location.x, location.y, location.z, location.heading);
        else
            e.self:Say("You have not unlocked this region yet. I cannot possibly take you there.");
        end
    end
end

return helper;