# AoTv4 Handoff — Player Shop (permanent escrow trader) + overlay polish

Last updated: 2026-07-02. This document captures the **player-shop** feature (the session's main work),
its architecture, every file touched, how to build/deploy/test, and what's still open. Read alongside
`CLAUDE.md` (project overview) — the shop is now documented there as **§13**.

---

## 1. TL;DR status

| Piece | State |
|---|---|
| Permanent escrow shop **backend** (zone C++ + Lua) | ✅ built + live |
| **Payout** (online paid once natively, offline escrow) | ✅ built + live (double-pay bug fixed) |
| **Two-tab `/shop` window** (Add Items / My Shop) in the dll | ✅ written + synced — **needs a Windows dll rebuild** |
| **Close [X]** on all overlays + **auto-hide when logged out** | ✅ written + synced — **same dll rebuild** |
| Buy from `/bazaar` (search + parcel delivery) | ✅ works |

**The only thing gating a full test is a `dinput8.dll` rebuild on Windows** (see §5). Server side is
fully built, world+zone restarted, running clean.

---

## 2. What the feature is

A player runs a **permanent, escrow-backed shop** from any city — no Bazaar zone, no NPC, no Trader's
Satchel. Flow:

1. `/shop` opens a layered overlay window with two tabs.
2. **Add Items** tab: lists the droppable items in your bags. Type a price, click **Add Priced Items** →
   each item is **escrowed** (removed from your bags) into a `trader` DB row.
3. Buyers find your wares via the native **`/bazaar` → all traders** search; goods ship by **parcel**.
4. On a sale you're paid the listed price — **instantly if you're online**, otherwise banked to
   `shop_escrow_<charid>` and handed over on your **next login**.
5. **My Shop** tab: everything you have listed, each with a **Pull** button → item back to your cursor.

Because items are escrowed (they leave your inventory when listed), there is **no dupe hole** — you
can't stash the physical item while it's also for sale. This replaced an earlier satchel-snapshot design
that had exactly that hole (list items → move the Trader's Satchel to the bank → sell copies + keep the
originals, because the login reconcile only scanned the main-inventory satchel).

---

## 3. Architecture

### 3a. Chat protocol (server ⇄ dll, all rides on chat; the dll swallows every line/echo)
- `/shop` → the dll's InterpretCmd hook rewrites it to `/say shopopen` (reliable path to the server).
- Server → dll (two lines, `MT.NPCQuestSay`):
  - `SHOPINVDATA slot|itemid|name|vendor^…` — droppable bag items (for the **Add Items** tab).
  - `MYSHOPDATA itemsn|itemid|name|cost^…` — current listings (for the **My Shop** tab).
- dll → server (via `/say`, echoes swallowed):
  - `shopadd slot:copper,slot:copper,…` — escrow those bag slots into the shop.
  - `shoppull <itemsn>` — pull one listing back to the cursor.
  - `shoprefresh` — re-send both DATA lines (used on tab-switch, Refresh, after add/pull).

### 3b. Server (C++)
- **`zone/trading.cpp`** (all methods on `Client`, Lua-bound in `zone/lua_client.{h,cpp}`):
  - `GetSellableInventory()` → `SHOPINVDATA` payload (scans `GENERAL_BEGIN..GENERAL_END` + bags; skips
    No-Drop + containers).
  - `AddItemsToShop("slot:price,…")` → **insert `trader` rows FIRST, then delete the items** (loss-safe
    ordering: a DB failure can't destroy items). Unique per-character `item_sn` from a `shopsn_<charid>`
    counter bucket (item-instance serials are runtime-generated and change across relog, so they can't be
    the stable key). Registers the seller as a trader (`SetTrader` + `SendBecomeTraderToWorld`).
  - `GetMyShopListing()` → `MYSHOPDATA` payload (reads the char's `trader` rows).
  - `PullShopItem(itemsn)` → recreate the item to the cursor, delete the row; drops trader status when the
    shop is empty.
  - `BuyTraderItemOutsideBazaar` (native, patched): **does NOT pay the seller** — see the payout note in
    the code. The escrow is credited **only** in the world handler (below).
- **`world/zoneserver.cpp`** — `ServerOP_BazaarPurchase` handler is the **single authoritative** payout
  point. `FindCLEByCharacterID(trader_id)`:
  - **found (seller online, any zone)** → routes the packet to the seller's zone, where
    `zone/worldserver.cpp` pays them via `trader_pc->AddMoneyToPP(price*qty)` (native, unchanged).
  - **not found (seller offline)** → banks `shop_escrow_<trader_id> += price*qty` (`DataBucket`).
  - ⚠️ **Do NOT also pay in the zone-side buy path** — that was the 2× double-pay bug (native + our add).

### 3c. Lua
- **`quests/lua_modules/bazaar_broker.lua`** (kept the filename; it's now just the shop backend, no NPC):
  - `M.handle_global_say(e)` — `shopopen`/`shoprefresh` → send both DATA lines; `shopadd …`; `shoppull …`.
  - `M.pay_escrow(c)` — on login, hand over `shop_escrow_<charid>` (copper → coin) and clear it.
- **`quests/global/global_player.lua`** — `event_say` → `bazaar_broker.handle_global_say(e)`;
  `event_connect` → `bazaar_broker.pay_escrow(e.self)` (no reconcile — the shop is permanent).

### 3d. dll (`.devcontainer/repo/eq-core-dll/src/`)
- **`core_spellwindow.cpp`** — the two-tab shop window (repurposed the old vendor window; reuses the
  coin-field/scroll code for the Add tab). `HandleShopInv`/`HandleMyShop` parse the DATA lines. Also this
  session: **`DrawCloseX`/`CloseXRect`** shared helpers → a red **[X]** in the top-right of the Journal,
  Portal, and Shop windows; and **`IsInGame()`** (`pinstLocalPlayer` @ `0xDD2630` non-null) gates every
  overlay's `WM_TIMER` `active` check so the overlays **auto-hide at char-select/login/zoning**.
- **`core_bazaar.h`** — the InterpretCmd hook that turns `/shop` into `/say shopopen`
  (`areTradeAnywhereEnabled = true`).
- Windows spawned: **Journal** (Spell/AA/Lost tabs, Ctrl+W), **Portal**, **Shop** (`/shop`).

---

## 4. Files changed this session

Server (main git tree — rebuild + restart):
- `zone/trading.cpp` — shop methods (add/pull/listing/inventory), escrow, removed the double-paying payout.
- `zone/client.h`, `zone/lua_client.cpp`, `zone/lua_client.h` — declarations + Lua bindings.
- `world/zoneserver.cpp` — offline-seller escrow in `ServerOP_BazaarPurchase` (+`data_bucket.h` include).
- `zone/pathing.cpp` — silenced the "Total points" GM debug spam (unrelated cleanup).

Quests/dll (edit in `.devcontainer/repo/…`, then `cp` to `.devcontainer/custom/…` — both trees tracked):
- `quests/lua_modules/bazaar_broker.lua`, `quests/global/global_player.lua`, `quests/global/global_npc.lua`.
- `eq-core-dll/src/core_spellwindow.cpp`, `eq-core-dll/src/core_bazaar.h`, `eq-core-dll/src/_options.h`.

Deleted (feature was removed — see §7): `quests/poknowledge/Bazaar_Broker.lua`,
`custom/sql/aotv4_bazaar_broker.sql`, `custom/sql/aotv4_shopkeeper.sql` + their DB rows/npc_types.

---

## 5. Build & deploy

### dll (Windows / Visual Studio) — **required for the shop window + [X] to appear**
Open `eq-core-dll/src/eq-core-dll-vs2022.vcxproj` (toolset v143), build, copy `dinput8.dll` next to
`eqgame.exe` (close EQ first — it locks the dll). Non-default settings are in `CLAUDE.md §8`.

### server (dev container)
```bash
cd /src/build && ninja world zone      # this session touched both
```
Then restart. **World changed → world must restart** (not just zones):
```bash
cd /src/build/bin
for p in $(pgrep -x zone); do kill -9 $p; done
kill -9 $(pgrep -x eqlaunch); kill -9 $(pgrep -x world)
setsid nohup ./world    > logs/world_manual.log    2>&1 < /dev/null &  # detached (see gotcha below)
sleep 18
setsid nohup ./eqlaunch zone > logs/eqlaunch_manual.log 2>&1 < /dev/null &
```
For **Lua-only** changes you don't need a rebuild — just reload zones (`for p in $(pgrep -x zone); do kill
-9 $p; done`, the current eqlaunch respawns them).

> **Two operational gotchas learned this session (also in CLAUDE.md §10):**
> 1. **Detach long-running procs with `setsid … < /dev/null &`.** A foreground shell that times out sends
>    SIGTERM to its whole process group and will kill a plain `nohup ./world &`.
> 2. **Never restart `eqlaunch` while old `zone` procs live.** They become orphans still registered with
>    world → clients get routed to dead zones → "**characters time out on login**." Symptom: many `zone`
>    procs with **pids lower than the current `eqlaunch`**. Fix: kill ALL zones + eqlaunch + world, then
>    restart world → eqlaunch. Keep exactly one eqlaunch.

---

## 6. Test checklist (after the dll rebuild)

1. `/shop` → **Add Items** tab lists your bag items (proves `GetSellableInventory`). Price a **junk** item →
   **Add Priced Items** → it leaves your bag.
2. **My Shop** tab shows it → **Pull** → returns to cursor.
3. **Buy (online seller):** re-add it, second char `/bazaar` → buy → seller gets **exactly the listed price
   once** (the 2× bug is fixed), item leaves My Shop.
4. **Buy (offline seller):** list something, log the seller out, buy it → log seller in → **"earned while
   away"** = single correct amount.
5. **[X]** closes each window (Journal via Ctrl+W to open, Portal via a PoK book, Shop via `/shop`).
6. **Log out to char-select** → all overlays disappear (the `IsInGame` gate).

---

## 7. Removed / abandoned (do NOT resurrect)

Earlier iterations that were explicitly removed at the user's request — don't bring them back:
- **Bazaar Broker NPC** (`npc_types 2000050`) — the shop is `/shop`-only now.
- **Shopkeeper stand-in NPC** (`npc_types 2000051`) — the physical "your wares" merchant; user found it
  bloat and wanted offline mode only.
- **Trader's-Satchel intake + `vpset`/`vshop`/`vclose` protocol** — replaced by add-from-any-bag escrow.
- **Native `Bazaar:UseAlternateBazaarSearch`** — it's Bazaar-zone/instance-sharded, the opposite of
  trader-anywhere; keep it **false**.

---

## 8. Known open items / TODO

- **Earlier double-payments** to the test seller (Ashrem) are still in his bags from before the 2× fix —
  cosmetic, not corrected. Offer to `#charge` it back if the user wants.
- **NPC item-upgrade mechanic** (base item → Hallowed/Mythic tier) — long-standing "later" TODO, untouched.
- The dll **shop window can't be reopened without `/shop`** (no hotkey) — by design; fine.
- No hard cap on number of listings per char (practically bounded by inventory).
