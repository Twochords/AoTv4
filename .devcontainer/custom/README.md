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
```

⚠️⚠️ **The dinput8.dll source is NOT here. It is `.devcontainer/repo/eq-core-dll/`.**
A partial 10-file snapshot used to sit at `custom/eq-core-dll/src/` and was removed on 2026-08-18: it
was 149 files behind the real tree and its `_options.h` still had `arePortalWindowEnabled = true`, so
building that vcxproj by mistake gave you the retired GDI portal overlay AND no fellowship, travel or
difficulty windows at all -- with nothing to indicate you had built the wrong tree. Its only unique
content was stale comments describing overlays that no longer exist.
📌 `.devcontainer/.gitignore` has `repo/`, so the real tree is invisible to this repo's `git status`
(CLAUDE.md section 0). That is what made a tracked copy here look authoritative.

Not here (tracked elsewhere or intentionally not committed):
- `special_attacks.cpp` cross-class ability patch -> `/src/zone/` (part of the EQEmu source).
- `maps/`, build artifacts, the upstream clone bodies -> regenerated, gitignored.
- the **dinput8.dll source** -> `.devcontainer/repo/eq-core-dll/` (its own git remote, own branch).

See the repo-root `CLAUDE.md` for how the whole system works. Regen commands for the
`*generated*` lua files are in CLAUDE.md §3/§5.
