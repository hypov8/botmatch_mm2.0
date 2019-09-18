#include "g_local.h"
#include "file.h"
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#define stat _stati64 // present in msvcrt.dll
#endif

#define		MAX_STRING_LENGTH	100
#define		ADD_MOTD_LINE			3

int fgetline (FILE* infile, char* buffer)
{
	int		i = 0;
	char	ch;

	ch = fgetc(infile);

	while ((ch != EOF) && ((unsigned)ch < ' '))
		ch = fgetc(infile);

	while ((ch != EOF) && (ch != '\n') && (ch != '\r'))
	{
		if (i < (MAX_STRING_LENGTH-1))
			buffer[i++] = ch;
		ch = fgetc(infile);
	}

	buffer[i] = 0;
	
	return i;
}

int proccess_ini_file()
{
	FILE*	infile;
	int	mode = -1, c;
	char	buffer[MAX_STRING_LENGTH];
	char	key[64], param[64];
	char	*dir;
	cvar_t	*game_dir;
	struct stat st;
	static time_t lasttime = 0;

	game_dir = gi.cvar("game", "", 0);
	dir = game_dir->string[0] ? game_dir->string : "main";

	if (stat(va("%s/comp.ini", dir), &st))
	{
		gi.error("comp.ini file is missing");
		return false;
	}
	if (st.st_mtime == lasttime)
		goto skipini;
	lasttime = st.st_mtime;

	default_map[0]=0;
	default_teamplay[0]=0;
	default_dmflags[0]=0;
	default_password[0]=0;
	default_dm_realmode[0]=0;
	default_anti_spawncamp[0]=0;
	default_bonus[0]=0;

	admincode[0]=0;

	map_list_filename[0]=0;
	ban_name_filename[0]=0;
	ban_ip_filename[0]=0;
	rconx_file[0]=0;

	allow_map_voting = false;
	disable_admin_voting = false;
	pregameframes = 300;
	fixed_gametype = false;
	enable_password = false;
	keep_admin_status = false;
	default_random_map = false;
	disable_curse = false;
	unlimited_curse = false;
	pickup_sounds = false;
	// BEGIN HITMEN
	enable_hitmen = false;
	//END
	enable_killerhealth = false;
	wait_for_players = false;
	num_MOTD_lines = 0;

	// Open config file
	infile = fopen(va("%s/comp.ini", dir), "r");
	if (infile == NULL)
	{
		gi.error("Failed to open comp.ini file");
		return false;
	}
	
	while (fgetline(infile, buffer))	// Retrieve line from the file
	{
		// Check to see if this is a comment line
		if (buffer[0] == '/' && buffer[1] == '/')
		{
			mode=-1; // end MOTD
			continue;
		}

		if (mode == ADD_MOTD_LINE)
		{
			if (num_MOTD_lines < 20)
			{
				strncpy(MOTD[num_MOTD_lines], buffer, sizeof(MOTD[num_MOTD_lines]) - 1);
			num_MOTD_lines++;
			}
			continue;
		}

		c = sscanf(buffer, "%s %s", key, param);
		if (!c) continue;

		if (!strcmp(key, "admin_code"))
			strncpy(admincode, param, 16);
		else if (!strcmp(key, "maps_file") || !strcmp(key, "custom_map_file"))
		{
			if ((int)teamplay->value != 1 || !map_list_filename[0])
			{
				if (c == 2) strncpy(map_list_filename, param, 32);
			}
		}
		else if (!strcmp(key, "bm_maps_file"))
		{
			if ((int)teamplay->value == 1)
			{
				if (c == 2) strncpy(map_list_filename, param, 32);
			}
		}
		else if (!strcmp(key, "default_map"))
		{
			if (c==2) 
				strncpy(default_map, param, 32);
		}
		else if (!strcmp(key, "default_teamplay"))
		{
			if (c==2) 
				strncpy(default_teamplay, param, 16);
		}
		else if (!strcmp(key, "default_dmflags"))
		{
			if (c==2) 
				strncpy(default_dmflags, param, 16);
		}
		else if (!strcmp(key, "default_password"))
		{
			if (c==2) 
				strncpy(default_password, param, 16);
		}
		else if (!strcmp(key, "default_timelimit"))
		{
			if (c==2) 
				strncpy(default_timelimit, param, 16);
		}
		else if (!strcmp(key, "default_fraglimit"))
		{
			if (c==2) 
				strncpy(default_fraglimit, param, 16);
		}
		else if (!strcmp(key, "default_cashlimit"))
		{
			if (c==2) 
				strncpy(default_cashlimit, param, 16);
		}
		else if (!strcmp(key, "default_dm_realmode"))
		{
			if (c==2) 
				strncpy(default_dm_realmode, param, 16);
		}
		else if (!strcmp(key, "default_anti_spawncamp"))
		{
			if (c==2) 
				strncpy(default_anti_spawncamp, param, 16);
		}
		else if (!strcmp(key, "default_bonus"))
		{
			if (c==2) 
				strncpy(default_bonus, param, 16);
		}
		else if (!strcmp(key, "MOTD"))
			mode = ADD_MOTD_LINE;
		else if (!strcmp(key, "allow_map_voting"))
			allow_map_voting = true;
		else if (!strcmp(key, "ban_name_filename"))
		{
			if (c==2) 
				strncpy(ban_name_filename, param, 32);
		}
		else if (!strcmp(key, "ban_ip_filename"))
		{
			if (c==2) 
				strncpy(ban_ip_filename, param, 32);
		}
		else if (!strcmp(key, "disable_admin_voting"))
			disable_admin_voting = true;
		else if (!strcmp(key, "pregame"))
		{
			if (c == 2)
				pregameframes = atoi(param) * 10;
		}
		else if (!strcmp(key, "fixed_gametype"))
			fixed_gametype = true;
		else if (!strcmp(key, "enable_password"))
			enable_password = true;
		else if (!strcmp(key, "rconx_file"))
		{
			if (c==2) 
				strncpy(rconx_file, param, 32);
		}
		else if (!strcmp(key, "keep_admin_status"))
			keep_admin_status = true;
		else if (!strcmp(key, "default_random_map"))
			default_random_map = true;
		else if (!strcmp(key, "disable_curse"))
			disable_curse = true;
		else if (!strcmp(key, "unlimited_curse"))
			unlimited_curse = true;
		else if (!strcmp(key, "pickup_sounds"))
			pickup_sounds = true;
		else if (!strcmp(key, "enable_killerhealth"))
			enable_killerhealth = true;
		else if (!strcmp(key, "wait_for_players"))
			wait_for_players = true;
		// BEGIN HITMEN
		else if (!strcmp(key, "enable_hitmen"))
		{
			enable_hitmen = true;
			sv_hitmen = gi.cvar_forceset("hitmen", "1");
		}
		//END
		else
			gi.dprintf("Unknown comp.ini line: %s\n",buffer);
	}

	// close the ini file
	fclose(infile);

	gi.dprintf("Processed comp.ini file\n");

skipini:

	if (ban_name_filename[0])
	{
		static time_t lasttime = 0;
		if (!lasttime)
			num_ban_names = 0;

		if (stat(va("%s/%s", dir, ban_name_filename), &st) || st.st_mtime == lasttime)
			goto skipban_name;
		lasttime = st.st_mtime;

		num_ban_names = 0;

		infile = fopen(va("%s/%s", dir, ban_name_filename), "r");
		if (infile != NULL)	
		{
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				kp_strlwr(buffer);
				strncpy(ban_name[num_ban_names].value, buffer, 16);
				num_ban_names++;
				if (num_ban_names == 100)
					break;
			}
			fclose(infile);
			gi.dprintf("Processed name bans file (%d bans)\n", num_ban_names);
		}
	}
	else
		num_ban_names = 0;

