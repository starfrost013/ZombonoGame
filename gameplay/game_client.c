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

// game_client.c: Main client loop

edict_t* pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}


void Client_UpdateCamera(edict_t* ent, usercmd_t* ucmd)
{
	edict_t* other;

	switch (ent->client->ps.camera_type)
	{
		// this is the normal camera type that is not handed by anything
	case camera_type_normal:
		VectorCopy3(ent->s.origin, ent->client->ps.vieworigin);
		return; 
		// this is separate to spectator mode
	case camera_type_chase:

		// update chase cam if being followed
		for (int32_t i = 1; i <= sv_maxclients->value; i++)
		{
			other = g_edicts + i;
			if (other->inuse && other->client->chase_target == ent)
				ChaseCam_Update(other);
		}
		return;
	case camera_type_topdown:
		//todo: cvar?
		vec3_t addition = { 0, 0, 150 };//temp
		vec3_t viewangle = { 0, 0, -180 };
		VectorAdd3(ent->client->ps.pmove.origin, addition, ent->client->ps.vieworigin);
		VectorCopy3(viewangle, ent->client->ps.viewangles);
		return;
	case camera_type_free:
		Com_Printf("CAMERA_TYPE_FREE NOT IMPLEMENTED!!!!!!!!");
		return; 
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void Client_Think(edict_t* ent, usercmd_t* ucmd)
{
	gclient_t*	client;
	edict_t*	other;
	int32_t		i, j;
	pmove_t		pm;

	level.current_entity = ent;
	client = ent->client;

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if (level.time > level.intermissiontime + 5.0) //&& (ucmd->buttons & BUTTON_ANY) 
		{
			level.exitintermission = true;
			level.intermissiontime = 0;
		}

		return;
	}

	pm_passent = ent;

	if (ent->client->chase_target) {

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	}
	else 
	{
		// set up for pmove
		memset(&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		client->ps.pmove.gravity = sv_gravity->value;
		pm.s = client->ps.pmove;

		for (i = 0; i < 3; i++)
		{
			pm.s.origin[i] = ent->s.origin[i];
			pm.s.velocity[i] = ent->velocity[i];
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
			//		gi.dprintf ("pmove changed!\n");
		}

		pm.cmd = *ucmd;
		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		VectorCopy3(client->ps.vieworigin, pm.vieworigin);

		// perform a pmove
		gi.Player_Move(&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		VectorCopy3(pm.mins, ent->mins);
		VectorCopy3(pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			Player_Noise(ent, ent->s.origin, PNOISE_SELF);

			// add the relative velocity of the object we're on
			// limit it to 2x velocity
			float max_relative_velocity_factor = 1.0f;
			
			bool add_relative_velocity = false;

			// don't take into account jumping
			if (ent->groundentity->velocity != vec3_origin
				&& abs(pm.s.velocity[0]) < abs(ent->groundentity->velocity[0] * max_relative_velocity_factor)
				&& abs(pm.s.velocity[1]) < abs(ent->groundentity->velocity[1] * max_relative_velocity_factor))
			{
				add_relative_velocity = true;
			}

			// tangfuslicator jump and rocketriding
			if (!strncmp(ent->groundentity->classname, "ammo_tangfuslicator", 19)
				|| !strncmp(ent->groundentity->classname, "rocket", 6))
			{
				add_relative_velocity = true;
			}

			if (add_relative_velocity)
				VectorAdd3(pm.s.velocity, ent->groundentity->velocity, pm.s.velocity);
		}

		for (i = 0; i < 3; i++)
		{
			ent->s.origin[i] = pm.s.origin[i];
			ent->velocity[i] = pm.s.velocity[i];
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;

		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy3(pm.viewangles, client->v_angle);
			VectorCopy3(pm.viewangles, client->ps.viewangles);
		}

		gi.Edict_Link(ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			Edict_TouchTriggers(ent);

		// touch other objects
		for (i = 0; i < pm.numtouch; i++)
		{
			other = pm.touchents[i];
			for (j = 0; j < i; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch(other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK1)
	{
		if (client->resp.spectator)
		{
			client->latched_buttons = 0;

			if (client->chase_target) {
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				ChaseCam_GetTarget(ent);

		}
		else if (!client->weapon_thunk) {
			client->weapon_thunk = true;
			Weapon_Think(ent);
		}
	}

	if (client->resp.spectator)
	{
		if (ucmd->upmove >= 10)
		{
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) 
			{
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseCam_Next(ent);
				else
					ChaseCam_GetTarget(ent);
			}
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	Client_UpdateCamera(ent, ucmd);

}



/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void Client_BeginServerFrame(edict_t* ent)
{
	gclient_t*	client;
	int32_t		buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	if (client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5) {
		Client_RespawnSpectator(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)
		Weapon_Think(ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if (level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
			// TODO: investigate (-1 for all buttons)
			buttonMask = BUTTON_ATTACK1;

			if ((client->latched_buttons & buttonMask) ||
				(((int32_t)gameflags->value & GF_FORCE_RESPAWN)))
			{
				Client_Respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!Edict_CanSee(ent, PlayerTrail_LastSpot()))
		PlayerTrail_Add(ent->s.old_origin);

	client->latched_buttons = 0;
}



/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void Client_EndServerFrame(edict_t* ent)
{
	float	bobtime;
	int32_t	i;

	current_player = ent;
	current_client = ent->client;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// 
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i = 0; i < 3; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i];
		current_client->ps.pmove.velocity[i] = ent->velocity[i];
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		// FIXME: add view drifting here?
		current_client->ps.blend[3] = 0;
		current_client->ps.fov = 90;
		GameUI_SetStats(ent);
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	Player_WorldEffects();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH]) / 3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH] / 3;
	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;
	ent->s.angles[ROLL] = Client_CalcRoll(ent->s.angles, ent->velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrtf(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0;	// start at beginning of cycle again
	}
	else if (ent->groundentity)
	{	// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25;
		else if (xyspeed > 100)
			bobmove = 0.125;
		else
			bobmove = 0.0625;
	}

	bobtime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4;

	bobcycle = (int32_t)bobtime;
	bobfracsin = fabsf(sinf(bobtime * M_PI));

	// detect hitting the floor
	Player_FallDamage(ent);

	// apply all the damage taken this frame
	Player_DamageFeedback(ent);

	// determine the view offsets
	Client_CalcViewOffset(ent);

	// determine the gun offsets
	Client_CalcGunOffset(ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	Client_CalcBlend(ent);

	// chase cam stuff
	if (ent->client->resp.spectator)
		GameUI_SetStatsSpectator(ent);
	else
		GameUI_SetStats(ent);

	GameUI_CheckChaseStats(ent);

	Client_SetEvent(ent);

	Client_SetEffects(ent);

	Client_SetSound(ent);

	Client_SetFrame(ent);

	VectorCopy3(ent->velocity, ent->client->oldvelocity);
	VectorCopy3(ent->client->ps.viewangles, ent->client->oldviewangles);

	// clear weapon kicks
	VectorClear3(ent->client->kick_origin);
	VectorClear3(ent->client->kick_angles);

	// update the leaderboard every 10 ticks (1 second)
	// BEFORE IT WAS UPDATING IT EVERY TICK WHILE ACTIVE???
	if ((level.framenum % (int32_t)(1 / TICK_TIME)) == 0)
		GameUI_SendLeaderboard(ent);
}