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
// g_misc.c

#include <game_local.h>

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal(edict_t* ent, edict_t* other, edict_t* activator)
{
	ent->count ^= 1;		// toggle state
	//	gi.dprintf ("portalstate: %i = %i\n", ent->style, ent->count);
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(edict_t* ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;		// always start closed;
}

//=====================================================

/*
=================
Misc functions
=================
*/
void VelocityForDamage(int32_t damage, vec3_t v)
{
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = 200.0f + 100.0f * random();

	if (damage < 50)
		VectorScale3(v, 0.7f, v);
	else
		VectorScale3(v, 1.2f, v);
}

void ClipGibVelocity(edict_t* ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200;	// always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}


/*
=================
gibs
=================
*/
void gib_think(edict_t* self)
{
	self->s.frame++;
	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == 10)
	{
		self->think = Edict_Free;
		self->nextthink = level.time + 8 + random() * 10;
	}
}

void gib_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	vec3_t	normal_angles, right;

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);

		vectoangles(plane->normal, normal_angles);
		AngleVectors(normal_angles, NULL, right, NULL);
		vectoangles(right, self->s.angles);

		if (self->s.modelindex == snd_meat_index)
		{
			self->s.frame++;
			self->think = gib_think;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

void gib_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	Edict_Free(self);
}

