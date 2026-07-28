#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctxPicker;
	superhudDrawContext_t ctxIcon;
	superhudTextContext_t ctxTime;
	superhudGlobalContext_t* gctx;
	int index;
} shudElementSpectatorItemPickups_t;

static void* CG_SHUDElementSpectatorItemPickupsCreate(const superhudConfig_t* config, int line)
{
	shudElementSpectatorItemPickups_t* element;
	superhudConfig_t lcfg;

	SHUD_ELEMENT_INIT(element, config);
	element->gctx = &superhudGlobalContext;
	element->index = line;

	memcpy(&lcfg, config, sizeof(superhudConfig_t));

	if (!lcfg.alignV.isSet)
	{
		lcfg.alignV.value = SUPERHUD_ALIGNV_CENTER;
		lcfg.alignV.isSet = qtrue;
	}

	CG_SHUDTextMakeContext(&lcfg, &element->ctxPicker);
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxIcon);
	CG_SHUDTextMakeContext(&lcfg, &element->ctxTime);

	if (!element->config.fontsize.isSet)
	{
		element->config.fontsize.value[0] = element->ctxPicker.coord.named.w;
		element->config.fontsize.value[1] = element->ctxPicker.coord.named.h;
		element->config.fontsize.isSet = qtrue;
	}

	return element;
}

void* CG_SHUDElementSpectatorItemPickups1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 1);
}

void* CG_SHUDElementSpectatorItemPickups2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 2);
}

void* CG_SHUDElementSpectatorItemPickups3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 3);
}

void* CG_SHUDElementSpectatorItemPickups4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 4);
}

void* CG_SHUDElementSpectatorItemPickups5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 5);
}

void* CG_SHUDElementSpectatorItemPickups6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 6);
}

void* CG_SHUDElementSpectatorItemPickups7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 7);
}

void* CG_SHUDElementSpectatorItemPickups8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementSpectatorItemPickupsCreate(config, 8);
}

static void CG_SHUDSpectatorItemPickupsClearHasActiveIfEmpty(superhudGlobalContext_t* ctx)
{
	int i;

	if (!ctx->spectatorItemPickups.hasActive)
	{
		return;
	}

	for (i = 0; i < SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES; i++)
	{
		if (ctx->spectatorItemPickups.line[i].time != 0)
		{
			return;
		}
	}

	ctx->spectatorItemPickups.hasActive = qfalse;
}

