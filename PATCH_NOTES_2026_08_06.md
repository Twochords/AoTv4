# AoTv4 Patch Notes — 6 August 2026

## Delves

- **Delve maps have doubled — LDoN is in.** Thirty-four Lost Dungeons zones join the six Dragons of
  Norrath ones: Deepest Guk, Miragul's Menagerie, Mistmoore's Catacombs, the Rujarkian Hills,
  Takish-Hiz and more.
  - The ladder now **alternates**: odd levels are a Dragons of Norrath dungeon, even levels are a
    Lost Dungeons one, so you see both equally as you climb.
  - **Each level is still a fixed, known dungeon** — you can see what you are walking into and plan
    for it. Repeat visits to the same dungeon use a different layout, so it is not the same run twice.
- **The ladder now opens up to your own level, plus one.** A character who has never delved no longer
  starts at level 1 and grinds up through content far beneath them — the list reaches one step above
  wherever you are. Clearing deeper than your level still keeps those levels open after you die back
  to level 1.
- **Delve bosses are less of a slog.** A warden is now **6x** the health of a regular creature in that
  delve, down from 7.5x.

## Fixed

- **A level 1 delve could spawn level 21 creatures.** Gear scales a delve up, but the cap on how far
  it could push was a flat 20 levels no matter which delve you entered — so at the bottom of the
  ladder it completely overwhelmed the level you actually picked. If you were reported as "joined a
  level 1 and the mobs were level 21", that is exactly what happened.
  - The cap is now proportional to the delve's own level: a level 1 delve can be pushed to 2, a level
    10 to 15, a level 30 to 45. **Deep delves are unchanged** — the old cap still applies from about
    level 40 up, so only the low end moves.
  - Gear still scales delves; it can simply no longer turn a beginner delve into a death trap.
- **Your run history names the dungeon again**, and now records it properly rather than guessing it
  from the level. Runs recorded before today will show "unknown" — that information was never stored
  and cannot be recovered.
