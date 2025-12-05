#include "ui_osp2be_shared.h"

/* Category definitions - item counts initialized at runtime */
static osp2beCategoryDef_t osp2beCategories[] = {
	{ "General", osp2beGeneralControls, 0 },
	{ "Player", osp2bePlayerControls, 0 },
	{ "Weapons", osp2beWeaponControls, 0 },
	{ "Railgun", osp2beRailgunControls, 0 },
	{ "Explosives", osp2beExplosivesControls, 0 },
	{ "Teams", osp2beTeamsControls, 0 },
	{ "SuperHUD", osp2beSuperHUDControls, 0 }
};

enum { OSP2BE_DEFINED_CATEGORY_COUNT = ARRAY_LEN( osp2beCategories ) };
typedef char osp2beCategoryCountCheck[(OSP2BE_DEFINED_CATEGORY_COUNT == OSP2BE_CATEGORY_COUNT) ? 1 : -1];

static const char *osp2beCategoryNames[] = { "General", "Player", "Weapons", "Railgun", "Explosives", "Teams", "SuperHUD", NULL };

/* Global menu state - exported for module access */
osp2beMenu_t s_osp2be;
vec4_t osp2be_sidebar_color = { 0.0f, 0.0f, 0.0f, 0.90f };
vec4_t osp2be_content_color = { 0.0f, 0.0f, 0.0f, 0.90f };
qhandle_t osp2beOutlineThin;
qhandle_t osp2beOutlineMedium;
qhandle_t osp2beOutlineWide;
qhandle_t osp2beFrozenOverlay;
qhandle_t osp2beFrozenIcon;

/* Font cvar */
static vmCvar_t ui_be_font;

static void Osp2Be_OpenColorPicker( int controlIndex );
static void Osp2Be_GetCurrentColorValue( int controlIndex, const osp2beCvarDef_t *def, char *buffer, int bufferSize );
static void Osp2Be_FieldEvent( void *ptr, int event );
static void Osp2Be_ColorPickerApply( const char *hexValue, void *userData );
static void Osp2Be_SetStatus( const char *text );
static void Osp2Be_ListEvent( void *ptr, int event );
static void Osp2Be_ButtonEvent( void *ptr, int event );
static void Osp2Be_PreviewFrozenEvent( void *ptr, int event );
static void Osp2Be_SliderEvent( void *ptr, int event );
static void Osp2Be_ResetControlPositions( void );
static void Osp2Be_UpdateLayoutPositions( void );
static void Osp2Be_ApplyHorizontalColorLayout( void );
static void Osp2Be_ApplyTeamsLayout( void );
static void Osp2Be_SetHorizontalRow( const int *indices, int count, int baseY );
static void Osp2Be_PositionButton( int index, int x, int y, int width, int height );
static void Osp2Be_UpdatePreviewVisibility( void );
qboolean Osp2Be_HandleSliderMouse( int cursorX, int cursorY );
static void Osp2Be_ModelSelectionApplied( const char *modelValue, const char *headValue, void *userData );
static void Osp2Be_ResetBufferedSettings( void );
static void Osp2Be_LoadBufferedSettings( qboolean force );
static void Osp2Be_SaveActiveControlsToBuffer( void );
static void Osp2Be_BufferedFieldChange( int index );
static void Osp2Be_BufferedToggleChange( int index );
static void Osp2Be_BufferedListChange( int index );
static void Osp2Be_BufferedSliderChange( int index );
static void Osp2Be_BufferedSizeChange( int index, int value );
static void Osp2Be_ApplyBufferedCategory( int categoryIndex );

typedef void ( *osp2beControlSetupFn )( int index, const osp2beCvarDef_t *def );
typedef void ( *osp2beControlApplyFn )( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );

typedef struct {
	osp2beControlSetupFn setup;
	osp2beControlApplyFn apply;
} osp2beControlHandler_t;

static const osp2beControlHandler_t osp2beControlHandlers[] = {
	{ Osp2Be_SetupFieldControl, Osp2Be_ApplyFieldControl },
	{ Osp2Be_SetupToggleControl, Osp2Be_ApplyToggleControl },
	{ Osp2Be_SetupListControl, Osp2Be_ApplyListControl },
	{ Osp2Be_SetupButtonControl, Osp2Be_ApplyButtonControl },
	{ Osp2Be_SetupColorButtonControl, Osp2Be_ApplyColorButtonControl },
	{ Osp2Be_SetupSizeButtonControl, Osp2Be_ApplySizeButtonControl },
	{ Osp2Be_SetupSliderControl, Osp2Be_ApplySliderControl }
};

#define OSP2BE_HANDLER_COUNT 7

