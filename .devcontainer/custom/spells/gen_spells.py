#!/usr/bin/env python3
"""
Generate the AoTv4 custom spell set from spell_design.csv.

Emits aotv4_custom_spells.sql:
  * a side table `aotv4_spell_origin` marking every EXISTING spell as 'native'
    (legacy, slated for removal) and the new set as 'aotv4'
  * 61 INSERTs into spells_new at ids BASE_ID.. (kept clear of native ids)

Nothing here deletes or edits a native spell row.

Auto-tiering: a power score is computed per ability from mana + endurance +
total damage/heal magnitude + a cooldown class bonus, then rank-mapped across
levels 1..LEVEL_CAP. Deterministic and reproducible -- retune SCORE_* below.
"""
import csv, os

BASE_ID   = 50000     # native max is 42602; this block is clearly separate
LEVEL_CAP = 30        # the whole set is tiered across levels 1-30

# ---- SPA numbers (common/spdat.h, namespace SpellEffect) --------------------
HP, AC, ATK, STR, DEX, AGI, STA, INT, WIS, CHA = 0, 1, 2, 4, 5, 6, 7, 8, 9, 10
ATKSPEED, MANA, STUN, SUMMONPET, DIVINEAURA    = 11, 15, 21, 33, 40
RUNE, DAMAGESHIELD, TOTALHP, ABSORBMAGIC       = 55, 59, 69, 78
HPONCE, HOT, IMPDMG, IMPHEAL, SPELLHASTE       = 79, 100, 124, 125, 127
REFLECT, MITMELEE, ENDURANCE, SKILLATTACK      = 158, 162, 189, 193
TAUNT, SKILLDMGAMT, SPELLPROCCHANCE            = 199, 220, 250
AMBIDEXTERITY, ADDMELEEPROC, LIMITTOSKILL      = 276, 419, 428
END_ABSORB_PCT                                 = 521

BLANK = 254           # empty effect slot

# ---- target / resist decoding ----------------------------------------------
ST_TARGET, ST_SELF, ST_AETARGET, ST_TAP, ST_GROUP = 5, 6, 8, 13, 41
RESIST = {"magic":1,"fire":2,"cold":3,"poison":4,"disease":5,
          "physical":8,"corruption":9,"beneficial":0,"self":0,"group":0}

DUR_FIXED = 3600      # formula >=200 -> temp=formula, capped by buffduration
                      # => a level-independent fixed tick count (206 native
                      #    spells use exactly this idiom)

# ---- icons: reused from thematically matching native spells ----------------
IC_FIRE, IC_MAGIC, IC_POISON, IC_DISEASE, IC_COLD = 51, 44, 42, 41, 56
IC_HEAL, IC_HOT, IC_TAP, IC_RUNE, IC_SHIELD       = 99, 118, 47, 77, 133
IC_DA, IC_STAT, IC_HASTE, IC_ALACRITY, IC_MANA    = 46, 6, 4, 16, 21
IC_THORNS, IC_COURAGE, IC_MELEE                   = 155, 132, 161

