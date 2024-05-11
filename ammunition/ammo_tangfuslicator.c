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

#define TANGFUSLICATOR_MAX_DISTANCE			512

// ammo_tangfuslicator.c: Code for the Lightning Gun thing...
void Ammo_Tangfuslicator(edict_t* self, vec3_t start, vec3_t aimdir)
{
	vec3_t end = { 0 };

	end[0] = start[0] + aimdir[0] * TANGFUSLICATOR_MAX_DISTANCE;
	end[1] = start[1] + aimdir[1] * TANGFUSLICATOR_MAX_DISTANCE;
	end[2] = start[2] + aimdir[2] * TANGFUSLICATOR_MAX_DISTANCE;

	//todo: actually implement
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.WriteDir(aimdir);
}