#!/usr/bin/env python3
"""
aotv4_migration.py -- write, check and merge an AoTv4 database change.

SEND THIS ONE FILE. It has no dependencies beyond python3 and needs neither the AoTv4 repository
nor a database to check your work.

    python3 aotv4_migration.py --template my_change.sql     # start from a template
    python3 aotv4_migration.py --format                     # print the file format
    python3 aotv4_migration.py my_change.sql                # CHECK it (this is all you need to do)

Send the .sql file back. The maintainer runs the same tool with --apply, which is the only mode that
needs the repository:

    python3 aotv4_migration.py my_change.sql --apply

Checks that always run, anywhere: the header, the SQL rules, reserved id bands and client id ceilings.
Checks that need a database (skipped with a notice if there is none): that the SQL actually executes,
and that the migration is idempotent.
"""
import argparse, os, re, subprocess, sys

CONDITIONS = {"contains", "match", "missing", "empty", "not_empty",
              "table_exists", "table_missing", "column_exists", "column_missing"}
# These four resolve themselves against information_schema and IGNORE `check`; `match` carries the
# object name ("my_table" or "my_table.my_column").
SCHEMA_CONDITIONS = {"table_exists", "table_missing", "column_exists", "column_missing"}
MANIFEST_REL = "common/database/database_update_manifest_custom.h"
VERSION_REL  = "common/version.h"

def find_repo(explicit=None):
    """The AoTv4 checkout, or None. Only --apply actually needs it."""
    for cand in filter(None, [explicit, os.environ.get("AOTV4_ROOT"),
                              os.path.dirname(os.path.abspath(__file__)),
                              os.getcwd(), "/src"]):
        d = os.path.abspath(cand)
        for _ in range(6):                       # walk up looking for the manifest
            if os.path.isfile(os.path.join(d, MANIFEST_REL)): return d
            parent = os.path.dirname(d)
            if parent == d: break
            d = parent
    return None

def db_cmd():
    """mysql invocation from the environment, or None if we cannot reach a database."""
    import shutil
    if not shutil.which("mysql"): return None
    cmd = ["mysql",
           "-h" + os.environ.get("AOTV4_DB_HOST", "127.0.0.1"),
           "-u" + os.environ.get("AOTV4_DB_USER", "peq"),
           "-p" + os.environ.get("AOTV4_DB_PASS", "peqpass"),
           os.environ.get("AOTV4_DB_NAME", "peq")]
    r = subprocess.run(cmd + ["-N", "--batch", "-e", "SELECT 1"], capture_output=True, text=True)
    return cmd if r.returncode == 0 else None

REPO = None      # resolved in main()
DB   = None      # resolved in main()

# Id space. "block" = refuse unless the submitter declares it (these collisions have cost real time);
# "home" = this is where such rows BELONG, so just say so and carry on.
BANDS = [
    ("block", "spells_new", 43000, 43149, "retired 43000-43112 custom set; 42 stock items still reference it"),
    ("home",  "spells_new", 43300, 43349, "custom spell lines (reptile/sloth/moonfire/sinew/promised/kindred/mark/thirst)"),
    ("block", "spells_new", 43350, 43399, "their triggers + Shield Wall buffs"),
    ("block", "spells_new", 43400, 43454, "AA tree buffs and pet wards"),
    ("block", "spells_new", 43500, 43565, "class auras"),
    ("block", "spells_new", 43576, 44327, "STOCK spell ranks -- LIVE on characters, never renumber"),
    ("block", "spells_new", 44328, 44332, "Insight, Fellowship Insignia/Health/Vigor, Light Campfire"),
    ("block", "spells_new", 44333, 44392, "CUSTOM spell ranks -- LIVE on characters, never renumber"),
    ("block", "spells_new", 44400, 44411, "tradeskill mask illusions"),
    ("home",  "spells_new", 44530, 44599, "heal lines (Mending Touch / Circle of Health / Circle of Renewal)"),
    ("home",  "items",     147500, 147999, "sigil, delve augments, currencies, tradeskill gear, tomes"),
    ("home",  "npc_types",2000000,2000999, "all AoTv4 custom NPCs"),
]

