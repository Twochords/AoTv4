#!/usr/bin/perl
# gen_stock_pool.pl -- regenerate the level-up reward pool from the STOCK EQ spell set.
# =============================================================================================
# 2026-07-26: the 113-spell custom set (43000-43149) turned out to be mostly redundant with
# native spells, so the picker goes back to the original setup described in CLAUDE.md section 5:
# draw from spells_new where id < 10000, indexed by classes8 (the spell's lowest learn level).
#
# The genuinely UNIQUE customs are kept and appended into the same pool: the ones whose behaviour
# lives in Lua (quests/global/spells/43xxx.lua, aotv4_summon_table, aotv4_reactions) rather than in
# effect slots, because no stock spell does what they do.  They are identified by having every
# effect slot empty (254) -- that is exactly the Lua-driven set.
#
# NOT touched: the class auras (43500+), which are achievement rewards, and the pet summon spells
# (43200+), which the kept summon abilities call into.
#
# Writes four generated files that spell_choice.lua consumes:
#   spell_pool.lua      pool[learn_level] = { {id=, name=}, ... }
#   spell_icons.lua     id -> spellbook icon index (the picker window shows it)
#   spell_desc.lua      id -> description text (sent as SPELLDESCDATA)
#   spell_blacklist.lua { [id]=true } never offered -- travel, rez, enchant materials, LDoN junk
#
# The blacklist MUST be regenerated with the pool. It was emptied when the custom set took over
# ("empty by design: the pool is a hand-authored custom set"), so re-pointing at the stock set
# without rebuilding it silently reopens every port, rez and Enchant-material spell to the picker.
#
#   perl .devcontainer/custom/spells/gen_stock_pool.pl
#
# Lua modules are require()d once per zone process, so a zone restart is needed to pick these up
# (#reloadquest does NOT reload required modules -- CLAUDE.md section 10).

use strict;
use warnings;

my $DIR = "/src/.devcontainer/repo/quests/lua_modules";

# ---------------------------------------------------------------- helpers
sub q_lua {                     # escape a value for a double-quoted Lua string
    my $s = shift // '';
    $s =~ s/\\/\\\\/g;
    $s =~ s/"/\\"/g;
    $s =~ s/[\r\n]+/ /g;
    return $s;
}

# A literal % is genuinely dangerous here: Client::Message is printf-style, so a stray % in a
# description is consumed as a format token -- it renders as garbage in the spellbook (CLAUDE.md
# section 14) and can eat the following bytes on the way out. Stock descriptions are full of them.
sub sanitize_desc {
    my $s = shift // '';
    $s =~ s/[\r\n]+/ /g;
    $s =~ s/\s+/ /g;
    $s =~ s/\s*%/ percent/g;    # "10%" -> "10 percent"
    $s =~ s/[\^|]/ /g;          # ^ and | are the wire-format separators
    $s =~ s/^\s+|\s+$//g;
    return $s;
}

sub mysql {
    my $sql = shift;
    open(my $h, "-|", "mysql", "-h127.0.0.1", "-upeq", "-ppeqpass", "peq", "-N", "--batch", "-e", $sql)
        or die "mysql: $!";
    my @rows;
    while (my $l = <$h>) { chomp $l; push @rows, [split(/\t/, $l, -1)] if length $l; }
    close $h;
    return @rows;
}

# ---------------------------------------------------------------- source data
# Stock pool: the same filter the original setup used. classes8 is the lowest level any class can
# learn the spell at, which doubles as the pool index.
my @stock = mysql(q{
    SELECT s.id, s.name, s.classes8, s.new_icon, IFNULL(d.value,''), IFNULL(s.teleport_zone,''),
           (33 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 71 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 152 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 106 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 108 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)) AS is_pet
    FROM spells_new s LEFT JOIN db_str d ON d.id = s.descnum AND d.type = 6
    WHERE s.id < 10000
      AND s.name NOT LIKE '% Rk. %'
      AND s.classes8 BETWEEN 1 AND 100
    ORDER BY s.classes8, s.id});

