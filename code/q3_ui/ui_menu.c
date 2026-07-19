// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER         10
#define ID_MULTIPLAYER          11
#define ID_SETUP                12
#define ID_DEMOS                13
#define ID_TEAMARENA            14
#define ID_CINEMATICS           15
#define ID_MODS                 16
#define ID_EXIT                 17

#define MAIN_BANNER_MODEL               "models/mapobjects/banner/banner5.md3"
#define MAIN_BANNER_MODEL_NEW           "models/mapobjects/banner/bannerNew.md3"
#define MAIN_MENU_VERTICAL_SPACING      34
#define MAIN_MENU_FOOTER_LABEL          "OSP2-BE (Black Edition), 2025-2026, diwoc"


typedef struct
{
	menuframework_s menu;

	menutext_s      singleplayer;
	menutext_s      multiplayer;
	menutext_s      setup;
	menutext_s      demos;
	menutext_s      teamArena;
	menutext_s      cinematics;
	menutext_s      mods;
	menutext_s      exit;

	qhandle_t       bannerModel;     /* standard banner (ui_banner == 1) */
	qhandle_t       bannerModelNew;  /* new banner (ui_banner == 2) */
} mainmenu_t;


static mainmenu_t s_main;
static float s_mainMenuLastFavShift = -1.0f;
static qboolean s_mainMenuTeamArena;

