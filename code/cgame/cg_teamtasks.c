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
// cg_teamtasks.c -- team task marks rendering

#include "cg_local.h"

#define TEAMTASK_MARK_FADE_TIME 800 /* Fade duration in milliseconds (0.8 seconds) */

/*
=================
CG_HasActiveFrozenAllyMark

Check if there is an active TEAMTASK_FROZEN_ALLY mark near the frozen entity
Returns qtrue if active mark found, qfalse otherwise
=================
*/
qboolean CG_HasActiveFrozenAllyMark(centity_t* frozenCent)
{
	int i;
	teamTaskMark_t* mark;
	float dist;
	float searchRadius;
	int frozenClientNum;
	clientInfo_t* frozenCi;
	
	if (!frozenCent || !CG_IsFrozenEntity(frozenCent))
	{
		return qfalse;
	}
	
	frozenClientNum = frozenCent->currentState.otherEntityNum;
	if (frozenClientNum < 0 || frozenClientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}
	
	frozenCi = &cgs.clientinfo[frozenClientNum];
	if (!frozenCi->infoValid || CG_IsEnemy(frozenCi))
	{
		/* Not an ally */
		return qfalse;
	}
	
	searchRadius = 64.0f; /* Search radius for finding mark near frozen entity */
	
	/* Check all active marks */
	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];
		
		/* Check if mark is active and of type TEAMTASK_FROZEN_ALLY */
		if (mark->markType == TEAMTASK_FROZEN_ALLY && cg.time < mark->endTime)
		{
			/* Check if mark is near frozen entity */
			dist = Distance(mark->origin, frozenCent->lerpOrigin);
			if (dist <= searchRadius)
			{
				return qtrue;
			}
		}
	}
	
	return qfalse;
}

/*
=================
TeamTask command definitions
=================
*/

/* Global teamtask commands array */
static teamTaskCommand_t teamTaskCommands[] = {
	{ "attack", NULL, TEAMTASK_OFFENSE, qfalse, qtrue, CG_TeamTaskMarkOffense, "tattack" },
	{ "defence", NULL, TEAMTASK_DEFENCE, qfalse, qtrue, CG_TeamTaskMarkDefence, "tdefence" },
	{ "watch", NULL, TEAMTASK_PATROL, qfalse, qtrue, CG_TeamTaskMarkWatch, "twatch" },
	{ "yes", NULL, TEAMTASK_YES, qtrue, qfalse, CG_TeamTaskMarkYes, "tyes" },
	{ "no", NULL, TEAMTASK_NO, qtrue, qfalse, CG_TeamTaskMarkNo, "tno" },
	{ "location", "position", TEAMTASK_LOCATION, qfalse, qtrue, CG_TeamTaskMarkLocation, "tlocation" },
	{ "target", NULL, TEAMTASK_TARGET, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "redarmor", NULL, TEAMTASK_REDARMOR, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "megahealth", NULL, TEAMTASK_MEGAHEALTH, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "medic", NULL, TEAMTASK_MEDIC, qtrue, qfalse, CG_TeamTaskMarkMedic, "tmedic" }
};

#define NUM_TEAMTASK_COMMANDS (sizeof(teamTaskCommands) / sizeof(teamTaskCommands[0]))

/*
=================
CG_GetTeamTaskShaderByType

Get shader handle for teamtask type
Returns 0 if not found or not loaded
=================
*/
static qhandle_t CG_GetTeamTaskShaderByType(teamtask_t taskType)
{
	switch (taskType)
	{
		case TEAMTASK_OFFENSE:
			return cgs.media.teamTasksAttack;
		case TEAMTASK_DEFENCE:
			return cgs.media.teamTasksDefence;
		case TEAMTASK_PATROL:
			return cgs.media.teamTasksWatch;
		case TEAMTASK_YES:
			return cgs.media.teamTasksYes;
		case TEAMTASK_NO:
			return cgs.media.teamTasksNo;
		case TEAMTASK_LOCATION:
			return cgs.media.teamTasksLocation;
		case TEAMTASK_MEDIC:
			return cgs.media.teamTasksMedic;
		default:
			return 0;
	}
}

/*
=================
CG_GetTeamTaskCommandByName

Get teamtask command by name (checks both name and altName)
Returns NULL if not found
=================
*/
teamTaskCommand_t* CG_GetTeamTaskCommandByName(const char* name)
{
	int i;
	
	if (!name || name[0] == '\0')
	{
		return NULL;
	}
	
	for (i = 0; i < NUM_TEAMTASK_COMMANDS; i++)
	{
		if (Q_stricmp(teamTaskCommands[i].name, name) == 0)
		{
			return &teamTaskCommands[i];
		}
		if (teamTaskCommands[i].altName && Q_stricmp(teamTaskCommands[i].altName, name) == 0)
		{
			return &teamTaskCommands[i];
		}
	}
	
	return NULL;
}

/*
=================
CG_GetTeamTaskCommandByType

Get teamtask command by type
Returns NULL if not found
=================
*/
static teamTaskCommand_t* CG_GetTeamTaskCommandByType(teamtask_t taskType)
{
	int i;
	
	for (i = 0; i < NUM_TEAMTASK_COMMANDS; i++)
	{
		if (teamTaskCommands[i].taskType == taskType)
		{
			return &teamTaskCommands[i];
		}
	}
	
	return NULL;
}

/*
=================
CG_IsTeamTaskOverhead

Check if teamtask type is overhead command
=================
*/
qboolean CG_IsTeamTaskOverhead(teamtask_t taskType)
{
	teamTaskCommand_t* cmd;
	
	cmd = CG_GetTeamTaskCommandByType(taskType);
	if (cmd)
	{
		return cmd->isOverhead;
	}
	
	return qfalse;
}

// Token Bucket functions for anti-flood
void TokenBucket_Init(tokenBucket_t* b, int maxTokens, int refillTime, int now)
{
	b->maxTokens = maxTokens;
	b->refillTime = refillTime;
	b->tokens = maxTokens;
	b->lastTime = now;
}

static void TokenBucket_Update(tokenBucket_t* b, int now)
{
	int delta;
	int add;

	delta = now - b->lastTime;
	if (delta <= 0)
	{
		return;
	}

	add = delta / b->refillTime;
	if (add <= 0)
	{
		return;
	}

	b->tokens += add;
	if (b->tokens > b->maxTokens)
	{
		b->tokens = b->maxTokens;
	}

	b->lastTime += add * b->refillTime;
}

qboolean TokenBucket_Consume(tokenBucket_t* b, int now)
{
	TokenBucket_Update(b, now);

	if (b->tokens <= 0)
	{
		return qfalse;
	}

	b->tokens--;
	return qtrue;
}

int TokenBucket_TimeToNext(tokenBucket_t* b, int now)
{
	int timeRemaining;

	/* Update bucket first to get accurate time */
	TokenBucket_Update(b, now);

	if (b->tokens > 0)
	{
		return 0;
	}

	timeRemaining = b->refillTime - (now - b->lastTime);
	if (timeRemaining < 0)
	{
		return 0;
	}

	return timeRemaining;
}

void CG_GetTeamTaskMarkerShaderAndColor(int markType, qhandle_t* shader, vec4_t color, qboolean useMarkerShader, int clientNum)
{
	clientInfo_t* cl;
	
	switch (markType)
	{
		case TEAMTASK_OFFENSE:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksAttack_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksAttack;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_DEFENCE:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksDefence_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksDefence;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_PATROL: /* TEAMTASK_PATROL is used for watch */
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksWatch_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksWatch;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_PLAYERDEAD:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksDead_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksDead;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_YES:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksYes;
			}
			else
			{
				*shader = cgs.media.teamTasksYes;
			}
			color[0] = 0.7f;
			color[1] = 1.0f;
			color[2] = 0.7f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_NO:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksNo;
			}
			else
			{
				*shader = cgs.media.teamTasksNo;
			}
			color[0] = 1.0f;
			color[1] = 0.7f;
			color[2] = 0.7f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_LOCATION:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksLocation_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksLocation;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_TARGET:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksTarget_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksTarget;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_REDARMOR:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksRedarmor_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksRedarmor;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_MEGAHEALTH:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksMegahealth_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksMegahealth;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
		case TEAMTASK_MEDIC:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksMedic_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksMedic;
			}
			/* Default white color */
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
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
				*shader = cgs.media.teamTasksFrozen_marker;
				/* HUD marker color is white */
				color[0] = 1.0f;
				color[1] = 1.0f;
				color[2] = 1.0f;
				color[3] = 1.0f;
			}
			else
			{
				*shader = cgs.media.frozenFoeTagShader;
				/* World marker color is own team color */
				{
					int myTeam;
					myTeam = cg.snap->ps.persistant[PERS_TEAM];
					if (myTeam == TEAM_RED)
					{
						VectorCopy(cgs.be.redTeamColor, color);
					}
					else if (myTeam == TEAM_BLUE)
					{
						VectorCopy(cgs.be.blueTeamColor, color);
					}
					else
					{
						color[0] = 1.0f;
						color[1] = 1.0f;
						color[2] = 1.0f;
					}
					color[3] = 1.0f;
				}
			}
			break;
		case TEAMTASK_FROZEN_ENEMY:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksFrozen_marker;
				/* HUD marker color is white */
				color[0] = 1.0f;
				color[1] = 1.0f;
				color[2] = 1.0f;
				color[3] = 1.0f;
			}
			else
			{
				*shader = cgs.media.frozenFoeTagShader;
				/* World marker color is enemy team color */
				{
					int myTeam;
					myTeam = cg.snap->ps.persistant[PERS_TEAM];
					if (myTeam == TEAM_RED)
					{
						VectorCopy(cgs.be.blueTeamColor, color);
					}
					else if (myTeam == TEAM_BLUE)
					{
						VectorCopy(cgs.be.redTeamColor, color);
					}
					else
					{
						color[0] = 1.0f;
						color[1] = 1.0f;
						color[2] = 1.0f;
					}
					color[3] = 1.0f;
				}
			}
			break;
		default:
			if (useMarkerShader)
			{
				*shader = cgs.media.teamTasksDefence_marker;
			}
			else
			{
				*shader = cgs.media.teamTasksDefence;
			}
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
			break;
	}
}

