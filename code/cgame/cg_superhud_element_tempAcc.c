#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

#define SHUD_STYLE_FLOAT       (1 << 0)
#define SHUD_STYLE_COLORIZED   (1 << 1)

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudGlobalContext_t* gctx;
}
shudElementTempAcc_t;

static void* CG_SHUDElementTempAccCreate(const superhudConfig_t* config)
{
	shudElementTempAcc_t* element;

	SHUD_ELEMENT_INIT(element, config);

	element->gctx = CG_SHUDGetContext();

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

static void CG_SHUDEStylesTempAcc_Color(vec4_t color, const superhudConfig_t* config, float accuracy)
{
	if (config->style.isSet && (config->style.value & SHUD_STYLE_COLORIZED))
	{
		if (accuracy < 30.0f)
		{
			Vector4Copy(colorRed, color);
		}
		else if (accuracy < 50.0f)
		{
			Vector4Copy(colorYellow, color);
		}
		else if (accuracy < 60.0f)
		{
			Vector4Copy(colorGreen, color);
		}
		else
		{
			Vector4Copy(colorMagenta, color);
		}
	}
	else
	{
		Vector4Copy(config->color.value.rgba, color);
	}
}

void CG_SHUDElementTempAccRoutine(void* context)
{
	shudElementTempAcc_t* element = (shudElementTempAcc_t*)context;
	char accuracyStr[8];
	vec4_t color;

	superhudTempAccEntry_t* entry = &element->gctx->tempAcc.weapon[WP_LIGHTNING];

	if (cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE)
	{
		return;
	}

	CG_GetWeaponTempAccuracy(WP_LIGHTNING);

	if (element->config.style.value & SHUD_STYLE_FLOAT)
		Com_sprintf(accuracyStr, sizeof(accuracyStr), "%.1f%%", entry->tempAccuracy);
	else
		Com_sprintf(accuracyStr, sizeof(accuracyStr), "%.0f%%", entry->tempAccuracy);

	element->ctx.text = va("%s", accuracyStr);

	CG_SHUDEStylesTempAcc_Color(color, &element->config, entry->tempAccuracy);

	Vector4Copy(color, element->config.color.value.rgba);

	if (entry->tempAccuracy > 0)
	{
		CG_SHUDTextPrint(&element->config, &element->ctx);
	}
}



void CG_SHUDElementTempAccDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
