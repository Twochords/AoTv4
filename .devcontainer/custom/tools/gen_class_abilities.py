#!/usr/bin/env python3
"""
Generate the AoTv4 class-ability disciplines: one migration submission per class, plus the
matching Lua stubs.

⚠️⚠️ THIS IS THE SINGLE SOURCE OF TRUTH FOR THE 44700-44755 BAND. Hand-editing a generated .sql
or a generated stub means the next run silently reverts it. Warrior (class 1) is deliberately
ABSENT: it shipped as migration v104 before this generator existed, and regenerating it would
renumber a migration that is already applied.

⚠️ Ids are arithmetic, NOT a lookup: 44700 + (class - 1) * 3 + (tier - 1). Helper/trigger spells
live at 44750+ and are never offered by the reward pool (the pool stops at 44599).

Reference values read out of the database rather than invented:
  melee discipline   spellanim 0    CastingAnim 44   (104 of the 111 stock discs use anim 0 or 1)
  heal               spellanim 278  CastingAnim 43   (13 Complete Heal)
  nuke               spellanim 202  CastingAnim 44   (466 Lightning Shock)
  debuff             spellanim 113  CastingAnim 44   (178 Color Skew)
  rune               spellanim 86   CastingAnim 42   (481 Rune I)
  pet                spellanim 306  CastingAnim 43   (285 Pendril's Animation)
  swarm              spellanim 80   CastingAnim 43   (3265 Servant of Ro)

⚠️ NO APOSTROPHES AND NO PERCENT SIGNS anywhere in a name or description. The migration validator
refuses both: an apostrophe aborts the SQL mid-run (section 51) and a literal percent is eaten as a
printf format token by the description path (section 14).
"""
import os, textwrap

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))   # /src/.devcontainer
SRC  = os.path.dirname(ROOT)
SUBS = os.path.join(ROOT, "custom", "migrations", "submissions")
LUA  = os.path.join(ROOT, "repo", "quests", "global", "spells")

# ---------------------------------------------------------------------------- presentation presets
MELEE = (0,   44)
HEAL  = (278, 43)
NUKE  = (202, 44)
DEBUF = (113, 44)
RUNE  = (86,  42)
PET   = (306, 43)
SWARM = (80,  43)

# ---------------------------------------------------------------------------- templates to clone
T_INSTANT = 4667   # Rebuke of the Ikaav  -- instant, single target, no buff
T_BUFF    = 4499   # Defensive Discipline -- self buff with a real duration
T_PET     = 285    # Pendril's Animation  -- pet summon
T_SWARM   = 3265   # Servant of Ro        -- swarm pet

INERT = []                       # no real effects; the whole ability is paid in Lua
CLASS_NAME = {2:"Cleric", 3:"Paladin", 4:"Ranger", 5:"Shadowknight", 6:"Druid", 7:"Monk", 8:"Bard",
              9:"Rogue", 10:"Shaman", 11:"Necromancer", 12:"Wizard", 13:"Magician",
              14:"Enchanter", 15:"Beastlord", 16:"Berserker"}
TIER_LEVEL = {1: 1, 2: 5, 3: 10}
TIER_TIMER = {1: 5, 2: 6, 3: 2}          # EndurTimerIndex -- see CLASS_ABILITIES_DESIGN.md section 1
TIER_RECAST = {1: 10000, 2: 120000, 3: 15000}

def A(cls, tier, name, icon, look, tt, good, effects, dur=0, durf=0,
      numhits=0, numhitstype=0, tmpl=T_INSTANT, tz=None, desc="", rid=None):
    """One spell row. `effects` is a list of (spa, base, limit, formula, max), max 12."""
    return dict(cls=cls, tier=tier, id=rid if rid else 44700 + (cls - 1) * 3 + (tier - 1),
                name=name, icon=icon, look=look, tt=tt, good=good, effects=effects,
                dur=dur, durf=durf, numhits=numhits, numhitstype=numhitstype,
                tmpl=tmpl, tz=tz, desc=desc)

def H(cls, rid, name, icon, look, tt, good, effects, dur=0, durf=0, tmpl=T_INSTANT, tz=None, desc=""):
    """A helper/trigger row: never trained, never offered, only ever cast BY something else."""
    d = A(cls, 0, name, icon, look, tt, good, effects, dur, durf, tmpl=tmpl, tz=tz, desc=desc, rid=rid)
    d["helper"] = True
    return d

ROWS = []

# ============================================================================= 2 CLERIC (mana)
ROWS += [
 A(2,1,"Templar Strike", 88, MELEE, 5, 0, INERT, desc=
   "Strike your target with your weapon and draw a measure of vigour back into yourself."),
 A(2,2,"Sanctuary", 99, HEAL, 41, 1,
   [(147,17,0,100,0), (289,44750,0,100,0)], dur=3, durf=11, tmpl=T_BUFF, desc=
   "Shelter your group. Each member is healed for a sixth of their own maximum health at once, "
   "and again when the shelter lifts three ticks later."),
 A(2,3,"Condemn", 26, NUKE, 5, 0, [(0,-20,0,4,250)], desc=
   "Call down judgement. Shortens Sanctuary by 5 seconds, or by 10 against the undead."),
 H(2,44750,"Sanctuary Bloom", 99, HEAL, 6, 1, [(147,17,0,100,0)], desc=
   "The delayed half of Sanctuary."),
]

