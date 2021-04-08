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
//
//  acebot_ai.c -      This file contains all of the 
//                     AI routines for the ACE II bot.
//
//
// NOTE: I went back and pulled out most of the brains from
//       a number of these functions. They can be expanded on 
//       to provide a "higher" level of AI. 
////////////////////////////////////////////////////////////////////////

#include "../g_local.h" //DIR_SLASH
#include "../m_player.h" //DIR_SLASH

#include "acebot.h"


///////////////////////////////////////////////////////////////////////
// Hold fire with RL/BFG?
//hypov8 match ACEAI_VisibleEnemy(). except for glass/fence
///////////////////////////////////////////////////////////////////////
static qboolean ACEAI_CheckNonLeadShot(edict_t *self, qboolean isRocket)
{
	trace_t tr_upper, tr_lower;
	vec3_t	eyes;
	vec3_t	body, legs;
	vec3_t	min = {-8,-8,-8};
	vec3_t	max = { 8, 8, 8};

	if (!self->enemy)
		return false; //double check

	//self->acebot.aimLegs = 0;

	VectorCopy(self->s.origin, eyes);	// bots eyes
	eyes[2] += self->viewheight;

	VectorCopy(self->enemy->s.origin, body);	// enemy body
	body[2] += self->enemy->viewheight; //36 (60 hight)

	VectorCopy(self->enemy->s.origin, legs);	// enemy legs
	legs[2] -= 8; //16 above floor

	tr_upper = gi.trace(eyes, min, max, body, self, MASK_BOT_ATTACK_NONLEAD);
	tr_lower = gi.trace(eyes, min, max, legs, self, MASK_BOT_ATTACK_NONLEAD);


	//check rocket(lower) first, then upper, then lower as last resort
	if (tr_lower.fraction == 1.0 || tr_upper.fraction == 1.0)
	{
		/*if (isRocket && tr_lower.fraction == 1.0)
			self->acebot.aimLegs = 1;
		else if (tr_upper.fraction != 1.0 && tr_lower.fraction == 1.0)
			self->acebot.aimLegs = 1;*/

		return true;
	}
	return false;
}

//setup rand order for weps
static char* ACEAI_Randwep(int index, int type)
{
	switch (index)
	{
	case 0:
		{
			switch (type)
			{
			case 1: return "Heavy machinegun";
			case 2: return "Bazooka";
			case 3: return "Grenade Launcher";
			case 4: return "FlameThrower";
			default:
			case 5: return "Tommygun";
			}
		}
	case 1:
		{
			switch (type)
			{
			case 1: return "Bazooka";
			case 2: return "Heavy machinegun";
			case 3: return "Tommygun";
			case 4: return "Grenade Launcher";
			default:
			case 5: return "FlameThrower";
			}
		}
	default:
	case 2:
		{
			switch (type)
			{
			case 1: return "Tommygun";
			case 2: return "Heavy machinegun";
			case 3: return "Bazooka";
			case 4: return "Grenade Launcher";
			default:
			case 5: return "FlameThrower";
			}
		}
	}


}


///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (simplified)
///////////////////////////////////////////////////////////////////////
static void ACEAI_ChooseWeapon(edict_t *self)
{
	float range;

	// if no enemy, then what are we doing here?
	if (!self->enemy)
		return;

	// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
		return;
	// END

	// Base selection on distance.
	range = VectorDistance(self->s.origin, self->enemy->s.origin);

	switch (self->acebot.randomWeapon)
	{
	case 0:
	{
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,1))))
			return;
		//RL.
		if (ACEAI_CheckNonLeadShot(self, true)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,2))))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,3))))
				return;
		//Flamer.
		if (range < 720)// Flamethrower.
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,4))))
				return;
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,5))))
			return;
	}
	break;

	case 1:
	{
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckNonLeadShot(self, true)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,1))))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,2))))
			return;
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,3))))
			return;

		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,4))))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,5))))
				return;
	}
	break;

	case 2:
	{
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,1))))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,2))))
			return;
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,3))))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,4))))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,5))))
				return;
	}
	break;
	}//end switch

	if (ACEIT_ChangeWeapon(self, FindItem("Shotgun")))
		return;
	if (ACEIT_ChangeWeapon(self, FindItem("SPistol")))
		return;

	if (ACEIT_ChangeWeapon(self, FindItem("Pistol")))
		return;

	if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
		&& ACEIT_ChangeWeapon(self, FindItem("Crowbar")))
		return;

	if (/*ACEAI_CheckNonLeadShot(self)  //check if fence infront
		&& */ACEIT_ChangeWeapon(self, FindItem("Pipe"))) //hypo last resort no checks
		return;

	return;

}

// BEGIN HITMEN
///////////////////////////////////////////////////////////////////////
// Choose weapon for hitmen
// used when ammo runs out then bot selects crow bar
// will select gun when ammo topes back up
///////////////////////////////////////////////////////////////////////
static void ACEAI_ChooseWeaponHM(edict_t *self)
{

	if (self->client->pers.weapon != FindItem("Crowbar"))
		return;

	switch (game.Current_Weapon)
	{
	default:
	case 1: // "pistol"
		ACEIT_ChangeWeapon(self, FindItem("Pistol"));			break;
	case 2: // "shotgun"
		ACEIT_ChangeWeapon(self, FindItem("Shotgun"));			break;
	case 3: // "tommygun"
		ACEIT_ChangeWeapon(self, FindItem("Tommygun"));			break;
	case 4: // "Heavy machinegun"
		ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")); break;
	case 5: // "Grenade Launcher"
		ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher"));	break;
	case 6: // "Bazooka"
		ACEIT_ChangeWeapon(self, FindItem("Bazooka")); break;
	case 7: // "FlameThrower"
		ACEIT_ChangeWeapon(self, FindItem("FlameThrower"));		break;
	}
}

