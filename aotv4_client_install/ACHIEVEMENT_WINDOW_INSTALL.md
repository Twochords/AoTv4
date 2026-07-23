# Native Achievement Window — client install

The achievement system is server-driven; these are the **client-side** files that render the native window.

## 1. dinput8.dll (rebuild ours)
The achievement module (`core_achievements_native.cpp/.h`) is integrated into **our** `eq-core-dll`
(`eq-core-dll-vs2022.vcxproj`, toolset v143). Rebuild `dinput8.dll` and drop it next to `eqgame.exe`.
Do NOT use the reference's prebuilt dll — it's a minimal standalone; ours has all our other features too.
(Feature flag: `areAchievementsNativeEnabled = true` in `_options.h`.)

## 2. UI files -> `<EQ>/uifiles/default/`
Copy from `uifiles_default/` here:
- `EQUI_NativeAchievementWnd.xml`
- `Achievement_*.tga`  (16 art files)

## 3. Register the window in `EQUI.xml`
Edit `<EQ>/uifiles/default/EQUI.xml` and add this line among the other `<Include>` entries:

```xml
<Include>EQUI_NativeAchievementWnd.xml</Include>
```

## 4. Use it
In-game: **`#ach`** (or `#ach window`) opens/refreshes the native window; `#ach status`, `#ach check`,
`#ach category <id>`, `#ach detail <id>` also work. The window is a display/input surface only —
the server owns all progress, completion, and rewards over the `ACH|...` chat transport (swallowed by
the dll, routed through our existing `dsp_chat`/`InterpretCmd` hooks, so it coexists with the other
overlays).

## Notes
- The server side is already live; the `#ach` **text** commands work without any client changes. Steps
  1–3 are only needed for the **native window UI**.
- If the window doesn't appear: confirm the `EQUI.xml` include, that the `.tga`s are in `uifiles/default/`,
  and that you're running the freshly-built dll (windowed mode).
