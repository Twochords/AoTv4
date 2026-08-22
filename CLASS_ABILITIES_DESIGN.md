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

### ⚠️⚠️ CORRECTION (2026-08-21, from building Warrior): `EndurTimerIndex` IS load bearing and MUST NOT be 0
This section previously said to leave it at 0 because "a spell's recast is already keyed on its own
spell id". **That is false for a discipline.** `CastSpell`'s per-spell branch is guarded by
`&& !spells[spell_id].is_discipline`, so `pTimerSpellStart + spell_id` is **never started** for one;
`Client::UseDiscipline` starts `pTimerDisciplineReuseStart + spell.timer_id` instead
(`effects.cpp:981`) with `recast_time / 1000` seconds. Leaving the index at 0 would put every class
ability in the game on **one shared cooldown**, alongside the 87 stock discs already using slot 0.
- ⚠️ **Only 0-10 are valid.** `pTimerDisciplineReuseEnd` is 24 and `pTimerCombatAbility` is 25, so
  timer_id 11 collides with Kick/Bash, 12 with Tiger Claw, 13 with Begging — it corrupts an unrelated
  persistent timer rather than erroring.
- **Three slots serve all sixteen classes**, one per tier: **tier 1 → 5, tier 2 → 6, tier 3 → 2**.
  A character is only ever one class, so nobody can hold two tier 1s. Chosen by how far the lowest
  stock discipline on that slot sits above our cap (68 / 66 / 56); **1/4/7/8/9 all carry discs
  reachable at level 35 or below**. Do NOT allocate per class — there are only 11 slots in total.
- 📌 This is also why `Client::AoTv4ReduceDisciplineTimer` takes a **timer id, not a spell id**.
  Anything written against the spell id silently does nothing and reads as "the cooldown reduction is
  broken".
- `EndurTimerIndex` links cooldowns deliberately -- stock's Defensive/Holyforge/Evasive family shares
  one so using any locks the rest. Here the sharing is across *classes*, which nobody can observe.
- ⚠️ Slots holding only content above our cap look free and are not permanently so. The level cap has
  already moved once (70 -> 35) and the era system exists to unlock expansions later; the day either
  happens, a class ability shares a cooldown with a stock line and nothing reports it. The three
  chosen above are the ones with the most headroom, not the ones that are empty.

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

**A pet you did not grant.** ⚠️⚠️ **REVISED 2026-08-20 — the objection was to DEPENDING on a pet,
and granting one dissolves it.** Pets come from *spells*, which on a random-progression server arrive
from the level-up picker, so a Magician may simply never be offered one and an ability that waits on a
random reward is not an ability. Magician and Beastlord tier 1 now **summon the pet themselves**, so
the dependency is on the class ability rather than on the reward pool. Anything that reads a pet the
player had to acquire elsewhere is still forbidden.
📌 The cost is the re-summon problem in the traps section: a pet's stats are frozen at summon, so
"levels with you" needs a re-summon hook, not a stat buff.

**A group.** Shaman's whole kit was group-facing. Soloing is normal here, and the roguelite loop
means a character spends a lot of its life alone. Group *upside* is fine — Cleric, Bard and Paladin
still have it — but nothing may be worthless without one. Shaman's signature now slows the target
and heals the shaman, so both halves work with nobody else present.

📌 Rule 2 is also what keeps the numbers honest: a guaranteed 5 s off a 120 s cooldown every 15 s is
a 40 percent uptime gain with no play attached. Making it conditional means the ceiling is only
reached by someone doing the class's job well.

