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


#define MAX_TOKEN_LEN MAX_QPATH


typedef struct beStatsSettings_t
{
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

static qboolean beStatsInitialized = qfalse;

static char tokens[32][12][64];
vec4_t tokenColors[32][12];
float tokenWidth[32][12];


void CG_BEStatsInitSettings(void)
{
	beStatsSettings_t* set = &beStatsSettings;
	globalBeStatsSettings_t* inc = &cgs.be.newStats.settings;

    set->x = inc->x;
    set->y = inc->y;

	set->textWidth = inc->textSize[0];
	set->textHeight = inc->textSize[1];

	set->iconSizeW = set->textWidth * 1.1;
    set->iconSizeH = set->textHeight * 1.1;

    set->padding = (set->textHeight + set->textWidth) / 8;

    set->rowHeight  = set->textHeight;
    set->rowSpacing = set->textHeight;
    set->colSpacing = set->textWidth;
    set->baseColWidth = set->textWidth * 6;

    set->widthCutoff = 0;
}

void CG_BEStatsInitColors(const vec4_t color)
{
	beStatsSettings_t* set = &beStatsSettings;
	int row, col;
	for (row = 0; row < 32; row++)
	{
		for (col = 0; col < 12; col++)
		{
			Vector4Copy(color, tokenColors[row][col]);
		}

	}
	// Default color sets
	Vector4Copy(color, set->defaultColor);
	Vector4Set(set->colorR, 1, 0, 0, 1);
	Vector4Set(set->colorG, 0, 1, 0, 1);
	Vector4Set(set->colorB, 0, 0, 1, 1);
	Vector4Set(set->colorY, 1, 1, 0, 1);
	Vector4Set(set->colorC, 0, 1, 1, 1);
	Vector4Set(set->colorM, 1, 0, 1, 1);

	// Team colors
	if (cgs.gametype >= GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			Vector4Copy(cgs.be.redTeamColor, set->teamColor);
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			Vector4Copy(cgs.be.blueTeamColor, set->teamColor);
		} else {
			Vector4Copy(colorMdGrey, set->teamColor);
		}
	} else {
		Vector4Copy(colorMdGrey, set->teamColor);
	}

	// Window colors
	// Vector4Set(set->bgColor, set->teamColor[0], set->teamColor[1], set->teamColor[2], (set->teamColor[3] / 2));
	if (!cgs.be.newStats.settings.bgColorIsSet)
		CG_OSPAdjustTeamColorHalfed(set->teamColor, set->bgColor);
	else {
		Vector4Set(set->bgColor, cgs.be.newStats.settings.bgColor[0], cgs.be.newStats.settings.bgColor[1], cgs.be.newStats.settings.bgColor[2], cg_bestats_bgOpaque.value);
	}

	Vector4Copy(colorBlack, set->borderColor);
}


void CG_BEStatsInit(void) {
    if (!beStatsInitialized)
    {
        CG_BEStatsInitSettings();
        CG_BEStatsInitColors(colorWhite);
        // CG_Printf("Init ^2Once\n");
        beStatsInitialized = qtrue;
    }
}

void CG_BEStatsResetInit(void) {
	beStatsInitialized = qfalse;
}


void CG_BEStatsSetTokenLabel(int row, int col, const char* label, const vec4_t color)
{
	Q_strncpyz(tokens[row][col], label, MAX_TOKEN_LEN);
	if (color)
	{
		Vector4Copy(color, tokenColors[row][col]);
	}
}

void CG_BEStatsSetTokenValueInt(int row, int col, const char* fmt, int value, const vec4_t color)
{
	char buf[MAX_TOKEN_LEN];
	Com_sprintf(buf, sizeof(buf), fmt, value);
	Q_strncpyz(tokens[row][col], buf, MAX_TOKEN_LEN);
	if (color)
	{
		Vector4Copy(color, tokenColors[row][col]);
	}
}

void CG_BEStatsSetTokenValueFloat(int row, int col, const char* fmt, float value, const vec4_t color)
{
	char buf[MAX_TOKEN_LEN];
	Com_sprintf(buf, sizeof(buf), fmt, value);
	Q_strncpyz(tokens[row][col], buf, MAX_TOKEN_LEN);
	if (color)
	{
		Vector4Copy(color, tokenColors[row][col]);
	}
}

void CG_BEStatsSetTokenWidth(int row, int col, float multipy)
{
	tokenWidth[row][col] = beStatsSettings.textWidth * multipy;
}

