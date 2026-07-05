/*
===========================================================================
cg_be_chatwindow_persist.c — History and sent-line persistence (cvars).
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

static int CG_BEChat_PersistHistSlotMax(void)
{
	if (!cg_shudChatEnable.integer)
	{
		return BE_CHAT_PERSIST_SLOTS * 2;
	}
	return BE_CHAT_PERSIST_SLOTS;
}

int CG_BEChat_HistoryLineMax(void)
{
	if (!cg_shudChatEnable.integer)
	{
		return BE_CHAT_HISTORY_LINES;
	}
	return BE_CHAT_HISTORY_LINES / 2;
}

static const char* CG_BEChat_PersistCvarName(int index)
{
	switch (index)
	{
	case 0: return "cg_bechat_hist0";
	case 1: return "cg_bechat_hist1";
	case 2: return "cg_bechat_hist2";
	case 3: return "cg_bechat_hist3";
	case 4: return "cg_bechat_hist4";
	case 5: return "cg_bechat_hist5";
	case 6: return "cg_bechat_hist6";
	case 7: return "cg_bechat_hist7";
	case 8: return "cg_bechat_hist8";
	case 9: return "cg_bechat_hist9";
	case 10: return "cg_bechat_hist10";
	case 11: return "cg_bechat_hist11";
	case 12: return "cg_bechat_hist12";
	case 13: return "cg_bechat_hist13";
	case 14: return "cg_bechat_hist14";
	case 15: return "cg_bechat_hist15";
	case 16: return "cg_shud_chatmsg0";
	case 17: return "cg_shud_chatmsg1";
	case 18: return "cg_shud_chatmsg2";
	case 19: return "cg_shud_chatmsg3";
	case 20: return "cg_shud_chatmsg4";
	case 21: return "cg_shud_chatmsg5";
	case 22: return "cg_shud_chatmsg6";
	case 23: return "cg_shud_chatmsg7";
	case 24: return "cg_shud_chatmsg8";
	case 25: return "cg_shud_chatmsg9";
	case 26: return "cg_shud_chatmsg10";
	case 27: return "cg_shud_chatmsg11";
	case 28: return "cg_shud_chatmsg12";
	case 29: return "cg_shud_chatmsg13";
	case 30: return "cg_shud_chatmsg14";
	case 31: return "cg_shud_chatmsg15";
	default: return NULL;
	}
}
static const char* CG_BEChat_PersistSentCvarName(int index)
{
	switch (index)
	{
	case 0: return "cg_bechat_sent0";
	case 1: return "cg_bechat_sent1";
	case 2: return "cg_bechat_sent2";
	case 3: return "cg_bechat_sent3";
	case 4: return "cg_bechat_sent4";
	case 5: return "cg_bechat_sent5";
	case 6: return "cg_bechat_sent6";
	case 7: return "cg_bechat_sent7";
	case 8: return "cg_bechat_sent8";
	case 9: return "cg_bechat_sent9";
	case 10: return "cg_bechat_sent10";
	case 11: return "cg_bechat_sent11";
	case 12: return "cg_bechat_sent12";
	case 13: return "cg_bechat_sent13";
	case 14: return "cg_bechat_sent14";
	case 15: return "cg_bechat_sent15";
	default: return NULL;
	}
}
static void CG_BEChat_PersistEscapeAppend(char* out, int outSize, const char* in)
{
	int i;
	int len;

	if (!out || outSize < 2 || !in)
	{
		return;
	}

	len = strlen(out);
	for (i = 0; in[i] && len < outSize - 1; i++)
	{
		if ((in[i] == '\\' || in[i] == '|' || in[i] == ';' || in[i] == ',') && len < outSize - 2)
		{
			out[len++] = '\\';
		}
		if (len < outSize - 1)
		{
			out[len++] = in[i];
		}
	}
	out[len] = '\0';
}
static qboolean CG_BEChat_PersistReadInt(const char** pp, int* outValue)
{
	const char* p;
	int sign;
	int value;

	if (!pp || !*pp || !outValue)
	{
		return qfalse;
	}

	p = *pp;
	sign = 1;
	value = 0;
	if (*p == '-')
	{
		sign = -1;
		p++;
	}
	if (*p < '0' || *p > '9')
	{
		return qfalse;
	}
	while (*p >= '0' && *p <= '9')
	{
		value = value * 10 + (*p - '0');
		p++;
	}
	*outValue = value * sign;
	*pp = p;
	return qtrue;
}
static qboolean CG_BEChat_PersistParseEntry(const char* entry, char* outText, int outTextSize,
        int* outSender, int* outMsgKind, int* outRepeat)
{
	const char* p;
	char textBuf[BE_CHAT_LINE_CHARS];
	int textLen;

	if (!entry || !entry[0] || !outText || outTextSize < 2 || !outSender || !outMsgKind || !outRepeat)
	{
		return qfalse;
	}

	p = entry;
	if (!CG_BEChat_PersistReadInt(&p, outMsgKind) || *p != ',')
	{
		return qfalse;
	}
	p++;
	if (!CG_BEChat_PersistReadInt(&p, outRepeat) || *p != ',')
	{
		return qfalse;
	}
	p++;
	if (!CG_BEChat_PersistReadInt(&p, outSender) || *p != ',')
	{
		return qfalse;
	}
	p++;

	textLen = 0;
	while (*p && *p != ';' && textLen < (int)sizeof(textBuf) - 1)
	{
		if (*p == '\\' && p[1])
		{
			p++;
		}
		textBuf[textLen++] = *p++;
	}
	textBuf[textLen] = '\0';
	Q_strncpyz(outText, textBuf, outTextSize);

	if (*outRepeat < 1)
	{
		*outRepeat = 1;
	}
	if (*outMsgKind < BE_CHAT_MSG_SAY || *outMsgKind > BE_CHAT_MSG_OTHER)
	{
		*outMsgKind = BE_CHAT_MSG_OTHER;
	}

	return textLen > 0;
}
static qboolean CG_BEChat_PersistReadEscapedUntil(const char** pp, char* out, int outSize, char stopChar)
{
	const char* p;
	int len;

	if (!pp || !*pp || !out || outSize < 1)
	{
		return qfalse;
	}

	p = *pp;
	len = 0;
	while (*p && *p != stopChar)
	{
		if (*p == '\\' && p[1])
		{
			p++;
		}
		if (len < outSize - 1)
		{
			out[len++] = *p;
		}
		p++;
	}
	out[len] = '\0';
	if (*p == stopChar)
	{
		p++;
	}
	*pp = p;
	return qtrue;
}
static qboolean CG_BEChat_PersistParseSentEntry(const char* entry, beChatSentHistEntry_t* outEntry)
{
	const char* p;
	char textBuf[MAX_SAY_TEXT];
	int textLen;

	if (!entry || !entry[0] || !outEntry)
	{
		return qfalse;
	}

	p = entry;
	if (!CG_BEChat_PersistReadInt(&p, &outEntry->destMode) || *p != ',')
	{
		return qfalse;
	}
	p++;
	if (!CG_BEChat_PersistReadInt(&p, &outEntry->tellClientNum) || *p != ',')
	{
		return qfalse;
	}
	p++;
	if (!CG_BEChat_PersistReadEscapedUntil(&p, outEntry->tellName, sizeof(outEntry->tellName), ',') ||
	        !CG_BEChat_PersistReadEscapedUntil(&p, textBuf, sizeof(textBuf), ';'))
	{
		return qfalse;
	}

	textLen = strlen(textBuf);
	if (textLen < 1)
	{
		return qfalse;
	}

	Q_strncpyz(outEntry->text, textBuf, sizeof(outEntry->text));
	if (outEntry->destMode < BE_CHAT_DEST_SAY || outEntry->destMode > BE_CHAT_DEST_REPLY)
	{
		outEntry->destMode = BE_CHAT_DEST_SAY;
	}

	return qtrue;
}

beChatHistoryLine_t* CG_BEChat_HistoryEntry(int linesBack)
{
	beChatHistory_t* history;
	int index;
	int lineMax;

	history = &cgs.beChatHistory;
	if (linesBack < 0 || linesBack >= history->count)
	{
		return NULL;
	}

	lineMax = CG_BEChat_HistoryLineMax();
	index = history->writeIndex - 1 - linesBack;
	while (index < 0)
	{
		index += lineMax;
	}
	index %= lineMax;
	return &history->lines[index];
}

void CG_BEChat_SaveHistoryToCvars(void)
{
	int linesBack;
	int slot;
	int persistSlots;
	char slotBuf[BE_CHAT_PERSIST_CVAR_MAX];
	char entry[BE_CHAT_PERSIST_ENTRY_MAX];
	const char* cvarName;
	beChatHistoryLine_t* line;
	int candidateLen;
	int slotLen;

	persistSlots = CG_BEChat_PersistHistSlotMax();
	slot = 0;
	slotBuf[0] = '\0';
	for (linesBack = 0; linesBack < cgs.beChatHistory.count && slot < persistSlots; linesBack++)
	{
		line = CG_BEChat_HistoryEntry(linesBack);
		if (!line || !line->text[0])
		{
			continue;
		}

		Com_sprintf(entry, sizeof(entry), "%i,%i,%i,", line->msgKind, line->repeat, line->senderClientNum);
		CG_BEChat_PersistEscapeAppend(entry, sizeof(entry), line->text);
		Q_strcat(entry, sizeof(entry), ";");

		slotLen = strlen(slotBuf);
		candidateLen = slotLen + (slotLen > 0 ? 1 : 0) + strlen(entry);
		if (candidateLen >= (int)sizeof(slotBuf))
		{
			cvarName = CG_BEChat_PersistCvarName(slot);
			if (cvarName)
			{
				trap_Cvar_Set(cvarName, slotBuf);
			}
			slot++;
			if (slot >= persistSlots)
			{
				break;
			}
			Q_strncpyz(slotBuf, entry, sizeof(slotBuf));
			continue;
		}

		if (slotLen > 0)
		{
			Q_strcat(slotBuf, sizeof(slotBuf), "|");
		}
		Q_strcat(slotBuf, sizeof(slotBuf), entry);
	}

	if (slot < persistSlots)
	{
		cvarName = CG_BEChat_PersistCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, slotBuf);
		}
		slot++;
	}
	for (; slot < persistSlots; slot++)
	{
		cvarName = CG_BEChat_PersistCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, "");
		}
	}
}
void CG_BEChat_ClearMessageHistory(void)
{
	memset(&cgs.beChatHistory, 0, sizeof(cgs.beChatHistory));
	beChatRowsCacheCount = 0;
	beChatRowsCacheWriteIdx = 0;
	beChatRowsCacheFrameTime = 0;
	beChatRowsCacheTotal = 0;
	beChatFlatCount = 0;
	beChatWindow.historyScroll = 0;
	CG_BEChat_SaveHistoryToCvars();
}
static qboolean CG_BEChat_PersistPopEntry(const char* src, int* ioEnd, char* out, int outSize)
{
	int end;
	int i;
	int sep;
	int outLen;
	int esc;

	if (!src || !ioEnd || !out || outSize < 2)
	{
		return qfalse;
	}

	end = *ioEnd;
	while (end > 0 && src[end - 1] == '|')
	{
		end--;
	}
	if (end <= 0)
	{
		*ioEnd = 0;
		return qfalse;
	}

	sep = -1;
	esc = 0;
	for (i = 0; i < end; i++)
	{
		if (!esc && src[i] == '\\')
		{
			esc = 1;
			continue;
		}
		if (!esc && src[i] == '|')
		{
			sep = i;
		}
		esc = 0;
	}

	outLen = end - (sep + 1);
	if (outLen >= outSize)
	{
		outLen = outSize - 1;
	}
	memcpy(out, src + sep + 1, outLen);
	out[outLen] = '\0';
	*ioEnd = (sep < 0) ? 0 : sep;
	return outLen > 0;
}
void CG_BEChat_RestoreHistoryFromCvars(void)
{
	char cvarBuf[BE_CHAT_PERSIST_CVAR_MAX];
	int slot;
	int persistSlots;
	int restored;

	restored = 0;
	persistSlots = CG_BEChat_PersistHistSlotMax();

	beChatHistoryRestoreActive = qtrue;
	for (slot = persistSlots - 1; slot >= 0 && restored < CG_BEChat_HistoryLineMax(); slot--)
	{
		const char* cvarName;
		int end;

		cvarName = CG_BEChat_PersistCvarName(slot);
		if (!cvarName)
		{
			continue;
		}
		trap_Cvar_VariableStringBuffer(cvarName, cvarBuf, sizeof(cvarBuf));
		if (!cvarBuf[0])
		{
			continue;
		}

		end = strlen(cvarBuf);
		while (end > 0 && restored < CG_BEChat_HistoryLineMax())
		{
			char entry[BE_CHAT_PERSIST_ENTRY_MAX];
			char text[BE_CHAT_LINE_CHARS];
			int sender;
			int msgKind;
			int repeat;
			int idx;

			if (!CG_BEChat_PersistPopEntry(cvarBuf, &end, entry, sizeof(entry)))
			{
				break;
			}
			if (!CG_BEChat_PersistParseEntry(entry, text, sizeof(text), &sender, &msgKind, &repeat))
			{
				continue;
			}

			idx = cgs.beChatHistory.writeIndex % CG_BEChat_HistoryLineMax();
			Q_strncpyz(cgs.beChatHistory.lines[idx].text, text, sizeof(cgs.beChatHistory.lines[idx].text));
			cgs.beChatHistory.lines[idx].senderClientNum = sender;
			cgs.beChatHistory.lines[idx].repeat = repeat;
			cgs.beChatHistory.lines[idx].addTime = cg.time;
			cgs.beChatHistory.lines[idx].msgKind = msgKind;
			cgs.beChatHistory.writeIndex++;
			if (cgs.beChatHistory.count < CG_BEChat_HistoryLineMax())
			{
				cgs.beChatHistory.count++;
			}
			restored++;
		}
	}
	beChatHistoryRestoreActive = qfalse;
}
void CG_BEChat_SaveSentHistToCvars(void)
{
	int indexFromNewest;
	int slot;
	char slotBuf[BE_CHAT_PERSIST_CVAR_MAX];
	char entry[BE_CHAT_SENT_PERSIST_ENTRY_MAX];
	const char* cvarName;
	beChatSentHistEntry_t* e;
	int candidateLen;
	int slotLen;

	slot = 0;
	slotBuf[0] = '\0';
	for (indexFromNewest = 0; indexFromNewest < beChatSentHistCount && slot < BE_CHAT_PERSIST_SLOTS; indexFromNewest++)
	{
		e = CG_BEChat_SentHistEntryNewest(indexFromNewest);
		if (!e || !e->text[0])
		{
			continue;
		}

		Com_sprintf(entry, sizeof(entry), "%i,%i,", e->destMode, e->tellClientNum);
		CG_BEChat_PersistEscapeAppend(entry, sizeof(entry), e->tellName);
		Q_strcat(entry, sizeof(entry), ",");
		CG_BEChat_PersistEscapeAppend(entry, sizeof(entry), e->text);
		Q_strcat(entry, sizeof(entry), ";");

		slotLen = strlen(slotBuf);
		candidateLen = slotLen + (slotLen > 0 ? 1 : 0) + strlen(entry);
		if (candidateLen >= (int)sizeof(slotBuf))
		{
			cvarName = CG_BEChat_PersistSentCvarName(slot);
			if (cvarName)
			{
				trap_Cvar_Set(cvarName, slotBuf);
			}
			slot++;
			if (slot >= BE_CHAT_PERSIST_SLOTS)
			{
				break;
			}
			Q_strncpyz(slotBuf, entry, sizeof(slotBuf));
			continue;
		}

		if (slotLen > 0)
		{
			Q_strcat(slotBuf, sizeof(slotBuf), "|");
		}
		Q_strcat(slotBuf, sizeof(slotBuf), entry);
	}

	if (slot < BE_CHAT_PERSIST_SLOTS)
	{
		cvarName = CG_BEChat_PersistSentCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, slotBuf);
		}
		slot++;
	}
	for (; slot < BE_CHAT_PERSIST_SLOTS; slot++)
	{
		cvarName = CG_BEChat_PersistSentCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, "");
		}
	}
}
void CG_BEChat_RestoreSentHistFromCvars(void)
{
	char cvarBuf[BE_CHAT_PERSIST_CVAR_MAX];
	beChatSentHistEntry_t parsed[BE_CHAT_SENT_HIST_SLOTS];
	int parsedCount;
	int slot;
	int i;

	parsedCount = 0;
	beChatSentHistWrite = 0;
	beChatSentHistCount = 0;

	for (slot = BE_CHAT_PERSIST_SLOTS - 1; slot >= 0 && parsedCount < BE_CHAT_SENT_HIST_SLOTS; slot--)
	{
		const char* cvarName;
		int end;

		cvarName = CG_BEChat_PersistSentCvarName(slot);
		if (!cvarName)
		{
			continue;
		}
		trap_Cvar_VariableStringBuffer(cvarName, cvarBuf, sizeof(cvarBuf));
		if (!cvarBuf[0])
		{
			continue;
		}

		end = strlen(cvarBuf);
		while (end > 0 && parsedCount < BE_CHAT_SENT_HIST_SLOTS)
		{
			char entry[BE_CHAT_SENT_PERSIST_ENTRY_MAX];

			if (!CG_BEChat_PersistPopEntry(cvarBuf, &end, entry, sizeof(entry)))
			{
				break;
			}
			if (!CG_BEChat_PersistParseSentEntry(entry, &parsed[parsedCount]))
			{
				continue;
			}
			parsedCount++;
		}
	}

	if (parsedCount < 1)
	{
		return;
	}

	for (i = 0; i < parsedCount; i++)
	{
		beChatSentHist[i] = parsed[i];
	}
	beChatSentHistCount = parsedCount;
	beChatSentHistWrite = parsedCount % BE_CHAT_SENT_HIST_SLOTS;
}