| # | Class | L1 | L5 — signature | L10 — cuts L5 by |
|---|---|---|---|---|
| 1 | **Warrior** | **Cleaving Blow** — swing, +hate | **Bulwark** — reduces the next **5** melee hits by **AC/10** each; hits under that are absorbed outright | **Broad Cleave** — frontal cone damage, **no hate component**; **−3 s per target struck**, max −9 s |
| 2 | **Cleric** | **Crusader's Mace** — swing, small self-heal | **Sanctuary** — group heal for **1/6 of each member's own max HP** + cure, healing again after 3 ticks | **Condemn** — smite; **5 s**, **10 s** vs undead |
| 3 | **Paladin** *(built)* | **Ardent Strike** — swing + STR bonus, **weaker than Cleaving Blow**, carries a **taunt** | **Hand of Conviction** — heal group 25% of **your** max HP | **Divine Reproach** — 2 s stun **+ hate**; **5 s**, only if the stun lands |
| 4 | **Ranger** | **Twin Slash** — two swings | **Volley** — AoE ranged burst to everything in front | **Point Blank Shot** — fire your bow in melee; **5 s**, **10 s** with a bow equipped |
| 5 | **Shadowknight** | **Reaving Strike** — swing, leeches HP | **Harrowing** — AoE **lifetap** | **Reaving Vow** — swing amplifying your **next** lifetap by 50%; **6 s** |
| 6 | **Druid** | **Thorned Strike** — the **enemy** gains a reverse damage shield: it wounds itself whenever it lands a melee hit **on anyone** | **Wildgrowth** — strong heal over time | **Sunflare** — direct damage, **bonus damage vs a stationary target** (rooted or snared counts); **5 s**, **10 s** on a stationary target |
| 7 | **Monk** | **Iron Palm** — unarmed strike, `delay × (0.2 + level × 0.035)` | **Void Stance** — **×10 the AGI term of your avoidance** for the next 4 melee attacks | **Pressure Point** — heavy single hit **+ hate**; **6 s**, **12 s** on a crit |
| 8 | **Bard** *(melee amp)* | **Discordant Strike** — swing, **−15% target AC** | **Crescendo** — restore **endurance** to the group | **Cadence Strike** — instrument-scaled damage; target takes **+10% melee damage for the next 5 attacks**; **5 s** |
| 9 | **Rogue** | **Vital Strike** — swing, damages and **snares** | **Rupture** — a heavy bleed over time | **Exploit Weakness** — damage, **next bleed lands 50% harder**; **5 s** |
| 10 | **Shaman** *(tank amp)* | **Spiritual Foresight** — single-target **rune**; anything striking through it is **slowed 15% for 2 ticks** | **Crippling Spirit** — **group rune** + slow scaling **20% → 50%** with level | **Malaise** — **STR / INT / CHA / ATK** debuff; **5 s**, **10 s** if already slowed |
| 11 | **Necromancer** | **Withering Touch** — swing, DoT | **Soul Harvest** — consumes every DoT on the target, dealing remaining damage at once and healing you | **Death's Toll** — instant nuke for **5% of all remaining DoT damage**, or **25%** if that total would already be lethal; **15 s** |
| 12 | **Wizard** *(burst)* | **Arcane Fist** — swing, mana return | **Overload** — nuke, base **20 × level**, self-stuns 2 s | **Ley Tap** — damage; each cast makes your next Overload hit **+15%**, stacking 3×; **5 s** |
| 13 | **Magician** *(pet)* | **Elemental Fist** — swing + fire, **and grants a tank pet that levels with you** | **Elemental Swarm** — a swarm pet alongside the tank pet | **Cinder Blast** — fire damage, **double if the target is your pet's target**; **7 s** |
| 14 | **Enchanter** *(spell amp)* | **Tashania** — **all-resist** debuff ⚠️ *not a weapon blow* | **Gift of Thought** — group mana restore | **Mind Fray** — 3-tick single-target **spell amp** on the caster; **5 s** |
| 15 | **Beastlord** *(pet)* | **Feral Swipe** — swing, **and grants a DPS pet that levels with you** | **Feral Frenzy** — large haste + regen, **applied to you AND your pet** | **Bloodscent** — damage; **at or below 50% target health**, bonus damage **and** the cut rises 5 s → 10 s |
| 16 | **Berserker** | **Reckless Cleave** — swing, hurts you slightly | **Frenzied Onslaught** — flurry of 5 rapid swings | **Blood Frenzy** — **2 s per 10% of your missing health** |

### The three utility classes are force multipliers
Weak alone, and built to amplify one playstyle rather than to compete with it.

| Class | Amplifies | How |
|---|---|---|
| **Bard** | **melee** | −15% target AC, +10% incoming melee for 5 attacks, group endurance |
| **Shaman** | **tankiness** | runes, slow on contact, and an **anti-offense** debuff — STR/INT/CHA/ATK, never resists |
| **Enchanter** | **spell damage** | all-resist debuff, group mana, single-target spell amp |

⚠️ The split is deliberate and must hold: **resist debuffs are Enchanter's domain, heals-over-time are
Druid's, offense debuffs are Shaman's.** Shaman's signature was a HoT in the previous draft and is now
a group rune for exactly this reason.

---

## 2b. Resource costs — no free lunch

**Two cost models, chosen by what the ability is**, never both on the same ability:

| Ability shape | Cost | Why |
|---|---|---|
| A **melee autoskill** (most tier 1s) | §22's formula — endurance = **33% of damage dealt** | It routes through `DoSpecialAttackDamage`, which already charges this. Adding a flat cost on top bills one swing twice, and the damage-scaled half is invisible in the spell row, so it reads as a bug later. |
| Everything else — nukes, buffs, heals, and **Enchanter's tier 1** | flat **`N × level`** | Nothing charges these automatically. |

Flat costs scale linearly with level: **T1 `1 × level`, T2 `5 × level`, T3 `2 × level`.** The resource
follows the ability — endurance for a swing, mana for a nuke, **health for Necromancer**, which is the
class that should be paying in it.

⚠️ Enchanter's tier 1 is **Tashania, not a weapon blow**, so it takes the flat model like any other
spell. It is the one tier 1 that does.

### What the numbers do against real pools
Base at level 30 (`base_data`): **450 endurance, 450 mana, 8 endurance regen per tick = 80/min.**

| Tier | per cast | cooldown | per minute | vs 80/min regen |
|---|---|---|---|---|
| 1 | 30 | 10 s | 180 | — (melee tier 1s use the §22 formula instead) |
| 2 | 150 | 120 s | **75** | ✅ **just under regen — sustainable forever** |
| 3 | 60 | 15 s | **240** | 3× regen — this is the real budget decision |

📌 **Tier 2 landing a hair under passive regen is the good outcome, not an inversion.** A signature you
can press every two minutes without ever going dry is a button you actually use; one that emptied a
full bar would be a slap in the face and would simply never be pressed. The 5× multiplier looks
expensive per cast precisely *because* it is rare, and that is what makes it affordable.

### ⚠️ The one thing to watch is TIER 3's value, not its drain
Value per press is what decides whether a button is worth it, and tier 3 is the only one pressed often
enough for the arithmetic to bite. Eight presses per tier-2 cycle costs **480 endurance** and buys
**40 s** off a 120 s cooldown — about a third of an extra tier 2, which itself costs 150. So **the
cooldown cut alone does not pay for the cost; the tier-3 damage has to carry it.**
📌 That is exactly what the tier-3 rule above already demands — *"it must be worth pressing for
ITSELF."* This is the same rule expressed in resources, and it is the number to check first when
tuning: if a tier 3's damage is not worth 2 × level on its own, the ability is wrong regardless of
how good the reduction is.

---

## 2c. Threat changes

