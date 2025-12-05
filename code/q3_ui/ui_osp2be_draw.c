#include "ui_osp2be_shared.h"

qboolean Osp2Be_HandleSliderMouse( int cursorX, int cursorY );

/* Theme colors - initialized once */
osp2beThemeColors_t osp2be_theme;

static void Osp2Be_InitTheme( void ) {
	/* Button colors */
	Osp2Be_SetColorVec4( osp2be_theme.button_bg, 0.1f, 0.1f, 0.1f, 0.5f );
	Osp2Be_SetColorVec4( osp2be_theme.button_border_normal, 0.3f, 0.3f, 0.3f, 0.8f );
	Osp2Be_SetColorVec4( osp2be_theme.button_border_focus, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.button_default_color, 0.2f, 0.2f, 0.2f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.button_icon_disabled, 0.4f, 0.4f, 0.4f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.button_icon_enabled, 1.0f, 1.0f, 1.0f, 1.0f );
	
	/* Slider colors - normal */
	Osp2Be_SetColorVec4( osp2be_theme.slider_track_normal, 0.30f, 0.30f, 0.30f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_border_normal, 0.45f, 0.45f, 0.45f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_tick_normal, 0.55f, 0.55f, 0.55f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_handle_normal, 0.90f, 0.90f, 0.90f, 1.0f );
	
	/* Slider colors - focus */
	Osp2Be_SetColorVec4( osp2be_theme.slider_track_focus, 0.60f, 0.60f, 0.70f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_border_focus, 0.85f, 0.85f, 1.00f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_tick_focus, 0.90f, 0.90f, 1.00f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_handle_focus, 1.00f, 1.00f, 1.00f, 1.0f );
	
	/* Slider colors - disabled */
	Osp2Be_SetColorVec4( osp2be_theme.slider_track_disabled, 0.20f, 0.20f, 0.20f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_border_disabled, 0.30f, 0.30f, 0.30f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_tick_disabled, 0.35f, 0.35f, 0.35f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.slider_handle_disabled, 0.55f, 0.55f, 0.55f, 1.0f );
	
	/* Toggle/Checkbox colors */
	Osp2Be_SetColorVec4( osp2be_theme.toggle_bg_normal, 0.08f, 0.08f, 0.08f, 0.75f );
	Osp2Be_SetColorVec4( osp2be_theme.toggle_bg_focus, 0.12f, 0.12f, 0.18f, 0.90f );
	Osp2Be_SetColorVec4( osp2be_theme.toggle_border_normal, 0.30f, 0.30f, 0.30f, 0.90f );
	Osp2Be_SetColorVec4( osp2be_theme.toggle_border_focus, 1.0f, 1.0f, 1.0f, 1.00f );
	Osp2Be_SetColorVec4( osp2be_theme.toggle_text_checked, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.toggle_text_unchecked, 0.5f, 0.5f, 0.5f, 1.0f );
	
	/* Category list colors - normal */
	Osp2Be_SetColorVec4( osp2be_theme.category_bg_normal, 0.1f, 0.1f, 0.1f, 0.3f );
	Osp2Be_SetColorVec4( osp2be_theme.category_border_normal, 0.5f, 0.5f, 0.5f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.category_text_normal, 1.0f, 1.0f, 1.0f, 1.0f );
	
	/* Category list colors - hover */
	Osp2Be_SetColorVec4( osp2be_theme.category_bg_hover, 0.1f, 0.1f, 0.1f, 0.5f );
	Osp2Be_SetColorVec4( osp2be_theme.category_border_hover, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.category_text_hover, 1.0f, 1.0f, 1.0f, 1.0f );
	
	/* Category list colors - selected */
	Osp2Be_SetColorVec4( osp2be_theme.category_bg_selected, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.category_border_selected, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.category_text_selected, 0.0f, 0.0f, 0.0f, 1.0f );
	
	/* Preview button text colors */
	Osp2Be_SetColorVec4( osp2be_theme.preview_text_active, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.preview_text_inactive, 0.7f, 0.7f, 0.7f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.preview_text_focus, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.preview_icon_active, 1.0f, 1.0f, 1.0f, 1.0f );
	Osp2Be_SetColorVec4( osp2be_theme.preview_icon_inactive, 0.5f, 0.5f, 0.5f, 1.0f );
}

/* Helper function to draw text using UI_OSPDrawStringNew */
static void Osp2Be_DrawText( float x, float y, const char *text, int style, vec4_t color ) {
	int flags;
	float charWidth;
	float charHeight;
	
	/* Convert UI style flags to DS flags */
	flags = osp2be_font.flags;
	if ( style & UI_SMALLFONT ) {
		charWidth = SMALLCHAR_WIDTH;
		charHeight = SMALLCHAR_HEIGHT;
	} else if ( style & UI_BIGFONT ) {
		charWidth = BIGCHAR_WIDTH;
		charHeight = BIGCHAR_HEIGHT;
	} else {
		charWidth = SMALLCHAR_WIDTH;
		charHeight = SMALLCHAR_HEIGHT;
	}
	
	if ( style & UI_CENTER ) {
		flags |= DS_HCENTER;
	} else if ( style & UI_RIGHT ) {
		flags |= DS_HRIGHT;
	}
	
	if ( style & UI_DROPSHADOW ) {
		flags |= DS_SHADOW;
	}
	
	UI_OSPDrawStringNew( x, y, text, color, color_black, charWidth, charHeight, SCREEN_WIDTH, flags, NULL, NULL, NULL );
}

static const char *osp2beWeaponIconPaths[OSP2BE_DLIGHT_WEAPON_COUNT] = {
	"icons/iconw_gauntlet",
	"icons/iconw_machinegun",
	"icons/iconw_shotgun",
	"icons/iconw_grenade",
	"icons/iconw_rocket",
	"icons/iconw_lightning",
	"icons/iconw_railgun",
	"icons/iconw_plasma",
	"icons/iconw_bfg"
};

