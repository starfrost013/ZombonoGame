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
// Weapon_base.c : Core weapon code spread across many or all weapons - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

bool		is_quad;
uint8_t		is_silenced;

void P_ProjectSource(edict_t* ent, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	gclient_t* client = ent->client;
	float* point = ent->s.origin;
	vec3_t     _distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);

	// Berserker: fix - now the projectile hits exactly where the scope is pointing.
	if (aimfix->value)
	{
		vec3_t start, end;
		VectorSet(start, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] + ent->viewheight);
		VectorMA(start, 8192, forward, end);

		trace_t	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);
		if (tr.fraction < 1)
		{
			VectorSubtract(tr.endpos, result, forward);
			VectorNormalize(forward);
		}
	}
}

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t* who, vec3_t where, int32_t type)
{
	edict_t* noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (who->flags & FL_NOTARGET)
		return;


	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet(noise->mins, -8, -8, -8);
		VectorSet(noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy(where, noise->s.origin);
	VectorSubtract(where, noise->maxs, noise->absmin);
	VectorAdd(where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity(noise);
}


bool Pickup_Weapon(edict_t* ent, edict_t* other)
{
	int			index;
	gitem_t*	ammo;
	loadout_entry_t* loadout_ptr = Loadout_GetItem(other, ent->item->pickup_name);

	if (!loadout_ptr)
		loadout_ptr = Loadout_AddItem(other, ent->item->pickup_name, ent->item->icon, loadout_entry_type_weapon, 1);

	index = ITEM_INDEX(ent->item);

	if ((((int32_t)(gameflags->value) & GF_WEAPONS_STAY))
		&& loadout_ptr->amount > 0)
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;	// leave the weapon for others to pickup
	}

	// if the user is on a team that cannot pick up this item (0 = all teams)
	if (ent->item->allowed_teams > 0
		&& !(ent->item->allowed_teams & other->team))
	{
		return false;
	}

	loadout_ptr->amount++;

	if (!(ent->spawnflags & DROPPED_ITEM))
	{
		// give them some ammo with it
		ammo = FindItem(ent->item->ammo);

		if (ammo)
		{
			if ((int32_t)gameflags->value & GF_INFINITE_AMMO)
				Add_Ammo(other, ammo, 1000);
			else
				Add_Ammo(other, ammo, ammo->quantity);

			if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
			{
				if ((int32_t)(gameflags->value) & GF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30);
			}
		}
	}

	// wtf does this do?
	if (other->client->pers.weapon != ent->item &&
		//(other->client->pers.inventory[index] == 1) && doesn't work with new loadout system
		(other->client->pers.weapon == FindItem("blaster")))
		other->client->newweapon = ent->item;

	return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(edict_t* ent)
{
	int32_t i;

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		Weapon_grenade_fire(ent, false);
		ent->client->grenade_time = 0;
	}

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;

	if (ent->client->newweapon != NULL)
	{
		// toggle the UI depending on if we are switching into or out of the bamfuslicator (TODO: HACK!!!!)
		if (!strcmp(ent->client->newweapon->classname, "weapon_bamfuslicator"))
		{
			G_UISend(ent, "BamfuslicatorUI", true, false, false);
			ent->client->pers.weapon->spawn_type = -1; // another hack, settype increments it so it will be set to 0
			Weapon_Bamfuslicator_SetType(ent);
		}
		else if (ent->client->pers.lastweapon != NULL
			&& !strcmp(ent->client->pers.lastweapon->classname, "weapon_bamfuslicator")) // switching out
		{
			G_UISend(ent, "BamfuslicatorUI", false, false, false);
		}
	}

	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255) {
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
	{
		gitem_t* item_ammo = FindItem(ent->client->pers.weapon->ammo);

		loadout_entry_t* item_ammo_loadout_ptr = Loadout_GetItem(ent, item_ammo->pickup_name);

		// if the ammo is not already there, add it (failsafe, really shouldn't happen)
		if (!item_ammo_loadout_ptr)
			gi.error("The loadout is fucked");

		// add it to the loadout
		ent->client->loadout_current_ammo = item_ammo_loadout_ptr;
	}
	else
	{
		ent->client->loadout_current_ammo = NULL; // TEMP
	}

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	ent->client->anim_priority = ANIM_PAIN;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crpain1;
		ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
		ent->s.frame = FRAME_pain301;
		ent->client->anim_end = FRAME_pain304;
	}
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t* ent)
{
	if (Loadout_GetItem(ent, "slugs")
		&& Loadout_GetItem(ent, "railgun"))
	{
		ent->client->newweapon = FindItem("railgun");
		return;
	}
	if (Loadout_GetItem(ent, "cells")
		&& Loadout_GetItem(ent, "hyperblaster"))
	{
		ent->client->newweapon = FindItem("hyperblaster");
		return;
	}
	if (Loadout_GetItem(ent, "bullets")
		&& Loadout_GetItem(ent, "chaingun"))
	{
		ent->client->newweapon = FindItem("chaingun");
		return;
	}
	if (Loadout_GetItem(ent, "bullets")
		&& Loadout_GetItem(ent, "machinegun"))
	{
		ent->client->newweapon = FindItem("machinegun");
		return;
	}
	//required as super shotgun uses 2 bullets
	loadout_entry_t* bullets = Loadout_GetItem(ent, "bullets");

	if (bullets != NULL
		&& bullets->amount >= 2
		&& Loadout_GetItem(ent, "super shotgun"))
	{
		ent->client->newweapon = FindItem("super shotgun");
		return;
	}
	if (Loadout_GetItem(ent, "shells")
		&& Loadout_GetItem(ent, "shotgun"))
	{
		ent->client->newweapon = FindItem("shotgun");
		return;
	}
	ent->client->newweapon = FindItem("blaster");
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t* ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon(ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;
		ent->client->pers.weapon->weaponthink(ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(edict_t* ent, gitem_t* item)
{
	gitem_t* ammo_item;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = FindItem(item->ammo);
		loadout_entry_t* ammo_item_loadout_ptr = Loadout_GetItem(ent, ammo_item->pickup_name);

		if (ammo_item_loadout_ptr == NULL
			|| ammo_item_loadout_ptr->amount == 0)
		{
			gi.cprintf(ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ammo_item_loadout_ptr->amount < item->quantity)
		{
			gi.cprintf(ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t* ent, gitem_t* item)
{
	int		index;

	if ((int32_t)(gameflags->value) & GF_WEAPONS_STAY)
		return;

	loadout_entry_t* loadout_ptr = Loadout_GetItem(ent, item->pickup_name);

	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) && loadout_ptr->amount == 1)
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item(ent, item);
	ent->client->loadout.num_items--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic(edict_t* ent, int32_t FRAME_ACTIVATE_LAST, int32_t FRAME_FIRE_LAST, int32_t FRAME_IDLE_LAST, int32_t FRAME_DEACTIVATE_LAST,
	int32_t* pause_frames, int32_t* fire_frames_primary, int32_t* fire_frames_secondary, void (*fire_primary)(edict_t* ent), void(*fire_secondary)(edict_t* ent))
{
	int			n;
	bool	no_ammo = false;

	if (ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
	{
		return;
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon(ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;

			}
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING_PRIMARY) && (ent->client->weaponstate != WEAPON_FIRING_SECONDARY))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;

			}
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK1))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK1;
			if ((!ent->client->loadout_current_ammo) ||
				(ent->client->loadout_current_ammo->amount >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING_PRIMARY;

				// start the animation
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
			else
			{
				no_ammo = true;
			}
		}
		else if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK2)
			&& fire_secondary != NULL)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK2;

			if ((!ent->client->loadout_current_ammo) ||
				(ent->client->loadout_current_ammo->amount >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING_SECONDARY;

				// start the animation (TODO: secondary animations)
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
			else
			{
				no_ammo = true;
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.gunframe == pause_frames[n])
					{
						if (rand() & 15)
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
			return;
		}

		if (no_ammo)
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange(ent);
		}

	}

	if (ent->client->weaponstate == WEAPON_FIRING_PRIMARY)
	{
		for (n = 0; fire_frames_primary[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames_primary[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

				fire_primary(ent);
			}
		}

		if (!fire_frames_primary[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}
	else if (ent->client->weaponstate == WEAPON_FIRING_SECONDARY)
	{
		for (n = 0; fire_frames_secondary[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames_primary[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

				// we already checked if it's not null while setting the weapon state
				fire_secondary(ent);

				break;
			}
		}

		if (!fire_frames_secondary[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}
}
