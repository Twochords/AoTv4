#!/usr/bin/perl
# AoTv4 -- generate rank 2-5 rows for every damage / heal spell reachable at the level cap.
# =============================================================================================
# Emits custom/sql/aotv4_spell_ranks.sql (the rows) and lua_modules/spell_ranks.lua (base -> chain,
# for the upgrade system to read at runtime).
#
# Each rank is +10 percent output and -5 percent mana, COMPOUNDING off the base:
#     output = base * 1.10^(rank-1)      rank 5 = +46.4 percent
#     mana   = mana * 0.95^(rank-1)      rank 5 = -18.5 percent
#
# ⚠️⚠️ WHY REAL SPELL ROWS RATHER THAN A RUNTIME MULTIPLIER. A real row shows its real damage and
# real mana in the NATIVE spellbook, and the mana saving is genuine -- the client checks against the
# rank's own cost. A per-character multiplier applied in GetActSpellDamage would leave the client
# displaying base values and would make the mana discount partly cosmetic, since the client decides
# whether you may BEGIN a cast from its own copy of the cost. This became affordable only after the
# rank-variant prune freed the id space (3,951 -> 10,077).
#
# ⚠️⚠️ SCOPE IS DAMAGE AND HEALING ONLY -- SPA 0 CurrentHP, 79 CurrentHPOnce, 100 HealOverTime.
# Buffs are excluded because a buff's magnitude has NO hook: it is read straight out of the spell row
# in SpellEffect, so "+10 percent to buff stats" could not be delivered even if the rows existed.
# DoTs ARE included: a DoT is SPA 0 with a duration, so it is the same field and scales identically.
#
# ⚠️ Naming is "<name> Rk. II".."Rk. V" ON PURPOSE. gen_stock_pool.pl filters the offerable pool with
# `name NOT LIKE '% Rk. %'`, so this convention makes rank rows UNOFFERABLE by the level-up picker
# for free -- they are reachable only by upgrading. Changing the naming silently puts them in the
# reward pool.
# ⚠️ spells_new.name is varchar(64) and the longest qualifying name is 55 -> 61 with " Rk. V". Any
# future widening of the scope must re-check that; MySQL truncates silently.
#
# ⚠️ Ids are assigned CONTIGUOUSLY from RANK_ID_FIRST. 188 spells x 4 = 752 rows, against 1,424 free
# in the unbroken 43576-44999 run. Keep them contiguous: the alternative is scattering across gaps,
# which is exactly the sporadic numbering the prune was meant to clean up.
#
# ⚠️ `spells_new` IS SHARED MEMORY: world down, ./shared_memory, restart, then export_client_files
# and reinstall spells_us.txt or the client will not know the rank rows exist.

use strict;
use warnings;

my $SQL_OUT  = ".devcontainer/custom/sql/aotv4_spell_ranks.sql";
my $LUA_OUT  = ".devcontainer/repo/quests/lua_modules/spell_ranks.lua";
my $RANK_ID_FIRST = 43576;   # first id of the unbroken free run (STOCK ranks)
my $RANK_ID_LAST  = 44327;   # last STOCK rank id -- 188 spells x 4. See the band note below.

# ⚠️⚠️ CUSTOM SPELLS RANK IN THEIR OWN BAND, AND THAT IS WHAT KEEPS THE STOCK IDS STILL.
# The 752 stock rank rows at 43576-44327 are LIVE ON PLAYER CHARACTERS -- `spellrank_<charid>` stores
# the rank and `ranked_id()` resolves base -> owned rank id, so renumbering them silently changes what
# people own. Appending customs to the single contiguous run would have done exactly that if the
# selection order ever shifted. Two bands, assigned independently, cannot interfere.
# ⚠️⚠️ IT CANNOT SIMPLY CONTINUE PAST 44327: 44328-44332 are Insight, Fellowship Insignia, Fellowship
# of Health, Fellowship of Vigor and Light Campfire. 44530-44547 are the 2026-08-16 heal lines.
# 44333-44399 is the largest genuinely free run below the ceiling; 15 qualifying customs need 60.
my $CUSTOM_RANK_FIRST = 44333;
my $CUSTOM_RANK_LAST  = 44399;
# The offerable custom bands, matching gen_stock_pool.pl. Helpers/triggers sit above each (43350+,
# 44600+) and are never ranked because they are never held by a player.
my $CUSTOM_POOL_WHERE = '(id BETWEEN 43300 AND 43349 OR id BETWEEN 44530 AND 44599)';
my $MAX_RANK      = 5;
my $LEVEL_CAP     = 35;

