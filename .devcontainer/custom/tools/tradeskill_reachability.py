#!/usr/bin/env python3
"""
AoTv4 -- which enabled tradeskill recipes can actually be MADE, and which never can.

A recipe is only as makeable as its worst component. This walks every enabled recipe, resolves each
component to the best region it can be obtained in, and classifies the recipe by the worst one.

  region 0   Always Available  -- makeable today
  region 1-6 a hub             -- makeable once that hub is unlocked
  region 99  "Unused"          -- 378 zones NO player can ever reach: effectively never
  (none)                       -- no source found anywhere: never

⚠️⚠️ SOURCES ARE UNION'd, AND MISSING ONE INVENTS FALSE "NEVER" ANSWERS. An item is obtainable if ANY
of these can produce it, so a source this script does not know about shows up as an unmakeable recipe
that is actually fine. Currently: merchants, loot, ground spawns, forage, quest `summonitem`, and
recursively anything an enabled recipe can craft.

⚠️⚠️ A ZONE WITH NO `zone_regions` ROW IS TREATED AS REGION 0, NOT AS UNREACHABLE. That mirrors
RegionManager::CanEnterZone, which treats an unmapped zone as unrestricted (§24 v27). Getting this
backwards would mark most of the game unreachable.

⚠️ A source only counts if its NPC actually SPAWNS. A merchant defined but never placed sells nothing,
which is exactly the §56 parcel-merchant trap: "does a row exist" is not "does it spawn".

  python3 .devcontainer/custom/tools/tradeskill_reachability.py [--skill N] [--limit N] [--tsv]
"""
import subprocess, sys, collections, re, os, glob, io

DB = ["mysql", "-h127.0.0.1", "-upeq", "-ppeqpass", "peq", "-N", "-B", "-e"]
NEVER = 99
NOSRC = 1000          # sorts after 99; "no source at all"

def q(sql):
    out = subprocess.run(DB + [sql], capture_output=True, text=True, timeout=600)
    if out.returncode != 0:
        sys.exit("query failed: " + out.stderr.strip())
    return [l.split("\t") for l in out.stdout.splitlines() if l.strip()]

# ------------------------------------------------------------------ zone -> region
# ⚠️ COALESCE to 0: an unmapped zone is unrestricted, not unreachable.
ZONE_REGION = {z: int(r) for z, r in q("""
    SELECT z.short_name, COALESCE(MIN(zr.region_id), 0)
    FROM zone z LEFT JOIN zone_regions zr ON zr.zone_id = z.zoneidnumber
    GROUP BY z.short_name""")}

def best(a, b):
    return a if a < b else b

# ------------------------------------------------------------------ item -> best region, per source
item_region = collections.defaultdict(lambda: NOSRC)

def absorb(rows, label):
    n = 0
    for item, zone in rows:
        r = ZONE_REGION.get(zone, 0)
        i = int(item)
        if r < item_region[i]:
            item_region[i] = r
        n += 1
    print(f"  {label:<14} {n:>7} item/zone pairs", file=sys.stderr)

print("resolving item sources...", file=sys.stderr)
absorb(q("""SELECT DISTINCT ml.item, s2.zone FROM merchantlist ml
            JOIN npc_types n ON n.merchant_id = ml.merchantid
            JOIN spawnentry se ON se.npcID = n.id
            JOIN spawn2 s2 ON s2.spawngroupID = se.spawngroupID"""), "merchant")
absorb(q("""SELECT DISTINCT lde.item_id, s2.zone FROM lootdrop_entries lde
            JOIN loottable_entries lte ON lte.lootdrop_id = lde.lootdrop_id
            JOIN npc_types n ON n.loottable_id = lte.loottable_id
            JOIN spawnentry se ON se.npcID = n.id
            JOIN spawn2 s2 ON s2.spawngroupID = se.spawngroupID"""), "loot")
absorb(q("""SELECT DISTINCT gs.item, z.short_name FROM ground_spawns gs
            JOIN zone z ON z.zoneidnumber = gs.zoneid"""), "ground")
absorb(q("""SELECT DISTINCT f.Itemid, z.short_name FROM forage f
            JOIN zone z ON z.zoneidnumber = f.zoneid"""), "forage")

