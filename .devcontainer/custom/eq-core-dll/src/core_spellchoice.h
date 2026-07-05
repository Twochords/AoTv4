#pragma once

// Level-up spell-choice overlay (DirectX9). Implementation in core_spellwindow.cpp.
// Server sends a plain chat line:  SPELLCHOICEDATA name1^name2^name3
// We swallow it, show a drawn overlay with 3 choices + Select buttons, and a click
// makes the client "/say spellpick <N>" which the server consumes.
void EnableSpellChoiceWindow();

// Death-driven AA-choice overlay (separate window, shares the spell window's hooks).
// Server sends:  AACHOICEDATA <banked>^name|icon|cost|cls^name|icon|cost|cls^...
// We swallow it, show a drawn overlay with banked points + 3 choices (name, cost, class marker)
// + Train buttons, and a click makes the client "/say aapick <N>" which the server consumes.
void EnableAAChoiceWindow();

// Hotkey-opened "Discovered Portals" travel window (shares the spell/AA window hooks).
// Server sends:  PORTALDATA short|Long^short|Long^...   (the player's discovered PoK-book zones)
// Hotkey toggles the window + requests a refresh ("/say portalreq"); a row click travels
// ("/say portalgo <short>"). See core_spellwindow.cpp.
void EnablePortalWindow();

// "You Lost" death window: server sends  LOSTDATA name^name^...  on death; we show a scrollable
// list of destroyed items. Right-click / idle-timeout to dismiss. See core_spellwindow.cpp.
void EnableLostWindow();

// The Reward Journal: one Quest-Journal-styled tabbed window holding the Spell, AA, and Lost
// panels (the three above become its tabs instead of separate windows). Auto-opens on
// level-up/death to the relevant tab and toggles with Ctrl+W. Portal stays its own window.
// Enabling this also enables the spell/AA/lost chat parsers. See core_spellwindow.cpp.
void EnableJournalWindow();

// The Set-Up-Shop (vendor) price window: server sends VENDORDATA id|name|vendorvalue^... for the
// player's Trader's Satchel; the window prices each item (real platinum) and "/say vpset.../vshop"
// back to open an AFK shop in any city. See core_bazaar.h note + core_spellwindow.cpp.
void EnableVendorWindow();
