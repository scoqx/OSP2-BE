#include "ui_osp2be_shared.h"

/* Outline size names for list control */
static const char *osp2beOutlineSizeNames[] = { "0", "1", "2", "3", NULL };

#define OSP2BE_STD_BUTTON_MIN_WIDTH 180
#define OSP2BE_SIZE_BUTTON_MIN_WIDTH 32
#define OSP2BE_SIZE_BUTTON_BASE_WIDTH 36
#define OSP2BE_COLOR_BUTTON_MIN_WIDTH 60
#define OSP2BE_COLOR_BUTTON_PADDING 10
#define OSP2BE_SIZE_BUTTON_PADDING 16

static void Osp2Be_GetSliderBounds( const osp2beCvarDef_t *def, float *minValue, float *maxValue ) {
	if ( minValue ) {
		*minValue = 0.0f;
	}
	if ( maxValue ) {
		*maxValue = 1.0f;
	}

	if ( !def ) {
		return;
	}

	switch ( def->special ) {
	case OSP2BE_SPECIAL_SLIDER_SENSITIVITY:
		if ( minValue ) {
			*minValue = 0.0f;
		}
		if ( maxValue ) {
			*maxValue = 20.0f;
		}
		break;
	case OSP2BE_SPECIAL_SLIDER_FOV:
		if ( minValue ) {
			*minValue = 80.0f;
		}
		if ( maxValue ) {
			*maxValue = 130.0f;
		}
		break;
	default:
		break;
	}
}

int Osp2Be_SliderGetPrecision( const osp2beCvarDef_t *def ) {
	if ( !def ) {
		return 0;
	}

	switch ( def->special ) {
	case OSP2BE_SPECIAL_SLIDER_SENSITIVITY:
		return 2;
	case OSP2BE_SPECIAL_SLIDER_FOV:
		return 0;
	default:
		return 2;
	}
}

float Osp2Be_SliderClampValue( const osp2beCvarDef_t *def, float value ) {
	float minValue;
	float maxValue;

	Osp2Be_GetSliderBounds( def, &minValue, &maxValue );

	if ( value < minValue ) {
		return minValue;
	}
	if ( value > maxValue ) {
		return maxValue;
	}
	return value;
}

static void Osp2Be_FormatSliderValue( const osp2beCvarDef_t *def, float value, char *buffer, int bufferSize ) {
	int precision;
	int decimals;
	int len;

	if ( !buffer || bufferSize <= 0 ) {
		return;
	}

	precision = Osp2Be_SliderGetPrecision( def );
	if ( precision <= 0 ) {
		Com_sprintf( buffer, bufferSize, "%d", (int)(value + 0.5f) );
		return;
	}

	Com_sprintf( buffer, bufferSize, "%.*f", precision, value );
	len = Q_PrintStrlen( buffer );
	decimals = precision;

	while ( len > 0 && decimals > 0 && buffer[len - 1] == '0' ) {
		buffer[len - 1] = '\0';
		len--;
		decimals--;
	}
	if ( len > 0 && buffer[len - 1] == '.' ) {
		buffer[len - 1] = '\0';
	}
}

float Osp2Be_ParseSliderInput( const char *text, float fallback ) {
	const char *ptr;
	float value;
	float fraction;
	float factor;
	int sign;
	qboolean haveDigit;

	if ( !text ) {
		return fallback;
	}

	ptr = text;
	while ( *ptr == ' ' || *ptr == '\t' ) {
		ptr++;
	}

	sign = 1;
	if ( *ptr == '+' ) {
		ptr++;
	} else if ( *ptr == '-' ) {
		sign = -1;
		ptr++;
	}

	value = 0.0f;
	fraction = 0.0f;
	factor = 0.1f;
	haveDigit = qfalse;

	while ( *ptr >= '0' && *ptr <= '9' ) {
		value = value * 10.0f + (float)( *ptr - '0' );
		ptr++;
		haveDigit = qtrue;
	}

	if ( *ptr == '.' ) {
		ptr++;
		while ( *ptr >= '0' && *ptr <= '9' ) {
			fraction += factor * (float)( *ptr - '0' );
			factor *= 0.1f;
			ptr++;
			haveDigit = qtrue;
		}
	}

	if ( !haveDigit ) {
		return fallback;
	}

	value = ( value + fraction ) * (float)sign;
	return value;
}

