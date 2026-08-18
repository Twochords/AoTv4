# Hub tutorial — one quest per AoT menu tab

**Status: DESIGN ONLY. Nothing built.**

Ten NPCs in the `freeporttheater` hub, one per button on the `/aot` menu. Each gives a real journal
task that makes the player *use* that window once, and pays a small reward. The point is discovery:
a player who never presses `/aot` never finds the Delve, the Trader, the difficulty ladder or the
travel network, and none of those advertise themselves.

Reserved bands, both confirmed free: **NPCs 2000620-2000629**, **tasks 2000600-2000609**.

---

## The ten tabs

Read off `EQUI_AoTMenuWnd.xml`, which is the authority:

| # | Button | What the player must learn |
|---|---|---|
| 1 | **Spells** | rewards are picked, not bought; Known/Pool tabs exist; rerolling costs coin |
| 2 | **Autoskill** | specials auto-fire, and only **4** may be enabled at once |
| 3 | **Adv Loot** | Loot / Leave / Never, and the persistent Never list |
| 4 | **Trader** | price book → list → sell offline, from any zone |
| 5 | **Achievements** | they exist at all, and some grant AA and gear |
| 6 | **Allaclone** | search items/npcs/spells, and the **Zone XP** browse list |
| 7 | **Death Book** | what a death cost you |
| 8 | **Delve** | the scaling dungeon ladder and its rewards |
| 9 | **Difficulty** | Nightmare/Hell/Inferno, and that shifting is a zone change |
| 10 | **Travel** | waypoints are discovered, not given |

---

## ⚠️⚠️ THE HARD PART: A TASK CANNOT SEE A WINDOW OPEN

`TaskActivityType` (`common/tasks.h:46`) offers Deliver, Kill, Loot, SpeakWith, Explore, TradeSkill,
Fish, Forage, CastOn, SkillOn, Touch, Collect, GiveCash. **There is no "used a UI window" type**, and
there cannot be — our windows are not part of the task system, they are chat-protocol overlays.

**The lever that makes this buildable is `Lua_Client::UpdateTaskActivity(task, activity, count)`**
(`zone/lua_client.cpp:1772`). Every window already talks to the server over `/say`, so the handler
that answers `/say delve` can *also* tick the tutorial objective. The task system never needs to know
what a window is.

### ⚠️⚠️ But only SIX of the ten reach Lua. The rest are swallowed in C++ first.

| Window | Its say command | Reaches Lua? |
|---|---|---|
| Spells | `spellpick`, `spellreroll`, `sjpool` | ✅ `global_player.event_say` |
| Trader | `shopopen`, `shopadd`, `shopsetprice` | ✅ `bazaar_broker` |
| Delve | `delve`, `delveenter` | ✅ `aotv4_dungeon` |
| Difficulty | `diffwin`, `diffset` | ✅ `aotv4_difficulty` |
| Travel | `travelwin`, `travelgo` | ✅ `aotv4_travel` |
| Allaclone | `srch`, `srchdet` | ✅ (verify — server-side `Client::SearchList` also handles it) |
| **Adv Loot** | `als*` | ❌ **`Client::ChannelMessageReceived`, client.cpp:1211** |
| **Autoskill** | `ask*` | ❌ **client.cpp:1216** |
| **Achievements** | `#ach` | ❌ a GM-style command, not a say |
| **Death Book** | *(none — server pushes `LOSTDATA`)* | ❌ nothing to hook |

Those two intercepts `return true`, which swallows the line **before** `EVENT_SAY` — so no Lua hook
can ever see them. Three options, in order of preference:

1. **Add one `UpdateTaskActivity` call inside each C++ handler** (`HandleAdvLootSay`,
   `HandleAutoSkillSay`, `#ach`). Four lines, needs a zone rebuild, and it is the only approach that
   makes the objective mean *"you really used the window"*.
2. **Give those four a different objective** the task system can already see — e.g. Adv Loot's quest
   completes on **Loot** of anything, Death Book's on the player's next death.
3. **Hand-in fallback**: the NPC asks a question whose answer is only visible in the window, and the
   player replies. Zero code, but it is a quiz, not a tutorial.

