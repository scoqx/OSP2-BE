// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

ADVANCED OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAMEL              "menu/art/frame2_l"
#define ART_FRAMER              "menu/art/frame1_r"
#define ART_BACK0               "menu/art/back_0"
#define ART_BACK1               "menu/art/back_1"

#define ADVANCED_X_POS      340
#define ADVANCED_X_POS_1    230
#define ADVANCED_X_POS_2    450

#define ID_FOV              132
#define ID_SENSITIVITY      133
#define ID_BACK             190

#define MAX_INFO_MESSAGES   2
static void Advanced_StatusBar(void* self)
{
	const char* infomsg;
	const char* infomsg2;
	int id;
	static const char* info_messages[MAX_INFO_MESSAGES][2] =
	{
		{ "Field of view in degrees", "Min - 1 Max - 160"},
		{ "Mouse sensitivity", "Adjust mouse movement speed"}
	};

	id = ((menucommon_s*)self)->id;
	if (id >= ID_FOV && id < ID_FOV + MAX_INFO_MESSAGES)
	{
		infomsg = info_messages[id - ID_FOV][0];
		infomsg2 = info_messages[id - ID_FOV][1];
	}
	else
	{
		infomsg = "";
		infomsg2 = "";
	}

	if (infomsg && infomsg[0])
	{
		if (infomsg2 && infomsg2[0])
		{
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.92, infomsg, UI_SMALLFONT | UI_CENTER, color_white);
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.96, infomsg2, UI_SMALLFONT | UI_CENTER, color_white);
		}
		else
		{
			UI_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.94, infomsg, UI_SMALLFONT | UI_CENTER, color_white);
		}
	}
}

typedef struct
{
	menuframework_s     menu;

	menutext_s          banner;
	menubitmap_s        framel;
	menubitmap_s        framer;

	menufield_s         fov;
	menufield_s         sensitivity;

	menubitmap_s        back;
} advanced_t;

static advanced_t s_advanced;

static void Advanced_SetMenuItems(void)
{
	trap_Cvar_VariableStringBuffer("cg_fov", s_advanced.fov.field.buffer, sizeof(s_advanced.fov.field.buffer));
	trap_Cvar_VariableStringBuffer("sensitivity", s_advanced.sensitivity.field.buffer, sizeof(s_advanced.sensitivity.field.buffer));
}

/*
=================
Advanced_SaveChanges
=================
*/
static void Advanced_SaveChanges(void)
{
	float fov, fov2;
	float sens, sens2;

	fov = fov2 = atof(s_advanced.fov.field.buffer);

	if (fov == 0) fov = 100;
	else if (fov < 1) fov = 1;
	else if (fov > 160) fov = 160;

	if (fov != fov2)
		Com_sprintf(s_advanced.fov.field.buffer, 5, "%f", fov);

	sens = sens2 = atof(s_advanced.sensitivity.field.buffer);

	if (sens < 0) sens = 0;
	else if (sens > 20) sens = 20;

	if (sens != sens2)
		Com_sprintf(s_advanced.sensitivity.field.buffer, 5, "%f", sens);

	trap_Cvar_Set("cg_fov", s_advanced.fov.field.buffer);
	trap_Cvar_Set("sensitivity", s_advanced.sensitivity.field.buffer);
}

static void Advanced_Event(void* ptr, int notification)
{

	if (notification != QM_ACTIVATED)
	{
		return;
	}

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_FOV:
			Advanced_SaveChanges();
			break;

		case ID_SENSITIVITY:
			Advanced_SaveChanges();
			break;

		case ID_BACK:
			Advanced_SaveChanges();
			UI_PopMenu();
			break;
	}
}

/*
=================
Advanced_MenuKey
=================
*/
static sfxHandle_t Advanced_MenuKey(int key)
{
	if (key == K_MOUSE2 || key == K_ESCAPE)
	{
		Advanced_SaveChanges();
	}
	return Menu_DefaultKey(&s_advanced.menu, key);
}

