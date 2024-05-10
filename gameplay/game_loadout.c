/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2023-2024 starfrost

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// game_loadout.c: Implements game loadout functionality

#include <game_local.h>

loadout_entry_t* Loadout_AddItem(edict_t *ent, const char* name, const char* icon, loadout_entry_type type, int32_t amount)
{
	// don't crash if we're not connected
	strncpy(ent->client->loadout.items[ent->client->loadout.num_items].item_name, name, LOADOUT_MAX_STRLEN);
	strncpy(ent->client->loadout.items[ent->client->loadout.num_items].icon, icon, MAX_QPATH);
	ent->client->loadout.items[ent->client->loadout.num_items].type = type;
	ent->client->loadout.items[ent->client->loadout.num_items].amount = amount;

	ent->client->loadout.num_items++;

	if (ent->client->pers.connected)
	{
		gi.WriteByte(svc_loadout_add);
		gi.WriteString(name);
		gi.WriteString(icon);
		gi.WriteByte(type);
		gi.WriteShort(amount);

		// reliable for now
		gi.unicast(ent, true);
	}

	return &ent->client->loadout.items[ent->client->loadout.num_items - 1];
}

void Loadout_DeleteItem(edict_t* ent, const char* name)
{
	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, name);

	if (loadout_entry_ptr == NULL)
	{
		gi.error("Tried to delete invalid loadout entry (name: %s)!", name);
		return;
	}

	memset(loadout_entry_ptr->item_name, 0x00, strlen(loadout_entry_ptr->item_name));
	loadout_entry_ptr->amount = 0;

	// only decrement num_items if its the last item (empty items get skipped)
	if (loadout_entry_ptr == &ent->client->loadout.items[ent->client->loadout.num_items - 1])
		ent->client->loadout.num_items--;

	// tell the client
	gi.WriteByte(svc_loadout_remove);
	gi.WriteString(name);
	
	gi.unicast(ent, true);
}

loadout_entry_t* Loadout_GetItem(edict_t* ent, const char* name)
{
	for (int32_t item_id = 0; item_id < ent->client->loadout.num_items; item_id++)
	{
		if (!strncmp(ent->client->loadout.items[item_id].item_name, name, LOADOUT_MAX_STRLEN))
		{
			return &ent->client->loadout.items[item_id];
		}
	}

	return NULL; 
}

bool Loadout_EntryIsItem(loadout_entry_t* entry, const char* name)
{
	return !strncmp(entry->item_name, name, LOADOUT_MAX_STRLEN);
}