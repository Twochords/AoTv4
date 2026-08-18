#!/usr/bin/env python3
"""Bundle the Plane of Knowledge book model into a zone archive.

⚠️⚠️ A DOOR'S MODEL MUST BE IN THE ZONE'S OWN .eqg. `freeporttheater.eqg` ships `obj_fp_bboard` and
nothing book-like, so setting doors.name to OBJ_POK_BOOK_ draws NOTHING AT ALL -- no error, no
UIErrorLog entry, the same silent failure section 3 records for a mis-cased texture name. The model
has to be bundled first; only then does the name resolve.

⚠️ MERGE, NEVER REBUILD FROM THE ORIGINAL. The hub archive already carries hand-added content (the
eight sakura trees). Repacking from a pristine copy silently deletes it and every placed tree becomes
invisible, with the doors rows still sitting there pointing at a model that is no longer present.

⚠️ Skips any file the target already has, by name. `book_stump_bark_01_c.dds` is in both archives;
adding it twice would put two directory entries under one CRC.

Usage:  add_book_to_hub.py <target.eqg> <staged_dir> <output.eqg>
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pfs

def main(target, staged, out):
    # ⚠️ `pfs_write` takes an ORDERED LIST of (name, data), not a dict -- the directory blob is
    # written in file order and the CRC table is sorted separately, so the order is part of the
    # format rather than incidental. Existing files keep their original order; new ones append.
    files, buf = pfs.pfs_list(target)
    items = [(e['name'], pfs.read_blocks(buf, e['off'], e['size'])) for e in files]
    before = len(items)
    have = {n.lower() for n, _ in items}

    added, skipped = [], []
    for name in sorted(os.listdir(staged)):
        if name.lower() in have:
            skipped.append(name)
            continue
        items.append((name, open(os.path.join(staged, name), 'rb').read()))
        added.append(name)

    pfs.pfs_write(out, items)

    print(f"{target}: {before} files")
    for n in added:   print(f"  + {n}")
    for n in skipped: print(f"  = {n} (already present)")
    print(f"{out}: {len(items)} files, {os.path.getsize(out)} bytes")

    # ⚠️ Read the result back. The writer is ours (custom CRC over name+NUL, CRC-sorted directory);
    # a malformed archive fails INSIDE the client with no diagnostic, so verify here or not at all.
    back, _ = pfs.pfs_list(out)
    names = {f['name'] for f in back}
    assert len(back) == len(items), f"roundtrip lost files: {len(back)} != {len(items)}"
    for n in added:
        assert n in names, f"roundtrip lost {n}"
    assert 'obj_pok_book_.mod' in names, "the book model is not in the output"
    print("roundtrip OK -- every file reads back")

if __name__ == '__main__':
    main(*sys.argv[1:4])
