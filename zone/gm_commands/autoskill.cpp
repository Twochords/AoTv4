#include "../client.h"


#include <algorithm>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

/*
Default autoskill list comes from Client::GetAvailableAutoSkills().

Common skill IDs:
8  Backstab
10 Bash
21 Dragon Punch
23 Eagle Strike
26 Flying Kick
30 Kick
38 Round Kick
52 Tiger Claw
73 Taunt
74 Frenzy
*/

enum CommandMode { Status, Enable, Disable };

std::tuple<std::map<std::string, int>, std::map<std::string, int>, std::map<int, std::string>> initialize_skill_maps(Client *c);
void display_skill_list(Client *c, const std::map<int, std::string> &id_to_name_map);
std::pair<CommandMode, bool> parse_command_mode(const std::string &arg);
std::string extract_skill_name(const Seperator *sep, bool has_command_param);
int find_skill_id(
	const std::string &skill_name_input,
	const std::map<std::string, int> &skill_lookup_map,
	const std::map<int, std::string> &id_to_name_map
);
void process_command(Client *c, CommandMode cmd_mode, int skill_id, const std::map<int, std::string> &id_to_name_map);

void command_autoskill(Client *c, const Seperator *sep)
{
	if (!c || !sep) {
		return;
	}

	std::string usage = "Usage: #autoskill [skill id or name] [enable/disable/status] or #autoskill list";

	if (sep->argnum < 1) {
		c->Message(Chat::Skills, usage.c_str());
		return;
	}

	static auto skill_maps = initialize_skill_maps(c);
	const auto &skill_lookup_map = std::get<1>(skill_maps);
	const auto &id_to_name_map = std::get<2>(skill_maps);

	if (Strings::ToLower(sep->arg[1]) == "list") {
		display_skill_list(c, id_to_name_map);
		return;
	}

	CommandMode cmd_mode;
	bool has_command_param;
	std::tie(cmd_mode, has_command_param) = parse_command_mode(sep->arg[sep->argnum]);

	if (has_command_param && sep->argnum < 2) {
		c->Message(Chat::Skills, "Autoskill configuration failed. Missing skill name or ID.");
		return;
	}

	std::string skill_name_input = extract_skill_name(sep, has_command_param);
	int skill_id = find_skill_id(skill_name_input, skill_lookup_map, id_to_name_map);

	if (skill_id == -1) {
		c->Message(Chat::Skills, "Autoskill configuration failed. Invalid skill name or ID.");
		c->Message(Chat::Skills, usage.c_str());
		return;
	}

	if (c->HasSkill(static_cast<EQ::skills::SkillType>(skill_id))) {
		process_command(c, cmd_mode, skill_id, id_to_name_map);
	} else {
		c->Message(Chat::Skills, "Autoskill configuration failed. You do not have that skill.");
		c->Message(Chat::Skills, usage.c_str());
	}
}

std::tuple<std::map<std::string, int>, std::map<std::string, int>, std::map<int, std::string>> initialize_skill_maps(Client *c)
{
	static std::map<std::string, int> skill_display_map;
	static std::map<std::string, int> skill_lookup_map;
	static std::map<int, std::string> id_to_name_map;

	if (skill_display_map.empty()) {
		for (const auto &skill_id : c->GetAvailableAutoSkills()) {
			std::string skill_name = EQ::skills::GetSkillName(skill_id);
			int skill_id_int = static_cast<int>(skill_id);

			skill_display_map[skill_name] = skill_id_int;

			std::string lowercase = Strings::ToLower(skill_name);
			skill_lookup_map[lowercase] = skill_id_int;

			std::string no_spaces = lowercase;
			Strings::FindReplace(no_spaces, " ", "");
			if (no_spaces != lowercase) {
				skill_lookup_map[no_spaces] = skill_id_int;
			}

			id_to_name_map[skill_id_int] = skill_name;
		}
	}

	return std::make_tuple(skill_display_map, skill_lookup_map, id_to_name_map);
}

