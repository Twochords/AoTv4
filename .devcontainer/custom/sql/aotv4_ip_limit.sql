-- aotv4_ip_limit.sql
-- Anti-box: limit to ONE simultaneous client connection per IP. The roguelite is a solo design --
-- boxing (multiple accounts on one machine) would trivialize progression and the death loop.
-- Enforced by world/clientlist.cpp against World:MaxClientsPerIP (count > limit -> excess disconnected).
--
-- GMs/admins (account status >= 250) are EXEMPT via World:ExemptMaxClientsStatus so staff can still run
-- multiple clients for testing/moderation. For a HARD limit that also applies to GMs, set that rule to -1.
--
-- These are WORLD rules -- read once at WORLD BOOT -- so a WORLD RESTART is required to apply (not a zone
-- bounce, not shared_memory). Ruleset 1 = 'default' (the active set, where Expansion:CurrentExpansion lives).

INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes) VALUES
  (1, 'World:MaxClientsPerIP', '1', 'AoTv4: one client per IP (anti-box)')
  ON DUPLICATE KEY UPDATE rule_value = VALUES(rule_value), notes = VALUES(notes);

INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes) VALUES
  (1, 'World:ExemptMaxClientsStatus', '250', 'AoTv4: GMs/admins (status >= 250) exempt from the IP limit')
  ON DUPLICATE KEY UPDATE rule_value = VALUES(rule_value), notes = VALUES(notes);

-- Over-limit action: DISCONNECT ALL sessions on the offending IP (both the existing session AND the new
-- one), instead of merely refusing the new login. So logging a second client over your first immediately
-- boots the whole IP -- a hard anti-box measure. GMs (>= ExemptMaxClientsStatus) are still uncounted/exempt.
INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes) VALUES
  (1, 'World:IPLimitDisconnectAll', 'true', 'AoTv4: over-limit -> disconnect every session on that IP')
  ON DUPLICATE KEY UPDATE rule_value = VALUES(rule_value), notes = VALUES(notes);
