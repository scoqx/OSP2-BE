#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

#define SHUD_ITEMSPAWNS_ROW_PAD    2
#define SHUD_ITEMSPAWNS_MIN_GAP    4
#define SHUD_ITEMSPAWNS_EDGE_PAD   2
#define SHUD_ITEMSPAWNS_ICON_SCALE 1.5f
#define SHUD_ITEMSPAWNS_BADGE_WIDTH_SCALE 0.5f
#define SHUD_ITEMSPAWNS_BADGE_HEIGHT_SCALE 0.5f
#define SHUD_ITEMSPAWNS_BADGE_STACK 0.48f

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	int trackSideFilter;
	qboolean iconOnRight;
	float maxTextW;
	char printBuf[64];
	cg_spectatorItemSpotDrawRef_t drawRefs[CG_ITEM_SPAWNS_DRAW_MAX];
} shudElementSpectatorItemSpawns_t;

static float CG_SHUDElementSpectatorItemSpawnsMaxTextWidth(shudElementSpectatorItemSpawns_t* element,
        float charW, float charH);

static void* CG_SHUDElementSpectatorItemSpawnsCreateFilter(const superhudConfig_t* config, int trackSideFilter)
{
	shudElementSpectatorItemSpawns_t* element;

	SHUD_ELEMENT_INIT(element, config);
	element->trackSideFilter = trackSideFilter;
	element->iconOnRight = (trackSideFilter == 2) ? qtrue : qfalse;

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	/* rowY = top for icon; text Y is set per row (vertical align in row) */
	element->ctx.flags &= ~(DS_VTOP | DS_VCENTER | DS_VBOTTOM);

	/* config-dependent (constant per element), so measure the widest label once here */
	if (element->config.fontsize.isSet)
	{
		element->maxTextW = CG_SHUDElementSpectatorItemSpawnsMaxTextWidth(element,
		                    element->config.fontsize.value[0], element->config.fontsize.value[1]);
	}

	return element;
}

void* CG_SHUDElementSpectatorItemSpawnsCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemSpawnsCreateFilter(config, -1);
}

void* CG_SHUDElementSpectatorItemSpawnsRedCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemSpawnsCreateFilter(config, 1);
}

void* CG_SHUDElementSpectatorItemSpawnsBlueCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemSpawnsCreateFilter(config, 2);
}

/* alignh C = horizontal layout; rect.x is the horizontal center of the whole block.
 * Horizontal: alignv is cell stack only (no block Y shift). */
static void CG_SHUDElementSpectatorItemSpawnsResolveRect(const superhudConfig_t* cfg,
        float contentW, float contentH, float* outX, float* outY, qboolean horizontal)
{
	superhudAlignH_t h;
	superhudAlignV_t v;
	float x;
	float y;

	x = cfg->rect.value[0];
	y = cfg->rect.value[1];

	if (horizontal)
	{
		x -= contentW * 0.5f;
		*outX = x;
		*outY = y;
		return;
	}

	if (cfg->alignH.isSet)
	{
		h = cfg->alignH.value;
	}
	else
	{
		h = SUPERHUD_ALIGNH_LEFT;
	}

	/* CENTER is layout-mode only; do not center the vertical block */
	if (h == SUPERHUD_ALIGNH_CENTER)
	{
		h = SUPERHUD_ALIGNH_LEFT;
	}

	if (h == SUPERHUD_ALIGNH_RIGHT)
	{
		x -= contentW;
	}

	if (cfg->alignV.isSet)
	{
		v = cfg->alignV.value;
	}
	else
	{
		v = SUPERHUD_ALIGNV_TOP;
	}

	if (v == SUPERHUD_ALIGNV_CENTER)
	{
		y -= contentH * 0.5f;
	}
	else if (v == SUPERHUD_ALIGNV_BOTTOM)
	{
		y -= contentH;
	}

	*outX = x;
	*outY = y;
}

