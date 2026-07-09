/*
===========================================================================
cg_worldflags.c

CTF world flag indicators (cg_worldFlags). Base positions from mapdata QSP1.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "superhud.h"
#include "../game/bg_public.h"

#define CG_WORLDFLAGS_WORLD         1
#define CG_WORLDFLAGS_HUDMARKER     2

#define CG_WORLDFLAGS_BASE_MATCH    96.0f
#define CG_WORLDFLAGS_FLOAT_MAX_DIST 1800
#define CG_WORLDFLAGS_Z_LIFT        (-16.0f)
#define CG_WORLDFLAGS_WORLD_ICON_SCALE 0.8f
#define CG_WORLDFLAGS_MARK_APPEAR_MS  350
#define CG_WORLDFLAGS_MARK_SPAWN_MS   1000
#define CG_WORLDFLAGS_AIM_RADIUS    64.0f

typedef struct
{
	vec3_t          origin;
	qboolean        dropValid;
	int             dropClientNum;
	qboolean        dropAtGlobalPos;
	int             dropSpawnTime;
} cg_worldFlagState_t;

static cg_worldFlagState_t  cg_worldFlagRed;
static cg_worldFlagState_t  cg_worldFlagBlue;
static qboolean             cg_worldFlagWasDead[MAX_CLIENTS];
static int                  cg_worldFlagRedModelIndex = -1;
static int                  cg_worldFlagBlueModelIndex = -1;

void CG_WorldFlags_Reset(void)
{
	memset(&cg_worldFlagRed, 0, sizeof(cg_worldFlagRed));
	memset(&cg_worldFlagBlue, 0, sizeof(cg_worldFlagBlue));
	memset(cg_worldFlagWasDead, 0, sizeof(cg_worldFlagWasDead));
	cg_worldFlagRed.dropClientNum = -1;
	cg_worldFlagBlue.dropClientNum = -1;
	cg_worldFlagRedModelIndex = -1;
	cg_worldFlagBlueModelIndex = -1;
}

static int CG_WorldFlags_FlagModelIndex(qboolean redFlag)
{
	gitem_t* item;

	item = BG_FindItemForPowerup(redFlag ? PW_REDFLAG : PW_BLUEFLAG);
	if (!item)
	{
		return 0;
	}
	return item - bg_itemlist;
}

static int CG_WorldFlags_FlagModelIndexCached(qboolean redFlag)
{
	int*    cache;
	int     modelIndex;

	cache = redFlag ? &cg_worldFlagRedModelIndex : &cg_worldFlagBlueModelIndex;
	if (*cache >= 0)
	{
		return *cache;
	}
	modelIndex = CG_WorldFlags_FlagModelIndex(redFlag);
	*cache = modelIndex;
	return modelIndex;
}

static qboolean CG_WorldFlags_FindDroppedFlagOrigin(qboolean redFlag, vec3_t outOrigin)
{
	int         i;
	int         modelIndex;
	centity_t*  cent;

	modelIndex = CG_WorldFlags_FlagModelIndexCached(redFlag);
	if (!modelIndex)
	{
		return qfalse;
	}

	for (i = 0; i < cg.numItemEntityIndices; i++)
	{
		cent = &cg_entities[cg.itemEntityIndices[i]];
		if (cent->currentState.modelindex != modelIndex)
		{
			continue;
		}
		if (cent->currentState.eFlags & EF_NODRAW)
		{
			continue;
		}
		VectorCopy(cent->lerpOrigin, outOrigin);
		return qtrue;
	}

	return qfalse;
}

static void CG_WorldFlags_GetFlagColor(qboolean redFlag, vec4_t color)
{
	if (redFlag)
	{
		VectorCopy(cgs.beRedTeamColor, color);
	}
	else
	{
		VectorCopy(cgs.beBlueTeamColor, color);
	}
	CG_ColorWhitenSmart(color, NULL, 0.4f);
	color[3] = 1.0f;
}

static qboolean CG_WorldFlags_FindFlagItemNear(const vec3_t nearOrigin, qboolean redFlag, vec3_t outOrigin)
{
	int         i;
	int         modelIndex;
	centity_t*  cent;
	float       bestDistSq;
	float       distSq;
	qboolean    found;

	modelIndex = CG_WorldFlags_FlagModelIndexCached(redFlag);
	if (!modelIndex)
	{
		return qfalse;
	}

	found = qfalse;
	bestDistSq = CG_WORLDFLAGS_BASE_MATCH * CG_WORLDFLAGS_BASE_MATCH;

	for (i = 0; i < cg.numItemEntityIndices; i++)
	{
		cent = &cg_entities[cg.itemEntityIndices[i]];
		if (cent->currentState.modelindex != modelIndex)
		{
			continue;
		}
		if (cent->currentState.eFlags & EF_NODRAW)
		{
			continue;
		}
		distSq = DistanceSquared(cent->lerpOrigin, nearOrigin);
		if (distSq <= bestDistSq)
		{
			VectorCopy(cent->lerpOrigin, outOrigin);
			bestDistSq = distSq;
			found = qtrue;
		}
	}

	return found;
}

static qboolean CG_WorldFlags_IsOnScreen(const vec3_t origin, float* outX, float* outY)
{
	float x;
	float y;

	if (!CG_WorldToScreen(origin, &x, &y))
	{
		return qfalse;
	}
	if (x < 0.0f || x >= SCREEN_WIDTH || y < 0.0f || y >= SCREEN_HEIGHT)
	{
		return qfalse;
	}
	if (outX)
	{
		*outX = x;
	}
	if (outY)
	{
		*outY = y;
	}
	return qtrue;
}

static qboolean CG_WorldFlags_FlagItemVisible(const vec3_t nearOrigin, qboolean redFlag)
{
	vec3_t itemOrigin;

	if (!CG_WorldFlags_FindFlagItemNear(nearOrigin, redFlag, itemOrigin))
	{
		return qfalse;
	}
	if (!CG_IsPointVisibleFromView(itemOrigin))
	{
		return qfalse;
	}
	return CG_WorldFlags_IsOnScreen(itemOrigin, NULL, NULL);
}

static qboolean CG_WorldFlags_DroppedFlagEntityVisible(qboolean redFlag)
{
	vec3_t itemOrigin;

	if (!CG_WorldFlags_FindDroppedFlagOrigin(redFlag, itemOrigin))
	{
		return qfalse;
	}
	if (!CG_IsPointVisibleFromView(itemOrigin))
	{
		return qfalse;
	}
	return CG_WorldFlags_IsOnScreen(itemOrigin, NULL, NULL);
}

static qboolean CG_WorldFlags_FlagCarrierVisible(qboolean redFlag)
{
	int         i;
	int         powerup;
	centity_t*  cent;
	clientInfo_t* ci;
	vec3_t      pos;

	powerup = redFlag ? PW_REDFLAG : PW_BLUEFLAG;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
		{
			continue;
		}
		if (!(ci->powerups & (1 << powerup)))
		{
			continue;
		}

		cent = &cg_entities[i];
		if (cent->currentValid && cent->currentState.eType == ET_PLAYER)
		{
			VectorCopy(cent->lerpOrigin, pos);
		}
		else if (ci->globalPos)
		{
			VectorCopy(ci->globalPosOrigin, pos);
		}
		else
		{
			continue;
		}

		pos[2] += 24.0f;
		if (!CG_IsPointVisibleFromView(pos))
		{
			continue;
		}
		if (CG_WorldFlags_IsOnScreen(pos, NULL, NULL))
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_WorldFlags_SkipWorldDraw(qboolean redFlag, const vec3_t baseOrigin, int flagStatus,
        const vec3_t drawOrigin, qboolean isDropMarker)
{
	if (isDropMarker)
	{
		if (flagStatus == 1 && CG_WorldFlags_FlagCarrierVisible(redFlag))
		{
			return qtrue;
		}
		if (CG_WorldFlags_DroppedFlagEntityVisible(redFlag))
		{
			return qtrue;
		}
		return CG_WorldFlags_FlagItemVisible(drawOrigin, redFlag);
	}

	if (flagStatus == 0)
	{
		return CG_WorldFlags_FlagItemVisible(baseOrigin, redFlag);
	}

	if (flagStatus == 1)
	{
		return CG_WorldFlags_FlagCarrierVisible(redFlag);
	}

	return CG_WorldFlags_DroppedFlagEntityVisible(redFlag);
}

static void CG_WorldFlags_ClearDrop(cg_worldFlagState_t* st)
{
	st->dropValid = qfalse;
	st->dropClientNum = -1;
	st->dropAtGlobalPos = qfalse;
	st->dropSpawnTime = 0;
}

static void CG_WorldFlags_LatchDrop(qboolean redFlag, const vec3_t origin, int clientNum, qboolean atGlobalPos)
{
	cg_worldFlagState_t* st;

	st = redFlag ? &cg_worldFlagRed : &cg_worldFlagBlue;
	VectorCopy(origin, st->origin);
	st->dropValid = qtrue;
	st->dropClientNum = clientNum;
	st->dropAtGlobalPos = atGlobalPos;
}

static void CG_WorldFlags_LatchDropWithSpawn(qboolean redFlag, const vec3_t origin, int clientNum,
        qboolean atGlobalPos, int spawnTime)
{
	CG_WorldFlags_LatchDrop(redFlag, origin, clientNum, atGlobalPos);
	if (spawnTime > 0)
	{
		if (redFlag)
		{
			cg_worldFlagRed.dropSpawnTime = spawnTime;
		}
		else
		{
			cg_worldFlagBlue.dropSpawnTime = spawnTime;
		}
	}
}

static qboolean CG_WorldFlags_OriginMatchesDrop(const vec3_t origin, const cg_worldFlagState_t* st)
{
	if (!st->dropValid || st->dropSpawnTime <= 0)
	{
		return qfalse;
	}
	return (DistanceSquared(origin, st->origin) < (4.0f * 4.0f));
}

static int CG_WorldFlags_DropSpawnTimeAtOrigin(const vec3_t origin)
{
	if (CG_WorldFlags_OriginMatchesDrop(origin, &cg_worldFlagRed))
	{
		return cg_worldFlagRed.dropSpawnTime;
	}
	if (CG_WorldFlags_OriginMatchesDrop(origin, &cg_worldFlagBlue))
	{
		return cg_worldFlagBlue.dropSpawnTime;
	}
	return 0;
}

qboolean CG_WorldFlags_TryConsumeDroppedFlagTeamMark(int markType, const vec3_t origin, int clientNum)
{
	qboolean redFlag;
	int      flagStatus;

	if (cgs.gametype != GT_CTF || !(cg_worldFlags.integer & CG_WORLDFLAGS_WORLD) || !origin)
	{
		return qfalse;
	}

	if (markType == TEAMTASK_REDFLAG)
	{
		redFlag = qtrue;
		flagStatus = cgs.redflag;
	}
	else if (markType == TEAMTASK_BLUEFLAG)
	{
		redFlag = qfalse;
		flagStatus = cgs.blueflag;
	}
	else
	{
		return qfalse;
	}

	if (flagStatus != 2)
	{
		return qfalse;
	}

	CG_WorldFlags_LatchDropWithSpawn(redFlag, origin, -1, qfalse, cg.time);

	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		CG_SHUDEventTeamTask(clientNum, markType, qfalse);
	}

	return qtrue;
}

void CG_WorldFlags_OnFlagMark(int markType, const vec3_t origin)
{
	qboolean redFlag;
	int      flagStatus;

	if (cgs.gametype != GT_CTF || !cg_worldFlags.integer)
	{
		return;
	}

	if (markType == TEAMTASK_REDFLAG)
	{
		redFlag = qtrue;
		flagStatus = cgs.redflag;
	}
	else if (markType == TEAMTASK_BLUEFLAG)
	{
		redFlag = qfalse;
		flagStatus = cgs.blueflag;
	}
	else
	{
		return;
	}

	if (flagStatus != 2)
	{
		return;
	}

	CG_WorldFlags_LatchDrop(redFlag, origin, -1, qfalse);
}

static qboolean CG_WorldFlags_IsAllyClientNum(int clientNum)
{
	clientInfo_t* ci;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid || ci->team == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	return !CG_CACHED_IS_ENEMY_CI(ci);
}

static qboolean CG_WorldFlags_TeamTaskMarkFromAlly(const teamTaskMark_t* mark)
{
	if (!mark)
	{
		return qfalse;
	}
	if (mark->clientNum < 0 || mark->clientNum >= MAX_CLIENTS)
	{
		return qtrue;
	}
	return CG_WorldFlags_IsAllyClientNum(mark->clientNum);
}

static void CG_WorldFlags_AbsorbDroppedFlagTeamMarks(void)
{
	int                 i;
	int                 w;
	teamTaskMark_t*     mark;
	cgs_teamMarkers_t*  tm;
	int                 flagStatus;
	qboolean            redFlag;

	if (!(cg_worldFlags.integer & CG_WORLDFLAGS_WORLD) || cgs.gametype != GT_CTF)
	{
		return;
	}

	tm = &cg.teamMarkers;
	if (!tm->drawTeamTasks || tm->numTeamTaskMarks <= 0)
	{
		return;
	}

	w = 0;
	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];
		if (mark->markType == TEAMTASK_REDFLAG)
		{
			redFlag = qtrue;
			flagStatus = cgs.redflag;
		}
		else if (mark->markType == TEAMTASK_BLUEFLAG)
		{
			redFlag = qfalse;
			flagStatus = cgs.blueflag;
		}
		else
		{
			if (w != i)
			{
				tm->teamTaskMarks[w] = tm->teamTaskMarks[i];
			}
			w++;
			continue;
		}

		if (flagStatus == 2)
		{
			if (!CG_WorldFlags_TeamTaskMarkFromAlly(mark))
			{
				if (w != i)
				{
					tm->teamTaskMarks[w] = tm->teamTaskMarks[i];
				}
				w++;
				continue;
			}
			CG_WorldFlags_LatchDropWithSpawn(redFlag, mark->origin, -1, qfalse, mark->startTime);
			continue;
		}

		if (w != i)
		{
			tm->teamTaskMarks[w] = tm->teamTaskMarks[i];
		}
		w++;
	}
	tm->numTeamTaskMarks = w;
}

static qboolean CG_WorldFlags_AnyClientHasPowerup(int powerup)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cgs.clientinfo[i].infoValid)
		{
			continue;
		}
		if (cgs.clientinfo[i].powerups & (1 << powerup))
		{
			return qtrue;
		}
	}

	return qfalse;
}

static void CG_WorldFlags_UpdateDropFromMarks(void)
{
	int                 i;
	teamTaskMark_t*     mark;
	cgs_teamMarkers_t*  tm;
	int                 flagStatus;
	qboolean            redFlag;

	tm = &cg.teamMarkers;
	if (!tm->drawTeamTasks)
	{
		return;
	}

	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];
		if (mark->markType == TEAMTASK_REDFLAG)
		{
			redFlag = qtrue;
			flagStatus = cgs.redflag;
		}
		else if (mark->markType == TEAMTASK_BLUEFLAG)
		{
			redFlag = qfalse;
			flagStatus = cgs.blueflag;
		}
		else
		{
			continue;
		}

		if (flagStatus == 2 && CG_WorldFlags_TeamTaskMarkFromAlly(mark))
		{
			CG_WorldFlags_LatchDrop(redFlag, mark->origin, -1, qfalse);
		}
	}
}

static void CG_WorldFlags_UpdateDropState(qboolean redFlag, cg_worldFlagState_t* st, int flagStatus)
{
	vec3_t  entOrigin;
	int     powerup;

	powerup = redFlag ? PW_REDFLAG : PW_BLUEFLAG;

	if (flagStatus != 2)
	{
		CG_WorldFlags_ClearDrop(st);
		return;
	}

	if (CG_WorldFlags_AnyClientHasPowerup(powerup))
	{
		CG_WorldFlags_ClearDrop(st);
		return;
	}

	if (cg.snap && cg.snap->ps.powerups[powerup])
	{
		CG_WorldFlags_ClearDrop(st);
		return;
	}

	if (CG_WorldFlags_FindDroppedFlagOrigin(redFlag, entOrigin))
	{
		VectorCopy(entOrigin, st->origin);
		st->dropValid = qtrue;
		st->dropClientNum = -1;
		st->dropAtGlobalPos = qfalse;
	}
}

static int CG_WorldFlags_AllyCarriedEnemyFlagPowerup(int clientNum)
{
	clientInfo_t* ci;
	centity_t*    cent;
	int           powerup;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return 0;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid || CG_CACHED_IS_ENEMY_CI(ci) || ci->team == TEAM_SPECTATOR)
	{
		return 0;
	}

	powerup = 0;
	cent = &cg_entities[clientNum];
	if (cent->currentValid && cent->currentState.eType == ET_PLAYER)
	{
		if (cent->currentState.powerups & (1 << PW_BLUEFLAG))
		{
			powerup = PW_BLUEFLAG;
		}
		else if (cent->currentState.powerups & (1 << PW_REDFLAG))
		{
			powerup = PW_REDFLAG;
		}
	}
	if (!powerup)
	{
		if (ci->powerups & (1 << PW_BLUEFLAG))
		{
			powerup = PW_BLUEFLAG;
		}
		else if (ci->powerups & (1 << PW_REDFLAG))
		{
			powerup = PW_REDFLAG;
		}
	}

	if (ci->team == TEAM_RED && powerup == PW_BLUEFLAG)
	{
		return PW_BLUEFLAG;
	}
	if (ci->team == TEAM_BLUE && powerup == PW_REDFLAG)
	{
		return PW_REDFLAG;
	}

	return 0;
}

static qboolean CG_WorldFlags_ClientIsDead(int clientNum, clientInfo_t* ci)
{
	centity_t* cent;

	if (ci->globalPos)
	{
		return (ci->health <= 0);
	}

	cent = &cg_entities[clientNum];
	if (cent->currentValid && cent->currentState.eType == ET_PLAYER &&
	        (cent->currentState.eFlags & EF_DEAD))
	{
		return qtrue;
	}

	return (ci->health <= 0);
}

static void CG_WorldFlags_GetAllyDeathOrigin(int clientNum, clientInfo_t* ci, vec3_t outOrigin)
{
	if (cg.teamMarkers.drawTeamTasks && CG_WorldFlags_IsAllyClientNum(clientNum) &&
	        cg.teamMarkers.allyDeathPositionValid[clientNum])
	{
		VectorCopy(cg.teamMarkers.allyDeathPosition[clientNum], outOrigin);
		return;
	}

	if (ci->globalPos)
	{
		VectorCopy(ci->globalPosOrigin, outOrigin);
		return;
	}

	if (CG_ClientTeamMarkerOrigin(clientNum, outOrigin))
	{
		return;
	}

	VectorClear(outOrigin);
}

static void CG_WorldFlags_UpdateAllyCarrierDrops(void)
{
	int i;
	int carried;
	qboolean dead;
	qboolean redFlag;
	clientInfo_t* ci;
	vec3_t deathOrigin;

	if (!cg.snap)
	{
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
		{
			cg_worldFlagWasDead[i] = qfalse;
			continue;
		}

		dead = CG_WorldFlags_ClientIsDead(i, ci);
		if (CG_CACHED_IS_ENEMY_CI(ci))
		{
			cg_worldFlagWasDead[i] = dead;
			continue;
		}

		carried = CG_WorldFlags_AllyCarriedEnemyFlagPowerup(i);

		if (!cg_worldFlagWasDead[i] && dead && carried != 0)
		{
			redFlag = (carried == PW_REDFLAG);
			CG_WorldFlags_GetAllyDeathOrigin(i, ci, deathOrigin);
			CG_WorldFlags_LatchDrop(redFlag, deathOrigin, i, ci->globalPos);
		}

		cg_worldFlagWasDead[i] = dead;
	}
}

qboolean CG_WorldFlags_ClientHasStackDropFlag(int clientNum)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}
	if (cg_worldFlagRed.dropValid && cg_worldFlagRed.dropAtGlobalPos && cg_worldFlagRed.dropClientNum == clientNum)
	{
		return qtrue;
	}
	if (cg_worldFlagBlue.dropValid && cg_worldFlagBlue.dropAtGlobalPos && cg_worldFlagBlue.dropClientNum == clientNum)
	{
		return qtrue;
	}
	return qfalse;
}

void CG_WorldFlags_GetStackDropFlagColor(int clientNum, vec4_t color)
{
	if (cg_worldFlagRed.dropValid && cg_worldFlagRed.dropAtGlobalPos && cg_worldFlagRed.dropClientNum == clientNum)
	{
		CG_WorldFlags_GetFlagColor(qtrue, color);
		return;
	}
	CG_WorldFlags_GetFlagColor(qfalse, color);
}

static qboolean CG_WorldFlags_IsLiftedOnScreen(const vec3_t origin, float* outX, float* outY)
{
	vec3_t lifted;

	VectorCopy(origin, lifted);
	lifted[2] += 48.0f + CG_WORLDFLAGS_Z_LIFT;
	return CG_WorldFlags_IsOnScreen(lifted, outX, outY);
}

#define CG_WORLDFLAGS_FADE_RADIUS   256.0f

static float CG_WorldFlags_ComputeAlpha(const vec3_t origin)
{
	vec3_t  lifted;
	float   sx;
	float   sy;
	float   fade;
	float   radiusSq;
	float   dx;
	float   dy;
	float   distSq;
	float   coreFade;

	VectorCopy(origin, lifted);
	lifted[2] += 48.0f + CG_WORLDFLAGS_Z_LIFT;

	if (!CG_WorldToScreen(lifted, &sx, &sy))
	{
		return 1.0f;
	}

	fade = 1.0f;
	radiusSq = CG_WORLDFLAGS_FADE_RADIUS * CG_WORLDFLAGS_FADE_RADIUS;
	dx = sx - HALF_SCREEN_W;
	dy = sy - HALF_SCREEN_H;
	distSq = dx * dx + dy * dy;
	if (distSq < radiusSq)
	{
		coreFade = SQRTFAST(distSq) / CG_WORLDFLAGS_FADE_RADIUS;
		if (coreFade < 0.0f)
		{
			coreFade = 0.0f;
		}
		/* Fade toward crosshair (center); minimum 0.1 at center */
		fade = 0.05f + 0.95f * coreFade;
	}
	return fade;
}

