/*
===========================================================================
cg_be_stats.c

BE (Black Edition) statistics display system

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com

This file contains original work created by diwoc.
It is included in the OSP2-BE modification for Quake III Arena and is
distributed under the terms of the project's primary license.
===========================================================================
*/

#include "cg_local.h"

#define MAX_TOKEN_LEN MAX_QPATH

typedef struct beStatsSettings_t
{
	int statsStyle;
	float x;
	float y;
	float width;
	float height;

	float textWidth;
	float textHeight;

	float iconSizeW;
	float iconSizeH;
	float baseColWidth;

	float rowHeight;
	float rowSpacing;
	float colSpacing;

	float colWidth;
	float widthCutoff;

	float padding;

	vec4_t defaultColor;
	vec4_t teamColor;
	vec4_t colorR;
	vec4_t colorG;
	vec4_t colorB;
	vec4_t colorY;
	vec4_t colorC;
	vec4_t colorM;

	vec4_t bgColor;
	vec4_t borderColor;
} beStatsSettings_t;

beStatsSettings_t beStatsSettings;
beStatsSettings_t* set = &beStatsSettings;

static qboolean beStatsInitialized = qfalse;
static int      beStatsLastObservedClient = -1;

typedef struct
{
	char text[64];
	int textFlags;
	float offset;
	vec4_t color;
	float width;
} BEStatToken_t;

BEStatToken_t tokens[32][12];

qboolean CG_BEStatsInitSettings(void)
{
	globalBeStatsSettings_t* inc = &cg.beSettings;

	if (!inc) return qfalse;

	set->x = inc->x;
	set->y = inc->y;

	set->textWidth = inc->textSize[0];
	set->textHeight = inc->textSize[1];

	set->iconSizeW = set->textWidth * 1.2f;
	set->iconSizeH = set->textHeight * 1.2f;

	set->padding = (set->textHeight + set->textWidth) / 8;
	set->rowHeight  = set->textHeight * 1;
	set->rowSpacing = set->textHeight;
	set->colSpacing = set->textWidth * cg_bestats_spacingAdjust.value;
	set->baseColWidth = set->textWidth * 5;

	set->widthCutoff = (set->textWidth * cg_bestats_widthCutoff.value);
	set->statsStyle = cg_bestats_style.integer;

	return qtrue;
}

qboolean CG_BEStatsInitColors(const vec4_t color)
{
	int row, col;

	if (!set) return qfalse;

	for (row = 0; row < 32; row++)
		for (col = 0; col < 12; col++)
			Vector4Copy(color, tokens[row][col].color);

	// Default color sets
	Vector4Copy(color, set->defaultColor);
	Vector4Set(set->colorR, 1, 0, 0, 1);
	Vector4Set(set->colorG, 0, 1, 0, 1);
	Vector4Set(set->colorB, 0, 0, 1, 1);
	Vector4Set(set->colorY, 1, 1, 0, 1);
	Vector4Set(set->colorC, 0, 1, 1, 1);
	Vector4Set(set->colorM, 1, 0, 1, 1);

	// Team color logic
	if (cgs.gametype >= GT_TEAM)
	{
		team_t team = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		              ? cgs.clientinfo[cg.clientNum].team
		              : cg.snap->ps.persistant[PERS_TEAM];

		if (team == TEAM_RED)
		{
			Vector4Copy(cgs.beRedTeamColor, set->teamColor);
		}
		else if (team == TEAM_BLUE)
		{
			Vector4Copy(cgs.beBlueTeamColor, set->teamColor);
		}
		else
		{
			Vector4Copy(colorDkGrey, set->teamColor);
		}
	}
	else
	{
		Vector4Copy(colorDkGrey, set->teamColor);
	}

	// Background color
	if (!cg.beSettings.bgColorIsSet)
	{
		CG_OSPAdjustTeamColorBEStats(set->teamColor, set->bgColor);
		set->bgColor[3] = cg_bestats_bgOpaque.value;
	}
	else
	{
		Vector4Set(set->bgColor, cg.beSettings.bgColor[0], cg.beSettings.bgColor[1], cg.beSettings.bgColor[2], cg_bestats_bgOpaque.value);
	}

	Vector4Copy(colorBlack, set->borderColor);

	return qtrue;
}

qboolean CG_BEStatsInitTokenDefaults(void)
{
	int i, j;
	for (i = 0; i < 32; ++i)
	{
		for (j = 0; j < 12; ++j)
		{
			tokens[i][j].offset = 0;
		}
	}
	return qtrue;
}

