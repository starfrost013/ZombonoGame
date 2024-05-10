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

// ogre - February 7, 2024

#include <game_local.h>
#include "mob_ogre.h"

static int	sound_idle;
static int	sound_sight1;
static int	sound_sight2;
//static int	sound_sight3;		// make it
static int	sound_pain_light;
static int	sound_pain;
static int	sound_pain_ss;
static int	sound_death;
static int	sound_hit;

void ogre_idle(edict_t* self)
{
	if (random() > 0.8)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

// STAND

void ogre_stand(edict_t* self);

mframe_t ogre_frames_stand1[] =
{
	ai_stand, 0, ogre_idle,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t ogre_move_stand1 = { FRAME_stand1, FRAME_stand15, ogre_frames_stand1, ogre_stand };

void ogre_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &ogre_move_stand1;
}

//
// WALK
//

void ogre_walk1_random(edict_t* self)
{
	if (random() > 0.1)
		self->monsterinfo.nextframe = FRAME_walk1;
}

mframe_t ogre_frames_walk1[] =
{
	ai_walk, 1,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 1,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 3,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, -1, ogre_walk1_random,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL
};
mmove_t ogre_move_walk1 = { FRAME_walk1, FRAME_walk19, ogre_frames_walk1, NULL };

void ogre_walk(edict_t* self)
{
	// use random to get multiple types of run anim when we have the new ogre models
	self->monsterinfo.currentmove = &ogre_move_walk1;
}

//
// RUN
//

void ogre_run(edict_t* self);

mframe_t ogre_frames_start_run[] =
{
	ai_run, 7,  NULL,
	ai_run, 5,  NULL
};
mmove_t ogre_move_start_run = { FRAME_run1, FRAME_run2, ogre_frames_start_run, ogre_run };

// this is 18 frames for some reason it crashes despite only being a 15 frame animation because of ogre_move_start_run WTF?
mframe_t ogre_frames_run[] =
{
	ai_run, 3, NULL,
	ai_run, 4, NULL,
	ai_run, 4, NULL,
	ai_run, 5, NULL,
	ai_run, 3, NULL,
	ai_run, 5, NULL,
	ai_run, 3, NULL,
	ai_run, 4, NULL,
	ai_run, 4, NULL,
	ai_run, 5, NULL,
	ai_run, 3, NULL,
	ai_run, 4, NULL,
	ai_run, 4, NULL,
	ai_run, 3, NULL,
	ai_run, 3, NULL,
	ai_run, 3, NULL,
	ai_run, 4, NULL,
	ai_run, 3, NULL,
};

mmove_t ogre_move_run = { FRAME_run3, FRAME_run18, ogre_frames_run, NULL };

void ogre_run(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &ogre_move_stand1;
		return;
	}

	if (self->monsterinfo.currentmove == &ogre_move_walk1 ||
		self->monsterinfo.currentmove == &ogre_move_start_run)
	{
		self->monsterinfo.currentmove = &ogre_move_run;
	}
	else
	{
		self->monsterinfo.currentmove = &ogre_move_start_run;
	}
}


//
// PAIN
//

mframe_t ogre_frames_pain1[] =
{
	ai_move, -1,  NULL,
	ai_move, -3,  NULL, // -4
	ai_move, -3,  NULL, // -7
	ai_move, -2,  NULL, // -9
	ai_move, -1,  NULL, // -10
	ai_move, 0,   NULL, // -10
	ai_move, 1,   NULL, // -9
	ai_move, 3,   NULL, // -6
	ai_move, 3,   NULL, // -3
	ai_move, 2,   NULL, // -1
	ai_move, 1,   NULL, // 0
	ai_move, 0,   NULL  // 0 
};
mmove_t ogre_move_pain1 = { FRAME_paina1, FRAME_paina12, ogre_frames_pain1, ogre_run };