static void CG_WorldFlags_RadiusToScreenIconSize(const vec3_t origin, float* iconW, float* iconH)
{
	vec3_t  spriteOrigin;
	vec3_t  lifted;
	float   dist;
	float   worldRadius;
	float   hfov_y;
	float   scale;

	VectorCopy(origin, spriteOrigin);
	VectorCopy(origin, lifted);
	lifted[2] += 48.0f + CG_WORLDFLAGS_Z_LIFT;

	worldRadius = CG_FloatSpriteRadiusAtDist(spriteOrigin, 1.0f,
	              CG_WORLDFLAGS_FLOAT_MAX_DIST, CG_WORLDFLAGS_Z_LIFT);
	dist = Distance(lifted, cg.refdef.vieworg);
	if (dist < 1.0f)
	{
		dist = 1.0f;
	}

	hfov_y = cg.refdef.fov_y * (float)M_PI / 360.0f;
	scale = ((float)cg.refdef.height * 0.5f) / (tan(hfov_y) * dist);
	*iconH = worldRadius * 2.0f * scale * CG_WORLDFLAGS_WORLD_ICON_SCALE;
	if (*iconH < 12.0f * CG_WORLDFLAGS_WORLD_ICON_SCALE)
	{
		*iconH = 12.0f * CG_WORLDFLAGS_WORLD_ICON_SCALE;
	}
	*iconW = *iconH * 0.65f;
}

