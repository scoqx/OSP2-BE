#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementLabel;

void* CG_SHUDElementLabelCreate(const superhudConfig_t* config)
{
	shudElementLabel* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);

	if (config->text.isSet) 
	{
		element->ctx.text = element->config.text.value;
	}

	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementLabelRoutine(void* context)
{
	shudElementLabel* element = (shudElementLabel*)context;

	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementLabelDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