void CG_DrawTeamTaskBorderHudMarkers(void)
{
	int i;
	float x;
	float y;
	teamTaskMark_t* mark;

	if (!cg_teamTask.integer && !CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}

	if (cgs.gametype < GT_TEAM)
	{
		return;
	}

	/* Check if local player is spectator */
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return;
	}

	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		/* Skip expired marks */
		if (cg.time >= mark->endTime)
		{
			continue;
		}

		/* Skip own overhead marks (YES, NO, MEDIC) - don't show in border markers */
		if (mark->isOverhead && mark->clientNum == cg.clientNum)
		{
			continue;
		}

		/* Check if mark is outside screen - if yes, draw border marker */
		if (CG_IsPointOutsideScreen(mark->origin, NULL, NULL))
		{
			if (cg_teamTask.integer & CG_TEAMTASK_HUDMARKER)
			{
				CG_DrawTeamTaskBorderMarker(mark);
			}
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
	int timeUntilEnd;
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
	timeUntilEnd = mark->endTime - cg.time;
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
		if (mark->markType == TEAMTASK_FROZEN_ALLY || mark->markType == TEAMTASK_FROZEN_ENEMY)
		{
			/* For frozen bodies, use team color for pulsing circle */
			int myTeam;
			myTeam = cg.snap->ps.persistant[PERS_TEAM];
			if (mark->markType == TEAMTASK_FROZEN_ALLY)
			{
				/* Use own team color */
				if (myTeam == TEAM_RED)
				{
					VectorCopy(cgs.be.redTeamColor, pulseColor);
				}
				else if (myTeam == TEAM_BLUE)
				{
					VectorCopy(cgs.be.blueTeamColor, pulseColor);
				}
				else
				{
					Vector4Copy(color, pulseColor);
				}
			}
			else /* TEAMTASK_FROZEN_ENEMY */
			{
				/* Use enemy team color */
				if (myTeam == TEAM_RED)
				{
					VectorCopy(cgs.be.blueTeamColor, pulseColor);
				}
				else if (myTeam == TEAM_BLUE)
				{
					VectorCopy(cgs.be.redTeamColor, pulseColor);
				}
				else
				{
					Vector4Copy(color, pulseColor);
				}
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

static qboolean CG_ProcessTeamTaskMark(teamTaskMark_t* mark, int markIndex, 
                                       const teamTaskMarkConstants_t* constants,
                                       teamTaskMarkRenderData_t* renderData)
{
	float t;
	int timeUntilEnd;
	int j;
	centity_t* cent;
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
	
	/* Skip death marks if CG_TEAMTASK_ONDEATH flag is not set */
	if (mark->markType == TEAMTASK_PLAYERDEAD && !(cg_teamTask.integer & CG_TEAMTASK_ONDEATH))
	{
		return qfalse;
	}
	
	/* Special handling for own overhead marks (YES, NO, MEDIC) - fix at top of screen */
	if (mark->isOverhead && mark->clientNum == cg.clientNum)
	{
		/* Fixed position at top center of screen */
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
		
		/* Set t to 0 for fixed size calculations */
		t = 0.0f;
	}
	else
	{
		/* Update position dynamically if this is an overhead mark (YES/NO) and player is available */
		if (mark->isOverhead && mark->clientNum >= 0 && mark->clientNum < MAX_CLIENTS)
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
					break;
				}
			}
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

		/* Convert world to screen coordinates and check if mark is outside screen */
		if (CG_IsPointOutsideScreen(mark->origin, &renderData->x, &renderData->y))
		{
			/* Mark is outside screen - skip, border marker drawn in CG_DrawTeamTaskBorderHudMarkers */
			return qfalse;
		}
		
		/* Shift marker up so that bottom of icon points to the mark position */
		renderData->y -= renderData->iconH * 0.5f;
	}

	/* Calculate end fade effect (linear fade from 1.0 to 0.0 over TEAMTASK_MARK_FADE_TIME before endTime) */
	timeUntilEnd = mark->endTime - cg.time;
	if (timeUntilEnd <= TEAMTASK_MARK_FADE_TIME)
	{
		renderData->endFade = (float)timeUntilEnd / (float)TEAMTASK_MARK_FADE_TIME;
		if (renderData->endFade < 0.0f)
		{
			renderData->endFade = 0.0f;
		}
	}
	else
	{
		renderData->endFade = 1.0f;
	}

	/* Calculate distance to crosshair for fade and hover detection */
	dx = renderData->x - constants->crosshairX;
	dy = renderData->y - constants->crosshairY;
	distToCrosshairSq = dx * dx + dy * dy;

	/* Initialize fade to 1.0 */
	renderData->fade = 1.0f;

	/* Calculate fade based on distance to crosshair - skip for own overhead icons */
	if (constants->useFade && !(mark->isOverhead && mark->clientNum == cg.clientNum))
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
		float appearanceProgress = (float)timeSinceStart / (float)appearanceDuration;
		
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
		if (mark->isOverhead && mark->clientNum == cg.clientNum)
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
	int j;
	teamTaskMarkConstants_t constants;
	teamTaskMarkRenderData_t renderData;
	teamTaskMark_t* mark;

	/* Check conditions */
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}

	if (CG_IsLocalClientSpectator())
	{
		return;
	}

	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return;
	}

	/* Initialize constants */
	CG_InitTeamTaskMarkConstants(&constants);

	/* Process and render each mark */
	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		/* Remove expired marks */
		if (cg.time >= mark->endTime)
		{
			for (j = i; j < cgs.be.teamMarkers.numTeamTaskMarks - 1; j++)
			{
				cgs.be.teamMarkers.teamTaskMarks[j] = cgs.be.teamMarkers.teamTaskMarks[j + 1];
			}
			cgs.be.teamMarkers.numTeamTaskMarks--;
			i--;
			continue;
		}

		/* Process mark (calculate positions, fade, hover state) */
		if (!CG_ProcessTeamTaskMark(mark, i, &constants, &renderData))
		{
			/* Mark is outside screen, skip rendering */
			continue;
		}

		/* Render mark (draw sprite, spawn effect, player name) */
		CG_RenderTeamTaskMark(mark, &renderData);
	}
}


