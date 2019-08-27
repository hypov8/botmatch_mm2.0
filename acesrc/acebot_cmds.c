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
//  acebot_cmds.c - Main internal command processor
//
///////////////////////////////////////////////////////////////////////

#include "../g_local.h" //DIR_SLASH
#include "acebot.h"

#ifndef bla//  HYPODEBUG
qboolean debug_mode=false;
qboolean debug_mode_origin_ents = false; //local node
#else
qboolean debug_mode=true;
qboolean debug_mode_origin_ents = true; //local node
#endif
// add hypov8

int ACECM_ReturnBotSkillWeb(void)
{
	float skill = sv_botskill->value;

	if (skill < .2) return 0;
	else if (skill < .6) return 1;
	else if (skill < 1.0) return 2;
	else if (skill < 1.4) return 3;
	else if (skill < 1.8) return 4;
	else if (skill < 2.2) return 5;
	else if (skill < 2.6) return 6;
	else if (skill < 3.0) return 7;
	else if (skill < 3.4) return 8;
	else if (skill < 3.8) return 9;
	else if (skill > 3.8) return 10;

	return 0;
}

int ACECM_ReturnBotSkillWeb_var(float skill)
{
	if (skill < .2) return 0;
	else if (skill < .6) return 1;
	else if (skill < 1.0) return 2;
	else if (skill < 1.4) return 3;
	else if (skill < 1.8) return 4;
	else if (skill < 2.2) return 5;
	else if (skill < 2.6) return 6;
	else if (skill < 3.0) return 7;
	else if (skill < 3.4) return 8;
	else if (skill < 3.8) return 9;
	else if (skill > 3.8) return 10;

	return 0;
}

float ACECM_ReturnBotSkillFloat(int skill)
{

		 if (skill == 0) return 0.0;
	else if (skill == 1) return 0.4;
	else if (skill == 2) return 0.8;
	else if (skill == 3) return 1.2;
	else if (skill == 4) return 1.6;
	else if (skill == 5) return 2.0;
	else if (skill == 6) return 2.4;
	else if (skill == 7) return 2.8;
	else if (skill == 8) return 3.2;
	else if (skill == 9) return 3.6;
	else if (skill == 10) return 4.0;

	return 0.0;
}


void ACECM_BotDebug(changeState)
{
	int		i;
	edict_t	*doot;
	//char string[10];
	//dm_statusbar
	char str[1024];
	char *newstatusbar = " xr -170"
		" yv 100 string \"  *KEYS REBOUND* \""
		" yv 110 string \" KEY 0 = ADD MOVE \""
		" yv 120 string \" KEY 5 = ADD WATER \""
		" yv 130 string \" KEY 6 = ADD LADDER \""
		" yv 140 string \" KEY 7 = ADD JUMP  \""
		" yv 150 string \" KEY 8 = findnode \""
		" yv 160 string \" KEY 9 = movenode \""
		" xr -130"
		" yv 190 string \"sv_botpath\""
		" yv 200 string \"localnode\""
		" yv 210 string \"clearnode #\""
		" yv 220 string \"addlink # #\""
		" yv 230 string \"removelink # #\""
		" yv 240 string \"nodeFinal\" "		;


	if (changeState) //toggle mode
		debug_mode = debug_mode ? false : true;


	if (debug_mode)
	{
		safe_bprintf(PRINT_MEDIUM, "ACE: Debug Mode On\n");

		strcpy(str, dm_statusbar);
		strcat(str, newstatusbar);
		gi.configstring(CS_STATUSBAR, str);

		for_each_player_not_bot(doot, i)
		{
			//sv botdebug on
			//safe_cprintf(doot, PRINT_MEDIUM, "0=MOVE 1=LADDER 2=PLATFORM 3=TELEPORTER 4=ITEM 5=WATER 7=JUMP\n");
			//=======================================================
			/*	safe_cprintf(doot, PRINT_MEDIUM, " \n");*/
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†====================\n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† findnode gets closes #node\n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†   then copys # to movenode\n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† movenode copys #node to player origin\n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†====================\n\n");

			gi.WriteByte(13);
			gi.WriteString("bind 0 addnode 0;bind 5 addnode 5;bind 6 addnode 1; bind 7 addnode 7; bind 8 findnode; bind 9 movenode 999\n");
			gi.unicast(doot, true);
		}
	}
	else if (changeState)
	{
		debug_mode_origin_ents = false;
		safe_bprintf(PRINT_MEDIUM, "ACE: Debug Mode Off\n");
	}
}
void ACECM_BotAdd(char *name, char *skin, char *team, char *skill)
{
	float _skill = 1.0f;

#if HYPODEBUG
	int i;
	edict_t	*ent;
	for (i = 0; i<(int)maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || ent->acebot.is_bot)
			continue;

		safe_centerprintf(ent, "Bot Added");
	}
