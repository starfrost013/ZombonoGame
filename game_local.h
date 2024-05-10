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
// game_local.h -- local definitions for game module
#pragma once
#include "q_shared.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "game.h"

#include <inttypes.h>

#define	GAMENAME	"Zombono"

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION GAMENAME " v0.0.9 " __DATE__

// protocol bytes that can be directly added to messages
#define	svc_muzzleflash			1
#define	svc_muzzleflash2		2
#define	svc_temp_entity			3
#define	svc_layout				4				// THIS IS GOING TO BE REMOVED FOR BEING A MESS !!!!!
#define svc_uidraw				5
#define svc_uisettext			6
#define svc_uisetimage			7
#define svc_leaderboard			8
#define svc_leaderboarddraw		9				// hack for tdm mode
#define svc_drawtext			10
#define	svc_loadout_add			11
#define svc_loadout_remove		12
#define svc_loadout_setcurrent	13	
#define svc_loadout_clear		14

#define	svc_stufftext			19

//==================================================================

// view pitching times
#define DAMAGE_TIME		0.5
#define	FALL_TIME		0.3

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_TDM			0x00000800

#define	SPAWNFLAG_NOT_HOSTAGE		0x00001000
#define SPAWNFLAG_NOT_WAVES			0x00002000
#define SPAWNFLAG_NOT_COOP			0x00004000
#define SPAWNFLAG_NOT_CONTROL_POINT	0x00008000

// edict->flags
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	// implied immunity to drowining
#define FL_IMMUNE_LASER			0x00000004
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define FL_IMMUNE_SLIME			0x00000040
#define FL_IMMUNE_LAVA			0x00000080
#define	FL_PARTIALGROUND		0x00000100	// not all corners are valid
#define	FL_WATERJUMP			0x00000200	// player jumping out of water
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_POWER_ARMOR			0x00001000	// power armor (if any) is active
#define FL_RESPAWN				0x80000000	// used for item respawning

#define	FRAMETIME		0.1					//1/(frametime) = tickrate

// memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME	765		// clear when unloading the dll
#define	TAG_LEVEL	766		// clear when loading a new level

#define MELEE_DISTANCE	80					// Vector magnitude distance within which an enemy can melee.

#define BODY_QUEUE_SIZE		8

// for gameplay/game_client.c

extern	edict_t* current_player;
extern	gclient_t* current_client;

extern	vec3_t	forward, right, up;

extern float	xyspeed;

extern float	bobmove;
extern int		bobcycle;		// odd cycles are right foot going forward
extern float	bobfracsin;		// sin(bobfrac*M_PI)

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,			// will take damage if hit
	DAMAGE_AIM			// auto targeting recognizes this
} damage_t;

typedef enum 
{
	WEAPON_READY, 
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING_PRIMARY,
	WEAPON_FIRING_SECONDARY,
} weaponstate_t;

extern bool		is_quad;
extern uint8_t	is_silenced;

// STUPID hack
void P_ProjectSource(edict_t* ent, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void Weapon_grenade_fire(edict_t* ent, bool held);
void Weapon_Bamfuslicator_SetType(edict_t* ent);

typedef enum
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS
} Ammo_t;

//deadflag
#define DEAD_NO					0
#define DEAD_DYING				1
#define DEAD_DEAD				2
#define DEAD_RESPAWNABLE		3

//range
#define RANGE_MELEE				0
#define RANGE_NEAR				1
#define RANGE_MID				2
#define RANGE_FAR				3

//gib types
#define GIB_ORGANIC				0
#define GIB_METALLIC			1

//monster ai flags
#define AI_STAND_GROUND			0x00000001
#define AI_TEMP_STAND_GROUND	0x00000002
#define AI_SOUND_TARGET			0x00000004
#define AI_LOST_SIGHT			0x00000008
#define AI_PURSUIT_LAST_SEEN	0x00000010
#define AI_PURSUE_NEXT			0x00000020
#define AI_PURSUE_TEMP			0x00000040
#define AI_HOLD_FRAME			0x00000080
#define AI_GOOD_GUY				0x00000100
#define AI_BRUTAL				0x00000200
#define AI_NOSTEP				0x00000400
#define AI_DUCKED				0x00000800
#define AI_COMBAT_POINT			0x00001000
#define AI_MEDIC				0x00002000
#define AI_RESURRECTING			0x00004000
#define AI_WANDER				0x00008000

