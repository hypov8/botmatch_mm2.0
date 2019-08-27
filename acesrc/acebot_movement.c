///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//	
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the 
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for 
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//  
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////
	
///////////////////////////////////////////////////////////////////////
//  acebot_movement.c - This file contains all of the 
//                      movement routines for the ACE bot
//           
///////////////////////////////////////////////////////////////////////

#include "../g_local.h" //DIR_SLASH
#include "acebot.h"
#include "../g_hitmen.h"

vec3_t ACE_look_out; //hypov8 global var

//static qboolean ACEMV_CheckLavaAndSky(edict_t *self);
static qboolean ACEMV_CheckLadder(edict_t *self, usercmd_t *ucmd, qboolean isTopOfLadder, qboolean isKnownLadder);
#define CHECKSKYDOWNDIST 3072

///////////////////////////////////////////////////////////////////////
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime. 
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_CanMove(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset,start,end;
	vec3_t angles;
	trace_t tr;

	// Now check to see if move will move us off an edgemap team_fast_cash
	VectorCopy(self->s.angles,angles);
	
	if(direction == MOVE_LEFT)
		angles[1] += 90;
	else if(direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == MOVE_BACK)
		angles[1] -=180;

	// Set up the vectors
	AngleVectors (angles, forward, right, NULL);
	
	VectorSet(offset, 36, 0, 24);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
		
	VectorSet(offset, 36, 0, -400);
	G_ProjectSource (self->s.origin, offset, forward, right, end);
	
	tr = gi.trace(start, NULL, NULL, end, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
	
	if(tr.fraction > 0.3 && tr.fraction != 1 || tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//if(debug_mode)
		//	debug_printf("%s: move blocked\n",self->client->pers.netname); //hypov8 disabled debug
		return false;	
	}
	
	return true; // yup, can move
}

//hypov8 added to check flat ground/edge
static qboolean ACEMV_CanMove_Simple(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset, end, down;
	vec3_t angles;
	trace_t tr;
	vec3_t minx = { -4, -4, 0 }; //was 24
	vec3_t maxx = { 4, 4, 0 }; //was 24
	vec3_t minWall = { -16, -16, 0 }; // fix for steps
	vec3_t maxWall = { 16, 16, 48 }; 

	// Now check to see if move will move us off an edgemap team_fast_cash, dm_fis_b1
	VectorCopy(self->s.angles, angles); //MOVE_FORWARD

	if (direction == MOVE_LEFT)
		angles[1] += 90;
	else if (direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if (direction == MOVE_BACK)
		angles[1] -= 180;

	// Set up the vectors
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 24, 0, 0); //hypo low value. incase steps
	G_ProjectSource(self->s.origin, offset, forward, right, end);

	tr = gi.trace(self->s.origin, minWall, maxWall, end, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);

	if (tr.fraction != 1 ) //wall hit
	{
		return false;
	}
	else //check for falling off edge
	{
		VectorSet(offset, 48, 0, 0); //hypo increase?
		G_ProjectSource(self->s.origin, offset, forward, right, end);

		VectorCopy(end, down);
		down[2] -= CHECKSKYDOWNDIST;
		tr = gi.trace(end, minx, maxx, down, self, MASK_BOT_DROP_SKY);
			///VectorCopy(start, down);

		if (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
			return false;
		if ((tr.surface->flags & SURF_SKY) && !tr.startsolid)
			return false;
	
	}

	return true; // yup, can move
}

static void ACEMV_ChangeBotAngle(edict_t *ent);

///////////////////////////////////////////////////////////////////////
// Handle special cases of crouch/jump
//
// If the move is resolved here, this function returns
// true.
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_SpecialMove(edict_t *self, usercmd_t *ucmd)
{
	int isStep;
	vec3_t dir,forward,right,start,end,offset, step;
	vec3_t bboxTop, tmpStart, tmpEnd;
	trace_t tr, tr_monster, tr_step; 


	//vec3_t ladderMin = { -16, -16, -32 };
	//vec3_t ladderMax = { 16, 16, 48 }; //thin ladder cods?
	
	if (self->acebot.dodge_time > level.framenum)
	{
		VectorSubtract(nodes[self->acebot.node_next].origin, self->s.origin, self->acebot.move_vector);
		return true;
	}

	//add hypov8 check node ontop of crates
	if (self->acebot.node_next > INVALID && self->acebot.state == BOTSTATE_MOVE)
	{
		vec_t jump_height = (nodes[self->acebot.node_next].origin[2] - 8) - self->s.origin[2];

		if (jump_height > 16 && jump_height <= 60)
		{
			float distToTarget;
			vec3_t player_move_up; // , dist_flat;
			//look at next node, flat
			VectorCopy(self->s.origin, player_move_up);
			player_move_up[2] = nodes[self->acebot.node_next].origin[2];

			distToTarget = VectorDistance(player_move_up, nodes[self->acebot.node_next].origin);
			if (distToTarget < 64)
			{
				trace_t trace;

				trace = gi.trace(player_move_up, self->mins, self->maxs, nodes[self->acebot.node_next].origin, self, MASK_BOT_SOLID_FENCE); //hypo can we jump up? minus 12, jump to 60 units hypo: todo

				if (trace.allsolid == 0 && trace.startsolid == 0 && trace.fraction == 1.0)
				{
					ucmd->forwardmove = BOT_FORWARD_VEL;
					ucmd->upmove = BOT_JUMP_VEL;
					self->acebot.isJumpToCrate= true;
					self->acebot.crate_time = level.framenum + 5;
					return true;
				}
			}
		}
	}
	// Get current direction
	VectorSet(dir, 0, self->s.angles[YAW], 0);
	AngleVectors (dir, forward, right, NULL);
	//VectorSet(offset, 18, 0, 0);
	//G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(self->s.origin, start);
	VectorSet(offset, 36, 0, 0);
	G_ProjectSource(self->s.origin, offset, forward, right, end);

	VectorCopy(start, tmpStart); //store orig posi. use to check jump
	VectorCopy(end, tmpEnd);
	start[2] += 18; // so they are not jumping all the time
	end[2] += 18;

#if 0
	// check if its a step infrontstep
	VectorSet(offset, 22, 0, 0);
	G_ProjectSource(start, offset, forward, right, step);
	tr_step = gi.trace(start, self->mins, self->maxs, step, self, MASK_BOT_SOLID_FENCE);
	if (!tr_step.allsolid)
	{
		isStep = 1;
	}
#endif

	/////////////////////////////////
	//VectorCopy(self->s.origin, start); //hypov8 scan from player centre? not outside player BBox



	// trace it
	//start[2] += 18; // so they are not jumping all the time
	//end[2] += 18;
	tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE |CONTENTS_LADDER);
	tr_monster = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE_MON);

#if 1
	//hypo check if a player is stoping bot
	if ((tr_monster.contents & CONTENTS_MONSTER 
		|| strcmp(tr_monster.ent->classname, "player") == 0 
		|| strcmp(tr_monster.ent->classname, "bot") == 0)
		&& tr.fraction == 1
		&& tr_monster.fraction != 1
		/*&& self->acebot.dodge_time < level.framenum*/ )
		{
			int i;
			vec3_t  angles2;
			qboolean canGoLeft = false;
			qboolean canGoRight = false;

			if (ACEMV_CanMove_Simple(self, MOVE_LEFT))	canGoLeft = true;
			if (ACEMV_CanMove_Simple(self, MOVE_RIGHT))	canGoRight = true;
			
			for (i = 0; i<2; i++)
			{
				VectorCopy(self->s.angles, angles2);
				if (i == 0)
					if (canGoLeft == true)
						angles2[1] += 90;//MOVE_LEFT
					else
						continue; // angles2[1] -= 90; //MOVE_RIGHT
				else
					if(canGoRight == true)
						angles2[1] -= 90; //MOVE_RIGHT
					else
						continue; // angles2[1] += 90;//MOVE_LEFT

				if (i == 0)
				{
					self->acebot.dodge_time = level.framenum+5;
					self->s.angles[1] += 45;
					ucmd->forwardmove = BOT_FORWARD_VEL;
					ucmd->sidemove = -BOT_SIDE_VEL;
					ACEMV_ChangeBotAngle(self);
					return true;
				}
				else
				{
					self->acebot.dodge_time = level.framenum +5;
					self->s.angles[1] -= 45;
					ucmd->forwardmove = BOT_FORWARD_VEL;
					ucmd->sidemove = BOT_SIDE_VEL;
					ACEMV_ChangeBotAngle(self);
					return true;
				}
			}

		}
#endif

		//////////////////////////
		// Check for ladder    //
		////////////////////////
	if (tr.fraction !=1 && tr.contents& CONTENTS_LADDER && ACEMV_CheckLadder(self, ucmd, false, true))
		return true;


	//hypov8 if infront is not totaly solid. try to move to there, check jump then crouch
	if(/*tr.allsolid*/ tr.startsolid || tr.fraction != 1.0) // hypov8 add tr.startsolid
	{	
		int i;

		///////////////////////////
		// Check for jump       //
		/////////////////////////
		int jumpV[4] = {60, 40, 32, 24};
		int jumpD[4] = {60, 42, 34, 26};
		int crateT[4] = {5, 4, 3, 2};
		for (i = 0; i < 4; i++)
		{

			start[2] = end[2] = tmpStart[2] + jumpV[i];
			tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);
			if (/*!tr.allsolid*/ tr.fraction == 1.0)
			{
				float dist;
				vec3_t	forward;
				ucmd->forwardmove = BOT_FORWARD_VEL;
				ucmd->upmove = BOT_JUMP_VEL;

				dist = jumpD[i];
				AngleVectors(self->s.angles, forward, NULL, NULL);
				VectorScale(forward, dist, self->velocity);
				self->velocity[2] = dist;

				if (self->groundentity)
					self->groundentity = NULL;

				self->acebot.isJumpToCrate = true;
				self->acebot.crate_time = level.framenum + crateT[i];
				return true;
			}
		}

		///////////////////////// 
		// Check for crouching // hypov8 crouch. kp 72-48 height.
		/////////////////////////
		VectorCopy(self->maxs, bboxTop);
		// VectorCopy(start, tmpStart);
		//VectorCopy(end, tmpEnd);
		bboxTop[2] = 24; // bbox crouching height (DUCKING_MAX_Z = 24)
		start[2] = end[2] = self->s.origin[2]; //set as player height

		for (i = 0; i < 3; i++)
		{
			if (i > 0)	{	start[2] += 8;	end[2] += 8;} //check for small steps into crouch
			tr = gi.trace(start, self->mins, bboxTop, end, self, MASK_BOT_SOLID_FENCE);
			// Crouch
			if (/*!tr.allsolid*/ tr.fraction == 1.0)
			{
				ucmd->forwardmove = BOT_FORWARD_VEL;
				ucmd->upmove = -400; 
				return true;
			}
		}

#if 0
		//////////////////////////
		// Check for step       //
		//////////////////////////
		VectorCopy(self->s.origin, start);
		VectorSet(offset, 8, 0, 0);
		G_ProjectSource(self->s.origin, offset, forward, right, end);

		start[2] +=18;//hypov8 max height we can jump is 60
		end[2] +=18;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		// Check for jump up a step 60 units
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{	
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 60;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.isJumpToCrate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
#endif
	}
	else
	{
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return true;
	}

	return false; // We did not resolve a move here
}

