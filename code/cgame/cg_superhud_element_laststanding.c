#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	qboolean useImage;
} shudElementLastStanding_t;

void* CG_SHUDElementLastStandingCreate(const superhudConfig_t* config)
{
	shudElementLastStanding_t* element;

	element = (shudElementLastStanding_t*)Z_Malloc(sizeof(*element));
	OSP_MEMORY_CHECK(element);
	memset(element, 0, sizeof(*element));

	memcpy(&element->config, config, sizeof(element->config));

	if (config->image.isSet)
	{
		element->useImage = qtrue;
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
		element->drawCtx.image = trap_R_RegisterShader(element->config.image.value);
		if (!element->drawCtx.image)
		{
			CG_Printf("^1LastStanding image %s is not found\n", element->config.image.value);
		}
	}

	if (!config->text.isSet && !element->useImage)
	{
		Q_strncpyz(element->config.text.value, "The Last Standing", sizeof(element->config.text.value));
	}

	if (!element->useImage)
	{
		CG_SHUDTextMakeContext(&element->config, &element->ctx);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	}

	return element;
}

void CG_SHUDElementLastStandingRoutine(void* context)
{
	shudElementLastStanding_t* element = (shudElementLastStanding_t*)context;
	int clientNum;
	int aliveCount;
	int teamSize;
	const clientInfo_t* ci;
	team_t team;

	element->ctx.text = NULL;

	if (!CG_OSPIsGameTypeFreeze() || !cg.snap || cg.warmup || cg.intermissionStarted)
	{
		return;
	}

	clientNum = cg.snap->ps.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	team = ci->team;
	if (team != TEAM_RED && team != TEAM_BLUE)
	{
		return;
	}

	teamSize = (team == TEAM_RED) ? cg.teamPlayerCountRed : cg.teamPlayerCountBlue;
	if (teamSize <= 1)
	{
		return;
	}

	if (team == TEAM_RED)
	{
		aliveCount = cgs.osp.osp_teamcount1;
	}
	else
	{
		aliveCount = cgs.osp.osp_teamcount2;
	}

	if (aliveCount != 1)
	{
		return;
	}

	if (element->useImage && element->drawCtx.image)
	{
		CG_SHUDFill(&element->config);
		CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		CG_SHUDDrawBorder(&element->config);
		return;
	}

	element->ctx.text = element->config.text.value;
	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementLastStandingDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
