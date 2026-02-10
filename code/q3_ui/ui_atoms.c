// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error( int level, const char *fmt, ... ) {
	va_list		argptr;
	char		text[2048];

	va_start( argptr, fmt );
	Q_vsprintf( text, fmt, argptr );
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[2048];

	va_start( argptr, fmt );
	Q_vsprintf( text, fmt, argptr );
	va_end (argptr);

	trap_Print( text );
}

#endif

/*
=================
UI_GetMenuBgStyle

Returns current ui_menuBgStyle (1 or 2), cached in uiStatic_t.
1 - first picture only on first page (main menu), everywhere else second picture.
2 - first picture everywhere (main menu and all other full-screen menus/dialogs).
=================
*/
static int UI_GetMenuBgStyle( void ) {
	int style;

	trap_Cvar_Update( &ui_menuBgStyle );

	if ( uis.menuBgStyleModCount != ui_menuBgStyle.modificationCount ) {
		uis.menuBgStyleModCount = ui_menuBgStyle.modificationCount;
		uis.menuBgStyle = ui_menuBgStyle.integer;

		/* Clamp to supported range */
		if ( uis.menuBgStyle < 1 || uis.menuBgStyle > 2 ) {
			uis.menuBgStyle = 1;
		}
	}

	style = uis.menuBgStyle;
	if ( style < 1 || style > 2 ) {
		style = 1;
	}

	return style;
}

/*
=================
UI_DrawMenuBackground

Draws either the "primary" or "secondary" menu background.

usePrimaryPicture:
 - qtrue  - "first" background (with logo), equivalent to old behaviour.
 - qfalse - "second" background (no-logo), used for non-main pages when style=1.
=================
*/
void UI_DrawMenuBackground( qboolean usePrimaryPicture ) {
	if ( usePrimaryPicture ) {
		/* no-logo underlay + logo overlay */
		if ( uis.menuBackNoLogoShader )
			trap_R_DrawStretchPic( 0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, uis.menuBackNoLogoShader );
		if ( uis.menuBackShader )
			trap_R_DrawStretchPic( 0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, uis.menuBackShader );
	} else {
		/* Second picture: only no-logo; fall back to logo if needed */
		if ( uis.menuBackNoLogoShader )
			trap_R_DrawStretchPic( 0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, uis.menuBackNoLogoShader );
		else if ( uis.menuBackShader )
			trap_R_DrawStretchPic( 0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 1, 1, uis.menuBackShader );
	}
}

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop( void ) {
	trap_Cmd_ExecuteText( EXEC_APPEND, "d1\n" );
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuframework_s *menu )
{
	int				i;
	menucommon_s*	item;

	// avoid stacking menus invoked by hotkeys
	for (i=0 ; i<uis.menusp ; i++)
	{
		if (uis.stack[i] == menu)
		{
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp)
	{
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	if ( trap_Cvar_VariableValue( "ui_forceGamma" ) && ( uis.menusp == 1 ) ) {
		{
			float rg = trap_Cvar_VariableValue( "r_gamma" );
			/* only save to uc_gamma if r_gamma looks like real value, not menu-forced 1 */
			if ( rg < 0.99f || rg > 1.01f ) {
				trap_Cvar_SetValue( "uc_gamma", rg );
			}
			trap_Cvar_SetValue( "r_gamma", 1.0f );
		}
	}

	// /* default cursor position */
	menu->cursor      = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher( KEYCATCH_UI );

	// force first available item to have focus
	for (i=0; i<menu->nitems; i++)
	{
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE)))
		{
			menu->cursor_prev = -1;
			Menu_SetCursor( menu, i );
			break;
		}
	}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu (void)
{
	trap_S_StartLocalSound( menu_out_sound, CHAN_LOCAL_SOUND );

	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error ("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	}
	else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	if ( trap_Cvar_VariableValue( "ui_forceGamma" ) ) {
		trap_Cvar_SetValue( "r_gamma", trap_Cvar_VariableValue( "uc_gamma" ) );
	}
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Key_ClearStates();
	trap_Cvar_Set( "cl_paused", "0" );
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

/*
=================
UI_DrawProportionalString2
=================
*/
static int	propMap[128][3] = {
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, PROP_SPACE_WIDTH},		// SPACE
{11, 122, 7},	// !
{154, 181, 14},	// "
{55, 122, 17},	// #
{79, 122, 18},	// $
{101, 122, 23},	// %
{153, 122, 18},	// &
{9, 93, 7},		// '
{207, 122, 8},	// (
{230, 122, 9},	// )
{177, 122, 18},	// *
{30, 152, 18},	// +
{85, 181, 7},	// ,
{34, 93, 11},	// -
{110, 181, 6},	// .
{130, 152, 14},	// /

{22, 64, 17},	// 0
{41, 64, 12},	// 1
{58, 64, 17},	// 2
{78, 64, 18},	// 3
{98, 64, 19},	// 4
{120, 64, 18},	// 5
{141, 64, 18},	// 6
{204, 64, 16},	// 7
{162, 64, 17},	// 8
{182, 64, 18},	// 9
{59, 181, 7},	// :
{35,181, 7},	// ;
{203, 152, 14},	// <
{56, 93, 14},	// =
{228, 152, 14},	// >
{177, 181, 18},	// ?

{28, 122, 22},	// @
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{60, 152, 7},	// [
{106, 151, 13},	// '\'
{83, 152, 7},	// ]
{128, 122, 17},	// ^
{4, 152, 21},	// _

{134, 181, 5},	// '
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{153, 152, 13},	// {
{11, 181, 5},	// |
{180, 152, 13},	// }
{79, 93, 17},	// ~
{0, 0, -1}		// DEL
};

static int propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

// bk001205 - code below duplicated in cgame/cg_drawtools.c
// bk001205 - FIXME: does this belong in ui_shared.c?
/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, vec4_t color )
{
	const char* s;
	unsigned char	ch; // bk001204 - unsigned
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// Com_Printf( "UI_DrawBannerString2: %s\n", str );
	// Com_Printf( "UI_DrawBannerString2: %f %f %f %f\n", color[0], color[1], color[2], color[3] );
	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x * uis.scale + uis.biasX;
	ay = y * uis.scale + uis.biasY;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* uis.scale;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * uis.scale;
			ah = (float)PROPB_HEIGHT * uis.scale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetPropBNew );
			ax += (aw + (float)PROPB_GAP_WIDTH * uis.scale);
		}
		s++;
	}

	trap_R_SetColor( NULL );
}