void Osp2Be_SliderSetValue( int index, const osp2beCvarDef_t *def, float value ) {
	float minValue;
	float maxValue;
	float clamped;
	char valueBuffer[MAX_EDIT_LINE];

	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	Osp2Be_GetSliderBounds( def, &minValue, &maxValue );
	clamped = Osp2Be_SliderClampValue( def, value );

	s_osp2be.sliders[index].minvalue = minValue;
	s_osp2be.sliders[index].maxvalue = maxValue;
	s_osp2be.sliders[index].curvalue = clamped;

	if ( maxValue > minValue ) {
		s_osp2be.sliders[index].range = ( clamped - minValue ) / ( maxValue - minValue );
		if ( s_osp2be.sliders[index].range < 0.0f ) {
			s_osp2be.sliders[index].range = 0.0f;
		}
		if ( s_osp2be.sliders[index].range > 1.0f ) {
			s_osp2be.sliders[index].range = 1.0f;
		}
	} else {
		s_osp2be.sliders[index].range = 0.0f;
	}

	Osp2Be_FormatSliderValue( def, clamped, valueBuffer, sizeof( valueBuffer ) );
	Q_strncpyz( s_osp2be.fields[index].field.buffer, valueBuffer, sizeof( s_osp2be.fields[index].field.buffer ) );
	Osp2Be_ClampFieldLength( &s_osp2be.fields[index] );
}

void Osp2Be_SliderRefreshField( int index, const osp2beCvarDef_t *def ) {
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	Osp2Be_SliderSetValue( index, def, s_osp2be.sliders[index].curvalue );
}

static void Osp2Be_SetButtonGeometry( menubitmap_s *button, int x, int y, int width, int height ) {
	if ( !button ) {
		return;
	}

	if ( width < 0 ) {
		width = 0;
	}
	if ( height < 0 ) {
		height = 0;
	}

	button->generic.x = x;
	button->generic.y = y;
	button->width = width;
	button->height = height;
	button->generic.left = x;
	button->generic.top = y;
	button->generic.right = x + width;
	button->generic.bottom = y + height;
}

void Osp2Be_ReadFieldValue( const osp2beCvarDef_t *def, char *buffer, int bufferSize ) {
	int modelSegment;
	int railSegment;

	if ( bufferSize <= 0 ) {
		return;
	}

	buffer[0] = '\0';

	if ( !def || !def->cvarName ) {
		return;
	}

	modelSegment = Osp2Be_GetModelSegmentIndex( def->special );
	if ( modelSegment >= 0 ) {
		Osp2Be_ReadModelSegment( def->cvarName, modelSegment, buffer, bufferSize );
		return;
	}

	railSegment = Osp2Be_GetRailSegmentIndex( def->special );
	if ( railSegment >= 0 ) {
		Osp2Be_ReadRailSegment( def->cvarName, railSegment, buffer, bufferSize );
		return;
	}

	trap_Cvar_VariableStringBuffer( def->cvarName, buffer, bufferSize );
}

void Osp2Be_ClampFieldLength( menufield_s *field ) {
	int len;

	len = Q_PrintStrlen( field->field.buffer );
	if ( len < 0 ) {
		len = 0;
	}
	if ( len > field->field.maxchars ) {
		len = field->field.maxchars;
		field->field.buffer[len] = '\0';
	}
	field->field.cursor = len;
}

void Osp2Be_WriteFieldValue( const osp2beCvarDef_t *def, const char *value, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	int modelSegment;
	int railSegment;
	int index;
	int j;

	if ( !def || !value ) {
		return;
	}

	modelSegment = Osp2Be_GetModelSegmentIndex( def->special );
	if ( modelSegment >= 0 ) {
		index = Osp2Be_FindModelAssemblyIndex( modelAssemblies, *modelAssemblyCount, def->cvarName );
		if ( index < 0 ) {
			if ( *modelAssemblyCount >= OSP2BE_MAX_CONTROLS ) {
				return;
			}
			index = *modelAssemblyCount;
			modelAssemblies[index].cvarName = def->cvarName;
			for ( j = 0; j < 3; j++ ) {
				modelAssemblies[index].segments[j][0] = '\0';
				modelAssemblies[index].used[j] = qfalse;
			}
			(*modelAssemblyCount)++;
		}
		Q_strncpyz( modelAssemblies[index].segments[modelSegment], value, sizeof( modelAssemblies[index].segments[modelSegment] ) );
		modelAssemblies[index].used[modelSegment] = qtrue;
		return;
	}

	railSegment = Osp2Be_GetRailSegmentIndex( def->special );
	if ( railSegment >= 0 ) {
		index = Osp2Be_FindRailAssemblyIndex( railAssemblies, *railAssemblyCount, def->cvarName );
		if ( index < 0 ) {
			if ( *railAssemblyCount >= OSP2BE_MAX_CONTROLS ) {
				return;
			}
			index = *railAssemblyCount;
			railAssemblies[index].cvarName = def->cvarName;
			for ( j = 0; j < 2; j++ ) {
				railAssemblies[index].segments[j][0] = '\0';
				railAssemblies[index].used[j] = qfalse;
			}
			(*railAssemblyCount)++;
		}
		Q_strncpyz( railAssemblies[index].segments[railSegment], value, sizeof( railAssemblies[index].segments[railSegment] ) );
		railAssemblies[index].used[railSegment] = qtrue;
		return;
	}

	trap_Cvar_Set( def->cvarName, value );
}

