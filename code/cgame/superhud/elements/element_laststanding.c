#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	qboolean useImage;
	qboolean wasActive;
	int appearTime;
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

static qboolean CG_SHUDElementLastStandingActive(void)
{
	int clientNum;
	int aliveCount;
	int teamSize;
	const clientInfo_t* ci;
	team_t team;

	if (!(CG_OSPIsGameTypeFreeze() || CG_OSPIsGameTypeCA(cgs.gametype)) || !cg.snap || cg.warmup || cg.intermissionStarted)
	{
		return qfalse;
	}

	clientNum = cg.snap->ps.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return qfalse;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return qfalse;
	}

	team = ci->team;
	if (team != TEAM_RED && team != TEAM_BLUE)
	{
		return qfalse;
	}

	teamSize = (team == TEAM_RED) ? cg.teamPlayerCountRed : cg.teamPlayerCountBlue;
	if (teamSize <= 1)
	{
		return qfalse;
	}

	if (team == TEAM_RED)
	{
		aliveCount = cgs.osp.osp_teamcount1;
	}
	else
	{
		aliveCount = cgs.osp.osp_teamcount2;
	}

	return (aliveCount == 1) ? qtrue : qfalse;
}

void CG_SHUDElementLastStandingRoutine(void* context)
{
	shudElementLastStanding_t* element = (shudElementLastStanding_t*)context;
	qboolean active;
	team_t team;
	qboolean* soundPlayed;

	element->ctx.text = NULL;

	active = CG_SHUDElementLastStandingActive();

	if (!active)
	{
		element->wasActive = qfalse;
		element->appearTime = 0;
		return;
	}

	if (!element->wasActive)
	{
		element->wasActive = qtrue;
		element->appearTime = cg.time;
	}

	if (element->config.style.isSet && element->config.style.value == 1 && cgs.media.lastStandingSound)
	{
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
		soundPlayed = (team == TEAM_RED) ? &cgs.osp.lastStandingSoundPlayed1 : &cgs.osp.lastStandingSoundPlayed2;
		if (!*soundPlayed)
		{
			trap_S_StartLocalSound(cgs.media.lastStandingSound, CHAN_LOCAL_SOUND);
			*soundPlayed = qtrue;
		}
	}

	if (element->config.time.isSet && (cg.time - element->appearTime > element->config.time.value))
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
