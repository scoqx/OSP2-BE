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
// cg_teamtasks_utils.c -- team task utility functions and command definitions

#include "cg_local.h"

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
CG_HandlePlayerDeathTeamTask

Handle team task marker when local player dies
Saves death coordinates for later use
=================
*/
void CG_HandlePlayerDeathTeamTask(int target, qboolean isUnfreezeEvent)
{
	centity_t* cent;
	vec3_t position;
	qboolean shouldSavePosition = qfalse;
	qboolean shouldSendMark = qfalse;

	/* Check conditions */
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	if (target != cg.clientNum)
	{
		return;
	}
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}
	if (isUnfreezeEvent)
	{
		return;
	}

	/* Get player entity */
	cent = &cg_entities[target];
	if (!cent)
	{
		return;
	}

	/* Get death position */
	VectorCopy(cent->lerpOrigin, position);
	position[2] += 6.0f;

	/* Always save death position for local client */
	shouldSavePosition = qtrue;

	/* Send team task mark only if enabled */
	if (cg_teamTask.integer & CG_TEAMTASK_ONDEATH)
	{
		shouldSendMark = qtrue;
	}

	/* Save death coordinates */
	if (shouldSavePosition)
	{
		VectorCopy(position, cgs.be.teamMarkers.deathPosition);
		cgs.be.teamMarkers.deathPositionValid = qtrue;
	}

	/* Send team task mark */
	if (shouldSendMark)
	{
		CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_PLAYERDEAD, position, qtrue);
	}
}

/*
=================
TeamTask command definitions
=================
*/

/* Global teamtask commands array */
teamTaskCommand_t teamTaskCommands[] =
{
	{ "attack", NULL, TEAMTASK_OFFENSE, qfalse, qtrue, CG_TeamTaskMarkOffense, "tAttack" },
	{ "defence", NULL, TEAMTASK_DEFENCE, qfalse, qtrue, CG_TeamTaskMarkDefence, "tDefence" },
	{ "watch", NULL, TEAMTASK_PATROL, qfalse, qtrue, CG_TeamTaskMarkWatch, "tWatch" },
	{ "yes", NULL, TEAMTASK_YES, qtrue, qfalse, CG_TeamTaskMarkYes, "tYes" },
	{ "no", NULL, TEAMTASK_NO, qtrue, qfalse, CG_TeamTaskMarkNo, "tNo" },
	{ "location", "position", TEAMTASK_LOCATION, qfalse, qtrue, CG_TeamTaskMarkLocation, "tLocation" },
	{ "here", NULL, TEAMTASK_HERE_ALIVE, qtrue, qtrue, CG_TeamTaskMarkHere, "tHere" },
	{ "target", NULL, TEAMTASK_TARGET, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "redarmor", NULL, TEAMTASK_REDARMOR, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "megahealth", NULL, TEAMTASK_MEGAHEALTH, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "medic", NULL, TEAMTASK_MEDIC, qtrue, qfalse, CG_TeamTaskMarkMedic, "tMedic" }
};

#define NUM_TEAMTASK_COMMANDS (sizeof(teamTaskCommands) / sizeof(teamTaskCommands[0]))

/*
=================
CG_GetTeamTaskShaderByType

Get shader handle for teamtask type
Returns 0 if not found or not loaded
=================
*/
qhandle_t CG_GetTeamTaskShaderByType(teamtask_t taskType)
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
		case TEAMTASK_HERE_ALIVE:
			return cgs.media.teamTasksLocation;  // Use existing location shader for alive player
		case TEAMTASK_HERE_DEAD:
			return cgs.media.teamTasksDead;      // Use existing skull shader for death location
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

