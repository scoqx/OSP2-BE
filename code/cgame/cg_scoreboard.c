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
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

vec4_t scoreboard_rtColor = {1, 0, 0, 1};
vec4_t scoreboard_btColor = {0, 0, 1, 1};

qboolean isCustomScoreboardColorIsSet_rt;
qboolean isCustomScoreboardColorIsSet_bt;
qboolean isCustomScoreboardColorIsSet_spec;
vec4_t scoreboard_rtColorBody = {1, 0, 0, 1};
vec4_t scoreboard_btColorBody = {0, 0, 1, 1};
vec4_t scoreboard_rtColorTitle = {1, 0, 0, 1};
vec4_t scoreboard_btColorTitle = {0, 0, 1, 1};
vec4_t scoreboard_specColor = {0, 0, 1, 1};

#define SCOREBOARD_X        (0)

#define SB_HEADER           86
#define SB_TOP              (SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR        420

#define SB_NORMAL_HEIGHT    40
#define SB_INTER_HEIGHT     16 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X   (SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X      (SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X  (SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X     (SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X        (SCOREBOARD_X+32)
#define SB_HEAD_X           (SCOREBOARD_X+64)

#define SB_SCORELINE_X      80

#define SB_RATING_WIDTH     (6 * BIGCHAR_WIDTH) // width 6
#define SB_SCORE_X          (SB_SCORELINE_X + BIGCHAR_WIDTH) // width 6
#define SB_RATING_X         (SB_SCORELINE_X + 6 * BIGCHAR_WIDTH) // width 6
#define SB_PING_X           (SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8) // width 5
#define SB_TIME_X           (SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8) // width 5
#define SB_NAME_X           (SB_SCORELINE_X + 22 * BIGCHAR_WIDTH) // width 15

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//
//  wins/losses are drawn on bot icon now

qboolean localClient = 0; // true if local client has been displayed


int sumScoresBlue;
int sumScoresRed;
int sumPingBlue;
int sumPingRed;
int sumThawsBlue;
int sumThawsRed;

/*
=================
CG_DrawScoreboard
=================
*/
void CG_DrawClientScore(int y, score_t* score, float* color, float fade, qboolean largeFormat)
{
	char    string[1024];
	vec3_t  headAngles;
	clientInfo_t*    ci;
	int iconx, headx;

	if (score->client < 0 || score->client >= cgs.maxclients)
	{
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + BIGCHAR_WIDTH; //48
	headx = SB_HEAD_X + BIGCHAR_WIDTH; // 80

	// draw the handicap or bot skill marker (unless player has flag)
	if (ci->powerups & (1 << PW_NEUTRALFLAG))
	{
		if (largeFormat)
		{
			CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_FREE, qfalse);
		}
		else
		{
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_FREE, qfalse);
		}
	}
	else if (ci->powerups & (1 << PW_REDFLAG))
	{
		if (largeFormat)
		{
			CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_RED, qfalse);
		}
		else
		{
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_RED, qfalse);
		}
	}
	else if (ci->powerups & (1 << PW_BLUEFLAG))
	{
		if (largeFormat)
		{
			CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_BLUE, qfalse);
		}
		else
		{
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_BLUE, qfalse);
		}
	}
	else
	{
		if (ci->botSkill > 0 && ci->botSkill <= 5)
		{
			if (cg_drawIcons.integer)
			{
				if (largeFormat)
				{
					CG_DrawPicOld(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				}
				else
				{
					CG_DrawPicOld(iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				}
			}
		}
		else if (ci->handicap < 100)
		{
			Com_sprintf(string, sizeof(string), "%i", ci->handicap);
			if (cgs.gametype == GT_TOURNAMENT)
				CG_DrawSmallStringColor(iconx, y, string, color, DS_HLEFT | DS_VCENTER, 2);
			else
				CG_DrawSmallStringColor(iconx, y, string, color, DS_HLEFT, 2);
		}

		// draw the wins / losses
		if (cgs.gametype == GT_TOURNAMENT)
		{
			int score_x = iconx;
			if (cg_scoreboardShowId.integer)
				score_x -= 20;

			Com_sprintf(string, sizeof(string), "%i/%i", ci->wins, ci->losses);
			if (ci->handicap < 100 && !ci->botSkill)
			{
				CG_DrawSmallStringColor(score_x, y, string, color, DS_HLEFT | DS_VCENTER, 2);
			}
			else
			{
				CG_DrawSmallStringColor(score_x, y, string, color, DS_HLEFT, 2);
			}
		}

	}

	// draw the face
	VectorClear(headAngles);

	headAngles[YAW] = 180;

	if (score->client == cg.snap->ps.clientNum)
	{
		headAngles[YAW] = (float)cg.time / 14 + 180;
	}
	else
	{
		headAngles[YAW] = 180;
	}

	if (largeFormat)
	{
		CG_DrawHead(headx, y - (float)(ICON_SIZE - BIGCHAR_HEIGHT) / 2, ICON_SIZE, ICON_SIZE, score->client, headAngles);
	}
	else
	{
		CG_DrawHead(headx, y, 16, 16, score->client, headAngles);
	}
	// draw the score line
	if (score->ping == -1)
	{
		Com_sprintf(string, sizeof(string), "^2 connecting^7    ");
	}
	else if (ci->st)
	{
		Com_sprintf(string, 1024, " %s.COACH^7 %3i %4i", ci->st == 1 ? "^1R" : "^2B", score->ping, score->time);
	}
	else if (ci->team == TEAM_6 || ci->team == TEAM_7)
	{
		Com_sprintf(string, 1024, " %s.SPEC^7%3i %4i", ci->team == TEAM_6 ? "^1R" : "^2B", score->ping, score->time);
	}
	else if (ci->team == TEAM_SPECTATOR && !CG_OSPIsGameTypeCA(cgs.gametype))
	{
		Com_sprintf(string, sizeof(string), "^5 SPECT^7 %3i %4i", score->ping, score->time);
	}
	else
	{
		Com_sprintf(string, sizeof(string), "^7%5i %4i %4i", score->score, score->ping, score->time);
	}

	// highlight your position
	if (score->client == cg.snap->ps.clientNum)
	{
		float   hcolor[4];
		int     rank;

		localClient = qtrue;

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
		        || cgs.gametype >= GT_TEAM)
		{
			rank = -1;
		}
		else
		{
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if (rank == 0)
		{
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		}
		else if (rank == 1)
		{
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		}
		else if (rank == 2)
		{
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		}
		else
		{
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect(SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2.0), y,
		            640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT + 1, hcolor);
		// 640 - 80 - 16 = 544 - 384 = 256
		// 128 + 16 * 16 = 384
	}
	if (cg_scoreboardShowId.integer)
	{
		int idX = 20 + ICON_SIZE + 8;
		char playerID[MAX_QPATH];
		Com_sprintf(playerID, sizeof(playerID), "%i", score->client);
		CG_FontSelect(0);
		CG_OSPDrawString(idX, y + 2, playerID, colorWhite, 14, 14, SCREEN_WIDTH, DS_HRIGHT | DS_SHADOW | DS_PROPORTIONAL, NULL);
	}

	CG_FontSelect(0);
	CG_OSPDrawString(128, y, string, color, 16, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);

	CG_FontSelect(2);
	CG_OSPDrawString(128 + 16 * 16, y, ci->name, color, 16, 16, 256, DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL, NULL);

	// add the "ready" marker for intermission exiting
	if (cg.warmup == 0 && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return;
	}
	if (cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client))
	{
		CG_DrawBigString(iconx, y, "^3READY", 1.0f, DS_HLEFT | DS_SHADOW, 0);
	}
}

