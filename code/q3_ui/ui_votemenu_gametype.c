/*
===========================================================================
Copyright (C) 2009 Poul Sander

CALL VOTE gametype + modes (Ruthless server GT numbers).
Inactive entries are omitted (not grayed). Modes: one Enable/Disable line
from server votemodes state (via beinfo-capable clients).
===========================================================================
*/

#include "ui_local.h"

#define VOTEMENU_BACK0  "menu/art/back_0"
#define VOTEMENU_BACK1  "menu/art/back_1"
#define ART_FIGHT0      "menu/art/accept_0"
#define ART_FIGHT1      "menu/art/accept_1"
#define ART_BACKGROUND  "menu/art/addbotframe"

static char* votemenu_Gametype_artlist[] =
{
	VOTEMENU_BACK0,
	VOTEMENU_BACK1,
	ART_FIGHT0,
	ART_FIGHT1,
	NULL
};

#define ID_BACK         100
#define ID_GO           101
#define ID_FFA          102
#define ID_TOURNAMENT   103
#define ID_TDM          104
#define ID_CTF          105
#define ID_1FCTF        106
#define ID_OVERLOAD     107
#define ID_HARVESTER    108
#define ID_ELIMINATION  109
#define ID_CTFE         110
#define ID_LMS          111
#define ID_MODE_HDR     120
#define ID_PROMODE      121
#define ID_INSTAGIB     122
#define ID_QUAD         123
#define ID_FREEZE       124
#define ID_AIM          125

#define Gametype_MENU_VERTICAL_SPACING	17
#define Gametype_LIST_CENTER_Y		197

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	go;

	menutext_s      bFFA;
	menutext_s      bTournament;
	menutext_s      bTDM;
	menutext_s      bCTF;
	menutext_s      b1FCTF;
	menutext_s      bOverload;
	menutext_s      bHarvester;
	menutext_s      bElimination;
	menutext_s      bCTFe;
	menutext_s      bLMS;

	menutext_s      bModesHdr;
	menutext_s      bPromode;
	menutext_s      bInstagib;
	menutext_s      bQuad;
	menutext_s      bFreeze;
	menutext_s      bAim;

	qboolean        FFA;
	qboolean        Tournament;
	qboolean        TDM;
	qboolean        CTF;
	qboolean        _1FCTF;
	qboolean        Overload;
	qboolean        Harvester;
	qboolean        Elimination;
	qboolean        CTFe;
	qboolean        LMS;

	qboolean        promode;
	qboolean        instagib;
	qboolean        quad;
	qboolean        freeze;
	qboolean        aim;

	/* callvote arg to send (toggle opposite of current). */
	int             promodeVal;
	int             instagibVal;
	int             quadVal;
	int             freezeVal;
	int             aimVal;

	char            promodeLabel[48];
	char            instagibLabel[48];
	char            quadLabel[48];
	char            freezeLabel[48];
	char            aimLabel[48];

	int selection;
} votemenu_t;

static votemenu_t	s_votemenu_Gametype;

void UI_VoteGametypeMenuInternal( void );

static void VoteMenu_Gametype_Confirm( void )
{
	switch ( s_votemenu_Gametype.selection ) {
	case ID_FFA:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 0" );
		break;
	case ID_TOURNAMENT:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 1" );
		break;
	case ID_TDM:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 3" );
		break;
	case ID_CTF:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 4" );
		break;
	case ID_1FCTF:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 5" );
		break;
	case ID_OVERLOAD:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 6" );
		break;
	case ID_HARVESTER:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 7" );
		break;
	case ID_ELIMINATION:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 8" );
		break;
	case ID_CTFE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 9" );
		break;
	case ID_LMS:
		trap_Cmd_ExecuteText( EXEC_APPEND, "callvote g_gametype 10" );
		break;
	case ID_PROMODE:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote promode %i", s_votemenu_Gametype.promodeVal ) );
		break;
	case ID_INSTAGIB:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote instagib %i", s_votemenu_Gametype.instagibVal ) );
		break;
	case ID_QUAD:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote quad %i", s_votemenu_Gametype.quadVal ) );
		break;
	case ID_FREEZE:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote freeze %i", s_votemenu_Gametype.freezeVal ) );
		break;
	case ID_AIM:
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote aim %i", s_votemenu_Gametype.aimVal ) );
		break;
	default:
		return;
	}
	UI_PopMenu();
	UI_PopMenu();
}

