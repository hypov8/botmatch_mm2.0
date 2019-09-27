
#include "g_local.h"
#include "g_hitmen.h"

/**************************************************************************
 Original idea by Mike Fox aka 

	24-25/12/99		Started Hitmen. all wepaons removed from the game as
					well as ammo and health so we have total control over
					what weapons people have so we can stop those weapon
					camping bitches. Scoreboard also displaying the weapon
					timer now.

	26-27/12/99		Added all the weapons into an array so its easier to set
					the next weapon by using array indexing. After countless
					bugs due to adding too much at once finally got the damn
					thing working. Next to sort out will be the ammo adding/
					setting up on a weapon change so the values are correct. 
					Then I'll sort out the timer's for health and ammo going
					up.

	28/12/99		Ammo added with correct values. Ammo was still being left
					over from b4 so had to add extra flag in add_ammo to set
					the clip value to 0 b4 we add the new ammo. Bug with the
					random weapon function will sort it tomorrow.

	29/12/99		Sorted the bug, the array of used weapons wasn't big
					enough. Random weapon is working okay and no weapons get
					selected again until all have been used. Starting weapon
					is now selected randomly and should also apply to people
					entering the game half way through.

	30/12/99		Added the incrementing of ammo. This is also setup so
					different weapons have different increments as well as
					different delays before its added, this way the weapons
					have to be used more carefully. Added extra code so that
					if random weapons aren't used we will start with the
					pistol.

	31/12/99		Health incrementing now. Sound fx added when we are at
					3 seconds for the weapon change. Also added sound for
					health increase and weapon change. Drop weapon fixed so
					that we never leave anything behind. Removed all pistol
					mods so that we can control what happens with the pistol
					as the damage it does is going to be well increased.

	03/01/00		Did some sound testing and added sequential weapons
					with leaving some out. Person who's died can now see 
					what the health of his attacker was. Ini file working
					without any problems. Random weapons from user choice
					working a treat. Added check so that if only 1 weapon
					then it won't bother switching.

	15/01/00		First bug found after server setup. Weapons weren't
					switching after a level change.

	18/01/00		Added new MOTD with a sexy font and a key bind to show
					it when you want. Also added a screen to show the
					current server config.

	20/01/00		Corrected game version number. Shortened time for spawn
					invunerability. Also shortened the MOTD time.

	06/02/00		Removed the SERVERINFO crap from all the Hook variables
					to possibly cure a server problem. Also shortened the
					MOTD a bit.

	22/03/00		Removed a stupid debug message and added STDLOG support.

	25/03/00		Added hook attached time so that people can't swing from
					the sky and walls for ever. Tested and working.

	26/03/00		Added extra check so that the hook won't fire when your
					dead. Changed the death message for the hook to "gutted
					by".

	27/03/00		Fixed GSlog so that kills by the Hook are logged.

	28/03/00		Updated GSlog so that kills from the Pistol, Crowbar,
					Flamethrower, HMG and Blackjack??? are logged as kills.

	29/03/00		Added all of the stats for the current player. Next to 
					add is the leaders stats on the same page so you can
					see if your doing shit or not. :-) Leaders stats also
					being shown now and if your the leader then they won't
					be.

	14/05/00		Finally got my act sorted and added the resume feature
					from the comp mod. I'll add a respect message for their
					code.

	07/06/00		Ooops I never set the last_weapon variable after getting
					the next one. This is why we got the same weapon sometimes.

	19/06/00		Botmen v1.0 release.

***************************************************************************/

void hm_ChangeWeapon( void );
void hm_ChangeClientWeapon( edict_t *ent, int oldweap, int newweap );
void hm_incrementammo(gclient_t* client);
void hm_incrementhealth(edict_t *ent, gclient_t* client);
void Hm_LoadMOTD( void );
void LoadHitmenWorldIni( void );
void VerifyIniFileValues( void );
void hm_ProcessWeaponsList( void );

static void Hm_set_ammo(gclient_t* cl, int count, int nWeap, qboolean ClearClip);
static void hm_next_weapon ( void );

extern void AutoLoadWeapon( gclient_t *client, gitem_t *weapon, gitem_t *ammo );
extern int QweryClipIndex (gitem_t *item);

//#define MAXHMWEAPONS	7	// Moved into G_locals.h
#define HMLISTITEMS		(MAXHMWEAPONS+2)

#define HM_ONESECOND	1.0

#define MOTD_lines		3

#define	CHAN_HLTH						 5 // seems to work fine (can go up to 7 for sound channels?)

#define hm_PISTOL_ROUNDS			50//10
#define hm_TOMMYGUN_ROUNDS			100//50
#define hm_SHOTGUN_ROUNDS			40//8
#define hm_BARMACHINEGUN_ROUNDS 	60//30
#define hm_GRENADELAUNCHER_ROUNDS	9//3
#define hm_ROCKETLAUNCHER_ROUNDS	15//5
#define	hm_FLAMEGUN_ROUNDS			100//50

#define hm_PISTOL					1
#define hm_SHOTGUN					2
#define hm_TOMMYGUN					4
#define hm_BARMACHINEGUN		 	8
#define hm_GRENADELAUNCHER			16
#define hm_ROCKETLAUNCHER			32
#define	hm_FLAMEGUN					64

typedef struct Hitmenitem_s
{
	char		*szName;                    // weapon name                    
    char		*szAmmo;                    // weapon ammo                    

    int         nAmmoInitial;               // amount of ammo on switch       
    int         nAmmoIncrement;             // amount of ammo given per second   
    int         nAmmoSecs;                  // secs to wait before adding ammo
    int         nAmmoMax;                   // max amount of ammo             

    int         nHmIndexWeapon;				// itemlist index for weapon (set in HitmenInit)
    int         nHmIndexAmmo;				// itemlist index for ammo (set in HitmenInit)

//    char*       szMessage;                  // switch message
} Hitmenitem_t;

/////////////////////////////////////////////////////////////////////////////

// Changed to differnet types see ini file load function.
//cvar_t	*HmRandomWeapon;
//cvar_t	*HmWeaponTime;
//cvar_t	*HmSoundWarn;

/////////////////////////////////////////////////////////////////////////////
// Note that the current weapon index (1-10) always corresponds to the 
// standard weapons in order (blaster through bfg10k). This is still the case 
// even if the user bans certain weapons (e.g. the bfg) and/or specifies a 
// specific order in which case the NIQ-determined current weapon is converted
// to the standard weapon index 1-10.