my @HP_SPA   = (0, 79, 100);          # CurrentHP, CurrentHPOnce, HealOverTime
my $HP_SPA_IN = join(',', @HP_SPA);

sub mysql {
    my ($sql) = @_;
    open(my $h, "-|", "mysql", "-h127.0.0.1", "-upeq", "-ppeqpass", "peq", "-N", "--batch", "-e", $sql)
        or die "mysql: $!";
    my @r; while (<$h>) { chomp; push @r, [split /\t/, $_, -1] } close $h; return @r;
}
die "run from /src\n" unless -d ".devcontainer/repo/quests";

# ---------------------------------------------------------------- pick the spells
# Any slot carrying an HP-change effect qualifies. Scanning only slot 1 would miss a nuke with a
# rider effect first, and every DoT whose damage sits in a later slot.
my @slot_tests;
for my $s (1 .. 12) {
    push @slot_tests, "(effectid$s IN ($HP_SPA_IN) AND effect_base_value$s <> 0)";
}
my $qualifies = join(' OR ', @slot_tests);

my @spells = mysql(qq{
    SELECT id, name, mana
    FROM   spells_new
    WHERE  id < 10000
      AND  name NOT LIKE '% Rk. %'
      AND  classes8 BETWEEN 1 AND $LEVEL_CAP
      AND  ($qualifies)
    ORDER  BY id
});
die "no qualifying spells found\n" unless @spells;

# ⚠️⚠️ A SEPARATE QUERY, NOT AN `OR` WIDENING THE ONE ABOVE. Widening it would fold customs into the
# same ORDER BY and the same contiguous run -- and while they happen to sort last today (43312 > 5225),
# that is an accident of the id space, not a guarantee. One reordering and every stock rank id shifts.
my @custom = mysql(qq{
    SELECT id, name, mana
    FROM   spells_new
    WHERE  $CUSTOM_POOL_WHERE
      AND  name NOT LIKE '% Rk. %'
      AND  classes8 BETWEEN 1 AND $LEVEL_CAP
      AND  ($qualifies)
    ORDER  BY id
});

my $need = @spells * ($MAX_RANK - 1);
my $room = $RANK_ID_LAST - $RANK_ID_FIRST + 1;
die sprintf("need %d ids but only %d in %d-%d -- narrow the scope or pick another band\n",
            $need, $room, $RANK_ID_FIRST, $RANK_ID_LAST) if $need > $room;

my $cneed = @custom * ($MAX_RANK - 1);
my $croom = $CUSTOM_RANK_LAST - $CUSTOM_RANK_FIRST + 1;
die sprintf("custom ranks need %d ids but only %d in %d-%d\n",
            $cneed, $croom, $CUSTOM_RANK_FIRST, $CUSTOM_RANK_LAST) if $cneed > $croom;

# ⚠️ Refuse to start if the band is not actually empty. Overwriting a live spell would be silent.
# ⚠️⚠️ THE GUARD ASKS "IS ANYTHING HERE THAT IS NOT OURS", NOT "IS ANYTHING HERE". Counting rows
# outright made the script refuse to run a SECOND time -- the 752 rank rows it wrote itself looked
# like an occupied band -- so it was single-use despite its header calling it re-runnable, and the
# only way to regenerate was to delete rows by hand first. A rank row is identifiable by its name, so
# the test excludes them: our own output is replaced by the DELETE below, anything else is a live
# spell and must stop the run.
# 📌 This is also the check that would have caught the real collision: 44328-44332 and 44530-44547 are
# NOT rank rows, so had the old whole-run band still been in use, this now refuses instead of eating
# five fellowship spells and eighteen heals.
my $NOT_OURS = "name NOT LIKE '% Rk. %'";
my ($busy) = mysql("SELECT COUNT(*) FROM spells_new WHERE $NOT_OURS AND id BETWEEN $RANK_ID_FIRST AND "
                   . ($RANK_ID_FIRST + $need - 1));
my ($cbusy) = $cneed ? mysql("SELECT COUNT(*) FROM spells_new WHERE $NOT_OURS AND id BETWEEN $CUSTOM_RANK_FIRST AND "
                   . ($CUSTOM_RANK_FIRST + $cneed - 1)) : [0];
