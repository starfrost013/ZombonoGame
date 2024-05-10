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

#include <game_local.h>
#include <mobs/mob_player.h>

void SelectNextItem(edict_t* ent, int32_t itflags)
{
	gclient_t* cl;
	gitem_t* it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	cl = ent->client;

	// return if no weapon
	if (!cl->pers.weapon)
		return;

	// new loadout system...
	loadout_entry_t* loadout_entry_ptr = &cl->loadout_current_weapon;

	for (int32_t item_num = 0; item_num < cl->loadout.num_items; item_num++)
	{
		loadout_entry_t* loadout_entry_ptr = &cl->loadout.items[item_num];

		it = FindItem(loadout_entry_ptr->item_name);

		if (it == NULL)
			continue;

		if (!it->use)
			continue;

		if (!(it->flags & itflags))
			continue;

		// cycle if its the first item
		if (cl->pers.selected_item == it)
		{
			int32_t new_item_num = item_num + 1;
			if (new_item_num > cl->loadout.num_items) new_item_num = 0;

			// get the current weapon
			cl->pers.selected_item = &cl->loadout.items[new_item_num];
			return;	// successful
		}
	}
}

void SelectPrevItem (edict_t *ent, int32_t itflags)
{
	gclient_t* cl;
	gitem_t* it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	cl = ent->client;

	// return if no weapon
	if (!cl->pers.weapon)
		return;

	// new loadout system...
	loadout_entry_t* loadout_entry_ptr = &cl->loadout_current_weapon;

	for (int32_t item_num = 0; item_num < cl->loadout.num_items; item_num++)
	{
		loadout_entry_t* loadout_entry_ptr = &cl->loadout.items[item_num];

		it = FindItem(loadout_entry_ptr->item_name);

		if (it == NULL)
			continue;

		if (!it->use)
			continue;

		if (!(it->flags & itflags))
			continue;

		// cycle if its the first item
		if (cl->pers.selected_item == it)
		{
			int32_t new_item_num = item_num - 1;
			if (new_item_num < 0) new_item_num = cl->loadout.num_items;

			// get the current weapon
			cl->pers.selected_item = &cl->loadout.items[new_item_num]; 
			return;	// successful
		}
	}
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	bool	give_all;
	edict_t		*it_ent;

	if (!sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			
			Loadout_AddItem(ent, it->pickup_name, it->icon, loadout_entry_type_weapon, 1);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t*		info;

		Loadout_DeleteItem(ent, "Jacket Armor");
		Loadout_DeleteItem(ent, "Combat Armor");
		
		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;

		// see if you already have it
		loadout_entry_t* body_armor_ptr = Loadout_GetItem(ent, it->pickup_name);

		// if its not there add it, otherwise set it to max_count
		if (!body_armor_ptr)
		{
			body_armor_ptr = Loadout_AddItem(ent, it->pickup_name, it->icon, loadout_entry_type_armor, info->max_count);
		}
		else
		{
			body_armor_ptr->amount = info->max_count;
		}

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;

			Loadout_AddItem(ent, it->pickup_name, it->icon, loadout_entry_type_armor, 1);
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, it->pickup_name);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			loadout_entry_ptr->amount = atoi(gi.argv(2));
		else
			loadout_entry_ptr->amount += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (!sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (!sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (!sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	char*	 s = gi.args();
	gitem_t* it = FindItem(s);

	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, it->pickup_name);

	if (loadout_entry_ptr == NULL
		|| !loadout_entry_ptr->amount)
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}

#define MAX_UI_STRLEN_GAME 256	// CHANGE!!!!
void Cmd_Loadout_f(edict_t* ent)
{
	// don't bother if there are no items
	if (ent->client->loadout.num_items == 0)
		return; 

	char* index_str = gi.argv(1);
	char ui_name[MAX_UI_STRLEN_GAME] = { 0 };

	// set the current loadout
	if (index_str)
	{
		// find the index of the current item
		// we don't just use the index as part of the loadout so it can't desync
		int32_t original_index = 0;

		for (int32_t loadout_index = 0; loadout_index < ent->client->loadout.num_items; loadout_index++)
		{
			if (&ent->client->loadout.items[loadout_index] == ent->client->loadout_current_weapon)
			{
				original_index = loadout_index;
				break; // we don't need to continue since we already have the current index
			}
		}

		int32_t index = 0;

		if (!strcmp(index_str, "next")) // cycle to next
		{
			index = original_index + 1;

			// make sure we have actually selected a weapon and not armour or somethnig else

			if (ent->client->loadout.items[index].type != loadout_entry_type_weapon)
			{
				while (ent->client->loadout.items[index].type != loadout_entry_type_weapon
					&& index < ent->client->loadout.num_items)
				{
					index++;
				}

			}

			if (index >= ent->client->loadout.num_items) // "rollover" behaviour so we can nicely scroll through the list
				index = 0;

		}
		else if (!strcmp(index_str, "prev")) // cycle to prev
		{
			index = original_index - 1;

			if (index < 0) // "rollover" behaviour so we can nicely scroll through the list
				index = ent->client->loadout.num_items - 1;

			// make sure we have actually selected a weapon and not armour or somethnig else
			if (ent->client->loadout.items[index].type != loadout_entry_type_weapon)
			{
				while (ent->client->loadout.items[index].type != loadout_entry_type_weapon
					&& index >= 0)
				{
					index--;
				}

			}
		}
		else // next part
		{
			index = atoi(index_str);

			if (ent->client->loadout.items[index].type != loadout_entry_type_weapon)
			{
				while (ent->client->loadout.items[index].type != loadout_entry_type_weapon
					&& index < ent->client->loadout.num_items)
				{
					index++;
				}
			}

			//don't rollover if the user pressed an invalid key, just return...
			if (index >= ent->client->loadout.num_items)
				return; 
		}

		// temp, clamp to available item range to absolutely MAKE SURE nothing invalid can be seleted
		if (index < 0) index = 0;
		if (index > 9) index = 9;

		// now clamp to available items
		if (index >= ent->client->loadout.num_items) index = ent->client->loadout.num_items - 1;

		gi.WriteByte(svc_loadout_setcurrent);
		gi.WriteByte(index);
		gi.unicast(ent, true); // rare

		// tell the server to give this client a new weapon if we aren't already selecting a new weapon
		// we don't need to check if it already exists because we did it already
		if (&ent->client->loadout.items[index] != ent->client->loadout_current_weapon)
		{
			ent->client->loadout_current_weapon = &ent->client->loadout.items[index];
			ent->client->newweapon = FindItem(&ent->client->loadout.items[index].item_name);
			ChangeWeapon(ent);
		}
	}


	// tell the client to turn on the loadout UI
	G_UISend(ent, "LoadoutUI", true, true, false);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	gitem_t		*it;
	char		*s;

	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, it->pickup_name);

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	if (loadout_entry_ptr->amount == 0)
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	// new loadout system...
	loadout_entry_t* loadout_entry_ptr = &cl->loadout_current_weapon;

	for (int32_t item_num = 0; item_num < cl->loadout.num_items; item_num++)
	{
		loadout_entry_t* loadout_entry_ptr = &cl->loadout.items[item_num];

		it = FindItem(loadout_entry_ptr->item_name);

		if (it == NULL)
			continue;

		if (!it->use)
			continue;

		if (!(it->flags & IT_WEAPON))
			continue;

		// cycle if its the first item
		if (cl->pers.weapon == it)
		{
			int32_t new_item_num = item_num - 1;
			if (new_item_num < 0) new_item_num = cl->loadout.num_items - 1;

			// get the current weapon
			cl->loadout_current_weapon = &cl->loadout.items[new_item_num];

			return;	// successful
		}
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t* cl;
	gitem_t* it;

	cl = ent->client;

	// return if no weapon
	if (!cl->pers.weapon)
		return;

	// new loadout system...
	loadout_entry_t* loadout_entry_ptr = &cl->loadout_current_weapon;

	for (int32_t item_num = 0; item_num < cl->loadout.num_items; item_num++)
	{
		loadout_entry_t* loadout_entry_ptr = &cl->loadout.items[item_num];

		it = FindItem(loadout_entry_ptr->item_name);

		if (it == NULL)
			continue;

		if (!it->use)
			continue;

		if (!(it->flags & IT_WEAPON))
			continue;

		// cycle if its the first item
		if (cl->pers.weapon == it)
		{
			int32_t new_item_num = item_num + 1;
			if (new_item_num > cl->loadout.num_items) 0;

			// get the current weapon
			cl->loadout_current_weapon = &cl->loadout.items[new_item_num];

			return;	// successful
		}
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	loadout_entry_t* loadout_entry_ptr = Loadout_GetItem(ent, cl->pers.lastweapon->pickup_name);

	it = FindItem(cl->pers.lastweapon->pickup_name);
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showhelp = false;
}


int32_t PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int32_t *)a;
	bnum = *(int32_t *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_SetTeam_f
(noconsole)
==================
*/
void Cmd_SetTeam_f(edict_t* ent, player_team team)
{
	// anti cheating:
	// on TDM mode, unassigneds are invincible (for spectating, and also for )
	// on non-teamed modes, *everyone* is team unassigned (as opposed to having no team), so we gate this in a teammode check
	if (gamemode->value == GAMEMODE_TDM)
	{
		if (team == team_unassigned)
		{
			return; 
		}
	}

	// reject no team or multiple teams
	if ((team <= 0)
		|| (team == (team_player | team_director))
		|| (team > team_max))
	{
		return;
	}

	vec3_t spawn_origin, spawn_angles;

	// If we are switching to player, disable the Director-only UI
	if (team == team_player)
	{
		// TDM mode only?
		G_UISend(ent, "BamfuslicatorUI", false, false, false);
	}

	ent->team = team;

	// teleport the player to the spanw point
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);
	GiveBaseWeaponForTeam(ent);
	ent->client->newweapon = ent->client->pers.weapon;
	ChangeWeapon(ent);

	VectorCopy(spawn_origin, ent->s.origin);
	VectorCopy(spawn_angles, ent->s.angles);
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, bool team, bool arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int32_t)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int32_t)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (ent->team != other->team);
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int32_t i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}


