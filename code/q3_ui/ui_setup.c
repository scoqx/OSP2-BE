// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

SETUP MENU

=======================================================================
*/


#include "ui_local.h"


#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_CUSTOMIZEPLAYER		10
#define ID_CUSTOMIZECONTROLS	11
#define ID_SYSTEMCONFIG			12
#define ID_GAME					13
#define ID_OTHER				14
#define ID_ADVANCED				15
#define ID_LOAD					17
#define ID_SAVE					18
#define ID_DEFAULTS				19
#define ID_BACK					20

#define ID_OTHER_CDKEY			30
#define ID_OTHER_CREDITS		32
#define ID_OTHER_BACK			33


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		setupplayer;
	menutext_s		setupcontrols;
	menutext_s		setupsystem;
	menutext_s		game;
	menutext_s		other;
	menutext_s		advanced;
//	menutext_s		load;
//	menutext_s		save;
	menutext_s		defaults;
	menubitmap_s	back;
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		cdkey;
	menutext_s		credits;
	menubitmap_s	back;
} otherMenuInfo_t;

static otherMenuInfo_t	otherMenuInfo;


/*
=================
Setup_ResetDefaults_Action
=================
*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
Setup_ResetDefaults_Draw
=================
*/
static void Setup_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset *ALL*", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "options to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
}


/*
===============
UI_SetupMenu_Event
===============
*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSettingsMenu();
		break;

	case ID_CUSTOMIZECONTROLS:
		UI_ControlsMenu();
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;

	case ID_OTHER:
		UI_OtherMenu();
		break;

//	case ID_ADVANCED:
//		UI_AdvancedMenu();
//		break;

//	case ID_LOAD:
//		UI_LoadConfigMenu();
//		break;

//	case ID_SAVE:
//		UI_SaveConfigMenu();
//		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SetupMenu_Init
===============
*/
static void UI_SetupMenu_Init( void ) {
	int				y;

	UI_SetupMenu_Cache();

	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
	setupMenuInfo.banner.string						= "SETUP";
	setupMenuInfo.banner.color						= color_white;
	setupMenuInfo.banner.style						= UI_CENTER;

	setupMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framel.generic.name				= ART_FRAMEL;
	setupMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framel.generic.x					= 0;  
	setupMenuInfo.framel.generic.y					= 78;
	setupMenuInfo.framel.width  					= 256;
	setupMenuInfo.framel.height  					= 329;

	setupMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framer.generic.name				= ART_FRAMER;
	setupMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framer.generic.x					= 376;
	setupMenuInfo.framer.generic.y					= 76;
	setupMenuInfo.framer.width  					= 256;
	setupMenuInfo.framer.height  					= 334;

	/* Calculate menu height and center it vertically */
	{
		int menuItemCount;
		int totalMenuHeight;
		int screenCenterY;

		menuItemCount = 5; /* PLAYER, CONTROLS, SYSTEM, GAME OPTIONS, OTHER */
		if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
			menuItemCount++; /* DEFAULTS */
		}
		totalMenuHeight = menuItemCount * SETUP_MENU_VERTICAL_SPACING;
		screenCenterY = 240; /* Center of 480px screen */
		y = screenCenterY - (totalMenuHeight / 2);
	}
	setupMenuInfo.setupplayer.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupplayer.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupplayer.generic.x				= 320;
	setupMenuInfo.setupplayer.generic.y				= y;
	setupMenuInfo.setupplayer.generic.id			= ID_CUSTOMIZEPLAYER;
	setupMenuInfo.setupplayer.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupplayer.string				= "PLAYER";
	setupMenuInfo.setupplayer.color					= color_red;
	setupMenuInfo.setupplayer.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupcontrols.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupcontrols.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupcontrols.generic.x			= 320;
	setupMenuInfo.setupcontrols.generic.y			= y;
	setupMenuInfo.setupcontrols.generic.id			= ID_CUSTOMIZECONTROLS;
	setupMenuInfo.setupcontrols.generic.callback	= UI_SetupMenu_Event; 
	setupMenuInfo.setupcontrols.string				= "CONTROLS";
	setupMenuInfo.setupcontrols.color				= color_red;
	setupMenuInfo.setupcontrols.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 320;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupsystem.string				= "SYSTEM";
	setupMenuInfo.setupsystem.color					= color_red;
	setupMenuInfo.setupsystem.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.game.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.game.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.game.generic.x					= 320;
	setupMenuInfo.game.generic.y					= y;
	setupMenuInfo.game.generic.id					= ID_GAME;
	setupMenuInfo.game.generic.callback				= UI_SetupMenu_Event; 
	setupMenuInfo.game.string						= "GAME OPTIONS";
	setupMenuInfo.game.color						= color_red;
	setupMenuInfo.game.style						= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.other.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.other.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.other.generic.x					= 320;
	setupMenuInfo.other.generic.y					= y;
	setupMenuInfo.other.generic.id					= ID_OTHER;
	setupMenuInfo.other.generic.callback			= UI_SetupMenu_Event; 
	setupMenuInfo.other.string						= "OTHER";
	setupMenuInfo.other.color						= color_red;
	setupMenuInfo.other.style						= UI_CENTER;

//	y += SETUP_MENU_VERTICAL_SPACING;
//	setupMenuInfo.advanced.generic.type				= MTYPE_PTEXT;
//	setupMenuInfo.advanced.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
//	setupMenuInfo.advanced.generic.x				= 320;
//	setupMenuInfo.advanced.generic.y				= y;
//	setupMenuInfo.advanced.generic.id				= ID_ADVANCED;
//	setupMenuInfo.advanced.generic.callback			= UI_SetupMenu_Event; 
//	setupMenuInfo.advanced.string					= "ADVANCED";
//	setupMenuInfo.advanced.color					= color_darkred;
//	setupMenuInfo.advanced.style					= UI_CENTER;

	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
