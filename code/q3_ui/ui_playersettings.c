// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"

#define ART_FRAMEL          "menu/art/frame2_l"
#define ART_FRAMER          "menu/art/frame1_r"
#define ART_MODEL0          "menu/art/model_0"
#define ART_MODEL1          "menu/art/model_1"
#define ART_BACK0           "menu/art/back_0"
#define ART_BACK1           "menu/art/back_1"
#define ART_FX_BASE         "menu/art/fx_base"
#define ART_FX_BLUE         "menu/art/fx_blue"
#define ART_FX_CYAN         "menu/art/fx_cyan"
#define ART_FX_GREEN        "menu/art/fx_grn"
#define ART_FX_RED          "menu/art/fx_red"
#define ART_FX_TEAL         "menu/art/fx_teal"
#define ART_FX_WHITE        "menu/art/fx_white"
#define ART_FX_YELLOW       "menu/art/fx_yel"

#define ID_NAME         10
#define ID_HANDICAP     11
#define ID_EFFECTS      12
#define ID_BACK         13
#define ID_MODEL        14

#define MAX_NAMELENGTH  32


typedef struct
{
	menuframework_s     menu;

	menutext_s          banner;
	menubitmap_s        framel;
	menubitmap_s        framer;
	menubitmap_s        player;

	menufield_s         name;
	menulist_s          handicap;
	menulist_s          effects;

	menubitmap_s        back;
	menubitmap_s        model;
	menubitmap_s        item_null;

	qhandle_t           fxBasePic;
	qhandle_t           fxPic;
	playerInfo_t        playerinfo;
	int                 current_fx;
	char                playerModel[MAX_QPATH];
} playersettings_t;

static playersettings_t s_playersettings;

static const char* handicap_items[] =
{
	"None",
	"95",
	"90",
	"85",
	"80",
	"75",
	"70",
	"65",
	"60",
	"55",
	"50",
	"45",
	"40",
	"35",
	"30",
	"25",
	"20",
	"15",
	"10",
	"5",
	NULL
};


