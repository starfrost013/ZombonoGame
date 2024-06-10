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

int	sm_meat_index;
int	snd_fry;
int32_t meansOfDeath;

edict_t		*g_edicts;

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

cvar_t  *aimfix;

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
bool ClientConnect (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void ClientCommand_NoConsole (edict_t* ent);
void WriteGame (char *filename, bool autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame ();
void G_RunFrame ();

// Gamemode-specific stuff
void CheckGamemodeRules();
void CheckTDMRules();

//===================================================================


void ShutdownGame ()
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;
	globals.ClientCommand_NoConsole = ClientCommand_NoConsole;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list	argptr;
	char	text[1024];

	va_start (argptr, error);
	vsnprintf (text, 1024, error, argptr);
	va_end (argptr);

	gi.error(text);
}

void Com_Printf (char *msg, ...)
{
	va_list	argptr;
	char	text[1024];

	va_start (argptr, msg);
	vsnprintf (text, 1024, msg, argptr);
	va_end (argptr);

	gi.dprintf (text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames ()
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
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
void EndMatch ()
{
	edict_t		*ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	// stay on same level flag
	if ((int32_t)gameflags->value & GF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string) {
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL) {
			if (Q_stricmp(t, level.mapname) == 0) {
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				} else
					BeginIntermission (CreateTargetChangeLevel (t) );
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
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else {	// search for a changelevel
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass ()
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

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckGamemodeRules
=================
*/
void CheckGamemodeRules ()
{
	if (level.intermissiontime)
		return;

	// todo: IMPLEMENT
	if (gamemode->value == GAMEMODE_TDM)
	{
		CheckTDMRules();
	}
}

#define TIME_BUF_LENGTH		20
#define SCORE_BUF_LENGTH	40

void CheckTDMRules()
{
	int32_t		i;
	gclient_t* cl;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value)
		{
			gi.bprintf(PRINT_HIGH, "Out of time!\n");
			EndMatch();
			return;
		}

		// update every 1 second roughly
		if (level.framenum % (int)(1/FRAMETIME) == 0)
		{
			char text[TIME_BUF_LENGTH] = { 0 };

			int32_t total_seconds = timelimit->value - (int32_t)level.time;
			// convert remaining time to integer mm:ss
			int32_t seconds = (int32_t)(total_seconds % 60);
			int32_t minutes = (int32_t)(total_seconds / 60);

			if (seconds < 10)
			{
				snprintf(&text, TIME_BUF_LENGTH, "Time: %d:0%d", minutes, seconds);
			}
			else
			{
				snprintf(&text, TIME_BUF_LENGTH, "Time: %d:%d", minutes, seconds);
			}

			// multicast time remaining to each client
			G_UISetText(NULL, "TimeUI", "TimeUI_Text", text, false);
		}

	}

	// every second update each team's score
	if (level.framenum % (int)(1 / FRAMETIME) == 0)
	{
		team_scores_t team_scores = Gamemode_TDMGetScores();
		char text[SCORE_BUF_LENGTH] = { 0 };

		snprintf(text, SCORE_BUF_LENGTH, "Directors %d : Players %d", team_scores.director_score, team_scores.player_score);

		G_UISetText(NULL, "ScoreUI", "ScoreUI_Text", text, false);
	}

	if (fraglimit->value)
	{
		for (i = 0; i < maxclients->value; i++)
		{
			if ((int32_t)gameflags->value & GF_INDIVIDUAL_FRAGLIMIT)
			{
				cl = game.clients + i;
				if (!g_edicts[i + 1].inuse)
					continue;

				if (cl->resp.score >= fraglimit->value)
				{
					gi.bprintf(PRINT_HIGH, "Fraglimit hit!\n");
					EndMatch();
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
					EndMatch();
					return;
				}
			}

		}
	}
}

/*
=============
ExitLevel
This runs after the end of a game
=============
*/
void ExitLevel ()
{
	int		i;
	edict_t	*ent;
	char	command [256];

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	level.framenum = 0; // reset frame number 
	level.time = 0;
	ClientEndServerFrames ();

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
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
void G_RunFrame ()
{
	int		i;
	edict_t	*ent;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
			continue;
		}

		G_RunEntity (ent);
	}

	// see if it is time to end a deathmatch
	CheckGamemodeRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();
}