static qhandle_t osp2beWeaponIcons[OSP2BE_DLIGHT_WEAPON_COUNT];

static int Osp2Be_GetWeaponIconIndex( osp2beSpecial_t special ) {
	int index;

	index = special - OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET;
	if ( index >= 0 && index < OSP2BE_DLIGHT_WEAPON_COUNT ) {
		return index;
	}
	return -1;
}

void Osp2Be_InitWeaponIcons( void ) {
	int i;

	for ( i = 0; i < OSP2BE_DLIGHT_WEAPON_COUNT; i++ ) {
		if ( !osp2beWeaponIcons[i] ) {
			osp2beWeaponIcons[i] = trap_R_RegisterShaderNoMip( osp2beWeaponIconPaths[i] );
		}
	}
}

static qhandle_t osp2beRailIcon = 0;
static qhandle_t osp2beExpandIcon = 0;
static qhandle_t osp2beDefaultIcon = 0;
static qhandle_t osp2beReadyIcon = 0;

static void Osp2Be_CopyColor( vec4_t dest, const vec4_t src ) {
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
	dest[3] = src[3];
}

static vec4_t osp2be_savedTextColorNormal;
static vec4_t osp2be_savedTextColorHighlight;
static vec4_t osp2be_savedTextColorDisabled;
static vec4_t osp2be_savedListbarColor;
static qboolean osp2be_themeActive = qfalse;

static void Osp2Be_PushMonochromeTheme( void ) {
	if ( osp2be_themeActive ) {
		return;
	}

	Osp2Be_CopyColor( osp2be_savedTextColorNormal, text_color_normal );
	Osp2Be_CopyColor( osp2be_savedTextColorHighlight, text_color_highlight );
	Osp2Be_CopyColor( osp2be_savedTextColorDisabled, text_color_disabled );
	Osp2Be_CopyColor( osp2be_savedListbarColor, listbar_color );

	text_color_normal[0] = 1.00f;
	text_color_normal[1] = 1.00f;
	text_color_normal[2] = 1.00f;
	text_color_normal[3] = 1.00f;

	text_color_highlight[0] = 0.70f;
	text_color_highlight[1] = 0.70f;
	text_color_highlight[2] = 0.70f;
	text_color_highlight[3] = 1.00f;

	text_color_disabled[0] = 0.50f;
	text_color_disabled[1] = 0.50f;
	text_color_disabled[2] = 0.50f;
	text_color_disabled[3] = 1.00f;

	listbar_color[0] = 1.00f;
	listbar_color[1] = 1.00f;
	listbar_color[2] = 1.00f;
	listbar_color[3] = 0.25f;

	osp2be_themeActive = qtrue;
}

static void Osp2Be_PopMonochromeTheme( void ) {
	if ( !osp2be_themeActive ) {
		return;
	}

	Osp2Be_CopyColor( text_color_normal, osp2be_savedTextColorNormal );
	Osp2Be_CopyColor( text_color_highlight, osp2be_savedTextColorHighlight );
	Osp2Be_CopyColor( text_color_disabled, osp2be_savedTextColorDisabled );
	Osp2Be_CopyColor( listbar_color, osp2be_savedListbarColor );

	osp2be_themeActive = qfalse;
}

