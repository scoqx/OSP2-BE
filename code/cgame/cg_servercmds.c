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
		client = atoi(CG_Argv(i * 6 + 2));

		ci = &cgs.clientinfo[ client ];

		sortedTeamPlayers[i] = client;

		if (customLocationsEnabled)
		{
			CG_CustomLocationsSetLocation(CG_Argv(i * 6 + 3), ci->customLocation);
		}
		ci->location = atoi(CG_Argv(i * 6 + 3));
		ci->health = atoi(CG_Argv(i * 6 + 4));
		ci->armor = atoi(CG_Argv(i * 6 + 5));
		ci->curWeapon = atoi(CG_Argv(i * 6 + 6));
		ci->powerups = atoi(CG_Argv(i * 6 + 7));
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
	const char*  info;
	char*    mapname;

	info = CG_ConfigString(CS_SERVERINFO);
	cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
	cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
	cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));
	cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
	cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
	cgs.timelimitMsec = atoi(Info_ValueForKey(info, "timelimit")) * 60 * 1000;
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
	Q_strncpyz(cgs.redTeam, Info_ValueForKey(info, "g_redTeam"), sizeof(cgs.redTeam));
	trap_Cvar_Set("g_redTeam", cgs.redTeam);
	Q_strncpyz(cgs.blueTeam, Info_ValueForKey(info, "g_blueTeam"), sizeof(cgs.blueTeam));
	trap_Cvar_Set("g_blueTeam", cgs.blueTeam);
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup(void)
{
	const char*  info;
	int         warmup;

	info = CG_ConfigString(CS_WARMUP);

	warmup = atoi(info);
	cg.warmupCount = -1;

	if (warmup == 0 && cg.warmup)
	{
		// Очистка новой статистики
		memset(&cgs.be.newStats, 0, sizeof(cgs.be.newStats));

		/* Execute warmup ended command */
		CG_ExecCommandIfSet(&cg_exec_warmupEnded);
	}
	else if (warmup > 0 && cg.warmup <= 0)
	{
		{
			trap_S_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
		}
	}

	cg.warmup = warmup;
}

