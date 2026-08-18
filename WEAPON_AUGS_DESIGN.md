# Weapon Augments — design outline for review

**Status: DESIGN AGREED, NOTHING BUILT.** Every decision is settled — there are no open questions.
Values are first-pass tuning unless marked as calibrated against stock data.

**Build order:** make **one** test augment and confirm an aug proc actually fires in play (the code
path exists at `attack.cpp:5603` but nothing on this server uses it yet), then generate the other 27
with a fixed-seed generator.

Proc augments that drop from **named creatures in the challenge difficulties**. They are a separate
line from the delve augments (147600-147915), which are flat stat sticks earned by clearing rungs —
these are *effects*, earned by hunting.

---

## 1. What drops, and from what

| condition | rule |
|---|---|
| difficulty | Nightmare, Hell or Inferno. **Normal drops none** |
| creature | **named only** |
| level | the creature's **effective** (post-scaling) level |

**"Named" uses the heuristic already shared by the world boss and `quest_difficulty.pl`**: EQ trash is
lowercase (`a gnoll pup`), named creatures are proper nouns or lead with `#`. One call, and consistent
with the rest of the server.

⚠️ This is a **different gate from the Tomes of Insight**, which drop on *con* (white or better) from
anything. Naming the wrong gate sends players hunting the wrong creatures — a warning already written
into `aotv4_difficulty.lua`.

---

## 2. Drop rate

```
chance% = 5.0 × diff_weight × (effective_level / 35)      capped at 5.0
```

| difficulty | weight |
|---|---|
| Nightmare | 0.40 |
| Hell | 0.70 |
| Inferno | 1.00 |

Resulting grid:

| effective level | Nightmare | Hell | Inferno |
|---|---|---|---|
| 10 | 0.6% | 1.0% | 1.4% |
| 20 | 1.1% | 2.0% | 2.9% |
| 30 | 1.7% | 3.0% | 4.3% |
| **35** | 2.0% | 3.5% | **5.0%** ← the stated target |

📌 **Effective level, not base level** — the difficulty already adds +2 / +4 / +6, so a level 30
creature in Inferno *is* level 36 and lands on the 5% ceiling naturally at the level cap. Using base
level instead would make 5% unreachable, since the cap is 30.

✅ **Nightmare is included** at the rates above — rare enough that it is a bonus rather than a reason
to farm the easiest challenge tier.

---

## 3. The seven families × four tiers = 28 augments

**Tier is magnitude, not rate** (decided) — a T4 Heal procs as often as a T1, it just heals more. That
keeps tiers from compounding rate × size, and means a player hunts for *their* effect then upgrades it.
The two exceptions are Slow's T3/T4, below.

### Proc frequency is FREE — the engine already does exactly what was asked

⚠️⚠️ **`Combat:AvgProcsPerMinute` is already `2.0` and `Combat:AdjustProcPerMinute` is `true`**, so the
baseline proc rate *is* "roughly twice a minute", automatically compensated for weapon speed
(`Mob::GetProcChances`). **These augments want `procrate = 0`** — the item's `procrate` is a
*modifier*, and 0 means "the server default", not "never procs".

📌 DEX nudges it: `ProcBonus += DEX × 0.075`, so ~2.15/min at 100 DEX and ~2.4/min at 255. Modest, and
it makes DEX quietly worth something to every class.

### Heal — instant, on the wielder

| tier | heal |
|---|---|
| T1 | 40 |
| T2 | 60 |
| T3 | 80 |
| T4 | 100 |

### Heal over time — 4 ticks, on the wielder

| tier | per tick | total |
|---|---|---|
| T1 | 40 | 160 |
| T2 | 60 | 240 |
| T3 | 80 | 320 |
| T4 | 100 | 400 |

📌 Deliberately the same per-tick numbers as the instant Heal. The HoT trades immediacy for four times
the total, which is the honest trade between the two families and needs no separate tuning pass.

### Damage / Damage over time — same ladder, on the target

| tier | Damage | DoT per tick | DoT total |
|---|---|---|---|
| T1 | 40 | 40 | 160 |
| T2 | 60 | 60 | 240 |
| T3 | 80 | 80 | 320 |
| T4 | 100 | 100 | 400 |

### Slow — magnitude caps at T2, then reliability

| tier | slow | SPA 11 base | note |
|---|---|---|---|
| T1 | 10% | **90** | |
| T2 | 15% | **85** | |
| T3 | 15% | 85 | **raised proc rate** (`procrate` > 0) |
| T4 | 15% | 85 | **easier to land** (negative `ResistDiff`) |

