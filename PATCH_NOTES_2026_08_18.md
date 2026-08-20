# AoTv4 Patch Notes — 18 August 2026

## Fellowships — new

A lasting group that survives logging out. Speak to **Fellowmaster Denara** in the Titan Hall.

- Up to **12 members**, bound across every zone — one roster, and a chat channel that reaches your
  kin wherever they are standing
- Open it from the **AoT menu → Fellowship**, or `/fellowship`
- An invitation is now an **offer** — the person you invite gets Accept and Decline and has two
  minutes to answer
- **Campfires.** Buy a Campfire Kit and a Lumber Bundle from Kindler Bram, combine them, and click
  what comes out — a fire appears at your feet and your fellowship can travel to it
- Three kinds: a plain fire, one that wards, one that restores vigour
- **One campfire at a time.** Lighting a second is refused rather than moving the first
- A fire burns for four hours, or thirty minutes unattended
- **Fellowship Insignia** — carry it and click it to return to your campfire. Out of combat, outside
  any instance, and only into a region you have already opened
- The insignia **survives death**, unlike almost everything else you carry

## The Titan Hall

- **The hub is open to everyone.** It was reachable only by staff — three separate locks, any one of
  which refused you
- **Fellowmaster Denara has joined the induction** as an eleventh lesson, and hands out your insignia
- The Plane of Knowledge book in the hall is now **an actual book** instead of a notice board, and is
  wired into the travel network — available to everyone from level 1, with nothing to discover
- The hall is **two-way**: travel to it from any book, and from it to anywhere you have opened
- Everyone now arrives at the same doorstep, whether by book or by Origin
- **Fixed:** entering the hall teleported you to the doorstep on *every* login, not just your first

## Challenge modes

- **Creatures now fight like their class.** A Rogue hits harder, a Warrior is tougher, a Wizard has
  the wits to cast. This applies everywhere, including Normal
- **Difficulty rebalanced.** Health and damage both come down from the previous pass — a harder world
  should mean longer, more dangerous fights, not being deleted from full health
- Health: Nightmare ×1.5 · Hell ×2 · Inferno ×2.5
- Damage now steps up gently on top of the class difference, rather than multiplying with it
- **Thorned/Barbed champions removed**

## Fixes

- **Fixed the Travel window crashing the client.** Opening a region and pressing Travel without
  choosing a destination — or choosing a row you have not discovered yet — took the client down
  every time. It now tells you to pick one
- **The Induction now actually completes.** Finishing Fellowmaster Denara's lesson never ticked its
  line on the record, so the Induction could not be finished by anyone. It also pays the Fellowship
  Insignia it always should have
- **Fellowship chat commands no longer say themselves out loud.** They were hidden from everyone
  standing nearby but still shown to the person pressing the button — the wrong way round
- **`/shield` no longer announces itself** to everyone in earshot
- **Destroy Camp did nothing at all** — the command had no handler
- Campfire buffs lasted **six seconds** instead of three minutes
- The campfire opened like a container when clicked, and could not be put out
- Creating a fellowship lowercased its name, and invites by name silently failed
- The old Plane of Knowledge overlay no longer opens alongside the Travel window
- Travelling from the Travel window failed outright — it had never worked
- The tradeskill skill filter is now only on the **Artisan's Universal Kit**, where it means
  something; picking Blacksmithing at a baking table returned nothing, correctly, and looked broken
- Fixed a database error when using that filter
- Clicking a Tome of Insight announced a weapon buff it does not grant
- Fixed placed doors and objects in the hall silently failing to save

## Notes for staff

- Database migrations run to **v91**; they apply themselves on world boot
- Client files changed: `freeporttheater.eqg`, `EQUI_AoTFellowshipWnd.xml`, `EQUI_AoTMenuWnd.xml`,
  `EQUI_AoTDifficultyWnd.xml`, `spells_us.txt`, `dbstr_us.txt`, `Resources/GlobalLoad_chr.txt`,
  plus an `EQUI.xml` include line — and a rebuilt `dinput8.dll`
