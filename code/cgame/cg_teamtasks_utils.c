/*
===========================================================================
cg_teamtasks_utils.c

Team task utility functions and command definitions

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com

This file contains original work created by diwoc.
It is included in the OSP2-BE modification for Quake III Arena and is
distributed under the terms of the project's primary license.
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

/*
=================
Token bucket anti-flood (team task send rate limits)
=================
*/

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

/*
=================
Team task filter (mute team tasks from specific players)
=================
*/

typedef struct teamTaskFilterTableNode_s
{
	char* name;
	struct teamTaskFilterTableNode_s* next;
	struct teamTaskFilterTableNode_s* prev;
} teamTaskFilterTableNode_t;

#define CG_TEAMTASKFILTER_MAX 128

static teamTaskFilterTableNode_t filterNames[CG_TEAMTASKFILTER_MAX];

static teamTaskFilterTableNode_t* CG_TeamTaskFilterFindName(const char* name)
{
	unsigned long hash;
	teamTaskFilterTableNode_t* target;
	qboolean isNotFound = qfalse;

	hash = Com_GenerateHashValue(name, CG_TEAMTASKFILTER_MAX);

	target = &filterNames[hash];

	while (target && (isNotFound = Q_stricmp(name, target->name)))
	{
		target = target->next;
	}
	if (!isNotFound)
	{
		return target;
	}

	return NULL;
}

void CG_TeamTaskFilterDump(void)
{
	int p;
	char nameCodesBuf[MAX_QPATH * 2];

	CG_Printf("^7| ^1%2s ^7| ^5%5s | ^5%32s | ^5%s\n", "ID", "Muted", "Player name", "Player name raw");
	CG_Printf("^7------------------------------------------------------------------------------\n");
	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (cgs.clientinfo[p].infoValid)
		{
			CG_StringMakeEscapeCharRAW(cgs.clientinfo[p].name_original, nameCodesBuf, sizeof(nameCodesBuf));
			CG_Printf("^7| ^1%2d ^7|  %s  ^7| %32s | %s\n", p,
			          CG_TeamTaskFilterIsNameMuted(cgs.clientinfo[p].name_original) ? "^1Yes" : "^2No ",
			          cgs.clientinfo[p].name_clean, nameCodesBuf);
		}
	}
}

qboolean CG_TeamTaskFilterIsNameMuted(const char* name)
{
	return CG_TeamTaskFilterFindName(name) != NULL;
}

void CG_TeamTaskFilterDeleteName(const char* name)
{
	unsigned long hash;
	teamTaskFilterTableNode_t* target;
	qboolean isNotFound = qfalse;

	hash = Com_GenerateHashValue(name, CG_TEAMTASKFILTER_MAX);

	target = &filterNames[hash];

	while (target && (isNotFound = Q_stricmp(name, target->name)))
	{
		target = target->next;
	}
	if (isNotFound)
	{
		return;
	}

	if (target == &filterNames[hash])
	{
		if (target->name)
		{
			Z_Free(target->name);
		}
		target->name = NULL;
		if (target->next)
		{
			target->name = target->next->name;
			target->next = target->next->next;
		}
	}
	else
	{
		if (target->prev)
		{
			target->prev->next = target->next;
		}
		if (target->next)
		{
			target->next->prev = target->prev;
		}
		Z_Free(target->name);
		Z_Free(target);
	}
}

void CG_TeamTaskFilterAddName(const char* name)
{
	unsigned long hash;
	teamTaskFilterTableNode_t* target;
	int len;

	if (CG_TeamTaskFilterFindName(name))
	{
		return;
	}

	len = strlen(name);

	hash = Com_GenerateHashValue(name, CG_TEAMTASKFILTER_MAX);

	target = &filterNames[hash];

	while (target->name)
	{
		if (!target->next)
		{
			target->next = Z_Malloc(sizeof(teamTaskFilterTableNode_t));
			OSP_MEMORY_CHECK(target->next);
			memset(target->next, 0, sizeof(teamTaskFilterTableNode_t));
		}
		target->next->prev = target;
		target = target->next;
	}

	target->name = Z_Malloc(len + 1);
	OSP_MEMORY_CHECK(target->name);

	Q_strncpyz(target->name, name, len + 1);
}

