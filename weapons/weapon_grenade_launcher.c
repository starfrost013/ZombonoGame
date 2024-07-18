
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

// Weapon_grenade_launcher.c : Grenade launcher weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

void Weapon_grenadelauncher_fire(edict_t* ent)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int32_t	damage = 120;
	float	radius;

	radius = damage + 40;
	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	Player_ProjectSource(ent, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	Ammo_Grenade(ent, start, forward, damage, 600, 2.5, radius);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	Player_Noise(ent, start, PNOISE_WEAPON);

	if (!((int32_t)gameflags->value & GF_INFINITE_AMMO))
		ent->client->loadout_current_ammo->amount--;
}

void Weapon_GrenadeLauncher(edict_t* ent)
{
	static int32_t pause_frames[] = { 34, 51, 59, 0 };
	static int32_t fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, NULL, Weapon_grenadelauncher_fire, NULL);
}