# Kept customs, ONE band: 43300-43349 -- custom spells written to fill gaps in native lines (e.g.
# the low-level Skin of the Reptile tiers, custom/sql/aotv4_reptile_line.sql). These DO use effect
# slots. Put helper / trigger spells at 43350+ so they are never offered.
#
# ⚠️⚠️ 43000-43149 IS DELIBERATELY EXCLUDED (2026-07-27) -- DO NOT ADD IT BACK. That band is the
# retired 113-spell custom reward set (Ember, Zap, Kick, Strike, Counterattack, Moonfire...). Twenty
# of them -- the ones whose every effect slot is 254, with the behaviour in quests/global/spells/
# 43xxx.lua -- were still being offered by an earlier clause here:
#     (s.id BETWEEN 43000 AND 43149 AND s.effectid1 = 254 AND s.effectid2 = 254 AND s.effectid3 = 254)
# They are retired for good: mostly redundant with native spells, and being inert markers they
# render as "no effects" in any client-side spell description, including the Spell Journal window.
# The ROWS remain in spells_new (dormant, referenced by their Lua scripts and by the 43150-43199
# helpers); this only stops them being offered. Full restore of all 202 custom rows, if it is ever
# wanted, is custom/sql/aotv4_custom_spells_backup.sql.
#
# ⚠️ Everything from 43300 up is LIVE and unrelated: 43300-43349 the custom lines below, 43350-43399
# their triggers plus the Shield Wall buffs, 43400-43454 the AA tree buffs and pet wards, 43500-43565
# the class auras. "Get rid of the 43xxx spells" must never be read as this whole range.
my @custom = mysql(q{
    SELECT s.id, s.name, s.classes8, s.new_icon, IFNULL(d.value,''), IFNULL(s.teleport_zone,''),
           (33 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 71 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 152 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 106 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
         OR 108 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)) AS is_pet
    FROM spells_new s LEFT JOIN db_str d ON d.id = s.descnum AND d.type = 6
    WHERE s.classes8 BETWEEN 1 AND 100
      AND s.id BETWEEN 43300 AND 43349
    ORDER BY s.classes8, s.id});

# ---------------------------------------------------------------- blacklist
# Categories of native spell that must never be offered as a level-up reward. Every SPA number
# below was read out of THIS database rather than from memory (e.g. SELECT effectid1 FROM
# spells_new WHERE name='Gate') so the constants match what the server actually runs.
# "does this spell carry SPA n in ANY of its twelve effect slots".
#
# ⚠️ Written once and shared, because the hand-rolled versions of this test drifted: travel checked
# slots 1-6, rez and summon-corpse checked only 1-3, so an effect sitting in a later slot slipped
# through whichever rule happened to be narrow. Nothing here should spell the list out by hand.
sub spa_in {
    my $spa = shift;
    return "$spa IN (" . join(',', map { "effectid$_" } 1 .. 12) . ")";
}

# The PURITY test, factored out of the twelve-line form the illusion rule writes by hand: every
# populated slot must be one of @ok, with 254 (empty) always allowed.
#
# ⚠️ This asks "does the spell do anything BESIDES the listed effects", which is the shape section 5
# insists on -- prune the spell that does ONLY the junk thing, so a cosmetic or utility effect riding
# along on something real keeps the spell in the pool.
# ⚠️ It does NOT understand the engine's other "blank" slots -- a 10 spacer (base 0, formula 100) and
# SPA 148/149, which `IsBlankSpellEffect` (common/spdat.cpp:950) also skips. Nothing needing that has
# come up yet; add them to @ok at the call site if it does.
sub only_spas {
    my $set = join(',', @_, 254);
    return join(' AND ', map { "effectid$_ IN ($set)" } 1 .. 12);
}

# "no slot pairs $spa with a POSITIVE base" -- the sign test, kept as a helper because the slot index
# and the base-value index have to move together and writing that out by hand is where it goes wrong.
# ⚠️ The `curecurse` rule already relies on this idea in the opposite direction (SPA 116 with a
# NEGATIVE base is the one that removes counters), so the sign of a base value is load bearing in both
# directions depending on the effect. Never assume it.
sub spa_no_positive {
    my $spa = shift;
    return '(' . join(' AND ', map { "NOT (effectid$_ = $spa AND effect_base_value$_ > 0)" } 1 .. 12) . ')';
}