void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}
	// Com_Printf( "UI_DrawBannerString: %s\n", str );
	// Com_Printf( "UI_DrawBannerString: %f %f %f %f\n", color[0], color[1], color[2], color[3] );
	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor );
	}

	UI_DrawBannerString2( x, y, str, color );
}

static qboolean UI_Hex16GetColor( const char* str, float* color );

int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;
	float			r;
	float			g;
	float			b;
	char			controlChar;

	s = str;
	width = 0;
	while ( *s ) {
		if ( s[0] == '^' && s[1] != '\0' && s[1] != '^' ) {
			controlChar = s[1];
			s += 2;

			switch ( controlChar ) {
				case 'X':
				case 'x':
					if ( UI_Hex16GetColor( &s[0], &r ) &&
						UI_Hex16GetColor( &s[2], &g ) &&
						UI_Hex16GetColor( &s[4], &b ) ) {
						s += 6;
						continue;
					}
					break;
				case 'f':
					if ( ( uis.realtime & 0x3ff ) >= 512 ) {
						while ( *s && !( s[0] == '^' && ( s[1] == 'N' || s[1] == 'F' ) ) ) {
							s++;
						}
					}
					continue;
				case 'F':
					if ( ( uis.realtime & 0x3ff ) < 512 ) {
						while ( *s && !( s[0] == '^' && ( s[1] == 'N' || s[1] == 'f' ) ) ) {
							s++;
						}
					}
					continue;
				default:
					continue;
			}
			continue;
		}

		ch = *s & 127;
		charWidth = propMap[ch][2];
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	if ( width > 0 ) {
		width -= PROP_GAP_WIDTH;
	}
	return width;
}

