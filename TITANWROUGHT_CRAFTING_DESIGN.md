# Mold crafting — Titanwrought armour from a mold plus tradeskill materials

> ⚠️ **The line was renamed from "Defiant" to "Titanwrought" by migration v151** (1,117 rows: 417
> bases + 700 Hallowed/Mythic clones). Item **ids are unchanged**, so every recipe, loot table,
> merchant row and player inventory reference survives. Where this document mentions *Defiant* it is
> referring to the real EverQuest line this one was built from, which keeps that name.

Design only. Nothing built. Modelled on PoP elemental/ornate armour: find a **mold**, combine it with
a small set of **crafted materials**, get an armour piece.

---

## 1. What the data already gives us, before designing anything

| fact | why it matters |
|---|---|
| Titanwrought tiers (the renamed Defiant line) are **Crude 10 · Simple 20 · Rough 30** · Ornate 40 · Intricate 60 · Elaborate 70 · Elegant 80 (by `reclevel`) | ⚠️⚠️ **THE FIRST THREE ALREADY FIT A LEVEL 30 CAP AND NEED NO RESCALE AT ALL.** The plan assumed rescaling up front; it is only needed to go past Rough. |
| Every piece is **`classes = 65535`, `races = 65535`** | Already all-class/all-race. On a 16-class server (§14) that is exactly right, with no edits. |
| **28 armour pieces per tier** — Plate / Chain / Leather / Cloth × 7 slots (Bracer, Gauntlets, Boots, Helm, Sleeves, Legs, Chest) | A clean, complete grid to generate against. Another ~16 weapons/shields/charms per tier sit outside the grid. |
| 352 Titanwrought items sit in `lootdrop_entries`, but **ZERO are reachable** | ⚠️ Every one hangs off an NPC that does not spawn, or spawns only in a region-99 zone. **They are already off.** "Do not turn them on as drops" therefore needs no work at all — and crafting becomes the *only* source by default rather than by deletion. |
| **0 on merchants, 0 craftable** | Nothing to unpick; the crafting space is empty. |

📌 Stat scale at Rough (level 30): Breastplate 38 AC / 47 hp / 42 mana, Bracer 18 AC / 23 hp. That is
the yardstick for whether a tier feels like an upgrade.

## 2. Slot coverage and molds — DECIDED: every slot, and a mold per (slot, type)

### ⚠️⚠️ THE DEFIANT LINE ONLY COVERS SEVEN SLOTS. THE REST DO NOT EXIST AND MUST BE CREATED.

Measured against `Rough Titanwrought%` (was `Rough Defiant%`):

| | slots | present in Titanwrought? |
|---|---|---|
| **type-specific armour** | head 4 · arms 128 · wrist 1536 · hands 4096 · chest 131072 · legs 262144 · feet 524288 | ✅ 4 types each |
| | **shoulders 64** | ❌ **missing** |
| **accessories** | **face 8 · ears 18 · neck 32 · back 256 · fingers 98304 · waist 1048576** | ❌ **all missing** |
| charm | charm 1 | ✅ one |
| weapons | primary 8192 · 1H/2H 24576 · shield 16384 · range 2048 | ✅ ~11 shapes |

📌 **This is cheap to fix, because the balance script assigns the stats.** A new item only needs an id,
a name, a slot, an itemtype and an icon — §5's scaler allocates AC/HP/mana/stats from the budget, so
nothing has to be hand-balanced. **84 new gear items** — shoulders, back, face and waist at 4 types
each, plus ears, neck and fingers at 4 metals each, x 3 tiers — in the same custom band. (It was 30
while back, face and waist were counted as single untyped accessories.)

⚠️⚠️ **SUPERSEDED — measured, and three quarters wrong.** It read: *"Accessories are NOT
type-specific — there is no plate ring. Only real armour carries a type, and shoulders is the one
missing slot that does."* The ring half is right and the rest is not: **back, face and waist all carry
an armour type in stock EQ** and come from the same two skills as a breastplate. See the slot map
below. Only ears, neck and fingers are genuinely type-free, and they take a **metal** instead.

### ✅ SHOULDERS AND CLOAK ARE NOT EXCEPTIONS — measured, they are ordinary armour

The worry was that shoulders and back have no obvious tradeskill home the way armour, weapons and
jewelry do. **The stock recipe data says they do, and it is the same home as every other armour slot.**
Counting only real armour (`itemtype 10`, single-slot masks) from enabled recipes:

| slot | Blacksmithing | Tailoring | Jewelcraft | reads as |
|---|---|---|---|---|
| shoulders | 120 | 54 | 3 | **armour** |
| back | 112 | 58 | 3 | **armour** |
| face | 115 | 52 | 20 | **armour** |
| waist | 118 | 57 | 0 | **armour** |
| neck | 122 | 50 | **95** | mixed |
| ears | 0 | 0 | **53** | **jewelry** |
| fingers | 0 | 0 | **175** | **jewelry** |

Named examples settle it beyond the counts: Blacksmithing makes *Bloodling Plate Mantle*, *Banded
Mantle*, *Bloodling Plate Cloak*, *Banded Cloak*, *Cabilis Scale Cape*; Tailoring makes *Raw Silk
Mantle*, *Leather Shoulderpads*, *Mystic Cloak*, *Raw Silk Cloak*. **A plate cloak is a real thing in
EverQuest.** So shoulders and back carry an armour type exactly like a breastplate, and the tradeskill
falls out of the type with no rule of its own: **plate and chain to Blacksmithing, cloth and leather to
Tailoring.**

✅ **DECIDED: `back`, `face` and `waist` move OUT of accessories and into type-specific armour.**
That was the actual error — they were filed as accessories on the assumption that only "real" armour
carries a type, and the data disagrees for three of the six.

⚠️⚠️ **A COUNT IS NOT AN ANSWER UNTIL YOU CHECK THE `itemtype`.** A first pass reported Alchemy and
Make Poison as the top producers of shoulder items at **1,755 each**, which is nonsense. Two faults
stacked: the filter was `slots & 64` rather than `slots = 64`, so it swept up **multi-slot** items
(`slots = 1048936` = face+neck+shoulders+back+waist), and those items are **`itemtype 54`, augments**
— *Citrine of Aggression*, not a mantle. Filter to a single-slot mask **and** `itemtype 10` or the
answer is dominated by things that are not armour at all.

### The slot map — DECIDED

| group | slots | axis | count per tier |
|---|---|---|---|
| **armour** | head · shoulders · arms · wrist · hands · chest · legs · feet · **back** · **face** · **waist** | **4 types** — cloth · leather · chain · plate | 11 x 4 = **44** |
| **jewelry** | ears · neck · fingers | **4 metals** — silver · gold · electrum · platinum | 3 x 4 = **12** |
| charm | charm | none | **1** |
| weapons | by shape (§8) | see below | **~11** |
| | | | **~68 per tier, ~204 total** |

⚠️ **`neck` is the one genuine coin-toss** — Blacksmithing 122 (gorgets) against Jewelcraft 95
(necklaces). It is assigned to **jewelry** because the jewelry group needs three slots to be worth a
metal ladder, and a necklace is the archetypal neck item. Moving it to armour is defensible and costs
nothing but a row count.

### ✅ JEWELRY'S FOUR-WAY AXIS IS THE METAL LADDER, AND IT IS EQ'S OWN

Jewelry has no armour type, so the four-way split needs a different axis. EverQuest already has one:
Jewelcraft's **silver -> gold -> electrum -> platinum** progression. It exists in this database —
**76 silver, 43 golden, 36 electrum, 54 platinum** items in jewelry slots — so the names, the
components and the player's expectations are all already in place. Four metals, one per mold variant,
exactly parallel to the four armour types.

### ⚠️⚠️ WEAPONS CANNOT BE SPLIT FOUR WAYS BY SKILL — the data will not support it

