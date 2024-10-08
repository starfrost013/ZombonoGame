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
// g_ai.c

#include <game_local.h>

bool AI_FindTarget (edict_t *self);
extern cvar_t* sv_maxclients;

bool AI_CheckForAttack (edict_t *self, float dist);

bool	enemy_vis;
bool	enemy_infront;
int32_t	enemy_range;
float	enemy_yaw;

//============================================================================


/*
=================
AI_SetSightClient

Called once each frame to set level.sight_client to the
player to be checked for in findtarget.

If all clients are either dead or in notarget, sight_client
will be null.

In coop games, sight_client will cycle between the clients.
=================
*/
void AI_SetSightClient ()
{
	edict_t*	ent;
	int32_t		start, check;

	if (level.sight_client == NULL)
		start = 1;
	else
		start = level.sight_client - g_edicts;

	check = start;
	while (1)
	{
		check++;
		if (check > game.maxclients)
			check = 1;
		ent = &g_edicts[check];
		if (ent->inuse
			&& ent->health > 0
			&& !(ent->flags & FL_NOTARGET) )
		{
			level.sight_client = ent;
			return;		// got one
		}
		if (check == start)
		{
			level.sight_client = NULL;
			return;		// nobody to see
		}
	}
}

//============================================================================

/*
=============
ai_move

Move the specified distance at current facing.
==============
*/
void AI_Move (edict_t *self, float dist)
{
	AI_MoveWalk (self, self->s.angles[YAW], dist);
}


