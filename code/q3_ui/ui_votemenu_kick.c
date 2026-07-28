/*
===========================================================================
Copyright (C) 2009 Poul Sander

This file is part of the Open Arena source code.

Open Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_FIGHT0			"menu/art/accept_0"
#define ART_FIGHT1			"menu/art/accept_1"
#define ART_BACKGROUND                  "menu/art/addbotframe"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWUP			"menu/art/arrows_vert_top"
#define ART_ARROWDOWN                   "menu/art/arrows_vert_bot"

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_UP				13
#define ID_DOWN				14
#define ID_PLAYERNAME0			20
#define ID_PLAYERNAME1			21
#define ID_PLAYERNAME2			22
#define ID_PLAYERNAME3			23
#define ID_PLAYERNAME4			24
#define ID_PLAYERNAME5			25
#define ID_PLAYERNAME6			26
#define ID_PLAYERNAME7			27
#define ID_PLAYERNAME8			28
#define ID_PLAYERNAME9			29
#define SIZE_OF_LIST                    10
//#define ID_PLAYERNAME10			30

#define SIZE_OF_NAME                    32

#define VOTEKICK_MENU_VERTICAL_SPACING	20
#define VOTEKICK_LIST_CENTER_Y		188

typedef struct {
    int id;
    char name[SIZE_OF_NAME];
} player;

typedef struct {
	menuframework_s	menu;
	menubitmap_s	arrows;
        menutext_s		banner;
	menubitmap_s	up;
	menubitmap_s	down;
	menutext_s	players[SIZE_OF_LIST];
	menubitmap_s	go;
	menubitmap_s	back;

	int		numPlayers;
	int		selected;
        int             startIndex;
        player          players_profiles[MAX_CLIENTS];
} votemenu_kick_t;

static votemenu_kick_t	s_votemenu_kick;

void UI_VoteKickMenuInternal( void );

static void populatePlayerList( void ) {
    int i;
    char playerinfo[MAX_INFO_STRING];
    s_votemenu_kick.numPlayers = 0;

    for(i=0;i<MAX_CLIENTS;i++) {
        trap_GetConfigString( CS_PLAYERS + i, playerinfo, MAX_INFO_STRING );
        if(strlen(playerinfo) ) {
            s_votemenu_kick.players_profiles[s_votemenu_kick.numPlayers].id = i;
            Q_strncpyz( s_votemenu_kick.players_profiles[s_votemenu_kick.numPlayers].name, Info_ValueForKey( playerinfo, "n" ), SIZE_OF_NAME - 2);
            s_votemenu_kick.numPlayers++;
        }
    }
}

static void VoteKickMenu_Confirm( void )
{
	if(!s_votemenu_kick.selected)
		return;
	trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote clientkick %d",s_votemenu_kick.players_profiles[(s_votemenu_kick.selected-20)+s_votemenu_kick.startIndex].id) );
	UI_PopMenu();
	UI_PopMenu();
}

/*
=================
VoteKickMenu_Event
=================
*/
static void VoteKickMenu_Event( void* ptr, int event )
{
	int id;

	if (event != QM_ACTIVATED) {
		return;
	}

	id = ((menucommon_s*)ptr)->id;
	switch (id)
	{
		case ID_BACK:
			UI_PopMenu();
			break;
		case ID_GO:
			VoteKickMenu_Confirm();
			break;
		default:
			if(s_votemenu_kick.selected == id) {
				VoteKickMenu_Confirm();
			} else {
				s_votemenu_kick.selected = id;
				UI_VoteKickMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

/*
=================
UI_VoteKickMenu_UpEvent
=================
*/
static void UI_VoteKickMenu_UpEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if( s_votemenu_kick.startIndex > 0 ) {
            s_votemenu_kick.startIndex--;
	}

        UI_VoteKickMenuInternal();
}

/*
=================
UI_VoteKickMenu_DownEvent
=================
*/
static void UI_VoteKickMenu_DownEvent( void* ptr, int event ) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if( s_votemenu_kick.startIndex+SIZE_OF_LIST < s_votemenu_kick.numPlayers ) {
            s_votemenu_kick.startIndex++;
	}

        UI_VoteKickMenuInternal();
}

/*
=================
UI_VoteKickMenu_Draw
=================
*/
static void UI_VoteKickMenu_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE KICK", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );

	// standard menu drawing
	Menu_Draw( &s_votemenu_kick.menu );
}

/*
=================
VoteKickMenu_Cache
=================
*/
static void VoteKickMenu_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FIGHT0 );
	trap_R_RegisterShaderNoMip( ART_FIGHT1 );
	trap_R_RegisterShaderNoMip( ART_BACKGROUND );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDOWN );
}

static void setKickMenutext(menutext_s *menu,int y,int id,qboolean active,char *text) {
    menu->generic.type            = MTYPE_PTEXT;
    menu->color               = uis.themeColor;
    menu->generic.flags       = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
    if(!active)
        menu->generic.flags   |= QMF_INACTIVE|QMF_HIDDEN;
    else
    if(s_votemenu_kick.selected == id)
         menu->color       = uis.themeColor2;
    menu->generic.x           = 320;
    menu->generic.y           = y;
    menu->generic.id          = id;
    menu->generic.callback    = VoteKickMenu_Event;
    menu->string              = text;
    menu->style               = UI_CENTER|UI_SMALLFONT;
}

