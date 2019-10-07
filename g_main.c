
#include "g_local.h"

#include "voice_bitch.h"
#include "voice_punk.h"

// BEGIN HITMEN
#include "g_hitmen.h"
//#include "stdlog.h"    // StdLog
//#include "gslog.h"    // StdLog
// END
game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;


int meansOfDeath;

edict_t		*g_edicts;

int		num_object_bounds=0;
object_bounds_t	*g_objbnds[MAX_OBJECT_BOUNDS];

//cvar_t	*deathmatch;

//cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*cashlimit;
cvar_t	*password;
cvar_t	*maxclients;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*dedicated;

// BEGIN HOOK
cvar_t  *hook_is_homing;
cvar_t  *hook_homing_radius;
cvar_t  *hook_homing_factor;
cvar_t  *hook_players;
cvar_t  *hook_sky;
cvar_t  *hook_min_length;
cvar_t  *hook_max_length;
cvar_t  *hook_pull_speed;
cvar_t  *hook_fire_speed;
cvar_t  *hook_messages;
cvar_t  *hook_vampirism;
cvar_t  *hook_vampire_ratio;
cvar_t  *hook_hold_time;
// END
cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

// ACEBOT_ADD
cvar_t *sv_botskill;
cvar_t *sv_bot_hunt;
cvar_t *sv_botpath;
cvar_t *sv_bot_allow_add;
cvar_t *sv_bot_allow_skill;
cvar_t *sv_bot_max;
cvar_t *sv_bot_max_players;
cvar_t *sv_hitmen;
cvar_t *sv_hook; //add hypov8
// ACEBOT_END

cvar_t *sv_keeppistol;//add hypov8
cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;
cvar_t	*no_spec;
cvar_t	*no_shadows;
cvar_t	*no_zoom;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t  *kick_flamehack;
cvar_t  *anti_spawncamp;
cvar_t  *idle_client;

// Ridah, new cvar's
cvar_t	*developer;

cvar_t	*g_vehicle_test;

cvar_t	*dm_locational_damage;

cvar_t	*showlights;

cvar_t	*timescale;

cvar_t	*teamplay;
cvar_t	*g_cashspawndelay;

cvar_t	*dm_realmode;

cvar_t	*g_mapcycle_file;
// Ridah, done.

cvar_t	*antilag;
cvar_t	*props;
cvar_t	*shadows;
cvar_t	*bonus;

qboolean	kpded2;

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);

//===================================================================

int gameinc = 0;

void ShutdownGame (void)
{
	int			i;
	edict_t		*ent;
	char buf[MAX_INFO_STRING];

// ACEBOT_ADD
	ACECM_LevelEnd();
	//PrintScoreMatchEnd();
// ACEBOT_END

	buf[0] = 0;
	for (i=0 ; i< (int)maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (ent->inuse && ent->client->pers.rconx[0])
			Info_SetValueForKey(buf, ent->client->pers.ip, ent->client->pers.rconx);
	}
	gi.cvar_set("rconx", buf);

	if (keep_admin_status)
	{
		buf[0] = 0;
		ent = GetAdmin();
		if (ent)
		{
			Info_SetValueForKey(buf, "ip", ent->client->pers.ip);
			Info_SetValueForKey(buf, "type", ent->client->pers.admin == ADMIN ? "2" : "1");
		}
		gi.cvar_set("modadmin", buf);
	}
	
	gi.dprintf ("==== ShutdownGame ====\n");

// BEGIN:	Xatrix/Ridah/Navigator/21-mar-1998
	NAV_PurgeActiveNodes (level.node_data);	
// END:		Xatrix/Ridah/Navigator/21-mar-1998

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);

	gi.ClearObjectBoundsCached();	// make sure we wipe the cached list

}

int *GetNumObjectBounds (void)
{
	return &num_object_bounds;
}

void *GetObjectBoundsPointer (void)
{
	return (void *)(&g_objbnds);
}