def fail(msg):
    print(f"REFUSED: {msg}", file=sys.stderr); sys.exit(1)

def warn(msg):
    print(f"  warning: {msg}")

def sql(q):
    r = subprocess.run(DB + ["-N", "--batch", "-e", q], capture_output=True, text=True)
    return r.returncode, r.stdout.strip(), r.stderr.strip()

# ----------------------------------------------------------------- parse
def parse(path):
    raw = open(path, encoding="utf-8").read()
    if "@aotv4-migration" not in raw:
        fail("missing the '-- @aotv4-migration' marker on the first line")
    meta, body, in_header = {}, [], True
    for line in raw.splitlines():
        if in_header:
            if line.strip().startswith("--"):
                m = re.match(r"^--\s*([a-z-]+)\s*:\s*(.*)$", line.strip())
                if m:
                    meta[m.group(1)] = m.group(2).strip(); continue
                if "@aotv4-migration" in line: continue
                continue
            if not line.strip(): continue
            in_header = False
        body.append(line)
    return meta, "\n".join(body).strip(), raw

# ----------------------------------------------------------------- validate
def validate(meta, body):
    for k in ("description", "condition"):
        if not meta.get(k): fail(f"header is missing required key '{k}'")
    if meta["condition"] not in CONDITIONS:
        fail(f"condition '{meta['condition']}' is not one of {sorted(CONDITIONS)}")
    if meta["condition"] in SCHEMA_CONDITIONS:
        if not meta.get("match"):
            fail(f"condition '{meta['condition']}' needs `match:` -- the table, or table.column")
        if meta["condition"].startswith("column_") and meta["match"].count(".") != 1:
            fail(f"condition '{meta['condition']}' needs match as `table.column`, got '{meta['match']}'")
        if meta.get("check"):
            warn("`check:` is ignored for a schema condition -- the query is built from `match:`")
    else:
        if not meta.get("check"): fail("header is missing required key 'check'")
        if meta["condition"] in ("contains", "match", "missing") and not meta.get("match"):
            fail(f"condition '{meta['condition']}' requires a 'match:' value")
    if not re.match(r"^\d{4}_\d{2}_\d{2}_[a-z0-9_]+$", meta["description"]):
        fail("description must look like 2026_08_19_short_snake_case_name")
    if not body: fail("no SQL found after the header")

    # --- the traps this project has actually hit ---
    if re.search(r"^\s*USE\s+`?peq`?\s*;", body, re.M | re.I):
        fail("contains `USE peq;` -- it overrides the database on the command line and has already "
             "damaged the test server once (CLAUDE.md section 35 note 7). Remove it.")
    if re.search(r"\bDROP\s+TABLE\b", body, re.I):
        fail("DROP TABLE removes triggers with it -- the Resplendent NPC lock has been silently lost "
             "this way (section 11). Use DELETE/TRUNCATE + reload, or declare `allow-drop: yes`.")
    if re.search(r"\bTRUNCATE\b", body, re.I) and "trader" in body.lower():
        fail("TRUNCATE on `trader` destroys REAL escrowed player items (section 13).")

    for m in re.finditer(r"'([^'\\]*(?:\\.[^'\\]*)*)'", body):
        pass  # well-formed strings consume here; the odd-quote check below catches the rest
    # unescaped apostrophe: count quotes outside of escapes, per line
    for i, line in enumerate(body.splitlines(), 1):
        if line.strip().startswith("--"): continue
        stripped = re.sub(r"\\'", "", line)
        if stripped.count("'") % 2:
            fail(f"line {i} has an odd number of single quotes -- an unescaped apostrophe "
                 f"(\"target's\") aborts the migration mid-run. Rephrase to avoid it.\n    {line.strip()}")

    if re.search(r"INSERT INTO\s+db_str", body, re.I) and re.search(r"%", body):
        fail("a literal '%' in a db_str description is eaten as a printf format token (section 14). "
             "Spell it out as 'percent'.")

    for m in re.finditer(r"\b(?:VALUES\s*\(|id\s*=\s*)(\d{4,7})\b", body):
        pass  # id extraction is done in band_check with table context

    if re.search(r"\bDELETE\s+FROM\b(?![\s\S]{0,200}?\bWHERE\b)", body, re.I):
        fail("a DELETE with no WHERE. Scope it to exactly the ids this migration creates "
             "(section 5: aotv4_moonfire_line.sql nearly ate the Sinew line).")

    if re.search(r"\b(items|spells_new)\b", body, re.I) and meta.get("shared-memory", "").lower() not in ("yes", "true"):
        warn("touches `items` or `spells_new`, which are SHARED MEMORY. Add `shared-memory: yes` so "
             "the deploy notes tell the operator to stop the stack and run ./shared_memory.")

