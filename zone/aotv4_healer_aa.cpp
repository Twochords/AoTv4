/*	EQEMu: Everquest Server Emulator

	AoTv4 Healer AA tree -- the behaviour behind custom/sql/aotv4_aa_healer_hosted.sql.

	All five healer AAs are MARKER AAs: they carry no aa_rank_effects, because every one of them is
	conditional on something no SPA can express -- the target's remaining health, the size of an
	overheal, a cure having succeeded. The AA row exists only to be read, and Mob::GetAA turns a
	rank id into "which rank of this do I own", 0 if none.

	⚠️ THE RANK IDS BELOW ARE THE ONLY JOIN TO THE SQL, AND NOTHING CHECKS THEM. A wrong id reads 0
	forever, which in game looks exactly like an AA you bought that quietly does nothing. If the
	hosts are ever moved, these five numbers and the SQL must change together.

	What is NOT here, on purpose: the buffs these apply are real spells doing real work through
	native SPAs (55 rune, 162 flat mitigation, 232 death save, 0 regen), not inert markers. The code
	decides when to apply them and how big the shield is; the engine does the rest.
*/

#include <algorithm>   // std::max -- the Grace shield refresh takes the better of old and new

#include "../common/spdat.h"
#include "client.h"
#include "entity.h"
#include "groups.h"
#include "mob.h"
#include "raids.h"
#include "zone.h"

extern Zone *zone;

namespace
{
	// ---------------------------------------------------------------- joins to the SQL
	// First rank id of each host ability. See aotv4_aa_healer_hosted.sql.
	constexpr int AA_GRACE   = 37;   // host  8, ranks 37-41
	constexpr int AA_TRIAGE  = 42;   // host  9, ranks 42-46
	constexpr int AA_ECHO    = 47;   // host 10, ranks 47-51
	constexpr int AA_RENEWAL = 52;   // host 11, ranks 52-56
	constexpr int AA_BREATH  = 57;   // host 12, ranks 57-61

	// Buff rows from aotv4_healer_buffs.sql (helper band, never offered as a reward).
	constexpr uint16 SPELL_GRACE_SHIELD  = 43390;
	constexpr uint16 SPELL_BREATH_RANK1  = 43391;   // 43391..43395, one row per rank
	constexpr uint16 SPELL_RENEWAL_REGEN = 43396;
	constexpr uint16 SPELL_GRACE_RENEWED = 43397;

	constexpr int RANKS = 5;

	// ---------------------------------------------------------------- Triage Instinct
	constexpr float TRIAGE_HP_PCT     = 35.0f;
	constexpr int   TRIAGE_HEAL[RANKS] = { 6, 10, 10, 15, 15 };
	constexpr int   TRIAGE_CRIT[RANKS] = { 0,  0, 15, 15, 15 };
	constexpr int   TRIAGE_REFUND_PCT  = 15;   // of the spell's mana cost, rank 5 only

	// ---------------------------------------------------------------- Overflowing Grace
	constexpr int GRACE_PCT[RANKS]        = { 10, 18, 18, 25, 25 };
	constexpr int GRACE_TICKS[RANKS]      = {  3,  3,  5,  5,  5 };
	constexpr int GRACE_CAP_PCT_OF_MAXHP  = 20;   // the shield can never exceed this share of max HP
	// ⚠️⚠️ THIS IS WHAT MAKES A PARKED HEAL SAFE, AND IT IS PER TARGET, NOT PER HEALER. Grace fires on
	// a fully wasted heal (see AoTv4HealerPostHeal), so without a window a healer could hold a heal on
	// a full-health tank and rebuild the shield every cast. Per target rather than per healer because
	// shielding a whole group IS the ability; re-shielding one body on repeat is the exploit.
	// 📌 30s against a shield that lasts 18-30s (GRACE_TICKS) means brief gaps between shields by
	// design -- it should not be permanently up.
	constexpr uint32 GRACE_COOLDOWN_MS    = 30000;

	// ---------------------------------------------------------------- Mender's Echo
	constexpr int   ECHO_PCT[RANKS]  = { 8, 12, 12, 18, 18 };
	constexpr int   ECHO_SECOND_PCT  = 50;    // rank 3+: the second ally gets half
	constexpr float ECHO_RANGE       = 100.0f;
	constexpr int   ECHO_CRIT_PCT    = 25;    // rank 5: chance for the echo itself to crit