//monster attack state
#define AS_STRAIGHT				1
#define AS_SLIDING				2
#define	AS_MELEE				3
#define	AS_MISSILE				4

// armor types
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4

// power armor types
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

// handedness values
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2


// game.serverflags values
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff


// noise types for PlayerNoise
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2


// edict->movetype values
typedef enum movetype
{
MOVETYPE_NONE,			// never moves
MOVETYPE_NOCLIP,		// origin and angles change with no interaction
MOVETYPE_PUSH,			// no clip to world, push on box contact
MOVETYPE_STOP,			// no clip to world, stops on box contact

MOVETYPE_WALK,			// gravity
MOVETYPE_STEP,			// gravity, special edge handling
MOVETYPE_FLY,
MOVETYPE_TOSS,			// gravity
MOVETYPE_FLYMISSILE,	// extra size to monsters
MOVETYPE_BOUNCE
} movetype_t;

typedef struct gitem_armor_s
{
	int32_t	base_count;
	int32_t	max_count;
	float	normal_protection;
	float	energy_protection;
	int32_t	armor;
} gitem_armor_t;


// gitem_t->flags
#define	IT_WEAPON		1		// use makes active weapon
#define	IT_AMMO			2
#define IT_ARMOR		4
#define IT_UNUSED1		8		// Was IT_STAY_COOP
#define IT_UNUSED2		16		// Was IT_KEY
#define IT_POWERUP		32

// gitem_t->weapmodel for weapons indicates model index
// Todo: Replace most of the Q2 weapons
#define WEAP_BLASTER				1 
#define WEAP_SHOTGUN				2 
#define WEAP_SUPERSHOTGUN			3 
#define WEAP_MACHINEGUN				4 
#define WEAP_CHAINGUN				5 
#define WEAP_GRENADES				6 
#define WEAP_GRENADELAUNCHER		7 
#define WEAP_ROCKETLAUNCHER			8 
#define WEAP_HYPERBLASTER			9 
#define WEAP_RAILGUN				10
#define WEAP_BAMFUSLICATOR			11
#define WEAP_PLANFUSLICATOR			12		// Materialiser
#define WEAP_TANGFUSLICATOR			13		// Fuslicator 3: Zombie Edition

typedef struct gitem_s
{
	char		*classname;	// spawning name
	bool		(*pickup)(struct edict_s *ent, struct edict_s *other);
	void		(*use)(struct edict_s *ent, struct gitem_s *item);
	void		(*drop)(struct edict_s *ent, struct gitem_s *item);
	void		(*weaponthink)(struct edict_s *ent);
	char		*pickup_sound;
	char		*world_model;
	int32_t		world_model_flags;
	char		*view_model;

	// client side info
	char		*icon;
	char		*pickup_name;	// for printing on pickup
	int32_t		count_width;		// number of digits to display by icon

	int32_t		quantity;		// for ammo how much, for weapons how much is used per shot
	char		*ammo;			// for weapons
	int32_t		flags;			// IT_* flags

	int32_t		weapmodel;		// weapon model index (for weapons)

	void		*info;
	int32_t		tag;

	char		*precaches;		// string of all models, sounds, and images this item will use

	int32_t		allowed_teams;	// The teams that are allowed for this item.
	int32_t		spawn_type;		// Type of entity to spawn (item-specific)
} gitem_t;

#define SPEED_DIRECTOR			0.75			// Speed multiplier for director team. 
#define SPEED_PLAYER			1.0				// Speed multiplier for player team.

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct game_locals_s
{
	char		helpmessage1[512];
	char		helpmessage2[512];
	int32_t		helpchanged;	// flash F1 icon if non 0, play sound
								// and increment only if 1, 2, or 3

	gclient_t	*clients;		// [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char		spawnpoint[512];	// needed for coop respawns

	// store latched cvars here that we want to get at often
	int32_t			maxclients;
	int32_t			maxentities;

	// cross level triggers
	int32_t			serverflags;

	// items
	int32_t			num_items;

	bool	autosaved;
} game_locals_t;