///////////////////////////////////////////////////////////////////////
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
// 
// Modified from the original id ChangeYaw code...
///////////////////////////////////////////////////////////////////////
static void ACEMV_ChangeBotAngle(edict_t *ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vec3_t  ideal_angle;

	// Normalize the move angle first
	VectorNormalize(ent->acebot.move_vector);

	current_yaw = anglemod(ent->s.angles[YAW]);
	current_pitch = anglemod(ent->s.angles[PITCH]);

	vectoangles(ent->acebot.move_vector, ideal_angle);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);

	// Yaw
	if (current_yaw != ideal_yaw)
	{
		move = ideal_yaw - current_yaw;
		speed = ent->yaw_speed;
		if (ideal_yaw > current_yaw)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[YAW] = anglemod(current_yaw + move);
	}

	// Pitch
	if (current_pitch != ideal_pitch)
	{
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed;
		if (ideal_pitch > current_pitch)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[PITCH] = anglemod(current_pitch + move);
	}
}


static int ACEMV_CheckLavaAndSky(edict_t *self) //add normal falling edges
{
	vec3_t dir, forward, right, offset, start, wall, down;
	trace_t trace; // for eyesight
	vec3_t minx = { 0, 0, 0 };
	vec3_t maxx = { 0, 0, 0 };
	vec3_t minx2 = { -12, -12, 0 }; //hypo 12, player not allways looking straight+steps
	vec3_t maxx2 = { 12, 12, 48 };

	//make sure we are not jumping. or allready falling :)
	if (!self->groundentity && self->velocity[2]< -120)
		return false; //return didnot resolve

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles, dir);
	dir[2] = 0.0f;
	AngleVectors(dir, forward, right, NULL);
	VectorSet(offset, 24, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, wall);


	trace = gi.trace(self->s.origin, minx2, maxx2, wall, self, MASK_BOT_SOLID_FENCE);
	if (trace.fraction != 1)
		return false; // must b hitting wall, return didnot resolve


	VectorSet(offset, 58, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(start, down);
	down[2] -= CHECKSKYDOWNDIST;

	trace = gi.trace(start, minx, maxx, down, self, MASK_BOT_DROP_SKY); //hypov8 todo water? & test alpha
	if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}
	else if ((trace.surface->flags & SURF_SKY) && !trace.startsolid)
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){ //add hypo fail if all sky
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}

	return false; //cant move
}



///////////////////////////////////////////////////////////////////////
// Checks for obstructions in front of bot
//
// This is a function I created origianlly for ACE that
// tries to help steer the bot around obstructions.
//
// If the move is resolved here, this function returns true.
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_CheckEyes(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  forward, right;
	vec3_t  leftstart, rightstart, focalpoint;
	vec3_t  upstart, upend;
	vec3_t  dir, offset;
	vec3_t	minx = { -16, -16, -24 };	//set new bb size, allow some errors from not walking totaly straight
	vec3_t	maxx = { 16, 16, 48 };
	qboolean isPlayerInfront = 0;
	trace_t traceFront, tr_monster, traceRight, traceLeft;

	//set crouch if needed
	maxx[2] = self->maxs[2];

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles, dir);
	AngleVectors(dir, forward, right, NULL);

	// Let them move to targets by walls
	if (!self->movetarget && self->acebot.uTurnTime < level.framenum)
		VectorSet(offset, 200, 0, 4); // focalpoint 
	else
		VectorSet(offset, 36, 0, 4); // focalpoint 

	// Check from self to focalpoint
	G_ProjectSource(self->s.origin, offset, forward, right, focalpoint);
	traceFront = gi.trace(self->s.origin, minx, maxx, focalpoint, self, MASK_BOT_SOLID_FENCE|CONTENTS_LADDER );


#if 1 //hypo check if a player is stoping bot
	tr_monster = gi.trace(self->s.origin, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE_MON);

	if ((tr_monster.contents & CONTENTS_MONSTER
		|| strcmp(tr_monster.ent->classname, "player") == 0
		|| strcmp(tr_monster.ent->classname, "bot") == 0)
		&& traceFront.fraction == 1
		&& tr_monster.fraction != 1
		/*&& self->acebot.dodge_time < level.framenum*/)
	{
		if (tr_monster.ent->client && tr_monster.ent->client->ps.pmove.pm_type == PM_NORMAL) //hypo make sure there not in noclip/dead etc
		{
			int i;
			vec3_t forward2, right2;
			vec3_t offset2, end2;
			vec3_t angles2;
			trace_t tr_sides;

			for (i = 0; i < 2; i++)
			{
				VectorCopy(self->s.angles, angles2);
				if (i == 0 && ACEMV_CanMove_Simple(self, MOVE_LEFT))
					angles2[1] += 90;//MOVE_LEFT
				else
					angles2[1] -= 90; //MOVE_RIGHT

				AngleVectors(angles2, forward2, right2, NULL);
				VectorSet(offset2, 36, 0, 0);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);

				tr_sides = gi.trace(self->s.origin, NULL, NULL, end2, self, MASK_BOT_SOLID_FENCE_MON);

				if (tr_sides.fraction == 1)
				{
					if (i == 0)
					{
						if (self->acebot.dodge_time < level.framenum)
						{
							self->acebot.dodge_time = level.framenum + 2;
							self->s.angles[1] += 45;
							ucmd->sidemove = -BOT_SIDE_VEL;
						}
						ucmd->forwardmove = BOT_FORWARD_VEL;
						return true;
					}
					else
					{
						if (self->acebot.dodge_time < level.framenum)
						{
							self->acebot.dodge_time = level.framenum + 2;
							self->s.angles[1] -= 45;
							ucmd->sidemove = BOT_SIDE_VEL;
						}
						ucmd->forwardmove = BOT_FORWARD_VEL;
						return true;
					}

				}
			}
		}
	}
#endif

	// If this check fails we need to continue on with more detailed checks
	if (traceFront.fraction == 1)
	{
		 if (ACEMV_CheckLavaAndSky(self) == 2) //add hypov8
			 return true;// standing on sky. dont move, die!!!

		ucmd->forwardmove = BOT_FORWARD_VEL;
		return true;
	}
#if 0
	//hypo bot wandering, no path, trace small step, return can move
	{
		vec3_t  stepstart, stepend;
		VectorSet(offset, 0, 0, 18);
		G_ProjectSource(self->s.origin, offset, forward, right, stepstart);
		VectorSet(offset, 8, 0, 18);
		G_ProjectSource(self->s.origin, offset, forward, right, stepend);

		traceFront = gi.trace(stepstart, self->mins, self->maxs, stepend, self, MASK_BOT_SOLID_FENCE);
		if (traceFront.fraction == 1 && !traceFront.allsolid && !traceFront.startsolid)
		{
			vec3_t minTmp, maxTmp;
			VectorSet(minTmp, -12, -12, -24);
			VectorSet(maxTmp, 12, 12, 48);
			//make sure we are clear for further and up
			VectorSet(offset, 36, 0, 52);
			G_ProjectSource(self->s.origin, offset, forward, right, stepend);
			traceFront = gi.trace(stepstart, minTmp, maxTmp, stepend, self, MASK_BOT_SOLID_FENCE);
			if (traceFront.fraction == 1 && !traceFront.allsolid && !traceFront.startsolid)
			{
				//hypov8 ToDo: moving slow up some stairs
				ucmd->forwardmove = BOT_FORWARD_VEL;
				return true;
			}
		}
	}
