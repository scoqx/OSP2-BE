#include "ui_local.h"
#include "ui_osp2be_shared.h"

#define ART_BACK0					"menu/art/back_0"
#define ART_BACK1					"menu/art/back_1"
#define ART_BACKGROUND				"menu/art/addbotframe"
#define ART_ARROWS					"menu/art/arrows_vert_0"
#define ART_ARROWUP					"menu/art/arrows_vert_top"
#define ART_ARROWDOWN				"menu/art/arrows_vert_bot"

#define SUPERHUD_CONFIG_LIST_WIDTH 26
#define SUPERHUD_CONFIG_LIST_HEIGHT 14
#define SUPERHUD_CONFIG_MAX_FILES 256

#define ID_SUPERHUD_LIST   10
#define ID_SUPERHUD_RESTORE 11
#define ID_SUPERHUD_BACK   12
#define ID_SUPERHUD_UP     13
#define ID_SUPERHUD_DOWN   14

typedef struct {
	menuframework_s menu;
	menulist_s list;
	menutext_s restore;
	menubitmap_s back;
	menubitmap_s arrows;
	menubitmap_s up;
	menubitmap_s down;
	char *itemnames[SUPERHUD_CONFIG_MAX_FILES];
	char filenames[SUPERHUD_CONFIG_MAX_FILES][MAX_QPATH];
	int font;
	int numfiles;
	char currentConfig[MAX_QPATH];
} superhudConfigMenu_t;

static superhudConfigMenu_t s_superhudconfig;

static void SuperHUDConfig_DrawList( void *self );

static void SuperHUDConfig_LoadFiles( void ) {
	char filelist[8192];
	char *fileptr;
	int numfiles;
	int len;
	char filename[MAX_QPATH];
	char *ext;
	int i;

	s_superhudconfig.numfiles = 0;

	/* trap_FS_GetFileList returns number of files, not bytes (same as demos) */
	numfiles = trap_FS_GetFileList( "hud", ".cfg", filelist, sizeof( filelist ) );
	if ( numfiles <= 0 ) {
		return;
	}
	
	/* Limit to max files */
	if ( numfiles > SUPERHUD_CONFIG_MAX_FILES ) {
		numfiles = SUPERHUD_CONFIG_MAX_FILES;
	}

	/* Parse file list - same logic as demos: for ( i = 0; i < n; i++ ) */
	fileptr = filelist;
	for ( i = 0; i < numfiles && s_superhudconfig.numfiles < SUPERHUD_CONFIG_MAX_FILES; i++ ) {
		/* Manual strlen for C89 compatibility - find length until null terminator */
		/* Same logic as demos: len = strlen(s) */
		len = 0;
		while ( fileptr[len] != '\0' ) {
			len++;
		}
		
		/* Skip empty strings */
		if ( len == 0 ) {
			fileptr++;
			continue;
		}
		
		/* Validate length */
		if ( len > 0 && len < MAX_QPATH ) {
			Q_strncpyz( filename, fileptr, sizeof( filename ) );
			
			/* Manual strstr for C89 compatibility - find .cfg extension */
			ext = NULL;
			{
				int j;
				if ( len >= 4 ) {
					for ( j = 0; j <= len - 4; j++ ) {
						if ( filename[j] == '.' && filename[j+1] == 'c' && 
						     filename[j+2] == 'f' && filename[j+3] == 'g' ) {
							ext = filename + j;
							break;
						}
					}
				}
			}
			if ( ext ) {
				*ext = '\0';
			} else {
				fileptr += len + 1;
				continue;
			}
			Q_strncpyz( s_superhudconfig.filenames[s_superhudconfig.numfiles], filename, sizeof( s_superhudconfig.filenames[0] ) );
			s_superhudconfig.itemnames[s_superhudconfig.numfiles] = s_superhudconfig.filenames[s_superhudconfig.numfiles];
			s_superhudconfig.numfiles++;
		}
		
		/* Move to next file (skip null terminator) - same as demos: s += len + 1 */
		fileptr += len + 1;
	}
}

