#include "editor/shud_editor.h"

void CG_SHUDEditorDrawHelp(void)
{
	static const char* lines[] =
	{
		"^3F1 help  ESC close",
		"F5 save   Ctrl+S save",
		"N add     D dup     Del delete",
		"Enter / click value to type",
		"Click RGB number for 0-255",
		"click search",
		"Bksp unset prop",
		"G grid mode   F3 grid size",
		"C center guides / neighbor snap",
		"A/V align screen   M mirror",
		"Ctrl+Arrows layer",
		"Ctrl+Z undo  Ctrl+Y redo",
		"Ctrl+C/V copy paste",
		"I pipette color   F force-show",
		"B hide boxes of other tabs",
		"Shift-click add to selection",
		"Shift-drag list: multi-select",
		"Drag empty: box-select",
		"H hide in editor   Shift resize lock",
		"Wheel scroll in panels",
		"Alt+wheel scroll on canvas",
		"O list  P props  Del delete",
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

void CG_SHUDEditorDrawGuides(void)
{
	vec4_t c;

	if (!editor.showGuides)
	{
		return;
	}
	Vector4Set(c, 1.0f, 0.85f, 0.2f, 0.18f);
	CG_FillRect(SCREEN_WIDTH * 0.5f - 0.5f, 0, 1, SCREEN_HEIGHT, c);
	CG_FillRect(0, SCREEN_HEIGHT * 0.5f - 0.5f, SCREEN_WIDTH, 1, c);
}

void CG_SHUDEditorDrawHandle(float x, float y)
{
	float half;

	half = SHUD_EDIT_HANDLE * 0.5f;
	CG_FillRect(x - half, y - half, SHUD_EDIT_HANDLE, SHUD_EDIT_HANDLE, shudEditHandle);
}

void CG_SHUDEditorDrawElementBox(superhudElement_t* el, qboolean selected, qboolean hover)
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

void CG_SHUDEditorDrawGrid(void)
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
		if (i == SCREEN_WIDTH / 2)
		{
			CG_FillRect((float)i - 0.5f, 0, 1, SCREEN_HEIGHT, shudEditGrid);
		}
		else
		{
			CG_FillRect((float)i, 0, 1, SCREEN_HEIGHT, shudEditGrid);
		}
	}
	for (i = 0; i <= SCREEN_HEIGHT; i += g)
	{
		if (i == SCREEN_HEIGHT / 2)
		{
			CG_FillRect(0, (float)i - 0.5f, SCREEN_WIDTH, 1, shudEditGrid);
		}
		else
		{
			CG_FillRect(0, (float)i, SCREEN_WIDTH, 1, shudEditGrid);
		}
	}
}

void CG_SHUDEditorDrawCursor(void)
{
	CG_FillRect(editor.cursorX - 6.0f, editor.cursorY - 0.5f, 13.0f, 1.0f, shudEditCursor);
	CG_FillRect(editor.cursorX - 0.5f, editor.cursorY - 6.0f, 1.0f, 13.0f, shudEditCursor);
	CG_FillRect(editor.cursorX - 0.5f, editor.cursorY - 0.5f, 1.0f, 1.0f, colorWhite);
}

void CG_SHUDEditorDrawProps(void)
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
		"image", "imgalnH", "imgalnV", "text",
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
			if (editor.typing && editor.typeProp == lay[i].prop && editor.typeChannel < 0)
			{
				Com_sprintf(typeLine, sizeof(typeLine), "> %s_", editor.typeBuf);
				CG_OSPDrawString(SHUD_EDIT_PROPS_X + 60, y + 1, typeLine, colorYellow,
				                 6, 8, (int)(SHUD_EDIT_PROPS_W - 82), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
			else
			{
				CG_OSPDrawString(SHUD_EDIT_PROPS_X + 60, y + 1, value,
				                 lay[i].prop == editor.propIndex ? colorWhite : dimColor,
				                 6, 8, (int)(SHUD_EDIT_PROPS_W - 82), DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
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
			if (editor.typing && editor.typeProp == lay[i].prop && editor.typeChannel == lay[i].channel)
			{
				CG_FillRect(SHUD_EDIT_SLIDER_NUM_X - 1, y, SHUD_EDIT_SLIDER_NUM_W + 2, SHUD_EDIT_PROP_ROW, shudEditRowSel);
				Com_sprintf(typeLine, sizeof(typeLine), "> %s_", editor.typeBuf);
				CG_OSPDrawString(SHUD_EDIT_SLIDER_NUM_X, y + 1, typeLine, colorYellow, 5, 7, 28, DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
			else
			{
				Com_sprintf(lab, sizeof(lab), "%d", (int)(rgba[lay[i].channel] * 255.0f + 0.5f));
				CG_OSPDrawString(SHUD_EDIT_SLIDER_NUM_X, y + 1, lab, dimColor, 5, 7, 28, DS_HLEFT | DS_PROPORTIONAL, NULL);
			}
		}
	}

	CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, 454, "click value / RGB # to type", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PROPS_X + 6, 464, "F1 help  B hide boxes", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PROPS_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
}

void CG_SHUDEditorDrawSearch(void)
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

void CG_SHUDEditorDrawPanel(void)
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

	if (editor.drag == SHUD_DRAG_LIST_MARQUEE)
	{
		CG_SHUDEditorMarqueeRect(rect);
		CG_FillRect(rect[0], rect[1], rect[2], rect[3], shudEditMarqueeFill);
		CG_DrawRect(rect[0], rect[1], rect[2], rect[3], 1, shudEditMarqueeLine);
	}

	Com_sprintf(line, sizeof(line), "%s%s  %d", editor.dirty ? "^1*" : " ", ch_file.string[0] ? ch_file.string : "?", els);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 444, line, colorWhite, 6, 8, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
	CG_OSPDrawString(SHUD_EDIT_PANEL_X + 6, 454, "Shift-drag list multi-sel", colorMdGrey, 5, 7, (int)(SHUD_EDIT_PANEL_W - 10), DS_HLEFT | DS_PROPORTIONAL, NULL);
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

