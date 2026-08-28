#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	int timePrev;
	superhudTextContext_t ctx;
} shudElementGameTime_t;

void* CG_SHUDElementGameTimeCreate(const superhudConfig_t* config)
{
	shudElementGameTime_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementGameTimeRoutine(void* context)
{
	shudElementGameTime_t* element = (shudElementGameTime_t*)context;
	char timeBuf[16];
	int msec;

	if (cg_countdownTimer.integer)
	{
		msec = CG_GetRemainingGameTimeMsec(qfalse);
	}
	else
	{
		msec = cg.time - cgs.levelStartTime;
	}

	SHUD_WRITE_MATCH_TIME_STR(timeBuf, sizeof(timeBuf), msec);
	element->ctx.text = timeBuf;
	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementGameTimeDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
