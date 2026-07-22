# AoTv4 custom spell set — rebuild notes

Source of truth for the design: [spell_design.csv](spell_design.csv) (61 abilities, transcribed
from the design sheet).

## Resist / target decoding

The sheet's **Resist** column conflates two things. Decoded:

| Sheet value | `spells_new.resisttype` | Implied `targettype` |
|---|---|---|
| fire / magic / poison / disease / cold | 2 / 1 / 4 / 5 / 3 | `ST_Target` (5) |
| physical | 8 | `ST_Target` (5) |
| corruption | 9 | `ST_Target` (5) |
| beneficial | 0 (`RESIST_NONE`) | `ST_Target` (5), beneficial |
| self | 0 | `ST_Self` (6) |
| group | 0 | `ST_Group` (41) |

Enum refs: `RESISTTYPE` at [common/spdat.h:980](../../../common/spdat.h#L980), target types at
[common/spdat.h:995](../../../common/spdat.h#L995).

A row with **cast_time = cooldown = duration = 0** is a **permanent passive**, not a castable
spell — 24 of the 61 are passives (all the group buffs, the summons, Concentration/Willpower,
Thorncoat, Reflect, etc.).

## Bucket A — pure `spells_new` data, no code (≈41 rows)

SPA numbers from the `SpellEffect` namespace at [common/spdat.h:1062](../../../common/spdat.h#L1062).

| Ability | Effects |
|---|---|
| Ember | `CurrentHPOnce`(79) -10 + `CurrentHP`(0) -1, dur 5 |
| Zap | `CurrentHPOnce` -10 + `Stun`(21) short (the interrupt) |
| Smolder / Poison / Fever | `CurrentHP`(0) -7 / -3 / -7 over duration |
| Shock | `CurrentHP` -43, no duration |
| Toxic Gas / Infection Cloud | `CurrentHP` DoT, `ST_AETarget`(8) + `aemaxtargets` 4 / 9 |
| Frost | `CurrentHPOnce` -16 + `ArmorClass`(1) -12, dur 6 |
| Chill | `CurrentHPOnce` -27 + `AttackSpeed`(11) 95, dur 5 |
| Lifetap | `CurrentHP` -16 with `targettype = ST_Tap`(13) |
| Dread | `STR`(4) -10 + `CurrentHP` -4, dur 11 |
| Throw Stone | `CurrentHP` -5, `skill` = Throwing, endurance cost |
| Basic Healing / Lay on Hands | `CurrentHP` +17 / +600 |
| Tepid / Desperate recovery | `HealOverTime`(100) +5 dur 10 / +1466 dur 5 |
| Woodskin | `Rune`(55) 13, dur 10 |
| Minor Magic Barrier | `AbsorbMagicAtt`(78) 20, dur 10 |
| Harm Touch | `CurrentHP` -200, `resist_diff` -200 |
| Minor Manaflow | `CurrentMana`(15) +30 |
| Manatap | `CurrentMana` -11 (tap) |
| Yawn | `CurrentEndurance`(189) -10 |
| Sturdy Footing | `Endurance_Absorb_Pct_Damage`(521) base 12 — **exact match**, incl. the endurance drain ratio |
| Passive Protection | `MitigateMeleeDamage`(162) or `Rune`(55) 1, group, permanent |
| Minor Fortify Attack | `SkillDamageAmount`(220) +1 |
| Concentration | `ImprovedDamage`(124) +8% |
| Willpower | `ImprovedHeal`(125) +8% |
| Courage | `TotalHP`(69) +15 + `ArmorClass`(1) +4 |
| Thoughfulness | `CHA`(10) + `INT`(8) + `WIS`(9), +10 each |
| Brawn | `STR`(4) + `STA`(7) +12 |
| Nimbleness | `AGI`(6) + `DEX`(5) +12 |
| Haste | `AttackSpeed`(11) 105 |
| Alacrity | `IncreaseSpellHaste`(127) 5 |
| Firefist / Chi Block | `AddMeleeProc`(419) → Ember / Yawn, + `LimitToSkill`(428) = Hand to Hand |
| Inner Focus | `SpellProcChance`(250) +50 (the "+1 proc cap" half needs code) |
| Ambidexterous | `Ambidexterity`(276) with a huge value |
| Reflect | `Reflect`(158) (the "costs 4% max mana" half needs code) |
| Thorncoat | `DamageShield`(59) 5 |
| Taunt | `Taunt`(199) |

## Bucket B — needs scripting (≈20 rows)

These have no stock SPA equivalent. All are reachable from **Lua** (no C++ rebuild) *if* we drive
them from a spell-effect hook, because the relevant primitives are already Lua-bound.

**%-of-weapon-DPS attacks** — Strike (750% primary), The Left (1000% secondary), Armor Rend (1250%
primary + AC debuff scaled off damage dealt), Open Wounds (300% both weapons + a bleed debuff),
Counterattack (250% primary riposte-style, 5 charges).
SPA `SkillAttack`(193) takes a **flat** `base_value`, not a percentage — but
`Mob::DoMeleeSkillAttackDmg` is Lua-bound at [zone/lua_mob.cpp:1485](../../../zone/lua_mob.cpp#L1485),
so Lua can compute weapon DPS and call it directly.

### Implementation: per-spell Lua

EQEmu resolves `quests/global/spells/<spell_id>.lua`
([quest_parser_collection.cpp:1232](../../../zone/quest_parser_collection.cpp#L1232)), so each
Bucket B ability gets its own script and the spell row keeps carrying name / cost / cooldown /
duration / icon. Shared helpers live in
[lua_modules/aotv4_abilities.lua](../quests/lua_modules/aotv4_abilities.lua).

⚠️ The handler names are `event_spell_effect`, `event_spell_buff_tic` and `event_spell_fade`
([lua_parser.cpp:104](../../../zone/lua_parser.cpp#L104)). There is **one** name for both client
and NPC targets — `event_spell_effect_client` / `_npc` do not exist and silently never fire.

**Done (15):**

| Ability | Script | Approach |
|---|---|---|
| Strike | `50011.lua` | 750% primary DPS via `DoMeleeSkillAttackDmg` |
| The Left | `50054.lua` | 1000% offhand DPS |
| Snipe | `50015.lua` | 900% range DPS (9s of bow) + to-hit bonus |
| Kick | `50010.lua` | boots AC as damage + `InterruptSpell` |
| Breeze | `50025.lua` | buff tic: 1 endurance → 2% max mana |
| Life Ebb | `50027.lua` | buff tic: 4% max HP → 1% max mana, with a 20% HP floor |
| Minor Regeneration | `50028.lua` | buff tic: 3 HP for 5 mana, paid per member |
| Languid Healing | `50017.lua` | heal on `event_spell_fade` after the 2-tick duration |
| Divine Aura | — | negate hits under 15% max HP |
| Blade Turn | `50035.lua` | negate 1 physical hit, 30s lazy recharge for 5 endurance |
| Counterattack | `50056.lua` | 5 charges, retaliate 250% primary DPS if attacker is in front |
| Vengeful Aura | `50059.lua` | 3 charges, double physical damage taken and reflect the final amount |

The last four are driven by one `EVENT_DAMAGE_TAKEN` hook in `global_player.lua` →
[aotv4_reactions.lua](../quests/lua_modules/aotv4_reactions.lua). That event can genuinely prevent
damage: [attack.cpp:4404](../../../zone/attack.cpp#L4404) assigns the Lua return to
`damage_override`, and a **negative** return zeroes the damage while a **positive** one replaces
it — all before `SetHP`.

Three findings shaped that module:

1. **`numhits` can't do the charges.** `numhitstype 6` (IncomingHitSuccess) looks right for "the
   next N attacks", but `CheckNumHitsRemaining` fires at
   [attack.cpp:4336](../../../zone/attack.cpp#L4336) — *before* the event at 4404. On the last
   charge the buff has already faded, so that hit would silently do nothing. Charges are counted
   in the module instead. Trade-off: state is per-zone-process, so zoning refills charges; the
   buffs last 4 tics, so it's rare and favours the player. Persisting would mean a DB write per
   melee swing.
2. **Divine Aura must NOT carry stock SPA 40.** That effect is *total* invulnerability and would
   absorb everything before the 15%-of-max-HP threshold could ever apply. Its spell row is now
   effect-free (`effectid1 = 254`) and the threshold lives in Lua.
3. **Facing check** uses the stock idiom from [attack.cpp:419](../../../zone/attack.cpp#L419):
   `not attacker:BehindMob(me, attacker:GetX(), attacker:GetY())`.

Note these four are `targettype 5` (single-target beneficial), not self-only — the design sheet
marks them `beneficial` while using `self` explicitly elsewhere (Breeze, Life Ebb, Sturdy
Footing), so they are deliberately castable on someone else.

| Ability | Script | Approach |
|---|---|---|
| Open Wounds | `50052.lua` + `50100.lua` | 300% combined DPS, then a 1-min marker banks 25% of each hit and bleeds it out |
| Armor Rend | `50055.lua` | 1250% primary DPS, then the closest rung of a 4-step AC-debuff ladder |
| Duel | `50030.lua` + `50105.lua` | Silence + HealRate −100 from the lock; 2× partner damage and outside-immunity in Lua |

### Helper carriers (ids 50100+)

Applied *by* Lua, never scribed or offered: `classes8 = 255` keeps them out of every class list and
the reward pool only draws from the 61 design ids. `ResistDiff = -1000` so a carrier is never
resisted away. Verified: 0 helpers are scribable.

Three trade-offs worth knowing:

1. **Armor Rend's AC debuff is a 4-rung ladder** (−5 / −15 / −40 / −100). The design wants "5% of
   damage dealt", but a spell's base value is **static** — there is no way to hand Lua's number to
   a buff. Lua computes the damage, takes 5%, and picks the largest rung earned. It scales with
   your weapon instead of being a flat constant, but it is stepped, not exact.
2. **Armor Rend always connects.** It needs the damage number back, and
   `DoMeleeSkillAttackDmg` returns nothing — so it uses `Damage()` with a self-computed value,
   skipping the hit roll. Every other weapon ability can miss.
3. **Duel does not stop fleeing.** "Cannot flee the fight" is the one clause not covered: casting
   is blocked by Silence, healing by HealRate −100, and outside interference by the Lua immunity,
   but nothing pins movement. An NPC that flees on low health still will.

Weapon DPS is `damage / (delay / 10)`; an empty slot falls back to a bare-hand basis so no ability
is ever a dead pick. Damage goes through `DoMeleeSkillAttackDmg`, so these attacks miss, crit and
get mitigated like normal swings rather than landing flat numbers.

### The summons (5) — [gen_summons.py](gen_summons.py)

Not cast: "Pets cannot be summoned via casting and instead will summon automatically outside of
combat." Each is a **scribed trait**, and a repeating `aotpet` timer in `global_player` calls
[aotv4_summons.lua](../quests/lua_modules/aotv4_summons.lua) — out of combat + no pet → summon.
Losing your pet mid-fight means waiting for the fight to end, which is the intent.

⚠️ **`Mob::MakePet` has no Lua binding**, so the only way a script can create a real owned pet is
to finish a spell carrying SPA 33 (SummonPet). Such a spell names **one** pet type in
`teleport_zone`, and pet types are level-banded — hence 40 generated helper spells (5 pets × 8
bands) and a band→spell lookup table.

⚠️ **`pets` rows are per PETPOWER tier, not per level.** `GetPoweredPetEntry`
([pets.cpp:382](../../../zone/pets.cpp#L382)) selects `type` + highest `petpower <= act_power`, so
each type has ~8 rows. Joining `pets` to `npc_types` without `petpower = 0` multiplies every type
by its tiers and scrambles the level ordering — which is exactly the bug that first produced
`L60 → skel_pet_65_` alongside `L65 → skel_pet_61_`.

Bear and Leopard have no usable native family (`BLpet` starts at level 9; `SpiritWolf` covers only
26–58 and is a wolf), so they are **cloned from `skel_pet_`** — the only family spanning 1–68 —
with the race swapped to 43 (bear) and 439 (leopard). Same stat scaling, right appearance, no
invented numbers. The clone uses `CREATE TEMPORARY TABLE ... SELECT n.*` so it survives
`npc_types` gaining columns.

| Pet | Type source | Behaviour ([aotv4_pet_behavior.lua](../quests/lua_modules/aotv4_pet_behavior.lua)) |
|---|---|---|
| Bear | cloned, race 43 | taunt left on to hold aggro |
| Leopard | cloned, race 439 | +100% damage when striking from behind |
| Skeletal Minion | `skel_pet_*` | leeches 25% of each hit, heals the owner's party |
| Fire Imp | `SumFireR*` | 20% chance per swing to cast Ember on its target |
| Willowisp | `SumAirR*` | leeches 25% of each hit as mana for the owner's party |

Pets are identified by **race**, not name — `petnaming 3` renames them after the owner, so name
matching would break immediately. Fire and air elementals share race 75, so those two are told
apart by which trait the owner has scribed.

## Final state

67 + 40 rows: **41 Bucket A, 20 Bucket B, 46 helpers.** Verified against the DB: all 61 design
abilities are scribable at levels 1–60, and **0 helpers are scribable** (`classes8 = 255`), so the
reward pool can never hand out a carrier or a summon helper.

**Other bespoke mechanics:**
- **Kick** — damage = your boots' AC
- **Snipe** — "damage your bow would do in 9 seconds", 1.5× accuracy
- **Languid Healing** — heal that lands after a 2-tick delay
- **Breeze / Life Ebb / Minor Regeneration** — per-tick resource conversion (end→mana, hp→mana, group hp at a mana cost)
- **Divine Aura** — absorb only hits under 15% max HP (stock SPA `DivineAura`(40) is *total* immunity)
- **Blade Turn** — 1-hit rune that self-recharges every 30s, 5 endurance to refresh
- **Vengeful Aura** — double physical damage taken, reflect final damage, 3 charges
- **Duel** — lock two combatants: 2× melee, no flee, no casting, no healing, immune to outside damage, 6 ticks. Biggest single mechanic in the set.
- **Summon Bear / Leopard / Skeletal Minion / Fire Imp / Willowisp** — a bespoke pet system: not castable, auto-summon out of combat, each with a custom behavior (aggro hold, backstab, lifetap-heal-party, cast fire, manaleech-to-party)

## Nothing is permanent

**Rule: no buff on this server is permanent.** Every buff runs the duration its spell data
declares and then fades.

Two changes enforce that:

1. **[zone/spells.cpp:3000](../../../zone/spells.cpp#L3000)** — removed the AoTv4 rule that
   extended beneficial player-side buffs to 1,000,000 tics (~69 days). It silently overrode the
   designed duration of every timed beneficial buff. See the resolved conflict below.
2. **Always-on abilities inherit a native duration profile** instead of using
   `buffdurationformula = 50` (permanent). The 26 abilities whose design row has no cast time,
   cooldown or duration are matched to a native spell of comparable quality:

| Ability | dform / dur | Matched to |
|---|---|---|
| Courage | 11 / 270 | Courage (202) |
| Brawn, Nimbleness | 11 / 270 | Strengthen (40) |
| Thoughfulness | 3 / 400 | Insight (175) |
| Haste | 9 / 110 | Quickness (39) |
| Thorncoat | 3 / 1950 | Thorncoat (519) |
| Concentration | 3 / 1950 | SPA 124 modal (206 spells) |
| Willpower | 3 / 1950 | SPA 125 modal (48 spells) |
| Alacrity | 3 / 1950 | SPA 127 modal (93 spells) |
| Minor Fortify Attack, Ambidexterous, Inner Focus | 3 / 1950 | focus profile (no native SPA 220/276/250) |
| Passive Protection | 3 / 360 | Ward of Vie (SPA 162 modal) |
| Blade Turn | 3 / 360 | Ward of Calliav (SPA 163 modal) |
| Sturdy Footing | 3 / 360 | Ward of Vie profile (no native SPA 521) |
| Reflect | 11 / 300 | Reflective Scales (SPA 158) |
| Firefist, Chi Block | 3 / 600 | Divine Might (693) |
| Breeze, Life Ebb | 3 / 270 | Clarity (174) |
| Minor Regeneration | 10 / 205 | Regeneration (144) |
| the 5 Summons | 0 / 0 | native pet spells — **not buffs at all** (131 natives at 0/0) |

Duration formula 3 is `30 × level` capped by `buffduration`, so these are long but finite and
level-scaled, exactly like their native counterparts. The summons take no buff slot.

## Resolved: the old permanent-buff conflict

[zone/spells.cpp:3020](../../../zone/spells.cpp#L3020) sets `res = 1000000` (~69 days) for **any
beneficial buff on a player-side target**, excluding only invulnerability, `HealOverTime`,
`CurrentHP`, and disciplines (`IsDiscipline` = `mana == 0 && endurance_cost`).

Three abilities were beneficial with a *deliberate* short duration and hit no exclusion, so they
would have silently become permanent. Removing the rule fixes all three — they now hold their
designed durations (verified against the DB):

| Ability | Designed | Was becoming | Now |
|---|---|---|---|
| Woodskin | 10 ticks | permanent | 10 ✓ |
| Minor Magic Barrier | 10 ticks | permanent | 10 ✓ |
| Vengeful Aura | 4 ticks | permanent | 4 ✓ |

Vengeful Aura was the dangerous one: it trades "all physical damage you take is doubled" for a
short retaliation window, so making it permanent turned an ability into a curse.

## The expansion set — [spell_design_extra.csv](spell_design_extra.csv)

The original 61-ability sheet spread over levels 1–30 is about two picks per level, which does not
feel like drawing from a pool. 38 more abilities widen it to **99, an even 3–4 per level**. They
live in their own CSV so the original design sheet stays the untouched source of truth.

All 38 are **Bucket A** — pure spell data, no new Lua:

- **Damage (12)** — upgraded lines off the core elements: Cinder/Scorch/Immolate/Ignite Blood
  (fire), Jolt/Arc Lightning (magic), Rime/Glacial Spike (cold), Venom (poison), Plague (disease),
  Withering Curse and Siphon Life (corruption)
- **Heals (5)** — Mend, Salve, Renewal (HoT), Group Mend, Second Wind (heal + endurance)
- **Defensive (6)** — Barkskin, Stoneskin, Arcane Ward, Shielding, Resist Elements, Thorns
- **Crowd control (8)** — Root, Ensnare, Mesmerize, Lull, Terror, Blinding Flash, Concussion,
  Cancel Magic
- **Utility (7)** — Spirit of Wolf, Invisibility, Levitate, Gate, Bind Affinity, Feign Death, Shrink

### From the unique later-expansion catalog

`EQEmu_Unique_Later_Expansion_Spells.xlsx` lists 66 rarely-seen spells by name and role, with the
numeric columns left blank. Each one below was looked up in `spells_new` to learn how it actually
works, then rebuilt at our scale — chosen for mechanics the pool did **not** already have:

| Ours | Modelled on | Mechanic that was missing |
|---|---|---|
| Sleetfall / Emberfall | Gelid Rains, Rain of Molten Steel | **rains** — `AEDuration` makes an AE fall in repeated waves instead of landing once |
| Veil of Forgetting | Blanket of Forgetfulness | **AE memory blur** (SPA 63) — an escape tool |
| Malaise / Scent of Ruin | Malaise, Scent of Terris | **resist debuffs** — make the elemental lines land |
| Falcon Eye | Falcon Eye | **accuracy** (SPA 184) + see invisible |
| Moonfire | Remote Moonfire | **a spell that triggers another spell** (SPA 374) — damage that self-heals |
| Mind Shatter | Mind Shatter | mana drain *over time* paired with damage |
| War March | War March of Muram | a **bundle** buff — haste + STR + attack + thorns in one cast |
| Chorus of Replenishment | Chorus of Replenishment | health **and** mana regen together |
| Torrent of Misery | Torrent of Misery | **AE hate** (SPA 192) — a threat tool |
| Bond of Xalgoz | Bond of Xalgoz | **lifetap over time** (we only had instant taps) |

Two candidates were rejected after checking: **Aura of the Muse** uses SPA 351
(`PersistentEffect`), which [common/spdat.h:1414](../../../common/spdat.h#L1414) marks
`*not implemented`; and **Balance of Discord** turned out to be an ordinary slow, not the
health-balancing effect its role column suggested.

### ⚠️ Base values were verified against the native spell of the same name

Several SPAs do **not** take the obvious value, and guessing produces a spell that loads fine and
does nothing. Checked and corrected:

| Spell | Assumed | Actual (native) |
|---|---|---|
| Root | `Root(99)` base 1 | base **−10000** |
| Lull | `Lull(18)` | **`ChangeFrenzyRad(30)` + `Harmony(86)`** — SPA 18 is unused by the native Lull line |
| Shrink | `ModelSize(89)` base −40 | base **66** (positive = resulting size %) |
| Gate | base 1 | base **98** |
| Feign Death | base 100 | base **90** (a % success chance) |
| Spirit of Wolf | `MovementSpeed` +55 | **+30** |
| Mesmerize | base 1 | base **2** |

Also worth knowing when reading native rows: **`CHA`(10) is used as a spacer effect**, so many
utility spells have a no-op in slot 1 and the real effect in slot 2 or 3. Reading only
`effectid1` on Root / Ensnare / Levitate / Spirit of Wolf shows `10:0` and tells you nothing.

## Auto-tiered levels

`classes8` doubles as the reward-pool index. Levels are assigned by
[gen_spells.py](gen_spells.py) from a power score = `mana + 2.5×endurance + 0.35×magnitude +
cooldown-class bonus`, rank-mapped across 1..60. Percentage/flag SPAs (Haste's `105` = +5%,
Taunt's `100` = 100% chance) carry an explicit `mag` because their raw base value is not a
magnitude. Retune the `SCORE_*` weights and re-run to reshape the curve.

## Open items

- **Descriptions**: `descnum` is 0 on every new row, so the client shows no description text.
  Descriptions live in `dbstr_us.txt`, not the DB — that file needs generating from the CSV's
  description column and shipping alongside `spells_us.txt`.
- **Client files**: RoF2 reads names/icons/descriptions from `spells_us.txt` + `dbstr_us.txt`,
  *not* the DB. The new set must be re-exported and shipped to clients (see
  [AoTClientFiles/](../../../AoTClientFiles/)).
- `spells_new` is in **shared memory** — changes need world down → `./shared_memory` → restart.
  The array is sized `MAX(id)+1` and densely indexed
  ([common/shareddb.cpp:1622](../../../common/shareddb.cpp#L1622)), so basing this set at 50000
  grows the mmap by ~8 MB (≈1,088 bytes × 7,458 unused slots). Acceptable; the alternative is
  packing at 42603 and losing the visual separation from native ids.
- **Bucket B mechanics** are not implemented yet — those 20 rows exist, are pickable, and have
  correct costs/durations/icons, but do nothing until the Lua lands.