/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
void AI_Stand (edict_t *self, float dist)
{
	vec3_t	v;

	if (dist)
		AI_MoveWalk (self, self->s.angles[YAW], dist);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		if (self->enemy)
		{
			VectorSubtract3 (self->enemy->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);
			if (self->s.angles[YAW] != self->ideal_yaw && self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
			{
				self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
				self->monsterinfo.run (self);
			}
			AI_ChangeYaw (self);
			AI_CheckForAttack (self, 0);
		}
		else
			AI_FindTarget (self);
		return;
	}

	if (AI_FindTarget (self))
		return;
	
	if (level.time > self->monsterinfo.pausetime)
	{
		self->monsterinfo.walk (self);
		return;
	}

	if (!(self->spawnflags & 1) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.idle (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
}


/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void AI_Walk (edict_t *self, float dist)
{
	AI_MoveToGoal (self, dist);

	// check for noticing a player
	if (AI_FindTarget (self))
		return;

	if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.search (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
	else if (!self->monsterinfo.search
		&& self->monsterinfo.aiflags & AI_WANDER)
	{
		// if we haven't moved long enough yet to change wander

		if (self->monsterinfo.wander_steps > self->monsterinfo.wander_steps_total)
		{
			// walk in a random direction
			self->ideal_yaw = rand() % 360;

			AI_ChangeYaw(self);

			// generate a number from zero to highest wand erstuep
			self->monsterinfo.wander_steps_total = rand() % (self->monsterinfo.wander_steps_max + 1);

			// if it's lower than the minimu
			while (self->monsterinfo.wander_steps_total < self->monsterinfo.wander_steps_min)
			{
				self->monsterinfo.wander_steps_total = rand() % (self->monsterinfo.wander_steps_max + 1);
			}

			self->monsterinfo.wander_steps = 0;
		}


		// wander randomly
		AI_MoveWalk(self, self->ideal_yaw, dist);

		self->monsterinfo.wander_steps++;
	}
}


/*
=============
ai_charge

Turns towards target and advances
Use this call with a distnace of 0 to replace ai_face
==============
*/
void AI_Charge (edict_t *self, float dist)
{
	vec3_t	v;

	VectorSubtract3 (self->enemy->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);
	AI_ChangeYaw (self);

	if (dist)
		AI_MoveWalk (self, self->s.angles[YAW], dist);
}


/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void AI_Turn (edict_t *self, float dist)
{
	if (dist)
		AI_MoveWalk (self, self->s.angles[YAW], dist);

	if (AI_FindTarget (self))
		return;
	
	AI_ChangeYaw (self);
}


/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

/*
=============
range

returns the range catagorization of an entity reletive to self
0	melee range, will become hostile even if back is turned
1	visibility and infront, or visibility and show hostile
2	infront and show hostile
3	only triggered by damage
=============
*/
int32_t AI_GetRange (edict_t *self, edict_t *other)
{
	vec3_t	v;
	float	len;

	VectorSubtract3 (self->s.origin, other->s.origin, v);
	len = VectorLength3 (v);
	if (len < MELEE_DISTANCE)
		return RANGE_MELEE;
	if (len < 500)
		return RANGE_NEAR;
	if (len < 1000)
		return RANGE_MID;
	return RANGE_FAR;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
bool Edict_CanSee (edict_t *self, edict_t *other)
{
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;

	VectorCopy3 (self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy3 (other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace (spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);
	
	if (trace.fraction == 1.0)
		return true;
	return false;
}


/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
bool Edict_IsInFront (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract3 (other->s.origin, self->s.origin, vec);
	VectorNormalize3 (vec);
	dot = DotProduct3 (vec, forward);
	
	if (dot > 0.3)
		return true;
	return false;
}


//============================================================================

void AI_HuntTarget (edict_t *self)
{
	vec3_t	vec;

	self->goalentity = self->enemy;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.stand (self);
	else
		self->monsterinfo.run (self);
	VectorSubtract3 (self->enemy->s.origin, self->s.origin, vec);
	self->ideal_yaw = vectoyaw(vec);
	// wait a while before first attack
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		AI_AttackFinished (self, 1);
}

void AI_FoundTarget (edict_t *self)
{
	// let other monsters see this monster for a while
	if (self->enemy->client)
	{
		level.sight_entity = self;
		level.sight_entity_framenum = level.framenum;
		level.sight_entity->light_level = 128;
	}

	self->show_hostile = level.time + 1;		// wake up other monsters

	VectorCopy3(self->enemy->s.origin, self->monsterinfo.last_sighting);
	self->monsterinfo.trail_time = level.time;

	if (!self->combattarget)
	{
		AI_HuntTarget (self);
		return;
	}

	self->goalentity = self->movetarget = Edict_PickTarget(self->combattarget);
	if (!self->movetarget)
	{
		self->goalentity = self->movetarget = self->enemy;
		AI_HuntTarget (self);
		gi.dprintf("%s at %s, combattarget %s not found\n", self->classname, vtos(self->s.origin), self->combattarget);
		return;
	}

	// clear out our combattarget, these are a one shot deal
	self->combattarget = NULL;
	self->monsterinfo.aiflags |= AI_COMBAT_POINT;

	// clear the targetname, that point is ours!
	self->movetarget->targetname = NULL;
	self->monsterinfo.pausetime = 0;

	// run for it
	self->monsterinfo.run (self);
}


/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
bool AI_FindTarget (edict_t *self)
{
	edict_t*	client;
	bool		heardit;
	int32_t		r;

	if (self->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (self->goalentity && self->goalentity->inuse && self->goalentity->classname)
		{
			if (strcmp(self->goalentity->classname, "target_actor") == 0)
				return false;
		}

		//FIXME look for monsters?
		return false;
	}

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry or hearing
// something

// revised behavior so they will wake up if they "see" a player make a noise
// but not weapon impact/explosion noises

	heardit = false;
	if ((level.sight_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1) )
	{
		client = level.sight_entity;
		if (client->enemy == self->enemy)
		{
			return false;
		}
	}
	else if (level.sound_entity_framenum >= (level.framenum - 1))
	{
		client = level.sound_entity;
		heardit = true;
	}
	else if (!(self->enemy) && (level.sound2_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1) )
	{
		client = level.sound2_entity;
		heardit = true;
	}
	else
	{
		client = level.sight_client;
		if (!client)
			return false;	// no clients to get mad at
	}

	// if the entity went away, forget it
	if (!client->inuse)
		return false;

	if (client == self->enemy)
		return true;	// JDC false;

	if (client->client)
	{
		if (client->flags & FL_NOTARGET)
			return false;
	}
	else if (client->svflags & SVF_MONSTER)
	{
		if (!client->enemy)
			return false;
		if (client->enemy->flags & FL_NOTARGET)
			return false;
	}
	else if (heardit)
	{
		if (client->owner->flags & FL_NOTARGET)
			return false;
	}
	else
		return false;

	// if item and zombie friendly fire is off, 
	if (self->team == client->team
		&& !((int32_t)gameflags->value & GF_ITEM_FRIENDLY_FIRE))
	{
		return false;
	}

	if (!heardit)
	{
		r = AI_GetRange (self, client);

		if (r == RANGE_FAR)
			return false;

		// this is where we would check invisibility

		// is client in an spot too dark to be seen?
		if (client->light_level <= 5)
			return false;

		if (!Edict_CanSee (self, client))
		{
			return false;
		}

		if (r == RANGE_NEAR)
		{
			if (client->show_hostile < level.time && !Edict_IsInFront (self, client))
			{
				return false;
			}
		}
		else if (r == RANGE_MID)
		{
			if (!Edict_IsInFront (self, client))
			{
				return false;
			}
		}

		self->enemy = client;

		if (strcmp(self->enemy->classname, "player_noise") != 0)
		{
			self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self->enemy->client)
			{
				self->enemy = self->enemy->enemy;
				if (!self->enemy->client)
				{
					self->enemy = NULL;
					return false;
				}
			}
		}
	}
	else	// heardit
	{
		vec3_t	temp;

		if (self->spawnflags & 1)
		{
			if (!Edict_CanSee (self, client))
				return false;
		}
		else
		{
			if (!gi.inPHS(self->s.origin, client->s.origin))
				return false;
		}

		VectorSubtract3 (client->s.origin, self->s.origin, temp);

		if (VectorLength3(temp) > 1000)	// too far to hear
		{
			return false;
		}

		// check area portals - if they are different and not connected then we can't hear it
		if (client->areanum != self->areanum)
			if (!gi.AreasConnected(self->areanum, client->areanum))
				return false;

		self->ideal_yaw = vectoyaw(temp);
		AI_ChangeYaw (self);

		// hunt the sound for a bit; hopefully find the real player
		self->monsterinfo.aiflags |= AI_SOUND_TARGET;
		self->enemy = client;
	}

//
// got one
//
	AI_FoundTarget (self);

	if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight))
		self->monsterinfo.sight (self, self->enemy);

	return true;
}


//=============================================================================

/*
============
FacingIdeal

============
*/
bool AI_FacingIdeal(edict_t *self)
{
	float	delta;

	delta = anglemod(self->s.angles[YAW] - self->ideal_yaw);
	if (delta > 45 && delta < 315)
		return false;
	return true;
}


//=============================================================================

bool AI_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	float	chance;
	trace_t	tr;

	if (self->enemy->health > 0)
	{
	// see if any entities are in the way of the shot
		VectorCopy3 (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy3 (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}
	
	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (rand()&3) )
			return false;
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
// missile attack
	if (!self->monsterinfo.attack)
		return false;
		
	if (level.time < self->monsterinfo.attack_finished)
		return false;
		
	if (enemy_range == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		chance = 0.4;
	}
	else if (enemy_range == RANGE_MELEE)
	{
		chance = 0.2;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.1;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.02;
	}
	else
	{
		return false;
	}

	if (skill->value == 0)
		chance *= 0.5;
	else if (skill->value >= 2)
		chance *= 2;

	if (random () < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.3)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}


/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
void AI_RunMelee(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	AI_ChangeYaw (self);

	if (AI_FacingIdeal(self))
	{
		self->monsterinfo.melee (self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
void AI_RunMissile(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	AI_ChangeYaw (self);

	if (AI_FacingIdeal(self))
	{
		self->monsterinfo.attack (self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
};


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void AI_RunSlide(edict_t *self, float distance)
{
	float	ofs;
	
	self->ideal_yaw = enemy_yaw;
	AI_ChangeYaw (self);

	if (self->monsterinfo.lefty)
		ofs = 90;
	else
		ofs = -90;
	
	if (AI_MoveWalk (self, self->ideal_yaw + ofs, distance))
		return;
		
	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	AI_MoveWalk (self, self->ideal_yaw - ofs, distance);
}


/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
bool AI_CheckForAttack (edict_t *self, float dist)
{
	vec3_t	temp;
	bool	hesDeadJim;

// this causes monsters to run blindly to the combat point w/o firing
	if (self->goalentity)
	{
		if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
		{
			if ((level.time - self->enemy->teleport_time) > 5.0)
			{
				if (self->goalentity == self->enemy)
					if (self->movetarget)
						self->goalentity = self->movetarget;
					else
						self->goalentity = NULL;
				self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
				if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			}
			else
			{
				self->show_hostile = level.time + 1;
				return false;
			}
		}
	}

	enemy_vis = false;

// see if the enemy is dead
	hesDeadJim = false;
	if ((!self->enemy) || (!self->enemy->inuse))
	{
		hesDeadJim = true;
	}
	else if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (self->enemy->health > 0)
		{
			hesDeadJim = true;
			self->monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else
	{
		if (self->monsterinfo.aiflags & AI_BRUTAL)
		{
			if (self->enemy->health <= -80)
				hesDeadJim = true;
		}
		else
		{
			if (self->enemy->health <= 0)
				hesDeadJim = true;
		}
	}

	if (hesDeadJim)
	{
		self->enemy = NULL;
	// FIXME: look all around for other targets
		if (self->oldenemy && self->oldenemy->health > 0)
		{
			self->enemy = self->oldenemy;
			self->oldenemy = NULL;
			AI_HuntTarget (self);
		}
		else
		{
			if (self->movetarget)
			{
				self->goalentity = self->movetarget;
				self->monsterinfo.walk (self);
			}
			else
			{
				// we need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self->monsterinfo.pausetime = level.time + 100000000;
				self->monsterinfo.stand (self);
			}
			return true;
		}
	}

	self->show_hostile = level.time + 1;		// wake up other monsters

// check knowledge of enemy
	enemy_vis = Edict_CanSee(self, self->enemy);
	if (enemy_vis)
	{
		self->monsterinfo.search_time = level.time + 5;
		VectorCopy3 (self->enemy->s.origin, self->monsterinfo.last_sighting);
	}

// look for other coop players here
//	if (coop && self->monsterinfo.search_time < level.time)
//	{
//		if (FindTarget (self))
//			return true;
//	}

	enemy_infront = Edict_IsInFront(self, self->enemy);
	enemy_range = AI_GetRange(self, self->enemy);
	VectorSubtract3 (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);

	// JDC self->ideal_yaw = enemy_yaw;

	if (self->monsterinfo.attack_state == AS_MISSILE)
	{
		AI_RunMissile (self);
		return true;
	}
	if (self->monsterinfo.attack_state == AS_MELEE)
	{
		AI_RunMelee (self);
		return true;
	}

	// if enemy is not currently visible, we will never attack
	if (!enemy_vis)
		return false;

	return self->monsterinfo.checkattack (self);
}


/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void AI_Run (edict_t *self, float dist)
{
	vec3_t		v;
	edict_t*	tempgoal;
	edict_t*	save;
	bool		new;
	edict_t*	marker;
	float		d1, d2;
	trace_t		tr;
	vec3_t		v_forward, v_right;
	float		left, center, right;
	vec3_t		left_target, right_target;

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
	{
		AI_MoveToGoal (self, dist);
		return;
	}

	if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
	{
		VectorSubtract3 (self->s.origin, self->enemy->s.origin, v);
		if (VectorLength3(v) < 64)
		{
			self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.stand (self);
			return;
		}

		AI_MoveToGoal (self, dist);

		if (!AI_FindTarget (self))
			return;
	}

	if (AI_CheckForAttack (self, dist))
		return;

	if (self->monsterinfo.attack_state == AS_SLIDING)
	{
		AI_RunSlide (self, dist);
		return;
	}

	if (enemy_vis)
	{
//		if (self.aiflags & AI_LOST_SIGHT)
//			dprint("regained sight\n");
		AI_MoveToGoal (self, dist);
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		VectorCopy3 (self->enemy->s.origin, self->monsterinfo.last_sighting);
		self->monsterinfo.trail_time = level.time;
		return;
	}


	if ((self->monsterinfo.search_time) && (level.time > (self->monsterinfo.search_time + 20)))
	{
		AI_MoveToGoal (self, dist);
		self->monsterinfo.search_time = 0;
//		dprint("search timeout\n");
		return;
	}

	save = self->goalentity;
	tempgoal = Edict_Spawn();
	self->goalentity = tempgoal;

	new = false;

	if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
	{
		// just lost sight of the player, decide where to go first
//		dprint("lost sight of player, last seen at "); dprint(vtos(self.last_sighting)); dprint("\n");
		self->monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
		self->monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		new = true;
	}

	if (self->monsterinfo.aiflags & AI_PURSUE_NEXT)
	{
		self->monsterinfo.aiflags &= ~AI_PURSUE_NEXT;
//		dprint("reached current goal: "); dprint(vtos(self.origin)); dprint(" "); dprint(vtos(self.last_sighting)); dprint(" "); dprint(ftos(vlen(self.origin - self.last_sighting))); dprint("\n");

		// give ourself more time since we got this far
		self->monsterinfo.search_time = level.time + 5;

		if (self->monsterinfo.aiflags & AI_PURSUE_TEMP)
		{
//			dprint("was temp goal; retrying original\n");
			self->monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
			marker = NULL;
			VectorCopy3 (self->monsterinfo.saved_goal, self->monsterinfo.last_sighting);
			new = true;
		}
		else if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
		{
			self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
			marker = PlayerTrail_PickFirst (self);
		}
		else
		{
			marker = PlayerTrail_PickNext (self);
		}

		if (marker)
		{
			VectorCopy3 (marker->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = marker->timestamp;
			self->s.angles[YAW] = self->ideal_yaw = marker->s.angles[YAW];
//			dprint("heading is "); dprint(ftos(self.ideal_yaw)); dprint("\n");

//			debug_drawline(self.origin, self.last_sighting, 52);
			new = true;
		}
	}

	VectorSubtract3 (self->s.origin, self->monsterinfo.last_sighting, v);
	d1 = VectorLength3(v);
	if (d1 <= dist)
	{
		self->monsterinfo.aiflags |= AI_PURSUE_NEXT;
		dist = d1;
	}

	VectorCopy3 (self->monsterinfo.last_sighting, self->goalentity->s.origin);

	if (new)
	{
//		gi.dprintf("checking for course correction\n");

		tr = gi.trace(self->s.origin, self->mins, self->maxs, self->monsterinfo.last_sighting, self, MASK_PLAYERSOLID);
		if (tr.fraction < 1)
		{
			VectorSubtract3 (self->goalentity->s.origin, self->s.origin, v);
			d1 = VectorLength3(v);
			center = tr.fraction;
			d2 = d1 * ((center+1)/2);
			self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
			AngleVectors(self->s.angles, v_forward, v_right, NULL);

			VectorSet3(v, d2, -16, 0);
			Game_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, left_target, self, MASK_PLAYERSOLID);
			left = tr.fraction;

			VectorSet3(v, d2, 16, 0);
			Game_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, right_target, self, MASK_PLAYERSOLID);
			right = tr.fraction;

			center = (d1*center)/d2;
			if (left >= center && left > right)
			{
				if (left < 1)
				{
					VectorSet3(v, d2 * left * 0.5f, -16, 0);
					Game_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
//					gi.dprintf("incomplete path, go part way and adjust again\n");
				}
				VectorCopy3 (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy3 (left_target, self->goalentity->s.origin);
				VectorCopy3 (left_target, self->monsterinfo.last_sighting);
				VectorSubtract3 (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
//				gi.dprintf("adjusted left\n");
//				debug_drawline(self.origin, self.last_sighting, 152);
			}
			else if (right >= center && right > left)
			{
				if (right < 1)
				{
					VectorSet3(v, d2 * right * 0.5f, 16, 0);
					Game_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
//					gi.dprintf("incomplete path, go part way and adjust again\n");
				}
				VectorCopy3 (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy3 (right_target, self->goalentity->s.origin);
				VectorCopy3 (right_target, self->monsterinfo.last_sighting);
				VectorSubtract3 (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
//				gi.dprintf("adjusted right\n");
//				debug_drawline(self.origin, self.last_sighting, 152);
			}
		}
//		else gi.dprintf("course was fine\n");
	}

	AI_MoveToGoal (self, dist);

	Edict_Free(tempgoal);

	if (self)
		self->goalentity = save;
}