# ⚠️⚠️ FISHING IS A SOURCE AND OMITTING IT PUT 56 PERCENT OF ALL RECIPES IN "no source at all".
# Every blocker in the first run was skill 55, which is what gave it away -- a whole tradeskill's
# inputs come from a table nothing else references. Treated as region 0: you can fish anywhere there
# is water, and the `fishing` rows are not zone-scoped in a way that gates them behind a hub.
absorb(q("""SELECT DISTINCT f.Itemid, z.short_name FROM fishing f
            JOIN zone z ON z.zoneidnumber = f.zoneid"""), "fishing")

# ⚠️ Items a character simply starts with. Small, but they are genuinely free and would otherwise
# read as unobtainable.
absorb(q("""SELECT DISTINCT si.item_id, "ALWAYS" FROM starting_items si"""), "starting")

# ⚠️ World containers and objects placed in zones -- the ground-spawn cousin.
absorb(q("""SELECT DISTINCT o.itemid, z.short_name FROM object o
            JOIN zone z ON z.zoneidnumber = o.zoneid WHERE o.itemid > 0"""), "object")

# ⚠️⚠️ ITEMS SUMMONED BY SPELLS ARE A SOURCE, AND THE MOST IMPORTANT ONE FOR TRADESKILLS. Enchanted
# metal bars, Imbued gems and the Research vellums are not dropped or sold by anything -- they are
# conjured by SPA 32 (SummonItem) spells like `Enchant Platinum` and `Imbue Amber`. Omitting them put
# 54 percent of every recipe in "no source at all".
#
# ⚠️⚠️ AND THE SPELL HAS TO BE LEARNABLE, WHICH ON THIS SERVER MEANS "OFFERED BY THE LEVEL-UP PICKER".
# The picker is the ONLY way to learn a spell here (§3), so a spell in spell_blacklist.lua can never be
# known by anybody -- and an item only that spell can summon is therefore unobtainable no matter how
# many merchants or mobs exist. That is a THIRD category, distinct from "never sold" and "wrong
# region", and it is invisible from the item tables alone.
SPELL_LOCKED = 500      # sorts between a hub (1-6) and region 99
qroot = "/src/.devcontainer/repo/quests"

def lua_ids(path, pat):
    try:
        txt = io.open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return set()
    return {int(m) for m in re.findall(pat, txt)}

blacklist = lua_ids(qroot + "/lua_modules/spell_blacklist.lua", r"\[(\d+)\]\s*=\s*true")
pool      = lua_ids(qroot + "/lua_modules/spell_pool.lua",      r"\{\s*id\s*=\s*(\d+)")
print(f"  pool {len(pool)} spells, blacklist {len(blacklist)}", file=sys.stderr)

summon_rows = q("""SELECT s.id, s.effect_base_value1 FROM spells_new s
                   WHERE s.effectid1 = 32 AND s.effect_base_value1 > 0""")
n_ok = n_locked = 0
for sid, item in summon_rows:
    sid, item = int(sid), int(item)
    learnable = (sid in pool) and (sid not in blacklist)
    r = 0 if learnable else SPELL_LOCKED
    if r < item_region[item]:
        item_region[item] = r
    n_ok += learnable
    n_locked += (not learnable)
print(f"  {'spell summon':<14} {len(summon_rows):>7} spells ({n_ok} learnable, {n_locked} not)", file=sys.stderr)

# ⚠️ Quest-granted items. Without this, anything handed out by a script reads as "no source" -- the
# §23 lesson, where the same omission made every quest look like it required a trip to 200 zones.
pat = re.compile(r"summonitem\s*\(\s*(\d+)")
nq = 0
for path in glob.glob(os.path.join(qroot, "**", "*.lua"), recursive=True) + \
            glob.glob(os.path.join(qroot, "**", "*.pl"), recursive=True):
    try:
        for m in pat.finditer(open(path, encoding="utf-8", errors="replace").read()):
            i = int(m.group(1))
            if item_region[i] > 0:
                item_region[i] = 0     # a quest reward is not region-bound in any way we can read
            nq += 1
    except OSError:
        pass