def band_check(meta, body):
    declared = {b.strip() for b in meta.get("band", "").split(",") if b.strip()}
    for sev, table, lo, hi, why in BANDS:
        if not re.search(rf"\b{table}\b", body, re.I): continue
        hit = None
        for m in re.finditer(r"\b(\d{4,7})\b", body):
            v = int(m.group(1))
            if lo <= v <= hi: hit = v; break
        if hit is None: continue
        name = f"{table}:{lo}-{hi}"
        if sev == "home":
            print(f"  id {hit} is in {name} -- {why}. Correct place for this; carrying on.")
            continue
        if name in declared:
            print(f"  id {hit} is in RESERVED {name}, declared by the submitter -- allowed")
            continue
        fail(f"id {hit} falls in RESERVED band {name} ({why}).\n"
             f"    If that is deliberate, add `band: {name}` to the header and say why in `notes:`.")

def ceiling_check(body):
    if re.search(r"\bspells_new\b", body, re.I):
        for m in re.finditer(r"\b(\d{5,7})\b", body):
            v = int(m.group(1))
            if 45000 <= v <= 99999:
                fail(f"spell id {v} is at or above 45000 -- RoF2 masks spell links and the spellbook "
                     f"packet above it, so the row exists and the client cannot use it (section 14).")
    if re.search(r"\bitems\b", body, re.I):
        for m in re.finditer(r"\b(\d{7,9})\b", body):
            v = int(m.group(1))
            if v >= 1048576:
                fail(f"item id {v} is >= 1048576 -- RoF2 packs item links into 5 hex digits and masks "
                     f"with 0xFFFFF, so its chat link renders as a different item (section 10).")

# ----------------------------------------------------------------- dry run
# ⚠️⚠️ MUST MIRROR `AoTv4SchemaCheckQuery` in common/database/database_update.cpp. If the two ever
# disagree, the tool validates a migration the server then evaluates differently -- which is worse
# than having no check at all, because it validates clean and misbehaves in production.
def schema_check_sql(meta):
    c, m = meta["condition"], meta.get("match", "")
    if c in ("table_missing", "table_exists"):
        return ("SELECT TABLE_NAME FROM information_schema.TABLES "
                f"WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '{m}'")
    if c in ("column_missing", "column_exists"):
        t, col = m.split(".", 1)
        return ("SELECT COLUMN_NAME FROM information_schema.COLUMNS "
                f"WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '{t}' AND COLUMN_NAME = '{col}'")
    return None

# Statements the server COMMITS the moment it sees them -- an implicit commit ends any open
# transaction, so nothing before or after them can be rolled back either.
# ⚠️ TEMPORARY is deliberately excluded: `CREATE TEMPORARY TABLE` does not force a commit.
DDL_RE = re.compile(
    r"^\s*(CREATE\s+(?!TEMPORARY\b)(TABLE|INDEX|DATABASE|VIEW|TRIGGER|PROCEDURE|FUNCTION|EVENT)"
    r"|ALTER\s+(TABLE|DATABASE|VIEW|EVENT)"
    r"|DROP\s+(?!TEMPORARY\b)(TABLE|INDEX|DATABASE|VIEW|TRIGGER|PROCEDURE|FUNCTION|EVENT)"
    r"|TRUNCATE\b|RENAME\s+TABLE\b)", re.I | re.M)

