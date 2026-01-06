/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "cg_superhud.h"

#define MODIFSTR_SIZE 256
qboolean modifState[5];
char modifStrDown[5][MODIFSTR_SIZE];
char modifStrUp[5][MODIFSTR_SIZE];


void CG_TargetCommand_f(void)
{
	int     targetNum;
	char    test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum)
	{
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendConsoleCommand(va("gc %i %i", targetNum, atoi(test)));
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer + 10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f(void)
{
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer - 10)));
}

/*
=================
CG_NextCrosshair_f

Keybinding command
=================
*/
static void CG_NextCrosshair_f(void)
{
	int curr = cg_drawCrosshair.integer + 1;

	if (curr < cgs.media.numberOfCrosshairs)
	{
		trap_Cvar_Set("cg_drawCrosshair", va("%i", curr));
	}

}

/*
=================
CG_PrefCrosshair_f

Keybinding command
=================
*/
static void CG_PrevCrosshair_f(void)
{
	int curr = cg_drawCrosshair.integer - 1;

	if (curr >= 0)
	{
		trap_Cvar_Set("cg_drawCrosshair", va("%i", curr));
	}
}

/*
=================
CG_NextCrosshairDecor_f

Keybinding command
=================
*/
static void CG_NextCrosshairDecor_f(void)
{
	int curr = ch_crosshairDecor.integer + 1;

	if (curr < cgs.media.numberOfCrosshairDecors)
	{
		trap_Cvar_Set("ch_crosshairDecor", va("%i", curr));
	}

}

