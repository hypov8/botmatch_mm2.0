#include "g_local.h"

char maplist[1024][32];

char admincode[16];		 // the admincode
char default_map[32];    // default settings
char default_teamplay[16];
char default_dmflags[16];
char default_password[16];
char default_timelimit[16];
char default_cashlimit[16];
char default_fraglimit[16];
char default_anti_spawncamp[16];
char default_dm_realmode[16];
char default_bonus[16];
char map_list_filename[32];
int allow_map_voting;
int wait_for_players;
int disable_admin_voting;
int pregameframes;
int num_maps;

int fixed_gametype;
int enable_password;
int keep_admin_status;
int default_random_map;
int disable_curse;

// ACEBOT_ADD
float default_botskill;
// ACEBOT_END
// BEGIN HITMEN
int enable_hitmen;
// END
int unlimited_curse;
int pickup_sounds;
int enable_killerhealth;

char	MOTD[20][80];
int		num_MOTD_lines;

player_t playerlist[64];

char ban_name_filename[32];
ban_t ban_name[100];
int num_ban_names;
char ban_ip_filename[32];
ban_t ban_ip[100];
int num_ban_ips;

char rconx_file[32];
ban_t	rconx_pass[100];
int num_rconx_pass;

int manual_tagset = 0;
int team_startcash[2] = {0, 0};


edict_t *GetAdmin()
{
	int		i;
	edict_t	*doot;

	for_each_player_not_bot(doot, i)// ACEBOT_ADD
	{
		if (doot->client->pers.admin > NOT_ADMIN)
			return doot;
	}
	return NULL;
}



//==============================================================
//
// Papa - This file contains all the functions that control the 
//        modes a server may be in.
//
//===============================================================

/*
================
MatchSetup
level.modeset = MATCHSETUP
teamplay. ONLY used when console command "matchsetup" is used

Places the server in prematch mode
================
*/
void MatchSetup () // Places the server in prematch mode
{
	edict_t		*self;
	int			i;

	if (level.modeset == MATCHSETUP && !level.intermissiontime)
		return;

	level.intermissiontime = 0;

// ACEBOT_ADD
	ACESP_RemoveBot("all", false);
	level.bots_spawned = false;
// ACEBOT_END

	level.modeset = MATCHSETUP;
	level.startframe = level.framenum;

	for_each_player_not_bot(self, i)
	{
		self->client->showscores = SCOREBOARD;
		self->client->resp.scoreboard_frame = 0;
		ClientBeginDeathmatch( self );
	}

	safe_bprintf(PRINT_HIGH, "The server is now ready to setup a match.\n");
	safe_bprintf(PRINT_HIGH, "Players need to join the correct teams.\n");
	
}

