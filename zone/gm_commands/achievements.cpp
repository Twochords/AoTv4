/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "zone/achievement_manager.h"
#include "zone/client.h"

void command_ach(Client *c, const Seperator *sep)
{
	achievement_manager.HandleCommand(c, sep);
}