/*
=================
CG_PrefCrosshairDecor_f

Keybinding command
=================
*/
static void CG_PrevCrosshairDecor_f(void)
{
	int curr = ch_crosshairDecor.integer - 1;

	if (curr >= 0)
	{
		trap_Cvar_Set("ch_crosshairDecor", va("%i", curr));
	}
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f(void)
{
	CG_Printf("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
	          (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
	          (int)cg.refdefViewAngles[YAW]);
}

/*
=============
CG_PMType_f

Debugging command to print the current pm_type
=============
*/
static void CG_PMType_f(void)
{
	int pmType;
	const char* pmTypeStr;

	pmType = cg.predictedPlayerState.pm_type;

	switch (pmType)
	{
		case PM_NORMAL:
			pmTypeStr = "PM_NORMAL";
			break;
		case PM_NOCLIP:
			pmTypeStr = "PM_NOCLIP";
			break;
		case PM_SPECTATOR:
			pmTypeStr = "PM_SPECTATOR";
			break;
		case PM_DEAD:
			pmTypeStr = "PM_DEAD";
			break;
		case PM_FREEZE:
			pmTypeStr = "PM_FREEZE";
			break;
		case PM_INTERMISSION:
			pmTypeStr = "PM_INTERMISSION";
			break;
		case PM_SPINTERMISSION:
			pmTypeStr = "PM_SPINTERMISSION";
			break;
		default:
			pmTypeStr = "UNKNOWN";
			break;
	}

	CG_Printf("pm_type: %i (%s)\n", pmType, pmTypeStr);
	if (cg.snap)
	{
		CG_Printf("snap->ps.pm_type: %i\n", cg.snap->ps.pm_type);
	}
}


void CG_ScoresDown_f(void)
{
	if (!cg.demoPlayback && cg.scoresRequestTime < cg.time)
	{
		cg.scoresRequestTime = cg.time + 2000;
		cg.realNumClients = CG_CountRealClients();
		trap_SendClientCommand("score");
		if (!cg.showScores)
		{
			cg.numScores = 0;
		}
	}
	if (cg_drawAccuracy.integer)
		if (!cg.demoPlayback && cg.statsRequestTime < cg.time)
		{
			cg.statsRequestTime = cg.time + 2250;
			trap_SendClientCommand("getstatsinfo");
		}
	cg.showScores = qtrue;
	cg.showAccuracy = qtrue;
}

static void CG_ScoresUp_f(void)
{
	if (cg.showScores)
	{
		cg.showScores = qfalse;
		cg.showAccuracy = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}



static void CG_TellTarget_f(void)
{
	int     clientNum;
	char    command[128];
	char    message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void)
{
	int     clientNum;
	char    command[128];
	char    message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f(void)
{
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if (!atoi(var))
	{
		return;
	}
	if (cg_cameraOrbit.value != 0)
	{
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	}
	else
	{
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

void CG_OSPAddStr_f(void)
{
	char new_value[256];
	char var_name[256];
	char var_value[256];
	int arg_c;
	arg_c = trap_Argc();
	if (arg_c < 3) return;

	trap_Argv(1, var_name, 256);
	trap_Cvar_VariableStringBuffer(var_name, var_value, 256);

	trap_Argv(2, new_value, 256);
	trap_Cvar_VariableStringBuffer(new_value, new_value, 256);

	Q_strcat(var_value, 256, new_value);
	trap_SendConsoleCommand(va("%s \"%s\";", var_name, var_value));
}

void CG_OSPVStrDown_f(void)
{
	char vstr[256];
	trap_Argv(1, vstr, 256);
	trap_SendConsoleCommand(va("vstr %s;", vstr));
}

void CG_OSPVStrUp_f(void)
{
	char vstr[256];
	trap_Argv(2, vstr, 256);
	trap_SendConsoleCommand(va("vstr %s;", vstr));
}

void CG_OSPFireDown_f(void)
{
	int arg_index = 1;
	int weapon_number;
	qboolean done = qfalse;

	if (cg.snap == NULL || cg.snap->ps.pm_flags & PMF_FOLLOW) return;

	while (!done)
	{
		weapon_number = atoi(CG_Argv(arg_index));
		if (weapon_number == 0) break;
		if ((weapon_number > 0) && (weapon_number < 11)
		        && (cg.snap->ps.stats[STAT_WEAPONS] & (1 << weapon_number))
		        && (cg.snap->ps.ammo[weapon_number])
		   )
		{
			trap_SendConsoleCommand(va("weapon %i; wait 2; +attack;", weapon_number));
			done = qtrue;
		}
	}
}

void CG_OSPFireUp_f(void)
{
	trap_SendConsoleCommand("-attack;");
}

void CG_OSPSelect_f(void)
{
	int arg_index = 1;
	int weapon_number;
	qboolean done = qfalse;

	if (cg.snap == NULL || cg.snap->ps.pm_flags & PMF_FOLLOW) return;

	while (!done)
	{
		weapon_number = atoi(CG_Argv(arg_index));
		if (weapon_number == 0) break;
		if ((weapon_number > 0) && (weapon_number < 11)
		        && (cg.snap->ps.stats[STAT_WEAPONS] & (1 << weapon_number))
		        && (cg.snap->ps.ammo[weapon_number])
		   )
		{
			trap_SendConsoleCommand(va("weapon %i;", weapon_number));
			done = qtrue;
		}
	}

}

void CG_OSPServerVersion_f(void)
{
	char* ptr;
	char* pos;
	char* pack;
	char buf[1024];


	CG_Printf("\n^3OSP Server version: ^5\"^7%s^5\"\n\n", Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "gameversion"));

	ptr = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "sv_referencedPakNames");
	CG_Printf("^3Referenced .pk3s:\n");

	if (!ptr)
	{
		CG_Printf("   ^7NONE found\n\n");
	}
	else
	{
		Q_strncpyz(buf, ptr, 1024);
		pos = buf;

		while (pos != 0)
		{
			pack = pos;
			pos = strchr(pos, ' ');
			if (pos == NULL) return;
			*pos = 0;
			CG_Printf("   ^7%s\n", pack);

			++pos;

			while (*pos == ' ')
			{
				++pos;
			}
		}
		CG_Printf("\n");
	}
}

void CG_OSPHUDStyle_f(void)
{
}

void CG_OSPHUDNext_f(void)
{
}

void CG_OSPHUDprev_f(void)
{
}

void CG_OSPClientVersion_f(void)
{
	CG_Printf("^3OSP2 Client Version:^7 %s\n", OSP_VERSION);
}

#define CG_YES_NO_STR(VAL) ((VAL) ? "^2Yes" : "^1No")
#define CG_LOL(VAL) ((VAL) ? "^2Yes" : "^1No, ^2but who cares")
#define CG_LOL_R(VAL) ((VAL) ? "^1No, ^2but who cares" : "^2Yes")

void CG_OSPClientConfig_f(void)
{
	const char* physics = "VQ3";

	if (cgs.osp.server_mode & 1)
	{
		physics = "Promode";
	}
	else if (cgs.osp.server_mode & 2)
	{
		physics = "CQ3";
	}


	CG_Printf("^5OSP2 Server-forced settings:\n", OSP_VERSION);
	CG_Printf("    ^3Physics:                     ^2%s\n", physics);
	CG_Printf("    ^3Alternative weapons:         %s\n", CG_YES_NO_STR(cgs.osp.custom_client & OSP_CUSTOM_CLIENT_ALT_WEAPON_FLAG));
	CG_Printf("    ^3Timer(deprecated):           %s\n", CG_YES_NO_STR(cgs.osp.custom_client & OSP_CUSTOM_CLIENT_TIMER_FLAG));
	CG_Printf("    ^3FPS restriction(deprecated): %s\n", CG_YES_NO_STR((cgs.osp.custom_client & OSP_CUSTOM_CLIENT_MAXFPS_FLAG) == 0));
	// CG_Printf("    ^3Damage info:                 %s\n", CG_YES_NO_STR(cgs.osp.custom_client_2 & OSP_CUSTOM_CLIENT_2_ENABLE_DMG_INFO));
	CG_Printf("    ^3Damage info:                 %s\n", CG_LOL(cgs.osp.custom_client_2 & OSP_CUSTOM_CLIENT_2_ENABLE_DMG_INFO));
	CG_Printf("    ^3Pmove allowed:               %s\n", CG_YES_NO_STR(cgs.osp.allow_pmove));
	CG_Printf("    ^3Timenudge minimum:           ^3%s\n", cgs.osp.serverConfigMinimumTimenudge ? va("%i", cgs.osp.serverConfigMinimumTimenudge) : "-");
	CG_Printf("    ^3Timenudge maximum:           ^3%s\n", cgs.osp.serverConfigMaximumTimenudge ? va("%i", cgs.osp.serverConfigMaximumTimenudge) : "-");
	CG_Printf("    ^3Maxpackets minimum:          ^3%s\n", cgs.osp.serverConfigMinimumMaxpackets ? va("%i", cgs.osp.serverConfigMinimumMaxpackets) : "-");
	CG_Printf("    ^3Maxpackets maximum:          ^3%s\n", cgs.osp.serverConfigMaximumMaxpackets ? va("%i", cgs.osp.serverConfigMinimumMaxpackets) : "-");
	CG_Printf("    ^3OSP2-BE ^3Supported server:    %s\n", CG_YES_NO_STR(cgs.be.supportedServer));
	CG_Printf("    ^3HitBox (XQ3E):               %s\n", CG_YES_NO_STR(cgs.osp.serverConfigXHitBox));
	// CG_Printf("    ^3FriendsWallhack (BE):        %s\n", CG_LOL_R(cgs.be.disableFeatures));

}

void CG_OSPDrawNewCredits_f(void)
{
	if (!cgs.be.showCredits)
	{
		cgs.be.showCredits = qtrue;
	}
	else
	{
		cgs.be.showCredits = qfalse;
	}
}
void CG_OSPMoTD_f(void)
{
	int i;
	if (cgs.osp.numberOfStringsMotd > 0)
	{
		CG_Printf("\n^5Message of the Day:\n");
		for (i = 0; i < cgs.osp.numberOfStringsMotd; ++i)
		{
			CG_Printf("^7%s\n", cgs.osp.motd[i]);
		}
		CG_Printf("\n");
	}
	else
	{
		CG_Printf("\n^3No MOTD set.\n\n");
	}
}
void CG_OSPMyName_f(void)
{
	CG_Printf("Your name is: '%s^7'\n", cgs.clientinfo[ cg.clientNum ].name);
}

void CG_OSPWStatsDown_f(void)
{
	if (cg.statsRequestTime + 1000 < cg.time)
	{
		cg.statsRequestTime = cg.time;
		trap_SendClientCommand("getstatsinfo");
	}
	cgs.be.newStats.drawWindow = qtrue;

}
void CG_OSPWStatsUp_f(void)
{
	cgs.be.newStats.drawWindow = qfalse;
	CG_BEStatsResetInit();
}

void CG_ShudChatDown_f(void)
{
	cgs.osp.shud.forceChat = qtrue;
}

void CG_ShudChatUp_f(void)
{
	cgs.osp.shud.forceChat = qfalse;
}

void CG_ShudKey1Down_f(void)
{
	cgs.osp.shud.key[0] = qtrue;
}

void CG_ShudKey1Up_f(void)
{
	cgs.osp.shud.key[0] = qfalse;
}

void CG_ShudKey2Down_f(void)
{
	cgs.osp.shud.key[1] = qtrue;
}

void CG_ShudKey2Up_f(void)
{
	cgs.osp.shud.key[1] = qfalse;
}

void CG_ShudKey3Down_f(void)
{
	cgs.osp.shud.key[2] = qtrue;
}

void CG_ShudKey3Up_f(void)
{
	cgs.osp.shud.key[2] = qfalse;
}

void CG_ShudKey4Down_f(void)
{
	cgs.osp.shud.key[3] = qtrue;
}

void CG_ShudKey4Up_f(void)
{
	cgs.osp.shud.key[3] = qfalse;
}

void CG_ChudKey1Down_f(void)
{
	cgs.osp.chud.key[0] = qtrue;
}

void CG_ChudKey1Up_f(void)
{
	cgs.osp.chud.key[0] = qfalse;
}

void CG_ChudKey2Down_f(void)
{
	cgs.osp.chud.key[1] = qtrue;
}

void CG_ChudKey2Up_f(void)
{
	cgs.osp.chud.key[1] = qfalse;
}

void CG_ChudKey3Down_f(void)
{
	cgs.osp.chud.key[2] = qtrue;
}

void CG_ChudKey3Up_f(void)
{
	cgs.osp.chud.key[2] = qfalse;
}

void CG_ChudKey4Down_f(void)
{
	cgs.osp.chud.key[3] = qtrue;
}

void CG_ChudKey4Up_f(void)
{
	cgs.osp.chud.key[3] = qfalse;
}

void CG_ChudScoreboardUp_f(void)
{
	cgs.osp.chud.scoreboard = qtrue;

	// Send commands to server to get scoreboard data (copied from bescores)
	if (!cg.demoPlayback && cg.scoresRequestTime < cg.time)
	{
		cg.scoresRequestTime = cg.time + 2000;
		cg.realNumClients = CG_CountRealClients();
		trap_SendClientCommand("score");
	}
	if (!cg.demoPlayback && cg.statsAllRequestTime < cg.time)
	{
		cg.statsAllRequestTime = cg.time + 2100;
		trap_SendClientCommand("statsall");
		cgs.be.statsAllRequested = qtrue;
	}
}

void CG_ChudScoreboardDown_f(void)
{
	if (cgs.osp.chud.scoreboard)
	{
		cgs.osp.chud.scoreboard = qfalse;
	}
	if ((cg.statsAllRequestTime + 500 < cg.time) && cgs.be.statsAllRequested)
	{
		cgs.be.statsAllRequested = qfalse;
	}
}

void CG_OSPPrintTime_f(void)
{
	qtime_t qtime;
	trap_RealTime(&qtime);
	CG_Printf("\n^9Current time: ^7%02d:%02d:%02d (%02d %s %d)\n\n", qtime.tm_hour, qtime.tm_min, qtime.tm_sec, qtime.tm_mday, monthName[qtime.tm_mon], qtime.tm_year + 1900);
}


void CG_OSPModifDown(int layout)
{
	if (!modifState[layout] && trap_Argc() >= 2 && atoi(CG_Argv(1)))
	{
		if (modifStrDown[layout][0])
		{
			trap_SendConsoleCommand(&modifStrDown[layout][0]);
		}
		else
		{
			CG_Printf("^3*** %s not set.\n", CG_Argv(0));
		}
		modifState[layout] = qtrue;
	}
	else if (trap_Argc() >= 2 && !atoi(CG_Argv(1)))
	{
		if (Q_stricmp(CG_Argv(1), "?") == 0)
		{
			if (modifStrDown[layout][0])
			{
				CG_Printf("^5%s is set to: ^7%s\n", CG_Argv(0), modifStrDown[layout][0]);
			}
			else
			{
				CG_Printf("^3*** %s not set.\n", CG_Argv(0));
			}
		}
		else
		{
			char msg[256];
			trap_Args(&modifStrDown[layout][0], MODIFSTR_SIZE);
			trap_Args(&msg[0], MODIFSTR_SIZE);
		}
	}
}

void CG_OSPModifUp(int layout)
{
	if (modifState[layout] && trap_Argc() >= 2 && atoi(CG_Argv(1)))
	{
		if (modifStrUp[layout][0])
		{
			trap_SendConsoleCommand(&modifStrUp[layout][0]);
		}
		else
		{
			CG_Printf("^3*** %s not set.\n", CG_Argv(0));
		}
		modifState[layout] = qfalse;
	}
	else if (trap_Argc() >= 2 && !atoi(CG_Argv(1)))
	{
		if (Q_stricmp(CG_Argv(1), "?") == 0)
		{
			if (modifStrUp[layout][0])
			{
				CG_Printf("^5%s is set to: ^7%s\n", CG_Argv(0), modifStrUp[layout][0]);
			}
			else
			{
				CG_Printf("^3*** %s not set.\n", CG_Argv(0));
			}
		}
		else
		{
			trap_Args(&modifStrUp[layout][0], MODIFSTR_SIZE);
		}
	}
}


void CG_OSPModif1Down_f(void)
{
	CG_OSPModifDown(0);
}

void CG_OSPModif1Up_f(void)
{
	CG_OSPModifUp(0);
}

void CG_OSPModif2Down_f(void)
{
	CG_OSPModifDown(1);
}

void CG_OSPModif2Up_f(void)
{
	CG_OSPModifUp(1);
}

void CG_OSPModif3Down_f(void)
{
	CG_OSPModifDown(2);
}

void CG_OSPModif3Up_f(void)
{
	CG_OSPModifUp(2);
}

void CG_OSPModif4Down_f(void)
{
	CG_OSPModifDown(3);
}

void CG_OSPModif4Up_f(void)
{
	CG_OSPModifUp(3);
}

void CG_OSPModif5Down_f(void)
{
	CG_OSPModifDown(4);
}

void CG_OSPModif5Up_f(void)
{
	CG_OSPModifUp(4);
}

void CG_OSPActionDown_f(void)
{
}

void CG_OSPActionUp_f(void)
{
}

void CG_OSPAddPos_f(void)
{
	if (trap_Argc() < 2)
	{
		CG_Printf("usage: %s <text>\n", CG_Argv(0));
	}
	else
	{
		char name[MAX_QPATH];
		trap_Args(name, MAX_QPATH);
		CG_CustomLocationsAddEntry(cg.refdef.vieworg, name);
	}
}

void CG_OSPDecalAdd_f(void)
{
}

void CG_OSPDecalDec_f(void)
{
}

void CG_OSPDecalDisable_f(void)
{
}

void CG_OSPDecalDump_f(void)
{
}

void CG_OSPDecalEdit_f(void)
{
}

void CG_OSPDecalEnable_f(void)
{
}

void CG_OSPDecalGFXNext_f(void)
{
}

void CG_OSPDecalGFXPrev_f(void)
{
}

void CG_OSPDecalInc_f(void)
{
}

void CG_OSPDcalNext_f(void)
{
}

void CG_OSPDcalPrev_f(void)
{
}

void CG_OSPDecalRotClock_f(void)
{
}

void CG_OSPDecalRotCounter_f(void)
{
}

void CG_OSPDynamicMem_f(void)
{
	CG_DynamicMemReport();
}

void CG_ReloadHud_f(void)
{
	CG_SHUDLoadConfig();
}

void CG_ReloadCHUD_f(void)
{
	CG_CHUDLoadConfig();
}

void CG_PrintPlayerIDs_f(void)
{
	int p;
	CG_Printf("^7| ^3%2s ^7| ^1%4s ^7| ^5%32s | ^5%s\n", "ID", "XID", "Player name", "Player name raw");
	CG_Printf("^7------------------------------------------------------------------------------\n");
	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (cgs.clientinfo[p].infoValid)
		{
			CG_Printf("^7| ^3%2d ^7| ^1%4s ^7| %32s | %s\n", p, cgs.clientinfo[p].xidStr, cgs.clientinfo[p].name_clean, cgs.clientinfo[p].name_codes);
		}
	}
}

void CG_TeamTaskClients_f(void)
{
	int p;
	CG_Printf("\n^5Team Task Support Status:\n");
	CG_Printf("^7| ^3%2s ^7| ^5%32s ^7| ^3%10s\n", "ID", "Player name", "Supports TT");
	CG_Printf("^7------------------------------------------------------------------------------\n");
	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (cgs.clientinfo[p].infoValid)
		{
			CG_Printf("^7| ^3%2d ^7| %32s ^7| %s\n", p, cgs.clientinfo[p].name_clean, 
				cgs.clientinfo[p].supportsTeamTask ? "^2Yes" : "^1No");
		}
	}
	CG_Printf("\n");
}

void CG_Fonts_f(void)
{
	int i;
	int fontCount;
	const char* fontName;

	fontCount = CG_GetFontCount();
	CG_Printf("^7Registered fonts:\n");
	for (i = 0; i < fontCount; i++)
	{
		fontName = CG_GetFontName(i);
		if (fontName)
		{
			CG_Printf("^3%2d^7. %s\n", i, fontName);
		}
	}
}


void CG_Mute_f(void)
{
	int p;

	if (trap_Argc() < 2)
	{
		CG_ChatfilterDump();
		CG_Printf("\nusage: %s <player ID>\n", CG_Argv(0));
		return;
	}

	p = atoi(CG_Argv(1));
	if (p < MAX_CLIENTS && cgs.clientinfo[p].infoValid)
	{
		CG_ChatfilterAddName(cgs.clientinfo[p].name_original);
		CG_ChatfilterSaveFile(CG_CHATFILTER_DEFAULT_FILE);
		CG_Printf("\n^1Player %s is muted\n", cgs.clientinfo[p].name_clean);
	}
	else
	{
		CG_Printf("\n^1Wrong player ID\n");
	}
}

void CG_UnMute_f(void)
{
	int p;

	if (trap_Argc() < 2)
	{
		CG_ChatfilterDump();
		CG_Printf("\nusage: %s <player ID>\n", CG_Argv(0));
		return;
	}

	p = atoi(CG_Argv(1));
	if (p < MAX_CLIENTS && cgs.clientinfo[p].infoValid)
	{
		CG_ChatfilterDeleteName(cgs.clientinfo[p].name_original);
		CG_ChatfilterSaveFile(CG_CHATFILTER_DEFAULT_FILE);
		CG_Printf("\n^2Player %s is unmuted\n", cgs.clientinfo[p].name_clean);
	}
	else
	{
		CG_Printf("\n^1Wrong player ID\n");
	}
}

void CG_MuteTasks_f(void)
{
	int p;

	if (trap_Argc() < 2)
	{
		CG_TeamTaskFilterDump();
		CG_Printf("\nusage: %s <player ID>\n", CG_Argv(0));
		return;
	}

	p = atoi(CG_Argv(1));
	if (p < MAX_CLIENTS && cgs.clientinfo[p].infoValid)
	{
		CG_TeamTaskFilterAddName(cgs.clientinfo[p].name_original);
		CG_TeamTaskFilterSaveFile(CG_TEAMTASKFILTER_DEFAULT_FILE);
		CG_Printf("\n^1Player %s team tasks are muted\n", cgs.clientinfo[p].name_clean);
	}
	else
	{
		CG_Printf("\n^1Wrong player ID\n");
	}
}

void CG_UnMuteTasks_f(void)
{
	int p;

	if (trap_Argc() < 2)
	{
		CG_TeamTaskFilterDump();
		CG_Printf("\nusage: %s <player ID>\n", CG_Argv(0));
		return;
	}

	p = atoi(CG_Argv(1));
	if (p < MAX_CLIENTS && cgs.clientinfo[p].infoValid)
	{
		CG_TeamTaskFilterDeleteName(cgs.clientinfo[p].name_original);
		CG_TeamTaskFilterSaveFile(CG_TEAMTASKFILTER_DEFAULT_FILE);
		CG_Printf("\n^2Player %s team tasks are unmuted\n", cgs.clientinfo[p].name_clean);
	}
	else
	{
		CG_Printf("\n^1Wrong player ID\n");
	}
}

void CG_ShowFont_f(void)
{
	const char* arg;
	int fontIndex;
	const char* fontName;

	if (trap_Argc() < 2)
	{
		cgs.osp.testFont[0] = 0;
		return;
	}

	arg = CG_Argv(1);
	fontIndex = CG_FontIndexFromString(arg);
	fontName = CG_GetFontName(fontIndex);

	if (fontName)
	{
		Com_sprintf(cgs.osp.testFont, MAX_QPATH, "%s", fontName);
	}
	else
	{
		/* Fallback: use argument as font name */
		Com_sprintf(cgs.osp.testFont, MAX_QPATH, "%s", arg);
	}
}


void CG_SendTeamTaskMark(int toClient, int templateID)
{
	int packed;
	int x;
	int y;
	int z;
	trace_t trace;
	vec3_t start;
	vec3_t end;

	if (!cg_teamTask.integer)
	{
		return;
	}
	if (cgs.gametype < GT_TEAM)
	{
		CG_PrintOnce("^3There are no teams\n");
		return;
	}
	if (!CG_IsTeamTaskSupported())
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	if (!CG_IsLocalClientOnScreen())
	{
		return;
	}
	if (CG_IsDead())
	{
		return;
	}
	/* Check if we are in spectator/following mode */
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}
	/* Check if local player is spectator */
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	/* Anti-flood check 1: fast limit (not more than once per 150ms) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFast, cg.time))
	{
		return; /* anti-flood: too many commands sent in short time */
	}
	/* Anti-flood check 2: bucket limit (6 tokens, 800ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucket, cg.time))
	{
		return; /* anti-flood: bucket limit exceeded */
	}
	/* Anti-flood check 3: flood protection (16 tokens, 1100ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFlood, cg.time))
	{
		return; /* anti-flood: flood protection activated */
	}

	/* Use view direction for positioning (like original) */
	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.snap->ps.clientNum, CONTENTS_SOLID);

	x = Quantize(trace.endpos[0], -3000, 3000, 7);
	y = Quantize(trace.endpos[1], -3000, 3000, 7);
	z = Quantize(trace.endpos[2], -3000, 3000, 7);

	/* Static mark at trace position */
	packed = CG_OSPTeamTaskPack(0, templateID, x, y, z);
	trap_SendClientCommand(va("teamtask %i", packed));
	trap_SendClientCommand("teamtask 0");
}