static void SuperHUDConfig_ApplyConfig( void ) {
	int selected;
	char configName[MAX_QPATH];

	selected = s_superhudconfig.list.curvalue;
	if ( selected >= 0 && selected < s_superhudconfig.numfiles ) {
		Q_strncpyz( configName, s_superhudconfig.filenames[selected], sizeof( configName ) );
		trap_Cvar_Set( "ch_file", configName );
		trap_Cvar_Set( "cg_shud", "1" );
		/* ch_file and cg_shud change callbacks already trigger config load - no need for reloadhud */
		/* Don't close menu after applying config */
	}
}

static void SuperHUDConfig_RestoreConfig( void ) {
	if ( s_superhudconfig.currentConfig[0] ) {
		trap_Cvar_Set( "ch_file", s_superhudconfig.currentConfig );
		trap_Cvar_Set( "cg_shud", "1" );
		/* ch_file and cg_shud change callbacks already trigger config load - no need for reloadhud */
	}
}

static void SuperHUDConfig_Event( void *ptr, int event ) {
	if ( event != QM_ACTIVATED ) {
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_UP ) {
		if ( s_superhudconfig.list.top > 0 ) {
			s_superhudconfig.list.top--;
			if ( s_superhudconfig.list.curvalue < s_superhudconfig.list.top ) {
				s_superhudconfig.list.curvalue = s_superhudconfig.list.top;
			}
		}
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_DOWN ) {
		if ( s_superhudconfig.list.top + s_superhudconfig.list.height < s_superhudconfig.list.numitems ) {
			s_superhudconfig.list.top++;
			if ( s_superhudconfig.list.curvalue >= s_superhudconfig.list.top + s_superhudconfig.list.height ) {
				s_superhudconfig.list.curvalue = s_superhudconfig.list.top + s_superhudconfig.list.height - 1;
			}
		}
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_RESTORE ) {
		SuperHUDConfig_RestoreConfig();
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_BACK ) {
		UI_PopMenu();
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_LIST ) {
		/* Apply currently selected config on activation */
		SuperHUDConfig_ApplyConfig();
		return;
	}
}

static sfxHandle_t SuperHUDConfig_Key( int key ) {
	menulist_s *l;

	if ( key == K_UPARROW || key == K_KP_UPARROW ) {
		if ( s_superhudconfig.list.curvalue > 0 ) {
			s_superhudconfig.list.curvalue--;
			if ( s_superhudconfig.list.curvalue < s_superhudconfig.list.top ) {
				s_superhudconfig.list.top = s_superhudconfig.list.curvalue;
			}
			return menu_move_sound;
		}
	}

	if ( key == K_DOWNARROW || key == K_KP_DOWNARROW ) {
		if ( s_superhudconfig.list.curvalue + 1 < s_superhudconfig.list.numitems ) {
			s_superhudconfig.list.curvalue++;
			if ( s_superhudconfig.list.curvalue >= s_superhudconfig.list.top + s_superhudconfig.list.height ) {
				s_superhudconfig.list.top = s_superhudconfig.list.curvalue - s_superhudconfig.list.height + 1;
			}
			return menu_move_sound;
		}
	}

	if ( key == K_ENTER || key == K_KP_ENTER ) {
		if ( s_superhudconfig.list.numitems > 0 ) {
			SuperHUDConfig_ApplyConfig();
			return menu_move_sound;
		}
	}

	/* Handle mouse click on the list: click on already selected item applies config */
	if ( key == K_MOUSE1 ) {
		int itemY;
		int itemH;
		int clickedIndex;
		int prevCur;

		l = &s_superhudconfig.list;
		if ( l->generic.flags & QMF_HASMOUSEFOCUS ) {
			itemY = l->generic.y;
			itemH = SMALLCHAR_HEIGHT;

			if ( UI_CursorInRect( l->generic.left, itemY,
			                      l->generic.right - l->generic.left,
			                      l->height * itemH ) ) {
				clickedIndex = ( uis.cursory - itemY ) / itemH + l->top;
				if ( clickedIndex >= 0 && clickedIndex < l->numitems ) {
					prevCur = l->curvalue;
					l->curvalue = clickedIndex;
					if ( clickedIndex == prevCur ) {
						SuperHUDConfig_ApplyConfig();
					}
					return menu_move_sound;
				}
			}
		}
	}

	return Menu_DefaultKey( &s_superhudconfig.menu, key );
}

