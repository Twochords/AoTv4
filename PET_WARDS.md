# Pet wards + Kindred Bond — build notes and test plan

Every pet now carries a standing self-buff suited to its family, and the Ranged AA **Kindred Bond**
hands a copy to its owner. Built 2026-07-26. **Untested in game.**

---

## 1. Why this shape

Stock EQ gives only the Magician fire pet a standing self-buff — its damage shield. That is a good
idea applied to exactly one pet out of hundreds. This extends it to every family, so a pet is
identifiably *what it is* beyond its model, and so there is something worth sharing.

⚠️ **The family comes from the `pets`.`type` string, not the owner's class.** A Magician has four
thematically different pets and they must not all get the same ward. Matching is by **prefix**, so
every rank of a line resolves together — `SumFireR10` through `SumFireR37` are all `SumFire`.

## 2. The wards (`custom/sql/aotv4_pet_wards.sql`, ids 43420-43429)

⚠️⚠️ **Everything here scales with the pet's level, and that is not optional.** Pets arrive at level
**one** — `285 Pendril's Animation` and `338 Cavorting Bones` are both level-1 spells, and the
Magician elementalkin line starts at 2. A flat value tuned for the mid levels is not merely strong
down there, it is decisive: 5 points of damage reduction against a mob that hits for 6, or a
150-point absorb against a mob with 40 health, simply ends the fight. **The first draft made exactly
that mistake and had to be rescaled.**

Scaling is done by the spell's `formula` field so the engine does the work and no extra rows are
needed (`Mob::CalcSpellEffectValue_formula`): `100` flat, `101` base + level/2, `103` base + level×2,
`109` base + level/4. `max_value` is the ceiling and is enforced in both directions.

| Pet family | Ward | Effect | L1 | L25 | L50 |
|---|---|---|---|---|---|
| `SumFire*` — Magician fire | Cinder Aura | damage shield (SPA 59) | −1 | −7 | −13 |
| `SumWater*` — Magician water | Tidal Renewal | health regen (SPA 0) | 1 | 7 | 13 |
| `SumEarth*` — Magician earth | Stoneflesh | max health (SPA 69) | 12 | 60 | 110 |
| `SumAir*` — Magician air | Gale Fervor | attacks at 110% (SPA 11) | — flat — |
| `skel_pet*`, `animateDead` — Necromancer | Graveborn Hunger | 8% melee lifetap (SPA 178) | — flat — |
| `BLpet*` — Beastlord warder | Feral Ferocity | +8% melee damage (SPA 185) | — flat — |
| `Animation*` — Enchanter | Arcane Weave | absorb rune (SPA 55) | 7 | 55 | 105 |
| `SpiritWolf*` | Spirits Grace | avoidance (SPA 172) | 1 | 7 | 13 |
| `Familiar*`, `CasterWolf*` | Kindred Insight | mana regen (SPA 15) | 1 | 7 | 13 |
| **anything else** | Bound Servant | max health (SPA 69) | 5 | 17 | 30 |

The three **percentage** effects are deliberately left flat: a percentage is already
level-appropriate, since it is a share of output that grows on its own. They were still tuned down
from the first draft — 25% haste on a level-2 pet is proportionate and still absurd.

⚠️ **Earth and the fallback use SPA 69 (max health), not SPA 162 (flat damage reduction)**, even
though 162 fits "stoneskin" better. SPA 162 keeps its magnitude in the **limit** field, and the
formula system only scales **base** — so a 162 ward cannot grow with level without one row per level
band. Max health scales cleanly and is just as much "this pet is hard to kill".

The fallback is what makes "every pet has one" literally true — quest pets, swarm pets, monster
summoning, and whatever gets added later all resolve to it.

Applied in `Mob::MakePoweredPet` right after the pet joins the entity list, **cast from the pet**, so
inspecting the buff credits the pet rather than the summoner.

⚠️ Two field traps in these rows, both of which read backwards or silently do nothing:
- **SPA 11 is the resulting speed, not the bonus.** 110 = "attacks at 110 percent". Below 100 is a
  *slow* — it is easy to build a debuff by accident.
- **SPA 59 damage shields need a NEGATIVE base**, which is the native convention. The formula's
  `updownsign` handles it, and a negative base forces a negative result, so it scales downward
  correctly rather than turning into a heal.

## 3. Kindred Bond (Ranged tab, `type = 3`)

Host **32 Finishing Blow**, ranks **119, 120, 121, 440, 441**. Currently the only AA on the Ranged
tab — the rest of that tree is still to be designed.

