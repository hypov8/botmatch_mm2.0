
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
//cvar_t *sv_botcfg;
cvar_t *sv_botskill;
cvar_t *sv_botpath;
cvar_t *sv_botjump;
cvar_t *sv_bot_allow_add;
cvar_t *sv_bot_allow_skill;
cvar_t *sv_bot_max;
cvar_t *sv_bot_max_players;
cvar_t *sv_hitmen;
cvar_t *sv_hook; //add hypov8
cvar_t *sv_pretime;
cvar_t *sv_pretimebm;
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

cvar_t	*bonus;

qboolean	kpded2;
int		starttime;


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

#if 0
// HYPOV8_ADD
static char *weapnames[8] = { "Pipe", "Pist", "Shot", "Tommy", " HMG", "  GL", "  RL", "Flame" }; //hypov8 duplicate p_hud.c

//hypov8 dm match end results printed
void PrintScoreMatchEnd(void)
{
	int i, j;


	gi.dprintf(" \n");
	gi.dprintf("--== results ==--\n");
	gi.dprintf("map              : %s\n", level.mapname);
	gi.dprintf("num score ping deaths acc fav   name           address               ver \n");
	gi.dprintf("--- ----- ---- ------ ---- ----- -------------- --------------------- ----\n");
	for (i = 1; i <= maxclients->value; i++)
	{
		if (g_edicts[i].inuse && g_edicts[i].client)
		{
			int fc = 0;
			char *favWep = "   -";
			gclient_t *c = g_edicts[i].client;

			for (j = 0; j<8; j++) 		{ 
				if (c->resp.fav[j]>fc) 		{ 
					fc = c->resp.fav[j];
					favWep = weapnames[j];
				} 
			}
			/*
			gi.dprintf("%3d", i - 1);				//client num
			gi.dprintf(" %5d", c->resp.score);		//score
			gi.dprintf(" %4d", c->ping);			//ping
			gi.dprintf(" %6d", c->resp.deposited);	//deaths DM
			gi.dprintf(" %4i", c->resp.accshot ? c->resp.acchit * 1000 / c->resp.accshot : 0); //acc
			gi.dprintf(" %5s", favWep);								//fav weapon
			gi.dprintf(" %-14s", c->pers.netname);					//player name
			gi.dprintf(" %-23s", c->pers.ip);						//ip
			gi.dprintf(" %.2f\n", (float)c->pers.version / 100.0);	//version
			*/

			gi.dprintf("%3d %5d %4d %6d %4i %5s %-14s %-23s %.2f\n"
				, i - 1									//client num
				, c->resp.score							//score
				, c->ping								//ping
				, c->resp.deposited						//deaths DM
				, c->resp.accshot ? c->resp.acchit * 1000 / c->resp.accshot : 0 //acc
				, favWep								//fav weapon
				, c->pers.netname						//player name
				, c->pers.ip							//ip
				, (float)c->pers.version / 100.0);		//version
		}
	}
	gi.dprintf("--==   end   ==--\n");
	gi.dprintf(" \n");
}

