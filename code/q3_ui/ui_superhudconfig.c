#include "ui_local.h"
#include "ui_osp2be_shared.h"

#define SUPERHUD_CONFIG_LIST_WIDTH 25
#define SUPERHUD_CONFIG_LIST_HEIGHT 17
#define SUPERHUD_CONFIG_MAX_FILES 256
#define SUPERHUD_CONFIG_MENU_WIDTH 200
#define SUPERHUD_CONFIG_MENU_HEIGHT 340

#define ID_SUPERHUD_LIST 10
#define ID_SUPERHUD_RESTORE 11
#define ID_SUPERHUD_BACK 12

typedef struct {
	menuframework_s menu;
	menulist_s list;
	menutext_s restore;
	menutext_s back;
	char *itemnames[SUPERHUD_CONFIG_MAX_FILES];
	char filenames[SUPERHUD_CONFIG_MAX_FILES][MAX_QPATH];
	int numfiles;
	char currentConfig[MAX_QPATH];
} superhudConfigMenu_t;

static superhudConfigMenu_t s_superhudconfig;

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

static void SuperHUDConfig_DblClick( void *ptr ) {
	SuperHUDConfig_ApplyConfig();
}

static void SuperHUDConfig_DrawList( void *self );

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

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_RESTORE ) {
		SuperHUDConfig_RestoreConfig();
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_BACK ) {
		UI_PopMenu();
		return;
	}

	if ( ((menucommon_s*)ptr)->id == ID_SUPERHUD_LIST ) {
		/* Handle clicks on list items */
		menulist_s *l;
		int clickedIndex;
		int itemY;
		int itemH;
		
		l = &s_superhudconfig.list;
		if ( l->generic.flags & QMF_HASMOUSEFOCUS ) {
			itemY = l->generic.y;
			itemH = SMALLCHAR_HEIGHT;
			clickedIndex = ( uis.cursory - itemY ) / itemH;
			if ( clickedIndex >= 0 && clickedIndex < l->numitems ) {
				clickedIndex += l->top;
				if ( clickedIndex >= 0 && clickedIndex < l->numitems ) {
					l->curvalue = clickedIndex;
					/* Update top to keep clicked item visible */
					if ( clickedIndex < l->top ) {
						l->top = clickedIndex;
					} else if ( clickedIndex >= l->top + l->height ) {
						l->top = clickedIndex - l->height + 1;
						if ( l->top < 0 ) {
							l->top = 0;
						}
					}
				}
			}
		}
		return;
	}
}

