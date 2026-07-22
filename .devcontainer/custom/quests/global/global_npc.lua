local aotv4_pets = require("aotv4_pets")  -- summon traits + pet behaviour

function event_spawn(e)
    aotv4_pets.on_pet_spawn(e.self)   -- Bear holds aggro

    -- peq_halloween
    if (eq.is_content_flag_enabled("peq_halloween")) then
        -- exclude mounts and pets
        if (e.self:GetCleanName():findi("mount") or e.self:IsPet()) then
            return;
        end

        -- soulbinders
        -- priest of discord
        if (e.self:GetCleanName():findi("soulbinder") or e.self:GetCleanName():findi("priest of discord")) then
            e.self:ChangeRace(eq.ChooseRandom(14,60,82,85));
            e.self:ChangeSize(6);
            e.self:ChangeTexture(1);
            e.self:ChangeGender(2);
        end

        -- Shadow Haven
        -- The Bazaar
        -- The Plane of Knowledge
        -- Guild Lobby
        local halloween_zones = eq.Set { 202, 150, 151, 344 }
        local not_allowed_bodytypes = eq.Set { 11, 60, 66, 67 }
        if (halloween_zones[eq.get_zone_id()] and not_allowed_bodytypes[e.self:GetBodyType()] == nil) then
            e.self:ChangeRace(eq.ChooseRandom(14,60,82,85));
            e.self:ChangeSize(6);
            e.self:ChangeTexture(1);
            e.self:ChangeGender(2);
        end
    end
end

-- Summoned-pet behaviours (Leopard backstab, Skeleton lifetap-heal, Willowisp
-- mana leech, Fire Imp burn). Fires for pets because NPCs get damage events too.
function event_damage_given(e)
  aotv4_pets.on_pet_damage(e.self, eq.get_entity_list():GetMobID(e.entity_id), e.damage, e.spell_id)
end