# ============================================================================= 3 PALADIN (endurance)
# ⚠️⚠️ THESE REPLACE THE THREE ACTIVATED AAs (abilities 45/55/79, hosted on native rows, casting
# 44600-44602). Paladin was the last class still on a different mechanism, which meant one class
# found its kit in the AA window while the other fifteen found theirs in Combat Abilities. The AA
# versions are retired by the companion migration 2026_08_21_paladin_retire_aas.sql -- run BOTH or a
# Paladin ends up holding each ability twice.
# 📌 Behaviour is ported verbatim from lua_modules/aotv4_paladin.lua; only the delivery changed.
ROWS += [
 A(3,1,"Ardent Strike", 94, MELEE, 5, 0, INERT, desc=
   "A weapon blow carried by conviction rather than force, and it draws the eye of what you strike."),
 A(3,2,"Hand of Conviction", 95, HEAL, 6, 1, INERT, desc=
   "Spend your own vitality to mend your group. Each member is healed for a quarter of YOUR maximum "
   "health, which is why a Paladin who builds health heals harder."),
 A(3,3,"Divine Reproach", 96, MELEE, 5, 0, [(21,1,0,100,0)], desc=
   "A rebuke that halts your target briefly and fixes its attention on you. Shortens Hand of "
   "Conviction by 5 seconds."),
]

# ============================================================================= 4 RANGER (endurance)
ROWS += [
 A(4,1,"Twin Slash", 91, MELEE, 5, 0, INERT, desc=
   "Two swings in the time of one."),
 A(4,2,"Volley", 92, MELEE, 5, 0, INERT, desc=
   "Loose a volley at everything in front of you."),
 A(4,3,"Point Blank Shot", 93, MELEE, 5, 0, INERT, desc=
   "Fire at a target already in your face. Shortens Volley by 5 seconds, or by 10 with a bow drawn."),
]

# ============================================================================= 5 SHADOWKNIGHT (endurance)
ROWS += [
 A(5,1,"Reaving Strike", 47, MELEE, 5, 0, INERT, desc=
   "A cruel swing that draws the life out of what it opens."),
 A(5,2,"Harrowing", 46, NUKE, 5, 0, INERT, desc=
   "Tear the life from everything around you and take it for your own."),
 A(5,3,"Reaving Vow", 48, MELEE, 5, 0, INERT, desc=
   "Swear the next wound to yourself: your next Reaving Strike drains twice as deeply. "
   "Shortens Harrowing by 6 seconds."),
 H(5,44753,"Reaving Fervor", 48, MELEE, 6, 1, INERT, dur=3, durf=11, tmpl=T_BUFF, desc=
   "Your next Reaving Strike drains twice as deeply."),
]

# ============================================================================= 6 DRUID (mana)
ROWS += [
 A(6,1,"Thorned Strike", 25, DEBUF, 5, 0, [(121,-8,0,100,0)], dur=5, durf=11, tmpl=T_BUFF, desc=
   "Drive thorns into your target. Every melee blow it lands wounds it in turn."),
 A(6,2,"Wildgrowth", 30, HEAL, 5, 1, [(0,12,0,1,0)], dur=10, durf=11, tmpl=T_BUFF, desc=
   "Growth closes wounds faster than they open, for a while."),
 A(6,3,"Sunflare", 31, NUKE, 5, 0, [(0,-25,0,4,300)], desc=
   "A lance of light, brighter against something that cannot move. Shortens Wildgrowth by "
   "5 seconds, or by 10 against a rooted or snared target."),
]

# ============================================================================= 7 MONK (endurance)
ROWS += [
 A(7,1,"Iron Palm", 51, MELEE, 5, 0, INERT, desc=
   "An open hand driven through the guard, weighted by the pace of your weapon."),
 A(7,2,"Void Stance", 52, RUNE, 6, 1, [(172,50,0,100,0)], dur=10, durf=11,
   numhits=4, numhitstype=1, tmpl=T_BUFF, desc=
   "Stand in the space between blows. The next 4 melee attacks against you are far likelier to "
   "find nothing there."),
 A(7,3,"Pressure Point", 53, MELEE, 5, 0, INERT, desc=
   "One strike, placed where it is felt. Shortens Void Stance by 6 seconds."),
]

