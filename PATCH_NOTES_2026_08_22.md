# AoTv4 Patch Notes — 22 August 2026

## Class Abilities

**Every class now has three combat abilities of its own**, earned at levels 1, 5 and 10 and used from
the Combat Abilities window. Paladins were the last class still receiving theirs as alternate
advancements; they now work the same way as everyone else.

- **They no longer drain your endurance.** Every ability with a duration was quietly costing 10
  endurance a tick — for a level 5 Druid that was the entire bar over one cast of Wildgrowth. The
  drain is gone, and the ability also no longer ends early when the bar empties.
- **Every ability now says what it did.** Damage, healing, absorbs, cooldown reductions and how many
  targets an area attack found are all reported in chat. Several abilities were working perfectly and
  simply had no way of telling you.
- **All 48 descriptions have been rewritten** to say what the ability actually does, with real
  numbers. They previously described the flavour and none of the effect.
- Abilities are **removed when you change class**. Reforging left the old class's abilities on your
  bar, still usable.
- Fixed the Combat Abilities window continuing to show abilities the server had already taken away —
  most visibly after dying and changing class.

### Specific abilities

- **Void Stance** (Monk) — a charge is now spent only when a blow actually **lands**. It was being
  spent on every swing at you, hit or miss, so the buff consumed itself faster the better it worked
  and was usually gone within a couple of rounds.
- **Spiritual Foresight / Crippling Spirit** (Shaman) — the ward was refreshing faster than it could
  be spent, which made Shamans effectively unkillable. Tier 1 is now a short ward on its cooldown, and
  both now scale sensibly instead of being worth five to ten hits at low level.
- **Wildgrowth** (Druid) — was applying its healing as a silent regeneration bonus. It is now a real
  heal over time, which also means healing gear and focus effects finally apply to it.
- **Ley Tap / Overload** (Wizard) — gathered ley threads now show as a **visible buff**, so you can
  see how many you are holding before you commit to an Overload.
- **Discordant Strike / Cadence Strike** (Bard) — both were working; neither said so. They now report
  the armour stripped and the extra damage your target will take.
- **Toll of the Dead / Soul Harvest** (Necromancer) — now use the **real** remaining damage on your
  target's afflictions instead of assuming three ticks each.

## Healing

- **Heals now report in chat, including every tick of a heal over time.** The message threshold was
  set for a game with ten times our health totals, so essentially every heal on the server was silent.
- Fixed heal amounts being applied incorrectly for characters with healing gear.
- The Circle of Renewal line was also applying its healing as a silent regeneration bonus and is now a
  proper heal over time.

## Alternate Advancement

- **AA is earned 30 percent faster.**
- **The AA experience slowdown is switched off.** It was reducing your normal experience as you
  accumulated AA, which also meant fewer AA per kill the more you had.

## Combat

- **Bows have no minimum range.** You can fire point blank, in melee, without backing away first. This
  also applies to thrown weapons — and to enemy archers, who can now shoot you at close range too.
- **Skin of the Reptile** now wards **5** hits rather than 72. At 72 the ward could never run out
  during its duration, so it was a passive heal rather than something you spend.

## The Gilded Wager

**A new NPC who trades coin for a random piece of gear**, standing in the Theater of the Tranquil.

- **1000 platinum** buys a random wearable as it is normally found.
  **5000** buys it Hallowed. **10000** buys it Mythic.
- The prizes are drawn only from **regions you have unlocked**, and never from above your own level —
  he deals only in things you could have found yourself.
- Click the price in his reply and the coin comes out of your purse; the prize lands on your cursor.
  Handing him coin in the trade window works too.
- He speaks to you in a tell rather than out loud, and any wager he cannot fill returns your money.

## Travel

- **Travelling now requires a Plane of Knowledge book, as intended.** The travel window still opens
  anywhere from the menu as a map of what you have found, but it will not carry you unless you are
  reading a book. Two ways around that have been closed.

## Advanced Loot

- **The filter list no longer silently loses rules.** Past roughly 94 entries the rest simply were not
  shown, and because the list is ordered by item id it was always the highest ones that vanished —
  which is why velium and blood runed gear "would not appear" while everything else looked fine. The
  list now tells you how many it could not fit.
- The **Never** button is a toggle, and pressing it on something that already has the rule removes it.
  It now says so plainly instead of quietly undoing your rule.
- Setting a Never rule while **Apply Filters is off** now warns you that it will not do anything yet.

## Fixes

- Fixed being unable to enter some zones, including Crushbone. The server had fewer zone ports
  configured than zones it was allowed to run, so a zone could simply fail to start with no message.