⚠️⚠️ **SPA 11's base is the resulting attack SPEED, not the slow amount** — 85 means "attacks at 85
percent" = a 15 percent slow, so **lower is stronger**. Writing 15 there would be a 85 percent slow.
This is already recorded in §5 and is the single easiest thing to get backwards here.

📌 Capping magnitude at 15% and spending T3/T4 on reliability is what stops the strongest debuff in EQ
trivialising the content it drops from. Calibration for T4: stock slows run `ResistDiff` 0 to −400
(*Slime Breath* −400 is effectively unresistable); something modest like −50 to −100 is a real
improvement without making it automatic.

### Snare — plain magnitude ladder

| tier | movement | SPA 3 base |
|---|---|---|
| T1 | −30% | −30 |
| T2 | −40% | −40 |
| T3 | −50% | −50 |
| T4 | −60% | −60 |

📌 Straight magnitude, unlike Slow, because snare is far less dangerous — it controls positioning, not
the length of the fight.

### Aggro — a hate ladder, **no stun**

✅ Decided: tiers buy **hate**, not stun duration. A stun proc firing twice a minute is an interrupt
engine — it would quietly become the best caster-killing tool in the game and a nuisance in every
group, and its value would have nothing to do with the tier on it.

**SPA 92 `InstantHate`.** Calibrated against stock rather than invented — the 21 stock spells using it
run **65 to 950**, and this ladder sits in the lower, classic half:

| tier | hate | stock landmark |
|---|---|---|
| T1 | 65 | *Provoke*, *Feral Roar I* |
| T2 | 100 | *Pique* |
| T3 | 150 | just under *Bellow* (148) |
| T4 | 200 | *Anger I* |

📌 Useful to a tank, harmless to everyone else — which is the point. It is the one family whose value
depends on your role rather than your gear, and the only one that makes a weapon aug interesting to a
Warrior who already has everything else.

### Tier roll

Weighted by effective level, so depth decides quality independently of the drop rate:

| effective level | T1 | T2 | T3 | T4 |
|---|---|---|---|---|
| ≤ 14 | 70 | 25 | 5 | 0 |
| 15-24 | 40 | 40 | 18 | 2 |
| 25-32 | 20 | 35 | 35 | 10 |
| 33+ | 10 | 25 | 40 | 25 |

Family is rolled flat (1 in 7) — nothing chase-rare for its own sake.

---

## 4. ⚠️⚠️ The constraint that shapes the whole thing

**Only ONE augment proc fires per swing** (`Mob::TrySpellProc`, `zone/attack.cpp:5594`):

```cpp
if (!proced && inst) {            // ← only if the WEAPON's own proc did not already fire
    for (each socket) { ... if (RuleB(Combat, OneProcPerWeapon)) break; }   // ← and then stop
}
```

So a three-socket Mythic weapon holding three proc augs does **not** proc three times — they compete,
and a weapon with a native proc suppresses them entirely.

**Consequences to decide on:**

1. **One proc aug per weapon is the real cap.** The other sockets stay stat augs. That is arguably
   good — it makes the choice of *which* effect meaningful — but it must be said plainly in the patch
   notes or players will socket three and feel cheated.
2. **A weapon with a native proc is a bad host.** Worth checking what share of reachable weapons have
   one before tuning rates.
3. ✅ `Combat:OneProcPerWeapon` is **`true`** on this server — verified, not assumed. So the cap above
   is real and the rates in §3 are sized against one proc per swing.

**Rate**: `APC = ProcChance × (100 + aug->ProcRate) / 100`, so `procrate` on the item is a *modifier*
to the weapon's chance, not an absolute. ✅ **Decided: magnitude only**, so tiers never compound rate ×
size — `procrate = 0` on every family except **Slow T3**, which is the one place a tier buys rate.

---

## 5. Id allocation (verified free)

| | band | count | notes |
|---|---|---|---|
| items | **148200-148227** | 28 | reserve 148200-148299; 147969-148199 is also empty |
| proc spells | **44500-44527** | 28 | reserve 44500-44599; all 500 ids 44500-44999 are free |

⚠️ Spell ids must stay **under 45000** — RoF2 caps spell links and the spellbook packet there (§14).
44500 is comfortably inside and clear of the rank rows (43576-44327) and the tome vehicle (44328).

### Field template — taken from the stock vendor augments, not invented

There are **26 augment vendors selling 925 augments**, and **113 of them are `augrestrict = 2`**
(weapons only). Those are the working reference; every field below was read off them.

