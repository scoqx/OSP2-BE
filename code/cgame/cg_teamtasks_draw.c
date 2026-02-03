/*
===========================================================================
cg_teamtasks_draw.c

Team task marker rendering functions

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com

This file contains original work created by diwoc.
It is included in the OSP2-BE modification for Quake III Arena and is
distributed under the terms of the project's primary license.

Additional licensing:
Original portions authored by diwoc may also be used under the
DIWOC Custom Non-Commercial License v1.0, provided such portions are
separated from all id Software code and Quake III Arena–derived material.

For details, refer to LICENSE_DIWOC.txt.
===========================================================================
*/
//
// cg_teamtasks_draw.c -- team task marker rendering functions

#include "cg_local.h"

#define TEAMTASK_MARK_FADE_TIME 800 /* Fade duration in milliseconds (0.8 seconds) */

/*
=================
CG_SetWhiteColor

Set color to white (1.0, 1.0, 1.0, 1.0)
=================
*/
static void CG_SetWhiteColor(vec4_t color)
{
	Vector4Set(color, 1.0f, 1.0f, 1.0f, 1.0f);
}

/*
=================
CG_GetTeamColorForFrozenMark

Get team color for frozen mark (ally or enemy)
Returns qtrue if color was set, qfalse otherwise
=================
*/
static qboolean CG_GetTeamColorForFrozenMark(int markType, vec4_t color, qboolean isAlly)
{
	int myRealTeam;

	myRealTeam = cgs.clientinfo[cg.clientNum].team;

	/* Determine which team color to use and copy it */
	if (isAlly)
	{
		/* Use own team color */
		if (myRealTeam == TEAM_RED)
		{
			VectorCopy(cgs.beRedTeamColor, color);
		}
		else if (myRealTeam == TEAM_BLUE)
		{
			VectorCopy(cgs.beBlueTeamColor, color);
		}
		else
		{
			return qfalse;
		}
	}
	else
	{
		/* Use enemy team color */
		if (myRealTeam == TEAM_RED)
		{
			VectorCopy(cgs.beBlueTeamColor, color);
		}
		else if (myRealTeam == TEAM_BLUE)
		{
			VectorCopy(cgs.beRedTeamColor, color);
		}
		else
		{
			return qfalse;
		}
	}

	/* Apply whitening (called once for all valid cases) */
	CG_ColorWhitenSmart(color, NULL, 0.4f);
	return qtrue;
}

/*
=================
CG_CalculateEndFade

Calculate fade effect based on time until mark expires
Returns fade value from 0.0 to 1.0
=================
*/
static float CG_CalculateEndFade(int endTime, int currentTime)
{
	int timeUntilEnd;
	float endFade;

	timeUntilEnd = endTime - currentTime;
	if (timeUntilEnd <= TEAMTASK_MARK_FADE_TIME)
	{
		endFade = (float)timeUntilEnd / (float)TEAMTASK_MARK_FADE_TIME;
		if (endFade < 0.0f)
		{
			endFade = 0.0f;
		}
	}
	else
	{
		endFade = 1.0f;
	}
	return endFade;
}

/*
=================
CG_GetTeamTaskShaderPair

Get shader pair (marker and world) for team task type
=================
*/
static void CG_GetTeamTaskShaderPair(int markType, qhandle_t* markerShader, qhandle_t* worldShader, qboolean isPickup)
{
	cgMedia_t* media;
	cgs_osp_t* osp;

	media = &cgs.media;
	osp = &cgs.osp;

	switch (markType)
	{
		case TEAMTASK_OFFENSE:
			*markerShader = media->teamTasksAttack_marker;
			*worldShader = media->teamTasksAttack;
			break;
		case TEAMTASK_DEFENCE:
			*markerShader = media->teamTasksDefence_marker;
			*worldShader = media->teamTasksDefence;
			break;
		case TEAMTASK_PATROL:
			*markerShader = media->teamTasksWatch_marker;
			*worldShader = media->teamTasksWatch;
			break;
		case TEAMTASK_PLAYERDEAD:
			*markerShader = media->teamTasksDead_marker;
			*worldShader = media->teamTasksDead;
			break;
		case TEAMTASK_YES:
			*markerShader = media->teamTasksYes;
			*worldShader = media->teamTasksYes;
			break;
		case TEAMTASK_NO:
			*markerShader = media->teamTasksNo;
			*worldShader = media->teamTasksNo;
			break;
		case TEAMTASK_LOCATION:
			*markerShader = media->teamTasksLocation_marker;
			*worldShader = media->teamTasksLocation;
			break;
		case TEAMTASK_TARGET:
			*markerShader = media->teamTasksTarget_marker;
			*worldShader = media->teamTasksTarget;
			break;
		case TEAMTASK_REDARMOR:
			if (isPickup)
			{
				*markerShader = media->teamTasksRedarmorPickup_marker;
				*worldShader = media->teamTasksRedarmorPickup;
			}
			else
			{
				*markerShader = media->teamTasksRedarmor_marker;
				*worldShader = media->teamTasksRedarmor;
			}
			break;
		case TEAMTASK_MEGAHEALTH:
			if (isPickup)
			{
				*markerShader = media->teamTasksMegahealthPickup_marker;
				*worldShader = media->teamTasksMegahealthPickup;
			}
			else
			{
				*markerShader = media->teamTasksMegahealth_marker;
				*worldShader = media->teamTasksMegahealth;
			}
			break;
		case TEAMTASK_MEDIC:
			*markerShader = media->teamTasksMedic_marker;
			*worldShader = media->teamTasksMedic;
			break;
		case TEAMTASK_HERE_ALIVE:
			*markerShader = media->teamTasksLocation_marker;
			*worldShader = media->teamTasksLocation;
			break;
		case TEAMTASK_HERE_FLAG:
			*markerShader = media->teamTasksFlagcarrier_marker;
			*worldShader = media->teamTasksFlagcarrier;
			break;
		case TEAMTASK_HERE_DEAD:
			/* Use frozen shader in freeze mode, otherwise use dead shader */
			if (cgs.gametype == GT_TEAM && osp->gameTypeFreeze)
			{
				*markerShader = media->teamTasksFrozen_marker;
				*worldShader = media->frozenFoeTagShader;
			}
			else
			{
				*markerShader = media->teamTasksDead_marker;
				*worldShader = media->teamTasksDead;
			}
			break;
		case TEAMTASK_WARN_ALIVE:
			*markerShader = media->teamTasksWarning_marker;
			*worldShader = media->teamTasksWarning;
			break;
		case TEAMTASK_WARN_DEAD:
			/* Always use warning shader for WARN_DEAD */
			*markerShader = media->teamTasksWarning_marker;
			*worldShader = media->teamTasksWarning;
			break;
		case TEAMTASK_FROZEN_ALLY:
		case TEAMTASK_FROZEN_ENEMY:
			*markerShader = media->teamTasksFrozen_marker;
			*worldShader = media->frozenFoeTagShader;
			break;
		case TEAMTASK_QUADDAMAGE:
			*markerShader = media->teamTasksQuad_marker;
			*worldShader = media->teamTasksQuad;
			break;
		case TEAMTASK_REGENERATION:
			*markerShader = media->teamTasksRegen_marker;
			*worldShader = media->teamTasksRegen;
			break;
		case TEAMTASK_BATTLESUIT:
			*markerShader = media->teamTasksBattlesuit_marker;
			*worldShader = media->teamTasksBattlesuit;
			break;
		case TEAMTASK_HASTE:
			*markerShader = media->teamTasksHaste_marker;
			*worldShader = media->teamTasksHaste;
			break;
		case TEAMTASK_YELLOWARMOR:
			if (isPickup)
			{
				*markerShader = media->teamTasksYellowarmorPickup_marker;
				*worldShader = media->teamTasksYellowarmorPickup;
			}
			else
			{
				*markerShader = media->teamTasksYellowarmor_marker;
				*worldShader = media->teamTasksYellowarmor;
			}
			break;
		case TEAMTASK_HEALTH:
			*markerShader = media->teamTasksHealth_marker;
			*worldShader = media->teamTasksHealth;
			break;
		case TEAMTASK_REDFLAG:
		case TEAMTASK_BLUEFLAG:
			*markerShader = media->teamTasksFlag_marker;
			*worldShader = media->teamTasksFlag;
			break;
		case TEAMTASK_REDFLAG_CARRIER:
		case TEAMTASK_BLUEFLAG_CARRIER:
			*markerShader = media->teamTasksFlagcarrier_marker;
			*worldShader = media->teamTasksFlagcarrier;
			break;
		default:
			*markerShader = media->teamTasksUnknown;
			*worldShader = media->teamTasksUnknown;
			break;
	}
}

