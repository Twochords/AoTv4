#pragma once

// AoTv4 damage meter -- who contributed what to the creature you are fighting.
// =========================================================================================
// ⚠️⚠️ THE ENCOUNTER LIVES ON THE NPC, NOT ON EACH PLAYER, AND THAT IS FORCED BY THE DESIGN.
// An encounter here is bounded by ONE creature dying. If each player accumulated their own totals,
// two group members fighting different mobs would be summed into a single meaningless row, and a
// fight with adds would attribute the adds' damage to the boss. Hanging the accumulator off the
// target means every row on screen is, by construction, damage done to the same creature.
//
// 📌 It is fed from the SAME choke point as the floating combat text (Mob::CommonDamage, via
// AoTv4SendFctDamage) rather than from a second hook. That path already sees every damage event in
// the game including damage over time, spell damage and combat abilities -- the three cases the
// OP_Damage packet path misses entirely -- so anything the meter can miss, the numbers on screen miss
// too, and the two can never disagree about what happened.
//
// ⚠️ Healing does not attach to a target: a healer heals the tank, not the mob. It is attributed to
// the encounter that the HEALED player is currently in (Client::m_aotv4_meter_npc), which is set
// whenever that player deals damage to or takes damage from a creature.

#include "../common/types.h"
#include <string>
#include <vector>

// One line of the drill-down: "where did this player's damage actually come from".
// ⚠️ Labelled by SOURCE, not by skill id, because the answer a player wants is "Reckless Cleave" or
// "Spike of Disease", not "2H Slashing". Melee collapses to one row per weapon skill on purpose --
// splitting autoattack by hand would be noise.
struct AoTv4MeterPart {
	std::string label;
	int64       damage  = 0;
	int64       healing = 0;
	uint32      hits    = 0;   // landed
	int64       max_hit = 0;   // biggest single one, which is what "did that crit" usually means
};

struct AoTv4MeterRow {
	uint32      char_id = 0;
	std::string name;
	int64       damage  = 0;   // dealt to this creature
	int64       healing = 0;   // healing this player did during this encounter
	int64       taken   = 0;   // damage this creature dealt to this player

	// ⚠️⚠️ EXACT, NOT INFERRED. A log parser has to read these out of message text and guess at the ones
	// the client never prints; every avoidance outcome arrives here as a negative sentinel from
	// zone/common.h (blocked -1, parried -2, riposted -3, dodged -4, invulnerable -5, rune -6), so
	// attempts and misses are counted rather than estimated.
	uint32      attempts = 0;   // swings and casts aimed at the creature
	uint32      avoided  = 0;   // of those, the ones that did nothing

	// ⚠️ Overheal is the one number a log parser CANNOT get: the client is never told what a heal
	// wanted to be, only what it did. Mob::HealDamage has both, so this is free and it is exact.
	int64       overheal = 0;

	// Best single second of damage -- burst, as against sustained. Kept as a one-second bucket rather
	// than a full timeline: a timeline is a chart, and this is a column.
	int64       best_sec = 0;
	int64       cur_sec  = 0;
	time_t      cur_sec_at = 0;

	std::vector<AoTv4MeterPart> parts;

	// Fold one damage event into the per-second bucket, rolling it over when the second changes.
	void NoteSecond(int64 amount);

	// Find or create the breakdown line for one source.
	AoTv4MeterPart *Part(const std::string &label);
};

class AoTv4Encounter {
public:
	// Returns the row for this character, creating it on first contribution.
	AoTv4MeterRow *Row(uint32 char_id, const char *name);

	void  Begin();                       // stamps the start time if this is the first event
	bool  Active() const { return m_start != 0; }
	uint32 Seconds() const;              // at least 1, so a one-shot kill is not a divide by zero

	// Freeze the clock. Called when the creature dies, so an archived fight does not keep ticking.
	void  Close(const std::string &target_name);

	// Fold this encounter's rows and breakdowns into another.
	// ⚠️⚠️ THE LIVE VIEW IS A MERGE OF EVERY CREATURE YOU ARE CURRENTLY FIGHTING, AND IT HAS TO BE.
	// An encounter is per creature, so in an AoE pull there are several live at once and a display that
	// showed "the one you hit most recently" flips between them several times a second -- reported from
	// play as bouncing between mobs. Merging costs nothing and makes an AoE read as one pull.
	// 📌 HISTORY stays per creature: each one archives its own fight as it dies, which is the encounter
	// rule that was actually chosen. Only the live view aggregates.
	void  MergeInto(AoTv4Encounter &out) const;

	// Earliest start across a merge, so a pull is timed from its first hit and not its latest.
	time_t StartTime() const { return m_start; }
	void   AdoptStart(time_t t);

	const std::vector<AoTv4MeterRow> &Rows() const { return m_rows; }
	const std::string &TargetName() const { return m_target; }

	int64 TotalDamage() const;

private:
	std::vector<AoTv4MeterRow> m_rows;
	std::string m_target;
	time_t m_start = 0;
	time_t m_end   = 0;   // 0 while the fight is live
};