skipban_name:

	if (ban_ip_filename[0])
	{
		static time_t lasttime = 0;
		if (!lasttime)
			num_ban_ips = 0;

		if (stat(va("%s/%s", dir, ban_ip_filename), &st) || st.st_mtime == lasttime)
			goto skipban_ip;
		lasttime = st.st_mtime;

		num_ban_ips = 0;

		infile = fopen(va("%s/%s", dir, ban_ip_filename), "r");
		if (infile != NULL)	
		{
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				strncpy(ban_ip[num_ban_ips].value, buffer, 16);
				num_ban_ips++;
				if (num_ban_ips == 100)
					break;
			}
			fclose(infile);
			gi.dprintf("Processed IP bans file (%d bans)\n", num_ban_ips);
		}
	}
	else
		num_ban_ips = 0;

skipban_ip:

	if (rconx_file[0])
	{
		static time_t lasttime = 0;
		if (!lasttime)
			num_rconx_pass = 0;

		if (stat(va("%s/%s", dir, rconx_file), &st) || st.st_mtime == lasttime)
			goto skiprconx;
		lasttime = st.st_mtime;

		num_rconx_pass = 0;

		infile = fopen(va("%s/%s", dir, rconx_file), "r");
		if (infile != NULL)	
		{
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				strncpy(rconx_pass[num_rconx_pass].value, buffer, sizeof(rconx_pass[num_rconx_pass].value) - 1);
				num_rconx_pass++;
				if (num_rconx_pass == 100)
					break;
			}
			fclose(infile);
			gi.dprintf("Processed rconx password file (%d passwords)\n", num_rconx_pass);
		}
	}
	else
		num_rconx_pass = 0;

