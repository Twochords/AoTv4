# AoTv4 Patch Notes — 4 August 2026

## Tradeskills

Tradeskills now matter. Skill decides what you make, and crafting has a reward ladder.

- **Crafted gear has augment sockets.** Native gear has **1**, Hallowed **2**, Mythic **3**. Any
  Delve augment fits any socket. Ornamentation slots are unchanged — nothing lost.
- **Combines no longer always produce Mythic.** The tier is now rolled from your tradeskill skill:
  around 100 skill you will see the occasional Hallowed; by 300 most combines come out Mythic. At the
  very top, with both bonus items below, effectively everything does.
- **Mastery AAs are granted automatically** at **50 / 100 / 150** skill, for each of the ten
  tradeskills that has one.
- **Salvage** is granted by the new **Master Artisan** achievements — reach 50, 100, 150, 200, 250 and
  300 in *every* mastery tradeskill for one rank each.
- **Two new bonus items per tradeskill**, both from achievements you can see in advance:
  - at **100 skill** — a wearable tool, **+20** to that tradeskill
  - at **200 skill** — a mask that casts an illusion, **+30** to that tradeskill
  - They **stack**, for +50 while you work.
  - Both are **kept through death** and are **No Drop** — they are yours once earned.

Fishing and Research have no Mastery AA in the game, so they are not part of the AA ladders — they
still earn their titles, and they still get their tool and mask.

## The Delve

- **Wardens are far tougher.** A warden is now reliably **7.5x** the health of a regular creature in
  that delve, at every rung — previously this drifted and was weakest at the highest rungs.
- **Delve experience is normalised.** Delve creatures were paying roughly **3.7x** what an equivalent
  open-world creature paid, which is why delving out-levelled its own content. A delve kill is now
  worth about what an open-world kill is worth; the delve's rewards come from its score, the Delver's
  Sigil, augments and coin instead.
- **Wardens have a class.** Every warden rolls one of the sixteen classes and fights like it, with
  class- and level-appropriate spells. Its title tells you what you are facing before it opens up.
- **Fixed:** a warden left alone for ten seconds was quietly reduced to an ordinary creature. Wardens
  now keep their strength.
- **You can no longer enter a delve while something is fighting you.** Entering was a free escape from
  any losing fight.

## Combat

- **Combat special abilities cost less endurance** — the cost is now a third of the damage dealt,
  down from a half. This lines up with the Sinew line, which returns a third of its damage as
  endurance.
- **Autoskill's four-ability limit is now actually enforced.** It could previously be exceeded two
  ways: abilities stayed switched on across a death and came back already enabled, and the
  `#autoskill` command skipped the check entirely. A newly earned ability now always starts **off**.
- **Mend and Feign Death** no longer appear for classes that do not get them. Monks keep both.
  Necromancers and Shadow Knights are unaffected — they get Feign Death as a spell, not a skill.

## Spells and Rewards

- **Parchment Fragments now pay correctly on death.** You are paid for **every** spell destroyed. The
  old payout only counted spells that happened to have a rank line, so most deaths paid a small
  fraction of what they should have.
- **Ink of the Lost is no longer duplicated.** Looting it gave you both the currency and the item.
- **Spell lines no longer cancel each other.** Skin of the Reptile, Lingering Sloth, Kindred and the
  Thirst line were overwriting one another. Each now stacks independently, and higher tiers still
  replace lower ones within their own line.

## Quality of Life

- **Your starting weapon now matches your class.** Everyone used to receive a short sword, including
  classes with no skill in it. Rogues now start with a dagger — Backstab requires a piercing weapon.
  Changing class also swaps the weapon, which it previously did not.