void CG_BEStatsSetTokenLabelValueInt(int rowLabel, int col, const char* label, const vec4_t labelColor,
                                     int rowValue, int value, const char* fmt, const vec4_t valueColor)
{
    CG_BEStatsSetTokenLabel(rowLabel, col, label, labelColor);
    CG_BEStatsSetTokenValueInt(rowValue, col, fmt, value, valueColor);
}

void CG_BEStatsSetTokenLabelValueFloat(int rowLabel, int col, const char* label, const vec4_t labelColor,
                                       int rowValue, float value, const char* fmt, const vec4_t valueColor)
{
    CG_BEStatsSetTokenLabel(rowLabel, col, label, labelColor);
    CG_BEStatsSetTokenValueFloat(rowValue, col, fmt, value, valueColor);
}


qhandle_t CG_BEStatsResolveMediaHandle(const char* mediaPath) {
    if (!mediaPath) return 0;

    // cgs.media.xxx
    if (Q_strncmp(mediaPath, "cgs.media.", 10) == 0) {
        const char* fieldName = mediaPath + 10;

        if (strcmp(fieldName, "whiteShader") == 0) return cgs.media.whiteShader;
        // Добавь другие поля по мере необходимости

        Com_Printf("^1Unknown cgs.media field: %s\n", fieldName);
        return 0;
    }

    // cg_weapons[N].weaponIcon
    if (Q_strncmp(mediaPath, "cg_weapons[", 11) == 0) {
        const char* p = mediaPath + 11;
        int index = 0;

        while (*p >= '0' && *p <= '9') {
            index = index * 10 + (*p - '0');
            p++;
        }

        if (*p != ']' || Q_strncmp(p + 1, ".weaponIcon", 11) != 0) {
            Com_Printf("^1Invalid cg_weapons field: %s\n", mediaPath);
            return 0;
        }

        if (index < 0 || index >= WP_NUM_WEAPONS) {
            Com_Printf("^1cg_weapons index out of range: %d\n", index);
            return 0;
        }

        return cg_weapons[index].weaponIcon;
    }

    return 0;
}

static void CG_BEStatsBuildGeneral_Tournament(char tokens[32][12][MAX_TOKEN_LEN], int* outCols)
{
    int col = 0;
    const newStatsInfo_t* s = &cgs.be.newStats;

    CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Sui", beStatsSettings.colorR);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->suicides, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Effncy", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "WINS", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->wins, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "LOSSES", beStatsSettings.colorR);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->losses, beStatsSettings.defaultColor);

    *outCols = col;
}

static void CG_BEStatsBuildGeneral_Team(char tokens[32][12][MAX_TOKEN_LEN], int* outCols, qboolean isFreeze)
{
    int col = 0;
    const newStatsInfo_t* s = &cgs.be.newStats;
    int style = cg_bestats_style.integer;  // предположим, это глобальная переменная или настройка

    if (style == 2) {
        // Второй стиль — 5 строк с двумя рядами заголовков и значений

        // Первая строка — заголовки
        CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
        col++;
        CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
        col++;
        if (isFreeze) {
            CG_BEStatsSetTokenLabel(0, col, "Thaws", beStatsSettings.colorG);
        } else {
            CG_BEStatsSetTokenLabel(0, col, "Wins/Net", beStatsSettings.colorY);
        }
        col++;

        // Вторая строка — значения под первыми заголовками
        col = 0;
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);
        if (isFreeze) {
            CG_BEStatsSetTokenValueInt(1, col++, "%d", s->losses, beStatsSettings.defaultColor);
        } else {
            CG_BEStatsSetTokenValueInt(1, col++, "%d", s->wins, beStatsSettings.defaultColor);
            // Можно сюда добавить "Net" (score - deaths) как отдельное поле, если нужно
        }

        // Третья строка — новые заголовки
        col = 0;
		CG_BEStatsSetTokenLabel(2, col++, "K/D", beStatsSettings.colorY);
        CG_BEStatsSetTokenLabel(2, col++, "Dths", beStatsSettings.colorR);
        CG_BEStatsSetTokenLabel(2, col++, "Sui", beStatsSettings.colorR);

        // Четвёртая строка — значения под ними
        col = 0;
        CG_BEStatsSetTokenValueInt(3, col++, "%d", s->deaths, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueInt(3, col++, "%d", s->suicides, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueFloat(3, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

        // Пятая строка — пустая, не трогаем

        *outCols = col; // 3 колонки

    } else {
        // Стандартный стиль — одна строка с колонками

        col = 0;

        CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);

        if (isFreeze) {
            CG_BEStatsSetTokenLabel(0, col, "WINS", beStatsSettings.colorY);
            CG_BEStatsSetTokenValueInt(1, col++, "%d", s->wins, beStatsSettings.defaultColor);
        } else {
            CG_BEStatsSetTokenLabel(0, col, "NET", beStatsSettings.colorY);
            CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score - s->deaths, beStatsSettings.defaultColor);
        }

        CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);

        if (isFreeze) {
            CG_BEStatsSetTokenLabel(0, col, "Thaws", beStatsSettings.colorG);
            CG_BEStatsSetTokenValueInt(1, col++, "%d", s->losses, beStatsSettings.defaultColor);
        }

        CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Sui", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->suicides, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "TmKills", beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->teamKills, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Effncy", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

        *outCols = col;
    }
}