void CG_TeamTaskFilterLoadFile(const char* filename)
{
	int rc;
	char path[MAX_QPATH];
	fileHandle_t filterFileHandle;
	char* fileContent;
	const char* ptr;
	char name[MAX_QPATH];
	int namesCounter = 0;

	Com_sprintf(path, MAX_QPATH, "%s.txt", filename);

	rc = trap_FS_FOpenFile(path, &filterFileHandle, FS_READ);
	if (rc < 0 || !filterFileHandle)
	{
		return;
	}

	if (rc == 0)
	{
		trap_FS_FCloseFile(filterFileHandle);
		return;
	}

	fileContent = Z_Malloc(rc + 1);
	OSP_MEMORY_CHECK(fileContent);

	trap_FS_Read(fileContent, rc, filterFileHandle);
	trap_FS_FCloseFile(filterFileHandle);
	fileContent[rc] = 0;

	ptr = fileContent;
	do
	{
		ptr = CG_LoadLine(ptr, &name[0], sizeof(name));
		if (name[0])
		{
			CG_TeamTaskFilterAddName(name);
			++namesCounter;
		}
	}
	while (*ptr);

	if (namesCounter > 0)
	{
		CG_Printf("^3>>> (%s):^5 %i name%s loaded.\n", path, namesCounter, namesCounter > 1 ? "s" : "");
	}

	Z_Free(fileContent);
}

void CG_TeamTaskFilterSaveFile(const char* filename)
{
	int rc;
	char path[MAX_QPATH];
	fileHandle_t filterFileHandle;
	int i;
	teamTaskFilterTableNode_t* ptr;

	Com_sprintf(path, MAX_QPATH, "%s.txt", filename);

	rc = trap_FS_FOpenFile(path, &filterFileHandle, FS_WRITE);
	if (rc < 0 || !filterFileHandle)
	{
		CG_Printf("^1team task filter file %s not found\n", path);
		return;
	}

	for (i = 0; i < CG_TEAMTASKFILTER_MAX; ++i)
	{
		ptr = &filterNames[i];

		while (ptr->name)
		{
			trap_FS_Write(ptr->name, strlen(ptr->name), filterFileHandle);
			trap_FS_Write("\n", 1, filterFileHandle);
			if (ptr->next == NULL)
			{
				break;
			}
			ptr = ptr->next;
		}
	}

	trap_FS_FCloseFile(filterFileHandle);
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

	/* Check conditions: single gate for teamtask + warmup/game/event */
	if (!cg.teamMarkers.teamTaskEnabled || cg.warmup || target != cg.clientNum || cgs.gametype < GT_TEAM || isUnfreezeEvent)
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
	VectorCopy(position, cg.teamMarkers.deathPosition);
	cg.teamMarkers.deathPositionValid = qtrue;

	/* Send team task mark */
	CG_SendTeamTaskMarkAtPosition(0, TEAMTASK_PLAYERDEAD, position, qtrue);
}

/*
=================
CG_TeamTaskStoreAllyDeathPosition

Save death coordinates from incoming TEAMTASK_PLAYERDEAD mark
=================
*/
void CG_TeamTaskStoreAllyDeathPosition(int clientNum, vec3_t coord)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	VectorCopy(coord, cg.teamMarkers.allyDeathPosition[clientNum]);
	cg.teamMarkers.allyDeathPositionValid[clientNum] = qtrue;
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
CG_GetPickupShader

Get shader for pickup command based on current pickup item state
Returns pickupEmpty if no item picked up or expired, otherwise returns corresponding pickup icon
=================
*/
qhandle_t CG_GetPickupShader(void)
{
	int itemType;

	/* Check if we have a valid pickup item (within 5 seconds) */
	if (cg.teamMarkers.pickupItemType == 0)
	{
		return cgs.media.teamTasksPickupEmpty;
	}

	/* Check if pickup item is still valid (5 seconds = 5000ms) */
	if ((cg.time - cg.teamMarkers.pickupItemTime) > 5000)
	{
		return cgs.media.teamTasksPickupEmpty;
	}

	itemType = cg.teamMarkers.pickupItemType;
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
	return cg.teamMarkers.supportedServer;
}

