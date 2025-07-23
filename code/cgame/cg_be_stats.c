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

static char tokens[32][12][64];
vec4_t tokenColors[32][12];

vec4_t colorA = {1, 0, 0, 1};
vec4_t colorB = {0, 1, 0, 1};
vec4_t colorC = {0, 0, 1, 1};
vec4_t colorD = {1, 1, 0, 1};
vec4_t colorE = {0, 1, 1, 1};
vec4_t colorF = {1, 0, 1, 1};
vec4_t colorG = {0, 0, 0, 1};

void CG_OSPInitTokenColors(const vec4_t color) {
    int row, col;
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 12; col++) {
            Vector4Copy(color, tokenColors[row][col]);
        }
    }
}



void CG_OSPSetToken(int row, int col, const char *label, const char *value, const vec4_t color) {
    Q_strncpyz(tokens[row][col], label, MAX_TOKEN_LEN);
    Q_strncpyz(tokens[row + 1][col], value, MAX_TOKEN_LEN);
    if (color) {
        Vector4Copy(color, tokenColors[row][col]);
        Vector4Copy(color, tokenColors[row + 1][col]);
    }
}

void CG_OSPSetTokenLabel(int row, int col, const char *label, const vec4_t color) {
    Q_strncpyz(tokens[row][col], label, MAX_TOKEN_LEN);
    if (color) {
        Vector4Copy(color, tokenColors[row][col]);
    }
}

void CG_OSPSetTokenValueInt(int row, int col, const char *fmt, int value, const vec4_t color) {
    char buf[MAX_TOKEN_LEN];
    Com_sprintf(buf, sizeof(buf), fmt, value);
    Q_strncpyz(tokens[row + 1][col], buf, MAX_TOKEN_LEN);
    if (color) {
        Vector4Copy(color, tokenColors[row + 1][col]);
    }
}

void CG_OSPSetTokenValueFloat(int row, int col, const char *fmt, float value, const vec4_t color) {
    char buf[MAX_TOKEN_LEN];
    Com_sprintf(buf, sizeof(buf), fmt, value);
    Q_strncpyz(tokens[row + 1][col], buf, MAX_TOKEN_LEN);
    if (color) {
        Vector4Copy(color, tokenColors[row + 1][col]);
    }
}

void CG_OSPSetWeaponTokenColored(int row, int col, const char *text, const vec4_t color) {
    Q_strncpyz(tokens[row][col], text, MAX_TOKEN_LEN);
    if (color) {
        Vector4Copy(color, tokenColors[row][col]);
    }
}



void CG_OSPSetTokenRow(char tokens[32][12][MAX_TOKEN_LEN], int row,
                       const char *labels[], const char *values[], int count) {
    int col;
    for (col = 0; col < count; col++) {
        strcpy(tokens[row][col], labels[col]);
        strcpy(tokens[row + 1][col], values[col]);
    }
}

void CG_OSPSetWeaponTokenRow(char tokens[][12][MAX_TOKEN_LEN], int row,
                              const char *values[], int count) {
    int col;
    for (col = 0; col < count; col++) {
        strcpy(tokens[row][col], values[col]);
    }
    for (col = count; col < 12; col++) {
        tokens[row][col][0] = '\0';
    }
}

void CG_OSPSetBonusTokenRow(char tokens[][12][MAX_TOKEN_LEN], int row,
                             const char *fields[], int count) {
    int col;
    for (col = 0; col < count; col++) {
        strcpy(tokens[row][col], fields[col]);
    }
    for (col = count; col < 12; col++) {
        tokens[row][col][0] = '\0';
    }
}


