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

	self->acebot.aimLegs = 0;

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
		if (isRocket && tr_lower.fraction == 1.0)
			self->acebot.aimLegs = 1;
		else if (tr_upper.fraction != 1.0 && tr_lower.fraction == 1.0)
			self->acebot.aimLegs = 1;

		return true;
	}
	return false;
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
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//RL.
		if (ACEAI_CheckNonLeadShot(self, true)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)// Flamethrower.
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;
	}
	break;

	case 1:
	{
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckNonLeadShot(self, true)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;

		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
	}
	break;

	case 2:
	{
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckNonLeadShot(self, false)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
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

	if (dot > -0.25)
		return true;
	return false;
}


static void ACEAI_PickShortRangeGoal_HM(edict_t *self)
{
	edict_t *target;
	float weight, best_weight = 0.0;
	edict_t *best = NULL;
	int index;

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);
	while (target)
	{
		if (target->classname == NULL)
			return;

		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if (strcmp(target->classname, "rocket") == 0)
		{
			if (ACEAI_InfrontBot(self, target)){
				self->movetarget = target;//hypov8 todo: timeout loop
				return;
			}
		}

		if ( strcmp(target->classname, "grenade") == 0)
		{
			self->movetarget = target;//hypov8 todo: timeout loop
			return;
		}


		if (ACEIT_IsReachable(self, target->s.origin) && target->solid != SOLID_NOT)
		{
			if (ACEAI_InfrontBot(self, target))
			{
				index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeed(self, index, target->timestamp, target->spawnflags);

				if (weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, 200); //true=bot
	}



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

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);

	while (target)
	{
		vec3_t itemOrigin, v1, v2;
		qboolean isRocket=false;
		int index_1 = ITEM_INDEX(target->item);

		if (target->classname == NULL)
			return;

		//int index_1 = ITEM_INDEX(FindItemByClassname(target->item->classname));
	
		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if (strcmp(target->classname, "rocket") == 0 )	{
			// acebot ToDo: hypo add player as rocket  target, so strafe/dodge is correct
			//need to work out when to dodge left or right?
			//self->acebot.rocketOwner = target->owner>
			//
			if (ACEAI_InfrontBot(self, target))
			{
				self->movetarget = target;//hypov8 todo: timeout loop;
				return;
			}
			isRocket = true;
		}
		if (strcmp(target->classname, "grenade") == 0)			{
			self->movetarget = target;//hypov8 todo: timeout loop;
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
				if (ACEAI_InfrontBot(self, target))
				{
					index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
					weight = ACEIT_ItemNeed(self, index, target->timestamp, target->spawnflags);

					if (weight > best_weight)
					{
						best_weight = weight;
						best = target;
					}
				}
			}
		}
		// next target
		target = findradius(target, self->s.origin, 200); //true=bot
	}

	if (best_weight)
	{

		self->movetarget = best;

		if (debug_mode && self->goalentity != self->movetarget && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
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

		if (debug_mode && self->goalentity != self->movetarget && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
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

	for (i = 1; i <= 20/*MAX_ITEMS*/; i++) //hypov8 last wep is item #16, change if we get more weps 
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


qboolean ACEAI_PickShortRangeGoal_Player (edict_t *self)
{
	if (self->acebot.state != BOTSTATE_WANDER)
		return false;
	if (self->client->hookstate)
		return false;
	if (self->acebot.isMovingUpPushed)
		return false;
	// enemy not valid yet(skill time)
	if (self->acebot.botSkillDeleyTimer > level.time)
		return false;
	if (self->acebot.old_targetID == -1)
		return false;
	//no longer intrested in enemy
	if (self->acebot.old_targetFrame <= level.framenum - 150)
		return false;

	//check our enemy. again?
	if (!self->acebot.isChasingEnemy)//self->acebot.chaseEnemyFrame < level.framenum -30) //2 x 3 seconds to seen enemy
	{
		int i;
		short int node; //hypo
		float best_weight=0.0;
		short current_node, goal_node = INVALID;
		self->acebot.targetPlayerNode = 0;

		for (i = 0; i<num_players; i++)
		{
			if (players[i] == self
				|| players[i]->solid == SOLID_NOT
				|| players[i]->movetype == MOVETYPE_NOCLIP
				|| players[i]->flags & FL_GODMODE
				|| players[i]->client->invincible_framenum > level.framenum)
				continue;

			if (i != self->acebot.old_targetID)
				continue;

			node = ACEND_FindClosestNode(players[i], BOTNODE_DENSITY, BOTNODE_ALL);
			if (node == INVALID)
				return false;

			current_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_ALL);
			if(current_node == INVALID)
			{
				ACEAI_Reset_Goal_Node(self, 1.0, "No close node for SRG (Player).");
				return false;
			}

			// OK, everything valid, let's start moving to our goal.
			self->acebot.targetPlayerNode = level.framenum + 50; //add hypov8 +3 sec timeout
			self->acebot.node_current = current_node;
			self->acebot.node_next = self->acebot.node_current; // make sure we get to the nearest node first
			self->acebot.node_timeout = 0;
			self->acebot.state = BOTSTATE_MOVE;
			self->acebot.node_tries = 0; // Reset the count of how many times we tried this goal
			self->acebot.chaseEnemyFrame = level.framenum;
			self->acebot.isChasingEnemy = true;
			self->acebot.node_goal = node;

			if (players[i] != NULL && debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
				debug_printf("%s selected a %s at node %d for SRG (Player).\n", self->client->pers.netname, players[i]->classname, node);

			return true;
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
			//break;
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
	for(i=0;i<num_players;i++)
	{
		if (players[i] == self
			|| players[i]->solid == SOLID_NOT
			|| players[i]->movetype == MOVETYPE_NOCLIP
			|| players[i]->flags & FL_GODMODE
			|| players[i]->client->invincible_framenum > level.framenum)
			continue;
		//node = ACEND_FindClosestReachableNode(players[i],BOTNODE_DENSITY,BOTNODE_ALL);
		node = ACEND_FindClosestNode(players[i],BOTNODE_DENSITY,BOTNODE_ALL);
		if (node == INVALID) ////hypo added to stop below returning crapola 
			continue;

		cost = ACEND_FindCost(current_node, node);
		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		// BEGIN HITMEN
		if (sv_hitmen->value /*enable_hitmen*/)
			weight = 2.5; 
		else
			 weight = 0.3; 
		
		weight *= random(); // Allow random variations
		weight /= (float)cost; // Check against cost of getting there
		
		if(weight > best_weight)
		{	
			self->acebot.targetPlayerNode = level.framenum + 100; //add hypov8 +10 sec timeout
			best_weight = weight;
			goal_node = node;
			goal_ent = players[i];
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
	 
	if (goal_ent != NULL && debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
		debug_printf("%s selected a %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node);

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
	qboolean isBooz=false,isBullet=true;

	self->acebot.aimLegs = 0;

	if (self->client->pers.weapon)
	{
		if (Q_stricmp(self->client->pers.weapon->classname, "weapon_bazooka") == 0)
		{
			isBooz = true;
			isBullet = false;
		}
		else if (Q_stricmp(self->client->pers.weapon->classname, "weapon_grenadelauncher") == 0 //hypov8 todo: flammer ok?
		|| Q_stricmp(self->client->pers.weapon->classname, "weapon_crowbar") == 0
		|| Q_stricmp(self->client->pers.weapon->classname, "Pipe") == 0)
		{
			isBullet = false;
		}

	}

	//used for top player using bullets. more accurate between gaps
	if (isBestplayer && isBullet){
		VectorCopy(vec3_origin,bbmin);
		VectorCopy(vec3_origin,bbmax);
	}

	if (isBullet)
		mask = MASK_BOT_ATTACK_LEAD;


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

static int botRankPlayerNum[MAX_CLIENTS];
static int botRankScores[MAX_CLIENTS];

static void ACEAI_GetScores()
{
	int		i, j, k;
	int		score, total;
	
	// sort the clients by score
	total = 0;
	for (i = 0; i < num_players; i++)
	{
		if (players[i] == NULL
			|| players[i]->solid == SOLID_NOT
			|| players[i]->movetype == MOVETYPE_NOCLIP
			|| players[i]->client->invincible_framenum > level.framenum	)
			continue;

		if (players[i]->client->pers.spectator == SPECTATING)
			continue;

		score = players[i]->client->resp.score;

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

}

static qboolean ACEAI_PickOnBestPlayer(int playerNum)
{
	int tmpScore;
	if (playerNum == botRankPlayerNum[0])
	{
		tmpScore = botRankScores[0] - botRankScores[1];
		if (tmpScore > 3)
		{
			if (debug_mode && level.framenum %10 == 0)
				debug_printf("    ******Best Player****** Targeted by bots.\n");
	
			return true;
		}
	}
	return false;
}


/*
=============
infrontEnem

returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
static qboolean ACEAI_InfrontEnemy(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot, fov;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);
	//hypov8
	fov = (4 - ACEMV_SkillMP(self)) / 2.25 - 1;	//-1 to 0.77 
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

	if (self->client->weaponstate != WEAPON_READY && self->client->weaponstate != WEAPON_FIRING)
		return false;

  //hypo stop bot attacking on ladders
	if (self->acebot.isOnLadder)
		return false;
	//add hypov8 stop bot shooting on ladders
	//if ((self->acebot.node_next != INVALID && nodes[self->acebot.node_next].type == BOTNODE_LADDER)
	//	|| (self->acebot.node_current != INVALID && nodes[self->acebot.node_current].type == BOTNODE_LADDER))
	//	return false;

	//hypo add timer for bot to not own so much
	if (self->acebot.botSkillDeleyTimer > level.time)
		return false;
	if (self->health < 1)
		return false;

	//hypov8 shoot enemy faster if being attacked
	if (self->acebot.lastDamageTimer >= (level.framenum - 40)) //4 seconds. allow for skill
		underAttack = true;

	//if (sv_botskill->value < 0.0f) sv_botskill->value = 0.0f;
	//if (sv_botskill->value > 4.0f) sv_botskill->value = 4.0f;

	//use player score to attack them harder
	 ACEAI_GetScores();

	 for (i = 0; i < num_players; i++)
	 {
		 if (players[i] == NULL)
			 continue;

		 //reset hunted. incase player NLA
		 players[i]->acebot.hunted = false;

		 //reset to no players stop bot hunting same player after death
		 if (self->acebot.old_targetID == i && players[i]->health < 1)		{
			 self->acebot.old_targetID = -1;
			 continue;
		 }

		 if (players[i] == self
			 || players[i]->solid == SOLID_NOT
			 || players[i]->movetype == MOVETYPE_NOCLIP
			 || players[i]->flags & FL_GODMODE //addhypov8
			 || players[i]->client->invincible_framenum > level.framenum
			 || (teamplay->value && self->client->pers.team == players[i]->client->pers.team) //on same team
			 || ACEAI_Enemy_ToFarHM(self, players[i])) // BEGIN HITMEN
			 continue;

#if 0//def HYPODEBUG
		 if (ACEAI_VisibleEnemy(self, players[i], true))
		 {
			vec3_t botsSight, enemySight, enemyLegs;
			VectorCopy(self->s.origin ,botsSight);
			VectorCopy(players[i]->s.origin, enemySight);
			VectorCopy(players[i]->s.origin, enemyLegs);
			botsSight[2] += self->viewheight;
			enemySight[2] += players[i]->viewheight;
			if (!gi.inPVS(botsSight, enemySight)&& !gi.inPVS(botsSight, enemyLegs))
				gi.dprintf("ERROR: Bot in view but not PVS\n");
			 
		 }
#endif
		// player is in direct sight. no solid walls
		 if (!players[i]->deadflag && ACEAI_VisibleEnemyPVS(self, players[i]))
		{
			if (players[i]->acebot.is_bot == 0 	&& ACEAI_PickOnBestPlayer(i) //hypov8 add. stop top player with force!! :)
				&& ACEAI_VisibleEnemy(self, players[i], true))
			{
				self->enemy = players[i];
				self->acebot.botSkillDeleyTimer = 0;
				players[i]->acebot.hunted = true;
				return true;
			}

			if (!ACEAI_VisibleEnemy(self, players[i], false)) //hypodebug.. temp test true
				continue;

			//moved down, anti spawn camp
			//if ( players[i]->client->invincible_framenum > level.framenum )
				//continue;
			
			//just keep shooting last target for now
			if (self->acebot.old_targetID == i)
			{
				self->enemy = players[i];
				if (self->client->weaponstate == WEAPON_FIRING)
					return true;
				//todo: check this. return even if not infront?
				else if (ACEAI_InfrontEnemy(self, players[i])) //hypo stop bot turning a full 360? using skill %))
					return true;
				else if (underAttack)
					return true;

				if (self->acebot.old_targetFrame < level.framenum - 150) //15 seconds, forget about old target
					self->acebot.old_targetID = -1;

				continue; // look 4 another player infront
			}

			//hypo dont add a new target if not infront
			if (!ACEAI_InfrontEnemy(self, players[i])  //hypo stop bot turning a full 360? using skill %))
				&& !underAttack) //understtack?
				continue;

			// Base selection on distance.	
			range = VectorDistance(self->s.origin, players[i]->s.origin);
			
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
		self->acebot.new_target = j;
		self->acebot.old_targetFrame = level.framenum;

		if (self->acebot.new_target != self->acebot.old_targetID)
		{
			self->acebot.old_targetID = j;
			if (self->client->weaponstate == WEAPON_FIRING)
				self->acebot.botSkillDeleyTimer = level.time + ((4 - ACEMV_SkillMP(self)) / 4);//hypov8 target changed. shoot next closer player sooner
			else if (underAttack)
				self->acebot.botSkillDeleyTimer = level.time + ((4 - ACEMV_SkillMP(self)) / 3); //hypov8 attacked. shoot faster
			else
				self->acebot.botSkillDeleyTimer = level.time +(4 - ACEMV_SkillMP(self)); //give player 2 seconds(default) leway between seeing n being shot

			return false; //self->client->weaponstate == WEAPON_FIRING;
		}

		self->enemy = players[j];
		return true;
	}
	return false; //nothing!!!
  
}

///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (hypo)
///////////////////////////////////////////////////////////////////////
static void ACEAI_PreChooseWeapon(edict_t *self)
{	
	switch (self->acebot.randomWeapon)
	{
	case 0:
		{
			// always favor the HMG
			if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
		}
		break;
	case 1:
		{
			// always favor the boozooka
			if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		}
		break;
	case 2:
	default:
		{
			// always favor the tommy
			if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		}
		break;
	} //end switch

	//no weps so chose shotty?
	if (ACEIT_ChangeWeapon(self, FindItem("Shotgun")))
		return;

}

 //ACEAI_Reset_Goal_Node(self, 1.0, "Found Enemy")

//add hypov8
//found an enemy, forget out goal
void ACEAI_Reset_Goal_Node(edict_t *self, float wanderTime, char* eventName)
{
	//if (self->acebot.node_goal != INVALID)
	{
		self->acebot.node_next = INVALID;
		self->acebot.node_goal = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + wanderTime;
		if (debug_mode&& !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s Wandering: %s\n", self->client->pers.netname, eventName);
	}
}

//add hypov8
//found an enemy, forget out goal
void ACEAI_ResetLRG_HM(edict_t *self)
{
	//if (self->acebot.node_goal != INVALID)
	{
		self->acebot.node_next = INVALID;
		self->acebot.node_goal = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		if (debug_mode&& !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s found enemy, remove LRG.\n", self->client->pers.netname);
	}
}


#if HYPODEBUG
//#define BOT_TIME_DIFF(base,diff) (diff=Sys_Milliseconds()-base;if(diff>1)gi.dprintf("timediff LRG=%d.\n",diff);)
#define BOT_TIME_DIFF(base,diff,str)\
			diff=Sys_Milliseconds()-base;\
			if(diff>1)\
				gi.dprintf("Bot TimeDiff %s: %d.\n",str,diff);
#else
#define BOT_TIME_DIFF(a,b,c) (b=a)
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

	VectorCopy(self->s.angles, self->acebot.deathAngles);
	VectorCopy(self->client->ps.viewangles, self->s.angles);
	VectorSet(self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset(&ucmd, 0, sizeof(ucmd));
	self->enemy = NULL;
	self->movetarget = NULL;
	self->client->resp.check_idle = level.framenum; //hypov8 just incase
	self->acebot.aimLegs = 0;

	if (self->acebot.isMovingUpPushed)	{ 
		self->acebot.next_move_time = level.time + 0.1f; //stops wander
		if (self->groundentity && self->acebot.trigPushTimer < level.framenum) //allow some time for lunch
		{
			self->acebot.isMovingUpPushed = false;
			self->acebot.wander_timeout = level.time -0.1f;
		}
	}

	//hypov8
	if (self->acebot.isJumpToCrate)	{
		if (level.framenum > self->acebot.crate_time)
			self->acebot.isJumpToCrate = false;
	}

	//hypov8
	if (self->acebot.water_time >= level.framenum)
	{
		if (!self->groundentity)
		{
			ucmd.forwardmove = BOT_FORWARD_VEL;
			ucmd.upmove = BOT_JUMP_VEL;
			ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
			ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
			ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);
			ucmd.msec = 100;
			self->client->ping = 0;

			ClientThink(self, &ucmd);
			self->nextthink = level.time + BOTFRAMETIME;
			return;
		}
		else
			self->acebot.water_time = 0;
	}

	if (level.framenum > self->acebot.ladder_time /*|| self->gravity*/)
		self->acebot.isOnLadder = false; //hypo stop bot attacking on ladders

	if (self->health <= 0)
	{
		if (!self->deadflag)
		self->deadflag = self->deadflag;
	}

	// Force respawn 
	if (self->deadflag)
	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
		//VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement
		if (self->velocity[2] > 0)
			self->velocity[2] = 0;
		//hypo add this to stop bots moving while dead. may need to reset some values?
		ClientThink(self, &ucmd);
		self->nextthink = level.time + BOTFRAMETIME;
		return;
	}

	//stop bot looking for recent enemy that didnt die
	if (self->acebot.isChasingEnemy)
	{
		if (self->acebot.chaseEnemyFrame < level.framenum - 50)
		{
			ACEAI_Reset_Goal_Node(self, 1.0, "Stoped looking for Enemy.");
			self->acebot.isChasingEnemy = false;
		}
	}



	// pick a new long range goal
	if (self->acebot.state == BOTSTATE_WANDER && self->acebot.wander_timeout < level.time
	&& !self->acebot.isMovingUpPushed && !self->acebot.isChasingEnemy)
		ACEAI_PickLongRangeGoal(self);

	BOT_TIME_DIFF(base, timediff, "LRG" );

	/////////////////////////////////////////////
	// check if bot is moving?
	/////////////////////////////////////////////
	self->acebot.moveDirVel = VectorDistance(self->acebot.oldOrigin, self->s.origin);	// setup velocity for timeouts
	if (self->acebot.moveDirVel > 2) //if (VectorLength(self->velocity) > 37) //
		self->acebot.suicide_timeout = level.time + 10.0;

	tmpTimeout = self->acebot.suicide_timeout - level.time;
	if (tmpTimeout < 6 && self->acebot.isOnLadder)
	{
		ACEAI_Reset_Goal_Node(self, 1.0, "Stuck on ladder, Turn 180.");

		self->acebot.isOnLadder = false;

		if (self->s.angles[YAW]>0) self->s.angles[YAW] -= 180.0;
		else					   self->s.angles[YAW] += 180.0;

		ucmd.forwardmove = BOT_FORWARD_VEL;
		ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);

		ClientThink(self, &ucmd);
		self->nextthink = level.time + BOTFRAMETIME+0.1f;//hypo add a frame between think
		return;
	}
	///////////////////
	// Stuck. Kill bot
	///////////////////
	if (tmpTimeout < 0)
	{
		self->flags &= ~FL_GODMODE; //hypov8 added. shown as player killed them selves now
		self->health = 0;
		meansOfDeath = MOD_BOT_SUICIDE;		//hypov8 added. shown as player killed them selves now
		VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement
		#if HYPODEBUG
		gi.dprintf("Bot %s Died at XYZ:(%f, %f, %f)\n",self->client->pers.netname, self->s.origin[0], self->s.origin[1], self->s.origin[2]);
		#endif
		player_die(self, self, self, 100000, vec3_origin, 0, 0); //hypov8 add null
	}

	////////////////////////////
	// Find a short range goal
	////////////////////////////
	if (!self->acebot.isMovingUpPushed && !self->client->hookstate)
	{
		ACEAI_PickShortRangeGoal(self);
		BOT_TIME_DIFF(base, timediff, "SRG" );
	}


	////////////////////////////////////////
	// Chose Weapons Before we see an anemy
	///////////////////////////////////////
	// BEGIN HITMEN
	if (sv_hitmen->value) /*enable_hitmen*/
		ACEAI_ChooseWeaponHM(self);//select wep if it has ammo
	else
	// END
		//select a weapon b4 we find enamy
		if (ACEAI_WeaponCount(self))
			ACEAI_PreChooseWeapon(self);
	

	/////////////////////////
	// Find enemies OR items 
	// hypo serch weps if reloading etc. can make it serch while fireing
	/////////////////////////
	if (ACEAI_FindEnemy(self))
	{
		BOT_TIME_DIFF(base, timediff, "Enemy" );
		ACEAI_ChooseWeapon(self);
		BOT_TIME_DIFF(base, timediff, "Choose Wep" );
		ACEMV_Attack(self, &ucmd);
		BOT_TIME_DIFF(base, timediff, "Atack" );
	}
	else //if (!self->acebot.isMovingUpPushed)
	{
		ACEAI_PickShortRangeGoal_Player(self); 	// keep chasing enemy?
		BOT_TIME_DIFF(base, timediff, "SRG Player");

		// Execute the move, or wander
		if (self->acebot.state == BOTSTATE_WANDER)
		{
			ACEMV_Wander(self, &ucmd);
			BOT_TIME_DIFF(base, timediff, "Wander");
		}
		else if (self->acebot.state == BOTSTATE_MOVE)
		{
			ACEMV_Move(self, &ucmd);
			BOT_TIME_DIFF(base, timediff, "Move");
		}
	}

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
	self->nextthink = level.time + BOTFRAMETIME;
	BOT_TIME_DIFF(base, timediff, "END" );
}