#if 0
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.load.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.load.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.load.generic.x					= 320;
		setupMenuInfo.load.generic.y					= y;
		setupMenuInfo.load.generic.id					= ID_LOAD;
		setupMenuInfo.load.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.load.string						= "LOAD";
		setupMenuInfo.load.color						= color_red;
		setupMenuInfo.load.style						= UI_CENTER;

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.save.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.save.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.save.generic.x					= 320;
		setupMenuInfo.save.generic.y					= y;
		setupMenuInfo.save.generic.id					= ID_SAVE;
		setupMenuInfo.save.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.save.string						= "SAVE";
		setupMenuInfo.save.color						= color_red;
		setupMenuInfo.save.style						= UI_CENTER;
#endif

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.defaults.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.defaults.generic.x				= 320;
		setupMenuInfo.defaults.generic.y				= y;
		setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
		setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.defaults.string					= "DEFAULTS";
		setupMenuInfo.defaults.color					= color_red;
		setupMenuInfo.defaults.style					= UI_CENTER;
	}

	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.banner );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupplayer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupcontrols );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsystem );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.game );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.other );
//	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.advanced );
//	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.load );
//	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.save );
	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
	}
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );
}


/*
=================
UI_SetupMenu_Cache
=================
*/
void UI_SetupMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_SetupMenu
===============
*/
void UI_SetupMenu( void ) {
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}


/*
===============
UI_OtherMenu_Event
===============
*/
static void UI_OtherMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_OTHER_CDKEY:
		UI_CDKeyMenu();
		break;

	case ID_OTHER_CREDITS:
		UI_CreditMenu();
		break;

	case ID_OTHER_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_OtherMenu_Init
===============
*/
static void UI_OtherMenu_Init( void ) {
	int				y;

	UI_SetupMenu_Cache();

	memset( &otherMenuInfo, 0, sizeof(otherMenuInfo) );
	otherMenuInfo.menu.wrapAround = qtrue;
	otherMenuInfo.menu.fullscreen = qtrue;

	otherMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	otherMenuInfo.banner.generic.x					= 320;
	otherMenuInfo.banner.generic.y					= 16;
	otherMenuInfo.banner.string						= "OTHER";
	otherMenuInfo.banner.color						= color_white;
	otherMenuInfo.banner.style						= UI_CENTER;

	otherMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	otherMenuInfo.framel.generic.name				= ART_FRAMEL;
	otherMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	otherMenuInfo.framel.generic.x					= 0;  
	otherMenuInfo.framel.generic.y					= 78;
	otherMenuInfo.framel.width  					= 256;
	otherMenuInfo.framel.height  					= 329;

	otherMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	otherMenuInfo.framer.generic.name				= ART_FRAMER;
	otherMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	otherMenuInfo.framer.generic.x					= 376;
	otherMenuInfo.framer.generic.y					= 76;
	otherMenuInfo.framer.width  					= 256;
	otherMenuInfo.framer.height  					= 334;

	/* Calculate menu height and center it vertically */
	{
		int menuItemCount;
		int totalMenuHeight;
		int screenCenterY;

		menuItemCount = 2; /* CD Key, CREDITS */
		totalMenuHeight = menuItemCount * SETUP_MENU_VERTICAL_SPACING;
		screenCenterY = 240; /* Center of 480px screen */
		y = screenCenterY - (totalMenuHeight / 2);
	}
	otherMenuInfo.cdkey.generic.type				= MTYPE_PTEXT;
	otherMenuInfo.cdkey.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	otherMenuInfo.cdkey.generic.x					= 320;
	otherMenuInfo.cdkey.generic.y					= y;
	otherMenuInfo.cdkey.generic.id					= ID_OTHER_CDKEY;
	otherMenuInfo.cdkey.generic.callback			= UI_OtherMenu_Event; 
	otherMenuInfo.cdkey.string						= "CD Key";
	otherMenuInfo.cdkey.color						= color_red;
	otherMenuInfo.cdkey.style						= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	otherMenuInfo.credits.generic.type				= MTYPE_PTEXT;
	otherMenuInfo.credits.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	otherMenuInfo.credits.generic.x					= 320;
	otherMenuInfo.credits.generic.y					= y;
	otherMenuInfo.credits.generic.id				= ID_OTHER_CREDITS;
	otherMenuInfo.credits.generic.callback			= UI_OtherMenu_Event;
	otherMenuInfo.credits.string					= "CREDITS";
	otherMenuInfo.credits.color						= color_red;
	otherMenuInfo.credits.style						= UI_CENTER;

	otherMenuInfo.back.generic.type					= MTYPE_BITMAP;
	otherMenuInfo.back.generic.name					= ART_BACK0;
	otherMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	otherMenuInfo.back.generic.id					= ID_OTHER_BACK;
	otherMenuInfo.back.generic.callback				= UI_OtherMenu_Event;
	otherMenuInfo.back.generic.x					= 0;
	otherMenuInfo.back.generic.y					= 480-64;
	otherMenuInfo.back.width						= 128;
	otherMenuInfo.back.height						= 64;
	otherMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.banner );
	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.framel );
	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.framer );
	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.cdkey );
	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.credits );
	Menu_AddItem( &otherMenuInfo.menu, &otherMenuInfo.back );
}


/*
===============
UI_OtherMenu
===============
*/
void UI_OtherMenu( void ) {
	UI_OtherMenu_Init();
	UI_PushMenu( &otherMenuInfo.menu );
}
