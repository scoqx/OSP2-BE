#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	float baseX;
	float baseY;
	float lineH;
	int maxRows; /* rect h: max player name rows; 0 = unlimited */
} shudElementSpectators_t;

void* CG_SHUDElementSpectatorsCreate(const superhudConfig_t* config)
{
	shudElementSpectators_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	element->baseX = element->ctx.coord.named.x;
	element->baseY = element->ctx.coord.named.y;
	element->lineH = element->config.fontsize.isSet
	                 ? element->config.fontsize.value[1]
	                 : element->ctx.coord.named.h;
	element->maxRows = 0;
	if (element->config.rect.isSet && element->config.rect.value[3] > 0.0f)
	{
		element->maxRows = (int)element->config.rect.value[3];
	}

	return element;
}

void CG_SHUDElementSpectatorsRoutine(void* context)
{
	shudElementSpectators_t* element = (shudElementSpectators_t*)context;
	int ids[MAX_CLIENTS];
	int count;
	int visible;
	int i;
	int line;
	int totalLines;
	qboolean showHeader;
	float startY;
	float blockH;
	float lineH;

	if (!cg.beFollowingMe)
	{
		return;
	}

	count = 0;
	for (i = 0; i < MAX_CLIENTS && i < 32; i++)
	{
		if (!(cg.beFollowingMe & (1 << i)))
		{
			continue;
		}
		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}
		ids[count++] = i;
	}

	if (count <= 0)
	{
		return;
	}

	visible = count;
	if (element->maxRows > 0 && visible > element->maxRows)
	{
		visible = element->maxRows;
	}

	showHeader = !(element->config.style.isSet && element->config.style.value == 2);
	totalLines = visible + (showHeader ? 1 : 0);
	lineH = element->lineH;
	if (lineH <= 0.0f)
	{
		lineH = 10.0f;
	}

	blockH = totalLines * lineH;
	startY = element->baseY;

	/* Block-level alignv: position stack relative to rect origin (like itemspawns) */
	if (element->config.alignV.isSet)
	{
		switch (element->config.alignV.value)
		{
			default:
			case SUPERHUD_ALIGNV_TOP:
				break;
			case SUPERHUD_ALIGNV_CENTER:
				startY = element->baseY - blockH * 0.5f;
				break;
			case SUPERHUD_ALIGNV_BOTTOM:
				startY = element->baseY - blockH;
				break;
		}
	}

	/* Per-line VTOP so stacking is by explicit Y, not draw-string valign */
	element->ctx.flags &= ~(DS_VTOP | DS_VCENTER | DS_VBOTTOM);
	element->ctx.flags |= DS_VTOP;

	line = 0;
	if (showHeader)
	{
		element->ctx.coord.named.x = element->baseX;
		element->ctx.coord.named.y = startY + line * lineH;
		element->ctx.text = "Spectators:";
		CG_SHUDTextPrint(&element->config, &element->ctx);
		line++;
	}

	for (i = 0; i < visible; i++)
	{
		element->ctx.coord.named.x = element->baseX;
		element->ctx.coord.named.y = startY + line * lineH;
		element->ctx.text = cgs.clientinfo[ids[i]].name;
		CG_SHUDTextPrint(&element->config, &element->ctx);
		line++;
	}
}

void CG_SHUDElementSpectatorsDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
