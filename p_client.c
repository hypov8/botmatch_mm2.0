
#include "g_local.h"
#include "m_player.h"
// BEGIN HITMEN
//#include "stdlog.h"    // StdLog
//#include "gslog.h"    // StdLog
#include "g_hitmen.h"
// END

#include "voice_bitch.h"
#include "voice_punk.h"

#include <stddef.h>

#define NAME_CLASH_STR    "<Name Clash>"
#define NAME_BLANK_STR    "<No Name>"

void ClientUserinfoChanged (edict_t *ent, char *userinfo);
static int CheckClientRejoin(edict_t *ent);

void think_new_first_raincloud_client (edict_t *self, edict_t *clent);
void think_new_first_snowcloud_client (edict_t *self, edict_t *clent);

static void playerskin(int playernum, char *s)
{
	// only update player's skin config if it has changed (saves a bit of bandwidth)
	if (strcmp(level.playerskins[playernum], s))
	{
		strncpy(level.playerskins[playernum], s, sizeof(level.playerskins[playernum]) - 1);
		gi.configstring(CS_PLAYERSKINS + playernum, s);
	}
}

char *ChatName(char *name)
{
	static char buf[20];
	if (name[0] == ':' || name[0] == '>')
	{
		// add a space so that it isn't confused for chat or yellow text
		Com_sprintf(buf, sizeof(buf) - 1, " %s", name);
		return buf;
	}
	return name;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 48)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 48)
potential spawning position for deathmatch games

  style - team # for Teamplay (1 or 2)
*/
void SP_info_player_deathmatch(edict_t *self)
{
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 48)
potential spawning position for coop games
*/

void SP_info_player_coop(edict_t *self)
{
}


/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}

void SP_info_box_intermission(void)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage, int mdx_part, int mdx_subobject)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}


qboolean IsFemale (edict_t *ent)
{
	if (!ent->client)
		return false;

	if (ent->gender == GENDER_FEMALE)
		return true;

	return false;
}

qboolean IsNeutral (edict_t *ent)
{
	if (!ent->client)
		return false;

	if (ent->gender == GENDER_NONE)
		return true;

	return false;
}

void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int			mod;
	char		*message;
	char		*message2;
	qboolean	ff;
	int			cashreturn = 0;

	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		// in deathmatch, track deaths
		if (mod != MOD_TELEFRAG && (int)teamplay->value != 1)
			self->client->resp.deposited++;

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides";
			break;
// ACEBOT_ADD
			case MOD_BOT_SUICIDE: //added hypov8 console write bot death
				message = "bot stuck, suicides";
				break;
// ACEBOT_END
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			if(self->client->fakeThief > 0)
				cashreturn = self->client->fakeThief;
			break;
		/*case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;*/
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			if(self->client->fakeThief > 0)
				cashreturn = self->client->fakeThief;
			break;
    // RAFAEL
		case MOD_GEKK:
		case MOD_BRAINTENTACLE:
			message = "that's gotta hurt";
			break;
		case MOD_SAFECAMPER:
			message = "stayed in the safe too long";
			break;
		case MOD_ELECTRIC:
			message = "was electrocuted";
			break;
	}
	if (attacker == self)
	{
		switch (mod)
		{
			case MOD_HELD_GRENADE:
				message = "tried to put the pin back in";
				break;
			case MOD_HG_SPLASH:
			case MOD_G_SPLASH:
				if (IsNeutral(self))
					message = "tripped on its own grenade";
				else if (IsFemale(self))
					message = "tripped on her own grenade";
				else
					message = "tripped on his own grenade";
				break;
			case MOD_R_SPLASH:
			case MOD_EXPLOSIVE:
			case MOD_BARREL:
				if (IsNeutral(self))
					message = "blew itself up";
				else if (IsFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			case MOD_BFG_BLAST:
				message = "should have used a smaller gun";
				break;
			// RAFAEL 03-MAY-98
			case MOD_TRAP:
			 	message = "sucked into his own trap";
				break;
			case MOD_FLAMETHROWER:
				if (IsNeutral(self))
					message = "roasted itself";
				else if (IsFemale(self))
					message = "roasted herself";
				else
					message = "roasted himself";
				break;
			}
		}
		if (message)
		{
			safe_bprintf(PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			{
// HYPOV8_ADD
				if (mod != MOD_BOT_SUICIDE) //add hypov8 stop stuck bots loosinbg frags
// HYPOV8_END
					self->client->resp.score--;

				// BEGIN HITMEN
				if (sv_hitmen->value /*enable_hitmen*/)
				{
					// Increase the suicide counter.
					self->client->resp.suicides++;

					// Because we killed ourselves we want to end our kill streak so
					// we have to check if we had a better streak than before.
					if (self->client->resp.killstreak > self->client->resp.maxkillstreak)
						self->client->resp.maxkillstreak = self->client->resp.killstreak;

					// Reset the kill streak.
					self->client->resp.killstreak = 0;
				}
				// END

				if ((int)teamplay->value == TM_GANGBANG)
				{
					team_cash[self->client->pers.team]--;
					UpdateScore();
				}
			}
			self->enemy = NULL;

			if (cashreturn)
			{
				int otherteam = 3 - self->client->pers.team;
				self->client->resp.stole -= cashreturn;
				self->client->pers.bagcash -= cashreturn;
				team_cash[otherteam] += cashreturn;
				UpdateScore();
				last_safe_deposit[otherteam] = level.time;
				safe_bprintf (PRINT_MEDIUM, "$%d was returned to %s's safe\n", cashreturn, team_names[otherteam]);
			}
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client)
		{
			switch (mod)
			{
			case MOD_EXPLOSIVE:
			case MOD_BARREL:
				if(attacker!=self)
					message = "was blown up by";
				break;
			case MOD_BLACKJACK:
				message = "was mashed by";
				break;
			case MOD_CROWBAR:
				message = "was severely dented by";
				break;
			case MOD_PISTOL:
				message = "was busted by";
				message2 = "'s cap";
				break;
			case MOD_SILENCER:
				message = "was silenced by";
				break;
			case MOD_SHOTGUN:
				message = "accepted";
				message2 = "'s load";
				break;
			case MOD_MACHINEGUN:
				message = "bows to";
				message2 = "'s Tommygun";
				break;
			case MOD_FLAMETHROWER:
				message = "roasted in";
				message2 = "'s torch";
				break;
			case MOD_GRENADE:
				message = "fumbled";
				message2 = "'s grenade";
				break;
			case MOD_G_SPLASH:
				message = "was mortally wounded by";
				message2 = "'s shrapnel";
				break;
			case MOD_ROCKET:
				message = "was minced by";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "couldn't escape";
				message2 = "'s blast";
				break;
			case MOD_TELEFRAG:
				message = "couldn't co-exist with";
				message2 = "";
				break;
			// JOSEPH 16-APR-99
			case MOD_BARMACHINEGUN:
				message = "was maimed by";
				message2 = "'s H.M.G.";
			// END JOSEPH
			// BEGIN HITMEN
				break;
			case MOD_HOOK_DAMAGE1:
				message = "was gutted by";
				message2 = "'s hook";
				break;
			case MOD_HOOK_VAMPIRE1:
				message = "was vampired1 by";
				message2 = "'s hook";
				break;
			case MOD_HOOK_VAMPIRE2:
				message = "was vampired2 by";
				message2 = "'s hook";
				break;
			// END
			}
			if (message)
			{
				safe_bprintf(PRINT_MEDIUM, "%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				if (enable_killerhealth)
					safe_cprintf (self, PRINT_MEDIUM, "%s had %i health\n", attacker->client->pers.netname, attacker->health);
				if (mod != MOD_TELEFRAG)
				{
					if (ff)
					{
						attacker->client->resp.score--;

						if ((int)teamplay->value == TM_GANGBANG)
						{
							team_cash[attacker->client->pers.team]--;
							UpdateScore();
						}
					}
					else
					{
#if 1	//MH Score
						if ((int)teamplay->value != 1 && !attacker->client->showscores )
						{
							attacker->client->message_name = self->client->pers.netname;
							attacker->client->message_bonus = false;
							if (attacker->client->message_frame > level.framenum + 10)
								attacker->client->message_count++;
							else
								attacker->client->message_count = 1;
							attacker->client->message_frame = level.framenum + 20;
							attacker->client->resp.scoreboard_frame = 0;
						}
#endif



						if (!(int)teamplay->value && (int)bonus->value && attacker->client->resp.time >= 600 && (attacker->client->resp.score + (int)bonus->value) <= self->client->resp.score
							&& (attacker->client->resp.score * 36000 / attacker->client->resp.time) < (self->client->resp.score * 36000 / self->client->resp.time))
						{
							attacker->client->resp.score++;
							attacker->client->message_bonus = true; //MH Score
							//safe_cprintf(attacker, PRINT_MEDIUM, "You received a bonus hit\n"); //MH: Score. disable old msg
						}

						// BEGIN HITMEN
						if (sv_hitmen->value /*enable_hitmen*/)
						{
							attacker->client->resp.killstreak++;
							self->client->resp.deaths++;

							// Oh dear we were just killed so that ends our kill streak.
							if (self->client->resp.killstreak > self->client->resp.maxkillstreak)
								self->client->resp.maxkillstreak = self->client->resp.killstreak;

							self->client->resp.killstreak = 0;
						}
						// END

						attacker->client->resp.score++;

						if ((int)teamplay->value == TM_GANGBANG)
						{
							team_cash[attacker->client->pers.team]++;
							UpdateScore();
						}
					}
				}
				return;
			}
		}
	}

	{
		self->client->resp.score--;

		if ((int)teamplay->value == TM_GANGBANG)
		{
			team_cash[self->client->pers.team]--;
			UpdateScore();
		}
	}
}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	// RAFAEL
	qboolean	quadfire;
	float		spread;

	// BEGIN HITMEN - We never want to drop weapons in Hitmen when we die.
	if (sv_hitmen->value /*enable_hitmen*/)
		return;
	// END
	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;

//	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
//	else
//		quad = (self->client->quad_framenum > (level.framenum + 10));

	// RAFAEL
//	if (!((int)(dmflags->value) & DF_QUADFIRE_DROP))
		quadfire = false;
//	else
//		quadfire = (self->client->quadfire_framenum > (level.framenum + 10));

	
	if (item && quad)
		spread = 22.5;
	else if (item && quadfire)
		spread = 12.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}

	// RAFAEL
	if (quadfire)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quadfire"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quadfire_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
}

