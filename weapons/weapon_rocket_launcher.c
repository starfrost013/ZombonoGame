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
// Weapon_rocket_launcher.c : Rocket launcher weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

#define ROCKETLAUNCHER_SPEED					850

void Weapon_RocketLauncher_Fire(edict_t* ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int32_t	damage;
	float	damage_radius;
	int32_t	radius_damage;

	damage = 100 + (int32_t)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
		damage *= 4;
		radius_damage *= 4;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent, offset, forward, right, start);
	Ammo_Rocket(ent, start, forward, damage, ROCKETLAUNCHER_SPEED, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int32_t)gameflags->value & GF_INFINITE_AMMO))
		ent->client->loadout_current_ammo->amount--;
}

void Weapon_RocketLauncher(edict_t* ent)
{
	static int32_t pause_frames[] = { 25, 33, 42, 50, 0 };
	static int32_t fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, NULL, Weapon_RocketLauncher_Fire, NULL);
}

