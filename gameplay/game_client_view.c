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

//game_client_view.c: Handles what clients see (server side)

#include <game_local.h>
#include <mobs/mob_player.h>

edict_t* current_player;
gclient_t* current_client;

vec3_t	forward, right, up;
float	xyspeed;

float	bobmove;
int32_t	bobcycle;		// odd cycles are right foot going forward
float	bobfracsin;		// sin(bobfrac*M_PI)

/*
===============
SV_CalcRoll

===============
*/
float Client_CalcRoll(vec3_t angles, vec3_t velocity)
{
	float sign;
	float side;
	float value;

	side = DotProduct3(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabsf(side);

	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;

	return side * sign;
}


/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void Player_DamageFeedback(edict_t* player)
{
	gclient_t* client;
	float	side;
	float	realcount, count, kick;
	vec3_t	v;
	int32_t	r, l;
	static vec3_t power_color = { 0.0, 1.0, 0.0 };
	static vec3_t acolor = { 1.0, 1.0, 1.0 };
	static vec3_t bcolor = { 1.0, 0.0, 0.0 };

	client = player->client;

	// flash the backgrounds behind the status numbers
	client->ps.stats[STAT_FLASHES] = 0;
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
		client->ps.stats[STAT_FLASHES] |= 2;

	// total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor + client->damage_parmor);
	if (count == 0)
		return;		// didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == 255)
	{
		static int32_t		i;

		client->anim_priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player->s.frame = FRAME_crpain1 - 1;
			client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i + 1) % 3;
			switch (i)
			{
			case 0:
				player->s.frame = FRAME_pain101 - 1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201 - 1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301 - 1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	realcount = count;
	if (count < 10)
		count = 10;	// always make a visible effect

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
	{
		r = 1 + (rand() & 1);
		player->pain_debounce_time = level.time + 0.7f;
		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;
		gi.sound(player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0.0f)
		client->damage_alpha = 0.0f;
	client->damage_alpha += count * 0.01f;
	if (client->damage_alpha < 0.2f)
		client->damage_alpha = 0.2f;
	if (client->damage_alpha > 0.6f)
		client->damage_alpha = 0.6f;		// don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear3(v);
	if (client->damage_parmor)
		VectorMA3(v, (float)client->damage_parmor / realcount, power_color, v);
	if (client->damage_armor)
		VectorMA3(v, (float)client->damage_armor / realcount, acolor, v);
	if (client->damage_blood)
		VectorMA3(v, (float)client->damage_blood / realcount, bcolor, v);
	VectorCopy3(v, client->damage_blend);


	//
	// calculate view angle kicks
	//
	kick = abs(client->damage_knockback);
	if (kick && player->health > 0)	// kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count * 0.5f)
			kick = count * 0.5f;
		if (kick > 50)
			kick = 50;

		VectorSubtract3(client->damage_from, player->s.origin, v);
		VectorNormalize3(v);

		side = DotProduct3(v, right);
		client->v_dmg_roll = kick * side * 0.3f;

		side = -DotProduct3(v, forward);
		client->v_dmg_pitch = kick * side * 0.3f;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
}




/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void Client_CalcViewOffset(edict_t* ent)
{
	float*	angles;
	float	bob;
	float	ratio;
	float	delta;
	vec3_t	v;


	//===================================
	// base angles

	angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		VectorClear3(angles);

		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick

		VectorCopy3(ent->client->kick_angles, angles);

		// add angles based on damage kick

		ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent->client->v_dmg_pitch = 0;
			ent->client->v_dmg_roll = 0;
		}
		angles[PITCH] += ratio * ent->client->v_dmg_pitch;
		angles[ROLL] += ratio * ent->client->v_dmg_roll;

		// add pitch based on fall kick

		ratio = (ent->client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		angles[PITCH] += ratio * ent->client->fall_value;

		// add angles based on velocity

		delta = DotProduct3(ent->velocity, forward);
		angles[PITCH] += delta * run_pitch->value;

		delta = DotProduct3(ent->velocity, right);
		angles[ROLL] += delta * run_roll->value;

		// add angles based on bob

		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;
	}

	//===================================

		// base origin

	VectorClear3(v);

	// add view height

	v[2] += ent->viewheight;

	// add fall height

	ratio = (ent->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v[2] -= ratio * ent->client->fall_value * 0.4f;

	// add bob height

	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6)
		bob = 6;

	v[2] += bob;

	// add kick offset

	VectorAdd3(v, ent->client->kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;
	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;
	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;

	VectorCopy3(v, ent->client->ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void Client_CalcGunOffset(edict_t* ent)
{
	int32_t	i;
	float	delta;

	// gun angles from bobbing
	ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005f;
	ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01f;
	if (bobcycle & 1)
	{
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
		ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
	}

	ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005f;

	// gun angles from delta movement
	for (i = 0; i < 3; i++)
	{
		delta = ent->client->oldviewangles[i] - ent->client->ps.viewangles[i];
		if (delta > 180)
			delta -= 360;
		if (delta < -180)
			delta += 360;
		if (delta > 45)
			delta = 45;
		if (delta < -45)
			delta = -45;
		if (i == YAW)
			ent->client->ps.gunangles[ROLL] += 0.1 * delta;
		ent->client->ps.gunangles[i] += 0.2 * delta;
	}

	// gun height
	VectorClear3(ent->client->ps.gunoffset);
	//	ent->ps->gunorigin[2] += bob;

		// gun_x / gun_y / gun_z are development tools
	for (i = 0; i < 3; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i] * (gun_y->value);
		ent->client->ps.gunoffset[i] += right[i] * gun_x->value;
		ent->client->ps.gunoffset[i] += up[i] * (-gun_z->value);
	}
}


/*
=============
SV_AddBlend
=============
*/
void Client_AddBlend(float r, float g, float b, float a, float* v_blend)
{
	float a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1 - v_blend[3]) * a;	// new total alpha
	a3 = v_blend[3] / a2;		// fraction of color from old

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}


/*
=============
SV_CalcBlend
=============
*/
void Client_CalcBlend(edict_t* ent)
{
	int32_t	contents;
	vec3_t	vieworg;
	int32_t	remaining;

	ent->client->ps.blend[0] = ent->client->ps.blend[1] =
		ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	// add for contents
	VectorAdd3(ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents(vieworg);
	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
		Client_AddBlend(1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		Client_AddBlend(0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_WATER)
		Client_AddBlend(0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);

	// add for powerups
	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4))
			Client_AddBlend(0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4))
			Client_AddBlend(1, 1, 0, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		remaining = ent->client->enviro_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4))
			Client_AddBlend(0, 1, 0, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		remaining = ent->client->breather_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4))
			Client_AddBlend(0.4, 1, 0.4, 0.04, ent->client->ps.blend);
	}

	// add for damage
	if (ent->client->damage_alpha > 0)
		Client_AddBlend(ent->client->damage_blend[0], ent->client->damage_blend[1]
			, ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.blend);

	if (ent->client->bonus_alpha > 0)
		Client_AddBlend(0.85, 0.7, 0.3, ent->client->bonus_alpha, ent->client->ps.blend);

	// drop the damage value
	ent->client->damage_alpha -= 0.06;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= 0.1;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;
}


