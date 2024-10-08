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

/*
=====================
Client_OnConnected

A client has just connected to the server
=====================
*/
void Client_OnConnected(edict_t* ent)
{
	ent->client = game.clients + (ent - g_edicts - 1);

	Edict_Init(ent);

	Client_InitRespawn(ent->client);

	// locate ent at a spawn point
	Client_JoinServer(ent);

	if (level.intermissiontime)
	{
		player_team winning_team = (gamemode->value == GAMEMODE_TDM) ? Gamemode_TDMGetWinner() : 0;
		Player_MoveToIntermission(ent, winning_team);
	}
	else
	{
		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	// make sure all view stuff is valid
	Client_EndServerFrame(ent);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void Client_UserinfoChanged(edict_t* ent, char* userinfo)
{
	char* s;
	int32_t	playernum;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	// set name
	s = Info_ValueForKey(userinfo, "name");
	strncpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname) - 1);

	// set spectator
	s = Info_ValueForKey(userinfo, "spectator");
	// spectators are only supported in deathmatch
	if (*s && strcmp(s, "0"))
		ent->client->pers.spectator = true;

	// set skin
	s = Info_ValueForKey(userinfo, "skin");

	playernum = ent - g_edicts - 1;

	// combine name and skin into a configstring
	gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s", ent->client->pers.netname, s));

	// fov
	ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
	if (ent->client->ps.fov < 1)
		ent->client->ps.fov = 90;
	else if (ent->client->ps.fov > 160)
		ent->client->ps.fov = 160;

	// handedness
	s = Info_ValueForKey(userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo) - 1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool Client_Connect(edict_t* ent, char* userinfo)
{
	char* value;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");

	if (Server_IsClientAllowed(value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a spectator
	value = Info_ValueForKey(userinfo, "spectator");

	if (*value && strcmp(value, "0"))
	{
		int32_t i, numspec;

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value)) 
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < sv_maxclients->value; i++)
		{
			if (g_edicts[i + 1].inuse && g_edicts[i + 1].client->pers.spectator)
				numspec++;
		}

		if (numspec >= maxspectators->value) 
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	}
	else
	{
		// check for a password
		value = Info_ValueForKey(userinfo, "password");
		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}


	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		Client_InitRespawn(ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			Client_InitPersistent(ent);
	}

	Client_UserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintf("%s connected\n", ent->client->pers.netname);

	ent->svflags = 0; // make sure we start with known default
	ent->client->pers.connected = true;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void Client_Disconnect(edict_t* ent)
{
	int32_t	playernum;

	if (!ent->client)
		return;

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	// send effect
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_LOGOUT);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.Edict_Unlink(ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent - g_edicts - 1;
	gi.configstring(CS_PLAYERSKINS + playernum, "");
}

