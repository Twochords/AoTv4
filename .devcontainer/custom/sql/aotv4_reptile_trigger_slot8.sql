-- ============================================================================================
-- AoTv4 -- clear the inherited stock heal in slot 8 of the reptile triggers (2026-08-02)
--
-- ============================================================================================
-- ⚠️⚠️ EVERY TIER OF THE REPTILE LINE HEALED A FLAT 393, REGARDLESS OF TIER
-- ============================================================================================
-- Reported from play as "Skin of the Lizard is healing for 287" and later 393 and 215. It was never
-- a scaling problem, a focus, a critical, gear stat scaling or a stacking problem -- all of which
-- were investigated and ruled out. The trigger spells simply have TWO heals in them:
--
--     slot 1   SPA  79  CurrentHPOnce   20 / 45 / 85 / 140 / 215 / 300   <- the intended tier value
--     slot 8   SPA 100  HealOverTime    393 on ALL SIX                   <- inherited, never cleared
--
-- The line was cloned from stock 8009 `Skin of the Rep. Trigger` (a level 68 druid spell), and 8009
-- carries 393 in BOTH slot 1 and slot 8. The clone overrode slot 1 with the tier ladder and left
-- slot 8 untouched, so a level 8 Skin of the Newt -- nominally a 20 point heal -- was actually
-- healing 20 + 393.
--
-- ⚠️ The observed numbers varied (287, 393, 215) purely because a heal reports what it RESTORED, and
-- that is capped by the player's missing health. 393 was the real figure every time. That variance
-- is what made it look like a multiplier, and it is why chasing a multiplier found nothing: the base
-- really was 393, which the Spells log said outright once it was enabled --
--     spell [43351] formula [100] base [45]  max [0] lvl [20]
--     spell [43351] formula [100] base [393] max [0] lvl [20]
-- two evaluations of one spell, two different bases. That single pair of lines is the whole answer.
--
-- ============================================================================================
-- ⚠️⚠️ THE LESSON: CHECK ALL TWELVE SLOTS OF A CLONE SOURCE, NOT JUST THE ONE YOU ARE OVERRIDING
-- ============================================================================================
-- Section 5 already warns "the proc SPA sits in slot 4 on the reptile line but slot 3 on the sloth
-- line -- check the template before overriding", and separately that cloning inherits formulaN/maxN.
-- Both warnings are about the slot you are WRITING. This is the other half: a stock spell can carry
-- a real effect in a slot you never look at, and slots 7-12 are invisible to every casual query
-- because they are almost always 254.
--
-- 📌 An audit of the whole 43300-43399 band found the reptile triggers to be the ONLY offender --
-- every other custom line is clean in slots 7-12. That is also why this survived so long: it is a
-- single line's mistake in a place nothing else shares.
--
-- ⚠️ Slot 8 is cleared to 254 (Blank) rather than being retuned into a per-tier HoT. The line is
-- designed as a flat heal per hit taken and its tuning ladder lives in slot 1; adding a second,
-- parallel heal would mean two ladders to keep in step for no design gain.
--
-- ⚠️⚠️ `spells_new` IS SHARED MEMORY. This is INERT until:
--   stop world + zones  ->  cd build/bin && ./shared_memory  ->  restart world
-- ============================================================================================

UPDATE spells_new
   SET effectid8 = 254, effect_base_value8 = 0, effect_limit_value8 = 0, formula8 = 0, max8 = 0
 WHERE id BETWEEN 43350 AND 43355;

-- Verification: slot 1 keeps the tier ladder, slot 8 is blank, and no other slot 7-12 is populated.
SELECT id, name,
       effect_base_value1 AS tier_heal,
       effectid8          AS slot8_effect,
       effect_base_value8 AS slot8_value,
       CONCAT_WS(' ',
         IF(effectid7  NOT IN (10,254), CONCAT('s7=',effectid7),  NULL),
         IF(effectid9  NOT IN (10,254), CONCAT('s9=',effectid9),  NULL),
         IF(effectid10 NOT IN (10,254), CONCAT('s10=',effectid10),NULL),
         IF(effectid11 NOT IN (10,254), CONCAT('s11=',effectid11),NULL),
         IF(effectid12 NOT IN (10,254), CONCAT('s12=',effectid12),NULL)) AS other_high_slots
FROM spells_new WHERE id BETWEEN 43350 AND 43355 ORDER BY id;