static superhudAlignH_t CG_SHUDElementSpectatorItemSpawnsEffectiveTextAlign(const superhudConfig_t* cfg,
        qboolean iconRight)
{
	superhudAlignH_t h;

	if (cfg->textAlign.isSet)
	{
		h = cfg->textAlign.value;
	}
	else
	{
		h = SUPERHUD_ALIGNH_LEFT;
	}

	if (!iconRight)
	{
		return h;
	}

	switch (h)
	{
		case SUPERHUD_ALIGNH_LEFT:
			return SUPERHUD_ALIGNH_RIGHT;
		case SUPERHUD_ALIGNH_RIGHT:
			return SUPERHUD_ALIGNH_LEFT;
		default:
		case SUPERHUD_ALIGNH_CENTER:
			return SUPERHUD_ALIGNH_CENTER;
	}
}

static void CG_SHUDElementSpectatorItemSpawnsRowGeometry(float baseX, float rowWidth, float iconSize,
        float gap, float pad, qboolean iconOnRight, float maxTextW,
        float* iconX, float* textCellX, float* textCellW)
{
	float cellW;

	cellW = rowWidth - pad * 2.0f - iconSize - gap;
	if (cellW < maxTextW)
	{
		cellW = maxTextW;
	}
	*textCellW = cellW;

	if (!iconOnRight)
	{
		*iconX = baseX + pad;
		*textCellX = baseX + pad + iconSize + gap;
	}
	else
	{
		*iconX = baseX + rowWidth - pad - iconSize;
		*textCellX = baseX + pad;
	}
}

static void CG_SHUDElementSpectatorItemSpawnsFormatLine(shudElementSpectatorItemSpawns_t* element,
        const char* value, char* line, int lineSize)
{
	const char* affix;
	qboolean postfix;

	if (!value || !value[0])
	{
		line[0] = '\0';
		return;
	}

	affix = element->config.text.isSet ? element->config.text.value : NULL;
	postfix = element->config.style.isSet && element->config.style.value == 1;

	if (!affix || !affix[0])
	{
		Q_strncpyz(line, value, lineSize);
		return;
	}

	if (postfix)
	{
		Com_sprintf(line, lineSize, "%s%s", value, affix);
	}
	else
	{
		Com_sprintf(line, lineSize, "%s%s", affix, value);
	}
}

static float CG_SHUDElementSpectatorItemSpawnsMaxTextWidth(shudElementSpectatorItemSpawns_t* element,
        float charW, float charH)
{
	float w;
	char sample[16];
	char line[64];

	Q_strncpyz(sample, "888", sizeof(sample));
	CG_SHUDElementSpectatorItemSpawnsFormatLine(element, sample, line, sizeof(line));
	w = CG_OSPDrawStringNewTextWidth(line, charW, charH, element->ctx.flags, element->ctx.fontIndex);
	if (w < charW * 3.0f)
	{
		w = charW * 3.0f;
	}
	return w;
}

static void CG_SHUDElementSpectatorItemSpawnsDrawIcon(float x, float y, float size,
        const vec4_t color, qhandle_t shader)
{
	float w;
	float h;

	if (!shader)
	{
		return;
	}

	w = size;
	h = size;
	trap_R_SetColor(color);
	CG_AdjustFrom640_Old(&x, &y, &w, &h, qtrue);
	trap_R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, shader);
	trap_R_SetColor(NULL);
}

static void CG_SHUDElementSpectatorItemSpawnsDrawIconWH(float x, float y, float w, float h,
        const vec4_t color, qhandle_t shader)
{
	if (!shader)
	{
		return;
	}

	trap_R_SetColor(color);
	CG_AdjustFrom640_Old(&x, &y, &w, &h, qtrue);
	trap_R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, shader);
	trap_R_SetColor(NULL);
}

