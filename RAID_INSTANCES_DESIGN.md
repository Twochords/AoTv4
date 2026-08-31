# Raid instances — design, not yet built — 2026-08-30

Raid encounters delivered through the **delve** system: the boss alone, in its own native zone, in a
private instance, entered by a **group**. No trash, no wandering adds, nothing but the encounter.

> Status: **design only.** Nothing in this document is implemented. Sections 1-3 are settled and
> verified against the database; section 7 lists what still needs an owner decision.

---

## 1. The empty zone is free — instance at a NON-ZERO version

**This is the whole mechanism and it costs nothing.** `Zone::LoadSpawnGroups` filters spawn points
with `(version = N OR version = -1)`, and in the three candidate zones **every spawn2 row is version
0 and not one is `-1`**:

| zone | rows | versions present | any `-1` |
|---|---|---|---|
| `kedge` Kedge Keep | 113 | 0 only | none |
| `mistmoore` Castle of Mistmoore | 175 | 0 only | none |
| `velketor` Velketor's Labyrinth | 262 | 0 only | none |

So an instance created at **any version other than 0 is completely empty** — geometry, doors and zone
points intact, zero NPCs. We then place the boss ourselves.

- ✅ **No data edits.** No `DELETE FROM spawn2`, no `enabled = 0`, no per-zone depop list. These are
  real world zones; stripping their spawn tables would empty them for everyone, permanently — the
  same reasoning §26 gives for why delve mobs are cleared at spawn rather than in `lootdrop_entries`.
- ✅ **No race to lose.** §26 records that an instance **populates before the player finishes zoning
  in**, which is what broke the delve's first scaling pass. Depopping on entry would inherit that bug
  exactly. Nothing spawning in the first place cannot be raced.
- ⚠️⚠️ **THIS IS THE OPPOSITE OF WHAT THE DELVE DOES, AND BOTH ARE RIGHT.** §24 warns in capitals that
  a delve must **never** use version 0, because for a DoN zone version 0 is the *open world* spawn set
  and the non-zero versions are the authored mission layouts. Here the intent is inverted: we want
  nothing at all, so we want a version nobody authored. **Do not "fix" one to match the other.**
- ⚠️ **Verify the invariant per zone before adding one.** A zone with `version = -1` rows, or with
  authored alternate versions, will not come up empty. The check is one query:
  ```sql
  SELECT version, COUNT(*) FROM spawn2 WHERE zone = '<zone>' GROUP BY version;
  ```
- 📌 **Reserve a version band** — suggest **900+** — so a raid instance can never collide with a real
  authored version if one is ever added to these zones, and so `version` alone identifies a raid
  instance when reading the database.

## 2. Placing the boss

`eq.spawn2(npc_id, 0, 0, x, y, z, heading)` after the zone is up. Native placements exist for two:

| boss | npc id | zone | x / y / z / heading | native level |
|---|---|---|---|---|
| Phinigel Autropos | 64001 | `kedge` | 111.5 / 37.0 / -275.8 / 179.5 | 35 |
| Velketor the Sorcerer | 112025 | `velketor` | 135.0 / 178.0 / -53.4 / 259.0 | 35 |
| Mayong Mistmoore | 351118 | `mistmoore` | **none — must be authored** | **85** |

- ⚠️⚠️ **THE DOCUMENT NAMES DO NOT MATCH THE DATABASE.** The design docs say *Phinigel Autotropus*
  (DB: `Phinigel_Autropos`) and *Velkator* (DB: `Velketor`, and that doc's own marquee text spells it
  the DB way). Resolve to the **npc id**, never the name.
- ⚠️⚠️ **MAYONG HAS NO `spawn2` ROW ANYWHERE** and is **level 85**. He needs authored coordinates and
  scaling; the other two are level 35 against a level 30 cap, which is close enough to tune.
- ⚠️ **`eq.spawn2` returns a `Lua_Mob`, NOT a `Lua_NPC`** (§26). `AddItem` / `AddCash` / `ScaleNPC`
  are bound on `Lua_NPC` only, so the result must go through `CastToNPC()` — which *is* bound, unlike
  `CastToClient`. Getting this wrong is a runtime error that aborts the rest of the handler, which in
  the delve silently cost a whole clear's score and history.