/*
=================
CG_UpdateSelwheelMark

Update mark position based on mouse movement from usercmd_t
=================
*/
void CG_UpdateSelwheelMark(void)
{
	int currentCmdNum;
	usercmd_t currentCmd;
	float sensitivity;
	float pitchDelta;
	float yawDelta;
	qboolean hasMovement;
	int timeSinceMovement;
	float centerX;
	float centerY;
	float returnSpeed;
	
	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}
	
	currentCmdNum = trap_GetCurrentCmdNumber();
	
	// Get current command
	if (!trap_GetUserCmd(currentCmdNum, &currentCmd))
	{
		return;
	}
	
	// ============================================
	// STEP 1: Check for mouse movement FIRST
	// This must be checked BEFORE any center return logic
	// ============================================
	hasMovement = qfalse;
	
	// Compare current angles with saved angles
	if (currentCmd.angles[PITCH] != cgs.be.teamMarkers.lastCmdAngles[PITCH] || 
	    currentCmd.angles[YAW] != cgs.be.teamMarkers.lastCmdAngles[YAW])
	{
		hasMovement = qtrue;
		
		// ============================================
		// STEP 2: Movement detected - update mark position
		// ============================================
		// Calculate angle differences (angles are in 16-bit short format)
		pitchDelta = (float)(SHORT2ANGLE(currentCmd.angles[PITCH]) - SHORT2ANGLE(cgs.be.teamMarkers.lastCmdAngles[PITCH]));
		yawDelta = (float)(SHORT2ANGLE(currentCmd.angles[YAW]) - SHORT2ANGLE(cgs.be.teamMarkers.lastCmdAngles[YAW]));
		
		// Normalize angles to -180..180 range
		if (pitchDelta > 180.0f)
		{
			pitchDelta -= 360.0f;
		}
		else if (pitchDelta < -180.0f)
		{
			pitchDelta += 360.0f;
		}
		
		if (yawDelta > 180.0f)
		{
			yawDelta -= 360.0f;
		}
		else if (yawDelta < -180.0f)
		{
			yawDelta += 360.0f;
		}
		
		// Update position based on movement
		sensitivity = cg_teamTaskWheelSensitivity.value; // Use cvar to control mark movement speed
		cgs.be.teamMarkers.ttwheelMarkX -= yawDelta * sensitivity; // Inverted: right mouse = right mark
		cgs.be.teamMarkers.ttwheelMarkY += pitchDelta * sensitivity;
		
		// Clamp to circular boundary (radius slightly larger than icon position)
		{
			float maxRadius;
			float iconRadius;
			float dx;
			float dy;
			float distFromCenter;
			float angle;
			
			centerX = SCREEN_WIDTH / 2.0f;
			centerY = SCREEN_HEIGHT / 2.0f;
			iconRadius = 20.0f; // Distance from center to icons (in 640x480 coordinates, reduced by 1/3)
			maxRadius = iconRadius + 10.0f; // Slightly larger than icon position (increased by 10 pixels)
			
			// Calculate distance from center (all in 640x480 coordinates)
			dx = cgs.be.teamMarkers.ttwheelMarkX - centerX;
			dy = cgs.be.teamMarkers.ttwheelMarkY - centerY;
			distFromCenter = sqrt(dx * dx + dy * dy);
			
			// If cursor is outside max radius, clamp it to the circle boundary
			if (distFromCenter > maxRadius)
			{
				// Calculate angle from center
				angle = atan2(dy, dx);
				
				// Set position to max radius at this angle
				cgs.be.teamMarkers.ttwheelMarkX = centerX + cos(angle) * maxRadius;
				cgs.be.teamMarkers.ttwheelMarkY = centerY + sin(angle) * maxRadius;
			}
		}
		
		// Update saved angles immediately - ONLY when movement detected
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
		cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];
		
		// Reset movement timer - movement detected
		cgs.be.teamMarkers.lastMovementTime = cg.time;
		
		// Update last command number and RETURN - movement detected, no center return
		cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
		return;
	}
	
	// ============================================
	// STEP 3: No movement detected - check for return to center
	// ============================================
	timeSinceMovement = cg.time - cgs.be.teamMarkers.lastMovementTime;
	
	// Return to center if no movement for 0.5 seconds (500ms)
	if (timeSinceMovement > 500)
	{
		float maxRadius;
		float iconRadius;
		float dx;
		float dy;
		float distFromCenter;
		float angle;
		
		centerX = SCREEN_WIDTH / 2.0f;
		centerY = SCREEN_HEIGHT / 2.0f;
		iconRadius = 20.0f; // Distance from center to icons (in 640x480 coordinates, reduced by 1/3)
		maxRadius = iconRadius + 5; // Slightly larger than icon position (increased by 10 pixels)
		
		// Smooth return to center (all in 640x480 coordinates)
		returnSpeed = 0.15f; // Adjust this value to control return speed (0.0 = instant, 1.0 = very slow)
		
		cgs.be.teamMarkers.ttwheelMarkX += (centerX - cgs.be.teamMarkers.ttwheelMarkX) * returnSpeed;
		cgs.be.teamMarkers.ttwheelMarkY += (centerY - cgs.be.teamMarkers.ttwheelMarkY) * returnSpeed;
		
		// Clamp to circular boundary after return movement
		dx = cgs.be.teamMarkers.ttwheelMarkX - centerX;
		dy = cgs.be.teamMarkers.ttwheelMarkY - centerY;
		distFromCenter = sqrt(dx * dx + dy * dy);
		
		if (distFromCenter > maxRadius)
		{
			// Calculate angle from center
			angle = atan2(dy, dx);
			
			// Set position to max radius at this angle
			cgs.be.teamMarkers.ttwheelMarkX = centerX + cos(angle) * maxRadius;
			cgs.be.teamMarkers.ttwheelMarkY = centerY + sin(angle) * maxRadius;
		}
		
		// If very close to center, snap to exact center
		if (fabs(cgs.be.teamMarkers.ttwheelMarkX - centerX) < 1.0f && fabs(cgs.be.teamMarkers.ttwheelMarkY - centerY) < 1.0f)
		{
			cgs.be.teamMarkers.ttwheelMarkX = centerX;
			cgs.be.teamMarkers.ttwheelMarkY = centerY;
		}
	}
	
	// Update last command number for next frame
	cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
}

/*
=================
CG_DrawSelwheelMark

Draw the ttwheel mark on screen
=================
*/
void CG_DrawSelwheelMark(void)
{
	float x;
	float y;
	float size;
	qhandle_t shader;
	vec4_t color;
	
	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	
	// Use white shader for the mark
	shader = cgs.media.whiteShader;
	if (!shader)
	{
		return;
	}
	size = 1.0f;
	
	// Mark color (theme color with darkening -0.4)
	Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, color);
	CG_ColorWhitenSmart(color, NULL, -0.4f);
	color[3] = 0.8f;
	
	x = cgs.be.teamMarkers.ttwheelMarkX;
	y = cgs.be.teamMarkers.ttwheelMarkY;
	
	// Adjust coordinates from 640x480 to actual screen size
	CG_AdjustFrom640_Old(&x, &y, &size, &size, qfalse);
	
	size = 4.0f;
	// Draw the mark as a small square
	trap_R_SetColor(color);
	trap_R_DrawStretchPic(
		x - size * 0.5f,
		y - size * 0.5f,
		size,
		size,
		0,
		0,
		1,
		1,
		shader);
	
	trap_R_SetColor(NULL);
}

/*
=================
CG_GetSelwheelIconAtPosition

Check if position (x, y) is over any icon, return icon index or -1
=================
*/
int CG_GetSelwheelIconAtPosition(float x, float y)
{
	float centerX;
	float centerY;
	float iconRadius;
	float iconSize;
	float distFromCenter;
	float cursorAngle;
	float iconAngle;
	float startAngle;
	float angleStep;
	float minRadius;
	float maxRadius;
	float angleDiff;
	float normalizedAngleDiff;
	int i;
	int elementCount;
	
	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return -1;
	}
	
	elementCount = cgs.be.teamMarkers.ttwheelElementCount;
	if (elementCount <= 0)
	{
		return -1;
	}
	
	// Icons are fixed at screen center, not following the mark
	centerX = SCREEN_WIDTH / 2.0f;
	centerY = SCREEN_HEIGHT / 2.0f;
	iconRadius = 20.0f; // Distance from center to icons (reduced by 1/3)
	iconSize = 6; // Base icon size (reduced by 1/3 from 8.0f)
	
	// Adjust coordinates from 640x480 to actual screen size
	CG_AdjustFrom640_Old(&centerX, &centerY, NULL, NULL, qfalse);
	CG_AdjustFrom640_Old(&x, &y, NULL, NULL, qfalse);
	CG_AdjustFrom640_Old(NULL, NULL, &iconRadius, &iconSize, qfalse);
	
	// Calculate distance from center and angle
	distFromCenter = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
	cursorAngle = atan2(-(y - centerY), x - centerX); // Negative Y because screen Y increases downward
	
	// Normalize angle to 0..2π range
	if (cursorAngle < 0.0f)
	{
		cursorAngle += 2.0f * M_PI;
	}
	
	// Define activation zone: from minRadius to maxRadius
	// minRadius: small area in center where we reset selection
	// maxRadius: extend to cover area between center and icons, and beyond
	minRadius = 12.0f; // Small center area for reset (increased for larger center zone)
	maxRadius = iconRadius + 40.0f; // Extend beyond icons to cover gaps (proportionally reduced)
	
	// Adjust radii to screen size
	{
		float tempW;
		float tempH;
		tempW = minRadius;
		tempH = minRadius;
		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		minRadius = tempW;
		
		tempW = maxRadius;
		tempH = maxRadius;
		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		maxRadius = tempW;
	}
	
	// Check if bitmask & 2 is set (center element exists)
	if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
	{
		// Center element exists at index 0, check if cursor is in center zone
		if (distFromCenter < minRadius)
		{
			return 0; // Center zone - select center element
		}
		
		// For other elements (1+), use elementCount - 1 for angle calculation
		angleStep = 2.0f * M_PI / (float)(elementCount - 1);
		i = 1; // Start checking from index 1
	}
	else if ((cg_teamTaskWheel.integer & 2) && elementCount == 1)
	{
		// Only center element exists
		if (distFromCenter < minRadius)
		{
			return 0; // Center zone - select center element
		}
		return -1; // Outside center zone - no selection
	}
	else
	{
		// No center element, check if cursor is too close to center (reset zone)
	if (distFromCenter < minRadius)
	{
		return -1; // Center zone - no selection
		}
		angleStep = 2.0f * M_PI / (float)elementCount;
		i = 0; // Start checking from index 0
	}
	
	// Check if cursor is too far from center
	if (distFromCenter > maxRadius)
	{
		return -1; // Too far - no selection
	}
	
	// Start angle: 9 o'clock = 270 degrees = 3π/2 radians
	startAngle = M_PI * 1.5f;
	
	// Check each element's sector (elements start from i)
	for (; i < cgs.be.teamMarkers.ttwheelElementCount; i++)
	{
		// Calculate element angle
		if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
		{
			// For elements around center (index 1+), use (i - 1) for angle calculation
			iconAngle = startAngle + angleStep * (float)(i - 1);
		}
		else
		{
			// Normal case: all elements around circle
		iconAngle = startAngle + angleStep * (float)i;
		}
		
		// Normalize iconAngle to 0..2π range
		if (iconAngle < 0.0f)
		{
			iconAngle += 2.0f * M_PI;
		}
		else if (iconAngle >= 2.0f * M_PI)
		{
			iconAngle -= 2.0f * M_PI;
		}
		
		// Calculate angle difference
		angleDiff = cursorAngle - iconAngle;
		
		// Normalize to -π..π range
		if (angleDiff > M_PI)
		{
			angleDiff -= 2.0f * M_PI;
		}
		else if (angleDiff < -M_PI)
		{
			angleDiff += 2.0f * M_PI;
		}
		
		// Check if cursor is within this element's sector
		// Sector covers half the angle between elements on each side
		normalizedAngleDiff = fabs(angleDiff);
		if (normalizedAngleDiff <= angleStep * 0.5f)
		{
			return i;
		}
	}
	
	return -1;
}

