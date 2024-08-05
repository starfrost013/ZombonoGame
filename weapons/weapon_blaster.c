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
// Weapon_blaster.c : Blaster weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

void Blaster_Fire(edict_t* ent, vec3_t g_offset, int32_t damage, bool hyper, int32_t effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset = { 0 };

	if (is_quad)
		damage *= 4;
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet3(offset, 24, 8, ent->viewheight - 8);
	VectorAdd3(offset, g_offset, offset);
	Player_ProjectSource(ent, offset, forward, right, start);

	VectorScale3(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	Ammo_Blaster(ent, start, forward, damage, 1000, effect, hyper);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	Player_Noise(ent, start, PNOISE_WEAPON);
}

void Weapon_Blaster_Fire(edict_t* ent)
{
	int32_t damage;

	damage = 15;

	Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
}

void Weapon_Blaster(edict_t* ent)
{
	static int32_t pause_frames[] = { 19, 32, 0 };
	static int32_t fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, NULL, Weapon_Blaster_Fire, NULL);
}