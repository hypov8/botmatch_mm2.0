#include "g_local.h"
#include "m_player.h"

#include "voice_punk.h"
#include "voice_bitch.h"

extern int manual_tagset;
extern int team_startcash[2];

char votemap[32];
// BEGIN HOOK
void Cmd_Hook_f (edict_t *ent);
extern int HmHookAvailable;
extern int HmStatTime;
// END

// ACEBOT_ADD
char changeMapName[32]; // = '\0';

char voteAddBot[32]; //team
char voteRemoveBot[32]; //name
float voteBotSkill; //skill 0.0 to 4.0
static void Cmd_VoteRemoveBot_f(edict_t *ent, qboolean isMenu, char botnames[32]);
static void Cmd_VoteAddBot_f(edict_t *ent, int teamUse);
static void Cmd_VoteSkill_f(edict_t *ent, qboolean skill);
void Cmd_Yes_f(edict_t *ent);
void Cmd_No_f(edict_t *ent);
void EndDMLevel(void); //hypov8 add
void Cmd_AntiLag_f(edict_t *ent, char *value); //hypov8 add
// ACEBOT_END


//tical - define taunts
#define KINGPIN		1
#define LEROY		2
#define MJ			3
#define MOMO		4
#define LAMONT		5
#define JESUS		6
#define TYRONE		7
#define WILLY		8
#define MOKER		9
#define HEILMAN		10

#define BAMBI		11
#define YOLANDA		12
#define MONA		13
#define LOLA		14
#define BLUNT		15
#define BETH		16

//------------------------------------------------------------------------------------


//--------------------------------------------------------
// TEAMPLAY commands

// Papa - add player to specator
void Cmd_Spec_f (edict_t *self)
{
	if (self->client->pers.spectator == SPECTATING)
		return;
// ACEBOT_ADD
	if (self->acebot.is_bot) /* hypov8 added so it dont check bots for idle issues(no nodes etc..) */
		return;
// ACEBOT_END

	if (self->solid != SOLID_NOT) DropCash(self);

	if (teamplay->value && self->client->pers.team)
		safe_bprintf (PRINT_HIGH, "%s left %s\n", self->client->pers.netname, team_names[self->client->pers.team]);
	else
		safe_bprintf (PRINT_HIGH, "%s became a spectator\n", self->client->pers.netname);

	self->client->pers.team = 0;
	self->client->pers.spectator = SPECTATING;
	if (self->solid != SOLID_NOT || self->movetype == MOVETYPE_NOCLIP) // HYPOV8_ADD noclip todo: check?
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (self - g_edicts);
		gi.WriteByte (MZ_LOGOUT);
		gi.multicast (self->s.origin, MULTICAST_PVS);

// ACEBOT_ADD	
	ACEIT_PlayerRemoved(self);
// ACEBOT_END
		ClientBeginDeathmatch( self );
		if (!self->client->showscores) Cmd_Score_f(self);
		self->client->resp.scoreboard_frame = 0;
	}
}

// Papa - add plater to a team
void Cmd_Join_f (edict_t *self, char *teamcmd)
{
	int	i;
	char str1[MAX_QPATH], varteam[MAX_QPATH];

	self->client->resp.check_idle = level.framenum;

	if (!teamplay->value)
	{
		if (self->client->pers.spectator != SPECTATING)
			return;
		if (self->client->resp.switch_teams_frame && level.framenum < (self->client->resp.switch_teams_frame + 20))
		{
			cprintf(self, PRINT_HIGH, "Overflow protection: Unable to rejoin yet\n");
			return;
		}
		self->client->resp.switch_teams_frame = level.framenum;
		self->client->pers.spectator = PLAYING;
		ClientBeginDeathmatch( self );
		return;
	}

	strcpy( varteam, teamcmd );

	// search for the team-name

	if (varteam[0])
	{
		for (i=1; team_names[i]; i++)
		{
			strcpy(str1, team_names[i]);
			kp_strlwr(str1);
			kp_strlwr(varteam);

			if (strstr( str1, varteam ) == str1)
			{	// found a match

				if (self->client->pers.team == i)
				{
					cprintf( self, PRINT_HIGH, "Already a member of %s\n", team_names[i] );
				}
				else
				{
					if (self->client->resp.switch_teams_frame && level.framenum < (self->client->resp.switch_teams_frame + 20))
					{
						cprintf(self, PRINT_HIGH, "Overflow protection: Unable to change team yet\n");
						return;
					}
					self->client->resp.switch_teams_frame = level.framenum;

					if (self->client->pers.team)
						Cmd_Spec_f (self);

					if (!Teamplay_ValidateJoinTeam( self, i ))
					{
						cprintf( self, PRINT_HIGH, "Unable to join %s\n", team_names[i] );
					}
				}

				return;
			}
		}

		cprintf( self, PRINT_HIGH, "Un-matched team: %s\n", varteam );
	}
}

//--------------------------------------------------------
void Cmd_GearUp_f (edict_t *self)
{
	vehicle_t *vehicle;

	if (!self->vehicle_index)
	{
		cprintf( self, PRINT_HIGH, "You aren't in a vehicle, can't change gears.\n");
		return;
	}

	vehicle = &global_vehicles[self->vehicle_index - 1];

	vehicle->gear++;
	if (vehicle->gear == vehicle->def->gearbox->num_gears)
		vehicle->gear--;

}

void Cmd_GearDown_f (edict_t *self)
{
	vehicle_t *vehicle;

	if (!self->vehicle_index)
	{
		cprintf( self, PRINT_HIGH, "You aren't in a vehicle, can't change gears.\n");
		return;
	}

	vehicle = &global_vehicles[self->vehicle_index - 1];

	vehicle->gear--;
	if (vehicle->gear < 0)
		vehicle->gear = vehicle->def->gearbox->num_gears - 1;
}

//-------------------------------------------------------------------------
// Generic "3-key" system

// JOSEPH 8-FEB-99
edict_t	*GetKeyEnt( edict_t *ent )
{
	vec3_t	dir;
	vec3_t	start, end;
	trace_t	tr;

	AngleVectors( ent->client->ps.viewangles, dir, NULL, NULL );

	VectorCopy( ent->s.origin, start );
	start[2] += ent->viewheight;

	if (deathmatch_value)
		VectorMA( start, 4000, dir, end );
	else
		VectorMA( start, 384, dir, end );

	tr = gi.trace( start, NULL, NULL, end, ent, MASK_SHOT );

	if ((tr.fraction < 1) && ((deathmatch_value && tr.ent->client) || (tr.ent->svflags & SVF_MONSTER)))
	{
		return tr.ent;
	}

	return NULL;
}
// END JOSEPH

void Cmd_Wave_f (edict_t *ent, edict_t *other, int who);

void Cmd_Key_f (edict_t *ent, int who)
{
	edict_t *key_ent;

	if (disable_curse) 
		return;

	if (ent->client->pers.spectator == SPECTATING) 
		return;
	
	if (level.speaktime > level.time)
		return;
	
	if (unlimited_curse) 
		key_ent = NULL;
	else if ((key_ent = GetKeyEnt(ent)) == NULL) 
		return; // HYPOV8_ADD !=0

	Cmd_Wave_f( ent, key_ent, who );

	if (unlimited_curse)
		level.speaktime = level.time + 7;
}

//-------------------------------------------------------------------------

// Ridah, Chasecam
void Cmd_ToggleCam_f ( edict_t *ent )
{
	if (ent->flags & FL_CHASECAM)
	{
		ent->flags -= FL_CHASECAM;
	}
	else
	{
		ent->flags += FL_CHASECAM;

//		safe_centerprintf( ent, "Chasecam is incomplete, and therefore\nunsupported at this stage\n" );
#ifdef HYPODEBUG
		if ((level.modeset == MATCH || level.modeset == PUBLIC)
			&& ent->client->pers.spectator == PLAYING)
		ent->flags |= FL_CHASECAM; //hypov8. enabled togglecam again
#endif
	}
}
// done.

// ==============================================================================

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];
	
	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
// Ridah, disabled this, teams are determined by model and skin (since that's the only way to make sure they appear the same
//		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (teamplay->value)
	{
		if (ent1 && ent2 && ent1->client && ent2->client && ent1->client->pers.team && (ent1->client->pers.team == ent2->client->pers.team))
			return true;
		else
			return false;
	}

	if (!((int)(dmflags->value) & (DF_MODELTEAMS /*| DF_SKINTEAMS*/)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;

}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;

// Papa - this is used to move down the vote map menu

	if (ent->client->showscores == SCORE_MAP_VOTE)
	{
		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->client->mapvote++;
			if (ent->client->mapvote == num_vote_set+1)
				ent->client->mapvote = 1;
			ent->client->resp.scoreboard_frame = 0;
		}
		return;
	}
// ACEBOT_ADD //MENU
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
		return;
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
		return;
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
		return;
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
		return;
	}

// ACEBOT_END //MENU
//hypov8 todo?: check this
/*	if (ent->client->showscores == SCORE_REJOIN)
	{
		if (level.framenum > (ent->client->resp.switch_teams_frame + 4))
		{
			if (ent->vote == 0)
				ent->vote = 1;
			else
				ent->vote = 0;
			ent->client->resp.switch_teams_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return;
	}
*/
	cl = ent->client;
	
	if (cl->chase_target)
	{
		ChaseNext(ent);
		return;
	}

	Cmd_Help_f (ent, 1);
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;

// Papa - this is used to move up the vote map menu

	if (ent->client->showscores == SCORE_MAP_VOTE)
	{
		if (level.framenum > (ent->client->resp.scoreboard_frame + 1))
		{
			ent->client->mapvote--;
			if (ent->client->mapvote < 1)
				ent->client->mapvote = num_vote_set;
			ent->client->resp.scoreboard_frame = 0;
		}
		return;
	}
// ACEBOT_ADD //MENU
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
		return;
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
		return;
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
		return;
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
		return;
	}

