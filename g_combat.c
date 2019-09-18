// g_combat.c

#include "g_local.h"

#include "voice_bitch.h"
#include "voice_punk.h"

int hit;

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/

qboolean CanDamageThroughAlpha (trace_t trace, edict_t *targ, edict_t *inflictor, vec3_t dest)
{
	
	vec3_t	dir;
	vec3_t	alpha_start;
	trace_t	tr;
		
	VectorSubtract (dest, inflictor->s.origin, dir);
	VectorNormalize (dir);
	VectorCopy (trace.endpos, alpha_start);
				
	if (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)
	{
		VectorMA (alpha_start, 4*3, dir, alpha_start);
	}
	else
	{
		VectorMA (alpha_start, 8, dir, alpha_start);
	}	
	
	if (targ->client)
		tr = gi.trace (alpha_start, NULL, NULL, dest, NULL, MASK_SHOT);
	else
	tr = gi.trace (alpha_start, NULL, NULL, dest, inflictor, MASK_SHOT);

	if (tr.ent == targ)
		return true;
	
	return false;
}

qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

	if (targ == inflictor)
		return false;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;
	else if (((trace.ent) && (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((trace.contents & MASK_ALPHA))) 
	{
		VectorCopy (targ->s.origin, dest);
		if (CanDamageThroughAlpha (trace, targ, inflictor, dest))
			return true;
	}

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;
	else if (((trace.ent) && (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((trace.contents & MASK_ALPHA))) 
	{
		VectorCopy (targ->s.origin, dest);
		if (CanDamageThroughAlpha (trace, targ, inflictor, dest))
			return true;
	}


	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;
	else if (((trace.ent) && (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((trace.contents & MASK_ALPHA))) 
	{
		VectorCopy (targ->s.origin, dest);
		if (CanDamageThroughAlpha (trace, targ, inflictor, dest))
			return true;
	}

	
	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;
	else if (((trace.ent) && (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((trace.contents & MASK_ALPHA))) 
	{
		VectorCopy (targ->s.origin, dest);
		if (CanDamageThroughAlpha (trace, targ, inflictor, dest))
			return true;
	}

	
	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;
	else if (((trace.ent) && (trace.ent->s.renderfx2 & RF2_SURF_ALPHA)) || ((trace.contents & MASK_ALPHA))) 
	{
		VectorCopy (targ->s.origin, dest);
		if (CanDamageThroughAlpha (trace, targ, inflictor, dest))
			return true;
	}

	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point, int mdx_part, int mdx_subobject)
{

	if (hit)
	{
		switch (meansOfDeath)
		{
			case MOD_BLACKJACK:
			case MOD_CROWBAR:
				attacker->client->resp.fav[0]++;
				break;
			case MOD_PISTOL:
			case MOD_SILENCER:
				attacker->client->resp.fav[1]++;
				break;
			case MOD_SHOTGUN:
				attacker->client->resp.fav[2]++;
				break;
			case MOD_MACHINEGUN:
				attacker->client->resp.fav[3]++;
				break;
			case MOD_BARMACHINEGUN:
				attacker->client->resp.fav[4]++;
				break;
			case MOD_G_SPLASH:
				attacker->client->resp.fav[5]++;
				break;
			case MOD_ROCKET:
			case MOD_R_SPLASH:
				attacker->client->resp.fav[6]++;
				break;
			case MOD_FLAMETHROWER:
				attacker->client->resp.fav[7]++;
				break;
		}
	}

	if (targ->health < -999)
		targ->health = -999;

	targ->enemy = attacker;

	// turn off Flamethrower
	if (targ->client || (targ->svflags & SVF_MONSTER))
		targ->s.renderfx2 &= ~RF2_FLAMETHROWER;

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point, mdx_part, mdx_subobject);
		return;
	}

	targ->die (targ, inflictor, attacker, damage, point, mdx_part, mdx_subobject);

}


/*
================
SpawnDamage
================
*/
void SpawnDamage (edict_t *self, int type, vec3_t origin, vec3_t normal, int damage)
{
	// make sure the damage shows up where the client currently is
	if (self->client && self->client->saved.time)
	{
		origin[0] += self->client->saved.origin[0] - self->s.origin[0];
		origin[1] += self->client->saved.origin[1] - self->s.origin[1];
		origin[2] += self->client->saved.origin[2] - self->s.origin[2];
	}

	if (damage > 127)
		damage = 127;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
	gi.WritePosition (origin);
	if (type != TE_BLOOD)
		gi.WriteDir (normal);
	// JOSEPH 12-MAY-99-B
	else
	{
		// As per Maxx request
		if (damage > 1)
			damage <<= 1;
		gi.WriteByte( damage );
	}
	// END JOSEPH
	gi.multicast (origin, MULTICAST_PVS);
}


/*
SpawnBloodPool
*/
void SpawnBloodPool (edict_t *self)
{
	vec3_t	origin;
	vec3_t	forward, right, up;
	vec3_t	end;
	vec3_t	vec;
	trace_t	tr;
	int		baseskin, currentskin;

	if (self->onfireent)
	{
		return;
	}

	VectorCopy (self->s.origin, origin);
	
	AngleVectors (self->s.angles, forward, right, up);
	VectorMA (origin, -8192, up, end);

	tr = gi.trace (origin, NULL, NULL, end, self, MASK_SHOT );
	
	if (tr.contents & CONTENTS_SOLID)
	{

		baseskin = self->s.model_parts[PART_HEAD].baseskin;
		currentskin = self->s.model_parts[PART_HEAD].skinnum[0];

		if (baseskin != currentskin)
		{
//			VectorCopy (tr.endpos, vec);
			// this will need to be computed for each mdx
//			VectorMA (vec, -(32), forward, vec);
//			VectorMA( vec, 0.1, tr.plane.normal, tr.endpos );	// lift it above the ground slightly

			// Ridah, use object_bounds to get head position
			if (self->s.model_parts[PART_HEAD].object_bounds[0] > 0)
			{
				vec3_t org, ang, mins, maxs;
				vec3_t forward, right, up;
				vec3_t rmins, rmaxs, pmins, pmaxs;

				VectorCopy (self->s.origin, org);
				VectorCopy (self->s.angles, ang);
				
				VectorCopy (g_objbnds[-1+self->s.model_parts[PART_HEAD].object_bounds[0]][self->s.frame].mins, mins);
				VectorCopy (g_objbnds[-1+self->s.model_parts[PART_HEAD].object_bounds[0]][self->s.frame].maxs, maxs);
				
				AngleVectors (ang, forward, right, up);

				VectorMA (org, ((mins[0] + maxs[0]) * 0.5), forward, org);
				VectorMA (org, -((mins[1] + maxs[1]) * 0.5), right, org);
				VectorMA (org, (mins[2] + maxs[2]) * 0.5, up, org);

				// find rotated positions of mins/maxs, and then build the new min/max
				VectorScale ( forward, mins[0], rmins);
				VectorMA (rmins, -mins[1], right, rmins);
				VectorMA (rmins, mins[2], up, rmins);

				VectorScale ( forward, maxs[0], rmaxs);
				VectorMA (rmaxs, -maxs[1], right, rmaxs);
				VectorMA (rmaxs, maxs[2], up, rmaxs);

				pmins[0] = (rmins[0] < rmaxs[0] ? rmins[0] : rmaxs[0]);
				pmins[1] = (rmins[1] < rmaxs[1] ? rmins[1] : rmaxs[1]);
				pmins[2] = (rmins[2] < rmaxs[2] ? rmins[2] : rmaxs[2]);

				pmaxs[0] = (rmins[0] > rmaxs[0] ? rmins[0] : rmaxs[0]);
				pmaxs[1] = (rmins[1] > rmaxs[1] ? rmins[1] : rmaxs[1]);
				pmaxs[2] = (rmins[2] > rmaxs[2] ? rmins[2] : rmaxs[2]);

				// now align the mins/maxs with the origin
				mins[0] = pmins[0] - (0.5*(pmaxs[0] + pmins[0]));
				mins[1] = pmins[1] - (0.5*(pmaxs[1] + pmins[1]));
				mins[2] = pmins[2] - (0.5*(pmaxs[2] + pmins[2]));
				maxs[0] = pmaxs[0] - (0.5*(pmaxs[0] + pmins[0]));
				maxs[1] = pmaxs[1] - (0.5*(pmaxs[1] + pmins[1]));
				maxs[2] = pmaxs[2] - (0.5*(pmaxs[2] + pmins[2]));

				vec[0] = org[0] + (mins[0] + maxs[0]) / 2;
				vec[1] = org[1] + (mins[1] + maxs[1]) / 2;
				vec[2] = org[2] + (mins[2] + maxs[2]) / 2;

				VectorMA( vec, -8192, up, end );

				tr = gi.trace (vec, NULL, NULL, end, self, MASK_SHOT );

				VectorMA( tr.endpos, 0.1, tr.plane.normal, tr.endpos );	// lift it above the ground slightly
			}
		}
		else
			VectorMA( tr.endpos, 0.1, tr.plane.normal, tr.endpos );	// lift it above the ground slightly

		// JOSEPH 18-DEC-98
		SurfaceSpriteEffect(SFX_SPRITE_SURF_BLOOD_POOL, 2, 2, tr.ent, tr.endpos, tr.plane.normal);
		/*gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLOOD_POOL);
		gi.WritePosition (tr.endpos);
		gi.WriteDir (tr.plane.normal);
		gi.multicast (origin, MULTICAST_PVS);*/
		// END JOSEPH
	}
	else
	{
		// TBD
		// the entity steped on the dead body so now we can leave a bloody trail
		;
	}
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			te_sparks;
	float		dmg;
	vec3_t		sparkspoint;

	hit = 0;

	dmg = (float)abs(damage);	// tical/jjaf

	if (!targ->takedamage)
		return;

	if (targ->cast_info.aiflags & AI_IMMORTAL)
	{
		return;
	}

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (!(dflags & DAMAGE_NO_PROTECTION) && (targ != attacker) && ((deathmatch_value && (teamplay->value || ((int)(dmflags->value) & (DF_MODELTEAMS/*| DF_SKINTEAMS*/))) /*|| coop->value*/)))
	{
		if ((OnSameTeam (targ, attacker)) && (!targ->client->pers.friendly_vulnerable))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
			{
				dmg = 0;
			}
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
		else if (attacker->client && !targ->deadflag)
			hit = 1;
	}
	else if (attacker->client && !targ->deadflag && targ != attacker)
		hit = 1;
	meansOfDeath = mod;

	if (dm_realmode->value == 2)
	{
		// only 20% (actually 10% due to DAMAGE_RADIUS) self-damage in rocketmode
		if (attacker == targ && inflictor != targ)
			dmg /= 5;
		// only 25% falling damage in rocketmode
		else if (mod == MOD_FALLING)
			dmg /= 4;
	}
	else if (dm_realmode->value && attacker != targ && attacker->client)
	{
		dmg *= 4;
	}

	client = targ->client;

	// JOSEPH 11-APR-99
	if ((mod == MOD_BLACKJACK) || (mod == MOD_CROWBAR))
		te_sparks = TE_SPARKS;
	else if (dflags & DAMAGE_BULLET && mod != MOD_DOGBITE)
		te_sparks = TE_BULLET_SPARKS;
	else if (mod != MOD_DOGBITE)
		te_sparks = TE_SPARKS;
	else
		te_sparks = TE_SPARKS;
	// END JOSEPH

	VectorCopy(point, sparkspoint);
	if (dflags & DAMAGE_RADIUS)
	{
		// make the blood come from the player (not the wall)
		sparkspoint[0] = targ->s.origin[0];
		sparkspoint[1] = targ->s.origin[1];
		if (sparkspoint[2] < targ->s.origin[2])
			sparkspoint[2] = targ->s.origin[2];
		else if (sparkspoint[2] > targ->s.origin[2] + targ->maxs[2])
			sparkspoint[2] = targ->s.origin[2] + targ->maxs[2];

		// 50% less radius damage in rocketmode
		if (dm_realmode->value == 2)
			dmg /= 2;
	}

	VectorNormalize(dir);

// bonus damage for suprising a monster
//	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
//		dmg *= 2;

	if (((dflags & DAMAGE_NO_KNOCKBACK)) || (targ->nokickbackflag))
		knockback = 0;


// figure momentum add
	// JOSEPH 7-MAR-99
	if ((!(dflags & DAMAGE_NO_KNOCKBACK)) && (mod == MOD_BLOWBACK))
	{
		vec3_t	kvel;
		float	mass;

		knockback *= 3;
		
		if (knockback < 100)
			knockback = 100;
		else if (knockback > 150)
			knockback = 150;

		if (targ->mass < 50)
			mass = 50;
		else
			mass = targ->mass;

		if (targ->client)
			VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
		else
			VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

		VectorAdd (targ->velocity, kvel, targ->velocity);		
	}
	else if (!(dflags & DAMAGE_NO_KNOCKBACK))
	// END JOSEPH
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			// JOSEPH 13-MAY-99
			if (targ->client  && (attacker == targ) && deathmatch_value)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else if (targ->client  && (attacker == targ))
				VectorScale (dir, 128.0 * (float)knockback / mass, kvel);
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);
			// END JOSEPH
	
			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = dmg;
	save = 0;

	// JOSEPH 1-APR-99-B
	if (targ->client && dmg && (!(dflags & DAMAGE_NO_ARMOR)))
	{
		int     index;
		gitem_t *item = NULL;
		int     random;
		float	takefactor;	

		if (point != targ->s.origin)
		{
			// take armour according to height of hit
			float p = (point[2] - (targ->s.origin[2] + targ->mins[2])) / (targ->maxs[2] - targ->mins[2]);
			if (p < 0.475) // legs
			{
				random = 2;
			}
			else if (p > 0.85) // head
			{
				random = 1;
			}
			else // body
			{
				random = 0;
			}
		}
		// platforms and vertical doors hit head
		else if (mod == MOD_CRUSH && inflictor->velocity[2])
			random = 1;
		else
		// JOSEPH 21-MAY-99
		if (mod == MOD_DOGBITE)
		{
			if (inflictor->s.origin[2] < targ->s.origin[2]) // legs
			{
				random = 2;
			}
			else if (inflictor->s.origin[2] > targ->s.origin[2] + targ->viewheight) // head
			{
				random = 1;
			}
			else // body
			{
				random = 0;
			}
		}
		else
			random = rand()%3;	
		// END JOSEPH


		// JOSEPH 5-APR-99
		if (mod == MOD_FALLING)
		{
			item = FindItem ("Legs Armor");
			takefactor = 0.5;
			if (!(targ->client->pers.inventory[ITEM_INDEX(item)]))
			{
				item = FindItem ("Legs Armor Heavy");
				takefactor = 0.25;						
			}
		}
		else if (random == 0)
		// END JOSEPH
		{
			item = FindItem ("Jacket Armor");
			takefactor = 0.3;
			if (!(targ->client->pers.inventory[ITEM_INDEX(item)]))
			{
				item = FindItem ("Jacket Armor Heavy");
				takefactor = 0.1;						
			}
		}
		else if (random == 1)
		{
			item = FindItem ("Helmet Armor");
			takefactor = 0.3;
			if (!(targ->client->pers.inventory[ITEM_INDEX(item)]))
			{
				item = FindItem ("Helmet Armor Heavy");
				takefactor = 0.1;						
			}
		}
		else 
		{
			item = FindItem ("Legs Armor");
			takefactor = 0.3;
			if (!(targ->client->pers.inventory[ITEM_INDEX(item)]))
			{
				item = FindItem ("Legs Armor Heavy");
				takefactor = 0.1;						
			}
		}
		
	    if ((item) && (targ->client->pers.inventory[ITEM_INDEX(item)]))
		{
			int takehealth;
			int takeshield;

			takehealth = take * takefactor;
			takeshield = take - takehealth;

			// Ridah, make Flamethrower burn armor away faster than normal
			if (mod == MOD_FLAMETHROWER)
			{
				if (takeshield < 1)
					takeshield = 1;

				takeshield *= 3;
			}
			
			index = ITEM_INDEX (item);					
					
			// JOSEPH 5-APR-99
			if (takeshield <= targ->client->pers.inventory[ index ])
			{
				targ->client->pers.inventory[ index ] -= takeshield;
				take = takehealth;
				if (!take)
					take = 1;
			}
			else
			{
				takehealth = ((take - targ->client->pers.inventory[ index ])+
					          (takefactor*targ->client->pers.inventory[ index ])); 
				targ->client->pers.inventory[ index ] = 0;
				take = takehealth;
				if (!take)
					take = 1;
			}
			// END JOSEPH
		}
	}
	// END JOSEPH

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = dmg;
		SpawnDamage (targ, te_sparks, sparkspoint, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			// JOSEPH 29-MAR-99
			//gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			// END JOSEPH
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = dmg;
	}

	// JOSEPH 1-APR-99
	//treat cheat/powerup savings the same as armor
	asave = 0;
	asave += save;
	// END JOSEPH

	// team damage avoidance
	//if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
	//	return;

// do the damage
	if (take)
	{
// ACEBOT_ADD
		if (targ->acebot.is_bot)
			targ->acebot.lastDamageTimer = level.framenum;
		//if (attacker->client)
		//	targ->acebot.lastDamagePlayerIndex = attacker->s.number;
// ACEBOT_END

		if (mod == MOD_FLAMETHROWER)
		{
		}
		else
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
				SpawnDamage (targ, TE_BLOOD, sparkspoint, normal, take);
			else
				SpawnDamage (targ, te_sparks, sparkspoint, normal, take);
		}

		targ->health = targ->health - take;


		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
			{
				targ->flags |= FL_NO_KNOCKBACK;
			}

			// JOSEPH 3-MAR-99
			if (targ->svflags & SVF_MONSTER)
			{
				// targ->solid = SOLID_NOT;			
				targ->svflags |= SVF_DEADMONSTER;
				
				if (mod == MOD_ROCKET)
				{
					targ->s.renderfx2 &= ~RF2_DIR_LIGHTS;
				}
			}
			// END JOSEPH

// Ridah, removed so grenades and rockets can gib bodies
//			if (!targ->deadflag)
				Killed (targ, inflictor, attacker, take, sparkspoint, 0, 0);

			return;
		}
	}

	if (	(targ->svflags & SVF_MONSTER)
		&&	(targ->health > 0))		// Ridah, 31-may-99, possibly fixes Whore crouching death bug
	{
		if (take)
		{
			targ->pain (targ, attacker, knockback, take, 0, 0);
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take, 0, 0);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take, 0, 0);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_armor += asave;
		if (mod == MOD_FLAMETHROWER)
			client->damage_flame += take*4;
		else
			client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}

void SpawnPartShotOff (edict_t *self, int mdx_part, vec3_t dir)
{

	edict_t	*dropped;
	int		skin, model;
	vec3_t	forward, angles;
	int		i;

	if (mdx_part == PART_CIGAR)
	{
		self->count &= ~1; 
		return; // no cigar
	}

	else if (mdx_part == PART_HAT)
	{
		if (self->count & 2)
		{
			model = gi.modelindex("models/props/fedora/fedora.mdx");
			// hat bug
			self->count &= ~2; 
		}
		else if (self->count & 4)
		{
			model = gi.modelindex("models/props/stetson/stetson.mdx");
			// hat bug
			self->count &= ~4; 
		}
		else if (self->count & 8)
		{
			model = gi.modelindex("models/props/cap/cap.mdx");
			// hat bug
			self->count &= ~8; 
		}
	}

	skin = self->s.model_parts[mdx_part].baseskin;

	dropped = G_Spawn();

	VectorSet (dropped->mins, -4, -4, -2);
	VectorSet (dropped->maxs, 4, 4, 2);
	
	memset(&(dropped->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
	
	dropped->s.num_parts++;
	dropped->s.model_parts[PART_HEAD].modelindex = model;
	for (i=0; i<MAX_MODELPART_OBJECTS; i++)
		self->s.model_parts[PART_HEAD].baseskin = skin;
	gi.GetObjectBounds( "models/actors/runt/fedora.mdx", &self->s.model_parts[PART_HEAD] );

	dropped->movetype = MOVETYPE_BOUNCE;
	dropped->clipmask = MASK_SHOT;
	dropped->solid = SOLID_NOT;
	dropped->s.renderfx2 |= RF2_NOSHADOW;

	VectorCopy (self->s.origin, dropped->s.origin);
	dropped->s.origin[2] += 24;
	
	vectoangles (dir, angles);
	AngleVectors (angles, forward, NULL, NULL);
	
	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity [2] += 300;

	gi.linkentity (dropped);
}


void gib_a_little_blood (edict_t *self)
{

	trace_t tr;
	vec3_t	end;
	float	rnd;

	VectorCopy (self->s.origin, end);
	end[2] -= 8000;	

	tr = gi.trace (self->s.origin, NULL, NULL, end, self, MASK_SOLID);

	rnd = (0.5 + 1.5*random());
	SurfaceSpriteEffect(SFX_SPRITE_SURF_BLOOD1, (unsigned char)(rnd*SFX_BLOOD_WIDTH), (unsigned char)(rnd*SFX_BLOOD_HEIGHT),
						tr.ent, tr.endpos, tr.plane.normal);
}

// JOSEPH 18-MAR-99
void Think_gib_fade (edict_t *ent)
{
	if (!ent->misstime--)
	{	
		ent->nextthink = 0;
		G_FreeEdict(ent);
	}
	else
	{
		if (ent->misstime <= 20)
		{
			if (ent->misstime == 20)
			{
				ent->s.renderfx2 |= RF2_PASSALPHA;
				ent->s.effects = 1;		// this is full alpha now
			}
			
			ent->s.effects += (255/20);
		
			if (ent->s.effects > 255)
				ent->s.effects = 255;
		}
	
		if (rand()%100 > 90)
			gib_a_little_blood (ent);
		
		ent->nextthink = level.time + 0.1;
/*
		// Ridah, optimized this to prevent SZ_GetSpace() overrun's
		if (random() < 0.5 && ent->velocity[2])
		{
			static float	last_blood;
			vec3_t	negvel;

//			if (last_blood != level.time)
			{
				VectorScale( ent->velocity, -1, negvel );
				VectorNormalize( negvel );

				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_SPLASH);
				gi.WriteByte (5);
				gi.WritePosition (ent->s.origin);
				gi.WriteDir (negvel);
				gi.WriteByte (6);
				gi.multicast (ent->s.origin, MULTICAST_PVS);

				last_blood = level.time;

			}
		}
*/
	}
}
// END JOSEPH

void gib_end_life (edict_t *self)
{
	gib_a_little_blood (self);

	// JOSEPH 18-MAR-99
	self->think = Think_gib_fade;
	self->nextthink = level.time + 0.1;
	self->misstime = 80;
	// END JOSEPH
}

void SpawnGib (edict_t *self, int mdx_part, int mdx_subobject, vec3_t dir)
{
	edict_t *gib;
	int		i;
	vec3_t	forward, right, up;
	char gibname[1024];
	int		max_rnd;

#if 0//def HYPODEBUG
	return; //hypov8 overflow test?
#endif
	gi.sound(self, CHAN_VOICE, gi.soundindex("actors/player/bodyfalls/jibs.wav"), 1, ATTN_NORM, 0);

	if (mdx_part == PART_BODY && mdx_subobject == 0)	// Torso is big, so allow big chunks
	{	// torso
		max_rnd = 2;
		Com_sprintf (gibname, sizeof(gibname), "models/props/gibs/gib%d.mdx", 3+(rand()%max_rnd) + 1);
	}
	else if (mdx_part == PART_LEGS)
	{
		max_rnd = 3;
		Com_sprintf (gibname, sizeof(gibname), "models/props/gibs/gib%d.mdx", 1+(rand()%max_rnd) + 1);
	}
	else	// arm/leg/head
	{
		max_rnd = 3;
		Com_sprintf (gibname, sizeof(gibname), "models/props/gibs/gib%d.mdx", (rand()%max_rnd) + 1);
	}

	gib = G_Spawn();

	memset(&(gib->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);

	gib->s.num_parts++;
	gib->s.model_parts[PART_HEAD].modelindex = gi.modelindex (gibname);
	for (i=0; i<MAX_MODELPART_OBJECTS; i++)
		gib->s.model_parts[PART_HEAD].baseskin = 0;
	
//	gib->movetype = MOVETYPE_TOSS;
	gib->movetype = MOVETYPE_BOUNCE;

	gib->solid = SOLID_NOT;
	gib->s.renderfx2 |= RF2_NOSHADOW;
	//gib->s.renderfx2 |= RF2_DIR_LIGHTS; //hypov8 disable dir light on gib

	gib->s.effects |= EF_GIB;

//gi.dprintf( "SpawnGib: shot ent: %i\n", (int)(self-g_edicts) );

	VectorCopy (self->s.origin, gib->s.origin);
	VectorCopy (self->s.angles, gib->s.angles);

	gib->s.angles[YAW] += (rand()%32) - 16;

	AngleVectors (gib->s.angles, forward, right, up);
			
	VectorCopy (g_objbnds[-1+self->s.model_parts[mdx_part].object_bounds[mdx_subobject]][self->s.frame].mins, gib->mins);
	VectorCopy (g_objbnds[-1+self->s.model_parts[mdx_part].object_bounds[mdx_subobject]][self->s.frame].maxs, gib->maxs);

	VectorMA (gib->s.origin, ((gib->mins[0] + gib->maxs[0]) * 0.5), forward, gib->s.origin);
	VectorMA (gib->s.origin, -((gib->mins[1] + gib->maxs[1]) * 0.5), right, gib->s.origin);
	VectorMA (gib->s.origin, (gib->mins[2] + gib->maxs[2]) * 0.5, up, gib->s.origin);


	// do client-side gibs in deathmatch
	{
		gib->s.origin[2] += 10;

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_GIBS);
		gi.WritePosition (gib->s.origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte ( 2 );	// number of gibs
		gi.WriteByte ( 0 );	// scale of direction to add to velocity
		gi.WriteByte ( 0 );	// random offset scale
		gi.WriteByte ( 10 );	// random velocity scale
		gi.multicast (self->s.origin, MULTICAST_PVS);

		G_FreeEdict( gib );
	}
}

// Rafael: this routine will need special handling if the jibed part is a helmet
void SpawnGibShotOff (edict_t *self, int mdx_part, int mdx_subobject, vec3_t dir)
{
#define	GIBS_HEAD	4
#define	GIBS_BODY	8
#define	GIBS_ARMS	6
#define	GIBS_LEGS	8

	int i;
	int numgibs;

	if (self->gender != GENDER_MALE && self->gender != GENDER_FEMALE)
		return;

//gi.dprintf( "shot ent: %i, part %i, object %i\n", (int)(self-g_edicts), mdx_part, mdx_subobject );

	// ok the head got shot off so spawn in the standin
	if (mdx_part == PART_HEAD)
	{
		numgibs = GIBS_HEAD;

		if (deathmatch_value)
			numgibs /= 2;

		self->s.model_parts[mdx_part].invisible_objects = (1<<0 | 1<<1);
		for (i=0; i<numgibs; i++)
			SpawnGib (self, mdx_part, mdx_subobject, dir);

		// Ridah, if the head goes, the hat and cigar must also go
		if (self->s.model_parts[PART_CIGAR].modelindex)
		{
			self->s.model_parts[PART_CIGAR].invisible_objects = (1<<0 | 1<<1);
			SpawnPartShotOff (self, PART_CIGAR, dir);
		}

		if (self->s.model_parts[PART_HAT].modelindex)
		{
			self->s.model_parts[PART_HAT].invisible_objects = (1<<0 | 1<<1);
			SpawnPartShotOff (self, PART_HAT, dir);
		}

		// hat and cigar bug
		self->count = 0;

	}
	else if (mdx_part == PART_LEGS || mdx_part == PART_BODY)
	{
		// sub object 0 cant be blown off
		if (mdx_subobject == 0)
		{
			if (	self->s.model_parts[PART_BODY].invisible_objects & ((1<<1) | (1<<2) | (1<<3))
				&&  self->s.model_parts[PART_BODY].invisible_objects & ((1<<4) | (1<<5) | (1<<6))
				&&	self->s.model_parts[PART_LEGS].invisible_objects & ((1<<1) | (1<<2) | (1<<3))
				&&  self->s.model_parts[PART_LEGS].invisible_objects & ((1<<4) | (1<<5) | (1<<6)) )
			{
				numgibs = GIBS_BODY;

				if (deathmatch_value)
					numgibs /= 2;

				for (i=0; i<numgibs; i++)
					SpawnGib (self, mdx_part, mdx_subobject, dir);

				// Ridah, couldn't free it since it has further processing in T_DamageMDX()
				self->nextthink = -1;
				self->svflags |= SVF_NOCLIENT;
				self->solid = SOLID_NOT;
				gi.linkentity( self );

//				self->s.model_parts[PART_BODY].invisible_objects |= (1<<0 | 1<<1);
//				self->s.model_parts[PART_LEGS].invisible_objects |= (1<<0 | 1<<1);
//				self->s.model_parts[PART_HEAD].invisible_objects |= (1<<0 | 1<<1);
				//self->s.model_parts[PART_HEAD].invisible_objects |= (1<<0 | 1<<1);
				//self->s.model_parts[PART_HEAD].invisible_objects |= (1<<0 | 1<<1);
			}
			else
			{
				return;
			}
		}
		// the left side
		else if (mdx_subobject <= 3)
		{
			self->s.model_parts[mdx_part].invisible_objects |= ((1<<1) | (1<<2) | (1<<3));

			if (mdx_part == PART_LEGS)
				numgibs = GIBS_LEGS;
			else
				numgibs = GIBS_ARMS;


			if (deathmatch_value)
				numgibs /= 2;

			// Ridah, increased this, we really need lots of gibs, and since they disappear pretty quick, and
			// the pody parts are harder to gib now, they shouldn't occur so often
			for (i=0; i<numgibs; i++)
				SpawnGib (self, mdx_part, 1 + i%3/*mdx_subobject*/, dir);	// Ridah, spawn gibs at each subobject

			// set health back up so next gib takes more shots
			self->health = 0;
		}
		// the right side
		else if (mdx_subobject <= 6) 
		{
			self->s.model_parts[mdx_part].invisible_objects |= ((1<<4) | (1<<5) | (1<<6));

			if (mdx_part == PART_LEGS)
				numgibs = GIBS_LEGS;
			else
				numgibs = GIBS_ARMS;

			if (deathmatch_value)
				numgibs /= 2;

			for (i=0; i<numgibs; i++)
				SpawnGib (self, mdx_part, 4 + i%3/*mdx_subobject*/, dir);	// Ridah, spawn gibs at each subobject

			// set health back up so next gib takes more shots
			self->health = 0;
		}
	}

}

void Think_Shard (edict_t *ent)
{
	if (!ent->misstime--)
	{	
		ent->nextthink = 0;
		G_FreeEdict(ent);
	}
	else
	{
		if (ent->misstime <= 15)
		{
			if (ent->misstime == 15)
			{
				ent->s.renderfx2 |= RF2_PASSALPHA;
				ent->s.effects = 1;		// this is full alpha now
			}
			
			ent->s.effects += (255/15);
		
			if (ent->s.effects > 255)
				ent->s.effects = 255;
		}

		ent->nextthink = level.time + 0.1;
	}
}

void shard_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point, int mdx_part, int mdx_subobject)
{
	G_FreeEdict (self);
}


void ThrowShard (edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t	*chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
	v[0] = speed * crandom();
	v[1] = speed * crandom();
	v[2] = speed + 16 * crandom();
	VectorMA (self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*128;
	chunk->avelocity[1] = random()*128;
	chunk->avelocity[2] = random()*128;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = shard_die;
	chunk->think = Think_Shard;
	chunk->misstime = 20;
	chunk->nextthink = level.time + 0.1;
    chunk->s.renderfx2 |= RF2_NOSHADOW;
	
	gi.linkentity (chunk);
}


// RAFAEL
void T_DamageMDX (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, 
				  vec3_t normal, int damage, int knockback, int dflags, int mod,
				  int mdx_part, int mdx_subobject)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			te_sparks;
	qboolean	hasarmor = false;

	hit = 0;

	if (!targ->takedamage)
		return;

	if (targ->cast_info.aiflags & AI_IMMORTAL)
	{
		return;
	}

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (!(dflags & DAMAGE_NO_PROTECTION) && (targ != attacker) && ((deathmatch_value && (teamplay->value || ((int)(dmflags->value) & (DF_MODELTEAMS/*| DF_SKINTEAMS*/))) /*|| coop->value*/)))
	{
		if ((OnSameTeam (targ, attacker)) && (!targ->client->pers.friendly_vulnerable))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
			{
				damage = 0;
			}
			else
				mod |= MOD_FRIENDLY_FIRE;
		} else if (attacker->client && !targ->deadflag)
			hit = 1;
	} else if (attacker->client && !targ->deadflag && targ != attacker)
		hit = 1;
	meansOfDeath = mod;

	if (dm_realmode->value && dm_realmode->value != 2 && attacker != targ && attacker->client)
	{
		damage *= 4;
	}

	client = targ->client;

	// JOSEPH 4-MAY-99
	if (mod == MOD_BLACKJACK)
		te_sparks = TE_IMPACT_CONCUSSION;	
	else if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;
	// END JOSEPH

	VectorNormalize(dir);

// bonus damage for suprising a monster
//	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
//		damage *= 2;

	if ((targ->flags & FL_NO_KNOCKBACK) || (targ->nokickbackflag))
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			// JOSEPH 13-MAY-99
			if ((targ->client  && attacker == targ) && deathmatch_value)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else if (targ->client  && attacker == targ)
				VectorScale (dir, 128.0 * (float)knockback / mass, kvel);
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);
			// END JOSEPH
			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (targ, te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			// JOSEPH 29-MAR-99
			//gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			// END JOSEPH
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}

	// JOSEPH 1-APR-99
	//treat cheat/powerup savings the same as armor
	asave = 0;
	asave += save;
	// END JOSEPH

	// team damage avoidance
	//if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
	//	return;

// do the damage
	if (take)
	{
		
		// JOSEPH 2-APR-99
		if (!deathmatch_value || dm_locational_damage->value)
		{
			int     index;
			gitem_t *item = NULL;
			float   takefactor; 
			int takehealth = 0;
			int takeshield = 0;

			if (mdx_part == PART_HEAD)
			{
				if (client)
				{
					item = FindItem ("Helmet Armor");
					takefactor = 0.3;
					if (!(client->pers.inventory[ITEM_INDEX(item)]))
					{
						item = FindItem ("Helmet Armor Heavy");
						takefactor = 0.1;						
					}
				
					takeshield = (take * (1.0 - takefactor));
					take *= 1.5;
					takehealth = take * takefactor;
				}
				else
				{
					take *= 1.5;			// Ridah, scaled this down, since it gives people with low pings an un-fair advantage
				}	
			}
			else if (mdx_part == PART_LEGS)
			{
				if (client)
				{
					item = FindItem ("Legs Armor");
					takefactor = 0.3;
					if (!(client->pers.inventory[ITEM_INDEX(item)]))
					{
						item = FindItem ("Legs Armor Heavy");
						takefactor = 0.1;						
					}
				    
					takeshield = (take * (1.0 - takefactor));
					takehealth = take * takefactor;
				}	
			}
			else if (mdx_part == PART_BODY)
			{
				if (client)
				{
					item = FindItem ("Jacket Armor");
					takefactor = 0.3;
					if (!(client->pers.inventory[ITEM_INDEX(item)]))
					{
						item = FindItem ("Jacket Armor Heavy");
						takefactor = 0.1;						
					}

				
					takeshield = (take * (1.0 - takefactor));
					take *= 1.5;
					takehealth = take * takefactor;				
				}
				else
				{
					take *= 1.5;
				}
			}
			else if (mdx_part == PART_CIGAR || mdx_part == PART_HAT)
			{
				// we hit a hat or something
				take = 0;
				targ->s.model_parts[mdx_part].modelindex = 0;

				// need to spawn the piece flying off
				{
					SpawnPartShotOff (targ, mdx_part, dir);
				}
			}
						
			if ((client) && (item) && (client->pers.inventory[ITEM_INDEX(item)]))
			{
				index = ITEM_INDEX (item);
				
				hasarmor = true;
						
				if (takeshield <= client->pers.inventory[ index ])
				{
					client->pers.inventory[ index ] -= takeshield;
					take = takehealth;
				}
				else
				{
					takehealth = ((take - client->pers.inventory[ index ])+
								  (takefactor*client->pers.inventory[ index ])); 
					client->pers.inventory[ index ] = 0;
					take = takehealth;
				}
			}
		}
		// END JOSEPH

		if (mod == MOD_FLAMETHROWER)
		{
		}
		else
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
				if (hasarmor)
				{

					// Ridah, this is a major bandwidth hog, need to make client-side, like gibs
					if (!deathmatch_value)
					{
						ThrowShard (targ, "models/props/glass/glass2.md2", 4, point);
						ThrowShard (targ, "models/props/glass/glass2.md2", 4, point);
						ThrowShard (targ, "models/props/glass/glass2.md2", 8, point);
						ThrowShard (targ, "models/props/glass/glass2.md2", 8, point);
						ThrowShard (targ, "models/props/glass/glass2.md2", 16, point);
						ThrowShard (targ, "models/props/glass/glass2.md2", 16, point);
					}

					{
						int rval;
						rval = rand();
						
						if (rval > 66)
							gi.sound(targ, CHAN_BULLET, gi.soundindex("weapons/bullethit_armor1.wav"), 1, ATTN_NORM, 0);
						else if (rval > 33)
							gi.sound(targ, CHAN_BULLET, gi.soundindex("weapons/bullethit_armor2.wav"), 1, ATTN_NORM, 0);
						else
							gi.sound(targ, CHAN_BULLET, gi.soundindex("weapons/bullethit_armor3.wav"), 1, ATTN_NORM, 0);
					}

				}
				else
					SpawnDamage (targ, TE_BLOOD, point, normal, take);
			else
				SpawnDamage (targ, te_sparks, point, normal, take);
		}


		targ->health = targ->health - take;

		// gi.dprintf ("health %d\n", targ->health);

		if ((mod != MOD_BLACKJACK) && (mod != MOD_CROWBAR))
		{
			if (targ->health <= targ->gib_health && targ->gender != GENDER_NONE && mdx_part <= PART_BODY)
			{
				if (!deathmatch_value)
				{
					SpawnGibShotOff (targ, mdx_part, mdx_subobject, dir);
				}
				else	// in deathmatch, take some beating before a limb gibs
				{
					if (((int)targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] + take) < 255)
						targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] += take;
					else
						targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] = 255;

					if (targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] > 150)
					{
						SpawnGibShotOff (targ, mdx_part, mdx_subobject, dir);
					}
				}
			}
			
			else if (!targ->deadflag)
			{
				if (((int)targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] + take) < 255)
					targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] += take;
				else
					targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] = 255;

				// tweek to help them stay alive a bit longer
				if (targ->s.model_parts[mdx_part].hitpoints[mdx_subobject] > ((rand()%150)+25) && targ->health < 50)
				{
					targ->health -= targ->max_health;	// make sure they die
					SpawnGibShotOff (targ, mdx_part, mdx_subobject, dir);
				}
			}
			
		}

		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
			{
				targ->flags |= FL_NO_KNOCKBACK;
			}

			// JOSEPH 3-MAR-99
			if (targ->svflags & SVF_MONSTER)
			{
				// targ->solid = SOLID_NOT;			
				targ->svflags |= SVF_DEADMONSTER;
				
				if (mod == MOD_ROCKET)
				{
					targ->s.renderfx2 &= ~RF2_DIR_LIGHTS;
				}
			}
			// END JOSEPH
			
