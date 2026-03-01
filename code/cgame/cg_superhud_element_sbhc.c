
#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementStatusbarHealthCount;

void* CG_SHUDElementSBHCCreate(const superhudConfig_t* config)
{
	shudElementStatusbarHealthCount* element;

	SHUD_ELEMENT_INIT(element, config);

	// //load defaults
	// if (!element->config.color.isSet)
	// {
	//  element->config.color.isSet = qtrue;
	//  element->config.color.value.type = SUPERHUD_COLOR_RGBA;
	//  Vector4Set(element->config.color.value.rgba, 1, 0.7, 0, 1);
	// }
	if (!element->config.text.isSet)
	{
		element->config.text.isSet = qtrue;
		Q_strncpyz(element->config.text.value, "%i", sizeof(element->config.text.value));
	}

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	element->ctx.flags |= DS_FORCE_COLOR;

	return element;
}

void CG_SHUDElementSBHCRoutine(void* context)
{
	shudElementStatusbarHealthCount* element = (shudElementStatusbarHealthCount*)context;
	int hp = cg.snap->ps.stats[STAT_HEALTH];

	element->ctx.text = va(element->config.text.value, hp > 0 ? hp : 0);

	if (hp <= 0 && element->config.color.isSet)
		Vector4Copy(element->config.color.value.rgba, element->ctx.color);
	else
		Vector4Copy(cg.healthColorLocal, element->ctx.color);
	CG_SHUDTextPrintNew(&element->config, &element->ctx, qfalse);
}

void CG_SHUDElementSBHCDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
