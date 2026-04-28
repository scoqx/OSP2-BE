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
#define UI_TIMER_ATTACK			500
#define UI_EASTER_ROCKET_TIME	800
#define UI_EASTER_GIBS_COUNT	10
#define UI_EASTER_GIBS_TIME		900
#define UI_EASTER_SMOKE_PUFFS	14

typedef struct {
	menuframework_s	menu;

	playerInfo_t		playerinfo[7];
	sfxHandle_t			nightmareSound;
	sfxHandle_t			rocketFireSound;
	sfxHandle_t			rocketFlySound;
	sfxHandle_t			silenceSound;
	sfxHandle_t			rocketExplodeSound;
	sfxHandle_t			maleDeathSound;
	sfxHandle_t			femaleDeathSound;
	sfxHandle_t			doomDeathSound;
	sfxHandle_t			sargeDeathSound;
	sfxHandle_t			slashDeathSound;
	sfxHandle_t			gibSound;
	qhandle_t			smokePuffShader;
	qhandle_t			rocketExplosionShader;
	qhandle_t			bloodExplosionShader;
} creditsmenu_t;

static creditsmenu_t	s_credits2;

static float y_pos = 0;
static float y_step = DEFAULT_YSTEP;
static char oldMusicVolume[16];
static qboolean p_taunt;
static qboolean p_stand;
static int old_uis;
static qboolean font_loaded = qfalse;
static int rocketStartTime;
static int rocketTargetIndex;
static int rocketNextVictim;
static qboolean rocketInFlight;
static int explosionStartTime;
static int gibStartTime;
static qboolean rocketVictimAlive[3];
static float gibVel[UI_EASTER_GIBS_COUNT][2];
static qboolean rocketHasTarget;
static vec2_t rocketStartPos;
static vec2_t rocketEndPos;
static vec2_t rocketImpactPos;

static const int rocketVictimModelIndices[3] = { 3, 4, 5 };
static const float rocketVictimCenters[3][2] = {
	{ 50.0f, 320.0f },   /* Doom */
	{ 120.0f, 280.0f },  /* Sarge */
	{ 50.0f, 280.0f }    /* Slash */
};
/* Muzzle point for Bones rocket launcher (aligned to tag_flash in credits pose). */
#define BONES_MUZZLE_X 560.0f
#define BONES_MUZZLE_Y 210.0f

static void UI_CreditMenu2_DrawRect640( float x, float y, float w, float h, const vec4_t color ) {
	UI_FillRect( x, y, w, h, color );
}

static void UI_CreditMenu2_DrawShader640( float x, float y, float w, float h, qhandle_t shader, const vec4_t color ) {
	if ( !shader ) {
		UI_CreditMenu2_DrawRect640( x, y, w, h, color );
		return;
	}
	UI_SetColor( color );
	UI_DrawHandlePic( x, y, w, h, shader );
	UI_SetColor( NULL );
}

static void UI_CreditMenu2_DrawRocketModel640( float x, float y, float yaw ) {
	refdef_t refdef;
	refEntity_t projectile;
	vec3_t origin;
	vec3_t angles;
	float rx, ry, rw, rh;
	float xx;

	if ( !s_credits2.playerinfo[0].headModel ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );
	memset( &projectile, 0, sizeof( projectile ) );

	rx = x - 24.0f;
	ry = y - 12.0f;
	rw = 48.0f;
	rh = 24.0f;
	UI_AdjustFrom640( &rx, &ry, &rw, &rh );

	refdef.fov_x = 32;
	xx = rw / tan( refdef.fov_x / 360.0f * M_PI );
	refdef.fov_y = atan2( rh, xx );
	refdef.fov_y *= ( 360.0f / M_PI );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.x = (int)rx;
	refdef.y = (int)ry;
	refdef.width = (int)rw;
	refdef.height = (int)rh;
	refdef.time = uis.realtime;
	AxisClear( refdef.viewaxis );

	projectile.hModel = s_credits2.playerinfo[0].headModel;
	projectile.customSkin = s_credits2.playerinfo[0].headSkin;
	projectile.reType = RT_MODEL;
	projectile.customShader = 0;
	projectile.renderfx = RF_NOSHADOW | RF_LIGHTING_ORIGIN;
	origin[0] = 30.0f;
	origin[1] = 0.0f;
	origin[2] = 0.0f;
	VectorCopy( origin, projectile.origin );
	VectorCopy( origin, projectile.oldorigin );
	VectorCopy( origin, projectile.lightingOrigin );
	angles[PITCH] = 0.0f;
	angles[YAW] = yaw + 180.0f;
	angles[ROLL] = 0.0f;
	AnglesToAxis( angles, projectile.axis );

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &projectile );
	trap_R_RenderScene( &refdef );
}