- **Autoskill bonus threat returns.** `AoT:SpecialBonusThreat` is currently **1**, which is what §42
  set it to when specials were stripped of bonus threat to make shield tanking attractive. Raising it
  restores the old behaviour.
- **Bash is double.** `AoT:BashThreatMultiplier` is already **2** and needs no change.
- ⚠️ **This is a deliberate partial revert of §42, and it is necessary.** Without a shield, no player
  is anywhere near able to hold aggro today — that pass stripped bonus threat from every special to
  make shields attractive, and it overshot into "shield or you cannot tank at all". Restoring special
  threat gives the non-shield tank a floor.
  📌 The shield is still strongly rewarded and does not need protecting here: `AoT:ShieldMeleeHatePct`
  adds **+50% threat to every swing AND every spell** while shielded, which applies to the restored
  special threat too — so raising `SpecialBonusThreat` raises the shielded number by 1.5× as much as
  the unshielded one. The gap widens in the shield's favour, it does not close.
- ⚠️ Two tier-1s and two tier-3s now carry explicit hate (Warrior, Paladin ×2, Monk) while **Broad
  Cleave explicitly does not**. Those are ability-level flags, independent of the rule above.

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

### Feasibility of the 2026-08-20 revisions specifically

| Revision | Verdict | Detail |
|---|---|---|
| Warrior Bulwark = AC/10 per hit, 5 hits | ✅ native | **SPA 162** flat per-hit cap, `base 100, limit N` — §14 records it subtracting exactly N per hit. `numhits 5 / numhitstype 5`. ⚠️ N is a **fixed number on the row**, so "AC/10" must be paid by Lua or shipped as a tier ladder. |
| Druid Thorned Strike on the ENEMY | ✅ native | **SPA 121** reverse damage shield — §5: a **debuff on the enemy**, `goodEffect 0`, and **base must be NEGATIVE** (`if (rev_ds < 0)`, attack.cpp:3476) or it silently does nothing. Fires when the mob lands a melee hit **on anyone**, which is exactly the ask. |
| Druid Sunflare vs stationary | **Lua** | No native "target is not moving" check. Rooted/snared is readable (`SpellEffect::Root` / `MovementSpeed` buffs); genuinely-standing-still is not, so treat **rooted or snared** as the whole definition. |
| Monk Iron Palm `delay × (0.2 + level × 0.035)` | **Lua** | Arithmetic in the tier-1 payload. |
| Monk Void Stance ×10 AGI | **C++** | ⚠️⚠️ See the trap below — the knob named is in a **disabled** branch. |
| Rogue evade always, with auto-attack | **C++** | Three one-line changes, see below. |
| Bard −15% AC | ✅ native | **SPA 1** with a negative base. ⚠️ Percentage, not flat — see the trap below. |
| Bard Cadence +10% melee taken, 5 attacks | ✅ native | **SPA 296** `FcDamageAmtIncoming`-style / incoming-melee mod with `numhits 5`, `numhitstype 5`. |
| Bard Crescendo endurance only | ✅ native | **SPA 189** alone. Drop the SPA 15 half. |
| Shaman rune + slow-on-contact | ✅ native | **SPA 55** rune with a **defensive proc** (SPA 323) — §5 records `TryDefensiveProc` routing through `ExecWeaponProc`, so the proc slows the attacker. |
| Shaman slow 20% → 50% by level | ✅ native | **SPA 11**. ⚠️ §5: base is the **resulting attack speed**, so 80 = 20% slow and **lower is stronger**. |
| Shaman Malaise STR/INT/CHA/ATK | ✅ native | SPA 2 (STR), 9 (INT), 10 (CHA), 11x ATK — all plain stat debuffs, negative base. |
| Necromancer Death's Toll % of remaining DoT | **Lua** | Walk the target's buff slots, sum `ticsremaining × per-tick`. Same shape as Soul Harvest, which already needs it. |
| Wizard Overload `20 × level` | ✅ native | ⚠️ **`formula 100 / max 0`** — §5: a level-scaled formula keys off the **caster's** level, which is what you want here, but the Lua-paid companion values elsewhere assume static. Ship it as a tier ladder or accept engine scaling, not both. |
| Wizard stun vs anti-CC | ✅ **already true** | §43's `M.CC_IMMUNITIES` is mez/charm/root+snare only; **stun and fear are deliberately excluded**. Nothing to change. |
| Magician / Beastlord granted pets | **Lua + native** | **SPA 33** summons a pet, and §5 records the pet type living in **`teleport_zone`**, not a destination. ⚠️ See the trap below — "levels with you" is the hard part. |
| Enchanter Tashania all-resist | ✅ native | **SPA 111** `ResistAll`, negative base. ⚠️ §15 records physical resists being unaffected by 111. |
| Enchanter 3-tick spell amp | ✅ native | SPA 302/303 focus with a 3-tick duration. ⚠️ §15: a focus takes the **highest** value, never the sum. |

### ⚠️⚠️ Traps specific to these revisions

**⚠️⚠️ SUPERSEDED (2026-08-21): Void Stance NEEDED NO C++ AT ALL.** It ships as **SPA 172
`AvoidMeleeChance`** with `numhits 4 / numhitstype 1` (Incoming Hit Attempts) — the engine owns the
avoidance roll, the charge counter and the client's charge display, and the feasibility table above
already listed 172 as one of the effects numhitstype 1 counts down. The `compute_defense` analysis
below is correct and is kept because it is the only written record of which branch is live, but
multiplying an AGI term in C++ was the wrong way to buy a 4-attack avoidance spike.