my @RULES = (
    # ⚠️ Do NOT key travel off `teleport_zone <> ''`. That column is not "destination zone", it is
    # "names an NPC or a zone": every pet/familiar/warder/Eye-of-Zomm spell puts its SUMMON TYPE
    # there (SPA 33/71/152 pets, 106 warders, 108 familiars, 67 eye). Using it pruned 95 stock pet
    # spells -- every Magician elemental, Necromancer pet, Shaman/Beastlord warder and Enchanter
    # animation -- out of the reward pool. The travel SPAs below are the reliable signal, and no
    # travel spell lacks one.
    # ⚠️ DO NOT ADD `targettype = 3` BACK. ST_Group is 3, and that is EVERY group-target spell, not
    # just a group teleport -- it was pruning 78 ordinary group buffs (Elixir of Divinity, Wave of
    # Marr, Eriki's Psalm of Power, the group heal lines) as if they were ports. Of the 150 pool
    # spells with targettype 3, the 72 that really are travel all carry one of the SPAs below and
    # are caught by them regardless, so the clause bought nothing and cost 78 legitimate rewards.
    # ⚠️ ALL TWELVE SLOTS, not the first three or six. The effect that makes a spell a port is not
    # required to sit in an early slot, and the narrower lists here previously let travel through.
    # $ALL12 is built once below so no rule can be widened and leave another behind.
    #
    # ⚠️ The SPA list is the WHOLE travel family, verified against common/spdat.h:
    #   25 BindAffinity   26 Gate      82 SummonPC   83 Teleport
    #   88 Succor         104 Translocate           145 Teleport2
    # 82 SummonPC relocates ANOTHER player and 145 Teleport2 is the Banishment of the Pantheon /
    # Dimensional Rift / Portal to Butcher family; both were missing and both are unambiguously travel.
    ["travel",  join(' OR ', map { spa_in($_) } (25, 26, 82, 83, 88, 104, 145)),
                "ports, gates, binds, evacs, summons -- they also defeat region locking"],
    ["enchant", q{name LIKE 'Enchant %' OR name LIKE 'Mass Enchant %'},
                "tradeskill material conversion, useless as a reward"],
    # ⚠️⚠️ FEAR IS PRUNED BECAUSE NOTHING ON THIS SERVER SHOULD RUN. AoT:NPCsNeverFlee (2026-08-08)
    # stops creatures fleeing from low health, and fear is the OTHER way a mob ends up running -- it
    # enters through StartFleeing rather than Mob::CheckFlee, so the flee guard deliberately does not
    # cover it. Removing fear from the pool closes that second door on the player side, and migration
    # v43 closes it on the NPC side.
    # ⚠️ NOT purity-tested, unlike illusion/vision/sense/truenorth. Those prune only when the junk
    # effect is the ONLY thing the spell does, because the junk rides along harmlessly. Fear is
    # different: a spell that fears AND does something real still makes the target run, which is the
    # entire behaviour being removed. Presence is the correct test here.
    # 📌 SPA 23 only. SPA 102 `Fearless` is the OPPOSITE (fear immunity) and stays offerable.
    ["fear",    spa_in(23),
                "fear -- mobs running is the behaviour AoT:NPCsNeverFlee exists to remove"],
    # ⚠️ Was slots 1-3 only. A resurrect in slot 4+ was offerable.
    ["rez",     spa_in(81),
                "resurrection"],
    # True North and anything else whose only trick is pointing you north. Purity tested like the
    # illusion and vision rules, so a spell that happens to carry a compass alongside something real
    # is kept -- see the note on testing purity rather than presence.
    # Sense the Dead / Sense Summoned / Sense Animals: a detector, no combat value. Purity tested
    # like the rest -- a spell that senses something AND does something real is kept.
    # Disciplines. ⚠️ MIRRORS common/spdat.cpp IsDiscipline EXACTLY -- mana 0 AND some endurance
    # cost -- so the rule cannot drift from what the engine actually treats as a discipline. Keying it
    # off "has an endurance cost" alone would be wrong the moment a spell has both.
    #
    # They are excluded because they are NOT spells and do not behave like one: a picked discipline is
    # trained into the Combat Abilities window, not the spellbook, and autoskill cannot fire it either
    # (that only handles the ten activated specials, Client::GetAvailableAutoSkills). A reward that
    # lands somewhere the player is not looking, and that none of our systems drive, is not a reward.
    ["discipline", q{mana = 0 AND (EndurCost > 0 OR EndurUpkeep > 0)},
                "disciplines -- they train to Combat Abilities, not the spellbook"],
    ["sense",   q{(52 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   OR 53 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   OR 54 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6))
                  AND effectid1  IN (52,53,54,254) AND effectid2  IN (52,53,54,254)
                  AND effectid3  IN (52,53,54,254) AND effectid4  IN (52,53,54,254)
                  AND effectid5  IN (52,53,54,254) AND effectid6  IN (52,53,54,254)
                  AND effectid7  IN (52,53,54,254) AND effectid8  IN (52,53,54,254)
                  AND effectid9  IN (52,53,54,254) AND effectid10 IN (52,53,54,254)
                  AND effectid11 IN (52,53,54,254) AND effectid12 IN (52,53,54,254)},
                "sense dead, summoned, animals -- a detector and nothing else"],
    ["truenorth", q{56 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6,
                           effectid7,effectid8,effectid9,effectid10,effectid11,effectid12)
                    AND effectid1  IN (56,254) AND effectid2  IN (56,254)
                    AND effectid3  IN (56,254) AND effectid4  IN (56,254)
                    AND effectid5  IN (56,254) AND effectid6  IN (56,254)
                    AND effectid7  IN (56,254) AND effectid8  IN (56,254)
                    AND effectid9  IN (56,254) AND effectid10 IN (56,254)
                    AND effectid11 IN (56,254) AND effectid12 IN (56,254)},
                "True North and friends -- a compass, nothing else"],
    ["curecurse", q{(effectid1 = 116 AND effect_base_value1 < 0)
                    OR (effectid2 = 116 AND effect_base_value2 < 0)
                    OR (effectid3 = 116 AND effect_base_value3 < 0)},
                "curse-counter cures"],
    # ⚠️ targettype 34 ALONE MISSES HALF THE FAMILY. It catches the Iony's and Reebo's lines, which
    # target a dungeon object, but the Wuggan's and Xalirilan's lines are cast on a PLAYER
    # (targettype 5) and sailed straight through -- 18 of the 36 were still offerable. Key off the
    # SPAs instead: 164 AppraiseLDonChest, 165 DisarmLDoNTrap, 166 UnlockLDoNChest. Every member of
    # the family carries one, whatever it targets.
    ["ldon",    join(' OR ', 'targettype = 34', map { spa_in($_) } (164, 165, 166)),
                "LDoN dungeon-object appraise/disarm/unlock"],
    # ⚠️ 91 SummonCorpse and 77 LocateCorpse are one rule: both are corpse admin, and 77 is the whole
    # of Locate Corpse, Track Corpse and Lyssa's Locating Lyric (all three pure). Death here drops you
    # at your bind with your corpse behind you, so finding it is not the puzzle it is on live -- and
    # spending one of three level-up slots on a spell that points at it is a wasted level.
    ["corpse",  join(' OR ', map { spa_in($_) } (91, 77)),
                "summon or locate a corpse"],
    # ---------------------------------------------------------------- 2026-08-13 removals
    # Seven junk families, all confirmed against the pool before being written (counts in the block
    # comment at the end of this list). Each is keyed off its SPA rather than a name, so any other
    # member of the family goes with it -- section 5's rule, and the reason `ldon` stopped keying off
    # targettype.
    #
    # ⚠️⚠️ BLIND IS THE ONLY ONE THAT NEEDED CARE, AND IT HAD A LIVE TRAP IN IT: SPA 20 WITH A
    # *POSITIVE* BASE IS **CURE** BLINDNESS (212, base +1). A naive purity test prunes it -- it is a
    # single pure SPA 20 slot and looks exactly like Sunbeam. Hence `spa_no_positive`.
    # ⚠️ SPA 2 (ArmorClass) is tolerated because Flash of Light and Blinding Luminance carry it at base
    # **-5** -- a trivial AC *debuff* that is part of the blind, not a benefit. It is tolerated rather
    # than ignored generally: a POSITIVE SPA 2 is a real AC buff and must never be swept up, which is
    # why the sign guard is on 20 and the tolerance list is this narrow.
    # ⚠️ It deliberately KEEPS `1545 The Unspoken Word`, which blinds AND carries SPA 79 for -605 -- a
    # real nuke with a blind rider is not a blind spell.
    # 📌 Two spells the owner did not name are caught, and that is the SPA-not-name rule working:
    # `297 Eye of Confusion` is Sunbeam, and `134 Blinding Luminance` is Flash of Light.
    ["blind",   spa_in(20) . ' AND ' . only_spas(20, 2) . ' AND ' . spa_no_positive(20),
                "inflicts blindness and nothing of value"],
    # ⚠️⚠️ SPA 76 IS NOT IMPLEMENTED -- `common/spdat.h:1139` says so outright ("just seems to send a
    # message"), so Sentinel and the CLASSIC `2501 Sanctuary` do literally nothing. That is the
    # `identify` argument in its purest form: a reward slot spent on a no-op.
    # ⚠️ Not to be confused with the modern Sanctuary (5912, SPA 312), which is real -- and is already
    # outside the pool on `classes8 = 254`.
    ["sentinel", spa_in(76),
                "proximity alarm -- SPA 76 is unimplemented, it only prints a message"],
    # A scouting eye: a pet you see through. Catches `323 Eye of Zomm` and its higher-level twin
    # `1720 Eye of Tallon`.
    # ⚠️ Section 5 already records SPA 67 spells storing their summon type in `teleport_zone` -- that
    # is why the TRAVEL rule must not key off that column, and is unrelated to pruning them here.
    ["eye",     spa_in(67),
                "summon a remote-vision eye"],
    # Destroys your own pet to refund a little mana. Useful to a Magician who knows what it is for,
    # and indistinguishable from a trap as one of three offered rewards.
    ["reclaimpet", spa_in(68),
                "destroys your pet for mana"],
    # Speak through your pet. No combat use at all.
    ["voicegraft", spa_in(75),
                "talk through your pet"],
    # ⚠️⚠️ SPA 24 `Stamina` IS **NOT** INERT -- do not reason from it sitting in the big
    # no-direct-action fall-through in `SpellEffect()` (spell_effects.cpp:3233). It has real handlers
    # in `bonuses.cpp` (:2046, :4355), exactly like STR/AC/ResistAll which share that block, so this
    # prunes a working spell rather than a no-op. It is here because the owner asked for `310 Flare`
    # and that is the ONLY spell in the pool carrying it.
    # ⚠️⚠️ THAT COUNT OF ONE IS WHAT MAKES A BLANKET SPA RULE SAFE HERE. If it ever rises, re-check
    # before trusting this: an Invigor-style stamina restore would be a genuinely useful reward on a
    # server where specials cost endurance (section 22), and this rule would silently eat it.
    ["stamina", spa_in(24),
                "Flare -- the pool's only SPA 24"],
    # SPA 61 SE_Identify prints an item's stats in chat. Harmless, and completely pointless as a
    # level-up reward: the client already shows every stat on the item's own display, so the spell
    # answers a question nobody has. Spending one of three reward slots on it is a wasted level.
    # ⚠️ Keyed off the SPA, not the name, so any other member of the family goes with it -- the same
    # reason the LDoN rule above stopped keying off targettype.
    ["identify", spa_in(61),
                "identify an item -- the client already shows its stats"],
    # SPA 32 SummonItem conjures an ITEM. That is one rule covering three things the reward picker
    # should never offer: Magician summons (weapons, bags, jewellery), the Enchanter "Enchant <metal>"
    # and "Mass Enchant <metal>" tradeskill lines, and the focus-essence junk. The older name-based
    # enchant rule above still runs first so those keep their own label in the counts.
    ["summonitem", q{32 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)},
                "conjures an item -- mage summons, enchant-metal, focus essences"],
    # ⚠️ ONLY PURELY COSMETIC illusions. Many SPA 58 spells carry something genuinely worth having
    # alongside the model change -- Boon of the Garou and Night's Dark Terror add a weapon proc
    # (SPA 85), the wolf/bear forms add stats and ultravision, Illusion: Fire Elemental is a damage
    # shield, Illusion: Air Elemental is levitate plus a stat. Pruning on "has SPA 58" threw all 31
    # of those away with the 23 that really are just a costume. So the test is that EVERY populated
    # slot is either the illusion itself or empty; anything with a second real effect stays.
    # All TWELVE slots are checked -- spells_new has effectid1..12, and the older rules above only
    # look at the first six.
    ["illusion", q{58 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   AND effectid1  IN (58,254) AND effectid2  IN (58,254) AND effectid3  IN (58,254)
                   AND effectid4  IN (58,254) AND effectid5  IN (58,254) AND effectid6  IN (58,254)
                   AND effectid7  IN (58,254) AND effectid8  IN (58,254) AND effectid9  IN (58,254)
                   AND effectid10 IN (58,254) AND effectid11 IN (58,254) AND effectid12 IN (58,254)},
                "illusions with no other effect -- pure costume"],
    # ⚠️⚠️ A COSTUME THAT ONLY DISABLES YOU STILL PASSES THE PURITY TEST ABOVE. Root (99) and
    # MovementSpeed (3) are real effects, so an illusion carrying one is "not pure" and survives --
    # but the question the purity rule is actually asking is "does this do anything for you besides
    # change how you look", and the answer here is "no, it does something TO you". Both offenders are
    # targettype 6 (Self) with a 360 tick duration, so as a level-up reward they are strictly worse
    # than nothing:
    #     287 Minor Illusion    lvl 1    MovementSpeed -7000  -- immobile for an hour
    #     601 Illusion: Tree    lvl 15   Root, and NOTHING else
    # ⚠️ Deliberately narrow: only a NEGATIVE MovementSpeed counts, and the spell must have no other
    # rider at all. That is what keeps the classic druid regen forms -- Treeform (258), Spirit of Oak
    # (1564) and Spirit of Ash (3580) all self-root too, but they pay for it with HP/mana regen, which
    # is a real tradeoff rather than a pure penalty. A positive MovementSpeed is the wolf-form run
    # buff and must never be caught here.
    ["rootform", q{58 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   AND targettype = 6 AND }
                 . join(' AND ', map {
                       "(effectid$_ IN (58,99,254) OR (effectid$_ = 3 AND effect_base_value$_ < 0))"
                   } 1 .. 12),
                "costume forms whose only extra effect roots or immobilises the caster"],
    # SeeInvis 13, InfraVision 65, UltraVision 66, MagnifyVision 87 (the "Glimpse" telescope line).
    # Same purity test as illusions, and for the same reason: the wolf and hunter forms carry
    # ultravision (SPA 66) ALONGSIDE their stats, so a bare "has a vision SPA" test caught every one
    # of them the moment the illusion rule stopped doing it first. Only prune a spell whose every
    # populated slot is a vision effect or empty.
    ["vision",  q{(13 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   OR 65 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   OR 66 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6)
                   OR 87 IN (effectid1,effectid2,effectid3,effectid4,effectid5,effectid6))
                  AND effectid1  IN (13,65,66,87,254) AND effectid2  IN (13,65,66,87,254)
                  AND effectid3  IN (13,65,66,87,254) AND effectid4  IN (13,65,66,87,254)
                  AND effectid5  IN (13,65,66,87,254) AND effectid6  IN (13,65,66,87,254)
                  AND effectid7  IN (13,65,66,87,254) AND effectid8  IN (13,65,66,87,254)
                  AND effectid9  IN (13,65,66,87,254) AND effectid10 IN (13,65,66,87,254)
                  AND effectid11 IN (13,65,66,87,254) AND effectid12 IN (13,65,66,87,254)},
                "sight only -- see invis, infra, ultra, telescope, with nothing else attached"],
);