void Osp2Be_DrawBackground( void ) {
	vec4_t backgroundColor;
	vec4_t headerColor;
	vec4_t teamsParamsColor;
	vec4_t previewColor;
	vec4_t singleColumnParamsColor;
	int designOffsetX;
	int designCenterX;
	int screenCenterX;
	int categoryBottom;
	int paramsStartY;
	int paramsEndY;
	int headerLeft;
	int headerWidth;
	int sidePadding;
	int paramsWidth;
	int paramsX;

	backgroundColor[0] = 0.0f;
	backgroundColor[1] = 0.0f;
	backgroundColor[2] = 0.0f;
	backgroundColor[3] = s_osp2be.isIngameContext ? osp2be_bg_alpha.bg_ingame : osp2be_bg_alpha.bg_menu;

	headerColor[0] = 0.0f;
	headerColor[1] = 0.0f;
	headerColor[2] = 0.0f;
	headerColor[3] = s_osp2be.isIngameContext ? osp2be_bg_alpha.header_ingame : osp2be_bg_alpha.header_menu;

	teamsParamsColor[0] = 0.0f;
	teamsParamsColor[1] = 0.0f;
	teamsParamsColor[2] = 0.0f;
	teamsParamsColor[3] = s_osp2be.isIngameContext ? osp2be_bg_alpha.params_ingame : osp2be_bg_alpha.params_menu;

	previewColor[0] = 0.0f;
	previewColor[1] = 0.0f;
	previewColor[2] = 0.0f;
	previewColor[3] = s_osp2be.isIngameContext ? osp2be_bg_alpha.preview_ingame : osp2be_bg_alpha.preview_menu;

	singleColumnParamsColor[0] = 0.0f;
	singleColumnParamsColor[1] = 0.0f;
	singleColumnParamsColor[2] = 0.0f;
	singleColumnParamsColor[3] = s_osp2be.isIngameContext ? osp2be_bg_alpha.params_ingame : osp2be_bg_alpha.params_menu;

	designCenterX = osp2be_layout.design_width / 2;
	screenCenterX = (int)(uis.screenXmin + uis.screenXmax) / 2;
	designOffsetX = screenCenterX - designCenterX;
	
	UI_FillRect( designOffsetX, 0, osp2be_layout.design_width, osp2be_layout.design_height, backgroundColor );

	categoryBottom = s_osp2be.category.generic.bottom;
	if ( categoryBottom > 0 ) {
		int singlePageBgX;
		
		singlePageBgX = designOffsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);
		headerLeft = singlePageBgX + osp2be_bg_layout.header_padding_left;
		headerWidth = osp2be_layout.single_page_bg_width - osp2be_bg_layout.header_padding_left - osp2be_bg_layout.header_padding_right;
		UI_FillRect( headerLeft, 0, headerWidth, categoryBottom, headerColor );
	}

	sidePadding = osp2be_bg_layout.params_side_padding;
	paramsStartY = s_osp2be.defaultRowY[0] + osp2be_bg_layout.params_top_offset;
	paramsEndY = osp2be_layout.design_height - osp2be_bg_layout.params_bottom_offset;

	{
		int singlePageBgX;
		int buttonWidth;
		int rightColumnX;
		int enemyColumnX;
		int enemyColumnWidth;

		singlePageBgX = designOffsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);

		if ( s_osp2be.activeCategory == 3 ) {
			if ( paramsStartY > 0 && paramsEndY > paramsStartY ) {
				buttonWidth = ( s_osp2be.formWidth - osp2be_layout.content_gap ) / 2;
				rightColumnX = s_osp2be.contentLeft + buttonWidth + osp2be_layout.content_gap;
				enemyColumnX = rightColumnX + 24;
				enemyColumnWidth = buttonWidth;

				UI_FillRect( s_osp2be.contentLeft + sidePadding, paramsStartY, buttonWidth - sidePadding * 2, paramsEndY - paramsStartY, teamsParamsColor );
				UI_FillRect( enemyColumnX + sidePadding, paramsStartY, enemyColumnWidth - sidePadding * 2, paramsEndY - paramsStartY, teamsParamsColor );
			}
		} else {
			if ( paramsStartY > 0 && paramsEndY > paramsStartY ) {
				paramsX = singlePageBgX + sidePadding;
				paramsWidth = osp2be_layout.single_page_bg_width - sidePadding * 2;
				UI_FillRect( paramsX, paramsStartY, paramsWidth, paramsEndY - paramsStartY, singleColumnParamsColor );
			}
		}
	}

	if ( !( s_osp2be.preview.generic.flags & QMF_HIDDEN ) ) {
		UI_FillRect( s_osp2be.preview.generic.x, s_osp2be.preview.generic.y, s_osp2be.preview.width, s_osp2be.preview.height, previewColor );
	}

	{
		int flags;
		float titleX;
		int singlePageBgX;
		
		singlePageBgX = designOffsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);
		titleX = (float)(singlePageBgX + osp2be_layout.single_page_bg_width / 2);
		
		flags = osp2be_font.flags | DS_PROPORTIONAL | DS_HCENTER;
		UI_OSPDrawStringNew( titleX, 24.0f, "OSP2 Black Edition", color_white, color_black, 
		                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT, SCREEN_WIDTH, flags, NULL, NULL, NULL );
	}
}

void Osp2Be_DrawStatus( void ) {
	int elapsed;

	if ( s_osp2be.statusText[0] == '\0' ) {
		return;
	}

	elapsed = uis.realtime - s_osp2be.statusTime;
	if ( elapsed > 3000 ) {
		s_osp2be.statusText[0] = '\0';
		return;
	}

	Osp2Be_DrawText( (float)s_osp2be.contentLeft, (float)( osp2be_layout.design_height - 72 ), s_osp2be.statusText, UI_LEFT|UI_SMALLFONT, color_white );
}

void Osp2Be_DrawPreview( void *self ) {
	menubitmap_s *b;

	b = (menubitmap_s *)self;

	Osp2Be_UpdatePreviewModel();

	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_osp2be.previewInfo, uis.realtime / 2 );
}

