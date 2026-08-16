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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS    16
#define MAX_LOADING_ITEM_ICONS      32

#define LOADING_PLAYER_ICON_SIZE    48.0f
#define LOADING_ITEM_ICON_SIZE      24.0f
#define LOADING_ICON_MARGIN_X       16.0f
#define LOADING_ICON_MARGIN_BOTTOM  12.0f
#define LOADING_ICON_ROW_GAP        6.0f
#define LOADING_ICON_GAP            2.0f
#define LOADING_ITEM_ICON_STEP      (LOADING_ITEM_ICON_SIZE + LOADING_ICON_GAP)
#define LOADING_PLAYER_ICON_STEP    (LOADING_PLAYER_ICON_SIZE + LOADING_ICON_GAP)

#define LOADING_INFO_FONT_SCALE     0.8f
#define LOADING_INFO_LINE_HEIGHT    ((int)(PROP_HEIGHT * LOADING_INFO_FONT_SCALE + 0.5f))

#define LOADING_STATUS_Y            (128 - 32)

#define LOADING_PROGRESS_COLLISION  0.05f
#define LOADING_PROGRESS_SOUNDS     0.10f
#define LOADING_PROGRESS_GRAPHICS   0.12f
#define LOADING_PROGRESS_MAPNAME    0.14f
#define LOADING_PROGRESS_GAME_MEDIA 0.16f
#define LOADING_PROGRESS_ITEMS_END  0.78f
#define LOADING_PROGRESS_EMOJI      0.88f
#define LOADING_PROGRESS_CLIENTS    0.80f
#define LOADING_PROGRESS_CLIENTS_END 0.96f
#define LOADING_PROGRESS_DONE       1.0f

typedef enum
{
	LOADING_PROGRESS_PHASE_NONE = 0,
	LOADING_PROGRESS_PHASE_ITEMS,
	LOADING_PROGRESS_PHASE_CLIENTS
} loadingProgressPhase_t;

static int          loadingPlayerIconCount;
static int          loadingItemIconCount;
static qhandle_t    loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t    loadingItemIcons[MAX_LOADING_ITEM_ICONS];

static float                    loadingBarFraction;
static loadingProgressPhase_t   loadingProgressPhase;
static int                      loadingItemsExpected;
static int                      loadingItemsReported;
static int                      loadingClientsExpected;
static int                      loadingClientsReported;


static void CG_DrawLoadingPicSquare(float x, float y, float w, float h, qhandle_t shader)
{
	float   ax;
	float   ay;
	float   aw;
	float   ah;

	ax = x * cgs.screenXScale + cgs.screenXBias;
	ay = y * cgs.screenYScale + cgs.screenYBias;
	aw = w * cgs.screenXScale;
	ah = h * cgs.screenYScale;
	trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, shader);
}

static float CG_LoadingIconBaseX(void)
{
	return cgs.screenXmin + LOADING_ICON_MARGIN_X;
}

static int CG_LoadingItemIconsPerRow(void)
{
	int     perRow;
	float   rowWidth;

	rowWidth = (cgs.screenXmax - cgs.screenXmin) - LOADING_ICON_MARGIN_X * 2.0f - LOADING_ITEM_ICON_SIZE;
	perRow = (int)(rowWidth / LOADING_ITEM_ICON_STEP) + 1;
	if (perRow < 1)
	{
		perRow = 1;
	}
	return perRow;
}

static void CG_LoadingProgressSet(float fraction)
{
	if (fraction > loadingBarFraction)
	{
		loadingBarFraction = fraction;
	}
}

static int CG_LoadingCountMapItems(void)
{
	char    items[MAX_ITEMS + 1];
	int     i;
	int     count;

	count = 0;
	strcpy(items, CG_ConfigString(CS_ITEMS));
	for (i = 1; i < bg_numItems; i++)
	{
		if (items[i] == '1' || cg_buildScript.integer)
		{
			count++;
		}
	}
	if (count < 1)
	{
		count = 1;
	}
	return count;
}

static int CG_LoadingCountActiveClients(void)
{
	int     i;
	int     count;

	count = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (CG_ConfigString(CS_PLAYERS + i)[0])
		{
			count++;
		}
	}
	if (count < 1)
	{
		count = 1;
	}
	return count;
}

static void CG_DrawLoadingProgressBarPic(float x, float y, float w, float h, float texS2,
        const vec4_t color, qhandle_t shader)
{
	if (!shader || w <= 0.0f || h <= 0.0f)
	{
		return;
	}

	trap_R_SetColor(color);
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, texS2, 1.0f, shader);
	trap_R_SetColor(NULL);
}

