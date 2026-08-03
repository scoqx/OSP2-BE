#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	int* time;
} shudElementKillConfirm_t;

void* CG_SHUDElementKillConfirmCreate(const superhudConfig_t* config)
{
	shudElementKillConfirm_t* element;
	superhudGlobalContext_t* gctx;

	SHUD_ELEMENT_INIT(element, config);

	if (!element->config.time.isSet)
	{
		element->config.time.isSet = qtrue;
		element->config.time.value = 2000;
	}

	gctx = &superhudGlobalContext;
	element->time = &gctx->killconfirm.time;

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	element->ctx.text = gctx->killconfirm.message;

	return element;
}

void CG_SHUDElementKillConfirmRoutine(void* context)
{
	shudElementKillConfirm_t* element = (shudElementKillConfirm_t*)context;

	if (!cgs.killConfirmMode || !*element->time)
	{
		return;
	}

	if (!CG_SHUDGetFadeColor(element->ctx.color_origin, element->ctx.color, &element->config, *element->time))
	{
		*element->time = 0;
		return;
	}

	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementKillConfirmDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