static void Advanced_MenuInit(void)
{
	int             y, ystart;

	memset(&s_advanced, 0, sizeof(advanced_t));

	Advanced_Cache();

	s_advanced.menu.key        = Advanced_MenuKey;
	s_advanced.menu.wrapAround = qtrue;
	s_advanced.menu.fullscreen = qtrue;

	s_advanced.banner.generic.type  = MTYPE_BTEXT;
	s_advanced.banner.generic.x     = 320;
	s_advanced.banner.generic.y     = 16;
	s_advanced.banner.string        = "ADVANCED SETTINGS";
	s_advanced.banner.color         = uis.themeColor;
	s_advanced.banner.style         = UI_CENTER;

	s_advanced.framel.generic.type  = MTYPE_BITMAP;
	s_advanced.framel.generic.name  = ART_FRAMEL;
	s_advanced.framel.generic.flags = QMF_INACTIVE;
	s_advanced.framel.generic.x    = FRAME_L_X;
	s_advanced.framel.generic.y    = FRAME_L_Y;
	s_advanced.framel.width        = FRAME_L_W;
	s_advanced.framel.height       = FRAME_L_H;

	s_advanced.framer.generic.type  = MTYPE_BITMAP;
	s_advanced.framer.generic.name  = ART_FRAMER;
	s_advanced.framer.generic.flags = QMF_INACTIVE;
	s_advanced.framer.generic.x    = FRAME_R_X;
	s_advanced.framer.generic.y    = FRAME_R_Y;
	s_advanced.framer.width        = FRAME_R_W;
	s_advanced.framer.height       = FRAME_R_H;

	ystart = 144 - BIGCHAR_HEIGHT * 3;
	y = ystart;

	y += BIGCHAR_HEIGHT + 2;
	s_advanced.fov.generic.type       = MTYPE_FIELD;
	s_advanced.fov.generic.name       = "FOV:";
	s_advanced.fov.generic.flags      = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_NUMBERSONLY;
	s_advanced.fov.generic.x          = ADVANCED_X_POS_1;
	s_advanced.fov.generic.callback   = Advanced_Event;
	s_advanced.fov.generic.statusbar = Advanced_StatusBar;
	s_advanced.fov.generic.id         = ID_FOV;
	s_advanced.fov.generic.y          = y;
	s_advanced.fov.field.widthInChars = 5;
	s_advanced.fov.field.maxchars     = 4;

	y += BIGCHAR_HEIGHT + 2;
	s_advanced.sensitivity.generic.type       = MTYPE_FIELD;
	s_advanced.sensitivity.generic.name       = "Sensitivity:";
	s_advanced.sensitivity.generic.flags      = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_NUMBERSONLY;
	s_advanced.sensitivity.generic.x          = ADVANCED_X_POS_2;
	s_advanced.sensitivity.generic.callback   = Advanced_Event;
	s_advanced.sensitivity.generic.statusbar = Advanced_StatusBar;
	s_advanced.sensitivity.generic.id         = ID_SENSITIVITY;
	s_advanced.sensitivity.generic.y          = y;
	s_advanced.sensitivity.field.widthInChars = 5;
	s_advanced.sensitivity.field.maxchars     = 4;

	y += BIGCHAR_HEIGHT + 2;
	s_advanced.back.generic.type        = MTYPE_BITMAP;
	s_advanced.back.generic.name     = ART_BACK0;
	s_advanced.back.generic.flags    = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_advanced.back.generic.callback = Advanced_Event;
	s_advanced.back.generic.id      = ID_BACK;
	s_advanced.back.generic.x       = 0;
	s_advanced.back.generic.y       = 480 - 64;
	s_advanced.back.width           = 128;
	s_advanced.back.height              = 64;
	s_advanced.back.focuspic         = ART_BACK1;

	Menu_AddItem(&s_advanced.menu, &s_advanced.banner);
	Menu_AddItem(&s_advanced.menu, &s_advanced.framel);
	Menu_AddItem(&s_advanced.menu, &s_advanced.framer);

	Menu_AddItem(&s_advanced.menu, &s_advanced.fov);
	Menu_AddItem(&s_advanced.menu, &s_advanced.sensitivity);

	Menu_AddItem(&s_advanced.menu, &s_advanced.back);

	Advanced_SetMenuItems();
}

/*
===============
Advanced_Cache
===============
*/
void Advanced_Cache(void)
{
	trap_R_RegisterShaderNoMip(ART_FRAMEL);
	trap_R_RegisterShaderNoMip(ART_FRAMER);
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);
}

/*
===============
UI_AdvancedMenu
===============
*/
void UI_AdvancedMenu(void)
{
	Advanced_MenuInit();
	UI_PushMenu(&s_advanced.menu);
}