static sfxHandle_t SuperHUDConfig_Key( int key ) {
	menulist_s *l;
	int clickedIndex;
	int itemY;
	int itemH;
	int itemX;
	int itemW;
	
	if ( key == K_ESCAPE || key == K_MOUSE2 ) {
		UI_PopMenu();
		return menu_out_sound;
	}
	
	/* Handle mouse clicks on list items */
	if ( key == K_MOUSE1 ) {
		int menuX;
		int menuY;
		int menuWidth;
		int maxVisibleY;
		
		l = &s_superhudconfig.list;
		if ( l->generic.flags & QMF_HASMOUSEFOCUS ) {
			menuX = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2;
			menuY = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2;
			menuWidth = SUPERHUD_CONFIG_MENU_WIDTH;
			maxVisibleY = menuY + SUPERHUD_CONFIG_MENU_HEIGHT - 50;
			
			itemX = menuX + 10;
			itemY = l->generic.y;
			itemW = menuWidth - 20;
			itemH = SMALLCHAR_HEIGHT;
			
			if ( UI_CursorInRect( itemX, itemY, itemW, l->height * itemH ) && uis.cursory < maxVisibleY ) {
				clickedIndex = ( uis.cursory - itemY ) / itemH;
				if ( clickedIndex >= 0 && clickedIndex < l->height ) {
					clickedIndex += l->top;
					if ( clickedIndex >= 0 && clickedIndex < l->numitems ) {
						/* If clicking on already selected item, apply config */
						if ( clickedIndex == l->curvalue ) {
							SuperHUDConfig_ApplyConfig();
							return menu_move_sound;
						}
						l->curvalue = clickedIndex;
						/* Update top to keep clicked item visible */
						if ( clickedIndex < l->top ) {
							l->top = clickedIndex;
						} else if ( clickedIndex >= l->top + l->height ) {
							l->top = clickedIndex - l->height + 1;
							if ( l->top < 0 ) {
								l->top = 0;
							}
						}
						return menu_move_sound;
					}
				}
			}
		}
	}
	
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		if ( s_superhudconfig.menu.cursor == 0 && s_superhudconfig.numfiles > 0 ) {
			SuperHUDConfig_ApplyConfig();
			return menu_move_sound;
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

	/* Initialize fonts */
	UI_LoadFonts();
	
	/* Initialize font cvar and select font */
	{
		static vmCvar_t ui_be_font;
		int fontIndex;
		
		trap_Cvar_Register( &ui_be_font, "ui_be_font", "0", CVAR_ARCHIVE );
		trap_Cvar_Update( &ui_be_font );
		
		fontIndex = UI_FontIndexFromCvar( &ui_be_font );
		if ( !UI_FontAvailable( fontIndex ) ) {
			fontIndex = 0;
			trap_Cvar_Set( "ui_be_font", "0" );
		}
		UI_FontSelect( fontIndex );
		osp2be_font.index = fontIndex;
		osp2be_font.flags = DS_PROPORTIONAL;
	}

	SuperHUDConfig_LoadFiles();

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
	s_superhudconfig.list.generic.x = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2 + 10;
	s_superhudconfig.list.generic.y = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2 + 10;
	s_superhudconfig.list.width = ( SUPERHUD_CONFIG_MENU_WIDTH - 20 ) / SMALLCHAR_WIDTH; /* Fit within menu width */
	s_superhudconfig.list.height = SUPERHUD_CONFIG_LIST_HEIGHT;
	s_superhudconfig.list.columns = 1;
	s_superhudconfig.list.scroll = 1;
	s_superhudconfig.list.generic.dblclick = SuperHUDConfig_DblClick;
	s_superhudconfig.list.itemnames = (const char **)s_superhudconfig.itemnames;
	if ( s_superhudconfig.numfiles > 0 ) {
		s_superhudconfig.list.numitems = s_superhudconfig.numfiles;
	}
	s_superhudconfig.list.generic.ownerdraw = SuperHUDConfig_DrawList;
	
	/* Set clickable area for list */
	{
		int menuX;
		int menuWidth;
		menuX = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2;
		menuWidth = SUPERHUD_CONFIG_MENU_WIDTH;
		s_superhudconfig.list.generic.left = menuX + 10;
		s_superhudconfig.list.generic.right = menuX + menuWidth - 10;
		s_superhudconfig.list.generic.top = s_superhudconfig.list.generic.y;
		s_superhudconfig.list.generic.bottom = s_superhudconfig.list.generic.y + s_superhudconfig.list.height * SMALLCHAR_HEIGHT;
	}
	
	/* Add list to menu so it can receive focus */
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.list );

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
	s_superhudconfig.restore.generic.x = 320;
	s_superhudconfig.restore.generic.y = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2 + SUPERHUD_CONFIG_MENU_HEIGHT - 50;
	s_superhudconfig.restore.string = "RESTORE";
	s_superhudconfig.restore.color = color_red;
	s_superhudconfig.restore.style = UI_CENTER|UI_SMALLFONT;

	s_superhudconfig.back.generic.type = MTYPE_PTEXT;
	s_superhudconfig.back.generic.flags = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_superhudconfig.back.generic.callback = SuperHUDConfig_Event;
	s_superhudconfig.back.generic.id = ID_SUPERHUD_BACK;
	s_superhudconfig.back.generic.x = 320;
	s_superhudconfig.back.generic.y = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2 + SUPERHUD_CONFIG_MENU_HEIGHT - 30;
	s_superhudconfig.back.string = "BACK";
	s_superhudconfig.back.color = color_red;
	s_superhudconfig.back.style = UI_CENTER|UI_SMALLFONT;

	/* List is drawn manually, don't add to menu */
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.restore );
	Menu_AddItem( &s_superhudconfig.menu, &s_superhudconfig.back );
}

