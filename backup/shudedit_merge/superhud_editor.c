#include "cg_local.h"
#include "superhud_private.h"
#include "superhud.h"
#include "../../ui/keycodes.h"

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
#define SHUD_EDIT_SLIDER_H    10.0f
#define SHUD_EDIT_GRID_SIZE   16.0f
#define SHUD_EDIT_UNDO_MAX    96
#define SHUD_EDIT_LAY_MAX     128
#define SHUD_EDIT_IMG_MAX     12
#define SHUD_EDIT_TXT_MAX     6
#define SHUD_EDIT_SNAP_PIX    4.0f
#define SHUD_EDIT_DRAG_MIN    3.0f
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
	SHUD_DRAG_MARQUEE
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
	qboolean dragMoved;
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
	char typeBuf[SHUD_EDIT_TYPE_MAX];
	char pickerFilter[64];
	char clipName[MAX_QPATH];
	superhudConfig_t clipCfg;
	superHUDConfigElement_t clipDef;
	qboolean clipValid;
	superhudElement_t* ghosts[SHUD_EDIT_GHOST_MAX];
	shudUndoEntry_t redo[SHUD_EDIT_UNDO_MAX];
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
	shudUndoEntry_t undo[SHUD_EDIT_UNDO_MAX];
} shudEditorState_t;

static shudEditorState_t editor;
static qboolean shudEditPreviewLive;
static int shudEditSavedAmmo;
static qboolean shudEditSavedForceChat;

static qboolean CG_SHUDEditorMatchFilter(const char* name, const char* filter);
static void CG_SHUDEditorPushRedo(shudUndoEntry_t* src);
static void CG_SHUDEditorRedo(void);
static qboolean CG_SHUDEditorIsGhost(const superhudElement_t* el);
static void CG_SHUDEditorToggleGhost(superhudElement_t* el);
static int CG_SHUDEditorVisBit(int prop);
static void CG_SHUDEditorTypicalSize(const char* name, vec4_t rect);
static void CG_SHUDEditorAlignScreen(int mode);
static void CG_SHUDEditorMirrorH(void);
static void CG_SHUDEditorCopy(void);
static void CG_SHUDEditorPaste(void);
static void CG_SHUDEditorLayer(int dir);
static void CG_SHUDEditorStartTypeIn(int prop);
static void CG_SHUDEditorCancelTypeIn(void);
static void CG_SHUDEditorTypeKey(int key);
static void CG_SHUDEditorDrawHelp(void);
static void CG_SHUDEditorDrawGuides(void);
static int CG_SHUDEditorPropVecIndex(int prop);
static qboolean CG_SHUDEditorAllowMulti(const char* name);
static qboolean CG_SHUDEditorElementAlive(superhudElement_t* el);
static void CG_SHUDEditorCopyProp(superhudElement_t* dst, superhudElement_t* src, int prop);

static vec4_t shudEditPanelBg = { 0.02f, 0.02f, 0.04f, 0.82f };
static vec4_t shudEditPanelBorder = { 0.35f, 0.75f, 1.0f, 0.85f };
static vec4_t shudEditRowSel = { 0.15f, 0.40f, 0.70f, 0.75f };
static vec4_t shudEditOutline = { 0.30f, 0.80f, 1.00f, 0.45f };
static vec4_t shudEditOutlineHidden = { 0.55f, 0.55f, 0.55f, 0.30f };
static vec4_t shudEditSelected = { 1.00f, 0.85f, 0.15f, 0.95f };
static vec4_t shudEditHover = { 1.00f, 1.00f, 1.00f, 0.80f };
static vec4_t shudEditHandle = { 1.00f, 0.85f, 0.15f, 1.00f };
static vec4_t shudEditGrid = { 1.00f, 1.00f, 1.00f, 0.07f };
static vec4_t shudEditCursor = { 1.00f, 0.20f, 0.20f, 0.95f };
static vec4_t shudEditSliderBg = { 0.12f, 0.12f, 0.16f, 0.95f };
static vec4_t shudEditSliderR = { 0.85f, 0.20f, 0.20f, 1.00f };
static vec4_t shudEditSliderG = { 0.20f, 0.75f, 0.25f, 1.00f };
static vec4_t shudEditSliderB = { 0.25f, 0.45f, 0.95f, 1.00f };
static vec4_t shudEditSliderA = { 0.85f, 0.85f, 0.85f, 1.00f };
static vec4_t shudEditMarqueeFill = { 0.20f, 0.55f, 1.00f, 0.14f };
static vec4_t shudEditMarqueeLine = { 0.35f, 0.80f, 1.00f, 0.90f };
static vec4_t shudEditSelMulti = { 1.00f, 0.70f, 0.20f, 0.85f };
static const char* shudEditTabShort[SHUD_TAB_COUNT] =
{
	"STAT", "CHAT", "CMBT", "SCOR", "TEAM", "WEAP", "PLYR", "MISC"
};

static const char* shudEditImages[SHUD_EDIT_IMG_MAX] =
{
	"white",
	"hud/lowhp",
	"icons/iconh_yellow",
	"icons/iconr_white",
	"icons/icona_red",
	"icons/icona_yellow",
	"icons/icona_green",
	"gfx/2d/grad_right",
	"gfx/2d/grad_left",
	"menu/art/3_cursor2",
	"gfx/2d/numbers/zero_32b",
	"gfx/2d/backtile"
};

static const char* shudEditTexts[SHUD_EDIT_TXT_MAX] =
{
	"*",
	"+",
	"HP",
	"ARM",
	"AMMO",
	"..."
};

static qboolean CG_SHUDEditorIsDefault(const superhudElement_t* el)
{
	return (el && CG_SHUDIsDefaultName(el->element.name)) ? qtrue : qfalse;
}

static qboolean CG_SHUDEditorIsEditable(const superhudElement_t* el)
{
	if (!el || !el->element.name)
	{
		return qfalse;
	}
	if (CG_SHUDEditorIsDefault(el))
	{
		return qtrue;
	}
	if (!el->element.create)
	{
		return qfalse;
	}
	return qtrue;
}

static superhudElement_t* CG_SHUDEditorFindTabDefault(int tab)
{
	superhudElement_t* el;

	el = CG_SHUDGetElementsHead();
	while (el)
	{
		if (CG_SHUDEditorIsDefault(el) && el->tab == tab)
		{
			return el;
		}
		el = el->next;
	}
	return NULL;
}

static superhudElement_t* CG_SHUDEditorEnsureTabDefault(int tab)
{
	superhudElement_t* el;
	const superHUDConfigElement_t* def;

	el = CG_SHUDEditorFindTabDefault(tab);
	if (el)
	{
		return el;
	}
	def = CG_SHUDFindConfigElementItem("!default");
	if (!def)
	{
		return NULL;
	}
	el = CG_SHUDAllocElement(def, NULL);
	if (!el)
	{
		return NULL;
	}
	el->tab = tab;
	CG_SHUDLinkElementAfter(NULL, el);
	return el;
}

static qboolean CG_SHUDEditorGetRect(const superhudElement_t* el, vec4_t out)
{
	if (!el || !el->config.rect.isSet)
	{
		return qfalse;
	}
	Vector4Copy(el->config.rect.value, out);
	if (out[2] < 1.0f)
	{
		out[2] = 1.0f;
	}
	if (out[3] < 1.0f)
	{
		out[3] = 1.0f;
	}
	if (el->config.alignH.isSet)
	{
		if (el->config.alignH.value == SUPERHUD_ALIGNH_CENTER)
		{
			out[0] -= out[2] * 0.5f;
		}
		else if (el->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
		{
			out[0] -= out[2];
		}
	}
	if (el->config.alignV.isSet)
	{
		if (el->config.alignV.value == SUPERHUD_ALIGNV_CENTER)
		{
			out[1] -= out[3] * 0.5f;
		}
		else if (el->config.alignV.value == SUPERHUD_ALIGNV_BOTTOM)
		{
			out[1] -= out[3];
		}
	}
	return qtrue;
}

static int CG_SHUDEditorNameGroup(const char* name)
{
	int t;

	t = CG_SHUDNameTab(name);
	if (t < 0)
	{
		return editor.tab;
	}
	return t;
}

static const char* CG_SHUDEditorGroupTitle(int group)
{
	return CG_SHUDTabTitle(group);
}

static qboolean CG_SHUDEditorNameMatches(const char* name, const char* filter)
{
	if (!filter || !filter[0])
	{
		return qtrue;
	}
	if (CG_SHUDEditorMatchFilter(name, filter))
	{
		return qtrue;
	}
	return CG_SHUDEditorMatchFilter(CG_SHUDEditorGroupTitle(CG_SHUDEditorNameGroup(name)), filter);
}

static unsigned CG_SHUDEditorNameFlags(const char* name)
{
	unsigned flags;

	flags = SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_VIS | SHUD_PF_TIME;
	if (!name || !name[0])
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_STYLE;
	}
	if (CG_SHUDIsDefaultName(name))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	if (!Q_stricmp(name, "grid"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_VIS;
	}
	if (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate"))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	if (CG_SHUDEditorMatchFilter(name, "statusbar_") && CG_SHUDEditorMatchFilter(name, "bar"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_BAR | SHUD_PF_STYLE | SHUD_PF_VIS;
	}
	if (CG_SHUDEditorMatchFilter(name, "icon") ||
	    CG_SHUDEditorMatchFilter(name, "flagstatus") ||
	    CG_SHUDEditorMatchFilter(name, "itempickupicon") ||
	    CG_SHUDEditorMatchFilter(name, "rewardicons"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_IMAGE | SHUD_PF_TIME | SHUD_PF_VIS;
	}
	if (CG_SHUDEditorMatchFilter(name, "obituar") ||
	    !Q_stricmp(name, "netgraph") ||
	    !Q_stricmp(name, "weaponlist") ||
	    (name[0] == 't' && name[1] == 'e' && name[2] == 'a' && name[3] == 'm' &&
	     name[4] >= '1' && name[4] <= '9'))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_STYLE;
}

static qboolean CG_SHUDEditorPropVisible(const superhudElement_t* el, int prop)
{
	unsigned flags;

	if (!el)
	{
		return qfalse;
	}
	flags = CG_SHUDEditorNameFlags(el->element.name);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			return (flags & SHUD_PF_RECT) ? qtrue : qfalse;
		case SHUD_PROP_FONT:
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			return (flags & SHUD_PF_FONT) ? qtrue : qfalse;
		case SHUD_PROP_ALIGNH:
		case SHUD_PROP_ALIGNV:
		case SHUD_PROP_TEXTALIGN:
			return (flags & SHUD_PF_ALIGN) ? qtrue : qfalse;
		case SHUD_PROP_TEXT:
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
		case SHUD_PROP_TEXTSTYLE:
		case SHUD_PROP_MONO:
		case SHUD_PROP_SHADOW:
			return (flags & SHUD_PF_TEXT) ? qtrue : qfalse;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_COLOR2:
			return (flags & SHUD_PF_COLOR) ? qtrue : qfalse;
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_FILL:
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			return (flags & SHUD_PF_BG) ? qtrue : qfalse;
		case SHUD_PROP_IMAGE:
			return (flags & SHUD_PF_IMAGE) ? qtrue : qfalse;
		case SHUD_PROP_DIRECTION:
		case SHUD_PROP_DOUBLEBAR:
			return (flags & SHUD_PF_BAR) ? qtrue : qfalse;
		case SHUD_PROP_STYLE:
			return (flags & SHUD_PF_STYLE) ? qtrue : qfalse;
		case SHUD_PROP_TIME:
		case SHUD_PROP_FADE:
		case SHUD_PROP_FADEDELAY:
			return (flags & SHUD_PF_TIME) ? qtrue : qfalse;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			return (flags & SHUD_PF_VIS) ? qtrue : qfalse;
		default:
			return qfalse;
	}
}

static void CG_SHUDEditorFixPropIndex(void)
{
	int p;

	if (!editor.selected)
	{
		editor.propIndex = 0;
		return;
	}
	if (CG_SHUDEditorPropVisible(editor.selected, editor.propIndex))
	{
		return;
	}
	for (p = 0; p < SHUD_PROP_COUNT; p++)
	{
		if (CG_SHUDEditorPropVisible(editor.selected, p))
		{
			editor.propIndex = p;
			return;
		}
	}
	editor.propIndex = 0;
}

static int CG_SHUDEditorCollect(superhudElement_t** out, int max)
{
	superhudElement_t* el;
	int count;

	count = 0;
	el = CG_SHUDGetElementsHead();
	while (el && count < max)
	{
		if (CG_SHUDEditorIsEditable(el) && !CG_SHUDEditorIsDefault(el))
		{
			if (out)
			{
				out[count] = el;
			}
			count++;
		}
		el = el->next;
	}
	return count;
}

static int CG_SHUDEditorCmpName(const char* a, const char* b)
{
	int ga;
	int gb;
	int c;

	ga = CG_SHUDEditorNameGroup(a);
	gb = CG_SHUDEditorNameGroup(b);
	if (ga != gb)
	{
		return ga - gb;
	}
	c = Q_stricmp(a ? a : "", b ? b : "");
	return c;
}

static void CG_SHUDEditorSortEls(superhudElement_t** items, int n)
{
	int i;
	int j;
	int c;
	superhudElement_t* tmp;

	for (i = 1; i < n; i++)
	{
		tmp = items[i];
		j = i;
		while (j > 0)
		{
			c = items[j - 1]->grp - tmp->grp;
			if (c == 0)
			{
				c = Q_stricmp(items[j - 1]->element.name ? items[j - 1]->element.name : "",
				              tmp->element.name ? tmp->element.name : "");
			}
			if (c <= 0)
			{
				break;
			}
			items[j] = items[j - 1];
			j--;
		}
		items[j] = tmp;
	}
}

static int CG_SHUDEditorBuildList(shudListRow_t* rows, int max)
{
	superhudElement_t* items[SHUD_EDIT_MAX_ITEMS];
	superhudElement_t* defEl;
	int n;
	int i;
	int r;
	int lastg;
	int g;

	if (max < 1)
	{
		return 0;
	}
	defEl = CG_SHUDEditorFindTabDefault(editor.tab);
	r = 0;
	rows[r].header = qfalse;
	rows[r].el = defEl;
	rows[r].def = NULL;
	rows[r].title = "DEFAULT";
	r++;

	n = CG_SHUDEditorCollect(items, SHUD_EDIT_MAX_ITEMS);
	CG_SHUDEditorSortEls(items, n);
	lastg = -1;
	for (i = 0; i < n && r < max; i++)
	{
		if (items[i]->tab != editor.tab)
		{
			continue;
		}
		if (!CG_SHUDEditorNameMatches(items[i]->element.name, editor.listFilter))
		{
			continue;
		}
		g = items[i]->grp;
		if (g != lastg)
		{
			rows[r].header = qtrue;
			rows[r].el = NULL;
			rows[r].def = NULL;
			rows[r].title = CG_SHUDGroupTitle(g);
			r++;
			lastg = g;
			if (r >= max)
			{
				break;
			}
		}
		rows[r].header = qfalse;
		rows[r].el = items[i];
		rows[r].def = NULL;
		rows[r].title = NULL;
		r++;
	}
	return r;
}

static superhudElement_t* CG_SHUDEditorFindByName(const char* name)
{
	superhudElement_t* el;

	if (!name || !name[0])
	{
		return NULL;
	}
	el = CG_SHUDGetElementsHead();
	while (el)
	{
		if (el->element.name && !Q_stricmp(el->element.name, name))
		{
			return el;
		}
		el = el->next;
	}
	return NULL;
}

static int CG_SHUDEditorIndexOf(superhudElement_t* el)
{
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];
	int count;
	int i;

	count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	for (i = 0; i < count; i++)
	{
		if (!rows[i].header && rows[i].el == el)
		{
			return i;
		}
	}
	return -1;
}

static int CG_SHUDEditorVisibleRows(void)
{
	int rows;

	rows = (int)((SHUD_EDIT_LIST_BOTTOM - SHUD_EDIT_LIST_Y) / SHUD_EDIT_ROW_H);
	if (rows < 1)
	{
		rows = 1;
	}
	return rows;
}

static void CG_SHUDEditorEnsureVisible(void)
{
	int vis;

	vis = CG_SHUDEditorVisibleRows();
	if (editor.selectedIndex < 0)
	{
		return;
	}
	if (editor.selectedIndex < editor.scroll)
	{
		editor.scroll = editor.selectedIndex;
	}
	if (editor.selectedIndex >= editor.scroll + vis)
	{
		editor.scroll = editor.selectedIndex - vis + 1;
	}
	if (editor.scroll < 0)
	{
		editor.scroll = 0;
	}
}

static void CG_SHUDEditorBeginUndoGroup(void)
{
	editor.undoGroupSeq++;
	if (editor.undoGroupSeq <= 0)
	{
		editor.undoGroupSeq = 1;
	}
	editor.undoGroup = editor.undoGroupSeq;
}

static void CG_SHUDEditorEndUndoGroup(void)
{
	editor.undoGroup = 0;
}