static void VoteMenu_Gametype_Event( void* ptr, int event )
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
			VoteMenu_Gametype_Confirm();
			break;
		case ID_MODE_HDR:
			break;
		default:
			if ( s_votemenu_Gametype.selection == id ) {
				VoteMenu_Gametype_Confirm();
			} else {
				s_votemenu_Gametype.selection = id;
				UI_VoteGametypeMenuInternal();
				UI_MouseEvent( 0, 0 );
			}
			break;
	}
}

static void setGametypeMenutext(menutext_s *menu,int y,int id,char *text) {
	menu->generic.type            = MTYPE_PTEXT;
	menu->color               = uis.themeColor;
	menu->generic.flags       = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	if(s_votemenu_Gametype.selection == id)
		menu->color       = uis.themeColor2;
	menu->generic.x           = 320;
	menu->generic.y           = y;
	menu->generic.id          = id;
	menu->generic.callback    = VoteMenu_Gametype_Event;
	menu->string              = text;
	menu->style               = UI_CENTER|UI_SMALLFONT;
}

static void VoteMenu_Gametype_AddRow( menutext_s *menu, int *y, int id, qboolean show, char *text )
{
	if ( !show ) {
		return;
	}
	setGametypeMenutext( menu, *y, id, text );
	*y += Gametype_MENU_VERTICAL_SPACING;
}

static int VoteMenu_Gametype_Count( void )
{
	int n = 0;
	qboolean anyMode;

	if ( s_votemenu_Gametype.FFA ) {
		n++;
	}
	if ( s_votemenu_Gametype.Tournament ) {
		n++;
	}
	if ( s_votemenu_Gametype.TDM ) {
		n++;
	}
	if ( s_votemenu_Gametype.CTF ) {
		n++;
	}
	if ( s_votemenu_Gametype._1FCTF ) {
		n++;
	}
	if ( s_votemenu_Gametype.Overload ) {
		n++;
	}
	if ( s_votemenu_Gametype.Harvester ) {
		n++;
	}
	if ( s_votemenu_Gametype.Elimination ) {
		n++;
	}
	if ( s_votemenu_Gametype.CTFe ) {
		n++;
	}
	if ( s_votemenu_Gametype.LMS ) {
		n++;
	}

	anyMode = (qboolean)( s_votemenu_Gametype.promode || s_votemenu_Gametype.instagib
		|| s_votemenu_Gametype.quad || s_votemenu_Gametype.freeze || s_votemenu_Gametype.aim );
	if ( anyMode ) {
		n++;
	}
	if ( s_votemenu_Gametype.promode ) {
		n++;
	}
	if ( s_votemenu_Gametype.instagib ) {
		n++;
	}
	if ( s_votemenu_Gametype.quad ) {
		n++;
	}
	if ( s_votemenu_Gametype.freeze ) {
		n++;
	}
	if ( s_votemenu_Gametype.aim ) {
		n++;
	}
	return n;
}