/*
==================
CG_ParseScoresUpdate

Parse and update scores from configstring.
Handles round change detection and clearing round-specific marks.
==================
*/
static void CG_ParseScoresUpdate(int teamNum, const char* str)
{
	int newScore;
	int* prevScore;
	int* currentScore;
	int* teamCount;

	if (teamNum == 1)
	{
		prevScore = &cg.prevScores1;
		currentScore = &cgs.scores1;
		teamCount = &cgs.osp.osp_teamcount1;
	}
	else
	{
		prevScore = &cg.prevScores2;
		currentScore = &cgs.scores2;
		teamCount = &cgs.osp.osp_teamcount2;
	}

	*teamCount = -1;
	Q_sscanf(str, "%i %i", &newScore, teamCount);

	/* Check if round changed before updating values */
	if (cgs.gametype == GT_TEAM && CG_OSPIsGameTypeFreeze() && *prevScore != newScore)
	{
		CG_ClearRoundSpecificTeamTaskMarks();
	}

	*prevScore = *currentScore;
	*currentScore = newScore;
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

	cgs.scores1 = atoi(CG_ConfigString(CS_SCORES1));
	cgs.scores2 = atoi(CG_ConfigString(CS_SCORES2));
	cg.prevScores1 = cgs.scores1;
	cg.prevScores2 = cgs.scores2;
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
	if (num == CS_MUSIC)
	{
		CG_StartMusic();
	}
	else if (num == CS_SERVERINFO)
	{
		CG_ParseServerinfo();
	}
	else if (num == CS_SYSTEMINFO)
	{
		CG_UpdateAutoHopFromSystemInfo();
	}
	else if (num == CS_WARMUP)
	{
		CG_ParseWarmup();
	}
	else if (num == CS_SCORES1)
	{
		CG_ParseScoresUpdate(1, str);
	}
	else if (num == CS_SCORES2)
	{
		CG_ParseScoresUpdate(2, str);
	}
	else if (num == CS_LEVEL_START_TIME)
	{
		cgs.levelStartTime = atoi(str);
	}
	else if (num == CS_VOTE_TIME)
	{
		cgs.voteTime = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if (num == CS_VOTE_YES)
	{
		cgs.voteYes = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if (num == CS_VOTE_NO)
	{
		cgs.voteNo = atoi(str);
		cgs.voteModified = qtrue;
	}
	else if (num == CS_VOTE_STRING)
	{
		Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
	}
	else if (num == CS_INTERMISSION)
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
			CG_OSPWStatsDown_f();
		}
		CG_ScoresDown_f();

		/* Execute intermission command when intermission starts */
		if (!prevIntermissionStarted && cg.intermissionStarted)
		{
			CG_ExecCommandIfSet(&cg_exec_intermission);
		}
	}
	else if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS)
	{
		cgs.gameModels[ num - CS_MODELS ] = trap_R_RegisterModel(str);
	}
	else if (num >= CS_SOUNDS && num < CS_SOUNDS + MAX_MODELS)
	{
		if (str[0] != '*')       // player specific sounds don't register here
		{
			cgs.gameSounds[ num - CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
		}
	}
	else if (num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS)
	{
		CG_NewClientInfo(num - CS_PLAYERS);
		CG_BuildSpectatorString();
	}
	else if (num == CS_FLAGSTATUS)
	{
		if (cgs.gametype == GT_CTF)
		{
			// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
			if (strlen(str) > 3)
			{
				cgs.osp.flagStatus = 1;
				cgs.osp.redflag = str[2] - '0';
				cgs.osp.blueflag = str[3] - '0';
			}
		}
	}
	else if (num == CS_OSP_ALLOW_PMOVE)
	{
		CG_OSPConfigPmoveSet(atoi(str));
	}
	else if (num == CS_OSP_MAXPACKETS_MIN)
	{
		CG_OSPConfigMinMaxPacketsSet(atoi(str));
	}
	else if (num == CS_OSP_MAXPACKETS_MAX)
	{
		CG_OSPConfigMaxMaxPacketsSet(atoi(str));
	}
	else if (num == CS_OSP_TIMENUDGE_MIN)
	{
		CG_OSPConfigMinTimenudgeSet(atoi(str));
	}
	else if (num == CS_OSP_TIMENUDGE_MAX)
	{
		CG_OSPConfigMaxTimenudgeSet(atoi(str));
	}
	else if (num == CS_OSP_AUTH)
	{
		CG_OSPConfig0x368Set(atoi(str) & 1);
	}
	else if (num == CS_OSP_CLAN_BASE_TEAM_DM)
	{
		CG_OSPConfigClanBaseTDMSet(atoi(str));
	}
	else if (num == CS_OSP_CUSTOM_CLIENT)
	{
		CG_OSPConfigCustomClientSet(atoi(str));
		CG_OSPCvarsRestrictValues();
	}
	else if (num == CS_OSP_CUSTOM_CLIENT2)
	{
		CG_OSPConfigCustomClient2Set(atoi(str));
	}
	else if (num == CS_OSP_SERVER_MODE)
	{
		CG_OSPConfigModeSet(atoi(str));
	}
	else if (num == CS_OSP_FREEZE_GAME_TYPE)
	{
		CG_OSPConfigFreezeModeSet(atoi(str));
	}
	else if (num == CS_SHADERSTATE)
	{
		CG_ShaderStateChanged();
	}
	else if (num == CS_OSP2BE_SUPPORTED)
	{
		CG_OSPSupportedBEServer(atoi(str));
	}
	else if (num == CS_OSP2BE_DISABLED_FEATURES)
	{
		cgs.be.disableFeatures = atoi(str);
		CG_PrintDisabledFeatures(qfalse);
	}
	else if (num == XQ3E_ALLOW_FEATURES)
	{
		CG_OSPConfigXHitBoxSet(atoi(str));
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
				i += 6;
			}
			++i;
		}
		else if (command)
		{
			switch (command)
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					text[l++] = text[i];
					break;
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
					++i;
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
				i += 7; /* Skip ^X and 6 hex characters (total 8 characters, -1 because for loop will increment) */
				continue;
			}
			++i; /* Skip ^ if invalid format, then continue to skip X via loop increment */
			continue;
		}
		else if (command == '^')
		{
			text[l++] = '^';
		}
		++i;
	}
	text[l] = '\0';
}