//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct level_locals_s
{
	int32_t		framenum;				// controls level time
	float		time;					// level time is incremented by framenum*(frametime in seconds) - - 1 instance of level time = 1 tick (10Hz)

	char		level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)
	char		mapname[MAX_QPATH];		// the server name (base1, etc)
	char		nextmap[MAX_QPATH];		// go here when fraglimit is hit

	// intermission state
	float		intermissiontime;		// time the intermission was started
	char		*changemap;
	int32_t		exitintermission;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;

	edict_t		*sight_client;	// changed once each frame for coop games

	edict_t		*sight_entity;
	int32_t		sight_entity_framenum;
	edict_t		*sound_entity;
	int32_t		sound_entity_framenum;
	edict_t		*sound2_entity;
	int32_t		sound2_entity_framenum;

	int32_t		pic_health;

	int32_t		total_secrets;
	int32_t		found_secrets;

	int32_t		total_goals;
	int32_t		found_goals;

	int32_t		total_monsters;
	int32_t		killed_monsters;

	edict_t		*current_entity;	// entity running from G_RunFrame
	int32_t		body_que;			// dead bodies

	int32_t		power_cubes;		// ugly necessity for coop
} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct spawn_temp_s
{
	// world vars
	char		*sky;
	float		skyrotate;
	vec3_t		skyaxis;
	char		*nextmap;

	int32_t		lip;
	int32_t		distance;
	int32_t		height;
	char		*noise;
	float		pausetime;
	char		*item;
	char		*gravity;

	float		minyaw;
	float		maxyaw;
	float		minpitch;
	float		maxpitch;
} spawn_temp_t;


typedef struct moveinfo_s
{
	// fixed data
	vec3_t		start_origin;
	vec3_t		start_angles;
	vec3_t		end_origin;
	vec3_t		end_angles;

	int32_t		sound_start;
	int32_t		sound_middle;
	int32_t		sound_end;

	float		accel;
	float		speed;
	float		decel;
	float		distance;

	float		wait;

	// state data
	int32_t		state;
	vec3_t		dir;
	float		current_speed;
	float		move_speed;
	float		next_speed;
	float		remaining_distance;
	float		decel_distance;
	void		(*endfunc)(edict_t *);
} moveinfo_t;