static void CG_SHUDElementSpectatorItemPickupsUpdatePosition(shudElementSpectatorItemPickups_t* element,
        superhudSpectatorItemPickupEntry_t* entry, float iconWidth)
{
	float pickerWidth;
	float timeWidth;
	float fontW;
	float fontH;

	fontW = element->ctxPicker.coord.named.w;
	fontH = element->ctxPicker.coord.named.h;
	if (fontW <= 0.0f)
	{
		fontW = element->config.fontsize.value[0];
	}
	if (fontH <= 0.0f)
	{
		fontH = element->config.fontsize.value[1];
	}

	pickerWidth = CG_OSPDrawStringNewTextWidth(entry->runtime.pickerName,
	              fontW, fontH, element->ctxPicker.flags, element->ctxPicker.fontIndex);
	if (pickerWidth > (float)entry->runtime.maxNameLenPix)
	{
		pickerWidth = (float)entry->runtime.maxNameLenPix;
	}

	fontW = element->ctxTime.coord.named.w;
	fontH = element->ctxTime.coord.named.h;
	if (fontW <= 0.0f)
	{
		fontW = element->config.fontsize.value[0];
	}
	if (fontH <= 0.0f)
	{
		fontH = element->config.fontsize.value[1];
	}

	timeWidth = CG_OSPDrawStringNewTextWidth(entry->runtime.timeString,
	            fontW, fontH, element->ctxTime.flags, element->ctxTime.fontIndex);

	entry->runtime.pickerWidth = pickerWidth;
	entry->runtime.timeWidth = timeWidth;

	if (element->config.alignH.value == SUPERHUD_ALIGNH_LEFT)
	{
		entry->runtime.baseX = element->config.rect.value[0];
	}
	else if (element->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
	{
		float totalWidth;

		totalWidth = entry->runtime.spacing * 2 + pickerWidth + iconWidth + timeWidth;
		entry->runtime.baseX = element->config.rect.value[0] - totalWidth;
	}
	else
	{
		float totalWidth;

		totalWidth = entry->runtime.spacing * 2 + pickerWidth + iconWidth + timeWidth;
		entry->runtime.baseX = element->config.rect.value[0] - (totalWidth / 2);
	}
}

static void CG_SHUDElementSpectatorItemPickupsInitializeRuntime(shudElementSpectatorItemPickups_t* element,
        superhudSpectatorItemPickupEntry_t* entry)
{
	int msec;
	int seconds;
	int mins;
	int tens;
	clientInfo_t* ci;

	entry->runtime.spacing = element->config.rect.value[3] / 4.0f;
	entry->runtime.maxNameLenPix = (element->config.rect.value[2] -
	                                (element->config.rect.value[3] + entry->runtime.spacing * 2 + 48)) / 2;
	if (entry->runtime.maxNameLenPix < 0)
	{
		entry->runtime.maxNameLenPix = 0;
	}

	msec = entry->gameTime;
	if (msec < 0)
	{
		msec *= -1;
	}
	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;
	Com_sprintf(entry->runtime.timeString, sizeof(entry->runtime.timeString), "%i:%i%i", mins, tens, seconds);

	entry->runtime.iconShader = CG_SpectatorItems_IconShader(entry->group);

	if (entry->pickerClient >= 0 && entry->pickerClient < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[entry->pickerClient];
		if (ci->infoValid)
		{
			if (element->config.style.isSet && element->config.style.value == 1)
			{
				Q_strncpyz(entry->runtime.pickerName, ci->name_clean, MAX_QPATH);
			}
			else
			{
				Q_strncpyz(entry->runtime.pickerName, ci->name, MAX_QPATH);
			}
		}
		else
		{
			entry->runtime.pickerName[0] = '\0';
		}
	}
	else
	{
		entry->runtime.pickerName[0] = '\0';
	}

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		Vector4Copy(colorWhite, entry->runtime.pickerColor);
	}

	entry->runtime.isInitialized = qtrue;
}

static superhudSpectatorItemPickupEntry_t* CG_SHUDElementSpectatorItemPickupsFindEntry(
        shudElementSpectatorItemPickups_t* element)
{
	superhudGlobalContext_t* ctx;
	int i;
	int checkIndex;
	int visibleSlot;
	superhudSpectatorItemPickupEntry_t* entry;

	ctx = element->gctx;
	visibleSlot = 0;

	for (i = 0; i < SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES; i++)
	{
		checkIndex = (ctx->spectatorItemPickups.index - 1 - i + SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES) %
		             SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES;
		entry = &ctx->spectatorItemPickups.line[checkIndex];

		if (entry->time == 0)
		{
			continue;
		}

		if (!CG_SHUDGetFadeColor(element->ctxPicker.color_origin, element->ctxPicker.color, &element->config,
		                         entry->time))
		{
			entry->time = 0;
			CG_SHUDSpectatorItemPickupsClearHasActiveIfEmpty(ctx);
			continue;
		}

		++visibleSlot;
		if (visibleSlot == element->index)
		{
			return entry;
		}
	}

	return NULL;
}

