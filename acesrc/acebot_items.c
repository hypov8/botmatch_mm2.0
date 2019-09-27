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
//  acebot_items.c - This file contains all of the 
//                   item handling routines for the 
//                   ACE bot, including fact table support
//           
///////////////////////////////////////////////////////////////////////

#include "../g_local.h" //DIR_SLASH
#include "acebot.h"

int	num_players;
int num_items;
int nodeFileComplet =0;
int botsRemoved = 0;
int num_bots;


item_table_t item_table[MAX_EDICTS];


//hypov8
//has item been picked up?
qboolean ACEIT_CheckIfItemExists(edict_t *self)
{
	int i;
	if (self->acebot.node_goal == INVALID)
		return true;
	if (nodes[self->acebot.node_goal].type != BOTNODE_ITEM)
		return true;
	if (self->acebot.targetPlayerNode > level.framenum)
		return true; //hypov8 player.

	//add hypov8 chect if item is still valid
	for (i = 0; i < num_items; i++)	{
		if (item_table[i].node == self->acebot.node_goal)
		{
			if (item_table[i].ent && !item_table[i].ent->solid)
				return false;
			return true; //found. still exist
		}
	}
	return false;
}


void ACEIT_PlayerCheckCount()
{
	edict_t	*bot;
	int i;
	char botname[16];

	if ((int)sv_bot_max_players->value != 0)
	{
		if (num_bots > 0 && num_players > 1)
		{
			int total = num_bots + num_players;
			if (total > (int)sv_bot_max_players->value)
			{
				for (i = 1; i <= (int) maxclients->value; i++) //num_players
				{
					bot = g_edicts + i;
					if (!bot->acebot.is_validTarget)
						continue;

					if (!bot->acebot.is_bot) 
						continue;

					if (bot->client)
					{
						strcpy(botname, bot->client->pers.netname);
						ACESP_RemoveBot(botname, true);
						botsRemoved++;
						return;
					}
				}
			}
		}
	}
}

