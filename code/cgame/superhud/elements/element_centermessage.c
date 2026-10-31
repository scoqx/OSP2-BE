#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

#define SHUD_CENTERMSG_WRAP_DEFAULT 40
#define SHUD_CENTERMSG_WRAP_MAX     255

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	qboolean useImage;
	float baseX;
	float baseY;
	float lineH;
	int wrapChars;
} shudElementCenterMessage_t;

static int shudCenterMessageInstances;

qboolean CG_SHUDHasCenterMessage(void)
{
	return shudCenterMessageInstances > 0 ? qtrue : qfalse;
}

void* CG_SHUDElementCenterMessageCreate(const superhudConfig_t* config)
{
	shudElementCenterMessage_t* element;

	SHUD_ELEMENT_INIT(element, config);

	if (!element->config.time.isSet)
	{
		element->config.time.isSet = qtrue;
		element->config.time.value = 3000;
	}

	if (config->image.isSet)
	{
		element->useImage = qtrue;
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
		element->drawCtx.image = trap_R_RegisterShader(element->config.image.value);
		if (!element->drawCtx.image)
		{
			CG_Printf("^1CenterMessage image %s is not found\n", element->config.image.value);
		}
	}

	if (!element->useImage)
	{
		if (!element->config.textAlign.isSet)
		{
			element->config.textAlign.value = SUPERHUD_ALIGNH_CENTER;
			element->config.textAlign.isSet = qtrue;
		}

		CG_SHUDTextMakeContext(&element->config, &element->ctx);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

		element->baseX = element->ctx.coord.named.x;
		element->baseY = element->ctx.coord.named.y;
		element->lineH = element->config.fontsize.isSet
		                 ? element->config.fontsize.value[1]
		                 : element->ctx.coord.named.h;

		element->wrapChars = (int)element->config.rect.value[2];
		if (element->wrapChars <= 0)
		{
			element->wrapChars = SHUD_CENTERMSG_WRAP_DEFAULT;
		}
		else if (element->wrapChars > SHUD_CENTERMSG_WRAP_MAX)
		{
			element->wrapChars = SHUD_CENTERMSG_WRAP_MAX;
		}
	}

	shudCenterMessageInstances++;
	return element;
}

void CG_SHUDElementCenterMessageRoutine(void* context)
{
	shudElementCenterMessage_t* element = (shudElementCenterMessage_t*)context;
	char* to_print;
	char* end;
	float startY;
	float lineH;
	float y;
	float fadeAlpha;
	int lineCount;
	int i;
	int space;
	int wrapChars;
	char buf[SHUD_CENTERMSG_WRAP_MAX + 1];

	if (!cg_drawCenterMessages.integer)
	{
		return;
	}

	if (!cg.centerPrintTime)
	{
		return;
	}

	if (element->useImage)
	{
		if (!element->drawCtx.image)
		{
			return;
		}
		if (!CG_SHUDGetFadeColor(element->drawCtx.color_origin, element->drawCtx.color, &element->config, cg.centerPrintTime))
		{
			return;
		}
		fadeAlpha = CG_SHUDFadeAlphaScale(element->drawCtx.color_origin, element->drawCtx.color);
		CG_SHUDFillFade(&element->config, fadeAlpha);
		CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		CG_SHUDDrawBorderFade(&element->config, fadeAlpha);
		return;
	}

	if (!CG_SHUDGetFadeColor(element->ctx.color_origin, element->ctx.color, &element->config, cg.centerPrintTime))
	{
		return;
	}

	lineH = element->lineH;
	if (lineH <= 0.0f)
	{
		lineH = 10.0f;
	}

	wrapChars = element->wrapChars;
	if (wrapChars <= 0)
	{
		wrapChars = SHUD_CENTERMSG_WRAP_DEFAULT;
	}
	else if (wrapChars > SHUD_CENTERMSG_WRAP_MAX)
	{
		wrapChars = SHUD_CENTERMSG_WRAP_MAX;
	}

	/* Count wrapped lines for block-level alignV. */
	lineCount = 0;
	to_print = cg.centerPrint;
	end = cg.centerPrint + sizeof(cg.centerPrint);
	while (to_print < end && *to_print != 0)
	{
		space = 0;
		i = 0;
		do
		{
			if (to_print[i] == ' ')
			{
				space = i;
			}
			if (!to_print[i] || to_print[i] == '\n')
			{
				space = i;
				break;
			}
		}
		while (++i < wrapChars);

		if (space && space != i)
		{
			to_print += space + 1;
		}
		else
		{
			to_print += i + 1;
		}
		lineCount++;
	}

	startY = element->baseY;
	if (element->config.alignV.isSet)
	{
		switch (element->config.alignV.value)
		{
			default:
			case SUPERHUD_ALIGNV_TOP:
				break;
			case SUPERHUD_ALIGNV_CENTER:
				startY = element->baseY - (float)lineCount * lineH * 0.5f;
				break;
			case SUPERHUD_ALIGNV_BOTTOM:
				startY = element->baseY - (float)lineCount * lineH;
				break;
		}
	}

	element->ctx.flags &= ~(DS_VTOP | DS_VCENTER | DS_VBOTTOM);
	element->ctx.flags |= DS_VTOP;

	y = startY;
	to_print = cg.centerPrint;
	while (to_print < end && *to_print != 0)
	{
		space = 0;
		i = 0;
		do
		{
			if (to_print[i] == ' ')
			{
				space = i;
			}
			if (!to_print[i] || to_print[i] == '\n')
			{
				space = i;
				break;
			}
			buf[i] = to_print[i];
		}
		while (++i < wrapChars);

		if (space && space != i)
		{
			buf[space] = 0;
			to_print += space + 1;
		}
		else
		{
			buf[i] = 0;
			to_print += i + 1;
		}

		element->ctx.coord.named.x = element->baseX;
		element->ctx.coord.named.y = y;
		element->ctx.text = buf;
		CG_SHUDTextPrintNew(&element->config, &element->ctx, qfalse);
		y += lineH;
	}
}

void CG_SHUDElementCenterMessageDestroy(void* context)
{
	if (shudCenterMessageInstances > 0)
	{
		shudCenterMessageInstances--;
	}
	if (context)
	{
		Z_Free(context);
	}
}
