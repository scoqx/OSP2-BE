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

static char tokens[64][16][64];


void CG_OSPBuildGeneralStatsTokens(char tokens[64][16][MAX_TOKEN_LEN], int startRow, int *outRows, int *outCols) {
    int col, row;
    const newStatsInfo_t *s = &cgs.be.newStats;
    int mins = s->flagTime / 60;
    float secs = (float)(s->flagTime % 60);
    int cols = 0;

    // Обнуляем строки в текущем блоке на всякий случай (защита от мусора)
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
    }

    if (cgs.gametype == GT_TOURNAMENT) {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "^2Klls");
        strcpy(tokens[0][2], "^1Dths");
        strcpy(tokens[0][3], "Sui");
        strcpy(tokens[0][4], "^3K/D");
        strcpy(tokens[0][5], "^3Effcny");
        strcpy(tokens[0][6], "^2WINS");
        strcpy(tokens[0][7], "^1LOSSES");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%d", s->suicides);
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%.1f", s->kdratio);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%.1f", s->efficiency);
        Com_sprintf(tokens[1][6], MAX_TOKEN_LEN, "%d", s->wins);
        Com_sprintf(tokens[1][7], MAX_TOKEN_LEN, "%d", s->losses);

        cols = 8;
    }
    else if (cgs.osp.gameTypeFreeze) {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "WINS");
        strcpy(tokens[0][2], "^2Klls");
        strcpy(tokens[0][3], "Thws");
        strcpy(tokens[0][4], "^1Dths");
        strcpy(tokens[0][5], "Sui");
        strcpy(tokens[0][6], "^1TmKlls");
        strcpy(tokens[0][7], "^3K/D");
        strcpy(tokens[0][8], "^3Effcny");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->wins);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%d", s->losses); // Thaws = losses
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%d", s->suicides);
        Com_sprintf(tokens[1][6], MAX_TOKEN_LEN, "%d", s->teamKills);
        Com_sprintf(tokens[1][7], MAX_TOKEN_LEN, "%.1f", s->kdratio);
        Com_sprintf(tokens[1][8], MAX_TOKEN_LEN, "%.1f", s->efficiency);

        cols = 9;
    }
    else if (cgs.gametype == GT_TEAM) {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "NET");
        strcpy(tokens[0][2], "^2Klls");
        strcpy(tokens[0][3], "^1Dths");
        strcpy(tokens[0][4], "Sui");
        strcpy(tokens[0][5], "^1TmKlls");
        strcpy(tokens[0][6], "^3K/D");
        strcpy(tokens[0][7], "^3Effcny");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->score - s->deaths);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%d", s->suicides);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%d", s->teamKills);
        Com_sprintf(tokens[1][6], MAX_TOKEN_LEN, "%.1f", s->kdratio);
        Com_sprintf(tokens[1][7], MAX_TOKEN_LEN, "%.1f", s->efficiency);

        cols = 8;
    }
    else if (cgs.gametype == GT_CTF) {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "^2Klls");
        strcpy(tokens[0][2], "^1Dths");
        strcpy(tokens[0][3], "^3Caps");
        strcpy(tokens[0][4], "^2Ftime");
        strcpy(tokens[0][5], "Asst");
        strcpy(tokens[0][6], "Dfns");
        strcpy(tokens[0][7], "Rtrn");
        strcpy(tokens[0][8], "KD");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%d", s->caps);
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%d:%02.1f", mins, secs);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%d", s->assists);
        Com_sprintf(tokens[1][6], MAX_TOKEN_LEN, "%d", s->defences);
        Com_sprintf(tokens[1][7], MAX_TOKEN_LEN, "%d", s->returns);
        Com_sprintf(tokens[1][8], MAX_TOKEN_LEN, "%.1f", s->kdratio);

        cols = 9;
    }
    else if (cgs.gametype == GT_CA) {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "^2Klls");
        strcpy(tokens[0][2], "^1Dths");
        strcpy(tokens[0][3], "^3K/D");
        strcpy(tokens[0][4], "^3Effcny");
        strcpy(tokens[0][5], "^5DmgScr");
        strcpy(tokens[0][6], "^2WINS");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%.1f", s->kdratio);
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%.1f", s->efficiency);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%d", (int)(s->damageKoeff));
        Com_sprintf(tokens[1][6], MAX_TOKEN_LEN, "%d", s->wins);

        cols = 7;
    }
    else {
        strcpy(tokens[0][0], "^B^3Score");
        strcpy(tokens[0][1], "^2Klls");
        strcpy(tokens[0][2], "^1Dths");
        strcpy(tokens[0][3], "Sui");
        strcpy(tokens[0][4], "^3K/D");
        strcpy(tokens[0][5], "^3Effcny");

        Com_sprintf(tokens[1][0], MAX_TOKEN_LEN, "%d", s->score);
        Com_sprintf(tokens[1][1], MAX_TOKEN_LEN, "%d", s->kills);
        Com_sprintf(tokens[1][2], MAX_TOKEN_LEN, "%d", s->deaths);
        Com_sprintf(tokens[1][3], MAX_TOKEN_LEN, "%d", s->suicides);
        Com_sprintf(tokens[1][4], MAX_TOKEN_LEN, "%.1f", s->kdratio);
        Com_sprintf(tokens[1][5], MAX_TOKEN_LEN, "%.1f", s->efficiency);

        cols = 6;
    }

    // Явно устанавливаем пустую строку для третьей строки (обычно пустой)
    strcpy(tokens[2][0], " ");
    for (col = 1; col < 16; col++) {
        tokens[2][col][0] = '\0';
    }

    *outRows = 3;
    *outCols = cols;
}