/*
=================
CG_NotifyTeamTaskServerSupportOnMapLoad

After map load: warn if team gametype, cg_teamTask enabled, server did not
disable team tasks, but server lacks team task protocol (no \\tt in configstring).
=================
*/
void CG_NotifyTeamTaskServerSupportOnMapLoad(void)
{
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}
	if (!cg_teamTask.integer)
	{
		return;
	}
	if (cgs.beDisableFeatures & CG_BE_TEAMTASKS)
	{
		return;
	}
	if (CG_IsTeamTaskSupported())
	{
		return;
	}
	CG_PrintOnce("[^9OSP2-BE^7] ^1Team tasks not supported on this server!\n");
}

/*
=================
CG_TeamTaskShouldUseDeadHereWarn

True when tHere/tWarn must use death position, not crosshair or live origin.
Uses local client state: cg.snap->ps follows spectated player, so CG_IS_DEAD()
alone is wrong while switching follow targets.
=================
*/
qboolean CG_TeamTaskShouldUseDeadHereWarn(void)
{
	if (!CG_IS_LOCAL_CLIENT_ON_SCREEN())
	{
		return qtrue;
	}
	if (CG_IS_LOCAL_CLIENT_SPECTATOR())
	{
		return qtrue;
	}
	if (CG_IsClientFreeFloat())
	{
		return qtrue;
	}
	if (CG_IS_DEAD())
	{
		return qtrue;
	}
	if (cg.snap && cg.snap->ps.clientNum == cg.clientNum)
	{
		if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
		{
			return qtrue;
		}
		if (cg.snap->ps.pm_type == PM_SPECTATOR)
		{
			return qtrue;
		}
	}
	return qfalse;
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
	cg.teamMarkers.packRoundNextFrame = qtrue;
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
		cg.teamMarkers.packHereDeadClientMask0 |= (1 << clientNum);
	else
		cg.teamMarkers.packHereDeadClientMask1 |= (1 << (clientNum - 32));
}

/*
=================
Team task pack key and bit packing (CG_TeamTaskPack / CG_TeamTaskUnpack)
=================
*/

static int CG_TeamTaskPackKeyFromClient(int clientID)
{
	int key;
	clientInfo_t* ci;
	int headModelHash;
	int i;
	const char* headModelName;

	ci = &cgs.clientinfo[clientID];
	if (!ci->infoValid)
	{
		return clientID * 0x4A3B2C1D;
	}

	headModelName = ci->originalHeadModel[0] != '\0' ? ci->originalHeadModel : ci->headModelName;
	headModelHash = 0;
	for (i = 0; headModelName[i] != '\0' && i < MAX_QPATH; i++)
	{
		headModelHash = (headModelHash * 31) + (int)headModelName[i];
	}

	key = clientID;
	key = (key << 16) | ((key >> 16) ^ headModelHash);
	key = key ^ (headModelHash << 8);

	if (key == 0)
	{
		key = 0x12345678;
	}

	return key;
}

static int CG_TeamTaskPermutePack(int toClient, int templateID, int x, int y, int z, int scheme)
{
	int packed;
	int coord1;
	int coord2;
	int coord3;

	switch (scheme & 0x07)
	{
		case 0:
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
		case 1:
			coord1 = x;
			coord2 = z;
			coord3 = y;
			break;
		case 2:
			coord1 = y;
			coord2 = x;
			coord3 = z;
			break;
		case 3:
			coord1 = y;
			coord2 = z;
			coord3 = x;
			break;
		case 4:
			coord1 = z;
			coord2 = x;
			coord3 = y;
			break;
		case 5:
			coord1 = z;
			coord2 = y;
			coord3 = x;
			break;
		case 6:
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
		case 7:
			coord1 = x;
			coord2 = z;
			coord3 = y;
			break;
		default:
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
	}

	packed = ((toClient & 0x01) << 31) |
	         ((templateID & 0x1F) << 26) |
	         ((coord1 & 0x7F) << 19) |
	         ((coord2 & 0x7F) << 12) |
	         ((coord3 & 0x7F) << 5);

	return packed;
}

static void CG_TeamTaskExtractCoords(int packed, int* coord1, int* coord2, int* coord3)
{
	*coord1 = (packed >> 19) & 0x7F;
	*coord2 = (packed >> 12) & 0x7F;
	*coord3 = (packed >> 5) & 0x7F;
}

