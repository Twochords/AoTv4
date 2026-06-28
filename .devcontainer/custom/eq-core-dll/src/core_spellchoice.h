#pragma once

// Level-up spell-choice overlay (DirectX9). Implementation in core_spellwindow.cpp.
// Server sends a plain chat line:  SPELLCHOICEDATA name1^name2^name3
// We swallow it, show a drawn overlay with 3 choices + Select buttons, and a click
// makes the client "/say spellpick <N>" which the server consumes.
void EnableSpellChoiceWindow();
