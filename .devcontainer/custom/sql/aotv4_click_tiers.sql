-- aotv4_click_tiers.sql
-- Faster click/clicky reuse on higher tiers: Hallowed halves the native reuse timer, Mythic halves it
-- again (= native / 4). Only items with an actual click effect (clickeffect > 0) and a real reuse
-- (recastdelay > 0) are touched; native is preserved. recastdelay is in seconds; FLOOR keeps it integer.
--
-- Note: items with a shared recast group (recasttype >= 0) use the group's timer, so halving the item's
-- recastdelay only fully applies to item-specific clicks (recasttype = -1), which is the vast majority.
--
-- items are SHARED MEMORY -> after this, rebuild ./shared_memory + restart world. Idempotent (always
-- recomputes from the current native value).

UPDATE items h JOIN items n ON n.id = h.id - 1000000
  SET h.recastdelay = FLOOR(n.recastdelay / 2)
  WHERE h.id BETWEEN 1000000 AND 1999999 AND n.clickeffect > 0 AND n.recastdelay > 0;

UPDATE items m JOIN items n ON n.id = m.id - 2000000
  SET m.recastdelay = FLOOR(n.recastdelay / 4)
  WHERE m.id BETWEEN 2000000 AND 2999999 AND n.clickeffect > 0 AND n.recastdelay > 0;
