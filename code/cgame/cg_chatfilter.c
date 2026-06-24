#include "cg_local.h"
#include "../qcommon/qcommon.h"

typedef struct chatFilterTableNode_s
{
	char* name;
	struct chatFilterTableNode_s* next;
	struct chatFilterTableNode_s* prev;
} chatFilterTableNode_t;

#define CG_CHATFILTER_MAX 128

static chatFilterTableNode_t filterNames[CG_CHATFILTER_MAX];

static chatFilterTableNode_t* CG_ChatfilterFindName(const char* name)
{
	unsigned long hash;
	chatFilterTableNode_t* target;
	qboolean isNotFound = qfalse;

	hash = Com_GenerateHashValue(name, CG_CHATFILTER_MAX);

	target = &filterNames[hash];

	while (target && (isNotFound = Q_stricmp(name, target->name)))
	{
		target = target->next;
	}
	if (!isNotFound)
	{
		return target; //found
	}

	return NULL; //not found
}

void CG_ChatfilterDump(void)
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
			          CG_ChatfilterIsNameMuted(cgs.clientinfo[p].name_original) ? "^1Yes" : "^2No ",
			          cgs.clientinfo[p].name_clean, nameCodesBuf);
		}
	}
}

qboolean CG_ChatfilterIsNameMuted(const char* name)
{
	return CG_ChatfilterFindName(name) != NULL;
}

void CG_ChatfilterDeleteName(const char* name)
{
	unsigned long hash;
	chatFilterTableNode_t* target;
	qboolean isNotFound = qfalse;

	hash = Com_GenerateHashValue(name, CG_CHATFILTER_MAX);

	target = &filterNames[hash];

	while (target && (isNotFound = Q_stricmp(name, target->name)))
	{
		target = target->next;
	}
	if (isNotFound) return;

	if (target == &filterNames[hash])
	{
		//root node
		if (target->name) Z_Free(target->name);
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

void CG_ChatfilterAddName(const char* name)
{
	unsigned long hash;
	chatFilterTableNode_t* target;
	int len = strlen(name);

	if (CG_ChatfilterFindName(name))
	{
		return;
	}

	hash = Com_GenerateHashValue(name, CG_CHATFILTER_MAX);

	target = &filterNames[hash];

	while (target->name)
	{
		if (!target->next)
		{
			target->next = Z_Malloc(sizeof(chatFilterTableNode_t));
			OSP_MEMORY_CHECK(target->next);
			memset(target->next, 0, sizeof(chatFilterTableNode_t));
		}
		target->next->prev = target;
		target = target->next;
	}

	target->name = Z_Malloc(len + 1);
	OSP_MEMORY_CHECK(target->name);

	Q_strncpyz(target->name, name, len + 1);
}

qboolean CG_ChatExtractSenderName(const char* message, char* name, int nameSize)
{
	const char* splitter;
	const char* start;
	int len;

	if (!message || !name || nameSize <= 0)
	{
		return qfalse;
	}

	start = message;
	if (start[0] == 25 && (start[1] == '(' || start[1] == '['))
	{
		start += 2;
	}

	splitter = strchr(start, 25);
	if (!splitter)
	{
		return qfalse;
	}

	splitter -= 2;
	len = splitter - start;
	if (len <= 0 || len >= nameSize)
	{
		return qfalse;
	}

	memcpy(name, start, len);
	name[len] = 0;
	return qtrue;
}

static int CG_ChatClientNumFromBracketId(const char* senderName)
{
	const char* rb;
	int clientNum;

	if (!senderName || senderName[0] != '[')
	{
		return -1;
	}

	rb = strchr(senderName, ']');
	if (!rb)
	{
		return -1;
	}

	clientNum = atoi(senderName + 1);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || !cgs.clientinfo[clientNum].infoValid)
	{
		return -1;
	}

	return clientNum;
}

int CG_ChatClientNumFromSenderName(const char* senderName)
{
	int i;
	char cleanSender[MAX_QPATH];
	char cleanCompare[MAX_QPATH];
	clientInfo_t* ci;

	if (!senderName || !senderName[0])
	{
		return -1;
	}

	i = CG_ChatClientNumFromBracketId(senderName);
	if (i >= 0)
	{
		return i;
	}

	Q_strncpyz(cleanSender, senderName, sizeof(cleanSender));
	CG_RemoveChatEscapeCharAll(cleanSender);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ci = &cgs.clientinfo[i];
		if (!ci->infoValid)
		{
			continue;
		}

		if (Q_stricmp(ci->name, senderName) == 0 || Q_stricmp(ci->name_original, senderName) == 0)
		{
			return i;
		}

		Q_strncpyz(cleanCompare, ci->name, sizeof(cleanCompare));
		CG_RemoveChatEscapeCharAll(cleanCompare);
		if (Q_stricmp(cleanCompare, cleanSender) == 0)
		{
			return i;
		}

		Q_strncpyz(cleanCompare, ci->name_original, sizeof(cleanCompare));
		CG_RemoveChatEscapeCharAll(cleanCompare);
		if (Q_stricmp(cleanCompare, cleanSender) == 0)
		{
			return i;
		}
	}

	return -1;
}

int CG_ChatResolveSenderClientNum(const char* rawMessage, int serverClientNum)
{
	char senderName[MAX_QPATH];

	if (serverClientNum >= 0 && serverClientNum < MAX_CLIENTS && cgs.clientinfo[serverClientNum].infoValid)
	{
		return serverClientNum;
	}

	if (!CG_ChatExtractSenderName(rawMessage, senderName, sizeof(senderName)))
	{
		return -1;
	}

	CG_RemoveChatEscapeChar(senderName);
	return CG_ChatClientNumFromSenderName(senderName);
}

messageAllowed_t CG_ChatCheckMessageAllowed(const char* message)
{
	const chatFilterTableNode_t* target;
	char name[MAX_QPATH];

	if (!CG_ChatExtractSenderName(message, name, sizeof(name)))
	{
		return MESSAGE_ALLOWED_OTHER;
	}

	target = CG_ChatfilterFindName(name);

	return target == NULL ? MESSAGE_ALLOWED_PLAYER : MESSAGE_NOTALLOWED;
}

void CG_ChatfilterLoadFile(const char* filename)
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
			//add name
			CG_ChatfilterAddName(name);
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


void CG_ChatfilterSaveFile(const char* filename)
{
	int rc;
	char path[MAX_QPATH];
	fileHandle_t filterFileHandle;
	int i;
	chatFilterTableNode_t* ptr;

	Com_sprintf(path, MAX_QPATH, "%s.txt", filename);

	rc = trap_FS_FOpenFile(path, &filterFileHandle, FS_WRITE);
	if (rc < 0 || !filterFileHandle)
	{
		CG_Printf("^1chat filter file %s not found\n", path);
		return;
	}


	for (i = 0; i < CG_CHATFILTER_MAX; ++i)
	{
		ptr = &filterNames[i];

		while (ptr->name)
		{
			trap_FS_Write(ptr->name, strlen(ptr->name), filterFileHandle);
			trap_FS_Write("\n", 1, filterFileHandle);
			if (ptr->next == NULL) break;
			ptr = ptr->next;
		}
	}

	trap_FS_FCloseFile(filterFileHandle);
}