static void SuperHUDConfig_MenuInit( void ) {
	int currentIndex;
	int i;
	char currentConfig[MAX_QPATH];
	unsigned char *bytes;
	int j;
	int total;

	/* Manual memset for C89 compatibility */
	bytes = (unsigned char *)&s_superhudconfig;
	total = (int)sizeof( s_superhudconfig );
	for ( j = 0; j < total; j++ ) {
		bytes[j] = 0;
	}

	trap_Cvar_VariableStringBuffer( "ch_file", currentConfig, sizeof( currentConfig ) );
	if ( !currentConfig[0] ) {
		Q_strncpyz( currentConfig, "default", sizeof( currentConfig ) );
	}
	Q_strncpyz( s_superhudconfig.currentConfig, currentConfig, sizeof( s_superhudconfig.currentConfig ) );

	SuperHUDConfig_LoadFiles();

	/* Initialize fonts for list drawing */
	UI_LoadFonts();
	s_superhudconfig.menu.wrapAround = qtrue;
	s_superhudconfig.menu.fullscreen = qfalse;
	s_superhudconfig.menu.key = SuperHUDConfig_Key;

	s_superhudconfig.list.generic.type = MTYPE_SCROLLLIST;
	if ( s_superhudconfig.numfiles > 0 ) {
		s_superhudconfig.list.generic.flags = QMF_PULSEIFFOCUS;
	} else {
		s_superhudconfig.list.generic.flags = QMF_INACTIVE;
		s_superhudconfig.itemnames[0] = "No configs found";
		s_superhudconfig.list.numitems = 1;
	}
	s_superhudconfig.list.generic.callback = SuperHUDConfig_Event;
	s_superhudconfig.list.generic.id = ID_SUPERHUD_LIST;
	s_superhudconfig.list.generic.y = 96;
	s_superhudconfig.list.width = SUPERHUD_CONFIG_LIST_WIDTH;
	s_superhudconfig.list.height = SUPERHUD_CONFIG_LIST_HEIGHT;
	s_superhudconfig.list.columns = 1;
	s_superhudconfig.list.scroll = 1;
	s_superhudconfig.list.generic.ownerdraw = SuperHUDConfig_DrawList;
	s_superhudconfig.list.itemnames = (const char **)s_superhudconfig.itemnames;
	if ( s_superhudconfig.numfiles > 0 ) {
		s_superhudconfig.list.numitems = s_superhudconfig.numfiles;
	}
	{
		int frameX;
		int barWidth;
		int leftMargin;
		int u;

		frameX = FRAME_INGAME_X;
		barWidth = SUPERHUD_CONFIG_LIST_WIDTH * SMALLCHAR_WIDTH;
		leftMargin = ( FRAME_INGAME_W - barWidth ) / 2;

		/* Center bar between frame edges so левый/правый отступы равны */
		s_superhudconfig.list.generic.x = frameX + leftMargin + 2;

		/* Hitbox совпадает с прямоугольником выделения */
		u = s_superhudconfig.list.generic.x - 2;
		s_superhudconfig.list.generic.left = u;
		s_superhudconfig.list.generic.right = u + barWidth;
		s_superhudconfig.list.generic.top = s_superhudconfig.list.generic.y;
		s_superhudconfig.list.generic.bottom = s_superhudconfig.list.generic.y + s_superhudconfig.list.height * SMALLCHAR_HEIGHT;

		/* Arrow “slider” на тех же координатах, что в ADD BOTS */
		s_superhudconfig.arrows.generic.type  = MTYPE_BITMAP;
		s_superhudconfig.arrows.generic.name  = ART_ARROWS;
		s_superhudconfig.arrows.generic.flags = QMF_INACTIVE;
		s_superhudconfig.arrows.generic.x     = 156;
		s_superhudconfig.arrows.generic.y     = 176;
		s_superhudconfig.arrows.width         = 64;
		s_superhudconfig.arrows.height        = 128;

		s_superhudconfig.up.generic.type      = MTYPE_BITMAP;
		s_superhudconfig.up.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_superhudconfig.up.generic.id        = ID_SUPERHUD_UP;
		s_superhudconfig.up.generic.callback  = SuperHUDConfig_Event;
		s_superhudconfig.up.generic.x         = 156;
		s_superhudconfig.up.generic.y         = 176;
		s_superhudconfig.up.width             = 64;
		s_superhudconfig.up.height            = 64;
		s_superhudconfig.up.focuspic          = ART_ARROWUP;

		s_superhudconfig.down.generic.type     = MTYPE_BITMAP;
		s_superhudconfig.down.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_superhudconfig.down.generic.id       = ID_SUPERHUD_DOWN;
		s_superhudconfig.down.generic.callback = SuperHUDConfig_Event;
		s_superhudconfig.down.generic.x        = 156;
		s_superhudconfig.down.generic.y        = 176 + 64;
		s_superhudconfig.down.width            = 64;
		s_superhudconfig.down.height           = 64;
		s_superhudconfig.down.focuspic         = ART_ARROWDOWN;
	}
	
	/* Add list to menu so it can receive focus */
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.list );
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.arrows );
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.up );
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.down );

	currentIndex = 0;
	if ( s_superhudconfig.numfiles > 0 ) {
		for ( i = 0; i < s_superhudconfig.numfiles; i++ ) {
			if ( !Q_stricmp( s_superhudconfig.filenames[i], s_superhudconfig.currentConfig ) ) {
				currentIndex = i;
				break;
			}
		}
	}
	s_superhudconfig.list.curvalue = currentIndex;
	/* Set top so that current item is visible, but try to show as many items as possible from the start */
	if ( currentIndex < s_superhudconfig.list.height ) {
		s_superhudconfig.list.top = 0;
	} else {
		/* Center the current item in the visible area */
		s_superhudconfig.list.top = currentIndex - ( s_superhudconfig.list.height / 2 );
		if ( s_superhudconfig.list.top < 0 ) {
			s_superhudconfig.list.top = 0;
		}
		if ( s_superhudconfig.list.top + s_superhudconfig.list.height > s_superhudconfig.numfiles ) {
			s_superhudconfig.list.top = s_superhudconfig.numfiles - s_superhudconfig.list.height;
			if ( s_superhudconfig.list.top < 0 ) {
				s_superhudconfig.list.top = 0;
			}
		}
	}

	s_superhudconfig.restore.generic.type = MTYPE_PTEXT;
	s_superhudconfig.restore.generic.flags = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_superhudconfig.restore.generic.callback = SuperHUDConfig_Event;
	s_superhudconfig.restore.generic.id = ID_SUPERHUD_RESTORE;
	/* Справа от BACK, по вертикали по центру кнопки */
	s_superhudconfig.restore.generic.x = 320 + 40; /* правее правого края BACK (320) */
	s_superhudconfig.restore.generic.y = ( 256 + 128 - 64 ) + ( 64 / 2 ) - ( SMALLCHAR_HEIGHT / 2 );
	s_superhudconfig.restore.string = "RESTORE";
	s_superhudconfig.restore.color = uis.themeColor;
	s_superhudconfig.restore.style = UI_CENTER|UI_SMALLFONT;

	s_superhudconfig.back.generic.type = MTYPE_BITMAP;
	s_superhudconfig.back.generic.name = ART_BACK0;
	s_superhudconfig.back.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_superhudconfig.back.generic.callback = SuperHUDConfig_Event;
	s_superhudconfig.back.generic.id = ID_SUPERHUD_BACK;
	s_superhudconfig.back.generic.x = 320 - 128;
	s_superhudconfig.back.generic.y = 256 + 128 - 64;
	s_superhudconfig.back.width = 128;
	s_superhudconfig.back.height = 64;
	s_superhudconfig.back.focuspic = ART_BACK1;

	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.restore );
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.back );
}