qboolean ResetServer (qboolean ifneeded) // completely resets the server including map
{
	char command[64];

	// refresh config
	proccess_ini_file();

	// these things don't need a restart
	if (default_dmflags[0])
		gi.cvar_set("dmflags", default_dmflags);
	if (default_timelimit[0])
		gi.cvar_set("timelimit", default_timelimit);
	if (default_fraglimit[0])
		gi.cvar_set("fraglimit", default_fraglimit);
	if (default_cashlimit[0])
		gi.cvar_set("cashlimit", default_cashlimit);
	if (default_anti_spawncamp[0])
		gi.cvar_set("anti_spawncamp", default_anti_spawncamp);
	if (default_bonus[0])
		gi.cvar_set("bonus", default_bonus);
	gi.cvar_set("password", default_password);
	gi.cvar_set("no_spec", "0");
	if (manual_tagset)
	{
		manual_tagset = 0;
		setTeamName(1, "Dragons");
		setTeamName(2, "Nikki's Boyz");
	}
// ACEBOT_ADD
	if (default_botskill != -1 && default_botskill != sv_botskill->value)
	{
		gi.cvar_set("sv_botskill", va("%1.1f", default_botskill));
		gi.dprintf("Bot-Skill set to %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
	}
// ACEBOT_END

	// these do
	if (ifneeded
		&& !(default_teamplay[0] && strcmp(teamplay->string, default_teamplay))
		&& !(default_dm_realmode[0] && strcmp(dm_realmode->string, default_dm_realmode))
		&& !(enable_hitmen && (int)sv_hitmen->value != enable_hitmen)
		)
		return false;

// BEGIN HITMEN
	if (enable_hitmen)
		sv_hitmen =	gi.cvar_set("hitmen", "1");
// END

	if (default_teamplay[0])
		gi.cvar_set("teamplay", default_teamplay);
	if (default_dm_realmode[0])
		gi.cvar_set("dm_realmode", default_dm_realmode);
	gi.cvar_set("cheats", "0");


	if (default_random_map && num_maps)
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", maplist[rand() % num_maps]);
	else
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", default_map[0] ? default_map : level.mapname);
	gi.AddCommandString (command);
	return true;
}

/*
================
MatchStart
level.modeset = TEAM_PRE_MATCH;
ONLY server command "matchstart"

start the match
================
*/
void MatchStart()  // start the match
{
	int			i;
	edict_t		*ent;

// ACEBOT_ADD
	ACESP_RemoveBot("all", false);
	level.bots_spawned = false;
// ACEBOT_END
		
	level.intermissiontime = 0;
	level.player_num = 0;
	level.modeset = MATCHCOUNT;
	level.startframe = level.framenum;
	safe_bprintf(PRINT_HIGH, "COUNTDOWN STARTED. 15 SECONDS TO MATCH.\n");
	team_cash[1] = team_startcash[0];
	team_cash[2] = team_startcash[1];
	team_startcash[1] = team_startcash[0] = 0;
	UpdateScore();

	G_ClearUp ();

	for_each_player_inc_bot(ent, i)
	{
		ent->client->resp.idle = level.framenum;
		ent->client->resp.time = 0;
		ent->client->resp.scoreboard_frame = 0;
		ent->client->pers.bagcash = 0;
		ent->client->resp.deposited = 0;
		ent->client->resp.stole = 0;
		ent->client->resp.score = 0;
		ent->client->pers.currentcash = 0;
		ent->client->resp.acchit = ent->client->resp.accshot = 0;
		memset(ent->client->resp.fav, 0, sizeof(ent->client->resp.fav));
		if (ent->client->pers.spectator == PLAYING)
			ent->client->showscores = NO_SCOREBOARD;
		ClientBeginDeathmatch( ent );
	}

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play world/cypress3\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);

	// turn back on any sounds that were turned off during intermission
	for (i=0; i<globals.num_edicts; i++)
	{
		ent = g_edicts + i;
		if (ent->inuse && (ent->spawnflags&1) && ent->classname && !strcmp(ent->classname, "target_speaker"))
			ent->s.sound = ent->noise_index;
	}
}


void SpawnPlayers ()  // spawn players - 2 per server frame (1 per team) in hopes of reducing overflows
{
	edict_t		*self;
	int			i, c;
	int			team1,team2;

	team1 = false;
	team2 = false;

	for (c=i=0; i<(int)maxclients->value; i++)
	{
		self = g_edicts + 1 + i;
		if (!self->inuse || self->client->resp.is_spawn)
			continue;
		if (self->client->pers.spectator == SPECTATING)
			continue;

// ACEBOT_ADD				
			if (self->acebot.is_bot)
				continue;//hypov8 todo: check this
// ACEBOT_END	

		if (teamplay->value)
		{
			if ((self->client->pers.team == 1) && (!team1))
			{
				team1 = true;
				ClientBeginDeathmatch( self );
				self->client->resp.is_spawn = true;
				if (++c == 2) break;
			}
			if ((self->client->pers.team == 2) && (!team2))
			{
				team2 = true;
				ClientBeginDeathmatch( self );
				self->client->resp.is_spawn = true;
				if (++c == 2) break;
			}
		}
		else
		{

			ClientBeginDeathmatch( self );
			self->client->resp.is_spawn = true;
			if (++c == 2) break;
		}
	}
	if (!c)
		level.is_spawn = true;
}
/*
================
Start_TeamMatch
level.modeset = TEAM_MATCH_SPAWNING;

game will now load clients as active
================
*/
void Start_Match () // Starts the match
{
	edict_t		*self;
	int			i;

	level.modeset = MATCHSPAWN;
	level.startframe = level.framenum;
	level.is_spawn = false;
	for_each_player_not_bot(self, i)
	{
		safe_centerprintf(self, "The match has begun!");
		self->client->resp.is_spawn = false;
	}
	gi.dprintf("The match has begun!\n");

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play world/pawnbuzz_out\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);
}

/*
================
Start_FFA
level.modeset = DM_MATCH_SPAWNING;

starting team game
================
*/
void Start_Pub () // Starts a public game
{
	edict_t		*self;
	int			i;

	level.modeset = PUBLICSPAWN;
	level.startframe = level.framenum;
	level.is_spawn = false;
	for_each_player_not_bot(self, i)
	{
		safe_centerprintf(self, "Let the fun begin!");
		self->client->resp.is_spawn = false;
	}
	gi.dprintf("Let the fun begin!\n");

	gi.WriteByte(svc_stufftext);
	gi.WriteString("play world/pawnbuzz_out\n");
	gi.multicast(vec3_origin, MULTICAST_ALL);
}