// END


/*
=============
infrontBot
hypov8
returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
qboolean ACEAI_InfrontBot(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	//look in 360 deg if pistol
	if (!(int)sv_hitmen->value 
		&& 	(self->acebot.num_weps <= 2 || (self->client->pers.weapon && !strcmp(self->client->pers.weapon->classname, "weapon_pistol")) ) 
		)
	{
		if (strncmp(other->classname, "weapon_", 7) == 0)
			return true;
	}


	//hypov8 make safe/cash look in 360 deg
	if (strcmp(other->classname, "item_cashroll") == 0
		|| strcmp(other->classname, "item_cashbagsmall") == 0
		|| strcmp(other->classname, "dm_safebag") == 0
		|| strcmp(other->classname, "trigger_push") == 0
		)
		return true;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.25)
		return true;
	return false;
}


///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
static void ACEAI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float weight, best_weight = 0.0;
	edict_t *best = NULL;
	int index;
	int lookDist = 200;

	//hypov8 add. stop bots trying to get SRG if on ladder
	if (self->acebot.isOnLadder)	
	{
		self->acebot.SRGoal_onLadder = level.framenum + 10;
		self->movetarget = NULL;
		return;
	}
	if (self->acebot.SRGoal_onLadder > level.framenum)
		return;

	if (self->groundentity == NULL && !self->acebot.isJumpToCrate)
		return;

	//hypov8 todo: timeout on node to

	//look further if we just spawned or attacking
	if (!(int)sv_hitmen->value && 
		(self->acebot.num_weps <= 2|| (self->client->pers.weapon && !strcmp(self->client->pers.weapon->classname, "weapon_pistol"))))
	{
		if (self->acebot.SRGoal_frameNum < level.framenum)
			lookDist = 512;
		if (!(level.framenum % 30))
			self->acebot.SRGoal_frameNum = level.framenum + 30;
	}

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, lookDist);

	while (target)
	{
		vec3_t itemOrigin, v1, v2;
		qboolean isRocket=false;
		int index_1 = ITEM_INDEX(target->item);

		if (target->classname == NULL)
			return;

	
		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if (strcmp(target->classname, "rocket") == 0 && target->owner != self)	
		{
			if (ACEAI_InfrontBot(self, target))
			{
				self->movetarget = target;//hypov8 todo: timeout loop
				return;
			}
			isRocket = true;
		}
		if (strcmp(target->classname, "grenade") == 0)			
		{
			self->movetarget = target;//hypov8 todo: timeout loop
			return;
		}

		if (!isRocket)
		{
			VectorCopy(target->s.origin, itemOrigin);

			//special. trig push dont have origin
			if (strcmp(target->classname, "trigger_push") == 0)
			{
				VectorCopy(target->maxs, v1);
				VectorCopy(target->mins, v2);
				itemOrigin[0] = (v1[0] - v2[0]) / 2 + v2[0];
				itemOrigin[1] = (v1[1] - v2[1]) / 2 + v2[1];
				itemOrigin[2] = (v1[2] - v2[2]) / 2 + v2[2];
			}

			if (ACEIT_IsReachable(self, itemOrigin))
			{
				if (ACEAI_InfrontBot(self, target)) //look 360 deg for weapons
				{
					float dist = VectorDistance(target->s.origin, self->s.origin);
					index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
					weight = ACEIT_ItemNeed(self, index, target->timestamp, target->spawnflags);

					weight /= dist;//closer??

					if (weight > best_weight)
					{
						best_weight = weight;
						best = target;
					}
				}
			}
		}
		// next target
		target = findradius(target, self->s.origin, lookDist); //true=bot
	}

	if (best_weight)
	{
		self->movetarget = best;

		if (level.bot_debug_mode == 1 && self->goalentity != self->movetarget) //add hypo stop console nag when localnode is on )
			debug_printf("%s selected a %s for SR goal.\n", self->client->pers.netname, self->movetarget->classname);
		
		self->goalentity = best;
	}

}



///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_PickShortRangeGoalSpawned(edict_t *self)
{
	edict_t *target;
	float weight, best_weight = 0.0;
	edict_t *best = NULL;
	int index;
	qboolean goal = false;
	int localWeaponDist = 416;

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, localWeaponDist);

	while (target)
	{
		if (target->classname == NULL)
			return goal;

		if (ACEIT_IsReachable(self, target->s.origin) && target->solid != SOLID_NOT)
		{		
			if (strncmp(target->classname, "w", 1) == 0)
			{
				index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeedSpawned(self, index, target->timestamp, target->spawnflags);

				if (weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
			
		}

		// next target
		target = findradius(target, self->s.origin, localWeaponDist); //true=bot
	}

	if (best_weight)
	{
		self->movetarget = best;

		if (level.bot_debug_mode == 1 && self->goalentity != self->movetarget) //add hypo stop console nag when localnode is on )
			debug_printf("%s selected a %s for SR Spawn goal.\n", self->client->pers.netname, self->movetarget->classname);

		self->goalentity = best;
		goal = true;
	}
	return goal;
}


//hypov8 add
//count if we picked up a new wep. should we change to it?
static qboolean ACEAI_WeaponCount(edict_t *self)
{
	int			i;
	gitem_t		*it;
	int wepCount = 0;

	for (i = 1; i <= game.num_items /*MAX_ITEMS*/; i++) //hypov8 last wep is item #16, change if we get more weps 
	{
		if (!self->client->pers.inventory[i])
			continue;

		it = &itemlist[i];
		if (!it->use)
			continue;

		if (!(it->flags & IT_WEAPON))
			continue;

		wepCount++;
	}

	if (wepCount > self->acebot.num_weps)
	{
		self->acebot.num_weps = wepCount;
		return true;
	}
	return false;
}

 //recheck keeps looking 4 same player once node reached
