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
// g_utils.c -- misc utility functions for game module

#include <game_local.h>


void Game_ProjectSource(vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t* Game_FindEdictByValue(edict_t* from, int32_t fieldofs, char* match)
{
	char* s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char**)((uint8_t*)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp(s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t* Game_FindEdictsWithinRadius(edict_t* from, vec3_t org, float rad)
{
	vec3_t	eorg;
	int32_t	j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}

/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES	8

edict_t* Edict_PickTarget(char* targetname)
{
	edict_t* ent = NULL;
	int32_t	num_choices = 0;
	edict_t* choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1)
	{
		ent = Game_FindEdictByValue(ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.dprintf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

void Think_Delay(edict_t* ent)
{
	Edict_UseTargets(ent, ent->activator);
	Edict_Free(ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void Edict_UseTargets(edict_t* ent, edict_t* activator)
{
	edict_t* t;

	//
	// check for a delay
	//
	if (ent->delay)
	{
		// create a temp object to fire at a later time
		t = Edict_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.dprintf("Think_Delay with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}


	//
	// print the message
	//
	if ((ent->message) && !(activator->svflags & SVF_MONSTER))
	{
		gi.centerprintf(activator, "%s", ent->message);
		if (ent->noise_index)
			gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	//
	// kill killtargets
	//
	if (ent->killtarget)
	{
		t = NULL;
		while ((t = Game_FindEdictByValue(t, FOFS(targetname), ent->killtarget)))
		{
			Edict_Free(t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

	//
	// fire targets
	//
	if (ent->target)
	{
		t = NULL;
		while ((t = Game_FindEdictByValue(t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") &&
				(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating")))
				continue;

			if (t == ent)
			{
				gi.dprintf("WARNING: Entity used itself.\n");
			}
			else
			{
				if (t->use)
					t->use(t, ent, activator);
			}
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float* tv(float x, float y, float z)
{
	static int32_t	index;
	static vec3_t	vecs[8];
	float* v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char* vtos(vec3_t v)
{
	static	int		index;
	static	char	str[8][32];
	char* s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1) & 7;

	Com_sprintf(s, 32, "(%i %i %i)", (int32_t)v[0], (int32_t)v[1], (int32_t)v[2]);

	return s;
}


vec3_t VEC_UP = { 0, -1, 0 };
vec3_t MOVEDIR_UP = { 0, 0, 1 };
vec3_t VEC_DOWN = { 0, -2, 0 };
vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

void Edict_SetMovedir(vec3_t angles, vec3_t movedir)
{
	if (VectorCompare(angles, VEC_UP))
	{
		VectorCopy(MOVEDIR_UP, movedir);
	}
	else if (VectorCompare(angles, VEC_DOWN))
	{
		VectorCopy(MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors(angles, movedir, NULL, NULL);
	}

	VectorClear(angles);
}


float vectoyaw(vec3_t vec)
{
	float	yaw;

	if (/*vec[YAW] == 0 &&*/ vec[PITCH] == 0)
	{
		yaw = 0;
		if (vec[YAW] > 0)
			yaw = 90;
		else if (vec[YAW] < 0)
			yaw = -90;
	}
	else
	{
		yaw = (int32_t)(atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


void vectoangles(vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (int32_t)(atan2f(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;
		if (yaw < 0)
			yaw += 360;

		forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (int32_t)(atan2f(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

char* Game_CopyString(char* in)
{
	char* out;

	out = gi.TagMalloc((int32_t)strlen(in) + 1, TAG_LEVEL);
	strcpy(out, in);
	return out;
}


void Edict_Init(edict_t* e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t* Edict_Spawn()
{
	int32_t	i;
	edict_t* e;

	e = &g_edicts[(int32_t)maxclients->value + 1];
	for (i = maxclients->value + 1; i < globals.num_edicts; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && (e->freetime < 2 || level.time - e->freetime > 0.5))
		{
			Edict_Init(e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.error("ED_Alloc: no free edicts");

	globals.num_edicts++;
	Edict_Init(e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void Edict_Free(edict_t* ed)
{
	gi.Edict_Unlink(ed);		// unlink from world

	if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
	{
		//		gi.dprintf("tried to free special edict\n");
		return;
	}

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}


/*
============
G_TouchTriggers

============
*/
void Edict_TouchTriggers(edict_t* ent)
{
	int32_t  i, num;
	edict_t* touch[MAX_EDICTS], * hit;

	// dead things don't activate triggers!
	if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
		return;

	num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		hit->touch(hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void Edict_TouchSolids(edict_t* ent)
{
	int32_t		i, num;
	edict_t* touch[MAX_EDICTS], * hit;

	num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (ent->touch)
			ent->touch(hit, ent, NULL, NULL);
		if (!ent->inuse)
			break;
	}
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
bool Game_KillBox(edict_t* ent)
{
	trace_t		tr;

	while (1)
	{
		tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;

		// nail it
		Player_Damage(tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent->solid)
			return false;
	}

	return true;		// all clear
}

//
// G_CountClients: Counts the number of clients currently playing the game, regardless of their status (alive, dead, spectating, etc)
//
int32_t Game_CountClients()
{
	edict_t* client_edict;
	int32_t	 real_client_count = 0;

	// count clients
	for (int32_t client_num = 0; client_num < game.maxclients; client_num++)
	{
		// is the client actually being used
		client_edict = g_edicts + client_num + 1; // client edicts are always at the start???

		if (client_edict->inuse) real_client_count++;
	}

	return real_client_count;
}

/*
==============================================================================

ZombonoUI-related utilities

==============================================================================
*/

// Sends a UI. ent NULL = multicast
void GameUI_Send(edict_t* ent, char* ui_name, bool enabled, bool activated, bool reliable)
{
	gi.WriteByte(svc_uidraw);
	gi.WriteString(ui_name);
	gi.WriteByte(enabled);
	gi.WriteByte(activated);

	if (ent == NULL)
	{
		if (reliable)
		{
			gi.multicast(vec3_origin, MULTICAST_ALL_R);
		}
		else
		{
			gi.multicast(vec3_origin, MULTICAST_ALL);
		}
	}
	else
	{
		gi.unicast(ent, reliable); // reliable as not used regularly
	}
}

void GameUI_SetText(edict_t* ent, char* ui_name, char* control_name, char* text, bool reliable)
{
	gi.WriteByte(svc_uisettext);
	gi.WriteString(ui_name);
	gi.WriteString(control_name);
	gi.WriteString(text);

	if (ent == NULL)
	{
		if (reliable)
		{
			gi.multicast(vec3_origin, MULTICAST_ALL_R);
		}
		else
		{
			gi.multicast(vec3_origin, MULTICAST_ALL);
		}
	}
	else
	{
		gi.unicast(ent, reliable); // reliable as not used regularly
	}
}

void GameUI_SetImage(edict_t* ent, char* ui_name, char* control_name, char* image_path, bool reliable)
{
	gi.WriteByte(svc_uisetimage);
	gi.WriteString(ui_name);
	gi.WriteString(control_name);
	gi.WriteString(image_path);

	if (ent == NULL)
	{
		if (reliable)
		{
			gi.multicast(vec3_origin, MULTICAST_ALL_R);
		}
		else
		{
			gi.multicast(vec3_origin, MULTICAST_ALL);
		}
	}
	else
	{
		gi.unicast(ent, reliable); // reliable as not used regularly
	}
}