/*
=================
CG_DrawSelwheelIcons

Draw team task icons in a circle around the ttwheel mark
=================
*/
void CG_DrawSelwheelIcons(void)
{
	float centerX;
	float centerY;
	float iconRadius;
	float iconSize;
	float backgroundSize;
	float iconAngle;
	float startAngle;
	vec4_t iconColor;
	vec4_t backgroundColor;
	float iconX;
	float iconY;
	float adjustedX;
	float adjustedY;
	float adjustedSize;
	float adjustedBgSize;
	float borderThickness;
	float tempW;
	float tempH;
	vec4_t debugColor;
	int hoveredIcon;
	int i;
	int font = CG_FontIndexFromCvar(&cg_teamTaskFont);
	
	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	
	// Icons are fixed at screen center, not following the mark
	centerX = SCREEN_WIDTH / 2.0f;
	centerY = SCREEN_HEIGHT / 2.0f;
	iconRadius = 20.0f; // Distance from center to icons (reduced by 1/3)
	iconSize = 6; // Base icon size (reduced by 1/3 from 8.0f)
	backgroundSize = iconSize * 1.2f; // Background size when hovered
	
	// Check for cooldown and display text above icons
	{
		int timeRemaining;
		int seconds;
		int milliseconds;
		float cooldownTextY;
		float cooldownTextX;
		float charWidth;
		float charHeight;
		vec4_t cooldownColor;
		char cooldownText[64];
		
		timeRemaining = TokenBucket_TimeToNext(&cgs.be.teamMarkers.sendCommandBucket, cg.time);
		
		/* Only show cooldown if there's actually a cooldown active */
		if (timeRemaining > 0)
		{
			seconds = timeRemaining / 1000;
			milliseconds = timeRemaining % 1000;
			
			/* Format: "Cooldown: X.X seconds" with 1 decimal place */
			Com_sprintf(cooldownText, sizeof(cooldownText), "Cooldown: %i.%i seconds", seconds, milliseconds / 100);
			
			/* Position text above icons - use screen coordinates directly */
			cooldownTextX = SCREEN_WIDTH / 2.0f;
			cooldownTextY = 100; /* Above icons with some spacing */
			
			/* Text size */
			charWidth = 8.0f;
			charHeight = 12.0f;
			
			/* Use theme color */
			Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, cooldownColor);
			
			/* Apply whitening to cooldown color with medium strength */
			CG_ColorWhitenSmart(cooldownColor, NULL, 0.5f);
			CG_FontSelect(font);
			/* Draw cooldown text */
			CG_OSPDrawStringNew(
				cooldownTextX, cooldownTextY,
				cooldownText,
				cooldownColor,
				colorBlack,
				charWidth, charHeight,
				SCREEN_WIDTH, /* max width for centering */
				DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
				NULL, NULL, NULL
			);
		}
	}
	
	// Icon color
	iconColor[0] = 1.0f;
	iconColor[1] = 1.0f;
	iconColor[2] = 1.0f;
	iconColor[3] = 1.0f;
	
	// Background color (use theme color with alpha 0.3)
	backgroundColor[0] = cgs.be.teamMarkers.teamTaskThemeColor[0];
	backgroundColor[1] = cgs.be.teamMarkers.teamTaskThemeColor[1];
	backgroundColor[2] = cgs.be.teamMarkers.teamTaskThemeColor[2];
	backgroundColor[3] = 0.3f;
	
	// Start angle: 9 o'clock = 270 degrees = 3π/2 radians
	startAngle = M_PI * 1.5f;
	
	// Draw center wheel background (size slightly smaller than icon radius)
	{
		float wheelX;
		float wheelY;
		float wheelSize;
		vec4_t wheelColor;
		
		wheelX = SCREEN_WIDTH / 2.0f;
		wheelY = SCREEN_HEIGHT / 2.0f;
		wheelSize = iconRadius * 5;
		
		// Use theme color for wheel background
		wheelColor[0] = 0;
		wheelColor[1] = 0;
		wheelColor[2] = 0;
		wheelColor[3] = 0.7f;
		
		CG_AdjustFrom640_Old(&wheelX, &wheelY, NULL, NULL, qfalse);
		CG_AdjustFrom640_Old(NULL, NULL, &wheelSize, &wheelSize, qfalse);
		
		if (cgs.media.teamTasksWheelbg)
		{
			trap_R_SetColor(wheelColor);
			trap_R_DrawStretchPic(
				wheelX - wheelSize * 0.5f,
				wheelY - wheelSize * 0.5f,
				wheelSize,
				wheelSize,
				0,
				0,
				1,
				1,
				cgs.media.teamTasksWheelbg);
			trap_R_SetColor(NULL);
		}
	}
	
	// Check which icon is hovered
	hoveredIcon = CG_GetSelwheelIconAtPosition(cgs.be.teamMarkers.ttwheelMarkX, cgs.be.teamMarkers.ttwheelMarkY);
	cgs.be.teamMarkers.ttwheelHoveredIcon = hoveredIcon;
	
	// Fix selected icon when hovered (will be used on release)
	if (hoveredIcon >= 0 && hoveredIcon < cgs.be.teamMarkers.ttwheelElementCount)
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = hoveredIcon;
	}
	else
	{
		// Check if cursor is in center reset zone (only if no center element)
		if (!(cg_teamTaskWheel.integer & 2))
		{
			float centerXCheck;
			float centerYCheck;
			float markXCheck;
			float markYCheck;
			float distFromCenter;
			float minRadiusCheck;
			float tempW;
			float tempH;
			
			centerXCheck = SCREEN_WIDTH / 2.0f;
			centerYCheck = SCREEN_HEIGHT / 2.0f;
			markXCheck = cgs.be.teamMarkers.ttwheelMarkX;
			markYCheck = cgs.be.teamMarkers.ttwheelMarkY;
			minRadiusCheck = 12.0f; // Match minRadius from CG_GetSelwheelIconAtPosition
			
			CG_AdjustFrom640_Old(&centerXCheck, &centerYCheck, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(&markXCheck, &markYCheck, NULL, NULL, qfalse);
			
			tempW = minRadiusCheck;
			tempH = minRadiusCheck;
			CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
			minRadiusCheck = tempW;
			
			distFromCenter = sqrt((markXCheck - centerXCheck) * (markXCheck - centerXCheck) + 
			                      (markYCheck - centerYCheck) * (markYCheck - centerYCheck));
			
			// Only reset if cursor is in center zone and no center element
			if (distFromCenter < minRadiusCheck)
			{
				cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
			}
			// Otherwise keep previous selection (don't reset when in empty area)
		}
	}
	
	// Draw center element first (if bitmask & 2 is set)
	if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 0)
	{
		if (cgs.be.teamMarkers.ttwheelElementShaders[0])
		{
			float centerIconX;
			float centerIconY;
			float centerIconSize;
			float centerIconBgSize;
			float smartIconSize;
			float smartIconX;
			float smartIconY;
			vec4_t smartIconColor;
			
			centerIconX = SCREEN_WIDTH / 2.0f;
			centerIconY = SCREEN_HEIGHT / 2.0f;
			centerIconSize = iconSize * 1.2f; // Larger center icon
			centerIconBgSize = backgroundSize * 1.2f; // Larger background for center icon
			smartIconSize = iconSize * 0.4f; // Smart icon size - small icon in corner
			
			CG_AdjustFrom640_Old(&centerIconX, &centerIconY, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &centerIconSize, &centerIconSize, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &centerIconBgSize, &centerIconBgSize, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &smartIconSize, &smartIconSize, qfalse);
			
			// Calculate smart icon position (right top corner of center icon)
			smartIconX = centerIconX + centerIconSize * 0.5f - smartIconSize * 0.5f;
			smartIconY = centerIconY - centerIconSize * 0.5f + smartIconSize * 0.5f;
			
			// If hovered or selected, draw background first
			if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
			{
				trap_R_SetColor(backgroundColor);
				trap_R_DrawStretchPic(
					centerIconX - centerIconBgSize * 0.5f,
					centerIconY - centerIconBgSize * 0.5f,
					centerIconBgSize,
					centerIconBgSize,
					0,
					0,
					1,
					1,
					cgs.be.teamMarkers.ttwheelElementShaders[0]);
			}
			
			// Draw center icon
			// Use gray color for non-selected element, white for selected
			if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
			{
				// Element is selected - use white color
				trap_R_SetColor(iconColor);
			}
			else
			{
				// Element is not selected - use gray color
				vec4_t grayColor;
				Vector4Set(grayColor, 0.75f, 0.75f, 0.75f, 1.0f);
				trap_R_SetColor(grayColor);
			}
			trap_R_DrawStretchPic(
				centerIconX - centerIconSize * 0.5f,
				centerIconY - centerIconSize * 0.5f,
				centerIconSize,
				centerIconSize,
				0,
				0,
				1,
				1,
				cgs.be.teamMarkers.ttwheelElementShaders[0]);
			trap_R_SetColor(NULL);
			
			// Draw smart icon in right top corner of center icon (always, if command is tsmart)
			// Draw after main icon so it appears on top
			if (cgs.media.teamTasksSmart && cgs.be.teamMarkers.ttwheelElementCommands[0][0] != '\0' &&
				!Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[0], "tsmart"))
			{
				// White color when element is not selected, theme color when selected
				if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
				{
					// Element is selected - use theme color
					Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, smartIconColor);
					CG_ColorWhitenSmart(smartIconColor, NULL, -0.4f);
				}
				else
				{
					// Element is not selected - use white color
					Vector4Set(smartIconColor, 0.0f, 0.0f, 0.0f, 1.0f);
				}
				trap_R_SetColor(smartIconColor);
				trap_R_DrawStretchPic(
					smartIconX - smartIconSize * 0.5f,
					smartIconY - smartIconSize * 0.5f,
					smartIconSize,
					smartIconSize,
					0,
					0,
					1,
					1,
					cgs.media.teamTasksSmart);
				trap_R_SetColor(NULL);
			}
		}
		
		// Draw other icons around circle (starting from index 1)
		i = 1;
	}
	else
	{
		// No center element, draw all icons around circle
		i = 0;
	}
	
	// Draw icons around circle
	for (; i < cgs.be.teamMarkers.ttwheelElementCount; i++)
	{
		if (!cgs.be.teamMarkers.ttwheelElementShaders[i])
		{
			continue;
		}
		
		// Distribute evenly around circle starting from 9 o'clock
		if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 1)
		{
			// Center element exists, distribute other elements (i-1) around circle
			iconAngle = startAngle + (2.0f * M_PI * (float)(i - 1) / (float)(cgs.be.teamMarkers.ttwheelElementCount - 1));
		}
		else
		{
			// No center element, distribute all elements around circle
		iconAngle = startAngle + (2.0f * M_PI * (float)i / (float)cgs.be.teamMarkers.ttwheelElementCount);
		}
		
		// Calculate icon position using the SAME logic as CG_GetSelwheelIconAtPosition
		// to ensure icon is centered in activation block
		{
			float centerXIcon;
			float centerYIcon;
			float iconRadiusIcon;
			float iconAngleIcon;
			
			// Use same calculation as in CG_GetSelwheelIconAtPosition
			centerXIcon = SCREEN_WIDTH / 2.0f;
			centerYIcon = SCREEN_HEIGHT / 2.0f;
			iconRadiusIcon = 60.0f;
			
			// Adjust coordinates from 640x480 to actual screen size
			CG_AdjustFrom640_Old(&centerXIcon, &centerYIcon, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &iconRadiusIcon, NULL, qfalse);
			
			// Calculate icon angle (same as in CG_GetSelwheelIconAtPosition)
			if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 1)
			{
				iconAngleIcon = startAngle + (2.0f * M_PI * (float)(i - 1) / (float)(cgs.be.teamMarkers.ttwheelElementCount - 1));
			}
			else
			{
			iconAngleIcon = startAngle + (2.0f * M_PI * (float)i / (float)cgs.be.teamMarkers.ttwheelElementCount);
			}
			
			// Calculate icon position (same as in CG_GetSelwheelIconAtPosition)
			// These coordinates are already scaled
			iconX = centerXIcon + cos(iconAngleIcon) * iconRadiusIcon;
			iconY = centerYIcon - sin(iconAngleIcon) * iconRadiusIcon; // Negative because screen Y increases downward
		}
		
		// iconX and iconY are already scaled, so use them directly
		adjustedX = iconX;
		adjustedY = iconY;
		adjustedSize = iconSize;
		adjustedBgSize = backgroundSize;
		
		// Adjust only size, not position (position is already scaled)
		CG_AdjustFrom640_Old(NULL, NULL, &adjustedSize, &adjustedSize, qfalse);
		CG_AdjustFrom640_Old(NULL, NULL, &adjustedBgSize, &adjustedBgSize, qfalse);
		
		// If hovered, draw background first (cyan, alpha 0.2, size 1.2)
		if (hoveredIcon == i)
		{
			// Draw background icon
			trap_R_SetColor(backgroundColor);
			trap_R_DrawStretchPic(
				adjustedX - adjustedBgSize * 0.5f,
				adjustedY - adjustedBgSize * 0.5f,
				adjustedBgSize,
				adjustedBgSize,
				0,
				0,
				1,
				1,
				cgs.be.teamMarkers.ttwheelElementShaders[i]);
		}
		
		// Draw icon
		// Use gray color for non-selected elements, white for selected
		if (hoveredIcon == i || cgs.be.teamMarkers.ttwheelSelectedIcon == i)
		{
			// Element is selected - use white color
			trap_R_SetColor(iconColor);
		}
		else
		{
			// Element is not selected - use gray color
			vec4_t grayColor;
			Vector4Set(grayColor, 0.6f, 0.6f, 0.6f, 1.0f);
			trap_R_SetColor(grayColor);
		}
		trap_R_DrawStretchPic(
			adjustedX - adjustedSize * 0.5f,
			adjustedY - adjustedSize * 0.5f,
			adjustedSize,
			adjustedSize,
			0,
			0,
			1,
			1,
			cgs.be.teamMarkers.ttwheelElementShaders[i]);
		
		// Draw smart icon in right top corner of icon (always, if command is tsmart)
		if (cgs.media.teamTasksSmart && cgs.be.teamMarkers.ttwheelElementCommands[i][0] != '\0' &&
			!Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[i], "tsmart"))
		{
			float smartIconSizeWheel;
			float smartIconXWheel;
			float smartIconYWheel;
			vec4_t smartIconColorWheel;
			
			smartIconSizeWheel = adjustedSize * 0.4f; // Smart icon size - small icon in corner
			
			// Calculate smart icon position (right top corner of icon)
			smartIconXWheel = adjustedX + adjustedSize * 0.5f - smartIconSizeWheel * 0.5f;
			smartIconYWheel = adjustedY - adjustedSize * 0.5f + smartIconSizeWheel * 0.5f;
			
			// White color when element is not selected, theme color when selected
			if (hoveredIcon == i || cgs.be.teamMarkers.ttwheelSelectedIcon == i)
			{
				// Element is selected - use theme color
				Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, smartIconColorWheel);
				CG_ColorWhitenSmart(smartIconColorWheel, NULL, -0.4f);
			}
			else
			{
				// Element is not selected - use white color
				Vector4Set(smartIconColorWheel, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			trap_R_SetColor(smartIconColorWheel);
			trap_R_DrawStretchPic(
				smartIconXWheel - smartIconSizeWheel * 0.5f,
				smartIconYWheel - smartIconSizeWheel * 0.5f,
				smartIconSizeWheel,
				smartIconSizeWheel,
				0,
				0,
				1,
				1,
				cgs.media.teamTasksSmart);
			trap_R_SetColor(NULL);
		}
		
		// // Debug: draw activation field border (yellow, sector)
		// // Draw sector boundaries from minRadius to maxRadius
		// {
		// 	float centerXDebug;
		// 	float centerYDebug;
		// 	float iconAngleDebug;
		// 	float startAngleDebug;
		// 	float angleStepDebug;
		// 	float minRadiusDebug;
		// 	float maxRadiusDebug;
		// 	float sectorStartAngle;
		// 	float sectorEndAngle;
		// 	float lineX1;
		// 	float lineY1;
		// 	float lineX2;
		// 	float lineY2;
		// 	float lineDX;
		// 	float lineDY;
		// 	float lineLength;
		// 	int segmentCount;
		// 	int seg;
			
		// 	// EXACT copy of logic from CG_GetSelwheelIconAtPosition
		// 	centerXDebug = SCREEN_WIDTH / 2.0f;
		// 	centerYDebug = SCREEN_HEIGHT / 2.0f;
		// 	minRadiusDebug = 5.0f; // Match minRadius from CG_GetSelwheelIconAtPosition
		// 	maxRadiusDebug = 60.0f + 60.0f; // iconRadius + extension (match maxRadius)
			
		// 	// Adjust coordinates from 640x480 to actual screen size
		// 	CG_AdjustFrom640_Old(&centerXDebug, &centerYDebug, NULL, NULL, qfalse);
		// 	{
		// 		float tempW;
		// 		float tempH;
		// 		tempW = minRadiusDebug;
		// 		tempH = minRadiusDebug;
		// 		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		// 		minRadiusDebug = tempW;
				
		// 		tempW = maxRadiusDebug;
		// 		tempH = maxRadiusDebug;
		// 		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		// 		maxRadiusDebug = tempW;
		// 	}
			
		// 	// Start angle: 9 o'clock = 270 degrees = 3π/2 radians
		// 	startAngleDebug = M_PI * 1.5f;
		// 	if ((cg_teamTaskWheel.integer == 2) && cgs.be.teamMarkers.ttwheelElementCount > 1)
		// 	{
		// 		// Center element exists, calculate angles for elements around circle (i-1)
		// 		angleStepDebug = 2.0f * M_PI / (float)(cgs.be.teamMarkers.ttwheelElementCount - 1);
		// 		iconAngleDebug = startAngleDebug + angleStepDebug * (float)(i - 1);
		// 	}
		// 	else
		// 	{
		// 		// No center element, calculate angles for all elements
		// 	angleStepDebug = 2.0f * M_PI / (float)cgs.be.teamMarkers.ttwheelElementCount;
		// 	iconAngleDebug = startAngleDebug + angleStepDebug * (float)i;
		// 	}
		// 	sectorStartAngle = iconAngleDebug - angleStepDebug * 0.5f;
		// 	sectorEndAngle = iconAngleDebug + angleStepDebug * 0.5f;
			
		// 	// Border thickness (1 pixel, scale it)
		// 	borderThickness = 1.0f;
		// 	CG_AdjustFrom640_Old(NULL, NULL, &borderThickness, NULL, qfalse);
			
		// 	// Yellow color
		// 	debugColor[0] = 1.0f; // R
		// 	debugColor[1] = 1.0f; // G
		// 	debugColor[2] = 0.0f; // B
		// 	debugColor[3] = 1.0f; // A
			
		// 	trap_R_SetColor(debugColor);
			
		// 	// Draw sector boundaries: two radial lines from minRadius to maxRadius
		// 	// Left boundary (sectorStartAngle)
		// 	lineX1 = centerXDebug + cos(sectorStartAngle) * minRadiusDebug;
		// 	lineY1 = centerYDebug - sin(sectorStartAngle) * minRadiusDebug;
		// 	lineX2 = centerXDebug + cos(sectorStartAngle) * maxRadiusDebug;
		// 	lineY2 = centerYDebug - sin(sectorStartAngle) * maxRadiusDebug;
		// 	lineDX = lineX2 - lineX1;
		// 	lineDY = lineY2 - lineY1;
		// 	lineLength = sqrt(lineDX * lineDX + lineDY * lineDY);
		// 	// Draw line as rotated rectangle (simplified - draw horizontal line and rotate)
		// 	// For simplicity, draw as multiple small segments
		// 	segmentCount = (int)(lineLength / borderThickness) + 1;
		// 	if (segmentCount > 100)
		// 	{
		// 		segmentCount = 100;
		// 	}
		// 	for (seg = 0; seg < segmentCount; seg++)
		// 	{
		// 		float segFrac;
		// 		float segX;
		// 		float segY;
		// 		segFrac = (float)seg / (float)segmentCount;
		// 		segX = lineX1 + lineDX * segFrac;
		// 		segY = lineY1 + lineDY * segFrac;
		// 		trap_R_DrawStretchPic(segX - borderThickness * 0.5f, segY - borderThickness * 0.5f, borderThickness, borderThickness, 0, 0, 0, 0, cgs.media.whiteShader);
		// 	}
			
		// 	// Right boundary (sectorEndAngle)
		// 	lineX1 = centerXDebug + cos(sectorEndAngle) * minRadiusDebug;
		// 	lineY1 = centerYDebug - sin(sectorEndAngle) * minRadiusDebug;
		// 	lineX2 = centerXDebug + cos(sectorEndAngle) * maxRadiusDebug;
		// 	lineY2 = centerYDebug - sin(sectorEndAngle) * maxRadiusDebug;
		// 	lineDX = lineX2 - lineX1;
		// 	lineDY = lineY2 - lineY1;
		// 	lineLength = sqrt(lineDX * lineDX + lineDY * lineDY);
		// 	segmentCount = (int)(lineLength / borderThickness) + 1;
		// 	if (segmentCount > 100)
		// 	{
		// 		segmentCount = 100;
		// 	}
		// 	for (seg = 0; seg < segmentCount; seg++)
		// 	{
		// 		float segFrac;
		// 		float segX;
		// 		float segY;
		// 		segFrac = (float)seg / (float)segmentCount;
		// 		segX = lineX1 + lineDX * segFrac;
		// 		segY = lineY1 + lineDY * segFrac;
		// 		trap_R_DrawStretchPic(segX - borderThickness * 0.5f, segY - borderThickness * 0.5f, borderThickness, borderThickness, 0, 0, 0, 0, cgs.media.whiteShader);
		// 	}
			
		// 	trap_R_SetColor(NULL);
		// }
	}
	
	trap_R_SetColor(NULL);
}