#endif

	if (skill[0] != '\0')
		_skill = (float)atof((const char*)skill);

	if (_skill < 0.0)
		_skill = 0.0f;
	else if (_skill > 2)
		_skill = 2.0f;

	/* bots need to be added between game start and end. */
	if (level.modeset == MATCH || level.modeset == PUBLIC)
	{
		if (teamplay->value) // name, skin, team 
			ACESP_SpawnBot(team, name, skin, NULL, _skill); //sv addbot thugBot "male_thug/009 031 031" dragon 1.0
		else // name, skin			
			ACESP_SpawnBot("\0", name, skin, NULL, _skill); //sv addbot thugBot "male_thug/009 031 031" null 1.0
	}
}
//end hypov8


///////////////////////////////////////////////////////////////////////
// Special command processor
///////////////////////////////////////////////////////////////////////
qboolean ACECM_Commands(edict_t *ent)
{
	char	*cmd;
	short	node = INVALID;

	if (!debug_mode)
		return false;

	cmd = gi.argv(0);

	//hypov8 also addnodelinked?
	if (Q_stricmp(cmd, "addnode") == 0)
	{
		ent->acebot.pm_last_node = ACEND_AddNode(ent, atoi(gi.argv(1)));
		if (!sv_botpath->value) 
			ent->acebot.pm_last_node = INVALID; //prevent bug when re'enable sv_botpath
	}
	
	else if(Q_stricmp (cmd, "removelink") == 0)
		ACEND_RemoveNodeEdge(ent,atoi(gi.argv(1)), atoi(gi.argv(2)));

	else if(Q_stricmp (cmd, "addlink") == 0)
		ACEND_UpdateNodeEdge(atoi(gi.argv(1)), atoi(gi.argv(2)), false, false, false, false);
	
	else if(Q_stricmp (cmd, "showpath") == 0)
    	ACEND_ShowPath(ent,atoi(gi.argv(1)));

	else if (Q_stricmp(cmd, "localnode") == 0) //hypov8 add. show nodes close by
	{
		if (!dedicated->value)
		{
			//arg1 = gi.argv(1);
			//if (Q_stricmp(arg1, "on") == 0)
			if (debug_mode_origin_ents == 0)
			{
				safe_cprintf(ent, PRINT_MEDIUM, "findlocalnode ON\n");
				debug_mode_origin_ents = 1;
			}
			else
			{
				safe_cprintf(ent, PRINT_MEDIUM, "findlocalnode OFF\n");
				debug_mode_origin_ents = 0;
			}
		}
	}
	else if(Q_stricmp (cmd, "findnode") == 0)
	{
		char strWrite[MAX_INFO_STRING];

		node = ACEND_FindClosestNode(ent,BOTNODE_DENSITY, BOTNODE_ALL);
		if (node != INVALID){
			if (dedicated->value)
				gi.dprintf("node: %d type: %d x: %f y: %f z %f\n", node, nodes[node].type, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
			safe_cprintf(ent, PRINT_MEDIUM, "node: %d type: %d x: %f y: %f z %f\n", node, nodes[node].type, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
		}
		else
			safe_cprintf(ent, PRINT_MEDIUM, "findnode: failed\n");

		ACEND_ShowNode(node, 1); //hypov8 show closest node

		sprintf(strWrite, "bind 9 movenode %d\n$Bound key 9 to %d\n", node, node);

		gi.WriteByte(13);
		gi.WriteString(strWrite);
		gi.unicast(ent, true);
	}

	else if (Q_stricmp(cmd, "movenode") == 0)
	{
		float x, y, z;
		char* nodeStr = gi.argv(1);

		if (stopNodeUpdate)
			return true;

		if (Q_stricmp(nodeStr, "") == 0) //hypov8 test for null
			return true;
		node = atoi(nodeStr);

		x = atof(gi.argv(2));
		y = atof(gi.argv(3));
		z = atof(gi.argv(4));

		if (node == INVALID)
			return true;

		//add hypov8 only allow to move general nodes
		if (nodes[node].type != BOTNODE_MOVE
			&& nodes[node].type != BOTNODE_JUMP
			&& nodes[node].type != BOTNODE_LADDER
			&& nodes[node].type != BOTNODE_GRAPPLE)
			return true;

		//hypov8 no node location specified, will use current player location
		if (!x && !y && !z)
		{
			VectorCopy(ent->s.origin, nodes[node].origin);
			nodes[node].origin[2] += BOTNODE_SHIFT;

			if (dedicated->value) //hypov8 todo: dont use debug in dedicated?
				gi.dprintf("node: %d moved to x: %f y: %f z %f\n",
				node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
			safe_cprintf(ent, PRINT_MEDIUM, "node: %d moved to x: %f y: %f z %f\n",
				node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
		}
		else
		{
			nodes[node].origin[0] = x;
			nodes[node].origin[1] = y;
			nodes[node].origin[2] = z;
			gi.dprintf("node: %d moved to x: %f y: %f z %f\n", node, x, y, z);
			safe_cprintf(ent, PRINT_MEDIUM, "node: %d moved to x: %f y: %f z %f\n", node, x, y, z);
		}

	}
	else if (Q_stricmp(cmd, "clearnode") == 0) //add hypov8 clear all paths to a node(cant be removed?)
		ACEND_RemovePaths(ent, (short)atoi(gi.argv(1)));

	else if (Q_stricmp(cmd, "clearallpaths") == 0) //add hypov8 clear all paths
		ACEND_RemoveallPaths(ent);

	else if (Q_stricmp(cmd, "nodefinal") == 0) //add hypov8 finalise node table
		stopNodeUpdate = 1;

	else
		return false;

	return true;
}


///////////////////////////////////////////////////////////////////////
// Called when the level changes, store maps and bots (disconnected)
///////////////////////////////////////////////////////////////////////
void ACECM_LevelEnd()
{
	ACEND_SaveNodes();
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
}

///////////////////////////////////////////////////////////////////////
// These routines are bot safe print routines, all id code needs to be 
// changed to these so the bots do not blow up on messages sent to them. 
// Do a find and replace on all code that matches the below criteria. 
//
// (Got the basic idea from Ridah)
//	
//  change: gi.cprintf to safe_cprintf
//  change: gi.bprintf to safe_bprintf
//  change: gi.centerprintf to safe_centerprintf
// 
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Debug print, could add a "logging" feature to print to a file
///////////////////////////////////////////////////////////////////////
void debug_printf(char *fmt, ...)
{
	int     i;
	char	bigbuffer[0x10000];
	int		len;
	va_list	argptr;
	edict_t	*cl_ent;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, PRINT_MEDIUM,"%s", bigbuffer);

	for (i=0 ; i<(int)maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->acebot.is_bot)
			continue;

		gi.cprintf(cl_ent, PRINT_MEDIUM,"%s", bigbuffer);
	}

}

///////////////////////////////////////////////////////////////////////
// botsafe cprintf
///////////////////////////////////////////////////////////////////////
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
#if 1
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (ent && (!ent->inuse || ent->acebot.is_bot))
		return;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.cprintf(ent, printlevel,"%s", bigbuffer);
#endif
}

///////////////////////////////////////////////////////////////////////
// botsafe centerprintf
///////////////////////////////////////////////////////////////////////
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
#if 1
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (!ent->inuse || ent->acebot.is_bot)
		return;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	
	gi.centerprintf(ent, "%s", bigbuffer); //hypo to keep
#endif
}

///////////////////////////////////////////////////////////////////////
// botsafe bprintf
///////////////////////////////////////////////////////////////////////
void safe_bprintf (int printlevel, char *fmt, ...)
{
#if 1
	int i;
	char	bigbuffer[0x10000];
	int		len;
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, printlevel,"%s", bigbuffer);

	for (i=0 ; i< (int)maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse) 
			continue;
		if (cl_ent->acebot.is_bot)
			continue;

		gi.cprintf(cl_ent, printlevel,"%s", bigbuffer);
	}
#endif
}