/*
=================
PlayerSettings_DrawName
=================
*/
static void PlayerSettings_DrawName(void* self)
{
	menufield_s*     f;
	qboolean        focus;
	int             style;
	char*            txt;
	char            c;
	vec4_t          color;
	int             n;
	int             basex, x, y;
	char            name[32];

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT | UI_SMALLFONT;
	Vector4Copy(text_color_normal, color);
	if (focus)
	{
		style |= UI_PULSE;
		Vector4Copy(text_color_highlight, color);
	}

	UI_DrawProportionalString(basex, y, "Name", style, color);

	// draw the actual name
	basex += 64;
	y += PROP_HEIGHT;
	txt = f->field.buffer;
	Vector4Copy(g_color_table[ColorIndex(COLOR_WHITE)], color);
	x = basex;
	while ((c = *txt) != 0)
	{
		/* Only parse color codes when field is NOT in focus - when in focus, display all characters as-is */
		if (!focus && Q_IsColorString(txt))
		{
			char controlChar;
			float r;
			float g;
			float b;
			int d1;
			int d2;
			int colorInt;
			vec4_t hexColor;

			controlChar = *(txt + 1);
			if (controlChar == 'X' || controlChar == 'x')
			{
				/* Check for ^Xrrggbb or ^xrrggbb format */
				if (txt[2] != '\0' && txt[3] != '\0' && txt[4] != '\0' &&
				        txt[5] != '\0' && txt[6] != '\0' && txt[7] != '\0')
				{
					/* Check if all 6 characters are hex and parse colors */
					if ((txt[2] >= '0' && txt[2] <= '9') || (txt[2] >= 'a' && txt[2] <= 'f') || (txt[2] >= 'A' && txt[2] <= 'F'))
					{
						if ((txt[3] >= '0' && txt[3] <= '9') || (txt[3] >= 'a' && txt[3] <= 'f') || (txt[3] >= 'A' && txt[3] <= 'F'))
						{
							if ((txt[4] >= '0' && txt[4] <= '9') || (txt[4] >= 'a' && txt[4] <= 'f') || (txt[4] >= 'A' && txt[4] <= 'F'))
							{
								if ((txt[5] >= '0' && txt[5] <= '9') || (txt[5] >= 'a' && txt[5] <= 'f') || (txt[5] >= 'A' && txt[5] <= 'F'))
								{
									if ((txt[6] >= '0' && txt[6] <= '9') || (txt[6] >= 'a' && txt[6] <= 'f') || (txt[6] >= 'A' && txt[6] <= 'F'))
									{
										if ((txt[7] >= '0' && txt[7] <= '9') || (txt[7] >= 'a' && txt[7] <= 'f') || (txt[7] >= 'A' && txt[7] <= 'F'))
										{
											/* Valid ^Xrrggbb format, parse and set color */
											/* Parse red */
											if (txt[2] >= '0' && txt[2] <= '9')
											{
												d1 = txt[2] - '0';
											}
											else if (txt[2] >= 'a' && txt[2] <= 'f')
											{
												d1 = txt[2] - 'a' + 10;
											}
											else
											{
												d1 = txt[2] - 'A' + 10;
											}
											if (txt[3] >= '0' && txt[3] <= '9')
											{
												d2 = txt[3] - '0';
											}
											else if (txt[3] >= 'a' && txt[3] <= 'f')
											{
												d2 = txt[3] - 'a' + 10;
											}
											else
											{
												d2 = txt[3] - 'A' + 10;
											}
											colorInt = d1 * 16 + d2;
											r = (float)colorInt / 255.0f;

											/* Parse green */
											if (txt[4] >= '0' && txt[4] <= '9')
											{
												d1 = txt[4] - '0';
											}
											else if (txt[4] >= 'a' && txt[4] <= 'f')
											{
												d1 = txt[4] - 'a' + 10;
											}
											else
											{
												d1 = txt[4] - 'A' + 10;
											}
											if (txt[5] >= '0' && txt[5] <= '9')
											{
												d2 = txt[5] - '0';
											}
											else if (txt[5] >= 'a' && txt[5] <= 'f')
											{
												d2 = txt[5] - 'a' + 10;
											}
											else
											{
												d2 = txt[5] - 'A' + 10;
											}
											colorInt = d1 * 16 + d2;
											g = (float)colorInt / 255.0f;

											/* Parse blue */
											if (txt[6] >= '0' && txt[6] <= '9')
											{
												d1 = txt[6] - '0';
											}
											else if (txt[6] >= 'a' && txt[6] <= 'f')
											{
												d1 = txt[6] - 'a' + 10;
											}
											else
											{
												d1 = txt[6] - 'A' + 10;
											}
											if (txt[7] >= '0' && txt[7] <= '9')
											{
												d2 = txt[7] - '0';
											}
											else if (txt[7] >= 'a' && txt[7] <= 'f')
											{
												d2 = txt[7] - 'a' + 10;
											}
											else
											{
												d2 = txt[7] - 'A' + 10;
											}
											colorInt = d1 * 16 + d2;
											b = (float)colorInt / 255.0f;

											/* Set color */
											hexColor[0] = r;
											hexColor[1] = g;
											hexColor[2] = b;
											hexColor[3] = color[3];
											Vector4Copy(hexColor, color);

											/* Skip all 8 characters */
											txt += 8;
											continue;
										}
									}
								}
							}
						}
					}
				}
				/* Invalid format, treat as regular text */
			}
			else
			{
				n = ColorIndex(controlChar);
				Vector4Copy(g_color_table[n], color);
				txt += 2;
				continue;
			}
		}
		/* Use UI_DrawStringRaw to avoid color code parsing when drawing individual characters */
		{
			char charBuf[2];
			charBuf[0] = c;
			charBuf[1] = '\0';
			UI_DrawStringRaw(x, y, charBuf, style, color);
		}
		txt++;
		x += SMALLCHAR_WIDTH;
	}

	// draw cursor if we have focus
	if (focus)
	{
		if (trap_Key_GetOverstrikeMode())
		{
			c = 11;
		}
		else
		{
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar(basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white);
	}

	// draw at bottom also using proportional font with colors preserved
	Q_strncpyz(name, f->field.buffer, sizeof(name));
	UI_DrawProportionalString(320, 440, name, UI_CENTER | UI_BIGFONT, text_color_normal);
}


/*
=================
PlayerSettings_DrawHandicap
=================
*/
static void PlayerSettings_DrawHandicap(void* self)
{
	menulist_s*      item;
	qboolean        focus;
	int             style;
	float*           color;

	item = (menulist_s*)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT | UI_SMALLFONT;
	color = text_color_normal;
	if (focus)
	{
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString(item->generic.x, item->generic.y, "Handicap", style, color);
	UI_DrawProportionalString(item->generic.x + 64, item->generic.y + PROP_HEIGHT, handicap_items[item->curvalue], style, color);
}


/*
=================
PlayerSettings_DrawEffects
=================
*/
static void PlayerSettings_DrawEffects(void* self)
{
	menulist_s*      item;
	qboolean        focus;
	int             style;
	float*           color;
	vec4_t          colors;
	int             c;

	item = (menulist_s*)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT | UI_SMALLFONT;
	color = text_color_normal;
	if (focus)
	{
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString(item->generic.x, item->generic.y, "Effects", style, color);

	UI_DrawHandlePic(item->generic.x + 64, item->generic.y + PROP_HEIGHT + 8, 128, 8, s_playersettings.fxBasePic);
	c = item->curvalue + 1;
	colors[0] = (c >> 0) & 1;
	colors[1] = (c >> 1) & 1;
	colors[2] = (c >> 2) & 1;
	colors[3] = 1.0;
	UI_SetColor(colors);
	UI_DrawHandlePic(item->generic.x + 64 + item->curvalue * 16 + 8, item->generic.y + PROP_HEIGHT + 6, 16, 12, s_playersettings.fxPic);
	UI_SetColor(NULL);
}


/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer(void* self)
{
	menubitmap_s*    b;
	vec3_t          viewangles;
	char            buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer("model", buf, sizeof(buf));
	if (strcmp(buf, s_playersettings.playerModel) != 0)
	{
		UI_PlayerInfo_SetModel(&s_playersettings.playerinfo, buf);
		strcpy(s_playersettings.playerModel, buf);

		viewangles[YAW]   = 180 - 30;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo(&s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse);
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer(b->generic.x, b->generic.y, b->width, b->height, &s_playersettings.playerinfo, uis.realtime / 2);
}


/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges(void)
{
	// name
	trap_Cvar_Set("name", s_playersettings.name.field.buffer);

	// handicap
	trap_Cvar_SetValue("handicap", 100 - s_playersettings.handicap.curvalue * 5);

	// effects color
	trap_Cvar_SetValue("color1", s_playersettings.effects.curvalue + 1);
}


/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey(int key)
{
	if (key == K_MOUSE2 || key == K_ESCAPE)
	{
		PlayerSettings_SaveChanges();
	}
	return Menu_DefaultKey(&s_playersettings.menu, key);
}


/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems(void)
{
	vec3_t  viewangles;
	int     c;
	int     h;

	// name
	Q_strncpyz(s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer));

	// effects color
	c = trap_Cvar_VariableValue("color1") - 1;
	if (c < 0 || c > 6)
	{
		c = 6;
	}
	s_playersettings.effects.curvalue = c;

	// model/skin
	memset(&s_playersettings.playerinfo, 0, sizeof(playerInfo_t));

	viewangles[YAW]   = 180 - 30;
	viewangles[PITCH] = 0;
	viewangles[ROLL]  = 0;

	UI_PlayerInfo_SetModel(&s_playersettings.playerinfo, UI_Cvar_VariableString("model"));
	UI_PlayerInfo_SetInfo(&s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse);

	// handicap
	h = Com_Clamp(5, 100, trap_Cvar_VariableValue("handicap"));
	s_playersettings.handicap.curvalue = 20 - h / 5;
}


/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
	{
		return;
	}

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_HANDICAP:
			trap_Cvar_Set("handicap", va("%i", 100 - 25 * s_playersettings.handicap.curvalue));
			break;

		case ID_MODEL:
			PlayerSettings_SaveChanges();
			UI_PlayerModelMenu();
			break;

		case ID_BACK:
			PlayerSettings_SaveChanges();
			UI_PopMenu();
			break;
	}
}


/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit(void)
{
	int     y;

	memset(&s_playersettings, 0, sizeof(playersettings_t));

	PlayerSettings_Cache();

	s_playersettings.menu.key        = PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround = qtrue;
	s_playersettings.menu.fullscreen = qtrue;

	s_playersettings.banner.generic.type  = MTYPE_BTEXT;
	s_playersettings.banner.generic.x     = 320;
	s_playersettings.banner.generic.y     = 16;
	s_playersettings.banner.string        = "PLAYER SETTINGS";
	s_playersettings.banner.color         = uis.themeColor;
	s_playersettings.banner.style         = UI_CENTER;

	s_playersettings.framel.generic.type  = MTYPE_BITMAP;
	s_playersettings.framel.generic.name  = ART_FRAMEL;
	s_playersettings.framel.generic.flags = QMF_LEFT_JUSTIFY | QMF_INACTIVE;
	s_playersettings.framel.generic.x     = FRAME_L_X;
	s_playersettings.framel.generic.y     = FRAME_L_Y;
	s_playersettings.framel.width         = FRAME_L_W;
	s_playersettings.framel.height        = FRAME_L_H;

	s_playersettings.framer.generic.type  = MTYPE_BITMAP;
	s_playersettings.framer.generic.name  = ART_FRAMER;
	s_playersettings.framer.generic.flags = QMF_LEFT_JUSTIFY | QMF_INACTIVE;
	s_playersettings.framer.generic.x     = FRAME_R_X;
	s_playersettings.framer.generic.y     = FRAME_R_Y;
	s_playersettings.framer.width         = FRAME_R_W;
	s_playersettings.framer.height        = FRAME_R_H;

	y = 144;
	s_playersettings.name.generic.type          = MTYPE_FIELD;
	s_playersettings.name.generic.flags         = QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw     = PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars    = MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars        = MAX_NAMELENGTH;
	s_playersettings.name.generic.x             = 192;
	s_playersettings.name.generic.y             = y;
	s_playersettings.name.generic.left          = 192 - 8;
	s_playersettings.name.generic.top           = y - 8;
	s_playersettings.name.generic.right         = 192 + 200;
	s_playersettings.name.generic.bottom        = y + 2 * PROP_HEIGHT;

	y += 3 * PROP_HEIGHT;
	s_playersettings.handicap.generic.type      = MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.flags     = QMF_NODEFAULTINIT;
	s_playersettings.handicap.generic.id        = ID_HANDICAP;
	s_playersettings.handicap.generic.ownerdraw = PlayerSettings_DrawHandicap;
	s_playersettings.handicap.generic.x         = 192;
	s_playersettings.handicap.generic.y         = y;
	s_playersettings.handicap.generic.left      = 192 - 8;
	s_playersettings.handicap.generic.top       = y - 8;
	s_playersettings.handicap.generic.right     = 192 + 200;
	s_playersettings.handicap.generic.bottom    = y + 2 * PROP_HEIGHT;
	s_playersettings.handicap.numitems          = 20;

	y += 3 * PROP_HEIGHT;
	s_playersettings.effects.generic.type       = MTYPE_SPINCONTROL;
	s_playersettings.effects.generic.flags      = QMF_NODEFAULTINIT;
	s_playersettings.effects.generic.id         = ID_EFFECTS;
	s_playersettings.effects.generic.ownerdraw  = PlayerSettings_DrawEffects;
	s_playersettings.effects.generic.x          = 192;
	s_playersettings.effects.generic.y          = y;
	s_playersettings.effects.generic.left       = 192 - 8;
	s_playersettings.effects.generic.top        = y - 8;
	s_playersettings.effects.generic.right      = 192 + 200;
	s_playersettings.effects.generic.bottom     = y + 2 * PROP_HEIGHT;
	s_playersettings.effects.numitems           = 7;

	s_playersettings.model.generic.type         = MTYPE_BITMAP;
	s_playersettings.model.generic.name         = ART_MODEL0;
	s_playersettings.model.generic.flags        = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_playersettings.model.generic.id           = ID_MODEL;
	s_playersettings.model.generic.callback     = PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x            = 640;
	s_playersettings.model.generic.y            = 480 - 64;
	s_playersettings.model.width                = 128;
	s_playersettings.model.height               = 64;
	s_playersettings.model.focuspic             = ART_MODEL1;

	s_playersettings.player.generic.type        = MTYPE_BITMAP;
	s_playersettings.player.generic.flags       = QMF_INACTIVE;
	s_playersettings.player.generic.ownerdraw   = PlayerSettings_DrawPlayer;
	s_playersettings.player.generic.x           = 400;
	s_playersettings.player.generic.y           = -40;
	s_playersettings.player.width               = 32 * 10;
	s_playersettings.player.height              = 56 * 10;

	s_playersettings.back.generic.type          = MTYPE_BITMAP;
	s_playersettings.back.generic.name          = ART_BACK0;
	s_playersettings.back.generic.flags         = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.id            = ID_BACK;
	s_playersettings.back.generic.callback      = PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x             = 0;
	s_playersettings.back.generic.y             = 480 - 64;
	s_playersettings.back.width                 = 128;
	s_playersettings.back.height                = 64;
	s_playersettings.back.focuspic              = ART_BACK1;

	s_playersettings.item_null.generic.type     = MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags    = QMF_LEFT_JUSTIFY | QMF_MOUSEONLY | QMF_SILENT;
	s_playersettings.item_null.generic.x        = 0;
	s_playersettings.item_null.generic.y        = 0;
	s_playersettings.item_null.width            = 640;
	s_playersettings.item_null.height           = 480;

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.banner);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.framel);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.framer);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.name);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.handicap);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.effects);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.model);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.back);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.player);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.item_null);

	PlayerSettings_SetMenuItems();
}


/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache(void)
{
	trap_R_RegisterShaderNoMip(ART_FRAMEL);
	trap_R_RegisterShaderNoMip(ART_FRAMER);
	trap_R_RegisterShaderNoMip(ART_MODEL0);
	trap_R_RegisterShaderNoMip(ART_MODEL1);
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);

	s_playersettings.fxBasePic = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	s_playersettings.fxPic = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
}


/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu(void)
{
	PlayerSettings_MenuInit();
	UI_PushMenu(&s_playersettings.menu);
}