void CG_OSPBuildGeneralStatsTokens(char tokens[32][12][MAX_TOKEN_LEN], int startRow, int *outRows, int *outCols) {
    int i;
    int row;
    int col = 0;
    const newStatsInfo_t *s = &cgs.be.newStats;
    int mins = s->flagTime / 60;
    float secs = (float)(s->flagTime % 60);
    vec4_t c = {1 , 1, 1, 1};

    // Очистка первых трёх строк
    for (row = 0; row < 3; row++) {
        for (i = 0; i < 12; i++) {
            tokens[row][i][0] = '\0';
        }
    }


    if (cgs.gametype == GT_TOURNAMENT) {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "Kills", colorB);    CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Deaths", c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Suicides",  c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->suicides, c);
        CG_OSPSetTokenLabel(0, col, "K/D",  c);    CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
        CG_OSPSetTokenLabel(0, col, "Effiecnity", c);  CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->efficiency, c);
        CG_OSPSetTokenLabel(0, col, "WINS", c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->wins, c);
        CG_OSPSetTokenLabel(0, col, "LOSSES", c);  CG_OSPSetTokenValueInt(0, col++, "%d", s->losses, c);
    }
    else if (cgs.osp.gameTypeFreeze) {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "WINS", colorB);    CG_OSPSetTokenValueInt(0, col++, "%d", s->wins, c);
        CG_OSPSetTokenLabel(0, col, "Kills", c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Thaws", c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->losses, c);
        CG_OSPSetTokenLabel(0, col, "Deaths", c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Suicides",  c);    CG_OSPSetTokenValueInt(0, col++, "%d", s->suicides, c);
        CG_OSPSetTokenLabel(0, col, "TeamKills", c);  CG_OSPSetTokenValueInt(0, col++, "%d", s->teamKills, c);
        CG_OSPSetTokenLabel(0, col, "K/D",  c);    CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
        CG_OSPSetTokenLabel(0, col, "Effiecnity", c);  CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->efficiency, c);
    }
    else if (cgs.gametype == GT_TEAM) {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "NET",   colorB);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score - s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Kills",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Deaths",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Suicides",   c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->suicides, c);
        CG_OSPSetTokenLabel(0, col, "TeamKills",c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->teamKills, c);
        CG_OSPSetTokenLabel(0, col, "K/D",   c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
        CG_OSPSetTokenLabel(0, col, "Effiecnity",c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->efficiency, c);
    }
    else if (cgs.gametype == GT_CTF) {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "Kills",  colorB);   CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Deaths",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Caps",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->caps, c);
        {
            char timeStr[MAX_TOKEN_LEN];
            Com_sprintf(timeStr, sizeof(timeStr), "%d:%02.1f", mins, secs);
            Q_strncpyz(tokens[1][col], timeStr, MAX_TOKEN_LEN);
            CG_OSPSetTokenLabel(0, col++, "Ftime", c);
        }
        CG_OSPSetTokenLabel(0, col, "Asst",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->assists, c);
        CG_OSPSetTokenLabel(0, col, "Dfns",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->defences, c);
        CG_OSPSetTokenLabel(0, col, "Rtrn",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->returns, c);
        CG_OSPSetTokenLabel(0, col, "KD",    c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
    }
    else if (cgs.gametype == GT_CA) {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "Kills",  colorB);   CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Deaths",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "K/D",   c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
        CG_OSPSetTokenLabel(0, col, "Effiecnity",c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->efficiency, c);
        CG_OSPSetTokenLabel(0, col, "DmgScr",c);   CG_OSPSetTokenValueInt(0, col++, "%d", (int)(s->damageRatio), c);
        CG_OSPSetTokenLabel(0, col, "WINS",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->wins, c);
    }
    else {
        CG_OSPSetTokenLabel(0, col, "Score", colorA);   CG_OSPSetTokenValueInt(0, col++, "%d", s->score, c);
        CG_OSPSetTokenLabel(0, col, "Kills",  colorB);   CG_OSPSetTokenValueInt(0, col++, "%d", s->kills, c);
        CG_OSPSetTokenLabel(0, col, "Deaths",  c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->deaths, c);
        CG_OSPSetTokenLabel(0, col, "Suicides",   c);   CG_OSPSetTokenValueInt(0, col++, "%d", s->suicides, c);
        CG_OSPSetTokenLabel(0, col, "K/D",   c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->kdratio, c);
        CG_OSPSetTokenLabel(0, col, "Effiecnity",c);   CG_OSPSetTokenValueFloat(0, col++, "%.1f", s->efficiency, c);
    }

    // Очистка третьей строки
    Q_strncpyz(tokens[2][0], " ", MAX_TOKEN_LEN);
    for (i = 1; i < 12; i++) {
        tokens[2][i][0] = '\0';
    }

    *outRows = 3;
    *outCols = col;
}