//-14
mframe_t ogre_frames_pain2[] =
{
	ai_move, -1,  NULL, // -1
	ai_move, -1,  NULL, // -2
	ai_move, -1,  NULL, // -3
	ai_move, -2,  NULL, // -5
	ai_move, -3,  NULL, // -7
	ai_move, -2,  NULL, // -9
	ai_move, -1,  NULL, // -10
	ai_move, -1,  NULL, // -11
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL, // -10
	ai_move, 1,  NULL, // -9
	ai_move, 2,  NULL, // -7
	ai_move, 3,  NULL, // -4
	ai_move, 2,  NULL, // -2
	ai_move, 1,  NULL, // -1
	ai_move, 1,  NULL, // 0
	ai_move, 0,  NULL  // 1
};
mmove_t ogre_move_pain2 = { FRAME_painb1, FRAME_painb28, ogre_frames_pain2, ogre_run };

mframe_t ogre_frames_pain3[] =
{
	ai_move, -4, NULL,
	ai_move, 5, NULL,
	ai_move, -2, NULL,
	ai_move, 0, NULL,
	ai_move, -2, NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 3,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 1,  NULL,
	ai_move, 2,  NULL,
	ai_move, 1,  NULL,
	ai_move, 1,  NULL
};
mmove_t ogre_move_pain3 = { FRAME_painc1, FRAME_painc18, ogre_frames_pain3, ogre_run };

mframe_t ogre_frames_pain4[] =
{
	ai_move, -3, NULL,
	ai_move, -2,  NULL,
	ai_move, 3,   NULL,
	ai_move, 2,   NULL,
	ai_move, 1,   NULL,
	ai_move, 0,   NULL,
	ai_move, 1,   NULL,
	ai_move, 3,   NULL,
	ai_move, 1,   NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, 1,   NULL,
	ai_move, 1,   NULL
};
mmove_t ogre_move_pain4 = { FRAME_paind1, FRAME_paind13, ogre_frames_pain4, ogre_run };

void ogre_pain(edict_t* self, edict_t* other, float kick, int32_t damage)
{
	float	r;
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && ((self->monsterinfo.currentmove == &ogre_move_pain1) || (self->monsterinfo.currentmove == &ogre_move_pain2) || (self->monsterinfo.currentmove == &ogre_move_pain3)))
			self->monsterinfo.currentmove = &ogre_move_pain4;
		return;
	}

	self->pain_debounce_time = level.time + 3;

	n = self->s.skinnum | 1;
	if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain_light, 1, ATTN_NORM, 0);
	else if (n == 3)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain_ss, 1, ATTN_NORM, 0);

	if (self->velocity[2] > 100)
	{
		self->monsterinfo.currentmove = &ogre_move_pain4;
		return;
	}

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	r = random();

	if (r < 0.33)
		self->monsterinfo.currentmove = &ogre_move_pain1;
	else if (r < 0.66)
		self->monsterinfo.currentmove = &ogre_move_pain2;
	else
		self->monsterinfo.currentmove = &ogre_move_pain3;
}


//
// ATTACK
//

//main attack code
void ogre_fire(edict_t* self)
{
	vec3_t	aim;
	vec3_t	forward;
	float	attack_random = random();

	//temp
	VectorSet(aim, OGRE_RANGE_MELEE, 0, 16);

	// random chance to fire a rocket
	// test for 3/23/2024
	if (attack_random > 0.75)
	{
		AngleVectors(self->s.angles, forward, NULL, NULL);
		Ammo_Rocket(self, self->s.origin, forward, OGRE_DAMAGE_FUNNY_SHREK, OGRE_SPEED_FUNNY_SHREK, 120, 120);
	}
	else
	{
		// todo: types
		Ammo_Melee(self, aim, OGRE_RANGE_MELEE, 0);
	}


}

// Attack 1:
// The ogre bites you.