Of the craftable weapons in this database, **Blacksmithing makes 641** and everything else is a
rounding error: Tailoring 20, Pottery 4, **Fletching 3**. There is no four-tradeskill weapon split to
be had, and inventing one means authoring recipes against skills that have never made a weapon.

Three ways to give weapons a four-way axis, none free:
1. ✅ **Four material lines inside Blacksmithing** (bronze / iron / steel / mithril, say). Preserves
   "work on all four lines" — the property actually wanted — without pretending four skills are
   involved. Costs no new skills and no new mold slots, since the material is chosen by the components
   rather than by the mold.
2. **Four weapon classes** (slash / blunt / pierce / ranged) replacing the ~11 shapes. Cuts weapon
   molds from ~11 to 4 per tier, but a "Blade Mold" no longer says whether you get a one-hander or a
   two-hander, which is the property §8 chose shape for.
3. **Leave weapons by shape and accept they do not split.** Simplest, and weapons are already the one
   group where a mold names the object precisely.

📌 **Recommended: (1).** It keeps §8's shape molds intact, keeps the four-line discipline the armour
types establish, and does not fight a 641-to-3 imbalance in the source data.

### The mold set — DECIDED: molds carry the TIER

| group | per tier | x3 tiers |
|---|---|---|
| armour — **11 slots x 4 types** | 44 | **132** |
| jewelry — **3 slots x 4 metals** | 12 | **36** |
| charm | 1 | **3** |
| weapons, by shape (§8) | ~11 | **~33** |
| **total** | **~68** | **~204** |

### ⚠️⚠️ TIERING THE MOLDS DOES **NOT** MAKE THE HUNT THREE TIMES WORSE — the tables never share

An earlier pass rejected slot x type x tier molds at 150+ on the grounds that *"at 7 molds a player
hunting a chest piece looked for 1 drop in 7; at 50 they are looking for 1 in 50, and 150 puts a
targeted set out of reach entirely."* **That arithmetic is wrong once the tiers are source-gated.**

Each tier drops from a **different activity**, so the three mold tables are never in the same loot roll:

| tier | table contains | so a targeted hunt is |
|---|---|---|
| Crude | the ~50 **Crude** molds only | 1 in ~50 |
| Simple | the ~50 **Simple** molds only | 1 in ~50 |
| Rough | the ~50 **Rough** molds only | 1 in ~50 |

A raider hunting a Rough breastplate is never handed a Crude one, because Crude does not drop in
raids. **The effective hunt is 1 in ~50 at every tier — identical to tier-agnostic molds.** The 150 is
a *build* cost (generated rows) and not a gameplay cost at all.
📌 It also costs no extra **recipes**: the gear is already tiered, so there is already one recipe per
output item. Tiering the mold only changes which mold that existing recipe asks for.

⚠️ The other half of the old objection — *"the mold IS the item and the crafting is a formality"* —
is a real concern and is **unchanged by tiering**. It applies equally to tier-agnostic molds carrying
(slot, type), and it is tracked as its own open question in §12: if the mold already names slot, type
and tier, the type-specific base and binding are saying something the mold has already said.

- **Bias the source** — ✅ ANSWERED (§9): the tier IS the source bias. Crude and Simple come from
  delves and open-world named mobs, Rough only from raids.
- **Make molds COMMON.** A mold is worthless without three crafted components and a temper, so a
  generous drop rate costs nothing — the tradeskilling is the real gate and the mold is only the key.

## 3. The materials — DECIDED: the mold plus THREE components

Four items in the combine. A forge holds 10 slots, so capacity is not a constraint.

### ⚠️⚠️ BIND COMPONENTS TO (ARMOUR TYPE, TIER) — NEVER TO THE SLOT

**DECIDED: the crafted components are specific to the armour type too.** The mold already names the
type, so this is thematic rather than mechanical -- but it is the right kind of redundancy: you do not
make a silk robe out of plate ingots, and a plate crafter's materials should be no use to a tailor.
📌 It also means each armour type is a genuinely separate craft path end to end, rather than four
molds feeding one shared material pool.

But they must **not** also be per slot. The mold already supplies the slot, so nothing else needs to
know about it, and that single constraint is what keeps the system buildable:

| crafted components bound to... | crafted materials | sub-recipes |
|---|---|---|
| **(type, tier)** ✅ | 2 x 4 x 3 = **24** | 24 |
| (type, tier, **slot**) ✗ | 2 x 4 x 3 x 7 = **168** | 168 |

📌 A Chest Mold plus *plate* materials gives a breastplate; the same mold plus *silk* materials gives a
robe. That is the property that keeps seven armour molds enough to cover 28 pieces per tier.

### The three components

| role | specific to | count | comes from |
|---|---|---|---|
| **Base** — Plate Ingot · Chain Links · Cured Hide · Silk Bolt, one per tier | **type + tier** | **12** | Blacksmithing · Tailoring |
| **Binding** — the second craft, deliberately a different skill from the base | **type + tier** | **12** | Jewelcraft · Alchemy · Pottery |
| **Temper** — the tier gate | **tier only** | **3** | ✅ *bought* |

⚠️ **The temper is type-agnostic ON PURPOSE, and that is not an inconsistency.** The rule is that the
*tradeskill* items are type-specific; the temper is the one component that is **not** a tradeskill
item. A flux or solvent is generic in a way an ingot is not, and keeping it shared is what stops the
bought ladder becoming twelve near-identical vendor entries.
📌 If you would rather it were type-specific too, it becomes 12 vendor items instead of 3 — no harder
to build, just a longer merchant list.

### ✅ Why the third component is BOUGHT rather than crafted

1. ⚠️⚠️ **ITS PRICE IS A PER-TIER DIFFICULTY GATE THAT COSTS NOTHING TO BUILD.** `items.price` is the
   only price lever there is (merchantlist has no price column), and v149 just proved the ladder works:
   the conjured materials run 0.5p Silver -> 10p Gold -> 100p Platinum. Price the three tempers on that
   curve and the tiers gate themselves with **no new mechanic at all**.
2. **There is coin to sink.** The delve pays **10 platinum per rung**, doubled under an affix -- a
   rung-30 clear is ~300p. At 100p a Rough temper, a full seven-piece Rough set costs ~700p in bought
   components: two or three clears. Meaningful, not a wall.
3. **A non-crafter can contribute.** Someone with coin and no tradeskill can buy tempers for the
   crafter in their group -- otherwise this system offers them no way in at all.

⚠️ **Buy the TIER, craft the TYPE.** If the bought item decided armour type instead, Blacksmithing and
Tailoring would lose the only thing that gives them a role here.

#### ⚠️⚠️ THE MATERIAL COUNT GREW WHEN JEWELRY AND WEAPONS GOT FOUR LINES EACH

§3's **27 materials** (12 base + 12 binding + 3 temper) was costed when only *armour* had a four-way
axis. Jewelry now has 4 metals and weapons 4 material lines (§2), and costing all three groups the same
way — base **and** binding per line — gives **75 materials and 72 sub-recipes**. That is a grind, not a
system: three separate four-line ladders, each needing two crafts per tier.

✅ **DECIDED: the BASE carries the four-way identity; the BINDING is per GROUP.**

| | axis | count | skill |
|---|---|---|---|
| **base** — armour | 4 types x 3 tiers | 12 | cloth/leather **Tailoring**, chain/plate **Blacksmithing** |
| **base** — jewelry | 4 metals x 3 tiers | 12 | **Jewelcraft** |
| **base** — weapons | 4 metals x 3 tiers | 12 | **Blacksmithing** |
| **binding** | 1 per group x 3 tiers | **9** | armour **Pottery** · jewelry **Alchemy** · weapons **Jewelcraft** |
| **temper** | 1 per tier | 3 | ✅ bought |
| | | **48 materials, 45 sub-recipes** | |