Hitmenitem_t Hitmenlist[HMLISTITEMS] = 
{
	{
		NULL
	},	// leave index 0 alone

	{//1
		"spistol",					// doesn't use any ammo!
        "bullets",                
        hm_PISTOL_ROUNDS,			// Initial ammo on weapon change
        15,							// Amount per seconds to give
        5,							// How many seconds before adding ammo
        200,						// Maximum amount of ammo full stop
        999,                     
        999
//        "switching to super magnum"        
	},

	{//2
		"shotgun",              
        "shells",               
        hm_SHOTGUN_ROUNDS,
        20,
        10,
        100,
        999,
        999
//      "switching to shotgun"        
	},

	{//3
		"tommygun",           
        "bullets",              
        hm_TOMMYGUN_ROUNDS,                     
        50,                     
        5,
        200,
        999,                     
        999
//        "switching to tommygun"        
	},

	{//4
		"Heavy machinegun",             
        "308cal",              
        hm_BARMACHINEGUN_ROUNDS,                     
        30,
        5,                     
        90,                    
        999,                     
        999
//        "switching to HMG"        
	},

	{//5
		"Grenade Launcher",     
        "grenades",             
        hm_GRENADELAUNCHER_ROUNDS,                     
        6,                     
        10,
        12,
        999,                     
        999
//        "switching to grenade launcher"        
	},

	{//6
		"Bazooka",      
        "rockets",              
        hm_ROCKETLAUNCHER_ROUNDS,                     
        10,                     
        5,
        25,                    
        999,                     
        999
//        "switching to bazooka"        
	},

	{//7
		"FlameThrower",         
        "Gas",                
        hm_FLAMEGUN_ROUNDS,                     
        100,                     
        5,
        200,                    
        999,                     
        999
 //       "switching to flamethrower"        
	},

	// end of list marker
	{NULL}
};

//*************************************************************************

// Variables and defines go here.
#define	HM_INI_FILE	"hitmen.ini"

int			HmRandomWeapon		= true; 	// Are the weapons changed randomly
int			HmSoundWarn			= true; 	// Do we want audible sound before weapon switch
int			HmHealthSound		= true;		// Do we want health inc sound 
int			HmKillForHealth		= false;	// Do we to kill to replenish our health
int			HmHookAvailable		= false;		// Is the hook available to use?
int			HmStdlogging		= false;		// Is the hook available to use?

int			HmWeaponTime		= 50;		// Number of seconds before weapon changes
int			HmWeaponsAvail		= 127;		// What weapons are available in the game

// 1 - Pistol  2 - Shotgun  4 - Tommy gun  8 - HMG
// 16 - Grenade launcher  32 - Bazooka  64 - Flamethrower

// 1+2+4+8+16+32+64 = 127 Subtract whichever to remove

//
// Hitmen Ini file options
//
typedef struct
{
	char	*ident;
	int		*variable;
	int		MinVariable;
	int		MaxVariable;
	int		DefaultVariable;
} INI_OPTION;

INI_OPTION	option[] = 

//	Noraml Hitmen Options

	{	{"randweap",		&HmRandomWeapon,	0,1,1},
		{"soundwarn",		&HmSoundWarn,		0,1,1},
		{"healthsnd",		&HmHealthSound,		0,1,1},
		{"killforhealth",	&HmKillForHealth,	0,1,0},
		{"hookavail",		&HmHookAvailable,	0,1,0}, //min, max, default //0,1,1
		{"stdlog",			&HmStdlogging,		0,1,1},

		{"weapontime",		&HmWeaponTime,		30,300,60},
		{"weapons",			&HmWeaponsAvail,	1,127,127}

	};

#define MAX_OPTIONS (sizeof(option)/sizeof(option[0]))

//*************************************************************************
#if 0 //HM HYPOV8 DUP
typedef struct   // Message of the Day
	{
	char textline[100];
	} MOTD_t;

	MOTD_t	MOTD[20];
#endif
//*************************************************************************

void hm_CheckWeaponTimer( void )
{
	int		i;
	edict_t	*ent;
	float	fTimeLeft;
	static qboolean bDidIntermission = false;
	static qboolean bReset_Timer = false;

	// If only 1 weapon don't bother doing anything.
	if (game.Num_Weapons == 1)
		return;

	if (!(level.modeset == MATCH || level.modeset == PUBLIC)){
		game.Weapon_Timer = level.time + game.Weapon_Timer_Reset;
		return; //add hypov8 tourney
	}

	//
	// If we are swapping levels then stop doing any weapon timer stuff
	if (level.intermissiontime)
    {
        if(!bDidIntermission)
		{
            // yes -- change it (no need to reset timer)
			hm_ChangeWeapon();

			bDidIntermission = true;
			bReset_Timer = true;
			return;
    	}
		return;
	}

    bDidIntermission = false;

	//
	// If we are in the game, but we've just started a new level then we need to
	// reset the weapon timer.
	if ((level.intermissiontime <= 0) && (bReset_Timer))
	{
		game.Weapon_Timer = level.time + game.Weapon_Timer_Reset;
		bReset_Timer = false;
	}

	if ( game.Weapon_Timer <= level.time )
	{
		game.Weapon_Timer = level.time + game.Weapon_Timer_Reset;
		hm_ChangeWeapon();
	}
	else if(HmSoundWarn)
    {
        fTimeLeft = game.Weapon_Timer - level.time;

		if ( fTimeLeft == 1.0 || fTimeLeft == 2.0 || fTimeLeft == 3.0 )
        {
			// warning sound before switching weapon

			// warn all clients that the weapon is about to change even if they are dead
    		for (i=1; i<=(int)maxclients->value; i++)
			{
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;

				if (!ent->client)
					continue;

    			gi.sound(ent, CHAN_ITEM, gi.soundindex("world/switches/thunk switch.wav"), 1, ATTN_NORM, 0);
			}
        }
    }

}

//***************************************************************************
//
//	Store the previous weapon so we know what to remove before giving the
//	player the next one. Then call the routine to do the swap.
//

void hm_ChangeWeapon( void )
	{
   	edict_t	*ent;
    int		i;
	int		HmOldweapon;

	HmOldweapon = game.Current_Weapon;

	hm_next_weapon();

	// get all current clients to change to new weapon
	for (i=1; i<=(int)maxclients->value; i++)
        {
		ent = g_edicts + i;

    	if (!ent->inuse)
	    	continue;

		if ((teamplay->value) && (ent->client->pers.team == 0))
			continue;

		//hypo include spectators
		if (ent->client->pers.spectator == SPECTATING)
			continue;

		hm_ChangeClientWeapon( ent, HmOldweapon, game.Current_Weapon );
    	}
	}

//***************************************************************************
//
//	This will do the actually swapping of the weapon by removing the current
//	one and allocating the next one.
//

