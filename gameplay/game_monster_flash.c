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
// game_monster_flash.c

// DON'T FIX THIS INCLUDE YOU WILL BREAK COMPILING!!!:
// this file is included in both the game dll and quake2,
// the game needs it to source shot locations, the client
// needs it to position muzzle flashes

#include "../q_shared.h"

vec3_t monster_flash_offset [] =
{
// flash 0 is not used
	0.0, 0.0, 0.0,

// MZ2_SOLDIER_BLASTER_1			1
	10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,
// MZ2_SOLDIER_SHOTGUN_1			2
	10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,
// MZ2_SOLDIER_MACHINEGUN_1			3
	10.6 * 1.2, 7.7 * 1.2, 7.8 * 1.2,

// end of table
	0.0, 0.0, 0.0
};