#endif

	//////////////
	// water
	// hypo step out onto ledge
	//////////////
	if (ucmd->upmove > 200 && gi.pointcontents(self->s.origin) & MASK_WATER 
		&& ((offset[0] == 200 && traceFront.fraction < 0.2) || (offset[0] == 36 && traceFront.fraction < 0.5)))
	{
		//dont allways jump out of water
		if (random() >= 0.8)
		{
			vec3_t waterStepStart, waterStepEnd;
			trace_t traceLedge;
			VectorCopy(self->s.origin, waterStepStart);
			waterStepStart[2] += 56; //48??
			VectorSet(offset, 48, 0, 0); // focalpoint 

			// Check from self to focalpoint
			G_ProjectSource(waterStepStart, offset, forward, right, waterStepEnd);
			traceLedge = gi.trace(waterStepStart, minx, maxx, waterStepEnd, self, MASK_BOT_SOLID_FENCE);

			if (traceLedge.fraction == 1)
			{
				self->acebot.water_time = level.framenum + 20;
				return true;
			 }
		}

	}

	VectorSet(offset, 30, 30, 0);
	G_ProjectSource(self->s.origin, offset, forward, right, rightstart);
	VectorSet(offset, 30, -30, 0);
	G_ProjectSource(self->s.origin, offset, forward, right,leftstart);

	traceRight = gi.trace(self->s.origin, minx, maxx, rightstart, self, MASK_BOT_SOLID_FENCE);
	traceLeft = gi.trace(self->s.origin, minx, maxx, leftstart, self, MASK_BOT_SOLID_FENCE);

	// Wall checking code, this will degenerate progressivly so the least cost 
	// check will be done first.

	// If open space move ok
	//hypov8 if slowed down, check and turn as needed
	//keep moveing forward if a door
	if ((traceRight.fraction != 1 || traceLeft.fraction != 1 || isPlayerInfront)
		&&( strcmp(traceLeft.ent->classname, "func_door") != 0
		&& strcmp(traceLeft.ent->classname, "func_door_rotating") != 0) 
		&& !self->acebot.isJumpToCrate)
	{
		trace_t traceUp;

		// Ladder code
		if (traceFront.fraction != 1 && traceFront.contents & CONTENTS_LADDER && ACEMV_CheckLadder(self, ucmd, false, true))
			return true;


		// Special uppoint logic to check for slopes/stairs/jumping etc.
		VectorSet(offset, 0, 0, 24); //hypov8 was  (, 0, 18, 24)
		G_ProjectSource(self->s.origin, offset, forward, right, upstart);

		VectorSet(offset, 0, 0, 48); // scan for height above head
		G_ProjectSource(self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE); //hypo min/max was null

		VectorSet(offset, 32, 0, 48 * traceUp.fraction /*- 5*/); // set as high as possible //hypov8 find roof??
		G_ProjectSource(self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE); //hypo min/max was null

		// If the upper trace is not open, we need to turn.
		if (traceUp.fraction != 1 || traceUp.allsolid || traceUp.startsolid)
		{
			int yaw = ((1 - traceRight.fraction) * 45.0) + ((1 - traceLeft.fraction) * 45.0);

			//hypov8 check time last uturned
			if (self->acebot.uTurnTime < level.framenum)
				self->acebot.uTurnCount = 0;

			if (traceRight.fraction > traceLeft.fraction)
			{
				//turn clockwise
				self->s.angles[YAW] -= yaw;

				//hypov8 add to uturn count
				if (self->acebot.uTurnTime > level.framenum)
					self->acebot.uTurnCount += 1;
				self->acebot.uTurnTime = level.framenum + 2;
			}
			else
			{
				//turn anticlockwise
				self->s.angles[YAW] += yaw;

				//hypov8 add to uturn count
				if (self->acebot.uTurnTime > level.framenum)
					self->acebot.uTurnCount += 1;
				self->acebot.uTurnTime = level.framenum + 2;
			}

			//U-turn when we keep spinning in circles
			if (self->acebot.uTurnCount > 20)
			{
				if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
					debug_printf(" *BOT STUCK* %s U-turn\n", self->client->pers.netname);
				self->s.angles[YAW] += 180.0;
				self->acebot.uTurnTime = 0;
			}
			if (!ACEMV_CheckLavaAndSky(self)) //add hypov8 //todo: check this
				ucmd->forwardmove = BOT_FORWARD_VEL;

			VectorCopy(self->s.angles, dir);
			AngleVectors(dir, forward, right, NULL);
			VectorSet(offset, 32, 0, 0);
			G_ProjectSource(self->s.origin, offset, forward, right, focalpoint);
			VectorSubtract(focalpoint, self->s.origin, self->acebot.move_vector);

			return true;
		}
	}	
	if (ACEMV_CheckLavaAndSky(self)== 1)
		return true; //add hypov8

	return false;
}

//hypov8
//func to stop bot looking up/down when not needed
//#define ACE_Look_Straight(target,player,out) (out[0]=target[0],out[1]=target[1],out[2]=player[2])
/*
void ACE_Look_Straight(vec3_t player, vec3_t target, vec3_t out)
{
	vec3_t out;
	out[0] = target[0];
	out[1] = target[1];
	out[2] = player[2]; //copy player height
}
*/



