#include "ui_local.h"

typedef struct
{
	const char* cvarName;
	const char* description;
} uiCvarDescriptionTable_t;

static uiCvarDescriptionTable_t uiCvarDescriptionTable[] =
{
	{ "ui_infoDetail", "Detail shader draw type in menu.\n  ^20^7 - disabled\n  ^21^7 - default\n  ^22^7 - fullscreen" },
	{ "ui_menuBgStyle", "Menu background display mode.\n  ^21^7 - standard (main menu and submenus use different backgrounds)\n  ^22^7 - entire UI uses the main menu background image" },
	{ "ui_menuDarken", "Darken menu background overlay. ^20^7 - none, up to ^21^7 - fully dark" },
	{ "ui_forceGamma", "Force ^3r_gamma^7 to ^21^7 while in menus. Set ^20^7 to disable." },
	{ "ui_themeColor", "Color of menu elements.\n \"^2color^7\" - color name, digit or hex" },
	{ "ui_themeColor2", "Color of secondary menu elements.\n \"^2color^7\" - color name, digit or hex" },
	{ "ui_selectionColor", "Menu selection highlight color.\n \"^2color^7\" - color name, digit or hex" },
	{ "ui_serverNameThemeColor", "Server browser hostname colors.\n  ^20^7 - keep color codes from hostname\n  ^21^7 - strip color codes (use theme color)" },
	{ "ui_shudListFont", "Font for the SuperHUD config file list in the in-game menu. Font index; use ^3/fonts^7 to list available fonts." },
	{ "ui_beCreditsFont", "Font for the BE credits screen. Font index; use ^3/fonts^7 to list available fonts." },
	{ "ui_framesFullscreen", "Menu frame layout.\n  ^20^7 - classic framed layout\n  ^21^7 - fullscreen frames" },
	{ "ui_banner", "Main menu banner.\n  ^21^7 - default\n  ^22^7 - alternative (supports ^3ui_bannerColor^7)" },
	{ "ui_bannerColor", "Alternative banner color.\n \"^2color^7\" - color name, digit or hex" },
	{ "ui_pallete_type", "Color picker palette mode index. Set from the in-menu color picker." },
	{ "ui_beStatusFont", "Font for the main menu BE status bar and changelog panel. Font index; use ^3/fonts^7 to list available fonts." },
	{ "ui_beStatusBorderColor", "Border color for the main menu BE status bar and changelog panel. Clear cvar to hide borders.\n \"^2color^7\" - color name, digit or hex" },
};

static const int uiCvarDescriptionTableSize = sizeof(uiCvarDescriptionTable) / sizeof(uiCvarDescriptionTable[0]);

void UI_RegisterCvarDescriptions(void)
{
	int i;

	for (i = 0; i < uiCvarDescriptionTableSize; i++)
	{
		if (uiCvarDescriptionTable[i].description && uiCvarDescriptionTable[i].description[0])
		{
			UI_SetCvarDescription(uiCvarDescriptionTable[i].cvarName, uiCvarDescriptionTable[i].description);
		}
	}
}