// ACEBOT_END //MENU
//hypov8 todo?: check this
/*
	if (ent->client->showscores == SCORE_REJOIN)
	{
		if (level.framenum > (ent->client->resp.switch_teams_frame + 4))
		{
			if (ent->vote == 0)
				ent->vote = 1;
			else
				ent->vote = 0;
			ent->client->resp.switch_teams_frame = level.framenum;
			DeathmatchScoreboard (ent);
		}
		return;
	}
*/
	cl = ent->client;

	if (cl->chase_target)
	{
		ChasePrev(ent);
		return;
	}

	Cmd_Help_f (ent, 1);
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (!developer->value)
		return;

	if (deathmatch_value && !sv_cheats->value)
	{
		cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	// JOSEPH 15-FEB-99
	if (Q_stricmp(gi.argv(1), "cash") == 0)
	{
		if (gi.argc() == 3)
			ent->client->pers.currentcash += atoi(gi.argv(2));
		else
			ent->client->pers.currentcash += 100;

		gi.sound (ent, CHAN_AUTO, gi.soundindex("world/pickups/cash.wav"), 1, ATTN_NORM, 0);
		
		if (!give_all)
			return;
	}
	// END JOSEPH
	
	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp (gi.argv(1), "mods") == 0)
	{
		ent->client->pers.pistol_mods |= WEAPON_MOD_ROF;
		
		ent->client->pers.pistol_mods |= WEAPON_MOD_RELOAD;
		
		ent->client->pers.pistol_mods |= WEAPON_MOD_DAMAGE;
		
		ent->client->pers.pistol_mods |= WEAPON_MOD_COOLING_JACKET;

		ent->client->pers.hmg_shots = 30;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			
			// ent->client->pers.inventory[i] += 1;
			ent->client->pers.inventory[i] = 1;
			if (it->flags & IT_SILENCER)
			{
				ent->client->pers.silencer_shots = 20;
			}
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			int item_index = 0;

			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			if (it->flags & IT_NOCHEATS)
				continue;
			Add_Ammo (ent, it, 1000);

			item_index = QweryClipIndex(it);
			if (item_index > 0)
				ent->client->pers.weapon_clip[item_index] = auto_rounds[item_index];

		}
		if (!give_all)
			return;
	}

	// JOSEPH 30-APR-99
	if (Q_stricmp(name, "armor") == 0)
	{
		gitem_t	*it;

		it = FindItem("Jacket Armor Heavy");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 100;

		it = FindItem("Legs Armor Heavy");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 100;

		it = FindItem("Helmet Armor Heavy");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 100;

			return;
	}
	// END JOSEPH

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			if (it->flags & IT_NOCHEATS)
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			cprintf (ent, PRINT_HIGH, "not a valid item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it->flags & IT_SILENCER)
			ent->client->pers.silencer_shots = 20;
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
		
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (!developer->value)
		return;

	if (deathmatch_value && !sv_cheats->value)
	{
		cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "Immortal OFF\n";
	else
		msg = "Immortal ON\n";

	cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (!developer->value)
		return;

	if (deathmatch_value && !sv_cheats->value)
	{
		cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (!developer->value)
		return;

	if (deathmatch_value && !sv_cheats->value)
	{
		cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}
// HYPOV8_ADD
	if (!(level.modeset == MATCH || level.modeset == PUBLIC))
		return;

	//add hypov8 stop noclip working if allready a spectator
	if 	(ent->client->pers.spectator == SPECTATING)
		return;
// HYPOV8_END
	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
		ent->solid = SOLID_BBOX; // HYPOV8_ADD
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
		ent->solid = SOLID_NOT; // HYPOV8_ADD
// ACEBOT_ADD noclip fix
		ent->acebot.pm_last_node = INVALID;
// ACEBOT_END
	}

	cprintf (ent, PRINT_HIGH, msg);
}

// ACEBOT_ADD
static void build_bot_list()
{
	int i,j,botCount,outCount;
	edict_t	*bot;

	botCount = ACESP_LoadRandomBotCFG();
	if (botCount > 0)
	{
		outCount = 0;
		for (i = 0; i <= botCount; i++)
		{
			for_each_player_inc_bot(bot, j)
			{
				//if (!bot->acebot.is_bot) continue; //hypo allow clients to use bot names?
				if (Q_stricmp(randomBotSkins[i].name, bot->client->pers.netname) == 0)
					continue;
			}
			outCount++;
			strcpy(VoteBotRemoveName[0], randomBotSkins[i].name);
		}
		
	}
}
// ACEBOT_END
/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	gitem_t		*it;
	char		*s;	
	int			index;

	s = gi.args();

// Papa
// Kingpin uses whatever key your weapon is bound to, to work its menus
// So I kept with this format for the menus that I added

    if (ent->client->showscores == SCORE_MAP_VOTE)  // next map vote menu
	{
		if (s && level.framenum >= (ent->client->resp.scoreboard_frame + 2))
		{
			int vote=0;
			if (!strcmp(s, "pipe"))
				vote = 1;
			if (!strcmp(s, "pistol"))
				vote = 2;
			if (!strcmp(s, "shotgun"))
				vote = 3;
			if (!strcmp(s, "tommygun"))
				vote = 4;
			if (!strcmp(s, "heavy machinegun"))
				vote = 5;
			if (!strcmp(s, "grenade launcher"))
				vote = 6;
			if (!strcmp(s, "bazooka"))
				vote = 7;
			if (!strcmp(s, "flamethrower"))
				vote = 8;
			if (vote > 0 && vote <= num_vote_set)
			{
				ent->client->mapvote = vote;
				ent->client->resp.scoreboard_frame = 0;
			}
		}
		return;
	}
// ACEBOT_ADD
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
				Cmd_Yes_f(ent); ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;			}
			else if (!strcmp(s, "grenade launcher")){
				Cmd_No_f(ent); ent->client->showscores = NO_SCOREBOARD;	ent->menu = 0;			}
			else if (!strcmp(s, "bazooka")){
				ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;		}
			else if (!strcmp(s, "flamethrower")){
				ent->client->showscores = NO_SCOREBOARD; ent->menu = 0;		}

			}


		ent->client->resp.switch_teams_frame = level.framenum;
		DeathmatchScoreboard (ent);
		return;
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
			return;
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
				return;
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
				return;
			}
		}
// ACEBOT_END
			
	if (ent->client->showscores == SCORE_REJOIN) // restores players frags, time, etc after they disconnect
	{
		if (s)
		{
			if (!strcmp(s, "pipe"))
				ClientRejoin(ent, true);
			else if (!strcmp(s, "pistol"))
				ClientRejoin(ent, false);
		}
		return;
	}
	else if (teamplay->value && (!ent->client->pers.team || level.modeset == MATCHSETUP || level.modeset == PREGAME))
	{
		if (s)
		{
			if (!strcmp(s, "pipe"))
			{	// Kings
				Cmd_Join_f( ent, team_names[1] );
			}
			else if (!strcmp(s, "pistol"))
			{	// Pins
				Cmd_Join_f( ent, team_names[2] );
			}
		}
		return;
	}

	if (ent->client->pers.spectator == SPECTATING)
	{
		if (!teamplay->value) Cmd_Join_f( ent, "" );
		return;
	}

	if (ent->solid == SOLID_NOT) return;

	it = FindItem (s);
	if (!it)
	{
		cprintf (ent, PRINT_HIGH, "not a valid item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		
		if (strcmp (it->pickup_name, "Pistol") == 0)
		{
		//	gi.dprintf ("silencer_shots: %d\n", ent->client->pers.silencer_shots);
			if (ent->client->pers.silencer_shots <= 0) //hypov8 was 0
			{
				cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
			it = FindItem ("SPistol");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
			return;
		}
	}

	it->use (ent, it);

	if (!ent->client->pers.weapon && ent->client->newweapon)
	{
		ChangeWeapon (ent);
	}
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	if (ent->solid == SOLID_NOT) return;

	//hypov8 todo: dont dro in spec

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		cprintf (ent, PRINT_HIGH, "not a valid item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}

	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		if (strcmp (it->pickup_name, "Pistol") == 0)
		{
			//gi.dprintf ("silencer_shots: %d\n", ent->client->pers.silencer_shots);

			if (ent->client->pers.silencer_shots <= 0)
			{
				cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
			
			it = FindItem ("SPistol");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
			return;
		}
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = NO_SCOREBOARD;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon)
		return;

	// Ridah, if already changing weapons, start from the next weapon, for faster cycling
	if (ent->client->weaponstate == WEAPON_DROPPING)
		selected_weapon = ITEM_INDEX(cl->newweapon);
	else
		selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		if (selected_weapon == ITEM_INDEX(it) && cl->newweapon)
		{
			// Ridah, show the current weapon on the hud, for easy scrolling
			if (deathmatch_value && !strstr(cl->newweapon->icon, "pipe"))
			{
				it = cl->newweapon;
				ent->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
				ent->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(it);
				ent->client->pickup_msg_time = level.time + 5.5;
			}

			return;	// successful
		}
		else
		{
			it->use (ent, it);
		}
	}
}

#define DREWACTIVATEDISTANCE	96

// JOSEPH 11-MAY-99
qboolean infront_angle_activate (vec3_t selfang, vec3_t selforg, vec3_t otherorg)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	AngleVectors (selfang, forward, NULL, NULL);
	VectorSubtract (otherorg, selforg, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > 0.95)
		return true;
	return false;
}
// END JOSEPH

// JOSEPH 21-SEP-98
//hypov8 use the highlighted menu with the action key
void Cmd_Activate_f (edict_t *ent)
{
	edict_t		*trav, *best;
	float		best_dist=9999, this_dist;
// ACEBOT_ADD //MENU
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
		return;
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
		return;
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
		return;
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
		return;
	}
// ACEBOT_END

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
		{
			if (level.modeset == MATCH && no_spec->value && !ent->client->pers.admin && !ent->client->pers.rconx[0]) return;
			if (maxclients->value > 1)
			{
				if (!ent->client->chase_target)
				{
					ent->client->chasemode = EYECAM_CHASE;  // snap
					ent->client->chase_frame = level.framenum;
					ChaseStart(ent);
					if (ent->client->chase_target)
						ent->client->showscores = NO_SCOREBOARD;
				}
				else	// disable it
				{
					ChaseStop(ent);
				}
			}
		}
		return;
	}

	// if we are ducking
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		// find the near enemy 
		trav = best = NULL;
		// JOSEPH 13-MAY-99
		while ((trav = findradius(trav, ent->s.origin, 80)) != 0) // HYPOV8_ADD !=0
		// END JOSEPH
		{
			// JOSEPH 14-MAY-99
			if (!(trav->svflags & SVF_MONSTER))
				continue;
			// END JOSEPH
			// JOSEPH 6-JAN-99
			if (trav == ent)
				continue;
			// END JOSEPH
			//if (!infront(ent, trav))
			//	continue;
			//if (!visible(ent, trav))
			//	continue;
			if (((this_dist = VectorDistance(ent->s.origin, trav->s.origin)) > best_dist) && (this_dist > 32))
				continue;
			
			best = trav;
			best_dist = this_dist;
		}

		// if the enemy has cash - then take it
		// Joseph 14-MAY-99
		if ((best) && (best->currentcash > 0) && best->health <= 0)
		// END JOSEPH
		{
			int     index;
			gitem_t *item;
			
			ent->client->pers.currentcash += best->currentcash;
			//cprintf (ent, PRINT_HIGH, "%i dollars found\n", best->currentcash);
			ent->client->ps.stats[STAT_CASH_PICKUP] = best->currentcash;
			best->currentcash = 0;
			// flash the screen green
			ent->client->bonus_alpha = 0.25;	
			ent->client->bonus_alpha_color = 2;	
			gi.sound (ent, CHAN_AUTO, gi.soundindex("world/pickups/cash.wav"), 1, ATTN_NORM, 0);		

			item = FindItem ("Cash");
			index = ITEM_INDEX (item);
			// show icon and name on status bar
			ent->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(item->icon);
			ent->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+index;
			ent->client->pickup_msg_time = level.time + 5.5;		
		}
	}

	// JOSEPH 11-MAY-99
	// find a usable brush entity and tag it as such
	{
		edict_t		*target, *toptarget;
		vec3_t		dest;
		trace_t		trace, tr;
		vec3_t		dir, neworigin, endorg;	
		float       topdistance;
		int			directtarget;

		target = NULL;
		toptarget = NULL;
		topdistance = 10000;
		directtarget = 0;

		while (((target = findradius(target, ent->s.origin, DREWACTIVATEDISTANCE * (1 + (deathmatch_value != 0))))!=0 || toptarget) && !directtarget)   // HYPOV8_ADD !=0
		{
			if (!target)
				goto startyourtriggers;
			if (!(target->activate_flags & ACTIVATE_GENERAL))
				continue;
			if (target->targetname && target->key != -1)
				continue;
			
			VectorCopy(ent->s.origin, neworigin);
			neworigin[2] += ent->viewheight;
			
			AngleVectors( ent->client->ps.viewangles, dir, NULL, NULL );
			VectorMA (neworigin, DREWACTIVATEDISTANCE * (1 + (deathmatch_value != 0)), dir, dest);

			trace = gi.trace (neworigin, vec3_origin, vec3_origin, dest, ent, MASK_SOLID);
			
			// JOSEPH 19-MAY-99
			if (trace.ent && trace.ent->classname && (!strcmp(trace.ent->classname, "func_lift")))
			{
				edict_t		*targetL;

				targetL = NULL;

				while ((targetL = findradius(targetL, trace.endpos, 16)) != 0) // HYPOV8_ADD !=0
				{
					if (!(targetL->activate_flags & ACTIVATE_GENERAL))
						continue;			
					if (targetL->targetname && targetL->key != -1)
						continue;
					if (targetL->classname && (!strcmp(targetL->classname, "func_button")))
					{
						toptarget = targetL;
						goto startyourtriggers;
					}
				}
			}
			// END JOSEPH
			
			// JOSEPH 14-MAY-99
			if (trace.ent == target)
			{
				directtarget = 1;	
				
				if ((strcmp (target->classname, "func_door") == 0) ||
					(strcmp (target->classname, "func_door_rotating") == 0))
				{
					if ((target->team) && (target->teammaster))
					{
						toptarget = target->teammaster;
						goto startyourtriggers;
					}
				}

				toptarget = target;
				goto startyourtriggers;
			}
			// END JOSEPH

			VectorAdd (target->absmin, target->absmax, endorg);				
			VectorScale (endorg, 0.5, endorg);

			if (!(infront_angle_activate(ent->client->v_angle, neworigin, endorg)))
				continue;

			if (VectorDistance(trace.endpos, dest ) > topdistance)
				continue;
			
			tr = gi.trace(neworigin, NULL, NULL, endorg, ent, MASK_SOLID);

			// Ridah, added this since it's frustrating hitting the switches in deathmatch
			if (target->target == NULL)  //hypov8 crashing server if no target
				target->target = "null";
			if (!deathmatch_value || (!Q_stricmp(target->classname, "func_button") && !Q_stricmp(target->target, "safe2")))
			if (tr.ent != target)
				continue;

			topdistance = VectorDistance(trace.endpos, dest);
			toptarget = target;
			continue;

startyourtriggers:			

			target = toptarget;	
			toptarget = NULL;
	
	// END JOSEPH

			// JOSEPH 12-MAR-99-B
			// If it must be trigger unlocked
			if (target->key < 0)
			{
				gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);
				continue;
			}
			// END JOSEPH

			// JOSEPH 19-MAR-99-B
			// Kingpin keys must be placed here to open doors
			if (target->key > 0)
			{
				switch(target->key)
				{
					case 1:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("StoreRoomKey"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);
							continue;
						}
					}
					break;			

					case 2:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Electrical_Room"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);
							continue;
						}
					}
					break;	
					
					case 3:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Chem_Plant_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);			
							continue;
						}
					}
					break;	

					case 4:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Bridge_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;	

					case 5:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Shipyard_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;	

					case 6:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Warehouse_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;	

					case 7:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Shop_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;
					
					case 8:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Ticket"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
						else
							ent->client->pers.inventory[ITEM_INDEX(FindItem("Ticket"))] = 0;
					}
					break;	

					case 9:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("Office_Key"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;	

					case 10:
					{
						if (!ent->client->pers.inventory[ITEM_INDEX(FindItem("key10"))])
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}
					break;					

/*					case 11:
					{
						if (!(EP_UnlockDoorFlag (ent)))
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("world/doors/dr_locked.wav"), 1, ATTN_NORM, 0);					
							continue;
						}
					}*/
				}

				// Ridah, once unlocked, stay unlocked
				target->key = 0;
			}
			// END JOSEPH			

			// we have a valid one so lets flag it
			target->activate_flags |= ACTIVATE_AND_OPEN;
			break;
		}
		
		if (target)
			if (target->activate_flags & ACTIVATE_AND_OPEN)
				if (target->use)
				{
//					gi.dprintf( "%s, %s\n", target->classname, target->target );
					target->use (target, ent, ent);
				}
	}
}
// END JOSEPH

