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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"
#include "../qcommon/qcommon.h"
#include "cg_superhud.h"

static int xstats1_received_count = 0;
static int last_xstats1_sequence = -1;
/* Previous scores for round detection, used only in this file */
static int s_prevScores1;
static int s_prevScores2;

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores(void)
{
	int i, client, base, powerups, index;
	int extraScoreClients[MAX_CLIENTS];
	int extraScoreCount;
	qboolean clientInScores[MAX_CLIENTS];

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		clientInScores[i] = qfalse;
	}

	cg.numScores = atoi(CG_Argv(1));
	if (cg.numScores > MAX_CLIENTS)
	{
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi(CG_Argv(2));
	cg.teamScores[1] = atoi(CG_Argv(3));

	memset(cg.scores, 0, sizeof(cg.scores));

	for (i = 0; i < cg.numScores; i++)
	{
		base = i * 14 + 4;
		client = atoi(CG_Argv(base));
		if (client < 0 || client >= MAX_CLIENTS)
		{
			client = 0;
		}

		cg.scores[i].client = client;
		clientInScores[client] = qtrue;

		cg.scores[i].score = atoi(CG_Argv(base + 1));
		cg.scores[i].ping = atoi(CG_Argv(base + 2));
		cg.scores[i].time = atoi(CG_Argv(base + 3));
		cg.scores[i].scoreFlags = atoi(CG_Argv(base + 4));
		powerups = atoi(CG_Argv(base + 5));
		cg.scores[i].accuracy = atoi(CG_Argv(base + 6));
		cg.scores[i].impressiveCount = atoi(CG_Argv(base + 7));
		cg.scores[i].excellentCount = atoi(CG_Argv(base + 8));
		cg.scores[i].guantletCount = atoi(CG_Argv(base + 9));
		cg.scores[i].defendCount = atoi(CG_Argv(base + 10));
		cg.scores[i].assistCount = atoi(CG_Argv(base + 11));
		cg.scores[i].perfect = atoi(CG_Argv(base + 12));
		cg.scores[i].captures = atoi(CG_Argv(base + 13));

		cgs.clientinfo[client].score = cg.scores[i].score;
		cgs.clientinfo[client].powerups = powerups;
		cg.scores[i].team = cgs.clientinfo[client].team;
	}

	extraScoreCount = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}
		if (!clientInScores[i])
		{
			extraScoreClients[extraScoreCount++] = i;
		}
	}

	for (i = 0; i < extraScoreCount; i++)
	{
		index = cg.numScores + i;
		if (index >= MAX_CLIENTS)
		{
			break;
		}
		client = extraScoreClients[i];
		cg.scores[index].client = client;
		cg.scores[index].team = cgs.clientinfo[client].team;
	}
}