static void Osp2Be_SetMainControlsEnabled( qboolean enable ) {
	int i;

	if ( enable ) {
		s_osp2be.category.generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		s_osp2be.apply.generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		s_osp2be.reload.generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		s_osp2be.close.generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		s_osp2be.previewFrozenButton.generic.flags &= ~(QMF_GRAYED|QMF_INACTIVE);
		for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
			s_osp2be.labels[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.labels[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.labels[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.labels[i].generic.flags &= ~QMF_INACTIVE;
			}
			s_osp2be.fields[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.fields[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.fields[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.fields[i].generic.flags &= ~QMF_INACTIVE;
			}
			s_osp2be.toggles[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.toggles[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.toggles[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.toggles[i].generic.flags &= ~QMF_INACTIVE;
			}
			s_osp2be.lists[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.lists[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.lists[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.lists[i].generic.flags &= ~QMF_INACTIVE;
			}
			s_osp2be.buttons[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.buttons[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.buttons[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.buttons[i].generic.flags &= ~QMF_INACTIVE;
			}
			s_osp2be.sliders[i].generic.flags &= ~QMF_GRAYED;
			if ( s_osp2be.sliders[i].generic.flags & QMF_HIDDEN ) {
				s_osp2be.sliders[i].generic.flags |= QMF_INACTIVE;
			} else {
				s_osp2be.sliders[i].generic.flags &= ~QMF_INACTIVE;
			}
		}
	} else {
		s_osp2be.category.generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		s_osp2be.apply.generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		s_osp2be.reload.generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		s_osp2be.close.generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		s_osp2be.previewFrozenButton.generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
			s_osp2be.labels[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
			s_osp2be.fields[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
			s_osp2be.toggles[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
			s_osp2be.lists[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
			s_osp2be.buttons[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
			s_osp2be.sliders[i].generic.flags |= QMF_GRAYED|QMF_INACTIVE;
		}
	}
}


static void Osp2Be_ModelSelectionApplied( const char *modelValue, const char *headValue, void *userData ) {
	char *buffer;
	int buttonIndex;
	const char *appliedModel;
	const osp2beCvarDef_t *def;
	qboolean switchToAlly;

	(void)userData;

	appliedModel = ( modelValue && modelValue[0] ) ? modelValue : "";
	switchToAlly = qfalse;

	buttonIndex = s_osp2be.pendingModelButtonIndex;

	if ( s_osp2be.pendingModelCategory == 3 && buttonIndex >= 0 && buttonIndex < OSP2BE_MAX_CONTROLS ) {
		def = s_osp2be.activeDefs[buttonIndex];
		if ( def && def->cvarName ) {
			if ( !Q_stricmp( def->cvarName, "cg_teamModel" ) ) {
				switchToAlly = qtrue;
			} else if ( !Q_stricmp( def->cvarName, "cg_enemyModel" ) ) {
				switchToAlly = qfalse;
			}
		}
	}

	if ( s_osp2be.pendingModelCategory == 3 ) {
		buffer = NULL;
		if ( buttonIndex >= 0 && buttonIndex < OSP2BE_MAX_CONTROLS ) {
			def = s_osp2be.activeDefs[buttonIndex];
			if ( def && def->cvarName ) {
				if ( !Q_stricmp( def->cvarName, "cg_teamModel" ) ) {
					buffer = s_osp2be.bufferedAllyModel;
				} else if ( !Q_stricmp( def->cvarName, "cg_enemyModel" ) ) {
					buffer = s_osp2be.bufferedEnemyModel;
				}
				if ( buffer ) {
					if ( appliedModel[0] ) {
						Q_strncpyz( buffer, appliedModel, MAX_QPATH );
					} else {
						buffer[0] = '\0';
					}
				}
			}
		}
	} else {
		buffer = Osp2Be_GetModelBufferForCategory( s_osp2be.pendingModelCategory );
		if ( buffer ) {
			if ( appliedModel[0] ) {
				Q_strncpyz( buffer, appliedModel, MAX_QPATH );
			} else {
				buffer[0] = '\0';
			}
		}
	}

	if ( s_osp2be.pendingModelCategory == 1 ) {
		if ( headValue && headValue[0] ) {
			Q_strncpyz( s_osp2be.bufferedPlayerHead, headValue, sizeof( s_osp2be.bufferedPlayerHead ) );
		} else if ( appliedModel[0] ) {
			Q_strncpyz( s_osp2be.bufferedPlayerHead, appliedModel, sizeof( s_osp2be.bufferedPlayerHead ) );
		} else {
			s_osp2be.bufferedPlayerHead[0] = '\0';
		}
	}

	if ( buttonIndex >= 0 && buttonIndex < OSP2BE_MAX_CONTROLS && buffer ) {
		Q_strncpyz( s_osp2be.fields[buttonIndex].field.buffer, buffer, sizeof( s_osp2be.fields[buttonIndex].field.buffer ) );
		Osp2Be_ClampFieldLength( &s_osp2be.fields[buttonIndex] );
		Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.buttons[buttonIndex] );
	}

	s_osp2be.pendingModelButtonIndex = -1;
	s_osp2be.pendingModelCategory = -1;

	s_osp2be.bufferedInitialized = qtrue;
	Osp2Be_SetMainControlsEnabled( qtrue );
	Osp2Be_SetStatus( "Buffered model updated" );

	if ( s_osp2be.activeCategory == 3 ) {
		if ( switchToAlly && s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qfalse;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		} else if ( !switchToAlly && !s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qtrue;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		}
	}

	Osp2Be_UpdatePreviewModel();
	if ( s_osp2be.activeCategory == 3 ) {
		Osp2Be_ApplyTeamsLayout();
	}
}

static void Osp2Be_ResetBufferedSettings( void ) {
	int categoryIndex;
	int controlIndex;

	for ( categoryIndex = 0; categoryIndex < (int)OSP2BE_CATEGORY_COUNT; categoryIndex++ ) {
		for ( controlIndex = 0; controlIndex < OSP2BE_MAX_CONTROLS; controlIndex++ ) {
			s_osp2be.bufferedFieldValues[categoryIndex][controlIndex][0] = '\0';
			s_osp2be.bufferedToggleValues[categoryIndex][controlIndex] = 0;
			s_osp2be.bufferedListValues[categoryIndex][controlIndex] = 0;
			s_osp2be.bufferedSliderValues[categoryIndex][controlIndex] = 0.0f;
			s_osp2be.bufferedSizeValues[categoryIndex][controlIndex] = 0;
			s_osp2be.appliedSliderValues[categoryIndex][controlIndex] = 0.0f;
		}
	}

	s_osp2be.bufferedInitialized = qfalse;
}

static void Osp2Be_LoadBufferedSettings( qboolean force ) {
	int categoryIndex;

	if ( s_osp2be.bufferedInitialized && !force ) {
		return;
	}

	for ( categoryIndex = 0; categoryIndex < (int)OSP2BE_CATEGORY_COUNT; categoryIndex++ ) {
		const osp2beCategoryDef_t *category;
		int count;
		int i;

		category = &osp2beCategories[categoryIndex];
		count = category->itemCount;
		if ( count > OSP2BE_MAX_CONTROLS ) {
			count = OSP2BE_MAX_CONTROLS;
		}

		for ( i = 0; i < count; i++ ) {
			const osp2beCvarDef_t *def;

			def = &category->items[i];
			switch ( def->controlType ) {
			case OSP2BE_CONTROL_FIELD:
			case OSP2BE_CONTROL_COLOR_BUTTON:
				Osp2Be_ReadFieldValue( def, s_osp2be.bufferedFieldValues[categoryIndex][i], sizeof( s_osp2be.bufferedFieldValues[0][0] ) );
				break;
			case OSP2BE_CONTROL_TOGGLE:
				if ( def->cvarName && def->cvarName[0] ) {
					s_osp2be.bufferedToggleValues[categoryIndex][i] = ( trap_Cvar_VariableValue( def->cvarName ) != 0.0f ) ? 1 : 0;
				} else {
					s_osp2be.bufferedToggleValues[categoryIndex][i] = 0;
				}
				break;
			case OSP2BE_CONTROL_LIST:
				if ( def->cvarName && def->cvarName[0] ) {
					s_osp2be.bufferedListValues[categoryIndex][i] = (int)trap_Cvar_VariableValue( def->cvarName );
				} else {
					s_osp2be.bufferedListValues[categoryIndex][i] = 0;
				}
				break;
			case OSP2BE_CONTROL_SLIDER:
				if ( def->cvarName && def->cvarName[0] ) {
					float value = (float)trap_Cvar_VariableValue( def->cvarName );
					value = Osp2Be_SliderClampValue( def, value );
					s_osp2be.bufferedSliderValues[categoryIndex][i] = value;
					s_osp2be.appliedSliderValues[categoryIndex][i] = value;
				} else {
					s_osp2be.bufferedSliderValues[categoryIndex][i] = 0.0f;
					s_osp2be.appliedSliderValues[categoryIndex][i] = 0.0f;
				}
				break;
			case OSP2BE_CONTROL_SIZE_BUTTON:
				if ( def->cvarName && def->cvarName[0] ) {
					int sizeValue = (int)trap_Cvar_VariableValue( def->cvarName );
					if ( sizeValue < 0 ) {
						sizeValue = 0;
					}
					if ( sizeValue > 3 ) {
						sizeValue = sizeValue % 4;
					}
					s_osp2be.bufferedSizeValues[categoryIndex][i] = sizeValue;
				} else {
					s_osp2be.bufferedSizeValues[categoryIndex][i] = 0;
				}
				break;
			default:
				break;
			}
		}
	}

	s_osp2be.bufferedInitialized = qtrue;
}

static void Osp2Be_SaveActiveControlsToBuffer( void ) {
	int i;
	int categoryIndex = s_osp2be.activeCategory;

	if ( !s_osp2be.bufferedInitialized ) {
		/* continue saving so that buffers are populated even if initialization was delayed */
	}

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}

	if ( s_osp2be.activeControlCount <= 0 ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}

	for ( i = 0; i < s_osp2be.activeControlCount && i < OSP2BE_MAX_CONTROLS; i++ ) {
		const osp2beCvarDef_t *def = s_osp2be.activeDefs[i];
		if ( !def ) {
			continue;
		}

		switch ( def->controlType ) {
		case OSP2BE_CONTROL_FIELD:
		case OSP2BE_CONTROL_COLOR_BUTTON:
			Q_strncpyz( s_osp2be.bufferedFieldValues[categoryIndex][i], s_osp2be.fields[i].field.buffer, sizeof( s_osp2be.bufferedFieldValues[0][0] ) );
			break;
		case OSP2BE_CONTROL_TOGGLE:
			s_osp2be.bufferedToggleValues[categoryIndex][i] = s_osp2be.toggles[i].curvalue ? 1 : 0;
			break;
		case OSP2BE_CONTROL_LIST:
			s_osp2be.bufferedListValues[categoryIndex][i] = s_osp2be.lists[i].curvalue;
			break;
		case OSP2BE_CONTROL_SLIDER:
		{
			float parsedValue;

			parsedValue = Osp2Be_ParseSliderInput( s_osp2be.fields[i].field.buffer, s_osp2be.sliders[i].curvalue );
			parsedValue = Osp2Be_SliderClampValue( s_osp2be.activeDefs[i], parsedValue );
			s_osp2be.bufferedSliderValues[categoryIndex][i] = parsedValue;
			Osp2Be_SliderSetValue( i, s_osp2be.activeDefs[i], parsedValue );
			break;
		}
			break;
		case OSP2BE_CONTROL_SIZE_BUTTON:
			break;
		default:
			break;
		}
	}
}

static void Osp2Be_BufferedFieldChange( int index ) {
	int categoryIndex = s_osp2be.activeCategory;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;
	Q_strncpyz( s_osp2be.bufferedFieldValues[categoryIndex][index], s_osp2be.fields[index].field.buffer, sizeof( s_osp2be.bufferedFieldValues[0][0] ) );
}

static void Osp2Be_BufferedToggleChange( int index ) {
	int categoryIndex = s_osp2be.activeCategory;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;
	s_osp2be.bufferedToggleValues[categoryIndex][index] = s_osp2be.toggles[index].curvalue ? 1 : 0;
}

static void Osp2Be_BufferedListChange( int index ) {
	int categoryIndex = s_osp2be.activeCategory;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;
	s_osp2be.bufferedListValues[categoryIndex][index] = s_osp2be.lists[index].curvalue;
}

static void Osp2Be_BufferedSliderChange( int index ) {
	int categoryIndex = s_osp2be.activeCategory;
	const osp2beCvarDef_t *def;
	float appliedValue;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;
	s_osp2be.bufferedSliderValues[categoryIndex][index] = s_osp2be.sliders[index].curvalue;

	def = s_osp2be.activeDefs[index];
	if ( !def || !def->cvarName || !def->cvarName[0] ) {
		return;
	}

	appliedValue = s_osp2be.appliedSliderValues[categoryIndex][index];
	trap_Cvar_SetValue( def->cvarName, appliedValue );
}

static void Osp2Be_BufferedSizeChange( int index, int value ) {
	int categoryIndex = s_osp2be.activeCategory;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.bufferedInitialized = qtrue;
	if ( value < 0 ) {
		value = 0;
	}
	if ( value > 3 ) {
		value = value % 4;
	}
	s_osp2be.bufferedSizeValues[categoryIndex][index] = value;
}

qboolean Osp2Be_GetBufferedNumericValue( const char *cvarName, float *outValue ) {
	int categoryIndex;

	if ( !cvarName || !cvarName[0] || !outValue ) {
		return qfalse;
	}

	if ( !s_osp2be.bufferedInitialized ) {
		return qfalse;
	}

	for ( categoryIndex = 0; categoryIndex < (int)OSP2BE_CATEGORY_COUNT; categoryIndex++ ) {
		const osp2beCategoryDef_t *category;
		int count;
		int i;

		category = &osp2beCategories[categoryIndex];
		count = category->itemCount;
		if ( count > OSP2BE_MAX_CONTROLS ) {
			count = OSP2BE_MAX_CONTROLS;
		}

		for ( i = 0; i < count; i++ ) {
			const osp2beCvarDef_t *def;

			def = &category->items[i];
			if ( !def->cvarName || Q_stricmp( def->cvarName, cvarName ) != 0 ) {
				continue;
			}

			switch ( def->controlType ) {
			case OSP2BE_CONTROL_TOGGLE:
				*outValue = (float)s_osp2be.bufferedToggleValues[categoryIndex][i];
				return qtrue;
			case OSP2BE_CONTROL_LIST:
				*outValue = (float)s_osp2be.bufferedListValues[categoryIndex][i];
				return qtrue;
			case OSP2BE_CONTROL_SLIDER:
				*outValue = s_osp2be.bufferedSliderValues[categoryIndex][i];
				return qtrue;
			case OSP2BE_CONTROL_SIZE_BUTTON:
				*outValue = (float)s_osp2be.bufferedSizeValues[categoryIndex][i];
				return qtrue;
			default:
				break;
			}
		}
	}

	return qfalse;
}

static void Osp2Be_PositionButton( int index, int x, int y, int width, int height )
{
	menubitmap_s *button;
	int buttonHeight;

	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	button = &s_osp2be.buttons[index];

	if ( width < 1 ) {
		width = 1;
	}

	buttonHeight = button->height;
	if ( buttonHeight <= 0 ) {
		buttonHeight = osp2be_button.height;
	}

	if ( height > 0 ) {
		buttonHeight = height;
	}

	button->generic.x = x;
	button->generic.y = y;
	button->width = width;
	button->height = buttonHeight;
	button->generic.left = x;
	button->generic.top = y;
	button->generic.right = x + width;
	button->generic.bottom = y + buttonHeight;
}

/* Helper function to position all controls (field, toggle, list) at the same position */
static void Osp2Be_PositionControls( int index, int x, int y ) {
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	s_osp2be.fields[index].generic.x = x;
	s_osp2be.fields[index].generic.y = y;
	s_osp2be.toggles[index].generic.x = x;
	s_osp2be.toggles[index].generic.y = y;
	s_osp2be.lists[index].generic.x = x;
	s_osp2be.lists[index].generic.y = y;
}

/* Helper function to set label color to white */
static void Osp2Be_SetLabelColorWhite( int index ) {
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}
	Osp2Be_SetColorVec4( s_osp2be.labels[index].color, 1.0f, 1.0f, 1.0f, 1.0f );
}

/* Helper function to adjust Y position of all controls by delta */
static void Osp2Be_AdjustControlsY( int index, int deltaY ) {
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}
	if ( deltaY == 0 ) {
		return;
	}
	s_osp2be.fields[index].generic.y += deltaY;
	s_osp2be.toggles[index].generic.y += deltaY;
	s_osp2be.lists[index].generic.y += deltaY;
	s_osp2be.labels[index].generic.y += deltaY;
}

static void Osp2Be_SetHorizontalRow( const int *indices, int count, int baseY )
{
	int totalWidth;
	int available;
	int gap;
	int extra;
	int i;
	int x;
	int widths[OSP2BE_MAX_CONTROLS];
	int heights[OSP2BE_MAX_CONTROLS];

	if ( !indices || count <= 0 ) {
		return;
	}

	totalWidth = 0;
	for ( i = 0; i < count; i++ ) {
		int idx = indices[i];
		int width;
		int height;

		if ( idx < 0 || idx >= OSP2BE_MAX_CONTROLS ) {
			widths[i] = 0;
			heights[i] = osp2be_button.height;
			continue;
		}

		width = s_osp2be.buttons[idx].width;
		if ( width <= 0 ) {
			width = osp2be_button.height * osp2be_button.default_width_multiplier;
		}
		height = s_osp2be.buttons[idx].height;
		if ( height <= 0 ) {
			height = osp2be_button.height;
		}
		widths[i] = width;
		heights[i] = height;
		totalWidth += width;
	}

	if ( totalWidth < 0 ) {
		totalWidth = 0;
	}

	available = s_osp2be.formWidth - totalWidth;
	if ( available < 0 ) {
		available = 0;
	}

	if ( count > 1 ) {
		gap = available / (count - 1);
		if ( gap > osp2be_row.gap_max ) {
			int reduction = gap - osp2be_row.gap_max;
			available -= reduction * (count - 1);
			if ( available < 0 ) {
				available = 0;
			}
			gap = available / (count - 1);
		}
		extra = available - gap * (count - 1);
	} else {
		gap = 0;
		extra = 0;
	}

	x = s_osp2be.contentLeft;
	for ( i = 0; i < count; i++ ) {
		int idx = indices[i];
		int width = widths[i];
		int height = heights[i];

		if ( idx >= 0 && idx < OSP2BE_MAX_CONTROLS ) {
			Osp2Be_PositionButton( idx, x, baseY, width, height );
		}

		if ( count > 1 ) {
			int gapStep = gap;
			if ( extra > 0 ) {
				gapStep++;
				extra--;
			}
			x += width + gapStep;
		}
	}
}

static void Osp2Be_ApplyHorizontalColorLayout( void )
{
	const osp2beCvarDef_t *def;
	const osp2beCategoryDef_t *category;
	int modelIndex = -1;
	int headIndices[3] = { -1, -1, -1 };
	int railBeam = -1;
	int railRing = -1;
	int railOutline = -1;
	int railSize = -1;
	int railFrozen = -1;
	int headBaseY = -1;
	int headHeight = osp2be_button.height;
	int railOriginalY = -1;
	int railBaseY = -1;
	int railHeight = osp2be_button.height;
	int count;
	int i;

	if ( s_osp2be.activeCategory != 1 ) {
		return;
	}

	category = &osp2beCategories[s_osp2be.activeCategory];
	count = category->itemCount;
	if ( count > OSP2BE_MAX_CONTROLS ) {
		count = OSP2BE_MAX_CONTROLS;
	}

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		def = s_osp2be.activeDefs[i];
		if ( !def ) {
			continue;
		}

		switch ( def->special ) {
		case OSP2BE_SPECIAL_MODEL_SELECTOR:
			modelIndex = i;
			break;
		case OSP2BE_SPECIAL_MODEL_HEAD:
			headIndices[0] = i;
			break;
		case OSP2BE_SPECIAL_MODEL_TORSO:
			headIndices[1] = i;
			break;
		case OSP2BE_SPECIAL_MODEL_LEGS:
			headIndices[2] = i;
			break;
		case OSP2BE_SPECIAL_RAIL_BEAM:
			railBeam = i;
			break;
		case OSP2BE_SPECIAL_RAIL_RING:
			railRing = i;
			break;
		case OSP2BE_SPECIAL_COLOR_GENERIC:
			if ( def->label && !Q_stricmp( def->label, "Outline" ) ) {
				railOutline = i;
			} else if ( def->label && !Q_stricmp( def->label, "Frozen" ) ) {
				railFrozen = i;
			}
			break;
		case OSP2BE_SPECIAL_OUTLINE_SIZE:
			railSize = i;
			break;
		default:
			break;
		}
	}

	if ( modelIndex >= 0 ) {
		int rowIndices[4];
		int originalY[4];
		int rowCount = 0;
		int baseY = s_osp2be.buttons[modelIndex].generic.y;

		rowIndices[rowCount] = modelIndex;
		originalY[rowCount++] = baseY;

		if ( headIndices[0] >= 0 ) {
			rowIndices[rowCount] = headIndices[0];
			originalY[rowCount++] = s_osp2be.buttons[headIndices[0]].generic.y;
		}
		if ( headIndices[1] >= 0 ) {
			rowIndices[rowCount] = headIndices[1];
			originalY[rowCount++] = s_osp2be.buttons[headIndices[1]].generic.y;
		}
		if ( headIndices[2] >= 0 ) {
			rowIndices[rowCount] = headIndices[2];
			originalY[rowCount++] = s_osp2be.buttons[headIndices[2]].generic.y;
		}

		if ( rowCount > 1 ) {
			int j;
			Osp2Be_SetHorizontalRow( rowIndices, rowCount, baseY );

		for ( j = 1; j < rowCount; j++ ) {
			int idx = rowIndices[j];
			int delta = baseY - originalY[j];
			Osp2Be_AdjustControlsY( idx, delta );
		}
		}
	}

	if ( headIndices[0] >= 0 ) {
		headBaseY = s_osp2be.buttons[headIndices[0]].generic.y;
		headHeight = s_osp2be.buttons[headIndices[0]].height;
		if ( headHeight <= 0 ) {
			headHeight = osp2be_button.height;
		}
	}

	if ( modelIndex < 0 && headIndices[0] >= 0 && headIndices[1] >= 0 && headIndices[2] >= 0 ) {
		int rowIndices[3];
		rowIndices[0] = headIndices[0];
		rowIndices[1] = headIndices[1];
		rowIndices[2] = headIndices[2];
		headBaseY = s_osp2be.buttons[rowIndices[0]].generic.y;
		headHeight = s_osp2be.buttons[rowIndices[0]].height;
		if ( headHeight <= 0 ) {
			headHeight = 28;
		}
		Osp2Be_SetHorizontalRow( rowIndices, 3, headBaseY );
	}

	if ( railBeam >= 0 && railRing >= 0 ) {
		int rowIndices[5];
		int originalRowY[5];
		int rowCount = 0;
		int desiredY;
		int delta;
		int primaryIndex;
		int j;

		rowIndices[rowCount] = railBeam;
		originalRowY[rowCount++] = s_osp2be.buttons[railBeam].generic.y;
		rowIndices[rowCount] = railRing;
		originalRowY[rowCount++] = s_osp2be.buttons[railRing].generic.y;
		if ( railOutline >= 0 ) {
			rowIndices[rowCount] = railOutline;
			originalRowY[rowCount++] = s_osp2be.buttons[railOutline].generic.y;
		}
		if ( railSize >= 0 ) {
			rowIndices[rowCount] = railSize;
			originalRowY[rowCount++] = s_osp2be.buttons[railSize].generic.y;
		}

		primaryIndex = rowIndices[0];
		railOriginalY = originalRowY[0];
		if ( headBaseY >= 0 ) {
			desiredY = headBaseY + headHeight + 6;
		} else {
			desiredY = railOriginalY;
		}

		Osp2Be_SetHorizontalRow( rowIndices, rowCount, desiredY );
		railBaseY = desiredY;
		railHeight = s_osp2be.buttons[primaryIndex].height;
		if ( railHeight <= 0 ) {
			railHeight = osp2be_button.height;
		}

		for ( j = 0; j < rowCount; j++ ) {
			int idx = rowIndices[j];
			int deltaLocal;

			deltaLocal = s_osp2be.buttons[idx].generic.y - originalRowY[j];
			Osp2Be_AdjustControlsY( idx, deltaLocal );
		}

		delta = desiredY - railOriginalY;
		if ( delta != 0 ) {
			for ( i = 0; i < count; i++ ) {
				qboolean skip = qfalse;
				for ( j = 0; j < rowCount; j++ ) {
					if ( rowIndices[j] == i ) {
						skip = qtrue;
						break;
					}
				}
				if ( skip ) {
					continue;
				}

				if ( s_osp2be.activeDefs[i] && s_osp2be.buttons[i].generic.y > railOriginalY ) {
					Osp2Be_PositionButton( i,
					                       s_osp2be.buttons[i].generic.x,
					                       s_osp2be.buttons[i].generic.y + delta,
					                       s_osp2be.buttons[i].width,
					                       s_osp2be.buttons[i].height );

					Osp2Be_AdjustControlsY( i, delta );
				}
			}
		}
	}

	if ( railFrozen >= 0 ) {
		int desiredY;
		int originalY;
		int deltaLocal;

		originalY = s_osp2be.buttons[railFrozen].generic.y;

		if ( railBaseY >= 0 ) {
			desiredY = railBaseY + railHeight + 6;
		} else {
			desiredY = originalY;
		}

		Osp2Be_PositionButton( railFrozen,
		                       s_osp2be.buttons[railFrozen].generic.x,
		                       desiredY,
		                       s_osp2be.buttons[railFrozen].width,
		                       s_osp2be.buttons[railFrozen].height );

		deltaLocal = desiredY - originalY;
		Osp2Be_AdjustControlsY( railFrozen, deltaLocal );
	}
}

static void Osp2Be_ApplyTeamsLayout( void ) {
	const osp2beCategoryDef_t *category;
	int allyModelIndex;
	int allyHeadIndex;
	int allyTorsoIndex;
	int allyLegsIndex;
	int allyRailBeamIndex;
	int allyRailRingIndex;
	int allyOutlineIndex;
	int allySizeIndex;
	int allyFrozenIndex;
	int allyDefaultIndex;
	int enemyModelIndex;
	int enemyDefaultIndex;
	int enemyHeadIndex;
	int enemyTorsoIndex;
	int enemyLegsIndex;
	int enemyRailBeamIndex;
	int enemyRailRingIndex;
	int enemyOutlineIndex;
	int enemySizeIndex;
	int enemyFrozenIndex;
	int baseY;
	int buttonWidth;
	int leftColumnX;
	int rightColumnX;
	int headerY;
	int rowY;
	int rowHeight;

	category = &osp2beCategories[s_osp2be.activeCategory];
	if ( category->itemCount < 20 ) {
		return;
	}

	allyModelIndex = 0;
	allyDefaultIndex = 1;
	allyHeadIndex = 2;
	allyTorsoIndex = 3;
	allyLegsIndex = 4;
	allyRailBeamIndex = 5;
	allyRailRingIndex = 6;
	allyOutlineIndex = 7;
	allySizeIndex = 8;
	allyFrozenIndex = 9;
	enemyModelIndex = 10;
	enemyDefaultIndex = 11;
	enemyHeadIndex = 12;
	enemyTorsoIndex = 13;
	enemyLegsIndex = 14;
	enemyRailBeamIndex = 15;
	enemyRailRingIndex = 16;
	enemyOutlineIndex = 17;
	enemySizeIndex = 18;
	enemyFrozenIndex = 19;

	baseY = s_osp2be.defaultRowY[0];
	headerY = baseY - 20 + 16;
	rowY = baseY + 8 + 16;
	rowHeight = osp2be_row.height;

	buttonWidth = ( s_osp2be.formWidth - osp2be_layout.content_gap ) / 2;
	leftColumnX = s_osp2be.contentLeft;
		rightColumnX = s_osp2be.contentLeft + buttonWidth + osp2be_layout.content_gap;

	if ( allyModelIndex >= 0 && allyModelIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyModelIndex].string = "Ally";
		s_osp2be.labels[allyModelIndex].generic.flags &= ~QMF_HIDDEN;
		s_osp2be.labels[allyModelIndex].generic.flags |= QMF_INACTIVE;
		s_osp2be.labels[allyModelIndex].style = UI_CENTER|UI_SMALLFONT;
		s_osp2be.labels[allyModelIndex].generic.x = leftColumnX + buttonWidth / 2;
		s_osp2be.labels[allyModelIndex].generic.y = headerY;
	}

	if ( allyModelIndex >= 0 && allyModelIndex < OSP2BE_MAX_CONTROLS && allyDefaultIndex >= 0 && allyDefaultIndex < OSP2BE_MAX_CONTROLS ) {
		int modelWidth;
		int defaultWidth;
		int gap;
		int buttonHeight;

		buttonHeight = osp2be_button.height;
		defaultWidth = buttonHeight;
		gap = osp2be_button.gap;
		modelWidth = buttonWidth - defaultWidth - gap;

		Osp2Be_PositionButton( allyModelIndex, leftColumnX, rowY, modelWidth, buttonHeight );
		Osp2Be_PositionControls( allyModelIndex, leftColumnX, rowY );

		Osp2Be_PositionButton( allyDefaultIndex, leftColumnX + modelWidth + gap, rowY, defaultWidth, buttonHeight );
		Osp2Be_PositionControls( allyDefaultIndex, leftColumnX + modelWidth + gap, rowY );

		rowY += rowHeight;
	}

	if ( allyHeadIndex >= 0 && allyHeadIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyHeadIndex].string = "Head";
		s_osp2be.labels[allyHeadIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[allyHeadIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[allyHeadIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[allyHeadIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[allyHeadIndex].generic.x = leftColumnX + osp2be_position.label_x_offset;
		s_osp2be.labels[allyHeadIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( allyHeadIndex );
		Osp2Be_PositionButton( allyHeadIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyHeadIndex, leftColumnX, rowY );
		rowY += rowHeight;
	}

	if ( allyTorsoIndex >= 0 && allyTorsoIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyTorsoIndex].string = "Body";
		s_osp2be.labels[allyTorsoIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[allyTorsoIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[allyTorsoIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[allyTorsoIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[allyTorsoIndex].generic.x = leftColumnX + osp2be_position.label_x_offset;
		s_osp2be.labels[allyTorsoIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( allyTorsoIndex );
		Osp2Be_PositionButton( allyTorsoIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyTorsoIndex, leftColumnX, rowY );
		rowY += rowHeight;
	}

	if ( allyLegsIndex >= 0 && allyLegsIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyLegsIndex].string = "Legs";
		s_osp2be.labels[allyLegsIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[allyLegsIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[allyLegsIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[allyLegsIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[allyLegsIndex].generic.x = leftColumnX + osp2be_position.label_x_offset;
		s_osp2be.labels[allyLegsIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( allyLegsIndex );
		Osp2Be_PositionButton( allyLegsIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyLegsIndex, leftColumnX, rowY );
		rowY += rowHeight;
	}

	if ( allyRailBeamIndex >= 0 && allyRailBeamIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( allyRailBeamIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyRailBeamIndex, leftColumnX, rowY );
		rowY += rowHeight;
	}

	if ( allyRailRingIndex >= 0 && allyRailRingIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( allyRailRingIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyRailRingIndex, leftColumnX, rowY );
		rowY += rowHeight;
	}

	if ( allyOutlineIndex >= 0 && allyOutlineIndex < OSP2BE_MAX_CONTROLS && allySizeIndex >= 0 && allySizeIndex < OSP2BE_MAX_CONTROLS ) {
		int outlineWidth;
		int sizeWidth;
		int gap;
		int buttonHeight;

		buttonHeight = osp2be_button.height;
		sizeWidth = buttonHeight;
		gap = osp2be_button.gap;
		outlineWidth = buttonWidth - sizeWidth - gap;

		Osp2Be_PositionButton( allyOutlineIndex, leftColumnX, rowY, outlineWidth, buttonHeight );
		Osp2Be_PositionControls( allyOutlineIndex, leftColumnX, rowY );

		Osp2Be_PositionButton( allySizeIndex, leftColumnX + outlineWidth + gap, rowY, sizeWidth, buttonHeight );
		Osp2Be_PositionControls( allySizeIndex, leftColumnX + outlineWidth + gap, rowY );

		rowY += rowHeight;
	}

	if ( allyFrozenIndex >= 0 && allyFrozenIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( allyFrozenIndex, leftColumnX, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( allyFrozenIndex, leftColumnX, rowY );
	}

	rowY = baseY + 8 + 16;

	if ( enemyModelIndex >= 0 && enemyModelIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyModelIndex].string = "Enemy";
		s_osp2be.labels[enemyModelIndex].generic.flags &= ~QMF_HIDDEN;
		s_osp2be.labels[enemyModelIndex].generic.flags |= QMF_INACTIVE;
		s_osp2be.labels[enemyModelIndex].style = UI_CENTER|UI_SMALLFONT;
		s_osp2be.labels[enemyModelIndex].generic.x = rightColumnX + buttonWidth / 2;
		s_osp2be.labels[enemyModelIndex].generic.y = headerY;
	}

	if ( enemyModelIndex >= 0 && enemyModelIndex < OSP2BE_MAX_CONTROLS && enemyDefaultIndex >= 0 && enemyDefaultIndex < OSP2BE_MAX_CONTROLS ) {
		int modelWidth;
		int defaultWidth;
		int gap;
		int buttonHeight;
		int defaultX;
		int modelX;

		buttonHeight = osp2be_button.height;
		defaultWidth = buttonHeight;
		gap = osp2be_button.gap;
		modelWidth = buttonWidth - defaultWidth - gap;

		defaultX = rightColumnX + buttonWidth - defaultWidth + 24;
		modelX = rightColumnX + buttonWidth - defaultWidth - gap - modelWidth + 24;

		Osp2Be_PositionButton( enemyModelIndex, modelX, rowY, modelWidth, buttonHeight );
		Osp2Be_PositionControls( enemyModelIndex, modelX, rowY );

		Osp2Be_PositionButton( enemyDefaultIndex, defaultX, rowY, defaultWidth, buttonHeight );
		Osp2Be_PositionControls( enemyDefaultIndex, defaultX, rowY );

		rowY += rowHeight;
	}

	if ( enemyHeadIndex >= 0 && enemyHeadIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyHeadIndex].string = "Head";
		s_osp2be.labels[enemyHeadIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[enemyHeadIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[enemyHeadIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[enemyHeadIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[enemyHeadIndex].generic.x = rightColumnX + osp2be_position.label_x_offset + 24;
		s_osp2be.labels[enemyHeadIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( enemyHeadIndex );
		Osp2Be_PositionButton( enemyHeadIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyHeadIndex, rightColumnX + 24, rowY );
		rowY += rowHeight;
	}

	if ( enemyTorsoIndex >= 0 && enemyTorsoIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyTorsoIndex].string = "Body";
		s_osp2be.labels[enemyTorsoIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[enemyTorsoIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[enemyTorsoIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[enemyTorsoIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[enemyTorsoIndex].generic.x = rightColumnX + osp2be_position.label_x_offset + 24;
		s_osp2be.labels[enemyTorsoIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( enemyTorsoIndex );
		Osp2Be_PositionButton( enemyTorsoIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyTorsoIndex, rightColumnX + 24, rowY );
		rowY += rowHeight;
	}

	if ( enemyLegsIndex >= 0 && enemyLegsIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyLegsIndex].string = "Legs";
		s_osp2be.labels[enemyLegsIndex].generic.flags |= QMF_HIDDEN;
		s_osp2be.labels[enemyLegsIndex].generic.flags &= ~QMF_INACTIVE;
		s_osp2be.labels[enemyLegsIndex].generic.flags &= ~QMF_GRAYED;
		s_osp2be.labels[enemyLegsIndex].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[enemyLegsIndex].generic.x = rightColumnX + osp2be_position.label_x_offset + 24;
		s_osp2be.labels[enemyLegsIndex].generic.y = rowY + (osp2be_row.height - osp2be_text.char_height) / 2;
		Osp2Be_SetLabelColorWhite( enemyLegsIndex );
		Osp2Be_PositionButton( enemyLegsIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyLegsIndex, rightColumnX + 24, rowY );
		rowY += rowHeight;
	}

	if ( enemyRailBeamIndex >= 0 && enemyRailBeamIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( enemyRailBeamIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyRailBeamIndex, rightColumnX + 24, rowY );
		rowY += rowHeight;
	}

	if ( enemyRailRingIndex >= 0 && enemyRailRingIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( enemyRailRingIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyRailRingIndex, rightColumnX + 24, rowY );
		rowY += rowHeight;
	}

	if ( enemyOutlineIndex >= 0 && enemyOutlineIndex < OSP2BE_MAX_CONTROLS && enemySizeIndex >= 0 && enemySizeIndex < OSP2BE_MAX_CONTROLS ) {
		int outlineWidth;
		int sizeWidth;
		int gap;
		int buttonHeight;
		int sizeX;
		int outlineX;

		buttonHeight = osp2be_button.height;
		sizeWidth = buttonHeight;
		gap = osp2be_button.gap;
		outlineWidth = buttonWidth - sizeWidth - gap;

		sizeX = rightColumnX + buttonWidth - sizeWidth + 24;
		outlineX = rightColumnX + buttonWidth - sizeWidth - gap - outlineWidth + 24;

		Osp2Be_PositionButton( enemyOutlineIndex, outlineX, rowY, outlineWidth, buttonHeight );
		Osp2Be_PositionControls( enemyOutlineIndex, outlineX, rowY );

		Osp2Be_PositionButton( enemySizeIndex, sizeX, rowY, sizeWidth, buttonHeight );
		Osp2Be_PositionControls( enemySizeIndex, sizeX, rowY );

		rowY += rowHeight;
	}

	if ( enemyFrozenIndex >= 0 && enemyFrozenIndex < OSP2BE_MAX_CONTROLS ) {
		Osp2Be_PositionButton( enemyFrozenIndex, rightColumnX + 24, rowY, buttonWidth, osp2be_button.height );
		Osp2Be_PositionControls( enemyFrozenIndex, rightColumnX + 24, rowY );
	}

	/* Position preview on the right side, after enemy column */
	{
		int enemyColumnEndX;
		int previewOffset;

		enemyColumnEndX = rightColumnX + 24 + buttonWidth;
		previewOffset = 18;
		s_osp2be.preview.generic.x = enemyColumnEndX + previewOffset;
	}
	s_osp2be.preview.generic.y = osp2be_layout.preview_y + 64;
	
	/* Position preview buttons above preview window, at params level */
	{
		int buttonY;
		int buttonSpacing;
		int buttonWidth;
		int buttonHeight;
		int frozenButtonX;
		int allyButtonX;
		int enemyButtonX;
		int paramsStartY;
		
		paramsStartY = s_osp2be.defaultRowY[0] + osp2be_bg_layout.params_top_offset;
		buttonY = paramsStartY;
		buttonSpacing = 8;
		buttonWidth = 40;
		buttonHeight = 20;
		
		/* Frozen button on the right */
		frozenButtonX = s_osp2be.preview.generic.x + s_osp2be.preview.width - 32;
		s_osp2be.previewFrozenButton.generic.x = frozenButtonX;
		s_osp2be.previewFrozenButton.generic.y = buttonY;
		s_osp2be.previewFrozenButton.generic.left = frozenButtonX;
		s_osp2be.previewFrozenButton.generic.right = frozenButtonX + s_osp2be.previewFrozenButton.width;
		s_osp2be.previewFrozenButton.generic.top = buttonY;
		s_osp2be.previewFrozenButton.generic.bottom = buttonY + s_osp2be.previewFrozenButton.height;
		
		/* Ally and Enemy buttons to the left of frozen button */
		allyButtonX = frozenButtonX - buttonWidth - buttonSpacing;
		enemyButtonX = allyButtonX - buttonWidth - buttonSpacing;
		
		s_osp2be.teamsAllyButton.generic.x = allyButtonX;
		s_osp2be.teamsAllyButton.generic.y = buttonY;
		s_osp2be.teamsAllyButton.width = buttonWidth;
		s_osp2be.teamsAllyButton.height = buttonHeight;
		s_osp2be.teamsAllyButton.generic.left = allyButtonX;
		s_osp2be.teamsAllyButton.generic.right = allyButtonX + buttonWidth;
		s_osp2be.teamsAllyButton.generic.top = buttonY;
		s_osp2be.teamsAllyButton.generic.bottom = buttonY + buttonHeight;
		
		s_osp2be.teamsEnemyButton.generic.x = enemyButtonX;
		s_osp2be.teamsEnemyButton.generic.y = buttonY;
		s_osp2be.teamsEnemyButton.width = buttonWidth;
		s_osp2be.teamsEnemyButton.height = buttonHeight;
		s_osp2be.teamsEnemyButton.generic.left = enemyButtonX;
		s_osp2be.teamsEnemyButton.generic.right = enemyButtonX + buttonWidth;
		s_osp2be.teamsEnemyButton.generic.top = buttonY;
		s_osp2be.teamsEnemyButton.generic.bottom = buttonY + buttonHeight;
	}

	/* Ensure labels are white and not grayed */
	if ( allyHeadIndex >= 0 && allyHeadIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyHeadIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( allyHeadIndex );
	}
	if ( allyTorsoIndex >= 0 && allyTorsoIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyTorsoIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( allyTorsoIndex );
	}
	if ( allyLegsIndex >= 0 && allyLegsIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[allyLegsIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( allyLegsIndex );
	}
	if ( enemyHeadIndex >= 0 && enemyHeadIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyHeadIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( enemyHeadIndex );
	}
	if ( enemyTorsoIndex >= 0 && enemyTorsoIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyTorsoIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( enemyTorsoIndex );
	}
	if ( enemyLegsIndex >= 0 && enemyLegsIndex < OSP2BE_MAX_CONTROLS ) {
		s_osp2be.labels[enemyLegsIndex].generic.flags &= ~(QMF_INACTIVE|QMF_GRAYED);
		Osp2Be_SetLabelColorWhite( enemyLegsIndex );
	}
}

static qboolean Osp2Be_GetControlColor( int controlIndex, vec4_t out, char *rawValue, int rawValueSize ) {
	const osp2beCvarDef_t *def;
	char temp[64];
	int segment;

	if ( rawValueSize > 0 ) {
		rawValue[0] = '\0';
	}

	if ( controlIndex < 0 || controlIndex >= OSP2BE_MAX_CONTROLS ) {
		return qfalse;
	}

	def = s_osp2be.activeDefs[controlIndex];
	if ( !def ) {
		return qfalse;
	}

	if ( s_osp2be.fields[controlIndex].field.buffer[0] ) {
		Q_strncpyz( temp, s_osp2be.fields[controlIndex].field.buffer, sizeof( temp ) );
		if ( rawValueSize > 0 ) {
			Q_strncpyz( rawValue, temp, rawValueSize );
		}
		if ( Osp2Be_ParseColorValue( temp, out ) ) {
			return qtrue;
		}
	}

	temp[0] = '\0';

	if ( def->cvarName && def->cvarName[0] ) {
		segment = Osp2Be_GetModelSegmentIndex( def->special );
		if ( segment >= 0 ) {
			Osp2Be_ReadModelSegment( def->cvarName, segment, temp, sizeof( temp ) );
		} else {
			segment = Osp2Be_GetRailSegmentIndex( def->special );
			if ( segment >= 0 ) {
				Osp2Be_ReadRailSegment( def->cvarName, segment, temp, sizeof( temp ) );
			}
		}

		if ( !temp[0] ) {
			trap_Cvar_VariableStringBuffer( def->cvarName, temp, sizeof( temp ) );
		}

		if ( temp[0] ) {
			if ( rawValueSize > 0 ) {
				Q_strncpyz( rawValue, temp, rawValueSize );
			}
			if ( Osp2Be_ParseColorValue( temp, out ) ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

static void Osp2Be_OpenColorPicker( int controlIndex ) {
	const osp2beCvarDef_t *def;
	char initialHex[16];
	char rawValue[64];
	vec4_t parsed;
	qboolean parsedColor;
	int red;
	int green;
	int blue;

	if ( controlIndex < 0 || controlIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	def = s_osp2be.activeDefs[controlIndex];
	if ( !def ) {
		return;
	}

	if ( !Osp2Be_CanUseColorPicker( def ) ) {
		return;
	}

	parsedColor = Osp2Be_GetControlColor( controlIndex, parsed, rawValue, sizeof( rawValue ) );
	if ( parsedColor && rawValue[0] ) {
		Q_strncpyz( initialHex, rawValue, sizeof( initialHex ) );
	} else if ( parsedColor ) {
		red = (int)( parsed[0] * 255.0f + 0.5f );
		green = (int)( parsed[1] * 255.0f + 0.5f );
		blue = (int)( parsed[2] * 255.0f + 0.5f );
		if ( red < 0 ) {
			red = 0;
		} else if ( red > 255 ) {
			red = 255;
		}
		if ( green < 0 ) {
			green = 0;
		} else if ( green > 255 ) {
			green = 255;
		}
		if ( blue < 0 ) {
			blue = 0;
		} else if ( blue > 255 ) {
			blue = 255;
		}
		Com_sprintf( initialHex, sizeof( initialHex ), "%02X%02X%02X", red, green, blue );
	} else if ( rawValue[0] ) {
		Q_strncpyz( initialHex, rawValue, sizeof( initialHex ) );
	} else {
		initialHex[0] = '\0';
	}

	if ( !initialHex[0] ) {
		Q_strncpyz( initialHex, "FFFFFF", sizeof( initialHex ) );
	}

	s_osp2be.colorPickerTargetIndex = controlIndex;
	Osp2Be_SetMainControlsEnabled( qfalse );
	UI_ColorPicker_Open( &s_osp2be.colorPicker, initialHex, Osp2Be_ColorPickerApply, NULL );
	trap_Key_ClearStates();
}

static void Osp2Be_FieldEvent( void *ptr, int event ) {
	menucommon_s *item;
	int fieldIndex;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	fieldIndex = item->id - ID_FIELD_BASE;
	if ( fieldIndex < 0 || fieldIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	if ( s_osp2be.activeTypes[fieldIndex] != OSP2BE_CONTROL_COLOR_BUTTON ) {
		return;
	}

	Osp2Be_OpenColorPicker( fieldIndex );
}

static void Osp2Be_ColorPickerApply( const char *hexValue, void *userData ) {
	int targetIndex;
	const osp2beCvarDef_t *def;
	qboolean switchToAlly;
	qboolean switchToEnemy;
	int savedPreviewCategory;

	(void)userData;

	targetIndex = s_osp2be.colorPickerTargetIndex;
	switchToAlly = qfalse;
	switchToEnemy = qfalse;

	/* Save preview category to restore it if mode doesn't switch */
	savedPreviewCategory = s_osp2be.previewCategory;

	if ( targetIndex >= 0 && targetIndex < OSP2BE_MAX_CONTROLS && hexValue && hexValue[0] ) {
		def = s_osp2be.activeDefs[targetIndex];
		if ( def && def->cvarName && s_osp2be.activeCategory == 3 ) {
			if ( !Q_stricmp( def->cvarName, "cg_teamModelColors" ) ||
			     !Q_stricmp( def->cvarName, "cg_teamRailColors" ) ||
			     !Q_stricmp( def->cvarName, "cg_teamOutlineColor" ) ||
			     !Q_stricmp( def->cvarName, "cg_teamFrozenColor" ) ) {
				switchToAlly = qtrue;
			} else if ( !Q_stricmp( def->cvarName, "cg_enemyModelColors" ) ||
			            !Q_stricmp( def->cvarName, "cg_enemyRailColors" ) ||
			            !Q_stricmp( def->cvarName, "cg_enemyOutlineColor" ) ||
			            !Q_stricmp( def->cvarName, "cg_enemyFrozenColor" ) ) {
				switchToEnemy = qtrue;
			}
		}

		Q_strncpyz( s_osp2be.fields[targetIndex].field.buffer, hexValue, sizeof( s_osp2be.fields[targetIndex].field.buffer ) );
		Osp2Be_ClampFieldLength( &s_osp2be.fields[targetIndex] );
		Osp2Be_BufferedFieldChange( targetIndex );
		Osp2Be_SetStatus( "Updated color" );

		if ( switchToAlly && s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qfalse;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		} else if ( switchToEnemy && !s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qtrue;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		} else {
			/* If mode didn't switch, restore saved preview category */
			s_osp2be.previewCategory = savedPreviewCategory;
		}
		Osp2Be_UpdatePreviewModel();
	}

	Osp2Be_SetMainControlsEnabled( qtrue );
	if ( targetIndex >= 0 && targetIndex < OSP2BE_MAX_CONTROLS ) {
		if ( s_osp2be.activeTypes[targetIndex] == OSP2BE_CONTROL_COLOR_BUTTON ) {
			Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.buttons[targetIndex] );
		} else {
		Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.fields[targetIndex] );
		}
	}
	s_osp2be.colorPickerTargetIndex = -1;
}

static void Osp2Be_SetStatus( const char *text ) {
	Q_strncpyz( s_osp2be.statusText, text, sizeof( s_osp2be.statusText ) );
	s_osp2be.statusTime = uis.realtime;
}

static void Osp2Be_HideAllControls( void ) {
	int i;
	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		s_osp2be.activeDefs[i] = NULL;
		s_osp2be.activeTypes[i] = OSP2BE_CONTROL_FIELD;
		s_osp2be.labels[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.fields[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.toggles[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.lists[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.buttons[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.sliders[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	}
}

static void Osp2Be_ResetControlPositions( void ) {
	int i;
	int rowY;
	int sliderY;

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		rowY = s_osp2be.defaultRowY[i];
		sliderY = rowY + s_osp2be.sliderRowOffset;

		s_osp2be.labels[i].generic.y = rowY;
		s_osp2be.fields[i].generic.y = rowY;
		s_osp2be.toggles[i].generic.y = rowY;
		s_osp2be.lists[i].generic.y = rowY;
		s_osp2be.buttons[i].generic.y = rowY;
		s_osp2be.sliders[i].generic.y = sliderY;
		s_osp2be.sliders[i].generic.top = sliderY - 6;
		s_osp2be.sliders[i].generic.bottom = sliderY + SMALLCHAR_HEIGHT + 6;
	}
}

static void Osp2Be_UpdateLayoutPositions( void ) {
	int screenCenterX;
	int designCenterX;
	int offsetX;
	int singlePageBgX;
	int categoryTotalWidth;
	int categoryX;

	screenCenterX = (int)(uis.screenXmin + uis.screenXmax) / 2;
	designCenterX = osp2be_layout.design_width / 2;
	offsetX = screenCenterX - designCenterX;
	categoryTotalWidth = ( ( s_osp2be.category.width + s_osp2be.category.seperation ) * s_osp2be.category.columns - s_osp2be.category.seperation ) * SMALLCHAR_WIDTH;

	singlePageBgX = offsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);
	s_osp2be.contentLeft = singlePageBgX + (osp2be_layout.single_page_bg_width - osp2be_layout.form_width) / 2;
		categoryX = singlePageBgX + (osp2be_layout.single_page_bg_width - categoryTotalWidth) / 2;

	s_osp2be.category.generic.x = categoryX;
	s_osp2be.category.generic.left = categoryX;
	s_osp2be.category.generic.right = categoryX + categoryTotalWidth;
}

static void Osp2Be_UpdatePreviewVisibility( void ) {
	if ( s_osp2be.activeCategory == 1 || s_osp2be.activeCategory == 3 ) {
		s_osp2be.preview.generic.flags &= ~(unsigned int)QMF_HIDDEN;
		s_osp2be.previewFrozenButton.generic.flags &= ~(unsigned int)QMF_HIDDEN;
		if ( s_osp2be.activeCategory == 3 ) {
			s_osp2be.teamsAllyButton.generic.flags &= ~(unsigned int)QMF_HIDDEN;
			s_osp2be.teamsEnemyButton.generic.flags &= ~(unsigned int)QMF_HIDDEN;
		} else {
			s_osp2be.teamsAllyButton.generic.flags |= QMF_HIDDEN;
			s_osp2be.teamsEnemyButton.generic.flags |= QMF_HIDDEN;
		}
	} else {
		s_osp2be.preview.generic.flags |= QMF_HIDDEN;
		s_osp2be.previewFrozenButton.generic.flags |= QMF_HIDDEN;
		s_osp2be.teamsAllyButton.generic.flags |= QMF_HIDDEN;
		s_osp2be.teamsEnemyButton.generic.flags |= QMF_HIDDEN;
		s_osp2be.previewFrozenActive = qfalse;
	}
}

static void Osp2Be_UpdateControls( void ) {
	const osp2beCategoryDef_t *category;
	int count;
	int i;
	const osp2beControlHandler_t *handler;
	int handlerCount;

	if ( s_osp2be.activeCategory < 0 || s_osp2be.activeCategory >= (int)OSP2BE_CATEGORY_COUNT ) {
		s_osp2be.activeCategory = 0;
	}

	category = &osp2beCategories[s_osp2be.activeCategory];
	Osp2Be_UpdateLayoutPositions();
	Osp2Be_ResetControlPositions();
	Osp2Be_HideAllControls();

	count = category->itemCount;
	if ( count > OSP2BE_MAX_CONTROLS ) {
		count = OSP2BE_MAX_CONTROLS;
	}

	handlerCount = OSP2BE_HANDLER_COUNT;
	s_osp2be.activeControlCount = 0;

	for ( i = 0; i < count; i++ ) {
		const osp2beCvarDef_t *def;
		def = &category->items[i];
		s_osp2be.activeDefs[i] = def;
		s_osp2be.activeTypes[i] = def->controlType;

		if ( def->special == OSP2BE_SPECIAL_SECTION_HEADER ) {
			int screenCenterX;
			int designCenterX;
			int offsetX;
			int singlePageBgX;
			
			screenCenterX = (int)(uis.screenXmin + uis.screenXmax) / 2;
			designCenterX = osp2be_layout.design_width / 2;
			offsetX = screenCenterX - designCenterX;
			singlePageBgX = offsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);
			
			s_osp2be.labels[i].string = (char *)def->label;
			s_osp2be.labels[i].generic.flags &= ~QMF_HIDDEN;
			s_osp2be.labels[i].generic.flags |= QMF_INACTIVE;
			s_osp2be.labels[i].style = UI_CENTER|UI_SMALLFONT;
			s_osp2be.labels[i].generic.x = singlePageBgX + osp2be_layout.single_page_bg_width / 2;
			s_osp2be.fields[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
			s_osp2be.toggles[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
			s_osp2be.lists[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
			s_osp2be.buttons[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
			s_osp2be.sliders[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
			continue;
		}

		if ( def->special >= OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET &&
			 def->special <= OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG ) {
			s_osp2be.labels[i].string = NULL;
			s_osp2be.labels[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		}

		if ( def->special == OSP2BE_SPECIAL_MODEL_SELECTOR ) {
			const char *bufferedModel;

			if ( s_osp2be.activeCategory == 3 ) {
				if ( def->cvarName && def->cvarName[0] ) {
					if ( !Q_stricmp( def->cvarName, "cg_teamModel" ) ) {
						if ( s_osp2be.bufferedAllyModel[0] ) {
							Q_strncpyz( s_osp2be.fields[i].field.buffer, s_osp2be.bufferedAllyModel, sizeof( s_osp2be.fields[i].field.buffer ) );
						} else {
							trap_Cvar_VariableStringBuffer( def->cvarName, s_osp2be.fields[i].field.buffer, sizeof( s_osp2be.fields[i].field.buffer ) );
						}
					} else if ( !Q_stricmp( def->cvarName, "cg_enemyModel" ) ) {
						if ( s_osp2be.bufferedEnemyModel[0] ) {
							Q_strncpyz( s_osp2be.fields[i].field.buffer, s_osp2be.bufferedEnemyModel, sizeof( s_osp2be.fields[i].field.buffer ) );
						} else {
							trap_Cvar_VariableStringBuffer( def->cvarName, s_osp2be.fields[i].field.buffer, sizeof( s_osp2be.fields[i].field.buffer ) );
						}
					} else {
						trap_Cvar_VariableStringBuffer( def->cvarName, s_osp2be.fields[i].field.buffer, sizeof( s_osp2be.fields[i].field.buffer ) );
					}
				} else {
					s_osp2be.fields[i].field.buffer[0] = '\0';
				}
			} else {
				bufferedModel = Osp2Be_GetBufferedModelForCategory( s_osp2be.activeCategory );
				if ( bufferedModel && bufferedModel[0] ) {
					Q_strncpyz( s_osp2be.fields[i].field.buffer, bufferedModel, sizeof( s_osp2be.fields[i].field.buffer ) );
				} else if ( def->cvarName && def->cvarName[0] ) {
					trap_Cvar_VariableStringBuffer( def->cvarName, s_osp2be.fields[i].field.buffer, sizeof( s_osp2be.fields[i].field.buffer ) );
				} else {
					s_osp2be.fields[i].field.buffer[0] = '\0';
				}
			}
			Osp2Be_ClampFieldLength( &s_osp2be.fields[i] );
		}

		if ( def->controlType == OSP2BE_CONTROL_COLOR_BUTTON || def->controlType == OSP2BE_CONTROL_BUTTON || def->controlType == OSP2BE_CONTROL_SIZE_BUTTON ) {
			s_osp2be.labels[i].string = NULL;
			s_osp2be.labels[i].generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
		} else {
			s_osp2be.labels[i].string = (char *)def->label;
			s_osp2be.labels[i].generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
			if ( s_osp2be.activeCategory == 3 && ( def->special == OSP2BE_SPECIAL_MODEL_HEAD || def->special == OSP2BE_SPECIAL_MODEL_TORSO || def->special == OSP2BE_SPECIAL_MODEL_LEGS ) ) {
				s_osp2be.labels[i].color = color_white;
			}
		}

		handler = NULL;
		if ( def->controlType >= 0 && def->controlType < handlerCount ) {
			handler = &osp2beControlHandlers[def->controlType];
		}

		if ( handler && handler->setup ) {
			handler->setup( i, def );
		}
	}

	Osp2Be_UpdatePreviewVisibility();
	if ( s_osp2be.activeCategory == 3 ) {
		s_osp2be.teamsPreviewMode = qfalse;
		s_osp2be.previewCategory = -1;
		Osp2Be_ApplyTeamsLayout();
		Osp2Be_UpdatePreviewVisibility();
		Osp2Be_UpdatePreviewModel();
	} else if ( s_osp2be.activeCategory != 0 ) {
		Osp2Be_UpdatePreviewModel();
	}
	if ( s_osp2be.activeCategory >= 1 && s_osp2be.activeCategory <= 2 ) {
		Osp2Be_ApplyHorizontalColorLayout();
	}

	s_osp2be.activeControlCount = count;
}

void Osp2Be_SelectCategory( int index ) {
	Osp2Be_SaveActiveControlsToBuffer();

	if ( index < 0 ) {
		index = 0;
	}
	if ( index >= (int)OSP2BE_CATEGORY_COUNT ) {
		index = (int)OSP2BE_CATEGORY_COUNT - 1;
	}
	if ( s_osp2be.activeCategory != index ) {
		s_osp2be.activeCategory = index;
		Osp2Be_UpdateControls();
	}
	Osp2Be_UpdatePreviewModel();
}

static void Osp2Be_ApplyControls( void ) {
	int i;

	if ( !s_osp2be.bufferedInitialized ) {
		Osp2Be_LoadBufferedSettings( qtrue );
	}

	Osp2Be_SaveActiveControlsToBuffer();

	for ( i = 0; i < (int)OSP2BE_CATEGORY_COUNT; i++ ) {
		Osp2Be_ApplyBufferedCategory( i );
	}

	if ( s_osp2be.bufferedPlayerModel[0] ) {
		trap_Cvar_Set( "model", s_osp2be.bufferedPlayerModel );
		if ( s_osp2be.bufferedPlayerHead[0] ) {
			trap_Cvar_Set( "headmodel", s_osp2be.bufferedPlayerHead );
		} else {
			trap_Cvar_Set( "headmodel", s_osp2be.bufferedPlayerModel );
			Q_strncpyz( s_osp2be.bufferedPlayerHead, s_osp2be.bufferedPlayerModel, sizeof( s_osp2be.bufferedPlayerHead ) );
		}
	}

	if ( s_osp2be.bufferedAllyModel[0] ) {
		trap_Cvar_Set( "cg_teamModel", s_osp2be.bufferedAllyModel );
	}

	if ( s_osp2be.bufferedEnemyModel[0] ) {
		trap_Cvar_Set( "cg_enemyModel", s_osp2be.bufferedEnemyModel );
	}

	Osp2Be_LoadBufferedModelsFromCvars();
	Osp2Be_LoadBufferedSettings( qtrue );
	Osp2Be_UpdateControls();
	Osp2Be_SetStatus( "Applied buffered settings" );
	Osp2Be_UpdatePreviewModel();
}

static void Osp2Be_ApplyBufferedCategory( int categoryIndex ) {
	const osp2beCategoryDef_t *category;
	int count;
	int i;
	int j;
	osp2beModelAssembly_t modelAssemblies[OSP2BE_MAX_CONTROLS];
	osp2beRailAssembly_t railAssemblies[OSP2BE_MAX_CONTROLS];
	int modelAssemblyCount;
	int railAssemblyCount;

	if ( categoryIndex < 0 || categoryIndex >= (int)OSP2BE_CATEGORY_COUNT ) {
		return;
	}

	category = &osp2beCategories[categoryIndex];
	count = category->itemCount;
	if ( count > OSP2BE_MAX_CONTROLS ) {
		count = OSP2BE_MAX_CONTROLS;
	}

	modelAssemblyCount = 0;
	railAssemblyCount = 0;

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		modelAssemblies[i].cvarName = NULL;
		for ( j = 0; j < 3; j++ ) {
			modelAssemblies[i].segments[j][0] = '\0';
			modelAssemblies[i].used[j] = qfalse;
		}
		railAssemblies[i].cvarName = NULL;
		for ( j = 0; j < 2; j++ ) {
			railAssemblies[i].segments[j][0] = '\0';
			railAssemblies[i].used[j] = qfalse;
		}
	}

	for ( i = 0; i < count; i++ ) {
		const osp2beCvarDef_t *def;
		const char *fieldValue;
		int toggleValue;
		int listValue;
		float sliderValue;
		int sizeValue;

		def = &category->items[i];
		if ( !def ) {
			continue;
		}

		switch ( def->controlType ) {
		case OSP2BE_CONTROL_FIELD:
		case OSP2BE_CONTROL_COLOR_BUTTON:
			fieldValue = s_osp2be.bufferedFieldValues[categoryIndex][i];
			if ( !fieldValue ) {
				fieldValue = "";
			}
			Osp2Be_WriteFieldValue( def, fieldValue, modelAssemblies, &modelAssemblyCount, railAssemblies, &railAssemblyCount );
			break;
		case OSP2BE_CONTROL_TOGGLE:
			if ( def->cvarName && def->cvarName[0] ) {
				toggleValue = s_osp2be.bufferedToggleValues[categoryIndex][i];
				trap_Cvar_SetValue( def->cvarName, toggleValue ? 1.0f : 0.0f );
			}
			break;
		case OSP2BE_CONTROL_LIST:
			if ( def->cvarName && def->cvarName[0] ) {
				listValue = s_osp2be.bufferedListValues[categoryIndex][i];
				trap_Cvar_SetValue( def->cvarName, listValue );
			}
			break;
		case OSP2BE_CONTROL_SLIDER:
			if ( def->cvarName && def->cvarName[0] ) {
				sliderValue = s_osp2be.bufferedSliderValues[categoryIndex][i];
				sliderValue = Osp2Be_SliderClampValue( def, sliderValue );
				trap_Cvar_SetValue( def->cvarName, sliderValue );
				s_osp2be.appliedSliderValues[categoryIndex][i] = sliderValue;
			} else {
				s_osp2be.appliedSliderValues[categoryIndex][i] = s_osp2be.bufferedSliderValues[categoryIndex][i];
			}
			break;
		case OSP2BE_CONTROL_SIZE_BUTTON:
			if ( def->cvarName && def->cvarName[0] ) {
				sizeValue = s_osp2be.bufferedSizeValues[categoryIndex][i];
				if ( sizeValue < 0 ) {
					sizeValue = 0;
				}
				if ( sizeValue > 3 ) {
					sizeValue = sizeValue % 4;
				}
				trap_Cvar_SetValue( def->cvarName, sizeValue );
			}
			break;
		default:
			break;
		}
	}

	for ( i = 0; i < modelAssemblyCount; i++ ) {
		if ( modelAssemblies[i].used[0] || modelAssemblies[i].used[1] || modelAssemblies[i].used[2] ) {
			Osp2Be_WriteModelSegments( modelAssemblies[i].cvarName, modelAssemblies[i].segments );
		}
	}

	for ( i = 0; i < railAssemblyCount; i++ ) {
		if ( railAssemblies[i].used[0] || railAssemblies[i].used[1] ) {
			Osp2Be_WriteRailSegments( railAssemblies[i].cvarName, railAssemblies[i].segments );
		}
	}
}

static void Osp2Be_ReloadControls( void ) {
	Osp2Be_ResetBufferedSettings();
	Osp2Be_LoadBufferedSettings( qtrue );
	Osp2Be_LoadBufferedModelsFromCvars();
	Osp2Be_UpdateControls();
	Osp2Be_SetStatus( "Reloaded from cvars" );
	Osp2Be_UpdatePreviewModel();
}

static void Osp2Be_ToggleEvent( void *ptr, int event ) {
	menucommon_s *item;
	int index;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	index = item->id - ID_TOGGLE_BASE;
	if ( index < 0 || index >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	if ( s_osp2be.activeDefs[index] && s_osp2be.activeDefs[index]->controlType == OSP2BE_CONTROL_TOGGLE ) {
		Osp2Be_BufferedToggleChange( index );
		Osp2Be_SetStatus( "Updated option" );
		Osp2Be_UpdatePreviewModel();
	}
}

static void Osp2Be_ListEvent( void *ptr, int event ) {
	menucommon_s *item;
	int index;
	int listIndex;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	listIndex = item->id - ( ID_FIELD_BASE + OSP2BE_MAX_CONTROLS );
	if ( listIndex < 0 || listIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	index = listIndex;
	if ( s_osp2be.activeDefs[index] && s_osp2be.activeDefs[index]->controlType == OSP2BE_CONTROL_LIST ) {
		Osp2Be_BufferedListChange( index );
		Osp2Be_SetStatus( "Updated option" );
		Osp2Be_UpdatePreviewModel();
	}
}

static void Osp2Be_ButtonEvent( void *ptr, int event ) {
	menucommon_s *item;
	int buttonIndex;
	const osp2beCvarDef_t *def;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	buttonIndex = item->id - ( ID_FIELD_BASE + OSP2BE_MAX_CONTROLS * 2 );
	if ( buttonIndex < 0 || buttonIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	def = s_osp2be.activeDefs[buttonIndex];
	if ( !def ) {
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_BUTTON && def->special == OSP2BE_SPECIAL_MODEL_DEFAULT ) {
		qboolean switchToAlly;
		qboolean switchToEnemy;

		switchToAlly = qfalse;
		switchToEnemy = qfalse;

		if ( def->cvarName && def->cvarName[0] ) {
			trap_Cvar_Set( def->cvarName, "" );
			Q_strncpyz( s_osp2be.fields[buttonIndex].field.buffer, "", sizeof( s_osp2be.fields[buttonIndex].field.buffer ) );
			Osp2Be_BufferedFieldChange( buttonIndex );

			if ( s_osp2be.activeCategory == 3 ) {
				if ( !Q_stricmp( def->cvarName, "cg_teamModel" ) ) {
					switchToAlly = qtrue;
					s_osp2be.bufferedAllyModel[0] = '\0';
				} else if ( !Q_stricmp( def->cvarName, "cg_enemyModel" ) ) {
					switchToEnemy = qtrue;
					s_osp2be.bufferedEnemyModel[0] = '\0';
				}
			}
		}

		Osp2Be_SetStatus( "Set to default model" );

		if ( switchToAlly && s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qfalse;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		} else if ( switchToEnemy && !s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qtrue;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		}

		Osp2Be_UpdatePreviewModel();
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_BUTTON && def->special == OSP2BE_SPECIAL_SUPERHUD_CONFIG ) {
		UI_SuperHUDConfigMenu();
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_BUTTON && def->special == OSP2BE_SPECIAL_MODEL_SELECTOR ) {
		const char *modelCvar = NULL;
		const char *headCvar = NULL;
		char currentModel[MAX_QPATH];
		char currentHead[MAX_QPATH];

		currentModel[0] = '\0';
		currentHead[0] = '\0';

		if ( s_osp2be.fields[buttonIndex].field.buffer[0] ) {
			Q_strncpyz( currentModel, s_osp2be.fields[buttonIndex].field.buffer, sizeof( currentModel ) );
		} else if ( def->cvarName && def->cvarName[0] ) {
			trap_Cvar_VariableStringBuffer( def->cvarName, currentModel, sizeof( currentModel ) );
		}

		if ( !currentModel[0] ) {
			if ( s_osp2be.activeCategory == 3 ) {
				if ( def->cvarName && !Q_stricmp( def->cvarName, "cg_teamModel" ) ) {
					if ( s_osp2be.bufferedAllyModel[0] ) {
						Q_strncpyz( currentModel, s_osp2be.bufferedAllyModel, sizeof( currentModel ) );
					}
				} else if ( def->cvarName && !Q_stricmp( def->cvarName, "cg_enemyModel" ) ) {
					if ( s_osp2be.bufferedEnemyModel[0] ) {
						Q_strncpyz( currentModel, s_osp2be.bufferedEnemyModel, sizeof( currentModel ) );
					}
				}
			} else {
				const char *bufferedModel = Osp2Be_GetBufferedModelForCategory( s_osp2be.activeCategory );
				if ( bufferedModel && bufferedModel[0] ) {
					Q_strncpyz( currentModel, bufferedModel, sizeof( currentModel ) );
				}
			}
		}

		if ( s_osp2be.activeCategory == 0 && s_osp2be.bufferedPlayerHead[0] ) {
			Q_strncpyz( currentHead, s_osp2be.bufferedPlayerHead, sizeof( currentHead ) );
		} else if ( s_osp2be.activeCategory == 0 ) {
			trap_Cvar_VariableStringBuffer( "headmodel", currentHead, sizeof( currentHead ) );
		}

		Osp2Be_SetMainControlsEnabled( qfalse );
		s_osp2be.pendingModelCategory = s_osp2be.activeCategory;
		s_osp2be.pendingModelButtonIndex = buttonIndex;

		if ( s_osp2be.activeCategory == 0 ) {
			UI_PlayerModelMenu_WithValues(
				currentModel[0] ? currentModel : NULL,
				currentHead[0] ? currentHead : NULL,
				qfalse,
				qfalse,
				Osp2Be_ModelSelectionApplied,
				NULL );
		} else if ( s_osp2be.activeCategory == 5 ) {
			UI_PlayerModelMenu_WithValues(
				currentModel[0] ? currentModel : NULL,
				NULL,
				qtrue,
				( def->cvarName && !Q_stricmp( def->cvarName, "cg_teamModel" ) ),
				Osp2Be_ModelSelectionApplied,
				NULL );
		} else {
			UI_PlayerModelMenu_WithValues(
				currentModel[0] ? currentModel : NULL,
				NULL,
				qtrue,
				( s_osp2be.activeCategory == 2 ),
				Osp2Be_ModelSelectionApplied,
				NULL );
		}
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_SIZE_BUTTON ) {
		float currentValue;
		int sizeValue;
		qboolean switchToAlly;
		qboolean switchToEnemy;

		switchToAlly = qfalse;
		switchToEnemy = qfalse;

		if ( s_osp2be.bufferedInitialized ) {
			sizeValue = s_osp2be.bufferedSizeValues[s_osp2be.activeCategory][buttonIndex];
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
		sizeValue++;
		if ( sizeValue > 3 ) {
			sizeValue = 0;
		}

		if ( def->cvarName && s_osp2be.activeCategory == 3 ) {
			if ( !Q_stricmp( def->cvarName, "cg_teamOutlineSize" ) ) {
				switchToAlly = qtrue;
			} else if ( !Q_stricmp( def->cvarName, "cg_enemyOutlineSize" ) ) {
				switchToEnemy = qtrue;
			}
		}

		Osp2Be_BufferedSizeChange( buttonIndex, sizeValue );
		Osp2Be_SetStatus( "Updated outline size" );

		if ( switchToAlly && s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qfalse;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		} else if ( switchToEnemy && !s_osp2be.teamsPreviewMode ) {
			s_osp2be.teamsPreviewMode = qtrue;
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
		}

		Osp2Be_UpdatePreviewModel();
		return;
	}

	if ( def->controlType == OSP2BE_CONTROL_COLOR_BUTTON ) {
		Osp2Be_OpenColorPicker( buttonIndex );
	}
}

static void Osp2Be_SliderEvent( void *ptr, int event ) {
	menucommon_s *item;
	int sliderIndex;
	const osp2beCvarDef_t *def;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	sliderIndex = item->id - ID_SLIDER_BASE;
	if ( sliderIndex < 0 || sliderIndex >= OSP2BE_MAX_CONTROLS ) {
		return;
	}

	def = s_osp2be.activeDefs[sliderIndex];
	if ( !def ) {
		return;
	}

	Osp2Be_SliderSetValue( sliderIndex, def, s_osp2be.sliders[sliderIndex].curvalue );
	Osp2Be_BufferedSliderChange( sliderIndex );
	Osp2Be_SetStatus( "Updated slider" );
	if ( s_osp2be.activeCategory != 0 ) {
		Osp2Be_UpdatePreviewModel();
	}
}

static void Osp2Be_PreviewFrozenEvent( void *ptr, int event ) {
	(void)ptr;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	s_osp2be.previewFrozenActive = !s_osp2be.previewFrozenActive;
	if ( s_osp2be.previewFrozenActive ) {
		Osp2Be_SetStatus( "Preview frozen" );
	} else {
		Osp2Be_SetStatus( "Preview unfrozen" );
	}
	Osp2Be_UpdatePreviewModel();
}

static void Osp2Be_TeamsPreviewEvent( void *ptr, int event ) {
	menucommon_s *item;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	if ( item->id == ID_TEAMS_ALLY ) {
		if ( s_osp2be.teamsPreviewMode != qfalse ) {
			s_osp2be.teamsPreviewMode = qfalse;
			Osp2Be_SetStatus( "Showing Ally preview" );
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
			Osp2Be_UpdatePreviewModel();
		}
	} else if ( item->id == ID_TEAMS_ENEMY ) {
		if ( s_osp2be.teamsPreviewMode != qtrue ) {
			s_osp2be.teamsPreviewMode = qtrue;
			Osp2Be_SetStatus( "Showing Enemy preview" );
			s_osp2be.previewCategory = -1;
			Osp2Be_UpdatePreviewVisibility();
			Osp2Be_UpdatePreviewModel();
		}
	}
}

static void Osp2Be_MenuEvent( void *ptr, int event ) {
	menucommon_s *item;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	switch ( item->id ) {
	case ID_APPLY:
		Osp2Be_ApplyControls();
		break;
	case ID_RELOAD:
		Osp2Be_ReloadControls();
		break;
	case ID_CLOSE:
		UI_PopMenu();
		break;
	}
}

static void Osp2Be_CategoryEvent( void *ptr, int event ) {
	menulist_s *list;
	static int lastCategoryValue = -1;

	list = (menulist_s *)ptr;
	if ( event == QM_LOSTFOCUS ) {
		lastCategoryValue = -1;
		return;
	}
	if ( event == QM_ACTIVATED || event == QM_GOTFOCUS ) {
		if ( list->curvalue != lastCategoryValue ) {
			Osp2Be_SelectCategory( list->curvalue );
			lastCategoryValue = list->curvalue;
			if ( !( list->generic.flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}
	}
}

static sfxHandle_t Osp2Be_MenuKey( int key ) {
	if ( UI_ColorPicker_IsActive( &s_osp2be.colorPicker ) ) {
		if ( UI_ColorPicker_HandleKey( &s_osp2be.colorPicker, key ) ) {
			Osp2Be_SetMainControlsEnabled( qtrue );
			if ( s_osp2be.colorPickerTargetIndex >= 0 && s_osp2be.colorPickerTargetIndex < OSP2BE_MAX_CONTROLS ) {
				if ( s_osp2be.activeTypes[s_osp2be.colorPickerTargetIndex] == OSP2BE_CONTROL_COLOR_BUTTON ) {
					Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.buttons[s_osp2be.colorPickerTargetIndex] );
				} else {
				Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.fields[s_osp2be.colorPickerTargetIndex] );
				}
			}
			s_osp2be.colorPickerTargetIndex = -1;
			return 0;
		}

		if ( key == K_MOUSE1 ) {
			UI_ColorPicker_HandleMouse( &s_osp2be.colorPicker, uis.cursorx, uis.cursory, qtrue );
			/* Check if picker was closed (e.g., clicked outside) */
			if ( !UI_ColorPicker_IsActive( &s_osp2be.colorPicker ) ) {
				Osp2Be_SetMainControlsEnabled( qtrue );
				if ( s_osp2be.colorPickerTargetIndex >= 0 && s_osp2be.colorPickerTargetIndex < OSP2BE_MAX_CONTROLS ) {
					if ( s_osp2be.activeTypes[s_osp2be.colorPickerTargetIndex] == OSP2BE_CONTROL_COLOR_BUTTON ) {
						Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.buttons[s_osp2be.colorPickerTargetIndex] );
					} else {
						Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.fields[s_osp2be.colorPickerTargetIndex] );
					}
				}
				s_osp2be.colorPickerTargetIndex = -1;
				return 0;
			}
			return menu_move_sound;
		}
		return 0;
	}

	if ( key == K_MOUSE1 ) {
		if ( Osp2Be_HandleSliderMouse( uis.cursorx, uis.cursory ) ) {
			return menu_move_sound;
		}
		if ( UI_ColorPicker_IsActive( &s_osp2be.colorPicker ) ) {
			UI_ColorPicker_HandleMouse( &s_osp2be.colorPicker, uis.cursorx, uis.cursory, qtrue );
			return menu_move_sound;
		}
	}

	if ( key == K_ESCAPE || key == K_MOUSE2 ) {
		UI_PopMenu();
		return menu_out_sound;
	}
	return Menu_DefaultKey( &s_osp2be.menu, key );
}

qboolean Osp2Be_HandleSliderMouse( int cursorX, int cursorY ) {
	int i;

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		const osp2beCvarDef_t *def;
		menuslider_s *slider;
		float sliderWidth;
		float fraction;
		float value;

		if ( s_osp2be.activeTypes[i] != OSP2BE_CONTROL_SLIDER ) {
			continue;
		}
		if ( s_osp2be.sliders[i].generic.flags & (QMF_HIDDEN | QMF_GRAYED | QMF_INACTIVE) ) {
			continue;
		}

		if ( cursorX < s_osp2be.sliders[i].generic.left || cursorX > s_osp2be.sliders[i].generic.right ) {
			continue;
		}
		if ( cursorY >= s_osp2be.sliders[i].generic.top && cursorY <= s_osp2be.sliders[i].generic.bottom ) {
			def = s_osp2be.activeDefs[i];
			if ( !def ) {
				continue;
			}

			slider = &s_osp2be.sliders[i];
			sliderWidth = slider->generic.right - slider->generic.left;
			if ( sliderWidth <= 0.0f ) {
				continue;
			}

			if ( cursorX < slider->generic.left ) {
				cursorX = slider->generic.left;
			}
			if ( cursorX > slider->generic.right ) {
				cursorX = slider->generic.right;
			}

			fraction = ( cursorX - slider->generic.left ) / sliderWidth;
			value = slider->minvalue + fraction * ( slider->maxvalue - slider->minvalue );
			value = Osp2Be_SliderClampValue( def, value );
			Osp2Be_SliderSetValue( i, def, value );
			Osp2Be_BufferedSliderChange( i );
			Osp2Be_SetStatus( "Updated slider" );
			Menu_SetCursorToItem( &s_osp2be.menu, slider );
			if ( s_osp2be.activeCategory != 0 ) {
				Osp2Be_UpdatePreviewModel();
			}
			return qtrue;
		}
	}

	return qfalse;
}

void UI_Osp2BeMenu( void ) {
	unsigned char *bytes;
	int index;
	int total;
	int i;
	int baseY;
	int rowHeight;
	int fieldWidthPixels;
	int fieldX;
	int formRight;
	float screenWidth;
	float contentWidth;
	int formWidth;
	int previewX;
	int buttonSpacing;
	int button1X;
	int button2X;
	int button3X;
	int sliderOffset;
	qboolean isIngameContext;

	isIngameContext = qfalse;
	if ( uis.activemenu && !uis.activemenu->fullscreen ) {
		isIngameContext = qtrue;
	}

	bytes = (unsigned char *)&s_osp2be;
	total = (int)sizeof( s_osp2be );
	for ( index = 0; index < total; index++ ) {
		bytes[index] = 0;
	}

	s_osp2be.pendingModelCategory = -1;
	s_osp2be.pendingModelButtonIndex = -1;

	Osp2Be_LoadBufferedModelsFromCvars();

	Osp2Be_InitWeaponIcons();

	UI_LoadFonts();
	
	/* Register and update font cvar */
	trap_Cvar_Register( &ui_be_font, "ui_be_font", "0", CVAR_ARCHIVE );
	trap_Cvar_Update( &ui_be_font );
	
	/* Select font from cvar */
	{
		int fontIndex;
		fontIndex = UI_FontIndexFromCvar( &ui_be_font );
		if ( !UI_FontAvailable( fontIndex ) ) {
			fontIndex = 0;
			trap_Cvar_Set( "ui_be_font", "0" );
		}
		UI_FontSelect( fontIndex );
		osp2be_font.index = fontIndex;
	}

	osp2beFrozenOverlay = trap_R_RegisterShaderNoMip( "textures/effects/frozen" );
	osp2beFrozenIcon = trap_R_RegisterShaderNoMip( "sprites/frozenFoeTag" );

	/* Initialize category item counts from extern variables */
	osp2beCategories[0].itemCount = osp2beGeneralControlsCount;
	osp2beCategories[1].itemCount = osp2bePlayerControlsCount;
	osp2beCategories[2].itemCount = osp2beWeaponControlsCount;
	osp2beCategories[3].itemCount = osp2beRailgunControlsCount;
	osp2beCategories[4].itemCount = osp2beExplosivesControlsCount;
	osp2beCategories[5].itemCount = osp2beTeamsControlsCount;
	osp2beCategories[6].itemCount = osp2beSuperHUDControlsCount;

	s_osp2be.colorPickerTargetIndex = -1;

	Osp2Be_ResetBufferedSettings();
	Osp2Be_LoadBufferedSettings( qtrue );
	s_osp2be.isIngameContext = isIngameContext;

	if ( s_osp2be.isIngameContext ) {
		Osp2Be_SetColorVec4( osp2be_sidebar_color, 0.0f, 0.0f, 0.0f, 0.25f );
		Osp2Be_SetColorVec4( osp2be_content_color, 0.0f, 0.0f, 0.0f, 0.25f );
	} else {
		Osp2Be_SetColorVec4( osp2be_sidebar_color, 0.0f, 0.0f, 0.0f, 0.90f );
		Osp2Be_SetColorVec4( osp2be_content_color, 0.0f, 0.0f, 0.0f, 0.90f );
	}

	/* Fixed layout for 640x480 (4:3) - full area, centered */
	screenWidth = uis.screenXmax - uis.screenXmin;
	
	/* Center the 640px design on screen */
	{
		int designCenterX;
		int screenCenterX;
		int offsetX;
		int singlePageBgCenterX;
		int singlePageBgOffsetX;
		
		designCenterX = osp2be_layout.design_width / 2;
		screenCenterX = (int)(uis.screenXmin + uis.screenXmax) / 2;
		offsetX = screenCenterX - designCenterX;
		
		s_osp2be.sidebarLeft = offsetX + osp2be_layout.margin_left;
		{
			int singlePageBgX;
			singlePageBgX = offsetX + osp2be_layout.design_center_x - (osp2be_layout.single_page_bg_width / 2);
			s_osp2be.contentLeft = singlePageBgX + (osp2be_layout.single_page_bg_width - osp2be_layout.form_width) / 2;
		}
		s_osp2be.formWidth = osp2be_layout.form_width;
		s_osp2be.previewX = s_osp2be.contentLeft + osp2be_layout.form_width + osp2be_layout.content_gap;
		s_osp2be.screenWidth = osp2be_layout.design_width;
		s_osp2be.contentWidth = osp2be_layout.design_width - osp2be_layout.margin_left - osp2be_layout.sidebar_width - osp2be_layout.content_gap - osp2be_layout.margin_right;
	}
	
	/* Calculate button positions - evenly distributed in form */
	{
		int buttonAreaWidth;
		int buttonAreaStart;
		
		buttonAreaWidth = osp2be_layout.form_width;
		buttonAreaStart = s_osp2be.contentLeft;
		buttonSpacing = buttonAreaWidth / 6;
		button1X = buttonAreaStart + buttonSpacing;
		button2X = buttonAreaStart + buttonAreaWidth / 2;
		button3X = buttonAreaStart + buttonAreaWidth - buttonSpacing;
	}

	s_osp2be.menu.draw = Osp2Be_MenuDraw;
	s_osp2be.menu.key = Osp2Be_MenuKey;
	s_osp2be.menu.fullscreen = s_osp2be.isIngameContext ? qfalse : qtrue;
	s_osp2be.menu.wrapAround = qfalse;

	s_osp2be.category.generic.type = MTYPE_SCROLLLIST;
	s_osp2be.category.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_OWNERDRAW;
	s_osp2be.category.generic.callback = Osp2Be_CategoryEvent;
	s_osp2be.category.generic.id = ID_CATEGORY_LIST;
	s_osp2be.category.generic.x = s_osp2be.contentLeft;
	s_osp2be.category.generic.y = 24 + SMALLCHAR_HEIGHT + 8;
	s_osp2be.category.width = 8;
	s_osp2be.category.height = 1;
	s_osp2be.category.columns = OSP2BE_CATEGORY_COUNT;
	s_osp2be.category.seperation = 1;
	s_osp2be.category.itemnames = osp2beCategoryNames;
	s_osp2be.category.numitems = OSP2BE_CATEGORY_COUNT;
	s_osp2be.category.curvalue = 0;
	s_osp2be.category.top = 0;
	s_osp2be.category.generic.ownerdraw = Osp2Be_DrawCategoryList;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.category );
	{
		int totalWidth;
		int itemHeight;

		totalWidth = ( ( s_osp2be.category.width + s_osp2be.category.seperation ) * s_osp2be.category.columns - s_osp2be.category.seperation ) * SMALLCHAR_WIDTH;
		itemHeight = SMALLCHAR_HEIGHT + 4;
		s_osp2be.category.generic.left = s_osp2be.category.generic.x;
		s_osp2be.category.generic.top = s_osp2be.category.generic.y - 2;
		s_osp2be.category.generic.right = s_osp2be.category.generic.x + totalWidth;
		s_osp2be.category.generic.bottom = s_osp2be.category.generic.y - 2 + itemHeight;
	}

	s_osp2be.apply.generic.type = MTYPE_PTEXT;
	s_osp2be.apply.generic.flags = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_osp2be.apply.generic.callback = Osp2Be_MenuEvent;
	s_osp2be.apply.generic.id = ID_APPLY;
	s_osp2be.apply.generic.x = button1X;
		s_osp2be.apply.generic.y = osp2be_layout.design_height - 24;
	s_osp2be.apply.string = "APPLY";
	s_osp2be.apply.style = UI_CENTER|UI_SMALLFONT;
	s_osp2be.apply.color = color_white;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.apply );

	s_osp2be.reload.generic.type = MTYPE_PTEXT;
	s_osp2be.reload.generic.flags = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_osp2be.reload.generic.callback = Osp2Be_MenuEvent;
	s_osp2be.reload.generic.id = ID_RELOAD;
	s_osp2be.reload.generic.x = button2X;
		s_osp2be.reload.generic.y = osp2be_layout.design_height - 24;
	s_osp2be.reload.string = "RELOAD";
	s_osp2be.reload.style = UI_CENTER|UI_SMALLFONT;
	s_osp2be.reload.color = color_white;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.reload );

	s_osp2be.close.generic.type = MTYPE_PTEXT;
	s_osp2be.close.generic.flags = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_osp2be.close.generic.callback = Osp2Be_MenuEvent;
	s_osp2be.close.generic.id = ID_CLOSE;
	s_osp2be.close.generic.x = button3X;
		s_osp2be.close.generic.y = osp2be_layout.design_height - 24;
	s_osp2be.close.string = "CLOSE";
	s_osp2be.close.style = UI_CENTER|UI_SMALLFONT;
	s_osp2be.close.color = color_white;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.close );

	baseY = 24 + SMALLCHAR_HEIGHT + 8 + (SMALLCHAR_HEIGHT + 8) + 8;
	rowHeight = osp2be_row.height;
	fieldWidthPixels = osp2be_layout.field_width_chars * SMALLCHAR_WIDTH;
	formRight = s_osp2be.contentLeft + formWidth;
	fieldX = formRight - fieldWidthPixels - osp2be_layout.field_padding + osp2be_layout.color_swatch_size + osp2be_layout.color_swatch_gap;
	sliderOffset = (rowHeight - SMALLCHAR_HEIGHT) / 2;
	if ( sliderOffset < 0 ) {
		sliderOffset = 0;
	}
	sliderOffset += 8;
	s_osp2be.rowHeight = rowHeight;
	s_osp2be.sliderRowOffset = sliderOffset;
		if ( fieldX < s_osp2be.contentLeft + 120 + osp2be_layout.color_swatch_size + osp2be_layout.color_swatch_gap ) {
			fieldX = s_osp2be.contentLeft + 120 + osp2be_layout.color_swatch_size + osp2be_layout.color_swatch_gap;
	}
	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		int rowY;
		int sliderY;

		rowY = baseY + i * rowHeight;
		sliderY = rowY + sliderOffset;
		s_osp2be.defaultRowY[i] = rowY;

		s_osp2be.labels[i].generic.type = MTYPE_TEXT;
		s_osp2be.labels[i].generic.flags = QMF_LEFT_JUSTIFY|QMF_SMALLFONT|QMF_HIDDEN|QMF_INACTIVE;
		s_osp2be.labels[i].generic.x = s_osp2be.contentLeft;
		s_osp2be.labels[i].generic.y = rowY;
		s_osp2be.labels[i].style = UI_LEFT|UI_SMALLFONT;
		s_osp2be.labels[i].color = color_white;
		Menu_AddItem( &s_osp2be.menu, &s_osp2be.labels[i] );

		s_osp2be.fields[i].generic.type = MTYPE_FIELD;
		s_osp2be.fields[i].generic.flags = QMF_SMALLFONT|QMF_LEFT_JUSTIFY|QMF_HIDDEN;
		s_osp2be.fields[i].generic.id = ID_FIELD_BASE + i;
		s_osp2be.fields[i].generic.callback = Osp2Be_FieldEvent;
		s_osp2be.fields[i].generic.x = fieldX;
		s_osp2be.fields[i].generic.y = rowY;
		s_osp2be.fields[i].field.widthInChars = osp2be_layout.field_width_chars;
		s_osp2be.fields[i].field.maxchars = MAX_EDIT_LINE - 1;
		Menu_AddItem( &s_osp2be.menu, &s_osp2be.fields[i] );

		s_osp2be.toggles[i].generic.type = MTYPE_RADIOBUTTON;
		s_osp2be.toggles[i].generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_HIDDEN;
		s_osp2be.toggles[i].generic.callback = Osp2Be_ToggleEvent;
		s_osp2be.toggles[i].generic.id = ID_TOGGLE_BASE + i;
		s_osp2be.toggles[i].generic.x = fieldX;
		s_osp2be.toggles[i].generic.y = rowY;
		s_osp2be.toggles[i].curvalue = 0;
		Menu_AddItem( &s_osp2be.menu, &s_osp2be.toggles[i] );

		s_osp2be.lists[i].generic.type = MTYPE_SPINCONTROL;
		s_osp2be.lists[i].generic.flags = QMF_SMALLFONT|QMF_LEFT_JUSTIFY|QMF_HIDDEN|QMF_PULSEIFFOCUS;
		s_osp2be.lists[i].generic.callback = Osp2Be_ListEvent;
		s_osp2be.lists[i].generic.id = ID_FIELD_BASE + OSP2BE_MAX_CONTROLS + i;
		s_osp2be.lists[i].generic.x = fieldX;
		s_osp2be.lists[i].generic.y = rowY;
		s_osp2be.lists[i].itemnames = NULL;
		s_osp2be.lists[i].numitems = 0;
		s_osp2be.lists[i].curvalue = 0;

		s_osp2be.sliders[i].generic.type = MTYPE_SLIDER;
		s_osp2be.sliders[i].generic.flags = QMF_SMALLFONT|QMF_PULSEIFFOCUS|QMF_HIDDEN;
		s_osp2be.sliders[i].generic.callback = Osp2Be_SliderEvent;
		s_osp2be.sliders[i].generic.id = ID_SLIDER_BASE + i;
		s_osp2be.sliders[i].generic.x = s_osp2be.contentLeft + 220;
		s_osp2be.sliders[i].generic.y = sliderY;
		s_osp2be.sliders[i].generic.ownerdraw = Osp2Be_DrawSlider;
		s_osp2be.sliders[i].minvalue = 0.0f;
		s_osp2be.sliders[i].maxvalue = 1.0f;
		s_osp2be.sliders[i].curvalue = 0.0f;
		s_osp2be.sliders[i].range = 0.0f;
		Menu_AddItem( &s_osp2be.menu, &s_osp2be.sliders[i] );

		s_osp2be.buttons[i].generic.type = MTYPE_BITMAP;
		s_osp2be.buttons[i].generic.flags = QMF_PULSEIFFOCUS|QMF_HIDDEN;
		s_osp2be.buttons[i].generic.callback = Osp2Be_ButtonEvent;
		s_osp2be.buttons[i].generic.ownerdraw = Osp2Be_DrawButton;
		s_osp2be.buttons[i].generic.id = ID_FIELD_BASE + OSP2BE_MAX_CONTROLS * 2 + i;
		s_osp2be.buttons[i].generic.x = fieldX;
		s_osp2be.buttons[i].generic.y = rowY;
		s_osp2be.buttons[i].width = 200;
		s_osp2be.buttons[i].height = osp2be_button.height;
		Menu_AddItem( &s_osp2be.menu, &s_osp2be.buttons[i] );
	}

	s_osp2be.preview.generic.type = MTYPE_BITMAP;
	s_osp2be.preview.generic.flags = QMF_INACTIVE;
	s_osp2be.preview.generic.ownerdraw = Osp2Be_DrawPreview;
	s_osp2be.preview.generic.x = previewX;
	s_osp2be.preview.generic.y = osp2be_layout.preview_y + 64;
	s_osp2be.preview.width = osp2be_layout.preview_width;
	s_osp2be.preview.height = osp2be_layout.preview_height;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.preview );

	s_osp2be.previewFrozenButton.generic.type = MTYPE_BITMAP;
	s_osp2be.previewFrozenButton.generic.flags = QMF_PULSEIFFOCUS;
	s_osp2be.previewFrozenButton.generic.callback = Osp2Be_PreviewFrozenEvent;
	s_osp2be.previewFrozenButton.generic.ownerdraw = Osp2Be_DrawFrozenButton;
	s_osp2be.previewFrozenButton.generic.id = ID_PREVIEW_FROZEN;
	s_osp2be.previewFrozenButton.generic.x = s_osp2be.preview.generic.x + s_osp2be.preview.width - 32;
	s_osp2be.previewFrozenButton.generic.y = s_osp2be.defaultRowY[0] + osp2be_bg_layout.params_top_offset;
	s_osp2be.previewFrozenButton.width = 32;
	s_osp2be.previewFrozenButton.height = 32;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.previewFrozenButton );

	s_osp2be.teamsAllyButton.generic.type = MTYPE_BITMAP;
	s_osp2be.teamsAllyButton.generic.flags = QMF_PULSEIFFOCUS|QMF_HIDDEN;
	s_osp2be.teamsAllyButton.generic.callback = Osp2Be_TeamsPreviewEvent;
	s_osp2be.teamsAllyButton.generic.ownerdraw = Osp2Be_DrawTeamsButton;
	s_osp2be.teamsAllyButton.generic.id = ID_TEAMS_ALLY;
	s_osp2be.teamsAllyButton.generic.x = s_osp2be.preview.generic.x + s_osp2be.preview.width - 32 - 40 - 8;
	s_osp2be.teamsAllyButton.generic.y = s_osp2be.defaultRowY[0] + osp2be_bg_layout.params_top_offset;
	s_osp2be.teamsAllyButton.width = 40;
	s_osp2be.teamsAllyButton.height = 20;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.teamsAllyButton );

	s_osp2be.teamsEnemyButton.generic.type = MTYPE_BITMAP;
	s_osp2be.teamsEnemyButton.generic.flags = QMF_PULSEIFFOCUS|QMF_HIDDEN;
	s_osp2be.teamsEnemyButton.generic.callback = Osp2Be_TeamsPreviewEvent;
	s_osp2be.teamsEnemyButton.generic.ownerdraw = Osp2Be_DrawTeamsButton;
	s_osp2be.teamsEnemyButton.generic.id = ID_TEAMS_ENEMY;
	s_osp2be.teamsEnemyButton.generic.x = s_osp2be.preview.generic.x + s_osp2be.preview.width - 32 - 40 - 8 - 40 - 8;
	s_osp2be.teamsEnemyButton.generic.y = s_osp2be.defaultRowY[0] + osp2be_bg_layout.params_top_offset;
	s_osp2be.teamsEnemyButton.width = 40;
	s_osp2be.teamsEnemyButton.height = 20;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.teamsEnemyButton );

	s_osp2be.status.generic.type = MTYPE_TEXT;
	s_osp2be.status.generic.flags = QMF_LEFT_JUSTIFY|QMF_SMALLFONT|QMF_INACTIVE;
	s_osp2be.status.generic.x = s_osp2be.contentLeft;
	s_osp2be.status.generic.y = osp2be_layout.design_height - 72;
	s_osp2be.status.style = UI_LEFT|UI_SMALLFONT;
	s_osp2be.status.color = color_white;
	s_osp2be.status.string = NULL;
	Menu_AddItem( &s_osp2be.menu, &s_osp2be.status );

	s_osp2be.statusText[0] = '\0';
	s_osp2be.activeCategory = 0;
	Osp2Be_ResetPreview();
	Osp2Be_UpdateControls();

	UI_ColorPicker_Init( &s_osp2be.colorPicker );

	UI_PushMenu( &s_osp2be.menu );
	Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.category );
}