# ---------------------------------------------------------------------------
# Per-ability effect spec.
#   fx      list of (spa, base) or (spa, base, limit) or (spa, base, limit, max)
#   dur     duration in ticks (0 = instant / no buff)
#   native  (dform, dur) inherited from a comparable native spell
#   bucket  'A' = works on stock spell data, 'B' = needs Lua scripting
#   mag     magnitude override for scoring, when fx can't express it
# ---------------------------------------------------------------------------
SPEC = {
 # --- Bucket A: pure spells_new data -----------------------------------------
 "Ember":               dict(fx=[(HPONCE,-10),(HP,-1)], icon=IC_FIRE, bucket="A"),
 "Zap":                 dict(fx=[(HPONCE,-10),(STUN,1)], icon=IC_MAGIC, bucket="A"),
 "Smolder":             dict(fx=[(HP,-7)], icon=IC_FIRE, bucket="A"),
 "Shock":               dict(fx=[(HP,-43)], icon=IC_MAGIC, bucket="A"),
 "Poison":              dict(fx=[(HP,-3)], icon=IC_POISON, bucket="A"),
 "Toxic Gas":           dict(fx=[(HP,-8)], icon=IC_POISON, target=ST_AETARGET, aemax=4, bucket="A"),
 "Fever":               dict(fx=[(HP,-7)], icon=IC_DISEASE, bucket="A"),
 "Infection Cloud":     dict(fx=[(HP,-11)], icon=IC_DISEASE, target=ST_AETARGET, aemax=9, bucket="A"),
 "Frost":               dict(fx=[(HPONCE,-16),(AC,-12)], icon=IC_COLD, bucket="A"),
 "Chill":               dict(fx=[(HPONCE,-27),(ATKSPEED,95)], icon=IC_COLD, bucket="A"),
 "Lifetap":             dict(fx=[(HP,-16)], icon=IC_TAP, target=ST_TAP, bucket="A"),
 "Dread":               dict(fx=[(STR,-10),(HP,-4)], icon=IC_TAP, bucket="A"),
 "Throw Stone":         dict(fx=[(HP,-5)], icon=IC_MELEE, bucket="A"),
 "Basic Healing":       dict(fx=[(HP,17)], icon=IC_HEAL, bucket="A"),
 "Woodskin":            dict(fx=[(RUNE,13)], icon=IC_RUNE, bucket="A"),
 "Minor Magic Barrier": dict(fx=[(ABSORBMAGIC,20)], icon=IC_SHIELD, bucket="A"),
 "Tepid recovery":      dict(fx=[(HOT,5)], icon=IC_HOT, bucket="A"),
 "Desperate recovery":  dict(fx=[(HOT,1466)], icon=IC_HOT, bucket="A"),
 "Lay on Hands":        dict(fx=[(HP,600)], icon=IC_HEAL, bucket="A"),
 "Harm Touch":          dict(fx=[(HP,-200)], icon=IC_TAP, bucket="A"),
 "Minor Manaflow":      dict(fx=[(MANA,30)], icon=IC_MANA, bucket="A"),
 "Manatap":             dict(fx=[(MANA,-11)], icon=IC_MANA, bucket="A"),
 "Yawn":                dict(fx=[(ENDURANCE,-10)], icon=IC_DISEASE, bucket="A"),
 "Sturdy Footing":      dict(fx=[(END_ABSORB_PCT,12)], icon=IC_SHIELD, native=(3,360), bucket="A", mag=55),  # ~ no native SPA521; Ward of Vie profile
 "Passive Protection":  dict(fx=[(MITMELEE,1)], icon=IC_SHIELD, native=(3,360), bucket="A", mag=30),  # ~ Ward of Vie (SPA162 modal)
 "Minor Fortify Attack":dict(fx=[(SKILLDMGAMT,1)], icon=IC_MELEE, native=(3,1950), bucket="A", mag=25),  # ~ no native SPA220; focus profile
 "Concentration":       dict(fx=[(IMPDMG,8)], icon=IC_MAGIC, native=(3,1950), bucket="A", mag=45),  # ~ SPA124 modal, 206 spells
 "Willpower":           dict(fx=[(IMPHEAL,8)], icon=IC_HEAL, native=(3,1950), bucket="A", mag=45),  # ~ SPA125 modal, 48 spells
 "Courage":             dict(fx=[(TOTALHP,15),(AC,4)], icon=IC_COURAGE, native=(11,270), bucket="A"),  # ~ Courage (202)
 "Thoughfulness":       dict(fx=[(CHA,10),(INT,10),(WIS,10)], icon=IC_STAT, native=(3,400), bucket="A"),  # ~ Insight (175)
 "Brawn":               dict(fx=[(STR,12),(STA,12)], icon=IC_STAT, native=(11,270), bucket="A"),  # ~ Strengthen (40)
 "Nimbleness":          dict(fx=[(AGI,12),(DEX,12)], icon=IC_STAT, native=(11,270), bucket="A"),  # ~ Strengthen (40)
 "Haste":               dict(fx=[(ATKSPEED,105)], icon=IC_HASTE, native=(9,110), bucket="A", mag=60),  # ~ Quickness (39)
 "Alacrity":            dict(fx=[(SPELLHASTE,5)], icon=IC_ALACRITY, native=(3,1950), bucket="A", mag=50),  # ~ SPA127 modal, 93 spells
 "Ambidexterous":       dict(fx=[(AMBIDEXTERITY,500)], icon=IC_MELEE, native=(3,1950), bucket="A", mag=70),  # ~ no native SPA276; focus profile
 "Thorncoat":           dict(fx=[(DAMAGESHIELD,5)], icon=IC_THORNS, native=(3,1950), bucket="A", mag=20),  # ~ Thorncoat (519)
 "Taunt":               dict(fx=[(TAUNT,100)], icon=IC_MELEE, bucket="A", mag=20),
 # proc-granting passives: limit_value on ADDMELEEPROC carries the proc spell id,
 # patched post-insert (see fixups) because it references our own new ids.
 "Firefist":            dict(fx=[(ADDMELEEPROC,0,0),(LIMITTOSKILL,100)], icon=IC_FIRE, native=(3,600), bucket="A", proc="Ember", mag=35),  # ~ Divine Might (693)
 "Chi Block":           dict(fx=[(ADDMELEEPROC,0,0),(LIMITTOSKILL,100)], icon=IC_DISEASE, native=(3,600), bucket="A", proc="Yawn", mag=30),  # ~ Divine Might (693)
 "Inner Focus":         dict(fx=[(SPELLPROCCHANCE,50)], icon=IC_MELEE, native=(3,1950), bucket="A", mag=40),  # ~ no native SPA250; focus profile
 "Reflect":             dict(fx=[(REFLECT,100)], icon=IC_MAGIC, native=(11,300), bucket="A", mag=40),  # ~ Reflective Scales (SPA158)

 # --- Bucket B: rows exist + are pickable, mechanics land in Lua -------------
 "Kick":                dict(fx=[], icon=IC_MELEE, bucket="B", mag=10),
 "Strike":              dict(fx=[], icon=IC_MELEE, bucket="B", mag=60),
 "Snipe":               dict(fx=[], icon=IC_MELEE, bucket="B", mag=50),
 "Languid Healing":     dict(fx=[], icon=IC_HEAL, bucket="B", mag=10),
 # NO stock DivineAura(40) effect: that SPA is TOTAL invulnerability, which would
 # absorb everything before EVENT_DAMAGE_TAKEN could apply the design's "only hits
 # under 15% of max HP" threshold. The threshold lives in aotv4_reactions.lua.
 "Divine Aura":         dict(fx=[], icon=IC_DA, bucket="B", mag=300),
 "Breeze":              dict(fx=[], icon=IC_MANA, native=(3,270), bucket="B", mag=25),  # ~ Clarity (174)
 "Life Ebb":            dict(fx=[], icon=IC_MANA, native=(3,270), bucket="B", mag=25),  # ~ Clarity (174)
 # SELF-targeted despite being a group effect: the mana drain is a flat 5 from the
 # CASTER per tick, so exactly one buff (and one tick) must exist. A group-targeted
 # buff tics once per member and would drain 5 x group size. The Lua distributes
 # the healing to the group instead -- see quests/global/spells/50028.lua.
 "Minor Regeneration":  dict(fx=[], icon=IC_HOT, native=(10,205), target=ST_SELF, bucket="B", mag=20),  # ~ Regeneration (144)
 "Duel":                dict(fx=[], icon=IC_MELEE, bucket="B", mag=90),
 "Blade Turn":          dict(fx=[], icon=IC_RUNE, native=(3,360), bucket="B", mag=30),  # ~ Ward of Calliav (SPA163 modal)
 "Open Wounds":         dict(fx=[], icon=IC_MELEE, bucket="B", mag=70),
 "The Left":            dict(fx=[], icon=IC_MELEE, bucket="B", mag=55),
 "Armor Rend":          dict(fx=[], icon=IC_MELEE, bucket="B", mag=85),
 "Counterattack":       dict(fx=[], icon=IC_MELEE, bucket="B", mag=40),
 "Vengeful Aura":       dict(fx=[], icon=IC_THORNS, bucket="B", mag=45),
 "Summon Bear":            dict(fx=[], icon=IC_MELEE, native=(0,0), bucket="B", mag=40),  # ~ native pet spells, 131 at 0/0
 "Summon Leopard":         dict(fx=[], icon=IC_MELEE, native=(0,0), bucket="B", mag=45),  # ~ native pet spells, 131 at 0/0
 "Summon Skeletal Minion": dict(fx=[], icon=IC_TAP,   native=(0,0), bucket="B", mag=50),  # ~ native pet spells, 131 at 0/0
 "Summon Fire Imp":        dict(fx=[], icon=IC_FIRE,  native=(0,0), bucket="B", mag=50),  # ~ native pet spells, 131 at 0/0
 "Summon Willowisp":       dict(fx=[], icon=IC_MANA,  native=(0,0), bucket="B", mag=45),  # ~ native pet spells, 131 at 0/0
}