def ddl_statements(body):
    out = []
    for line in body.splitlines():
        if line.strip().startswith("--"): continue
        if DDL_RE.match(line):
            out.append(line.strip()[:88])
    return out

def should_run_result(meta, res):
    res = (res or "").strip(); c, m = meta["condition"], meta.get("match", "")
    return {"contains": m in res, "match": res == m, "missing": m not in res,
            "empty": res == "", "not_empty": res != "",
            "table_missing": res == "", "column_missing": res == "",
            "table_exists": res != "", "column_exists": res != ""}[c]

def dry_run(meta, body):
    if DB is None:
        print("  no database reachable -- SKIPPING the execution and idempotency checks.")
        print("  That is fine for a submission: the maintainer runs them before merging.")
        print("  (To run them yourself, set AOTV4_DB_HOST/USER/PASS/NAME and have the `mysql` client.)")
        return
    check_sql = schema_check_sql(meta) or meta["check"]
    rc, before, err = sql(check_sql)
    if rc: fail(f"the check query is not valid SQL: {err}")

    # ⚠️⚠️ DDL CANNOT BE ROLLED BACK. MySQL/MariaDB IMPLICITLY COMMITS on CREATE / ALTER / DROP /
    # TRUNCATE / RENAME, so wrapping them in START TRANSACTION ... ROLLBACK does NOT undo them -- the
    # dry run APPLIES the migration for real. This tool did exactly that and permanently changed a
    # contributor's database on the first run; the tell was the check reading '0' on one invocation
    # and '1' on the next, which looks like caching and is not.
    # 📌 CREATE TEMPORARY TABLE is exempt -- it does not force a commit -- which is why the clone-via-
    # temp-table idiom used all over this project stays safe to dry-run.
    ddl = ddl_statements(body)
    if ddl:
        print("  ⚠ contains DDL that the database COMMITS immediately and cannot roll back:")
        for d in ddl[:4]: print(f"      {d}")
        if len(ddl) > 4: print(f"      ...and {len(ddl)-4} more")
        print("  SKIPPING the execution and idempotency checks -- running them would apply this")
        print("  migration for real. The static checks above still ran.")
        print(f"  check reads {before!r} right now; condition is '{meta['condition']}'.")
        if not should_run_result(meta, before):
            fail(f"the check/condition says this would NOT run (check reads {before!r}, condition "
                 f"'{meta['condition']}'"
                 + (f", match '{meta['match']}'" if meta.get("match") else "") + ").\n"
                 "    Either it is already applied, or the condition is inverted -- a CREATE TABLE\n"
                 "    almost always wants `condition: table_missing` with `match: <the table>`.")
        print("  the check says it WOULD run now. Idempotency is UNVERIFIED for DDL -- make sure the")
        print("  check tests the LAST object the SQL creates.")
        return

    script = "START TRANSACTION;\n" + body + "\n" + check_sql + ";\nROLLBACK;\n"
    r = subprocess.run(DB, input=script, capture_output=True, text=True)
    if r.returncode or "ERROR" in r.stderr:
        fail("the SQL failed when dry-run inside a rolled-back transaction:\n    "
             + (r.stderr.strip().splitlines() or ["(no message)"])[0])

    lines = [l for l in r.stdout.strip().splitlines() if l.strip()]
    after = lines[-1] if lines else ""
    print(f"  dry run OK (rolled back).  check before: {before!r}  after: {after!r}")

    def should_run(res): return should_run_result(meta, res)

    if not should_run(before):
        fail(f"the check/condition says this migration would NOT run against the current database "
             f"(check returned {before!r}, condition '{meta['condition']}'). Either it is already "
             f"applied, or the check is wrong.")
    if should_run(after):
        fail(f"NOT IDEMPOTENT: after applying, the check still says 'run me' (returned {after!r}). "
             f"World would re-run this on every boot. Make the check test for the thing the SQL "
             f"CREATES -- e.g. the LAST id it inserts.")
    print("  idempotency OK: would run now, would not run again")

