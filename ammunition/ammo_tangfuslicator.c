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
#include <entities/entity_base.h>

#define TANGFUSLICATOR_MAX_DISTANCE	1536
#define TANGFUSLICATOR_NON_HUMAN_DAMAGE_PERCENT 0.15

edict_t* do_not_zombify;

void Ammo_Tangfuslicator_Touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	edict_t* zombie;

	// so you can't zombify yourself, inuse check is so the game doesn't crash if you leave while being shot
	if (do_not_zombify != NULL
		&& do_not_zombify->inuse
		&& other == do_not_zombify)
	{
		return; 
	}

	if (!strncmp(other->classname, "player", 6))
	{
		if (!((int32_t)gameflags->value & GF_ITEM_FRIENDLY_FIRE)
			&& other->team != team_player)
		{
			// TODO: PLAY SOUND
			gi.cprintf(do_not_zombify, PRINT_CHAT, "Can't zombify a director!");
			return;
		}

		// spawn a zombie where the player is starting
		
		// spawn the zombie
		zombie = G_Spawn();

		// move the zombie to where the player spawned it
		// the zombie is on director team (this is used so they don't harm directors unless the requisite gameflag is set)
		zombie->team = team_director;

		VectorCopy(self->s.origin, zombie->s.origin);

		// make him upright
		zombie->s.angles[0] = 0;
		zombie->s.angles[1] = 0;
		zombie->s.angles[2] = 0;

		SP_monster_zombie(zombie);

		// kill the fucker
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 200, 0, 0, MOD_ZOMBIFIED);
	}
	else
	{
		// can't hurt zombies
		if (strstr(other->classname, "zombie"))
			return;

		// hurt other enemies by TANGFUSLICATOR_NON_HUMAN_DAMAGE_PERCENT of their health if we accidentlaly hit them
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, other->max_health * TANGFUSLICATOR_NON_HUMAN_DAMAGE_PERCENT, 0, 0, MOD_ZOMBIE);
	}
}

// ammo_tangfuslicator.c: Code for the Lightning Gun thing...
void Ammo_Tangfuslicator(edict_t* self, vec3_t trace_start, vec3_t aimdir)
{
	edict_t* lightning_bolt;
	vec3_t dir = { 0 };
	vec3_t forward = { 0 }, right = { 0 }, up = { 0 };
	// the velocity of the lightning
	float lightning_velocity = 1750;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	// do the trace for the particles
	vec3_t trace_end = { 0 };
	trace_t trace_result = { 0 }; 
	trace_end[0] = trace_start[0] + aimdir[0] * TANGFUSLICATOR_MAX_DISTANCE;
	trace_end[1] = trace_start[1] + aimdir[1] * TANGFUSLICATOR_MAX_DISTANCE;
	trace_end[2] = trace_start[2] + aimdir[2] * TANGFUSLICATOR_MAX_DISTANCE;

	// trace
	trace_result = gi.trace(trace_start, NULL, NULL, trace_end, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW | CONTENTS_WATER); // maybe remove water?

	// spawn the (invisible) bolt
	lightning_bolt = G_Spawn();
	lightning_bolt->classname = "ammo_tangfuslicator";
	lightning_bolt->movetype = MOVETYPE_FLYMISSILE; // is this good?
	lightning_bolt->nextthink = level.time + (TANGFUSLICATOR_MAX_DISTANCE / lightning_velocity);
	lightning_bolt->think = G_FreeEdict;
	lightning_bolt->clipmask = MASK_SHOT;
	lightning_bolt->s.effects |= EF_LIGHTNING; // see cl_ents.c
	VectorCopy(trace_start, lightning_bolt->s.origin);
	VectorCopy(aimdir, lightning_bolt->s.angles);

	VectorScale(aimdir, lightning_velocity, lightning_bolt->velocity);
	lightning_bolt->solid = SOLID_BBOX;
	lightning_bolt->touch = Ammo_Tangfuslicator_Touch;

	// since the only visual indication is the lightning particles
	// player edict size
	VectorSet(lightning_bolt->mins, -16, -16, -24);
	VectorSet(lightning_bolt->maxs, 16, 16, 32);

	do_not_zombify = self;

	// go
	gi.linkentity(lightning_bolt);
}