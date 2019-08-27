
// voice_bitch.h

// Conversation:
// The neutrals would say to you
// 20
#define F_NUM_NEUTRAL_TALK		16
extern voice_table_t f_neutral_talk[];

#define	F_NUM_NEUTRAL_ASSHOLE_TALK	8
extern voice_table_t f_neutral_asshole_talk[];

// Player talk to a neutral

// 10
#define F_NUM_NEUTRAL_TALK_PLAYER		3
extern voice_table_t f_neutral_talk_player[];

// Neutrals converse among themselves

// 20
#define F_NUM_NEUTRAL_CONVERSE	16
extern voice_table_t f_neutral_converse[];

// profanity
// 20
#define F_NUM_PROFANITY_LEVEL1	5
extern voice_table_t f_profanity_level1[];
//20
#define F_NUM_PROFANITY_LEVEL2	(4+39)
extern voice_table_t f_profanity_level2[];

//3
#define F_NUM_PROFANITY_LEVEL3	3
extern voice_table_t f_profanity_level3[];

//===================================================================================

#define F_NUM_FOLLOWME		5
extern voice_table_t f_followme[];

#define F_NUM_MOVEOUT			4
extern voice_table_t f_moveout[];

#define F_NUM_HOLDPOSITION	6
extern voice_table_t f_holdposition[];

//===================================================================================

#define F_NUM_FIGHTING	(8+1)
extern voice_table_t f_fightsounds[];

#define F_NUM_SPECIFIC	4
extern voice_table_t f_specific[];

#define F_NUM_BACKOFF	4
extern voice_table_t f_backoff[];

#define F_NUM_LISA_SPECIFIC 8
extern voice_table_t lisa_specific[];

#define F_NUM_BETH_SPECIFIC 8
extern voice_table_t beth_specific[];

// JOSEPH 29-MAY-99
#define F_NUM_FEMALE_SPECIFIC 9
extern voice_table_t female_specific[];
// END JOSEPH

// JOSEPH 19-MAR-99
#define F_NUM_MONA_SPECIFIC 20
extern voice_table_t mona_specific[];
// END JOSEPH

// JOSEPH 12-MAR-99
#define F_NUM_GRUNTING 5
extern voice_table_t f_grunting[];
// END JOSEPH

#define F_NUM_YOLANDA_SPECIFIC	26
extern voice_table_t yolanda_specific[];


#define F_NUM_HIREDGAL_SPECIFIC	11
extern voice_table_t hiredgal_specific[];

#define F_NUM_HIREDGAL_ASK	9
extern voice_table_t hiredgal_ask[];

extern voice_table_t sy_selma[];
extern voice_table_t sy_jane[];

//TiCaL
#define F_NUM_YOLANDA_RANDOM 7
extern voice_table_t yolanda_random[];

#define F_NUM_MONA_RANDOM 11
extern voice_table_t mona_random[];

#define F_NUM_LOLA_RANDOM 7
extern voice_table_t lola_random[];

#define F_NUM_BLUNT_RANDOM 17
extern voice_table_t blunt_random[];

#define F_NUM_BETH_RANDOM 3
extern voice_table_t beth_random[];

#define F_NUM_BAMBI_RANDOM 3
extern voice_table_t bambi_random[];
//tical
/////////////////////////////////////////////////
// radio city voice files
/////////////////////////////////////////////////

extern voice_table_t rc_f_neutral_talk[];
extern voice_table_t rc_f_neutral_asshole_talk[];
extern voice_table_t rc_f_neutral_talk_player[];
extern voice_table_t rc_f_neutral_converse[];

extern voice_table_t rc_f_profanity_level1[];
extern voice_table_t rc_f_profanity_level2[];
extern voice_table_t rc_f_profanity_level3[];

extern voice_table_t rc_f_fightsounds[];
extern voice_table_t rc_f_specific[];
extern voice_table_t rc_f_backoff[];
extern voice_table_t rc_female_specific[];
extern voice_table_t rc_f_grunting[];
extern voice_table_t rc_lola[];

extern voice_table_t blunt[];