void CG_SendTeamTaskMarkOverhead(int toClient, int templateID)
{
	int packed;
	int x;
	int y;
	int z;
	vec3_t playerPos;

	if (!cg_teamTask.integer)
	{
		return;
	}
	if (cgs.gametype < GT_TEAM)
	{
		CG_PrintOnce("^3There are no teams\n");
		return;
	}
	if (!CG_IsTeamTaskSupported())
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	if (!CG_IsLocalClientOnScreen())
	{
		return;
	}
	if (CG_IsDead())
	{
		return;
	}
	/* Check if we are in spectator/following mode */
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}
	/* Check if local player is spectator */
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	/* Anti-flood check 1: fast limit (not more than once per 150ms) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFast, cg.time))
	{
		return; /* anti-flood: too many commands sent in short time */
	}
	/* Anti-flood check 2: bucket limit (6 tokens, 800ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucket, cg.time))
	{
		return; /* anti-flood: bucket limit exceeded */
	}
	/* Anti-flood check 3: flood protection (16 tokens, 1100ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFlood, cg.time))
	{
		return; /* anti-flood: flood protection activated */
	}

	/* Get local player position and add offset above head */
	VectorCopy(cg.predictedPlayerState.origin, playerPos);
	playerPos[2] += 48.0f; /* Offset above head */

	x = Quantize(playerPos[0], -3000, 3000, 7);
	y = Quantize(playerPos[1], -3000, 3000, 7);
	z = Quantize(playerPos[2], -3000, 3000, 7);

	/* Send to all (0 = broadcast) - templateID is already correct (YES/NO) */
	packed = CG_OSPTeamTaskPack(toClient, templateID, x, y, z);
	trap_SendClientCommand(va("teamtask %i", packed));
	trap_SendClientCommand("teamtask 0");
}