- 📌 Place the boss **before** moving anyone in, so nobody watches it appear.

## 3. Group entry — the part the delve does not have

§24 lists group entry as explicitly not built: *"`assign_to_instance` is per character today"*. The
bindings are all per character and there is no group form:
`eq.create_instance(zone, version, duration)`, `eq.assign_to_instance_by_char_id(instance, char_id)`,
`eq.remove_from_instance_by_char_id`, `eq.destroy_instance`, `client:MovePCInstance(...)`.

**Order, and every step of it is load-bearing:**

1. **Validate** — leader only, group exists, every member present and in the same zone, nobody in
   combat, nobody in a delve, lockout clear.
2. **Snapshot the member list** into a plain table of `(char_id, client)`.
3. **Create** the instance.
4. **Assign every member** by char id.
5. **Spawn and scale** the boss.
6. **Move the members**, and **move the leader LAST**.

- ⚠️⚠️ **VALIDATE BEFORE CREATING.** §24: a refusal after `create_instance` leaks an instance every
  time it fires. This is why the combat check belongs in step 1.
- ⚠️⚠️ **ASSIGN BEFORE MOVING, OR WORLD SILENTLY REDIRECTS THEM.** `world/client.cpp` runs
  `VerifyInstanceAlive` → `CheckInstanceByCharID` and on failure calls
  `MoveCharacterToInstanceSafeReturn`. `create_instance` adds **nobody**. The symptom is "it opened
  the raid and then ported me to the bazaar", with no error and nothing wrong in the zone that
  booted. §24 lost a session to this.
- ⚠️⚠️ **MOVE THE LEADER LAST — MOVING THEM MID-LOOP STRANDS THE REST.** `MovePC` to another zone
  tears down the group member pointers the loop is walking. §54 hit exactly this in the travel
  window's group port and the fix was to collect first and move the caller last. Same trap, same fix.
- ⚠️⚠️ **THE TRANSIT FLAG IS MANDATORY, PER MEMBER.** `EVENT_DISCONNECT` fires on **every zone
  transfer**, not just camp — `client_process.cpp` tests `bZoning` thirty lines earlier for other work
  and then fires the event unconditionally. §24 records this making the Delve un-enterable for an
  entire session: the source zone raised disconnect the instant the player was moved, the handler
  concluded the run was over, and it tore the state down while they were still on the loading screen.
  A raid moves **several** players, so it fires several times. Every member needs
  `raid_transit_<charid>` set immediately before their move, cleared on arrival, expiring after ~60s.
- ⚠️ **A fresh instance every time.** `eq.get_instance_id` hands back the one that character used
  last — full of corpses and an emptied spawn table.
- ⚠️ **Do NOT use an expedition.** §24: create-and-enter in one breath is reaped by world as
  `ExpiredEmpty` within ~270 ms, taking its `instance_list_player` rows with it. Expeditions work only
  when entry is a separate act through the engine's own portal. A plain instance avoids all of it, at
  the cost of no Ctrl+Z entry and no engine-managed safe return — both of which we are replacing
  anyway.

## 4. Living in the instance — and coming back to it

- ⚠️⚠️ **DYING KEEPS YOUR RUN, AND THAT IS THE WHOLE REJOIN MECHANISM.** The roguelite reset puts a
  dead player at their bind point, in another zone, at level 1. The obvious thing is to clear their
  run there — and it would throw away their only route back to a share that individual loot (§31)
  rolled **for them personally** and owner-stamped, so nobody else can take it and it simply rots.
  The run is kept; pressing **Enter on the same raid row** routes to rejoin rather than starting a
  second raid, so this needs no new command and no client change.
- ⚠️⚠️ **THE RUN CARRIES A TOKEN, WITHOUT WHICH KEEPING IT WOULD BE DANGEROUS.**
  `Instances:RecycleInstanceIds` is on and ids are reissued lowest-free above 100, so a closed raid's
  id is quickly claimed by something else — most likely a delve. A stale run naming only an id could
  drop a dead player into **a stranger's instance**. The token is stamped into both the run and the
  instance marker and both must match. It is used *instead of* an existence check because there is no
  `is_instance_valid` binding — `eq.get_instance_id` only answers about the zone you are standing in,
  which is no use to somebody trying to get back in from outside.
