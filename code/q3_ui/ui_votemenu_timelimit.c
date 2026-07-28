/*
===========================================================================
CALL VOTE timelimit — values from server votelimits cmd (ui_voteTimelimits).
===========================================================================
*/

#include "ui_local.h"

#define VOTEMENU_BACK0          "menu/art/back_0"
#define VOTEMENU_BACK1          "menu/art/back_1"
#define ART_FIGHT0		"menu/art/accept_0"
#define ART_FIGHT1		"menu/art/accept_1"
#define ART_BACKGROUND          "menu/art/addbotframe"

static char* votemenu_Timelmit_artlist[] =
{
	VOTEMENU_BACK0,
	VOTEMENU_BACK1,
	ART_FIGHT0,
	ART_FIGHT1,
	NULL
};

#define ID_BACK     100
#define ID_GO       101
#define ID_VALUE0   110

#define TimeLIMIT_MENU_VERTICAL_SPACING	28
#define TimeLIMIT_LIST_CENTER_Y		196
#define VOTE_LIMIT_MAX_OPTIONS 8

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	go;
	menutext_s      bLimit[VOTE_LIMIT_MAX_OPTIONS];
	char            labels[VOTE_LIMIT_MAX_OPTIONS][32];
	int             values[VOTE_LIMIT_MAX_OPTIONS];
	int             numValues;
	int selection;
} votemenu_t;

static votemenu_t	s_votemenu_Timelmit;

void UI_VoteTimelimitMenuInternal( void );

static int VoteTimelimit_ParseList( void )
{
	char buf[MAX_INFO_STRING];
	char *p;
	char token[16];
	int n;
	int i;

	trap_Cvar_VariableStringBuffer( "ui_voteTimelimits", buf, sizeof( buf ) );
	s_votemenu_Timelmit.numValues = 0;
	if ( !buf[0] ) {
		return 0;
	}

	p = buf;
	while ( *p && s_votemenu_Timelmit.numValues < VOTE_LIMIT_MAX_OPTIONS ) {
		while ( *p == '/' || *p == ' ' ) {
			p++;
		}
		if ( !*p ) {
			break;
		}
		n = 0;
		while ( *p >= '0' && *p <= '9' && n < (int)sizeof( token ) - 1 ) {
			token[n++] = *p++;
		}
		token[n] = '\0';
		if ( n > 0 ) {
			i = s_votemenu_Timelmit.numValues;
			s_votemenu_Timelmit.values[i] = atoi( token );
			Com_sprintf( s_votemenu_Timelmit.labels[i], sizeof( s_votemenu_Timelmit.labels[i] ),
				"%i minutes", s_votemenu_Timelmit.values[i] );
			s_votemenu_Timelmit.numValues++;
		}
		while ( *p && *p != '/' ) {
			p++;
		}
	}
	return s_votemenu_Timelmit.numValues;
}