void ogre_attack1_refire1(edict_t* self)
{
	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_atta2;
	else
		self->monsterinfo.nextframe = FRAME_atta10;
}

void ogre_attack1_refire2(edict_t* self)
{
	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_atta2;
}

mframe_t ogre_frames_attack1[] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  ogre_fire,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  ogre_attack1_refire1,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  ogre_attack1_refire2,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL
};
mmove_t ogre_move_attack1 = { FRAME_atta1, FRAME_atta12, ogre_frames_attack1, ogre_run };


void ogre_attack2_refire1(edict_t* self)
{
	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attb4;
	else
		self->monsterinfo.nextframe = FRAME_attb14;
}

void ogre_attack2_refire2(edict_t* self)
{
	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attb4;
}

mframe_t ogre_frames_attack2[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ogre_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ogre_attack2_refire1,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0,  ogre_attack2_refire2,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};

mmove_t ogre_move_attack2 = { FRAME_attb1, FRAME_attb14, ogre_frames_attack2, ogre_run };

// ATTACK3 (duck and shoot)

void ogre_duck_down(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1;
	gi.linkentity(self);
}

void ogre_duck_up(edict_t* self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

void ogre_attack3_refire(edict_t* self)
{
	if ((level.time + 0.4) < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = FRAME_attc3;
}

mframe_t ogre_frames_attack3[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ogre_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ogre_attack3_refire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, ogre_duck_up,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
};
mmove_t ogre_move_attack3 = { FRAME_attc1, FRAME_attc12, ogre_frames_attack3, ogre_run };


// ATTACK6 (run & shoot)

void ogre_attack6_refire(edict_t* self)
{
	if (self->enemy->health <= 0)
		return;

	if (range(self, self->enemy) < RANGE_MID)
		return;

	if (skill->value == 3)
		self->monsterinfo.nextframe = FRAME_run3;
}

mframe_t ogre_frames_attack6[] =
{
	ai_charge, 3, NULL,
	ai_charge, 1, NULL,
	ai_charge, 3, NULL,
	ai_charge, 4, ogre_fire,
	ai_charge, 3, NULL,
	ai_charge, 4, NULL,
	ai_charge, 3, NULL,
	ai_charge, 4, NULL,
	ai_charge, 3, NULL,
	ai_charge, 2, NULL,
	ai_charge, 4, NULL,
	ai_charge, 5, NULL,
	ai_charge, 3, NULL,
	ai_charge, 2, ogre_attack6_refire,
	ai_charge, 3,  NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 5, NULL,
};

mmove_t ogre_move_attack6 = { FRAME_run1, FRAME_run18, ogre_frames_attack6, ogre_run };

void ogre_attack(edict_t* self)
{
	if (random() < 0.5)
		self->monsterinfo.currentmove = &ogre_move_attack1;
	else
		self->monsterinfo.currentmove = &ogre_move_attack2;
}

//
// SIGHT
//

void ogre_sight(edict_t* self, edict_t* other)
{
	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);

	if ((skill->value > 0) && (range(self, self->enemy) >= RANGE_MID))
	{
		if (random() > 0.5)
			self->monsterinfo.currentmove = &ogre_move_attack6;
	}
}

//
// DUCK
//

void ogre_duck_hold(edict_t* self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t ogre_frames_duck[] =
{
	ai_move, 5, ogre_duck_down,
	ai_move, -1, ogre_duck_hold,
	ai_move, 1,  NULL,
	ai_move, 0,  ogre_duck_up,
	ai_move, 5,  NULL
};
mmove_t ogre_move_duck = { FRAME_paind1, FRAME_paind5, ogre_frames_duck, ogre_run };

void ogre_dodge(edict_t* self, edict_t* attacker, float eta)
{
	float	r;

	r = random();
	if (r > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	if (skill->value == 0)
	{
		self->monsterinfo.currentmove = &ogre_move_duck;
		return;
	}

	self->monsterinfo.pausetime = level.time + eta + 0.3;
	r = random();

	if (skill->value == 1)
	{
		if (r > 0.33)
			self->monsterinfo.currentmove = &ogre_move_duck;
		else
			self->monsterinfo.currentmove = &ogre_move_attack3;
		return;
	}

	if (skill->value >= 2)
	{
		if (r > 0.66)
			self->monsterinfo.currentmove = &ogre_move_duck;
		else
			self->monsterinfo.currentmove = &ogre_move_attack3;
		return;
	}

	self->monsterinfo.currentmove = &ogre_move_attack3;
}


//
// DEATH
//

void ogre_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

mframe_t ogre_frames_death1[] =
{
	ai_move, 0,   NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -1,  NULL,
	ai_move, 0,   ogre_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   ogre_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};

// TEMP
mmove_t ogre_move_death1 = { FRAME_paine1, FRAME_paine12, ogre_frames_death1, ogre_dead };

mframe_t ogre_frames_death2[] =
{
	ai_move, -1,  NULL,
	ai_move, -2,  NULL,
	ai_move, -1,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
};

// same as ogre_move_death1 but he moves differently and doesn't attack you 
// maybe i will add ogre_move_death3 with a lot of attacks but maybe this is bad in multiplayer so i will remove it!!!!
mmove_t ogre_move_death2 = { FRAME_paine1, FRAME_paine12, ogre_frames_death2, ogre_dead };

void ogre_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	int		n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 3; n++)
			ThrowGib(self, "models/objects/gibs/ogre/ogre_hand.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/ogre/ogre_main.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		G_FreeEdict(self);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum |= 1;
	//todo: add multiple death sounds
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);

	if (fabs((self->s.origin[2] + self->viewheight) - point[2]) <= 4)
	{
		// head shot
		G_FreeEdict(self);
		return;
	}

	n = rand() % 2;
	if (n == 0)
		self->monsterinfo.currentmove = &ogre_move_death1;
	else if (n == 1)
		self->monsterinfo.currentmove = &ogre_move_death2;

	// TODO: temp until not fucked animations
	G_FreeEdict(self);
}

//
// SPAWN
//

void SP_monster_ogre_x(edict_t* self)
{
	self->classname = "monster_ogre"; // for Bamfuslicator
	self->s.modelindex = gi.modelindex("models/monsters/ogre/tris.md2");
	self->monsterinfo.scale = MODEL_SCALE;
	VectorSet(self->mins, -12, -12, -24);
	VectorSet(self->maxs, 12, 12, 24);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->monsterinfo.aiflags |= AI_WANDER;
	// move this?
	self->monsterinfo.wander_steps_min = 30;
	self->monsterinfo.wander_steps_max = 50;

	sound_idle = gi.soundindex("ogre/ogre_idle.wav");
	sound_sight1 = gi.soundindex("ogre/ogre_sight1.wav");
	sound_sight2 = gi.soundindex("ogre/ogre_sight2.wav");
	sound_hit = gi.soundindex("ogre/ogre_hit.wav");
	sound_pain = gi.soundindex("ogre/ogre_pain1.wav");
	sound_death = gi.soundindex("ogre/ogre_gib.wav");

	self->mass = 100;

	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterinfo.stand = ogre_stand;
	self->monsterinfo.walk = ogre_walk;
	self->monsterinfo.run = ogre_run;
	self->monsterinfo.dodge = ogre_dodge;
	self->monsterinfo.attack = ogre_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = ogre_sight;

	gi.linkentity(self);

	self->monsterinfo.stand(self);

	walkmonster_start(self);
}

/*Ambush Trigger_Spawn Sight
*/
SP_monster_ogre(edict_t* self)
{
	SP_monster_ogre_x(self);

	self->s.skinnum = 2;
	self->health = OGRE_HEALTH;
	self->max_health = OGRE_HEALTH;
	self->gib_health = 0; //test
}