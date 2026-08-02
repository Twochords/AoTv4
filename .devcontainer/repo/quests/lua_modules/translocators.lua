local helper = {}
-- TODO: Replace this with a GetZoneRegion()
-- TODO: Replace this with a GetZoneRegion()
local zone_regions = {
    [98] = 99, -- Erud's Crossing -> Region 99
    [96] = 4,  -- Timorous -> Region 4
    [68] = 1,  -- Butcherblock -> Region 1
    [69] = 2,  -- Ocean of Tears -> Region 2
    [110] = 3, -- Iceclad -> Region 3
    [37] = 2,  -- Oasis -> Region 2
}

function can_zone(e, zone_id)
    local region_id = zone_regions[zone_id]

    if region_id then
        return e.other:HasRegion(region_id)
    end

    -- No region assigned, allow zoning by default
    return true
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