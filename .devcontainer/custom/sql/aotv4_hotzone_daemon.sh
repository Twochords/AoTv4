#!/bin/bash
# aotv4_hotzone_daemon.sh -- fires the hot-zone roll every day at 00:00 America/Los_Angeles (Pacific,
# DST-aware). Self-contained scheduler (no cron dependency): sleeps until the next Pacific midnight,
# rolls, repeats. On startup it catches up if a day was missed while the server was down.
#
# Start detached alongside the server stack (survives shell timeouts):
#   setsid nohup /src/.devcontainer/custom/sql/aotv4_hotzone_daemon.sh \
#          > /src/build/bin/logs/hotzone_daemon.log 2>&1 < /dev/null &
# Keep exactly one running:  pgrep -f aotv4_hotzone_daemon

export TZ=America/Los_Angeles
ROLL=/src/.devcontainer/custom/sql/aotv4_hotzone_roll.sh
LOG=/src/build/bin/logs/hotzone_roll.log
Q() { mysql -h127.0.0.1 -upeq -ppeqpass peq -N -e "$1" 2>/dev/null; }

log() { echo "[$(date '+%F %T %Z')] $*"; }

# catch-up: run once now if today's Pacific roll hasn't happened yet (fresh install or missed a day)
today=$(date +%F)
last=$(Q "SELECT value FROM variables WHERE varname='aotv4_hotzone_day';")
if [ "$last" != "$today" ]; then
  log "catch-up roll (last=[$last] today=[$today])"
  bash "$ROLL" >> "$LOG" 2>&1 && log "catch-up roll done" || log "catch-up roll FAILED"
else
  log "today's roll already done ($today); waiting for next midnight"
fi

while true; do
  now=$(date +%s)
  next=$(date -d 'tomorrow 00:00' +%s)   # next Pacific midnight (TZ set above)
  secs=$(( next - now ))
  [ "$secs" -lt 1 ] && secs=1
  log "sleeping ${secs}s until next Pacific midnight ($(date -d @"$next" '+%F %T %Z'))"
  sleep "$secs"
  log "midnight -- rolling hot zones"
  bash "$ROLL" >> "$LOG" 2>&1 && log "roll done" || log "roll FAILED"
done