static void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, qhandle_t charset )
{
	const char* s;
	unsigned char	ch; // bk001204 - unsigned
	float	ax;
	float	ay;
	float	aw = 0; // bk001204 - init
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	vec4_t	localColor;
	vec4_t	tempcolor;
	float	defaultAlpha;
	float	r;
	float	g;
	float	b;
	int	blink;
	char	controlChar;

	Vector4Copy( color, localColor );
	defaultAlpha = localColor[3];

	// draw the colored text
	trap_R_SetColor( localColor );
	
	ax = x * uis.scale + uis.biasX;
	ay = y * uis.scale + uis.biasY;

	s = str;
	while ( *s )
	{
		/* Always hide ^ followed by a digit (0-9) - skip the symbols but apply color */
		if ( s[0] == '^' && s[1] >= '0' && s[1] <= '9' )
		{
			controlChar = s[1];
			s += 2;
			
			/* Use color as-is - don't replace ^0 with white in proportional font */
			memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
			tempcolor[3] = localColor[3];
			Vector4Copy( tempcolor, localColor );
			trap_R_SetColor( localColor );
			continue;
		}
		
		/* Handle ^ if it's not part of a valid color code */
		if ( s[0] == '^' && ( s[1] == '\0' || s[1] == '^' ) )
		{
			/* Display ^ as regular character */
			ch = '^' & 127;
			if ( propMap[ch][2] != -1 ) {
				fcol = (float)propMap[ch][0] / 256.0f;
				frow = (float)propMap[ch][1] / 256.0f;
				fwidth = (float)propMap[ch][2] / 256.0f;
				fheight = (float)PROP_HEIGHT / 256.0f;
				aw = (float)propMap[ch][2] * uis.scale * sizeScale;
				ah = (float)PROP_HEIGHT * uis.scale * sizeScale;
				trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
			}
			ax += aw;
			s++;
			continue;
		}
		
		if ( s[0] == '^' && s[1] != '\0' && s[1] != '^' ) {
			controlChar = s[1];
			s += 2;

			switch ( controlChar ) {
				case 'N':
				case 'n':
					localColor[3] = defaultAlpha;
					trap_R_SetColor( localColor );
					continue;
				case 'B':
					blink = uis.realtime & 0x7ff;
					if ( blink > 1024 ) {
						blink = (~blink) & 0x7ff;
					}
					localColor[3] = blink / 1463.0f + 0.3f;
					trap_R_SetColor( localColor );
					continue;
				case 'b':
					blink = uis.realtime & 0x7ff;
					if ( blink > 1024 ) {
						blink = (~blink) & 0x7ff;
					}
					localColor[3] = blink / 1024.0f;
					trap_R_SetColor( localColor );
					continue;
				case 'f':
					if ( ( uis.realtime & 0x3ff ) >= 512 ) {
						while ( *s && !( s[0] == '^' && ( s[1] == 'N' || s[1] == 'F' ) ) ) {
							s++;
						}
					}
					continue;
				case 'F':
					if ( ( uis.realtime & 0x3ff ) < 512 ) {
						while ( *s && !( s[0] == '^' && ( s[1] == 'N' || s[1] == 'f' ) ) ) {
							s++;
						}
					}
					continue;
				case 'X':
				case 'x':
					if ( UI_Hex16GetColor( &s[0], &r ) &&
						UI_Hex16GetColor( &s[2], &g ) &&
						UI_Hex16GetColor( &s[4], &b ) ) {
						localColor[0] = r;
						localColor[1] = g;
						localColor[2] = b;
						trap_R_SetColor( localColor );
						s += 6;
						continue;
					}
					break;
				default:
					memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
					tempcolor[3] = localColor[3];
					Vector4Copy( tempcolor, localColor );
					trap_R_SetColor( localColor );
					continue;
			}
			continue;
		}

		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * uis.scale * sizeScale;
		}
		else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * uis.scale * sizeScale;
			ah = (float)PROP_HEIGHT * uis.scale * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
		}

		ax += (aw + (float)PROP_GAP_WIDTH * uis.scale * sizeScale);
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
	if(  style & UI_SMALLFONT ) {
		return PROP_SMALL_SIZE_SCALE;
	}

	return 1.00;
}


/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, uis.charsetProp );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetProp );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, color, sizeScale, uis.charsetProp );

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin( ( uis.realtime % TMOD_1000 ) / PULSE_DIVISOR );
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetPropGlow );
		return;
	}
	
	if ( style & UI_PULSE_2 ) {
		float pulseAmount;
		float targetR, targetG, targetB;
		
		// Определяем направление пульсации для каждого канала
		// Если цвет ближе к 1, идём к 0. Если ближе к 0, идём к 1.
		if ( color[0] > 0.5 ) {
			targetR = 0.0; // светлый -> тёмный
		} else {
			targetR = 1.0; // тёмный -> светлый
		}
		
		if ( color[1] > 0.5 ) {
			targetG = 0.0;
		} else {
			targetG = 1.0;
		}
		
		if ( color[2] > 0.5 ) {
			targetB = 0.0;
		} else {
			targetB = 1.0;
		}

		// Пульсация от 0 до 1 и обратно
		pulseAmount = 0.5 + 0.5 * cos( 2.0 * 3.14159265358979323846 * ( uis.realtime % 1000 ) / 1000.0 );

		// Интерполяция между исходным цветом и целевым
		drawcolor[0] = color[0] + (targetR - color[0]) * pulseAmount;
		drawcolor[1] = color[1] + (targetG - color[1]) * pulseAmount;
		drawcolor[2] = color[2] + (targetB - color[2]) * pulseAmount;
		drawcolor[3] = color[3]; // альфа остаётся как есть
		
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetPropGlow );
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, sizeScale, uis.charsetProp );
}

/*
=================
UI_DrawProportionalString_Wrapped
=================
*/
void UI_DrawProportionalString_AutoWrapped( int x, int y, int xmax, int ystep, const char* str, int style, vec4_t color ) {
	int width;
	char *s1,*s2,*s3;
	char c_bcp;
	char buf[1024];
	float   sizeScale;

	if (!str || str[0]=='\0')
		return;
	
	sizeScale = UI_ProportionalSizeScale( style );
	
	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3!=' ' && *s3!='\0');
		c_bcp = *s3;
		*s3 = '\0';
		width = UI_ProportionalStringWidth(s1) * sizeScale;
		*s3 = c_bcp;
		if (width > xmax) {
			if (s1==s2)
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			UI_DrawProportionalString(x, y, s1, style, color);
			y += ystep;
			if (c_bcp == '\0')
      {
        // that was the last word
        // we could start a new loop, but that wouldn't be much use
        // even if the word is too long, we would overflow it (see above)
        // so just print it now if needed
        s2++;
        if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
          UI_DrawProportionalString(x, y, s2, style, color);
				break; 
      }
			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				UI_DrawProportionalString(x, y, s1, style, color);
				break;
			}
		}
	}
}