void Osp2Be_DrawButton( void *self ) {
	menubitmap_s *b;
	int buttonIndex;
	const osp2beCvarDef_t *def;
	char modelBuffer[MAX_QPATH];
	char modelName[64];
	char skinName[64];
	vec4_t colorValue;
	qboolean hasColor;
	const char *value;
	int squareSize;
	int squareX;
	int squareY;
	int iconSize;
	int iconX;
	int iconY;
	int textX;
	int textY;
	const char *slash;
	char displayText[128];
	char iconPath[MAX_QPATH];
	qhandle_t iconShader;
	float iconColor[4];
	float sizeValueFloat;
	int sizeValue;
	qboolean useBuffered;

	b = (menubitmap_s *)self;
	buttonIndex = b->generic.id - ( ID_FIELD_BASE + OSP2BE_MAX_CONTROLS * 2 );
	
	if ( buttonIndex < 0 || buttonIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	def = s_osp2be.activeDefs[buttonIndex];
	if ( !def ) {
		return;
	}

	/* Draw background */
	UI_FillRect( b->generic.x, b->generic.y, b->width, b->height, osp2be_theme.button_bg );

	/* Draw border */
	if ( b->generic.flags & QMF_HASMOUSEFOCUS ) {
		UI_DrawRect( b->generic.x, b->generic.y, b->width, b->height, osp2be_theme.button_border_focus );
	} else {
		UI_DrawRect( b->generic.x, b->generic.y, b->width, b->height, osp2be_theme.button_border_normal );
	}

	if ( def->controlType == OSP2BE_CONTROL_COLOR_BUTTON ) {
		qhandle_t iconHandle = 0;
		const char *labelText = NULL;

		value = s_osp2be.fields[buttonIndex].field.buffer;
		hasColor = Osp2Be_ParseColorValue( value, colorValue );
		if ( !hasColor ) {
			Osp2Be_CopyColor( colorValue, osp2be_theme.button_default_color );
		}

		squareSize = b->height - osp2be_square.size_offset;
		if ( squareSize < osp2be_square.size_min ) {
			squareSize = b->height - osp2be_square.size_fallback_offset;
		}
		{
			int squareXOffset;
			if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
			     def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
				squareXOffset = osp2be_position.square_x_offset_weapon;
			} else {
				squareXOffset = osp2be_position.square_x_offset;
			}
			squareX = b->generic.x + squareXOffset;
		}
		squareY = b->generic.y + ( b->height - squareSize ) / 2;

		UI_FillRect( squareX + osp2be_square.border_offset, squareY + osp2be_square.border_offset, squareSize - osp2be_square.border_offset * 2, squareSize - osp2be_square.border_offset * 2, colorValue );
		UI_DrawRect( squareX, squareY, squareSize, squareSize, color_white );

		if ( def->special == OSP2BE_SPECIAL_RAIL_BEAM || def->special == OSP2BE_SPECIAL_RAIL_RING ) {
			if ( !osp2beRailIcon ) {
				osp2beRailIcon = trap_R_RegisterShaderNoMip( "icons/iconw_railgun" );
			}
			iconHandle = osp2beRailIcon;
			labelText = (def->special == OSP2BE_SPECIAL_RAIL_BEAM) ? "Beam" : "Rings";
		} else if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ||
		            def->special == OSP2BE_SPECIAL_MODEL_TORSO ||
		            def->special == OSP2BE_SPECIAL_MODEL_LEGS ) {
			/* Body parts - no icon, just label */
			iconHandle = 0;
			if ( def->label && def->label[0] ) {
				labelText = def->label;
			}
		} else if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
			def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
			int iconIndex;

			iconIndex = Osp2Be_GetWeaponIconIndex( def->special );
			if ( iconIndex >= 0 ) {
				iconHandle = osp2beWeaponIcons[iconIndex];
			}
			labelText = NULL;
		} else if ( def->label && def->label[0] ) {
			labelText = def->label;
		}

		{
			int contentGap;
			int contentX;

			if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
			     def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
				contentGap = osp2be_position.content_gap_weapon;
			} else {
				contentGap = osp2be_position.content_gap;
			}
			contentX = squareX + squareSize + contentGap;

			/* Skip icon for body parts */
			if ( iconHandle && 
			     def->special != OSP2BE_SPECIAL_MODEL_HEAD &&
			     def->special != OSP2BE_SPECIAL_MODEL_TORSO &&
			     def->special != OSP2BE_SPECIAL_MODEL_LEGS ) {
				iconSize = squareSize;
				iconX = contentX;
				iconY = b->generic.y + ( b->height - iconSize ) / 2;
				trap_R_SetColor( colorWhite );
				UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, iconHandle );
				trap_R_SetColor( NULL );
				contentX = iconX + iconSize + ((labelText && labelText[0]) ? osp2be_icon.gap : 0);
			}

			if ( labelText && labelText[0] ) {
				textX = contentX;
				textY = b->generic.y + ( b->height - SMALLCHAR_HEIGHT ) / 2;
				/* Use UI_OSPDrawStringNew directly for body parts */
				if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ||
				     def->special == OSP2BE_SPECIAL_MODEL_TORSO ||
				     def->special == OSP2BE_SPECIAL_MODEL_LEGS ) {
					int flags;
					
					flags = osp2be_font.flags | DS_SHADOW;
					UI_OSPDrawStringNew( (float)textX, (float)textY, labelText, color_white, color_black,
					                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT,
					                     SCREEN_WIDTH, flags, NULL, NULL, NULL );
				} else {
					Osp2Be_DrawText( (float)textX, (float)textY, labelText, UI_LEFT|UI_SMALLFONT,
					                 ( b->generic.flags & QMF_HASMOUSEFOCUS ) ? text_color_highlight : text_color_normal );
				}
			}
		}
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_SIZE_BUTTON ) {
		if ( s_osp2be.bufferedInitialized ) {
			sizeValue = s_osp2be.bufferedSizeValues[s_osp2be.activeCategory][buttonIndex];
		} else {
			sizeValueFloat = trap_Cvar_VariableValue( def->cvarName );
			sizeValue = (int)sizeValueFloat;
		}
		if ( sizeValue < 0 ) {
			sizeValue = 0;
		}

		if ( sizeValue == 0 ) {
			Osp2Be_CopyColor( iconColor, osp2be_theme.button_icon_disabled );
		} else {
			Osp2Be_CopyColor( iconColor, osp2be_theme.button_icon_enabled );
		}

		if ( !osp2beExpandIcon ) {
			osp2beExpandIcon = trap_R_RegisterShaderNoMip( "menu/be/expand.png" );
		}
		if ( osp2beExpandIcon ) {
			iconSize = b->height - 6;
			if ( iconSize < 12 ) {
				iconSize = b->height - 2;
			}
			if ( iconSize < 8 ) {
				iconSize = b->height;
			}
			iconX = b->generic.x + ( b->width - iconSize ) / 2;
			iconY = b->generic.y + ( b->height - iconSize ) / 2;
			trap_R_SetColor( iconColor );
			UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, osp2beExpandIcon );
			trap_R_SetColor( NULL );
		}
		return;
	}

	if ( def->controlType != OSP2BE_CONTROL_BUTTON ) {
		return;
	}

	if ( def->special == OSP2BE_SPECIAL_MODEL_DEFAULT ) {
		if ( !osp2beDefaultIcon ) {
			osp2beDefaultIcon = trap_R_RegisterShaderNoMip( "menu/be/default.png" );
		}
		if ( osp2beDefaultIcon ) {
			iconSize = b->height - 6;
			if ( iconSize < 12 ) {
				iconSize = b->height - 2;
			}
			if ( iconSize < 8 ) {
				iconSize = b->height;
			}
			iconX = b->generic.x + ( b->width - iconSize ) / 2;
			iconY = b->generic.y + ( b->height - iconSize ) / 2;
			trap_R_SetColor( NULL );
			UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, osp2beDefaultIcon );
		}
		return;
	}

	if ( def->special == OSP2BE_SPECIAL_SUPERHUD_CONFIG ) {
		char configName[MAX_QPATH];
		char displayName[MAX_QPATH];
		char buttonText[256];

		trap_Cvar_VariableStringBuffer( "ch_file", configName, sizeof( configName ) );
		if ( !configName[0] ) {
			Q_strncpyz( displayName, "default", sizeof( displayName ) );
		} else {
			Q_strncpyz( displayName, configName, sizeof( displayName ) );
		}

		Com_sprintf( buttonText, sizeof( buttonText ), "Current config: %s", displayName );

		textX = b->generic.x + 4;
		textY = b->generic.y + ( b->height - SMALLCHAR_HEIGHT ) / 2;
		Osp2Be_DrawText( (float)textX, (float)textY, buttonText, UI_LEFT|UI_SMALLFONT,
		                 ( b->generic.flags & QMF_HASMOUSEFOCUS ) ? text_color_highlight : text_color_normal );
		return;
	}

	useBuffered = qfalse;
	if ( def->special == OSP2BE_SPECIAL_MODEL_SELECTOR ) {
		if ( s_osp2be.activeCategory == 1 && s_osp2be.bufferedPlayerModel[0] ) {
			Q_strncpyz( modelBuffer, s_osp2be.bufferedPlayerModel, sizeof( modelBuffer ) );
			useBuffered = qtrue;
		} else if ( s_osp2be.activeCategory == 3 ) {
			if ( def->cvarName && !Q_stricmp( def->cvarName, "cg_teamModel" ) && s_osp2be.bufferedAllyModel[0] ) {
				Q_strncpyz( modelBuffer, s_osp2be.bufferedAllyModel, sizeof( modelBuffer ) );
				useBuffered = qtrue;
			} else if ( def->cvarName && !Q_stricmp( def->cvarName, "cg_enemyModel" ) && s_osp2be.bufferedEnemyModel[0] ) {
				Q_strncpyz( modelBuffer, s_osp2be.bufferedEnemyModel, sizeof( modelBuffer ) );
				useBuffered = qtrue;
			}
		}
	}

	if ( !useBuffered ) {
		trap_Cvar_VariableStringBuffer( def->cvarName, modelBuffer, sizeof( modelBuffer ) );
		if ( !modelBuffer[0] || !Q_stricmp( modelBuffer, "0" ) || !Q_stricmp( modelBuffer, "none" ) ) {
			trap_Cvar_VariableStringBuffer( "model", modelBuffer, sizeof( modelBuffer ) );
		}
		if ( !modelBuffer[0] ) {
			Q_strncpyz( modelBuffer, "sarge/default", sizeof( modelBuffer ) );
		}
	}

	/* Parse model/skin */
	slash = NULL;
	{
		const char *ptr;
		ptr = modelBuffer;
		while ( *ptr ) {
			if ( *ptr == '/' ) {
				slash = ptr;
				break;
			}
			ptr++;
		}
	}
	
	if ( slash ) {
		int nameLen;
		nameLen = slash - modelBuffer;
		if ( nameLen >= (int)sizeof( modelName ) ) {
			nameLen = sizeof( modelName ) - 1;
		}
		Q_strncpyz( modelName, modelBuffer, nameLen + 1 );
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
	} else {
		Q_strncpyz( modelName, modelBuffer, sizeof( modelName ) );
		Q_strncpyz( skinName, "default", sizeof( skinName ) );
	}

	/* Draw model head icon from icon_*.tga */
	iconSize = s_osp2be.rowHeight - osp2be_icon.size_offset;
	iconX = b->generic.x + osp2be_position.model_icon_x_offset;
	iconY = b->generic.y + (b->height - iconSize) / 2;

	/* Build path to icon: models/players/<modelname>/icon_<skinname>.tga */
	Com_sprintf( iconPath, sizeof( iconPath ), "models/players/%s/icon_%s.tga", modelName, skinName );
	iconShader = trap_R_RegisterShaderNoMip( iconPath );
	
	if ( iconShader ) {
		UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, iconShader );
	}

	/* Draw model/skin text */
	textX = iconX + iconSize + 4;
	textY = b->generic.y + (b->height - SMALLCHAR_HEIGHT) / 2;
	
	/* Capitalize first letter of model name */
	if ( modelName[0] >= 'a' && modelName[0] <= 'z' ) {
		modelName[0] = modelName[0] - 'a' + 'A';
	}
	if ( skinName[0] >= 'a' && skinName[0] <= 'z' ) {
		skinName[0] = skinName[0] - 'a' + 'A';
	}
	
	Com_sprintf( displayText, sizeof( displayText ), "%s/%s", modelName, skinName );
	Osp2Be_DrawText( (float)textX, (float)textY, displayText, UI_LEFT|UI_SMALLFONT,
	                 ( b->generic.flags & QMF_HASMOUSEFOCUS ) ? text_color_highlight : text_color_normal );
}

