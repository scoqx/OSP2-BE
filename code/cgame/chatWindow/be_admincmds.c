/*
===========================================================================
be_admincmds.c — Server-pushed ! admin command list for BE chat/console AC.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"

beAdminCmdList_t beAdminCmdList;

static char CG_BEAdminCmd_ToLower(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return (char)(c - 'A' + 'a');
	}

	return c;
}

static int CG_BEAdminCmd_ICmp(const char* a, const char* b)
{
	int i;
	char ca;
	char cb;

	for (i = 0; a[i] && b[i]; ++i)
	{
		ca = CG_BEAdminCmd_ToLower(a[i]);
		cb = CG_BEAdminCmd_ToLower(b[i]);
		if (ca != cb)
		{
			return ca - cb;
		}
	}

	return (int)CG_BEAdminCmd_ToLower(a[i]) - (int)CG_BEAdminCmd_ToLower(b[i]);
}

static qboolean CG_BEAdminCmd_PrefixMatch(const char* cmd, const char* prefix, int prefixLen)
{
	int i;

	for (i = 0; i < prefixLen; ++i)
	{
		if (!cmd[i])
		{
			return qfalse;
		}
		if (CG_BEAdminCmd_ToLower(cmd[i]) != CG_BEAdminCmd_ToLower(prefix[i]))
		{
			return qfalse;
		}
	}

	return qtrue;
}

static int CG_BEAdminCmd_LowerBound(const char* prefix)
{
	int lo;
	int hi;
	int mid;

	lo = 0;
	hi = beAdminCmdList.count;

	while (lo < hi)
	{
		mid = (lo + hi) / 2;
		if (CG_BEAdminCmd_ICmp(beAdminCmdList.cmds[mid], prefix) < 0)
		{
			lo = mid + 1;
		}
		else
		{
			hi = mid;
		}
	}

	return lo;
}

static qboolean CG_BEAdminCmd_ValidToken(const char* token)
{
	int i;
	int start;

	if (!token || !token[0])
	{
		return qfalse;
	}

	start = 0;
	if (token[0] == '!')
	{
		start = 1;
	}

	for (i = start; token[i]; ++i)
	{
		if (!((token[i] >= 'a' && token[i] <= 'z') ||
		        (token[i] >= 'A' && token[i] <= 'Z') ||
		        (token[i] >= '0' && token[i] <= '9') ||
		        (token[i] == '_')))
		{
			return qfalse;
		}
	}

	return token[start] != '\0';
}

void CG_BEAdminCmd_Clear(void)
{
	memset(&beAdminCmdList, 0, sizeof(beAdminCmdList));
	CG_BEChat_CmdAcClear();
}

void CG_BEAdminCmd_Register(const char* token)
{
	int ins;
	int cmp;

	if (!CG_BEAdminCmd_ValidToken(token))
	{
		return;
	}

	if (beAdminCmdList.count >= BE_ADMIN_CMD_MAX)
	{
		return;
	}

	for (ins = 0; ins < beAdminCmdList.count; ++ins)
	{
		cmp = CG_BEAdminCmd_ICmp(token, beAdminCmdList.cmds[ins]);
		if (cmp == 0)
		{
			return;
		}
		if (cmp < 0)
		{
			break;
		}
	}

	if (beAdminCmdList.count > ins)
	{
		memmove(beAdminCmdList.cmds[ins + 1], beAdminCmdList.cmds[ins],
		        (beAdminCmdList.count - ins) * BE_ADMIN_CMD_LEN);
	}

	Q_strncpyz(beAdminCmdList.cmds[ins], token, BE_ADMIN_CMD_LEN);
	beAdminCmdList.count++;
}

void CG_BEAdminCmd_FillEntries(const char* prefix, int prefixLen)
{
	int i;
	int start;
	const char* cmd;

	beChatCmdAc.count = 0;

	if (beAdminCmdList.count < 1)
	{
		return;
	}

	if (!prefix || prefixLen < 1 || prefix[0] != '!')
	{
		return;
	}

	start = CG_BEAdminCmd_LowerBound(prefix);

	for (i = start; i < beAdminCmdList.count && beChatCmdAc.count < BE_CHAT_CMD_AC_MAX; ++i)
	{
		cmd = beAdminCmdList.cmds[i];

		if (!CG_BEAdminCmd_PrefixMatch(cmd, prefix, prefixLen))
		{
			continue;
		}

		Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], cmd,
		           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
		beChatCmdAc.count++;
	}

	if (beChatCmdAc.sel >= beChatCmdAc.count)
	{
		beChatCmdAc.sel = 0;
	}
}

qboolean CG_BEAdminCmd_IsTokenValid(const char* token)
{
	return CG_BEAdminCmd_ValidToken(token);
}