/*
=================
UI_VoteKickMenuInternal
 *Used then forcing a redraw
=================
*/
void UI_VoteKickMenuInternal( void )
{
    int y;
    int i;
    int n;
    qboolean active;

    s_votemenu_kick.menu.wrapAround = qtrue;
    s_votemenu_kick.menu.fullscreen = qfalse;
    s_votemenu_kick.menu.draw = UI_VoteKickMenu_Draw;

    s_votemenu_kick.banner.generic.type  = MTYPE_BTEXT;
    s_votemenu_kick.banner.generic.x	  = 320;
    s_votemenu_kick.banner.generic.y	  = 16;
    s_votemenu_kick.banner.string		  = "CALL VOTE KICK";
    s_votemenu_kick.banner.color	      = uis.themeColor;
    s_votemenu_kick.banner.style	      = UI_CENTER;

    s_votemenu_kick.arrows.generic.type  = MTYPE_BITMAP;
    s_votemenu_kick.arrows.generic.name  = ART_ARROWS;
    s_votemenu_kick.arrows.generic.flags = QMF_INACTIVE;
    s_votemenu_kick.arrows.generic.x	 = 200;
    s_votemenu_kick.arrows.generic.y	 = 128;
    s_votemenu_kick.arrows.width  	     = 64;
    s_votemenu_kick.arrows.height  	     = 128;

    n = s_votemenu_kick.numPlayers - s_votemenu_kick.startIndex;
    if ( n > SIZE_OF_LIST ) {
        n = SIZE_OF_LIST;
    }
    if ( n < 0 ) {
        n = 0;
    }
    if ( n > 0 ) {
        y = VOTEKICK_LIST_CENTER_Y - ( ( n - 1 ) * VOTEKICK_MENU_VERTICAL_SPACING ) / 2;
    } else {
        y = VOTEKICK_LIST_CENTER_Y;
    }

    for ( i = 0; i < SIZE_OF_LIST; i++ ) {
        active = ( s_votemenu_kick.startIndex + i < s_votemenu_kick.numPlayers );
        if ( active ) {
            setKickMenutext( &s_votemenu_kick.players[i], y, ID_PLAYERNAME0 + i, qtrue,
                s_votemenu_kick.players_profiles[s_votemenu_kick.startIndex + i].name );
            y += VOTEKICK_MENU_VERTICAL_SPACING;
        } else {
            setKickMenutext( &s_votemenu_kick.players[i], 0, ID_PLAYERNAME0 + i, qfalse, "" );
        }
    }

    s_votemenu_kick.up.generic.type	    = MTYPE_BITMAP;
    s_votemenu_kick.up.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_kick.up.generic.x		= 200;
    s_votemenu_kick.up.generic.y		= 128;
    s_votemenu_kick.up.generic.id	    = ID_UP;
    s_votemenu_kick.up.generic.callback = UI_VoteKickMenu_UpEvent;
    s_votemenu_kick.up.width  		    = 64;
    s_votemenu_kick.up.height  		    = 64;
    s_votemenu_kick.up.focuspic         = ART_ARROWUP;

    s_votemenu_kick.down.generic.type	  = MTYPE_BITMAP;
    s_votemenu_kick.down.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_kick.down.generic.x		  = 200;
    s_votemenu_kick.down.generic.y		  = 128+64;
    s_votemenu_kick.down.generic.id	      = ID_DOWN;
    s_votemenu_kick.down.generic.callback = UI_VoteKickMenu_DownEvent;
    s_votemenu_kick.down.width  		  = 64;
    s_votemenu_kick.down.height  		  = 64;
    s_votemenu_kick.down.focuspic         = ART_ARROWDOWN;

    s_votemenu_kick.go.generic.type			= MTYPE_BITMAP;
    s_votemenu_kick.go.generic.name			= ART_FIGHT0;
    s_votemenu_kick.go.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_kick.go.generic.id			= ID_GO;
    s_votemenu_kick.go.generic.callback		= VoteKickMenu_Event;
    s_votemenu_kick.go.generic.x			= 320+128-128;
    s_votemenu_kick.go.generic.y			= 256+128-64;
    s_votemenu_kick.go.width  				= 128;
    s_votemenu_kick.go.height  				= 64;
    s_votemenu_kick.go.focuspic				= ART_FIGHT1;

    s_votemenu_kick.back.generic.type		= MTYPE_BITMAP;
    s_votemenu_kick.back.generic.name		= ART_BACK0;
    s_votemenu_kick.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_kick.back.generic.id			= ID_BACK;
    s_votemenu_kick.back.generic.callback	= VoteKickMenu_Event;
    s_votemenu_kick.back.generic.x			= 320-128;
    s_votemenu_kick.back.generic.y			= 256+128-64;
    s_votemenu_kick.back.width				= 128;
    s_votemenu_kick.back.height				= 64;
    s_votemenu_kick.back.focuspic			= ART_BACK1;
}

/*
=================
UI_VoteKickMenu
 *Called from outside
=================
*/
void UI_VoteKickMenu( void ) {
    int i;
    VoteKickMenu_Cache();
    memset( &s_votemenu_kick, 0 ,sizeof(votemenu_kick_t) );
    populatePlayerList();

    UI_VoteKickMenuInternal();

    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.banner );
    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.back );
    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.go );
    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.arrows );
    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.down );
    Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.up );
    for(i=0;i<SIZE_OF_LIST;i++)
        Menu_AddItem( &s_votemenu_kick.menu, (void*) &s_votemenu_kick.players[i] );

    UI_PushMenu( &s_votemenu_kick.menu );
}