qboolean ACEAI_PickShortRangeGoal_Player (edict_t *self, qboolean reCheck)
{
	if (self->acebot.state != BOTSTATE_WANDER && !self->acebot.isChasingEnemy)
		return false;
	if (self->client->hookstate)
		return false;
	if (self->acebot.isMovingUpPushed)
		return false;
	// enemy not valid yet(skill time)
	if (self->acebot.botSkillDeleyTimer > level.time)
		return false;
	if (!self->acebot.enemyID)
		return false;
	//no longer intrested in enemy
	if (self->acebot.enemyAddFrame <= level.framenum - 150)
		return false;

	//check our enemy. again?
	if (reCheck || self->acebot.enemyChaseFrame <= level.framenum)
	{
		if (random() > 0.5/*&&self->acebot.isChasingEnemy*/)
		{
			self->acebot.isChasingEnemy = false;
			self->acebot.enemyID = -1;
			self->acebot.enemyAddFrame = 0;
			ACEAI_PickLongRangeGoal(self);
			return false;
		}
		else
		{
			int i;
			short int node; //hypo
			float best_weight = 0.0;
			short current_node, goal_node = INVALID;
			self->acebot.targetPlayerNode = 0;

			for (i = 1; i <= (int)maxclients->value; i++)
			{
				edict_t *players = g_edicts + i;
				if (!players->acebot.is_validTarget)
					continue;

				if (players == self
					|| players->solid == SOLID_NOT
					|| players->movetype == MOVETYPE_NOCLIP
					|| players->flags & FL_GODMODE
					|| players->client->invincible_framenum > level.framenum
					|| (teamplay->value && players->client->pers.team == self->client->pers.team))
					continue;

				if (i != self->acebot.enemyID)
					continue;

				node = ACEND_FindClosestNode(players, BOTNODE_DENSITY, BOTNODE_ALL);
				if (node == INVALID)
				{
					ACEAI_Reset_Goal_Node(self, 1.0, "No close node for SRG (Player).");
					return false;
				}

				current_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);
				if (current_node == INVALID)
				{
					ACEAI_Reset_Goal_Node(self, 1.0, "No close node for SRG (Bot).");
					return false;
				}

				// OK, everything valid, let's start moving to our goal.
				self->acebot.targetPlayerNode = level.framenum + 50; //add hypov8 +3 sec timeout
				self->acebot.node_current = current_node;
				self->acebot.node_next = self->acebot.node_current; // make sure we get to the nearest node first
				self->acebot.node_timeout = 0;
				self->acebot.state = BOTSTATE_MOVE;
				self->acebot.node_tries = 0; // Reset the count of how many times we tried this goal
				if (!self->acebot.isChasingEnemy)
					self->acebot.enemyChaseFrame = level.framenum + 80;
				self->acebot.isChasingEnemy = true;
				self->acebot.node_goal = node;

				if (players != NULL && level.bot_debug_mode == 1) //add hypo stop console nag when localnode is on )
					debug_printf("%s selected a %s at node %d for SRG (Player).\n", self->client->pers.netname, players->classname, node);

				return true;
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickLongRangeGoal(edict_t *self)
{

	int i;
	short int node; //hypo
	float weight,best_weight=0.0;
	short current_node, goal_node = INVALID;
	edict_t *goal_ent = NULL;
	edict_t*players;
	int cost;


	// look for a target 
	current_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_ALL);

	self->acebot.node_current = current_node;
	
	if(current_node == INVALID)
	{
		ACEAI_Reset_Goal_Node(self, 1.0, "No close node for LRG.");
		return;
	}

	///////////////////////////////////////////////////////
	// Items
	///////////////////////////////////////////////////////
	for(i=0;i<num_items;i++)
	{
		if(item_table[i].ent == NULL) // ignore items that are not there.
			continue;
		if (!item_table[i].ent->solid) // ignore items that are not there.
			continue;
		if (item_table[i].ent->solid == SOLID_NOT)
			continue;

		switch (nodes[item_table[i].node].type)
		{
			case BOTNODE_PLATFORM:
			case BOTNODE_LADDER:
			case BOTNODE_TELEPORTER:
			case BOTNODE_WATER:
			case BOTNODE_GRAPPLE:
			case BOTNODE_JUMP:
			continue;
		}

		weight = ACEIT_ItemNeed(self, item_table[i].item, 0.0, 0);
		if (weight <= 0.0) 
			continue;

		cost = ACEND_FindCost(current_node, item_table[i].node);
		if (cost == INVALID ||cost < 2) // ignore invalid and very short hops 
			continue;
	

		weight *= random(); // Allow random variations
		weight /= (float)cost; // Check against cost of getting there
				
		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = item_table[i].node;
			goal_ent = item_table[i].ent;
		}
	}

	self->acebot.targetPlayerNode = 0; //add hypov8
	///////////////////////////////////////////////////////
	// Players
	///////////////////////////////////////////////////////
	// This should be its own function and is for now just
	// finds a player to set as the goal.
	for (i = 1; i <= (int)maxclients->value; i++)
	{
		players = g_edicts + i;
		if (!players->acebot.is_validTarget)
			continue;

		if (players == self
			|| players->solid == SOLID_NOT
			|| players->movetype == MOVETYPE_NOCLIP
			|| players->flags & FL_GODMODE
			|| players->client->invincible_framenum > level.framenum
			|| (teamplay->value && players->client->pers.team == self->client->pers.team))
			continue;
		//node = ACEND_FindClosestReachableNode(players[i],BOTNODE_DENSITY,BOTNODE_ALL);
		node = ACEND_FindClosestNode(players,BOTNODE_DENSITY,BOTNODE_ALL);
		if (node == INVALID) ////hypo added to stop below returning crapola 
			continue;

		cost = ACEND_FindCost(current_node, node);
		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		// BEGIN HITMEN
		if (sv_hitmen->value /*enable_hitmen*/)
		{
			weight = 2.5;
			if (players->acebot.is_hunted){
				cost /= 2; //perswade some more
				weight = 6; //lets go hunting
			}
		}
		else
		{
			weight = 0.3;
			if (players->acebot.is_hunted && self->health>80 && self->acebot.num_weps > 3){
				cost /= 2; //perswade some more
				weight = 6; //lets go hunting
			}
		}
		
		weight *= random(); // Allow random variations
		weight /= (float)cost; // Check against cost of getting there
		
		if(weight > best_weight)
		{	
			self->acebot.targetPlayerNode = level.framenum + 100; //add hypov8 +10 sec timeout
			best_weight = weight;
			goal_node = node;
			goal_ent = players;
		}	
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == INVALID)
	{
		ACEAI_Reset_Goal_Node(self, 1.0, "Did not find a LR goal");
		return; // no path? 
	}
	
	// OK, everything valid, let's start moving to our goal.
	self->acebot.state = BOTSTATE_MOVE;
	self->acebot.node_tries = 0; // Reset the count of how many times we tried this goal
	 
	if (goal_ent != NULL && level.bot_debug_mode == 1) //add hypo stop console nag when localnode is on )
		if (goal_ent->client)
			debug_printf("%s selected %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->client->pers.netname, goal_node);
		else
			debug_printf("%s selected a %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node, goal_ent);

}

//hypov8 add
//use eyesight to check pvs
static qboolean ACEAI_VisibleEnemyPVS(edict_t *self, edict_t *other)
{
	vec3_t botsSight, enemySight, enemyLegs;
	VectorCopy(self->s.origin, botsSight);
	VectorCopy(other->s.origin, enemySight);
	VectorCopy(other->s.origin, enemyLegs);
	botsSight[2] += self->viewheight;
	enemySight[2] += other->viewheight;

	if (!gi.inPVS(botsSight, enemySight) && !gi.inPVS(botsSight, enemyLegs))
		return false;

	return true;
}

//hypov8 add
/*
=============
ACEAI_VisibleEnemy

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
static qboolean ACEAI_VisibleEnemy(edict_t *self, edict_t *other, qboolean isBestplayer)
{
	trace_t tr_upper, tr_lower;
	vec3_t	eyes;
	vec3_t	body, legs;
	vec3_t	bbmin = { -8, -8, 0 }; //allow for rocket thickness
	vec3_t	bbmax = { 8, 8, 0 };
	int mask = MASK_BOT_ATTACK_NONLEAD;
	qboolean isBooz=false,isBullet=true, isCrowbar= false;

	self->acebot.aimLegs = 0;
	self->acebot.enemyOriginZoffset = 0;

	if (self->client->pers.weapon)
	{
		if (Q_stricmp(self->client->pers.weapon->classname, "weapon_bazooka") == 0)
		{
			isBooz = true;
			isBullet = false;
		}
		else if (Q_stricmp(self->client->pers.weapon->classname, "weapon_grenadelauncher") == 0) //hypov8 todo: flammer ok?
		{
			isBullet = false;
		}
		else if (Q_stricmp(self->client->pers.weapon->classname, "weapon_crowbar") == 0
				|| Q_stricmp(self->client->pers.weapon->classname, "Pipe") == 0)
		{
			isBullet = false;
			isCrowbar = true;
		}
	}

	//used for top player using bullets. more accurate between gaps

	if (isBullet)
	{
		mask = MASK_BOT_ATTACK_LEAD;
		if (isBestplayer)
		{
			VectorCopy(vec3_origin, bbmin);
			VectorCopy(vec3_origin, bbmax);
		}
		else if (self->acebot.botSkillCalculated < 3)
		{
			VectorSet(bbmin, -16, -16, 0);
			VectorSet(bbmax, 16, 16, 0);
		}
	}
	else if (isCrowbar)//only target close players	
	{
		vec3_t v;
		float leng;
		VectorSubtract(self->s.origin, other->s.origin, v);
		leng = VectorLength(v);
		//to far
		if (leng > 256)
			return false;
		else
			leng = 0;
	}
	else if (!isBooz)
	{	//rl
		vec3_t v;
		float leng;
		VectorSubtract(self->s.origin, other->s.origin, v);
		leng = VectorLength(v);
		//to far
		if (leng > 850)
			return false; 
	}

	VectorCopy(self->s.origin, eyes);	// bots eyes
	eyes[2] += self->viewheight;

	VectorCopy(other->s.origin, body);	// enemy body
	body[2] += other->viewheight; //36 (60 hight)

	VectorCopy(other->s.origin, legs);	// enemy legs
	legs[2] -= 8; //16 above floor

	//hypov8 trace solid objects. trace fence etc when chosing wep
	tr_upper = gi.trace(eyes, bbmin, bbmax, body, self, mask); //hypov8 todo: check this: fix non trans aux
	tr_lower = gi.trace(eyes, bbmin, bbmax, legs, self, mask);  /*MASK_OPAQUE MASK_BOT_SOLID_FENCE*/

	//check rocket(lower) first, then upper, then lower as last resort
	if (self->client->pers.weapon && (tr_upper.fraction == 1.0 || tr_lower.fraction == 1.0))
	{
		if (tr_lower.fraction == 1.0 && isBooz)
			self->acebot.aimLegs = 1;
		else if (tr_upper.fraction != 1.0 && tr_lower.fraction == 1.0)
			self->acebot.aimLegs = 1;

		return true;
	}

	return false;
}


//skill multiplier
static float ACEAI_SkillMP(float skillMulti, float CvarSkill)
{
	float skill = CvarSkill * skillMulti;

	if (skill < 0.0f) 		skill = 0.0f;
	else if (skill >4.0f)	skill = 4.0f;

	return skill;
}

static void ACEAI_CalculatePlayerState()
{
	int		i, j, k, count;
	int		score, total;
	int		botRankPlayerNum[MAX_CLIENTS];
	int		botRankScores[MAX_CLIENTS];
	float skill = sv_botskill->value;
	edict_t	*players;

	num_players = 0;
	num_bots = 0;


	total = 0;
	for (i = 1; i <= (int)maxclients->value; i++)
	{
		players = g_edicts + i;

		//reset values every frame
		players->acebot.is_validTarget = false;
		players->acebot.is_hunted = false;


		if (!players->inuse
			|| players->flags & FL_GODMODE
			|| players->client == NULL
			|| players->client->pers.spectator == SPECTATING)
			continue;

		if (players->acebot.is_bot)
		{
			num_bots++; //allow calculation at vote
			players->acebot.botSkillCalculated = ACEAI_SkillMP(players->acebot.botSkillMultiplier, skill);
		}

		//make sure they are in game and ready to die
		if (players->solid == SOLID_NOT 
			|| players->movetype == MOVETYPE_NOCLIP)
			continue;

		if (!players->acebot.is_bot)		
			num_players++;


		if (players->client->invincible_framenum > level.framenum)	{
			players->acebot.pm_last_node = INVALID;	//is_validTarget = false;
		}
		else // route/attack this player		
			players->acebot.is_validTarget = true;	

		// sort the clients by score
		score = players->client->resp.score;
		for (j = 0; j<total; j++)
		{
			if (score > botRankScores[j])
				break;
		}
		for (k = total; k>j; k--)
		{
			botRankPlayerNum[k] = botRankPlayerNum[k - 1];
			botRankScores[k] = botRankScores[k - 1];
		}
		botRankPlayerNum[j] = i;
		botRankScores[j] = score;
		total++;
	}

	//set hunted
	count = num_bots + num_players;
	if (total >1 && count >1)
	{
		score = botRankScores[0] - botRankScores[1];
		if (score > 3 && !g_edicts[botRankPlayerNum[0]].acebot.is_bot)
			(g_edicts + botRankPlayerNum[0])->acebot.is_hunted = true;
	}
}
#if HYPOBOTS
static qboolean ACEAI_PickOnBestPlayer(int playerNum)
{
	int tmpScore;
	if (playerNum == botRankPlayerNum[0])
	{
		tmpScore = botRankScores[0] - botRankScores[1];
		if (tmpScore > 3)
		{
			if (level.bot_debug_mode && level.framenum %10 == 0)
				debug_printf("    ******Best Player****** Targeted by bots.\n");
	
			return true;
		}
	}
	return false;
}
#endif


void ACEAI_G_RunFrame(void)
{
	int i, foundFirstPlyr = false;
	edict_t *ent;

	if (sv_botskill->value < 0.0f) 
		gi.cvar_set("sv_botskill", "0.0");
	else if (sv_botskill->value > 4.0f)
		gi.cvar_set("sv_botskill", "4.0");

	ACEAI_CalculatePlayerState(); //set hunted, player/bot count etc..

	if (level.bots_spawned && (level.modeset == MATCH || level.modeset == PUBLIC))
	{
		ent = &g_edicts[0];
		ent++;
		for (i = 1; i <= (int)maxclients->value; i++, ent++)
		{
			//ent = g_edicts + i;

			if (ent->acebot.is_bot)
				ACEAI_Think(ent);
			else
			{
				ent->acebot.PM_firstPlayer = false;
				if (!foundFirstPlyr) //auto route on player 1
				{
					if (ACEND_PathMapValidPlayer(ent))
					{
						if ((int)teamplay->value != 1) //route all players in teamplay
							foundFirstPlyr = true;
						ent->acebot.PM_firstPlayer = true;
						ACEND_PathMap(ent, false); //run every 100ms. jump is caught in p_client
					}
					else
						ent->acebot.pm_last_node = INVALID;
				}
				else
					ent->acebot.pm_last_node = INVALID;
			}
		}
	}
}


/*
=============
infrontEnem

returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg(180 left/right)
=============
*/
static qboolean ACEAI_InfrontEnemy(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot, fov, skill = self->acebot.botSkillCalculated;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);
	//hypov8
	fov = .25;
	if (skill>3.0f)
		fov = (4 - skill) - 1;	//-1 to 0.0 
	//1.0 is dead ahead
	if (dot > fov)
		return true;
	return false;
}


// BEGIN HITMEN
/*
=============
ACEAI_Enemy_ToFarHM

return true if enamy is to far away
only used for hitmen
bots cant change to a suitable weapon
=============
*/
static qboolean ACEAI_Enemy_ToFarHM(edict_t *self,edict_t *enemy )
{
	//vec3_t v;
	float range;
	if (!sv_hitmen->value /*enable_hitmen*/)
		return false;


	//if (self->client->pers.weapon == FindItem("Crowbar"))
		//return;

	//distance.
	range = VectorDistance(self->s.origin, enemy->s.origin);

	// Crowbar
	if (!strcmp(self->client->pers.weapon->pickup_name, "Crowbar"))
		if (range > 48)	
			return true;
	// "Grenade Launcher"
	if (game.Current_Weapon == 5)
		if (range < 50 || range >720)	
			return true;
	// "FlameThrower"
	if (game.Current_Weapon == 7)			
		if (range < 25|| range >980)	
			return true;	

	return false;

}

///////////////////////////////////////////////////////////////////////
// Scan for enemy (simplifed for now to just pick any visible enemy)
///////////////////////////////////////////////////////////////////////
static qboolean ACEAI_FindEnemy(edict_t *self)
{
	int i;
	int	j = -1;
	float range, range_tmp = 0;
	qboolean underAttack = false;

#if 0// HYPODEBUG
	if (self->s.number == 25)
		return false;

#endif

	//reloading?
	if (self->client->weaponstate != WEAPON_READY && self->client->weaponstate != WEAPON_FIRING)
		return false;
	//hypo stop bot attacking on ladders
	if (self->acebot.isOnLadder)//  == 1) //allow attackt at top of ladder??
		return false;
	//hypo add timer for bot to not own so much
	if (self->acebot.botSkillDeleyTimer > level.time)
		return false;
	//dead!!
	if (self->health < 1)
		return false;

	//hypov8 shoot enemy faster if being attacked
	if (self->acebot.lastDamageTimer >= (level.framenum - 40)) //4 seconds. allow for skill
		underAttack = true;


	 for (i = 1; i <= (int) maxclients->value; i++) //num_players
	 {
		 edict_t *players;
		 players = g_edicts + i;

		 if (!players->acebot.is_validTarget)
			 continue;

		 //reset to no players stop bot hunting same player after death
		 if (self->acebot.enemyID == i && players->health < 1)		{
			 self->acebot.enemyID = -1;
			 self->enemy = NULL;
			 continue;
		 }

		 if (players->deadflag)
			 continue;
		 if (players->health < 1)
			 continue;

		 if (players == self
			 || players->solid == SOLID_NOT
			 || players->movetype == MOVETYPE_NOCLIP
			 || players->flags & FL_GODMODE //addhypov8
			 || players->client->invincible_framenum > level.framenum
			 || (teamplay->value && self->client->pers.team == players->client->pers.team) //on same team
			 || ACEAI_Enemy_ToFarHM(self, players)) // BEGIN HITMEN
			 continue;

		// player is in direct sight. no solid walls
		 if (!players->deadflag && ACEAI_VisibleEnemyPVS(self, players))
		{
			qboolean visable = ACEAI_VisibleEnemy(self, players, players->acebot.is_hunted);

			if ((int)sv_bot_hunt->value && players->acebot.is_bot == 0 	
				&& players->acebot.is_hunted //hypov8 add. stop top player with force!! :)
				&& visable)
			{
				self->enemy = players;
				self->acebot.botSkillDeleyTimer = 0;
				return true;
			}

			//line of sight?
			if (!visable) //HYPODEBUG.. enable true to test
				continue;
			
			//just keep shooting last target for now
			if (self->acebot.enemyID == i)
			{
				self->enemy = players;
				return true;
			}

			//hypo dont add a new target if not infront. ignore if hunted or under attack
			//this stops bot turning a full 360. using skill %))
			if (!players->acebot.is_hunted && !underAttack && !ACEAI_InfrontEnemy(self, players))
				continue;

			// Base selection on distance.	
			range = VectorDistance(self->s.origin, players->s.origin);
			if (players->acebot.is_hunted)
				range /= 2;
			
			if (range_tmp)
			{
				if (range < range_tmp)
					j = i; //j is new closer player
			}
			else
			{
				range_tmp = range;
				j = i;
			}
		}
	}

	// set a new target
	if (j != -1)
	{
		self->acebot.enemyID = j;
		self->acebot.enemyAddFrame = level.framenum;

		if (self->client->weaponstate == WEAPON_FIRING)
			self->acebot.botSkillDeleyTimer = level.time + ((4.0f - self->acebot.botSkillCalculated) *0.35f); //hypov8 target changed. shoot next closer player sooner
		else if (underAttack)
			self->acebot.botSkillDeleyTimer = level.time + ((4.0f - self->acebot.botSkillCalculated) *0.5f); //hypov8 attacked. shoot faster. 2 sec max
		else
			self->acebot.botSkillDeleyTimer = level.time + ((4.0f - self->acebot.botSkillCalculated) *0.75f); //give player 3 seconds(default) leway between seeing n being shot

		return false;

	}


	return false; //nothing!!!
  
}



///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (hypo)
///////////////////////////////////////////////////////////////////////
static void ACEAI_PreChooseWeaponDM(edict_t *self)
{	
	switch (self->acebot.randomWeapon)
	{
	case 0:
		{
			// always favor the HMG
			if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,1))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,2))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,3))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,4))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(0,5))))
				return;
		}
		break;
	case 1:
		{
			// always favor the boozooka
			if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,1))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,2))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,3))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,4))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(1,5))))
				return;
		}
		break;
	case 2:
	default:
		{
			// always favor the tommy
			if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,1))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,2))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,3))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,4))))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem(ACEAI_Randwep(2,5))))
				return;
		}
		break;
	} //end switch

	//no weps so chose shotty?
	if (ACEIT_ChangeWeapon(self, FindItem("Shotgun")))
		return;

}