void hm_ChangeClientWeapon( edict_t *ent, int HmOldweapon, int HmNewweapon )
	{
	gclient_t	*client;
	gitem_t		*item, *ammo;
	int			nNewAmmoAmount;

	client = ent->client;
	if (!client)
		return;

    // is he dead?
	if (ent->health < 1)
       return;

	// Remove the old weapon.
    client->pers.inventory[Hitmenlist[HmOldweapon].nHmIndexWeapon] = 0;

	// Remove old ammo.
    //if((nOldAmmoIndex != 999) && (nOldAmmoIndex != nNewAmmoIndex))
//		Hm_set_ammo (ent->client, 0, HmOldweapon);
	
	// Add the new weapon to the inventory.
	client->pers.inventory[Hitmenlist[HmNewweapon].nHmIndexWeapon] = 1;

	// Make the new weapon the current one.
	client->pers.selected_item = Hitmenlist[HmNewweapon].nHmIndexWeapon;
	client->newweapon = &itemlist[Hitmenlist[HmNewweapon].nHmIndexWeapon];

    // allocate new ammo:
	nNewAmmoAmount = Hitmenlist[HmNewweapon].nAmmoInitial;
   	Hm_set_ammo(ent->client, nNewAmmoAmount, HmNewweapon, true);

	// Set the index for this ammo type.
	ent->client->ammo_index = Hitmenlist[HmNewweapon].nHmIndexAmmo;

	// Load the weapon with the correct ammo.
	item = FindItem(Hitmenlist[HmNewweapon].szName);
	ammo = FindItem (item->ammo);
	AutoLoadWeapon( client, item, ammo );

	ChangeWeapon(ent);

	gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/w_pkup.wav"), 1, ATTN_NORM, 0);
	}

/////////////////////////////////////////////////////////////////////////////
// Hm_set_ammo:
//
static void Hm_set_ammo(gclient_t* cl, int count, int nWeap, qboolean ClearClip)
	{
	gitem_t		*weapon;
	int			clip_index;

	if (!cl) 
		return; 

	// long-time bug fix?: if count is 0, we are clearing the ammo which we always have to do
    // if user has cheated, don't reset ammo until back down to normal levels
//	if ((count != 0) && (cl->pers.inventory[Hitmenlist[nWeap].nListIndexAmmo] > Hitmenlist[nWeap].nAmmoMax))
//        return;

	if (count > Hitmenlist[nWeap].nAmmoMax)
		count = Hitmenlist[nWeap].nAmmoMax;

	// If the clearclip flag is set then we want to make sure the ammo added doesn't
	// include whats in the clip from the last time we had this weapon.
	if (ClearClip)
		{
		weapon = FindItem(Hitmenlist[nWeap].szName);
		clip_index = QweryClipIndex( weapon );
		cl->pers.weapon_clip[clip_index] = 0;
		}

    cl->pers.inventory[Hitmenlist[nWeap].nHmIndexAmmo] = count;

	}

/////////////////////////////////////////////////////////////////////////////
// Hm_add_ammo:
//
static void Hm_add_ammo(gclient_t* cl, int count, int nWeap)
	{
	if (!cl) 
		return; 

	// long-time bug fix?: if count is 0, we are clearing the ammo which we always have to do
    // if user has cheated, don't reset ammo until back down to normal levels
//	if ((count != 0) && (cl->pers.inventory[Hitmenlist[nWeap].nListIndexAmmo] > Hitmenlist[nWeap].nAmmoMax))
//        return;

	count += cl->pers.inventory[Hitmenlist[nWeap].nHmIndexAmmo];

	if (count > Hitmenlist[nWeap].nAmmoMax)
		count = Hitmenlist[nWeap].nAmmoMax;

    cl->pers.inventory[Hitmenlist[nWeap].nHmIndexAmmo] = count;

	}

/////////////////////////////////////////////////////////////////////////////
// Hm_next_weapon
//

void hm_next_weapon ( void )
	{
	int			i, NewWeapIdx;
	qboolean	AnyUnused=false;

	if (game.Num_Weapons == 1)
		{
			game.Current_Weapon = game.HmWeaponsAvailList[0].nWeaponNumber;
			return;
		}
	if (game.Num_Weapons == 2) //hypov8 fix for 2 weapons.
	{
		if (game.Current_Weapon == game.HmWeaponsAvailList[0].nWeaponNumber)
			game.Current_Weapon = game.HmWeaponsAvailList[1].nWeaponNumber;
		else
			game.Current_Weapon = game.HmWeaponsAvailList[0].nWeaponNumber;
		return;
	}

	// Get the next weapon in a random order.
	if (HmRandomWeapon)
		{
			for (i = 0; i < game.Num_Weapons; i++)	{
				if (game.HmWeaponsAvailList[i].nAlreadyUsed == false)	{
					AnyUnused = true;
					break; //add hypo. stop wasting time
				}
			}

		if(!AnyUnused)
			{
				// clear the used flag for all weapons then pick a new index at random
				for(i=0; i < game.Num_Weapons; i++)
					game.HmWeaponsAvailList[i].nAlreadyUsed = false;

				do
					{
						NewWeapIdx = (rand() % (game.Num_Weapons /*-1*/)); //hypov8 fix for rand, 3 = 0to2
					} while (game.HmWeaponsAvailList[NewWeapIdx].nWeaponNumber == game.Current_Weapon);
			}
		else
			{            
				// pick any weapon at random
				NewWeapIdx = (rand() % (game.Num_Weapons /*-1*/)); //hypov8 fix for rand, 3 = 0to2

				// scan until weapon is unused (we know there is at least one)
				while(game.HmWeaponsAvailList[NewWeapIdx].nAlreadyUsed == true)
					{
						NewWeapIdx++;
						if( NewWeapIdx > (game.Num_Weapons-1) )
							NewWeapIdx = 0;
					}
			}

		game.HmWeaponsAvailList[NewWeapIdx].nAlreadyUsed = true;
		}
	else	// otherwise just give us whats available next.
		{
		game.Weapon_Index++;

		if (game.Weapon_Index > (game.Num_Weapons-1) )
			game.Weapon_Index = 0;

		NewWeapIdx = game.Weapon_Index;
		}

	//game.Last_Weapon = game.Current_Weapon; //hypo disable
	game.Current_Weapon = game.HmWeaponsAvailList[NewWeapIdx].nWeaponNumber;
	}


/////////////////////////////////////////////////////////////////////////////
// hm_Initialise
//
//	This will setup all of the weapons index's so we can do without the
//	constant use of FindItem, it also sets the ammo ones too.
// 