| Rank | Effect | Where |
|---|---|---|
| 1 | You gain your pet's ward while it lives | code |
| 2 | …and so does your group | code |
| 3 | Your pet is hardier — SPA 213 PetMaxHP | **native** |
| 4 | …and strikes true — SPA 218 PetCriticalHit | **native** |
| 5 | The ward outlives the pet by a minute | code |

Passive, so unlike Sanctified Blow and Sanguine Frenzy it **may** carry `aa_rank_effects` — the "no
effect rows" rule applies only to activated abilities. Ranks 3 and 4 exploit that and need no code.

The point of the AA is that *what your companion is changes what you are*: a fire pet lends you its
burning aura, an air pet its speed, a familiar its clarity.

## 4. Known limitations — read before testing

- ✅ **BOTH OF THE "SUMMON TIME ONLY" LIMITS ARE GONE (2026-08-30).**
  `Mob::AoTv4RefreshPetWard` runs every 6 seconds and now tops up the **owner's** copy and the
  **group's** as well as the pet's, so all three are upkeep rather than a one-shot grant.
  - Buying or ranking up Kindred Bond with a pet already out now works on the next tick — the ward
    id is remembered at summon regardless of rank, and the refresh reads the rank live.
  - Someone who joins the group after the summon picks it up on the next tick.
  - ⚠️ Someone who **leaves** still keeps what they were given until it expires or the pet dies.
    Unchanged, and not made worse: an ex-member is simply no longer walked.
  - ⚠️⚠️ This was a reported bug, not a nicety: *"the buff will fade and doesn't get re-applied to
    you, just the pet. You have to kill your pet and then recast."* The pet's copy returned within
    six seconds and the owner's never did, so re-summoning was the only cure — at the cost of every
    buff and every weapon the player had put on the pet.
- **The owner's copy is stripped when the pet dies** (`NPC::Death` → `AoTv4PetWardEnded`). The rows
  carry an effectively permanent duration on purpose: the pet's life is the intended lifetime, not a
  timer. Without that hook the owner would simply keep it forever.
- **Haste and mitigation buffs do not stack in EQ** — the strongest wins. Gale Fervor shared to an
  owner who already has a haste buff will do nothing visible, and Stoneflesh may be overwritten by
  Stonestride from the tank tree. Both are expected, not bugs.
- **Arcane Weave and the tank tree's Sanctified Ward are both SPA 55**, and the bonus calculation
  keeps a single rune slot — they will not stack either.

## 5. Test plan

1. **Every family gets something.** Summon one of each you can reach — mage fire/water/earth/air,
   a necro skeleton, a beastlord warder, an enchanter animation, a familiar. Each should show a
   distinct buff on the pet.
2. **The fallback works.** Summon something obscure (monster summoning, a swarm pet). It should show
   **Bound Servant**, not nothing. A pet with no buff at all means the prefix table was not reached.
3. **Kindred Bond rank 1:** buy it **with a pet already out** and confirm the buff appears on you
   within ~6 seconds, no re-summon. Then **remove it deliberately** — the sharp test is to land a
   competing buff that overwrites it (a stronger haste over Gale Fervor, Stonestride over
   Stoneflesh) and confirm it comes back on the next tick rather than staying gone until re-summon.
   That overwrite is the bug this replaced, and it is why the pet looked fine while the owner did not.
4. **Rank 2:** group up **after** the summon and confirm members get it without a re-summon.
5. **Pet death:** kill your own pet. Your copy and the group's should vanish immediately.
6. **Rank 5:** kill your pet — the ward should linger about a minute rather than vanishing.
7. ⚠️ **Check `Gale Fervor` is haste and not a slow.** If the pet suddenly attacks slowly, the SPA 11
   base is being read the way it looks rather than the way it works.

## 6. Files

| File | What |
|---|---|
| `custom/sql/aotv4_pet_wards.sql` | the ten ward spells, 43420-43429 |
| `custom/sql/aotv4_aa_ranged_kindred.sql` | Kindred Bond |
| `zone/aotv4_pet_aa.cpp` | family→ward mapping, sharing, teardown |
| `zone/pets.cpp` | `MakePoweredPet` — applies the ward at summon |
| `zone/attack.cpp` | `NPC::Death` — strips the owner's copy |

Deploy: both SQL files → `ninja zone` → world down → `./shared_memory` → world up → `eqlaunch` →
`./export_client_files`, copy `dbstr_us.txt` + `spells_us.txt` to the EQ root.