**Monk Void Stance — the AGI knob is in a branch that is switched OFF.**
`Mob::compute_defense` (attack.cpp:249) has two paths. The one with `agi_scale_factor = 1000` — the
literal "agility factor" — is gated on `RuleB(Combat, LegacyComputeDefense)`, and that rule is
**false** here. The live path is the else branch:
```cpp
defense += (8000 * (GetAGI() - 40)) / 36000;      // ≈ (AGI − 40) × 0.222
```
So "×10 the agility factor" has to multiply **that** term instead. At AGI 200 it moves the AGI
contribution from ~35 to ~355, taking total defense from roughly 570 to 890 — about **+56% avoidance
rating**, not immunity.
📌 That difference is the point: Bulwark is a **guaranteed** absorb, Void Stance is a **chance**
spike. It stops the two being the same ability, which was the complaint.

**Rogue evade — three separate gates, all in `Handle_OP_Hide` (client_packet.cpp:8944).**
```cpp
if (!auto_attack && (evadetar && evadetar->CheckAggro(this) && evadetar->IsNPC())) {
    if (zone->random.Int(0, 260) < (int)GetSkill(EQ::skills::SkillHide)) { RogueEvade(evadetar); }
```
1. **`!auto_attack`** is what blocks evading while attacking — remove it.
2. The `random.Int(0,260)` roll is what makes evade occasional — remove it to make it every time.
3. `Mob::RogueEvade` (aggro.cpp:1753) sets hate to a random **40-70%** of current, i.e. cuts 30-60%.
   For a flat 20% cut that becomes a fixed **80%**.
⚠️ `HideReuseTime` is **8 s** minus skill-based reuse reduction, floored at 1 s — so at capped Hide
this becomes a near-spammable detaunt. The 20% cut is what keeps that sane; do not also shorten it.

**⚠️⚠️ CORRECTION (2026-08-21): SPA 1 IS FLAT, SO −15% IS NOT SHIPPABLE AS WRITTEN.**
`bonuses.cpp:648` is `newbon->AC += base_value` — there is no percentage form of SPA 1. The built
ability (Discordant Strike, 44721) therefore carries a **flat −25**, which is ~15% of the average
level-30 mob's 152 AC and was chosen from that measurement rather than from the level. The analysis
below is still the right analysis; only its conclusion ("ship the −15% directly") was unbuildable.

**Bard AC debuff — the flat approximation, and why 10 x level was wrong.**
15% is the design target; a flat `10 × level` was floated only as an easy way to approximate it. It
does not approximate it. After the NPC rescale, average mob AC at level 30 is **152** (max 529 on
elites), so `10 × 30 = 300` is **roughly twice a typical mob's entire armour** — it would zero out AC
on everything non-elite and add nothing on the elites where the debuff matters.
**Ship the −15% directly** (SPA 1, negative base, percentage): 23 on an average mob, 79 on a 529-AC
elite. It scales with the target, which is the behaviour the flat version was trying to imitate.

**Pets that "level with you" — solved by rescaling, not re-summoning.**
A pet's stats are fixed **at summon** from the caster's level (`Mob::MakePet`, pets.cpp:140 — the same
reason §15 records `PetMaxHP` being useless on an existing pet), so a pet granted at level 1 does not
grow on its own. **The fix is a Lua rescale on level-up**, exactly as `aotv4_dungeon_scale` already
does for delve creatures: `ModifyNPCStat` on `max_hp`, `min_hit`, `max_hit`, `ac`, `atk` and `level`
from `global_player.event_level_up`.
⚠️ Two traps that module already records apply here: **`ScaleNPC` must come before any
`ModifyNPCStat`** (it rewrites stats wholesale and discards anything applied first), and **`max_hp`
clamps DOWN only** — raising the maximum does not raise current health, so it needs an explicit
`SetHP` or the pet levels up and stays wounded.
📌 This resolves the earlier "cannot be pet based" objection: the ability **grants** the pet rather
than depending on a pet spell arriving from the random reward pool, so the dead-ability risk is gone.

**Enchanter tier 1 is not a weapon blow, and takes the flat cost model.**
Tashania breaks the uniform shape — every other class's tier 1 is a swing routed through
`DoSpecialAttackDamage`. That is a deliberate exception for the one class that should never be
meleeing: it carries a **flat `1 × level` mana cost** like any other spell, and does not use the
tier-1 damage model. §2b covers this; it is the reason two cost models exist rather than one.

**Cleric vs Paladin, the comparison that was asked for.**
Paladin heals the group **25% of the Paladin's own max HP** — one number applied to everyone.
Cleric at **1/6 (16.7%) of each member's own max HP** is per-member, so it is *stronger for anyone
with more HP than the Paladin* and weaker for anyone with less. Against a tank at 2× the Paladin's
HP, the Cleric heals 33% of the Paladin's pool to that tank versus the Paladin's 25%. At **1/8**
(12.5%) it only wins on a member with 2× the Paladin's HP, which is why 1/6 is the right floor.

⚠️ **SPA 213 `PetMaxHP` is read at SUMMON time only** (`pets.cpp:140`) — useless for the Magician's
rune-on-pet. Use 215/397, which are read live in combat.
⚠️ A `formula` of 100 and `max` of 0 on every slot: a level-scaled formula keys off the CASTER's
level, which is wrong for anything cast by a pet or a proc.

### 📌 Parked for later — Lifeburn (Necromancer)
Requested as a future addition. The EQ2 shape — convert a large share of your health into damage,
with the tension being *"do I finish it, or cancel before the threat lands and kills me"* — needs two
things this design already has: threat that actually accumulates (§2c) and a health-cost mechanic
(the Necromancer pays in health throughout). The cancellable channel is the missing piece; there is no
native "player-cancellable damage-over-time on yourself that scales threat" and it would be a Lua
payload on a self-buff with `numhits`, cancelled by clicking the buff off.
⚠️ Do not build it until threat is retuned and measured — its entire appeal is the threat race, so it
is meaningless while `SpecialBonusThreat` is unsettled.