	// ---------------------------------------------------------------- Cleansing Renewal
	constexpr int    RENEWAL_PER_LEVEL[RANKS] = { 2, 3, 3, 5, 5 };
	constexpr int    RENEWAL_NEXT_HEAL_PCT    = 20;      // rank 5
	constexpr uint32 RENEWAL_WINDOW_MS        = 10000;   // ...and how long that offer stands

	// ---------------------------------------------------------------- Borrowed Breath
	constexpr float  BREATH_HP_PCT           = 15.0f;
	// The cooldown is spent when a death is actually AVOIDED, not when the save is armed -- arming
	// is free. Otherwise a heal on someone who then survives on their own would burn the whole
	// window for nothing, and "once every two minutes" would not be true of anything observable.
	constexpr uint32 BREATH_SAVE_COOLDOWN_MS = 120000;   // two minutes, per HEALER
	constexpr uint32 BREATH_SAVE_WINDOW_MS   = 20000;    // how long a save stays armed on the TARGET
	constexpr int    BREATH_SAVE_HP_PCT      = 15;       // health they come back at

	// Clamp a rank read from GetAA into a table index. Ranks come from the DB, so a chain that was
	// extended by hand could hand us a 6 -- reading past the end of a constexpr array would be far
	// worse than quietly capping at the top rank.
	inline int RankIndex(int rank)
	{
		if (rank > RANKS) {
			rank = RANKS;
		}
		return rank - 1;
	}
}

// =================================================================================================
// Triage Instinct + the Cleansing Renewal rank 5 payoff.
//
// Called from Mob::GetActSpellHealing, which is the single funnel every heal passes through and the
// only place where the caster, the target, the amount and the crit chance are all in scope at once.
// `this` is the HEALER.
//
// Returns a percentage to add to the heal, and writes any crit-chance bonus into crit_chance_add.
// =================================================================================================
int Mob::AoTv4HealBonus(Mob *target, uint16 spell_id, bool from_buff_tic, int &crit_chance_add)
{
	crit_chance_add = 0;

	// GetActSpellHealing is called with a null target from a couple of sites (buff tics and the
	// bard pulse path), and for heal-over-time tics. Neither should feed a burst-heal AA.
	if (!target || from_buff_tic || !IsValidSpell(spell_id)) {
		return 0;
	}

	int bonus = 0;

	// --- Triage Instinct: you heal hardest where it is needed most.
	if (target->GetHPRatio() < TRIAGE_HP_PCT) {
		const int rank = static_cast<int>(GetAA(AA_TRIAGE));
		if (rank > 0) {
			const int i = RankIndex(rank);
			bonus           += TRIAGE_HEAL[i];
			crit_chance_add += TRIAGE_CRIT[i];
		}
	}

	// --- Cleansing Renewal rank 5: a cure I landed on this target primes my next heal on them.
	// Consumed here whether or not Triage also fired. Cleared on use so it cannot be banked.
	if (m_aotv4_renewal_target == target->GetID() && m_aotv4_renewal_expire > Timer::GetCurrentTime()) {
		bonus += RENEWAL_NEXT_HEAL_PCT;
		m_aotv4_renewal_target = 0;
		m_aotv4_renewal_expire = 0;
	}

	return bonus;
}

// Rank 5 of Triage Instinct: a critical heal on a dying target refunds part of its mana.
// Only reached when AoTv4HealBonus already confirmed the target was below the threshold, so the
// health test is not repeated here -- but the rank is, because the bonus can also come from
// Cleansing Renewal, which does not pay mana back.
void Mob::AoTv4HealCritReward(Mob *target, uint16 spell_id)
{
	if (!IsClient() || !target || !IsValidSpell(spell_id)) {
		return;
	}
	if (target->GetHPRatio() >= TRIAGE_HP_PCT) {
		return;
	}
	if (static_cast<int>(GetAA(AA_TRIAGE)) < RANKS) {
		return;
	}

	const int refund = spells[spell_id].mana * TRIAGE_REFUND_PCT / 100;
	if (refund > 0) {
		SetMana(GetMana() + refund);
	}
}

