
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
// Weapon_machinegun.c : Machine gun weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>
/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Weapon_Machinegun_Fire(edict_t* ent)
{
	int32_t	i;
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	angles;
	int32_t	damage = 8;
	int32_t	kick = 2;
	vec3_t	offset = { 0 };

	if (!(ent->client->buttons & BUTTON_ATTACK1))
	{
		ent->client->machinegun_shots = 0;
		if (!(level.framenum % (int32_t)(0.1f / FRAMETIME)))
		{
			// todo: separate primary and secondary fire frames
			ent->client->ps.gunframe++; // increment anim frame
		}

		return;
	}

	if (ent->client->ps.gunframe == 5)
		ent->client->ps.gunframe = 4;
	else
		ent->client->ps.gunframe = 5;

	if (!((int32_t)gameflags->value & GF_INFINITE_AMMO))
	{
		// decrement the amount of ammo
		ent->client->loadout_current_ammo->amount--;

		if (ent->client->loadout_current_ammo->amount < 1)
		{
			ent->client->ps.gunframe = 6;
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			Player_WeaponChangeNoAmmo(ent);
			return;
		}
	}

	if (is_quad)
	{
		damage *= 4;
		kick *= 4;
	}

	for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35f;
		ent->client->kick_angles[i] = crandom() * 0.7f;
	}

	ent->client->kick_origin[0] = crandom() * 0.35f;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// get start / end positions
	VectorAdd3(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet3(offset, 0, 8, ent->viewheight - 8);
	Player_ProjectSource(ent, offset, forward, right, start);
	Ammo_Bullet(ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	Player_Noise(ent, start, PNOISE_WEAPON);

	ent->client->anim_priority = ANIM_ATTACK;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int32_t)(random() + 0.25);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int32_t)(random() + 0.25);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun(edict_t* ent)
{
	static int32_t pause_frames[] = { 23, 45, 0 };
	static int32_t fire_frames[] = { 4, 5, 0 };

	Weapon_Generic(ent, 3, 5, 45, 49, pause_frames, fire_frames, NULL, Weapon_Machinegun_Fire, NULL);
}