# ============================================================================= 8 BARD (endurance)
ROWS += [
 A(8,1,"Discordant Strike", 60, DEBUF, 5, 0, [(1,-25,0,100,0)], dur=5, durf=11, tmpl=T_BUFF, desc=
   "A note struck wrong. Your target guards itself worse for it."),
 A(8,2,"Crescendo", 61, HEAL, 41, 1, [(189,100,0,3,0)], desc=
   "The music carries your group. Their second wind arrives early."),
 A(8,3,"Cadence Strike", 62, DEBUF, 5, 0, [(197,10,-1,100,0)], dur=5, durf=11,
   numhits=5, numhitstype=6, tmpl=T_BUFF, desc=
   "Set the beat of the fight. The next 5 blows landed on your target hurt it more. "
   "Shortens Crescendo by 5 seconds."),
]

# ============================================================================= 9 ROGUE (endurance)
ROWS += [
 A(9,1,"Vital Strike", 71, DEBUF, 5, 0, [(3,-35,0,100,0)], dur=4, durf=11, tmpl=T_BUFF, desc=
   "Open a leg. What you have cut cannot run."),
 A(9,2,"Rupture", 72, DEBUF, 5, 0, [(0,-18,0,1,0)], dur=8, durf=11, tmpl=T_BUFF, desc=
   "A wound that will not close on its own."),
 A(9,3,"Exploit Weakness", 73, MELEE, 5, 0, INERT, desc=
   "Put a blade where it is already bleeding, for half as much again. Shortens Rupture by 5 seconds."),
]

# ============================================================================= 10 SHAMAN (mana)
ROWS += [
 A(10,1,"Spiritual Foresight", 77, RUNE, 5, 1,
   [(55,60,0,3,0), (323,44751,0,100,0)], dur=10, durf=11, tmpl=T_BUFF, desc=
   "A ward that sees the blow coming. What strikes through it is slowed for its trouble."),
 A(10,2,"Crippling Spirit", 78, RUNE, 41, 1,
   [(55,120,0,5,0), (323,44752,0,100,0)], dur=10, durf=11, tmpl=T_BUFF, desc=
   "The same ward, over your whole group, and it bites harder."),
 A(10,3,"Malaise", 79, DEBUF, 5, 0,
   [(4,-20,0,100,0), (8,-20,0,100,0), (10,-20,0,100,0), (2,-25,0,100,0)], dur=6, durf=11,
   tmpl=T_BUFF, desc=
   "Sap what your target swings, thinks and commands with. Shortens Crippling Spirit by 5 seconds, "
   "or by 10 against something already slowed."),
 H(10,44751,"Foresight Chill", 77, DEBUF, 5, 0, [(11,85,0,100,0)], dur=2, durf=11, tmpl=T_BUFF, desc=
   "The cold left behind by a warded blow."),
 H(10,44752,"Crippling Chill", 78, DEBUF, 5, 0, [(11,70,0,100,0)], dur=2, durf=11, tmpl=T_BUFF, desc=
   "The cold left behind by a warded blow."),
]

# ============================================================================= 11 NECROMANCER (health)
ROWS += [
 A(11,1,"Withering Touch", 41, DEBUF, 5, 0, [(0,-10,0,1,0)], dur=6, durf=11, tmpl=T_BUFF, desc=
   "Your hand leaves rot behind it."),
 A(11,2,"Soul Harvest", 42, NUKE, 5, 0, INERT, desc=
   "Call in every debt at once. Each affliction on your target is spent for damage and health."),
 A(11,3,"Toll of the Dead", 43, NUKE, 5, 0, INERT, desc=
   "Name the price of what is already killing your target. Shortens Soul Harvest by 15 seconds."),
]

# ============================================================================= 12 WIZARD (mana)
ROWS += [
 A(12,1,"Arcane Fist", 162, MELEE, 5, 0, INERT, desc=
   "Strike with the hand that is not casting, and take a little power back from the impact."),
 A(12,2,"Overload", 163, NUKE, 5, 0, [(0,-40,0,20,900)], desc=
   "Pour everything into one cast. The recoil leaves you reeling for a moment."),
 A(12,3,"Ley Tap", 164, NUKE, 5, 0, [(0,-15,0,3,200)], desc=
   "Draw off a thread of the ley. Each thread makes your next Overload land harder, up to three. "
   "Shortens Overload by 5 seconds."),
]

# ============================================================================= 13 MAGICIAN (mana)
ROWS += [
 A(13,1,"Elemental Fist", 38, MELEE, 5, 0, INERT, desc=
   "A fist wrapped in flame, and a guardian at your shoulder if you have none."),
 A(13,2,"Elemental Swarm", 105, SWARM, 6, 1, [(152,1,0,100,0)], tmpl=T_SWARM, tz="ServantRo", desc=
   "Call a brief host of servants to fight beside your guardian."),
 A(13,3,"Cinder Blast", 106, NUKE, 5, 0, [(0,-30,0,3,250)], desc=
   "A burst of cinders, twice as fierce on whatever your pet is already fighting. "
   "Shortens Elemental Swarm by 7 seconds."),
 H(13,44754,"Elemental Guardian", 38, PET, 6, 1, [(33,1,0,100,0)], tmpl=T_PET, tz="SumEarthR2", desc=
   "The guardian granted by Elemental Fist."),
]