# ---------------------------------------------------------------------------
# Expansion set (spell_design_extra.csv). The original 61-ability sheet is thin
# once spread over levels 1-30 -- roughly two picks per level, which does not
# feel like drawing from a pool. These widen it: upgraded versions of the core
# damage/heal lines, plus classic utility and crowd control given our own
# numbers. All are Bucket A, so the whole expansion needs no new Lua.
# ---------------------------------------------------------------------------
MOVEMENT, INVIS, SEEINVIS, BLIND_SPA, STUN_SPA          = 3, 12, 13, 20, 21
# Lull is ChangeFrenzyRad(30) + Harmony(86), NOT the Lull(18) constant -- 18 is
# unused by the native Lull line. Verified against native spell data.
FRENZYRAD, HARMONY, LEVITATE_SPA                        = 30, 86, 57
FEAR_SPA, BINDAFFINITY, GATE_SPA, CANCELMAGIC, MEZ_SPA   = 23, 25, 26, 27, 31
REVERSE_DS, DEATHSAVE, MIT_SPELL, MIT_MELEE, NEGATE_ATK  = 121, 150, 161, 162, 163
WEAPONPROC, DEFENSIVEPROC, FC_HEAL_IN, SPELL_THRESH      = 85, 323, 393, 452
MELEE_THRESH                                             = 453