void CG_SendTeamTaskMarkAtPosition(int toClient, int templateID, vec3_t position, qboolean ignoreDead)
{
	int packed;
	int x;
	int y;
	int z;

	if (!cg_teamTask.integer)
	{
		return;
	}
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}
	if (!CG_IsTeamTaskSupported())
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	if (!ignoreDead)
	{
		if (!CG_IsLocalClientOnScreen())
		{
			return;
		}
	}
	if (!ignoreDead && CG_IsDead())
	{
		return;
	}
	/* Anti-flood check 1: fast limit (not more than once per 150ms) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFast, cg.time))
	{
		return; /* anti-flood: too many commands sent in short time */
	}
	/* Anti-flood check 2: bucket limit (6 tokens, 800ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucket, cg.time))
	{
		return; /* anti-flood: bucket limit exceeded */
	}
	/* Anti-flood check 3: flood protection (16 tokens, 1100ms refill) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketFlood, cg.time))
	{
		return; /* anti-flood: flood protection activated */
	}

	x = Quantize(position[0], -3000, 3000, 7);
	y = Quantize(position[1], -3000, 3000, 7);
	z = Quantize(position[2], -3000, 3000, 7);

	packed = CG_OSPTeamTaskPack(toClient, templateID, x, y, z);
	trap_SendClientCommand(va("teamtask %i", packed));
	trap_SendClientCommand("teamtask 0");
}