# ============================================================================= 14 ENCHANTER (mana)
ROWS += [
 A(14,1,"Tashania", 21, DEBUF, 5, 0, [(111,-15,0,100,0)], dur=8, durf=11, tmpl=T_BUFF, desc=
   "Thin every ward your target has against magic."),
 A(14,2,"Gift of Thought", 22, HEAL, 41, 1, [(15,100,0,3,0)], desc=
   "Hand your group back the thoughts they have spent."),
 A(14,3,"Mind Fray", 23, DEBUF, 6, 1, [(286,30,0,100,0)], dur=3, durf=11, tmpl=T_BUFF, desc=
   "Fray your own mind against the working. Your spells land harder for three ticks. "
   "Shortens Gift of Thought by 5 seconds."),
]

# ============================================================================= 15 BEASTLORD (endurance)
ROWS += [
 A(15,1,"Feral Swipe", 108, MELEE, 5, 0, INERT, desc=
   "A raking blow, and a companion at your side if you have none."),
 A(15,2,"Feral Frenzy", 109, RUNE, 6, 1, [(11,125,0,100,0), (0,10,0,1,0)], dur=10, durf=11,
   tmpl=T_BUFF, desc=
   "You and your companion both move faster and mend faster."),
 A(15,3,"Bloodscent", 110, MELEE, 5, 0, INERT, desc=
   "You can smell the end of it. Far worse for a target below half health, and it shortens "
   "Feral Frenzy by 5 seconds, or by 10 on wounded prey."),
 H(15,44755,"Feral Companion", 108, PET, 6, 1, [(33,1,0,100,0)], tmpl=T_PET, tz="SpiritWolf224", desc=
   "The companion granted by Feral Swipe."),
]

# ============================================================================= 16 BERSERKER (endurance)
ROWS += [
 A(16,1,"Reckless Cleave", 55, MELEE, 5, 0, INERT, desc=
   "Everything behind the swing and nothing behind the guard. It costs you a little blood."),
 A(16,2,"Frenzied Onslaught", 56, MELEE, 5, 0, INERT, desc=
   "Five swings, as fast as you can put them in."),
 A(16,3,"Blood Frenzy", 57, MELEE, 5, 0, INERT, desc=
   "The worse your own wounds, the sooner you can do that again. Shortens Frenzied Onslaught by "
   "2 seconds for every tenth of your health that is gone."),
]


# ---------------------------------------------------------------------------------- melee range
# ⚠️⚠️ EVERY ROW SHIPPED AT `range = 200`, WHICH MADE THE SWINGS LONG-RANGE. The value was copied
# from the stock disciplines, where it is harmless because those are self buffs -- but a TARGETED
# ability is range-checked against it in Mob::CastSpell (spells.cpp:2600, plus a target-size mod), so
# Ardent Strike could be swung from 200 units away. Reported from play.
# 📌 THE LIST IS "WHICH PAYLOADS ACTUALLY SWING", derived from the module rather than from the
# presentation preset -- several swings wear a DEBUF or NUKE look because they also apply a real SPA
# (Thorned Strike, Discordant Strike, Withering Touch). Keep it in step with the payloads that call
# M.weapon_blow; a mismatch here is silent in both directions.
# ⚠️ Includes Warrior 44700/44702, which the generator does not otherwise emit.
MELEE_RANGE = 25
SWINGS = [44700, 44702, 44703, 44706, 44708, 44709, 44710, 44711, 44712, 44713, 44714, 44715,
          44718, 44720, 44721, 44723, 44724, 44726, 44730, 44733, 44736, 44742, 44744, 44745,
          44746, 44747]