int GetNumJuniors (void)
{
	return level.num_light_sources;
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
#if __linux__
__attribute__((visibility("default")))
#endif
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	globals.GetNumObjectBounds = GetNumObjectBounds;
	globals.GetObjectBoundsPointer = GetObjectBoundsPointer;

	globals.GetNumJuniors = GetNumJuniors;

	// check if the server is kpded2
	kpded2 = !strncmp(gi.cvar("version", "", 0)->string, "kpded", 5);

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	gi.error ("%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

#endif

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<(int)maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || ent->client->chase_target)
			continue;
		ClientEndServerFrame (ent);
	}
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client->chase_target)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
MapCycleNext
=================
*/
char *MapCycleNext( char *map )
{
	static char	nextmap[MAX_QPATH];
	int		i;

	if (!num_maps) 
		return NULL;

	for (i=0; i<num_maps; i++)
	{
		if (!Q_stricmp(maplist[i], level.mapname))
		{
			if (++i == num_maps) 
				i = 0;
			strcpy(nextmap, maplist[i]);
			return nextmap;
		}
	}
	strcpy(nextmap, maplist[default_random_map ? rand() % num_maps : 0]);
	return nextmap;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	edict_t		*ent;
	char		*nextmap;

// ACEBOT_ADD
	ACECM_LevelEnd();
	PrintScoreMatchEnd();
// ACEBOT_END

	level.startframe = level.framenum;
	level.modeset = ENDGAME;

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.mapname;
		level.startframe +=10; //add hypov8 why?

		goto done;
	}

	if (level.num_vote_set)
	{
		level.modeset = ENDGAMEVOTE;
		ent = NULL;
		goto done;
	}

	if ((nextmap = MapCycleNext(level.mapname)) != 0)
	{
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = nextmap;
//		safe_bprintf(PRINT_HIGH, "Next map will be: %s\n", ent->map);

		goto done;
	}

	if (level.nextmap[0])
	{	// go to a specific map
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.nextmap;
	}
	else
	{	// search for a changeleve
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			ent = G_Spawn ();
			ent->classname = "target_changelevel";
			ent->map = level.mapname;
		}
	}

done:
	BeginIntermission (ent);
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int			i;
	gclient_t	*cl;

	if (level.intermissiontime)
		return;

// ACEBOT_ADD
	if (!level.bots_spawned)
	{
		int		count = 0;
		edict_t	*doot;

		//hypov8 allows to add 1 bot and rest will join. with no players
		for_each_player_inc_bot(doot, i)
		count++;

		if (count)		
		{
			ACEND_InitNodes();
			ACEND_LoadNodes();
			ACESP_LoadBots();
			level.bots_spawned = true;
		}
	}
// ACEBOT_END

	if (level.framenum - level.lastactive == 600)
	{
		// the server has been idle for a minute, reset to default settings if needed
		if (ResetServer(true)) 
			return;

		if (wait_for_players && !(int)timelimit->value)
		{
			level.lastactive = -1;
			gi.dprintf("Waiting for players\n");
			UpdateTime();
			if (kpded2) // enable kpded2's idle mode for reduced CPU usage while waiting for players (automatically disabled when players join)
				gi.cvar_forceset("g_idle", "1");
		}
	}

	if ((int)teamplay->value == 1)
	{
		if ((int)cashlimit->value)
		{
			if ((team_cash[1] >= (int)cashlimit->value) || (team_cash[2] >= (int)cashlimit->value))
			{
				safe_bprintf(PRINT_HIGH, "Cashlimit hit.\n");
				EndDMLevel ();
				return;
			}
		}
	}
	else if ((int)fraglimit->value)
	{
		if (teamplay->value)
		{
			if (team_cash[1] >= (int)fraglimit->value || team_cash[2] >= (int)fraglimit->value)
			{
				safe_bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel ();
				return;
			}
		}
		else
		{
			for (i=0 ; i< (int)maxclients->value ; i++)
			{
				cl = game.clients + i;
				if (!g_edicts[i+1].inuse)
					continue;

				if (cl->resp.score >= (int)fraglimit->value)
				{
					safe_bprintf(PRINT_HIGH, "Fraglimit hit.\n");
					EndDMLevel ();
					return;
				}
			}
		}
	}

	if ((int)timelimit->value)
	{
		if (level.framenum >= (level.startframe + ((int)timelimit->value) * 600))
		{
			safe_bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
		}
	}
}

/*
=============
ExitLevel
=============
*/

void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];
	int		count = 0;

	for_each_player_not_bot(ent, i) {
		count++;
	}

	if (!count && ResetServer(true))
		return; // server reset instead

	if (!level.changemap)
	{
		safe_bprintf(PRINT_HIGH, "Map change failed, restarting current map\n");
		level.changemap = level.mapname;
	}


// ACEBOT_ADD
	ACECM_LevelEnd();
	PrintScoreMatchEnd();
	ACESP_RemoveBot("all", false);

	if (teamplay->latched_string || dm_realmode->latched_string|| sv_hitmen->latched_string)
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", level.mapname);
	else