/*
=============
infrontBot
hypov8
returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
qboolean ACEMV_InfrontRocket(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.0)
		return true;
	return false;
}


static qboolean ACEMV_MoveDodgeRocket(edict_t *self, usercmd_t *ucmd, qboolean targetAquired)
{
	vec_t angleDif;		
	if (self->movetarget->owner) 
	{
		//look at player
		if (targetAquired || ACEMV_InfrontRocket(self, self->movetarget->owner))
		{	
			vec3_t  toRocket,rocketAngles, toPlayer,playerAngles;
			VectorSubtract(self->movetarget->s.origin, self->s.origin, toRocket);
			VectorSubtract(self->movetarget->owner->s.origin, self->s.origin, toPlayer);
			vectoangles(toRocket,rocketAngles);
			vectoangles(toPlayer,playerAngles);

			if (!targetAquired){
				self->client->ps.viewangles[2] = 0;
				VectorSubtract(self->movetarget->owner->s.origin, self->s.origin, self->acebot.move_vector);
				ACEMV_ChangeBotAngle(self);
			}
			angleDif = playerAngles[1] - rocketAngles[1];
			if (angleDif < 0.0 && angleDif > -180)// rocketAngles[1] > playerAngles[1])
			{
				if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
					ucmd->sidemove = BOT_SIDE_VEL;
					self->acebot.last_strafeDir = MOVE_RIGHT;
					self->acebot.last_strafeTime = level.framenum + 5;
				}
			}
			else
			{
				if (ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT)){
					ucmd->sidemove = -BOT_SIDE_VEL;
					self->acebot.last_strafeDir = MOVE_LEFT;
					self->acebot.last_strafeTime = level.framenum + 5;
				}
			}
			//move back to if we can
			if (ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
				ucmd->forwardmove = -BOT_FORWARD_VEL;
		}
		else //look away from rocket
		{
			self->client->ps.viewangles[2] = 0;
			VectorSubtract(self->s.origin, self->movetarget->s.origin, self->acebot.move_vector);
			ACEMV_ChangeBotAngle(self);

			if (ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD))
				ucmd->forwardmove = BOT_FORWARD_VEL;
		}
	}
	else
	{

				return true;
	}

	ACEAI_Reset_Goal_Node(self, 2.0, "Move Dodge Rocket.");
	return true;
}

static qboolean ACEMV_MoveDodgeNad(edict_t *self, usercmd_t *ucmd, qboolean targetAquired)
{
	if (!targetAquired)
	{
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(self->s.origin, self->movetarget->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
		// strafe left/right
		if (rand() % 2 && ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD))
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->sidemove = 0;
		}
		else if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove(self, MOVE_FORWARD)
			&& ACEMV_CanMove_Simple(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_FORWARD))
		{
			ucmd->sidemove = BOT_SIDE_VEL;
			ucmd->forwardmove = BOT_FORWARD_VEL;
		}
		else if (ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT))
			ucmd->sidemove = -BOT_SIDE_VEL;
	}
	else
	{
		vec3_t  toRocket, rocketAngles, toPlayer, playerAngles;
		vec_t angleDif;
		VectorSubtract(self->movetarget->s.origin, self->s.origin, toRocket);
		VectorSubtract(self->movetarget->owner->s.origin, self->s.origin, toPlayer);
		vectoangles(toRocket, rocketAngles);
		vectoangles(toPlayer, playerAngles);

		angleDif = playerAngles[1] - rocketAngles[1];
		if (angleDif < 0.0 && angleDif > -180)
		{
			if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
				ucmd->sidemove = BOT_SIDE_VEL;
				self->acebot.last_strafeDir = MOVE_RIGHT;
				self->acebot.last_strafeTime = level.framenum + 10;
			}
		}
		else
		{
			if (ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT)){
				ucmd->sidemove = -BOT_SIDE_VEL;
				self->acebot.last_strafeDir = MOVE_LEFT;
				self->acebot.last_strafeTime = level.framenum + 10;
			}
		}


		//also try to move back to if posible

		if (angleDif < 90 || angleDif > -90)
		{
			if (ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
				ucmd->forwardmove = -BOT_FORWARD_VEL;
			else 
				if (ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD))
					ucmd->forwardmove = BOT_FORWARD_VEL;
		}
	}

	ACEAI_Reset_Goal_Node(self, 2.0, "Move Dodge Nad.");
	return true;

}

//skill multiplier
float ACEMV_SkillMP(edict_t *self)
{
	float skill = sv_botskill->value * self->acebot.botSkillMultiplier;
	if (skill < 0.0f) 
		skill = 0.0f;
	else 
		if (skill >4.0f) skill = 4.0f;

	return skill;
}

//add hypov8 stops bots avoiding all rockets/nads
static qboolean ACEMV_Attack_Dodge_bySkill(edict_t *bot)
{
	static int i=0;
	float skill = ACEMV_SkillMP(bot);

	if (skill >= 3.0f)
		return true;

	if (bot->acebot.botSkillDeleyTimer2 > level.framenum){
		if (bot->acebot.last_dodgeRocket)
			return true;
		else
			return false;
	}

	bot->acebot.botSkillDeleyTimer2 = level.framenum + 40;
	i += 1;

	if (skill >= 2.0f)
	{
		if (i % 2){
			bot->acebot.last_dodgeRocket = true;
			return true;
		}
	}
	else if (skill >= 1.0f)
	{
		if (i % 3){
			bot->acebot.last_dodgeRocket = true;
			return true;
		}
	}
	else if (skill >= 0.5f)
	{
		if (i % 4){
			bot->acebot.last_dodgeRocket = true;
			return true;
		}
	}

	bot->acebot.last_dodgeRocket = false;
	return false;
}

///////////////////////////////////////////////////////////////////////
// Set bot to move to it's movetarget. (following node path)
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_MoveToGoal(edict_t *self, usercmd_t *ucmd)
{
	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	if (strcmp(self->movetarget->classname, "rocket") == 0)
	{

		if (ACEMV_Attack_Dodge_bySkill(self))
			ACEMV_MoveDodgeRocket(self, ucmd, false);
		else
		{
			self->client->ps.viewangles[2] = 0;
			VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
			ACEMV_ChangeBotAngle(self);

			// strafe left/right
			if (rand() % 2 && ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT))
				ucmd->sidemove = -BOT_SIDE_VEL;
			else if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT))
				ucmd->sidemove = BOT_SIDE_VEL;
		}
		return true;
	}
	// acebot //hypo change grenade to move back. disable walk forward to?
	else if (strcmp(self->movetarget->classname, "grenade") == 0)
	{
		if (ACEMV_Attack_Dodge_bySkill(self))
			ACEMV_MoveDodgeNad(self, ucmd, false);
		else
		{

			self->client->ps.viewangles[2] = 0;
			VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
			ACEMV_ChangeBotAngle(self);

			// strafe left/right
			if (rand() % 2 && ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
			{
				ucmd->forwardmove = -BOT_FORWARD_VEL;
				ucmd->sidemove = 0;
			}
			else if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove(self, MOVE_BACK)
				&& ACEMV_CanMove_Simple(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_BACK))
			{
				ucmd->sidemove = BOT_SIDE_VEL;
				ucmd->forwardmove = -BOT_FORWARD_VEL;
			}
			else if (ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT))
				ucmd->sidemove = -BOT_SIDE_VEL;


		}
		return true;
	}
	else if (strcmp(self->movetarget->classname, "trigger_push") == 0)
	{
		vec3_t v1, v2, itemOrigin;

		VectorCopy(self->movetarget->maxs, v1);
		VectorCopy(self->movetarget->mins, v2);
		itemOrigin[0] = (v1[0] - v2[0]) / 2 + v2[0];
		itemOrigin[1] = (v1[1] - v2[1]) / 2 + v2[1];
		itemOrigin[2] = (v1[2] - v2[2]) / 2 + v2[2];

		// Set bot's movement direction
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(itemOrigin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return false; //not explosive
	}
	else
	{		//hypov8 make bot crough if close to cash
		if (self->movetarget)
		{
			if (strcmp(self->movetarget->classname, "item_cashroll")==0)
			{
				float distToTarget = VectorDistance(self->s.origin, self->movetarget->s.origin);

				if (distToTarget <= 32)
					ucmd->upmove = -400;
			}
		}

		// Set bot's movement direction
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return false;
	}
}


//face towards ladder surface
qboolean ACEMV_CheckLadder(edict_t *self, usercmd_t *ucmd, qboolean isTopOfLadder, qboolean isKnownLadder)
{
	int i;
	trace_t tr; // for eyesight
	qboolean checkDir= false;
	vec3_t dir, forward, right, offset, wall;

	if (!isKnownLadder && !self->acebot.isOnLadder)
	{
		VectorCopy(self->s.angles, dir);
		dir[PITCH] = dir[ROLL] = 0.0f;
		AngleVectors(dir, forward, right, NULL);
		VectorSet(offset, 8, 0, 0); // focalpoint 
		G_ProjectSource(self->s.origin, offset, forward, right, wall);

		tr = gi.trace(self->s.origin, self->mins, self->maxs, wall, self, CONTENTS_SOLID | CONTENTS_LADDER);
		if (tr.contents & CONTENTS_LADDER)
			checkDir = true;
	}

	if (isKnownLadder || checkDir||self->acebot.isOnLadder)
	{
		qboolean foundSurfaceDir = false;

		for (i = 0; i <= 3; i++)
		{
			vec3_t minx = { -2, -2, -22 };
			vec3_t maxx = { 2, 2, 48 };
			vec3_t angles;
			vec3_t target;
			VectorCopy(self->s.origin, target);
			switch (i)
			{
				case 0:	target[0] += 18; break; //20 units infront
				case 1:	target[0] -= 18; break;
				case 2:	target[1] += 18; break;
				case 3:	target[1] -= 18; break;
			}

			tr = gi.trace(self->s.origin, minx, maxx, target, self, CONTENTS_SOLID | CONTENTS_LADDER);
			if (tr.contents & CONTENTS_LADDER)
			{ //face ladder
				VectorSubtract(target, self->s.origin, self->acebot.move_vector);
				vectoangles(self->acebot.move_vector, angles);
				VectorCopy(angles, self->s.angles);
				foundSurfaceDir = true;
				ACEMV_ChangeBotAngle(self);
				break;
			}
		}

		if (foundSurfaceDir) //found ladder face
		{
			if (self->groundentity==NULL&& self->acebot.ladder_time <= level.framenum 
				&& (!isKnownLadder || self->acebot.state == BOTSTATE_WANDER))
			{
				short closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY_QUART, BOTNODE_LADDER);
				if (closest_node != INVALID && nodes[closest_node].origin[2] > self->s.origin[2])
				{
					self->s.origin[0] = nodes[closest_node].origin[0];
					self->s.origin[1] = nodes[closest_node].origin[1];
				}
			}
			ucmd->upmove = BOT_JUMP_VEL;
			//ucmd->forwardmove = 0;
			self->acebot.ladder_time = level.framenum + 2; //hypov8 was 3
			self->acebot.isOnLadder = true; //hypo stop bot attacking on ladders
			return true;
		}
	}

	return false; //trace null


}


//hypov8
//remove short range goals and set jump pad node as hit, incase in bad location
void ACEMV_JumpPadUpdate(edict_t *bot/*, float pushSpeed*/)
{
	//bot used jump pad.
	if (bot->acebot.is_bot)
	{
		//hypov8 todo: wander/goal. make jump pads a goal?
		bot->acebot.trigPushTimer = level.framenum + 5; //0.5 seconds untill bot can move
		bot->acebot.isMovingUpPushed = true;
		bot->goalentity = NULL; //wander?
		bot->goal_ent;
		bot->last_goal;
		//find closest node and link to next node
		bot->movetarget = NULL;

		bot->acebot.state = BOTSTATE_WANDER;
		bot->acebot.node_current = INVALID;
		bot->acebot.node_next = INVALID;
		bot->acebot.node_goal = INVALID;

		{ //hypov8 new ver
			bot->acebot.isTrigPush = true;
			VectorCopy(bot->s.origin, bot->acebot.oldOrigin);
			bot->acebot.state = BOTSTATE_WANDER; //reset goal
			bot->acebot.targetPlayerNode = 0;
			bot->acebot.wander_timeout = level.time + 0.2;
			bot->s.angles[PITCH] = 0;
		}
	}
}


