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

#define VOTEMENU_BACK0          "menu/art/back_0"
#define VOTEMENU_BACK1          "menu/art/back_1"
#define ART_FIGHT0		"menu/art/accept_0"
#define ART_FIGHT1		"menu/art/accept_1"
#define ART_BACKGROUND          "menu/art/addbotframe"

static char* votemenu_custom_artlist[] =
{
	VOTEMENU_BACK0,
	VOTEMENU_BACK1,
        ART_FIGHT0,
        ART_FIGHT1,
	NULL
};

#define ID_BACK     100
#define ID_GO       101
#define ID_CUSTOM0        102
//next 11 for ID_CUSTOM1-CUSTOM11

#define CUSTOM_MENU_VERTICAL_SPACING	19
#define CUSTOM_MENU_MAX_ENTRIES 12
#define CUSTOM_LIST_CENTER_Y		202

typedef struct
{
    menuframework_s	menu;
    menutext_s		banner;
    menubitmap_s	back;
    menubitmap_s	go;

    //Buttons:
    menutext_s      bEntry[CUSTOM_MENU_MAX_ENTRIES];

    //The text:
    char            text[CUSTOM_MENU_MAX_ENTRIES][32];

    int selection;
} votemenu_t;

static votemenu_t	s_votemenu_custom;

void UI_VoteCustomMenuInternal( void );

static void VoteMenu_custom_Confirm( void )
{
	if (!s_votemenu_custom.selection) {
		return;
	}
	trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote custom %s\n",
		s_votemenu_custom.bEntry[s_votemenu_custom.selection-ID_CUSTOM0].string));
	UI_PopMenu();
	UI_PopMenu();
}

/*
=================
VoteMenu_custom_Event
=================
*/
static void VoteMenu_custom_Event( void* ptr, int event )
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
			VoteMenu_custom_Confirm();
			break;
		default:
			if(s_votemenu_custom.selection == id) {
				VoteMenu_custom_Confirm();
			} else {
				s_votemenu_custom.selection = id;
				UI_VoteCustomMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

static void setCustomMenutext(menutext_s *menu,int y,int id,char *text) {
    menu->generic.type            = MTYPE_PTEXT;
    menu->color               = uis.themeColor;
    menu->generic.flags       = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
    if(s_votemenu_custom.selection == id)
         menu->color       = uis.themeColor2;
    menu->generic.x           = 320;
    menu->generic.y           = y;
    menu->generic.id          = id;
    menu->generic.callback    = VoteMenu_custom_Event;
    menu->string              = text;
    menu->style               = UI_CENTER|UI_SMALLFONT;
}

/*
=================
VoteMenu_Custom_Cache
=================
*/
static void VoteMenu_Custom_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!votemenu_custom_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(votemenu_custom_artlist[i]);
	}
}

/*
=================
UI_VoteMenu_Custom_Draw
=================
*/
static void UI_VoteMenu_Custom_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE CUSTOM", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );

	// standard menu drawing
	Menu_Draw( &s_votemenu_custom.menu );
}

/*
=================
UI_VoteCustomMenuInternal
 *Used then forcing a redraw
=================
*/
void UI_VoteCustomMenuInternal( void )
{
    int y,i,n;
    char serverinfo[MAX_INFO_STRING], custominfo[MAX_INFO_STRING], *token,*pointer;

    VoteMenu_Custom_Cache();

    trap_GetConfigString(CS_SERVERINFO, serverinfo, sizeof(serverinfo));
    Q_strncpyz(custominfo, Info_ValueForKey(serverinfo, "customvotes"), sizeof(custominfo));

    s_votemenu_custom.menu.wrapAround = qtrue;
    s_votemenu_custom.menu.fullscreen = qfalse;
    s_votemenu_custom.menu.draw = UI_VoteMenu_Custom_Draw;

    s_votemenu_custom.banner.generic.type  = MTYPE_BTEXT;
    s_votemenu_custom.banner.generic.x	  = 320;
    s_votemenu_custom.banner.generic.y	  = 16;
    s_votemenu_custom.banner.string		  = "CALL VOTE CUSTOM";
    s_votemenu_custom.banner.color	      = uis.themeColor;
    s_votemenu_custom.banner.style	      = UI_CENTER;

    pointer = custominfo;

    n = 0;
    for(i=0;i<CUSTOM_MENU_MAX_ENTRIES;i++) {
        token = COM_Parse( &pointer );
        Q_strncpyz(s_votemenu_custom.text[i],token,sizeof(s_votemenu_custom.text[0]));
        if(s_votemenu_custom.text[i][0]) {
            n++;
        }
    }

    if ( n > 0 ) {
        y = CUSTOM_LIST_CENTER_Y - ( ( n - 1 ) * CUSTOM_MENU_VERTICAL_SPACING ) / 2;
    } else {
        y = CUSTOM_LIST_CENTER_Y;
    }

    for(i=0;i<CUSTOM_MENU_MAX_ENTRIES;i++) {
        if(!s_votemenu_custom.text[i][0]) {
            continue;
        }
        setCustomMenutext(&s_votemenu_custom.bEntry[i],y,ID_CUSTOM0+i,s_votemenu_custom.text[i]);
        y+=CUSTOM_MENU_VERTICAL_SPACING;
    }

    s_votemenu_custom.back.generic.type	   = MTYPE_BITMAP;
    s_votemenu_custom.back.generic.name     = VOTEMENU_BACK0;
    s_votemenu_custom.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_custom.back.generic.callback = VoteMenu_custom_Event;
    s_votemenu_custom.back.generic.id	   = ID_BACK;
    s_votemenu_custom.back.generic.x		   = 320-128;
    s_votemenu_custom.back.generic.y		   = 256+128-64;
    s_votemenu_custom.back.width  		   = 128;
    s_votemenu_custom.back.height  		   = 64;
    s_votemenu_custom.back.focuspic         = VOTEMENU_BACK1;

    s_votemenu_custom.go.generic.type	   = MTYPE_BITMAP;
    s_votemenu_custom.go.generic.name     = ART_FIGHT0;
    s_votemenu_custom.go.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_votemenu_custom.go.generic.callback = VoteMenu_custom_Event;
    s_votemenu_custom.go.generic.id	   = ID_GO;
    s_votemenu_custom.go.generic.x		   = 320;
    s_votemenu_custom.go.generic.y		   = 256+128-64;
    s_votemenu_custom.go.width  		   = 128;
    s_votemenu_custom.go.height  		   = 64;
    s_votemenu_custom.go.focuspic         = ART_FIGHT1;
}

/*
=================
UI_VoteCustomMenu
 *Called from outside
=================
*/
void UI_VoteCustomMenu( void ) {
        int i;
        // zero set all our globals
	memset( &s_votemenu_custom, 0 ,sizeof(votemenu_t) );
        
        UI_VoteCustomMenuInternal();

	Menu_AddItem( &s_votemenu_custom.menu, (void*) &s_votemenu_custom.banner );
	Menu_AddItem( &s_votemenu_custom.menu, (void*) &s_votemenu_custom.back );
        Menu_AddItem( &s_votemenu_custom.menu, (void*) &s_votemenu_custom.go );
        for(i=0;i<CUSTOM_MENU_MAX_ENTRIES;i++) {
            if(!s_votemenu_custom.text[i][0]) {
                continue;
            }
            Menu_AddItem( &s_votemenu_custom.menu, (void*) &s_votemenu_custom.bEntry[i] );
        }

	UI_PushMenu( &s_votemenu_custom.menu );
}
