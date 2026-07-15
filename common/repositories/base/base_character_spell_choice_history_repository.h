/**
 * DO NOT MODIFY THIS FILE
 *
 * This repository was automatically generated and is NOT to be modified directly.
 * Any repository modifications are meant to be made to the repository extending the base.
 * Any modifications to base repositories are to be made by the generator only
 *
 * @generator ./utils/scripts/generators/repository-generator.pl
 * @docs https://docs.eqemu.dev/developer/repositories
 */

#pragma once

#include "common/database.h"
#include "common/strings.h"

{{ADDITIONAL_INCLUDES}}
#include <ctime>

class BaseCharacterSpellChoiceHistoryRepository {
public:
	struct CharacterSpellChoiceHistory {
		uint64_t    id;
		uint32_t    character_id;
		uint64_t    offer_id;
		uint16_t    generator_version;
		uint32_t    selected_spell_id;
		std::string choices;
		time_t      created_at;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"id",
			"character_id",
			"offer_id",
			"generator_version",
			"selected_spell_id",
			"choices",
			"created_at",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"character_id",
			"offer_id",
			"generator_version",
			"selected_spell_id",
			"choices",
			"UNIX_TIMESTAMP(created_at)",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(Strings::Implode(", ", Columns()));
	}

	static std::string SelectColumnsRaw()
	{
		return std::string(Strings::Implode(", ", SelectColumns()));
	}

	static std::string TableName()
	{
		return std::string("character_spell_choice_history");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			SelectColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static CharacterSpellChoiceHistory NewEntity()
	{
		CharacterSpellChoiceHistory e{};

		e.id                = 0;
		e.character_id      = 0;
		e.offer_id          = 0;
		e.generator_version = 0;
		e.selected_spell_id = 0;
		e.choices           = "";
		e.created_at        = std::time(nullptr);

		return e;
	}

	static CharacterSpellChoiceHistory GetCharacterSpellChoiceHistory(
		const std::vector<CharacterSpellChoiceHistory> &character_spell_choice_historys,
		int character_spell_choice_history_id
	)
	{
		for (auto &character_spell_choice_history : character_spell_choice_historys) {
			if (character_spell_choice_history.id == character_spell_choice_history_id) {
				return character_spell_choice_history;
			}
		}

		return NewEntity();
	}

	static CharacterSpellChoiceHistory FindOne(
		Database& db,
		int character_spell_choice_history_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				character_spell_choice_history_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			CharacterSpellChoiceHistory e{};

			e.id                = row[0] ? strtoull(row[0], nullptr, 10) : 0;
			e.character_id      = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.offer_id          = row[2] ? strtoull(row[2], nullptr, 10) : 0;
			e.generator_version = row[3] ? static_cast<uint16_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.selected_spell_id = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.choices           = row[5] ? row[5] : "";
			e.created_at        = strtoll(row[6] ? row[6] : "-1", nullptr, 10);

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int character_spell_choice_history_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				character_spell_choice_history_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const CharacterSpellChoiceHistory &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[1] + " = " + std::to_string(e.character_id));
		v.push_back(columns[2] + " = " + std::to_string(e.offer_id));
		v.push_back(columns[3] + " = " + std::to_string(e.generator_version));
		v.push_back(columns[4] + " = " + std::to_string(e.selected_spell_id));
		v.push_back(columns[5] + " = '" + Strings::Escape(e.choices) + "'");
		v.push_back(columns[6] + " = FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static CharacterSpellChoiceHistory InsertOne(
		Database& db,
		CharacterSpellChoiceHistory e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.character_id));
		v.push_back(std::to_string(e.offer_id));
		v.push_back(std::to_string(e.generator_version));
		v.push_back(std::to_string(e.selected_spell_id));
		v.push_back("'" + Strings::Escape(e.choices) + "'");
		v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<CharacterSpellChoiceHistory> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.character_id));
			v.push_back(std::to_string(e.offer_id));
			v.push_back(std::to_string(e.generator_version));
			v.push_back(std::to_string(e.selected_spell_id));
			v.push_back("'" + Strings::Escape(e.choices) + "'");
			v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static std::vector<CharacterSpellChoiceHistory> All(Database& db)
	{
		std::vector<CharacterSpellChoiceHistory> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			CharacterSpellChoiceHistory e{};

			e.id                = row[0] ? strtoull(row[0], nullptr, 10) : 0;
			e.character_id      = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.offer_id          = row[2] ? strtoull(row[2], nullptr, 10) : 0;
			e.generator_version = row[3] ? static_cast<uint16_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.selected_spell_id = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.choices           = row[5] ? row[5] : "";
			e.created_at        = strtoll(row[6] ? row[6] : "-1", nullptr, 10);

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<CharacterSpellChoiceHistory> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<CharacterSpellChoiceHistory> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			CharacterSpellChoiceHistory e{};

			e.id                = row[0] ? strtoull(row[0], nullptr, 10) : 0;
			e.character_id      = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.offer_id          = row[2] ? strtoull(row[2], nullptr, 10) : 0;
			e.generator_version = row[3] ? static_cast<uint16_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.selected_spell_id = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.choices           = row[5] ? row[5] : "";
			e.created_at        = strtoll(row[6] ? row[6] : "-1", nullptr, 10);

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static int DeleteWhere(Database& db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {}",
				TableName(),
				where_filter
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int Truncate(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"TRUNCATE TABLE {}",
				TableName()
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int64 GetMaxId(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COALESCE(MAX({}), 0) FROM {}",
				PrimaryKey(),
				TableName()
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static int64 Count(Database& db, const std::string &where_filter = "")
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COUNT(*) FROM {} {}",
				TableName(),
				(where_filter.empty() ? "" : "WHERE " + where_filter)
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static std::string BaseReplace()
	{
		return fmt::format(
			"REPLACE INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static int ReplaceOne(
		Database& db,
		const CharacterSpellChoiceHistory &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.character_id));
		v.push_back(std::to_string(e.offer_id));
		v.push_back(std::to_string(e.generator_version));
		v.push_back(std::to_string(e.selected_spell_id));
		v.push_back("'" + Strings::Escape(e.choices) + "'");
		v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseReplace(),
				Strings::Implode(",", v)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int ReplaceMany(
		Database& db,
		const std::vector<CharacterSpellChoiceHistory> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.character_id));
			v.push_back(std::to_string(e.offer_id));
			v.push_back(std::to_string(e.generator_version));
			v.push_back(std::to_string(e.selected_spell_id));
			v.push_back("'" + Strings::Escape(e.choices) + "'");
			v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseReplace(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}
};