qboolean CG_OSPBuildWeaponStatsTokens(char tokens[][12][64], int *rowOut) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int col, row, i;
    const char *name;
    const weaponStats_t *ws;
    char buf[64];
    qboolean any = qfalse;
    vec4_t c = {1, 1, 1, 1};

    row = *rowOut;

    // Заголовок
    CG_OSPSetWeaponTokenColored(row, 0, "Weapon", c);
    CG_OSPSetWeaponTokenColored(row, 1, "Accrcy", c);
    CG_OSPSetWeaponTokenColored(row, 2, "Hits/Atts", c);
    CG_OSPSetWeaponTokenColored(row, 3, "Kills", c);
    CG_OSPSetWeaponTokenColored(row, 4, "Deaths", c);
    CG_OSPSetWeaponTokenColored(row, 5, "PkUp", c);
    CG_OSPSetWeaponTokenColored(row, 6, "Drop", c);
    row++;

    // Разделитель
    CG_OSPSetWeaponTokenColored(row, 0, "-------------------------------------------------", c);
    for (col = 1; col < 12; col++) {
        tokens[row][col][0] = '\0';
    }
    row++;

    for (i = WP_GAUNTLET + 1; i < WP_NUM_WEAPONS; i++) {
        ws = &s->stats[i];

        if (ws->shots == 0 && ws->hits == 0 && ws->kills == 0 &&
            ws->deaths == 0 && ws->pickUps == 0 && ws->drops == 0) {
            continue;
        }

        col = 0;

        CG_OSPSetWeaponTokenColored(row, col++, weaponNames[i], c);

        if (ws->shots > 0 || ws->hits > 0) {
            Com_sprintf(buf, sizeof(buf), "%3.1f", ws->accuracy);
            CG_OSPSetWeaponTokenColored(row, col++, buf, c);

            Com_sprintf(buf, sizeof(buf), "%d/%d", ws->hits, ws->shots);
            CG_OSPSetWeaponTokenColored(row, col++, buf, c);
        } else {
            CG_OSPSetWeaponTokenColored(row, col++, "", c);
            CG_OSPSetWeaponTokenColored(row, col++, "", c);
        }

        Com_sprintf(buf, sizeof(buf), "%d", ws->kills);
        CG_OSPSetWeaponTokenColored(row, col++, buf, c);

        Com_sprintf(buf, sizeof(buf), "%d", ws->deaths);
        CG_OSPSetWeaponTokenColored(row, col++, buf, c);

        if (i > WP_SHOTGUN) {
            Com_sprintf(buf, sizeof(buf), "%d", ws->pickUps);
            CG_OSPSetWeaponTokenColored(row, col++, buf, c);

            Com_sprintf(buf, sizeof(buf), "%d", ws->drops);
            CG_OSPSetWeaponTokenColored(row, col++, buf, c);
        } else {
            CG_OSPSetWeaponTokenColored(row, col++, "", c);
            CG_OSPSetWeaponTokenColored(row, col++, "", c);
        }

        for (; col < 12; col++) {
            tokens[row][col][0] = '\0';
        }

        row++;
        any = qtrue;
    }

    if (!any) {
        CG_OSPSetWeaponTokenColored(row++, 0, "No additional weapon info available.", c);
        CG_OSPSetWeaponTokenColored(row++, 0, " ", c);
    }

    *rowOut = row;
    return any;
}


void CG_OSPBuildBonusStatsTokens(char tokens[32][12][MAX_TOKEN_LEN], int *rowOut, int hasWeaponStats) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int row = *rowOut;
    char buf[MAX_TOKEN_LEN];
    char armorText[MAX_TOKEN_LEN];
    char healthText[MAX_TOKEN_LEN];


    vec4_t c = {1, 1, 1, 1};

    // Armor buffer
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

    // Health buffer
    if (s->megahealth > 0)
        Com_sprintf(healthText, sizeof(healthText), "(%d ^5MH^7)", s->megahealth);
    else
        healthText[0] = '\0';

    // Пустая строка
    CG_OSPSetWeaponTokenColored(row++, 0, " ", c);

    if (!hasWeaponStats) {
        CG_OSPSetTokenLabel(row, 0, "Armor Taken:", c);
        CG_OSPSetTokenValueInt(row, 2, "%d", s->wins, c);
        Q_strncpyz(tokens[row + 1][3], armorText, MAX_TOKEN_LEN);
        Vector4Copy(c, tokenColors[row + 1][3]);
        row++;

        CG_OSPSetTokenLabel(row, 0, "Health Taken:", c);
        CG_OSPSetTokenValueInt(row, 2, "%d", s->losses, c);
        Q_strncpyz(tokens[row + 1][3], healthText, MAX_TOKEN_LEN);
        Vector4Copy(c, tokenColors[row + 1][3]);
        row++;
    } else {
        CG_OSPSetTokenLabel(row, 0, "Damage Given:", c);
        CG_OSPSetTokenValueInt(row, 2, "%d", (int)s->dmgGiven, c);
        CG_OSPSetTokenLabel(row, 3, "Armor:", c);
        CG_OSPSetTokenValueInt(row, 4, "%d", s->wins, c);
        Q_strncpyz(tokens[row + 1][5], armorText, MAX_TOKEN_LEN);
        Vector4Copy(c, tokenColors[row + 1][5]);
        row++;

        CG_OSPSetTokenLabel(row, 0, "Damage Recvd:", c);
        CG_OSPSetTokenValueInt(row, 2, "%d", (int)s->dmgReceived, c);
        CG_OSPSetTokenLabel(row, 3, "Health:", c);
        CG_OSPSetTokenValueInt(row, 4, "%d", s->losses, c);
        Q_strncpyz(tokens[row + 1][5], healthText, MAX_TOKEN_LEN);
        Vector4Copy(c, tokenColors[row + 1][5]);
        row++;

        if (cgs.gametype == GT_TEAM) {
            CG_OSPSetTokenLabel(row, 0, "Team Damage:", c);
            CG_OSPSetTokenValueInt(row, 2, "%d", s->teamKills, c);
            row++;
        }

        CG_OSPSetTokenLabel(row, 0, "Damage Ratio:", c);
        CG_OSPSetTokenValueFloat(row, 2, "%.2f", s->damageRatio, c);
        row++;
    }

    *rowOut = row;
}



