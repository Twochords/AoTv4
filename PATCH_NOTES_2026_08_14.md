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

## World difficulties — Nightmare, Hell and Inferno

The four-difficulty ladder announced on 10 August has now been played rather than just built, and
several things it promised were not actually happening. The full write-up of what each difficulty
does is in those notes; this is what has changed since.

- **Difficulties now work in every zone.** They were restricted to the Estate of Unrest while the
  ladder was being proven; that restriction is lifted
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