void CG_TeamTaskMarkOffense(void)
{
	CG_SendTeamTaskMark(0, TEAMTASK_OFFENSE);
}

void CG_TeamTaskMarkDefence(void)
{
	CG_SendTeamTaskMark(0, TEAMTASK_DEFENCE);
}

void CG_TeamTaskMarkWatch(void)
{
	CG_SendTeamTaskMark(0, TEAMTASK_PATROL); /* TEAMTASK_PATROL is used for watch */
}

void CG_TeamTaskMarkYes(void)
{
	CG_SendTeamTaskMarkOverhead(0, TEAMTASK_YES);
}

void CG_TeamTaskMarkNo(void)
{
	CG_SendTeamTaskMarkOverhead(0, TEAMTASK_NO);
}

void CG_TeamTaskMarkMedic(void)
{
	CG_SendTeamTaskMarkOverhead(0, TEAMTASK_MEDIC);
}

void CG_TeamTaskMarkLocation(void)
{
	CG_SendTeamTaskMark(0, TEAMTASK_LOCATION);
}

void CG_TeamTaskMarkHere(void)
{
	int i;
	centity_t* cent;
	vec3_t backupPosition;
	qboolean backUpPositionValid;
	int timeRemaining;

	if (cgs.gametype < GT_TEAM)
	{
		CG_PrintOnce("^3There are no teams\n");
		return;
	}
	/* Check cooldown for tHere command (2 seconds) */
	if (!TokenBucket_Consume(&cgs.be.teamMarkers.sendCommandBucketHere, cg.time))
	{
		return;
	}

	/* Find frozen entity that belongs to local player */
	for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
	{
		cent = &cg_entities[i];
		if (cent->currentState.eType == ET_PLAYER &&
		        cent->currentState.clientNum == cg.clientNum &&
		        cent->currentValid &&
		        CG_IsFrozenEntity(cent))
		{
			/* Found our frozen entity - copy coordinates */
			VectorCopy(cent->lerpOrigin, backupPosition);
			backupPosition[2] += 24.0f;
			backUpPositionValid = qtrue;
			break;
		}
	}

	if (CG_IsDead() || !CG_IsLocalClientOnScreen())
	{
		if (cgs.be.teamMarkers.deathPositionValid)
		{
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_HERE_DEAD, cgs.be.teamMarkers.deathPosition, qtrue);
		}
		else if (backUpPositionValid)
		{
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_HERE_DEAD, backupPosition, qtrue);
		}
		else
		{
			/* Player is dead/spectating but no death position available */
			CG_Printf("^3No death position available\n");
		}
		return;
	}


	/* Check if we're currently alive */
	if (!CG_IsDead() && cg.predictedPlayerState.pm_type != PM_SPECTATOR && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		/* Player is alive - send HERE_ALIVE with current position */
		CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_HERE_ALIVE, cg.predictedPlayerState.origin, qfalse);
	}
	else
	{
		if (cgs.be.teamMarkers.deathPositionValid)
		{
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_HERE_DEAD, cgs.be.teamMarkers.deathPosition, qtrue);
		}
		else if (backUpPositionValid)
		{
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_HERE_DEAD, backupPosition, qtrue);
		}
		else
		{
			/* Player is dead/spectating but no death position available */
			CG_Printf("^3No death position available\n");
		}
	}
}

