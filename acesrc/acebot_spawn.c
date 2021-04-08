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
//  acebot_spawn.c - This file contains all of the 
//                   spawing support routines for the ACE bot.
//
///////////////////////////////////////////////////////////////////////

#include "../g_local.h" //DIR_SLASH
#include "../m_player.h" //DIR_SLASH
#include "acebot.h"

// BEGIN HITMEN
#include "../g_hitmen.h" //DIR_SLASH
// END


///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// load the bots from a file.
//
// Side effect/benifit are that the bots persist between games.
///////////////////////////////////////////////////////////////////////
void ACESP_LoadBots()
{

    FILE *pIn;
	char buffer[MAX_STRING_LENGTH];
	int i;
	cvar_t	*game_dir, *map_name;
	char filename[MAX_QPATH];
	static qboolean flip;
	qboolean valid;

	char *line, *token;
	bot_skin_t player; // name, skin, team, skill;

	//hypo mod folder for bots dir
	map_name = gi.cvar("mapname", "", 0);
	game_dir = gi.cvar("game", "", 0);

	//check for individual bot config

	Com_sprintf(filename, sizeof(filename), "%s/bots/%s.cfg", game_dir->string, map_name->string); // comp\bots\mapname.cfg
	if ((pIn = fopen(filename, "r")) == NULL)
		Com_sprintf(filename, sizeof(filename), "%s/bots/_default.cfg", game_dir->string); // comp\bots\mapname.cfg

	if (pIn == NULL && (pIn = fopen(filename, "r")) == NULL)
		return; // bail


	while (fgetline(pIn, buffer))
	{
		valid = false;
		line = buffer;

		for (i = 1; i <= 4; i++)
		{
			token = COM_Parse(&line);
			if (token[0] == '\0'){
				if (i != 2 && i != 3) //allow null skin and null team
					break;
			}

			switch (i)
			{	
			case 1: strcpy(player.name, token); break;
			case 2: strcpy(player.skin, token); break;
			case 3: strcpy(player.team, token); break;
			case 4: if (token[0] != '\0') 
						player.skill = (float)atof((const char *)token); 
						break;
			}

			if (!valid && i == 1)
			{
				valid = true; //valid with only name
				//flip = flip ? false : true;
				//team = flip ? "nikki": "dragon";
				strcpy(player.skin, ""); //male_thug/001 001 001
				strcpy(player.team, ""); //team
				player.skill = 1.0f; //default. if missing from old versions
			}
		}

		if (valid)
		{
			if (player.skill > 2) 			player.skill = 2;
			else if (player.skill < 0.0f) 	player.skill = 0.0f;

			if (teamplay->value) // name, skin, team 
				ACESP_SpawnBot(player.team, player.name, player.skin, NULL, player.skill); //sv addbot thugBot "male_thug/009 031 031" dragon
			else // name, skin			
				ACESP_SpawnBot("\0", player.name, player.skin, NULL, player.skill); //sv addbot thugBot "male_thug/009 031 031"
		}
	}

	fclose(pIn);
}

static  void ACESP_PutClientInServer(edict_t *bot, qboolean respawn, int team);

///////////////////////////////////////////////////////////////////////
// Called by PutClient in Server to actually release the bot into the game
// Keep from killin' each other when all spawned at once
///////////////////////////////////////////////////////////////////////
static void ACESP_HoldSpawn(edict_t *self)
{
	//ACESP_Respawn(self);

	if (!(level.modeset == MATCH || level.modeset == PUBLIC))
	{
		self->deadflag = 0;
		gi.dprintf("bot respawned after match\n");
		return; //hypov8 dont respawn, fixes last person dying loosing there mouse pitch
	}

	if (teamplay->value)
		ACESP_PutClientInServer(self, true, self->client->pers.team);
	else
		ACESP_PutClientInServer(self, true, 0);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time; // +5;//hypov8 add 5 secs to respawn


	safe_bprintf (PRINT_MEDIUM, "%s entered the game\n", self->client->pers.netname);
}