static void CG_WorldFlags_DrawWorldPic2D(const vec3_t origin, qhandle_t shader, const vec4_t color)
{
	float   x;
	float   y;
	float   iconW;
	float   iconH;
	float   drawW;
	float   drawH;
	vec4_t  drawColor;
	vec4_t  spawnColor;
	float   alpha;
	int     spawnStart;
	int     timeSinceStart;
	float   spawnFade;
	float   spawnScale;
	float   spawnProgress;
	float   spawnAlpha;
	float   anchorY;
	int     appearanceDuration;
	int     spawnEffectDuration;

	if (!shader)
	{
		return;
	}

	alpha = color[3] * CG_WorldFlags_ComputeAlpha(origin);
	if (alpha <= 0.0f)
	{
		return;
	}
	if (!CG_WorldFlags_IsLiftedOnScreen(origin, &x, &y))
	{
		return;
	}

	CG_WorldFlags_RadiusToScreenIconSize(origin, &iconW, &iconH);
	anchorY = y;
	y = anchorY - iconH * 0.5f;
	spawnFade = 1.0f;
	spawnScale = 1.0f;
	spawnStart = CG_WorldFlags_DropSpawnTimeAtOrigin(origin);
	appearanceDuration = CG_WORLDFLAGS_MARK_APPEAR_MS;
	spawnEffectDuration = CG_WORLDFLAGS_MARK_SPAWN_MS;
	if (spawnStart > 0)
	{
		timeSinceStart = cg.time - spawnStart;
		if (timeSinceStart >= 0 && timeSinceStart < appearanceDuration)
		{
			spawnProgress = (float)timeSinceStart / (float)appearanceDuration;
			spawnFade = spawnProgress;
			spawnScale = 0.25f + spawnProgress * 0.75f;
		}
		if (timeSinceStart >= 0 && timeSinceStart < spawnEffectDuration)
		{
			spawnProgress = (float)timeSinceStart / (float)spawnEffectDuration;
			spawnAlpha = 1.0f - spawnProgress;
			drawW = iconW * (0.8f + spawnProgress * 1.5f);
			drawH = iconH * (0.8f + spawnProgress * 1.5f);
			Vector4Copy(color, spawnColor);
			spawnColor[3] = alpha * spawnAlpha;
			CG_DrawPicWithColor(x - drawW * 0.5f, y - (drawH - iconH) * 0.5f, drawW, drawH, spawnColor, shader);
		}
	}

	drawW = iconW * spawnScale;
	drawH = iconH * spawnScale;
	Vector4Copy(color, drawColor);
	drawColor[3] = alpha * spawnFade;
	CG_DrawPicWithColor(x - drawW * 0.5f, y - (drawH - iconH) * 0.5f, drawW, drawH, drawColor, shader);
}

