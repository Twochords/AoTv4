# Native SIDL Shop Window (`/shop`) — client install

The `/shop` window is now a **real native EQ SIDL window** (`ShopWnd`), skinned by the client's own UI
engine (like the achievement window) instead of the self-drawn GDI overlay. It renders in **any zone** —
no Bazaar zone required. Backend is unchanged (`SHOPINVDATA`/`MYSHOPDATA` chat + `/say shopadd/shoppull`).

## Install (client side)
1. Rebuild **our `dinput8.dll`** (the `ShopWnd` C++ + routing live in `core_spellwindow.cpp`).
2. Copy **`EQUI_ShopWnd.xml`** → `<EQ>/uifiles/default/`.
3. Add this line inside `<EQ>/uifiles/default/EQUI.xml` (next to the other `<Include>` lines):
   ```xml
   <Include>EQUI_ShopWnd.xml</Include>
   ```
4. Launch, run **`/trader`** (or `/shop`). The native "Trader" window opens with **three tabs**:
   **Set Price / List Item / My Shop**.

## Flow (three tabs)
- **Set Price tab:** select a bag item → type P/G/S/C → **Set Price**. The price is saved to a persistent
  per-character **price book** (survives relog). The panel below shows that item's **price history**
  (old → new, when). The Price column shows the item's current book price (or "-- set price --").
- **List Item tab:** select a **priced** item → type a **Quantity** (blank = whole stack) → **Add to Shop**
  (escrows that many out of your bags into a listing). Price comes from the book — no re-typing.
- **My Shop tab:** select a listing → **Pull Item** (returns it to your cursor).
- Sales pay you online (any zone) or bank to next login; buyers find your shop via the global `/bazaar`.

## Notes
- If the window doesn't appear, check `native_achievements.log` behavior for the SIDL manager wiring — the
  shop reuses the same `ppSidlMgr`/`ppWndMgr` fix (`EnsureShopWindow` sets them from client globals if unset).
- The old GDI shop overlay code is still in the dll but is never shown (its open trigger now opens `ShopWnd`).