/*
==================
player_die
==================
*/
extern void VelocityForDamage (int damage, vec3_t v);

void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point, int mdx_part, int mdx_subobject)
{
	int		n;

	// make sure the body shows up in the client's current position
	G_UnTimeShiftClient( self );

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

//	self->s.modelindex2 = 0;	// remove linked weapon model
	self->s.model_parts[PART_GUN].modelindex = 0;

	self->s.renderfx2 &= ~RF2_FLAMETHROWER;
	self->s.renderfx2 &= ~RF2_MONEYBAG;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -4;

//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag && (self->health + damage > 0))
	{
		self->client->respawn_time = level.time + 1.0;
		LookAtKiller (self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);
		TossClientWeapon (self);
		if (!self->client->showscores)
			Cmd_Help_f (self, 0);		// show scores

		// clear inventory
		// this is kind of ugly, but it's how we want to handle keys in coop
		for (n = 0; n < game.num_items; n++)
		{
			self->client->pers.inventory[n] = 0;
		}

		// yell at us?
		if (rand()%6 == 0 && attacker->last_talk_time < (level.time - TALK_FIGHTING_DELAY))
		{
			if (attacker->gender == GENDER_MALE)
				Voice_Random(attacker, self, fightsounds, NUM_FIGHTING);
			else if (attacker->gender == GENDER_FEMALE)
				Voice_Random(attacker, self, f_fightsounds, F_NUM_FIGHTING);
		}

		// drop cash if we have some
		if ((int)teamplay->value == 1)
		{
			// always drop at least 10 bucks to reward the killer (if on other team)
			if ((attacker->client) && (attacker->client->pers.team != self->client->pers.team))
			{
				self->client->pers.currentcash += 10;
				if (self->client->pers.currentcash > MAX_CASH_PLAYER)
					self->client->pers.currentcash = MAX_CASH_PLAYER;

				if (self->client->pers.bagcash < MAX_BAGCASH_PLAYER)
				{
					// if they were killed in the enemy base, reward them with some extra cash
					edict_t	*safes[2] = { NULL, NULL };
					edict_t *cash = NULL;
					while ((cash = G_Find(cash, FOFS(classname), "dm_safebag")) != 0) // HYPOV8_ADD !=0
						safes[cash->style == self->client->pers.team] = cash;
					if (safes[0] && safes[1])
					{
						float dist = VectorDistance(safes[0]->s.origin, self->s.origin);
						if (dist < VectorDistance(safes[1]->s.origin, self->s.origin) / 2)
						{
							if (dist < 512 || gi.inPHS( safes[0]->s.origin, self->s.origin ))
							{
								self->client->pers.bagcash += 30;
								if (self->client->pers.bagcash > MAX_BAGCASH_PLAYER)
									self->client->pers.bagcash = MAX_BAGCASH_PLAYER;
							}
						}
					}
				}
			}

			DropCash(self);
		}
#if 1 //MH: Pan Death Cam
		if (attacker != world && attacker != self)
			self->client->killer_4camera = attacker;
		self->client->ps.viewangles[PITCH] = 20;
		self->client->ps.viewangles[YAW] = self->client->killer_yaw;
		self->client->ps.viewangles[ROLL] = 0;
#endif
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~FL_POWER_ARMOR;

	// RAFAEL
	self->client->quadfire_framenum = 0;

	self->s.renderfx2 = 0;

	if (damage >= 50 && self->health < -30 && !inflictor->client)
	{	// gib
		GibEntity( self, inflictor, damage );
		self->svflags |= SVF_NOCLIENT;
		self->s.renderfx2 |= RF2_ONLY_PARENTAL_LOCKED;
		VectorClear(self->velocity);
	}

	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%4;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crouch_death_01-1;
				self->client->anim_end = FRAME_crouch_death_12;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death1_01-1;
				self->client->anim_end = FRAME_death1_19;
				break;
			case 1:
				self->s.frame = FRAME_death2_01-1;
				self->client->anim_end = FRAME_death2_16;
				break;
			case 2:
				self->s.frame = FRAME_death3_01-1;
				self->client->anim_end = FRAME_death3_28;
				break;
			default:
				self->s.frame = FRAME_death4_01-1;
				self->client->anim_end = FRAME_death4_13;
				break;
			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

	self->deadflag = DEAD_DEAD;

	gi.linkentity (self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
extern void AutoLoadWeapon( gclient_t *client, gitem_t *weapon, gitem_t *ammo );

void InitClientPersistant (gclient_t *client)
{
	gitem_t		*item, *ammo= NULL;

	memset(&client->pers, 0, offsetof(client_persistant_t, version));

	// JOSEPH 5-FEB-99-B
	item = FindItem("Pipe");
	// END JOSEPH
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;
	// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
	{
		item = FindItem("Crowbar");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}
	// END

	// start with bazooka in "rocketmode"
	if (dm_realmode->value == 2)
	{
		item = FindItem("bazooka");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;

		client->ammo_index = ITEM_INDEX(FindItem(item->ammo));
		client->pers.inventory[client->ammo_index] = 30;

		client->pers.weapon = item;

		ammo = FindItem (item->ammo);

		AutoLoadWeapon( client, item, ammo );
	}
	else
	// Ridah, start with Pistol in deathmatch
// BEGIN HITMEN
if (!sv_hitmen->value /*enable_hitmen*/)
// END
	{
#if HYPODEBUG
		item = FindItem("Crowbar");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;
		client->pers.weapon = item;
#else
		item = FindItem("pistol");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;

		client->ammo_index = ITEM_INDEX(FindItem(item->ammo));
		client->pers.inventory[client->ammo_index] = 50;

		client->pers.weapon = item;

		// Ridah, start with the pistol loaded
		ammo = FindItem (item->ammo);

#endif


		AutoLoadWeapon( client, item, ammo );

	}
// BEGIN HITMEN
	else
	{
		// We want the magnum instead of the pistol.
		client->pers.pistol_mods |= WEAPON_MOD_DAMAGE;
		client->pers.pistol_mods |= WEAPON_MOD_ROF;
		client->pers.pistol_mods |= WEAPON_MOD_RELOAD;
		client->pers.silencer_shots = 200;
		//client->pers.inventory[ITEM_INDEX(FindItem("Pistol RoF Mod"))]++;
		//client->pers.inventory[ITEM_INDEX(FindItem("Pistol Reload Mod"))]++;
		//client->pers.inventory[ITEM_INDEX(FindItem("Pistol Magnum Mod"))]++;

		// Set the players initial weapon to whatever the current game one is so
		// they don't feel left out using a pistol against HMG's
		Hm_Setcurrentweapon(client, false);
	}
// END


	client->pers.health			= 100;
	client->pers.max_health		= 100;

	client->pers.max_bullets	= 200;
	client->pers.max_shells		= 100;
	client->pers.max_rockets	= 25;
	client->pers.max_grenades	= 12;
	client->pers.max_cells		= 200;
	client->pers.max_slugs		= 90;

	// RAFAEL
	client->pers.max_magslug	= 50;
	client->pers.max_trap		= 5;

	client->pers.connected = 1;
}

void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));

	client->resp.enterframe = level.framenum;

	// no need to reset cl_nodelta when it's disabled by kpded2
	if (kpded2 && !(int)gi.cvar("sv_allownodelta", "", 0)->value)
		client->resp.checkframe[0] = 0x7fffffff;
}


void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float	PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999;

	for (n = 1; n <= (int)maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0 || player->solid == SOLID_NOT)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance <= 32) // at least partially occupied
			return 0;

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players or any occupied points (unless all occupied)
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint (edict_t *ent)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0, count0 = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		count++;
		if (ent->client->pers.spectator != SPECTATING)
		{
			range = PlayersRangeFromSpot(spot);
			if (range < range1)
			{
				range2 = range1;
				spot2 = spot1;
				range1 = range;
				spot1 = spot;
			}
			else if (range < range2)
			{
				range2 = range;
				spot2 = spot;
			}
			// count occupied spots
			if (!range)
				count0++;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
		spot1 = spot2 = NULL;
	if (count0 == count) // all spots occupied
		count0 = 0;
	else if (count0 > 2)
		count -= count0 - 2;
	count -= (spot1 != NULL) + (spot2 != NULL);

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2 || (count0 > 2 && !PlayersRangeFromSpot(spot)))
			selection++;
	}
	while (selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint (edict_t *ent, qboolean team_spawnbase)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;

	// prevent respawning where they died
	if (ent->deadflag)
		ent->health = 1;

spotagain:

	spot = NULL;
	bestspot = NULL;
	bestdistance = -1;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		// Teamplay, don't go here if it's not in our base
		if (teamplay->value && ent->client->pers.team
			&&	spot->style && spot->style != ent->client->pers.team)	// Never spawn in the enemy base
		{
			continue;
		}

		if (team_spawnbase && spot->style != ent->client->pers.team)
		{
			continue;
		}
		// teamplay, done.

		bestplayerdistance = PlayersRangeFromSpot (spot);

		if ((0.8 * bestplayerdistance) > bestdistance
			|| (bestplayerdistance >= (0.8 * bestdistance) && !(rand() & 3)))
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (!bestspot && team_spawnbase)
	{
		team_spawnbase = false;
		goto spotagain;
	}

	return bestspot;
}