/*
=================
CG_ParseDstatus

Server response to getdstatus: "dstatus id id id ..." — client IDs that are dead in freeze (ctf/tdm unfreeze).
If the first argument is -1, all are alive (no one dead).
=================
*/
static void CG_ParseDstatus(void)
{
	int i, argc, id;

	argc = trap_Argc();
	if (argc >= 2 && atoi(CG_Argv(1)) == -1)
	{
		/* -1 = all alive */
		for (i = 0; i < MAX_CLIENTS; i++)
			cgs.deadInFreeze[i] = qfalse;
		for (i = 0; i < MAX_CLIENTS; i++)
			cg.predictedDeadInFreeze[i] = -1;
		return;
	}
	/* List of dead IDs: clear all, then set only those in the list (revived players become alive) */
	for (i = 0; i < MAX_CLIENTS; i++)
		cgs.deadInFreeze[i] = qfalse;
	for (i = 1; i < argc; i++)
	{
		id = atoi(CG_Argv(i));
		if (id >= 0 && id < MAX_CLIENTS)
			cgs.deadInFreeze[id] = qtrue;
	}
	/* Clear predictions: server state is authoritative */
	for (i = 0; i < MAX_CLIENTS; i++)
		cg.predictedDeadInFreeze[i] = -1;
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo(void)
{
	int     i;
	int     client;
	clientInfo_t* ci;

	numSortedTeamPlayers = atoi(CG_Argv(1));
	if (numSortedTeamPlayers > TEAM_MAXOVERLAY)
	{
		numSortedTeamPlayers = TEAM_MAXOVERLAY;
	}

	for (i = 0 ; i < numSortedTeamPlayers ; i++)
	{
		int oldHealth;
		int oldArmor;

		client = atoi(CG_Argv(i * 6 + 2));

		ci = &cgs.clientinfo[ client ];

		sortedTeamPlayers[i] = client;

		if (customLocationsEnabled)
		{
			CG_CustomLocationsSetLocation(CG_Argv(i * 6 + 3), ci->customLocation);
		}
		oldHealth = ci->health;
		oldArmor = ci->armor;
		ci->location = atoi(CG_Argv(i * 6 + 3));
		ci->health = atoi(CG_Argv(i * 6 + 4));
		ci->armor = atoi(CG_Argv(i * 6 + 5));
		ci->curWeapon = atoi(CG_Argv(i * 6 + 6));
		ci->powerups = atoi(CG_Argv(i * 6 + 7));

		CG_HealthColorFromStats(ci->health, ci->armor, cg.healthColorByClient[client],
		                        &cg.allyHealthBand[client]);

		/* Ally damage blink: -1 on HP or AP at 100+ between tInfo updates is handicap noise */
		{
			int levels;
			int drop;
			qboolean damage;

			levels = cg_healthColorLevels.integer;
			if (levels && !(levels & 4) && client != cg.clientNum)
			{
				damage = qfalse;
				if (ci->health < oldHealth)
				{
					drop = oldHealth - ci->health;
					if (drop > 1 || oldHealth < 100)
					{
						damage = qtrue;
					}
				}
				if (!damage && ci->armor < oldArmor)
				{
					drop = oldArmor - ci->armor;
					if (drop > 1 || oldArmor < 100)
					{
						damage = qtrue;
					}
				}
				if (damage && ci->health > 0)
				{
					cg.healthBlinkUntil[client] = cg.time + CG_HEALTH_BLINK_MSEC;
					cg.allyHealthBlinkMidLow[client] =
					    (qboolean)(cg.allyHealthBand[client] == CG_HEALTH_BAND_LOW);
				}
			}
		}
	}
}


/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo(void)
{
	const char* info;
	const char* val;
	int timelimitVal = 0;

	info = CG_ConfigString(CS_SERVERINFO);
	val = Info_ValueForKey(info, "g_gametype");
	cgs.gametype = atoi(val);
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
	cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
	cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));

	// This is for all OSDF-based mods
	cgs.osdf_signature = atoi( Info_ValueForKey(info, "osdf_signature") );

	// Metarena-specific
	cgs.phy_cpm_crouchslide_time = atoi( Info_ValueForKey(info, "phy_cpm_crouchslide_time") );

	cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
	timelimitVal = atoi(Info_ValueForKey(info, "timelimit"));
	cgs.timelimit = timelimitVal;
	cgs.timelimitMsec = timelimitVal * 60 * 1000;
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	val = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", val);
	val = Info_ValueForKey(info, "g_redTeam");
	Q_strncpyz(cgs.redTeam, val, sizeof(cgs.redTeam));
	trap_Cvar_Set("g_redTeam", cgs.redTeam);
	val = Info_ValueForKey(info, "g_blueTeam");
	Q_strncpyz(cgs.blueTeam, val, sizeof(cgs.blueTeam));
	trap_Cvar_Set("g_blueTeam", cgs.blueTeam);
	// cgs.isOspServer = (Info_ValueForKey(info, "g_unlagged")[0] != '\0') ? qfalse : qtrue;

	/* Parse Score_Time: Started -> 1, Warmup -> 2, else -> 0.
	 * Round warmup is separate from the normal pre-match warmup. */
	val = Info_ValueForKey(info, "Score_Time");
	{
		int newState = 0;

		if (!atoi(CG_ConfigString(CS_WARMUP)))
		{
			if (val[0] == 'S' || val[0] == 's')
			{
				newState = 1;
			}
			else if (val[0] == 'W' || val[0] == 'w')
			{
				newState = 2;
			}
		}

		if (newState != cgs.roundStartState)
		{
			cgs.roundStartState = newState;
		}
	}

	/* CG_SetServerSettingsBE inlined - enableBreath from CS_SYSTEMINFO */
	/* g_enableBreath: disabled only when value is exactly "0" */
	val = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "g_enableBreath");
	cgs.beServerEnableBreath = (val[0] == '\0' || val[0] != '0' || val[1] != '\0') ? 1 : 0;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues(void)
{
	const char* s;

	s = CG_ConfigString(CS_SCORES1);
	cgs.scores1 = atoi(s);
	if (!cgs.fpsclassico)
	{
		cgs.osp.osp_teamcount1 = -1;
		Q_sscanf(s, "%i %i", &cgs.scores1, &cgs.osp.osp_teamcount1);
	}
	s_prevScores1 = cgs.scores1;

	s = CG_ConfigString(CS_SCORES2);
	cgs.scores2 = atoi(s);
	if (!cgs.fpsclassico)
	{
		cgs.osp.osp_teamcount2 = -1;
		Q_sscanf(s, "%i %i", &cgs.scores2, &cgs.osp.osp_teamcount2);
	}
	s_prevScores2 = cgs.scores2;

	if (cgs.fpsclassico)
	{
		cgs.osp.osp_teamcount1 = atoi(CG_ConfigString(CS_REDPLAYERALIVE));
		cgs.osp.osp_teamcount2 = atoi(CG_ConfigString(CS_BLUEPLAYERALIVE));
	}

	cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	if (cgs.gametype == GT_CTF)
	{
		s = CG_ConfigString(CS_FLAGSTATUS);
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}
	cg.warmup = atoi(CG_ConfigString(CS_WARMUP));
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void)
{
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char* o;
	char* n, *t;

	o = CG_ConfigString(CS_SHADERSTATE);
	while (o && *o)
	{
		n = strstr(o, "=");
		if (n && *n)
		{
			strncpy(originalShader, o, n - o);
			originalShader[n - o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t)
			{
				strncpy(newShader, n, t - n);
				newShader[t - n] = 0;
			}
			else
			{
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o)
			{
				strncpy(timeOffset, t, o - t);
				timeOffset[o - t] = 0;
				o++;
				trap_R_RemapShader(originalShader, newShader, timeOffset);
			}
		}
		else
		{
			break;
		}
	}
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified(void)
{
	const char*  str;
	int     num;

	num = atoi(CG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = CG_ConfigString(num);

	// do something with it if necessary
	switch (num)
	{
		case CS_MUSIC:
			CG_StartMusic();
			break;
		case CS_SERVERINFO:
			CG_ParseServerinfo();
			break;
		case CS_SYSTEMINFO:
			break;
		case CS_WARMUP:
		{
			int warmup;

			warmup = atoi(str);
			cg.warmupCount = -1;
			if (warmup == 0 && cg.warmup)
			{
				memset(&cgs.newStats, 0, sizeof(cgs.newStats));
				CG_InitTeamTaskMarkers();
				if (cg_exec_warmupEnded.string[0])
					trap_SendConsoleCommand(cg_exec_warmupEnded.string);
				str = Info_ValueForKey(CG_ConfigString(CS_SERVERINFO), "Score_Time");
				if (str[0] == 'W' || str[0] == 'w')
				{
					cgs.roundStartState = 2;
				}
				else if (str[0] == 'S' || str[0] == 's')
				{
					cgs.roundStartState = 1;
				}
				else
				{
					cgs.roundStartState = 0;
				}
			}
			else if (warmup > 0 && cg.warmup <= 0)
			{
				trap_S_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
				cgs.roundStartState = 0;
			}
			/* if (warmup == 0 && cg.warmup > 0)
			{
				CG_OSP2BEOnWarmupEnded();
			} */
			cg.warmup = warmup;
		}
		break;
		case CS_SCORES1:
		case CS_SCORES2:
		{
			int newScore;
			int teamNum;
			int* prevScore;
			int* currentScore;
			cgs_osp_t* osp;

			teamNum = (num == CS_SCORES1) ? 1 : 2;
			prevScore = (teamNum == 1) ? &s_prevScores1 : &s_prevScores2;
			currentScore = (teamNum == 1) ? &cgs.scores1 : &cgs.scores2;
			osp = &cgs.osp;

			Q_sscanf(str, "%i", &newScore);
			if (!cgs.fpsclassico)
			{
				if (teamNum == 1)
				{
					osp->osp_teamcount1 = -1;
					Q_sscanf(str, "%i %i", &newScore, &osp->osp_teamcount1);
				}
				else
				{
					osp->osp_teamcount2 = -1;
					Q_sscanf(str, "%i %i", &newScore, &osp->osp_teamcount2);
				}
			}
			if (cgs.gametype == GT_TEAM && CG_OSPIsGameTypeFreeze() && *prevScore != newScore)
			{
				CG_ClearRoundSpecificTeamTaskMarks();
			}
			*prevScore = *currentScore;
			*currentScore = newScore;
		}
		break;
		case CS_REDPLAYERALIVE:
			if (cgs.fpsclassico)
			{
				cgs.osp.osp_teamcount1 = atoi(str);
			}
			break;
		case CS_BLUEPLAYERALIVE:
			if (cgs.fpsclassico)
			{
				cgs.osp.osp_teamcount2 = atoi(str);
			}
			break;
		case CS_LEVEL_START_TIME:
			cgs.levelStartTime = atoi(str);
			break;
		case CS_VOTE_TIME:
			cgs.voteTime = atoi(str);
			cgs.voteModified = qtrue;
			break;
		case CS_VOTE_YES:
			cgs.voteYes = atoi(str);
			cgs.voteModified = qtrue;
			break;
		case CS_VOTE_NO:
			cgs.voteNo = atoi(str);
			cgs.voteModified = qtrue;
			break;
		case CS_VOTE_STRING:
			Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
			break;
		case CS_INTERMISSION:
		{
			qboolean prevIntermissionStarted;

			prevIntermissionStarted = cg.intermissionStarted;
			cg.intermissionStarted = atoi(str);
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR &&
			        CG_OSPIsGameTypeCA(cgs.gametype) &&
			        cgs.clientinfo[cg.snap->ps.clientNum].rt == TEAM_SPECTATOR)
			{
				return;
			}
			if (cg_bestats_autoaction.integer)
			{
				cg.showWStats = qtrue;
				/* send getstatsinfo only once when intermission starts, not every second */
				if (!prevIntermissionStarted && cg.intermissionStarted && !cgs.fpsclassico)
				{
					trap_SendClientCommand("getstatsinfo");
					cg.statsRequestSentInIntermission = qtrue;
				}
			}
			CG_ScoresDown_f();

			if (!prevIntermissionStarted && cg.intermissionStarted && cg_exec_intermission.string[0])
				trap_SendConsoleCommand(cg_exec_intermission.string);
		}
		break;
		case CS_FLAGSTATUS:
			if (cgs.gametype == GT_CTF)
			{
				/* format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped */
				cgs.redflag = str[0] - '0';
				cgs.blueflag = str[1] - '0';
				if (strlen(str) > 3)
				{
					cgs.osp.flagStatus = 1;
					cgs.osp.redflag = str[2] - '0';
					cgs.osp.blueflag = str[3] - '0';
				}
			}
			break;
		case CS_OSP_ALLOW_PMOVE:
			CG_OSPConfigPmoveSet(atoi(str));
			break;
		case CS_OSP_MAXPACKETS_MIN:
			CG_OSPConfigMinMaxPacketsSet(atoi(str));
			break;
		case CS_OSP_MAXPACKETS_MAX:
			CG_OSPConfigMaxMaxPacketsSet(atoi(str));
			break;
		case CS_OSP_TIMENUDGE_MIN:
			CG_OSPConfigMinTimenudgeSet(atoi(str));
			break;
		case CS_OSP_TIMENUDGE_MAX:
			CG_OSPConfigMaxTimenudgeSet(atoi(str));
			break;
		case CS_OSP_AUTH:
			cgs.osp.configString_0x368 = atoi(str) & 1;
			break;
		case CS_OSP_CLAN_BASE_TEAM_DM:
			cgs.osp.clanBaseTeamDM = atoi(str);
			break;
		case CS_OSP_CUSTOM_CLIENT:
			CG_OSPConfigCustomClientSet(atoi(str));
			CG_OSPCvarsRestrictValues();
			break;
		case CS_OSP_CUSTOM_CLIENT2:
			cgs.osp.custom_client_2 = atoi(str);
			CG_UpdateDamageInfoAllowed();
			break;
		case CS_OSP_SERVER_MODE:
			CG_OSPConfigModeSet(atoi(str));
			break;
		case CS_OSP_FREEZE_GAME_TYPE:
		{
			int value;

			value = atoi(str);
			cgs.osp.gameTypeFreeze = (value == 2) ? 2 : (value != 0);
		}
		break;
		case CS_SHADERSTATE:
			CG_ShaderStateChanged();
			break;
		// case CS_OSP2BE_SUPPORTED:
		//  cgs.beSupportedServer = atoi(str);
		//  break;
		case CS_OSP2BE_DISABLED_FEATURES:
			cgs.beDisableFeatures = atoi(str);
			CG_PrintDisabledFeatures(qfalse);
			CG_UpdateBeFeatures();
			CG_UpdateDamageInfoAllowed();
			break;
		// case CS_OSP2BE_TOURNEY_MODE:
		// 	CG_OSP2BEConfigTourneyModeSet(atoi(str));
		// 	break;
		default:
			if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS)
			{
				cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(str);
			}
			else if (num >= CS_SOUNDS && num < CS_SOUNDS + MAX_MODELS)
			{
				if (str[0] != '*')       /* player specific sounds don't register here */
				{
					cgs.gameSounds[num - CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
				}
			}
			else if (num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS)
			{
				CG_NewClientInfo(num - CS_PLAYERS);
				CG_BuildSpectatorString();
			}
			else if (num >= CS_LOCATIONS && num < CS_LOCATIONS + MAX_LOCATIONS)
			{
				CG_MapLocs_NotifyLocationConfigStringChanged();
			}
			break;
	}
}

/*
=======================
CG_AddToTeamChat

=======================
*/
void CG_AddToTeamChat(char* str, int size)
{
	int len;
	int w;
	int h;
	int chatHeight;
	char* p, *ls;
	int lastcolor;
	int overlayWidth;
	int teamchatWidth;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
	{
		chatHeight = cg_teamChatHeight.integer;
	}
	else
	{
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0)
	{
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;
	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;
	lastcolor = '7';
	ls = NULL;
	CG_OSPGetClientFontSize(&cf_Teamchat, &w, &h);

	if (cg_drawTeamOverlay.integer == 2)
	{
		overlayWidth = teamOverlayWidth;
	}
	else
	{
		overlayWidth = 0;
	}

	teamchatWidth = (SCREEN_WIDTH - overlayWidth) / w;

	(*(p) = 0);
	while (*str)
	{
		if (len > teamchatWidth - 1)
		{
			if (ls)
			{
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			if (ch_InverseTeamChat.integer != 0)
			{
				Q_strncpyz(cgs.teamChatMsgs[cgs.teamChatPos % chatHeight], cgs.teamChatMsgs[(cgs.teamChatPos - 1) % chatHeight], TEAMCHAT_WIDTH * 3 + 1);
				p = cgs.teamChatMsgs[(cgs.teamChatPos - 1) % chatHeight];
			}
			else
			{
				p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			}
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if (Q_IsColorString(str))
		{
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ')
		{
			ls = p;
		}
		*p++ = *str++;
		len++;
	}

	*p = 0;

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if (cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart(void)
{
	if (cg_showmiss.integer)
	{
		CG_Printf("CG_MapRestart\n");
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;

	cg.intermissionStarted = qfalse;
	cg.statsRequestSentInIntermission = qfalse;

	cgs.voteTime = 0;

	cg.mapRestart = qtrue;

	if (cg.showScores)
	{
		cg.showScores = qfalse;
		cg.showAccuracy = qfalse;
	}

	cg.scoreFadeTime = 0;

	CG_OSPWStatsUp_f();

	CG_StartMusic();

	trap_S_ClearLoopingSounds(qtrue);

	if (cgs.osp.isOSPv1)
	{
		CG_OSPUpdateUserInfo(qtrue);
	}

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if (cg.warmup == 0 /* && cgs.gametype == GT_TOURNAMENT */)
	{
		trap_S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
		if (!cg_shud.integer) CG_CenterPrint("^1FIGHT!", 20, GIANTCHAR_WIDTH * 2);
	}

	trap_Cvar_Set("cg_thirdPerson", "0");
}


static qboolean CG_ChatIsPaletteColorChar(char c)
{
	if (c >= '0' && c <= '9')
	{
		return qtrue;
	}
	if (c >= 'a' && c <= 'z')
	{
		if (c == 'b' || c == 'f' || c == 'n')
		{
			return qfalse;
		}
		return qtrue;
	}
	if (c >= 'A' && c <= 'Z')
	{
		if (c == 'B' || c == 'F' || c == 'N')
		{
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_RemoveChatEscapeChar
=================
*/
void CG_RemoveChatEscapeChar(char* text)
{
	int i, l;
	char command;

	l = 0;
	for (i = 0; text[i]; i++)
	{
		if (text[i] == '\x19')
			continue;

		if (text[i] != '^')
		{
			text[l++] = text[i];
			continue;
		}

		command = text[i + 1];

		if (command == 'X' || command == 'x')
		{
			float tmp;
			if (CG_Hex16GetColor(&text[i + 2], &tmp) &&
			        CG_Hex16GetColor(&text[i + 4], &tmp) &&
			        CG_Hex16GetColor(&text[i + 6], &tmp))
			{
				i += 7;
				if (text[i + 1] == '^' && (text[i + 2] == 'n' || text[i + 2] == 'N'))
				{
					i += 2;
				}
			}
			else
			{
				++i;
			}
		}
		else if (command)
		{
			switch (command)
			{
				case '9':
					++i;
					text[l++] = '^';
					text[l++] = '7';
					break;
				case '8':
					++i;
					text[l++] = '^';
					text[l++] = '3';
					break;
				case '^':
					++i;
					text[l++] = '^';
					text[l++] = '^';
					break;
				default:
					if (CG_ChatIsPaletteColorChar(command))
					{
						text[l++] = text[i];
					}
					else
					{
						++i;
					}
					break;
			}
		}

	}
	text[l] = '\0';
}

void CG_StringMakeEscapeCharRAW(const char* in, char* out, int max)
{
	int i, l;
	qboolean fix_color = qfalse;

	l = 0;
	for (i = 0; in[i] && l < max - 5; ++i)
	{
		out[l++] = in[i];
		if (fix_color)
		{
			// small hack. as client responds ^^4 as color anyway, lets make it looks like ^^4^74
			out[l++] = '^';
			out[l++] = '7';
			out[l++] = in[i];
			fix_color = qfalse;
		}
		if (in[i] == '^')
		{
			out[l++] = '^';
			if ((in[i + 1] >= '0' && in[i + 1] <= '9') || (in[i + 1] >= 'a' && in[i + 1] <= 'z') || (in[i + 1] >= 'A' && in[i + 1] <= 'Z'))
			{
				fix_color = qtrue;
			}
		}
	}
	out[l] = '\0';
}

/*
=================
CG_RemoveChatEscapeCharAll
=================
*/
void CG_RemoveChatEscapeCharAll(char* text)
{
	const char* s;
	char* d;
	char c;
	float color;

	if (!text)
	{
		return;
	}

	s = text;
	d = text;

	while (*s)
	{
		if (*s == '\x19')
		{
			s++;
			continue;
		}

		if (*s != '^')
		{
			*d++ = *s++;
			continue;
		}

		s++;
		if (!*s)
		{
			break;
		}

		c = *s;
		if (CG_TextControlDisabled(c))
		{
			if (c == 'x' || c == 'X')
			{
				s++;
				continue;
			}
			if (c != '^')
			{
				s++;
				continue;
			}
		}

		if (c == 'X' || c == 'x')
		{
			s++;
			if (CG_Hex16GetColor(s, &color) &&
			        CG_Hex16GetColor(s + 2, &color) &&
			        CG_Hex16GetColor(s + 4, &color))
			{
				s += 6;
			}
			if (*s == '^' && (s[1] == 'n' || s[1] == 'N'))
			{
				s += 2;
			}
			continue;
		}

		if (c == '^')
		{
			*d++ = '^';
			s++;
			continue;
		}

		s++;
	}

	*d = '\0';
}

static void CG_OSPPrintXStats(void)
{
	int i;
	int index;
	int wstats_condition;
	char str[32];
	qboolean damage_activity;
	int client_id;
	int stats_mask;
	int stats_shift;

	index = 1;
	damage_activity = qfalse;
	stats_mask = cgs.osp.stats_mask;
	stats_shift = cgs.osp.stats_shift;

	client_id = atoi(CG_Argv(index++));
	wstats_condition = atoi(CG_Argv(index++));

	Q_strncpyz(&str[0], cgs.clientinfo[client_id].name, 32);

	CG_Printf(va("\n^7Accuracy info for: ^3%s^7\n\n", str));
	CG_Printf("Weapon          Accrcy Hits/Atts Kills Deaths Pickup Drops\n");
	CG_Printf("----------------------------------------------------------\n");
	if (wstats_condition == 0)
	{
		CG_Printf("No weapon info available.\n");
		return;
	}
	for (i = 1; i < 10 ; ++i)
	{
		if ((wstats_condition & (1 << i)) != 0)
		{
			int atts_value;
			int hits_value;
			int kills_value;
			int deaths_value;
			float accuracy;
			hits_value = atoi(CG_Argv(index++));
			atts_value = atoi(CG_Argv(index++));
			kills_value = atoi(CG_Argv(index++));
			deaths_value = atoi(CG_Argv(index++));

			CG_Printf(va("^3%-12s: ", weaponNames[i]));

			if (atts_value & stats_mask || hits_value & stats_mask)
			{
				if (!(atts_value & stats_mask))
				{
					accuracy = 0.0;
				}
				else
				{
					accuracy = 100.0 * ((hits_value & stats_mask) / (float)(atts_value & stats_mask));
				}
				CG_Printf(va("^7%6.1f ^5%4d/%-4d ", accuracy, hits_value & stats_mask, atts_value & stats_mask));
				damage_activity = qtrue;
			}
			else
			{
				CG_Printf("                 ");
				if (kills_value || deaths_value)
				{
					damage_activity = qtrue;
				}
			}

			if (i > 2)
			{
				CG_Printf(va("^2%5d ^1%6d ^3%6d ^1%5d\n", kills_value, deaths_value, atts_value >> stats_shift, hits_value >> stats_shift));
			}
			else
			{
				CG_Printf(va("^2%5d ^1%6d\n", kills_value, deaths_value));
			}
		}
	}

	if (CG_OSPIsStatsHidden(qtrue, qtrue))
	{
		CG_Printf("\n^3Damage Given: ^7XX  ^2Armor Taken : ^7XX\n");
		CG_Printf("^3Damage Recvd: ^7XX  ^2Health Taken: ^7XX\n");
	}
	else
	{
		int yellow_armor;
		int green_armor;
		int red_armor;
		int megahealth;
		int armor_taken;
		int health_taken;
		int damage_given;
		int damage_rcvd;
		char* char_ptr1;
		char* char_ptr2;
		char tmp_str[32];
		char tmp_str2[32];
		damage_given = atoi(CG_Argv((index + 2)));
		damage_rcvd = atoi(CG_Argv((index + 3)));

		armor_taken = atoi(CG_Argv(index));
		health_taken = atoi(CG_Argv((index + 1)));
		megahealth = atoi(CG_Argv((index + 4)));
		green_armor = atoi(CG_Argv((index + 5)));
		red_armor = atoi(CG_Argv((index + 6)));
		yellow_armor = atoi(CG_Argv((index + 7)));

		tmp_str[0] = 0;
		tmp_str2[0] = 0;
		if (megahealth != 0)
		{
			strcpy(tmp_str, va("^2(^7%d ^5MH^2)", megahealth));
		}
		if (green_armor != 0)
		{
			if (red_armor == 0 && yellow_armor == 0)
			{
				char_ptr1 = "^2)";
			}
			else
			{
				char_ptr1 = "^7,";
			}

			strcpy(tmp_str, va("^2(^7%d ^3GA%s", green_armor, char_ptr1));
		}

		if (yellow_armor != 0)
		{
			if (green_armor == 0)
			{
				char_ptr1 = "^2(^7";
				char_ptr2 = "";
			}
			else
			{
				char_ptr1 = " ";
				if (red_armor != 0)
				{
					char_ptr2 = "^7,";
				}
				else
				{
					char_ptr2 = "^2)";
				}
			}
			strcat(tmp_str, va("%s%d ^3YA%s", char_ptr1, yellow_armor, char_ptr2));
		}
		if (red_armor != 0)
		{
			char_ptr1 = " ";
			if (yellow_armor == 0 && green_armor == 0)
			{
				char_ptr1 = "^2(^7";
			}
			strcat(tmp_str, va("%s%d ^1RA^2)", char_ptr1, red_armor));
		}

		if (damage_activity != 0)
		{
			CG_Printf("\n^3Damage Given: ^7%-6d ^2Armor Taken : ^7%d %s\n", damage_given, armor_taken, tmp_str);
			CG_Printf("^3Damage Recvd: ^7%-6d ^2Health Taken: ^7%d %s\n", damage_rcvd, health_taken, tmp_str2);

			if (damage_given != 0)
			{
				float damage_ratio;
				if (damage_rcvd == 0)
					damage_ratio = (float)damage_given;
				else
					damage_ratio = (float)damage_given / damage_rcvd;

				CG_Printf("^3Damage Ratio: ^7%.2f\n", damage_ratio);
			}
		}
		else
		{
			CG_Printf("\n^2Armor Taken : ^7%d %s\n", armor_taken, tmp_str);
			CG_Printf("^2Health Taken: ^7%d %s\n\n", health_taken, tmp_str2);
		}
	}
}

/*
=================
CG_InjectCustomLoc

Replace location coordinates with location name
=================
*/
static void CG_InjectCustomLoc(char* str, int size)
{
	char* loc_start;
	char* loc_end;

	if (customLocationsEnabled != 0)
	{
		char* cloc_begin, *cloc_end;
		int free_left;
		vec3_t cloc;
		if (CG_CustomLocationsTeamChatCode(str, cloc, &cloc_begin, &cloc_end))
		{
			int location_len;
			const char* location_name;
			char* tmp;

			location_name = CG_CustomLocationsGetName(cloc);
			location_len = strlen(location_name); //size of message without location

			tmp = Z_Malloc(size);
			OSP_MEMORY_CHECK(tmp);

			Q_strncpyz(tmp, cloc_end, size);
			free_left = size - (cloc_begin - str);
			Q_strncpyz(cloc_begin, location_name, free_left);
			free_left -= location_len;
			Q_strncpyz(cloc_begin + location_len, tmp, free_left);

			Z_Free(tmp);
		}
	}

	if (ch_FilterLocationsTeamchat.integer)
	{
		loc_start = strchr(str, '(');
		if (loc_start)
		{
			loc_start = strchr(loc_start + 1, '(');
			if (loc_start)
			{
				loc_end = strchr(loc_start, ')');
				if (loc_end)
				{
					strcpy(loc_start - 1, loc_end + 1);
				}
			}
		}
	}
}

void CG_BEParseStatsInfo(void)
{
	static int lastStatsInfo[OSP_STATS_NUM] = { 0 };
	char args[1024];
	int i, weaponIndex, arg_cnt;
	int hits, shots, kills, deaths;
	int pickUps, drops;
	int killsTotal;
	int deathsTotal;
	int suicides;
	int dmgGiven;
	int dmgReceived;
	int wins;
	int losses;
	int totalArgs = trap_Argc();
	qboolean changed = qfalse;
	newStatsInfo_t* ws;
	cgs_osp_t* osp;

	ws = &cgs.newStats;
	osp = &cgs.osp;


	// Don't leave garbage; useful when totalArgs < OSP_STATS_NUM
	memset(statsInfo, 0, sizeof(statsInfo));

	for (i = 0; i < totalArgs - 1 && i < OSP_STATS_NUM; i++)
	{
		trap_Argv(i + 1, args, sizeof(args));
		statsInfo[i] = atoi(args);

		if (statsInfo[i] != lastStatsInfo[i])
			changed = qtrue;
	}

	if (!changed)
		return;

	memcpy(lastStatsInfo, statsInfo, sizeof(statsInfo));

	if (osp->gameTypeFreeze)
	{
		statsInfo[OSP_STATS_LOSSES] &= osp->stats_mask;
	}

	if (statsInfo[OSP_STATS_WEAPON_MASK] == 0)
	{
		return;
	}

	arg_cnt = 23;

	/* weaponIndex = 1: WP_NONE should always be ignored even if weapon mask has it flagged */
	{
		int weaponMask;
		weaponStats_t* wstat;

		weaponMask = statsInfo[OSP_STATS_WEAPON_MASK];
		for (weaponIndex = 1; weaponIndex < WP_NUM_WEAPONS; weaponIndex++)
		{
			wstat = &ws->stats[weaponIndex];
			if (weaponMask & (1 << weaponIndex))
			{
				trap_Argv(arg_cnt++, args, sizeof(args));
				hits = atoi(args);

				trap_Argv(arg_cnt++, args, sizeof(args));
				shots = atoi(args);

				trap_Argv(arg_cnt++, args, sizeof(args));
				kills = atoi(args);

				trap_Argv(arg_cnt++, args, sizeof(args));
				deaths = atoi(args);

				pickUps = shots >> 16;  /* / 65536 */
				drops = hits >> 16;     /* / 65536 */

				shots = shots & 0xFFFF; /* % 65536 */
				hits = hits & 0xFFFF;   /* % 65536 */

				wstat->hits = hits;
				wstat->shots = shots;
				wstat->kills = kills;
				wstat->deaths = deaths;
				wstat->pickUps = pickUps;
				wstat->drops = drops;
				wstat->accuracy = (shots > 0) ? ((float)hits / shots) * 100.0f : 0.0f;
			}
			else
			{
				/* Clear weapon stats that are not in the mask */
				memset(wstat, 0, sizeof(*wstat));
			}
		}
	}

	// General
	killsTotal = statsInfo[OSP_STATS_KILLS];
	deathsTotal = statsInfo[OSP_STATS_DEATHS];
	suicides = statsInfo[OSP_STATS_SUCIDES];
	dmgGiven = statsInfo[OSP_STATS_DMG_GIVEN];
	dmgReceived = statsInfo[OSP_STATS_DMG_RCVD];
	wins = statsInfo[OSP_STATS_WINS] & osp->stats_mask;
	losses = statsInfo[OSP_STATS_LOSSES] & osp->stats_mask;

	// MH / Armor pickups
	ws->megahealth = statsInfo[OSP_STATS_MH];
	ws->ga         = statsInfo[OSP_STATS_GA];
	ws->ya         = statsInfo[OSP_STATS_YA];
	ws->ra         = statsInfo[OSP_STATS_RA];

	ws->armor  = statsInfo[OSP_STATS_WINS] >> osp->stats_shift;
	ws->health = statsInfo[OSP_STATS_LOSSES] >> osp->stats_shift;

	// K/D ratio
	ws->kdratio = (killsTotal > 0 && (deathsTotal + suicides) == 0) ?
	              (float)killsTotal :
	              ((deathsTotal + suicides) > 0) ?
	              (float)killsTotal / (deathsTotal + suicides) : 0.0f;

	// Efficiency: kills / (kills + deaths)
	ws->dmgReceived = statsInfo[OSP_STATS_DMG_RCVD];
	ws->dmgGiven    = statsInfo[OSP_STATS_DMG_GIVEN];

	ws->efficiency = (killsTotal + deathsTotal > 0) ?
	                 (100.0f * (float)killsTotal / (killsTotal + deathsTotal)) : 0.0f;
	if (ws->efficiency < 0.0f)
		ws->efficiency = 0.0f;

	// Damage ratio
	ws->damageRatio = (dmgGiven > 0 || dmgReceived > 0) ?
	                  (float)dmgGiven / (dmgReceived > 0 ? dmgReceived : 1) : 0.0f;

	// General data
	ws->score     = statsInfo[OSP_STATS_SCORE];
	ws->kills     = killsTotal;
	ws->deaths    = deathsTotal;
	ws->suicides  = suicides;
	ws->teamKills = statsInfo[OSP_STATS_TEAM_KILLS];
	ws->teamDamage = statsInfo[OSP_STATS_DMG_TEAM];
	ws->wins      = wins;
	ws->losses    = losses;

	// CTF / Objective data
	ws->caps     = statsInfo[OSP_STATS_CAPS];
	ws->assists  = statsInfo[OSP_STATS_ASSIST];
	ws->defences = statsInfo[OSP_STATS_DEFENCES];
	ws->returns  = statsInfo[OSP_STATS_RETURNS];
	ws->flagTime = statsInfo[OSP_STATS_TIME];
}

/*
=================
CG_OSPAstats

Print topshots weapon or bottomshots weapon
=================
*/
static void CG_OSPAstats(qboolean top)
{
	int a = 1;
	int s = 0;
	float average;
	int number_of_entries;
	int atts;
	float acc;
	int hits;
	char* color_code;
	int client_num;
	int kills;
	int deaths;
	float level;
	float treshold;

	number_of_entries = atoi(CG_Argv(a++));
	if (number_of_entries == 0)
	{
		return;
	}

	average = atoi(CG_Argv(a++));

	CG_Printf("\n^3  Acc Hits/Atts Kills Deaths\n");
	CG_Printf("----------------------------------------------------------\n");
	for (s = 0; s < number_of_entries; ++s)
	{
		client_num = atoi(CG_Argv(a++));
		atts = atoi(CG_Argv(a++));
		hits = atoi(CG_Argv(a++));
		kills = atoi(CG_Argv(a++));
		deaths = atoi(CG_Argv(a++));

		acc = atts > 0 ? (float)(100 * atts) / (float)hits : 0;
		level = top ? acc : (float)average + 0.999f;
		treshold = top ? average : acc;
		color_code = level >= treshold ? "^3" : "^7";

		CG_Printf(va("%s%5.1f ^5%4d/%-4d ^2%5d ^1%6d %s%s\n", color_code, acc, hits, atts, kills, deaths, color_code, cgs.clientinfo[client_num].name_clean));
	}
	CG_Printf("\n\n");
}

/*
=================
CG_OSPBstats

Print topshots weapon or bottomshots weapon
=================
*/
static void CG_OSPBstats(qboolean top)
{
	int a = 1;
	int number_of_weapon;
	int hits;
	int atts;
	int client_num;
	int kills;
	int deaths;
	float acc;

	number_of_weapon = atoi(CG_Argv(a++));
	if (number_of_weapon == 0)
	{
		CG_Printf("\n^3No qualifying %sshot info available.\n\n", top ? "top" : "bottom");
		return;
	}

	CG_Printf("\n^2%sst Match Accuracies:\n", top ? "Be" : "Wor");
	CG_Printf("^3WP   Acc Hits/Atts Kills Deaths\n");
	CG_Printf("-------------------------------------------------------------\n");

	while (number_of_weapon)
	{
		client_num = atoi(CG_Argv(a++));
		atts = atoi(CG_Argv(a++));
		hits = atoi(CG_Argv(a++));
		kills = atoi(CG_Argv(a++));
		deaths = atoi(CG_Argv(a++));

		acc = atts > 0 ? (float)(100 * atts) / (float)hits : 0;

		CG_Printf(va("^3%s ^7%5.1f ^5%4d/%-4d ^2%5d ^1%6d ^7%s\n", weaponShortNames[number_of_weapon], acc, atts, hits, kills, deaths, cgs.clientinfo[client_num].name_clean));

		number_of_weapon = atoi(CG_Argv(a++));
	}
	CG_Printf("\n\n");
}


static void CG_ParseSpecsInfo(void)
{
	int argc = trap_Argc();
	int i;

	cg.beFollowingMe = 0;

	for (i = 1; i < argc; ++i)
	{
		int clientNum;

		clientNum = atoi(CG_Argv(i));

		if (clientNum == -1)
		{
			cg.beFollowingMe = 0;
			break;
		}
		else if (clientNum >= 0 && clientNum < MAX_CLIENTS && clientNum != cg.snap->ps.clientNum)
		{
			cg.beFollowingMe |= (1 << clientNum);
		}
	}
}

/*
=================
CG_InsertChatColor

Insert or replace color code at the beginning of message text (after colon and spaces).
Format: "Name: message" -> "Name: ^Xmessage" where X is color code (digit 0-9 or letter a-z/A-Z).
Server sends: "name^Xmessage" format, so we need to find colon, then find/replace color in message text.
=================
*/
static void CG_InsertChatColor(char* text, int textSize, char colorChar)
{
	const char* colonPos;
	char newText[1024];
	int colonIndex;
	int messageStart;
	int textLen;
	int colorCodeLen;
	int colorInsertPos;

	/* Find colon position */
	colonPos = strchr(text, ':');
	if (!colonPos)
	{
		return;
	}

	colonIndex = colonPos - text;
	textLen = strlen(text);

	/* Find the start of actual message text (skip colon and spaces) */
	messageStart = colonIndex + 1;
	while (messageStart < textLen && text[messageStart] == ' ')
	{
		messageStart++;
	}

	/* If no message text after colon, don't insert color */
	if (messageStart >= textLen)
	{
		return;
	}

	colorInsertPos = messageStart;

	/* Check if color code already exists at message start (server may have added it) */
	/* Support both digits (0-9) and letters (a-z, A-Z) */
	if (messageStart < textLen && text[messageStart] == '^' &&
	        messageStart + 1 < textLen &&
	        ((text[messageStart + 1] >= '0' && text[messageStart + 1] <= '9') ||
	         (text[messageStart + 1] >= 'a' && text[messageStart + 1] <= 'z') ||
	         (text[messageStart + 1] >= 'A' && text[messageStart + 1] <= 'Z')))
	{
		/* Color code already exists at message start, replace it */
		text[messageStart + 1] = colorChar;
		return;
	}

	/* Build color code string (^X where X is colorChar) */
	colorCodeLen = 2; /* ^ + character */

	/* Check if we have enough space */
	if (textLen + colorCodeLen >= textSize - 1)
	{
		return;
	}

	/* Build new message: everything before message start + color code + message text */
	memcpy(newText, text, colorInsertPos); /* Copy up to message start */
	newText[colorInsertPos] = '^';
	newText[colorInsertPos + 1] = colorChar;
	memcpy(newText + colorInsertPos + 2, text + colorInsertPos, textLen - colorInsertPos + 1); /* Copy rest including null terminator */

	Q_strncpyz(text, newText, textSize);
}

/*
=================
CG_WrapPlayerNameInParentheses

Wrap player name in parentheses for team chat.
=================
*/
static void CG_WrapPlayerNameInParentheses(char* text, int textSize)
{
	const char* colonPos;
	char newText[1024];
	int nameLen;
	int restLen;
	const char* restOfMessage;
	const char* nameStart;
	int prefixLen;
	const char* nameStartInText;
	int i;

	/* Find colon position */
	colonPos = strchr(text, ':');
	if (!colonPos)
	{
		return;
	}

	/* Check if there's a prefix like "(team) " at the start */
	nameStartInText = NULL;
	if (text[0] == '(')
	{
		/* Find the closing parenthesis and space after it */
		nameStartInText = strchr(text, ')');
		if (nameStartInText && nameStartInText[1] == ' ')
		{
			prefixLen = nameStartInText - text + 2; /* Include ") " */
			nameStart = text + prefixLen;
		}
		else
		{
			prefixLen = 0;
			nameStart = text;
		}
	}
	else
	{
		prefixLen = 0;
		nameStart = text;
	}

	/* Check if name is already wrapped in parentheses */
	if (nameStart[0] == '(')
	{
		/* Already wrapped, don't wrap again */
		return;
	}

	nameLen = colonPos - nameStart;
	restOfMessage = colonPos; /* Include colon and rest of message */
	restLen = strlen(restOfMessage);

	/* Build new message: prefix + ( + name + ) + colon + rest of message */
	/* Use memcpy instead of sprintf to avoid issues with special characters in name */
	if (prefixLen > 0)
	{
		/* Copy prefix to newText */
		memcpy(newText, text, prefixLen);
		i = prefixLen;

		/* Add opening parenthesis */
		newText[i++] = '(';

		/* Copy name (everything before colon) */
		memcpy(newText + i, nameStart, nameLen);
		i += nameLen;

		/* Add closing parenthesis after name */
		newText[i++] = ')';

		/* Copy rest of message (colon and message text) */
		memcpy(newText + i, restOfMessage, restLen + 1); /* +1 for null terminator */

		Q_strncpyz(text, newText, textSize);
	}
	else
	{
		/* No prefix, just wrap name */
		i = 0;

		/* Add opening parenthesis */
		newText[i++] = '(';

		/* Copy name (everything before colon) */
		memcpy(newText + i, nameStart, nameLen);
		i += nameLen;

		/* Add closing parenthesis after name */
		newText[i++] = ')';

		/* Copy rest of message (colon and message text) */
		memcpy(newText + i, restOfMessage, restLen + 1); /* +1 for null terminator */

		Q_strncpyz(text, newText, textSize);
	}
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/

void CG_ServerCommand(void)
{
	const char* cmd;
	char        cmdBuf[64];   /* CG_Argv uses single static buffer; save argv(0) before reading argv(1) */
	char        text[1024];
	int         ignoreMask;

	Q_strncpyz(cmdBuf, CG_Argv(0), sizeof(cmdBuf));
	cmd = cmdBuf;
	if (!cmd || !cmd[0])
	{
		return;
	}
	ignoreMask = cg_ignoreServerMessages.integer;

	switch (cmd[0])
	{
		case 'p':
			/* "print" - check first (common, real work) */
			if (cmd[1] == 'r' && cmd[2] == 'i' && cmd[3] == 'n' && cmd[4] == 't' && cmd[5] == '\0')
			{
				const char* had_pos;

				Q_strncpyz(text, CG_Argv(1), sizeof(text));
				CG_RemoveChatEscapeChar(text);

				if ((ignoreMask & 1) && (had_pos = strstr(text, " had ")) != NULL)
				{
					if ((size_t)(had_pos - text) > 0)
						return;
				}
				if ((ignoreMask & 2) && strstr(text, "chat fragged") != NULL)
				{
					return;
				}
				if ((ignoreMask & 8) && strstr(text, "Backpack") != NULL)
				{
					return;
				}

				CG_Printf("%s", text);
				return;
			}
			/* psohtua - no-op, check last */
			if (cmd[1] == 's' && cmd[2] == 'o' && cmd[3] == 'h' && cmd[4] == 't' && cmd[5] == 'u' && cmd[6] == 'a' && cmd[7] == '\0')
			{
				return;
			}
			break;

		case 'c':
			/* cp - 2 chars */
			if (cmd[1] == 'p' && cmd[2] == '\0')
			{
				const char* arg;
				const char* meatArg;
				char*       meatPos;
				int         meatOffset;
				int         tailLen;
				arg = CG_Argv(1);

				if (arg[0] == 'M' && arg[1] == 'u' && arg[2] == 'l' && arg[3] == 't' &&
				        arg[4] == 'i' && arg[5] == '-' && arg[6] == 'v' && arg[7] == 'i' &&
				        arg[8] == 'e' && arg[9] == 'w' && arg[10] == ' ' && arg[11] == 'd' &&
				        arg[12] == 'e' && arg[13] == 'm' && arg[14] == 'o' && arg[15] == ' ' &&
				        arg[16] == 'c' && arg[17] == 'r' && arg[18] == 'e' && arg[19] == 'a' &&
				        arg[20] == 't' && arg[21] == 'e' && arg[22] == 'd' && arg[23] == ' ' &&
				        arg[24] == 'w' && arg[25] == 'i' && arg[26] == 't' && arg[27] == 'h')
				{
					return;
				}

				meatArg = strstr(arg, "Enjoy the Meat");
				if (!meatArg)
				{
					CG_CenterPrint(arg, 120, SMALLCHAR_WIDTH);
					return;
				}

				Q_strncpyz(text, arg, sizeof(text));
				meatOffset = meatArg - arg + 10; /* skip "Enjoy the " */
				if (meatOffset + 11 < (int)sizeof(text))
				{
					meatPos = text + meatOffset;
					tailLen = strlen(meatPos + 4);
					if (meatOffset + 11 + tailLen < (int)sizeof(text))
					{
						memmove(meatPos + 11, meatPos + 4, tailLen + 1);
						memcpy(meatPos, "Noob Server", 11);
					}
				}

				CG_CenterPrint(text, 120, SMALLCHAR_WIDTH);
				return;
			}
			/* cs - 2 chars */
			if (cmd[1] == 's' && cmd[2] == '\0')
			{
				CG_ConfigStringModified();
				return;
			}
			/* chat */
			if (cmd[1] == 'h' && cmd[2] == 'a' && cmd[3] == 't' && cmd[4] == '\0')
			{
				messageAllowed_t ma;
				qboolean isVanilaChatEnabled;
				qboolean isShudChatEnabled;
				qboolean isChatWindowEnabled;
				qboolean isTellMessage;
				int senderClientNum;
				char rawChat[MAX_STRING_CHARS];

				Q_strncpyz(rawChat, CG_Argv(1), sizeof(rawChat));
				Q_strncpyz(text, rawChat, 1024);

				/* Tell messages should use color ^6 (magenta) instead of regular chat color */
				isTellMessage = (text[0] == 25 && (text[1] == '(' || text[1] == '['));

				ma = CG_ChatCheckMessageAllowed(text);
				if (ma == MESSAGE_NOTALLOWED)
				{
					return;
				}

				if ((ma != MESSAGE_ALLOWED_PLAYER) && (ma != MESSAGE_NOTALLOWED) && (ignoreMask & 4))
				{
					return;
				}

				if (ma == MESSAGE_ALLOWED_PLAYER)
				{
					senderClientNum = CG_ChatResolveSenderClientNum(rawChat, -1);
				}
				else
				{
					senderClientNum = -1;
				}

				if (isTellMessage && ma == MESSAGE_ALLOWED_PLAYER)
				{
					char senderName[MAX_QPATH];
					int  replyTo;

					if (CG_ChatExtractSenderName(rawChat, senderName, sizeof(senderName)))
					{
						CG_RemoveChatEscapeChar(senderName);
						if (senderName[0])
						{
							replyTo = CG_ChatClientNumFromSenderName(senderName);
							if (replyTo != cg.clientNum)
							{
								CG_SetLastTellSenderName(senderName);
							}
						}
					}
				}

				isVanilaChatEnabled = cg_chatEnable.integer & CG_CHAT_COMMMON_ENABLED;
				isShudChatEnabled = cg_shudChatEnable.integer & CG_CHAT_COMMMON_ENABLED && ma == MESSAGE_ALLOWED_PLAYER;
				isChatWindowEnabled = (cg_chatWindow.integer) != 0;

				if (!isVanilaChatEnabled && !isShudChatEnabled && !isChatWindowEnabled)
				{
					return;
				}

				if (!cg_nochatbeep.integer && (isVanilaChatEnabled || isShudChatEnabled || isChatWindowEnabled))
				{
					trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
				}

				CG_RemoveChatEscapeChar(text);

				if (isTellMessage)
				{
					/* Insert magenta color (^6) for tell messages */
					CG_InsertChatColor(text, 1024, '6');
				}
				else if (ma == MESSAGE_ALLOWED_PLAYER)
				{
					/* Insert default chat color only for player messages (with ID) */
					/* Server messages (MESSAGE_ALLOWED_OTHER) keep their original color */
					/* Support both digits (0-9) and letters (a-z, A-Z) */
					if (cg_chatColor.string[0] != '\0')
					{
						char colorChar;
						colorChar = cg_chatColor.string[0];
						if ((colorChar >= '0' && colorChar <= '9') ||
						        (colorChar >= 'a' && colorChar <= 'z') ||
						        (colorChar >= 'A' && colorChar <= 'Z'))
						{
							CG_InsertChatColor(text, 1024, colorChar);
						}
					}
				}

				if (isVanilaChatEnabled)
				{
					CG_Printf("%s\n", text);
				}
				if (isShudChatEnabled)
				{
					CG_SHUDEventChat(text, senderClientNum);
				}
				CG_BEChat_AddLine(text, senderClientNum,
				                  isTellMessage ? BE_CHAT_MSG_TELL :
				                  (ma == MESSAGE_ALLOWED_PLAYER ? BE_CHAT_MSG_SAY : BE_CHAT_MSG_OTHER));
				return;
			}
			/* clientLevelShot - real work (camelCase, need Q_stricmp) */
			if (Q_stricmp(cmd, "clientLevelShot") == 0)
			{
				cg.levelShot = qtrue;
				return;
			}
			/* crecord, cscreenshot - no-op, check last */
			if (cmd[1] == 'r' && cmd[2] == 'e' && cmd[3] == 'c' && cmd[4] == 'o' && cmd[5] == 'r' && cmd[6] == 'd' && cmd[7] == '\0')
			{
				return;
			}
			if (cmd[1] == 's' && cmd[2] == 'c' && cmd[3] == 'r' && cmd[4] == 'e' && cmd[5] == 'e' &&
			        cmd[6] == 'n' && cmd[7] == 's' && cmd[8] == 'h' && cmd[9] == 'o' && cmd[10] == 'o' && cmd[11] == 't' && cmd[12] == '\0')
			{
				return;
			}
			break;

		case 't':
			/* tchat */
			if (cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == '\0')
			{
				messageAllowed_t ma;
				qboolean isVanilaChatEnabled;
				qboolean isShudChatEnabled;
				qboolean isChatWindowEnabled;
				int senderClientNum;
				char rawChat[MAX_STRING_CHARS];

				Q_strncpyz(rawChat, CG_Argv(1), sizeof(rawChat));
				Q_strncpyz(text, rawChat, 1024);
				ma = CG_ChatCheckMessageAllowed(text);
				if (ma == MESSAGE_NOTALLOWED)
				{
					return;
				}

				if ((ma != MESSAGE_ALLOWED_PLAYER) && (ma != MESSAGE_NOTALLOWED) && (ignoreMask & 4))
				{
					return;
				}

				if (ma == MESSAGE_ALLOWED_PLAYER)
				{
					senderClientNum = CG_ChatResolveSenderClientNum(rawChat, -1);
				}
				else
				{
					senderClientNum = -1;
				}

				isVanilaChatEnabled = cg_chatEnable.integer & CG_CHAT_TEAM_ENABLED;
				isShudChatEnabled = cg_shudChatEnable.integer & CG_CHAT_TEAM_ENABLED && ma == MESSAGE_ALLOWED_PLAYER;
				isChatWindowEnabled = (cg_chatWindow.integer) != 0;

				if (!isVanilaChatEnabled && !isShudChatEnabled && !isChatWindowEnabled)
				{
					return;
				}

				if (!cg_noTeamChatBeep.integer && (isVanilaChatEnabled || isShudChatEnabled || isChatWindowEnabled))
				{
					trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
				}

				CG_RemoveChatEscapeChar(text);

				/* Wrap player name in parentheses for team chat (after removing escape chars) */
				CG_WrapPlayerNameInParentheses(text, 1024);

				CG_InjectCustomLoc(text, 1024);

				/* Insert default team chat color at the beginning of message text (after colon) */
				/* This replaces any server-provided color with our default color */
				/* Support both digits (0-9) and letters (a-z, A-Z) */
				if (cg_teamChatColor.string[0] != '\0')
				{
					char colorChar;
					colorChar = cg_teamChatColor.string[0];
					if ((colorChar >= '0' && colorChar <= '9') ||
					        (colorChar >= 'a' && colorChar <= 'z') ||
					        (colorChar >= 'A' && colorChar <= 'Z'))
					{
						CG_InsertChatColor(text, 1024, colorChar);
					}
				}

				if (isVanilaChatEnabled)
				{
					CG_AddToTeamChat(text, 1024);
					if (!ch_TeamchatOnly.integer || cgs.gametype == GT_TOURNAMENT)
					{
						CG_Printf("%s\n", text);
					}
				}

				if (isShudChatEnabled)
				{
					CG_SHUDEventChat(text, senderClientNum);
				}

				CG_BEChat_AddLine(text, senderClientNum, BE_CHAT_MSG_TEAM);
				return;
			}
			/* "tinfo" */
			if (cmd[1] == 'i' && cmd[2] == 'n' && cmd[3] == 'f' && cmd[4] == 'o' && cmd[5] == '\0')
			{
				CG_ParseTeamInfo();
				return;
			}
			break;

		case 's':
			/* sset, scmds - char comparison (no Q_stricmp) */
			/* "scores" */
			if (cmd[1] == 'c' && cmd[2] == 'o' && cmd[3] == 'r' && cmd[4] == 'e' && cmd[5] == 's' && cmd[6] == '\0')
			{
				CG_ParseScores();
				return;
			}
			/* "statsinfo" */
			if (cmd[1] == 't' && cmd[2] == 'a' && cmd[3] == 't' && cmd[4] == 's' && cmd[5] == 'i' &&
			        cmd[6] == 'n' && cmd[7] == 'f' && cmd[8] == 'o' && cmd[9] == '\0')
			{
				CG_BEParseStatsInfo();
				return;
			}
			/* "sset" */
			if (cmd[1] == 's' && cmd[2] == 'e' && cmd[3] == 't' && cmd[4] == '\0')
			{
				int i;
				char buf[512];
				char* prefix;
				int k;
				int argNum;

				prefix = "";
				argNum = trap_Argc();
				for (k = 0, buf[0] = 0, i = 1; i < argNum; i += 2)
				{
					strcat(buf, va("%s%20s ", prefix, CG_Argv(i)));
					strcat(buf, va("%-3s", CG_Argv((i + 1))));
					prefix = "  ";
					if (k++ == 3)
					{
						CG_Printf("%s\n", buf);
						k = 0;
						prefix = "";
						buf[0] = 0;
					}
				}
				if (buf[0])
				{
					CG_Printf("%s\n", buf);
				}
				return;
			}
			/* "stuff" */
			if (cmd[1] == 't' && cmd[2] == 'u' && cmd[3] == 'f' && cmd[4] == 'f' && cmd[5] == '\0')
			{
				char command[1024];
				char command2[1024];
				int len;
				char* ptr;
				char* out;
				int i;

				if (!cg.demoPlayback)
				{
					trap_Args(&command[0], 0x400);
					command[1023] = 0;
					len = strlen(&command[0]);
					ptr = &command[0];
					out = &command2[0];
					for (i = 0; *ptr && i < len;)
					{
						if (*ptr == '\'')
						{
							++ptr;
							*out++ = (*ptr == '\'') ? '\'' : '"';
							++ptr;
							++i;
						}
						else
						{
							*out++ = *ptr++;
							++i;
						}
					}
					*out = 0;
					if (strstr(command2, "screenshot") ||
					        strstr(command2, "set g_synchronous") ||
					        strstr(command2, "clear") ||
					        strstr(command2, "currenttime") ||
					        strstr(command2, "stoprecord"))
					{
						trap_SendConsoleCommand(command2);
					}
				}
				return;
			}
			/* "specsinfo" */
			if (cmd[1] == 'p' && cmd[2] == 'e' && cmd[3] == 'c' && cmd[4] == 's' && cmd[5] == 'i' &&
			        cmd[6] == 'n' && cmd[7] == 'f' && cmd[8] == 'o' && cmd[9] == '\0')
			{
				CG_ParseSpecsInfo();
				return;
			}
			/* "scmds" */
			if (cmd[1] == 'c' && cmd[2] == 'm' && cmd[3] == 'd' && cmd[4] == 's' && cmd[5] == '\0')
			{
				const char* part;
				char buffer[2048];
				char* token;
				int i;
				qboolean valid;

				part = CG_Argv(1);
				Q_strncpyz(buffer, part, sizeof(buffer));
				token = Q_strtok(buffer, "/");
				while (token != NULL)
				{
					if (token[0] != '\0')
					{
						valid = qtrue;
						for (i = 0; token[i]; ++i)
						{
							if (!((token[i] >= 'a' && token[i] <= 'z') ||
							        (token[i] >= 'A' && token[i] <= 'Z') ||
							        (token[i] >= '0' && token[i] <= '9') ||
							        (token[i] == '_')))
							{
								valid = qfalse;
								break;
							}
						}
						if (valid)
						{
							trap_AddCommand(token);
						}
					}
					token = Q_strtok(NULL, "/");
				}
				return;
			}
			break;

		case 'm':
			/* "map_restart" */
			if (cmd[1] == 'a' && cmd[2] == 'p' && cmd[3] == '_' && cmd[4] == 'r' && cmd[5] == 'e' &&
			        cmd[6] == 's' && cmd[7] == 't' && cmd[8] == 'a' && cmd[9] == 'r' && cmd[10] == 't' && cmd[11] == '\0')
			{
				CG_MapRestart();
				return;
			}
			break;

		case 'r':
			/* remapShader - camelCase, use Q_stricmp for safety */
			if (Q_stricmp(cmd, "remapShader") == 0)
			{
				if (trap_Argc() == 4)
				{
					const char* arg[3];
					arg[0] = CG_Argv(1);
					arg[1] = CG_Argv(2);
					arg[2] = CG_Argv(3);
					trap_R_RemapShader(arg[0], arg[1], arg[2]);
				}
				return;
			}
			break;

		case 'v':
			/* "viewactive" - does something, check first */
			if (cmd[1] == 'i' && cmd[2] == 'e' && cmd[3] == 'w' && cmd[4] == 'a' && cmd[5] == 'c' &&
			        cmd[6] == 't' && cmd[7] == 'i' && cmd[8] == 'v' && cmd[9] == 'e' && cmd[10] == '\0')
			{
				CG_Printf("viewactive is not implimented\n");
				return;
			}
			/* viewlist - no-op, check last */
			if (cmd[1] == 'i' && cmd[2] == 'e' && cmd[3] == 'w' && cmd[4] == 'l' && cmd[5] == 'i' &&
			        cmd[6] == 's' && cmd[7] == 't' && cmd[8] == '\0')
			{
				return;
			}
			break;

		case 'd':
			/* dstatus - dead clients in freeze (response to getdstatus) */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 'u' &&
			        cmd[6] == 's' && cmd[7] == '\0')
			{
				CG_ParseDstatus();
				return;
			}
			/* dmgplum_proj - damage plum for projectile weapons (GL/RL) via server command */
			if (cmd[1] == 'm' && cmd[2] == 'g' && cmd[3] == 'p' && cmd[4] == 'l' && cmd[5] == 'u' &&
			        cmd[6] == 'm' && cmd[7] == '_' && cmd[8] == 'p' && cmd[9] == 'r' && cmd[10] == 'o' && cmd[11] == 'j' && cmd[12] == '\0')
			{
				int damage;
				int argc;
				vec3_t position;

				if (!(cg_damagePlums.integer & CG_DAMAGEPLUMS_ACTIVE))
				{
					return;
				}

				argc = trap_Argc();
				if (argc >= 5)
				{
					damage = atoi(CG_Argv(1));
					position[0] = atof(CG_Argv(2));
					position[1] = atof(CG_Argv(3));
					position[2] = atof(CG_Argv(4));

					CG_DamagePlum(cg.predictedPlayerState.clientNum, position, damage);
				}
				return;
			}
			break;

		case 'x':
			/* xstats1 */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 's' &&
			        cmd[6] == '1' && cmd[7] == '\0')
			{
				if (cg.beStatsAllRequested)
				{
					CG_BEParseXStatsToStatsAll();
					xstats1_received_count++;
					last_xstats1_sequence = cgs.serverCommandSequence;
					return;
				}
				else
				{
					CG_OSPPrintXStats();
					return;
				}
			}
			break;

		case 'a':
			/* astats - real work, check first */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 's' && cmd[6] == '\0')
			{
				CG_OSPAstats(qtrue);
				return;
			}
			/* astatsb - no-op, check last */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 's' &&
			        cmd[6] == 'b' && cmd[7] == '\0')
			{
				return;
			}
			break;

		case 'b':
			/* bstats - real work, check first */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 's' && cmd[6] == '\0')
			{
				CG_OSPBstats(qtrue);
				return;
			}
			/* bstatsb - no-op, check last */
			if (cmd[1] == 's' && cmd[2] == 't' && cmd[3] == 'a' && cmd[4] == 't' && cmd[5] == 's' &&
			        cmd[6] == 'b' && cmd[7] == '\0')
			{
				return;
			}
			break;
	}

	/* Unknown command - silently ignored */
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands(int latestSequence)
{
	while (cgs.serverCommandSequence < latestSequence)
	{
		if (trap_GetServerCommand(++cgs.serverCommandSequence))
		{
			CG_ServerCommand();
		}
	}
	if (last_xstats1_sequence == latestSequence && cg.beStatsAllRequested)
	{
		cg.beStatsAllRequested = qfalse;
		last_xstats1_sequence = -1;
	}
}
