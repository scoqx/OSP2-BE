#include "editor/shud_editor.h"

void CG_SHUDEditorWheelScroll(int dir, int count, int vis)
{
	if (CG_SHUDEditorCursorInProps())
	{
		CG_SHUDEditorScrollProps(dir * 2);
		return;
	}
	if (CG_SHUDEditorCursorInPanel())
	{
		if (editor.picker)
		{
			editor.pickerScroll += dir;
			if (editor.pickerScroll < 0)
			{
				editor.pickerScroll = 0;
			}
			if (editor.pickerScroll > count - vis)
			{
				editor.pickerScroll = count - vis;
			}
			if (editor.pickerScroll < 0)
			{
				editor.pickerScroll = 0;
			}
		}
		else
		{
			editor.scroll += dir;
			if (editor.scroll > count - vis)
			{
				editor.scroll = count - vis;
			}
			if (editor.scroll < 0)
			{
				editor.scroll = 0;
			}
		}
		return;
	}
	if (trap_Key_IsDown(K_ALT))
	{
		if (editor.cursorX < SCREEN_WIDTH * 0.5f)
		{
			if (editor.showList)
			{
				if (editor.picker)
				{
					editor.pickerScroll += dir;
					if (editor.pickerScroll < 0)
					{
						editor.pickerScroll = 0;
					}
					if (editor.pickerScroll > count - vis)
					{
						editor.pickerScroll = count - vis;
					}
					if (editor.pickerScroll < 0)
					{
						editor.pickerScroll = 0;
					}
				}
				else
				{
					editor.scroll += dir;
					if (editor.scroll > count - vis)
					{
						editor.scroll = count - vis;
					}
					if (editor.scroll < 0)
					{
						editor.scroll = 0;
					}
				}
			}
		}
		else if (editor.showProps)
		{
			CG_SHUDEditorScrollProps(dir * 2);
		}
	}
}

void CG_SHUDEditorClickPanel(void)
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
		CG_SHUDEditorStartTypeIn(editor.picker ? -2 : -1, -1);
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

qboolean CG_SHUDEditorRectsOverlap(const vec4_t a, const vec4_t b)
{
	return (a[0] < b[0] + b[2] && b[0] < a[0] + a[2] &&
	        a[1] < b[1] + b[3] && b[1] < a[1] + a[3]) ? qtrue : qfalse;
}

void CG_SHUDEditorMarqueeRect(vec4_t out)
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

void CG_SHUDEditorFinishMarquee(void)
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

void CG_SHUDEditorFinishListMarquee(void)
{
	shudListRow_t rows[SHUD_EDIT_ROW_MAX];
	vec4_t box;
	vec4_t rowBox;
	int count;
	int vis;
	int i;
	int row;

	CG_SHUDEditorMarqueeRect(box);
	if (box[2] < 4.0f && box[3] < 4.0f)
	{
		CG_SHUDEditorClickPanel();
		return;
	}
	count = CG_SHUDEditorBuildList(rows, SHUD_EDIT_ROW_MAX);
	vis = CG_SHUDEditorVisibleRows();
	rowBox[0] = SHUD_EDIT_PANEL_X + 2.0f;
	rowBox[2] = SHUD_EDIT_PANEL_W - 4.0f;
	rowBox[3] = SHUD_EDIT_ROW_H;
	for (i = 0; i < vis; i++)
	{
		row = editor.scroll + i;
		if (row < 0 || row >= count)
		{
			continue;
		}
		if (rows[row].header || !rows[row].el || CG_SHUDEditorIsDefault(rows[row].el))
		{
			continue;
		}
		rowBox[1] = SHUD_EDIT_LIST_Y + (float)i * SHUD_EDIT_ROW_H;
		if (!CG_SHUDEditorRectsOverlap(box, rowBox))
		{
			continue;
		}
		CG_SHUDEditorSelAdd(rows[row].el);
	}
	if (editor.selCount > 0)
	{
		CG_SHUDEditorSetPrimary(editor.sel[editor.selCount - 1]);
	}
}

void CG_SHUDEditorStartGroupMove(void)
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
	CG_SHUDEditorBeginUndoGroup();
	for (i = 0; i < editor.selCount; i++)
	{
		CG_SHUDEditorPushConfigUndo(editor.sel[i]);
	}
	CG_SHUDEditorEndUndoGroup();
	editor.coalesceProp = -1;
	editor.undoLock = qtrue;
	editor.drag = SHUD_DRAG_MOVE;
}

void CG_SHUDEditorMouseDown(void)
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
		if (!editor.picker && trap_Key_IsDown(K_SHIFT) && CG_SHUDEditorCursorInList())
		{
			editor.drag = SHUD_DRAG_LIST_MARQUEE;
			editor.marqueeAdd = qtrue;
			editor.dragGrabX = editor.cursorX;
			editor.dragGrabY = editor.cursorY;
			return;
		}
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

void CG_SHUDEditorMouseUp(void)
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
	if (editor.drag == SHUD_DRAG_LIST_MARQUEE)
	{
		CG_SHUDEditorFinishListMarquee();
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
			CG_SHUDEditorApplyRect(editor.sel[i], rect, qtrue,
			                       editor.showGuides && !trap_Key_IsDown(K_ALT));
			editor.showGuides = savedGuides;
			editor.snapGrid = savedGrid;
		}
	}
	else if (editor.drag != SHUD_DRAG_NONE && CG_SHUDEditorGetRect(editor.selected, rect))
	{
		CG_SHUDEditorApplyRect(editor.selected, rect, qtrue,
		                       editor.showGuides && !trap_Key_IsDown(K_ALT));
	}
	editor.mouse1 = qfalse;
	editor.drag = SHUD_DRAG_NONE;
	editor.undoLock = qfalse;
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
		step = trap_Key_IsDown(K_SHIFT) ? 10 : 1;
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
		CG_SHUDEditorStartTypeIn(editor.propIndex, -1);
		return;
	}
	if (key == '/')
	{
		CG_SHUDEditorStartTypeIn(editor.picker ? -2 : -1, -1);
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
	if (key == 'o' || key == 'O')
	{
		editor.showList = editor.showList ? qfalse : qtrue;
		return;
	}
	if (key == 'p' || key == 'P')
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
		CG_SHUDEditorWheelScroll(1, count, vis);
		return;
	}
	if (key == K_MWHEELUP)
	{
		CG_SHUDEditorWheelScroll(-1, count, vis);
		return;
	}
	if (key == K_PGDN)
	{
		if (editor.picker)
		{
			editor.pickerScroll += vis;
			return;
		}
		editor.scroll += vis;
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
	if (key == K_PGUP)
	{
		if (editor.picker)
		{
			editor.pickerScroll -= vis;
			if (editor.pickerScroll < 0)
			{
				editor.pickerScroll = 0;
			}
			return;
		}
		editor.scroll -= vis;
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
	if (key == K_LEFTARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(0);
			return;
		}
		CG_SHUDEditorNudge((float)(-step), 0, trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_RIGHTARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorAlignScreen(2);
			return;
		}
		CG_SHUDEditorNudge((float)step, 0, trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_UPARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorLayer(-1);
			return;
		}
		CG_SHUDEditorNudge(0, (float)(-step), trap_Key_IsDown(K_ALT));
		return;
	}
	if (key == K_DOWNARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			CG_SHUDEditorLayer(1);
			return;
		}
		CG_SHUDEditorNudge(0, (float)step, trap_Key_IsDown(K_ALT));
		return;
	}
}

