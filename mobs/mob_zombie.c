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

// Zombie - November 30, 2023

#include <game_local.h>
#include "mob_zombie.h"

static int	sound_idle;
static int	sound_sight1;
static int	sound_sight2;
static int	sound_pain_light;
static int	sound_pain;
static int	sound_pain_ss;
static int	sound_death_light;
static int	sound_death;
static int	sound_death_ss;
static int	sound_hit;

void zombie_idle(edict_t* self)
{
	if (random() > 0.8)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

// STAND

void zombie_stand(edict_t* self);

mframe_t zombie_frames_stand1[] =
{
	ai_stand, 0, zombie_idle,
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
mmove_t zombie_move_stand1 = { FRAME_stand1, FRAME_stand15, zombie_frames_stand1, zombie_stand };

void zombie_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &zombie_move_stand1;
}

//
// WALK
//

void zombie_walk1_random(edict_t* self)
{
	if (random() > 0.1)
		self->monsterinfo.nextframe = FRAME_walk1;
}

mframe_t zombie_frames_walk1[] =
{
	ai_walk, 3,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 1,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 3,  NULL,
	ai_walk, -1, zombie_walk1_random,
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
mmove_t zombie_move_walk1 = { FRAME_walk1, FRAME_walk19, zombie_frames_walk1, NULL };

void zombie_walk(edict_t* self)
{
	// use random to get multiple types of run anim when we have the new zombie models
	self->monsterinfo.currentmove = &zombie_move_walk1;
}

//
// RUN
//

void zombie_run(edict_t* self);

mframe_t zombie_frames_start_run[] =
{
	ai_run, 7,  NULL,
	ai_run, 5,  NULL
};
mmove_t zombie_move_start_run = { FRAME_run1, FRAME_run2, zombie_frames_start_run, zombie_run };

// this is 18 frames for some reason it crashes despite only being a 15 frame animation because of zombie_move_start_run WTF?
mframe_t zombie_frames_run[] =
{
	ai_run, 10, NULL,
	ai_run, 11, NULL,
	ai_run, 11, NULL,
	ai_run, 16, NULL,
	ai_run, 10, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 11, NULL,
	ai_run, 11, NULL,
	ai_run, 16, NULL,
	ai_run, 10, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 11, NULL,
	ai_run, 11, NULL,
	ai_run, 16, NULL,
	ai_run, 10, NULL,
	ai_run, 15, NULL,
};

mmove_t zombie_move_run = { FRAME_run3, FRAME_run18, zombie_frames_run, NULL };

void zombie_run(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &zombie_move_stand1;
		return;
	}

	if (self->monsterinfo.currentmove == &zombie_move_walk1 ||
		self->monsterinfo.currentmove == &zombie_move_start_run)
	{
		self->monsterinfo.currentmove = &zombie_move_run;
	}
	else
	{
		self->monsterinfo.currentmove = &zombie_move_start_run;
	}
}


//
// PAIN
//

mframe_t zombie_frames_pain1[] =
{
	ai_move, -1,  NULL,
	ai_move, -3,  NULL, // -4
	ai_move, -4,  NULL, // -8
	ai_move, -2,  NULL, // -10
	ai_move, -1,  NULL, // -11
	ai_move, 0,   NULL, // -11
	ai_move, 1,   NULL, // -10
	ai_move, 3,   NULL, // -7
	ai_move, 4,   NULL, // -3
	ai_move, 2,   NULL, // -1
	ai_move, 1,   NULL, // 0
	ai_move, 0,   NULL  // 0 
};
mmove_t zombie_move_pain1 = { FRAME_paina1, FRAME_paina12, zombie_frames_pain1, zombie_run };

//-14
mframe_t zombie_frames_pain2[] =
{
	ai_move, -1,  NULL, // -1
	ai_move, -1,  NULL, // -2
	ai_move, -2,  NULL, // -4
	ai_move, -3,  NULL, // -7
	ai_move, -3,  NULL, // -10
	ai_move, -2,  NULL, // -12
	ai_move, -1,  NULL, // -13
	ai_move, -1,  NULL, // -14
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
	ai_move, 1,  NULL, // 1
	ai_move, 1,  NULL, // 2
	ai_move, 2,  NULL, // 4
	ai_move, 3,  NULL, // 7
	ai_move, 3,  NULL, // 10
	ai_move, 2,  NULL, // 12
	ai_move, 1,  NULL, // 13
	ai_move, 1,  NULL  // 14
};
mmove_t zombie_move_pain2 = { FRAME_painb1, FRAME_painb28, zombie_frames_pain2, zombie_run };

mframe_t zombie_frames_pain3[] =
{
	ai_move, -8, NULL,
	ai_move, 10, NULL,
	ai_move, -4, NULL,
	ai_move, -1, NULL,
	ai_move, -3, NULL,
	ai_move, 0,  NULL,
	ai_move, 3,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 2,  NULL,
	ai_move, 4,  NULL,
	ai_move, 3,  NULL,
	ai_move, 2,  NULL
};
mmove_t zombie_move_pain3 = { FRAME_painc1, FRAME_painc18, zombie_frames_pain3, zombie_run };

mframe_t zombie_frames_pain4[] =
{
	ai_move, -10, NULL,
	ai_move, -6,  NULL,
	ai_move, 8,   NULL,
	ai_move, 4,   NULL,
	ai_move, 1,   NULL,
	ai_move, 0,   NULL,
	ai_move, 2,   NULL,
	ai_move, 5,   NULL,
	ai_move, 2,   NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, 3,   NULL,
	ai_move, 2,   NULL
};
mmove_t zombie_move_pain4 = { FRAME_paind1, FRAME_paind13, zombie_frames_pain4, zombie_run };


void zombie_pain(edict_t* self, edict_t* other, float kick, int32_t damage)
{
	float	r;
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && ((self->monsterinfo.currentmove == &zombie_move_pain1) || (self->monsterinfo.currentmove == &zombie_move_pain2) || (self->monsterinfo.currentmove == &zombie_move_pain3)))
			self->monsterinfo.currentmove = &zombie_move_pain4;
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
		self->monsterinfo.currentmove = &zombie_move_pain4;
		return;
	}

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	r = random();

	if (r < 0.33)
		self->monsterinfo.currentmove = &zombie_move_pain1;
	else if (r < 0.66)
		self->monsterinfo.currentmove = &zombie_move_pain2;
	else
		self->monsterinfo.currentmove = &zombie_move_pain3;
}