void ThrowGib(edict_t* self, char* gibname, int32_t damage, int32_t type)
{
	edict_t* gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;

	gib = Edict_Spawn();

	VectorScale3(self->size, 0.5, size);
	VectorAdd3(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel(gib, gibname);
	gib->solid = SOLID_NOT;
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA3(self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random() * 600;
	gib->avelocity[1] = random() * 600;
	gib->avelocity[2] = random() * 600;

	gib->think = Edict_Free;
	gib->nextthink = level.time + 10 + random() * 10;

	gi.Edict_Link(gib);
}

void ThrowHead(edict_t* self, char* gibname, int32_t damage, int32_t type)
{
	vec3_t	vd;
	float	vscale;

	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear3(self->mins);
	VectorClear3(self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel(self, gibname);
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		self->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage(damage, vd);
	VectorMA3(self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity(self);

	self->avelocity[YAW] = crandom() * 600;

	self->think = Edict_Free;
	self->nextthink = level.time + 10 + random() * 10;

	gi.Edict_Link(self);
}


void ThrowClientHead(edict_t* self, int32_t damage)
{
	vec3_t	vd;
	char* gibname;

	if (rand() & 1)
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1;		// second skin is player
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	VectorSet3(self->mins, -16, -16, 0);
	VectorSet3(self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage(damage, vd);
	VectorAdd3(self->velocity, vd, self->velocity);

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.Edict_Link(self);
}


/*
=================
debris
=================
*/
void debris_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	Edict_Free(self);
}

void ThrowDebris(edict_t* self, char* modelname, float speed, vec3_t origin)
{
	edict_t* chunk;
	vec3_t	v;

	chunk = Edict_Spawn();
	VectorCopy3(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = 100.0f + 100.0f * crandom();
	VectorMA3(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = Edict_Free;
	chunk->nextthink = level.time + 5 + random() * 5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	gi.Edict_Link(chunk);
}


void BecomeExplosion1(edict_t* self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePos(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	Edict_Free(self);
}


void BecomeExplosion2(edict_t* self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePos(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	Edict_Free(self);
}


/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

void path_corner_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	vec3_t		v;
	edict_t* next;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char* savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		Edict_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = Edict_PickTarget(self->target);
	else
		next = NULL;

	if ((next) && (next->spawnflags & 1))
	{
		VectorCopy3(next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy3(v, other->s.origin);
		next = Edict_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand(other);
		return;
	}

	if (!other->movetarget)
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.stand(other);
	}
	else
	{
		VectorSubtract3(other->goalentity->s.origin, other->s.origin, v);
		other->ideal_yaw = vectoyaw(v);
	}
}

void SP_path_corner(edict_t* self)
{
	if (!self->targetname)
	{
		gi.dprintf("path_corner with no targetname at %s\n", vtos(self->s.origin));
		Edict_Free(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet3(self->mins, -8, -8, -8);
	VectorSet3(self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	gi.Edict_Link(self);
}


/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void point_combat_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	edict_t* activator;

	if (other->movetarget != self)
		return;

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = Edict_PickTarget(other->target);
		if (!other->goalentity)
		{
			gi.dprintf("%s at %s target %s does not exist\n", self->classname, vtos(self->s.origin), self->target);
			other->movetarget = self;
		}
		self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM | FL_FLY)))
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand(other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		char* savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;
		Edict_UseTargets(self, activator);
		self->target = savetarget;
	}
}

void SP_point_combat(edict_t* self)
{
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet3(self->mins, -8, -8, -16);
	VectorSet3(self->maxs, 8, 8, 16);
	self->svflags = SVF_NOCLIENT;
	gi.Edict_Link(self);
};

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null(edict_t* self)
{
	Edict_Free(self);
};


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull(edict_t* self)
{
	VectorCopy3(self->s.origin, self->absmin);
	VectorCopy3(self->s.origin, self->absmax);
};

/*
light_spot (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed spot light as implemented by QBSP
Default _cone value is 10 (used to set size of light for spotlights)
*/

#define START_OFF	1

static void light_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring(CS_LIGHTS + self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

void SP_light_spot(edict_t* self)
{
	if (!self->targetname)
	{
		Edict_Free(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS + self->style, "a");
		else
			gi.configstring(CS_LIGHTS + self->style, "m");
	}
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed point light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
*/


void SP_light(edict_t* self)
{
	if (!self->targetname)
	{
		Edict_Free(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS + self->style, "a");
		else
			gi.configstring(CS_LIGHTS + self->style, "m");
	}
}


/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present

ANIMATED		slow animation

ANIMATED_FAST	fast animation

WALKTHROUGH		this wall is visible, but you can walk through it
*/

void func_wall_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (self->solid == SOLID_NOT)
	{
		// ignore walkthrough walls
		if (!(self->spawnflags & 32)) self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		Game_KillBox(self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.Edict_Link(self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void func_wall_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (!other->client)
		return;

	// failsafe - won't be triggered if it's walkthrough anyway
	if (self->spawnflags & 32)
		return;

	bool allowed = false;

	if (self->team > 0)
	{
		allowed = (self->team == other->team);
	}
	else if (self->allowed_teams > 0)
	{
		allowed = (self->team & other->team);
	}

	// race condition here?

	if (allowed)
	{
		self->solid = SOLID_NOT;
	}
	else
	{
		self->solid = SOLID_BSP;
	}
}

void SP_func_wall(edict_t* self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & 7) == 0)
	{
		// 32 = walkthrough
		// implemented like this so you can use triggers with walk-through walls
		if (self->spawnflags & 32)
		{
			self->s.solid = SOLID_NOT;
		}
		else
		{
			self->solid = SOLID_BSP;
		}

		gi.Edict_Link(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1))
	{
		//		gi.dprintf("func_wall missing TRIGGER_SPAWN\n");
		self->spawnflags |= 1;
	}

	// yell if the spawnflags are odd
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->use = func_wall_use;
	self->touch = func_wall_touch;

	if (self->spawnflags & 4)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.Edict_Link(self);
}

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	// only squash thing we fall on top of
	if (!plane)
		return;
	if (plane->normal[2] < 1.0)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;
	Player_Damage(other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void func_object_release(edict_t* self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	Game_KillBox(self);
	func_object_release(self);
}

void SP_func_object(edict_t* self)
{
	gi.setmodel(self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * FRAMETIME;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;

	gi.Edict_Link(self);
}


/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
void func_explosive_explode(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale3(self->size, 0.5, size);
	VectorAdd3(self->absmin, size, origin);
	VectorCopy3(origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		Player_RadiusDamage(self, attacker, self->dmg, NULL, self->dmg + 40, MOD_EXPLOSIVE);

	VectorSubtract3(self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize3(self->velocity);
	VectorScale3(self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale3(size, 0.5, size);

	mass = self->mass;
	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;
		if (count > 8)
			count = 8;
		while (count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris(self, "models/objects/debris1/tris.md2", 1, chunkorigin);
		}
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	while (count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris(self, "models/objects/debris2/tris.md2", 2, chunkorigin);
	}

	Edict_UseTargets(self, attacker);

	if (self->dmg)
		BecomeExplosion1(self);
	else
		Edict_Free(self);
}

void func_explosive_use(edict_t* self, edict_t* other, edict_t* activator)
{
	func_explosive_explode(self, self, other, self->health, vec3_origin);
}

void func_explosive_spawn(edict_t* self, edict_t* other, edict_t* activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	Game_KillBox(self);
	gi.Edict_Link(self);
}

void SP_func_explosive(edict_t* self)
{
	self->movetype = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self->model);

	if (self->spawnflags & 1)
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->use != func_explosive_use)
	{
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_explode;
		self->takedamage = DAMAGE_YES;
	}

	gi.Edict_Link(self);
}


/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/

void barrel_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)

{
	float	ratio;
	vec3_t	v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract3(self->s.origin, other->s.origin, v);
	AI_MoveWalk(self, vectoyaw(v), 20 * ratio * FRAMETIME);
}

void barrel_explode(edict_t* self)
{
	vec3_t	org;
	float	spd;
	vec3_t	save;

	Player_RadiusDamage(self, self->activator, self->dmg, NULL, self->dmg + 40, MOD_BARREL);

	VectorCopy3(self->s.origin, save);
	VectorMA3(self->absmin, 0.5, self->size, self->s.origin);

	// a few big chunks
	spd = 1.5f * (float)self->dmg / 200.0f;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);

	// bottom corners
	spd = 1.75f * (float)self->dmg / 200.0f;
	VectorCopy3(self->absmin, org);
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy3(self->absmin, org);
	org[0] += self->size[0];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy3(self->absmin, org);
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy3(self->absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);

	// a bunch of little chunks
	spd = 2 * self->dmg / 200;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);

	VectorCopy3(save, self->s.origin);
	if (self->groundentity)
		BecomeExplosion2(self);
	else
		BecomeExplosion1(self);
}

void barrel_delay(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2 * FRAMETIME;
	self->think = barrel_explode;
	self->activator = attacker;
}

void SP_misc_explobox(edict_t* self)
{
	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet3(self->mins, -16, -16, 0);
	VectorSet3(self->maxs, 16, 16, 40);

	if (!self->mass)
		self->mass = 400;
	if (!self->health)
		self->health = 10;
	if (!self->dmg)
		self->dmg = 150;

	self->die = barrel_delay;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.aiflags = AI_NOSTEP;

	self->touch = barrel_touch;

	self->think = AI_MonsterDropToFloor;
	self->nextthink = level.time + 2 * FRAMETIME;

	gi.Edict_Link(self);
}


//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
*/

void misc_blackhole_use(edict_t* ent, edict_t* other, edict_t* activator)
{
	/*
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/
	Edict_Free(ent);
}

void misc_blackhole_think(edict_t* self)
{
	if (++self->s.frame < 19)
		self->nextthink = level.time + FRAMETIME;
	else
	{
		self->s.frame = 0;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_blackhole(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	VectorSet3(ent->mins, -64, -64, 0);
	VectorSet3(ent->maxs, 64, 64, 8);
	ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.Edict_Link(ent);
}


/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
*/
void misc_banner_think(edict_t* ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_banner(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->s.frame = rand() % 16;
	gi.Edict_Link(ent);

	ent->think = misc_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/
void misc_deadsoldier_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	int32_t		n;

	if (self->health > -80)
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
	for (n = 0; n < 4; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}

void SP_misc_deadsoldier(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent->spawnflags & 2)
		ent->s.frame = 1;
	else if (ent->spawnflags & 4)
		ent->s.frame = 2;
	else if (ent->spawnflags & 8)
		ent->s.frame = 3;
	else if (ent->spawnflags & 16)
		ent->s.frame = 4;
	else if (ent->spawnflags & 32)
		ent->s.frame = 5;
	else
		ent->s.frame = 0;

	VectorSet3(ent->mins, -16, -16, 0);
	VectorSet3(ent->maxs, 16, 16, 16);
	ent->deadflag = DEAD_DEAD;
	ent->takedamage = DAMAGE_YES;
	ent->svflags |= SVF_MONSTER | SVF_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.Edict_Link(ent);
}

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
*/
void misc_satellite_dish_think(edict_t* self)
{
	self->s.frame++;
	if (self->s.frame < 38)
		self->nextthink = level.time + FRAMETIME;
}

void misc_satellite_dish_use(edict_t* self, edict_t* other, edict_t* activator)
{
	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_misc_satellite_dish(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet3(ent->mins, -64, -64, 0);
	VectorSet3(ent->maxs, 64, 64, 128);
	ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.Edict_Link(ent);
}


/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine1(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.Edict_Link(ent);
}


/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine2(edict_t* ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.Edict_Link(ent);
}


/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = Edict_Free;
	ent->nextthink = level.time + 30;
	gi.Edict_Link(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = Edict_Free;
	ent->nextthink = level.time + 30;
	gi.Edict_Link(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random() * 200;
	ent->avelocity[1] = random() * 200;
	ent->avelocity[2] = random() * 200;
	ent->think = Edict_Free;
	ent->nextthink = level.time + 30;
	gi.Edict_Link(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(edict_t* self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.Edict_Link(self);
	return;
}


/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/

void target_string_use(edict_t* self, edict_t* other, edict_t* activator)
{
	edict_t* e;
	int		n, l;
	char	c;

	l = (int32_t)strlen(self->message);
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string(edict_t* self)
{
	if (!self->message)
		self->message = "";
	self->use = target_string_use;
}


/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

#define	CLOCK_MESSAGE_SIZE	16

// don't let field width of any clock messages change, or it
// could cause an overwrite after a game load

static void func_clock_reset(edict_t* self)
{
	self->activator = NULL;
	if (self->spawnflags & 1)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown(edict_t* self)
{
	if (self->style == 0)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		return;
	}

	if (self->style == 2)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
		return;
	}
}

void func_clock_think(edict_t* self)
{
	if (!self->enemy)
	{
		self->enemy = Game_FindEdictByValue(NULL, FOFS(targetname), self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags & 1)
	{
		func_clock_format_countdown(self);
		self->health++;
	}
	else if (self->spawnflags & 2)
	{
		func_clock_format_countdown(self);
		self->health--;
	}
	else
	{
		struct tm* ltime;
		time_t		gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use(self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) ||
		((self->spawnflags & 2) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char* savetarget;
			char* savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			Edict_UseTargets(self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8))
			return;

		func_clock_reset(self);

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1;
}

void func_clock_use(edict_t* self, edict_t* other, edict_t* activator)
{
	if (!(self->spawnflags & 8))
		self->use = NULL;
	if (self->activator)
		return;
	self->activator = activator;
	self->think(self);
}

void SP_func_clock(edict_t* self)
{
	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		Edict_Free(self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count))
	{
		gi.dprintf("%s with no count at %s\n", self->classname, vtos(self->s.origin));
		Edict_Free(self);
		return;
	}

	if ((self->spawnflags & 1) && (!self->count))
		self->count = 60 * 60;;

	func_clock_reset(self);

	self->message = gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = func_clock_think;

	if (self->spawnflags & 4)
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1;
}

//=================================================================================

void teleporter_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	edict_t* dest;
	int			i;

	if (!other->client)
		return;
	dest = Game_FindEdictByValue(NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with KillBox
	gi.Edict_Unlink(other);

	VectorCopy3(dest->s.origin, other->s.origin);
	VectorCopy3(dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	VectorClear3(other->velocity);
	other->client->ps.pmove.pm_time = 160 >> 3;		// hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self->owner->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	for (i = 0; i < 3; i++)
	{
		other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);
	}

	VectorClear3(other->s.angles);
	VectorClear3(other->client->ps.viewangles);
	VectorClear3(other->client->v_angle);

	// kill anything at the destination
	Game_KillBox(other);

	gi.Edict_Link(other);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter(edict_t* ent)
{
	edict_t* trig;

	if (!ent->target)
	{
		gi.dprintf("teleporter without a target.\n");
		Edict_Free(ent);
		return;
	}

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 1;
	ent->s.effects = EF_TELEPORTER;
	ent->s.sound = gi.soundindex("world/amb10.wav");
	ent->solid = SOLID_BBOX;

	VectorSet3(ent->mins, -32, -32, -24);
	VectorSet3(ent->maxs, 32, 32, -16);
	gi.Edict_Link(ent);

	trig = Edict_Spawn();
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	VectorCopy3(ent->s.origin, trig->s.origin);
	VectorSet3(trig->mins, -8, -8, 8);
	VectorSet3(trig->maxs, 8, 8, 24);
	gi.Edict_Link(trig);

}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(edict_t* ent)
{
	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	//	ent->s.effects |= EF_FLIES;
	VectorSet3(ent->mins, -32, -32, -24);
	VectorSet3(ent->maxs, 32, 32, -16);
	gi.Edict_Link(ent);
}

