/*
===========================================================================
cg_globalpos.c

Team tinfo zone positions (globalPos) when the server exposes location CS
and mapdata QLM1 is loaded.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"

#define GLOBAL_POS_DRAW_ALPHA 0.66f

/*
=================
CG_GlobalPosFriendMarkersAllowed

Playing team client with map tinfo locations (not local/snap spectator).
=================
*/
static qboolean CG_GlobalPosFriendMarkersAllowed(void)
{
	if (!cg.snap || !cg.teamTinfoLocations || cgs.gametype < GT_TEAM)
	{
		return qfalse;
	}
	if (CG_IS_LOCAL_CLIENT_SPECTATOR())
	{
		return qfalse;
	}
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_ClientHasDrawablePlayerEntity(int clientNum)
{
	clientInfo_t* ci;
	centity_t* cent;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return qfalse;
	}
	if (!ci->legsModel && !ci->torsoModel && !ci->headModel)
	{
		return qfalse;
	}

	if (ci->isFrozenEnt)
	{
		if (ci->frozenEntity < 0 || ci->frozenEntity >= MAX_GENTITIES)
		{
			return qfalse;
		}
		cent = &cg_entities[ci->frozenEntity];
	}
	else
	{
		cent = &cg_entities[clientNum];
	}

	if (!cent->currentValid || cent->currentState.eType != ET_PLAYER)
	{
		return qfalse;
	}

	return qtrue;
}

void CG_ClearTeamGlobalPosFrame(void)
{
	int i;

	cg.teamTinfoLocations = qfalse;
	cg.teamMarkerStackCacheTime = -1;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cgs.clientinfo[i].globalPos = qfalse;
	}
}

void CG_UpdateTeamGlobalPosFrame(void)
{
	int i;
	clientInfo_t* ci;

	if (!cg.snap)
	{
		CG_ClearTeamGlobalPosFrame();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
	        CG_IS_LOCAL_CLIENT_SPECTATOR())
	{
		CG_ClearTeamGlobalPosFrame();
		return;
	}

	if (!CG_MapLocs_Count() || !CG_MapLocs_ServerHasLocationNames())
	{
		CG_ClearTeamGlobalPosFrame();
		return;
	}

	cg.teamTinfoLocations = qtrue;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		ci->globalPos = qfalse;

		if (!ci->infoValid)
		{
			continue;
		}
		/* Same ally/enemy as team indicator (per-frame cache), not raw team/rt */
		if (CG_CACHED_IS_ENEMY_CI(ci))
		{
			continue;
		}
		if (ci->team == TEAM_SPECTATOR)
		{
			continue;
		}
		if (ci->location <= 0)
		{
			continue;
		}
		if (CG_ClientHasDrawablePlayerEntity(i))
		{
			continue;
		}
		if (CG_MapLocs_GetById(ci->location, ci->globalPosOrigin, NULL, 0))
		{
			ci->globalPos = qtrue;
		}
	}
}

qboolean CG_ClientTeamMarkerOrigin(int clientNum, vec3_t out)
{
	clientInfo_t* ci;
	centity_t* cent;

	if (!out || clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientNum];

	if (ci->isFrozenEnt)
	{
		if (ci->frozenEntity >= 0 && ci->frozenEntity < MAX_GENTITIES)
		{
			cent = &cg_entities[ci->frozenEntity];
			if (cent->currentValid && cent->currentState.eType == ET_PLAYER &&
			        cent->currentState.otherEntityNum == clientNum)
			{
				VectorCopy(cent->lerpOrigin, out);
				return qtrue;
			}
		}
	}
	else
	{
		cent = &cg_entities[clientNum];
		if (cent->currentValid && cent->currentState.eType == ET_PLAYER)
		{
			VectorCopy(cent->lerpOrigin, out);
			return qtrue;
		}
	}

	if (ci->globalPos)
	{
		VectorCopy(ci->globalPosOrigin, out);
		return qtrue;
	}

	return qfalse;
}

