#include "g_local.h"

// snap - new chasecam mode...
void UpdateChaseCam(edict_t *ent)
{
	static vec3_t	mins = {-10, -10, -10}, maxs = {10, 10, 10};
	vec3_t o, ownerv;
	edict_t *targ;
	vec3_t forward, up;
	trace_t trace;
	int i;
	vec3_t angles;

	if (!ent->client->prechase_ps.fov)
		ent->client->prechase_ps = ent->client->ps;

	targ = ent->client->chase_target;

	VectorCopy(targ->s.origin, ownerv);

	// normal locked chase mode...
	if (ent->client->chasemode == LOCKED_CHASE)
	{
		ownerv[2] += targ->viewheight;

		VectorCopy(targ->client->v_angle, angles);
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors (angles, forward, NULL, up);
		VectorNormalize(forward);
		VectorMA(ownerv, -150, forward, o);
		VectorMA(o, 24, up, o);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		// jump animation lifts
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.trace(ownerv, mins, maxs, o, targ, MASK_SOLID);
		VectorCopy(trace.endpos, ent->s.origin);

		for (i=0 ; i<3 ; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(targ->client->v_angle[i] - ent->client->resp.cmd_angles[i]);

		VectorCopy(targ->client->v_angle, ent->client->ps.viewangles);
		VectorCopy(targ->client->v_angle, ent->client->v_angle);
		// Ridah, angle down a bit since we moved it up
		ent->client->v_angle[2] += 35;
	}
	// free to spin around the target...
	else if (ent->client->chasemode == FREE_CHASE)
	{
		ownerv[2] += targ->viewheight;

		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		VectorNormalize(forward);
		VectorMA(ownerv, -150, forward, o);

		// jump animation lifts
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.trace(ownerv, mins, maxs, o, targ, MASK_SOLID);        
		VectorCopy(trace.endpos, ent->s.origin);

		for (i=0 ; i<3 ; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->v_angle[i] - ent->client->resp.cmd_angles[i]);
	}
	// eyecam chase mode...
	else
	{
		ent->client->ps = targ->client->ps;

		if (kpded2)
		{
			// kpded2 can remove the target's body from view, so no need to move ahead of them
			VectorCopy(ownerv, ent->s.origin);
		}
		else
		{
			VectorCopy(targ->client->v_angle, angles);
			AngleVectors (angles, forward, NULL, NULL);
			if (forward[2] < 0) forward[2] = 0;
			VectorNormalize(forward);
			VectorMA(ownerv, 45, forward, o);
			trace = gi.trace(ownerv, mins, maxs, o, targ, MASK_SOLID);
			VectorCopy(trace.endpos, ent->s.origin);
		}

		for (i=0 ; i<3 ; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(targ->client->v_angle[i] - ent->client->resp.cmd_angles[i]);

		VectorCopy(targ->client->v_angle, ent->client->ps.viewangles);
		VectorCopy(targ->client->v_angle, ent->client->v_angle);
	}

	ent->viewheight = 0;
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent)
{
	int i;
	edict_t *e;

	i = ent->client->chase_target - g_edicts;
	do
	{
		i++;
		if (i > maxclients->value)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->solid != SOLID_NOT)
			break;
	}
	while (e != ent->client->chase_target);

	if (e == ent || !e->inuse || e->solid == SOLID_NOT)
	{
		ChaseStop(ent);
		return;
	}

	ent->client->chase_target = e;
	ent->client->resp.scoreboard_frame = 0;
}

void ChasePrev(edict_t *ent)
{
	int i;
	edict_t *e;

	i = ent->client->chase_target - g_edicts;
	do
	{
		i--;
		if (i < 1)
			i = maxclients->value;
		e = g_edicts + i;
		if (!e->inuse) 
			continue;
		if (e->solid != SOLID_NOT)
			break;
	}
	while (e != ent->client->chase_target);

	if (e == ent || !e->inuse || e->solid == SOLID_NOT)
	{
		ChaseStop(ent);
		return;
	}

	ent->client->chase_target = e;
	ent->client->resp.scoreboard_frame = 0;
}

void ChaseStart(edict_t *ent)
{
	int i;
	edict_t *e;
	float dist, bestdist = 99999;

	for (i=1; i<=maxclients->value; i++)
	{
		e = g_edicts + i;
		if (!e->inuse || e->solid == SOLID_NOT)
			continue;
		dist = VectorDistance(ent->s.origin, e->s.origin);
		if (dist < bestdist)
		{
			bestdist = dist;
			ent->client->chase_target = e;
		}
	}

	if (ent->client->chase_target)
		ent->client->resp.scoreboard_frame = 0;
}

void ChaseStop(edict_t *ent)
{
	if (ent->client->prechase_ps.fov)
	{
		int i;
		for (i=0 ; i<3 ; i++)
			ent->client->prechase_ps.pmove.delta_angles[i] = ent->client->ps.pmove.delta_angles[i];
		VectorCopy(ent->client->ps.viewangles, ent->client->prechase_ps.viewangles);
		ent->client->ps = ent->client->prechase_ps;
		ent->client->prechase_ps.fov = 0;
	}
	ent->client->chase_target = NULL;
	ent->client->resp.scoreboard_frame = 0;
}
