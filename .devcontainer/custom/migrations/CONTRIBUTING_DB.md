# Adding to the AoTv4 database

**You do not edit the database, and you do not edit the manifest.** You write **one `.sql` file**
with a short header, and a tool turns it into a real migration. That is the whole contract.

Doing it this way is not bureaucracy. Every database change has to reach a **live server that is a
different machine with a different database** — so a change that only exists as "I ran some SQL" is a
change that reaches nobody. A migration applies itself when world boots, on every server, exactly
once, in order.

---

## 1. The workflow

You need **one file: `aotv4_migration.py`**. Nothing else — no copy of the repository, no database,
no Python packages. If someone sent you just that file, you have everything.

```bash
python3 aotv4_migration.py --format                  # what the file format is
python3 aotv4_migration.py --template my_change.sql  # start from a template
#   ...edit my_change.sql...
python3 aotv4_migration.py my_change.sql             # check it
```

When it prints **`CHECKS PASSED`**, send `my_change.sql` back. That is the whole job.

Everything the tool knows travels inside it — `--format` prints the spec, `--template` writes a
starter file, and the checks run with or without a database. There is no wiki to go and read.

### What the maintainer then does

```bash
python3 aotv4_migration.py my_change.sql --apply
```

`--apply` is the only mode needing the checkout and the database. It assigns the next version number,
injects the entry into `common/database/database_update_manifest_custom.h`, and bumps
`CUSTOM_BINARY_DATABASE_VERSION` in `common/version.h`. **Neither file is ever edited by hand** — that
is how two people take the same version number, and how an entry lands in the wrong part of the file.

It finds the checkout automatically; override with `--repo /path/to/AoTv4` or `AOTV4_ROOT`. Database
settings come from `AOTV4_DB_HOST` / `_USER` / `_PASS` / `_NAME`.

### Checks you get offline, and checks that need a database

| Always, anywhere | Needs a database |
|---|---|
| header is complete and well formed | the SQL actually executes |
| the SQL rules in §4 | the migration is **idempotent** (§3) |
| reserved id bands (§5) | |
| client id ceilings (§4) | |

Without a database the tool says so plainly and skips those two. A submission checked offline is
still worth sending — the maintainer runs the rest before merging.

## 2. The file

```sql
-- @aotv4-migration
-- description: 2026_08_19_frost_line
-- check: SELECT id FROM spells_new WHERE id = 44705
-- condition: empty
-- shared-memory: yes
-- author: Jane
-- notes: Adds a six-tier Frost line to fill the gap between levels 8 and 58, where Wizards
--        currently have nothing at all below Ice Comet.

CREATE TEMPORARY TABLE tmp_frost AS SELECT * FROM spells_new WHERE id = 466;
UPDATE tmp_frost SET id = 44700, name = 'Frostbite', effect_base_value1 = -30,
                     formula1 = 100, max1 = 0;
INSERT INTO spells_new SELECT * FROM tmp_frost;
DROP TEMPORARY TABLE tmp_frost;
-- ...five more tiers, last one id 44705...

INSERT INTO db_str (id, type, value) VALUES
  (44700, 6, 'Sheathes the target in frost, dealing 30 damage.');
```

The first line **must** be `-- @aotv4-migration`. Header keys are `-- key: value` and stop at the
first blank line.

| key | required | what it is |
|---|---|---|
| `description` | **yes** | `YYYY_MM_DD_snake_case`. It is the migration's name in the world log. |
| `check` | yes, unless a schema condition | A query that answers *"has this already been applied?"* See §3 — this is the part people get wrong. Schema conditions build their own and ignore it. |
| `condition` | **yes** | `empty`, `not_empty`, `match`, `contains`, `missing`, or a **schema condition** (§3a). |
| `match` | only for `match`/`contains`/`missing` | The string to compare the check's output against. |
| `shared-memory` | when you touch `items` or `spells_new` | `yes`. Those two tables live in shared memory; without a rebuild the rows are invisible to every zone. |
| `band` | only if the tool asks | Acknowledges that you are deliberately writing into a reserved id range. |
| `author` | no | Recorded in the manifest comment. |
| `notes` | strongly encouraged | **Why**, not what. The SQL already says what. |

---

## 3. `check` and `condition` — the part that goes wrong

The server runs your `check` query and compares the result to `condition`. If it says "run me", the
SQL runs. **This happens on every world boot forever**, so:

> The check must test for something **your SQL creates**, and it must stop being true afterwards.

