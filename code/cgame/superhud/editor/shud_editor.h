#ifndef SHUD_EDITOR_H
#define SHUD_EDITOR_H

#include "cg_local.h"
#include "superhud_private.h"
#include "superhud.h"
#include "../../../ui/keycodes.h"


#define SHUD_EDIT_PANEL_X     4.0f
#define SHUD_EDIT_PANEL_Y     4.0f
#define SHUD_EDIT_PANEL_W     148.0f
#define SHUD_EDIT_PANEL_H     472.0f
#define SHUD_EDIT_ROW_H       10.0f
#define SHUD_EDIT_HANDLE      6.0f
#define SHUD_EDIT_MIN_SIZE    4.0f
#define SHUD_EDIT_MAX_ITEMS   256
#define SHUD_EDIT_ROW_MAX     280
#define SHUD_EDIT_SEARCH_Y    40.0f
#define SHUD_EDIT_SEARCH_H    14.0f
#define SHUD_EDIT_TAB_Y       16.0f
#define SHUD_EDIT_TAB_H       22.0f
#define SHUD_EDIT_LIST_Y      56.0f
#define SHUD_EDIT_LIST_BOTTOM 418.0f
#define SHUD_EDIT_PROPS_W     184.0f
#define SHUD_EDIT_PROPS_X     (SCREEN_WIDTH - SHUD_EDIT_PROPS_W - 4.0f)
#define SHUD_EDIT_PROPS_Y     4.0f
#define SHUD_EDIT_PROPS_H     472.0f
#define SHUD_EDIT_PROP_ROW    10.0f
#define SHUD_EDIT_PROP_LIST_Y 28.0f
#define SHUD_EDIT_PROP_BOTTOM 430.0f
#define SHUD_EDIT_PROP_VAL_X  (SHUD_EDIT_PROPS_X + 58.0f)
#define SHUD_EDIT_PROP_VAL_W  (SHUD_EDIT_PROPS_W - 70.0f)
#define SHUD_EDIT_SLIDER_TRACK_X (SHUD_EDIT_PROPS_X + 24.0f)
#define SHUD_EDIT_SLIDER_TRACK_W (SHUD_EDIT_PROPS_W - 72.0f)
#define SHUD_EDIT_SLIDER_NUM_X  (SHUD_EDIT_SLIDER_TRACK_X + SHUD_EDIT_SLIDER_TRACK_W + 4.0f)
#define SHUD_EDIT_SLIDER_NUM_W  28.0f
#define SHUD_EDIT_SLIDER_H    10.0f
#define SHUD_EDIT_GRID_SIZE   16.0f
#define SHUD_EDIT_UNDO_MAX    12
#define SHUD_EDIT_LAY_MAX     128
#define SHUD_EDIT_IMG_MAX     12
#define SHUD_EDIT_TXT_MAX     6
#define SHUD_EDIT_SNAP_PIX    4.0f
#define SHUD_EDIT_TYPE_MAX    64
#define SHUD_EDIT_GHOST_MAX   32
#define SHUD_EDIT_SEL_MAX     256

typedef enum
{
	SHUD_DRAG_NONE,
	SHUD_DRAG_MOVE,
	SHUD_DRAG_RESIZE_TL,
	SHUD_DRAG_RESIZE_TR,
	SHUD_DRAG_RESIZE_BL,
	SHUD_DRAG_RESIZE_BR,
	SHUD_DRAG_SLIDER,
	SHUD_DRAG_MARQUEE,
	SHUD_DRAG_LIST_MARQUEE
} shudEditorDrag_t;