typedef struct
{
	menuframework_s menu;
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
Main_MenuSetPTextY
=================
*/
static void Main_MenuSetPTextY(menutext_s* t, int y)
{
	float   sizeScale;
	int     x;
	int     w;
	int     h;

	sizeScale = UI_ProportionalSizeScale(t->style);
	x = t->generic.x;
	w = UI_ProportionalStringWidth(t->string) * sizeScale;
	h = PROP_HEIGHT * sizeScale;

	t->generic.y = y;
	t->generic.top = y;
	t->generic.bottom = y + h;

	if (t->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x -= w / 2;
	}

	t->generic.left = x - PROP_GAP_WIDTH * sizeScale;
	t->generic.right = x + w + PROP_GAP_WIDTH * sizeScale;
}

/*
=================
Main_MenuUpdateFavLayout
=================
*/
static void Main_MenuUpdateFavLayout(float shift)
{
	int     y;
	int     spacing;

	y = 134 + (int)shift;
	spacing = MAIN_MENU_VERTICAL_SPACING;

	Main_MenuSetPTextY(&s_main.singleplayer, y);
	y += spacing;
	Main_MenuSetPTextY(&s_main.multiplayer, y);
	y += spacing;
	Main_MenuSetPTextY(&s_main.setup, y);
	y += spacing;
	Main_MenuSetPTextY(&s_main.demos, y);
	y += spacing;
	if (s_mainMenuTeamArena)
	{
		Main_MenuSetPTextY(&s_main.teamArena, y);
		y += spacing;
	}
	Main_MenuSetPTextY(&s_main.cinematics, y);
	y += spacing;
	Main_MenuSetPTextY(&s_main.mods, y);
	y += spacing;
	Main_MenuSetPTextY(&s_main.exit, y);
}

/*
=================
Main_MenuKey
=================
*/
static sfxHandle_t Main_MenuKey(int key)
{
	int textY;
	int textHeight;
	int textWidth;
	int clickX;
	int clickY;

	if (UI_ServerStatus_HandleModalKey(key))
	{
		if (key == K_ESCAPE)
		{
			return menu_out_sound;
		}

		if (key == K_MOUSE1)
		{
			return menu_move_sound;
		}
	}

	if (UI_ServerStatus_IsModal())
	{
		return 0;
	}

	if (UI_FavouriteServer_HandleKey(key))
	{
		if (key == K_ESCAPE)
		{
			return menu_out_sound;
		}

		if (key == K_MOUSE1)
		{
			return menu_move_sound;
		}
	}

	if (UI_FavouriteServer_IsModal())
	{
		return 0;
	}

	if (key == K_MOUSE1)
	{
		if (UI_ServerStatus_HandleBarClick(key))
		{
			return menu_move_sound;
		}

		clickX = uis.cursorx;
		clickY = uis.cursory;

		if (uis.demoversion)
		{
			textY = 380;
		}
		else
		{
			textY = 415;
		}

		textHeight = SMALLCHAR_HEIGHT;
		textWidth = strlen(MAIN_MENU_FOOTER_LABEL) * SMALLCHAR_WIDTH;

		if (clickY >= textY && clickY <= textY + textHeight &&
		        clickX >= 320 - textWidth / 2 && clickX <= 320 + textWidth / 2)
		{
			UI_CreditMenu2();
			return menu_move_sound;
		}
	}

	return Menu_DefaultKey(&s_main.menu, key);
}

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction(qboolean result)
{
	if (!result)
	{
		return;
	}
	UI_PopMenu();
	trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
	{
		return;
	}

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_SINGLEPLAYER:
			UI_SPLevelMenu();
			break;

		case ID_MULTIPLAYER:
			UI_ArenaServersMenu();
			break;

		case ID_SETUP:
			UI_SetupMenu();
			break;

		case ID_DEMOS:
			UI_DemosMenu();
			break;

		case ID_CINEMATICS:
			UI_CinematicsMenu();
			break;

		case ID_MODS:
			UI_ModsMenu();
			break;

		case ID_TEAMARENA:
			trap_Cvar_Set("fs_game", "missionpack");
			trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart;");
			break;

		case ID_EXIT:
			UI_ConfirmMenu("EXIT GAME?", NULL, MainMenu_ExitAction);
			break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache(void)
{
	s_main.bannerModel = trap_R_RegisterModel(MAIN_BANNER_MODEL);
	s_main.bannerModelNew = trap_R_RegisterModel(MAIN_BANNER_MODEL_NEW);
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set("com_errorMessage", "");
	UI_MainMenu();
	return (menu_null_sound);
}

static void UI_DrawErrorScreenInfoVersion(void)
{
	char            versionStr[64];
	const char*     versionPtr;
	int             fontIndex;
	float           charW;
	float           charH;

	if (!UI_FontLoaded(2))
	{
		UI_LoadFonts();
	}

	fontIndex = UI_FontIndexFromCvar(&be_infoVersionFont);
	if (!UI_FontAvailable(fontIndex))
	{
		fontIndex = 2;
	}

	if (UI_FontLoaded(fontIndex))
	{
		UI_FontSelect(fontIndex);
	}

	versionPtr = OSP_VERSION;
	if (!Q_strncmp(versionPtr, "be-", 3))
	{
		versionPtr += 3;
	}

	/* Match cgame CG_OSPDrawStringNew + CG_AdjustFrom640 (separate X/Y scale), not UI letterbox uniform scale */
	charW = 8.0f;
	charH = 12.0f;
	if (uis.scale > 0.0f)
	{
		charW = 8.0f * ((float)uis.glconfig.vidWidth / 640.0f) / uis.scale;
		charH = 12.0f * ((float)uis.glconfig.vidHeight / 480.0f) / uis.scale;
	}

	Com_sprintf(versionStr, sizeof(versionStr), "OSP2-BE(%s)", versionPtr);
	UI_OSPDrawStringNew(uis.screenXmax, 467.0f, versionStr,
	                    color_white, color_black,
	                    charW, charH,
	                    (int)(uis.screenXmax - uis.screenXmin),
	                    DS_HRIGHT | DS_PROPORTIONAL | DS_SHADOW,
	                    NULL, NULL, NULL);
}

static void Main_MenuDrawBannerScene(void)
{
	refdef_t        refdef;
	refEntity_t     ent;
	vec3_t          origin;
	vec3_t          angles;
	float           adjust;
	float           x, y, w, h;

	if (!ui_banner.integer)
	{
		return;
	}

	memset(&refdef, 0, sizeof(refdef));

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640(&x, &y, &w, &h);
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0;
	refdef.fov_x = 60 + adjust;
	refdef.fov_y = 19.6875 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	memset(&ent, 0, sizeof(ent));

	adjust = 5.0 * sin((float)uis.realtime / 5000);
	VectorSet(angles, 0, 180 + adjust, 0);
	AnglesToAxis(angles, ent.axis);
	ent.hModel = (ui_banner.integer == 2) ? s_main.bannerModelNew : s_main.bannerModel;
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.lightingOrigin);
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	if (ui_banner.integer == 2)
	{
		ent.shaderRGBA[0] = uis.bannerColor[0] * 255;
		ent.shaderRGBA[1] = uis.bannerColor[1] * 255;
		ent.shaderRGBA[2] = uis.bannerColor[2] * 255;
		ent.shaderRGBA[3] = uis.bannerColor[3] * 255;
	}

	VectorCopy(ent.origin, ent.oldorigin);

	trap_R_AddRefEntityToScene(&ent);

	trap_R_RenderScene(&refdef);
}

static void Main_MenuDrawFooter(void)
{
	vec4_t          color;
	vec4_t          colorBE = {0.25f, 0.25f, 0.25f, 0.1f};

	Vector4Copy(uis.themeColor, color);

	if (uis.demoversion)
	{
		UI_DrawProportionalString(320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER | UI_SMALLFONT, color);
		UI_DrawString(320, 380, MAIN_MENU_FOOTER_LABEL, UI_CENTER | UI_SMALLFONT | UI_PULSE_2, colorBE);
		UI_DrawString(320, 405, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER | UI_SMALLFONT, color);
	}
	else
	{
		UI_DrawString(320, 415, MAIN_MENU_FOOTER_LABEL, UI_CENTER | UI_SMALLFONT | UI_PULSE_2, colorBE);
		UI_DrawString(320, 440, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER | UI_SMALLFONT, color);
	}
}

static void ErrorMessage_MenuDraw(void)
{
	Main_MenuDrawBannerScene();

	UI_DrawProportionalString_AutoWrapped(320, 192, 600, 20, s_errorMessage.errorMessage,
	                                      UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, menu_text_color);
	UI_DrawErrorScreenInfoVersion();

	Main_MenuDrawFooter();
}

/*
===============
Main_MenuDraw
===============
*/

static void Main_MenuDraw(void)
{
	float   shift;

	UI_FavouriteServer_Tick();
	UI_FavouriteServer_OnMainMenuDraw();

	shift = UI_FavouriteServer_GetMenuShift();
	if (shift != s_mainMenuLastFavShift)
	{
		Main_MenuUpdateFavLayout(shift);
		s_mainMenuLastFavShift = shift;
	}

	Main_MenuDrawBannerScene();

	Menu_Draw(&s_main.menu);

	Main_MenuDrawFooter();

	UI_FavouriteServer_Draw();

	UI_ServerStatus_DrawBar();
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists(void)
{
	int     numdirs;
	char    dirlist[2048];
	char*    dirptr;
	char*  descptr;
	int     i;
	int     dirlen;

	numdirs = trap_FS_GetFileList("$modlist", "", dirlist, sizeof(dirlist));
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++)
	{
		dirlen = strlen(dirptr) + 1;
		descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0)
		{
			return qtrue;
		}
		dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu(void)
{
	int     y;
	qboolean teamArena = qfalse;
	int     style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set("sv_killserver", "1");

	if (!uis.demoversion && !ui_cdkeychecked.integer)
	{
		char    key[17];

		trap_GetCDKey(key, sizeof(key));
		if (trap_VerifyCDKey(key, NULL) == qfalse)
		{
			UI_CDKeyMenu();
			return;
		}
	}

	memset(&s_main, 0, sizeof(mainmenu_t));
	memset(&s_errorMessage, 0, sizeof(errorMessage_t));

	// com_errorMessage would need that too
	MainMenu_Cache();

	trap_Cvar_VariableStringBuffer("com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage));
	if (s_errorMessage.errorMessage[0])
	{
		s_errorMessage.menu.draw = ErrorMessage_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;

		trap_Key_SetCatcher(KEYCATCH_UI);
		uis.menusp = 0;
		UI_PushMenu(&s_errorMessage.menu);

		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.key = Main_MenuKey;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	s_mainMenuLastFavShift = -1.0f;

	y = 134;
	s_main.singleplayer.generic.type        = MTYPE_PTEXT;
	s_main.singleplayer.generic.flags       = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x           = 320;
	s_main.singleplayer.generic.y           = y;
	s_main.singleplayer.generic.id          = ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback    = Main_MenuEvent;
	s_main.singleplayer.string              = "SINGLE PLAYER";
	s_main.singleplayer.color               = uis.themeColor;
	s_main.singleplayer.style               = style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type         = MTYPE_PTEXT;
	s_main.multiplayer.generic.flags        = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x            = 320;
	s_main.multiplayer.generic.y            = y;
	s_main.multiplayer.generic.id           = ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback     = Main_MenuEvent;
	s_main.multiplayer.string               = "MULTIPLAYER";
	s_main.multiplayer.color                = uis.themeColor;
	s_main.multiplayer.style                = style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type               = MTYPE_PTEXT;
	s_main.setup.generic.flags              = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.setup.generic.x                  = 320;
	s_main.setup.generic.y                  = y;
	s_main.setup.generic.id                 = ID_SETUP;
	s_main.setup.generic.callback           = Main_MenuEvent;
	s_main.setup.string                     = "SETUP";
	s_main.setup.color                      = uis.themeColor;
	s_main.setup.style                      = style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type               = MTYPE_PTEXT;
	s_main.demos.generic.flags              = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.demos.generic.x                  = 320;
	s_main.demos.generic.y                  = y;
	s_main.demos.generic.id                 = ID_DEMOS;
	s_main.demos.generic.callback           = Main_MenuEvent;
	s_main.demos.string                     = "DEMOS";
	s_main.demos.color                      = uis.themeColor;
	s_main.demos.style                      = style;

	if (UI_TeamArenaExists())
	{
		teamArena = qtrue;
		y += MAIN_MENU_VERTICAL_SPACING;
		s_main.teamArena.generic.type           = MTYPE_PTEXT;
		s_main.teamArena.generic.flags          = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
		s_main.teamArena.generic.x              = 320;
		s_main.teamArena.generic.y              = y;
		s_main.teamArena.generic.id             = ID_TEAMARENA;
		s_main.teamArena.generic.callback       = Main_MenuEvent;
		s_main.teamArena.string                 = "TEAM ARENA";
		s_main.teamArena.color                  = uis.themeColor;
		s_main.teamArena.style                  = style;
	}

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.cinematics.generic.type          = MTYPE_PTEXT;
	s_main.cinematics.generic.flags         = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x             = 320;
	s_main.cinematics.generic.y             = y;
	s_main.cinematics.generic.id            = ID_CINEMATICS;
	s_main.cinematics.generic.callback      = Main_MenuEvent;
	s_main.cinematics.string                    = "CINEMATICS";
	s_main.cinematics.color                 = uis.themeColor;
	s_main.cinematics.style                 = style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.mods.generic.type            = MTYPE_PTEXT;
	s_main.mods.generic.flags           = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.mods.generic.x               = 320;
	s_main.mods.generic.y               = y;
	s_main.mods.generic.id              = ID_MODS;
	s_main.mods.generic.callback        = Main_MenuEvent;
	s_main.mods.string                  = "MODS";
	s_main.mods.color                   = uis.themeColor;
	s_main.mods.style                   = style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type                = MTYPE_PTEXT;
	s_main.exit.generic.flags               = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_main.exit.generic.x                   = 320;
	s_main.exit.generic.y                   = y;
	s_main.exit.generic.id                  = ID_EXIT;
	s_main.exit.generic.callback            = Main_MenuEvent;
	s_main.exit.string                      = "EXIT";
	s_main.exit.color                       = uis.themeColor;
	s_main.exit.style                       = style;

	s_mainMenuTeamArena = teamArena;

	Menu_AddItem(&s_main.menu,  &s_main.singleplayer);
	Menu_AddItem(&s_main.menu,  &s_main.multiplayer);
	Menu_AddItem(&s_main.menu,  &s_main.setup);
	Menu_AddItem(&s_main.menu,  &s_main.demos);
	if (teamArena)
	{
		Menu_AddItem(&s_main.menu,  &s_main.teamArena);
	}
	Menu_AddItem(&s_main.menu,  &s_main.cinematics);
	Menu_AddItem(&s_main.menu,  &s_main.mods);
	Menu_AddItem(&s_main.menu,  &s_main.exit);

	Main_MenuUpdateFavLayout(UI_FavouriteServer_GetMenuShift());
	s_mainMenuLastFavShift = UI_FavouriteServer_GetMenuShift();

	trap_Key_SetCatcher(KEYCATCH_UI);
	uis.menusp = 0;
	UI_PushMenu(&s_main.menu);

	UI_FavouriteServer_FetchOnMainMenu();
	UI_ServerStatus_FetchOnMainMenu();
}
