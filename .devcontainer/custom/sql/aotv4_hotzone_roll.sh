#!/bin/bash
# aotv4_hotzone_roll.sh -- pick the day's rotating hot zones and publish them.
#
# COUNT scales with the unlocked era (global bucket aotv4_era, 0..8): 5 + era hot zones.
#   Classic(0)=5  Kunark(1)=6  Velious(2)=7  Luclin(3)=8  PoP(4)=9 ...  OoW(8)=13.
# One hot zone per 10-level band up to the era's level cap (1-10 .. cap); any EXTRA zones (when the
# count exceeds the number of bands, e.g. same-cap expansions) are placed in the highest bands
# (endgame). A zone is only eligible once its expansion is unlocked (pool.expansion <= era).
#
# Within each band the zone is chosen weighted INVERSELY to its last-24h population (distinct chars that
# zoned in, from player_event_logs event 2), so emptier zones are far likelier. The single LEAST-
# populated pick gets 3x ZEM, the rest 2x -- "2x/3x" = the zone's base ZEM doubled / tripled.
#
# Publishes to aotv4_hotzone_current and rewrites the MOTD (variables.MOTD, read live per login).
# Run daily at 00:00 Pacific by aotv4_hotzone_daemon.sh.  Keep the CAPS/NAMES tables in step with
# era_system.lua.

set -uo pipefail
Q() { mysql -h127.0.0.1 -upeq -ppeqpass peq -N -e "$1"; }
X() { mysql -h127.0.0.1 -upeq -ppeqpass peq      -e "$1"; }

# era -> level cap and label (must match era_system.lua M.ERA)
CAPS=(50 60 60 60 65 65 65 65 70)
NAMES=("Classic" "Kunark" "Velious" "Luclin" "Planes of Power" "Legacy of Ykesha" "Lost Dungeons of Norrath" "Gates of Discord" "Omens of War")

ERA=$(Q "SELECT value FROM data_buckets WHERE \`key\`='aotv4_era' LIMIT 1;")
case "$ERA" in ''|*[!0-9]*) ERA=0 ;; esac
[ "$ERA" -gt 8 ] && ERA=8
CAP=${CAPS[$ERA]}
ERA_NAME=${NAMES[$ERA]}
NUM_ZONES=$(( 5 + ERA ))
NUM_BANDS=$(( (CAP + 9) / 10 ))

# last-24h popularity join (distinct chars that zoned INTO each zone)
POP="LEFT JOIN (
       SELECT CAST(JSON_EXTRACT(event_data,'\$.to_zone_id') AS UNSIGNED) AS zid,
              COUNT(DISTINCT character_id) AS players
       FROM player_event_logs
       WHERE event_type_id = 2 AND created_at > NOW() - INTERVAL 24 HOUR
       GROUP BY zid
     ) pop ON pop.zid = p.zone_id"

picked=""   # csv of already-picked zone ids (kept distinct)
pick_from_band() {   # $1 = band -> "zone_id<TAB>players" or empty
  local band=$1 notin=""
  [ -n "$picked" ] && notin="AND p.zone_id NOT IN ($picked)"
  Q "SELECT p.zone_id, COALESCE(pop.players,0)
     FROM aotv4_zone_pool p $POP
     WHERE p.bucket = $band AND p.expansion <= $ERA $notin
     ORDER BY POW(RAND(), COALESCE(pop.players,0) + 1) DESC
     LIMIT 1;"
}

# 1) restore yesterday's picks to base ZEM, then clear
X "UPDATE zone z
     JOIN aotv4_hotzone_current c ON c.zone_id = z.zoneidnumber AND z.version = 0
     JOIN aotv4_zone_zem_base   b ON b.zone_id = z.zoneidnumber
     SET z.zone_exp_multiplier = b.base_zem;"
X "DELETE FROM aotv4_hotzone_current;"

# 2) pick zones: one per band, then extras from the highest bands downward
declare -a PB PZ PP; i=0
add_pick() { PB[$i]=$1; PZ[$i]=$2; PP[$i]=$3; picked="${picked:+$picked,}$2"; i=$((i+1)); }

for band in $(seq 1 "$NUM_BANDS"); do
  row=$(pick_from_band "$band"); [ -z "$row" ] && continue
  add_pick "$band" "$(echo "$row" | awk '{print $1}')" "$(echo "$row" | awk '{print $2}')"