die "band $RANK_ID_FIRST..".($RANK_ID_FIRST+$need-1)." holds $busy->[0] NON-rank rows -- refusing\n"
    if $busy->[0] > 0;
die "custom band $CUSTOM_RANK_FIRST..".($CUSTOM_RANK_FIRST+$cneed-1)." holds $cbusy->[0] NON-rank rows -- refusing\n"
    if $cbusy->[0] > 0;

# ---------------------------------------------------------------- emit SQL
my @ROMAN = ('', 'I', 'II', 'III', 'IV', 'V');
open(my $out, '>', $SQL_OUT) or die "write $SQL_OUT: $!";
print $out <<"HEAD";
-- AoTv4 spell ranks -- GENERATED by custom/tools/gen_spell_ranks.pl. DO NOT HAND EDIT.
-- $need rows: ranks 2-$MAX_RANK for @{[scalar @spells]} damage/heal spells reachable at level $LEVEL_CAP.
-- Ids $RANK_ID_FIRST-@{[$RANK_ID_FIRST + $need - 1]}, contiguous.
--
-- ⚠️ Each rank is cloned VIA A TEMP TABLE so all ~236 columns stay byte-identical to the base spell.
-- Hand-listing columns is the section 5 rule; a cloned spell that silently differs in `formula` or
-- `max` is the trap that bit the Sinew line.
-- ⚠️ Named "Rk. II".."Rk. V" so gen_stock_pool.pl's `name NOT LIKE '% Rk. %'` filter keeps them OUT
-- of the level-up reward pool. They are obtainable only by upgrading.
-- ⚠️ spells_new IS SHARED MEMORY: world down, ./shared_memory, restart, re-export client files.
-- ⚠️⚠️ THE DELETE IS SCOPED TO THE TWO RANK BANDS AND MUST STAY THAT WAY. It used to read
-- `BETWEEN 43576 AND 44999` -- "the band this script owns" -- and that band has since been colonised
-- by other features: 44328-44332 (Insight, Fellowship Insignia, Fellowship of Health, Fellowship of
-- Vigor, Light Campfire) and 44530-44547 (the Mending Touch / Circle of Health / Circle of Renewal
-- heal lines). Re-running this script would have DELETED all 23 of them, silently, and the only
-- symptom would have been spells vanishing from players' books.
-- 📌 A re-runnable script's cleanup DELETE must name only the ids that script itself creates -- the
-- same rule CLAUDE.md section 5 records after aotv4_moonfire_line.sql nearly ate the Sinew line.

DELETE FROM spells_new WHERE id BETWEEN $RANK_ID_FIRST AND @{[$RANK_ID_FIRST + $need - 1]};
DELETE FROM spells_new WHERE id BETWEEN $CUSTOM_RANK_FIRST AND @{[$CUSTOM_RANK_FIRST + $cneed - 1]};

HEAD

my (%chain, $next_id);
$next_id = $RANK_ID_FIRST;

for my $sp (@spells) {
    my ($base_id, $base_name) = ($sp->[0], $sp->[1]);
    for my $rank (2 .. $MAX_RANK) {
        my $id  = $next_id++;
        my $out_mult  = 1.10 ** ($rank - 1);
        my $mana_mult = 0.95 ** ($rank - 1);
        my $name = "$base_name Rk. $ROMAN[$rank]";
        $name = substr($name, 0, 64);

        push @{ $chain{$base_id} }, $id;

        print $out "-- $base_name -> rank $rank\n";
        print $out "CREATE TEMPORARY TABLE aotv4_tmp_rank AS SELECT * FROM spells_new WHERE id = $base_id;\n";
        print $out "UPDATE aotv4_tmp_rank SET id = $id, name = " . dbq($name) . ",\n";
        print $out "    `rank` = $rank, spellgroup = $base_id,\n";
        # ⚠️ ROUND, not truncate: at 1.10 a small heal would otherwise gain nothing per rank.
        printf $out "    mana = GREATEST(0, ROUND(mana * %.6f))", $mana_mult;
        for my $s (1 .. 12) {
            # ⚠️ Scale BOTH the base value and `max`. `max` caps a level-scaling formula, so leaving
            # it alone would let the cap silently erase the rank's benefit at higher levels.
            printf $out ",\n    effect_base_value$s = IF(effectid$s IN ($HP_SPA_IN), ROUND(effect_base_value$s * %.6f), effect_base_value$s)", $out_mult;
            printf $out ",\n    max$s = IF(effectid$s IN ($HP_SPA_IN) AND max$s <> 0, ROUND(max$s * %.6f), max$s)", $out_mult;
        }
        print $out ";\n";
        print $out "INSERT INTO spells_new SELECT * FROM aotv4_tmp_rank;\n";
        print $out "DROP TEMPORARY TABLE aotv4_tmp_rank;\n\n";
    }
}

