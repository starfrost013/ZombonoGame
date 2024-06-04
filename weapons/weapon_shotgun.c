
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
// Weapon_shotgun.c : Shotgun weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

void Weapon_shotgun_fire(edict_t* ent)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	offset;
	int32_t	damage = 4;
	int32_t	kick = 8;

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;
		return;
	}

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	Ammo_Bullet_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int32_t)gameflags->value & GF_INFINITE_AMMO))
		ent->client->loadout_current_ammo->amount--;
}

void Weapon_Shotgun(edict_t* ent)
{
	static int	pause_frames[] = { 22, 28, 34, 0 };
	static int	fire_frames[] = { 8, 9, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, NULL, Weapon_shotgun_fire, NULL);
}

