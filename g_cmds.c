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
//char changeMapName[32]; // = '\0';
char voteAddBot[32]; //team
char voteRemoveBot[32]; //name
float voteBotSkill; //skill 0.0 to 4.0
void EndDMLevel(void); //hypov8 add
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

#define NEUTRAL		-1

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

	if (self->solid != SOLID_NOT)
		DropCash(self);

	if (teamplay->value && self->client->pers.team)
		safe_bprintf (PRINT_HIGH, "%s left %s\n", self->client->pers.netname, team_names[self->client->pers.team]);
	else
		safe_bprintf (PRINT_HIGH, "%s became a spectator\n", self->client->pers.netname);

	self->client->pers.team = 0;
	self->client->pers.spectator = SPECTATING;
	if (self->solid != SOLID_NOT || self->deadflag || self->movetype == MOVETYPE_NOCLIP) // HYPOV8_ADD noclip todo: check?
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
		if (!self->client->showscores)
			Cmd_Score_f(self);
		self->client->resp.scoreboard_frame = 0;
	}
}

// Papa - add plater to a team
void Cmd_Join_f (edict_t *self, char *teamcmd)
{
	int	i;
	char str1[MAX_QPATH], varteam[MAX_QPATH];

	self->client->pers.idle = curtime;

	if (!teamplay->value)
	{
		if (self->client->pers.spectator != SPECTATING)
			return;
		if (level.framenum < self->client->resp.switch_teams_frame)
		{
			safe_cprintf(self, PRINT_HIGH, "Overflow protection: Unable to rejoin yet\n");
			return;
		}
		self->client->resp.switch_teams_frame = level.framenum + 20;
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
					safe_cprintf( self, PRINT_HIGH, "Already a member of %s\n", team_names[i] );
				}
				else
				{
					if (level.framenum < self->client->resp.switch_teams_frame)
					{
						safe_cprintf(self, PRINT_HIGH, "Overflow protection: Unable to change team yet\n");
						return;
					}
					self->client->resp.switch_teams_frame = level.framenum;

					if (self->client->pers.team)
						Cmd_Spec_f (self);

					if (!Teamplay_ValidateJoinTeam( self, i ))
					{
						safe_cprintf( self, PRINT_HIGH, "Unable to join %s\n", team_names[i] );
					}
				}

				return;
			}
		}

		safe_cprintf( self, PRINT_HIGH, "Un-matched team: %s\n", varteam );
	}
}

//--------------------------------------------------------
void Cmd_GearUp_f (edict_t *self)
{
	vehicle_t *vehicle;

	if (!self->vehicle_index)
	{
		safe_cprintf( self, PRINT_HIGH, "You aren't in a vehicle, can't change gears.\n");
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
		safe_cprintf( self, PRINT_HIGH, "You aren't in a vehicle, can't change gears.\n");
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

	VectorMA( start, 1200, dir, end );

	G_DoTimeShiftFor(ent);

	tr = gi.trace( start, NULL, NULL, end, ent, MASK_SHOT );

	G_UndoTimeShiftFor(ent);

	if ((tr.fraction < 1) && tr.ent->client)
		return tr.ent;

	return NULL;
}
// END JOSEPH

void Cmd_Key_f (edict_t *ent, int who)
{
	edict_t *key_ent;
	voice_table_t *voice_table;
	int num_entries = 0;

	if (disable_curse) 
		return;

	if (ent->client->pers.spectator == SPECTATING || ent->deadflag)
		return;

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	// nor when under water
	if (ent->waterlevel == 3)
		return;

	if (ent->last_talk_time > (level.time - 5))
		return;
	
	key_ent = ent->solid ? GetKeyEnt(ent) : NULL;
	if (!key_ent && (!unlimited_curse || (!ent->solid && level.last_talk_time > level.time - 3)))
		return;
	
	// neutral chat with teammates
	if (!who && ent->client->team && key_ent && key_ent->client->team == ent->client->team)
		who = NEUTRAL;

	// say something
	{
		if (ent->gender == GENDER_MALE)
		{
			switch(who)
			{
				case 0:
					voice_table = player_profanity_level2;
					num_entries = NUM_PLAYER_PROFANITY_LEVEL2;
					break;
				case NEUTRAL:
					if (rand() & 1)
					{
						voice_table = neutral_talk_player;
						num_entries = NUM_NEUTRAL_TALK_PLAYER;
					}
					else
					{
						voice_table = neutral_talk;
						num_entries = NUM_NEUTRAL_TALK;
					}
					break;
				case KINGPIN:
					voice_table = kingpin_random;
					num_entries = NUM_KINGPIN_RANDOM;
					break;
				case LEROY:
					voice_table = leroy_random;
					num_entries = NUM_LEROY_RANDOM;
					break;
				case MJ:
					voice_table = mj_random;
					num_entries = NUM_MJ_RANDOM;
					break;
				case MOMO:
					voice_table = momo_random;
					num_entries = NUM_MOMO_RANDOM;
					break;
				case LAMONT:
					voice_table = lamont_random;
					num_entries = NUM_LAMONT_RANDOM;
					break;
				case JESUS:
					voice_table = jesus_random;
					num_entries = NUM_JESUS_RANDOM;
					break;
				case TYRONE:
					voice_table = tyrone_random;
					num_entries = NUM_TYRONE_RANDOM;
					break;
				case WILLY:
					voice_table = willy_random;
					num_entries = NUM_WILLY_RANDOM;
					break;
				case MOKER:
					voice_table = moker_random;
					num_entries = NUM_MOKER_RANDOM;
					break;
				case HEILMAN:
					voice_table = heilman_random;
					num_entries = NUM_HEILMAN_RANDOM;
					break;
			}
		}
		else if (ent->gender == GENDER_FEMALE)
		{
			switch (who)
			{
				case 0:
					voice_table = f_profanity_level2;
					num_entries = F_NUM_PROFANITY_LEVEL2;
					break;
				case NEUTRAL:
					voice_table = f_neutral_talk;
					num_entries = F_NUM_NEUTRAL_TALK;
					break;
				case BAMBI:
					voice_table = bambi_random;
					num_entries = F_NUM_BAMBI_RANDOM;
					break;
				case YOLANDA:
					voice_table = yolanda_random;
					num_entries = F_NUM_YOLANDA_RANDOM;
					break;
				case MONA:
					voice_table = mona_random;
					num_entries = F_NUM_MONA_RANDOM;
					break;
				case LOLA:
					voice_table = lola_random;
					num_entries = F_NUM_LOLA_RANDOM;
					break;
				case BLUNT:
					voice_table = blunt_random;
					num_entries = F_NUM_BLUNT_RANDOM;
					break;
				case BETH:
					voice_table = beth_random;
					num_entries = F_NUM_BETH_RANDOM;
					break;
			}
		}
		if (num_entries)
		{
			int n;
			if (who > 0 && gi.argc() > 1)
			{
				n = atoi(gi.argv(1));
				if (n < 1 || n > num_entries)
				{
					safe_cprintf(ent, PRINT_HIGH, "Invalid %s taunt number, valid range: 1 - %d\n", gi.argv(0), num_entries);
					return;
				}
				n--;
			}
			else
			{
				n = rand() % num_entries;
			}
			Voice_Specific(ent, key_ent, voice_table, n);
		}
	}

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;
	switch (rand() % 3)
	{
		case 0:
			ent->s.frame = FRAME_tg_bird_01-1;
			ent->client->anim_end = FRAME_tg_bird_10;
			break;
		case 1:
			ent->s.frame = FRAME_tg_crch_grab_01-1;
			ent->client->anim_end = FRAME_tg_crch_grab_16;
			break;
		case 2:
			ent->s.frame = FRAME_tg_chin_flip_01-1;
			ent->client->anim_end = FRAME_tg_chin_flip_15;
			break;
	}
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
		if (level.vote_winner)
			return;
			ent->client->mapvote++;
		if (ent->client->mapvote == level.num_vote_set+1)
				ent->client->mapvote = 1;
			ent->client->resp.scoreboard_frame = 0;
		return;
	}
// ACEBOT_ADD //MENU
	if (ACECM_G_SelectNextItem(ent))
		return;
// ACEBOT_END //MENU

	cl = ent->client;
	
	if (cl->chase_target)
	{
		ChaseNext(ent);
		return;
	}

	Cmd_Help_f (ent, 1);
}