static void SuperHUDConfig_DrawList( void *self ) {
	menulist_s *l;
#ifdef __GNUC__
	__attribute__((unused))
#endif
	int x;
	int y;
	int base;
	int i;
	int itemX;
	int itemY;
	int itemW;
	int itemH;
	vec4_t borderColor;
	vec4_t backgroundColor;
	vec4_t textColor;
	vec4_t shadowColor;
	const char *itemText;
	int flags;
	int hoverIndex;
	int checkX;
	int checkY;
	int checkW;
	int checkH;
#ifdef __GNUC__
	__attribute__((unused))
#endif
	int itemStartX;
#ifdef __GNUC__
	__attribute__((unused))
#endif
	int itemEndX;

	l = (menulist_s *)self;
	if ( !l ) {
		return;
	}

	hoverIndex = -1;
	if ( l->generic.flags & QMF_HASMOUSEFOCUS ) {
#ifdef __GNUC__
		__attribute__((unused))
#endif
		int menuX;
		int menuY;
		int menuWidth;
		int maxVisibleY;
#ifdef __GNUC__
		__attribute__((unused))
#endif
		int visibleItemY;
		int visibleItemIndex;
		
		menuX = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2;
		menuY = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2;
		menuWidth = SUPERHUD_CONFIG_MENU_WIDTH;
		maxVisibleY = menuY + SUPERHUD_CONFIG_MENU_HEIGHT - 50;
		
		checkX = l->generic.x;
		checkY = l->generic.y;
		checkW = menuWidth - 20; /* Leave padding on both sides */
		checkH = l->height * SMALLCHAR_HEIGHT;

		if ( UI_CursorInRect( checkX, checkY, checkW, checkH ) && uis.cursory < maxVisibleY ) {
			visibleItemIndex = ( uis.cursory - checkY ) / SMALLCHAR_HEIGHT;
			if ( visibleItemIndex >= 0 && visibleItemIndex < l->height ) {
				visibleItemIndex += l->top;
				if ( visibleItemIndex >= 0 && visibleItemIndex < l->numitems ) {
					hoverIndex = visibleItemIndex;
				}
			}
		}
	}

	x = l->generic.x;
	y = l->generic.y;
	base = l->top;


	shadowColor[0] = 0.0f;
	shadowColor[1] = 0.0f;
	shadowColor[2] = 0.0f;
	shadowColor[3] = 1.0f;

	/* Calculate max visible items based on menu height */
	{
		int menuY;
		int menuHeight;
		int maxVisibleY;
#ifdef __GNUC__
		__attribute__((unused))
#endif
		int itemsDrawn;
		int maxItemsToDraw;
		
		menuY = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2;
		menuHeight = SUPERHUD_CONFIG_MENU_HEIGHT;
		maxVisibleY = menuY + menuHeight - 50; /* Leave space for restore and back buttons */
		itemsDrawn = 0;
		
		/* Calculate how many items can fit in the visible area */
		maxItemsToDraw = ( maxVisibleY - y ) / SMALLCHAR_HEIGHT;
		if ( maxItemsToDraw > l->height ) {
			maxItemsToDraw = l->height;
		}
		/* Limit by how many items are available starting from base */
		if ( maxItemsToDraw > l->numitems - base ) {
			maxItemsToDraw = l->numitems - base;
		}
		
		
		for ( i = base; i < l->numitems && i < base + maxItemsToDraw; i++ ) {
			int menuX;
			int menuWidth;
			
			menuX = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2;
			menuWidth = SUPERHUD_CONFIG_MENU_WIDTH;
			
			itemW = menuWidth - 20; /* Leave padding on both sides */
			itemH = SMALLCHAR_HEIGHT;
			itemX = menuX + 10; /* Center with padding */
			itemY = y;

			if ( i == l->curvalue ) {
				/* Selected: white background, black text */
				backgroundColor[0] = 1.0f;
				backgroundColor[1] = 1.0f;
				backgroundColor[2] = 1.0f;
				backgroundColor[3] = 0.4f;
				borderColor[0] = 1.0f;
				borderColor[1] = 1.0f;
				borderColor[2] = 1.0f;
				borderColor[3] = 1.0f;
				textColor[0] = 0.0f;
				textColor[1] = 0.0f;
				textColor[2] = 0.0f;
				textColor[3] = 1.0f;
			} else if ( i == hoverIndex ) {
				/* Hover: gray background, white text */
				backgroundColor[0] = 0.5f;
				backgroundColor[1] = 0.5f;
				backgroundColor[2] = 0.5f;
				backgroundColor[3] = 0.4f;
				borderColor[0] = 1.0f;
				borderColor[1] = 1.0f;
				borderColor[2] = 1.0f;
				borderColor[3] = 1.0f;
				textColor[0] = 1.0f;
				textColor[1] = 1.0f;
				textColor[2] = 1.0f;
				textColor[3] = 1.0f;
			} else {
				/* Normal: black background, white text */
				backgroundColor[0] = 0.0f;
				backgroundColor[1] = 0.0f;
				backgroundColor[2] = 0.0f;
				backgroundColor[3] = 0.4f;
				borderColor[0] = 0.5f;
				borderColor[1] = 0.5f;
				borderColor[2] = 0.5f;
				borderColor[3] = 1.0f;
				textColor[0] = 1.0f;
				textColor[1] = 1.0f;
				textColor[2] = 1.0f;
				textColor[3] = 1.0f;
			}

			UI_FillRect( itemX, itemY, itemW, itemH, backgroundColor );
			UI_DrawRect( itemX, itemY, itemW, itemH, borderColor );

			if ( l->itemnames && l->itemnames[i] ) {
				itemText = l->itemnames[i];
			} else {
				itemText = "";
			}

			flags = osp2be_font.flags | DS_HCENTER | DS_SHADOW;
			UI_OSPDrawStringNew( (float)itemX + (float)itemW / 2.0f, (float)y, itemText, textColor, shadowColor,
			                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT, SCREEN_WIDTH, flags, NULL, NULL, NULL );

			y += SMALLCHAR_HEIGHT;
		}
		
	}
}

