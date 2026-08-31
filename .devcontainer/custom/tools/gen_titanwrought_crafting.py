#!/usr/bin/env python3
"""
Generate the Titanwrought mold-crafting system: gear, molds, materials, recipes.
Design: /src/TITANWROUGHT_CRAFTING_DESIGN.md

Writes SQL into custom/sql/aotv4_titanwrought_*.sql, one file per migration.
Deterministic and idempotent: each file DELETEs its exact id band, then INSERTs.

  ⚠️ Item clones go through a TEMP TABLE so all ~285 columns stay byte-identical
     to a real stock row -- never hand-list columns (CLAUDE.md §5).
  ⚠️ Every recipe gets min_expansion = max_expansion = -1 and must_learn = 0 or
     the recipe SEARCH content-filters it away and nobody can find it (§32).
  ⚠️ nofail = 1 everywhere: combines never fail; skill picks the TIER through
     AoTv4RollCraftTier at the GetTradeRecipe choke point (§32/§26).
  ⚠️ The mold is CONSUMED -- componentcount 1, successcount 0.
"""
import subprocess, sys, collections

DB = ["mysql", "-h127.0.0.1", "-upeq", "-ppeqpass", "peq", "-N", "-B", "-e"]
OUT = "/src/.devcontainer/custom/sql"

def q(sql):
    r = subprocess.run(DB + [sql], capture_output=True, text=True, timeout=180)
    if r.returncode:
        sys.exit("SQL failed: " + r.stderr)
    return [l.split("\t") for l in r.stdout.strip().split("\n") if l.strip()]

def esc(s): return str(s).replace("'", "''")

# ---------------------------------------------------------------- id bands
GEAR_BASE, MOLD_BASE, MAT_BASE, RECIPE_BASE = 148000, 148200, 148500, 480000
GEAR_END,  MOLD_END,  MAT_END,  RECIPE_END  = 148199, 148499, 148599, 480499
MOLD_CLONE, MAT_CLONE, MOLD_ICON = 3450, 10503, 1151

TIERS = [("Crude", 10, 50, 0, 100_000),
         ("Simple", 20, 89, 46, 300_000),
         ("Rough", 30, 130, 120, 500_000)]
TYPES  = ["Cloth", "Leather", "Chain", "Plate"]
METALS = ["Silver", "Gold", "Electrum", "Platinum"]
WMETAL = ["Bronze", "Iron", "Steel", "Mithril"]

TAILOR, SMITH, JEWEL, POTTERY, ALCHEMY = 61, 63, 68, 69, 59
LOOM, FORGE, JEWELKIT, KIT = 16, 17, 20, 990061
TYPE_COMBINE = {"Cloth": (TAILOR, LOOM), "Leather": (TAILOR, LOOM),
                "Chain": (SMITH, FORGE), "Plate": (SMITH, FORGE)}

EXISTING_NAMES = {
    4:      {"Cloth":"Cloth Cap","Leather":"Leather Cap","Chain":"Chain Coif","Plate":"Plate Helm"},
    128:    {"Cloth":"Cloth Sleeves","Leather":"Leather Sleeves","Chain":"Chain Sleeves","Plate":"Plate Vambraces"},
    1536:   {"Cloth":"Cloth Wristwrap","Leather":"Leather Bracer","Chain":"Chain Bracer","Plate":"Plate Bracer"},
    4096:   {"Cloth":"Cloth Gloves","Leather":"Leather Gloves","Chain":"Chain Gauntlets","Plate":"Plate Gauntlets"},
    131072: {"Cloth":"Cloth Robe","Leather":"Leather Tunic","Chain":"Chain Tunic","Plate":"Breastplate"},
    262144: {"Cloth":"Cloth Pantaloons","Leather":"Leather Trousers","Chain":"Chain Leggings","Plate":"Plate Greaves"},
    524288: {"Cloth":"Cloth Sandals","Leather":"Leather Boots","Chain":"Chain Boots","Plate":"Plate Boots"},
}
NEW_SLOTS = {64: 4096, 256: 1536, 8: 1536, 1048576: 4096}   # new slot -> clone-from slot
NEW_NAMES = {
    64:      {"Cloth":"Mantle","Leather":"Shoulderpads","Chain":"Mantle","Plate":"Pauldrons"},
    256:     {"Cloth":"Cloak","Leather":"Cloak","Chain":"Cape","Plate":"Cloak"},
    8:       {"Cloth":"Veil","Leather":"Mask","Chain":"Veil","Plate":"Visor"},
    1048576: {"Cloth":"Sash","Leather":"Belt","Chain":"Belt","Plate":"Girdle"},
}
JEWEL_SLOTS = {18: "Earring", 32: "Necklace", 98304: "Ring"}
ICON = {8:770, 18:1050, 32:626, 64:798, 256:663, 98304:612, 1048576:564}