void ACEIT_PlayerCheckAddCount()
{
	int count;
	// countBot = 0, countPlayer = 0;

	if (botsRemoved > 0)
	{
		if (sv_bot_max_players->value > 0)
		{
			count = num_bots + num_players;

			if ((int)sv_bot_max_players->value > count)
			{
				botsRemoved--;
				if (botsRemoved < 0) botsRemoved = 0;
				ACESP_SpawnBot_Random('\0', "\0", "\0", NULL);
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////
// Add the player to our list
///////////////////////////////////////////////////////////////////////

void ACEIT_PlayerAdded(edict_t *ent)
{
	if (!ent->acebot.is_bot)
		ACEIT_PlayerCheckCount();
}

///////////////////////////////////////////////////////////////////////
// Remove player from list
///////////////////////////////////////////////////////////////////////

void ACEIT_PlayerRemoved(edict_t *ent)
{

	if (!ent->acebot.is_bot)
		ACEIT_PlayerCheckAddCount();
}

///////////////////////////////////////////////////////////////////////
// Can we get there?
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsReachable(edict_t *self, vec3_t goal)
{
	trace_t trace;
	vec3_t goal_move_dn, player_move_up;
	vec_t jump_height;
	vec3_t minx, maxx;

	//hypo todo: check/cleanup. SRG. do we need on crouch?

	VectorCopy(goal, goal_move_dn);
	goal_move_dn[2] += BOTNODE_SHIFT;//hypov8 was minus. "items" are 15 units. ==old(goal move down 8. match player height)

	VectorCopy(self->mins,minx);
	VectorCopy(self->maxs, maxx);
	minx[2] += 18;// Stepsize/rough terain. hypov8 note: can get caught trying to goto crouch spots!!!
	//maxx[2] += 18;
	minx[0] = minx[1] = -15;//stop it catching on walls
	maxx[0] = maxx[1] = 15;
	

	trace = gi.trace(self->s.origin, minx, maxx, goal_move_dn, self, MASK_BOT_SOLID_FENCE); //hypo can we jump up? minus 12, jump to 60 units hypo: todo
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;

	//hypov8 also check ledges for items
	jump_height = goal_move_dn[2] - self->s.origin[2];
	if (jump_height >= 16 && jump_height <= 60) /*&& !self->acebot.isJumpToCrate*/
	{
		//match player height to entity
		VectorCopy(self->s.origin, player_move_up);
		player_move_up[2] = goal_move_dn[2];

		//move player bbox back down
		minx[2] -= 18;
		//maxx[2] -= 18;
		trace = gi.trace(player_move_up, minx, maxx, goal_move_dn, self, MASK_BOT_SOLID_FENCE); //hypo can we jump up? minus 12, jump to 60 units hypo: todo

		if (trace.allsolid == 0 && trace.startsolid == 0 && trace.fraction == 1.0)
		{
			self->acebot.isJumpToCrate= true;
			self->acebot.crate_time = level.framenum + 5;
			return true;
		}
	}

	return false;
}

#if 0 //hypov8 not used??
///////////////////////////////////////////////////////////////////////
// Visiblilty check 
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal)
{
	trace_t trace;
	
	trace = gi.trace (self->s.origin, vec3_origin, vec3_origin, goal, self, MASK_OPAQUE);
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}
#endif


static int ACEIT_ClipNameIndex(gitem_t *item)
{

	if (!strcmp(item->pickup_name, "Pipe"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Crowbar"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Pistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "SPistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "Shotgun"))
		return CLIP_SHOTGUN;
	else if (!strcmp(item->pickup_name, "Tommygun"))
		return CLIP_TOMMYGUN;
	else if (!strcmp(item->pickup_name, "FlameThrower"))
		return CLIP_FLAMEGUN;
	else if (!strcmp(item->pickup_name, "Bazooka"))
		return CLIP_ROCKETS;
	else if (!strcmp(item->pickup_name, "Grenade Launcher"))
		return CLIP_GRENADES;
	// JOSEPH 16-APR-99
	else if (!strcmp(item->pickup_name, "Heavy machinegun"))
		return CLIP_SLUGS;
	// END JOSEPH

	return (0);
}


///////////////////////////////////////////////////////////////////////
//  Weapon changing support
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
	int			clip_index; //ammon on wep

		
	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return true; 

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		clip_index = ACEIT_ClipNameIndex(item);


		if ((!ent->client->pers.inventory[ammo_index] && !ent->client->pers.weapon_clip[clip_index]) 
			&& !g_select_empty->value)
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;
	
	return true;
}


//add hypov8
qboolean ACEIT_Needs_Armor(edict_t *bot, char * itemName)
{
	if (strcmp(itemName, "item_armor_helmet") == 0)
	{
		gitem_t *itemh = FindItem("Helmet Armor");
		gitem_t *itemhh = FindItem("Helmet Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(itemh)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemhh)] > 50))
			return false;

	}
	else if (strcmp(itemName, "item_armor_jacket") == 0)
	{
		gitem_t *itemj = FindItem("Jacket Armor");
		gitem_t *itemjh = FindItem("Jacket Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(itemj)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemjh)] > 50))
			return false;
	}
	else if (strcmp(itemName, "item_armor_legs") == 0)
	{
		gitem_t *iteml = FindItem("Legs Armor");
		gitem_t *itemlh = FindItem("Legs Armor Heavy");

		if ((bot->client->pers.inventory[ITEM_INDEX(iteml)] > 80) ||
			(bot->client->pers.inventory[ITEM_INDEX(itemlh)] >50))
			return false;

	}
	else if (strcmp(itemName, "item_armor_helmet_heavy") == 0)
	{
		gitem_t *itemhh = FindItem("Helmet Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemhh)] > 50)
			return false;

	}
	else if (strcmp(itemName, "item_armor_jacket_heavy") == 0)
	{
		gitem_t *itemjh = FindItem("Jacket Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemjh)] > 50)
			return false;
	}
	else if (strcmp(itemName, "item_armor_legs_heavy") == 0)
	{
		gitem_t *itemlh = FindItem("Legs Armor Heavy");

		if (bot->client->pers.inventory[ITEM_INDEX(itemlh)] > 50)
			return false;
	}
	return true;
}



///////////////////////////////////////////////////////////////////////
// Determins the NEED for an item
//
// This function can be modified to support new items to pick up
// Any other logic that needs to be added for custom decision making
// can be added here. For now it is very simple.
///////////////////////////////////////////////////////////////////////
float ACEIT_ItemNeed(edict_t *self, int item, float timestamp, int spawnflags)
{
	//weapon search multiplyer. stop bot using pistol to much
	int nedWepMulti = (self->acebot.num_weps <= 4) ? 2.0 : 1.0;

	// Make sure item is at least close to being valid
	if(item < 0 || item > 100)
		return 0.0;


	//hypov8 make bot ignore all items if they have cash
	if (teamplay->value == 1)
		if ((self->client->pers.currentcash >= MAX_CASH_PLAYER || self->client->pers.bagcash >= 100))
		{
			if (item != ITEMLIST_SAFEBAG1 && self->client->pers.team == TEAM_1)
				return 0.0;
			if (item != ITEMLIST_SAFEBAG2 && self->client->pers.team == TEAM_2)
				return 0.0;
		}

// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
	{	//skip items that are auto asigned
		switch (item)
		{
			//weapons
			case ITEMLIST_PISTOL:
			case ITEMLIST_SPISTOL:
			case ITEMLIST_TOMMYGUN:
			case ITEMLIST_SHOTGUN:
			case ITEMLIST_GRENADELAUNCHER:
			case ITEMLIST_FLAMETHROWER:
			case ITEMLIST_BAZOOKA:
			case ITEMLIST_HEAVYMACHINEGUN:
			// Ammo
			case ITEMLIST_GRENADES: 
			case ITEMLIST_SHELLS: 
			case ITEMLIST_BULLETS: 
			case ITEMLIST_ROCKETS: 
			case ITEMLIST_AMMO308:
			case ITEMLIST_CYLINDER:	
			case ITEMLIST_FLAMETANK:
			return 0.0;
			break;

		}
	}
// END


	//hypov8 calculate if we need the ammo, then the gun
	if (spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
	{
		switch (item)
		{
			// Weapons that are droped. when player dies..
		case ITEMLIST_PISTOL:
		case ITEMLIST_SPISTOL:
		case ITEMLIST_TOMMYGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 0.3*nedWepMulti; break;
		case ITEMLIST_SHOTGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] < self->client->pers.max_shells) return 0.4*nedWepMulti; break;
		case ITEMLIST_GRENADELAUNCHER:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Grenades"))] < self->client->pers.max_grenades) return 0.3*nedWepMulti; break;
		case ITEMLIST_FLAMETHROWER:	if (self->client->pers.inventory[ITEM_INDEX(FindItem("Gas"))] < self->client->pers.max_cells) return 0.6*nedWepMulti; break;
		case ITEMLIST_BAZOOKA:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] < self->client->pers.max_rockets)	return 1.5*nedWepMulti;	break;
		case ITEMLIST_HEAVYMACHINEGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))] < self->client->pers.max_slugs) return 1.5*nedWepMulti; break;
		}
	}



	switch (item)
	{
		// Health
		case ITEMLIST_HEALTH_SMALL:
		case ITEMLIST_HEALTH_LARGE:
		case ITEMLIST_ADRENALINE: if (self->health < 100) return 4.0 - (float)self->health / 100.0f; // worse off, higher priority
			break;
		// Cash
		case ITEMLIST_CASHROLL:
		case ITEMLIST_CASHBAGLARGE:				//hypov8 let cash fall for 5 sec first
		case ITEMLIST_CASHBAGSMALL:	if (/*timestamp &&*/ timestamp <= level.time + 55) return 4.0;break;

		// Weapons
		case ITEMLIST_CROWBAR: 	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_crowbar"))]) return 0.1; break;
		case ITEMLIST_PISTOL:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_pistol"))]) return 0.1*nedWepMulti; break;
		case ITEMLIST_SPISTOL:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_spistol"))]) return 0.1*nedWepMulti; break;
		case ITEMLIST_SHOTGUN:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_shotgun"))]) return 2.8*nedWepMulti; break;
		case ITEMLIST_TOMMYGUN:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_tommygun"))]) return 2.8*nedWepMulti; break;
		case ITEMLIST_GRENADELAUNCHER:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_grenadelauncher"))]) return 2.8*nedWepMulti; break;
		case ITEMLIST_FLAMETHROWER:	if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_flamethrower"))]) return 2.8*nedWepMulti; break;

		case ITEMLIST_BAZOOKA: if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_bazooka"))]) return 3.0*nedWepMulti; break;
		case ITEMLIST_HEAVYMACHINEGUN: if (!self->client->pers.inventory[ITEM_INDEX(FindItemByClassname("weapon_heavymachinegun"))]) return 3.0*nedWepMulti; break;

		// Ammo
		case ITEMLIST_GRENADES: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Grenades"))] < self->client->pers.max_grenades) return 0.3; break;
		case ITEMLIST_SHELLS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] < self->client->pers.max_shells) return 0.4; break;
		case ITEMLIST_BULLETS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 0.3; break;
		case ITEMLIST_ROCKETS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] < self->client->pers.max_rockets)	return 1.5;	break;
		case ITEMLIST_AMMO308: if (self->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))] < self->client->pers.max_slugs) return 1.5; break;
		case ITEMLIST_CYLINDER:	if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 1.4; break;
		case ITEMLIST_FLAMETANK: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Gas"))] < self->client->pers.max_cells) return 0.6; break;

		// Armor
		case ITEMLIST_ARMORHELMET:			if (ACEIT_Needs_Armor(self, "item_armor_helmet"))			return  0.4; break;
		case ITEMLIST_ARMORJACKET:			if (ACEIT_Needs_Armor(self, "item_armor_jacket"))			return 0.4;	break;
		case ITEMLIST_ARMORLEGS:			if (ACEIT_Needs_Armor(self, "item_armor_legs"))				return 0.4;	break;

		case ITEMLIST_ARMORHELMETHEAVY:		if (ACEIT_Needs_Armor(self, "item_armor_helmet_heavy"))		return 0.8;	break;
		case ITEMLIST_ARMORJACKETHEAVY:		if (ACEIT_Needs_Armor(self, "item_armor_jacket_heavy"))		return 0.8;	break;
		case ITEMLIST_ARMORLEGSHEAVY:		if (ACEIT_Needs_Armor(self, "item_armor_legs_heavy"))		return 0.8;	break;


		// Bagman
		case ITEMLIST_SAFEBAG1:
			if (teamplay->value == 1)
			{	//deposit cash
				if (self->client->pers.team == TEAM_1 && (self->client->pers.currentcash >= 50 || self->client->pers.bagcash >= 50))
				{
					//ToDo: set long term goal if cash was picked up
					return 4.0;
				}
				else if (self->client->pers.team == TEAM_2)
				{	//if enamy safe has cash and player is not full
					if (team_cash[TEAM_1] > 0 && self->client->pers.bagcash < MAX_BAGCASH_PLAYER)
						return 4.0;
				}
			}
			break;
		case ITEMLIST_SAFEBAG2:
			if (teamplay->value == 1)
			{	//deposit cash
				if (self->client->pers.team == TEAM_2 && (self->client->pers.currentcash >= 50 || self->client->pers.bagcash >= 50))
				{
					//ToDo: set long term goal if cash was picked up
					return 4.0;
				}
				else 
				if (self->client->pers.team == TEAM_1)
				{	//if enamy safe has cash and player is not full
					if (team_cash[TEAM_2] > 0 && self->client->pers.bagcash < MAX_BAGCASH_PLAYER)
						return 4.0;
				}
			}
			break;

		// Weapon Mods
		case ITEMLIST_HMG_COOL_MOD:
			if (!(self->client->pers.pistol_mods & WEAPON_MOD_COOLING_JACKET)) 
				return 1.2; 
			break;
		case ITEMLIST_PISTOLMOD_DAMAGE:
			if (!(self->client->pers.pistol_mods & WEAPON_MOD_DAMAGE))
				return 0.5;
			break;
		case ITEMLIST_PISTOLMOD_RELOAD:
			if (!(self->client->pers.pistol_mods & WEAPON_MOD_RELOAD))
				return 0.5;
			break;
		case ITEMLIST_PISTOLMOD_ROF:
			if (!(self->client->pers.pistol_mods & WEAPON_MOD_ROF))
				return 0.5;
			break;
	

		case ITEMLIST_PACK: 
			if (self->client->pers.max_bullets < 300) //must not have pack yet. does not check low ammo..
			return 0.5; break;

		case ITEMLIST_TRIG_PUSH://"trigger_push"
			if ( (rand() % 10)== 5)
				return 0.1;
			break;

		case ITEMLIST_TELEPORTER:return 0.1;

		default:	return 0.0;			
	}

	return 0.0;	
}