void SelectPrevItem (edict_t *ent, int itflags) //hypov8 flags??
{
	gclient_t	*cl;

// Papa - this is used to move up the vote map menu

	if (ent->client->showscores == SCORE_MAP_VOTE)
	{
		if (level.vote_winner)
			return;
			ent->client->mapvote--;
			if (ent->client->mapvote < 1)
			ent->client->mapvote = level.num_vote_set;
			ent->client->resp.scoreboard_frame = 0;
		return;
	}
// ACEBOT_ADD //MENU
	if (ACECM_G_SelectPrevItem(ent))
		return;
// ACEBOT_END //MENU


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

//	SelectNextItem (ent, -1);
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
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
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
		int j;
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
			//hypov8 instantly fill ammo in gun
			for (j = 0; j < MAX_WEAPONS; j++)
				ent->client->pers.weapon_clip[j] = auto_rounds[j]; //todo QweryClipIndex
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
//add hypov8. fill clip?
			item_index = QweryClipIndex(it);
			if (item_index > 0)
				ent->client->pers.weapon_clip[item_index] = auto_rounds[item_index];
//
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
			safe_cprintf (ent, PRINT_HIGH, "not a valid item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		safe_cprintf (ent, PRINT_HIGH, "non-pickup item\n");
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
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "Immortal OFF\n";
	else
		msg = "Immortal ON\n";

	safe_cprintf (ent, PRINT_HIGH, msg);
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
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	safe_cprintf (ent, PRINT_HIGH, msg);
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
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}
// HYPOV8_ADD
	if (!(level.modeset == MATCH || level.modeset == PUBLIC))
		return;

	//add hypov8 stop noclip working if allready a spectator
	if 	(ent->client->pers.spectator == SPECTATING)
		return;
// HYPOV8_END

// ACEBOT_ADD
	ent->acebot.PM_Jumping = 0;
	ent->acebot.pm_last_node = INVALID;
	ent->acebot.pm_jumpPadMove = false;
// ACEBOT_END


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
	}

	safe_cprintf (ent, PRINT_HIGH, msg);
}


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
		int vote=0;
		if (level.vote_winner)
			return;
		if (!strcmp(s, "pipe"))
			vote = 1;
		else if (!strcmp(s, "pistol"))
			vote = 2;
		else if (!strcmp(s, "shotgun"))
			vote = 3;
		else if (!strcmp(s, "tommygun"))
			vote = 4;
		else if (!strcmp(s, "heavy machinegun"))
			vote = 5;
		else if (!strcmp(s, "grenade launcher"))
			vote = 6;
		else if (!strcmp(s, "bazooka"))
			vote = 7;
		else if (!strcmp(s, "flamethrower"))
			vote = 8;
		else
			return;
		// hypov8 todo else return;
		if (vote > 0 && vote <= level.num_vote_set)
		{
			ent->client->mapvote = vote;
			ent->client->resp.scoreboard_frame = 0;
		}
		return;
	}

