#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementItemPickup_t;

void* CG_SHUDElementItemPickupCreate(const superhudConfig_t* config)
{
	shudElementItemPickup_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	if (!element->config.time.isSet)
	{
		element->config.time.isSet = qtrue;
		element->config.time.value = 1000;
	}

	return element;
}

void CG_SHUDElementItemPickupRoutine(void* context)
{
	shudElementItemPickup_t* element = (shudElementItemPickup_t*)context;
	qboolean visible;

	visible = CG_SHUDGetFadeColor(element->ctx.color_origin, element->ctx.color, &element->config, cg.itemPickupTime);

	if (visible && bg_itemlist[cg.itemPickup].pickup_name)
	{
		if (element->config.style.isSet && element->config.style.value == 3)
		{
			element->ctx.text = bg_itemlist[cg.itemPickup].pickup_name;
		}
		else
		{
			int msec;
			char timeBuf[16];

			if (cg_countdownTimer.integer)
			{
				msec = CG_GetRemainingGameTimeMsec(qtrue);
			}
			else
			{
				msec = cg.itemPickupTime - cgs.levelStartTime;
			}

			SHUD_WRITE_MATCH_TIME_STR(timeBuf, sizeof(timeBuf), msec);
			if (element->config.style.isSet && element->config.style.value == 2)
			{
				element->ctx.text = timeBuf;
			}
			else
			{
				element->ctx.text = va("%s %s", timeBuf, bg_itemlist[cg.itemPickup].pickup_name);
			}
		}

		CG_SHUDTextPrint(&element->config, &element->ctx);
	}
}

void CG_SHUDElementItemPickupDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