my (%black, %black_why, %cat_n);
for my $r (@RULES) {
    my ($cat, $where, $note) = @$r;
    for my $row (mysql("SELECT id FROM spells_new WHERE id < 10000 AND classes8 BETWEEN 1 AND 100 AND ($where)")) {
        my $id = $row->[0];
        $cat_n{$cat}++ unless $black{$id};      # count each spell once, under its first matching rule
        next if $black{$id};
        $black{$id}     = 1;
        $black_why{$id} = $cat;
    }
}

# ---------------------------------------------------------------- pet wards
# ⚠️⚠️ EVERY PET CARRIES A STANDING WARD (zone/aotv4_pet_aa.cpp) AND NOTHING TOLD THE PLAYER.
# A summon spell's stock description talks only about the pet; the ward its family gets -- a damage
# shield, regeneration, a lifetap -- was invisible until you inspected the pet's buffs. Picking a pet
# spell in the reward window is exactly when that matters, so the ward is appended to the description.
#
# ⚠️ `teleport_zone` IS THE PET TYPE, NOT A ZONE (CLAUDE.md section 5). Every pet, familiar, warder
# and Eye of Zomm spell stores its summon type there, which is the same string pets.type carries and
# the same string the C++ matches on.
#
# ⚠️⚠️ THIS TABLE MIRRORS `kPetWards` IN zone/aotv4_pet_aa.cpp AND MUST BE KEPT IN STEP WITH IT.
# Matching is by PREFIX for the same reason it is there: SumFireR10 through SumFireR37 are all
# "SumFire". Anything unmatched falls through to the generic ward, so the text is never wrong, only
# less specific -- which is also true of the server behaviour it describes.
my @PET_WARDS = (
    [ 'SumFire',     'a damage shield'            ],
    [ 'SumWater',    'health regeneration'        ],
    [ 'SumEarth',    'increased maximum health'   ],
    [ 'SumAir',      'increased attack speed'     ],
    [ 'skel_pet',    'a melee lifetap'            ],
    [ 'animateDead', 'a melee lifetap'            ],
    [ 'blood_skel',  'a melee lifetap'            ],
    [ 'BLpet',       'increased melee damage'     ],
    [ 'Animation',   'an absorbing rune'          ],
    [ 'SpiritWolf',  'improved avoidance'         ],
    [ 'Familiar',    'mana regeneration'          ],
    [ 'CasterWolf',  'mana regeneration'          ],
);
my $PET_WARD_FALLBACK = 'increased maximum health';