# ---------------------------------------------------------------------------------- messages
# ⚠️⚠️ A CLONE INHERITS ITS TEMPLATE'S MESSAGES TOO, AND THAT IS HOW EVERY ONE OF THESE SHIPPED
# SAYING "You are hit by an invisible force." or "You assume a defensive fighting style." -- a group
# heal announcing itself as a defensive stance. Same class of bug as the icons, and as descnum in
# v56: the columns that hurt are the ones with no obvious connection to what you changed.
# ⚠️ A SWING IS DELIBERATELY BLANK. The damage message already says what happened, and a second line
# on every hit is chat noise.
# 📌 These are emitted as their OWN migration rather than inside each class's, because the fourteen
# class migrations were already merged and applied -- rewriting them would drift from the manifest.
MESSAGES = {
 44701: ("You set yourself behind your guard.", " sets himself behind his guard.", "Your guard drops."),
 44707: ("Conviction mends you.", " is mended by conviction.", ""),
 44708: ("You are rebuked.", " is rebuked.", "The rebuke passes."),
 44704: ("You are sheltered.", " is sheltered.", "The shelter lifts."),
 44715: ("Thorns tear at you.", " is wreathed in thorns.", "The thorns wither."),
 44716: ("Growth closes your wounds.", " is wreathed in growth.", "The growth fades."),
 44719: ("You stand in the space between blows.", " stands very still.", "Your stance breaks."),
 44721: ("Your guard falters.", " guards himself worse.", "Your guard steadies."),
 44722: ("Your second wind arrives early.", " catches a second wind.", ""),
 44723: ("You are struck on the beat.", " is struck on the beat.", "The beat is lost."),
 44724: ("Your leg gives under you.", " staggers.", "Your leg steadies."),
 44725: ("You are bleeding badly.", " is bleeding badly.", "The bleeding stops."),
 44727: ("A ward settles over you.", " is warded.", "The ward gutters out."),
 44728: ("A crippling ward settles over you.", " is warded.", "The ward gutters out."),
 44729: ("Your strength deserts you.", " sags.", "Your strength returns."),
 44730: ("Rot spreads through you.", " begins to rot.", "The rot clears."),
 44737: ("Servants answer the call.", " calls servants.", ""),
 44739: ("Your wards against magic thin.", " is stripped of wards.", "Your wards close again."),
 44740: ("Your thoughts come back to you.", " is given back his thoughts.", ""),
 44741: ("You fray your mind against the working.", " frays his own mind.", "Your mind settles."),
 44743: ("You move faster and mend faster.", " turns feral.", "The frenzy passes."),
 44750: ("The shelter blooms.", " is healed by the shelter.", ""),
 44751: ("The cold slows you.", " slows.", "The cold passes."),
 44752: ("The cold slows you.", " slows.", "The cold passes."),
 44753: ("You swear the next wound to yourself.", " swears a vow.", "The vow lapses."),
 44754: ("A guardian answers.", " calls a guardian.", ""),
 44755: ("A companion answers.", " calls a companion.", ""),
}

MSG_HEADER = """-- @aotv4-migration
-- description: 2026_08_21_class_ability_messages
-- check: SELECT `cast_on_you` FROM `spells_new` WHERE `id` = 44701
-- condition: missing
-- match: You set yourself behind your guard.
-- ⚠️⚠️ KEYED ON A MESSAGE THIS MIGRATION WRITES, NOT ON ONE IT BLANKS. The first version keyed on
-- 44700, which is a swing and is deliberately set EMPTY -- and an empty result reads back as the
-- column name rather than as an empty string, so `not_empty` was true forever and world would have
-- re-run this on every boot. The validator caught it. Test for the thing the SQL CREATES.
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: GENERATED by custom/tools/gen_class_abilities.py -- edit the spec there, never this file.
-- notes: Every class-ability row was cloned from a stock spell and inherited that spell's cast
-- notes: messages, so Sanctuary announced itself as a defensive fighting style and every swing
-- notes: printed "You are hit by an invisible force." A swing is blanked on purpose: its damage
-- notes: message already says what happened.

"""

# ---------------------------------------------------------------------------------- SQL emission
def effect_sql(effects):
    """Set ALL TWELVE slots explicitly. spells_new has effectid1..12, not 3 -- leaving the tail
    inherited is how a clone quietly keeps an effect nobody wrote (the section 5 clone trap)."""
    out = []
    for i in range(1, 13):
        if i <= len(effects):
            spa, base, limit, formula, mx = effects[i - 1]
        else:
            spa, base, limit, formula, mx = 254, 0, 0, 100, 0
        out.append(f"    effectid{i} = {spa}, effect_base_value{i} = {base}, "
                   f"effect_limit_value{i} = {limit}, formula{i} = {formula}, max{i} = {mx}")
    return ",\n".join(out)

def classes_sql(cls, level, helper):
    """⚠️ 255 means "this class can never use it" and is the class gate -- stricter and simpler than
    the AA bitmask. A HELPER is 255 for everyone: it is cast BY a spell, never by a player."""
    parts = []
    for c in range(1, 17):
        parts.append(f"classes{c} = {255 if (helper or c != cls) else level}")
    return "    " + ",\n    ".join(", ".join(parts[i:i+4]) for i in range(0, 16, 4))