static qboolean CG_SHUDEditorSelHas(const superhudElement_t* el)
{
	int i;

	if (!el)
	{
		return qfalse;
	}
	for (i = 0; i < editor.selCount; i++)
	{
		if (editor.sel[i] == el)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_SHUDEditorSelAdd(superhudElement_t* el)
{
	if (!el || CG_SHUDEditorIsDefault(el) || CG_SHUDEditorSelHas(el))
	{
		return;
	}
	if (editor.selCount >= SHUD_EDIT_SEL_MAX)
	{
		return;
	}
	editor.sel[editor.selCount] = el;
	editor.selCount++;
}

static void CG_SHUDEditorSelRemove(superhudElement_t* el)
{
	int i;

	for (i = 0; i < editor.selCount; i++)
	{
		if (editor.sel[i] == el)
		{
			if (i < editor.selCount - 1)
			{
				memmove(&editor.sel[i], &editor.sel[i + 1],
				        sizeof(editor.sel[0]) * (editor.selCount - i - 1));
			}
			editor.selCount--;
			return;
		}
	}
}

static void CG_SHUDEditorSetPrimary(superhudElement_t* el)
{
	editor.selected = el;
	editor.selectedIndex = CG_SHUDEditorIndexOf(el);
	if (el && el->element.name)
	{
		Q_strncpyz(editor.selectedName, el->element.name, sizeof(editor.selectedName));
		if (el->tab >= 0 && el->tab < SHUD_TAB_COUNT)
		{
			editor.tab = el->tab;
		}
	}
	else
	{
		editor.selectedName[0] = 0;
		editor.selectedIndex = -1;
	}
	CG_SHUDEditorFixPropIndex();
	CG_SHUDEditorEnsureVisible();
}

static void CG_SHUDEditorSelect(superhudElement_t* el)
{
	editor.drag = SHUD_DRAG_NONE;
	editor.selCount = 0;
	CG_SHUDEditorSetPrimary(el);
	if (el && !CG_SHUDEditorIsDefault(el))
	{
		CG_SHUDEditorSelAdd(el);
	}
}

static void CG_SHUDEditorSelectIndex(int index)
{
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];
	int count;
	int i;
	int dir;

	count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	if (count <= 0)
	{
		CG_SHUDEditorSelect(NULL);
		return;
	}
	if (index < 0)
	{
		index = count - 1;
		dir = -1;
	}
	else if (index >= count)
	{
		index = 0;
		dir = 1;
	}
	else
	{
		dir = 1;
	}
	for (i = 0; i < count; i++)
	{
		if (rows[index].header)
		{
			index += dir;
		}
		else if (rows[index].el)
		{
			CG_SHUDEditorSelect(rows[index].el);
			return;
		}
		else if (rows[index].title && !Q_stricmp(rows[index].title, "DEFAULT"))
		{
			CG_SHUDEditorSelect(CG_SHUDEditorEnsureTabDefault(editor.tab));
			return;
		}
		else
		{
			index += dir;
		}
		if (index < 0)
		{
			index = count - 1;
		}
		if (index >= count)
		{
			index = 0;
		}
	}
	CG_SHUDEditorSelect(NULL);
}

static void CG_SHUDEditorClampRect(vec4_t rect)
{
	if (rect[2] < SHUD_EDIT_MIN_SIZE)
	{
		rect[2] = SHUD_EDIT_MIN_SIZE;
	}
	if (rect[3] < SHUD_EDIT_MIN_SIZE)
	{
		rect[3] = SHUD_EDIT_MIN_SIZE;
	}
	if (rect[0] < -SCREEN_WIDTH)
	{
		rect[0] = -SCREEN_WIDTH;
	}
	if (rect[1] < -SCREEN_HEIGHT)
	{
		rect[1] = -SCREEN_HEIGHT;
	}
	if (rect[0] > SCREEN_WIDTH * 2)
	{
		rect[0] = SCREEN_WIDTH * 2;
	}
	if (rect[1] > SCREEN_HEIGHT * 2)
	{
		rect[1] = SCREEN_HEIGHT * 2;
	}
}

static float CG_SHUDEditorSnapVal(float v)
{
	int g;
	int n;

	g = editor.gridSize > 0 ? editor.gridSize : (int)SHUD_EDIT_GRID_SIZE;
	if (g < 1)
	{
		return v;
	}
	if (v >= 0.0f)
	{
		n = (int)((v + (float)g * 0.5f) / (float)g);
	}
	else
	{
		n = (int)((v - (float)g * 0.5f) / (float)g);
	}
	return (float)(n * g);
}

static float CG_SHUDEditorSnapToList(float v, const float* list, int n, float pix)
{
	int i;
	float d;
	float best;
	float bestD;

	best = v;
	bestD = pix + 1.0f;
	for (i = 0; i < n; i++)
	{
		d = v - list[i];
		if (d < 0.0f)
		{
			d = -d;
		}
		if (d <= pix && d < bestD)
		{
			bestD = d;
			best = list[i];
		}
	}
	return best;
}

static void CG_SHUDEditorSnapNeighbors(vec4_t tmp)
{
	superhudElement_t* items[SHUD_EDIT_MAX_ITEMS];
	vec4_t r;
	float xs[SHUD_EDIT_MAX_ITEMS * 2 + 8];
	float ys[SHUD_EDIT_MAX_ITEMS * 2 + 8];
	int count;
	int i;
	int nx;
	int ny;
	float left;
	float right;
	float top;
	float bottom;
	float dl;
	float dr;
	float dt;
	float db;
	float pix;

	pix = SHUD_EDIT_SNAP_PIX;
	nx = 0;
	ny = 0;
	xs[nx++] = 0.0f;
	xs[nx++] = SCREEN_WIDTH * 0.5f;
	xs[nx++] = SCREEN_WIDTH;
	ys[ny++] = 0.0f;
	ys[ny++] = SCREEN_HEIGHT * 0.5f;
	ys[ny++] = SCREEN_HEIGHT;
	count = CG_SHUDEditorCollect(items, SHUD_EDIT_MAX_ITEMS);
	for (i = 0; i < count; i++)
	{
		if (items[i] == editor.selected)
		{
			continue;
		}
		if (!CG_SHUDEditorGetRect(items[i], r))
		{
			continue;
		}
		xs[nx++] = r[0];
		xs[nx++] = r[0] + r[2];
		ys[ny++] = r[1];
		ys[ny++] = r[1] + r[3];
	}
	left = CG_SHUDEditorSnapToList(tmp[0], xs, nx, pix);
	right = CG_SHUDEditorSnapToList(tmp[0] + tmp[2], xs, nx, pix);
	top = CG_SHUDEditorSnapToList(tmp[1], ys, ny, pix);
	bottom = CG_SHUDEditorSnapToList(tmp[1] + tmp[3], ys, ny, pix);
	dl = left - tmp[0];
	if (dl < 0.0f)
	{
		dl = -dl;
	}
	dr = (right - tmp[2]) - tmp[0];
	if (dr < 0.0f)
	{
		dr = -dr;
	}
	if (dl <= pix || dr <= pix)
	{
		tmp[0] = (dl <= dr) ? left : (right - tmp[2]);
	}
	dt = top - tmp[1];
	if (dt < 0.0f)
	{
		dt = -dt;
	}
	db = (bottom - tmp[3]) - tmp[1];
	if (db < 0.0f)
	{
		db = -db;
	}
	if (dt <= pix || db <= pix)
	{
		tmp[1] = (dt <= db) ? top : (bottom - tmp[3]);
	}
}

static void CG_SHUDEditorApplyRect(superhudElement_t* el, const vec4_t rect, qboolean round)
{
	vec4_t tmp;

	if (!el)
	{
		return;
	}
	Vector4Copy(rect, tmp);
	if (editor.snapGrid && editor.showGrid && !trap_Key_IsDown(K_ALT))
	{
		tmp[0] = CG_SHUDEditorSnapVal(tmp[0]);
		tmp[1] = CG_SHUDEditorSnapVal(tmp[1]);
		if (editor.drag != SHUD_DRAG_MOVE)
		{
			tmp[2] = CG_SHUDEditorSnapVal(tmp[2]);
			tmp[3] = CG_SHUDEditorSnapVal(tmp[3]);
		}
	}
	else if (round)
	{
		tmp[0] = (float)((int)(tmp[0] + (tmp[0] >= 0 ? 0.5f : -0.5f)));
		tmp[1] = (float)((int)(tmp[1] + (tmp[1] >= 0 ? 0.5f : -0.5f)));
		tmp[2] = (float)((int)(tmp[2] + 0.5f));
		tmp[3] = (float)((int)(tmp[3] + 0.5f));
	}
	if (editor.showGuides && !trap_Key_IsDown(K_ALT))
	{
		CG_SHUDEditorSnapNeighbors(tmp);
	}
	else if (round)
	{
		tmp[0] = (float)((int)(tmp[0] + (tmp[0] >= 0 ? 0.5f : -0.5f)));
		tmp[1] = (float)((int)(tmp[1] + (tmp[1] >= 0 ? 0.5f : -0.5f)));
		tmp[2] = (float)((int)(tmp[2] + 0.5f));
		tmp[3] = (float)((int)(tmp[3] + 0.5f));
	}
	CG_SHUDEditorClampRect(tmp);
	if (el->config.alignH.isSet)
	{
		if (el->config.alignH.value == SUPERHUD_ALIGNH_CENTER)
		{
			tmp[0] += tmp[2] * 0.5f;
		}
		else if (el->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
		{
			tmp[0] += tmp[2];
		}
	}
	if (el->config.alignV.isSet)
	{
		if (el->config.alignV.value == SUPERHUD_ALIGNV_CENTER)
		{
			tmp[1] += tmp[3] * 0.5f;
		}
		else if (el->config.alignV.value == SUPERHUD_ALIGNV_BOTTOM)
		{
			tmp[1] += tmp[3];
		}
	}
	el->config.rect.isSet = qtrue;
	Vector4Copy(tmp, el->config.rect.value);
	CG_SHUDRecreateElement(el);
	editor.dirty = qtrue;
}

static qboolean CG_SHUDEditorPointInRect(float x, float y, const vec4_t rect)
{
	return (x >= rect[0] && y >= rect[1] && x <= rect[0] + rect[2] && y <= rect[1] + rect[3]);
}

static qboolean CG_SHUDEditorPointInHandle(float x, float y, float hx, float hy)
{
	float half;

	half = SHUD_EDIT_HANDLE * 0.5f;
	return (x >= hx - half && x <= hx + half && y >= hy - half && y <= hy + half);
}

static shudEditorDrag_t CG_SHUDEditorHitHandle(float x, float y, const vec4_t rect)
{
	if (CG_SHUDEditorPointInHandle(x, y, rect[0], rect[1]))
	{
		return SHUD_DRAG_RESIZE_TL;
	}
	if (CG_SHUDEditorPointInHandle(x, y, rect[0] + rect[2], rect[1]))
	{
		return SHUD_DRAG_RESIZE_TR;
	}
	if (CG_SHUDEditorPointInHandle(x, y, rect[0], rect[1] + rect[3]))
	{
		return SHUD_DRAG_RESIZE_BL;
	}
	if (CG_SHUDEditorPointInHandle(x, y, rect[0] + rect[2], rect[1] + rect[3]))
	{
		return SHUD_DRAG_RESIZE_BR;
	}
	return SHUD_DRAG_NONE;
}

static qboolean CG_SHUDEditorHideBox(const superhudElement_t* el)
{
	if (!editor.hideOtherBoxes || !el)
	{
		return qfalse;
	}
	if (el == editor.selected)
	{
		return qfalse;
	}
	if (el->tab == editor.tab)
	{
		return qfalse;
	}
	return qtrue;
}

static superhudElement_t* CG_SHUDEditorHitElement(float x, float y)
{
	superhudElement_t* items[SHUD_EDIT_MAX_ITEMS];
	vec4_t rect;
	int count;
	int i;

	count = CG_SHUDEditorCollect(items, SHUD_EDIT_MAX_ITEMS);
	for (i = count - 1; i >= 0; i--)
	{
		if (CG_SHUDEditorIsGhost(items[i]))
		{
			continue;
		}
		if (CG_SHUDEditorHideBox(items[i]))
		{
			continue;
		}
		if (!CG_SHUDEditorGetRect(items[i], rect))
		{
			continue;
		}
		if (rect[2] < 8.0f)
		{
			rect[0] -= (8.0f - rect[2]) * 0.5f;
			rect[2] = 8.0f;
		}
		if (rect[3] < 8.0f)
		{
			rect[1] -= (8.0f - rect[3]) * 0.5f;
			rect[3] = 8.0f;
		}
		if (CG_SHUDEditorPointInRect(x, y, rect))
		{
			return items[i];
		}
	}
	return NULL;
}

static qboolean CG_SHUDEditorCursorInPanel(void)
{
	if (!editor.showList)
	{
		return qfalse;
	}
	return (editor.cursorX >= SHUD_EDIT_PANEL_X &&
	        editor.cursorX <= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W &&
	        editor.cursorY >= SHUD_EDIT_PANEL_Y &&
	        editor.cursorY <= SHUD_EDIT_PANEL_Y + SHUD_EDIT_PANEL_H);
}

static qboolean CG_SHUDEditorCursorInSearch(void)
{
	if (!editor.showList)
	{
		return qfalse;
	}
	return (editor.cursorX >= SHUD_EDIT_PANEL_X &&
	        editor.cursorX <= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W &&
	        editor.cursorY >= SHUD_EDIT_SEARCH_Y &&
	        editor.cursorY <= SHUD_EDIT_SEARCH_Y + SHUD_EDIT_SEARCH_H);
}

static qboolean CG_SHUDEditorCursorInProps(void)
{
	if (!editor.showProps)
	{
		return qfalse;
	}
	return (editor.cursorX >= SHUD_EDIT_PROPS_X &&
	        editor.cursorX <= SHUD_EDIT_PROPS_X + SHUD_EDIT_PROPS_W &&
	        editor.cursorY >= SHUD_EDIT_PROPS_Y &&
	        editor.cursorY <= SHUD_EDIT_PROPS_Y + SHUD_EDIT_PROPS_H);
}

static void CG_SHUDEditorCommit(int prop)
{
	int i;
	superhudElement_t* el;

	if (!editor.selected)
	{
		return;
	}
	editor.dirty = qtrue;
	CG_SHUDRecreateElement(editor.selected);
	if (prop < 0 || editor.selCount <= 1)
	{
		return;
	}
	for (i = 0; i < editor.selCount; i++)
	{
		el = editor.sel[i];
		if (!el || el == editor.selected)
		{
			continue;
		}
		if (!CG_SHUDEditorElementAlive(el))
		{
			continue;
		}
		CG_SHUDEditorCopyProp(el, editor.selected, prop);
		CG_SHUDRecreateElement(el);
	}
}

static qboolean CG_SHUDEditorElementAlive(superhudElement_t* el)
{
	return CG_SHUDElementChainIndex(el) >= 0 ? qtrue : qfalse;
}

static void CG_SHUDEditorClearUndo(void)
{
	editor.undoCount = 0;
	editor.redoCount = 0;
	editor.coalesceProp = -1;
	memset(editor.undo, 0, sizeof(editor.undo));
	memset(editor.redo, 0, sizeof(editor.redo));
}

static void CG_SHUDEditorPushUndo(shudUndoType_t type, superhudElement_t* el)
{
	shudUndoEntry_t* e;

	if (editor.undoLock)
	{
		return;
	}
	if (editor.undoCount == SHUD_EDIT_UNDO_MAX)
	{
		memmove(&editor.undo[0], &editor.undo[1], sizeof(editor.undo[0]) * (SHUD_EDIT_UNDO_MAX - 1));
		editor.undoCount--;
	}
	e = &editor.undo[editor.undoCount];
	memset(e, 0, sizeof(*e));
	e->type = type;
	e->el = el;
	if (el)
	{
		e->cfg = el->config;
		e->def = el->element;
		e->chainIndex = CG_SHUDElementChainIndex(el);
		e->tab = el->tab;
		e->grp = el->grp;
	}
	editor.undoCount++;
	e->batch = editor.undoGroup;
	editor.coalesceProp = -1;
}

static void CG_SHUDEditorPushConfigUndo(superhudElement_t* el)
{
	if (!el)
	{
		return;
	}
	if (editor.undoCount > 0 && editor.coalesceProp >= 0)
	{
		if (editor.undo[editor.undoCount - 1].type == SHUD_UNDO_CONFIG &&
		    editor.undo[editor.undoCount - 1].el == el)
		{
			return;
		}
	}
	CG_SHUDEditorPushUndo(SHUD_UNDO_CONFIG, el);
	editor.redoCount = 0;
}

static void CG_SHUDEditorPushSelConfigUndo(int prop)
{
	int i;
	superhudElement_t* el;
	qboolean didGroup;

	if (!editor.selected || editor.undoLock)
	{
		return;
	}
	didGroup = qfalse;
	if (editor.selCount > 1 && editor.coalesceProp != prop)
	{
		CG_SHUDEditorBeginUndoGroup();
		didGroup = qtrue;
		for (i = 0; i < editor.selCount; i++)
		{
			el = editor.sel[i];
			if (!el || el == editor.selected)
			{
				continue;
			}
			if (!CG_SHUDEditorElementAlive(el))
			{
				continue;
			}
			CG_SHUDEditorPushConfigUndo(el);
		}
	}
	CG_SHUDEditorPushConfigUndo(editor.selected);
	if (didGroup)
	{
		CG_SHUDEditorEndUndoGroup();
	}
}

static void CG_SHUDEditorUndo(void)
{
	shudUndoEntry_t e;
	shudUndoEntry_t redoE;
	superhudElement_t* el;
	superhudConfig_t tmpCfg;
	int batch;
	int restoredCount;
	superhudElement_t* restored[SHUD_EDIT_SEL_MAX];
	int i;

	if (editor.undoCount <= 0)
	{
		CG_Printf("^3SuperHUD editor: nothing to undo\n");
		return;
	}
	batch = editor.undo[editor.undoCount - 1].batch;
	restoredCount = 0;
	editor.undoLock = qtrue;
	do
	{
		editor.undoCount--;
		e = editor.undo[editor.undoCount];
		redoE = e;
		if (e.type == SHUD_UNDO_CONFIG)
		{
			if (CG_SHUDEditorElementAlive(e.el))
			{
				tmpCfg = e.el->config;
				e.el->config = e.cfg;
				CG_SHUDRecreateElement(e.el);
				editor.dirty = qtrue;
				redoE.cfg = tmpCfg;
				CG_SHUDEditorPushRedo(&redoE);
				if (restoredCount < SHUD_EDIT_SEL_MAX)
				{
					restored[restoredCount] = e.el;
					restoredCount++;
				}
			}
		}
		else if (e.type == SHUD_UNDO_INSERT)
		{
			if (CG_SHUDEditorElementAlive(e.el))
			{
				redoE.chainIndex = CG_SHUDElementChainIndex(e.el);
				redoE.cfg = e.el->config;
				redoE.def = e.el->element;
				CG_SHUDUnlinkElement(e.el);
				CG_SHUDDestroyElementNode(e.el);
				if (editor.selected == e.el)
				{
					editor.selected = NULL;
				}
				CG_SHUDEditorSelRemove(e.el);
				editor.dirty = qtrue;
				CG_SHUDEditorPushRedo(&redoE);
			}
		}
		else if (e.type == SHUD_UNDO_DELETE)
		{
			el = CG_SHUDAllocElement(&e.def, &e.cfg);
			if (el)
			{
				el->tab = e.tab;
				el->grp = e.grp;
				CG_SHUDInsertAtChainIndex(el, e.chainIndex);
				editor.dirty = qtrue;
				redoE.el = el;
				CG_SHUDEditorPushRedo(&redoE);
				if (restoredCount < SHUD_EDIT_SEL_MAX)
				{
					restored[restoredCount] = el;
					restoredCount++;
				}
			}
		}
	}
	while (batch && editor.undoCount > 0 && editor.undo[editor.undoCount - 1].batch == batch);
	editor.undoLock = qfalse;
	editor.coalesceProp = -1;
	if (restoredCount > 0)
	{
		editor.selCount = 0;
		for (i = 0; i < restoredCount; i++)
		{
			CG_SHUDEditorSelAdd(restored[i]);
		}
		CG_SHUDEditorSetPrimary(restored[restoredCount - 1]);
	}
	else if (!editor.selected)
	{
		CG_SHUDEditorSelectIndex(editor.selectedIndex >= 0 ? editor.selectedIndex : 0);
	}
}

static int CG_SHUDEditorCycle(int value, int dir, int count)
{
	if (count <= 0)
	{
		return 0;
	}
	value += dir;
	while (value < 0)
	{
		value += count;
	}
	while (value >= count)
	{
		value -= count;
	}
	return value;
}

static void CG_SHUDEditorClamp01(float* v)
{
	if (*v < 0.0f)
	{
		*v = 0.0f;
	}
	if (*v > 1.0f)
	{
		*v = 1.0f;
	}
}

static int CG_SHUDEditorCurrentVis(const superhudElement_t* el)
{
	if (el->config.visflags.isSet)
	{
		return el->config.visflags.value;
	}
	return el->element.visibility;
}

static void CG_SHUDEditorToggleVis(superhudElement_t* el, int bit)
{
	int flags;

	flags = CG_SHUDEditorCurrentVis(el);
	flags ^= bit;
	el->config.visflags.isSet = qtrue;
	el->config.visflags.value = flags;
}

static void CG_SHUDEditorEnsureRect(superhudElement_t* el)
{
	if (!el->config.rect.isSet)
	{
		el->config.rect.isSet = qtrue;
		el->config.rect.value[0] = 100.0f;
		el->config.rect.value[1] = 100.0f;
		el->config.rect.value[2] = 64.0f;
		el->config.rect.value[3] = 16.0f;
	}
}

static void CG_SHUDEditorEnsureFontSize(superhudElement_t* el)
{
	if (!el->config.fontsize.isSet)
	{
		el->config.fontsize.isSet = qtrue;
		el->config.fontsize.value[0] = 8.0f;
		el->config.fontsize.value[1] = 8.0f;
	}
}

static qboolean CG_SHUDEditorPropIsColor(int prop)
{
	return (prop == SHUD_PROP_COLOR ||
	        prop == SHUD_PROP_BGCOLOR ||
	        prop == SHUD_PROP_COLOR2 ||
	        prop == SHUD_PROP_BGCOLOR2 ||
	        prop == SHUD_PROP_BORDERC ||
	        prop == SHUD_PROP_BORDERC2);
}

static qboolean CG_SHUDEditorPropIsVec4(int prop)
{
	return (prop == SHUD_PROP_FADE ||
	        prop == SHUD_PROP_SHADOW ||
	        prop == SHUD_PROP_HLCOLOR);
}

static superhudColor_t* CG_SHUDEditorColorPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_BGCOLOR)
	{
		return &el->config.bgcolor.value;
	}
	if (prop == SHUD_PROP_COLOR2)
	{
		return &el->config.color2.value;
	}
	if (prop == SHUD_PROP_BGCOLOR2)
	{
		return &el->config.bgcolor2.value;
	}
	if (prop == SHUD_PROP_BORDERC)
	{
		return &el->config.borderColor.value;
	}
	if (prop == SHUD_PROP_BORDERC2)
	{
		return &el->config.borderColor2.value;
	}
	return &el->config.color.value;
}

static qboolean* CG_SHUDEditorColorSetPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_BGCOLOR)
	{
		return &el->config.bgcolor.isSet;
	}
	if (prop == SHUD_PROP_COLOR2)
	{
		return &el->config.color2.isSet;
	}
	if (prop == SHUD_PROP_BGCOLOR2)
	{
		return &el->config.bgcolor2.isSet;
	}
	if (prop == SHUD_PROP_BORDERC)
	{
		return &el->config.borderColor.isSet;
	}
	if (prop == SHUD_PROP_BORDERC2)
	{
		return &el->config.borderColor2.isSet;
	}
	return &el->config.color.isSet;
}

