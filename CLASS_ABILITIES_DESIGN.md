# Class abilities — three per class, sixteen classes

Every class gets the same three-tier shape. Paladin is built (v94-v98) and is the reference.

| Tier | Level | Shape | Cooldown |
|---|---|---|---|
| 1 | 1 | a melee blow that scales off the equipped weapon | 10 s |
| 2 | 5 | the class's signature effect | 120 s |
| 3 | 10 | an ability that **shortens tier 2's cooldown** | 15 s |

---

## 1. Architecture — these are DISCIPLINES, not AAs

The Paladin three were built as activated AAs. **Do not repeat that for the other fifteen.**

| Mechanism | What bounds it | Free today |
|---|---|---|
| Activated AA | `aa_ranks.spell_type`, a client timer slot | **1** |
| Combat ability | skill ids 0-77, all defined and used | **0** |
| Discipline / spell | spell ids under 45000 | **~400** in one band |

⚠️⚠️ **THE AA ROUTE IS ALREADY EXHAUSTED AND THE CEILING IS NOT THE AA COUNT.** 354 native rows are
free to host, which makes it look roomy — but an *activated* AA needs a `spell_type`, the client
tracks its hotkey cooldown by that number, and **nothing in stock data exceeds 99**. Values above it
are accepted by the server and silently render no hotkey timer at all (found the hard way; the AA
window showed the recast correctly while the hotkey stayed blank). Three Paladin abilities took
82/83/84 and **one slot remains**. 6 x 16 activated AAs is not off by a little, it is impossible.

**A discipline is just a spell** — a row in `spells_new` with `mana = 0` and `EndurCost > 0`, which is
all `IsDiscipline` tests. It trains into the **Combat Abilities window** rather than the spellbook, so
it costs no spell gem and sits beside Kick and Frenzy, which is where these belong.

### ⚠️⚠️ Leave `EndurTimerIndex` at 0 — do NOT allocate shared timers
The instinct to give each tier a shared timer slot is right about the *risk* and wrong about the
*need*. A character is only ever one class, so no one can hold two tier-1 abilities; sharing would
therefore be harmless. But **a spell's recast is already keyed on its own spell id**
(`pTimerSpellStart + spell_id`, spells.cpp:2931) and has no ceiling, so per-spell recast produces the
identical player-visible result for **zero** slot cost.
- `EndurTimerIndex` exists for cooldowns you *want* linked -- stock's Defensive/Holyforge/Evasive
  family shares one so using any locks the rest. That is not what we want here.
- ⚠️ **`MAX_DISCIPLINE_TIMERS` is 20 and only slot 20 is free.** Fourteen more hold nothing reachable
  at level 35, which makes them look reusable -- **do not**. The level cap has already moved once
  (70 -> 35) and the era system exists to unlock expansions later; the day either happens, a class
  ability silently shares a cooldown with the Lesion line and nothing reports it.

### Id allocation
- **Spells 44700-44747** — 16 classes x 3, allocated `44700 + (class-1)*3 + tier`. Contiguous and
  derivable, so the class of any id is arithmetic rather than a lookup.
- **44750+** for any helper/trigger spell, never offered.
- ⚠️ Band is above the offerable pool (44530-44599), so these can never be handed out as a level-up
  reward. Set `classes<N> = <level>` for the owning class and **255 for the other fifteen** -- that is
  the class gate, and it is stricter and simpler than the AA bitmask.

### One C++ prerequisite
Tier 3 shortens tier 2's cooldown. For AAs I added `Client::AoTv4ReduceAATimer`. **The spell
equivalent does not exist** — `ResetCastbarCooldownBySpellID` only ever *clears*
(`spells.cpp:7544`). One new method + Lua binding, modelled on the AA one, including its two traps:
`GetRemainingTime` returns `0xFFFFFFFF` for a disabled timer, and the client must be re-sent the
timer or the button stays greyed while the server considers it ready.