//hypo add. get gun items after spawning
float ACEIT_ItemNeedSpawned(edict_t *self, int item, float timestamp, int spawnflags)
{
	//gitem_t *itemArmor;

	// Make sure item is at least close to being valid
	if (item < 0 || item > 100)
		return 0.0;

	switch (item)
	{
		// Weapons
	case ITEMLIST_CROWBAR: if (!self->client->pers.inventory[item])return 0.1; break;
	case ITEMLIST_PISTOL:
	case ITEMLIST_SPISTOL:
	case ITEMLIST_SHOTGUN:
	case ITEMLIST_TOMMYGUN:
	case ITEMLIST_GRENADELAUNCHER:
	case ITEMLIST_FLAMETHROWER:	if (!self->client->pers.inventory[item]) return 2.8; break; //was 1.8

	case ITEMLIST_BAZOOKA:
	case ITEMLIST_HEAVYMACHINEGUN: if (!self->client->pers.inventory[item]) return 3.0; break; //was 2.0


	case ITEMLIST_PISTOLMOD_DAMAGE:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_DAMAGE))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_RELOAD:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_RELOAD))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_ROF:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_ROF))
			return 0.5;
		break;

	default:
		return 0.0;

	}
	return 0.0;

}





///////////////////////////////////////////////////////////////////////
// Convert a classname to its index value
//
// I prefer to use integers/defines for simplicity sake. This routine
// can lead to some slowdowns I guess, but makes the rest of the code
// easier to deal with.
///////////////////////////////////////////////////////////////////////
int ACEIT_ClassnameToIndex(char *classname, int style)
{
	//int value = INVALID;

	int index = ITEM_INDEX(FindItemByClassname("item_armor_helmet"));

	if (strncmp(classname, "i", 1) == 0)
	{
		if (strncmp(classname, "item_a", 6) == 0)
		{//armor
			if (strcmp(classname, "item_armor_helmet") == 0) 		return ITEMLIST_ARMORHELMET;
			if (strcmp(classname, "item_armor_jacket") == 0)		return ITEMLIST_ARMORJACKET;
			if (strcmp(classname, "item_armor_legs") == 0)			return ITEMLIST_ARMORLEGS;
			if (strcmp(classname, "item_armor_helmet_heavy") == 0) 	return ITEMLIST_ARMORHELMETHEAVY;
			if (strcmp(classname, "item_armor_jacket_heavy") == 0)	return ITEMLIST_ARMORJACKETHEAVY;
			if (strcmp(classname, "item_armor_legs_heavy") == 0)	return ITEMLIST_ARMORLEGSHEAVY;
		}
		else
		{//misc
			if (strcmp(classname, "item_health_lg") == 0)			return ITEMLIST_HEALTH_LARGE;
			if (strcmp(classname, "item_health_sm") == 0)			return ITEMLIST_HEALTH_SMALL;
			if (strcmp(classname, "item_pack") == 0)				return ITEMLIST_PACK;
			if (strcmp(classname, "item_adrenaline") == 0)			return ITEMLIST_ADRENALINE;
			if (strcmp(classname, "item_cashroll") == 0)			return ITEMLIST_CASHROLL;
			if (strcmp(classname, "item_cashbaglarge") == 0)		return ITEMLIST_CASHBAGLARGE;
			if (strcmp(classname, "item_cashbagsmall") == 0)		return ITEMLIST_CASHBAGSMALL;
		}
	}
	else if (strncmp(classname, "w", 1) == 0)
	{
		//weapons
		if (strcmp(classname, "weapon_crowbar") == 0)			return ITEMLIST_CROWBAR;
		if (strcmp(classname, "weapon_pistol") == 0)			return ITEMLIST_PISTOL;
		if (strcmp(classname, "weapon_spistol") == 0)			return ITEMLIST_SPISTOL;
		if (strcmp(classname, "weapon_shotgun") == 0)			return ITEMLIST_SHOTGUN;
		if (strcmp(classname, "weapon_tommygun") == 0)			return ITEMLIST_TOMMYGUN;
		if (strcmp(classname, "weapon_heavymachinegun") == 0)	return ITEMLIST_HEAVYMACHINEGUN;
		if (strcmp(classname, "weapon_grenadelauncher") == 0)	return ITEMLIST_GRENADELAUNCHER;
		if (strcmp(classname, "weapon_bazooka") == 0)			return ITEMLIST_BAZOOKA;
		if (strcmp(classname, "weapon_flamethrower") == 0)		return ITEMLIST_FLAMETHROWER;
	}
	else if (strncmp(classname, "a", 1) == 0)
	{
		//ammo
		if (strcmp(classname,"ammo_grenades")==0)			return ITEMLIST_GRENADES;
		if (strcmp(classname,"ammo_shells")==0)				return ITEMLIST_SHELLS;
		if (strcmp(classname,"ammo_bullets")==0)			return ITEMLIST_BULLETS;
		if (strcmp(classname,"ammo_rockets")==0)			return ITEMLIST_ROCKETS;
		if (strcmp(classname,"ammo_308")==0)				return ITEMLIST_AMMO308;
		if (strcmp(classname,"ammo_cylinder")==0)			return ITEMLIST_CYLINDER;
		if (strcmp(classname,"ammo_flametank")==0)			return ITEMLIST_FLAMETANK;
	}
	else  if (strncmp(classname, "p", 1) == 0)
	{
		//pistol
		if (strcmp(classname,"pistol_mod_damage")==0)		return ITEMLIST_PISTOLMOD_DAMAGE;
		if (strcmp(classname, "pistol_mod_reload") == 0)	return ITEMLIST_PISTOLMOD_RELOAD;
		if (strcmp(classname, "pistol_mod_rof") == 0)		return ITEMLIST_PISTOLMOD_ROF;
	}
	else
	{
		//misc items
		if (strcmp(classname, "hmg_mod_cooling") == 0)		return ITEMLIST_HMG_COOL_MOD;
		if (strcmp(classname, "trigger_push") == 0)			return ITEMLIST_TRIG_PUSH;
		if (strcmp(classname, "misc_teleporter") == 0)		return ITEMLIST_TELEPORTER;
		if (strcmp(classname, "dm_safebag") == 0)
			if (style && style == 1)	return ITEMLIST_SAFEBAG1;	else return ITEMLIST_SAFEBAG2;

	}

	return INVALID;
}



