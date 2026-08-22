/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "common/data_bucket.h"
#include "common/data_verification.h"
#include "common/events/player_event_logs.h"
#include "common/features.h"
#include "common/rulesys.h"
#include "common/strings.h"
#include "zone/bot.h"
#include "zone/client.h"
#include "zone/achievement_manager.h"
#include "zone/groups.h"
#include "zone/lua_parser.h"
#include "zone/mob.h"
#include "zone/queryserv.h"
#include "zone/quest_parser_collection.h"
#include "zone/raids.h"
#include "zone/string_ids.h"
#include "zone/worldserver.h"

extern WorldServer worldserver;

extern QueryServ* QServ;

static uint64 ScaleAAXPBasedOnCurrentAATotal(int earnedAA, uint64 add_aaxp)
{
	float baseModifier = RuleR(AA, ModernAAScalingStartPercent);
	int aaMinimum = RuleI(AA, ModernAAScalingAAMinimum);
	int aaLimit = RuleI(AA, ModernAAScalingAALimit);

	// Are we within the scaling window?
	if (earnedAA >= aaLimit || earnedAA < aaMinimum)
	{
		LogDebug("Not within AA scaling window");

		// At or past the limit.  We're done.
		return add_aaxp;
	}

	// We're not at the limit yet.  How close are we?
	int remainingAA = aaLimit - earnedAA;

	// We might not always be X - 0
	int scaleRange = aaLimit - aaMinimum;

	// Normalize and get the effectiveness based on the range and the character's
	// current spent AA.
	float normalizedScale = (float)remainingAA / scaleRange;

	// Scale.
	uint64 totalWithExpMod = add_aaxp * (baseModifier / 100) * normalizedScale;

	// Are we so close to the scale limit that we're earning more XP without scaling?  This
	// will happen when we get very close to the limit.  In this case, just grant the unscaled
	// amount.
	if (totalWithExpMod < add_aaxp)
	{
		return add_aaxp;
	}

	Log(Logs::Detail,
		Logs::None,
		"Total before the modifier %d :: NewTotal %d :: ScaleRange: %d, SpentAA: %d, RemainingAA: %d, normalizedScale: %0.3f",
		add_aaxp, totalWithExpMod, scaleRange, earnedAA, remainingAA, normalizedScale);

	return totalWithExpMod;
}

static uint32 MaxBankedGroupLeadershipPoints(int Level)
{
	if(Level < 35)
		return 4;

	if(Level < 51)
		return 6;

	return 8;
}

static uint32 MaxBankedRaidLeadershipPoints(int Level)
{
	if(Level < 45)
		return 6;

	if(Level < 55)
		return 8;

	return 10;
}

uint64 Client::CalcEXP(uint8 consider_level, bool ignore_modifiers) {
	uint64 in_add_exp = EXP_FORMULA;
	// AoTv4 Carolus: Scale xp gain by AAs here
	int aas = GetSpentAA() + GetAAPoints();
	int aa_xp_penalty = 100; // percent
	int aa_xp_penalty_cap = 10;

	aa_xp_penalty = (aa_xp_penalty + aas) / (aa_xp_penalty + aas * aa_xp_penalty_cap);

	in_add_exp = in_add_exp * aa_xp_penalty / 100;


	if (XPRate != 0) {
		in_add_exp = static_cast<uint64>(in_add_exp * (static_cast<float>(XPRate) / 100.0f));
	}

	if (!ignore_modifiers) {
		auto total_modifier = 1.0f;
		auto zone_modifier  = 1.0f;

		if (RuleR(Character, ExpMultiplier) >= 0) {
			total_modifier *= RuleR(Character, ExpMultiplier);
		}

		if (zone->newzone_data.zone_exp_multiplier >= 0) {
			zone_modifier *= zone->newzone_data.zone_exp_multiplier;
		}

		if (RuleB(Character, UseRaceClassExpBonuses)) {
			if (
				GetClass() == Class::Warrior ||
				GetClass() == Class::Rogue ||
				GetBaseRace() == Race::Halfling
			) {
				total_modifier *= 1.05;
			}
		}

		if (zone->IsHotzone()) {
			total_modifier += RuleR(Zone, HotZoneBonus);
		}

		in_add_exp = uint64(float(in_add_exp) * total_modifier * zone_modifier);
	}

	if (RuleB(Character,UseXPConScaling)) {
		if (consider_level != 0xFF) {
			switch (consider_level) {
				case ConsiderColor::Gray:
					in_add_exp = 0;
					return 0;
				case ConsiderColor::Green:
					in_add_exp = in_add_exp * RuleI(Character, GreenModifier) / 100;
					break;
				case ConsiderColor::LightBlue:
					in_add_exp = in_add_exp * RuleI(Character, LightBlueModifier) / 100;
					break;
				case ConsiderColor::DarkBlue:
					in_add_exp = in_add_exp * RuleI(Character, BlueModifier) / 100;
					break;
				case ConsiderColor::White:
					in_add_exp = in_add_exp * RuleI(Character, WhiteModifier) / 100;
					break;
				case ConsiderColor::Yellow:
					in_add_exp = in_add_exp * RuleI(Character, YellowModifier) / 100;
					break;
				case ConsiderColor::Red:
					in_add_exp = in_add_exp * RuleI(Character, RedModifier) / 100;
					break;
			}
		}
	}

	if (!ignore_modifiers) {
		if (RuleB(Zone, LevelBasedEXPMods)) {
			if (zone->level_exp_mod[GetLevel()].ExpMod) {
				in_add_exp *= zone->level_exp_mod[GetLevel()].ExpMod;
			}
		}

		if (RuleR(Character, FinalExpMultiplier) >= 0) {
			in_add_exp *= RuleR(Character, FinalExpMultiplier);
		}

		if (RuleB(Character, EnableCharacterEXPMods)) {
			in_add_exp *= zone->GetEXPModifier(this);
		}
	}

	return in_add_exp;
}

uint64 Client::GetExperienceForKill(Mob *against)
{
#ifdef LUA_EQEMU
	uint64 lua_ret = 0;
	bool ignoreDefault = false;
	lua_ret = LuaParser::Instance()->GetExperienceForKill(this, against, ignoreDefault);

	if (ignoreDefault) {
		return lua_ret;
	}
#endif

	if (against && against->IsNPC()) {
		uint32 level = (uint32)against->GetLevel();
		uint64 ret = EXP_FORMULA;

		auto mod = against->GetKillExpMod();
		if(mod >= 0) {
			ret *= mod;
			ret /= 100;
		}



		return ret;
	}

	return 0;
}

float static GetConLevelModifierPercent(uint8 conlevel)
{
	switch (conlevel)
	{
	case ConsiderColor::Green:
		return (float)RuleI(Character, GreenModifier) / 100;
		break;
	case ConsiderColor::LightBlue:
		return (float)RuleI(Character, LightBlueModifier) / 100;
		break;
	case ConsiderColor::DarkBlue:
		return (float)RuleI(Character, BlueModifier) / 100;
		break;
	case ConsiderColor::White:
		return (float)RuleI(Character, WhiteModifier) / 100;
		break;
	case ConsiderColor::Yellow:
		return (float)RuleI(Character, YellowModifier) / 100;
		break;
	case ConsiderColor::Red:
		return (float)RuleI(Character, RedModifier) / 100;
		break;
	default:
		return 0;
	}
}

void Client::CalculateNormalizedAAExp(uint64 &add_aaxp, uint8 conlevel, bool resexp)
{
	// Functionally this is the same as having the case in the switch, but this is
	// cleaner to read.
	if (ConsiderColor::Gray == conlevel || resexp)
	{
		add_aaxp = 0;
		return;
	}

	// For this, we ignore the provided value of add_aaxp because it doesn't
	// apply.  XP per AA is normalized such that there are X white con kills
	// per AA.

	uint32 whiteConKillsPerAA = RuleI(AA, NormalizedAANumberOfWhiteConPerAA);
	uint32 xpPerAA = RuleI(AA, ExpPerPoint);

	float colorModifier = GetConLevelModifierPercent(conlevel);
	float percentToAAXp = (float)m_epp.perAA / 100;

	// Normalize the amount of AA XP we earned for this kill.
	add_aaxp = percentToAAXp * (xpPerAA / (whiteConKillsPerAA / colorModifier));
}