---

## 2d. ⚠️⚠️ Four traps found BUILDING Warrior — every one of them is silent, and every one recurs

These bit while writing the reference class. All four generalise to the other fifteen.

**A Lua-paid charge effect cannot use `numhits`.** `CheckNumHitsRemaining` spends the last charge and
**fades the buff** at `attack.cpp:4621`; `EVENT_DAMAGE_TAKEN` fires at `attack.cpp:4689` — later in
the *same* `Mob::CommonDamage` call. So the payload finds no buff on the final hit and does nothing:
an ability that advertises five absorbs and delivers four, with no error. The engine counter is only
safe when the **engine** also pays the effect (a real SPA). Count charges in Lua and fade the buff
yourself. ⚠️ This applies unchanged to **Monk Void Stance**, and to anything else in the table above
whose "absorb / for the next N attacks" is paid by a script.

**`Mob::InFrontMob` has no Lua binding on this build** — only `BehindMob` does — so any cone ability
(Warrior Broad Cleave, Ranger Volley, Berserker Frenzied Onslaught) must be written as
`not m:BehindMob(c, m:GetX(), m:GetY())`, which is a **180° arc**, not InFrontMob's 56° one.
⚠️ `MobAngle` reads the **heading of the `other` argument** and the position passed in, so the caster
must be the argument and the creature's own coordinates the point; written the other way round it
measures the creature's facing instead of the player's.
⚠️⚠️ And an AoE must not pick targets by proximity alone: `IsAttackAllowed` says the banker behind
your target *can* be hit, not that it should be. Broad Cleave is limited to the current target plus
anything already holding the caster on its hate list — one cleave through a town is a faction hit
nobody asked for (§28 exists because being attacked for existing is miserable).

**`DoSpecialAttackDamage` takes at most SIX arguments** (other, skill, base_damage, min_damage,
hate_override, reuse_seconds). A seventh matches no overload and is a luabind error at runtime.
⚠️ **Pass the equipped weapon's own skill, not Frenzy.** `my_hit.offense` and `GetTotalToHit` are
both computed from the skill argument (`special_attacks.cpp:296`), and on this server a Warrior may
genuinely have **0 Frenzy** — §4: a cap exists for every class, but the *value* is only granted if the
special is native or picked as a reward. Naming an untrained skill quietly wrecks the to-hit roll, and
it makes the client's damage message read "you frenzy on" instead of an ordinary weapon hit. The
shared `aotv4_class_abilities.weapon_profile` maps `itemtype` → skill and reads the weapon's damage
with `eq.get_item_stat`, which counts gear tiers for free (a Mythic is its own row).
⚠️ `hate_override` is pointless for anything but Bash: `AoT:SpecialBonusThreat` overwrites it for
every other special (`special_attacks.cpp:293`). An ability that wants threat adds it **explicitly**
after the swing, which is what Cleaving Blow does.

**A self-buff discipline cannot be recast while any disc buff is up** — `UseDiscipline` refuses on
`HasDiscBuff()` (`effects.cpp:977`). Fine for Bulwark (it stops charges being topped up early), but
any tier 2 that is a self buff inherits it, and two self-buff disciplines on one class would lock
each other out.
📌 `buffduration` is a **cap on** `buffdurationformula`, not an alternative to it
(`spells.cpp:3300`), so formula 11 with duration 10 is a flat 10 tics at every level.

### ⚠️⚠️ Five more traps, found building the other fourteen (2026-08-21)

**⚠️⚠️ `IsDiscipline` IS A COLUMN AND IT IS NOT THE SAME THING AS THE `IsDiscipline()` FUNCTION.**
This is the one that made the abilities read as spells, and it survived three separate checks that
all said they were disciplines. The **function** (`common/spdat.cpp:1190`) is DERIVED --
`mana == 0 && (EndurCost || EndurUpkeep)` -- and was true for all 51 rows from the day they landed.
The **column** is `spells_new` field **168**, loaded into `spells[].is_discipline`, and its header
comment in `spdat.h:1686` is *"Will goto the combat window when cast -- IS_SKILL"*. A clone inherits
it like any other column, so the 19 rows cloned from 4499 Defensive Discipline got it and the **32
cloned from 4667 / 285 / 3265 did not** -- those three templates are not disciplines.
- ⚠️ Stock writes **-1**, never 1. `Strings::ToBool` accepts any non-zero number, but match stock.
- ⚠️ It is read by `Client::CastSpell` (`spells.cpp:2909`, `:2937`) to decide whether a recast goes
  to `pTimerSpellStart + spell_id` instead of the shared discipline slot, and by
  `Client::CalcHaste`'s bard branch (`client_mods.cpp:1503`).
- 📌 **Only 281 stock spells carry it**, against thousands that pass the derived function. If you
  want to know whether something is a discipline, read the column.
- Repaired by **v120**, and the generator now writes it inline.
- 📌 Skill 98 is fine and needed no change: **34 stock disciplines already use it.**

**⚠️⚠️ `DoSpecialAttackDamage` DOES NOT TRAIN THE SKILL.** It contains no `CheckIncreaseSkill` at
all -- every stock special calls it separately *after* the damage (Bash `special_attacks.cpp:533`,
Frenzy `:563`, Kick `:696`, Backstab `:899`). That is easy to miss because the calls look like part
of the special rather than part of the swing, and without it these were the only melee attacks in
the game that never raised the weapon skill they use. `M.weapon_blow` now calls it, chance_mod 10.

