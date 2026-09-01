#include "client.h"
#include "aotv4_meter.h"
#include "../common/strings.h"
#include "entity.h"
#include <fmt/format.h>
#include <ctime>

// ⚠️ Linear scan, not a map. A row per contributor means at most a raid's worth of entries, and the
// vector is walked once per damage event -- a std::map's allocation per insert costs more than the
// scan saves at this size, and the ORDER matters for display (first to contribute reads first).
AoTv4MeterRow *AoTv4Encounter::Row(uint32 char_id, const char *name)
{
	for (auto &r : m_rows) {
		if (r.char_id == char_id) { return &r; }
	}

	// ⚠️ Bounded. A creature the whole server piles onto (a world boss, section 17c, allows 72 looters)
	// must not grow this without limit -- the whole thing is rebuilt into a chat line every second.
	if (m_rows.size() >= 24) { return nullptr; }

	AoTv4MeterRow row;
	row.char_id = char_id;
	row.name    = name ? name : "";
	m_rows.push_back(row);
	return &m_rows.back();
}

// ⚠️ Bounded like the row list. A caster with a deep spell book plus melee plus procs can produce a
// long tail; past this the breakdown stops being readable anyway.
AoTv4MeterPart *AoTv4MeterRow::Part(const std::string &label)
{
	for (auto &p : parts) {
		if (p.label == label) { return &p; }
	}
	if (parts.size() >= 16) { return nullptr; }
	AoTv4MeterPart p;
	p.label = label;
	parts.push_back(p);
	return &parts.back();
}

void AoTv4Encounter::Close(const std::string &target_name)
{
	if (m_end == 0) { m_end = time(nullptr); }
	if (m_target.empty()) { m_target = target_name; }
}

int64 AoTv4Encounter::TotalDamage() const
{
	int64 t = 0;
	for (const auto &r : m_rows) { t += r.damage; }
	return t;
}

void AoTv4Encounter::Begin()
{
	if (m_start == 0) { m_start = time(nullptr); }
}

uint32 AoTv4Encounter::Seconds() const
{
	if (m_start == 0) { return 1; }
	// ⚠️ A CLOSED fight measures to its end, not to now. Without this an archived encounter's duration
	// keeps growing while you look at it, and every rate in it falls towards zero.
	const time_t now = (m_end != 0) ? m_end : time(nullptr);
	const time_t d   = (now > m_start) ? (now - m_start) : 0;
	// ⚠️ Never zero. A creature killed inside one second is entirely normal at low level, and dividing
	// by it would report an infinite rate rather than a large one.
	return (uint32)(d < 1 ? 1 : d);
}

// Remember which fight this player is in, so a heal on them can be attributed.
// ⚠️ Only real creatures. A pet, a swarm pet or another player is not an encounter, and stamping one
// here would send every heal into a fight that does not exist.
void Client::AoTv4MeterNote(Mob *npc)
{
	if (!npc || !npc->IsNPC() || npc->HasOwner()) { return; }

	const uint16 id = npc->GetID();
	m_aotv4_meter_npc = id;   // the most recent, which is what a heal attaches to

	// Move to front, deduped, so the list is "what am I fighting" and not "everything I have ever hit".
	for (size_t i = 0; i < m_aotv4_meter_engaged.size(); ++i) {
		if (m_aotv4_meter_engaged[i] == id) {
			if (i) { m_aotv4_meter_engaged.erase(m_aotv4_meter_engaged.begin() + i);
			         m_aotv4_meter_engaged.insert(m_aotv4_meter_engaged.begin(), id); }
			return;
		}
	}
	m_aotv4_meter_engaged.insert(m_aotv4_meter_engaged.begin(), id);
	// ⚠️ 12 is a generous AoE and a hard stop. Dead ids fall out on their own -- they simply stop
	// resolving -- so nothing has to prune this on a timer.
	if (m_aotv4_meter_engaged.size() > 12) { m_aotv4_meter_engaged.resize(12); }
}

// Push the current encounter to this client.
// Wire: METERDATA <target>|<seconds>^name|damage|healing|taken^...
//
// ⚠️⚠️ THE ROWS COME OFF THE TARGET, NOT OFF THE GROUP. Walking the group and reading each member's
// own totals would be wrong for the same reason the accumulator is not per player: two members on
// different mobs would appear in one another's fight. Everyone who contributed to THIS creature is
// already in its row list, group member or not -- which also means an outside helper shows up, which
// is correct and is what a meter is for.
// Defined below, beside the history it falls back to.
static const AoTv4Encounter *AoTv4MeterCurrent(Client *c, AoTv4Encounter &scratch, std::string &name_out);

