-- aotv4_proc_tiers.sql
-- Tier proc-rate scaling: Hallowed procs 2x as often as its native base, Mythic 2x Hallowed (= 4x native).
-- items.procrate is a PERCENT bonus to proc chance (WPC = base * (100 + procrate) / 100), so doubling the
-- value doubles the bonus. Native is preserved as-is; only proc items (proceffect > 0) are touched. A
-- native with procrate 0 stays 0 across all tiers (nothing to double).
--
-- items are SHARED MEMORY -> after this, rebuild ./shared_memory + restart world. Idempotent (always
-- recomputes tiers from the current native value).

-- Floor of 10/20 so every proc tier is an upgrade even when native is 0 or negative (some items ship
-- with a negative procrate = rarer proc); weapons that already proc more scale past the floor by 2x/4x.
-- Mythic always works out to 2x Hallowed.
UPDATE items h JOIN items n ON n.id = h.id - 1000000
  SET h.procrate = GREATEST(10, n.procrate * 2)
  WHERE h.id BETWEEN 1000000 AND 1999999 AND n.proceffect > 0;

UPDATE items m JOIN items n ON n.id = m.id - 2000000
  SET m.procrate = GREATEST(20, n.procrate * 4)
  WHERE m.id BETWEEN 2000000 AND 2999999 AND n.proceffect > 0;