/*
=============
P_WorldEffects
=============
*/
void Player_WorldEffects()
{
	bool	breather;
	bool	envirosuit;
	int32_t	waterlevel, old_waterlevel;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 12;	// don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;

	breather = current_client->breather_framenum > level.framenum;
	envirosuit = current_client->enviro_framenum > level.framenum;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		Player_Noise(current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && !waterlevel)
	{
		Player_Noise(current_player, current_player->s.origin, PNOISE_SELF);
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && waterlevel == 3)
	{
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && waterlevel != 3)
	{
		if (current_player->air_finished < level.time)
		{	// gasp for air
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			Player_Noise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else  if (current_player->air_finished < level.time + 11)
		{	// just break surface
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
		// breather or envirosuit give air
		if (breather || envirosuit)
		{
			current_player->air_finished = level.time + 10;

			if (((int32_t)(current_client->breather_framenum - level.framenum) % 25) == 0)
			{
				if (!current_client->breather_sound)
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
				current_client->breather_sound ^= 1;
				Player_Noise(current_player, current_player->s.origin, PNOISE_SELF);
				//FIXME: release a bubble?
			}
		}

		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{	// drown!
			if (current_player->client->next_drown_time < level.time
				&& current_player->health > 0)
			{
				current_player->client->next_drown_time = level.time + 1;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
				else if (rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_time = level.time;

				Player_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if (current_player->health > 0
				&& current_player->pain_debounce_time <= level.time
				&& current_client->invincible_framenum < level.framenum)
			{
				if (rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
				current_player->pain_debounce_time = level.time + 1;
			}

			if (envirosuit)	// take 1/3 damage with envirosuit
				Player_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1 * waterlevel, 0, 0, MOD_LAVA);
			else
				Player_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 3 * waterlevel, 0, 0, MOD_LAVA);
		}

		if (current_player->watertype & CONTENTS_SLIME)
		{
			if (!envirosuit)
			{	// no damage from slime with envirosuit
				Player_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1 * waterlevel, 0, 0, MOD_SLIME);
			}
		}
	}
}


/*
===============
G_SetClientEffects
===============
*/
void Client_SetEffects(edict_t* ent)
{
	int32_t	pa_type = Armor_GetCurrentPowerArmor(ent);
	int32_t	remaining;

	ent->s.effects = 0;
	ent->s.renderfx = 0;

	if (ent->health <= 0 || level.intermissiontime)
		return;

	if (ent->powerarmor_time > level.time
		&& pa_type == POWER_ARMOR_SHIELD)
	{
		pa_type = Armor_GetCurrentPowerArmor(ent);

		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_GREEN;
	}

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = (int32_t)ent->client->quad_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4))
			ent->s.effects |= EF_QUAD;
	}

	if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = (int32_t)ent->client->invincible_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4))
			ent->s.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
}


