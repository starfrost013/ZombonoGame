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
#pragma once
#include <game_local.h>

void SP_item_health(edict_t* self);
void SP_item_health_small(edict_t* self);
void SP_item_health_large(edict_t* self);
void SP_item_health_super(edict_t* self);
void SP_item_health_mega(edict_t* self);

void SP_info_player_start(edict_t* ent);
void SP_info_player_start_director(edict_t* ent);
void SP_info_player_start_player(edict_t* ent);
void SP_info_team_door(edict_t* ent);

void SP_func_plat(edict_t* ent);
void SP_func_rotating(edict_t* ent);
void SP_func_button(edict_t* ent);
void SP_func_door(edict_t* ent);
void SP_func_door_secret(edict_t* ent);
void SP_func_door_rotating(edict_t* ent);
void SP_func_water(edict_t* ent);
void SP_func_train(edict_t* ent);
void SP_func_conveyor(edict_t* self);
void SP_func_wall(edict_t* self);
void SP_func_object(edict_t* self);
void SP_func_explosive(edict_t* self);
void SP_func_timer(edict_t* self);
void SP_func_areaportal(edict_t* ent);
void SP_func_clock(edict_t* ent);
void SP_func_killbox(edict_t* ent);
void SP_func_trampoline(edict_t* ent);

void SP_trigger_always(edict_t* ent);
void SP_trigger_once(edict_t* ent);
void SP_trigger_multiple(edict_t* ent);
void SP_trigger_relay(edict_t* ent);
void SP_trigger_push(edict_t* ent);
void SP_trigger_hurt(edict_t* ent);
void SP_trigger_counter(edict_t* ent);
void SP_trigger_elevator(edict_t* ent);
void SP_trigger_gravity(edict_t* ent);
void SP_trigger_monsterjump(edict_t* ent);

void SP_target_temp_entity(edict_t* ent);
void SP_target_speaker(edict_t* ent);
void SP_target_explosion(edict_t* ent);
void SP_target_changelevel(edict_t* ent);
void SP_target_secret(edict_t* ent);
void SP_target_goal(edict_t* ent);
void SP_target_splash(edict_t* ent);
void SP_target_spawner(edict_t* ent);
void SP_target_blaster(edict_t* ent);
void SP_target_crosslevel_trigger(edict_t* ent);
void SP_target_crosslevel_target(edict_t* ent);
void SP_target_laser(edict_t* self);
void SP_target_help(edict_t* ent);
void SP_target_lightramp(edict_t* self);
void SP_target_earthquake(edict_t* ent);
void SP_target_character(edict_t* ent);
void SP_target_string(edict_t* ent);

void SP_worldspawn(edict_t* ent);
void SP_viewthing(edict_t* ent);

void SP_light(edict_t* self);
void SP_light_mine1(edict_t* ent);
void SP_light_mine2(edict_t* ent);
void SP_light_spot(edict_t* self);
void SP_info_null(edict_t* self);
void SP_info_notnull(edict_t* self);
void SP_path_corner(edict_t* self);
void SP_point_combat(edict_t* self);

void SP_misc_explobox(edict_t* self);
void SP_misc_banner(edict_t* self);
void SP_misc_gib_arm(edict_t* self);
void SP_misc_gib_leg(edict_t* self);
void SP_misc_gib_head(edict_t* self);
void SP_misc_deadsoldier(edict_t* self);
void SP_misc_teleporter(edict_t* self);
void SP_misc_teleporter_dest(edict_t* self);
void SP_misc_blackhole(edict_t* self);

void SP_monster_zombie(edict_t* self);
void SP_monster_zombie_fast(edict_t* self);
void SP_monster_ogre(edict_t* self);