void Client::AoTv4MeterSend()
{
	if (!m_aotv4_meter_on) { return; }

	AoTv4Encounter scratch;
	std::string    name;
	const AoTv4Encounter *enc = AoTv4MeterCurrent(this, scratch, name);
	if (!enc) { return; }

	const char *target = name.c_str();
	const uint32 secs = enc->Seconds();

	std::string line = fmt::format("METERDATA {}|{}|{}", target, secs, m_aotv4_meter_view);
	for (const auto &r : enc->Rows()) {
		// ⚠️ A row with nothing in it is skipped rather than sent as zeroes. Being on a hate list is not
		// a contribution, and a screen full of 0 dps rows buries the ones that matter.
		if (r.damage == 0 && r.healing == 0 && r.taken == 0) { continue; }
		// ⚠️ Fields are APPENDED, never reordered or inserted. An older dll splits on '|' into the first
		// four and ignores the rest, so it keeps working with reduced columns instead of misreading
		// every number -- the same reason the difficulty window rides its affixes inside a field.
		line += fmt::format("^{}|{}|{}|{}|{}|{}|{}|{}", r.name, r.damage, r.healing, r.taken,
		                    r.attempts, r.avoided, r.best_sec, r.overheal);
	}

	// ⚠️⚠️ Client::Message IS PRINTF-STYLE, so the payload goes through a "%s" argument -- a character
	// name cannot contain '%', but nothing here should depend on that. And Chat::White, NOT
	// Chat::NonMelee: the latter sits behind the player's FilterSpellDamage setting and a transport the
	// player can silently filter off is not a transport (the floating text feed learned this the hard
	// way).
	Message(Chat::White, "%s", line.c_str());
}

// How many finished fights each player keeps.
// ⚠️ Small on purpose. Every entry holds a full row list AND every row's breakdown, and the whole
// thing is per client and per zone process; ten fights of a raid is already a few thousand strings.
static const size_t AOTV4_METER_HISTORY = 10;

void Client::AoTv4MeterArchive(const AoTv4Encounter &enc)
{
	// ⚠️ A fight nobody contributed damage to is not a fight. Walking into a creature's aggro range and
	// walking out again would otherwise fill the history with empty entries and push out real ones.
	if (enc.TotalDamage() <= 0) { return; }

	m_aotv4_meter_hist.insert(m_aotv4_meter_hist.begin(), enc);
	if (m_aotv4_meter_hist.size() > AOTV4_METER_HISTORY) {
		m_aotv4_meter_hist.resize(AOTV4_METER_HISTORY);
	}

	// ⚠️⚠️ VIEWING INDEX SHIFTS WITH THE INSERT. A player reading fight 3 when a new one is archived is
	// still looking at the same fight, which is now index 4. Without this the window silently jumps to a
	// different fight mid-read -- and during a chain pull that is every few seconds.
	if (m_aotv4_meter_view >= 0) {
		++m_aotv4_meter_view;
		if (m_aotv4_meter_view >= (int)m_aotv4_meter_hist.size()) {
			m_aotv4_meter_view = (int)m_aotv4_meter_hist.size() - 1;
		}
	}
}

// The fight picker: METERLIST ^index|target|seconds|total^...

// Which encounter is this player looking at, live or stored.
// ⚠️⚠️ ONE HELPER BECAUSE THE PUSH AND THE DRILL-DOWN MUST NEVER DISAGREE. They resolved it separately
// and both bailed when nothing was alive -- so once the creature died, clicking a row asked about an
// encounter the server would not answer for, and the breakdown opened empty. Reported from play as
// there being no breakdown at all, because you click a row AFTER the kill, not during it.
// 📌 The fallback to the newest archived fight is also what a meter should do when nothing is being
// fought: show the last one rather than going blank.
static const AoTv4Encounter *AoTv4MeterCurrent(Client *c, AoTv4Encounter &scratch, std::string &name_out)
{
	if (c->m_aotv4_meter_view >= 0 && c->m_aotv4_meter_view < (int)c->m_aotv4_meter_hist.size()) {
		const AoTv4Encounter &e = c->m_aotv4_meter_hist[c->m_aotv4_meter_view];
		name_out = e.TargetName();
		return &e;
	}

	int alive = 0;
	for (uint16 id : c->m_aotv4_meter_engaged) {
		Mob *npc = entity_list.GetMobID(id);
		// ⚠️ A dead creature's id simply stops resolving, which is how the engaged list prunes itself.
		if (!npc || !npc->m_aotv4_meter.Active()) { continue; }
		npc->m_aotv4_meter.MergeInto(scratch);
		if (alive == 0) { name_out = npc->GetCleanName(); }
		++alive;
	}
	if (alive > 0) {
		// ⚠️ Naming a pull after ONE of its creatures would be a lie about what the numbers cover. The
		// count is honest and is also the signal that this is a pull rather than a single fight.
		if (alive > 1) { name_out = fmt::format("{} creatures", alive); }
		return &scratch;
	}

	if (!c->m_aotv4_meter_hist.empty()) {
		const AoTv4Encounter &e = c->m_aotv4_meter_hist[0];
		name_out = e.TargetName();
		return &e;
	}
	return nullptr;
}