void CG_GetTeamTaskMarkerShaderAndColor(int markType, qhandle_t* shader, vec4_t color, qboolean useMarkerShader, int clientNum, qboolean isPickup)
{
	clientInfo_t* cl;
	qhandle_t markerShader;
	qhandle_t worldShader;


	/* Get shader pair for this mark type */
	CG_GetTeamTaskShaderPair(markType, &markerShader, &worldShader, isPickup);
	*shader = useMarkerShader ? markerShader : worldShader;

	/* Set color based on mark type */
	switch (markType)
	{
		case TEAMTASK_YES:
			Vector4Set(color, 0.7f, 1.0f, 0.7f, 1.0f);
			break;
		case TEAMTASK_NO:
			Vector4Set(color, 1.0f, 0.7f, 0.7f, 1.0f);
			break;
		case TEAMTASK_MEDIC:
			/* Default white color */
			CG_SetWhiteColor(color);
			/* Check health and armor of client who sent the command */
			if (clientNum >= 0 && clientNum < MAX_CLIENTS)
			{
				cl = &cgs.clientinfo[clientNum];
				if (!(cg_healthColorLevels.integer & 2) && cl->health > 0)
				{
					Vector4Copy(cgs.healthColorByClient[clientNum], color);
				}
			}
			break;
		case TEAMTASK_FROZEN_ALLY:
			if (useMarkerShader)
			{
				/* HUD marker color is white */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is own team color */
				if (!CG_GetTeamColorForFrozenMark(markType, color, qtrue))
				{
					CG_SetWhiteColor(color);
				}
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_FROZEN_ENEMY:
			if (useMarkerShader)
			{
				/* HUD marker color is white */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is enemy team color */
				if (!CG_GetTeamColorForFrozenMark(markType, color, qfalse))
				{
					CG_SetWhiteColor(color);
				}
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_HERE_DEAD:
			if (useMarkerShader)
			{
				/* HUD marker color is white */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is own team color (HERE_DEAD is always for own team) */
				if (!CG_GetTeamColorForFrozenMark(markType, color, qtrue))
				{
					CG_SetWhiteColor(color);
				}
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_HERE_FLAG:
			if (useMarkerShader)
			{
				/* HUD marker (overhead) color is white */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is enemy team color relative to sender (player always carries enemy flag) */
				if (clientNum >= 0 && clientNum < MAX_CLIENTS)
				{
					clientInfo_t* senderCi;
					senderCi = &cgs.clientinfo[clientNum];
					if (senderCi->infoValid)
					{
						/* Get sender's team and use enemy team color */
						if (senderCi->team == TEAM_RED)
						{
							/* Sender is RED, carries BLUE flag */
							VectorCopy(cgs.beBlueTeamColor, color);
						}
						else if (senderCi->team == TEAM_BLUE)
						{
							/* Sender is BLUE, carries RED flag */
							VectorCopy(cgs.beRedTeamColor, color);
						}
						else
						{
							CG_SetWhiteColor(color);
						}
					}
					else
					{
						CG_SetWhiteColor(color);
					}
				}
				else
				{
					CG_SetWhiteColor(color);
				}
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_REDFLAG:
			if (useMarkerShader)
			{
				/* HUD marker: white (not colored), pulsing circle will be colored */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is red team color, lightened like frozen marks */
				VectorCopy(cgs.beRedTeamColor, color);
				CG_ColorWhitenSmart(color, NULL, 0.4f);
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_BLUEFLAG:
			if (useMarkerShader)
			{
				/* HUD marker: white (not colored), pulsing circle will be colored */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker: blue team color, stronger lighten so blue becomes pastel (0.6 leaves B=1 too saturated) */
				VectorCopy(cgs.beBlueTeamColor, color);
				CG_ColorWhitenSmart(color, NULL, 0.4f);
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_REDFLAG_CARRIER:
			if (useMarkerShader)
			{
				/* HUD marker: white (not colored), pulsing circle will be colored */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker color is red team color, lightened like frozen marks */
				VectorCopy(cgs.beRedTeamColor, color);
				CG_ColorWhitenSmart(color, NULL, 0.4f);
				color[3] = 1.0f;
			}
			break;
		case TEAMTASK_BLUEFLAG_CARRIER:
			if (useMarkerShader)
			{
				/* HUD marker: white (not colored), pulsing circle will be colored */
				CG_SetWhiteColor(color);
			}
			else
			{
				/* World marker: blue team color, stronger lighten so blue becomes pastel (0.6 leaves B=1 too saturated) */
				VectorCopy(cgs.beBlueTeamColor, color);
				CG_ColorWhitenSmart(color, NULL, 0.4f);
				color[3] = 1.0f;
			}
			break;
		default:
			CG_SetWhiteColor(color);
			break;
	}
}

/*
=================
CG_TeamTaskUpdateFrame

Single place per frame: pack (expired + event flags), then all per-frame caches for hot paths.
- drawTeamTasks, teamTaskFlags, clientHasOverhead, markHasPickupNear, numFrozenAllyMarks, clientHasFrozenAllyMarkNear
- markDurationMs (cvar clamp 500–5000)
- Draw constants: teamTaskFade, teamTaskFadeRadius, teamTaskScale, teamTaskIconMinSize, teamTaskOpaque
  (CG_InitTeamTaskMarkConstants uses these; avoids 5 cvar reads per frame in draw path)
- HUD border marker: hudMarkerMaxDistInt/Value, hudMarkerMaxScale/MinScale, hudMarkerSize
  (CG_DrawTeamTaskBorderMarker uses these; avoids 5 cvar reads per mark)
Registration (add) is only in CG_TeamTaskMark; removal is only here. Hot paths only read cached fields.
=================
*/
void CG_TeamTaskUpdateFrame(void)
{
	int i;
	int j;
	int w;
	teamTaskMark_t* mark;
	teamTaskMark_t* other;
	cgs_teamMarkers_t* tm;
	qboolean roundMark;
	qboolean localDeathMark;
	qboolean hereDeadRemove;

	tm = &cgs.teamMarkers;

	/* Lifetime: single place — refresh from cvar so CG_TeamTaskMark uses cache (no cvar read in add path) */
	tm->markDurationMs = cg_teamTaskMarkTime.integer;
	if (tm->markDurationMs < 500)
		tm->markDurationMs = 500;
	if (tm->markDurationMs > 5000)
		tm->markDurationMs = 5000;

	/* One combined pack: expired + round + local death + HERE_DEAD for mask */
	if (tm->numTeamTaskMarks > 0)
	{
		w = 0;
		for (i = 0; i < tm->numTeamTaskMarks; i++)
		{
			mark = &tm->teamTaskMarks[i];
			if (cg.time >= mark->endTime)
			{
				continue;
			}
			roundMark = (mark->markType == TEAMTASK_FROZEN_ALLY ||
			             mark->markType == TEAMTASK_FROZEN_ENEMY ||
			             mark->markType == TEAMTASK_HERE_DEAD ||
			             mark->markType == TEAMTASK_WARN_DEAD ||
			             mark->markType == TEAMTASK_PLAYERDEAD);
			localDeathMark = (mark->clientNum == cg.clientNum &&
			                  (mark->markType == TEAMTASK_PLAYERDEAD ||
			                   mark->markType == TEAMTASK_HERE_DEAD ||
			                   mark->markType == TEAMTASK_WARN_DEAD));
			hereDeadRemove = (mark->markType == TEAMTASK_HERE_DEAD &&
			                  mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS &&
			                  ((mark->clientNum < 32 && (tm->packHereDeadClientMask0 & (1 << mark->clientNum))) ||
			                   (mark->clientNum >= 32 && (tm->packHereDeadClientMask1 & (1 << (mark->clientNum - 32))))));
			if ((tm->packRoundNextFrame && roundMark) ||
			    (tm->packLocalDeathNextFrame && localDeathMark) || hereDeadRemove)
				continue;
			if (w != i)
				tm->teamTaskMarks[w] = tm->teamTaskMarks[i];
			w++;
		}
		tm->numTeamTaskMarks = w;
	}
	tm->packRoundNextFrame = qfalse;
	tm->packLocalDeathNextFrame = qfalse;
	tm->packHereDeadClientMask1 = 0;
	tm->packHereDeadClientMask0 = 0;

	tm->teamTaskFlags = cg_teamTask.integer;
	tm->teamTaskEnabled = (tm->teamTaskFlags != 0 || CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS));
	tm->drawTeamTasks = tm->teamTaskEnabled && cgs.gametype >= GT_TEAM && !CG_IS_LOCAL_CLIENT_SPECTATOR();

	/* Cached draw/hud cvars: one read per frame here; hot paths (CG_InitTeamTaskMarkConstants, CG_DrawTeamTaskBorderMarker) use cache */
	tm->teamTaskFade = cg_teamTaskFade.value;
	tm->teamTaskFadeRadius = cg_teamTaskFadeRadius.value;
	tm->teamTaskScale = cg_teamTaskScale.value;
	tm->teamTaskIconMinSize = cg_teamTaskIconMinSize.value;
	tm->teamTaskOpaque = cg_teamTaskOpaque.value;
	tm->hudMarkerMaxDistInt = cg_teamTaskHudMarkerMaxDist.integer;
	tm->hudMarkerMaxDistValue = cg_teamTaskHudMarkerMaxDist.value;
	tm->hudMarkerMaxScale = cg_teamTaskHudMarkerMaxScale.value;
	tm->hudMarkerMinScale = cg_teamTaskHudMarkerMinScale.value;
	tm->hudMarkerSize = cg_teamTaskHudMarkerSize.value;

	/* One pass: clientHasOverhead + markHasPickupNear (no separate loops) */
	memset(tm->clientHasOverhead, 0, sizeof(tm->clientHasOverhead));
	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];
		if (mark->isOverhead && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
			tm->clientHasOverhead[mark->clientNum] = qtrue;
		tm->markHasPickupNear[i] = qfalse;
		if (!mark->isPickup &&
		    (mark->markType == TEAMTASK_REDARMOR || mark->markType == TEAMTASK_MEGAHEALTH || mark->markType == TEAMTASK_YELLOWARMOR))
		{
			for (j = 0; j < tm->numTeamTaskMarks; j++)
			{
				other = &tm->teamTaskMarks[j];
				if (other->isPickup && other->markType == mark->markType &&
				    Distance(mark->origin, other->origin) <= 64.0f)
				{
					tm->markHasPickupNear[i] = qtrue;
					break;
				}
			}
		}
	}

	/* Frozen ally: positions and per-client "has mark near" (hot path only reads clientHasFrozenAllyMarkNear) */
	tm->numFrozenAllyMarks = 0;
	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];
		if (mark->markType == TEAMTASK_FROZEN_ALLY && cg.time < mark->endTime)
		{
			if (tm->numFrozenAllyMarks < MAX_TEAMTASK_MARKS)
			{
				VectorCopy(mark->origin, tm->frozenAllyMarkOrigin[tm->numFrozenAllyMarks]);
				tm->numFrozenAllyMarks++;
			}
		}
	}

	memset(tm->clientHasFrozenAllyMarkNear, 0, sizeof(tm->clientHasFrozenAllyMarkNear));
	if (tm->numFrozenAllyMarks > 0 && cgs.osp.gameTypeFreeze)
	{
		centity_t* cent;
		int clientNum;

		for (clientNum = 0; clientNum < MAX_CLIENTS; clientNum++)
		{
			if (!cgs.clientinfo[clientNum].isFrozenEnt)
				continue;
			if (!cgs.clientinfo[clientNum].infoValid || CG_IS_ENEMY_SIMPLE(&cgs.clientinfo[clientNum]))
				continue;
			cent = &cg_entities[cgs.clientinfo[clientNum].frozenEntity];
			if (!cent->currentValid)
				continue;
			for (i = 0; i < tm->numFrozenAllyMarks; i++)
			{
				if (Distance(tm->frozenAllyMarkOrigin[i], cent->lerpOrigin) <= 64.0f)
				{
					tm->clientHasFrozenAllyMarkNear[clientNum] = qtrue;
					break;
				}
			}
		}
	}
}

void CG_DrawTeamTaskBorderHudMarkers(void)
{
	int i;
	teamTaskMark_t* mark;
	cgs_teamMarkers_t* tm;
	float x;
	float y;

	tm = &cgs.teamMarkers;
	if (!tm->drawTeamTasks || tm->numTeamTaskMarks == 0 || !(tm->teamTaskFlags & CG_TEAMTASK_HUDMARKER))
		return;

	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];

		/* Skip own marks: overhead, pickup, or HERE_ALIVE/HERE_FLAG in first person */
		if (mark->clientNum == cg.clientNum &&
		        (mark->isOverhead || mark->isPickup ||
		         ((mark->markType == TEAMTASK_HERE_ALIVE || mark->markType == TEAMTASK_HERE_FLAG) && !cg.renderingThirdPerson)))
		{
			continue;
		}

		/* Skip item marks that have pickup nearby (precomputed in CG_TeamTaskUpdateFrame) */
		if (tm->markHasPickupNear[i])
			continue;

		/* Inline CG_IsPointOutsideScreen(origin, NULL, NULL) */
		if (!CG_WorldToScreen(mark->origin, &x, &y) ||
		        x < 0.0f || x >= SCREEN_WIDTH || y < 0.0f || y >= SCREEN_HEIGHT)
		{
			CG_DrawTeamTaskBorderMarker(mark);
		}
	}
}