//
// ATTACK
//

void zombie_fire(edict_t* self)
{
	vec3_t	aim;

	VectorSet(aim, ZOMBIE_RANGE_STANDARD, 0, 16);
	// todo: types
	Ammo_Melee(self, aim, ZOMBIE_DAMAGE_STANDARD, 0);
}

// Attack 1:
// The zombie bites you.

void zombie_attack1_refire1(edict_t* self)
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

void zombie_attack1_refire2(edict_t* self)
{
	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_atta2;
}

mframe_t zombie_frames_attack1[] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  zombie_fire,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  zombie_attack1_refire1,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  zombie_attack1_refire2,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL
};
mmove_t zombie_move_attack1 = { FRAME_atta1, FRAME_atta12, zombie_frames_attack1, zombie_run };


void zombie_attack2_refire1(edict_t* self)
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

void zombie_attack2_refire2(edict_t* self)
{
	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attb4;
}

mframe_t zombie_frames_attack2[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, zombie_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, zombie_attack2_refire1,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0,  zombie_attack2_refire2,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};

mmove_t zombie_move_attack2 = { FRAME_attb1, FRAME_attb14, zombie_frames_attack2, zombie_run };

// ATTACK3 (duck and shoot)

void zombie_duck_down(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1;
	gi.linkentity(self);
}