void CG_TeamTaskMarkSend(void)
{
	int targetType;
	int markType;
	trace_t trace;
	vec3_t start;
	vec3_t end;
	vec3_t enemyPos;
	vec3_t itemPos;
	int i;
	centity_t* cent;

	if (!cg_teamTask.integer)
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	if (!CG_IsLocalClientOnScreen())
	{
		return;
	}
	if (CG_IsDead())
	{
		return;
	}

	/* Check if we have a saved target from wheel opening (valid for 500ms) */
	if ((cg_teamTaskWheel.integer & 2) &&
	        cgs.be.teamMarkers.smartTargetTime > 0 &&
	        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
	        cgs.be.teamMarkers.smartTargetType != 1)
	{
		/* Use saved target */
		targetType = cgs.be.teamMarkers.smartTargetType;
		VectorCopy(cgs.be.teamMarkers.smartTargetPos, itemPos);
	}
	else
	{
		/* Check what we're aiming at using CG_CrosshairTarget */
		/* Returns: 1 = nothing, 2 = enemy, 3 = red armor, 4 = megahealth, 5 = frozen ally, 6 = frozen enemy */
		/* If item found, itemPos contains item center position */
		targetType = CG_CrosshairTarget(itemPos);
	}

	switch (targetType)
	{
		case 5: /* frozen ally */
			/* Use saved frozen position if available, otherwise use current itemPos */
			if ((cg_teamTaskWheel.integer & 2) &&
			        cgs.be.teamMarkers.smartTargetTime > 0 &&
			        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
			        cgs.be.teamMarkers.smartTargetType == 5)
			{
				VectorCopy(cgs.be.teamMarkers.smartTargetPos, itemPos);
			}
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_FROZEN_ALLY, itemPos, qfalse);
			return;
		case 6: /* frozen enemy */
			/* Use saved frozen position if available, otherwise use current itemPos */
			if ((cg_teamTaskWheel.integer & 2) &&
			        cgs.be.teamMarkers.smartTargetTime > 0 &&
			        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
			        cgs.be.teamMarkers.smartTargetType == 6)
			{
				VectorCopy(cgs.be.teamMarkers.smartTargetPos, itemPos);
			}
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_FROZEN_ENEMY, itemPos, qfalse);
			return;
		case 2: /* enemy */
		{
			/* Check if we're using saved target */
			if ((cg_teamTaskWheel.integer & 2) &&
			        cgs.be.teamMarkers.smartTargetTime > 0 &&
			        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
			        cgs.be.teamMarkers.smartTargetType == 2)
			{
				/* Use saved enemy position */
				VectorCopy(cgs.be.teamMarkers.smartTargetPos, enemyPos);
				CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_TARGET, enemyPos, qfalse);
				return;
			}

			/* For enemy, get their position and mark at their body (+24 height) */
			VectorCopy(cg.refdef.vieworg, start);
			VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

			CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
			         cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);

			/* Check if we hit a player */
			if (trace.entityNum >= 0 && trace.entityNum < MAX_CLIENTS)
			{
				/* Find player entity and get their position */
				for (i = 0; i < MAX_GENTITIES; i++)
				{
					cent = &cg_entities[i];
					if (cent->currentState.eType == ET_PLAYER &&
					        cent->currentState.clientNum == trace.entityNum &&
					        cent->currentValid)
					{
						/* Use enemy player position with +24 height offset */
						VectorCopy(cent->lerpOrigin, enemyPos);
						enemyPos[2] += 24.0f;
						CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_TARGET, enemyPos, qfalse);
						return;
					}
				}
			}
			/* Fallback: if enemy entity not found, use trace endpos */
			VectorCopy(trace.endpos, enemyPos);
			enemyPos[2] += 24.0f;
			CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_TARGET, enemyPos, qfalse);
			return;
		}
		case 3: /* red armor */
			markType = TEAMTASK_REDARMOR;
			/* Use saved item position if available, otherwise use current itemPos */
			if ((cg_teamTaskWheel.integer & 2) &&
			        cgs.be.teamMarkers.smartTargetTime > 0 &&
			        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
			        cgs.be.teamMarkers.smartTargetType == 3)
			{
				VectorCopy(cgs.be.teamMarkers.smartTargetPos, itemPos);
			}
			CG_SendTeamTaskMarkAtPosition(0, markType, itemPos, qfalse);
			return;
		case 4: /* megahealth */
			markType = TEAMTASK_MEGAHEALTH;
			/* Use saved item position if available, otherwise use current itemPos */
			if ((cg_teamTaskWheel.integer & 2) &&
			        cgs.be.teamMarkers.smartTargetTime > 0 &&
			        (cg.time - cgs.be.teamMarkers.smartTargetTime) < 500 &&
			        cgs.be.teamMarkers.smartTargetType == 4)
			{
				VectorCopy(cgs.be.teamMarkers.smartTargetPos, itemPos);
			}
			CG_SendTeamTaskMarkAtPosition(0, markType, itemPos, qfalse);
			return;
		default: /* nothing or other */
		{
			teamTaskCommand_t* cmd;
			int defaultType;

			// Get default type from cvar (string name)
			cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);

			// Check if command is valid and selectable
			if (cmd && cmd->isSelectable)
			{
				defaultType = cmd->taskType;
			}
			else
			{
				// Invalid name, overhead command, or auto-selected commands
				// Use LOCATION as fallback
				defaultType = TEAMTASK_LOCATION;
			}

			markType = defaultType;
			CG_SendTeamTaskMark(0, markType);
			return;
		}
	}
}


void CG_DPI_f(void)
{
	float local_yaw;
	float local_sens;
	float local_dpi;
	float local_360;
	char var[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2 || trap_Argc() > 4)
	{
		CG_Printf("\n^1Usage:\n");
		CG_Printf("\n^1dpi ^2<dpi_value> ^3[sens] [m_yaw]\n");
		return;
	}

	local_dpi = atof(CG_Argv(1));

	if (trap_Argc() > 2)
	{
		local_sens = atof(CG_Argv(2));
	}
	else
	{
		trap_Cvar_VariableStringBuffer("sensitivity", var, sizeof(var));
		local_sens = atof(var);
	}

	if (trap_Argc() > 3)
	{
		local_yaw = atof(CG_Argv(3));
	}
	else
	{
		trap_Cvar_VariableStringBuffer("m_yaw", var, sizeof(var));
		local_yaw = atof(var);
	}

	local_360 = (360.0f / (local_yaw * local_sens * local_dpi)) * 2.54f;

	CG_Printf("\n^1DPI:          ^2%d\n", (int)local_dpi);
	CG_Printf("^1sensitivity:  ^2%.3f\n", local_sens);
	CG_Printf("^1m_yaw:        ^2%.3f\n", local_yaw);
	CG_Printf("^1cm/360:       ^2%.2f\n", local_360);
}

void CG_BEdisabledFeatures_f()
{
	CG_PrintDisabledFeatures(qtrue);
}


void CG_Stub_f(void) { }

/*
=================
CG_TeamTaskHelp_f

Display help information about team tasks
Usage: teamtaskhelp
=================
*/
void CG_TeamTaskHelp_f(void)
{
	CG_Printf("\n^5=== Team Task Help ===\n\n");

	CG_Printf("^3How to open the wheel menu:\n");
	CG_Printf("  ^2+ttwheel^7 - Open team task selection wheel\n");
	CG_Printf("  ^2-ttwheel^7 - Close wheel and send selected command\n");
	CG_Printf("  ^7Bind a key: ^2bind <key> \"+ttwheel\"^7\n");
	CG_Printf("  ^7Example: ^2bind mouse3 \"+ttwheel\"^7\n\n");

	CG_Printf("^3How to use the wheel:\n");
	CG_Printf("  ^71. Hold the key bound to ^2+ttwheel^7\n");
	CG_Printf("  ^72. Move mouse to select a task icon\n");
	CG_Printf("  ^73. Release the key to send the command\n\n");
	CG_Printf("^3Wheel settings:\n");
	CG_Printf("  ^1Aim lock ^7enabled by default\n");
	CG_Printf("  ^7To disable add to ^3cg_teamTaskWheel ^216^7 (bitmask)\n");
	CG_Printf("  ^7An example: your value is ^24^7 -> need set to ^220^7 (^24^7+^216^7=^220^7)\n\n");
	CG_Printf("^3Available team task commands:\n");
	CG_Printf("  ^2tAttack^7    - Mark attack position\n");
	CG_Printf("  ^2tDefence^7   - Mark defence position\n");
	CG_Printf("  ^2tWatch^7      - Mark watch/patrol position\n");
	CG_Printf("  ^2tLocation^7  - Mark location/position\n");
	CG_Printf("  ^2tYes^7        - Send YES (overhead mark)\n");
	CG_Printf("  ^2tNo^7         - Send NO (overhead mark)\n");
	CG_Printf("  ^2tMedic^7      - Request medic (overhead mark)\n");
	CG_Printf("  ^2tHere^7       - Mark your current position (alive) or death location (dead)\n");
	CG_Printf("  ^2tSmart^7      - Smart mode (auto-detects target)\n");
	CG_Printf("  ^7Smart mode automatically detects:\n");
	CG_Printf("    - Enemy players (marks as target)\n");
	CG_Printf("    - Frozen enemy and ally bodies\n");
	CG_Printf("    - Red armor (marks as redarmor)\n");
	CG_Printf("    - Megahealth (marks as megahealth)\n");
	CG_Printf("    - Nothing (uses default from ^3cg_teamTaskSmartDefault^7)\n\n");

	CG_Printf("^3Muting team tasks:\n");
	CG_Printf("  ^2mutetasks <player ID>^7   - Mute team tasks from a player\n");
	CG_Printf("  ^2unmutetasks <player ID>^7 - Unmute team tasks from a player\n");

	CG_Printf("Cvars for teamTasks starts with ^3cg_teamTask^7\n");
	CG_Printf("^3For more information about a specific cvar:\n");
	CG_Printf("  ^2behelp ^3<cvar_name>^7\n\n");
}

