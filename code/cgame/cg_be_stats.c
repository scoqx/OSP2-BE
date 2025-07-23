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


void CG_OSPBuildGeneralStats(char strings[24][128], int *rowOut) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int mins = s->flagTime / 60;
    float secs = (float)(s->flagTime % 60);

    if (cgs.gametype == GT_TOURNAMENT) {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score  ^2Klls ^1Dths Sui   ^3K/D   ^3Effcny ^2WINS ^1LOSSES");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d  %4d %4d %3d %3.1f %6.1f ^3%4d^7 %6d",
                s->score, s->kills, s->deaths, s->suicides,
                s->kdratio, s->efficiency, s->wins, s->losses);
    }
    else if (cgs.gametype == GT_TEAM && !cgs.osp.gameTypeFreeze) {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score NET  ^2Klls ^1Dths Sui ^1TmKlls   ^3K/D ^3Effcny");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d ^5%3d^7  %4d %4d %3d %6d %3.1f %4.1f",
                s->score, s->score - s->deaths, s->kills, s->deaths,
                s->suicides, s->teamKills, s->kdratio, s->efficiency);
    }
    else if (cgs.osp.gameTypeFreeze) {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score WINS  ^2Klls Thws ^1Dths Sui ^1TmKlls   ^3K/D ^3Effcny");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d ^5%4d^7  %4d %4d %4d %3d %6d %3.1f %4.1f",
                s->score, s->wins, s->kills, s->losses, s->deaths,
                s->suicides, s->teamKills, s->kdratio, s->efficiency);
    }
    else if (cgs.gametype == GT_CTF) {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score  ^2Klls ^1Dths ^3Caps   ^2Ftime Asst Dfns Rtrn KD");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d  %4d %4d ^3%4d^7 %2d:%02.1f %4d ^5%4d^7 %4d %.1f",
                s->score, s->kills, s->deaths, s->caps,
                mins, secs, s->assists, s->defences, s->returns, s->kdratio);
    }
    else if (cgs.gametype == GT_CA) {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score  ^2Klls ^1Dths   ^3K/D ^3Effcny ^5DmgScr  ^2WINS");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d  %4d %4d %3.1f %4.1f ^3%6d^5  %4d",
                s->score, s->kills, s->deaths, s->kdratio, s->efficiency,
                (int)(s->damageKoeff), s->wins);
    }
    else {
        Com_sprintf(strings[0], sizeof(strings[0]), "^B^3Score  ^2Klls ^1Dths Sui   ^3K/D  ^3Effcny");
        Com_sprintf(strings[1], sizeof(strings[1]), "%5d  %4d %4d %3d %3.1f  %4.1f",
                s->score, s->kills, s->deaths, s->suicides, s->kdratio, s->efficiency);
    }

    strcpy(strings[2], " ");
    *rowOut = 3;
}


qboolean CG_OSPBuildWeaponStats(char strings[24][128], int *rowOut) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int row = *rowOut;
    qboolean any = qfalse;
    int i;
    const char *name;
    const weaponStats_t *ws;

    Com_sprintf(strings[row++], sizeof(strings[0]), "^5Weapon       ^3Accrcy ^7Hits/Atts ^2Klls ^1Dths ^3PkUp ^1Drop");
    Com_sprintf(strings[row++], sizeof(strings[0]), "^7-------------------------------------------------");

    for (i = WP_GAUNTLET + 1; i < WP_NUM_WEAPONS; i++) {
        ws = &s->stats[i];

        if (ws->shots == 0 && ws->hits == 0 && ws->kills == 0 && ws->deaths == 0 && ws->pickUps == 0 && ws->drops == 0) {
            continue;
        }

        name = weaponNames[i];

        Com_sprintf(strings[row], sizeof(strings[0]), "%-12s: ", name);

        if (ws->shots > 0 || ws->hits > 0) {
            char buf[64];
			int len = strlen(strings[row]);

            Com_sprintf(buf, sizeof(buf), "^3%3.1f ^7%4d/%-4d ", ws->accuracy, ws->hits, ws->shots);
            Com_sprintf(strings[row] + len, sizeof(strings[0]) - len, "%s", buf);
            any = qtrue;
        } else {
            int len = strlen(strings[row]);
            Com_sprintf(strings[row] + len, sizeof(strings[0]) - len, "                ");
        }

        if (i > WP_SHOTGUN) {
            char buf[64];
			int len = strlen(strings[row]);
            Com_sprintf(buf, sizeof(buf), "^2%4d ^1%4d ^3%4d ^1%4d", ws->kills, ws->deaths, ws->pickUps, ws->drops);
            Com_sprintf(strings[row] + len, sizeof(strings[0]) - len, "%s", buf);
        } else {
            char buf[64];
			int len = strlen(strings[row]);
            Com_sprintf(buf, sizeof(buf), "^2%4d ^1%4d", ws->kills, ws->deaths);
            Com_sprintf(strings[row] + len, sizeof(strings[0]) - len, "%s", buf);
        }

        row++;
    }

    if (!any) {
        Com_sprintf(strings[row++], sizeof(strings[0]), "^3No additional weapon info available.");
        Com_sprintf(strings[row++], sizeof(strings[0]), " ");
    }

    *rowOut = row;
    return any;
}