static qboolean CG_TeamMarkerSameStackPoint(const clientInfo_t* a, const vec3_t oa, const clientInfo_t* b, const vec3_t ob)
{
	if (a->globalPos && b->globalPos && a->location > 0 && a->location == b->location)
	{
		return qtrue;
	}
	if (DistanceSquared(oa, ob) < (72.0f * 72.0f))
	{
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_TeamMarkerStackSortKey

Vertical stack order at one marker point (0 = bottom on screen):
  dead, then frozen, then alive.
=================
*/
static int CG_TeamMarkerStackSortKey(int clientNum)
{
	clientInfo_t* ci;
	centity_t* cent;
	qboolean frozenTinfo;
	qboolean dead;

	ci = &cgs.clientinfo[clientNum];
	frozenTinfo = ci->globalPos && CG_ClientInfoIsFreezeTinfoFrozen(ci);

	if (ci->isFrozenEnt || frozenTinfo)
	{
		return 1;
	}

	if (ci->globalPos)
	{
		dead = (ci->health <= 0);
	}
	else
	{
		cent = &cg_entities[clientNum];
		if (cent->currentValid && cent->currentState.eType == ET_PLAYER &&
		        (cent->currentState.eFlags & EF_DEAD))
		{
			dead = qtrue;
		}
		else
		{
			dead = (ci->health <= 0);
		}
	}

	if (dead)
	{
		return 0;
	}
	return 2;
}

/*
=================
CG_FillTeamMarkerStackIndex

Allies sharing a tinfo zone or the same world origin get 0,1,2…
sorted dead / frozen / alive (0 = lowest on screen), then clientNum.
=================
*/
static void CG_FillTeamMarkerStackIndex(int stackIndex[MAX_CLIENTS], int stackGroupSize[MAX_CLIENTS], qboolean gameModeNotSpectator)
{
	vec3_t origins[MAX_CLIENTS];
	qboolean hasOrigin[MAX_CLIENTS];
	int sortKey[MAX_CLIENTS];
	int i;
	int j;

	memset(stackIndex, 0, MAX_CLIENTS * sizeof(stackIndex[0]));
	if (stackGroupSize)
	{
		memset(stackGroupSize, 0, MAX_CLIENTS * sizeof(stackGroupSize[0]));
	}
	memset(hasOrigin, 0, sizeof(hasOrigin));

	if (!cg.snap)
	{
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		clientInfo_t* ci;

		if (i == cg.snap->ps.clientNum)
		{
			continue;
		}

		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
		{
			continue;
		}
		if (gameModeNotSpectator && CG_CACHED_IS_ENEMY_CI(ci))
		{
			continue;
		}
		if (ci->team == TEAM_SPECTATOR)
		{
			continue;
		}
		if (!CG_ClientTeamMarkerOrigin(i, origins[i]))
		{
			continue;
		}
		hasOrigin[i] = qtrue;
		sortKey[i] = CG_TeamMarkerStackSortKey(i);
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!hasOrigin[i])
		{
			continue;
		}
		for (j = 0; j < MAX_CLIENTS; j++)
		{
			if (!hasOrigin[j])
			{
				continue;
			}
			if (!CG_TeamMarkerSameStackPoint(&cgs.clientinfo[i], origins[i], &cgs.clientinfo[j], origins[j]))
			{
				continue;
			}
			if (stackGroupSize)
			{
				stackGroupSize[i]++;
			}
			if (j == i)
			{
				continue;
			}
			if (sortKey[j] < sortKey[i] || (sortKey[j] == sortKey[i] && j < i))
			{
				stackIndex[i]++;
			}
		}
	}
}

int CG_TeamMarkerStackEffectiveSize(int clientNum)
{
	int           n;
	vec3_t        origin;
	clientInfo_t* ci;
	int           i;
	vec3_t        dropOrigin;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return 0;
	}

	n = cg.teamMarkerStackGroupSize[clientNum];
	if (n < 1)
	{
		return n;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid || !CG_ClientTeamMarkerOrigin(clientNum, origin))
	{
		return n;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!CG_WorldFlags_ClientHasStackDropFlag(i))
		{
			continue;
		}
		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}
		if (!CG_ClientTeamMarkerOrigin(i, dropOrigin))
		{
			if (!cgs.clientinfo[i].globalPos)
			{
				continue;
			}
			VectorCopy(cgs.clientinfo[i].globalPosOrigin, dropOrigin);
		}
		if (CG_TeamMarkerSameStackPoint(ci, origin, &cgs.clientinfo[i], dropOrigin))
		{
			return n + 1;
		}
	}

	return n;
}