print(f"  {'quest summon':<14} {nq:>7} grants", file=sys.stderr)

# ------------------------------------------------------------------ recipes
recipes = {int(i): (n, int(sk), int(tv)) for i, n, sk, tv in
           q("SELECT id, name, tradeskill, trivial FROM tradeskill_recipe WHERE enabled = 1")}
comps  = collections.defaultdict(list)
makes  = collections.defaultdict(list)
for rid, item, cc, isc, sc in q("""SELECT recipe_id, item_id, componentcount, iscontainer, successcount
                                   FROM tradeskill_recipe_entries"""):
    rid, item = int(rid), int(item)
    if rid not in recipes:
        continue
    if int(cc) > 0:
        comps[rid].append(item)
    if int(sc) > 0:
        makes[item].append(rid)

# ⚠️⚠️ RESOLVED TO A FIXED POINT, NOT IN ONE PASS. A component may itself be crafted, and that chain
# runs several deep (a bar smelted from ore, a mould made from a bar). One pass would mark a recipe
# unmakeable purely because its sub-recipe had not been resolved yet.
for _ in range(12):
    changed = False
    for rid, cs in comps.items():
        worst = max((item_region[c] for c in cs), default=0)
        for out in [i for i, rs in makes.items() if rid in rs]:
            if worst < item_region[out]:
                item_region[out] = worst
                changed = True
    if not changed:
        break

rows = []
for rid, (name, skill, trivial) in recipes.items():
    cs = comps.get(rid, [])
    if not cs:
        continue
    worst = max(item_region[c] for c in cs)
    blockers = sorted({c for c in cs if item_region[c] == worst}) if worst >= SPELL_LOCKED else []
    rows.append((worst, skill, trivial, rid, name, blockers))

# ⚠️ THE BLOCKER FREQUENCY IS THE FIRST THING TO READ, NOT THE RECIPE LIST. If one item blocks
# hundreds of recipes it is almost always a SOURCE THIS SCRIPT DOES NOT KNOW ABOUT rather than a real
# content gap -- that is how the missing `fishing` table was found, and it is worth re-checking every
# time a bucket looks implausibly large.
blockers_freq = collections.Counter()
for rid, cs in comps.items():
    worst = max(item_region[c] for c in cs)
    if worst >= SPELL_LOCKED:
        for c in cs:
            if item_region[c] == worst:
                blockers_freq[c] += 1

rows.sort(key=lambda r: (-r[0], r[1], r[2]))
buckets = collections.Counter(r[0] for r in rows)
print()
print(f"{'ENABLED RECIPES WITH COMPONENTS':<44} {len(rows)}")
for r in sorted(buckets):
    label = {0: "region 0  makeable now", NEVER: "region 99 UNREACHABLE (Unused)",
             SPELL_LOCKED: "SPELL-LOCKED (summoner is blacklisted)",
             NOSRC: "no source found at all"}.get(r, f"region {r}  needs that hub")
    print(f"  {label:<42} {buckets[r]:>6}")
print()
names = {}
if blockers_freq:
    ids = ",".join(str(i) for i, _ in blockers_freq.most_common(25))
    names = {int(i): n for i, n in q(f"SELECT id, name FROM items WHERE id IN ({ids})")}
    print("TOP BLOCKERS (one item gating many recipes usually means a MISSING SOURCE, not a gap)")
    for item, n in blockers_freq.most_common(25):
        where = ("no source" if item_region[item] >= NOSRC
                 else "region 99" if item_region[item] == NEVER else "spell-locked")
        print(f"  {n:>5} recipes  item {item:<7} {names.get(item,'?')[:40]:<40} {where}")
    print()

lim = int(sys.argv[sys.argv.index("--limit") + 1]) if "--limit" in sys.argv else 40
print(f"{'worst':>5} {'skill':>5} {'triv':>5}  recipe")
for worst, skill, trivial, rid, name, blockers in rows[:lim]:
    if worst < NEVER:
        break
    tag = "NEVER" if worst == NEVER else "NOSRC"
    print(f"{tag:>5} {skill:>5} {trivial:>5}  {name[:52]:<52} blockers={blockers[:4]}")