/*
=================
CG_BEDrawClientScore
=================
*/
void CG_BEDrawClientScore(int y, score_t* score, float* color, float fade, qboolean largeFormat)
{
	char string[1024], string2[1024], string3[1024];
	vec3_t headAngles;
	clientInfo_t* ci;
	int iconx, headx;
	float bWidth = 14;
	float bHeight = 16;
	int font = cg_scoreboardFont.integer;
	int proportional = (cg_scoreboardBE.integer & 1) ? DS_PROPORTIONAL : 0;

	if (score->client < 0 || score->client >= cgs.maxclients)
	{
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}
	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + BIGCHAR_WIDTH;
	headx = SB_HEAD_X + BIGCHAR_WIDTH;

	CG_FontSelect(font);

	if (ci->powerups & (1 << PW_NEUTRALFLAG) ||
	        ci->powerups & (1 << PW_REDFLAG) ||
	        ci->powerups & (1 << PW_BLUEFLAG))
	{
		int team = TEAM_FREE;
		if (ci->powerups & (1 << PW_REDFLAG))
		{
			team = TEAM_RED;
		}
		else if (ci->powerups & (1 << PW_BLUEFLAG))
		{
			team = TEAM_BLUE;
		}
		if (largeFormat)
		{
			CG_DrawFlagModel(iconx, y - (32 - bHeight) / 2, 32, 32, team, qfalse);
		}
		else
		{
			CG_DrawFlagModel(iconx, y, 16, 16, team, qfalse);
		}
	}
	else
	{
		if (ci->botSkill > 0 && ci->botSkill <= 5)
		{
			if (cg_drawIcons.integer)
			{
				if (largeFormat)
				{
					CG_DrawPicOld(iconx, y - (32 - bHeight) / 2, 32, 32, cgs.media.botSkillShaders[ci->botSkill - 1]);
				}
				else
				{
					CG_DrawPicOld(iconx, y, 16, 16, cgs.media.botSkillShaders[ci->botSkill - 1]);
				}
			}
		}
		else if (ci->handicap < 100)
		{
			Com_sprintf(string, sizeof(string), "%i", ci->handicap);
			if (cgs.gametype == GT_TOURNAMENT)
				CG_OSPDrawStringNew(iconx - 10, y + 1, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_HCENTER | proportional, NULL, NULL, NULL);
			else
				CG_OSPDrawStringNew(iconx - 10, y + 1, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_SHADOW | proportional, NULL, NULL, NULL);
		}

		if (cgs.gametype == GT_TOURNAMENT)
		{
			int score_x = iconx;
			if (cg_scoreboardShowId.integer)
				score_x -= 20;

			Com_sprintf(string, sizeof(string), "%i/%i", ci->wins, ci->losses);
			if (ci->handicap < 100 && !ci->botSkill)
				CG_OSPDrawStringNew(score_x, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_SHADOW | DS_HCENTER | proportional, NULL, NULL, NULL);
			else
				CG_OSPDrawStringNew(score_x, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_SHADOW | proportional, NULL, NULL, NULL);
		}
	}

	VectorClear(headAngles);
	headAngles[YAW] = (score->client == cg.snap->ps.clientNum)
	                  ? ((float)cg.time / 14 + 180) : 180;

	if (largeFormat)
		CG_DrawHead(headx, y - (float)(ICON_SIZE - BIGCHAR_HEIGHT) / 2, ICON_SIZE, ICON_SIZE, score->client, headAngles);
	else
		CG_DrawHead(headx, y, 16, 16, score->client, headAngles);

	if (score->ping == -1)
	{
		Com_sprintf(string, sizeof(string), "^2 connecting^7    ");
	}
	else if (ci->st)
	{
		Com_sprintf(string, sizeof(string), " %s.COACH^7", (ci->st == 1 ? "^1R" : "^2B"));
		Com_sprintf(string2, sizeof(string2), "%i", score->ping);
		Com_sprintf(string3, sizeof(string3), "%i", score->time);
	}
	else if (ci->team == TEAM_6 || ci->team == TEAM_7)
	{
		Com_sprintf(string, sizeof(string), " %s.SPEC^7", (ci->team == TEAM_6 ? "^1R" : "^2B"));
		Com_sprintf(string2, sizeof(string2), "%i", score->ping);
		Com_sprintf(string3, sizeof(string3), "%i", score->time);
	}
	else if (ci->team == TEAM_SPECTATOR && !CG_OSPIsGameTypeCA(cgs.gametype))
	{
		Com_sprintf(string, sizeof(string), "^5 SPECT^7");
		Com_sprintf(string2, sizeof(string2), "%i", score->ping);
		Com_sprintf(string3, sizeof(string3), "%i", score->time);
	}
	else
	{
		Com_sprintf(string, sizeof(string), "^7%i", score->score);
		Com_sprintf(string2, sizeof(string2), "%i", score->ping);
		Com_sprintf(string3, sizeof(string3), "%i", score->time);
	}


	if (score->client == cg.snap->ps.clientNum)
	{
		float hcolor[4];
		int rank;

		localClient = qtrue;
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.gametype >= GT_TEAM)
			rank = -1;
		else
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;

		switch (rank)
		{
			case 0:
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 0.7f;
				break;
			case 1:
				hcolor[0] = 0.7f;
				hcolor[1] = 0;
				hcolor[2] = 0;
				break;
			case 2:
				hcolor[0] = 0.7f;
				hcolor[1] = 0.7f;
				hcolor[2] = 0;
				break;
			default:
				hcolor[0] = 0.7f;
				hcolor[1] = 0.7f;
				hcolor[2] = 0.7f;
				break;
		}
		hcolor[3] = fade * 0.7;
		CG_FillRect(SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2.0), y,
		            640 - SB_SCORELINE_X - bWidth, bHeight + 1, hcolor);
	}

	if (cg_scoreboardShowId.integer)
	{
		int idX = 20 + ICON_SIZE + 8;
		char playerID[MAX_QPATH];
		Com_sprintf(playerID, sizeof(playerID), "%i", score->client);
		CG_OSPDrawStringNew(idX, y + 2, playerID, color, colorBlack, bWidth - 2, bHeight - 2, SCREEN_WIDTH,
		                    DS_SHADOW | DS_HRIGHT | proportional, NULL, NULL, NULL);
	}

	CG_OSPDrawStringNew(206, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH,
	                    DS_HRIGHT | DS_SHADOW | proportional, NULL, NULL, NULL);
	CG_OSPDrawStringNew(286, y, string2, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH,
	                    DS_HRIGHT | DS_SHADOW | proportional, NULL, NULL, NULL);
	CG_OSPDrawStringNew(366, y, string3, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH,
	                    DS_HRIGHT | DS_SHADOW | proportional, NULL, NULL, NULL);

	CG_OSPDrawStringNew(128 + 4 + 16 * 16, y, ci->name, color, colorBlack, bWidth, bHeight, 256,
	                    DS_SHADOW | proportional, NULL, NULL, NULL);


	if (cg.warmup != 0 || cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		if (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client))
		{
			const vec4_t readyColor = { 0.5f, 0.5f, 0.5f, 0.5f };
			CG_OSPDrawStringNew(iconx, y, "^3READY", readyColor, colorBlack, bWidth, bHeight, 256,
			                    DS_SHADOW | proportional, NULL, NULL, NULL);
		}
	}
}


void CG_DrawClientScoreNew(int y, score_t* score, float* color, float fade, qboolean largeFormat)
{
	if (cg_scoreboardBE.integer)
		CG_BEDrawClientScore(y, score, color, fade, largeFormat);
	else
		CG_DrawClientScore(y, score, color, fade, largeFormat);
}

/*
=================
CG_TeamScoreboard
isn't for team. Uses only for single player modes
=================
*/
int CG_TeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight)
{
	int     i;
	score_t* score;
	vec4_t   color1;
	int     count;
	clientInfo_t*    ci;

	qboolean isGametypeCA;
	qboolean isTeamSpectator;
	isTeamSpectator = team == TEAM_SPECTATOR;

	color1[0] = color1[1] = color1[2] = 1.0;
	color1[3] = fade;

	isGametypeCA = cgs.gametype == GT_CA;

	count = 0;
	for (i = 0 ; i < cg.numScores && count < maxClients ; i++)
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];
		if (team == ci->team || (isGametypeCA && ci->rt == team))
		{
			qboolean isClientReady;

			isClientReady = (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client)) != 0;

			if ((!isGametypeCA && !isClientReady) || (isGametypeCA && !isTeamSpectator))
			{
				CG_DrawClientScoreNew(y + lineHeight * count, score, color1, fade, lineHeight == 40);
			}
			else if ((!isGametypeCA && isClientReady) || (isGametypeCA && isTeamSpectator))
			{
				vec4_t   color2 = { 0.2f, 0.2f, 0.2f, 0.5f };
				CG_DrawClientScoreNew(y + lineHeight * count, score, color2, fade, lineHeight == 40);
			}
			++count;
		}
	}
	return count;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard(void)
{
	int     x, y, w, i, n1, n2;
	float   fade;
	float*   fadeColor;
	char*    s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	int font = cg_scoreboardFont.integer;

	if (cg_hideScores.integer)
	{
		return qfalse;
	}

	// don't draw amuthing if the menu or console is up
	if (cg_paused.integer)
	{
		return qfalse;
	}

	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		return qfalse;
	}

	if (cg.warmup && !cg.showScores && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return qfalse;
	}

	if (cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
	        cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		fade = cg_scoreTransparency.value;
		fadeColor = colorWhite;
	}
	else
	{
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor)
		{
			// next time scoreboard comes up, don't print killer
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	if (cg.demoPlayback != 0)
	{
		CG_DrawBigString(SCREEN_WIDTH / 2.0f, 40, "^3Demo Playback", fade, DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, font);
	}
	// fragged by ... line
	else if (cg.killerName[0])
	{
		s = va("Fragged by %s", cg.killerName);
		CG_DrawBigString(SCREEN_WIDTH / 2.0f, 40, s, fade, DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, font);
	}

	// current rank
	if (cgs.gametype < GT_TEAM)
	{
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		{
			s = va("%s place with %i",
			       CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1),
			       cg.snap->ps.persistant[PERS_SCORE]);
			CG_DrawBigString(SCREEN_WIDTH / 2.0f, 60, s, fade, DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, font);
		}
	}
	else
	{
		if (cg.teamScores[0] == cg.teamScores[1])
		{
			s = va("Teams are tied at %i", cg.teamScores[0]);
		}
		else if (cg.teamScores[0] >= cg.teamScores[1])
		{
			s = va("Red leads %i to %i", cg.teamScores[0], cg.teamScores[1]);
		}
		else
		{
			s = va("Blue leads %i to %i", cg.teamScores[1], cg.teamScores[0]);
		}

		CG_DrawBigString(SCREEN_WIDTH / 2.0f, 60, s, fade, DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, font);
	}

	// scoreboard
	y = SB_HEADER;

	CG_DrawPicOld(SB_SCORE_X + (SB_RATING_WIDTH / 2.0), y, 64, 32, cgs.media.scoreboardScore);
	CG_DrawPicOld(SB_PING_X - (SB_RATING_WIDTH / 2.0), y, 64, 32, cgs.media.scoreboardPing);
	CG_DrawPicOld(SB_TIME_X - (SB_RATING_WIDTH / 2.0), y, 64, 32, cgs.media.scoreboardTime);
	CG_DrawPicOld(SB_NAME_X - (SB_RATING_WIDTH / 2.0), y, 64, 32, cgs.media.scoreboardName);

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if (cg.numScores > SB_MAXCLIENTS_NORMAL)
	{
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	}
	else
	{
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	localClient = qfalse;

	if (cgs.gametype >= GT_TEAM)
	{
		//
		// teamplay scoreboard
		//
		y += lineHeight / 2;

		if (cg.teamScores[0] >= cg.teamScores[1])
		{
			n1 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		else
		{
			n1 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;

		}

		n1 = CG_TeamScoreboard(y, TEAM_6, fade, maxClients, lineHeight);
		y += n1 * lineHeight + BIGCHAR_HEIGHT;
		n1 = CG_TeamScoreboard(y, TEAM_7, fade, maxClients, lineHeight);
		y += n1 * lineHeight + BIGCHAR_HEIGHT;
		n1 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight);
		y += n1 * lineHeight + BIGCHAR_HEIGHT;
	}
	else
	{
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight);
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient)
	{
		// draw local client at the bottom
		for (i = 0 ; i < cg.numScores ; i++)
		{
			if (cg.scores[i].client == cg.snap->ps.clientNum)
			{
				CG_DrawClientScore(y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT);
				break;
			}
		}
	}
	if (cg_drawAccuracy.integer)
		CG_DrawWeaponStatsWrapper();
	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