// =================================================================================================
// Everything that keys off a heal having LANDED. Called at the end of Mob::HealDamage, so `this` is
// the HEALED TARGET and `caster` is the healer.
//
//   requested  what the heal was worth before the overheal clamp
//   acthealed  what it actually restored (0 if the target was already full)
//   pre_ratio  the target's health BEFORE the heal, which is what the thresholds mean
// =================================================================================================
void Mob::AoTv4HealerPostHeal(Mob *caster, uint64 requested, uint64 acthealed, uint16 spell_id, float pre_ratio)
{
	if (!caster || !IsValidSpell(spell_id)) {
		return;
	}

	// Heal-over-time tics are excluded throughout. A HoT ticking on a nearly-full target would
	// otherwise trickle out shields, echoes and death saves for the price of one cast.
	const bool from_hot = IsBuffSpell(spell_id);

	// ------------------------------------------------------------------ Overflowing Grace
	// ⚠️⚠️ GRACE IS DELIBERATELY THE ONE THING ABOVE THE `acthealed == 0` GATE BELOW, because a heal
	// landing on a target at FULL health is the case it exists for -- "healing past a target maximum
	// health", which is what the AA's own description promises. It used to sit below that gate, so a
	// fully wasted heal produced nothing at all and the ability appeared broken to anyone who tested
	// it the obvious way. Reported from play as "it doesn't put a buff or anything when overhealing".
	// ⚠️ Everything BELOW the gate still requires a real heal: Triage, Echo, Renewal and Breath would
	// all be farmable off a parked heal on a full-health target, and none of them is capped the way
	// this is.
	if (!from_hot && requested > acthealed && m_aotv4_grace_ready <= Timer::GetCurrentTime()) {
		const int rank = static_cast<int>(caster->GetAA(AA_GRACE));
		if (rank > 0) {
			const int    i        = RankIndex(rank);
			const uint64 overheal = requested - acthealed;
			int64        shield   = static_cast<int64>(overheal) * GRACE_PCT[i] / 100;
			const int64  cap      = GetMaxHP() * GRACE_CAP_PCT_OF_MAXHP / 100;

			if (shield > cap) {
				shield = cap;
			}

			if (shield > 0) {
				caster->SpellOnTarget(SPELL_GRACE_SHIELD, this, 0, false, 0, false, -1, GRACE_TICKS[i]);

				// The absorb pool is a share of an overheal, so it is only known now -- it cannot
				// live in the spell row. SPA 55 stores the pool per buff instance
				// (spell_effects.cpp:1411), so we find the buff we just applied and overwrite it.
				//
				// ⚠️⚠️ IT REFRESHES, IT DOES NOT STACK. This used to ADD to whatever was left of an
				// existing shield, which is what made parking a heal dangerous -- repeated casts
				// climbed straight to the cap and stayed there. One heal is worth one shield, so the
				// pool is REPLACED and the duration starts again.
				// ⚠️ It never DOWNGRADES, though: a small overheal landing on a big surviving shield
				// keeps the bigger number, which is the trap the old top-up comment was written to
				// avoid. So this is "refresh to the better of the two", not a blind overwrite.
				// 📌 For a strict overwrite instead, drop the std::max and assign `shield`.
				const uint32 slots = GetMaxTotalSlots();
				for (uint32 s = 0; s < slots; ++s) {
					if (buffs[s].spellid == SPELL_GRACE_SHIELD) {
						const int64 remaining = static_cast<int64>(buffs[s].melee_rune);
						buffs[s].melee_rune   = static_cast<uint32>(std::max(remaining, shield));
						m_aotv4_grace_rank    = static_cast<uint8>(rank);
						// ⚠️ The cooldown is stamped on the TARGET, not the healer, and only when a
						// shield is actually built -- a refused build must not start it. It is what
						// makes a parked heal safe: the shield can be rebuilt on me at most once per
						// window however many heals land, and by however many healers.
						m_aotv4_grace_ready   = Timer::GetCurrentTime() + GRACE_COOLDOWN_MS;
						break;
					}
				}
			}
		}
	}

	// ------------------------------------------------------------------ Borrowed Breath
	// The threshold is the health the target was AT when you healed them, not what they are at
	// afterwards -- the whole point is rewarding a save, and a good save ends above the line.
	if (pre_ratio < BREATH_HP_PCT) {
		const int rank = static_cast<int>(caster->GetAA(AA_BREATH));
		if (rank > 0) {
			int i = RankIndex(rank);

			// Rank 5 additionally ARMS the death save on the target. Arming costs nothing and is
			// not blocked by the cooldown -- the cooldown is checked and spent at the moment a
			// death is actually avoided, so several people can be armed at once but only the first
			// to die is saved.
			if (i == RANKS - 1) {
				m_aotv4_breath_save   = Timer::GetCurrentTime() + BREATH_SAVE_WINDOW_MS;
				m_aotv4_breath_healer = caster->GetID();
			}

			caster->SpellOnTarget(static_cast<uint16>(SPELL_BREATH_RANK1 + i), this);
		}
	}

	// ------------------------------------------------------------------ Mender's Echo
	// The echo is itself a heal and lands through HealDamage, so without a guard it would echo its
	// own echo. The guard is on the HEALER, so two healers can still echo independently.
	if (!from_hot && !caster->m_aotv4_heal_echoing) {
		const int rank = static_cast<int>(caster->GetAA(AA_ECHO));
		if (rank > 0) {
			const int i = RankIndex(rank);

			// Gather wounded allies near the heal's target, excluding the target itself: this is
			// meant to spread healing, not double up on the one person already being healed.
			Mob   *best   = nullptr;
			Mob   *second = nullptr;
			float  best_r = 101.0f;
			float  second_r = 101.0f;

			// There is no GetRaidByMob -- raids are a client-only concept here.
			Group *group = entity_list.GetGroupByMob(caster);
			Raid  *raid  = caster->IsClient() ? entity_list.GetRaidByClient(caster->CastToClient()) : nullptr;

			auto consider = [&](Mob *m) {
				if (!m || m == this || m->GetHPRatio() >= 100.0f) {
					return;
				}
				if (m->CalculateDistance(GetX(), GetY(), GetZ()) > ECHO_RANGE) {
					return;
				}
				const float r = m->GetHPRatio();
				if (r < best_r) {
					second = best;   second_r = best_r;
					best   = m;      best_r   = r;
				}
				else if (r < second_r) {
					second = m;      second_r = r;
				}
			};

			if (group) {
				for (int gi = 0; gi < MAX_GROUP_MEMBERS; ++gi) {
					consider(group->members[gi]);
				}
			}
			else if (raid) {
				for (const auto &member : raid->members) {
					consider(member.member);
				}
			}
			else {
				// Ungrouped, the echo has nowhere to go but back to the healer. Without this the
				// AA would be dead weight for solo play, which is a large share of this server.
				consider(caster);
			}

			if (best) {
				int64 echo = static_cast<int64>(acthealed) * ECHO_PCT[i] / 100;

				// Rank 5: the echo can crit. Deliberately a self-contained roll rather than routing
				// the echo back through GetActSpellHealing -- that would let it pick up Triage
				// Instinct and Cleansing Renewal too, and stack the whole tree onto one cast.
				if (i == RANKS - 1 && zone && zone->random.Roll(ECHO_CRIT_PCT)) {
					echo *= 2;
				}

				if (echo > 0) {
					caster->m_aotv4_heal_echoing = true;
					best->HealDamage(static_cast<uint64>(echo), caster, spell_id);

					if (second && rank >= 3) {
						const int64 half = echo * ECHO_SECOND_PCT / 100;
						if (half > 0) {
							second->HealDamage(static_cast<uint64>(half), caster, spell_id);
						}
					}
					caster->m_aotv4_heal_echoing = false;
				}
			}
		}
	}
}

