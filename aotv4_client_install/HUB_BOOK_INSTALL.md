# Hub PoK book — client install

The Titan Hall's Plane of Knowledge book renders as an actual book instead of a Freeport bulletin
board. **Two halves, and shipping one without the other breaks it.**

1. Copy **`eqg/freeporttheater.eqg`** (152 files, 8,431,587 bytes) over `<EQ>\freeporttheater.eqg`.
2. Server migration **v87** points the door at `OBJ_POK_BOOK_`.

⚠️⚠️ **Apply v87 without shipping the archive and the book goes INVISIBLE.** A door's model resolves
against the ZONE'S OWN `.eqg`, and the stock `freeporttheater.eqg` contains `obj_fp_bboard` and
nothing book-like. A name that does not resolve creates a door that is present, findable and
clickable, and draws an empty hole — no error, no `UIErrorLog.txt` entry, nothing in any log. Same
silent failure §3 records for a mis-cased texture name.
📌 That is also why the board was there in the first place: it was not a placeholder anyone forgot to
replace, it was the only model the zone had.

## How the archive was built

`obj_pok_book_.mod` plus the nine textures it references were merged out of the client's standalone
`pok_book.eqg` using `.devcontainer/custom/tools/add_book_to_hub.py`.

- ⚠️⚠️ **It MERGES, it does not rebuild.** The hub archive already carries hand-added content — the
  eight sakura trees. Repacking from a pristine copy silently deletes them and every placed tree goes
  invisible while its `doors` row still points at the missing model. The script asserts the original
  file count and every original byte survives.
- ⚠️ **Textures are found by scanning the WHOLE `.mod`, not its string pool.** The string pool alone
  missed `ra_sakuratree_03.dds` on the tree pass; the same shortcut here would have dropped a texture
  and left the book part-untextured rather than obviously broken.
- ⚠️ `book_stump_bark_01_c.dds` is in both archives and is **skipped**, not added twice — two
  directory entries under one CRC.
- 📌 `grid_standard.dds` is referenced by the model and ships in **neither** archive. Stock
  `pok_book.eqg` does not carry it either and the book renders correctly on live, so it is a dead
  reference, not a missing file.
- ⚠️ The `doors.name` column is UPPERCASE (`OBJ_POK_BOOK_`) while the archive member is lowercase
  (`obj_pok_book_.mod`). Normal here — the eight `OBP_TREE_SAKURA` rows are the same.

## ⚠️⚠️ Never allocate a door id above 254

Unrelated to the model, and it cost a placed portal: `Door_Struct.doorId` is **uint8** and
`Doors::m_door_id` is uint8, so 254 is a hard ceiling. Worse, `Doors::CreateDatabaseEntry` aborts if
`MAX(doorid) + 1 - 1 >= 255` — and `#door save` prints *"Door saved"* regardless, because it calls a
void function and messages unconditionally. The hub's book was at doorid 300, which silently disabled
`#door save` **for the whole zone**. Migration **v86** moved them to 12 and 13.