📌 **Why this split and not the reverse.** The four-way axis exists to make a player work all four
lines, and that only bites on the component that *names* the line — the base. A per-line binding would
triple the ladder while saying the same thing four more times. Putting the binding at group level
instead is what keeps Pottery, Alchemy and Jewelcraft in the system: every crafter touches all three
regardless of which line they are working.
📌 So a full kit still requires **five** tradeskills — Tailoring and Blacksmithing for bases, Pottery,
Alchemy and Jewelcraft for bindings — which was the point of the four-line rule in the first place.

### Material count

**27 materials** (12 base + 12 binding + 3 temper) + **~204 molds** (tiered, §2) + **84 new gear
items** = about **315 new items**, id band 148000+, plus **24 sub-recipes** for the crafted materials.
⚠️ Every one is a **generated row**, not a hand-authored item — §2's note that the balance script
assigns the stats is what makes a number this size routine rather than alarming.

⚠️ Every one of those is generated, not hand-written -- see §7.

## 4. Which tradeskill performs the combine, and in what

PoP used the forge for plate/chain and the loom for cloth/leather. Same here: the **container decides
the skill**, which comes free — no code, just the recipe's container entry.

⚠️⚠️ **THE TRIVIAL IS NOT JUST A DIFFICULTY GATE — IT SETS THE ITEM'S POWER.** See §5a. An earlier
draft of this document suggested trivials of 50 / 120 / **200**; 200 would have produced quality 11 and
a budget of **1,331**, more than **three times** the best non-raid item in the game. Do not pick
trivials by feel.

### ✅ RECIPE COVERAGE — every combine must exist, and be FINDABLE

A generated system fails quietly in two different ways: a mold with no recipe is an item that cannot be
used, and a recipe the search window hides is a recipe nobody knows to look for. **Both must be checked
at build time, because neither produces an error.**

**Scale: 228 recipes.** One per output item — **204 final combines** (68 per tier x 3) plus **24
material sub-recipes** (12 base + 12 binding). The tempers are bought and need none.

#### The skill and container map — DECIDED, and read off the stock data

| group | type | tradeskill | containers to list |
|---|---|---|---|
| armour | cloth · leather | **61 Tailoring** | Loom **16** + Kit **990061** |
| armour | chain · plate | **63 Blacksmithing** | Forge **17** + Kit **990061** |
| jewelry | all 4 metals | **68 Jewelcraft** | Jeweler's Kit **20** + Kit **990061** |
| weapons | all | **63 Blacksmithing** | Forge **17** + Kit **990061** |

⚠️ A recipe may list **many** acceptable containers — stock Tailoring recipes list the loom, the
universal kit and two dozen sewing kits at once — so naming two costs nothing and never narrows a
player's options.

✅ **PUT `Artisan's Universal Kit` (990061) ON EVERY RECIPE.** Measured, it is genuinely universal:
it appears as a valid container for **all 13 tradeskills** (156 to 4,047 recipes each). It is also
**obtainable from the start** — 1 platinum, sold by 793 merchants including **10+ in the hub**
(`freeporttheater`), among them Audri_Deepfacet (202069), the jewelcrafting vendor v149 already
touched. `nodrop = 1` (tradeable, the inverted flag) and `norent = 1` (permanent).
📌 That single entry is what guarantees "people can in fact make them": a player who cannot find a
forge, or is standing in the hub, can still perform every combine in the system for a one-platinum
purchase. **Without it the whole system is gated behind world objects in zones region locking may
close.**

#### ✅ THE MOLD IS CONSUMED, AND NO COMBINE EVER FAILS

**DECIDED: a successful combine consumes the mold, and every combine succeeds.** Skill does not decide
*whether* you get an item — it decides *how good* the item is.

| entry | componentcount | successcount | failcount |
|---|---|---|---|
| **mold** | 1 | **0 — consumed** | — |
| base material | 1 | 0 | — |
| binding material | 1 | 0 | — |
| temper | 1 | 0 | — |
| **the gear item** | 0 | 1 | — |

**`nofail = 1` on every recipe in this system.** Verified in `Client::TradeskillExecute`
(`zone/tradeskills.cpp:1328`): `if (spec->nofail) { chance = 100; }` — *"This combine cannot fail"*.