/*
===========
ACESP_ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
static void ACESP_ClientConnect(edict_t *ent, char *userinfo)
{
	char *bestWepName = '\0';

	//hypov8 add randomness to best wep when connected. each map
	ent->acebot.randomWeapon = rand() % 3;
	//hypo another random, try make hmg more domanant
	if (ent->acebot.randomWeapon != 0)
		ent->acebot.randomWeapon = rand() % 3;

	switch (ent->acebot.randomWeapon)
	{
	case 0: bestWepName = "Heavy machinegun"; break;
	case 1: bestWepName = "Bazooka"; break;
	case 2: bestWepName = "Tommygun"; break;
	}

	gi.dprintf("%s (bestwep: %s)\n", ent->client->pers.netname, bestWepName);

	if (password->string[0])
		Info_SetValueForKey(userinfo, "password", password->string);

	Info_SetValueForKey(userinfo, "ver", "121");
	Info_SetValueForKey(userinfo, "country", "Botville");
	//hypov8 todo ip?

	ClientConnect(ent, userinfo); //todo return test fail??

	ent->client->showscores = NO_SCOREBOARD;
	ent->acebot.is_bot = true; //not needed
}





///////////////////////////////////////////////////////////////////////
// Modified version of id's code
///////////////////////////////////////////////////////////////////////
#if 1 //HYPOBOTS
static  void ACESP_PutClientInServer(edict_t *bot, qboolean respawn, int team)
{
	//vec3_t	spawn_origin, spawn_angles;

	ClientBegin(bot);
	bot->classname = "bot"; // "bot"
	//bot->acebot.is_jumping = false;
	bot->acebot.isTrigPush = false;
	bot->acebot.enemyID = -1; //hypo add
	bot->acebot.num_weps = 2;  //hypo add. 2= pistol+pipe
	bot->acebot.lastDamageTimer = 0;  //hypo add
	bot->acebot.enemyAddFrame = 0;	//hypov8 add
	bot->acebot.tauntTime = level.framenum + (random() * 100);
	//bot->acebot.spawnedTime = level.framenum + 30; //hypo add 3 seconds to look for weps?
	bot->acebot.SRGoal_frameNum = 0;
	bot->client->pers.team = team;
	bot->client->pers.noantilag = true;
	bot->client->ps.fov = 90;

	//hypo
	bot->client->resp.is_spawn = true;
	bot->inuse = true;
	bot->acebot.is_bot = true;
//end


	bot->s.angles[PITCH] = 0;
	bot->s.angles[YAW] = 0;// spawn_angles[YAW];
	bot->s.angles[ROLL] = 0;
	VectorCopy(bot->s.angles, bot->client->ps.viewangles);
	VectorCopy(bot->s.angles, bot->client->v_angle);

	bot->enemy = NULL;
	bot->movetarget = NULL;
	bot->acebot.state = BOTSTATE_MOVE;
	bot->acebot.suicide_timeout = level.time + 15.0;

	// Set the current node
	bot->acebot.node_ent = NULL;
	bot->acebot.next_move_time = level.time;
	bot->acebot.node_current = ACEND_FindClosestReachableNode(bot, BOTNODE_DENSITY, BOTNODE_ALL);
	bot->acebot.node_goal = bot->acebot.node_current;
	bot->acebot.node_next = bot->acebot.node_current;
	if (bot->acebot.node_current == INVALID) 
		bot->acebot.state = BOTSTATE_WANDER;

	if (!ACEAI_PickShortRangeGoalSpawned(bot))
		ACEAI_PickLongRangeGoal(bot);//todo: lrg weapon


#if HYPOBOTS
		bot->think = ACEAI_Think;
		bot->nextthink = level.time + FRAMETIME;
#endif

}

#else
static  void ACESP_PutClientInServer(edict_t *bot, qboolean respawn, int team)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 48};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;
	
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (bot, spawn_origin, spawn_angles);
	
	index = bot-g_edicts-1;
	client = bot->client;

	// deathmatch wipes most client data every spawn
	{
		char userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		bot->client->move_frame = bot->client->resp.name_change_frame = -80;  //just to be sure
		ClientUserinfoChanged (bot, userinfo);
	}

	bot->name_index = -1;

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;
	
	// copy some data from the client to the entity
	FetchClientEntData (bot);
	
	// clear entity values
	bot->groundentity = NULL;
	bot->client = &game.clients[index];
	bot->takedamage = DAMAGE_AIM;

	if (!respawn)
	{
		bot->movetype = MOVETYPE_NOCLIP;
		bot->solid = SOLID_NOT;
		bot->svflags |= SVF_NOCLIENT;
		bot->client->pers.weapon = NULL;
		bot->client->pers.spectator = SPECTATING; // add hypov8
	}
	else
	{
		bot->movetype = MOVETYPE_WALK;
		bot->solid = SOLID_BBOX; //hypov8 done spawn yet
		bot->svflags &= ~(SVF_DEADMONSTER | SVF_NOCLIENT);
		bot->client->pers.spectator = PLAYING; // add hypov8

		//give 3 seconds of imortality on each spawn (anti-camp) 
		client->invincible_framenum = level.framenum + 6; //hypov8 allow for antilag
		if (anti_spawncamp->value)
			client->invincible_framenum = level.framenum + 10;  //1 second (1.5 for players)
	}

	// RAFAEL
	bot->viewheight = 40;

	bot->inuse = true;
	bot->classname = "bot"; // "bot"
	bot->mass = 200;
	bot->deadflag = DEAD_NO;
	bot->air_finished = level.time + 12;
	bot->clipmask = MASK_PLAYERSOLID;
//	bot->model = "players/male/tris.md2";
	bot->pain = player_pain;
	bot->die = player_die;
	bot->waterlevel = 0;
	bot->watertype = 0;
	bot->flags &= ~FL_NO_KNOCKBACK;
	bot->s.renderfx2 = 0;
	bot->onfiretime = 0;
	bot->cast_info.aiflags |= AI_GOAL_RUN;	// make AI run towards us if in pursuit
	bot->flags &= ~FL_CHASECAM; //hypov8 turn off togglecam
	VectorCopy(mins, bot->mins);
	VectorCopy(maxs, bot->maxs);
	VectorClear(bot->velocity);
	bot->cast_info.standing_max_z = bot->maxs[2];
	bot->cast_info.scale = MODEL_SCALE;
	bot->s.scale = bot->cast_info.scale - 1.0;

	bot->hasSelectedPistol = false; // HYPOV8_ADD

	//acebot
	//bot->acebot.is_jumping = false;
	bot->acebot.isTrigPush = false;
	bot->acebot.enemyID = -1; //hypo add
	bot->acebot.num_weps = 2;  //hypo add. 2= pistol+pipe
	bot->acebot.lastDamageTimer = 0;  //hypo add
	bot->acebot.enemyAddFrame = 0;	//hypov8 add
	bot->acebot.tauntTime = level.framenum + (random() * 100);
	//bot->acebot.spawnedTime = level.framenum + 30; //hypo add 3 seconds to look for weps?
	client->pers.team = team;
	bot->client->pers.noantilag = true;
	//end

	if (bot->solid)
	{
		trace_t tr;
		tr = gi.trace(spawn_origin, bot->mins, bot->maxs, spawn_origin, NULL, CONTENTS_MONSTER);
		if (tr.startsolid)
		{
			// spawn point is occupied, try next to it
			vec3_t origin1;
			int c;
			VectorCopy(spawn_origin, origin1);
			for (c = 0;;)
			{
				for (i = 0; i<4; i++)
				{
					vec3_t start, end;
					float angle = (spawn_angles[YAW] + i * 90 - 45) / 360 * M_PI * 2;
					start[0] = spawn_origin[0] + cos(angle) * 50;
					start[1] = spawn_origin[1] + sin(angle) * 50;
					start[2] = spawn_origin[2];
					VectorCopy(start, end);
					end[2] -= 25;
					tr = gi.trace(start, bot->mins, bot->maxs, end, NULL, MASK_PLAYERSOLID);
					if (!tr.startsolid && tr.fraction < 1)
					{
						VectorCopy(start, spawn_origin);
						break;
					}
				}
				if (i < 4) break;
				if (++c == 2) break;
				// try another spawn point
				for (i = 0; i<3; i++)
				{
					SelectSpawnPoint(bot, spawn_origin, spawn_angles);
					if (!VectorCompare(spawn_origin, origin1))
						break;
				}
				if (i == 3) break;
			}
		}
	}

	
	
	client->ps.pmove.origin[0] = spawn_origin[0]*8; //hypov8 todo: check this for spec players
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;


	client->ps.fov = 90;


	// RAFAEL
	// weapon mdx
	{
		int i;
	
		memset(&(client->ps.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);

		client->ps.num_parts++;
	// JOSEPH 22-JAN-99
		if (client->pers.weapon)
			client->ps.model_parts[PART_HEAD].modelindex = gi.modelindex(client->pers.weapon->view_model);
		
		for (i=0; i<MAX_MODELPART_OBJECTS; i++)
			client->ps.model_parts[PART_HEAD].skinnum[i] = 0; // will we have more than one skin???
	}

	if (client->pers.weapon)
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	// END JOSEPH


	// clear entity state values
	bot->s.effects = 0;
	bot->s.skinnum = bot - g_edicts - 1;
	bot->s.modelindex = 255;		// will use the skin specified model
//	bot->s.modelindex2 = 255;		// custom gun model
	bot->s.frame = 0;
	VectorCopy (spawn_origin, bot->s.origin);
	bot->s.origin[2] += 1;	// make sure off ground
	VectorCopy (bot->s.origin, bot->s.old_origin);

	//add hypov8. calculate bots movement
	VectorCopy(spawn_origin, bot->acebot.oldOrigin);


	// bikestuff
	bot->biketime = 0;
	bot->bikestate = 0;


// Ridah, Hovercars
	if (g_vehicle_test->value)
	{
		if (g_vehicle_test->value == 3)
			bot->s.modelindex = gi.modelindex ("models/props/moto/moto.mdx");
		else
			bot->s.modelindex = gi.modelindex ("models/vehicles/cars/viper/tris_test.md2");

//		ent->s.modelindex2 = 0;
		bot->s.skinnum = 0;
		bot->s.frame = 0;

		if ((int)g_vehicle_test->value == 1)
			bot->flags |= FL_HOVERCAR_GROUND;
		else if ((int)g_vehicle_test->value == 2)
			bot->flags |= FL_HOVERCAR;
		else if ((int)g_vehicle_test->value == 3)
			bot->flags |= FL_BIKE;
		else if ((int)g_vehicle_test->value == 4)
			bot->flags |= FL_CAR;
	}
// done.
	else if (dm_locational_damage->value)	// deathmatch, note models must exist on server for client's to use them, but if the server has a model a client doesn't that client will see the default male model
	{
		char	*s;
		char	modeldir[MAX_QPATH];//, *skins;
		int		len;
		int		did_slash;
		char	modelname[MAX_QPATH];
//		int		skin;

		// NOTE: this is just here for collision detection, modelindex's aren't actually set

		bot->s.num_parts = 0;		// so the client's setup the model for viewing

		s = Info_ValueForKey (client->pers.userinfo, "skin");

//		skins = strstr( s, "/" ) + 1;

		// converts some characters to NULL's
		len = strlen( s );
		did_slash = 0;
		for (i=0; i<len; i++)
		{
			if (s[i] == '/')
			{
				s[i] = '\0';
				did_slash = true;
			}
			else if (s[i] == ' ' && did_slash)
			{
				s[i] = '\0';
			}
		}

		if (strlen(s) > MAX_QPATH-1)
			s[MAX_QPATH-1] = '\0';

		strcpy(modeldir, s);
		
		if (!modeldir[0])
			strcpy( modeldir, "male_thug" );
		
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		
		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/head.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/head.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/legs.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/legs.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/body.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/body.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		bot->s.model_parts[PART_GUN].modelindex = 255;
	}
	else	// make sure we can see their weapon
	{
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		bot->s.model_parts[PART_GUN].modelindex = 255;
		bot->s.num_parts = PART_GUN+1;	// make sure old clients recieve the view weapon index
	}

	// randomize spectator's direction in "no spec" mode
	if (level.modeset == MATCH && no_spec->value && bot->client->pers.spectator == SPECTATING && !bot->client->pers.admin && !bot->client->pers.rconx[0])
		spawn_angles[YAW] = rand() % 360;



	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	{
		bot->s.angles[PITCH] = 0;
		bot->s.angles[YAW] = spawn_angles[YAW];
		bot->s.angles[ROLL] = 0;
		VectorCopy(bot->s.angles, client->ps.viewangles);
		VectorCopy(bot->s.angles, client->v_angle);

		bot->enemy = NULL;
		bot->movetarget = NULL;
		bot->acebot.state = BOTSTATE_MOVE;
		bot->acebot.suicide_timeout = level.time + 15.0;

		// Set the current node
		if (respawn)
		{
			bot->acebot.next_move_time = level.time;
			bot->acebot.node_current = ACEND_FindClosestReachableNode(bot, BOTNODE_DENSITY, BOTNODE_ALL);
			bot->acebot.node_goal = bot->acebot.node_current;
			bot->acebot.node_next = bot->acebot.node_current;
			if (bot->acebot.node_current == INVALID) bot->acebot.state = BOTSTATE_WANDER;
// BEGIN HITMEN
			if (!sv_hitmen->value)
//END
				if (!ACEAI_PickShortRangeGoalSpawned(bot))
					ACEAI_PickLongRangeGoal(bot);//todo: lrg weapon
		}

		// If we are not respawning hold off for up to three seconds before releasing into game
		if (!respawn)
		{
			bot->think = ACESP_HoldSpawn; //hypov8 ToDo: causing telfrag of waiting at spawn?
			bot->nextthink = level.time + 0.2 /*0.5 + (rand() % 4)*/; // up to three seconds
		}
		else
		{
			bot->think = ACEAI_Think;
			bot->nextthink = level.time + FRAMETIME;
		}
	}

	bot->s.angles[PITCH] = 0;
	bot->s.angles[YAW] = spawn_angles[YAW];
	bot->s.angles[ROLL] = 0;
	VectorCopy(bot->s.angles, client->ps.viewangles);
	VectorCopy(bot->s.angles, client->v_angle);

	if (bot->solid)
		KillBox(bot);

	gi.linkentity (bot);

	// we don't want players being backward-reconciled to the place they died
	if (antilag->value && bot->solid != SOLID_NOT)
		G_ResetHistory(bot);

	// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
	{
		float timediff;
		// this should only work once we've been killed once and respawned
		timediff = 0;
		if (bot->client->resp.spawntime != 0)
			timediff = level.framenum - bot->client->resp.spawntime;

		// Once we've respawned set the players time alive.
		timediff /= 10;
		if ((timediff > 0) && timediff > (bot->client->resp.timealive))
			bot->client->resp.timealive = timediff;

		bot->client->resp.spawntime = level.framenum;

		Hm_Set_Timers(client);
	}
	// END


	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon(bot);

	if (respawn)
	{
		if (bot->solid != SOLID_NOT || bot->client->resp.enterframe == level.framenum)
		{
			// send effect
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(bot - g_edicts);
			gi.WriteByte(MZ_LOGIN);
			if (bot->solid != SOLID_NOT)
				gi.multicast(bot->s.origin, MULTICAST_PVS);
			else
				gi.unicast(bot, false);
		}
	}

	if (level.intermissiontime)
		MoveClientToIntermission(bot);

}
#endif
///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_Respawn (edict_t *ent)
{
#if 1
		if (ent->acebot.is_bot)
	{
		ent->acebot.isTrigPush = false;
		ent->acebot.enemyID = -1; //hypo add
		ent->acebot.num_weps = 2;  //hypo add. 2= pistol+pipe
		ent->acebot.lastDamageTimer = 0;  //hypo add
		ent->acebot.enemyAddFrame = 0;	//hypov8 add
		ent->acebot.tauntTime = level.framenum + (random() * 100);
		//ent->acebot.spawnedTime = level.framenum + 10; 
		ent->acebot.SRGoal_frameNum = 0;
		ent->enemy = NULL;
		ent->movetarget = NULL;
		ent->acebot.state = BOTSTATE_MOVE;
		ent->acebot.suicide_timeout = level.time + 15.0;

		// Set the current node
		ent->acebot.node_ent = NULL;
		ent->acebot.next_move_time = level.time;
		ent->acebot.node_current = ACEND_FindClosestReachableNode(ent, BOTNODE_DENSITY, BOTNODE_ALL);
		ent->acebot.node_goal = ent->acebot.node_current;
		ent->acebot.node_next = ent->acebot.node_current;
		if (ent->acebot.node_current == INVALID) 
			ent->acebot.state = BOTSTATE_WANDER;

		// BEGIN HITMEN
		if (sv_hitmen->value) //todo: check this
			ACEAI_PickLongRangeGoal(ent);//todo: lrg weapon
		else
		//END
			if (!ACEAI_PickShortRangeGoalSpawned(ent))
				ACEAI_PickLongRangeGoal(ent);//todo: lrg weapon
	}

#else
	if (!(level.modeset == MATCH || level.modeset == PUBLIC))
	{
		self->deadflag = 0;
		gi.dprintf("bot respawned after match\n");
		return; //hypov8 dont respawn, fixes last person dying loosing there mouse pitch
	}


	CopyToBodyQue (self);

	if (teamplay->value)
		ACESP_PutClientInServer(self, true, self->client->pers.team);
	else
		ACESP_PutClientInServer (self,true,0);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time; // +5;//hypov8 add 5 secs to respawn
#endif
}

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
static edict_t *ACESP_FindFreeClient (void)
{
	edict_t *bot = NULL;
	int	i;
	int max_count=0;

	//only effective after level playable
	if (num_bots || num_players)
	{
		int total = num_bots + num_players;
		int maxBotPlyr = (int)sv_bot_max_players->value;
		int maxBot = (int)sv_bot_max->value;

		if (maxBotPlyr && total >= maxBotPlyr)
			return NULL;
		if (maxBot && num_bots >= maxBot)
			return NULL;			
	}
	
	// This is for the naming of the bots
	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number
		
		if(bot->count > max_count)
			max_count = bot->count;
	}

	// Check for free spot
	for (i = (int)maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i /*+ 1*/; //hypov8 bug invalid client number

		if (!bot->inuse)
			break;
	}

	bot->count = max_count + 1; // Will become bot name...

	if (bot->inuse)
		bot = NULL;
	
	return bot;
}