static void CG_TeamTaskPermuteUnpack(int packed, int scheme, int* toClient, int* templateID, int* x, int* y, int* z)
{
	int coord1;
	int coord2;
	int coord3;

	switch (scheme & 0x07)
	{
		case 0:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
		case 1:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord3;
			*z = coord2;
			break;
		case 2:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord2;
			*y = coord1;
			*z = coord3;
			break;
		case 3:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord3;
			*y = coord1;
			*z = coord2;
			break;
		case 4:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord2;
			*y = coord3;
			*z = coord1;
			break;
		case 5:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord3;
			*y = coord2;
			*z = coord1;
			break;
		case 6:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
		case 7:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord3;
			*z = coord2;
			break;
		default:
			CG_TeamTaskExtractCoords(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
	}

	*toClient = (packed >> 31) & 0x01;
	*templateID = (packed >> 26) & 0x1F;
}

int CG_TeamTaskPack(int toClient, int templateID,
                    int x, int y, int z)
{
	int packed;
	int key;
	int scrambled;
	int senderClientID;
	int senderIDPacked;
	int verificationBit;
	int scheme;

	senderClientID = cg.clientNum;

	scheme = ((senderClientID * 17) ^ (senderClientID >> 2) ^ (senderClientID << 3)) & 0x07;

	packed = CG_TeamTaskPermutePack(toClient, templateID, x, y, z, scheme);

	key = CG_TeamTaskPackKeyFromClient(senderClientID);

	scrambled = packed & 0xFFFFFFE0;
	scrambled = scrambled ^ (key & 0xFFFFFFE0);

	senderIDPacked = ((senderClientID + 52) & 0x0F);
	senderIDPacked = senderIDPacked ^ ((key >> 5) & 0x0F);

	verificationBit = ((key >> 11) ^ (key >> 17) ^
	                   (scrambled >> 11) ^ (scrambled >> 17) ^
	                   senderIDPacked) & 0x01;

	return scrambled | (verificationBit << 4) | senderIDPacked;
}

void CG_TeamTaskUnpack(int packed, int senderClientID, int* toClient, int* templateID,
                       int* x, int* y, int* z)
{
	int scrambled;
	int senderIDPacked;
	int key;
	int plainPacked;
	int expectedVerificationBit;
	int verificationBit;
	int scheme;

	verificationBit = (packed >> 4) & 0x01;
	senderIDPacked = packed & 0x0F;

	scrambled = packed & 0xFFFFFFE0;

	if (scrambled == 0)
	{
		*toClient = 0;
		*templateID = 0;
		*x = 0;
		*y = 0;
		*z = 0;
		return;
	}

	if (senderClientID >= 0 && senderClientID < MAX_CLIENTS && cgs.clientinfo[senderClientID].infoValid)
	{
		int expectedSenderIDPacked;

		key = CG_TeamTaskPackKeyFromClient(senderClientID);

		expectedSenderIDPacked = ((senderClientID + 52) & 0x0F);
		expectedSenderIDPacked = expectedSenderIDPacked ^ ((key >> 5) & 0x0F);

		if (expectedSenderIDPacked != senderIDPacked)
		{
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}

		expectedVerificationBit = ((key >> 11) ^ (key >> 17) ^
		                           (scrambled >> 11) ^ (scrambled >> 17) ^
		                           senderIDPacked) & 0x01;
		if (verificationBit != expectedVerificationBit)
		{
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}
	}
	else
	{
		int i;
		int found;

		found = 0;
		for (i = 0; i < MAX_CLIENTS && !found; i++)
		{
			int expectedSenderIDPacked;

			if (!cgs.clientinfo[i].infoValid)
			{
				continue;
			}
			key = CG_TeamTaskPackKeyFromClient(i);

			expectedSenderIDPacked = ((i + 52) & 0x0F);
			expectedSenderIDPacked = expectedSenderIDPacked ^ ((key >> 5) & 0x0F);

			if (expectedSenderIDPacked == senderIDPacked)
			{
				expectedVerificationBit = ((key >> 11) ^ (key >> 17) ^
				                           (scrambled >> 11) ^ (scrambled >> 17) ^
				                           senderIDPacked) & 0x01;
				if (verificationBit == expectedVerificationBit)
				{
					senderClientID = i;
					found = 1;
				}
			}
		}

		if (!found)
		{
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}
	}

	plainPacked = scrambled ^ (key & 0xFFFFFFE0);

	scheme = ((senderClientID * 17) ^ (senderClientID >> 2) ^ (senderClientID << 3)) & 0x07;

	CG_TeamTaskPermuteUnpack(plainPacked, scheme, toClient, templateID, x, y, z);
}