⚠️⚠️ **`nofail` DOES NOT COST YOU SKILL-UPS, WHICH IS THE THING THAT LOOKS LIKE IT SHOULD.**
`CheckIncreaseTradeskill` sits **inside** the success branch (`:1381`) and is gated only on
`over_trivial < 0` — the ordinary EQ rule that you skill up on recipes below their trivial. A combine
that cannot fail always takes that branch, so it always gets the roll. **Crafting this line still
raises the skill that decides its tier.**
📌 The one thing `nofail` genuinely skips is the **salvage** list (`:1949`, *"Don't bother with the
query if TS is nofail"*), which is meaningless when nothing can fail. `onfail` is likewise dead. Do not
author either for these recipes.

⚠️ **`skillneeded` still bites and is now the ONLY hard gate.** `if (spec.skill_needed > 0 &&
user->GetSkill(spec.tradeskill) < spec.skill_needed)` refuses with *"You are not skilled enough."*
(`:707`). That is what stops a skill-1 crafter making Rough gear the moment they hold the parts.

##### ⚠️⚠️ THIS REPLACES THE FAILURE LADDER, SO §9's DIFFICULTY GATE HAD TO MOVE
§9 reasoned that *"material trivials are a free difficulty ladder"* — true only while a combine can
fail. With `nofail` a trivial no longer gates anything by risk. The ladder is now two things, and both
are stronger than the failure it replaced:

| lever | effect |
|---|---|
| **`skillneeded`** | a hard floor — you cannot attempt the combine at all |
| **the tier roll** | a soft reward — low skill yields native, high skill yields Mythic |

✅ **The tier roll is already built** and needs no new code: `AoTv4RollCraftTier`
(`zone/tradeskills.cpp`, at the `GetTradeRecipe` choke point, §32) reads effective tradeskill and picks
native / Hallowed / Mythic — 0% Mythic at skill 100, 27% at 200, 77% at 300, 100% at 345. **That IS
"more successful depending on your skill".**

⚠️ **Verified it is not double-applied**: the success path calls the raw `Client::SummonItem`
(`:1394`), and `AoTv4MythicReward` — §26's silent Mythic upgrade — is wired only into the **Lua**
bindings and `QuestManager::summonitem`, never into `Client::SummonItem`. So the roll at
`GetTradeRecipe` is the sole decider. **If that upgrade is ever moved into `Client::SummonItem`, this
whole system collapses to always-Mythic and the skill roll becomes decorative.**

⚠️ Molds carry `slots = 0`, so `aotv4_gear_tiers.sql` — which only generates tiers for `slots > 0`
items (§10) — will not clone them, and a mold can never itself be Hallowed or Mythic. Keep it that way.

##### ⚠️⚠️ CONSEQUENCE: MOLD DROP RATES ARE NOW LOAD-BEARING, NOT COSMETIC
A consumed mold is a **charge, not a pattern**. Combined with the roguelite death destroying crafted
gear (§9), a player re-crafts their kit every run and **spends a mold per piece, every time**. §2's
*"make molds COMMON — a mold is worthless without three crafted components and a temper, so a generous
drop rate costs nothing"* stops being a nicety and becomes the thing that decides whether the system is
usable at all.
- ⚠️⚠️ **SUPERSEDED 2026-08-30: molds do NOT survive death.** This paragraph argued the opposite --
  that they were "the banked stock the next run is rebuilt from" and so had to be exempt. The call went
  the other way: only items that **cannot be re-earned** are spared the wipe, and a mold can always be
  found again. Combined with consumption-on-combine, a mold is a per-run resource end to end.
  📌 Which makes the drop rate matter *more* than this section already said, not less -- a player now
  needs molds every run rather than a stock that lasts several.
- ⚠️⚠️ **Watch the Rough tier especially.** Rough molds drop only from raids and are consumed per
  piece, so a full raid set costs as many raid mold drops as it has slots. Either raids drop several
  molds, or a set takes many lockouts — and the lockout is **24 hours**. Size the raid mold drop
  against the slot count, not against "one nice reward".

#### ⚠️⚠️ THE FINDABILITY TRAP — a recipe can exist, work, and still be invisible

§32 records this costing 388 recipes: `tradeskill_recipe` carries its **own**
`min_expansion`/`max_expansion`, and the recipe **search** ends with `ContentFilterCriteria::apply()`
while the **combine** path filters on `tr.enabled` alone. A gated recipe therefore **still works if you
already know the components** and simply never appears in the recipe window.
- For 388 stock PoP recipes that was merely annoying. **For a brand-new system it is fatal** — nobody
  can know components that have never existed before.
- ✅ Both columns **default to -1** in the schema, which is the safe value, so a plain INSERT is
  correct. The risk is a generator that clones a stock recipe row and inherits its gates — the same
  class of bug as §5's cloned damage formula and §51's inherited `descnum`.
- ⚠️ Likewise leave **`must_learn = 0`** (the default). Setting it hides the recipe until a scroll is
  found, which §32 already rejected for this system: *a recipe scroll hides the recipe until earned, so
  you cannot see what you are working toward.*

#### The four invariants — check them after every generator run

Nothing in the engine enforces any of these, and each fails silently:

1. **Every mold is used** — each of the ~204 mold ids appears as a component in >= 1 `enabled` recipe.
2. **Every gear item is made** — each of the 84 new gear ids is the output of >= 1 `enabled` recipe.
3. **Every recipe has a container** — >= 1 entry with `iscontainer = 1`.
4. **Every recipe is findable** — `min_expansion = -1`, `max_expansion = -1`, `must_learn = 0`.

```sql
-- 1 + 2: orphans in the Titanwrought band (adjust the band to match the generator)
SELECT i.id, i.Name,
       (SELECT COUNT(*) FROM tradeskill_recipe_entries e JOIN tradeskill_recipe r ON r.id=e.recipe_id
        WHERE r.enabled=1 AND e.item_id=i.id AND e.componentcount>0) AS used_as_component,
       (SELECT COUNT(*) FROM tradeskill_recipe_entries e JOIN tradeskill_recipe r ON r.id=e.recipe_id
        WHERE r.enabled=1 AND e.item_id=i.id AND e.successcount>0)   AS made_by_recipe
FROM items i WHERE i.id BETWEEN 148000 AND 148999
HAVING used_as_component = 0 AND made_by_recipe = 0;

-- 3 + 4: recipes that cannot be combined, or cannot be found
SELECT r.id, r.name, r.min_expansion, r.max_expansion, r.must_learn,
       (SELECT COUNT(*) FROM tradeskill_recipe_entries e WHERE e.recipe_id=r.id AND e.iscontainer=1) AS containers
FROM tradeskill_recipe r WHERE r.name LIKE 'Titanwrought%'
HAVING containers = 0 OR r.min_expansion <> -1 OR r.max_expansion <> -1 OR r.must_learn <> 0;
```

📌 `custom/tools/tradeskill_reachability.py` already answers the deeper question these queries do not —
whether every *component* is obtainable at all. Run it after the generator: the base and binding
sub-recipes are new, so their own components are the part most likely to reference something this
server cannot supply.

## 5. ⚠️⚠️ THE RESCALE — required, and sized backwards from Mythic

**DECIDED: skill decides the output tier (§32's existing roll), so Mythic is the intended END
STATE.** That is what makes the rescale mandatory rather than eventual: an unscaled Mythic Rough
Breastplate would be ~76 AC and ~118 hp at a level 30 cap.

### Why the line needs rescaling at all

Measured, per equippable item:

| | AC | HP | mana | stat points |
|---|---|---|---|---|
| **obtainable today** (2,151 items, mobs ≤30, reachable zones) | 6.3 | 5.0 | 4.5 | 5.9 |
| Crude Titanwrought (rec 10) | 4.5 | 8.5 | 8.8 | **32.5** |
| Rough Titanwrought (rec 30) | 13.5 | 29.8 | 29.9 | **62.3** |
| Rough as a multiple of today | ×2.1 | ×6.0 | ×6.6 | **×10.6** |

⚠️⚠️ **A SINGLE RESCALE FACTOR WOULD BE WRONG.** AC is only twice what players already have; stats are
**ten times**. Scale everything by one number and either AC collapses to nothing or stats stay absurd.
📌 Even **Crude** — nominally level 10 gear — already carries **5.5× the stat points** of anything
obtainable today. The problem is not "high tiers are too strong", it is that the whole Titanwrought line is
authored for a game with a much flatter stat curve than this one.

⚠️⚠️ **THE PROBLEM IS NOT THAT THE HIGH TIERS ARE TOO STRONG.** Even *Crude* — nominally level 10
gear — already carries **5.5x the stat points** of anything obtainable today. The whole Titanwrought line
is authored for a game with a much flatter stat curve than this one.

📌 An earlier draft of this section proposed hand-picked per-axis scale factors (AC 0.58, HP 0.17,
stats 0.12). That is superseded: the scaler below reallocates every stat from a single budget, so
there is nothing to pick per axis.

### ✅ HOW TO HIT "10-15% BETTER THAN NON-RAID" — with the scaler we already have

`aotv4_scaling/item_scaling_main.py` is a **budget allocator**: every stat has a point cost
(AC 0.5, STR 1, HP 0.20, Mana 0.20) and each item gets a budget to spend on them.

```
quality  ->  budget = floor(quality ** 3)  ->  stats allocated from the sheet
reclevel = max(0, floor(quality - 2) * 5)
```

Quality comes from how the item is **acquired** — drop, tradeskill, or vendor — and the item takes the
**minimum** of those. Titanwrought is craft-only, so exactly one applies:

```
tradeskill_quality(trivial) = 1.0 + trivial / 20
```

✅ **So the whole lever is the recipe's trivial.** No new code, no special case, no hand-edited stats:
set the trivial, run the existing scaler, and the item comes out on-budget.

**Measured target.** Across 2,088 non-raid equippable items obtainable from mobs ≤30 in reachable
zones, the best is quality **7.485**, budget **419**.

⚠️⚠️ **BUDGET IS NOT POWER, AND CONFUSING THE TWO UNDERSHOOTS BADLY.** `stat_point_cost` is
`base x (current_value + 1)` -- every additional point of a stat costs more than the last -- so the
total cost of N points is roughly quadratic and **actual stats scale as the SQUARE ROOT of budget**.
Combined with `budget = quality ** 3`, there are three different scales in play and "+15%" means a
different thing at each stage. **Always quote power in STAT POINTS.**

| budget | Δ budget | **Δ actual stats** | trivial | reclevel |
|---|---|---|---|---|
| 419 — best non-raid today | — | — | 130 | 25 |
| 461 | +10% | **+4.6%** | 135 | 25 |
| 482 | +15% | **+7.3%** | 137 | 25 |
| **512** | +22% | **+10.1%** | **140** | **30** |
| **560** | +34% | **+15.6%** | **145** | **30** |

✅ **DECIDED: keep reclevel 30, which costs nothing.** A first pass here recommended budget +10-15%
and reclevel 25 on the grounds that reclevel 30 was "outside the band" -- that was measuring the wrong
thing. In stat points, reclevel 30 (trivial 140) IS +10%, and the whole 10-15% band sits at reclevel 30:

  **trivial 140 -> +10% · trivial 145 -> +15.6%**

On a representative armour piece, +10% looks like this:

| | AC | HP | Mana | STA/STR/WIS | total pts |
|---|---|---|---|---|---|
| best non-raid today (419) | 14 | 37 | 37 | 7 each | 109 |
| Rough Titanwrought (512) | 16 | 40 | 40 | 8 each | 120 |

⚠️ Representative, not exact: this is an equal-weighted six-stat spread, while the real allocator picks
stats per item with scores. Increments are also lumpy at this scale -- AC moves in whole points, so a
small budget change shows as either nothing or a whole step.

📌 **The "best non-raid item" is a PLATEAU, not a peak.** Best and 99th percentile are both exactly
7.485, because the formula tops out at `30//5 + 1.35 x 1.10`. Plenty of gear already sits there, so
this is a modest target rather than a stretch goal.

**The tier ladder**, from the same formula:

| tier | trivial | quality | budget | reclevel |
|---|---|---|---|---|
| Crude | **60** | 4.0 | 64 | 10 |
| Simple | **100** | 6.0 | 216 | 20 |
| Rough | **130** | 7.5 | 419 | **30** |   ⬅ parity; the premium is applied on top, see §10

📌 That is a genuinely steep ladder (64 -> 216 -> 483) because of the cube, which is what makes each
tier feel like a real step rather than a re-skin.

⚠️⚠️ **THE TIER CLONES MUST BE REGENERATED IN THE SAME PASS.** Hallowed and Mythic rows are generated
FROM the base by `aotv4_gear_tiers.sql`. Rescale the bases and leave the clones and roughly half of
them are derived from numbers that no longer exist — §35 records exactly this after the 0.1.2 item
rework, where 3,166 Mythics ended up with LESS AC than their own base while the row counts stayed
perfectly healthy. **Row counts prove nothing; check the RATIO.**

## 6. Drops — nothing to do

**DECIDED: no drops.** Already true: zero Titanwrought items are reachable, so crafting is the only source
without deleting anything. ⚠️ The 352 `lootdrop_entries` rows are left alone deliberately — they are
inert while their NPCs do not spawn, and removing them would be a destructive edit for no gain. If a
future zone unlock ever makes one of those NPCs live, this becomes real; the reachability query in
§1 is the check.

## 7. Shape of the build

| piece | count | where |
|---|---|---|
| molds | **32 armour** (8 slots x 4 types) + **6 accessory** + charm + ~11 weapon = **~50** | new items, id band **148000+** |
| new gear items | **84** — shoulders/back/face/waist x 4 types, plus ears/neck/fingers x 4 metals, x 3 tiers | Titanwrought has no items for those slots |
| base materials | 3 | new items or existing tradeskill items |
| tempers | 3 | new items, one per tier |
| recipes | **228** — 204 final combines (68 per tier x 3) + 24 material sub-recipes | **generated**, not hand-written; see the coverage invariants in §4 |
| loot rows | per region | migration |

⚠️⚠️ **GENERATE THE RECIPES, DO NOT TYPE THEM.** 84 recipes × ~4 entries each is 336 rows, and the
grid is perfectly regular. §26's `gen_delve_augs.pl` is the pattern — and its lessons apply verbatim:
**sort every `keys` whose order can reach the output** (Perl hash order is per-process random and made
that generator non-deterministic across runs), and **verify by regenerating and diffing.**

⚠️ `items` is shared memory — new molds and materials need the stack down and `./shared_memory`.
Recipes are not: `tradeskill_recipe` is read at combine time, so those need only a zone restart.

## 8. Weapon molds — by SHAPE, which is the same rule as armour

**DECIDED: weapons are included.** But slot-only does not survive contact with them. Armour is a clean
grid — 7 slots × 4 types, so a Chest Mold plus plate materials gives a breastplate and plus silk gives
a robe. Weapons are **11 shapes in 3 slots** (Longsword, Shortsword, Hammer, Halberd, Trident, Bow,
Scepter, Knuckle, Spiked, Serrated, plus a shield and a charm).

Slot-only for weapons would mean a single "Primary Mold" and **eleven material variants** to say which
shape came out — pushing the complexity into materials, where it is worse.

✅ **DECIDED: weapon molds by SHAPE (~11).** With armour molds now carrying slot AND type, this is the
same rule rather than an exception: **a mold names the finished object closely enough that a player
knows what they are hunting for.** For armour that is slot plus type; for a weapon it is its shape.

## 9. ⚠️⚠️ THE ROGUELITE DEATH DESTROYS CRAFTED GEAR — which is what the tiers are FOR

`death_loss.M.process` wipes every carried and equipped item on death. The only survivors are the
tradeskill tools (147930-147965), the Fellowship Insignia, **evolving items**, and epics.

**So a crafted armour set lasts exactly one run.** That is not a flaw to design around — it is the
thing that makes a three-tier crafting system make sense at all, and it inverts the usual assumption
about which tier matters:

| tier | rec | when it is worn | how often |
|---|---|---|---|
| Crude | 10 | early in every run | **constantly** |
| Simple | 20 | mid run | **constantly** |
| Rough | 30 | at cap | least often, unless parked at cap |

📌 **The LOW tiers get the most use, not the least.** A character spends most of its life climbing
1 -> 30 over and over, so Crude and Simple are the working gear and Rough is the trophy. Any tuning
that treats Crude as a throwaway starter tier has it backwards.

### ⚠️⚠️ REVERSED 2026-08-30: NOTHING SURVIVES DEATH -- NOT THE GEAR, NOT THE MOLD

This is the single most important consequence, and it makes the whole system coherent:

| | survives death? | why |
|---|---|---|
| **Mold** | ❌ **no** — DECIDED, and an earlier pass had this backwards | Only things that **cannot be re-earned** are exempt: the tradeskill tools (their achievements are `claim_once`) and the fellowship insignia. A mold can always be found again, so it takes the wipe like everything else. |
| **Materials** | ❌ no | Renewable by design — bought or crafted, cheaply, every run. |
| **Gear** | ❌ no | The consumable. Re-crafted each run from molds you kept and materials you replace. |

⚠️ `is_kept` is an explicit id band plus two single ids, with a comment saying the next kept item gets
its **own line** rather than silently widening an existing one. Follow that: add a molds band, do not
stretch `TS_ITEM_FIRST..TS_ITEM_LAST`.

📌 This also answers what a **raid** is worth on a server where the reward would otherwise evaporate on
the next death: a raid drops **molds**, which are permanent. The gear the mold enables is temporary;
the right to make it is not. Same logic the Delver's Sigil already uses — it survives because it is an
evolving item, and it is the only durable reward in the game today.

### ✅ WHAT GATES A TIER: THE MOLD SOURCE, AND THE MATERIAL DIFFICULTY

⚠️ **It is NOT the final combine's trivial, and it cannot be.** `trivial` does double duty: it is the
difficulty gate *and* it sets the item's power (`quality = 1 + trivial/20`). The trivials are already
pinned at 60/100/140 by the power target, so spreading them to gate difficulty would spread the power
with it — trivial 220 is quality 12, budget **1,728**, four times the best item in the game.

✅ **DECIDED: the gate is the MOLD SOURCE, plus how hard the MATERIALS are to make.**

| tier | mold drops from | material difficulty |
|---|---|---|
| **Crude** | delve rungs **1-10** + named mobs **1-15** | low |
| **Simple** | delve rungs **11-20** + named mobs **14-30** | medium |
| **Rough** | ⚠️⚠️ **RAIDS ONLY — never a delve, never the open world** | **hardest** |

⚠️⚠️ **THE MATERIAL SUB-RECIPES HAVE THEIR OWN TRIVIALS, AND THAT IS THE LEVER THE FINAL COMBINE
CANNOT BE.** A material's trivial gates *making the material*; it does not touch the finished armour's
quality, because the scaler reads quality from the recipe that produces **that item** — for a
breastplate, the breastplate recipe (140), not the ingot's. So material trivials are a free difficulty
ladder that leaves the power table untouched. **This is the one place difficulty and power are
separable, and it is why the tier gate lives here.**

📌 **Every tier follows the SAME pattern** — mold + base + binding + temper, with lower level
requirements further down. Crude is not a simplified recipe; it is the same recipe with easier inputs,
so a player learns the system once.

📌 Molds are **destroyed by death** like everything else, and are **consumed on a successful combine**
(§4), so a mold is a per-run resource end to end -- not a collection, not even a balance that carries
over. Two earlier drafts here were wrong in sequence: first that it was a permanent collection that
"grows across the whole account", then that it merely survived death as banked stock. Neither holds.
⚠️⚠️ **THE ROUGH MOLD IS THE SHARP CASE AND IS THE FIRST THING TO WATCH IN PLAY.** It drops only from
a raid boss on a **24-hour lockout**, so dying while holding one destroys something that cannot be
replaced for a day. The intended loop is therefore **raid, then craft before you die** -- never bank
molds for later. If that proves too punishing the fix is one line in `death_loss.M.is_kept`. Crude is
not a stepping stone you outgrow in ten levels — it is what you can make **before you have beaten a
raid**.

⚠️ **OPEN: rungs 21-70 currently drop no molds at all** under this split. Either Simple keeps dropping
above rung 20 (the best the delve can offer), or the delve stops rewarding molds entirely past 20 --
worth deciding, because rung 30 is where a capped character actually farms.

### ⚠️⚠️ ONLY THE RAID TIER BEATS WHAT YOU CAN FIND — the lower tiers sit at PARITY

A first pass set trivials at 60/100/140 across the board. Measured against the best *findable* item at
each tier's level, that made crafted gear better than anything in the world at **every** tier, and
worst at the bottom:

| tier | best findable (budget) | trivial 60/100/140 | crafted is |
|---|---|---|---|
| Crude | 42 | 64 | **+23.4%** |
| Simple | 162 | 216 | **+15.5%** |
| Rough | 419 | 512 | +10.5% |

⚠️⚠️ **THAT MAKES DELVES THE ONLY ROUTE TO GOOD GEAR.** Crude and Simple molds drop in the delve, so if
crafted gear also beats every world drop, there is no reason to loot anything, anywhere, ever — and
the margin being *largest* at Crude means it bites hardest on a brand-new character, before they have
seen any of the game.

✅ **DECIDED: parity below, premium only at the raid tier.**

| tier | trivial | vs best findable | what crafting buys you |
|---|---|---|---|
| Crude | **50** | **parity** | **certainty** — the exact slot, on demand |
| Simple | **89** | **parity** | **certainty** |
| **Rough** | **130** | **parity**, +10/12/15% premium applied after (§10) | **power** — raid-gated, best in slot |

📌 **The design statement: crafting buys CERTAINTY at the low tiers and POWER only at the raid tier.**
World drops stay fully competitive while levelling; the crafted set is the one you can aim at a slot
you keep getting unlucky in. Only beating a raid moves you past what the world can give you.

📌 **Certainty is worth the 100p/300p even at parity**, and more than it looks: the accessory slots
(§2) have thin drop coverage, and gear dies on death, so a reliable re-kit is worth real money even
when each piece merely matches a lucky find.

### ⚠️ AND MOLDS MUST NOT ALL COME FROM DELVES

Parity fixes the power problem but not the *access* one. If every mold drops in a delve, the delve is
still the only path — it just stops being a strictly better path.

✅ Give each tier a different activity, so three ways to play feed three tiers:

| tier | mold source |
|---|---|
| **Crude** | delve rungs **1-10** — **and** open-world **named** mobs, levels 1-15 |
| **Simple** | delve rungs **11-20** — **and** open-world **named** mobs, levels 14-30 |
| **Rough** | ⚠️⚠️ **RAIDS ONLY. Never a delve, never the open world.** |

⚠️ **Both low tiers deliberately have two sources**, delve and world. They are what a character meets
first, and gating the entry point to the whole crafting system behind one activity is how a system
ends up unused. The delve is the *reliable* route (a chest, on a known rung) and named mobs are the
*opportunistic* one, so neither activity is mandatory.

⚠️⚠️ **ROUGH HAS EXACTLY ONE SOURCE AND THAT IS THE POINT.** It is the only tier that beats what the
world can give you (§ above), so any second route to a Rough mold is a route around the raid. It gets
no `global_loot` row of any kind — a row with `raid = 1` would be *nearly* right and is still wrong,
because `IsRaidTarget()` is a per-NPC flag that any future content could set. Rough molds are placed
by the **raid encounter itself**, in `aotv4_raid.lua`, where the only thing that can drop one is a
boss this server deliberately built.

### ⚠️⚠️ NAMED MOBS DROP MOLDS — and the `rare` flag is the right test, not the naming heuristic

✅ **DECIDED: a small chance off named mobs in every unlocked region**, on top of the delve rungs. It
is the source that makes the mold hunt a reason to play the open world rather than a reason to farm
one activity, and it is the second source §2 already called for at the entry tier.

**Mechanism: a `global_loot` row with `rare = 1`.** No code — `global_loot` attaches a loot table to
every NPC matching a filter, independent of that NPC's own `loottable_id`, and it is already how this
server drops the Titanwrought line itself and every tradeskill material line (`GLB-Smithing`,
`GLB-Tailoring`, `GLB-SpellResearch`, `GLB-PoisonMaking`). Filters available: `min_level`, `max_level`,
`rare`, `raid`, `race`, `class`, `bodytype`, `zone`, `hot_zone`, expansion and content flags.
`GlobalLootEntry::PassesRules` (`zone/global_loot_manager.cpp:117`) evaluates them; the `Rare` case is
*"value == 0 must not be rare, value != 0 must be rare"*, reading `Mob::IsRareSpawn()` —
`inline bool IsRareSpawn() const { return rare_spawn; }` (`zone/mob.h:933`), straight off `npc_types`.

⚠️⚠️ **DO NOT USE THIS PROJECT'S USUAL NAMED HEURISTIC HERE.** §17c, `quest_difficulty.pl` and the
delve ledger all define "named" as *lowercase is trash, proper noun or `#` prefix is named*, and that
is correct **inside a dungeon**. Applied to the open world it is badly wrong: measured against this
database, the heuristic matches **17,747** NPC types against `rare_spawn`'s **568**, and the excess is
almost entirely **civic** — guards, merchants, bankers and quest givers all carry proper names. A mold
table keyed on the naming heuristic would make Freeport guards a farming route. The delve can use the
heuristic safely only because a dungeon contains no civic NPCs.
📌 So the two definitions of "named" disagree by 31x and **each is right in its own place**. Anything
that classifies NPCs in the open world wants `rare_spawn`; anything inside a delve wants the name.

**Coverage, measured** — `rare_spawn = 1` NPCs that are actually spawned, in the 1-25 band:

| region | rare NPCs |
|---|---|
| 3 | 37 |
| 5 | 35 |
| 1 | 19 |
| 4 | 18 |
| 6 | 18 |
| 2 | 9 |
| 0 | 3 |
| ~~99 "Unused"~~ | ~~27~~ — unreachable, harmless |

**139 across the six playable regions**, 9-37 each, so every region has a mold route. Thin enough that
a mold off a named mob reads as a find rather than a tax on every kill.

⚠️ **A per-zone count is misleading here and nearly sent this the wrong way.** Grouping by zone shows
velketor 12, frozenshadow 6, greatdivide 5 and reads as "clustered in Velious"; grouping by *region*
shows it is spread. Group by the axis the gate actually uses.

⚠️⚠️ **THE LEVEL BAND MUST BE WRITTEN AS TWO RULES, AND `max_level` IS THE ONE THAT BITES.** A row with
only `min_level` matches everything above it — the existing `AoTv4 Ink of the Lost` row (id 100) is
exactly that, `min_level 10, max_level 0`. Fine for ink; for molds it would put the top tier on every
named mob in the game.

⚠️⚠️ **GLOBAL LOOT ROLLS AT SPAWN AND STAYS SHARED (§31), SO A GROUP GETS *ONE* MOLD.** Individual loot
rolls the table once per eligible player, but global loot is deliberately excluded — §29 records the
reasoning: *"a 1.5 percent Ink of the Lost multiplied by group size is a different drop rate."* That is
right for a common drop and wrong for a hunted one: six people kill a named mob, one mold appears, and
five of them watch someone else take it. **Decide before building** — either accept it (the mold is
common, another will come) or grant molds per player in Lua from `global_npc.event_death_complete`,
which `npc:IsRareSpawn()` supports directly since it is Lua-bound (`zone/lua_npc.cpp:1080`).
📌 If it goes to Lua, grant with **`SummonItemExact`** — every ordinary `SummonItem` silently upgrades
to Mythic (§26).

### ✅ RESOLVED: THE MOLD CARRIES THE TIER (§2 corrected)

This request landed on a genuine contradiction — §2 had molds tier-agnostic (~50) while §9 gated the
tier on the mold's source, and both cannot hold: a tier-agnostic Chest Mold from rung 5 would combine
with a Rough temper and yield Rough gear.

✅ **DECIDED in favour of §9: molds are tiered.** §2's mold set and its "1 in 150" objection are
corrected there — tiering costs ~150 rows to generate and **nothing** in hunt difficulty, because the
three tiers drop from three different activities and so never share a loot table.

⚠️ **The temper stays BOUGHT and stays tiered — the redundancy is deliberate.** Mold and temper now
both name the tier, doing two different jobs:

| gate | asks | tier ladder |
|---|---|---|
| **Mold** | *can you get there?* | delve/world — delve/world — **raid** |
| **Temper** | *can you pay for it?* | 100p — 300p — 500p |

That keeps every earlier decision intact: the coin sink survives, and §3's *"a non-crafter can
contribute"* still reaches **all three tiers**, since a player with money can buy tempers for the
group's crafter even at Rough. Making the Rough temper a raid drop instead — the alternative
considered — would have closed that door for no gain, since the Rough *mold* already gates the tier
behind the raid.

### ⚠️ COST LADDER — DECIDED: 100p / 300p / 500p per combine

Anchored to delve income of 10p per rung:

| tier | per combine | 7-piece set | rung you would be farming | ≈ clears |
|---|---|---|---|---|
| Crude | **100p** | 700p | ~10 (100p) | **7** |
| Simple | **300p** | 2,100p | ~20 (200p) | **10** |
| Rough | **500p** | 3,500p | ~30 (300p) | **12** |

📌 **These numbers hold together well.** Because delve income scales with rung and the cost scales with
tier, each set costs roughly **7-12 clears at the rung you would be running while wearing it** — a
consistent price in *time* even though the plat figures triple. That is a better property than a flat
cost would have given.

📌 There is a natural brake on over-gearing, so none of this needs policing: the delve scales to
measured gear (§24 `M.power`), so a better set makes your delves harder. Kitting up is a real choice,
not a free win.

### Consequences for tuning

- **Materials must be cheap enough to buy every run.** A Rough temper at 100p is fine when a rung-30
  delve pays ~300p; a 1,000p component would mean levelling naked because the gear is not worth it.
- **Crafting time matters more than usual.** If a full re-kit is thirty combines, players will skip it
  and the system dies. Keeping components at (type, tier) rather than per piece is what keeps a re-kit
  to a handful of combines rather than dozens.
- ⚠️ **Do NOT exempt crafted gear from the wipe as a shortcut.** It would make the first set the last
  set anyone ever makes, and collapse the whole loop into a one-off.

## 10. ✅ THE ROUGH TIER LIVES ABOVE THE SCRIPT'S REACH

**DECIDED: Crude and Simple are owned by the scaling script. Rough is scaled by it too, and then gets
a premium on top — and it lives at an id the script cannot reach on a redeploy.**

### The id map

`item_scaling_main.py` is bounded by `id BETWEEN 1 AND 200000` in **four** places, and
**200000-299999 is completely empty** (0 items). That is the landing zone.

| | ids | who owns it |
|---|---|---|
| Crude, Simple | 50000-50xxx | ⬅ the scaling script, every redeploy |
| **Rough (boosted)** | **250000+** | ⬅ **us — above the script's ceiling** |
| Rough Hallowed | 550000+ | us |
| Rough Mythic | 850000+ | us |

⚠️ The tier maths still works at these ids: `AoTv4TierBaseId(550000) = 550000 % 300000 = 250000` and
`850000 % 300000 = 250000`. And 850000 is comfortably under RoF2's chat-link ceiling of **1,048,575**
(§10), which is the constraint that forced the 300,000 step in the first place.

### ⚠️⚠️ TWO EXCLUSIONS ARE REQUIRED, AND MISSING EITHER SILENTLY UNDOES THE PREMIUM

1. **The scaling script** already stops at 200000, so the band is safe *by construction* — but that
   bound is a literal repeated in four queries. If it is ever raised, the boost is silently erased on
   the next run and the only symptom is Rough quietly becoming ordinary.
2. **`aotv4_gear_tiers.sql` scopes `id < 300000`**, so it WOULD treat 250000 as a base and regenerate
   550000/850000 with its own ×1.5 / ×2 multipliers — overwriting the per-tier premium. **The band must
   be excluded from that script**, and all three rows generated together by ours instead.

📌 Both are the same failure: a generator that owns an id range quietly reclaiming rows somebody else
is maintaining. §35 records exactly this shape after the 0.1.2 item rework.

### The premium — 10 / 12 / 15 percent by crafted quality

| crafted result | premium over what the script would give |
|---|---|
| native | **+10%** |
| Hallowed | **+12%** |
| Mythic | **+15%** |

📌 This is what makes the raid tier best in slot *and* makes crafting skill matter twice: a better
tradeskill roll gets you a better tier **and** a larger premium on that tier.

✅ **DECIDED: the premium REPLACES the trivial margin. Rough drops to trivial 130 — parity, exactly
like Crude and Simple — and the +10/12/15% IS the raid margin.**

⚠️⚠️ **SO ALL THREE TIERS ARE SCALED TO PARITY AND THE ONLY THING ABOVE THE CURVE IS THE PREMIUM.**
That is the whole balance statement in one line, and it is why this was chosen over stacking: two
overlapping margins would both have to be remembered every time either was retuned, and the
combination (~21-26%) was more than was ever asked for.
📌 It also means the trivials now do ONE job — set the item to its tier's parity budget — and the
premium does the other. Nothing about "is this gear good" is hidden in a recipe row any more.

## 11. ⏸ PARKED UNTIL RAIDS HAVE BEEN PLAYED

**DECIDED: change nothing else until the raid encounters have been tried.** Cloth stays exactly as it
is, the scaling script keeps its current cloth handling, and the whole gear picture is judged once
there is something at the top of it to judge against. Tuning two systems at once and then guessing
which one moved is how the delve's boss multiplier got retuned four times.

⚠️⚠️ **BUT THE CLOTH BEHAVIOUR IS A KNOWN QUANTITY, NOT AN UNKNOWN — DO NOT RE-DIAGNOSE IT.** When the
script next runs over cloth, measured across 200 seeded items at Rough:

- **40 percent of cloth items come out with ZERO AC**, and the rest range up to 31.
- A cloth item that *does* win the AC roll can end up with **more AC than plate** (measured: robe AC39
  against breastplate AC37) **and lose its mana entirely**.

The cause is `forced_ac: False` on `cloth_stat_affinity` plus the allocator picking by
`score / next_cost` — AC costs 0.5 a point, the cheapest on the sheet, so once AC is *selected* it
dominates the budget regardless of its weight.

📌 **It will present as "casters are randomly broken", not as "cloth feels weak"** — two robes of the
same tier will differ wildly for no reason a player can see, because the roll is seeded off the item
id. If that is reported from play, this is the answer; it is not a new bug.

📌 The fix, when it is wanted: force AC on cloth and apply a per-armour-type AC multiplier AFTER
allocation. Affinity weights cannot do it -- the budget normalises them away, which is why 80/70/55 for
plate/chain/leather produces 40.2 / 39.9 / 38.2, a five percent spread.

## 12. What is still open

1. ~~**`T`**~~ **ANSWERED: target the BUDGET, not a stat multiple.** Rough Titanwrought = +10-15% over the
   best non-raid item **measured in stat points**. Rough sits at trivial 130 (parity) with a
   +10/12/15% premium applied above the script's reach (§10). The per-axis
   rescale table in §5 is superseded by this: the scaler reallocates every stat from the budget, so
   there is no need to pick separate factors for AC, HP, mana and stats at all.
3. ~~**Mold sources**~~ ✅ **ANSWERED AND CLOSED (§9).** Molds carry the tier; Crude and Simple drop
   from delve rungs **and** open-world named mobs (`global_loot`, `rare = 1`); **Rough drops only from
   raid encounters**, placed by `aotv4_raid.lua` rather than by any loot rule. Two things still hang
   off it:
   - ⚠️ **shared vs per-player** — global loot rolls at spawn and stays shared (§31), so a group that
     kills a named mob gets **one** mold between them. Accept, or grant per player in Lua off
     `npc:IsRareSpawn()` (Lua-bound) with `SummonItemExact`.
   - ⚠️ delve rungs **21-70** still drop no molds. Named mobs now cover Simple to level 30, so a capped
     character has a world route — but the rung they actually farm rewards no mold at all.
6. ~~**Does the mold say too much?**~~ ✅ **ANSWERED: keep the type on the mold.** The four armour
   types are what make a player work all four material lines instead of maxing one; that is the whole
   point, and the redundancy with the type-specific base and binding is the mechanism rather than a
   flaw. §3's stale *"a Chest Mold plus plate materials gives a breastplate; the same mold plus silk
   gives a robe"* is corrected — it described the earlier 7-slot-only mold set. Jewelry takes the same
   discipline via **4 metals**, and weapons via **4 material lines inside Blacksmithing** (§2), since
   the data offers no four-*skill* weapon split — 641 craftable weapons are Blacksmithing against
   Fletching's 3.
5. **How far up the tiers?** Crude/Simple/Rough cover levels 10/20/30. Ornate (40) and above only
   matter if the cap moves.

## 13. ✅ BUILT — 2026-08-30, migrations v152-v157

Everything below is applied to the dev database and merged into the manifest. **Nothing has been
crafted in game yet** — this is built and verified against the data, not played (§30's distinction).

| migration | what | ids |
|---|---|---|
| **v152** | 86 gear items — shoulders/back/face/waist x 4 types, ears/neck/fingers x 4 metals, Crude + Simple charms | 148000-148085 |
| **v153** | 210 molds, one per craftable output | Crude 148200-148269 · Simple 148270-148339 · Rough 148340-148409 |
| **v154** | 48 materials — 36 bases, 9 bindings, 3 tempers | 148500-148547 |
| **v155** | 255 recipes — 45 sub-recipes + 210 final combines | 480000-480254 |
| **v156** | hub vendor: 3 raw components + 3 tempers on Audri_Deepfacet (202069) | slots 144-149 |
| **v157** | mold `global_loot` off named mobs; **disables gear drops 1-4** | global_loot 101-102 |

Generated by **`custom/tools/gen_titanwrought_crafting.py`** (re-runnable; regenerates all four SQL
files). Lua: `aotv4_dungeon.lua` `M.roll_mold` puts a mold in
the delve chest on rungs 1-20 at **`M.MOLD_CHANCE` = 10 percent** (it was a guaranteed drop until
2026-08-30 — see that constant's note: a certain chest strictly dominated the 25 percent named-mob
route), `aotv4_raid.lua` `M.grant_molds` gives each raider **2** Rough molds.

**Invariants, all measured green:** 0 molds unused · 0 gear items unmade · 0 recipes without a
container · 0 recipes content-filtered or must_learn · 0 recipes that can fail · 0 combines that
return the mold · 0 duplicate names across the whole band · 344 items, 344 distinct names.
📌 The reachability check reports **70 unobtainable components** and that is CORRECT: they are exactly
the Rough molds, which are granted by `M.grant_molds` in Lua and so are invisible to any table-based
query. A source that lives in code cannot be seen by a query over `merchantlist`/`lootdrop_entries`.

### ⚠️⚠️ FOUR TRAPS HIT WHILE BUILDING THIS — all silent, all worth knowing

1. ⚠️⚠️ **THE GENERATOR READ ITS OWN OUTPUT AND SILENTLY SHRANK.** It clones from "existing"
   Titanwrought items found by name, and the items it *creates* are named the same way — so the
   second run found the two new charms already present and produced **84 items instead of 86**.
   Nothing errored; the count just quietly dropped. Fixed by excluding the generated band from the
   read query. 📌 **Any generator that discovers its inputs by pattern must exclude its own output
   band, or it is not idempotent** — and the failure is a smaller number, not an error.
2. ⚠️⚠️ **`items` HAS NO PRIMARY KEY — it has a UNIQUE INDEX NAMED `ID`.** `SHOW COLUMNS` reports it
   as `PRI` because MySQL labels the first unique NOT NULL index that way, so every sign says
   PRIMARY. `ALTER TABLE ... DROP PRIMARY KEY` fails with *Can't DROP INDEX `PRIMARY`*.
3. ⚠️⚠️ **THE MIGRATION VALIDATOR REFUSES `ALTER TABLE`, EVEN ON A TEMP TABLE — and it is right to.**
   DDL commits immediately, so the tool cannot dry-run the file or prove idempotency, and it declines
   to merge. The first set-based clone needed a helper column and an index drop; it was rejected and
   **three of six migrations silently did not merge**, leaving recipes in the manifest referencing
   items nothing created. ✅ The fix is a **single-row temp table**: reseed it per item, override,
   `INSERT INTO items SELECT *`. No helper column, no index to drop, and the `SELECT *` guarantee
   (§5 — never hand-list 285 columns) is preserved.
4. ⚠️⚠️⚠️ **MIGRATION ORDER IS LOAD-BEARING BECAUSE ONE OF THEM READS `items`.** The mold loot rows
   are built with `INSERT INTO lootdrop_entries SELECT 200040, id ... FROM items WHERE id BETWEEN
   148200 AND 148269`. Merged in the wrong order it runs **before the molds exist**, matches nothing,
   and inserts **zero rows** — a mold table that is present, enabled, and empty, with no error
   anywhere. The six were unwound and re-merged in dependency order (items → recipes → drops).
   📌 **A migration that populates one table by SELECTing another is ordered, and nothing enforces
   it.** The tool assigns versions in the order you merge, so merge order *is* run order.

### ⏸ Still open
- **Nothing has been crafted in game.** The sharp first test is a **Crude Cloth Cap**: buy a Crude
  Temper (100p) and the raws from Audri, craft a Crude Silk Bolt and a Crude Armour Fitting, find a
  Crude Cloth Cap Mold, and combine at a loom. Confirm the combine cannot fail, the mold is consumed,
  and the recipe is **findable in the recipe window** (that last one is what §32's trap breaks).
- **`probability 25` on the named-mob mold rows is the one number to tune**, and it is a first guess.
- **Rough mold pacing**: 3 per raider per kill against a 24-hour lockout and 70 possible molds.
- Delve rungs **21-70** still drop no mold (§12).