**⚠️⚠️ `range = 200` MAKES A MELEE ABILITY LONG-RANGE.** Copied from the stock disciplines, where it
is harmless because those are self buffs -- but a TARGETED ability is range-checked against it in
`Mob::CastSpell` (`spells.cpp:2600`, plus a target-size mod), so Ardent Strike could be swung from
200 units away. Reported from play. The 26 abilities whose payload actually swings are now **25**;
heals, nukes, runes, pet summons and group buffs keep 200.
📌 The list is *"which payloads call `M.weapon_blow`"*, not *"which use the MELEE preset"* -- several
swings wear a DEBUF or NUKE look because they also apply a real SPA (Thorned Strike, Discordant
Strike, Withering Touch).

**⚠️⚠️ AN ABILITY CAN BE DEAD IN BOTH HALVES AT ONCE, and neither list shows it alone.** Intersecting
"row has no real SPAs" with "module has no PAYLOAD entry" found **Arcane Fist** -- an inert row with
no payload, so pressing it did literally nothing. Run that intersection after any change here.

**A clone inherits its template's PRESENTATION, not just its numbers, and it took FOUR migrations
to finish paying for that.** Icons, `spellanim`, `CastingAnim`, the cast messages, the `IsDiscipline`
column and **`player_1`** all come across. `player_1` is the particle/trail graphic and the instant
template carries **BLUE_TRAIL**, so 29 rows fired a blue projectile trail on what are meant to be
sword swings; 263 of the 281 stock disciplines use `PLAYER_1`, the no-effect value, and so does every
spell the presets were sampled from. Repaired by v119 (messages), v120 (IsDiscipline) and v121
(particle).
📌 **The pattern: after cloning, diff the new row against a row of the shape you actually want -- not
against the template you cloned.** Every one of these was invisible to "does the ability work".

**The original note, kept because the reasoning generalises.** Icons, `spellanim`,
`CastingAnim` **and the cast messages** all come across. Every ability shipped saying *"You are hit
by an invisible force."* or *"You assume a defensive fighting style."* — a group heal announcing
itself as a defensive stance — and Warrior's three shared two icons and fired a spell particle on
what is meant to look like a swing. Fixed by v119 and by setting presentation explicitly in the
generator. **This is the third time this class of bug has landed** (v56's `descnum`, §5's damage
formula, now this): *the columns that hurt are the ones with no obvious connection to what you
changed.*

**The payload runs BEFORE the spell applies its own effects.** `EVENT_SPELL_EFFECT_*` fires at
`spell_effects.cpp:163`; the effect-slot loop starts at `:225`. So a pet summoned by SPA 33 does not
exist when the script runs, and a buff's stat changes have not happened yet.
⚠️⚠️ **Returning non-zero from that event CANCELS the effects outright** (`:172`) — a payload that
returns a value silently deletes the native half of its own ability.

**`DoSpecialAttackDamage` returns void.** The damage actually dealt is unknowable from Lua, so every
"leech a share of the damage" rider in the design is implemented as a **level-scaled flat amount**,
and "shorten the cooldown on a crit" (Monk Pressure Point) is a flat cut instead.

**`eq.get_spell_stat`'s slot argument is 1-BASED.** `GetSpellStatValue` does `if (slot > 0) slot -= 1`
(`common/spdat.cpp:2433`), so slots 0 and 1 both read effect slot 1. A loop from 0 reads the first
slot twice and never sees the third. ⚠️ It is also the reason a Necromancer can detect somebody
else's DoT at all — but **`ticsremaining` has no binding**, so "all remaining DoT damage" is
approximated as three ticks of each affliction.

**A migration's check must test what the SQL CREATES, never what it blanks.** The messages migration
first keyed on a column it sets EMPTY, and an empty result reads back as the **column name** rather
than as an empty string — so `not_empty` was true forever and world would have re-run it on every
boot. The validator's idempotency pass caught it; nothing else would have.

---

### ✅✅ SOLVED PROPERLY: `EQ_Character::HasCombatAbilities` at **0x582350** (2026-08-22)
The Combat Abilities icon was blacked out and the window would not open for a pure caster. It is a
**class predicate, exactly like `IsSpellcaster`** -- `core_allcasters.cpp` now detours it to return 1
and every class gets the button and the window.
- `CSelectorWnd`'s update calls it on the local PC at **0x751fc7** and feeds the bool straight into
  the combat button's state setter (**0x866610**). Return 0 and the icon is dead.
- It reads the class byte (char struct **+0x3374**) and indexes a 16-entry table at **0x58238C**,
  jumping through **0x582384** (entry 0 -> return 1, entry 1 -> return 0):
  ```
  00 01 00 00 00 01 00 00 00 01 01 01 01 01 00 00
  War T  Clr f  Pal T  Rng T  SK  T  Dru f  Mnk T  Brd T
  Rog T  Shm f  Nec f  Wiz f  Mag f  Enc f  Bst T  Ber T
  ```
  The nine TRUE entries are exactly the classes with disciplines on live, and exactly the set where
  the window already worked.
- 📌 It sits 5KB from `Max_Mana` (0x581E60), already detoured here. **The client keeps these little
  class predicates together** -- when one is found, look for siblings nearby.