// ACEBOT_END
		Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
extern int bbox_cnt;
extern edict_t	*mdx_bbox[];

void AI_ProcessCombat (void);

void G_RunFrame (void)
{
	int		i;
	edict_t	*ent;

	if (level.framenum == 50 && wait_for_players && !level.num_vote_set && !level.lastactive)
	{
		level.lastactive = -1;
		gi.dprintf("Waiting for players\n");
		UpdateTime();
		if (kpded2) // enable kpded2's idle mode for reduced CPU usage while waiting for players (automatically disabled when players join)
			gi.cvar_forceset("g_idle", "1");
	}

	// skip frame processing if the server is waiting for players
	if (level.lastactive < 0)
		return;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	level.frameStartTime = Sys_Milliseconds();

	// exit intermissions

	if (level.exitintermission)	
	{
		ExitLevel ();
		return;
	}


// ACEBOT_ADD
	//run each bot and calc player movement
	ACEAI_G_RunFrame();
// ACEBOT_END


	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);


		if ((ent->svflags & SVF_MONSTER) || ent->client)
		{
			if (ent->waterlevel > 1)
			{
				ent->onfiretime = 0;
			}

			// On fire
			if (ent->onfiretime < 0)
			{
				ent->onfiretime++;
			}
			else if (ent->onfiretime > 0)
			{
				vec3_t	point, org, dir;
				int		i,j;
				float	dist;

				// Deathmatch flames done on client-side
				if ((!deathmatch_value /*|| ent->onfiretime == 1*/) && (deathmatch_value || !ent->client))
				{
					VectorSubtract( g_edicts[1].s.origin, ent->s.origin, dir );
					dist = VectorNormalize( dir );

					// Ridah, spawn flames at each body part
					MDX_HitCheck( ent, world, world, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0, 0, vec3_origin );

					for (i = 0; i < bbox_cnt; i++)
					{

						// don't draw so many if the client is up close
						if (dist < 256)
						{
							if (random() > dist/256)
								continue;
						}

						VectorAdd( mdx_bbox[i]->s.origin, dir, org );

						if (!deathmatch_value)
						{
							for (j=0; j<2; j++)
							{
								point[2] = (org[2] + ((rand()%18) - 6) + 6);
								point[1] = (org[1] + ((rand()%10) - 5));
								point[0] = (org[0] + ((rand()%10) - 5));

								gi.WriteByte (svc_temp_entity);
								gi.WriteByte (TE_SFXFIREGO);
								gi.WritePosition (point);

								if (ent->onfiretime == 1)
									gi.WriteByte (1.2 * 10.0);
								else
									gi.WriteByte (0.6 * 10.0);

								gi.multicast (point, MULTICAST_PVS);		
							}
						}

						// just do one smoke cloud
						if ((ent->onfiretime == 1) && (rand()%2))
						{
							point[2] = (org[2] + 20);// + ((rand()&31) - 16) + 20);
							point[1] = (org[1]);// + ((rand()%14) - 7));
							point[0] = (org[0]);// + ((rand()%14) - 7));

							gi.WriteByte (svc_temp_entity);
							gi.WriteByte (TE_SFXSMOKE);
							gi.WritePosition (point);
							// gi.WriteDir (ent->s.angles);
							gi.WriteByte (16 + (rand()%24));
							gi.WriteByte (0);
							gi.multicast (point, MULTICAST_PVS);
						}
					}
				}

			    if (!ent->deadflag)
				{
					edict_t *trav=NULL;
					float	damage=1;

					T_Damage( ent, ent->onfireent, ent->onfireent, vec3_origin, ent->s.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER );

					// make sure they are in the "catch_fire" motion
					if (!deathmatch_value && (ent->health > 0) && ent->cast_info.catch_fire)
					{
						ent->cast_info.catch_fire( ent, ent->onfireent );
					}

				}

				ent->onfiretime--;	

				if (ent->onfiretime <= 0)
				{
					ent->onfireent = NULL;
					ent->onfiretime = 0;
				}

				// JOSEPH 3-JUN-99
				if (ent->health > 0 && ent->onfiretime == 0)
				{
					ent->s.model_parts[PART_GUN].invisible_objects = 0;
					ent->s.model_parts[PART_GUN2].invisible_objects = 0;
				}
				else
				{
					ent->s.model_parts[PART_GUN].invisible_objects = (1<<0 | 1<<1);
					ent->s.model_parts[PART_GUN2].invisible_objects = (1<<0 | 1<<1);
				}
				// END JOSEPH

				if (ent->health > 0)
				{
					// use voice tables for this?

					// gi.dprintf( "SOUND TODO: ARRRGGGHH!!! (on fire)\n" );
				}
				else	// dead
				{
					if (ent->onfiretime > 20)
						ent->onfiretime = 20;

					if (ent->onfiretime == 1)
					{
						gi.WriteByte (svc_temp_entity);
						gi.WriteByte (TE_BURN_TO_A_CRISP);
						gi.WritePosition (ent->s.origin);
						gi.multicast (ent->s.origin, MULTICAST_PVS);
					}

				}
			}
		}

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= (int)maxclients->value)
		{
			ClientBeginServerFrame (ent);
		}
		else //--> end player
		{
			G_RunEntity (ent);

			// Ridah, fast walking speed
			if (	(ent->cast_info.aiflags & AI_FASTWALK)
				&&	(ent->svflags & SVF_MONSTER)
				&&	(ent->cast_info.currentmove)
//				&&	(ent->cast_info.currentmove->frame->aifunc == ai_run)
				&&	(ent->cast_info.currentmove->frame->dist < 20)
				&&	(!ent->enemy))
			{
				G_RunEntity (ent);
				// JOSEPH 12-MAR-99
				if (ent->think) ent->think(ent);
				// END JOSEPH
			}
		}

		if (((ent->s.renderfx2 & RF2_DIR_LIGHTS) || (ent->client) || deathmatch_value))
		{
			if (!level.num_light_sources)	// no lights to source from, so default back to no dir lighting
			{
				ent->s.renderfx2 &= ~RF2_DIR_LIGHTS;
			}
			else
			{

				if (ent->client)
					ent->s.renderfx2 |= RF2_DIR_LIGHTS;

				if (!VectorCompare(ent->s.last_lighting_update_pos, ent->s.origin)
					&& VectorDistance( ent->s.origin, ent->s.last_lighting_update_pos ) > 64)
				{
					UpdateDirLights( ent );

					VectorCopy( ent->s.origin, ent->s.last_lighting_update_pos );
				}
				else if (showlights->value && ent->client)
				{
					UpdateDirLights( ent );
				}

				if (!ent->s.model_lighting.num_dir_lights)
					ent->s.renderfx2 &= ~RF2_DIR_LIGHTS;
			}
		}

	}