// =================================================================================================
// Cleansing Renewal. Called from the four counter-cure sites in spell_effects.cpp, at the moment a
// poison, disease, curse or corruption effect is fully removed. `this` is the CURER.
//
// The engine already had the hook we needed: CastOnCurer/CastOnCure fire exactly there.
// =================================================================================================
void Mob::AoTv4CureRenewal(Mob *target)
{
	if (!target) {
		return;
	}

	const int rank = static_cast<int>(GetAA(AA_RENEWAL));
	if (rank < 1) {
		return;
	}

	const int i = RankIndex(rank);

	// Scaled by the curer's level rather than flat, so it does not become irrelevant by the cap.
	const int64 heal = static_cast<int64>(GetLevel()) * RENEWAL_PER_LEVEL[i];
	if (heal > 0) {
		target->HealDamage(static_cast<uint64>(heal), this);
	}

	if (rank >= 3) {
		SpellOnTarget(SPELL_RENEWAL_REGEN, target);
	}

	if (rank >= RANKS) {
		// Prime the next heal I cast on this target. Consumed in AoTv4HealBonus.
		m_aotv4_renewal_target = target->GetID();
		m_aotv4_renewal_expire = Timer::GetCurrentTime() + RENEWAL_WINDOW_MS;
	}
}

// =================================================================================================
// Overflowing Grace rank 5: the shield gave everything it had.
//
// Called from Mob::RuneAbsorb when the shield's pool is exhausted and the buff is about to fade --
// NOT when it merely absorbs a hit. "Fully absorbs" is read as "was spent entirely", so this pays
// once per shield instead of once per absorbed swing, which would be constant.
//
// `this` is the shielded mob. The healer is long out of the picture by now, which is why the rank
// that built the shield was stashed on the target when it was applied.
// =================================================================================================
void Mob::AoTv4GraceShieldSpent()
{
	if (m_aotv4_grace_rank < RANKS) {
		m_aotv4_grace_rank = 0;
		return;
	}
	m_aotv4_grace_rank = 0;

	SpellOnTarget(SPELL_GRACE_RENEWED, this);
}