void Client::CalculateStandardAAExp(uint64 &add_aaxp, uint8 conlevel, bool resexp)
{
	if (!resexp)
	{
		//if XP scaling is based on the con of a monster, do that now.
		if (RuleB(Character, UseXPConScaling))
		{
			if (conlevel != 0xFF && !resexp)
			{
				add_aaxp *= GetConLevelModifierPercent(conlevel);
			}
		}
	}	//end !resexp

	float aatotalmod = 1.0;
	if (zone->newzone_data.zone_exp_multiplier >= 0) {
		aatotalmod *= zone->newzone_data.zone_exp_multiplier;
	}

	// Shouldn't race not affect AA XP?
	if (RuleB(Character, UseRaceClassExpBonuses))
	{
		if (GetBaseRace() == Race::Halfling) {
			aatotalmod *= 1.05;
		}

		if (GetClass() == Class::Rogue || GetClass() == Class::Warrior) {
			aatotalmod *= 1.05;
		}
	}

	// why wasn't this here? Where should it be?
	if (zone->IsHotzone())
	{
		aatotalmod += RuleR(Zone, HotZoneBonus);
	}

	if (RuleB(Zone, LevelBasedEXPMods)) {
		if (zone->level_exp_mod[GetLevel()].ExpMod) {
			add_aaxp *= zone->level_exp_mod[GetLevel()].AAExpMod;
		}
	}

	if (RuleR(Character, FinalExpMultiplier) >= 0) {
		add_aaxp *= RuleR(Character, FinalExpMultiplier);
	}

	if (RuleB(Character, EnableCharacterEXPMods)) {
		add_aaxp *= zone->GetAAEXPModifier(this);
	}

	add_aaxp = (uint64)(RuleR(Character, AAExpMultiplier) * add_aaxp * aatotalmod);
}

void Client::CalculateLeadershipExp(uint64 &add_exp, uint8 conlevel)
{
	if (IsLeadershipEXPOn() && (conlevel == ConsiderColor::DarkBlue || conlevel == ConsiderColor::White || conlevel == ConsiderColor::Yellow || conlevel == ConsiderColor::Red))
	{
		add_exp = static_cast<uint64>(static_cast<float>(add_exp) * 0.8f);

		if (GetGroup())
		{
			if (m_pp.group_leadership_points < MaxBankedGroupLeadershipPoints(GetLevel())
				&& RuleI(Character, KillsPerGroupLeadershipAA) > 0)
			{
				uint64 exp = GROUP_EXP_PER_POINT / RuleI(Character, KillsPerGroupLeadershipAA);
				Client *mentoree = GetGroup()->GetMentoree();
				if (GetGroup()->GetMentorPercent() && mentoree &&
					mentoree->GetGroupPoints() < MaxBankedGroupLeadershipPoints(mentoree->GetLevel()))
				{
					uint64 mentor_exp = exp * (GetGroup()->GetMentorPercent() / 100.0f);
					exp -= mentor_exp;
					mentoree->AddLeadershipEXP(mentor_exp, 0); // ends up rounded down
					mentoree->MessageString(Chat::LeaderShip, GAIN_GROUP_LEADERSHIP_EXP);
				}
				if (exp > 0)
				{
					// possible if you mentor 100% to the other client
					AddLeadershipEXP(exp, 0); // ends up rounded up if mentored, no idea how live actually does it
					MessageString(Chat::LeaderShip, GAIN_GROUP_LEADERSHIP_EXP);
				}
			}
			else
			{
				MessageString(Chat::LeaderShip, MAX_GROUP_LEADERSHIP_POINTS);
			}
		}
		else
		{
			Raid *raid = GetRaid();
			// Raid leaders CAN NOT gain group AA XP, other group leaders can though!
			if (raid->IsLeader(this))
			{
				if (m_pp.raid_leadership_points < MaxBankedRaidLeadershipPoints(GetLevel())
					&& RuleI(Character, KillsPerRaidLeadershipAA) > 0)
				{
					AddLeadershipEXP(0, RAID_EXP_PER_POINT / RuleI(Character, KillsPerRaidLeadershipAA));
					MessageString(Chat::LeaderShip, GAIN_RAID_LEADERSHIP_EXP);
				}
				else
				{
					MessageString(Chat::LeaderShip, MAX_RAID_LEADERSHIP_POINTS);
				}
			}
			else
			{
				if (m_pp.group_leadership_points < MaxBankedGroupLeadershipPoints(GetLevel())
					&& RuleI(Character, KillsPerGroupLeadershipAA) > 0)
				{
					uint32 group_id = raid->GetGroup(this);
					uint64 exp = GROUP_EXP_PER_POINT / RuleI(Character, KillsPerGroupLeadershipAA);
					Client *mentoree = raid->GetMentoree(group_id);
					if (raid->GetMentorPercent(group_id) && mentoree &&
						mentoree->GetGroupPoints() < MaxBankedGroupLeadershipPoints(mentoree->GetLevel()))
					{
						uint64 mentor_exp = exp * (raid->GetMentorPercent(group_id) / 100.0f);
						exp -= mentor_exp;
						mentoree->AddLeadershipEXP(mentor_exp, 0);
						mentoree->MessageString(Chat::LeaderShip, GAIN_GROUP_LEADERSHIP_EXP);
					}
					if (exp > 0)
					{
						AddLeadershipEXP(exp, 0);
						MessageString(Chat::LeaderShip, GAIN_GROUP_LEADERSHIP_EXP);
					}
				}
				else
				{
					MessageString(Chat::LeaderShip, MAX_GROUP_LEADERSHIP_POINTS);
				}
			}
		}
	}
}

void Client::CalculateExp(uint64 in_add_exp, uint64 &add_exp, uint64 &add_aaxp, uint8 conlevel, bool resexp)
{
	add_exp = in_add_exp;

	if (!resexp && (XPRate != 0))
	{
		add_exp = static_cast<uint64>(in_add_exp * (static_cast<float>(XPRate) / 100.0f));
	}

	// Make sure it was initialized.
	add_aaxp = 0;

	if (!resexp)
	{
		//figure out how much of this goes to AAs
		add_aaxp = add_exp * m_epp.perAA / 100;

		//take that amount away from regular exp
		add_exp -= add_aaxp;

		float totalmod = 1.0;
		float zemmod = 1.0;

		//get modifiers
		if (RuleR(Character, ExpMultiplier) >= 0) {
			totalmod *= RuleR(Character, ExpMultiplier);
		}

		//add the zone exp modifier.
		if (zone->newzone_data.zone_exp_multiplier >= 0) {
			zemmod *= zone->newzone_data.zone_exp_multiplier;
		}

		if (RuleB(Character, UseRaceClassExpBonuses))
		{
			if (GetBaseRace() == Race::Halfling) {
				totalmod *= 1.05;
			}

			if (GetClass() == Class::Rogue || GetClass() == Class::Warrior) {
				totalmod *= 1.05;
			}
		}

		//add hotzone modifier if one has been set.
		if (zone->IsHotzone())
		{
			totalmod += RuleR(Zone, HotZoneBonus);
		}

		add_exp = uint64(float(add_exp) * totalmod * zemmod);

		//if XP scaling is based on the con of a monster, do that now.
		if (RuleB(Character, UseXPConScaling))
		{
			if (conlevel != 0xFF && !resexp)
			{
				add_exp = add_exp * GetConLevelModifierPercent(conlevel);
			}
		}

		// Calculate any changes to leadership experience.
		CalculateLeadershipExp(add_exp, conlevel);
	}	//end !resexp

	if (RuleB(Zone, LevelBasedEXPMods)) {
		if (zone->level_exp_mod[GetLevel()].ExpMod) {
			add_exp *= zone->level_exp_mod[GetLevel()].ExpMod;
		}
	}

	if (RuleR(Character, FinalExpMultiplier) >= 0) {
		add_exp *= RuleR(Character, FinalExpMultiplier);
	}

	if (RuleB(Character, EnableCharacterEXPMods)) {
		add_exp *= zone->GetEXPModifier(this);
	}

	//Enforce Percent XP Cap per kill, if rule is enabled
	int kill_percent_xp_cap = RuleI(Character, ExperiencePercentCapPerKill);
	if (kill_percent_xp_cap >= 0) {
		auto experience_for_level = (GetEXPForLevel(GetLevel() + 1) - GetEXPForLevel(GetLevel()));
		float exp_percent = (float)((float)add_exp / (float)(GetEXPForLevel(GetLevel() + 1) - GetEXPForLevel(GetLevel()))) * (float)100; //EXP needed for level
		if (exp_percent > kill_percent_xp_cap) {
			add_exp = GetEXP() + static_cast<uint64>(std::floor(experience_for_level * (kill_percent_xp_cap / 100.0f)));
			return;
		}
	}

	add_exp = GetEXP() + add_exp;
}