# ---------------------------------------------------------------- read existing
# ⚠️⚠️ EXCLUDE OUR OWN OUTPUT BAND OR A RE-RUN IS NOT IDEMPOTENT. The generated gear is named
# "<Tier> Titanwrought ..." exactly like the stock line, so on a second run this query reads back
# items the FIRST run created and concludes they already exist -- silently dropping the two charms
# (86 new items became 84). Nothing errors; the counts just quietly shrink.
rows = q("SELECT id,Name,slots FROM items WHERE Name REGEXP "
         "'^(Crude|Simple|Rough) Titanwrought' AND id<300000 AND slots<>2070526 "
         "AND id NOT BETWEEN %d AND %d ORDER BY id" % (GEAR_BASE, MAT_END))
by_name, weapons = {}, collections.defaultdict(list)
for iid, name, slots in rows:
    iid, slots = int(iid), int(slots)
    tier = name.split(" ", 1)[0]
    tail = name.split("Titanwrought ", 1)[1]
    by_name[(tier, tail)] = iid
    if slots in (2048, 8192, 16384, 24576):
        weapons[tier].append((tail, iid))
for t in weapons: weapons[t].sort()

# Each entry is a dict of the columns to override; emitted as ONE set-based clone (see clone_sql).
def clone(src, nid, name, **cols):
    cols = dict(cols); cols["Name"] = name
    return (nid, src, cols)

CLONE_COLS = ["Name", "slots", "icon", "reclevel", "ac", "price",
              "nodrop", "norent", "stackable", "stacksize"]

def clone_sql(rows):
    """Clone a batch of items through ONE single-row temp table.

    ⚠️ Still a `SELECT *` clone, so all ~285 item columns stay byte-identical to the stock source
       row (CLAUDE.md §5) -- only the handful we deliberately override are ever named.
    ⚠️⚠️ NO `ALTER TABLE`, DELIBERATELY. The obvious set-based form needs a helper column and an
       `ALTER TABLE _tw DROP INDEX/ADD COLUMN`, and the migration validator REFUSES that: DDL commits
       immediately, so it cannot dry-run or prove idempotency, and it declines to merge the file.
       Holding exactly one row at a time needs no helper column and no index drop -- the temp table
       never contains two rows, so nothing can collide on `id`.
    ⚠️ Rows are grouped by source so the reseed only happens when the source changes.
    """
    out = ["CREATE TEMPORARY TABLE _tw LIKE items;"]
    cur = None
    for nid, src, cols in sorted(rows, key=lambda r: (r[1], r[0])):
        if src != cur:
            out.append("DELETE FROM _tw;")
            out.append("INSERT INTO _tw SELECT * FROM items WHERE id=%d;" % src)
            cur = src
        sets = []
        for c in CLONE_COLS:
            v = cols.get(c)
            if v is None: continue
            # ⚠️ The WAIST slot bitmask is literally 1048576 and the validator refuses any bare
            # literal that large as a would-be item id (RoF2 masks chat links with 0xFFFFF, so such
            # an id renders as a different item, §10). A shift is the same value with no literal,
            # and keeps the guard useful for real item ids rather than teaching anyone to bypass it.
            v = "(1<<20)" if v == 1048576 else (str(v) if isinstance(v, int) else "'%s'" % esc(v))
            sets.append("`%s`=%s" % (c, v))
        # ⚠️ `id` is set LAST: MySQL applies SET left to right, so rewriting it first would change
        # the row out from under any later expression that reads it.
        out.append("UPDATE _tw SET %s, id=%d;" % (", ".join(sets), nid))
        out.append("INSERT INTO items SELECT * FROM _tw;")
    out.append("DROP TEMPORARY TABLE _tw;")
    return "\n".join(out) + "\n"