void CG_OSPDrawStatsWindowTokens(char tokens[32][12][MAX_TOKEN_LEN], int rows, int maxColsPerRow[12]) {
    const float x = 4.0f;
    const float y = 340.0f;
    const float colSpacing = 4.0f;
    const float rowSpacing = 8.0f;
    const float padding = 6.0f;
    const vec2_t fontSize = { 6, 8 };
    const float baseColWidth = 36.0f;
    const float lineHeight = rowSpacing;
    float maxWidth = 0.0f;
    float width;
    float windowHeight;
    float widthCutoff = 0.0f;
    int i, j;
    float baseX, baseY;
    vec4_t bgColor = { 0.1f, 0.1f, 0.1f, 0.75f };
    vec4_t teamColor;
    const char *text;
    vec4_t defaultColor = {1, 1, 1, 1};
    Vector4Copy(scoreboard_rtColor, teamColor);

    for (i = 0; i < rows; ++i) {
        width = maxColsPerRow[i] * baseColWidth;
        if (maxColsPerRow[i] > 1)
            width += (maxColsPerRow[i] - 1) * colSpacing - widthCutoff;
        if (width > maxWidth)
            maxWidth = width;
    }


    windowHeight = rows * lineHeight + padding * 2;
    CG_FontSelect(4);
    CG_FillRect(x, y, maxWidth + padding * 2, windowHeight, bgColor);
    CG_OSPDrawFrameAdjusted(x, y, maxWidth + padding * 2, windowHeight, defaultBorderSize, teamColor, 0);

    for (i = 0; i < rows; ++i) {
    baseY = y + padding + i * rowSpacing;
    for (j = 0; j < maxColsPerRow[i]; ++j) {
        baseX = x + padding + j * (baseColWidth + colSpacing);
        text = tokens[i][j];

        if (text[0]) {
            CG_OSPDrawStringNew(baseX, baseY, text,
            tokenColors[i][j], colorBlack,
            fontSize[0], fontSize[1],
            SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW,
            NULL, NULL, NULL);
        }
    }
}
}

void CG_OSPShowStatsInfoNew(void) {
    int i, c;
    int row = 0;
    int cols;
    int rowsAdded;
    int maxColsPerRow[64]; // для каждой строки — количество колонок
    qboolean hasWeapons;
    int bonusStartRow, bonusRowsCount;

    // General
    CG_OSPInitTokenColors(colorWhite);
    CG_OSPBuildGeneralStatsTokens(tokens, row, &rowsAdded, &cols);
    for (i = row; i < row + rowsAdded; i++) {
        maxColsPerRow[i] = cols;
    }
    row += rowsAdded;

    // Weapon
    hasWeapons = CG_OSPBuildWeaponStatsTokens(tokens, &row);

    for (i = 3; i < row; i++) {
        c = 0;
        while (c < 12 && tokens[i][c][0])
            ++c;
        maxColsPerRow[i] = c ? c : 1;
    }

    // Bonus
    bonusStartRow = row;
    CG_OSPBuildBonusStatsTokens(tokens, &row, hasWeapons);
    bonusRowsCount = row - bonusStartRow;

    for (i = bonusStartRow; i < row; ++i) {
        maxColsPerRow[i] = 8;
    }

    CG_OSPDrawStatsWindowTokens(tokens, row, maxColsPerRow);
}