/*
=================
CG_DrawSelwheelPointers

Draw arrows pointing to ttwheel elements (must be called before trap_R_RenderScene)
=================
*/
void CG_DrawSelwheelPointers(void)
{
	float centerX;
	float centerY;
	float pointerRadius;
	float pointerSize;
	float pointerAngle;
	float startAngle;
	float angleStep;
	vec4_t activeColor;
	vec4_t inactiveColor;
	int selectedIcon;
	int elementCount;
	int i;
	int startIndex;
	
	if (!cg_teamTaskWheel.integer && 8)
	{
		return;
	}

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}
	
	// Check if shader is loaded, if not try to use whiteShader as fallback
	if (!cgs.media.teamTasksPointing)
	{
		// Try fallback to whiteShader for testing
		if (!cgs.media.whiteShader)
		{
			return;
		}
		// Will use whiteShader below
	}
	
	elementCount = cgs.be.teamMarkers.ttwheelElementCount;
	if (elementCount <= 0)
	{
		return;
	}
	
	selectedIcon = cgs.be.teamMarkers.ttwheelSelectedIcon;
	
	// Pointer radius: between center and icons (in unused zone)
	// Icons use iconRadius = 20.0f in 640x480 coordinates
	// Use smaller radius for pointers (between center and icons)
	pointerRadius = 24.0f; // Position between center and icons (in 640x480 coordinates)
	pointerSize = 8.0f; // Size of pointer arrow (increased for visibility)
	
	// Start angle: 9 o'clock = 270 degrees = 3π/2 radians
	startAngle = M_PI * 1.5f;
	Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, activeColor);
	CG_ColorWhitenSmart(activeColor, NULL, 0.4f);
	// Active color (white) for selected element
	
	// Inactive color (gray) for non-selected elements
	Vector4Set(inactiveColor, 0.6f, 0.6f, 0.6f, 0.8f);
	
	// Determine start index based on center element
	if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
	{
		// Center element exists at index 0, draw pointers for other elements (1+)
		angleStep = 2.0f * M_PI / (float)(elementCount - 1);
		startIndex = 1;
	}
	else if ((cg_teamTaskWheel.integer & 2) && elementCount == 1)
	{
		// Only center element exists, no pointers needed
		return;
	}
	else
	{
		// No center element, draw pointers for all elements
		angleStep = 2.0f * M_PI / (float)elementCount;
		startIndex = 0;
	}
	
	// Draw pointers for each element
	for (i = startIndex; i < elementCount; i++)
	{
		// Calculate angle for this element
		if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
		{
			// Center element exists, use (i - 1) for angle calculation
			pointerAngle = startAngle + angleStep * (float)(i - 1);
		}
		else
		{
			// No center element, use i for angle calculation
			pointerAngle = startAngle + angleStep * (float)i;
		}
		
		// Calculate pointer position
		// CG_DrawRotatedPic expects coordinates in 640x480 virtual system, not scaled
		// Compensate for different X/Y scaling due to FOV to keep perfect circle
		{
			float pointerX;
			float pointerY;
			qhandle_t pointerShader;
			vec4_t pointerColor;
			float centerX;
			float centerY;
			float hfov_x;
			float hfov_y;
			float compensation;
			float dx;
			float dy;
			
			// Use 640x480 coordinates (320, 240 = center)
			// CG_DrawRotatedPic will handle scaling internally
			centerX = 320.0f; // Center X in 640x480 coordinates
			centerY = 240.0f; // Center Y in 640x480 coordinates
			
			// Calculate FOV to compensate for ellipse distortion
			hfov_x = cg.refdef.fov_x * M_PI / 360.0f;
			hfov_y = cg.refdef.fov_y * M_PI / 360.0f;
			
			// Compensation factor to make circle perfect (compensate for different X/Y scaling)
			// CG_DrawRotatedPic uses: offsetX = dx * tan(hfov_x) / (SCREEN_WIDTH * 0.5f)
			//                        offsetY = dy * tan(hfov_y) / (SCREEN_HEIGHT * 0.5f)
			// To make circle, we need: offsetX/offsetY ratio to be 1:1
			// So we compensate Y coordinate
			compensation = (tan(hfov_x) * SCREEN_HEIGHT) / (tan(hfov_y) * SCREEN_WIDTH);
			
			// Calculate position in 640x480 coordinates
			dx = -cos(pointerAngle) * pointerRadius;
			dy = -sin(pointerAngle) * pointerRadius;
			
			// Apply compensation to Y to keep perfect circle
			dy = dy * compensation;
			
			pointerX = centerX + dx;
			pointerY = centerY + dy; // Negative because screen Y increases downward
			
			// Use teamTasksPointing if available, otherwise fallback to whiteShader
			if (cgs.media.teamTasksPointing)
			{
				pointerShader = cgs.media.teamTasksPointing;
			}
			else
			{
				pointerShader = cgs.media.whiteShader;
			}
			
			// Determine color based on selection
			if (selectedIcon == i)
			{
				// Selected element - white
				Vector4Copy(activeColor, pointerColor);
			}
			else
			{
				// Non-selected element - gray
				Vector4Copy(inactiveColor, pointerColor);
			}
			
			// Draw pointer arrow
			// CG_DrawRotatedPic expects coordinates and size in 640x480 virtual system
			// Arrow should point outward (toward icon), so add 180 degrees to point away from center
			// Rotate 90 degrees counter-clockwise (subtract 90 degrees)
			CG_DrawRotatedPic(
				pointerX,
				pointerY,
				pointerSize,
				pointerSize,
				pointerShader,
				pointerAngle * 180.0f / M_PI + 180.0f + 90.0f, // Convert to degrees, add 180 to point outward, subtract 90 for counter-clockwise rotation
				pointerColor
			);
		}
	}
}