void hm_Initialise( void )
	{
    int nWeap, i;
	int	nRndWeap=0;	// This must be set to 0 as its used for setting index 0 if no weapon is randomized

	// Load any extra text for the message of the day
	//Hm_LoadMOTD();

	// load the ini file with the required settings
	LoadHitmenWorldIni();

	// Check to see if the values we have are correct.
	VerifyIniFileValues();

    // Set the random number seed.
    srand( (unsigned)time( NULL ) );

    for(nWeap=1; nWeap<=MAXHMWEAPONS; nWeap++)
        {
        Hitmenlist[nWeap].nHmIndexWeapon = ITEM_INDEX(FindItem(Hitmenlist[nWeap].szName));
		Hitmenlist[nWeap].nHmIndexAmmo   = ITEM_INDEX(FindItem(Hitmenlist[nWeap].szAmmo));
        }

	// This will set the available weapons and how many
	hm_ProcessWeaponsList();

	// Clear the list of used weapons for getting the next available one
	for ( i=0; i < MAXHMWEAPONS; i++ )
		game.HmWeaponsAvailList[i].nAlreadyUsed = false;

	// If we are using random weapons then set the starting weapon random as well.
	if (HmRandomWeapon)
		if (game.Num_Weapons > 1)
			{
				nRndWeap = (rand() % (game.Num_Weapons /*-1*/)); //hypov8 fix for rand, 3 = 0to2
				nWeap = game.HmWeaponsAvailList[ nRndWeap ].nWeaponNumber;
			}
		else
			nWeap = game.HmWeaponsAvailList[0].nWeaponNumber;
	else
		nWeap = game.HmWeaponsAvailList[0].nWeaponNumber;

	game.HmWeaponsAvailList[nRndWeap].nAlreadyUsed = true;
	game.Current_Weapon = nWeap;
	game.Last_Weapon = nWeap;
	game.Weapon_Index = 0;	// only used for sequential weapon access.

	game.Weapon_Timer_Reset = HmWeaponTime;  //hypov8 inital timer for comp?
	if (teamplay->value)
		game.Weapon_Timer = level.time + game.Weapon_Timer_Reset; //+((PRE_MATCH_TIME - 40)* 0.1f);
	else
		game.Weapon_Timer = level.time + game.Weapon_Timer_Reset; //+((PRE_MATCH_TIME_BM - 40)* 0.1f);

	}

/////////////////////////////////////////////////////////////////////////////
// hm_ProcessWeaponsList
//
// Sets up the list of available weapons.

void hm_ProcessWeaponsList( void )
	{
	int		WeaponCount=0, idx=0;

	if (HmWeaponsAvail & hm_PISTOL)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 1;
		}

	if (HmWeaponsAvail & hm_SHOTGUN)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 2;
		}

	if (HmWeaponsAvail & hm_TOMMYGUN)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 3;
		}

	if (HmWeaponsAvail & hm_BARMACHINEGUN)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 4;
		}

	if (HmWeaponsAvail & hm_GRENADELAUNCHER)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 5;
		}

	if (HmWeaponsAvail & hm_ROCKETLAUNCHER)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 6;
		}

	if (HmWeaponsAvail & hm_FLAMEGUN)
		{
		WeaponCount++;
		game.HmWeaponsAvailList[idx++].nWeaponNumber = 7;
		}

	game.Num_Weapons = WeaponCount;

	}

/////////////////////////////////////////////////////////////////////////////
// hm_Setcurrentweapon:
//
// Used to set the current weapon and ammo amount for new clients.

void Hm_Setcurrentweapon(gclient_t* cl, qboolean bResetAmmo)
	{
    int nCurWeaponIndex;
	int nAmmoIndex;
	int nAmmoInitial;
	gitem_t		*item, *ammo;

	if(!cl)
		return;

	nAmmoIndex       = Hitmenlist[game.Current_Weapon].nHmIndexAmmo;
    nCurWeaponIndex	 = Hitmenlist[game.Current_Weapon].nHmIndexWeapon;

//	if(bResetAmmo && (deathmatch->value || !level.intermissiontime))
//		{
        // not in intermission and/or in deathmatch allocate default ammo
		nAmmoInitial = Hitmenlist[game.Current_Weapon].nAmmoInitial;

		// Set the initial ammo for this weapon
		Hm_set_ammo(cl, nAmmoInitial, game.Current_Weapon, true);

//		}

	// allocate new weapon (in case not already done)
	cl->pers.inventory[nCurWeaponIndex] = 1;
	cl->newweapon                       = &itemlist[nCurWeaponIndex];
	cl->pers.weapon                     = &itemlist[nCurWeaponIndex];
    cl->pers.selected_item              = nCurWeaponIndex;
	cl->ammo_index						= nAmmoIndex;

	// Load the weapon with the correct ammo.
	item = FindItem(Hitmenlist[game.Current_Weapon].szName);
	ammo = FindItem (item->ammo);
	AutoLoadWeapon( cl, item, ammo );

	}

//////////////////////////////////////////////////////////////////////////
//
// Set the weapon Timer for 1 second and clear the second counter.
//

void Hm_Set_Timers(gclient_t* client)
	{
    if(!client)
        return;

	client->Hm_sectimer		= level.time + HM_ONESECOND;
	client->Hm_ammotics		= 0;                               
	}

/////////////////////////////////////////////////////////////////////////////
//
//	check to see if we need to add some health or ammo.

void Hm_Check_Timers(edict_t* ent)
	{
	gclient_t* client;

  	if (!ent->inuse)
    	return;

    // don't do anything after client dies
	if (ent->health < 1)
        return;

	client = ent->client;
	if (!client)
		return;

	// nothing to do if client is observing
//	if(ent->svflags & SVF_NOCLIENT)
//		return;

	if (client->Hm_sectimer < level.time)
		{
	    // increment health unless HmKillForHealth is true (means you have to kill clients to get health)
		if(!HmKillForHealth)
			hm_incrementhealth(ent, client);

	    // increment ammo unless infinite ammo is enabled
		if ( !((int)dmflags->value & DF_INFINITE_AMMO) )
	   		hm_incrementammo(client);

		// reset timer
		client->Hm_sectimer = level.time + HM_ONESECOND;
		}
	}

/////////////////////////////////////////////////////////////////////////////
// Hm_incrementammo:
//
// Each client maitains its own ammo timer?