qboolean CG_OSPBuildWeaponStatsTokens(char tokens[][16][64], int *rowOut) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int col;
    int row = *rowOut;
    qboolean any = qfalse;
    int i;
    const char *name;
    const weaponStats_t *ws;

    // Заголовок — явно обнуляем все токены для безопасности
    strcpy(tokens[row][0], "^5Weapon");
    strcpy(tokens[row][1], "^3Accrcy");
    strcpy(tokens[row][2], "^7Hits/Atts");
    strcpy(tokens[row][3], "^2Klls");
    strcpy(tokens[row][4], "^1Dths");
    strcpy(tokens[row][5], "^3PkUp");
    strcpy(tokens[row][6], "^1Drop");
    for (col = 7; col < 16; col++) {
        tokens[row][col][0] = '\0';
    }
    row++;

    // Разделитель
    strcpy(tokens[row][0], "^7-------------------------------------------------");
    for (col = 1; col < 16; col++) {
        tokens[row][col][0] = '\0';
    }
    row++;

    for (i = WP_GAUNTLET + 1; i < WP_NUM_WEAPONS; i++) {
        ws = &s->stats[i];

        if (ws->shots == 0 && ws->hits == 0 && ws->kills == 0 &&
            ws->deaths == 0 && ws->pickUps == 0 && ws->drops == 0) {
            continue;
        }

        name = weaponNames[i];
        strcpy(tokens[row][0], name);  // Weapon name

        // Accuracy and hits/atts
        if (ws->shots > 0 || ws->hits > 0) {
            Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^3%3.1f", ws->accuracy);
            Com_sprintf(tokens[row][2], MAX_TOKEN_LEN, "^7%d/%d", ws->hits, ws->shots);
        } else {
            strcpy(tokens[row][1], "");
            strcpy(tokens[row][2], "");
        }

        // Kills / Deaths
        Com_sprintf(tokens[row][3], MAX_TOKEN_LEN, "^2%d", ws->kills);
        Com_sprintf(tokens[row][4], MAX_TOKEN_LEN, "^1%d", ws->deaths);

        // Pickups / Drops (только для оружия после WP_SHOTGUN)
        if (i > WP_SHOTGUN) {
            Com_sprintf(tokens[row][5], MAX_TOKEN_LEN, "^3%d", ws->pickUps);
            Com_sprintf(tokens[row][6], MAX_TOKEN_LEN, "^1%d", ws->drops);
        } else {
            strcpy(tokens[row][5], "");
            strcpy(tokens[row][6], "");
        }

        // Обнулим остаток столбцов на всякий случай
        for (col = 7; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }

        any = qtrue;
        row++;
    }

    if (!any) {
        strcpy(tokens[row][0], "^3No additional weapon info available.");
        for (col = 1; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;
        strcpy(tokens[row][0], " ");
        for (col = 1; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;
    }

    *rowOut = row;
    return any;
}

void CG_OSPBuildBonusStatsTokens(char tokens[][16][64], int *rowOut, qboolean hasWeaponStats) {
    int col;
    const newStatsInfo_t *s = &cgs.be.newStats;
    int row = *rowOut;

    char armorBuf[MAX_TOKEN_LEN] = "";
    char healthBuf[MAX_TOKEN_LEN] = "";

    // Armor breakdown
    if (s->ga > 0)
        Com_sprintf(armorBuf + strlen(armorBuf), sizeof(armorBuf) - strlen(armorBuf), "^7%d ^2GA", s->ga);
    if (s->ya > 0)
        Com_sprintf(armorBuf + strlen(armorBuf), sizeof(armorBuf) - strlen(armorBuf), "%s^7%d ^3YA", armorBuf[0] ? " ^7" : "", s->ya);
    if (s->ra > 0)
        Com_sprintf(armorBuf + strlen(armorBuf), sizeof(armorBuf) - strlen(armorBuf), "%s^7%d ^1RA", armorBuf[0] ? " ^7" : "", s->ra);

    // Health breakdown
    if (s->megahealth > 0)
        Com_sprintf(healthBuf, sizeof(healthBuf), "^7%d ^5MH", s->megahealth);

    // Пустая строка
    strcpy(tokens[row][0], " ");
    for (col = 1; col < 16; col++) {
        tokens[row][col][0] = '\0';
    }
    row++;

    if (!hasWeaponStats) {
        strcpy(tokens[row][0], "^2Armor Taken:");
        Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%d", s->wins);
        if (armorBuf[0])
            Com_sprintf(tokens[row][2], MAX_TOKEN_LEN, "^2(%s^2)", armorBuf);
        else
            strcpy(tokens[row][2], "");
        for (col = 3; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;

        strcpy(tokens[row][0], "^2Health Taken:");
        Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%d", s->losses);
        if (healthBuf[0])
            Com_sprintf(tokens[row][2], MAX_TOKEN_LEN, "^2(%s^2)", healthBuf);
        else
            strcpy(tokens[row][2], "");
        for (col = 3; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;
    } else {
        // Damage Given
        strcpy(tokens[row][0], "^3Damage Given:");
        Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%d", (int)(s->damageKoeff * 100));
        strcpy(tokens[row][2], "^2Armor:");
        Com_sprintf(tokens[row][3], MAX_TOKEN_LEN, "^7%d", s->wins);
        if (armorBuf[0])
            Com_sprintf(tokens[row][4], MAX_TOKEN_LEN, "^2(%s^2)", armorBuf);
        else
            strcpy(tokens[row][4], "");
        for (col = 5; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;

        // Damage Received
        strcpy(tokens[row][0], "^3Damage Recvd:");
        Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%d", (int)(s->damageKoeff * 100 / s->damageRatio));
        strcpy(tokens[row][2], "^2Health:");
        Com_sprintf(tokens[row][3], MAX_TOKEN_LEN, "^7%d", s->losses);
        if (healthBuf[0])
            Com_sprintf(tokens[row][4], MAX_TOKEN_LEN, "^2(%s^2)", healthBuf);
        else
            strcpy(tokens[row][4], "");
        for (col = 5; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;

        if (cgs.gametype == GT_TEAM) {
            strcpy(tokens[row][0], "^1Team Damage:");
            Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%d", s->teamKills);
            for (col = 2; col < 16; col++) {
                tokens[row][col][0] = '\0';
            }
            row++;
        }

        strcpy(tokens[row][0], "^3Damage Ratio:");
        Com_sprintf(tokens[row][1], MAX_TOKEN_LEN, "^7%.2f", s->damageRatio);
        for (col = 2; col < 16; col++) {
            tokens[row][col][0] = '\0';
        }
        row++;
    }

    *rowOut = row;
}

void CG_OSPDrawStatsWindowTokens(char tokens[64][16][64], int rows, int maxColsPerRow[64]) {
	const float x = 32.0f;
	const float y = 120.0f;
	const float colSpacing = 8.0f;
	const float rowSpacing = 14.0f;
	const float padding = 8.0f;

	const float baseColWidth = 64.0f;
	const float lineHeight = rowSpacing;
	float maxWidth = 0.0f;
    float width;
    float windowHeight;
	int i, j;
    float baseX, baseY;
	vec4_t bgColor = { 0.1f, 0.1f, 0.1f, 0.75f };
	vec4_t teamColor;
    const char *text;


	Vector4Copy(scoreboard_rtColor, teamColor);
	// Вычисляем ширину окна: ширина максимального ряда по колонкам
	for (i = 0; i < rows; ++i) {
		width = 0.0f;
		for (j = 0; j < maxColsPerRow[i]; ++j)
			width += baseColWidth; // фиксированная ширина пока
		if (width > maxWidth)
			maxWidth = width;
	}

	windowHeight = rows * lineHeight + padding * 2;

	CG_FillRect(x, y, maxWidth + padding * 2, windowHeight, bgColor);
	CG_OSPDrawFrameAdjusted(x, y, maxWidth + padding * 2, windowHeight, defaultBorderSize, teamColor, qfalse);

	// Рисуем каждую ячейку
	for (i = 0; i < rows; ++i) {
		baseY = y + padding + i * rowSpacing;
		for (j = 0; j < maxColsPerRow[i]; ++j) {
			baseX = x + padding + j * baseColWidth;
			text = tokens[i][j];
			if (text[0]) {
				CG_OSPDrawStringNew(baseX, baseY, text, colorWhite, colorBlack, 8, 8, SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW, NULL, NULL, NULL);
			}
		}
	}

	wstatsWndId = 1;
}



void CG_OSPShowStatsInfoNew(void) {
    int i, c;
    int row = 0;
    int cols;
    int rowsAdded;
    int maxColsPerRow[64]; // для каждой строки — количество колонок
    qboolean hasWeapons;

    // General
    CG_OSPBuildGeneralStatsTokens(tokens, row, &rowsAdded, &cols);

    // Запоминаем количество колонок для строк, которые добавили
    maxColsPerRow[row] = cols;
    maxColsPerRow[row + 1] = cols;
    maxColsPerRow[row + 2] = 1;

    // Увеличиваем row на количество добавленных строк
    row += rowsAdded;

	// Weapon
	hasWeapons = CG_OSPBuildWeaponStatsTokens(&tokens[row], &row);
	// здесь `row` обновляется внутри

	// Посчитаем количество колонок для каждой weapon-строки
	for (i = 3; i < row; i++) {
		c = 0;
		while (c < 16 && tokens[i][c][0])
			++c;
		maxColsPerRow[i] = c ? c : 1;
	}

	// Bonus
	CG_OSPBuildBonusStatsTokens(&tokens[row], &cols, hasWeapons);
	for (i = row; i < row + cols; ++i)
		maxColsPerRow[i] = 8;
	row += cols;

	// Отрисовка
	CG_OSPDrawStatsWindowTokens(tokens, row, maxColsPerRow);
}