static void UI_CreditMenu2_InitEaster( void ) {
	int i;
	rocketStartTime = 0;
	rocketTargetIndex = -1;
	rocketNextVictim = 0;
	rocketInFlight = qfalse;
	rocketHasTarget = qfalse;
	explosionStartTime = 0;
	gibStartTime = 0;
	rocketStartPos[0] = BONES_MUZZLE_X;
	rocketStartPos[1] = BONES_MUZZLE_Y;
	rocketEndPos[0] = BONES_MUZZLE_X;
	rocketEndPos[1] = BONES_MUZZLE_Y;
	rocketImpactPos[0] = BONES_MUZZLE_X;
	rocketImpactPos[1] = BONES_MUZZLE_Y;
	for ( i = 0; i < 3; i++ ) {
		rocketVictimAlive[i] = qtrue;
	}
	for ( i = 0; i < UI_EASTER_GIBS_COUNT; i++ ) {
		gibVel[i][0] = 0.0f;
		gibVel[i][1] = 0.0f;
	}
}

static void UI_CreditMenu2_SpawnGibs( void ) {
	int i;
	for ( i = 0; i < UI_EASTER_GIBS_COUNT; i++ ) {
		gibVel[i][0] = crandom() * 95.0f;
		gibVel[i][1] = -35.0f - random() * 95.0f;
	}
	gibStartTime = uis.realtime;
}