/*
=================
UI_DrawString2
=================
*/
static qboolean UI_OSPCharHexToInt( char c, int* out )
{
	if ( c >= '0' && c <= '9' )
	{
		*out = c - '0';
		return qtrue;
	}
	if ( c >= 'a' && c <= 'f' )
	{
		*out = c - 'a' + 10;
		return qtrue;
	}
	if ( c >= 'A' && c <= 'F' )
	{
		*out = c - 'A' + 10;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Hex16GetColor( const char* str, float* color )
{
	int d1;
	int d2;
	int colorInt;

	if ( !str )
	{
		return qfalse;
	}

	if ( !UI_OSPCharHexToInt( str[0], &d1 ) )
	{
		return qfalse;
	}

	if ( !UI_OSPCharHexToInt( str[1], &d2 ) )
	{
		return qfalse;
	}

	colorInt = d1 * 16 + d2;
	*color = (float)colorInt;
	*color /= 255.0f;

	return qtrue;
}

static void UI_DrawString2( int x, int y, const char* str, vec4_t color, int charw, int charh, qboolean parseColorCodes, qboolean disableColorChange, qboolean convertBlackToWhite )
{
	char	ch;
	vec4_t	tempcolor;
	vec4_t	localColor;
	float	defaultAlpha;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	const char* strPtr;
	float r;
	float g;
	float b;
	int blink;

	if (y < -charh)
		// offscreen
		return;

	Vector4Copy( color, localColor );
	defaultAlpha = localColor[3];

	// draw the colored text
	trap_R_SetColor( localColor );
	
	ax = x * uis.scale + uis.biasX;
	ay = y * uis.scale + uis.biasY;
	aw = charw * uis.scale;
	ah = charh * uis.scale;

	strPtr = str;

	while ( *strPtr )
	{
		/* Always hide ^ followed by a digit (0-9) - skip the symbols but apply color if parseColorCodes is true */
		if ( strPtr[0] == '^' && strPtr[1] >= '0' && strPtr[1] <= '9' )
		{
			char controlChar;
			
			controlChar = strPtr[1];
			strPtr += 2;
			
			/* Apply color if parseColorCodes is enabled */
			if ( parseColorCodes && !disableColorChange )
			{
				if ( controlChar == '0' && convertBlackToWhite )
				{
					/* Replace black (^0) with white (^7) for visibility - only in server browser */
					memcpy( tempcolor, g_color_table[ColorIndex( '7' )], sizeof( tempcolor ) );
				}
				else
				{
					memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
				}
				tempcolor[3] = localColor[3];
				Vector4Copy( tempcolor, localColor );
				trap_R_SetColor( localColor );
			}
			continue;
		}
		
		/* Handle ^ if it's not part of a valid color code */
		if ( parseColorCodes && strPtr[0] == '^' )
		{
			/* If ^ is at end of string or followed by ^, display it as regular character */
			if ( strPtr[1] == '\0' || strPtr[1] == '^' )
			{
				/* Display ^ as regular character */
				ch = '^';
				frow = (ch>>4)*0.0625;
				fcol = (ch&15)*0.0625;
				trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset );
				ax += aw;
				strPtr++;
				continue;
			}
		}
		
		if ( parseColorCodes && strPtr[0] == '^' && strPtr[1] != '\0' && strPtr[1] != '^' )
		{
			char controlChar;
			qboolean processed;
#ifdef __GNUC__
			__attribute__((unused))
#endif
			const char* saved_strPtr;

			controlChar = strPtr[1];
			saved_strPtr = strPtr;
			processed = qfalse;

			/* Skip ^ and controlChar */
			strPtr += 2;

			switch ( controlChar )
			{
				case '0':
					/* Replace black (^0) with white (^7) for visibility - only in server browser */
					if ( !disableColorChange )
					{
						if ( convertBlackToWhite )
						{
							memcpy( tempcolor, g_color_table[ColorIndex( '7' )], sizeof( tempcolor ) );
						}
						else
						{
							memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
						}
						tempcolor[3] = localColor[3];
						Vector4Copy( tempcolor, localColor );
						trap_R_SetColor( localColor );
					}
					processed = qtrue;
					break;
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if ( !disableColorChange )
					{
						memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
						tempcolor[3] = localColor[3];
						Vector4Copy( tempcolor, localColor );
						trap_R_SetColor( localColor );
					}
					processed = qtrue;
					break;
				case 'N':
				case 'n':
					localColor[3] = defaultAlpha;
					trap_R_SetColor( localColor );
					processed = qtrue;
					break;
				case 'B':
					blink = uis.realtime & 0x7ff;
					if ( blink > 1024 )
					{
						blink = (~blink) & 0x7ff;
					}
					localColor[3] = blink / 1463.0f + 0.3f;
					trap_R_SetColor( localColor );
					processed = qtrue;
					break;
				case 'b':
					blink = uis.realtime & 0x7ff;
					if ( blink > 1024 )
					{
						blink = (~blink) & 0x7ff;
					}
					localColor[3] = blink / 1024.0f;
					trap_R_SetColor( localColor );
					processed = qtrue;
					break;
				case 'f':
					if ( ( uis.realtime & 0x3ff ) >= 512 )
					{
						while ( *strPtr && !( strPtr[0] == '^' && ( strPtr[1] == 'N' || strPtr[1] == 'F' ) ) )
						{
							strPtr++;
						}
					}
					processed = qtrue;
					break;
				case 'F':
					if ( ( uis.realtime & 0x3ff ) < 512 )
					{
						while ( *strPtr && !( strPtr[0] == '^' && ( strPtr[1] == 'N' || strPtr[1] == 'f' ) ) )
						{
							strPtr++;
						}
					}
					processed = qtrue;
					break;
				case 'X':
				case 'x':
					if ( UI_Hex16GetColor( &strPtr[0], &r ) &&
						UI_Hex16GetColor( &strPtr[2], &g ) &&
						UI_Hex16GetColor( &strPtr[4], &b ) )
					{
						if ( !disableColorChange )
						{
							localColor[0] = r;
							localColor[1] = g;
							localColor[2] = b;
							trap_R_SetColor( localColor );
						}
						strPtr += 6;
						processed = qtrue;
					}
					else
					{
						/* Invalid ^Xrrggbb format - skip ^X and continue with next character */
						processed = qtrue;
					}
					break;
				default:
					if ( !disableColorChange )
					{
						memcpy( tempcolor, g_color_table[ColorIndex( controlChar )], sizeof( tempcolor ) );
						tempcolor[3] = localColor[3];
						Vector4Copy( tempcolor, localColor );
						trap_R_SetColor( localColor );
					}
					processed = qtrue;
					break;
			}
			if ( processed )
			{
				continue;
			}
			/* If code not processed (shouldn't happen, but just in case), skip ^ and control char */
			continue;
		}

		ch = *strPtr & 255;
		if (ch != ' ')
		{
			frow = (ch>>4)*0.0625;
			fcol = (ch&15)*0.0625;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset );
		}

		ax += aw;
		strPtr++;
	}
	trap_R_SetColor( NULL );
}


