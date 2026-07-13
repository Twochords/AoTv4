-- hotzones.lua -- AoTv4 daily-rotating Hot Zones (flat 2x EXP).
--
-- Deterministic from the calendar date, so NO cron / "midnight roll" is needed: every zone process
-- computes today's hot set the same way, which means the #hotzone popup, the connect welcome message,
-- and the actual XP apply (global_player.event_enter_zone -> eq.set_hotzone) always agree.
--
-- The 2x itself is delivered by the native hotzone path in exp.cpp:
--     modifier = Character:ExpMultiplier (0.65);  if IsHotzone(): modifier += Zone:HotZoneBonus
-- With Zone:HotZoneBonus = 0.65  ->  0.65 + 0.65 = 1.30 = 2 x 0.65  (exactly 2x the normal rate).
-- (Keep HotZoneBonus in step with ExpMultiplier: HotZoneBonus = ExpMultiplier for 2x.)

local M = {}

M.HOT_PER_DAY = 6   -- how many zones are hot each day

-- curated classic/era leveling pool (short_name, long_name) -- verified against the zone table.
M.POOL = {
  { s = "blackburrow",  l = "Blackburrow" },
  { s = "befallen",     l = "Befallen" },
  { s = "chardok",      l = "Chardok" },
  { s = "eastkarana",   l = "Eastern Plains of Karana" },
  { s = "everfrost",    l = "Everfrost Peaks" },
  { s = "beholder",     l = "Gorge of King Xorbb" },
  { s = "highkeep",     l = "High Keep" },
  { s = "highpass",     l = "Highpass Hold" },
  { s = "innothule",    l = "Innothule Swamp" },
  { s = "kaesora",      l = "Kaesora" },
  { s = "karnor",       l = "Karnor's Castle" },
  { s = "kedge",        l = "Kedge Keep" },
  { s = "kerraridge",   l = "Kerra Isle" },
  { s = "kithicor",     l = "Kithicor Forest" },
  { s = "lakerathe",    l = "Lake Rathetear" },
  { s = "soldungb",     l = "Nagafen's Lair" },
  { s = "najena",       l = "Najena" },
  { s = "nro",          l = "Northern Desert of Ro" },
  { s = "oasis",        l = "Oasis of Marr" },
  { s = "hateplane",    l = "Plane of Hate" },
  { s = "soldunga",     l = "Solusek's Eye" },
  { s = "sro",          l = "Southern Desert of Ro" },
  { s = "burningwood",  l = "The Burning Wood" },
  { s = "mistmoore",    l = "The Castle of Mistmoore" },
  { s = "guktop",       l = "The City of Guk" },
  { s = "citymist",     l = "The City of Mist" },
  { s = "dalnir",       l = "The Crypt of Dalnir" },
  { s = "crystal",      l = "The Crystal Caverns" },
  { s = "dreadlands",   l = "The Dreadlands" },
  { s = "unrest",       l = "The Estate of Unrest" },
  { s = "feerrott",     l = "The Feerrott" },
  { s = "charasis",     l = "The Howling Stones" },
  { s = "paw",          l = "The Lair of the Splitpaw" },
  { s = "runnyeye",     l = "The Liberated Citadel of Runnyeye" },
  { s = "nurga",        l = "The Mines of Nurga" },
  { s = "northkarana",  l = "The Northern Plains of Karana" },
  { s = "overthere",    l = "The Overthere" },
  { s = "permafrost",   l = "The Permafrost Caverns" },
  { s = "fearplane",    l = "The Plane of Fear" },
  { s = "rathemtn",     l = "The Rathe Mountains" },
  { s = "gukbottom",    l = "The Ruins of Old Guk" },
  { s = "sebilis",      l = "The Ruins of Sebilis" },
  { s = "stonebrunt",   l = "The Stonebrunt Mountains" },
  { s = "droga",        l = "The Temple of Droga" },
  { s = "frozenshadow", l = "The Tower of Frozen Shadow" },
  { s = "warrens",      l = "The Warrens" },
  { s = "warslikswood", l = "The Warsliks Woods" },
  { s = "tox",          l = "Toxxulia Forest" },
  { s = "velketor",     l = "Velketor's Labyrinth" },
  { s = "commons",      l = "West Commonlands" },
}

-- monotonic day number; changes at local midnight (rolls the set each calendar day)
local function day_seed()
  local t = os.date("*t")
  return t.year * 366 + t.yday
end

-- today's hot set: a fresh block of HOT_PER_DAY zones that slides a full block each day (so the whole
-- set turns over daily, cycling through the pool every ~ceil(#POOL/HOT_PER_DAY) days).
function M.today()
  local n = #M.POOL
  if n == 0 then return {} end
  local per = M.HOT_PER_DAY
  if per > n then per = n end
  local offset = (day_seed() * per) % n
  local out = {}
  for i = 0, per - 1 do
    out[#out + 1] = M.POOL[(offset + i) % n + 1]
  end
  return out
end

-- is the given zone short-name one of today's hot zones?
function M.is_hot(short)
  if not short or short == "" then return false end
  short = string.lower(short)
  for _, z in ipairs(M.today()) do
    if z.s == short then return true end
  end
  return false
end

return M
