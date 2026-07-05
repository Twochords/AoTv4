# AoTv4 custom overlay

`.devcontainer/repo/` is **bootstrapped** by `.devcontainer/Makefile` (it `git clone`s
`quests` from ProjectEQ and `eq-core-dll` from xackery, and downloads `maps`). Those are
nested git repos, so the project's own customizations can't be tracked there from this repo.

This directory holds **only AoTv4's own source**, mirroring the `repo/` layout. `make prep`
copies it **over** the fresh clones, so a clean checkout + `make prep` reproduces the server:

```
custom/quests/global/global_player.lua          -- Bard-only + level-up reward + skill auto-grant + SKILLUNLOCKDATA
custom/quests/lua_modules/spell_choice.lua       -- reward picker core
custom/quests/lua_modules/spell_pool.lua         -- generated: level-indexed classic-era spell pool
custom/quests/lua_modules/spell_icons.lua        -- generated: spell id -> spellbook icon
custom/quests/lua_modules/skill_pool.lua         -- combat-ability reward pool
custom/quests/lua_modules/spell_blacklist.lua    -- generated: spells never offered (rez/enchant/curse/LDoN)
custom/eq-core-dll/src/*                          -- the dinput8.dll source (overlaid onto xackery/eq-core-dll)
```

Not here (tracked elsewhere or intentionally not committed):
- `special_attacks.cpp` cross-class ability patch -> `/src/zone/` (part of the EQEmu source).
- `maps/`, build artifacts, the upstream clone bodies -> regenerated, gitignored.

See the repo-root `CLAUDE.md` for how the whole system works. Regen commands for the
`*generated*` lua files are in CLAUDE.md §3/§5.