static void CG_SHUDElementSpectatorItemSpawnsDrawSpotBadges(float layoutIconX, float iconY, float iconSize,
        float layoutW, const vec4_t color, const cg_spectatorItemSpotDrawRef_t* spot)
{
	float badgeW;
	float badgeH;
	float badgeX;
	float badgeY;
	float slotRight;
	float slotBottom;
	int j;
	qhandle_t badgeShader;

	if (!spot || spot->overlayCount <= 0)
	{
		return;
	}

	if (layoutW <= 0.0f)
	{
		layoutW = iconSize;
	}

	badgeW = iconSize * SHUD_ITEMSPAWNS_BADGE_WIDTH_SCALE;
	badgeH = iconSize * SHUD_ITEMSPAWNS_BADGE_HEIGHT_SCALE;
	slotRight = layoutIconX + layoutW;
	slotBottom = iconY + iconSize;

	badgeX = slotRight - badgeW;
	if (badgeX < layoutIconX)
	{
		badgeX = layoutIconX;
	}

	badgeY = slotBottom - badgeH;
	if (badgeY < iconY)
	{
		badgeY = iconY;
	}

	for (j = 0; j < spot->overlayCount; j++)
	{
		badgeShader = CG_SpectatorItems_IconShader(spot->overlayGroups[j]);
		CG_SHUDElementSpectatorItemSpawnsDrawIconWH(badgeX, badgeY, badgeW, badgeH, color, badgeShader);
		badgeY -= badgeH * SHUD_ITEMSPAWNS_BADGE_STACK;
		if (badgeY < iconY)
		{
			badgeY = iconY;
		}
	}
}

static void CG_SHUDElementSpectatorItemSpawnsDrawBlock(const superhudConfig_t* cfg,
        float blockX, float blockY, float blockW, float blockH)
{
	superhudConfig_t tmp;
	qboolean drawFill;
	qboolean drawBorder;

	drawFill = cfg->fill.isSet && (cfg->bgcolor.isSet || cfg->image.isSet);
	drawBorder = cfg->border.isSet && cfg->borderColor.isSet;
	if (!drawFill && !drawBorder)
	{
		return;
	}

	if (blockW <= 0.0f || blockH <= 0.0f)
	{
		return;
	}

	memcpy(&tmp, cfg, sizeof(tmp));
	tmp.rect.isSet = qtrue;
	tmp.rect.value[0] = blockX;
	tmp.rect.value[1] = blockY;
	tmp.rect.value[2] = blockW;
	tmp.rect.value[3] = blockH;
	tmp.alignH.isSet = qfalse;
	tmp.alignV.isSet = qfalse;

	if (drawFill)
	{
		/* CG_SHUDFill prefers bgcolor over image; image must win when set */
		if (tmp.image.isSet)
		{
			tmp.bgcolor.isSet = qfalse;
		}
		CG_SHUDFill(&tmp);
	}
	if (drawBorder)
	{
		CG_SHUDDrawBorder(&tmp);
	}
}

