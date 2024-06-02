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

// game.h -- game dll information visible to server

#define	GAME_API_VERSION	8

// edict->svflags

#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects
#define	SVF_DEADMONSTER			0x00000002	// treat as CONTENTS_DEADMONSTER for collision
#define	SVF_MONSTER				0x00000004	// treat as CONTENTS_MONSTER for collision

// edict->solid values

typedef enum
{
SOLID_NOT,			// no interaction with other objects
SOLID_TRIGGER,		// only touch when inside, after moving
SOLID_BBOX,			// touch on edge
SOLID_BSP			// bsp clip, touch on edge
} solid_t;

//===============================================================

// link_t is only used for entity area links now
typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

#define	MAX_ENT_CLUSTERS	16

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

#ifndef GAME_INCLUDE

struct gclient_s
{
	player_state_t	ps;		// communicated by server to clients
	int32_t 			ping;
	// the game dll can add anything it wants after
	// this point in the structure
};


struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;
	bool	inuse;
	int32_t 		linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf
	
	int32_t 		num_clusters;		// if -1, use headnode instead
	int32_t 		clusternums[MAX_ENT_CLUSTERS];
	int32_t 		headnode;			// unused if num_clusters != -1
	int32_t 		areanum, areanum2;

	//================================

	int32_t 		svflags;			// SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int32_t 		clipmask;
	edict_t		*owner;

	// the game dll can add anything it wants after
	// this point in the structure
};

#endif		// GAME_INCLUDE

//===============================================================

//
// functions provided by the main engine
//
typedef struct game_import_s
{
	// special messages
	void	(*bprintf) (int32_t printlevel, char *fmt, ...);
	void	(*dprintf) (char *fmt, ...);
	void	(*cprintf) (edict_t *ent, int32_t printlevel, char *fmt, ...);
	void	(*Text_Draw) (edict_t *ent, const char* font, int32_t x, int32_t y, const char* text, ...);
	void	(*centerprintf) (edict_t *ent, char *fmt, ...);
	void	(*sound) (edict_t *ent, int32_t channel, int32_t soundindex, float volume, float attenuation, float timeofs);
	void	(*positioned_sound) (vec3_t origin, edict_t *ent, int32_t channel, int32_t soundinedex, float volume, float attenuation, float timeofs);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void	(*configstring) (int32_t num, char *string);

	void	(*error) (char *fmt, ...);

	// the *index functions create configstrings and some internal server state
	int32_t 	(*modelindex) (char *name);
	int32_t 	(*soundindex) (char *name);
	int32_t 	(*imageindex) (char *name);

	void	(*setmodel) (edict_t *ent, char *name);

	// collision detection
	trace_t	(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int32_t contentmask);
	int32_t 	(*pointcontents) (vec3_t point);
	bool	(*inPVS) (vec3_t p1, vec3_t p2);
	bool	(*inPHS) (vec3_t p1, vec3_t p2);
	void		(*SetAreaPortalState) (int32_t portalnum, bool open);
	bool	(*AreasConnected) (int32_t area1, int32_t area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void	(*linkentity) (edict_t *ent);
	void	(*unlinkentity) (edict_t *ent);		// call before removing an interactive edict
	int32_t (*BoxEdicts) (vec3_t mins, vec3_t maxs, edict_t **list,	int32_t maxcount, int32_t areatype);
	void	(*Pmove) (pmove_t *pmove);		// player movement code common with client prediction

	// network messaging
	void	(*multicast) (vec3_t origin, multicast_t to);
	void	(*unicast) (edict_t *ent, bool reliable);
	void	(*WriteChar) (int32_t c);
	void	(*WriteByte) (int32_t c);
	void	(*WriteShort) (int32_t c);
	void	(*WriteInt) (int32_t c);
	void	(*WriteFloat) (float f);
	void	(*WriteString) (char *s);
	void	(*WritePos) (vec3_t pos);	// some fractional bits
	void	(*WriteDir) (vec3_t dir);
	void	(*WriteColor) (vec4_t color);
	void	(*WriteAngle) (float f);

	// managed memory allocation
	void	*(*TagMalloc) (int32_t size, int32_t tag);
	void	(*TagFree) (void *block);
	void	(*FreeTags) (int32_t tag);

	// console variable interaction
	cvar_t* (*cvar) (char *var_name, char *value, int32_t flags);
	cvar_t* (*cvar_set) (char *var_name, char *value);
	cvar_t* (*cvar_forceset) (char *var_name, char *value);

	// ClientCommand and ServerCommand parameter access
	int32_t 	(*argc) ();
	char	*(*argv) (int32_t n);
	char	*(*args) ();	// concatenation of all argv >= 1

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void	(*AddCommandString) (char *text);

	void	(*DebugGraph) (float value, int32_t color);
} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	int32_t 	apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void		(*Init) ();
	void		(*Shutdown) ();

	// each new level entered will cause a call to SpawnEntities
	void		(*SpawnEntities) (char *mapname, char *entstring, char *spawnpoint);

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void		(*WriteGame) (char *filename, bool autosave);
	void		(*ReadGame) (char *filename);

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void		(*WriteLevel) (char *filename);
	void		(*ReadLevel) (char *filename);

	bool		(*ClientConnect) (edict_t *ent, char *userinfo);
	void		(*ClientBegin) (edict_t *ent);
	void		(*ClientUserinfoChanged) (edict_t *ent, char *userinfo);
	void		(*ClientDisconnect) (edict_t *ent);
	void		(*ClientCommand) (edict_t *ent);
	void		(*ClientCommand_NoConsole) (edict_t* ent);
	void		(*ClientThink) (edict_t *ent, usercmd_t *cmd);

	void		(*RunFrame) ();

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void		(*ServerCommand) ();

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	// 
	// The size will be fixed when ge->Init() is called
	struct edict_s	*edicts;
	int32_t 		edict_size;
	int32_t 		num_edicts;		// current number, <= max_edicts
	int32_t 		max_edicts;
} game_export_t;

game_export_t *Sys_GetGameApi (game_import_t *import);