void CG_EnsureTeamMarkerStackCache(qboolean gameModeNotSpectator)
{
	if (cg.teamMarkerStackCacheTime == cg.time &&
	        cg.teamMarkerStackCacheAlliesOnly == gameModeNotSpectator)
	{
		return;
	}

	CG_FillTeamMarkerStackIndex(cg.teamMarkerStackIndex, cg.teamMarkerStackGroupSize, gameModeNotSpectator);
	cg.teamMarkerStackCacheTime = cg.time;
	cg.teamMarkerStackCacheAlliesOnly = gameModeNotSpectator;
}

static void CG_GlobalPosFriendFloatSpritePick(int clientNum, clientInfo_t* ci, qboolean* wantShow,
        qhandle_t* friendShader, float** friendColor, qboolean useTeamFoeWh, qboolean useTeamFrozen)
{
	qboolean isFrozenTinfo;

	*wantShow = qfalse;
	*friendShader = 0;
	*friendColor = NULL;

	if (CG_ClientHasDrawablePlayerEntity(clientNum))
	{
		return;
	}

	if (!ci->infoValid || !ci->globalPos || CG_CACHED_IS_ENEMY_CI(ci) || ci->team == TEAM_SPECTATOR)
	{
		return;
	}

	isFrozenTinfo = CG_ClientInfoIsFreezeTinfoFrozen(ci);
	if ((ci->health <= 0 && !isFrozenTinfo) ||
	    (cg.teamMarkers.drawTeamTasks && cg.teamMarkers.clientHasOverhead[clientNum]))
	{
		return;
	}

	if (isFrozenTinfo)
	{
		if (!cg.teamMarkers.clientHasFrozenAllyMarkNear[clientNum] &&
		    (cg_drawFriend.integer == 2 || cg_teamFrozenFoe.integer || cg_drawFriend.integer == 1))
		{
			if (!useTeamFrozen && cg_teamFrozenFoe.integer)
			{
				*friendShader = cgs.media.frozenFoeTagShader;
			}
			else if (useTeamFoeWh && (cg_friendsWallhack.integer & 1) &&
			         cg_friendsWallhackFrozen.integer && useTeamFrozen)
			{
				*friendShader = cg_friendsWallhackFrozen.integer ?
				                cgs.media.frozenFoeTagShaderWallhack : cgs.media.frozenFoeTagShader;
			}
			else
			{
				*friendShader = cgs.media.frozenFoeTagShader;
			}
		}
	}
	else if (cg_drawFriend.integer != 2)
	{
		*friendShader = (useTeamFoeWh && (cg_friendsWallhack.integer & 1)) ?
		                cgs.media.friendShaderWallhack : cgs.media.friendShader;
		*friendColor = ((cg_healthColorLevels.integer & 2) || ci->health <= 0) ?
		               NULL : (float*)cg.healthColorByClient[clientNum];
	}

	*wantShow = (*friendShader != 0);
}

static float CG_GlobalPosFloatSpriteStackStep(const vec3_t origin)
{
	vec3_t entOrigin;
	vec3_t delta;
	float minSize;
	float radius;
	float dist;
	int maxDist;

	minSize = cg_floatSpriteMinSize.value;
	if (minSize <= 0.0f)
	{
		minSize = 1.0f;
	}
	radius = 10.0f * minSize;
	VectorCopy(origin, entOrigin);
	entOrigin[2] += 48.0f;
	maxDist = cg_floatSpriteMaxDist.integer;
	if (maxDist > 0)
	{
		VectorSubtract(entOrigin, cg.refdef.vieworg, delta);
		dist = (float)VectorLength(delta);
		if (dist >= (float)maxDist)
		{
			radius *= dist / (float)maxDist;
		}
	}
	return 0.6f * (radius * 2.0f);
}