static void CG_BEStatsBuildGeneral_CTF(char tokens[32][12][MAX_TOKEN_LEN], int* outCols)
{
    int col = 0;
    const newStatsInfo_t* s = &cgs.be.newStats;
    int mins = s->flagTime / 60;
    float secs = (float)(s->flagTime % 60);
    char timeStr[MAX_TOKEN_LEN];

    CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Caps", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->caps, beStatsSettings.defaultColor);

    Com_sprintf(timeStr, sizeof(timeStr), "%d:%02.1f", mins, secs);
    Q_strncpyz(tokens[1][col], timeStr, MAX_TOKEN_LEN);
    CG_BEStatsSetTokenLabel(0, col++, "Ftime", beStatsSettings.colorG);

    CG_BEStatsSetTokenLabel(0, col, "Asst", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->assists, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Dfns", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->defences, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Rtrn", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->returns, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

    *outCols = col;
}

static void CG_BEStatsBuildGeneral_CA(char tokens[32][12][MAX_TOKEN_LEN], int* outCols)
{
    int col = 0;
    const newStatsInfo_t* s = &cgs.be.newStats;

    CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "Effncy", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "DmgScr", beStatsSettings.colorC);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", (int)(s->dmgGiven / 100), beStatsSettings.defaultColor);

    CG_BEStatsSetTokenLabel(0, col, "WINS", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(1, col++, "%d", s->wins, beStatsSettings.defaultColor);

    *outCols = col;
}

static void CG_BEStatsBuildGeneral_Default(char tokens[32][12][MAX_TOKEN_LEN], int* outCols)
{
    int style = cg_bestats_style.integer;
    const newStatsInfo_t* s = &cgs.be.newStats;

    if (style == 2) {
        // style == 2 — 5 строк по твоему макету

        // 1 строка: заголовки Score Klls K/D
        CG_BEStatsSetTokenLabel(0, 0, "Score", beStatsSettings.colorY);
        CG_BEStatsSetTokenLabel(0, 1, "Klls", beStatsSettings.colorG);
        CG_BEStatsSetTokenLabel(0, 2, "K/D", beStatsSettings.colorY);

        // 2 строка: значения для Score Klls K/D
        CG_BEStatsSetTokenValueInt(1, 0, "%d", s->score, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueInt(1, 1, "%d", s->kills, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueFloat(1, 2, "%.1f", s->kdratio, beStatsSettings.defaultColor);

        // 3 строка: заголовки Dths Sui
        CG_BEStatsSetTokenLabel(2, 0, "Dths", beStatsSettings.colorR);
        CG_BEStatsSetTokenLabel(2, 1, "Sui", beStatsSettings.colorR);

        // 4 строка: значения для Dths Sui
        CG_BEStatsSetTokenValueInt(3, 0, "%d", s->deaths, beStatsSettings.defaultColor);
        CG_BEStatsSetTokenValueInt(3, 1, "%d", s->suicides, beStatsSettings.defaultColor);

        *outCols = 3; // максимум по столбцам (для первой и второй строки)
    }
    else {

        int col = 0;

        CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Sui", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(1, col++, "%d", s->suicides, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);

        CG_BEStatsSetTokenLabel(0, col, "Effncy", beStatsSettings.colorY);
        CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->efficiency, beStatsSettings.defaultColor);

        *outCols = col;
    }
}


void CG_BEStatsBuildGeneral(char tokens[32][12][MAX_TOKEN_LEN], int startRow, int* outRows, int* outCols)
{
    int i;
	int row;
    int style = cg_bestats_style.integer;
    int numRows = (style == 2) ? 5 : 3;

    // Очистка строк 0 и 1 всегда
    for (i = 0; i < 12; i++) {
        tokens[0][i][0] = '\0';  // заголовки
        tokens[1][i][0] = '\0';  // значения
    }

    // Очистка запасных строк в зависимости от numRows
    for (row = 2; row < numRows; row++) {
        for (i = 0; i < 12; i++) {
            tokens[row][i][0] = '\0';
        }
    }

    // Вызов специфичных функций заполнения
    if (cgs.gametype == GT_TOURNAMENT) {
        CG_BEStatsBuildGeneral_Tournament(tokens, outCols);
    }
    else if (cgs.osp.gameTypeFreeze) {
        CG_BEStatsBuildGeneral_Team(tokens, outCols, qtrue);
    }
    else if (cgs.gametype == GT_TEAM) {
        CG_BEStatsBuildGeneral_Team(tokens, outCols, qfalse);
    }
    else if (cgs.gametype == GT_CTF) {
        CG_BEStatsBuildGeneral_CTF(tokens, outCols);
    }
    else if (cgs.gametype == GT_CA) {
        CG_BEStatsBuildGeneral_CA(tokens, outCols);
    }
    else {
        CG_BEStatsBuildGeneral_Default(tokens, outCols);
    }

    *outRows = numRows;
}



qboolean CG_BEStatsBuildWeaponStats(char tokens[][12][64], int* rowOut)
{
    const newStatsInfo_t* s = &cgs.be.newStats;
    int col, row, i;
    const weaponStats_t* ws;
    char buf[64];
    qboolean any = qfalse;
	int statsStyle = cg_bestats_style.integer;
    row = *rowOut;

    // Заголовок
    CG_BEStatsSetTokenLabel(row, 0, (cg_bestats_wpStyle.integer == 2 || statsStyle == 2) ? "WP" : "Weapon", beStatsSettings.defaultColor);
    CG_BEStatsSetTokenWidth(row, 0, (cg_bestats_wpStyle.integer == 2 || statsStyle == 2) ? 1.0f : 10.0f);
	CG_BEStatsSetTokenLabel(row, 1, (statsStyle == 2) ? "Acc" : "Accrcy", beStatsSettings.colorY);
	if (statsStyle == 2)
		CG_BEStatsSetTokenWidth(row, 1, 3.0f);
    CG_BEStatsSetTokenLabel(row, 2, "Hits/Atts", beStatsSettings.defaultColor);
    CG_BEStatsSetTokenLabel(row, 3, "Kills", beStatsSettings.colorG);

    if (statsStyle != 2)
    {
        CG_BEStatsSetTokenLabel(row, 4, "Dths", beStatsSettings.colorR);
        CG_BEStatsSetTokenLabel(row, 5, "PkUp", beStatsSettings.colorG);
        CG_BEStatsSetTokenLabel(row, 6, "Drop", beStatsSettings.colorR);
    }

    row++;

    // Разделитель
    CG_BEStatsSetTokenLabel(row, 0, "#hr", beStatsSettings.defaultColor);
    for (col = 1; col < 12; col++)
    {
        tokens[row][col][0] = '\0';
    }
    row++;

    for (i = WP_GAUNTLET + 1; i < WP_NUM_WEAPONS; i++)
    {
        ws = &s->stats[i];

        if (ws->shots == 0 && ws->hits == 0 && ws->kills == 0 &&
            ws->deaths == 0 && ws->pickUps == 0 && ws->drops == 0)
        {
            continue;
        }

        col = 0;

        if (statsStyle == 2)
        {
            // Иконка оружия
            Com_sprintf(buf, sizeof(buf), "#image cg_weapons[%d].weaponIcon", i);
            CG_BEStatsSetTokenLabel(row, col, buf, beStatsSettings.defaultColor);
            CG_BEStatsSetTokenWidth(row, col, 1.0f);
			col++;
			// Accuracy
			Com_sprintf(buf, sizeof(buf), "%.1f", ws->accuracy);
			CG_BEStatsSetTokenLabel(row, col, buf, beStatsSettings.colorY);
			CG_BEStatsSetTokenWidth(row, col, 3.0f);
			col++;
        }
        else
        {
            // Название оружия
            CG_BEStatsSetTokenLabel(row, col, weaponNames[i], beStatsSettings.defaultColor);
            CG_BEStatsSetTokenWidth(row, col, 10.0f);
			col++;
			// Accuracy
			Com_sprintf(buf, sizeof(buf), "%.1f", ws->accuracy);
			CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorY);
			col++;
        }



        // Hits/Atts
        Com_sprintf(buf, sizeof(buf), "%d/%d", ws->hits, ws->shots);
        CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.defaultColor);

        // Kills
        Com_sprintf(buf, sizeof(buf), "%d", ws->kills);
        CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorG);

        if (cg_bestats_style.integer != 2)
        {
            // Deaths
            Com_sprintf(buf, sizeof(buf), "%d", ws->deaths);
            CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorR);

            // PickUps
            Com_sprintf(buf, sizeof(buf), "%d", ws->pickUps);
            CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorG);

            // Drops
            Com_sprintf(buf, sizeof(buf), "%d", ws->drops);
            CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorR);
        }

        // Очищаем остальные токены на этой строке
        for (; col < 12; col++)
        {
            tokens[row][col][0] = '\0';
        }

        row++;
        any = qtrue;
    }

    if (!any)
	{
		if (statsStyle == 2)
			CG_BEStatsSetTokenLabel(row++, 0, "No weapon data", beStatsSettings.colorY);
		else 
			CG_BEStatsSetTokenLabel(row++, 0, "No additional weapon info available.", beStatsSettings.colorY);
        CG_BEStatsSetTokenLabel(row++, 0, " ", beStatsSettings.defaultColor);
    }

    *rowOut = row;
    return any;
}