/*
================
SetupMapVote
ENDMATCHVOTING
game ended. vote
================
*/
void SetupMapVote () // sets up the vote options for the next map
{
	int		i, j, k;
	int		unique;
	int		selection;

// ACEBOT_ADD
	//ACECM_LevelEnd();
// ACEBOT_END

	// find current map index
	i = 0;
	level.vote_set[0] = -1;
	while (i < num_maps) 
	{	
		if (Q_stricmp (maplist[i], level.mapname) == 0)
		{
			level.vote_set[0] = i;
			break;
		}
		i++;
	}

	if (num_maps < 9) // less than 9 maps found, just display them all
	{
		i = level.vote_set[0];
		for (j=1; j<=num_maps; j++)
		{
			i++;
			if (i == num_maps)
				i=0;
			level.vote_set[j] = i;
		}
		level.num_vote_set = num_maps;
		return;
	}

	for (i=1; i<9; i++)
	{
		unique = false;
		while (!unique)
		{		
			selection = rand() % num_maps;
			level.vote_set[i] = selection;
			unique = true;
			for (k=0; k<i; k++)
				if (level.vote_set[i] == level.vote_set[k])
				{
					unique = false;
					break;
				}
		}
	}

	level.num_vote_set = 8;
}

/*
================
MatchEnd
level.modeset =  ENDMATCHVOTING?
ONLY used for server cmd "matchend"
move players to spec
================
*/
void MatchEnd () // end of the match
{
	level.modeset = MATCHSETUP;
	level.startframe = level.framenum;

	BeginIntermission(NULL);
}

/*
================
CheckAllPlayersSpawned
TEAM_MATCH_SPAWNING = level.modeset = TEAM_MATCH_RUNNING
DM_MATCH_SPAWNING = level.modeset = DM_MATCH_RUNNING;

when starting a match this function is called until all the players are in the game
then sets mode dm or bm
================
*/
void CheckAllPlayersSpawned () // when starting a match this function is called until all the players are in the game
{
	level.startframe = level.framenum; // delay clock until all players have spawned

	SpawnPlayers ();
	if (level.is_spawn)
	{
		if (level.modeset == MATCHSPAWN)
			level.modeset = MATCH;
		else
			level.modeset = PUBLIC;
	}
}

/*
========
CheckIdleMatchSetup
command "matchsetup"
========
*/
void CheckIdleMatchSetup () // restart the server if its empty in matchsetup mode
{
	int		count = 0;
	int		i;
	edict_t	*doot;

	for_each_player_not_bot(doot, i)
	{ //hypov8 bots count??
		count++;
	}
	if (count == 0)
		ResetServer (false);
}