/*
=================
UI_StringWidth
=================
Calculate visible string width excluding color codes
*/
static int UI_StringWidth( const char* str, qboolean parseColorCodes )
{
	const char* strPtr;
	int width;
	int charw;

	if ( !str || !*str ) {
		return 0;
	}

	/* Use SMALLCHAR_WIDTH as default - will be adjusted by caller */
	charw = SMALLCHAR_WIDTH;

	strPtr = str;
	width = 0;
	while ( *strPtr )
	{
		if ( parseColorCodes && strPtr[0] == '^' && strPtr[1] != '\0' && strPtr[1] != '^' )
		{
			char controlChar;

			controlChar = strPtr[1];
			strPtr += 2;

			switch ( controlChar )
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case 'N':
				case 'n':
				case 'B':
				case 'b':
				case 'f':
				case 'F':
					/* Skip color codes - don't count them */
					continue;
				case 'X':
				case 'x':
					/* Check for ^Xrrggbb or ^xrrggbb format */
					if ( strPtr[0] != '\0' && strPtr[1] != '\0' && strPtr[2] != '\0' &&
					     strPtr[3] != '\0' && strPtr[4] != '\0' && strPtr[5] != '\0' )
					{
						/* Check if all 6 characters are hex */
						if ( ( ( strPtr[0] >= '0' && strPtr[0] <= '9' ) || ( strPtr[0] >= 'a' && strPtr[0] <= 'f' ) || ( strPtr[0] >= 'A' && strPtr[0] <= 'F' ) ) &&
						     ( ( strPtr[1] >= '0' && strPtr[1] <= '9' ) || ( strPtr[1] >= 'a' && strPtr[1] <= 'f' ) || ( strPtr[1] >= 'A' && strPtr[1] <= 'F' ) ) &&
						     ( ( strPtr[2] >= '0' && strPtr[2] <= '9' ) || ( strPtr[2] >= 'a' && strPtr[2] <= 'f' ) || ( strPtr[2] >= 'A' && strPtr[2] <= 'F' ) ) &&
						     ( ( strPtr[3] >= '0' && strPtr[3] <= '9' ) || ( strPtr[3] >= 'a' && strPtr[3] <= 'f' ) || ( strPtr[3] >= 'A' && strPtr[3] <= 'F' ) ) &&
						     ( ( strPtr[4] >= '0' && strPtr[4] <= '9' ) || ( strPtr[4] >= 'a' && strPtr[4] <= 'f' ) || ( strPtr[4] >= 'A' && strPtr[4] <= 'F' ) ) &&
						     ( ( strPtr[5] >= '0' && strPtr[5] <= '9' ) || ( strPtr[5] >= 'a' && strPtr[5] <= 'f' ) || ( strPtr[5] >= 'A' && strPtr[5] <= 'F' ) ) )
						{
							/* Valid ^Xrrggbb format - skip all 8 characters (^X + 6 hex) */
							strPtr += 6;
							continue;
						}
					}
					/* Invalid format - skip ^X only */
					break;
				default:
					/* Other color codes - skip them */
					break;
			}
			continue;
		}

		/* Count visible character */
		if ( *strPtr != ' ' )
		{
			width += charw;
		}
		else
		{
			width += charw;
		}
		strPtr++;
	}

	return width;
}