// select a weapon b4 we find enamy
static void ACEAI_PreChooseWeapon(edict_t *self)
{
	if (sv_hitmen->value) /*enable_hitmen*/
		ACEAI_ChooseWeaponHM(self);//select wep if it has ammo
	else if (ACEAI_WeaponCount(self))
		ACEAI_PreChooseWeaponDM(self);
}


//add hypov8
//found an enemy, forget out goal
void ACEAI_Reset_Goal_Node(edict_t *self, float wanderTime, char* eventName)
{
	//if (self->acebot.node_goal != INVALID)
	{
		self->acebot.node_ent = NULL;
		self->acebot.node_next = INVALID;
		self->acebot.node_goal = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + wanderTime;
		if (eventName[0])
		{
			if (level.bot_debug_mode == 1) //add hypo stop console nag when localnode is on )
				debug_printf("%s Wandering: %s\n", self->client->pers.netname, eventName);
		}
	}
}


#if HYPODEBUG
//#define BOT_TIME_DIFF(base,diff) (diff=Sys_Milliseconds()-base;if(diff>1)gi.dprintf("timediff LRG=%d.\n",diff);)
#define BOT_TIME_DIFF(base,diff,str,mode)\
			diff=Sys_Milliseconds()-base;\
			if(diff>1)\
				gi.dprintf("Bot SyncDelay At %s (%dms) MoveMode: %s\n",str,diff, (mode==1)?"MOVE": "WANDER");