void CG_BEStatsInit(void)
{
	if (!beStatsInitialized)
	{
		if (!CG_BEStatsInitSettings())
		{
			CG_Printf("BEStats: Failed to init settings\n");
			return;
		}
		if (!CG_BEStatsInitColors(colorWhite))
		{
			CG_Printf("BEStats: Failed to init colors\n");
			return;
		}
		if (!CG_BEStatsInitTokenDefaults())
		{
			CG_Printf("BEStats: Failed to init token defaults\n");
			return;
		}

		beStatsInitialized = qtrue;
	}
}

void CG_BEStatsResetInit(void)
{
	beStatsInitialized = qfalse;
	beStatsLastObservedClient = -1;
	cg.statsRequestLastSendTime = 0;
}

void CG_BEStatsSetTokenLabel(BEStatToken_t tokens[32][12], int row, int col, const char* label, const vec4_t color)
{
	Q_strncpyz(tokens[row][col].text, label, sizeof(tokens[row][col].text));
	if (color)
	{
		Vector4Copy(color, tokens[row][col].color);
	}
}

void CG_BEStatsSetTokenValueInt(BEStatToken_t tokens[32][12], int row, int col, const char* fmt, int value, const vec4_t color)
{
	char buf[MAX_TOKEN_LEN];
	Com_sprintf(buf, sizeof(buf), fmt, value);
	Q_strncpyz(tokens[row][col].text, buf, sizeof(tokens[row][col].text));
	if (color)
	{
		Vector4Copy(color, tokens[row][col].color);
	}
}

void CG_BEStatsSetTokenValueFloat(BEStatToken_t tokens[32][12], int row, int col, const char* fmt, float value, const vec4_t color)
{
	char buf[MAX_TOKEN_LEN];
	Com_sprintf(buf, sizeof(buf), fmt, value);
	Q_strncpyz(tokens[row][col].text, buf, sizeof(tokens[row][col].text));
	if (color)
	{
		Vector4Copy(color, tokens[row][col].color);
	}
}



void CG_BEStatsSetTokenWidth(BEStatToken_t tokens[32][12], int row, int col, float multiply)
{
	tokens[row][col].width = beStatsSettings.colSpacing * multiply;
}

void CG_BEStatsAddTokenTextFlags(BEStatToken_t tokens[32][12], int row, int col, int textFlags)
{
	tokens[row][col].textFlags = textFlags;
}


void CG_BEStatsSetTokenOffset(BEStatToken_t tokens[32][12], int row, int col, float multiply)
{
	tokens[row][col].offset = beStatsSettings.colSpacing * multiply;
}

qhandle_t CG_BEStatsResolveMediaHandle(const char* mediaPath)
{
	if (!mediaPath) return 0;

	// cgs.media.xxx
	if (Q_strncmp(mediaPath, "cgs.media.", 10) == 0)
	{
		const char* fieldName = mediaPath + 10;

		if (strcmp(fieldName, "whiteShader") == 0) return cgs.media.whiteShader;

		return 0;
	}

	// cg_weapons[N].weaponIcon
	if (Q_strncmp(mediaPath, "cg_weapons[", 11) == 0)
	{
		const char* p = mediaPath + 11;
		int index = 0;

		while (*p >= '0' && *p <= '9')
		{
			index = index * 10 + (*p - '0');
			p++;
		}

		if (*p != ']' || Q_strncmp(p + 1, ".weaponIcon", 11) != 0)
		{
			return 0;
		}

		if (index < 0 || index >= WP_NUM_WEAPONS)
		{
			return 0;
		}

		return cg_weapons[index].weaponIcon;
	}

	return 0;
}

static void CG_BEStatsBuildGeneral_Tournament(BEStatToken_t tokens[32][12], int* outCols)
{
	int col = 0;
	const newStatsInfo_t* s = &cgs.newStats;

	if (outCols == NULL)
	{
		outCols = &col; // dummy address to avoid segfault
	}

	if (cg_bestats_style.integer == 2)
	{
		// 1st row
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		col = 0;
		// 2nd row
		CG_BEStatsSetTokenLabel(tokens, 2, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 3, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "Sui", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->suicides, beStatsSettings.defaultColor);

		*outCols = col;

	}
	else
	{
		// default style
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Sui", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->suicides, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Effncy", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "WINS", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->wins, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "LOSSES", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->losses, beStatsSettings.defaultColor);

		*outCols = col;
	}
}



