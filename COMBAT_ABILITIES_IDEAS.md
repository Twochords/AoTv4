# Per-class combat abilities — idea list for review

**Status: IDEAS ONLY. Nothing designed in detail, nothing built.**
Warrior, Paladin and Shadowknight are excluded — already planned.

Slots are **level 1 / 5 / 10 / 15 / 20 / 25**.

---

## How many each class actually needs

`skill_pool.NATIVE` is the surviving record of what each class gets natively, and it is very uneven:

| class | already has | new ideas needed |
|---|---|---|
| **Monk** | Dragon Punch, Eagle Strike, Flying Kick, Round Kick, Tiger Claw, Kick, Disarm | **1** (a capstone) |
| **Berserker** | Frenzy, Kick, Disarm | **3** |
| **Rogue** | Backstab, Disarm | **4** |
| **Ranger** | Kick, Disarm | **4** |
| **Cleric** | Bash | **5** |
| **Beastlord** | Kick | **5** |
| Druid, Bard, Shaman, Necromancer, Wizard, Magician, Enchanter | *nothing* | **6 each** |

⚠️ **Bard's emptiness is deliberate, not an oversight.** `skill_pool.NATIVE` notes Bard was given every combat skill by hand on the old Bard-only server, so its `skill_caps` presence was our edit rather than what the class has. Treat Bard as a blank slate.

---

## ⚠️⚠️ These cannot be new SKILLS — pick a vehicle first

The twelve activated specials are **client-side skill ids**; the list is fixed in `eqgame.exe` and a new one cannot be added. So every idea below has to ship as one of:

| vehicle | fits | cost |
|---|---|---|
| **Discipline** (spell row with `EndurCost`) | best — lands in the **Combat Abilities** window, which is where a player looks for these | ⚠️ §20 excludes disciplines from the *reward pool* for exactly this reason; granting them directly at a level is the opposite case and is fine |
| **AA** | activated, already has four trees | ⚠️ must **reuse a native row** (§10) and is bought, not granted at a level |
| **Spell** | simplest to author | ends up in the spellbook, not Combat Abilities — wrong drawer |

**Recommendation: disciplines**, auto-granted at the level. They appear where a player expects, they can carry an endurance cost (which §22 already established as the melee resource), and they need no new client anything.

⚠️ Autoskill (§19) only fires the **ten activated specials** — it will not fire a discipline. If these are meant to be usable by the autoskill window, that is a separate piece of work and should be decided now, not after 60 abilities exist.

---

## Design rules these follow

Taken from what this server has already learned rather than invented:

- **Scale off the user's own maximum, not off damage dealt.** Sanctified Blow's note: a share of your own health is self-normalising, a share of damage compounds across gear, offense and hit count and has to be capped.
- **No raw damage multipliers.** §43's Fragile mode died because a damage number is either invisible on trash or one-shots on real content.
- **Pair abilities so one feeds the other**, like the Paladin cone-cleave shortening the group heal. It rewards sequencing rather than mashing.
- **Flat numbers go stale.** §15's aura rework converted eight flat effects to percentages for exactly this reason.

---

# The ideas

Format: **level — name** — what it does. **Bold pairs** mark two abilities meant to be used together.

## Cleric — 5 needed (has Bash)

- **1 — Censure** — melee strike; heals you for a share of the damage dealt. Gives a Cleric something to do while out of mana.
- **5 — Reproach** — adds hate and reduces the target's melee damage briefly. A pull tool that is not a taunt.
- **10 — Hammer of Faith** ⭐ — frontal cone strike; **refunds mana per target hit**.
- **15 — Martyr's Grasp** ⭐ — for a short time you take a share of your group's incoming damage and are healed for part of what you absorb. Pairs with Hammer: mana in, damage soak out.
- **20 — Absolution** — instant cure of one detrimental effect on a group member, on a short reuse.
- **25 — Sanctified Ground** — a small area where allies take reduced damage while standing in it. Positional, not a buff you forget about.

## Druid — 6 needed

- **1 — Thornlash** — melee strike that stacks a damage shield on yourself, up to a cap.
- **5 — Rooted Stance** — while you do not move, strong regen and reduced damage taken. Ends the moment you step.
- **10 — Wildfire Swipe** ⭐ — cone strike that ignites everything hit.
- **15 — Sunburst** ⭐ — area effect that heals your group for a share of the damage it deals. Better the more Wildfire targets are burning.
- **20 — Nature's Recoil** — the next few hits you take reflect a share back.
- **25 — Verdant Surge** — resets Thornlash and Rooted Stance, and doubles regen briefly.

## Monk — 1 needed (already has six)

- **25 — Stillness** — a capstone: for a few seconds every strike is a critical and you cannot be interrupted, but you cannot move. The Monk's kit is already the richest in the game; it needs a ceiling, not more buttons.

## Bard — 6 needed (blank slate)

- **1 — Discordant Strike** — melee strike that briefly lowers the target's resistance to your next song.
- **5 — Cadence** — your next song lands instantly instead of taking its cast time.
- **10 — Dirge of Ruin** ⭐ — cone strike; every target hit takes increased damage from songs for a short time.
- **15 — Crescendo** ⭐ — the currently playing song hits for double for one pulse. Set up with Dirge.
- **20 — Requiem** — short-duration group damage reduction that scales with your Singing skill.
- **25 — Finale** — ends your current song and converts its remaining duration into an instant burst of the same effect.

## Rogue — 4 needed (has Backstab, Disarm)