- ⚠️⚠️ **REJOIN IS GATED ON THE BOSS BEING DEAD, NOT ON "COMBAT HAS ENDED".** The rejoining player is
  in another zone and `eq.get_entity_list()` only reaches the zone you are in, so there is no way to
  ask whether anything is still fighting in that instance. The boss's death is recorded in a bucket,
  which is readable from anywhere, and it is the case that matters. **A mid-fight rejoin is therefore
  refused** — at level 1 with nothing on, it only feeds the boss another death, and it would let a
  raid rotate corpses back in indefinitely.
- ⚠️⚠️ **THE FIVE MINUTES IS A WINDOW FOR THE DEAD, NOT A PROMISE THE ROOM PERSISTS.**
  `Zone:AutoShutdownDelay` is **60 seconds**, so an empty instance zone terminates itself — and
  **NPC corpses are not persisted**, so it takes the boss's corpse and every roll on it. Returning
  inside the window re-boots the zone into an **empty room**. Nothing server-side can hold an empty
  zone open, so the only thing that makes the window real is a living player choosing to stand in it.
  📌 That is why `M.on_boss_death` **tells the survivors**: *"Stay in the zone until your fallen
  return — if everyone leaves, the corpse and its loot go with the room."* It is the difference
  between a rule players can follow and a trap. Accepted knowingly: the failure mode is loot nobody
  was present to collect.
- **Leaving** drops that member only. These are real zones with real zone lines, and the delve's
  answer — fail the run the moment you are outside your instance — is wrong here: one person walking
  a zone line cannot end a raid for five others.
- ⚠️ **Adds that drop combat full heal** (`AoT:NPCFullHealOnReset`, §37). Relevant to Phinigel's swarm
  waves and Velketor's golems: a reset add comes back whole, including a healer add.

## 5. Closing it down

Two conditions, and **the post-kill clock is checked first and ignores occupancy**:

1. **Boss dead + 5 minutes** — held open whether anyone is inside or not, because the people it is
   being held for are by definition *not* inside.
2. **Zone empty + 5 minutes, boss still alive** — a wipe or an abandon. The delay stops a single
   zone-line crossing, or the gap between one member zoning out and another zoning in, ending it.

⚠️⚠️ **THE ORDER IS THE POINT.** Checked the other way round, the last survivor stepping out after
the kill empties the zone and closes the raid **on top of the loot the grace exists to protect**.

- ⚠️ The window is enforced in the **rejoin path as well as the sweep**, and it has to be: the sweep
  runs on a client timer *inside* the instance, and the post-kill window is often empty, so there is
  nobody in there to run it. Left to the sweep alone the marker would survive until the instance
  expired and somebody could stroll back in hours later.
- 📌 **Nothing is leaked by an instance that closes late.** The empty zone process ends itself after
  60 seconds and releases its port; all that survives is one cheap `instance_list` row.
- ⚠️ `M.close` is the single teardown: every marker cleared together, and **`destroy_instance` last** —
  destroying an instance somebody is still standing in strands them in a zone that no longer exists.

## 5a. ✅ DELVE MODES AND DIFFICULTY CANNOT REACH A RAID — verified, not assumed

Raids reuse the delve's window, its Enter button and several of its hooks, so "no affixes in a raid"
needed checking rather than hoping. All three systems are already inert inside a raid instance, and by
construction rather than luck — **each one verifies its own marker instead of assuming that being in
an instance means it owns that instance**:

| system | why it cannot touch a raid |
|---|---|
| `aotv4_dungeon.on_npc_spawn` | calls `M.layer_for_zone()`, which requires a `delve_layer_<inst>` bucket whose recorded zone MATCHES the current zone, and then that the zone be in `M.ZONES`. Raid zones are not delve zones, and there is **no zone-scan fallback** — it returns nil and the handler exits before any scaling, thinning or loot stripping. |
| `aotv4_dungeon.on_tick` | needs a `delve_run_<charid>` bucket. Raids use `raid_run_<charid>`. |
| `aotv4_difficulty.here()` | needs a `zdiff_of_<inst>` marker, and round-trips it against the forward map. A raid instance has neither. |

