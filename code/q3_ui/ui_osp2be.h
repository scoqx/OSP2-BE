#ifndef UI_OSP2BE_H
#define UI_OSP2BE_H

#include "ui_local.h"
#include "ui_colorpicker.h"

#define OSP2BE_MAX_CONTROLS 20
#define OSP2BE_CATEGORY_COUNT 7
#define OSP2BE_DLIGHT_WEAPON_COUNT 9

/* Control type definitions */
typedef enum {
	OSP2BE_CONTROL_FIELD,
	OSP2BE_CONTROL_TOGGLE,
	OSP2BE_CONTROL_LIST,
	OSP2BE_CONTROL_BUTTON,
	OSP2BE_CONTROL_COLOR_BUTTON,
	OSP2BE_CONTROL_SIZE_BUTTON,
	OSP2BE_CONTROL_SLIDER
} osp2beControlType_t;

/* Special handling flags */
typedef enum {
	OSP2BE_SPECIAL_NONE = 0,
	OSP2BE_SPECIAL_MODEL_HEAD,
	OSP2BE_SPECIAL_MODEL_TORSO,
	OSP2BE_SPECIAL_MODEL_LEGS,
	OSP2BE_SPECIAL_RAIL_BEAM,
	OSP2BE_SPECIAL_RAIL_RING,
	OSP2BE_SPECIAL_COLOR_GENERIC,
	OSP2BE_SPECIAL_OUTLINE_SIZE,
	OSP2BE_SPECIAL_MODEL_SELECTOR,
	OSP2BE_SPECIAL_MODEL_DEFAULT,
	OSP2BE_SPECIAL_SLIDER_SENSITIVITY,
	OSP2BE_SPECIAL_SLIDER_FOV,
	OSP2BE_SPECIAL_TOGGLE_CHECKBOX,
	OSP2BE_SPECIAL_SECTION_HEADER,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_GAUNTLET,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_MACHINEGUN,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_SHOTGUN,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_GRENADE,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_ROCKET,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_LIGHTNING,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_RAIL,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_PLASMA,
	OSP2BE_SPECIAL_DLIGHT_WEAPON_BFG,
	OSP2BE_SPECIAL_SUPERHUD_CONFIG
} osp2beSpecial_t;

/* Control definition structure */
typedef struct {
	const char *label;
	const char *cvarName;
	osp2beControlType_t controlType;
	osp2beSpecial_t special;
} osp2beCvarDef_t;

/* Category definition structure */
typedef struct {
	const char *title;
	const osp2beCvarDef_t *items;
	int itemCount;
} osp2beCategoryDef_t;

/* Model assembly structure */
typedef struct {
	const char *cvarName;
	char segments[3][MAX_EDIT_LINE];
	qboolean used[3];
} osp2beModelAssembly_t;

/* Rail assembly structure */
typedef struct {
	const char *cvarName;
	char segments[2][MAX_EDIT_LINE];
	qboolean used[2];
} osp2beRailAssembly_t;

/* Named color structure */
typedef struct {
	const char *name;
	float r;
	float g;
	float b;
} osp2beNamedColor_t;