static void SuperHUDConfig_MenuDraw( void ) {
	int menuX;
	int menuY;
	vec4_t backgroundColor;
	vec4_t borderColor;

	menuX = ( 640 - SUPERHUD_CONFIG_MENU_WIDTH ) / 2;
	menuY = ( 480 - SUPERHUD_CONFIG_MENU_HEIGHT ) / 2;

	backgroundColor[0] = 0.0f;
	backgroundColor[1] = 0.0f;
	backgroundColor[2] = 0.0f;
	backgroundColor[3] = 0.4f;

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	UI_FillRect( menuX, menuY, SUPERHUD_CONFIG_MENU_WIDTH, SUPERHUD_CONFIG_MENU_HEIGHT, backgroundColor );
	UI_DrawRect( menuX, menuY, SUPERHUD_CONFIG_MENU_WIDTH, SUPERHUD_CONFIG_MENU_HEIGHT, borderColor );

	/* Draw list manually with new function */
	SuperHUDConfig_DrawList( &s_superhudconfig.list );

	/* Draw restore button with new function */
	{
		vec4_t textColor;
		vec4_t shadowColor;
		int flags;
		int textX;
		int textY;

		textX = s_superhudconfig.restore.generic.x;
		textY = s_superhudconfig.restore.generic.y;
		textColor[0] = 1.0f;
		textColor[1] = 1.0f;
		textColor[2] = 1.0f;
		textColor[3] = 1.0f;
		shadowColor[0] = 0.0f;
		shadowColor[1] = 0.0f;
		shadowColor[2] = 0.0f;
		shadowColor[3] = 1.0f;
		flags = osp2be_font.flags | DS_HCENTER | DS_SHADOW;

		if ( s_superhudconfig.restore.generic.flags & QMF_HASMOUSEFOCUS ) {
			textColor[0] = 1.0f;
			textColor[1] = 1.0f;
			textColor[2] = 1.0f;
		} else {
			textColor[0] = 0.5f;
			textColor[1] = 0.5f;
			textColor[2] = 0.5f;
		}

		UI_OSPDrawStringNew( (float)textX, (float)textY, s_superhudconfig.restore.string, textColor, shadowColor,
		                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT, SCREEN_WIDTH, flags, NULL, NULL, NULL );
	}

	/* Draw back button with new function */
	{
		vec4_t textColor;
		vec4_t shadowColor;
		int flags;
		int textX;
		int textY;

		textX = s_superhudconfig.back.generic.x;
		textY = s_superhudconfig.back.generic.y;
		textColor[0] = 1.0f;
		textColor[1] = 1.0f;
		textColor[2] = 1.0f;
		textColor[3] = 1.0f;
		shadowColor[0] = 0.0f;
		shadowColor[1] = 0.0f;
		shadowColor[2] = 0.0f;
		shadowColor[3] = 1.0f;
		flags = osp2be_font.flags | DS_HCENTER | DS_SHADOW;

		if ( s_superhudconfig.back.generic.flags & QMF_HASMOUSEFOCUS ) {
			textColor[0] = 1.0f;
			textColor[1] = 1.0f;
			textColor[2] = 1.0f;
		} else {
			textColor[0] = 0.5f;
			textColor[1] = 0.5f;
			textColor[2] = 0.5f;
		}

		UI_OSPDrawStringNew( (float)textX, (float)textY, s_superhudconfig.back.string, textColor, shadowColor,
		                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT, SCREEN_WIDTH, flags, NULL, NULL, NULL );
	}
}

void UI_SuperHUDConfigMenu( void ) {
	SuperHUDConfig_MenuInit();
	s_superhudconfig.menu.draw = SuperHUDConfig_MenuDraw;
	UI_PushMenu( &s_superhudconfig.menu );
}