# Carriers these procs/triggers fire. Fixed ids, so they can be referenced
# directly -- see HELPERS below.
TRIG_REPTILE, TRIG_ADRENALINE, PROC_LYNX                 = 50156, 50157, 50158
TRIG_MOONFIRE                                            = 50159

# From EQEmu_Unique_Later_Expansion_Spells.xlsx. Conventions verified against
# the real spell of that name:
#   63  base = memblur pct chance          (Blanket of Forgetfulness 20)
#   184 base = accuracy, limit = SKILL id  (Falcon Eye 20/7=Archery)
#   192 base = hate amount
#   374 base = trigger CHANCE, limit = SPELL ID  (Lich 100/9490) -- NOT a raw id
#   AEDuration turns an AE into a rain of waves (Gelid Rains 7500)
MEMBLUR, HITCHANCE, HATE_SPA, APPLYEFFECT                = 63, 184, 192, 374
SKILL_ARCHERY_ID                                         = 7
RES_FIRE, RES_COLD, RES_POISON, RES_DISEASE, RES_MAGIC   = 46, 47, 48, 49, 50

SPEC_EXTRA = {
 # --- damage ---------------------------------------------------------------
 "Cinder":          dict(fx=[(HP,-22)], icon=IC_FIRE, bucket="A"),
 "Scorch":          dict(fx=[(HP,-38)], icon=IC_FIRE, bucket="A"),
 "Immolate":        dict(fx=[(HP,-12)], icon=IC_FIRE, bucket="A"),
 "Ignite Blood":    dict(fx=[(HP,-9),(RES_FIRE,-20)], icon=IC_FIRE, bucket="A"),
 "Jolt":            dict(fx=[(HPONCE,-28),(STUN_SPA,1)], icon=IC_MAGIC, bucket="A"),
 "Arc Lightning":   dict(fx=[(HP,-60)], icon=IC_MAGIC, bucket="A"),
 "Rime":            dict(fx=[(HPONCE,-33),(AC,-18)], icon=IC_COLD, bucket="A"),
 "Glacial Spike":   dict(fx=[(HP,-55)], icon=IC_COLD, bucket="A"),
 "Venom":           dict(fx=[(HP,-6)], icon=IC_POISON, bucket="A"),
 "Plague":          dict(fx=[(HP,-14)], icon=IC_DISEASE, bucket="A"),
 "Withering Curse": dict(fx=[(STR,-20),(HP,-9)], icon=IC_TAP, bucket="A"),
 "Siphon Life":     dict(fx=[(HP,-30)], icon=IC_TAP, target=ST_TAP, bucket="A"),
 # --- heals ----------------------------------------------------------------
 "Mend":            dict(fx=[(HP,40)], icon=IC_HEAL, bucket="A"),
 "Salve":           dict(fx=[(HP,75)], icon=IC_HEAL, bucket="A"),
 "Renewal":         dict(fx=[(HOT,12)], icon=IC_HOT, bucket="A"),
 "Group Mend":      dict(fx=[(HP,25)], icon=IC_HEAL, bucket="A"),
 "Second Wind":     dict(fx=[(HP,30),(ENDURANCE,20)], icon=IC_HEAL, bucket="A"),
 # --- wards / defensive buffs ----------------------------------------------
 "Barkskin":        dict(fx=[(RUNE,35)], icon=IC_RUNE, bucket="A"),
 "Stoneskin":       dict(fx=[(RUNE,80)], icon=IC_RUNE, bucket="A"),
 "Arcane Ward":     dict(fx=[(ABSORBMAGIC,60)], icon=IC_SHIELD, bucket="A"),
 "Shielding":       dict(fx=[(AC,18)], icon=IC_SHIELD, native=(3,400), bucket="A"),
 "Resist Elements": dict(fx=[(RES_FIRE,15),(RES_COLD,15),(RES_POISON,15),
                            (RES_DISEASE,15),(RES_MAGIC,15)],
                         icon=IC_SHIELD, native=(3,400), bucket="A"),
 "Thorns":          dict(fx=[(DAMAGESHIELD,12)], icon=IC_THORNS, native=(3,400), bucket="A"),
 # --- crowd control --------------------------------------------------------
 "Ensnare":         dict(fx=[(MOVEMENT,-50)], icon=IC_MAGIC, bucket="A"),
 "Terror":          dict(fx=[(FEAR_SPA,1)], icon=IC_MAGIC, bucket="A", mag=40),
 "Blinding Flash":  dict(fx=[(BLIND_SPA,1)], icon=IC_MAGIC, bucket="A", mag=30),
 "Concussion":      dict(fx=[(STUN_SPA,1)], icon=IC_MAGIC, bucket="A", mag=30),
 "Cancel Magic":    dict(fx=[(CANCELMAGIC,1)], icon=IC_MAGIC, bucket="A", mag=25),
 # --- unusual heals: none of these are a plain heal, a HoT, or a group heal --
 "Reptile Hide":      dict(fx=[(DEFENSIVEPROC,TRIG_REPTILE,300)], icon=IC_RUNE, bucket="A", mag=60),
 "Bloodward":         dict(fx=[(REVERSE_DS,12)], icon=IC_HOT, bucket="A", mag=45),
 "Last Rites":        dict(fx=[(DEATHSAVE,1)], icon=IC_HEAL, bucket="A", mag=90),
 "Adrenaline Surge":  dict(fx=[(MELEE_THRESH,TRIG_ADRENALINE,150)], icon=IC_HEAL, bucket="A", mag=70),
 "Arcane Bulwark":    dict(fx=[(SPELL_THRESH,50,200,1500)], icon=IC_SHIELD, bucket="A", mag=65),
 "Spirit of the Lynx":dict(fx=[(WEAPONPROC,PROC_LYNX)], icon=IC_TAP, bucket="A", mag=60),
 "Mending Grace":     dict(fx=[(FC_HEAL_IN,15)], icon=IC_HEAL, bucket="A", mag=40),
 "Blade Ward":        dict(fx=[(NEGATE_ATK,2)], icon=IC_RUNE, bucket="A", mag=50),
 "Stoneshield":       dict(fx=[(MIT_MELEE,15,0,500)], icon=IC_SHIELD, bucket="A", mag=55),
 "Spellshield":       dict(fx=[(MIT_SPELL,25,0,500)], icon=IC_SHIELD, bucket="A", mag=55),

 # --- from the unique later-expansion catalog -------------------------------
 # Rains: AEDuration makes the AE fall in repeated waves instead of landing once.
 "Sleetfall":         dict(fx=[(HP,-18)], icon=IC_COLD, target=ST_AETARGET,
                           aoe=25, aedur=7500, bucket="A"),
 "Emberfall":         dict(fx=[(HP,-20)], icon=IC_FIRE, target=ST_AETARGET,
                           aoe=25, aedur=7500, bucket="A"),
 "Veil of Forgetting":dict(fx=[(MEMBLUR,25)], icon=IC_MAGIC, target=ST_AETARGET,
                           aoe=60, bucket="A", mag=55),
 "Malaise":           dict(fx=[(RES_COLD,-12),(RES_POISON,-12),(RES_MAGIC,-12)],
                           icon=IC_MAGIC, bucket="A", mag=45),
 "Scent of Ruin":     dict(fx=[(RES_FIRE,-10),(RES_COLD,-10),(RES_POISON,-10),
                               (RES_DISEASE,-10),(RES_MAGIC,-10)],
                           icon=IC_MAGIC, bucket="A", mag=55),
 "Falcon Eye":        dict(fx=[(HITCHANCE,20,SKILL_ARCHERY_ID),(SEEINVIS,1)],
                           icon=IC_MELEE, native=(3,700), bucket="A", mag=45),
 "Moonfire":          dict(fx=[(HP,-45),(APPLYEFFECT,100,TRIG_MOONFIRE)],
                           icon=IC_MAGIC, bucket="A", mag=70),
 "Mind Shatter":      dict(fx=[(MANA,-15),(HP,-11)], icon=IC_MANA, bucket="A"),
 "War March":         dict(fx=[(ATKSPEED,125),(STR,20),(ATK,15),(DAMAGESHIELD,8)],
                           icon=IC_HASTE, bucket="A", mag=70),
 "Chorus of Replenishment": dict(fx=[(HOT,8),(MANA,6)], icon=IC_HOT, bucket="A"),
 "Torrent of Misery": dict(fx=[(HATE_SPA,400)], icon=IC_MELEE, target=ST_AETARGET,
                           aoe=40, bucket="A", mag=50),
 "Bond of Xalgoz":    dict(fx=[(HP,-13)], icon=IC_TAP, target=ST_TAP, bucket="A"),

 # --- utility --------------------------------------------------------------
 "Invisibility":    dict(fx=[(INVIS,1)], icon=IC_MAGIC, native=(3,180), bucket="A", mag=30),
 "Levitate":        dict(fx=[(LEVITATE_SPA,1)], icon=IC_MAGIC, native=(3,360), bucket="A", mag=25),
}