/*
=================
CG_SelwheelMarkStart

Start ttwheel mark tracking
=================
*/
void CG_SelwheelMarkStart(void)
{
	int currentCmdNum;
	usercmd_t currentCmd;
	
	/* Check if teamTask is disabled */
	if (!cg_teamTask.integer)
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	/* Check if teamTaskWheel is disabled */
	if (!cg_teamTaskWheel.integer)
	{
		return;
	}
	/* Check if we are not playing (observing another client) */
	if (!CG_IsLocalClientOnScreen())
	{
		return;
	}
	if (CG_IsDead())
	{
		return;
	}
	/* Check if we are in spectator/following mode */
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}
	/* Check if local player is spectator */
	if (CG_IsLocalClientSpectator())
	{
		return;
	}
	
	// If already active, don't reset position - just update angles
	if (cgs.be.teamMarkers.ttwheelMarkActive)
	{
		currentCmdNum = trap_GetCurrentCmdNumber();
		if (trap_GetUserCmd(currentCmdNum, &currentCmd))
		{
			cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
			cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
			cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];
		}
		return;
	}
	
	// First time activation - initialize position to center
	cgs.be.teamMarkers.ttwheelMarkActive = qtrue;
	cgs.be.teamMarkers.ttwheelMarkX = SCREEN_WIDTH / 2.0f;
	cgs.be.teamMarkers.ttwheelMarkY = SCREEN_HEIGHT / 2.0f;
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;
	
	// If smart mode is enabled (bitmask & 2), save target at wheel opening
	if (cg_teamTaskWheel.integer & 2)
	{
		vec3_t itemPos;
		trace_t trace;
		vec3_t start;
		vec3_t end;
		int i;
		centity_t* cent;
		
		// Check what we're aiming at using CG_CrosshairTarget
		cgs.be.teamMarkers.smartTargetType = CG_CrosshairTarget(itemPos);
		cgs.be.teamMarkers.smartTargetTime = cg.time;
		cgs.be.teamMarkers.smartTargetEntityNum = -1;
		
		// Save target position and entity number
		if (cgs.be.teamMarkers.smartTargetType == 2)
		{
			// Enemy - get their position and entity number
			VectorCopy(cg.refdef.vieworg, start);
			VectorMA(start, 131072, cg.refdef.viewaxis[0], end);
			
			CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
			         cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
			
			if (trace.entityNum >= 0 && trace.entityNum < MAX_CLIENTS)
			{
				cgs.be.teamMarkers.smartTargetEntityNum = trace.entityNum;
				// Find player entity and get their position
				for (i = 0; i < MAX_GENTITIES; i++)
				{
					cent = &cg_entities[i];
					if (cent->currentState.eType == ET_PLAYER && 
					    cent->currentState.clientNum == trace.entityNum &&
					    cent->currentValid)
					{
						VectorCopy(cent->lerpOrigin, cgs.be.teamMarkers.smartTargetPos);
						cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
						break;
					}
				}
				// Fallback: if enemy entity not found, use trace endpos
				if (i >= MAX_GENTITIES)
				{
					VectorCopy(trace.endpos, cgs.be.teamMarkers.smartTargetPos);
					cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
				}
			}
			else
			{
				// Fallback: use trace endpos
				VectorCopy(trace.endpos, cgs.be.teamMarkers.smartTargetPos);
				cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
			}
		}
		else if (cgs.be.teamMarkers.smartTargetType == 3 || cgs.be.teamMarkers.smartTargetType == 4)
		{
			// Item (red armor or megahealth) - use item center position
			VectorCopy(itemPos, cgs.be.teamMarkers.smartTargetPos);
		}
		else if (cgs.be.teamMarkers.smartTargetType == 5 || cgs.be.teamMarkers.smartTargetType == 6)
		{
			// Frozen body (ally or enemy) - use frozen body position
			VectorCopy(itemPos, cgs.be.teamMarkers.smartTargetPos);
		}
		else
		{
			// Nothing - clear target
			cgs.be.teamMarkers.smartTargetType = 1;
			VectorClear(cgs.be.teamMarkers.smartTargetPos);
		}
	}
	else
	{
		// Not smart mode - clear saved target
		cgs.be.teamMarkers.smartTargetType = 1;
		VectorClear(cgs.be.teamMarkers.smartTargetPos);
		cgs.be.teamMarkers.smartTargetTime = 0;
		cgs.be.teamMarkers.smartTargetEntityNum = -1;
	}
	
	// Initialize default elements if not set
	if (cgs.be.teamMarkers.ttwheelElementCount == 0)
	{
		CG_SelwheelParseTeamTaskWheel();
	}
	
	// If bitmask & 2 is set, select center element (index 0) by default
	if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 0)
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = 0;
	}
	else
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
	}
	
	currentCmdNum = trap_GetCurrentCmdNumber();
	if (trap_GetUserCmd(currentCmdNum, &currentCmd))
	{
		cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
		cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];
	}
	else
	{
		cgs.be.teamMarkers.lastCmdNum = 0;
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = 0;
		cgs.be.teamMarkers.lastCmdAngles[YAW] = 0;
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = 0;
	}
	
	cgs.be.teamMarkers.lastMovementTime = cg.time;
}