typedef enum
{
	SHUD_PROP_RECT_X = 0,
	SHUD_PROP_RECT_Y,
	SHUD_PROP_RECT_W,
	SHUD_PROP_RECT_H,
	SHUD_PROP_FONT,
	SHUD_PROP_FS_W,
	SHUD_PROP_FS_H,
	SHUD_PROP_ALIGNH,
	SHUD_PROP_ALIGNV,
	SHUD_PROP_TEXTALIGN,
	SHUD_PROP_DIRECTION,
	SHUD_PROP_COLOR,
	SHUD_PROP_BGCOLOR,
	SHUD_PROP_COLOR2,
	SHUD_PROP_BGCOLOR2,
	SHUD_PROP_IMAGE,
	SHUD_PROP_IMAGEALIGNH,
	SHUD_PROP_IMAGEALIGNV,
	SHUD_PROP_TEXT,
	SHUD_PROP_TEXTOFF_X,
	SHUD_PROP_TEXTOFF_Y,
	SHUD_PROP_ITTEAM,
	SHUD_PROP_TIME,
	SHUD_PROP_FADEDELAY,
	SHUD_PROP_BORDER_L,
	SHUD_PROP_BORDER_T,
	SHUD_PROP_BORDER_R,
	SHUD_PROP_BORDER_B,
	SHUD_PROP_BORDERC,
	SHUD_PROP_BORDERC2,
	SHUD_PROP_HLSIZE,
	SHUD_PROP_FADE,
	SHUD_PROP_SHADOW,
	SHUD_PROP_HLCOLOR,
	SHUD_PROP_IMAGETC_S,
	SHUD_PROP_IMAGETC_T,
	SHUD_PROP_IMAGETC_U,
	SHUD_PROP_IMAGETC_V,
	SHUD_PROP_MARGIN_L,
	SHUD_PROP_MARGIN_T,
	SHUD_PROP_MARGIN_R,
	SHUD_PROP_MARGIN_B,
	SHUD_PROP_STYLE,
	SHUD_PROP_TEXTSTYLE,
	SHUD_PROP_FILL,
	SHUD_PROP_MONO,
	SHUD_PROP_DOUBLEBAR,
	SHUD_PROP_VIS_IM,
	SHUD_PROP_VIS_SPECT,
	SHUD_PROP_VIS_DEAD,
	SHUD_PROP_VIS_SCOREHIDE,
	SHUD_PROP_VIS_SHOWEMPTY,
	SHUD_PROP_VIS_TEAMONLY,
	SHUD_PROP_VIS_DEMOHIDE,
	SHUD_PROP_VIS_KEY1,
	SHUD_PROP_VIS_KEY2,
	SHUD_PROP_VIS_KEY3,
	SHUD_PROP_VIS_KEY4,
	SHUD_PROP_VIS_GTFFA,
	SHUD_PROP_VIS_GTTOURNEY,
	SHUD_PROP_VIS_GTTDM,
	SHUD_PROP_VIS_GTCTF,
	SHUD_PROP_VIS_GTFT,
	SHUD_PROP_VIS_GTCA,
	SHUD_PROP_COUNT
} shudProp_t;

typedef enum
{
	SHUD_UNDO_NONE,
	SHUD_UNDO_CONFIG,
	SHUD_UNDO_INSERT,
	SHUD_UNDO_DELETE
} shudUndoType_t;

typedef struct
{
	shudUndoType_t type;
	superhudElement_t* el;
	superhudConfig_t cfg;
	superHUDConfigElement_t def;
	int chainIndex;
	int tab;
	int grp;
	int batch;
} shudUndoEntry_t;

typedef struct
{
	int kind;
	int prop;
	int channel;
	float y;
} shudLay_t;

typedef struct
{
	qboolean header;
	superhudElement_t* el;
	const superHUDConfigElement_t* def;
	const char* title;
} shudListRow_t;

#define SHUD_PF_RECT   (1 << 0)
#define SHUD_PF_FONT   (1 << 1)
#define SHUD_PF_ALIGN  (1 << 2)
#define SHUD_PF_TEXT   (1 << 3)
#define SHUD_PF_COLOR  (1 << 4)
#define SHUD_PF_BG     (1 << 5)
#define SHUD_PF_IMAGE  (1 << 6)
#define SHUD_PF_BAR    (1 << 7)
#define SHUD_PF_STYLE  (1 << 8)
#define SHUD_PF_TIME   (1 << 9)
#define SHUD_PF_VIS    (1 << 10)

