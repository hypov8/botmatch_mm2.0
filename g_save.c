
#include "g_local.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// BEGIN HITMEN
#include "g_hitmen.h"
// END
field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
	{"move_angles", FOFS(move_angles), F_VECTOR},
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},

	{"light", FOFS(light_level), F_INT},	// Ridah, used by model lighting code
	{"_color", FOFS(rotate), F_VECTOR},		// Ridah, used by model lighting code
	{"radius", FOFS(dmg_radius), F_VECTOR},		// Ridah, used by model lighting code

	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},

	{"objectbounds_filename1", FOFS(s.model_parts[0].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename2", FOFS(s.model_parts[1].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename3", FOFS(s.model_parts[2].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename4", FOFS(s.model_parts[3].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename5", FOFS(s.model_parts[4].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename6", FOFS(s.model_parts[5].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename7", FOFS(s.model_parts[6].objectbounds_filename), F_LSTRING},
	{"objectbounds_filename8", FOFS(s.model_parts[7].objectbounds_filename), F_LSTRING},

// JOSEPH 19-MAR-99
	{"rotate", FOFS(rotate), F_VECTOR},
	{"duration", FOFS(duration), F_FLOAT},
	{"alphalevel", FOFS(alphalevel), F_INT},
	{"fxdensity", FOFS(fxdensity), F_INT},
	{"healspeed", FOFS(healspeed), F_INT}, 
	{"deadticks", FOFS(deadticks), F_INT}, 
	{"missteam", FOFS(missteam), F_INT}, 
	{"misstime", FOFS(misstime), F_INT}, 
	{"cameraangle", FOFS(cameraangle), F_VECTOR}, 
	{"cameraorigin", FOFS(cameraorigin), F_VECTOR}, 
	{"cameravel", FOFS(cameravel), F_VECTOR},
	{"cameravelrel", FOFS(cameravelrel), F_VECTOR},
	{"debugprint", FOFS(debugprint), F_INT},
	{"target2", FOFS(target2), F_LSTRING},
	{"localteam", FOFS(localteam), F_LSTRING},
	{"reactdelay", FOFS(reactdelay), F_FLOAT}, 
	{"currentcash", FOFS(currentcash), F_INT},
	{"type", FOFS(type), F_LSTRING},
	{"head", FOFS(head), F_INT},
	{"key", FOFS(key), F_INT},
	{"target2_ent", FOFS(target2_ent), F_EDICT},
	{"missent", FOFS(missent), F_EDICT},
	{"handle", FOFS(handle), F_EDICT},
	{"handle2", FOFS(handle2), F_EDICT},
	{"save_self", FOFS(save_self), F_EDICT},
	{"save_other", FOFS(save_other), F_EDICT},
	{"deadticks", FOFS(deadticks), F_INT},
	{"thudsnd", FOFS(thudsnd), F_INT},
	{"head", FOFS(head), F_INT},
	{"firetype", FOFS(firetype), F_INT},
	{"thudsurf", FOFS(thudsurf), F_INT},
	{"lightit", FOFS(lightit), F_INT},
	{"option", FOFS(option), F_INT},
	{"noshadow", FOFS(noshadow), F_INT},
// END JOSEPH

	{"acc", FOFS (acc), F_INT},
	{"cal", FOFS (cal), F_INT},

	// Ridah, new stuff

	{"cast_group", FOFS(cast_group), F_INT},
	{"skin", FOFS(skin), F_INT},
	{"moral", FOFS(moral), F_INT},
	{"guard_radius", FOFS(guard_radius), F_INT},
	{"guard_target", FOFS(guard_target), F_LSTRING},
	{"name", FOFS(name), F_LSTRING},
	{"episode", FOFS(count), F_INT},				// used by worldspawn
	{"scriptname", FOFS(scriptname), F_LSTRING},

	{"onfireent", FOFS(onfireent), F_EDICT},
	{"leader", FOFS(leader), F_EDICT},
	{"leader_target", FOFS(leader_target), F_LSTRING},
	{"last_goal", FOFS(last_goal), F_EDICT},

	{"order", FOFS(order), F_INT},
	{"order_timestamp", FOFS(order_timestamp), F_FLOAT},
	{"moveout_ent", FOFS(moveout_ent), F_EDICT},
	{"character_index", FOFS(character_index), F_INT},
	{"last_talk_time", FOFS(last_talk_time), F_FLOAT},
	{"profanity_level", FOFS(profanity_level), F_INT},
	{"guard_ent", FOFS(guard_ent), F_EDICT},
	{"sight_target", FOFS(sight_target), F_LSTRING},
	{"goal_ent", FOFS(goal_ent), F_EDICT},
	{"combat_goalent", FOFS(combat_goalent), F_EDICT},
	{"cover_ent", FOFS(cover_ent), F_EDICT},
	
	{"episode_flags", FOFS(episode_flags), F_INT},

	{"name_index", FOFS(name_index), F_INT},
	{"last_territory_touched", FOFS(last_territory_touched), F_EDICT},
	{"response_ent", FOFS(response_ent), F_EDICT},
	{"last_response_time", FOFS(last_response_time), F_FLOAT},
	{"last_response", FOFS(last_response), F_INT},

	{"start_ent", FOFS(start_ent), F_EDICT},
	{"holdpos_ent", FOFS(holdpos_ent), F_EDICT},

	{"next_combattarget", FOFS(next_combattarget), F_LSTRING},

	{"activate_flags", FOFS(activate_flags), F_INT},
	{"biketime", FOFS(biketime), F_FLOAT},
	{"bikestate", FOFS(bikestate), F_INT},

	{"vehicle_index", FOFS(vehicle_index), F_INT},

	{"art_skins", FOFS(art_skins), F_LSTRING},

	{"aiflags", FOFS(cast_info.aiflags), F_INT},

	{"gun_noise_delay", FOFS(gun_noise_delay), F_FLOAT},

	{"scale", FOFS(cast_info.scale), F_FLOAT},

	{"voice_pitch", FOFS(voice_pitch), F_FLOAT},

	{"health_threshold", FOFS(health_threshold), F_INT},
	{"health_target", FOFS(health_target), F_LSTRING},
	{"health_threshold2", FOFS(health_threshold2), F_INT},
	{"health_target2", FOFS(health_target2), F_LSTRING},
	{"health_threshold3", FOFS(health_threshold3), F_INT},
	{"health_target3", FOFS(health_target3), F_LSTRING},
	// Ridah, done.

	
	// temp spawn vars -- only valid when the spawn function is called
	{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
	{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
	{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
	{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
	{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
	{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},

//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item", FOFS(item), F_ITEM},

	{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
	{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
	{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},
	{"fogdensity", STOFS(fogdensity), F_FLOAT, FFL_SPAWNTEMP}, 
	{"fogval", STOFS(fogval), F_VECTOR, FFL_SPAWNTEMP},

	{"fogdensity2", STOFS(fogdensity2), F_FLOAT, FFL_SPAWNTEMP}, 
	{"fogval2", STOFS(fogval2), F_VECTOR, FFL_SPAWNTEMP},


	{0, 0, 0, 0}

};


char cmd_check[8];

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/

#ifdef _DEBUG
#include <crtdbg.h>
#endif

void InitGame (void)
{
	int i;

#ifdef _DEBUG
   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
//   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
		|_CRTDBG_CHECK_ALWAYS_DF
//		|_CRTDBG_CHECK_CRT_DF
		|_CRTDBG_LEAK_CHECK_DF);
	gi.dprintf ("!!!! DEBUGGING !!!! \n");
#endif

	gi.dprintf ("==== InitGame ====\n");

	// BEGIN HITMEN
	//sl_Logging( &gi, GAMEVERSION );	// StdLog 
	// END
	srand( (unsigned)time( NULL ) );

	gun_x = gi.cvar ("gun_x", "0", 0);
	gun_y = gi.cvar ("gun_y", "0", 0);
	gun_z = gi.cvar ("gun_z", "0", 0);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar ("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar ("sv_rollangle", "0", 0);
	sv_maxvelocity = gi.cvar ("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar ("sv_gravity", "800", 0);

// ACEBOT_ADD
	//sv_botcfg			= gi.cvar("sv_botcfg", "1", CVAR_NOSET);
	sv_botskill			= gi.cvar("sv_botskill", "2", CVAR_SERVERINFO);
	sv_botpath			= gi.cvar("sv_botpath", "1", 0);
	sv_botjump			= gi.cvar("sv_botjump", "0", 0); //todo remove this

	sv_bot_allow_add	= gi.cvar("sv_bot_allow_add", "0", 0); //stops players voting 
	sv_bot_allow_skill	= gi.cvar("sv_bot_allow_skill", "0", 0); //stops players voting 

	sv_bot_max			= gi.cvar("sv_bot_max", "8", 0);
	sv_bot_max_players	= gi.cvar("sv_bot_max_players", "0", 0);
	sv_hitmen			= gi.cvar("hitmen", "0", CVAR_SERVERINFO|CVAR_LATCH);
	sv_hook				= gi.cvar("sv_hook", "0", 0); //HmHookAvailable //enable hook to work out of hitman
// ACEBOT_END

	// add hypov8
	sv_keeppistol = gi.cvar("sv_keeppistol", "1", 0);

	// noset vars
	dedicated = gi.cvar ("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar ("cheats", "0", CVAR_LATCH);
	gi.cvar ("gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar ("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);

	no_spec = gi.cvar ("no_spec", "0", 0);
	no_shadows = gi.cvar ("no_shadows", "0", 0);
	no_zoom = gi.cvar ("no_zoom", "0", 0);

	maxclients = gi.cvar ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);

	// JOSEPH 16-OCT-98
	maxentities = gi.cvar ("maxentities", /*"1024"*/"2048", CVAR_LATCH);


	//////////////////////
	// change anytime vars
	dmflags = gi.cvar ("dmflags", "0", CVAR_SERVERINFO|CVAR_ARCHIVE);


	fraglimit = gi.cvar ("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar ("timelimit", "0", CVAR_SERVERINFO);
	password = gi.cvar ("password", "", CVAR_USERINFO);
	filterban = gi.cvar ("filterban", "1", 0);

	antilag = gi.cvar("antilag", "1", CVAR_SERVERINFO);
	props = gi.cvar("props", "0", 0);
	shadows = gi.cvar("shadows", "1", 0);

// BEGIN HITMEN
// BEGIN HOOK
	hook_is_homing     = gi.cvar ("hook_is_homing", "0", 0);
	hook_homing_radius = gi.cvar ("hook_homing_radius", "200", 0);
	hook_homing_factor = gi.cvar ("hook_homing_factor", "5", 0);
	hook_players       = gi.cvar ("hook_players", "0", 0);
	hook_sky           = gi.cvar ("hook_sky", "0", 0);
	hook_min_length    = gi.cvar ("hook_min_length", "20", 0);
	hook_max_length    = gi.cvar ("hook_max_length", "2000", 0);
	hook_pull_speed    = gi.cvar ("hook_pull_speed", "40", 0);
	hook_fire_speed    = gi.cvar ("hook_fire_speed", "1000", 0);
	hook_messages      = gi.cvar ("hook_messages", "0", 0);
	hook_vampirism     = gi.cvar ("hook_vampirism", "0", 0);
	hook_vampire_ratio = gi.cvar ("hook_vampire_ratio", "0.5", 0);
	hook_hold_time     = gi.cvar ("hook_hold_time", "20", 0);

	if (hook_hold_time->value < 5)
		hook_hold_time->value = 15;

	if (hook_hold_time->value > 60)
		hook_hold_time->value = 30;
// END
	bonus = gi.cvar("bonus", "0", 0);

	g_select_empty = gi.cvar ("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar ("run_pitch", "0.002", 0);
	run_roll = gi.cvar ("run_roll", "0.005", 0);
	bob_up  = gi.cvar ("bob_up", "0.005", 0);
	bob_pitch = gi.cvar ("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar ("bob_roll", "0.002", 0);

	// flood control
	flood_msgs = gi.cvar ("flood_msgs", "4", 0);
	flood_persecond = gi.cvar ("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar ("flood_waitdelay", "10", 0);

	kick_flamehack = gi.cvar ("kick_flamehack", "1", CVAR_SERVERINFO);
	anti_spawncamp = gi.cvar ("anti_spawncamp", "1", 0);
	idle_client = gi.cvar("idle_client", "240", 0);

// Ridah, new cvar's
	developer = gi.cvar ("developer", "0", 0);
	g_vehicle_test = gi.cvar ("g_vehicle_test", "0", CVAR_LATCH);	// Enables Hovercars for all players
	dm_locational_damage = gi.cvar ("dm_locational_damage", "0", 0);
	showlights =  gi.cvar ("showlights", "0", 0);
	timescale = gi.cvar("timescale", "1.0", 0);

	// speed hack fix
	gi.cvar_set("sv_enforcetime", "1");
	teamplay = gi.cvar("teamplay", "0", CVAR_LATCH|CVAR_SERVERINFO);
	if (teamplay->value != 0 && teamplay->value != 1 && teamplay->value != 4)
		gi.cvar_set("teamplay", "1");
	cashlimit = gi.cvar ("cashlimit", "0", teamplay->value == 1 ? CVAR_SERVERINFO : 0);
	g_cashspawndelay = gi.cvar("g_cashspawndelay", "5", CVAR_ARCHIVE|CVAR_LATCH);
	dm_realmode = gi.cvar( "dm_realmode", "0", CVAR_LATCH|CVAR_SERVERINFO);
	g_mapcycle_file = gi.cvar( "g_mapcycle_file", "", 0);
// Ridah, done.

	// snap - team tags
	gi.cvar(TEAMNAME, "", CVAR_SERVERINFO);
	gi.cvar_set(TEAMNAME, "");

	gi.cvar(SCORENAME, "", CVAR_SERVERINFO);
	gi.cvar_set(SCORENAME, "");
	// the "rconx serverinfo" command needs this to be the final serverinfo cvar
	gi.cvar(TIMENAME, "", CVAR_SERVERINFO);
	gi.cvar_set(TIMENAME, "");

	// items
	InitItems ();

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = (int)maxclients->value;
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients+1;

	// disable single player and co-op modes
	gi.cvar_set("deathmatch", "1");
	gi.cvar_set("coop", "0");

	// load config
	proccess_ini_file();

	cmd_check[0] = '\177';
	for (i=1; i<3; i++)
		cmd_check[i] = 'A'+(rand()%26);
	cmd_check[i] = 0;

// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
			hm_Initialise();
// END


	if (kpded2)
	{
		/*
			enable kpded2 features:
			GMF_CLIENTPOV - improved eyecam chase mode
			GMF_CLIENTTEAM - team info in server browsers
			GMF_CLIENTNOENTS - removes everything when spectating is disabled
			GMF_WANT_ALL_DISCONNECTS - cancelled connection notifications
			GMF_WANT_COUNTRY - receive country info in ClientConnect
		*/
		char buf[10];
		sprintf(buf, "%d", GMF_CLIENTPOV | GMF_CLIENTNOENTS | GMF_WANT_ALL_DISCONNECTS |
			 GMF_WANT_COUNTRY //GeoIP2
			| (teamplay->value ? GMF_CLIENTTEAM : 0));
		gi.cvar_forceset("g_features", buf);
	}
}

//=========================================================

void WriteGame (char *filename, qboolean autosave)
{
	// not needed in deathmatch
}

void ReadGame (char *filename)
{
	// not needed in deathmatch
}

void WriteLevel (char *filename)
{
	// not needed in deathmatch
}

void ReadLevel (char *filename)
{
	// not needed in deathmatch
}
