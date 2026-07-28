/*
===========================================================================
CALL VOTE fraglimit — values from server votelimits cmd (ui_voteFraglimits).
===========================================================================
*/

#include "ui_local.h"

#define VOTEMENU_BACK0          "menu/art/back_0"
#define VOTEMENU_BACK1          "menu/art/back_1"
#define ART_FIGHT0		"menu/art/accept_0"
#define ART_FIGHT1		"menu/art/accept_1"
#define ART_BACKGROUND          "menu/art/addbotframe"

static char* votemenu_fraglmit_artlist[] =
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

#define FRAGLIMIT_MENU_VERTICAL_SPACING	28
#define FRAGLIMIT_LIST_CENTER_Y		196
#define VOTE_LIMIT_MAX_OPTIONS 8

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	go;
	menutext_s      bLimit[VOTE_LIMIT_MAX_OPTIONS];
	char            labels[VOTE_LIMIT_MAX_OPTIONS][16];
	int             values[VOTE_LIMIT_MAX_OPTIONS];
	int             numValues;
	int selection;
} votemenu_t;

static votemenu_t	s_votemenu_fraglmit;

void UI_VoteFraglimitMenuInternal( void );

static int VoteFraglimit_ParseList( void )
{
	char buf[MAX_INFO_STRING];
	char *p;
	char token[16];
	int n;
	int i;

	trap_Cvar_VariableStringBuffer( "ui_voteFraglimits", buf, sizeof( buf ) );
	s_votemenu_fraglmit.numValues = 0;
	if ( !buf[0] ) {
		return 0;
	}

	p = buf;
	while ( *p && s_votemenu_fraglmit.numValues < VOTE_LIMIT_MAX_OPTIONS ) {
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
			i = s_votemenu_fraglmit.numValues;
			s_votemenu_fraglmit.values[i] = atoi( token );
			Com_sprintf( s_votemenu_fraglmit.labels[i], sizeof( s_votemenu_fraglmit.labels[i] ), "%i",
				s_votemenu_fraglmit.values[i] );
			s_votemenu_fraglmit.numValues++;
		}
		while ( *p && *p != '/' ) {
			p++;
		}
	}
	return s_votemenu_fraglmit.numValues;
}