typedef struct
{
	qboolean active;
	qboolean dirty;
	qboolean showList;
	qboolean showProps;
	qboolean mouse1;
	qboolean picker;
	qboolean undoLock;
	qboolean typing;
	qboolean help;
	qboolean pipette;
	qboolean showAll;
	qboolean hideOtherBoxes;
	qboolean showGrid;
	qboolean showGuides;
	qboolean snapGrid;
	qboolean filterOn;
	qboolean marqueeAdd;
	int tab;
	int undoGroup;
	int undoGroupSeq;
	int selCount;
	int deleteArmed;
	int gridSize;
	char listFilter[64];
	int propScroll;
	int coalesceProp;
	int redoCount;
	int ghostCount;
	int typeProp;
	int typeChannel;
	char typeBuf[SHUD_EDIT_TYPE_MAX];
	char pickerFilter[64];
	char clipName[MAX_QPATH];
	superhudConfig_t clipCfg;
	superHUDConfigElement_t clipDef;
	qboolean clipValid;
	superhudElement_t* ghosts[SHUD_EDIT_GHOST_MAX];
	float cursorX;
	float cursorY;
	int scroll;
	int pickerScroll;
	int pickerIndex;
	int selectedIndex;
	int propIndex;
	int sliderChannel;
	int sliderProp;
	char selectedName[MAX_QPATH];
	superhudElement_t* selected;
	superhudElement_t* sel[SHUD_EDIT_SEL_MAX];
	shudEditorDrag_t drag;
	float dragGrabX;
	float dragGrabY;
	vec4_t dragStartRect;
	vec4_t dragStartRects[SHUD_EDIT_SEL_MAX];
	int undoCount;
} shudEditorState_t;

extern shudEditorState_t editor;
extern shudUndoEntry_t editorUndo[SHUD_EDIT_UNDO_MAX];
extern shudUndoEntry_t editorRedo[SHUD_EDIT_UNDO_MAX];

extern vec4_t shudEditPanelBg;
extern vec4_t shudEditPanelBorder;
extern vec4_t shudEditRowSel;
extern vec4_t shudEditOutline;
extern vec4_t shudEditOutlineHidden;
extern vec4_t shudEditSelected;
extern vec4_t shudEditHover;
extern vec4_t shudEditHandle;
extern vec4_t shudEditGrid;
extern vec4_t shudEditCursor;
extern vec4_t shudEditSliderBg;
extern vec4_t shudEditSliderR;
extern vec4_t shudEditSliderG;
extern vec4_t shudEditSliderB;
extern vec4_t shudEditSliderA;
extern vec4_t shudEditMarqueeFill;
extern vec4_t shudEditMarqueeLine;
extern vec4_t shudEditSelMulti;
extern const char* shudEditTabShort[SHUD_TAB_COUNT];
extern const char* shudEditImages[SHUD_EDIT_IMG_MAX];
extern const char* shudEditTexts[SHUD_EDIT_TXT_MAX];

