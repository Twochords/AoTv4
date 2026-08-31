# AoTv4 Patch Notes — 30-31 August 2026

## Titanwrought crafting

- **Gear is now crafted, not looted.** A **mold** plus a base component, a binding and a bought temper
  makes one piece of Titanwrought armour or jewelry. 210 molds, 48 materials and 255 recipes.
- **Combines never fail.** Your tradeskill skill decides how *good* the result is, not whether you get
  one: higher skill rolls a better tier, up to guaranteed Mythic at 345 effective skill.
- The **mold is consumed** by the combine, and molds do **not** survive death.
- **Titanwrought gear no longer drops.** Until somebody crafts it, none enters the world.
- The old **Defiant** line has been renamed **Titanwrought** — 1,117 items. It covers the armour
  slots; the new crafted set fills in the jewelry and accessories, so together they are one set.
- Molds are now named for what they make — *Rough Titanwrought Cloth Cap Mold* rather than *Rough
  Cloth Cap Mold* — so one search finds the mold and the recipe that consumes it.
- **Fixed: crafted Titanwrought had no Hallowed or Mythic versions at all.** The tier rows were never
  generated, so every combine produced the base tier no matter your skill, and the entire
  skill-decides-the-tier mechanic did nothing. It works now.

## Raids

- **Three raid encounters, in private instances.** Phinigel Autropos and Mayong Mistmoore out of
  Kelethin; Velketor the Sorcerer waits behind the Thurgadin region lock.
- Entering creates a **real expedition** — it appears in your expedition window with a safe return,
  and your whole group is brought in together.
- **One lockout per character per encounter, 24 hours, taken when you enter.** Entering spends your
  attempt, so a wipe costs the same as a win.
- **Each boss now has mechanics**, telegraphed before they land:
  - **Phinigel** calls a swarm of piranha at 75%, 50% and 25%.
  - **Mayong** never casts — he speeds up at 66% and 33% and does not slow down again.
  - **Velketor** wakes two crystal statues at 75%, 50% and 25%.
- All three summon and are immune to mesmerise, charm, fear and fleeing. **Slow still lands** on every
  one of them.
- **Loot is exactly three items per person**: two Rough Titanwrought molds, plus one from the boss's
  spoils — a signature piece, zone gear, a Tome of Insight, a weapon augment or a tier-3 augment.
- **Nine new signature items**, three per boss, at the level cap.
- Raid weapons can no longer out-perform a Mythic Titanwrought weapon at any tier.
- Velketor no longer drops spell scrolls.
- **Fixed: Phinigel could not be entered at all.** The encounter pointed at the wrong zone, so the
  game built the instance in one place and sent you to another, then put you back where you stood.
- **Fixed: killing a raid boss granted no loot rights and no molds.** Every kill, since the feature
  shipped.
- **Fixed: Mayong stopped attacking partway through the fight.**
- **Fixed: Velketor's statues stood still instead of attacking.**
- **Fixed: leaving a raid could strand you** with no way out. Exit now works from inside a raid
  whatever else has gone wrong, and returns you to your bind point if it cannot find where you came
  from.

## Delves

- The reward chest now has a **10% chance** of a mold instead of always giving one. Named mobs remain
  the other route at 25%.
- **Raids have moved off the Layers list onto their own tab**, with tier, region and lockout status.
  The Layers tab is now called **Dungeons**. *(Requires the client update below.)*

## Items and gear

- **Every item in the game has been rescaled.** Recommended levels and stats were re-derived from how
  an item is actually obtained, so gear is priced for this server rather than for level 65+ content.
  The Robe of the Kedge, for example, went from recommended level 65 to 20.
- **Augment sockets on crafted gear are now three different types.** They were all the same type,
  which the client renders badly — one socket vanished entirely and the rest drew out of order.
- Every augment that fitted before still fits all three sockets, and around **2,200 more** stock
  augments are now usable in crafted gear.
- Mythic gear is no longer occasionally weaker than the plain version it was made from.

## Alternate advancement

- **Bloodletting did nothing above rank 1.** Ranks 2 through 5 were never created, so every point past
  the first was wasted. Fixed.
- **Borrowed Breath had the same fault** — ranks 2 to 5 are now real.
- **Bracing** and **Refuse to Fall** have been retired. Both defended against things that no longer
  happen on this server, and neither did anything at all.
- **26 AA ranks across 9 abilities required a level above the cap** and could never be trained,
  including most of the tradeskill masteries handed out by achievements. They are trainable now.

## Skills

- **Fifteen skills were being silently reset to zero** on login for any class with no cap entry —
  Meditate for the melee classes, and Block, Dodge, Parry, Riposte, Double Attack and others besides.
- Percussion Instruments and the other instrument skills no longer get stuck at 1.

## Combat

- **Weapon procs are now on a timer rather than a chance per swing**: roughly every 15 seconds for
  Mythic, 20 for Hallowed, 25 for a native weapon. A Mythic weapon was procing on three swings in four.

## Fixes

- **Kindred Bond's ward now refreshes on you and your group**, not only on the pet. Buying or ranking
  it up with a pet already out now works, and group members who join later pick it up.
- Spell descriptions no longer come back blank in the spell window or Allaclone.

---

## Client update

**Required for the Raids tab.** Copy the updated `EQUI_AoTDungeonWnd.xml` into `<EQ>\uifiles\default\`
and take the new `dinput8.dll`. Without both, raids will not appear anywhere.

Also worth taking if you have not already: `spells_us.txt`, `dbstr_us.txt` and
`Resources\GlobalLoad_chr.txt`. Without the last one, delve bosses, travel waypoints and fellowship
campfires all draw as a generic placeholder model.
