/*
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

// gamemode_waves.c: The "Waves" gamemode...
// June 10, 2024
#include <game_local.h>

void Gamemode_WavesUpdate()
{

}

void Gamemode_WavesCheckRules()
{

}

// figure out if the other modes use this
edict_t* Gamemode_WavesSpawnPlayer(edict_t* player)
{
	if (player->team == 0)
	{
		gi.bprintf(PRINT_ALL, "Error - Player Teamflag not set! Defaulting to info_player_start");
		return Player_SpawnSelectUnassigned();
	}
	// Teamflags are used to allow items to be used with multiple teams, but not all
	// 
	// Teamflag 1 - Director
	char* spawn_class_name = "info_player_start_director";

	// Teamflag 2 - Player
	if (player->team == team_player)
	{
		spawn_class_name = "info_player_start_player";
	}
	// Teamflag 4 - Unassigned
	else if (player->team == team_unassigned)
	{
		return Player_SpawnSelectUnassigned();
	}

	if ((int32_t)(gameflags->value) & GF_SPAWN_FARTHEST)
	{
		return Player_SpawnSelectFarthest(spawn_class_name);
	}
	else
	{
		return Player_SpawnSelectRandom(spawn_class_name);
	}
}