// =================================================================================================
// Borrowed Breath rank 5: survive the blow that would have killed you.
//
// Called from Mob::Damage on the killing blow, BEFORE the native saves. `this` is the dying mob.
//
// ⚠️ WHY THIS IS HAND-ROLLED RATHER THAN A NATIVE SPA. Neither native death save is usable here:
//
//   SPA 232 DivineSave is the only effect that fires on an actual killing blow, but
//   Mob::TryDivineSave unconditionally casts 4789 Touch of the Divine on top of the save -- up to
//   36 seconds of Divine Aura. Under it you cannot attack (attack.cpp), cannot cast (spells.cpp),
//   and no other caster can land ANY spell on you, heals included (spells.cpp:4147, "invuln mobs
//   can't be affected by any spells, good or bad"). You are also unattackable, so a boss drops the
//   tank it just failed to kill and turns on the healer who saved them. It keeps you alive by
//   removing you from the fight, which is worse than not saving you.
//
//   SPA 150 DeathSave -- Divine Intervention and Death Pact -- is NOT a death save in this
//   codebase at all. Mob::TryDeathSave is called from the ELSE branch of the death check
//   (attack.cpp), only when you cross below 16 percent health AND SURVIVE. It never runs on the
//   killing blow.
//
// So the save is done here: no invulnerability, no lockout, no aggro drop. You come back on a
// sliver of health and can still be healed, which is the entire point of the ability.
// =================================================================================================
bool Mob::AoTv4TryBorrowedBreath()
{
	if (m_aotv4_breath_save <= Timer::GetCurrentTime()) {
		return false;
	}

	// The two-minute limit belongs to the HEALER, and it is spent here rather than at arming time.
	// Arming is free, so a healer can have saves standing on several people at once -- but only the
	// first death they actually avert is prevented, which is what "once every two minutes" means.
	Mob *healer = entity_list.GetMob(m_aotv4_breath_healer);
	if (healer && healer->m_aotv4_breath_ready > Timer::GetCurrentTime()) {
		m_aotv4_breath_save   = 0;   // this one is spent whether or not it fired
		m_aotv4_breath_healer = 0;
		return false;
	}

	m_aotv4_breath_save   = 0;   // one shot
	m_aotv4_breath_healer = 0;

	// A healer who has zoned, died or despawned cannot be put on cooldown. The save still lands:
	// the arming window is only 20 seconds, so there is no meaningful way to farm that, and the
	// alternative punishes the target for something the healer did.
	if (healer) {
		healer->m_aotv4_breath_ready = Timer::GetCurrentTime() + BREATH_SAVE_COOLDOWN_MS;
	}

	int64 hp = GetMaxHP() * BREATH_SAVE_HP_PCT / 100;
	if (hp < 1) {
		hp = 1;
	}
	SetHP(hp);
	SendHPUpdate();

	// Left at 1 HP you simply die to the next tick of anything, which makes the save feel like a
	// formality. A sliver with some substance is what buys the "one more breath".
	Message(Chat::Emote, "You draw one more breath.");
	entity_list.MessageClose(this, true, 200, Chat::MeleeCrit,
		"%s draws one more breath!", GetCleanName());

	return true;
}