static float* CG_SHUDEditorVec4Ptr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_FADE)
	{
		return el->config.fade.value;
	}
	if (prop == SHUD_PROP_SHADOW)
	{
		return el->config.shadowColor.value;
	}
	if (prop == SHUD_PROP_HLCOLOR)
	{
		return el->config.hlcolor.value;
	}
	return NULL;
}

static qboolean* CG_SHUDEditorVec4SetPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_FADE)
	{
		return &el->config.fade.isSet;
	}
	if (prop == SHUD_PROP_SHADOW)
	{
		return &el->config.shadowColor.isSet;
	}
	if (prop == SHUD_PROP_HLCOLOR)
	{
		return &el->config.hlcolor.isSet;
	}
	return NULL;
}

static float* CG_SHUDEditorRgbaPtr(superhudElement_t* el, int prop)
{
	superhudColor_t* c;

	if (CG_SHUDEditorPropIsVec4(prop))
	{
		return CG_SHUDEditorVec4Ptr(el, prop);
	}
	if (!CG_SHUDEditorPropIsColor(prop))
	{
		return NULL;
	}
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!c || c->type != SUPERHUD_COLOR_RGBA)
	{
		return NULL;
	}
	return c->rgba;
}

static void CG_SHUDEditorEnsureColorProp(superhudElement_t* el, int prop)
{
	qboolean* setp;
	superhudColor_t* c;
	float* rgba;

	if (CG_SHUDEditorPropIsVec4(prop))
	{
		setp = CG_SHUDEditorVec4SetPtr(el, prop);
		rgba = CG_SHUDEditorVec4Ptr(el, prop);
		if (setp && rgba && !*setp)
		{
			*setp = qtrue;
			rgba[0] = 1.0f;
			rgba[1] = 1.0f;
			rgba[2] = 1.0f;
			rgba[3] = 1.0f;
		}
		else if (setp)
		{
			*setp = qtrue;
		}
		return;
	}
	setp = CG_SHUDEditorColorSetPtr(el, prop);
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!*setp || c->type != SUPERHUD_COLOR_RGBA)
	{
		*setp = qtrue;
		c->type = SUPERHUD_COLOR_RGBA;
		c->rgba[0] = 1.0f;
		c->rgba[1] = 1.0f;
		c->rgba[2] = 1.0f;
		c->rgba[3] = 1.0f;
	}
}

static void CG_SHUDEditorCycleColorType(superhudElement_t* el, int prop, int dir)
{
	qboolean* setp;
	superhudColor_t* c;
	int type;

	setp = CG_SHUDEditorColorSetPtr(el, prop);
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!*setp)
	{
		if (dir > 0)
		{
			CG_SHUDEditorEnsureColorProp(el, prop);
		}
		else
		{
			*setp = qtrue;
			c->type = SUPERHUD_COLOR_I;
		}
	}
	else
	{
		type = CG_SHUDEditorCycle((int)c->type, dir, 4);
		if (type == SUPERHUD_COLOR_RGBA)
		{
			CG_SHUDEditorEnsureColorProp(el, prop);
		}
		else
		{
			c->type = (superhudColorType_t)type;
		}
	}
	*setp = qtrue;
}

static qboolean CG_SHUDEditorColorHasSliders(const superhudElement_t* el, int prop)
{
	const superhudColor_t* c;
	qboolean set;

	if (!el)
	{
		return qfalse;
	}
	if (CG_SHUDEditorPropIsVec4(prop))
	{
		if (prop == SHUD_PROP_FADE)
		{
			return el->config.fade.isSet;
		}
		if (prop == SHUD_PROP_SHADOW)
		{
			return el->config.shadowColor.isSet;
		}
		if (prop == SHUD_PROP_HLCOLOR)
		{
			return el->config.hlcolor.isSet;
		}
		return qfalse;
	}
	if (!CG_SHUDEditorPropIsColor(prop))
	{
		return qfalse;
	}
	set = *CG_SHUDEditorColorSetPtr((superhudElement_t*)el, prop);
	c = CG_SHUDEditorColorPtr((superhudElement_t*)el, prop);
	return (set && c->type == SUPERHUD_COLOR_RGBA) ? qtrue : qfalse;
}

static void CG_SHUDEditorPropValue(const superhudElement_t* el, int prop, char* out, int outSize);
static void CG_SHUDEditorDeleteSelected(void);

