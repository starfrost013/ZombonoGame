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

// game_client_spawn.c: Spawn code

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistent(edict_t* client_edict)
{
	gitem_t* item;

	gclient_t* client = client_edict->client;

	// fail
	if (client == NULL)
	{
		Sys_Error("Tried to InitClientPersistent a non-client!");
		return;
	}

	memset(&client->pers, 0, sizeof(client->pers));

	// Give them the default weapon!

	GiveBaseWeaponForTeam(client_edict);

	client->pers.health = 100;
	client->pers.max_health = 100;

	client->pers.max_bullets = 200;
	client->pers.max_shells = 100;
	client->pers.max_rockets = 50;
	client->pers.max_grenades = 50;
	client->pers.max_cells = 200;
	client->pers.max_slugs = 50;

	client->pers.connected = true;
}


void GiveBaseWeaponForTeam(edict_t* client_edict)
{
	gclient_t* client = client_edict->client;

	if (client_edict->team == team_director)
	{
		// add the bamfuslicator
		client->newweapon = FindItem("Director - Bamfuslicator");
		Loadout_AddItem(client_edict, client->newweapon->pickup_name, client->newweapon->icon, loadout_entry_type_weapon, 1);
		
		// and the tangfuslicator...
		//gitem_t* tangfuslicator = FindItem("Director - Tangfuslicator");
		//Loadout_AddItem(client_edict, tangfuslicator->pickup_name, tangfuslicator->icon, loadout_entry_type_weapon, 1);

		ChangeWeapon(client_edict);

	}
	else
	{
		client->newweapon = FindItem("Blaster");
		Loadout_AddItem(client_edict, client->newweapon->pickup_name, client->newweapon->icon, loadout_entry_type_weapon, 1);
		ChangeWeapon(client_edict);
	}
}

