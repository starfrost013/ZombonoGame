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
// game_combat.c : Everything related to damage and death!

#include <game_local.h>
#include <mobs/mob_player.h>

void Touch_Item(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int32_t damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	targ->enemy = attacker;

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
//		targ->svflags |= SVF_DEADMONSTER;	// now treat as a different content type
		if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			level.killed_monsters++;

			// In zombono you get points for killing zombies. I need to figure out if this is a good idea.
			if (attacker->client)
			{
				if (attacker->team == targ->team)
				{
					attacker->client->resp.score_monsters--;
				}
				else
				{
					attacker->client->resp.score_monsters++;
				}

			}

			// medics won't heal monsters that they kill themselves
			if (strcmp(attacker->classname, "monster_medic") == 0)
				targ->owner = attacker;
		}
	}

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use (targ);
	}

	targ->die (targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
void SpawnDamage (int32_t type, vec3_t origin, vec3_t normal, int32_t damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int32_t CheckPowerArmor (edict_t *ent, vec3_t point, vec3_t normal, int32_t damage, int32_t dflags)
{
	gclient_t*	client;
	int32_t		save;
	int32_t		power_armor_type;
	loadout_entry_t* loadout_ptr_cells = NULL;
	int32_t		damage_per_cell;
	int32_t		temp_entity_type;
	int32_t		power = 0;
	int32_t		power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		loadout_ptr_cells = Loadout_GetItem(ent, "cells");

		power_armor_type = GetCurrentPowerArmor (ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			power = loadout_ptr_cells->amount;
		}
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec = { 0 };
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorSubtract (point, ent->s.origin, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			return 0;

		damage_per_cell = 1;
		temp_entity_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damage_per_cell = 2;
		temp_entity_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damage_per_cell;
	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage (temp_entity_type, point, normal, save);
	ent->powerarmor_time = level.time + 0.2;

	power_used = save / damage_per_cell;

	if (client)
		loadout_ptr_cells -= power_used;
	else
		ent->monsterinfo.power_armor_power -= power_used;
	return save;
}

static int32_t CheckArmor (edict_t *ent, vec3_t point, vec3_t normal, int32_t damage, int32_t te_sparks, int32_t dflags)
{
	gclient_t	*client;
	int32_t		save;
	loadout_entry_t*	current_armor = GetCurrentArmor(ent);
	gitem_t		*armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (!current_armor)
		return 0;

	armor = GetItemByIndex (current_armor);

	if (dflags & DAMAGE_ENERGY)
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection*damage);
	else
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection*damage);
	if (save >= current_armor->amount)
		save = current_armor->amount;

	if (!save)
		return 0;

	current_armor->amount -= save;
	SpawnDamage (te_sparks, point, normal, save);

	return save;
}

void M_ReactToDamage (edict_t *targ, edict_t *attacker)
{
	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

	if (attacker == targ || attacker == targ->enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy && targ->enemy->client)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}
			targ->oldenemy = targ->enemy;
		}
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	if (((targ->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		 (strcmp (targ->classname, attacker->classname) != 0))
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker->enemy == targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker->enemy;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
}

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int32_t damage, int32_t knockback, int32_t dflags, int32_t mod)
{
	gclient_t	*client;
	int32_t		take;
	int32_t		save;
	int32_t		asave;
	int32_t		psave;
	int32_t		te_sparks;

	if (!targ->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker))
	{
		// prevent team-specific weapons from harming the teams
		// todo: gamemode def for more "dynamic" checks? probably stupid and overengineered

		// hack! (if the target is a player and the attacker isn't)
		if (!strcmp(targ->classname, "player")
			&& strcmp(attacker->classname, "player"))
		{
			if (gamemode->value != GAMEMODE_COOP
				&& gamemode->value != GAMEMODE_WAVES
				&& !((int32_t)gameflags->value & GF_ITEM_FRIENDLY_FIRE))
			{
				if ((targ->team) & (attacker->team))
				{
					return;
				}
			}
		}
		else
		{
			// prevent people getting murdered in prespawn on TDM games by simply making unassigneds invincible
			if (gamemode->value == GAMEMODE_TDM
				&& targ->team == team_unassigned
				|| attacker->team == team_unassigned)
			{
				return false;
			}

			if (targ->team == attacker->team)
			{
				if ((int32_t)(gameflags->value) & GF_NO_FRIENDLY_FIRE)
					damage = 0;
				else
					mod |= MOD_FRIENDLY_FIRE;
			}
		}
	}

	meansOfDeath = mod;

	// easy mode takes half damage
	// easy mode sucks
	if (skill->value == 0 
		&& targ->client)
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

