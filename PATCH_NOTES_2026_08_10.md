# AoTv4 Patch Notes — 10 August 2026

## Alternate Advancement

- **AA is now earned as you play, not in a lump when you die.** Your AA bar fills alongside your
  normal experience, and a point arrives roughly at level 20 and again near level 28.
- The total is unchanged — a full climb to the cap is still worth the same **2.32 points**. Only the
  timing has changed, so you get the reward on the way up instead of after the fall.
- Whatever you had banked before this patch was carried over in full. Nobody lost progress.
- Dying is still the only way to keep earning: at the level cap you stop gaining experience, so you
  stop gaining AA until you start a new run.
- The advancement window now opens on its own when you earn a point.
- Fixed AA points occasionally being earned and then vanishing without ever being offered.
- AA points still cannot be spent in the native AA window — the picker is the only way, and that now
  holds for every source, including quest and achievement grants.

## Experience

- **Experience is now capped properly at level 30.** Sitting at the cap no longer quietly banks
  experience toward a level you cannot reach, which was paying out far more AA on death than intended.
- "Welcome to level 35!" and similar messages when crossing several levels at once were wrong — the
  cap was always being applied correctly, the message just lied about it.
- Kaesora has been rescaled to a **level 11-23** zone, and the Zone XP list now says so.

## Songs

- **Songs sung by a non-Bard are now cast once, like any other spell**, instead of being sustained.
  They no longer play forever, no longer lock you out of your abilities, and no longer stop you
  camping out.
- A song that slows or debuffs now lasts its own duration rather than for as long as you sing.
- **Bards are unchanged** — your songs still sustain exactly as before.

## Combat

- **Melee pushback is off.** Nothing shoves you around the field any more.

## Delves

- Killing the boss no longer occasionally leaves no chest. If the chest cannot appear, its contents
  come straight to you.
- **You can no longer leave a delve while something is fighting you.** Exiting mid-fight was the
  cheapest escape in the game — it skipped the death entirely and cost only rewards you had not
  earned yet.
- Wardens hit less hard and have less health when fought alone.
- Onslaught runs advertise **30 minutes** in the journal, which is the clock that was always being
  enforced. It used to claim six hours.

## Items

- The **Refining Crucible** no longer shows junk characters in front of its name when linked in chat.
- The Crucible now refuses a combine if any of the items has an augment in it, instead of destroying
  the augment along with the item.
- Starter weapons rebalanced: the Dagger, Club and Dull Axe hit harder, and the Short Sword — which
  was far stronger than the rest because everyone used to get it — is now in line with them.

## World difficulties — `/pick`

Four difficulties, chosen from a new **Zone Difficulty** window on `/pick` (or the Difficulty button
on `/aot`). Each is a private copy of the world shared with everyone else at the same difficulty.
**Normal is the ordinary world** and always available.

- Difficulty is a **property of your character**, not a place. Pick one and it applies to the zone
  you are standing in.
- **Zoning always returns you to Normal.** Choose a difficulty from inside the zone you mean to fight
  in — it is never carried into a new one.
- You **cannot switch while something is hunting you**, and there is a **5 minute cooldown** between
  switches, so a difficulty is a decision rather than a per-pull tactic.
- Creatures are tougher at every step: **×2 / ×3 / ×4 health**, **+2 / +4 / +6 levels**, and
  **×1.2 / ×1.5 / ×2.1 damage**, with AC, attack and resists rising alongside. Health is the big
  dial and damage the small one — a harder world should cost you time and resources, not delete you.
- **Rewards only come from creatures worth fighting.** Anything that cons light blue or below pays
  nothing extra at any difficulty, so taking Inferno into a starter zone earns you nothing.

### Nightmare — Awake and Unbindable

- Nothing here is fooled by **invisibility, sneak, hide or improved hide**.
- Nothing here can be **mezzed, charmed, rooted or snared**.
- **Stun and fear still work.** Stun is a core melee tool and fear a core Necromancer one; taking
  those away does not make the world scarier, it deletes two classes' kits.
- **Worn Tomes of Insight** begin to drop from creatures that con white or better.

### Hell — champions

- **Champions.** One creature in five spawns carrying a permanent affix. It **glows and is tagged
  under its name** — the way a guild tag sits under a player's — so you always see it before you
  pull: *a gnoll pup* with **&lt;Barbed&gt;** beneath it.

  | tag | what it does |
  |---|---|
  | **Armored** | double armour class |
  | **Hardened** | double health, on top of Hell's own |
  | **Mending** | regenerates steadily — you cannot chip it and walk away |
  | **Frenzied** | attacks about 40 percent faster |
  | **Barbed** | reflects damage back at you on every melee hit you land |
  | **Draining** | its hits take mana and endurance as well as health |
  | **Leeching** | heals itself for a share of the damage it deals |

- **Feigning death is unreliable.** The feign always works — you drop, melee stops — but each
  creature rolls on its own, and about **half** of them are not fooled. You shed part of a camp
  rather than all of it, so feign is still worth using and is no longer a clean escape.
- **Etched Tomes of Insight** drop instead of Worn.

### Inferno — vigilant

- **Creatures notice you from twice as far away.** There is no clean approach and no quiet corner of
  a camp — you commit to a room, not to a pull.
- **Only horizontally.** Nothing gains the ability to notice you through a floor — vertical range is
  unchanged, so a wider radius does not turn a multi-level dungeon into one giant room.
- **Feigning death is more unreliable still** — about **three in four** are not fooled.
- **Radiant Tomes of Insight** drop instead of Etched.

> Each difficulty stands on its own — the harder ones do **not** simply inherit what the easier ones
> do. Only the numbers climb. That means Hell creatures can be mezzed where Nightmare's cannot, which
> is deliberate: each difficulty is a different fight rather than a strictly worse one.

## Tomes of Insight

- A new consumable that grants **one extra reward pick** without levelling you up. Three tiers, one
  per level band — **Worn (1-10)**, **Etched (11-20)**, **Radiant (21-30)** — and each can only be
  used inside its own band.
- Clicking one opens the usual reward window with three choices.
- Don't want any of them? **Decline**, and your reroll price is cut instead — by a quarter, a half,
  or **all the way back to 5p** depending on the tome's tier. The tome is spent either way.
- A tome's offer **cannot be rerolled for coin** — the tome is the reroll.
- They are **No Drop**, stack, and survive death in your bank like anything else.

## Quality of life

- **Camping out no longer hides a level-up reward or an AA choice you had not picked yet.** The
  choice was never lost, but there was no way to see it again until your next level.
- The **Zone XP** list is complete again. It used to stop partway through and cut off Cabilis and
  everything after it.

---

⚠️ **Grab the updated `dinput8.dll` and the UI files.** The advancement window opening on its own,
the full Zone XP list, the **Zone Difficulty** window on `/pick` and the **Decline** button on the
reward window all need them. Everything else in this patch is server-side.

New or changed UI files — copy to `uifiles/default/`:

- `EQUI_AoTDifficultyWnd.xml` *(new — also needs an `<Include>` line in `EQUI.xml`)*
- `EQUI_AoTSpellChoiceWnd.xml` *(changed — the Decline button)*
- `EQUI_AoTMenuWnd.xml` *(changed — the Difficulty button)*
