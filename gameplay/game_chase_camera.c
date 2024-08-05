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

void ChaseCam_Update(edict_t* ent)
{
	vec3_t o, ownerv, goal;
	edict_t* targ;
	vec3_t forward, right;
	trace_t trace;
	int32_t i;
	vec3_t oldgoal;
	vec3_t angles;

	// is our chase target gone?
	if (!ent->client->chase_target->inuse
		|| ent->client->chase_target->client->resp.spectator)
	{
		edict_t* old = ent->client->chase_target;
		ChaseCam_Next(ent);

		if (ent->client->chase_target == old)
		{
			ent->client->chase_target = NULL;
			ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			return;
		}
	}

	targ = ent->client->chase_target;

	VectorCopy3(targ->s.origin, ownerv);
	VectorCopy3(ent->s.origin, oldgoal);

	ownerv[2] += targ->viewheight;

	VectorCopy3(targ->client->v_angle, angles);
	if (angles[PITCH] > 56)
		angles[PITCH] = 56;
	AngleVectors(angles, forward, right, NULL);
	VectorNormalize3(forward);
	VectorMA3(ownerv, -30, forward, o);

	if (o[2] < targ->s.origin[2] + 20)
		o[2] = targ->s.origin[2] + 20;

	// jump animation lifts
	if (!targ->groundentity)
		o[2] += 16;

	trace = gi.trace(ownerv, vec3_origin, vec3_origin, o, targ, MASK_SOLID);

	VectorCopy3(trace.endpos, goal);

	VectorMA3(goal, 2, forward, goal);

	// pad for floors and ceilings
	VectorCopy3(goal, o);
	o[2] += 6;
	trace = gi.trace(goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID);
	if (trace.fraction < 1) {
		VectorCopy3(trace.endpos, goal);
		goal[2] -= 6;
	}

	VectorCopy3(goal, o);
	o[2] -= 6;
	trace = gi.trace(goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID);
	if (trace.fraction < 1) {
		VectorCopy3(trace.endpos, goal);
		goal[2] += 6;
	}

	if (targ->deadflag)
		ent->client->ps.pmove.pm_type = PM_DEAD;
	else
		ent->client->ps.pmove.pm_type = PM_FREEZE;

	VectorCopy3(goal, ent->s.origin);

	for (i = 0; i < 3; i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(targ->client->v_angle[i] - ent->client->resp.cmd_angles[i]);

	if (targ->deadflag) {
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	}
	else
	{
		VectorCopy3(targ->client->v_angle, ent->client->ps.viewangles);
		VectorCopy3(targ->client->v_angle, ent->client->v_angle);
	}

	ent->viewheight = 0;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.Edict_Link(ent);
}

void ChaseCam_Next(edict_t* ent)
{
	int32_t i;
	edict_t* e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do
	{
		i++;

		if (i > maxclients->value)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!e->client->resp.spectator)
			break;
	} 
	while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void ChaseCam_Prev(edict_t* ent)
{
	int32_t i;
	edict_t* e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i--;

		if (i < 1)
			i = maxclients->value;

		e = g_edicts + i;

		if (!e->inuse)
			continue;

		if (!e->client->resp.spectator)
			break;
	} 
	while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void ChaseCam_GetTarget(edict_t* ent)
{
	int32_t i;
	edict_t* other;

	for (i = 1; i <= maxclients->value; i++)
	{
		other = g_edicts + i;
		if (other->inuse && !other->client->resp.spectator) 
		{
			ent->client->chase_target = other;
			ent->client->update_chase = true;
			ChaseCam_Update(ent);
			return;
		}
	}
	gi.centerprintf(ent, "No other players to chase.");
}