void hm_incrementammo(gclient_t* client)
	{
	int nAmmoIncrease, nAmmoIndex;

    client->Hm_ammotics++;

    if (client->Hm_ammotics >= Hitmenlist[game.Current_Weapon].nAmmoSecs) 
        {
        client->Hm_ammotics = 0;

		nAmmoIncrease   = Hitmenlist[game.Current_Weapon].nAmmoIncrement;
       	nAmmoIndex      = Hitmenlist[game.Current_Weapon].nHmIndexAmmo;

       	if(nAmmoIncrease <= 0)
	    	{
//	   	    gi.dprintf ("NIQ: invalid ammo increase amount!\n");
        	return;							
		    }

       	if(nAmmoIndex <= 0)
	    	{
//	   	    gi.dprintf ("NIQ: invalid ammo index!\n");
   	    	return;							
		    }

		// goose up ammo (niq_add_ammo enforces limits)
		Hm_add_ammo(client, nAmmoIncrease, game.Current_Weapon);
        }
	}


/////////////////////////////////////////////////////////////////////////////
// hm_incrementhealth:
//
// Each client maintains his own health timer?

void hm_incrementhealth(edict_t *ent, gclient_t* client)
	{
	qboolean noise = false;
	qboolean bDrowning;
	qboolean bInLava;
	qboolean bInSlime;
	qboolean bFighting;

    // health can't increase while drowning etc.
	bDrowning = (ent->waterlevel >= 3 && ent->air_finished < level.time);
	bInLava   = (ent->watertype & CONTENTS_LAVA);
	bInSlime  = (ent->watertype & CONTENTS_SLIME);

	// in case hlthmax is reduced during a game
//	if (ent->health > niq_hlthmax->value)
//		ent->health = niq_hlthmax->value;

	// Can't add health whilst we are in the shit - so to speak.
	if(bDrowning || bInLava || bInSlime)
		return;

//	if (ent->health < niq_hlthmax->value)
	if (ent->health < 100)
        {
		noise = HmHealthSound;

		// 28/03/98 niq: client health can't improve while fighting (firing weapon) in DM???
		bFighting = false;

		// test if human player is actively fighting -- defined as weapon is firing or client trying to fire weapon
		if(client->weaponstate == WEAPON_FIRING || ((client->latched_buttons|client->buttons) & BUTTON_ATTACK))
			bFighting = true;

		if(!bFighting)
			{
//    		ent->health += niq_hlthinc->value;
    		ent->health += 5;
			}
		else
			noise = false;

//		if (ent->health > niq_hlthmax->value)
//			ent->health = niq_hlthmax->value;
		if (ent->health > 100)
			ent->health = 100;
		}

    // do health increment sound?
	if (noise)
		gi.sound (ent, CHAN_AUTO, gi.soundindex ("world/pickups/health.wav"), 1, ATTN_STATIC, 0);			 
	}

/////////////////////////////////////////////////////////////////////////////
// hm_clientkill:
//
//	Once you've killed someone increase your health.
//

void hm_KilledCheckHealthIncrease(edict_t* attacker)
{
	int			nOrigHealth;
	gclient_t	*client;

    // if we are autoincrementing nothing to do
    if(!HmKillForHealth)
        return;
        
    // don't increment health after client dies!
  	if (!attacker->inuse)
    	return;

	client = attacker->client;
	if (!client)
		return;

    // is he dead?
	if (attacker->health < 1)
        return;

    nOrigHealth = attacker->health;

    attacker->health += 25; //niq_hlthinc->value;

    // make sure we didn't exceed the maximum allowed health
    if(attacker->health > 100 /*niq_hlthmax->value*/)
        attacker->health = 100; //niq_hlthmax->value;

    // do health increment sound?
    if( HmHealthSound && (attacker->health > nOrigHealth) )
		gi.sound (attacker, CHAN_AUTO, gi.soundindex ("world/pickups/health.wav"), 1, ATTN_STATIC, 0);			 
	}

/***********************************************************************
/*
/*	Function:	Displays text on screen upon entry into game
/*
/*	Parameters:	edict_t = player entity structure
/*
/**********************************************************************/
/*void Hm_DisplayMOTD(edict_t *ent)
	{

	FILE *motd_file;
	char motd[500];
	char line[80];

	// read the motd from a file
	if (motd_file = fopen("hitmen/motd.txt", "r"))
		{
		// we successfully opened the file "motd.txt"
		if ( fgets(motd, 500, motd_file) )
			{

			// we successfully read a line from "motd.txt" into motd
			// ... read the remaining lines now
			while ( fgets(line, 80, motd_file) )
				{
				// add each new line to motd, to create a BIG message string.
				// we are using strcat: STRing conCATenation function here.
				strcat(motd, line);
				}

			// print our message.
			safe_centerprintf (ent, motd);
			}

		// be good now ! ... close the file
		fclose(motd_file);
		}
	}
*/
void Hm_LoadMOTD( void )
	{
#if 0 //hypov8 disables

	FILE	*motd_file;
	char	line[80];
	int		i;

	char filename[60];
	cvar_t	*game_dir;
	char buf[32];



	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s/hitmen_motd.txt", game_dir->string);
	strcpy(filename, buf);

	
	// Open the motd file
	if ((motd_file = fopen(filename, "r"))!=0)  // HYPOV8_ADD !=0 //"botmen/motd.txt"
		{
		i = 0;

		// Read the lines now
		while ( fgets(line, 80, motd_file) )
			{
			// Once we've read a line copy it to the MOTD array.
			strcpy(MOTD[i].textline, line);
			i++;

			// We don't want more than 3 lines so lets piss off.
			if (i>3)
				break;
			}

		// be good now ! ... close the file
		fclose(motd_file);
		}
#endif
	}

/***********************************************************************
/*
/*	Function:	Loads all the game settings.
/*
/*	Parameters:	None
/*
/**********************************************************************/
void LoadHitmenWorldIni( void )
	{	
	FILE	*f;
	cvar_t	*game_dir;
	int		IniOption = 0, Processed = 0;
	char	Buffer[256], filename[256];
	char	*VariableName = NULL, *VariableValue = NULL;
	static	qboolean	AlreadyRead = false;


	if (AlreadyRead)
		return;

	game_dir = gi.cvar ("game", "", 0);

	Com_sprintf(filename, sizeof(filename), "./%s/%s", game_dir->string, HM_INI_FILE);


	// open the *.ini file

	if ((f = fopen (filename, "r")) == NULL)
		{
		gi.dprintf("Unable to read %s. Using defaults.\n", HM_INI_FILE);
		return;
		}

	gi.dprintf("\nProcessing Hitmen %s.. \n", HM_INI_FILE);

	// read 256 characters or until we get to the eof or a return for a newline.

	while (fgets(Buffer, sizeof(Buffer), f) != NULL)
		{

		
		// Ignore this line if it starts with a #, newline, space or [ bracket.

		if (Buffer[0] != '\t' && Buffer[0] != ' ' && Buffer[0] != '\n' && Buffer[0] != '#' && Buffer[0] != '[')
			{

			// Get the variable name, skipping spaces, tabs, and newlines.

			VariableName	= strtok(Buffer, " \t\n");
			IniOption	= 0;

			// If we haven't processed the maximum number of options then keep going
			while (IniOption < MAX_OPTIONS)
				{

				// Find this option in the array of options, if we don't find it tough

				if (!strcmp(VariableName, option[IniOption].ident))
					{

					// Using NULL will continue the search for the value from where the previous
					// strtok for the variable name left off.
					VariableValue = strtok(NULL, " \t\n#");

					// If the variable name is stdlog then we want to set the flag to turn
					// logging on
					if (!strcmp(VariableName, "stdlog"))
						gi.cvar_set("stdlogfile", VariableValue);
					else
					// This will set the valu in the array using string value to integer conversion
					*option[IniOption].variable = atoi(VariableValue);

					Processed++;
					break;
					}

				IniOption++;
				}
			}
		}

	gi.dprintf("%d Hitmen Options processed\n", Processed);
	fclose (f);
	AlreadyRead = true;	
	}