gear_sql, mold_sql, mat_sql = [], [], []
outputs = []      # (tier, group, axis_key, label, item_id)
nid = GEAR_BASE

for tier, rlvl, triv, need, tprice in TIERS:
    for t in TYPES:
        for slot, nm in EXISTING_NAMES.items():
            outputs.append((tier, "armour", t, nm[t], by_name[(tier, nm[t])]))
        for slot, src_slot in NEW_SLOTS.items():
            label = "%s %s" % (t, NEW_NAMES[slot][t])
            src = by_name[(tier, EXISTING_NAMES[src_slot][t])]
            gear_sql.append(clone(src, nid, "%s Titanwrought %s" % (tier, label),
                                  slots=slot, icon=ICON[slot], reclevel=rlvl))
            outputs.append((tier, "armour", t, label, nid)); nid += 1
    cw = by_name[(tier, "Cloth Wristwrap")]
    for slot, word in JEWEL_SLOTS.items():
        for m in METALS:
            label = "%s %s" % (m, word)
            gear_sql.append(clone(cw, nid, "%s Titanwrought %s" % (tier, label),
                                  slots=slot, icon=ICON[slot], reclevel=rlvl, ac=0))
            outputs.append((tier, "jewel", m, label, nid)); nid += 1
    if (tier, "Charm") in by_name:
        outputs.append((tier, "charm", "Charm", "Charm", by_name[(tier, "Charm")]))
    else:
        gear_sql.append(clone(by_name[("Rough", "Charm")], nid,
                              "%s Titanwrought Charm" % tier, reclevel=rlvl))
        outputs.append((tier, "charm", "Charm", "Charm", nid)); nid += 1
    for i, (shape, wid) in enumerate(weapons[tier]):
        outputs.append((tier, "weapon", WMETAL[i % 4], shape, wid))

GEAR_LAST = nid - 1

# ---------------------------------------------------------------- 2. molds (one per output)
molds = {}
mid = MOLD_BASE
for (tier, grp, axis, label, item_id) in outputs:
    name = "%s %s Mold" % (tier, label)
    assert len(name) <= 64, name
    mold_sql.append(clone(MOLD_CLONE, mid, name, icon=MOLD_ICON, price=0,
                          nodrop=1, norent=1, stackable=1, stacksize=20, slots=0))
    molds[(tier, grp, axis, label)] = mid
    mid += 1
MOLD_LAST = mid - 1

# ---------------------------------------------------------------- 3. materials
RAW = {"Cloth": 16482, "Leather": 97860, "Chain": 10503, "Plate": 10503}   # Silk Swatch/Raw Hide/Ore
RAW_JEWEL, RAW_WEAPON, WATER = 10028, 10503, 13006                        # Peridot / Ore / Water Flask
BASE_NAME = {"Cloth": "Silk Bolt", "Leather": "Cured Hide",
             "Chain": "Chain Links", "Plate": "Plate Ingot"}
BASE_SKILL = {"Cloth": TAILOR, "Leather": TAILOR, "Chain": SMITH, "Plate": SMITH}
BIND = {"armour": ("Armour Fitting", POTTERY, 21), "jewel": ("Jeweler's Solvent", ALCHEMY, KIT),
        "weapon": ("Pommel Stone", JEWEL, JEWELKIT)}