void Client::AddEXP(ExpSource exp_source, uint64 in_add_exp, uint8 conlevel, bool resexp, NPC* npc) {
	if (!IsEXPEnabled()) {
		return;
	}

	uint64 exp = 0;
	uint64 aaexp = 0;

	// Carolus: AoT disable AA standard exp gain
	m_epp.perAA = 0;

	if (m_epp.perAA < 0 || m_epp.perAA > 100) {
		m_epp.perAA = 0;    // stop exploit with sanity check
	}

	// Calculate regular XP
	CalculateExp(in_add_exp, exp, aaexp, conlevel, resexp);
	// Calculate regular AA XP
	if (!RuleB(AA, NormalizedAAEnabled))
	{
		CalculateStandardAAExp(aaexp, conlevel, resexp);
	}
	else
	{
		CalculateNormalizedAAExp(aaexp, conlevel, resexp);
	}

	// Are we also doing linear AA acceleration?
	if (RuleB(AA, ModernAAScalingEnabled) && aaexp > 0)
	{
		aaexp = ScaleAAXPBasedOnCurrentAATotal(GetSpentAA() + GetAAPoints(), aaexp);
	}

	// AoTv4: the more AA a character has earned, the slower NORMAL experience comes in.
	//
	// ⚠️⚠️ THIS IS DELIBERATELY APPLIED TO `exp` ONLY, NEVER TO `aaexp`. AA power is what makes each
	// re-climb to the cap faster (characters keep their AA through the death reset, section 24), so
	// this is the brake on that. Applying it to AA experience as well would make the system throttle
	// ITSELF -- more AA would mean slower AA -- and the AA pool would become progressively harder to
	// finish rather than the levelling being progressively slower. Those are opposite designs.
	//
	// ⚠️⚠️ IT READS TOTAL AA, NOT SPENT AA. `GetSpentAA() + GetAAPoints()` is the same idiom
	// ScaleAAXPBasedOnCurrentAATotal uses just above. Reading only SPENT points would hand players an
	// obvious exploit: bank the points unspent, level at full speed, then spend them at the cap.
	//
	// Multiplier = (Base + aa) / (Base + Factor * aa), asymptotic to 1/Factor. At the defaults
	// (100 / 10): 0 AA = full rate, 12.5 AA = half, 35 AA = 30 percent, 100 AA = 18 percent, and it
	// never falls below 10 percent however much AA is earned. Most of the braking therefore happens
	// over the first ~50 AA and the curve is close to flat after that -- that is the intended shape,
	// not an oversight.
	if (RuleB(AoT, AAExpSlowdownEnabled) && exp > 0) {
		const uint32 total_aa = GetSpentAA() + GetAAPoints();
		if (total_aa > 0) {
			const double base   = static_cast<double>(RuleI(AoT, AAExpSlowdownBase));
			const double factor = static_cast<double>(RuleI(AoT, AAExpSlowdownFactor));
			if (base > 0.0 && factor > 0.0) {
				const double mult = (base + total_aa) / (base + factor * total_aa);
				// ⚠️⚠️ `exp` here is the NEW TOTAL: CalculateExp already ran `add_exp = GetEXP() + gain`
				// (exp.cpp:498), so it is current-exp + this-kill's-gain, NOT the gain alone. Multiplying
				// the whole total by mult<1 shrank it BELOW the current exp for any character with AA ->
				// "You have lost experience" on every kill and the level could never fill (Shadorn, 1 AA,
				// stuck at 1974/2000). Slow ONLY the gain portion; never touch the exp already banked.
				const uint64 had_exp = GetEXP();
				if (exp > had_exp) {
					const uint64 gained = exp - had_exp;
					exp = had_exp + static_cast<uint64>(gained * mult);
				}
			}
		}
	}

	// Check for AA XP Cap
	if (RuleI(AA, MaxAAEXPPerKill) >= 0 && aaexp > RuleI(AA, MaxAAEXPPerKill)) {
		aaexp = RuleI(AA, MaxAAEXPPerKill);
	}

	// Get current AA XP total
	uint32 had_aaexp = GetAAXP();

	// Add it to the XP we just earned.
	aaexp += had_aaexp;

	// Make sure our new total (existing + just earned) isn't lower than the
	// existing total.  If it is, we overflowed the bounds of uint32 and wrapped.
	// Reset to the existing total.
	if (aaexp < had_aaexp)
	{
		aaexp = had_aaexp;	//watch for wrap
	}

	// Check for Unused AA Cap.  If at or above cap, set AAs to cap, set aaexp to 0 and set aa percentage to 0.
	// Doing this here means potentially one kill wasted worth of experience, but easiest to put it here than to rewrite this function.
	int aa_cap = RuleI(AA, UnusedAAPointCap);

	if (aa_cap >= 0 && aaexp > 0) {
		if (m_pp.aapoints == aa_cap) {
			MessageString(Chat::Red, AA_CAP);
			aaexp = 0;
			m_epp.perAA = 0;
		} else if (m_pp.aapoints > aa_cap) {
			MessageString(Chat::Red, OVER_AA_CAP, fmt::format_int(aa_cap).c_str(), fmt::format_int(aa_cap).c_str());
			m_pp.aapoints = aa_cap;
			aaexp = 0;
			m_epp.perAA = 0;
		}
	}

	// AA Sanity Checking for players who set aa exp and deleveled below allowed aa level.
	// ⚠️⚠️ AoTv4: the level was HARDCODED to 50 here and to 51 in SetEXP below, which on a server
	// whose cap is under 51 makes AA experience completely unearnable -- silently, because the AA
	// window still works and the percentage slider still moves. Both are now RuleI(AoT, AAExpMinLevel).
	if (GetLevel() < RuleI(AoT, AAExpMinLevel) && m_epp.perAA > 0) {
		Message(Chat::Yellow, "You are below the level allowed to gain AA Experience. AA Experience set to 0%");
		aaexp = 0;
		m_epp.perAA = 0;
	}

	// Now update our character's normal and AA xp
	SetEXP(exp_source, exp, aaexp, resexp, npc);
}