static void CG_DrawLoadingProgressBar(void)
{
	vec4_t  barColor;
	vec4_t  shadowColor;
	float   x;
	float   y;
	float   w;
	float   h;
	float   fillW;
	float   fraction;

	if (loadingBarFraction <= 0.0f)
	{
		return;
	}

	if (!cgs.media.loadingBarBorder1Shader || !cgs.media.loadingBarFill1Shader)
	{
		return;
	}

	w = 240.0f;
	h = 12.0f;
	x = (640.0f - w) * 0.5f;
	if (Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "sv_cheats")[0] == '1')
	{
		y = LOADING_STATUS_Y + 68.0f;
	}
	else
	{
		y = LOADING_STATUS_Y + 76.0f;
	}

	fraction = loadingBarFraction;
	if (fraction > 1.0f)
	{
		fraction = 1.0f;
	}
	fillW = w * fraction;

	/* Same offset as UI_DROPSHADOW in UI_DrawProportionalString (+2, +2). */
	Vector4Set(shadowColor, 0.0f, 0.0f, 0.0f, 1.0f);
	CG_DrawLoadingProgressBarPic(x + 2.0f, y + 2.0f, w, h, 1.0f, shadowColor,
	                             cgs.media.loadingBarBorder1Shader);
	if (fillW > 0.0f)
	{
		shadowColor[3] = 1.0f;
		CG_DrawLoadingProgressBarPic(x + 2.0f, y + 2.0f, fillW, h, fraction, shadowColor,
		                             cgs.media.loadingBarFill1Shader);
	}

	Vector4Set(barColor, 1.0f, 1.0f, 1.0f, 1.0f);
	CG_DrawLoadingProgressBarPic(x, y, w, h, 1.0f, barColor, cgs.media.loadingBarBorder1Shader);
	if (fillW > 0.0f)
	{
		CG_DrawLoadingProgressBarPic(x, y, fillW, h, fraction, barColor, cgs.media.loadingBarFill1Shader);
	}
}

