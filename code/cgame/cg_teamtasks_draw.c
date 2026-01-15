/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
			VectorCopy(cgs.be.redTeamColor, color);
		}
		else if (myRealTeam == TEAM_BLUE)
		{
			VectorCopy(cgs.be.blueTeamColor, color);
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
			VectorCopy(cgs.be.blueTeamColor, color);
		}
		else if (myRealTeam == TEAM_BLUE)
		{
			VectorCopy(cgs.be.redTeamColor, color);
		}
		else
		{
			return qfalse;
		}
	}

	/* Apply whitening (called once for all valid cases) */
	CG_ColorWhitenSmart(color, NULL, 0.6f);
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
static void CG_GetTeamTaskShaderPair(int markType, qhandle_t* markerShader, qhandle_t* worldShader)
{
	switch (markType)
	{
		case TEAMTASK_OFFENSE:
			*markerShader = cgs.media.teamTasksAttack_marker;
			*worldShader = cgs.media.teamTasksAttack;
			break;
		case TEAMTASK_DEFENCE:
			*markerShader = cgs.media.teamTasksDefence_marker;
			*worldShader = cgs.media.teamTasksDefence;
			break;
		case TEAMTASK_PATROL:
			*markerShader = cgs.media.teamTasksWatch_marker;
			*worldShader = cgs.media.teamTasksWatch;
			break;
		case TEAMTASK_PLAYERDEAD:
			*markerShader = cgs.media.teamTasksDead_marker;
			*worldShader = cgs.media.teamTasksDead;
			break;
		case TEAMTASK_YES:
			*markerShader = cgs.media.teamTasksYes;
			*worldShader = cgs.media.teamTasksYes;
			break;
		case TEAMTASK_NO:
			*markerShader = cgs.media.teamTasksNo;
			*worldShader = cgs.media.teamTasksNo;
			break;
		case TEAMTASK_LOCATION:
			*markerShader = cgs.media.teamTasksLocation_marker;
			*worldShader = cgs.media.teamTasksLocation;
			break;
		case TEAMTASK_TARGET:
			*markerShader = cgs.media.teamTasksTarget_marker;
			*worldShader = cgs.media.teamTasksTarget;
			break;
		case TEAMTASK_REDARMOR:
			*markerShader = cgs.media.teamTasksRedarmor_marker;
			*worldShader = cgs.media.teamTasksRedarmor;
			break;
		case TEAMTASK_MEGAHEALTH:
			*markerShader = cgs.media.teamTasksMegahealth_marker;
			*worldShader = cgs.media.teamTasksMegahealth;
			break;
		case TEAMTASK_MEDIC:
			*markerShader = cgs.media.teamTasksMedic_marker;
			*worldShader = cgs.media.teamTasksMedic;
			break;
		case TEAMTASK_HERE_ALIVE:
			*markerShader = cgs.media.teamTasksLocation_marker;
			*worldShader = cgs.media.teamTasksLocation;
			break;
		case TEAMTASK_HERE_FLAG:
			*markerShader = cgs.media.teamTasksFlagcarrier_marker;
			*worldShader = cgs.media.teamTasksFlagcarrier;
			break;
		case TEAMTASK_HERE_DEAD:
			/* Use frozen shader in freeze mode, otherwise use dead shader */
			if (cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze)
			{
				*markerShader = cgs.media.teamTasksFrozen_marker;
				*worldShader = cgs.media.frozenFoeTagShader;
			}
			else
			{
				*markerShader = cgs.media.teamTasksDead_marker;
				*worldShader = cgs.media.teamTasksDead;
			}
			break;
		case TEAMTASK_WARN_ALIVE:
			*markerShader = cgs.media.teamTasksWarning_marker;
			*worldShader = cgs.media.teamTasksWarning;
			break;
		case TEAMTASK_WARN_DEAD:
			/* Always use warning shader for WARN_DEAD */
			*markerShader = cgs.media.teamTasksWarning_marker;
			*worldShader = cgs.media.teamTasksWarning;
			break;
		case TEAMTASK_FROZEN_ALLY:
		case TEAMTASK_FROZEN_ENEMY:
			*markerShader = cgs.media.teamTasksFrozen_marker;
			*worldShader = cgs.media.frozenFoeTagShader;
			break;
		case TEAMTASK_QUADDAMAGE:
			*markerShader = cgs.media.teamTasksQuad_marker;
			*worldShader = cgs.media.teamTasksQuad;
			break;
		case TEAMTASK_REGENERATION:
			*markerShader = cgs.media.teamTasksRegen_marker;
			*worldShader = cgs.media.teamTasksRegen;
			break;
		case TEAMTASK_BATTLESUIT:
			*markerShader = cgs.media.teamTasksBattlesuit_marker;
			*worldShader = cgs.media.teamTasksBattlesuit;
			break;
		case TEAMTASK_HASTE:
			*markerShader = cgs.media.teamTasksHaste_marker;
			*worldShader = cgs.media.teamTasksHaste;
			break;
		case TEAMTASK_YELLOWARMOR:
			*markerShader = cgs.media.teamTasksYellowarmor_marker;
			*worldShader = cgs.media.teamTasksYellowarmor;
			break;
		case TEAMTASK_HEALTH:
			*markerShader = cgs.media.teamTasksHealth_marker;
			*worldShader = cgs.media.teamTasksHealth;
			break;
		case TEAMTASK_REDFLAG:
		case TEAMTASK_BLUEFLAG:
			*markerShader = cgs.media.teamTasksFlag_marker;
			*worldShader = cgs.media.teamTasksFlag;
			break;
		case TEAMTASK_REDFLAG_CARRIER:
		case TEAMTASK_BLUEFLAG_CARRIER:
			*markerShader = cgs.media.teamTasksFlagcarrier_marker;
			*worldShader = cgs.media.teamTasksFlagcarrier;
			break;
		default:
			*markerShader = cgs.media.teamTasksUnknown;
			*worldShader = cgs.media.teamTasksUnknown;
			break;
	}
}

