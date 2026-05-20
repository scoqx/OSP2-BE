/*
===========================================================================
CG dye item icons: cvar-driven tint for powerups with custom dye visuals.
===========================================================================
*/
#include "cg_local.h"

#define CG_DYE_FLAG_BLACK_EPS (1.0f / 255.0f)

static qboolean CG_DyeFlagTintRgbIsBlack(const vec4_t v)
{
	return (v[0] <= CG_DYE_FLAG_BLACK_EPS && v[1] <= CG_DYE_FLAG_BLACK_EPS && v[2] <= CG_DYE_FLAG_BLACK_EPS);
}

void CG_ItemDyeResolveTint(const gitem_t* item, vec4_t out)
{
	Vector4Set(out, 1.0f, 1.0f, 1.0f, 1.0f);

	if (!item)
	{
		return;
	}

	if (item->giType == IT_TEAM)
	{
		if (item->giTag == PW_REDFLAG)
		{
			CG_GetDyeFlagTintVec4(TEAM_RED, out);
		}
		else if (item->giTag == PW_BLUEFLAG)
		{
			CG_GetDyeFlagTintVec4(TEAM_BLUE, out);
		}
		return;
	}

	if (item->giType != IT_POWERUP)
	{
		return;
	}

	if (item->giTag == PW_QUAD && cg.quadDamageColorEnabled)
	{
		out[0] = cg.beQuadDamageColor[0];
		out[1] = cg.beQuadDamageColor[1];
		out[2] = cg.beQuadDamageColor[2];
		return;
	}
	if (item->giTag == PW_BATTLESUIT && cg.battleSuitColorEnabled)
	{
		out[0] = cg.beBattleSuitColor[0];
		out[1] = cg.beBattleSuitColor[1];
		out[2] = cg.beBattleSuitColor[2];
	}
}

qboolean CG_GetDyeFlagTintVec4(int team, vec4_t out)
{
	if (team == TEAM_RED)
	{
		Vector4Copy(cgs.beRedTeamColor, out);
		out[3] = 1.0f;
		return qtrue;
	}
	if (team == TEAM_BLUE)
	{
		Vector4Copy(cgs.beBlueTeamColor, out);
		out[3] = 1.0f;
		return qtrue;
	}

	Vector4Set(out, 1.0f, 1.0f, 1.0f, 1.0f);
	return qfalse;
}

void CG_GetDyeFlagCarrierLightRgb(int team, vec3_t rgb)
{
	vec4_t tint;

	if (!CG_GetDyeFlagTintVec4(team, tint))
	{
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		return;
	}
	if (CG_DyeFlagTintRgbIsBlack(tint))
	{
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		return;
	}
	rgb[0] = tint[0];
	rgb[1] = tint[1];
	rgb[2] = tint[2];
}

qboolean CG_ItemDyeFlagInverseHudPath(const gitem_t* item)
{
	vec4_t     t;
	team_t     tm;

	if (!item || item->giType != IT_TEAM)
	{
		return qfalse;
	}
	if (item->giTag == PW_REDFLAG)
	{
		tm = TEAM_RED;
	}
	else if (item->giTag == PW_BLUEFLAG)
	{
		tm = TEAM_BLUE;
	}
	else
	{
		return qfalse;
	}
	if (!CG_GetDyeFlagTintVec4(tm, t))
	{
		return qfalse;
	}
	return CG_DyeFlagTintRgbIsBlack(t) ? qtrue : qfalse;
}

qhandle_t CG_DyeFlagResolveInverseIcon(int itemNum, qhandle_t shader)
{
	int i;

	if (itemNum < 1 || itemNum >= bg_numItems)
	{
		return shader;
	}
	if (!CG_ItemDyeFlagInverseHudPath(&bg_itemlist[itemNum]))
	{
		return shader;
	}
	for (i = 0; i < 3; i++)
	{
		if (shader == cgs.media.dyeFlagShader[i] && cgs.media.dyeFlagShaderInv[i])
		{
			return cgs.media.dyeFlagShaderInv[i];
		}
	}
	return shader;
}

void CG_DrawDyeFlagPicOld(float x, float y, float w, float h, int team, qhandle_t shader)
{
	vec4_t    tint;
	qhandle_t drawShader;
	int       i;

	if (!shader)
	{
		return;
	}
	drawShader = shader;
	if (CG_GetDyeFlagTintVec4(team, tint))
	{
		if (CG_DyeFlagTintRgbIsBlack(tint))
		{
			for (i = 0; i < 3; i++)
			{
				if (shader == cgs.media.dyeFlagShader[i] && cgs.media.dyeFlagShaderInv[i])
				{
					drawShader = cgs.media.dyeFlagShaderInv[i];
					break;
				}
			}
			CG_DrawPicOld(x, y, w, h, drawShader);
			return;
		}
		trap_R_SetColor(tint);
		CG_DrawPicOld(x, y, w, h, drawShader);
		trap_R_SetColor(NULL);
		return;
	}
	CG_DrawPicOld(x, y, w, h, drawShader);
}