/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons(void)
{
	int     n;
	float   x;
	float   bottomItemY;
	float   playerY;

	bottomItemY = 480.0f - LOADING_ICON_MARGIN_BOTTOM - LOADING_ITEM_ICON_SIZE;

	for (n = 0; n < loadingItemIconCount; n++)
	{
		x = CG_LoadingIconBaseX() + (float)n * LOADING_ITEM_ICON_STEP;
		CG_DrawLoadingPicSquare(x, bottomItemY, LOADING_ITEM_ICON_SIZE, LOADING_ITEM_ICON_SIZE, loadingItemIcons[n]);
	}

	if (loadingItemIconCount > 0)
	{
		playerY = bottomItemY - LOADING_ICON_ROW_GAP - LOADING_PLAYER_ICON_SIZE;
	}
	else
	{
		playerY = 480.0f - LOADING_ICON_MARGIN_BOTTOM - LOADING_PLAYER_ICON_SIZE;
	}

	for (n = 0; n < loadingPlayerIconCount; n++)
	{
		x = CG_LoadingIconBaseX() + (float)n * LOADING_PLAYER_ICON_STEP;
		CG_DrawLoadingPicSquare(x, playerY, LOADING_PLAYER_ICON_SIZE, LOADING_PLAYER_ICON_SIZE, loadingPlayerIcons[n]);
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString(const char* s)
{
	Q_strncpyz(cg.infoScreenText, s, sizeof(cg.infoScreenText));

	if (!s[0])
	{
		CG_LoadingProgressSet(LOADING_PROGRESS_DONE);
	}
	else if (!Q_stricmp(s, "collision map"))
	{
		loadingProgressPhase = LOADING_PROGRESS_PHASE_NONE;
		loadingPlayerIconCount = 0;
		loadingItemIconCount = 0;
		CG_LoadingProgressSet(LOADING_PROGRESS_COLLISION);
	}
	else if (!Q_stricmp(s, "sounds"))
	{
		CG_LoadingProgressSet(LOADING_PROGRESS_SOUNDS);
	}
	else if (!Q_stricmp(s, "graphics"))
	{
		loadingProgressPhase = LOADING_PROGRESS_PHASE_NONE;
		CG_LoadingProgressSet(LOADING_PROGRESS_GRAPHICS);
	}
	else if (cgs.mapname[0] && !Q_stricmp(s, cgs.mapname))
	{
		CG_LoadingProgressSet(LOADING_PROGRESS_MAPNAME);
	}
	else if (!Q_stricmp(s, "game media"))
	{
		loadingProgressPhase = LOADING_PROGRESS_PHASE_ITEMS;
		loadingItemsReported = 0;
		loadingItemsExpected = CG_LoadingCountMapItems();
		CG_LoadingProgressSet(LOADING_PROGRESS_GAME_MEDIA);
	}
	else if (!Q_stricmp(s, "emoji"))
	{
		loadingProgressPhase = LOADING_PROGRESS_PHASE_NONE;
		CG_LoadingProgressSet(LOADING_PROGRESS_EMOJI);
	}
	else if (!Q_stricmp(s, "clients"))
	{
		loadingProgressPhase = LOADING_PROGRESS_PHASE_CLIENTS;
		loadingClientsReported = 0;
		loadingClientsExpected = CG_LoadingCountActiveClients();
		if (loadingBarFraction < LOADING_PROGRESS_CLIENTS)
		{
			CG_LoadingProgressSet(LOADING_PROGRESS_CLIENTS);
		}
	}
	else if (loadingProgressPhase == LOADING_PROGRESS_PHASE_ITEMS)
	{
		loadingItemsReported++;
		if (loadingItemsReported > loadingItemsExpected)
		{
			loadingItemsReported = loadingItemsExpected;
		}
		CG_LoadingProgressSet(LOADING_PROGRESS_GAME_MEDIA +
		                      (LOADING_PROGRESS_ITEMS_END - LOADING_PROGRESS_GAME_MEDIA) *
		                      (float)loadingItemsReported / (float)loadingItemsExpected);
	}
	else if (loadingProgressPhase == LOADING_PROGRESS_PHASE_CLIENTS)
	{
		float   start;

		loadingClientsReported++;
		if (loadingClientsReported > loadingClientsExpected)
		{
			loadingClientsReported = loadingClientsExpected;
		}
		start = loadingBarFraction;
		if (start < LOADING_PROGRESS_CLIENTS)
		{
			start = LOADING_PROGRESS_CLIENTS;
		}
		CG_LoadingProgressSet(start +
		                      (LOADING_PROGRESS_CLIENTS_END - start) *
		                      (float)loadingClientsReported / (float)loadingClientsExpected);
	}

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem(int itemNum)
{
	gitem_t*     item;

	item = &bg_itemlist[itemNum];

	if (item->icon && loadingItemIconCount < CG_LoadingItemIconsPerRow())
	{
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip(item->icon);
	}

	CG_LoadingString(item->pickup_name);
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient(int clientNum)
{
	const char*      info;
	char*            skin;
	char            personality[MAX_QPATH];
	char            model[MAX_QPATH];
	char            iconName[MAX_QPATH];

	info = CG_ConfigString(CS_PLAYERS + clientNum);

	if (loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS)
	{
		Q_strncpyz(model, Info_ValueForKey(info, "model"), sizeof(model));
		skin = Q_strrchr(model, '/');
		if (skin)
		{
			*skin++ = '\0';
		}
		else
		{
			skin = "default";
		}

		Com_sprintf(iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin);

		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		if (!loadingPlayerIcons[loadingPlayerIconCount])
		{
			Com_sprintf(iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin);
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		}
		if (!loadingPlayerIcons[loadingPlayerIconCount])
		{
			Com_sprintf(iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default");
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip(iconName);
		}
		if (loadingPlayerIcons[loadingPlayerIconCount])
		{
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz(personality, Info_ValueForKey(info, "n"), sizeof(personality));
	Q_CleanStr(personality);

	if (cgs.gametype == GT_SINGLE_PLAYER)
	{
		trap_S_RegisterSound(va("sound/player/announce/%s.wav", personality), qtrue);
	}

	CG_LoadingString(personality);
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation(void)
{
	const char*  s;
	const char*  info;
	const char*  sysInfo;
	int         y;
	int         value;
	qhandle_t   levelshot;
	char        buf[1024];
	char        versionStr[64];
	const char* versionPtr;

	info = CG_ConfigString(CS_SERVERINFO);
	sysInfo = CG_ConfigString(CS_SYSTEMINFO);

	s = Info_ValueForKey(info, "mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("levelshots/%s.tga", s));
	if (!levelshot)
	{
		levelshot = trap_R_RegisterShaderNoMip("menu/art/unknownmap");
	}
	trap_R_SetColor(NULL);
	CG_DrawPicOld(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot);

	// blend a detail texture over it (be_infoDetail: 0 off, 1 tiled, 2 fullscreen)
	if (be_infoDetail.integer != 0)
	{
		qhandle_t detail;
		detail = trap_R_RegisterShader("levelShotDetail");
		if (be_infoDetail.integer == 2)
			trap_R_DrawStretchPic(0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 1, 1, detail);
		else
			trap_R_DrawStretchPic(0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail);
	}

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if (cg.infoScreenText[0])
	{
		UI_DrawProportionalString(320, LOADING_STATUS_Y, va("Loading... %s", cg.infoScreenText),
		                          UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
	}
	else
	{
		UI_DrawProportionalString(320, LOADING_STATUS_Y, "Awaiting snapshot...",
		                          UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
	}

	// draw info string information

	y = 180 - 32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if (!atoi(buf))
	{
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 1024);
		Q_CleanStr(buf);
		UI_DrawProportionalString(320, y, buf,
		                          UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if (s[0] == '1')
		{
			UI_DrawProportionalString(320, y, "Pure Server",
			                          UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString(CS_MOTD);
		if (s[0])
		{
			UI_DrawProportionalString(320, y, s,
			                          UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	y -= (PROP_HEIGHT - LOADING_INFO_LINE_HEIGHT) + 6;
	s = CG_ConfigString(CS_MESSAGE);
	if (s[0])
	{
		UI_DrawProportionalStringScaled(320, y, s,
		                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
		                                LOADING_INFO_FONT_SCALE);
		y += LOADING_INFO_LINE_HEIGHT;
	}

	// cheats warning
	s = Info_ValueForKey(sysInfo, "sv_cheats");
	if (s[0] == '1')
	{
		UI_DrawProportionalStringScaled(320, y, "CHEATS ARE ENABLED",
		                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
		                                LOADING_INFO_FONT_SCALE);
		y += LOADING_INFO_LINE_HEIGHT;
	}

	// game type
	switch (cgs.gametype)
	{
		case GT_FFA:
			s = "Free For All";
			break;
		case GT_SINGLE_PLAYER:
			s = "Single Player";
			break;
		case GT_TOURNAMENT:
			s = "Tournament";
			break;
		case GT_TEAM:
			s = "Team Deathmatch";
			break;
		case GT_CTF:
			s = "Capture The Flag";
			break;
		case GT_CA:
			s = "CLAN ARENA";
			break;
		default:
			s = "Unknown Gametype";
			break;
	}
	UI_DrawProportionalStringScaled(320, y, s,
	                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
	                                LOADING_INFO_FONT_SCALE);
	y += LOADING_INFO_LINE_HEIGHT;

	value = atoi(Info_ValueForKey(info, "timelimit"));
	if (value)
	{
		UI_DrawProportionalStringScaled(320, y, va("timelimit %i", value),
		                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
		                                LOADING_INFO_FONT_SCALE);
		y += LOADING_INFO_LINE_HEIGHT;
	}

	if (cgs.gametype < GT_CTF)
	{
		value = atoi(Info_ValueForKey(info, "fraglimit"));
		if (value)
		{
			UI_DrawProportionalStringScaled(320, y, va("fraglimit %i", value),
			                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
			                                LOADING_INFO_FONT_SCALE);
			y += LOADING_INFO_LINE_HEIGHT;
		}
	}

	if (cgs.gametype >= GT_CTF)
	{
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if (value)
		{
			UI_DrawProportionalStringScaled(320, y, va("capturelimit %i", value),
			                                UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite,
			                                LOADING_INFO_FONT_SCALE);
			y += LOADING_INFO_LINE_HEIGHT;
		}
	}

	// Draw OSP2-BE version in bottom right corner
	versionPtr = OSP_VERSION;
	if (Q_strncmp(versionPtr, "be-", 3) == 0)
	{
		versionPtr += 3; // Skip "be-" prefix
	}
	Com_sprintf(versionStr, sizeof(versionStr), "OSP2-BE(%s)", versionPtr);
	CG_OSPDrawStringNew(640.0f, 467.0f, versionStr,
	                    colorWhite, colorBlack,
	                    8.0f, 12.0f,
	                    SCREEN_WIDTH,
	                    DS_HRIGHT | DS_PROPORTIONAL | DS_SHADOW,
	                    cgFontIndices.infoVersion, NULL, NULL, NULL);

	CG_DrawLoadingProgressBar();
}