void Client::SetEXP(ExpSource exp_source, uint64 set_exp, uint64 set_aaxp, bool isrezzexp, NPC* npc) {
	uint64 current_exp = GetEXP();
	uint64 current_aa_exp = GetAAXP();
	uint64 total_current_exp = current_exp + current_aa_exp;
	uint64 total_add_exp = set_exp + set_aaxp;

#ifdef LUA_EQEMU
	uint64 lua_ret = 0;
	bool ignore_default = false;
	lua_ret = LuaParser::Instance()->SetEXP(this, exp_source, current_exp, set_exp, isrezzexp, ignore_default);
	if (ignore_default) {
		set_exp = lua_ret;
	}

	lua_ret = 0;
	ignore_default = false;
	lua_ret = LuaParser::Instance()->SetAAEXP(this, exp_source, current_aa_exp, set_aaxp, isrezzexp, ignore_default);
	if (ignore_default) {
		set_aaxp = lua_ret;
	}
	total_add_exp = set_exp + set_aaxp;
#endif

	LogDebug("Attempting to Set Exp for [{}] (XP: [{}], AAXP: [{}], Rez: [{}])", GetCleanName(), set_exp, set_aaxp, isrezzexp ? "true" : "false");

	auto max_AAXP = GetRequiredAAExperience();
	if (max_AAXP == 0 || GetEXPForLevel(GetLevel()) == 0xFFFFFFFF) {
		Message(Chat::Red, "Error in Client::SetEXP. EXP not set.");
		return; // Must be invalid class/race
	}

	uint32 i = 0;
	uint32 membercount = 0;
	if(GetGroup()) {
		for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
			if (GetGroup()->members[i] != nullptr) {
				membercount++;
			}
		}
	}

	if (total_add_exp > total_current_exp) {
		uint64 exp_gained = set_exp - current_exp;
		uint64 aa_exp_gained = set_aaxp - current_aa_exp;
		float exp_percent = (float)((float)exp_gained / (float)(GetEXPForLevel(GetLevel() + 1) - GetEXPForLevel(GetLevel())))*(float)100; //EXP needed for level
		float aa_exp_percent = (float)((float)aa_exp_gained / (float)(RuleI(AA, ExpPerPoint)))*(float)100; //AAEXP needed for level
		std::string exp_amount_message = "";
		if (RuleI(Character, ShowExpValues) >= 1) {
			if (exp_gained > 0 && aa_exp_gained > 0) {
				exp_amount_message = fmt::format("({}) ({})", exp_gained, aa_exp_gained);
			} else if (exp_gained > 0) {
				exp_amount_message = fmt::format("({})", exp_gained);
			} else {
				exp_amount_message = fmt::format("({}) AA", aa_exp_gained);
			}
		}

		std::string exp_percent_message = "";
		if (RuleI(Character, ShowExpValues) >= 2) {
			if (exp_gained > 0 && aa_exp_gained > 0) {
				exp_percent_message = StringFormat("(%.3f%%, %.3f%%AA)", exp_percent, aa_exp_percent);
			} else if (exp_gained > 0) {
				exp_percent_message = StringFormat("(%.3f%%)", exp_percent);
			} else {
				exp_percent_message = StringFormat("(%.3f%%AA)", aa_exp_percent);
			}
		}
		if (isrezzexp) {
			if (RuleI(Character, ShowExpValues) > 0) {
				Message(Chat::Experience, "You regain %s experience from resurrection. %s", exp_amount_message.c_str(), exp_percent_message.c_str());
			} else {
				MessageString(Chat::Experience, REZ_REGAIN);
			}
		} else {
			if (membercount > 1) {
				if (RuleI(Character, ShowExpValues) > 0) {
					Message(Chat::Experience, "You have gained %s party experience! %s", exp_amount_message.c_str(), exp_percent_message.c_str());
				} else if (zone->IsHotzone()) {
					Message(Chat::Experience, "You gain party experience (with a bonus)!");
				} else {
					MessageString(Chat::Experience, GAIN_GROUPXP);
				}
			} else if (IsRaidGrouped()) {
				if (RuleI(Character, ShowExpValues) > 0) {
					Message(Chat::Experience, "You have gained %s raid experience! %s", exp_amount_message.c_str(), exp_percent_message.c_str());
				} else if (zone->IsHotzone()) {
					Message(Chat::Experience, "You gained raid experience (with a bonus)!");
				} else {
					MessageString(Chat::Experience, GAIN_RAIDEXP);
				}
			} else {
				if (RuleI(Character, ShowExpValues) > 0) {
					Message(Chat::Experience, "You have gained %s experience! %s", exp_amount_message.c_str(), exp_percent_message.c_str());
				} else if (zone->IsHotzone()) {
					Message(Chat::Experience, "You gain experience (with a bonus)!");
				} else {
					MessageString(Chat::Experience, GAIN_XP);
				}
			}
		}
		ProcessEvolvingItem(exp_gained, npc);
	} else if(total_add_exp < total_current_exp) { //only loss message if you lose exp, no message if you gained/lost nothing.
		uint64 exp_lost = current_exp - set_exp;
		float exp_percent = (float)((float)exp_lost / (float)(GetEXPForLevel(GetLevel() + 1) - GetEXPForLevel(GetLevel())))*(float)100;

		if (RuleI(Character, ShowExpValues) == 1 && exp_lost > 0) {
			Message(Chat::Yellow, "You have lost %i experience.", exp_lost);
		} else if (RuleI(Character, ShowExpValues) == 2 && exp_lost > 0) {
			Message(Chat::Yellow, "You have lost %i experience. (%.3f%%)", exp_lost, exp_percent);
		} else {
			Message(Chat::Yellow, "You have lost experience.");
		}
	}

	//check_level represents the level we should be when we have
	//this ammount of exp (once these loops complete)
	uint16 check_level = GetLevel()+1;
	//see if we gained any levels
	bool level_increase = true;
	int8 level_count = 0;

	while (set_exp >= GetEXPForLevel(check_level)) {
		check_level++;
		if (check_level > 127) {	//hard level cap
			check_level = 127;
			break;
		}
		level_count++;

		if (GetMercenaryID()) {
			UpdateMercLevel();
		}
	}
	//see if we lost any levels
	while (set_exp < GetEXPForLevel(check_level-1)) {
		check_level--;
		if (check_level < 2) {	//hard level minimum
			check_level = 2;
			break;
		}
		level_increase = false;
		if (GetMercenaryID()) {
			UpdateMercLevel();
		}
	}
	check_level--;

	// AoTv4: AA EXPERIENCE IS EARNED FROM THE EXPERIENCE ACTUALLY APPLIED, AT AoT:LiveAAExpPct.
	//
	// The roguelite already converted a run's experience into AA at exactly this rate, but only as a
	// lump at death, so the AA bar sat dead for the whole run. This pays the same total continuously
	// instead: a full climb to the cap is 464,000 experience = 2.32 points at the stock 1:1 rate.
	//
	// The RATE IS A RULE, AoT:LiveAAExpPct, and it is 130 -- a deliberate 30 percent income increase
	// (2026-08-22), so a full climb yields 3.02 points. Scaling it here is the ONLY correct place: it
	// leaves normal experience, the level curve and the cap untouched, where changing AA:ExpPerPoint
	// instead would silently disagree with the hardcoded AA_EXP_PER_POINT copy in global_player.lua.
	//
	// WARNING: THE CAP CLAMP MUST HAPPEN HERE, ABOVE THE AA AWARD BLOCK, AND THAT IS THE WHOLE REASON
	// IT MOVED UP FROM ITS ORIGINAL POSITION FURTHER DOWN THIS FUNCTION. Deriving AA from the applied
	// delta is what makes AA stop at the level cap automatically: a character parked at the cap has
	// set_exp clamped back to where it already was, so the delta is 0 and no AA is earned. Compute the
	// delta BEFORE the clamp and a capped character farms AA forever -- which is exactly the v50 bug
	// (deaths paying 7 points against an intended 2.32) reintroduced by another route. The check_level
	// clamp stays below, where check_level is finalised.
	//
	// WARNING: NO LEVEL SCALING, DELIBERATELY. 1:1 is ALREADY steeply depth weighted, twice over: a
	// higher level mob grants more experience, and the curve is quadratic in cumulative terms
	// (1000 * L to gain level L). Levels 1-10 are 11.6 percent of a climb's AA and levels 20-30 are
	// 55 percent. A (level/cap) style multiplier was tried on the death payout and reverted for this
	// same reason -- see the note in global_player.lua. Adding one here double counts it a third time.
	//
	// Not paid on resurrection experience: it is a refund of experience already earned once.
	const int aotv4_hard_cap_early = RuleI(AoT, HardLevelCap);
	if (aotv4_hard_cap_early > 0 && GetLevel() <= static_cast<uint8>(aotv4_hard_cap_early)) {
		const uint64 aotv4_cap_exp = GetEXPForLevel(static_cast<uint16>(aotv4_hard_cap_early));
		if (set_exp > aotv4_cap_exp) {
			set_exp = aotv4_cap_exp;
		}
	}

	if (RuleB(AoT, LiveAAExp) && !isrezzexp && set_exp > current_exp) {
		// AoT:LiveAAExpPct scales the 1:1 above (130 = 30 percent more AA per kill, 2026-08-22).
		// Multiply BEFORE dividing, or a small experience delta truncates to zero AA and low level
		// kills silently pay nothing -- the same trap as the Shield Wall split and the endurance cost.
		const int aotv4_aa_pct = RuleI(AoT, LiveAAExpPct);
		set_aaxp += ((set_exp - current_exp) * static_cast<uint64>(aotv4_aa_pct > 0 ? aotv4_aa_pct : 0)) / 100;
	}

	//see if we gained any AAs
	if (set_aaxp >= max_AAXP) {
		/*
			Note: AA exp is stored differently than normal exp.
			Exp points are only stored in m_pp.expAA until you
			gain a full AA point, once you gain it, a point is
			added to m_pp.aapoints and the ammount needed to gain
			that point is subtracted from m_pp.expAA

			then, once they spend an AA point, it is subtracted from
			m_pp.aapoints. In theory it then goes into m_pp.aapoints_spent,
			but im not sure if we have that in the right spot.
		*/
		//record how many points we have
		uint32 last_unspentAA = m_pp.aapoints;

		//figure out how many AA points we get from the exp were setting
		m_pp.aapoints = set_aaxp / max_AAXP;
		LogDebug("Calculating additional AA Points from AAXP for [{}]: [{}] / [{}] = [{}] points", GetCleanName(), set_aaxp, max_AAXP, (float)set_aaxp / (float)max_AAXP);

		//get remainder exp points, set in PP below
		set_aaxp = set_aaxp - (max_AAXP * m_pp.aapoints);

		//add in how many points we had
		m_pp.aapoints += last_unspentAA;

		//figure out how many points were actually gained
		uint32 gained = (m_pp.aapoints - last_unspentAA);

		//Message(Chat::Yellow, "You have gained %d skill points!!", m_pp.aapoints - last_unspentAA);
		char val1[20] = { 0 };
		char val2[20] = { 0 };

		// AoTv4: WHEN THE PICKER OWNS AA, SUPPRESS THE STOCK "you now have N ability points" LINES.
		// They name m_pp.aapoints, which is about to be forced back to 0 -- so they would promise a
		// spendable pool that the native window then correctly shows as empty. The picker prints its
		// own message from Lua, against the private bank, which is the number that is actually real.
		const bool aotv4_divert_aa = RuleB(AoT, AAPointsToPicker);
		if (!aotv4_divert_aa) {
			if (gained == 1 && m_pp.aapoints == 1) {
				MessageString(Chat::Experience, GAIN_SINGLE_AA_SINGLE_AA, ConvertArray(m_pp.aapoints, val1)); //You have gained an ability point!  You now have %1 ability point.
			} else if (gained == 1 && m_pp.aapoints > 1) {
				MessageString(Chat::Experience, GAIN_SINGLE_AA_MULTI_AA, ConvertArray(m_pp.aapoints, val1)); //You have gained an ability point!  You now have %1 ability points.
			} else {
				MessageString(Chat::Experience, GAIN_MULTI_AA_MULTI_AA, ConvertArray(gained, val1), ConvertArray(m_pp.aapoints, val2)); //You have gained %1 ability point(s)!  You now have %2 ability point(s).
			}
		}

		if (RuleB(AA, SoundForAAEarned)) {
			SendSound();
		}

		// AoTv4: when diverting, AoTv4DivertAAPoints fires this event itself -- AFTER it has credited
		// the bank, so the handler sees the real total. Firing here as well would offer twice and pop
		// the window twice for one point.
		if (!aotv4_divert_aa && parse->PlayerHasQuestSub(EVENT_AA_GAIN)) {
			parse->EventPlayer(EVENT_AA_GAIN, this, std::to_string(gained), 0);
		}

		RecordPlayerEventLog(PlayerEvent::AA_GAIN, PlayerEvent::AAGainedEvent{gained});

		/* QS: PlayerLogAARate */
		if (RuleB(QueryServ, PlayerLogAARate)) {
			int add_points = (m_pp.aapoints - last_unspentAA);
			std::string query = StringFormat("INSERT INTO `qs_player_aa_rate_hourly` (char_id, aa_count, hour_time) VALUES (%i, %i, UNIX_TIMESTAMP() - MOD(UNIX_TIMESTAMP(), 3600)) ON DUPLICATE KEY UPDATE `aa_count` = `aa_count` + %i", CharacterID(), add_points, add_points);
			QServ->SendQuery(query.c_str());
		}

		//Message(Chat::Yellow, "You now have %d skill points available to spend.", m_pp.aapoints);

		// AoTv4: HAND THE POINTS TO THE PICKER AND LEAVE THE NATIVE POOL EMPTY.
		//
		// The design is that AA is spent through the random picker, never bought directly in the
		// native AA window -- so m_pp.aapoints must end at 0. The points themselves are not lost: the
		// EVENT_AA_GAIN fired just above carries `gained` to global_player.lua, which banks it in the
		// private aa_bank_<charid> bucket that the picker spends from.
		//
		// WARNING: THIS MUST STAY BELOW BOTH THE EVENT AND THE QUERYSERV BLOCK. Both derive the number
		// gained from m_pp.aapoints (the event via `gained`, QS via aapoints - last_unspentAA), so
		// zeroing any earlier hands Lua a 0 and silently drops the point on the floor -- the player
		// earns nothing and nothing reports it.
		//
		// The remainder in set_aaxp is deliberately NOT cleared: it is what keeps the AA bar sitting at
		// the correct partial fill toward the next point, and it carries across the roguelite death the
		// same way the old aa_xp_<charid> remainder did.
		//
		// WARNING: THIS GOES THROUGH AoTv4DivertAAPoints AND MUST NOT GO BACK TO A BARE
		// `m_pp.aapoints = 0`. That is what it used to be, and it destroyed points whenever the quest
		// hook did not run -- the bank was credited by Lua while the zeroing happened here regardless,
		// so the two could disagree and the player silently lost the point. The helper credits the
		// bank in C++ first, so the grant and the zeroing are now inseparable.
		if (aotv4_divert_aa) {
			AoTv4DivertAAPoints(gained);
		}
	}

	uint8 max_level = RuleI(Character, MaxExpLevel) + 1;

	if (max_level <= 1) {
		max_level = RuleI(Character, MaxLevel) + 1;
	}

	auto client_max_level = GetClientMaxLevel();
	if (client_max_level) {
		max_level = client_max_level + 1;
	}

	if (check_level > max_level) {
		check_level = max_level;

		if (RuleB(Character, KeepLevelOverMax)) {
			set_exp = GetEXPForLevel(GetLevel()+1);
		} else {
			set_exp = GetEXPForLevel(max_level);
		}
	}

	// ⚠️⚠️ AoTv4: THE HARD LEVEL CAP MUST CAP *EXPERIENCE* TOO, NOT JUST THE LEVEL.
	//
	// `AoT:HardLevelCap` clamps the LEVEL in SetLevel, but the ceiling above is computed from
	// `Character:MaxExpLevel` (70), so a character held at 30 kept accruing experience toward the
	// level 71 threshold -- unbounded as far as any player could tell. That matters here far more
	// than it would on a stock server, because the roguelite death banks AA from TOTAL RUN
	// EXPERIENCE at AA:ExpPerPoint (200,000). A full climb to the cap is 464,000 = 2.32 points,
	// which is the intended payout; farming at cap pushed real characters to 7 points and the
	// engine ceiling allowed 12.7. Reported from play as "people are getting more AAs after 30".
	//
	// ⚠️⚠️ THE LUA PIN IN `era_system.clamp_level` CANNOT DO THIS AND NEVER COULD. It is guarded on
	// `client:GetLevel() > cap`, and the C++ clamp in SetLevel means the level NEVER exceeds the cap
	// -- so the guard is unreachable and the pin never fires. The comment on that clamp claiming it
	// "still pins experience at the threshold" is wrong. The Lua pin is still load-bearing for the
	// per-character REGION caps, which sit BELOW this one and therefore can still be crossed.
	//
	// ⚠️⚠️ `GetLevel() <= hard_cap` IS LOAD BEARING -- WITHOUT IT THIS DELEVELS GMs. A GM who has
	// #levelled to 60 would have set_exp pinned to the level 30 value, `check_level` would then
	// resolve to 30, and `SetLevel(check_level)` directly below fires whenever GetLevel() differs --
	// silently dropping them 60 -> 30. Anyone already above the cap is left completely alone.
	//
	// ⚠️ Pinned at GetEXPForLevel(cap), not cap + 1: the experience bar therefore sits at 0 percent
	// on reaching the cap rather than filling. That is the exact 2.32 point ceiling asked for --
	// cap + 1 would be 495,000 = 2.47 and is still over it.
	// ⚠️⚠️ AND `check_level` IS CLAMPED UNCONDITIONALLY, NOT ONLY WHEN THE EXPERIENCE IS TRIMMED --
	// BECAUSE THE LEVEL-UP MESSAGE TWENTY LINES BELOW PRINTS IT RAW.
	//
	//     Message(Chat::Yellow, "Welcome to level %i!", check_level);   // <-- unclamped
	//     ...
	//     SetLevel(check_level);                                        // <-- clamps to the cap INSIDE
	//
	// Client::SetLevel clamps its ARGUMENT (see the note at the top of it), so the level actually
	// applied has always been correct. The message did not know that, so a player crossing several
	// levels in one kill was told "Welcome to level 35!" and then quietly set to 30. Reported from
	// play, repeatedly, as "people are still able to level up past 30" -- they were not; every one of
	// those reports was this line lying, and the cap was doing its job the whole time.
	// 📌 That is also why it kept coming back: nothing was broken to find, so nothing got fixed.
	//
	// ⚠️ The `level_count == 1` branch beside it uses GAIN_LEVEL, which the CLIENT renders from its own
	// level -- so single-level dings never showed this. Only a multi-level gain does, which the linear
	// experience curve made common.
	// ⚠️ `GetLevel() <= hard_cap` still guards the whole block so a GM who has #levelled above the cap
	// is not dragged back down by their next kill.
	// WARNING: THE `set_exp` HALF OF THIS CLAMP NOW LIVES FURTHER UP, ABOVE THE AA AWARD BLOCK, AND
	// MUST NOT BE DUPLICATED BACK IN HERE. AA experience is derived from the applied experience delta,
	// so the clamp has to run BEFORE that delta is measured or a character parked at the cap keeps
	// earning AA forever -- the v50 bug by another route. Only the check_level half belongs here,
	// because check_level is not finalised until the level loops above have run.
	const int aotv4_hard_cap = RuleI(AoT, HardLevelCap);
	if (aotv4_hard_cap > 0 && GetLevel() <= static_cast<uint8>(aotv4_hard_cap)) {
		if (check_level > static_cast<uint16>(aotv4_hard_cap)) {
			check_level = static_cast<uint16>(aotv4_hard_cap);
		}
	}

	if ((GetLevel() != check_level) && !(check_level >= max_level)) {
		char val1[20]={0};
		if (level_increase) {
			if (level_count == 1) {
				MessageString(Chat::Experience, GAIN_LEVEL, ConvertArray(check_level, val1));
			} else {
				Message(Chat::Yellow, "Welcome to level %i!", check_level);
			}

			if (check_level == RuleI(Character, DeathItemLossLevel) &&
			    m_ClientVersionBit & EQ::versions::maskUFAndEarlier) {
				MessageString(Chat::Yellow, CORPSE_ITEM_LOST);
				}

			if (check_level == RuleI(Character, DeathExpLossLevel)) {
				MessageString(Chat::Yellow, CORPSE_EXP_LOST);
			}
		}

		uint8 myoldlevel = GetLevel();

		SetLevel(check_level);

		if (RuleB(Bots, Enabled) && RuleB(Bots, BotLevelsWithOwner)) {
			// hack way of doing this..but, least invasive... (same criteria as gain level for sendlvlapp)
			Bot::LevelBotWithClient(this, GetLevel(), (myoldlevel == check_level - 1));
		}
	}

	//If were at max level then stop gaining experience if we make it to the cap
	if (GetLevel() == max_level - 1){
		uint32 expneeded = GetEXPForLevel(max_level);
		if (set_exp > expneeded) {
			set_exp = expneeded;
		}
	}

	if (parse->PlayerHasQuestSub(EVENT_EXP_GAIN) && m_pp.exp != set_exp) {
		parse->EventPlayer(EVENT_EXP_GAIN, this, std::to_string(set_exp - m_pp.exp), 0);
	}

	if (parse->PlayerHasQuestSub(EVENT_AA_EXP_GAIN) && m_pp.expAA != set_aaxp) {
		parse->EventPlayer(EVENT_AA_EXP_GAIN, this, std::to_string(set_aaxp - m_pp.expAA), 0);
	}

	//set the client's EXP and AAEXP
	m_pp.exp = set_exp;
	m_pp.expAA = set_aaxp;

	// ⚠️ AoTv4: was hardcoded to 51 -- see the note on the matching gate in AddEXP.
	if (GetLevel() < RuleI(AoT, AAExpMinLevel)) {
		m_epp.perAA = 0;	// turn off aa exp below the configured AA level
	} else {
		SendAlternateAdvancementStats();    //otherwise, send them an AA update
	}

	//send the expdata in any case so the xp bar isnt stuck after leveling
	uint32 tmpxp1 = GetEXPForLevel(GetLevel()+1);
	uint32 tmpxp2 = GetEXPForLevel(GetLevel());
	// Quag: crash bug fix... Divide by zero when tmpxp1 and 2 equalled each other, most likely the error case from GetEXPForLevel() (invalid class, etc)
	if (tmpxp1 != tmpxp2 && tmpxp1 != 0xFFFFFFFF && tmpxp2 != 0xFFFFFFFF) {
		auto outapp = new EQApplicationPacket(OP_ExpUpdate, sizeof(ExpUpdate_Struct));
		ExpUpdate_Struct* eu = (ExpUpdate_Struct*)outapp->pBuffer;
		float tmpxp = (float) ( (float) set_exp-tmpxp2 ) / ( (float) tmpxp1-tmpxp2 );
		eu->exp = (uint32)(330.0f * tmpxp);
		FastQueuePacket(&outapp);
	}

	if (admin >= AccountStatus::GMAdmin && GetGM()) {
		char val1[20]={0};
		char val2[20]={0};
		char val3[20]={0};
		MessageString(Chat::Experience, GM_GAINXP, ConvertArray(set_aaxp,val1),ConvertArray(set_exp,val2),ConvertArray(GetEXPForLevel(GetLevel()+1),val3));	//[GM] You have gained %1 AXP and %2 EXP (%3).
	}
}