static void VoteMenu_Gametype_Cache( void )
{
	int	i;

	for (i=0; ;i++)
	{
		if (!votemenu_Gametype_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(votemenu_Gametype_artlist[i]);
	}
}

static void UI_VoteMenu_Gametype_Draw( void ) {
	UI_DrawBannerString( 320, 16, "CALL VOTE GAMETYPE", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( 320-233, 240-166, 466, 332, ART_BACKGROUND );
	Menu_Draw( &s_votemenu_Gametype.menu );
}

static qboolean VoteGametype_Allowed( const char *gametypeinfo, const char *token )
{
	if ( !gametypeinfo || !gametypeinfo[0] ) {
		return qfalse;
	}
	if ( !Q_stricmp( gametypeinfo, "*" ) ) {
		return qtrue;
	}
	return Q_stristr( gametypeinfo, token ) ? qtrue : qfalse;
}

static void VoteMenu_SetModeToggle( int on, int *outVal, char *label, int labelSize, const char *name )
{
	if ( on ) {
		*outVal = 0;
		Com_sprintf( label, labelSize, "Disable %s", name );
	} else {
		*outVal = 1;
		Com_sprintf( label, labelSize, "Enable %s", name );
	}
}

/*
=================
UI_VoteGametypeMenuInternal
=================
*/
void UI_VoteGametypeMenuInternal( void )
{
	int y;
	int n;
	qboolean anyMode;

	VoteMenu_Gametype_Cache();

	s_votemenu_Gametype.menu.wrapAround = qtrue;
	s_votemenu_Gametype.menu.fullscreen = qfalse;
	s_votemenu_Gametype.menu.draw = UI_VoteMenu_Gametype_Draw;

	s_votemenu_Gametype.banner.generic.type  = MTYPE_BTEXT;
	s_votemenu_Gametype.banner.generic.x	  = 320;
	s_votemenu_Gametype.banner.generic.y	  = 16;
	s_votemenu_Gametype.banner.string		  = "CALL VOTE GAMETYPE";
	s_votemenu_Gametype.banner.color	      = uis.themeColor;
	s_votemenu_Gametype.banner.style	      = UI_CENTER;

	n = VoteMenu_Gametype_Count();
	if ( n > 0 ) {
		y = Gametype_LIST_CENTER_Y - ( ( n - 1 ) * Gametype_MENU_VERTICAL_SPACING ) / 2;
	} else {
		y = Gametype_LIST_CENTER_Y;
	}

	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bFFA, &y, ID_FFA, s_votemenu_Gametype.FFA, "Free For All" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bTournament, &y, ID_TOURNAMENT, s_votemenu_Gametype.Tournament, "Tournament" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bTDM, &y, ID_TDM, s_votemenu_Gametype.TDM, "Team Deathmatch" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bCTF, &y, ID_CTF, s_votemenu_Gametype.CTF, "Capture The Flag" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.b1FCTF, &y, ID_1FCTF, s_votemenu_Gametype._1FCTF, "One Flag Capture" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bOverload, &y, ID_OVERLOAD, s_votemenu_Gametype.Overload, "Overload" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bHarvester, &y, ID_HARVESTER, s_votemenu_Gametype.Harvester, "Harvester" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bElimination, &y, ID_ELIMINATION, s_votemenu_Gametype.Elimination, "Elimination" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bCTFe, &y, ID_CTFE, s_votemenu_Gametype.CTFe, "CTF Elimination" );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bLMS, &y, ID_LMS, s_votemenu_Gametype.LMS, "Last Man Standing" );

	anyMode = (qboolean)( s_votemenu_Gametype.promode || s_votemenu_Gametype.instagib
		|| s_votemenu_Gametype.quad || s_votemenu_Gametype.freeze || s_votemenu_Gametype.aim );
	if ( anyMode ) {
		setGametypeMenutext( &s_votemenu_Gametype.bModesHdr, y, ID_MODE_HDR, "Modes" );
		s_votemenu_Gametype.bModesHdr.generic.flags |= QMF_INACTIVE;
		s_votemenu_Gametype.bModesHdr.color = color_white;
		y += Gametype_MENU_VERTICAL_SPACING;
	}
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bPromode, &y, ID_PROMODE, s_votemenu_Gametype.promode, s_votemenu_Gametype.promodeLabel );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bInstagib, &y, ID_INSTAGIB, s_votemenu_Gametype.instagib, s_votemenu_Gametype.instagibLabel );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bQuad, &y, ID_QUAD, s_votemenu_Gametype.quad, s_votemenu_Gametype.quadLabel );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bFreeze, &y, ID_FREEZE, s_votemenu_Gametype.freeze, s_votemenu_Gametype.freezeLabel );
	VoteMenu_Gametype_AddRow( &s_votemenu_Gametype.bAim, &y, ID_AIM, s_votemenu_Gametype.aim, s_votemenu_Gametype.aimLabel );

	s_votemenu_Gametype.back.generic.type	   = MTYPE_BITMAP;
	s_votemenu_Gametype.back.generic.name     = VOTEMENU_BACK0;
	s_votemenu_Gametype.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_Gametype.back.generic.callback = VoteMenu_Gametype_Event;
	s_votemenu_Gametype.back.generic.id	   = ID_BACK;
	s_votemenu_Gametype.back.generic.x		   = 320-128;
	s_votemenu_Gametype.back.generic.y		   = 256+128-64;
	s_votemenu_Gametype.back.width  		   = 128;
	s_votemenu_Gametype.back.height  		   = 64;
	s_votemenu_Gametype.back.focuspic         = VOTEMENU_BACK1;

	s_votemenu_Gametype.go.generic.type	   = MTYPE_BITMAP;
	s_votemenu_Gametype.go.generic.name     = ART_FIGHT0;
	s_votemenu_Gametype.go.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_votemenu_Gametype.go.generic.callback = VoteMenu_Gametype_Event;
	s_votemenu_Gametype.go.generic.id	   = ID_GO;
	s_votemenu_Gametype.go.generic.x		   = 320;
	s_votemenu_Gametype.go.generic.y		   = 256+128-64;
	s_votemenu_Gametype.go.width  		   = 128;
	s_votemenu_Gametype.go.height  		   = 64;
	s_votemenu_Gametype.go.focuspic         = ART_FIGHT1;
}