typedef struct mframe_s
{
	void	(*aifunc)(edict_t *self, float dist);
	float	dist;
	void	(*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct mmove_s
{
	int32_t		firstframe;
	int32_t		lastframe;
	mframe_t	*frame;
	void		(*endfunc)(edict_t *self);
} mmove_t;

typedef struct monsterinfo_s
{
	mmove_t	*currentmove;
	int32_t	aiflags;
	int32_t	nextframe;
	float	scale;

	void	(*stand)(edict_t *self);
	void	(*idle)(edict_t *self);
	void	(*search)(edict_t *self);
	void	(*walk)(edict_t *self);
	void	(*run)(edict_t *self);
	void	(*dodge)(edict_t *self, edict_t *other, float eta);
	void	(*attack)(edict_t *self);
	void	(*melee)(edict_t *self);
	void	(*sight)(edict_t *self, edict_t *other);
	bool	(*checkattack)(edict_t *self);

	float	pausetime;
	float	attack_finished;

	vec3_t	saved_goal;
	float	search_time;
	float	trail_time;
	vec3_t	last_sighting;
	int32_t	attack_state;
	int32_t	lefty;
	float	idle_time;
	int32_t	linkcount;

	int32_t	power_armor_type;
	int32_t	power_armor_power;

	// AI_Wander only.
	int32_t	wander_steps_min;		// The minimum number of wander steps.
	int32_t	wander_steps_max;		// The maximum number of wander steps.
	int32_t	wander_steps;			// The current number of wander steps.
	int32_t	wander_steps_total;		// The current total number of wander steps.
} monsterinfo_t;

extern	game_locals_t	game;
extern	level_locals_t	level;
extern	game_import_t	gi;
extern	game_export_t	globals;
extern	spawn_temp_t	st;

extern	int	sm_meat_index;
extern	int	snd_fry;

// means of death
#define MOD_UNKNOWN			0
#define MOD_BLASTER			1
#define MOD_SHOTGUN			2
#define MOD_SSHOTGUN		3
#define MOD_MACHINEGUN		4
#define MOD_CHAINGUN		5
#define MOD_GRENADE			6
#define MOD_G_SPLASH		7
#define MOD_ROCKET			8
#define MOD_R_SPLASH		9
#define MOD_HYPERBLASTER	10
#define MOD_RAILGUN			11
#define MOD_HANDGRENADE		15
#define MOD_HG_SPLASH		16
#define MOD_WATER			17
#define MOD_SLIME			18
#define MOD_LAVA			19
#define MOD_CRUSH			20
#define MOD_TELEFRAG		21
#define MOD_FALLING			22
#define MOD_SUICIDE			23
#define MOD_HELD_GRENADE	24
#define MOD_EXPLOSIVE		25
#define MOD_BARREL			26
#define MOD_BOMB			27
#define MOD_EXIT			28
#define MOD_SPLASH			29
#define MOD_TARGET_LASER	30
#define MOD_TRIGGER_HURT	31
#define MOD_HIT				32
#define MOD_TARGET_BLASTER	33
#define MOD_ZOMBIE			34
#define MOD_CRUSHED			35
#define MOD_FRIENDLY_FIRE	0x8000000

extern	int	meansOfDeath;


extern	edict_t			*g_edicts;

#define	FOFS(x) (intptr_t)&(((edict_t *)0)->x)
#define	STOFS(x) (intptr_t)&(((spawn_temp_t *)0)->x)
#define	LLOFS(x) (intptr_t)&(((level_locals_t *)0)->x)
#define	CLOFS(x) (intptr_t)&(((gclient_t *)0)->x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

extern	cvar_t* gamemode;
extern	cvar_t* gameflags;
extern	cvar_t* skill;
extern	cvar_t* fraglimit;
extern	cvar_t* timelimit;
extern	cvar_t* password;
extern	cvar_t* spectator_password;
extern	cvar_t* needpass;
extern	cvar_t* g_select_empty;
extern	cvar_t* dedicated;

extern	cvar_t* filterban;

extern	cvar_t* sv_gravity;
extern	cvar_t* sv_maxvelocity;

extern	cvar_t* gun_x, *gun_y, *gun_z;
extern	cvar_t* sv_rollspeed;
extern	cvar_t* sv_rollangle;

extern	cvar_t* run_pitch;
extern	cvar_t* run_roll;
extern	cvar_t* bob_up;
extern	cvar_t* bob_pitch;
extern	cvar_t* bob_roll;

extern	cvar_t* sv_cheats;
extern	cvar_t* maxclients;
extern	cvar_t* maxspectators;

extern	cvar_t* flood_msgs;
extern	cvar_t* flood_persecond;
extern	cvar_t* flood_waitdelay;

extern	cvar_t* sv_maplist;

extern  cvar_t  *aimfix;

#define world	(&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN		0x00000001
#define ITEM_NO_TOUCH			0x00000002
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM			0x00010000
#define	DROPPED_PLAYER_ITEM		0x00020000
#define ITEM_TARGETS_USED		0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN			2

typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_FUNCTION,
	F_MMOVE,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char	*name;
	int32_t		ofs;
	fieldtype_t	type;
	int32_t		flags;
} field_t;


extern	field_t fields[];
extern	gitem_t	itemlist[];

// Zombie specific defines
// test zombie types
// maybe make these load from a file

typedef enum zombie_type_e
{
	zombie_type_normal = 0,

	zombie_type_fast = 1,

	zombie_type_ogre = 2,

} zombie_type;

#define MAX_ZOMBIE_TYPE				2

#define ZOMBIE_HEALTH_STANDARD		60
#define	ZOMBIE_HEALTH_FAST			50

#define ZOMBIE_DAMAGE_STANDARD		15
#define ZOMBIE_DAMAGE_FAST			25

#define ZOMBIE_RANGE_STANDARD		MELEE_DISTANCE
// todo: fast has more range?

// ogre stuff
// shrek attack
#define OGRE_RANGE_MELEE			MELEE_DISTANCE		// Range of the ogre's melee attack

#define OGRE_DAMAGE_MELEE			80					// Damage of the ogre's melee attack (80 because it moves so slow only incompetents walk into it)
#define OGRE_DAMAGE_FUNNY_SHREK		40					// Damage of the ogre's rockets
#define OGRE_SPEED_FUNNY_SHREK		850					// Speed of the ogre's rockets

#define OGRE_HEALTH					105

//
// g_cmds.c
//
void Cmd_Leaderboard_f (edict_t* ent);

//
// g_items.c
//
void PrecacheItem (gitem_t *it);
void InitItems (void);
void SetItemNames (void);
gitem_t	*FindItem (char *pickup_name);
gitem_t	*FindItemByClassname (char *classname);
#define	ITEM_INDEX(x) ((x)-itemlist)
edict_t *Drop_Item (edict_t *ent, gitem_t *item);
void SetRespawn (edict_t *ent, float delay);
void ChangeWeapon (edict_t *ent);
void SpawnItem (edict_t *ent, gitem_t *item);
void Think_Weapon (edict_t *ent);
loadout_entry_t* GetCurrentArmor (edict_t *ent);
int32_t GetCurrentPowerArmor (edict_t *ent);
gitem_t	*GetItemByIndex (int32_t index);
bool Add_Ammo (edict_t *ent, gitem_t *item, int32_t count);
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

//
// g_utils.c
//
bool	KillBox (edict_t *ent);
void	G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t *G_Find (edict_t *from, int32_t fieldofs, char *match);
edict_t *findradius (edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget (char *targetname);
void	G_UseTargets (edict_t *ent, edict_t *activator);
void	G_SetMovedir (vec3_t angles, vec3_t movedir);

void	G_InitEdict (edict_t *e);
edict_t	*G_Spawn (void);
void	G_FreeEdict (edict_t *e);

void	G_TouchTriggers (edict_t *ent);
void	G_TouchSolids (edict_t *ent);

void	G_UISend (edict_t *ent, char* ui_name, bool enabled, bool activated, bool reliable);
void	G_UISetText(edict_t* ent, char* ui_name, char* control_name, char* text, bool reliable);
void	G_UISetImage(edict_t* ent, char* ui_name, char* control_name, char* image_path, bool reliable);

int32_t		G_CountClients();

void	G_LeaderboardSend(edict_t* ent);

char	*G_CopyString (char *in);

float	*tv (float x, float y, float z);
char	*vtos (vec3_t v);

float vectoyaw (vec3_t vec);
void vectoangles (vec3_t vec, vec3_t angles);

// A tiny little struct containing both teams' scores.
typedef struct team_scores_s
{
	int32_t director_score;
	int32_t player_score;
} team_scores_t;

player_team	G_TDMGetWinner();
team_scores_t G_TDMGetScores();

//
// g_combat.c
//
bool CanDamage (edict_t *targ, edict_t *inflictor);
void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int32_t damage, int32_t knockback, int32_t dflags, int32_t mod);
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int32_t mod);

