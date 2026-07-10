AoTv4 - Client Files
====================

Copy the files below into your EverQuest (RoF2) client folder -- the folder that
contains eqgame.exe. Close EQ first.

Windows path to this folder:  C:\AoTv3\AoTv4\AoTClientFiles


WHAT GOES WHERE
---------------

1) dinput8.dll   *** NOT in this folder -- YOU build it ***
   - Compile in Visual Studio from:
       .devcontainer\repo\eq-core-dll\src\eq-core-dll-vs2022.vcxproj
       (Release / Win32, toolset v143)
   - Copy the built dinput8.dll next to eqgame.exe.
   - This one dll carries ALL client-side features (map overlay, reward/portal/
     shop windows, skill unlock, etc.). EQ locks it while running -- close EQ
     before copying.

2) mq2_map.ini   -> next to eqgame.exe
   - Config for the MQ2-style map overlay (/nimap, /mapfilter, ...).
   - Optional: without it the overlay uses built-in defaults.
   - The overlay writes mq2_map.log next to eqgame.exe for diagnostics.

3) SkillCaps.txt -> EQ client root (next to eqgame.exe)
   - Makes the client's Skills window match the server's skill levels
     (Dual Wield @17, Double Attack @32, Triple Attack @58, raised Bard caps).
   - Recommended. The server enforces the actual behavior with or without it.

4) spells_us.txt / dbstr_us.txt / BaseData.txt -> EQ client root  (OPTIONAL)
   - Keep spell names/levels, DB strings, and per-level base data in sync with
     the server. Not required to play; nice for accuracy.


QUICK CHECK
-----------
- Server-only features need NO client files (e.g. #autoskill, permanent buffs,
  faction, no-reagents, tutorial changes). Just relog.
- The MAP needs: your compiled dinput8.dll + mq2_map.ini, then /nimap on.