# ⚠️⚠️ GATED ON A PET SPA, NOT ON `teleport_zone` BEING NON-EMPTY. That column names "an NPC OR a
# ZONE", so every TRAVEL spell fills it in with a destination -- keying off it alone tagged 328
# spells with a ward line, including ports and gates, when there are only about a hundred real pet
# spells. This is the same trap section 5 records for the travel blacklist rule, arrived at from the
# opposite direction: there `teleport_zone` wrongly caught pets, here it wrongly caught travel.
# The SPAs are the reliable signal: 33/71/152 pets, 106 warders, 108 familiars.
sub pet_ward_text {
    my ($pet_type, $is_pet) = @_;
    return '' unless $is_pet;
    return '' unless defined $pet_type && length $pet_type;
    for my $w (@PET_WARDS) {
        my ($prefix, $text) = @$w;
        return $text if lc(substr($pet_type, 0, length $prefix)) eq lc($prefix);
    }
    return $PET_WARD_FALLBACK;
}

my (%pool, %icons, %descs, $n_stock, $n_custom, $n_ward);
for my $r (@stock, @custom) {
    my ($id, $name, $lvl, $icon, $desc, $pet_type, $is_pet) = @$r;
    push @{ $pool{$lvl} }, { id => $id, name => $name };
    $icons{$id} = $icon if $icon && $icon > 0;
    my $d = sanitize_desc($desc);

    # ⚠️ Appended AFTER sanitize_desc, not before: that function spells out '%' because the client
    # treats it as a printf token (section 14). The ward text carries no '%' today, but appending
    # afterwards means a future edit to it cannot reintroduce one behind the sanitiser's back.
    my $ward = pet_ward_text($pet_type, $is_pet);
    if (length $ward) {
        $d .= ' ' if length $d;
        $d .= "Your pet gains $ward for as long as it lives.";
        $n_ward++;
    }

    $descs{$id} = $d if length $d;
    $id >= 43000 ? $n_custom++ : $n_stock++;
}

