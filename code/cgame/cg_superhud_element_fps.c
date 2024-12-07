#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

#define FPS_FRAMES 4

typedef struct
{
	superhudConfig_t config;
	int timePrev;
	int frameIndex;
	int timeArray[FPS_FRAMES];
	superhudTextContext_t ctx;
} shudElementFPS_t;

void* CG_SHUDElementFPSCreate(const superhudConfig_t* config)
{
	shudElementFPS_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	element->ctx.maxchars = 6;

	return element;
}



void CG_SHUDElementFPSRoutine(void* context)
{
	shudElementFPS_t* element = (shudElementFPS_t*)context;
	int i, total, fps, t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - element->timePrev;
	element->timePrev = t;

	element->timeArray[element->frameIndex % FPS_FRAMES] = frameTime;
	element->frameIndex++;
	if (element->frameIndex > FPS_FRAMES)
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0; i < FPS_FRAMES; i++)
		{
			total += element->timeArray[i];
		}
		if (!total)
		{
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		element->ctx.text = va("%ifps", fps);
	}

	CG_SHUDFill(&element->config);
	CG_SHUDTextPrint(&element->config, &element->ctx);
}




void CG_SHUDElementFPSDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