///////////////////////////////////////////////////////////////////////
// Main movement code. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_Move(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dist;
	int current_node_type = -1;
	int next_node_type = -1;
	int i;
	qboolean isExplosive = 0;

	//add hypov8
	if (self->acebot.isTrigPush && self->groundentity) //todo:
		self->acebot.isTrigPush = false;

	// Get current and next node back from nav code.
	if (!ACEND_FollowPath(self))
	{
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.targetPlayerNode = 0;
		self->acebot.wander_timeout = level.time + 1.0;
		return;
	}

	current_node_type = nodes[self->acebot.node_current].type;
	next_node_type = nodes[self->acebot.node_next].type;

	///////////////////////////
	// Move To Goal
	///////////////////////////
	if (self->movetarget)
		isExplosive = ACEMV_MoveToGoal(self, ucmd);

	////////////////////////////////////////////////////////
	// Grapple
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_GRAPPLE)
	{
		if (!(self->client->hookstate & HOOK_ON))
		{
			vec3_t vHeight;
			VectorCopy(self->s.origin, vHeight);
			vHeight[2] += self->viewheight;

			// Set viewport Aim direction
			VectorSubtract(nodes[self->acebot.node_next].origin, vHeight, self->acebot.move_vector);
			vectoangles(self->acebot.move_vector, self->s.angles);
			VectorCopy(self->s.angles, self->client->v_angle);

			//stop moving
			VectorCopy(vec3_origin, self->velocity);

			// flags hook as being active 
			self->client->hookstate = HOOK_ON;
			self->client->hookstate |= SHRINK_ON;
			self->acebot.hookDistLast = hook_max_length->value;
			self->acebot.hookDistCurrent = hook_max_length->value-1;
			FireHook(self);
		}
		else
			ACEMV_ChangeBotAngle(self);
		return;
	}

	// Reset the grapple if hangin on a graple node
	if(current_node_type == BOTNODE_GRAPPLE)
	{
		if (self->groundentity)
			ucmd->forwardmove = BOT_FORWARD_VEL;

		self->client->hookstate = 0;
		ACEMV_ChangeBotAngle(self);
		return;
	}

	////////////////////////////////////////////////////////
	// Platforms
	///////////////////////////////////////////////////////
	if (current_node_type != BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// check to see if lift is down?
		for (i = 0; i < num_items; i++)
			if (item_table[i].node == self->acebot.node_next)
				if (item_table[i].ent->moveinfo.state != STATE_BOTTOM)
				{
					if (self->acebot.plateWaitTim >= level.framenum && self->acebot.plateWaitTim <= level.framenum + 10)
					{
						self->acebot.state = BOTSTATE_WANDER;
						self->acebot.wander_timeout = level.time + 2.0;
						return;
					}
					if (self->acebot.plateWaitTim < level.framenum)

						self->acebot.plateWaitTim = level.framenum + 70;
					return; // Wait for elevator
				}
	}
	if (current_node_type == BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// Move to the center
		self->acebot.move_vector[2] = 0; // kill z movement	
		if (VectorLength(self->acebot.move_vector) > 10)
			ucmd->forwardmove = (BOT_FORWARD_VEL * 0.75); // walk to center

		ACEMV_ChangeBotAngle(self);

		return; // No move, riding elevator
	}

	////////////////////////////////////////////////////////
	// Jumpto Nodes
	///////////////////////////////////////////////////////
	if (next_node_type == BOTNODE_JUMP /*&& self->groundentity*/ ||
		(current_node_type == BOTNODE_JUMP 	&& next_node_type != BOTNODE_ITEM && nodes[self->acebot.node_next].origin[2] > self->s.origin[2]))
	{
		// Set up a jump move

		if (self->acebot.moveDirVel > 2) //add hypov8. bot can be blocked in mid air. //&& self->groundentity == NULL
		{
			if (!self->acebot.isTrigPush) //todo:
			{
				ucmd->forwardmove = BOT_FORWARD_VEL;
				ucmd->upmove = BOT_JUMP_VEL;
				ACEMV_ChangeBotAngle(self);
				VectorCopy(self->acebot.move_vector, dist);
				VectorScale(dist, 440, self->velocity);
			}
			else
			{
				ucmd->forwardmove = 0;
				ucmd->upmove = 0;
				self->s.angles[PITCH] = 0;
			}
			return;
		}

		else
		{
			//add hypov8. drop to floor if we didnt reach target.
			ACEAI_Reset_Goal_Node(self, 1.0, "Bot Stuck in the air.");
			//self->velocity[2] -= self->gravity * sv_gravity->value * FRAMETIME;
#if HYPODEBUG
			gi.dprintf("Bot %s Stuck in air at XYZ:(%f, %f, %f)\n", self->client->pers.netname, self->s.origin[0], self->s.origin[1], self->s.origin[2]);
#endif		
			return;
		}
	}
	
	////////////////////////////////////////////////////////
	// Ladder Nodes
	///////////////////////////////////////////////////////
#ifdef HYPODEBUG
	if (self->s.origin[0]==794.125000 && self->s.origin[1] > -1500 &&self->s.origin[2]> 64)
		gi.dprintf("ladder\n" );
#endif


	if (next_node_type == BOTNODE_LADDER && nodes[self->acebot.node_next].origin[2] > self->s.origin[2])
	{
		if (current_node_type == BOTNODE_LADDER)
		{
			if (self->acebot.node_next != self->acebot.node_current							//make sure its a differnt node
				&& self->s.origin[2] > nodes[self->acebot.node_current].origin[2]			//make sure we are higher then last node
				&& nodes[self->acebot.node_next].origin[2] > nodes[self->acebot.node_current].origin[2] )	//make sure next node is above
			{
				vec3_t moveDist,moveTO;
				vec_t heightUP,  heightTOTAL,heightMOVE;
				trace_t trace;

				//hypov8 add: adjust bots X/Y origin to line up between the current and next nodes
				//stop a bug caused from bot slowly looking towards next node because of direction change
				heightTOTAL = nodes[self->acebot.node_next].origin[2] - nodes[self->acebot.node_current].origin[2];
				heightUP = heightTOTAL - (nodes[self->acebot.node_next].origin[2] - self->s.origin[2]);
				heightMOVE = heightUP / heightTOTAL;

				VectorSubtract(nodes[self->acebot.node_next].origin, nodes[self->acebot.node_current].origin, moveDist);
				VectorMA(nodes[self->acebot.node_current].origin, heightMOVE, moveDist, moveTO);

				trace = gi.trace(self->s.origin, self->mins, self->maxs, moveTO, self, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
				if (trace.fraction == 1.0f && !trace.allsolid && !trace.startsolid) //double check our destintion. old maps..
				{
					self->s.origin[0] = moveTO[0];
					self->s.origin[1] = moveTO[1];
				}
#if HYPODEBUG
				else
					gi.dprintf("ERROR Bot caught in void\n" );

#endif

			}
		}

		if (ACEMV_CheckLadder(self, ucmd, false, true))
			return;
		else
		{
#if 0 //hypov8 todo: should we stop bots jumping if current node to far from ladder?
			float distToTarget;
			//look at next node, flat
			VectorCopy(nodes[self->acebot.node_next].origin, dist);
			dist[2] = self->s.origin[2];
			VectorSubtract(dist, self->s.origin, self->acebot.move_vector);
			distToTarget = VectorDistance(self->s.origin, dist);
			if (distToTarget< 16)
				ucmd->upmove = BOT_JUMP_VEL;
			ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
			ACEMV_ChangeBotAngle(self);
			return;
#else
			ucmd->upmove = BOT_JUMP_VEL;
			ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
			ACEMV_ChangeBotAngle(self);
			return;
#endif
		}
	}

	// If getting off the ladder
	if(current_node_type == BOTNODE_LADDER && next_node_type != BOTNODE_LADDER /*&&
		nodes[self->acebot.node_next].origin[2] > self->s.origin[2]-8*/) //move player origin down// increase??
	{
		if (!ACEMV_CheckLadder(self, ucmd,true,true))
		{
			// keep moving up
			if (self->acebot.isOnLadder)
			{
				ucmd->forwardmove = BOT_FORWARD_VEL;
				ucmd->upmove = 50;
			}
			else//move only forward
			{
				vec3_t angles;
				ucmd->forwardmove = BOT_FORWARD_VEL;
				VectorSubtract(nodes[self->acebot.node_next].origin, self->s.origin, self->acebot.move_vector);
				vectoangles(self->acebot.move_vector, angles);
				VectorCopy(angles, self->s.angles);
				ACEMV_ChangeBotAngle(self);
			}
		}
		return;
	}

	////////////////////////////////////////////////////////
	// Water Nodes
	///////////////////////////////////////////////////////
	if(current_node_type == BOTNODE_WATER)
	{
		// We need to be pointed up/down
		ACEMV_ChangeBotAngle(self);

		// If the next node is not in the water, then move up to get out.
		if (next_node_type != BOTNODE_WATER && !(gi.pointcontents(nodes[self->acebot.node_next].origin) & MASK_WATER)) // Exit water
			ucmd->upmove = BOT_JUMP_VEL;
		
		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
		return;

	}
	
	// Falling off ledge? //hypov8 or on ladder
	if (!self->groundentity && !self->acebot.isMovingUpPushed)
	{

		if (!ACEMV_CheckLadder(self, ucmd, false, false))
		{
			//if (self->acebot.node_next != INVALID)
				//VectorSubtract(nodes[self->acebot.node_next].origin, self->s.origin, self->acebot.move_vector);

			ACEMV_ChangeBotAngle(self);
			self->velocity[0] = self->acebot.move_vector[0] * 360;
			self->velocity[1] = self->acebot.move_vector[1] * 360;
		}
		return;	
	}
#if 0
	//hypov8 add. if node just above us, jump. fix missplaced nodes
	if (self->acebot.moveDirVel < 5 && next_node_type == BOTNODE_MOVE)
	{
		if ((nodes[self->acebot.node_next].origin[2] - self->s.origin[2]- BOTNODE_SHIFT) > 18 &&
			(nodes[self->acebot.node_next].origin[2] - self->s.origin[2]- BOTNODE_SHIFT) <= 60)
		{
			float distToTarget = VectorDistance(self->s.origin, nodes[self->acebot.node_next].origin);

			if (distToTarget <= BOTNODE_DENSITY_HALVE)
			{
				ucmd->upmove = BOT_JUMP_VEL;
			}
		}

	}
#endif	
	// Check to see if stuck, and if so try to free us
	// Also handles crouching
	if (self->acebot.moveDirVel < 15 && !self->acebot.isMovingUpPushed)
	//if (VectorLength(self->velocity) < 37 && !self->acebot.isMovingUpPushed)
	{
		// Keep a random factor just in case....
		if(random() > 0.1 && ACEMV_SpecialMove(self, ucmd))
			return;
		
		self->s.angles[YAW] += random() * 180 - 90; 

		ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}

	// Otherwise move as fast as we can
	if (!isExplosive)
		ucmd->forwardmove = BOT_FORWARD_VEL;


	if (self->acebot.isMovingUpPushed && next_node_type == BOTNODE_MOVE)
	{
		vec3_t angles;
		//self->client->ps.viewangles[2] = 0;
		//VectorSubtract(nodes[self->acebot.node_next].origin, self->s.origin, self->acebot.move_vector);

		VectorSubtract(nodes[self->acebot.node_next].origin, self->s.origin, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);
	}

	ACEMV_ChangeBotAngle(self);	
}

