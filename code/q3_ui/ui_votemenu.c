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

#define VOTEMENU_BACK0      "menu/art/back_0"
#define VOTEMENU_BACK1      "menu/art/back_1"
#define ART_FIGHT0          "menu/art/accept_0"
#define ART_FIGHT1          "menu/art/accept_1"
#define ART_BACKGROUND      "menu/art/addbotframe"

static char* votemenu_artlist[] =
{
	VOTEMENU_BACK0,
	VOTEMENU_BACK1,
        ART_FIGHT0,
        ART_FIGHT1,
	NULL
};

#define ID_BACK     100
#define ID_GO       101
#define ID_NEXTMAP  102
#define ID_RESTART  103
#define ID_DOWARMUP 104
#define ID_MAP      105
#define ID_KICK     106
#define ID_FRAG     107
#define ID_TIME     108
#define ID_GAMETYPE 109
#define ID_SHUFFLE  111
#define ID_SETNEXTMAP 112

//This sorta dependend on number of vote options
#define VOTEMENU_MENU_VERTICAL_SPACING	22
#define VOTEMENU_LIST_CENTER_Y		197

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
        menubitmap_s	go;

        //Allowed votes:
        qboolean        map_restart;
        qboolean        nextmap;
        qboolean        g_doWarmup;
        qboolean        g_doWarmupEnabled;
        qboolean        clientkick;
        qboolean        map;
        qboolean        setnextmap;
        qboolean        gametype;
        qboolean        fraglimit;
        qboolean        timelimit;
        qboolean        shuffle;

        //Buttons:
        menutext_s      bMapRestart;
        menutext_s      bNextmap;
        menutext_s      bDoWarmup;
        menutext_s      bKick;
        menutext_s      bMap;
        menutext_s      bSetNextMap;
        menutext_s      bGametype;
        menutext_s      bTimelimit;
        menutext_s      bFraglimit;
        menutext_s      bShuffle;

        int             selection;
} votemenu_t;

static votemenu_t	s_votemenu;

void UI_VoteMenuMenuInternal( void );


/*
=================
VoteMenu_CheckVoteNames
=================
*/

static void VoteMenu_CheckVoteNames( void ) {
    char info[MAX_INFO_STRING];
    int voteflags;

    trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
    voteflags = atoi(Info_ValueForKey(info, "voteflags"));
    s_votemenu.map_restart = voteflags&VF_map_restart;
    s_votemenu.nextmap = voteflags&VF_nextmap;
    s_votemenu.map = voteflags&VF_map;
    s_votemenu.setnextmap = voteflags&VF_setnextmap;
    s_votemenu.gametype = (voteflags & (VF_g_gametype | VF_promode | VF_instagib | VF_quad | VF_freeze | VF_aim)) ? qtrue : qfalse;
    //We never use "kick" in menues, always clientkick
    s_votemenu.clientkick = voteflags&VF_clientkick;
    s_votemenu.g_doWarmup = voteflags&VF_g_doWarmup;
    s_votemenu.timelimit = voteflags&VF_timelimit;
    s_votemenu.fraglimit = voteflags&VF_fraglimit;
    s_votemenu.shuffle = voteflags&VF_shuffle;
}

/*
=================
VoteMenu_Event
=================
*/
static void VoteMenu_Confirm( void )
{
	switch(s_votemenu.selection) {
		case ID_NEXTMAP:
			trap_Cmd_ExecuteText( EXEC_APPEND, "callvote nextmap" );
			UI_PopMenu();
			break;
		case ID_RESTART:
			trap_Cmd_ExecuteText( EXEC_APPEND, "callvote map_restart" );
			UI_PopMenu();
			break;
		case ID_DOWARMUP:
			if(s_votemenu.g_doWarmupEnabled)
				trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_doWarmup 0" );
			else
				trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_doWarmup 1" );
			UI_PopMenu();
			break;
		case ID_SHUFFLE:
			trap_Cmd_ExecuteText( EXEC_APPEND, "callvote shuffle" );
			UI_PopMenu();
			break;
		case ID_FRAG:
			UI_VoteFraglimitMenu();
			break;
		case ID_TIME:
			UI_VoteTimelimitMenu();
			break;
		case ID_GAMETYPE:
			UI_VoteGametypeMenu();
			break;
		case ID_KICK:
			UI_VoteKickMenu();
			break;
		case ID_MAP:
			UI_VoteMapMenu();
			break;
		case ID_SETNEXTMAP:
			UI_VoteSetNextMapMenu();
			break;
	}
}