def row_sql(r):
    helper = r.get("helper", False)
    level  = TIER_LEVEL.get(r["tier"], 255)
    anim, cast_anim = r["look"]
    timer  = TIER_TIMER.get(r["tier"], 0)
    recast = TIER_RECAST.get(r["tier"], 0)
    tz     = f"'{r['tz']}'" if r["tz"] else "''"
    rng    = MELEE_RANGE if r['id'] in SWINGS else 200
    return f"""
-- ---------------------------------------------------------------- {r['name']} ({r['id']})
INSERT INTO aotv4_disc_tmpl SELECT * FROM spells_new WHERE id = {r['tmpl']};
UPDATE aotv4_disc_tmpl SET
    id = {r['id']}, name = '{r['name']}', descnum = {r['id']},
    EndurCost = 1, EndurTimerIndex = {timer}, recast_time = {recast}, recovery_time = 0,
    cast_time = 0, mana = 0, skill = 98, targettype = {r['tt']}, goodEffect = {r['good']},
    new_icon = {r['icon']}, spellanim = {anim}, CastingAnim = {cast_anim},
    -- ⚠️⚠️ `IsDiscipline` IS A COLUMN AND IT IS NOT THE SAME THING AS THE IsDiscipline() FUNCTION.
    -- The function is DERIVED (mana == 0 AND EndurCost > 0) and was true for all of these from the
    -- start; the COLUMN is spells_new field 168, loaded into spells[].is_discipline, and its own
    -- header comment is "Will goto the combat window when cast". Cloning from a template that is not
    -- a discipline leaves it 0 -- and 0 is what makes the client treat the row as a spell.
    -- ⚠️ Stock writes -1, not 1. Strings::ToBool takes any non-zero number, but match stock.
    IsDiscipline = -1,
    -- ⚠️⚠️ `player_1` IS THE PARTICLE/TRAIL GRAPHIC, AND THE INSTANT TEMPLATE CARRIES `BLUE_TRAIL`.
    -- Cloning 4667 put a blue projectile trail on every sword swing. 263 of the 281 stock
    -- disciplines are `PLAYER_1`, which is the "no special effect" value, and so is every reference
    -- spell used for the presets above -- including the nukes and heals. So it is PLAYER_1 for all
    -- of these, not just the melee ones.
    player_1 = 'PLAYER_1',
    resisttype = 0, `range` = {rng}, AEDuration = 0, pushback = 0, pushup = 0,
    spellgroup = 0, `rank` = 0, teleport_zone = {tz},
    buffduration = {r['dur']}, buffdurationformula = {r['durf']},
    numhits = {r['numhits']}, numhitstype = {r['numhitstype']},
{effect_sql(r['effects'])},
{classes_sql(r['cls'], level, helper)};
INSERT INTO spells_new SELECT * FROM aotv4_disc_tmpl;
DELETE FROM aotv4_disc_tmpl;"""

HEADER = """-- @aotv4-migration
-- description: {desc}
-- check: SELECT `value` FROM `db_str` WHERE `id` = {last} AND `type` = 6
-- condition: empty
-- match:
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: {cname} tier 1/2/3 as DISCIPLINES. GENERATED by custom/tools/gen_class_abilities.py --
-- notes: edit the spec there, never this file.
-- notes: Cloned via temp table from stock rows so all ~236 columns stay byte-identical.

-- ⚠️⚠️ EndurTimerIndex IS LOAD BEARING AND MUST NOT BE 0. A discipline's recast lives EXCLUSIVELY in
-- `pTimerDisciplineReuseStart + timer_id` -- CastSpell's per-spell branch is guarded by
-- `&& !spells[spell_id].is_discipline`, so pTimerSpellStart is never started for one. Tier 1 takes
-- slot 5, tier 2 slot 6, tier 3 slot 2, the SAME three slots every class uses: a character is only
-- ever one class, so nobody can hold two tier 1s, and only slots 0-10 exist against 48 abilities.
-- ⚠️ EndurCost is 1, not 0 and not the real cost. IsDiscipline requires mana = 0 AND EndurCost > 0,
-- so 0 would leave the row out of the Combat Abilities window entirely. The real level-scaled cost
-- is charged by Lua, because EndurCost is a flat int column and cannot express `N x level`.

-- ⚠️ Scoped to exactly the ids this migration creates, so a half-applied run (the check keys on the
-- LAST thing written) re-runs cleanly instead of dying on a duplicate key.
DELETE FROM spells_new WHERE id IN ({ids});

DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;
CREATE TEMPORARY TABLE aotv4_disc_tmpl LIKE spells_new;
"""

FOOTER = """
DROP TEMPORARY TABLE IF EXISTS aotv4_disc_tmpl;

-- ---------------------------------------------------------------- descriptions
-- ⚠️⚠️ A spell with no db_str type 6 row renders BLANK and nothing errors. Written in the same
-- migration as the rows, deliberately.
-- ⚠️ The CLIENT resolves these from its own dbstr_us.txt, so this migration alone changes nothing
-- in game: it needs ./export_client_files and that file shipped to players.
DELETE FROM db_str WHERE id IN ({ids}) AND type = 6;
INSERT INTO db_str (id, type, value) VALUES
{values};
"""