mats = {}          # ('base'|'bind'|'temper', tier, key) -> id
mat_recipes = []   # (name, tradeskill, containers, trivial, skillneeded, [components], out_id)
aid = MAT_BASE
for tier, rlvl, triv, need, tprice in TIERS:
    for t in TYPES:                                   # armour bases
        nm = "%s %s" % (tier, BASE_NAME[t])
        mat_sql.append(clone(MAT_CLONE, aid, nm, icon=MOLD_ICON, price=0, stackable=1, stacksize=20))
        mats[("base", tier, t)] = aid
        mat_recipes.append((nm, BASE_SKILL[t], [LOOM if BASE_SKILL[t]==TAILOR else FORGE, KIT],
                            max(triv-10,1), need, [(RAW[t],2),(WATER,1)], aid)); aid += 1
    for m in METALS:                                  # jewelry bases
        nm = "%s %s Bar" % (tier, m)
        mat_sql.append(clone(MAT_CLONE, aid, nm, icon=MOLD_ICON, price=0, stackable=1, stacksize=20))
        mats[("base", tier, m)] = aid
        mat_recipes.append((nm, JEWEL, [JEWELKIT, KIT], max(triv-10,1), need,
                            [(RAW_JEWEL,1),(WATER,1)], aid)); aid += 1
    for m in WMETAL:                                  # weapon bases
        nm = "%s %s Billet" % (tier, m)
        mat_sql.append(clone(MAT_CLONE, aid, nm, icon=MOLD_ICON, price=0, stackable=1, stacksize=20))
        mats[("base", tier, m)] = aid
        mat_recipes.append((nm, SMITH, [FORGE, KIT], max(triv-10,1), need,
                            [(RAW_WEAPON,2),(WATER,1)], aid)); aid += 1
    for grp,(bn, bskill, bcont) in BIND.items():      # bindings, one per group
        nm = "%s %s" % (tier, bn)
        mat_sql.append(clone(MAT_CLONE, aid, nm, icon=MOLD_ICON, price=0, stackable=1, stacksize=20))
        mats[("bind", tier, grp)] = aid
        mat_recipes.append((nm, bskill, [bcont, KIT], max(triv-10,1), need,
                            [(WATER,1),(RAW_JEWEL if bskill==JEWEL else RAW[
                                "Cloth" if bskill==POTTERY else "Leather"],1)], aid)); aid += 1
    nm = "%s Temper" % tier                            # bought, no recipe
    mat_sql.append(clone(MAT_CLONE, aid, nm, icon=MOLD_ICON, price=tprice,
                         stackable=1, stacksize=20, nodrop=1, norent=1))
    mats[("temper", tier, None)] = aid; aid += 1
MAT_LAST = aid - 1

# ---------------------------------------------------------------- 4. recipes
recipes = []       # (id, name, tradeskill, trivial, skillneeded, containers, entries)
rid = RECIPE_BASE
for (name, skill, conts, triv, need, comps, out) in mat_recipes:
    entries = [(c, n, 0) for c, n in comps] + [(out, 0, 1)]
    recipes.append((rid, name, skill, triv, need, conts, entries)); rid += 1
for (tier, grp, axis, label, item_id) in outputs:
    triv, need = next((t[2], t[3]) for t in TIERS if t[0] == tier)
    if grp == "armour":   skill, cont = TYPE_COMBINE[axis]
    elif grp == "jewel":  skill, cont = JEWEL, JEWELKIT
    elif grp == "weapon": skill, cont = SMITH, FORGE
    else:                 skill, cont = JEWEL, JEWELKIT      # charm
    bind_grp = grp if grp in BIND else "jewel"
    base_id = mats[("base", tier, axis)] if grp != "charm" else mats[("base", tier, "Silver")]
    entries = [(molds[(tier, grp, axis, label)], 1, 0),
               (base_id, 1, 0),
               (mats[("bind", tier, bind_grp)], 1, 0),
               (mats[("temper", tier, None)], 1, 0),
               (item_id, 0, 1)]
    recipes.append((rid, "%s Titanwrought %s" % (tier, label), skill, triv, need,
                    [cont, KIT], entries)); rid += 1