/*
=================
CG_BEHelp_f

Get cvar description
Usage: behelp <cvar_name>
=================
*/
void CG_BEHelp_f(void)
{
	const char* cvarName;
	const char* description;
	char currentValue[MAX_CVAR_VALUE_STRING];
	cvarTable_t* cvarTable;
	const char* defaultValue;

	if (trap_Argc() < 2)
	{
		CG_Printf("^3Usage: behelp <cvar_name>\n");
		return;
	}

	cvarName = CG_Argv(1);

	/* Check if description exists */
	description = CG_GetCvarDescription(cvarName);
	if (description && description[0])
	{
		/* Get current value */
		trap_Cvar_VariableStringBuffer(cvarName, currentValue, sizeof(currentValue));

		/* Get default value */
		cvarTable = CG_GetCgCvarByName(cvarName);
		if (cvarTable && cvarTable->defaultString)
		{
			defaultValue = cvarTable->defaultString;
		}
		else
		{
			defaultValue = "unknown";
		}

		/* Print cvar info */
		CG_Printf("\"%s\" is \"%s\" default:\"%s\"\n", cvarName, currentValue, defaultValue);

		/* Print description */
		CG_Printf("%s\n", description);
	}
	else
	{
		/* No description found, send print command to client */
		trap_SendConsoleCommand(va("print %s", cvarName));
	}
}

/*
=================
CG_ColorCharList_f

Display color codes in alphabetical order
Usage: colorcharlist
=================
*/
static void CG_ColorCharList_f(void)
{
	int i;
	char ch;

	CG_Printf("Color codes:\n");
	for (i = 0; i < 26; i++)
	{
		ch = 'a' + i;
		CG_Printf("^%c%c ", ch, ch);
	}
	CG_Printf("\n");
}