| field | value | why |
|---|---|---|
| `augtype` | **255** | see the warning below — **NOT** the vendors' 8 |
| `augrestrict` | **2** | weapons only, server-enforced |
| `slots` | **26624** | primary + secondary + range, exactly what the vendor augs use |
| `proceffect` | the spell id | |
| `proctype` | **0** | ✅ `ItemEffectCombatProc = 0` (`common/item_data.h:305`) — 0 really is "combat proc", not "none". Every stock vendor proc aug carries 0 |
| `procrate` | **0** | modifier, not absolute — 0 = server default of 2/min |
| `proclevel2` | **5 / 10 / 20 / 25** by tier | ⚠️ **this is the only gate that actually works** — see below |
| `nodrop` | **0** | ⚠️ inverted: 0 = No Drop |
| `norent` | **1** | ⚠️ inverted: 1 = survives logout |
| `classes` / `races` | 65535 / 65535 | as the vendor augs; the gate is the drop, not the item |
| `reqlevel` | **5 / 10 / 20 / 25** by tier | for display and intent; see the warning below |
| `reclevel` | **0** | ⚠️ the vendors pace tiers with reclevel (20/25/30/40/45) and that would be **inert here** — see below |

### ⚠️⚠️ THE LEVEL GATE IS `proclevel2`, NOT `reqlevel` OR `reclevel`

Tier level requirements are **5 / 10 / 20 / 25**. Three fields look like they could carry that and only
one does:

| field | what it really does here |
|---|---|
| **`proclevel2`** | ✅ **enforced.** `if (aug->Proc.Level2 > ourlevel)` → prints "too low" and skips the proc (`attack.cpp:5605`). This is the gate |
| `reclevel` | ⚠️ **inert.** `CalcRecommendedLevelBonus` scales *stat bonuses* down for an under-level wearer — and these augs have **no stats**, only a proc. The stock vendor augs pace their tiers this way because they ARE stat augs; copying that here would gate nothing |
| `reqlevel` | ⚠️ **not enforced on the equip path** — `inventory.cpp:1205` explicitly excludes augments from the ReqLevel test (`ItemType != ItemTypeAugmentation`). Set it to match for display and intent, but do not rely on it |

✅ **ACCEPTED BEHAVIOUR, not a gap to close.** An under-level character **can** socket and wear the
augment; it simply will not proc, and the engine tells them why. Nobody should later "fix" this by
adding a hard equip block.
⚠️ It does mean the **patch notes must say "will not proc below level X"**, never "requires level X".
Promising a requirement the server does not enforce is how a player concludes the item is broken when
it is behaving exactly as designed.

📌 5/10/20/25 against a level cap of 30 puts T4 in the last stretch of a run, and T1 within reach of a
fresh character — which fits the difficulty drop rates, since the level term in §2 means low-level
characters mostly see T1 anyway.

### ⚠️⚠️ COPYING THE VENDORS' `augtype` WOULD BREAK THESE

The stock weapon augs are **`augtype 8`**, and the socket test is
`(1 << (slot_type - 1)) & AugType` (`zone/inventory.cpp:356`) — so 8 matches socket type **4** only.

Socket types actually present on weapons here:

| socket type | weapons |
|---|---|
| **4** | 5,479 |
| **1** | 888 |
| 8 | 854 |
| other | 158 |
| *no socket at all* | 592 |

📌 **Type 1 is OUR sockets** — `aotv4_craft_sockets.sql` gives every craftable wearable type-1 sockets
(1/2/3 by tier), which is where a crafted or Mythic weapon's sockets come from. So `augtype 8` would
fit 5,479 stock weapons and **none of the crafted ones**, which are exactly the weapons players make
and keep.

**Use `augtype 255`** (all socket types 1-8), as the delve augs do. Combined with `augrestrict = 2`
that reads: *fits any socket, but only on a weapon* — which is the intent, and covers 7,379 of the
7,384 socketed weapons.

---

## 6. Open questions

1. ~~Weapon-only~~ **DECIDED: weapons only.** `augrestrict = 2` (`AugRestrictionWeapons`,
   `common/item_data.h:214`).
   ✅ **Verified SERVER-ENFORCED**, not a client-side hint: `Client::IsAugmentRestricted` is called
   from `zone/inventory.cpp:419` on the insert path, so a modified client cannot socket one into
   armour. That was worth checking — a client-only restriction would have needed a server guard added.
   📌 This is a real difference from the delve augs, which are `augrestrict 0` (anything) so they fit
   every socket on every piece. These deliberately do not.
2. ~~Crucible~~ **DECIDED: yes, DIRECT upgrades, FOUR inputs.** Four of the **same family and tier**
   become one of the **next tier in that family** — a Heal T1 stack makes a Heal T2, never a random
   effect. Deliberately unlike the delve augs, which upgrade to a random variant.
   ⚠️ T4 is the top: T4s are left alone rather than silently eaten.
   📌 Four is the agreed count. See §7 for what that costs in kills — it makes these a **long-term
   chase**, and it means the lever for pacing is the **drop rate or the tier weights**, never the count.