void CG_SHUDElementSpectatorItemPickupsRoutine(void* context)
{
	shudElementSpectatorItemPickups_t* element;
	superhudSpectatorItemPickupEntry_t* entry;
	float currentX;
	float fadeScale;
	float iconWidth;

	element = (shudElementSpectatorItemPickups_t*)context;

	if (!CG_IS_LOCAL_CLIENT_SPECTATOR())
	{
		return;
	}

	if (!element->config.rect.isSet)
	{
		return;
	}

	if (!element->gctx->spectatorItemPickups.hasActive)
	{
		return;
	}

	entry = CG_SHUDElementSpectatorItemPickupsFindEntry(element);
	if (!entry)
	{
		return;
	}

	fadeScale = CG_SHUDFadeAlphaScale(element->ctxPicker.color_origin, element->ctxPicker.color);

	element->ctxIcon.coord.named.w = element->config.rect.value[3];
	element->ctxIcon.coord.named.h = element->config.rect.value[3];
	iconWidth = element->ctxIcon.coord.named.h * 0.75f;

	if (!entry->runtime.isInitialized)
	{
		CG_SHUDElementSpectatorItemPickupsInitializeRuntime(element, entry);
	}

	CG_SHUDElementSpectatorItemPickupsUpdatePosition(element, entry, iconWidth);

	currentX = entry->runtime.baseX;

	{
		float totalWidth;
		float bgX;
		float bgY;
		float bgW;
		float bgH;
		superhudCoord_t bgCoord;
		vec4_t bgColor;
		vec4_t borderColor;

		totalWidth = entry->runtime.spacing * 2 + entry->runtime.pickerWidth + iconWidth +
		             entry->runtime.timeWidth;

		bgX = entry->runtime.baseX;
		bgY = element->config.rect.value[1];
		bgW = totalWidth;
		bgH = element->config.rect.value[3];

		if (element->config.fill.isSet && element->config.bgcolor.isSet)
		{
			float fadeAlpha;

			bgCoord.named.x = bgX;
			bgCoord.named.y = bgY;
			bgCoord.named.w = bgW;
			bgCoord.named.h = bgH;

			fadeAlpha = fadeScale;
			CG_SHUDConfigPickBgColor(&element->config, bgColor, qfalse);
			if (fadeAlpha != 1.0f)
			{
				bgColor[3] *= fadeAlpha;
			}

			CG_SHUDFillWithColor(&bgCoord, bgColor);
		}

		if (element->config.border.isSet && element->config.borderColor.isSet)
		{
			float borderX;
			float borderY;
			float borderW;
			float borderH;
			float fadeAlpha;

			borderX = bgX;
			borderY = bgY;
			borderW = bgW;
			borderH = bgH;

			CG_AdjustFrom640(&borderX, &borderY, &borderW, &borderH);

			fadeAlpha = fadeScale;
			CG_SHUDConfigPickBorderColor(&element->config, borderColor, qfalse);
			if (fadeAlpha != 1.0f)
			{
				borderColor[3] *= fadeAlpha;
			}

			CG_OSPDrawFrame(borderX, borderY, borderW, borderH,
			                (float*)element->config.border.value, borderColor, qfalse);
		}
	}

	element->ctxPicker.text = entry->runtime.pickerName;
	element->ctxPicker.coord.named.x = currentX;
	element->ctxPicker.background[3] = 0;

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		Vector4Copy(entry->runtime.pickerColor, element->ctxPicker.color);
	}
	else
	{
		Vector4Copy(element->ctxPicker.color_origin, element->ctxPicker.color);
		CG_SHUDConfigPickColor_Runtime(&element->config, element->ctxPicker.color);
	}

	if (fadeScale != 1.0f)
	{
		element->ctxPicker.color[3] *= fadeScale;
	}

	element->ctxPicker.width = entry->runtime.maxNameLenPix;
	CG_SHUDTextPrintNew(&element->config, &element->ctxPicker, qfalse);
	currentX += entry->runtime.pickerWidth + entry->runtime.spacing;

	if (entry->runtime.iconShader)
	{
		element->ctxIcon.image = entry->runtime.iconShader;
		element->ctxIcon.coord.named.x = currentX;
		element->ctxIcon.coord.named.w = iconWidth;
		Vector4Copy(element->ctxIcon.color_origin, element->ctxIcon.color);
		CG_SHUDConfigPickColor_Runtime(&element->config, element->ctxIcon.color);
		if (fadeScale != 1.0f)
		{
			element->ctxIcon.color[3] *= fadeScale;
		}
		CG_SHUDDrawStretchPic(element->ctxIcon.coord, element->ctxIcon.coordPicture, element->ctxIcon.color,
		                      element->ctxIcon.image);
		currentX += element->ctxIcon.coord.named.w + entry->runtime.spacing;
	}

	element->ctxTime.text = entry->runtime.timeString;
	element->ctxTime.coord.named.x = currentX;
	Vector4Copy(element->ctxPicker.color, element->ctxTime.color);
	CG_SHUDTextPrintNew(&element->config, &element->ctxTime, qfalse);
}

void CG_SHUDElementSpectatorItemPickupsDestroy(void* context)
{
	Z_Free(context);
}
