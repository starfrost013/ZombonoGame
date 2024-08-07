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

// ammo_rocket.c: Code for the rocket launcher's rockets (split from g_weapon.c)

#define ROCKET_MAX_DISTANCE	8000

/*
=================
fire_rocket
=================
*/
void Ammo_Rocket_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	vec3_t		origin;
	int			n;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		Edict_Free(ent);
		return;
	}

	if (ent->owner->client)
		Player_Noise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA3(ent->s.origin, -0.02, ent->velocity, origin);

	if (other->takedamage)
	{
		Player_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_ROCKET);
	}
	else
	{
		if ((surf) && !(surf->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING)))
		{
			n = rand() % 5;
			while (n--)
				ThrowDebris(ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
		}
	}

	Player_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_SPLASH_ROCKET);

	gi.WriteByte(svc_temp_entity);
	if (ent->waterlevel)
		gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	Edict_Free(ent);
}

void Ammo_Rocket(edict_t* self, vec3_t start, vec3_t dir, int32_t damage, int32_t speed, float damage_radius, int32_t radius_damage)
{
	edict_t* rocket;

	rocket = Edict_Spawn();
	VectorCopy3(start, rocket->s.origin);
	VectorCopy3(dir, rocket->movedir);
	vectoangles(dir, rocket->s.angles);
	VectorScale3(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	VectorClear3(rocket->mins);
	VectorClear3(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = Ammo_Rocket_touch;
	rocket->nextthink = level.time + ROCKET_MAX_DISTANCE / speed;
	rocket->think = Edict_Free;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->dmg_radius = damage_radius;
	rocket->s.sound = gi.soundindex("weapons/rockfly.wav");
	rocket->classname = "rocket";

	if (self->client)
		AI_MonsterCheckDodge(self, rocket->s.origin, dir, speed);

	gi.Edict_Link(rocket);
}

void Ammo_Rocket_monster(edict_t* self, vec3_t start, vec3_t dir, int32_t damage, int32_t speed, int32_t flashtype)
{
	Ammo_Rocket(self, start, dir, damage, speed, damage + 20, damage);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}