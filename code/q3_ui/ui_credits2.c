// Copyright (C) 2025-2026 diwoc
//
/*
=======================================================================

OSP2-BE CREDITS

=======================================================================
*/


#include "ui_local.h"

#define CGX_CREDITS_END 2000
#define DEFAULT_YSTEP 0.406f

#define PITCH1 21
#define YAW1 30
#define YAW2 50

#define UI_TIMER_GESTURE		2300

typedef struct {
	menuframework_s	menu;

	playerInfo_t		playerinfo[7];
} creditsmenu_t;

static creditsmenu_t	s_credits2;

static float y_pos = 0;
static float y_step = DEFAULT_YSTEP;
static char oldMusicVolume[16];
static qboolean p_taunt;
static qboolean p_stand;
static int old_uis;
static qboolean font_loaded = qfalse;

/*
=================
UI_CreditMenu2_Key
=================
*/
static sfxHandle_t UI_CreditMenu2_Key( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE || key == K_SPACE || key == K_ENTER) {
		trap_Cmd_ExecuteText(EXEC_APPEND, va("s_musicvolume %s; stopmusic;", oldMusicVolume));

		UI_PopMenu();
		return 0;
	} else if ((key == K_MWHEELDOWN  || key == K_DOWNARROW || key == K_PGDN)
		&& y_pos > -CGX_CREDITS_END) {
		y_pos -= PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	} else if ((key == K_MWHEELUP  || key == K_UPARROW || key == K_PGUP)
		&& y_pos > -CGX_CREDITS_END) {
		y_pos += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	}
	return 0;
}

/*
=================
UI_InitPlayerModels
=================
*/
static void UI_InitPlayerModels( void ) {
	char			buf[MAX_QPATH];
	vec3_t			viewangles;

	p_taunt = qfalse;
	p_stand = qfalse;
	
	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[0], "bones/default" );	
	viewangles[YAW]   = 180 - YAW1;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[0], LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_ROCKET_LAUNCHER, qfalse );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[1], "crash/default" );	
	viewangles[YAW]	  = 180 - YAW1;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[1], LEGS_IDLECR, TORSO_STAND, viewangles, vec3_origin, WP_LIGHTNING, qfalse );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[2], "visor/default" );	
	viewangles[YAW]	  = 180 - YAW1;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[2], LEGS_IDLECR, TORSO_STAND, viewangles, vec3_origin, WP_GRENADE_LAUNCHER, qfalse );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[3], "doom/default" );	
	viewangles[YAW]   = 270 - YAW2;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[3], LEGS_IDLECR, TORSO_STAND, viewangles, vec3_origin, WP_SHOTGUN, qfalse );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[4], "sarge/default" );	
	viewangles[YAW]   = 270 - YAW2;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[4], LEGS_IDLECR, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[5], "slash/yuriko" );	
	viewangles[YAW]   = 270 - YAW2;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[5], LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_RAILGUN, qfalse );
	// Set magenta color for railgun weapon model (R=1, G=0, B=1, A=1)
	s_credits2.playerinfo[5].useCustomWeaponColor = qtrue;
	s_credits2.playerinfo[5].customWeaponColor[0] = 1.0f;
	s_credits2.playerinfo[5].customWeaponColor[1] = 0.0f;
	s_credits2.playerinfo[5].customWeaponColor[2] = 1.0f;
	s_credits2.playerinfo[5].customWeaponColor[3] = 1.0f;

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );

	UI_PlayerInfo_SetModel( &s_credits2.playerinfo[6], buf );	
	viewangles[YAW]   = 180;
	viewangles[PITCH] = PITCH1;
	viewangles[ROLL]  = 0;
	UI_PlayerInfo_SetInfo( &s_credits2.playerinfo[6], LEGS_IDLE, TORSO_STAND2, viewangles, vec3_origin, WP_GAUNTLET, qfalse );	
}

