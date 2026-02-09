/*
===========================================================================
cg_teamtasks_utils.c

Team task utility functions and command definitions

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
// cg_teamtasks_utils.c -- team task utility functions and command definitions

#include "cg_local.h"

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

	/* Check conditions: single gate for teamtask + warmup/game/event */
	if (!cgs.teamMarkers.teamTaskEnabled || cg.warmup || target != cg.clientNum || cgs.gametype < GT_TEAM || isUnfreezeEvent)
		return;

	/* Get player entity */
	cent = &cg_entities[target];
	if (!cent)
	{
		return;
	}

	/* Get death position */
	VectorCopy(cent->lerpOrigin, position);
	position[2] += 6.0f;

	/* Save death coordinates */
	VectorCopy(position, cgs.teamMarkers.deathPosition);
	cgs.teamMarkers.deathPositionValid = qtrue;

	/* Send team task mark */
	CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_PLAYERDEAD, position, qtrue);
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
	{ "watch", NULL, TEAMTASK_WATCH, qfalse, qtrue, CG_TeamTaskMarkWatch, "tWatch" },
	{ "yes", NULL, TEAMTASK_YES, qtrue, qfalse, CG_TeamTaskMarkYes, "tYes" },
	{ "no", NULL, TEAMTASK_NO, qtrue, qfalse, CG_TeamTaskMarkNo, "tNo" },
	{ "location", "position", TEAMTASK_LOCATION, qfalse, qtrue, CG_TeamTaskMarkLocation, "tLocation" },
	{ "here", NULL, TEAMTASK_HERE_ALIVE, qtrue, qtrue, CG_TeamTaskMarkHere, "tHere" },
	{ "hereflag", NULL, TEAMTASK_HERE_FLAG, qtrue, qtrue, NULL, NULL }, /* Auto-selected by tHere when player has flag */
	{ "warn", NULL, TEAMTASK_WARN_ALIVE, qfalse, qtrue, CG_TeamTaskMarkWarn, "tWarn" },
	{ "target", NULL, TEAMTASK_TARGET, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "redarmor", NULL, TEAMTASK_REDARMOR, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "megahealth", NULL, TEAMTASK_MEGAHEALTH, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "quaddamage", NULL, TEAMTASK_QUADDAMAGE, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "regeneration", NULL, TEAMTASK_REGENERATION, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "battlesuit", NULL, TEAMTASK_BATTLESUIT, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "haste", NULL, TEAMTASK_HASTE, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "yellowarmor", NULL, TEAMTASK_YELLOWARMOR, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "health", NULL, TEAMTASK_HEALTH, qfalse, qfalse, NULL, NULL }, /* Auto-selected by smart mode */
	{ "medic", NULL, TEAMTASK_MEDIC, qtrue, qfalse, CG_TeamTaskMarkMedic, "tMedic" },
	{ "pickup", NULL, TEAMTASK_NONE, qfalse, qtrue, CG_TeamTaskMarkPickup, "tPickup" }
};

#define NUM_TEAMTASK_COMMANDS (sizeof(teamTaskCommands) / sizeof(teamTaskCommands[0]))

/*
=================
CG_GetTeamTaskShaderByType

Get shader handle for teamtask type
Returns 0 if not found or not loaded
=================
*/
/*
=================
CG_GetPickupShader

Get shader for pickup command based on current pickup item state
Returns pickupEmpty if no item picked up or expired, otherwise returns corresponding pickup icon
=================
*/
qhandle_t CG_GetPickupShader(void)
{
	int itemType;

	/* Check if we have a valid pickup item (within 5 seconds) */
	if (cgs.teamMarkers.pickupItemType == 0)
	{
		return cgs.media.teamTasksPickupEmpty;
	}

	/* Check if pickup item is still valid (5 seconds = 5000ms) */
	if ((cg.time - cgs.teamMarkers.pickupItemTime) > 5000)
	{
		return cgs.media.teamTasksPickupEmpty;
	}

	itemType = cgs.teamMarkers.pickupItemType;
	if (itemType == TEAMTASK_MEGAHEALTH)
	{
		return cgs.media.teamTasksMegahealthPickup;
	}
	else if (itemType == TEAMTASK_REDARMOR)
	{
		return cgs.media.teamTasksRedarmorPickup;
	}
	else if (itemType == TEAMTASK_YELLOWARMOR)
	{
		return cgs.media.teamTasksYellowarmorPickup;
	}

	return cgs.media.teamTasksPickupEmpty;
}

qhandle_t CG_GetTeamTaskShaderByType(teamtask_t taskType)
{
	switch (taskType)
	{
		case TEAMTASK_OFFENSE:
			return cgs.media.teamTasksAttack;
		case TEAMTASK_DEFENCE:
			return cgs.media.teamTasksDefence;
		case TEAMTASK_WATCH:
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
		case TEAMTASK_HERE_FLAG:
			return cgs.media.teamTasksFlagcarrier;   // Use flagcarrier shader (will be colored based on flag type)
		case TEAMTASK_WARN_ALIVE:
			return cgs.media.teamTasksWarning;
		case TEAMTASK_WARN_DEAD:
			return cgs.media.teamTasksWarning;
		case TEAMTASK_REDFLAG:
		case TEAMTASK_BLUEFLAG:
			return cgs.media.teamTasksFlag;
		case TEAMTASK_REDFLAG_CARRIER:
		case TEAMTASK_BLUEFLAG_CARRIER:
			return cgs.media.teamTasksFlagcarrier;
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


/*
=================
CG_IsTeamTaskSupported

Check if server supports team task
=================
*/
qboolean CG_IsTeamTaskSupported(void)
{
	return cgs.teamMarkers.supportedServer;
}

/*
=================
CG_ClearRoundSpecificTeamTaskMarks

Remove specific team task marks when round changes:
FROZEN_ALLY, FROZEN_ENEMY, HERE_DEAD, WARN_DEAD, PLAYERDEAD.
Removal is done in CG_TeamTaskUpdateFrame (single place).
=================
*/
void CG_ClearRoundSpecificTeamTaskMarks(void)
{
	cgs.teamMarkers.packRoundNextFrame = qtrue;
}

/*
=================
CG_ClearClientHereDeadMarks

Clears HERE_DEAD team task marks for a specific client (unfreeze).
Removal is done in CG_TeamTaskUpdateFrame (single place).
=================
*/
void CG_ClearClientHereDeadMarks(int clientNum)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;
	if (clientNum < 32)
		cgs.teamMarkers.packHereDeadClientMask0 |= (1 << clientNum);
	else
		cgs.teamMarkers.packHereDeadClientMask1 |= (1 << (clientNum - 32));
}