// =====================================================================================
// Floating combat text -- the HEALING feed.
//
// ⚠️⚠️ HEALING IS NOT ON THE WIRE ANYWHERE. Damage reaches the client as OP_Damage
// (CombatDamage_Struct, 30 bytes on RoF2) carrying source, target and amount, which is what the
// dll's floating damage numbers read. There is NO equivalent for healing: Mob::HealDamage
// communicates a heal purely through FilteredMessageString, i.e. localised chat text with the
// amount baked into a string. So a floating-heal display cannot be built client-side from packets
// at all -- it needs this.
//
// 📌 Sent from HealDamage, which is the single choke point every heal passes through: direct heals,
// HoT tics, lifetaps and the Lua-paid heals all arrive here. Same reason section 22 puts the
// endurance cost in DoSpecialAttackDamage rather than in each ability.
//
// ⚠️⚠️ OPT-IN, VIA AoTv4WantsFctHeals. Without that gate a player running a vanilla client would see
// raw "FCTHEAL 12|340|1234|7" text on every tick of every heal landing anywhere near them. The dll
// announces itself once with "/say fctheal 1"; until it does, nothing is sent and the cost is one
// boolean test per heal.
//
// Wire: FCTHEAL <target_entity_id>|<amount>|<spell_id>|<caster_entity_id>
// ⚠ ENTITY ids, not character ids -- the dll resolves them with GetSpawnByID to place the number,
// exactly as it does for the damage packet. A character id would not resolve to a spawn.
// Shared by the heal and spell-damage feeds. `this` is the TARGET in both cases.
// What produced this damage or healing, as the player would name it.
// ⚠️⚠️ NOT THE SKILL NAME. A class ability swings with the WEAPON's skill (aotv4_class_abilities.lua),
// so labelling by skill would file Reckless Cleave under "2H Slashing" alongside the autoattacks it is
// meant to be compared against -- which is the same confusion that made it look like the ability was
// not showing up at all. The Lua names the ability in `aotv4_fct_ability` when it fires one.
// 📌 Ordinary melee collapses to one line per weapon skill on purpose: splitting autoattack any finer
// is noise, and the breakdown exists to answer "what is my damage made of", not "list every swing".
static std::string AoTv4MeterLabel(Mob *source, uint16 spell_id, const char *fallback)
{
	if (IsValidSpell(spell_id)) { return spells[spell_id].name; }

	if (source && !source->GetEntityVariable("aotv4_fct_swing").empty()) {
		const std::string id = source->GetEntityVariable("aotv4_fct_ability");
		const uint16 aid = (uint16)atoi(id.c_str());
		if (IsValidSpell(aid)) { return spells[aid].name; }
	}

	// ⚠️ The caller supplies the fallback because the two feeds mean different things by "no spell": a
	// damage event with no spell is a weapon swing, a HEAL with no spell is one paid by Lua (the
	// Moonfire and Thirst lines, section 5). Passing a skill enum for both put Lua heals under
	// "Begging", which is the skill id that happened to be handy.
	return fallback ? std::string(fallback) : std::string("Other");
}

// Is anybody in this exchange actually listening? Split out so the damage path can answer it BEFORE
// doing any work, and so the answer cannot drift from what AoTv4FctSend goes on to do.
static bool AoTv4FctWanted(Mob *target, Mob *source)
{
	Client *t = (target && target->IsClient()) ? target->CastToClient() : nullptr;
	Client *s = (source && source->IsClient()) ? source->CastToClient() : nullptr;
	return (t && t->AoTv4WantsFctHeals()) || (s && s->AoTv4WantsFctHeals());
}

static void AoTv4FctSend(Mob *target, Mob *source, const char *tag, uint32 amount, uint16 spell_id,
                         int kind = 0)
{
	if (!target || amount == 0) { return; }

	// ⚠️ Built ONLY when somebody is actually listening. This runs on every damage event on the server,
	// so an unconditional snprintf would be real work done for nothing on a busy zone.
	Client *t = target->IsClient() ? target->CastToClient() : nullptr;
	Client *s = (source && source->IsClient()) ? source->CastToClient() : nullptr;
	const bool want_t = t && t->AoTv4WantsFctHeals();
	const bool want_s = s && s != t && s->AoTv4WantsFctHeals();
	if (!want_t && !want_s) { return; }

	char line[128];
	snprintf(line, sizeof(line), "%s %u|%u|%u|%u|%d", tag,
	         (unsigned)target->GetID(), (unsigned)amount, (unsigned)spell_id,
	         (unsigned)(source ? source->GetID() : 0), kind);

	// ⚠️⚠️ Chat::White, AND THE PAYLOAD GOES THROUGH A "%s" FORMAT ARGUMENT. Two separate traps:
	//
	//   1. NOT Chat::NonMelee. That is the spell damage channel, and this codebase already records it
	//      as sitting behind the player's FilterSpellDamage setting -- aotv4_class_abilities.lua calls
	//      it "the first thing to check when spell damage seems to be missing". A transport the player
	//      can silently filter off is not a transport. Chat::White is what LOOTDATA and every other
	//      AoTv4 transport uses (client_packet.cpp:10609).
	//   2. Client::Message IS PRINTF-STYLE. Passing the payload as the format string means a stray
	//      '%' in it is eaten as a format token -- the same trap section 14 records for spell
	//      descriptions. There is no '%' here today, but a future field could carry one.
	if (want_t) { t->Message(Chat::White, "%s", line); }
	if (want_s) { s->Message(Chat::White, "%s", line); }
}