static void VoteMenu_Event( void* ptr, int event )
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
			VoteMenu_Confirm();
			break;
		default:
			if(s_votemenu.selection == id) {
				VoteMenu_Confirm();
			} else {
				s_votemenu.selection = id;
				UI_VoteMenuMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

/*
=================
VoteMenu_Cache
=================
*/
static void VoteMenu_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!votemenu_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(votemenu_artlist[i]);
	}

        //Check all names
        VoteMenu_CheckVoteNames();
}

/*
=================
UI_VoteMenu_Draw
=================
*/
static void UI_VoteMenu_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );

	// standard menu drawing
	Menu_Draw( &s_votemenu.menu );
}

static void VoteMenu_SetMenutext( menutext_s *menu, int y, int id, char *text )
{
	menu->generic.type     = MTYPE_PTEXT;
	menu->color            = uis.themeColor;
	menu->generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	if ( s_votemenu.selection == id ) {
		menu->color = uis.themeColor2;
	}
	menu->generic.x        = 320;
	menu->generic.y        = y;
	menu->generic.id       = id;
	menu->generic.callback = VoteMenu_Event;
	menu->string           = text;
	menu->style            = UI_CENTER|UI_SMALLFONT;
}

static void VoteMenu_AddRow( menutext_s *menu, int *y, int id, qboolean show, char *text )
{
	if ( !show ) {
		return;
	}
	VoteMenu_SetMenutext( menu, *y, id, text );
	*y += VOTEMENU_MENU_VERTICAL_SPACING;
}

static int VoteMenu_CountEnabled( void )
{
	int n = 0;

	if ( s_votemenu.nextmap ) {
		n++;
	}
	if ( s_votemenu.map_restart ) {
		n++;
	}
	if ( s_votemenu.shuffle ) {
		n++;
	}
	if ( s_votemenu.map ) {
		n++;
	}
	if ( s_votemenu.setnextmap ) {
		n++;
	}
	if ( s_votemenu.gametype ) {
		n++;
	}
	if ( s_votemenu.clientkick ) {
		n++;
	}
	if ( s_votemenu.g_doWarmup ) {
		n++;
	}
	if ( s_votemenu.fraglimit ) {
		n++;
	}
	if ( s_votemenu.timelimit ) {
		n++;
	}
	return n;
}

