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

	float baseColWidth;

    float rowHeight;
    float rowSpacing;
    float colSpacing;

	float colWidth;
	float widthCutoff;

	float padding;

	vec4_t defaultColor;
	vec4_t colorR;
	vec4_t colorG;
	vec4_t colorB;
	vec4_t colorY;
	vec4_t colorC;
	vec4_t colorM;
} beStatsSettings_t;

beStatsSettings_t beStatsSettings;

static qboolean beStatsInitialized = qfalse;

static char tokens[32][12][64];
vec4_t tokenColors[32][12];
float tokenWidth[32][12];


void CG_BEStatsInitSettings(void)
{
	beStatsSettings_t* s = &beStatsSettings;

    s->x = 4;
    s->y = 340;

	s->textHeight = cg_bestats_textH.value;
	s->textWidth = cg_bestats_textW.value;

    s->padding = (s->textHeight + s->textWidth) / 4;

    s->rowHeight  = s->textHeight;
    s->rowSpacing = s->textHeight;
    s->colSpacing = s->textWidth;
    s->baseColWidth = s->textWidth * 6;

    s->widthCutoff = s->textWidth * 3;
}

void CG_BEStatsInitColors(const vec4_t color)
{
	int row, col;
	for (row = 0; row < 32; row++)
	{
		for (col = 0; col < 12; col++)
		{
			Vector4Copy(color, tokenColors[row][col]);
		}

		Vector4Copy(color, beStatsSettings.defaultColor);
		Vector4Set(beStatsSettings.colorR, 1, 0, 0, 1);
		Vector4Set(beStatsSettings.colorG, 0, 1, 0, 1);
		Vector4Set(beStatsSettings.colorB, 0, 0, 1, 1);
		Vector4Set(beStatsSettings.colorY, 1, 1, 0, 1);
		Vector4Set(beStatsSettings.colorC, 0, 1, 1, 1);
		Vector4Set(beStatsSettings.colorM, 1, 0, 1, 1);
	}
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


void CG_BEStatsBuildGeneral(char tokens[32][12][MAX_TOKEN_LEN], int startRow, int* outRows, int* outCols)
{
	int i;
	int col = 0;
	const newStatsInfo_t* s = &cgs.be.newStats;
	int mins = s->flagTime / 60;
	float secs = (float)(s->flagTime % 60);
	vec4_t c = {1, 1, 1, 1};

	// Очистка первых трёх строк
	for (i = 0; i < 12; i++)
	{
		tokens[0][i][0] = '\0';  // заголовки
		tokens[1][i][0] = '\0';  // значения
		tokens[2][i][0] = '\0';  // запасная строка
	}

	if (cgs.gametype == GT_TOURNAMENT)
	{
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
	}
	else if (cgs.osp.gameTypeFreeze)
	{
		CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "WINS", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->wins, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Thaws", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->losses, beStatsSettings.defaultColor);
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
	}
	else if (cgs.gametype == GT_TEAM)
	{
		CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "NET", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score - s->deaths, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);
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
	}
	else if (cgs.gametype == GT_CTF)
	{
		CG_BEStatsSetTokenLabel(0, col, "Score", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->score, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Klls", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->kills, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Dths", beStatsSettings.colorR);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->deaths, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Caps", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->caps, beStatsSettings.defaultColor);
		{
			char timeStr[MAX_TOKEN_LEN];
			Com_sprintf(timeStr, sizeof(timeStr), "%d:%02.1f", mins, secs);
			Q_strncpyz(tokens[1][col], timeStr, MAX_TOKEN_LEN);
			CG_BEStatsSetTokenLabel(0, col++, "Ftime", beStatsSettings.colorG);
		}
		CG_BEStatsSetTokenLabel(0, col, "Asst", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->assists, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Dfns", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->defences, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "Rtrn", beStatsSettings.colorG);
		CG_BEStatsSetTokenValueInt(1, col++, "%d", s->returns, beStatsSettings.defaultColor);
		CG_BEStatsSetTokenLabel(0, col, "K/D", beStatsSettings.colorY);
		CG_BEStatsSetTokenValueFloat(1, col++, "%.1f", s->kdratio, beStatsSettings.defaultColor);
	}
	else if (cgs.gametype == GT_CA)
	{
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
	}
	else
	{
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
	}

	// Очистка третьей строки (можно оставить как заглушку)
	Q_strncpyz(tokens[2][0], " ", MAX_TOKEN_LEN);
	for (i = 1; i < 12; i++)
	{
		tokens[2][i][0] = '\0';
	}

	*outRows = 3;
	*outCols = col;
}


qboolean CG_BEStatsBuildWeaponStats(char tokens[][12][64], int* rowOut)
{
	const newStatsInfo_t* s = &cgs.be.newStats;
	int col, row, i;
	const char* name;
	const weaponStats_t* ws;
	char buf[64];
	qboolean any = qfalse;
	vec4_t c = {1, 1, 1, 1};

	row = *rowOut;

	// Заголовок
	CG_BEStatsSetTokenLabel(row, 0, "Weapon", beStatsSettings.defaultColor);
	CG_BEStatsSetTokenWidth(row, 0, 10.0f);
	CG_BEStatsSetTokenLabel(row, 1, "Accrcy", beStatsSettings.colorY);
	CG_BEStatsSetTokenLabel(row, 2, "Hits/Atts", beStatsSettings.defaultColor);
	CG_BEStatsSetTokenLabel(row, 3, "Kills", beStatsSettings.colorG);
	CG_BEStatsSetTokenLabel(row, 4, "Dths", beStatsSettings.colorR);
	CG_BEStatsSetTokenLabel(row, 5, "PkUp", beStatsSettings.colorG);
	CG_BEStatsSetTokenLabel(row, 6, "Drop", beStatsSettings.colorR);

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

		CG_BEStatsSetTokenLabel(row, col, weaponNames[i], beStatsSettings.defaultColor);
		CG_BEStatsSetTokenWidth(row, col, 10.0f);
		col++;

		Com_sprintf(buf, sizeof(buf), "%.1f", ws->accuracy);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorY);

		Com_sprintf(buf, sizeof(buf), "%d/%d", ws->hits, ws->shots);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.defaultColor);

		Com_sprintf(buf, sizeof(buf), "%d", ws->kills);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorG);

		Com_sprintf(buf, sizeof(buf), "%d", ws->deaths);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorR);

		Com_sprintf(buf, sizeof(buf), "%d", ws->pickUps);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorG);

		Com_sprintf(buf, sizeof(buf), "%d", ws->drops);
		CG_BEStatsSetTokenLabel(row, col++, buf, beStatsSettings.colorR);


		for (; col < 12; col++)
		{
			tokens[row][col][0] = '\0';
		}

		row++;
		any = qtrue;
	}

	if (!any)
	{
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
	vec4_t c = {1, 1, 1, 1};
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

	// Пустая строка
	CG_BEStatsSetTokenLabel(row++, 0, " ", beStatsSettings.defaultColor);

	
    // Сначала выводим урон
    CG_BEStatsSetTokenLabel(row, 0, "Damage Given:", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(row, 2, "%d", (int)s->dmgGiven, beStatsSettings.defaultColor);

    // Armor
    CG_BEStatsSetTokenLabel(row, 3, "Armor:", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(row, 4, "%d", s->armor, beStatsSettings.defaultColor);
    Q_strncpyz(tokens[row][5], armorText, MAX_TOKEN_LEN);
    Vector4Copy(beStatsSettings.defaultColor, tokenColors[row][5]);
    row++;

    // Damage Received
    CG_BEStatsSetTokenLabel(row, 0, "Damage Recvd:", beStatsSettings.colorY);
    CG_BEStatsSetTokenValueInt(row, 2, "%d", (int)s->dmgReceived, beStatsSettings.defaultColor);

    // Health
    CG_BEStatsSetTokenLabel(row, 3, "Health:", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueInt(row, 4, "%d", s->health, beStatsSettings.defaultColor);
    Vector4Copy(beStatsSettings.defaultColor, tokenColors[row][5]);
    Q_strncpyz(tokens[row][5], healthText, MAX_TOKEN_LEN);
    row++;

    if (cgs.gametype == GT_TEAM)
    {
        CG_BEStatsSetTokenLabel(row, 0, "Team Damage:", beStatsSettings.colorR);
        CG_BEStatsSetTokenValueInt(row, 2, "%d", s->teamDamage, beStatsSettings.defaultColor);
        row++;
    }
    // Damage Ratio:
    CG_BEStatsSetTokenLabel(row, 0, "Damage Ratio:", beStatsSettings.colorG);
    CG_BEStatsSetTokenValueFloat(row, 2, "%.2f", s->damageRatio, beStatsSettings.defaultColor);
    row++;
    
	*rowOut = row;
}




void CG_BEStatsDrawWindow(char tokens[32][12][MAX_TOKEN_LEN], int rows, int maxColsPerRow[12])
{
    beStatsSettings_t* s = &beStatsSettings;
    float maxWidth = 0.0f;
    float windowHeight, windowWidth;
    float colWidth;
    int i, j;

    vec4_t bgColor = { 0.1f, 0.1f, 0.1f, 0.75f };
    vec4_t teamColor;
    const char* text;
    vec4_t defaultColor = {1, 1, 1, 1};
    Vector4Copy(scoreboard_rtColor, teamColor);

    // Вычисление максимальной ширины окна по всем строкам
    for (i = 0; i < rows; ++i)
    {
        float rowWidth = 0.0f;
        for (j = 0; j < maxColsPerRow[i]; ++j)
        {
            colWidth = tokenWidth[i][j] > 0 ? tokenWidth[i][j] : s->baseColWidth;
            rowWidth += colWidth;
            if (j < maxColsPerRow[i] - 1)
            {
                rowWidth += s->colSpacing;
            }
        }
        if (rowWidth > maxWidth)
        {
            maxWidth = rowWidth;
        }
    }

    // Применяем widthCutoff
    maxWidth -= s->widthCutoff;
    if (maxWidth < 0)
        maxWidth = 0;

    s->width = maxWidth;

    windowHeight = rows * s->rowHeight + s->padding * 2;
    s->height = windowHeight;
    windowWidth = s->width + s->padding * 2;

    CG_FontSelect(cg_bestats_font.integer);
    CG_FillRect(s->x, s->y, windowWidth, s->height, bgColor);
    CG_OSPDrawFrameAdjusted(s->x, s->y, windowWidth, s->height, defaultBorderSize, teamColor, 0);

    // Отрисовка текста и спец. токенов
    for (i = 0; i < rows; ++i)
    {
        float baseY = s->y + s->padding + i * s->rowSpacing;
        float baseX = s->x + s->padding;

        for (j = 0; j < maxColsPerRow[i]; ++j)
        {
            text = tokens[i][j];
            colWidth = tokenWidth[i][j] > 0 ? tokenWidth[i][j] : s->baseColWidth;

            if (text[0] == '#')
            {
                if (strcmp(text, "#hr") == 0)
                {
                    vec4_t lineColor;
                    float lineY = baseY + s->textHeight * 0.5f;
                    Vector4Copy(s->defaultColor, lineColor);
                    lineColor[3] = 0.3f;

                    CG_DrawRect(s->x, lineY, windowWidth, 1.0f, 1.0f, lineColor);
                    break;
                }
            }

            if (text[0])
            {
                CG_OSPDrawStringNew(baseX, baseY, text,
                                    tokenColors[i][j], colorBlack,
                                    s->textWidth, s->textHeight,
                                    SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW,
                                    NULL, NULL, NULL);
            }

            baseX += colWidth + s->colSpacing;
        }
    }
}



void CG_BEStatsShowStatsInfo(void)
{
	int i, c;
	int row = 0;
	int cols;
	int rowsAdded;
	int maxColsPerRow[64]; // для каждой строки — количество колонок
    int bonusStartRow, bonusRowsCount;
    static qboolean initialized = qfalse;


	// Очистка таблицы
	memset(tokens, 0, sizeof(tokens));
	memset(tokenColors, 0, sizeof(tokenColors));
	memset(tokenWidth, 0, sizeof(tokenWidth));
	memset(maxColsPerRow, 0, sizeof(maxColsPerRow));

	// General
    CG_BEStatsInit();
    CG_MaybeRequestStatsInfo();
	CG_BEStatsBuildGeneral(tokens, row, &rowsAdded, &cols);
	for (i = row; i < row + rowsAdded; i++)
	{
		maxColsPerRow[i] = cols;
	}
	row += rowsAdded;

	// Weapon
	CG_BEStatsBuildWeaponStats(tokens, &row);

	for (i = 3; i < row; i++)
	{
		c = 0;
		while (c < 12 && tokens[i][c][0])
			++c;
		maxColsPerRow[i] = c ? c : 1;
	}

	// Bonus
	bonusStartRow = row;
	CG_BEStatsBuildBonusStats(tokens, &row);
	bonusRowsCount = row - bonusStartRow;

	for (i = bonusStartRow; i < row; ++i)
	{
		maxColsPerRow[i] = 8;
	}

	CG_BEStatsDrawWindow(tokens, row, maxColsPerRow);
}