void ACEMV_WanderPlat(edict_t *self, usercmd_t *ucmd, edict_t *plate)
{
	vec3_t vCenter, v1, v2;
	float distToTarget;
	VectorCopy(plate->maxs, v1);
	VectorCopy(plate->mins, v2);

	// To get the center of plate
	vCenter[0] = (v1[0] - v2[0]) / 2 + v2[0];
	vCenter[1] = (v1[1] - v2[1]) / 2 + v2[1];
	vCenter[2] = self->s.origin[2];

	distToTarget = VectorDistance(self->s.origin, vCenter);

	// hypo moving up on a plate, stand still //todo move to centre
	if (distToTarget <= 16 && self->s.origin[2] > self->acebot.oldOrigin[2] + 2)
	{
		self->velocity[0] = 0;
		self->velocity[1] = 0;
		self->velocity[2] = 0;
		self->acebot.next_move_time = level.time + 0.5;
		return;
	}
	else
	{
		vec3_t angles;
		VectorSubtract(vCenter, self->s.origin, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);
		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
		return;
	}
}

///////////////////////////////////////////////////////////////////////
// Wandering code (based on old ACE movement code) 
///////////////////////////////////////////////////////////////////////
void ACEMV_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;
	qboolean isExplosive = 0;

	trace_t trace; // for lift
	vec3_t minx = { -4, -4, -24 };
	vec3_t maxx = { 4, 4, -24 };

	// Do not move
	if (self->acebot.next_move_time > level.time)
		return; //todo: func plate timmer. check if stuck underneath, allow move

	self->s.angles[PITCH] = 0; //hypov8 reset pitch

	self->client->hookstate = 0; //reset hook. should not be on

	//hypov8 jumping upto crate timmer //todo: check this
	/*if (self->acebot.isJumpToCrate) {
		if (level.framenum > self->acebot.crate_time)
			self->acebot.isJumpToCrate = false;	}*/

	////////////////////////////////
	// Plate?
	////////////////////////////////
	VectorCopy(self->s.origin, temp);
	temp[2] -= 2;
	trace = gi.trace(self->s.origin, minx, maxx, temp, self, CONTENTS_SOLID | CONTENTS_ALPHA);

	if (Q_stricmp(trace.ent->classname, "func_plat")== 0)
	{
		if (trace.ent->moveinfo.state == STATE_UP ||
			trace.ent->moveinfo.state == STATE_DOWN) // only move when platform not
		{
			ACEMV_WanderPlat(self, ucmd, trace.ent);
			return;
		}
	}

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if (self->groundentity != NULL && self->groundentity->use == Use_Plat)
	{
		if (self->groundentity->moveinfo.state == STATE_UP ||
			self->groundentity->moveinfo.state == STATE_DOWN) // only move when platform not
		{
			ACEMV_WanderPlat(self, ucmd, self->groundentity);
			return;
		}
	}
	
	
	// Is there a target to move to
	if (self->movetarget)
		isExplosive = ACEMV_MoveToGoal(self, ucmd);
		
	////////////////////////////////
	// Swimming?
	////////////////////////////////
	VectorCopy(self->s.origin,temp); //hypov8 swimming, water depth???
	temp[2]+=8;

	if(gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if(self->client->next_drown_time > 0)
		{
			ucmd->upmove = 1;	
			self->s.angles[PITCH] = -45; 
		}
		else
		{
			if (self->goalentity  && self->goalentity->solid != SOLID_NOT)
			{
				int index;
				float weight;
				index = ACEIT_ClassnameToIndex(self->goalentity->classname, self->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeed(self, index, 0.0, self->goalentity->spawnflags);

				if (weight > 0.0f)
				{
					vec3_t angles;
					// Set direction
					VectorSubtract(self->goalentity->s.origin, self->s.origin, self->acebot.move_vector);
					vectoangles(self->acebot.move_vector, angles);
					VectorCopy(angles, self->s.angles);
					ucmd->forwardmove = BOT_FORWARD_VEL *0.75;

					//add hypov8 jump if target above us
					if (self->s.origin[2] < self->goalentity->s.origin[2])
						ucmd->upmove = BOT_JUMP_VEL;

				}
				//self->movetarget->s.origin;
				else
					ucmd->upmove = BOT_JUMP_VEL;
			}
			else
			{
				ucmd->upmove = BOT_JUMP_VEL;
				self->s.angles[PITCH] = -10;
			}
		}

		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
	}
	else
		self->client->next_drown_time = 0; // probably shound not be messing with this, but
	
	////////////////////////////////
	// Lava?
	////////////////////////////////
	temp[2]-=48;	//hypov8 was 48
	if(gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//	safe_bprintf(PRINT_MEDIUM,"lava jump\n");
		self->s.angles[YAW] += random() * 360 - 180; 
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = BOT_JUMP_VEL;
		return;
	}

	if (!self->acebot.isMovingUpPushed)
		if(ACEMV_CheckEyes(self,ucmd))
			return;


#if 0 // add hypo dont fall to death or lava
	{
		vec3_t dir, forward, right, offset,start, down;
		trace_t trace; // for eyesight
		vec3_t minx = { 0, 0, -24 };
		vec3_t maxx = { 0, 0, -24 };

		// Get current angle and set up "eyes"
		VectorCopy(self->s.angles, dir);
		AngleVectors(dir, forward, right, NULL);
		VectorSet(offset, 64, 0, 0); // focalpoint 
		G_ProjectSource(self->s.origin, offset, forward, right, start);
		VectorCopy(forward, down);
		down[2] -= CHECKSKYDOWNDIST;

		trace = gi.trace(start, self->mins, self->maxs, down, self, CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_SOLID);
		if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
		{
			self->s.angles[YAW] += 90;
			return;
		}
		if ((trace.surface->flags & SURF_SKY))
		{
			self->s.angles[YAW] += 90;
			return;
		}
	}

#endif
	// Check for special movement if we have a normal move (have to test)
	if (self->acebot.moveDirVel < 15 && !self->acebot.isMovingUpPushed) //todo was 2. 5ok?
	//if (VectorLength(self->velocity) < 37 && !self->acebot.isMovingUpPushed) //hypov8 jump pad
	{
		//vec3_t start;
		if(random() > 0.1 && ACEMV_SpecialMove(self,ucmd))
			return;

		self->s.angles[YAW] += random() * 180 - 90; 

		if(!M_CheckBottom(self) || !self->groundentity) // if there is ground continue otherwise wait for next move
			ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}

	//look in direction of movement
	if (self->acebot.isMovingUpPushed)
	{
		vec3_t  tmp2, angles;

		VectorCopy(self->acebot.oldOrigin, tmp2);
		tmp2[2] = self->s.origin[2];

		VectorSubtract(self->s.origin, tmp2, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);
	}

	if (!isExplosive)
		ucmd->forwardmove = BOT_FORWARD_VEL;

}