//
// p_client.c
// 
void	SelectSpawnPoint(edict_t* ent, vec3_t origin, vec3_t angles);
void	GiveBaseWeaponForTeam(edict_t* client_edict);

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_ENERGY			0x00000004	// damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK		0x00000008	// do not affect velocity, just view angles
#define DAMAGE_BULLET			0x00000010  // damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION	0x00000020  // armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
#define DEFAULT_SHOTGUN_HSPREAD	1000
#define DEFAULT_SHOTGUN_VSPREAD	500
#define DEFAULT_SHOTGUN_COUNT	12
#define DEFAULT_SSHOTGUN_COUNT	20

//
// ai_monster.c
//
void Ammo_Bullet_monster (edict_t *self, vec3_t start, vec3_t dir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t flashtype);
void Ammo_Shotgun_monster (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, int32_t flashtype);
void Ammo_Blaster_monster (edict_t *self, vec3_t start, vec3_t dir, int32_t damage, int32_t speed, int32_t flashtype, int32_t effect);
void Ammo_Grenade_monster (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, int32_t flashtype);
void Ammo_Rocket_monster (edict_t *self, vec3_t start, vec3_t dir, int32_t damage, int32_t speed, int32_t flashtype);
void Ammo_Rail_monster (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t kick, int32_t flashtype);
void M_droptofloor (edict_t *ent);
void monster_think (edict_t *self);
void monster_check_dodge(edict_t* self, vec3_t start, vec3_t dir, int32_t speed);
void walkmonster_start (edict_t *self);
void swimmonster_start (edict_t *self);
void flymonster_start (edict_t *self);
void AttackFinished (edict_t *self, float time);
void monster_death_use (edict_t *self);
void M_CategorizePosition (edict_t *ent);
bool M_CheckAttack (edict_t *self);
void M_FlyCheck (edict_t *self);
void M_CheckGround (edict_t *ent);