/***********************************************************************
/*
/*	Function:	Ensures that no knobs try to frig the values.
/*
/*	Parameters:	None
/*
/**********************************************************************/
void VerifyIniFileValues( void )
	{	
	int	Loop;

	for ( Loop=0; Loop<MAX_OPTIONS; Loop++ )
		{

		// If the value which has been set isn't in the normal range then
		// set it to a default value.

		if ((*option[Loop].variable < option[Loop].MinVariable) ||
			(*option[Loop].variable > option[Loop].MaxVariable))
			{
			*option[Loop].variable = option[Loop].DefaultVariable;
			}
		}
	}

/***********************************************************************
/*
/*	Function:	Displays a superb message of the day 
/*
/**********************************************************************/
//hm
#if 0
void MOTDScoreboardMessage (edict_t *ent)
	{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j;
	int		yofs;
	char	*seperator = "+++++++++++++++++++++++++";

	char	*selectheader[] =
			{
			"Atrophy Presents",
			"HITMEN BOTS v1.0",
			"By Rat Instinct",
			"Original idea by Mike Fox",
			"www.atrophy.co.uk",
			NULL
			};

	string[0] = 0;
	stringlength = 0;

	yofs = 80 - MOTD_lines * 10;
	if (yofs < 0 )
		yofs = 0;

	for (i=0; selectheader[i]; i++)
		{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 752 \"%s\" ",
			-5*strlen(selectheader[i]), yofs + (int)(-60.0+-3.5*14), selectheader[i] );

		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
		}

	yofs += 10;
	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 772 \"%s\" ",
		-5*strlen(seperator), yofs + (int)(-60.0+-3.5*14), seperator );

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	yofs += 30;

	for (i=0; i< MOTD_lines; i++)
		{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 842 \"%s\" ",
			-5*strlen(MOTD[i].textline), yofs + (int)(-60.0+-3.5*14), MOTD[i].textline );

		j = strlen(entry);
		if (stringlength + j < 1024)
			{
			strcpy (string + stringlength, entry);
			stringlength += j;
			}

		yofs += 20;
		}

//	yofs += 10;
	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 772 \"%s\" ",
		-5*strlen(seperator), yofs + (int)(-60.0+-3.5*14), seperator );

	j = strlen(entry);
	if (stringlength + j < 1024)
		{
		strcpy (string + stringlength, entry);
		stringlength += j;
		}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);

	}

#endif
/***********************************************************************
/*
/*	Function:	Displays what the current game config is.
/*
/**********************************************************************/
void ShowHitmenConfig(edict_t *ent)
	{
	char	entry[1024];
	char	string[1400];
	char	*yesno;
	int		stringlength;
	int		i, j, IniOption;
	int		yofs;
	char	*seperator = "--------------------------------";

	char	*selectheader[] =
			{
			"HITMEN CONFIGURATION",
			"--------------------------------",
			"Random Weapon Sequence - ",
			"Sound Weapon Changing  - ",
			"Sound Health Increase  - ",
			"Kill For Health        - ",
			"Show Attackers Health  - ",
			"Grappling hook active  - ",
			"GSLogging active       - ",
			"Weapon Change Time     - ",
			"Weapons available      - ",
			NULL
			};

	yesno = NULL;
	
	string[0] = 0;
	stringlength = 0;

	yofs = 50;

	if (yofs < 0 )
		yofs = 0;

	for (i=0; i<2; i++)
		{
		Com_sprintf (entry, sizeof(entry),
			"xl %i yv %i dmstr 960 \"%s\" ", 30 , yofs + (int)(-60.0 + -3.5*14), selectheader[i] );

		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
		}

	IniOption=0;

	for (i=2; i<9; i++)
		{
		if (*option[IniOption++].variable == 1)
			yesno = "Yes";
		else
			yesno = "No ";
			
		Com_sprintf (entry, sizeof(entry),
			"xl %i yv %i dmstr 999 \"%s%s\" ", 30 , yofs + (int)(-60.0 + -3.5*14), selectheader[i], yesno );

		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
		}

	// Show the weapon change in seconds
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 999 \"%s%i%s\" ",
		 30 , yofs + (int)(-60.0 + -3.5*14), selectheader[9], *option[IniOption++].variable, "s" );

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	yofs += 20;

	// Show the value for the weapons
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 999 \"%s%i\" ",
		 30 , yofs + (int)(-60.0 + -3.5*14), selectheader[10], *option[IniOption].variable );

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	yofs += 20;

	// End of the table
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 442 \"%s\" ", 30 , yofs + (int)(-60.0+-3.5*14), seperator );

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	j = strlen(entry);

	if (stringlength + j < 1024)
		{
		strcpy (string + stringlength, entry);
		stringlength += j;
		}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);

	}