//hypov8 random player taunts
void ACEMV_BotTaunt(edict_t *self, edict_t *enemy)
{
	int randomTaunt;

	if (self->gender == GENDER_MALE)
	{
		randomTaunt = rand() % 11;
		switch (randomTaunt)
		{
		case 0:		Voice_Random(self, enemy, player_profanity_level2, NUM_PLAYER_PROFANITY_LEVEL2);break;
		case 1:		Voice_Random(self, enemy, kingpin_random, NUM_KINGPIN_RANDOM);					break;
		case 2:		Voice_Random(self, enemy, leroy_random, NUM_LEROY_RANDOM);						break;
		case 3:		Voice_Random(self, enemy, mj_random, NUM_MJ_RANDOM);							break;
		case 4:		Voice_Random(self, enemy, momo_random, NUM_MOMO_RANDOM);						break;
		case 5:		Voice_Random(self, enemy, lamont_random, NUM_LAMONT_RANDOM);					break;
		case 6:		Voice_Random(self, enemy, jesus_random, NUM_JESUS_RANDOM);						break;
		case 7:		Voice_Random(self, enemy, tyrone_random, NUM_TYRONE_RANDOM);					break;
		case 8:		Voice_Random(self, enemy, willy_random, NUM_WILLY_RANDOM);						break;
		case 9:		Voice_Random(self, enemy, moker_random, NUM_MOKER_RANDOM);						break;
		case 10:	
		default:	Voice_Random(self, enemy, heilman_random, NUM_HEILMAN_RANDOM);					break;
		}
	}
	else if (self->gender == GENDER_FEMALE)
	{
		randomTaunt = rand() % 7;
		switch (randomTaunt)
		{
		case 0: 	Voice_Random(self, enemy, f_profanity_level2, F_NUM_PROFANITY_LEVEL2);	break;
		case 1:		Voice_Random(self, enemy, bambi_random, F_NUM_BAMBI_RANDOM);			break;
		case 2:		Voice_Random(self, enemy, yolanda_random, F_NUM_YOLANDA_RANDOM);		break;
		case 3:		Voice_Random(self, enemy, mona_random, F_NUM_MONA_RANDOM);				break;
		case 4:		Voice_Random(self, enemy, lola_random, F_NUM_LOLA_RANDOM);				break;
		case 5:		Voice_Random(self, enemy, blunt_random, F_NUM_BLUNT_RANDOM);			break;
		case 6:	
		default:	Voice_Random(self, enemy, beth_random, F_NUM_BETH_RANDOM);				break;
		}
	}
}


static void ACEMV_Attack_AimRandom(edict_t *self)
{
	float range, ran, rand_y, disAcc, skill, vel;
	vec3_t velTmp;

	if (!self->enemy)
		return;


	if (self->enemy->acebot.hunted)
	{
		self->acebot.bot_accuracy = 0; //no randomness for top players
	}
	else
	{
		// Get distance.
		range = VectorDistance(self->acebot.enemyOrigin, self->s.origin);
		//get enemy speed
		VectorCopy(self->enemy->velocity, velTmp);
		velTmp[2] = 0;
		vel = (float)VectorLength(velTmp);

		// the closer the distance, the more accurate..
		// so we make it less accurate the closer we get
		if (range < 120)		disAcc = 35;
		else if (range < 200)	disAcc = 30;
		else if (range < 250)	disAcc = 26;
		else if (range < 300)	disAcc = 22;
		else if (range < 350)	disAcc = 17;
		else if (range < 475)	disAcc = 13;
		else if (range < 555)	disAcc = 10;
		else					disAcc = 6;

#if 0//def HYPODEBUG
		disAcc = 500;
#endif

		//rand scaled between -1 to 1
		ran = random();
		 if (ran > 0.30 && ran < 0.70)
			ran = random(); //2nd try at less accurecy
		ran = (ran * 2) - 1;

		//inverse skill 1.0 to 0.0 (0.0 more accurate)
		skill = (1 - (ACEMV_SkillMP(self) *.25));
		if (vel < 50)		skill *= .25;//skill *= (vel / 250); //more accurate when standing still	
		else if (vel < 100)	skill *= .5;
		else if (vel < 150)	skill *= .75;

		rand_y = skill * ran;

		if (self->onfiretime > 0)
			disAcc *= 2;
			//rand_y *= 2;

		if (Q_stricmp(self->client->pers.weapon->classname, "weapon_flamethrower") == 0)
			disAcc *= 1.5;//less skill for flammer


		self->acebot.bot_accuracy = rand_y * disAcc;
	}
}

void ACEMV_Attack_AimRandom_offset(edict_t *self, vec3_t aimdir)
{
	vec3_t v, v2, viewH;

	VectorCopy(self->s.origin, viewH);
	//viewH[2] += self->viewheight;

	VectorSubtract(self->acebot.enemyOrigin, viewH, v2);
	vectoangles(v2, v);

	v[YAW] += self->acebot.bot_accuracy;
	if (v[YAW] < 0)		
		v[YAW] += 360;
	else if	(v[YAW] > 360)		
		v[YAW] -= 360;

	AngleVectors(v, v2, NULL, NULL);
	VectorCopy(v2, aimdir);
}

//add hypov8
//random bullet dir. stops bot view being jerky
void ACEMV_Attack_CalcRandDir(edict_t *self, vec3_t aimdir)
{
	//random aim
	ACEMV_Attack_AimRandom(self);
	ACEMV_Attack_AimRandom_offset(self, aimdir); //flammer just do view offset
}


static void ACEMV_Attack_Dodge(edict_t *self, usercmd_t *ucmd)
{
	float c;
	qboolean moveResolved = false;
	qboolean strafe = false;
	qboolean strafeDir;
	static const int frames = 5;

	//stop strafe with low skill
	if (ACEMV_SkillMP(self) <= 3)	
	{
		float c;
		float skill= ACEMV_SkillMP(self) * .25; //0 to 1
		c = random();

		if (c > .2 && skill < c)
			return;
	}

#if 1
	if (self->movetarget && self->movetarget->owner == self->enemy && ACEMV_Attack_Dodge_bySkill(self))
	{
		if (strcmp(self->movetarget->classname, "rocket") == 0)
		{
			ACEMV_MoveDodgeRocket(self, ucmd, true);
			return;
		}
		else if (strcmp(self->movetarget->classname, "grenade") == 0)
		{
			ACEMV_MoveDodgeNad(self, ucmd, true);
			return;
		}
	}

#endif

	//hypo make player strafe in 1 dir longer
	if (self->acebot.last_strafeTime >= level.framenum)
	{
		strafe = true;
		if (self->acebot.last_strafeDir == MOVE_LEFT)
			strafeDir = MOVE_LEFT;
		else if (self->acebot.last_strafeDir == MOVE_RIGHT)
			strafeDir = MOVE_RIGHT;
	}

	// Randomly choose a movement direction
	c = random();

#if 1 //ndef HYPODEBUG

	//dont move on jump pads
	if (!self->acebot.isMovingUpPushed)
	{
		//Com_Printf("strafeDir=%i, strafe=%i, rand=%d\n", self->acebot.last_strafeDir, strafe, c);

		if (((c < 0.500f && !strafe) || (strafe && strafeDir == MOVE_LEFT))
			&& ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			ucmd->sidemove -= BOT_SIDE_VEL;
			moveResolved = true;
			if (!strafe){
				self->acebot.last_strafeTime = level.framenum + frames;
				self->acebot.last_strafeDir = MOVE_LEFT;
			}
		}
		else if (((c >= 0.500f && !strafe) || (strafe && strafeDir == MOVE_RIGHT))
			&& ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			ucmd->sidemove += BOT_SIDE_VEL;
			moveResolved = true;
			if (!strafe){
				self->acebot.last_strafeTime = level.framenum + frames;
				self->acebot.last_strafeDir = MOVE_RIGHT;
			}
		}

		if (c < 0.3 && ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD)){
			ucmd->forwardmove += BOT_FORWARD_VEL;
			moveResolved = true;
		}
		else if (c > 0.7 && ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK)){ //was forward??
			ucmd->forwardmove -= BOT_FORWARD_VEL;
			moveResolved = true;
		}
	}

	//hyopv8 stop bots momentum running off edges, with no move random resolves
	if (!moveResolved && !self->acebot.isMovingUpPushed && self->groundentity)
		if (c < 0.500f)
		{
			if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
				ucmd->sidemove -= BOT_SIDE_VEL;
				moveResolved = true;
			}
			else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
				ucmd->sidemove += BOT_SIDE_VEL;
				moveResolved = true;
			}
		}
		else
		{
			if (ACEMV_CanMove_Simple(self, MOVE_RIGHT))	{
				ucmd->sidemove += BOT_SIDE_VEL;
				moveResolved = true;
			}
			else if (ACEMV_CanMove_Simple(self, MOVE_LEFT))	{
				ucmd->sidemove -= BOT_SIDE_VEL;
				moveResolved = true;
			}
		}

	if (!moveResolved)
	{
		if (ACEMV_CanMove_Simple(self, MOVE_FORWARD))
			ucmd->forwardmove += BOT_FORWARD_VEL;
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK))
			ucmd->forwardmove -= BOT_FORWARD_VEL;
	}
#endif

}