// RAFAEL 01-11-99
void Cmd_Reload_f (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;
	
	if (!cl->pers.weapon)
		return;

	// Ridah, fixes Tommygun reloading twice, if hit "reload" as it starts to auto-reload when out of ammo
	if (ent->client->weaponstate == WEAPON_RELOADING)
		return;

	if (ent->client->weaponstate != WEAPON_READY)
		return;

//hypov8
	if (!ent->client->pers.inventory[ent->client->ammo_index])
		return; //mm stop reloading if empty
//end
	if (dm_realmode->value == 2)
		return;

	ent->client->reload_weapon = true;
}
// END 01-11-99


void Cmd_Holster_f (edict_t *ent)
{
	cprintf (ent, PRINT_HIGH, "no holstering\n");
}

// JOSEPH 29-DEC-98
void Cmd_Hud_f (edict_t *ent)
{
	gi.WriteByte (svc_hud);
	gi.unicast (ent, true);
}
// END JOSEPH

void Cmd_Flashlight_f (edict_t *ent)
{
	if (!ent->client->flashlight && ent->client->pers.inventory[ITEM_INDEX(FindItem("Flashlight"))])
		ent->client->flashlight = true;
	else
		ent->client->flashlight = false;
}


/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon)
		return;

	// Ridah, if already changing weapons, start from the next weapon, for faster cycling
	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		selected_weapon = ITEM_INDEX(cl->newweapon);
	}
	else
	{
		selected_weapon = ITEM_INDEX(cl->pers.weapon);
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		if (selected_weapon == ITEM_INDEX(it) && cl->newweapon)
		{
			// Ridah, show the current weapon on the hud, for easy scrolling
			if (deathmatch_value && !strstr(cl->newweapon->icon, "pipe"))
			{
				it = cl->newweapon;
				ent->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
				ent->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(it);
				ent->client->pickup_msg_time = level.time + 5.5;
			}

			return;	// successful
		}
		else
		{
			it->use (ent, it);
		}
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;

	it->use (ent, it);

	// Ridah, show the current weapon on the hud, for easy scrolling
	if (deathmatch_value && !strstr(it->icon, "pipe"))
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
		ent->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(it);
		ent->client->pickup_msg_time = level.time + 5.5;
	}

}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;
	int		index;
	char		*s;
	//hypov8 todo: dont dro in spec
	s = gi.args();

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}

	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if (ent->solid != SOLID_NOT)
		cprintf (ent, PRINT_HIGH, "no suiciding!\n");
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
// ACEBOT_ADD
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
		return;
	}
// ACEBOT_END

// HYPOV8 ADD
	//hypo spec fixed
	if ( ent->client->pers.spectator == SPECTATING 
		&& ent->client->showscores == NO_SCOREBOARD
		&&	(level.modeset == MATCH || level.modeset == PUBLIC) ) 
	{
		char* string = "menu_main";
		gi.WriteByte(svc_stufftext);
		gi.WriteString(va("%s\n", string));
		gi.unicast(ent, true);
		ent->client->resp.scoreboard_frame = level.framenum + 50 - 30;
	}
// HYPOV8 END

	ent->client->showscores = NO_SCOREBOARD;
	ent->client->showinventory = false;

// ACEBOT_ADD
	DeathmatchScoreboard(ent); //hypov8 add, update scoreboard straight away