/*
=================
CG_SelwheelMarkStop

Stop ttwheel mark tracking and execute selected command
=================
*/
void CG_SelwheelMarkStop(void)
{
	int selectedIcon;
	char commandStr[MAX_STRING_CHARS];
	
	// Use fixed selected icon (was set when cursor hovered over icon)
	selectedIcon = cgs.be.teamMarkers.ttwheelSelectedIcon;
	
	// Send command based on selected icon
	if (selectedIcon >= 0 && selectedIcon < cgs.be.teamMarkers.ttwheelElementCount)
	{
		// Send console command first (add semicolon if not present)
		if (cgs.be.teamMarkers.ttwheelElementCommands[selectedIcon][0] != '\0')
		{
			int cmdLen;
			Q_strncpyz(commandStr, cgs.be.teamMarkers.ttwheelElementCommands[selectedIcon], sizeof(commandStr));
			// Ensure command ends with semicolon
			cmdLen = strlen(commandStr);
			if (cmdLen > 0 && commandStr[cmdLen - 1] != ';')
			{
				Q_strcat(commandStr, sizeof(commandStr), ";");
			}
			// Send command BEFORE deactivating ttwheel
			trap_SendConsoleCommand(commandStr);
		}
		
		// Call the action callback for this element
		if (cgs.be.teamMarkers.ttwheelElementActions[selectedIcon])
		{
			cgs.be.teamMarkers.ttwheelElementActions[selectedIcon]();
		}
	}
	
	// Deactivate ttwheel AFTER sending command
	cgs.be.teamMarkers.ttwheelMarkActive = qfalse;
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;
	cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
	
	// Clear saved smart target
	cgs.be.teamMarkers.smartTargetType = 1;
	VectorClear(cgs.be.teamMarkers.smartTargetPos);
	cgs.be.teamMarkers.smartTargetTime = 0;
	cgs.be.teamMarkers.smartTargetEntityNum = -1;
}