// ⚠️⚠️ THE SERVER IS AUTHORITATIVE FOR FLOATING COMBAT TEXT. THE CLIENT NO LONGER READS OP_Damage.
//
// Reading the damage packet was the obvious design and it cannot be made to work, because there is no
// single packet path. Three separate holes, each found the hard way:
//
//   1. A DAMAGE OVER TIME TIC SENDS NOTHING. The whole OP_Damage block in Mob::CommonDamage sits under
//      `if (!iBuffTic)`, with the stock comment "buff ticks do not send damage".
//   2. CLIENT SPELL DAMAGE SENDS TEXT, NOT A PACKET -- `FilteredMessageCloseString(... HIT_NON_MELEE)`,
//      under a stock comment reading "special crap for spell damage, looks hackish to me".
//   3. WHAT IS LEFT IS GATED ON THE PLAYER'S OWN CHAT FILTERS. The broadcast passes an eqFilterType,
//      so FilterSpellDamage or FilterDOT set to hide means no packet -- which is why direct nukes
//      appeared "sometimes". aotv4_class_abilities.lua already records this as the first thing to
//      check when spell damage goes missing.
//
// 📌 Chasing those branch by branch produced a display that showed melee and nothing else, and each
// fix risked double-drawing whatever the packet path DID cover. Sending from one choke point instead
// makes the feed exhaustive by construction: if damage happened, this ran.
//
// ⚠️⚠️ OPT-IN AND NARROW. Only clients whose dll asked for it receive anything, and only for damage
// they dealt or took -- never for other people's fights, which is what would make this a chat flood.
// The dll's "everyone else" toggle therefore has nothing to show and is deliberately not fed here.
//
// kind: 0 melee (including combat abilities and procs), 1 direct spell, 2 damage over time.
void Mob::AoTv4SendFctDamage(Mob *attacker, uint64 damage, uint16 spell_id, bool iBuffTic,
                             EQ::skills::SkillType skill_used)
{
	if (damage == 0) { return; }

	// ---- damage meter -------------------------------------------------------------------------
	// ⚠️⚠️ RECORDED BEFORE THE FLOATING-TEXT OPT-IN TEST, DELIBERATELY. The meter must accumulate for
	// everyone in the fight whether or not THIS client is running the dll -- otherwise a group's totals
	// would silently depend on who had which client build, and the one number a meter exists to compare
	// would be the one that is wrong.
	// 📌 Both features are fed from this one function because it is the only place that sees every
	// damage event: melee, spell, damage over time and combat abilities alike.
	{
		Client *pc_attacker = (attacker && attacker->IsClient()) ? attacker->CastToClient() : nullptr;
		Client *pc_victim   = IsClient() ? CastToClient() : nullptr;

		if (pc_attacker && IsNPC() && !HasOwner()) {
			// A player hitting a creature: their contribution to that creature's encounter.
			m_aotv4_meter.Begin();
			if (auto *row = m_aotv4_meter.Row(pc_attacker->CharacterID(), pc_attacker->GetCleanName())) {
				row->damage += (int64)damage;
				row->NoteSecond((int64)damage);
				// ⚠️ A landed hit is an attempt too. The miss path above counts the other kind; both have
				// to increment attempts or the accuracy column reads as 0 percent.
				++row->attempts;

				const std::string skill = EQ::skills::GetSkillName(skill_used);
				if (auto *part = row->Part(AoTv4MeterLabel(attacker, spell_id,
				                            skill.empty() ? "Melee" : skill.c_str()))) {
					part->damage += (int64)damage;
					++part->hits;
					if ((int64)damage > part->max_hit) { part->max_hit = (int64)damage; }
				}
			}
			pc_attacker->AoTv4MeterNote(this);
		}
		else if (pc_victim && attacker && attacker->IsNPC() && !attacker->HasOwner()) {
			// A creature hitting a player: damage taken, recorded on the CREATURE's encounter so it sits
			// beside the damage that player dealt to it.
			attacker->m_aotv4_meter.Begin();
			if (auto *row = attacker->m_aotv4_meter.Row(pc_victim->CharacterID(), pc_victim->GetCleanName())) {
				row->taken += (int64)damage;
			}
			pc_victim->AoTv4MeterNote(attacker);
		}
	}

	// ⚠️⚠️ THE OPT-IN TEST COMES FIRST, BEFORE ANY WORK. Everything below -- an entity variable lookup,
	// which is a string map probe -- runs on EVERY damage event on the server, and for the overwhelming
	// majority of them nobody involved is running the feature at all.
	if (!AoTv4FctWanted(this, attacker)) { return; }

	// ⚠️⚠️ AN AoTv4 CLASS ABILITY IS INDISTINGUISHABLE FROM AN AUTOATTACK BY THE TIME IT REACHES HERE.
	// aotv4_class_abilities.lua swings them with the WEAPON's own skill, not with Bash or Kick, so a
	// Reckless Cleave arrives as an ordinary 2H Slashing hit -- same skill, same path, same message.
	// Keying on the skill therefore catches the STOCK specials and misses every class ability, which is
	// what "Reckless Cleave just does not show up" turned out to be: it was showing, in the same white
	// as the autoattacks either side of it.
	// 📌 The Lua sets an entity variable immediately before the swing, because that is the only layer
	// that knows. Consumed and cleared here, so the next ordinary swing is not mis-tagged.
	// ⚠️ At most ONE later hit can be mis-coloured: if DoSpecialAttackDamage bails before dealing any
	// damage (too winded, see section 22), the flag survives until the following hit clears it. Living
	// with that beats a timestamp on a per-damage-event path.
	bool from_ability = false;
	if (attacker && !attacker->GetEntityVariable("aotv4_fct_swing").empty()) {
		from_ability = true;
		// ⚠️ Only the per-SWING flag is cleared. `aotv4_fct_ability` names which ability it was and is
		// left alone -- it is only ever read while this flag is set, so a stale value cannot be misread.
		attacker->SetEntityVariable("aotv4_fct_swing", "");
	}

	int kind;
	if (iBuffTic)                    { kind = 2; }
	else if (IsValidSpell(spell_id)) { kind = 1; }
	else if (from_ability)           { kind = 3; }
	else {
		switch (skill_used) {
			// The stock activated specials, which DO carry their own skill.
			case EQ::skills::SkillBash:        case EQ::skills::SkillKick:
			case EQ::skills::SkillBackstab:    case EQ::skills::SkillFrenzy:
			case EQ::skills::SkillDragonPunch: case EQ::skills::SkillEagleStrike:
			case EQ::skills::SkillFlyingKick:  case EQ::skills::SkillRoundKick:
			case EQ::skills::SkillTigerClaw:   case EQ::skills::SkillIntimidation:
				kind = 3; break;
			default:
				kind = 0; break;
		}
	}

	const uint32 amount = (damage > 0x7FFFFFFFull) ? 0x7FFFFFFFu : (uint32)damage;
	AoTv4FctSend(this, attacker, "FCTDMG", amount, spell_id, kind);
}

