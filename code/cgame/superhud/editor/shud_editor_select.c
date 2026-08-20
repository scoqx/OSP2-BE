#include "editor/shud_editor.h"

qboolean CG_SHUDEditorIsDefault(const superhudElement_t* el)
{
	return (el && CG_SHUDIsDefaultName(el->element.name)) ? qtrue : qfalse;
}

qboolean CG_SHUDEditorIsEditable(const superhudElement_t* el)
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

superhudElement_t* CG_SHUDEditorFindTabDefault(int tab)
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

superhudElement_t* CG_SHUDEditorEnsureTabDefault(int tab)
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

qboolean CG_SHUDEditorGetRect(const superhudElement_t* el, vec4_t out)
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
	return qtrue;
}

int CG_SHUDEditorNameGroup(const char* name)
{
	int t;

	t = CG_SHUDNameTab(name);
	if (t < 0)
	{
		return editor.tab;
	}
	return t;
}

const char* CG_SHUDEditorGroupTitle(int group)
{
	return CG_SHUDTabTitle(group);
}

qboolean CG_SHUDEditorNameMatches(const char* name, const char* filter)
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

int CG_SHUDEditorCollect(superhudElement_t** out, int max)
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

int CG_SHUDEditorCmpName(const char* a, const char* b)
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

void CG_SHUDEditorSortEls(superhudElement_t** items, int n)
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

int CG_SHUDEditorBuildList(shudListRow_t* rows, int max)
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

superhudElement_t* CG_SHUDEditorFindByName(const char* name)
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

int CG_SHUDEditorIndexOf(superhudElement_t* el)
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

int CG_SHUDEditorVisibleRows(void)
{
	int rows;

	rows = (int)((SHUD_EDIT_LIST_BOTTOM - SHUD_EDIT_LIST_Y) / SHUD_EDIT_ROW_H);
	if (rows < 1)
	{
		rows = 1;
	}
	return rows;
}

void CG_SHUDEditorEnsureVisible(void)
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

void CG_SHUDEditorBeginUndoGroup(void)
{
	editor.undoGroupSeq++;
	if (editor.undoGroupSeq <= 0)
	{
		editor.undoGroupSeq = 1;
	}
	editor.undoGroup = editor.undoGroupSeq;
}

void CG_SHUDEditorEndUndoGroup(void)
{
	editor.undoGroup = 0;
}

qboolean CG_SHUDEditorSelHas(const superhudElement_t* el)
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

void CG_SHUDEditorSelAdd(superhudElement_t* el)
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

void CG_SHUDEditorSelRemove(superhudElement_t* el)
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

void CG_SHUDEditorSetPrimary(superhudElement_t* el)
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

void CG_SHUDEditorSelect(superhudElement_t* el)
{
	editor.drag = SHUD_DRAG_NONE;
	editor.selCount = 0;
	CG_SHUDEditorSetPrimary(el);
	if (el && !CG_SHUDEditorIsDefault(el))
	{
		CG_SHUDEditorSelAdd(el);
	}
}

void CG_SHUDEditorSelectIndex(int index)
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

void CG_SHUDEditorClampRect(vec4_t rect)
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

float CG_SHUDEditorSnapVal(float v)
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

float CG_SHUDEditorSnapToList(float v, const float* list, int n, float pix)
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

void CG_SHUDEditorSnapNeighbors(vec4_t tmp)
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

void CG_SHUDEditorApplyRect(superhudElement_t* el, const vec4_t rect, qboolean round, qboolean snapNeighbors)
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
	if (snapNeighbors && editor.showGuides && !trap_Key_IsDown(K_ALT))
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
	el->config.rect.isSet = qtrue;
	Vector4Copy(tmp, el->config.rect.value);
	CG_SHUDRecreateElement(el);
	editor.dirty = qtrue;
}

qboolean CG_SHUDEditorPointInRect(float x, float y, const vec4_t rect)
{
	return (x >= rect[0] && y >= rect[1] && x <= rect[0] + rect[2] && y <= rect[1] + rect[3]);
}

qboolean CG_SHUDEditorPointInHandle(float x, float y, float hx, float hy)
{
	float half;

	half = SHUD_EDIT_HANDLE * 0.5f;
	return (x >= hx - half && x <= hx + half && y >= hy - half && y <= hy + half);
}

shudEditorDrag_t CG_SHUDEditorHitHandle(float x, float y, const vec4_t rect)
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

qboolean CG_SHUDEditorHideBox(const superhudElement_t* el)
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

superhudElement_t* CG_SHUDEditorHitElement(float x, float y)
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