/*
=================
CG_SelwheelMarkClose

Close ttwheel without sending command or executing action
=================
*/
void CG_SelwheelMarkClose(void)
{
	/* Deactivate ttwheel without sending command or executing action */
	cgs.be.teamMarkers.ttwheelMarkActive = qfalse;
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;
	cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
	
	/* Clear saved smart target */
	cgs.be.teamMarkers.smartTargetType = 1;
	VectorClear(cgs.be.teamMarkers.smartTargetPos);
	cgs.be.teamMarkers.smartTargetTime = 0;
	cgs.be.teamMarkers.smartTargetEntityNum = -1;
}

/*
=================
CG_SelwheelSetElements

Set the elements for ttwheel menu
count: number of elements (max MAX_SELWHEEL_ELEMENTS)
shaders: array of shader handles for icons
actions: array of function pointers for actions
commands: array of console command strings (e.g. "tattack", "tdefence", etc.)
=================
*/
void CG_SelwheelSetElements(int count, qhandle_t* shaders, void (**actions)(void), const char** commands)
{
	int i;
	
	if (count < 0 || count > MAX_SELWHEEL_ELEMENTS)
	{
		count = 0;
	}
	
	cgs.be.teamMarkers.ttwheelElementCount = count;
	
	for (i = 0; i < count; i++)
	{
		if (shaders)
		{
			cgs.be.teamMarkers.ttwheelElementShaders[i] = shaders[i];
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementShaders[i] = 0;
		}
		
		if (actions)
		{
			cgs.be.teamMarkers.ttwheelElementActions[i] = actions[i];
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementActions[i] = NULL;
		}
		
		if (commands && commands[i])
		{
			Q_strncpyz(cgs.be.teamMarkers.ttwheelElementCommands[i], commands[i], sizeof(cgs.be.teamMarkers.ttwheelElementCommands[i]));
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementCommands[i][0] = '\0';
		}
	}
	
	// Clear remaining slots
	for (i = count; i < MAX_SELWHEEL_ELEMENTS; i++)
	{
		cgs.be.teamMarkers.ttwheelElementShaders[i] = 0;
		cgs.be.teamMarkers.ttwheelElementActions[i] = NULL;
		cgs.be.teamMarkers.ttwheelElementCommands[i][0] = '\0';
	}
}

/*
=================
CG_SelwheelParseTeamTaskWheel

Parse cg_teamTaskWheelSet cvar and set up ttwheel elements
=================
*/
void CG_SelwheelParseTeamTaskWheel(void)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* parse;
	char* token;
	int count;
	qhandle_t shaders[MAX_SELWHEEL_ELEMENTS];
	void (*actions[MAX_SELWHEEL_ELEMENTS])(void);
	const char* commands[MAX_SELWHEEL_ELEMENTS];
	int i;
	qboolean hasCenterElement;

	/* Use global teamTaskCommands array */

	count = 0;
	hasCenterElement = (cg_teamTaskWheel.integer & 2) ? qtrue : qfalse;

	/* If bitmask & 2 is set and cg_teamTaskWheelSet is not empty, use first element as center */
	if (hasCenterElement && cg_teamTaskWheelSet.string[0] != '\0')
	{
		char* firstToken;
		char* firstParse;
		teamTaskCommand_t* cmd;
		qhandle_t centerShader;
		qboolean centerElementSet;
		
		centerElementSet = qfalse;
		
		/* Parse first token from cvar */
		Q_strncpyz(buffer, cg_teamTaskWheelSet.string, sizeof(buffer));
		firstParse = buffer;
		
		/* Skip whitespace */
		while (*firstParse == ' ' || *firstParse == '\t')
		{
			firstParse++;
		}
		
		if (*firstParse != '\0')
		{
			firstToken = firstParse;
			while (*firstParse && *firstParse != ' ' && *firstParse != '\t')
			{
				firstParse++;
			}
			
			if (*firstParse)
			{
				*firstParse = '\0';
			}
			
			/* Find matching element using global array */
			cmd = CG_GetTeamTaskCommandByName(firstToken);
			if (cmd && cmd->action != NULL)
			{
				centerShader = CG_GetTeamTaskShaderByType(cmd->taskType);
				if (centerShader)
				{
					shaders[count] = centerShader;
					actions[count] = cmd->action;
					commands[count] = cmd->command;
					count++;
					centerElementSet = qtrue;
				}
			}
		}
		
		/* If first element from cvar failed, fallback to tsmart */
		if (!centerElementSet)
		{
			cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);
			if (cmd && cmd->action != NULL)
			{
				centerShader = CG_GetTeamTaskShaderByType(cmd->taskType);
				if (centerShader)
				{
					shaders[count] = centerShader;
					actions[count] = CG_TeamTaskMarkSend;
					commands[count] = "tsmart";
					count++;
					centerElementSet = qtrue;
				}
			}
			if (!centerElementSet && cgs.media.teamTasksLocation)
			{
				shaders[count] = cgs.media.teamTasksLocation;
				actions[count] = CG_TeamTaskMarkSend;
				commands[count] = "tsmart";
				count++;
			}
		}
	}
	/* If bitmask & 2 is set but cg_teamTaskWheelSet is empty, add tsmart as center element (index 0) */
	else if (hasCenterElement)
	{
		teamTaskCommand_t* cmd;
		qhandle_t smartShader;
		
		/* Get default type from cvar (string name) */
		cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);
		
		/* Check if command is valid and has action (for icon display, isSelectable check is not needed) */
		if (cmd && cmd->action != NULL)
		{
			smartShader = CG_GetTeamTaskShaderByType(cmd->taskType);
			if (smartShader)
			{
				shaders[count] = smartShader;
				actions[count] = CG_TeamTaskMarkSend;
				commands[count] = "tsmart";
				count++;
			}
		}
		/* Fallback to location if cvar is invalid or shader not loaded */
		if (count == 0 && cgs.media.teamTasksLocation)
		{
			shaders[count] = cgs.media.teamTasksLocation;
			actions[count] = CG_TeamTaskMarkSend;
			commands[count] = "tsmart";
			count++;
		}
	}

	/* If cvar is empty, use default elements (all commands with actions, excluding auto-selected) */
	if (cg_teamTaskWheelSet.string[0] == '\0')
	{
		/* Use all default elements that have actions (exclude auto-selected: target, redarmor, megahealth) */
		for (i = 0; i < NUM_TEAMTASK_COMMANDS && count < MAX_SELWHEEL_ELEMENTS; i++)
		{
			if (teamTaskCommands[i].action != NULL)
			{
				shaders[count] = CG_GetTeamTaskShaderByType(teamTaskCommands[i].taskType);
				actions[count] = teamTaskCommands[i].action;
				commands[count] = teamTaskCommands[i].command;
				count++;
			}
		}
	}
	else
	{
		/* Parse cvar string - skip first token if it was used as center element */
		Q_strncpyz(buffer, cg_teamTaskWheelSet.string, sizeof(buffer));
		parse = buffer;
		
		/* Skip first token if center element was set from cvar */
		if (hasCenterElement && count > 0)
		{
			/* Skip whitespace */
			while (*parse == ' ' || *parse == '\t')
			{
				parse++;
			}
			
			/* Skip first token */
			while (*parse && *parse != ' ' && *parse != '\t')
			{
				parse++;
			}
		}

		while (*parse && count < MAX_SELWHEEL_ELEMENTS)
		{
			/* Skip whitespace */
			while (*parse == ' ' || *parse == '\t')
			{
				parse++;
			}

			if (*parse == '\0')
			{
				break;
			}

			/* Get token */
			token = parse;
			while (*parse && *parse != ' ' && *parse != '\t')
			{
				parse++;
			}

			if (*parse)
			{
				*parse = '\0';
				parse++;
			}

			/* Find matching element using global array */
			{
				teamTaskCommand_t* cmd;
				
				cmd = CG_GetTeamTaskCommandByName(token);
				if (cmd && cmd->action != NULL)
				{
					shaders[count] = CG_GetTeamTaskShaderByType(cmd->taskType);
					actions[count] = cmd->action;
					commands[count] = cmd->command;
					count++;
				}
			}
		}
	}

	/* Set elements */
	if (count > 0)
	{
		CG_SelwheelSetElements(count, shaders, actions, commands);
	}
	else
	{
		/* Fallback to default if parsing failed */
		CG_SelwheelSetElements(0, NULL, NULL, NULL);
	}
}
