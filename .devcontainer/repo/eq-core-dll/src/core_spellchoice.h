#pragma once

// (The level-up reward picker is a native SIDL window in its OWN translation unit -- see
//  core_spellchoice_native.h / .cpp. It is initialised via InitSpellChoiceNative(), not from here.
//  The old self-drawn spell and AA overlays, and the tabbed Reward Journal that later hosted them,
//  are gone: the native window is the only reward picker, and random AA is retired.)

// Hotkey-opened "Discovered Portals" travel window (shares this TU's dsp_chat + ProcessGameEvents
// detours, like every window below).
// Server sends:  PORTALDATA short|Long^short|Long^...   (the player's discovered PoK-book zones)
// Hotkey toggles the window + requests a refresh ("/say portalreq"); a row click travels
// ("/say portalgo <short>"). See core_spellwindow.cpp.
void EnablePortalWindow();

// (The Advanced Loot window moved to its OWN translation unit -- see core_advloot.h / .cpp.
//  It is initialised via InitAdvLoot(), not from here.)

// "You Lost" death window: server sends  LOSTDATA name^name^...  on death; we show a scrollable
// list of destroyed items. Right-click / idle-timeout to dismiss. See core_spellwindow.cpp.
void EnableLostWindow();

// The Set-Up-Shop (vendor) price window: server sends VENDORDATA id|name|vendorvalue^... for the
// player's Trader's Satchel; the window prices each item (real platinum) and "/say vpset.../vshop"
// back to open an AFK shop in any city. See core_bazaar.h note + core_spellwindow.cpp.
void EnableVendorWindow();

// AoTv4 in-game search window ("allaclone"): tabbed Items/Mobs/Spells lookup. "/search" opens it
// (core_bazaar.h -> ShowSearchWindow); it sends "/say srch <kind> <term>" and renders the server's
// SRCHDATA/SRCHDET replies. See core_spellwindow.cpp.
void EnableSearchWindow();