void Client::SetLevel(uint8 set_level, bool command)
{
	// ================================================================================================
	// ⚠️⚠️ AoTv4 HARD LEVEL CAP -- IT HAS TO BE HERE, NOT IN LUA
	// ================================================================================================
	// era_system.clamp_level runs from event_level_up and CANNOT hold the cap, for two independent
	// reasons -- players reached 31 with the cap set to 30:
	//   1. OFF BY ONE. The event fires further down this function while `m_pp.level` still holds the
	//      OLD level (it is assigned afterwards, `m_pp.level = set_level`), so Lua's GetLevel() is
	//      stale. Going 30 -> 31 the clamp tests `30 > 30` and declines to act.
	//   2. OVERWRITTEN ANYWAY. Even when it does fire, the SetLevel(cap) it performs is undone the
	//      moment this outer call resumes and assigns m_pp.level = set_level.
	// Clamping the ARGUMENT is the only point in the flow that neither problem can reach.
	//
	// ⚠️ `command` is exempt on purpose: that flag marks a GM #level, and a GM must be able to go
	// above the cap to reach and test content. Organic experience gain always arrives with it false.
	// 📌 The Lua clamp is deliberately LEFT IN PLACE. It no longer decides the cap, but it still pins
	// experience at the threshold, which is what stops a capped character re-crossing on every kill.
	if (!command) {
		const int hard_cap = RuleI(AoT, HardLevelCap);
		if (hard_cap > 0 && set_level > static_cast<uint8>(hard_cap)) {
			set_level = static_cast<uint8>(hard_cap);
		}
	}

	if (GetEXPForLevel(set_level) == 0xFFFFFFFF) {
		LogError("GetEXPForLevel([{}]) = 0xFFFFFFFF", set_level);
		return;
	}

	auto outapp = new EQApplicationPacket(OP_LevelUpdate, sizeof(LevelUpdate_Struct));
	auto* lu = (LevelUpdate_Struct *) outapp->pBuffer;
	lu->level = set_level;

	if (m_pp.level2 != 0) {
		lu->level_old = m_pp.level2;
	} else {
		lu->level_old = level;
	}

	level = set_level;

	if (IsRaidGrouped()) {
		Raid *r = GetRaid();
		if (r) {
			r->UpdateLevel(GetName(), set_level);
		}
	}

	if (set_level > m_pp.level2) {
		if (m_pp.level2 == 0) {
			m_pp.points += 5;
		} else {
			m_pp.points += (5 * (set_level - m_pp.level2));
		}

		m_pp.level2 = set_level;
	}

	if (set_level > m_pp.level) {
		int levels_gained = (set_level - m_pp.level);

		if (parse->PlayerHasQuestSub(EVENT_LEVEL_UP)) {
			parse->EventPlayer(EVENT_LEVEL_UP, this, std::to_string(levels_gained), 0);
		}

		if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::LEVEL_GAIN)) {
			auto e = PlayerEvent::LevelGainedEvent{
				.from_level = m_pp.level,
				.to_level = set_level,
				.levels_gained = levels_gained
			};

			RecordPlayerEventLog(PlayerEvent::LEVEL_GAIN, e);
		}
	} else if (set_level < m_pp.level) {
		int levels_lost = (m_pp.level - set_level);

		if (parse->PlayerHasQuestSub(EVENT_LEVEL_DOWN)) {
			parse->EventPlayer(EVENT_LEVEL_DOWN, this, std::to_string(levels_lost), 0);
		}

		if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::LEVEL_LOSS)) {
			auto e = PlayerEvent::LevelLostEvent{
				.from_level = m_pp.level,
				.to_level = set_level,
				.levels_lost = levels_lost
			};

			RecordPlayerEventLog(PlayerEvent::LEVEL_LOSS, e);
		}
	}

	m_pp.level = set_level;

	if (command) {
		m_pp.exp = GetEXPForLevel(set_level);
		Message(Chat::Yellow, fmt::format("Welcome to level {}!", set_level).c_str());
		lu->exp = 0;

		AutoGrantAAPoints();
	} else {
		const auto temporary_xp = (
			static_cast<float>(m_pp.exp - GetEXPForLevel(GetLevel())) /
			static_cast<float>(GetEXPForLevel(GetLevel() + 1) - GetEXPForLevel(GetLevel()))
		);
		lu->exp = static_cast<uint32>(330.0f * temporary_xp);
	}

	QueuePacket(outapp);
	safe_delete(outapp);
	SendAppearancePacket(AppearanceType::WhoLevel, set_level); // who level change

	LogInfo("Setting Level for [{}] to [{}]", GetName(), set_level);

	CalcBonuses();

	if (!RuleB(Character, HealOnLevel)) {
		const auto max_hp = CalcMaxHP();
		if (GetHP() > max_hp) {
			SetHP(max_hp);
		}
	} else {
		SetHP(CalcMaxHP()); // Why not, lets give them a free heal
	}

	if (RuleI(World, PVPMinLevel) > 0 && level >= RuleI(World, PVPMinLevel) && m_pp.pvp == 0) {
		SetPVP(true);
	}

	if (IsInAGuild()) {
		guild_mgr.SendToWorldMemberLevelUpdate(GuildID(), GetLevel(), std::string(GetCleanName()));
		DoGuildTributeUpdate();
	}

	DoTributeUpdate();
	SendHPUpdate();
	SetMana(CalcMaxMana());
	UpdateWho();

	UpdateMercLevel();

	Save();

	achievement_manager.ProcessLevel(this);
}

