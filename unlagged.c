/*
===========================================================================
Copyright (C) 2006 Neil Toronto.

This file is part of the Unlagged source code.

Unlagged source code is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Unlagged source code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Unlagged source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================

Adapted and modified for Kingpin by Monkey Harris.

*/

#include "g_local.h"

/*
============
G_ResetHistory

Clear out the given client's history (should be called when the teleport bit is flipped)
============
*/
void G_ResetHistory( edict_t *ent ) {
/*	int		i, time;

	// fill up the history with data (assume the current position)
	ent->client->historyHead = NUM_CLIENT_HISTORY - 1;
	for ( i = ent->client->historyHead, time = level.time; i >= 0; i--, time -= 50 ) {
		VectorCopy( ent->mins, ent->client->history[i].mins );
		VectorCopy( ent->maxs, ent->client->history[i].maxs );
		VectorCopy( ent->s.origin, ent->client->history[i].origin );
		ent->client->history[i].time = time;
	}
*/
	memset(ent->client->history, 0, sizeof(ent->client->history));
	ent->client->saved.time = 0;
}


/*
============
G_StoreHistory

Keep track of where the client's been
============
*/
void G_StoreHistory( edict_t *ent ) {
	int		head;

	ent->client->historyHead++;
	if ( ent->client->historyHead >= NUM_CLIENT_HISTORY ) {
		ent->client->historyHead = 0;
	}

	head = ent->client->historyHead;

	// store all the collision-detection info and the time
	VectorCopy( ent->mins, ent->client->history[head].mins );
	VectorCopy( ent->maxs, ent->client->history[head].maxs );
	VectorCopy( ent->s.origin, ent->client->history[head].origin );
	ent->client->history[head].time = level.framenum * 100;
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp( float frac, vec3_t start, vec3_t end, vec3_t result ) {
// From CG_InterpolateEntityPosition in cg_ents.c:
/*
	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );
*/
// Making these exactly the same should avoid floating-point error

	result[0] = start[0] + frac * ( end[0] - start[0] );
	result[1] = start[1] + frac * ( end[1] - start[1] );
	result[2] = start[2] + frac * ( end[2] - start[2] );
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified "time"
=================
*/
static void G_TimeShiftClient( edict_t *ent, int time ) {
	int		j, k;

	// find two entries in the history whose times sandwich "time"
	// assumes no two adjacent records have the same timestamp
	j = k = ent->client->historyHead;
	do {
		if ( ent->client->history[j].time <= time )
			break;

		k = j;
		j--;
		if ( j < 0 ) {
			j = NUM_CLIENT_HISTORY - 1;
		}
	}
	while ( j != ent->client->historyHead );

	{
		// make sure it doesn't get re-saved
		if ( ent->client->saved.time != level.framenum * 100 ) {
			// save the current origin and bounding box
			VectorCopy( ent->mins, ent->client->saved.mins );
			VectorCopy( ent->maxs, ent->client->saved.maxs );
			VectorCopy( ent->s.origin, ent->client->saved.origin );
			ent->client->saved.time = level.framenum * 100;
		}

		if (!ent->client->history[j].time) // didn't exist at time
		{
			gi.unlinkentity( ent ); // remove them from world so they don't get hit
			return;
		}
		if (j == k) {
			// ahead of the stored history, use head and current position
			int		endtime = level.framenum * 100 + curtime - level.frameStartTime;
			float	frac = (float)(time - ent->client->history[j].time) /
				(float)(endtime - ent->client->history[j].time);

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac,
				ent->client->history[j].origin, ent->s.origin,
				ent->s.origin );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac,
				ent->client->history[j].mins, ent->mins,
				ent->mins );

			TimeShiftLerp( frac,
				ent->client->history[j].maxs, ent->maxs,
				ent->maxs );
		}
		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		else if ( j != ent->client->historyHead ) {
			float	frac = (float)(time - ent->client->history[j].time) /
				(float)(ent->client->history[k].time - ent->client->history[j].time);

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac,
				ent->client->history[j].origin, ent->client->history[k].origin,
				ent->s.origin );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac,
				ent->client->history[j].mins, ent->client->history[k].mins,
				ent->mins );

			TimeShiftLerp( frac,
				ent->client->history[j].maxs, ent->client->history[k].maxs,
				ent->maxs );
		} else {
			// we wrapped, so grab the earliest
			VectorCopy( ent->client->history[k].origin, ent->s.origin );
			VectorCopy( ent->client->history[k].mins, ent->mins );
			VectorCopy( ent->client->history[k].maxs, ent->maxs );
		}
		gi.linkentity( ent );
	}
}


/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
static void G_TimeShiftAllClients( int time, edict_t *skip ) {
	int			i;
	edict_t	*ent;

	// for every client
	ent = &g_edicts[1];
	for ( i = 0; i < game.maxclients; i++, ent++ ) {
		if (ent->inuse && ent->solid != SOLID_NOT && ent != skip ) {
			G_TimeShiftClient( ent, time );
		}
	}
}


/*
================
G_DoTimeShiftFor

Decide what time to shift everyone back to, and do it
================
*/
void G_DoTimeShiftFor( edict_t *ent ) {	
	int time;

	// if it's enabled server-side and the client wants it
	if (antilag->value && ent->client && !ent->client->pers.noantilag) {
		time = (level.framenum - 1) * 100 + curtime - level.frameStartTime - ent->client->ping;
		G_TimeShiftAllClients( time, ent );
	}
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( edict_t *ent ) {
	// if it was saved
	if ( ent->client->saved.time == level.framenum * 100 ) {
		// move it back
		VectorCopy( ent->client->saved.mins, ent->mins );
		VectorCopy( ent->client->saved.maxs, ent->maxs );
		VectorCopy( ent->client->saved.origin, ent->s.origin );
		ent->client->saved.time = 0;

		gi.linkentity( ent );
	}
}


/*
=======================
G_UnTimeShiftAllClients

Move ALL the clients back to where they were before the time shift,
except for "skip"
=======================
*/
static void G_UnTimeShiftAllClients( edict_t *skip ) {
	int			i;
	edict_t	*ent;

	ent = &g_edicts[1];
	for ( i = 0; i < game.maxclients; i++, ent++ ) {
		if (ent->inuse && ent->solid != SOLID_NOT && ent != skip ) {
			G_UnTimeShiftClient( ent );
		}
	}
}


/*
==================
G_UndoTimeShiftFor

Put everyone except for this client back where they were
==================
*/
void G_UndoTimeShiftFor( edict_t *ent ) {

	if (antilag->value && ent->client && !ent->client->pers.noantilag) {
		G_UnTimeShiftAllClients( ent );
	}
}
