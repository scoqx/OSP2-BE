#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementRespawnMessage_t;

static void CG_SHUDRespawnFormatTenths(int tenths, char* buf, int size)
{
	if (tenths < 0)
	{
		tenths = 0;
	}
	if (tenths >= 30)
	{
		Com_sprintf(buf, size, "%d", tenths / 10);
	}
	else
	{
		Com_sprintf(buf, size, "%d.%d", tenths / 10, tenths % 10);
	}
}

void* CG_SHUDElementRespawnMessageCreate(const superhudConfig_t* config)
{
	shudElementRespawnMessage_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementRespawnMessageRoutine(void* context)
{
	shudElementRespawnMessage_t* element = (shudElementRespawnMessage_t*)context;
	int v;
	int canTenths;
	int forceTenths;
	char canBuf[16];
	char forceBuf[16];

	/* STAT_BE_CUSTOM respawn pack — only on OSP2-BE servers */
	if (!cgs.beSupportedServer)
	{
		return;
	}
	if (CG_OSPIsGameTypeFreeze() || !cg.snap || cg.intermissionStarted)
	{
		return;
	}
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		return;
	}

	v = cg.snap->ps.stats[STAT_BE_CUSTOM];
	if (BE_CUSTOM_ACTIVE(v))
	{
		return;
	}

	canTenths = RESPAWN_TIMER_CAN(v);
	forceTenths = RESPAWN_TIMER_FORCE(v);

	if (canTenths <= 0 && forceTenths <= 0)
	{
		element->ctx.text = "Respawn";
	}
	else if (canTenths <= 0)
	{
		CG_SHUDRespawnFormatTenths(forceTenths, forceBuf, sizeof(forceBuf));
		element->ctx.text = va("Respawn (%ss)", forceBuf);
	}
	else if (forceTenths <= 0)
	{
		CG_SHUDRespawnFormatTenths(canTenths, canBuf, sizeof(canBuf));
		element->ctx.text = va("Respawn in %ss", canBuf);
	}
	else
	{
		CG_SHUDRespawnFormatTenths(canTenths, canBuf, sizeof(canBuf));
		CG_SHUDRespawnFormatTenths(forceTenths, forceBuf, sizeof(forceBuf));
		element->ctx.text = va("Respawn in %ss (%ss)", canBuf, forceBuf);
	}

	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementRespawnMessageDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