/*
================
CheckStartTeamMatch
calls Start_TeamMatch() when time is up
level.modeset = TEAM_MATCH_SPAWNING;

15 countdown before matches
================
*/
void CheckStartMatch () // 15 countdown before matches
{
	int framenum = level.framenum - level.startframe;
	
	if (framenum >= 150)
	{
		Start_Match ();
		return;
	}
	
	if ((framenum % 10 == 0) && (framenum > 99))
	{
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play world/pawnomatic/menubuzz\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
}


/*
================
CheckStartDM
calls Start_DM()
->level.modeset = DM_MATCH_SPAWNING;

35 second countdown before server starts
================
*/
void CheckStartPub () // 35 second countdown before server starts
{

	if (level.framenum >= level.pregameframes)
	{
		Start_Pub ();	
		return;
	}

	if (level.pregameframes - level.framenum <= 40 && !((level.pregameframes - level.framenum) % 10))
	{
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play world/pawnomatic/menubuzz\n");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
}

void getTeamTags();

void CheckEndMatch () // check if time,frag,cash limits have been reached in a match
{
	int		i;
	int		count = 0;
	edict_t	*doot;

    // snap - team tags
	if (!manual_tagset && (level.framenum % 100) == 0)
		getTeamTags();

	for_each_player_inc_bot(doot, i)
		count++;
		
// ACEBOT_ADD
	if (count && !level.bots_spawned)
	{
		ACEND_InitNodes();
		ACEND_LoadNodes();
		ACESP_LoadBots();
		level.bots_spawned = true;
	}
// ACEBOT_END
		
		
	if (count == 0)
	{
		ResetServer (false);
		return;
	}

	if ((int)teamplay->value == 1)
	{
		if ((int)cashlimit->value)
		{
			if ((team_cash[1] >= (int)cashlimit->value) || (team_cash[2] >= (int)cashlimit->value))
			{
			safe_bprintf(PRINT_HIGH, "Cashlimit hit.\n");
				MatchEnd ();
				return;
			}
		}
	}
	else if ((int)fraglimit->value)
	{
		if (team_cash[1] >= (int)fraglimit->value || team_cash[2] >= (int)fraglimit->value)
		{
			safe_bprintf(PRINT_HIGH, "Fraglimit hit.\n");
			MatchEnd ();
			return;
		}
	}

	if ((int)timelimit->value)
	{
		if (level.framenum > (level.startframe + ((int)timelimit->value) * 600 - 1))
		{
			safe_bprintf(PRINT_HIGH, "Timelimit hit.\n");
			MatchEnd();
			return;
		}
		if (((level.framenum - level.startframe ) % 10 == 0 ) && (level.framenum > (level.startframe + (((int)timelimit->value  * 600) - 155))))  
		{
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d seconds\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 10);
			return;
		}
		if (((level.framenum - level.startframe ) % 600 == 0 ) && (level.framenum > (level.startframe + (((int)timelimit->value * 600) - 3000))))  
		{
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
			return;
		}
		if ((((int)timelimit->value * 600) - (level.framenum - level.startframe) ) % 3000 == 0 )
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
	}
}

void CheckEndVoteTime () // check the timelimit for voting next level/start next map
{
	int		i, count = 0, votes[9];
	edict_t *player;

	if (level.vote_winner)
	{
		if (level.framenum > (level.startframe + 310))
			level.exitintermission = true;
		return;
	}

	memset (&votes, 0, sizeof(votes));

	for_each_player_not_bot(player,i)// ACEBOT_ADD
	{
		count++;
		votes[player->client->mapvote]++;
	}
	if (!count && level.framenum > (level.lastactive + 30))
	{
		if (ResetServer(true))
			return;
		if (wait_for_players)
		{
			level.startframe = level.framenum;
			level.player_num = 0;
			if (team_cash[1] || team_cash[2])
			{
				team_cash[2] = team_cash[1] = 0;
				UpdateScore();
			}
			level.lastactive = -1;
			gi.dprintf("Waiting for players\n");
			UpdateTime();
			if (kpded2) // enable kpded2's idle mode for reduced CPU usage while waiting for players (automatically disabled when players join)
				gi.cvar_forceset("g_idle", "1");
		}
	}

	if (level.framenum == (level.startframe + 300))
	{
		level.vote_winner = 1;
		for (i = 2; i <= level.num_vote_set; i++)
		{
			if (votes[i] > votes[level.vote_winner])
				level.vote_winner = i;
		}
		level.changemap = maplist[level.vote_set[level.vote_winner]];
	}
}

void CheckEndTime() // HYPOV8 todo: check this
{
	if (level.framenum > (level.startframe + 200))
		level.exitintermission = true;
}

void CheckVote() // check the timelimit for an admin or map vote
{
	if (level.framenum == (level.voteframe + 600))
		safe_bprintf(PRINT_HIGH, "30 seconds left for voting\n");
	if (level.framenum >= (level.voteframe + 900))
	{
		switch (level.voteset)
		{
			case VOTE_ON_ADMIN:
				safe_bprintf(PRINT_HIGH, "The request for admin has failed\n");
				break;
			case VOTE_ON_MAP:
				safe_bprintf(PRINT_HIGH, "The request for a map change has failed\n");
				break;
		}
		level.voteset = NO_VOTES;
	}
}

int	CheckNameBan (char *name)
{
	char n[64];
	int i;

	strncpy(n, name, sizeof(n)-1);
	kp_strlwr(n);
	for (i=0; i<num_ban_names; i++)
	{
		if (strstr(n, ban_name[i].value))
			return true;
	}
	return false;
}

int	CheckPlayerBan (char *userinfo)
{
	char	*value;
	int		i;

	if (num_ban_names)
	{
		value = Info_ValueForKey (userinfo, "name");
		if (CheckNameBan(value))
			return true;
	}

	if (num_ban_ips)
	{
		value = Info_ValueForKey (userinfo, "ip");
		for (i=0; i<num_ban_ips; i++)
			if (!strncmp(value, ban_ip[i].value, strlen(ban_ip[i].value)))
				return true;
	}

	return false;
}

void UpdateTeams()
{
	char buf[48];
	sprintf(buf,"%s : %s", team_names[1], team_names[2]);
	gi.cvar_set(TEAMNAME, buf);
}

void UpdateScore()
{
	char buf[20];
	sprintf(buf,"%d : %d", team_cash[1], team_cash[2]);
	gi.cvar_set(SCORENAME, buf);
}

void UpdateTime()
{
	char buf[32] = " ";
	if (level.lastactive < 0)
		strcpy(buf, "waiting");
	else if (level.modeset == MATCHCOUNT)
	{
		int t =	((150 - (level.framenum - level.startframe)) / 10);
		sprintf(buf, "start in %d", t);
	}
	else if (level.modeset == PREGAME)
	{
		int t = ((350 -  level.framenum) / 10);
		sprintf(buf, "start in %d", t);
	}
	else if ((level.modeset == MATCH || level.modeset == PUBLIC))
	{
		if ((int)timelimit->value)
		{
			int t = ((((int)timelimit->value * 600) + level.startframe - level.framenum ) / 10);
			if (t > 0)
				sprintf(buf, "%d:%02d", t / 60, t % 60);
		}
	}
	else if (level.intermissiontime) //hypov8 ToDo: timmer
		strcpy(buf, "intermission");
	gi.cvar_set(TIMENAME, buf);
}

/////////////////////////////////////////////////////
// snap - team tags
void setTeamName (int team, char *name) // tical's original code :D
{ 
	static int team_alloc[3] = {0, 0, 0};

	if (!name || !*name)
		return;

	if (strcmp(name, team_names[team])) 
	{
		if (team_alloc[team])
			gi.TagFree(team_names[team]);

		team_names[team] = strcpy(gi.TagMalloc(strlen(name) + 1, TAG_GAME), name);
		team_alloc[team] = 1;
	}
}

// snap - new function.
void getTeamTags()
{

	int			i;
	edict_t		*doot;
	char		names[2][64][16];
	int			namesLen[2] = { 0, 0 };
	char		teamTag[2][12];
	int			teamTagsFound[2]= { FALSE, FALSE };

	for_each_player_inc_bot(doot, i)
	{
		int team = doot->client->pers.team;
		if (team && namesLen[team-1] < 64)
		{
			strcpy(names[team-1][namesLen[team-1]++], doot->client->pers.netname);
		}
	}


	for(i=0; i<2; i++)
	{
		int	j;
		for (j=0; j<namesLen[i] && teamTagsFound[i] == FALSE; j++)
		{
			int	k;
			for (k=0; k<namesLen[i] && j != k && teamTagsFound[i] == FALSE; k++)
			{
				char theTag[12];
				int	theTagNum = 0;
				int	y = 0;
				char s = names[i][j][y];
					
				while (s != '\0' && theTagNum == 0)
				{
					int	z = 0;
					char t = names[i][k][z];
					while (t != '\0')
					{
						if (s == t && s != ' ')
						{ // we have a matched char
							int	posY = y+1;
							int	posZ = z+1;
							char ss = names[i][j][posY];
							char tt = names[i][k][posZ];

							while (ss != '\0' && tt != '\0' && ss == tt && theTagNum < 11)
							{
								if (theTagNum == 0)
								{ // we have two consecutive matches, this is a tag
									theTag[theTagNum++] = s;
									theTag[theTagNum++] = ss;
								}
								else
								{
									theTag[theTagNum++] = ss;
								}
								ss = names[i][j][++posY];
								tt = names[i][k][++posZ];
							}
						}
						t = names[i][k][++z];
					}
					s = names[i][j][++y];
				}
				if (theTagNum > 0)
				{
					int	e;
					float howmany = 0.0;
					float percentage; 
					theTag[theTagNum] = '\0';
					
					for (e=0; e<namesLen[i]; e++)
					{
						if (strstr(names[i][e], theTag) != NULL)
						{
							howmany += 1.0;
						}
					}
					percentage = howmany/(float)namesLen[i]*100.0;
					if (percentage > 75.0)
					{
						strcpy(teamTag[i], theTag);
						teamTagsFound[i] = TRUE;
					}	
				}
			}
		}
	}

	setTeamName(1, teamTagsFound[0] == TRUE ? teamTag[0] : "Dragons");
	setTeamName(2, teamTagsFound[1] == TRUE ? teamTag[1] : "Nikki's Boyz");
	UpdateTeams();
}
