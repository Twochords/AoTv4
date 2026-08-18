# Fellowships — design spec

**Status: DESIGN ONLY. Nothing built.**

A 12-person social group that persists across logout, with its own chat and a **campfire** your
members can travel to. Scope decided 2026-08-16:

| piece | in? | why |
|---|---|---|
| Roster + persistent chat | ✅ | no conflicts with anything existing |
| **Campfire travel** | ✅ | it is "letting them come to you" — the inverse of the travel system, not a duplicate of it |
| **Vendor + components** | ✅ | wanted as a **platinum sink**; the server currently has almost none |
| XP sharing / Vitality | ❌ | declined for now |

---

## ⚠️⚠️ EQEmu IMPLEMENTS NONE OF THIS — the whole path is ours

`OP_FellowshipUpdate` exists as an **opcode constant with no handler**, `Class::FellowshipMaster`
(69) is defined and unused, and there are **no fellowship tables at all**. Nothing can be "turned
on"; every part below is built the same way every other AoTv4 system is — Lua state in data
buckets, a chat-line protocol, and a native SIDL window.

⚠️ **The five stock fellowship NPCs are unreachable and must not be used.**
`Randall_of_the_Fellows`, `Fellow_Wyllie`, `Fellow_Byllie`, `Rhondda_the_Flagger` and
`Sephorra_the_Flagger` (202396-202400) all spawn in **`poknowledge`, region 99** — the "Unused"
bucket. Registration and the vendor go to the **Resplendent hub** instead, alongside the three NPCs
already there (2000400-2000402).
📌 Those three carry lock triggers (`aotv4_resplendent_npc_lock.sql`) because they were clobbered
twice by full imports. **Any fellowship NPC added to the hub gets the same treatment**, or the next
import silently turns it into an untargetable merchant.

---

## What live actually does (the reference)

Kept here so the deltas below are legible.

- Up to **12 players**, one fellowship per character, formed at Randall in PoK. Permanent chat channel.
- **Campfire**: 3+ members in the same zone plant one; lasts ~6 hours. The basic *Fellowship of Honor*
  fire is free and inert; specialised fires need a **Kit + Lumber Bundle** from nearby vendors and
  grant buffs (a health fire is ~1500 HP).
- **Fellowship Registration Insignia**: say `how` to Randall; instant cast, **~15 min recast**,
  unlimited clicks. Refused in combat mode or while invisible; must be in rest state.

