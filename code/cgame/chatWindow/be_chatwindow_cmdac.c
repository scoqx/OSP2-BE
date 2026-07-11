/*
===========================================================================
cg_be_chatwindow_cmdac.c — Slash commands, tell/mute/reply, command autocomplete.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

static qboolean CG_BEChat_LineCmdChannelOpen(void);
static qboolean CG_BEChat_LineSlashCmdAfter(const char* buf, const char* cmdWord, int* outNameStart);
static qboolean CG_BEChat_CmdAcRefreshMuteUnmutePickDigits(beChatInput_t* edit, const char* buf,
        int cursor);
static qboolean CG_BEChat_CmdAcRefreshMuteUnmuteLine(beChatInput_t* edit, const char* buf, int cursor);

void CG_BEChat_CmdAcClear(void)
{
	memset(&beChatCmdAc, 0, sizeof(beChatCmdAc));
}
static void CG_BEChat_CmdAcFillSlashCommandEntries(const char* prefix, int prefixLen)
{
	if (prefixLen <= (int)strlen(BE_CHAT_CMD_TELL) &&
	        !Q_stricmpn(BE_CHAT_CMD_TELL, prefix, prefixLen))
	{
		Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], BE_CHAT_CMD_TELL,
		           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
		beChatCmdAc.count++;
	}
	if (prefixLen <= (int)strlen(BE_CHAT_CMD_REPLY) &&
	        !Q_stricmpn(BE_CHAT_CMD_REPLY, prefix, prefixLen))
	{
		Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], BE_CHAT_CMD_REPLY,
		           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
		beChatCmdAc.count++;
	}
	if (prefixLen <= (int)strlen(BE_CHAT_CMD_MUTE) &&
	        !Q_stricmpn(BE_CHAT_CMD_MUTE, prefix, prefixLen))
	{
		Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], BE_CHAT_CMD_MUTE,
		           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
		beChatCmdAc.count++;
	}
	if (prefixLen <= (int)strlen(BE_CHAT_CMD_UNMUTE) &&
	        !Q_stricmpn(BE_CHAT_CMD_UNMUTE, prefix, prefixLen))
	{
		Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], BE_CHAT_CMD_UNMUTE,
		           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
		beChatCmdAc.count++;
	}

	if (beChatCmdAc.sel >= beChatCmdAc.count)
	{
		beChatCmdAc.sel = 0;
	}
}
static void CG_BEChat_CmdAcTabSlashCommand(beChatInput_t* edit, float maxTextW)
{
	int len;

	if (!edit)
	{
		return;
	}

	if (beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_PLAIN_CMD)
	{
		len = strlen(edit->buffer);
		if (len + 1 >= MAX_SAY_TEXT)
		{
			CG_BEChat_InputClampScroll(edit, maxTextW);
			return;
		}
		memmove(edit->buffer + 1, edit->buffer, len + 1);
		edit->buffer[0] = '/';
		edit->cursor++;
		CG_BEChat_EmojiOnInputChanged(edit);
		CG_BEChat_CmdAcRefresh(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	if (beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_SLASH_CMD)
	{
		CG_BEChat_CmdAcComplete(edit, maxTextW);
		return;
	}

	CG_BEChat_CmdAcComplete(edit, maxTextW);
}
static void CG_BEChat_PlayerPlainName(int clientNum, char* out, int outSize)
{
	clientInfo_t* ci;

	if (!out || outSize < 1)
	{
		return;
	}

	out[0] = '\0';
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return;
	}

	if (ci->name_clean[0])
	{
		Q_strncpyz(out, ci->name_clean, outSize);
		return;
	}

	Q_strncpyz(out, ci->name, outSize);
	CG_RemoveChatEscapeCharAll(out);
}
static qboolean CG_BEChat_IsDigitsOnly(const char* s)
{
	if (!s || !s[0])
	{
		return qfalse;
	}

	while (*s)
	{
		if (*s < '0' || *s > '9')
		{
			return qfalse;
		}
		s++;
	}

	return qtrue;
}
void CG_BEChat_CmdAcRefresh(beChatInput_t* edit)
{
	const char* buf;
	int         cursor;
	int         i;
	int         spacePos;
	int         prefixLen;
	char        prefix[MAX_NAME_LENGTH];
	char        plainName[MAX_NAME_LENGTH];
	int         p;
	int         nameStart;
	int         nameEnd;

	beChatCmdAc.count = 0;
	beChatCmdAc.mode = 0;
	beChatCmdAc.replaceFrom = 0;
	beChatCmdAc.replaceTo = 0;

	if (!edit)
	{
		return;
	}

	buf = edit->buffer;
	cursor = edit->cursor;

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK ||
	        beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK)
	{
		CG_BEChat_CmdAcRefreshMuteUnmutePickDigits(edit, buf, cursor);
		return;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
	{
		return;
	}

	if (!edit->buffer[0])
	{
		return;
	}

	if (edit->buffer[0] == '!')
	{
		int i;

		if (beAdminCmdList.count < 1)
		{
			return;
		}

		if (cursor < 1)
		{
			return;
		}
		for (i = 0; i < cursor; i++)
		{
			if (buf[i] == ' ')
			{
				return;
			}
		}
		prefixLen = cursor;
		beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_BANG_CMD;
		beChatCmdAc.replaceFrom = 0;
		beChatCmdAc.replaceTo = cursor;
		CG_BEAdminCmd_FillEntries(buf, prefixLen);
		return;
	}

	if (CG_BEChat_LineCmdChannelOpen() && edit->buffer[0] != '/')
	{
		int i;

		if (cursor < 1)
		{
			return;
		}
		for (i = 0; i < cursor; i++)
		{
			if (buf[i] == ' ')
			{
				return;
			}
		}
		prefixLen = cursor;
		beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_PLAIN_CMD;
		beChatCmdAc.replaceFrom = 0;
		beChatCmdAc.replaceTo = cursor;
		CG_BEChat_CmdAcFillSlashCommandEntries(buf, prefixLen);
		return;
	}

	if (edit->buffer[0] != '/')
	{
		return;
	}

	if (cursor < 1)
	{
		return;
	}

	if (CG_BEChat_CmdAcRefreshMuteUnmuteLine(edit, buf, cursor))
	{
		return;
	}

	if (CG_BEChat_LineSlashCmdAfter(buf, BE_CHAT_CMD_TELL, &nameStart))
	{
		while (nameStart < cursor && buf[nameStart] == ' ')
		{
			nameStart++;
		}
		if (nameStart < cursor)
		{
			nameEnd = cursor;
			for (i = nameStart; i < cursor; i++)
			{
				if (buf[i] == ' ')
				{
					return;
				}
			}

			prefixLen = nameEnd - nameStart;
			if (prefixLen >= (int)sizeof(prefix))
			{
				prefixLen = (int)sizeof(prefix) - 1;
			}
			memcpy(prefix, buf + nameStart, prefixLen);
			prefix[prefixLen] = '\0';

			if (CG_BEChat_IsDigitsOnly(prefix))
			{
				return;
			}

			beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_TELL_NAME;
			beChatCmdAc.replaceFrom = nameStart;
			beChatCmdAc.replaceTo = nameEnd;

			for (p = 0; p < MAX_CLIENTS && beChatCmdAc.count < BE_CHAT_CMD_AC_MAX; p++)
			{
				CG_BEChat_PlayerPlainName(p, plainName, sizeof(plainName));
				if (!plainName[0])
				{
					continue;
				}
				if (p == cg.clientNum)
				{
					continue;
				}
				if (!Q_stricmpn(plainName, prefix, prefixLen))
				{
					Q_strncpyz(beChatCmdAc.entries[beChatCmdAc.count], plainName,
					           sizeof(beChatCmdAc.entries[beChatCmdAc.count]));
					beChatCmdAc.count++;
				}
			}

			if (beChatCmdAc.sel >= beChatCmdAc.count)
			{
				beChatCmdAc.sel = 0;
			}
		}
		return;
	}

	spacePos = -1;
	for (i = 0; i < cursor; i++)
	{
		if (buf[i] == ' ')
		{
			spacePos = i;
			break;
		}
	}

	if (spacePos >= 0)
	{
		return;
	}

	prefixLen = cursor - 1;
	if (prefixLen < 0)
	{
		prefixLen = 0;
	}

	beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_SLASH_CMD;
	beChatCmdAc.replaceFrom = 1;
	beChatCmdAc.replaceTo = cursor;
	CG_BEChat_CmdAcFillSlashCommandEntries(buf + 1, prefixLen);
}
void CG_BEChat_ResetDestMode(void)
{
	if (beChatWindow.savedChannelDest == BE_CHAT_DEST_TEAM)
	{
		beChatWindow.destMode = BE_CHAT_DEST_TEAM;
		beChatWindow.channelDest = BE_CHAT_DEST_TEAM;
	}
	else if (beChatWindow.savedChannelDest == BE_CHAT_DEST_SAY)
	{
		beChatWindow.destMode = BE_CHAT_DEST_SAY;
		beChatWindow.channelDest = BE_CHAT_DEST_SAY;
	}
	else if (beChatWindow.channelDest == BE_CHAT_DEST_TEAM)
	{
		beChatWindow.destMode = BE_CHAT_DEST_TEAM;
	}
	else
	{
		beChatWindow.destMode = BE_CHAT_DEST_SAY;
		beChatWindow.channelDest = BE_CHAT_DEST_SAY;
	}
	beChatWindow.savedChannelDest = -1;
	beChatWindow.tellClientNum = -1;
	beChatWindow.tellName[0] = '\0';
}
void CG_BEChat_CancelMuteConfirm(void)
{
	if (beChatWindow.destMode != BE_CHAT_DEST_MUTE_CONFIRM)
	{
		return;
	}

	beChatWindow.tellClientNum = -1;
	CG_BEChat_InputClear(&beChatWindow.input);
	CG_BEChat_ResetDestMode();
	CG_BEChat_CmdAcClear();
	CG_BEChat_EmojiOnInputChanged(&beChatWindow.input);
}
static qboolean CG_BEChat_DestLocksTab(void)
{
	return (beChatWindow.destMode == BE_CHAT_DEST_TELL ||
	        beChatWindow.destMode == BE_CHAT_DEST_REPLY ||
	        beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK ||
	        beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM ||
	        beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK) ? qtrue : qfalse;
}
qboolean CG_BEChat_TellClientFromIdToken(const char* token, int* outClientNum)
{
	int clientNum;
	char plainName[MAX_NAME_LENGTH];

	if (!token || !outClientNum || !CG_BEChat_IsDigitsOnly(token))
	{
		return qfalse;
	}

	clientNum = atoi(token);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	if (!cgs.clientinfo[clientNum].infoValid)
	{
		return qfalse;
	}

	CG_BEChat_PlayerPlainName(clientNum, plainName, sizeof(plainName));
	if (!plainName[0])
	{
		return qfalse;
	}

	*outClientNum = clientNum;
	return qtrue;
}
static qboolean CG_BEChat_LineSlashCmdAfter(const char* buf, const char* cmdWord, int* outNameStart)
{
	const char* p;
	int         cmdLen;

	if (!buf || !cmdWord || !cmdWord[0])
	{
		return qfalse;
	}

	p = buf;
	if (p[0] == '/')
	{
		p++;
	}

	cmdLen = (int)strlen(cmdWord);
	if (Q_stricmpn(p, cmdWord, cmdLen) != 0)
	{
		return qfalse;
	}

	if (p[cmdLen] != ' ')
	{
		return qfalse;
	}

	if (outNameStart)
	{
		*outNameStart = (int)(p + cmdLen + 1 - buf);
	}

	return qtrue;
}
static qboolean CG_BEChat_LineMuteAfterCmd(const char* buf, int* outNameStart)
{
	return CG_BEChat_LineSlashCmdAfter(buf, BE_CHAT_CMD_MUTE, outNameStart);
}
static qboolean CG_BEChat_LineUnmuteAfterCmd(const char* buf, int* outNameStart)
{
	return CG_BEChat_LineSlashCmdAfter(buf, BE_CHAT_CMD_UNMUTE, outNameStart);
}
static qboolean CG_BEChat_DestIsMuteFlow(void)
{
	return (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK ||
	        beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM ||
	        beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK) ? qtrue : qfalse;
}
static void CG_BEChat_SaveChannelDestBeforePick(void)
{
	if (beChatWindow.destMode == BE_CHAT_DEST_TEAM)
	{
		beChatWindow.savedChannelDest = BE_CHAT_DEST_TEAM;
	}
	else
	{
		beChatWindow.savedChannelDest = BE_CHAT_DEST_SAY;
	}
}
static void CG_BEChat_IdPickBegin(int pickDestMode, beChatInput_t* edit)
{
	CG_BEChat_SaveChannelDestBeforePick();
	beChatWindow.destMode = pickDestMode;
	beChatWindow.tellClientNum = -1;
	beChatWindow.tellName[0] = '\0';
	if (edit)
	{
		edit->buffer[0] = '\0';
		edit->cursor = 0;
		edit->scroll = 0;
		CG_BEChat_EmojiOnInputChanged(edit);
	}
	CG_BEChat_CmdAcClear();
}
static void CG_BEChat_MutePickBegin(beChatInput_t* edit)
{
	CG_BEChat_IdPickBegin(BE_CHAT_DEST_MUTE_PICK, edit);
}
static void CG_BEChat_UnmutePickBegin(beChatInput_t* edit)
{
	CG_BEChat_IdPickBegin(BE_CHAT_DEST_UNMUTE_PICK, edit);
}
static qboolean CG_BEChat_TryEnterSlashPickFromBuffer(beChatInput_t* edit,
        qboolean (*lineAfterCmd)(const char* buf, int* outNameStart),
        int pickDestMode)
{
	const char* buf;
	int         nameStart;

	if (!edit || beChatWindow.destMode == pickDestMode ||
	        beChatWindow.destMode == BE_CHAT_DEST_TELL ||
	        CG_BEChat_DestIsMuteFlow())
	{
		return qfalse;
	}

	buf = edit->buffer;
	if (buf[0] != '/')
	{
		return qfalse;
	}

	if (!lineAfterCmd(buf, &nameStart))
	{
		return qfalse;
	}

	while (buf[nameStart] == ' ')
	{
		nameStart++;
	}

	if (buf[nameStart])
	{
		return qfalse;
	}

	CG_BEChat_IdPickBegin(pickDestMode, edit);
	return qtrue;
}
qboolean CG_BEChat_TryEnterMutePickFromBuffer(beChatInput_t* edit)
{
	return CG_BEChat_TryEnterSlashPickFromBuffer(edit, CG_BEChat_LineMuteAfterCmd,
	        BE_CHAT_DEST_MUTE_PICK);
}
qboolean CG_BEChat_TryEnterUnmutePickFromBuffer(beChatInput_t* edit)
{
	return CG_BEChat_TryEnterSlashPickFromBuffer(edit, CG_BEChat_LineUnmuteAfterCmd,
	        BE_CHAT_DEST_UNMUTE_PICK);
}
static qboolean CG_BEChat_EnterMuteConfirm(beChatInput_t* edit, int clientNum)
{
	clientInfo_t* ci;
	char          confirm[MAX_SAY_TEXT];

	if (!edit || clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return qfalse;
	}

	beChatWindow.tellClientNum = clientNum;
	confirm[0] = '\0';
	Q_strncpyz(confirm, "mute: ", sizeof(confirm));
	Q_strcat(confirm, sizeof(confirm), ci->name);
	Q_strcat(confirm, sizeof(confirm), "?");
	Q_strncpyz(edit->buffer, confirm, sizeof(edit->buffer));
	edit->cursor = strlen(edit->buffer);
	edit->scroll = 0;
	beChatWindow.destMode = BE_CHAT_DEST_MUTE_CONFIRM;
	CG_BEChat_CmdAcClear();
	CG_BEChat_EmojiOnInputChanged(edit);
	return qtrue;
}
static void CG_BEChat_FinishUnmutePick(beChatInput_t* edit, int clientNum)
{
	CG_ChatfilterUnmuteClient(clientNum);
	CG_BEChat_InputClear(edit);
	CG_BEChat_ResetDestMode();
	beChatWindow.historyScroll = 0;
	if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
	{
		CG_BEChat_Close();
	}
}
static qboolean CG_BEChat_ParseMuteUnmuteIdAtCursor(const char* buf, const char* cmdWord,
        int cursor, int* outClientNum)
{
	int  nameStart;
	int  i;
	int  nameLen;
	char name[16];

	if (!buf || !outClientNum || !CG_BEChat_LineSlashCmdAfter(buf, cmdWord, &nameStart))
	{
		return qfalse;
	}

	while (buf[nameStart] == ' ')
	{
		nameStart++;
	}

	nameLen = 0;
	for (i = nameStart; i < cursor; i++)
	{
		if (buf[i] == ' ')
		{
			break;
		}
		if (nameLen >= (int)sizeof(name) - 1)
		{
			break;
		}
		name[nameLen++] = buf[i];
	}
	name[nameLen] = '\0';

	if (!name[0] || !CG_BEChat_IsDigitsOnly(name))
	{
		return qfalse;
	}

	while (i < cursor && buf[i] == ' ')
	{
		i++;
	}
	if (i != cursor)
	{
		return qfalse;
	}

	return CG_BEChat_TellClientFromIdToken(name, outClientNum);
}
static qboolean CG_BEChat_ParseMuteUnmuteIdFullLine(const char* buf, const char* cmdWord,
        int* outClientNum)
{
	int  nameStart;
	int  i;
	int  nameLen;
	char name[16];

	if (!buf || !outClientNum || !CG_BEChat_LineSlashCmdAfter(buf, cmdWord, &nameStart))
	{
		return qfalse;
	}

	while (buf[nameStart] == ' ')
	{
		nameStart++;
	}

	nameLen = 0;
	for (i = nameStart; buf[i] && buf[i] != ' '; i++)
	{
		if (nameLen >= (int)sizeof(name) - 1)
		{
			break;
		}
		name[nameLen++] = buf[i];
	}
	name[nameLen] = '\0';

	if (!name[0] || !CG_BEChat_IsDigitsOnly(name))
	{
		return qfalse;
	}

	while (buf[i] == ' ')
	{
		i++;
	}
	if (buf[i])
	{
		return qfalse;
	}

	return CG_BEChat_TellClientFromIdToken(name, outClientNum);
}
qboolean CG_BEChat_TryCommitMutePick(beChatInput_t* edit)
{
	int clientNum;

	if (!edit || beChatWindow.destMode != BE_CHAT_DEST_MUTE_PICK)
	{
		return qfalse;
	}

	if (!edit->buffer[0] || !CG_BEChat_IsDigitsOnly(edit->buffer))
	{
		return qfalse;
	}

	if (!CG_BEChat_TellClientFromIdToken(edit->buffer, &clientNum))
	{
		return qfalse;
	}

	return CG_BEChat_EnterMuteConfirm(edit, clientNum);
}
qboolean CG_BEChat_TryCommitUnmutePick(beChatInput_t* edit)
{
	int clientNum;

	if (!edit || beChatWindow.destMode != BE_CHAT_DEST_UNMUTE_PICK)
	{
		return qfalse;
	}

	if (!edit->buffer[0] || !CG_BEChat_IsDigitsOnly(edit->buffer))
	{
		return qfalse;
	}

	if (!CG_BEChat_TellClientFromIdToken(edit->buffer, &clientNum))
	{
		return qfalse;
	}

	CG_BEChat_FinishUnmutePick(edit, clientNum);
	return qtrue;
}
static qboolean CG_BEChat_LineCmdChannelOpen(void)
{
	return (beChatWindow.destMode == BE_CHAT_DEST_SAY ||
	        beChatWindow.destMode == BE_CHAT_DEST_TEAM) ? qtrue : qfalse;
}
qboolean CG_BEChat_TryCommitMuteIdLine(beChatInput_t* edit)
{
	int clientNum;

	if (!edit || !CG_BEChat_LineCmdChannelOpen())
	{
		return qfalse;
	}

	if (CG_BEChat_ParseMuteUnmuteIdAtCursor(edit->buffer, BE_CHAT_CMD_MUTE, edit->cursor, &clientNum))
	{
		return CG_BEChat_EnterMuteConfirm(edit, clientNum);
	}

	if (CG_BEChat_ParseMuteUnmuteIdFullLine(edit->buffer, BE_CHAT_CMD_MUTE, &clientNum))
	{
		return CG_BEChat_EnterMuteConfirm(edit, clientNum);
	}

	return qfalse;
}
qboolean CG_BEChat_TryCommitUnmuteIdLine(beChatInput_t* edit)
{
	int clientNum;

	if (!edit || !CG_BEChat_LineCmdChannelOpen())
	{
		return qfalse;
	}

	if (CG_BEChat_ParseMuteUnmuteIdAtCursor(edit->buffer, BE_CHAT_CMD_UNMUTE, edit->cursor, &clientNum))
	{
		CG_BEChat_FinishUnmutePick(edit, clientNum);
		return qtrue;
	}

	if (CG_BEChat_ParseMuteUnmuteIdFullLine(edit->buffer, BE_CHAT_CMD_UNMUTE, &clientNum))
	{
		CG_BEChat_FinishUnmutePick(edit, clientNum);
		return qtrue;
	}

	return qfalse;
}
static qboolean CG_BEChat_CmdAcRefreshMuteUnmutePickDigits(beChatInput_t* edit, const char* buf,
        int cursor)
{
	int  prefixLen;
	char prefix[16];
	int  acMode;

	if (cursor < 1)
	{
		return qfalse;
	}

	prefixLen = cursor;
	if (prefixLen >= (int)sizeof(prefix))
	{
		prefixLen = (int)sizeof(prefix) - 1;
	}
	if (prefixLen > 0)
	{
		memcpy(prefix, buf, prefixLen);
	}
	prefix[prefixLen] = '\0';

	if (!CG_BEChat_IsDigitsOnly(prefix) || prefixLen < 1)
	{
		return qfalse;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK)
	{
		acMode = BE_CHAT_CMD_AC_MODE_MUTE_PICK_DIGIT;
	}
	else
	{
		acMode = BE_CHAT_CMD_AC_MODE_UNMUTE_PICK_DIGIT;
	}

	beChatCmdAc.mode = acMode;
	beChatCmdAc.replaceFrom = 0;
	beChatCmdAc.replaceTo = cursor;
	beChatCmdAc.count = 1;
	beChatCmdAc.entries[0][0] = '\0';
	if (beChatCmdAc.sel >= beChatCmdAc.count)
	{
		beChatCmdAc.sel = 0;
	}
	return qtrue;
}
static qboolean CG_BEChat_ParseLineMuteUnmute(const char* buf, char* outCmd, int outCmdSize,
        int* outLineFrom, int* outCmdLettersEnd, int* outMsgStart)
{
	const char* p;
	int         cmdLen;

	if (!buf || !outCmd || outCmdSize < 1)
	{
		return qfalse;
	}

	p = buf;
	if (outLineFrom)
	{
		*outLineFrom = 0;
	}
	if (p[0] == '/')
	{
		p++;
	}

	cmdLen = 0;
	if (!Q_stricmpn(p, BE_CHAT_CMD_MUTE, (int)strlen(BE_CHAT_CMD_MUTE)))
	{
		cmdLen = (int)strlen(BE_CHAT_CMD_MUTE);
		Q_strncpyz(outCmd, BE_CHAT_CMD_MUTE, outCmdSize);
	}
	else if (!Q_stricmpn(p, BE_CHAT_CMD_UNMUTE, (int)strlen(BE_CHAT_CMD_UNMUTE)))
	{
		cmdLen = (int)strlen(BE_CHAT_CMD_UNMUTE);
		Q_strncpyz(outCmd, BE_CHAT_CMD_UNMUTE, outCmdSize);
	}
	else
	{
		return qfalse;
	}

	if (p[cmdLen] != '\0' && p[cmdLen] != ' ')
	{
		return qfalse;
	}

	if (outCmdLettersEnd)
	{
		*outCmdLettersEnd = (int)(p - buf) + cmdLen;
	}

	if (outMsgStart)
	{
		int i;

		i = (int)(p - buf) + cmdLen;
		while (buf[i] == ' ')
		{
			i++;
		}
		*outMsgStart = i;
	}

	return qtrue;
}
static qboolean CG_BEChat_CmdAcRefreshMuteUnmuteLine(beChatInput_t* edit, const char* buf, int cursor)
{
	char     cmdWord[MAX_NAME_LENGTH];
	int      lineFrom;
	int      msgStart;
	qboolean hasMsg;

	if (!CG_BEChat_ParseLineMuteUnmute(buf, cmdWord, sizeof(cmdWord),
	                                   &lineFrom, NULL, &msgStart))
	{
		return qfalse;
	}

	hasMsg = (buf[msgStart] != '\0') ? qtrue : qfalse;
	beChatCmdAc.replaceFrom = (buf[0] == '/') ? 0 : lineFrom;

	if (hasMsg)
	{
		beChatCmdAc.replaceTo = msgStart;
		if (!Q_stricmp(cmdWord, BE_CHAT_CMD_MUTE))
		{
			beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_MUTE_COMMIT;
		}
		else
		{
			beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_UNMUTE_COMMIT;
		}
	}
	else
	{
		beChatCmdAc.replaceTo = cursor;
		beChatCmdAc.mode = BE_CHAT_CMD_AC_MODE_SLASH_CMD;
	}

	beChatCmdAc.count = 1;
	Q_strncpyz(beChatCmdAc.entries[0], cmdWord, sizeof(beChatCmdAc.entries[0]));
	if (beChatCmdAc.sel >= beChatCmdAc.count)
	{
		beChatCmdAc.sel = 0;
	}

	return qtrue;
}
int CG_BEChat_ClientFromTellName(const char* name)
{
	int     i;
	int     match;
	int     count;
	char    plainName[MAX_NAME_LENGTH];

	if (!name || !name[0])
	{
		return -1;
	}

	match = -1;
	count = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		CG_BEChat_PlayerPlainName(i, plainName, sizeof(plainName));
		if (!plainName[0])
		{
			continue;
		}
		if (!Q_stricmp(plainName, name))
		{
			return i;
		}
		if (!Q_stricmpn(plainName, name, strlen(name)))
		{
			match = i;
			count++;
		}
	}

	if (count == 1)
	{
		return match;
	}

	return -1;
}
qboolean CG_BEChat_TryCommitTellTarget(beChatInput_t* edit)
{
	const char* buf;
	int         nameStart;
	int         nameLen;
	int         afterName;
	int         clientNum;
	int         i;
	char        name[MAX_NAME_LENGTH];
	char        plainName[MAX_NAME_LENGTH];

	if (!edit || beChatWindow.destMode == BE_CHAT_DEST_TELL ||
	        CG_BEChat_DestIsMuteFlow())
	{
		return qfalse;
	}

	buf = edit->buffer;
	if (!CG_BEChat_LineSlashCmdAfter(buf, BE_CHAT_CMD_TELL, &nameStart))
	{
		return qfalse;
	}

	while (buf[nameStart] == ' ')
	{
		nameStart++;
	}

	for (nameLen = 0; buf[nameStart + nameLen] && buf[nameStart + nameLen] != ' '; nameLen++)
	{
		if (nameLen >= (int)sizeof(name) - 1)
		{
			break;
		}
		name[nameLen] = buf[nameStart + nameLen];
	}
	name[nameLen] = '\0';
	if (!name[0])
	{
		return qfalse;
	}

	afterName = nameStart + nameLen;

	if (CG_BEChat_IsDigitsOnly(name))
	{
		i = afterName;
		while (buf[i] == ' ' && i < edit->cursor)
		{
			i++;
		}
		if (i != edit->cursor)
		{
			return qfalse;
		}

		if (!CG_BEChat_TellClientFromIdToken(name, &clientNum))
		{
			return qfalse;
		}

		CG_BEChat_PlayerPlainName(clientNum, plainName, sizeof(plainName));
		while (buf[afterName] == ' ')
		{
			afterName++;
		}

		beChatWindow.destMode = BE_CHAT_DEST_TELL;
		beChatWindow.tellClientNum = clientNum;
		Q_strncpyz(beChatWindow.tellName, plainName, sizeof(beChatWindow.tellName));

		memmove(edit->buffer, buf + afterName, strlen(buf + afterName) + 1);
		edit->cursor = strlen(edit->buffer);
		edit->scroll = 0;
		CG_BEChat_CmdAcClear();
		CG_BEChat_EmojiOnInputChanged(edit);
		return qtrue;
	}

	clientNum = CG_BEChat_ClientFromTellName(name);
	if (clientNum < 0)
	{
		return qfalse;
	}

	CG_BEChat_PlayerPlainName(clientNum, plainName, sizeof(plainName));
	if (Q_stricmp(plainName, name) != 0)
	{
		return qfalse;
	}

	if (buf[afterName] != '\0' && buf[afterName] != ' ')
	{
		return qfalse;
	}

	while (buf[afterName] == ' ')
	{
		afterName++;
	}

	beChatWindow.destMode = BE_CHAT_DEST_TELL;
	beChatWindow.tellClientNum = clientNum;
	Q_strncpyz(beChatWindow.tellName, plainName, sizeof(beChatWindow.tellName));

	memmove(edit->buffer, buf + afterName, strlen(buf + afterName) + 1);
	edit->cursor = strlen(edit->buffer);
	edit->scroll = 0;
	CG_BEChat_CmdAcClear();
	CG_BEChat_EmojiOnInputChanged(edit);
	return qtrue;
}
qboolean CG_BEChat_IsReplyLinePrefix(const char* buf, int* outMsgStart)
{
	const char* p;
	int         i;

	if (!buf)
	{
		return qfalse;
	}

	p = buf;
	if (p[0] == '/')
	{
		p++;
	}

	if (Q_stricmpn(p, BE_CHAT_CMD_REPLY, (int)strlen(BE_CHAT_CMD_REPLY)) != 0)
	{
		return qfalse;
	}

	i = (int)strlen(BE_CHAT_CMD_REPLY);
	if (p[i] != '\0' && p[i] != ' ')
	{
		return qfalse;
	}

	while (p[i] == ' ')
	{
		i++;
	}

	if (outMsgStart)
	{
		*outMsgStart = (int)(p - buf) + i;
	}

	return qtrue;
}
qboolean CG_BEChat_TryCommitReply(beChatInput_t* edit)
{
	const char* buf;
	int         msgStart;

	if (!edit || beChatWindow.destMode == BE_CHAT_DEST_REPLY)
	{
		return qfalse;
	}

	buf = edit->buffer;
	if (!CG_BEChat_IsReplyLinePrefix(buf, &msgStart))
	{
		return qfalse;
	}

	beChatWindow.destMode = BE_CHAT_DEST_REPLY;
	beChatWindow.tellClientNum = -1;
	beChatWindow.tellName[0] = '\0';

	memmove(edit->buffer, buf + msgStart, strlen(buf + msgStart) + 1);
	edit->cursor = strlen(edit->buffer);
	edit->scroll = 0;
	CG_BEChat_CmdAcClear();
	CG_BEChat_EmojiOnInputChanged(edit);
	return qtrue;
}
qboolean CG_BEChat_CmdAcIsActive(void)
{
	return (beChatCmdAc.count > 0) ? qtrue : qfalse;
}
void CG_BEChat_CmdAcComplete(beChatInput_t* edit, float maxTextW)
{
	char    insert[MAX_NAME_LENGTH + 2];
	char    completedCmd[MAX_NAME_LENGTH];
	int     insertLen;
	int     replaceLen;
	int     tail;
	int     sel;
	int     completedMode;

	if (!edit || beChatCmdAc.count < 1)
	{
		return;
	}

	sel = beChatCmdAc.sel;
	if (sel < 0 || sel >= beChatCmdAc.count)
	{
		sel = 0;
	}

	completedMode = beChatCmdAc.mode;
	Q_strncpyz(completedCmd, beChatCmdAc.entries[sel], sizeof(completedCmd));

	if (completedMode == BE_CHAT_CMD_AC_MODE_MUTE_PICK_DIGIT)
	{
		CG_BEChat_TryCommitMutePick(edit);
		CG_BEChat_CmdAcClear();
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	if (completedMode == BE_CHAT_CMD_AC_MODE_UNMUTE_PICK_DIGIT)
	{
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	if (completedMode == BE_CHAT_CMD_AC_MODE_MUTE_COMMIT)
	{
		CG_BEChat_TryCommitMuteIdLine(edit);
		CG_BEChat_CmdAcClear();
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	if (completedMode == BE_CHAT_CMD_AC_MODE_UNMUTE_COMMIT)
	{
		CG_BEChat_CmdAcRefresh(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	Q_strncpyz(insert, beChatCmdAc.entries[sel], sizeof(insert));
	insertLen = strlen(insert);
	if (insertLen + 1 >= (int)sizeof(insert))
	{
		return;
	}
	insert[insertLen] = ' ';
	insert[insertLen + 1] = '\0';
	insertLen++;

	replaceLen = beChatCmdAc.replaceTo - beChatCmdAc.replaceFrom;
	if (replaceLen < 0)
	{
		replaceLen = 0;
	}
	tail = strlen(edit->buffer) - beChatCmdAc.replaceTo;
	if (tail < 0)
	{
		tail = 0;
	}
	if (beChatCmdAc.replaceFrom + insertLen + tail >= MAX_SAY_TEXT)
	{
		return;
	}

	memmove(edit->buffer + beChatCmdAc.replaceFrom + insertLen,
	        edit->buffer + beChatCmdAc.replaceTo, tail + 1);
	memcpy(edit->buffer + beChatCmdAc.replaceFrom, insert, insertLen);
	edit->cursor = beChatCmdAc.replaceFrom + insertLen;
	CG_BEChat_EmojiOnInputChanged(edit);
	CG_BEChat_CmdAcRefresh(edit);
	CG_BEChat_InputClampScroll(edit, maxTextW);

	if (completedMode == BE_CHAT_CMD_AC_MODE_TELL_NAME)
	{
		CG_BEChat_TryCommitTellTarget(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
	else if (completedMode == BE_CHAT_CMD_AC_MODE_SLASH_CMD &&
	         !Q_stricmp(completedCmd, BE_CHAT_CMD_REPLY))
	{
		CG_BEChat_TryCommitReply(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
	else if (completedMode == BE_CHAT_CMD_AC_MODE_SLASH_CMD &&
	         !Q_stricmp(completedCmd, BE_CHAT_CMD_MUTE))
	{
		CG_BEChat_MutePickBegin(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
	else if (completedMode == BE_CHAT_CMD_AC_MODE_SLASH_CMD &&
	         !Q_stricmp(completedCmd, BE_CHAT_CMD_UNMUTE))
	{
		CG_BEChat_UnmutePickBegin(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
}
qboolean CG_BEChat_CmdAcProcessKey(int keycode, beChatInput_t* edit, float maxTextW)
{
	if (!edit || beChatCmdAc.count < 1)
	{
		return qfalse;
	}

	if (keycode == K_TAB ||
	        ((keycode == K_RIGHTARROW || keycode == K_KP_RIGHTARROW) && !trap_Key_IsDown(K_CTRL)))
	{
		if (beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK ||
		        beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_UNMUTE_PICK_DIGIT)
		{
			return qtrue;
		}
		if (beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_PLAIN_CMD ||
		        beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_SLASH_CMD)
		{
			CG_BEChat_CmdAcTabSlashCommand(edit, maxTextW);
		}
		else if (beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_BANG_CMD)
		{
			CG_BEChat_CmdAcComplete(edit, maxTextW);
		}
		else
		{
			CG_BEChat_CmdAcComplete(edit, maxTextW);
		}
		return qtrue;
	}

	if (!trap_Key_IsDown(K_CTRL))
	{
		if (keycode == K_UPARROW || keycode == K_KP_UPARROW)
		{
			beChatCmdAc.sel--;
			if (beChatCmdAc.sel < 0)
			{
				beChatCmdAc.sel = beChatCmdAc.count - 1;
			}
			return qtrue;
		}
		if (keycode == K_DOWNARROW || keycode == K_KP_DOWNARROW)
		{
			beChatCmdAc.sel++;
			if (beChatCmdAc.sel >= beChatCmdAc.count)
			{
				beChatCmdAc.sel = 0;
			}
			return qtrue;
		}
	}

	return qfalse;
}
const char* CG_BEChat_InputPromptText(void)
{
	if (beChatWindow.destMode == BE_CHAT_DEST_TEAM)
	{
		return "> (TEAM)";
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK)
	{
		return "> mute:";
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK)
	{
		return "> unmute:";
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
	{
		return ">";
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_TELL && beChatWindow.tellName[0])
	{
		Com_sprintf(beChatPromptText, sizeof(beChatPromptText), "> (%s)", beChatWindow.tellName);
		return beChatPromptText;
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_REPLY)
	{
		return "> reply";
	}
	return ">";
}
float CG_BEChat_InputTextDrawX(float contentX)
{
	float promptW;

	promptW = CG_OSPDrawStringNewTextWidth(CG_BEChat_InputPromptText(),
	                                       BECHAT_CHAR_W, BECHAT_CHAR_H,
	                                       BE_CHAT_TEXT_FLAGS, BE_CHAT_FONT);
	return contentX + promptW + BECHAT_CHAR_W * 0.5f;
}
void CG_BEChat_CmdAcDraw(float inputX, float inputY, beChatInput_t* edit,
                         const vec4_t color, int textFlags)
{
	char        ghostBuf[MAX_STRING_CHARS];
	char        prefixBuf[MAX_STRING_CHARS];
	const char* full;
	int         prefixLen;
	int         ghostLen;
	int         prestep;
	int         prefixLenDraw;
	int         textFlagsDraw;
	float       drawX;
	float       ghostX;
	float       tokenX;
	float       previewRowY;
	float       previewW;
	float       previewCharW;
	float       previewCharH;
	qboolean    drawGhost;
	vec4_t      ghostColor;
	vec4_t      ghostBg;
	vec4_t      previewBg;

	if (!edit || beChatCmdAc.count < 1 || beChatCmdAc.sel < 0 ||
	        beChatCmdAc.sel >= beChatCmdAc.count)
	{
		return;
	}

	full = beChatCmdAc.entries[beChatCmdAc.sel];
	if (!full[0])
	{
		return;
	}

	prefixLen = beChatCmdAc.replaceTo - beChatCmdAc.replaceFrom;
	if (prefixLen < 0)
	{
		prefixLen = 0;
	}
	if (prefixLen >= (int)sizeof(prefixBuf))
	{
		prefixLen = (int)sizeof(prefixBuf) - 1;
	}
	memcpy(prefixBuf, edit->buffer + beChatCmdAc.replaceFrom, prefixLen);
	prefixBuf[prefixLen] = '\0';

	if (Q_stricmpn(full, prefixBuf, prefixLen) != 0)
	{
		return;
	}

	textFlagsDraw = CG_BEChat_EmojiTextFlags(textFlags);
	previewCharW = BECHAT_CHAR_W * BE_CHAT_CMD_AC_PREVIEW_SCALE;
	previewCharH = BECHAT_CHAR_H * BE_CHAT_CMD_AC_PREVIEW_SCALE;

	drawX = CG_BEChat_InputTextDrawX(inputX);
	prestep = edit->scroll;
	tokenX = drawX;
	if (beChatCmdAc.replaceFrom > prestep)
	{
		tokenX = drawX + CG_OSPDrawStringNewSliceWidth(edit->buffer + prestep,
		        edit->buffer + beChatCmdAc.replaceFrom,
		        BECHAT_CHAR_W, BECHAT_CHAR_H,
		        textFlagsDraw, BE_CHAT_FONT);
	}

	previewRowY = inputY - BECHAT_EMOJI_PREVIEW_GAP - previewCharH;
	previewW = CG_OSPDrawStringNewTextWidth(full, previewCharW, previewCharH,
	                                        textFlagsDraw, BE_CHAT_FONT);
	if (previewW < previewCharW)
	{
		previewW = previewCharW;
	}

	previewBg[0] = 1.0f;
	previewBg[1] = 1.0f;
	previewBg[2] = 1.0f;
	previewBg[3] = 0.2f;
	CG_FillRect(tokenX, previewRowY, previewW, previewCharH, previewBg);
	CG_OSPDrawStringNew(tokenX, previewRowY, full, color, colorBlack,
	                    previewCharW, previewCharH, 0, textFlagsDraw, BE_CHAT_FONT,
	                    NULL, NULL, NULL);

	ghostLen = strlen(full) - prefixLen;
	drawGhost = qfalse;
	ghostBuf[0] = '\0';
	if (ghostLen > 0)
	{
		Q_strncpyz(ghostBuf, full + prefixLen, sizeof(ghostBuf));
		Q_strcat(ghostBuf, sizeof(ghostBuf), " ");
		ghostLen = strlen(ghostBuf);
		drawGhost = qtrue;
	}
	else if (beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_SLASH_CMD ||
	         beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_PLAIN_CMD ||
	         beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_BANG_CMD)
	{
		Q_strncpyz(ghostBuf, " ", sizeof(ghostBuf));
		ghostLen = 1;
		drawGhost = qtrue;
	}

	if (!drawGhost || beChatCmdAc.replaceTo <= prestep)
	{
		return;
	}

	ghostBg[0] = 1.0f;
	ghostBg[1] = 1.0f;
	ghostBg[2] = 1.0f;
	ghostBg[3] = 0.2f;
	ghostColor[0] = 0.0f;
	ghostColor[1] = 0.0f;
	ghostColor[2] = 0.0f;
	ghostColor[3] = 0.55f;

	prefixLenDraw = beChatCmdAc.replaceTo - prestep;
	if (prefixLenDraw < 0)
	{
		prefixLenDraw = 0;
	}
	if (prefixLenDraw > 0)
	{
		memcpy(prefixBuf, edit->buffer + prestep, prefixLenDraw);
		prefixBuf[prefixLenDraw] = '\0';
	}
	else
	{
		prefixBuf[0] = '\0';
	}

	ghostX = drawX + CG_OSPDrawStringNewTextWidth(prefixBuf, BECHAT_CHAR_W, BECHAT_CHAR_H,
	        textFlagsDraw, BE_CHAT_FONT);
	CG_OSPDrawStringNew(ghostX, inputY, ghostBuf, ghostColor, colorBlack,
	                    BECHAT_CHAR_W, BECHAT_CHAR_H, 0, textFlagsDraw, BE_CHAT_FONT,
	                    ghostBg, NULL, NULL);
}
void CG_BEChat_ProcessTabKey(float maxTextW)
{
	beChatInput_t* edit;

	edit = &beChatWindow.input;
	if (CG_BEChat_CmdAcProcessKey(K_TAB, edit, maxTextW))
	{
		return;
	}
	if (CG_BEChat_EmojiProcessKey(K_TAB, edit, maxTextW))
	{
		return;
	}
	if (CG_BEChat_TryCommitTellTarget(edit))
	{
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}
	if (CG_BEChat_TryCommitMutePick(edit))
	{
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}
	if (CG_BEChat_TryCommitMuteIdLine(edit))
	{
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}
	if (CG_BEChat_DestLocksTab())
	{
		return;
	}
	if (beChatWindow.destMode == BE_CHAT_DEST_TEAM)
	{
		beChatWindow.destMode = BE_CHAT_DEST_SAY;
		beChatWindow.channelDest = BE_CHAT_DEST_SAY;
	}
	else
	{
		beChatWindow.destMode = BE_CHAT_DEST_TEAM;
		beChatWindow.channelDest = BE_CHAT_DEST_TEAM;
	}
}