// Note: The client calculates exp separately, we cant change this function
// Add: You can set the values you want now, client will be always sync :) - Merkur
uint32 Client::GetEXPForLevel(uint16 check_level)
{
	// AoTv4 Carolus: exp formula
	// minor bug fix, there is probably a better way
	check_level--;
	// This is the sumation of x+1
	int base = (1+check_level) * check_level / 2 + check_level;
	int mult = 1000;
	return base * mult;

#ifdef LUA_EQEMU
	uint32 lua_ret = 0;
	bool ignoreDefault = false;
	lua_ret = LuaParser::Instance()->GetEXPForLevel(this, check_level, ignoreDefault);

	if (ignoreDefault) {
		return lua_ret;
	}
#endif



#if 0
	uint16 check_levelm1 = check_level-1;

	// AoTv4: LINEAR level curve, per the balance sheet (exp.xlsx).
	//
	// Cost of the NEXT level is a flat 1000 per level: level 1 -> 2 costs 2,000, level 34 -> 35 costs
	// 35,000. Cumulative cost to REACH level L is therefore the triangular sum
	//     1000 * n * (n + 3) / 2      where n = L - 1
	// which gives 0 at level 1, 2,000 at level 2, and 629,000 at level 35 (665,000 to complete 35).
	//
	// ⚠️⚠️ THIS REPLACES A CUBIC CURVE AND THE DIFFERENCE IS ENORMOUS -- roughly 65x. Stock EQ (and
	// the AoTv4 variant before this) used (level-1)^3 * 1000, which needed about 43 MILLION experience
	// to reach level 35 against 629 THOUSAND here. Anything tuned against the old curve -- quest
	// rewards, the delve's experience payouts, mob experience values -- is now worth ~65x more in
	// levels than it was. That is the intended redesign for a level 35 cap, not an oversight, but it
	// is the first thing to suspect if levelling feels instant.
	//
	// ⚠️ Hell levels cannot occur here by construction: the per-level cost rises by a constant 1000,
	// so there is no multiplier to spike. The previous implementation needed an explicit smooth ramp
	// to avoid the stock stepwise multiplier's spikes at 31/36/41/46/52/56-60; that problem is gone
	// rather than solved.
	// ⚠️ The AA cost per point is NOT derived from this -- it is the separate AA:ExpPerPoint rule
	// (200,000, also from the sheet). Reaching level 35 is worth about 3.3 AA points at that rate, so
	// AA is earned at the cap rather than on the way up.
	float base = 0.5f * float(check_levelm1) * float(check_levelm1 + 3);

	uint32 finalxp = uint32(base * 1000.0f);

	if(RuleB(Character,UseOldRaceExpPenalties))
	{
		float racemod = 1.0;
		if(GetBaseRace() == Race::Troll || GetBaseRace() == Race::Iksar) {
			racemod = 1.2;
		} else if(GetBaseRace() == Race::Ogre) {
			racemod = 1.15;
		} else if(GetBaseRace() == Race::Barbarian) {
			racemod = 1.05;
		} else if(GetBaseRace() == Race::Halfling) {
			racemod = 0.95;
		}

		finalxp = uint64(finalxp * racemod);
	}

	if(RuleB(Character,UseOldClassExpPenalties))
	{
		float classmod = 1.0;
		if(GetClass() == Class::Paladin || GetClass() == Class::ShadowKnight || GetClass() == Class::Ranger || GetClass() == Class::Bard) {
			classmod = 1.4;
		} else if(GetClass() == Class::Monk) {
			classmod = 1.2;
		} else if(GetClass() == Class::Wizard || GetClass() == Class::Enchanter || GetClass() == Class::Magician || GetClass() == Class::Necromancer) {
			classmod = 1.1;
		} else if(GetClass() == Class::Rogue) {
			classmod = 0.91;
		} else if(GetClass() == Class::Warrior) {
			classmod = 0.9;
		}

		finalxp = uint64(finalxp * classmod);
	}

	return finalxp;
#endif
}