/*
===============
G_SetClientEvent
===============
*/
void Client_SetEvent(edict_t* ent)
{
	if (ent->s.event)
		return;

	if (ent->groundentity && xyspeed > 225)
	{
		if ((int32_t)(current_client->bobtime + bobmove) != bobcycle)
			ent->s.event = EV_FOOTSTEP;
	}
}

/*
===============
G_SetClientSound
===============
*/
void Client_SetSound(edict_t* ent)
{
	char* weap;

	if (ent->client->pers.weapon)
		weap = ent->client->pers.weapon->classname;
	else
		weap = "";

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
		ent->s.sound = snd_fry_index;
	// stupid hack
	else if (strcmp(weap, "weapon_railgun") == 0)
		ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
	else if (ent->client->weapon_sound)
		ent->s.sound = ent->client->weapon_sound;
	else
		ent->s.sound = 0;
}

/*
===============
G_SetClientFrame
===============
*/
void Client_SetFrame(edict_t* ent)
{
	gclient_t*	client;
	bool		duck, run;

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;
	if (xyspeed)
		run = true;
	else
		run = false;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
		goto newanim;
	if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
		goto newanim;
	if (!ent->groundentity && client->anim_priority <= ANIM_WAVE)
		goto newanim;

	if (client->anim_priority == ANIM_REVERSE)
	{
		if (ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end
		&& !(level.framenum % (int32_t)(0.1f / TICK_TIME))) // this is a complete hack until we have a new time-based animation system. DON'T MODIFY !
	{	// continue an animation
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return;		// stay there

	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return;		// stay there
		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
		client->anim_priority = ANIM_JUMP;
		if (ent->s.frame != FRAME_jump2)
			ent->s.frame = FRAME_jump1;
		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{	// running
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{	// standing
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}