# ---------------------------------------------------------------------------
# Helper carriers (ids 50150+). Applied BY the Bucket B Lua, never scribed or
# offered: classes8 = 255 keeps them out of every class's spell list, and the
# reward pool only ever draws from the 61 design ids.
#
# The Armor Rend ladder exists because a spell's base value is STATIC while the
# design wants "AC down by 5% of damage dealt". Lua computes the damage, then
# picks the closest rung -- approximate proportionality without a spell row per
# possible number.
# ---------------------------------------------------------------------------
HELPER_ID = 50150   # design ids run from BASE_ID; keep this above them
SILENCE, HEALRATE = 96, 120

HELPERS = [
    dict(id=50150, name="Open Wounds", fx=[], dur=10, target=ST_TARGET, icon=IC_MELEE,
         note="bleed marker; Lua pools 25% of each hit and bleeds it out on tic"),
    dict(id=50151, name="Armor Rend", fx=[(AC, -5)],   dur=12, target=ST_TARGET, icon=IC_MELEE,
         note="AC debuff rung 1"),
    dict(id=50152, name="Armor Rend", fx=[(AC, -15)],  dur=12, target=ST_TARGET, icon=IC_MELEE,
         note="AC debuff rung 2"),
    dict(id=50153, name="Armor Rend", fx=[(AC, -40)],  dur=12, target=ST_TARGET, icon=IC_MELEE,
         note="AC debuff rung 3"),
    dict(id=50154, name="Armor Rend", fx=[(AC, -100)], dur=12, target=ST_TARGET, icon=IC_MELEE,
         note="AC debuff rung 4"),
    # Duel lock: Silence blocks casting and HealRate -100 blocks healing outright
    # (zone/effects.cpp:498 does value += value * HealRate / 100). The damage
    # rules -- 2x between partners, immunity to everyone else -- are in Lua.
    # Carriers fired BY the proc/trigger abilities above. Their base values are
    # the actual heal/tap; the ability row just points at them.
    dict(id=50156, name="Reptile Hide", fx=[(HPONCE, 45)], dur=0, target=ST_SELF,
         icon=IC_RUNE, good=1, note="healed when struck (DefensiveProc target)"),
    dict(id=50157, name="Adrenaline Surge", fx=[(HPONCE, 90)], dur=0, target=ST_SELF,
         icon=IC_HEAL, good=1, note="healed after a big hit (MeleeThreshold target)"),
    dict(id=50159, name="Moonfire", fx=[(HPONCE, 25)], dur=0, target=ST_SELF,
         icon=IC_HEAL, good=1, note="self-heal fired by Moonfire (ApplyEffect target)"),
    dict(id=50158, name="Lynx Maw", fx=[(HP, -30)], dur=0, target=ST_TAP,
         icon=IC_TAP, note="lifetap proc (WeaponProc target)"),
    dict(id=50155, name="Duel", fx=[(SILENCE, 1), (HEALRATE, -100)], dur=6,
         target=ST_TARGET, icon=IC_MELEE, note="duel lock, applied to BOTH participants"),
]

