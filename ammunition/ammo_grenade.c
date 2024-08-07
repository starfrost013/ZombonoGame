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

// ammo_grenade.c: Code for the grenade launcher's grenades (split from g_weapon.c)

/*
=================
fire_grenade
=================
*/
static void Ammo_Grenade_explode(edict_t* ent)
{
	vec3_t		origin;
	int32_t			mod;

	if (ent->owner->client)
		Player_Noise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent->enemy)
	{
		float	points;
		vec3_t	v;
		vec3_t	dir;

		VectorAdd3(ent->enemy->mins, ent->enemy->maxs, v);
		VectorMA3(ent->enemy->s.origin, 0.5, v, v);
		VectorSubtract3(ent->s.origin, v, v);
		points = ent->dmg - 0.5f * VectorLength3(v);
		VectorSubtract3(ent->enemy->s.origin, ent->s.origin, dir);
		if (ent->spawnflags & 1)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;

		Player_Damage(ent->enemy, ent, ent->owner, dir, ent->s.origin, vec3_origin, (int32_t)points, (int32_t)points, DAMAGE_RADIUS, mod);
	}

	if (ent->spawnflags & 2)
		mod = MOD_HELD_GRENADE;
	else if (ent->spawnflags & 1)
		mod = MOD_SPLASH_HANDGRENADE;
	else
		mod = MOD_SPLASH_GRENADE;

	Player_RadiusDamage(ent, ent->owner, ent->dmg, ent->enemy, ent->dmg_radius, mod);

	VectorMA3(ent->s.origin, -0.02, ent->velocity, origin);
	gi.WriteByte(svc_temp_entity);
	if (ent->waterlevel)
	{
		if (ent->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (ent->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION);
	}
	gi.WritePos(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	Edict_Free(ent);
}

static void Ammo_Grenade_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		Edict_Free(ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	ent->enemy = other;
	Ammo_Grenade_explode(ent);
}

void Ammo_Grenade(edict_t* self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, float timer, float damage_radius)
{
	edict_t*	grenade;
	vec3_t		dir;
	vec3_t		forward, right, up;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = Edict_Spawn();
	VectorCopy3(start, grenade->s.origin);
	VectorScale3(aimdir, speed, grenade->velocity);
	VectorMA3(grenade->velocity, 200 + crandom() * 10.0f, up, grenade->velocity);
	VectorMA3(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet3(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear3(grenade->mins);
	VectorClear3(grenade->maxs);
	grenade->s.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
	grenade->owner = self;
	grenade->touch = Ammo_Grenade_touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Ammo_Grenade_explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "grenade";

	gi.Edict_Link(grenade);
}

void Ammo_Grenade2(edict_t* self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, float timer, float damage_radius, bool held)
{
	edict_t* grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = Edict_Spawn();
	VectorCopy3(start, grenade->s.origin);
	VectorScale3(aimdir, speed, grenade->velocity);
	VectorMA3(grenade->velocity, 200 + crandom() * 10.0f, up, grenade->velocity);
	VectorMA3(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet3(grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear3(grenade->mins);
	VectorClear3(grenade->maxs);
	grenade->s.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
	grenade->owner = self;
	grenade->touch = Ammo_Grenade_touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Ammo_Grenade_explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "hgrenade";
	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;
	grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0)
		Ammo_Grenade_explode(grenade);
	else
	{
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.Edict_Link(grenade);
	}
}

void Ammo_Grenade_monster(edict_t* self, vec3_t start, vec3_t aimdir, int32_t damage, int32_t speed, int32_t flashtype)
{
	Ammo_Grenade(self, start, aimdir, damage, speed, 2.5, (float)(damage + 40.0f));

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashtype);
	gi.multicast(start, MULTICAST_PVS);
}
