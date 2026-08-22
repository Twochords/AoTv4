-- Warrior tier 3 -- Broad Cleave.
-- ⚠️ The payload is aotv4_class_abilities.PAYLOAD[44702]; this file only routes to it.
-- ⚠️⚠️ THE HANDLER IS `event_spell_effect`, WITH NO SUFFIX. LuaParser::ConvertLuaEvent
-- (lua_parser.cpp:1604) collapses EVENT_SPELL_EFFECT_BOT/_CLIENT/_NPC into one and the name table
-- maps all three to "event_spell_effect" (:104). A suffixed handler is never called -- the spell
-- still animates and applies its own SPAs, so the ability looks alive and does nothing.
-- ⚠️⚠️ THE SCRIPT RUNS **BEFORE** THE SPELL APPLIES ITS OWN EFFECTS (event at
-- spell_effects.cpp:163, slot loop at :225), and RETURNING NON-ZERO CANCELS THEM.
-- 📌 Warrior is NOT emitted by custom/tools/gen_class_abilities.py: it shipped as migration v104
-- before that generator existed, and regenerating it would renumber an applied migration.
local ab = require("aotv4_class_abilities")

function event_spell_effect(e) ab.fire(44702, e) end