void CG_SHUDElementSpectatorItemSpawnsRoutine(void* context)
{
	shudElementSpectatorItemSpawns_t* element;
	const cg_spectatorItemSpotDrawRef_t* spot;
	float baseX;
	float baseY;
	float rowWidth;
	float blockW;
	float charW;
	float charH;
	float iconSize;
	float rowStep;
	float rowGap;
	float cellGap;
	float cellW;
	float cellH;
	float cellInnerW;
	float minCellH;
	float stackExtra;
	float gap;
	float minInner;
	float textCellX;
	float textCellW;
	float rowIconX;
	float rowY;
	float colX;
	float iconY;
	float textRowY;
	float textX;
	float textYOffset;
	float blockH;
	float drawIconX;
	float layoutIconX;
	superhudAlignH_t textAlignH;
	int count;
	int i;
	int spawnTime;
	int ms;
	int sec;
	int group;
	int timerTrackId;
	qboolean iconOnRight;
	qboolean horizontal;
	qboolean iconOnTop;
	vec4_t iconColor;
	qhandle_t shader;
	char num[16];

	element = (shudElementSpectatorItemSpawns_t*)context;

	if (!CG_IS_LOCAL_CLIENT_SPECTATOR())
	{
		return;
	}

	if (!element->config.rect.isSet)
	{
		return;
	}

	if (!element->config.fontsize.isSet)
	{
		return;
	}

	count = CG_SpectatorItems_BuildSpotDrawList(element->trackSideFilter, element->drawRefs,
	                                          CG_ITEM_SPAWNS_DRAW_MAX);
	if (count <= 0)
	{
		return;
	}

	horizontal = (element->config.alignH.isSet &&
	              element->config.alignH.value == SUPERHUD_ALIGNH_CENTER) ? qtrue : qfalse;

	baseX = element->config.rect.value[0];
	baseY = element->config.rect.value[1];
	charW = element->config.fontsize.value[0];
	charH = element->config.fontsize.value[1];
	iconSize = charH * SHUD_ITEMSPAWNS_ICON_SCALE;
	iconOnRight = element->iconOnRight;
	/* vertical: alignh R/L flips icon|text order (Blue default still applies if unset) */
	if (!horizontal && element->config.alignH.isSet)
	{
		if (element->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
		{
			iconOnRight = qtrue;
		}
		else if (element->config.alignH.value == SUPERHUD_ALIGNH_LEFT)
		{
			iconOnRight = qfalse;
		}
	}
	textAlignH = CG_SHUDElementSpectatorItemSpawnsEffectiveTextAlign(&element->config, iconOnRight);

	CG_SHUDConfigPickColor_Init(&element->config, element->ctx.color);
	Vector4Copy(element->ctx.color, iconColor);

	element->ctx.flags &= ~(DS_HLEFT | DS_HCENTER | DS_HRIGHT | DS_VTOP | DS_VCENTER | DS_VBOTTOM);
	if (textAlignH == SUPERHUD_ALIGNH_CENTER)
	{
		element->ctx.flags |= DS_HCENTER;
	}
	else if (textAlignH == SUPERHUD_ALIGNH_RIGHT)
	{
		element->ctx.flags |= DS_HRIGHT;
	}
	else
	{
		element->ctx.flags |= DS_HLEFT;
	}

	if (horizontal)
	{
		float ratio;
		float iconDrawW;

		cellGap = element->config.rect.value[2];
		if (cellGap <= 0.0f)
		{
			cellGap = (float)SHUD_ITEMSPAWNS_ROW_PAD;
		}

		/* DrawIcon uses correctWide: layout X width is iconSize * ratio, not iconSize */
		if (cgs.glconfig.vidWidth <= 0)
		{
			ratio = 1.0f;
		}
		else
		{
			ratio = (cgs.glconfig.vidHeight * 640.0f) / (cgs.glconfig.vidWidth * 480.0f);
		}
		if (ratio <= 0.0f)
		{
			ratio = 1.0f;
		}
		iconDrawW = iconSize * ratio;

		cellInnerW = iconDrawW;
		if (cellInnerW < element->maxTextW)
		{
			cellInnerW = element->maxTextW;
		}
		cellW = cellInnerW + SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f;

		minCellH = SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f + iconSize + charH;
		cellH = element->config.rect.value[3];
		if (cellH <= 0.0f)
		{
			cellH = minCellH;
		}
		else if (cellH < minCellH)
		{
			cellH = minCellH;
		}

		blockW = (float)count * cellW;
		if (count > 1)
		{
			blockW += (float)(count - 1) * cellGap;
		}
		blockH = cellH;

		CG_SHUDElementSpectatorItemSpawnsResolveRect(&element->config, blockW, blockH, &baseX, &baseY,
		        qtrue);
		CG_SHUDElementSpectatorItemSpawnsDrawBlock(&element->config, baseX, baseY, blockW, blockH);

		/* unset/C default: icon top, value bottom; B: icon bottom, value top */
		iconOnTop = qtrue;
		if (element->config.alignV.isSet && element->config.alignV.value == SUPERHUD_ALIGNV_BOTTOM)
		{
			iconOnTop = qfalse;
		}

		stackExtra = cellH - SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f - iconSize - charH;
		if (stackExtra < 0.0f)
		{
			stackExtra = 0.0f;
		}

		/* alignh C: icon and value are centered in each cell */
		element->ctx.flags &= ~(DS_HLEFT | DS_HCENTER | DS_HRIGHT);
		element->ctx.flags |= DS_HCENTER;

		colX = baseX;
		for (i = 0; i < count; i++)
		{
			spot = &element->drawRefs[i];
			group = spot->primaryGroup;
			if (group < 0)
			{
				colX += cellW + cellGap;
				continue;
			}

			layoutIconX = colX + (cellW - iconDrawW) * 0.5f;
			textX = colX + cellW * 0.5f;
			if (iconOnTop)
			{
				iconY = baseY + SHUD_ITEMSPAWNS_EDGE_PAD;
				textRowY = iconY + iconSize + stackExtra;
			}
			else
			{
				textRowY = baseY + SHUD_ITEMSPAWNS_EDGE_PAD;
				iconY = textRowY + charH + stackExtra;
			}

			shader = CG_SpectatorItems_IconShader(group);
			if (shader)
			{
				CG_SHUDElementSpectatorItemSpawnsDrawIcon(layoutIconX, iconY, iconSize, iconColor, shader);
			}

			element->printBuf[0] = '\0';
			timerTrackId = spot->timerTrackId;
			if (timerTrackId >= 0)
			{
				spawnTime = CG_SpectatorItems_GetNextSpawnTime(timerTrackId);
				if (spawnTime > 0)
				{
					ms = CG_SpectatorItems_RemainingMs(timerTrackId);
					if (ms > 0)
					{
						sec = (ms + 999) / 1000;
						if (sec >= 1)
						{
							if (cg_drawTimer.integer == 0 && !CG_IS_LOCAL_CLIENT_SPECTATOR())
							{
								Q_strncpyz(num, "XX", sizeof(num));
							}
							else
							{
								Com_sprintf(num, sizeof(num), "%i", sec);
							}
							CG_SHUDElementSpectatorItemSpawnsFormatLine(element, num, element->printBuf,
							        sizeof(element->printBuf));
						}
					}
				}
			}

			if (element->printBuf[0])
			{
				element->ctx.text = element->printBuf;
				element->ctx.coord.named.x = textX;
				element->ctx.coord.named.y = textRowY;
				element->ctx.coord.named.w = charW;
				element->ctx.coord.named.h = charH;
				CG_SHUDTextPrint(&element->config, &element->ctx);
			}

			if (spot->overlayCount > 0)
			{
				CG_SHUDElementSpectatorItemSpawnsDrawSpotBadges(layoutIconX, iconY, iconSize, iconDrawW,
				        iconColor, spot);
			}

			colX += cellW + cellGap;
		}
		return;
	}

	rowWidth = element->config.rect.value[2];
	rowGap = element->config.rect.value[3];
	if (rowGap < 0.0f)
	{
		rowGap = 0.0f;
	}
	if (rowGap == 0.0f)
	{
		rowStep = iconSize + (float)SHUD_ITEMSPAWNS_ROW_PAD;
	}
	else
	{
		rowStep = iconSize + rowGap;
	}

	minInner = SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f + iconSize + SHUD_ITEMSPAWNS_MIN_GAP + element->maxTextW;
	gap = SHUD_ITEMSPAWNS_MIN_GAP;
	if (rowWidth <= 0.0f)
	{
		rowWidth = minInner;
	}

	blockH = SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f + (float)count * rowStep;
	if (count > 0)
	{
		blockH -= (rowGap == 0.0f) ? (float)SHUD_ITEMSPAWNS_ROW_PAD : rowGap;
	}
	if (blockH < iconSize + SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f)
	{
		blockH = iconSize + SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f;
	}

	CG_SHUDElementSpectatorItemSpawnsResolveRect(&element->config, rowWidth, blockH, &baseX, &baseY,
	        qfalse);
	CG_SHUDElementSpectatorItemSpawnsRowGeometry(baseX, rowWidth, iconSize, gap, SHUD_ITEMSPAWNS_EDGE_PAD,
	        iconOnRight, element->maxTextW, &rowIconX, &textCellX, &textCellW);
	CG_SHUDElementSpectatorItemSpawnsDrawBlock(&element->config, baseX, baseY, rowWidth, blockH);

	if (textAlignH == SUPERHUD_ALIGNH_CENTER)
	{
		textX = textCellX + textCellW * 0.5f;
	}
	else if (textAlignH == SUPERHUD_ALIGNH_RIGHT)
	{
		textX = textCellX + textCellW;
	}
	else
	{
		textX = textCellX;
	}

	/* vertical row: always center value against icon; alignv is block anchor only here */
	textYOffset = (iconSize - charH) * 0.5f;

	drawIconX = rowIconX;
	if (iconOnRight)
	{
		float ratio;

		if (cgs.glconfig.vidWidth <= 0)
		{
			ratio = 1.0f;
		}
		else
		{
			ratio = (cgs.glconfig.vidHeight * 640.0f) / (cgs.glconfig.vidWidth * 480.0f);
		}
		if (ratio < 1.0f)
		{
			drawIconX = rowIconX + iconSize * (1.0f - ratio);
		}
	}

	rowY = baseY + SHUD_ITEMSPAWNS_EDGE_PAD;
	for (i = 0; i < count; i++)
	{
		spot = &element->drawRefs[i];
		group = spot->primaryGroup;
		if (group < 0)
		{
			rowY += rowStep;
			continue;
		}

		shader = CG_SpectatorItems_IconShader(group);
		iconY = rowY;
		if (shader)
		{
			CG_SHUDElementSpectatorItemSpawnsDrawIcon(drawIconX, iconY, iconSize, iconColor, shader);
		}

		element->printBuf[0] = '\0';
		timerTrackId = spot->timerTrackId;
		if (timerTrackId >= 0)
		{
			spawnTime = CG_SpectatorItems_GetNextSpawnTime(timerTrackId);
			if (spawnTime > 0)
			{
				ms = CG_SpectatorItems_RemainingMs(timerTrackId);
				if (ms > 0)
				{
					sec = (ms + 999) / 1000;
					if (sec >= 1)
					{
						if (cg_drawTimer.integer == 0 && !CG_IS_LOCAL_CLIENT_SPECTATOR())
						{
							Q_strncpyz(num, "XX", sizeof(num));
						}
						else
						{
							Com_sprintf(num, sizeof(num), "%i", sec);
						}
						CG_SHUDElementSpectatorItemSpawnsFormatLine(element, num, element->printBuf,
						        sizeof(element->printBuf));
					}
				}
			}
		}

		if (element->printBuf[0])
		{
			textRowY = rowY + textYOffset;

			element->ctx.text = element->printBuf;
			element->ctx.coord.named.x = textX;
			element->ctx.coord.named.y = textRowY;
			element->ctx.coord.named.w = charW;
			element->ctx.coord.named.h = charH;
			CG_SHUDTextPrint(&element->config, &element->ctx);
		}

		if (spot->overlayCount > 0)
		{
			CG_SHUDElementSpectatorItemSpawnsDrawSpotBadges(rowIconX, iconY, iconSize, iconSize, iconColor,
			        spot);
		}

		rowY += rowStep;
	}
}

void CG_SHUDElementSpectatorItemSpawnsDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
