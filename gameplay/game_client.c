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

uint32_t CheckBlock(void* b, int32_t c)
{
	int	v, i;
	v = 0;
	for (i = 0; i < c; i++)
		v += ((uint8_t*)b)[i];
	return v;
}

void PrintPmove(pmove_t* pm)
{
	uint32_t	c1, c2;

	c1 = CheckBlock(&pm->s, sizeof(pm->s));
	c2 = CheckBlock(&pm->cmd, sizeof(pm->cmd));
	Com_Printf("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink(edict_t* ent, usercmd_t* ucmd)
{
	gclient_t* client;
	edict_t* other;
	int		i, j;
	pmove_t	pm;

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
	else {

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

		// set team speed
		if (ent->team == team_director)
		{
			pm.cmd.forwardmove *= SPEED_DIRECTOR;
			pm.cmd.sidemove *= SPEED_DIRECTOR;
		}
		else if (ent->team == team_player)
		{
			pm.cmd.forwardmove *= SPEED_PLAYER;
			pm.cmd.sidemove *= SPEED_PLAYER;
		}

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		// perform a pmove
		gi.Pmove(&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i = 0; i < 3; i++)
		{
			ent->s.origin[i] = pm.s.origin[i];
			ent->velocity[i] = pm.s.velocity[i];
		}

		VectorCopy(pm.mins, ent->mins);
		VectorCopy(pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
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
			VectorCopy(pm.viewangles, client->v_angle);
			VectorCopy(pm.viewangles, client->ps.viewangles);
		}

		gi.linkentity(ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers(ent);

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
		if (client->resp.spectator) {

			client->latched_buttons = 0;

			if (client->chase_target) {
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
				GetChaseTarget(ent);

		}
		else if (!client->weapon_thunk) {
			client->weapon_thunk = true;
			Think_Weapon(ent);
		}
	}

	if (client->resp.spectator) {
		if (ucmd->upmove >= 10) {
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++) {
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}
}



/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t* ent)
{
	gclient_t* client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	if (client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5) {
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)
		Think_Weapon(ent);
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
				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!visible(ent, PlayerTrail_LastSpot()))
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
void ClientEndServerFrame(edict_t* ent)
{
	float	bobtime;
	int		i;

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
		G_SetStats(ent);
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects();

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
	ent->s.angles[ROLL] = SV_CalcRoll(ent->s.angles, ent->velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrt(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

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
	bobfracsin = fabs(sin(bobtime * M_PI));

	// detect hitting the floor
	P_FallingDamage(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// determine the view offsets
	SV_CalcViewOffset(ent);

	// determine the gun offsets
	SV_CalcGunOffset(ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend(ent);

	// chase cam stuff
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats(ent);
	G_CheckChaseStats(ent);

	G_SetClientEvent(ent);

	G_SetClientEffects(ent);

	G_SetClientSound(ent);

	G_SetClientFrame(ent);

	VectorCopy(ent->velocity, ent->client->oldvelocity);
	VectorCopy(ent->client->ps.viewangles, ent->client->oldviewangles);

	// clear weapon kicks
	VectorClear(ent->client->kick_origin);
	VectorClear(ent->client->kick_angles);

	// update the leaderboard every 10 ticks (1 second)
	// BEFORE IT WAS UPDATING IT EVERY FRAME WHILE ACTIVE???
	if ((level.framenum % (int)(1 / FRAMETIME)) == 0)
	{
		G_LeaderboardSend(ent);
	}
}