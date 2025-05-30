#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef enum
{
	SHUD_ELEMENT_LOCAL_TIME,
	SHUD_ELEMENT_LOCAL_DATE,
} shudElementLocalTimeType_t;

typedef struct
{
	superhudConfig_t config;
	int timePrev;
	char s[MAX_QPATH];
	superhudTextContext_t ctx;
	shudElementLocalTimeType_t type;
} shudElementLocalTime_t;

void* CG_SHUDElementCreateDateTime(const superhudConfig_t* config, shudElementLocalTimeType_t type)
{
	shudElementLocalTime_t* element;

	SHUD_ELEMENT_INIT(element, config);

	element->type = type;

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	element->ctx.text = &element->s[0];

	return element;
}

void* CG_SHUDElementLocalTimeCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementCreateDateTime(config, SHUD_ELEMENT_LOCAL_TIME);
}

void* CG_SHUDElementLocalDateCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementCreateDateTime(config, SHUD_ELEMENT_LOCAL_DATE);
}

void CG_SHUDElementLocalTimeRoutine(void* context)
{
    shudElementLocalTime_t* element = (shudElementLocalTime_t*)context;
    qtime_t qtime;

    if (cg.time - element->timePrev > 1000)
    {
        element->timePrev = cg.time;
        trap_RealTime(&qtime);
		if(element->type == SHUD_ELEMENT_LOCAL_TIME)
		{
			Com_sprintf(element->s, MAX_QPATH, "%02d:%02d", qtime.tm_hour, qtime.tm_min);
		}
		else if(element->type == SHUD_ELEMENT_LOCAL_DATE)
		{
			if(element->config.style.isSet && element->config.style.value == 1)
			{
				Com_sprintf(element->s, MAX_QPATH, "%02d.%02d.%04d",
							qtime.tm_mon + 1,
							qtime.tm_mday,
							qtime.tm_year + 1900
							);
			}
			else
			    Com_sprintf(element->s, MAX_QPATH, "%02d.%02d.%04d",
                qtime.tm_mday,
                qtime.tm_mon + 1,
                qtime.tm_year + 1900
				);
		}

    }
    CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementLocalTimeDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

