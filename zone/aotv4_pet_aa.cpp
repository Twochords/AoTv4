/*	EQEMu: Everquest Server Emulator

	AoTv4 pet wards, and the Ranged AA that shares one with its owner.

	Stock EQ gives only the Magician fire pet a standing self-buff (its damage shield). Here every
	family gets one suited to what it is -- see custom/sql/aotv4_pet_wards.sql -- and Kindred Bond
	hands a copy to the owner.

	⚠️ THE FAMILY IS READ FROM THE `pets`.`type` STRING, NOT FROM THE OWNER'S CLASS. A Magician has
	four thematically different pets and they must not all receive the same ward. Matching is by
	PREFIX so that every rank of a line resolves together: SumFireR10 through SumFireR37 are all
	"SumFire". Anything unmatched still gets a ward -- the fallback exists so "every pet has one"
	is literally true, including quest pets, swarm pets and whatever gets added later.
*/

#include "../common/spdat.h"
#include "client.h"
#include "entity.h"
#include "groups.h"
#include "mob.h"
#include "raids.h"
#include "zone.h"

#include <cstring>

extern Zone *zone;

namespace
{
	// Kindred Bond -- host 32 Finishing Blow, ranks 119,120,121,440,441. Ranged tab (type 3).
	constexpr int AA_KINDRED = 119;
	constexpr int RANKS      = 5;

	// How long the owner keeps the ward after the pet dies, at rank 5. In TICKS: buff durations are
	// whole 6 second tics, so 10 is a minute.
	constexpr int KINDRED_LINGER_TICKS = 10;

	struct PetWard {
		const char *prefix;
		uint16      spell_id;
	};

	// Longest-meaningful prefixes first is not required here because none of these is a prefix of
	// another, but keep that in mind if more are added.
	constexpr PetWard kPetWards[] = {
		{ "SumFire",     43420 },   // Magician fire      -- damage shield
		{ "SumWater",    43421 },   // Magician water     -- regeneration
		{ "SumEarth",    43422 },   // Magician earth     -- flat damage reduction
		{ "SumAir",      43423 },   // Magician air       -- attack speed
		{ "skel_pet",    43424 },   // Necromancer        -- melee lifetap
		{ "animateDead", 43424 },
		{ "blood_skel",  43424 },
		{ "BLpet",       43425 },   // Beastlord warder   -- melee damage
		{ "Animation",   43426 },   // Enchanter          -- absorb rune
		{ "SpiritWolf",  43427 },   // spirit wolf        -- avoidance
		{ "Familiar",    43428 },   // familiars          -- mana regeneration
		{ "CasterWolf",  43428 },
	};

	constexpr uint16 SPELL_WARD_FALLBACK = 43429;

	uint16 WardForPetType(const char *pet_type)
	{
		if (!pet_type) {
			return SPELL_WARD_FALLBACK;
		}
		for (const auto &w : kPetWards) {
			if (strncasecmp(pet_type, w.prefix, strlen(w.prefix)) == 0) {
				return w.spell_id;
			}
		}
		return SPELL_WARD_FALLBACK;
	}

	inline int RankIndex(int rank)
	{
		if (rank > RANKS) {
			rank = RANKS;
		}
		return rank - 1;
	}
}

// =================================================================================================
// Called from Mob::MakePoweredPet once the pet is in the entity list. `this` is the OWNER.
//
// The ward goes on the pet from the PET, so inspecting it credits the pet rather than the summoner.
// =================================================================================================
void Mob::AoTv4ApplyPetWard(Mob *pet, const char *pet_type)
{
	if (!pet) {
		return;
	}

	const uint16 ward = WardForPetType(pet_type);
	pet->SpellOnTarget(ward, pet);

	// ⚠️⚠️ REMEMBERED UNCONDITIONALLY, not just when Kindred Bond is trained. Two things need it and
	// only one of them is the AA:
	//   * AoTv4PetWardEnded strips the owner's copy when the pet dies (the original reason).
	//   * AoTv4RefreshPetWard re-applies it every 6 seconds if it has gone missing, and CANNOT
	//     recompute it -- `Mob::GetPetType()` returns the pet-type ENUM, not the `pets`.`type`
	//     string, and the string is only in scope here at creation.
	// Setting it at rank 0 is harmless: AoTv4PetWardEnded then fades a buff the owner never had.
	m_aotv4_petward_spell = ward;

	// ------------------------------------------------------------------ Kindred Bond
	const int rank = static_cast<int>(GetAA(AA_KINDRED));
	if (rank < 1) {
		return;
	}
	pet->SpellOnTarget(ward, this);

	// Rank 2 spreads it to the group. Cast from the pet as well, for the same reason.
	if (rank >= 2) {
		Group *group = entity_list.GetGroupByMob(this);
		if (group) {
			for (int i = 0; i < MAX_GROUP_MEMBERS; ++i) {
				Mob *m = group->members[i];
				if (m && m != this) {
					pet->SpellOnTarget(ward, m);
				}
			}
		}
	}

	Message(Chat::Emote, "You share your companion's nature.");
}