edict_t *SelectDeathmatchSpawnPoint (edict_t *ent)
{
	if (ent->client->pers.spectator == SPECTATING)
		return SelectRandomDeathmatchSpawnPoint(ent);

	// Ridah, in teamplay, spawn at base
	if ((int)teamplay->value == 1 && ent->client->pers.team)
		return SelectFarthestDeathmatchSpawnPoint (ent, true);
	else if ( (int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint (ent, false);
	else
		return SelectRandomDeathmatchSpawnPoint (ent);
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void	SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t	*spot = NULL;

	spot = SelectDeathmatchSpawnPoint (ent);

	// find a single player start spot
	if (!spot)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_start");
		if (!spot)
		{
			gi.error("Couldn't find  SP spawn point\n");

//HYPOV8_ADD //fix for testing maps in sp when missing an info_player_start
			spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
			if (!spot)
				gi.error("Couldn't find DM spawn point\n");
//HYPOV8_END							

		}
	}


	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;	//hypov8 ToDo: why is this +9? move spawn up? MH: check needs to add this into account
	//there is also a +1 when we spawn to. to 10 total
	VectorCopy (spot->s.angles, angles);
}

//======================================================================

//MH:
void BodyImpactSound(edict_t *self, int type, float impact)
{
	char *fn;
	float volume = impact / 800.f;
	if (volume > 1)
		volume = 1;
	if (type == 2)
		fn = "gravel";
	else if (type == 3 || type == 4)
		fn = "metall";
	else if (type == 5)
		fn = "tin";
	else
		fn = "pavement";
	gi.sound(self, CHAN_BODY, gi.soundindex(va("actors/player/bodyfalls/%sd1.wav", fn)), volume, ATTN_IDLE, 0);
}
//end


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->svflags = SVF_NOCLIENT;
	}
}

void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point, int mdx_part, int mdx_subobject)
{
#if 1 //hypov8 gibs
	if (damage > 50)
	{
		// send the client-side gib message
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_GIBS);
		gi.WritePosition (self->s.origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte ( dm_realmode->value == 3 ? 8 : 16 );	// number of gibs
		gi.WriteByte ( 0 );	// scale of direction to add to velocity
		gi.WriteByte ( 16 );	// random offset scale
		gi.WriteByte ( 100 );	// random velocity scale
		gi.multicast (self->s.origin, MULTICAST_PVS);

		// done with this body
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
		self->nextthink = 0;
	}
#endif
}

//MH:
void body_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int footsteptype;

	if (!plane || plane->normal[2] < 0.5 || -self->velocity[2] < 100)
		return;

	// waterlevel is updated after touches, so need to check that here
	if (self->waterlevel || (gi.pointcontents(self->s.origin) & MASK_WATER))
		return;

	footsteptype = 0;
	if (!(surf->flags & SURF_CONCRETE))
	{
		if (surf->flags & SURF_GRAVEL)
			footsteptype = 2;
		else if (surf->flags & SURF_METAL)
			footsteptype = 3;
		else if (surf->flags & SURF_METAL_L)
			footsteptype = 4;
		else if (surf->flags & SURF_SNOW)
			footsteptype = 5;
	}
	BodyImpactSound(self, footsteptype, -self->velocity[2]);
}


void Body_Animate( edict_t *ent )
{
	ent->s.frame++;

	if (ent->s.frame >= ent->cal)
	{
		ent->s.frame = ent->cal;

		if (!(ent->flags & FL_SPAWNED_BLOODPOOL) && ent->groundentity)
		{
			SpawnBloodPool (ent);
			ent->flags |= FL_SPAWNED_BLOODPOOL;
		}
	}

	// sink into ground
	if ((ent->timestamp < (level.time - 5)) && (level.framenum & 1))
	{
		ent->s.origin[2] -= 0.5;
		ent->s.renderfx2 |= RF2_NOSHADOW;

		if (ent->timestamp < (level.time - 12))
		{
			// done with this body
			ent->solid = SOLID_NOT;
			ent->svflags |= SVF_NOCLIENT;
			return;
		}
	}

	ent->nextthink = level.time + 0.1;
}

void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;
	trace_t		tr;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	if (!(body->svflags & SVF_NOCLIENT))
		body->s.event = EV_OTHER_TELEPORT;

	body->cal = ent->client->anim_end;

	body->svflags = ent->svflags;
//	VectorCopy (ent->mins, body->mins);
//	VectorCopy (ent->maxs, body->maxs);

	VectorSet (body->mins, -32, -32, -24);
	VectorSet (body->maxs,  32,  32, -4);

	// don't expand bounding box if it won't fit
	tr = gi.trace(body->s.origin, body->mins, body->maxs, body->s.origin, NULL, MASK_SOLID);
	if (tr.startsolid)
	{
		body->mins[1] = body->mins[0] = -16;
		body->maxs[1] = body->maxs[0] = 16;
	}

	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	VectorCopy (ent->velocity, body->velocity);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->svflags &= ~SVF_NOCLIENT;

	// Ridah so we can shoot the body
	body->svflags |= (SVF_MONSTER | SVF_DEADMONSTER);

	body->cast_info.scale = 1.0;

	body->s.renderfx = 0;
	body->s.renderfx2 = (ent->s.renderfx2 & RF2_ONLY_PARENTAL_LOCKED);
	body->s.renderfx2 |= (ent->s.renderfx2 & (RF2_NOSHADOW | RF2_DIR_LIGHTS));

	body->s.effects = 0;
	body->s.angles[PITCH] = 0;
	// ACEBOT_ADD
	if (ent->acebot.is_bot)
		VectorCopy(ent->acebot.deathAngles, body->s.angles);

	// ACEBOT_END

	body->gender = ent->gender;
	body->deadflag = ent->deadflag;
	body->flags = ent->flags;
	body->groundentity = ent->groundentity;

	body->touch = body_touch; //MH:
	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	body->timestamp = level.time;

	body->think = Body_Animate;
	body->nextthink = level.time + 0.1;

	gi.linkentity (body);

	// if the body que is full, make sure the oldest is sinking
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	if (body->timestamp > (level.time - 5))
		body->timestamp = (level.time - 5);
}


void respawn (edict_t *self)
{
// HYPOV8_ADD
	if (level.modeset != MATCH && level.modeset != PUBLIC)
	{
		self->deadflag = 0;
		gi.dprintf("%s caught respawing after match\n", self->client->pers.netname);
		return; //hypov8 dont respawn, fixes last person dying loosing there mouse pitch
	}
// HYPOV8_END


		if (!(self->svflags & SVF_NOCLIENT))
			CopyToBodyQue (self);
		PutClientInServer (self);

		// EV_OTHER_TELEPORT prevents lerping (unlike EV_PLAYER_TELEPORT)
		self->s.event = EV_OTHER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;


}

//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer (edict_t *ent)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 48};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;

// ACEBOT_ADD
	//add hypov8 stop player moving to a spawnpoint
	qboolean isUsingSpec = false;// HYPOV8_ADD
	short		delta_anglesOld[3];

	if (!ent->acebot.is_bot 
		&& (ent->client->pers.spectator == SPECTATING /*|| (teamplay->value && ent->client->pers.team == 0)*/)
		&& (level.modeset == MATCH || level.modeset == PUBLIC)) //rembers last posi
	{
		VectorCopy(ent->s.origin, spawn_origin);
		VectorCopy(ent->s.angles, spawn_angles);

		for (i = 0; i < 3; i++)
			delta_anglesOld[i] = ent->client->old_pmove.delta_angles[i];

		if (spawn_angles[1] < 0)
			spawn_angles[1] += 360;

		isUsingSpec = true;
	}
	else //end hypov8
// ACEBOT_END

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (ent, spawn_origin, spawn_angles);

	index = ent-g_edicts - 1;
	client = ent->client;

	// deathmatch wipes most client data every spawn
	{
		char		userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		ClientUserinfoChanged (ent, userinfo);
	}

	ent->name_index = -1;

	//hypov8 todo: check this add scoreboard menu. after death
	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->takedamage = DAMAGE_AIM;
	if ((level.modeset == MATCHSETUP) || (level.modeset == MATCHCOUNT)
		|| (level.modeset == PREGAME) || (ent->client->pers.spectator == SPECTATING) || level.intermissiontime)
	{
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->pers.weapon = NULL;
	}
	else
	{
		ent->movetype = MOVETYPE_WALK;
		ent->solid = SOLID_BBOX;
		ent->svflags &= ~(SVF_DEADMONSTER|SVF_NOCLIENT);

		//give 3 seconds of imortality on each spawn (anti-camp) 
		client->invincible_framenum = level.framenum + 6;	//hypov8 allow for antilag. 
		if (anti_spawncamp->value)
			client->invincible_framenum = level.framenum + 15;  //3 seconds 
	}
	// RAFAEL
	ent->viewheight = 40;

	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
//	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~(FL_NO_KNOCKBACK | FL_SPAWNED_BLOODPOOL);

	ent->s.renderfx2 = 0;
	ent->onfiretime = 0;
	ent->cast_info.aiflags |= AI_GOAL_RUN;	// make AI run towards us if in pursuit
	ent->flags &= ~FL_CHASECAM; //hypov8 turn off togglecam
	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);
	ent->cast_info.standing_max_z = ent->maxs[2];
	ent->cast_info.scale = MODEL_SCALE;
	ent->s.scale = ent->cast_info.scale - 1.0;

	ent->hasSelectedPistol = false; // HYPOV8_ADD

// ACEBOT_ADD
	ent->acebot.PM_Jumping = 0;
	ent->acebot.pm_last_node = INVALID;
	ent->acebot.pm_jumpPadMove = false;
