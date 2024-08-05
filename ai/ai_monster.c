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

//
// Monster utility functions
//

static void AI_TurnOffFliesEffect(edict_t* self)
{
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

static void AI_TurnOnFliesEffect(edict_t* self)
{
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex("infantry/inflies1.wav");
	self->think = AI_TurnOffFliesEffect;
	self->nextthink = level.time + 60;
}

void AI_CheckFliesEffect(edict_t* self)
{
	if (self->waterlevel)
		return;

	if (random() > 0.5)
		return;

	self->think = AI_TurnOnFliesEffect;
	self->nextthink = level.time + 5 + 10 * random();
}

void AI_AttackFinished(edict_t* self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}


void AI_CheckGround(edict_t* ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->flags & (FL_SWIM | FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

	// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if (trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy3(trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}


void AI_CategorizePosition(edict_t* ent)
{
	vec3_t		point;
	int32_t		cont;

	//
	// get waterlevel
	//
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void AI_MonsterWorldEffects(edict_t* ent)
{
	int32_t	dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{	// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					Player_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{	// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floorf(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					Player_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}

	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2f;
			Player_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10 * ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			Player_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4 * ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}

	if (!(ent->flags & FL_INWATER))
	{
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5)
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void AI_MonsterDropToFloor(edict_t* ent)
{
	vec3_t		end;
	trace_t		trace;

	ent->s.origin[2] += 1;
	VectorCopy3(ent->s.origin, end);
	end[2] -= 256;

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy3(trace.endpos, ent->s.origin);

	gi.Edict_Link(ent);
	AI_CheckGround(ent);
	AI_CategorizePosition(ent);
}


void AI_MonsterSetEffects(edict_t* ent)
{
	ent->s.effects &= ~(EF_COLOR_SHELL);
	ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time
		&& ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_GREEN;
	}
}

void AI_MonsterMove(edict_t* self)
{
	mmove_t* move;
	int32_t	index;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		if (self->s.frame == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc(self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				self->s.frame++;
				if (self->s.frame > move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc(self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc(self, 0);

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);
}


void AI_MonsterThink(edict_t* self)
{
	AI_MonsterMove(self);

	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		AI_CheckGround(self);
	}

	AI_CategorizePosition(self);
	AI_MonsterWorldEffects(self);
	AI_MonsterSetEffects(self);
}

/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void AI_MonsterActivate(edict_t* self, edict_t* other, edict_t* activator)
{
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;

	// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	AI_FoundTarget(self);
}


void AI_MonsterStartGo(edict_t* self);

void AI_MonsterTriggeredSpawn(edict_t* self)
{
	self->s.origin[2] += 1;
	Game_KillBox(self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.Edict_Link(self);

	AI_MonsterStartGo(self);

	if (self->enemy && !(self->spawnflags & 1) && !(self->enemy->flags & FL_NOTARGET))
	{
		AI_FoundTarget(self);
	}
	else
	{
		self->enemy = NULL;
	}
}

void AI_MonsterTriggeredSpawnUse(edict_t* self, edict_t* other, edict_t* activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = AI_MonsterTriggeredSpawn;
	self->nextthink = level.time + FRAMETIME;
	if (activator->client)
		self->enemy = activator;
	self->use = AI_MonsterActivate;
}

void AI_MonsterTriggeredStart(edict_t* self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = AI_MonsterTriggeredSpawnUse;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void AI_MonsterStartUse(edict_t* self)
{
	self->flags &= ~(FL_FLY | FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Item_Drop(self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	Edict_UseTargets(self, self->enemy);
}


//============================================================================

bool AI_MonsterStart(edict_t* self)
{
	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
		//		gi.dprintf("fixed spawnflags on %s at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12;
	self->use = AI_MonsterActivate;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = AI_CheckAttack;
	VectorCopy3(self->s.origin, self->s.old_origin);

	if (st.item)
	{
		self->item = Item_FindByClassname(st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.currentmove)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	return true;
}

void AI_MonsterStartGo(edict_t* self)
{
	vec3_t	v;

	if (self->health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		bool	notcombat;
		bool	fixup;
		edict_t* target;

		target = NULL;
		notcombat = false;
		fixup = false;
		while ((target = Game_FindEdictByValue(target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->classname, vtos(self->s.origin));
		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t* target;

		target = NULL;
		while ((target = Game_FindEdictByValue(target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->classname, (int32_t)self->s.origin[0], (int32_t)self->s.origin[1], (int32_t)self->s.origin[2],
					self->combattarget, target->classname, (int32_t)target->s.origin[0], (int32_t)target->s.origin[1],
					(int32_t)target->s.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = Edict_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.dprintf("%s can't find target %s at %s\n", self->classname, self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand(self);
		}
		else if (strcmp(self->movetarget->classname, "path_corner") == 0)
		{
			VectorSubtract3(self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk(self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand(self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand(self);
	}

	self->think = AI_MonsterThink;
	self->nextthink = level.time + FRAMETIME;
}


void AI_MonsterWalkGo(edict_t* self)
{
	if (!(self->spawnflags & 2) && level.time < 1)
	{
		AI_MonsterDropToFloor(self);

		if (self->groundentity)
			if (!AI_MoveWalk(self, 0, 0))
				gi.dprintf("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!self->yaw_speed)
		self->yaw_speed = 20;
	self->viewheight = 25;

	AI_MonsterStartGo(self);

	if (self->spawnflags & 2)
		AI_MonsterTriggeredStart(self);
}

void AI_MonsterWalkStart(edict_t* self)
{
	self->think = AI_MonsterWalkGo;
	AI_MonsterStart(self);
}


void AI_MonsterFlyGo(edict_t* self)
{
	if (!AI_MoveWalk(self, 0, 0))
		gi.dprintf("%s in solid at %s\n", self->classname, vtos(self->s.origin));

	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 25;

	AI_MonsterStartGo(self);

	if (self->spawnflags & 2)
		AI_MonsterTriggeredStart(self);
}


void AI_MonsterFlyStart(edict_t* self)
{
	self->flags |= FL_FLY;
	self->think = AI_MonsterFlyGo;
	AI_MonsterStart(self);
}


void AI_MonsterSwimGo(edict_t* self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 10;

	AI_MonsterStartGo(self);

	if (self->spawnflags & 2)
		AI_MonsterTriggeredStart(self);
}

void AI_MonsterSwimStart(edict_t* self)
{
	self->flags |= FL_SWIM;
	self->think = AI_MonsterSwimGo;
	AI_MonsterStart(self);
}


/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/
void AI_MonsterCheckDodge(edict_t* self, vec3_t start, vec3_t dir, int32_t speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25)
			return;
	}
	VectorMA3(start, 8192, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) && (tr.ent->svflags & SVF_MONSTER) && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && Edict_IsInFront(tr.ent, self))
	{
		VectorSubtract3(tr.endpos, start, v);
		eta = (VectorLength3(v) - tr.ent->maxs[0]) / speed;
		tr.ent->monsterinfo.dodge(tr.ent, self, eta);
	}
}