# Armor Rend rungs, paired with the minimum AC reduction each represents.
ARMOR_REND_LADDER = [(5, 50151), (15, 50152), (40, 50153), (100, 50154)]

# ---- scoring weights -------------------------------------------------------
SCORE_MANA, SCORE_END, SCORE_MAG = 1.0, 2.5, 0.35
SCORE_CD_LONG, SCORE_CD_MED      = 150, 30   # >=15min ultimate / >=100s
RIDER_PTS                        = 10        # flat worth of a non-resource effect


def esc(s):
    return s.replace("\\", "\\\\").replace("'", "''")


def magnitude(name, spec, ticks):
    """Total damage/heal an ability represents -- feeds the power score."""
    if "mag" in spec:
        return spec["mag"]
    total = 0
    for e in spec["fx"]:
        spa, base = e[0], e[1]
        if spa in (HP, HOT):
            total += abs(base) * (ticks if ticks else 1)
        elif spa in (HPONCE, MANA, ENDURANCE, RUNE, ABSORBMAGIC, TOTALHP):
            total += abs(base)
        else:
            # Percentage- and flag-style SPAs encode their base as a rate or a
            # chance (Haste 105 = +5%, Taunt 100 = 100% chance), so the raw base
            # is meaningless as magnitude. Count a flat rider instead; abilities
            # whose whole value is such an effect carry an explicit `mag`.
            total += RIDER_PTS
    return total


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    # The original design sheet stays untouched as the source of truth; the
    # expansion set lives in its own file so provenance is obvious.
    with open(os.path.join(here, "spell_design.csv")) as f:
        rows = list(csv.DictReader(f))
    with open(os.path.join(here, "spell_design_extra.csv")) as f:
        rows += list(csv.DictReader(f))

    SPEC.update(SPEC_EXTRA)

    missing = [r["name"] for r in rows if r["name"] not in SPEC]
    if missing:
        raise SystemExit("No effect spec for: " + ", ".join(missing))

    # ---- score + auto-tier -------------------------------------------------
    scored = []
    for r in rows:
        spec  = SPEC[r["name"]]
        mana  = int(r["mana"]); end = int(r["end_cost"])
        cd    = int(r["cooldown_ms"]); ticks = int(r["duration_ticks"])
        mag   = magnitude(r["name"], spec, ticks)
        score = mana * SCORE_MANA + end * SCORE_END + mag * SCORE_MAG
        score += SCORE_CD_LONG if cd >= 900000 else (SCORE_CD_MED if cd >= 100000 else 0)
        scored.append((score, r, spec))

    # rank-map onto 1..LEVEL_CAP so the curve is evenly populated
    order = sorted(range(len(scored)), key=lambda i: scored[i][0])
    level = {}
    for rank, i in enumerate(order):
        name = scored[i][1]["name"]
        level[name] = max(1, round((rank + 1) / len(order) * LEVEL_CAP))

    ids = {r["name"]: BASE_ID + i for i, r in enumerate(rows)}
    # Design ids grow with the CSVs; helpers sit at fixed ids above them. Adding
    # ~40 abilities once pushed the design block straight into HELPER_ID and the
    # insert died on a duplicate primary key -- fail loudly instead.
    if BASE_ID + len(rows) > HELPER_ID:
        raise SystemExit(
            "design ids (%d..%d) collide with HELPER_ID %d -- raise HELPER_ID"
            % (BASE_ID, BASE_ID + len(rows) - 1, HELPER_ID))

    # ---- emit --------------------------------------------------------------
    out = [
        "-- AoTv4 custom spell set -- GENERATED by gen_spells.py, do not hand-edit.",
        "-- Source of truth: spell_design.csv   Mapping notes: SPELL_REBUILD.md",
        "-- Inserts %d abilities at ids %d..%d. Does NOT delete or modify any native spell."
        % (len(rows), BASE_ID, BASE_ID + len(rows) - 1),
        "",
        "-- 1) Mark spell provenance so the native set can be retired later.",
        "CREATE TABLE IF NOT EXISTS aotv4_spell_origin (",
        "  spell_id INT NOT NULL PRIMARY KEY,",
        "  origin   VARCHAR(16) NOT NULL,   -- 'native' (legacy) | 'aotv4' (this set)",
        "  bucket   CHAR(1) NULL,           -- A = stock SPA data, B = needs Lua",
        "  KEY origin_idx (origin)",
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;",
        "",
        "INSERT IGNORE INTO aotv4_spell_origin (spell_id, origin)",
        "  SELECT id, 'native' FROM spells_new;",
        "",
        "-- 2) The new set.",
        "DELETE FROM spells_new         WHERE id       BETWEEN %d AND %d;" % (BASE_ID, BASE_ID + 999),
        "DELETE FROM aotv4_spell_origin WHERE spell_id BETWEEN %d AND %d;" % (BASE_ID, BASE_ID + 999),
        "",
    ]

    for r in rows:
        name  = r["name"]
        spec  = SPEC[name]
        sid   = ids[name]
        kind  = r["resist"]
        ticks = int(r["duration_ticks"])
        good  = 1 if kind in ("beneficial", "self", "group") else 0

        target = spec.get("target",
                          ST_SELF if kind == "self" else
                          ST_GROUP if kind == "group" else ST_TARGET)

        if "native" in spec:
            # always-on ability: inherit a comparable native spell's duration
            # profile rather than being made permanent
            dur_formula, dur = spec["native"]
        elif ticks:
            dur_formula, dur = DUR_FIXED, ticks
        else:
            dur_formula, dur = 0, 0

        cols = {
            "id": sid, "name": "'%s'" % esc(name),
            # These varchar columns DEFAULT NULL, and the shared-memory loader builds
            # std::string straight from them -- a NULL aborts the spell load with
            # "basic_string: construction from null is not valid". Native rows are
            # never NULL here, so always write empty strings.
                        # typedescnum/effectdescnum also DEFAULT NULL and are read through
            # Strings::To*, which builds a std::string from the raw column -- a NULL
            # throws "construction from null" and aborts the whole spell load.
            "typedescnum": 0, "effectdescnum": 0,
"teleport_zone": "''", "you_cast": "''", "other_casts": "''",
            "cast_on_you": "''", "cast_on_other": "''", "spell_fades": "''",

            "range": 0 if target == ST_SELF else 200,
            "aoerange": spec.get("aoe", 40 if target == ST_AETARGET else 0),
            # AEDuration turns an AE into a RAIN -- repeated waves over that many
            # ms rather than one hit (Gelid Rains: AEDuration 7500, aoerange 25).
            "AEDuration": spec.get("aedur", 0),
            "cast_time": int(r["cast_time_ms"]),
            "recast_time": int(r["cooldown_ms"]),
            "recovery_time": 1500 if int(r["cast_time_ms"]) else 0,
            "buffdurationformula": dur_formula, "buffduration": dur,
            "mana": int(r["mana"]),
            "EndurCost": int(r["end_cost"]),
            "EndurTimerIndex": 1 if int(r["end_cost"]) else 0,
            "icon": 2511, "new_icon": spec["icon"], "memicon": spec["icon"],
            "targettype": target,
            "resisttype": RESIST[kind],
            "ResistDiff": int(r["resist_adj"]),
            "goodEffect": good,
            "aemaxtargets": spec.get("aemax", 0),
            "classes8": level[name],          # Bard = the reward-pool index
            "spell_category": -99,
            "not_extendable": 1,
        }
        for slot in range(1, 13):
            e = spec["fx"][slot - 1] if slot <= len(spec["fx"]) else None
            cols["effectid%d" % slot]          = e[0] if e else BLANK
            cols["effect_base_value%d" % slot] = e[1] if e else 0
            cols["effect_limit_value%d" % slot] = e[2] if e and len(e) > 2 else 0
            cols["max%d" % slot]               = e[3] if e and len(e) > 3 else 0
            cols["formula%d" % slot]           = 100

        keys = ", ".join("`%s`" % k for k in cols)
        vals = ", ".join(str(v) for v in cols.values())
        out.append("INSERT INTO spells_new (%s) VALUES (%s);" % (keys, vals))
        out.append("INSERT INTO aotv4_spell_origin (spell_id, origin, bucket) "
                   "VALUES (%d, 'aotv4', '%s');" % (sid, spec["bucket"]))

    # ---- helper spells -----------------------------------------------------
    # Not player-facing and never offered as a reward: these are carriers the
    # Bucket B Lua applies to a target. They live above HELPER_ID so the reward
    # pool (which only knows the 61 design ids) can never hand one out.
    out.append("")
    out.append("-- 3) Helper carriers applied by Bucket B Lua. Not reward-pool entries.")
    for h in HELPERS:
        cols = {
            "id": h["id"], "name": "'%s'" % esc(h["name"]),
            # These varchar columns DEFAULT NULL, and the shared-memory loader builds
            # std::string straight from them -- a NULL aborts the spell load with
            # "basic_string: construction from null is not valid". Native rows are
            # never NULL here, so always write empty strings.
                        # typedescnum/effectdescnum also DEFAULT NULL and are read through
            # Strings::To*, which builds a std::string from the raw column -- a NULL
            # throws "construction from null" and aborts the whole spell load.
            "typedescnum": 0, "effectdescnum": 0,
"teleport_zone": "''", "you_cast": "''", "other_casts": "''",
            "cast_on_you": "''", "cast_on_other": "''", "spell_fades": "''",

            "range": 0 if h["target"] == ST_SELF else 200,
            "aoerange": 0, "cast_time": 0, "recast_time": 0, "recovery_time": 0,
            "buffdurationformula": DUR_FIXED, "buffduration": h["dur"],
            "mana": 0, "EndurCost": 0, "EndurTimerIndex": 0,
            "icon": 2511, "new_icon": h["icon"], "memicon": h["icon"],
            "targettype": h["target"],
            "resisttype": h.get("resist", 0),
            "ResistDiff": -1000,          # carriers must never be resisted away
            "goodEffect": h.get("good", 0),
            "aemaxtargets": 0,
            "classes8": 255,              # 255 = unusable/unscribable by any class
            "spell_category": -99,
            "not_extendable": 1,
        }
        for slot in range(1, 13):
            e = h["fx"][slot - 1] if slot <= len(h["fx"]) else None
            cols["effectid%d" % slot]           = e[0] if e else BLANK
            cols["effect_base_value%d" % slot]  = e[1] if e else 0
            cols["effect_limit_value%d" % slot] = e[2] if e and len(e) > 2 else 0
            cols["max%d" % slot]                = e[3] if e and len(e) > 3 else 0
            cols["formula%d" % slot]            = 100
        keys = ", ".join("`%s`" % k for k in cols)
        vals = ", ".join(str(v) for v in cols.values())
        out.append("INSERT INTO spells_new (%s) VALUES (%s);  -- %s" % (keys, vals, h["note"]))
        out.append("INSERT INTO aotv4_spell_origin (spell_id, origin, bucket) "
                   "VALUES (%d, 'aotv4', 'H');" % h["id"])

    # ---- fixups: proc effects that point at our own new spell ids -----------
    out.append("")
    out.append("-- 3) Proc grants reference ids from this same set.")
    for name, spec in SPEC.items():
        if "proc" in spec:
            out.append("UPDATE spells_new SET effect_limit_value1 = %d WHERE id = %d;  -- %s procs %s"
                       % (ids[spec["proc"]], ids[name], name, spec["proc"]))

    sql = os.path.join(here, "aotv4_custom_spells.sql")
    with open(sql, "w") as f:
        f.write("\n".join(out) + "\n")

    # ---- review table ------------------------------------------------------
    print("%-24s %5s %6s %5s  %s" % ("ABILITY", "ID", "SCORE", "LVL", "BUCKET"))
    for score, r, spec in sorted(scored, key=lambda t: t[0]):
        n = r["name"]
        print("%-24s %5d %6.1f %5d  %s" % (n, ids[n], score, level[n], spec["bucket"]))
    print("\nWrote %s" % sql)


if __name__ == "__main__":
    main()