void Osp2Be_DrawTeamsButton( void *self ) {
	menubitmap_s *b;
	vec4_t background;
	vec4_t border;
	vec4_t textColor;
	const char *text;
	int flags;
	qboolean isActive;
	float textX;
	float textY;

	b = (menubitmap_s *)self;

	if ( b->generic.id == ID_TEAMS_ALLY ) {
		text = "Ally";
		isActive = ( s_osp2be.teamsPreviewMode == qfalse );
	} else if ( b->generic.id == ID_TEAMS_ENEMY ) {
		text = "Enemy";
		isActive = ( s_osp2be.teamsPreviewMode == qtrue );
	} else {
		return;
	}

	/* Background color */
	background[0] = osp2be_preview_button.bg_r;
	background[1] = osp2be_preview_button.bg_g;
	background[2] = osp2be_preview_button.bg_b;
	background[3] = isActive ? osp2be_preview_button.bg_alpha_active : osp2be_preview_button.bg_alpha_inactive;

	/* Border color */
	if ( b->generic.flags & QMF_HASMOUSEFOCUS ) {
		border[0] = osp2be_preview_button.border_r;
		border[1] = osp2be_preview_button.border_g;
		border[2] = osp2be_preview_button.border_b;
	} else if ( isActive ) {
		border[0] = osp2be_preview_button.border_r;
		border[1] = osp2be_preview_button.border_g;
		border[2] = osp2be_preview_button.border_b;
	} else {
		border[0] = osp2be_preview_button.border_inactive_r;
		border[1] = osp2be_preview_button.border_inactive_g;
		border[2] = osp2be_preview_button.border_inactive_b;
	}
	border[3] = 1.0f;

	/* Text color */
	if ( b->generic.flags & QMF_HASMOUSEFOCUS ) {
		Osp2Be_CopyColor( textColor, osp2be_theme.preview_text_focus );
	} else if ( isActive ) {
		Osp2Be_CopyColor( textColor, osp2be_theme.preview_text_active );
	} else {
		Osp2Be_CopyColor( textColor, osp2be_theme.preview_text_inactive );
	}

	/* Draw background and border */
	UI_FillRect( b->generic.x, b->generic.y, b->width, b->height, background );
	UI_DrawRect( b->generic.x, b->generic.y, b->width, b->height, border );

	/* Draw text using UI_OSPDrawStringNew */
	flags = osp2be_font.flags | DS_HCENTER | DS_VCENTER | DS_SHADOW;
	textX = (float)b->generic.x + (float)b->width / 2.0f;
	textY = (float)b->generic.y + (float)b->height / 2.0f;
	
	UI_OSPDrawStringNew( textX, textY, text, textColor, color_black,
	                     (float)SMALLCHAR_WIDTH, (float)SMALLCHAR_HEIGHT,
	                     SCREEN_WIDTH, flags, NULL, NULL, NULL );
}

