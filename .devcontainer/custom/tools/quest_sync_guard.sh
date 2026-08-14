#!/usr/bin/env bash
# quest_sync_guard.sh -- REFUSE a fork->live quest sync that would REVERT live-only fixes.
#
# THE BUG THIS PREVENTS (2026-08-10): a blind `rsync fork -> live` overwrote SIX quest modules with
# fork versions that were BEHIND live -- the dev's "fold in live-only fixes" reconciliation had missed
# the 2026-08-06 delve fixes (MAX_BUMP_FRAC scaling clamp, Onslaught mode offsets, BOSS_HP_MULT retune)
# and others. Result: delves scaled wrong, gave the wrong affix, and the warder stopped spawning.
#
# The tell was always visible BEFORE the sync: for every regressed file, LIVE was LARGER than the fork
# and carried newer dated markers. This script surfaces exactly that and blocks the sync until reviewed.
#
# Usage:  quest_sync_guard.sh <fork_quests_dir> <live_quests_dir>
#   exit 0 = safe (fork is same-or-ahead everywhere);  exit 1 = REVIEW REQUIRED (live ahead somewhere).
#
# It does NOT sync. Run it, read the report, and only then rsync -- and only the files it cleared.
set -u
FORK="${1:?fork quests dir}"; LIVE="${2:?live quests dir}"
today_year=2026
suspect=0; changed=0

# newest YYYY-MM-DD marker in a file (0 if none)
newest_marker(){ grep -hoE "${today_year}-[01][0-9]-[0-3][0-9]" "$1" 2>/dev/null | sort | tail -1 | tr -d '-'; }

while IFS= read -r -d '' ff; do
  rel="${ff#"$FORK"/}"
  lf="$LIVE/$rel"
  [ -f "$lf" ] || continue                       # new file, nothing to revert
  cmp -s "$ff" "$lf" && continue                  # identical
  changed=$((changed+1))
  fl=$(wc -l < "$ff"); ll=$(wc -l < "$lf")
  fm=$(newest_marker "$ff"); lm=$(newest_marker "$lf"); fm=${fm:-0}; lm=${lm:-0}
  reason=""
  [ "$ll" -gt "$fl" ]  && reason="LIVE LARGER ($ll>$fl)"
  [ "$lm" -gt "$fm" ]  && reason="${reason:+$reason; }LIVE has NEWER marker ($lm>$fm)"
  if [ -n "$reason" ]; then
    suspect=$((suspect+1))
    printf '  ⚠️  %-46s %s\n' "$rel" "$reason"
  else
    printf '  ok  %-46s fork ahead/equal (F=%s L=%s)\n' "$rel" "$fl" "$ll"
  fi
done < <(find "$FORK" -name '*.lua' -type f -print0)

echo "---"
echo "changed files: $changed | SUSPECT (live may be ahead): $suspect"
if [ "$suspect" -gt 0 ]; then
  echo "❌ REVIEW REQUIRED: the fork would REVERT live-only work on the ⚠️ files above."
  echo "   For each, diff live-vs-fork, decide, and only sync files that are genuinely fork-ahead."
  echo "   See LIVE_ONLY_FIXES.md for the registry of hotfixes that must never be reverted."
  exit 1
fi
echo "✅ safe: fork is same-or-ahead on every changed file."
exit 0
