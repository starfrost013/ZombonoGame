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

// ammo_bamfuslicator.c: Code for the Director team's bamfuslicator weapon ammo (split from g_weapon.c)
// WHERE IS THE FUCKING AMMUNITION?

#define BAMFUSLICATOR_MIN_DISTANCE		64		// so you don't get stuck
#define BAMFUSLICATOR_MAX_DISTANCE		768

/* Bamfuslicator */
void Ammo_Bamfuslicator(edict_t* self, vec3_t start, vec3_t aimdir, zombie_type zombie_type)
{
	// Sorry for anyone reading this code

	edict_t*	monster = { 0 };
	edict_t*	within_player_bounds[MAX_EDICTS] = { 0 };
	edict_t*	within_monster_bounds[MAX_EDICTS] = { 0 };
	trace_t		trace = { 0 };
	vec3_t		trace_start = { 0 };
	vec3_t		trace_end = { 0 };
	vec3_t		vec_absmax = { 0 }, vec_absmin = { 0 };
	// distance AROUND the hitbox to check. for some reason just having this doesn't work and you need the other "rollback" thing later or you can still spawn in walls
	vec3_t		min_dist = { 12, 12, 12 };

	trace_start[0] = start[0];
	trace_start[1] = start[1];
	trace_start[2] = start[2];

	// set up positions 
	trace_end[0] = trace_start[0] + (aimdir[0] * BAMFUSLICATOR_MAX_DISTANCE);
	trace_end[1] = trace_start[1] + (aimdir[1] * BAMFUSLICATOR_MAX_DISTANCE);
	trace_end[2] = trace_start[2] + (aimdir[2] * BAMFUSLICATOR_MAX_DISTANCE);

	// raycast from where we fired the weapon
	// check if we hit somethoing
	trace = gi.trace(trace_start, NULL, NULL, trace_end, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW | CONTENTS_WATER); // zombies don't like water!

	// rollback the raycast by a tiny amount because the visual hitbox and the hitbox required to have fun are not the same
	// horrifying hacks

	// avoid divide by zero
	if (trace.endpos[0] == 0) trace.endpos[0] = 0.001;
	if (trace.endpos[1] == 0) trace.endpos[1] = 0.001;

	if (aimdir[0] > 0)
	{
		trace.endpos[0] -= aimdir[0] * (16 / aimdir[0]);
	}
	else
	{
		trace.endpos[0] += aimdir[0] * (16 / aimdir[0]);
	}

	if (aimdir[1] > 0)
	{
		trace.endpos[1] -= aimdir[1] * (16 / aimdir[1]);
	}
	else
	{
		trace.endpos[1] += aimdir[1] * (16 / aimdir[1]);
	}

	// don't get the monster stuck in the wall while also not being able to spawn through ceilings, so we push DOWN (positive-Y) if we are looking above the horizon,
	// and UP 32 units (multiplied by aimdir[2] in this case, if you multiply up by aimdir[2] you get zombies half stuck in the wall)...this seems to give best results
	// why is this code so bad
	if (aimdir[2] < 0)
	{
		trace.endpos[2] += 32;
	}
	else
	{
		trace.endpos[2] -= 32*aimdir[2];
	}

	// completed, too far away to spawn a monster
	if (trace.fraction == 1.0f)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bamfuslicator/spawn_failed.wav"), 1, ATTN_NORM, 0);
		return;
	}

	// can't spawn if inside a player
	if (trace.ent != NULL)
	{
		if (!strncmp(trace.ent->classname, "player", 6))
		{
			gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bamfuslicator/spawn_failed.wav"), 1, ATTN_NORM, 0);
			G_FreeEdict(monster);

			return;
		}

	}

	// spawn the monster
	monster = G_Spawn();

	// move the zombie to where the player spawned it
	// the zombie is on director team (this is used so they don't harm directors unless the requisite gameflag is set)
	monster->team = team_director;
	VectorCopy(trace.endpos, monster->s.origin);
	VectorCopy(aimdir, monster->s.angles);

	switch (zombie_type)
	{
	case zombie_type_normal:
		SP_monster_zombie(monster);
		break;
	case zombie_type_fast:
		SP_monster_zombie_fast(monster);
		break;
	case zombie_type_ogre:
		SP_monster_ogre(monster);
		break;
	}

	VectorSubtract(self->absmin, min_dist, vec_absmin);
	VectorAdd(self->absmax, min_dist, vec_absmax);

	// see if we are trying to spawn inside of the player
	// 64 to limit time this function takes as it's recursive (also would there really be more than 64 in a 64x64 box around the player???)
	int32_t num_within_player_bounds = gi.BoxEdicts( vec_absmin, vec_absmax, &within_player_bounds, 64, AREA_SOLID);

	for (int32_t edict = 0; edict < num_within_player_bounds; edict++)
	{
		if (!strncmp(within_player_bounds[edict]->classname, "monster_", 8)) // check for any monster
		{
			gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bamfuslicator/spawn_failed.wav"), 1, ATTN_NORM, 0);
			//todo: push out
			G_FreeEdict(monster);
			return;
		}
	}

	// see if we are trying to spawn inside of a wall
	VectorSubtract(monster->absmin, min_dist, vec_absmin);
	VectorAdd(monster->absmax, min_dist, vec_absmax);

	// now see if we are tyrign 
	int32_t num_within_monster_bounds = gi.BoxEdicts(vec_absmin, vec_absmax, &within_monster_bounds, 64, AREA_SOLID);

	for (int32_t edict = 0; edict < num_within_monster_bounds; edict++)
	{
		if (!strncmp(within_monster_bounds[edict]->classname, "worldspawn", 11))
		{
			gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bamfuslicator/spawn_failed.wav"), 1, ATTN_NORM, 0);
			G_FreeEdict(monster);
			return;
		}
	}

	// we finally succeded
	// spawn some nice particles
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT);
	gi.WritePosition(trace.endpos);

	gi.sound(self, CHAN_VOICE, gi.soundindex("zombie/zombie_spawn.wav"), 1, ATTN_NORM, 0);
	//gi.WriteDir(zombie->s.angles);
}