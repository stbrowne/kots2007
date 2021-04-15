/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "g_local.h"


//SWB
#include "kots_character.h"
#include "kots_items.h"


void G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
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
edict_t *G_Find (edict_t *from, int fieldofs, char *match)
{
	char	*s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
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
edict_t *findradius (edict_t *from, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
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

edict_t *G_PickTarget (char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
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



void Think_Delay (edict_t *ent)
{
	G_UseTargets (ent, ent->activator);
	G_FreeEdict (ent);
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
void G_UseTargets (edict_t *ent, edict_t *activator)
{
	edict_t		*t;

//
// check for a delay
//
	if (ent->delay)
	{
	// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.dprintf ("Think_Delay with no activator\n");
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
		gi.centerprintf (activator, "%s", ent->message);
		if (ent->noise_index)
			gi.sound (activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent->killtarget)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->killtarget)))
		{
			G_FreeEdict (t);
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
		while ((t = G_Find (t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") &&
				(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating")))
				continue;

			if (t == ent)
			{
				gi.dprintf ("WARNING: Entity used itself.\n");
			}
			else
			{
				if (t->use)
					t->use (t, ent, activator);
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
float	*tv (float x, float y, float z)
{
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

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
char	*vtos (vec3_t v)
{
	static	int		index;
	static	char	str[8][32];
	char	*s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1)&7;

	Com_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}


vec3_t VEC_UP		= {0, -1, 0};
vec3_t MOVEDIR_UP	= {0, 0, 1};
vec3_t VEC_DOWN		= {0, -2, 0};
vec3_t MOVEDIR_DOWN	= {0, 0, -1};

void G_SetMovedir (vec3_t angles, vec3_t movedir)
{
	if (VectorCompare (angles, VEC_UP))
	{
		VectorCopy (MOVEDIR_UP, movedir);
	}
	else if (VectorCompare (angles, VEC_DOWN))
	{
		VectorCopy (MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors (angles, movedir, NULL, NULL);
	}

	VectorClear (angles);
}


float vectoyaw (vec3_t vec)
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
		yaw = (int) (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


void vectoangles (vec3_t value1, vec3_t angles)
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
			yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

char *G_CopyString (char *in)
{
	char	*out;
	
	out = gi.TagMalloc (strlen(in)+1, TAG_LEVEL);
	strcpy (out, in);
	return out;
}


void G_InitEdict (edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;

	//SWB
	e->character = NULL;
	e->pack = NULL;

	if (e->client)
		Kots_CharacterInit(e);
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
edict_t *G_Spawn (void)
{
	int			i;
	edict_t		*e;

	e = &g_edicts[(int)maxclients->value+1];
	for ( i=maxclients->value+1 ; i<globals.num_edicts ; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && ( e->freetime < 2 || level.time - e->freetime > 0.5 ) )
		{
			G_InitEdict (e);
			return e;
		}
	}
	
	if (i == game.maxentities)
		gi.error ("ED_Alloc: no free edicts");
		
	globals.num_edicts++;
	G_InitEdict (e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict (edict_t *ed)
{
	//SWB - handle monsters a little differently
	//we wait a bit before freeing them to ensure all projectiles and kills
	//they get are registered properly
	if ((ed->svflags & SVF_MONSTER) && ed->character)
	{
		//check to see if the time hasn't ellapsed because they'll
		//be have the noclient serverflag if it has
		if (!(ed->svflags & SVF_NOCLIENT))
		{
			ed->svflags |= SVF_NOCLIENT;
			ed->movetype = MOVETYPE_NOCLIP;
			ed->solid = SOLID_NOT;
			ed->deadflag = DEAD_DEAD;
			ed->nextthink = level.time + 10.0;
			ed->think = G_FreeEdict;
			gi.linkentity(ed); // relink entity
			return;
		}

		//the monster has had plenty of time so let's ditch all his stuff
		Kots_CharacterClearEdicts(ed);
	}

	gi.unlinkentity (ed);		// unlink from world

	if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
	{
//		gi.dprintf("tried to free special edict\n");
		return;
	}

	//SWB
	//Free the character when no longer in use
	Kots_CharacterFree(ed);

	//check for special freeing instructions before actually freeing it
	if (ed->item)
		Kots_FreeItem(ed);

	//free the pack
	if (ed->pack)
	{
		gi.TagFree(ed->pack);
		ed->pack = NULL;
	}

	//SWB - this should not be done because if it's freed in a loop it could mess things up
	//		just let map changes etc handle this
	//free the rune
	//if (ed->rune)
	//	Kots_RuneRemoveDropped(ed, ed->rune);

	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}


/*
============
G_TouchTriggers

============
*/
void	G_TouchTriggers (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;

	// dead things don't activate triggers!
	if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
		return;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;

		hit->touch (hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void	G_TouchSolids (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (ent->touch)
			ent->touch (hit, ent, NULL, NULL);
		if (!ent->inuse)
			break;
	}
}




/*
==============================================================================

Kill box

==============================================================================
*/

qboolean Kots_SpawnKick(edict_t *targ, edict_t *attacker)
{
	vec3_t forward;
	vec3_t dest;
	float distance;
	trace_t tr;
	int i;
	int dir = 30 * (rand() % 12); //determine a random 30 degree angle between 0 and 360
								  //this will be our first angle to attempt if the attackers angle doesn't work

	//use the angle the target is facing as the initial angle to attempt the kick at
	if (targ->client)
		AngleVectors (targ->client->v_angle, forward, NULL, NULL);
	else
		AngleVectors (targ->s.angles, forward, NULL, NULL);

	//determine the maximum distance from the attacker that the target must be to prevent overlapping
	//NOTE: Size won't be set yet for the attacker so we must determine it by the mins and maxs
	distance = hypot(targ->size[0], targ->size[1]) + hypot(abs(attacker->maxs[0] - attacker->mins[0]), abs(attacker->maxs[1] - attacker->mins[1]));

	//try kicking at the initial angle and all multiples of 30
	for (i = 0; i <= 12; i++)
	{
		VectorMA(attacker->s.origin, distance, forward, dest);

		//determine if there will be anything in the way (ignoring target)
		tr = gi.trace (targ->s.origin, targ->mins, targ->maxs, dest, targ, MASK_PLAYERSOLID);

		if (tr.fraction >= 1.0)
		{
			//move the target to the destination
			VectorCopy(dest, targ->s.origin);
			
			//try to raise the destination a bit to keep him from dragging on the ground
			dest[2] += 10;
			tr = gi.trace(targ->s.origin, targ->mins, targ->maxs, dest, targ, MASK_PLAYERSOLID);

			//if we can raise the destination great, otherwise ohh well
			//we don't want to get stuck in the ceiling so we'll just drag on the ground
			if (tr.fraction >= 1.0)
				targ->s.origin[2] += 10;

			gi.linkentity(targ); //relink entity at new destination

			//do spawn kick damage
			T_Damage(targ, attacker, attacker, forward, targ->s.origin, vec3_origin, 50, 500, DAMAGE_NO_PROTECTION | DAMAGE_NO_RESIST, MOD_TELEFRAG);
			gi.sound(targ, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);

			return true; //kick successful
		}

		//determine the next angle to attempt to kick at
		VectorSet(forward, 0, dir, 0);
		dir += 30; //increase dir for next iteration
		AngleVectors(forward, forward, NULL, NULL); //create angles
	}

	return false; //couldn't kick
}

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
qboolean KillBox (edict_t *ent)
{
	trace_t		tr;
	int count = 0;
	edict_t		*other = ent;
	float		radius = (abs(ent->mins[1]) + abs(ent->maxs[1])) * 2;
	vec3_t		dest;
	int			takedamage = ent->takedamage; //SWB - store this for later

	//SWB - Ensure that we won't take any kind of damage while we kill items
	ent->takedamage = DAMAGE_NO;

	//SWB - do some special handling for items on spawns
	while ((other = findradius(other, ent->s.origin, radius)) != NULL)
	{
		//SWB - check to prevent infinite loops
		if (++count > MAX_EDICTS)
			break;

		if (other->item)
		{
			//telefrag items with the telefrag tag on them
			Kots_TelefragItem(other);

			//not in use now so skip to the next ent
			if (!other->inuse)
				continue;
		}
	}

	//SWB - Restore our ability to take damage
	ent->takedamage = takedamage;

	//reset count back to 0 
	count = 0;

	//SWB - make sure we're not starting off in the floor
	//Some maps do this and prevent spawn kick from working
	VectorCopy(ent->s.origin, dest);
	while (1)
	{
		//give up after 8 attempts
		if (++count > 8)
			break;

		tr = gi.trace(dest, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_SOLID);

		if (tr.allsolid)
			dest[2] += 4;
		else
		{
			//put us back on the ground
			ent->s.origin[2] = tr.endpos[2];
			break;
		}
	}

	//reset count back to 0
	count = 0;

	while (1)
	{
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;

		//SWB - check to prevent infinite loops
		if (++count > MAX_EDICTS)
			break;


		//SWB
		//if this is a character then insure it has no invuln to prevent it from dying
		if (tr.ent->character)
		{
			//if we successfully kicked then continue to the next iteration
			if (Kots_SpawnKick(tr.ent, ent))
				break;
		}

		// nail it
		T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent->solid)
			return false;
	}

	return true;		// all clear
}