// =================================================================================================
// Called from NPC::Death when a pet dies. `this` is the OWNER.
//
// Without this the owner would keep the ward indefinitely -- the rows carry an effectively
// permanent duration on purpose, because the pet's life is what the ward is supposed to be bound to,
// not a timer.
// =================================================================================================
void Mob::AoTv4PetWardEnded()
{
	if (!m_aotv4_petward_spell) {
		return;
	}

	const uint16 ward = m_aotv4_petward_spell;
	m_aotv4_petward_spell = 0;

	const int rank = static_cast<int>(GetAA(AA_KINDRED));

	// Rank 5: the ward outlives the pet for a minute. Re-applied with a short duration rather than
	// simply left alone, so it cannot be kept forever by never summoning again.
	if (rank >= RANKS) {
		BuffFadeBySpellID(ward);
		SpellOnTarget(ward, this, 0, false, 0, false, -1, KINDRED_LINGER_TICKS);
		Message(Chat::Emote, "Your companion is gone, but its nature lingers.");
	}
	else {
		BuffFadeBySpellID(ward);
	}

	// The group's copies always go when the pet does, at every rank.
	Group *group = entity_list.GetGroupByMob(this);
	if (group) {
		for (int i = 0; i < MAX_GROUP_MEMBERS; ++i) {
			Mob *m = group->members[i];
			if (m && m != this) {
				m->BuffFadeBySpellID(ward);
			}
		}
	}
}

// =================================================================================================
// Called every 6 seconds from Client::Process (beside CalcItemScale). `this` is the OWNER.
//
// The ward is applied once, in Mob::MakePoweredPet -- at summon, and again on zone-in because the
// restore path re-runs that same function. Nothing refreshed it in between, so anything that removed
// it left the pet permanently without it until it was resummoned.
//
// ⚠️ THE REALISTIC WAY TO LOSE IT IS BUFF SLOT PRESSURE, NOT DISPEL. There is no dispel on this
// server, so that is not the case being defended against: a pet has a limited buff bar, and an owner
// stacking enough buffs on it can push the ward out of the last slot. That is silent, permanent, and
// indistinguishable from the ward never having existed.
//
// ⚠️ Deliberately hung off an EXISTING per-client timer rather than a new one, and off a CLIENT timer
// rather than the pet's AI tick: this is one FindBuff per player every six seconds, bounded by player
// count. On the NPC side it would run for every creature in the zone.
//
// ⚠️ Re-applied FROM THE PET, matching MakePoweredPet, so inspecting the buff still credits the pet
// rather than the summoner. Kindred Bond's copies on the owner and group are NOT refreshed here --
// those are bound to the pet's life and are torn down by AoTv4PetWardEnded; reapplying them on a
// timer would resurrect a share the owner is no longer entitled to.
// =================================================================================================
void Mob::AoTv4RefreshPetWard()
{
	Mob *pet = GetPet();
	if (!pet || pet->GetHP() <= 0) {
		return;
	}

	// ⚠️ The ward is the one recorded at summon, NOT recomputed. `Mob::GetPetType()` looks like the
	// right call and is not -- it returns the pet-type enum (charmed, animation, familiar), while the
	// ward is keyed on the `pets`.`type` STRING, which is only in scope inside MakePoweredPet.
	const uint16 ward = m_aotv4_petward_spell;
	if (!IsValidSpell(ward)) {
		return;
	}

	if (pet->FindBuff(ward)) {
		return;   // still there; nothing to do
	}

	pet->SpellOnTarget(ward, pet);
}