//
// g_misc.c
//
void ThrowHead (edict_t *self, char *gibname, int32_t damage, int32_t type);
void ThrowClientHead (edict_t *self, int32_t damage);
void ThrowGib (edict_t *self, char *gibname, int32_t damage, int32_t type);
void BecomeExplosion1(edict_t *self);

//
// ai_base.c
//
void AI_SetSightClient (void);

void ai_stand (edict_t *self, float dist);
void ai_move (edict_t *self, float dist);
void ai_walk (edict_t *self, float dist);
void ai_turn (edict_t *self, float dist);
void ai_run (edict_t *self, float dist);
void ai_charge (edict_t *self, float dist);
int32_t range (edict_t *self, edict_t *other);

void FoundTarget (edict_t *self);
bool infront (edict_t *self, edict_t *other);
bool visible (edict_t *self, edict_t *other);
bool FacingIdeal(edict_t *self);

bool M_CheckBottom(edict_t* ent);
bool M_walkmove(edict_t* ent, float yaw, float dist);
void M_MoveToGoal(edict_t* ent, float dist);
void M_ChangeYaw(edict_t* ent);


//
// Ammo_*.c
//

void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin);
bool Ammo_Melee (edict_t *self, vec3_t aim, int32_t damage, int32_t kick);
void Ammo_Bullet (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t mod);
void Ammo_Bullet_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t kick, int32_t hspread, int32_t vspread, int32_t count, int32_t mod);
void Ammo_Blaster (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, int32_t effect, bool hyper);
void Ammo_Grenade (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, float timer, float damage_radius);
void Ammo_Grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held);
void Ammo_Rocket (edict_t *self, vec3_t start, vec3_t dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage);
void Ammo_Rail (edict_t *self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t kick);
void Ammo_Bamfuslicator (edict_t* self, vec3_t start, vec3_t aimdir, zombie_type zombie_type);
void Ammo_Tangfuslicator (edict_t* self, vec3_t start, vec3_t aimdir); //todo: recoil

//
// game_player_trail.c
//
void PlayerTrail_Init (void);
void PlayerTrail_Add (vec3_t spot);
void PlayerTrail_New (vec3_t spot);
edict_t *PlayerTrail_PickFirst (edict_t *self);
edict_t *PlayerTrail_PickNext (edict_t *self);
edict_t	*PlayerTrail_LastSpot (void);

//
// game_client_*.c
//
void respawn (edict_t *ent);
void BeginIntermission (edict_t *targ);
void PutClientInServer (edict_t *ent);
void InitClientPersistent (edict_t *client_edict);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientBeginServerFrame (edict_t *ent);

//
// files that use mob_player.h
//
void player_pain (edict_t *self, edict_t *other, float kick, int32_t damage);
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int32_t damage, vec3_t point);

//
// game_cmds_server.c
//
void	ServerCommand (void);
bool	SV_FilterPacket (char *from);

//
// game_client_view.c
//
void ClientEndServerFrame (edict_t *ent);

//
// game_ui.c
//
void MoveClientToIntermission (edict_t *ent, player_team winning_team);
void G_SetStats (edict_t *ent);
void G_SetSpectatorStats (edict_t *ent);
void G_CheckChaseStats (edict_t *ent);