/*
=================
UI_DrawStringCommon
=================
*/
static void UI_DrawStringCommon( int x, int y, const char* str, int style, vec4_t color, qboolean parseColorCodes, qboolean convertBlackToWhite )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	vec4_t	lowlight;
	float	*drawcolor;
	vec4_t	dropcolor;
	double	pulse;
	int		visibleWidth;
	vec4_t	pulseColor;

	if ( !str ) {
		return;
	}

	if ( ( style & UI_BLINK ) && ( ( uis.realtime / BLINK_DIVISOR ) & 1 ) )
	{
		return;
	}

	if ( style & UI_SMALLFONT )
	{
		charw = SMALLCHAR_WIDTH;
		charh = SMALLCHAR_HEIGHT;
	}
	else if ( style & UI_GIANTFONT )
	{
		charw = GIANTCHAR_WIDTH;
		charh = GIANTCHAR_HEIGHT;
	}
	else
	{
		charw = BIGCHAR_WIDTH;
		charh = BIGCHAR_HEIGHT;
	}

	if ( style & UI_PULSE )
	{
		lowlight[0] = 0.8f * color[0];
		lowlight[1] = 0.8f * color[1];
		lowlight[2] = 0.8f * color[2];
		lowlight[3] = 0.8f * color[3];
		pulse = (double)uis.realtime / (double)PULSE_DIVISOR;
		UI_LerpColor( color, lowlight, newcolor, 0.5f + 0.5f * (float)sin( pulse ) );
		drawcolor = newcolor;
	}
	else
	{
		drawcolor = color;
	}

	switch ( style & UI_FORMATMASK )
	{
		case UI_CENTER:
			if ( parseColorCodes )
			{
				visibleWidth = UI_StringWidth( str, parseColorCodes );
				/* Adjust charw in UI_StringWidth calculation */
				visibleWidth = visibleWidth * charw / SMALLCHAR_WIDTH;
				x = x - visibleWidth / 2;
			}
			else
			{
				len = strlen( str );
				x = x - len * charw / 2;
			}
			break;

		case UI_RIGHT:
			if ( parseColorCodes )
			{
				visibleWidth = UI_StringWidth( str, parseColorCodes );
				/* Adjust charw in UI_StringWidth calculation */
				visibleWidth = visibleWidth * charw / SMALLCHAR_WIDTH;
				x = x - visibleWidth;
			}
			else
			{
				len = strlen( str );
				x = x - len * charw;
			}
			break;

		default:
			break;
	}

	if ( style & UI_PULSE_2 )
	{
		float pulseAmount;
		float targetR, targetG, targetB;
		
		// Определяем направление пульсации для каждого канала
		if ( color[0] > 0.5 ) {
			targetR = 0.0f;
		} else {
			targetR = 1.0f;
		}
		
		if ( color[1] > 0.5 ) {
			targetG = 0.0f;
		} else {
			targetG = 1.0f;
		}
		
		if ( color[2] > 0.5 ) {
			targetB = 0.0f;
		} else {
			targetB = 1.0f;
		}
		
		// Пульсация от 0 до 1 и обратно
		pulseAmount = 0.5f + 0.5f * (float)cos( 2.0f * 3.14159265358979323846f * ( uis.realtime % 1000 ) / 1000.0f );
		
		// Интерполяция между исходным цветом и целевым
		pulseColor[0] = color[0] + (targetR - color[0]) * pulseAmount;
		pulseColor[1] = color[1] + (targetG - color[1]) * pulseAmount;
		pulseColor[2] = color[2] + (targetB - color[2]) * pulseAmount;
		pulseColor[3] = color[3];
		
		if ( style & UI_DROPSHADOW )
		{
			dropcolor[0] = dropcolor[1] = dropcolor[2] = 0.0f;
			dropcolor[3] = pulseColor[3];
			UI_DrawString2( x + 2, y + 2, str, dropcolor, charw, charh, parseColorCodes, qtrue, convertBlackToWhite );
		}
		
		UI_DrawString2( x, y, str, pulseColor, charw, charh, parseColorCodes, qfalse, convertBlackToWhite );
		return;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0.0f;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2( x + 2, y + 2, str, dropcolor, charw, charh, parseColorCodes, qtrue, convertBlackToWhite );
	}

	UI_DrawString2( x, y, str, drawcolor, charw, charh, parseColorCodes, qfalse, convertBlackToWhite );
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	UI_DrawStringCommon( x, y, str, style, color, qtrue, qfalse );
}

