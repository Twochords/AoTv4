# Zone Difficulty window (`/pick`) — install

Normal / Nightmare / Hell / Inferno. Difficulty is a property of the **character**, not a place:
pick one and it applies to every zone you enter from then on, by putting you in a private copy of
that zone shared with everyone else at the same difficulty. Normal is the real zone, no instance.

## Client

1. Rebuild `dinput8.dll` (`eq-core-dll-vs2022.vcxproj`). `core_difficulty.cpp` is a **new
   translation unit** and is already added to the project file — a new `.cpp` that is not listed
   there silently does not compile.
2. Copy to `<EQ>/uifiles/default/`:
   - `EQUI_AoTDifficultyWnd.xml` (new)
   - `EQUI_AoTMenuWnd.xml` (changed — grew a **Difficulty** button, 262 → 292 tall)
3. Add to `<EQ>/uifiles/default/EQUI.xml`:
   ```xml
   <Include>EQUI_AoTDifficultyWnd.xml</Include>
   ```
   ⚠️ Miss this and `CCustomWnd` cannot find the screen and returns **silently** — no error, no
   window. `<EQ>/aotv4_difficulty.log` is the tell: it logs `pXWnd=NULL` in exactly that case,
   and logs nothing at all if the dll was never rebuilt.

## Server

Lua only — no binary change:

- `lua_modules/aotv4_difficulty.lua` (new)
- `global/global_player.lua` (require, `event_say` routing, `event_enter_zone` hook)
- `global/global_npc.lua` (`event_spawn` hook)

Restart zones so the modules reload (`#reloadquest` does **not** reload `require`d modules).

## ⚠️ It replaces the native `/pick`, deliberately

The dll's `InterpretCmd` detour swallows `/pick` and never calls the trampoline, so the client's own
command never fires. That is the point, not a side effect:

- `PickZoneEntry_Struct` carries only `{zone_id, player_count, instance_id}` — **no text field** — so
  all four difficulties would render as the same zone name with different populations, and nothing
  on screen could say which was which.
- `Handle_OP_PickZone` is a bare stub server side (the whole body is `// handle`), and nothing in the
  codebase has ever sent `OP_PickZoneWindow`. The stock command does nothing here anyway.

Set `areDifficultyWindowEnabled = false` in `_options.h` to restore the stock (non-functional) one.

## ⚠️ Ports

Every non-Normal difficulty is an **instance**, and an occupied instance is a zone process on a zone
port. Three difficulties across several zones adds up fast, and the delve already burns one per run.

**Current state — not yet widened, and fine for testing.** The dev box is on **7000-7029** (30
ports) with `launcher.dynamics` = 28 for the `zone` launcher. A tester or two moving through a few
zones stays well inside that; a populated server will not.

⚠️⚠️ **Three things must move together, and widening only some of them is worse than widening
none.** `devcontainer.json` `appPort`, `eqemu_config.json` `server.zones.ports`, and the `launcher`
table's `dynamics`. Widening the config alone boots zones on ports the Windows client cannot reach,
which turns a clear *"That zone is unavailable"* into a **silent timeout on zone-in**.

⚠️ The `appPort` half needs a **container rebuild**, which is the operation CLAUDE.md §25 records as
the most expensive recurring failure in this project (two Docker engines, one volume name, a
stale-but-healthy-looking database). Run `db_sanity.sh` afterwards and read §25 first.

## Testing

- `/pick` (or the **Difficulty** button on `/aot`) opens the window; the row you are on is
  highlighted and marked `>` in the **Now** column.
- Select a row, press **Travel**. You stay at the same coordinates, in a different copy of the zone.
- Confirm it is refused **in combat** and refused **inside a delve** — both are server-side.
- Zone somewhere else and confirm you are still at your difficulty (the shard is re-entered on
  arrival).
- Creatures should be `+2 / +4 / +6` levels and `x2 / x3 / x4` health.
- **Nightmare: awake and unbindable.** Invisibility, sneak and hide should all fail; mez, charm,
  root and snare should all be refused. Stun and fear must still WORK — they are deliberately kept.
- **Hell: champions.** One creature in five spawns glowing and renamed — `a barbed gnoll pup`. The
  affix is permanent and visible before you pull.
- **Hell and Inferno: unfoolable.** Feign death still drops you and stops melee, but nothing forgets
  you — they are on you the moment you stand.
- **Inferno: vigilant.** Creatures notice you from twice as far, **horizontally only**. Test this in
  a multi-level dungeon: a creature on the floor below must NOT aggro through the floor.
  ⚠️ Needs a `zone` rebuild — it is backed by `Mob::pAggroRangeZ` (built 2026-08-12).

⚠️ **The rising dead is NOT in this build.** It was removed from Hell on 2026-08-12; the code remains
in `aotv4_difficulty.lua` but no tier enables it, so nothing should ever get back up.

Still unwired from the design: mob classes, no-leash, doubled loot and doubled ink.