The reliable pattern is to check the **last** id your SQL inserts:

```sql
-- check: SELECT id FROM spells_new WHERE id = 44705
-- condition: empty
```

Because 44705 is the *last* tier, a run that dies halfway leaves the check still saying "run me", and
the next boot finishes the job. Check the *first* id instead and a half-applied migration is recorded
as complete.

The tool enforces this: it dry-runs your SQL in a transaction, re-runs your check, and **refuses the
submission if the check still says "run me" afterwards**. That is a migration that would re-run on
every boot forever.

⚠️ **Do not check a version number, a date, or a row count.** Counts drift for unrelated reasons; a
migration keyed on `COUNT(*) = 12` fires again the moment someone adds a thirteenth row.

---

## 3a. Creating a table or a column — use a schema condition

⚠️⚠️ **Do NOT try to check for a new table with an ordinary query.** A
`SELECT ... FROM a_table_that_does_not_exist_yet` **errors**, and the server turns a failed query into
an **empty result** — which `condition: empty` cannot tell apart from a table that exists and simply
has no rows. There was no honest way to write this check, so use one of these instead:

```sql
-- condition: table_missing
-- match: my_new_table
```

| condition | `match` | runs when |
|---|---|---|
| `table_missing` | `my_table` | the table is **not** there — what a `CREATE TABLE` wants |
| `table_exists` | `my_table` | the table **is** there |
| `column_missing` | `my_table.my_column` | the column is not there — what `ALTER TABLE ADD COLUMN` wants |
| `column_exists` | `my_table.my_column` | the column is there |

These **ignore `check:`** and build their own `information_schema` query from `match:`, so you do not
write one and cannot get its form subtly wrong. They use `DATABASE()`, so they are correct against
either the main or the content database.

📌 For a migration that creates a table **and** seeds it, `table_missing` is still the right check:
the table is the thing whose absence means "not applied yet". Seeding rows are covered by the same
run, and if it dies part-way the table already exists — so keep the create and the seed in one
migration, and make the seed safe to re-run.

---

## 4. What the tool refuses, and why

Every one of these is something that has actually cost this project time.

| Refusal | Why |
|---|---|
| An **unescaped apostrophe** | `'your target's wounds'` is a syntax error. The migration aborts *part-way*, leaving the database between versions. Rephrase — do not escape it; `\'` in a dump defeats a plain `grep` for the text later. |
| A literal **`%`** in a `db_str` description | The description path is printf-style and eats it as a format token. Write "percent". |
| **`USE \`peq\`;`** | It overrides the database given on the command line. This is how a file loaded into a scratch schema damaged the real one. |
| **`DROP TABLE`** | Takes the table's **triggers** with it. The Resplendent NPC guard has been silently lost this way, and nothing looked wrong afterwards. |
| **`TRUNCATE`** on `trader` | Those rows are **real escrowed player items**, not a cache. |
| **`DELETE` with no `WHERE`** | Scope it to exactly the ids you create. A cleanup DELETE that was one band too wide nearly destroyed a spell line belonging to a different script. |
| A **spell id ≥ 45000** | RoF2 masks spell links and the spellbook packet above it. The row exists, the client cannot use it, and nothing errors. |
| An **item id ≥ 1,048,576** | RoF2 packs item links into five hex digits. The link renders as a *different* item. |
| Writing into a **reserved band** | See §5. |
| A **non-idempotent** check | See §3. |

It also *warns* (does not refuse) when you touch `items` or `spells_new` without
`shared-memory: yes`, because the operator needs to know to rebuild.

---

## 5. Reserved id bands

Some ranges are **live on player characters** — renumbering them silently changes what people own.
The tool blocks these unless you declare `band:` and explain yourself in `notes:`.

| Range | What lives there |
|---|---|
| `spells_new 43000-43149` | retired custom set; 42 stock items still reference it |
| `spells_new 43350-43399` | triggers + Shield Wall buffs |
| `spells_new 43400-43454` | AA tree buffs, pet wards |
| `spells_new 43500-43565` | class auras |
| `spells_new 43576-44327` | **stock spell ranks — live on characters** |
| `spells_new 44328-44332` | Insight, Fellowship Insignia / Health / Vigor, Light Campfire |
| `spells_new 44333-44392` | **custom spell ranks — live on characters** |
| `spells_new 44400-44411` | tradeskill mask illusions |

And these are **where things belong** — the tool says so and carries on:

| Range | For |
|---|---|
| `spells_new 43300-43349` | custom spell lines (full; use the band below) |
| `spells_new 44530-44599` | custom spell lines, current band |
| `items 147500-147999` | custom items |
| `npc_types 2000000-2000999` | custom NPCs |

⚠️ **Adding a new band is not just picking free ids.** At least three places enumerate the custom
bands — the pool generator, the search query, and the rank generator. A band that only exists in your
SQL produces a spell nobody can be offered, nobody can look up, and nobody can rank, with no error
anywhere. Grep for an existing band number and fix every site that knows about it.

---

## 6. A new spell? Write its description in the same file

A spell with no `db_str` row renders **blank** in every window that describes it, and nothing errors —
the spell casts perfectly. Eighteen heal spells shipped this way and it was only caught from play.

- Description is **`db_str` type 6**, keyed on the spell's **`descnum`** (which is usually, but *not
  always*, the spell id — set `descnum = id` on anything you create and it stays simple).
- Type 4 is the **AA** description. Wrong type, same blank panel.

Clone spells **via a temporary table**, never by hand-listing columns — `spells_new` has ~236 of them,
and a clone that silently differs in `formula` or `max` is a real bug that has shipped here before.
⚠️ Cloning also inherits `descnum`, so **repoint it**, or your new spell shows the *original's* text.

---

## 7. What is *not* a migration

| Change | Where it goes |
|---|---|
| Quest / module behaviour | `.devcontainer/repo/quests/**.lua` — tracked in this repo, ships with a clone |
| Generated files (`spell_pool.lua`, `spell_ranks.lua`, `spell_icons.lua`, …) | **Never hand-edit.** Re-run the generator in `.devcontainer/custom/tools/` |
| UI windows, `spells_us.txt`, `dbstr_us.txt` | `aotv4_client_install/` — these must be *shipped to players*, a migration cannot do it |
| The client dll | A separate repo, deliberately not in this one |

⚠️ If your change adds or renames a **spell or a description**, the database is only half of it. The
client resolves both from its own `spells_us.txt` / `dbstr_us.txt`, so it needs
`./export_client_files` and those files shipped before any player sees the change.

---

## 8. After it merges

```bash
cd /src/build && ninja world zone
```

Then, in this order:

1. **Stop the stack.**
2. If your migration set `shared-memory: yes`: `cd build/bin && ./shared_memory`
3. **Start `world` first.** It applies the migration.
4. Then start `eqlaunch` — exactly one.

⚠️⚠️ **World first is not a style preference.** A `zone` binary whose compiled version is ahead of the
database **refuses to boot** — every zone exits, none binds a port, world answers *"no zoneserver
available"*, and players see "server is down" while `eqlaunch` looks perfectly healthy. Starting world
first closes the gap before any zone looks at it.

Confirm with `SELECT custom_version FROM db_version;` and by finding
`Custom | database [N] binary [N] up to date` in a zone log.


## Two rules the tool now enforces, added 2026-08-20

Both came out of a real submission that was correct in every other way.

### Creating tables and filling them is TWO files

`CREATE TABLE` commits immediately and cannot be rolled back. A migration that creates *and*
populates therefore can never be re-run: the tables already exist, the condition reads as
satisfied, and the inserts sit stranded behind it with nothing on screen to say why. Send one
submission that creates and one that fills, and let the second check a row it inserts.

### Point the check at the LAST thing your SQL touches

If your SQL creates three tables, `match:` the **third**. If it fills three tables, check a row in
the **third**. Keyed on the first, a run that dies halfway records itself as finished and the
remainder is never applied — silently, because the server writes the new version and moves on.

The dry run cannot catch this: it applies the whole file, so a first-object check looks perfectly
idempotent. The tool therefore reads the order of your own statements and compares. It is the one
check with no runtime equivalent, which is why it is worth understanding rather than working around.

### And a message that is not an error

`NOTE: already applied here` means the maintainer's database already contains your change — usually
because it was applied by hand while testing. Your submission is fine; it simply has nothing left to
do against that particular database. Only `REFUSED` stops a merge.

## Do not hand-edit the manifest

`--apply` writes the entry, allocates the version and bumps `CUSTOM_BINARY_DATABASE_VERSION` for
you. Editing `database_update_manifest_custom.h` by hand is how you get a conflict on the one file
every contributor touches — and a conflict resolved carelessly there deletes other people's
migrations without leaving a mark on the lines it removed.
