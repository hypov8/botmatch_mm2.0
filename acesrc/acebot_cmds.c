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

//hypo vote add bot
static void Cmd_VoteAddBot_f(edict_t *ent, int teamUse)
{
	int i, count;
	edict_t *dude;
	char	*s = '\0';
	char *team = '\0';


	if (!sv_bot_allow_add->value)
	{
		safe_cprintf(ent, PRINT_HIGH, "Clients NOT allowed to add bots\n");
		return;
	}

	if (num_bots >= (int)sv_bot_max->value)
	{
		safe_cprintf(ent, PRINT_HIGH, "Maximum Bots Reached\n");
		return;
	}


	if (teamplay->value)
	{
		if (teamUse)
		{
			if (teamUse == 1){
				team = "d";
				s = "d";
			}
			else if (teamUse == 2){
				team = "n";
				s = "n";
			}
			else team = '\0';
		}
		else
		{
			s = gi.args();
			if (s[0])
				team = s;
		}
	}

	if (level.voteset == NO_VOTES)
	{
		count = 0;
		for_each_player_not_bot(dude, i)
		{
			dude->client->resp.vote = 0;
			count++;
		}

		if (count == 1)
		{
			//safe_bprintf(PRINT_HIGH, "%s alowed to change map.\n", ent->client->pers.netname);
			gi.dprintf("Vote AddBot by: %s accepted.\n", ent->client->pers.netname);
			safe_cprintf(ent, PRINT_CHAT, "¡­%s alowed to add bot.\n", ent->client->pers.netname);
			//ACESP_SpawnBot(team, "\0", "\0", NULL);
			ACESP_SpawnBot_Random(team, "\0", "\0", NULL);
			return;
		}						//¡¢£¤¥¦§¨©ª«¬­­
		safe_bprintf(PRINT_CHAT, "%s has requested to add a bot.\n\nPLEASE VOTE\n\n", ent->client->pers.netname);
		ent->client->resp.vote = CALLED_VOTE;
		level.voteframe = level.framenum;
		level.voteset = VOTE_ADDBOT;

		if (teamplay->value)
			strcpy(voteAddBot, s);

		gi.WriteByte(svc_stufftext);
		gi.WriteString("play misc/talk.wav");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
	else if (level.voteset != NO_VOTES)
		safe_cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

}

//hypo vote remove bot
void Cmd_VoteRemoveBot_f(edict_t *ent, qboolean isMenu, char botnames[32]) //VOTE_REMOVEBOT;
{
	int i, count;
	edict_t *dude;
	char		*s;
	char *name = '\0';

	if (!sv_bot_allow_add->value)
	{
		safe_cprintf(ent, PRINT_HIGH, "Bot Vote Disabled.\n", ent->client->pers.netname);
		return;
	}

	if (!isMenu)
	{
		s = gi.args();
		if (s[0])
			name = s;
		else if (s[0] == '\0')
		{
			safe_cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
			return;
		}
	}
	else
	{
		if (botnames == NULL || !botnames[0])		
		{
			safe_cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
			return;
		}
		s = botnames;
		name = botnames;
	}


	if (level.voteset == NO_VOTES)
	{
		count = 0;
		for_each_player_inc_bot(dude, i)
		{
			if (!dude->acebot.is_bot)
				continue;
			if (_strcmpi(dude->client->pers.netname, s) == 0)
				count++;
		}

		if (count == 0)
		{
			safe_cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
			return;
		}
	}

	if (level.voteset == NO_VOTES)
	{
		count = 0;
		for_each_player_not_bot(dude, i)
		{
			dude->client->resp.vote = 0;
			count++;
		}

		if (count == 1)
		{
			gi.dprintf("Vote BotRemove accepted. by: %s\n", ent->client->pers.netname);
			//safe_bprintf(PRINT_HIGH, "%s alowed to change map.\n", ent->client->pers.netname);
			safe_cprintf(ent, PRINT_CHAT, "%s alowed to remove bot.\n", ent->client->pers.netname);
			ACESP_RemoveBot(s);
			return;
		}						//¡¢£¤¥¦§¨©ª«¬­­
		safe_bprintf(PRINT_CHAT, "%s has requested to remove bot %s.\n\nPLEASE VOTE\n\n", ent->client->pers.netname, name);
		ent->client->resp.vote = CALLED_VOTE;
		level.voteframe = level.framenum;
		level.voteset = VOTE_REMOVEBOT;

		strcpy(voteRemoveBot, s);

		gi.WriteByte(svc_stufftext);
		gi.WriteString("play misc/talk.wav");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
	else if (level.voteset != NO_VOTES)
		safe_cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

}

//hypo vote skill
void Cmd_VoteSkill_f(edict_t *ent, qboolean usedMenu) //VOTE_BOTSKILL;

{
	int i, count;
	edict_t *dude;
	char		*s;
	float skill_f;

	if (!sv_bot_allow_skill->value)	{
		safe_cprintf(ent, PRINT_HIGH, "Bot-Skill Voting Disabled.\n", ent->client->pers.netname);
		return;
	}

	if (!usedMenu) 
	{
		int skill;
		s = gi.args();
		skill = atoi(s);

		if (s[0] == '\0' || skill < 0 || skill > 10)
		{
			cprintf(ent, PRINT_HIGH, "INVALID BOT SKILL. VALUE= 0 to 10.(current: %d)\n",
				ent->client->pers.netname, ACECM_ReturnBotSkillWeb());
			return;
		}
		skill_f = ACECM_ReturnBotSkillFloat(skill);
	}
	else
		skill_f = menuBotSkill;


	if (level.voteset == NO_VOTES)
	{
		count = 0;
		for_each_player_not_bot(dude, i)
		{
			dude->client->resp.vote = 0;
			count++;
		}

		if (count == 1)
		{
			char sSkill[16];
			Com_sprintf(sSkill, sizeof(sSkill), "%1.1f", skill_f);
			sv_botskill = gi.cvar_set("sv_botskill", sSkill);
			safe_bprintf(PRINT_HIGH, "Bot-Skill set to %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
			cprintf(ent, PRINT_HIGH, "This setting takes effect immediately\n");
			return;
		}						//¡¢£¤¥¦§¨©ª«¬­­

		safe_bprintf(PRINT_CHAT, "%s has requested to change bot skill to %d of 10.\n\nPLEASE VOTE\n\n", ent->client->pers.netname, ACECM_ReturnBotSkillWeb_var(skill_f));
		ent->client->resp.vote = CALLED_VOTE;
		level.voteframe = level.framenum;
		level.voteset = VOTE_BOTSKILL;

		voteBotSkill = skill_f;


		gi.WriteByte(svc_stufftext);
		gi.WriteString("play misc/talk.wav");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
	else if (level.voteset != NO_VOTES)
		cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

}





qboolean ACECM_G_Activate_f(edict_t *ent)
{
	if (ent->client->showscores == SCORE_BOT_VOTE)
	{
		switch (ent->menu)
		{
		case 1:ent->client->showscores = SCORE_BOT_ADD; ent->menu = 0; break;
		case 2:ent->client->showscores = SCORE_BOT_REMOVE; ent->menu = 0; break;
		case 3:ent->client->showscores = SCORE_BOT_SKILL; ent->menu = 0; break;
		case 4:	if (ent->client->pers.noantilag) 
					Cmd_AntiLag_f(ent, "1"); //ent->client->pers.noantilag = 0;
				else	
					Cmd_AntiLag_f(ent, "0"); //ent->client->pers.noantilag = 1;
				ent->menu = 4; break;

	
		case 5:Cmd_Yes_f(ent); ent->client->showscores = NO_SCOREBOARD; ent->menu = 0; break;
		case 6:Cmd_No_f(ent); ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;	break;

		default: ent->client->showscores = NO_SCOREBOARD; ent->menu = 0; break;
			

		}
		//ent->menu = 0;
		DeathmatchScoreboard(ent);
		return true;
	}
	
	if (ent->client->showscores == SCORE_BOT_ADD)
	{
		if (level.modeset == MATCH || level.modeset == PUBLIC)
		{
			switch (ent->menu)
			{
			case 1: Cmd_VoteAddBot_f(ent,1) ; break;
			case 2: Cmd_VoteAddBot_f(ent, 2); break;
			case 3:Cmd_VoteAddBot_f(ent, 0); break;
			default: ; break;

			}
		}
		//ent->client->showscores = NO_SCOREBOARD;
		ent->client->showscores = SCORE_BOT_VOTE;
		ent->menu = 0;
		DeathmatchScoreboard(ent);
		return true;
	}


	if (ent->client->showscores == SCORE_BOT_REMOVE)
	{
		if (level.modeset == MATCH || level.modeset == PUBLIC)
		{
			switch (ent->menu)
			{
			case 1: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[0]); break;
			case 2: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[1]); break;
			case 3: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[2]); break;
			case 4: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[3]); break;
			case 5: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[4]); break;
			case 6: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[5]); break;
			case 7: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[6]); break;
			case 8: Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[7]); break;
			default: Cmd_VoteRemoveBot_f(ent, true, ""); break; //invalid!!!

			}
		}
		//ent->client->showscores = NO_SCOREBOARD;
		ent->client->showscores = SCORE_BOT_VOTE;
		ent->menu = 0;
		DeathmatchScoreboard(ent);
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_SKILL)
	{
		if (level.modeset == MATCH || level.modeset == PUBLIC)
		{
			switch (ent->menu)
			{
			case 1: menuBotSkill = 0.0; Cmd_VoteSkill_f(ent, 1); break;
			case 2: menuBotSkill = 0.4; Cmd_VoteSkill_f(ent, 1); break;
			case 3: menuBotSkill = 0.8; Cmd_VoteSkill_f(ent, 1); break;
			case 4: menuBotSkill = 1.2; Cmd_VoteSkill_f(ent, 1); break;
			case 5: menuBotSkill = 1.6; Cmd_VoteSkill_f(ent, 1); break;
			case 6: menuBotSkill = 2.0; Cmd_VoteSkill_f(ent, 1); break;
			case 7: menuBotSkill = 2.4; Cmd_VoteSkill_f(ent, 1); break;
			case 8: menuBotSkill = 2.8; Cmd_VoteSkill_f(ent, 1); break;
			case 9: menuBotSkill = 3.2; Cmd_VoteSkill_f(ent, 1); break;
			case 10: menuBotSkill = 3.6; Cmd_VoteSkill_f(ent, 1); break;
			case 11: menuBotSkill = 4.0; Cmd_VoteSkill_f(ent, 1); break;
			//default:Cmd_VoteSkill_f(ent, 1); break;

			}
		}
		ent->client->showscores = NO_SCOREBOARD;
		ent->menu = 0;
		DeathmatchScoreboard(ent);
		return true;
	}

	return false;
}