/*
=================
UI_VoteMenuMenuInternal
 *Used then forcing a redraw
=================
*/
void UI_VoteMenuMenuInternal( void )
{
	int y;
	int n;
	char *warmupText;

	VoteMenu_Cache();

	s_votemenu.menu.wrapAround = qtrue;
	s_votemenu.menu.fullscreen = qfalse;
	s_votemenu.menu.draw = UI_VoteMenu_Draw;

	s_votemenu.banner.generic.type  = MTYPE_BTEXT;
	s_votemenu.banner.generic.x	  = 320;
	s_votemenu.banner.generic.y	  = 16;
	s_votemenu.banner.string		  = "CALL VOTE";
	s_votemenu.banner.color	      = uis.themeColor;
	s_votemenu.banner.style	      = UI_CENTER;

	n = VoteMenu_CountEnabled();
	if ( n > 0 ) {
		y = VOTEMENU_LIST_CENTER_Y - ( ( n - 1 ) * VOTEMENU_MENU_VERTICAL_SPACING ) / 2;
	} else {
		y = VOTEMENU_LIST_CENTER_Y;
	}

	VoteMenu_AddRow( &s_votemenu.bNextmap, &y, ID_NEXTMAP, s_votemenu.nextmap, "Next map" );
	VoteMenu_AddRow( &s_votemenu.bMapRestart, &y, ID_RESTART, s_votemenu.map_restart, "Restart match" );
	VoteMenu_AddRow( &s_votemenu.bShuffle, &y, ID_SHUFFLE, s_votemenu.shuffle, "Shuffle teams" );
	VoteMenu_AddRow( &s_votemenu.bMap, &y, ID_MAP, s_votemenu.map, "Change map" );
	VoteMenu_AddRow( &s_votemenu.bSetNextMap, &y, ID_SETNEXTMAP, s_votemenu.setnextmap, "Set next map" );
	VoteMenu_AddRow( &s_votemenu.bGametype, &y, ID_GAMETYPE, s_votemenu.gametype, "Change gametype" );
	VoteMenu_AddRow( &s_votemenu.bKick, &y, ID_KICK, s_votemenu.clientkick, "Kick player" );

	if ( s_votemenu.g_doWarmupEnabled ) {
		warmupText = "Disable warmup";
	} else {
		warmupText = "Enable warmup";
	}
	VoteMenu_AddRow( &s_votemenu.bDoWarmup, &y, ID_DOWARMUP, s_votemenu.g_doWarmup, warmupText );
	VoteMenu_AddRow( &s_votemenu.bFraglimit, &y, ID_FRAG, s_votemenu.fraglimit, "Change fraglimit" );
	VoteMenu_AddRow( &s_votemenu.bTimelimit, &y, ID_TIME, s_votemenu.timelimit, "Change timelimit" );

	s_votemenu.back.generic.type	   = MTYPE_BITMAP;
	s_votemenu.back.generic.name     = VOTEMENU_BACK0;
	s_votemenu.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu.back.generic.callback = VoteMenu_Event;
	s_votemenu.back.generic.id	   = ID_BACK;
	s_votemenu.back.generic.x		   = 320-128;
	s_votemenu.back.generic.y		   = 256+128-64;
	s_votemenu.back.width  		   = 128;
	s_votemenu.back.height  		   = 64;
	s_votemenu.back.focuspic         = VOTEMENU_BACK1;

	s_votemenu.go.generic.type	   = MTYPE_BITMAP;
	s_votemenu.go.generic.name     = ART_FIGHT0;
	s_votemenu.go.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu.go.generic.callback = VoteMenu_Event;
	s_votemenu.go.generic.id	   = ID_GO;
	s_votemenu.go.generic.x		   = 320;
	s_votemenu.go.generic.y		   = 256+128-64;
	s_votemenu.go.width  		   = 128;
	s_votemenu.go.height  		   = 64;
	s_votemenu.go.focuspic         = ART_FIGHT1;

}

/*
=================
UI_VoteMenuMenu
 *Called from outside
=================
*/
void UI_VoteMenuMenu( void ) {
	// zero set all our globals
	char serverinfo[MAX_INFO_STRING];
	memset( &s_votemenu, 0 ,sizeof(votemenu_t) );
	trap_GetConfigString( CS_SERVERINFO, serverinfo, MAX_INFO_STRING );
	s_votemenu.g_doWarmupEnabled = atoi(Info_ValueForKey(serverinfo,"g_doWarmup"));
	UI_VoteMenuMenuInternal();

	Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.banner );
	Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.back );
	Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.go );
	if ( s_votemenu.nextmap ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bNextmap );
	}
	if ( s_votemenu.map_restart ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bMapRestart );
	}
	if ( s_votemenu.shuffle ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bShuffle );
	}
	if ( s_votemenu.map ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bMap );
	}
	if ( s_votemenu.setnextmap ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bSetNextMap );
	}
	if ( s_votemenu.gametype ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bGametype );
	}
	if ( s_votemenu.clientkick ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bKick );
	}
	if ( s_votemenu.g_doWarmup ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bDoWarmup );
	}
	if ( s_votemenu.fraglimit ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bFraglimit );
	}
	if ( s_votemenu.timelimit ) {
		Menu_AddItem( &s_votemenu.menu, (void*) &s_votemenu.bTimelimit );
	}

	UI_PushMenu( &s_votemenu.menu );
}