#### ⚠️⚠️ HOW IT WAS FOUND, AND WHY THE FIRST SIX ATTEMPTS FAILED
Searching the WINDOW, the keybind handler and the button for a class gate found nothing, because
they are all genuinely class-blind. Six attempts at reasoning from the disassembly produced six wrong
answers: the `+0x1d4` activation flag (section 13's `CBazaarWnd` note, which does NOT apply --
`CSkillsWnd` opens fine with it at 0), `vtable[0x170]` (which was itself SETTING the field the probe
then "found"), `+0x1e4`, `+0x19` (a byte-order slip -- the real one was `+0x1a`), and forcing the
button's enabled byte (which lit the icon but left the window shut).
**What worked: follow the button's state BACKWARDS to whoever supplies the bool.** One `grep` for
writes to `CSelectorWnd+0x2a0` landed on 0x751fc7 and the predicate was the instruction before it.
📌 The lesson for next time: when a widget is disabled, do not ask "what disables it" -- ask **"who
computes the value it is set from"**, and grep for the writes to that field.
⚠️ A probe must never share a tick with a write to the thing it probes; two rebuilds were spent
chasing our own footprint.

### (superseded, kept for the byte offsets) The greyed icon is `+0x1a` on the BUTTON
The Combat Abilities icon on the Window Selector was blacked out for a caster. **Fixed and confirmed
in game**: `CSelectorWnd+0x2a0` is `SELW_CombatSkillsToggleButton`, and the byte at **button+0x1a**
is 0 on the greyed button and 1 on one that works. Forcing it to 1 lights the icon and makes it
clickable. `AllCastersTick()` in `core_allcasters.cpp` does it, and the log shows the client sets it
**once per session**, so it is a single write.
- ⚠️⚠️ **IT IS A BYTE INSIDE A DWORD, WHICH IS WHY FOUR ROUNDS OF DWORD DIFFING MISSED IT.**
  `+0x18` reads `0000FF01` on the greyed button and `0001FF01` on the working one; little-endian,
  that is `+18=01 +19=FF +1a=00/01`. Reading a dword diff as if it were in address order sends you to
  `+0x19`, which is `0xFF` on both.
- 📌 **The method that found it is the only one that worked all session: diff the broken widget
  against a working one and let the difference speak.** Four attempts at reasoning from the
  disassembly (activation flag, `vtable[0x170]`, `+0x1e4`, `+0x19`) were all wrong.
- ⚠️⚠️ **NEVER LET A PROBE SHARE A TICK WITH A WRITE TO WHAT IT PROBES.** Calling `vtable[0x170]`
  was itself setting `+0x1e4`; the diff then "found" that field and two rebuilds were spent chasing
  our own footprint.

### ⚠️⚠️ UNSOLVED: the window still will not open, and `+0x1d4` was a red herring
Clicking the (now working) icon animates the button and **never flips the window's shown flag**
`+0x196`, so the click never reaches `Show`. 
- ⚠️⚠️ **`CSkillsWnd`, WHICH OPENS FINE, ALSO HAS `+0x1d4 = 0`.** So the `CSidlScreenWnd` "activated"
  flag that section 13 records for `CBazaarWnd` is **NOT** a precondition for a window to open, and
  forcing it only pinned the selector button in the pressed state (that flag drives the button's
  toggle, not its availability). Do not repeat that experiment.
- A full diff of the combat window against `CSkillsWnd` over the shared base shows only position and
  layout differences -- no boolean of the kind that fixed the button.
- 📌 Next lead if anyone returns to this: the **selector's click handler**, not the window. The
  button's own state is now byte-identical to a working button, so whatever declines to open it is in
  `CSelectorWnd`'s notification path, not in `CCombatAbilityWnd` and not in the button.

### (superseded) The search that ruled out a class gate
The Window Selector icon was blacked out for a Cleric and Alt+C did nothing. **It is not a class
check -- there is none anywhere in the path.** The window is created for every class, its
constructor, the show dispatch and the entire keybind handler are class-blind, and the only class
read in the window's own code is the per-row population filter, which works (Warrior and Paladin see
our abilities listed).
- The Window Selector stores its button as **`SELW_CombatSkillsToggleButton`** at `CSelectorWnd+0x2a0`
  and, at **0x7524c9**, sets that button's state from **`pinstCCombatAbilityWnd->+0x1d4`** -- read
  through `0x864140`, which is literally `mov 0x1d4(%ecx),%al; ret`.
- ⚠️⚠️ **+0x1d4 is the `CSidlScreenWnd` ACTIVATED flag -- the SAME field section 13 documented on
  `CBazaarWnd`**, where `Show()` on a never-activated window draws nothing. Section 13 even recorded
  the Bazaar's writes to it (`0x652bba` / `0x654d28`). The window is simply never activated for a
  class that has no disciplines on live.
- **Fix: set the byte.** `AllCastersTick()` in `core_allcasters.cpp` re-asserts it every frame while
  in game. That module is the right home -- it already carries three sibling patches that make the
  client stop treating the four melee classes differently; this is the same thing in reverse.
- ⚠️ Re-asserted per frame rather than once: the client clears it on deactivate (`0x864150`) and a UI
  reload rebuilds the window with it back at 0.
- 📌 **The lesson is section 13's, and it cost this whole hunt to relearn:** when a stock window will
  not appear, check the ACTIVATED flag before looking for a class gate. Two windows now, same byte.

### The search that got there (kept -- it is what rules a class gate out)
Disassembled `eqgame.exe` (it is at `/src/eqgame.exe`; `objdump -d` handles it as pei-i386) looking
for the class gate. **I did not find one, and these are the places it would be:**
- **The window is CREATED UNCONDITIONALLY for every class.** At `0x498eba` the UI-load path is
  `cmpl $0x0, 0xd1fca0; jne skip` -- the only test is "does it already exist". Its constructor
  (`0x65b3c0`, size 0x278) contains no class test either.
- **The window-id dispatch at `0x487d72`** is a plain switch to the instance pointer followed by
  `vtable[0x90]` (the show wrapper) -- no class test.
