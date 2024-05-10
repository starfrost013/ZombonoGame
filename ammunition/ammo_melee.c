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

// ammo_melee.c: Code for melee attacks (split from g_weapon.c)

/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
bool Ammo_Melee(edict_t* self, vec3_t attack_radius, int32_t damage, int32_t kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	//see if enemy is in range
	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);
	if (range > attack_radius[0])
		return false;

	if (attack_radius[1] > self->mins[0] && attack_radius[1] < self->maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (attack_radius[1] < 0)
			attack_radius[1] = self->enemy->mins[0];
		else
			attack_radius[1] = self->enemy->maxs[0];
	}

	VectorMA(self->s.origin, range, dir, point);

	tr = gi.trace(self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA(self->s.origin, range, forward, point);
	VectorMA(point, attack_radius[1], right, point);
	VectorMA(point, attack_radius[2], up, point);
	VectorSubtract(point, self->enemy->s.origin, dir);

	// do the damage
	T_Damage(tr.ent, self, self, dir, point, vec3_origin, damage, kick / 2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// do our special form of knockback here
	VectorMA(self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract(v, point, v);
	VectorNormalize(v);
	VectorMA(self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}