// bonus damage for suprising a monster
	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
		damage *= 2;

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client && attacker == targ)
			{
				// the rocket jump hack...
				VectorScale(dir, 1600.0 * (float)knockback / mass, kvel);	

				// make rocket jumping easier
				//TODO: refactor this whole thing so checks like the rocket jump check can be done in ammo_*
				//does this halve the penalty for stupidity? or does it just allow rocket/grenade/etc (answer in 3/23/2024 0.0.5 playtest0
				damage /= 2;
			}
			else
			{
				VectorScale(dir, 500.0 * (float)knockback / mass, kvel);
			}

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}

	psave = CheckPowerArmor (targ, point, normal, take, dflags);
	take -= psave;

	asave = CheckArmor (targ, point, normal, take, te_sparks, dflags);
	take -= asave;

	//treat cheat/powerup savings the same as armor
	asave += save;

// do the damage
	if (take)
	{
		if ((targ->svflags & SVF_MONSTER) || (client))
			SpawnDamage (TE_BLOOD, point, normal, take);
		else
			SpawnDamage (te_sparks, point, normal, take);


		targ->health = targ->health - take;
			
		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (targ->svflags & SVF_MONSTER)
	{
		M_ReactToDamage (targ, attacker);
		if (!(targ->monsterinfo.aiflags & AI_DUCKED) && (take))
		{
			targ->pain (targ, attacker, knockback, take);
			// nightmare mode monsters don't go into pain frames often
			if (skill->value == 3)
				targ->pain_debounce_time = level.time + 5;
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int32_t mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int32_t)points, (int32_t)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

/* Player specific */


void player_pain(edict_t* self, edict_t* other, float kick, int32_t damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}

bool IsFemale(edict_t* ent)
{
	char* info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey(ent->client->pers.userinfo, "gender");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}

// WOKE!
bool IsOther(edict_t* ent)
{
	char* info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey(ent->client->pers.userinfo, "gender");
	if (info[0] == 'o' || info[0] == 'O')
		return true;
	return false;
}


bool IsNeutral(edict_t* ent)
{
	char* info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey(ent->client->pers.userinfo, "gender");
	if (info[0] != 'f' && info[0] != 'F' && info[0] != 'm' && info[0] != 'M'
		&& info[0] != 'o' && info[0] != 'O')
		return true;
	return false;
}

void ClientObituary(edict_t* self, edict_t* inflictor, edict_t* attacker)
{
	int		mod;
	char*	message;
	char*	message2;
	bool	ff;

	if ((!(int32_t)(gameflags->value) & GF_NO_FRIENDLY_FIRE) && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	ff = meansOfDeath & MOD_FRIENDLY_FIRE;
	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
	message = NULL;
	message2 = "";

	switch (mod)
	{
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_EXPLOSIVE:
	case MOD_BARREL:
		message = "blew up";
		break;
	case MOD_EXIT:
		message = "found a way out";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TARGET_BLASTER:
		message = "got blasted";
		break;
	case MOD_BOMB:
	case MOD_SPLASH:
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	}
	if (attacker == self)
	{
		switch (mod)
		{
		case MOD_HELD_GRENADE:
			message = "tried to put the pin back in";
			break;
		case MOD_HG_SPLASH:
		case MOD_G_SPLASH:
			if (IsNeutral(self))
				message = "tripped on its own grenade";
			else if (IsFemale(self))
				message = "tripped on her own grenade";
			else if (IsOther(self))
				message = "tripped on their own grenade";
			else
				message = "tripped on his own grenade";
			break;
		case MOD_R_SPLASH:
			if (IsNeutral(self))
				message = "blew itself up";
			else if (IsFemale(self))
				message = "blew herself up";
			else if (IsOther(self))
				message = "blew themselves up";
			else
				message = "blew himself up";
			break;
		default:
			if (IsNeutral(self))
				message = "killed itself";
			else if (IsFemale(self))
				message = "killed herself";
			else if (IsOther(self))
				message = "killed themselves";

			else
				message = "killed himself";
			break;
		}
	}
	if (message)
	{
		gi.bprintf(PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
		self->client->resp.score--;
		self->enemy = NULL;
		return;
	}

	self->enemy = attacker;
	if (attacker && attacker->client)
	{
		switch (mod)
		{
		case MOD_BLASTER:
			message = "was blasted by";
			break;
		case MOD_SHOTGUN:
			message = "was gunned down by";
			break;
		case MOD_SSHOTGUN:
			message = "was blown away by";
			message2 = "'s super shotgun";
			break;
		case MOD_MACHINEGUN:
			message = "was machinegunned by";
			break;
		case MOD_CHAINGUN:
			message = "was cut in half by";
			message2 = "'s chaingun";
			break;
		case MOD_GRENADE:
			message = "was popped by";
			message2 = "'s grenade";
			break;
		case MOD_G_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_ROCKET:
			message = "ate";
			message2 = "'s rocket";
			break;
		case MOD_R_SPLASH:
			message = "almost dodged";
			message2 = "'s rocket";
			break;
		case MOD_HYPERBLASTER:
			message = "was melted by";
			message2 = "'s hyperblaster";
			break;
		case MOD_RAILGUN:
			message = "was railed by";
			break;
		case MOD_HANDGRENADE:
			message = "caught";
			message2 = "'s handgrenade";
			break;
		case MOD_HG_SPLASH:
			message = "didn't see";
			message2 = "'s handgrenade";
			break;
		case MOD_HELD_GRENADE:
			message = "feels";
			message2 = "'s pain";
			break;
		case MOD_TELEFRAG:
			message = "tried to invade";
			message2 = "'s personal space";
			break;
		}
		if (message)
		{
			// TODO: send a message if you friendly fired, and then kick
			gi.bprintf(PRINT_MEDIUM, "%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);

			if (ff)
				attacker->client->resp.score--;
			else
				attacker->client->resp.score++;

			return;
		}
	}

	gi.bprintf(PRINT_MEDIUM, "%s died.\n", self->client->pers.netname);

	self->client->resp.score--;
}

void TossClientWeapon(edict_t* self)
{
	gitem_t*	item;
	edict_t*	drop;
	bool		quad;
	float		spread;

	item = self->client->pers.weapon;
	if (!self->client->loadout_current_ammo)
		item = NULL;
	if (item && (strcmp(item->pickup_name, "Blaster") == 0))
		item = NULL;

	if (!((int32_t)(gameflags->value) & GF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + 10));

	if (item && quad)
		spread = 22.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item(self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item(self, FindItemByClassname("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller(edict_t* self, edict_t* inflictor, edict_t* attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract(attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract(inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
		self->client->killer_yaw = 180 / M_PI * atan2(dir[1], dir[0]);
	else {
		self->client->killer_yaw = 0;
		if (dir[1] > 0)
			self->client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->client->killer_yaw = -90;
	}
	if (self->client->killer_yaw < 0)
		self->client->killer_yaw += 360;


}

/*
==================
player_die
==================
*/
void player_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	int		n;

	VectorClear(self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// remove linked weapon model

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 1.0;
		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker);
		TossClientWeapon(self);

		// show scores
		G_LeaderboardSend(self);
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~FL_POWER_ARMOR;

	if (self->health < -40)
	{	// gib
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowClientHead(self, damage);

		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int32_t i;

			i = (i + 1) % 3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1 - 1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death101 - 1;
				self->client->anim_end = FRAME_death106;
				break;
			case 1:
				self->s.frame = FRAME_death201 - 1;
				self->client->anim_end = FRAME_death206;
				break;
			case 2:
				self->s.frame = FRAME_death301 - 1;
				self->client->anim_end = FRAME_death308;
				break;
			}
			gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1, ATTN_NORM, 0);
		}
	}

	self->deadflag = DEAD_DEAD;

	gi.linkentity(self);
}

/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage(edict_t* ent)
{
	float	delta;
	int32_t	damage;
	vec3_t	dir;

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if ((ent->client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity))
	{
		delta = ent->client->oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
			return;
		delta = ent->velocity[2] - ent->client->oldvelocity[2];
	}
	delta = delta * delta * 0.0001;

	// never take falling damage if completely underwater
	if (ent->waterlevel == 3)
		return;
	if (ent->waterlevel == 2)
		delta *= 0.25;
	if (ent->waterlevel == 1)
		delta *= 0.5;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->fall_value = delta * 0.5;
	if (ent->client->fall_value > 40)
		ent->client->fall_value = 40;
	ent->client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent->health > 0)
		{
			if (delta >= 55)
				ent->s.event = EV_FALLFAR;
			else
				ent->s.event = EV_FALL;
		}
		ent->pain_debounce_time = level.time;	// no normal pain sound
		damage = (delta - 30) / 2;
		if (damage < 1)
			damage = 1;
		VectorSet(dir, 0, 0, 1);

		if (!((int32_t)gameflags->value & GF_NO_FALL_DAMAGE))
		{
			T_Damage(ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
		}

	}
	else
	{
		ent->s.event = EV_FALLSHORT;
		return;
	}
}