void Client::AddLevelBasedExp(ExpSource exp_source, uint8 exp_percentage, uint8 max_level, bool ignore_mods)
{
	uint64	award;
	uint64	xp_for_level;

	if (exp_percentage > 100)
	{
		exp_percentage = 100;
	}

	if (!max_level || GetLevel() < max_level)
	{
		max_level = GetLevel();
	}

	xp_for_level = GetEXPForLevel(max_level + 1) - GetEXPForLevel(max_level);
	award = xp_for_level * exp_percentage / 100;

	if (RuleB(Zone, LevelBasedEXPMods) && !ignore_mods) {
		if (zone->level_exp_mod[GetLevel()].ExpMod) {
			award *= zone->level_exp_mod[GetLevel()].ExpMod;
		}
	}

	if (RuleR(Character, FinalExpMultiplier) >= 0) {
		award *= RuleR(Character, FinalExpMultiplier);
	}

	uint64 newexp = GetEXP() + award;
	SetEXP(exp_source, newexp, GetAAXP());
}

void Group::SplitExp(ExpSource exp_source, const uint64 exp, Mob* other) {
	if (other->CastToNPC()->MerchantType != 0) {
		return;
	}

	if (other->GetOwner() && other->GetOwner()->IsClient()) {
		return;
	}

	const auto member_count = GroupCount();
	if (!member_count) {
		return;
	}

	auto       group_experience = exp;
	const auto highest_level    = GetHighestLevel();

	auto group_modifier = 1.0f;
	if (RuleB(Character, EnableGroupMemberEXPModifier)) {
		if (EQ::ValueWithin(member_count, 2, 5)) {
			group_modifier = 1 + RuleR(Character, GroupMemberEXPModifier) * (member_count - 1); // 2 = 1.2x, 3 = 1.4x, 4 = 1.6x, 5 = 1.8x
		} else if (member_count == 6) {
			group_modifier = RuleR(Character, FullGroupEXPModifier);
		}
	}

	if (EQ::ValueWithin(member_count, 2, 6)) {
		if (RuleB(Character, EnableGroupEXPModifier)) {
			group_experience += static_cast<uint64>(
				static_cast<float>(exp) *
				group_modifier *
				RuleR(Character, GroupExpMultiplier)
			);
		} else {
			group_experience += static_cast<uint64>(
				static_cast<float>(exp) *
				group_modifier
			);
		}
	}

	const uint8 consider_level = Mob::GetLevelCon(highest_level, other->GetLevel());
	if (consider_level == ConsiderColor::Gray) {
		return;
	}

	for (const auto& m : members) {
		if (m && m->IsClient()) {
			const int32 diff     = m->GetLevel() - highest_level;
			int32       max_diff = -(m->GetLevel() * 15 / 10 - m->GetLevel());

			if (max_diff > -5) {
				max_diff = -5;
			}

			if (diff >= max_diff) {
				const uint64 tmp  = (m->GetLevel() + 3) * (m->GetLevel() + 3) * 75 * 35 / 10;
				const uint64 tmp2 = group_experience / member_count;
				m->CastToClient()->AddEXP(exp_source, tmp < tmp2 ? tmp : tmp2, consider_level, false, other->CastToNPC());
			}
		}
	}
}

