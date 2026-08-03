#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

#define TARGET_STATUS_BAR_MAX_HP 400.0f

typedef enum
{
	SHUD_ELEMENT_TARGETSTATUS_TEXT,
	SHUD_ELEMENT_TARGETSTATUS_BAR
} shudElementTargetStatusType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t textCtx;
	superhudBarContext_t barCtx;
	shudElementTargetStatusType_t type;
} shudElementTargetStatus_t;

static qboolean CG_SHUDElementTargetStatusShouldDraw(clientInfo_t* ci)
{
	if (cg_drawCrosshair.integer == 0) return qfalse;
	if (cg_drawCrosshairNames.integer == 0) return qfalse;
	if (cg.renderingThirdPerson != 0) return qfalse;
	if (global_viewlistFirstOption > 1) return qfalse;
	if (cg.crosshairClientTime == 0) return qfalse;

	if (!((ci->team == cg.snap->ps.persistant[PERS_TEAM]) || (cgs.osp.gameTypeFreeze && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)))
	{
		return qfalse;
	}
	if (ci->team == TEAM_FREE || ci->team != cg.snap->ps.persistant[PERS_TEAM])
	{
		return qfalse;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return qfalse;
	}
	if (ci->health == 0 && ci->armor == 0)
	{
		return qfalse;
	}
	return qtrue;
}

static void* CG_SHUDElementTargetStatusCreateEx(const superhudConfig_t* config, shudElementTargetStatusType_t type)
{
	shudElementTargetStatus_t* element;

	SHUD_ELEMENT_INIT(element, config);

	element->type = type;

	if (element->type == SHUD_ELEMENT_TARGETSTATUS_TEXT)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
		CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
	}
	else
	{
		CG_SHUDBarMakeContext(&element->config, &element->barCtx, TARGET_STATUS_BAR_MAX_HP);
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
	}

	return element;
}

void* CG_SHUDElementTargetStatusCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementTargetStatusCreateEx(config, SHUD_ELEMENT_TARGETSTATUS_TEXT);
}

void* CG_SHUDElementTargetStatusBarCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementTargetStatusCreateEx(config, SHUD_ELEMENT_TARGETSTATUS_BAR);
}

void CG_SHUDElementTargetStatusRoutine(void* context)
{
	shudElementTargetStatus_t* element = (shudElementTargetStatus_t*)context;
	clientInfo_t* ci;

	ci = &cgs.clientinfo[cg.crosshairClientNum];
	if (!CG_SHUDElementTargetStatusShouldDraw(ci))
	{
		return;
	}

	if (!CG_SHUDGetFadeColor(element->textCtx.color_origin, element->textCtx.color, &element->config, cg.crosshairClientTime))
	{
		cg.crosshairClientTime = 0;
		return;
	}

	if (element->type == SHUD_ELEMENT_TARGETSTATUS_TEXT)
	{
		char s[1024];
		float fadeAlpha;

		/* Keep fade alpha from CG_SHUDGetFadeColor; health color supplies RGB only */
		fadeAlpha = element->textCtx.color[3];
		VectorCopy(cg.healthColorByClient[cg.crosshairClientNum], element->textCtx.color);
		element->textCtx.color[3] = fadeAlpha;

		if (element->config.style.isSet && element->config.style.value == 1)
		{
			Com_sprintf(s, sizeof(s), "%i/%i", ci->health, ci->armor);
		}
		else
		{
			Com_sprintf(s, sizeof(s), "[%i/%i]", ci->health, ci->armor);
		}

		element->textCtx.text = s;
		CG_SHUDTextPrintNew(&element->config, &element->textCtx, qfalse);
		element->textCtx.text = NULL;
	}
	else
	{
		float barAlpha;
		superhudConfig_t frameCfg;

		/* Alpha already includes config color + SuperHUD fade */
		barAlpha = element->textCtx.color[3];

		CG_SHUDBarMakeContext(&element->config, &element->barCtx, TARGET_STATUS_BAR_MAX_HP);

		memcpy(&frameCfg, &element->config, sizeof(frameCfg));
		frameCfg.rect.value[0] = element->barCtx.box[0];
		frameCfg.rect.value[1] = element->barCtx.box[1];
		frameCfg.rect.value[2] = element->barCtx.box[2];
		frameCfg.rect.value[3] = element->barCtx.box[3];
		frameCfg.alignH.isSet = qfalse;
		frameCfg.alignV.isSet = qfalse;

		CG_SHUDFillAndFrameForElement(&frameCfg, element->textCtx.color_origin, element->textCtx.color);

		CG_DrawHealthBar(element->barCtx.box[0],
		                 element->barCtx.box[1],
		                 element->barCtx.box[2],
		                 element->barCtx.box[3],
		                 ci->health, ci->armor, barAlpha, -1, qfalse,
		                 &cg.healthColorByClient[cg.crosshairClientNum], qfalse);
	}
}

void CG_SHUDElementTargetStatusDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
