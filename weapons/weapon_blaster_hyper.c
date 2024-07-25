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
// Weapon_blaster_hyper.c : HyperBlaster weapon code - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

extern void Blaster_Fire(edict_t* ent, vec3_t g_offset, int32_t damage, bool hyper, int32_t effect);

void Weapon_HyperBlaster_Fire(edict_t* ent)
{
	float	rotation;
	vec3_t	offset;
	int32_t	effect;
	int32_t	damage;

	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent->client->buttons & BUTTON_ATTACK1))
	{
		ent->client->ps.gunframe++;
	}
	else
	{
		if (ent->client->loadout_current_ammo
			|| ent->client->loadout_current_ammo->amount == 0)
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			Player_WeaponChangeNoAmmo(ent);
		}
		else
		{
			rotation = (ent->client->ps.gunframe - 5) * 2 * M_PI / 6;
			offset[0] = -4.0f * sinf(rotation);
			offset[1] = 0;
			offset[2] = 4.0f * cosf(rotation);

			if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;

			damage = 15;

			Blaster_Fire(ent, offset, damage, true, effect);
			if (!((int32_t)gameflags->value & GF_INFINITE_AMMO))
				ent->client->loadout_current_ammo->amount--;

			ent->client->anim_priority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->anim_end = FRAME_crattak9;
			}
			else
			{
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
		}

		ent->client->ps.gunframe++;
		if (ent->client->ps.gunframe == 12 && ent->client->loadout_current_ammo)
			ent->client->ps.gunframe = 6;
	}

	if (ent->client->ps.gunframe == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;
	}

}

void Weapon_HyperBlaster(edict_t* ent)
{
	static int32_t pause_frames[] = { 0 };
	static int32_t fire_frames[] = { 6, 7, 8, 9, 10, 11, 0 };

	Weapon_Generic(ent, 5, 20, 49, 53, pause_frames, fire_frames, NULL, Weapon_HyperBlaster_Fire, NULL);

}