// ACEBOT_ADD //debug with showing local nodes. use "localnode"
	ACEND_DebugNodesLocal();
// ACEBOT_END

	if (level.framenum > 0x100000)
	{
		// the map started a long time ago, restart before counters overflow
		int		i, count = 0;
		edict_t *player;

		for_each_player_not_bot(player, i)
			count++;


		if (!count)
		{
			char	command[64];
// ACEBOT_ADD
			ACECM_LevelEnd();
			PrintScoreMatchEnd();
			ACESP_RemoveBot("all", false);

			if (teamplay->latched_string || dm_realmode->latched_string|| sv_hitmen->latched_string)
				Com_sprintf (command, sizeof(command), "map \"%s\"\n", level.mapname);
			else
// ACEBOT_END
				Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.mapname);
			gi.AddCommandString (command);
			return;
		}
	}

// Papa 10.6.99
// these is where the server checks the state of the current mode
// all the called functions are found in tourney.c

	if (level.modeset == PREGAME)
		CheckStartPub ();

	if (level.modeset == MATCHSETUP)
		CheckIdleMatchSetup ();
	/* server cmd "matchstart" ..hypo now used in teampay */
	if (level.modeset == MATCHCOUNT)
		CheckStartMatch ();

	if ((level.modeset == MATCHSPAWN) || (level.modeset == PUBLICSPAWN))
		CheckAllPlayersSpawned ();

	if (level.modeset == MATCH)
		CheckEndMatch ();

	if (level.modeset == PUBLIC) 
		CheckDMRules ();

	if (level.modeset == ENDGAMEVOTE) 
		CheckEndVoteTime ();

	if (level.modeset == ENDGAME) 
		CheckEndTime ();

	if (level.voteset != NO_VOTES)
		CheckVote();

	// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
		hm_CheckWeaponTimer();
	// END
	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

	if (idle_client->value < 60)
		gi.cvar_set("idle_client", "60");

	if (!(level.framenum % 10))
		UpdateTime();
}