Sources: [Fanra — Fellowships](https://fanra.fandom.com/wiki/Fellowships),
[Fanra — Campfires](https://everquest.fanra.info/wiki/Campfires),
[EQ Freelance](https://eqfreelance.net/basics/fellowships.html).

---

## 1. Roster

State, all in data buckets, all **global** (a fellowship spans zones):

| bucket | holds |
|---|---|
| `fship_of_<charid>` | the fellowship id this character belongs to |
| `fship_<fid>` | `leader_charid\|charid,charid,…` |
| `fship_name_<fid>` | display name |
| `fship_next` | id counter |

- Cap **12**, one fellowship per character, both re-checked server side.
- Leader can invite, kick and rename; anyone can leave; last one out disbands.
- ⚠️ **Both buckets must be written together on every membership change.** `fship_of_` is the
  reverse index and is what every hook reads first; a stale one leaves a character receiving chat
  for a fellowship whose roster no longer lists them.
- ⚠️ **The roster survives the roguelite death untouched.** Fellowship is social state, not earned
  progress, and dying already costs enough.

## 2. Chat

`/fsay <message>` → loop the roster → `cross_zone_message_player_by_char_id`.

- ⚠️⚠️ **There is no `cross_zone_*_by_fellowship`, and there cannot be** — the engine has no concept
  of one. Coverage is assembled from the roster, exactly as §18's `#worldbuff` assembles server-wide
  coverage from four hooks because no single call reaches everybody. At a cap of 12 the loop is free.
- ⚠️ **Not a real UCS channel.** A genuine channel would need UCS work and client wiring for one
  line of text; the message relay needs neither and rides the pattern every other window uses.
- ⚠️ Offline members are skipped silently — the binding is a no-op for them, which is the wanted
  behaviour.

## 3. ⚠️⚠️ The campfire is DATA, not an NPC — a zone that empties would take it with it

This is the design's load-bearing constraint and it is not obvious.

**Zones are dynamic: an idle one self-terminates** (§2), and `eq.spawn2` is zone-local. A campfire
that exists only as a spawned NPC therefore dies the moment its zone empties — which, for a fire
whose entire purpose is "somewhere for people who are *not here* to travel to", is precisely the
case that matters. A 6-hour duration would in practice mean "until the last person walks out".

**So the fire is a bucket, and the NPC is a rendering of it:**

```
fship_fire_<fid> = "zone|x|y|z|h|expiry|type"
```

and the **first player to enter that zone spawns the model**, on `event_enter_zone`. That is exactly
the world boss pattern (§17c), which exists for the same reason and is already proven here.

- ⚠️ **Travel reads the bucket, never the NPC.** A member can arrive at a fire in a zone nobody has
  entered since it was lit; the model appears when someone gets there.
- ⚠️ **Plant only in the open world.** Refuse inside a **delve** or a **difficulty shard** — both are
  instances, and a fire in one is a permanent door into another player's private instance. Test the
  instance id, not the zone.
- ⚠️ Expiry is checked on read as well as swept, so a lapsed fire cannot be travelled to even if its
  model is still standing.
- 📌 **Live's "3 members in the same zone to plant" is dropped.** One person planting and summoning
  eleven *is* the feature as scoped ("letting them come to you"). The brake is the region gate below,
  not a headcount.

## 4. Travel — and the four gates that keep it honest

A **Travel to Campfire** button in the fellowship window → `/say fshipgo` → server moves you.

⚠️⚠️ **Deliberately NOT an item, despite live using one.** An insignia in your bags is destroyed by
the roguelite death wipe unless it is added to `death_loss.M.is_kept` — the same trap the tradeskill
tools hit (§32), where `claim_once = 1` meant one death cost them permanently with no way back. A
window button cannot be lost, and the window is where everything else already lives.
📌 The cost of that choice: items give a native `recastdelay`/`recasttype` cooldown for free
(`recastdelay 900` = 15 min). Ours is a Lua stamp instead, which the difficulty system already does.

Gates, in order — **resolve everything before moving anyone**:

1. **Fellowship + live fire** — you are in one, it has an unexpired campfire.
2. ⚠️⚠️ **THE DESTINATION REGION MUST BE UNLOCKED.** `eq.get_zone_region()` is Lua-bound and
   `aotv4_regions` already gates every travel destination. **Without this the campfire is a
   region-lock bypass**: a friend standing in a locked region becomes a free door into it, which is
   the single thing §11 is most emphatic about ("a hub is a source that is never a destination").
3. ⚠️ **Refused in combat** (`GetAggroCount() > 0`). Same reasoning as delve entry (§24) and the
   difficulty shift (§43): without it this is a **better escape than Gate** — instant, no reagent, no
   cast time, breaks every hate list at once. Live's "not in combat mode / must be resting" is the
   same intent.
4. ⚠️ **Not while in a delve or a shard** — leaving from inside one strands the run.

Then a **cooldown stamp** (15 min, matching live), written **only on a successful move** so a refusal
never spends it. ⚠️ That ordering bug has bitten twice already — §43 records the difficulty shift
consuming its cooldown while declining to move the player.

## 5. The vendor — the platinum sink

At the Resplendent hub. Sells the two components; the fellowship window spends them.

| item | buys |
|---|---|
| **Campfire Kit** | the fire itself |
| **Lumber Bundle** | the fire's *type* — which buff it carries |

- **Basic fire: free, no components, no buff.** It is the travel target and must never be gated
  behind coin, or a broke fellowship cannot use the feature at all.
- **Buff fires cost.** This is the sink.
- ⚠️ `merchantlist` prices are **static** — no per-character column, no hook (§3). That is fine here,
  unlike the reroll's escalating price which is why *that* could not be a merchant.
- ⚠️⚠️ **I cannot size the price from this container.** `/src` is the test environment and live is a
  different database (§25); the only wealth here is a GM test character holding 18,902p. Reference
  points that *are* known: a reroll is **5p × (purchases + 1)** and a delve rung pays **10p**. A buff
  fire wants to cost several delve runs — call it **250-500p** — but that number should be checked
  against real live holdings before it ships.

**The buff itself** is applied by Lua from a per-client proximity check while you are near the fire
and in the fellowship — the `#worldbuff` shape (§18), not an `auras` row.
⚠️ **An aura row would be wrong**: `aura_type 1` is *OnAllGroupMembers*, and a fellowship is not a
group. Half your fellowship would get nothing.

---

## Build list

| piece | file |
|---|---|
| All logic | `lua_modules/aotv4_fellowship.lua` |
| Window | `core_fellowship.cpp/.h` + `EQUI_AoTFellowshipWnd.xml`, own TU, no detours |
| Launcher | a button in `EQUI_AoTMenuWnd.xml` (§21) |
| Registrar NPC, vendor, components, campfire NPC, buff spells | one migration + the hub lock triggers |

**Protocol**: `FSHIPDATA <fid>^name\|online\|zone^…` and `FSHIPFIRE <zone>\|<expiry>\|<type>` out;
`/say fshipinv <name>`, `fshipgo`, `fshiplight <type>`, `fshipleave`, `fshipreq` in — all swallowed
by the dll and intercepted in `Client::ChannelMessageReceived` so they never reach chat or quests.

⚠️ **Every gate re-checked server side.** The window is display only.

---

## Open questions

1. **Buff fire price** — needs a look at real live platinum (see above).
2. **Which buffs?** Live's health fire is ~1500 HP; at a level 30 cap with ~1,500 HP characters that
   is a *doubling*, so the numbers need deriving here rather than copying.
3. **Fire duration** — live is 6 hours. Longer is friendlier on a small population; shorter makes the
   Kit a repeat purchase, which is the better sink.
4. **Can a member re-light someone else's fire?** Leader-only is simplest; anyone is friendlier.
