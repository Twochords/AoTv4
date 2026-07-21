#!/usr/bin/env bash
# AoTv4 post-rebuild recovery — one command to get the server up.
# Idempotent: seeds the peq DB only if the (now persistent) volume is empty, then
# rebuilds shared memory and starts the 5-process stack. Safe to re-run.
#
# See POST_REBUILD_RECOVERY.md / CLAUDE.md §2. Run from anywhere:
#     bash /src/.devcontainer/recover.sh
set -uo pipefail

BIN=/src/build/bin
SNAP=/src/aotv4_current.sql
DBPASS=peqpass

say(){ printf '\n=== %s ===\n' "$*"; }

say "1. Start MariaDB"
sudo service mariadb start
for i in $(seq 1 10); do
  (ss -ltn 2>/dev/null || netstat -ltn) | grep -q ':3306' && break
  sleep 1
done
(ss -ltn 2>/dev/null || netstat -ltn) | grep -q ':3306' && echo "3306 UP" || { echo "MariaDB DOWN — abort"; exit 1; }

say "2/3. Seed peq DB + grants (only if empty)"
TABLES=$(sudo mariadb -N -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='peq';" 2>/dev/null)
if [ "${TABLES:-0}" -lt 100 ]; then
  echo "peq has ${TABLES:-0} tables — seeding from $SNAP ..."
  [ -f "$SNAP" ] || { echo "SNAPSHOT MISSING: $SNAP — cannot seed. Copy it onto /src and re-run."; exit 1; }
  pv -f "$SNAP" 2>/dev/null | sudo mariadb || { echo "import failed"; exit 1; }
else
  echo "peq already has $TABLES tables (persistent volume intact) — skipping import."
fi
sudo mariadb -e "CREATE USER IF NOT EXISTS 'peq'@'127.0.0.1' IDENTIFIED BY '$DBPASS';
                 GRANT ALL PRIVILEGES ON *.* TO 'peq'@'127.0.0.1'; FLUSH PRIVILEGES;"

say "4. Verify (expect chars>=1, hallowed/mythic>0, spelldmg_rule=true)"
mysql -h127.0.0.1 -upeq -p"$DBPASS" peq -N -e "
  SELECT 'chars', COUNT(*) FROM character_data
  UNION ALL SELECT 'hallowed', COUNT(*) FROM items WHERE id BETWEEN 300000 AND 599999
  UNION ALL SELECT 'mythic',   COUNT(*) FROM items WHERE id BETWEEN 600000 AND 899999
  UNION ALL SELECT 'spelldmg_rule', rule_value FROM rule_values WHERE rule_name='Spells:IgnoreSpellDmgLvlRestriction'
  UNION ALL SELECT 'aa_bard_tagged', COUNT(*) FROM aa_ability WHERE classes & 128;" || { echo "DB verify failed"; exit 1; }

say "5. Rebuild shared memory (world must be down — nothing started yet)"
( cd "$BIN" && ./shared_memory 2>&1 | tail -3 )

say "6. Start the stack (detached)"
cd "$BIN"
for p in loginserver world eqlaunch ucs queryserv; do pkill -x "$p" 2>/dev/null; done
sleep 1
setsid nohup ./loginserver > logs/loginserver_manual.log 2>&1 < /dev/null &
sleep 2
setsid nohup ./world       > logs/world_manual.log       2>&1 < /dev/null &
sleep 6
setsid nohup ./eqlaunch zone > logs/eqlaunch_manual.log  2>&1 < /dev/null &
setsid nohup ./ucs         > logs/ucs_manual.log         2>&1 < /dev/null &
setsid nohup ./queryserv   > logs/queryserv_manual.log   2>&1 < /dev/null &
sleep 6

say "7. Health"
for p in loginserver world eqlaunch ucs queryserv; do echo "$p: $(pgrep -x "$p" | wc -l)"; done
echo "--- login UDP ports (need 5998 + 5999 bound by loginserver) ---"
(ss -lunp 2>/dev/null || netstat -lunp) | grep -E ':5998|:5999'
echo "--- world DB connect ---"
grep -iE 'Connected to (the )?database' logs/world_manual.log | tail -1
grep -iE 'fatal|Cannot continue|Access denied' logs/world_manual.log logs/loginserver_manual.log && echo "!! CHECK LOGS" || echo "no fatal errors"
echo
echo "Done. Client: account mikethefiend, eqhost Host=127.0.0.1:5999, run WINDOWED."
