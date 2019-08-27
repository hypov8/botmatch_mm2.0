#include "g_local.h"
#include "file.h"
#include <stdio.h>

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
	char	filename[64],dir[32];
	cvar_t	*game_dir;

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
	num_maps = 0;
	num_netnames = 0;
	num_ips = 0;
	fixed_gametype = false;
	enable_password = false;
	keep_admin_status = false;
	default_random_map = false;
	disable_anon_text = false;
	disable_curse = false;
	unlimited_curse = false;
	// BEGIN HITMEN
	enable_hitmen = false;
	//END
	enable_killerhealth = false;
	wait_for_players = false;
	num_MOTD_lines = 0;

	// Open config file
	game_dir = gi.cvar("game", "", 0);
	strcpy(dir, game_dir->string[0] ? game_dir->string : "main");
	Com_sprintf (filename, sizeof(filename), "%s/comp.ini", dir);
	infile = fopen(filename, "r");
	if (infile == NULL)	return FILE_OPEN_ERROR;
	
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
			strncpy(MOTD[num_MOTD_lines].textline, buffer, sizeof(MOTD[num_MOTD_lines].textline) - 1);
			num_MOTD_lines++;
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
			if (c==2) strncpy(default_map, param, 32);
		}
		else if (!strcmp(key, "default_teamplay"))
		{
			if (c==2) strncpy(default_teamplay, param, 16);
		}
		else if (!strcmp(key, "default_dmflags"))
		{
			if (c==2) strncpy(default_dmflags, param, 16);
		}
		else if (!strcmp(key, "default_password"))
		{
			if (c==2) strncpy(default_password, param, 16);
		}
		else if (!strcmp(key, "default_timelimit"))
		{
			if (c==2) strncpy(default_timelimit, param, 16);
		}
		else if (!strcmp(key, "default_fraglimit"))
		{
			if (c==2) strncpy(default_fraglimit, param, 16);
		}
		else if (!strcmp(key, "default_cashlimit"))
		{
			if (c==2) strncpy(default_cashlimit, param, 16);
		}
		else if (!strcmp(key, "default_dm_realmode"))
		{
			if (c==2) strncpy(default_dm_realmode, param, 16);
		}
		else if (!strcmp(key, "default_anti_spawncamp"))
		{
			if (c==2) strncpy(default_anti_spawncamp, param, 16);
		}
		else if (!strcmp(key, "default_bonus"))
		{
			if (c==2) strncpy(default_bonus, param, 16);
		}
		else if (!strcmp(key, "MOTD"))
			mode = ADD_MOTD_LINE;
		else if (!strcmp(key, "allow_map_voting"))
			allow_map_voting = true;
		else if (!strcmp(key, "ban_name_filename"))
		{
			if (c==2) strncpy(ban_name_filename, param, 32);
		}
		else if (!strcmp(key, "ban_ip_filename"))
		{
			if (c==2) strncpy(ban_ip_filename, param, 32);
		}
		else if (!strcmp(key, "frags_per_hour_scoreboard"))
			fph_scoreboard = true;
		else if (!strcmp(key, "disable_admin_voting"))
			disable_admin_voting = true;
		else if (!strcmp(key, "fixed_gametype"))
			fixed_gametype = true;
		else if (!strcmp(key, "enable_password"))
			enable_password = true;
		else if (!strcmp(key, "rconx_file"))
		{
			if (c==2) strncpy(rconx_file, param, 32);
		}
		else if (!strcmp(key, "keep_admin_status"))
			keep_admin_status = true;
		else if (!strcmp(key, "default_random_map"))
			default_random_map = true;
		else if (!strcmp(key, "disable_anon_text"))
			disable_anon_text = true;
		else if (!strcmp(key, "disable_curse"))
			disable_curse = true;
		else if (!strcmp(key, "unlimited_curse"))
			unlimited_curse = true;
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

	if (ban_name_filename[0])
	{
		Com_sprintf (filename, sizeof(filename), "%s/%s", dir, ban_name_filename);
		infile = fopen(filename, "r");
		if (infile != NULL)	
		{
			num_netnames = 0;
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				kp_strlwr(buffer);
				strncpy(netname[num_netnames].value, buffer, 16);
				num_netnames++;
				if (num_netnames == 100) break;
			}
			fclose(infile);
		}
	}

	if (ban_ip_filename[0])
	{
		Com_sprintf (filename, sizeof(filename), "%s/%s", dir, ban_ip_filename);
		infile = fopen(filename, "r");
		if (infile != NULL)	
		{
			num_ips = 0;
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				strncpy(ip[num_ips].value, buffer, 16);
				num_ips++;
				if (num_ips == 100) break;
			}
			fclose(infile);
		}
	}

	if (rconx_file[0])
	{
		Com_sprintf (filename, sizeof(filename), "%s/%s", dir, rconx_file);
		infile = fopen(filename, "r");
		if (infile != NULL)	
		{
			num_rconx_pass = 0;
			while (fgetline(infile, buffer))	// Retrieve line from the file
			{
				// Check to see if this is a comment line
				if (buffer[0] == '/' && buffer[1] == '/')
					continue;

				strncpy(rconx_pass[num_rconx_pass].value, buffer, sizeof(rconx_pass[num_rconx_pass].value) - 1);
				num_rconx_pass++;
				if (num_rconx_pass == 100) break;
			}
			fclose(infile);
		}
	}

	return OK;
}

int read_map_file()
{
	FILE*	infile;
	char	buffer[MAX_STRING_LENGTH];
	char	map[32], map2[32];
	char	filename[64];
	int		c;
	cvar_t	*game_dir;

	game_dir = gi.cvar("game", "", 0);
	Com_sprintf (filename, sizeof(filename), "%s/%s", game_dir->string[0] ? game_dir->string : "main", map_list_filename);
	infile = fopen(filename, "r");
	if (infile == NULL) return FILE_OPEN_ERROR;

	num_maps = 0;

	while (fgetline(infile, buffer))	// Retrieve line from the file
	{
		// Check to see if this is a comment line
		if (buffer[0] == '/' && buffer[1] == '/')
			continue;

		c = sscanf(buffer, "%s %s", map, map2);
		if (c == 1)
			strncpy(maplist[num_maps], map, sizeof(maplist[num_maps]) - 1);
		else if (c == 2 && map[0]>='1' && map[0]<='9') // old map list with rank included?
			strncpy(maplist[num_maps], map2, sizeof(maplist[num_maps]) - 1);
		else
			continue;
		kp_strlwr(maplist[num_maps]); // prevent MAX_GLTEXTURES errors caused by uppercase letters in the map vote pic names
		Com_sprintf(buffer, sizeof(buffer), "maps/%s.bsp", maplist[num_maps]);
		if (!file_exist(buffer))
		{
			if (kpded2)
			{
				// check for an "override" file
				Com_sprintf(buffer, sizeof(buffer), "maps/%s.bsp.override", maplist[num_maps]);
				if (file_exist(buffer))
					goto ok;
			}
			gi.dprintf("warning: \"%s\" map is missing (removed from map list)\n", maplist[num_maps]);
			continue;
		}
ok:
		num_maps++;
		if (num_maps == 1024) break;
	}
	fclose(infile);

	return OK;
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
