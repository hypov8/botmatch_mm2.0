
#include "g_local.h"

#include "voice_punk.h"
#include "voice_bitch.h"

float	last_client_talk;		// records the last time the client was spoken to, so AI characters don't all respond at once

void Voice_Random( edict_t *self, edict_t *other, voice_table_t *voice_table, int num_entries )
{
	int entry=-1, best_entry=-1, count=0;
	int	i;

	if (self->cast_info.aiflags & AI_NO_TALK)
		return;

	if (self->health <= 0)
		return;

again:

	if (	!deathmatch_value
		&&	(other)
		&&	(other->last_talk_time > (level.time - 5))
		&&	(other->last_voice)
		&&	(other->last_voice->response_table == voice_table)
		&&	(other->last_voice->num_responses))
	{
		// we should pick a specific response
		i = (int)floor( random()*other->last_voice->num_responses );

		while ((entry < 0) || (voice_table[entry].last_played > 0.1))
		{
			entry = other->last_voice->responses[ i++ ];

			if ((best_entry < 0) || (voice_table[entry].last_played < voice_table[best_entry].last_played))
			{
				best_entry = entry;
			}
			else if (voice_table[ entry ].last_played > level.time)
			{	// still set from a previous level
				voice_table[ entry ].last_played = 0;
			}

			if (i >= other->last_voice->num_responses)
				i = 0;

			if (count++ > other->last_voice->num_responses)
				break;
		}

		entry = best_entry;

	}
	else	// not responding, just pick any non-response
	{

		entry = (int)floor( random()*num_entries );

		while (count++ < num_entries)
		{
			if (	(!voice_table[ entry ].response)
				&&	(self->last_voice != &voice_table[ entry ])
				&&	(	(best_entry < 0)
					 ||	(voice_table[ entry ].last_played < voice_table[ best_entry ].last_played)))
			{
				best_entry = entry;
			}
			else if (voice_table[ entry ].last_played > level.time)
			{	// still set from a previous level
				voice_table[ entry ].last_played = 0;
			}

			entry++;

			if (entry >= num_entries)
				entry = 0;
		}

		if (best_entry < 0)
			best_entry = (int)floor( random()*num_entries );

		entry = best_entry;
	}

	// HACK, don't say "Fuck me, Freddy is it you?" to the bitch (we need a better way to handle things like this)
	if (	self->client
		&&	other
		&&	other->gender == GENDER_FEMALE
		&&	voice_table == neutral_talk_player
		&&	entry == 4)
	{
		voice_table[ entry ].last_played = level.time;
		entry=-1;
		best_entry=-1;
		count=0;
		goto again;
	}

	if (voice_table[ entry ].filename)
	{
		if (!voice_table[ entry ].last_played || (voice_table[ entry ].last_played > level.time)
			|| (voice_table[ entry ].gameinc_soundindex != gameinc))
		{
			voice_table[ entry ].soundindex = 0;
		}

		if (deathmatch_value || VectorDistance( g_edicts[1].s.origin, self->s.origin ) < 1024)
		{
			if ((other && other->client) || self->client)
				gi.sound( self, CHAN_VOICE | CHAN_RELIABLE, (voice_table[ entry ].soundindex ? voice_table[ entry ].soundindex - 1 : (voice_table[ entry ].soundindex = 1 + gi.soundindex ( voice_table[ entry ].filename )) - 1 ), 1.0, 1, 0 );
			else
				gi.sound( self, CHAN_VOICE | CHAN_RELIABLE, (voice_table[ entry ].soundindex ? voice_table[ entry ].soundindex - 1 : (voice_table[ entry ].soundindex = 1 + gi.soundindex ( voice_table[ entry ].filename )) - 1 ), 0.2, 1, 0 );

			voice_table[ entry ].gameinc_soundindex = gameinc;
		}
	}
	else
	{
		gi.dprintf("VOICE TODO: %s\n", voice_table[ entry ].text);
	}

	if (self->client || (other && other->client))
	{
		if (other && other->client)	// tell this client to talk back to us next time they chat
			other->cast_info.talk_ent = self;

		self->cast_info.talk_ent = other;

		last_client_talk = level.time;
	}

	self->last_voice = &( voice_table[ entry ] );
	voice_table[ entry ].last_played = level.time;

	self->last_talk_time = level.time;

// JOSEPH 2-FEB-99 
	if (other && other->client)
	{
		other->client->ps.stats[STAT_HUD_ENEMY_TALK] = voice_table[ entry ].type;
		other->client->hud_enemy_talk_time = level.time + 2.0;
	}
	// END JOSEPH

}

void Voice_Specific( edict_t *self, edict_t *other, voice_table_t *voice_table, int entry )
{

	if (self->cast_info.aiflags & AI_NO_TALK)
		return;

	if (self->health <= 0)
		return;

	// JOSEPH 2-FEB-99 
	if (other && other->client)
	{
		other->client->ps.stats[STAT_HUD_ENEMY_TALK] = voice_table[ entry ].type;
		other->client->hud_enemy_talk_time = level.time + 2.0;
	}
	// END JOSEPH
	
	if (voice_table[ entry ].filename)
	{
		if (!voice_table[ entry ].last_played || (voice_table[ entry ].last_played > level.time)
			|| (voice_table[ entry ].gameinc_soundindex != gameinc))
		{
			voice_table[ entry ].soundindex = 0;
		}

		if (VectorDistance( g_edicts[1].s.origin, self->s.origin ) < 1024)
		{
			if ((other && other->client) || self->client)
				gi.sound( self, CHAN_VOICE + CHAN_RELIABLE, (voice_table[ entry ].soundindex ? voice_table[ entry ].soundindex - 1 : (voice_table[ entry ].soundindex = 1 + gi.soundindex ( voice_table[ entry ].filename )) - 1 ), 1.0, 1, 0 );
			else
				gi.sound( self, CHAN_VOICE + CHAN_RELIABLE, (voice_table[ entry ].soundindex ? voice_table[ entry ].soundindex - 1 : (voice_table[ entry ].soundindex = 1 + gi.soundindex ( voice_table[ entry ].filename )) - 1 ), 0.2, 1, 0 );

			voice_table[ entry ].gameinc_soundindex = gameinc;
		}
	}
	else
	{
		gi.dprintf("VOICE: %s\n", voice_table[ entry ].text);
	}

	if (self->client || (other && other->client))
	{
		if (other && other->client)	// tell this client to talk back to us next time they chat
			other->cast_info.talk_ent = self;

		last_client_talk = level.time;
	}

	self->last_voice = &( voice_table[ entry ] );
	voice_table[ entry ].last_played = level.time;

	self->last_talk_time = level.time;
}

// JOSEPH 13-FEB-99
void Voice_Player_Specific( edict_t *player, int entry )
{

	if (player->health <= 0)
		return;

	// JOSEPH 2-FEB-99 
	if (player && player->client)
	{
		player->client->ps.stats[STAT_HUD_SELF_TALK] = entry;
		player->client->hud_self_talk_time = level.time + 2.0;
	}
	// END JOSEPH

	last_client_talk = level.time;
	player->last_talk_time = level.time;
}
// END JOSEPH
