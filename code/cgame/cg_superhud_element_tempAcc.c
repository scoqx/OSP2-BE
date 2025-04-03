#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

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
	if (config->style.isSet)
	{
		if (config->style.value == 1)
		{
			const vec4_t colors[] =
			{
				{1.0f, 0.0f, 0.0f, 1.0f}, // < 30%
				{1.0f, 1.0f, 0.0f, 1.0f}, // < 50%
				{0.0f, 1.0f, 0.0f, 1.0f}, // < 60%
				{1.0f, 0.0f, 1.0f, 1.0f}  // >= 60%
			};

			int index = (accuracy < 30.0f) ? 0 :
			            (accuracy < 50.0f) ? 1 :
			            (accuracy < 60.0f) ? 2 : 3;

			Vector4Copy(colors[index], color);
		}
		else
		{
			Vector4Copy(config->color.value.rgba, color);
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

	if (cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE) // ignore promode
	{
		return;
	}

	CG_GetWeaponTempAccuracy(WP_LIGHTNING);

	Com_sprintf(accuracyStr, sizeof(accuracyStr), "%.0f%%", entry->tempAccuracy);
	element->ctx.text = va("%s", accuracyStr);


	CG_SHUDEStylesTempAcc_Color(color, &element->config, entry->tempAccuracy);

	Vector4Copy(color, element->config.color.value.rgba);

	if (!entry->tempAccuracy <= 0)
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