void Osp2Be_SetupFieldControl( int index, const osp2beCvarDef_t *def ) {
	char valueBuffer[MAX_EDIT_LINE];
	int formRight;
	int defaultFieldX;
	int rowTop;

	if ( !def ) {
		return;
	}

	rowTop = s_osp2be.labels[index].generic.y;
	formRight = s_osp2be.contentLeft + s_osp2be.formWidth;
	defaultFieldX = formRight - ( osp2be_layout.field_width_chars * SMALLCHAR_WIDTH ) - osp2be_layout.field_padding + osp2be_layout.color_swatch_size + osp2be_layout.color_swatch_gap;

	if ( s_osp2be.bufferedInitialized ) {
		Q_strncpyz( valueBuffer, s_osp2be.bufferedFieldValues[s_osp2be.activeCategory][index], sizeof( valueBuffer ) );
	} else {
		Osp2Be_ReadFieldValue( def, valueBuffer, sizeof( valueBuffer ) );
	}
	Q_strncpyz( s_osp2be.fields[index].field.buffer, valueBuffer, sizeof( s_osp2be.fields[index].field.buffer ) );
	Osp2Be_ClampFieldLength( &s_osp2be.fields[index] );

	s_osp2be.fields[index].field.widthInChars = osp2be_layout.field_width_chars;
	s_osp2be.fields[index].generic.x = defaultFieldX;
	s_osp2be.fields[index].generic.y = rowTop;
	s_osp2be.fields[index].generic.left = defaultFieldX - 4;
	s_osp2be.fields[index].generic.right = defaultFieldX + osp2be_layout.field_width_chars * SMALLCHAR_WIDTH + 4;
	s_osp2be.fields[index].generic.top = rowTop - 2;
	s_osp2be.fields[index].generic.bottom = rowTop + SMALLCHAR_HEIGHT + 2;
	s_osp2be.fields[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
}

void Osp2Be_SetupToggleControl( int index, const osp2beCvarDef_t *def ) {
	float value;
	qboolean useCheckbox;
	int rowTop;
	int height;
	int width;
	int x;

	if ( !def ) {
		return;
	}

	useCheckbox = ( def->special == OSP2BE_SPECIAL_TOGGLE_CHECKBOX );
	if ( s_osp2be.bufferedInitialized ) {
		value = s_osp2be.bufferedToggleValues[s_osp2be.activeCategory][index];
	} else if ( def->cvarName && def->cvarName[0] ) {
		value = trap_Cvar_VariableValue( def->cvarName );
	} else {
		value = 0.0f;
	}
	s_osp2be.toggles[index].generic.name = NULL;
	s_osp2be.toggles[index].curvalue = value != 0.0f;
	s_osp2be.toggles[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	if ( useCheckbox ) {
		rowTop = s_osp2be.labels[index].generic.y;
		height = s_osp2be.rowHeight - 4;
		if ( height < SMALLCHAR_HEIGHT + 6 ) {
			height = SMALLCHAR_HEIGHT + 6;
		}
		width = s_osp2be.formWidth;
		if ( width < SMALLCHAR_WIDTH * 8 ) {
			width = SMALLCHAR_WIDTH * 8;
		}
		x = s_osp2be.contentLeft;

		s_osp2be.toggles[index].generic.x = x;
		s_osp2be.toggles[index].generic.y = rowTop;
		s_osp2be.toggles[index].generic.left = x;
		s_osp2be.toggles[index].generic.right = x + width;
		s_osp2be.toggles[index].generic.top = rowTop;
		s_osp2be.toggles[index].generic.bottom = rowTop + height;
		s_osp2be.toggles[index].generic.ownerdraw = Osp2Be_DrawToggleButton;
		s_osp2be.labels[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	} else {
		s_osp2be.toggles[index].generic.x = s_osp2be.fields[index].generic.x;
		s_osp2be.toggles[index].generic.y = s_osp2be.fields[index].generic.y;
		s_osp2be.toggles[index].generic.left = s_osp2be.toggles[index].generic.x - 16;
		s_osp2be.toggles[index].generic.right = s_osp2be.toggles[index].generic.x + 16;
		s_osp2be.toggles[index].generic.top = s_osp2be.toggles[index].generic.y - 4;
		s_osp2be.toggles[index].generic.bottom = s_osp2be.toggles[index].generic.y + SMALLCHAR_HEIGHT + 4;
		s_osp2be.toggles[index].generic.ownerdraw = NULL;
		s_osp2be.labels[index].generic.flags &= ~QMF_HIDDEN;
		s_osp2be.labels[index].generic.flags &= ~QMF_INACTIVE;
	}

	s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.fields[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
}

void Osp2Be_SetupListControl( int index, const osp2beCvarDef_t *def ) {
	int intValue;

	if ( !def || !def->cvarName ) {
		return;
	}

	if ( s_osp2be.bufferedInitialized ) {
		intValue = s_osp2be.bufferedListValues[s_osp2be.activeCategory][index];
	} else {
		intValue = (int)trap_Cvar_VariableValue( def->cvarName );
	}
	if ( intValue < 0 ) {
		intValue = 0;
	}
	if ( intValue > 3 ) {
		intValue = 3;
	}

	s_osp2be.lists[index].generic.x = s_osp2be.fields[index].generic.x;
	s_osp2be.lists[index].generic.y = s_osp2be.fields[index].generic.y;
	s_osp2be.lists[index].itemnames = osp2beOutlineSizeNames;
	s_osp2be.lists[index].numitems = 4;
	s_osp2be.lists[index].curvalue = intValue;
	s_osp2be.lists[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.fields[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
}

void Osp2Be_SetupButtonControl( int index, const osp2beCvarDef_t *def ) {
	int buttonWidth;
	int textChars;
	int textWidth;
	int x;
	int y;
	int sidePadding;

	if ( !def ) {
		return;
	}

	/* Special handling for SuperHUD config button - center it on screen */
	if ( def->special == OSP2BE_SPECIAL_SUPERHUD_CONFIG ) {
		sidePadding = osp2be_bg_layout.params_side_padding;
		buttonWidth = s_osp2be.formWidth - sidePadding * 2;
		x = ( osp2be_layout.design_width - buttonWidth ) / 2;
	} else {
		if ( def->label && def->label[0] ) {
			textChars = Osp2Be_StringLength( def->label );
		} else {
			textChars = 0;
		}
		textWidth = textChars * SMALLCHAR_WIDTH;

		buttonWidth = textWidth + OSP2BE_SIZE_BUTTON_PADDING * 2;
		if ( buttonWidth < OSP2BE_STD_BUTTON_MIN_WIDTH ) {
			buttonWidth = OSP2BE_STD_BUTTON_MIN_WIDTH;
		}
		if ( buttonWidth > s_osp2be.formWidth ) {
			buttonWidth = s_osp2be.formWidth;
		}

		x = s_osp2be.contentLeft;
	}

	y = s_osp2be.fields[index].generic.y;
	Osp2Be_SetButtonGeometry( &s_osp2be.buttons[index], x, y, buttonWidth, osp2be_button.height );
	s_osp2be.buttons[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.fields[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
}

void Osp2Be_SetupColorButtonControl( int index, const osp2beCvarDef_t *def ) {
	char valueBuffer[MAX_EDIT_LINE];
	int buttonWidth;
	int textChars;
	int textWidth;
	int swatchWidth;
	int iconWidth;
	int x;
	int y;

	if ( !def ) {
		return;
	}

	if ( s_osp2be.bufferedInitialized ) {
		Q_strncpyz( valueBuffer, s_osp2be.bufferedFieldValues[s_osp2be.activeCategory][index], sizeof( valueBuffer ) );
	} else {
		Osp2Be_ReadFieldValue( def, valueBuffer, sizeof( valueBuffer ) );
	}
	Q_strncpyz( s_osp2be.fields[index].field.buffer, valueBuffer, sizeof( s_osp2be.fields[index].field.buffer ) );
	Osp2Be_ClampFieldLength( &s_osp2be.fields[index] );

	if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
		 def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
		int order;
		int headerIndex;
		int buttonsPerRow;
		int totalButtons;
		int row;
		int col;
		int colCount;
		int remaining;
		int buttonWidth;
		int buttonHeight;
		int spacing;
#ifdef __GNUC__
		__attribute__((unused))
#endif
		double rowWidth;
		int rowStartX;
		int buttonX;
		int buttonY;
		int baseRowY;
		int marginLeft;
		int marginRight;
		int availableWidth;

		order = def->special - OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET;
		headerIndex = index - (order + 1);
		if ( headerIndex < 0 ) {
			headerIndex = index;
		}

		buttonsPerRow = 9;
		totalButtons = OSP2BE_DLIGHT_WEAPON_COUNT;
		row = order / buttonsPerRow;
		col = order % buttonsPerRow;
		remaining = totalButtons - row * buttonsPerRow;
		colCount = ( remaining < buttonsPerRow ) ? remaining : buttonsPerRow;
		if ( colCount <= 0 ) {
			colCount = buttonsPerRow;
		}

		buttonHeight = osp2be_button.height;
		spacing = osp2be_button.weapon_spacing;
		marginLeft = osp2be_position.square_x_offset_weapon;
		marginRight = osp2be_position.weapon_icon_right_margin;
		availableWidth = s_osp2be.formWidth - marginLeft - marginRight;
		buttonWidth = ( availableWidth - ( colCount - 1 ) * spacing ) / colCount;
		{
			int minButtonWidth;
			int squareSize;
			
			squareSize = osp2be_button.height - osp2be_square.size_offset;
			if ( squareSize < osp2be_square.size_min ) {
				squareSize = osp2be_button.height - osp2be_square.size_fallback_offset;
			}
			minButtonWidth = osp2be_position.square_x_offset_weapon + squareSize + osp2be_position.content_gap_weapon + squareSize + osp2be_position.weapon_icon_right_margin;
			if ( buttonWidth < minButtonWidth ) {
				buttonWidth = minButtonWidth;
			}
		}

		rowWidth = colCount * buttonWidth + ( colCount - 1 ) * spacing;
		rowStartX = s_osp2be.contentLeft + marginLeft;
		if ( rowStartX < s_osp2be.contentLeft ) {
			rowStartX = s_osp2be.contentLeft;
		}

		buttonX = rowStartX + col * ( buttonWidth + spacing );
		baseRowY = s_osp2be.defaultRowY[headerIndex] + s_osp2be.rowHeight + osp2be_row.weapon_offset;
		buttonY = baseRowY + row * ( buttonHeight + spacing );

		Osp2Be_SetButtonGeometry( &s_osp2be.buttons[index], buttonX, buttonY, buttonWidth, buttonHeight );
		s_osp2be.buttons[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

		s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.fields[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
		s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

		s_osp2be.buttons[index].generic.left = s_osp2be.buttons[index].generic.x;
		s_osp2be.buttons[index].generic.right = s_osp2be.buttons[index].generic.x + s_osp2be.buttons[index].width;
		s_osp2be.buttons[index].generic.top = s_osp2be.buttons[index].generic.y;
		s_osp2be.buttons[index].generic.bottom = s_osp2be.buttons[index].generic.y + s_osp2be.buttons[index].height;
		return;
	}

	if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ||
	     def->special == OSP2BE_SPECIAL_MODEL_TORSO ||
	     def->special == OSP2BE_SPECIAL_MODEL_LEGS ) {
		textChars = 0;
	} else if ( def->special == OSP2BE_SPECIAL_RAIL_BEAM ) {
		textChars = 4; /* "Beam" */
	} else if ( def->special == OSP2BE_SPECIAL_RAIL_RING ) {
		textChars = 5; /* "Rings" */
	} else if ( def->label && def->label[0] ) {
		textChars = Q_PrintStrlen( def->label );
		if ( textChars < 0 ) {
			textChars = 0;
		}
	} else {
		textChars = 0;
	}

	textWidth = textChars * SMALLCHAR_WIDTH;

	{
		int squareSize = osp2be_button.height - osp2be_square.size_offset;
		if ( squareSize < osp2be_layout.color_swatch_size ) {
			squareSize = osp2be_layout.color_swatch_size;
		}
		swatchWidth = squareSize + osp2be_square.swatch_padding;
		iconWidth = 0;

		if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ||
		     def->special == OSP2BE_SPECIAL_MODEL_TORSO ||
		     def->special == OSP2BE_SPECIAL_MODEL_LEGS ||
		     def->special == OSP2BE_SPECIAL_RAIL_BEAM ||
		     def->special == OSP2BE_SPECIAL_RAIL_RING ) {
			if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ||
			     def->special == OSP2BE_SPECIAL_MODEL_TORSO ||
			     def->special == OSP2BE_SPECIAL_MODEL_LEGS ) {
				swatchWidth = squareSize + osp2be_square.swatch_padding_head;
				iconWidth = s_osp2be.rowHeight - osp2be_icon.size_offset;
				if ( iconWidth < osp2be_icon.size_min_head ) {
					iconWidth = osp2be_icon.size_min_head;
				}
			} else {
				iconWidth = squareSize;
			}
		}
	}

	{
		int iconGap = (iconWidth > 0 && textWidth > 0) ? osp2be_icon.gap : 0;
		buttonWidth = swatchWidth + iconWidth + iconGap + textWidth + OSP2BE_COLOR_BUTTON_PADDING;

		if ( def->special == OSP2BE_SPECIAL_RAIL_BEAM ||
		     def->special == OSP2BE_SPECIAL_RAIL_RING ) {
			buttonWidth += osp2be_square.swatch_rail_extra_width;
		}
	}
	if ( buttonWidth > s_osp2be.formWidth ) {
		buttonWidth = s_osp2be.formWidth;
	}
	if ( buttonWidth < OSP2BE_COLOR_BUTTON_MIN_WIDTH ) {
		buttonWidth = OSP2BE_COLOR_BUTTON_MIN_WIDTH;
		if ( buttonWidth > s_osp2be.formWidth ) {
			buttonWidth = s_osp2be.formWidth;
		}
	}

	/* Ensure field regions do not overlap button hit test */
	s_osp2be.fields[index].generic.left = -9999;
	s_osp2be.fields[index].generic.right = -9999;
	s_osp2be.fields[index].generic.top = -9999;
	s_osp2be.fields[index].generic.bottom = -9999;

	x = s_osp2be.contentLeft;
	y = s_osp2be.fields[index].generic.y;
	Osp2Be_SetButtonGeometry( &s_osp2be.buttons[index], x, y, buttonWidth, osp2be_button.height );
	s_osp2be.buttons[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.fields[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags &= ~QMF_HASMOUSEFOCUS;

	s_osp2be.buttons[index].generic.left = s_osp2be.buttons[index].generic.x;
	s_osp2be.buttons[index].generic.right = s_osp2be.buttons[index].generic.x + s_osp2be.buttons[index].width;
	s_osp2be.buttons[index].generic.top = s_osp2be.buttons[index].generic.y;
	s_osp2be.buttons[index].generic.bottom = s_osp2be.buttons[index].generic.y + s_osp2be.buttons[index].height;
}

void Osp2Be_SetupSizeButtonControl( int index, const osp2beCvarDef_t *def ) {
	int buttonWidth;
	int minWidth;
	int x;
	int y;
	int gap;
	menubitmap_s *prevButton;
	int prevIndex;
	int maxColorWidth;
	int colorMinWidth;
	int remainingWidth;

	if ( !def ) {
		return;
	}

	buttonWidth = OSP2BE_SIZE_BUTTON_BASE_WIDTH;
	minWidth = OSP2BE_SIZE_BUTTON_MIN_WIDTH;
	gap = 8;
	x = s_osp2be.contentLeft;
	y = s_osp2be.fields[index].generic.y;
	colorMinWidth = OSP2BE_COLOR_BUTTON_MIN_WIDTH;

	prevIndex = index - 1;
	while ( prevIndex >= 0 ) {
		if ( s_osp2be.activeTypes[prevIndex] == OSP2BE_CONTROL_COLOR_BUTTON ) {
			break;
		}
		prevIndex--;
	}

	if ( prevIndex >= 0 && s_osp2be.activeTypes[prevIndex] == OSP2BE_CONTROL_COLOR_BUTTON && s_osp2be.formWidth >= ( colorMinWidth + gap + minWidth ) ) {
		prevButton = &s_osp2be.buttons[prevIndex];
		maxColorWidth = s_osp2be.formWidth - ( buttonWidth + gap );
		if ( maxColorWidth < colorMinWidth ) {
			maxColorWidth = colorMinWidth;
		}
		if ( maxColorWidth > s_osp2be.formWidth ) {
			maxColorWidth = s_osp2be.formWidth;
		}
		if ( prevButton->width > maxColorWidth ) {
			prevButton->width = maxColorWidth;
		}
		if ( prevButton->width < colorMinWidth ) {
			prevButton->width = colorMinWidth;
			if ( prevButton->width > s_osp2be.formWidth ) {
				prevButton->width = s_osp2be.formWidth;
			}
		}
		Osp2Be_SetButtonGeometry( prevButton, prevButton->generic.x, prevButton->generic.y, prevButton->width, prevButton->height );

		x = prevButton->generic.x + prevButton->width + gap;
		y = prevButton->generic.y;
		remainingWidth = s_osp2be.contentLeft + s_osp2be.formWidth - x;
		if ( remainingWidth > 0 && remainingWidth < buttonWidth ) {
			buttonWidth = remainingWidth;
		}
	}

	if ( buttonWidth > s_osp2be.formWidth ) {
		buttonWidth = s_osp2be.formWidth;
	}
	if ( buttonWidth < minWidth ) {
		buttonWidth = minWidth;
	}

	if ( x + buttonWidth > s_osp2be.contentLeft + s_osp2be.formWidth ) {
		buttonWidth = s_osp2be.contentLeft + s_osp2be.formWidth - x;
		if ( buttonWidth < minWidth ) {
			buttonWidth = minWidth;
			x = s_osp2be.contentLeft + s_osp2be.formWidth - buttonWidth;
			if ( x < s_osp2be.contentLeft ) {
				x = s_osp2be.contentLeft;
			}
		}
	}

	Osp2Be_SetButtonGeometry( &s_osp2be.buttons[index], x, y, buttonWidth, osp2be_button.height );
	s_osp2be.buttons[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.buttons[index].generic.left = s_osp2be.buttons[index].generic.x;
	s_osp2be.buttons[index].generic.right = s_osp2be.buttons[index].generic.x + s_osp2be.buttons[index].width;
	s_osp2be.buttons[index].generic.top = s_osp2be.buttons[index].generic.y;
	s_osp2be.buttons[index].generic.bottom = s_osp2be.buttons[index].generic.y + s_osp2be.buttons[index].height;
	s_osp2be.fields[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
}

void Osp2Be_SetupSliderControl( int index, const osp2beCvarDef_t *def ) {
	float value;
	float minValue;
	float maxValue;
	int sliderRight;
	int sliderLeft;
	int sliderWidthPixels;
	int formRight;
	int rowTop;
	int sliderMinWidth;
	int valueChars;
	int valueWidthPixels;
	int valueX;
	int gap;

	if ( !def || index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	Osp2Be_GetSliderBounds( def, &minValue, &maxValue );

	if ( s_osp2be.bufferedInitialized ) {
		value = s_osp2be.bufferedSliderValues[s_osp2be.activeCategory][index];
	} else {
		value = trap_Cvar_VariableValue( def->cvarName );
	}
	value = Osp2Be_SliderClampValue( def, value );

	formRight = s_osp2be.contentLeft + s_osp2be.formWidth;
	valueChars = 6;
	valueWidthPixels = valueChars * SMALLCHAR_WIDTH;
	gap = 16;
	sliderMinWidth = 200;

	s_osp2be.labels[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.labels[index].generic.x = s_osp2be.contentLeft;
	s_osp2be.labels[index].generic.right = s_osp2be.contentLeft + 150;
	s_osp2be.fields[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_osp2be.toggles[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.toggles[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.lists[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.lists[index].generic.flags &= ~QMF_HASMOUSEFOCUS;
	s_osp2be.buttons[index].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	s_osp2be.buttons[index].generic.flags &= ~QMF_HASMOUSEFOCUS;

	rowTop = s_osp2be.labels[index].generic.y;
	
	sliderLeft = s_osp2be.contentLeft + 170;
	sliderWidthPixels = sliderMinWidth;
	
	valueX = sliderLeft + sliderWidthPixels + gap;
	if ( valueX + valueWidthPixels > formRight - 12 ) {
		valueX = formRight - 12 - valueWidthPixels;
		sliderWidthPixels = valueX - gap - sliderLeft;
		if ( sliderWidthPixels < sliderMinWidth ) {
			sliderWidthPixels = sliderMinWidth;
			valueX = sliderLeft + sliderWidthPixels + gap;
			if ( valueX + valueWidthPixels > formRight - 12 ) {
				sliderWidthPixels = formRight - 12 - valueWidthPixels - gap - sliderLeft;
				if ( sliderWidthPixels < sliderMinWidth ) {
					sliderWidthPixels = sliderMinWidth;
				}
				valueX = sliderLeft + sliderWidthPixels + gap;
			}
		}
	}
	
	sliderRight = sliderLeft + sliderWidthPixels;

	s_osp2be.fields[index].generic.x = valueX;
	s_osp2be.fields[index].generic.y = rowTop;
	s_osp2be.fields[index].generic.left = valueX - 4;
	s_osp2be.fields[index].generic.right = valueX + valueWidthPixels + 4;
	s_osp2be.fields[index].generic.top = rowTop - 2;
	s_osp2be.fields[index].generic.bottom = rowTop + SMALLCHAR_HEIGHT + 2;
	s_osp2be.fields[index].field.widthInChars = valueChars;

	s_osp2be.sliders[index].generic.x = sliderLeft;
	s_osp2be.sliders[index].generic.y = rowTop;
	s_osp2be.sliders[index].generic.left = sliderLeft;
	s_osp2be.sliders[index].generic.right = sliderRight;
	s_osp2be.sliders[index].generic.top = rowTop - 6;
	s_osp2be.sliders[index].generic.bottom = rowTop + SMALLCHAR_HEIGHT + 6;
	s_osp2be.sliders[index].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);

	Osp2Be_SliderSetValue( index, def, value );
}

void Osp2Be_ApplyFieldControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	if ( !def ) {
		return;
	}

	Osp2Be_WriteFieldValue( def, s_osp2be.fields[index].field.buffer, modelAssemblies, modelAssemblyCount, railAssemblies, railAssemblyCount );
}

void Osp2Be_ApplyToggleControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	(void)modelAssemblies;
	(void)modelAssemblyCount;
	(void)railAssemblies;
	(void)railAssemblyCount;

	if ( !def || !def->cvarName ) {
		return;
	}

	trap_Cvar_SetValue( def->cvarName, s_osp2be.toggles[index].curvalue );
}

void Osp2Be_ApplyListControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	(void)modelAssemblies;
	(void)modelAssemblyCount;
	(void)railAssemblies;
	(void)railAssemblyCount;

	if ( !def || !def->cvarName ) {
		return;
	}

	trap_Cvar_SetValue( def->cvarName, s_osp2be.lists[index].curvalue );
}

void Osp2Be_ApplyButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	/* Button controls don't need to apply anything - they handle their logic in the event callback */
	(void)index;
	(void)def;
	(void)modelAssemblies;
	(void)modelAssemblyCount;
	(void)railAssemblies;
	(void)railAssemblyCount;
}

void Osp2Be_ApplyColorButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	if ( !def ) {
		return;
	}

	Osp2Be_WriteFieldValue( def, s_osp2be.fields[index].field.buffer, modelAssemblies, modelAssemblyCount, railAssemblies, railAssemblyCount );
}

void Osp2Be_ApplySizeButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	float currentValue;
	int sizeValue;

	(void)index;
	(void)modelAssemblies;
	(void)modelAssemblyCount;
	(void)railAssemblies;
	(void)railAssemblyCount;

	if ( !def || !def->cvarName ) {
		return;
	}

	sizeValue = 0;
	if ( s_osp2be.bufferedInitialized ) {
		sizeValue = s_osp2be.bufferedSizeValues[s_osp2be.activeCategory][index];
	} else {
		currentValue = trap_Cvar_VariableValue( def->cvarName );
		sizeValue = (int)currentValue;
	}
	if ( sizeValue < 0 ) {
		sizeValue = 0;
	}
	if ( sizeValue > 3 ) {
		sizeValue = sizeValue % 4;
	}
	trap_Cvar_SetValue( def->cvarName, sizeValue );
}

void Osp2Be_ApplySliderControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount ) {
	float value;

	(void)modelAssemblies;
	(void)modelAssemblyCount;
	(void)railAssemblies;
	(void)railAssemblyCount;

	if ( !def || !def->cvarName || index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	if ( s_osp2be.bufferedInitialized ) {
		value = s_osp2be.bufferedSliderValues[s_osp2be.activeCategory][index];
	} else {
		value = Osp2Be_ParseSliderInput( s_osp2be.fields[index].field.buffer, s_osp2be.sliders[index].curvalue );
		value = Osp2Be_SliderClampValue( def, value );
	}

	Osp2Be_SliderSetValue( index, def, value );
	trap_Cvar_SetValue( def->cvarName, value );
}

qboolean Osp2Be_CanUseColorPicker( const osp2beCvarDef_t *def ) {
	osp2beSpecial_t special;

	if ( !def ) {
		return qfalse;
	}

	if ( def->controlType != OSP2BE_CONTROL_FIELD && def->controlType != OSP2BE_CONTROL_COLOR_BUTTON ) {
		return qfalse;
	}

	special = def->special;
	if ( special == OSP2BE_SPECIAL_MODEL_HEAD ||
		special == OSP2BE_SPECIAL_MODEL_TORSO ||
		special == OSP2BE_SPECIAL_MODEL_LEGS ||
		special == OSP2BE_SPECIAL_RAIL_BEAM ||
		special == OSP2BE_SPECIAL_RAIL_RING ||
		special == OSP2BE_SPECIAL_COLOR_GENERIC ||
		( special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
		  special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) ) {
		return qtrue;
	}

	return qfalse;
}