/*
=================
UI_DrawStringRaw
=================
*/
void UI_DrawStringRaw( int x, int y, const char* str, int style, vec4_t color )
{
	UI_DrawStringCommon( x, y, str, style, color, qfalse, qfalse );
}

/*
=================
UI_DrawStringServerBrowser
=================
*/
void UI_DrawStringServerBrowser( int x, int y, const char* str, int style, vec4_t color )
{
	char cleaned[1024];
	const char* src;
	char* dst;
	const char* lookahead;
	int remaining;

	src = str;
	dst = cleaned;
	remaining = sizeof( cleaned ) - 1;

	/* Copy string and cut single ^ at the end (or when no following char) */
	while ( *src && remaining > 0 )
	{
		if ( src[0] == '^' )
		{
			/* Check if this ^ is "lonely": only spaces (or nothing) after it */
			lookahead = src + 1;
			while ( *lookahead == ' ' )
			{
				lookahead++;
			}

			if ( *lookahead == '\0' )
			{
				/* Skip lone ^ and any trailing spaces */
				src = lookahead;
				continue;
			}
		}

		*dst++ = *src++;
		remaining--;
	}

	*dst = '\0';

	UI_DrawStringCommon( x, y, cleaned, style, color, qtrue, qtrue );
}


/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar( int x, int y, int ch, int style, vec4_t color )
{
	char	buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString( x, y, buff, style, color );
}


qboolean UI_IsFullscreen( void ) {
	if ( uis.activemenu && ( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}


static void NeedCDAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}

static void NeedCDKeyAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}


void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	Menu_Cache();

	switch ( menu ) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_NEED_CD:
		UI_ConfirmMenu( "Insert the CD", (voidfunc_f)NULL, NeedCDAction );
		return;
	case UIMENU_BAD_CD_KEY:
		UI_ConfirmMenu( "Bad CD Key", (voidfunc_f)NULL, NeedCDKeyAction );
		return;
	case UIMENU_INGAME:
		/*
		//GRank
		UI_RankingsMenu();
		return;
		*/
		trap_Cvar_Set( "cl_paused", "1" );
		UI_InGameMenu();
		return;
		
	// bk001204
	case UIMENU_TEAM:
	case UIMENU_POSTGAME:
	default:
#ifndef NDEBUG
	  Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu );
#endif
	  break;
	}
}


/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down ) {
	sfxHandle_t		s;

	UI_VideoCheck( trap_Milliseconds() );
	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key( key );
	else
		s = Menu_DefaultKey( uis.activemenu, key );

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
}


/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	int				i;
	menucommon_s*	m;

	if ( !uis.activemenu )
		return;

	// update virtual mouse cursor coordinates
	uis.cursorx += dx * uis.cursorScaleR;
	uis.cursory += dy * uis.cursorScaleR;
	
	// clamp virtual coordinates
	if ( uis.cursorx < uis.screenXmin )
		uis.cursorx = uis.screenXmin;
	else if ( uis.cursorx > uis.screenXmax )
		uis.cursorx = uis.screenXmax;

	if ( uis.cursory < uis.screenYmin )
		uis.cursory = uis.screenYmin;
	else if ( uis.cursory > uis.screenYmax )
		uis.cursory = uis.screenYmax;

	// region test the active menu items
	for ( i = 0; i < uis.activemenu->nitems; i++ )
	{
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) ||
			(uis.cursorx > m->right) ||
			(uis.cursory < m->top) ||
			(uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i || !(((menucommon_s*)(uis.activemenu->items[i]))->flags & QMF_HASMOUSEFOCUS))
		{
			Menu_SetCursor( uis.activemenu, i );
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if ( !(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}  

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}


char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}


/*
=================
UI_Cache
=================
*/
void UI_Cache_f( void ) {
	MainMenu_Cache();
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerModel_Cache();
	PlayerSettings_Cache();
	Controls_Cache();
	Demos_Cache();
	UI_CinematicsMenu_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
	ServerOptions_Cache();
	DriverInfo_Cache();
	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	UI_SPLevelMenu_Cache();
	UI_SPSkillMenu_Cache();
	UI_SPPostgameMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();
//	UI_LoadConfig_Cache();
//	UI_SaveConfigMenu_Cache();
	UI_BotSelectMenu_Cache();
	UI_CDKeyMenu_Cache();
	UI_ModsMenu_Cache();
}


/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	Menu_Cache();

	if ( Q_stricmp (cmd, "levelselect") == 0 ) {
		UI_SPLevelMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "postgame") == 0 ) {
		UI_SPPostgameMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cinematics") == 0 ) {
		UI_CinematicsMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		UI_TeamOrdersMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "iamacheater") == 0 ) {
		UI_SPUnlock_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "iamamonkey") == 0 ) {
		UI_SPUnlockMedals_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "fonts") == 0 ) {
		UI_Fonts_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cdkey") == 0 ) {
		UI_CDKeyMenu_f();
		return qtrue;
	}

	return qfalse;
}