#else
#define BOT_TIME_DIFF(a,b,c,d)(b=1)
#endif

///////////////////////////////////////////////////////////////////////
// Main Think function for bot
///////////////////////////////////////////////////////////////////////
void ACEAI_Think(edict_t *self)
{
	usercmd_t	ucmd;
	float		tmpTimeout;
	int base = curtime;
	int timediff;

#if HYPODEBUG
	if (!gi.inPVS(self->s.origin, self->s.origin))
		gi.dprintf("Bot %s In Void at XYZ:(%f, %f, %f)\n",self->client->pers.netname, self->s.origin[0], self->s.origin[1], self->s.origin[2]);
#endif
	////////////////////
	// reset some values
	////////////////////
	VectorCopy(self->s.angles, self->acebot.deathAngles);
	VectorCopy(self->client->ps.viewangles, self->s.angles);
	VectorSet(self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset(&ucmd, 0, sizeof(ucmd));
	self->movetarget = NULL;
	self->client->resp.idle = level.framenum; //hypov8 just incase
	//todo check if needed??
	self->client->pers.idle = curtime;
	self->client->pers.lastpacket = curtime;

	if (self->acebot.isMovingUpPushed)	{ 
		self->acebot.next_move_time = level.time + 0.1f; //stops wander
		if (self->groundentity && self->acebot.trigPushTimer < level.framenum){ //allow some time for lunch	
			self->acebot.isMovingUpPushed = false;
			self->acebot.wander_timeout = level.time -0.1f;
		}
	}
	if (self->acebot.isJumpToCrate)	{
		if (level.framenum > self->acebot.crate_time || self->groundentity)
			self->acebot.isJumpToCrate = false;
	}
	if (self->acebot.is_Jumping)
	{
		if (self->groundentity)
			self->acebot.is_Jumping = false;
	}


	if (self->acebot.water_time >= level.framenum){
		if (!self->groundentity){
			ucmd.forwardmove = BOT_FORWARD_VEL;
			ucmd.upmove = BOT_JUMP_VEL;
			ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
			ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
			ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);
			ucmd.msec = 100;
			self->client->ping = 0;
			ClientThink(self, &ucmd);
			return;
		}
		else
			self->acebot.water_time = 0;
	}
	if (self->acebot.isOnLadder)
	{
		if (self->acebot.ladder_time < level.framenum || (self->groundentity && !self->groundentity->client))
		{
			self->acebot.ladder_time = 0;
			self->acebot.isOnLadder = false; //hypo stop bot attacking on ladders
		}
	}

	//////////////////////////
	// Force respawn when dead
	//////////////////////////
	if (self->deadflag)	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
		if (self->velocity[2] > 0)
			self->velocity[2] = 0;
		ClientThink(self, &ucmd);
		return;
	}

	/////////////////////////////////////////////
	//stop bot looking for recent enemy that didnt die
	/////////////////////////////////////////////
	if (self->acebot.isChasingEnemy){
		if (self->acebot.enemyChaseFrame < level.framenum){
			ACEAI_Reset_Goal_Node(self, 1.0, "Stoped looking for Enemy.");
			self->acebot.isChasingEnemy = false;
			self->acebot.enemyID = -1;
			self->enemy = NULL;
		}
	}
	if (self->acebot.enemyAddFrame < level.framenum - 150){ //15 seconds, forget about old target
		self->acebot.enemyID = -1;
		self->enemy = NULL;
	}

	/////////////////////////////////////////////
	// pick a new long range goal
	/////////////////////////////////////////////
	if (self->acebot.state == BOTSTATE_WANDER && self->acebot.wander_timeout < level.time
	&& !self->acebot.isMovingUpPushed && !self->acebot.isChasingEnemy)
		ACEAI_PickLongRangeGoal(self);

	BOT_TIME_DIFF(base, timediff, "LRG" , self->acebot.state);

	/////////////////////////////////////////////
	// check if bot is moving?
	/////////////////////////////////////////////
	self->acebot.moveDirVel = VectorDistance(self->acebot.oldOrigin, self->s.origin);
	if (self->acebot.moveDirVel > 2)
		self->acebot.suicide_timeout = level.time + 10.0;

	tmpTimeout = self->acebot.suicide_timeout - level.time;
	if (tmpTimeout < 6 && self->acebot.isOnLadder){
		ACEAI_Reset_Goal_Node(self, 1.0, "Stuck on ladder, Turn 180.");
		self->acebot.isOnLadder = false;
		if (self->s.angles[YAW]>0) 
			self->s.angles[YAW] -= 180.0;
		else					   
			self->s.angles[YAW] += 180.0;

		ucmd.forwardmove = BOT_FORWARD_VEL;
		ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
		ClientThink(self, &ucmd);
		return;
	}

	///////////////////
	// Stuck. Kill bot
	///////////////////
	if (tmpTimeout < 0)
	{
		ACESP_KillBot(self);
		return;
	}

	////////////////////////////
	// Find a short range goal
	////////////////////////////
	if (!self->acebot.isMovingUpPushed && !self->client->hookstate)
	{
		ACEAI_PickShortRangeGoal(self);
		BOT_TIME_DIFF(base, timediff, "SRG" , self->acebot.state);
	}


	////////////////////////////////////////
	// Chose Weapons Before we see an anemy
	///////////////////////////////////////
	ACEAI_PreChooseWeapon(self);


	////////////////////////////////////
	// Find enemies OR items 
	// hypo serch weps if reloading etc.
	////////////////////////////////////
	if (ACEAI_FindEnemy(self))
	{
		//todo BOTSTATE_ATTACK. resume LRG
		BOT_TIME_DIFF(base, timediff, "Enemy" , self->acebot.state);
		ACEAI_ChooseWeapon(self);
		BOT_TIME_DIFF(base, timediff, "Choose Wep" , self->acebot.state);
		ACEMV_Attack(self, &ucmd);
		BOT_TIME_DIFF(base, timediff, "Atack" , self->acebot.state);
	}
	else 		// Execute the move, or wander
	{
		ACEAI_PickShortRangeGoal_Player(self, false); 	// keep chasing enemy?
		BOT_TIME_DIFF(base, timediff, "SRG Player", self->acebot.state);

		if (self->acebot.state == BOTSTATE_WANDER)	{
			ACEMV_Wander(self, &ucmd);
			BOT_TIME_DIFF(base, timediff, "Wander", self->acebot.state);
		}
		else if (self->acebot.state == BOTSTATE_MOVE)	{
			ACEMV_Move(self, &ucmd);
			BOT_TIME_DIFF(base, timediff, "Move", self->acebot.state);
		}
	}

#if 0// HYPODEBUG
	if (self->enemy)
	{
		if (self->s.number == 25)
		{
			ucmd.forwardmove = 0;
			ucmd.sidemove = 0;
		}
	}

#endif


	//////////////////////////////
	// Setup Final player movement
	//////////////////////////////
	ucmd.msec = 100;		//set command interval. 10fps
	self->client->ping = 0; //set ping
	VectorCopy(self->s.origin, self->acebot.oldOrigin);	//hypo set old origin for movement calculations

	// set bot's view angle
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);

	if (self->acebot.isMovingUpPushed) //stop bot moving on jumppad
		ucmd.forwardmove = ucmd.sidemove = ucmd.upmove = 0;

	/////////////////////////////////
	// send command through id's code
	/////////////////////////////////
	ClientThink(self, &ucmd);

	BOT_TIME_DIFF(base, timediff, "END", self->acebot.state);
}