✅ And `aotv4_dungeon.M.enter` routes to the raid path **before the mode is ever resolved**, so no
multiplier, swarm count, thinning fraction or boss count can reach a raid encounter. The dropdown
selection is read and ignored.

⚠️⚠️ **THE INVARIANT WORTH KEEPING: A SYSTEM MUST IDENTIFY ITS OWN INSTANCES BY A MARKER IT WROTE,
NEVER BY "I AM IN AN INSTANCE".** Instance ids are recycled on this server (§43), so the weaker test is
not merely sloppy — it actively mis-fires as soon as an id is reused, and silently. All three checks
above already do the strong version; anything added later must too.

## 6. Loot

- ⚠️⚠️ **`Lua_Raid` HAS NO `AddMember` BINDING** (§17c) — the raid API is read-only from Lua. Loot
  rights must be granted the way `#worldboss` does it: `corpse:AddLooter(mob)` over the hate list,
  `MAX_LOOTERS = 72`.
- ⚠️ **Individual loot rolls the table once per eligible player** (§31), so a six-person group puts six
  personal rolls on one corpse. Lootslots are numbered **per player**, so the 34-slot ceiling applies
  per player and this scales — but §30 records the sharp test as only ever having been done with
  **two** characters. A raid is the first thing that will push it.
- 📌 Coin is paid directly at death per player (§31) and never lands on the corpse.

## 7. Decisions needed before any of this is built

1. ~~**Where does it live in the Delve window?**~~ **ANSWERED: option (a), and BUILT.** Raid rows are
   appended to the same Layers list at levels above `RAID_BASE`, which ships with no client change.
   ⚠️ **CORRECTION to what follows:** the claim that (a) needs no dll change was only half right. The
   *mode dropdown* is fully server-driven, but the client reads the combo **only when Enter is
   pressed** and never tells the server the selection changed — so "the list swaps when Raid is
   selected" does need a dll change. Raid rows being always-present is what avoids it.
   **(a) A `Raid` difficulty**, reusing the existing `DUNGDATA` wire format and the Layers list, with
   the raid encounters replacing the rung list when selected. **No dll change, no new XML.**
   **(b) A third tab** beside Layers and Score Sheet. Cleaner conceptually, but a new wire line, a dll
   change and an XML re-ship. ⚠️ Note the mode offsets: §24 records `M.MODES.taskoff` at 0/40/80/120/
   160/200 with **200 retired but its 39 task rows still in `tasks`** — a new mode reusing 200
   inherits the name *Fragile* in the journal.
   **Recommendation: (a)**, because it ships without touching the client at all.
2. **Lockout.** Without one these are farmable on repeat. Per character, or per group? How long?
3. **Scaling reference for a group.** The delve uses *"the first client found in the instance"*, which
   §24 already flags as a known simplification. A raid needs a real answer: highest member, average,
   or a flat encounter level that ignores the party.
4. **Minimum group size.** "Group compatible, not aimed at solo" — is solo *refused*, or merely
   unwise? Refusing is one check; allowing it means the encounter must not be trivially soloable.
5. ~~**Wipe cost.**~~ **ANSWERED: a wipe is a death, with the full roguelite reset.** That is the
   point, and it is why the rejoin window exists at all — losing the run should not also silently lose
   the loot you had already earned a roll on.
6. ~~**Capacity.**~~ **ANSWERED: 150 ports on live, and an empty instance frees its own.** I had this
   wrong first time and asserted it: an idle dynamic zone terminates after `Zone:AutoShutdownDelay`
   (60s), so a finished raid costs nothing but an `instance_list` row. §27's warning still stands for
   *concurrent* raids — the port range and `launcher.zone.dynamics` must move together — but there is
   no per-raid lingering cost to budget for.
7. **Era.** Kedge Keep and Castle of Mistmoore are expansion 0, region 1 — reachable today.
   **Velketor's Labyrinth is expansion 2, region 3**, the era is Classic, and §12 records the
   era-advance trigger as **orphaned** since random AA was retired, so nothing advances it. The
   Thurgadin encounter is blocked on that, not on anything in this document.

## Boss tuning and mechanics — 2026-08-30