static void CG_BEStatsBuildGeneral_Team(BEStatToken_t tokens[32][12], int* outCols)
{
	int col = 0;
	const newStatsInfo_t* s = &cgs.newStats;
	int style = cg_bestats_style.integer;  // global cvar/setting
	qboolean isFreeze = cgs.osp.gameTypeFreeze;

	if (outCols == NULL)
	{
		outCols = &col; // dummy address to avoid segfault
	}

	if (style == 2)
	{
		// Titles
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		col++;
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		col++;
		if (isFreeze)
		{
			CG_BEStatsSetTokenLabel(tokens, 0, col, "Thws", beStatsSettings.colorC);
		}
		col++;
		// Values
		col = 0;
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);
		if (isFreeze)
		{
			CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->losses, beStatsSettings.defaultColor);
		}
		// Titles 2
		col = 0;
		CG_BEStatsSetTokenLabel(tokens, 2, col++, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenLabel(tokens, 2, col++, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenLabel(tokens, 2, col++, "Sui", beStatsSettings.colorR);
		// Values 2
		col = 0;
		CG_BEStatsSetTokenValueFloat(tokens, 3, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->deaths, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->suicides, beStatsSettings.defaultColor);

		*outCols = col; // 3

	}
	else
	{
		// Default style
		col = 0;
		// Title + Value
		// Score
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);
		// wtf?
		if (isFreeze)
		{
			CG_BEStatsSetTokenLabel(tokens, 0, col, "WINS", beStatsSettings.colorY);
			CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->wins, beStatsSettings.defaultColor);
		}
		else
		{
			CG_BEStatsSetTokenLabel(tokens, 0, col, "NET", beStatsSettings.colorY);
			CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score - s->deaths, beStatsSettings.defaultColor);
		}
		// Kills
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);
		// Thaws
		if (isFreeze)
		{
			CG_BEStatsSetTokenLabel(tokens, 0, col, "Thaws", beStatsSettings.colorC);
			CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->losses, beStatsSettings.defaultColor);
		}
		// Death
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->deaths, beStatsSettings.defaultColor);
		// Suicides
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Sui", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->suicides, beStatsSettings.defaultColor);
		// Team Kills
		CG_BEStatsSetTokenLabel(tokens, 0, col, "TmKlls", beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 1, col, "%d", s->teamKills, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenOffset(tokens, 0, col, -1);
		CG_BEStatsSetTokenOffset(tokens, 1, col, -1);
		col++;
		// KD
		CG_BEStatsSetTokenLabel(tokens, 0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);
		// Effiencity
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Effncy", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

		*outCols = col;
	}
}



static void CG_BEStatsBuildGeneral_CTF(BEStatToken_t tokens[32][12], int* outCols)
{
	int col = 0;
	const newStatsInfo_t* s = &cgs.newStats;
	char timeStr[MAX_TOKEN_LEN];

	if (outCols == NULL)
	{
		outCols = &col; // dummy address to avoid segfault
	}

	if (cg_bestats_style.integer == 2)
	{
		int mins = s->flagTime / 60;
		float secs = (float)s->flagTime - 60.0f * (float)mins;
		// 1st row
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Caps", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->caps, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Rtrn", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->returns, beStatsSettings.defaultColor);

		col = 0;
		// 2nd row
		CG_BEStatsSetTokenLabel(tokens, 2, col, "Asst", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->assists, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "Dfns", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->defences, beStatsSettings.defaultColor);


		Com_sprintf(timeStr, sizeof(timeStr), "%d:%02.1f", mins, secs);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "Ftime", beStatsSettings.colorG);
		Q_strncpyz(tokens[3][col].text, timeStr, MAX_TOKEN_LEN);
		Vector4Copy(beStatsSettings.defaultColor, tokens[3][col].color);
		col++;

		col = 0;
		// 3rd row
		CG_BEStatsSetTokenLabel(tokens, 4, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 5, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 4, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 5, col++, "%d", s->kills, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 4, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 5, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		*outCols = col;
	}
	else
	{
		// ===== Default style =====
		int mins = s->flagTime / 60;
		float secs = (float)s->flagTime - 60.0f * (float)mins;

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Caps", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->caps, beStatsSettings.defaultColor);

		Com_sprintf(timeStr, sizeof(timeStr), "%d:%02.1f", mins, secs);
		Q_strncpyz(tokens[1][col].text, timeStr, MAX_TOKEN_LEN);
		Vector4Copy(beStatsSettings.defaultColor, tokens[1][col].color);
		CG_BEStatsSetTokenLabel(tokens, 0, col++, "Ftime", beStatsSettings.colorG);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Asst", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->assists, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dfns", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->defences, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Rtrn", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->returns, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		*outCols = col;
	}
}



