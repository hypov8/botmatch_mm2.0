# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=game - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\cds\Release"
# PROP Intermediate_Dir "c:\cds\Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 navlib\navlib.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:I386 /nodefaultlib:"LIBC" /out:"c:\cds\Release\gamex86.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "game - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ai_bitch.c
# End Source File
# Begin Source File

SOURCE=.\ai_bum_sit.c
# End Source File
# Begin Source File

SOURCE=.\ai_dog.c
# End Source File
# Begin Source File

SOURCE=.\ai_punk.c
# End Source File
# Begin Source File

SOURCE=.\ai_runt.c
# End Source File
# Begin Source File

SOURCE=.\ai_shorty.c
# End Source File
# Begin Source File

SOURCE=.\ai_thug.c
# End Source File
# Begin Source File

SOURCE=.\ai_thug_sit.c
# End Source File
# Begin Source File

SOURCE=.\ai_whore.c
# End Source File
# Begin Source File

SOURCE=.\ep_all.c
# End Source File
# Begin Source File

SOURCE=.\ep_crystalpalace.c
# End Source File
# Begin Source File

SOURCE=.\ep_log.c
# End Source File
# Begin Source File

SOURCE=.\ep_posionville.c
# End Source File
# Begin Source File

SOURCE=.\ep_radiocity.c
# End Source File
# Begin Source File

SOURCE=.\ep_shipyards.c
# End Source File
# Begin Source File

SOURCE=.\ep_skidrow.c
# End Source File
# Begin Source File

SOURCE=.\ep_steeltown.c
# End Source File
# Begin Source File

SOURCE=.\ep_trainyard.c
# End Source File
# Begin Source File

SOURCE=.\file.c
# End Source File
# Begin Source File

SOURCE=.\g_ai.c
# End Source File
# Begin Source File

SOURCE=.\g_ai_ents.c
# End Source File
# Begin Source File

SOURCE=.\g_ai_fight.c
# End Source File
# Begin Source File

SOURCE=.\g_ai_memory.c
# End Source File
# Begin Source File

SOURCE=.\g_cast.c
# End Source File
# Begin Source File

SOURCE=.\G_chase.c
# End Source File
# Begin Source File

SOURCE=.\g_cmds.c
# End Source File
# Begin Source File

SOURCE=.\g_combat.c
# End Source File
# Begin Source File

SOURCE=.\g_func.c
# End Source File
# Begin Source File

SOURCE=.\g_items.c
# End Source File
# Begin Source File

SOURCE=.\g_joe_misc.c
# End Source File
# Begin Source File

SOURCE=.\g_main.c
# End Source File
# Begin Source File

SOURCE=.\g_misc.c
# End Source File
# Begin Source File

SOURCE=.\g_pawn.c
# End Source File
# Begin Source File

SOURCE=.\g_phys.c
# End Source File
# Begin Source File

SOURCE=.\g_save.c
# End Source File
# Begin Source File

SOURCE=.\g_spawn.c
# End Source File
# Begin Source File

SOURCE=.\g_svcmds.c
# End Source File
# Begin Source File

SOURCE=.\g_target.c
# End Source File
# Begin Source File

SOURCE=.\g_teamplay.c
# End Source File
# Begin Source File

SOURCE=.\g_trigger.c
# End Source File
# Begin Source File

SOURCE=.\g_utils.c
# End Source File
# Begin Source File

SOURCE=.\g_weapon.c
# End Source File
# Begin Source File

SOURCE=.\game.def
# End Source File
# Begin Source File

SOURCE=.\gslog.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\m_bbox.c
# End Source File
# Begin Source File

SOURCE=.\m_move.c
# End Source File
# Begin Source File

SOURCE=.\p_client.c
# End Source File
# Begin Source File

SOURCE=.\p_hud.c
# End Source File
# Begin Source File

SOURCE=.\p_view.c
# End Source File
# Begin Source File

SOURCE=.\p_weapon.c
# End Source File
# Begin Source File

SOURCE=.\q_shared.c
# End Source File
# Begin Source File

SOURCE=.\stdlog.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\tourney.c
# End Source File
# Begin Source File

SOURCE=.\vehicles.c
# End Source File
# Begin Source File

SOURCE=.\voice.c
# End Source File
# Begin Source File

SOURCE=.\voice_bitch.c
# End Source File
# Begin Source File

SOURCE=.\voice_punk.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ai_bitch.h
# End Source File
# Begin Source File

SOURCE=.\ai_bitch_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_bum_sit.h
# End Source File
# Begin Source File

SOURCE=.\ai_dog.h
# End Source File
# Begin Source File

SOURCE=.\ai_dog_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_punk.h
# End Source File
# Begin Source File

SOURCE=.\ai_punk_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_runt.h
# End Source File
# Begin Source File

SOURCE=.\ai_runt_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_shorty.h
# End Source File
# Begin Source File

SOURCE=.\ai_shorty_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_thug.h
# End Source File
# Begin Source File

SOURCE=.\ai_thug2.h
# End Source File
# Begin Source File

SOURCE=.\ai_thug2_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_thug_tables.h
# End Source File
# Begin Source File

SOURCE=.\ai_whore.h
# End Source File
# Begin Source File

SOURCE=.\ai_whore_tables.h
# End Source File
# Begin Source File

SOURCE=.\ep_all.h
# End Source File
# Begin Source File

SOURCE=.\ep_log.h
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\g_ai.h
# End Source File
# Begin Source File

SOURCE=.\g_func.h
# End Source File
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\g_nav.h
# End Source File
# Begin Source File

SOURCE=.\g_teamplay.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\gslog.h
# End Source File
# Begin Source File

SOURCE=.\m_player.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\stdlog.h
# End Source File
# Begin Source File

SOURCE=.\veh_defs.h
# End Source File
# Begin Source File

SOURCE=.\vehicles.h
# End Source File
# Begin Source File

SOURCE=.\voice.h
# End Source File
# Begin Source File

SOURCE=.\voice_bitch.h
# End Source File
# Begin Source File

SOURCE=.\voice_punk.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=G:\Kingpin\comp\history.txt
# End Source File
# End Target
# End Project
