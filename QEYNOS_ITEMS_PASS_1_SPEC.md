# Qeynos Items Pass 1 — build spec

Derived from `Qeynos_Items_Pass_1.xlsx` (140 items) plus measurements taken from this database.
**Nothing is built.** Ids, stats and level bands below are proposals for review.

---

## 1. Counts and id allocation

| block | in sheet | after Jaggedpine removal |
|---|---|---|
| armour set pieces | 64 (13 sets) | **64** (13 sets) |
| weapons | 22 | **21** |
| shields | 10 | **9** |
| jewellery | 20 | **17** |
| standalone armour | 24 | **23** |
| **total** | 140 | **134** |

**Proposed band: `148300-148433`** (134 items). Verified empty (147969-149999 holds nothing), clear of the delve
augs (147600-147915), the tomes (147966-147968) and the weapon augs reserved at 148200-148299.

⚠️ Base ids must stay well under **1,048,576** — RoF2 encodes item links in a 5-hex-digit field, and
the gear tiers add +300,000 / +600,000, so 148300 → Mythic 748300, comfortably inside the ceiling.

✅ **Centaur Warrior stays at FOUR pieces** (Chest/Arms/Legs/Boots) — decided, not an oversight. It is
the one four-piece set in the pass.
📌 Its *Arms/Boots* wording maps to the same slots the other sets call *Shoulders/Feet*; only the
vocabulary differs, and the generator uses the slot ids, so nothing needs renaming.

---

## 2. Level bands, from the source zones

Each item's `reclevel` follows the zone its source creature lives in, read from `aotv4_zone_xp`:

| band | reclevel | source zones |
|---|---|---|
| **low** | **10** | Blackburrow (2-15), Qeynos Hills (1-16), W Karana (2-13), N Karana (3-17) |
| **mid** | **20** | E Karana (9-21), Rathe Mountains (5-25) |
| **high** | **30** | S Karana (8-30), Splitpaw (24-35), Gorge of King Xorbb (25-35) |

- `reqlevel` = **0** everywhere (no level requirement, as specified)
- `classes` = **65535**, `races` = **65535** (all/all, as specified)

---

## 3. Stat budgets — measured, not invented

Averages of every existing item at that `reclevel`, taken after the rescale so they reflect the
current stat model.

### Armour (per piece)

| band | AC | HP | mana | 7 stats total | 5 resists total |
|---|---|---|---|---|---|
| 10 | 7 | 5 | 4 | 7 | 8 |
| 20 | 12 | 9 | 9 | 14 | 18 |
| 30 | 18 | 17 | 16 | 23 | 37 |

⚠️⚠️ **These are averages across ALL items in the band, which includes vendor trash and quest junk.**
A piece that drops from a *named* creature should sit **above** the line — suggest **×1.4** for named
drops and **×1.0** for the trash-drop pieces. Using the raw average would make a named set feel worse
than the random gear players already have.

### Armour type shapes the spread (same budget, different split)

| type | weighting |
|---|---|
| Plate | AC heavy, HP over mana, STR/STA |
| Chain | balanced AC/HP, DEX/STA |
| Leather | lower AC, higher stats, DEX/AGI |
| Cloth | lowest AC, mana and INT/WIS |

### Weapons

| band | 1H damage / delay | ratio | 2H damage / delay | ratio |
|---|---|---|---|---|
| 10 | 11 / 28 | 0.40 | 31 / 43 | 0.73 |
| 20 | 17 / 26 | 0.65 | 54 / 40 | 1.35 |
| 30 | 22 / 26 | 0.84 | 70 / 41 | 1.73 |

📌 Delay barely moves with level (1H sits at 26-28 throughout) — **damage is the dial**, which matches
how the rest of the server is tuned.

### Shields

| band | AC | HP |
|---|---|---|
| 10 | 16 | 7 |
| 20 | 28 | 13 |
| 30 | 39 | 21 |

---

## 4. ✅ Jaggedpine: the SET stays and moves, the loose items are cut — 6 items removed

The Jaggedpine Forest is region **99 "Unused"**, unreachable by design, so anything sourced there could
never drop and nothing would report it.

**The Jaggedpine Gnollhide set (5 leather pieces) is KEPT and re-sourced to Splitpaw.** The six loose
Jaggedpine items are cut.