void Client::AoTv4MeterSendList()
{
	if (!m_aotv4_meter_on) { return; }

	std::string line = "METERLIST";
	for (size_t i = 0; i < m_aotv4_meter_hist.size(); ++i) {
		const AoTv4Encounter &e = m_aotv4_meter_hist[i];
		line += fmt::format("^{}|{}|{}|{}", i, e.TargetName(), e.Seconds(), e.TotalDamage());
	}
	Message(Chat::White, "%s", line.c_str());
}

// The drill-down: METERDETAIL <who>^label|damage|healing^...
// ⚠️ Read from whichever fight is being VIEWED, not always the live one -- the whole point is being able
// to ask what a past fight was made of.
void Client::AoTv4MeterSendDetail(const char *who)
{
	if (!m_aotv4_meter_on || !who) { return; }

	AoTv4Encounter scratch;
	std::string    name;
	const AoTv4Encounter *enc = AoTv4MeterCurrent(this, scratch, name);

	// ⚠️⚠️ ALWAYS REPLY, even with nothing to say. Returning silently leaves the window sitting in
	// breakdown mode with whatever it had, which is indistinguishable from a breakdown that is genuinely
	// empty -- and that ambiguity is what made this take two rounds to find.
	if (!enc) {
		Message(Chat::White, "%s", fmt::format("METERDETAIL {}", who).c_str());
		return;
	}

	std::string line = fmt::format("METERDETAIL {}", who);
	for (const auto &r : enc->Rows()) {
		if (strcasecmp(r.name.c_str(), who) != 0) { continue; }
		for (const auto &p : r.parts) {
			if (p.damage == 0 && p.healing == 0) { continue; }
			line += fmt::format("^{}|{}|{}|{}|{}", p.label, p.damage, p.healing, p.hits, p.max_hit);
		}
		break;
	}
	Message(Chat::White, "%s", line.c_str());
}

void AoTv4Encounter::AdoptStart(time_t t)
{
	if (t != 0 && (m_start == 0 || t < m_start)) { m_start = t; }
}

void AoTv4Encounter::MergeInto(AoTv4Encounter &out) const
{
	out.AdoptStart(m_start);

	for (const auto &r : m_rows) {
		AoTv4MeterRow *dst = out.Row(r.char_id, r.name.c_str());
		// ⚠️ Row() refuses past its cap and returns null. Skipping is correct -- the alternative is
		// writing through a null pointer to save a row nobody can see anyway.
		if (!dst) { continue; }

		dst->damage  += r.damage;
		dst->healing += r.healing;
		dst->taken   += r.taken;

		for (const auto &p : r.parts) {
			if (auto *dp = dst->Part(p.label)) {
				dp->damage  += p.damage;
				dp->healing += p.healing;
			}
		}
	}
}

// ⚠️ Whole seconds, not a sliding window. A sliding window needs a ring of samples per row and a walk
// per event; a bucket that rolls over on the second costs two comparisons and answers the same
// question closely enough to sort a meter by.
void AoTv4MeterRow::NoteSecond(int64 amount)
{
	const time_t now = time(nullptr);
	if (now != cur_sec_at) {
		if (cur_sec > best_sec) { best_sec = cur_sec; }
		cur_sec    = 0;
		cur_sec_at = now;
	}
	cur_sec += amount;
	// ⚠️ Compared on every event, not only at rollover. A fight that ends mid-second would otherwise
	// throw away its best second -- and the killing blow is usually in it.
	if (cur_sec > best_sec) { best_sec = cur_sec; }
}