qboolean ACECM_G_PutAway_f(edict_t *ent)
{

	if (ent->client->showscores == SCORE_BOT_ADD
		|| ent->client->showscores == SCORE_BOT_REMOVE
		|| ent->client->showscores == SCORE_BOT_SKILL)
	{
		ent->client->showscores = SCORE_BOT_VOTE;
		//ent->client->showhelp = false;
		ent->client->showinventory = false;
		ent->client->resp.vote = 0;
		ent->menu = 0;
		DeathmatchScoreboard(ent); //hypov8 add, update scoreboard straight away
		return true;
	}
	return false;
}

qboolean ACECM_G_Use_f(edict_t *ent, char *s)
{
	if (ent->client->showscores == SCORE_BOT_VOTE)
	{
		if (s)
		{

			if (!strcmp(s, "pipe")){
				ent->client->showscores = SCORE_BOT_ADD; ent->menu = 0;		}
			else if (!strcmp(s, "pistol")){
				ent->client->showscores = SCORE_BOT_REMOVE; ent->menu = 0;		}
			else if (!strcmp(s, "shotgun")){
				ent->client->showscores = SCORE_BOT_SKILL; ent->menu = 0;		}
			else if (!strcmp(s, "tommygun")){
				if (ent->client->pers.noantilag)
					Cmd_AntiLag_f(ent, "1");	//ent->client->pers.noantilag = 0;
				else								
					Cmd_AntiLag_f(ent, "0");	//ent->client->pers.noantilag = 1;
				ent->menu = 4;
			}
			else if (!strcmp(s, "heavy machinegun")){
				Cmd_Yes_f(ent); ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;}
			else if (!strcmp(s, "grenade launcher")){
				Cmd_No_f(ent); ent->client->showscores = NO_SCOREBOARD;	ent->menu = 0;}
			else if (!strcmp(s, "bazooka")){
				ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;}
			else if (!strcmp(s, "flamethrower")){
				ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;}

		}


		ent->client->resp.switch_teams_frame = level.framenum;
		DeathmatchScoreboard (ent);
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_ADD)
	{
		if (level.modeset == MATCH || level.modeset == PUBLIC)
		{
			if (s)
			{
				if (!strcmp(s, "pipe"))
					Cmd_VoteAddBot_f(ent, 1);
				else if (!strcmp(s, "pistol"))
					Cmd_VoteAddBot_f(ent, 2);
				else if (!strcmp(s, "shotgun"))
					Cmd_VoteAddBot_f(ent, 3);
				else if (!strcmp(s, "tommygun"))
					Cmd_VoteAddBot_f(ent, 3);
				else if (!strcmp(s, "heavy machinegun"))
					Cmd_VoteAddBot_f(ent, 3);
				else if (!strcmp(s, "grenade launcher"))
					Cmd_VoteAddBot_f(ent, 3);
				else if (!strcmp(s, "bazooka"))
					Cmd_VoteAddBot_f(ent, 3);
				else if (!strcmp(s, "flamethrower"))
					Cmd_VoteAddBot_f(ent, 3);
			}
			//ent->client->showscores = NO_SCOREBOARD;
			ent->client->showscores = SCORE_BOT_VOTE;
			ent->menu = 0;
			DeathmatchScoreboard(ent);
			return true;
		}
	}


	if (ent->client->showscores == SCORE_BOT_REMOVE)
		{
			if (level.modeset == MATCH || level.modeset == PUBLIC)
			{
				if (s)
				{
					if (!strcmp(s, "pipe"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[0]);
					else if (!strcmp(s, "pistol"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[1]);
					else if (!strcmp(s, "shotgun"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[2]);
					else if (!strcmp(s, "tommygun"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[3]);
					else if (!strcmp(s, "heavy machinegun"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[4]);
					else if (!strcmp(s, "grenade launcher"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[5]);
					else if (!strcmp(s, "bazooka"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[6]);
					else if (!strcmp(s, "flamethrower"))
						Cmd_VoteRemoveBot_f(ent, true, VoteBotRemoveName[7]);
				}
				//ent->client->showscores = NO_SCOREBOARD;
				ent->client->showscores = SCORE_BOT_VOTE;
				ent->menu = 0;
				DeathmatchScoreboard(ent);
				return true;
			}
		}


	if (ent->client->showscores == SCORE_BOT_SKILL)
		{
			if (level.modeset == MATCH || level.modeset == PUBLIC)
			{
				if (s)
				{
					if (!strcmp(s, "pipe")){
						menuBotSkill = 0.4;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "pistol")){
						menuBotSkill = 0.8;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "shotgun")){
						menuBotSkill = 1.2;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "tommygun")){
						menuBotSkill = 1.6;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "heavy machinegun")){
						menuBotSkill = 2.0;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "grenade launcher")){
						menuBotSkill = 2.4;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "bazooka")){
						menuBotSkill = 2.8;
						Cmd_VoteSkill_f(ent, 1);
					}
					else if (!strcmp(s, "flamethrower")){
						menuBotSkill = 3.2;
						Cmd_VoteSkill_f(ent, 1);
					}
				}
				ent->client->showscores = NO_SCOREBOARD;
				ent->menu = 0;
				DeathmatchScoreboard(ent);
				return true;
			}
		}
	return false;
}

qboolean ACECM_G_SelectNextItem(edict_t *ent)
{
	if (ent->client->showscores == SCORE_BOT_VOTE)
	{

		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu++;
			if (ent->menu > 7)
				ent->menu = 1;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_ADD)
	{

		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu++;
			if (ent->menu > 3)
				ent->menu = 1;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_REMOVE)
	{

		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu++;
			if (ent->menu > 8)
				ent->menu = 1;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_SKILL)
	{

		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu++;
			if (ent->menu > 11)
				ent->menu = 1;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	return false;
}

qboolean ACECM_G_SelectPrevItem(edict_t *ent)
{
	if (ent->client->showscores == SCORE_BOT_VOTE)
	{
		if (level.framenum >(ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu--;
			if (ent->menu < 1)
				ent->menu = 7;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_ADD)
	{
		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu--;
			if (ent->menu < 1)
				ent->menu = 3;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}

	if (ent->client->showscores == SCORE_BOT_REMOVE)
	{
		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu--;
			if (ent->menu < 1)
				ent->menu = 8;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return true;
	}
	if (ent->client->showscores == SCORE_BOT_SKILL)
	{
		if (level.framenum >(ent->client->resp.scoreboard_frame + 1))
		{
			ent->menu--;
			if (ent->menu < 1)
				ent->menu = 11;
			ent->client->resp.scoreboard_frame = level.framenum;
			DeathmatchScoreboard(ent);
		}
		return true;
	}

	return false;

}





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
	for (i = 1; i<=(int)maxclients->value; i++)
	{
		ent = g_edicts + i;
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

void ACECM_Menu_f(edict_t *ent)
{

	ent->client->showinventory = false;
	//ent->client->showhelp = false; //hypov8 todo?: check this

	if (ent->client->showscores == SCORE_BOT_VOTE
		|| ent->client->showscores == SCORE_BOT_ADD
		|| ent->client->showscores == SCORE_BOT_REMOVE
		|| ent->client->showscores == SCORE_BOT_SKILL)
		ent->client->showscores = NO_SCOREBOARD;
	else
		ent->client->showscores = SCORE_BOT_VOTE;

	DeathmatchScoreboard(ent);
}
// ACEBOT_END

// ACEBOT_ADD
void ACECM_SetBotSkill_f(edict_t *ent, char *value) //add hypov8
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int skill = atoi(value);
		if (Q_stricmp(value, "") == 0){
			cprintf(ent, PRINT_HIGH, "Invalid value. Current botskill is %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
			return;
		}
		switch (skill)
		{
		default:
		case 0: value = "0.0"; break;
		case 1: value = "0.4"; break;
		case 2: value = "0.8"; break;
		case 3: value = "1.2"; break;
		case 4: value = "1.6"; break;
		case 5: value = "2.0"; break;
		case 6: value = "2.4"; break;
		case 7: value = "2.8"; break;
		case 8: value = "3.2"; break;
		case 9: value = "3.6"; break;
		case 10: value = "4.0"; break;
		}

		sv_botskill = gi.cvar_set("sv_botskill", value);
		safe_bprintf(PRINT_HIGH, "Bot-Skill set to %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
		cprintf(ent, PRINT_HIGH, "This setting takes effect immediately\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin, but botskill is %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
}
// ACEBOT_END


///////////////////////////////////////////////////////////////////////
// Special command processor
///////////////////////////////////////////////////////////////////////
qboolean ACECM_Commands(edict_t *ent)
{
	char	*cmd;
	short	node = INVALID;

	//hypo bot wont use console
	if (ent->acebot.is_bot)
		return true;

	cmd = gi.argv(0);

	//client commands
	if (Q_stricmp(cmd, "votebotadd") == 0)
		Cmd_VoteAddBot_f(ent, 0);
	else if (Q_stricmp(cmd, "votebotremove") == 0)
		Cmd_VoteRemoveBot_f(ent, false, NULL);
	else if (Q_stricmp(cmd, "votebotskill") == 0)
		Cmd_VoteSkill_f(ent, false);
	else if (Q_stricmp(cmd, "menu") == 0)
		ACECM_Menu_f(ent);
	else if (Q_stricmp(cmd, "botskill") == 0)
		ACECM_SetBotSkill_f(ent, gi.argv(1));
	//else if (Q_stricmp(cmd, "votebotcount") == 0)		//hypo todo:
	//	Cmd_VoteBotCount_f(ent);  


	if (!debug_mode)
		return false;



	//dev commands
	else if (Q_stricmp(cmd, "addnode") == 0)
	{
		ent->acebot.pm_last_node = ACEND_AddNode(ent, atoi(gi.argv(1)));
		if (!sv_botpath->value)
			ent->acebot.pm_last_node = INVALID; //prevent bug when re'enable sv_botpath
	}
	else if (Q_stricmp(cmd, "removelink") == 0){
		ACEND_RemoveNodeEdge(ent, atoi(gi.argv(1)), atoi(gi.argv(2)));
		return true;
	}
	else if(Q_stricmp (cmd, "addlink") == 0)
		ACEND_UpdateNodeEdge(atoi(gi.argv(1)), atoi(gi.argv(2)), false, false, false, false);
	else if(Q_stricmp (cmd, "showpath") == 0)
    	ACEND_ShowPath(ent,atoi(gi.argv(1)));
	else if (Q_stricmp(cmd, "localnode") == 0) //hypov8 add. show nodes close by
	{
		if (!dedicated->value)
		{
			if (debug_mode_origin_ents == 0){
				safe_cprintf(ent, PRINT_MEDIUM, "findlocalnode ON\n");
				debug_mode_origin_ents = 1;
			}
			else{
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
void ACECM_LevelEnd(void)
{
	ACEND_SaveNodes();
	//num_players = 0;
	botsRemoved = 0;
	//num_bots = 0;
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