static void UI_CreditMenu2_RocketImpact( void ) {
	sfxHandle_t deathSound;
	int modelIndex;

	if ( !rocketHasTarget || rocketTargetIndex < 0 || rocketTargetIndex > 2 ) {
		return;
	}

	modelIndex = rocketVictimModelIndices[rocketTargetIndex];
	rocketVictimAlive[rocketTargetIndex] = qfalse;
	s_credits2.playerinfo[modelIndex].torsoAnim = ( ( s_credits2.playerinfo[modelIndex].torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_DEATH1;
	s_credits2.playerinfo[modelIndex].legsAnim = ( ( s_credits2.playerinfo[modelIndex].legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_DEATH1;

	if ( rocketTargetIndex == 0 ) {
		deathSound = s_credits2.doomDeathSound;
		if ( !deathSound ) {
			deathSound = s_credits2.maleDeathSound;
		}
	} else if ( rocketTargetIndex == 1 ) {
		deathSound = s_credits2.sargeDeathSound;
		if ( !deathSound ) {
			deathSound = s_credits2.maleDeathSound;
		}
	} else {
		deathSound = s_credits2.slashDeathSound;
		if ( !deathSound ) {
			deathSound = s_credits2.femaleDeathSound;
		}
	}
	if ( !deathSound ) {
		deathSound = s_credits2.femaleDeathSound;
	}
	/* Stop fly sound exactly on impact, then play explosion on same channel. */
	trap_S_StartLocalSound( s_credits2.silenceSound, CHAN_AUTO );
	trap_S_StartLocalSound( s_credits2.rocketExplodeSound, CHAN_AUTO );
	trap_S_StartLocalSound( deathSound, CHAN_AUTO );
	trap_S_StartLocalSound( s_credits2.gibSound, CHAN_AUTO );
	if ( !rocketVictimAlive[0] && !rocketVictimAlive[1] && !rocketVictimAlive[2] ) {
		trap_S_StartLocalSound( s_credits2.nightmareSound, CHAN_ANNOUNCER );
	}

	explosionStartTime = uis.realtime;
	rocketImpactPos[0] = rocketVictimCenters[rocketTargetIndex][0];
	rocketImpactPos[1] = rocketVictimCenters[rocketTargetIndex][1];
	UI_CreditMenu2_SpawnGibs();
	rocketInFlight = qfalse;
	rocketNextVictim++;
	if ( rocketNextVictim >= 3 ) {
		rocketNextVictim = 0;
	}
}

static void UI_CreditMenu2_StartRocket( void ) {
	playerInfo_t *bones;
	int tries;
	int victim;

	if ( rocketInFlight ) {
		return;
	}
 
	victim = rocketNextVictim;
	for ( tries = 0; tries < 3; tries++ ) {
		if ( rocketVictimAlive[victim] ) {
			break;
		}
		victim = ( victim + 1 ) % 3;
	}

	if ( !rocketVictimAlive[victim] ) {
		rocketTargetIndex = -1;
		rocketHasTarget = qfalse;
		rocketEndPos[0] = -40.0f;
		rocketEndPos[1] = BONES_MUZZLE_Y;
	} else {
		rocketTargetIndex = victim;
		rocketHasTarget = qtrue;
		rocketEndPos[0] = rocketVictimCenters[victim][0];
		rocketEndPos[1] = rocketVictimCenters[victim][1];
	}

	rocketInFlight = qtrue;
	rocketStartTime = uis.realtime;
	explosionStartTime = 0;

	bones = &s_credits2.playerinfo[0];
	bones->torsoAnim = ( ( bones->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_ATTACK;
	bones->torsoAnimationTimer = UI_TIMER_ATTACK;

	trap_S_StartLocalSound( s_credits2.rocketFireSound, CHAN_AUTO );
	trap_S_StartLocalSound( s_credits2.rocketFlySound, CHAN_AUTO );
}

static void UI_CreditMenu2_DrawEasterEffects( void ) {
	vec4_t smokeColor = { 0.92f, 0.92f, 0.92f, 0.55f };
	vec4_t boomColor = { 1.0f, 0.35f, 0.1f, 0.8f };
	vec4_t gibColor = { 0.75f, 0.05f, 0.05f, 0.9f };
	int now;

	now = uis.realtime;

	if ( rocketInFlight ) {
		float t;
		int i;
		float rocketX, rocketY;
		float rocketYaw;

		t = (float)( now - rocketStartTime ) / (float)UI_EASTER_ROCKET_TIME;
		if ( t >= 1.0f ) {
			t = 1.0f;
		}

		rocketX = rocketStartPos[0] + ( rocketEndPos[0] - rocketStartPos[0] ) * t;
		rocketY = rocketStartPos[1] + ( rocketEndPos[1] - rocketStartPos[1] ) * t;
		rocketYaw = atan2( rocketEndPos[1] - rocketStartPos[1], rocketEndPos[0] - rocketStartPos[0] ) * 180.0f / M_PI;

		for ( i = 0; i < UI_EASTER_SMOKE_PUFFS; i++ ) {
			float st;
			float sx;
			float sy;
			float size;
			vec4_t smoke;

			st = t - (float)i * 0.03f;
			if ( st <= 0.0f ) {
				break;
			}

			sx = rocketStartPos[0] + ( rocketEndPos[0] - rocketStartPos[0] ) * st;
			sy = rocketStartPos[1] + ( rocketEndPos[1] - rocketStartPos[1] ) * st;
			size = 4.0f + (float)i * 0.85f;
			smoke[0] = smokeColor[0];
			smoke[1] = smokeColor[1];
			smoke[2] = smokeColor[2];
			smoke[3] = smokeColor[3] * ( 1.0f - (float)i / (float)UI_EASTER_SMOKE_PUFFS );
			UI_CreditMenu2_DrawShader640( sx - size * 0.5f, sy - size * 0.5f, size, size, s_credits2.smokePuffShader, smoke );
		}

		if ( s_credits2.playerinfo[0].headModel ) {
			UI_CreditMenu2_DrawRocketModel640( rocketX, rocketY, rocketYaw );
		} else {
			vec4_t rocketColor = { 1.0f, 0.45f, 0.1f, 1.0f };
			UI_CreditMenu2_DrawRect640( rocketX - 12.0f, rocketY - 5.0f, 24.0f, 10.0f, rocketColor );
		}

		if ( now - rocketStartTime >= UI_EASTER_ROCKET_TIME ) {
			if ( rocketHasTarget ) {
				UI_CreditMenu2_RocketImpact();
			} else {
				trap_S_StartLocalSound( s_credits2.silenceSound, CHAN_WEAPON );
				rocketInFlight = qfalse;
				rocketTargetIndex = -1;
			}
		}
	}

	if ( explosionStartTime > 0 ) {
		int elapsed;
		elapsed = now - explosionStartTime;
		if ( elapsed < 350 ) {
			float radius;
			float alpha;
			vec4_t expColor;

			radius = 14.0f + (float)elapsed * 0.16f;
			alpha = 1.0f - (float)elapsed / 350.0f;
			expColor[0] = boomColor[0];
			expColor[1] = boomColor[1];
			expColor[2] = boomColor[2];
			expColor[3] = alpha;

			UI_CreditMenu2_DrawShader640(
				rocketImpactPos[0] - radius,
				rocketImpactPos[1] - radius,
				radius * 2.0f,
				radius * 2.0f,
				s_credits2.rocketExplosionShader,
				expColor
			);
		} else if ( elapsed >= 350 ) {
			explosionStartTime = 0;
		}
	}

	if ( gibStartTime > 0 ) {
		int i;
		int elapsed;
		elapsed = now - gibStartTime;
		if ( elapsed < UI_EASTER_GIBS_TIME ) {
			float t;
			t = (float)elapsed * 0.001f;
			for ( i = 0; i < UI_EASTER_GIBS_COUNT; i++ ) {
				float gx;
				float gy;
				gx = rocketImpactPos[0] + gibVel[i][0] * t;
				gy = rocketImpactPos[1] + gibVel[i][1] * t + 80.0f * t * t;
				UI_CreditMenu2_DrawShader640( gx - 5.0f, gy - 5.0f, 10.0f, 10.0f, s_credits2.bloodExplosionShader, gibColor );
			}
		} else {
			gibStartTime = 0;
		}
	}
}

/*
=================
UI_CreditMenu2_Key
=================
*/
static sfxHandle_t UI_CreditMenu2_Key( int key ) {
	if ( trap_Key_IsDown( K_CTRL ) && key == '5' ) {
		UI_CreditMenu2_StartRocket();
		return 0;
	}

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

	if ( rocketVictimAlive[2] ) {
		UI_DrawPlayer( -70-40,  0, 32*10, 56*10, &s_credits2.playerinfo[5], t );
	}
	if ( rocketVictimAlive[1] ) {
		UI_DrawPlayer(   0-40,  0, 32*10, 56*10, &s_credits2.playerinfo[4], t );
	}
	if ( rocketVictimAlive[0] ) {
		UI_DrawPlayer( -70-40, 40, 32*10, 56*10, &s_credits2.playerinfo[3], t );
	}

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
	UI_CreditMenu2_DrawEasterEffects();

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
	DRAW_WSTEP1("Konstalker")
	// 2nd block
	DRAW_WSTEP1("-=Financial Backers=-")
	DRAW_WSTEP1("enq3")
	DRAW_WSTEP1("John Coffey")
	DRAW_WSTEP1("Maked")
	DRAW_WSTEP1("Roskoshny")
	DRAW_WSTEP1("Anonymous player #1")
	DRAW_WSTEP1("Anonymous player #2")
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

	/* Avoid reloading/parsing fonts every open, but ensure fonts exist once. */
	if ( !UI_FontLoaded( 2 ) ) {
		UI_LoadFonts();
	}
	
	/* Update and select font from cvar */
	{
		int fontIndex;
		fontIndex = UI_FontIndexFromCvar( &ui_beCreditsFont );
		if ( !UI_FontLoaded( fontIndex ) ) {
			fontIndex = 2; /* fallback to sansman */
			trap_Cvar_Set( "ui_beCreditsFont", "2" );
		}
		if (UI_FontLoaded(fontIndex)) {
			UI_FontSelect(fontIndex);
			font_loaded = qtrue;
		} else {
			font_loaded = qfalse;
		}
	}

	UI_InitPlayerModels();
	UI_CreditMenu2_InitEaster();
	s_credits2.nightmareSound = trap_S_RegisterSound( "sound/misc/nightmare.wav", qfalse );
	s_credits2.rocketFireSound = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav", qfalse );
	s_credits2.rocketFlySound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
	s_credits2.silenceSound = trap_S_RegisterSound( "sound/misc/silence.wav", qfalse );
	s_credits2.rocketExplodeSound = trap_S_RegisterSound( "sound/weapons/rocket/rocklx1a.wav", qfalse );
	s_credits2.maleDeathSound = trap_S_RegisterSound( "sound/player/male1/death1.wav", qfalse );
	s_credits2.femaleDeathSound = trap_S_RegisterSound( "sound/player/female1/death1.wav", qfalse );
	s_credits2.doomDeathSound = trap_S_RegisterSound( "sound/player/doom/death1.wav", qfalse );
	s_credits2.sargeDeathSound = trap_S_RegisterSound( "sound/player/sarge/death1.wav", qfalse );
	s_credits2.slashDeathSound = trap_S_RegisterSound( "sound/player/slash/death1.wav", qfalse );
	s_credits2.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	s_credits2.smokePuffShader = trap_R_RegisterShaderNoMip( "smokePuff" );
	s_credits2.rocketExplosionShader = trap_R_RegisterShaderNoMip( "rocketExplosion" );
	s_credits2.bloodExplosionShader = trap_R_RegisterShaderNoMip( "bloodExplosion" );

	UI_PushMenu ( &s_credits2.menu );
	// set start pos
	y_pos = SCREEN_HEIGHT;	
	// calc y_step based on fps
	fps = trap_Cvar_VariableValue("com_maxfps");

	if (fps)
		y_step = 125.0f / fps * DEFAULT_YSTEP;
	
	trap_Cvar_VariableStringBuffer( "s_musicvolume", oldMusicVolume, sizeof( oldMusicVolume ) );
	/* set to 10% of s_volume if music is off, only when that would be > 0 */
	{
		float totalVol, musicVol;
		totalVol = trap_Cvar_VariableValue( "s_volume" );
		musicVol = totalVol * 0.25f;
		if ( !trap_Cvar_VariableValue( "s_musicvolume" ) && musicVol > 0.0f )
			trap_Cvar_SetValue( "s_musicvolume", musicVol );
	}

	// Music placeholder
	trap_Cmd_ExecuteText(EXEC_APPEND, "music music/TZ_Inc_Free_Song.ogg");

}