void Osp2Be_DrawFrozenButton( void *self ) {
	menubitmap_s *b;
	vec4_t background;
	vec4_t border;
	vec4_t iconColor;

	b = (menubitmap_s *)self;

	if ( !osp2beFrozenIcon ) {
		return;
	}

	background[0] = osp2be_preview_button.bg_r;
	background[1] = osp2be_preview_button.bg_g;
	background[2] = osp2be_preview_button.bg_b;
	background[3] = s_osp2be.previewFrozenActive ? osp2be_preview_button.bg_alpha_active : osp2be_preview_button.bg_alpha_inactive;

	if ( b->generic.flags & QMF_HASMOUSEFOCUS ) {
		border[0] = osp2be_preview_button.border_r;
		border[1] = osp2be_preview_button.border_g;
		border[2] = osp2be_preview_button.border_b;
	} else if ( s_osp2be.previewFrozenActive ) {
		border[0] = osp2be_preview_button.border_r;
		border[1] = osp2be_preview_button.border_g;
		border[2] = osp2be_preview_button.border_b;
	} else {
		border[0] = osp2be_preview_button.border_inactive_r;
		border[1] = osp2be_preview_button.border_inactive_g;
		border[2] = osp2be_preview_button.border_inactive_b;
	}
	border[3] = 1.0f;

	UI_FillRect( b->generic.x, b->generic.y, b->width, b->height, background );
	UI_DrawRect( b->generic.x, b->generic.y, b->width, b->height, border );

	if ( s_osp2be.previewFrozenActive ) {
		Osp2Be_CopyColor( iconColor, osp2be_theme.preview_icon_active );
	} else {
		Osp2Be_CopyColor( iconColor, osp2be_theme.preview_icon_inactive );
	}

	trap_R_SetColor( iconColor );
	UI_DrawHandlePic( b->generic.x + 4, b->generic.y + 4, b->width - 8, b->height - 8, osp2beFrozenIcon );
	trap_R_SetColor( NULL );
}

void Osp2Be_DrawColorSwatches( void ) {
	int i;
	int swatchX;
	int swatchY;
	vec4_t color;
	const osp2beCvarDef_t *def;

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		def = s_osp2be.activeDefs[i];
		if ( !def || def->controlType != OSP2BE_CONTROL_FIELD ) {
			continue;
		}

		if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
			 def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
			continue;
		}

		if ( !Osp2Be_CanUseColorPicker( def ) ) {
			continue;
		}

		if ( s_osp2be.fields[i].generic.flags & QMF_HIDDEN ) {
			continue;
		}

		swatchX = s_osp2be.fields[i].generic.x - osp2be_layout.color_swatch_size - osp2be_layout.color_swatch_gap;
		swatchY = s_osp2be.fields[i].generic.y + (SMALLCHAR_HEIGHT - osp2be_layout.color_swatch_size) / 2;

		if ( Osp2Be_ParseColorValue( s_osp2be.fields[i].field.buffer, color ) ) {
			UI_FillRect( swatchX, swatchY, osp2be_layout.color_swatch_size, osp2be_layout.color_swatch_size, color );
			UI_DrawRect( swatchX, swatchY, osp2be_layout.color_swatch_size, osp2be_layout.color_swatch_size, colorWhite );
		}
	}
}