void zombie_duck_up(edict_t* self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

void zombie_attack3_refire(edict_t* self)
{
	if ((level.time + 0.4) < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = FRAME_attc3;
}

mframe_t zombie_frames_attack3[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, zombie_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, zombie_attack3_refire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, zombie_duck_up,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
};
mmove_t zombie_move_attack3 = { FRAME_attc1, FRAME_attc12, zombie_frames_attack3, zombie_run };


// ATTACK6 (run & shoot)

void zombie_attack6_refire(edict_t* self)
{
	if (self->enemy->health <= 0)
		return;

	if (range(self, self->enemy) < RANGE_MID)
		return;

	if (skill->value == 3)
		self->monsterinfo.nextframe = FRAME_run3;
}

mframe_t zombie_frames_attack6[] =
{
	ai_charge, 10, NULL,
	ai_charge,  4, NULL,
	ai_charge, 12, NULL,
	ai_charge, 11, zombie_fire,
	ai_charge, 13, NULL,
	ai_charge, 18, NULL,
	ai_charge, 15, NULL,
	ai_charge, 14, NULL,
	ai_charge, 11, NULL,
	ai_charge,  8, NULL,
	ai_charge, 11, NULL,
	ai_charge, 12, NULL,
	ai_charge, 12, NULL,
	ai_charge, 17, zombie_attack6_refire,
	ai_charge, 9,  NULL,
	ai_charge, 12, NULL,
	ai_charge, 12, NULL,
	ai_charge, 17, NULL,
};

mmove_t zombie_move_attack6 = { FRAME_run1, FRAME_run18, zombie_frames_attack6, zombie_run };

void zombie_attack(edict_t* self)
{
	if (random() < 0.5)
		self->monsterinfo.currentmove = &zombie_move_attack1;
	else
		self->monsterinfo.currentmove = &zombie_move_attack2;
}

//
// SIGHT
//

void zombie_sight(edict_t* self, edict_t* other)
{
	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);

	if ((skill->value > 0) && (range(self, self->enemy) >= RANGE_MID))
	{
		if (random() > 0.5)
			self->monsterinfo.currentmove = &zombie_move_attack6;
	}
}

//
// DUCK
//

void zombie_duck_hold(edict_t* self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t zombie_frames_duck[] =
{
	ai_move, 5, zombie_duck_down,
	ai_move, -1, zombie_duck_hold,
	ai_move, 1,  NULL,
	ai_move, 0,  zombie_duck_up,
	ai_move, 5,  NULL
};
mmove_t zombie_move_duck = { FRAME_paind1, FRAME_paind5, zombie_frames_duck, zombie_run };

void zombie_dodge(edict_t* self, edict_t* attacker, float eta)
{
	float	r;

	r = random();
	if (r > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	if (skill->value == 0)
	{
		self->monsterinfo.currentmove = &zombie_move_duck;
		return;
	}

	self->monsterinfo.pausetime = level.time + eta + 0.3;
	r = random();

	if (skill->value == 1)
	{
		if (r > 0.33)
			self->monsterinfo.currentmove = &zombie_move_duck;
		else
			self->monsterinfo.currentmove = &zombie_move_attack3;
		return;
	}

	if (skill->value >= 2)
	{
		if (r > 0.66)
			self->monsterinfo.currentmove = &zombie_move_duck;
		else
			self->monsterinfo.currentmove = &zombie_move_attack3;
		return;
	}

	self->monsterinfo.currentmove = &zombie_move_attack3;
}


//
// DEATH
//

void zombie_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

mframe_t zombie_frames_death1[] =
{
	ai_move, 0,   NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   zombie_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   zombie_fire,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};

// TEMP
mmove_t zombie_move_death1 = { FRAME_paine1, FRAME_paine12, zombie_frames_death1, zombie_dead };

mframe_t zombie_frames_death2[] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
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

// same as zombie_move_death1 but he moves differently and doesn't attack you 
// maybe i will add zombie_move_death3 with a lot of attacks but maybe this is bad in multiplayer so i will remove it!!!!
mmove_t zombie_move_death2 = { FRAME_paine1, FRAME_paine12, zombie_frames_death2, zombie_dead }; 

void zombie_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int32_t damage, vec3_t point)
{
	int		n;

	// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 3; n++)
			ThrowGib(self, "models/objects/gibs/zombie/zombie_hand.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/zombie/zombie_main.md2", damage, GIB_ORGANIC);
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
		self->monsterinfo.currentmove = &zombie_move_death1;
	else if (n == 1)
		self->monsterinfo.currentmove = &zombie_move_death2;

	// TODO: temp until not fucked animations
	G_FreeEdict(self);
}

//
// SPAWN
//

void SP_monster_zombie_x(edict_t* self)
{
	self->classname = "monster_zombie"; // for Bamfuslicator
	self->s.modelindex = gi.modelindex("models/monsters/zombie/tris.md2");
	self->monsterinfo.scale = MODEL_SCALE;
	VectorSet(self->mins, -12, -12, -24);
	VectorSet(self->maxs, 12, 12, 24);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->monsterinfo.aiflags |= AI_WANDER;
	// move this?
	self->monsterinfo.wander_steps_min = 20;
	self->monsterinfo.wander_steps_max = 100;

	sound_idle = gi.soundindex("zombie/idle_w2.wav");
	sound_sight1 = gi.soundindex("zombie/z_idle.wav");
	sound_sight2 = gi.soundindex("zombie/z_idle1.wav");
	sound_hit = gi.soundindex("zombie/z_hit.wav");
	sound_pain = gi.soundindex("zombie/z_pain1.wav");
	sound_death = gi.soundindex("zombie/z_gib.wav");
	gi.soundindex("zombie/z_miss.wav");

	self->mass = 100;

	self->pain = zombie_pain;
	self->die = zombie_die;

	self->monsterinfo.stand = zombie_stand;
	self->monsterinfo.walk = zombie_walk;
	self->monsterinfo.run = zombie_run;
	self->monsterinfo.dodge = zombie_dodge;
	self->monsterinfo.attack = zombie_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = zombie_sight;

	gi.linkentity(self);

	self->monsterinfo.stand(self);

	walkmonster_start(self);
}

/*Ambush Trigger_Spawn Sight
*/
SP_monster_zombie(edict_t* self)
{
	SP_monster_zombie_x(self);

	self->s.skinnum = 2;
	self->health = ZOMBIE_HEALTH_STANDARD;
	self->max_health = ZOMBIE_HEALTH_STANDARD;
	self->gib_health = 0; //test
}