//
// weapon_base.c
//
void PlayerNoise(edict_t *who, vec3_t where, int32_t type);

//
// physics_base.c
//
void G_RunEntity (edict_t *ent);

//
// game_save.c
//
void SaveClientData (void);
void FetchClientEntData (edict_t *ent);

//
// game_chase_camera.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

//============================================================================

// client_t->anim_priority
#define	ANIM_BASIC		0		// stand / run
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6

// client data that stays across multiple levels
typedef struct client_persistant_s
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[PLAYER_NAME_LENGTH];
	int32_t		hand;

	bool		connected;			// a loadgame will leave valid entities that
									// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int32_t		health;
	int32_t		max_health;
	int32_t		savedFlags;

	int32_t		selected_item;

	// ammo capacities
	int32_t		max_bullets;
	int32_t		max_shells;
	int32_t		max_rockets;
	int32_t		max_grenades;
	int32_t		max_cells;
	int32_t		max_slugs;

	gitem_t		*weapon;
	gitem_t		*lastweapon;

	//restore this if we have resumable co-op games or that 6-level single player campaign.
	//int32_t		coop_score;

	int32_t		game_helpchanged;
	int32_t		helpchanged;

	bool		spectator;			// client is a spectator
} client_persistant_t;

// client data that stays across respawns
typedef struct client_respawn_t
{
	client_persistant_t	coop_respawn;	// what to set client->pers to on a respawn
	int32_t		enterframe;			// level.framenum the client entered the game
	int32_t		score;				// frags, etc
	int32_t		score_monsters;		// Monster score
	vec3_t		cmd_angles;			// angles sent over in the last command

	bool	spectator;			// client is a spectator
} client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
typedef struct gclient_s
{
	// known to server
	player_state_t	ps;				// communicated by server to clients
	int32_t			ping;

	// private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	// for detecting out-of-pmove changes

	loadout_t	loadout;
	loadout_entry_t* loadout_current_weapon;
	loadout_entry_t* loadout_current_ammo;

	bool		showhelp;
	bool		showhelpicon;

	int32_t		buttons;
	int32_t		oldbuttons;
	int32_t		latched_buttons;

	bool		weapon_thunk;

	gitem_t*	newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int32_t		damage_armor;		// damage absorbed by armor
	int32_t		damage_parmor;		// damage absorbed by power armor
	int32_t		damage_blood;		// damage taken out of health
	int32_t		damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponstate_t	weaponstate;
	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;		// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	float		next_drown_time;
	int32_t		old_waterlevel;
	int32_t		breather_sound;

	int32_t		machinegun_shots;	// for weapon raising

	// animation vars
	int32_t		anim_end;
	int32_t		anim_priority;
	bool		anim_duck;
	bool		anim_run;

	// powerup timers
	float		quad_framenum;
	float		invincible_framenum;
	float		breather_framenum;
	float		enviro_framenum;

	bool		grenade_blew_up;
	float		grenade_time;
	int32_t		silencer_shots;
	int32_t		weapon_sound;

	float		pickup_msg_time;

	float		flood_locktill;		// locked from talking
	float		flood_when[10];		// when messages were said
	int32_t		flood_whenhead;		// head pointer for when said

	float		respawn_time;		// can respawn when time > this

	edict_t		*chase_target;		// player we are chasing
	bool		update_chase;		// need to update chase info? 
} gclient_t;

struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;	// NULL if not a player
									// the server expects the first part
									// of gclient_s to be a player_state_t
									// but the rest of it is opaque
	bool		inuse;
	int32_t		linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf
	
	int32_t		num_clusters;		// if -1, use headnode instead
	int32_t		clusternums[MAX_ENT_CLUSTERS];
	int32_t		headnode;			// unused if num_clusters != -1
	int32_t		areanum, areanum2;

	//================================

	int32_t		svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int32_t		clipmask;
	edict_t*	owner;

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	player_team	team;

	//================================
	int32_t		movetype;
	int32_t		flags;

	char*		model;
	float		freetime;			// sv.time when the object was freed
	
	//
	// only used locally in game, not by server
	//
	char*		message;
	char*		classname;
	int32_t		spawnflags;

	float		timestamp;

	float		angle;			// set in qe3, -1 = up, -2 = down
	char*		target;
	char*		targetname;
	char*		killtarget;

	char*		pathtarget;
	char*		deathtarget;
	char*		combattarget;
	edict_t*	target_ent;

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2;

	vec3_t		velocity;
	vec3_t		avelocity;
	int32_t		mass;
	float		air_finished;
	float		gravity;		// per entity gravity multiplier (1.0 is normal)
								// use for lowgrav artifact, flares

	edict_t*	goalentity;
	edict_t*	movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	float		nextthink;
	void		(*prethink) (edict_t *ent);
	void		(*think)(edict_t *self);
	void		(*blocked)(edict_t *self, edict_t *other);	//move to moveinfo?
	void		(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void		(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void		(*pain)(edict_t *self, edict_t *other, float kick, int32_t damage);
	void		(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int32_t damage, vec3_t point);

	float		touch_debounce_time;		// are all these legit?  do we need more/less of them?
	float		pain_debounce_time;
	float		damage_debounce_time;
	float		fly_sound_debounce_time;	//move to clientinfo
	float		last_move_time;

	int32_t		health;
	int32_t		max_health;
	int32_t		gib_health;
	int32_t		deadflag;
	bool		show_hostile;

	float		powerarmor_time;

	char		*map;			// target_changelevel

	int32_t		viewheight;		// height above origin where eyesight is determined
	int32_t		takedamage;
	int32_t		dmg;
	int32_t		radius_dmg;
	float		dmg_radius;
	int32_t		sounds;			//make this a spawntemp var?
	int32_t		count;

	edict_t		*chain;
	edict_t		*enemy;
	edict_t		*oldenemy;
	edict_t		*activator;
	edict_t		*groundentity;
	int32_t		groundentity_linkcount;
	edict_t		*teamchain;
	edict_t		*teammaster;

	edict_t		*mynoise;		// can go in client only
	edict_t		*mynoise2;

	int32_t		noise_index;
	int32_t		noise_index2;
	float		volume;
	float		attenuation;

	int32_t		jump_height; // for func_trampoline

	// timing variables
	float		wait;
	float		delay;			// before firing targets
	float		random;

	float		teleport_time;

	int32_t		watertype;
	int32_t		waterlevel;

	vec3_t		move_origin;
	vec3_t		move_angles;

	int32_t		allowed_teams;	// Allowed Teams for Zombono Teamdoors

	// move this to clientinfo?
	int32_t		light_level;

	int32_t		style;			// also used as areaportal number

	gitem_t		*item;			// for bonus items

	// common data blocks
	moveinfo_t		moveinfo;
	monsterinfo_t	monsterinfo;
};


// /weapons/weapon_base.c

void P_ProjectSource(edict_t* ent, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void PlayerNoise(edict_t* who, vec3_t where, int32_t type);
bool Pickup_Weapon(edict_t* ent, edict_t* other);
void ChangeWeapon(edict_t* ent);
void NoAmmoWeaponChange(edict_t* ent);
void Think_Weapon(edict_t* ent);
void Use_Weapon(edict_t* ent, gitem_t* item);
void Drop_Weapon(edict_t* ent, gitem_t* item);
void Weapon_Generic(edict_t* ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST,
	int32_t* pause_frames, int32_t* fire_frames_primary, int32_t* fire_frames_secondary, void (*fire_primary)(edict_t* ent), void(*fire_secondary)(edict_t* ent));

// game_loadout.c
loadout_entry_t* Loadout_AddItem(edict_t* ent, const char* item_name, const char* icon, loadout_entry_type type, int32_t amount);		// Adds the loadout item with the name name, the icon icon and the amount amount. Returns the new loadout item
void Loadout_DeleteItem(edict_t* ent, const char* item_name);				// Deletes the loadout item with the name name.
loadout_entry_t* Loadout_GetItem(edict_t* ent, const char* item_name);		// Returns the loadout item with the name item_name.
bool Loadout_EntryIsItem(loadout_entry_t* entry, const char* item_name);	// Determines if the loadout entry entry is the item with the name item_name.