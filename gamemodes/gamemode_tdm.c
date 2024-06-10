/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak
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

// gamemode_tdm.c: The TDM gamemode.
// June 10, 2024

#include "game_local.h"

// Game_TDMGetScores: Gets the current scores for both teams in TDM mode.
team_scores_t Gamemode_TDMGetScores()
{
	edict_t* client_edict;

	team_scores_t team_scores = { 0 };

	for (int32_t client_num = 0; client_num < game.maxclients; client_num++)
	{
		client_edict = g_edicts + client_num + 1;

		if (client_edict->inuse)
		{
			// add up the scores
			switch (client_edict->team)
			{
			case team_player:
				team_scores.player_score += client_edict->client->resp.score;
				break;
			case team_director:
				team_scores.director_score += client_edict->client->resp.score;
				break;
			}
		}
	}

	return team_scores;
}

//
// Game_TDMGetWinner : Gets the winner in TDM gamemode.
// 
// Returns BOTH teams if there is a draw.
//
player_team Gamemode_TDMGetWinner()
{
	team_scores_t team_scores = Gamemode_TDMGetScores();

	if (team_scores.player_score > team_scores.director_score)
	{
		return team_player;
	}
	else if (team_scores.player_score < team_scores.director_score)
	{
		return team_director;
	}
	else
	{
		return team_director | team_player; // this indicates a draw
	}
}

// figure out if the other modes use this
edict_t* SelectTeamSpawnPoint(edict_t* player)
{
	if ((int32_t)gamemode->value != 0)
	{
		gi.bprintf(PRINT_ALL, "Can't currently spawn for non-TDM gamemodes");

		return SelectUnassignedSpawnPoint();
	}
	else
	{
		if (player->team == 0)
		{
			gi.bprintf(PRINT_ALL, "Error - Player Teamflag not set! Defaulting to info_player_start");
			return SelectUnassignedSpawnPoint();
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
			return SelectUnassignedSpawnPoint();
		}

		if ((int32_t)(gameflags->value) & GF_SPAWN_FARTHEST)
		{
			return SelectFarthestSpawnPoint(spawn_class_name);
		}
		else
		{
			return SelectRandomSpawnPoint(spawn_class_name);
		}
	}
}