static void VoteMenu_Timelimit_Confirm( void )
{
	int idx;

	idx = s_votemenu_Timelmit.selection - ID_VALUE0;
	if ( idx < 0 || idx >= s_votemenu_Timelmit.numValues ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote timelimit %i", s_votemenu_Timelmit.values[idx] ) );
	UI_PopMenu();
	UI_PopMenu();
}

static void VoteMenu_Timelimit_Event( void* ptr, int event )
{
	int id;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	id = ((menucommon_s*)ptr)->id;
	switch ( id )
	{
		case ID_BACK:
			UI_PopMenu();
			break;
		case ID_GO:
			VoteMenu_Timelimit_Confirm();
			break;
		default:
			if ( s_votemenu_Timelmit.selection == id ) {
				VoteMenu_Timelimit_Confirm();
			} else {
				s_votemenu_Timelmit.selection = id;
				UI_VoteTimelimitMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

static void setTimeMenutext(menutext_s *menu,int y,int id,char *text) {
	menu->generic.type            = MTYPE_PTEXT;
	menu->color               = uis.themeColor;
	menu->generic.flags       = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	if ( s_votemenu_Timelmit.selection == id ) {
		menu->color       = uis.themeColor2;
	}
	menu->generic.x           = 320;
	menu->generic.y           = y;
	menu->generic.id          = id;
	menu->generic.callback    = VoteMenu_Timelimit_Event;
	menu->string              = text;
	menu->style               = UI_CENTER|UI_SMALLFONT;
}

static void VoteMenu_Timelimit_Cache( void )
{
	int	i;

	for (i=0; ;i++)
	{
		if (!votemenu_Timelmit_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(votemenu_Timelmit_artlist[i]);
	}
}

static void UI_VoteMenu_Timelimit_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE - TIMELIMIT", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );
	Menu_Draw( &s_votemenu_Timelmit.menu );
}

void UI_VoteTimelimitMenuInternal( void )
{
	int y;
	int i;

	VoteMenu_Timelimit_Cache();

	s_votemenu_Timelmit.menu.wrapAround = qtrue;
	s_votemenu_Timelmit.menu.fullscreen = qfalse;
	s_votemenu_Timelmit.menu.draw = UI_VoteMenu_Timelimit_Draw;

	s_votemenu_Timelmit.banner.generic.type  = MTYPE_BTEXT;
	s_votemenu_Timelmit.banner.generic.x	  = 320;
	s_votemenu_Timelmit.banner.generic.y	  = 16;
	s_votemenu_Timelmit.banner.string		  = "CALL VOTE - TIMELIMIT";
	s_votemenu_Timelmit.banner.color	      = uis.themeColor;
	s_votemenu_Timelmit.banner.style	      = UI_CENTER;

	if ( s_votemenu_Timelmit.numValues > 0 ) {
		y = TimeLIMIT_LIST_CENTER_Y
			- ( ( s_votemenu_Timelmit.numValues - 1 ) * TimeLIMIT_MENU_VERTICAL_SPACING ) / 2;
	} else {
		y = TimeLIMIT_LIST_CENTER_Y;
	}
	for ( i = 0; i < s_votemenu_Timelmit.numValues; i++ ) {
		setTimeMenutext( &s_votemenu_Timelmit.bLimit[i], y, ID_VALUE0 + i, s_votemenu_Timelmit.labels[i] );
		y += TimeLIMIT_MENU_VERTICAL_SPACING;
	}

	s_votemenu_Timelmit.back.generic.type	   = MTYPE_BITMAP;
	s_votemenu_Timelmit.back.generic.name     = VOTEMENU_BACK0;
	s_votemenu_Timelmit.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_Timelmit.back.generic.callback = VoteMenu_Timelimit_Event;
	s_votemenu_Timelmit.back.generic.id	   = ID_BACK;
	s_votemenu_Timelmit.back.generic.x		   = 320-128;
	s_votemenu_Timelmit.back.generic.y		   = 256+128-64;
	s_votemenu_Timelmit.back.width  		   = 128;
	s_votemenu_Timelmit.back.height  		   = 64;
	s_votemenu_Timelmit.back.focuspic         = VOTEMENU_BACK1;

	s_votemenu_Timelmit.go.generic.type	   = MTYPE_BITMAP;
	s_votemenu_Timelmit.go.generic.name     = ART_FIGHT0;
	s_votemenu_Timelmit.go.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_Timelmit.go.generic.callback = VoteMenu_Timelimit_Event;
	s_votemenu_Timelmit.go.generic.id	   = ID_GO;
	s_votemenu_Timelmit.go.generic.x		   = 320;
	s_votemenu_Timelmit.go.generic.y		   = 256+128-64;
	s_votemenu_Timelmit.go.width  		   = 128;
	s_votemenu_Timelmit.go.height  		   = 64;
	s_votemenu_Timelmit.go.focuspic         = ART_FIGHT1;
	if ( s_votemenu_Timelmit.numValues < 1 ) {
		s_votemenu_Timelmit.go.generic.flags |= QMF_INACTIVE|QMF_GRAYED;
	}
}

void UI_VoteTimelimitMenu( void ) {
	int i;

	memset( &s_votemenu_Timelmit, 0, sizeof(votemenu_t) );
	VoteTimelimit_ParseList();
	UI_VoteTimelimitMenuInternal();

	Menu_AddItem( &s_votemenu_Timelmit.menu, (void*) &s_votemenu_Timelmit.banner );
	Menu_AddItem( &s_votemenu_Timelmit.menu, (void*) &s_votemenu_Timelmit.back );
	Menu_AddItem( &s_votemenu_Timelmit.menu, (void*) &s_votemenu_Timelmit.go );
	for ( i = 0; i < s_votemenu_Timelmit.numValues; i++ ) {
		Menu_AddItem( &s_votemenu_Timelmit.menu, (void*) &s_votemenu_Timelmit.bLimit[i] );
	}

	UI_PushMenu( &s_votemenu_Timelmit.menu );
}