# ---------------------------------------------------------------- spell_pool.lua
open(my $p, '>', "$DIR/spell_pool.lua") or die $!;
print $p <<'HDR';
-- AUTO-GENERATED by .devcontainer/custom/spells/gen_stock_pool.pl -- do not hand-edit.
-- pool[learn_level] = { {id=, name=}, ... }
--
-- Drawn from the STOCK EQ spell set (id < 10000, indexed by classes8 = lowest learn level), which
-- is the original setup: the 113-spell custom set that briefly replaced it was mostly redundant
-- with native spells. The UNIQUE customs are kept and appear here alongside the stock ones -- the
-- Lua-driven abilities (quests/global/spells/43xxx.lua) that no native spell replicates.
--
-- Runtime filters still apply on top of this (spell_choice.gather_candidates): spell_blacklist.lua,
-- already-known, and the "Enchant %s" name reject.
return {
HDR
for my $lvl (sort { $a <=> $b } keys %pool) {
    my @e = map { sprintf('{id=%d,name="%s"}', $_->{id}, q_lua($_->{name})) } @{ $pool{$lvl} };
    print $p "\t[$lvl]={ " . join(", ", @e) . " },\n";
}
print $p "}\n";
close $p;

# ---------------------------------------------------------------- spell_icons.lua
open(my $i, '>', "$DIR/spell_icons.lua") or die $!;
print $i "-- AUTO-GENERATED by .devcontainer/custom/spells/gen_stock_pool.pl -- do not hand-edit.\n";
print $i "-- id -> spellbook icon index, for the level-up reward window.\n";
print $i "return {\n";
print $i "\t[$_]=$icons{$_},\n" for sort { $a <=> $b } keys %icons;
print $i "}\n";
close $i;

# ---------------------------------------------------------------- spell_desc.lua
open(my $d, '>', "$DIR/spell_desc.lua") or die $!;
print $d "-- AUTO-GENERATED by .devcontainer/custom/spells/gen_stock_pool.pl -- do not hand-edit.\n";
print $d "-- id -> description (db_str type 6), sent to the picker as SPELLDESCDATA.\n";
print $d "-- Literal percent signs are spelled out: Client::Message is printf-style, so a stray %\n";
print $d "-- is eaten as a format token and renders as garbage.\n";
print $d "return {\n";
printf $d "\t[%d]=\"%s\",\n", $_, q_lua($descs{$_}) for sort { $a <=> $b } keys %descs;
print $d "}\n";
close $d;

# ---------------------------------------------------------------- spell_blacklist.lua
open(my $bl, '>', "$DIR/spell_blacklist.lua") or die $!;
print $bl "-- AUTO-GENERATED by .devcontainer/custom/spells/gen_stock_pool.pl -- do not hand-edit.\n";
print $bl "-- { [id]=true } -- spells that are NEVER offered as a level-up reward. Applied at runtime\n";
print $bl "-- in spell_choice.gather_candidates, so the pool itself still lists them.\n--\n";
printf $bl "-- %-10s %5d  %s\n", $_->[0], ($cat_n{$_->[0]} // 0), $_->[2] for @RULES;
print $bl "return {\n";
printf $bl "\t[%d]=true,  -- %s\n", $_, $black_why{$_} for sort { $a <=> $b } keys %black;
print $bl "}\n";
close $bl;

printf "regenerated from the stock spell set:\n";
printf "  spell_pool.lua   %d levels, %d stock + %d unique custom = %d spells\n",
       scalar(keys %pool), $n_stock, $n_custom, $n_stock + $n_custom;
printf "  spell_icons.lua  %d icons\n", scalar(keys %icons);
printf "  spell_desc.lua   %d descriptions (%d with a pet ward line)\n", scalar(keys %descs), $n_ward;
printf "  spell_blacklist.lua %d pruned:\n", scalar(keys %black);
printf "      %-10s %5d  %s\n", $_->[0], ($cat_n{$_->[0]} // 0), $_->[2] for @RULES;
printf "  => %d spells actually offerable\n",
       $n_stock + $n_custom - scalar(grep { $black{$_} } keys %black);