static qboolean CG_SHUDEditorMatchFilter(const char* name, const char* filter)
{
	int i;
	int j;
	char a;
	char b;

	if (!filter || !filter[0])
	{
		return qtrue;
	}
	if (!name)
	{
		return qfalse;
	}
	for (i = 0; name[i]; i++)
	{
		for (j = 0; filter[j]; j++)
		{
			if (!name[i + j])
			{
				break;
			}
			a = name[i + j];
			b = filter[j];
			if (a >= 'A' && a <= 'Z')
			{
				a += 32;
			}
			if (b >= 'A' && b <= 'Z')
			{
				b += 32;
			}
			if (a != b)
			{
				break;
			}
		}
		if (!filter[j])
		{
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean CG_SHUDEditorIsGhost(const superhudElement_t* el)
{
	int i;

	for (i = 0; i < editor.ghostCount; i++)
	{
		if (editor.ghosts[i] == el)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_SHUDEditorToggleGhost(superhudElement_t* el)
{
	int i;

	if (!el)
	{
		return;
	}
	for (i = 0; i < editor.ghostCount; i++)
	{
		if (editor.ghosts[i] == el)
		{
			editor.ghosts[i] = editor.ghosts[editor.ghostCount - 1];
			editor.ghostCount--;
			return;
		}
	}
	if (editor.ghostCount < SHUD_EDIT_GHOST_MAX)
	{
		editor.ghosts[editor.ghostCount++] = el;
	}
}

static int CG_SHUDEditorVisBit(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_VIS_IM: return SE_IM;
		case SHUD_PROP_VIS_SPECT: return SE_SPECT;
		case SHUD_PROP_VIS_DEAD: return SE_DEAD;
		case SHUD_PROP_VIS_SCOREHIDE: return SE_SCORES_HIDE;
		case SHUD_PROP_VIS_SHOWEMPTY: return SE_SHOW_EMPTY;
		case SHUD_PROP_VIS_TEAMONLY: return SE_TEAM_ONLY;
		case SHUD_PROP_VIS_DEMOHIDE: return SE_DEMO_HIDE;
		case SHUD_PROP_VIS_KEY1: return SE_KEY1_SHOW;
		case SHUD_PROP_VIS_KEY2: return SE_KEY2_SHOW;
		case SHUD_PROP_VIS_KEY3: return SE_KEY3_SHOW;
		case SHUD_PROP_VIS_KEY4: return SE_KEY4_SHOW;
		case SHUD_PROP_VIS_GTFFA: return SE_GT_FFA;
		case SHUD_PROP_VIS_GTTOURNEY: return SE_GT_TOURNEY;
		case SHUD_PROP_VIS_GTTDM: return SE_GT_TDM;
		case SHUD_PROP_VIS_GTCTF: return SE_GT_CTF;
		case SHUD_PROP_VIS_GTFT: return SE_GT_FREEZETAG;
		case SHUD_PROP_VIS_GTCA: return SE_GT_CLANARENA;
		default: return 0;
	}
}

static void CG_SHUDEditorClearRedo(void)
{
	editor.redoCount = 0;
	memset(editor.redo, 0, sizeof(editor.redo));
}

static void CG_SHUDEditorPushRedo(shudUndoEntry_t* src)
{
	shudUndoEntry_t* e;

	if (editor.redoCount == SHUD_EDIT_UNDO_MAX)
	{
		memmove(&editor.redo[0], &editor.redo[1], sizeof(editor.redo[0]) * (SHUD_EDIT_UNDO_MAX - 1));
		editor.redoCount--;
	}
	e = &editor.redo[editor.redoCount];
	*e = *src;
	editor.redoCount++;
}

static void CG_SHUDEditorRedo(void)
{
	shudUndoEntry_t e;
	superhudConfig_t tmpCfg;
	superhudElement_t* el;
	int batch;
	int restoredCount;
	superhudElement_t* restored[SHUD_EDIT_SEL_MAX];
	int i;

	if (editor.redoCount <= 0)
	{
		CG_Printf("^3SuperHUD editor: nothing to redo\n");
		return;
	}
	batch = editor.redo[editor.redoCount - 1].batch;
	restoredCount = 0;
	editor.undoLock = qtrue;
	do
	{
		editor.redoCount--;
		e = editor.redo[editor.redoCount];
		if (e.type == SHUD_UNDO_CONFIG)
		{
			if (CG_SHUDEditorElementAlive(e.el))
			{
				tmpCfg = e.el->config;
				e.el->config = e.cfg;
				e.cfg = tmpCfg;
				CG_SHUDRecreateElement(e.el);
				editor.dirty = qtrue;
				editor.undoLock = qfalse;
				CG_SHUDEditorPushUndo(SHUD_UNDO_CONFIG, e.el);
				editor.undo[editor.undoCount - 1].cfg = e.cfg;
				editor.undo[editor.undoCount - 1].batch = e.batch;
				editor.undoLock = qtrue;
				if (restoredCount < SHUD_EDIT_SEL_MAX)
				{
					restored[restoredCount] = e.el;
					restoredCount++;
				}
			}
		}
		else if (e.type == SHUD_UNDO_INSERT)
		{
			el = CG_SHUDAllocElement(&e.def, &e.cfg);
			if (el)
			{
				el->tab = e.tab;
				el->grp = e.grp;
				CG_SHUDInsertAtChainIndex(el, e.chainIndex);
				editor.dirty = qtrue;
				editor.undoLock = qfalse;
				CG_SHUDEditorPushUndo(SHUD_UNDO_INSERT, el);
				editor.undo[editor.undoCount - 1].batch = e.batch;
				editor.undoLock = qtrue;
				if (restoredCount < SHUD_EDIT_SEL_MAX)
				{
					restored[restoredCount] = el;
					restoredCount++;
				}
			}
		}
		else if (e.type == SHUD_UNDO_DELETE)
		{
			if (CG_SHUDEditorElementAlive(e.el))
			{
				editor.undoLock = qfalse;
				editor.deleteArmed = 1;
				CG_SHUDEditorSelect(e.el);
				CG_SHUDEditorDeleteSelected();
				if (editor.undoCount > 0)
				{
					editor.undo[editor.undoCount - 1].batch = e.batch;
				}
				editor.undoLock = qtrue;
			}
		}
	}
	while (batch && editor.redoCount > 0 && editor.redo[editor.redoCount - 1].batch == batch);
	editor.undoLock = qfalse;
	if (restoredCount > 0)
	{
		editor.selCount = 0;
		for (i = 0; i < restoredCount; i++)
		{
			CG_SHUDEditorSelAdd(restored[i]);
		}
		CG_SHUDEditorSetPrimary(restored[restoredCount - 1]);
	}
}

static void CG_SHUDEditorTypicalSize(const char* name, vec4_t rect)
{
	rect[0] = editor.cursorX;
	rect[1] = editor.cursorY;
	rect[2] = 80.0f;
	rect[3] = 16.0f;
	if (!name)
	{
		return;
	}
	if (CG_SHUDEditorCursorInPanel() || CG_SHUDEditorCursorInProps())
	{
		rect[0] = 200.0f;
		rect[1] = 200.0f;
	}
	if (strstr(name, "icon") || strstr(name, "Icon") || strstr(name, "FlagStatus") || strstr(name, "rewardicon"))
	{
		rect[2] = 24.0f;
		rect[3] = 24.0f;
	}
	else if (strstr(name, "bar") || strstr(name, "Bar") || strstr(name, "sbhb") || strstr(name, "sbab") || strstr(name, "sbamb"))
	{
		rect[2] = 100.0f;
		rect[3] = 8.0f;
	}
	else if (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate"))
	{
		rect[2] = 64.0f;
		rect[3] = 64.0f;
	}
	else if (strstr(name, "chat") || strstr(name, "obituaries"))
	{
		rect[2] = 240.0f;
		rect[3] = 12.0f;
	}
}

static void CG_SHUDEditorAlignScreen(int mode)
{
	vec4_t rect;
	int i;
	int n;
	superhudElement_t* el;

	n = editor.selCount;
	if (n <= 0)
	{
		return;
	}
	CG_SHUDEditorBeginUndoGroup();
	for (i = 0; i < n; i++)
	{
		el = editor.sel[i];
		if (!CG_SHUDEditorGetRect(el, rect))
		{
			continue;
		}
		CG_SHUDEditorPushConfigUndo(el);
		if (mode == 0)
		{
			rect[0] = 0.0f;
		}
		else if (mode == 1)
		{
			rect[0] = (SCREEN_WIDTH - rect[2]) * 0.5f;
		}
		else if (mode == 2)
		{
			rect[0] = SCREEN_WIDTH - rect[2];
		}
		else if (mode == 3)
		{
			rect[1] = 0.0f;
		}
		else if (mode == 4)
		{
			rect[1] = (SCREEN_HEIGHT - rect[3]) * 0.5f;
		}
		else
		{
			rect[1] = SCREEN_HEIGHT - rect[3];
		}
		CG_SHUDEditorApplyRect(el, rect, qtrue);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.coalesceProp = -1;
}

static void CG_SHUDEditorMirrorH(void)
{
	vec4_t rect;
	int i;
	int n;
	superhudElement_t* el;

	n = editor.selCount;
	if (n <= 0)
	{
		return;
	}
	CG_SHUDEditorBeginUndoGroup();
	for (i = 0; i < n; i++)
	{
		el = editor.sel[i];
		if (!CG_SHUDEditorGetRect(el, rect))
		{
			continue;
		}
		CG_SHUDEditorPushConfigUndo(el);
		rect[0] = SCREEN_WIDTH - rect[0] - rect[2];
		CG_SHUDEditorApplyRect(el, rect, qtrue);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.coalesceProp = -1;
}

static void CG_SHUDEditorCopy(void)
{
	if (!editor.selected)
	{
		return;
	}
	editor.clipValid = qtrue;
	editor.clipCfg = editor.selected->config;
	editor.clipDef = editor.selected->element;
	Q_strncpyz(editor.clipName, editor.selected->element.name ? editor.selected->element.name : "", sizeof(editor.clipName));
	CG_Printf("^3SuperHUD editor: copied %s\n", editor.clipName);
}

static void CG_SHUDEditorPaste(void)
{
	superhudElement_t* el;
	superhudConfig_t cfg;

	if (!editor.clipValid)
	{
		CG_Printf("^3SuperHUD editor: clipboard empty\n");
		return;
	}
	if (!CG_SHUDEditorAllowMulti(editor.clipDef.name))
	{
		if (CG_SHUDEditorFindByName(editor.clipDef.name))
		{
			CG_Printf("^3SuperHUD editor: %s already exists, not pasted\n", editor.clipDef.name);
			return;
		}
	}
	memcpy(&cfg, &editor.clipCfg, sizeof(cfg));
	if (cfg.rect.isSet)
	{
		cfg.rect.value[0] += 8.0f;
		cfg.rect.value[1] += 8.0f;
	}
	el = CG_SHUDAllocElement(&editor.clipDef, &cfg);
	if (!el)
	{
		return;
	}
	el->tab = CG_SHUDNameTab(editor.clipDef.name);
	if (el->tab < 0)
	{
		el->tab = editor.tab;
	}
	el->grp = CG_SHUDNameGroup(editor.clipDef.name);
	if (el->grp < 0)
	{
		if (editor.selected && !CG_SHUDEditorIsDefault(editor.selected))
		{
			el->grp = editor.selected->grp;
		}
		else
		{
			el->grp = SHUD_GRP_DECOR;
		}
	}
	CG_SHUDLinkElementAfter(editor.selected, el);
	CG_SHUDEditorPushUndo(SHUD_UNDO_INSERT, el);
	CG_SHUDEditorSelect(el);
	editor.dirty = qtrue;
}

static void CG_SHUDEditorLayer(int dir)
{
	if (!editor.selected)
	{
		return;
	}
	editor.coalesceProp = -1;
	if (dir < 0)
	{
		CG_SHUDSwapWithPrev(editor.selected);
	}
	else
	{
		CG_SHUDSwapWithNext(editor.selected);
	}
	CG_SHUDEditorSelect(editor.selected);
	editor.dirty = qtrue;
}

static void CG_SHUDEditorStartTypeIn(int prop)
{
	char cur[64];

	if (!editor.selected && prop >= 0)
	{
		return;
	}
	editor.typing = qtrue;
	editor.typeProp = prop;
	editor.typeBuf[0] = 0;
	if (prop == -1)
	{
		Q_strncpyz(editor.typeBuf, editor.listFilter, sizeof(editor.typeBuf));
	}
	else if (prop == -2)
	{
		Q_strncpyz(editor.typeBuf, editor.pickerFilter, sizeof(editor.typeBuf));
	}
	else if (prop >= 0)
	{
		CG_SHUDEditorPropValue(editor.selected, prop, cur, sizeof(cur));
		if (Q_stricmp(cur, "--") && Q_stricmp(cur, "?"))
		{
			Q_strncpyz(editor.typeBuf, cur, sizeof(editor.typeBuf));
		}
	}
}

static void CG_SHUDEditorCancelTypeIn(void)
{
	editor.typing = qfalse;
	editor.typeBuf[0] = 0;
}

static void CG_SHUDEditorSyncFilter(void)
{
	if (editor.typeProp == -1)
	{
		Q_strncpyz(editor.listFilter, editor.typeBuf, sizeof(editor.listFilter));
		editor.filterOn = editor.listFilter[0] ? qtrue : qfalse;
		editor.scroll = 0;
	}
	else if (editor.typeProp == -2)
	{
		Q_strncpyz(editor.pickerFilter, editor.typeBuf, sizeof(editor.pickerFilter));
		editor.pickerScroll = 0;
	}
}

static void CG_SHUDEditorCommitTypeIn(void)
{
	superhudConfig_t* cfg;
	float v;
	int prop;

	prop = editor.typeProp;
	editor.typing = qfalse;
	if (prop == -1 || prop == -2)
	{
		CG_SHUDEditorSyncFilter();
		return;
	}
	if (!editor.selected)
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	editor.coalesceProp = -1;
	cfg = &editor.selected->config;
	v = (float)atof(editor.typeBuf);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[0] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_Y:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[1] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_W:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[2] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_H:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[3] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_FS_W:
			CG_SHUDEditorEnsureFontSize(editor.selected);
			cfg->fontsize.value[0] = v < 1.0f ? 1.0f : v;
			break;
		case SHUD_PROP_FS_H:
			CG_SHUDEditorEnsureFontSize(editor.selected);
			cfg->fontsize.value[1] = v < 1.0f ? 1.0f : v;
			break;
		case SHUD_PROP_FONT:
			cfg->font.isSet = qtrue;
			Q_strncpyz(cfg->font.value, editor.typeBuf, sizeof(cfg->font.value));
			break;
		case SHUD_PROP_IMAGE:
			cfg->image.isSet = qtrue;
			Q_strncpyz(cfg->image.value, editor.typeBuf, sizeof(cfg->image.value));
			break;
		case SHUD_PROP_TEXT:
			cfg->text.isSet = qtrue;
			Q_strncpyz(cfg->text.value, editor.typeBuf, sizeof(cfg->text.value));
			break;
		case SHUD_PROP_TEXTOFF_X:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[0] = v;
			break;
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[1] = v;
			break;
		case SHUD_PROP_TIME:
			cfg->time.isSet = qtrue;
			cfg->time.value = (int)v;
			break;
		case SHUD_PROP_FADEDELAY:
			cfg->fadedelay.isSet = qtrue;
			cfg->fadedelay.value = (int)v;
			break;
		case SHUD_PROP_HLSIZE:
			cfg->hlsize.isSet = qtrue;
			cfg->hlsize.value = (int)v;
			break;
		case SHUD_PROP_STYLE:
			cfg->style.isSet = qtrue;
			cfg->style.value = (int)v;
			break;
		case SHUD_PROP_BORDER_L:
			cfg->border.isSet = qtrue;
			cfg->border.value[0] = v;
			break;
		case SHUD_PROP_BORDER_T:
			cfg->border.isSet = qtrue;
			cfg->border.value[1] = v;
			break;
		case SHUD_PROP_BORDER_R:
			cfg->border.isSet = qtrue;
			cfg->border.value[2] = v;
			break;
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qtrue;
			cfg->border.value[3] = v;
			break;
		case SHUD_PROP_IMAGETC_S:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[0] = v;
			break;
		case SHUD_PROP_IMAGETC_T:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[1] = v;
			break;
		case SHUD_PROP_IMAGETC_U:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[2] = v;
			break;
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[3] = v;
			break;
		case SHUD_PROP_MARGIN_L:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[0] = v;
			break;
		case SHUD_PROP_MARGIN_T:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[1] = v;
			break;
		case SHUD_PROP_MARGIN_R:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[2] = v;
			break;
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[3] = v;
			break;
		default:
			break;
	}
	CG_SHUDEditorCommit(prop);
}

static void CG_SHUDEditorTypeKey(int key)
{
	int len;

	if (key == K_ESCAPE)
	{
		CG_SHUDEditorCancelTypeIn();
		return;
	}
	if (key == K_ENTER)
	{
		CG_SHUDEditorCommitTypeIn();
		return;
	}
	if (key == K_BACKSPACE)
	{
		len = (int)strlen(editor.typeBuf);
		if (len > 0)
		{
			editor.typeBuf[len - 1] = 0;
			CG_SHUDEditorSyncFilter();
		}
		return;
	}
	if (key < 32 || key > 126)
	{
		return;
	}
	len = (int)strlen(editor.typeBuf);
	if (len >= SHUD_EDIT_TYPE_MAX - 1)
	{
		return;
	}
	editor.typeBuf[len] = (char)key;
	editor.typeBuf[len + 1] = 0;
	CG_SHUDEditorSyncFilter();
}

static void CG_SHUDEditorDrawHelp(void)
{
	static const char* lines[] =
	{
		"^3F1 help  ESC close",
		"F5 save   Ctrl+S save",
		"N add     D dup     Del delete",
		"Enter type-in   click search",
		"Bksp unset prop",
		"G grid mode   F3 grid size",
		"C center guides / neighbor snap",
		"A/V align screen   M mirror",
		"Ctrl+Arrows align to edges",
		"Shift+Arrows faster move",
		"Ctrl+Z undo  Ctrl+Y redo",
		"Ctrl+C/V copy paste",
		"I pipette color   F force-show",
		"B hide boxes of other tabs",
		"Shift-click add to selection",
		"Drag empty: box-select",
		"H hide in editor   Shift resize lock",
		"P list  O props  Del delete",
		"1-8 tabs   click DEFAULT",
		NULL
	};
	int i;
	float y;

	CG_FillRect(170, 40, 300, 356, shudEditPanelBg);
	CG_DrawRect(170, 40, 300, 356, 1, shudEditPanelBorder);
	y = 48.0f;
	for (i = 0; lines[i]; i++)
	{
		CG_OSPDrawString(178, y, lines[i], colorWhite, 6, 8, 284, DS_HLEFT | DS_PROPORTIONAL, NULL);
		y += 12.0f;
	}
}

static void CG_SHUDEditorDrawGuides(void)
{
	vec4_t c;

	if (!editor.showGuides)
	{
		return;
	}
	Vector4Set(c, 1.0f, 0.85f, 0.2f, 0.18f);
	CG_FillRect(SCREEN_WIDTH * 0.5f, 0, 1, SCREEN_HEIGHT, c);
	CG_FillRect(0, SCREEN_HEIGHT * 0.5f, SCREEN_WIDTH, 1, c);
}

static int CG_SHUDEditorPropVecIndex(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_TEXTOFF_X: return 0;
		case SHUD_PROP_TEXTOFF_Y: return 1;
		case SHUD_PROP_BORDER_L: return 0;
		case SHUD_PROP_BORDER_T: return 1;
		case SHUD_PROP_BORDER_R: return 2;
		case SHUD_PROP_BORDER_B: return 3;
		case SHUD_PROP_IMAGETC_S: return 0;
		case SHUD_PROP_IMAGETC_T: return 1;
		case SHUD_PROP_IMAGETC_U: return 2;
		case SHUD_PROP_IMAGETC_V: return 3;
		case SHUD_PROP_MARGIN_L: return 0;
		case SHUD_PROP_MARGIN_T: return 1;
		case SHUD_PROP_MARGIN_R: return 2;
		case SHUD_PROP_MARGIN_B: return 3;
		default: return -1;
	}
}

static int CG_SHUDEditorPropCat(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			return 0;
		case SHUD_PROP_ALIGNH:
		case SHUD_PROP_ALIGNV:
		case SHUD_PROP_TEXTALIGN:
		case SHUD_PROP_DIRECTION:
			return 1;
		case SHUD_PROP_FONT:
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
		case SHUD_PROP_TEXT:
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
		case SHUD_PROP_TEXTSTYLE:
		case SHUD_PROP_MONO:
		case SHUD_PROP_SHADOW:
			return 2;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_FILL:
		case SHUD_PROP_FADE:
			return 3;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			return 4;
		case SHUD_PROP_IMAGE:
			return 5;
		case SHUD_PROP_STYLE:
		case SHUD_PROP_DOUBLEBAR:
			return 6;
		case SHUD_PROP_TIME:
		case SHUD_PROP_FADEDELAY:
			return 7;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			return 8;
		default:
			return 9;
	}
}

static const char* CG_SHUDEditorPropCatTitle(int cat)
{
	switch (cat)
	{
		case 0: return "RECT";
		case 1: return "ALIGN";
		case 2: return "TEXT";
		case 3: return "COLOR";
		case 4: return "BORDER";
		case 5: return "IMAGE";
		case 6: return "STYLE";
		case 7: return "TIME";
		case 8: return "VIS";
		default: return "OTHER";
	}
}

static int CG_SHUDEditorLayout(shudLay_t* out, int max)
{
	int n;
	int p;
	int ch;
	int cat;
	int have;
	float y;

	n = 0;
	y = SHUD_EDIT_PROP_LIST_Y - (float)editor.propScroll * SHUD_EDIT_PROP_ROW;
	for (cat = 0; cat <= 9 && n < max; cat++)
	{
		have = 0;
		for (p = 0; p < SHUD_PROP_COUNT && n < max; p++)
		{
			if (CG_SHUDEditorPropCat(p) != cat)
			{
				continue;
			}
			if (editor.selected && !CG_SHUDEditorPropVisible(editor.selected, p))
			{
				continue;
			}
			if (!have)
			{
				out[n].kind = 2;
				out[n].prop = cat;
				out[n].channel = 0;
				out[n].y = y;
				n++;
				y += SHUD_EDIT_PROP_ROW;
				have = 1;
				if (n >= max)
				{
					return n;
				}
			}
			out[n].kind = 0;
			out[n].prop = p;
			out[n].channel = 0;
			out[n].y = y;
			n++;
			y += SHUD_EDIT_PROP_ROW;
			if (CG_SHUDEditorColorHasSliders(editor.selected, p))
			{
				for (ch = 0; ch < 4 && n < max; ch++)
				{
					out[n].kind = 1;
					out[n].prop = p;
					out[n].channel = ch;
					out[n].y = y;
					n++;
					y += SHUD_EDIT_SLIDER_H;
				}
			}
		}
	}
	return n;
}

static int CG_SHUDEditorHitLayout(shudLay_t* item)
{
	shudLay_t lay[SHUD_EDIT_LAY_MAX];
	int n;
	int i;
	float h;

	if (!CG_SHUDEditorCursorInProps())
	{
		return -1;
	}
	n = CG_SHUDEditorLayout(lay, SHUD_EDIT_LAY_MAX);
	for (i = 0; i < n; i++)
	{
		h = (lay[i].kind == 1) ? SHUD_EDIT_SLIDER_H : SHUD_EDIT_PROP_ROW;
		if (lay[i].kind == 2)
		{
			continue;
		}
		if (editor.cursorY >= lay[i].y && editor.cursorY < lay[i].y + h &&
		    editor.cursorY >= SHUD_EDIT_PROP_LIST_Y && editor.cursorY < SHUD_EDIT_PROP_BOTTOM)
		{
			if (item)
			{
				*item = lay[i];
			}
			return i;
		}
	}
	return -1;
}

static void CG_SHUDEditorSetSlider(int prop, int channel, float t)
{
	float* rgba;

	if (!editor.selected)
	{
		return;
	}
	if (channel < 0)
	{
		channel = 0;
	}
	if (channel > 3)
	{
		channel = 3;
	}
	CG_SHUDEditorEnsureColorProp(editor.selected, prop);
	rgba = CG_SHUDEditorRgbaPtr(editor.selected, prop);
	if (!rgba)
	{
		return;
	}
	if (t < 0.0f)
	{
		t = 0.0f;
	}
	if (t > 1.0f)
	{
		t = 1.0f;
	}
	rgba[channel] = t;
	CG_SHUDEditorCommit(prop);
}

static void CG_SHUDEditorSliderFromCursor(int prop, int channel)
{
	float t;

	t = (editor.cursorX - (SHUD_EDIT_PROPS_X + 24.0f)) / (SHUD_EDIT_PROPS_W - 72.0f);
	CG_SHUDEditorSetSlider(prop, channel, t);
}

static qboolean CG_SHUDEditorAllowMulti(const char* name)
{
	if (!name)
	{
		return qfalse;
	}
	return (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate")) ? qtrue : qfalse;
}

static int CG_SHUDEditorCollectTypes(const superHUDConfigElement_t** out, int max)
{
	const superHUDConfigElement_t* e;
	int count;
	int i;
	int j;
	int t;
	const superHUDConfigElement_t* tmp;

	count = 0;
	e = CG_SHUDAvailableElementsGet();
	while (e && e->name && count < max)
	{
		if (e->create && !CG_SHUDIsDefaultName(e->name))
		{
			t = CG_SHUDNameTab(e->name);
			if (t >= 0 && t != editor.tab && !CG_SHUDIsDecorName(e->name))
			{
				e++;
				continue;
			}
			if (CG_SHUDEditorNameMatches(e->name, editor.pickerFilter))
			{
				if (out)
				{
					out[count] = e;
				}
				count++;
			}
		}
		e++;
	}
	if (!out)
	{
		return count;
	}
	for (i = 1; i < count; i++)
	{
		tmp = out[i];
		j = i;
		while (j > 0 && CG_SHUDEditorCmpName(out[j - 1]->name, tmp->name) > 0)
		{
			out[j] = out[j - 1];
			j--;
		}
		out[j] = tmp;
	}
	return count;
}

static int CG_SHUDEditorBuildTypeList(shudListRow_t* rows, int max)
{
	const superHUDConfigElement_t* types[SHUD_EDIT_MAX_ITEMS];
	int n;
	int i;
	int r;
	int g;
	int lastg;

	n = CG_SHUDEditorCollectTypes(types, SHUD_EDIT_MAX_ITEMS);
	r = 0;
	lastg = -1;
	for (i = 0; i < n && r < max; i++)
	{
		g = CG_SHUDEditorNameGroup(types[i]->name);
		if (g != lastg)
		{
			rows[r].header = qtrue;
			rows[r].el = NULL;
			rows[r].def = NULL;
			rows[r].title = CG_SHUDEditorGroupTitle(g);
			r++;
			lastg = g;
			if (r >= max)
			{
				break;
			}
		}
		rows[r].header = qfalse;
		rows[r].el = NULL;
		rows[r].def = types[i];
		rows[r].title = NULL;
		r++;
	}
	return r;
}

static superhudElement_t* CG_SHUDEditorFindDefault(void)
{
	superhudElement_t* el;

	el = CG_SHUDGetElementsHead();
	while (el)
	{
		if (el->element.name && !Q_stricmp(el->element.name, "!default"))
		{
			return el;
		}
		el = el->next;
	}
	return NULL;
}

static void CG_SHUDEditorAddType(const superHUDConfigElement_t* def)
{
	superhudElement_t* el;
	superhudElement_t* defEl;
	superhudElement_t* existing;
	superhudConfig_t cfg;
	int t;

	if (!def)
	{
		return;
	}
	if (!CG_SHUDEditorAllowMulti(def->name))
	{
		existing = CG_SHUDEditorFindByName(def->name);
		if (existing)
		{
			CG_SHUDEditorSelect(existing);
			editor.picker = qfalse;
			CG_Printf("^3SuperHUD editor: %s already exists\n", def->name);
			return;
		}
	}
	t = CG_SHUDNameTab(def->name);
	if (t < 0)
	{
		t = editor.tab;
	}
	memset(&cfg, 0, sizeof(cfg));
	defEl = CG_SHUDEditorFindTabDefault(t);
	if (defEl)
	{
		memcpy(&cfg, &defEl->config, sizeof(cfg));
	}
	cfg.rect.isSet = qtrue;
	CG_SHUDEditorTypicalSize(def->name, cfg.rect.value);
	el = CG_SHUDAllocElement(def, &cfg);
	if (!el)
	{
		return;
	}
	el->tab = t;
	el->grp = CG_SHUDNameGroup(def->name);
	if (el->grp < 0)
	{
		if (editor.selected && editor.selected->tab == t && !CG_SHUDEditorIsDefault(editor.selected))
		{
			el->grp = editor.selected->grp;
		}
		else
		{
			el->grp = SHUD_GRP_DECOR;
		}
	}
	CG_SHUDLinkElementAfter(editor.selected, el);
	CG_SHUDEditorPushUndo(SHUD_UNDO_INSERT, el);
	editor.redoCount = 0;
	CG_SHUDEditorSelect(el);
	editor.picker = qfalse;
	editor.dirty = qtrue;
}

static void CG_SHUDEditorDuplicate(void)
{
	superhudElement_t* el;
	superhudConfig_t cfg;

	if (!editor.selected)
	{
		return;
	}
	if (editor.selected->element.name && !Q_stricmp(editor.selected->element.name, "!default"))
	{
		CG_Printf("^3SuperHUD editor: cannot duplicate !default\n");
		return;
	}
	if (!CG_SHUDEditorAllowMulti(editor.selected->element.name))
	{
		CG_Printf("^3SuperHUD editor: %s is unique, not duplicated\n", editor.selected->element.name);
		return;
	}
	memcpy(&cfg, &editor.selected->config, sizeof(cfg));
	if (cfg.rect.isSet)
	{
		cfg.rect.value[0] += 8.0f;
		cfg.rect.value[1] += 8.0f;
	}
	el = CG_SHUDAllocElement(&editor.selected->element, &cfg);
	if (!el)
	{
		return;
	}
	el->tab = editor.selected->tab;
	el->grp = editor.selected->grp;
	CG_SHUDLinkElementAfter(editor.selected, el);
	CG_SHUDEditorPushUndo(SHUD_UNDO_INSERT, el);
	editor.redoCount = 0;
	CG_SHUDEditorSelect(el);
	editor.dirty = qtrue;
}

static void CG_SHUDEditorDeleteSelected(void)
{
	superhudElement_t* list[SHUD_EDIT_SEL_MAX];
	superhudElement_t* el;
	int n;
	int i;
	int idx;

	if (!editor.selected)
	{
		return;
	}
	if (CG_SHUDEditorIsDefault(editor.selected) && editor.selCount <= 0)
	{
		CG_Printf("^3SuperHUD editor: DEFAULT is the tab inherit, not deleted. Backspace unsets fields.\n");
		return;
	}
	n = editor.selCount;
	if (n <= 0)
	{
		el = editor.selected;
		if (CG_SHUDEditorIsDefault(el))
		{
			CG_Printf("^3SuperHUD editor: DEFAULT is the tab inherit, not deleted. Backspace unsets fields.\n");
			return;
		}
		idx = editor.selectedIndex;
		CG_SHUDEditorPushUndo(SHUD_UNDO_DELETE, el);
		editor.redoCount = 0;
		CG_SHUDUnlinkElement(el);
		CG_SHUDDestroyElementNode(el);
		editor.selected = NULL;
		editor.selectedName[0] = 0;
		editor.selCount = 0;
		CG_SHUDEditorSelectIndex(idx);
		editor.dirty = qtrue;
		return;
	}
	idx = editor.selectedIndex;
	for (i = 0; i < n; i++)
	{
		list[i] = editor.sel[i];
	}
	CG_SHUDEditorBeginUndoGroup();
	for (i = 0; i < n; i++)
	{
		el = list[i];
		if (!el || CG_SHUDEditorIsDefault(el))
		{
			continue;
		}
		CG_SHUDEditorPushUndo(SHUD_UNDO_DELETE, el);
		CG_SHUDUnlinkElement(el);
		CG_SHUDDestroyElementNode(el);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.redoCount = 0;
	editor.selected = NULL;
	editor.selectedName[0] = 0;
	editor.selCount = 0;
	CG_SHUDEditorSelectIndex(idx);
	editor.dirty = qtrue;
}

static void CG_SHUDEditorCopyProp(superhudElement_t* dst, superhudElement_t* src, int prop)
{
	int bit;
	int flags;
	int idx;

	if (!dst || !src || dst == src)
	{
		return;
	}
	idx = CG_SHUDEditorPropVecIndex(prop);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			if (!src->config.rect.isSet)
			{
				dst->config.rect.isSet = qfalse;
				break;
			}
			CG_SHUDEditorEnsureRect(dst);
			if (prop == SHUD_PROP_RECT_X)
			{
				dst->config.rect.value[0] = src->config.rect.value[0];
			}
			else if (prop == SHUD_PROP_RECT_Y)
			{
				dst->config.rect.value[1] = src->config.rect.value[1];
			}
			else if (prop == SHUD_PROP_RECT_W)
			{
				dst->config.rect.value[2] = src->config.rect.value[2];
			}
			else
			{
				dst->config.rect.value[3] = src->config.rect.value[3];
			}
			break;
		case SHUD_PROP_FONT:
			memcpy(&dst->config.font, &src->config.font, sizeof(dst->config.font));
			break;
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			if (!src->config.fontsize.isSet)
			{
				dst->config.fontsize.isSet = qfalse;
				break;
			}
			CG_SHUDEditorEnsureFontSize(dst);
			if (prop == SHUD_PROP_FS_W)
			{
				dst->config.fontsize.value[0] = src->config.fontsize.value[0];
			}
			else
			{
				dst->config.fontsize.value[1] = src->config.fontsize.value[1];
			}
			break;
		case SHUD_PROP_ALIGNH:
			dst->config.alignH.isSet = src->config.alignH.isSet;
			dst->config.alignH.value = src->config.alignH.value;
			break;
		case SHUD_PROP_ALIGNV:
			dst->config.alignV.isSet = src->config.alignV.isSet;
			dst->config.alignV.value = src->config.alignV.value;
			break;
		case SHUD_PROP_TEXTALIGN:
			dst->config.textAlign.isSet = src->config.textAlign.isSet;
			dst->config.textAlign.value = src->config.textAlign.value;
			break;
		case SHUD_PROP_DIRECTION:
			dst->config.direction.isSet = src->config.direction.isSet;
			dst->config.direction.value = src->config.direction.value;
			break;
		case SHUD_PROP_COLOR:
			memcpy(&dst->config.color, &src->config.color, sizeof(dst->config.color));
			break;
		case SHUD_PROP_BGCOLOR:
			memcpy(&dst->config.bgcolor, &src->config.bgcolor, sizeof(dst->config.bgcolor));
			break;
		case SHUD_PROP_COLOR2:
			memcpy(&dst->config.color2, &src->config.color2, sizeof(dst->config.color2));
			break;
		case SHUD_PROP_BGCOLOR2:
			memcpy(&dst->config.bgcolor2, &src->config.bgcolor2, sizeof(dst->config.bgcolor2));
			break;
		case SHUD_PROP_BORDERC:
			memcpy(&dst->config.borderColor, &src->config.borderColor, sizeof(dst->config.borderColor));
			break;
		case SHUD_PROP_BORDERC2:
			memcpy(&dst->config.borderColor2, &src->config.borderColor2, sizeof(dst->config.borderColor2));
			break;
		case SHUD_PROP_FADE:
			memcpy(&dst->config.fade, &src->config.fade, sizeof(dst->config.fade));
			break;
		case SHUD_PROP_SHADOW:
			memcpy(&dst->config.shadowColor, &src->config.shadowColor, sizeof(dst->config.shadowColor));
			break;
		case SHUD_PROP_HLCOLOR:
			memcpy(&dst->config.hlcolor, &src->config.hlcolor, sizeof(dst->config.hlcolor));
			break;
		case SHUD_PROP_IMAGE:
			memcpy(&dst->config.image, &src->config.image, sizeof(dst->config.image));
			break;
		case SHUD_PROP_TEXT:
			memcpy(&dst->config.text, &src->config.text, sizeof(dst->config.text));
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			if (!src->config.textOffset.isSet)
			{
				dst->config.textOffset.isSet = qfalse;
				break;
			}
			dst->config.textOffset.isSet = qtrue;
			dst->config.textOffset.value[idx] = src->config.textOffset.value[idx];
			break;
		case SHUD_PROP_ITTEAM:
			dst->config.itTeam.isSet = src->config.itTeam.isSet;
			dst->config.itTeam.value = src->config.itTeam.value;
			break;
		case SHUD_PROP_TIME:
			dst->config.time.isSet = src->config.time.isSet;
			dst->config.time.value = src->config.time.value;
			break;
		case SHUD_PROP_FADEDELAY:
			dst->config.fadedelay.isSet = src->config.fadedelay.isSet;
			dst->config.fadedelay.value = src->config.fadedelay.value;
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			if (!src->config.border.isSet)
			{
				dst->config.border.isSet = qfalse;
				break;
			}
			dst->config.border.isSet = qtrue;
			dst->config.border.value[idx] = src->config.border.value[idx];
			break;
		case SHUD_PROP_HLSIZE:
			dst->config.hlsize.isSet = src->config.hlsize.isSet;
			dst->config.hlsize.value = src->config.hlsize.value;
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			if (!src->config.imagetc.isSet)
			{
				dst->config.imagetc.isSet = qfalse;
				break;
			}
			dst->config.imagetc.isSet = qtrue;
			dst->config.imagetc.value[idx] = src->config.imagetc.value[idx];
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			if (!src->config.margins.isSet)
			{
				dst->config.margins.isSet = qfalse;
				break;
			}
			dst->config.margins.isSet = qtrue;
			dst->config.margins.value[idx] = src->config.margins.value[idx];
			break;
		case SHUD_PROP_STYLE:
			dst->config.style.isSet = src->config.style.isSet;
			dst->config.style.value = src->config.style.value;
			break;
		case SHUD_PROP_TEXTSTYLE:
			dst->config.textStyle.isSet = src->config.textStyle.isSet;
			dst->config.textStyle.value = src->config.textStyle.value;
			break;
		case SHUD_PROP_FILL:
			dst->config.fill.isSet = src->config.fill.isSet;
			break;
		case SHUD_PROP_MONO:
			dst->config.monospace.isSet = src->config.monospace.isSet;
			break;
		case SHUD_PROP_DOUBLEBAR:
			dst->config.doublebar.isSet = src->config.doublebar.isSet;
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			if (!src->config.visflags.isSet)
			{
				dst->config.visflags.isSet = qfalse;
				break;
			}
			bit = CG_SHUDEditorVisBit(prop);
			flags = CG_SHUDEditorCurrentVis(dst);
			if (CG_SHUDEditorCurrentVis(src) & bit)
			{
				flags |= bit;
			}
			else
			{
				flags &= ~bit;
			}
			dst->config.visflags.isSet = qtrue;
			dst->config.visflags.value = flags;
			break;
		default:
			break;
	}
}

static void CG_SHUDEditorUnsetProp(int prop)
{
	superhudConfig_t* cfg;

	if (!editor.selected)
	{
		return;
	}
	if (!CG_SHUDEditorPropVisible(editor.selected, prop))
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	cfg = &editor.selected->config;
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			cfg->rect.isSet = qfalse;
			break;
		case SHUD_PROP_FONT:
			cfg->font.isSet = qfalse;
			break;
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			cfg->fontsize.isSet = qfalse;
			break;
		case SHUD_PROP_ALIGNH:
			cfg->alignH.isSet = qfalse;
			break;
		case SHUD_PROP_ALIGNV:
			cfg->alignV.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTALIGN:
			cfg->textAlign.isSet = qfalse;
			break;
		case SHUD_PROP_DIRECTION:
			cfg->direction.isSet = qfalse;
			break;
		case SHUD_PROP_COLOR:
			cfg->color.isSet = qfalse;
			break;
		case SHUD_PROP_BGCOLOR:
			cfg->bgcolor.isSet = qfalse;
			break;
		case SHUD_PROP_COLOR2:
			cfg->color2.isSet = qfalse;
			break;
		case SHUD_PROP_BGCOLOR2:
			cfg->bgcolor2.isSet = qfalse;
			break;
		case SHUD_PROP_BORDERC:
			cfg->borderColor.isSet = qfalse;
			break;
		case SHUD_PROP_BORDERC2:
			cfg->borderColor2.isSet = qfalse;
			break;
		case SHUD_PROP_FADE:
			cfg->fade.isSet = qfalse;
			break;
		case SHUD_PROP_SHADOW:
			cfg->shadowColor.isSet = qfalse;
			break;
		case SHUD_PROP_HLCOLOR:
			cfg->hlcolor.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGE:
			cfg->image.isSet = qfalse;
			break;
		case SHUD_PROP_TEXT:
			cfg->text.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qfalse;
			break;
		case SHUD_PROP_ITTEAM:
			cfg->itTeam.isSet = qfalse;
			break;
		case SHUD_PROP_TIME:
			cfg->time.isSet = qfalse;
			break;
		case SHUD_PROP_FADEDELAY:
			cfg->fadedelay.isSet = qfalse;
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qfalse;
			break;
		case SHUD_PROP_HLSIZE:
			cfg->hlsize.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qfalse;
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qfalse;
			break;
		case SHUD_PROP_STYLE:
			cfg->style.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTSTYLE:
			cfg->textStyle.isSet = qfalse;
			break;
		case SHUD_PROP_FILL:
			cfg->fill.isSet = qfalse;
			break;
		case SHUD_PROP_MONO:
			cfg->monospace.isSet = qfalse;
			break;
		case SHUD_PROP_DOUBLEBAR:
			cfg->doublebar.isSet = qfalse;
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			cfg->visflags.isSet = qfalse;
			break;
		default:
			break;
	}
	CG_SHUDEditorCommit(prop);
}

static void CG_SHUDEditorAdjustProp(int prop, int dir)
{
	superhudElement_t* el;
	superhudConfig_t* cfg;
	float step;
	int fontIndex;
	int fontCount;
	int i;
	int idx;
	const char* fontName;

	el = editor.selected;
	if (!el || dir == 0)
	{
		return;
	}
	if (!CG_SHUDEditorPropVisible(el, prop))
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	editor.coalesceProp = prop;
	cfg = &el->config;
	step = trap_Key_IsDown(K_SHIFT) ? 10.0f : 1.0f;

	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[0] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_Y:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[1] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_W:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[2] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_H:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[3] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FONT:
			fontCount = CG_GetFontCount();
			if (fontCount <= 0)
			{
				break;
			}
			fontIndex = cfg->font.isSet ? CG_FontIndexFromName(cfg->font.value) : 0;
			fontIndex = CG_SHUDEditorCycle(fontIndex, dir, fontCount);
			cfg->font.isSet = qtrue;
			fontName = CG_GetFontName(fontIndex);
			if (!fontName)
			{
				fontName = "";
			}
			Q_strncpyz(cfg->font.value, fontName, sizeof(cfg->font.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FS_W:
			CG_SHUDEditorEnsureFontSize(el);
			cfg->fontsize.value[0] += (float)dir * step;
			if (cfg->fontsize.value[0] < 1.0f)
			{
				cfg->fontsize.value[0] = 1.0f;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FS_H:
			CG_SHUDEditorEnsureFontSize(el);
			cfg->fontsize.value[1] += (float)dir * step;
			if (cfg->fontsize.value[1] < 1.0f)
			{
				cfg->fontsize.value[1] = 1.0f;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ALIGNH:
			if (!cfg->alignH.isSet)
			{
				cfg->alignH.isSet = qtrue;
				cfg->alignH.value = SUPERHUD_ALIGNH_LEFT;
			}
			else
			{
				cfg->alignH.value = (superhudAlignH_t)CG_SHUDEditorCycle((int)cfg->alignH.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ALIGNV:
			if (!cfg->alignV.isSet)
			{
				cfg->alignV.isSet = qtrue;
				cfg->alignV.value = SUPERHUD_ALIGNV_TOP;
			}
			else
			{
				cfg->alignV.value = (superhudAlignV_t)CG_SHUDEditorCycle((int)cfg->alignV.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTALIGN:
			if (!cfg->textAlign.isSet)
			{
				cfg->textAlign.isSet = qtrue;
				cfg->textAlign.value = SUPERHUD_ALIGNH_LEFT;
			}
			else
			{
				cfg->textAlign.value = (superhudAlignH_t)CG_SHUDEditorCycle((int)cfg->textAlign.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_DIRECTION:
			if (!cfg->direction.isSet)
			{
				cfg->direction.isSet = qtrue;
				cfg->direction.value = SUPERHUD_DIR_LEFT_TO_RIGHT;
			}
			else
			{
				cfg->direction.value = (superhudDirection_t)CG_SHUDEditorCycle((int)cfg->direction.value, dir, 4);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			CG_SHUDEditorCycleColorType(el, prop, dir);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FADE:
		case SHUD_PROP_SHADOW:
		case SHUD_PROP_HLCOLOR:
			CG_SHUDEditorEnsureColorProp(el, prop);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ITTEAM:
			if (!cfg->itTeam.isSet)
			{
				cfg->itTeam.isSet = qtrue;
				cfg->itTeam.value = SUPERHUD_ITTEAM_OWN;
			}
			else
			{
				cfg->itTeam.value = (superhudItTeam_t)CG_SHUDEditorCycle((int)cfg->itTeam.value, dir, 5);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TIME:
			if (!cfg->time.isSet)
			{
				cfg->time.isSet = qtrue;
				cfg->time.value = 0;
			}
			cfg->time.value += dir * (int)step * 100;
			if (cfg->time.value < 0)
			{
				cfg->time.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FADEDELAY:
			if (!cfg->fadedelay.isSet)
			{
				cfg->fadedelay.isSet = qtrue;
				cfg->fadedelay.value = 1000;
			}
			cfg->fadedelay.value += dir * (int)step * 50;
			if (cfg->fadedelay.value < 0)
			{
				cfg->fadedelay.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_HLSIZE:
			if (!cfg->hlsize.isSet)
			{
				cfg->hlsize.isSet = qtrue;
				cfg->hlsize.value = 1;
			}
			cfg->hlsize.value += dir * (int)step;
			if (cfg->hlsize.value < 0)
			{
				cfg->hlsize.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qtrue;
			cfg->border.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * (trap_Key_IsDown(K_SHIFT) ? 0.1f : 0.01f);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGE:
			idx = 0;
			if (cfg->image.isSet)
			{
				for (i = 0; i < SHUD_EDIT_IMG_MAX; i++)
				{
					if (!Q_stricmp(cfg->image.value, shudEditImages[i]))
					{
						idx = i;
						break;
					}
				}
			}
			idx = CG_SHUDEditorCycle(idx, dir, SHUD_EDIT_IMG_MAX);
			cfg->image.isSet = qtrue;
			Q_strncpyz(cfg->image.value, shudEditImages[idx], sizeof(cfg->image.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXT:
			idx = 0;
			if (cfg->text.isSet)
			{
				for (i = 0; i < SHUD_EDIT_TXT_MAX; i++)
				{
					if (!Q_stricmp(cfg->text.value, shudEditTexts[i]))
					{
						idx = i;
						break;
					}
				}
			}
			idx = CG_SHUDEditorCycle(idx, dir, SHUD_EDIT_TXT_MAX);
			cfg->text.isSet = qtrue;
			Q_strncpyz(cfg->text.value, shudEditTexts[idx], sizeof(cfg->text.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_STYLE:
			if (!cfg->style.isSet)
			{
				cfg->style.isSet = qtrue;
				cfg->style.value = 0;
			}
			else
			{
				cfg->style.value += dir;
				if (cfg->style.value < 0)
				{
					cfg->style.value = 0;
				}
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTSTYLE:
			if (!cfg->textStyle.isSet)
			{
				cfg->textStyle.isSet = qtrue;
				cfg->textStyle.value = 0;
			}
			else
			{
				cfg->textStyle.value = CG_SHUDEditorCycle(cfg->textStyle.value, dir, 2);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FILL:
			cfg->fill.isSet = cfg->fill.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_MONO:
			cfg->monospace.isSet = cfg->monospace.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_DOUBLEBAR:
			cfg->doublebar.isSet = cfg->doublebar.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			CG_SHUDEditorToggleVis(el, CG_SHUDEditorVisBit(prop));
			CG_SHUDEditorCommit(prop);
			break;
		default:
			break;
	}
}

static qboolean CG_SHUDEditorPropIsToggle(int prop)
{
	if (CG_SHUDEditorVisBit(prop))
	{
		return qtrue;
	}
	return (prop == SHUD_PROP_COLOR ||
	        prop == SHUD_PROP_BGCOLOR ||
	        prop == SHUD_PROP_COLOR2 ||
	        prop == SHUD_PROP_BGCOLOR2 ||
	        prop == SHUD_PROP_BORDERC ||
	        prop == SHUD_PROP_BORDERC2 ||
	        prop == SHUD_PROP_FADE ||
	        prop == SHUD_PROP_SHADOW ||
	        prop == SHUD_PROP_HLCOLOR ||
	        prop == SHUD_PROP_FONT ||
	        prop == SHUD_PROP_ALIGNH ||
	        prop == SHUD_PROP_ALIGNV ||
	        prop == SHUD_PROP_TEXTALIGN ||
	        prop == SHUD_PROP_DIRECTION ||
	        prop == SHUD_PROP_ITTEAM ||
	        prop == SHUD_PROP_IMAGE ||
	        prop == SHUD_PROP_TEXT ||
	        prop == SHUD_PROP_FILL ||
	        prop == SHUD_PROP_MONO ||
	        prop == SHUD_PROP_DOUBLEBAR ||
	        prop == SHUD_PROP_TEXTSTYLE);
}

static void CG_SHUDEditorClickProps(void)
{
	shudLay_t item;

	if (editor.cursorX >= SHUD_EDIT_PROPS_X + SHUD_EDIT_PROPS_W - 16.0f &&
	    editor.cursorY >= SHUD_EDIT_PROPS_Y + 2.0f &&
	    editor.cursorY <= SHUD_EDIT_PROPS_Y + 14.0f)
	{
		editor.showProps = qfalse;
		return;
	}
	if (CG_SHUDEditorHitLayout(&item) < 0)
	{
		return;
	}
	editor.propIndex = item.prop;
	if (item.kind == 1)
	{
		editor.sliderProp = item.prop;
		editor.sliderChannel = item.channel;
		CG_SHUDEditorPushSelConfigUndo(item.prop);
		editor.undoLock = qtrue;
		editor.drag = SHUD_DRAG_SLIDER;
		CG_SHUDEditorSliderFromCursor(item.prop, item.channel);
		return;
	}
	if (CG_SHUDEditorPropIsToggle(item.prop))
	{
		CG_SHUDEditorAdjustProp(item.prop, 1);
	}
}

static qboolean CG_SHUDEditorIsProtectedName(const char* name)
{
	if (!name || !name[0])
	{
		return qtrue;
	}
	return (Q_stricmp(name, "default") == 0 || Q_stricmp(name, "roamer") == 0) ? qtrue : qfalse;
}

static qboolean CG_SHUDEditorDoSave(const char* name, qboolean allowDefault)
{
	if (!allowDefault && CG_SHUDEditorIsProtectedName(name))
	{
		CG_Printf("^3SuperHUD: not overwriting default.cfg/roamer.cfg. Use ^2shudsave myhud\n");
		return qfalse;
	}
	if (!CG_SHUDSaveConfig(name))
	{
		return qfalse;
	}
	editor.dirty = qfalse;
	return qtrue;
}

static void CG_SHUDEditorCatchKeys(qboolean enable)
{
	int catcher;

	catcher = trap_Key_GetCatcher();
	if (enable)
	{
		trap_Key_SetCatcher(catcher | KEYCATCH_CGAME);
	}
	else
	{
		trap_Key_SetCatcher(catcher & ~KEYCATCH_CGAME);
	}
}

static void CG_SHUDEditorPreviewStop(void)
{
	if (!shudEditPreviewLive)
	{
		return;
	}
	shudEditPreviewLive = qfalse;
	cg.lowAmmoWarning = shudEditSavedAmmo;
	cgs.osp.shud.forceChat = shudEditSavedForceChat;
}

static void CG_SHUDEditorPreviewStart(void)
{
	if (shudEditPreviewLive)
	{
		return;
	}
	shudEditPreviewLive = qtrue;
	shudEditSavedAmmo = cg.lowAmmoWarning;
	shudEditSavedForceChat = cgs.osp.shud.forceChat;
	cgs.osp.shud.forceChat = qtrue;
}

void CG_SHUDEditorPumpPreview(void)
{
	superhudGlobalContext_t* ctx;
	superhudObituariesEntry_t* entry;
	int i;
	int n;
	qboolean anyChat;
	qboolean anyObit;
	static const char* dummyChat[8] =
	{
		"^2Player^7: nice shot",
		"^1Red^7: gg",
		"^4Blue^7: wait",
		"^3Spec^7: 1v1?",
		"^2Player^7: rail",
		"^5Bot^7: defending",
		"^1Red^7: mid",
		"^4Blue^7: incoming"
	};
	static const char* dummyAtk[8] =
	{
		"^1RedPlayer",
		"^4BluePlayer",
		"^2Green",
		"^3Yellow",
		"^1Alice",
		"^4Bob",
		"^2Carol",
		"^5Dave"
	};
	static const char* dummyTgt[8] =
	{
		"^4BluePlayer",
		"^1RedPlayer",
		"^3Yellow",
		"^2Green",
		"^4Bob",
		"^1Alice",
		"^5Dave",
		"^2Carol"
	};
	static const int dummyMod[8] =
	{
		MOD_RAILGUN,
		MOD_ROCKET,
		MOD_LIGHTNING,
		MOD_SHOTGUN,
		MOD_MACHINEGUN,
		MOD_PLASMA,
		MOD_GRENADE,
		MOD_GAUNTLET
	};

	if (!editor.active)
	{
		return;
	}
	ctx = CG_SHUDGetContext();
	if (!ctx)
	{
		return;
	}

	anyChat = qfalse;
	for (i = 0; i < SHUD_MAX_CHAT_LINES; i++)
	{
		if (ctx->chat.line[i].message[0])
		{
			ctx->chat.line[i].time = cg.time;
			anyChat = qtrue;
		}
	}
	if (!anyChat)
	{
		for (i = 0; i < 8; i++)
		{
			CG_SHUDEventChat(dummyChat[i], -1);
		}
	}

	anyObit = qfalse;
	for (i = 0; i < SHUD_MAX_OBITUARIES_LINES; i++)
	{
		if (ctx->obituaries.line[i].time)
		{
			ctx->obituaries.line[i].time = cg.time;
			anyObit = qtrue;
		}
	}
	if (!anyObit)
	{
		for (i = 0; i < SHUD_MAX_OBITUARIES_LINES; i++)
		{
			CG_SHUDEventObituaries(999, 998, dummyMod[i], qfalse);
			n = (int)((ctx->obituaries.index - 1) % SHUD_MAX_OBITUARIES_LINES);
			entry = &ctx->obituaries.line[n];
			entry->attackerTeam = TEAM_RED;
			entry->targetTeam = TEAM_BLUE;
			Q_strncpyz(entry->runtime.attackerName, dummyAtk[i], sizeof(entry->runtime.attackerName));
			Q_strncpyz(entry->runtime.targetName, dummyTgt[i], sizeof(entry->runtime.targetName));
			entry->runtime.isInitialized = qfalse;
		}
	}

	if (!ctx->fragmessage.message[0])
	{
		Q_strncpyz(ctx->fragmessage.message, "You fragged Player", sizeof(ctx->fragmessage.message));
	}
	ctx->fragmessage.time = cg.time;
	if (!ctx->rankmessage.message[0])
	{
		Q_strncpyz(ctx->rankmessage.message, "1st place with 0", sizeof(ctx->rankmessage.message));
	}
	ctx->rankmessage.time = cg.time;

	if (cg.itemPickup <= 0 || cg.itemPickup >= bg_numItems)
	{
		cg.itemPickup = 1;
	}
	CG_RegisterItemVisuals(cg.itemPickup);
	cg.itemPickupTime = cg.time;
	cg.attackerTime = cg.time;
	cg.crosshairClientTime = cg.time;
	if (!cg.rewardShader[0])
	{
		cg.rewardShader[0] = cgs.media.medalExcellent;
		cg.rewardCount[0] = 3;
		cg.rewardStack = 0;
	}
	cg.rewardTime = cg.time;
	cg.lowAmmoWarning = 1;
	CG_SHUDEventTempAccuracy(WP_LIGHTNING, 45.5f);
}

static void CG_SHUDEditorActivate(qboolean enable)
{
	if (enable)
	{
		if (!cg_shud.integer)
		{
			CG_Printf("^1SuperHUD editor: enable cg_shud first.\n");
			return;
		}
		if (!CG_SHUDGetElementsHead())
		{
			CG_Printf("^1SuperHUD editor: no HUD loaded.\n");
			return;
		}
		memset(&editor, 0, sizeof(editor));
		editor.active = qtrue;
		editor.showList = qtrue;
		editor.showProps = qtrue;
		editor.cursorX = SCREEN_WIDTH * 0.5f;
		editor.cursorY = SCREEN_HEIGHT * 0.5f;
		editor.gridSize = 16;
		editor.showGuides = qtrue;
		editor.coalesceProp = -1;
		editor.selectedIndex = -1;
		CG_SHUDAssignTabs(CG_SHUDGetElementsHead());
		editor.tab = 0;
		editor.showAll = qtrue;
		CG_SHUDEditorEnsureTabDefault(0);
		CG_SHUDEditorSelectIndex(1);
		CG_SHUDEditorCatchKeys(qtrue);
		CG_SHUDEditorPreviewStart();
		CG_Printf("^3SuperHUD editor: on  ^7F1 help, ESC exit, F5 save, N add, / filter\n");
	}
	else if (editor.active)
	{
		CG_SHUDEditorCatchKeys(qfalse);
		CG_SHUDEditorPreviewStop();
		if (editor.dirty)
		{
			CG_Printf("^3SuperHUD editor: off. Unsaved changes - ^2shudsave myhud^3 (F5 will not overwrite default.cfg/roamer.cfg)\n");
		}
		else
		{
			CG_Printf("^3SuperHUD editor: off.\n");
		}
		memset(&editor, 0, sizeof(editor));
	}
}

qboolean CG_SHUDEditorIsActive(void)
{
	return editor.active;
}

qboolean CG_SHUDEditorShowAll(void)
{
	return (editor.active && editor.showAll) ? qtrue : qfalse;
}

void CG_SHUDEditorToggle_f(void)
{
	if (editor.active)
	{
		CG_SHUDEditorActivate(qfalse);
	}
	else
	{
		CG_SHUDEditorActivate(qtrue);
	}
}

void CG_SHUDEditorSave_f(void)
{
	char name[MAX_QPATH];

	if (trap_Argc() >= 2)
	{
		Q_strncpyz(name, CG_Argv(1), sizeof(name));
		if (CG_SHUDEditorDoSave(name, qtrue))
		{
			if (Q_stricmp(ch_file.string, name))
			{
				trap_Cvar_Set("ch_file", name);
			}
		}
	}
	else
	{
		CG_SHUDEditorDoSave(ch_file.string, qfalse);
	}
}

void CG_SHUDEditorShutdown(void)
{
	if (editor.active)
	{
		CG_SHUDEditorCatchKeys(qfalse);
		CG_SHUDEditorPreviewStop();
	}
	memset(&editor, 0, sizeof(editor));
}

void CG_SHUDEditorOnConfigReloaded(void)
{
	if (!editor.active)
	{
		return;
	}
	editor.drag = SHUD_DRAG_NONE;
	editor.dirty = qfalse;
	editor.picker = qfalse;
	CG_SHUDEditorClearUndo();
	if (editor.selectedName[0])
	{
		CG_SHUDEditorSelect(CG_SHUDEditorFindByName(editor.selectedName));
	}
	else
	{
		CG_SHUDEditorSelectIndex(0);
	}
}

static void CG_SHUDEditorNudge(float dx, float dy, qboolean resize)
{
	vec4_t rect;
	int i;
	int n;
	int needPush;
	superhudElement_t* el;
	qboolean savedGuides;

	n = editor.selCount;
	if (n <= 0)
	{
		return;
	}
	needPush = (editor.coalesceProp != 1000) ? 1 : 0;
	if (needPush)
	{
		CG_SHUDEditorBeginUndoGroup();
	}
	savedGuides = editor.showGuides;
	editor.showGuides = qfalse;
	for (i = 0; i < n; i++)
	{
		el = editor.sel[i];
		if (!CG_SHUDEditorGetRect(el, rect))
		{
			continue;
		}
		if (needPush)
		{
			CG_SHUDEditorPushConfigUndo(el);
		}
		if (resize)
		{
			rect[2] += dx;
			rect[3] += dy;
		}
		else
		{
			rect[0] += dx;
			rect[1] += dy;
		}
		CG_SHUDEditorApplyRect(el, rect, qtrue);
	}
	editor.showGuides = savedGuides;
	if (needPush)
	{
		CG_SHUDEditorEndUndoGroup();
	}
	editor.coalesceProp = 1000;
}

static void CG_SHUDEditorSetTab(int tab)
{
	if (tab < 0)
	{
		tab = 0;
	}
	if (tab >= SHUD_TAB_COUNT)
	{
		tab = SHUD_TAB_COUNT - 1;
	}
	editor.tab = tab;
	editor.scroll = 0;
	editor.pickerScroll = 0;
	editor.pickerIndex = 0;
	CG_SHUDEditorEnsureTabDefault(tab);
}

static void CG_SHUDEditorClickPanel(void)
{
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];
	int row;
	int vis;
	int count;
	int tab;
	int col;
	int trow;
	float tw;

	vis = CG_SHUDEditorVisibleRows();
	if (editor.cursorX >= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W - 16.0f &&
	    editor.cursorY >= SHUD_EDIT_PANEL_Y + 2.0f &&
	    editor.cursorY <= SHUD_EDIT_PANEL_Y + 14.0f)
	{
		editor.showList = qfalse;
		return;
	}
	if (editor.cursorY >= SHUD_EDIT_TAB_Y && editor.cursorY < SHUD_EDIT_TAB_Y + SHUD_EDIT_TAB_H &&
	    editor.cursorX >= SHUD_EDIT_PANEL_X + 2.0f && editor.cursorX <= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W - 2.0f)
	{
		tw = (SHUD_EDIT_PANEL_W - 4.0f) / 4.0f;
		col = (int)((editor.cursorX - SHUD_EDIT_PANEL_X - 2.0f) / tw);
		trow = (int)((editor.cursorY - SHUD_EDIT_TAB_Y) / (SHUD_EDIT_TAB_H * 0.5f));
		if (col < 0)
		{
			col = 0;
		}
		if (col > 3)
		{
			col = 3;
		}
		if (trow < 0)
		{
			trow = 0;
		}
		if (trow > 1)
		{
			trow = 1;
		}
		tab = trow * 4 + col;
		if (tab >= 0 && tab < SHUD_TAB_COUNT)
		{
			CG_SHUDEditorSetTab(tab);
			CG_SHUDEditorSelectIndex(1);
		}
		return;
	}
	if (CG_SHUDEditorCursorInSearch())
	{
		CG_SHUDEditorStartTypeIn(editor.picker ? -2 : -1);
		return;
	}
	if (!editor.picker && editor.selected && !CG_SHUDEditorIsDefault(editor.selected) &&
	    editor.cursorY >= 422.0f && editor.cursorY < 434.0f &&
	    editor.cursorX >= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W - 40.0f)
	{
		CG_SHUDEditorDeleteSelected();
		return;
	}
	if (editor.cursorY < SHUD_EDIT_LIST_Y || editor.cursorY >= SHUD_EDIT_LIST_BOTTOM)
	{
		return;
	}
	row = (int)((editor.cursorY - SHUD_EDIT_LIST_Y) / SHUD_EDIT_ROW_H);
	if (row < 0 || row >= vis)
	{
		return;
	}
	if (editor.picker)
	{
		count = CG_SHUDEditorBuildTypeList(rows, SHUD_EDIT_ROW_MAX);
		row += editor.pickerScroll;
		if (row >= 0 && row < count && !rows[row].header && rows[row].def)
		{
			editor.pickerIndex = row;
			CG_SHUDEditorAddType(rows[row].def);
		}
		return;
	}
	count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	row += editor.scroll;
	if (row < 0 || row >= count || rows[row].header)
	{
		return;
	}
	if (rows[row].el)
	{
		if (trap_Key_IsDown(K_SHIFT) && !CG_SHUDEditorIsDefault(rows[row].el))
		{
			if (CG_SHUDEditorSelHas(rows[row].el))
			{
				CG_SHUDEditorSelRemove(rows[row].el);
				if (editor.selected == rows[row].el)
				{
					CG_SHUDEditorSetPrimary(editor.selCount > 0 ? editor.sel[0] : NULL);
				}
			}
			else
			{
				CG_SHUDEditorSelAdd(rows[row].el);
				CG_SHUDEditorSetPrimary(rows[row].el);
			}
			return;
		}
		CG_SHUDEditorSelect(rows[row].el);
		return;
	}
	if (rows[row].title && !Q_stricmp(rows[row].title, "DEFAULT"))
	{
		CG_SHUDEditorSelect(CG_SHUDEditorEnsureTabDefault(editor.tab));
	}
}

static qboolean CG_SHUDEditorRectsOverlap(const vec4_t a, const vec4_t b)
{
	return (a[0] < b[0] + b[2] && b[0] < a[0] + a[2] &&
	        a[1] < b[1] + b[3] && b[1] < a[1] + a[3]) ? qtrue : qfalse;
}

static void CG_SHUDEditorMarqueeRect(vec4_t out)
{
	float x0;
	float y0;
	float x1;
	float y1;

	x0 = editor.dragGrabX;
	y0 = editor.dragGrabY;
	x1 = editor.cursorX;
	y1 = editor.cursorY;
	if (x1 < x0)
	{
		out[0] = x1;
		out[2] = x0 - x1;
	}
	else
	{
		out[0] = x0;
		out[2] = x1 - x0;
	}
	if (y1 < y0)
	{
		out[1] = y1;
		out[3] = y0 - y1;
	}
	else
	{
		out[1] = y0;
		out[3] = y1 - y0;
	}
}

static void CG_SHUDEditorFinishMarquee(void)
{
	superhudElement_t* items[SHUD_EDIT_MAX_ITEMS];
	vec4_t box;
	vec4_t rect;
	int count;
	int i;

	CG_SHUDEditorMarqueeRect(box);
	if (box[2] < 4.0f && box[3] < 4.0f)
	{
		if (!editor.marqueeAdd)
		{
			CG_SHUDEditorSelect(NULL);
		}
		return;
	}
	if (!editor.marqueeAdd)
	{
		editor.selCount = 0;
		editor.selected = NULL;
		editor.selectedName[0] = 0;
		editor.selectedIndex = -1;
	}
	count = CG_SHUDEditorCollect(items, SHUD_EDIT_MAX_ITEMS);
	for (i = 0; i < count; i++)
	{
		if (CG_SHUDEditorIsGhost(items[i]) || CG_SHUDEditorHideBox(items[i]))
		{
			continue;
		}
		if (!CG_SHUDEditorGetRect(items[i], rect))
		{
			continue;
		}
		if (CG_SHUDEditorRectsOverlap(box, rect))
		{
			CG_SHUDEditorSelAdd(items[i]);
		}
	}
	if (editor.selCount > 0)
	{
		CG_SHUDEditorSetPrimary(editor.sel[editor.selCount - 1]);
	}
	else
	{
		CG_SHUDEditorSetPrimary(NULL);
	}
}

static void CG_SHUDEditorStartGroupMove(void)
{
	int i;

	editor.dragGrabX = editor.cursorX;
	editor.dragGrabY = editor.cursorY;
	for (i = 0; i < editor.selCount; i++)
	{
		if (!CG_SHUDEditorGetRect(editor.sel[i], editor.dragStartRects[i]))
		{
			Vector4Set(editor.dragStartRects[i], 0, 0, 1, 1);
		}
		if (editor.sel[i] == editor.selected)
		{
			Vector4Copy(editor.dragStartRects[i], editor.dragStartRect);
		}
	}
	editor.dragMoved = qfalse;
	editor.coalesceProp = -1;
	editor.drag = SHUD_DRAG_MOVE;
}

static void CG_SHUDEditorMouseDown(void)
{
	vec4_t rect;
	shudEditorDrag_t handle;
	superhudElement_t* hit;

	editor.mouse1 = qtrue;
	if (!editor.showList && editor.cursorX <= 18.0f && editor.cursorY <= 28.0f)
	{
		editor.showList = qtrue;
		return;
	}
	if (!editor.showProps && editor.cursorX >= SCREEN_WIDTH - 18.0f && editor.cursorY <= 28.0f)
	{
		editor.showProps = qtrue;
		return;
	}
	if (editor.pipette && editor.selected)
	{
		hit = CG_SHUDEditorHitElement(editor.cursorX, editor.cursorY);
		if (hit && hit != editor.selected)
		{
			CG_SHUDEditorPushSelConfigUndo(SHUD_PROP_COLOR);
			editor.coalesceProp = -1;
			if (hit->config.color.isSet)
			{
				editor.selected->config.color = hit->config.color;
			}
			CG_SHUDEditorCommit(SHUD_PROP_COLOR);
			editor.pipette = qfalse;
			CG_Printf("^3SuperHUD editor: color copied from %s\n",
			          hit->element.name ? hit->element.name : "?");
		}
		else
		{
			editor.pipette = qfalse;
		}
		return;
	}
	if (CG_SHUDEditorCursorInPanel())
	{
		CG_SHUDEditorClickPanel();
		return;
	}
	if (CG_SHUDEditorCursorInProps())
	{
		CG_SHUDEditorClickProps();
		return;
	}

	if (CG_SHUDEditorGetRect(editor.selected, rect))
	{
		handle = CG_SHUDEditorHitHandle(editor.cursorX, editor.cursorY, rect);
		if (handle != SHUD_DRAG_NONE)
		{
			CG_SHUDEditorPushConfigUndo(editor.selected);
			editor.coalesceProp = -1;
			editor.undoLock = qtrue;
			editor.drag = handle;
			editor.dragGrabX = editor.cursorX;
			editor.dragGrabY = editor.cursorY;
			Vector4Copy(rect, editor.dragStartRect);
			return;
		}
	}

	hit = CG_SHUDEditorHitElement(editor.cursorX, editor.cursorY);
	if (hit)
	{
		if (trap_Key_IsDown(K_SHIFT) && !CG_SHUDEditorIsDefault(hit))
		{
			if (CG_SHUDEditorSelHas(hit))
			{
				CG_SHUDEditorSelRemove(hit);
				if (editor.selected == hit)
				{
					CG_SHUDEditorSetPrimary(editor.selCount > 0 ? editor.sel[0] : NULL);
				}
			}
			else
			{
				CG_SHUDEditorSelAdd(hit);
				CG_SHUDEditorSetPrimary(hit);
			}
			return;
		}
		if (!CG_SHUDEditorSelHas(hit))
		{
			CG_SHUDEditorSelect(hit);
		}
		else
		{
			CG_SHUDEditorSetPrimary(hit);
		}
		if (CG_SHUDEditorGetRect(hit, rect))
		{
			CG_SHUDEditorStartGroupMove();
		}
		return;
	}

	editor.drag = SHUD_DRAG_MARQUEE;
	editor.marqueeAdd = trap_Key_IsDown(K_SHIFT) ? qtrue : qfalse;
	editor.dragGrabX = editor.cursorX;
	editor.dragGrabY = editor.cursorY;
}

static void CG_SHUDEditorMouseUp(void)
{
	vec4_t rect;
	int i;
	qboolean savedGuides;
	qboolean savedGrid;

	if (editor.drag == SHUD_DRAG_SLIDER)
	{
		editor.mouse1 = qfalse;
		editor.drag = SHUD_DRAG_NONE;
		editor.undoLock = qfalse;
		return;
	}
	if (editor.drag == SHUD_DRAG_MARQUEE)
	{
		CG_SHUDEditorFinishMarquee();
		editor.mouse1 = qfalse;
		editor.drag = SHUD_DRAG_NONE;
		editor.undoLock = qfalse;
		return;
	}
	if (editor.drag == SHUD_DRAG_MOVE && !editor.dragMoved)
	{
		editor.mouse1 = qfalse;
		editor.drag = SHUD_DRAG_NONE;
		editor.undoLock = qfalse;
		return;
	}
	if (editor.drag == SHUD_DRAG_MOVE && editor.selCount > 0)
	{
		savedGuides = editor.showGuides;
		savedGrid = editor.snapGrid;
		for (i = 0; i < editor.selCount; i++)
		{
			if (!CG_SHUDEditorGetRect(editor.sel[i], rect))
			{
				continue;
			}
			if (editor.sel[i] != editor.selected)
			{
				editor.showGuides = qfalse;
				editor.snapGrid = qfalse;
			}
			CG_SHUDEditorApplyRect(editor.sel[i], rect, qtrue);
			editor.showGuides = savedGuides;
			editor.snapGrid = savedGrid;
		}
	}
	else if (editor.drag != SHUD_DRAG_NONE && CG_SHUDEditorGetRect(editor.selected, rect))
	{
		CG_SHUDEditorApplyRect(editor.selected, rect, qtrue);
	}
	editor.mouse1 = qfalse;
	editor.drag = SHUD_DRAG_NONE;
	editor.undoLock = qfalse;
}

static void CG_SHUDEditorApplyDrag(void)
{
	vec4_t rect;
	float x2;
	float y2;
	float ar;
	float fx2;
	float fy2;
	float dx;
	float dy;
	int i;
	int pi;
	qboolean savedGuides;
	qboolean savedGrid;

	if (editor.drag == SHUD_DRAG_NONE)
	{
		return;
	}
	if (editor.drag == SHUD_DRAG_MARQUEE)
	{
		return;
	}
	if (editor.drag == SHUD_DRAG_SLIDER)
	{
		CG_SHUDEditorSliderFromCursor(editor.sliderProp, editor.sliderChannel);
		return;
	}
	if (!editor.selected)
	{
		return;
	}

	if (editor.drag == SHUD_DRAG_MOVE && editor.selCount > 0)
	{
		dx = editor.cursorX - editor.dragGrabX;
		dy = editor.cursorY - editor.dragGrabY;
		if (!editor.dragMoved)
		{
			if (dx < 0.0f)
			{
				dx = -dx;
			}
			if (dy < 0.0f)
			{
				dy = -dy;
			}
			if (dx < SHUD_EDIT_DRAG_MIN && dy < SHUD_EDIT_DRAG_MIN)
			{
				return;
			}
			editor.dragMoved = qtrue;
			CG_SHUDEditorBeginUndoGroup();
			for (i = 0; i < editor.selCount; i++)
			{
				CG_SHUDEditorPushConfigUndo(editor.sel[i]);
			}
			CG_SHUDEditorEndUndoGroup();
			editor.coalesceProp = -1;
			editor.undoLock = qtrue;
		}
		pi = 0;
		for (i = 0; i < editor.selCount; i++)
		{
			if (editor.sel[i] == editor.selected)
			{
				pi = i;
			}
		}
		Vector4Copy(editor.dragStartRects[pi], rect);
		rect[0] += editor.cursorX - editor.dragGrabX;
		rect[1] += editor.cursorY - editor.dragGrabY;
		CG_SHUDEditorApplyRect(editor.selected, rect, qfalse);
		if (!CG_SHUDEditorGetRect(editor.selected, rect))
		{
			return;
		}
		dx = rect[0] - editor.dragStartRects[pi][0];
		dy = rect[1] - editor.dragStartRects[pi][1];
		savedGuides = editor.showGuides;
		savedGrid = editor.snapGrid;
		editor.showGuides = qfalse;
		editor.snapGrid = qfalse;
		for (i = 0; i < editor.selCount; i++)
		{
			if (editor.sel[i] == editor.selected)
			{
				continue;
			}
			Vector4Copy(editor.dragStartRects[i], rect);
			rect[0] += dx;
			rect[1] += dy;
			CG_SHUDEditorApplyRect(editor.sel[i], rect, qfalse);
		}
		editor.showGuides = savedGuides;
		editor.snapGrid = savedGrid;
		return;
	}

	Vector4Copy(editor.dragStartRect, rect);
	if (editor.drag == SHUD_DRAG_MOVE)
	{
		rect[0] = editor.dragStartRect[0] + editor.cursorX - editor.dragGrabX;
		rect[1] = editor.dragStartRect[1] + editor.cursorY - editor.dragGrabY;
	}
	else if (editor.drag == SHUD_DRAG_RESIZE_BR)
	{
		rect[2] = editor.cursorX - rect[0];
		rect[3] = editor.cursorY - rect[1];
	}
	else if (editor.drag == SHUD_DRAG_RESIZE_TR)
	{
		y2 = rect[1] + rect[3];
		rect[2] = editor.cursorX - rect[0];
		rect[1] = editor.cursorY;
		rect[3] = y2 - rect[1];
	}
	else if (editor.drag == SHUD_DRAG_RESIZE_BL)
	{
		x2 = rect[0] + rect[2];
		rect[0] = editor.cursorX;
		rect[2] = x2 - rect[0];
		rect[3] = editor.cursorY - rect[1];
	}
	else if (editor.drag == SHUD_DRAG_RESIZE_TL)
	{
		x2 = rect[0] + rect[2];
		y2 = rect[1] + rect[3];
		rect[0] = editor.cursorX;
		rect[1] = editor.cursorY;
		rect[2] = x2 - rect[0];
		rect[3] = y2 - rect[1];
	}
	if (trap_Key_IsDown(K_SHIFT) && editor.drag != SHUD_DRAG_MOVE && editor.dragStartRect[3] > 0.01f)
	{
		ar = editor.dragStartRect[2] / editor.dragStartRect[3];
		fx2 = editor.dragStartRect[0] + editor.dragStartRect[2];
		fy2 = editor.dragStartRect[1] + editor.dragStartRect[3];
		rect[3] = rect[2] / ar;
		if (editor.drag == SHUD_DRAG_RESIZE_TL || editor.drag == SHUD_DRAG_RESIZE_BL)
		{
			rect[0] = fx2 - rect[2];
		}
		if (editor.drag == SHUD_DRAG_RESIZE_TL || editor.drag == SHUD_DRAG_RESIZE_TR)
		{
			rect[1] = fy2 - rect[3];
		}
	}
	CG_SHUDEditorApplyRect(editor.selected, rect, qfalse);
}

void CG_SHUDEditorMouseEvent(int dx, int dy)
{
	float scaleX;
	float scaleY;

	if (!editor.active)
	{
		return;
	}
	scaleX = cgs.screenXScale_Old > 0.0f ? cgs.screenXScale_Old : 1.0f;
	scaleY = cgs.screenYScale_Old > 0.0f ? cgs.screenYScale_Old : 1.0f;
	editor.cursorX += ((float)dx) / scaleX;
	editor.cursorY += ((float)dy) / scaleY;
	if (editor.cursorX < 0.0f)
	{
		editor.cursorX = 0.0f;
	}
	if (editor.cursorY < 0.0f)
	{
		editor.cursorY = 0.0f;
	}
	if (editor.cursorX > SCREEN_WIDTH)
	{
		editor.cursorX = SCREEN_WIDTH;
	}
	if (editor.cursorY > SCREEN_HEIGHT)
	{
		editor.cursorY = SCREEN_HEIGHT;
	}
	if (editor.mouse1 && editor.drag != SHUD_DRAG_NONE)
	{
		CG_SHUDEditorApplyDrag();
	}
}

void CG_SHUDEditorKeyEvent(int key, qboolean down)
{
	int step;
	int vis;
	int count;
	shudLay_t lay;
	float* rgba;
	float t;
	vec4_t arect;
	float cx;
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];

	if (!editor.active)
	{
		return;
	}
	if (trap_Key_GetCatcher() & KEYCATCH_CONSOLE)
	{
		return;
	}

	if (key == K_MOUSE1)
	{
		if (down)
		{
			CG_SHUDEditorMouseDown();
		}
		else
		{
			CG_SHUDEditorMouseUp();
		}
		return;
	}

	if (!down)
	{
		return;
	}

	if (editor.typing)
	{
		CG_SHUDEditorTypeKey(key);
		return;
	}

	if (editor.help)
	{
		if (key == K_F1 || key == K_ESCAPE)
		{
			editor.help = qfalse;
		}
		return;
	}

	if (key != K_DEL)
	{
		editor.deleteArmed = 0;
	}

	if (editor.picker)
	{
		count = CG_SHUDEditorBuildTypeList(rows, SHUD_EDIT_ROW_MAX);
	}
	else
	{
		count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	}
	vis = CG_SHUDEditorVisibleRows();
	if (editor.snapGrid && editor.showGrid && !trap_Key_IsDown(K_ALT))
	{
		step = trap_Key_IsDown(K_SHIFT) ? editor.gridSize * 2 : editor.gridSize;
		if (step < 1)
		{
			step = 16;
		}
	}
	else
	{
		step = trap_Key_IsDown(K_SHIFT) ? 16 : 1;
	}

	if (key == K_ESCAPE)
	{
		if (editor.picker)
		{
			editor.picker = qfalse;
			editor.pickerFilter[0] = 0;
			return;
		}
		if (editor.listFilter[0])
		{
			editor.listFilter[0] = 0;
			editor.filterOn = qfalse;
			return;
		}
		CG_SHUDEditorActivate(qfalse);
		return;
	}
	if (key == K_F1)
	{
		editor.help = qtrue;
		return;
	}
	if (key >= '1' && key <= '8')
	{
		CG_SHUDEditorSetTab(key - '1');
		CG_SHUDEditorSelectIndex(1);
		return;
	}
	if (key == K_F3)
	{
		if (editor.gridSize <= 4)
		{
			editor.gridSize = 8;
		}
		else if (editor.gridSize <= 8)
		{
			editor.gridSize = 16;
		}
		else if (editor.gridSize <= 16)
		{
			editor.gridSize = 32;
		}
		else
		{
			editor.gridSize = 4;
		}
		CG_Printf("^3SuperHUD editor: grid %d\n", editor.gridSize);
		return;
	}
	if (key == K_F5 || ((key == 's' || key == 'S') && trap_Key_IsDown(K_CTRL)))
	{
		CG_SHUDEditorDoSave(ch_file.string, qfalse);
		return;
	}
	if ((key == 'y' || key == 'Y') && trap_Key_IsDown(K_CTRL))
	{
		CG_SHUDEditorRedo();
		return;
	}
	if ((key == 'z' || key == 'Z') && trap_Key_IsDown(K_CTRL))
	{
		CG_SHUDEditorUndo();
		return;
	}
	if ((key == 'c' || key == 'C') && trap_Key_IsDown(K_CTRL))
	{
		CG_SHUDEditorCopy();
		return;
	}
	if ((key == 'v' || key == 'V') && trap_Key_IsDown(K_CTRL))
	{
		CG_SHUDEditorPaste();
		return;
	}
	if (key == K_ENTER)
	{
		if (editor.picker)
		{
			return;
		}
		CG_SHUDEditorStartTypeIn(editor.propIndex);
		return;
	}
	if (key == '/')
	{
		CG_SHUDEditorStartTypeIn(editor.picker ? -2 : -1);
		return;
	}
	if (key == 'n' || key == 'N' || key == K_INS)
	{
		editor.picker = editor.picker ? qfalse : qtrue;
		editor.pickerScroll = 0;
		return;
	}
	if (key == 'd' || key == 'D')
	{
		CG_SHUDEditorDuplicate();
		return;
	}
	if (key == K_DEL)
	{
		CG_SHUDEditorDeleteSelected();
		return;
	}
	if (key == K_BACKSPACE)
	{
		CG_SHUDEditorUnsetProp(editor.propIndex);
		return;
	}
	if (key == K_TAB)
	{
		if (trap_Key_IsDown(K_SHIFT))
		{
			CG_SHUDEditorSelectIndex(editor.selectedIndex - 1);
		}
		else
		{
			CG_SHUDEditorSelectIndex(editor.selectedIndex + 1);
		}
		return;
	}
	if (key == 'p' || key == 'P')
	{
		editor.showList = editor.showList ? qfalse : qtrue;
		return;
	}
	if (key == 'o' || key == 'O')
	{
		editor.showProps = editor.showProps ? qfalse : qtrue;
		return;
	}
	if (key == 'g' || key == 'G')
	{
		if (!editor.showGrid)
		{
			editor.showGrid = qtrue;
			editor.snapGrid = qfalse;
			CG_Printf("^3SuperHUD editor: grid draw\n");
		}
		else if (!editor.snapGrid)
		{
			editor.snapGrid = qtrue;
			CG_Printf("^3SuperHUD editor: grid snap %d\n", editor.gridSize);
		}
		else
		{
			editor.showGrid = qfalse;
			editor.snapGrid = qfalse;
			CG_Printf("^3SuperHUD editor: grid off\n");
		}
		return;
	}
	if (key == 'c' || key == 'C')
	{
		editor.showGuides = editor.showGuides ? qfalse : qtrue;
		return;
	}
	if (key == 'a' || key == 'A')
	{
		if (CG_SHUDEditorGetRect(editor.selected, arect))
		{
			cx = arect[0] + arect[2] * 0.5f;
			if (cx < SCREEN_WIDTH / 3.0f)
			{
				CG_SHUDEditorAlignScreen(1);
			}
			else if (cx < SCREEN_WIDTH * 2.0f / 3.0f)
			{
				CG_SHUDEditorAlignScreen(2);
			}
			else
			{
				CG_SHUDEditorAlignScreen(0);
			}
		}
		return;
	}
	if (key == 'v' || key == 'V')
	{
		if (CG_SHUDEditorGetRect(editor.selected, arect))
		{
			cx = arect[1] + arect[3] * 0.5f;
			if (cx < SCREEN_HEIGHT / 3.0f)
			{
				CG_SHUDEditorAlignScreen(4);
			}
			else if (cx < SCREEN_HEIGHT * 2.0f / 3.0f)
			{
				CG_SHUDEditorAlignScreen(5);
			}
			else
			{
				CG_SHUDEditorAlignScreen(3);
			}
		}
		return;
	}
	if (key == 'm' || key == 'M')
	{
		CG_SHUDEditorMirrorH();
		return;
	}
	if (key == 'f' || key == 'F')
	{
		editor.showAll = editor.showAll ? qfalse : qtrue;
		CG_Printf("^3SuperHUD editor: force-show %s\n", editor.showAll ? "on" : "off");
		return;
	}
	if (key == 'b' || key == 'B')
	{
		editor.hideOtherBoxes = editor.hideOtherBoxes ? qfalse : qtrue;
		CG_Printf("^3SuperHUD editor: hide other boxes %s\n", editor.hideOtherBoxes ? "on" : "off");
		return;
	}
	if (key == 'h' || key == 'H')
	{
		CG_SHUDEditorToggleGhost(editor.selected);
		return;
	}
	if (key == 'i' || key == 'I')
	{
		editor.pipette = editor.pipette ? qfalse : qtrue;
		CG_Printf("^3SuperHUD editor: pipette %s\n", editor.pipette ? "click source" : "off");
		return;
	}
	if (key == '[' || key == '-')
	{
		CG_SHUDEditorAdjustProp(editor.propIndex, -1);
		return;
	}
	if (key == ']' || key == '=' || key == '+')
	{
		CG_SHUDEditorAdjustProp(editor.propIndex, 1);
		return;
	}
	if (key == K_MWHEELDOWN)
	{
		if (CG_SHUDEditorCursorInProps())
		{
			if (trap_Key_IsDown(K_CTRL))
			{
				editor.propScroll += 2;
				return;
			}
			if (CG_SHUDEditorHitLayout(&lay) >= 0)
			{
				editor.propIndex = lay.prop;
				if (lay.kind == 1)
				{
					CG_SHUDEditorPushSelConfigUndo(lay.prop);
					editor.coalesceProp = lay.prop;
					CG_SHUDEditorEnsureColorProp(editor.selected, lay.prop);
					rgba = CG_SHUDEditorRgbaPtr(editor.selected, lay.prop);
					t = rgba ? rgba[lay.channel] : 0.0f;
					t -= trap_Key_IsDown(K_SHIFT) ? 0.1f : 0.05f;
					CG_SHUDEditorSetSlider(lay.prop, lay.channel, t);
					return;
				}
			}
			CG_SHUDEditorAdjustProp(editor.propIndex, -1);
			return;
		}
		if (editor.picker)
		{
			editor.pickerScroll += 1;
			return;
		}
	}
	if (key == K_MWHEELDOWN || key == K_PGDN)
	{
		if (editor.picker)
		{
			editor.pickerScroll += (key == K_PGDN) ? vis : 1;
			return;
		}
		editor.scroll += (key == K_PGDN) ? vis : 1;
		if (editor.scroll > count - vis)
		{
			editor.scroll = count - vis;
		}
		if (editor.scroll < 0)
		{
			editor.scroll = 0;
		}
		return;
	}
	if (key == K_MWHEELUP)
	{
		if (CG_SHUDEditorCursorInProps())
		{
			if (trap_Key_IsDown(K_CTRL))
			{
				editor.propScroll -= 2;
				if (editor.propScroll < 0)
				{
					editor.propScroll = 0;
				}
				return;
			}
			if (CG_SHUDEditorHitLayout(&lay) >= 0)
			{
				editor.propIndex = lay.prop;
				if (lay.kind == 1)
				{
					CG_SHUDEditorPushSelConfigUndo(lay.prop);
					editor.coalesceProp = lay.prop;
					CG_SHUDEditorEnsureColorProp(editor.selected, lay.prop);
					rgba = CG_SHUDEditorRgbaPtr(editor.selected, lay.prop);
					t = rgba ? rgba[lay.channel] : 0.0f;
					t += trap_Key_IsDown(K_SHIFT) ? 0.1f : 0.05f;
					CG_SHUDEditorSetSlider(lay.prop, lay.channel, t);
					return;
				}
			}
			CG_SHUDEditorAdjustProp(editor.propIndex, 1);
			return;
		}
		if (editor.picker)
		{
			editor.pickerScroll -= 1;
			if (editor.pickerScroll < 0)
			{
				editor.pickerScroll = 0;
			}
			return;
		}
	}
	if (key == K_MWHEELUP || key == K_PGUP)
	{
		if (editor.picker)
		{
			editor.pickerScroll -= (key == K_PGUP) ? vis : 1;
			if (editor.pickerScroll < 0)
			{
				editor.pickerScroll = 0;
			}
			return;
		}
		editor.scroll -= (key == K_PGUP) ? vis : 1;
		if (editor.scroll < 0)
		{
			editor.scroll = 0;
		}
		return;
	}
	if (key == K_HOME)
	{
		CG_SHUDEditorSelectIndex(0);
		return;
	}
	if (key == K_END)
	{
		CG_SHUDEditorSelectIndex(count - 1);
		return;
	}
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(0);
			return;
		}
		CG_SHUDEditorNudge((float)(-step), 0, trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(2);
			return;
		}
		CG_SHUDEditorNudge((float)step, 0, trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_UPARROW || key == K_KP_UPARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(3);
			return;
		}
		CG_SHUDEditorNudge(0, (float)(-step), trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_DOWNARROW || key == K_KP_DOWNARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(5);
			return;
		}
		CG_SHUDEditorNudge(0, (float)step, trap_Key_IsDown(K_ALT));
		return;
	}
}

static void CG_SHUDEditorDrawHandle(float x, float y)
{
	float half;

	half = SHUD_EDIT_HANDLE * 0.5f;
	CG_FillRect(x - half, y - half, SHUD_EDIT_HANDLE, SHUD_EDIT_HANDLE, shudEditHandle);
}

static void CG_SHUDEditorDrawElementBox(superhudElement_t* el, qboolean selected, qboolean hover)
{
	vec4_t rect;
	const float* color;

	if (!CG_SHUDEditorGetRect(el, rect))
	{
		return;
	}
	if (el == editor.selected)
	{
		color = shudEditSelected;
	}
	else if (selected)
	{
		color = shudEditSelMulti;
	}
	else if (hover)
	{
		color = shudEditHover;
	}
	else if (CG_SHUDElementIsHidden(el))
	{
		color = shudEditOutlineHidden;
	}
	else
	{
		color = shudEditOutline;
	}
	CG_DrawRect(rect[0], rect[1], rect[2], rect[3], selected ? 1.5f : 1.0f, color);
	if (selected && el == editor.selected)
	{
		CG_SHUDEditorDrawHandle(rect[0], rect[1]);
		CG_SHUDEditorDrawHandle(rect[0] + rect[2], rect[1]);
		CG_SHUDEditorDrawHandle(rect[0], rect[1] + rect[3]);
		CG_SHUDEditorDrawHandle(rect[0] + rect[2], rect[1] + rect[3]);
	}
}

static void CG_SHUDEditorDrawGrid(void)
{
	int i;
	int g;

	if (!editor.showGrid)
	{
		return;
	}
	g = editor.gridSize > 0 ? editor.gridSize : 16;
	for (i = 0; i <= SCREEN_WIDTH; i += g)
	{
		CG_FillRect((float)i, 0, 1, SCREEN_HEIGHT, shudEditGrid);
	}
	for (i = 0; i <= SCREEN_HEIGHT; i += g)
	{
		CG_FillRect(0, (float)i, SCREEN_WIDTH, 1, shudEditGrid);
	}
}

static void CG_SHUDEditorDrawCursor(void)
{
	CG_FillRect(editor.cursorX - 6.0f, editor.cursorY, 13.0f, 1.0f, shudEditCursor);
	CG_FillRect(editor.cursorX, editor.cursorY - 6.0f, 1.0f, 13.0f, shudEditCursor);
	CG_FillRect(editor.cursorX - 1.5f, editor.cursorY - 1.5f, 3.0f, 3.0f, colorWhite);
}

static void CG_SHUDEditorPropValue(const superhudElement_t* el, int prop, char* out, int outSize)
{
	const superhudConfig_t* cfg;
	int flags;
	static const char* hnames[3] = { "L", "C", "R" };
	static const char* vnames[3] = { "T", "C", "B" };

	if (!out || outSize < 1)
	{
		return;
	}
	out[0] = 0;
	if (!el)
	{
		return;
	}
	cfg = &el->config;

	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[0]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_Y:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[1]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_W:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[2]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_H:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[3]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FONT:
			if (cfg->font.isSet)
			{
				Q_strncpyz(out, cfg->font.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FS_W:
			if (cfg->fontsize.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->fontsize.value[0]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FS_H:
			if (cfg->fontsize.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->fontsize.value[1]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_ALIGNH:
			if (cfg->alignH.isSet && (int)cfg->alignH.value >= 0 && (int)cfg->alignH.value < 3)
			{
				Q_strncpyz(out, hnames[cfg->alignH.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_ALIGNV:
			if (cfg->alignV.isSet && (int)cfg->alignV.value >= 0 && (int)cfg->alignV.value < 3)
			{
				Q_strncpyz(out, vnames[cfg->alignV.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTALIGN:
			if (cfg->textAlign.isSet && (int)cfg->textAlign.value >= 0 && (int)cfg->textAlign.value < 3)
			{
				Q_strncpyz(out, hnames[cfg->textAlign.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			{
				const superhudColor_t* c;
				qboolean set;
				c = CG_SHUDEditorColorPtr((superhudElement_t*)el, prop);
				set = *CG_SHUDEditorColorSetPtr((superhudElement_t*)el, prop);
				if (!set)
				{
					Q_strncpyz(out, "--", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_T)
				{
					Q_strncpyz(out, "T", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_E)
				{
					Q_strncpyz(out, "E", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_I)
				{
					Q_strncpyz(out, "I", outSize);
				}
				else
				{
					Q_strncpyz(out, "RGBA", outSize);
				}
			}
			break;
		case SHUD_PROP_FADE:
		case SHUD_PROP_SHADOW:
		case SHUD_PROP_HLCOLOR:
			{
				qboolean* sp;
				sp = CG_SHUDEditorVec4SetPtr((superhudElement_t*)el, prop);
				Q_strncpyz(out, (sp && *sp) ? "RGBA" : "--", outSize);
			}
			break;
		case SHUD_PROP_DIRECTION:
			if (!cfg->direction.isSet)
			{
				Q_strncpyz(out, "--", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_RIGHT_TO_LEFT)
			{
				Q_strncpyz(out, "R", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_TOP_TO_BOTTOM)
			{
				Q_strncpyz(out, "T", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_BOTTOM_TO_TOP)
			{
				Q_strncpyz(out, "B", outSize);
			}
			else
			{
				Q_strncpyz(out, "L", outSize);
			}
			break;
		case SHUD_PROP_IMAGE:
			if (cfg->image.isSet)
			{
				Q_strncpyz(out, cfg->image.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXT:
			if (cfg->text.isSet)
			{
				Q_strncpyz(out, cfg->text.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_STYLE:
			if (cfg->style.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->style.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTSTYLE:
			if (cfg->textStyle.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->textStyle.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FILL:
			Q_strncpyz(out, cfg->fill.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_MONO:
			Q_strncpyz(out, cfg->monospace.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_DOUBLEBAR:
			Q_strncpyz(out, cfg->doublebar.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_IM:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_IM) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SPECT:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SPECT) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_DEAD:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_DEAD) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SCOREHIDE:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SCORES_HIDE) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SHOWEMPTY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SHOW_EMPTY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_TEAMONLY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_TEAM_ONLY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_DEMOHIDE:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_DEMO_HIDE) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY1:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY1_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY2:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY2_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY3:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY3_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY4:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY4_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTFFA:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_FFA) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTTOURNEY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_TOURNEY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTTDM:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_TDM) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTCTF:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_CTF) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTFT:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_FREEZETAG) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTCA:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_CLANARENA) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_ITTEAM:
			if (!cfg->itTeam.isSet)
			{
				Q_strncpyz(out, "--", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_BLUE)
			{
				Q_strncpyz(out, "blue", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_RED)
			{
				Q_strncpyz(out, "red", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_NEUTRAL)
			{
				Q_strncpyz(out, "neu", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_ENEMY)
			{
				Q_strncpyz(out, "enemy", outSize);
			}
			else
			{
				Q_strncpyz(out, "own", outSize);
			}
			break;
		case SHUD_PROP_TIME:
			if (cfg->time.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->time.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FADEDELAY:
			if (cfg->fadedelay.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->fadedelay.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_HLSIZE:
			if (cfg->hlsize.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->hlsize.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			if (cfg->textOffset.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->textOffset.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			if (cfg->border.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->border.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			if (cfg->imagetc.isSet)
			{
				Com_sprintf(out, outSize, "%.2f", cfg->imagetc.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			if (cfg->margins.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->margins.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		default:
			Q_strncpyz(out, "?", outSize);
			break;
	}
}

static void CG_SHUDEditorDrawProps(void)
{
	char value[64];
	char lab[8];
	int i;
	int n;
	int hoverIdx;
	float y;
	float trackX;
	float trackW;
	float fillW;
	vec4_t dimColor;
	vec4_t swatch;
	vec4_t bar;
	shudLay_t lay[SHUD_EDIT_LAY_MAX];
	shudLay_t hover;
	const float* scol;
	float* rgba;
	char typeLine[80];
	static const char* labels[SHUD_PROP_COUNT] =
	{
		"x", "y", "w", "h",
		"font", "fs w", "fs h",
		"alignh", "alignv", "talign", "dir",
		"color", "bgcolor", "color2", "bgcol2",
		"image", "text",
		"tx", "ty",
		"itteam", "time", "fdelay",
		"bL", "bT", "bR", "bB",
		"bcol", "bcol2",
		"hlsize", "fade", "shadow", "hlcol",
		"tcS", "tcT", "tcU", "tcV",
		"mL", "mT", "mR", "mB",
		"style", "tstyle",
		"fill", "mono", "dbar",
		"im", "spect", "dead", "sc.hide", "empty",
		"team", "demohide",
		"key1", "key2", "key3", "key4",
		"ffa", "tourney", "tdm", "ctf", "ft", "ca"
	};
	static const char* chlab[4] = { "R", "G", "B", "A" };

	if (!editor.showProps)
	{
		return;
	}

	Vector4Set(dimColor, 0.65f, 0.65f, 0.70f, 1.0f);
	hoverIdx = CG_SHUDEditorHitLayout(&hover);
	n = CG_SHUDEditorLayout(lay, SHUD_EDIT_LAY_MAX);
	trackX = SHUD_EDIT_PROPS_X + 24.0f;
	trackW = SHUD_EDIT_PROPS_W - 72.0f;

	CG_FillRect(SHUD_EDIT_PROPS_X, SHUD_EDIT_PROPS_Y, SHUD_EDIT_PROPS_W, SHUD_EDIT_PROPS_H, shudEditPanelBg);
	CG_DrawRect(SHUD_EDIT_PROPS_X, SHUD_EDIT_PROPS_Y, SHUD_EDIT_PROPS_W, SHUD_EDIT_PROPS_H, 1, shudEditPanelBorder);

	CG_FontSelect(0);
	CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, SHUD_EDIT_PROPS_Y + 6, "^3PROPS", colorYellow, 8, 8, (int)SHUD_EDIT_PROPS_W - 20, DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PROPS_X + SHUD_EDIT_PROPS_W - 14, SHUD_EDIT_PROPS_Y + 4, "x", colorMdGrey, 8, 8, 12, DS_HLEFT | DS_PROPORTIONAL, NULL);

	if (!editor.selected)
	{
		CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, SHUD_EDIT_PROP_LIST_Y, "select element", colorMdGrey, 6, 8, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
		return;
	}

	for (i = 0; i < n; i++)
	{
		y = lay[i].y;
		if (y < SHUD_EDIT_PROP_LIST_Y || y >= SHUD_EDIT_PROP_BOTTOM)
		{
			continue;
		}
		if (lay[i].kind == 2)
		{
			CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, y + 1, CG_SHUDEditorPropCatTitle(lay[i].prop),
			                 colorCyan, 6, 8, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
			continue;
		}
		if (lay[i].kind == 0)
		{
			if (lay[i].prop == editor.propIndex)
			{
				CG_FillRect(SHUD_EDIT_PROPS_X + 2, y, SHUD_EDIT_PROPS_W - 4, SHUD_EDIT_PROP_ROW, shudEditRowSel);
			}
			else if (hoverIdx == i)
			{
				CG_FillRect(SHUD_EDIT_PROPS_X + 2, y, SHUD_EDIT_PROPS_W - 4, SHUD_EDIT_PROP_ROW, shudEditOutlineHidden);
			}
			CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, y + 1, labels[lay[i].prop],
			                 lay[i].prop == editor.propIndex ? colorWhite : dimColor,
			                 6, 8, 52, DS_HLEFT | DS_PROPORTIONAL, NULL);
			CG_SHUDEditorPropValue(editor.selected, lay[i].prop, value, sizeof(value));
			CG_OSPDrawString(SHUD_EDIT_PROPS_X + 60, y + 1, value,
			                 lay[i].prop == editor.propIndex ? colorWhite : dimColor,
			                 6, 8, (int)(SHUD_EDIT_PROPS_W - 82), DS_HLEFT | DS_PROPORTIONAL, NULL);
			if (CG_SHUDEditorColorHasSliders(editor.selected, lay[i].prop))
			{
				rgba = CG_SHUDEditorRgbaPtr(editor.selected, lay[i].prop);
				if (rgba)
				{
					Vector4Copy(rgba, swatch);
					swatch[3] = 1.0f;
					CG_FillRect(SHUD_EDIT_PROPS_X + SHUD_EDIT_PROPS_W - 18, y + 1, 12, 8, swatch);
				}
			}
		}
		else if (lay[i].kind == 1)
		{
			rgba = CG_SHUDEditorRgbaPtr(editor.selected, lay[i].prop);
			if (!rgba)
			{
				continue;
			}
			if (lay[i].channel == 0)
			{
				scol = shudEditSliderR;
			}
			else if (lay[i].channel == 1)
			{
				scol = shudEditSliderG;
			}
			else if (lay[i].channel == 2)
			{
				scol = shudEditSliderB;
			}
			else
			{
				scol = shudEditSliderA;
			}
			CG_OSPDrawString(SHUD_EDIT_PROPS_X + 8, y + 1, chlab[lay[i].channel], scol, 5, 7, 12, DS_HLEFT | DS_PROPORTIONAL, NULL);
			CG_FillRect(trackX, y + 2, trackW, 6, shudEditSliderBg);
			fillW = trackW * rgba[lay[i].channel];
			if (fillW < 1.0f)
			{
				fillW = 1.0f;
			}
			Vector4Copy(scol, bar);
			CG_FillRect(trackX, y + 2, fillW, 6, bar);
			Com_sprintf(lab, sizeof(lab), "%d", (int)(rgba[lay[i].channel] * 255.0f + 0.5f));
			CG_OSPDrawString(trackX + trackW + 4, y + 1, lab, dimColor, 5, 7, 28, DS_HLEFT | DS_PROPORTIONAL, NULL);
		}
	}

	if (editor.typing && editor.typeProp >= 0)
	{
		Com_sprintf(typeLine, sizeof(typeLine), "> %s_", editor.typeBuf);
		CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, 444, typeLine, colorYellow, 6, 8, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, 454, "Enter type  Bksp unset", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, 464, "F1 help  B hide boxes", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
}

static void CG_SHUDEditorDrawSearch(void)
{
	char line[80];
	vec4_t bg;
	const char* filter;

	Vector4Set(bg, 0.06f, 0.08f, 0.12f, 0.95f);
	CG_FillRect(SHUD_EDIT_PANEL_X + 2, SHUD_EDIT_SEARCH_Y, SHUD_EDIT_PANEL_W - 4, SHUD_EDIT_SEARCH_H, bg);
	filter = editor.picker ? editor.pickerFilter : editor.listFilter;
	if (editor.typing && editor.typeProp < 0)
	{
		Com_sprintf(line, sizeof(line), "/%s_", editor.typeBuf);
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, SHUD_EDIT_SEARCH_Y + 3, line, colorYellow, 6, 8, (int)(SHUD_EDIT_PANEL_W - 12), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	else if (filter && filter[0])
	{
		Com_sprintf(line, sizeof(line), "/%s", filter);
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, SHUD_EDIT_SEARCH_Y + 3, line, colorCyan, 6, 8, (int)(SHUD_EDIT_PANEL_W - 12), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	else
	{
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, SHUD_EDIT_SEARCH_Y + 3, "search...", colorMdGrey, 6, 8, (int)(SHUD_EDIT_PANEL_W - 12), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
}

static void CG_SHUDEditorDrawPanel(void)
{
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];
	vec4_t rect;
	char line[128];
	int count;
	int vis;
	int i;
	int row;
	int els;
	float y;
	vec4_t dimColor;
	vec4_t headColor;
	int t;
	int col;
	float tw;
	float tx;
	float ty;

	if (!editor.showList)
	{
		return;
	}

	Vector4Set(dimColor, 0.65f, 0.65f, 0.70f, 1.0f);
	Vector4Set(headColor, 0.45f, 0.75f, 1.00f, 1.0f);

	CG_FillRect(SHUD_EDIT_PANEL_X, SHUD_EDIT_PANEL_Y, SHUD_EDIT_PANEL_W, SHUD_EDIT_PANEL_H, shudEditPanelBg);
	CG_DrawRect(SHUD_EDIT_PANEL_X, SHUD_EDIT_PANEL_Y, SHUD_EDIT_PANEL_W, SHUD_EDIT_PANEL_H, 1, shudEditPanelBorder);

	CG_FontSelect(0);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, SHUD_EDIT_PANEL_Y + 4,
	                 editor.picker ? "^3ADD" : "^3SHUD EDIT",
	                 colorYellow, 8, 8, (int)SHUD_EDIT_PANEL_W - 20, DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W - 14, SHUD_EDIT_PANEL_Y + 4, "x", colorMdGrey, 8, 8, 12, DS_HLEFT | DS_PROPORTIONAL, NULL);

	tw = (SHUD_EDIT_PANEL_W - 4.0f) / 4.0f;
	for (t = 0; t < SHUD_TAB_COUNT; t++)
	{
		col = t % 4;
		tx = SHUD_EDIT_PANEL_X + 2.0f + (float)col * tw;
		ty = SHUD_EDIT_TAB_Y + (float)(t / 4) * (SHUD_EDIT_TAB_H * 0.5f);
		if (t == editor.tab)
		{
			CG_FillRect(tx, ty, tw - 1.0f, SHUD_EDIT_TAB_H * 0.5f - 1.0f, shudEditRowSel);
		}
		CG_OSPDrawString(tx + 2, ty + 1, shudEditTabShort[t],
		                 t == editor.tab ? colorWhite : dimColor,
		                 5, 7, (int)(tw - 3), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	CG_SHUDEditorDrawSearch();

	vis = CG_SHUDEditorVisibleRows();
	if (editor.picker)
	{
		count = CG_SHUDEditorBuildTypeList(rows, SHUD_EDIT_ROW_MAX);
		if (editor.pickerScroll > count - vis)
		{
			editor.pickerScroll = count - vis;
		}
		if (editor.pickerScroll < 0)
		{
			editor.pickerScroll = 0;
		}
		for (i = 0; i < vis; i++)
		{
			row = editor.pickerScroll + i;
			if (row >= count)
			{
				break;
			}
			y = SHUD_EDIT_LIST_Y + i * SHUD_EDIT_ROW_H;
			if (rows[row].header)
			{
				Com_sprintf(line, sizeof(line), "^5%s", rows[row].title ? rows[row].title : "");
				CG_OSPDrawString(SHUD_EDIT_PANEL_X + 4, y + 1, line, headColor, 6, 8, (int)(SHUD_EDIT_PANEL_W - 8), DS_HLEFT | DS_PROPORTIONAL, NULL);
				continue;
			}
			if (row == editor.pickerIndex)
			{
				CG_FillRect(SHUD_EDIT_PANEL_X + 2, y, SHUD_EDIT_PANEL_W - 4, SHUD_EDIT_ROW_H, shudEditRowSel);
			}
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + 10, y + 1,
			                 rows[row].def && rows[row].def->name ? rows[row].def->name : "?",
			                 row == editor.pickerIndex ? colorWhite : dimColor,
			                 6, 8, (int)(SHUD_EDIT_PANEL_W - 14), DS_HLEFT | DS_PROPORTIONAL, NULL);
		}
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 444, "click to add", colorCyan, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
		Com_sprintf(line, sizeof(line), "%d types", count);
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 454, line, colorWhite, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 464, "N/ESC close", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
		return;
	}

	count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	if (editor.scroll > count - vis)
	{
		editor.scroll = count - vis;
	}
	if (editor.scroll < 0)
	{
		editor.scroll = 0;
	}

	els = 0;
	for (i = 0; i < count; i++)
	{
		if (rows[i].el && !CG_SHUDEditorIsDefault(rows[i].el))
		{
			els++;
		}
	}

	for (i = 0; i < vis; i++)
	{
		row = editor.scroll + i;
		if (row >= count)
		{
			break;
		}
		y = SHUD_EDIT_LIST_Y + i * SHUD_EDIT_ROW_H;
		if (rows[row].header)
		{
			Com_sprintf(line, sizeof(line), "^5%s", rows[row].title ? rows[row].title : "");
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + 4, y + 1, line, headColor, 6, 8, (int)(SHUD_EDIT_PANEL_W - 8), DS_HLEFT | DS_PROPORTIONAL, NULL);
			continue;
		}
		if (rows[row].el == editor.selected || CG_SHUDEditorSelHas(rows[row].el) ||
		    (rows[row].title && !Q_stricmp(rows[row].title, "DEFAULT") && CG_SHUDEditorIsDefault(editor.selected)))
		{
			CG_FillRect(SHUD_EDIT_PANEL_X + 2, y, SHUD_EDIT_PANEL_W - 4, SHUD_EDIT_ROW_H, shudEditRowSel);
		}
		if (rows[row].title && !Q_stricmp(rows[row].title, "DEFAULT"))
		{
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + 10, y + 1, "^3DEFAULT",
			                 CG_SHUDEditorIsDefault(editor.selected) ? colorWhite : colorYellow,
			                 6, 8, (int)(SHUD_EDIT_PANEL_W - 14), DS_HLEFT | DS_PROPORTIONAL, NULL);
			continue;
		}
		if (!rows[row].el)
		{
			continue;
		}
		Com_sprintf(line, sizeof(line), "%s%s%s",
		            CG_SHUDEditorIsGhost(rows[row].el) ? "^9" : (CG_SHUDElementIsHidden(rows[row].el) ? "^8" : ""),
		            rows[row].el->element.name ? rows[row].el->element.name : "?",
		            CG_SHUDEditorIsGhost(rows[row].el) ? " *" : "");
		CG_OSPDrawString(SHUD_EDIT_PANEL_X + 10, y + 1, line,
		                 rows[row].el == editor.selected ? colorWhite : (CG_SHUDEditorSelHas(rows[row].el) ? colorYellow : dimColor),
		                 6, 8, (int)(SHUD_EDIT_PANEL_W - 14), DS_HLEFT | DS_PROPORTIONAL, NULL);
	}

	if (editor.selected && editor.selected->element.name)
	{
		if (CG_SHUDEditorIsDefault(editor.selected))
		{
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 422, "DEFAULT", colorYellow, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 432, "inherit for tab", colorMdGrey, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
		}
		else
		{
			if (editor.selCount > 1)
			{
				Com_sprintf(line, sizeof(line), "%s  +%d", editor.selected->element.name, editor.selCount - 1);
				CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 422, line, colorCyan, 6, 8, (int)(SHUD_EDIT_PANEL_W - 44), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
			else
			{
				CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 422, editor.selected->element.name, colorCyan, 6, 8, (int)(SHUD_EDIT_PANEL_W - 44), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
			CG_OSPDrawString(SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W - 28, 422, "^1del", colorRed, 6, 8, 26, DS_HLEFT | DS_PROPORTIONAL, NULL);
			if (CG_SHUDEditorGetRect(editor.selected, rect))
			{
				Com_sprintf(line, sizeof(line), "%.0f %.0f %.0f %.0f", rect[0], rect[1], rect[2], rect[3]);
				CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 432, line, colorWhite, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
			else
			{
				CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 432, "no rect", colorMdGrey, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
		}
	}

	Com_sprintf(line, sizeof(line), "%s%s  %d", editor.dirty ? "^1*" : " ", ch_file.string[0] ? ch_file.string : "?", els);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 444, line, colorWhite, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 454, "Shift-click  box-select", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 464, "B boxes  F5 save", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
}

void CG_SHUDEditorDraw(void)
{
	superhudElement_t* items[SHUD_EDIT_MAX_ITEMS];
	superhudElement_t* hover;
	int count;
	int i;
	vec4_t mrect;

	if (!editor.active)
	{
		return;
	}
	if (!cg_shud.integer)
	{
		CG_SHUDEditorActivate(qfalse);
		return;
	}
	if (!(trap_Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CONSOLE)))
	{
		CG_SHUDEditorCatchKeys(qtrue);
	}

	CG_SHUDEditorDrawGrid();
	CG_SHUDEditorDrawGuides();

	count = CG_SHUDEditorCollect(items, SHUD_EDIT_MAX_ITEMS);
	hover = NULL;
	if (!CG_SHUDEditorCursorInPanel() && !CG_SHUDEditorCursorInProps() && editor.drag == SHUD_DRAG_NONE && !editor.help)
	{
		hover = CG_SHUDEditorHitElement(editor.cursorX, editor.cursorY);
	}

	for (i = 0; i < count; i++)
	{
		if (CG_SHUDEditorIsGhost(items[i]))
		{
			continue;
		}
		if (CG_SHUDEditorHideBox(items[i]))
		{
			continue;
		}
		if (items[i] != editor.selected)
		{
			CG_SHUDEditorDrawElementBox(items[i], CG_SHUDEditorSelHas(items[i]), items[i] == hover);
		}
	}
	if (editor.selected && !CG_SHUDEditorIsGhost(editor.selected))
	{
		CG_SHUDEditorDrawElementBox(editor.selected, qtrue, qfalse);
	}
	if (editor.drag == SHUD_DRAG_MARQUEE)
	{
		CG_SHUDEditorMarqueeRect(mrect);
		CG_FillRect(mrect[0], mrect[1], mrect[2], mrect[3], shudEditMarqueeFill);
		CG_DrawRect(mrect[0], mrect[1], mrect[2], mrect[3], 1, shudEditMarqueeLine);
	}

	CG_SHUDEditorDrawPanel();
	CG_SHUDEditorDrawProps();
	if (!editor.showList)
	{
		CG_FillRect(2, 4, 16, 16, shudEditPanelBg);
		CG_OSPDrawString(5, 7, "L", colorCyan, 6, 8, 12, DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	if (!editor.showProps)
	{
		CG_FillRect(SCREEN_WIDTH - 18, 4, 16, 16, shudEditPanelBg);
		CG_OSPDrawString(SCREEN_WIDTH - 15, 7, "R", colorCyan, 6, 8, 12, DS_HLEFT | DS_PROPORTIONAL, NULL);
	}
	if (editor.help)
	{
		CG_SHUDEditorDrawHelp();
	}
	CG_SHUDEditorDrawCursor();
}
