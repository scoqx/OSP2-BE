/*
===========================================================================
cg_teamtaskfilter.c

Team task filter (mute team tasks from specific players)

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
// cg_teamtaskfilter.c -- team task filter (mute team tasks from specific players)

#include "cg_local.h"
#include "../qcommon/qcommon.h"

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
		return target; /* found */
	}

	return NULL; /* not found */
}

void CG_TeamTaskFilterDump(void)
{
	int p;

	CG_Printf("^7| ^1%2s ^7| ^5%5s | ^5%32s | ^5%s\n", "ID", "Muted", "Player name", "Player name raw");
	CG_Printf("^7------------------------------------------------------------------------------\n");
	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (cgs.clientinfo[p].infoValid)
		{
			CG_Printf("^7| ^1%2d ^7|  %s  ^7| %32s | %s\n", p,
			          CG_TeamTaskFilterIsNameMuted(cgs.clientinfo[p].name_original) ? "^1Yes" : "^2No ",
			          cgs.clientinfo[p].name_clean, cgs.clientinfo[p].name_codes);
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
		/* root node */
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
		CG_Printf("^1team task filter file %s not found\n", path);
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
			/* add name */
			CG_TeamTaskFilterAddName(name);
			++namesCounter;
		}
	}
	while (*ptr);

	CG_Printf("^3>>> (%s):^5 %i name%s loaded.\n", path, namesCounter, namesCounter > 1 ? "s" : "");

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

