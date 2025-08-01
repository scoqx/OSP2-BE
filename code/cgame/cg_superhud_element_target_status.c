#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementTargetStatus_t;

void* CG_SHUDElementTargetStatusCreate(const superhudConfig_t* config)
{
	shudElementTargetStatus_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementTargetStatusRoutine(void* context)
{
	shudElementTargetStatus_t* element = (shudElementTargetStatus_t*)context;
	char    s[1024];
	clientInfo_t* ci;

	if (cg_drawCrosshair.integer == 0) return;
	if (cg_drawCrosshairNames.integer == 0) return;
	if (cg.renderingThirdPerson != 0) return;
	if (global_viewlistFirstOption > 1) return;

	CG_ScanForCrosshairEntity();
	if (cg.crosshairClientTime == 0) return;


	if (!CG_SHUDGetFadeColor(element->ctx.color_origin, element->ctx.color, &element->config, cg.crosshairClientTime))
	{
		cg.crosshairClientTime = 0;
		return;
	}

	ci = &cgs.clientinfo[cg.crosshairClientNum];

	if ((ci->team == cg.snap->ps.persistant[PERS_TEAM]) || (cgs.osp.gameTypeFreeze && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team))
	{
		if (ci->team != TEAM_FREE && ci->team == cg.snap->ps.persistant[PERS_TEAM] && ch_TeamCrosshairHealth.integer != 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			vec4_t hcolor;
			char s[1024];

			CG_GetColorForHealth(ci->health, ci->armor, hcolor, NULL);

			Com_sprintf(s, sizeof(s), "[%i/%i]", ci->health, ci->armor);

			element->ctx.text = s;

			VectorCopy(hcolor, element->ctx.color);
			if (element->config.color.isSet)
			{
				element->ctx.color[3] = element->config.color.value.rgba[3];
			}
			CG_SHUDTextPrintNew(&element->config, &element->ctx, qfalse);

			element->ctx.text = NULL;
		}
	}

}

void CG_SHUDElementTargetStatusDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