📌 **Recommendation: (1) for Adv Loot and Autoskill, (2) for Death Book, (3) for Achievements.**
The Death Book one is nearly poetic as a Kill/death objective, and nobody should be made to die on
purpose for a tutorial — it completes on your first death whenever that happens.

---

## The ten NPCs

All in `freeporttheater`, ids **2000620-2000629**, tasks **2000600-2000609**. Names follow the
gnome-mech theme already established by the guards, except where a different caste reads better.

| id | NPC | tab | objective | reward |
|---|---|---|---|---|
| 2000620 | **Loremaster Ythel** | Spells | open the spell window and confirm one pick | 1 Tome of Insight (Worn) |
| 2000621 | **Drillmaster Kort** | Autoskill | enable any autoskill | 5p |
| 2000622 | **Quartermaster Bindle** | Adv Loot | loot one item through the window | a 10-slot container |
| 2000623 | **Broker Sarine** | Trader | set a price and list one item | 10p |
| 2000624 | **Chronicler Vess** | Achievements | open the window, hand back the count | 1 skill-up potion |
| 2000625 | **Archivist Talvo** | Allaclone | run one search **and** open Zone XP | a map/notebook trinket |
| 2000626 | **Keeper of Names** | Death Book | open it, then **spend your first AA point on Origin** | Origin itself (the AA is the reward) |
| 2000627 | **Delvemaster Rhun** | Delve | clear rung 1 | 1 delve augment (T1) |
| 2000628 | **Warden Ashka** | Difficulty | shift to Nightmare and kill one creature | 10p + Ink of the Lost |
| 2000629 | **Pathfinder Wynn** | Travel | discover any one waypoint | free reroll (cut the counter) |

### ⚠️⚠️ The Death Book quest is really the AA quest — and AA has no button

**There is no AA tab on the `/aot` menu, and the dll's AA overlay was deleted** (§6). AA points are
banked by `Client::AoTv4DivertAAPoints` and offered by `aa_choice` as **chat saylinks** — which is
the one progression system on this server a player can miss entirely, because nothing on screen
points at it. Pairing it with the Death Book is right on the merits: AA *is* the death-driven meta
progression, so the window that shows what a death cost you is the natural place to explain what a
death earned you.

**Origin (AA id 331)** is the pick. It casts spell **5824** — SPA 322, self, 15s cast, **18 minute
reuse** — a teleport to your bind point.

- 📌 **Thematically exact**: everyone binds at the hub, so Origin is "get home after dying" — the
  same thing the Death Book is about — and it is a *voluntary, faster* version of the death you would
  otherwise take. It is the most useful level-1 AA on the list and the one a new player benefits from
  immediately.
- ⚠️⚠️ **It is `enabled = 0` today, so it does not exist at runtime.** `zone/aa.cpp:1823` loads only
  enabled rows — an offered-but-disabled AA is granted, silently refused, and reports nothing (§32).
  Enabling it is step one.
- ⚠️⚠️ **Then `aa_pool.lua` MUST be regenerated** (`custom/tools/gen_aa_pool.pl`). Its own header
  says it: *"Regenerate after enabling or disabling ANY AA: a pool entry that is not enabled gets
  offered and then silently refused, with no message anywhere."* Today the pool holds **90** AAs and
  Origin is not among them.
- ⚠️ **Hide it from the random rotation.** Origin should be the *taught* first pick, not something a
  player might roll blind at death 30. Two ways: flag it `grant_only = 1` (§6's mechanism, which also
  keeps it out of the native AA window), or exclude id 331 from the generator's WHERE.
- ✅ **The hook for "first pick" already exists**: `aa_choice` keeps a per-character counter in
  `aa_pcount_<charid>`. When it is 0 and the tutorial task is active, offer Origin alone instead of
  three random affordables.
- ⚠️ **Cost 0, level_req 5, `next_id -1`** — a single rank, free, so it cannot strand a player who has
  banked only one point. ⚠️ `level_req 5` still needs lowering to 1 like the rest of the pool
  (`aotv4_aa_level1.sql`), or a level-1 character is refused with no message.