static void CG_BEStatsBuildGeneral_CA(BEStatToken_t tokens[32][12], int* outCols)
{
	int col = 0;
	const newStatsInfo_t* s = &cgs.newStats;

	if (outCols == NULL)
	{
		outCols = &col; // dummy address to avoid segfault
	}

	if (cg_bestats_style.integer == 2)
	{
		// 1st row
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		col = 0;
		// 2nd row
		CG_BEStatsSetTokenLabel(tokens, 2, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 3, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 2, col, "DmgScr", beStatsSettings.colorC);
		CG_BEStatsSetTokenValueInt(tokens, 3, col++, "%d", (int)(s->dmgGiven / 100), beStatsSettings.defaultColor);

		*outCols = col;
	}
	else
	{
		// default style
		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Effncy", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "DmgScr", beStatsSettings.colorC);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", (int)(s->dmgGiven / 100), beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "WINS", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->wins, beStatsSettings.defaultColor);

		*outCols = col;
	}
}

static void CG_BEStatsBuildGeneral_Default(BEStatToken_t tokens[32][12], int* outCols)
{
	int dummy = 0;
	int style = cg_bestats_style.integer;
	const newStatsInfo_t* s = &cgs.newStats;

	if (outCols == NULL)
	{
		outCols = &dummy; // dummy address to avoid segfault
	}

	if (style == 2)
	{
		// style == 2 — 5 rows
		// row 1: labels Score Klls K/D
		CG_BEStatsSetTokenLabel(tokens, 0, 0, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenLabel(tokens, 0, 1, "Klls", beStatsSettings.colorG);
		// row 2: values for Score Klls K/D
		CG_BEStatsSetTokenValueInt(tokens, 1, 0, "%d", s->score, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 1, 1, "%d", s->kills, beStatsSettings.defaultColor);
		// row 3: labels Dths Sui
		CG_BEStatsSetTokenLabel(tokens, 2, 0, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenLabel(tokens, 2, 1, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenLabel(tokens, 2, 2, "Sui", beStatsSettings.colorR);
		// row 4: values for Dths Sui
		CG_BEStatsSetTokenValueFloat(tokens, 3, 0, "%.1f", s->kdratio, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 3, 1, "%d", s->deaths, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueInt(tokens, 3, 2, "%d", s->suicides, beStatsSettings.defaultColor);
		*outCols = 3; // max columns (for first and second rows)
	}
	else
	{
		int col = 0;

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->score, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->kills, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Sui", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, 1, col++, "%d", s->suicides, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, 0, col, "Effncy", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, 1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

		*outCols = col;
	}
}



void CG_BEStatsBuildGeneral(BEStatToken_t tokens[32][12], int startRow, int* outRows, int* outCols)
{
	int i, row;
	int numRows = (set->statsStyle == 2) ? 4 : 3;

	for (i = 0; i < 12; i++)
	{
		tokens[0][i].text[0] = '\0';
		tokens[1][i].text[0] = '\0';
	}


	for (row = 2; row < numRows; row++)
	{
		for (i = 0; i < 12; i++)
		{
			tokens[row][i].text[0] = '\0';
		}
	}


	if (cgs.gametype == GT_TOURNAMENT)
	{
		CG_BEStatsBuildGeneral_Tournament(tokens, outCols);
	}
	else if (cgs.gametype == GT_TEAM)
	{
		CG_BEStatsBuildGeneral_Team(tokens, outCols);
	}
	else if (cgs.gametype == GT_CTF)
	{
		CG_BEStatsBuildGeneral_CTF(tokens, outCols);
		numRows = (set->statsStyle == 2) ? 6 : 3;
	}
	else if (cgs.gametype == GT_CA)
	{
		CG_BEStatsBuildGeneral_CA(tokens, outCols);
	}
	else
	{
		CG_BEStatsBuildGeneral_Default(tokens, outCols);
	}

	*outRows = numRows;
}





qboolean CG_BEStatsBuildWeaponStats(BEStatToken_t tokens[32][12], int* rowOut)
{
	const newStatsInfo_t* s = &cgs.newStats;
	int col, row, i;
	const weaponStats_t* ws;
	char buf[64];
	qboolean any = qfalse;
	row = *rowOut;

	// Header
	if (set->statsStyle != 2)
	{
		CG_BEStatsSetTokenLabel(tokens, row, 0, "Weapon", beStatsSettings.defaultColor);
		CG_BEStatsSetTokenWidth(tokens, row, 0,  10.0f);

		CG_BEStatsSetTokenLabel(tokens, row, 1, "Accrcy", beStatsSettings.colorY);

		CG_BEStatsSetTokenLabel(tokens, row, 2, "Hits/Atts", beStatsSettings.defaultColor);
		CG_BEStatsSetTokenWidth(tokens, row, 2, 8.0f);
		CG_BEStatsSetTokenOffset(tokens, row, 2, 4.0f);
		CG_BEStatsAddTokenTextFlags(tokens, row, 2, DS_HCENTER);

		CG_BEStatsSetTokenLabel(tokens, row, 3, "Kills", beStatsSettings.colorG);


		CG_BEStatsSetTokenLabel(tokens, row, 4, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenLabel(tokens, row, 5, "PkUp", beStatsSettings.colorG);
		CG_BEStatsSetTokenLabel(tokens, row, 6, "Drop", beStatsSettings.colorR);

		row++;
	}
	// Separator
	CG_BEStatsSetTokenLabel(tokens, row, 0, "#hr", beStatsSettings.defaultColor);
	for (col = 1; col < 12; col++)
	{
		tokens[row][col].text[0] = '\0';
	}
	row++;

	for (i = WP_GAUNTLET; i < WP_NUM_WEAPONS; i++)
	{
		ws = &s->stats[i];

		if (ws->shots == 0 && ws->hits == 0 && ws->kills == 0 &&
		        ws->deaths == 0 && ws->pickUps == 0 && ws->drops == 0)
		{
			continue;
		}

		if (set->statsStyle == 2 && ws->deaths > 0 &&
		        ws->shots == 0 && ws->hits == 0 && ws->kills == 0 &&
		        ws->pickUps == 0 && ws->drops == 0)
		{
			continue;
		}

		col = 0;

		if (set->statsStyle == 2)
		{
			// Weapon icon
			Com_sprintf(buf, sizeof(buf), "#image cg_weapons[%d].weaponIcon", i);
			CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.defaultColor);
			CG_BEStatsSetTokenWidth(tokens, row, col, 1.4f);
			col++;

			// Accuracy
			if (i == WP_GAUNTLET)
			{
				CG_BEStatsSetTokenLabel(tokens, row, col, "", beStatsSettings.defaultColor);
			}
			else
			{
				Com_sprintf(buf, sizeof(buf), "%.1f", ws->accuracy);
				CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorY);
			}
			CG_BEStatsSetTokenWidth(tokens, row, col, 3.0f);
			col++;
		}
		else
		{
			// Weapon name
			CG_BEStatsSetTokenLabel(tokens, row, col, weaponNames[i], beStatsSettings.defaultColor);
			CG_BEStatsSetTokenWidth(tokens, row, col, 10.0f);
			col++;

			// Accuracy
			if (i == WP_GAUNTLET)
			{
				CG_BEStatsSetTokenLabel(tokens, row, col, "", beStatsSettings.defaultColor);
			}
			else
			{
				Com_sprintf(buf, sizeof(buf), "%.1f", ws->accuracy);
				CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorY);
			}
			col++;
		}

		// Hits/Atts
		Com_sprintf(buf, sizeof(buf), "%d/%d", ws->hits, ws->shots);
		CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenWidth(tokens, row, col, 8.0f);
		CG_BEStatsSetTokenOffset(tokens, row, 2, 4.0f);
		CG_BEStatsAddTokenTextFlags(tokens, row, col, DS_HCENTER);


		col++;

		// Kills
		Com_sprintf(buf, sizeof(buf), "%d", ws->kills);
		CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorG);
		col++;

		if (set->statsStyle != 2)
		{
			// Deaths
			Com_sprintf(buf, sizeof(buf), "%d", ws->deaths);
			CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorR);
			col++;

			// PickUps
			Com_sprintf(buf, sizeof(buf), "%d", ws->pickUps);
			CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorG);
			col++;

			// Drops
			Com_sprintf(buf, sizeof(buf), "%d", ws->drops);
			CG_BEStatsSetTokenLabel(tokens, row, col, buf, beStatsSettings.colorR);
			col++;
		}

		// Clear remaining tokens on this row
		for (; col < 12; col++)
		{
			tokens[row][col].text[0] = '\0';
		}

		row++;
		any = qtrue;
	}

	if (!any)
	{
		if (set->statsStyle == 2)
		{
			CG_BEStatsSetTokenLabel(tokens, row, 0, "No weapon data", beStatsSettings.colorY);
			CG_BEStatsSetTokenWidth(tokens, row, 0, 20.0f);
			row++;
		}
		else
		{
			CG_BEStatsSetTokenLabel(tokens, row++, 0, "No additional weapon info available.", beStatsSettings.colorY);
		}
		CG_BEStatsSetTokenLabel(tokens, row++, 0, " ", beStatsSettings.defaultColor);
	}

	*rowOut = row;
	return any;
}