done
extras=$(( NUM_ZONES - NUM_BANDS ))
for ((e=0; e<extras; e++)); do
  for band in $(seq "$NUM_BANDS" -1 1); do
    row=$(pick_from_band "$band"); [ -z "$row" ] && continue
    add_pick "$band" "$(echo "$row" | awk '{print $1}')" "$(echo "$row" | awk '{print $2}')"
    break
  done
done

# 3) least-populated pick gets 3x
min_players=-1; min_idx=0
for j in "${!PB[@]}"; do
  if [ "$min_players" -lt 0 ] || [ "${PP[$j]}" -lt "$min_players" ]; then min_players=${PP[$j]}; min_idx=$j; fi
done

# 4) apply ZEM + record picks
pick_no=0
for j in "${!PB[@]}"; do
  pick_no=$(( pick_no + 1 )); band=${PB[$j]}; zid=${PZ[$j]}; pl=${PP[$j]}
  if [ "$j" -eq "$min_idx" ]; then mult=3; else mult=2; fi
  lo=$(( (band-1)*10 + 1 )); hi=$(( band*10 )); [ "$hi" -gt "$CAP" ] && hi=$CAP
  short=$(Q "SELECT short_name FROM zone WHERE zoneidnumber=$zid AND version=0 LIMIT 1;")
  long=$(Q  "SELECT long_name  FROM zone WHERE zoneidnumber=$zid AND version=0 LIMIT 1;")
  base=$(Q  "SELECT base_zem   FROM aotv4_zone_zem_base WHERE zone_id=$zid LIMIT 1;")
  newzem=$(awk -v a="$base" -v m="$mult" 'BEGIN{printf "%.2f", a*m}')
  X "UPDATE zone SET zone_exp_multiplier = $newzem WHERE zoneidnumber = $zid AND version = 0;"
  X "INSERT INTO aotv4_hotzone_current
       (pick_no,bucket,zone_id,short_name,long_name,level_range,multiplier,base_zem,new_zem,distinct_players,picked_at)
       VALUES ($pick_no,$band,$zid,'$(echo "$short"|sed "s/'/''/g")','$(echo "$long"|sed "s/'/''/g")',
               '${lo}-${hi}',$mult,$base,$newzem,$pl,NOW());"
done

# 5) the hot zones are shown via the per-line login welcome (global_player event_connect + the bucket
# published below), NOT the MOTD -- the RoF2 MOTD control can't render line breaks. Keep the MOTD blank.
X "UPDATE variables SET value = '', ts = NOW() WHERE varname = 'MOTD';"

PDAY=$(TZ=America/Los_Angeles date +%F)
X "INSERT INTO variables (varname, value, information, ts)
     VALUES ('aotv4_hotzone_day', '${PDAY}', 'AoTv4 hotzone last roll date (Pacific)', NOW())
     ON DUPLICATE KEY UPDATE value = VALUES(value), ts = NOW();"

# publish the list to a GLOBAL data bucket (all scope cols 0 -> read LIVE, never cached) so the
# server can print a per-line hot-zone welcome on login. Format: longname|mult^longname|mult^...
HZLIST=""
while IFS=$'\t' read -r longn mult; do HZLIST+="${longn}|${mult%.*}^"; done \
  < <(Q "SELECT long_name, multiplier FROM aotv4_hotzone_current ORDER BY multiplier, bucket;")
HZLIST=${HZLIST%^}
HZ_SQL=$(printf '%s' "$HZLIST" | sed "s/'/''/g")
X "DELETE FROM data_buckets WHERE \`key\`='aotv4_hotzone_list'
     AND character_id=0 AND account_id=0 AND npc_id=0 AND bot_id=0 AND zone_id=0 AND instance_id=0;
   INSERT INTO data_buckets (\`key\`,value,expires,account_id,character_id,npc_id,bot_id,zone_id,instance_id)
     VALUES ('aotv4_hotzone_list','${HZ_SQL}',0,0,0,0,0,0,0);"

echo "===== hot zones for today (${ERA_NAME} era, ${NUM_ZONES} zones) ====="
X "SELECT pick_no, level_range AS lvl, long_name AS zone, CONCAT(multiplier,'x') AS mult,
          base_zem, new_zem, distinct_players AS pop_24h
   FROM aotv4_hotzone_current ORDER BY pick_no;"
echo "(MOTD cleared; hot zones are shown via the login welcome message)"