/***********************************************************************
/*
/*	Function:	Displays the current players stats compared to the
/*				leaders
/*
/**********************************************************************/
#if 0
void ShowHitmenStats(edict_t *ent)
	{
	char	entry[1024];
	char	string[1400];
	int		stringlength, PlayerStats[7];
	int		i, j, stat, x, y, total, StatToView;
	int		yofs, score;
	char	*seperator = "-----------------------------";
	edict_t	*cl_ent;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];


	char	*selectheader[] =
			{
			"STATS FOR ",							//0
			"-----------------------------",		//1
			"Frags               - ",				//2
			"Deaths by opponents - ",				//3
			"Suicides            - ",				//4
			"Kills per minute    - ",				//5
			"Longest time alive  - ",				//6
			"Current kill streak - ",				//7
			"Highest kill streak - ",				//8
			NULL
			};

	// Sort the clients by score so we can display the leaders score
	// in the game

	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
		{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse)
			continue;

		score = game.clients[i].resp.score;

		for (x=0 ; x<total ; x++)
			{
			if (score > sortedscores[x])
				break;
			}

		for (y=total ; y>x ; y--)
			{
			sorted[y] = sorted[y-1];
			sortedscores[y] = sortedscores[y-1];
			}

		sorted[x] = i;
		sortedscores[x] = score;
		total++;
		}

	// Get the stats number we want to view
	if ((ent->client->resp.currentstat > total) || (ent->client->resp.currentstat < 1))
		ent->client->resp.currentstat = 1;

	// We need to adjust this for starting at zero
	StatToView = ent->client->resp.currentstat - 1;

	// Get the leaders edict structure.
	cl_ent = g_edicts + 1 + sorted[StatToView];

	PlayerStats[0] = cl_ent->client->resp.score;
	PlayerStats[1] = cl_ent->client->resp.deaths;
	PlayerStats[2] = cl_ent->client->resp.suicides;

	// Only do frags per minute if we have a positive score so we know we've actually
	// killed someone.
	if (cl_ent->client->resp.score > 0)
	{
		int  timeIn;
		double tmp1, tmp2;
		
		timeIn = (level.framenum - cl_ent->client->resp.enterframe);
		if (timeIn > 300) //hypov8 fix stats. 30 secs
		{
			tmp1 = ((double)timeIn / 600);
			tmp2 = (double)cl_ent->client->resp.score / tmp1;
			PlayerStats[3] = (int)tmp2;
			//PlayerStats[3] = (int)(cl_ent->client->resp.score / ((level.framenum - cl_ent->client->resp.enterframe) / 600));
		}
		else
			PlayerStats[3] = 0;

	}
	else
		PlayerStats[3] = 0;

	PlayerStats[4] = cl_ent->client->resp.timealive;
	PlayerStats[5] = cl_ent->client->resp.killstreak;
	PlayerStats[6] = cl_ent->client->resp.maxkillstreak;
//	PlayerStats[6] = StatToView;

	string[0] = 0;
	stringlength = 0;

	yofs = 10;

	if (yofs < 0 )
		yofs = 0;

	//
	// Write main heading with players names.
	//
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 960 \"%s%s\" ", 10 , yofs + (int)(-60.0 + -3.5*14), selectheader[0], cl_ent->client->pers.netname);

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	yofs += 20;

	//
	// Just the underline
	//
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 442 \"%s\" ", 10 , yofs + (int)(-60.0 + -3.5*14), selectheader[1]);

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;

	yofs += 20;
	stat = 0;

	//
	// Now write the 7 lines of the actual stats
	//
	for (i=2; i<9; i++)
		{

		if (stat != 4)
		Com_sprintf (entry, sizeof(entry),
			"xl %i yv %i dmstr 999 \"%s%i\" ", 10 , yofs + (int)(-60.0 + -3.5*14), selectheader[i], PlayerStats[stat++] );
		else
		Com_sprintf (entry, sizeof(entry),
			"xl %i yv %i dmstr 999 \"%s%i%s\" ", 10 , yofs + (int)(-60.0 + -3.5*14), selectheader[i], PlayerStats[stat++], " Secs" );

		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
		}

	// End of the table
	Com_sprintf (entry, sizeof(entry),
		"xl %i yv %i dmstr 442 \"%s\" ", 10 , yofs + (int)(-60.0+-3.5*14), seperator );

//	j = strlen(entry);
//	strcpy (string + stringlength, entry);
//	stringlength += j;

	j = strlen(entry);
	if (stringlength + j < 1024)
		{
		strcpy (string + stringlength, entry);
		stringlength += j;
		}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);

	}
