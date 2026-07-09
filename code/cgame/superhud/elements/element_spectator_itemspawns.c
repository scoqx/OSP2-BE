#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

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

static void CG_SHUDElementSpectatorItemSpawnsResolveRect(const superhudConfig_t* cfg,
        float contentW, float contentH, float* outX, float* outY)
{
	superhudAlignH_t h;
	superhudAlignV_t v;
	float x;
	float y;

	x = cfg->rect.value[0];
	y = cfg->rect.value[1];

	if (cfg->alignH.isSet)
	{
		h = cfg->alignH.value;
	}
	else
	{
		h = SUPERHUD_ALIGNH_LEFT;
	}

	if (cfg->alignV.isSet)
	{
		v = cfg->alignV.value;
	}
	else
	{
		v = SUPERHUD_ALIGNV_TOP;
	}

	switch (h)
	{
		default:
		case SUPERHUD_ALIGNH_LEFT:
			break;
		case SUPERHUD_ALIGNH_CENTER:
			x -= contentW * 0.5f;
			break;
		case SUPERHUD_ALIGNH_RIGHT:
			x -= contentW;
			break;
	}

	switch (v)
	{
		default:
		case SUPERHUD_ALIGNV_TOP:
			break;
		case SUPERHUD_ALIGNV_CENTER:
			y -= contentH * 0.5f;
			break;
		case SUPERHUD_ALIGNV_BOTTOM:
			y -= contentH;
			break;
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
        const vec4_t color, const cg_spectatorItemSpotDrawRef_t* spot)
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

	badgeW = iconSize * SHUD_ITEMSPAWNS_BADGE_WIDTH_SCALE;
	badgeH = iconSize * SHUD_ITEMSPAWNS_BADGE_HEIGHT_SCALE;
	slotRight = layoutIconX + iconSize;
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
	float charW;
	float charH;
	float iconSize;
	float rowStep;
	float gap;
	float minInner;
	float textCellX;
	float textCellW;
	float rowIconX;
	float rowY;
	float iconY;
	float textRowY;
	float textX;
	float textYOffset;
	float blockH;
	float drawIconX;
	superhudAlignH_t textAlignH;
	superhudAlignV_t textAlignV;
	int count;
	int i;
	int spawnTime;
	int ms;
	int sec;
	int group;
	int timerTrackId;
	qboolean iconOnRight;
	vec4_t iconColor;
	qhandle_t shader;
	char num[16];

	element = (shudElementSpectatorItemSpawns_t*)context;

	// if (!CG_IS_LOCAL_CLIENT_SPECTATOR())
	// {
	// 	return;
	// }

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

	baseX = element->config.rect.value[0];
	baseY = element->config.rect.value[1];
	rowWidth = element->config.rect.value[2];
	charW = element->config.fontsize.value[0];
	charH = element->config.fontsize.value[1];
	iconSize = charH * SHUD_ITEMSPAWNS_ICON_SCALE;
	rowStep = iconSize + SHUD_ITEMSPAWNS_ROW_PAD;
	iconOnRight = element->iconOnRight;

	minInner = SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f + iconSize + SHUD_ITEMSPAWNS_MIN_GAP + element->maxTextW;
	gap = SHUD_ITEMSPAWNS_MIN_GAP;
	if (rowWidth <= 0.0f)
	{
		rowWidth = minInner;
	}

	blockH = SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f + (float)count * rowStep - SHUD_ITEMSPAWNS_ROW_PAD;
	if (blockH < iconSize + SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f)
	{
		blockH = iconSize + SHUD_ITEMSPAWNS_EDGE_PAD * 2.0f;
	}

	CG_SHUDElementSpectatorItemSpawnsResolveRect(&element->config, rowWidth, blockH, &baseX, &baseY);

	textAlignH = CG_SHUDElementSpectatorItemSpawnsEffectiveTextAlign(&element->config, iconOnRight);
	CG_SHUDElementSpectatorItemSpawnsRowGeometry(baseX, rowWidth, iconSize, gap, SHUD_ITEMSPAWNS_EDGE_PAD,
	        iconOnRight, element->maxTextW, &rowIconX, &textCellX, &textCellW);

	CG_SHUDElementSpectatorItemSpawnsDrawBlock(&element->config, baseX, baseY, rowWidth, blockH);

	CG_SHUDConfigPickColor_Init(&element->config, element->ctx.color);
	Vector4Copy(element->ctx.color, iconColor);

	/* text H-align flags, text anchor X and icon draw X are the same for every row: resolve once */
	element->ctx.flags &= ~(DS_HLEFT | DS_HCENTER | DS_HRIGHT | DS_VTOP | DS_VCENTER | DS_VBOTTOM);
	if (textAlignH == SUPERHUD_ALIGNH_CENTER)
	{
		element->ctx.flags |= DS_HCENTER;
		textX = textCellX + textCellW * 0.5f;
	}
	else if (textAlignH == SUPERHUD_ALIGNH_RIGHT)
	{
		element->ctx.flags |= DS_HRIGHT;
		textX = textCellX + textCellW;
	}
	else
	{
		element->ctx.flags |= DS_HLEFT;
		textX = textCellX;
	}

	textAlignV = element->config.alignV.isSet ? element->config.alignV.value : SUPERHUD_ALIGNV_CENTER;
	if (textAlignV == SUPERHUD_ALIGNV_TOP)
	{
		textYOffset = 0.0f;
	}
	else if (textAlignV == SUPERHUD_ALIGNV_BOTTOM)
	{
		textYOffset = iconSize - charH;
	}
	else
	{
		textYOffset = (iconSize - charH) * 0.5f;
	}

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
			CG_SHUDElementSpectatorItemSpawnsDrawSpotBadges(rowIconX, iconY, iconSize, iconColor, spot);
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