- **1 — Shank** — usable from any angle; small damage, refunds endurance. Backstab needs a filler when you cannot get behind.
- **5 — Cutthroat** — bleed that scales with the weapon's damage rather than a flat number.
- **10 — Smoke Step** ⭐ — short reuse, drops all hate on one target.
- **15 — Ambush** ⭐ — the next Backstab within a few seconds cannot be resisted or dodged. Smoke Step into Ambush is the rotation.
- **20 — Pilfer** — strike that has a chance to grant a small amount of coin or a fragment. Thematic, low power.
- **25 — Assassinate** — very long reuse; a single strike that scales with how far below full health the target is.

## Ranger — 4 needed (has Kick, Disarm)

- **1 — Hamstring** — melee strike that snares. Ranger's identity is control at range.
- **5 — Marked Prey** ⭐ — the target takes increased damage from your ranged attacks.
- **10 — Rain of Arrows** ⭐ — a short burst of extra ranged attacks; far stronger on a Marked target.
- **15 — Thicket Step** — instant short-range reposition backwards, breaking melee contact.
- **20 — Hunter's Focus** — for a few seconds your ranged attacks cannot miss.
- **25 — Trueshot Volley** — a long-reuse burst that consumes Marked Prey for a large hit.

## Shaman — 6 needed

- **1 — Spirit Strike** — melee strike that returns mana.
- **5 — Bloodrite** ⭐ — spend a share of your own health to shorten your next spell's cast time.
- **10 — Totemic Slam** — cone strike that slows everything hit slightly. ⚠️ Small and short — §43 records slow as the single most powerful debuff in the game.
- **15 — Ancestral Ward** ⭐ — absorbs damage equal to a share of the health Bloodrite spent. Turns the cost into a resource.
- **20 — Feral Vigour** — group attack speed for a short time, on a long reuse.
- **25 — Spirit Walk** — brief immunity to movement effects and greatly increased run speed.

## Necromancer — 6 needed

- **1 — Grave Touch** — melee strike that returns health and mana in small amounts.
- **5 — Wither** — a stacking damage-over-time applied by melee rather than by casting.
- **10 — Siphon Vitality** ⭐ — converts a share of your pet's health into your mana.
- **15 — Bone Shield** ⭐ — your pet takes a share of the damage aimed at you. Feeds Siphon: the pet is the battery.
- **20 — Dread Presence** — nearby enemies attack more slowly while you stand still.
- **25 — Second Death** — on a long reuse, your pet dying instantly resummons it once.

## Wizard — 6 needed

- **1 — Arcane Lash** — melee strike that adds a charge to your next spell.
- **5 — Mana Shield** — for a short time a share of damage taken is paid from mana instead of health.
- **10 — Concussive Blast** ⭐ — cone knockback that interrupts casting.
- **15 — Overload** ⭐ — your next damage spell deals more damage per charge from Arcane Lash, and the charges are cleared.
- **20 — Displacement** — a short window where the next attack against you misses entirely.
- **25 — Runic Collapse** — a long-reuse strike that consumes all remaining mana for damage proportional to it. ⚠️ Explicitly an all-or-nothing, like Iron Will.

## Magician — 6 needed

- **1 — Elemental Fist** — melee strike whose damage type follows your currently summoned pet.
- **5 — Forge Weapon** — summons a temporary weapon better than your current one, for a duration.
- **10 — Elemental Bond** ⭐ — your pet's next few attacks also hit your target.
- **15 — Sacrifice Servant** ⭐ — destroys your pet for a large heal and a damage burst. Pairs with Bond as the finisher.
- **20 — Shard Barrier** — summons a barrier that absorbs a fixed share of your maximum health.
- **25 — Elemental Fury** — your pet gains greatly increased attack speed for a short time.

## Enchanter — 6 needed

- **1 — Mind Spike** — melee strike that returns mana and is stronger against a mezzed target.
- **5 — Fracture** ⭐ — reduces the target's resistance to your next mesmerise.
- **10 — Reflexive Ward** — the next attack against you is reflected as a stun.
- **15 — Mass Bewilder** ⭐ — cone mesmerise, short duration; far more reliable on a Fractured target.
- **20 — Borrowed Time** — briefly increases your group's attack speed at the cost of your own mana regen.
- **25 — Mindwrack** — a long-reuse strike that deals damage equal to a share of the target's remaining mana.

## Beastlord — 5 needed (has Kick)

- **1 — Maul** — melee strike; your warder strikes the same target immediately after.
- **5 — Pack Tactics** ⭐ — you and your warder both gain attack speed while within melee range of the same target.
- **10 — Feral Swipe** — cone strike that applies a bleed.
- **15 — Spirit Link** ⭐ — you and your warder share incoming damage evenly. Strong with Pack Tactics, dangerous alone.
- **20 — Warder's Fury** — your warder's next attacks are critical hits.
- **25 — Bestial Alignment** — for a short time both you and your warder gain a share of each other's attack power.

## Berserker — 3 needed (has Frenzy, Kick, Disarm)

- **10 — Reckless Abandon** ⭐ — you take increased damage and deal more, for a short time.
- **20 — Blood Frenzy** ⭐ — Frenzy's reuse is shortened by each hit you take while Reckless Abandon is up. The Paladin pattern, applied to the class that should be rewarded for standing in it.
- **25 — Decapitate** — a long-reuse strike with a chance to instantly kill a non-named target below a health threshold.

---

## Open questions before any of this gets built

1. **Vehicle** — disciplines, AAs or spells? (recommendation above)
2. **Should autoskill fire them?** It currently handles only the ten specials.
3. **Endurance cost** — §22 charges specials a share of the damage dealt. Do these follow that, or carry authored costs?
4. **How are they granted?** A level hook like `grant_native_combat_skills`, or scribed rewards?
5. **Do they survive the roguelite death?** They are class identity rather than earned loot, so re-granting on the way back up seems right — which is how skills already behave after the auto-level fix.