// ACEBOT_ADD
	if (ACECM_G_Use_f(ent, s))
		return;
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
			if (!strcmp(s, "pipe"))// team 1
				Cmd_Join_f( ent, team_names[1] );
			else if (!strcmp(s, "pistol"))// team 2
				Cmd_Join_f( ent, team_names[2] );
		}
		return;
	}

	if (ent->client->pers.spectator == SPECTATING)
	{
		if (!teamplay->value) Cmd_Join_f( ent, "" );
		return;
	}

	if (ent->solid == SOLID_NOT || ent->deadflag)
		return;


	it = FindItem (s);
	if (!it)
	{
		safe_cprintf (ent, PRINT_HIGH, "not a valid item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		
		if (strcmp (it->pickup_name, "Pipe") == 0)
		{
			it = FindItem ("Crowbar");
			index = ITEM_INDEX(it);
			if (!ent->client->pers.inventory[index])
			{
				safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else if (strcmp (it->pickup_name, "Pistol") == 0)
		{
		//	gi.dprintf ("silencer_shots: %d\n", ent->client->pers.silencer_shots);
			if (ent->client->pers.silencer_shots <= 0) //hypov8 was 0
			{
				safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
			it = FindItem ("SPistol");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
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

	if (ent->solid == SOLID_NOT) 
		return;

	s = gi.args();

	if (!Q_stricmp (s, "cash"))
	{
		if (((int)(dmflags->value) & DF_DROP_CASH) && ent->client->pers.currentcash)
		{
			edict_t *cash = Drop_Item(ent, FindItemByClassname ("item_cashroll"));
			cash->currentcash = ent->client->pers.currentcash;
			ent->client->pers.currentcash = 0;
		}
		return;
	}

	it = FindItem (s);
	if (!it)
	{
		safe_cprintf (ent, PRINT_HIGH, "not a valid item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
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
				safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
			
			it = FindItem ("SPistol");
			index = ITEM_INDEX (it);
			if (!ent->client->pers.inventory[index])
			{
				safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
		}
		else 
		{
			safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
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

	if (ent->solid == SOLID_NOT || ent->deadflag || dm_realmode->value == 3)
		return;

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

	if (ent->solid == SOLID_NOT)
		return;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		safe_cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
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
	gitem_t		*it, *newweapon;
	int			selected_weapon;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon)
		return;

	// Ridah, if already changing weapons, start from the next weapon, for faster cycling
	if (cl->weaponstate == WEAPON_DROPPING)
		selected_weapon = ITEM_INDEX(cl->newweapon);
	else
		selected_weapon = ITEM_INDEX(cl->pers.weapon);

	newweapon = cl->newweapon;

	// scan  for the next valid one
	for (i=1 ; i<MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i) % MAX_ITEMS;

		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;

		it->use (ent, it);
		if (cl->newweapon != newweapon)
		{
			it = cl->newweapon;
			// Ridah, show the current weapon on the hud, for easy scrolling
			if (!strstr(it->icon, "pipe"))
			{
				cl->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
				cl->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ITEM_INDEX(it);
				cl->pickup_msg_time = level.time + 5.5;
			}
			else
				cl->pickup_msg_time = 0;

			return;	// successful
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

	if (ent->client->showscores == SCORE_REJOIN)
		return;


// ACEBOT_ADD //MENU
	if (ACECM_G_Activate_f(ent))
		return;
// ACEBOT_END

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		if (ent->client->pers.spectator == SPECTATING && (level.modeset == PUBLIC || level.modeset == MATCH))
		{
			if (level.modeset == MATCH && no_spec->value && !ent->client->pers.admin && !ent->client->pers.rconx[0]) 
				return;
			if ((int)maxclients->value > 1)
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
			//safe_cprintf (ent, PRINT_HIGH, "%i dollars found\n", best->currentcash);
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
				target->target = "null"; //todo: fix
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
	safe_cprintf (ent, PRINT_HIGH, "no holstering\n");
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
	gitem_t		*it, *newweapon;
	int			selected_weapon;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon)
		return;

	// Ridah, if already changing weapons, start from the next weapon, for faster cycling
	if (cl->weaponstate == WEAPON_DROPPING)
		selected_weapon = ITEM_INDEX(cl->newweapon);
	else
		selected_weapon = ITEM_INDEX(cl->pers.weapon);

	newweapon = cl->newweapon;

	// scan  for the next valid one
	for (i=1 ; i<MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i) % MAX_ITEMS;
		
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;

		it->use (ent, it);
		if (cl->newweapon != newweapon)
		{
			it = cl->newweapon;
			// Ridah, show the current weapon on the hud, for easy scrolling
			if (!strstr(it->icon, "pipe"))
			{
				cl->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
				cl->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ITEM_INDEX(it);
				cl->pickup_msg_time = level.time + 5.5;
			}
			else
				cl->pickup_msg_time = 0;

			return;	// successful
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
	gitem_t		*it, *newweapon;

	cl = ent->client;

	if (cl->pers.spectator == SPECTATING)
		return;

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	newweapon = cl->newweapon;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;

	it->use (ent, it);
	if (cl->newweapon == newweapon)
	{
		it = cl->newweapon;
	// Ridah, show the current weapon on the hud, for easy scrolling
		if (!strstr(it->icon, "pipe"))
	{
			cl->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(it->icon);
			cl->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ITEM_INDEX(it);
			cl->pickup_msg_time = level.time + 5.5;

	}
		else
			cl->pickup_msg_time = 0;
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

	if (ent->solid == SOLID_NOT)
		return;


	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		safe_cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}

	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", it->pickup_name);
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
		safe_cprintf (ent, PRINT_HIGH, "no suiciding!\n");
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
// ACEBOT_ADD
	if (ACECM_G_PutAway_f(ent))
		return;
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
		ent->client->resp.scoreboard_frame = level.framenum + 20; // 50 - 30
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

	safe_cprintf(ent, PRINT_HIGH, 
		"num score ping name            lastmsg country\\skill\n"
		"--- ----- ---- --------------- ------- ---------------------\n");
	for (a=1; a<=(int)maxclients->value; a++)
	{
		gclient_t *c = g_edicts[a].client;
		if (c && (g_edicts[a].inuse || (c->pers.connected && (kpded2 || curtime-c->pers.lastpacket < 120000))))
		{
			char buf[16];
			Com_sprintf(buf,sizeof(buf), "%s", g_edicts[a].inuse ? va("%i",c->ping) : "CNCT");
// ACEBOT_ADD
			if (g_edicts[a].acebot.is_bot)
				safe_cprintf(ent, PRINT_HIGH, "%3d %5d %4s %-15s %7d %s\n",
					a-1, c->resp.score, buf, c->pers.netname, curtime-c->pers.lastpacket, va("(bot) %1.1fx", g_edicts[a].acebot.botSkillMultiplier) );
			else
// ACEBOT_END
				safe_cprintf(ent, PRINT_HIGH, "%3d %5d %4s %-15s %7d %s\n",
					a-1, c->resp.score, buf, c->pers.netname, curtime-c->pers.lastpacket, c->pers.country ? c->pers.country : "");

			count++;
		}
	}

	safe_cprintf (ent, PRINT_HIGH, "\n%i players\n", count);
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

	ent->client->pers.idle = curtime;

	if (ent->client->pers.mute)
		return;
	if (gi.argc () < 2 && !arg0)
		return;
	if (!arg0 && !*gi.argv(1)) //dont print empty chat
		return;

	if (!teamplay->value && !((int)(dmflags->value) & (DF_MODELTEAMS /*| DF_SKINTEAMS*/))
		&& ent->client->pers.spectator != SPECTATING)
		team = false;

	if (ent->client->pers.admin > NOT_ADMIN && level.modeset >= MATCHSETUP && (ent->client->pers.spectator == SPECTATING || level.modeset == MATCHSETUP))
	{
		if (team)
			Com_sprintf (text, sizeof(text), ">(admin:%s): ", ent->client->pers.netname);
		else
			Com_sprintf (text, sizeof(text), ">admin:%s: ", ent->client->pers.netname);
	}
	else if (team)
		Com_sprintf (text, sizeof(text), ">(%s): ", ent->client->pers.netname);
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

	if (strchr(text, '\r'))
	{
		safe_cprintf(ent, PRINT_CHAT, "Anonymous text is not allowed on this server\n");
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
			safe_cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
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


//===================================================================================
// Papa 10.6.99
//===================================================================================

//===================================================================================
// This is the start for all the commands that players can enter to run the mod.
//===================================================================================

void Cmd_PrintSettings_f (edict_t *ent)
{
	safe_cprintf(ent, PRINT_HIGH,"\nCurrent Game Settings\n"
		"=====================\n");
	switch (level.modeset)
	{
		case PREGAME :
		case PUBLIC :
			safe_cprintf(ent, PRINT_HIGH, "Server State    : Public\n");
			break;
		case MATCHSETUP :
			safe_cprintf(ent, PRINT_HIGH, "Server State    : Match Setup\n");
			break;
		case MATCHCOUNT :
			safe_cprintf(ent, PRINT_HIGH, "Server State    : Match Countdown\n");
			break;
		case MATCH :
			safe_cprintf(ent, PRINT_HIGH, "Server State    : Match\n");
			break;
	}
	if (level.modeset == MATCHSETUP)
		safe_cprintf(ent, PRINT_HIGH,"Matchstart score: %d : %d\n", team_startcash[0], team_startcash[1]);
	safe_cprintf(ent, PRINT_HIGH,"Time limit      : %d\n", (int)timelimit->value);
	safe_cprintf(ent, PRINT_HIGH,"Frag limit      : %d\n", (int)fraglimit->value);
	safe_cprintf(ent, PRINT_HIGH,"Cash limit      : %d\n", (int)cashlimit->value);
	safe_cprintf(ent, PRINT_HIGH,"dmflags         : %d\n", (int)dmflags->value);
	safe_cprintf(ent, PRINT_HIGH,"dm_realmode     : %d\n", (int)dm_realmode->value);
	safe_cprintf(ent, PRINT_HIGH,"Teamplay mode   : %d\n", (int)teamplay->value);
	if (!teamplay->value)
		safe_cprintf(ent, PRINT_HIGH,"Bonus hit       : %d\n", (int)bonus->value);
	safe_cprintf(ent, PRINT_HIGH,"Bunny-hopping   : %s\n", (int)dmflags->value&DF_NO_BUNNY ? "off" : "on");
	safe_cprintf(ent, PRINT_HIGH,"Anti-spawncamp  : %s\n", anti_spawncamp->value? "on" : "off");
	safe_cprintf(ent, PRINT_HIGH,"Shadows         : %s\n", no_shadows->value? "off" : "on");
	safe_cprintf(ent, PRINT_HIGH,"FOV zooming     : %s\n", no_zoom->value? "off" : "on");
	if (level.modeset >= MATCHSETUP && level.modeset <= MATCH)
		safe_cprintf(ent, PRINT_HIGH,"Spectating      : %s\n", no_spec->value? "off" : "on");
	if (password->string[0])
		safe_cprintf(ent, PRINT_HIGH,"Server password : %s\n", password->string);
	{
		edict_t *admin = GetAdmin();
		if (admin)
			safe_cprintf(ent, PRINT_HIGH, "Current admin   : %s\n", admin->client->pers.netname);
	}
}

void Cmd_CurseList_f (edict_t *ent)
{
	if (disable_curse)
	{
		safe_cprintf(ent, PRINT_HIGH,"Player taunts are disabled\n");
		return;
	}
	safe_cprintf(ent, PRINT_HIGH,"\nList of Curse Commands\n"
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

	safe_cprintf(ent, PRINT_HIGH, "\nCurrent Console Commands\n"
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

	strcat(buf, " \n");

	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (teamplay->value)
			strcat(buf, "matchsetup, matchscore, matchstart, matchend, team1name, team2name\n");
		strcat(buf, "resetserver, settimelimit, setfraglimit, setcashlimit, setdmflags\n" "endmap, setidletime, toggle_asc, toggle_bunny, toggle_shadows, toggle_zoom");
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

	safe_cprintf(ent, PRINT_HIGH, "%s\n", buf);

	if (ent->client->pers.admin > NOT_ADMIN && !fixed_gametype)
		safe_cprintf(ent, PRINT_HIGH,
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
	int			i, nop, novy, idle;
	char		command [256];

	if (level.voteset == NO_VOTES)
		safe_cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time\n");
	else if (ent->client->resp.vote != HASNT_VOTED)
		safe_cprintf(ent, PRINT_HIGH, "You have already voted\n");
	else
	{
		ent->client->resp.vote = YES;
		nop=0;
		novy=0;
		idle = 0;
		for_each_player_not_bot(dude, i)// ACEBOT_ADD
		{
			if ((dude->client->resp.vote == YES) || (dude->client->resp.vote == CALLED_VOTE))
				novy ++;
			else if (!dude->client->resp.vote && curtime - dude->client->pers.idle > 120000)
				idle++;
			nop++;
		}
		if ((novy *2) > (nop - idle))
		{
			switch (level.voteset) // Papa - if you wanted to add different types of votes, you could do it here
			{
				case VOTE_ON_ADMIN:
					for_each_player_not_bot(dude, i)// ACEBOT_ADD
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
					//ACESP_RemoveBot("all", false);
// ACEBOT_END
					safe_bprintf (PRINT_HIGH, "The map change vote has passed\n");
					if (teamplay->latched_string || dm_realmode->latched_string|| sv_hitmen->latched_string)
						Com_sprintf (command, sizeof(command), "map \"%s\"\n", votemap);
					else
						Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", votemap);
					gi.AddCommandString (command);
					break;

// ACEBOT_ADD			
				//hypov8 todo?: check this
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

							ACESP_RemoveBot(voteRemoveBot, true);
						}
					break;
				case VOTE_BOTSKILL:
					{
						char sSkill[16];
						Com_sprintf(sSkill, sizeof(sSkill), "%1.1f", voteBotSkill);
						gi.cvar_set("sv_botskill", sSkill);
						safe_bprintf(PRINT_HIGH, "Bot-Skill set to %d of 10 (sv_botskill %s)\n", ACECM_ReturnBotSkillWeb(), sv_botskill->string);
						safe_cprintf(ent, PRINT_HIGH, "This setting takes effect immediately\n");
					}
					break;
// ACEBOT_END
			}

			/*for_each_player_not_bot(dude, i)// ACEBOT_ADD
			{
				if (dude->client->resp.vote == CALLED_VOTE)
					dude->client->resp.vote = HASNT_VOTED;
			}*/

			level.voteset = NO_VOTES;
		}
		else
			safe_bprintf (PRINT_HIGH, "%d have voted YES (%d more needed)\n", novy, (nop - idle) / 2 + 1 - novy);
	}
	//safe_cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time!\n");
}

void Cmd_No_f (edict_t *ent)
{
	edict_t		*dude;
	int			i, nop, novn;

	if (level.voteset == NO_VOTES)
		safe_cprintf(ent, PRINT_HIGH, "There is nothing to vote on at this time\n");
	else if (ent->client->resp.vote != HASNT_VOTED)
		safe_cprintf(ent, PRINT_HIGH, "You have already voted\n");
	else
	{
		ent->client->resp.vote = NO;
		nop = 0;
		novn = 0;
		for_each_player_not_bot(dude, i)// ACEBOT_ADD
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
			/*for_each_player_not_bot(dude, i)// ACEBOT_ADD
			{
				if (dude->client->resp.vote == CALLED_VOTE)
					dude->client->resp.vote = HASNT_VOTED;
			}*/

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
		safe_cprintf(ent, PRINT_HIGH, "Vote YES or NO\n");
}

void Cmd_Elect_f (edict_t *ent)
{
	edict_t		*dude;
	int			count=0;
	int			i;

	if (disable_admin_voting)
	{
		safe_cprintf(ent, PRINT_HIGH, "Electable admins has been disabled on this server\n");
		return;
	}

	dude = GetAdmin();
	if (dude)
	{
		if (dude == ent)
			safe_cprintf(ent, PRINT_HIGH, "You already have admin\n");
		else
			safe_cprintf(ent, PRINT_HIGH, "%s already has admin\n", dude->client->pers.netname);
		return;
	}

	if (level.voteset == NO_VOTES)
	{
		for_each_player_not_bot(dude, i)// ACEBOT_ADD
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
		safe_cprintf(ent, PRINT_HIGH, "A vote is already in progress\n");
}

void Cmd_MapList_f (edict_t *ent)
{
	int		i;

	if (!num_maps)
	{
		safe_cprintf(ent, PRINT_HIGH, "Any map on this server may be loaded\n");
		return;
	}

	safe_cprintf(ent, PRINT_HIGH, "Valid maps for this server:\n");
	i = 0;
	if (num_maps > 20)
		for (; i<num_maps-1; i+=2)
			safe_cprintf(ent, PRINT_HIGH, "   %-17s   %s\n", maplist[i], maplist[i+1]);
	for (; i<num_maps; i++)
		safe_cprintf(ent, PRINT_HIGH, "   %s\n", maplist[i]);
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
		if (file_exist(va("maps/%s.bsp", mapname)))
			return true;
		if (kpded2)
		{
			// check for an "override" file
			if (file_exist(va("maps/%s.bsp.override", mapname)))
				return true;
		}
	}
	return false;
}

void Cmd_ChangeMap_f (edict_t *ent, qboolean vote)
{
	char		*s;
	char		command [256];

	if (gi.argc() < 2)
	{
		safe_cprintf(ent, PRINT_HIGH, "Usage: changemap <mapname>\n");
		return;
	}

	s = gi.args();
	kp_strlwr(s);
	if (!ValidMap (s)) // Always make sure the map is on the server before switching
	{
		safe_cprintf(ent, PRINT_HIGH, "%s is not a valid map\n", s);
		return;
	}

	if (!ent->client->pers.admin || vote)
	{
		if (level.voteset == NO_VOTES)  
		{
			edict_t		*dude;
			int			count=0;
			int			i;

			for_each_player_not_bot(dude, i)// ACEBOT_ADD
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
			safe_cprintf(ent, PRINT_HIGH, vote ? "A vote is already in progress\n" : "You do not have admin and a vote is already in progress\n");
			return;
		}
	}

// ACEBOT_ADD
	ACECM_LevelEnd();
	PrintScoreMatchEnd();
	//ACESP_RemoveBot("all", false);
// ACEBOT_END

	if (teamplay->latched_string || dm_realmode->latched_string|| sv_hitmen->latched_string)
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_MatchStart_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
		MatchStart ();
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}



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
				EndDMLevel(); //hypov8 todo: urm.. call it now?
		}
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin.\n");
}
//END



void Cmd_MatchEnd_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (level.modeset != MATCH)
			safe_cprintf(ent, PRINT_HIGH, "A match has not been started\n");
		else
			MatchEnd ();
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
            safe_cprintf(dude,PRINT_HIGH,"You are phrozen.\n");
        }
    }
    else
		safe_cprintf(ent,PRINT_HIGH,"You do not have permission\n");
}*/

void Cmd_MatchScore_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int t0, t1;
		if (level.modeset != MATCHSETUP)
		{
			safe_cprintf(ent, PRINT_HIGH, "Can only set the scores when in matchsetup mode\n");
			return;
		}
		if (gi.argc() != 3)
		{
			safe_cprintf(ent, PRINT_HIGH, "Usage: matchscore <drags> <nicks>\n");
			return;
		}
		t0 = atoi(gi.argv(1));
		t1 = atoi(gi.argv(2));
		if (t0 > 9999 || t1 > 9999)
		{
			safe_cprintf(ent, PRINT_HIGH, "You can not set scores that high\n");
			return;
		}
		team_startcash[0] = t0;
		team_startcash[1] = t1;
		safe_bprintf(PRINT_HIGH, "The match score will start at %d (%s) to %d (%s)\n", team_startcash[0], team_names[1], team_startcash[1], team_names[2]);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetTimeLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if ((i < 0) || (i > 60))
		{
			safe_cprintf(ent, PRINT_HIGH, "Please choose a timelimit between 0 and 60\n");
			return;
		}
		gi.cvar_set("timelimit", value);
		safe_bprintf(PRINT_HIGH, "The timelimit has been changed to %d\n", i);
		UpdateTime();
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetClientIdle_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 60)
		{
			safe_cprintf(ent, PRINT_HIGH, "Please choose an idle time of at least 60\n");
			return;
		}
		gi.cvar_set("idle_client", value);
		safe_bprintf(PRINT_HIGH, "The idle time has been changed to %d seconds\n", i);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetFragLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0) 
		{
			safe_cprintf(ent, PRINT_HIGH, "Please choose a positive fraglimit\n");
			return;
		}
		gi.cvar_set("fraglimit", value);
		safe_bprintf(PRINT_HIGH, "The fraglimit has been changed to %d\n", i);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetCashLimit_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0 ) 
		{
			safe_cprintf(ent, PRINT_HIGH, "Please choose a positive cashlimit\n");
			return;
		}
		gi.cvar_set("cashlimit", value);
		safe_bprintf(PRINT_HIGH, "The cashlimit has been changed to %d\n",i);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
			safe_cprintf(ent, PRINT_HIGH, "This server's game type may not be changed\n");
			return;
		}
		gi.cvar_set("dm_realmode", value);
		safe_cprintf(ent, PRINT_HIGH, "This setting will take effect with a map change\n");
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}



void Cmd_SetPassword_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (enable_password)
			gi.cvar_set("password", value);
		else
			safe_cprintf(ent, PRINT_HIGH, "The password option is disabled\n");
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetTeamplay_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (fixed_gametype)
		{
			safe_cprintf(ent, PRINT_HIGH, "This server's game type may not be changed\n");
			return;
		}
		if (value[0] && (i == 0) || (i == 1) || (i == 4))
		{
			gi.cvar_set("teamplay", value);
			safe_cprintf(ent, PRINT_HIGH, "This setting will take effect with a map change\n");
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "Teamplay settings are as follows:\n 0: Standard DM\n 1: Bagman\n 4: Team DM\n");
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_SetBonus_f (edict_t *ent, char *value)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		int		i = atoi (value);
		if (i < 0) 
		{
			safe_cprintf(ent, PRINT_HIGH, "Please choose a positive bonus hit deficit\n");
			return;
		}
		gi.cvar_set("bonus", value);
		safe_bprintf(PRINT_HIGH, "The bonus hit deficit has been changed to %d\n", i);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_ResetServer_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN || ent->client->pers.rconx[0]) 
		ResetServer(false);
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have permission\n");
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Toggle_Spec_f(edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN)
	{
		if (level.modeset < MATCHSETUP || level.modeset > MATCH)
		{
			safe_cprintf(ent, PRINT_HIGH, "Spectating can only be disabled in match mode\n");
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
				for_each_player_not_bot(dood, i)// ACEBOT_ADD
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
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
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_AntiLag_f(edict_t *ent, char *value)
{
	if (!value[0])
	{
		safe_cprintf(ent, PRINT_HIGH,	"Usage: antilag <0/1>\n"
										"0 = off, 1 = on (if enabled by server)\n");
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

		safe_cprintf(ent, PRINT_HIGH, "map : %s\n", level.mapname);
		safe_cprintf(ent, PRINT_HIGH, "num score ping name            lastmsg address               ver\n"
			"--- ----- ---- --------------- ------- --------------------- ----\n");
		for (a=1; a<=(int)maxclients->value; a++)
		{
			gclient_t *c = g_edicts[a].client;
			if (c && (g_edicts[a].inuse || (c->pers.connected && (kpded2 || curtime-c->pers.lastpacket < 120000))))
			{
				char buf[16];
				Com_sprintf(buf,sizeof(buf), g_edicts[a].inuse ? "%d" : "CNCT", c->ping);
				safe_cprintf(ent, PRINT_HIGH, "%3d %5d %4s %-15s %7d %-21s %.2f\n",
					a-1, c->resp.score, buf, c->pers.netname, curtime-c->pers.lastpacket, c->pers.ip, c->pers.version/100.0);
			}
		}
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have permission\n");
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
			safe_cprintf(ent, PRINT_HIGH, "Team name can't be longer than 15 letters or start with whitespace\n");
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n");
}

void Cmd_Admin_f (edict_t *ent, char *value)
{
	cvar_t *rcon_password;

	if (ent->client->pers.admin == ADMIN)
	{
		safe_cprintf(ent, PRINT_HIGH, "You already have admin\n");
		return;
	}
	if (level.framenum < ent->client->resp.login_frame)
	{
		safe_cprintf(ent, PRINT_HIGH, "Only 1 attempt allowed per second\n");
		return;
	}
	ent->client->resp.login_frame = level.framenum + 10;

	rcon_password = gi.cvar("rcon_password", "", 0);
	if (ent->client->pers.rconx[0] || (admincode[0] && !strcmp(admincode, value)) || (rcon_password->string[0] && !strcmp(rcon_password->string, value)))
	{
		if (!ent->client->pers.admin)
		{
			edict_t *admin = GetAdmin();
			if (admin)
			{
				if (admin->client->pers.admin == ADMIN)
				{
					safe_cprintf(ent, PRINT_HIGH, "%s already has admin\n", admin->client->pers.netname);
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
		safe_cprintf(ent, PRINT_HIGH, "Incorrect admin code\n");
}

void Cmd_Resign_f (edict_t *ent)
{
	if (ent->client->pers.admin > NOT_ADMIN) 
	{
		ent->client->pers.admin = NOT_ADMIN;
		safe_bprintf(PRINT_HIGH, "%s is no longer admin\n", ent->client->pers.netname);
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have admin\n"); 
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
				if (num_ban_ips < 100)
				{
					strncpy(ban_ip[num_ban_ips].value, value, 16);
					num_ban_ips++;
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
				if (num_ban_names < 100)
				{
					strncpy(ban_name[num_ban_names].value, value, 16);
					num_ban_names++;
				}
			}
			else
				safe_cprintf(ent, PRINT_HIGH, "Name banning is disabled\n");
		}
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have permission\n");
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
			for (a=0; a<num_ban_ips; a++)
				safe_cprintf(ent, PRINT_HIGH, "%s\n", ban_ip[a].value);
			safe_cprintf(ent, PRINT_HIGH, "\n"); // cosmetic stuff
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "IP banning is disabled\n");
    
		if (ban_name_filename[0]) 
		{
			safe_cprintf(ent, PRINT_HIGH, "List of banned names\n====================\n");
			for (a=0; a<num_ban_names; a++)
				safe_cprintf(ent, PRINT_HIGH, "%s\n", ban_name[a].value);
			safe_cprintf(ent, PRINT_HIGH, "\n"); // cosmetic stuff
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "Name banning is disabled\n");
	}
	else 
		safe_cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void checkkick(edict_t *ent, char *cmd, char *action)
{
	int a;
	char *name;

	if (gi.argc() < 3 || gi.argc() > 4)
	{
		safe_cprintf(ent, PRINT_HIGH, "Usage: %s <userid> <reason>\nNOTE: <reason> is optional\n", cmd);
		return;
	}

	name = gi.argv(2);
	a = atoi(name);
	if (!strcmp(name,"0") || (a > 0 && a < (int)maxclients->value))
	{
		a++;
		if (g_edicts[a].inuse)
		{
			if (gi.argc() == 4)
    			safe_bprintf(PRINT_HIGH, "%s is being %s by %s because %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname, gi.argv(3));
			else
				safe_bprintf(PRINT_HIGH, "%s is being %s by %s\n", g_edicts[a].client->pers.netname, action, ent->client->pers.netname);
		}
		else
			safe_cprintf(ent, PRINT_HIGH, "Client %s is not active\n", name);
	}
	else
	{
		for (a=1; a<=(int)maxclients->value; a++)
			if (g_edicts[a].inuse && !Q_strcasecmp(g_edicts[a].client->pers.netname, name))
				break;
		if (a > (int)maxclients->value)
			safe_cprintf(ent,PRINT_HIGH,"Userid %s is not on the server\n",name);
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

	safe_cprintf(ent, PRINT_HIGH, "userinfo\n--------\n");
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
		safe_cprintf(ent, PRINT_HIGH, "%-20s %.*s\n", buf, s - info, info);
		info = s;
	}
}

void Cmd_Mute_f (edict_t *ent, char *value)
{
	if (ent == NULL  // via sv rcon
		|| ent->client->pers.admin > ELECTED || ent->client->pers.rconx[0]) 
	{
		int		i = atoi (value);
		if (!*value || (i < 0 || (i+1) > (int)maxclients->value) || !g_edicts[i+1].inuse)
		{
			if (*value) safe_cprintf(ent, PRINT_HIGH, "Unable to find client id match\n");
			safe_cprintf(ent, PRINT_HIGH, "Usage: mute <userid>\n");
			return; 
		}  
		if (g_edicts[i+1].client->pers.mute == 0)
		{
			safe_cprintf(ent, PRINT_HIGH, "Enabled mute on %s\n", g_edicts[i + 1].client->pers.netname);
			safe_cprintf(&g_edicts[i+1], PRINT_HIGH, "Admin has muted you\n");
			g_edicts[i+1].client->pers.mute = 1; 
		}
		else
		{
			safe_cprintf(ent, PRINT_HIGH, "Disabled mute on %s\n", g_edicts[i + 1].client->pers.netname);
			safe_cprintf(&g_edicts[i+1], PRINT_HIGH, "Admin has unmuted you\n");
			g_edicts[i+1].client->pers.mute = 0; 
		}
	}
	else
		safe_cprintf(ent, PRINT_HIGH, "You do not have permission\n");
}

void Cmd_Rcon_f (edict_t *ent)
{
	char *cmd, cmdline[256];
	int a;

	if (!num_rconx_pass)
	{
		safe_cprintf(ent, PRINT_HIGH, "The rconx options are disabled\n");
		return;
	}

	if (!ent->client->pers.rconx[0])
	{
		safe_cprintf(ent, PRINT_HIGH, "You must login with \"rconx_login\" before using \"rconx\"\n");
		return;
	}

	if (gi.argc() < 2) 
		return;
	cmd = gi.argv(1);
	if (!Q_strncasecmp(cmd, "rcon", 4)) 
		return;

	strcpy(cmdline, cmd);
	for (a=2; a<gi.argc(); a++)
	{
		// reason hack
		if ((!Q_stricmp(cmd, "kick") || !Q_stricmp(cmd, "kickban"))	&& gi.argc() == 4 && a == 3) 
			break;
    
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
		safe_cprintf(ent, PRINT_HIGH, "Server info settings:\n");
		safe_cprintf(ent, PRINT_HIGH, "mapname              %s\n", level.mapname);
		while (v)
		{
			if ((v->flags & CVAR_SERVERINFO) && v->string[0])
				safe_cprintf(ent, PRINT_HIGH, "%-20s%s\n", v->name, v->string);
			v = v->next;
		}
		return;
	}
	else if (!Q_stricmp(cmd, "dumpuser"))
	{
		char *name;
		if (gi.argc() != 3)
		{
			safe_cprintf(ent, PRINT_HIGH, "Usage: %s <userid>\n", cmd);
			return;
		}
		name = gi.argv(2);
		a = atoi(name);
		if (!strcmp(name, "0") || (a > 0 && a < (int)maxclients->value))
		{
			a++;
			if (g_edicts[a].inuse)
				dumpuser(ent, g_edicts + a);
			else
				safe_cprintf(ent, PRINT_HIGH, "Client %s is not active\n", name);
		}
		else
		{
			for (a=1; a<=(int)maxclients->value; a++)
				if (g_edicts[a].inuse && !Q_strcasecmp(g_edicts[a].client->pers.netname, name)) 
				break;
			if (a>(int)maxclients->value)
				safe_cprintf(ent, PRINT_HIGH, "Userid %s is not on the server\n", name);
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
// ACEBOT_ADD //hypov8 todo: test this
	else if (!Q_stricmp(cmd, "gamemap") || !Q_stricmp(cmd, "map"))
	{
		ACECM_LevelEnd();
		PrintScoreMatchEnd();
		ACESP_RemoveBot("all", false);
	}
// ACEBOT_END
	else if (gi.argc() == 2)
	{
		char *val=gi.cvar(cmd, "", 0)->string;
		if (val[0])
		{
			safe_cprintf(ent, PRINT_HIGH, "\"%s\" is \"%s\"\n", cmd, val);
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
		safe_cprintf(ent, PRINT_HIGH, "The rconx options are disabled\n");
		return;
	}


	if (level.framenum < ent->client->resp.login_frame)
	{
		safe_cprintf(ent, PRINT_HIGH, "Only 1 attempt allowed per second\n");
		return;
	}
	ent->client->resp.login_frame = level.framenum + 10;

	for (a=0; a<num_rconx_pass; a++)
		if (!Q_stricmp(pass, rconx_pass[a].value)) 
			break;
	if (a == num_rconx_pass)
	{
		ent->client->pers.rconx[0] = 0;
		safe_cprintf(ent, PRINT_HIGH, "Invalid rconx login\n");
		return;
	}
	strcpy(ent->client->pers.rconx, pass);
	time(&t);
	gi.dprintf("rconx_login (%s:%s) ip: %s time: %s", ent->client->pers.netname, ent->client->pers.rconx, ent->client->pers.ip, ctime(&t));
	safe_cprintf(ent, PRINT_HIGH, "Successfully logged in\n");
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

	if (!ent->inuse)
		return;		// not fully in game yet

	ent->client->pers.lastpacket = curtime;

// ACEBOT_ADD
	if (ACECM_Commands(ent))
		return;
// ACEBOT_END

	cmd = gi.argv(0);

	if (!strncmp(cmd, cmd_check, 3))

	{
		int argc = gi.argc();
		if (!cmd[3])

		{
			// $m_pitch $antilag
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
			if (argc == 3)
				ent->client->pers.noantilag = !atoi(gi.argv(2));
		}
		else
		{
#ifndef HYPODEBUG //remove mm kick commands
			int c = atoi(cmd + 3);
			if (c != ent->client->resp.checked)
				return;
			if (c & 1)
			{
				// $vid_gamma $gl_picmip $intensity $gl_maxtexsize
				if (argc != 5 || atof(gi.argv(1)) < 0.2999f || atof(gi.argv(2)) != 0 || atof(gi.argv(3)) != 2 || atof(gi.argv(4)) < 16)
				{
					char stuff[64] = "";
					if (argc == 5 && level.framenum < ent->client->resp.enterframe + 100)
					{
						if (atof(gi.argv(1)) < 0.2999f)
							strcat(stuff, "set vid_gamma 0.3;");
						if (atof(gi.argv(3)) != 2)
							strcat(stuff, "set intensity 2;");
						if (stuff[0])
						{
							strcat(stuff, "vid_restart\r\n");
							gi.WriteByte(svc_stufftext);
							gi.WriteString(stuff);
							gi.unicast(ent, true);
						}
					}
					if (!stuff[0])
					{
						if (atof(gi.argv(1)) < 0.2999f)
						{
							KICKENT(ent, "%s is being kicked for using a lighting cheat!\n");
						}
						else
				{
					KICKENT(ent, "%s is being kicked for using a texture cheat!\n");
				}
					}
				}
			}
			else
			{
				// $gl_clear $r_showbbox $gl_polyblend $r_debug_lighting
				if (argc != 5 || atof(gi.argv(2)) != 0)
				{
					KICKENT(ent, "%s is being kicked for using a cheat!\n");
				}
				else if (atof(gi.argv(1)) != 0)
				{
					KICKENT(ent, "%s is being kicked for using a see-thru cheat!\n");
				}
				else if (atof(gi.argv(4)) != 0)
				{
					KICKENT(ent, "%s is being kicked for using a lighting cheat!\n");
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
								KICKENT(ent, "%s is being kicked for using a flame hack!\n");
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
					else
						ent->client->pers.polyblender = (v == 2);
				}
			}
#endif
			ent->client->resp.checked += 100;
		}

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
	//else if ((Q_stricmp (cmd, "drop") == 0) && (Q_stricmp (gi.argv (1), "cash") == 0))
	//	Cmd_DropCash_f (ent);
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

// BEGIN HOOK
	// The hook will only work if its available as a server option
	else if ((Q_stricmp(cmd, "hook") == 0))
	{
		if (sv_hook->value || HmHookAvailable) /*enable_hitmen*/
			Cmd_Hook_f(ent);
	}
// END
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
		Cmd_ChangeMap_f (ent, false);
	else if (Q_stricmp (cmd, "votemap") == 0)
		Cmd_ChangeMap_f (ent, true);
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
