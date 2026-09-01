# Combat Text — install

Floating damage and healing numbers drawn in the world, with a window to control them.

## Copy

Into `<EQ>\uifiles\default\`:

| file | notes |
|---|---|
| `EQUI_AoTFctWnd.xml` | the Combat Text window |
| `EQUI_AoTFctAnchorWnd.xml` | the two draggable markers |
| `EQUI_AoTMenuWnd.xml` | the AoT launcher, which gained a Combat Text button |

## ⚠️⚠️ Then add BOTH includes to `EQUI.xml`

```xml
<Include>EQUI_AoTFctWnd.xml</Include>
<Include>EQUI_AoTFctAnchorWnd.xml</Include>
```

**A copy on its own does nothing.** `CCustomWnd` resolves its screen by NAME against the parsed UI, so
a file the client never parsed leaves the lookup returning null — no window, no error, no line in any
log. It presents as a button that does nothing at all, and it is the single most common cause of
"nothing happened" for every native window in this dll.

Both windows now say so in chat rather than failing silently, but only from the build that carries this
note; an older dll is quiet.

## Rebuild the dll

`core_fctwindow.cpp` is in the project. Close EQ first — it locks `dinput8.dll`.

## Server

The damage and healing numbers are sent BY THE SERVER, not read from packets, so a `zone` rebuild is
required and the zones must be restarted onto it. There is no migration and no shared-memory rebuild.

⚠️ The client announces itself with `/say fctheal 1`; nothing is sent to a client that has not. So a
player on a stock client sees no transport text, and a dll with the feature disabled costs the server
nothing.

## Checking it works

`/fct` opens the window. `/fct` with arguments still works as a text command if the XML is missing:
`/fct on|off|mine|taken|others|size N|rise N|fade N|reset`.

The status line in `/fct` breaks down what happened to each incoming number:

```
hits seen 214:  drawn 191, no damage 68, filtered 0, no spawn 0
```

Those separate the failure modes that look identical in game — nothing arriving, arriving but filtered
out by the Show toggles, arriving but the target spawn cannot be resolved, or arriving and drawn.
`/fct reset` zeroes them so a controlled test means something.