#endif
// HYPOV8_END

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
	for (i=0 ; i<maxclients->value ; i++)
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

	gi.error (ERR_FATAL, "%s", text);
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
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
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

	if (!num_maps) return NULL;

	for (i=0; i<num_maps; i++)
	{
		if (!Q_stricmp(maplist[i], level.mapname))
		{
			if (++i == num_maps) i = 0;
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
		level.startframe = level.framenum + 10; //add hypov8

		goto done;
	}

	level.startframe = level.framenum;
	if (allow_map_voting)
	{
		level.modeset = ENDGAMEVOTE;
		ent = NULL;
		goto done;
	}

	if ((nextmap = MapCycleNext(level.mapname)) != 0) // HYPOV8_ADD !=0
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
	//for_each_player_inc_bot(player, i) //hypov8 todo: hide wep // ACEBOT_ADD
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
// ACEBOT_ADD
	int		count = 0;
	edict_t	*doot;
// ACEBOT_END

	if (level.intermissiontime)
		return;
// ACEBOT_ADD
	for_each_player_inc_bot(doot, i) //hypov8 allows to add 1 bot and rest will join. with no players
	{
		count++;
		/*if (!doot->acebot.is_bot)
		{
			if (doot->velocity[2] <0.0)
					gi.dprintf ("%5.3f\n", doot->velocity[2]);

		}*/ //debug

	}
	if (count && !level.bots_spawned)
	{
		ACEND_InitNodes();
		ACEND_LoadNodes();
		ACESP_LoadBots();
		level.bots_spawned = true;
	}
// ACEBOT_END
	if (level.framenum - level.lastactive == 600)
	{
		// the server has been idle for a minute, reset to default settings if needed
		if (ResetServer(true)) return;
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
			for (i=0 ; i<maxclients->value ; i++)
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
	{
		return; // server reset instead
	}
// ACEBOT_ADD
	ACECM_LevelEnd();
	PrintScoreMatchEnd();
	//FreeBots();
// ACEBOT_END

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();

// RAFAEL
	level.speaktime = 0;

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;

	}

// BEGIN:	Xatrix/Ridah/19-apr-1998
	// make sure Nav data isn't carried over to next level
	NAV_PurgeActiveNodes(level.node_data);
	level.node_data = NULL;
// BEGIN:	Xatrix/Ridah/19-apr-1998

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

	if (level.framenum == 50 && wait_for_players && !allow_map_voting && !level.lastactive)
	{
		level.lastactive = -1;
		gi.dprintf("Waiting for players\n");
		UpdateTime();
		if (kpded2) // enable kpded2's idle mode for reduced CPU usage while waiting for players (automatically disabled when players join)
			gi.cvar_forceset("g_idle", "1");
	}


// ACEBOT_ADD
	if (level.framenum == 1)
		ACESP_RemoveBot("all");
	if (sv_botskill->value < 0.0f) 
		gi.cvar_set("sv_botskill", "0");
	else if (sv_botskill->value > 4.0f)
		gi.cvar_set("sv_botskill", "4.0");
// ACEBOT_END

	// skip frame processing if the server is waiting for players
	if (level.lastactive < 0)
		goto uptime;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	level.frameStartTime = Sys_Milliseconds();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

// ACEBOT_ADD
		//if ((ent->is_bot) && !((level.modeset == TEAM_MATCH_RUNNING) || (level.modeset == DM_MATCH_RUNNING)))
		if ((ent->client) && !(level.modeset == MATCH || level.modeset == PUBLIC))
				continue; //hypov8 todo: check this
// ACEBOT_END
		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);


		if (ent->svflags & SVF_MONSTER || ent->client)
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

		if (i > 0 && i <= maxclients->value)
		{
// ACEBOT_ADD
			if (ent->acebot.is_bot)
			{		//dont run bots when not ingame
				if (level.modeset == MATCH || level.modeset == PUBLIC)
				G_RunEntity(ent);
			}
// ACEBOT_END
			ClientBeginServerFrame (ent);
		}
		else //--> end player
		{
// NET_ANTILAG	//et-xreal antilag
			//start trace for non player objects
			/*
			qboolean antilagtotrace = 0;
			edict_t	*owner = NULL;

			if (ent->antilagToTrace && !sv_antilag_noexp->value) //item will cause damage so back trace
			{
				antilagtotrace = 1; //incase item is removed from world
				owner = ent->owner;
				G_HistoricalTraceBegin(ent, owner);
			}*/
// END_LAG

			G_RunEntity (ent);

// NET_ANTILAG	//et-xreal antilag
			//if (antilagtotrace)
			//	G_HistoricalTraceEnd(ent, owner);
// END_LAG

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

				// if single player, only calculate if it's visible to our player
				if (	(!VectorCompare(ent->s.last_lighting_update_pos, ent->s.origin))
					 &&	(	(ent->client && !deathmatch_value)
						||	(	(VectorDistance( ent->s.origin, ent->s.last_lighting_update_pos ) > (deathmatch_value ? 128 : 64))
							 &&	(	(deathmatch_value)
								 ||	(	(gi.inPVS( g_edicts[1].s.origin, ent->s.origin))
									 &&	(infront( &g_edicts[1], ent ) ))))))
				{
					UpdateDirLights( ent );

					VectorCopy( ent->s.origin, ent->s.last_lighting_update_pos );
				}
				else if (showlights->value && gi.inPVS( g_edicts[1].s.origin, ent->s.origin))
				{
					UpdateDirLights( ent );
				}

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

		for_each_player_not_bot(player, i)	{
			count++;
		}

		if (!count)
		{
			char	command[64];

// ACEBOT_ADD
			ACECM_LevelEnd();
			PrintScoreMatchEnd();
			//FreeBots();

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
		CheckStartPub (); 		//CheckStartDM(); //hypov8 edited, game is allways dm mode?, make teamplay games use TEAM_PRE_MATCH


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
	{
		UpdateTime();
uptime:
		if (starttime)
		{
			char buf[32];
			int secs = (int)time(NULL) - starttime;
			int mins = secs / 60;
			int hours = mins / 60;
			int days = hours / 24;
			if (days)
				sprintf(buf, "%dd %dh %dm", days, hours % 24, mins % 60);
			else if (hours)
				sprintf(buf, "%dh %dm", hours, mins % 60);
			else if (mins)
				sprintf(buf, "%dm", mins);
			else
				sprintf(buf, "%ds", secs);
			gi.cvar_set("uptime", buf);
		}
	}
}
