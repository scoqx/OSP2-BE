#include "cg_local.h"
#include "cg_superhud_private.h"
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
		vec4_t hcolor;
		char s[1024];

		Vector4Copy(cg.healthColorByClient[cg.crosshairClientNum], hcolor);

		Com_sprintf(s, sizeof(s), "[%i/%i]", ci->health, ci->armor);

		element->textCtx.text = s;

		VectorCopy(hcolor, element->textCtx.color);
		if (element->config.color.isSet)
		{
			element->textCtx.color[3] = element->config.color.value.rgba[3];
		}
		CG_SHUDTextPrintNew(&element->config, &element->textCtx, qfalse);

		element->textCtx.text = NULL;
	}
	else
	{
		float fadeAlpha;
		float value;
		vec4_t barColor;

		fadeAlpha = element->textCtx.color[3];

		CG_SHUDBarMakeContext(&element->config, &element->barCtx, TARGET_STATUS_BAR_MAX_HP);

		Vector4Copy(cg.healthColorByClient[cg.crosshairClientNum], barColor);

		switch (element->config.style.isSet ? element->config.style.value : 1)
		{
			case 3:
			{
				qboolean targetHealthLow;

				targetHealthLow = qfalse;
				if (cg_healthColorLevels.integer && ci->health > 0)
				{
					if (CG_GetTotalHitPoints(ci->health, ci->armor) <= cg_healthLow.integer)
					{
						targetHealthLow = qtrue;
					}
				}

				if (targetHealthLow)
				{
					Vector4Copy(barColor, element->barCtx.color_top);
					if (element->config.color.isSet)
					{
						element->barCtx.color_top[3] = element->config.color.value.rgba[3];
					}

					if (element->barCtx.two_bars)
					{
						if (!element->config.color2.isSet)
						{
							Vector4Copy(element->barCtx.color_top, element->barCtx.color2_top);
							element->barCtx.color2_top[3] *= 3.0f;
						}
						else
						{
							Vector4Copy(barColor, element->barCtx.color2_top);
							element->barCtx.color2_top[3] = element->config.color2.value.rgba[3];
						}
					}
				}
				else
				{
					Vector4Copy(element->config.color.value.rgba, element->barCtx.color_top);

					if (element->barCtx.two_bars)
					{
						if (!element->config.color2.isSet)
						{
							Vector4Copy(element->barCtx.color_top, element->barCtx.color2_top);
							element->barCtx.color2_top[3] *= 3.0f;
						}
						else
						{
							Vector4Copy(element->config.color2.value.rgba, element->barCtx.color2_top);
						}
					}
				}
				break;
			}

			case 2:
				Vector4Copy(element->config.color.value.rgba, element->barCtx.color_top);
				if (element->barCtx.two_bars)
				{
					if (element->config.color2.isSet)
					{
						Vector4Copy(element->config.color2.value.rgba, element->barCtx.color2_top);
					}
					else
					{
						Vector4Copy(element->barCtx.color_top, element->barCtx.color2_top);
						element->barCtx.color2_top[3] *= 3.0f;
					}
				}
				break;

			case 1:
			default:
				Vector4Copy(barColor, element->barCtx.color_top);
				if (element->config.color.isSet)
				{
					element->barCtx.color_top[3] = element->config.color.value.rgba[3];
				}
				if (element->barCtx.two_bars)
				{
					if (element->config.color2.isSet)
					{
						Vector4Copy(element->config.color2.value.rgba, element->barCtx.color2_top);
					}
					else
					{
						Vector4Copy(element->barCtx.color_top, element->barCtx.color2_top);
						element->barCtx.color2_top[3] *= 3.0f;
					}
				}
				break;
		}

		element->barCtx.color_top[3] *= fadeAlpha;
		element->barCtx.color2_top[3] *= fadeAlpha;
		element->barCtx.color_back[3] *= fadeAlpha;

		value = (float)CG_GetTotalHitPoints(ci->health, ci->armor);
		CG_SHUDBarPrint(&element->config, &element->barCtx, value);
	}
}

void CG_SHUDElementTargetStatusDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