- ✅ **OUT OF COMBAT ONLY — done natively, no code.** `spells_new` carries `InCombat` / `OutofCombat`
  (columns 215/216), enforced in `Mob::CheckCastRestrictions` (`zone/spells.cpp:601` and `:806`):
  with `InCombat = 0, OutofCombat = 1` the cast is refused while `GetAggroCount() > 0` and the client
  is told *"You cannot cast this spell in combat"* (`NO_CAST_IN_COMBAT`). Applied to 5824.
  - ⚠️⚠️ **That check only fires on a BENEFICIAL spell.** Origin is `goodEffect = 1` so it bites — but
    the same two columns on a detrimental spell do nothing at all, which is a silent no-op waiting for
    whoever copies this pattern next.
  - 📌 `GetAggroCount()` is the same predicate delve entry (§24) and the difficulty shift (§43) use,
    so "in combat" means one thing across the server: something still has you on its hate list, which
    is exactly when an escape is worth most.
  - 📌 Native precedent: `940 Mana Convert`, `1342/1344 Reviviscence`. Not a custom mechanism.
  - ⚠️ The **15 second cast** is a second, independent brake — it can be interrupted, so even the
    moment aggro drops it is not an instant escape.
  - ⚠️⚠️ **`spells_new` IS SHARED MEMORY.** World down → `./shared_memory` → restart, or no zone sees
    the change. This ships as part of the Origin migration, not on its own.
- ⚠️ **AA is now earned continuously** (§45, 1:1 with applied experience), so a new character banks
  their first point within the first few levels. The quest should **not** require a death — that was
  the old model and would now make the tutorial wait on something that no longer gates AA.

⚠️ **Rewards are deliberately small and mostly consumable.** A tutorial that pays real gear competes
with the delve and the loot tiers; the point is to teach the loop, not to be a source. The two that
are *not* consumable — the container and the trinket — are chosen because they make the window they
taught more usable.

📌 **The Travel reward is the reroll-counter cut** already built for the Tome of Insight
(`spell_choice.M.BOOK_REROLL_CUT`). It costs nothing to author and is genuinely wanted early.

---

## Structure

- **One task per NPC**, 1-2 activities each. Short: a tutorial the player abandons teaches nothing.
- **`SpeakWith` opens and closes every quest** — the player hails, gets the task, does the thing,
  returns. That is the shape the stock tutorial uses and it is what makes the journal readable.
- ⚠️ **`min_level` 1 and no level cap.** These must work for a level 1 character on their first run
  *and* for an existing level 30 who has never pressed `/aot`.
- ⚠️ **Repeatable: NO.** `task_activities.zones` left **empty**, matching the delve tasks — an empty
  list credits anywhere (`TaskActivity::CheckZone`), which matters because half these objectives are
  completed in a delve, a shard, or another zone entirely.
- 📌 A **meta-achievement** for finishing all ten is the natural capstone, and category 8 "Server
  Custom" already exists for exactly this.

## ⚠️ Things that will bite

- ⚠️⚠️ **`assign_task` with `enforce_level_requirement = false`**, as the delve does — the tutorial
  must not be gated by the level checks the tasks carry for cosmetic reasons.
- ⚠️⚠️ **A task id is not a task band.** The delve occupies **2000300-2000538** and its mode offsets
  are 40 apart; keep the tutorial's ten well clear at 2000600+ so a future mode cannot collide.
- ⚠️ **The hub is instance-free open world.** Several objectives complete *inside* an instance (delve,
  difficulty shard). Since `zones` is empty this works, but the **return** hail happens back in the
  hub — so the quest text must say so, or players will stand in the delve wondering.
- ⚠️ **`event_task_complete` is already used by the delve** (`aotv4_dungeon.on_task_complete`). The
  tutorial's handler must check the task id band and return early otherwise, or a tutorial completion
  will be read as a delve clear.
- ⚠️ Ten new NPCs in a zone with **no faction** — copy the `npc_faction_id = 0` the guards use, or a
  quest-giver could end up KOS to somebody.

## Open questions

1. **Rebuild `zone` for the two C++ hooks, or accept weaker objectives for Adv Loot and Autoskill?**
2. **Should the ten be a chain** (each NPC points at the next) or ten independent quests? A chain
   teaches the menu in a sensible order; independents let a returning player do only what they need.
3. **Does the tutorial replace the stock `tutorialb` one**, or sit alongside it?
