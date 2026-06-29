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