void InitClientResp(gclient_t* client)
{
	memset(&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData(void)
{
	int		i;
	edict_t* ent;

	for (i = 0; i < game.maxclients; i++)
	{
		ent = &g_edicts[1 + i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags = (ent->flags & (FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR));
	}
}

void FetchClientEntData(edict_t* ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/

float	PlayersRangeFromSpot(edict_t* spot)
{
	edict_t* player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract(spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength(v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}


/*
===========
SelectSpawnPoint

Chooses a player start for zombono debugging
============
*/
edict_t* SelectUnassignedSpawnPoint()
{
	edict_t* spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL)
	{
		if (!game.spawnpoint[0] && !spot->targetname)
			return spot;

		if (!game.spawnpoint[0] || !spot->targetname)
			continue;

		if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
			return spot;
	}

	if (!spot)
	{
		if (!game.spawnpoint[0])
		{	// there wasn't a spawnpoint without a target, so use any
			spot = G_Find(spot, FOFS(classname), "info_player_start");
		}

		if (!spot)
		{
			gi.error("Couldn't find spawn point %s\n", game.spawnpoint);
		}

		return spot;
	}

	return NULL;
}

/*
================
SelectRandomSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t* SelectRandomSpawnPoint(char* spawn_class_name)
{
	edict_t* spot, * spot1, * spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find(spot, FOFS(classname), spawn_class_name)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	// if we failed, try unasigned
	if (!count)
	{
		gi.bprintf(PRINT_ALL, "Failed to spawn %s (mode: random), trying unassigned", spawn_class_name);
		return SelectUnassignedSpawnPoint();
	}

	// is it one of the two closest to other players?
	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find(spot, FOFS(classname), spawn_class_name);
		if (spot == spot1 || spot == spot2)
			selection++;
	} while (selection--);

	return spot;
}

/*
================
SelectFarthestSpawnPoint
================
*/
edict_t* SelectFarthestSpawnPoint(char* spawn_class_name)
{
	edict_t* bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t* spot;

	if (spawn_class_name == NULL)
	{
		gi.error("Tried to spawn a NULL classname");
		return NULL;
	}

	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find(spot, FOFS(classname), spawn_class_name)) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
		return bestspot;

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find(NULL, FOFS(classname), spawn_class_name);

	// still null? try unassigned
	if (spot == NULL)
	{
		gi.bprintf(PRINT_ALL, "Failed to spawn %s (mode: furthest), trying unassigned", spawn_class_name);
		return SelectUnassignedSpawnPoint();
	}

	return spot;
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

/*
===========
SelectSpawnPoint

Chooses a player start, gamemode-specific start, etc
============
*/
void	SelectSpawnPoint(edict_t* ent, vec3_t origin, vec3_t angles)
{
	edict_t* spot = NULL;

	// TODO: Change this for zombono.
	if (gamemode->value == GAMEMODE_TDM)
	{
		spot = SelectTeamSpawnPoint(ent);
	}
	else
	{
		gi.bprintf(PRINT_DEVELOPER, "Tried to spawn for unimplemented gamemode %d! Trying unassigned...", gamemode->value);
		spot = SelectUnassignedSpawnPoint();
	}

	// find a single player start spot
	if (!spot)
	{
		spot = SelectUnassignedSpawnPoint();
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
}


//======================================================================


void InitBodyQue(void)
{
	int		i;
	edict_t* ent;

	level.body_que = 0;
	for (i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

void body_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	int	n;

	if (self->health < -40)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self->s.origin[2] -= 48;
		ThrowClientHead(self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

void CopyToBodyQue(edict_t* ent)
{
	edict_t* body;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int32_t)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity(ent);

	gi.unlinkentity(body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;
	VectorCopy(ent->mins, body->mins);
	VectorCopy(ent->maxs, body->maxs);
	VectorCopy(ent->absmin, body->absmin);
	VectorCopy(ent->absmax, body->absmax);
	VectorCopy(ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity(body);
}


void respawn(edict_t* self)
{
	// spectator's don't leave bodies
	if (self->movetype != MOVETYPE_NOCLIP)
		CopyToBodyQue(self);
	self->svflags &= ~SVF_NOCLIENT;
	PutClientInServer(self);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;

	return;
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn(edict_t* ent)
{
	int32_t i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator) {
		char* value = Info_ValueForKey(ent->client->pers.userinfo, "spectator");
		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	}
	else {
		// he was a spectator and wants to join the game
		// he must have the right password
		char* value = Info_ValueForKey(ent->client->pers.userinfo, "password");
		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent->client->resp.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer(ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator) {
		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator)
		gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	else
		gi.bprintf(PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}

//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(edict_t* ent)
{
	vec3_t	mins = { -16, -16, -24 };
	vec3_t	maxs = { 16, 16, 32 };
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t* client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;

	// TEMPORARY HACK FOR PLAYTEST - TODO: THIS *WILL* BREAK FOR EXISTING CLIENTS IF THE TIMELIMIT OR FRAGLIMIT IS CHANGED AFTER SERVER CREATION UNTIL YOU RESPAWN

	if (timelimit->value)
	{
		G_UISend(ent, "TimeUI", true, false, true);
	}

	G_UISend(ent, "ScoreUI", true, false, true);

	// HACK: THIS MUST BE THE LAST ONE OTHERWISE IT WILL NOT BE SET AS THE CURRENT UI AND YOU CAN'T SPAWN
	G_UISend(ent, "TeamUI", true, true, true);

	// tell the client to wipe its loadout information
	gi.WriteByte(svc_loadout_clear);
	gi.unicast(ent, true);

	// every player starts out as unassigned
	ent->team = team_unassigned;

	// tell the player to spawn;
	// start by finding a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);

	index = ent - g_edicts - 1;
	client = ent->client;

	char		userinfo[MAX_INFO_STRING];

	resp = client->resp;
	memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
	InitClientPersistent(ent);
	ClientUserinfoChanged(ent, userinfo);

	// clear everything but the persistant data
	saved = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved;
	if (client->pers.health <= 0)
		InitClientPersistent(ent);
	client->resp = resp;

	// copy some data from the client to the entity
	FetchClientEntData(ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);
	VectorClear(ent->velocity);

	// clear playerstate values
	memset(&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0];
	client->ps.pmove.origin[1] = spawn_origin[1];
	client->ps.pmove.origin[2] = spawn_origin[2];

	client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
	if (client->ps.fov < 1)
		client->ps.fov = 90;
	else if (client->ps.fov > 160)
		client->ps.fov = 160;

	client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);

	// clear entity state values
	ent->s.effects = 0;
	ent->s.modelindex = 255;		// will use the skin specified model
	ent->s.modelindex2 = 255;		// custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent->s.skinnum = ent - g_edicts - 1;

	ent->s.frame = 0;
	VectorCopy(spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy(ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i = 0; i < 3; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);
	}

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy(ent->s.angles, client->ps.viewangles);
	VectorCopy(ent->s.angles, client->v_angle);

	// spawn a spectator
	if (client->pers.spectator) {
		client->chase_target = NULL;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity(ent);
		return;
	}
	else
		client->resp.spectator = false;

	if (!KillBox(ent))
	{	// could't spawn in?
	}

	gi.linkentity(ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon(ent);
}

/* Dummy spawns go HERE */

void SP_misc_teleporter_dest(edict_t* ent);

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t* self)
{

}

/* Zombono director team start. */
void SP_info_player_start_director(edict_t* self)
{

}

/* Zombono player team start. */
void SP_info_player_start_player(edict_t* self)
{

}

/* Team door
Must be implemented at runtime as there is no way during BSP compile process to know what teams there are*/
void SP_info_team_door(edict_t* self)
{

}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{

}