static void CG_OSPPrintXStats(void)
{
	int i;
	int index = 1;
	int wstats_condition;
	char str[32];
	qboolean damage_activity = qfalse;
	int client_id;

	index = 1;
	damage_activity = qfalse;

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

			if (atts_value & cgs.osp.stats_mask || hits_value & cgs.osp.stats_mask)
			{
				if (!(atts_value & cgs.osp.stats_mask))
				{
					accuracy = 0.0;
				}
				else
				{
					accuracy = 100.0 * ((hits_value & cgs.osp.stats_mask) / (float)(atts_value & cgs.osp.stats_mask));
				}
				CG_Printf(va("^7%6.1f ^5%4d/%-4d ", accuracy, hits_value & cgs.osp.stats_mask, atts_value & cgs.osp.stats_mask));
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
				CG_Printf(va("^2%5d ^1%6d ^3%6d ^1%5d\n", kills_value, deaths_value, atts_value >> cgs.osp.stats_shift, hits_value >> cgs.osp.stats_shift));
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

static void CG_OSPPrintSSet(void)
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
}

static void CG_ServerCommandStuff(void)
{
	char command[1024];
	char command2[1024];
	int len;

	if (cg.demoPlayback)
	{
		return;
	}

	trap_Args(&command[0], 0x400);
	command[1023] = 0;

	len = strlen(&command[0]);

	{
		char* ptr = &command[0];
		char* out = &command2[0];
		int i;
		for (i = 0; *ptr && i < len;)
		{
			if (*ptr == '\'')
			{
				++ptr;
				if (*ptr == '\'')
				{
					*out++ = '\'';
				}
				else
				{
					*out++ = '"';
				}
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
	}

	if (strstr(command2, "screenshot") ||
	        strstr(command2, "set g_synchronous") ||
	        strstr(command2, "clear") ||
	        strstr(command2, "currenttime") ||
	        strstr(command2, "stoprecord"))
	{
		trap_SendConsoleCommand(command2);
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
	qboolean changed = qfalse;

	newStatsInfo_t* ws = &cgs.be.newStats;

	int totalArgs = trap_Argc();

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

	if (cgs.osp.gameTypeFreeze)
	{
		statsInfo[OSP_STATS_LOSSES] &= cgs.osp.stats_mask;
	}

	if (statsInfo[OSP_STATS_WEAPON_MASK] == 0)
	{
		return;
	}

	arg_cnt = 23;

	// weaponIndex = 1: WP_NONE should always be ignored even if weapon mask has it flagged
	for (weaponIndex = 1; weaponIndex < WP_NUM_WEAPONS; weaponIndex++)
	{
		if (statsInfo[OSP_STATS_WEAPON_MASK] & (1 << weaponIndex))
		{
			trap_Argv(arg_cnt++, args, sizeof(args));
			hits = atoi(args);

			trap_Argv(arg_cnt++, args, sizeof(args));
			shots = atoi(args);

			trap_Argv(arg_cnt++, args, sizeof(args));
			kills = atoi(args);

			trap_Argv(arg_cnt++, args, sizeof(args));
			deaths = atoi(args);

			pickUps = shots / 65536;
			drops = hits / 65536;

			shots = shots % 65536;
			hits = hits % 65536;

			ws->stats[weaponIndex].hits = hits;
			ws->stats[weaponIndex].shots = shots;
			ws->stats[weaponIndex].kills = kills;
			ws->stats[weaponIndex].deaths = deaths;
			ws->stats[weaponIndex].pickUps = pickUps;
			ws->stats[weaponIndex].drops = drops;
			ws->stats[weaponIndex].accuracy = (shots > 0) ? ((float)hits / shots) * 100.0f : 0.0f;
		}
		else
		{
			// Clear weapon stats that are not in the mask
			ws->stats[weaponIndex].hits = 0;
			ws->stats[weaponIndex].shots = 0;
			ws->stats[weaponIndex].kills = 0;
			ws->stats[weaponIndex].deaths = 0;
			ws->stats[weaponIndex].pickUps = 0;
			ws->stats[weaponIndex].drops = 0;
			ws->stats[weaponIndex].accuracy = 0.0f;
		}
	}

	// General
	killsTotal = statsInfo[OSP_STATS_KILLS];
	deathsTotal = statsInfo[OSP_STATS_DEATHS];
	suicides = statsInfo[OSP_STATS_SUCIDES];
	dmgGiven = statsInfo[OSP_STATS_DMG_GIVEN];
	dmgReceived = statsInfo[OSP_STATS_DMG_RCVD];
	wins = statsInfo[OSP_STATS_WINS] & cgs.osp.stats_mask;
	losses = statsInfo[OSP_STATS_LOSSES] & cgs.osp.stats_mask;

	// MH / Armor pickups
	ws->megahealth = statsInfo[OSP_STATS_MH];
	ws->ga         = statsInfo[OSP_STATS_GA];
	ws->ya         = statsInfo[OSP_STATS_YA];
	ws->ra         = statsInfo[OSP_STATS_RA];

	ws->armor  = statsInfo[OSP_STATS_WINS] >> cgs.osp.stats_shift;
	ws->health = statsInfo[OSP_STATS_LOSSES] >> cgs.osp.stats_shift;

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


static void CG_HandleScmdsCommand(void)
{
	const char* part;
	char buffer[2048];
	char* token;

	part = CG_Argv(1);

	Q_strncpyz(buffer, part, sizeof(buffer));
	token = Q_strtok(buffer, "/");
	while (token != NULL)
	{
		if (token[0] != '\0')
		{
			int i;
			qboolean valid = qtrue;
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
}

static void CG_ParseSpecsInfo(void)
{
	int argc = trap_Argc();
	int i;

	cgs.be.followingMe = 0;

	for (i = 1; i < argc; ++i)
	{
		int clientNum;

		clientNum = atoi(CG_Argv(i));

		if (clientNum == -1)
		{
			cgs.be.followingMe = 0;
			break;
		}
		else if (clientNum >= 0 && clientNum < MAX_CLIENTS && clientNum != cg.snap->ps.clientNum)
		{
			cgs.be.followingMe |= (1 << clientNum);
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
CG_ReplacePlayerNameInChat

Replace player name in chat message with replacement name if cg_giveName is enabled.
=================
*/
static void CG_ReplacePlayerNameInChat(char* text, int textSize, int clientNum)
{
	const char* colonPos;
	const clientInfo_t* ci;
	const char* replacementName;

	if (!cg_giveName.integer)
	{
		return;
	}

	/* Validate clientNum */
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	/* Never replace name for local player */
	if (clientNum == cg.clientNum)
	{
		return;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return;
	}

	/* Find colon position */
	colonPos = strchr(text, ':');
	if (!colonPos)
	{
		return;
	}

	/* Check if this player has a replacement name */
	/* Replacement names end with '*' character in name_clean */
	{
		int nameCleanLen;
		char replacementNameClean[MAX_QPATH];
		char tempName[MAX_QPATH];
		const char* nameInCiName;
		const char* nameStart;
		const char* nameEnd;
		int nameWithColorLen;

		nameCleanLen = strlen(ci->name_clean);

		/* Check if name_clean ends with '*' (replacement marker) */
		if (nameCleanLen == 0 || ci->name_clean[nameCleanLen - 1] != '*')
		{
			return;
		}

		/* Player has replacement name */
		/* Extract replacement name from name_clean (without the '*' marker) */
		Q_strncpyz(replacementNameClean, ci->name_clean, sizeof(replacementNameClean));
		if (nameCleanLen > 0 && replacementNameClean[nameCleanLen - 1] == '*')
		{
			replacementNameClean[nameCleanLen - 1] = 0; /* Remove '*' */
		}

		/* Check if ci->name starts with replacement name (no ID/XID prefixes) */
		/* If it does, we can use it directly (it should be in format ^Xname^0*) */
		if (strstr(ci->name, replacementNameClean) == ci->name ||
		        (ci->name[0] == '^' && strstr(ci->name + 2, replacementNameClean) == ci->name + 2))
		{
			/* ci->name contains replacement name directly, extract it without ^0* */
			nameEnd = strstr(ci->name, "^0*");
			if (nameEnd)
			{
				/* Extract name up to (but not including) "^0*" to preserve color code */
				nameWithColorLen = nameEnd - ci->name;
				if (nameWithColorLen > 0 && nameWithColorLen < sizeof(tempName) - 3)
				{
					memcpy(tempName, ci->name, nameWithColorLen);
					tempName[nameWithColorLen] = 0;
					/* Add ^7 at the end for chat to reset color */
					Q_strcat(tempName, sizeof(tempName), "^7");
					replacementName = tempName;
				}
				else
				{
					/* Fallback: use replacementNameClean with default color */
					Com_sprintf(tempName, sizeof(tempName), "^7%s^7", replacementNameClean);
					replacementName = tempName;
				}
			}
			else
			{
				/* Fallback: use replacementNameClean with default color */
				Com_sprintf(tempName, sizeof(tempName), "^7%s^7", replacementNameClean);
				replacementName = tempName;
			}
		}
		else
		{
			/* ci->name has ID/XID prefixes, find the replacement name in it */
			nameInCiName = strstr(ci->name, replacementNameClean);
			if (nameInCiName)
			{
				/* Find the start of the name (go back to find color code) */
				nameStart = nameInCiName;
				/* Go back to find the color code (^X where X is 0-9) */
				while (nameStart > ci->name && nameStart[-1] != '^' && nameStart[-1] != ':' && nameStart[-1] != ']' && nameStart[-1] != '[')
				{
					nameStart--;
				}
				/* If we found '^', verify it's a color code and include it */
				if (nameStart > ci->name && nameStart[-1] == '^' && nameStart[0] >= '0' && nameStart[0] <= '9')
				{
					nameStart--; /* Include the '^' (color digit is already at nameStart[0]) */
				}

				/* Find where the name ends (before '^0*' sequence) */
				nameEnd = strstr(nameInCiName, "^0*");
				if (nameEnd)
				{
					/* Extract name up to (but not including) "^0*" to preserve color code */
					nameWithColorLen = nameEnd - nameStart;
					if (nameWithColorLen > 0 && nameWithColorLen < sizeof(tempName) - 3)
					{
						memcpy(tempName, nameStart, nameWithColorLen);
						tempName[nameWithColorLen] = 0;
						/* Add ^7 at the end for chat to reset color */
						Q_strcat(tempName, sizeof(tempName), "^7");
						replacementName = tempName;
					}
					else
					{
						/* Fallback: use replacementNameClean with default color */
						Com_sprintf(tempName, sizeof(tempName), "^7%s^7", replacementNameClean);
						replacementName = tempName;
					}
				}
				else
				{
					/* Fallback: use replacementNameClean with default color */
					Com_sprintf(tempName, sizeof(tempName), "^7%s^7", replacementNameClean);
					replacementName = tempName;
				}
			}
			else
			{
				/* Fallback: use replacementNameClean with default color */
				Com_sprintf(tempName, sizeof(tempName), "^7%s^7", replacementNameClean);
				replacementName = tempName;
			}
		}

		/* Build new message: preserve prefix + replacement name + colon + rest of message */
		{
			char newText[1024];
			int replacementLen;
			int restLen;
			const char* restOfMessage;
			const char* nameStartInText;
			int prefixLen;

			restOfMessage = colonPos + 1; /* Skip colon */
			restLen = strlen(restOfMessage);
			replacementLen = strlen(replacementName);

			/* Check if there's a prefix like "(team) " at the start */
			nameStartInText = NULL;
			if (text[0] == '(')
			{
				/* Find the closing parenthesis and space after it */
				nameStartInText = strchr(text, ')');
				if (nameStartInText && nameStartInText[1] == ' ')
				{
					prefixLen = nameStartInText - text + 2; /* Include ") " */
				}
				else
				{
					prefixLen = 0;
				}
			}
			else
			{
				prefixLen = 0;
			}

			/* Copy prefix if present, then add replacement name */
			if (prefixLen > 0 && prefixLen < sizeof(newText) - 1)
			{
				/* Copy prefix to newText */
				memcpy(newText, text, prefixLen);
				newText[prefixLen] = 0;

				/* Append replacement name, colon, and rest of message */
				if (prefixLen + replacementLen + 1 + restLen < sizeof(newText))
				{
					Com_sprintf(newText + prefixLen, sizeof(newText) - prefixLen, "%s:%s", replacementName, restOfMessage);
					Q_strncpyz(text, newText, textSize);
				}
			}
			else
			{
				/* No prefix, just replace name */
				if (replacementLen + 1 + restLen < sizeof(newText))
				{
					/* Copy tell prefix if present (special case for tell messages) */
					if (text[0] == 25 && (text[1] == '(' || text[1] == '['))
					{
						newText[0] = text[0];
						newText[1] = text[1];
						Com_sprintf(newText + 2, sizeof(newText) - 2, "%s:%s", replacementName, restOfMessage);
					}
					else
					{
						Com_sprintf(newText, sizeof(newText), "%s:%s", replacementName, restOfMessage);
					}

					Q_strncpyz(text, newText, textSize);
				}
			}
		}
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
	char        text[1024];

	cmd = CG_Argv(0);
	if (!cmd)
	{
		return;
	}
//psohtua
	if (strcmp(cmd, "psohtua") == 0)
	{
		return;
	}
//cp
	if (strcmp(cmd, "cp") == 0)
	{
		const char* arg;
		arg = CG_Argv(1);
		if (!strstr(arg, "Multi-view demo created with"))
		{
			CG_CenterPrint(arg, 120, SMALLCHAR_WIDTH);
		}
		return;
	}
//cs
	if (strcmp(cmd, "cs") == 0)
	{
		CG_ConfigStringModified();
		return;
	}
// print
	if (strcmp(cmd, "print") == 0)
	{
		Q_strncpyz(text, CG_Argv(1), sizeof(text));
		CG_RemoveChatEscapeChar(text);

		// фильтрация сообщений сервера к3мск по маске "had <число> health and <число> armor left"
		if (cg_ignoreServerMessages.integer & 1)
		{
			// Проверка фильтра "had <число> health and <число> armor left"
			const char* had_pos = strstr(text, " had ");
			if (had_pos)
			{
				size_t nick_len = had_pos - text;
				if (nick_len > 0)
				{
					return;
				}
			}
		}

		if (cg_ignoreServerMessages.integer & 2)
		{
			if (strstr(text, "chat fragged") != NULL)
			{
				return;
			}
		}
		CG_Printf("%s", text);
		return;
	}
//chat
	if (strcmp(cmd, "chat") == 0)
	{
		messageAllowed_t ma;
		qboolean isVanilaChatEnabled;
		qboolean isShudChatEnabled;

		Q_strncpyz(text, CG_Argv(1), 1024);

		ma = CG_ChatCheckMessageAllowed(text);
		if (ma == MESSAGE_NOTALLOWED)
		{
			return;
		}

		if ((ma != MESSAGE_ALLOWED_PLAYER) && (ma != MESSAGE_NOTALLOWED) && (cg_ignoreServerMessages.integer & 4))
		{
			return;
		}

		/* Replace player name with replacement name if cg_giveName is enabled */
		/* Get clientNum from command (CG_Argv(2) if available) */
		{
			int chatClientNum;
			int argc;

			argc = trap_Argc();
			if (argc >= 3)
			{
				chatClientNum = atoi(CG_Argv(2));
				if (chatClientNum >= 0 && chatClientNum < MAX_CLIENTS)
				{
					CG_ReplacePlayerNameInChat(text, sizeof(text), chatClientNum);
				}
			}
		}

		isVanilaChatEnabled = cg_chatEnable.integer & CG_CHAT_COMMMON_ENABLED;
		isShudChatEnabled = cg_shudChatEnable.integer & CG_CHAT_COMMMON_ENABLED && ma == MESSAGE_ALLOWED_PLAYER;

		if (!isVanilaChatEnabled && !isShudChatEnabled)
		{
			return;
		}

		if (!cg_nochatbeep.integer)
		{
			trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		}

		CG_RemoveChatEscapeChar(text);

		/* Insert default chat color at the beginning of message text (after colon) */
		/* This replaces any server-provided color with our default color */
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

		if (isVanilaChatEnabled)
		{
			CG_Printf("%s\n", text);
		}
		if (isShudChatEnabled)
		{
			CG_SHUDEventChat(text);
		}
		return;
	}
//tchat
	if (strcmp(cmd, "tchat") == 0)
	{
		messageAllowed_t ma;
		qboolean isVanilaChatEnabled;
		qboolean isShudChatEnabled;

		Q_strncpyz(text, CG_Argv(1), 1024);
		ma = CG_ChatCheckMessageAllowed(text);
		if (ma == MESSAGE_NOTALLOWED)
		{
			return;
		}

		if ((ma != MESSAGE_ALLOWED_PLAYER) && (ma != MESSAGE_NOTALLOWED) && (cg_ignoreServerMessages.integer & 4))
		{
			return;
		}

		/* Replace player name with replacement name if cg_giveName is enabled */
		/* Get clientNum from command (CG_Argv(2) if available) */
		{
			int chatClientNum;
			int argc;

			argc = trap_Argc();
			if (argc >= 3)
			{
				chatClientNum = atoi(CG_Argv(2));
				if (chatClientNum >= 0 && chatClientNum < MAX_CLIENTS)
				{
					CG_ReplacePlayerNameInChat(text, sizeof(text), chatClientNum);
				}
			}
		}

		isVanilaChatEnabled = cg_chatEnable.integer & CG_CHAT_TEAM_ENABLED;
		isShudChatEnabled = cg_shudChatEnable.integer & CG_CHAT_TEAM_ENABLED && ma == MESSAGE_ALLOWED_PLAYER;

		if (!isVanilaChatEnabled && !isShudChatEnabled)
		{
			return;
		}

		if (!cg_noTeamChatBeep.integer)
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
			CG_SHUDEventChat(text);
		}

		return;
	}
//scores
	if (Q_stricmp(cmd, "scores") == 0)
	{
		CG_ParseScores();
		return;
	}
//tinfo
	if (Q_stricmp(cmd, "tinfo") == 0)
	{
		CG_ParseTeamInfo();
		return;
	}
//map_restart
	if (Q_stricmp(cmd, "map_restart") == 0)
	{
		CG_MapRestart();
		return;
	}
//remapShader
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
//statsinfo
	if (Q_stricmp(cmd, "statsinfo") == 0)
	{
		CG_BEParseStatsInfo();
		return;
	}
//viewlist
	if (Q_stricmp(cmd, "viewlist") == 0)
	{
		return;
	}
//sset
	if (Q_stricmp(cmd, "sset") == 0)
	{
		CG_OSPPrintSSet();
		return;
	}
//viewactive
	if (Q_stricmp(cmd, "viewactive") == 0)
	{
		CG_Printf("viewactive is not implimented\n");
		return;
	}
//stuff
	if (Q_stricmp(cmd, "stuff") == 0)
	{
		CG_ServerCommandStuff();
		return;
	}
//crecord
	if (Q_stricmp(cmd, "crecord") == 0)
	{
		return;
	}
//cscreenshot
	if (Q_stricmp(cmd, "cscreenshot") == 0)
	{
		return;
	}
//dmgplum - damage plum for projectile weapons (GL/RL) via server command
	if (Q_stricmp(cmd, "dmgplum_proj") == 0)
	{
		int damage;
		int argc;
		vec3_t position;

		// Only process if enabled (1 = normal, 2 = stacking)
		if (cg_damagePlums.integer < 1)
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
//xstats1
	if (Q_stricmp(cmd, "xstats1") == 0)
	{
		if (cgs.be.statsAllRequested)
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
//specsinfo
	if (Q_stricmp(cmd, "specsinfo") == 0)
	{
		CG_ParseSpecsInfo();
		return;
	}
//astats
	if (Q_stricmp(cmd, "astats") == 0)
	{
		CG_OSPAstats(qtrue);
		return;
	}
//astatsb
	if (Q_stricmp(cmd, "astatsb") == 0)
	{
		return;
	}
//bstats
	if (Q_stricmp(cmd, "bstats") == 0)
	{
		CG_OSPBstats(qtrue);
		return;
	}
//bstatsb
	if (Q_stricmp(cmd, "bstatsb") == 0)
	{
		return;
	}
//clientLevelShot
	if (Q_stricmp(cmd, "clientLevelShot") == 0)
	{
		cg.levelShot = qtrue;
		return;
	}
// Server sharing console comands
	if (Q_stricmp(cmd, "scmds") == 0)
	{
		CG_HandleScmdsCommand();
		return;
	}
//UKNOWN COMMAND
//	CG_Printf("Unknown client game command: %s\n", cmd);
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
	if (last_xstats1_sequence == latestSequence && cgs.be.statsAllRequested)
	{
		cgs.be.statsAllRequested = qfalse;
		last_xstats1_sequence = -1;
	}
}