// Ridah, removed so grenades and rockets can gib bodies
//			if (!targ->deadflag)
				Killed (targ, inflictor, attacker, take, point, mdx_part, mdx_subobject);

			return;
		}
	}

	if (	(targ->svflags & SVF_MONSTER)
		&&	(targ->health > 0))		// Ridah, 31-may-99, possibly fixes Whore crouching death bug
	{
		if (take)
		{
			targ->pain (targ, attacker, knockback, take, mdx_part, mdx_subobject);
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take, mdx_part, mdx_subobject);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take, mdx_part, mdx_subobject);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_armor += asave;
		if (mod == MOD_FLAMETHROWER)
			client->damage_flame += take*4;
		else
			client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;
//	int		mdx_part, mdx_subobject;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);

//		points = damage - 0.5 * VectorLength (v);
		points = damage * (1.0 - VectorLength (v)/radius);		// Ridah, changed this

		if (ent == attacker)
		{
			if (mod == MOD_FLAMETHROWER)	// don't damage self from flames, only if really close to wall, in which case we do a T_Damage()
				continue;

			// points = points * 0.5;
		}

		// JOSEPH 26-MAY-99
		if ((ent->svflags & SVF_MONSTER) && inflictor->classname &&
			(!strcmp(inflictor->classname, "grenade")) &&
			 attacker && (attacker->svflags & SVF_MONSTER))
		{
			points *= 0.5;
		}
		// END JOSEPH
		
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);

				if (mod == MOD_FLAMETHROWER && ent->health > 0 && (ent->svflags & SVF_MONSTER || ent->client))
				{

					// JOSEPH 29-MAY-99
					if (ent->onfiretime < -20)
					{
						ent->onfiretime = 1;

						if (ent->svflags & SVF_MONSTER)
							ent->cast_info.aiflags &= ~AI_NO_TALK;
						
						if (ent->gender == GENDER_FEMALE)
						{
/*							if (ent->health > 80)
							{
								Voice_Specific (ent, attacker, female_specific, 8);
							}
							else if (ent->health > 40)
							{
								Voice_Specific (ent, attacker, female_specific, 7);
							}
							else
							{
								Voice_Specific (ent, attacker, female_specific, 6);
							}*/
							Voice_Specific (ent, attacker, female_specific, 8);
						}
						else if (ent->gender == GENDER_MALE)
						{
/*							if (ent->health > 80)
							{
								Voice_Specific (ent, attacker, male_specific, 12);
							}
							else if (ent->health > 40)
							{
								Voice_Specific (ent, attacker, male_specific, 11);
							}
							else
							{
								Voice_Specific (ent, attacker, male_specific, 10);
							}*/
							Voice_Specific (ent, attacker, male_specific, 12);
						}
						
						if (ent->svflags & SVF_MONSTER)
							ent->cast_info.aiflags |= AI_NO_TALK;
						{	// play the sound on different channels so it's less likely to get over-written
							if (ent->last_talk_time == level.time && ent->last_voice->soundindex)
							{
								gi.sound( ent, CHAN_SPECIAL | CHAN_RELIABLE, ent->last_voice->soundindex-1, 1.0, 2, 0 );
//								gi.sound( ent, CHAN_BODY | CHAN_RELIABLE, ent->last_voice->soundindex-1, 1.0, 2, 0 );
							}
						}

						ent->pain_debounce_time = level.time + 5;

						if (ent->svflags & SVF_MONSTER)
							ent->cast_info.aiflags |= AI_NO_TALK;	// can't talk anymore, burning
					}

					if (ent->onfiretime > 0)
					{
						if (attacker->client || attacker->svflags & SVF_MONSTER)
							ent->onfireent = attacker;
						else
							ent->onfireent = NULL;

						if (deathmatch_value && (ent->pain_debounce_time < (level.time+3)))
						{
							if (ent->gender == GENDER_FEMALE)
							{
/*								if (ent->health > 80)
								{
									Voice_Specific (ent, attacker, female_specific, 8);
								}
								else if (ent->health > 40)
								{
									Voice_Specific (ent, attacker, female_specific, 7);
								}
								else
								{
									Voice_Specific (ent, attacker, female_specific, 6);
								}							*/
								Voice_Specific (ent, attacker, female_specific, 8);
							}
							else if (ent->gender == GENDER_MALE)
							{
/*								if (ent->health > 80)
								{
									Voice_Specific (ent, attacker, male_specific, 12);
								}
								else if (ent->health > 40)
								{
									Voice_Specific (ent, attacker, male_specific, 11);
								}
								else
								{
									Voice_Specific (ent, attacker, male_specific, 10);
								}					*/
								Voice_Specific (ent, attacker, male_specific, 12);
							}
	
							if (deathmatch_value)
							{	// play the sound on different channels so it's less likely to get over-written
								if (ent->last_talk_time == level.time && ent->last_voice->soundindex)
								{
									gi.sound( ent, CHAN_SPECIAL | CHAN_RELIABLE, ent->last_voice->soundindex-1, 1.0, 2, 0 );
//									gi.sound( ent, CHAN_ITEM | CHAN_RELIABLE, ent->last_voice->soundindex-1, 1.0, 2, 0 );
//									gi.sound( ent, CHAN_BODY | CHAN_RELIABLE, ent->last_voice->soundindex-1, 1.0, 2, 0 );
								}
							}

							ent->pain_debounce_time = level.time + 5;
						}
						// END JOSEPH

						ent->onfiretime = 50;

						if (ent->client)
						{
							ent->onfiretime = 25;

							// Ridah, in Deathmatch, you should be rewarded for flaming them, not so much just setting them on fire and waiting for them to burn to death
							if (deathmatch_value)
							{
								ent->onfiretime = 10;
								T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
							}
						}

						if (ent->cast_info.catch_fire)
						{
							ent->cast_info.catch_fire( ent, attacker );
						}

						if (!ent->client)
							ent->s.renderfx2 &= ~RF2_DIR_LIGHTS;
					}
					else
					{
						ent->onfiretime -= (int) (3.0 * (1.0 - VectorLength (v)/radius) * (float)((deathmatch_value!=0)*2 + 1));
					}
				}
				else	// do the damage as normal
				{
					T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
				}

			}
		}
	}
}

