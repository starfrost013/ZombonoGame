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
#include <game_local.h>


/*
======================================================================

INTERMISSION

======================================================================
*/

void Player_MoveToIntermission(edict_t* ent, player_team winning_team)
{

	VectorCopy3(level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8;
	VectorCopy3(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// tell the client to display the leaderboard
	// 
	// send the leaderboard info
	GameUI_SendLeaderboard(ent);

	// make it draw the leaderboard (hack, reliable)
	gi.WriteByte(svc_event);
	gi.WriteByte(event_type_sv_leaderboard_draw);
	gi.unicast(ent, true);

	// ...and if they won or lost
	// TODO: MAKE THIS GAMEMODE-SPECIFIC
	if (gamemode->value == GAMEMODE_TDM
		&& winning_team > 0)
	{
		bool player_won = (ent->team == winning_team);

		// check there wasn't a draw

		if (winning_team != (team_director | team_player))
		{
			if (player_won)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("player/player_team_won.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("player/player_team_lost.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("player/player_team_drew.wav"), 1, ATTN_NORM, 0);
		}


		if (winning_team == team_director)
		{
			if (player_won)
			{
				GameUI_SetImage(ent, "LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_win_director", true);
			}
			else
			{
				GameUI_SetImage(ent, "LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_lose_director", true);
			}
		}
		else if (winning_team == team_player)
		{
			if (player_won)
			{
				GameUI_SetImage(ent, "LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_win_player", true);

				
			}
			else
			{
				GameUI_SetImage(ent, "LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_win_player", true);
			}
		}
		else
		{
			GameUI_SetImage(ent, "LeaderboardUI", "LeaderboardUI_Header", "2d/ui/leaderboardui_draw", true);
		}
	}

}

// Run when the gamemode exit conditions are satisfied
void Game_TransitionToNextMatch (edict_t *targ)
{
	int32_t	i, n;
	edict_t	*ent, *client;
	player_team winning_team;

	if (level.intermissiontime)
		return;		// already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i=0 ; i<sv_maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			Client_Respawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	level.exitintermission = 0;

	// find an intermission spot
	ent = Game_FindEdictByValue (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point, so use the unassigned MP start...
		ent = Game_FindEdictByValue (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = Game_FindEdictByValue (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = Game_FindEdictByValue (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = Game_FindEdictByValue (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy3 (ent->s.origin, level.intermission_origin);
	VectorCopy3 (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i = 0; i < sv_maxclients->value; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;

		if (gamemode->value == GAMEMODE_TDM)
		{
			winning_team = Gamemode_TDMGetWinner();
			Player_MoveToIntermission(client, winning_team);
		}
		else // indicate no winner as non-TDM gamemode
		{
			Player_MoveToIntermission(client, 0);
		}
	}
}

/*
==================
G_LeaderboardSend

Sends over the leaderboard
==================
*/
void GameUI_SendLeaderboard(edict_t* ent)
{
	// don't send leaderboard during intermission (there is no reason to do so as the game has stopped)
	if (level.intermissiontime)
	{
		return;
	}

	edict_t* client_edict;
	// tell the client there is a leaderboard update coming
	gi.WriteByte(svc_event);
	gi.WriteByte(event_type_sv_leaderboard_update);

	int32_t client_count = Game_CountClients();
	int32_t total = 0, score = 0;
	int32_t j, k;

	edict_t* sorted_client_edicts[MAX_CLIENTS] = { 0 }; 

	// if someone leaves before someone who joins earlier the edicts may not be perfectly sorted. so we don't use this to iterate.

	gi.WriteByte(client_count);

	// 	// loop through them again to actually send each client's data
	// TODO: SORT SCORES FOR EACH TEAM!!!

	for (int32_t i=0 ; i<game.maxclients ; i++)
	{
		client_edict = g_edicts + 1 + i;

		// not in use, ignore
		if (!client_edict->inuse || game.clients[i].resp.spectator)
			continue;

		score = game.clients[i].resp.score;

		for (j=0 ; j<total ; j++)
		{
			if (score > sorted_client_edicts[j]->client->resp.score)
				break;
		}

		for (k=total ; k>j ; k--)
		{
			sorted_client_edicts[k] = sorted_client_edicts[k-1];
		}

		sorted_client_edicts[j] = client_edict;
		total++;
	}

	for (int32_t client_num = 0; client_num < game.maxclients; client_num++)
	{
		client_edict = sorted_client_edicts[client_num]; // client edicts are always at the start???

		// is the client actually being used?
		if (client_edict != NULL) // always the same number of clients
		{
			gi.WriteString(client_edict->client->pers.netname);
			gi.WriteShort(client_edict->client->ping);
			gi.WriteShort(client_edict->client->resp.score);
			gi.WriteShort(client_edict->team);
			gi.WriteShort((level.framenum - client_edict->client->resp.enterframe) / 600); // inherited from earlier code figure out what this actually does
			gi.WriteByte(client_edict->client->resp.spectator);
			gi.WriteString(level.level_name);

			// send the time remaining if timelimit > 0
			if (timelimit->value > 0)
			{
				gi.WriteShort((timelimit->value) - level.time);
			}
			else
			{
				gi.WriteShort(level.time);
			}
		}
	}

	// send it!
	gi.unicast(ent, false); // make it false? it's still not very common...(only when the user presses TAB)
}

void Client_CommandLeaderboard(edict_t* ent)
{
	GameUI_SendLeaderboard(ent);
}

/*
===============
G_SetStats
===============
*/
void GameUI_SetStats (edict_t *ent)
{
	gitem_t			*item;
	loadout_entry_t* cells = Loadout_GetItem(ent, "cells");
	loadout_entry_t* armor = Armor_GetCurrent(ent);

	int32_t			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	if (!ent->client->loadout_current_ammo->item_name)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = Item_FindByPickupName(ent->client->loadout_current_ammo->item_name);
		
		if (!item)
		{
			ent->client->ps.stats[STAT_AMMO_ICON] = 0;
			ent->client->ps.stats[STAT_AMMO] = 0;
		}
		else
		{
			ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
			ent->client->ps.stats[STAT_AMMO] = ent->client->loadout_current_ammo->amount;
		}
	}
	
	//
	// armor
	//
	power_armor_type = Armor_GetCurrentPowerArmor (ent);

	if (power_armor_type)
	{
		if (cells == NULL
			|| cells->amount == 0)
		{	
			// we have run out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;;
		}
	}

	if (power_armor_type
	&& (!armor || (level.framenum & 8)) )
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex("2d/i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells->amount;
	}
	else if (armor)
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(armor->icon);
		ent->client->ps.stats[STAT_ARMOR] = armor->amount;
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("2d/p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("2d/p_invulnerability");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("2d/p_envirosuit");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum)/10;
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("2d/p_rebreather");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum)/10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (ent->client->pers.health <= 0 || level.intermissiontime)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	ent->client->ps.stats[STAT_SPECTATOR] = 0;
}

/*
===============
G_CheckChaseStats
===============
*/
void GameUI_CheckChaseStats (edict_t *ent)
{
	int32_t i;
	gclient_t *cl;

	for (i = 1; i <= sv_maxclients->value; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		GameUI_SetStatsSpectator(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void GameUI_SetStatsSpectator (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		GameUI_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime)
		cl->ps.stats[STAT_LAYOUTS] |= 1;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + 
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}

