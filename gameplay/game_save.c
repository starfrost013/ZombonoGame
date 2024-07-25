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
// game_save.c: Deals with saving and loading the game to SSV files

#include <game_local.h>

#define Function(f) {#f, f}

mmove_t mmove_reloc;

field_t fields[] = 
{
	{ "classname", FOFS(classname), F_LSTRING },
	{ "model", FOFS(model), F_LSTRING },
	{ "spawnflags", FOFS(spawnflags), F_INT },
	{ "speed", FOFS(speed), F_FLOAT },
	{ "accel", FOFS(accel), F_FLOAT },
	{ "decel", FOFS(decel), F_FLOAT },
	{ "target", FOFS(target), F_LSTRING },
	{ "targetname", FOFS(targetname), F_LSTRING },
	{ "pathtarget", FOFS(pathtarget), F_LSTRING },
	{ "deathtarget", FOFS(deathtarget), F_LSTRING },
	{ "killtarget", FOFS(killtarget), F_LSTRING },
	{ "combattarget", FOFS(combattarget), F_LSTRING },
	{ "message", FOFS(message), F_LSTRING },
	{ "team", FOFS(team), F_INT }, // 0 = director, 1 = player
	{ "wait", FOFS(wait), F_FLOAT },
	{ "delay", FOFS(delay), F_FLOAT },
	{ "random", FOFS(random), F_FLOAT },
	{ "move_origin", FOFS(move_origin), F_VECTOR3 },
	{ "move_angles", FOFS(move_angles), F_VECTOR3 },
	{ "style", FOFS(style), F_INT },
	{ "count", FOFS(count), F_INT },
	{ "health", FOFS(health), F_INT },
	{ "sounds", FOFS(sounds), F_INT },
	{ "light", 0, F_IGNORE },
	{ "dmg", FOFS(dmg), F_INT },
	{ "mass", FOFS(mass), F_INT },
	{ "volume", FOFS(volume), F_FLOAT },
	{ "attenuation", FOFS(attenuation), F_FLOAT },
	{ "map", FOFS(map), F_LSTRING },
	{ "origin", FOFS(s.origin), F_VECTOR3 },
	{ "angle", FOFS(s.angles), F_ANGLEHACK },
	{ "angles", FOFS(s.angles), F_VECTOR3 },
	{ "velocity", FOFS(velocity), F_VECTOR3 },
	{ "angles_spread", FOFS(angles_spread), F_INT },
	{ "jump_velocity", FOFS(jump_height), F_INT },
	{ "particle_effect", FOFS(particle_effect), F_INT },
	{ "particle_rate", FOFS(particle_rate), F_INT },
	{ "particle_lifetime", FOFS(particle_lifetime), F_INT },
	{ "particle_magnitude", FOFS(particle_magnitude), F_INT },
	{ "alphavel", FOFS(alphavel), F_FLOAT},
	{ "color", FOFS(color), F_VECTOR4},
	{ "color_run", FOFS(color_run), F_VECTOR3},

	// Area of effects of entity
	{ "extents", FOFS(s.extents), F_VECTOR3 },

	{ "goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN },
	{ "movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN },
	{ "enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN },
	{ "oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN },
	{ "activator", FOFS(activator), F_EDICT, FFL_NOSPAWN },
	{ "groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN },
	{ "teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN },
	{ "teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN },
	{ "owner", FOFS(owner), F_EDICT, FFL_NOSPAWN },
	{ "mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN },
	{ "mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN },
	{ "target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN },
	{ "chain", FOFS(chain), F_EDICT, FFL_NOSPAWN },

	{ "prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN },
	{ "think", FOFS(think), F_FUNCTION, FFL_NOSPAWN },
	{ "blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN },
	{ "touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN },
	{ "use", FOFS(use), F_FUNCTION, FFL_NOSPAWN },
	{ "pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN },
	{ "die", FOFS(die), F_FUNCTION, FFL_NOSPAWN },

	{ "stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN },
	{ "idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN },
	{ "search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN },
	{ "walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN },
	{ "run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN },
	{ "dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN },
	{ "attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN },
	{ "melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN },
	{ "sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN },
	{ "checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN },
	{ "currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN },

	{ "endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN },
	{ "allowed_teams", FOFS(allowed_teams), F_INT, FFL_NOSPAWN },							// Team doors 

	// temp spawn vars -- only valid when the spawn function is called
	{ "lip", STOFS(lip), F_INT, FFL_SPAWNTEMP },
	{ "distance", STOFS(distance), F_INT, FFL_SPAWNTEMP },
	{ "height", STOFS(height), F_INT, FFL_SPAWNTEMP },
	{ "noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP },
	{ "pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP },
	{ "item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP },

	//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves 
	{ "item", FOFS(item), F_ITEM },

	{ "gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP },
	{ "sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP },
	{ "skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP },
	{ "skyaxis", STOFS(skyaxis), F_VECTOR3, FFL_SPAWNTEMP },
	{ "minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP },
	{ "minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP },
	{ "nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP },

	// Valve 220 map support 
	{ "mapversion", 0, F_IGNORE },

	{0, 0, 0, 0}

};

field_t		levelfields[] =
{
	{"changemap", LLOFS(changemap), F_LSTRING},
                   
	{"sight_client", LLOFS(sight_client), F_EDICT},
	{"sight_entity", LLOFS(sight_entity), F_EDICT},
	{"sound_entity", LLOFS(sound_entity), F_EDICT},
	{"sound2_entity", LLOFS(sound2_entity), F_EDICT},

	{NULL, 0, F_INT}
};

field_t		clientfields[] =
{
	{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
	{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
	{"newweapon", CLOFS(newweapon), F_ITEM},

	{NULL, 0, F_INT}
};

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void Game_Init ()
{
	gi.dprintf("==== InitGame ====\n");

	gun_x = gi.Cvar_Get("gun_x", "0", 0);
	gun_y = gi.Cvar_Get("gun_y", "0", 0);
	gun_z = gi.Cvar_Get("gun_z", "0", 0);

	//use phys prefix for these?
	sv_rollspeed = gi.Cvar_Get("sv_rollspeed", "200", 0);
	sv_rollangle = gi.Cvar_Get("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.Cvar_Get("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.Cvar_Get("sv_gravity", "800", 0);
	sv_stopspeed = gi.Cvar_Get("sv_stopspeed", "100", 0);
	sv_friction = gi.Cvar_Get("sv_friction", "6", 0);
	sv_waterfriction = gi.Cvar_Get("sv_waterfriction", "1", 0);

	// noset vars
	dedicated = gi.Cvar_Get("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);

	gi.Cvar_Get("gamename", GAME_NAME, CVAR_SERVERINFO | CVAR_LATCH);
	gi.Cvar_Get("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH);
	gi.Cvar_Get("gameversion", GAME_VERSION, CVAR_SERVERINFO | CVAR_LATCH);

	maxclients = gi.Cvar_Get("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.Cvar_Get("maxspectators", "4", CVAR_SERVERINFO);
	gamemode = gi.Cvar_Get("gamemode", "0", CVAR_SERVERINFO | CVAR_LATCH);
	skill = gi.Cvar_Get("skill", "1", CVAR_LATCH);

	// change anytime vars
	gameflags = gi.Cvar_Get("gameflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	password = gi.Cvar_Get("password", "", CVAR_USERINFO);
	spectator_password = gi.Cvar_Get("spectator_password", "", CVAR_USERINFO);
	needpass = gi.Cvar_Get("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.Cvar_Get("filterban", "1", 0);

	g_select_empty = gi.Cvar_Get("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.Cvar_Get("run_pitch", "0.002", 0);
	run_roll = gi.Cvar_Get("run_roll", "0.005", 0);
	bob_up = gi.Cvar_Get("bob_up", "0.005", 0);
	bob_pitch = gi.Cvar_Get("bob_pitch", "0.002", 0);
	bob_roll = gi.Cvar_Get("bob_roll", "0.002", 0);

	// flood control
	flood_msgs = gi.Cvar_Get("flood_msgs", "4", 0);
	flood_persecond = gi.Cvar_Get("flood_persecond", "4", 0);
	flood_waitdelay = gi.Cvar_Get("flood_waitdelay", "10", 0);

	// multiplayer server map list
	sv_maplist = gi.Cvar_Get("sv_maplist", "", 0);

	/* others */
	aimfix = gi.Cvar_Get("aimfix", "0", CVAR_ARCHIVE);

	// items
	ItemList_Init();

	// initialize all entities for this game
	game.maxentities = MAX_EDICTS;
	g_edicts = gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = (int32_t)maxclients->value;
	game.clients = gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients + 1;
}

//=========================================================

void WriteField1 (FILE *f, field_t *field, uint8_t *base)
{
	void*	p;
	int32_t	len;
	int32_t	index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR3:
	case F_VECTOR4:
	case F_IGNORE:
		break;

	case F_LSTRING:
	case F_GSTRING:
		if ( *(char **)p )
			len = (int32_t)strlen(*(char **)p) + 1;
		else
			len = 0;
		*(int32_t *)p = len;
		break;
	case F_EDICT:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(edict_t **)p - g_edicts;
		*(int32_t *)p = index;
		break;
	case F_CLIENT:
		if ( *(gclient_t **)p == NULL)
			index = -1;
		else
			index = *(gclient_t **)p - game.clients;
		*(int32_t *)p = index;
		break;
	case F_ITEM:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(gitem_t **)p - itemlist;
		*(int32_t *)p = index;
		break;

	//relative to code segment
	case F_FUNCTION:
		if (*(uint8_t**)p == NULL)
			index = 0;
		else
			index = *(uint8_t**)p - ((uint8_t *)Game_Init);
		*(int32_t *)p = index;
		break;

	//relative to data segment
	case F_MMOVE:
		if (*(uint8_t**)p == NULL)
			index = 0;
		else
			index = *(uint8_t**)p - (uint8_t*)&mmove_reloc;
		*(int32_t *)p = index;
		break;

	default:
		gi.error ("WriteEdict: unknown field type");
	}
}


void WriteField2 (FILE *f, field_t *field, uint8_t *base)
{
	int32_t	len;
	void*	p;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_LSTRING:
		if ( *(char **)p )
		{
			len = (int32_t)strlen(*(char **)p) + 1;
			fwrite (*(char **)p, len, 1, f);
		}
		break;
	default:
		break;
	}
}

void ReadField (FILE *f, field_t *field, uint8_t *base)
{
	void*	p;
	int32_t	len;
	int32_t	index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
		// these are already parsed
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR3:
	case F_VECTOR4:
	case F_IGNORE:
		break;

	case F_LSTRING:
		len = *(int32_t *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = gi.TagMalloc (len, TAG_LEVEL);
			fread (*(char **)p, len, 1, f);
		}
		break;
	case F_EDICT:
		index = *(int32_t *)p;
		if ( index == -1 )
			*(edict_t **)p = NULL;
		else
			*(edict_t **)p = &g_edicts[index];
		break;
	case F_CLIENT:
		index = *(int32_t *)p;
		if ( index == -1 )
			*(gclient_t **)p = NULL;
		else
			*(gclient_t **)p = &game.clients[index];
		break;
	case F_ITEM:
		index = *(int32_t *)p;
		if ( index == -1 )
			*(gitem_t **)p = NULL;
		else
			*(gitem_t **)p = &itemlist[index];
		break;

	//relative to code segment
	case F_FUNCTION:
		index = *(int32_t *)p;
		if ( index == 0 )
			*(uint8_t **)p = NULL;
		else
			*(uint8_t**)p = ((uint8_t*)Game_Init) + index;
		break;

	//relative to data segment
	case F_MMOVE:
		index = *(int32_t *)p;
		if (index == 0)
			*(uint8_t**)p = NULL;
		else
			*(uint8_t **)p = (uint8_t *)&mmove_reloc + index;
		break;

	default:
		gi.error ("ReadEdict: unknown field type");
	}
}

//=========================================================

/*
==============
WriteClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteClient (FILE *f, gclient_t *client)
{
	field_t*	field;
	gclient_t	temp;
	
	// all of the ints, floats, and vectors stay as they are
	temp = *client;

	// change the pointers to lengths or indexes
	for (field=clientfields ; field->name ; field++)
	{
		WriteField1 (f, field, (uint8_t *)&temp);
	}

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field=clientfields ; field->name ; field++)
	{
		WriteField2 (f, field, (uint8_t*)client);
	}
}

/*
==============
ReadClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadClient (FILE *f, gclient_t *client)
{
	field_t* field;

	fread (client, sizeof(*client), 1, f);

	for (field=clientfields ; field->name ; field++)
	{
		ReadField (f, field, (uint8_t*)client);
	}
}

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void Game_Write (char *filename, bool autosave)
{
	FILE*	f;
	int32_t	i;
	char	str[16];

	if (!autosave)
		SaveClientData ();

	f = fopen (filename, "wb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	memset (str, 0, sizeof(str));
	strcpy (str, __DATE__);
	fwrite (str, sizeof(str), 1, f);

	game.autosaved = autosave;
	fwrite (&game, sizeof(game), 1, f);
	game.autosaved = false;

	for (i=0 ; i<game.maxclients ; i++)
		WriteClient (f, &game.clients[i]);

	fclose (f);
}

void Game_Read (char *filename)
{
	FILE*	f;
	int32_t	i;
	char	str[16];

	gi.FreeTags (TAG_GAME);

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	fread (str, sizeof(str), 1, f);
	if (strcmp (str, __DATE__))
	{
		fclose (f);
		gi.error ("Savegame from an older version.\n");
	}

	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;

	fread (&game, sizeof(game), 1, f);
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	for (i=0 ; i<game.maxclients ; i++)
		ReadClient (f, &game.clients[i]);

	fclose (f);
}

//==========================================================


/*
==============
Edict_Write

All pointer variables (except function pointers) must be handled specially.
==============
*/
void Edict_Write (FILE* f, edict_t* ent)
{
	field_t		*field;
	edict_t		temp;

	// all of the ints, floats, and vectors stay as they are
	temp = *ent;

	// change the pointers to lengths or indexes
	for (field=fields ; field->name ; field++)
	{
		WriteField1 (f, field, (uint8_t*)&temp);
	}

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field=fields ; field->name ; field++)
	{
		WriteField2 (f, field, (uint8_t*)ent);
	}

}

/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void Level_WriteLocals (FILE *f)
{
	field_t		*field;
	level_locals_t		temp;

	// all of the ints, floats, and vectors stay as they are
	temp = level;

	// change the pointers to lengths or indexes
	for (field=levelfields ; field->name ; field++)
	{
		WriteField1 (f, field, (uint8_t*)&temp);
	}

	// write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field=levelfields ; field->name ; field++)
	{
		WriteField2 (f, field, (uint8_t *)&level);
	}
}


/*
==============
ReadEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
void Edict_Read (FILE *f, edict_t *ent)
{
	field_t		*field;

	fread (ent, sizeof(*ent), 1, f);

	for (field=fields ; field->name ; field++)
	{
		ReadField (f, field, (uint8_t *)ent);
	}
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void Level_ReadLocals (FILE *f)
{
	field_t		*field;

	fread (&level, sizeof(level), 1, f);

	for (field=levelfields ; field->name ; field++)
	{
		ReadField (f, field, (uint8_t *)&level);
	}
}

/*
=================
WriteLevel

=================
*/
void Level_Write (char *filename)
{
	int32_t	i;
	edict_t	*ent;
	FILE	*f;
	void	*base;

	f = fopen (filename, "wb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// write out edict size for checking
	i = sizeof(edict_t);
	fwrite (&i, sizeof(i), 1, f);

	// write out a function pointer for checking
	base = (void *)Game_Init;
	fwrite (&base, sizeof(base), 1, f);

	// write out level_locals_t
	Level_WriteLocals (f);

	// write out all the entities
	for (i=0 ; i<globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];

		// ignore nonexistent edicts
		if (!ent->inuse)
			continue;

		// ignore ephemeral stuff
		if (ent->flags & FL_NO_SAVE)
			continue;

		fwrite (&i, sizeof(i), 1, f);
		Edict_Write (f, ent);
	}
	i = -1;
	fwrite (&i, sizeof(i), 1, f);

	fclose (f);
}


/*
=================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void Level_Read (char *filename)
{
	int32_t	entnum;
	FILE*	f;
	int32_t	i;
	void*	base;
	edict_t* ent;

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags (TAG_LEVEL);

	// wipe all the entities
	memset (g_edicts, 0, game.maxentities*sizeof(g_edicts[0]));
	globals.num_edicts = maxclients->value+1;

	// check edict size
	fread (&i, sizeof(i), 1, f);
	if (i != sizeof(edict_t))
	{
		fclose (f);
		gi.error ("ReadLevel: mismatched edict size");
	}

	// check function pointer base address
	fread (&base, sizeof(base), 1, f);

	// load the level locals
	Level_ReadLocals (f);

	// load all the entities
	while (1)
	{
		if (fread (&entnum, sizeof(entnum), 1, f) != 1)
		{
			fclose (f);
			gi.error ("ReadLevel: failed to read entnum");
		}
		if (entnum == -1)
			break;
		if (entnum >= globals.num_edicts)
			globals.num_edicts = entnum+1;

		ent = &g_edicts[entnum];
		Edict_Read (f, ent);

		// let the server rebuild world links for this ent
		memset (&ent->area, 0, sizeof(ent->area));
		gi.Edict_Link (ent);
	}

	fclose (f);

	// mark all clients as unconnected
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = &g_edicts[i+1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
	}

	// do any load time things at this point
	for (i=0 ; i<globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level triggers
		if (ent->classname)
			if (strcmp(ent->classname, "target_crosslevel_target") == 0)
				ent->nextthink = level.time + ent->delay;
	}
}