#if 0
/////////// orig
static const char bitch_head[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "030", "031", "032", "033", "040", "041", "042", "043", "044", "045", "046", "047", "048", "055", "056", "057", "058", "059", "060", "061", "070", "071", "072", "073", "074", "080", "600", "601", "602", "603", "604" };
static const char bitch_body[] = { "001", "002", "004", "005", "006", "007", "009", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "030", "031", "032", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "056", "057", "058", "059", "060", "070", "071", "072", "073", "074", "300", "301" };
static const char bitch_legs[] = { "001", "002", "003", "004", "005", "006", "010", "011", "012", "013", "014", "015", "020", "021", "030", "031", "032", "033", "056", "059", "060", "300", "301" };

static const char thug_head[] = { "001", "002", "003", "004", "005", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "026", "027", "028", "029", "030", "031", "035", "036", "041", "043", "044", "045", "046", "047", "048", "049", "051", "052", "053", "054", "055", "056", "057", "058", "063", "064", "065", "066", "070", "071", "072", "073", "074", "080", "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "122", "132", "133", "134", "135", "136", "300", "500", "501", "502", "503", "504", "505", "506", "507", "508", "509", "510", "511", "512", "513", "514", "515", "700" };
static const char thug_body[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "026", "027", "028", "029", "030", "031", "032", "033", "034", "035", "036", "039", "040", "041", "042", "043", "044", "045", "046", "047", "050", "051", "052", "053", "054", "055", "056", "057", "058", "059", "060", "061", "062", "063", "064", "065", "066", "067", "070", "071", "072", "073", "080", "081", "082", "090", "091", "122", "123", "124", "130", "131", "132", "300", "301" };
static const char thug_legs[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "020", "021", "022", "023", "030", "031", "032", "033", "034", "040", "041", "046", "047", "048", "050", "052", "053", "054", "056", "057", "058", "059", "060", "070", "071", "072", "073", "120", "121", "122", "123", "124", "132", "140", "300", "301" };

static const char runt_head[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "030", "031", "032", "033", "034", "035", "036", "037", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "051", "052", "053", "054", "055", "056", "057", "058", "059", "060", "061", "062", "063", "064", "065", "066", "070", "071", "072", "073", "074", "120", "121", "123", "130", "131", "132", "133", "134", "140", "141", "142", "300", "301", "302", "303", "304", "305", "306", "307", "308", "309", "310", "311", "312", "313", "314", "315", "316", "317", "318", "319", "320", "320" };
static const char runt_body[] = { "001", "002", "003", "004", "005", "006", "007", "008", "010", "011", "012", "013", "014", "015", "016", "017", "019", "020", "021", "022", "023", "024", "025", "026", "030", "031", "032", "034", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "051", "052", "053", "055", "056", "060", "061", "062", "063", "064", "065", "066", "070", "071", "072", "073", "074", "081", "120", "121", "123", "124", "130", "131", "132", "140", "141", "300", "301" };

/////////////// matched?
static const char bitch_head[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "030", "031", "032", "033", "040", "041", "042", "043", "044", "045", "046", "047", "048", "055", "056", "057", "058", "059", "060", "061", "070", "071", "072", "073", "074", "080", "600", "601", "602", "603", "604" };
static const char bitch_body[] = { "001", "002", "004", "005", "006", "007", "009", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "030", "031", "032", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "056", "057", "058", "059", "060", "070", "071", "072", "073", "074", "300", "301" };
static const char bitch_legs[] = { "001", "002", "003", "004", "005", "006", "010", "011", "012", "013", "014", "015", "020", "021", "030", "031", "032", "033", "056", "059", "060", "300", "301" };

static const char thug_head[] = { "001", "002", "003", "004", "005", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "026", "027", "028", "029", "030", "031", "035", "036", "041", "043", "044", "045", "046", "047", "048", "049", "051", "052", "053", "054", "055", "056", "057", "058", "063", "064", "065", "066", "070", "071", "072", "073", "074", "080", "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "122", "132", "133", "134", "135", "136", "300", "500", "501", "502", "503", "504", "505", "506", "507", "508", "509", "510", "511", "512", "513", "514", "515", "700" };
static const char thug_body[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "025", "026", "027", "028", "031", "032", "033", "034", "035", "036", "039", "040", "041", "042", "043", "044", "045", "046", "047", "050", "051", "052", "053", "054", "055", "056", "057", "058", "059", "060", "061", "062", "063", "064", "065", "066", "067", "070", "071", "072", "073", "080", "081", "082", "090", "091", "122", "123", "124", "130", "131", "132", "300", "301" };
static const char thug_legs[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "017", "017", "020", "021", "022", "023", "020", "012", "010", "009", "031", "032", "033", "034", "040", "041", "046", "047", "048", "050", "052", "053", "054", "056", "057", "058", "059", "060", "070", "071", "072", "073", "120", "121", "122", "123", "124", "132", "140", "300", "301" };

static const char runt_head[] = { "001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "030", "031", "032", "033", "034", "035", "036", "037", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "051", "052", "053", "054", "055", "056", "057", "058", "059", "060", "061", "062", "063", "064", "065", "066", "070", "071", "072", "073", "074", "120", "121", "123", "130", "131", "132", "133", "134", "140", "141", "142", "300", "301", "302", "303", "304", "305", "306", "307", "308", "309", "310", "311", "312", "313", "314", "315", "316", "317", "318", "319", "320", "320" };
static const char runt_body[] = { "001", "002", "003", "004", "005", "006", "007", "008", "010", "011", "012", "013", "014", "015", "016", "017", "019", "020", "021", "022", "023", "024", "025", "026", "030", "031", "032", "034", "040", "041", "042", "043", "044", "045", "046", "047", "048", "049", "050", "051", "052", "053", "055", "056", "060", "061", "062", "063", "064", "065", "066", "070", "071", "072", "073", "074", "081", "120", "121", "123", "124", "130", "131", "132", "140", "141", "300", "301" };

#endif




///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
void ACESP_SetName(edict_t *bot, char *name, char *skin/*, char *team*/)
{
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];

	// Set the name for the bot.
	if(strlen(name) == 0)
		sprintf(bot_name,"HypBot_%d",bot->count);
	else
		strcpy(bot_name,name);

	Com_sprintf(bot->client->pers.netname, sizeof(bot->client->pers.netname),"%s", bot_name);

	//strcpy(bot->client->pers.netname, name);
	
	// skin
	if(strlen(skin) == 0)
	{
#if 1
		// randomly choose skin 
		int randomSkin = rand() % 56;
		switch (randomSkin)
		{
			//bitch
			case 0:		sprintf(bot_skin, "female_chick/001 005 005");	break;
			case 1:		sprintf(bot_skin, "female_chick/040 006 014");	break;
			case 2:		sprintf(bot_skin, "female_chick/006 009 010");	break;
			case 3:		sprintf(bot_skin, "female_chick/006 006 006");	break;
			case 4:		sprintf(bot_skin, "female_chick/010 012 015");	break;
			case 5:		sprintf(bot_skin, "female_chick/003 013 006");	break;
			case 6:		sprintf(bot_skin, "female_chick/018 015 020");	break;
			case 7:		sprintf(bot_skin, "female_chick/044 017 013");	break;
			case 8:		sprintf(bot_skin, "female_chick/044 019 015");	break;
			case 9:		sprintf(bot_skin, "female_chick/020 020 020");	break;
			case 10:	sprintf(bot_skin, "female_chick/032 032 032"); 	break;
			case 11:	sprintf(bot_skin, "female_chick/046 040 010");	break;
			case 12:	sprintf(bot_skin, "female_chick/061 042 060");	break;
			case 13:	sprintf(bot_skin, "female_chick/600 049 056");	break;
			case 14:	sprintf(bot_skin, "female_chick/059 056 056"); 	break;
			case 15:	sprintf(bot_skin, "female_chick/058 058 056");	break;
			case 16:	sprintf(bot_skin, "female_chick/080 059 059");	break;
			case 17:	sprintf(bot_skin, "female_chick/046 072 011");	break;
			//thug
			case 18:	sprintf(bot_skin, "male_thug/004 004 004"); 	break;
			case 19:	sprintf(bot_skin, "male_thug/008 005 005"); 	break;
			case 20:	sprintf(bot_skin, "male_thug/008 008 008"); 	break;
			case 21:	sprintf(bot_skin, "male_thug/010 010 010");		break;
			case 22:	sprintf(bot_skin, "male_thug/011 011 011");		break;
			case 23:	sprintf(bot_skin, "male_thug/012 012 012");		break;
			case 24:	sprintf(bot_skin, "male_thug/010 013 014");		break;
			case 25:	sprintf(bot_skin, "male_thug/015 015 015");		break;
			case 26:	sprintf(bot_skin, "male_thug/016 016 016"); 	break;
			case 27:	sprintf(bot_skin, "male_thug/017 017 017"); 	break;
			case 28:	sprintf(bot_skin, "male_thug/018 027 023"); 	break;
			case 29:	sprintf(bot_skin, "male_thug/009 019 017"); 	break;
			case 30:	sprintf(bot_skin, "male_thug/031 045 120"); 	break;
			case 31:	sprintf(bot_skin, "male_thug/071 071 071"); 	break;
			case 32:	sprintf(bot_skin, "male_thug/073 073 073"); 	break;
			case 33:	sprintf(bot_skin, "male_thug/024 132 048"); 	break;
			case 34:	sprintf(bot_skin, "male_thug/070 130 052"); 	break;
			case 35:	sprintf(bot_skin, "male_thug/500 123 053"); 	break;
			//runt
			case 36:	sprintf(bot_skin, "male_runt/001 001 017"); 	break;
			case 37:	sprintf(bot_skin, "male_runt/004 002 009"); 	break;
			case 38:	sprintf(bot_skin, "male_runt/004 004 004"); 	break;
			case 39:	sprintf(bot_skin, "male_runt/004 004 013"); 	break;
			case 40:	sprintf(bot_skin, "male_runt/003 005 004"); 	break;
			case 41:	sprintf(bot_skin, "male_runt/010 010 017"); 	break;
			case 42:	sprintf(bot_skin, "male_runt/011 011 013"); 	break;
			case 43:	sprintf(bot_skin, "male_runt/011 012 041"); 	break;
			case 44:	sprintf(bot_skin, "male_runt/019 013 047"); 	break;
			case 45:	sprintf(bot_skin, "male_runt/011 014 132"); 	break;
			case 46:	sprintf(bot_skin, "male_runt/020 016 300"); 	break;
			case 47:	sprintf(bot_skin, "male_runt/021 017 071"); 	break;
			case 48:	sprintf(bot_skin, "male_runt/035 019 132"); 	break;
			case 49:	sprintf(bot_skin, "male_runt/017 024 023"); 	break;
			case 50:	sprintf(bot_skin, "male_runt/019 026 013"); 	break;
			case 51:	sprintf(bot_skin, "male_runt/073 072 072"); 	break;
			case 52:	sprintf(bot_skin, "male_runt/120 120 120"); 	break;
			case 53:	sprintf(bot_skin, "male_runt/121 121 121"); 	break;
			case 54:	sprintf(bot_skin, "male_runt/072 142 017"); 	break;
			default:
			case 55:	sprintf(bot_skin, "male_runt/140 140 140"); 	break;

		}
#else
		int randomSkin = rand() % 3;

		if (randomSkin == 0) // chick model with random skin		head				body			legs
			sprintf(bot_skin,"female_chick/%03d %03d %03d", 1 + (rand() % 22), 12 + (rand() % 10), 1 + (rand() % 6));
		else if (randomSkin == 1) // thug model with random skin
			sprintf(bot_skin,"male_thug/%03d %03d %03d", 8 + (rand() % 24), 1 + (rand() % 36), 1 + (rand() % 17));
		else
		{
			int runtHead[] = {001,002}
			int head;
			int body;
			head = 1 + (rand() % 25);
			body = 1 + (rand() % 8);

			sprintf(bot_skin, "male_runt/%03d %03d %03d", 1 + (rand() % 24), 1 + (rand() % 36), 1 + (rand() % 17));
		}
#endif
	}
	else
		strcpy(bot_skin,skin);

	// initialise userinfo
	memset (userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey(userinfo, "ver", "121");
	Info_SetValueForKey(userinfo, "fov", "90");
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "extras", "0000");
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "name", bot_name);

	Info_SetValueForKey(userinfo, "gl_mode", "1");
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!
	Info_SetValueForKey(userinfo, "ip", "loopback");
	Info_SetValueForKey(userinfo, "msg", "0");

#if 0
	{
		edict_t	*doot;
		int j;
		// send joined info if bot was added during gameplay
		gi.dprintf("ACE: bot (%s) connected\n", bot_name);

		for_each_player_not_bot(doot, j)
			safe_cprintf(doot, PRINT_CHAT, "%s connected from %s\n", bot_name, bot->client->pers.country);
	}
#endif	

	ACESP_ClientConnect(bot, userinfo);

	//ACESP_SaveBots(); // make sure to save the bots
}

extern void Teamplay_AutoJoinTeam(edict_t *self);

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_SpawnBot (char *team, char *name, char *skin, char *userinfo, float skill)
{
	edict_t	*bot;
	int team2=0;
	
	bot = ACESP_FindFreeClient ();
	
	if (!bot)
	{
		gi.dprintf("Server is full, increase bot/player max values.\n");
		return;
	}

 //hypov8 added here when multiple bots get added at match start(g_runframe not called yet)
	num_bots++;


	bot->flags &= ~FL_GODMODE;
	bot->health = 0;
	meansOfDeath = MOD_UNKNOWN;
	bot->acebot.enemyID = -1;
	bot->yaw_speed = 100; // yaw speed //hypov8 bots turn speed
	bot->inuse = true;
	bot->acebot.is_bot = true;
	bot->acebot.botSkillMultiplier = skill;
	strncpy(bot->client->pers.country, "Botville", sizeof(bot->client->pers.country) - 1); //GeoIP2

	bot->client->pers.team = 0; //hypo set default

	if (teamplay->value)
	{
		if (team != NULL && team[0] != '\0') // && team[0] != '0') //hypo console spits out '\0'
		{
			if (team[0] == 'd' || team[0] == 'D' || team[0] == '1')
				bot->client->pers.team = TEAM_1;
			else if (team[0] == 'n' || team[0] == 'N' || team[0] == '2')
				bot->client->pers.team = TEAM_2;
			else Teamplay_AutoJoinTeam(bot); //add hypov8 auto team
		}
		else //null
		{
			Teamplay_AutoJoinTeam(bot); //add hypov8 auto team
		}
	}


	if (teamplay->value)
		team2 = bot->client->pers.team;

	// To allow bots to respawn
	if(userinfo == NULL)
		ACESP_SetName(bot, name, skin/*, team*/);
	else
		ACESP_ClientConnect(bot, userinfo);
	
	G_InitEdict (bot);

	//InitClientResp (bot->client);
	//hypov8 reset scores?
	InitClientResp(bot->client);

	bot->client->pers.team = team2;
	// locate ent at a spawn point
	ACESP_PutClientInServer(bot, false, bot->client->pers.team /*TEAM_NONE*/);

	// make sure all view stuff is valid
	ClientEndServerFrame (bot);

}


///////////////////////////////////////////////////////////////////////
// Load a predefined bot cfg
///////////////////////////////////////////////////////////////////////

int ACESP_LoadRandomBotCFG(void)
{
	FILE *pIn;
	char buffer[MAX_STRING_LENGTH];
	int i, count;
	cvar_t	*game_dir;
	char filename[MAX_QPATH];
	char *line, *token;
	static qboolean flip;
	qboolean valid;

	level.bots_spawned = true;

	game_dir = gi.cvar("game", "", 0);
	Com_sprintf(filename, sizeof(filename), "%s/bots/_vote_bots.cfg", game_dir->string); // comp\bots\mapname.cfg  


	count = 0;
	if ((pIn = fopen(filename, "r")) != NULL)
	{
		while (fgetline(pIn, buffer))
		{
			valid = false;
			line = buffer;

			for (i = 1; i <= 4; i++)
			{
				token = COM_Parse(&line);
				if (token[0] == '\0'){
					if (i != 2 && i != 3) //allow null skin and null team
						break;
				}


				switch (i)
				{
				case 1: strcpy(randomBotSkins[count].name, token); break;
				case 2: strcpy(randomBotSkins[count].skin, token); break;
				case 3: strcpy(randomBotSkins[count].team, token); break;
				case 4: randomBotSkins[count].skill = (float)atof((const char *)token); 
						break;
				}

				//only name needs to exist
				if (!valid && i == 1)
				{
					valid = true;
					//flip = flip ? false : true;
					//team = flip ? "nikki": "dragon";
					strcpy(randomBotSkins[count].skin, ""); //male_thug/001 001 001
					strcpy(randomBotSkins[count].team, "");//team
					randomBotSkins[count].skill = 1.0f;
				}
			}
			if (valid)
			{
				count++; //add to skin count
				if (randomBotSkins[count].skill > 2)
					randomBotSkins[count].skill = 2;
				else if (randomBotSkins[count].skill < 0.0f)
					randomBotSkins[count].skill = 0.0f;
			}
		}

		fclose(pIn);
		return count;
	}

	return 0;
}


void ACESP_SpawnBot_Random(char *team, char *name, char *skin, char *userinfo)
{
	edict_t	*bot;
	int i, j, k, l, count;
	int randm, numBotCFGs;
	int countArray[64]; //64 max players

	level.bots_spawned = true;

	numBotCFGs = ACESP_LoadRandomBotCFG(); //check file every time?
	if (numBotCFGs)
	{
		// run through all players/bots names. add a random bot
		memset(&countArray, false, sizeof(countArray));
		j = 0;
		if (numBotCFGs)
		{

			//1024 should n more than enough times to test random
			for (k = 0; k < 1024; k++) 
			{
				count = 0; 
				randm = rand() % numBotCFGs;

				for_each_player_inc_bot(bot, i)
				{
					//if (!bot->acebot.is_bot) continue; //hypo allow clients to use bot names?
					if (Q_stricmp(randomBotSkins[randm].name, bot->client->pers.netname) == 0)
					{
						count++;
						break;
					}
				}

				if (!count)
				{
					if (teamplay->value) // name, skin, team 
						ACESP_SpawnBot(team, randomBotSkins[randm].name, randomBotSkins[randm].skin, NULL, randomBotSkins[randm].skill); //sv addbot thugBot "male_thug/009 031 031" dragon
					else // name, skin			
						ACESP_SpawnBot("\0", randomBotSkins[randm].name, randomBotSkins[randm].skin, NULL, randomBotSkins[randm].skill); //sv addbot thugBot "male_thug/009 031 031"
					
					return;
				}

				countArray[randm] = true;

				//check if every cfg name has been compared( because of random)
				for (l = 0; l < numBotCFGs; l++)
				{
					if (countArray[l] == false)
						break;

					//run out of names. spawn generic
					if (l == numBotCFGs-1)
						ACESP_SpawnBot(team, name, skin, userinfo, 1.0f);
				}
			}
		}
	}
	else	//no bot cfg
		ACESP_SpawnBot(team, name, skin, userinfo, 1.0f);
}


///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void ACESP_RemoveBot(char *name, qboolean print)
{
	int i;
	qboolean freed=false;
	edict_t *bot;


	if (!level.bots_spawned || num_bots <= 0) /*|| !(level.modeset == MATCH || level.modeset == PUBLIC)*/
	{
		//gi.dprintf("Cannot use removebot right now\n");
		return;
	}

	for(i=1;i<=(int)maxclients->value;i++)
	{
		bot = g_edicts + i ;
		if(bot->inuse && bot->acebot.is_bot) //hypov8 Q_stricmp
		{
			if (Q_stricmp(bot->client->pers.netname, name) == 0 || Q_stricmp(name, "all") == 0 || Q_stricmp(name, "single") == 0)
			{
				freed = true;
				if (print)
					safe_bprintf (PRINT_MEDIUM, "%s (bot) removed\n", bot->client->pers.netname);

				ClientDisconnect(bot);//add hypov8
				bot->svflags |= SVF_NOCLIENT;

				if (game.clients[i-1].pers.is_bot)
					memset(&game.clients[i-1], 0, sizeof(gclient_t)) ;

				if (Q_stricmp(name, "single") == 0) //hypov8 remove 1 bot then exit
					break;
			}
		}
	}

	if(!freed)	
		gi.dprintf("%s (bot) not found\n", name);
		//safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
}


//respawn(self);
void ACESP_KillBot(edict_t *self)
{
	self->client->latched_buttons = 0;
	self->acebot.suicide_timeout = level.time + 10.0; //reset since not using ACESP
	self->flags &= ~FL_GODMODE; //hypov8 added. shown as player killed them selves now
	self->health = 0;
	meansOfDeath = MOD_BOT_SUICIDE;		//hypov8 added. shown as player killed them selves now
	VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement
#if HYPODEBUG
	gi.dprintf(" ACE: Bot %s Died at XYZ:(%f, %f, %f)\n", self->client->pers.netname, self->s.origin[0], self->s.origin[1], self->s.origin[2]);
#endif
	player_die(self, self, self, 100000, vec3_origin, 0, 0); //hypov8 add null
}


//fix for using console to change map
void ACESP_FreeBots(void)
{
	int		i;

	for (i = 0; i < game.maxclients; i++)
	{
		if (game.clients[i].pers.is_bot)
			memset(&game.clients[i], 0, sizeof(gclient_t)) ;
	}
}