void CG_GetTeamTaskMarkerShaderAndColor(int markType, qhandle_t* shader, vec4_t color, qboolean useMarkerShader, int clientNum)
{
	clientInfo_t* cl;
	qhandle_t markerShader;
	qhandle_t worldShader;

	/* Get shader pair for this mark type */
	CG_GetTeamTaskShaderPair(markType, &markerShader, &worldShader);
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
					CG_GetColorForHealth(cl->health, cl->armor, color, NULL);
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
							VectorCopy(cgs.be.blueTeamColor, color);
						}
						else if (senderCi->team == TEAM_BLUE)
						{
							/* Sender is BLUE, carries RED flag */
							VectorCopy(cgs.be.redTeamColor, color);
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
				/* World marker color is red team color */
				VectorCopy(cgs.be.redTeamColor, color);
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
				/* World marker color is blue team color */
				VectorCopy(cgs.be.blueTeamColor, color);
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
				/* World marker color is red team color */
				VectorCopy(cgs.be.redTeamColor, color);
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
				/* World marker color is blue team color */
				VectorCopy(cgs.be.blueTeamColor, color);
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
CG_RemoveExpiredTeamTaskMarks

Remove expired marks using pack array algorithm (O(n) instead of O(n²))
=================
*/
static void CG_RemoveExpiredTeamTaskMarks(void)
{
	int i;
	int writeIndex;
	teamTaskMark_t* mark;

	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return;
	}

	/* Pack array: copy valid marks to the beginning */
	writeIndex = 0;
	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		/* Keep only non-expired marks */
		if (cg.time < mark->endTime)
		{
			if (writeIndex != i)
			{
				cgs.be.teamMarkers.teamTaskMarks[writeIndex] = cgs.be.teamMarkers.teamTaskMarks[i];
			}
			writeIndex++;
		}
	}

	/* Update count */
	cgs.be.teamMarkers.numTeamTaskMarks = writeIndex;
}