// ACEBOT_END

	if (ent->solid)
	{
		trace_t tr;
		tr = gi.trace(spawn_origin, ent->mins, ent->maxs, spawn_origin, NULL, CONTENTS_MONSTER);
		if (tr.startsolid)
		{
			// spawn point is occupied, try next to it
			vec3_t origin1, start, end;
			float elev;
			int c;
			VectorCopy(spawn_origin, end);
			end[2] -= 100;
			tr = gi.trace(spawn_origin, ent->mins, ent->maxs, end, NULL, MASK_SOLID);
			elev = tr.fraction * 100;
			VectorCopy(spawn_origin, origin1);
			for (c=0;;)
			{
				for (i=0; i<4; i++)
				{
					float angle = (spawn_angles[YAW] + i * 90 - 45) / 360 * M_PI * 2;
					start[0] = spawn_origin[0] + cos(angle) * 50;
					start[1] = spawn_origin[1] + sin(angle) * 50;
					start[2] = spawn_origin[2];
					VectorCopy(start, end);
					end[2] -= 25;
					tr = gi.trace(start, ent->mins, ent->maxs, end, NULL, MASK_PLAYERSOLID);
					if (!tr.startsolid && tr.fraction < 1)
					{
						VectorCopy(start, spawn_origin);
						break;
					}
				}
				if (i < 4) break;
				if (++c == 2) break;
				// try another spawn point
				for (i=0; i<3; i++)
				{
					SelectSpawnPoint(ent, spawn_origin, spawn_angles);
					if (!VectorCompare(spawn_origin, origin1))
						break;
				}
				if (i == 3) break;
			}
		}
	}

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	if (((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

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
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255;		// will use the skin specified model
//	ent->s.modelindex2 = 255;		// custom gun model
	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

// ACEBOT_ADD
	if (ent->acebot.is_bot)
		ACESP_Respawn(ent);
	VectorCopy( ent->s.origin,ent->acebot.oldOrigin);
// ACEBOT_END 

// bikestuff
ent->biketime = 0;
ent->bikestate = 0;

// JOSEPH 29-MAR-99
//gi.soundindex ("vehicles/motorcycle/idle.wav");
// gi.soundindex ("motorcycle/running.wav");
//gi.soundindex ("vehicles/motorcycle/decel.wav");
//gi.soundindex ("vehicles/motorcycle/accel1.wav");
//gi.soundindex ("vehicles/motorcycle/accel2.wav");
//gi.soundindex ("vehicles/motorcycle/accel3.wav");
//gi.soundindex ("vehicles/motorcycle/accel4.wav");
// END JOSEPH


// Ridah, Hovercars
	if (g_vehicle_test->value)
	{
		if (g_vehicle_test->value == 3)
			ent->s.modelindex = gi.modelindex ("models/props/moto/moto.mdx");
		else
			ent->s.modelindex = gi.modelindex ("models/vehicles/cars/viper/tris_test.md2");

//		ent->s.modelindex2 = 0;
		ent->s.skinnum = 0;
		ent->s.frame = 0;

		if ((int)g_vehicle_test->value == 1)
			ent->flags |= FL_HOVERCAR_GROUND;
		else if ((int)g_vehicle_test->value == 2)
			ent->flags |= FL_HOVERCAR;
		else if ((int)g_vehicle_test->value == 3)
			ent->flags |= FL_BIKE;
		else if ((int)g_vehicle_test->value == 4)
			ent->flags |= FL_CAR;
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

		ent->s.num_parts = 0;		// so the client's setup the model for viewing

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
		
		memset(&(ent->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		
		ent->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/head.mdx" );
		ent->s.model_parts[ent->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &ent->s.model_parts[ent->s.num_parts-1] );
		if (!ent->s.model_parts[ent->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/head.mdx", &ent->s.model_parts[ent->s.num_parts-1] );

		ent->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/legs.mdx" );
		ent->s.model_parts[ent->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &ent->s.model_parts[ent->s.num_parts-1] );
		if (!ent->s.model_parts[ent->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/legs.mdx", &ent->s.model_parts[ent->s.num_parts-1] );

		ent->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/body.mdx" );
		ent->s.model_parts[ent->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &ent->s.model_parts[ent->s.num_parts-1] );
		if (!ent->s.model_parts[ent->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/body.mdx", &ent->s.model_parts[ent->s.num_parts-1] );

		ent->s.num_parts++;
		ent->s.model_parts[PART_GUN].modelindex = 255;
	}
	else	// make sure we can see their weapon
	{
		memset(&(ent->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		ent->s.model_parts[PART_GUN].modelindex = 255;
		ent->s.num_parts = PART_GUN+1;	// make sure old clients recieve the view weapon index
	}

	// randomize spectator's direction in "no spec" mode
	if (level.modeset == MATCH && no_spec->value && ent->client->pers.spectator == SPECTATING && !ent->client->pers.admin && !ent->client->pers.rconx[0])
		spawn_angles[YAW] = rand() % 360;

	// set the delta angle
	for (i=0 ; i<3 ; i++)
	{
// HYPOV8_ADD 
		if (!isUsingSpec)
			client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);
		else
// HYPOV8_END 
			client->ps.pmove.delta_angles[i] = delta_anglesOld[i];

	}

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	if (ent->solid)
		KillBox (ent);

	gi.linkentity (ent);

	// we don't want players being backward-reconciled to the place they died
	if (antilag->value && ent->solid != SOLID_NOT)
		G_ResetHistory(ent);

	// BEGIN HITMEN
	if (sv_hitmen->value /*enable_hitmen*/)
	{
		float	timediff;
		// this should only work once we've been killed once and respawned
		timediff = 0;
		if (ent->client->resp.spawntime != 0)
			timediff = level.framenum - ent->client->resp.spawntime;

		// Once we've respawned set the players time alive.
		timediff /= 10;
		if ((timediff > 0) && timediff > (ent->client->resp.timealive))
			ent->client->resp.timealive = timediff;

		ent->client->resp.spawntime = level.framenum;

		Hm_Set_Timers(client);
	}
	// END

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);

// ACEBOT_ADD
	if (!ent->acebot.is_bot)
// ACEBOT_END

	if (ent->solid != SOLID_NOT || client->resp.enterframe == level.framenum)
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent - g_edicts);
		gi.WriteByte (MZ_LOGIN);
		if (ent->solid != SOLID_NOT)
			gi.multicast (ent->s.origin, MULTICAST_PVS);
		else
			gi.unicast (ent, false);
	}

	if (level.intermissiontime)
		MoveClientToIntermission (ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.

  NOTE: called every level load/change in deathmatch
=====================
*/
void Teamplay_AutoJoinTeam( edict_t *self );

void ClientBeginDeathmatch (edict_t *ent)
{
	int		save;
// ACEBOT_ADD
	static char current_map[55];
// ACEBOT_END

	save = ent->client->showscores;
	G_InitEdict (ent);
// HYPOV8_ADD	//if ((level.modeset == DM_PRE_MATCH) || (level.modeset == DM_MATCH_RUNNING))
	//hypov8 could be used to rejoin and use old stats? or cleared on exit?
	if ((ent->client->pers.spectator != SPECTATING && (level.modeset == PUBLIC || level.modeset == PUBLICSPAWN || level.modeset == MATCHSPAWN)) || ent->client->ps.pmove.pm_type >= PM_DEAD || ent->client->resp.enterframe == level.framenum)
	{
		if (ent->client->resp.enterframe == level.framenum)
		{
			if (save == SCORE_REJOIN)
			{
				if (level.intermissiontime)
				{
					ClientRejoin(ent, true); // auto-rejoin
					save=0;
				}
			}
			else if (teamplay->value && !ent->client->pers.team && ((int)dmflags->value & DF_AUTO_JOIN_TEAM) && !level.intermissiontime && level.modeset != MATCH)
				Teamplay_AutoJoinTeam( ent );
		}

// ACEBOT_ADD
		if (level.modeset == MATCH || level.modeset == PUBLIC
		|| level.modeset == MATCHSPAWN || level.modeset == PUBLICSPAWN) //free???
		{
			if(teamplay->value == 0)
				if (ent->inuse && ent->client->pers.spectator != SPECTATING)
					ACEIT_PlayerAdded(ent); //only add to bot list if player can enter game
		}									//also called in match begin
// ACEBOT_END

		// locate ent at a spawn point
		PutClientInServer (ent);
		ent->client->pers.idle = curtime;
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		gi.linkentity (ent);
		ent->client->newweapon = ent->client->pers.weapon = NULL;
		ChangeWeapon (ent);
	}
	if (!teamplay->value && ent->client->pers.spectator != SPECTATING && level.modeset != PUBLICSPAWN)
	{
		safe_bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
	}

	if (save && ent->solid == SOLID_NOT)
		ent->client->showscores = save;
// ACEBOT_ADD
	if (ent->acebot.is_bot)
		ent->client->showscores = NO_SCOREBOARD;
// ACEBOT_END
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{
	char *a;

// Papa - either show rejoin or MOTD scoreboards
	if (ent->client->showscores != SCORE_REJOIN || !level.player_num)
		ent->client->showscores = (level.modeset == ENDGAMEVOTE ? SCORE_MAP_VOTE : SCORE_MOTD);

// ACEBOT_ADD //new kpded.exe
	if (ent->acebot.is_bot)
		ent->client->showscores = NO_SCOREBOARD;
	else
	{
		if (!ent->client->pers.is_ready)
		{
			//gi.dprintf("new client: %s\n", ent->client->pers.netname);
			//ent->client->pers.is_ready = true;
		}
	}
// ACEBOT_END

	if (keep_admin_status)
	{
		edict_t *admin = GetAdmin();
		if (admin)
		{
			gi.cvar_set("modadmin", "");
			ent->client->pers.admin = NOT_ADMIN;
		}
		else
		{
			a = gi.cvar("modadmin", "", 0)->string;
			if (a[0] && !strcmp(Info_ValueForKey(a, "ip"), ent->client->pers.ip))
			{
				ent->client->pers.admin = atoi(Info_ValueForKey(a, "type"));
				gi.cvar_set("modadmin", "");
			}
		}
	}
	else
		ent->client->pers.admin = NOT_ADMIN;
	a = gi.cvar("rconx", "", 0)->string;
	if (a[0])
	{
		char *s, buf[MAX_INFO_STRING];
		s=Info_ValueForKey(a, ent->client->pers.ip);
		if (s[0])
		{
			strcpy(ent->client->pers.rconx, s);
			strncpy(buf, a, sizeof(buf)-1);
			Info_RemoveKey(buf, ent->client->pers.ip);
			gi.cvar_set("rconx", buf);
		}
	}

	ent->client->pers.lastpacket = curtime;

	{ // send weather effects
		edict_t *cloud = NULL;
		while ((cloud = G_Find(cloud, FOFS(classname), "elements_raincloud"))!=0) // HYPOV8_ADD !=0
			think_new_first_raincloud_client(cloud, ent);
		while ((cloud = G_Find(cloud, FOFS(classname), "elements_snowcloud"))!=0) // HYPOV8_ADD !=0
			think_new_first_snowcloud_client(cloud, ent);
	}

	{
		InitClientResp (ent->client);
		ClientBeginDeathmatch (ent);
		// make sure all view stuff is valid
		ClientEndServerFrame (ent);
		return;
	}
}

void nameclash_think(edict_t *self)
{
	safe_cprintf(self->owner, PRINT_HIGH, "Another player on the server is already using this name\n");
	G_FreeEdict(self);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	char	*s, *s2;
	char	*extras;
	int		a, update;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		// strcpy (userinfo, "\\name\\badinfo\\skin\\male_thug/018 016 010\\extras\\0");
		strcpy (userinfo, "\\name\\badinfo\\skin\\male_thug/009 019 017\\extras\\0");
	}

	if (!ent->client->resp.enterframe)
		Info_SetValueForKey(userinfo, "msg", "0");

	// set name
	s = Info_ValueForKey (userinfo, "name");
	if (ent->client->pers.netname[0] || !ent->client->resp.enterframe)
	{
	update = false;
		if (strchr(s, '%')) 
		{
			s2 = s;
			while ((s2 = strchr(s2, '%')) != 0) // HYPOV8_ADD !=0
				*s2 = ' ';
			update = true;
		}
		a = strlen(s);
		if (a > 15)	// only 13 chars are shown in scoreboard //hypov8 13 woot!!!!!!!!
			a = 15;
		while (--a >= 0)
		{
			if (s[a] > ' ')
				break;
		}
		if (a < 0) // blank name
		{
			if (ent->client->pers.netname[0])
				s = ent->client->pers.netname; // keep the existing name
			else
			s = NAME_BLANK_STR;
			update = true;
		}
		else
		{
			if (s[a+1])
			{
				s[a+1] = 0;
				update = true;
			}

			if (CheckNameBan(s))
				KICKENT(ent,"%s is being kicked because they're banned!\n");

			{
				// stop name clashes
				edict_t		*cl_ent;
				int i;
				for (i=0 ; i<game.maxclients ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (cl_ent->inuse && cl_ent != ent && !strcmp(cl_ent->client->pers.netname, s))
					{
						if (!ent->client->resp.enterframe)
						{
							edict_t *thinker;
							thinker = G_Spawn();
							thinker->think = nameclash_think;
							thinker->nextthink = level.time + 2 + random();
							thinker->owner = ent;
							safe_bprintf(PRINT_HIGH, "A new player is trying to use %s's name\n", s);
						}
						else
							safe_cprintf(ent, PRINT_HIGH, "Another player on the server is already using this name\n");
						if (ent->client->pers.netname[0])
							s = ent->client->pers.netname; // keep the existing name
						else
						s = NAME_CLASH_STR;
						update = true;
						break;
					}
				}
			}
		}
	}
	if (ent->client->pers.netname[0])
	{
		// has the name changed
		if (strcmp(ent->client->pers.netname, s))
		{
			// stop flooding
			if (level.framenum < ent->client->resp.name_change_frame)
			{
				safe_cprintf(ent, PRINT_HIGH, "Overflow protection: Unable to change name yet\n");
				s = ent->client->pers.netname; // keep the existing name
				update = true;
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s changed name to %s\n", ent->client->pers.netname, s);
				ent->client->resp.name_change_frame = level.framenum+20;
			}
		}
		if (update)
			Info_SetValueForKey(userinfo, "name", s);
	}
	if (s != ent->client->pers.netname)
		strcpy(ent->client->pers.netname, s);

	// set skin
	if (level.framenum < (ent->client->move_frame + 10))
	{
		s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
		if (s[0])
			Info_SetValueForKey(userinfo, "skin", s);
	}
	s = Info_ValueForKey (userinfo, "skin");
	kp_strlwr(s);

	if (teamplay->value)
	{
		// NOTE: skin order is "HEAD BODY LEGS"
		char *skin, *body, *legs;
		char tempstr[MAX_QPATH];
		int i, valid, model_index;

		// Hard-coded skin sets for each model

		static char *valid_models[] = { "female_chick", "male_thug", "male_runt", NULL };
/*		static char *valid_skinsets[][2][2][2] =

			// ordering here is {"LEGS", "BODY"}
			{
				{	// Bitch
					{{"056","057"}, {"056","058"}},		// Team 1
					{{"033","032"}, {"031","031"}}		// Team 2
				},
				{	// Thug
					{{"057","056"}, {"058","091"}},
					{{"031","031"}, {"032","035"}}
				},
				{	// Runt
					{{"058","056"}, {"057","056"}},
					{{"031","030"}, {"032","031"}}
				}
			};*/
		static char *valid_skinsets[][2][2] =

			// ordering here is {"LEGS", "BODY"}
			{
				{	// Bitch
					{"056","058"},		// Team 1
					{"033","032"}		// Team 2
				},
				{	// Thug
					{"058","091"},
					{"031","031"}
				},
				{	// Runt
					{"058","056"},
					{"031","030"}
				}
			};

		// make sure they are using one of the standard models
		valid = false;
		i = 0;
		strcpy( tempstr, s );
		skin = strrchr( tempstr, '/' );

		if (!skin)
		{	// invalid model, so assign a default
			model_index = 2;
			strcpy( tempstr, valid_models[model_index] );

			// also recreate a new skin for "s"
			strcpy( s, tempstr );
			strcat( s, "/001 001 001" );

			valid = true;
		}
		else
		{
			skin[0] = '\0';

			while (valid_models[i])
			{
				if (!Q_stricmp( tempstr, valid_models[i] ))
				{
					valid = true;
					model_index = i;
					break;
				}

				i++;
			}
		}

		if (!valid)
		{	// assign a model
			model_index = -1;

			// look for a gender match
			i = 0;
			while (valid_models[i])
			{
				if (!strncmp( tempstr, valid_models[i], 4 ))
				{
					model_index = i;
					strcpy( tempstr, valid_models[model_index] );
					break;
				}

				i++;
			}

			if (model_index < 0)
			{
				model_index = 2;
				strcpy( tempstr, valid_models[model_index] );
			}
		}

		// At this point, tempstr = model only (eg. "male_thug")

		// check that skin is valid
		skin = strrchr( s, '/' ) + 1;
		skin[3] = skin[7] = '\0';

		body = &skin[4];
		legs = &skin[8];

		i = (ent->client->pers.team ? ent->client->pers.team - 1 : rand() & 1);
			strcpy( body, valid_skinsets[model_index][i][1] );
			strcpy( legs, valid_skinsets[model_index][i][0] );

		skin[3] = skin[7] = ' ';

		// paste the skin into the tempstr
		strcat( tempstr, "/" );
		strcat( tempstr, skin );

		Info_SetValueForKey( userinfo, "skin", tempstr );
		s = Info_ValueForKey (userinfo, "skin");
	}

	// set the gender accordingly
	if (!strncmp(s, "female", 6))
		ent->gender = GENDER_FEMALE;
	else if (!strncmp(s, "male", 4))
		ent->gender = GENDER_MALE;
	else
		ent->gender = GENDER_NONE;

	extras = Info_ValueForKey (userinfo, "extras");

	// combine name and skin into a configstring
	if (ent->client->pers.spectator != SPECTATING || !level.playerskins[ent - g_edicts - 1][0])
	playerskin(ent - g_edicts - 1, va("%s\\%s %s", ent->client->pers.netname, s, extras));

	// fov
	if (((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < (no_zoom->value ? 90 : 1))
		{
			if (no_zoom->value && ent->client->resp.enterframe)
				safe_cprintf(ent, PRINT_HIGH, "FOV zooming is not allowed\n");
			Info_SetValueForKey(userinfo, "fov", "90");
			ent->client->ps.fov = 90;
		}
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (s[0])
		ent->client->pers.hand = atoi(s);

	// screen width
	s = Info_ValueForKey(userinfo, "gl_mode");
	s2 = strchr(s, '(');
	if (s2)
	{
		// resolution set by patch
		ent->client->pers.screenwidth = atoi(s2 + 1);
	}
	else
	{
		switch (atoi(s))
		{
			case 0:
				ent->client->pers.screenwidth = 640;
				break;
			case 1:
				ent->client->pers.screenwidth = 800;
				break;
			case 2:
				ent->client->pers.screenwidth = 960;
				break;
			case 3:
				ent->client->pers.screenwidth = 1024;
				break;
			case 4:
				ent->client->pers.screenwidth = 1152;
				break;
			default:
				// assuming anything else is at least 1280
				ent->client->pers.screenwidth = 1280;
				break;
		}
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);

}



/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char	*value;
	edict_t	*doot;
	int j;


	ent->client = NULL;
	ent->inuse = false;
	ent->flags = 0;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (!value[0]) return false;
	if (SV_FilterPacket(value))
	{
		if (kpded2)
			Info_SetValueForKey(userinfo, "rejmsg", "Banned."); // reason to give the client
		return false;
	}

	// check for a password
	if (password->string[0])
	{
		value = Info_ValueForKey (userinfo, "password");
		if (strcmp(password->string, value) != 0)
			return false;
	}
	
	if (CheckPlayerBan (userinfo))
	{
		if (kpded2)
			Info_SetValueForKey(userinfo, "rejmsg", "Banned."); // reason to give the client
		return false;
	}


	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// clear the respawning variables
	InitClientResp (ent->client);
	{
		memset(&ent->client->pers, 0, sizeof(ent->client->pers));
		InitClientPersistant (ent->client);
		ent->client->pers.connected = -1; // distinguish between initial and map change connections
	}

	value = Info_ValueForKey (userinfo, "ip");
	strncpy(ent->client->pers.ip, value, sizeof(ent->client->pers.ip)-1);

	value = Info_ValueForKey (userinfo, "country");	//GeoIP2
	strncpy(ent->client->pers.country, value, sizeof(ent->client->pers.country)-1); //GeoIP2

	// client exe version
	value = Info_ValueForKey (userinfo, "ver");
	if (value[0])
		ent->client->pers.version = atoi(value);
	else	// assume client is old version
		ent->client->pers.version = 100;

	ent->client->resp.enterframe = 0;
	ent->client->move_frame = ent->client->resp.name_change_frame = -80;  //just to be sure
	ClientUserinfoChanged (ent, userinfo);


// ACEBOT_ADD
	if (!ent->acebot.is_bot || (ent->acebot.is_bot && level.bots_spawned))
	{
// ACEBOT_END
		//print in develope console
		if (ent->client->pers.country[0]) //GeoIP2
			gi.dprintf("%s (%s) connected from %s\n", ent->client->pers.netname, ent->client->pers.ip, ent->client->pers.country);
		else
			gi.dprintf("%s (%s) connected\n", ent->client->pers.netname, ent->client->pers.ip);

		//print in clients console
		for_each_player_not_bot(doot, j)// ACEBOT_ADD
		{
			if ((doot->client->pers.admin == ADMIN) || doot->client->pers.rconx[0])
			{
				if (ent->client->pers.country[0]) //GeoIP2
					safe_cprintf(doot, PRINT_CHAT, "%s (%s) connected from %s\n", ent->client->pers.netname, ent->client->pers.ip, ent->client->pers.country);
				else
					safe_cprintf(doot, PRINT_CHAT, "%s (%s) connected\n", ent->client->pers.netname, ent->client->pers.ip);
			}
			else
			{
				if (ent->client->pers.country[0]) //GeoIP2
					safe_cprintf(doot, PRINT_CHAT, "%s connected from %s\n", ent->client->pers.netname, ent->client->pers.country);
				else
					safe_cprintf(doot, PRINT_CHAT, "%s connected\n", ent->client->pers.netname);
			}
		}
	}

// ACEBOT_ADD
	if (!ent->acebot.is_bot)
// ACEBOT_END
	{
		if (teamplay->value)
			ent->client->pers.spectator = SPECTATING;
		else
			ent->client->pers.spectator = PLAYING;
	}

	// check to see if a player was disconnected
	if (CheckClientRejoin(ent) >= 0)
	{
		ent->client->showscores = SCORE_REJOIN;
		ent->client->pers.spectator = SPECTATING;
	}

	ent->client->pers.idle = curtime;
	ent->client->pers.lastpacket = curtime;
	level.lastactive = level.framenum;

// ACEBOT_ADD
	if (ent->acebot.is_bot)
	{
		ent->client->pers.is_bot = true;
		ent->client->pers.spectator = PLAYING;
		ent->client->showscores = NO_SCOREBOARD;
	}
// ACEBOT_END

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int		playernum;
	int		i;

	if (!ent->client || !ent->client->pers.connected)
		return;

	if (ent->inuse)
	{
		if (ent->solid != SOLID_NOT) 
			DropCash(ent);
		if (ent->client->resp.vote == CALLED_VOTE)
			level.voteset = NO_VOTES;

		playernum = ent - g_edicts - 1;

		if (ent->client->resp.time && (ent->client->pers.team || ent->client->resp.score > 0))
		{
			if (level.player_num == 64)
			{
				memmove(playerlist, playerlist + 1, 63 * sizeof(playerlist[0]));
				level.player_num--;
			}
			playerlist[level.player_num].frags = ent->client->resp.score;
			playerlist[level.player_num].deposits = ent->client->resp.deposited;
			playerlist[level.player_num].stole = ent->client->resp.stole;
			playerlist[level.player_num].acchit = ent->client->resp.acchit;
			playerlist[level.player_num].accshot = ent->client->resp.accshot;
			for (i=0; i<8; i++)
				playerlist[level.player_num].fav[i] = ent->client->resp.fav[i];
			playerlist[level.player_num].team = ent->client->pers.team;
			playerlist[level.player_num].time = ent->client->resp.time;
			strcpy (playerlist[level.player_num].player, level.playerskins[playernum]);
			if (teamplay->value)
			{
				char *p = strrchr(playerlist[level.player_num].player, '/');
				if (!p) goto skiplist; // shouldn't happen but just in case
				memset(p+5, ' ', 7); // ignore body+legs
			}
			level.player_num++;
		}

skiplist:
		// inform any chasers
		for (i=1; i<=(int)maxclients->value; i++)
		{
			if (!g_edicts[i].inuse)
				continue;
			if (!g_edicts[i].client)
				continue;
			if (g_edicts[i].client->chase_target == ent)
				ChaseStop(&g_edicts[i]);
		}

		if (ent->solid != SOLID_NOT)
		{
			// send effect
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (ent - g_edicts);
			gi.WriteByte (MZ_LOGOUT);
			gi.multicast (ent->s.origin, MULTICAST_PVS);
		}

		gi.unlinkentity (ent);
		ent->s.modelindex = 0;
		ent->s.num_parts = 0;
		ent->solid = SOLID_NOT;
		ent->inuse = false;

		playerskin(playernum, "");
// ACEBOT_ADD
		if (level.modeset == MATCH || level.modeset == PUBLIC
		|| level.modeset == MATCHSPAWN || level.modeset == PUBLICSPAWN)
		{
			if (ent->client->pers.spectator != SPECTATING)
			ACEIT_PlayerRemoved(ent);
		}
		if (ent->acebot.is_bot) //fix for "bot checked out"
		{
			ent->acebot.is_bot = false; //bug fix
			ent->classname = "disconnected";
			ent->client->pers.connected = 0;
			return;
		}
// ACEBOT_END
		
		
	}

	if (kpded2 && ent->client->ping < 0)
		safe_bprintf(PRINT_HIGH, "%s is reconnecting\n", ent->client->pers.netname);
	else
		safe_bprintf(PRINT_HIGH, ent->client->pers.connected < 0 ? "%s cancelled connecting\n" : "%s checked out\n", ent->client->pers.netname);

	ent->classname = "disconnected";
	ent->client->pers.connected = 0;
}

//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
#if __linux__
trace_t	__attribute__((callee_pop_aggregate_return(0))) PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
#else
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
#endif
{
	if (pm_passent->health > 0)
	{
		if (nav_dynamic->value)	// if dynamic on, get blocked by MONSTERCLIP brushes as the AI will be
			return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP);
		else
			return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	}
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

unsigned CheckBlock (void *b, int c)
{
	int	v,i;
	v = 0;
	for (i=0 ; i<c ; i++)
		v+= ((byte *)b)[i];
	return v;
}
void PrintPmove (pmove_t *pm)
{
	unsigned	c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j;
	pmove_t	pm;

	vec3_t	bike_premove_vel;

	level.lastactive = level.framenum;
	ent->client->pers.lastpacket = Sys_Milliseconds();

	// BOT! (*nothing* uses impulse command so safe to assume)
	if (ucmd->impulse)
	{
		gi.dprintf("Received impulse: %s (%d)\n", ent->client->pers.netname, ucmd->impulse);
		KICKENT(ent, "%s is being kicked for using a bot!\n");
	}

	level.current_entity = ent;
	client = ent->client;

	if (ucmd->forwardmove|ucmd->sidemove|ucmd->upmove)
		client->move_frame = level.framenum;

	if (ucmd->buttons|ucmd->forwardmove|ucmd->sidemove|ucmd->upmove)
		client->pers.idle = curtime;

	client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
	client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
	client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	if (level.modeset == MATCH && no_spec->value && ent->client->pers.spectator == SPECTATING && !ent->client->pers.admin && !ent->client->pers.rconx[0])
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}

	pm_passent = ent;

	// set up for pmove
	memset (&pm, 0, sizeof(pm));

	if (ent->client->chase_target)
	{
		// snap, for alternate chase modes...
		if (ucmd->upmove && 5 < (level.framenum - ent->client->chase_frame))
		{
			ent->client->chase_frame = level.framenum;
			if (ent->client->chasemode == LOCKED_CHASE)
				ent->client->chasemode = FREE_CHASE;
			else if(ent->client->chasemode == FREE_CHASE)
				ent->client->chasemode = EYECAM_CHASE;
			else
				ent->client->chasemode = LOCKED_CHASE;
			if (ent->client->prechase_ps.fov)
			{
				if (ent->client->chasemode == FREE_CHASE) //hypov8 todo: check this
				{
					ent->client->prechase_ps.pmove.delta_angles[PITCH] = ANGLE2SHORT(ent->client->ps.viewangles[PITCH] - ent->client->resp.cmd_angles[PITCH] + 10);
					ent->client->prechase_ps.pmove.delta_angles[YAW] = ANGLE2SHORT(ent->client->ps.viewangles[YAW] - ent->client->resp.cmd_angles[YAW]);
				}
				ent->client->ps = ent->client->prechase_ps;
			}
			ent->client->resp.scoreboard_frame = 0;
		}//end snap
		if (ent->solid != SOLID_NOT || ent->client->chase_target->solid == SOLID_NOT)
		{	// stop chasing
			ChaseStop(ent);
		}
		else
		{
			if (ent->client->chasemode != FREE_CHASE) return;
			goto chasing;
		}
	}

	if ((ent->flags & FL_CHASECAM) && ent->solid != SOLID_NOT)
	{
		client->ps.pmove.pm_flags |= PMF_CHASECAM;
	}
	else
	{
		client->ps.pmove.pm_flags &= ~PMF_CHASECAM;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
		client->ps.pmove.pm_type = PM_SPECTATOR;

	// Ridah, Hovercars
	else if (ent->flags & FL_HOVERCAR)
	{
		ent->viewheight = 0;
		client->ps.pmove.pm_type = PM_HOVERCAR;

		ent->s.renderfx |= RF_REFL_MAP;		// FIXME: remove this once this flag is set in .mdx
	}
	else if (ent->flags & FL_HOVERCAR_GROUND)
	{
		ent->viewheight = 0;
		client->ps.pmove.pm_type = PM_HOVERCAR_GROUND;

		ent->s.renderfx |= RF_REFL_MAP;		// FIXME: remove this once this flag is set in .mdx
	}
	else if (ent->flags & FL_BIKE)
	{
		client->ps.pmove.pm_type = PM_BIKE;

		ent->s.renderfx |= RF_REFL_MAP;		// FIXME: remove this once this flag is set in .mdx

		if ((client->latched_buttons & BUTTON_ACTIVATE) && (ent->duration < level.time))
		{	// Thruster
			VectorScale( ent->velocity, 2, ent->velocity );
			ent->duration = level.time + 4;

			client->kick_angles[PITCH] = -20;

			safe_cprintf( ent, PRINT_HIGH, "Sound Todo: Thruster\n");
		}

		VectorCopy( ent->velocity, bike_premove_vel );
	}
	else if (ent->flags & FL_CAR)
	{
		// Cars don't use client-side prediction

		client->ps.pmove.pm_type = PM_CAR;
		client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

		ent->s.renderfx |= RF_REFL_MAP;		// FIXME: remove this once this flag is set in .mdx

		// Set the pmove up as usual..

		client->ps.pmove.gravity = sv_gravity->value;
		pm.s = client->ps.pmove;

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
		}

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms

		pm.pointcontents = gi.pointcontents;

		// do controls, then get outta here

		Veh_ProcessFrame( ent, ucmd, &pm );

		goto car_resume;
	}
	// done.

	else if (ent->s.modelindex != 255)
		client->ps.pmove.pm_type = PM_GIB;
	else if (ent->deadflag)
		client->ps.pmove.pm_type = PM_DEAD;
	else
	{

		if (ent->flags & FL_JETPACK)
		{
			client->ps.pmove.pm_type = PM_NORMAL_WITH_JETPACK;	// Ridah, debugging
			gi.dprintf( "SOUND TODO: Jet Pack firing\n" );
			ent->s.sound = gi.soundindex("weapons/flame_thrower/flamepilot.wav");	// this should go into G_SetClientSound()
		}
		else
		{
			client->ps.pmove.pm_type = PM_NORMAL;
		}

	}

chasing:

	client->ps.pmove.gravity = sv_gravity->value;
	pm.s = client->ps.pmove;

	for (i=0 ; i<3 ; i++)
	{
		pm.s.origin[i] = ent->s.origin[i]*8;
		// make sure the velocity can overflow ("trigger_push" may exceed the limit) for back-compatibility (newer GCC may otherwise give 0x8000)
		pm.s.velocity[i] = (int)(ent->velocity[i]*8);
	}

	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
	{
		pm.snapinitial = true;
//		gi.dprintf ("pmove changed!\n");
	}

	pm.cmd = *ucmd;

	pm.trace = PM_trace;	// adds default parms

	pm.pointcontents = gi.pointcontents;

	// perform a pmove
	gi.Pmove (&pm);

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	// JOSEPH 1-SEP-98
	ent->footsteptype = pm.footsteptype;

	//MH:
	if (ent->deadflag && !(ent->svflags & SVF_NOCLIENT) && !ent->groundentity && pm.groundentity && !pm.waterlevel && -ent->velocity[2] >= 100)
		BodyImpactSound(ent, pm.footsteptype, -ent->velocity[2]);
	
	for (i=0 ; i<3 ; i++)
	{
		ent->s.origin[i] = pm.s.origin[i]*0.125;
		ent->velocity[i] = pm.s.velocity[i]*0.125;
	}

	VectorCopy (pm.mins, ent->mins);
	VectorCopy (pm.maxs, ent->maxs);

	// Ridah, Hovercars
	if (!(ent->flags & (FL_HOVERCAR | FL_HOVERCAR_GROUND)))
	// done.
	if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
	{
		int rval;

		rval = rand()%100;
		if (rval > 66)	
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
		else if (rval > 33)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump2.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump3.wav"), 1, ATTN_NORM, 0);
			
// BEGIN Snap, bunnyhop
		if (((int)dmflags->value & DF_NO_BUNNY) && client->land_framenum == level.framenum) // they did a doublejump
		{
			if (client->strafejump_count == 0)
				client->firstjump_frame = level.framenum;
			client->strafejump_count++;
			client->land_framenum--;  // so they wont be equal
			if (client->strafejump_count == 2)
			{
				if (client->firstjump_frame >= level.framenum - 50) // they are bunnyhopping
				{
					float xyspeed = sqrt(ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);
					if (xyspeed > 300)
					{
						// correct their speed back down to 'normal'
						ent->velocity[0] *= 300.0 / xyspeed;
						ent->velocity[1] *= 300.0 / xyspeed;
					}
				}
				client->strafejump_count = 0;
			}
		}
	}
	if (!ent->groundentity && pm.groundentity) // client landing
	{
		client->land_framenum = level.framenum;

	}
// END Snap


// ACEBOT_ADD
	//normal move
	if (ent->acebot.PM_Jumping && ent->groundentity && pm.groundentity)
		ent->acebot.PM_Jumping = 0;

	//start jump
	else if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		ent->acebot.PM_Jumping = 1;

	//landing
	else if (!ent->groundentity && pm.groundentity)
		ent->acebot.PM_Jumping = 2;
// ACEBOT_END



#if 0 //!DEMO
	// bikestuff
	if (ent->flags & (FL_BIKE) || ent->flags & (FL_HOVERCAR | FL_HOVERCAR_GROUND) )
	{
		
		int		oldbikestate;
		qboolean accel = false;
		static  int bikegear = 0;
		float	xyspeed;
		static	float	old_xyspeed;
		vec3_t	xyvel;

		if (ent->flags & FL_BIKE)
		{
			vec3_t diffvec;
			float	difflength, prelength;

			VectorSubtract( bike_premove_vel, ent->velocity, diffvec );

			difflength = VectorLength( diffvec );
			prelength = VectorLength( bike_premove_vel );

			if (	((prelength > 300) && (difflength >= 300)))
//				||	((VectorLength( bike_premove_vel ) > 300) && (DotProduct(bike_premove_vel, ent->velocity) < 0)))
			{
				gi.dprintf( "SOUND TODO: CRASH!\n" );
			}
			else if (pm.wall_collision)
			{
				gi.dprintf( "SOUND TODO: Scraped wall\n");
			}
		}

		VectorCopy( ent->velocity, xyvel );
		xyvel[2] = 0;

		xyspeed = VectorLength( xyvel );
		
		oldbikestate = ent->bikestate;

		if (ucmd->forwardmove > 0 && ((old_xyspeed < xyspeed) || xyspeed>50))
		{
			//gi.dprintf ("ACCEL: %5.3f\n", xyspeed);
			accel = true;
			ent->bikestate = 2;
		}
		else
		{
			//gi.dprintf ("NO ACCEL: %5.3f\n", xyspeed);
			if (ent->bikestate == 2)
				ent->bikestate = 1;
			else if (ent->bikestate == 1)
			{
				if (xyspeed < 100)
					ent->bikestate = 0;
			}
		}

		// need a state change check
		
		if (ent->biketime < level.time || oldbikestate != ent->bikestate)	
		{
			if (xyspeed < 400 && (accel == false))
			{
				if ((bikegear <= 1) || ent->biketime < level.time)
				{
					gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/idle.wav"), 0.5, ATTN_NORM, 0);
					ent->s.sound = 0;
					ent->biketime = level.time + 2.4;
				}

				bikegear = 0;
			}
			else 
			{
				if (accel)
				{
					bikegear = (int)floor((xyspeed+100) / 280);

					if (oldbikestate == 0 || bikegear == 0)
					{
						gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/accel1.wav"), 1, ATTN_NORM, 0);
						ent->s.sound = 0;
						ent->biketime = level.time + 1.8;
						bikegear = 1;
					}
					else
					{
						if (bikegear == 1)
						{
							gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/accel2.wav"), 1, ATTN_NORM, 0);
							ent->s.sound = 0;
							ent->biketime = level.time + 2.4;
						}
						else if (bikegear == 2)
						{
							gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/accel3.wav"), 1, ATTN_NORM, 0);
							ent->s.sound = 0;
							ent->biketime = level.time + 2.4;
						}
/*
						else if (bikegear == 3)
						{
							gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/accel4.wav"), 1, ATTN_NORM, 0);
							ent->biketime = level.time + 2.1;
						}
*/
						else	// TODO: high speed rev (looped)
						{
//							gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/running.wav"), 1, ATTN_NORM, 0);
							ent->s.sound = gi.soundindex ("motorcycle/running.wav");
							ent->biketime = level.time + 9999;
							ent->volume = 1.0;
						}
/*
						bikegear++;
						if (bikegear >= 3)
							bikegear = 3;
*/
					}
				}
				else
				{
					ent->s.sound = 0;
					gi.sound ( ent, CHAN_VOICE, gi.soundindex ("motorcycle/decel.wav"), 1, ATTN_NORM, 0);

					bikegear--;
					if (bikegear > 0 && xyspeed > 100)
					{
						ent->biketime = level.time + 0.7 - (0.2 * bikegear);
						bikegear = 0;		// only do this short one once
					}
					else
					{
						bikegear = 0;
						ent->biketime = level.time + 2.4;
					}
				}
			}
		}

		old_xyspeed = xyspeed;
	}
#endif // DEMO

	ent->viewheight = pm.viewheight;
	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;
	ent->groundentity = pm.groundentity;
	if (pm.groundentity)
	{
		ent->groundentity_linkcount = pm.groundentity->linkcount;

		// if standing on an AI, get off
		if (pm.groundentity->svflags & SVF_MONSTER)
		{
			VectorSet( ent->velocity, rand()%400 - 200, rand()%400 - 200, 200 );

			if (pm.groundentity->maxs[2] == pm.groundentity->cast_info.standing_max_z)
			{	// duck
				if (pm.groundentity->cast_info.move_crouch_down)
					pm.groundentity->cast_info.currentmove = pm.groundentity->cast_info.move_crouch_down;
				pm.groundentity->maxs[2] = DUCKING_MAX_Z;
			}

			// avoid
			pm.groundentity->cast_info.avoid( pm.groundentity, ent, false );

		}
	}

#if 0 //MH
	if (ent->deadflag)
	{
		client->ps.viewangles[ROLL] = 40;
		client->ps.viewangles[PITCH] = -15;
		client->ps.viewangles[YAW] = client->killer_yaw;
	}
	else
#else
	if (!ent->deadflag)
#endif
	{
		VectorCopy (pm.viewangles, client->v_angle);
		VectorCopy (pm.viewangles, client->ps.viewangles);
	}

	gi.linkentity (ent);

	if (ent->movetype != MOVETYPE_NOCLIP)
		G_TouchTriggers (ent);

	// touch other objects
	for (i=0 ; i<pm.numtouch ; i++)
	{
		other = pm.touchents[i];
		for (j=0 ; j<i ; j++)
			if (pm.touchents[j] == other)
				break;
		if (j != i)
			continue;	// duplicated
		if (!other->touch)
			continue;
		other->touch (other, ent, NULL, NULL);
	}

// ACEBOT_ADD 
#if 0	//hypov8 disable ASC. allow players grace against bots.
// ACEBOT_END
	//they shoot...they are mortal
	if (((client->latched_buttons|client->buttons) & BUTTON_ATTACK)
		&& (client->invincible_framenum < level.framenum + 29))
		client->invincible_framenum = 0;
// ACEBOT_ADD 
#endif
// ACEBOT_END

	// JOSEPH 22-JAN-99
	// Activate button is pressed
	if (((client->latched_buttons|client->buttons) & BUTTON_ACTIVATE))
	{
		edict_t		*trav, *best;
		float		best_dist=9999, this_dist;
	
		// find the nearest pull-enabled object 
		trav = best = NULL;
		while ((trav = findradius(trav, ent->s.origin, 48)) != 0) // HYPOV8_ADD !=0
		{
			if (!trav->pullable)
				continue;
			//if (!infront(ent, trav))
			//	continue;
			//if (!visible(ent, trav))
			//	continue;
			if (((this_dist = VectorDistance(ent->s.origin, trav->s.origin)) > best_dist) && (this_dist > 64))
				continue;
			
			best = trav;
			best_dist = this_dist;
		}

		// If we find something to drag
		if (best)
		{
			cplane_t plane;
			
			plane.type = 123;
			best->touch (best, ent, &plane, NULL);	
		
			// Slow down the player
			// JOSEPH 24-MAY-99
			ent->velocity[0] /= 8;
			ent->velocity[1] /= 8;
			// END JOSEPH
		}
	}
	// END JOSEPH

#if !DEMO
car_resume:
#endif

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;
	
	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (!client->weapon_thunk)
		{
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

// BEGIN HITMEN
	// Do we need to add some health or ammo ?
	if (sv_hitmen->value /*enable_hitmen*/)
		Hm_Check_Timers(ent);
// END

// ACEBOT_ADD 
	if (!ent->acebot.is_bot)
	{
		if (ent->acebot.PM_Jumping)
			ACEND_PathMap(ent, true);	// hypov8 auto generate path for bots
		ent->acebot.PM_Jumping = 0;
	}
	else
		ent->client->flashlight = false; //skip flastlight
// ACEBOT_END

	Think_FlashLight (ent);

}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;
	int			buttonMask;

	client = ent->client;

	if (client->resp.kickdelay)
	{
		if (!--client->resp.kickdelay)
		{
			safe_bprintf(PRINT_HIGH, client->resp.kickmess, client->pers.netname);
			gi.AddCommandString(va("kick %i\n", (int)(ent - g_edicts - 1)));
		}
		return;
	}

	if (level.intermissiontime)
		return;

#if HYPODEBUG
	client->pers.lastpacket = curtime;
	client->pers.idle = curtime;
#endif


	if (client->pers.spectator != SPECTATING && curtime-client->pers.lastpacket >= 5000)
	{
		// 5 seconds since last contact from the client
		safe_bprintf(PRINT_HIGH, "%s has lost contact with the server\n", client->pers.netname);
		// make them a spectator
		Cmd_Spec_f(ent);
	}
	else if (client->pers.spectator != SPECTATING && (level.modeset == MATCH || level.modeset == PUBLIC) && level.framenum - level.startframe >= 30)
	{
		if (curtime - client->pers.idle > idle_client->value * 1000)
		{
			safe_bprintf(PRINT_HIGH, "%s has been idle for over %d seconds\n", client->pers.netname, (int)idle_client->value);
			// make them a spectator
			Cmd_Spec_f(ent);
		}
		else
			client->resp.time++;
	}
#if 0 //hypov8 not used
	else if (client->pers.spectator != SPECTATING && level.modeset == PREGAME && level.framenum < level.pregameframes - 40)
	{
		if (client->latched_buttons & BUTTON_ATTACK)
		{
			if (client->showscores != SCOREBOARD)
				client->showscores = SCOREBOARD;
			else
				client->resp.ready ^= 1;
			client->resp.scoreboard_frame = 0;
		}
	}
#endif
	// Ridah, hack, make sure we duplicate the episode flags
	ent->episode_flags |= ent->client->pers.episode_flags;
	ent->client->pers.episode_flags |= ent->episode_flags;

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk)
		Think_Weapon (ent); //run once per serverframe. catch bot not allowed to shoot earlyer?
	else
		client->weapon_thunk = false;

	Think_FlashLight (ent);

	if (ent->deadflag)
	{
		// don't respawn if lagging-out
		if (curtime - ent->client->pers.lastpacket > 100)
			return;

		// wait for any button just going down
		if ( level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
			buttonMask = BUTTON_ATTACK;

			if ( ( client->latched_buttons & buttonMask ) ||
				(((int)dmflags->value & DF_FORCE_RESPAWN) ) )
			{
				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	client->latched_buttons = 0;

	if (client->pers.spectator == SPECTATING)
		return;

	if (!(ent->flags & FL_JETPACK))
	{
		ent->client->jetpack_warned = false;

		if (ent->client->jetpack_power < 15.0)
			ent->client->jetpack_power += 0.05;
	}
	else
	{
		ent->client->jetpack_power -= 0.1;

		if (ent->client->jetpack_power <= 0.0)
		{	// disable the jetpack
			gitem_t	*jetpack;

			jetpack = FindItem("Jet Pack");
			jetpack->use( ent, jetpack );
		}
		else if (!ent->client->jetpack_warned && ent->client->jetpack_power < 5.0)
		{
			ent->client->jetpack_warned = true;
			safe_cprintf( ent, PRINT_HIGH, "SOUND TODO: WARNING: Jet Pack power is LOW\n");
		}
	}
}

#if 0
void cprintf(edict_t *ent, int printlevel, char *fmt, ...) //hypov8 todo: not in mm2.0
{
	int n;
	va_list vl;

	assert(printlevel == PRINT_HIGH);

	n = strlen(ent->client->resp.textbuf);
	if (n == sizeof(ent->client->resp.textbuf) - 1)
		return;

	va_start(vl, fmt);
#ifdef _WIN32
	n = _vsnprintf(ent->client->resp.textbuf + n, sizeof(ent->client->resp.textbuf) - n - 1, fmt, vl);
#else
	n = vsnprintf(ent->client->resp.textbuf + n, sizeof(ent->client->resp.textbuf) - n - 1, fmt, vl);
#endif
	va_end(vl);
	if (n < 0)
		ent->client->resp.textbuf[sizeof(ent->client->resp.textbuf) - 2] = '\n';

	safe_cprintf(ent, printlevel, "%s", ent->client->resp.textbuf);
	// kpded2 has its own buffering, so no need to keep text buffered here
	if (kpded2)
		ent->client->resp.textbuf[0] = 0;

}
#endif

static int CheckClientRejoin(edict_t *ent)
{
	int i, playernum = ent - g_edicts - 1;
	char s[MAX_QPATH];

	strcpy(s, level.playerskins[playernum]);
	if (teamplay->value)
	{
		char *p = strrchr(s, '/' );
		memset(p+5, ' ', 7); // ignore body+legs
	}
	for (i=level.player_num-1; i>=0; i--)
	{
		if (!strcmp(s, playerlist[i].player))
			break;
	}
	return i;
}

void ClientRejoin(edict_t *ent, qboolean rejoin)
{
	int	i, index;

	index = CheckClientRejoin(ent);
	if (rejoin && index >= 0)
	{
		ent->client->resp.score = playerlist[index].frags;
		ent->client->resp.deposited = playerlist[index].deposits;
		ent->client->resp.stole = playerlist[index].stole;
		ent->client->resp.acchit = playerlist[index].acchit;
		ent->client->resp.accshot = playerlist[index].accshot;
		for (i=0; i<8; i++)
			ent->client->resp.fav[i] = playerlist[index].fav[i];
		ent->client->pers.team = playerlist[index].team;
		ent->client->resp.time = playerlist[index].time;
		if (ent->client->pers.team || !teamplay->value)
		{
			ent->client->pers.spectator = PLAYING;
			if (ent->client->resp.enterframe != level.framenum)
			{
				if (teamplay->value)
					Teamplay_ValidateJoinTeam(ent,ent->client->pers.team);
				else
					ClientBeginDeathmatch(ent); //hypov8 todo: use this when bot was auto removed??
			}
		}
		else
		{
			ent->client->showscores = SCOREBOARD;
			ent->client->resp.scoreboard_frame = 0;
		}
	}
	else
	{
		if (teamplay->value)
		{
			ent->client->showscores = SCOREBOARD;
			if (((int)dmflags->value & DF_AUTO_JOIN_TEAM) && !level.intermissiontime && level.modeset != MATCH)
				Teamplay_AutoJoinTeam(ent);
		}
		else
		{
			ent->client->showscores = NO_SCOREBOARD;
			ent->client->pers.spectator = PLAYING;
			if (ent->client->resp.enterframe != level.framenum)
				ClientBeginDeathmatch(ent);
		}
		ent->client->resp.scoreboard_frame = 0;
	}
	if (index >= 0)
	{
		level.player_num--;
		memmove(playerlist + index, playerlist + index + 1, (level.player_num - index) * sizeof(playerlist[0]));
	}
}

void DropCash(edict_t *self)
{
	edict_t *cash;

	if (self->client->pers.currentcash)
	{
		cash = Drop_Item(self, FindItemByClassname("item_cashroll"));
		cash->currentcash = self->client->pers.currentcash;
		self->client->pers.currentcash = 0;

		cash->velocity[0] = crandom() * 100;
		cash->velocity[1] = crandom() * 100;
		cash->velocity[2] = 0;

		VectorNormalize( cash->velocity );
		VectorScale( cash->velocity, 100, cash->velocity );
		cash->velocity[2] = 300;
	}

	if (self->client->pers.bagcash)
	{
		if (self->client->pers.bagcash > 100)
			cash = Drop_Item(self, FindItemByClassname("item_cashbaglarge"));
		else
			cash = Drop_Item(self, FindItemByClassname("item_cashbagsmall"));

//		cash->nextthink = level.time + 120;

		cash->currentcash = -self->client->pers.bagcash;
		self->client->pers.bagcash = 0;

		cash->velocity[0] = crandom() * 100;
		cash->velocity[1] = crandom() * 100;
		cash->velocity[2] = 0;

		VectorNormalize( cash->velocity );
		VectorScale( cash->velocity, 100, cash->velocity );
		cash->velocity[2] = 300;
	}
}