### Damage model for tier 1
Route every tier-1 through **`DoSpecialAttackDamage`** (already Lua-bound, four overloads). It is the
one choke point Kick, Bash, Frenzy and the monk strikes all use, so tier 1 inherits their mitigation,
avoidance, hate and — per §22 — an endurance charge **proportional to damage dealt**.
⚠️ That means tier 1 must NOT also carry a flat `EndurCost`, or it pays twice.
📌 Scale it like `GetBaseSkillDamage` does: `base + skill/8 + <slot>AC/2` for unarmed classes,
`primary weapon damage / 2` for armed ones (that is literally Frenzy's formula).

---

## 2. The sixteen classes

Tier 1 is a weapon blow for everyone; the flavour is in what rides along.

### ⚠️⚠️ THE TIER 3 RULE: it must be worth pressing for ITSELF, and the cut must be EARNED
A tier 3 whose only job is shortening tier 2 becomes a button you mash on cooldown with no decision
in it -- and that is actively bad when the effect has a downside. The first draft had four of these:
an AoE taunt (Warrior) that pulls everything you were not fighting, an AoE blind (Druid) that sends
mobs wandering, an AoE silence (Bard) that is useless on trash and pulls, and a mez (Enchanter) that
breaks the moment anyone hits it. Spamming those is the *optimal* play and the *wrong* play at once,
which is the worst place a design can put a player.

Two rules fix it, and every entry below obeys both:

1. **The effect must be something you would press anyway** -- damage, a self buff, a resource return.
   Nothing whose downside makes you avoid it.
2. **The reduction is earned by the ability WORKING, not by it being pressed.** A resisted stun, a
   swing that misses, a mark on something that never dies -- these give nothing. Pressing it out of
   position is a wasted global, so there is a real decision about when.

### ⚠️⚠️ Two things an ability here may NOT depend on
Both were in the first draft and both are wrong on THIS server specifically.

**A pet.** Magician and Beastlord were built around one, and pets come from *spells* — which on a
random-progression server arrive from the level-up picker. A Magician may simply never have been
offered a pet spell. An ability that is dead until a random reward turns up is not an ability, so
neither class's kit mentions a pet any more.

**A group.** Shaman's whole kit was group-facing. Soloing is normal here, and the roguelite loop
means a character spends a lot of its life alone. Group *upside* is fine — Cleric, Bard and Paladin
still have it — but nothing may be worthless without one. Shaman's signature now slows the target
and heals the shaman, so both halves work with nobody else present.

📌 Rule 2 is also what keeps the numbers honest: a guaranteed 5 s off a 120 s cooldown every 15 s is
a 40 percent uptime gain with no play attached. Making it conditional means the ceiling is only
reached by someone doing the class's job well.

| # | Class | L1 — weapon blow | L5 — signature | L10 — cuts L5 by |
|---|---|---|---|---|
| 1 | **Warrior** | **Cleaving Blow** — swing, +hate | **Bulwark** — absorb the next 5 melee hits outright (SPA 55 rune sized off AC) | **Broad Cleave** — frontal cone damage; **−3 s per target struck**, max −9 s |
| 2 | **Cleric** | **Crusader's Mace** — swing, small self-heal | **Sanctuary** — group heal + cure, healing again after 3 ticks | **Condemn** — smite; **5 s**, **10 s** vs undead |
| 3 | **Paladin** *(built)* | **Ardent Strike** — swing + STR bonus | **Hand of Conviction** — heal group 25% of your max HP | **Divine Reproach** — 2 s stun; **5 s**, only if the stun lands |
| 4 | **Ranger** | **Twin Slash** — two swings | **Volley** — AoE ranged burst to everything in front | **Point Blank Shot** — fire your bow while in melee; **5 s**, **10 s** with a bow equipped |
| 5 | **Shadowknight** | **Reaving Strike** — swing, leeches HP | **Harrowing** — AoE **lifetap**: damages everything nearby and returns it as health | **Reaving Vow** — swing that amplifies your **next** lifetap by 50%; **6 s** |
| 6 | **Druid** | **Thorned Strike** — swing; **you** gain a damage shield that scales with your level | **Wildgrowth** — strong heal over time | **Sunflare** — direct damage; **5 s**, **10 s** if the target is snared or rooted |
| 7 | **Monk** | **Iron Palm** — unarmed strike (hands AC) | **Void Stance** — avoid the next 4 melee attacks entirely | **Pressure Point** — heavy single hit; **6 s**, **12 s** on a crit |
| 8 | **Bard** | **Discordant Strike** — swing, brief slow | **Crescendo** — restore mana AND endurance to the group | **Cadence Strike** — damage scaled by your **instrument mod**, and strengthens your running song; **5 s** |
| 9 | **Rogue** | **Vital Strike** — swing, damages and **snares** | **Rupture** — a heavy bleed over time | **Exploit Weakness** — damage, and your **next bleed lands 50% harder**; **5 s** |
| 10 | **Shaman** | **Spirit Strike** — swing, slows the target | **Crippling Spirit** — heavy slow + attack-power cut on the target, **and a heal over time on yourself** | **Malaise** — resist debuff; **5 s**, **10 s** if the target is already slowed |
| 11 | **Necromancer** | **Withering Touch** — swing, DoT | **Soul Harvest** — consumes every damage-over-time on the target, dealing its remaining damage at once and healing you | **Death's Toll** — **15 s** when it lands the killing blow, else nothing |
| 12 | **Wizard** | **Arcane Fist** — swing, mana return | **Overload** — very large single-target nuke, self-stuns 2 s | **Ley Tap** — damage; each cast makes your next Overload hit harder, **stacking 3 times**; **5 s** |
| 13 | **Magician** | **Elemental Fist** — swing with added fire damage | **Elemental Bulwark** — absorbs damage **and** burns whoever strikes you, 30 s | **Cinder Blast** — fire damage; **7 s**, **14 s** while Elemental Bulwark is up |
| 14 | **Enchanter** | **Mind Blade** — swing, small mana drain | **Stasis** — stun everything within 30 units, 4 s | **Mind Fray** — damage that returns mana; **5 s**, **10 s** if the target is stunned |
| 15 | **Beastlord** | **Feral Swipe** — swing that briefly hastes **you** | **Feral Frenzy** — large self haste + heal over time | **Bloodscent** — damage; **5 s**, **10 s** against a target below half health |
| 16 | **Berserker** | **Reckless Cleave** — swing, hurts you slightly | **Frenzied Onslaught** — flurry of 5 rapid swings | **Blood Frenzy** — **2 s per 10% of your missing health** |

### Feasibility, per mechanic
Most of tier 2 is a **real SPA on the spell row**, which is always preferable to a Lua payload — the
engine then owns resist, stacking, the buff icon and the message.

| Mechanic | How | Lua needed? |
|---|---|---|
| Absorb N melee hits (Warrior, Monk) | SPA 55 rune + `numhits` / `numhitstype 5` | no |
| AoE stun (Enchanter) | SPA 21 + AE target type | no |
| Group heal / regen (Cleric, Druid, Paladin) | SPA 0, group target type | Paladin only, because it is a % of the CASTER's max HP |
| Damage reduction (Shaman) | SPA 162 flat, or 55 | no |
| Haste (Beastlord, Magician) | SPA 11 / 119 | no |
| Mana + endurance return (Bard) | SPA 15 + 189 | no |
| Delayed second heal (Cleric) | SPA 289 `CastOnFadeEffect` — same trick as the Promised line | no |
| Drain-from-many (Necromancer, Shadowknight) | AoE damage + heal scaled by targets hit | **yes** |
| Pet interactions (Magician, Beastlord) | SPA 218/215/397 read live off the owner | mostly no |
| Guaranteed crit / teleport behind (Rogue) | | **yes** |
| Point blank bow shot (Ranger) | `Lua_Mob::RangedAttack`, already bound | **yes**, one line |
| Consume all DoTs at once (Necromancer) | walk the target's buff slots, sum remaining ticks | **yes** |
| Instrument-scaled damage (Bard) | `GetInstrumentMod` — ⚠️ **Bard-only, returns a flat 10 for anyone else** (§14) | **yes** |
| Bleed amplification (Rogue) | a marker buff read when the next bleed lands | **yes** |
| Stacking nuke amplifier (Wizard) | marker buff with `numhits`, consumed by Overload | **yes** |
| Every tier-3 reduction | the new reduce-recast binding | **yes** |

⚠️ **SPA 213 `PetMaxHP` is read at SUMMON time only** (`pets.cpp:140`) — useless for the Magician's
rune-on-pet. Use 215/397, which are read live in combat.
⚠️ A `formula` of 100 and `max` of 0 on every slot: a level-scaled formula keys off the CASTER's
level, which is wrong for anything cast by a pet or a proc.

---

## 3. Build order

1. **The reduce-recast binding** — nothing in tier 3 works without it. One method, one binding.
2. **One class end to end** (suggest **Warrior**: tier 2 is a pure SPA 55 rune, no Lua) to prove the
   discipline route — scribing at level, the Combat Abilities window, the endurance charge, the
   cooldown cut.
3. **The other fourteen**, in whatever order matters. Each is one migration (3 spells + db_str) plus
   a payload module only where the table above says Lua is needed.
4. **Retire the three Paladin AAs** last, converting them to disciplines — which hands back
   `spell_type` slots 82/83/84 and leaves the AA budget for genuine AAs.

### ⚠️ Ranger needs a bow at creation
Tier III fires the bow, and `aotv4_reforge.M.STARTER_WEAPON` gives a Ranger a short sword (9998) and
nothing else — so the ability is dead on a new character. A Ranger needs a **bow and arrows in the
range slot**, granted alongside the primary at creation and on reforge.
⚠️ It must be an ADDITION, not a substitution: §33 records that the primary has to stay one of the
four weapons Absor accepts or the tutorial becomes uncompletable. The range slot is untouched by that
constraint.

### Things that will bite
- ⚠️⚠️ **A spell with no `db_str` type 6 row renders BLANK and nothing errors.** Write the
  descriptions in the same migration as the rows -- eighteen heals shipped without them.
- ⚠️⚠️ **The client resolves names and descriptions from its own `dbstr_us.txt`.** Every one of these
  needs `./export_client_files` and that file shipped, or they read as the wrong ability.
- ⚠️ `spells_new` is **shared memory** — stack down, `./shared_memory`, restart.
- ⚠️ Auto-scribe at level from `global_player`, the way `grant_class_aas` already does; a discipline
  nobody is told about is not an ability.