typedef struct
{
	char*    cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] =
{
	{ "hudstyle", CG_OSPHUDStyle_f },
	{ "hudnext", CG_OSPHUDNext_f },
	{ "hudprev", CG_OSPHUDprev_f },
	{ "addstr", CG_OSPAddStr_f },
	{ "+vstr", CG_OSPVStrDown_f },
	{ "-vstr", CG_OSPVStrUp_f },
	{ "+fire", CG_OSPFireDown_f },
	{ "-fire", CG_OSPFireUp_f },
	{ "select", CG_OSPSelect_f },
	{ "clientversion", CG_OSPClientVersion_f },
	{ "clientconfig", CG_OSPClientConfig_f },
	{ "credits", CG_OSPDrawNewCredits_f },
	{ "motd", CG_OSPMoTD_f },
	{ "myname", CG_OSPMyName_f },
	{ "serverversion", CG_OSPServerVersion_f },
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	// { "pmtype", CG_PMType_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+wstats", CG_OSPWStatsDown_f },
	{ "-wstats", CG_OSPWStatsUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "+zoomtoggle", CG_ZoomToggle_f},
	{ "-zoomtoggle", CG_Stub_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "crossnext", CG_NextCrosshair_f },
	{ "crossprev", CG_PrevCrosshair_f },
	{ "crossdecornext", CG_NextCrosshairDecor_f },
	{ "crossdecorprev", CG_PrevCrosshairDecor_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "startOrbit", CG_StartOrbit_f },
	{ "currenttime", CG_OSPPrintTime_f },
	{ "+modif1", CG_OSPModif1Down_f },
	{ "-modif1", CG_OSPModif1Up_f },
	{ "+modif2", CG_OSPModif2Down_f },
	{ "-modif2", CG_OSPModif2Up_f },
	{ "+modif3", CG_OSPModif3Down_f },
	{ "-modif3", CG_OSPModif3Up_f },
	{ "+modif4", CG_OSPModif4Down_f },
	{ "-modif4", CG_OSPModif4Up_f },
	{ "+modif5", CG_OSPModif5Down_f },
	{ "-modif5", CG_OSPModif5Up_f },
	{ "+action", CG_OSPActionDown_f },
	{ "-action", CG_OSPActionUp_f },
	{ "+shudchat", CG_ShudChatDown_f },
	{ "-shudchat", CG_ShudChatUp_f },
	{ "+shudkey1", CG_ShudKey1Down_f },
	{ "-shudkey1", CG_ShudKey1Up_f },
	{ "+shudkey2", CG_ShudKey2Down_f },
	{ "-shudkey2", CG_ShudKey2Up_f },
	{ "+shudkey3", CG_ShudKey3Down_f },
	{ "-shudkey3", CG_ShudKey3Up_f },
	{ "+shudkey4", CG_ShudKey4Down_f },
	{ "-shudkey4", CG_ShudKey4Up_f },
	{ "+chudkey1", CG_ChudKey1Down_f },
	{ "-chudkey1", CG_ChudKey1Up_f },
	{ "+chudkey2", CG_ChudKey2Down_f },
	{ "-chudkey2", CG_ChudKey2Up_f },
	{ "+chudkey3", CG_ChudKey3Down_f },
	{ "-chudkey3", CG_ChudKey3Up_f },
	{ "+chudkey4", CG_ChudKey4Down_f },
	{ "-chudkey4", CG_ChudKey4Up_f },
	{ "+chudscoreboard", CG_ChudScoreboardUp_f },
	{ "-chudscoreboard", CG_ChudScoreboardDown_f },
	{ "cg_dynamicmem", CG_OSPDynamicMem_f },
	{ "addpos", CG_OSPAddPos_f },
	{ "decaladd", CG_OSPDecalAdd_f },
	{ "decaldec", CG_OSPDecalDec_f },
	{ "decaldisable", CG_OSPDecalDisable_f },
	{ "decaldump", CG_OSPDecalDump_f },
	{ "decaledit", CG_OSPDecalEdit_f },
	{ "decalenable", CG_OSPDecalEnable_f },
	{ "decalgfxnext", CG_OSPDecalGFXNext_f },
	{ "decalgfxprev", CG_OSPDecalGFXPrev_f },
	{ "decalinc", CG_OSPDecalInc_f },
	{ "decalnext", CG_OSPDcalNext_f },
	{ "decalprev", CG_OSPDcalPrev_f },
	{ "decalrotclock", CG_OSPDecalRotClock_f },
	{ "decalrotcounter", CG_OSPDecalRotCounter_f },
	{ "reloadHUD", CG_ReloadHud_f },
	{ "reloadCHUD", CG_ReloadCHUD_f },
	{ "playersid", CG_PrintPlayerIDs_f },
	{ "mute", CG_Mute_f },
	{ "unmute", CG_UnMute_f },
	{ "mutetasks", CG_MuteTasks_f },
	{ "unmutetasks", CG_UnMuteTasks_f },
	{ "showfont", CG_ShowFont_f },
	{ "dpi", CG_DPI_f },
	{ "belist", CG_PrintNewCommandsBE_f },
	// { "salist", cg_printsa_f },
	{ "allowedfeatures", CG_BEdisabledFeatures_f },
	{ "fonts", CG_Fonts_f },
	{ "printui", CG_PrintUI_f },
	{ "tAttack", CG_TeamTaskMarkOffense },
	{ "tDefence", CG_TeamTaskMarkDefence },
	{ "tWatch", CG_TeamTaskMarkWatch },
	{ "tYes", CG_TeamTaskMarkYes },
	{ "tNo", CG_TeamTaskMarkNo },
	{ "tMedic", CG_TeamTaskMarkMedic },
	{ "tLocation", CG_TeamTaskMarkLocation },
	{ "tHere", CG_TeamTaskMarkHere },
	{ "tSmart", CG_TeamTaskMarkSend },
	{ "+ttwheel", CG_SelwheelMarkStart },
	{ "-ttwheel", CG_SelwheelMarkStop },
	{ "ttwheelclose", CG_SelwheelMarkClose },
	{ "behelp", CG_BEHelp_f },
	{ "teamTaskHelp", CG_TeamTaskHelp_f },
	{ "teamTaskClients", CG_TeamTaskClients_f },
	{ "colorcharlist", CG_ColorCharList_f },
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand(void)
{
	const char* cmd = CG_Argv(0);
	int i;

	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		if (!Q_stricmp(cmd, commands[i].cmd))
		{
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}



/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands(void)
{
	int     i;

	for (i = 0 ; i < sizeof(commands) / sizeof(commands[0]) ; i++)
	{
		trap_AddCommand(commands[i].cmd);
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand("allready");
	trap_AddCommand("addbot");
	trap_AddCommand("callvote");
	trap_AddCommand("follow");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("levelshot");
	trap_AddCommand("loaddeferred");
	trap_AddCommand("noclip");
	trap_AddCommand("notarget");
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("setviewpos");
	trap_AddCommand("team");
	trap_AddCommand("teamtask");
	trap_AddCommand("teamvote");
	trap_AddCommand("tell");
	trap_AddCommand("vote");
	trap_AddCommand("?");
	trap_AddCommand("acc");
	trap_AddCommand("accuracy");
	trap_AddCommand("autorecord");
	trap_AddCommand("autoscreenshot");
	trap_AddCommand("admin");
	trap_AddCommand("bottomshots");
	trap_AddCommand("captains");
	trap_AddCommand("clientversion");
	trap_AddCommand("coachdecline");
	trap_AddCommand("coachinvite");
	trap_AddCommand("coachkick");
	trap_AddCommand("coach");
	trap_AddCommand("commands");
	trap_AddCommand("cv");
	trap_AddCommand("drop");
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("followpowerup");
	trap_AddCommand("help");
	trap_AddCommand("hudnext");
	trap_AddCommand("hudprev");
	trap_AddCommand("hudstyle");
	trap_AddCommand("invite");
	trap_AddCommand("joincode");
	trap_AddCommand("kickplayer");
	trap_AddCommand("killstats");
	trap_AddCommand("kill");
	trap_AddCommand("lock");
	trap_AddCommand("locations");
	trap_AddCommand("maplist");
	trap_AddCommand("mapload");
	trap_AddCommand("myname");
	trap_AddCommand("notready");
	trap_AddCommand("nowin");
	trap_AddCommand("osp");
	trap_AddCommand("pause");
	trap_AddCommand("pickplayer");
	trap_AddCommand("playerlist");
	trap_AddCommand("playernames");
	trap_AddCommand("players");
	trap_AddCommand("r_allcaptain");
	trap_AddCommand("r_allready");
	trap_AddCommand("r_help");
	trap_AddCommand("r_kick");
	trap_AddCommand("r_lockred");
	trap_AddCommand("r_lockblue");
	trap_AddCommand("r_map");
	trap_AddCommand("r_mpause");
	trap_AddCommand("r_mutespecs");
	trap_AddCommand("r_pause");
	trap_AddCommand("r_promote");
	trap_AddCommand("r_putred");
	trap_AddCommand("r_putblue");
	trap_AddCommand("r_remove");
	trap_AddCommand("r_restart");
	trap_AddCommand("range");
	trap_AddCommand("readyteam");
	trap_AddCommand("ready");
	trap_AddCommand("ref");
	trap_AddCommand("referee");
	trap_AddCommand("remove");
	trap_AddCommand("resign");
	trap_AddCommand("scores");
	trap_AddCommand("settings");
	trap_AddCommand("snapshot");
	trap_AddCommand("snapshotdump");
	trap_AddCommand("snaps");
	trap_AddCommand("specinvite");
	trap_AddCommand("speclock");
	trap_AddCommand("speconly");
	trap_AddCommand("specunlock");
	trap_AddCommand("statsdump");
	trap_AddCommand("statsred");
	trap_AddCommand("statsblue");
	trap_AddCommand("statsall");
	trap_AddCommand("stats");
	trap_AddCommand("teamready");
	trap_AddCommand("time");
	trap_AddCommand("timein");
	trap_AddCommand("timeout");
	trap_AddCommand("topshots");
	trap_AddCommand("unlock");
	trap_AddCommand("unready");
	trap_AddCommand("vc_follow");
	trap_AddCommand("vc_free");
	trap_AddCommand("vc_info");
	trap_AddCommand("vc_proximity");
	trap_AddCommand("viewadd");
	trap_AddCommand("viewall");
	trap_AddCommand("viewblue");
	trap_AddCommand("viewcam");
	trap_AddCommand("viewcyclenext");
	trap_AddCommand("viewcycleprev");
	trap_AddCommand("viewfollow");
	trap_AddCommand("viewlist");
	trap_AddCommand("viewnext");
	trap_AddCommand("viewnone");
	trap_AddCommand("viewprev");
	trap_AddCommand("viewred");
	trap_AddCommand("viewremove");
	trap_AddCommand("decaladd");
	trap_AddCommand("decaldec");
	trap_AddCommand("decaldisable");
	trap_AddCommand("decaldump");
	trap_AddCommand("decaledit");
	trap_AddCommand("decalenable");
	trap_AddCommand("decalgfxnext");
	trap_AddCommand("decalgfxprev");
	trap_AddCommand("decalinc");
	trap_AddCommand("decalnext");
	trap_AddCommand("decalprev");
	trap_AddCommand("decalrotclock");
	trap_AddCommand("decalrotcounter");
	trap_AddCommand("testfont");
	trap_AddCommand("builds");
}
