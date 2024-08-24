/*
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
// Weapon_bamfuslicator.c: Weapon code for the Director team's Bamfuslicator - split from p_weapon.c

#include <game_local.h>
#include <mobs/mob_player.h>

void Weapon_Bamfuslicator_SetType(edict_t* ent)
{
	// cycle spawn types
	ent->client->pers.weapon->spawn_type++;
	if (ent->client->pers.weapon->spawn_type > MAX_ZOMBIE_TYPE) ent->client->pers.weapon->spawn_type = 0;

	switch (ent->client->pers.weapon->spawn_type)
	{
	case zombie_type_normal:
		GameUI_SetText(ent, "BamfuslicatorUI", "BamfuslicatorUI_Text", "Zombie Type: ^2Regular^7", false);
		break;
	case zombie_type_fast:
		GameUI_SetText(ent, "BamfuslicatorUI", "BamfuslicatorUI_Text", "Zombie Type: ^4On Speed^7", false);
		break;
	case zombie_type_ogre:
		GameUI_SetText(ent, "BamfuslicatorUI", "BamfuslicatorUI_Text", "Zombie Type: ^5Ogre^7", false);
		break;
	}

	if (!(level.framenum % (int32_t)(0.1f / TICK_TIME)))
	{
		// todo: separate primary and secondary fire frames
		ent->client->ps.gunframe++; // increment anim frame
	}

}

void Weapon_Bamfuslicator_Fire(edict_t* ent)
{
	// TEST code

	if (ent->client->ps.camera_type != camera_type_topdown)
	{
		ent->client->ps.camera_type = camera_type_topdown;
		ent->movetype = MOVETYPE_FLY;
	}
	else
	{
		ent->client->ps.camera_type = camera_type_normal;
		ent->movetype = MOVETYPE_WALK;
	}


	//todo: audio
	vec3_t offset = { 0 }, start = { 0 }, forward = { 0 }, right = { 0 };

	// only fire forward
	AngleVectors(ent->client->v_angle, forward, right, NULL);

	// set offset
	VectorSet3(offset, 0, 8, ent->viewheight - 8);

	Player_ProjectSource(ent, offset, forward, right, start);

	Ammo_Bamfuslicator(ent, start, forward, ent->client->pers.weapon->spawn_type); // will always store current spawn type

	if (!(level.framenum % (int32_t)(0.1f / TICK_TIME)))
	{
		// todo: separate primary and secondary fire frames
		ent->client->ps.gunframe++; // increment anim frame
	}

}

void Weapon_Bamfuslicator(edict_t* ent)
{
	static int32_t pause_frames[] = { 29, 42, 57, 0 };
	static int32_t fire_frames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 56, 61, pause_frames, fire_frames, fire_frames, Weapon_Bamfuslicator_Fire, Weapon_Bamfuslicator_SetType);
}

//======================================================================