3. ~~Tradeable~~ **DECIDED: NOT tradeable.** ⚠️⚠️ That is **`nodrop = 0`** — the polarity is INVERTED
   (`Handle_OP_ShopPlayerSell` tests `if (!item->NoDrop) return;`), and it has caught this project
   three separate times. Writing the intuitive `nodrop = 1` would make them freely tradeable.
   📌 Pair with **`norent = 1`** — also inverted, and 1 is the one that means "survives logout".
4. ~~Survive death~~ **DECIDED: they do NOT survive.** So there is **nothing to add** to
   `death_loss.M.is_kept` — the wipe destroys them like any other carried gear, which is the default.
   ⚠️⚠️ The work here is making sure nobody later "fixes" this by adding the band to that list, so it is
   written down: hunted augments are meant to be lost, unlike the tradeskill tools (147930-147965)
   which ARE spared because tradeskill skill is the one thing death does not reset.
   📌 It also settles a knock-on: an augment socketed into a weapon is destroyed **with** the weapon,
   and §26 records the Crucible REFUSING an augmented item for exactly that reason. Nothing extra to
   build — but expect "I lost my T4 in the socket" reports, and the answer is that it is intended.
5. ~~Nightmare~~ **DECIDED: included**, at the rates in §2.
6. ~~Per-player or shared~~ **DECIDED: per player**, like ordinary individual loot. ⚠️ Note this means
   the effective server-wide rate scales with group size — a six-person group kills a named and rolls
   six times. Intended, but worth knowing when judging whether 5% feels right in play.
7. ~~Snare and Aggro magnitudes~~ **DECIDED** — see §3. Nothing is left open.

---

## 7. ⚠️⚠️ Direct upgrades change the farming maths by 7×

The delve augs upgrade to a **random** variant precisely because a targeted upgrade is punishing. Now
that these upgrade **within a family**, every input must match on family *and* tier, so the drop you
need is 1-in-7 of the drops you get.

Per named kill in Inferno at effective level 35 (the 5% ceiling), for **one specific family**:

| tier wanted | chance per named kill | named kills per drop | ×4 to upgrade |
|---|---|---|---|
| T3 (40% weight at 33+) | 0.29% | ~350 | **~1,400** |
| T1 (10% weight at 33+) | 0.07% | ~1,400 | ~5,600 |

**Four inputs is the agreed count.** These are therefore a long-term chase, not a steady ladder: a
T3→T4 step is on the order of 1,400 named kills at the top of the curve. That is a deliberate choice,
and it has two consequences worth holding onto.

1. **The pacing lever is the drop rate or the tier weights, never the input count.** If upgrades feel
   too slow in play, raise the 5% ceiling or shift the tier table toward high tiers at high levels —
   both move the numerator. Cutting the count would change the shape of the system instead of its pace.
2. **The T4 tier will be rare, and should read as rare.** With ~1,400 kills per step, most players will
   live on T2/T3 and a T4 will be an event. Size the T4 effect values so that is satisfying rather than
   mandatory — the ladder in §3 already does this (T4 is 2.5× T1, not 10×).

📌 The tier-roll table already concentrates high tiers at high effective levels, so the practical
numbers are better than the worst case above — the table is the honest floor, not the expectation.

---

## 8. Traps this must not walk into

- ⚠️⚠️ **Generate them with a FIXED SEED**, like `gen_delve_augs.pl`. Once these are on player
  characters, a regeneration with a different seed silently rewrites what people are wearing.
- ⚠️⚠️ **`items` and `spells_new` are SHARED MEMORY.** World down → `./shared_memory` → restart. A
  migration applying at boot is not enough.
- ⚠️ **Grant, do not drop, if possible.** §29 records the `EVENT_LOOT` ordering trap — the item is not
  in the player's bags when that event fires, so a conversion there double-granted Ink of the Lost.
  The delve chest places items at spawn instead; a named kill should place on the corpse at death or
  summon directly.
- ⚠️ **Individual loot (§31) rolls per eligible player.** Decide whether an aug is per-player (like
  ordinary loot) or one-per-corpse shared (like global loot / ink). Per-player multiplies the real
  drop rate by group size.
- ⚠️ **`items.Name` is varchar(64)** and silently truncates.
- ⚠️ **Verify an aug proc actually fires in play before generating 28 of them.** The code path exists
  (`attack.cpp:5603`) but nothing on this server uses it yet. Build **one** test augment first.

---

## 9. Not in scope

- Changing how delve augments work
- Weapon-restricted *stat* augments
- Any interaction between difficulties and delve rewards (they are deliberately separate — `/pick` is
  refused inside a delve)