///////////////////////////////////////////////////////////////////////
// Attack movement routine
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
///////////////////////////////////////////////////////////////////////
void ACEMV_Attack (edict_t *self, usercmd_t *ucmd)
{
	vec3_t  target, angles;
	vec3_t player_origin;
	float range, pSpeed;
	qboolean boozooka = false;
	qboolean grenad = false;
	qboolean flameGun = false;

// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
		ACEAI_ResetLRG_HM(self); //attack instead of going to goal
// END

	//setup dodge
	//ACEMV_Attack_Dodge(self, ucmd); //hypov8 meved down. after we look at enemy

	//hypov8 taunt
	if (self->acebot.tauntTime < level.framenum)
	{
		float c = random();
		ACEMV_BotTaunt(self, self->enemy);
		self->acebot.tauntTime = level.framenum + (c * 900);
	}

	//on hook while attacking
	if (self->client->hookstate != 0 &&
		self->acebot.node_next != INVALID && nodes[self->acebot.node_next].type == BOTNODE_GRAPPLE)
	{
		vec3_t to,v;
		// Are we there yet?
		VectorCopy(self->s.origin, to);
		to[2] += 8; //shift up!
		VectorSubtract(to, nodes[self->acebot.node_next].origin, v);

		if  ( VectorLength(v) < (64 + hook_min_length->value))
		{
			//stop velocity. drop to floor
			VectorCopy(vec3_origin, self->velocity);
			self->client->hookstate = 0;
		}
	}
	
	// Set the attack 
	ucmd->buttons = BUTTON_ATTACK;
	
	// Location to Aim at
	VectorCopy(self->enemy->s.origin,target);
	//target[2] -= 8; //move down some for recoil

	// Get distance.
	range = VectorDistance(target, self->s.origin);
	pSpeed = VectorDistance(self->enemy->velocity, vec3_origin);

#if 0	//ATTACK RANDOM
	{	
		float rand_xy, skillMove_xy, disAcc;
		float ran = (random() - 0.5);

		// closer the distance, the more accurate
		if (range < 120)		disAcc = 10;
		else if (range < 200)	disAcc = 20;
		else if (range < 350)	disAcc = 30;
		else if (range < 550)	disAcc = 40;
		else if (range < 750)	disAcc = 50;
		else					disAcc = 60;

		rand_xy = (4 * ran) - (sv_botskill->value * ran);
		skillMove_xy = (4 * disAcc) - (sv_botskill->value * disAcc);

		if (self->onfiretime > 0)
			skillMove_xy += (disAcc* 0.75);

		if (self->enemy->acebot.hunted)
			skillMove_xy = 1;

		//modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
		target[0] += rand_xy * skillMove_xy;
		target[1] += rand_xy * skillMove_xy;
	}
#endif	//end ATTACK RANDOM

#if 1 // hypov8 special movement for crouch enemy and using GL/RL
	
	if (self->client->pers.weapon)
	{
		if ((Q_stricmp(self->client->pers.weapon->classname, "weapon_bazooka") == 0))
		{
			if (range >= 300)
			{
				//predict player movement
				vec3_t out, velFlat, minx = {-8,-8,-8}, maxx = {8,8,8};
				trace_t trace;
				float skill = ACEMV_SkillMP(self) * .25; //0-1
				float move = ((range/900)*FRAMETIME) * skill;

				VectorCopy(self->enemy->velocity,velFlat);
				//player jumping?
				if (velFlat[2] > 0.0f)
					velFlat[2] = 0.0f;
				if (velFlat[2] < 350.0f)
					velFlat[2] = 0.0f;


				trace= gi.trace(self->enemy->s.origin, minx, maxx, velFlat, self, MASK_BOT_ATTACK_NONLEAD);
				if ( trace.fraction != 1.0f)
					move *= trace.fraction;

				VectorMA(self->enemy->s.origin, move,velFlat,out); //+trace
				VectorCopy(out,target);
			}


			if (self->acebot.aimLegs)	{
				boozooka = true;
				target[2] -= 32; //hypov8 move bots aim at feet (72 kp)
			}

			if (range < 80)	{
				if (ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
					ucmd->forwardmove -= BOT_FORWARD_VEL;
				ucmd->buttons = 0;
			}
		}
		else if ((Q_stricmp(self->client->pers.weapon->classname, "weapon_grenadelauncher") == 0))
		{
			float range;
			vec3_t v;
			VectorSubtract(self->enemy->s.origin, self->s.origin, v);
			range = VectorLength(v);
			if (range >=300) 
				target[2] += 100;
			else if (range <150)
				target[2] -= 50;
			else if (range <80)
				target[2] -= 70;
			else if (range <40)
				target[2] -= 90;

			grenad = true;
		}
		else if ((Q_stricmp(self->client->pers.weapon->classname, "weapon_flamethrower") == 0))
		{
			if( ACEMV_SkillMP(self) < 3.2f)
				flameGun=true;
		}

		else if (Q_stricmp(self->client->pers.weapon->classname, "weapon_crowbar") == 0
			||Q_stricmp(self->client->pers.weapon->classname, "weapon_blackjack") == 0)
		{
			vec3_t forward2, right2;
			vec3_t offset2, start2, end2;
			vec3_t angles2;
			trace_t tr2;

			ucmd->forwardmove = BOT_FORWARD_VEL; //hypov8 todo: jump.. to void?

			// Now check to see if move will move us off an edgemap team_fast_cash
			VectorCopy(self->s.angles, angles2);

			// Set up the vectors
			AngleVectors(angles2, forward2, right2, NULL);

			//check ground for obsticle
			VectorSet(offset2, 0, 0, 4);
			G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
			VectorSet(offset2, 16, 0, 4);
			G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
			tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, MASK_BOT_ATTACK_NONLEAD /*MASK_OPAQUE MASK_BOT_SOLID_FENCE*/);
			if (tr2.fraction != 1.0)
			{
				VectorSet(offset2, 0, 0, 60);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
				VectorSet(offset2, 16, 0, 60);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
				tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, MASK_BOT_ATTACK_NONLEAD /*MASK_OPAQUE MASK_BOT_SOLID_FENCE*/);
				if (tr2.fraction == 1.0 /*&& tr.fraction != 1 || tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)*/)
				{
					ucmd->upmove = BOT_JUMP_VEL; //hypo was 400
				}
				else
				{
					VectorSet(offset2, 0, 0, 36);
					G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
					VectorSet(offset2, 16, 0, 36);
					G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
					tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, MASK_BOT_ATTACK_NONLEAD /*MASK_OPAQUE MASK_BOT_SOLID_FENCE*/);
					if (tr2.fraction == 1.0 /*&& tr.fraction != 1 || tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)*/)
					{
						ucmd->upmove = BOT_JUMP_VEL; //hypo was 400
					}
				}
			}
		}
	}
#endif

	//move aim down if player is crouch. not for RL or GL
	if (!boozooka && !grenad)
	{
		if ((self->enemy->client->ps.pmove.pm_flags & PMF_DUCKED) && !self->acebot.aimLegs)
			target[2] -= 28; //hypov8 move bots aim down todo: check fence???
		else if (self->acebot.aimLegs)
			target[2] -= 36; //player obstructed with something above legs
	}


	//set enemy origin for bullet calculation. includes look up/down
	VectorCopy(target, self->acebot.enemyOrigin);

	if (flameGun)
	{
		vec3_t fDir;
		if (self->acebot.flame_frameNum < level.framenum)
		{
			self->acebot.flame_frameNum = level.framenum + 20;
			ACEMV_Attack_CalcRandDir(self, fDir);
		}

		VectorSubtract(self->acebot.enemyOrigin, self->s.origin, self->acebot.move_vector);
		ACEMV_Attack_AimRandom_offset(self, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
	}
	else
	{
		// Set viewport Aim direction
		VectorSubtract(target, self->s.origin, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);
	}

	//setup dodge
	ACEMV_Attack_Dodge(self, ucmd); //hypov8 moved down here

	//set aim dir, compensate for movement. todo not needed anymore??
	{
		float fwd;
		float side;
		vec3_t dist;
		vec3_t forward3, right3;
		vec3_t offset3, botMoveComp; //compensate for movement error
		vec3_t angles3;

		if (!self->acebot.isMovingUpPushed)
		{
			fwd = (float)ucmd->forwardmove / 11;
			side = (float)ucmd->sidemove / 11; //10 fps

			VectorSet(offset3, fwd, side, 0);
			VectorCopy(self->s.angles, angles3);
			AngleVectors(angles3, forward3, right3, NULL);
			G_ProjectSource(self->s.origin, offset3, forward3, right3, botMoveComp);
		}
		else
		{	// use momentum on jump pad
			VectorSubtract(self->s.origin, self->acebot.oldOrigin, dist);
			VectorMA(self->acebot.oldOrigin, 2.0f, dist, botMoveComp);
		}

		// Set Aim direction
		if (!flameGun)
		{
			VectorSubtract(target, botMoveComp, self->acebot.move_vector);
			vectoangles(self->acebot.move_vector, angles);
			VectorCopy(angles, self->s.angles);
		}
	}

	//hypov8 move player up if it has a target in water
	VectorCopy(self->s.origin, player_origin);
	player_origin[2] += 8; //hypov8 was 24
	if (gi.pointcontents(player_origin) & MASK_WATER)
	{
		ucmd->upmove = BOT_JUMP_VEL;
	}
	//else
		//self->client->next_drown_time = 0; // probably shound not be messing with this, but

	//hypov8 stop bot movement if in the air
	if (!self->groundentity)
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
	}

	if (self->client->hookstate == 0) //if not on hook
	{
		if (self->acebot.isChasingEnemy)
		{
			//add hypov8. try to stop bots being jerky after target is dead
			ACEAI_Reset_Goal_Node(self, 1.0, "Found Enemy");
		}
		else
		{
			self->acebot.node_goal = INVALID;
			//self->acebot.node_next = INVALID; //hypov8 why??
			self->acebot.state = BOTSTATE_WANDER;
			self->acebot.wander_timeout = level.time + 1;
		}
	}

	self->acebot.isChasingEnemy = false;

}