// ACEBOT_END
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		a;
	int		count;

	count = 0;

	cprintf(ent, PRINT_HIGH, "num score ping name            lastmsg country\n"
		"--- ----- ---- --------------- ------- ---------------------\n");
	for (a=1; a<=maxclients->value; a++)
	{
		gclient_t *c = g_edicts[a].client;
		if (c && (g_edicts[a].inuse || (c->pers.connected && (kpded2 || curtime-c->pers.lastpacket < 120000))))
		{
			char buf[16];
			Com_sprintf(buf,sizeof(buf), "%s", g_edicts[a].inuse ? va("%i",c->ping) : "CNCT");
			cprintf(ent, PRINT_HIGH, "%3d %5d %4s %-15s %7d %s\n",
				a-1, c->resp.score, buf, c->pers.netname, curtime-c->pers.lastpacket, c->pers.country ? c->pers.country : "");
			count++;
		}
	}

	cprintf (ent, PRINT_HIGH, "\n%i players\n", count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent, edict_t *other, int who)
{
	char *cmd;

	cmd = gi.argv(0);


	if (other!=NULL)
	{
		if (!other->client)
			return;

		if (!ent->solid)
			return;
	}

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->last_wave > (level.time - 2.5) && (ent->client->last_wave <= level.time))
		return;

	ent->client->last_wave = level.time;

	// say something
	{
		if (ent->gender == GENDER_MALE)
		{
			switch(who)
			{
				case 0: //random
					Voice_Random(ent, other, player_profanity_level2, NUM_PLAYER_PROFANITY_LEVEL2);
					break;
				case KINGPIN:
					Voice_Random(ent, other, kingpin_random, NUM_KINGPIN_RANDOM);
					break;
				case LEROY:
					Voice_Random(ent, other, leroy_random, NUM_LEROY_RANDOM);
					break;
				case MJ:
					Voice_Random(ent, other, mj_random, NUM_MJ_RANDOM);
					break;
				case MOMO:
					Voice_Random(ent, other, momo_random, NUM_MOMO_RANDOM);
					break;
				case LAMONT:
					Voice_Random(ent, other, lamont_random, NUM_LAMONT_RANDOM);
					break;
				case JESUS:
					Voice_Random(ent, other, jesus_random, NUM_JESUS_RANDOM);
					break;
				case TYRONE:
					Voice_Random(ent, other, tyrone_random, NUM_TYRONE_RANDOM);
					break;
				case WILLY:
					Voice_Random(ent, other, willy_random, NUM_WILLY_RANDOM);
					break;
				case MOKER:
					Voice_Random(ent, other, moker_random, NUM_MOKER_RANDOM);
					break;
				case HEILMAN:
					Voice_Random(ent, other, heilman_random, NUM_HEILMAN_RANDOM);
					break;
				default:
					Voice_Random(ent, other, player_profanity_level2, NUM_PLAYER_PROFANITY_LEVEL2);
			}
		}
		else if (ent->gender == GENDER_FEMALE)
		{
			switch(who)
			{
				case 0: //random
					Voice_Random(ent, other, f_profanity_level2, F_NUM_PROFANITY_LEVEL2);
					break;
				case BAMBI:
					Voice_Random(ent, other, bambi_random, F_NUM_BAMBI_RANDOM);
					break;
				case YOLANDA:
					Voice_Random(ent, other, yolanda_random, F_NUM_YOLANDA_RANDOM);
					break;
				case MONA:
					Voice_Random(ent, other, mona_random, F_NUM_MONA_RANDOM);
					break;
				case LOLA:
					Voice_Random(ent, other, lola_random, F_NUM_LOLA_RANDOM);
					break;
				case BLUNT:
					Voice_Random(ent, other, blunt_random, F_NUM_BLUNT_RANDOM);
					break;
				case BETH:
					Voice_Random(ent, other, beth_random, F_NUM_BETH_RANDOM);
					break;
				default:
					Voice_Random(ent, other, f_profanity_level2, F_NUM_PROFANITY_LEVEL2);
			}
		}
	}


	//hypov8 taunt ani re'enabled
	{
		int rnd;

		if (ent->client->anim_priority > ANIM_WAVE)
			return;
		if (ent->client->anim_priority == ANIM_ATTACK) //add hypov8 stop taunt ani when attacking
			return;

		ent->client->anim_priority = ANIM_WAVE;

		rnd = rand() % 3;

		switch (rnd)
		{
		case 0:
			//		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
			ent->s.frame = FRAME_tg_bird_01 - 1;
			ent->client->anim_end = FRAME_tg_bird_10;
			break;
		case 1:
			//		gi.cprintf (ent, PRINT_HIGH, "salute\n");
			ent->s.frame = FRAME_tg_crch_grab_01 - 1;
			ent->client->anim_end = FRAME_tg_crch_grab_16;
			break;
		case 2:
			//		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
			ent->s.frame = FRAME_tg_chin_flip_01 - 1;
			ent->client->anim_end = FRAME_tg_chin_flip_15;
			break;
		}
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	ent->client->resp.check_idle = level.framenum;

	if (ent->client->pers.mute) return;
	if (gi.argc () < 2 && !arg0)
		return;
	if (!arg0 && !*gi.argv(1)) //dont print empty chat
		return;

	if (!teamplay->value && !((int)(dmflags->value) & (DF_MODELTEAMS /*| DF_SKINTEAMS*/))
		&& ent->client->pers.spectator != SPECTATING)
		team = false;

	if (team && ((ent->client->pers.admin > NOT_ADMIN) || (ent->client->pers.rconx[0])))
		Com_sprintf (text, sizeof(text), ">(%s)<: ", ent->client->pers.netname);
	else if ((ent->client->pers.admin > NOT_ADMIN) || (ent->client->pers.rconx[0]))
		Com_sprintf (text, sizeof(text), ">|%s|<: ", ent->client->pers.netname);
	else if (team)
		Com_sprintf (text, sizeof(text), ">%s<: ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), ":%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	if (disable_anon_text && strchr(text, '\r'))
	{
		if (!ent->client->pers.anonwarn)
		{
			safe_cprintf(ent, PRINT_CHAT, "anonymous text is not allowed on this server\n");
			ent->client->pers.anonwarn++;
		}
		return;
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (!(team && level.modeset == MATCHSETUP) && flood_msgs->value)
	{
		cl = ent->client;

		if (level.time < cl->flood_locktill)
		{
			cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
			return;
		}
		i = cl->flood_whenhead - flood_msgs->value + 1;
		if (i < 0)
			i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value)
		{
			cl->flood_locktill = level.time + flood_waitdelay->value;
			safe_cprintf(ent, PRINT_CHAT, "Flood protection: You can't talk for %d seconds\n",
				(int)flood_waitdelay->value);
			return;
		}

		// if they repeat themselves really quickly, bitch-slap time
		if (cl->flood_when[cl->flood_whenhead] && (cl->flood_when[cl->flood_whenhead] > level.time - 1) &&
			!strcmp( ent->client->flood_lastmsg, text ))
		{
			cl->flood_locktill = level.time + flood_waitdelay->value;
			safe_cprintf(ent, PRINT_CHAT, "Flood protection: You can't talk for %d seconds\n",
				(int)flood_waitdelay->value);
			return;
		}

		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	strcpy( ent->client->flood_lastmsg, text );
	
	if (dedicated->value)
		safe_cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!(other->inuse))
			continue;
		if (!(other->client))
			continue;
		if (team && (ent->client->pers.spectator != SPECTATING || other->client->pers.spectator != SPECTATING))
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		safe_cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_DropCash_f (edict_t *self)
{
	edict_t		*cash;

	if (self->solid == SOLID_NOT) return;

	if ((int)(dmflags->value) & DF_DROP_CASH)
	{
		if (self->client->pers.currentcash)
		{
			cash = SpawnTheWeapon( self, "item_cashroll" );
			cash->currentcash = self->client->pers.currentcash;
			self->client->pers.currentcash = 0;
		}
	}
}

//===================================================================================
// Papa 10.6.99
//===================================================================================

//===================================================================================
// This is the start for all the commands that players can enter to run the mod.
//===================================================================================

void Cmd_PrintSettings_f (edict_t *ent)
{
	cprintf(ent, PRINT_HIGH,"\nCurrent Game Settings\n"
		"=====================\n");
	switch (level.modeset)
	{
		case PREGAME :
		case PUBLIC :
			cprintf(ent, PRINT_HIGH, "Server State    : Public\n");
			break;
		case MATCHSETUP :
			cprintf(ent, PRINT_HIGH, "Server State    : Match Setup\n");
			break;
		case MATCHCOUNT :
			cprintf(ent, PRINT_HIGH, "Server State    : Match Countdown\n");
			break;
		case MATCH :
			cprintf(ent, PRINT_HIGH, "Server State    : Match\n");
			break;
	}
	if (level.modeset == MATCHSETUP)
		cprintf(ent, PRINT_HIGH,"Matchstart score: %d : %d\n", team_startcash[0], team_startcash[1]);
	cprintf(ent, PRINT_HIGH,"Time limit      : %d\n", (int)timelimit->value);
	cprintf(ent, PRINT_HIGH,"Frag limit      : %d\n", (int)fraglimit->value);
	cprintf(ent, PRINT_HIGH,"Cash limit      : %d\n", (int)cashlimit->value);
	cprintf(ent, PRINT_HIGH,"dmflags         : %d\n", (int)dmflags->value);
	cprintf(ent, PRINT_HIGH,"dm_realmode     : %d\n", (int)dm_realmode->value);
	cprintf(ent, PRINT_HIGH,"Teamplay mode   : %d\n", (int)teamplay->value);
	if (!teamplay->value)
		cprintf(ent, PRINT_HIGH,"Bonus hit       : %d\n", (int)bonus->value);
	cprintf(ent, PRINT_HIGH,"Bunny-hopping   : %s\n", (int)dmflags->value&DF_NO_BUNNY ? "off" : "on");
	cprintf(ent, PRINT_HIGH,"Anti-spawncamp  : %s\n", anti_spawncamp->value? "on" : "off");
	cprintf(ent, PRINT_HIGH,"Shadows         : %s\n", no_shadows->value? "off" : "on");
	cprintf(ent, PRINT_HIGH,"FOV zooming     : %s\n", no_zoom->value? "off" : "on");
	if (level.modeset >= MATCHSETUP && level.modeset <= MATCH)
		cprintf(ent, PRINT_HIGH,"Spectating      : %s\n", no_spec->value? "off" : "on");
	if (password->string[0])
		cprintf(ent, PRINT_HIGH,"Server password : %s\n", password->string);
	{
		edict_t *admin = GetAdmin();
		if (admin)
			cprintf(ent, PRINT_HIGH, "Current admin   : %s\n", admin->client->pers.netname);
	}
}

void Cmd_CurseList_f (edict_t *ent)
{
	if (disable_curse)
	{
		cprintf(ent, PRINT_HIGH,"Player taunts are disabled\n");
		return;
	}
	cprintf(ent, PRINT_HIGH,"\nList of Curse Commands\n"
		"=============================\n"
		"MALE       FEMALE      RANDOM\n"
		"=============================\n"
		"KINGPIN    BAMBI       CURSE\n"
		"LEROY      YOLANDA     TAUNT\n"
		"MJ         MONA\n"
		"MOMO       LOLA\n"
		"LAMONT     BLUNT\n"
		"JESUS      BETH\n"
		"TYRONE\n"
		"WILLY\n"
		"MOKER\n"
		"HEILMAN\n");
}

void Cmd_CommandList_f (edict_t *ent)
{
	char buf[1000];

	cprintf(ent, PRINT_HIGH, "\nCurrent Console Commands\n"
		"========================\n");
	strcpy(buf, "menu, maplist, changemap, players, motd, settings, curselist, commands\n");

// ACEBOT_ADD
	if (sv_bot_allow_skill->value || sv_bot_allow_add->value)
	{
		if (sv_bot_allow_skill->value)
		{
			if (ent->client->pers.admin > NOT_ADMIN)
				strcat(buf, "botskill 0-10");
			else
				strcat(buf, "VoteBotSkill 0-10"); // ACEBOT_ADD
		}
		if (sv_bot_allow_add->value)
			strcat(buf, ", VoteBotRemove x, VoteBotAdd 1=Dragons"); // ACEBOT_ADD

	strcat(buf, " \n"); // ACEBOT_ADD
	}
// ACEBOT_END

	if (antilag->value)
		strcat(buf, "antilag");
	if (admincode[0] || ent->client->pers.rconx[0])
		strcat(buf, ", admin");
	if (ent->client->pers.admin > NOT_ADMIN)
		strcat(buf, ", resign");
	else if (!disable_admin_voting)
		strcat(buf, ", elect");


	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (teamplay->value)
			strcat(buf, "matchsetup, matchscore, matchstart, matchend, team1name, team2name\n");
		strcat(buf, "resetserver, settimelimit, setfraglimit, setcashlimit, setdmflags\nendmap, setidletime, toggle_asc, toggle_bunny, toggle_shadows, toggle_zoom");
		if (teamplay->value)
			strcat(buf, ", toggle_spec");
		else
			strcat(buf, ", setbonus");
		if (ent->client->pers.admin > ELECTED)
			strcat(buf, ", clientlist, mute");
		if (enable_password)
			strcat(buf, ", setpassword");
		if (!fixed_gametype)
			strcat(buf, ", setdm_realmode, setteamplay\n");
	} 
	else if (ent->client->pers.rconx[0])
		strcat(buf, ", resetserver, clientlist, mute\n");
	else
		strcat(buf, " \n");

	cprintf(ent, PRINT_HIGH, "%s\n", buf);

	if (ent->client->pers.admin > NOT_ADMIN && !fixed_gametype)
		cprintf(ent, PRINT_HIGH,
			"   The dm_realmode settings are:\n"
			"      0 : Standard\n"
			"      1 : Realmode\n"
			"      2 : Rocketmode\n"
			"   The teamplay settings are:\n"
			"      0 : Standard DM\n"
			"      1 : Bagman\n"
			"      4 : Team DM\n");
}

void Cmd_Yes_f (edict_t *ent)
{
	edict_t		*dude;
	int			i, nop, novy;
	char		command [256];

	if (level.voteset == NO_VOTES)
		cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time\n");
	else if (ent->client->resp.vote != HASNT_VOTED)
		cprintf(ent, PRINT_HIGH, "You have already voted\n");
	else
	{
		ent->client->resp.vote = YES;
		nop=0;
		novy=0;
		for_each_player_not_bot(dude, i)
		{
			if ((dude->client->resp.vote == YES) || (dude->client->resp.vote == CALLED_VOTE))
				novy ++;
			nop++;
		}

		safe_bprintf (PRINT_HIGH, "%d out of %d have voted YES\n", novy, nop);
		if ((novy *2) > nop)
		{
			switch (level.voteset) // Papa - if you wanted to add different types of votes, you could do it here
			{
				case VOTE_ON_ADMIN:
					for_each_player_not_bot(dude, i)
					{
						if (dude->client->resp.vote == CALLED_VOTE)
						{
							dude->client->pers.admin = ELECTED;
							safe_bprintf (PRINT_HIGH, "%s has been elected admin\n", dude->client->pers.netname);
							Cmd_CommandList_f(dude);
							break;
						}
					}
					break;
				case VOTE_ON_MAP:
// ACEBOT_ADD
					ACECM_LevelEnd();
					PrintScoreMatchEnd();

					//FreeBots();
// ACEBOT_END
					safe_bprintf (PRINT_HIGH, "The map change vote has passed\n");
					if (teamplay->latched_string || dm_realmode->latched_string)
						Com_sprintf (command, sizeof(command), "map \"%s\"\n", votemap);
					else
						Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", votemap);
					gi.AddCommandString (command);
					break;

// ACEBOT_ADD			//hypov8 todo?: check this
				case	VOTE_ADDBOT:
					if (level.modeset == MATCH || level.modeset == PUBLIC)
					{
						safe_bprintf(PRINT_HIGH, "Add Bot by: %s accepted.\n", dude->client->pers.netname);
						gi.dprintf("Vote AddBot by: %s accepted.\n", dude->client->pers.netname);

						if (teamplay->value)
							ACESP_SpawnBot_Random(voteAddBot, "\0", "\0", NULL);
						else 		
							ACESP_SpawnBot_Random("\0", "\0", "\0", NULL);
					}
					break;
				case	VOTE_REMOVEBOT:
					if (level.modeset == MATCH || level.modeset == PUBLIC)
					{
						safe_bprintf(PRINT_HIGH, "Remove Bot by: %s accepted. \n", dude->client->pers.netname);
						gi.dprintf("Vote AddRemove by: %s accepted. \n", dude->client->pers.netname);

						ACESP_RemoveBot(voteRemoveBot);
					}
					break;

				case VOTE_BOTSKILL:
					//if (level.modeset == MATCH || level.modeset == PUBLIC)
					{
						char sSkill[16];
						Com_sprintf(sSkill, sizeof(sSkill), "%1.1f", voteBotSkill);
						sv_botskill = gi.cvar_set("sv_botskill", sSkill);
						safe_bprintf(PRINT_HIGH, "Bot-Skill set to %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
						cprintf(ent, PRINT_HIGH, "This setting takes effect immediately\n");
					}
				break;
// ACEBOT_END
			}
			for_each_player_not_bot(dude, i)
			{
				if (dude->client->resp.vote == CALLED_VOTE)
					dude->client->resp.vote = HASNT_VOTED;
			}
			level.voteset = NO_VOTES;
		}
	}
// ACEBOT_ADD	
	//else
	{
		if (!ent->client->resp.vote == HASNT_VOTED)
			cprintf(ent, PRINT_HIGH, "You have already voted!\n");
		else
// ACEBOT_END
			cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time!\n");
	}
}

void Cmd_No_f (edict_t *ent)
{
	edict_t		*dude;
	int			i, nop, novn;

	if (level.voteset == NO_VOTES)
		cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time\n");
	else if (ent->client->resp.vote != HASNT_VOTED)
		cprintf(ent, PRINT_HIGH, "You have already voted\n");
	else
	{
		ent->client->resp.vote = NO;
		nop = 0;
		novn = 0;
		for_each_player_not_bot(dude, i)
		{
			if (dude->client->resp.vote == NO)
				novn ++;
			nop++;
		}
		if ((novn *2) >= nop)
		{
			switch (level.voteset) // Papa - if you wanted to add different types of votes, you could do it here
			{
				case VOTE_ON_ADMIN:
					safe_bprintf(PRINT_HIGH, "The request for admin has been voted down\n");
					break;
				case VOTE_ON_MAP:
					safe_bprintf(PRINT_HIGH, "The map change request has been voted down\n");
					break;
// ACEBOT_ADD //hypov8 todo?: check this
				case	VOTE_ADDBOT:
					safe_bprintf(PRINT_HIGH, "The request for votaddbot has been voted down!\n");
					break;
				case	VOTE_REMOVEBOT:
					safe_bprintf(PRINT_HIGH, "The request for voteremovebot has been voted down!\n");
					break;
				case	VOTE_BOTSKILL:
					safe_bprintf(PRINT_HIGH, "The request for votebotskill has been voted down!\n");
					break;
// ACEBOT_END
			}
			for_each_player_not_bot(dude, i)
			{
				if (dude->client->resp.vote == CALLED_VOTE)
					dude->client->resp.vote = HASNT_VOTED;
			}

			level.voteset = NO_VOTES;
		}
	}
}

void Cmd_Vote_f (edict_t *ent, char *vote)
{
	if (Q_stricmp (vote, "yes") == 0)
		Cmd_Yes_f (ent);
	else if (Q_stricmp (vote, "no") == 0)
		Cmd_No_f (ent);
	else
		cprintf(ent, PRINT_HIGH, "Vote YES or NO\n");
}

void Cmd_Elect_f (edict_t *ent)
{
	edict_t		*dude;
	int			count=0;
	int			i;

	if (disable_admin_voting)
	{
		cprintf(ent, PRINT_HIGH, "Electable admins has been disabled on this server\n");
		return;
	}

	dude = GetAdmin();
	if (dude)
	{
		if (dude == ent)
			cprintf(ent, PRINT_HIGH, "You already have admin\n");
		else
			cprintf(ent, PRINT_HIGH, "%s already has admin\n", dude->client->pers.netname);
		return;
	}

	if (level.voteset == NO_VOTES)
	{
		for_each_player_not_bot(dude, i)
		{
			dude->client->resp.vote = 0;
			count++;
		}
		if (count == 1)
		{
			ent->client->pers.admin = ELECTED;
			safe_bprintf (PRINT_HIGH, "%s has been elected admin\n", ent->client->pers.netname);
			Cmd_CommandList_f(ent);
			return;
		}
		safe_bprintf(PRINT_CHAT, "%s has requested admin privilages. Please vote YES or NO\n", ent->client->pers.netname);
		ent->client->resp.vote = CALLED_VOTE;
		level.voteframe = level.framenum;
		level.voteset= VOTE_ON_ADMIN;
// HYPOV8_ADD
		gi.WriteByte(svc_stufftext);
		gi.WriteString("play misc/talk.wav");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
	else
		cprintf(ent, PRINT_HIGH, "A vote is already in progress\n");
}

void Cmd_MapList_f (edict_t *ent)
{
	int		i;

	if (!num_maps)
	{
		cprintf(ent, PRINT_HIGH, "Any map on this server may be loaded\n");
		return;
	}

	cprintf(ent, PRINT_HIGH, "Valid maps for this server:\n");
	i = 0;
	if (num_maps > 20)
		for (; i<num_maps-1; i+=2)
			cprintf(ent, PRINT_HIGH, "   %-17s   %s\n", maplist[i], maplist[i+1]);
	for (; i<num_maps; i++)
		cprintf(ent, PRINT_HIGH, "   %s\n", maplist[i]);
}

qboolean ValidMap (char *mapname)
{
	int		i;

	if (num_maps && fixed_gametype)
	{
		for (i=0; i<num_maps; i++)
			if (!strcmp(maplist[i], mapname))
				return true;
	}
	else
	{
		char filename[MAX_QPATH];
		Com_sprintf(filename, sizeof(filename), "maps/%s.bsp", mapname);
		if (file_exist(filename))
			return true;
		if (kpded2)
		{
			// check for an "override" file
			Com_sprintf(filename, sizeof(filename), "maps/%s.bsp.override", mapname);
			if (file_exist(filename))
				return true;
		}
	}
	return false;
}

void Cmd_ChangeMap_f (edict_t *ent)
{
	char		*s;
	char		command [256];

	if (gi.argc() < 2)
	{
		cprintf(ent, PRINT_HIGH, "Usage: changemap <mapname>\n");
		return;
	}

	s = gi.args();
	kp_strlwr(s);
	if (!ValidMap (s)) // Always make sure the map is on the server before switching
	{
		cprintf(ent, PRINT_HIGH, "%s is not a valid map\n", s);
		return;
	}

	if (!ent->client->pers.admin)
	{
		if (level.voteset == NO_VOTES)  
		{
			edict_t		*dude;
			int			count=0;
			int			i;

			for_each_player_not_bot(dude, i)
			{
				dude->client->resp.vote = 0;
				count++;
			}
			if (count > 1)
			{
				strncpy(votemap, s, sizeof(votemap)-1);
				safe_bprintf(PRINT_CHAT, "%s has requested a map change to %s. Please vote YES or NO\n", ent->client->pers.netname, votemap);
				ent->client->resp.vote = CALLED_VOTE;
				level.voteframe = level.framenum;
				level.voteset= VOTE_ON_MAP;
				return;
			}
		}
		else
		{
			cprintf(ent, PRINT_HIGH, "You do not have admin and a vote is already in progress\n");
			return;
		}
	}

	// ACEBOT_ADD
	ACECM_LevelEnd();
	PrintScoreMatchEnd();
	//FreeBots();
	// ACEBOT_END

	if (teamplay->latched_string || dm_realmode->latched_string)
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", s);
	else
		Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", s);
	gi.AddCommandString (command);
}

void Cmd_MatchSetup_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		MatchSetup ();
		Cmd_PrintSettings_f (ent);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_MatchStart_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
		MatchStart ();
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

#if 0
void Cmd_VoteMap_f(edict_t *ent)
{
	int i, f, count;
	edict_t *dude;
	char		*s;
	char	command[256];
	struct stat st;
	char mapInMod[60], mapInMain[60];
	cvar_t	*game_dir;


	s = gi.args();
	kp_strlwr(s);

	//hypo mod folder
	game_dir = gi.cvar("game", "", 0);
	sprintf(mapInMod, "%s/maps/%s.bsp", game_dir->string, s); //hypov8 DIR_SLASH

	//hypov8 main folder
	sprintf(mapInMain, "%s/maps/%s.bsp", "main", s); //hypov8 DIR_SLASH


	if (!ValidMap(s)) // Always make sure the map is on the server before switching
	{
		f = stat(mapInMod, &st);
		if (f == -1)
		{
			f = stat(mapInMain, &st);
			if (f == -1)
			{
				cprintf(ent, PRINT_HIGH, "%s is not a valid map\n", s);
				return;
			}
			else
				cprintf(ent, PRINT_HIGH, "%s exists in 'main' dir but not in map cycle file\n", s);

		}
		else
			cprintf(ent, PRINT_HIGH, "%s exists in %s dir but not in map cycle file\n", s, game_dir);
	}

	if (level.voteset == NO_VOTES)
	{
		count = 0;
		for_each_player_not_bot(dude, i)
		{
			dude->vote = 0;
			count++;
		}
		if (count == 1)
		{
			//safe_bprintf(PRINT_HIGH, "%s alowed to change map.\n", ent->client->pers.netname);
			cprintf(ent, PRINT_CHAT, "%s alowed to change map.\n", ent->client->pers.netname);

			Com_sprintf(command, sizeof(command), "map \"%s\"\n", s);
			gi.AddCommandString(command);

			return;
		}						//¡¢£¤¥¦§¨©ª«¬­­
		safe_bprintf(PRINT_CHAT, "%s has requested map change to %s.\n\nPLEASE VOTE\n\n", ent->client->pers.netname, s);
		ent->vote = CALLED_VOTE;
		level.voteframe = level.framenum;
		level.voteset = VOTE_ON_MAP;

		strcpy(changeMapName, s);

		gi.WriteByte(svc_stufftext);
		gi.WriteString("play misc/talk.wav");
		gi.multicast(vec3_origin, MULTICAST_ALL);
	}
	else if (level.voteset != NO_VOTES)
		cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

}
//END
#endif

// ACEBOT_ADD
static void Cmd_VoteAddBot_f(edict_t *ent, int teamUse)
{
	int i, count;
	edict_t *dude;
	char	*s = '\0';
	char *team = '\0';


	if (!sv_bot_allow_add->value)
	{
		cprintf(ent, PRINT_HIGH, "Clients NOT allowed to add bots\n");
		return;
	}

	if (num_bots >= (int)sv_bot_max->value)
	{
		cprintf(ent, PRINT_HIGH, "Maximum Bots Reached\n");
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
			cprintf(ent, PRINT_CHAT, "¡­%s alowed to add bot.\n", ent->client->pers.netname);
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
		cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

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
		cprintf(ent, PRINT_HIGH, "Bot Vote Disabled.\n", ent->client->pers.netname);
		return;
	}

	if (!isMenu)
	{
		s = gi.args();
		if (s[0])
			name = s;
		else if (s[0] == '\0')
		{
			cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
			return;
		}
	}
	else
	{
		if (botnames == NULL || !botnames[0])		
		{
			cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
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
			cprintf(ent, PRINT_HIGH, "INVALID BOT NAME.\n", ent->client->pers.netname);
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
			cprintf(ent, PRINT_CHAT, "%s alowed to remove bot.\n", ent->client->pers.netname);
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
		cprintf(ent, PRINT_HIGH, "Vote is allready in progress.\n");

}


//hypo vote skill
void Cmd_VoteSkill_f(edict_t *ent, qboolean usedMenu) //VOTE_BOTSKILL;

{
	int i, count;
	edict_t *dude;
	char		*s;
	float skill_f;

	if (!sv_bot_allow_skill->value)	{
		cprintf(ent, PRINT_HIGH, "Bot-Skill Voting Disabled.\n", ent->client->pers.netname);
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



void Cmd_Menu_f(edict_t *ent)
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

// HYPOV8_ADD
void Cmd_EndMap_f(edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (level.modeset != ENDGAMEVOTE || level.modeset != ENDGAME) //hypov8 todo?: check this
		{
			safe_bprintf(PRINT_HIGH, "Admin ended map.\n");
			if(	level.modeset == MATCH)
				MatchEnd();
			else if (level.modeset == PUBLIC)
				EndDMLevel();
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin.\n");
}
//END



void Cmd_MatchEnd_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (level.modeset != MATCH)
			cprintf(ent, PRINT_HIGH, "A match has not been started\n");
		else
			MatchEnd ();
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

/*void Cmd_PauseMatch_f(edict_t *ent)
{
    edict_t		*dude;
    int		    i;
    
    if (ent->client->pers.admin > ELECTED)
    {
        for_each_player(dude,i)
        {
            dude->client->ps.pmove.pm_type = PM_FREEZE;
            dude->movetype = MOVETYPE_NONE;
            cprintf(dude,PRINT_HIGH,"You are phrozen.\n");
        }
    }
    else
		cprintf(ent,PRINT_HIGH,"You do not have permission\n");
}*/

void Cmd_MatchScore_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int t0, t1;
		if (level.modeset != MATCHSETUP)
		{
			cprintf(ent, PRINT_HIGH, "Can only set the scores when in matchsetup mode\n");
			return;
		}
		if (gi.argc() != 3)
		{
			cprintf(ent, PRINT_HIGH, "Usage: matchscore <drags> <nicks>\n");
			return;
		}
		t0 = atoi(gi.argv(1));
		t1 = atoi(gi.argv(2));
		if (t0 > 9999 || t1 > 9999)
		{
			cprintf(ent, PRINT_HIGH, "You can not set scores that high\n");
			return;
		}
		team_startcash[0] = t0;
		team_startcash[1] = t1;
		safe_bprintf(PRINT_HIGH, "The match score will start at %d (%s) to %d (%s)\n", team_startcash[0], team_names[1], team_startcash[1], team_names[2]);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetTimeLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if ((i < 0) || (i > 60))
		{
			cprintf(ent, PRINT_HIGH, "Please choose a timelimit between 0 and 60\n");
			return;
		}
		gi.cvar_set("timelimit", value);
		safe_bprintf(PRINT_HIGH, "The timelimit has been changed to %d\n", i);
		UpdateTime();
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetClientIdle_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 60)
		{
			cprintf(ent, PRINT_HIGH, "Please choose an idle time of at least 60\n");
			return;
		}
		gi.cvar_set("idle_client", value);
		safe_bprintf(PRINT_HIGH, "The idle time has been changed to %d seconds\n", i);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetFragLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0) 
		{
			cprintf(ent, PRINT_HIGH, "Please choose a positive fraglimit\n");
			return;
		}
		gi.cvar_set("fraglimit", value);
		safe_bprintf(PRINT_HIGH, "The fraglimit has been changed to %d\n", i);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetCashLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0 ) 
		{
			cprintf(ent, PRINT_HIGH, "Please choose a positive cashlimit\n");
			return;
		}
		gi.cvar_set("cashlimit", value);
		safe_bprintf(PRINT_HIGH, "The cashlimit has been changed to %d\n",i);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetDmFlags_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int i;
		if (value[0]=='+')
			i = (int)dmflags->value | atoi(value+1);
		else if (value[0]=='-')
			i = (int)dmflags->value & ~atoi(value+1);
		else
			i = atoi(value);
		gi.cvar_set("dmflags", va("%i", i));
		safe_bprintf(PRINT_HIGH, "dmflags has been changed to %d\n", i);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

/*void Cmd_Clear_f (edict_t *ent)
{
	ent->client->pers.bagcash = 0;
	ent->client->resp.deposited = 0;
	ent->client->resp.score = 0;
	ent->client->pers.currentcash = 0;
	ent->client->resp.acchit = ent->client->resp.accshot = 0;
	memset(ent->client->resp.fav,0,8*sizeof(int));
	safe_bprintf(PRINT_HIGH,"%s cleared its score\n",ent->client->pers.netname);
}*/

void Cmd_SetRealMode_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (fixed_gametype)
		{
			cprintf(ent, PRINT_HIGH, "This server's game type may not be changed\n");
			return;
		}
		gi.cvar_set("dm_realmode", value);
		cprintf(ent, PRINT_HIGH, "This setting will take effect with a map change\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}


// ACEBOT_ADD
void Cmd_SetBotSkill_f(edict_t *ent, char *value) //add hypov8
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

void Cmd_SetPassword_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (enable_password)
			gi.cvar_set("password", value);
		else
			cprintf(ent, PRINT_HIGH, "The password option is disabled\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetTeamplay_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (fixed_gametype)
		{
			cprintf(ent, PRINT_HIGH, "This server's game type may not be changed\n");
			return;
		}
		if (value[0] && (i == 0) || (i == 1) || (i == 4))
		{
			gi.cvar_set("teamplay", value);
			cprintf(ent, PRINT_HIGH, "This setting will take effect with a map change\n");
		}
		else
			cprintf(ent, PRINT_HIGH, "Teamplay settings are as follows:\n 0: Standard DM\n 1: Bagman\n 4: Team DM\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetBonus_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0) 
		{
			cprintf(ent, PRINT_HIGH, "Please choose a positive bonus hit deficit\n");
			return;
		}
		gi.cvar_set("bonus", value);
		safe_bprintf(PRINT_HIGH, "The bonus hit deficit has been changed to %d\n", i);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_ResetServer_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN || ent->client->pers.rconx[0]) 
		ResetServer(false);
	else
		cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void Cmd_Toggle_ASC_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (anti_spawncamp->value)
		{
			gi.cvar_set("anti_spawncamp", "0");
			safe_bprintf(PRINT_HIGH, "Anti-spawn camping is now off\n");
		}
		else
		{
			gi.cvar_set("anti_spawncamp", "1");
			safe_bprintf(PRINT_HIGH, "Anti-spawn camping is now on\n");
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Toggle_Bunny_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		gi.cvar_set("dmflags", va("%i", (int)dmflags->value ^ DF_NO_BUNNY));
		if ((int)dmflags->value & DF_NO_BUNNY)
			safe_bprintf(PRINT_HIGH, "Bunny-hopping is now disabled\n");
		else
			safe_bprintf(PRINT_HIGH, "Bunny-hopping is now enabled\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Toggle_Spec_f(edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (level.modeset < MATCHSETUP || level.modeset > MATCH)
		{
			cprintf(ent, PRINT_HIGH, "Spectating can only be disabled in match mode\n");
			return;
		}
		if (no_spec->value)
		{
			gi.cvar_set("no_spec", "0");
			safe_bprintf(PRINT_HIGH, "Spectating is now enabled\n");
		}
		else
		{
			gi.cvar_set("no_spec", "1");
			safe_bprintf(PRINT_HIGH, "Spectating is now disabled\n");
			if (level.modeset == MATCH)
			{
				int		i;
				edict_t	*dood;
				for_each_player_not_bot(dood, i)
				{
					if (dood->client->pers.spectator == SPECTATING && !dood->client->pers.admin && !dood->client->pers.rconx[0])
					{
						int save = dood->client->showscores;
						PutClientInServer(dood);
						dood->client->showscores = (save ? save : SCOREBOARD);
					}
				}
			}
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Toggle_Shadows_f(edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (no_shadows->value)
		{
			gi.cvar_set("no_shadows", "0");
			safe_bprintf(PRINT_HIGH, "Shadows are now on (if enabled by client)\n");
		}
		else
		{
			gi.cvar_set("no_shadows", "1");
			safe_bprintf(PRINT_HIGH, "Shadows are now off\n");
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Toggle_Zoom_f(edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (no_zoom->value)
		{
			gi.cvar_set("no_zoom", "0");
			safe_bprintf(PRINT_HIGH, "FOV zooming is now enabled\n");
		}
		else
		{
			gi.cvar_set("no_zoom", "1");
			safe_bprintf(PRINT_HIGH, "FOV zooming is now disabled\n");
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_AntiLag_f(edict_t *ent, char *value)
{
	if (!value[0])
	{
		cprintf(ent, PRINT_HIGH, "Usage: antilag <0/1>\n0 = off, 1 = on (if enabled by server)\n");
		return;
	}
	ent->client->pers.noantilag = !atoi(value);
	// put it in a client variable (survives server resets)
	gi.WriteByte(svc_stufftext);
	gi.WriteString(va("set antilag \"%s\"\n", value));
	gi.unicast(ent, true);
}

void Cmd_Status_f(edict_t *ent)
{
	if (ent->client->pers.admin > ELECTED || ent->client->pers.rconx[0])
	{
		int a;

		cprintf(ent, PRINT_HIGH, "map : %s\n", level.mapname);
		cprintf(ent, PRINT_HIGH, "num score ping name            lastmsg address               ver\n"
			"--- ----- ---- --------------- ------- --------------------- ----\n");
		for (a=1; a<=maxclients->value; a++)
		{
			gclient_t *c = g_edicts[a].client;
			if (c && (g_edicts[a].inuse || (c->pers.connected && (kpded2 || curtime-c->pers.lastpacket < 120000))))
			{
				char buf[16];
				Com_sprintf(buf,sizeof(buf), g_edicts[a].inuse ? "%d" : "CNCT", c->ping);
				cprintf(ent, PRINT_HIGH, "%3d %5d %4s %-15s %7d %-21s %.2f\n",
					a-1, c->resp.score, buf, c->pers.netname, curtime-c->pers.lastpacket, c->pers.ip, c->pers.version/100.0);
			}
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void Cmd_SetTeamName_f (edict_t *ent, int team, char *name)
{
	if (!name || !*name) return;
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (strlen(name) < 16 && name[0] != ' ')
		{
			setTeamName(team, name);
			manual_tagset = 1;
			UpdateTeams();
		}
		else
			cprintf(ent, PRINT_HIGH, "Team name can't be longer than 15 letters or start with whitespace\n");
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Admin_f (edict_t *ent, char *value)
{
	if (!admincode[0] && !ent->client->pers.rconx[0])
	{
		cprintf(ent, PRINT_HIGH, "The admin option is disabled\n");
		return;
	}
	if (ent->client->pers.admin == ADMIN)
	{
		cprintf(ent, PRINT_HIGH, "You already have admin\n");
		return;
	}
	if (!Q_stricmp(admincode, value) || ent->client->pers.rconx[0])
	{
		if (!ent->client->pers.admin)
		{
			edict_t *admin = GetAdmin();
			if (admin)
			{
				if (admin->client->pers.admin == ADMIN)
				{
					cprintf(ent, PRINT_HIGH, "%s already has admin\n", admin->client->pers.netname);
					return;
				}
				else
				{
					admin->client->pers.admin = NOT_ADMIN;
					safe_bprintf(PRINT_HIGH, "%s has been removed from admin\n", admin->client->pers.netname);
				}
			}
			safe_bprintf(PRINT_HIGH, "%s is now admin\n", ent->client->pers.netname);
		}
		ent->client->pers.admin = ADMIN;
		Cmd_CommandList_f(ent);
		if (level.voteset == VOTE_ON_ADMIN)
			level.voteset = NO_VOTES;
	}
	else
		cprintf(ent, PRINT_HIGH, "Incorrect admin code\n");
}

void Cmd_Resign_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN) 
	{
		ent->client->pers.admin = NOT_ADMIN;
		safe_bprintf(PRINT_HIGH, "%s is no longer admin\n", ent->client->pers.netname);
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have admin\n"); 
}

void Cmd_BanDicks_f(edict_t *ent, int type)	
{
	char	filename[64], dir[32], *value;
	cvar_t	*game_dir;	
	FILE 	*list;

	if (ent == NULL  // via sv rcon
		|| ent->client->pers.admin > ELECTED || ent->client->pers.rconx[0]) 
	{
		if (!gi.argv(2) || !*gi.argv(2)) 
		{
			if (type)
				safe_cprintf(ent, PRINT_HIGH, "Usage: banip <ip>\n");
			else
				safe_cprintf(ent, PRINT_HIGH, "Usage: banname <name>\n");		
			return;
        } 
    
		value = gi.argv(2);
    
		game_dir = gi.cvar("game", "", 0);
		strcpy(dir, game_dir->string[0] ? game_dir->string : "main");
    
		if (type) //ip
		{
			if (ban_ip_filename[0]) 
			{
				Com_sprintf (filename, sizeof(filename), "%s/%s",dir, ban_ip_filename);
				list = fopen(filename, "a");
				if (!list)
				{
					safe_cprintf(ent, PRINT_HIGH, "Failed to open banned IPs file\n");
					return;
				}
				fprintf(list,"%s\n", value);
				fclose(list);
				if (num_ips < 100)
				{
					strncpy(ip[num_ips].value, value, 16);
					num_ips++;
				}
			}
			else
				safe_cprintf(ent, PRINT_HIGH, "IP banning is disabled\n");
		}
		else //name
		{
			if (ban_name_filename[0]) 
			{
				Com_sprintf (filename, sizeof(filename), "%s/%s",dir, ban_name_filename);
				list = fopen(filename, "a");
				if (!list)
				{
					safe_cprintf(ent, PRINT_HIGH, "Failed to open banned names file\n");
					return;
				}
				fprintf(list,"%s\n", value);
				fclose(list);
				if (num_netnames < 100)
				{
					strncpy(netname[num_netnames].value, value, 16);
					num_netnames++;
				}
			}
			else
				safe_cprintf(ent, PRINT_HIGH, "Name banning is disabled\n");
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void Cmd_ListBans_f(edict_t *ent)
{
	int		a;

	if (ent == NULL  // via sv rcon
		|| ent->client->pers.admin > ELECTED || ent->client->pers.rconx[0]) 
	{
		if (ban_ip_filename[0]) 
		{
			safe_cprintf(ent, PRINT_HIGH, "List of banned IP addresses\n===========================\n");         
			for (a=0; a<num_ips; a++)
				safe_cprintf(ent, PRINT_HIGH, "%s\n", ip[a].value);
			safe_cprintf(ent, PRINT_HIGH, "\n"); // cosmetic stuff
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "IP banning is disabled\n");
    
		if (ban_name_filename[0]) 
		{
			safe_cprintf(ent, PRINT_HIGH, "List of banned names\n====================\n");
			for (a=0; a<num_netnames; a++)
				safe_cprintf(ent, PRINT_HIGH, "%s\n", netname[a].value);
			safe_cprintf(ent, PRINT_HIGH, "\n"); // cosmetic stuff
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "Name banning is disabled\n");
	}
	else 
		cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void checkkick(edict_t *ent, char *cmd, char *action)
{
	int a;
	char *name;

	if (gi.argc() < 3 || gi.argc() > 4)
	{
		cprintf(ent, PRINT_HIGH, "Usage: %s <userid> <reason>\nNOTE: <reason> is optional\n", cmd);
		return;
	}

	name = gi.argv(2);
	a = atoi(name);
	if (!strcmp(name,"0") || (a > 0 && a < maxclients->value))
	{
		a++;
		if (g_edicts[a].inuse && g_edicts[a].client)
		{
			if (gi.argc() == 4)
    			safe_bprintf(PRINT_HIGH, "%s is being %s by %s because %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname, gi.argv(3));
			else
				safe_bprintf(PRINT_HIGH, "%s is being %s by %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname);
		}
		else
			cprintf(ent, PRINT_HIGH, "Client %s is not active\n", name);
	}
	else
	{
		for (a=1; a<=maxclients->value; a++)
			if (g_edicts[a].inuse && g_edicts[a].client
				&& !Q_strcasecmp(g_edicts[a].client->pers.netname, name)) break;
		if (a > maxclients->value)
			cprintf(ent,PRINT_HIGH,"Userid %s is not on the server\n",name);
		else
		{
			if (gi.argc() == 4)
    			safe_bprintf(PRINT_HIGH, "%s is being %s by %s because %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname, gi.argv(3));
			else
				safe_bprintf(PRINT_HIGH, "%s is being %s by %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname);
		}
	}
}

void dumpuser(edict_t *ent, edict_t *target)
{
	char *info = target->client->pers.userinfo, *s, buf[64];

	cprintf(ent, PRINT_HIGH, "userinfo\n--------\n");
	while (*info == '\\')
	{
		s = ++info;
		while (*s!='\\')
		{
			if (!*s) return;
			s++;
		}
		memcpy(buf, info, s - info);
		buf[s - info] = 0;
		info = ++s;
		while (*s && *s != '\\') s++;
		cprintf(ent, PRINT_HIGH, "%-19s %.*s\n", buf, s - info, info);
		info = s;
	}
}

void Cmd_Mute_f (edict_t *ent, char *value)
{
	if (ent == NULL  // via sv rcon
		|| ent->client->pers.admin > ELECTED || ent->client->pers.rconx[0]) 
	{
		int		i = atoi (value);
		if (!*value || (i < 0 || (i+1) > maxclients->value) || !(g_edicts[i+1].inuse && g_edicts[i+1].client))
		{
			if (*value) safe_cprintf(ent, PRINT_HIGH, "Unable to find client id match\n");
			safe_cprintf(ent, PRINT_HIGH, "Usage: mute <userid>\n");
			return; 
		}  
		if (g_edicts[i+1].client->pers.mute == 0)
		{
			safe_cprintf(ent, PRINT_HIGH, "Enabled mute on %s\n", g_edicts[i + 1].client->pers.netname);
			cprintf(&g_edicts[i+1], PRINT_HIGH, "Admin has muted you\n");
			g_edicts[i+1].client->pers.mute = 1; 
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Disabled mute on %s\n", g_edicts[i + 1].client->pers.netname);
			cprintf(&g_edicts[i+1], PRINT_HIGH, "Admin has unmuted you\n");
			g_edicts[i+1].client->pers.mute = 0; 
		}
	}
	else
		cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void Cmd_Rcon_f (edict_t *ent)
{
	char *cmd, cmdline[256];
	int a;

	if (!num_rconx_pass)
	{
		cprintf(ent, PRINT_HIGH, "The rconx options are disabled\n");
		return;
	}

	if (!ent->client->pers.rconx[0])
	{
		cprintf(ent, PRINT_HIGH, "You must login with \"rconx_login\" before using \"rconx\"\n");
		return;
	}

	if (gi.argc() < 2) return;
	cmd = gi.argv(1);
	if (!Q_strncasecmp(cmd, "rcon", 4)) return;

	strcpy(cmdline, cmd);
	for (a=2; a<gi.argc(); a++)
	{
		// reason hack
		if ((!Q_stricmp(cmd, "kick") || !Q_stricmp(cmd, "kickban")) 
		&& gi.argc() == 4 
		&& a == 3) break;
    
		strcat(cmdline, " \"");
		strcat(cmdline, gi.argv(a));
		strcat(cmdline, "\"");
	}
	gi.dprintf("rconx (%s:%s) %s\n", ent->client->pers.netname, ent->client->pers.rconx, cmdline);

	if (!Q_stricmp(cmd, "status"))
	{
		Cmd_Status_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "serverinfo"))
	{
		cvar_t *v = gi.cvar(TIMENAME, "", 0);
		cprintf(ent, PRINT_HIGH, "Server info settings:\n");
		cprintf(ent, PRINT_HIGH, "mapname              %s\n", level.mapname);
		while (v)
		{
			if ((v->flags & CVAR_SERVERINFO) && v->string[0])
				cprintf(ent, PRINT_HIGH, "%-21s%s\n", v->name, v->string);
			v = v->next;
		}
		return;
	}
	else if (!Q_stricmp(cmd, "dumpuser"))
	{
		char *name;
		if (gi.argc() != 3)
		{
			cprintf(ent, PRINT_HIGH, "Usage: %s <userid>\n", cmd);
			return;
		}
		name = gi.argv(2);
		a = atoi(name);
		if (!strcmp(name, "0") || (a > 0 && a < maxclients->value))
		{
			a++;
			if (g_edicts[a].inuse && g_edicts[a].client)
				dumpuser(ent, g_edicts + a);
			else
				cprintf(ent, PRINT_HIGH, "Client %s is not active\n", name);
		}
		else
		{
			for (a=1; a<=maxclients->value; a++)
				if (g_edicts[a].inuse && g_edicts[a].client
					&& !Q_strcasecmp(g_edicts[a].client->pers.netname, name)) break;
			if (a>maxclients->value)
				cprintf(ent, PRINT_HIGH, "Userid %s is not on the server\n", name);
			else
				dumpuser(ent, g_edicts + a);
		}
		return;
	}
	else if (!Q_stricmp(cmd, "nopassword"))
	{
		gi.cvar_set("password", "");
		return;
	}
    // add reason code here
	else if (!Q_stricmp(cmd, "kick")) 
	{
		checkkick(ent, cmd, "kicked");
	}
	else if (!Q_stricmp(cmd, "kickban")) 
	{
		checkkick(ent, cmd, "kicked & banned");
	}
	else if (!Q_stricmp(cmd, "banip")) 
	{
		Cmd_BanDicks_f(ent, 1);
		return;
	}
	else if (!Q_stricmp(cmd, "banname")) 
	{
		Cmd_BanDicks_f(ent, 0);
		return;
	}
	else if (!Q_stricmp(cmd, "listbans"))
	{
		Cmd_ListBans_f(ent);
		return;
	}
// ACEBOT_ADD
	else if (!Q_stricmp(cmd, "gamemap") || !Q_stricmp(cmd, "map"))
	{
		ACECM_LevelEnd();
		PrintScoreMatchEnd();
		//FreeBots();
	}
// ACEBOT_END
	else if (gi.argc() == 2)
	{
		char *val=gi.cvar(cmd, "", 0)->string;
		if (val[0])
		{
			cprintf(ent, PRINT_HIGH, "\"%s\" is \"%s\"\n", cmd, val);
			return;
		}
	}

	gi.AddCommandString(cmdline);
}

void Cmd_Rcon_login_f (edict_t *ent, char *pass)
{
	int a;
	time_t t;

	if (!num_rconx_pass)
	{
		cprintf(ent, PRINT_HIGH, "The rconx options are disabled\n");
		return;
	}

	for (a=0; a<num_rconx_pass; a++)
		if (!Q_stricmp(pass, rconx_pass[a].value)) break;
	if (a == num_rconx_pass)
	{
		ent->client->pers.rconx[0] = 0;
		cprintf(ent, PRINT_HIGH, "Invalid rconx login\n");
		return;
	}
	strcpy(ent->client->pers.rconx, pass);
	time(&t);
	gi.dprintf("rconx_login (%s:%s) ip: %s time: %s", ent->client->pers.netname, ent->client->pers.rconx, ent->client->pers.ip, ctime(&t));
	cprintf(ent, PRINT_HIGH, "Successfully logged in\n");
}

void Cmd_MOTD_f (edict_t *ent)
{
	ent->client->showscores = SCORE_MOTD;
	ent->client->resp.scoreboard_frame = 0;
}

//must pass a quoted string (otherwise only first word will be displayed)
void ErrorMSGBox(edict_t *ent, char *msg)
{
    char errmsg[256];
    Com_sprintf(errmsg, sizeof(errmsg), "error %s", msg);
    gi.WriteByte(svc_stufftext);
    gi.WriteString(errmsg);
    gi.unicast(ent, true);
}

/*
=================
ClientCommand
=================
*/

void ClientCommand (edict_t *ent)
{
	char	*cmd;
//gi.dprintf("cmd: %s [%s]\n",gi.argv(0), gi.args());

	if (!ent->client)
		return;		// not fully in game yet

	ent->client->pers.lastpacket = curtime;

	if (!ent->inuse)
		return;

// ACEBOT_ADD
	if (ACECM_Commands(ent))
		return;

	if (ent->acebot.is_bot)
		return; //hypo bot wont use console
// ACEBOT_END

	cmd = gi.argv(0);

	if (!strncmp(cmd, cmd_check, 7))
	{
#ifndef HYPODEBUG //add hypov8 allow developer
		switch (cmd[7])
		{
			case 'A':
				if (gi.argc() != 4 || atof(gi.argv(1)) != 0 || atof(gi.argv(2)) < 16)
				{
					KICKENT(ent, "%s is being kicked for using a texture cheat!\n");
				}
				else
				{
					float v = atof(gi.argv(3));
					if (v == 0)
					{
						if (kick_flamehack->value || (ent->client->pers.spectator == SPECTATING && no_spec->value))
						{
							if (ent->client->pers.polyblender) // they have reset gl_polyblend to 0
							{
								KICKENT(ent, "%s is being kicked for having a flame hack!\n");
							}
							else
							{
								gi.WriteByte(svc_stufftext);
								gi.WriteString("gl_polyblend 2\n");
								gi.unicast(ent, true);
								ent->client->pers.polyblender = 1;
							}
						}
					}
					else if (v == 2)
						ent->client->pers.polyblender = 1;
					else
						ent->client->pers.polyblender = 0;
				}
				break;

			case 'B':
				if (gi.argc() != 3 || atof(gi.argv(1)) != 0 || atof(gi.argv(2)) != 1/* || atof(gi.argv(3)) != 1*/)
					KICKENT(ent, "%s is being kicked for using a see-thru cheat!\n");
				break;

			case 'C':
				{
					float v = atof(gi.argv(1));
					if (fabs(fabs(v) - 0.022f) > 0.0001f)
					{
						gi.WriteByte(svc_stufftext);
						if (v < 0)
							gi.WriteString("m_yaw 0.022\nm_pitch -0.022\n");
						else
							gi.WriteString("m_yaw 0.022\nm_pitch 0.022\n");
						gi.unicast(ent, true);
					}
					if (gi.argc() == 3)
						ent->client->pers.noantilag = !atoi(gi.argv(2));
				}
				break;
		}
#endif //hypov8
		return;
	}

	if (!Q_stricmp(cmd, "clientlist"))
	{
		Cmd_Status_f (ent);
		return;
	}

	if (Q_stricmp (cmd, "rconx_login") == 0)
	{
		Cmd_Rcon_login_f (ent,gi.argv(1));
		return;
	}
	if (Q_stricmp (cmd, "rconx") == 0)
	{
		Cmd_Rcon_f (ent);
		return;
	}

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		if (level.voteset && !ent->client->resp.vote)
		{
			char *arg = gi.args();
			if (Q_stricmp (arg, "\"yes\"") == 0)
			{
				Cmd_Yes_f (ent);
				return;
			}
			else if (Q_stricmp (arg, "\"no\"") == 0)
			{
				Cmd_No_f (ent);
				return;
			}
		}
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}

	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent, 0);
		return;
	}
	else if (Q_stricmp (cmd, "invnext") == 0)
	{
		SelectNextItem (ent, -1);
		return;
	}
	else if (Q_stricmp (cmd, "invprev") == 0)
	{
		SelectPrevItem (ent, -1);
		return;
	}

/*
	if (level.intermissiontime)
		return;
*/
	// JOSEPH 6-FEB-99
	if (Q_stricmp (cmd, "leftarrow") == 0)
		;
	else if (Q_stricmp (cmd, "rightarrow") == 0)
		;
	else if (Q_stricmp (cmd, "uparrow") == 0)
		;
	else if (Q_stricmp (cmd, "downarrow") == 0)
		;		
	else if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	// END JOSEPH
	else if ((Q_stricmp (cmd, "drop") == 0) && (Q_stricmp (gi.argv (1), "cash") == 0))
		Cmd_DropCash_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "immortal") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	// JOSEPH 29-DEC-98
	else if (Q_stricmp (cmd, "+activate") == 0)
		Cmd_Activate_f (ent);
	else if (Q_stricmp (cmd, "holster") == 0)
		Cmd_Holster_f (ent);
	else if (Q_stricmp (cmd, "hud") == 0)
		Cmd_Hud_f (ent);
/*	else if (Q_stricmp (cmd, "clearme") == 0)
		Cmd_Clear_f (ent);*/
	// END JOSEPH
	
	// BEGIN HOOK
	// The hook will only work if its available as a server option
	else if ((Q_stricmp(cmd, "hook") == 0))
	{
		if (sv_hook->value || HmHookAvailable) /*enable_hitmen*/
			Cmd_Hook_f(ent);
	}
	// END
	// RAFAEL
	else if (Q_stricmp (cmd, "flashlight") == 0)
		Cmd_Flashlight_f (ent);
	// RAFAEL 01-11-99
	else if (Q_stricmp (cmd, "reload") == 0)
		Cmd_Reload_f (ent);
	// END 01-11-99	
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);

	// Ridah, new 3 key command system
	else if (strstr (cmd, "key") == cmd)
		Cmd_Key_f (ent,0);

	// Ridah, Chasecam
	else if (Q_stricmp (cmd, "togglecam") == 0)
		Cmd_ToggleCam_f (ent);

	// BEGIN HITMEN
	else if (Q_stricmp (cmd, "showmotd") == 0)
		{
		ent->client->showscores = SCORE_MOTD;
		DeathmatchScoreboard (ent);
		}
	// END
	// Ridah, Vehicles
	else if (Q_stricmp (cmd, "gear_up") == 0)
		Cmd_GearUp_f (ent);
	else if (Q_stricmp (cmd, "gear_down") == 0)
		Cmd_GearDown_f (ent);

	// Teamplay commands
	else if ((Q_stricmp (cmd, "join") == 0) || (Q_stricmp (cmd, "team") == 0))
		Cmd_Join_f (ent, gi.argv (1));
	else if ((Q_stricmp (cmd, "spec") == 0) || (Q_stricmp (cmd, "spectator") == 0))
		Cmd_Spec_f (ent);

// Papa 10.6.99 Tourney Commands
	
	else if (Q_stricmp (cmd, "admin") == 0)
		Cmd_Admin_f (ent, gi.argv(1));
	else if (Q_stricmp (cmd, "resign") == 0)
		Cmd_Resign_f (ent);

	else if (Q_stricmp (cmd, "changemap") == 0)
		Cmd_ChangeMap_f (ent);
	else if (Q_stricmp (cmd, "maplist") == 0)
		Cmd_MapList_f (ent);
	else if (Q_stricmp (cmd, "commands") == 0)
		Cmd_CommandList_f (ent);
	else if (Q_stricmp (cmd, "settings") == 0)
		Cmd_PrintSettings_f (ent);

	else if (Q_stricmp (cmd, "setdmflags") == 0) 
		Cmd_SetDmFlags_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setpassword") == 0) 
		Cmd_SetPassword_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "removepassword") == 0) 
		Cmd_SetPassword_f (ent, "");
	else if (Q_stricmp (cmd, "setdm_realmode") == 0) 
		Cmd_SetRealMode_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setteamplay") == 0) 
		Cmd_SetTeamplay_f (ent, gi.argv (1));

	else if (Q_stricmp (cmd, "yes") == 0)
		Cmd_Yes_f (ent);
	else if (Q_stricmp (cmd, "no") == 0)
		Cmd_No_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent, gi.argv(1));
	else if (Q_stricmp (cmd, "elect") == 0)
		Cmd_Elect_f (ent);

	else if (Q_stricmp (cmd, "resetserver") == 0)
		Cmd_ResetServer_f(ent);

	else if (Q_stricmp (cmd, "settimelimit") == 0)
		Cmd_SetTimeLimit_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setidletime") == 0)
		Cmd_SetClientIdle_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setfraglimit") == 0)
		Cmd_SetFragLimit_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setcashlimit") == 0)
		Cmd_SetCashLimit_f (ent, gi.argv (1));
	else if (Q_stricmp (cmd, "setbonus") == 0)
		Cmd_SetBonus_f (ent, gi.argv (1));

	else if (Q_stricmp (cmd, "mute") == 0) 
		Cmd_Mute_f(ent,gi.argv(1)); 

	else if (Q_stricmp (cmd, "listbans") == 0) 
		Cmd_ListBans_f(ent);

	else if (Q_stricmp (cmd, "motd") == 0)
		Cmd_MOTD_f(ent);

	//taunt commands
	else if (Q_stricmp (cmd, "kingpin") == 0) 
		Cmd_Key_f(ent,KINGPIN); 
	else if (Q_stricmp (cmd, "leroy") == 0) 
		Cmd_Key_f(ent,LEROY); 
	else if (Q_stricmp (cmd, "mj") == 0) 
		Cmd_Key_f(ent,MJ); 
	else if (Q_stricmp (cmd, "momo") == 0) 
		Cmd_Key_f(ent,MOMO); 
	else if (Q_stricmp (cmd, "lamont") == 0) 
		Cmd_Key_f(ent,LAMONT); 
	else if (Q_stricmp (cmd, "jesus") == 0) 
		Cmd_Key_f(ent,JESUS); 
	else if (Q_stricmp (cmd, "tyrone") == 0) 
		Cmd_Key_f(ent,TYRONE); 
	else if (Q_stricmp (cmd, "willy") == 0) 
		Cmd_Key_f(ent,WILLY); 
	else if (Q_stricmp (cmd, "moker") == 0) 
		Cmd_Key_f(ent,MOKER); 
	else if (Q_stricmp (cmd, "heilman") == 0) 
		Cmd_Key_f(ent,HEILMAN); 
	else if (Q_stricmp (cmd, "bambi") == 0) 
		Cmd_Key_f(ent,BAMBI); 
	else if (Q_stricmp (cmd, "yolanda") == 0) 
		Cmd_Key_f(ent,YOLANDA); 
	else if (Q_stricmp (cmd, "lola") == 0) 
		Cmd_Key_f(ent,LOLA); 
	else if (Q_stricmp (cmd, "mona") == 0) 
		Cmd_Key_f(ent,MONA); 
	else if (Q_stricmp (cmd, "blunt") == 0) 
		Cmd_Key_f(ent,BLUNT); 
	else if (Q_stricmp (cmd, "beth") == 0) 
		Cmd_Key_f(ent,BETH); 
	else if (Q_stricmp (cmd, "curselist") == 0) 
		Cmd_CurseList_f(ent); 
	else if (strstr (cmd, "curse") == cmd)
		Cmd_Key_f (ent,0);
	else if (strstr (cmd, "taunt") == cmd)
		Cmd_Key_f (ent,0);
	//end -taunts tical

	else if (Q_stricmp (cmd, "toggle_asc") == 0) 
		Cmd_Toggle_ASC_f(ent);
	else if (Q_stricmp (cmd, "toggle_bunny") == 0) 
		Cmd_Toggle_Bunny_f(ent);
	else if (Q_stricmp (cmd, "toggle_spec") == 0) 
		Cmd_Toggle_Spec_f(ent);
	else if (Q_stricmp (cmd, "toggle_shadows") == 0) 
		Cmd_Toggle_Shadows_f(ent);
	else if (Q_stricmp (cmd, "toggle_zoom") == 0) 
		Cmd_Toggle_Zoom_f(ent);
	else if (Q_stricmp (cmd, "antilag") == 0) 
		Cmd_AntiLag_f(ent, gi.argv(1));
	//hypov8 votemap
	//else if (Q_stricmp(cmd, "votemap") == 0)	//allow map voting early map vote
	//	Cmd_VoteMap_f(ent);

	else if (Q_stricmp(cmd, "endmap") == 0)	// hypov8 call early map end, goto level select?
		Cmd_EndMap_f(ent);
//END
// ACEBOT_ADD
	else if (Q_stricmp(cmd, "votebotadd") == 0)		//hypo add bot
		Cmd_VoteAddBot_f(ent, 0);
	else if (Q_stricmp(cmd, "votebotremove") == 0)		//hypo add bot
		Cmd_VoteRemoveBot_f(ent,false, NULL);
	else if (Q_stricmp(cmd, "votebotskill") == 0)		//hypo add bot
		Cmd_VoteSkill_f(ent, false);
	//else if (Q_stricmp(cmd, "votebotcount") == 0)		//hypo todo:
	//	Cmd_VoteBotCount_f(ent);  
	else if (Q_stricmp(cmd, "menu") == 0)		//hypo add bot
		Cmd_Menu_f(ent);
	else if (Q_stricmp(cmd, "botskill") == 0) //add hypov8
		Cmd_SetBotSkill_f(ent, gi.argv(1));
// ACEBOT_END

	else if (teamplay->value)
	{
		if (Q_stricmp (cmd, "matchsetup") == 0)
			Cmd_MatchSetup_f (ent);
		else if (Q_stricmp (cmd, "matchstart") == 0)
			Cmd_MatchStart_f (ent);
		else if (Q_stricmp (cmd, "matchend") == 0)
			Cmd_MatchEnd_f (ent);
		else if (Q_stricmp (cmd, "matchscore") == 0)
			Cmd_MatchScore_f (ent);
		else if (Q_stricmp (cmd, "team1name") == 0) 
			Cmd_SetTeamName_f(ent, 1, gi.argv(1)); 
		else if (Q_stricmp (cmd, "team2name") == 0) 
			Cmd_SetTeamName_f(ent, 2, gi.argv(1)); 

		else	// anything that doesn't match a command will be a chat
		    Cmd_Say_f (ent, false, true);
	}

	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
