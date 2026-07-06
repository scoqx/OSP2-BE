#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementTimelimit_t;

void* CG_SHUDElementTimelimitCreate(const superhudConfig_t* config)
{
	shudElementTimelimit_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementTimelimitRoutine(void* context)
{
	shudElementTimelimit_t* element = (shudElementTimelimit_t*)context;
	int timelimit;

	timelimit = cgs.timelimit;

	element->ctx.text = CG_SHUDVaTextWithInt(&element->config, timelimit);
	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementTimelimitDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
