# AoTv4 Patch Notes — 14 August 2026

## Gear

- Every piece of wearable gear in the game has been rescaled from a single model — armor class, health, mana, resists, all seven stats, weapon damage and range
- Gear now carries a recommended level, so you can tell at a glance what a piece is meant for
- Hallowed and Mythic tiers rebuilt from the new numbers
- Fixed thousands of Mythic items having **less** armor class than the base item they are supposed to upgrade
- Fixed Hallowed and Mythic weapons dealing identical damage instead of stepping up
- Crafted gear keeps its augment sockets — one on a normal item, two on Hallowed, three on Mythic
- Delve augments, the Delver's Sigil and the tradeskill tools, masks and gloves were deliberately left out of the rescale, so nothing you are already wearing changed underneath you

## Monsters

- Monster health, armor, damage, resists and stats rescaled across every organised zone

## Alternate Advancement

- **The entire healer AA line has never worked and now does.** Overflowing Grace, Triage Instinct, Mender's Echo and Borrowed Breath never fired from a direct heal or from a lifetap — only from a heal-over-time tick, which is the one case they are written to ignore
- **Overflowing Grace** turns healing wasted on a full-health target into a melee shield
- It now works even when the heal is *entirely* wasted, which is the obvious way to test it and previously gave nothing at all
- The shield refreshes rather than stacks, and never downgrades a bigger shield that is still running
- Thirty second cooldown per target, so it cannot be built up by parking a heal on one person
- Capped at a fifth of the target's maximum health
- **Steady Nerve is now Sinister Strikes** — increases your offhand damage
- **Run Them Down is now Blindside** — five ranks of offhand accuracy
- Nothing Left to Fear has been removed
- Both of the old AAs bought something this server already gives away or forbids: fear spells are not in the reward pool, and monsters already never flee

## Travel — new

- **Hidden waypoints are now scattered across the world.** Walk near one and you commit it to memory: *"This spot seems of importance, you seem to remember it."*
- Thirteen to find, spread across all six regions — Mistmoore, Unrest, Butcherblock Docks, EC Tunnel, Runnyeye, Iceclad, Ry'Gorr, Velketor, Karnor's Castle, Mines of Nurga, Splitpaw, Veksar and the Crypt of Dalnir
- Nothing marks them. You have to walk over them
- **New "Attune the Map" window**, opened by clicking any Plane of Knowledge book — regions on the left, that region's destinations on the right
- Travel is **one way**: you can port to a spot you have found, not back from it
- **Port Group** takes your whole group, and refuses unless every member qualifies, naming anyone who does not
- **Auto Confirm** skips the confirmation box; leave it off and you are asked before you move
- Regions you have not opened still show how much is left to find there, without telling you where
- Plane of Knowledge books are now listed inside the region they actually lead to, instead of in a section of their own
- Seventeen books that only lead to zones this server does not open are no longer tracked at all — they were showing as holes in your map that could never be filled
- `/travel` prints what you have found and nothing else; the window opens at a book, deliberately
- A Plane of Knowledge book has been added to the Resplendent Temple
- The Butcherblock Docks book has been removed — that destination is now one of the hidden waypoints

## Level-up rewards

- **A spell you already know at a higher rank is no longer offered back to you as the weaker version**
- A spell you have chosen to keep is never offered again in any form
- Fourteen more spells removed from the reward pool: the blind line, Sentinel, the Eye of Zomm line, Reclaim Pet, Voice Graft, Flare, and the corpse-locating spells
- Identify has been removed — the item window already shows you everything it would tell you
- Fear and Panic Animal were already excluded and remain so

## Tomes of Insight

- Fixed clicking a Tome of Insight failing with an error and consuming nothing
- Fixed a Tome's description showing the text for an unrelated Shield Wall effect

## Zones opened

- **The Plane of Hate is now enterable.** It sits in the Freeport region and has always had a way in,
  but it carried a minimum level of **46** against a level cap of 30, so no character could pass the
  door. That requirement is gone
- **Kaesora** and **The Sleeper's Tomb** are open
- The Plane of Hate was the only zone in an open region still walled off this way — every other zone
  your regions cover is genuinely reachable

## World difficulties — Normal, Nightmare, Hell and Inferno

### How you get in

- Type **`/pick`**, or press **Difficulty** on the `/aot` menu, and choose from the four
- The zone you are **standing in** becomes a private copy of itself at that difficulty, shared with
  everyone else who picked the same one. You do not travel anywhere — you stay exactly where you are
- **Normal is the ordinary world**, always available everywhere, and it is how you get back
- **Difficulty follows your character, not the place.** Pick one and it applies to the zone you are in
- **Zoning always returns you to Normal.** A difficulty is never carried into a new zone, so you
  choose it from inside the zone you mean to fight in
- **You cannot switch while something is hunting you**, and there is a **5 minute cooldown** between
  switches — including switching back down. A difficulty is a decision, not a per-pull tactic
- **Now available in every zone.** It was restricted to the Estate of Unrest while the ladder was
  being proven; that restriction is lifted

### What is different from the ordinary world

Creatures are tougher at every step, and the numbers climb together:

| | health | level | damage | armour, attack and resists |
|---|---|---|---|---|
| **Normal** | — | — | — | — |
| **Nightmare** | ×2 | +2 | ×1.2 | ×1.15 |
| **Hell** | ×3 | +4 | ×1.5 | ×1.3 |
| **Inferno** | ×4 | +6 | ×2.1 | ×1.45 |

Health is the big dial and damage the small one, deliberately — a harder world should cost you time
and resources rather than delete you outright.

On top of the numbers, each difficulty changes the **rules**, and they do **not** stack up as you
climb. Only the numbers do. Each step is a different fight, not a strictly worse one — Hell creatures
can be mezzed where Nightmare's cannot, and that is on purpose.

- **Nightmare — awake and unbindable.** Nothing is fooled by invisibility, sneak, hide or improved
  hide, and nothing can be mezzed, charmed, rooted or snared. Stun and fear still work
- **Hell — champions.** One creature in five carries a permanent affix. It glows and is tagged under
  its name, the way a guild tag sits under a player's, so you always see it before you pull:
  *a gnoll pup* with **&lt;Barbed&gt;** beneath it. Feigning death also becomes unreliable — the feign
  itself always works, but each creature rolls on its own and about **half** are not fooled, so you
  shed part of a camp rather than all of it
- **Inferno — vigilant.** Creatures notice you from twice as far away, horizontally only, so a wider
  radius never lets anything see you through a floor. Feign is unreliable for about **three in four**

The champion affixes:

| tag | what it does |
|---|---|
| **Armored** | double armour class |
| **Hardened** | double health, on top of the difficulty's own |
| **Mending** | regenerates steadily — you cannot chip it and walk away |
| **Frenzied** | attacks about 40 percent faster |
| **Barbed** | reflects damage back at you on every melee hit you land |
| **Draining** | its hits take mana and endurance as well as health |
| **Leeching** | heals itself for a share of the damage it deals |

### Fixed since it launched

- **Hardened champions were not getting their bonus health.** The affix and the difficulty's own
  multiplier were written one after the other, so the second replaced the first
- **Leeching champions healed invisibly.** The leech worked, but the creature's health bar never
  moved. It now visibly heals and tells you when it drains you
- **Champion tags were invisible if the creature spawned before you arrived** — which was almost
  always, since a zone fills before the first player finishes zoning in. Champions now keep their tag
- **Fragile has been removed from the delve mode list.** Half health and double damage cancelled out
  to exactly normal difficulty while paying 1.5 times the score

## Tomes of Insight

A consumable that grants **one extra level-up reward pick without levelling you up** — the same three
choices you get on a level, offered on demand.

- **They only drop in the harder difficulties.** Normal drops none at all
- **Nightmare** drops **Worn** tomes, **Hell** drops **Etched**, **Inferno** drops **Radiant** —
  rarer as you climb, because the higher tiers are the ones that stay useful at the level cap
- Each tier is tied to a level band: **Worn 1-10**, **Etched 11-20**, **Radiant 21-30**
- **You can use a tome from below your band** — it simply draws from the top of its own band. You
  **cannot** use one from above it, and it is not consumed if you try
- **Only creatures worth fighting drop them.** Anything conning light blue or below pays nothing, so
  parking at the top of a band and farming trivial creatures earns you no tomes
- **Decline instead of picking, and your reroll price is cut** — 25, 50 or 100 percent by tier. That
  price otherwise rises for the whole life of your character and is never reset by death, so a tome is
  the only thing in the game that winds it back
- A tome's offer cannot be rerolled for coin. The tome **is** the reroll
- Fixed clicking a tome failing with an error and consuming nothing
- Fixed a tome's description showing the text for an unrelated Shield Wall effect
- **Fixed Hardened champions not getting their bonus health.** The affix and the difficulty's own
  health multiplier were being written one after the other, so the second quietly replaced the first
  instead of stacking with it
- **Fixed Leeching champions healing invisibly.** The leech was working the whole time, but the
  creature's health bar never moved, so it looked like the affix did nothing. It now visibly heals
  and tells you when it drains you
- **Fixed champion tags being invisible if the creature spawned before you arrived.** The tag was
  only ever sent to players already standing in the zone at that moment — and since a zone populates
  before the first player finishes zoning in, that was almost everyone. Champions now carry their tag
  permanently
- **Armored** doubles armor class; **Hardened** doubles health. Worth stating plainly — they were
  easy to confuse, and a Hardened creature was never meant to be harder to hit
- **Fragile has been removed from the delve mode list.** Half health and double damage cancelled out
  to exactly normal difficulty while paying 1.5 times the score — it was free points, and it played
  like Standard with spikier hits

> A reminder from the original notes, because it is the part people expect to work the other way:
> the difficulties do **not** inherit each other's mechanics. Only the numbers climb. Hell creatures
> can be mezzed where Nightmare's cannot, and that is deliberate — each step is a different fight,
> not a strictly worse one.

## Interface

- The Travel window's button is no longer stretched across the whole window and blurry
- Port Group and Auto Confirm now show `[X]` or `[  ]` in their label, so you can tell at a glance whether they are on
