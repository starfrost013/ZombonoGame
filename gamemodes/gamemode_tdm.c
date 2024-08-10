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

#define TIME_BUF_LENGTH		20
#define SCORE_BUF_LENGTH	40

void Gamemode_TDMCheckRules()
{
	int32_t		i;
	gclient_t* cl;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value)
		{
			gi.bprintf(PRINT_HIGH, "Out of time!\n");
			Game_EndMatch();
			return;
		}

		// update every 1 second roughly
		if (level.framenum % (int32_t)(1.0f / FRAMETIME) == 0)
		{
			char text[TIME_BUF_LENGTH] = { 0 };

			int32_t total_seconds = timelimit->value - (int32_t)level.time;
			// convert remaining time to integer mm:ss
			int32_t minutes = (int32_t)(total_seconds / 60);
			int32_t seconds = (int32_t)(total_seconds % 60);

			if (minutes < 10)
			{
				if (seconds < 10)
				{
					snprintf(text, TIME_BUF_LENGTH, "0%d:0%d", minutes, seconds);
				}
				else
				{
					snprintf(text, TIME_BUF_LENGTH, "0%d:%d", minutes, seconds);
				}
			}
			else
			{
				if (seconds < 10)
				{
					snprintf(text, TIME_BUF_LENGTH, "%d:0%d", minutes, seconds);
				}
				else
				{
					snprintf(text, TIME_BUF_LENGTH, "%d:%d", minutes, seconds);
				}
			}

			// multicast time remaining to each client
			GameUI_SetText(NULL, "TimeUI", "TimeUI_Text", text, false);
		}

	}

	// every second update each team's score
	if (level.framenum % (int32_t)(1.0f / FRAMETIME) == 0)
	{
		team_scores_t team_scores = Gamemode_TDMGetScores();
		char text[SCORE_BUF_LENGTH] = { 0 };

		snprintf(text, SCORE_BUF_LENGTH, "Directors %d : Players %d", team_scores.director_score, team_scores.player_score);

		GameUI_SetText(NULL, "ScoreUI", "ScoreUI_Text", text, false);
	}

	if (fraglimit->value)
	{
		for (i = 0; i < sv_maxclients->value; i++)
		{
			if ((int32_t)gameflags->value & GF_INDIVIDUAL_FRAGLIMIT)
			{
				cl = game.clients + i;
				if (!g_edicts[i + 1].inuse)
					continue;

				if (cl->resp.score >= fraglimit->value)
				{
					gi.bprintf(PRINT_HIGH, "Fraglimit hit!\n");
					Game_EndMatch();
					return;
				}
			}
			else // if individual fraglimit is off, use the aggregate scores of either team instead
			{
				team_scores_t team_scores = Gamemode_TDMGetScores();

				if (team_scores.director_score >= fraglimit->value
					|| team_scores.player_score >= fraglimit->value)
				{
					gi.bprintf(PRINT_HIGH, "Fraglimit hit!\n");
					Game_EndMatch();
					return;
				}
			}

		}
	}
}

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
edict_t* Gamemode_TDMSpawnPlayer(edict_t* player)
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