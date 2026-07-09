/*
===========================================================================
Kill confirm tokens (g_killConfirm / metarena "kc" server commands).

Server protocol: the token (ET_ITEM) and its decorative helper ring (ET_GENERAL)
both carry otherEntityNum == KILLCONFIRM_MARKER with the team in generic1.
CG_Item calls CG_KillConfirm_AddToken(cent), which detects the token (via the
internal CG_KillConfirm_TokenTeam) and draws it; CG_General hides the helper by
the same marker. A real item_armor_shard has otherEntityNum == 0.
===========================================================================
*/
#include "cg_local.h"

static int CG_KillConfirm_TokenTeam(const entityState_t* es)
{
	if (!cgs.killConfirmMode || !es)
	{
		return 0;
	}
	if (es->otherEntityNum != KILLCONFIRM_MARKER)
	{
		return 0;
	}
	if (es->generic1 == TEAM_RED || es->generic1 == TEAM_BLUE)
	{
		return es->generic1;
	}
	return 0;
}

qboolean CG_KillConfirm_ApplyItemStyle(refEntity_t* ent, int team)
{
	const float* tc;

	if (team != TEAM_RED && team != TEAM_BLUE)
	{
		return qfalse;
	}

	tc = (team == TEAM_RED) ? cgs.beRedTeamColor : cgs.beBlueTeamColor;
	if (cgs.media.altFullBrightShader)
	{
		ent->customShader = cgs.media.altFullBrightShader;
	}
	ent->shaderRGBA[0] = (byte)(tc[0] * 255.0f);
	ent->shaderRGBA[1] = (byte)(tc[1] * 255.0f);
	ent->shaderRGBA[2] = (byte)(tc[2] * 255.0f);
	ent->shaderRGBA[3] = 128;
	return qtrue;
}

void CG_KillConfirm_AddQuadOverlay(const refEntity_t* baseEnt, int team)
{
	refEntity_t  ent;
	const float* tc;

	if ((team != TEAM_RED && team != TEAM_BLUE) || !cgs.media.quadShaderDye)
	{
		return;
	}

	tc = (team == TEAM_RED) ? cgs.beRedTeamColor : cgs.beBlueTeamColor;
	ent = *baseEnt;
	ent.customShader = cgs.media.quadShaderDye;
	ent.shaderRGBA[0] = (byte)(tc[0] * 255.0f);
	ent.shaderRGBA[1] = (byte)(tc[1] * 255.0f);
	ent.shaderRGBA[2] = (byte)(tc[2] * 255.0f);
	ent.shaderRGBA[3] = 128;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===========================================================================
CG_KillConfirm_AddToken

Self-contained 3D draw for a kill-confirm token (ET_ITEM tagged with team in
generic1). Kept out of the shared CG_AddWorldItem3D so the hot item path stays
free of kill-confirm logic. Detects the token team internally and returns
qtrue when the entity was a token (and was drawn), qfalse otherwise. Mirrors
the relevant item transforms and applies the team tint + quad overlay.
===========================================================================
*/
static qboolean CG_KillConfirm_AddToken(const centity_t* cent)
{
	refEntity_t     ent;
	vec3_t          origin;
	const entityState_t* es;
	gitem_t*        item;
	int             itemNum;
	int             itemFx;
	int             gitype;
	int             team;
	float           frac;

	if (!cent)
	{
		return qfalse;
	}

	es = &cent->currentState;
	team = CG_KillConfirm_TokenTeam(es);
	if (!team)
	{
		return qfalse;
	}

	itemNum = es->modelindex;
	if (itemNum < 1 || itemNum >= bg_numItems)
	{
		return qfalse;
	}

	item = &bg_itemlist[itemNum];
	gitype = item->giType;
	itemFx = cg_itemFx.integer;
	frac = 1.0f;

	VectorCopy(cent->lerpOrigin, origin);
	memset(&ent, 0, sizeof(ent));

	if (itemFx & 1)
	{
		float scale = 0.005f + es->number * 0.00001f;
		origin[2] += 4.0f + cos((cg.time + 1000) * scale) * 4.0f;
	}

	if (itemFx & 2)
	{
		if (gitype == IT_HEALTH)
		{
			AxisCopy(cg.autoAxisFast, ent.axis);
		}
		else
		{
			AxisCopy(cg.autoAxis, ent.axis);
		}
	}
	else
	{
		AxisClear(ent.axis);
		ent.axis[0][0] = 1;
		ent.axis[1][1] = 1;
		ent.axis[2][2] = 1;
	}

	ent.hModel = cg_items[itemNum].models[0];
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.oldorigin);
	ent.nonNormalizedAxes = qfalse;

	if (itemFx & 4)
	{
		int msec = cg.time - cent->miscTime;
		frac = (msec >= 0 && msec < ITEM_SCALEUP_TIME) ? (float)msec / ITEM_SCALEUP_TIME : 1.0f;
		if (frac != 1.0f)
		{
			VectorScale(ent.axis[0], frac, ent.axis[0]);
			VectorScale(ent.axis[1], frac, ent.axis[1]);
			VectorScale(ent.axis[2], frac, ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;
		}
	}

	if (gitype == IT_WEAPON || gitype == IT_ARMOR)
	{
		ent.renderfx |= RF_MINLIGHT;
	}

	CG_KillConfirm_ApplyItemStyle(&ent, team);
	trap_R_AddRefEntityToScene(&ent);
	CG_KillConfirm_AddQuadOverlay(&ent, team);
	return qtrue;
}

/*
===========================================================================
CG_KillConfirm_HandleEntity

Single pre-switch dispatch for kill-confirm entities (called from
CG_AddCEntity after lerp/effects). Returns qtrue when the entity is a
kill-confirm entity and was consumed, so the caller skips normal handling:
 - ET_ITEM token  -> drawn here, normal item render skipped.
 - ET_GENERAL ring -> hidden (helper ring is never drawn).
===========================================================================
*/
qboolean CG_KillConfirm_HandleEntity(centity_t* cent)
{
	const entityState_t* es;

	if (!cent)
	{
		return qfalse;
	}

	es = &cent->currentState;
	if (es->otherEntityNum != KILLCONFIRM_MARKER)
	{
		return qfalse;
	}

	if (es->eType == ET_ITEM)
	{
		return CG_KillConfirm_AddToken(cent);
	}

	if (es->eType == ET_GENERAL)
	{
		/* two-field sentinel (marker + valid team) so an ordinary world
		   ET_GENERAL that merely references entity #666 in otherEntityNum
		   is not mistaken for the helper ring and hidden */
		return (es->generic1 == TEAM_RED || es->generic1 == TEAM_BLUE) ? qtrue : qfalse;
	}

	return qfalse;
}

void CG_KillConfirm_HandleServerCmd(const char* action)
{
	if (!cgs.killConfirmMode || !action)
	{
		return;
	}
	if (action[0] == 'c' && action[1] == 'o' && action[2] == 'n' && action[3] == 'f'
	        && action[4] == 'i' && action[5] == 'r' && action[6] == 'm' && action[7] == 'e'
	        && action[8] == 'd' && action[9] == '\0')
	{
	}
	else if (action[0] == 'd' && action[1] == 'e' && action[2] == 'n' && action[3] == 'i'
	         && action[4] == 'e' && action[5] == 'd' && action[6] == '\0')
	{
	}
}