| row | item | pieces | outcome |
|---|---|---|---|
| 7 | Jaggedpine Gnollhide (set) | 5 | **KEPT — moved to Splitpaw** |
| 30 | Jaggedpine Spear | 1 | cut |
| 42 | Jaggedpine Warden's Shield | 1 | cut |
| 51 | Jaggedpine Band | 1 | cut |
| 61 | Warden's Pendant | 1 | cut |
| 65 | Jaggedpine Bracer | 1 | cut |
| 80 | Jaggedpine Cloak | 1 | cut |
| | **total cut** | **6** | |

**Why Splitpaw** (`paw`) rather than Blackburrow: it is gnoll-themed so the set reads correctly, it has
**25 named hosts at levels 24-35**, and it currently carries only the *Paw Stalker* chain set — so a
leather set fills a real gap there. Blackburrow already hosts two leather sets (*Blackburrow Hunter*,
*Gnollskin*) and sits in the low band, where the pass is already dense.
📌 Moving it to Splitpaw also shifts it from the low band to the **high** band (reclevel ~30), which is
the thinner half of the pass.

⚠️ **The name still says Jaggedpine.** Mechanically irrelevant, but a set called *Jaggedpine Gnollhide*
dropping in Splitpaw reads oddly. Say the word and it becomes e.g. *Splitpaw Gnollhide*; left as-is by
default, since renaming was not asked for.

⚠️ **`Gnollhide Bracelet` (row 66) is also KEPT.** Its source reads *"Blackburrow and Jaggedpine gnolls"* —
Blackburrow is reachable, so it has a valid home. It is the only dual-sourced row, and a name-match
sweep would have deleted it.

📌 **Gorge of King Xorbb is a separate matter and is NOT cut.** It is region **2 (Freeport)** rather
than Qeynos, but it is fully playable and correctly level-banded at 25-35, so its ~20 items work
exactly as written. They are simply another region's content living in a Qeynos pass — worth knowing,
not a defect.

---

## 5. Loot wiring

Named creatures carry most of it, with some on trash, as specified.

- **Set pieces and shields** → named creatures of the matching family
- **Weapons and jewellery** → split, the better ones named
- **Standalone armour (block 5)** → the named-specific pieces (*Bayle's Breastplate*, *Guard
  Captain's Bracers*, *Gnoll Chieftain's Headdress*) are already written as named drops in the sheet

### Host eligibility — the rule, and why each clause is there

A creature may host a drop only if **all** of these hold:

```sql
  (name LIKE '#%' OR (name REGEXP '^[A-Z]' AND name NOT LIKE 'a\_%' AND name NOT LIKE 'an\_%'))
  AND class NOT IN (40, 41)            -- banker, merchant
  AND class NOT BETWEEN 20 AND 35      -- the trainer classes
  AND name NOT LIKE '%uard%'           -- guards
  AND name NOT LIKE '%erchant%'
  AND name NOT LIKE '%anker%'
  AND level BETWEEN <zone lo> AND <zone hi>     -- the zone's own band
```

#### ⚠️⚠️ Merchants and guards must be excluded — 141 of them would otherwise be picked

They pass the named test on their own names (`Guard_Mezzt` cleans to *Guard Mezzt*), so a
named-only query hands them loot:

| zone | civic NPCs the bare heuristic picks |
|---|---|
| Rathe Mountains | 73 |
| East Karana | 33 |
| Qeynos Hills | 17 |
| North Karana | 16 |
| South Karana | 2 |

📌 Qeynos guards are level **3-4** on town factions 109/985 — civic furniture, not content, and putting
gear on them would also mean players killing the town to farm it, which §28's faction floor exists to
make unnecessary.

#### ⚠️⚠️ The host's level must sit inside the zone's band — no level 99s

Under option B the host's level **is** the item's power (`quality = level // 5`). Several source zones
hold named creatures far above their own band — North Karana, South Karana and Rathe Mountains all run
to **level 99**. Wiring a set piece to one yields quality ~19.8 → **reclevel 85** and a stat budget
near **7,700**: an item far beyond anything else on a level-30 server, produced silently, with nobody
authoring a number.

Clamping the host to the zone's own `aotv4_zone_xp` band leaves plenty to work with:

| zone | band | valid hosts | their levels |
|---|---|---|---|
| Blackburrow | 2-15 | 11 | 2-15 |
| West Karana (`qey2hh1`) | 2-13 | 51 | 3-13 |
| Qeynos Hills | 1-16 | 40 | 1-14 |
| North Karana | 3-17 | 28 | 5-17 |
| East Karana | 9-21 | 49 | 10-21 |
| Rathe Mountains | 5-25 | 116 | 5-25 |
| South Karana | 8-30 | 63 | 8-30 |
| Splitpaw | 24-35 | 25 | 24-35 |
| Gorge of King Xorbb | 25-35 | 10 | 26-35 |

⚠️ **West Karana's short name is `qey2hh1`, not `westkarana`.** Querying the obvious name returns zero
rows and looks exactly like "this zone has no valid hosts" — it briefly did here. The Karanas are
`qey2hh1` / `northkarana` / `southkarana` / `eastkarana`, and only one of the four is irregular.

📌 Blackburrow returns **zero** hosts under a bare `#`-prefix test but **11** under the full proper-noun
heuristic. Use the full test, or the zone reads as having no named creatures at all.

⚠️ **Individual loot (§31) rolls the table once per eligible player**, so a drop chance is *per
player*, not per corpse. A six-person group multiplies the visible rate by six — size chances against
that, not against a solo kill.

---

## 6. ✅ DECIDED: the scaler owns the stats (option B)

These items are created with **identity fields only** — name, `itemtype`, `slots`, material/icon,
`classes`/`races` 65535, `reqlevel` 0 — and `item_scaling_main.py` assigns every stat. The band is NOT
excluded from it. The measured budgets in §3 become a **prediction to check the output against**,
not values to author.

### ⚠️⚠️ THIS INVERTS THE BUILD ORDER — LOOT WIRING MUST COME FIRST

```python
# No valid acquisition source
if not qualities:
    continue
```

**An item with no drop, no recipe and no vendor price is SKIPPED and gets nothing.** Create the rows,
run the scaler, and wire loot afterwards, and you have 140 statless items that look correctly
configured. Loot first, scaler second — always.

### The source column now literally sets the power

The scaler derives everything from where an item comes from:

```
quality   = level_quality(npc_level) × chance_modifier(drop_chance) × scaling_type_modifier(named)
          = (level // 5)             × up to 1.35                   × 1.0 / 1.10 / 1.25
reclevel  = max(0, floor(quality - 2) × 5)
budget    = floor(quality ** 3)                        # CUBIC in quality
```

Worked examples:

| source | quality | reclevel | stat budget |
|---|---|---|---|
| level 30 named, 5% drop | 8.9 | **30** | 707 |
| level 15 named, 10% drop | 4.4 | **10** | 83 |
| level 10 trash, common | 2.0 | **0** | 8 |

📌 This lands on the §2 bands on its own — a Splitpaw named yields a level-30 item, a Blackburrow one a
level-10 item — so the sheet's *Source* column is doing the design work. That is the real argument for
option B: the themeing survives through the loot wiring instead of through authored numbers.

⚠️ **Drop chance is a power lever, not just a frequency.** `chance_modifier` pays up to **×1.35** for
rarity, so making something rarer makes it *better*. And **budget is cubic in quality**, so small
quality differences are large stat differences — a 20% quality bump is a 73% budget bump.

⚠️⚠️ **NEVER put these on a vendor.** `quality = min(qualities)` takes the **lowest** of the drop,
tradeskill and vendor figures, so one cheap merchant listing collapses an item the named drop would
have made excellent.

⚠️ `scaling_type_modifier` pays 1.10 for named and 1.25 for raid — confirm which field feeds it before
wiring, so the named premium actually applies rather than silently defaulting to 1.0.

## 7. Build order

1. Agree the id band and resolve Jaggedpine
2. Generate the 140 rows — **identity fields only, no stats, no reclevel**
3. ⚠️⚠️ **Wire `lootdrop_entries` / `loottable_entries` NOW**, before the scaler. An unwired item is
   skipped and stays blank
4. Run `item_scaling_main.py` — it assigns stats and reclevel
5. Check the output against the §3 budgets. They were measured from existing items at those levels, so
   a wild divergence means the loot wiring is sending the wrong level or chance
6. ⚠️ Re-run **`aotv4_gear_tiers.sql`** — Hallowed and Mythic are generated FROM these base stats and
   will not exist until it does
7. ⚠️ `items` is **shared memory**: world down → `./shared_memory` → restart

📌 No fixed-seed generator discipline is needed for the stats under option B — the scaler is already
deterministic per item id. It IS still needed for anything the generator picks itself (icons, material)
if that is ever randomised.