- **Only TWO functions in the whole binary read the class byte (`+0x3374`) into a jump table**, and
  one of them is `IsSpellcaster` (`0x443f50`, table at `0x443f90` = `00 01 01 01 01 01 00 01 00 01
  01 01 01 01 01 00` -- index is class-1, so Warrior/Monk/Rogue/Berserker are the zeroes, matching
  section 14 exactly). Neither is a combat-ability gate.
- ✅ **There is a bindable keybind: `CMD_TOGGLE_COMBAT_ABILITY_WIN`.** So the window can be opened by
  a key regardless of any menu entry, which is the likeliest explanation for "melee classes can open
  it and casters cannot" -- a menu entry, not a hard gate.
📌 **Next step is a TEST, not more RE**: bind that key on a caster. If the window opens and lists the
abilities, there is nothing to fix. If it opens EMPTY, the gate is in list population and the search
resumes there.
⚠️ Do not start writing a detour before that test. Section 13 records a long dead end doing exactly
that on `CBazaarWnd`, which also existed in every zone and still would not render.

### ⚠️⚠️ If it does need a client fix, build OUR OWN window -- section 13's conclusion
RoF2 hides the combat-abilities UI from classes that never have disciplines on live, so a Cleric or
a Wizard here has real disciplines and nowhere to click them. **This is the exact mirror of section
14**, where the client hides the spellbook and gem bar from the four pure-melee classes and
`core_allcasters.cpp` forces `EQ_Character::IsSpellcaster` to 1 to get them back. The equivalent
patch has not been written, and it needs the Windows/VS build.
- 📌 The addresses are already in the dll's table: `pinstCCombatAbilityWnd` **0xD1FCA0**,
  `pinstCCombatSkillSelectWnd` **0xD1FC08**, `EQ_PC__GetCombatAbility` **0x7C44F0**,
  `EQ_Character__doCombatAbility` **0x5808C0**. Start there.
- ✅ **Workaround shipped: `#ability 1|2|3`** (`lua_modules/commands/ability.lua`, access **0**). It
  goes through the SAME `Client::UseDiscipline` entry point as the hotbutton, so the level gate, the
  endurance cost and the recast timer all still apply -- it is another way to press the button, not
  a way around it. A player can put `#ability 1` in a **social** and drag that to a hotbar, which
  gives every class a working button today with no client change.

---

## 3. Build order

1. ✅ **The reduce-recast binding** — `Client::AoTv4ReduceDisciplineTimer` (`zone/effects.cpp`) +
   `Lua_Client::AoTv4ReduceDisciplineTimer`. ⚠️ Takes a **timer id**, not a spell id.
2. ✅ **Warrior, end to end** — migration **v104** (spells 44700-44702 + their `db_str` type 6 rows),
   `lua_modules/aotv4_class_abilities.lua` (the shared payload: cost, swing, grant, absorb),
   `global/spells/44700-44702.lua`, and four `global_player.lua` call sites (connect, level up,
   **death**, and the damage-taken chain).
   ⚠️ Tier 2 is **not** the pure SPA 55 rune this line originally assumed — `AC/10` per hit is not a
   number that can live on a spell row, so Bulwark is an inert marker paid in Lua like the rest.
   ⚠️⚠️ **UNTESTED IN GAME.** It compiles, luachecks and the migration dry-runs clean, but no
   character has pressed any of the three. §30 is where this belongs until it has been played.
3. ✅ **The other fourteen** — migrations **v105-v118**, one per class, all generated by
   **`custom/tools/gen_class_abilities.py`** from a single spec table, together with the 45 Lua
   stubs. 51 rows: 45 abilities + 6 helper/trigger spells at 44750-44755.
   ⚠️ The generator is the source of truth for the band. Hand-editing a generated `.sql` or stub is
   reverted by the next run — and the fourteen class migrations are **already merged**, so
   regenerating them would drift from the manifest. That is why the cast messages (v119) are their
   own migration rather than being folded back into each class's.
   ⚠️⚠️ **UNTESTED IN GAME.** Applied to the dev database, shared memory rebuilt, client files
   exported. Nobody has pressed any of them.
4. ✅ **Retired the three Paladin AAs** — migrations **v122** (disciplines 44706-44708) and
   **v123** (disable hosts 45/55/79, delete the trained ranks), plus `CLASS_AAS` emptied. Hands back
   `spell_type` slots 82/83/84 and leaves the AA budget for genuine AAs.
   ⚠️ **They are a PAIR.** The spells alone give a Paladin each ability twice; the retirement alone
   gives them none.
   📌 Behaviour was ported verbatim from `aotv4_paladin.lua` into the shared module -- Ardent Strike
   still scales off Strength, Hand of Conviction still heals a quarter of the CASTER's max HP. Only
   the delivery changed. **All sixteen classes are now on one mechanism.**
   📌 **This is the only class not built**, and deliberately: its three abilities already work as
   AAs, so `M.BUILT` omits it. Listing it there before the conversion would hand a Paladin the same
   three abilities twice.

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
- ⚠️⚠️ **AND RE-GRANT ON DEATH.** `death_loss` calls `UntrainDiscAll`, so the roguelite wipe takes the
  class abilities with it — they live in `character_disciplines`, not the spellbook. Class AAs are
  permanent and need no such hook, which is exactly why `grant_class_aas` has only two callers and
  `aotv4_class_abilities.grant` has three.
- ⚠️ **`TrainDiscBySpellID` validates nothing** — not class, not level, not whether you already know
  it (`effects.cpp:883`). It fills the first empty slot, so without a `HasDisciplineLearned` guard the
  same ability is listed twice.