void CG_OSPAppendBonusStats(char strings[24][128], int *rowOut, qboolean hasWeaponStats) {
    const newStatsInfo_t *s = &cgs.be.newStats;
    int row = *rowOut;

    char armorStr[64] = "";
    char healthStr[64] = "";

    // Armor breakdown
    if (s->ga > 0)
        Com_sprintf(armorStr + strlen(armorStr), sizeof(armorStr) - strlen(armorStr), "^2(^7%d ^2GA", s->ga);
    if (s->ya > 0)
        Com_sprintf(armorStr + strlen(armorStr), sizeof(armorStr) - strlen(armorStr), "%s%d ^3YA", armorStr[0] ? " ^7" : "^2(^7", s->ya);
    if (s->ra > 0)
        Com_sprintf(armorStr + strlen(armorStr), sizeof(armorStr) - strlen(armorStr), "%s%d ^1RA", armorStr[0] ? " ^7" : "^2(^7", s->ra);
    if (armorStr[0])
        strcat(armorStr, "^2)");

    // MH info
    if (s->megahealth > 0)
        Com_sprintf(healthStr, sizeof(healthStr), "^2(^7%d ^5MH^2)", s->megahealth);

    Com_sprintf(strings[row++], sizeof(strings[0]), " ");

    if (!hasWeaponStats) {
        Com_sprintf(strings[row++], sizeof(strings[0]), "^2Armor Taken : ^7%d %s", s->wins, armorStr);
        Com_sprintf(strings[row++], sizeof(strings[0]), "^2Health Taken: ^7%d %s", s->losses, healthStr);
    } else {
        Com_sprintf(strings[row++], sizeof(strings[0]), "^3Damage Given: ^7%-8d ^2Armor : ^7%d %s", (int)(s->damageKoeff * 100), s->wins, armorStr);
        Com_sprintf(strings[row++], sizeof(strings[0]), "^3Damage Recvd: ^7%-8d ^2Health: ^7%d %s", (int)(s->damageKoeff * 100 / s->damageRatio), s->losses, healthStr);

        if (cgs.gametype == GT_TEAM)
            Com_sprintf(strings[row++], sizeof(strings[0]), "^1Team Damage : ^7%d", s->teamKills);

        Com_sprintf(strings[row++], sizeof(strings[0]), "^3Damage Ratio: ^7%.2f", s->damageRatio);
    }

    *rowOut = row;
}

void CG_OSPDrawStatsWindow(char strings[24][128], int row) {
    const float x = 32.0f;
    const float y = 120.0f;
    const float width = 320.0f;
    const float lineHeight = 14.0f;
    const float height = row * lineHeight + 16.0f;
    int i;
    vec4_t bgColor = { 0.1f, 0.1f, 0.1f, 0.75f };
    vec4_t teamColor;
    int team;

    Vector4Copy(scoreboard_rtColor, teamColor);

    CG_FillRect(x, y, width, height, bgColor);

    CG_OSPDrawFrameAdjusted(x, y, width, height, defaultBorderSize, teamColor, qfalse);

    for (i = 0; i < row; ++i) {
		CG_OSPDrawStringNew(x + 8, y + 8 + i * lineHeight, strings[i], colorWhite, colorBlack, 8 , 8, SCREEN_WIDTH, DS_PROPORTIONAL | DS_SHADOW, NULL, NULL, NULL);
    }

    wstatsWndId = 1;
}



void CG_OSPShowStatsInfoNew(void) {
	char strings[24][128];
	int row = 0;
	qboolean hasWeapons;

	CG_OSPBuildGeneralStats(strings, &row);
	hasWeapons = CG_OSPBuildWeaponStats(strings, &row);
	CG_OSPAppendBonusStats(strings, &row, hasWeapons);
	CG_OSPDrawStatsWindow(strings, row);
}