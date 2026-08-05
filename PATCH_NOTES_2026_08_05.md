# AoTv4 Patch Notes — 5 August 2026

## Buffs

- **Buffs are permanent — for as long as you keep the spell memorised.** A buff you cast on yourself
  no longer expires. The catch is the leash: **the moment that spell leaves your spell gems, the buff
  drops.** So you can carry as many permanent buffs as you have gems, and swapping a gem for something
  else is what costs you the buff. Live EQ's durations assume you re-buff constantly; that is most of
  what a run was.
- **Buffing someone else is long, not permanent.** A buff you cast on another player runs **12 minutes
  at level 1**, rising to **99 minutes** at the level cap. Group buffing works exactly as before — it
  just is not eternal, so a group cannot hand each other permanent buffs and sidestep the gem limit.
- **Buffs with charges still run out.** Anything spent by use rather than by time keeps its old
  duration and its charges, so it goes away when used up instead of sitting on you forever.
- **Bard songs are permanent too.** Most beneficial songs ran **12 seconds**, because on live a Bard
  twists them continuously. They now last like any other buff.
  - Songs still **pulse** to group members in range every 6 seconds — that is unchanged.
- Deliberately **not** made permanent: **invulnerability effects, true heal-over-time spells and death
  saves** — a permanent invulnerability is not a buff, it is an unkillable character, and a permanent
  heal-over-time is infinite healing. Bard heal songs (the Cantata and Chorus lines) are excluded for
  the same reason, though the regen songs like Hymn of Restoration are included. **Disciplines** are
  unchanged — a permanent Trueshot is not a buff either. NPC self-buffs, procs and item clicks are
  untouched, so monsters do not keep their buffs forever.

## Spells

- **Bard songs finally work for every class.** The reward pool hands out genuine bard songs to all
  sixteen classes, but only an actual Bard entered "song mode" — so for everyone else a song was sung
  **once**, faded almost immediately, and was **interrupted by moving**. Song behaviour now follows the
  *song*, not the singer's class.
  - A Bard is unaffected. Reward spells that merely look like songs are unaffected.
  - Note: the instrument bonus is still Bard-only, so a non-Bard sings at baseline power.

## Combat

- **Monsters no longer enrage.** Enraged creatures riposted every frontal melee attack, which meant
  backing off and waiting it out. Your own **pets still enrage** — that one is in your favour.
- **Bows have no minimum range.** You had to be **25 units away** to fire, so an archer had to back
  out of melee to use their own weapon. You can now shoot at any range, point blank included. Throwing
  weapons are unchanged, and monsters that shoot at you still keep their old minimum.
- **Creatures recover when they lose you.** Anything that drops out of combat — you ran, you feigned,
  you zoned out, or it simply lost interest — returns to **full health and mana**. Damage no longer
  banks across attempts, so a fight cannot be chipped away over several pulls, and you will never
  arrive to find something already softened by somebody else. **Your own pets and charmed creatures
  are exempt** and keep their health as before.

## Fixed

- **Stuck at 0 health.** Two separate causes, both of which left you alive on the server but pinned at
  0 on your screen, unable to regenerate or zone:
  - **Dying somewhere you are bound to** — which, since everyone binds at the hub, was most deaths.
    You were revived but never told you were alive again.
  - **Changing class** — your health was saved against your *old* class's maximum, so you could come
    back above your new maximum and bleed down from an impossible number.
  - If you reported "I took my charm off and dropped to 0" — the charm was never the cause. It just
    made an already-broken state visible.
- **Spell damage and healing were being doubled if you had gear.** An item bonus that was meant to
  contribute nothing was instead adding your spell's full base amount a second time — so any character
  wearing gear with spell damage or heal amount (which is most Hallowed and Mythic pieces) dealt and
  healed **twice** what the spell said. Spells now do what they say.
  - ⚠️ This is a **significant reduction** to nukes and heals if you are geared. Nothing was nerfed —
    the numbers on your spells are simply real now. It is most obvious on small procs: the Skin of the
    Reptile line was healing 40 on a 20 point proc, which is how it was found.
- **Mythic weapons now hit harder than Hallowed.** Both tiers were being given the same weapon damage,
  so a Mythic weapon looked like no upgrade at all. Hallowed is **1.5x** native damage and Mythic is
  **2x**. Every other stat was already correct — this only ever affected weapon damage.
- **The Refining Crucible no longer destroys your augments.** Refining destroys the items you put in,
  and anything socketed into them went with it, silently. The crucible now **refuses the combine** and
  tells you which item still has an augment in it. Remove your augments first and refine as normal.
- **Alternate Advancement abilities that grant health now actually grant it.** Several AAs gave health
  as a *percentage* while giving mana and endurance as flat amounts. At our level cap a few percent is
  almost nothing, so they read as broken — most visibly **Bulwark Within**. Those now give a flat
  amount, in proportion to what they were worth.