/*
=================
UI_CreditMenu2_DrawPlayer
=================
*/
static void UI_CreditMenu2_DrawPlayer( void ) {
	int t;
	int i;
	
	t = uis.realtime / 2;	

	UI_DrawPlayer( 450,     0, 32*10, 56*10, &s_credits2.playerinfo[0], t );
	UI_DrawPlayer( 450-70,  0, 32*10, 56*10, &s_credits2.playerinfo[2], t );
	UI_DrawPlayer( 450,    40, 32*10, 56*10, &s_credits2.playerinfo[1], t );

	UI_DrawPlayer( -70-40,  0, 32*10, 56*10, &s_credits2.playerinfo[5], t );	
	UI_DrawPlayer(   0-40,  0, 32*10, 56*10, &s_credits2.playerinfo[4], t );	
	UI_DrawPlayer( -70-40, 40, 32*10, 56*10, &s_credits2.playerinfo[3], t );	

	if (CGX_CREDITS_END <= -y_pos && !p_taunt) {
		p_taunt = qtrue;
		for (i = 0; i < 7; i++) {
			s_credits2.playerinfo[i].torsoAnim = ( ( s_credits2.playerinfo[i].torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_GESTURE;
			s_credits2.playerinfo[i].torsoAnimationTimer = UI_TIMER_GESTURE;
		}
		old_uis = uis.realtime;
	}
	if (p_taunt && old_uis + 2500 <= uis.realtime && !p_stand) {
		p_stand = qtrue;
		s_credits2.playerinfo[6].torsoAnim = ( ( s_credits2.playerinfo[6].torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_STAND2;
	}
	
	UI_DrawPlayer(170, CGX_CREDITS_END + y_pos, 32 * 10, 56 * 10, &s_credits2.playerinfo[6], t);	
}

/*
===============
UI_CreditMenu2_Draw
===============
*/
#define BECHAR_TITLE_SIZE_WIDTH  16.0f
#define BECHAR_TITLE_SIZE_HEIGHT 24.0f
#define BECHAR_SIZE_WIDTH  12.0f
#define BECHAR_SIZE_HEIGHT 18.0f

#define DRAW_W(x) UI_OSPDrawStringNew( 320.0f, (float)y, x, color_white, color_black, BECHAR_SIZE_WIDTH, BECHAR_SIZE_HEIGHT, SCREEN_WIDTH, DS_PROPORTIONAL|DS_HCENTER|DS_SHADOW, NULL, NULL, NULL );
#define DRAW_R(x) UI_OSPDrawStringNew( 320.0f, (float)y, x, uis.themeColor, color_black, BECHAR_SIZE_WIDTH, BECHAR_SIZE_HEIGHT, SCREEN_WIDTH, DS_PROPORTIONAL|DS_HCENTER|DS_SHADOW, NULL, NULL, NULL );
#define DRAW_G(x) UI_OSPDrawStringNew( 320.0f, (float)y, x, text_color_disabled, color_black, BECHAR_SIZE_WIDTH, BECHAR_SIZE_HEIGHT, SCREEN_WIDTH, DS_PROPORTIONAL|DS_HCENTER|DS_SHADOW, NULL, NULL, NULL );
#define DRAW_GTITLE(x) UI_OSPDrawStringNew( 320.0f, (float)y, x, text_color_disabled, color_black, BECHAR_TITLE_SIZE_WIDTH, BECHAR_TITLE_SIZE_HEIGHT, SCREEN_WIDTH, DS_PROPORTIONAL|DS_HCENTER|DS_SHADOW, NULL, NULL, NULL );
#define STEP_Y() y += BECHAR_SIZE_HEIGHT;
#define STEP_Y1() y += 1.65f * BECHAR_SIZE_HEIGHT;

#define STEP_YTITLE() y += BECHAR_TITLE_SIZE_HEIGHT;
#define DRAW_WSTEP(x) DRAW_W(x) STEP_Y()
#define DRAW_WSTEP1(x) DRAW_W(x) STEP_Y1()
#define DRAW_RSTEP(x) DRAW_R(x) STEP_Y()
#define DRAW_RSTEP1(x) DRAW_R(x) STEP_Y1()
#define DRAW_GSTEP(x) DRAW_G(x) STEP_Y()
#define DRAW_GSTEP1(x) DRAW_G(x) STEP_Y1()
#define DRAW_GTITLESTEP(x) DRAW_GTITLE(x) STEP_YTITLE()

static void UI_CreditMenu2_Draw( void ) {
	float		y;

	y_pos -= y_step;

	if (y_pos < -CGX_CREDITS_END)
		y_pos = -CGX_CREDITS_END;
	else if (y_pos > SCREEN_HEIGHT)
		y_pos = SCREEN_HEIGHT;

	UI_CreditMenu2_DrawPlayer();

	/* Ensure selected font is active for credits text */
	if (font_loaded) {
		int fontIndex;
		fontIndex = UI_FontIndexFromCvar( &ui_beCreditsFont );
		if ( !UI_FontAvailable( fontIndex ) ) {
			fontIndex = 2; /* fallback to sansman */
		}
		UI_FontSelect(fontIndex);
	}

	y = y_pos;	

	DRAW_GTITLESTEP("OSP2-BE (Black Edition)")
	DRAW_WSTEP1("Programming")
	DRAW_WSTEP1("diwoc")
	DRAW_WSTEP1("2025-2026")
	DRAW_WSTEP1("https://scoqx.github.io/")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Based on OSP2")
	DRAW_WSTEP1("written by Snems")
	DRAW_WSTEP1("https://github.com/snems/OSP2")
	DRAW_WSTEP1("")
	DRAW_GTITLESTEP("Special thanks to:")
	// 1st block
	DRAW_WSTEP1("-=Friends & Contributors=-")
	DRAW_WSTEP1("Snems")
	DRAW_WSTEP1("Kr3m")
	DRAW_WSTEP1("Mirage")
	DRAW_WSTEP1("Mus1n")
	DRAW_WSTEP1("MrX")
	DRAW_WSTEP1("Zenx")
	DRAW_WSTEP1("Tuborg")
	DRAW_WSTEP1("Meta")
	// 2nd block
	DRAW_WSTEP1("-=Financial Backers=-")
	DRAW_WSTEP1("enq3")
	DRAW_WSTEP1("John Coffey")
	DRAW_WSTEP1("Maked")
	DRAW_WSTEP1("Anonymous player #1")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Based on source codes:")
	DRAW_WSTEP1("OSP")
	DRAW_WSTEP1("https://www.orangesmoothie.org")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Cyrax")
	DRAW_WSTEP1("https://github.com/ec-/baseq3a")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("x0ry")
	DRAW_WSTEP1("https://github.com/xq3e/engine")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Neil Toronto")
	DRAW_WSTEP1("http://ra.is/unlagged")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Ratmod")
	DRAW_WSTEP1("https://github.com/rdntcntrl/ratoa_gamecode")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("Music:")
	DRAW_WSTEP1("TZ-Inc. - Free Song")
	DRAW_WSTEP1("")
	DRAW_WSTEP1("")
	DRAW_RSTEP1("Quake III Arena")
	DRAW_WSTEP1("Id Software, Inc.")
	DRAW_WSTEP1("All Rights Reserved")

	y += 345;

	UI_DrawProportionalString( 320, y, "QUAKE III ARENA", UI_CENTER|UI_GIANTFONT|UI_PULSE, uis.themeColor );
}


/*
===============
UI_CreditMenu2
===============
*/
void UI_CreditMenu2( void ) {
	float fps;
	memset( &s_credits2, 0 ,sizeof(s_credits2) );

	s_credits2.menu.draw = UI_CreditMenu2_Draw;
	s_credits2.menu.key = UI_CreditMenu2_Key;
	s_credits2.menu.fullscreen = qtrue;

	UI_LoadFonts();
	
	/* Update and select font from cvar */
	{
		int fontIndex;
		fontIndex = UI_FontIndexFromCvar( &ui_beCreditsFont );
		if ( !UI_FontAvailable( fontIndex ) ) {
			fontIndex = 2; /* fallback to sansman */
			trap_Cvar_Set( "ui_beCreditsFont", "2" );
		}
		if (UI_FontAvailable(fontIndex)) {
			UI_FontSelect(fontIndex);
			font_loaded = qtrue;
		} else {
			font_loaded = qfalse;
		}
	}

	UI_InitPlayerModels();

	UI_PushMenu ( &s_credits2.menu );
	// set start pos
	y_pos = SCREEN_HEIGHT;	
	// calc y_step based on fps
	fps = trap_Cvar_VariableValue("com_maxfps");

	if (fps)
		y_step = 125.0f / fps * DEFAULT_YSTEP;
	
	trap_Cvar_VariableStringBuffer( "s_musicvolume", oldMusicVolume, sizeof( oldMusicVolume ) );
	// set a little music volume if it's off and restore it on menu pop
	if (!trap_Cvar_VariableValue("s_musicvolume"))
		trap_Cmd_ExecuteText(EXEC_APPEND, "s_musicvolume 0.2;");

	// Music placeholder
	trap_Cmd_ExecuteText(EXEC_APPEND, "music music/TZ_Inc_Free_Song.ogg");

}
