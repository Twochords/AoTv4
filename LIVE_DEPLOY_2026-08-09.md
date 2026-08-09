# Live deployment — migrations v37 → v48 + binaries

Everything below was built and verified on the **test** server (`/src`) on 2026-08-08/09.
Live is a **separate database** (§25) and is known to be behind — the enrage report proved it.

---

## 0. FIRST: find out how far behind live is

```sql
SELECT custom_version FROM db_version;
```

That number decides how much of this applies. Anything below **48** means some of the work below is
missing there. ⚠️ If it is below **18**, live has never had the enrage fix, the Bulwark Within fix, or
the zone XP normalisation — and a lot of "it's not working" reports will resolve themselves the moment
it catches up.

---

## 1. Ship the binaries

`world` and `zone` both changed. Neither is optional:

| Binary | Why it must ship |
|---|---|
| `zone` | flee/enrage guards, threat + shield rework, tradeskill gates, guard-kill credit, delve fixes |
| `world` | **character creation skills** — a new character is born at its caps and tradeskill floor |

⚠️ Several of tonight's fixes are **code guards that default to the wanted behaviour**, specifically so
they work even where a rule row is missing or a content dump has cleared one. `AoT:NPCsNeverEnrage` is
the example: the stock `NPC:LiveLikeEnrage` rule was already set correctly on test and enrage still
happened on live, because live's DB was behind. Shipping the binary fixes it regardless of DB state.

---

## 2. Apply the migrations

World applies them at boot. It needs `login.my.cnf` present or the pre-migration backup fails and
**world exits** (§15/§35):

```bash
cd <server>/build/bin
printf '[mysqldump]\nuser=peq\npassword=peqpass\nhost=127.0.0.1\n' > login.my.cnf
chmod 444 login.my.cnf     # read-only, so world cannot overwrite it with blank creds
```

⚠️ This has now bitten twice. World deletes that file after its backup; read-only is the fix.

Then start world once and confirm:

```sql
SELECT custom_version FROM db_version;   -- expect 48
```

---

## 3. ⚠️⚠️ v48 IS NOT SUFFICIENT ON ITS OWN

v48 changes **base** item data. The Hallowed and Mythic tiers are **derived**, so they must be
regenerated afterwards or every tier weapon keeps pre-nerf damage while row counts look perfect.

Run **in this order**:

```bash
mysql peq < .devcontainer/custom/sql/aotv4_gear_tiers.sql      # 1. regenerate both tiers
mysql peq < .devcontainer/custom/sql/aotv4_craft_sockets.sql   # 2. MUST follow — the regen drops tier sockets
mysql peq -e "UPDATE items SET nodrop = 0 WHERE id BETWEEN 147930 AND 147965;"   # 3. tier script sets nodrop across its scope
```

Then, with the stack **down**:

```bash
./shared_memory && <restart>
```

⚠️ `items` and `spells_new` are shared memory. Migrations alone change nothing a player can see.

---

## 4. Verify — BY RATIO, NEVER BY ROW COUNT

§35 records 3,166 Mythics once having **less AC than their own base** while every count looked healthy.

```sql
-- all four must return 0
SELECT COUNT(*) FROM items m JOIN items b ON b.id=m.id-600000
 WHERE m.id BETWEEN 600000 AND 899999 AND b.damage>0 AND m.damage <> b.damage*2;
SELECT COUNT(*) FROM items h JOIN items b ON b.id=h.id-300000
 WHERE h.id BETWEEN 300000 AND 599999 AND b.damage>0 AND h.damage <> FLOOR(b.damage*1.5);
SELECT COUNT(*) FROM items m JOIN items b ON b.id=m.id-600000
 WHERE m.id BETWEEN 600000 AND 899999 AND b.ac>0 AND m.ac <> b.ac*2;
-- the 2026-08-05 bug: both tiers computing damage*2, so they came out identical
SELECT COUNT(*) FROM items m JOIN items h ON h.id=m.id-300000
 WHERE m.id BETWEEN 600000 AND 899999 AND m.damage>0 AND m.damage = h.damage;
```

Spot check: **item 25615 Frozen Two Handed Sword** — base **33**, Hallowed **49**, Mythic **66**.

---

## 5. Client files — three of tonight's changes are NOT server-side

| File | Why |
|---|---|
| `EQUI_AoTSpellChoiceWnd.xml` | regenerated after fear was pruned from the reward pool |
| `EQUI_AoTAllacloneWnd.xml` | the new **Zone XP** browse tab |
| `spells_us.txt` | v42 opened 74 outdoor-only spells; the client keeps its own `zonetype` copy and will refuse the cast without this |
| `dbstr_us.txt` | Bulwark Within's corrected description |

⚠️ Run `./export_client_files` **after** the migrations, then ship. Without `spells_us.txt` those 74
spells look fixed on the server and still refuse in game — the three-layer problem from §4 and §39.

---

## 6. ⚠️ TEST AS A NON-GM

The GM flag silently bypassed **four** separate gates during this work — the delve expansion check, the
region lock, trap triggering, and spell fizzling. Every one of them looked fine to a GM and was broken
for players. Use a normal character for:

- entering an even-numbered delve rung (the LDoN half)
- selling in a home town as a race that could not before
- an Alchemy combine on a non-Shaman, and training Tinkering on a non-Gnome at a GM trainer
- a delve boss step — trash kills must NOT close it

---

## 7. ⚠️ Do NOT run `aot_npcs_items_1.0.0.sql` anywhere

It carries `USE `peq`;` on line 20, which **overrides the database given on the command line**. Loading
it into a scratch schema applies it to the real one — that is how it damaged the test server.

Its entire useful payload is already captured in **v48**. Everything else it contains is a fortnight
stale and reverts craft sockets, the tradeskill gear rework and the gear-tier edits.

If it must ever be inspected, strip the header first and **verify the strip before loading**:

```bash
sed '/^USE `peq`;/d; /^CREATE DATABASE/d' file.sql > safe.sql
grep -cE "^USE |^CREATE DATABASE" safe.sql    # MUST print 0
```
