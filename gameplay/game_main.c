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

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int32_t	snd_meat_index;
int32_t	snd_fry_index;
int32_t means_of_death;

edict_t* g_edicts;

cvar_t* gamemode;
cvar_t* gameflags;
cvar_t* skill;
cvar_t* fraglimit;
cvar_t* timelimit;
cvar_t* password;
cvar_t* spectator_password;
cvar_t* needpass;
cvar_t* maxclients;
cvar_t* maxspectators;
cvar_t* g_select_empty;
cvar_t* dedicated;

cvar_t* filterban;

cvar_t* sv_maxvelocity;
cvar_t* sv_gravity;
cvar_t* sv_stopspeed;
cvar_t* sv_friction;
cvar_t* sv_waterfriction;

cvar_t* sv_rollspeed;
cvar_t* sv_rollangle;
cvar_t* gun_x;
cvar_t* gun_y;
cvar_t* gun_z;

cvar_t* run_pitch;
cvar_t* run_roll;
cvar_t* bob_up;
cvar_t* bob_pitch;
cvar_t* bob_roll;

cvar_t* sv_cheats;

cvar_t* flood_msgs;
cvar_t* flood_persecond;
cvar_t* flood_waitdelay;

cvar_t* sv_maplist;

cvar_t* aimfix;

void Game_Write(char* filename, bool autosave);
void Game_Read(char* filename);
void Game_SpawnEntities(char* mapname, char* entities, char* spawnpoint);
void Game_Init();
void Game_RunFrame();

void Gamemode_Update();

void Level_Write(char* filename);
void Level_Read(char* filename);

void Client_Think(edict_t* ent, usercmd_t* cmd);
bool Client_Connect(edict_t* ent, char* userinfo);
void Client_Disconnect(edict_t* ent);
void Client_OnConnected(edict_t* ent);
void Client_Command(edict_t* ent);
void Client_CommandNoConsole(edict_t* ent);

//===================================================================


void Game_Shutdown()
{
	gi.dprintf("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t * GetGameAPI(game_import_t * import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;

	globals.Game_Init = Game_Init;
	globals.Game_Shutdown = Game_Shutdown;
	globals.Game_SpawnEntities = Game_SpawnEntities;
	globals.Game_Write = Game_Write;
	globals.Game_Read = Game_Read;
	globals.Game_RunFrame = Game_RunFrame;

	globals.Level_Write = Level_Write;
	globals.Level_Read = Level_Read;

	globals.Client_Think = Client_Think;
	globals.Client_Connect = Client_Connect;
	globals.Client_UserinfoChanged = Client_UserinfoChanged;
	globals.Client_Disconnect = Client_Disconnect;
	globals.Client_OnConnected = Client_OnConnected;
	globals.Client_Command = Client_Command;
	globals.Client_CommandNoConsole = Client_CommandNoConsole;

	globals.Server_Command = Server_Command;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error(char* error, ...)
{
	va_list	argptr;
	char	text[1024];

	va_start(argptr, error);
	vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	gi.error(text);
}

void Com_Printf(char* msg, ...)
{
	va_list	argptr;
	char	text[1024];

	va_start(argptr, msg);
	vsnprintf(text, 1024, msg, argptr);
	va_end(argptr);

	gi.dprintf(text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames()
{
	int32_t  i;
	edict_t* ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		Client_EndServerFrame(ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t* CreateTargetChangeLevel(char* map)
{
	edict_t* ent;

	ent = Edict_Spawn();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndMatch

The conditions to end the current gamemode have been reached.
=================
*/
void Game_EndMatch()
{
	edict_t* ent;
	char* s, * t, * f;
	static const char* seps = " ,\n\r";

	// stay on same level flag
	if ((int32_t)gameflags->value & GF_SAME_LEVEL)
	{
		Game_TransitionToNextMatch(CreateTargetChangeLevel(level.mapname));
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string)
	{
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL)
		{
			if (Q_stricmp(t, level.mapname) == 0)
			{
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						Game_TransitionToNextMatch(CreateTargetChangeLevel(level.mapname));
					else
						Game_TransitionToNextMatch(CreateTargetChangeLevel(f));
				}
				else
					Game_TransitionToNextMatch(CreateTargetChangeLevel(t));
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok(NULL, seps);
		}

		free(s);
	}

	if (level.nextmap[0]) // go to a specific map
		Game_TransitionToNextMatch(CreateTargetChangeLevel(level.nextmap));
	else {	// search for a changelevel
		ent = Game_FindEdictByValue(NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			Game_TransitionToNextMatch(CreateTargetChangeLevel(level.mapname));
			return;
		}
		Game_TransitionToNextMatch(ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void Game_CheckIfPasswordRequired()
{
	int32_t need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.Cvar_Set("needpass", va("%d", need));
	}
}

/*
=================
CheckGamemodeRules
=================
*/
void Gamemode_Update()
{
	if (level.intermissiontime)
		return;

	// todo: IMPLEMENT
	if (gamemode->value == GAMEMODE_TDM)
	{
		Gamemode_TDMCheckRules();
	}
	else if (gamemode->value == GAMEMODE_WAVES)
	{
		Gamemode_WavesUpdate();
		Gamemode_WavesCheckRules();
	}
}

/*
=============
ExitLevel
This runs after the end of a game
=============
*/
void Level_Exit()
{
	int32_t i;
	edict_t* ent;
	char	command[256];

	Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	level.framenum = 0; // reset frame number 
	level.time = 0;
	ClientEndServerFrames();

	// clear some things before going to next level
	for (i = 0; i < maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}

}

/*
================
G_RunFrame

Advances the world by (1/TICKRATE) seconds
================
*/
void Game_RunFrame()
{
	int32_t  i;
	edict_t* ent;

	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient();

	// exit intermissions

	if (level.exitintermission)
	{
		Level_Exit();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i = 0; i < globals.num_edicts; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy3(ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;

			if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svflags & SVF_MONSTER))
			{
				AI_CheckGround(ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			Client_BeginServerFrame(ent); 
			continue;
		}

		Physics_RunEntity(ent);
	}

	// update the gamemode 
	// see if it is time to end a deathmatch
	Gamemode_Update();

	// see if needpass needs updated
	Game_CheckIfPasswordRequired();

	// build the playerstate_t structures for all players
	ClientEndServerFrames();
}