qboolean CG_SHUDEditorIsDefault(const superhudElement_t* el);
qboolean CG_SHUDEditorIsEditable(const superhudElement_t* el);
superhudElement_t* CG_SHUDEditorFindTabDefault(int tab);
superhudElement_t* CG_SHUDEditorEnsureTabDefault(int tab);
qboolean CG_SHUDEditorGetRect(const superhudElement_t* el, vec4_t out);
int CG_SHUDEditorNameGroup(const char* name);
const char* CG_SHUDEditorGroupTitle(int group);
qboolean CG_SHUDEditorNameMatches(const char* name, const char* filter);
unsigned CG_SHUDEditorNameFlags(const char* name);
qboolean CG_SHUDEditorPropVisible(const superhudElement_t* el, int prop);
void CG_SHUDEditorFixPropIndex(void);
int CG_SHUDEditorCollect(superhudElement_t** out, int max);
int CG_SHUDEditorCmpName(const char* a, const char* b);
void CG_SHUDEditorSortEls(superhudElement_t** items, int n);
int CG_SHUDEditorBuildList(shudListRow_t* rows, int max);
superhudElement_t* CG_SHUDEditorFindByName(const char* name);
int CG_SHUDEditorIndexOf(superhudElement_t* el);
int CG_SHUDEditorVisibleRows(void);
void CG_SHUDEditorEnsureVisible(void);
void CG_SHUDEditorBeginUndoGroup(void);
void CG_SHUDEditorEndUndoGroup(void);
qboolean CG_SHUDEditorSelHas(const superhudElement_t* el);
void CG_SHUDEditorSelAdd(superhudElement_t* el);
void CG_SHUDEditorSelRemove(superhudElement_t* el);
void CG_SHUDEditorSetPrimary(superhudElement_t* el);
void CG_SHUDEditorSelect(superhudElement_t* el);
void CG_SHUDEditorSelectIndex(int index);
void CG_SHUDEditorClampRect(vec4_t rect);
float CG_SHUDEditorSnapVal(float v);
float CG_SHUDEditorSnapToList(float v, const float* list, int n, float pix);
void CG_SHUDEditorSnapNeighbors(vec4_t tmp);
void CG_SHUDEditorApplyRect(superhudElement_t* el, const vec4_t rect, qboolean round, qboolean snapNeighbors);
qboolean CG_SHUDEditorPointInRect(float x, float y, const vec4_t rect);
qboolean CG_SHUDEditorPointInHandle(float x, float y, float hx, float hy);
shudEditorDrag_t CG_SHUDEditorHitHandle(float x, float y, const vec4_t rect);
qboolean CG_SHUDEditorHideBox(const superhudElement_t* el);
superhudElement_t* CG_SHUDEditorHitElement(float x, float y);
qboolean CG_SHUDEditorCursorInPanel(void);
qboolean CG_SHUDEditorCursorInSearch(void);
qboolean CG_SHUDEditorCursorInList(void);
qboolean CG_SHUDEditorCursorInProps(void);
void CG_SHUDEditorCommit(int prop);
qboolean CG_SHUDEditorElementAlive(superhudElement_t* el);
void CG_SHUDEditorClearUndo(void);
void CG_SHUDEditorPushUndo(shudUndoType_t type, superhudElement_t* el);
void CG_SHUDEditorPushConfigUndo(superhudElement_t* el);
void CG_SHUDEditorPushSelConfigUndo(int prop);
void CG_SHUDEditorUndo(void);
int CG_SHUDEditorCycle(int value, int dir, int count);
void CG_SHUDEditorClamp01(float* v);
int CG_SHUDEditorCurrentVis(const superhudElement_t* el);
void CG_SHUDEditorToggleVis(superhudElement_t* el, int bit);
void CG_SHUDEditorEnsureRect(superhudElement_t* el);
void CG_SHUDEditorEnsureFontSize(superhudElement_t* el);
qboolean CG_SHUDEditorPropIsColor(int prop);
qboolean CG_SHUDEditorPropIsVec4(int prop);
superhudColor_t* CG_SHUDEditorColorPtr(superhudElement_t* el, int prop);
qboolean* CG_SHUDEditorColorSetPtr(superhudElement_t* el, int prop);
float* CG_SHUDEditorVec4Ptr(superhudElement_t* el, int prop);
qboolean* CG_SHUDEditorVec4SetPtr(superhudElement_t* el, int prop);
float* CG_SHUDEditorRgbaPtr(superhudElement_t* el, int prop);
void CG_SHUDEditorEnsureColorProp(superhudElement_t* el, int prop);
void CG_SHUDEditorCycleColorType(superhudElement_t* el, int prop, int dir);
qboolean CG_SHUDEditorColorHasSliders(const superhudElement_t* el, int prop);
qboolean CG_SHUDEditorMatchFilter(const char* name, const char* filter);
qboolean CG_SHUDEditorIsGhost(const superhudElement_t* el);
void CG_SHUDEditorToggleGhost(superhudElement_t* el);
int CG_SHUDEditorVisBit(int prop);
void CG_SHUDEditorClearRedo(void);
void CG_SHUDEditorPushRedo(shudUndoEntry_t* src);
void CG_SHUDEditorRedo(void);
void CG_SHUDEditorTypicalSize(const char* name, vec4_t rect);
void CG_SHUDEditorAlignScreen(int mode);
void CG_SHUDEditorMirrorH(void);
void CG_SHUDEditorCopy(void);
void CG_SHUDEditorPaste(void);
void CG_SHUDEditorLayer(int dir);
void CG_SHUDEditorStartTypeIn(int prop, int channel);
void CG_SHUDEditorCancelTypeIn(void);
void CG_SHUDEditorSyncFilter(void);
void CG_SHUDEditorCommitTypeIn(void);
void CG_SHUDEditorTypeKey(int key);
void CG_SHUDEditorDrawHelp(void);
void CG_SHUDEditorDrawGuides(void);
int CG_SHUDEditorPropVecIndex(int prop);
int CG_SHUDEditorPropCat(int prop);
const char* CG_SHUDEditorPropCatTitle(int cat);
int CG_SHUDEditorLayout(shudLay_t* out, int max);
int CG_SHUDEditorHitLayout(shudLay_t* item);
qboolean CG_SHUDEditorPropIsTypeable(int prop);
int CG_SHUDEditorHitPropsClick(shudLay_t* item, int* clickKind);
void CG_SHUDEditorSetSlider(int prop, int channel, float t);
void CG_SHUDEditorSliderFromCursor(int prop, int channel);
qboolean CG_SHUDEditorAllowMulti(const char* name);
int CG_SHUDEditorCollectTypes(const superHUDConfigElement_t** out, int max);
int CG_SHUDEditorBuildTypeList(shudListRow_t* rows, int max);
superhudElement_t* CG_SHUDEditorFindDefault(void);
void CG_SHUDEditorAddType(const superHUDConfigElement_t* def);
void CG_SHUDEditorDuplicate(void);
void CG_SHUDEditorDeleteSelected(void);
void CG_SHUDEditorCopyProp(superhudElement_t* dst, superhudElement_t* src, int prop);
void CG_SHUDEditorUnsetProp(int prop);
void CG_SHUDEditorAdjustProp(int prop, int dir);
qboolean CG_SHUDEditorPropIsToggle(int prop);
void CG_SHUDEditorClickProps(void);
qboolean CG_SHUDEditorIsProtectedName(const char* name);
qboolean CG_SHUDEditorDoSave(const char* name, qboolean allowDefault);
void CG_SHUDEditorEnsureObituaryPreview(superhudGlobalContext_t* ctx);
void CG_SHUDEditorEnsureTeamTaskPreview(superhudGlobalContext_t* ctx);
int CG_SHUDEditorPropScrollMax(void);
void CG_SHUDEditorScrollProps(int delta);
void CG_SHUDEditorWheelScroll(int dir, int count, int vis);
void CG_SHUDEditorEnsureTeamOverlayPreview(void);
void CG_SHUDEditorRestoreTeamOverlayPreview(void);
void CG_SHUDEditorCatchKeys(qboolean enable);
void CG_SHUDEditorPreviewStop(void);
void CG_SHUDEditorPreviewStart(void);
void CG_SHUDEditorPumpPreview(void);
void CG_SHUDEditorActivate(qboolean enable);
qboolean CG_SHUDEditorIsActive(void);
qboolean CG_SHUDEditorShowAll(void);
void CG_SHUDEditorToggle_f(void);
void CG_SHUDEditorSave_f(void);
void CG_SHUDEditorShutdown(void);
void CG_SHUDEditorOnConfigReloaded(void);
void CG_SHUDEditorNudge(float dx, float dy, qboolean resize);
void CG_SHUDEditorSetTab(int tab);
void CG_SHUDEditorClickPanel(void);
qboolean CG_SHUDEditorRectsOverlap(const vec4_t a, const vec4_t b);
void CG_SHUDEditorMarqueeRect(vec4_t out);
void CG_SHUDEditorFinishMarquee(void);
void CG_SHUDEditorFinishListMarquee(void);
void CG_SHUDEditorStartGroupMove(void);
void CG_SHUDEditorMouseDown(void);
void CG_SHUDEditorMouseUp(void);
void CG_SHUDEditorApplyDrag(void);
void CG_SHUDEditorMouseEvent(int dx, int dy);
void CG_SHUDEditorKeyEvent(int key, qboolean down);
void CG_SHUDEditorDrawHandle(float x, float y);
void CG_SHUDEditorDrawElementBox(superhudElement_t* el, qboolean selected, qboolean hover);
void CG_SHUDEditorDrawGrid(void);
void CG_SHUDEditorDrawCursor(void);
void CG_SHUDEditorPropValue(const superhudElement_t* el, int prop, char* out, int outSize);
void CG_SHUDEditorDrawProps(void);
void CG_SHUDEditorDrawSearch(void);
void CG_SHUDEditorDrawPanel(void);
void CG_SHUDEditorDraw(void);

#endif