void CG_BEStatsBuildBonusStats(BEStatToken_t tokens[32][12], int* rowOut)
{
	const newStatsInfo_t* s = &cgs.newStats;
	int row = *rowOut;
	char buf[MAX_TOKEN_LEN];
	char armorText[MAX_TOKEN_LEN];
	char healthText[MAX_TOKEN_LEN];

	// Armor
	buf[0] = '\0';
	if (s->ga > 0)
		Com_sprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d ^2GA^7", s->ga);
	if (s->ya > 0)
		Com_sprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s%d ^3YA^7", buf[0] ? " " : "", s->ya);
	if (s->ra > 0)
		Com_sprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s%d ^1RA^7", buf[0] ? " " : "", s->ra);
	if (buf[0])
		Com_sprintf(armorText, sizeof(armorText), "(%s)", buf);
	else
		armorText[0] = '\0';

	// Health
	if (s->megahealth > 0)
		Com_sprintf(healthText, sizeof(healthText), "(%d ^5MH^7)", s->megahealth);
	else
		healthText[0] = '\0';

	// Empty
	CG_BEStatsSetTokenLabel(tokens, row++, 0, " ", beStatsSettings.defaultColor);

	// style 2
	if (set->statsStyle == 2)
	{
		char givenReceived[32];
		Com_sprintf(givenReceived, sizeof(givenReceived), "^2%d^7/^1%d", (int)s->dmgGiven, (int)s->dmgReceived);

		CG_BEStatsSetTokenLabel(tokens, row, 0, givenReceived, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenValueFloat(tokens, row, 2, "%.2f", s->damageRatio, beStatsSettings.colorY);
		row++;
	}
	else // Style 1
	{


		CG_BEStatsSetTokenLabel(tokens, row, 0, "Damage Given:", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, row, 2, "%d", (int)s->dmgGiven, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, row, 3, "Armor:", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, row, 4, "%d", s->armor, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(tokens, row, 5, armorText, beStatsSettings.defaultColor);
		row++;

		CG_BEStatsSetTokenLabel(tokens, row, 0, "Damage Recvd:", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(tokens, row, 2, "%d", (int)s->dmgReceived, beStatsSettings.defaultColor);

		CG_BEStatsSetTokenLabel(tokens, row, 3, "Health:", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(tokens, row, 4, "%d", s->health, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(tokens, row, 5, healthText, beStatsSettings.defaultColor);
		row++;

		CG_BEStatsSetTokenLabel(tokens, row, 0, "Damage Ratio:", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(tokens, row, 2, "%.2f", s->damageRatio, beStatsSettings.defaultColor);
		row++;
	}

	if (cgs.gametype == GT_TEAM && set->statsStyle != 2)
	{
		CG_BEStatsSetTokenLabel(tokens, row, 0, "Team Damage:", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(tokens, row, 2, "%d", s->teamDamage, beStatsSettings.defaultColor);
		row++;
	}

	*rowOut = row;
}





void CG_BEStatsDrawWindowBackground(float x, float y, float w, float h)
{

	/* if (style == 2) {
	    // trap_R_SetColor(bgColor);
	    CG_AdjustFrom640(&x, &y, &w, &h);
	    trap_R_DrawStretchPic(x, y, w, h,
	                            0, 0, 1, 1, cgs.media.teamStatusBar);
	    // trap_R_SetColor(NULL);
	}
	else { */
	CG_FillRect(x, y, w, h, set->bgColor);
	/* } */
}

void CG_BEStatsDrawWindow(BEStatToken_t tokens[32][12], int rows)
{
	float maxWidth = 0.0f;
	float windowHeight, windowWidth;
	float colWidth;
	int i, j;
	const char* text;
	int textFlags = DS_PROPORTIONAL | DS_SHADOW;

	for (i = 0; i < rows; ++i)
	{
		float rowWidth = 0.0f;

		int lastNonEmpty = -1;
		for (j = 0; j < 12; ++j)
		{
			if (tokens[i][j].text[0] != '\0')
				lastNonEmpty = j;
		}

		for (j = 0; j <= lastNonEmpty; ++j)
		{
			colWidth = (tokens[i][j].width > 0.0f) ? tokens[i][j].width : set->baseColWidth;
			rowWidth += colWidth;

			if (j < lastNonEmpty)
				rowWidth += set->colSpacing;
		}

		if (rowWidth > maxWidth)
			maxWidth = rowWidth;
	}

	// Apply widthCutoff
	maxWidth -= set->widthCutoff;
	if (maxWidth < 0)
		maxWidth = 0;

	set->width = maxWidth;

	windowHeight = rows * set->rowHeight + set->padding * 2;
	set->height = windowHeight;
	windowWidth = set->width + set->padding * 2;

	CG_BEStatsDrawWindowBackground(set->x, set->y, windowWidth, set->height);
	CG_OSPDrawFrameAdjusted(set->x, set->y, windowWidth, set->height, defaultBorderSize, set->borderColor, 0);

	// Specials (#)
	for (i = 0; i < rows; ++i)
	{
		float baseY = set->y + set->padding + i * set->rowSpacing;
		float baseX = set->x + set->padding;

		int lastNonEmpty = -1;
		for (j = 0; j < 12; ++j)
		{
			if (tokens[i][j].text[0] != '\0')
				lastNonEmpty = j;
		}

		for (j = 0; j <= lastNonEmpty; ++j)
		{
			text = tokens[i][j].text;
			colWidth = (tokens[i][j].width > 0.0f) ? tokens[i][j].width : set->baseColWidth;

			if (text[0] == '#')
			{
				if (strcmp(text, "#hr") == 0)
				{
					float lineY = baseY + set->textHeight * 0.5f;
					vec4_t singleBorder = { 0, 0, 0, 1 };

					CG_OSPDrawFrameAdjusted(set->x, lineY, windowWidth, defaultBorderSize[3], singleBorder, set->borderColor, qfalse);
					break;
				}
				else if (Q_strncmp(text, "#image ", 7) == 0)
				{
					const char* mediaPath = text + 7;
					qhandle_t shader = CG_BEStatsResolveMediaHandle(mediaPath);

					if (shader)
					{
						float iconX = baseX + (colWidth - set->iconSizeW) * 0.5f;
						float iconY = baseY + (set->rowHeight - set->iconSizeH) * 0.5f;

						CG_DrawPicWithColor(iconX, iconY, set->iconSizeW, set->iconSizeH, colorWhite, shader);
					}
					else
					{
						CG_OSPDrawStringNew(baseX + tokens[i][j].offset, baseY, "img?", beStatsSettings.colorR, colorBlack,
						                    set->textWidth, set->textHeight,
						                    SCREEN_WIDTH, textFlags | tokens[i][j].textFlags,
						                    cgFontIndices.bestats, NULL, NULL, NULL);
					}

					baseX += colWidth + set->colSpacing;
					continue;
				}
			}

			if (text[0])
			{
				CG_OSPDrawStringNew(baseX + tokens[i][j].offset, baseY, text,
				                    tokens[i][j].color, colorBlack,
				                    set->textWidth, set->textHeight,
				                    SCREEN_WIDTH, textFlags | tokens[i][j].textFlags,
				                    cgFontIndices.bestats, NULL, NULL, NULL);
			}

			baseX += colWidth + set->colSpacing;
		}
	}
}

/*
 * Token bucket for getstatsinfo: 5 tokens, ~3.33/sec refill (1 token / 300 ms).
 * Refill is done lazily when we attempt to send a request.
 */
#define STATS_REQUEST_REFILL_PER_SEC (1000.0f / 300.0f)
void CG_RefillStatsRequestTokens(void)
{
	float elapsed;

	if (cg.statsRequestTokenTime == 0)
	{
		cg.statsRequestTokens = 5.0f;
		cg.statsRequestTokenTime = cg.time;
		return;
	}
	elapsed = (float)(cg.time - cg.statsRequestTokenTime) / 1000.0f;
	cg.statsRequestTokenTime = cg.time;
	cg.statsRequestTokens += STATS_REQUEST_REFILL_PER_SEC * elapsed;
	if (cg.statsRequestTokens > 5.0f)
		cg.statsRequestTokens = 5.0f;
}

qboolean CG_TrySendStatsRequest(void)
{
	if (cgs.fpsclassico)
		return qfalse;
	if (cg.intermissionStarted)
		return qfalse;

	CG_RefillStatsRequestTokens();

	if (cg.statsRequestTokens < 1.0f)
		return qfalse;

	cg.statsRequestTokens -= 1.0f;
	cg.statsRequestLastSendTime = cg.time;
	trap_SendClientCommand("getstatsinfo");
	return qtrue;
}

void CG_AddStatsRequestToken(void)
{
	cg.statsRequestTokens += 1.0f;
	if (cg.statsRequestTokens > 5.0f)
		cg.statsRequestTokens = 5.0f;
}

/*
 * Token bucket for getdstatus: 5 tokens, 1.0/sec refill (same as original getstatsinfo).
 * Refill is done lazily when we attempt to send a request. Not sent during intermission (table opens once, one send is enough).
 */
#define DSTATUS_REQUEST_REFILL_PER_SEC 1.0f
void CG_RefillDstatusRequestTokens(void)
{
	float elapsed;

	if (cg.dstatusRequestTokenTime == 0)
	{
		cg.dstatusRequestTokens = 5.0f;
		cg.dstatusRequestTokenTime = cg.time;
		return;
	}
	elapsed = (float)(cg.time - cg.dstatusRequestTokenTime) / 1000.0f;
	cg.dstatusRequestTokenTime = cg.time;
	cg.dstatusRequestTokens += DSTATUS_REQUEST_REFILL_PER_SEC * elapsed;
	if (cg.dstatusRequestTokens > 5.0f)
		cg.dstatusRequestTokens = 5.0f;
}

#define DSTATUS_REQUEST_INTERVAL_MS        800
#define DSTATUS_REQUEST_INTERVAL_DEAD_MS 1500  /* when dead (e.g. warmup) not in freeze: same idea as frozen — throttle to 1.5s */

qboolean CG_TrySendDstatusRequest(void)
{
	int intervalMs;

	if (cg.intermissionStarted)
		return qfalse;
	if (!cgs.q3msk || (cgs.gametype != GT_TEAM && cgs.gametype != GT_CTF) || !cgs.osp.gameTypeFreeze)
		return qfalse;

	CG_RefillDstatusRequestTokens();

	if (cg.dstatusRequestTokens < 1.0f)
		return qfalse;
	/* When dead but not in freeze (e.g. warmup), throttle like when frozen — once per 1.5s to avoid spam */
	intervalMs = CG_IS_DEAD() ? DSTATUS_REQUEST_INTERVAL_DEAD_MS : DSTATUS_REQUEST_INTERVAL_MS;
	if (cg.dstatusRequestTime + intervalMs >= cg.time)
		return qfalse;

	cg.dstatusRequestTokens -= 1.0f;
	cg.dstatusRequestTime = cg.time;
	trap_SendClientCommand("getdstatus");
	return qtrue;
}

qboolean CG_GetDeadInFreeze(int clientNum)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return qfalse;
	if (cg.predictedDeadInFreeze[clientNum] >= 0)
		return (qboolean)cg.predictedDeadInFreeze[clientNum];
	return cgs.deadInFreeze[clientNum];
}

#define STATS_REQUEST_PERIODIC_MS 1500

void CG_BEStatsShowStatsInfo(void)
{
	int row = 0;
	int rowsAdded;
	int             current_target;
	qboolean        viewing_other;
	qboolean        target_changed;
	qboolean        periodic_send;

	if (cgs.fpsclassico)
	{
		return;
	}

	if (!cg.showWStats)
		return;

	if (cg.snap)
	{
		viewing_other = (cg.snap->ps.clientNum != cg.clientNum);
		current_target = viewing_other ? cg.snap->ps.clientNum : cg.clientNum;
		target_changed = (beStatsLastObservedClient != current_target);
		periodic_send = (cg.statsRequestLastSendTime == 0 ||
		                 cg.time - cg.statsRequestLastSendTime >= STATS_REQUEST_PERIODIC_MS);

		if (target_changed)
			beStatsLastObservedClient = current_target;
		if (target_changed && (viewing_other || !CG_GetDeadInFreeze(cg.clientNum)))
			CG_AddStatsRequestToken();
		/* When dead and not observing anyone: send only once. When dead and observing someone, or alive: allow periodic send. */
		if (target_changed || (periodic_send && (viewing_other || !CG_IS_DEAD())))
			CG_TrySendStatsRequest();
	}

	// Clear table here
	memset(tokens, 0, sizeof(tokens));
	// Init
	CG_BEStatsInit();
	// Request via CG_TrySendStatsRequest (token bucket): observer target change (cg_snapshot), +wstats window, scoreboard with cg_drawAccuracy
	// General
	CG_BEStatsBuildGeneral(tokens, row, &rowsAdded, NULL);
	row += rowsAdded;
	// Weapon
	CG_BEStatsBuildWeaponStats(tokens, &row);
	// Bonus
	CG_BEStatsBuildBonusStats(tokens, &row);
	// Draw
	CG_BEStatsDrawWindow(tokens, row);
}