void CG_DrawTeamTaskBorderMarker(teamTaskMark_t* mark)
{
	vec4_t color;
	float distance;
	float hfov_x;
	float size;
	float baseRadius;
	float pulseTime;
	float pulseProgress;
	float pulseScale;
	vec4_t pulseColor;
	float endFade;
	refdef_t* refdef;
	int maxDistInt;
	float maxDistValue;
	float maxScale;
	float minScale;
	float markerSize;

	if (!mark || !mark->markerShader)
		return;

	Vector4Copy(mark->markerColor, color);

	/* Use per-frame cache (set in CG_TeamTaskUpdateFrame); avoids 5 cvar reads per mark */
	maxDistInt = cgs.teamMarkers.hudMarkerMaxDistInt;
	maxDistValue = cgs.teamMarkers.hudMarkerMaxDistValue;
	maxScale = cgs.teamMarkers.hudMarkerMaxScale;
	minScale = cgs.teamMarkers.hudMarkerMinScale;
	markerSize = cgs.teamMarkers.hudMarkerSize;

	refdef = &cg.refdef;

	distance = Distance(mark->origin, cg.predictedPlayerState.origin);

	/* Check maximum distance */
	if (maxDistInt > 0 && distance > maxDistValue)
	{
		return;
	}

	distance = MAX(distance, 1.0f);
	hfov_x = refdef->fov_x * (float)M_PI / 360.0f;
	size = 1.0f / tan(hfov_x) * 1000.0f / distance;
	size = MIN(maxScale, size);
	size = MAX(minScale, size);

	baseRadius = markerSize * size;

	/* Calculate end fade effect (linear fade from 1.0 to 0.0 over TEAMTASK_MARK_FADE_TIME before endTime) */
	endFade = CG_CalculateEndFade(mark->endTime, cg.time);

	/* Apply fade to main marker color */
	{
		vec4_t fadedColor;
		Vector4Copy(color, fadedColor);
		fadedColor[3] *= endFade;
		CG_HudBorderMarker(mark->origin, fadedColor, baseRadius, mark->markerShader, 0);
	}

	/* Draw pulsing circle effect */
	if (cgs.media.teamTasksCircle)
	{
		vec3_t dir;
		float front, left, up;
		vec3_t flu;
		float r, inc, az;
		float hfov_y;
		refEntity_t ent;
		float phi;
		float dist;
		vec3_t middleOfPlane;
		vec3_t planeAxis[2];
		float ratio;
		float circleSz;
		float h1, h2;
		float cosPhi, sinPhi;

		/* Calculate pulse animation - more pronounced effect */
		pulseTime = (cg.time - mark->startTime) / 1000.0f;
		pulseTime = pulseTime - floor(pulseTime); /* Keep in 0.0 to 1.0 range */

		/* Use smoothstep for more pronounced pulse effect */
		if (pulseTime < 0.5f)
		{
			/* Grow from 0.9 to 1.5 in first half */
			pulseProgress = pulseTime * 2.0f; /* 0.0 to 1.0 */
			pulseProgress = pulseProgress * pulseProgress * (3.0f - 2.0f * pulseProgress); /* Smoothstep */
			pulseScale = 0.9f + pulseProgress * (1.5f - 0.9f);
		}
		else
		{
			/* Shrink from 1.5 to 0.9 in second half */
			pulseProgress = (pulseTime - 0.5f) * 2.0f; /* 0.0 to 1.0 */
			pulseProgress = pulseProgress * pulseProgress * (3.0f - 2.0f * pulseProgress); /* Smoothstep */
			pulseScale = 1.5f - pulseProgress * (1.5f - 0.9f);
		}

		/* Set color with alpha 0.25 and apply fade */
		if (mark->markType == TEAMTASK_FROZEN_ALLY || mark->markType == TEAMTASK_FROZEN_ENEMY || mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD)
		{
			/* For frozen bodies, HERE_DEAD, and WARN_DEAD, use team color for pulsing circle */
			qboolean isAlly;
			if (mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD)
			{
				/* HERE_DEAD is always for own team */
				isAlly = qtrue;
			}
			else
			{
				isAlly = (mark->markType == TEAMTASK_FROZEN_ALLY);
			}
			if (!CG_GetTeamColorForFrozenMark(mark->markType, pulseColor, isAlly))
			{
				Vector4Copy(color, pulseColor);
			}
			pulseColor[3] = 0.25f * endFade;
		}
		else if (mark->markType == TEAMTASK_REDFLAG || mark->markType == TEAMTASK_REDFLAG_CARRIER)
		{
			/* For red flag and carrier, use red team color for pulsing circle */
			VectorCopy(cgs.beRedTeamColor, pulseColor);
			pulseColor[3] = 0.25f * endFade;
		}
		else if (mark->markType == TEAMTASK_BLUEFLAG || mark->markType == TEAMTASK_BLUEFLAG_CARRIER)
		{
			/* For blue flag and carrier, use blue team color for pulsing circle */
			VectorCopy(cgs.beBlueTeamColor, pulseColor);
			pulseColor[3] = 0.25f * endFade;
		}
		else if (mark->markType == TEAMTASK_HERE_FLAG)
		{
			/* For HERE_FLAG, use enemy team color relative to sender (player always carries enemy flag) */
			if (mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
			{
				clientInfo_t* senderCi;
				senderCi = &cgs.clientinfo[mark->clientNum];
				if (senderCi->infoValid)
				{
					/* Get sender's team and use enemy team color */
					if (senderCi->team == TEAM_RED)
					{
						/* Sender is RED, carries BLUE flag */
						VectorCopy(cgs.beBlueTeamColor, pulseColor);
					}
					else if (senderCi->team == TEAM_BLUE)
					{
						/* Sender is BLUE, carries RED flag */
						VectorCopy(cgs.beRedTeamColor, pulseColor);
					}
					else
					{
						Vector4Copy(color, pulseColor);
					}
				}
				else
				{
					Vector4Copy(color, pulseColor);
				}
			}
			else
			{
				Vector4Copy(color, pulseColor);
			}
			pulseColor[3] = 0.25f * endFade;
		}
		else if ((cgs.teamMarkers.teamTaskFlags & CG_TEAMTASK_UNIQUECOLORS) && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* Use unique color for pulsing circle */
			VectorCopy(UNIQUE_COLOR(mark->clientNum), pulseColor);
			pulseColor[3] = 0.25f * endFade;
		}
		else
		{
			Vector4Copy(color, pulseColor);
			pulseColor[3] = 0.25f * endFade;
		}

		/* Use same positioning logic as CG_HudBorderMarker but with baseRadius for positioning */
		hfov_y = refdef->fov_y * (float)M_PI / 360.0f;
		hfov_x = refdef->fov_x * (float)M_PI / 360.0f;

		VectorSubtract(mark->origin, refdef->vieworg, dir);
		front = DotProduct(dir, refdef->viewaxis[0]);
		left = DotProduct(dir, refdef->viewaxis[1]);
		up = DotProduct(dir, refdef->viewaxis[2]);

		flu[0] = front;
		flu[1] = left;
		flu[2] = up;

		r = VectorLength(flu);
		if (r < 0.001f)
		{
			return;
		}

		az = atan2(flu[1], flu[0]);
		inc = acos(flu[2] / r);
		inc = M_PI * 0.5 - inc;

		/* Only skip drawing if point is in front of camera AND inside FOV */
		if (front > 0.0f && fabs(az) < hfov_x && fabs(inc) < hfov_y)
		{
			return;
		}

		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		phi = atan2(-flu[2], flu[1]);
		ent.rotation = phi / M_PI * 180;
		ent.rotation += 180;

		/* Use baseRadius for positioning (to keep center aligned), but pulseScale * baseRadius for visual size */
		ent.radius = 0.5 * baseRadius * pulseScale * tan(hfov_x);
		ent.customShader = cgs.media.teamTasksCircle;

		if (ent.radius < 0.1f)
		{
			ent.radius = 0.1f;
		}

		ent.shaderRGBA[0] = pulseColor[0] * 255;
		ent.shaderRGBA[1] = pulseColor[1] * 255;
		ent.shaderRGBA[2] = pulseColor[2] * 255;
		ent.shaderRGBA[3] = pulseColor[3] * 255;

		ratio = tan(hfov_y) / tan(hfov_x);

		dist = 8;
		VectorScale(refdef->viewaxis[0], dist, middleOfPlane);

		{
			float cosHfovX = cos(hfov_x);
			float cosHfovY = cos(hfov_y);
			float baseRadiusForPos = 0.5 * baseRadius * tan(hfov_x);

			r = dist / cosHfovX;
			VectorScale(refdef->viewaxis[1], r * sin(hfov_x) - baseRadiusForPos, planeAxis[0]);

			r = dist / cosHfovY;
			VectorScale(refdef->viewaxis[2], -(r * sin(hfov_y) - baseRadiusForPos), planeAxis[1]);
		}

		circleSz = 100;
		r = cos(phi) * circleSz;
		VectorScale(planeAxis[0], r, ent.origin);

		r = sin(phi) * circleSz * 1.0 / ratio;
		VectorMA(ent.origin, r, planeAxis[1], ent.origin);

		cosPhi = cos(phi);
		sinPhi = sin(phi);

		if (fabs(cosPhi) < 0.001f)
		{
			cosPhi = (cosPhi >= 0) ? 0.001f : -0.001f;
		}
		if (fabs(sinPhi) < 0.001f)
		{
			sinPhi = (sinPhi >= 0) ? 0.001f : -0.001f;
		}

		h1 = fabs(VectorLength(planeAxis[0]) / cosPhi);
		h2 = fabs(VectorLength(planeAxis[1]) / sinPhi);
		if (h2 < h1)
		{
			h1 = h2;
		}
		if (VectorLength(ent.origin) > h1)
		{
			VectorNormalize(ent.origin);
			VectorScale(ent.origin, h1, ent.origin);
		}

		VectorAdd(middleOfPlane, ent.origin, ent.origin);
		VectorAdd(refdef->vieworg, ent.origin, ent.origin);

		trap_R_AddRefEntityToScene(&ent);
	}
}

typedef struct
{
	float distMin;
	float distMax;
	float fadeStrength;
	float fadeRadius;
	float fadeRadiusSq;
	float crosshairX;
	float crosshairY;
	qboolean useFade;
	int font;
	float scale;
	float iconMinSize;
	int teamTaskFlags;
	float opaque;
} teamTaskMarkConstants_t;

typedef struct
{
	float x;
	float y;
	float distance;
	float h;
	float w;
	float iconW;
	float iconH;
	float fade;
	float endFade;
	float nameFade;
	qboolean showSpawnEffect;
	float spawnIconW;
	float spawnIconH;
	vec4_t spawnColor;
	vec4_t color;
	qboolean isHovering;
	int font;
	float spawnFade; /* Alpha fade-in for main icon (0.0 to 1.0) */
	float spawnScale; /* Scale for main icon (0.25 to 1.0) */
} teamTaskMarkRenderData_t;

static void CG_InitTeamTaskMarkConstants(teamTaskMarkConstants_t* constants)
{
	const cgs_teamMarkers_t* tm = &cgs.teamMarkers;

	constants->distMin = 0.0f;
	constants->distMax = 2500.0f;
	constants->fadeStrength = tm->teamTaskFade;
	constants->fadeRadius = tm->teamTaskFadeRadius;
	constants->fadeRadiusSq = tm->teamTaskFadeRadius * tm->teamTaskFadeRadius;
	constants->crosshairX = SCREEN_WIDTH * 0.5f;
	constants->crosshairY = SCREEN_HEIGHT * 0.5f;
	constants->useFade = (tm->teamTaskFade > 0.0f);
	constants->font = cgFontIndices.teamTask;
	constants->scale = tm->teamTaskScale;
	constants->iconMinSize = tm->teamTaskIconMinSize;
	constants->teamTaskFlags = tm->teamTaskFlags;
	constants->opaque = tm->teamTaskOpaque;
}

/*
=================
CG_ProcessOverheadMark

Process overhead marks (YES, NO, MEDIC, HERE_ALIVE, item/pickup in first person)
Fixed position at top center of screen — 2D HUD only, no world render
=================
*/
static void CG_ProcessOverheadMark(teamTaskMark_t* mark, const teamTaskMarkConstants_t* constants,
                                   teamTaskMarkRenderData_t* renderData)
{
	/* First person view - fixed position at top center of screen */
	renderData->x = SCREEN_WIDTH * 0.5f;
	renderData->y = 20.0f; /* Fixed position from top */

	/* Fixed size for own overhead icons */
	renderData->h = 10.0f * constants->scale;
	renderData->w = renderData->h * 0.75f;
	renderData->iconW = renderData->w * 4;
	renderData->iconH = renderData->h * 4;

	/* Copy font from constants */
	renderData->font = constants->font;

	/* Fixed distance for fade calculations */
	renderData->distance = 100.0f;
}

/*
=================
CG_ProcessHereAliveMark

Process HERE_ALIVE marks - update position above player's head
=================
*/
static void CG_ProcessHereAliveMark(teamTaskMark_t* mark)
{
	int j;
	centity_t* cent;
	qboolean playerFound;

	playerFound = qfalse;

	/* Special case for local player - use predicted player entity */
	if (mark->clientNum == cg.clientNum)
	{
		cent = &cg.predictedPlayerEntity;
		if (cent->currentValid)
		{
			/* Update position above player's head */
			VectorCopy(cent->lerpOrigin, mark->origin);
			mark->origin[2] += 48.0f; /* Offset above head */
			playerFound = qtrue;
		}
		else
		{
			/* Fallback: use predicted player state origin */
			VectorCopy(cg.predictedPlayerState.origin, mark->origin);
			mark->origin[2] += 48.0f; /* Offset above head */
			playerFound = qtrue;
		}
	}
	else
	{
		/* Try to find player entity and update position */
		for (j = 0; j < MAX_GENTITIES; j++)
		{
			cent = &cg_entities[j];
			if (cent->currentState.eType == ET_PLAYER &&
			        cent->currentState.clientNum == mark->clientNum &&
			        cent->currentValid)
			{
				/* Update position above player's head */
				VectorCopy(cent->lerpOrigin, mark->origin);
				mark->origin[2] += 48.0f; /* Offset above head */
				playerFound = qtrue;
				break;
			}
		}
	}

	/* If player not found, keep the position that was set when mark was created */
	if (!playerFound)
	{
		/* Player not available, keep the position that was set when mark was created */
	}
}

/*
=================
CG_ProcessHereDeadMark

Process HERE_DEAD marks - use death coordinates or frozen entity position
=================
*/
static void CG_ProcessHereDeadMark(teamTaskMark_t* mark)
{
	int j;
	centity_t* cent;
	qboolean frozenFound;

	frozenFound = qfalse;

	/* Special case for local player - use frozen entity from cache (set/clear in CG_Player) */
	if (mark->clientNum == cg.clientNum)
	{
		if (cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze &&
		        cgs.clientinfo[cg.clientNum].isFrozenEnt)
		{
			cent = &cg_entities[cgs.clientinfo[cg.clientNum].frozenEntity];
			if (cent->currentValid)
			{
				VectorCopy(cent->lerpOrigin, mark->origin);
				mark->origin[2] += 48.0f;
				frozenFound = qtrue;
			}
		}
	}
	else
	{
		/* For other players: frozen from cache first, else find live player entity */
		if (cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze &&
		        cgs.clientinfo[mark->clientNum].isFrozenEnt)
		{
			cent = &cg_entities[cgs.clientinfo[mark->clientNum].frozenEntity];
			if (cent->currentValid)
			{
				VectorCopy(cent->lerpOrigin, mark->origin);
				mark->origin[2] += 48.0f;
				frozenFound = qtrue;
			}
		}
		if (!frozenFound)
		{
			for (j = 0; j < MAX_GENTITIES; j++)
			{
				cent = &cg_entities[j];
				if (cent->currentState.eType != ET_PLAYER || !cent->currentValid)
					continue;
				if (cent->currentState.clientNum == mark->clientNum)
				{
					VectorCopy(cent->lerpOrigin, mark->origin);
					mark->origin[2] += 48.0f;
					break;
				}
			}
		}
	}

	/* Special handling for HERE_DEAD marks in freeze mode - use snowflake for frozen bodies */
	/* WARN_DEAD always uses warning shader, not snowflake */
	if (mark->markType == TEAMTASK_HERE_DEAD && cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze)
	{
		mark->shader = cgs.media.frozenFoeTagShader;  /* Snowflake for frozen death location */
	}
}

/*
=================
CG_ProcessStandardMark

Process standard marks (OFFENSE, DEFENCE, WATCH, LOCATION, TARGET, REDARMOR, MEGAHEALTH)
Static coordinates - no position updates needed
=================
*/
static void CG_ProcessStandardMark(teamTaskMark_t* mark)
{
	/* Standard marks use static coordinates set when mark was created */
	/* No position updates needed */
}

/*
=================
CG_ProcessCommonMarkData

Process common mark data (fade, hover, spawn effects, name fade)
Called after position and size calculations
=================
*/
static void CG_ProcessCommonMarkData(teamTaskMark_t* mark, float t,
                                     const teamTaskMarkConstants_t* constants,
                                     teamTaskMarkRenderData_t* renderData,
                                     qboolean shouldDisplayAsOverhead)
{
	float dx;
	float dy;
	float distToCrosshairSq;
	float distToCrosshair;
	float coreFade;
	int timeSinceStart;
	int spawnEffectDuration;
	float spawnProgress;
	float spawnAlpha;
	const float hoverRadius = 32.0f; /* Distance in pixels to consider as "hovering" */
	const float hoverRadiusSq = hoverRadius * hoverRadius;
	const int appearanceDuration = 350;

	/* Calculate end fade effect (linear fade from 1.0 to 0.0 over TEAMTASK_MARK_FADE_TIME before endTime) */
	renderData->endFade = CG_CalculateEndFade(mark->endTime, cg.time);

	/* Calculate distance to crosshair for fade and hover detection */
	dx = renderData->x - constants->crosshairX;
	dy = renderData->y - constants->crosshairY;
	distToCrosshairSq = dx * dx + dy * dy;

	/* Initialize fade to 1.0 */
	renderData->fade = 1.0f;

	/* Calculate fade based on distance to crosshair - skip for own overhead icons */
	if (constants->useFade && !((mark->isOverhead && mark->clientNum == cg.clientNum) || shouldDisplayAsOverhead))
	{
		if (distToCrosshairSq < constants->fadeRadiusSq)
		{
			distToCrosshair = SQRTFAST(distToCrosshairSq);
			coreFade = distToCrosshair / constants->fadeRadius;
			/* Clamp coreFade to valid range */
			if (coreFade < 0.0f)
			{
				coreFade = 0.0f;
			}
			/* When distToCrosshair = 0 (on crosshair), fade should be (1.0 - fadeStrength) */
			/* When distToCrosshair = fadeRadius (at edge), fade should be 1.0 (no fade) */
			/* This makes markers fade out (become more transparent) when aiming at them */
			renderData->fade = 1.0f - ((1.0f - coreFade) * constants->fadeStrength);
			/* Clamp fade to valid range */
			if (renderData->fade < 0.0f)
			{
				renderData->fade = 0.0f;
			}
			else if (renderData->fade > 1.0f)
			{
				renderData->fade = 1.0f;
			}
		}
	}

	/* Apply end fade effect (must be applied after crosshair fade, always applies) */
	renderData->fade = renderData->fade * renderData->endFade;

	/* Calculate appearance effect (main icon fade-in and scale) */
	timeSinceStart = cg.time - mark->startTime;

	if (timeSinceStart >= 0 && timeSinceStart < appearanceDuration)
	{
		/* Calculate appearance progress (0.0 to 1.0) */
		float appearanceProgress;
		appearanceProgress = (float)timeSinceStart / (float)appearanceDuration;

		/* Main icon fade-in: alpha from 0.0 to 1.0 */
		renderData->spawnFade = appearanceProgress;

		/* Main icon scale: from 0.25 to 1.0 */
		renderData->spawnScale = 0.25f + appearanceProgress * 0.75f;
	}
	else
	{
		/* After appearance effect - full visibility and scale */
		renderData->spawnFade = 1.0f;
		renderData->spawnScale = 1.0f;
	}

	/* Calculate spawn effect (second icon) */
	spawnEffectDuration = 1000; /* 1 second */
	renderData->showSpawnEffect = (timeSinceStart >= 0 && timeSinceStart < spawnEffectDuration);

	if (renderData->showSpawnEffect)
	{
		/* Calculate spawn effect progress (0.0 to 1.0) */
		spawnProgress = (float)timeSinceStart / (float)spawnEffectDuration;

		/* Alpha for spawn effect (second icon) decreases linearly from 1.0 to 0.0 */
		spawnAlpha = 1.0f - spawnProgress;

		/* Spawn effect icon size increases from original size to 3x size (scale already applied to iconW/iconH) */
		renderData->spawnIconW = renderData->iconW * (0.8f + spawnProgress * 1.5f);
		renderData->spawnIconH = renderData->iconH * (0.8f + spawnProgress * 1.5f);

		/* Set spawn effect color with alpha */
		if ((constants->teamTaskFlags & CG_TEAMTASK_UNIQUECOLORS) && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* Use unique color for spawn effect */
			VectorCopy(UNIQUE_COLOR(mark->clientNum), renderData->spawnColor);
			renderData->spawnColor[3] = spawnAlpha * constants->opaque * renderData->fade;
		}
		else
		{
			Vector4Copy(mark->color, renderData->spawnColor);
			renderData->spawnColor[3] *= spawnAlpha * constants->opaque * renderData->fade;
		}
	}

	/* Set color with alpha (apply spawn fade for fade-in effect) */
	Vector4Copy(mark->color, renderData->color);
	renderData->color[3] *= renderData->spawnFade * constants->opaque * renderData->fade;

	/* Calculate hover state and name fade based on distance to marker */
	renderData->isHovering = (distToCrosshairSq < hoverRadiusSq && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS);

	/* Calculate name fade based on distance to crosshair - closer = more visible */
	if (mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
	{
		const float nameFadeRadiusBase = 32.0f; /* Base distance in pixels where name starts to appear */
		float nameFadeRadius;
		float nameFadeRadiusSq;

		/* Scale nameFadeRadius based on distance to marker - farther markers need smaller radius */
		/* Use same scale factor 't' that's used for text size scaling */
		nameFadeRadius = nameFadeRadiusBase * (1.0f - t * 0.25f); /* Scale from 1x to 0.5x based on distance */
		nameFadeRadiusSq = nameFadeRadius * nameFadeRadius;

		/* For own overhead icons, don't show name */
		/* HERE_DEAD shows name in world (like other marks) */
		if ((mark->isOverhead && mark->clientNum == cg.clientNum) || shouldDisplayAsOverhead)
		{
			renderData->nameFade = 0.0f;
		}
		/* Use scaled radius for name visibility - closer to marker = more visible name */
		else if (distToCrosshairSq < nameFadeRadiusSq)
		{
			distToCrosshair = SQRTFAST(distToCrosshairSq);
			/* Inverse fade: closer to center (distToCrosshair = 0) = nameFade = 1.0, at edge = nameFade = 0.0 */
			renderData->nameFade = 1.0f - (distToCrosshair / nameFadeRadius);
			if (renderData->nameFade < 0.0f)
			{
				renderData->nameFade = 0.0f;
			}
			if (renderData->nameFade > 1.0f)
			{
				renderData->nameFade = 1.0f;
			}
		}
		else
		{
			/* Outside scaled radius - name not visible */
			renderData->nameFade = 0.0f;
		}
	}
	else
	{
		/* Not a player mark - no name to show */
		renderData->nameFade = 0.0f;
	}
}

static qboolean CG_ProcessTeamTaskMark(teamTaskMark_t* mark, int markIndex,
                                       const teamTaskMarkConstants_t* constants,
                                       teamTaskMarkRenderData_t* renderData)
{
	float t;
	qboolean shouldDisplayAsOverhead;
	qboolean isOwnHereDead;
	qboolean isOutsideScreen;
	refdef_t* refdef;
	cg_t* cgP;

	refdef = &cg.refdef;
	cgP = &cg;

	/* Skip death marks if CG_TEAMTASK_ONDEATH flag is not set */
	if (mark->markType == TEAMTASK_PLAYERDEAD && !(constants->teamTaskFlags & CG_TEAMTASK_ONDEATH))
	{
		return qfalse;
	}

	/* Skip item marks that have pickup nearby (precomputed in CG_TeamTaskUpdateFrame) */
	if (cgs.teamMarkers.markHasPickupNear[markIndex])
		return qfalse;

	/* Check if this is own HERE_ALIVE, HERE_FLAG, or item (pickup) mark that should be displayed as overhead in first person (2D only, like tHere) */
	shouldDisplayAsOverhead = (mark->clientNum == cgP->clientNum &&
	                           (mark->markType == TEAMTASK_HERE_ALIVE || mark->markType == TEAMTASK_HERE_FLAG || mark->isPickup) &&
	                           !cgP->renderingThirdPerson);

	/* ============================================
	   CATEGORY 1: Overhead marks (YES, NO, MEDIC, HERE_ALIVE, item/pickup in first person) — 2D only, like tHere
	   ============================================ */
	/* WARN_ALIVE should never be displayed as overhead - always show in world */
	if ((mark->isOverhead && mark->clientNum == cgP->clientNum && !cgP->renderingThirdPerson && mark->markType != TEAMTASK_WARN_ALIVE) || shouldDisplayAsOverhead)
	{
		CG_ProcessOverheadMark(mark, constants, renderData);
		t = 0.0f; /* Fixed size for overhead marks */
	}
	/* ============================================
	   CATEGORY 2: World marks (all other marks)
	   ============================================ */
	else
	{
		/* Process mark position based on type */
		if (mark->markType == TEAMTASK_HERE_ALIVE && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* HERE_ALIVE: Update position above player's head */
			CG_ProcessHereAliveMark(mark);
		}
		else if (mark->markType == TEAMTASK_HERE_FLAG && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* HERE_FLAG: Update position above player's head (same as HERE_ALIVE, but player has flag) */
			CG_ProcessHereAliveMark(mark);
		}
		else if (mark->markType == TEAMTASK_HERE_DEAD && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* HERE_DEAD: Use death coordinates or frozen entity position */
			CG_ProcessHereDeadMark(mark);
		}
		else if (mark->markType == TEAMTASK_WARN_ALIVE && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* WARN_ALIVE: Always use static position (coordinates sent from view direction trace) */
			/* Position is where player was looking at (CONTENT_SOLID hit), no dynamic update */
			CG_ProcessStandardMark(mark);
		}
		else if (mark->markType == TEAMTASK_WARN_DEAD && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* WARN_DEAD: Use death coordinates or frozen entity position (same as HERE_DEAD) */
			CG_ProcessHereDeadMark(mark);
		}
		else if (mark->isPickup && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* Pickup marks: Update position above player's head if player is available, otherwise use static coordinates */
			CG_ProcessHereAliveMark(mark); /* Same logic as HERE_ALIVE */
		}
		else if (mark->isOverhead && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* Other overhead marks (YES, NO, MEDIC) in third person: Update position above player's head */
			CG_ProcessHereAliveMark(mark); /* Same logic as HERE_ALIVE */
		}
		else
		{
			/* Standard marks: Use static coordinates */
			CG_ProcessStandardMark(mark);
		}

		/* Calculate distance from camera to mark */
		renderData->distance = Distance(mark->origin, refdef->vieworg);

		/* Copy font from constants */
		renderData->font = constants->font;

		/* Calculate scale factor using same logic as CG_DrawPlayerIndicator */
		t = (renderData->distance - constants->distMin) / (constants->distMax - constants->distMin);
		if (t < 0.0f)
		{
			t = 0.0f;
		}
		if (t > 1.0f)
		{
			t = 1.0f;
		}

		/* Calculate text size (same as CG_DrawPlayerIndicator) */
		renderData->h = 10.0f - (t * 8.0f);
		renderData->w = renderData->h * 0.75f;

		/* Apply scale */
		renderData->h *= constants->scale;
		renderData->w *= constants->scale;

		/* Apply minimum text size limit - preserve aspect ratio (w/h = 0.75) */
		{
			float minSize;
			float scaleFactor;
			minSize = constants->iconMinSize;
			/* Check if height is below minimum, scale both dimensions proportionally to maintain aspect ratio */
			if (renderData->h < minSize)
			{
				scaleFactor = minSize / renderData->h;
				renderData->h = minSize;
				renderData->w = renderData->w * scaleFactor;
				/* After scaling, w should be minSize * 0.75, but recalculate to ensure exact ratio */
				renderData->w = renderData->h * 0.75f;
			}
		}

		/* Calculate icon size (4 times text width, but 2 times for death marks) */
		if (mark->markType == TEAMTASK_PLAYERDEAD)
		{
			renderData->iconW = renderData->w * 2;
			renderData->iconH = renderData->h * 2;
		}
		else
		{
			renderData->iconW = renderData->w * 4;
			renderData->iconH = renderData->h * 4;
		}

		/* Apply minimum icon size limit - preserve aspect ratio (iconW/iconH = w/h = 0.75) */
		{
			float minSize;
			float scaleFactor;
			minSize = constants->iconMinSize;
			/* Check if icon height is below minimum, scale both dimensions proportionally to maintain aspect ratio */
			if (renderData->iconH < minSize)
			{
				scaleFactor = minSize / renderData->iconH;
				renderData->iconH = minSize;
				renderData->iconW = renderData->iconW * scaleFactor;
				/* After scaling, iconW should be minSize * 0.75, but recalculate to ensure exact ratio */
				renderData->iconW = renderData->iconH * 0.75f;
			}
		}

		/* Convert world to screen coordinates and check if mark is outside screen */
		isOwnHereDead = (mark->clientNum == cgP->clientNum && (mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD));

		/* Convert to screen coordinates */
		isOutsideScreen = CG_IsPointOutsideScreen(mark->origin, &renderData->x, &renderData->y);

		if (isOutsideScreen)
		{
			/* For own HERE_DEAD and WARN_DEAD, always process it (will be shown in border markers) */
			/* For other marks, skip if outside screen */
			if (!isOwnHereDead)
			{
				/* Mark is outside screen - skip, border marker drawn in CG_DrawTeamTaskBorderHudMarkers */
				return qfalse;
			}
			/* For own HERE_DEAD, continue processing even if outside screen */
		}

		/* Shift marker up so that bottom of icon points to the mark position */
		renderData->y -= renderData->iconH * 0.5f;
	}

	/* ============================================
	   CATEGORY 3: Common processing (fade, hover, spawn effects, name fade)
	   ============================================ */
	CG_ProcessCommonMarkData(mark, t, constants, renderData, shouldDisplayAsOverhead);

	return qtrue;
}

static void CG_RenderTeamTaskMark(teamTaskMark_t* mark, const teamTaskMarkRenderData_t* renderData,
                                  const teamTaskMarkConstants_t* constants)
{
	vec4_t nameColor;
	vec4_t shadowColor;
	float nameY;
	clientInfo_t* ci;

	/* Draw spawn effect sprite */
	if (renderData->showSpawnEffect)
	{
		trap_R_SetColor(renderData->spawnColor);
		CG_DrawPic(renderData->x - renderData->spawnIconW * 0.5f,
		           renderData->y - renderData->spawnIconH * 0.5f,
		           renderData->spawnIconW, renderData->spawnIconH, mark->shader);
		trap_R_SetColor(NULL);
	}

	/* Draw sprite with spawn scale */
	trap_R_SetColor(renderData->color);
	CG_DrawPic(renderData->x - renderData->iconW * 0.5f * renderData->spawnScale,
	           renderData->y - renderData->iconH * 0.5f * renderData->spawnScale,
	           renderData->iconW * renderData->spawnScale,
	           renderData->iconH * renderData->spawnScale, mark->shader);
	trap_R_SetColor(NULL);

	/* Draw distance above mark when enabled */
	if ((constants->teamTaskFlags & CG_TEAMTASK_SHOWDISTANCE) && renderData->nameFade > 0.0f)
	{
		char distanceStr[16];
		vec4_t distanceColor;
		vec4_t distanceShadowColor;
		float distanceY;

		/* Format distance as integer meters */
		Com_sprintf(distanceStr, sizeof(distanceStr), "%dm", (int)renderData->distance / 10);

		/* White color with same alpha as mark, multiplied by name fade */
		Vector4Set(distanceColor, 1.0f, 1.0f, 1.0f, renderData->color[3] * renderData->nameFade);
		Vector4Set(distanceShadowColor, 0.0f, 0.0f, 0.0f, renderData->color[3] * renderData->nameFade);

		/* Position distance above mark */
		distanceY = (-2) + renderData->y - renderData->iconH * 0.5f - renderData->h * 0.5f;

		/* Draw distance */
		CG_OSPDrawStringNew(
		    renderData->x, distanceY,
		    distanceStr,
		    distanceColor,
		    distanceShadowColor,
		    renderData->w, renderData->h,
		    0, /* no max width limit */
		    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
		    renderData->font,
		    NULL, NULL, NULL
		);
	}

	/* Draw player name below mark when hovering */
	if ((constants->teamTaskFlags & CG_TEAMTASK_SHOWNAME) && renderData->nameFade > 0.0f && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[mark->clientNum];
		if (ci->infoValid && ci->name[0])
		{
			/* White color with same alpha as mark, multiplied by name fade */
			Vector4Set(nameColor, 1.0f, 1.0f, 1.0f, renderData->color[3] * renderData->nameFade);
			Vector4Set(shadowColor, 0.0f, 0.0f, 0.0f, renderData->color[3] * renderData->nameFade);
			/* Position name below mark */
			nameY = renderData->y + renderData->iconH * 0.5f + renderData->h * 0.5f;
			/* Draw player name */
			CG_OSPDrawStringNew(
			    renderData->x, nameY,
			    ci->name,
			    nameColor,
			    shadowColor,
			    renderData->w, renderData->h,
			    0, /* no max width limit */
			    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
			    renderData->font,
			    NULL, NULL, NULL
			);
		}
	}
}

void CG_DrawTeamTaskMarks(void)
{
	int i;
	teamTaskMarkConstants_t constants;
	teamTaskMarkRenderData_t renderData;
	teamTaskMark_t* mark;
	cgs_teamMarkers_t* tm;

	tm = &cgs.teamMarkers;
	if (!tm->drawTeamTasks || tm->numTeamTaskMarks == 0)
		return;

	/* Expired marks are already removed in CG_TeamTaskUpdateFrame (one place per frame). */

	/* Initialize constants */
	CG_InitTeamTaskMarkConstants(&constants);

	/* One loop: world process+render and own HERE_DEAD/WARN_DEAD/PICKUP overhead in first person */
	for (i = 0; i < tm->numTeamTaskMarks; i++)
	{
		mark = &tm->teamTaskMarks[i];

		if (!CG_ProcessTeamTaskMark(mark, i, &constants, &renderData))
			continue;
		CG_RenderTeamTaskMark(mark, &renderData, &constants);

			/* Own HERE_DEAD/WARN_DEAD as overhead in first person (pickup/item uses CATEGORY 1 overhead path above, no second draw) */
		if (!cg.renderingThirdPerson && CG_IS_LOCAL_CLIENT_ON_SCREEN() && !CG_IsClientFreeFloat() &&
		    mark->clientNum == cg.clientNum &&
		    (mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD))
		{
			teamTaskMarkRenderData_t overheadRenderData;

			overheadRenderData.x = SCREEN_WIDTH * 0.5f;
			overheadRenderData.y = 20.0f;
			overheadRenderData.h = 10.0f * constants.scale;
			overheadRenderData.w = overheadRenderData.h * 0.75f;
			overheadRenderData.iconW = overheadRenderData.w * 4;
			overheadRenderData.iconH = overheadRenderData.h * 4;
			overheadRenderData.font = constants.font;
			overheadRenderData.distance = 100.0f;
			overheadRenderData.fade = 1.0f;
			overheadRenderData.endFade = CG_CalculateEndFade(mark->endTime, cg.time);
			overheadRenderData.nameFade = 0.0f;
			overheadRenderData.showSpawnEffect = qfalse;
			overheadRenderData.spawnFade = 1.0f;
			overheadRenderData.spawnScale = 1.0f;
			overheadRenderData.isHovering = qfalse;
			Vector4Copy(mark->color, overheadRenderData.color);
			overheadRenderData.color[3] *= constants.opaque * overheadRenderData.endFade;
			CG_RenderTeamTaskMark(mark, &overheadRenderData, &constants);
		}
	}
}