⚠️⚠️ **BEFORE THIS, ALL THREE BOSSES WERE WEAKER THAN A DELVE WARDEN.** `ensure_boss` called
`ScaleNPC(30)` and stopped, and `ScaleNPC` rewrites the stat block **wholesale** from
`npc_scale_global_base` (§24) — so every hand-authored stock stat was discarded. Measured:

| boss | scale type | hp | max hit | AC |
|---|---|---|---|---|
| Phinigel · Velketor | 2 (`IsRaidTarget`) | **1,800** | 102 | 191 |
| Mayong | 1 (name starts uppercase, not raid_target) | **1,440** | 82 | 152 |

A delve warden at rung 30 is ~6,750 hp (`M.BOSS_HP_MULT` 5.0 over a measured regular mob), so the
**raid boss was a quarter of the warm-up encounter**, and at a duo's ~300 dps Phinigel died in **six
seconds**.

### The numbers, and where they come from
Player baseline: a tanky character is ~3,000 max hp at ~50 percent mitigation; a duo does ~300 dps and
a full group ~900. So **every 10,000 boss hp is ~30 seconds of duo fight**, and auto-attack is
targeted at **100-200 max hit per SECOND of swing time** (a 2.5s swing wants a 250-500 max hit).

| boss | hp | swing | max hit | per sec | duo | group of 6 | tank unhealed |
|---|---|---|---|---|---|---|---|
| Phinigel | 120,000 | 2.5s | 350 | 140 | ~6.7 min | ~2.2 min | ~43s |
| Mayong | 160,000 | 2.5s | 400 | 160 | ~8.9 min | ~3.0 min | ~38s |
| Velketor | 200,000 | 2.5s | 450 | 180 | ~11 min | ~3.7 min | ~33s |

⚠️ **AC is pinned at 191 for all three, not left to the scaler.** Mayong resolved to type 1 and would
have carried 152 while the others carried 191 — players connect more often, so he dies faster than his
hp implies, for a reason invisible in the encounter table. 191 is what the level 30 raid row already
gives, so this pins rather than inflates. **Tune hp and hit first; raising AC invalidates the ~300 dps
estimate the hp totals are derived from.**

### Mechanics — two of the three had none
| | summon | immunities | spells | melee |
|---|---|---|---|---|
| Phinigel | ✅ stock | mez, charm, fear, flee | ❌ **list 1346 has ZERO entries** | — |
| Velketor | ✅ stock | + stun, snare | ✅ 63 (`Wizard_spellset_pre_pop`) | dual wield |
| Mayong | ❌ | ❌ **none at all** | ❌ (class 1, 0 mana) | ❌ |

✅ **`M.BASE_ABILITIES = {1, 13, 14, 17, 21}`** on every boss — Summon, and immunity to Mesmerize,
Charm, Fear and Fleeing. A bare Mayong was mezzable, **charmable**, snareable and unable to summon, so
one enchanter deleted the encounter and anyone could kite him indefinitely.
⚠️⚠️ **SLOW IS DELIBERATELY STILL LANDABLE** (ability 12 is absent). Slow is the core group
contribution of shamans, enchanters and beastlords; a slow-immune boss deletes their reason to be
there. Stun is left alone too — Velketor is stun immune because his stock row says so, not by fiat.
✅ Phinigel takes the **Default Wizard List (2)** — he is class 12 with 6,052 mana and an empty list.
✅ Mayong takes **Flurry + Triple Attack**; class 1 with 0 mana cannot cast, so his identity is melee.
⚠️ **Rampage was considered and NOT given.** It hits the whole hate list, and at a 400 max hit against
3,000 hp characters it would kill healers outright in a duo. Revisit once fights have been seen.

⚠️ Applied with **`SetSpecialAbility`, never `ModifyNPCStat("special_abilities", …)`** — that key
replaces the entire set from a packed string and would wipe Phinigel's summon and magical-attack
requirement and Velketor's stun/snare immunity (§43).
⚠️ The spell list is attached **after** `ScaleNPC`: `AI_AddNPCSpells` filters by the level at the
moment it is called (§24), so attaching first would freeze a list filtered for the stock row's level
— 35 for Phinigel, 85 for Mayong.

📌 **All of this is authored and none of it has been fought.** The hp totals follow arithmetic from an
estimate, not from a measured kill.