/*
=================
UI_VoteGametypeMenu
=================
*/
void UI_VoteGametypeMenu( void ) {
	char serverinfo[MAX_INFO_STRING];
	char *gametypeinfo;
	int voteflags;
	int promodeOn;
	int instagibOn;
	int quadOn;
	int freezeOn;
	int aimOn;
	qboolean anyMode;

	memset( &s_votemenu_Gametype, 0, sizeof(votemenu_t) );
	trap_GetConfigString( CS_SERVERINFO, serverinfo, MAX_INFO_STRING );

	voteflags = atoi( Info_ValueForKey( serverinfo, "voteflags" ) );
	s_votemenu_Gametype.promode  = ( voteflags & VF_promode ) ? qtrue : qfalse;
	s_votemenu_Gametype.instagib = ( voteflags & VF_instagib ) ? qtrue : qfalse;
	s_votemenu_Gametype.quad     = ( voteflags & VF_quad ) ? qtrue : qfalse;
	s_votemenu_Gametype.freeze   = ( voteflags & VF_freeze ) ? qtrue : qfalse;
	s_votemenu_Gametype.aim      = ( voteflags & VF_aim ) ? qtrue : qfalse;

	gametypeinfo = Info_ValueForKey( serverinfo, "g_voteGametypes" );
	if ( voteflags & VF_g_gametype ) {
		s_votemenu_Gametype.FFA         = VoteGametype_Allowed( gametypeinfo, "/0/" );
		s_votemenu_Gametype.Tournament  = VoteGametype_Allowed( gametypeinfo, "/1/" );
		s_votemenu_Gametype.TDM         = VoteGametype_Allowed( gametypeinfo, "/3/" );
		s_votemenu_Gametype.CTF         = VoteGametype_Allowed( gametypeinfo, "/4/" );
		s_votemenu_Gametype._1FCTF      = VoteGametype_Allowed( gametypeinfo, "/5/" );
		s_votemenu_Gametype.Overload    = VoteGametype_Allowed( gametypeinfo, "/6/" );
		s_votemenu_Gametype.Harvester   = VoteGametype_Allowed( gametypeinfo, "/7/" );
		s_votemenu_Gametype.Elimination = VoteGametype_Allowed( gametypeinfo, "/8/" );
		s_votemenu_Gametype.CTFe        = VoteGametype_Allowed( gametypeinfo, "/9/" );
		s_votemenu_Gametype.LMS         = VoteGametype_Allowed( gametypeinfo, "/10/" );
	}

	/* States from server votemodes (beinfo clients); fallback 0 if not received. */
	promodeOn  = (int)trap_Cvar_VariableValue( "ui_voteModePromode" );
	instagibOn = (int)trap_Cvar_VariableValue( "ui_voteModeInstagib" );
	quadOn     = (int)trap_Cvar_VariableValue( "ui_voteModeQuad" );
	freezeOn   = (int)trap_Cvar_VariableValue( "ui_voteModeFreeze" );
	aimOn      = (int)trap_Cvar_VariableValue( "ui_voteModeAim" );

	VoteMenu_SetModeToggle( promodeOn, &s_votemenu_Gametype.promodeVal,
		s_votemenu_Gametype.promodeLabel, sizeof( s_votemenu_Gametype.promodeLabel ), "Promode" );
	VoteMenu_SetModeToggle( instagibOn, &s_votemenu_Gametype.instagibVal,
		s_votemenu_Gametype.instagibLabel, sizeof( s_votemenu_Gametype.instagibLabel ), "Instagib" );
	VoteMenu_SetModeToggle( quadOn, &s_votemenu_Gametype.quadVal,
		s_votemenu_Gametype.quadLabel, sizeof( s_votemenu_Gametype.quadLabel ), "Quad" );
	VoteMenu_SetModeToggle( freezeOn, &s_votemenu_Gametype.freezeVal,
		s_votemenu_Gametype.freezeLabel, sizeof( s_votemenu_Gametype.freezeLabel ), "Freeze" );
	VoteMenu_SetModeToggle( aimOn, &s_votemenu_Gametype.aimVal,
		s_votemenu_Gametype.aimLabel, sizeof( s_votemenu_Gametype.aimLabel ), "Aim" );

	UI_VoteGametypeMenuInternal();

	Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.banner );
	Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.back );
	Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.go );

	if ( s_votemenu_Gametype.FFA )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bFFA );
	if ( s_votemenu_Gametype.Tournament )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bTournament );
	if ( s_votemenu_Gametype.TDM )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bTDM );
	if ( s_votemenu_Gametype.CTF )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bCTF );
	if ( s_votemenu_Gametype._1FCTF )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.b1FCTF );
	if ( s_votemenu_Gametype.Overload )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bOverload );
	if ( s_votemenu_Gametype.Harvester )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bHarvester );
	if ( s_votemenu_Gametype.Elimination )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bElimination );
	if ( s_votemenu_Gametype.CTFe )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bCTFe );
	if ( s_votemenu_Gametype.LMS )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bLMS );

	anyMode = (qboolean)( s_votemenu_Gametype.promode || s_votemenu_Gametype.instagib
		|| s_votemenu_Gametype.quad || s_votemenu_Gametype.freeze || s_votemenu_Gametype.aim );
	if ( anyMode )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bModesHdr );
	if ( s_votemenu_Gametype.promode )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bPromode );
	if ( s_votemenu_Gametype.instagib )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bInstagib );
	if ( s_votemenu_Gametype.quad )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bQuad );
	if ( s_votemenu_Gametype.freeze )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bFreeze );
	if ( s_votemenu_Gametype.aim )
		Menu_AddItem( &s_votemenu_Gametype.menu, (void*) &s_votemenu_Gametype.bAim );

	UI_PushMenu( &s_votemenu_Gametype.menu );
}