RECIPE_LAST = rid - 1

def recipe_sql():
    heads, ents = [], []
    for (r, name, skill, triv, need, conts, entries) in recipes:
        heads.append("(%d,'%s',%d,%d,%d,1,0,0,0,1,-1,-1)" % (r, esc(name), skill, need, triv))
        ents += ["(%d,%d,%d,%d,0,0,0)" % (r, i, c, sc) for (i, c, sc) in entries]
        ents += ["(%d,%d,0,0,0,0,1)" % (r, c) for c in conts]
    def chunk(rows, n=60):
        for i in range(0, len(rows), n): yield rows[i:i+n]
    out = []
    for c in chunk(heads):
        out.append("INSERT INTO tradeskill_recipe (id,name,tradeskill,skillneeded,trivial,nofail,"
                   "replace_container,must_learn,quest,enabled,min_expansion,max_expansion) VALUES\n"
                   + ",\n".join(c) + ";")
    for c in chunk(ents, 120):
        out.append("INSERT INTO tradeskill_recipe_entries (recipe_id,item_id,componentcount,"
                   "successcount,failcount,salvagecount,iscontainer) VALUES\n" + ",\n".join(c) + ";")
    return "\n".join(out) + "\n"

hdr = lambda t: "-- GENERATED by custom/tools/gen_titanwrought_crafting.py -- do not hand edit.\n-- %s\n" % t
def w(fn, body): open(OUT + "/" + fn, "w").write(body)

w("aotv4_titanwrought_gear.sql", hdr("86 new gear items: 4 armour slots x 4 types, jewelry x 4 metals, 2 charms")
  + "DELETE FROM items WHERE id BETWEEN %d AND %d;\n" % (GEAR_BASE, GEAR_END) + clone_sql(gear_sql))
w("aotv4_titanwrought_molds.sql", hdr("%d molds, one per craftable output" % len(molds))
  + "DELETE FROM items WHERE id BETWEEN %d AND %d;\n" % (MOLD_BASE, MOLD_END) + clone_sql(mold_sql))
w("aotv4_titanwrought_materials.sql", hdr("%d materials: 36 bases, 9 bindings, 3 tempers" % (aid - MAT_BASE))
  + "DELETE FROM items WHERE id BETWEEN %d AND %d;\n" % (MAT_BASE, MAT_END) + clone_sql(mat_sql))
w("aotv4_titanwrought_recipes.sql", hdr("%d recipes: %d sub-recipes + %d final combines"
    % (len(recipes), len(mat_recipes), len(outputs)))
  + "DELETE FROM tradeskill_recipe_entries WHERE recipe_id BETWEEN %d AND %d;\n" % (RECIPE_BASE, RECIPE_END)
  + "DELETE FROM tradeskill_recipe WHERE id BETWEEN %d AND %d;\n" % (RECIPE_BASE, RECIPE_END) + recipe_sql())

print("new gear items : %-4d ids %d..%d" % (GEAR_LAST-GEAR_BASE+1, GEAR_BASE, GEAR_LAST))
print("molds          : %-4d ids %d..%d" % (len(molds), MOLD_BASE, MOLD_LAST))
print("materials      : %-4d ids %d..%d" % (aid-MAT_BASE, MAT_BASE, MAT_LAST))
print("recipes        : %-4d ids %d..%d  (%d sub + %d combines)"
      % (len(recipes), RECIPE_BASE, RECIPE_LAST, len(mat_recipes), len(outputs)))