#else //hypo redo scores
void ShowHitmenStats(edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength, PlayerStats[12]; //hypov8 was 7
	int		i, j, stat, x, y, total, StatToView;
	int		yofs, score, headderCount=0;
	char	*seperator = "-----------------------";
	edict_t	*cl_ent;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];


	char	*selectheader[] =
	{
		"STATS FOR ",							//0
		"-----------------------",				//1
		"Frags               - ",				//2
		"Deaths by opponents - ",				//3
		"Suicides            - ",				//4
		"Kills per minute    - ",				//5
		"Longest time alive  - ",				//6
		"Current kill streak - ",				//7
		"Highest kill streak - ",				//8
		//" ",									//9
		"My Stats ",							//10
		"-----------------------",				//11
		"Suicides            - ",				//12
		"Kills per minute    - ",				//13
		"Longest time alive  - ",				//14
		"Current kill streak - ",				//15
		"Highest kill streak - ",				//16
		NULL
	};


	// Sort the clients by score so we can display the leaders score
	// in the game
	total = 0;
	for (i = 0; i < game.maxclients; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (!cl_ent->inuse)
			continue;

		score = game.clients[i].resp.score;

		for (x = 0; x < total; x++)
		{
			if (score > sortedscores[x])
				break;
		}

		for (y = total; y > x; y--)
		{
			sorted[y] = sorted[y - 1];
			sortedscores[y] = sortedscores[y - 1];
		}

		sorted[x] = i;
		sortedscores[x] = score;
		total++;
	}

	// Get the stats number we want to view
	if ((ent->client->resp.currentstat > total) || (ent->client->resp.currentstat < 1))
		ent->client->resp.currentstat = 1;

	// We need to adjust this for starting at zero
	StatToView = ent->client->resp.currentstat - 1;

	// Get the leaders edict structure.
	cl_ent = g_edicts + 1 + sorted[StatToView];

	PlayerStats[0] = cl_ent->client->resp.score;
	PlayerStats[1] = cl_ent->client->resp.deaths;
	PlayerStats[2] = cl_ent->client->resp.suicides;


	// Only do frags per minute if we have a positive score so we know we've actually
	// killed someone.
	if (cl_ent->client->resp.score > 0)
	{
		int  timeIn;

		timeIn = (level.framenum - cl_ent->client->resp.enterframe);
		if (timeIn > 300) //hypov8 fix stats. 30 secs
			PlayerStats[8] = (int)((double)cl_ent->client->resp.score / ((double)timeIn / 600));
		else
			PlayerStats[3] = 0;
	}
	else
		PlayerStats[3] = 0;

	PlayerStats[4] = cl_ent->client->resp.timealive;
	PlayerStats[5] = cl_ent->client->resp.killstreak;
	PlayerStats[6] = cl_ent->client->resp.maxkillstreak;
	//	PlayerStats[6] = StatToView;

	//player "self" status
	PlayerStats[7] = ent->client->resp.suicides;
	if (ent->client->resp.score > 0)
	{
		int  timeIn;

		timeIn = (level.framenum - ent->client->resp.enterframe);
		if (timeIn > 300) //hypov8 fix stats. 30 secs
			PlayerStats[8] = (int)((double)ent->client->resp.score / ((double)timeIn / 600));
		else
			PlayerStats[8] = 0;
	}
	else
		PlayerStats[8] = 0;

	PlayerStats[9] = ent->client->resp.timealive;
	PlayerStats[10] = ent->client->resp.killstreak;
	PlayerStats[11] = ent->client->resp.maxkillstreak;
	//end self

	/////////////
	// begin menu
	/////////////
	string[0] = 0;
	stringlength = 0;

	yofs = 10;

	if (yofs < 0)
		yofs = 0;

	///////////////////////////////////////////////
	// Write main heading with winning player name.
	///////////////////////////////////////////////
	Com_sprintf(entry, sizeof(entry),
		"xl %i yv %i dmstr 773 \"%s%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount], cl_ent->client->pers.netname);

	j = strlen(entry);
	strcpy(string + stringlength, entry);
	stringlength += j;

	yofs += 12;
	headderCount += 1;

	/////////////////////
	// 1st underline
	/////////////////////
	Com_sprintf(entry, sizeof(entry),
		"xl %i yv %i dmstr 773 \"%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount]);

	j = strlen(entry);
	strcpy(string + stringlength, entry);
	stringlength += j;

	yofs += 16;
	stat = 0;
	headderCount += 1;

	////////////////////////////////////////////
	// Write 7 lines of the winners stats
	////////////////////////////////////////////
	for (i = 1; i<=7; i++)
	{

		if (stat != 4)
			Com_sprintf(entry, sizeof(entry),
			"xl %i yv %i dmstr 777 \"%s%i\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount], PlayerStats[stat++]);
		else
			Com_sprintf(entry, sizeof(entry),
			"xl %i yv %i dmstr 777 \"%s%i%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount], PlayerStats[stat++], "s");

		j = strlen(entry);
		strcpy(string + stringlength, entry);
		stringlength += j;

		//stat += 1;
		yofs += 20;
		headderCount += 1;
	}


	/////////////////////
	// My Stats
	/////////////////////
	if (ent->s.number != cl_ent->s.number){
		yofs += 8;
		Com_sprintf(entry, sizeof(entry),
			"xl %i yv %i dmstr 773 \"%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount]);

		j = strlen(entry);
		strcpy(string + stringlength, entry);
		stringlength += j;

		yofs += 12;
		headderCount += 1;
	}

	/////////////////////
	// 2nd underline
	/////////////////////
	if (ent->s.number != cl_ent->s.number){
		Com_sprintf(entry, sizeof(entry),
			"xl %i yv %i dmstr 773 \"%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount]);

		j = strlen(entry);
		strcpy(string + stringlength, entry);
		stringlength += j;

		yofs += 16;
		headderCount += 1;
	}

	////////////////////////////////////////////
	// Write owners stats
	////////////////////////////////////////////
	if (ent->s.number != cl_ent->s.number){
		for (i = 1; i <= 5; i++)
		{

			if (stat != 9)
				Com_sprintf(entry, sizeof(entry),
				"xl %i yv %i dmstr 777 \"%s%i\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount], PlayerStats[stat++]);
			else
				Com_sprintf(entry, sizeof(entry),
				"xl %i yv %i dmstr 777 \"%s%i%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), selectheader[headderCount], PlayerStats[stat++], "s");

			j = strlen(entry);
			strcpy(string + stringlength, entry);
			stringlength += j;

			yofs += 20;
			headderCount += 1;
		}
	}

	/////////////////////
	// last underline
	/////////////////////
	yofs -= 4;
	Com_sprintf(entry, sizeof(entry),
		"xl %i yv %i dmstr 773 \"%s\" ", 10, yofs + (int)(-60.0 + -3.5 * 14), seperator);


	j = strlen(entry);
	if (stringlength + j < 1024)
	{
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string);

}
#endif
/////////////////////////////////////////////////////////////////////////////////

//===================================================================

#if 0 //HM HYPO
void RejoinScoreboardMessage (edict_t *ent)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j;
	int		yofs;
	char	*seperator = "==================================";
	char	*selectheader[] =
		{
			"Atrophy Presents HITMEN BOTS v1.0",
			"By Rat Instinct",
			"Send comments to : ratinstinct@atrophy.co.uk",
			"Respect goes to the Comp Mod lads for this code.",
			NULL
		};

	char	*rejoinheader[] =
		{
			"You were just playing on this server.",
			"Would you like to continue where you left off?",
			"Select the corresponding number or "
			"use [ ] and enter to make your selection.",
			NULL
		};
	char	*choices[] =
		{
			"1 - Yes",
			"2 - No",
			NULL
		};


	string[0] = 0;
	stringlength = 0;
	
	yofs = 0;

	for (i=0; selectheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 752 \"%s\" ",
			-5*strlen(selectheader[i]), yofs + (int)(-60.0+-3.5*14), selectheader[i] );

		j = strlen(entry);
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
	}
	yofs += 10;
	Com_sprintf (entry, sizeof(entry),
		"xm %i yv %i dmstr 772 \"%s\" ",
		-5*strlen(seperator), yofs + (int)(-60.0+-3.5*14), seperator );

	j = strlen(entry);
	strcpy (string + stringlength, entry);
	stringlength += j;
	yofs += 30;

	for (i=0; rejoinheader[i]; i++)
	{
		Com_sprintf (entry, sizeof(entry),
			"xm %i yv %i dmstr 842 \"%s\" ",
			-5*strlen(rejoinheader[i]), yofs + (int)(-60.0+-3.5*14), rejoinheader[i] );

		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;

		yofs += 20;
	}
	yofs += 30;
	for (i=0; choices[i]; i++)
	{
		if (ent->vote == i)
			Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 999 \"--> %s\" ",
					-5*40, yofs + (int)(-60.0+-3.5*14), choices[i]);
		else
			Com_sprintf (entry, sizeof(entry), "xm %i yv %i dmstr 777 \"    %s\" ",
					-5*40, yofs + (int)(-60.0+-3.5*14), choices[i]);

		j = strlen(entry);
		if (stringlength + j < 1024)
		{
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		yofs += 20;

	}


	gi.WriteByte (svc_layout);
	gi.WriteString (string);

}
//HME
#endif


/////////////////////////////////////////////////////////////////////////////////