void CG_AddGlobalPosFriendFloatSprites(void)
{
	int i;
	clientInfo_t* ci;
	qboolean      useTeamFoeWh;
	qboolean      useTeamFrozen;
	qboolean      wantShow;
	qhandle_t     friendShader;
	float*        friendColor;
	vec3_t        spriteOrigin;

	if (!cg_drawFriend.integer || !CG_GlobalPosFriendMarkersAllowed())
	{
		return;
	}

	useTeamFoeWh = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH);
	useTeamFrozen = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FROZEN);

	CG_EnsureTeamMarkerStackCache(qtrue);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		vec4_t dropColor;
		int n;
		float step;

		if (i == cg.snap->ps.clientNum)
		{
			continue;
		}
		if (!CG_WorldFlags_ClientHasStackDropFlag(i))
		{
			continue;
		}

		ci = &cgs.clientinfo[i];
		if (!ci->infoValid || !ci->globalPos)
		{
			continue;
		}

		VectorCopy(ci->globalPosOrigin, spriteOrigin);
		n = CG_TeamMarkerStackEffectiveSize(i);
		if (n > 1)
		{
			step = CG_GlobalPosFloatSpriteStackStep(ci->globalPosOrigin);
			spriteOrigin[2] += ((n - 1) * 2 - (n - 1)) * (step * 0.5f);
		}
		CG_WorldFlags_GetStackDropFlagColor(i, dropColor);
		CG_AddFloatSpriteAtOrigin(spriteOrigin, i, cgs.media.teamTasksFlag, dropColor, qtrue, GLOBAL_POS_DRAW_ALPHA, 0.0f);
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		int n;
		int idx;
		float step;

		if (i == cg.snap->ps.clientNum)
		{
			continue;
		}

		ci = &cgs.clientinfo[i];
		CG_GlobalPosFriendFloatSpritePick(i, ci, &wantShow, &friendShader, &friendColor,
		                                  useTeamFoeWh, useTeamFrozen);
		if (!wantShow)
		{
			continue;
		}

		VectorCopy(ci->globalPosOrigin, spriteOrigin);
		n = CG_TeamMarkerStackEffectiveSize(i);
		if (n > 1)
		{
			idx = cg.teamMarkerStackIndex[i];
			step = CG_GlobalPosFloatSpriteStackStep(ci->globalPosOrigin);
			spriteOrigin[2] += (idx * 2 - (n - 1)) * (step * 0.5f);
		}

		CG_AddFloatSpriteAtOrigin(spriteOrigin, i, friendShader, friendColor, qtrue, GLOBAL_POS_DRAW_ALPHA, 0.0f);
	}
}

void CG_DrawGlobalPosFriendHudMarkers(void)
{
	int i;
	clientInfo_t* ci;
	qboolean useTeamFrozen;
	qboolean useTeamFoeWh;

	if (!CG_GlobalPosFriendMarkersAllowed())
	{
		return;
	}

	useTeamFrozen = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FROZEN);
	useTeamFoeWh = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH);

	{
		CG_EnsureTeamMarkerStackCache(qtrue);

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			int angle;
			int n;
			int idx;
			qboolean isFrozenTinfo;

			if (!cg.snap || i == cg.snap->ps.clientNum)
			{
				continue;
			}

			ci = &cgs.clientinfo[i];
			if (!ci->infoValid || !ci->globalPos)
			{
				continue;
			}
			if (CG_ClientHasDrawablePlayerEntity(i))
			{
				continue;
			}
			if (CG_CACHED_IS_ENEMY_CI(ci) || ci->team == TEAM_SPECTATOR)
			{
				continue;
			}

			isFrozenTinfo = CG_ClientInfoIsFreezeTinfoFrozen(ci);
			if (ci->health <= 0 && !isFrozenTinfo)
			{
				continue;
			}

			n = CG_TeamMarkerStackEffectiveSize(i);
			idx = cg.teamMarkerStackIndex[i];
			if (n > 1)
			{
				angle = 270 + (idx * 2 - (n - 1)) * 7;
			}
			else
			{
				angle = 270;
			}
			CG_FriendHudMarkerAtOrigin(ci->globalPosOrigin, ci, i, isFrozenTinfo, useTeamFrozen, useTeamFoeWh, angle, GLOBAL_POS_DRAW_ALPHA);
		}
	}
}