void display_skill_list(Client *c, const std::map<int, std::string> &id_to_name_map)
{
	c->Message(Chat::Skills, "Available skills for auto-skill:");

	std::vector<std::pair<int, std::string>> available_skills;

	for (const auto &pair : id_to_name_map) {
		int skill_id = pair.first;
		if (c->HasSkill(static_cast<EQ::skills::SkillType>(skill_id))) {
			available_skills.push_back(std::make_pair(skill_id, pair.second));
		}
	}

	std::sort(
		available_skills.begin(),
		available_skills.end(),
		[](const std::pair<int, std::string> &a, const std::pair<int, std::string> &b) {
			return a.first < b.first;
		}
	);

	for (const auto &skill : available_skills) {
		bool is_enabled = c->GetAutoSkillStatus(static_cast<EQ::skills::SkillType>(skill.first));

		c->Message(
			Chat::Skills,
			"%s (ID: %d) - Currently %s",
			skill.second.c_str(),
			skill.first,
			is_enabled ? "enabled" : "disabled"
		);
	}

	if (available_skills.empty()) {
		c->Message(Chat::Skills, "You have no skills available for auto-skill.");
	}
}

std::pair<CommandMode, bool> parse_command_mode(const std::string &arg)
{
	CommandMode cmd_mode = Status;
	bool has_command_param = false;

	if (arg.empty()) {
		return std::pair<CommandMode, bool>(cmd_mode, has_command_param);
	}

	std::string arg_lower = Strings::ToLower(arg);

	if (
		arg_lower == "enable" ||
		arg_lower == "on" ||
		arg_lower == "true" ||
		arg_lower == "1" ||
		arg_lower == "yes"
	) {
		has_command_param = true;
		cmd_mode = Enable;
	} else if (
		arg_lower == "disable" ||
		arg_lower == "off" ||
		arg_lower == "false" ||
		arg_lower == "0" ||
		arg_lower == "no" ||
		arg_lower == "disabled"
	) {
		has_command_param = true;
		cmd_mode = Disable;
	} else if (arg_lower == "status") {
		has_command_param = true;
		cmd_mode = Status;
	}

	return std::pair<CommandMode, bool>(cmd_mode, has_command_param);
}

std::string extract_skill_name(const Seperator *sep, bool has_command_param)
{
	std::string skill_name_input;

	if (has_command_param) {
		for (int i = 1; i < sep->argnum; i++) {
			skill_name_input += sep->arg[i];
			if (i < sep->argnum - 1) {
				skill_name_input += " ";
			}
		}
	} else {
		for (int i = 1; i <= sep->argnum; i++) {
			skill_name_input += sep->arg[i];
			if (i < sep->argnum) {
				skill_name_input += " ";
			}
		}
	}

	return skill_name_input;
}

int find_skill_id(
	const std::string &skill_name_input,
	const std::map<std::string, int> &skill_lookup_map,
	const std::map<int, std::string> &id_to_name_map
)
{
	if (Strings::IsNumber(skill_name_input)) {
		int skill_id = Strings::ToInt(skill_name_input);
		if (id_to_name_map.find(skill_id) != id_to_name_map.end()) {
			return skill_id;
		}

		return -1;
	}

	std::string skill_name_lower = Strings::ToLower(skill_name_input);
	auto it = skill_lookup_map.find(skill_name_lower);

	if (it != skill_lookup_map.end()) {
		return it->second;
	}

	return -1;
}

void process_command(Client *c, CommandMode cmd_mode, int skill_id, const std::map<int, std::string> &id_to_name_map)
{
	auto it = id_to_name_map.find(skill_id);
	if (it == id_to_name_map.end()) {
		c->Message(Chat::Skills, "Invalid skill ID: %d", skill_id);
		return;
	}

	const std::string &skill_name = it->second;

	switch (cmd_mode) {
	case Enable:
		c->SetAutoSkillStatus(static_cast<EQ::skills::SkillType>(skill_id), true);
		c->Message(Chat::Skills, "Auto-skill %s (%d) enabled.", skill_name.c_str(), skill_id);
		break;

	case Disable:
		c->SetAutoSkillStatus(static_cast<EQ::skills::SkillType>(skill_id), false);
		c->Message(Chat::Skills, "Auto-skill %s (%d) disabled.", skill_name.c_str(), skill_id);
		break;

	case Status:
	default: {
		bool is_enabled = c->GetAutoSkillStatus(static_cast<EQ::skills::SkillType>(skill_id));
		c->Message(
			Chat::Skills,
			"Auto-skill: %s (ID: %d) is currently %s.",
			skill_name.c_str(),
			skill_id,
			is_enabled ? "enabled" : "disabled"
		);
		break;
	}
	}
}