qboolean CG_SHUDEditorCursorInPanel(void)
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

qboolean CG_SHUDEditorCursorInSearch(void)
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

qboolean CG_SHUDEditorCursorInList(void)
{
	if (!editor.showList || editor.picker)
	{
		return qfalse;
	}
	return (editor.cursorX >= SHUD_EDIT_PANEL_X &&
	        editor.cursorX <= SHUD_EDIT_PANEL_X + SHUD_EDIT_PANEL_W &&
	        editor.cursorY >= SHUD_EDIT_LIST_Y &&
	        editor.cursorY < SHUD_EDIT_LIST_BOTTOM);
}

qboolean CG_SHUDEditorCursorInProps(void)
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

qboolean CG_SHUDEditorElementAlive(superhudElement_t* el)
{
	return CG_SHUDElementChainIndex(el) >= 0 ? qtrue : qfalse;
}

void CG_SHUDEditorClearUndo(void)
{
	editor.undoCount = 0;
	editor.redoCount = 0;
	editor.coalesceProp = -1;
	memset(editor.undo, 0, sizeof(editor.undo));
	memset(editor.redo, 0, sizeof(editor.redo));
}

void CG_SHUDEditorPushUndo(shudUndoType_t type, superhudElement_t* el)
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

void CG_SHUDEditorPushConfigUndo(superhudElement_t* el)
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

void CG_SHUDEditorPushSelConfigUndo(int prop)
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

void CG_SHUDEditorUndo(void)
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

qboolean CG_SHUDEditorMatchFilter(const char* name, const char* filter)
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

qboolean CG_SHUDEditorIsGhost(const superhudElement_t* el)
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

void CG_SHUDEditorToggleGhost(superhudElement_t* el)
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

void CG_SHUDEditorClearRedo(void)
{
	editor.redoCount = 0;
	memset(editor.redo, 0, sizeof(editor.redo));
}

void CG_SHUDEditorPushRedo(shudUndoEntry_t* src)
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

void CG_SHUDEditorRedo(void)
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

void CG_SHUDEditorTypicalSize(const char* name, vec4_t rect)
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

void CG_SHUDEditorAlignScreen(int mode)
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
		CG_SHUDEditorApplyRect(el, rect, qtrue, qfalse);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.coalesceProp = -1;
}

void CG_SHUDEditorMirrorH(void)
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
		CG_SHUDEditorApplyRect(el, rect, qtrue, qfalse);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.coalesceProp = -1;
}

void CG_SHUDEditorCopy(void)
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

void CG_SHUDEditorPaste(void)
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

void CG_SHUDEditorLayer(int dir)
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

qboolean CG_SHUDEditorAllowMulti(const char* name)
{
	if (!name)
	{
		return qfalse;
	}
	return (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate")) ? qtrue : qfalse;
}

int CG_SHUDEditorCollectTypes(const superHUDConfigElement_t** out, int max)
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

int CG_SHUDEditorBuildTypeList(shudListRow_t* rows, int max)
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

superhudElement_t* CG_SHUDEditorFindDefault(void)
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

void CG_SHUDEditorAddType(const superHUDConfigElement_t* def)
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

void CG_SHUDEditorDuplicate(void)
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

void CG_SHUDEditorDeleteSelected(void)
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

void CG_SHUDEditorNudge(float dx, float dy, qboolean resize)
{
	vec4_t rect;
	int i;
	int n;
	int needPush;
	superhudElement_t* el;

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
		CG_SHUDEditorApplyRect(el, rect, qfalse, qfalse);
	}
	if (needPush)
	{
		CG_SHUDEditorEndUndoGroup();
	}
	editor.coalesceProp = 1000;
}

void CG_SHUDEditorSetTab(int tab)
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

void CG_SHUDEditorApplyDrag(void)
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
	if (editor.drag == SHUD_DRAG_MARQUEE || editor.drag == SHUD_DRAG_LIST_MARQUEE)
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
		CG_SHUDEditorApplyRect(editor.selected, rect, qfalse,
		                       editor.showGuides && !trap_Key_IsDown(K_ALT));
		dx = editor.selected->config.rect.value[0] - editor.dragStartRects[pi][0];
		dy = editor.selected->config.rect.value[1] - editor.dragStartRects[pi][1];
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
			CG_SHUDEditorApplyRect(editor.sel[i], rect, qfalse, qfalse);
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
	CG_SHUDEditorApplyRect(editor.selected, rect, qfalse,
	                       editor.showGuides && !trap_Key_IsDown(K_ALT));
}