/* Main menu structure */
typedef struct {
	menuframework_s	menu;
	menulist_s		category;
	menutext_s		apply;
	menutext_s		reload;
	menutext_s		close;
	menutext_s		status;

	menutext_s		labels[OSP2BE_MAX_CONTROLS];
	menufield_s		fields[OSP2BE_MAX_CONTROLS];
	menuradiobutton_s toggles[OSP2BE_MAX_CONTROLS];
	menulist_s		lists[OSP2BE_MAX_CONTROLS];
	menubitmap_s	buttons[OSP2BE_MAX_CONTROLS];
	menuslider_s	sliders[OSP2BE_MAX_CONTROLS];
	menubitmap_s	preview;
	menubitmap_s	previewFrozenButton;
	menubitmap_s	teamsAllyButton;
	menubitmap_s	teamsEnemyButton;

	const osp2beCvarDef_t *activeDefs[OSP2BE_MAX_CONTROLS];
	osp2beControlType_t activeTypes[OSP2BE_MAX_CONTROLS];

	int				activeCategory;
	char			statusText[64];
	int				statusTime;
	playerInfo_t	previewInfo;
	char			previewModel[MAX_QPATH];
	int				previewCategory;
	int				colorPickerTargetIndex;
	uiColorPicker_t	colorPicker;
	
	float			screenWidth;
	float			contentWidth;
	int				formWidth;
	int				previewX;
	int				contentLeft;
	int				sidebarLeft;
	qboolean		previewFrozenActive;
	qboolean		teamsPreviewMode; /* qfalse = Ally, qtrue = Enemy */
	char			bufferedPlayerModel[MAX_QPATH];
	char			bufferedPlayerHead[MAX_QPATH];
	char			bufferedAllyModel[MAX_QPATH];
	char			bufferedEnemyModel[MAX_QPATH];
	int				pendingModelCategory;
	int				pendingModelButtonIndex;
	qboolean		bufferedInitialized;
	char			bufferedFieldValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS][MAX_EDIT_LINE];
	int				bufferedToggleValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS];
	int				bufferedListValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS];
	float			bufferedSliderValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS];
	int				bufferedSizeValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS];
	float			appliedSliderValues[OSP2BE_CATEGORY_COUNT][OSP2BE_MAX_CONTROLS];
	int				activeControlCount;
	int				defaultRowY[OSP2BE_MAX_CONTROLS];
	int				rowHeight;
	int				sliderRowOffset;
	qboolean		isIngameContext;
} osp2beMenu_t;

/* Category control arrays - defined in separate files */
extern const osp2beCvarDef_t osp2bePlayerControls[];
extern const int osp2bePlayerControlsCount;

extern const osp2beCvarDef_t osp2beGeneralControls[];
extern const int osp2beGeneralControlsCount;


extern const osp2beCvarDef_t osp2beWeaponControls[];
extern const int osp2beWeaponControlsCount;

extern const osp2beCvarDef_t osp2beRailgunControls[];
extern const int osp2beRailgunControlsCount;

extern const osp2beCvarDef_t osp2beExplosivesControls[];
extern const int osp2beExplosivesControlsCount;

extern const osp2beCvarDef_t osp2beTeamsControls[];
extern const int osp2beTeamsControlsCount;

extern const osp2beCvarDef_t osp2beSuperHUDControls[];
extern const int osp2beSuperHUDControlsCount;

/* Global menu state - defined in ui_osp2be.c */
extern osp2beMenu_t s_osp2be;
extern vec4_t osp2be_sidebar_color;
extern vec4_t osp2be_content_color;
extern qhandle_t osp2beOutlineThin;
extern qhandle_t osp2beOutlineMedium;
extern qhandle_t osp2beOutlineWide;
extern qhandle_t osp2beFrozenOverlay;
extern qhandle_t osp2beFrozenIcon;

/* Utility functions - ui_osp2be_utils.c */
int Osp2Be_StringLength( const char *text );
qboolean Osp2Be_StringContainsChar( const char *text, char ch );
void Osp2Be_SetColorVec4( vec4_t color, float r, float g, float b, float a );
int Osp2Be_HexDigit( char c );
float Osp2Be_ParseHexComponent( const char *ptr );
qboolean Osp2Be_ParseHexColor( const char *value, vec4_t out );
void Osp2Be_OSPColorFromNumber( int number, vec4_t out );
qboolean Osp2Be_ColorFromChar( char code, vec4_t out );
qboolean Osp2Be_ParseColorValue( const char *value, vec4_t out );
qboolean Osp2Be_ParseModelColors( const char *value, vec4_t head, vec4_t torso, vec4_t legs );
qhandle_t Osp2Be_GetOutlineShader( int size );
void Osp2Be_ReadModelSegment( const char *cvarName, int segment, char *buffer, int bufferSize );
void Osp2Be_ReadRailSegment( const char *cvarName, int segment, char *buffer, int bufferSize );
void Osp2Be_WriteModelSegments( const char *cvarName, char segments[3][MAX_EDIT_LINE] );
void Osp2Be_WriteRailSegments( const char *cvarName, char segments[2][MAX_EDIT_LINE] );
int Osp2Be_GetModelSegmentIndex( osp2beSpecial_t special );
int Osp2Be_GetRailSegmentIndex( osp2beSpecial_t special );
int Osp2Be_FindModelAssemblyIndex( osp2beModelAssembly_t *assemblies, int count, const char *cvarName );
int Osp2Be_FindRailAssemblyIndex( osp2beRailAssembly_t *assemblies, int count, const char *cvarName );