static void VoteMenu_Fraglimit_Confirm( void )
{
	int idx;

	idx = s_votemenu_fraglmit.selection - ID_VALUE0;
	if ( idx < 0 || idx >= s_votemenu_fraglmit.numValues ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote fraglimit %i", s_votemenu_fraglmit.values[idx] ) );
	UI_PopMenu();
	UI_PopMenu();
}

static void VoteMenu_Fraglimit_Event( void* ptr, int event )
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
			VoteMenu_Fraglimit_Confirm();
			break;
		default:
			if ( s_votemenu_fraglmit.selection == id ) {
				VoteMenu_Fraglimit_Confirm();
			} else {
				s_votemenu_fraglmit.selection = id;
				UI_VoteFraglimitMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

static void setMenutext(menutext_s *menu,int y,int id,char *text) {
	menu->generic.type            = MTYPE_PTEXT;
	menu->color               = uis.themeColor;
	menu->generic.flags       = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	if ( s_votemenu_fraglmit.selection == id ) {
		menu->color       = uis.themeColor2;
	}
	menu->generic.x           = 320;
	menu->generic.y           = y;
	menu->generic.id          = id;
	menu->generic.callback    = VoteMenu_Fraglimit_Event;
	menu->string              = text;
	menu->style               = UI_CENTER|UI_SMALLFONT;
}

static void VoteMenu_Fraglimit_Cache( void )
{
	int	i;

	for (i=0; ;i++)
	{
		if (!votemenu_fraglmit_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(votemenu_fraglmit_artlist[i]);
	}
}

static void UI_VoteMenu_Fraglimit_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE - FRAGLIMIT", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );
	Menu_Draw( &s_votemenu_fraglmit.menu );
}

void UI_VoteFraglimitMenuInternal( void )
{
	int y;
	int i;

	VoteMenu_Fraglimit_Cache();

	s_votemenu_fraglmit.menu.wrapAround = qtrue;
	s_votemenu_fraglmit.menu.fullscreen = qfalse;
	s_votemenu_fraglmit.menu.draw = UI_VoteMenu_Fraglimit_Draw;

	s_votemenu_fraglmit.banner.generic.type  = MTYPE_BTEXT;
	s_votemenu_fraglmit.banner.generic.x	  = 320;
	s_votemenu_fraglmit.banner.generic.y	  = 16;
	s_votemenu_fraglmit.banner.string		  = "CALL VOTE - FRAGLIMIT";
	s_votemenu_fraglmit.banner.color	      = uis.themeColor;
	s_votemenu_fraglmit.banner.style	      = UI_CENTER;

	if ( s_votemenu_fraglmit.numValues > 0 ) {
		y = FRAGLIMIT_LIST_CENTER_Y
			- ( ( s_votemenu_fraglmit.numValues - 1 ) * FRAGLIMIT_MENU_VERTICAL_SPACING ) / 2;
	} else {
		y = FRAGLIMIT_LIST_CENTER_Y;
	}
	for ( i = 0; i < s_votemenu_fraglmit.numValues; i++ ) {
		setMenutext( &s_votemenu_fraglmit.bLimit[i], y, ID_VALUE0 + i, s_votemenu_fraglmit.labels[i] );
		y += FRAGLIMIT_MENU_VERTICAL_SPACING;
	}

	s_votemenu_fraglmit.back.generic.type	   = MTYPE_BITMAP;
	s_votemenu_fraglmit.back.generic.name     = VOTEMENU_BACK0;
	s_votemenu_fraglmit.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_fraglmit.back.generic.callback = VoteMenu_Fraglimit_Event;
	s_votemenu_fraglmit.back.generic.id	   = ID_BACK;
	s_votemenu_fraglmit.back.generic.x		   = 320-128;
	s_votemenu_fraglmit.back.generic.y		   = 256+128-64;
	s_votemenu_fraglmit.back.width  		   = 128;
	s_votemenu_fraglmit.back.height  		   = 64;
	s_votemenu_fraglmit.back.focuspic         = VOTEMENU_BACK1;

	s_votemenu_fraglmit.go.generic.type	   = MTYPE_BITMAP;
	s_votemenu_fraglmit.go.generic.name     = ART_FIGHT0;
	s_votemenu_fraglmit.go.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_fraglmit.go.generic.callback = VoteMenu_Fraglimit_Event;
	s_votemenu_fraglmit.go.generic.id	   = ID_GO;
	s_votemenu_fraglmit.go.generic.x		   = 320;
	s_votemenu_fraglmit.go.generic.y		   = 256+128-64;
	s_votemenu_fraglmit.go.width  		   = 128;
	s_votemenu_fraglmit.go.height  		   = 64;
	s_votemenu_fraglmit.go.focuspic         = ART_FIGHT1;
	if ( s_votemenu_fraglmit.numValues < 1 ) {
		s_votemenu_fraglmit.go.generic.flags |= QMF_INACTIVE|QMF_GRAYED;
	}
}

void UI_VoteFraglimitMenu( void ) {
	int i;

	memset( &s_votemenu_fraglmit, 0, sizeof(votemenu_t) );
	VoteFraglimit_ParseList();
	UI_VoteFraglimitMenuInternal();

	Menu_AddItem( &s_votemenu_fraglmit.menu, (void*) &s_votemenu_fraglmit.banner );
	Menu_AddItem( &s_votemenu_fraglmit.menu, (void*) &s_votemenu_fraglmit.back );
	Menu_AddItem( &s_votemenu_fraglmit.menu, (void*) &s_votemenu_fraglmit.go );
	for ( i = 0; i < s_votemenu_fraglmit.numValues; i++ ) {
		Menu_AddItem( &s_votemenu_fraglmit.menu, (void*) &s_votemenu_fraglmit.bLimit[i] );
	}

	UI_PushMenu( &s_votemenu_fraglmit.menu );
}