void Mob::AoTv4SendFctHeal(Mob *caster, uint64 requested, uint64 acthealed, uint16 spell_id)
{
	if (acthealed == 0) { return; }

	// ⚠️ Clamped to what the transport and the client can carry. A heal larger than this is either a
	// full heal on an enormous pool or a bug, and either way the exact figure is not what the player
	// is reading off a number that fades in under a second.
	const uint32 amount = (acthealed > 0x7FFFFFFFull) ? 0x7FFFFFFFu : (uint32)acthealed;

	// ---- damage meter -------------------------------------------------------------------------
	// ⚠️ A heal names the healer and the healed; neither is the creature. It attaches to whatever fight
	// the HEALED player is in, which is why Client::m_aotv4_meter_npc exists at all.
	if (caster && caster->IsClient() && IsClient()) {
		Client *healed = CastToClient();
		Mob    *npc    = entity_list.GetMobID(healed->m_aotv4_meter_npc);
		if (npc && npc->m_aotv4_meter.Active()) {
			Client *healer = caster->CastToClient();
			if (auto *row = npc->m_aotv4_meter.Row(healer->CharacterID(), healer->GetCleanName())) {
				row->healing += (int64)acthealed;
				// ⚠️ `requested` is what the spell wanted to heal and `acthealed` is what it could. The
				// client is only ever told the second, which is why a log parser cannot compute this.
				if (requested > acthealed) { row->overheal += (int64)(requested - acthealed); }
				if (auto *part = row->Part(AoTv4MeterLabel(caster, spell_id, "Heal"))) {
					part->healing += (int64)acthealed;
				}
			}
		}
	}

	AoTv4FctSend(this, caster, "FCTHEAL", amount, spell_id);
}