# ----------------------------------------------------------------- inject
def next_version():
    txt = open(os.path.join(REPO, MANIFEST_REL), encoding="utf-8").read()
    return max(int(v) for v in re.findall(r"\.version\s*=\s*(\d+),", txt)) + 1

def inject(meta, body, version):
    txt = open(os.path.join(REPO, MANIFEST_REL), encoding="utf-8", newline="").read()
    notes = meta.get("notes", "")
    comment = "\n".join(f"\t\t// {l}" for l in
                        ([f"Submitted by: {meta['author']}"] if meta.get("author") else []) +
                        ([notes] if notes else []))
    entry = (f"\tManifestEntry{{\n"
             f"\t\t.version     = {version},\n"
             f"\t\t.description = \"{meta['description']}\",\n"
             + (comment + "\n" if comment else "") +
             f"\t\t.check       = \"{meta.get('check','').replace(chr(34), chr(92)+chr(34))}\",\n"
             f"\t\t.condition   = \"{meta['condition']}\",\n"
             f"\t\t.match       = \"{meta.get('match','')}\",\n"
             f"\t\t.sql         = R\"(\n{body}\n)\",\n"
             f"\t\t.content_schema_update = false,\n"
             f"\t}},\n")

    # ⚠️ anchor on the ARRAY's close, never on "the last }" -- the last one in this file is inside
    # the trailing comment block that documents ManifestEntry, and appending there compiles as garbage.
    anchor = "\t},\n};\n"
    idx = txt.find(anchor)
    if idx == -1: fail("could not find the manifest array close -- has the file changed shape?")
    txt = txt[:idx + len("\t},\n")] + entry + txt[idx + len("\t},\n"):]
    open(os.path.join(REPO, MANIFEST_REL), "w", encoding="utf-8", newline="").write(txt)

    vpath = os.path.join(REPO, VERSION_REL)
    v = open(vpath, encoding="utf-8", newline="").read()
    v2 = re.sub(r"(#define CUSTOM_BINARY_DATABASE_VERSION\s+)\d+", rf"\g<1>{version}", v)
    if v == v2: fail("could not bump CUSTOM_BINARY_DATABASE_VERSION in version.h")
    open(vpath, "w", encoding="utf-8", newline="").write(v2)

# ----------------------------------------------------------------- main
TEMPLATE = """-- @aotv4-migration
-- description: YYYY_MM_DD_short_snake_case_name
-- check: SELECT id FROM npc_types WHERE id = 2000900
-- condition: empty
-- match:
-- shared-memory: no
-- band:
-- author: your name
-- notes: WHY this exists. Not what the SQL says -- what breaks without it.

-- SQL goes below the blank line.
--
-- The `check` above must ask "has this already been applied?" and must test something this SQL
-- CREATES -- normally the LAST id it inserts. If it tests the first, a half-applied run is recorded
-- as finished. The tool refuses a check that would still say "run me" afterwards, because world
-- would then re-apply this on every single boot.

INSERT INTO npc_types (id, name, level, race, gender, class, bodytype, hp, size)
VALUES (2000900, 'Example_Npc', 1, 12, 0, 1, 1, 100, 3);
"""