/* Control functions - ui_osp2be_controls.c */
void Osp2Be_ReadFieldValue( const osp2beCvarDef_t *def, char *buffer, int bufferSize );
void Osp2Be_ClampFieldLength( menufield_s *field );
void Osp2Be_WriteFieldValue( const osp2beCvarDef_t *def, const char *value, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_SetupFieldControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupToggleControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupListControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupButtonControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupColorButtonControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupSizeButtonControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_SetupSliderControl( int index, const osp2beCvarDef_t *def );
void Osp2Be_ApplyFieldControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplyToggleControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplyListControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplyButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplyColorButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplySizeButtonControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_ApplySliderControl( int index, const osp2beCvarDef_t *def, osp2beModelAssembly_t *modelAssemblies, int *modelAssemblyCount, osp2beRailAssembly_t *railAssemblies, int *railAssemblyCount );
void Osp2Be_SliderSetValue( int index, const osp2beCvarDef_t *def, float value );
float Osp2Be_SliderClampValue( const osp2beCvarDef_t *def, float value );
int Osp2Be_SliderGetPrecision( const osp2beCvarDef_t *def );
void Osp2Be_SliderRefreshField( int index, const osp2beCvarDef_t *def );
float Osp2Be_ParseSliderInput( const char *text, float fallback );
qboolean Osp2Be_CanUseColorPicker( const osp2beCvarDef_t *def );

/* Preview functions - ui_osp2be_preview.c */
void Osp2Be_ResetPreview( void );
const char *Osp2Be_GetModelCvarForCategory( int category );
void Osp2Be_GetModelColorsFromFields( char *buffer, int bufferSize );
qboolean Osp2Be_GetOutlineColorFromFields( char *buffer, int bufferSize );
int Osp2Be_GetOutlineSizeFromFields( void );
void Osp2Be_UpdatePreviewModel( void );

/* Draw functions - ui_osp2be_draw.c */
void Osp2Be_DrawBackground( void );
void Osp2Be_DrawStatus( void );
void Osp2Be_DrawPreview( void *self );
void Osp2Be_DrawButton( void *self );
void Osp2Be_DrawTeamsButton( void *self );
void Osp2Be_DrawFrozenButton( void *self );
void Osp2Be_DrawSlider( void *self );
void Osp2Be_DrawToggleButton( void *self );
void Osp2Be_DrawColorSwatches( void );
void Osp2Be_MenuDraw( void );
void Osp2Be_DrawCategoryList( void *self );
void Osp2Be_InitWeaponIcons( void );

/* Main functions - ui_osp2be.c */
void Osp2Be_SelectCategory( int index );
qboolean Osp2Be_GetBufferedNumericValue( const char *cvarName, float *outValue );

/* Model buffer functions - ui_osp2be_utils.c */
void Osp2Be_LoadBufferedModelsFromCvars( void );
char *Osp2Be_GetModelBufferForCategory( int category );
const char *Osp2Be_GetBufferedModelForCategory( int category );

#endif /* UI_OSP2BE_H */