static void CG_WorldFlags_DrawBorder(const vec3_t origin, const vec4_t color, qhandle_t markerShader)
{
	float       distance;
	float       hfov_x;
	float       size;
	float       maxDistValue;
	int         maxDistInt;
	float       maxScale;
	float       minScale;
	float       markerSize;
	float       baseRadius;
	vec4_t      fadedColor;
	float       fade;

	if (!markerShader)
	{
		return;
	}

	fade = CG_WorldFlags_ComputeAlpha(origin);
	if (fade <= 0.0f)
	{
		return;
	}

	maxDistInt = cg_teamTaskHudMarkerMaxDist.integer;
	maxDistValue = cg_teamTaskHudMarkerMaxDist.value;
	maxScale = cg_teamTaskHudMarkerMaxScale.value;
	minScale = cg_teamTaskHudMarkerMinScale.value;
	markerSize = cg_teamTaskHudMarkerSize.value;

	distance = Distance(origin, cg.predictedPlayerState.origin);
	if (maxDistInt > 0 && distance > maxDistValue)
	{
		return;
	}

	distance = MAX(distance, 1.0f);
	hfov_x = cg.refdef.fov_x * (float)M_PI / 360.0f;
	size = 1.0f / tan(hfov_x) * 1000.0f / distance;
	size = MIN(maxScale, size);
	size = MAX(minScale, size);
	baseRadius = markerSize * size;

	Vector4Copy(color, fadedColor);
	fadedColor[3] *= fade;
	CG_HudBorderMarker((float*)origin, fadedColor, baseRadius, markerShader, 0);
}

