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
#pragma once

#include "common/types.h"

#include <list>
#include <string>

struct LootItem {
	uint32      item_id;
	int16       equip_slot;
	uint16      charges;
	uint16      lootslot;
	// AoTv4 INDIVIDUAL LOOT: which character this drop belongs to. 0 means SHARED -- anyone with kill
	// credit may take it, which is what quest-placed drops (npc:AddItem during the NPC's life) and any
	// pre-existing loot get. Anything rolled per player at death carries that player's character id.
	// ⚠️ It rides on the ITEM, not on the lootslot, deliberately: Corpse::AssignLootSlots numbers the
	// items and a slot-keyed side table would desync the moment anything renumbered.
	// ⚠️ Runtime only -- never written to character_corpse_items. NPC corpses are not persisted, and a
	// player corpse has no use for it.
	uint32      owner_char_id{0};
	// AoTv4 ADVANCED LOOT "Leave": the character who declined this item. Hides the row from THEIR
	// personal list only -- the item stays on the corpse and is still lootable from the stock window,
	// so nothing is destroyed and there is a way back if they change their mind.
	// ⚠️ One field, so a SHARED item (owner 0, seen by several players) remembers only the most recent
	// decline. That is the safe direction: it can forget a decline, but it can never hide a row from
	// somebody who did not decline it. Shared items are rare under individual loot (quest-placed and
	// global loot only), so this is not worth a per-item list.
	// ⚠️ Runtime only -- never written to character_corpse_items, like owner_char_id above.
	uint32      declined_by_char_id{0};
	uint32      aug_1;
	uint32      aug_2;
	uint32      aug_3;
	uint32      aug_4;
	uint32      aug_5;
	uint32      aug_6;
	bool        attuned;
	std::string custom_data;
	uint32      ornamenticon{};
	uint32      ornamentidfile{};
	uint32      ornament_hero_model{};
	uint16      trivial_min_level;
	uint16      trivial_max_level;
	uint16      npc_min_level;
	uint16      npc_max_level;
	uint32      lootdrop_id; // required for zone state referencing
};

using LootItems = std::list<LootItem*>;