void CG_BEStatsBuildBonusStats(char tokens[32][12][MAX_TOKEN_LEN], int* rowOut)
{
    const newStatsInfo_t* s = &cgs.be.newStats;
    int row = *rowOut;
    char buf[MAX_TOKEN_LEN];
    char armorText[MAX_TOKEN_LEN];
    char healthText[MAX_TOKEN_LEN];

    // Формируем пояснения для брони
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

    // Пояснение для здоровья
    if (s->megahealth > 0)
        Com_sprintf(healthText, sizeof(healthText), "(%d ^5MH^7)", s->megahealth);
    else
        healthText[0] = '\0';

    // Пустая строка - всегда выводим
    CG_BEStatsSetTokenLabel(row++, 0, " ", beStatsSettings.defaultColor);

    // Всегда выводим основные строки:
    CG_BEStatsSetTokenLabel(row, 0, "Damage Given:", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(row, 2, "%d", (int)s->dmgGiven, beStatsSettings.defaultColor);

    if (cg_bestats_style.integer != 2)
    {
        // Вывод брони только если стиль != 2
        CG_BEStatsSetTokenLabel(row, 3, "Armor:", beStatsSettings.colorG);
        CG_BEStatsSetTokenValueInt(row, 4, "%d", s->armor, beStatsSettings.defaultColor);
        Q_strncpyz(tokens[row][5], armorText, MAX_TOKEN_LEN);
        Vector4Copy(beStatsSettings.defaultColor, tokenColors[row][5]);
    }
    row++;

    CG_BEStatsSetTokenLabel(row, 0, "Damage Recvd:", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(row, 2, "%d", (int)s->dmgReceived, beStatsSettings.defaultColor);

    if (cg_bestats_style.integer != 2)
    {
        CG_BEStatsSetTokenLabel(row, 3, "Health:", beStatsSettings.colorG);
        CG_BEStatsSetTokenValueInt(row, 4, "%d", s->health, beStatsSettings.defaultColor);
        Vector4Copy(beStatsSettings.defaultColor, tokenColors[row][5]);
        Q_strncpyz(tokens[row][5], healthText, MAX_TOKEN_LEN);
    }
    row++;

    if (cgs.gametype == GT_TEAM && cg_bestats_style.integer != 2)
    {
        CG_BEStatsSetTokenLabel(row, 0, "Team Damage:", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(row, 2, "%d", s->teamDamage, beStatsSettings.defaultColor);
        row++;
    }

    CG_BEStatsSetTokenLabel(row, 0, "Damage Ratio:", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueFloat(row, 2, "%.2f", s->damageRatio, beStatsSettings.defaultColor);
    row++;

    *rowOut = row;
}



void CG_BEStatsDrawWindowBackground(float x, float y, float w, float h) {
	beStatsSettings_t* set = &beStatsSettings;
	int style = cg_bestats_bgStyle.integer;
	
	if (style == 1) {
		// trap_R_SetColor(bgColor);
		CG_AdjustFrom640(&x, &y, &w, &h);
		trap_R_DrawStretchPic(x, y, w, h,
								0, 0, 1, 1, cgs.media.beStats_background);
		// trap_R_SetColor(NULL);
	}
	else {
		set->teamColor[3] = 0.35;
		CG_FillRect(x, y, w, h, set->bgColor);
	}						  
}

void CG_BEStatsDrawWindow(char tokens[32][12][MAX_TOKEN_LEN], int rows)
{
    beStatsSettings_t* set = &beStatsSettings;
    float maxWidth = 0.0f;
    float windowHeight, windowWidth;
    float colWidth;
    int i, j;
    const char* text;

    // Вычисление максимальной ширины окна по всем строкам
    for (i = 0; i < rows; ++i)
    {
        float rowWidth = 0.0f;

        // Найти последнюю непустую колонку в строке
        int lastNonEmpty = -1;
        for (j = 0; j < 12; ++j)
        {
            if (tokens[i][j][0] != '\0')
                lastNonEmpty = j;
        }

        for (j = 0; j <= lastNonEmpty; ++j)
        {
            colWidth = tokenWidth[i][j] > 0 ? tokenWidth[i][j] : set->baseColWidth;
            rowWidth += colWidth;

            if (j < lastNonEmpty)
                rowWidth += set->colSpacing;
        }

        if (rowWidth > maxWidth)
            maxWidth = rowWidth;
    }

    // Применяем widthCutoff
    maxWidth -= set->widthCutoff;
    if (maxWidth < 0)
        maxWidth = 0;

    set->width = maxWidth;

    windowHeight = rows * set->rowHeight + set->padding * 2;
    set->height = windowHeight;
    windowWidth = set->width + set->padding * 2;

    CG_FontSelect(cg_bestats_font.integer);
    CG_BEStatsDrawWindowBackground(set->x, set->y, windowWidth, set->height);
    CG_OSPDrawFrameAdjusted(set->x, set->y, windowWidth, set->height, defaultBorderSize, set->borderColor, 0);

    // Отрисовка текста и спец. токенов
    for (i = 0; i < rows; ++i)
    {
        float baseY = set->y + set->padding + i * set->rowSpacing;
        float baseX = set->x + set->padding;

        // Найти последнюю непустую колонку
        int lastNonEmpty = -1;
        for (j = 0; j < 12; ++j)
        {
            if (tokens[i][j][0] != '\0')
                lastNonEmpty = j;
        }

        for (j = 0; j <= lastNonEmpty; ++j)
        {
            text = tokens[i][j];
            colWidth = tokenWidth[i][j] > 0 ? tokenWidth[i][j] : set->baseColWidth;

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
                        CG_OSPDrawStringNew(baseX, baseY, "[img?]", beStatsSettings.colorR, colorBlack,
                                            set->textWidth, set->textHeight,
                                            SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW,
                                            NULL, NULL, NULL);
                    }

                    baseX += colWidth + set->colSpacing;
                    continue;
                }
            }

            if (text[0])
            {
                CG_OSPDrawStringNew(baseX, baseY, text,
                                    tokenColors[i][j], colorBlack,
                                    set->textWidth, set->textHeight,
                                    SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW,
                                    NULL, NULL, NULL);
            }

            baseX += colWidth + set->colSpacing;
        }
    }
}

void CG_BEStatsShowStatsInfo(void)
{
	int row = 0;
	int rowsAdded;
    static qboolean initialized = qfalse;
    CG_Printf("CG_BEStatsShowStatsInfo called\n");

	// Очистка таблицы
	memset(tokens, 0, sizeof(tokens));
	memset(tokenColors, 0, sizeof(tokenColors));
	memset(tokenWidth, 0, sizeof(tokenWidth));

	// General
	CG_BEStatsInit();
	CG_MaybeRequestStatsInfo();
	CG_BEStatsBuildGeneral(tokens, row, &rowsAdded, NULL);
	row += rowsAdded;

	// Weapon
	CG_BEStatsBuildWeaponStats(tokens, &row);

	// Bonus
	CG_BEStatsBuildBonusStats(tokens, &row);

	CG_BEStatsDrawWindow(tokens, row);
}