static void CG_WorldFlags_DrawPoint(const vec3_t origin, qhandle_t shader, const vec4_t color,
                                    qhandle_t borderShader, qboolean allowWorld, qboolean allowBorder)
{
	float x;
	float y;

	if (allowWorld)
	{
		CG_WorldFlags_DrawWorldPic2D(origin, shader, color);
	}
	if (allowBorder && (!(CG_WorldToScreen(origin, &x, &y)) ||
	                    x < 0.0f || x >= SCREEN_WIDTH || y < 0.0f || y >= SCREEN_HEIGHT))
	{
		CG_WorldFlags_DrawBorder(origin, color, borderShader);
	}
}

static void CG_WorldFlags_DrawOneFlag(qboolean redFlag, const vec3_t baseOrigin, int flagStatus,
                                      cg_worldFlagState_t* st, int worldFlags)
{
	vec4_t  color;
	vec4_t  takenColor;
	qboolean allowWorld;
	qboolean allowBorder;
	qboolean atBase;
	qhandle_t borderAtBase;
	qhandle_t borderTaken;

	allowWorld = (worldFlags & CG_WORLDFLAGS_WORLD) != 0;
	allowBorder = (worldFlags & CG_WORLDFLAGS_HUDMARKER) != 0;
	if (!allowWorld && !allowBorder)
	{
		return;
	}

	borderAtBase = cgs.media.teamTasksFlag_marker;
	borderTaken = cgs.media.worldFlagsFlagTaken_marker;
	if (!borderTaken)
	{
		borderTaken = borderAtBase;
	}

	CG_WorldFlags_GetFlagColor(redFlag, color);
	Vector4Copy(color, takenColor);
	takenColor[3] = 0.75f;

	atBase = (flagStatus == 0);

	if (atBase)
	{
		if (allowWorld && !CG_WorldFlags_SkipWorldDraw(redFlag, baseOrigin, flagStatus, baseOrigin, qfalse))
		{
			CG_WorldFlags_DrawPoint(baseOrigin, cgs.media.teamTasksFlag, color, borderAtBase, qtrue, qfalse);
		}
		if (allowBorder)
		{
			CG_WorldFlags_DrawPoint(baseOrigin, cgs.media.teamTasksFlag, color, borderAtBase, qfalse, qtrue);
		}
	}
	else
	{
		if (allowWorld && !CG_WorldFlags_SkipWorldDraw(redFlag, baseOrigin, flagStatus, baseOrigin, qfalse))
		{
			CG_WorldFlags_DrawPoint(baseOrigin, cgs.media.worldFlagsFlagTaken, takenColor, borderTaken, qtrue, qfalse);
		}
		if (allowBorder)
		{
			CG_WorldFlags_DrawPoint(baseOrigin, cgs.media.worldFlagsFlagTaken, takenColor, borderTaken, qfalse, qtrue);
		}
	}

	if (!atBase && flagStatus == 2 && st->dropValid)
	{
		qboolean drawWorldDrop;
		qboolean drawBorderDrop;

		drawWorldDrop = allowWorld &&
		                !CG_WorldFlags_SkipWorldDraw(redFlag, baseOrigin, flagStatus, st->origin, qtrue);
		drawBorderDrop = allowBorder;
		if (drawWorldDrop || drawBorderDrop)
		{
			CG_WorldFlags_DrawPoint(st->origin, cgs.media.teamTasksFlag, color, borderAtBase,
			                        drawWorldDrop, drawBorderDrop);
		}
	}
}