FORMAT_HELP = """AoTv4 migration submission format
=================================

One .sql file. First line must be:   -- @aotv4-migration
Then `-- key: value` header lines, a blank line, then your SQL.

  description     required  YYYY_MM_DD_snake_case. The migration's name in the world log.
  check           required  A query answering "has this already been applied?"
  condition       required  empty | not_empty | match | contains | missing
                            ...or a SCHEMA condition (see below)
  match           required by match/contains/missing, and by every schema condition
  shared-memory   yes, if you touch `items` or `spells_new` (they live in shared memory)
  band            only if the tool tells you to -- acknowledges a reserved id range
  author          optional
  notes           optional but wanted: WHY, not what

Creating a table or a column?  Use a SCHEMA condition.
  A `SELECT ... FROM a_table_that_does_not_exist_yet` ERRORS, and the server turns a failed query
  into an EMPTY result -- which `condition: empty` cannot tell apart from a table that exists and is
  simply empty. So an ordinary check cannot honestly test for a new table. These can:

    condition: table_missing    match: my_table              <- what CREATE TABLE wants
    condition: table_exists     match: my_table
    condition: column_missing   match: my_table.my_column    <- what ALTER TABLE ADD COLUMN wants
    condition: column_exists    match: my_table.my_column

  They IGNORE `check:` and build their own information_schema query from `match:`, so you neither
  write one nor get its form subtly wrong.

The one rule that matters:  your migration must be safe to run twice. Either the check stops the
second run, or the second run changes nothing.

Refused automatically, because each has cost this project real time:
  - an unescaped apostrophe        (aborts the migration part-way, leaving the DB between versions)
  - a literal % in a db_str value  (eaten as a printf format token)
  - USE `peq`;                     (overrides the database on the command line)
  - DROP TABLE                     (takes the table's triggers with it, silently)
  - TRUNCATE on `trader`           (those rows are real escrowed player items)
  - DELETE with no WHERE
  - a spell id >= 45000            (client masks it; row exists, unusable, no error)
  - an item id >= 1048576          (chat link renders as a different item)
  - writing into a live id band    (spell ranks are resolved per character)
  - a non-idempotent check         (would re-run on every world boot forever)
"""

def main():
    global REPO, DB
    ap = argparse.ArgumentParser(add_help=True, description="Write, check and merge an AoTv4 database change.")
    ap.add_argument("file", nargs="?", help="the .sql submission to check")
    ap.add_argument("--apply", action="store_true", help="merge it (maintainer only; needs the repo)")
    ap.add_argument("--template", metavar="PATH", help="write a starter submission and exit")
    ap.add_argument("--format", action="store_true", help="print the file format and exit")
    ap.add_argument("--repo", help="path to the AoTv4 checkout (only needed for --apply)")
    a = ap.parse_args()

    if a.format:
        print(FORMAT_HELP); return
    if a.template:
        if os.path.exists(a.template): fail(f"{a.template} already exists")
        open(a.template, "w", encoding="utf-8").write(TEMPLATE)
        print(f"wrote {a.template}\n  edit it, then:  python3 {os.path.basename(__file__)} {a.template}")
        return
    if not a.file:
        ap.print_help(); return

    REPO = find_repo(a.repo)
    DB   = db_cmd()

    meta, body, _ = parse(a.file)
    print(f"submission: {a.file}")
    print(f"  description: {meta['description']}")
    validate(meta, body)
    band_check(meta, body)
    ceiling_check(body)
    dry_run(meta, body)

    if not a.apply:
        if DB is None:
            print("\nCHECKS PASSED (offline). Send this file to the maintainer.")
        else:
            print(f"\nVALIDATED. Would become migration v{next_version() if REPO else '?'}."
                  f"  Maintainer merges with --apply.")
        return

    if REPO is None:
        fail("--apply needs the AoTv4 checkout. Pass --repo /path/to/AoTv4 or set AOTV4_ROOT.")
    if DB is None:
        fail("--apply needs a database to dry-run against. Set AOTV4_DB_HOST/USER/PASS/NAME.")

    v = next_version()
    inject(meta, body, v)
    print(f"\nMERGED as v{v}.")
    print(f"  next: cd {REPO}/build && ninja world zone")
    if meta.get("shared-memory", "").lower() in ("yes", "true"):
        print("  * touches shared memory: stop the stack, ./shared_memory, then restart")
    print("  * restart WORLD FIRST (it applies the migration), then zones -- a zone binary")
    print("    ahead of the database refuses to boot and every zone exits.")

main()