skiprconx:

	if (!map_list_filename[0])
		strcpy(map_list_filename, g_mapcycle_file->string);
	if (map_list_filename[0])
{
	char	map[32], map2[32];
		static time_t lasttime = 0;
		if (!lasttime)
			num_maps = 0;

		if (stat(va("%s/%s", dir, map_list_filename), &st) || st.st_mtime == lasttime)
			goto skipmaps;
		lasttime = st.st_mtime;

	num_maps = 0;

		infile = fopen(va("%s/%s", dir, map_list_filename), "r");
		if (infile != NULL)
		{
	while (fgetline(infile, buffer))	// Retrieve line from the file
	{
		// Check to see if this is a comment line
		if (buffer[0] == '/' && buffer[1] == '/')
			continue;

		c = sscanf(buffer, "%s %s", map, map2);
				if (c == 2 && map[0]>='1' && map[0]<='9') // old map list with rank included?
					strncpy(maplist[num_maps], map2, sizeof(maplist[num_maps]) - 1);
				else if (c)
			strncpy(maplist[num_maps], map, sizeof(maplist[num_maps]) - 1);
		else
			continue;
		kp_strlwr(maplist[num_maps]); // prevent MAX_GLTEXTURES errors caused by uppercase letters in the map vote pic names
				if (!file_exist(va("maps/%s.bsp", maplist[num_maps])))
		{
			if (kpded2)
			{
				// check for an "override" file
						if (file_exist(va("maps/%s.bsp.override", maplist[num_maps])))
							goto mapok;
			}
			gi.dprintf("warning: \"%s\" map is missing (removed from map list)\n", maplist[num_maps]);
			continue;
		}
mapok:
		num_maps++;
		if (num_maps == 1024) break;
	}
	fclose(infile);
			gi.dprintf("Processed map list file (%s = %d maps)\n", map_list_filename, num_maps);
		}
	}
	else
		num_maps = 0;

skipmaps:

	return true;
}

#ifdef _WIN32
#include <io.h>
#define access(a,b) _access(a,b)
#else
#include <unistd.h>
#endif

int file_exist(const char *file)
{
	char	buf[MAX_QPATH];
	cvar_t	*game_dir = gi.cvar("game", "", 0);
	if (game_dir->string[0])
	{
		Com_sprintf(buf, sizeof(buf), "%s/%s", game_dir->string, file);
		if (!access(buf, 4)) return true;
	}
	Com_sprintf(buf, sizeof(buf), "main/%s", file);
	if (!access(buf, 4)) return true;
	// assume kpdm1-5 and team_pv/rc/sr are available if pak0.pak exists
	if (!Q_stricmp(file,"maps/kpdm1.bsp") || !Q_stricmp(file,"maps/kpdm2.bsp") || !Q_stricmp(file,"maps/kpdm3.bsp") || !Q_stricmp(file,"maps/kpdm4.bsp") || !Q_stricmp(file,"maps/kpdm5.bsp")
			|| !Q_stricmp(file,"maps/team_pv.bsp") || !Q_stricmp(file,"maps/teamp_rc.bsp") || !Q_stricmp(file,"maps/team_sr.bsp"))
		return !access("main/pak0.pak", 4);
	return false;
}