void CG_WorldFlags_Update(void)
{
	if (cgs.gametype != GT_CTF || !cg_worldFlags.integer)
	{
		return;
	}

	CG_WorldFlags_UpdateAllyCarrierDrops();
	CG_WorldFlags_AbsorbDroppedFlagTeamMarks();
	CG_WorldFlags_UpdateDropFromMarks();
	CG_WorldFlags_UpdateDropState(qtrue, &cg_worldFlagRed, cgs.redflag);
	CG_WorldFlags_UpdateDropState(qfalse, &cg_worldFlagBlue, cgs.blueflag);
}

void CG_WorldFlags_DrawWorld(void)
{
	vec3_t  redBase;
	vec3_t  blueBase;
	int     flags;

	if (!cg.snap || cgs.gametype != GT_CTF)
	{
		return;
	}

	flags = cg_worldFlags.integer & CG_WORLDFLAGS_WORLD;
	if (!flags)
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	if (!CG_MapSpawns_GetFlagStandOrigin(qtrue, redBase))
	{
		return;
	}
	if (!CG_MapSpawns_GetFlagStandOrigin(qfalse, blueBase))
	{
		return;
	}

	if (cgs.redflag >= 0)
	{
		CG_WorldFlags_DrawOneFlag(qtrue, redBase, cgs.redflag, &cg_worldFlagRed, flags);
	}
	if (cgs.blueflag >= 0)
	{
		CG_WorldFlags_DrawOneFlag(qfalse, blueBase, cgs.blueflag, &cg_worldFlagBlue, flags);
	}
}

void CG_WorldFlags_AddSceneSprites(void)
{
	/* World icons are drawn in 2D (CG_WorldFlags_DrawWorld); 3D RT_SPRITE shaders do not show. */
}

void CG_WorldFlags_DrawBorderHudMarkers(void)
{
	vec3_t  redBase;
	vec3_t  blueBase;
	int     flags;

	if (!cg.snap || cgs.gametype != GT_CTF)
	{
		return;
	}

	flags = cg_worldFlags.integer;
	if (!(flags & CG_WORLDFLAGS_HUDMARKER))
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	if (!CG_MapSpawns_GetFlagStandOrigin(qtrue, redBase))
	{
		return;
	}
	if (!CG_MapSpawns_GetFlagStandOrigin(qfalse, blueBase))
	{
		return;
	}

	flags = CG_WORLDFLAGS_HUDMARKER;
	if (cgs.redflag >= 0)
	{
		CG_WorldFlags_DrawOneFlag(qtrue, redBase, cgs.redflag, &cg_worldFlagRed, flags);
	}
	if (cgs.blueflag >= 0)
	{
		CG_WorldFlags_DrawOneFlag(qfalse, blueBase, cgs.blueflag, &cg_worldFlagBlue, flags);
	}
}