/*
=================
CG_CanDrawTeamTaskMarks

Common validation checks for team task marker drawing functions
Returns qfalse if marks should not be drawn
=================
*/
static qboolean CG_CanDrawTeamTaskMarks(qboolean checkTeamTaskCvar)
{
	/* Check teamTask cvar and feature flag if requested */
	if (checkTeamTaskCvar)
	{
		if (!cg_teamTask.integer && !CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
		{
			return qfalse;
		}
	}

	/* Check gametype */
	if (cgs.gametype < GT_TEAM)
	{
		return qfalse;
	}

	/* Check if local player is spectator */
	if (CG_IsLocalClientSpectator())
	{
		return qfalse;
	}

	/* Check if there are any marks */
	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return qfalse;
	}

	return qtrue;
}

void CG_DrawTeamTaskBorderHudMarkers(void)
{
	int i;
	teamTaskMark_t* mark;
	qboolean drawHudMarkers;

	/* Common validation checks */
	if (!CG_CanDrawTeamTaskMarks(qtrue))
	{
		return;
	}

	/* Remove expired marks first (optimized pack array) */
	CG_RemoveExpiredTeamTaskMarks();

	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return;
	}

	/* Check HUD marker flag once before loop */
	drawHudMarkers = (cg_teamTask.integer & CG_TEAMTASK_HUDMARKER) ? qtrue : qfalse;

	if (!drawHudMarkers)
	{
		return;
	}

	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		/* Skip own overhead marks (YES, NO, MEDIC) - don't show in border markers */
		/* Also skip own HERE_ALIVE and HERE_FLAG marks in first person (they're displayed as overhead) */
		/* HERE_DEAD always shows in world, so don't skip it in border markers */
		if ((mark->isOverhead && mark->clientNum == cg.clientNum) ||
		        (mark->clientNum == cg.clientNum &&
		         (mark->markType == TEAMTASK_HERE_ALIVE || mark->markType == TEAMTASK_HERE_FLAG) &&
		         !cg.renderingThirdPerson))
		{
			continue;
		}

		/* Check if mark is outside screen - if yes, draw border marker */
		if (CG_IsPointOutsideScreen(mark->origin, NULL, NULL))
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
	qhandle_t shader;
	float baseRadius;
	float pulseTime;
	float pulseProgress;
	float pulseScale;
	vec4_t pulseColor;
	float endFade;

	if (!mark)
	{
		return;
	}

	CG_GetTeamTaskMarkerShaderAndColor(mark->markType, &shader, color, qtrue, mark->clientNum);

	if (!shader)
	{
		return;
	}

	distance = Distance(mark->origin, cg.predictedPlayerState.origin);

	// Check maximum distance
	if (cg_teamTaskHudMarkerMaxDist.integer > 0 && distance > cg_teamTaskHudMarkerMaxDist.value)
	{
		return;
	}

	distance = MAX(distance, 1.0f);
	hfov_x = cg.refdef.fov_x * M_PI / 360.0f;
	size = 1.0f / tan(hfov_x) * 1000.0f / distance;
	size = MIN(cg_teamTaskHudMarkerMaxScale.value, size);
	size = MAX(cg_teamTaskHudMarkerMinScale.value, size);

	baseRadius = cg_teamTaskHudMarkerSize.value * size;

	/* Calculate end fade effect (linear fade from 1.0 to 0.0 over TEAMTASK_MARK_FADE_TIME before endTime) */
	endFade = CG_CalculateEndFade(mark->endTime, cg.time);

	/* Apply fade to main marker color */
	{
		vec4_t fadedColor;
		Vector4Copy(color, fadedColor);
		fadedColor[3] *= endFade;
		CG_HudBorderMarker(mark->origin, fadedColor, baseRadius, shader, 0);
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
			VectorCopy(cgs.be.redTeamColor, pulseColor);
			pulseColor[3] = 0.25f * endFade;
		}
		else if (mark->markType == TEAMTASK_BLUEFLAG || mark->markType == TEAMTASK_BLUEFLAG_CARRIER)
		{
			/* For blue flag and carrier, use blue team color for pulsing circle */
			VectorCopy(cgs.be.blueTeamColor, pulseColor);
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
						VectorCopy(cgs.be.blueTeamColor, pulseColor);
					}
					else if (senderCi->team == TEAM_BLUE)
					{
						/* Sender is BLUE, carries RED flag */
						VectorCopy(cgs.be.redTeamColor, pulseColor);
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
		else if ((cg_teamTask.integer & CG_TEAMTASK_UNIQUECOLORS) && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
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
		hfov_y = cg.refdef.fov_y * M_PI / 360.0f;
		hfov_x = cg.refdef.fov_x * M_PI / 360.0f;

		VectorSubtract(mark->origin, cg.refdef.vieworg, dir);
		front = DotProduct(dir, cg.refdef.viewaxis[0]);
		left = DotProduct(dir, cg.refdef.viewaxis[1]);
		up = DotProduct(dir, cg.refdef.viewaxis[2]);

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
		inc = M_PI / 2.0 - inc;

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
		VectorScale(cg.refdef.viewaxis[0], dist, middleOfPlane);

		{
			float cosHfovX = cos(hfov_x);
			float cosHfovY = cos(hfov_y);
			float baseRadiusForPos = 0.5 * baseRadius * tan(hfov_x);

			r = dist / cosHfovX;
			VectorScale(cg.refdef.viewaxis[1], r * sin(hfov_x) - baseRadiusForPos, planeAxis[0]);

			r = dist / cosHfovY;
			VectorScale(cg.refdef.viewaxis[2], -(r * sin(hfov_y) - baseRadiusForPos), planeAxis[1]);
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
		VectorAdd(cg.refdef.vieworg, ent.origin, ent.origin);

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
	constants->distMin = 0.0f;
	constants->distMax = 2500.0f;
	constants->fadeStrength = cg_teamTaskFade.value; // 0.0 = no fade, 1.0 = full fade
	constants->fadeRadius = cg_teamTaskFadeRadius.value;
	constants->fadeRadiusSq = constants->fadeRadius * constants->fadeRadius;
	constants->crosshairX = SCREEN_WIDTH * 0.5f;
	constants->crosshairY = SCREEN_HEIGHT * 0.5f;
	constants->useFade = (constants->fadeStrength > 0.0f);
	constants->font = CG_FontIndexFromCvar(&cg_teamTaskFont);
}

/*
=================
CG_ProcessOverheadMark

Process overhead marks (YES, NO, MEDIC, HERE_ALIVE in first person)
Fixed position at top center of screen
=================
*/
static void CG_ProcessOverheadMark(teamTaskMark_t* mark, const teamTaskMarkConstants_t* constants,
                                   teamTaskMarkRenderData_t* renderData)
{
	/* First person view - fixed position at top center of screen */
	renderData->x = SCREEN_WIDTH * 0.5f;
	renderData->y = 20.0f; /* Fixed position from top */

	/* Fixed size for own overhead icons */
	renderData->h = 10.0f * cg_teamTaskScale.value;
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
	qboolean playerFound;
	qboolean isFollowing;
	qboolean isFreeFloat;

	playerFound = qfalse;
	isFollowing = CG_IsFollowing();
	isFreeFloat = CG_IsClientFreeFloat();

	/* Special case for local player */
	if (mark->clientNum == cg.clientNum)
	{
		if (isFollowing || isFreeFloat)
		{
			/* When following another player, use death coordinates for own HERE_DEAD */
			/* Don't update position - keep original death position */
			playerFound = qfalse; /* Will be handled below */
		}
		else
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

	/* If player not found, check freeze mode */
	if (!playerFound)
	{
		/* Try to find frozen entity in freeze mode */
		if (cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze)
		{
			qboolean frozenFound;

			frozenFound = qfalse;

			/* Search for frozen entity in range MAX_CLIENTS..MAX_GENTITIES */
			for (j = MAX_CLIENTS; j < MAX_GENTITIES; j++)
			{
				cent = &cg_entities[j];
				if (cent->currentState.eType == ET_PLAYER && cent->currentValid)
				{
					/* Check if this is a frozen entity */
					if (CG_IsFrozenEntity(cent))
					{
						int frozenClientNum;
						frozenClientNum = cent->currentState.otherEntityNum;
						if (frozenClientNum == mark->clientNum)
						{
							/* Found frozen entity for this client - use its position above head */
							VectorCopy(cent->lerpOrigin, mark->origin);
							mark->origin[2] += 48.0f; /* Offset above head */
							frozenFound = qtrue;
							break;
						}
					}
				}
			}

			/* If frozen entity not found, keep original death position */
			if (!frozenFound)
			{
				/* Frozen entity not available, keep the death position that was set when mark was created */
			}
		}
		else
		{
			/* Not freeze mode, keep the death position that was set when mark was created */
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
		if ((cg_teamTask.integer & CG_TEAMTASK_UNIQUECOLORS) && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
		{
			/* Use unique color for spawn effect */
			VectorCopy(UNIQUE_COLOR(mark->clientNum), renderData->spawnColor);
			renderData->spawnColor[3] = spawnAlpha * cg_teamTaskOpaque.value * renderData->fade;
		}
		else
		{
			Vector4Copy(mark->color, renderData->spawnColor);
			renderData->spawnColor[3] *= spawnAlpha * cg_teamTaskOpaque.value * renderData->fade;
		}
	}

	/* Set color with alpha (apply spawn fade for fade-in effect) */
	Vector4Copy(mark->color, renderData->color);
	renderData->color[3] *= renderData->spawnFade * cg_teamTaskOpaque.value * renderData->fade;

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

	/* Skip death marks if CG_TEAMTASK_ONDEATH flag is not set */
	if (mark->markType == TEAMTASK_PLAYERDEAD && !(cg_teamTask.integer & CG_TEAMTASK_ONDEATH))
	{
		return qfalse;
	}

	/* Check if this is own HERE_ALIVE or HERE_FLAG mark that should be displayed as overhead in first person */
	shouldDisplayAsOverhead = (mark->clientNum == cg.clientNum &&
	                           (mark->markType == TEAMTASK_HERE_ALIVE || mark->markType == TEAMTASK_HERE_FLAG) &&
	                           !cg.renderingThirdPerson);

	/* ============================================
	   CATEGORY 1: Overhead marks (YES, NO, MEDIC, HERE_ALIVE in first person)
	   ============================================ */
	/* WARN_ALIVE should never be displayed as overhead - always show in world */
	if ((mark->isOverhead && mark->clientNum == cg.clientNum && !cg.renderingThirdPerson && mark->markType != TEAMTASK_WARN_ALIVE) || shouldDisplayAsOverhead)
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
		renderData->distance = Distance(mark->origin, cg.refdef.vieworg);

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
		renderData->h *= cg_teamTaskScale.value;
		renderData->w *= cg_teamTaskScale.value;

		/* Apply minimum text size limit */
		if (renderData->w < cg_teamTaskIconMinSize.value)
			renderData->w = cg_teamTaskIconMinSize.value * 0.75;
		if (renderData->h < cg_teamTaskIconMinSize.value)
			renderData->h = cg_teamTaskIconMinSize.value;

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

		/* Apply minimum icon size limit */
		if (renderData->iconW < cg_teamTaskIconMinSize.value)
			renderData->iconW = cg_teamTaskIconMinSize.value * 0.75f;
		if (renderData->iconH < cg_teamTaskIconMinSize.value)
			renderData->iconH = cg_teamTaskIconMinSize.value;

		/* Convert world to screen coordinates and check if mark is outside screen */
		isOwnHereDead = (mark->clientNum == cg.clientNum && (mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD));

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

static void CG_RenderTeamTaskMark(teamTaskMark_t* mark, const teamTaskMarkRenderData_t* renderData)
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
	if ((cg_teamTask.integer & CG_TEAMTASK_SHOWDISTANCE) && renderData->nameFade > 0.0f)
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
		CG_FontSelect(renderData->font);

		/* Draw distance */
		CG_OSPDrawStringNew(
		    renderData->x, distanceY,
		    distanceStr,
		    distanceColor,
		    distanceShadowColor,
		    renderData->w, renderData->h,
		    0, /* no max width limit */
		    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
		    NULL, NULL, NULL
		);
	}

	/* Draw player name below mark when hovering */
	if ((cg_teamTask.integer & CG_TEAMTASK_SHOWNAME) && renderData->nameFade > 0.0f && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[mark->clientNum];
		if (ci->infoValid && ci->name[0])
		{
			/* White color with same alpha as mark, multiplied by name fade */
			Vector4Set(nameColor, 1.0f, 1.0f, 1.0f, renderData->color[3] * renderData->nameFade);
			Vector4Set(shadowColor, 0.0f, 0.0f, 0.0f, renderData->color[3] * renderData->nameFade);
			/* Position name below mark */
			nameY = renderData->y + renderData->iconH * 0.5f + renderData->h * 0.5f;
			CG_FontSelect(renderData->font);
			/* Draw player name */
			CG_OSPDrawStringNew(
			    renderData->x, nameY,
			    ci->name,
			    nameColor,
			    shadowColor,
			    renderData->w, renderData->h,
			    0, /* no max width limit */
			    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
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

	/* Common validation checks (don't check cvar here, it's already checked in CG_DrawTeamTaskBorderHudMarkers) */
	if (!CG_CanDrawTeamTaskMarks(qfalse))
	{
		return;
	}

	/* Note: expired marks are already removed in CG_DrawTeamTaskBorderHudMarkers */
	/* (which is called before this function) */

	/* Initialize constants */
	CG_InitTeamTaskMarkConstants(&constants);

	/* Process and render each mark */
	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		/* Process mark (calculate positions, fade, hover state) */
		if (!CG_ProcessTeamTaskMark(mark, i, &constants, &renderData))
		{
			/* Mark is outside screen, skip rendering */
			continue;
		}

		/* Render mark (draw sprite, spawn effect, player name) */
		CG_RenderTeamTaskMark(mark, &renderData);
	}

	/* Draw own HERE_DEAD marks as overhead in first person (in addition to world rendering) */
	/* Only show overhead if we're observing ourselves (not following another player) */
	/* Don't show overhead in free float mode (already shown in HUD marker) */
	if (!cg.renderingThirdPerson && CG_IsLocalClientOnScreen() && !CG_IsClientFreeFloat())
	{
		for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
		{
			mark = &cgs.be.teamMarkers.teamTaskMarks[i];

			/* Check if this is own HERE_DEAD or WARN_DEAD mark */
			if (mark->clientNum == cg.clientNum && (mark->markType == TEAMTASK_HERE_DEAD || mark->markType == TEAMTASK_WARN_DEAD))
			{
				teamTaskMarkRenderData_t overheadRenderData;

				/* Set up overhead rendering data */
				overheadRenderData.x = SCREEN_WIDTH * 0.5f;
				overheadRenderData.y = 20.0f; /* Fixed position from top */
				overheadRenderData.h = 10.0f * cg_teamTaskScale.value;
				overheadRenderData.w = overheadRenderData.h * 0.75f;
				overheadRenderData.iconW = overheadRenderData.w * 4;
				overheadRenderData.iconH = overheadRenderData.h * 4;
				overheadRenderData.font = constants.font;
				overheadRenderData.distance = 100.0f;
				overheadRenderData.fade = 1.0f;
				overheadRenderData.endFade = CG_CalculateEndFade(mark->endTime, cg.time);
				overheadRenderData.nameFade = 0.0f; /* Don't show name for overhead */
				overheadRenderData.showSpawnEffect = qfalse; /* No spawn effect for overhead */
				overheadRenderData.spawnFade = 1.0f;
				overheadRenderData.spawnScale = 1.0f;
				overheadRenderData.isHovering = qfalse;

				/* Set color with end fade */
				Vector4Copy(mark->color, overheadRenderData.color);
				overheadRenderData.color[3] *= cg_teamTaskOpaque.value * overheadRenderData.endFade;

				/* Render overhead mark */
				CG_RenderTeamTaskMark(mark, &overheadRenderData);
			}
		}
	}
}