void CG_CenterGiantLine(float y, const char* string)
{
	float       x;
	vec4_t      color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(string));

	CG_DrawStringExt(x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard(void)
{
	const char*      s;
	vec4_t          color;
	int             min, tens, ones;
	clientInfo_t*    ci;
	int             y;
	int             i;

	if (cg_hideScores.integer)
	{
		return ;
	}

	// request more scores regularly
	if (cg.scoresRequestTime + 2000 < cg.time)
	{
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");
	}
	if (cg_drawAccuracy.integer && !cg.showAccuracy && cg.statsRequestTime + 2500 < cg.time)
	{
		cg.statsRequestTime = cg.time;
		trap_SendClientCommand("getstatsinfo");
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);

	// print the mesage of the day
	s = CG_ConfigString(CS_MOTD);
	if (!s[0])
	{
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine(8, s);

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones);

	CG_CenterGiantLine(64, s);


	// print the two scores

	y = 160;
	if (cgs.gametype >= GT_TEAM)
	{
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt(8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
		s = va("%i", cg.teamScores[0]);
		CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

		y += 64;

		CG_DrawStringExt(8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
		s = va("%i", cg.teamScores[1]);
		CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
	}
	else
	{
		//
		// free for all scoreboard
		//
		for (i = 0 ; i < MAX_CLIENTS ; i++)
		{
			ci = &cgs.clientinfo[i];
			if (!ci->infoValid)
			{
				continue;
			}
			if (ci->team != TEAM_FREE)
			{
				continue;
			}

			CG_DrawStringExt(8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
			s = va("%i", ci->score);
			CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
			y += 64;
		}
	}
}
// todel
// void CG_OSPShowStatsInfo(void)
// {
//  char args[1024];
//  char strings[24][128];
//  int i;
//  int arg_cnt;
//  int tmp;
//  float tmpf;
//  int row;
//  float effiency;
//  int w_hits;
//  int w_attacks;
//  int w_kills;
//  int w_deaths;
//  float kd;
//  float damageRatio;
//  qboolean flag = qfalse;

//  if (!cg.demoPlayback && wstatsWndId <= 0 && !wstatsEnabled)
//  {
//      return;
//  }

//  wstatsEnabled = qfalse;

//  for (i = 0; i < 24; ++i)
//  {
//      trap_Argv(i + 1, args, 1024);
//      statsInfo[i] = atoi(args);

//      if (i == 0 && statsInfo[i] == 0)
//      {
//          return;
//      }
//  }
//  if (!CG_OSPIsGameTypeCA(cgs.gametype) && !cgs.osp.gameTypeFreeze)
//  {
//      tmp = statsInfo[OSP_STATS_SCORE];
//  }
//  else
//  {
//      tmp = statsInfo[OSP_STATS_KILLS];
//  }

//  if ((tmp + statsInfo[OSP_STATS_DEATHS]) == 0)
//  {
//      effiency = 0;
//  }
//  else
//  {
//      effiency = 100.0f * tmp / (statsInfo[OSP_STATS_DEATHS] + tmp);
//      if (effiency < 0) effiency = 0;
//  }
//  if (statsInfo[OSP_STATS_DMG_GIVEN] > 0 || statsInfo[OSP_STATS_DMG_RCVD] > 0)
//  {
//      damageRatio = (float)statsInfo[OSP_STATS_DMG_GIVEN] /
//                    (statsInfo[OSP_STATS_DMG_RCVD] > 0 ? statsInfo[OSP_STATS_DMG_RCVD] : 1);
//  }

//  kd = (statsInfo[OSP_STATS_KILLS] > 0 && (statsInfo[OSP_STATS_DEATHS] + statsInfo[OSP_STATS_SUCIDES]) == 0) ?
//       (float)statsInfo[OSP_STATS_KILLS] :
//       ((statsInfo[OSP_STATS_DEATHS] + statsInfo[OSP_STATS_SUCIDES]) > 0) ?
//       (float)statsInfo[OSP_STATS_KILLS] / (statsInfo[OSP_STATS_DEATHS] + statsInfo[OSP_STATS_SUCIDES]) : 0.0f;

//  if (cgs.gametype == GT_TOURNAMENT)
//  {
//      strcpy(&strings[0][0], "^B^3Score  ^2Klls ^1Dths Sui   ^3K/D   ^3Effcny ^2WINS ^1LOSSES");
//      strcpy(&strings[1][0], va("%5d  %4d %4d %3d %3.1f %6.1f ^3%4d^7 %6d",
//                                statsInfo[OSP_STATS_SCORE],
//                                statsInfo[OSP_STATS_KILLS],
//                                statsInfo[OSP_STATS_DEATHS],
//                                statsInfo[OSP_STATS_SUCIDES],
//                                kd,
//                                effiency,
//                                statsInfo[OSP_STATS_WINS] & cgs.osp.stats_mask,
//                                statsInfo[OSP_STATS_LOSSES] & cgs.osp.stats_mask));
//  }
//  else if (cgs.gametype == GT_TEAM)
//  {
//      if (cgs.osp.gameTypeFreeze == 0)
//      {
//          strcpy(&strings[0][0], "^B^3Score NET  ^2Klls ^1Dths Sui ^1TmKlls   ^3K/D ^3Effcny");
//          strcpy(&strings[1][0], va("%5d ^5%3d^7  %4d %4d %3d %6d %3.1f %4.1f",
//                                    statsInfo[OSP_STATS_SCORE],
//                                    statsInfo[OSP_STATS_SCORE] - statsInfo[OSP_STATS_DEATHS],
//                                    statsInfo[OSP_STATS_KILLS],
//                                    statsInfo[OSP_STATS_DEATHS],
//                                    statsInfo[OSP_STATS_SUCIDES],
//                                    statsInfo[OSP_STATS_TEAM_KILLS],
//                                    kd,
//                                    effiency));
//      }
//      else
//      {
//          strcpy(&strings[0][0], "^B^3Score WINS  ^2Klls Thws ^1Dths Sui ^1TmKlls   ^3K/D ^3Effcny");
//          strcpy(&strings[1][0], va("%5d ^5%4d^7  %4d %4d %4d %3d %6d %3.1f %4.1f",
//                                    statsInfo[OSP_STATS_SCORE],
//                                    statsInfo[OSP_STATS_WINS] & cgs.osp.stats_mask,
//                                    statsInfo[OSP_STATS_KILLS],
//                                    statsInfo[OSP_STATS_LOSSES] & cgs.osp.stats_mask,
//                                    statsInfo[OSP_STATS_DEATHS],
//                                    statsInfo[OSP_STATS_SUCIDES],
//                                    statsInfo[OSP_STATS_TEAM_KILLS],
//                                    kd,
//                                    effiency));
//      }
//  }
//  else if (cgs.gametype == GT_CTF)
//  {
//      int flag_time_int;
//      int flag_time_float;
//      flag_time_int = statsInfo[OSP_STATS_TIME] / 1000 / 60;
//      flag_time_float = ((float)statsInfo[OSP_STATS_TIME] - 60000.0f * (float)flag_time_int) / 1000.0f;
//      strcpy(&strings[0][0], "^B^3Score  ^2Klls ^1Dths ^3Caps   ^2Ftime Asst Dfns Rtrn KD");
//      strcpy(&strings[1][0], va("%5d  %4d %4d ^3%4d^7 %2d:%02.1f %4d ^5%4d^7 %4d %.1f",
//                                statsInfo[OSP_STATS_SCORE],
//                                statsInfo[OSP_STATS_KILLS],
//                                statsInfo[OSP_STATS_DEATHS],
//                                statsInfo[OSP_STATS_CAPS],
//                                flag_time_int,
//                                flag_time_float,
//                                statsInfo[OSP_STATS_ASSIST],
//                                statsInfo[OSP_STATS_DEFENCES],
//                                statsInfo[OSP_STATS_RETURNS],
//                                kd));

//  }
//  else if (cgs.gametype == GT_CA)
//  {
//      strcpy(&strings[0][0], "^B^3Score  ^2Klls ^1Dths   ^3K/D ^3Effcny ^5DmgScr  ^2WINS");
//      strcpy(&strings[1][0], va("%5d  %4d %4d %3.1f %4.1f ^3%6d^5  %4d",
//                                statsInfo[OSP_STATS_SCORE],
//                                statsInfo[OSP_STATS_KILLS],
//                                statsInfo[OSP_STATS_DEATHS],
//                                kd,
//                                effiency,
//                                statsInfo[OSP_STATS_DMG_GIVEN] / 100,
//                                statsInfo[OSP_STATS_WINS] & cgs.osp.stats_mask));
//  }
//  else
//  {
//      strcpy(&strings[0][0], "^B^3Score  ^2Klls ^1Dths Sui   ^3K/D  ^3Effcny");
//      strcpy(&strings[1][0], va("%5d  %4d %4d %3d %3.1f  %4.1f",
//                                statsInfo[OSP_STATS_SCORE],
//                                statsInfo[OSP_STATS_KILLS],
//                                statsInfo[OSP_STATS_DEATHS],
//                                statsInfo[OSP_STATS_SUCIDES],
//                                kd,
//                                effiency));
//  }
//  strcpy(&strings[2][0], " ");
//  strcpy(&strings[3][0], "^5Weapon       ^3Accrcy ^7Hits/Atts ^2Klls ^1Dths ^3PkUp ^1Drop");
//  strcpy(&strings[4][0], "^7-------------------------------------------------");
//  i = 1;
//  row = 5;
//  arg_cnt = 23;

//  do
//  {
//      if (statsInfo[OSP_STATS_WEAPON_MASK] & (1 << i))
//      {
//          w_hits = atoi(CG_Argv(arg_cnt++));
//          w_attacks = atoi(CG_Argv(arg_cnt++));
//          w_kills = atoi(CG_Argv(arg_cnt++));
//          w_deaths = atoi(CG_Argv(arg_cnt++));

//          strcpy(&strings[row][0], va("%-12s: ", weaponNames[i]));
//          if ((w_attacks & cgs.osp.stats_mask) || (w_hits & cgs.osp.stats_mask))
//          {
//              if ((w_attacks & cgs.osp.stats_mask) == 0)
//              {
//                  tmpf = 0;
//              }
//              else
//              {
//                  tmpf = 100.0 * (float)(w_hits & cgs.osp.stats_mask) / (float)(w_attacks & cgs.osp.stats_mask);
//              }
//              strcat(&strings[row][0], va("^3%3.1f ^7%4d/%-4d ", tmpf, w_hits & cgs.osp.stats_mask, w_attacks & cgs.osp.stats_mask));
//              flag = qtrue;
//          }
//          else
//          {
//              strcat(&strings[row][0], "                ");
//              if (w_kills || w_deaths)
//              {
//                  flag = qtrue;
//              }
//          }

//          if (i > 2)
//          {
//              strcat(&strings[row][0], va("^2%4d ^1%4d ^3%4d ^1%4d", w_kills, w_deaths, w_attacks >> cgs.osp.stats_shift, w_hits >> cgs.osp.stats_shift));
//          }
//          else
//          {
//              strcat(&strings[row][0], va("^2%4d ^1%4d", w_kills, w_deaths));
//          }
//          ++row;
//      }
//  }
//  while (++i < 10);

//  if (row == 5)
//  {
//      strcpy(&strings[5][0], "^3No additional weapon info available.");
//      strcpy(&strings[6][0], " ");
//      row += 2;
//  }
//  else if (CG_OSPIsStatsHidden(qtrue, qtrue))
//  {
//      strcpy(&strings[row++][0], " ");
//      strcpy(&strings[row++][0], "^3Damage Given: ^7XX     ^2Armor : ^7XX");
//      strcpy(&strings[row++][0], "^3Damage Recvd: ^7XX     ^2Health: ^7XX");
//      strcpy(&strings[row++][0], "^3Damage Ratio: ^7XXX");
//  }
//  else
//  {
//      char str1[128] = {0};
//      char str2[128] = {0};
//      if (statsInfo[OSP_STATS_MH])
//      {
//          strcpy(str1, va("^2(^7%d ^5MH^2)", statsInfo[OSP_STATS_MH]));
//      }
//      if (statsInfo[OSP_STATS_GA])
//      {
//          strcpy(str2, va("^2(^7%d ^2GA%s", statsInfo[OSP_STATS_GA],
//                          !statsInfo[OSP_STATS_RA] && !statsInfo[OSP_STATS_YA] ? "^2)" : "^7"
//                         ));
//      }
//      if (statsInfo[OSP_STATS_YA])
//      {
//          strcat(str2, va("%s%d ^3YA%s",
//                          statsInfo[OSP_STATS_GA] ? " " : "^2(^7",
//                          statsInfo[20],
//                          statsInfo[OSP_STATS_RA] ? "^7" : "^2)"));
//      }
//      if (statsInfo[OSP_STATS_RA])
//      {
//          strcat(str2, va("%s%d ^1RA^2)",
//                          !statsInfo[OSP_STATS_GA] && !statsInfo[OSP_STATS_YA] ? "^2(" : " ",
//                          statsInfo[OSP_STATS_RA]));
//      }

//      strcpy(&strings[row++][0], " ");
//      if (flag)
//      {
//          strcpy(&strings[row++][0], va("^3Damage Given: ^7%-8d ^2Armor : ^7%d %s", statsInfo[OSP_STATS_DMG_GIVEN], statsInfo[OSP_STATS_WINS] >> cgs.osp.stats_shift, str2));
//          strcpy(&strings[row++][0], va("^3Damage Recvd: ^7%-8d ^2Health: ^7%d %s", statsInfo[OSP_STATS_DMG_RCVD], statsInfo[OSP_STATS_LOSSES] >> cgs.osp.stats_shift, str1));

//          if (cgs.gametype == GT_TEAM)
//          {
//              strcpy(&strings[row++][0], va("^1Team Damage : ^7%d", statsInfo[OSP_STATS_DMG_TEAM]));
//          }
//          strcpy(&strings[row++][0], va("^3Damage Ratio: ^7%.2f", damageRatio));
//      }
//      else
//      {

//          strcpy(&strings[row++][0], va("^2Armor Taken : ^7%d %s", statsInfo[OSP_STATS_WINS] >> cgs.osp.stats_shift, str2));
//          strcpy(&strings[row++][0], va("^2Health Taken: ^7%d %s", statsInfo[OSP_STATS_LOSSES] >> cgs.osp.stats_shift, str1));
//      }

//  }

//  {
//      float scrollTime;
//      float wpos;

//      scrollTime = cg_statScrollTime.value;

//      if (cg.intermissionStarted)
//      {
//          wpos = 435.0f;
//      }
//      else
//      {
//          wpos = 420.0f;
//      }

//      wstatsWndId = 1 + CG_OSPDrawLeftSlidingWindow(
//                        scrollTime,
//                        scrollTime,
//                        cg.demoPlayback ? 4.0f : 9999.0f,
//                        0,
//                        row,
//                        128,
//                        10,
//                        10,
//                        &strings[0][0],
//                        wpos,
//                        NULL,
//                        statsInfo[OSP_STATS_TEAM] == TEAM_RED ? scoreboard_rtColor : (statsInfo[OSP_STATS_TEAM] == TEAM_BLUE ? scoreboard_btColor : NULL));
//  }
// }

void CG_OSPDrawClientScore(int x, int y, const score_t* score, const float* color, float fade)
{
	char string[1024];
	clientInfo_t* ci;
	vec3_t  headAngles;

	if (score->client < 0 || score->client >= cgs.maxclients)
	{
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	if (score->client == cg.snap->ps.clientNum)
	{
		vec4_t ourColor;
		localClient = qtrue;
		ourColor[0] = 0.7f;
		ourColor[1] = 0.7f;
		ourColor[2] = 0.7f;
		ourColor[3] = 0.2f * fade;
		CG_FillRect(x + 8, y, 304.0f, 17.0f, ourColor);
	}
	if (ci->powerups & (1 << PW_REDFLAG))
	{
		CG_DrawFlagModel(x + 4, y, 16.0f, 16.0f, TEAM_RED, qfalse);
	}
	else if (ci->powerups & (1 << PW_BLUEFLAG))
	{
		CG_DrawFlagModel(x + 4, y, 16.0f, 16.0f, TEAM_BLUE, qfalse);
	}
	trap_R_SetColor(NULL);

	if (cg_scoreboardShowId.integer)
	{
		Com_sprintf(string, 1024, "%i", score->client);
		CG_OSPDrawString(x + 20, y + 4, string, colorWhite, 6, 10, SCREEN_WIDTH, DS_HRIGHT | DS_SHADOW, NULL);
	}
	VectorClear(headAngles);
	headAngles[1] = 180.0f;
	CG_DrawHead(x + 22, y, 16.0f, 16.0f, score->client, headAngles);

	if (!cg.warmup && cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze && cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client))
	{
		CG_OSPDrawPic(x + 22, y, 16.0f, 16.0f, cgs.media.frozenFoeTagShader);
	}

	if (score->ping == -1)
	{
		Com_sprintf(string, 1024, " ^2connecting^7      %s", ci->name);
		CG_OSPDrawString(x + 34, y + 2, string, colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}
	else
	{
		int pingColor = 1;
		if (score->ping < 40)
		{
			pingColor = 7;
		}
		else if (score->ping < 70)
		{
			pingColor = 2;
		}
		else if (score->ping < 100)
		{
			pingColor = 3;
		}
		else if (score->ping < 200)
		{
			pingColor = 8;
		}
		else if (score->ping < 400)
		{
			pingColor = 6;
		}
		Com_sprintf(string, 1024, "%3i", score->score);
		CG_OSPDrawString(x + 46, y, string, color, 12, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		if (cgs.gametype == GT_TEAM)
		{
			if (cgs.osp.gameTypeFreeze)
			{
				Com_sprintf(string, 1024, "%3i", score->scoreFlags);
			}
			else
			{
				Com_sprintf(string, 1024, "^%i%3i", score->scoreFlags < 0 ? 3 : 7, score->scoreFlags);
			}
			CG_OSPDrawString(x + 90, y + 4, string, color, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
		else
		{
			CG_OSPDrawString(x + 90, y + 4, " 0", color, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
		Com_sprintf(string, 1024, "^%i%3i", pingColor, score->ping);
		CG_OSPDrawString(x + 118, y, string, color, 12, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		Com_sprintf(string, 1024, "%3i", score->time);
		CG_OSPDrawString(x + 150, y, string, color, 12, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		Com_sprintf(string, 1024, "%s", &ci->name);
		CG_OSPDrawString(x + 202, y + 4, string, color, 8, 12, 102, DS_HLEFT | DS_SHADOW, NULL);

	}
	if (cgs.clientinfo[score->client].st)
	{
		char* tmp;
		tmp = va(" ^B%sCoach", cgs.clientinfo[0].st == TEAM_RED ? "^1" : "^4");
		CG_OSPDrawString(x, y, tmp, color, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}
	if (!cg.warmup && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return;
	}
	if (cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client))
	{
		CG_OSPDrawString(x, y, " ^B^3READY", colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}
	return;
}

static void CG_OSPDrawPowerupFrame(int x, int y, const clientInfo_t* ci)
{
	vec4_t borderSize;
	qboolean drawAny = qfalse;

	team_t myTeam = cgs.clientinfo[cg.clientNum].team;

	float x1 = (float)x + 8;
	float y1 = (float)y;
	float w1 = 304.0f;
	float h1 = 17.0f;

	if (myTeam != TEAM_SPECTATOR && ci->team != myTeam && ci->team != TEAM_SPECTATOR)
	{
		return;
	}

	if (cg_scoreboardDrawPowerUps.integer == 2)
	{
		Vector4Set(borderSize, 2, 0, 0, 0);
	}
	else
	{
		Vector4Copy(defaultBorderSize, borderSize);
	}


	CG_AdjustFrom640(&x1, &y1, &w1, &h1);

	if (ci->powerups & (1 << PW_QUAD))
	{
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorCyan, qtrue);
	}

	if (ci->powerups & (1 << PW_BATTLESUIT))
	{
		x1 += borderSize[0];
		y1 += borderSize[1];
		w1 -= borderSize[0] + borderSize[2];
		h1 -= borderSize[1] + borderSize[3];
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorOrange, qtrue);
	}

	if (ci->powerups & (1 << PW_HASTE))
	{
		x1 += borderSize[0];
		y1 += borderSize[1];
		w1 -= borderSize[0] + borderSize[2];
		h1 -= borderSize[1] + borderSize[3];
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorYellow, qtrue);
	}

	if (ci->powerups & (1 << PW_INVIS))
	{
		x1 += borderSize[0];
		y1 += borderSize[1];
		w1 -= borderSize[0] + borderSize[2];
		h1 -= borderSize[1] + borderSize[3];
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorWhite, qtrue);
	}

	if (ci->powerups & (1 << PW_REGEN))
	{
		x1 += borderSize[0];
		y1 += borderSize[1];
		w1 -= borderSize[0] + borderSize[2];
		h1 -= borderSize[1] + borderSize[3];
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorRed, qtrue);
	}

	if (ci->powerups & (1 << PW_FLIGHT))
	{
		x1 += borderSize[0];
		y1 += borderSize[1];
		w1 -= borderSize[0] + borderSize[2];
		h1 -= borderSize[1] + borderSize[3];
		drawAny = qtrue;
		CG_OSPDrawFrame(x1, y1, w1, h1, borderSize, colorMagenta, qtrue);
	}

	if (!drawAny)
	{
		return;
	}
}


void CG_BEDrawTeamClientScore(int x, int y, const score_t* score, const float* color, float fade)
{
	char string[1024];
	float lWidth = 6, lHeight = 10;
	float mWidth = 8, mHeight = 12;
	float bWidth = 12, bHeight = 16;
	clientInfo_t* ci;
	vec3_t  headAngles;
	int font = cg_scoreboardFont.integer;
	int proportional = 0;

	if (score->client < 0 || score->client >= cgs.maxclients)
	{
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];
	if (score->client == cg.snap->ps.clientNum)
	{
		vec4_t ourColor;
		localClient = qtrue;
		ourColor[0] = 0.7f;
		ourColor[1] = 0.7f;
		ourColor[2] = 0.7f;
		ourColor[3] = 0.2f * fade;
		CG_FillRect(x + 8, y, 304.0f, 17.0f, ourColor);
	}
	if (ci->powerups & (1 << PW_REDFLAG))
	{
		CG_DrawFlagModel(x + 4, y, 16.0f, 16.0f, TEAM_RED, qfalse);
	}
	else if (ci->powerups & (1 << PW_BLUEFLAG))
	{
		CG_DrawFlagModel(x + 4, y, 16.0f, 16.0f, TEAM_BLUE, qfalse);
	}
	trap_R_SetColor(NULL);

	if (cg_scoreboardDrawPowerUps.integer)
	{
		CG_OSPDrawPowerupFrame(x, y, ci);
	}

	if (cg_scoreboardBE.integer & 1)
	{
		proportional = DS_PROPORTIONAL;
	}

	CG_FontSelect(font);
	if (cg_scoreboardShowId.integer)
	{
		Com_sprintf(string, 1024, "%i", score->client);
		CG_OSPDrawStringNew(x + 20, y + 4, string, colorWhite, colorBlack, lWidth, lHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
	}
	VectorClear(headAngles);
	headAngles[1] = 180.0f;
	CG_DrawHead(x + 22, y, 16.0f, 16.0f, score->client, headAngles);

	if (!cg.warmup && cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze && cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client))
	{
		CG_OSPDrawPic(x + 22, y, 16.0f, 16.0f, cgs.media.frozenFoeTagShader);
	}

	if (score->ping == -1)
	{
		Com_sprintf(string, 1024, " ^2connecting^7      %s", ci->name);
		CG_OSPDrawStringNew(x + 34, y + 2, string, colorWhite, colorBlack, mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | proportional | DS_SHADOW, NULL, NULL, NULL);
	}
	else
	{
		int pingColor = 1;
		if (score->ping < 40)
		{
			pingColor = 7;
		}
		else if (score->ping < 70)
		{
			pingColor = 2;
		}
		else if (score->ping < 100)
		{
			pingColor = 3;
		}
		else if (score->ping < 200)
		{
			pingColor = 8;
		}
		else if (score->ping < 400)
		{
			pingColor = 6;
		}
		Com_sprintf(string, 1024, "%3i", score->score);
		CG_OSPDrawStringNew(x + 80, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
		if (cgs.gametype == GT_TEAM)
		{
			if (cgs.osp.gameTypeFreeze)
			{
				Com_sprintf(string, 1024, "%i", score->scoreFlags);
			}
			else
			{
				Com_sprintf(string, 1024, "^%i%3i", score->scoreFlags < 0 ? 3 : 7, score->scoreFlags);
			}
			CG_OSPDrawStringNew(x + 112, y + 4, string, color, colorBlack, mWidth, mHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
		}
		else
		{
			CG_OSPDrawStringNew(x + 112, y + 4, "0", color, colorBlack, mWidth, mHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
		}
		Com_sprintf(string, 1024, "^%i%3i", pingColor, score->ping);
		CG_OSPDrawStringNew(x + 152, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);

		Com_sprintf(string, 1024, "%i", score->time);
		CG_OSPDrawStringNew(x + 184, y, string, color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);

		Com_sprintf(string, 1024, "%s", &ci->name);
		CG_OSPDrawStringNew(x + 202, y + 4, string, color, colorBlack, mWidth, mHeight, 102, DS_HLEFT | proportional | DS_SHADOW, NULL, NULL, NULL);

	}
	if (cgs.clientinfo[score->client].st)
	{
		char* tmp;
		tmp = va(" ^B%sCoach", cgs.clientinfo[0].st == TEAM_RED ? "^1" : "^4");
		CG_OSPDrawStringNew(x, y, tmp, color, colorBlack, mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | proportional | DS_SHADOW, NULL, NULL, NULL);
	}
	if (!cg.warmup && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return;
	}
	if (cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client))
	{
		CG_OSPDrawStringNew(x + 30, y, "^B^3READY", colorWhite, colorBlack, mWidth, mHeight, SCREEN_WIDTH, DS_HCENTER | proportional | DS_SHADOW, NULL, NULL, NULL);
	}
	return;
}

void CG_OSPDrawClientScoreNew(int x, int y, const score_t* score, const float* color, float fade)
{
	if (cg_scoreboardBE.integer)
		CG_BEDrawTeamClientScore(x, y, score, color, fade);
	else
		CG_OSPDrawClientScore(x, y, score, color, fade);
}


int CG_OSPDrawTeamScores(int x, int y, int team, float fade, int maxScores)
{
	int i;
	qboolean isCAGame;
	int scoresPrinted = 0;
	clientInfo_t* ci;
	score_t* score;
	vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
	const vec4_t readyColor = { 0.5f, 0.5f, 0.5f, 0.5f };
	color[3] = fade;

	isCAGame = CG_OSPIsGameTypeCA(cgs.gametype);

	for (i = 0, scoresPrinted = 0; i < cg.realNumClients && scoresPrinted < maxScores; ++i)
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];
		if (ci->team != team && ci->rt != team)
		{
			continue;
		}

		if (team == TEAM_RED)
		{
			sumPingRed += score->ping;
			sumScoresRed += score->score;
			sumThawsRed += score->scoreFlags;
		}
		else if (team == TEAM_BLUE)
		{
			sumPingBlue += score->ping;
			sumScoresBlue += score->score;
			sumThawsBlue += score->scoreFlags;
		}

		if (team == TEAM_SPECTATOR && ci->rt == TEAM_SPECTATOR)
		{
			int tmp = scoresPrinted / 2;
			if (scoresPrinted % 2 == 0)
			{
				CG_OSPDrawClientScoreNew(x, y + 18 * tmp - 18, score, color, fade);
			}
			else
			{
				CG_OSPDrawClientScoreNew(x + 320, y + 18 * tmp - 18, score, color, fade);
			}
			++scoresPrinted;
		}
		else
		{
			qboolean isReady;
			isReady = cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client);

			if (!isCAGame)
			{
				if (ci->team == team)
				{
					if (!isReady)
					{
						CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, color, fade);
					}
					else
					{
						CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, readyColor, fade);
					}
				}
			}
			else if (team != TEAM_SPECTATOR)
			{
				// draw red/blue teams in CA
				if (ci->team != TEAM_SPECTATOR)
				{
					CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, color, fade);
				}
				else
				{
					CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, readyColor, fade);
				}
			}
			else if (ci->rt == TEAM_SPECTATOR) // redundant but left for clarity
			{
				if (isReady)
				{
					CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, readyColor, fade);
				}
				else
				{
					CG_OSPDrawClientScoreNew(x, y + 18 * scoresPrinted++, score, color, fade);
				}
			}
		}
	}

	return scoresPrinted;
}


qboolean CG_OSPDrawScoretable(void)
{
	vec4_t* color;
	vec4_t colorRect;
	int drewRed;
	int drewBlue;
	int drewSpect;
	int y;
	sumScoresBlue = 0;
	sumScoresRed = 0;
	sumPingBlue = 0;
	sumPingRed = 0;
	sumThawsBlue = 0;
	sumThawsRed = 0;

	if (cg_hideScores.integer)
	{
		return qfalse;
	}

	if (cg_paused.integer)
	{
		return qfalse;
	}
	if ((cgs.gametype == GT_SINGLE_PLAYER) && (cg.predictedPlayerState.pm_type == PM_INTERMISSION))
	{
		return qfalse;
	}
	if (cg.warmup && !cg.showScores && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return qfalse;
	}
	if (!cg.showScores && cg.predictedPlayerState.pm_type != PM_DEAD && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		color = (vec4_t*)CG_FadeColor(cg.scoreFadeTime, 0xc8);
	}
	else
	{
		color = &colorWhite;
	}

	if (color == NULL)
	{
		cg.killerName[0] = 0;
		return qfalse;
	}
	y = 40;
	if (cg.demoPlayback)
	{
		CG_DrawBigString(SCREEN_WIDTH / 2.0f, y, "^3Demo Playback", *color[0], DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, 2);
	}
	else if (cg.killerName[0])
	{
		CG_DrawBigString(SCREEN_WIDTH / 2.0f, y, va("Fragged by %s", cg.killerName), *color[0], DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW, 2);
	}


	y = 64;

	CG_OSPAdjustTeamColor(scoreboard_rtColor, colorRect);
	CG_FillRect(8.0f, (float)y, 304.0f, 48.0f, colorRect);

	CG_OSPAdjustTeamColor(scoreboard_btColor, colorRect);
	CG_FillRect(328.0f, (float)y, 304.0f, 48.0f, colorRect);

	CG_OSPDrawField(8, y, cg.teamScores[0]);
	trap_R_SetColor(NULL);
	CG_OSPDrawField(328, y, cg.teamScores[1]);
	trap_R_SetColor(NULL);
	y = 116;

	CG_FontSelect(0);

	{
		char* tmpStr;
		char* tmpArgStr;
		if (cgs.gametype == GT_TEAM)
		{
			if (cgs.osp.gameTypeFreeze)
			{
				tmpArgStr = "THW";
			}
			else
			{
				tmpArgStr = "NET";
			}
		}
		else
		{
			tmpArgStr = "PL ";
		}
		tmpStr = va("^1Score %s Ping Min  Name", tmpArgStr);
		CG_OSPDrawString(40, y, tmpStr, colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}

	{
		char* tmpStr;
		char* tmpArgStr;
		if (cgs.gametype == 0x3)
		{
			if (cgs.osp.gameTypeFreeze != 0)
			{
				tmpArgStr = "THW";
			}
			else
			{
				tmpArgStr = "NET";
			}
		}
		else
		{
			tmpArgStr = "PL ";
		}
		tmpStr = va("^4Score %s Ping Min  Name", tmpArgStr);
		CG_OSPDrawString(360, y, tmpStr, colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}

	y = 140;
	drewRed = CG_OSPDrawTeamScores(0, y, TEAM_RED, *color[0], 12);
	drewBlue = CG_OSPDrawTeamScores(320, y, TEAM_BLUE, *color[0], 12);

	if (drewRed)
	{
		char string[128];
		char* tmpStr;
		if (cgs.gametype >= GT_CTF)
		{
			tmpStr = va("^1Points  Players  AvgPing");
			CG_OSPDrawString(116, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			Com_sprintf(string, 128, "^3%3i^7  %2i  %3i", sumScoresRed, drewRed, sumPingRed / drewRed);
			CG_OSPDrawString(116, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
		else
		{
			if (cgs.gametype == GT_TEAM && !CG_OSPIsGameTypeFreeze()) // Обычный TDM
			{
				tmpStr = va("^1Players  AvgPing");
				CG_OSPDrawString(104, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
				Com_sprintf(string, 128, " %2i  %3i", drewRed, sumPingRed / drewRed);
				CG_OSPDrawString(88, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			}
			else if (CG_OSPIsGameTypeFreeze()) // Freeze Tag
			{
				tmpStr = va("^1Scores   Thaws Players");
				CG_OSPDrawString(80, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
				Com_sprintf(string, 128, " %3i %3i  %2i", sumScoresRed, sumThawsRed, drewRed);
				CG_OSPDrawString(64, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			}
		}
	}

	if (drewBlue)
	{
		char string[128];
		char* tmpStr;
		if (cgs.gametype >= GT_CTF)
		{
			tmpStr = va("^4Points  Players  AvgPing");
			CG_OSPDrawString(436, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			Com_sprintf(string, 128, "^3%3i^7  %2i  %3i", sumScoresBlue, drewBlue, sumPingBlue / drewBlue);
			CG_OSPDrawString(436, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
		else if (cgs.gametype == GT_TEAM && !CG_OSPIsGameTypeFreeze()) // Обычный TDM
		{
			tmpStr = va("^4Players  AvgPing");
			CG_OSPDrawString(424, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			Com_sprintf(string, 128, " %2i  %3i", drewBlue, sumPingBlue / drewBlue);
			CG_OSPDrawString(408, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
		else if (CG_OSPIsGameTypeFreeze()) // Freeze Tag
		{
			tmpStr = va("^4Scores   Thaws Players");
			CG_OSPDrawString(400, 64, tmpStr, colorWhite, 8, 16, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
			Com_sprintf(string, 128, " %3i %3i  %2i", sumScoresBlue, sumThawsBlue, drewBlue);
			CG_OSPDrawString(384, 80, string, colorWhite, 16, 20, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
		}
	}




	{
		int max;

		max = drewRed < drewBlue ? drewBlue : drewRed;
		y += 18 * max + 18;

		drewRed  = CG_OSPDrawTeamScores(0, y, TEAM_6, *color[0], 8);
		drewBlue = CG_OSPDrawTeamScores(320, y, TEAM_7, *color[0], 8);
	}

	if (drewRed != 0)
	{
		CG_OSPDrawString(60, y - 14, "^1Blue Team Spectator", colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}
	if (drewBlue != 0)
	{
		CG_OSPDrawString(380, y - 14, "^4Blue Team Spectator", colorWhite, 8, 12, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW, NULL);
	}

	{
		int max;
		vec4_t bgColor;

		max = drewRed < drewBlue ? drewBlue : drewRed;

		y = y + 18 * max + 36;

		CG_OSPAdjustTeamColor(scoreboard_rtColor, bgColor);
		CG_FillRect(8.0f, 112.0f, 304.0f, (float)y - 148, bgColor);

		CG_OSPAdjustTeamColor(scoreboard_btColor, bgColor);
		CG_FillRect(328.0f, 112.0f, 304.0f, (float)y - 148, bgColor);

		drewSpect = CG_OSPDrawTeamScores(0, y, TEAM_SPECTATOR, *color[0], 24);

		if (drewSpect)
		{
			CG_OSPDrawString(SCREEN_WIDTH / 2.0f, y - 32, "Spectator", colorWhite, 8, 12, SCREEN_WIDTH, DS_HCENTER | DS_SHADOW, NULL);
			bgColor[0] = bgColor[1] = bgColor[2] = 0.5f;
			bgColor[3] = 0.2f;
			CG_FillRect(8.0f, (float)y - 0x22, 624.0f, (float)(9 * drewSpect + 9 + 20), bgColor);
		}
	}

	if (cg_drawAccuracy.integer)
		CG_DrawWeaponStatsWrapper();
	return qtrue;
}

void SetScoreboardColors(vec4_t* rtColorTitle, vec4_t* rtColorBody, vec4_t* btColorTitle, vec4_t* btColorBody)
{
	if (!isCustomScoreboardColorIsSet_rt)
	{
		Vector4Copy(scoreboard_rtColor, *rtColorTitle);
		Vector4Copy(scoreboard_rtColor, *rtColorBody);
	}
	else
	{
		Vector4Copy(scoreboard_rtColorTitle, *rtColorTitle);
		Vector4Copy(scoreboard_rtColorBody, *rtColorBody);
	}

	if (!isCustomScoreboardColorIsSet_bt)
	{
		Vector4Copy(scoreboard_btColor, *btColorTitle);
		Vector4Copy(scoreboard_btColor, *btColorBody);
	}
	else
	{
		Vector4Copy(scoreboard_btColorTitle, *btColorTitle);
		Vector4Copy(scoreboard_btColorBody, *btColorBody);
	}
}

int proportional = 0;
float bWidth = 16, bHeight = 16;
float bWidth2 = 16, bHeight2 = 20;
float mWidth = 8, mHeight = 12;
float mWidth2 = 8, mHeight2 = 16;
float leftX = 40, rightX = 360;
float pos1X = 40;
float pos2X = 72;
float pos3X = 112;
float pos4X = 144;
float pos5X = 162;
float titlePos1X = 48;
float titlePos2X = 112;
float titlePos3X = 176;
float row1Y = 64;
float row2Y = 80;

static void CG_OSPDrawTeamSummary(
    float baseX,
    int drewPlayers,
    int sumScores,
    int sumThaws,
    int sumPing,
    const vec4_t titleColor)
{
    const char* labels[3];
    char values[3][128];
    float posX[3];
    int i, count = 0;
	vec4_t headerColor;
	vec4_t curColor;
    /* proportional по битам */
    int proportional = (cg_scoreboardBE.integer & 2) ? 0 :
                       (cg_scoreboardBE.integer & 1) ? DS_PROPORTIONAL : 0;


	if (cg_scoreboardBE.integer & 4) {
		Vector4Copy(colorWhite, headerColor);
	} else {
		Vector4Copy(titleColor, headerColor);
	}


    /* Заполняем колонки в зависимости от режима */
    if (cgs.gametype >= GT_CTF) {
        labels[count] = "Points";
        Com_sprintf(values[count], sizeof(values[count]), "^3%i^7", sumScores);
        posX[count] = titlePos1X;
        count++;

        labels[count] = "Players";
        Com_sprintf(values[count], sizeof(values[count]), "%i", drewPlayers);
        posX[count] = titlePos2X;
        count++;

        labels[count] = "AvgPing";
        Com_sprintf(values[count], sizeof(values[count]), "%i", sumPing / drewPlayers);
        posX[count] = titlePos3X;
        count++;
    }
    else if (cgs.gametype == GT_TEAM && !CG_OSPIsGameTypeFreeze()) {
        labels[count] = "Players";
        Com_sprintf(values[count], sizeof(values[count]), "%i", drewPlayers);
        posX[count] = titlePos2X;
        count++;

        labels[count] = "AvgPing";
        Com_sprintf(values[count], sizeof(values[count]), "%i", sumPing / drewPlayers);
        posX[count] = titlePos3X;
        count++;
    }
    else if (CG_OSPIsGameTypeFreeze()) {
        labels[count] = "Scores";
        Com_sprintf(values[count], sizeof(values[count]), "%i", sumScores);
        posX[count] = titlePos1X;
        count++;

        labels[count] = "Thaws";
        Com_sprintf(values[count], sizeof(values[count]), "%i", sumThaws);
        posX[count] = titlePos2X;
        count++;

        labels[count] = "Players";
        Com_sprintf(values[count], sizeof(values[count]), "%i", drewPlayers);
        posX[count] = titlePos3X;
        count++;
    }

    for (i = 0; i < count; i++) {

    if (cg_scoreboardBE.integer & 4) {
        Vector4Copy(colorWhite, curColor); // все белые
    } else {
        if (i == 0)
            Vector4Copy(titleColor, curColor); // первый - цвет команды
        else
            Vector4Copy(colorWhite, curColor); // остальные - белые
    }

    CG_OSPDrawStringNew(baseX + posX[i], row1Y, labels[i], curColor, colorBlack,
                        mWidth2, mHeight2, SCREEN_WIDTH,
                        DS_HRIGHT | DS_SHADOW | proportional, NULL, NULL, NULL);

    CG_OSPDrawStringNew(baseX + posX[i], row2Y, values[i], colorWhite, colorBlack,
                        bWidth2, bHeight2, SCREEN_WIDTH,
                        DS_HRIGHT | DS_SHADOW | proportional, NULL, NULL, NULL);
	}
}


void CG_OSPDrawScoreHeader(float baseX, float y, vec4_t colorBody, vec4_t colorBlack, int mWidth, int mHeight, int screenWidth, int proportional)
{
	const char* label1 = "Score";
	const char* label2 = (cgs.gametype == GT_TEAM)
	                     ? (cgs.osp.gameTypeFreeze ? "THW" : "NET")
	                     : "PL";
	const char* label3 = "Ping";
	const char* label4 = "Min";
	const char* label5 = "Name";

	CG_OSPDrawStringNew(baseX + pos1X, y, label1, (cg_scoreboardBE.integer & 4) ? colorWhite : colorBody, colorBlack, mWidth, mHeight, screenWidth,
	                    DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
	CG_OSPDrawStringNew(baseX + pos2X, y, label2, colorWhite, colorBlack, mWidth, mHeight, screenWidth,
	                    DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
	CG_OSPDrawStringNew(baseX + pos3X, y, label3, colorWhite, colorBlack, mWidth, mHeight, screenWidth,
	                    DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
	CG_OSPDrawStringNew(baseX + pos4X, y, label4, colorWhite, colorBlack, mWidth, mHeight, screenWidth,
	                    DS_HRIGHT | proportional | DS_SHADOW, NULL, NULL, NULL);
	CG_OSPDrawStringNew(baseX + pos5X, y, label5, colorWhite, colorBlack, mWidth, mHeight, screenWidth,
	                    DS_HLEFT | proportional | DS_SHADOW, NULL, NULL, NULL);
}


qboolean CG_BEDrawTeamScoretable(void)
{
	vec4_t* color;
	vec4_t colorRect;
	vec4_t rtColorTitle, rtColorBody;
	vec4_t btColorTitle, btColorBody;
	int drewRed;
	int drewBlue;
	int drewSpect;
	int y;
	int font = cg_scoreboardFont.integer;

	sumScoresBlue = 0;
	sumScoresRed = 0;
	sumPingBlue = 0;
	sumPingRed = 0;
	sumThawsBlue = 0;
	sumThawsRed = 0;

	if (cg_hideScores.integer)
	{
		return qfalse;
	}

	if (cg_paused.integer)
	{
		return qfalse;
	}

	if ((cgs.gametype == GT_SINGLE_PLAYER) && (cg.predictedPlayerState.pm_type == PM_INTERMISSION))
	{
		return qfalse;
	}

	if (cg.warmup && !cg.showScores && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return qfalse;
	}

	if (!cg.showScores && cg.predictedPlayerState.pm_type != PM_DEAD && cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		color = (vec4_t*)CG_FadeColor(cg.scoreFadeTime, 0xc8);
	}
	else
	{
		color = &colorWhite;
	}

	if (color == NULL)
	{
		cg.killerName[0] = 0;
		return qfalse;
	}

	y = 40;

	if (cg_scoreboardBE.integer & 1)
	{
		proportional = DS_PROPORTIONAL;
	}
	else
	{
		proportional = 0;
	}

	CG_FontSelect(font);

	if (cg.demoPlayback)
	{
		CG_OSPDrawStringNew(SCREEN_WIDTH / 2.0f, y, "^3Demo Playback", *color, colorBlack, bWidth, bHeight, SCREEN_WIDTH, DS_HCENTER | proportional | DS_SHADOW, NULL, NULL, NULL);
	}
	else if (cg.killerName[0])
	{
		CG_OSPDrawStringNew(SCREEN_WIDTH / 2.0f, y, va("Fragged by %s", cg.killerName), *color, colorBlack, mWidth, bHeight, SCREEN_WIDTH, DS_HCENTER | proportional | DS_SHADOW, NULL, NULL, NULL);
	}

	SetScoreboardColors(&rtColorTitle, &rtColorBody, &btColorTitle, &btColorBody);

	// Header background
	y = 64;

	CG_OSPAdjustTeamColor(rtColorTitle, colorRect);
	colorRect[3] *= 1.5;
	CG_FillRect(8.0f, (float)y, 304.0f, 48.0f, colorRect);

	CG_OSPAdjustTeamColor(btColorTitle, colorRect);
	colorRect[3] *= 1.5;
	CG_FillRect(328.0f, (float)y, 304.0f, 48.0f, colorRect);

	// main team scores
	if (cg_scoreboardBE.integer & 2)
	{
		CG_OSPDrawField(8, y, cg.teamScores[0]);
		trap_R_SetColor(NULL);
		CG_OSPDrawField(328, y, cg.teamScores[1]);
		trap_R_SetColor(NULL);
	}
	else
	{
		CG_OSPDrawStringNew(leftX - 32, 87, va("%d", cg.teamScores[0]), colorWhite, colorBlack, 42, 60, SCREEN_WIDTH, DS_HLEFT | proportional | DS_SHADOW | DS_VCENTER, NULL, NULL, NULL);
		CG_OSPDrawStringNew(rightX - 32, 87, va("%d", cg.teamScores[1]), colorWhite, colorBlack, 42, 60, SCREEN_WIDTH, DS_HLEFT | proportional | DS_SHADOW | DS_VCENTER, NULL, NULL, NULL);
	}

	y = 116;
	// Header text
	CG_OSPDrawScoreHeader(leftX, y, rtColorBody, colorBlack, mWidth, mHeight, SCREEN_WIDTH, proportional);
	CG_OSPDrawScoreHeader(rightX, y, btColorBody, colorBlack, mWidth, mHeight, SCREEN_WIDTH, proportional);

	y = 140;
	// Team score lines
	drewRed = CG_OSPDrawTeamScores(0, y, TEAM_RED, *color[0], 32);
	drewBlue = CG_OSPDrawTeamScores(320, y, TEAM_BLUE, *color[0], 32);

	if (drewRed)
	{
		float baseX = (cgs.gametype >= GT_CTF) ? (leftX + 76) :
		              (cgs.gametype == GT_TEAM && !CG_OSPIsGameTypeFreeze()) ? (leftX + 64) :
		              (CG_OSPIsGameTypeFreeze()) ? (leftX + 40) : leftX;
		CG_OSPDrawTeamSummary(baseX, drewRed, sumScoresRed, sumThawsRed, sumPingRed, rtColorTitle);
	}

	if (drewBlue)
	{
		float baseX = (cgs.gametype >= GT_CTF) ? (rightX + 76) :
		              (cgs.gametype == GT_TEAM && !CG_OSPIsGameTypeFreeze()) ? (rightX + 64) :
		              (CG_OSPIsGameTypeFreeze()) ? (rightX + 40) : rightX;
		CG_OSPDrawTeamSummary(baseX, drewBlue, sumScoresBlue, sumThawsBlue, sumPingBlue, btColorTitle);
	}

	{
		int max = (drewRed < drewBlue) ? drewBlue : drewRed;
		y += 18 * max + 18;

		drewRed = CG_OSPDrawTeamScores(0, y, TEAM_6, *color[0], 32);
		drewBlue = CG_OSPDrawTeamScores(320, y, TEAM_7, *color[0], 32);
	}

	if (drewRed || drewBlue)
	{
		const char* label1 = NULL;
		const char* label2 = "Team";
		const char* label3 = "Spectator";

		if (drewRed)
		{
			label1 = "Red";
			CG_OSPDrawStringNew(leftX + 20, y - 14, label1, rtColorTitle, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
			CG_OSPDrawStringNew(leftX + 60, y - 14, label2, colorWhite, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
			CG_OSPDrawStringNew(leftX + 100, y - 14, label3, colorWhite, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
		}

		if (drewBlue)
		{
			label1 = "Blue";
			CG_OSPDrawStringNew(rightX + 20, y - 14, label1, btColorTitle, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
			CG_OSPDrawStringNew(rightX + 60, y - 14, label2, colorWhite, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
			CG_OSPDrawStringNew(rightX + 100, y - 14, label3, colorWhite, colorBlack,
			                    mWidth, mHeight, SCREEN_WIDTH, DS_HLEFT | DS_SHADOW | proportional, NULL, NULL, NULL);
		}
	}

	// BODY
	{
		int max = (drewRed > drewBlue) ? drewRed : drewBlue;
		vec4_t bgColor;

		y += 18 * max + 36;

		CG_OSPAdjustTeamColor(rtColorBody, bgColor);
		CG_FillRect(8.0f, 112.0f, 304.0f, (float)(y - 148), bgColor);

		CG_OSPAdjustTeamColor(btColorBody, bgColor);
		CG_FillRect(328.0f, 112.0f, 304.0f, (float)(y - 148), bgColor);

		drewSpect = CG_OSPDrawTeamScores(0, y, TEAM_SPECTATOR, *color[0], 24);

		if (drewSpect)
		{
			CG_OSPDrawString(SCREEN_WIDTH / 2.0f, y - 32, "Spectator", colorWhite,
			                 8, 12, SCREEN_WIDTH, DS_HCENTER | DS_SHADOW | proportional, NULL);

			if (!isCustomScoreboardColorIsSet_spec)
			{
				bgColor[0] = bgColor[1] = bgColor[2] = 0.5f;
			}
			else
			{
				Vector4Copy(scoreboard_specColor, bgColor);
			}
			bgColor[3] = 0.15f;

			CG_FillRect(8.0f, (float)(y - 34), 624.0f, (float)(9 * drewSpect + 29), bgColor);
		}
	}

	if (cg_drawAccuracy.integer)
	{
		CG_DrawWeaponStatsWrapper();
	}

	if (!cg.showScores && cg.scoresRequestTime + 2000 < cg.time) // in some situations the score appears without pressing +scores
	{
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");
		cg.realNumClients = CG_CountRealClients();
	}
	if (cg_drawAccuracy.integer && !cg.showAccuracy && cg.statsRequestTime + 2500 < cg.time)
	{
		cg.statsRequestTime = cg.time;
		trap_SendClientCommand("getstatsinfo");
	}

	return qtrue;
}