static void SuperHUDConfig_UpdateFont( void ) {
	trap_Cvar_Update( &ui_shudListFont );

	s_superhudconfig.font = UI_FontIndexFromCvar( &ui_shudListFont );
	if ( !UI_FontAvailable( s_superhudconfig.font ) ) {
		s_superhudconfig.font = 2;
		trap_Cvar_Set( "ui_shudListFont", "2" );
	}
}

static void SuperHUDConfig_DrawList( void *self ) {
	menulist_s *l;
	int x;
	int y;
	int i;
	int base;
	int barWidth;
	int u;

	vec4_t textColor;
	vec4_t shadowColor;
	vec4_t bgColor;
	int flags;

	l = (menulist_s *)self;
	if ( !l || l->numitems <= 0 ) {
		return;
	}

	SuperHUDConfig_UpdateFont();

	x = l->generic.x;
	y = l->generic.y;
	base = l->top;
	barWidth = l->width * SMALLCHAR_WIDTH;
	u = x - 2;

	shadowColor[0] = 0.0f;
	shadowColor[1] = 0.0f;
	shadowColor[2] = 0.0f;
	shadowColor[3] = 1.0f;

	flags = DS_PROPORTIONAL | DS_HLEFT | DS_SHADOW;

	for ( i = base; i < base + l->height && i < l->numitems; i++ ) {
		const char *itemText;

		if ( i == l->curvalue ) {
			/* Selected item background */
			bgColor[0] = 1.0f;
			bgColor[1] = 1.0f;
			bgColor[2] = 1.0f;
			bgColor[3] = 0.25f;
			UI_FillRect( u, y, barWidth, SMALLCHAR_HEIGHT, bgColor );

			textColor[0] = 0.0f;
			textColor[1] = 0.0f;
			textColor[2] = 0.0f;
			textColor[3] = 1.0f;
		} else {
			textColor[0] = uis.themeColor2[0];
			textColor[1] = uis.themeColor2[1];
			textColor[2] = uis.themeColor2[2];
			textColor[3] = 1.0f;
		}

		if ( l->itemnames && l->itemnames[i] ) {
			itemText = l->itemnames[i];
		} else {
			itemText = "";
		}

		UI_FontSelect(s_superhudconfig.font);

		UI_OSPDrawStringNew( (float)x, (float)y, itemText, textColor, shadowColor,
		                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT,
		                     SCREEN_WIDTH, flags, NULL, NULL, NULL );

		y += SMALLCHAR_HEIGHT;
	}
}

static void SuperHUDConfig_MenuDraw( void ) {
	UI_DrawBannerString( 320, 16, "SUPERHUD", UI_CENTER, uis.themeColor );
	UI_DrawNamedPic( FRAME_INGAME_X, FRAME_INGAME_Y, FRAME_INGAME_W, FRAME_INGAME_H, ART_BACKGROUND );

	/* Draw menu (list via ownerdraw, buttons, arrows, etc.) */
	Menu_Draw( &s_superhudconfig.menu );
}

static void SuperHUDConfig_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_BACKGROUND );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDOWN );
}

void UI_SuperHUDConfigMenu( void ) {
	SuperHUDConfig_Cache();
	SuperHUDConfig_MenuInit();
	s_superhudconfig.menu.draw = SuperHUDConfig_MenuDraw;
	UI_PushMenu( &s_superhudconfig.menu );
}