/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	bool valid_command = false;

	if (!Q_stricmp (cmd, "players"))
	{
		Cmd_Players_f (ent);
		return;
	}
	else if (!Q_stricmp (cmd, "say"))
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	else if (!Q_stricmp (cmd, "say_team"))
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	else if (!Q_stricmp(cmd, "leaderboard"))
	{
		Cmd_Leaderboard_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	// commands below here can not be used during intermission
	if (!Q_stricmp(cmd, "use"))
	{
		Cmd_Use_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "drop"))
	{
		Cmd_Drop_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "give"))
	{
		Cmd_Give_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "god"))
	{
		Cmd_God_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "notarget"))
	{
		Cmd_Notarget_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "noclip"))
	{
		Cmd_Noclip_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "invnext"))
	{
		SelectNextItem(ent, -1);
		return;
	}
	else if (!Q_stricmp(cmd, "invprev"))
	{
		SelectPrevItem(ent, -1);
		return;
	}
	else if (!Q_stricmp(cmd, "invnextw"))
	{
		SelectNextItem(ent, IT_WEAPON);
		return;
	}
	else if (!Q_stricmp(cmd, "invprevw"))
	{
		SelectPrevItem(ent, IT_WEAPON);
		return;
	}
	else if (!Q_stricmp(cmd, "invnextp"))
	{
		SelectNextItem(ent, IT_POWERUP);
		return;
	}
	else if (!Q_stricmp(cmd, "invprevp"))
	{
		SelectPrevItem(ent, IT_POWERUP);
		return;
	}
	else if (!Q_stricmp(cmd, "invuse"))
	{
		Cmd_InvUse_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "invdrop"))
	{
		Cmd_InvDrop_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "weapprev"))
	{
		Cmd_WeapPrev_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "weapnext"))
	{
		Cmd_WeapNext_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "weaplast"))
	{
		Cmd_WeapLast_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "kill"))
	{
		Cmd_Kill_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "putaway"))
	{
		Cmd_PutAway_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "wave"))
	{
		Cmd_Wave_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "playerlist"))
	{
		Cmd_PlayerList_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "loadout"))
	{
		Cmd_Loadout_f(ent);
		return;
	}

	// anything that doesn't match a command will be a chat
	Cmd_Say_f (ent, false, true);
}

// ClientCommands that cannot be entered from teh console.
void ClientCommand_NoConsole(edict_t* ent)
{
	char* cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (!Q_stricmp(cmd, "setteam"))
	{
		Cmd_SetTeam_f(ent, atoi(gi.argv(1)));
	}
}