void Osp2Be_DrawToggleButton( void *self ) {
	menuradiobutton_s *toggle;
	const osp2beCvarDef_t *def;
	int index;
	int width;
	int height;
	float backgroundColor[4];
	float borderColor[4];
	float textColor[4];
	int textX;
	int textY;
	int boxSize;
	int boxX;
	int boxY;
	int iconSize;
	int iconX;
	int iconY;
	qboolean checked;

	toggle = (menuradiobutton_s *)self;
	if ( !toggle ) {
		return;
	}

	index = toggle->generic.id - ID_TOGGLE_BASE;
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	def = s_osp2be.activeDefs[index];
	if ( !def || def->special != OSP2BE_SPECIAL_TOGGLE_CHECKBOX ) {
		return;
	}

	width = toggle->generic.right - toggle->generic.left;
	if ( width <= 0 ) {
		width = s_osp2be.formWidth;
	}
	if ( width <= 0 ) {
		width = 200;
	}
	height = toggle->generic.bottom - toggle->generic.top;
	if ( height <= 0 ) {
		height = s_osp2be.rowHeight - 4;
	}
	if ( height <= 0 ) {
		height = SMALLCHAR_HEIGHT + 6;
	}

	checked = ( toggle->curvalue != 0 );

	if ( toggle->generic.flags & QMF_HASMOUSEFOCUS ) {
		Osp2Be_CopyColor( backgroundColor, osp2be_theme.toggle_bg_focus );
		Osp2Be_CopyColor( borderColor, osp2be_theme.toggle_border_focus );
	} else {
		Osp2Be_CopyColor( backgroundColor, osp2be_theme.toggle_bg_normal );
		Osp2Be_CopyColor( borderColor, osp2be_theme.toggle_border_normal );
	}

	UI_FillRect( toggle->generic.left, toggle->generic.top, width, height, backgroundColor );
	UI_DrawRect( toggle->generic.left, toggle->generic.top, width, height, borderColor );

	textX = toggle->generic.left + 12;
	textY = toggle->generic.top + ( height - SMALLCHAR_HEIGHT ) / 2;
	if ( textY < toggle->generic.top + 2 ) {
		textY = toggle->generic.top + 2;
	}

	if ( checked ) {
		Osp2Be_CopyColor( textColor, osp2be_theme.toggle_text_checked );
	} else {
		Osp2Be_CopyColor( textColor, osp2be_theme.toggle_text_unchecked );
	}

	if ( def->label && def->label[0] ) {
		Osp2Be_DrawText( (float)textX, (float)textY, def->label, UI_LEFT|UI_SMALLFONT, textColor );
	}

	boxSize = height - 10;
	if ( boxSize < 14 ) {
		boxSize = height - 6;
	}
	if ( boxSize < 12 ) {
		boxSize = 12;
	}
	boxX = toggle->generic.left + width - boxSize - 10;
	boxY = toggle->generic.top + ( height - boxSize ) / 2;

	UI_DrawRect( boxX, boxY, boxSize, boxSize, borderColor );

	if ( checked ) {
		if ( !osp2beReadyIcon ) {
			osp2beReadyIcon = trap_R_RegisterShaderNoMip( "icons/ready" );
		}
		if ( osp2beReadyIcon ) {
			iconSize = boxSize - 6;
			if ( iconSize < 8 ) {
				iconSize = boxSize - 4;
			}
			if ( iconSize < 6 ) {
				iconSize = boxSize;
			}
			iconX = boxX + ( boxSize - iconSize ) / 2;
			iconY = boxY + ( boxSize - iconSize ) / 2;
			trap_R_SetColor( colorWhite );
			UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, osp2beReadyIcon );
			trap_R_SetColor( NULL );
		}
	}
}

