
////////////////////////////////////////////////////////////////////////////////
//
//	g_Hitmen.h
//
//	These are the routines which are called from outside the Hitmen code
//
//

extern void hm_CheckWeaponTimer( void );
extern void hm_Initialise( void );
extern void Hm_Setcurrentweapon(gclient_t* cl, qboolean bResetAmmo);
extern void Hm_Set_Timers(gclient_t* client);
extern void Hm_Check_Timers(edict_t* ent);
extern void Hm_DisplayMOTD(edict_t *ent);
extern void hm_KilledCheckHealthIncrease(edict_t* attacker);

//p_hook.c
void FireHook(edict_t *ent);


//hypov8 add
extern int	HmHookAvailable;
// edict->hookstate bit constants
#define HOOK_ON		0x00000001 // set if hook command is active
#define HOOK_IN		0x00000002 // set if hook has attached
#define SHRINK_ON	0x00000004 // set if shrink chain is active 
#define GROW_ON		0x00000008 // set if grow chain is active

// edict->sounds constants
#define MOTOR_OFF	0	   // motor sound has not been triggered
#define MOTOR_START	1	   // motor start sound has been triggered
#define MOTOR_ON	2	   // motor running sound has been triggered