/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}


/*
=================
UI_Init
=================
*/
void UI_Init( void ) {
	int start = trap_Milliseconds();

	UI_RegisterCvars();

	UI_InitGameinfo();

	// cache redundant calulations
	UI_VideoCheck( -99999 );

	uis.menuBgStyle = 1;
	uis.menuBgStyleModCount = -1;

	/* Theme colors: defaults then load from cvars once */
	Vector4Copy( color_red, uis.themeColor );
	uis.themeColor2[0] = 1.0f;
	uis.themeColor2[1] = 0.43f;
	uis.themeColor2[2] = 0.0f;
	uis.themeColor2[3] = 1.0f;
	uis.selectColor[0] = 1.0f;
	uis.selectColor[1] = 1.0f;
	uis.selectColor[2] = 0.0f;
	uis.selectColor[3] = 1.0f;
	uis.bannerColor[0] = 1.0f;
	uis.bannerColor[1] = 1.0f;
	uis.bannerColor[2] = 1.0f;
	uis.bannerColor[3] = 1.0f;
	UI_InitThemeColors();

	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp     = 0;

	trap_Print( va( "UI loaded in %d ms\n", trap_Milliseconds() - start ) );
}


/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
	*x = *x * uis.scale + uis.biasX;
	*y = *y * uis.scale + uis.biasY;
	*w *= uis.scale;
	*h *= uis.scale;
}


void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}


static void UI_DrawCursor( float x, float y, float w, float h ) {

	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, uis.cursor );
}


/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );

	trap_R_DrawStretchPic( x, y, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y, 1, height, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	trap_R_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	trap_UpdateScreen();
}

/*
=================
UI_Refresh
=================
*/
void UI_Refresh( int realtime )
{
	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	UI_UpdateCvars();
	UI_InitThemeColors();
	UI_VideoCheck( realtime );

	Vector4Copy( uis.themeColor2, text_color_normal );
	Vector4Copy( uis.themeColor2, color_orange );
	Vector4Copy( uis.themeColor2, listbar_color );
	listbar_color[3] = 0.30f;
	Vector4Copy( uis.selectColor, text_color_highlight );

	if ( uis.activemenu )
	{
		if (uis.activemenu->fullscreen)
		{
			/* draw the background */
			{
				int bgStyle = UI_GetMenuBgStyle();
				qboolean usePrimaryPicture;

				if ( bgStyle == 2 ) {
					/* Style 2: use first picture everywhere */
					usePrimaryPicture = qtrue;
				} else {
					/* Style 1: first picture only on main page (menus with showlogo), others use second */
					usePrimaryPicture = ( uis.activemenu->showlogo ? qtrue : qfalse );
				}

				UI_DrawMenuBackground( usePrimaryPicture );
			}

			{
				float d = ui_menuDarken.value;
				if ( d > 0.0f ) {
					float darkenColor[4];
					darkenColor[0] = 0.0f;
					darkenColor[1] = 0.0f;
					darkenColor[2] = 0.0f;
					darkenColor[3] = ( d > 1.0f ) ? 1.0f : d;
					trap_R_SetColor( darkenColor );
					trap_R_DrawStretchPic( 0, 0, (float)uis.glconfig.vidWidth, (float)uis.glconfig.vidHeight, 0, 0, 1, 1, uis.whiteShader );
					trap_R_SetColor( NULL );
				}
			}
		}

		if (uis.activemenu->draw)
			uis.activemenu->draw();
		else
			Menu_Draw( uis.activemenu );

		if( uis.firstdraw ) {
			UI_MouseEvent( 0, 0 );
			uis.firstdraw = qfalse;
		}
	}

	// draw cursor
	UI_SetColor( NULL );
	UI_DrawCursor( uis.cursorx-16, uis.cursory-16, 32, 32 );

#ifndef NDEBUG
	if ( uis.debug )
	{
		// cursor coordinates
		UI_DrawString( 0, 0, va( "(%1.1f,%1.1f)", uis.cursorx, uis.cursory ), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		trap_S_StartLocalSound( menu_in_sound, CHAN_LOCAL_SOUND );
		m_entersound = qfalse;
	}
}

void UI_DrawTextBox (int x, int y, int width, int lines)
{
	float offsetX;
	float offsetY;
	float totalWidth;
	float totalHeight;

	offsetX = 0.5f * (float)BIGCHAR_WIDTH;
	offsetY = 0.5f * (float)BIGCHAR_HEIGHT;
	totalWidth = (float)( width + 1 ) * (float)BIGCHAR_WIDTH;
	totalHeight = (float)( lines + 1 ) * (float)BIGCHAR_HEIGHT;

	UI_FillRect( x + offsetX, y + offsetY, totalWidth, totalHeight, colorBlack );
	UI_DrawRect( x + offsetX, y + offsetY, totalWidth, totalHeight, colorWhite );
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x ||
		uis.cursory < y ||
		uis.cursorx > x+width ||
		uis.cursory > y+height)
		return qfalse;

	return qtrue;
}