def main():
    by_class = {}
    for r in ROWS:
        by_class.setdefault(r["cls"], []).append(r)

    for cls, rows in sorted(by_class.items()):
        cname = CLASS_NAME[cls]
        ids = [r["id"] for r in rows]
        # ⚠️ The check keys on the LAST description written, which is the last row here. Keying on
        # the first would let a run that died partway record itself as finished.
        last = ids[-1]
        desc = f"2026_08_21_{cname.lower()}_class_abilities"
        body = HEADER.format(desc=desc, last=last, cname=cname, ids=", ".join(map(str, ids)))
        body += "".join(row_sql(r) for r in rows)
        vals = ",\n".join(f" ({r['id']}, 6, '{r['desc']}')" for r in rows)
        body += FOOTER.format(ids=", ".join(map(str, ids)), values=vals)

        for r in rows:
            for bad, why in (("'", "apostrophe aborts the SQL"), ("%", "percent is eaten as a format token")):
                assert bad not in r["name"] and bad not in r["desc"], f"{r['name']}: {why}"

        path = os.path.join(SUBS, f"{desc}.sql")
        open(path, "w").write(body)
        print("wrote", os.path.relpath(path, SRC))

        # ------------------------------------------------------------------ Lua stubs
        # ⚠️ A stub, not a script. Every payload lives in ONE module keyed by spell id: 42 near
        # identical files is 42 chances to drift, and the shared helpers (cost, swing, cooldown cut)
        # are the whole point of having a module.
        for r in rows:
            if r.get("helper"):
                continue        # a helper is cast BY a spell and has no payload of its own
            stub = (f'-- {cname} tier {r["tier"]} -- {r["name"]}. GENERATED by '
                    f'custom/tools/gen_class_abilities.py.\n'
                    f'-- ⚠️ The payload is aotv4_class_abilities.PAYLOAD[{r["id"]}]; this file only routes to it.\n'
                    f'-- ⚠️⚠️ THE SCRIPT RUNS **BEFORE** THE SPELL APPLIES ITS OWN EFFECTS (the event fires at\n'
                    f'-- spell_effects.cpp:163, the slot loop starts at :225) -- and RETURNING NON-ZERO CANCELS\n'
                    f'-- THEM. Never return a value from these handlers.\n'
                    f'-- ⚠️⚠️ THE HANDLER IS `event_spell_effect`, WITH NO SUFFIX. LuaParser::ConvertLuaEvent\n'
                    f'-- (lua_parser.cpp:1604) collapses EVENT_SPELL_EFFECT_BOT/_CLIENT/_NPC into one, and the\n'
                    f'-- name table maps all of them to "event_spell_effect" (:104). Defining\n'
                    f'-- event_spell_effect_client / _npc compiles, luachecks and is NEVER CALLED -- the spell\n'
                    f'-- still plays its animation and applies its own SPAs, so the ability looks alive and does\n'
                    f'-- nothing. That is exactly how all 48 of these shipped inert the first time.\n'
                    f'local ab = require("aotv4_class_abilities")\n\n'
                    f'function event_spell_effect(e) ab.fire({r["id"]}, e) end\n')
            open(os.path.join(LUA, f'{r["id"]}.lua'), "w").write(stub)
        print("   stubs:", " ".join(str(r["id"]) for r in rows if not r.get("helper")))

    # ------------------------------------------------------------------ messages migration
    # ⚠️ WARRIOR'S THREE ARE INCLUDED HERE EVEN THOUGH ROWS DOES NOT CARRY THEM. v104 predates this
    # generator, and 44700 is the row that shows the inherited "invisible force" message most
    # obviously -- leaving it out made the first version of this migration a no-op on the one id its
    # own check reads, which the validator caught as non-idempotent.
    ids = sorted(set([44700, 44701, 44702] + [r["id"] for r in ROWS]))
    lines = []
    for i in ids:
        on_you, on_other, fades = MESSAGES.get(i, ("", "", ""))
        for t in (on_you, on_other, fades):
            assert "'" not in t and "%" not in t, f"{i}: apostrophe or percent in a message"
        lines.append(f"UPDATE spells_new SET cast_on_you = '{on_you}', cast_on_other = '{on_other}', "
                     f"spell_fades = '{fades}' WHERE id = {i};")
    body = MSG_HEADER + "\n".join(lines) + "\n"
    path = os.path.join(SUBS, "2026_08_21_class_ability_messages.sql")
    open(path, "w").write(body)
    print("wrote", os.path.relpath(path, SRC))

    # ------------------------------------------------------------ IsDiscipline repair migration
    # ⚠️⚠️ Its own migration for the same reason the messages are: v104-v118 are already merged, so
    # the inline fix above only helps a regeneration from scratch. 32 of the 51 rows shipped with
    # IsDiscipline = 0 because they were cloned from templates that are not disciplines (4667, 285,
    # 3265) -- only the 19 cloned from 4499 Defensive Discipline inherited it.
    disc = """-- @aotv4-migration
-- description: 2026_08_21_class_ability_is_discipline
-- check: SELECT `IsDiscipline` FROM `spells_new` WHERE `id` = 44700
-- condition: missing
-- match: -1
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: GENERATED by custom/tools/gen_class_abilities.py -- edit the spec there, never this file.
-- notes: THE COLUMN IsDiscipline IS NOT THE FUNCTION IsDiscipline(). The function is derived from
-- notes: mana and EndurCost and was always true here; the column is spells_new field 168, whose
-- notes: header comment is "Will goto the combat window when cast", and a clone inherits it from
-- notes: its template. 32 of these 51 rows were cloned from spells that are not disciplines.

UPDATE spells_new SET IsDiscipline = -1 WHERE id BETWEEN 44700 AND 44755;
"""
    path = os.path.join(SUBS, "2026_08_21_class_ability_is_discipline.sql")
    open(path, "w").write(disc)
    print("wrote", os.path.relpath(path, SRC))

    # ------------------------------------------------------------ particle repair migration
    trail = """-- @aotv4-migration
-- description: 2026_08_21_class_ability_particle
-- check: SELECT `player_1` FROM `spells_new` WHERE `id` = 44700
-- condition: contains
-- match: BLUE_TRAIL
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: GENERATED by custom/tools/gen_class_abilities.py -- edit the spec there, never this file.
-- notes: player_1 is the particle/trail graphic. The instant template (4667 Rebuke of the Ikaav) is
-- notes: BLUE_TRAIL, so 29 of these 51 rows fired a blue projectile trail -- on sword swings. 263 of
-- notes: the 281 stock disciplines use PLAYER_1, the no-effect value, and so does every spell the
-- notes: presets were sampled from, nukes and heals included.

UPDATE spells_new SET player_1 = 'PLAYER_1' WHERE id BETWEEN 44700 AND 44755;
"""
    path = os.path.join(SUBS, "2026_08_21_class_ability_particle.sql")
    open(path, "w").write(trail)
    print("wrote", os.path.relpath(path, SRC))

    # ------------------------------------------------------------ descriptions, for the WINDOW
    # ⚠️⚠️ THE SAME TEXT AS THE db_str ROWS, FROM THE SAME SPEC. The Combat Skills tab shows a
    # description, and there is no Lua binding to read db_str -- so rather than hand-copying the
    # strings into a module (two sources of truth, guaranteed to drift the first time one is edited)
    # the generator emits both from `desc`.
    # ⚠️ NO `|` OR `^` -- those are the ABILDATA wire separators and would split a row in half.
    desc_lines = []
    for r in sorted(ROWS, key=lambda x: x["id"]):
        if r.get("helper"):
            continue
        assert "|" not in r["desc"] and "^" not in r["desc"], f"{r['name']}: wire separator in desc"
        desc_lines.append('\t[%d] = "%s",' % (r["id"], r["desc"].replace('"', "'")))
    # Warrior predates this generator (v104) but its window rows still need text.
    war = {
        44700: "Cleave into your target with your equipped weapon, striking for weapon damage and seizing its attention.",
        44701: "Brace behind your guard. The next 5 melee attacks against you are each reduced by a tenth of your armor class, and any blow weaker than that is turned aside entirely.",
        44702: "Sweep everything in front of you. Each target struck shortens the recovery of Bulwark by 3 seconds, to a maximum of 9. Generates no additional threat.",
    }
    for i in sorted(war):
        desc_lines.insert(0, '\t[%d] = "%s",' % (i, war[i]))
    mod = ("-- GENERATED by custom/tools/gen_class_abilities.py -- do not edit.\n"
           "-- Descriptions for the Combat Skills tab, emitted from the SAME spec as the db_str rows\n"
           "-- so the window and the spellbook can never disagree.\n"
           "-- \u26a0\ufe0f No pipe or caret characters: those are the ABILDATA wire separators.\n"
           "return {\n" + "\n".join(sorted(set(desc_lines))) + "\n}\n")
    path = os.path.join(ROOT, "repo", "quests", "lua_modules", "aotv4_class_ability_desc.lua")
    open(path, "w").write(mod)
    print("wrote", os.path.relpath(path, SRC))

    # ------------------------------------------------------------ melee range repair migration
    rng = """-- @aotv4-migration
-- description: 2026_08_21_class_ability_melee_range
-- check: SELECT `range` FROM `spells_new` WHERE `id` = 44700
-- condition: missing
-- match: %d
-- shared-memory: yes
-- band:
-- author: Claude
-- notes: GENERATED by custom/tools/gen_class_abilities.py -- edit the spec there, never this file.
-- notes: Every class ability shipped at range 200, copied from the stock disciplines where it is
-- notes: harmless because those are self buffs. A TARGETED ability is range-checked against it in
-- notes: Mob::CastSpell, so the melee ones could be swung from 200 units away.
-- notes: Only the abilities whose payload actually swings are narrowed; heals, nukes, runes, pet
-- notes: summons and group buffs keep 200.

UPDATE spells_new SET `range` = %d WHERE id IN (%s);
""" % (MELEE_RANGE, MELEE_RANGE, ", ".join(str(i) for i in SWINGS))
    path = os.path.join(SUBS, "2026_08_21_class_ability_melee_range.sql")
    open(path, "w").write(rng)
    print("wrote", os.path.relpath(path, SRC))

if __name__ == "__main__":
    main()