// JOSEPH 21-APR-99
/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage_Fire (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;
	int     mod = MOD_FLAMETHROWER;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);

		points = damage * (1.0 - VectorLength (v)/radius);
		//points = damage;
		
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);

				if (ent->health > 0 && (ent->svflags & SVF_MONSTER || ent->client))
				{
					T_Damage (ent, inflictor, attacker, vec3_origin, ent->s.origin, vec3_origin, points, points, 0, MOD_FLAMETHROWER);
				}
			}
		}
	}
}
// END JOSEPH

edict_t  *SpawnTheWeapon (edict_t *self, char *var_itemname)
{
	edict_t *drop = NULL;
	int		model;
	qboolean e_weapon = false;
	char		itemname[MAX_QPATH];

	strcpy( itemname, var_itemname );

	if (!(strcmp (itemname, "weapon_pistol_e")))
	{
		strcpy (itemname, "weapon_pistol");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_pistol/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_shotgun_e")))
	{
		strcpy (itemname, "weapon_shotgun");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_shotgun/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_heavymachinegun_e")))
	{
		strcpy (itemname, "weapon_heavymachinegun");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_hmg/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_bazooka_e")))
	{
		strcpy (itemname, "weapon_bazooka");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_rocket_launcher/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_flamethrower_e")))
	{
		strcpy (itemname, "weapon_flamethrower");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_flamegun/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_grenadelauncher_e")))
	{
		strcpy (itemname, "weapon_grenadelauncher");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_grenade_launcher/tris.md2");
	}
	else if (!(strcmp (itemname, "weapon_tommygun_e")))
	{
		strcpy (itemname, "weapon_tommygun");
		e_weapon = true;
		model = gi.modelindex("models/weapons/e_tomgun/tris.md2");
	}
	
	drop = Drop_Item (self, FindItemByClassname (itemname));

	if (e_weapon)
		drop->s.modelindex = model;
	
	if (drop)
		drop->spawnflags &= ~DROPPED_ITEM;

	return drop;
}