# ⚠️ Customs are emitted by the SAME loop body, only the id counter changes -- one code path so a
# custom rank can never scale differently from a stock one.
$next_id = $CUSTOM_RANK_FIRST;
for my $sp (@custom) {
    my ($base_id, $base_name) = ($sp->[0], $sp->[1]);
    for my $rank (2 .. $MAX_RANK) {
        my $id  = $next_id++;
        my $out_mult  = 1.10 ** ($rank - 1);
        my $mana_mult = 0.95 ** ($rank - 1);
        my $name = substr("$base_name Rk. $ROMAN[$rank]", 0, 64);
        push @{ $chain{$base_id} }, $id;
        print $out "-- [custom] $base_name -> rank $rank\n";
        print $out "CREATE TEMPORARY TABLE aotv4_tmp_rank AS SELECT * FROM spells_new WHERE id = $base_id;\n";
        print $out "UPDATE aotv4_tmp_rank SET id = $id, name = " . dbq($name) . ",\n";
        print $out "    `rank` = $rank, spellgroup = $base_id,\n";
        printf $out "    mana = GREATEST(0, ROUND(mana * %.6f))", $mana_mult;
        for my $s (1 .. 12) {
            printf $out ",\n    effect_base_value$s = IF(effectid$s IN ($HP_SPA_IN), ROUND(effect_base_value$s * %.6f), effect_base_value$s)", $out_mult;
            printf $out ",\n    max$s = IF(effectid$s IN ($HP_SPA_IN) AND max$s <> 0, ROUND(max$s * %.6f), max$s)", $out_mult;
        }
        print $out ";\n";
        print $out "INSERT INTO spells_new SELECT * FROM aotv4_tmp_rank;\n";
        print $out "DROP TEMPORARY TABLE aotv4_tmp_rank;\n\n";
    }
}

print $out "SELECT CONCAT('stock rank rows: ', COUNT(*)) AS x FROM spells_new WHERE id BETWEEN $RANK_ID_FIRST AND @{[$RANK_ID_FIRST + $need - 1]};\n";
print $out "SELECT CONCAT('custom rank rows: ', COUNT(*)) AS x FROM spells_new WHERE id BETWEEN $CUSTOM_RANK_FIRST AND @{[$CUSTOM_RANK_FIRST + $cneed - 1]};\n";
close $out;

sub dbq { my $s = shift; $s =~ s/\\/\\\\/g; $s =~ s/'/\\'/g; return "'$s'" }

# ---------------------------------------------------------------- emit the Lua chain map
open(my $lua, '>', $LUA_OUT) or die "write $LUA_OUT: $!";
print $lua <<'LHEAD';
-- AoTv4 spell rank chains -- GENERATED by custom/tools/gen_spell_ranks.pl. DO NOT HAND EDIT.
--
-- M.chain[base_spell_id] = { rank2_id, rank3_id, rank4_id, rank5_id }
-- M.base[any_rank_id]    = base_spell_id      (reverse lookup)
--
-- ⚠️ Upgrading is unscribe-old / scribe-new, exactly as native Rk. lines work -- a rank is a
-- DIFFERENT SPELL, not a modifier on the old one.
-- ⚠️ Regenerate together with the SQL. A chain pointing at an id the DB does not have fails silently:
-- the upgrade appears to work and the player ends up with nothing scribed.
local M = {}

LHEAD
print $lua "M.chain = {\n";
for my $base (sort { $a <=> $b } keys %chain) {
    printf $lua "  [%d] = {%s},\n", $base, join(',', @{ $chain{$base} });
}
print $lua "}\n\nM.base = {}\nfor b, ids in pairs(M.chain) do for _, id in ipairs(ids) do M.base[id] = b end end\n\nreturn M\n";
close $lua;

printf "wrote %s\n  spells: %d   rank rows: %d   ids %d-%d (%d spare in the band)\n",
    $SQL_OUT, scalar @spells, $need, $RANK_ID_FIRST, $RANK_ID_FIRST + $need - 1,
    $RANK_ID_LAST - ($RANK_ID_FIRST + $need - 1);
printf "wrote %s\n", $LUA_OUT;