///////////////////////////////////////////////////////////////////////
// Only called once per level, when saved will not be called again
//
// Downside of the routine is that items can not move about. If the level
// has been saved before and reloaded, it could cause a problem if there
// are items that spawn at random locations.
//
#if HYPODEBUG
#define DEBUG_ACE // uncomment to write out items to a file.
#endif
///////////////////////////////////////////////////////////////////////
void ACEIT_BuildItemNodeTable(qboolean reLinkEnts)
{
	edict_t *items;
	int item_index;
	short i;
	vec3_t v, v1, v2;

#ifdef DEBUG_ACE
	FILE *pOut; // for testing
	cvar_t	*game_dir;
	char buf[MAX_QPATH];

	game_dir = gi.cvar("game", "", 0);
	Com_sprintf(buf, sizeof(buf), "%s/items.txt", game_dir->string);

	if ((pOut = fopen(buf, "wt")) == NULL) //hypov8 //comp/items.txt
		return;

	fprintf(pOut, "Map: %s\n",level.mapname);
#endif

	num_items = 0;

	// Add game items
	for (items = g_edicts; items < &g_edicts[globals.num_edicts]; items++)
	{
		// filter out crap
		if (items->solid == SOLID_NOT)
			continue;

		if (!items->classname)
			continue;

		/////////////////////////////////////////////////////////////////
		// Items
		/////////////////////////////////////////////////////////////////
		item_index = ACEIT_ClassnameToIndex(items->classname, items->style); //hypov8 add safe styles

		////////////////////////////////////////////////////////////////
		// SPECIAL NAV NODE DROPPING CODE
		////////////////////////////////////////////////////////////////
		// Special node dropping for platforms
		if (strcmp(items->classname, "func_plat") == 0)
		{
			if (!reLinkEnts)
			{
				item_table[num_items].node = ACEND_AddNode(items, BOTNODE_PLATFORM);
				item_table[num_items].ent = items;
				item_table[num_items].item = 99;
				num_items++;
				continue; //add hypov8
			}
			item_index = 99;
		}

		// Special node dropping for teleporters
		if (strcmp(items->classname, "misc_teleporter") == 0)
		{
			if (!reLinkEnts)
			{
				item_table[num_items].node = ACEND_AddNode(items, BOTNODE_TELEPORTER);
				item_table[num_items].item = 99;
				num_items++;
				continue; //add hypov8
			}
			item_index = 99;
		}

		// Special node dropping for trigger_push
		if (strcmp(items->classname, "trigger_push") == 0)
		{
			if (!reLinkEnts)
				ACEND_AddNode(items, BOTNODE_TRIGPUSH);
			continue; //just drop a node. not linked to an entity
		}



#ifdef DEBUG_ACE
		if (item_index == INVALID)
			fprintf(pOut, "Rejected item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
		else
			fprintf(pOut, "item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif		

		if (item_index == INVALID)
			continue;

		// add a pointer to the item entity
		item_table[num_items].ent = items;
		item_table[num_items].item = item_index;

		// If new, add nodes for items
		if (!reLinkEnts)
		{
			// Add a new node at the item's location.
			item_table[num_items].node = ACEND_AddNode(items, BOTNODE_ITEM);
			num_items++;
		}
		else // Now if rebuilding, just relink ent structures 
		{
			// Find stored location
			for (i = 0; i < numnodes; i++)
			{
				if (nodes[i].type == BOTNODE_ITEM || nodes[i].type == BOTNODE_TELEPORTER ||
					nodes[i].type == BOTNODE_DRAGON_SAFE || nodes[i].type == BOTNODE_NIKKISAFE ||
					nodes[i].type == BOTNODE_PLATFORM) // valid types
				{
					VectorCopy(items->s.origin, v);

					// shift nodes up to match PathMap creation height of 24 units
					if (nodes[i].type == BOTNODE_ITEM)			v[2] += BOTNODE_ITEM_16;
					if (nodes[i].type == BOTNODE_TELEPORTER)	v[2] += BOTNODE_TELEPORTER_16;
					if (nodes[i].type == BOTNODE_DRAGON_SAFE)	v[2] += BOTNODE_DRAGON_SAFE_8;
					if (nodes[i].type == BOTNODE_NIKKISAFE)		v[2] += BOTNODE_NIKKISAFE_8;

					if (nodes[i].type == BOTNODE_PLATFORM)
					{
						VectorCopy(items->maxs, v1);
						VectorCopy(items->mins, v2);

						// To get the center
						v[0] = (v1[0] - v2[0]) / 2 + v2[0];
						v[1] = (v1[1] - v2[1]) / 2 + v2[1];
						v[2] = items->maxs[2] + items->pos2[2] + BOTNODE_PLATFORM_32;
					}

					if (v[0] == nodes[i].origin[0] &&
						v[1] == nodes[i].origin[1] &&
						v[2] == nodes[i].origin[2])
					{
						// found a match now link to facts
						item_table[num_items].node = i;
#ifdef DEBUG_ACE
						fprintf(pOut, "Relink item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif							
						num_items++;
						break; //add hypov8. stop it serching for new items. will get stuck if item is at same origin
					}
				}
			}
			if (i == numnodes)
			{	//cant find a match. try rebild
				if (!level.aceNodesCurupt)
				{
					level.aceNodesCurupt = true;
#ifdef DEBUG_ACE
					fclose(pOut);
#endif
					return;
				}
				//this should not happen anymore.. but who knows!!!
				//fprintf(pOut, "ERROR Relink item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
				if (debug_mode)	
					gi.dprintf("ERROR Relink item: %s node: %d pos: %f %f %f\n",items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
			}
		}

	}

#ifdef DEBUG_ACE
	fclose(pOut);
#endif

}