void Osp2Be_DrawSlider( void *self ) {
	menuslider_s *s;
	vec4_t trackColor;
	vec4_t borderColor;
	vec4_t tickColor;
	vec4_t handleColor;
	int trackX;
	int trackY;
	int trackWidth;
	int trackHeight;
	int handleWidth;
	int handleHeight;
	int handleHalf;
	int handleX;
	int handleY;
	int tickCount;
	int i;
	int tickX;
	qboolean focus;

	s = (menuslider_s *)self;

	if ( !s ) {
		return;
	}

	focus = ( s->generic.parent && s->generic.parent->cursor == s->generic.menuPosition );

	if ( s->generic.flags & QMF_GRAYED ) {
		Osp2Be_CopyColor( trackColor, osp2be_theme.slider_track_disabled );
		Osp2Be_CopyColor( borderColor, osp2be_theme.slider_border_disabled );
		Osp2Be_CopyColor( tickColor, osp2be_theme.slider_tick_disabled );
		Osp2Be_CopyColor( handleColor, osp2be_theme.slider_handle_disabled );
	} else if ( focus ) {
		Osp2Be_CopyColor( trackColor, osp2be_theme.slider_track_focus );
		Osp2Be_CopyColor( borderColor, osp2be_theme.slider_border_focus );
		Osp2Be_CopyColor( tickColor, osp2be_theme.slider_tick_focus );
		Osp2Be_CopyColor( handleColor, osp2be_theme.slider_handle_focus );
	} else {
		Osp2Be_CopyColor( trackColor, osp2be_theme.slider_track_normal );
		Osp2Be_CopyColor( borderColor, osp2be_theme.slider_border_normal );
		Osp2Be_CopyColor( tickColor, osp2be_theme.slider_tick_normal );
		Osp2Be_CopyColor( handleColor, osp2be_theme.slider_handle_normal );
	}

	trackX = s->generic.left;
	trackWidth = s->generic.right - s->generic.left;
	if ( trackWidth < 1 ) {
		trackWidth = 1;
	}

	trackHeight = 6;
	trackY = s->generic.y + (SMALLCHAR_HEIGHT - trackHeight) / 2;

	UI_FillRect( trackX, trackY, trackWidth, trackHeight, trackColor );
	UI_DrawRect( trackX - 1, trackY - 1, trackWidth + 2, trackHeight + 2, borderColor );

	tickCount = SLIDER_RANGE;
	if ( tickCount < 1 ) {
		tickCount = 1;
	}
	{
		float tickStep;

		tickStep = trackWidth / (float)tickCount;
		for ( i = 0; i <= tickCount; i++ ) {
			tickX = trackX + (int)( tickStep * i + 0.5f );
			UI_FillRect( tickX, trackY - 5, 1, trackHeight + 10, tickColor );
		}
	}

	if ( s->maxvalue > s->minvalue ) {
		s->range = ( s->curvalue - s->minvalue ) / ( s->maxvalue - s->minvalue );
	} else {
		s->range = 0.0f;
	}
	if ( s->range < 0.0f ) {
		s->range = 0.0f;
	}
	if ( s->range > 1.0f ) {
		s->range = 1.0f;
	}

	handleWidth = 12;
	handleHeight = trackHeight + 12;
	handleHalf = handleWidth / 2;
	handleX = trackX + (int)( trackWidth * s->range + 0.5f );
	if ( handleX < trackX ) {
		handleX = trackX;
	}
	if ( handleX > trackX + trackWidth ) {
		handleX = trackX + trackWidth;
	}
	handleY = trackY - (handleHeight - trackHeight) / 2;

	UI_FillRect( handleX - handleHalf, handleY, handleWidth, handleHeight, handleColor );
	UI_DrawRect( handleX - handleHalf, handleY, handleWidth, handleHeight, borderColor );
}
void Osp2Be_DrawCategoryList( void *self ) {
	menulist_s *l;
	int x;
	int column;
	int y;
	int base;
	int i;
	int style;
	vec4_t drawColor;
	const char *itemText;
	int hoverIndex;
	int itemX;
	int itemY;
	int itemW;
	int itemH;
	vec4_t borderColor;
	vec4_t backgroundColor;
	int checkX;
	int checkY;
	int checkW;
	int checkH;
	int itemStartX;
	int itemEndX;

	l = (menulist_s *)self;
	if ( !l ) {
		return;
	}

	hoverIndex = -1;
	if ( l->generic.flags & QMF_HASMOUSEFOCUS ) {
		checkX = l->generic.x;
		checkY = l->generic.y - 2;
		checkW = ( ( l->width + l->seperation ) * l->columns - l->seperation ) * SMALLCHAR_WIDTH;
		checkH = SMALLCHAR_HEIGHT + 4;

		if ( UI_CursorInRect( checkX, checkY, checkW, checkH ) ) {
			itemStartX = checkX;
			for ( i = 0; i < l->numitems; i++ ) {
				itemW = l->width * SMALLCHAR_WIDTH;
				itemEndX = itemStartX + itemW;
				if ( uis.cursorx >= itemStartX && uis.cursorx < itemEndX &&
				     uis.cursory >= checkY && uis.cursory < checkY + checkH ) {
					hoverIndex = i;
					break;
				}
				itemStartX = itemEndX + l->seperation * SMALLCHAR_WIDTH;
			}
		}
	}

	x = l->generic.x;
	for ( column = 0; column < l->columns; column++ ) {
		y = l->generic.y;
		base = l->top + column * l->height;
		for ( i = base; i < base + l->height; i++ ) {
			if ( i >= l->numitems ) {
				break;
			}

			itemW = l->width * SMALLCHAR_WIDTH;
			itemH = SMALLCHAR_HEIGHT + 4;
			itemX = x;
			itemY = y - 2;

			if ( i == l->curvalue ) {
				Osp2Be_CopyColor( backgroundColor, osp2be_theme.category_bg_selected );
				Osp2Be_CopyColor( borderColor, osp2be_theme.category_border_selected );
				Osp2Be_CopyColor( drawColor, osp2be_theme.category_text_selected );
				style = UI_CENTER|UI_SMALLFONT;
			} else if ( i == hoverIndex ) {
				Osp2Be_CopyColor( backgroundColor, osp2be_theme.category_bg_hover );
				Osp2Be_CopyColor( borderColor, osp2be_theme.category_border_hover );
				Osp2Be_CopyColor( drawColor, osp2be_theme.category_text_hover );
				style = UI_CENTER|UI_SMALLFONT;
			} else {
				Osp2Be_CopyColor( backgroundColor, osp2be_theme.category_bg_normal );
				Osp2Be_CopyColor( borderColor, osp2be_theme.category_border_normal );
				Osp2Be_CopyColor( drawColor, osp2be_theme.category_text_normal );
				style = UI_CENTER|UI_SMALLFONT;
			}

			UI_FillRect( itemX, itemY, itemW, itemH, backgroundColor );
			UI_DrawRect( itemX, itemY, itemW, itemH, borderColor );

			if ( l->itemnames && l->itemnames[i] ) {
				itemText = l->itemnames[i];
			} else {
				itemText = "";
			}

			Osp2Be_DrawText( (float)itemX + (float)itemW / 2.0f, (float)y, itemText, style, drawColor );

			y += SMALLCHAR_HEIGHT;
		}

		x += ( l->width + l->seperation ) * SMALLCHAR_WIDTH;
	}
}

static qboolean osp2be_theme_initialized = qfalse;

void Osp2Be_MenuDraw( void ) {
	if ( !osp2be_theme_initialized ) {
		Osp2Be_InitTheme();
		osp2be_theme_initialized = qtrue;
	}

	if ( s_osp2be.category.curvalue != s_osp2be.activeCategory ) {
		Osp2Be_SelectCategory( s_osp2be.category.curvalue );
	}

	if ( UI_ColorPicker_IsActive( &s_osp2be.colorPicker ) && trap_Key_IsDown( K_MOUSE1 ) ) {
		UI_ColorPicker_HandleMouse( &s_osp2be.colorPicker, uis.cursorx, uis.cursory, qfalse );
	} else if ( trap_Key_IsDown( K_MOUSE1 ) ) {
		Osp2Be_HandleSliderMouse( uis.cursorx, uis.cursory );
	}

	Osp2Be_PushMonochromeTheme();
	Osp2Be_DrawBackground();
	Menu_Draw( &s_osp2be.menu );
	Osp2Be_PopMonochromeTheme();
	Osp2Be_DrawColorSwatches();
	Osp2Be_DrawStatus();
	UI_ColorPicker_Draw( &s_osp2be.colorPicker );
}

