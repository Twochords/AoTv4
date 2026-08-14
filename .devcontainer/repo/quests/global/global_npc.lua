local aotv4_worldboss = require("aotv4_worldboss")  -- roaming world boss: loot rights on death
local aotv4_dungeon = require("aotv4_dungeon")  -- scaling dungeon ("Delve")

function event_spawn(e)
    -- Delve: scale this mob to the layer's level if it is spawning inside a delve instance.
    -- ⚠️ FIRST THING IN THE HOOK and before any of the seasonal string matching below: it
    -- early-outs on the instance id, so the normal world pays one integer compare for it.
    aotv4_dungeon.on_npc_spawn(e)

    -- World difficulty: bump level and health inside a Nightmare/Hell/Inferno shard. Early-outs on
    -- a cached instance id, so the open world pays one integer compare.
    -- ⚠️ AFTER the delve hook and harmless beside it: a delve instance carries no difficulty
    -- marker, so this reads 0 there and does nothing.
    require("aotv4_difficulty").on_npc_spawn(e)

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

-- ⚠️ There is deliberately NO event_damage_given here any more. It existed solely to drive the
-- summoned-pet behaviours of the retired 43000-43112 ability set (Leopard backstab, Skeleton
-- lifetap-heal, Willowisp mana leech, Fire Imp burn) and did nothing else, so it was removed with
-- them. The player-side damage hooks (Thirst, Sinew, reactions) live in global_player.lua and are
-- untouched -- do not re-add this one expecting to find them here.

-- AoTv4: the roaming world boss can die in any zone, so its loot rights are granted from the global
-- NPC hook rather than a per-zone script. Everyone on its hate list may loot -- see
-- lua_modules/aotv4_worldboss.lua.
function event_death_complete(e)
  aotv4_worldboss.on_death(e)

  -- Ink of the Lost, granted straight as currency rather than dropped as an item (section 29).
  -- ⚠️⚠️ `e.other` IS A Lua_Mob, NOT A Lua_Client -- IsClient() is true on it but CharacterID and the
  -- currency bindings are only defined on Lua_Client, so calling them directly is "attempt to call
  -- method (a nil value)" on EVERY kill. Resolve through the entity list, exactly as the delve's
  -- M.as_client does (section 24 records this costing a whole system once already).
  if e.other and e.other.valid and e.other:IsClient() then
    local killer = eq.get_entity_list():GetClientByID(e.other:GetID())
    if killer and killer.valid then
      require("aotv4_spell_ranks_sys").grant_ink_on_kill(killer)

      -- Tomes of Insight: an extra reward pick, dropped only in the harder world difficulties and
      -- only off creatures that con white or better. Shares the killer lookup above rather than
      -- repeating it -- this hook runs on every death on the server.
      -- ⚠️ Returns immediately on Normal, so a player who never leaves the ordinary world pays one
      -- bucket read per kill and nothing else.
      require("aotv4_spell_books").on_npc_death(e.self, killer)

      -- Hell and Inferno: the corpse may get straight back up as a skeleton of itself, at half
      -- health, where it fell. ⚠️ Shares the killer lookup, and is passed it so the risen creature
      -- can start on the hate list of whoever put the first one down.
      require("aotv4_difficulty").on_npc_death(e, killer)
    end
  end

  -- Delve: bank what this creature was worth into the run's difficulty ledger. Keyed off the mob's
  -- OWN scaled level, stamped when it spawned, so no later change of gear can revalue it.
  aotv4_dungeon.on_npc_death(e)

  -- Delve: opening the reward chest ends the run and closes the instance.
  -- ⚠️ Keyed on the chest's npc type id, not on its name -- a_delve_reward_chest is a clone of
  -- a_gilded_chest and there are several similarly named chests in the world.
  -- ⚠️ e.other is a Lua_Mob; on_chest_looted needs a real Lua_Client (CharacterID is not on Lua_Mob).
  if e.self and e.self.valid and e.self:GetNPCTypeID() == aotv4_dungeon.CHEST_NPC then
    aotv4_dungeon.on_chest_looted(aotv4_dungeon.as_client(e.other))
  end
end