void Raid::SplitExp(ExpSource exp_source, const uint64 exp, Mob* other) {
	if (other->CastToNPC()->MerchantType != 0) {
		return;
	}

	if (other->GetOwner() && other->GetOwner()->IsClient()) {
		return;
	}

	const auto member_count = RaidCount();
	if (!member_count) {
		return;
	}

	auto       raid_experience = exp;
	const auto highest_level   = GetHighestLevel();

	if (RuleB(Character, EnableRaidEXPModifier)) {
		raid_experience = static_cast<uint64>(static_cast<float>(raid_experience) *	(1.0f - RuleR(Character, RaidExpMultiplier)));
	}

	raid_experience = static_cast<uint64>(static_cast<float>(raid_experience) * RuleR(Character, FinalRaidExpMultiplier));

	const auto consider_level = Mob::GetLevelCon(highest_level, other->GetLevel());
	if (consider_level == ConsiderColor::Gray) {
		return;
	}

	uint32 member_modifier = 1;
	if (RuleB(Character, EnableRaidMemberEXPModifier)) {
		member_modifier = member_count;
	}

	for (const auto& m : members) {
		if (m.member && !m.is_bot) {
			const int32 diff     = m.member->GetLevel() - highest_level;
			int32       max_diff = -(m.member->GetLevel() * 15 / 10 - m.member->GetLevel());

			if (max_diff > -5) {
				max_diff = -5;
			}

			if (diff >= max_diff) {
				const uint64 tmp  = (m.member->GetLevel() + 3) * (m.member->GetLevel() + 3) * 75 * 35 / 10;
				const uint64 tmp2 = (raid_experience / member_modifier) + 1;
				m.member->AddEXP(exp_source, tmp < tmp2 ? tmp : tmp2, consider_level, false, other->CastToNPC());
			}
		}
	}
}

void Client::SetLeadershipEXP(uint64 group_exp, uint64 raid_exp) {
	while(group_exp >= GROUP_EXP_PER_POINT) {
		group_exp -= GROUP_EXP_PER_POINT;
		m_pp.group_leadership_points++;
		MessageString(Chat::LeaderShip, GAIN_GROUP_LEADERSHIP_POINT);
	}
	while(raid_exp >= RAID_EXP_PER_POINT) {
		raid_exp -= RAID_EXP_PER_POINT;
		m_pp.raid_leadership_points++;
		MessageString(Chat::LeaderShip, GAIN_RAID_LEADERSHIP_POINT);
	}

	m_pp.group_leadership_exp = group_exp;
	m_pp.raid_leadership_exp = raid_exp;

	SendLeadershipEXPUpdate();
}

void Client::AddLeadershipEXP(uint64 group_exp, uint64 raid_exp) {
	SetLeadershipEXP(GetGroupEXP() + group_exp, GetRaidEXP() + raid_exp);
}

void Client::SendLeadershipEXPUpdate() {
	auto outapp = new EQApplicationPacket(OP_LeadershipExpUpdate, sizeof(LeadershipExpUpdate_Struct));
	LeadershipExpUpdate_Struct* eu = (LeadershipExpUpdate_Struct *) outapp->pBuffer;

	eu->group_leadership_exp = m_pp.group_leadership_exp;
	eu->group_leadership_points = m_pp.group_leadership_points;
	eu->raid_leadership_exp = m_pp.raid_leadership_exp;
	eu->raid_leadership_points = m_pp.raid_leadership_points;

	FastQueuePacket(&outapp);
}

uint8 Client::GetCharMaxLevelFromQGlobal() {
	auto char_cache = GetQGlobals();

	std::list<QGlobal> global_map;

	const uint32 zone_id = zone ? zone->GetZoneID() : 0;

	if (char_cache) {
		QGlobalCache::Combine(global_map, char_cache->GetBucket(), 0, CharacterID(), zone_id);
	}

	for (const auto& global : global_map) {
		if (global.name == "CharMaxLevel") {
			if (Strings::IsNumber(global.value)) {
				return static_cast<uint8>(Strings::ToUnsignedInt(global.value));
			}
		}
	}

	return 0;
}

uint8 Client::GetCharMaxLevelFromBucket()
{
	DataBucketKey k = GetScopedBucketKeys();
	k.key = "CharMaxLevel";

	auto b = DataBucket::GetData(&database, k);
	if (!b.value.empty()) {
		if (Strings::IsNumber(b.value)) {
			return static_cast<uint8>(Strings::ToUnsignedInt(b.value));
		}
	}

	return 0;
}

uint32 Client::GetRequiredAAExperience() {
#ifdef LUA_EQEMU
	uint32 lua_ret = 0;
	bool ignoreDefault = false;
	lua_ret = LuaParser::Instance()->GetRequiredAAExperience(this, ignoreDefault);

	if (ignoreDefault) {
		return lua_ret;
	}
#endif

	return RuleI(AA, ExpPerPoint);
}
