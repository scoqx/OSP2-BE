/*
===========================================================================
cg_be_chatwindow.c

In-game chat window (enterchat): history, scroll, text input.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../ui/keycodes.h"

extern qboolean cg_keyCapsLockOnAvail;

#define BE_CHAT_FONT_W_BASE     6.0f
#define BE_CHAT_FONT_H_BASE     10.0f
#define BE_CHAT_INPUT_CHARS     48
#define BE_CHAT_FONT            cgFontIndices.chatWindow
#define BE_CHAT_TEXT_FLAGS      (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL)
#define BE_CHAT_KEEP_OPEN_BIT   2
#define BE_CHAT_INPUT_ONLY_BG_BIT 4
#define BE_CHAT_NO_REPEAT_MERGE_BIT 8
#define BE_CHAT_EMOJI_PREVIEW_SCALE 1.2f
/* Wire escape for '%' (Quake chat breaks on percent). Input/history use '%'; network uses "'/,". */
#define BE_CHAT_PCT_WIRE0       '\''
#define BE_CHAT_PCT_WIRE1       '/'
#define BE_CHAT_PCT_WIRE2       ','

#define BE_CHAT_WIN_DEF_X       220
#define BE_CHAT_WIN_DEF_Y       240
#define BE_CHAT_WIN_DEF_W       200
#define BE_CHAT_WIN_DEF_H       120
#define BE_CHAT_WIN_MIN_W       80.0f

#define BE_CHAT_REPEAT_DELAY_MS     500
#define BE_CHAT_REPEAT_INTERVAL_MS  50
#define BE_CHAT_SCROLL_CTRL_STEP    4
#define BE_CHAT_FLAT_ROWS_MAX       256

#define BE_CHAT_FADE_MAX_MS         800
#define BE_CHAT_FADE_TIME_PERCENT   15
#define BE_CHAT_PERSIST_SLOTS       16
#define BE_CHAT_PERSIST_ENTRY_MAX   (BE_CHAT_LINE_CHARS + 64)
#define BE_CHAT_PERSIST_CVAR_MAX    (MAX_CVAR_VALUE_STRING - 8)
#define BE_CHAT_SENT_HIST_SLOTS     16
#define BE_CHAT_SENT_PERSIST_ENTRY_MAX (MAX_SAY_TEXT + MAX_NAME_LENGTH + 64)

/* Set to 1 to print enterchat mouse debug to console */
#define BE_CHAT_MOUSE_DEBUG     0

#define BE_CHAT_HELP_ALPHA          0.65f
#define BE_CHAT_HELP_TIP_DELAY_MS   800
#define BE_CHAT_HELP_TIP_FADE_MS    100
#define BE_CHAT_HELP_TIP_LINE_COUNT 7
#define BE_CHAT_HELP_TIP_BG_ALPHA     0.8f
#define BE_CHAT_HELP_QMARK_FLAGS    (DS_HCENTER | DS_VCENTER | DS_PROPORTIONAL)
#define BE_CHAT_HELP_TIP_FLAGS      (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL)


typedef struct
{
	qboolean    open;
	qboolean    historyOnly;
	int         historyScroll;
	beChatInput_t input;
	float       cursorX;    /* screen pixels, hotspot center */
	float       cursorY;    /* screen pixels, hotspot center */
	qboolean    keyDown[256];
	int         inputGraceUntil;
	qboolean    sbActive;
	qboolean    sbDragging;
	float       sbTrackX;
	float       sbTrackY;
	float       sbTrackW;
	float       sbTrackH;
	float       sbThumbH;
	float       sbThumbY;
	float       sbGrabDeltaY;
	float       sbMaxScrollPx;
	int         sbMaxScrollLines;
	int         repeatKey;
	int         repeatNextTime;
	int         destMode;
	int         tellClientNum;
	char        tellName[MAX_NAME_LENGTH];
	qboolean    helpHover;
	int         helpHoverStartTime;
	qboolean    helpTipFadingOut;
	int         helpTipHideStartTime;
	int         latchCloseKey; /* key held at open (bind); repeat press closes */
	int         sentHistBrowse; /* -1 draft; 0 newest sent; higher = older */
	int         savedChannelDest;
	int         channelDest; /* BE_CHAT_DEST_SAY or BE_CHAT_DEST_TEAM while chat is open */
} beChatWindowState_t;

typedef struct
{
	char    text[MAX_SAY_TEXT];
	int     destMode;
	int     tellClientNum;
	char    tellName[MAX_NAME_LENGTH];
} beChatSentHistEntry_t;

#define BE_CHAT_DEST_SAY        0
#define BE_CHAT_DEST_TEAM       1
#define BE_CHAT_DEST_TELL       2
#define BE_CHAT_DEST_REPLY      3
#define BE_CHAT_DEST_MUTE_PICK  4
#define BE_CHAT_DEST_MUTE_CONFIRM 5
#define BE_CHAT_DEST_UNMUTE_PICK 6

#define BE_CHAT_CMD_AC_MAX      32
#define BE_CHAT_CMD_TELL        "tell"
#define BE_CHAT_CMD_REPLY       "reply"
#define BE_CHAT_CMD_MUTE        "mute"
#define BE_CHAT_CMD_UNMUTE      "unmute"

#define BE_CHAT_CMD_AC_MODE_SLASH_CMD       1
#define BE_CHAT_CMD_AC_MODE_TELL_NAME       2
#define BE_CHAT_CMD_AC_MODE_MUTE_PICK_DIGIT 3
#define BE_CHAT_CMD_AC_MODE_UNMUTE_PICK_DIGIT 4
#define BE_CHAT_CMD_AC_MODE_MUTE_COMMIT     5
#define BE_CHAT_CMD_AC_MODE_UNMUTE_COMMIT   6
#define BE_CHAT_CMD_AC_MODE_PLAIN_CMD       7

static char beChatPromptText[64];

typedef struct
{
	int         mode;
	int         count;
	int         sel;
	int         replaceFrom;
	int         replaceTo;
	char        entries[BE_CHAT_CMD_AC_MAX][MAX_NAME_LENGTH];
} beChatCmdAc_t;

static beChatWindowState_t beChatWindow;
static beChatCmdAc_t beChatCmdAc;
static beChatSentHistEntry_t beChatSentHist[BE_CHAT_SENT_HIST_SLOTS];
static int beChatSentHistWrite;
static int beChatSentHistCount;

static beChatSentHistEntry_t* CG_BEChat_SentHistEntryNewest(int indexFromNewest);
static void CG_BEChat_ResetDestMode(void);
static void CG_BEChat_InputClear(beChatInput_t* edit);
static qboolean CG_BEChat_LineCmdChannelOpen(void);
static qboolean CG_BEChat_LineSlashCmdAfter(const char* buf, const char* cmdWord,
        int* outNameStart);
static void CG_BEChat_CmdAcRefresh(beChatInput_t* edit);
static void CG_BEChat_CmdAcComplete(beChatInput_t* edit, float maxTextW);
static qboolean CG_BEChat_CmdAcRefreshMuteUnmutePickDigits(beChatInput_t* edit, const char* buf,
        int cursor);
static qboolean CG_BEChat_CmdAcRefreshMuteUnmuteLine(beChatInput_t* edit, const char* buf,
        int cursor);

static qboolean CG_BEChat_ConsoleBlocksInput(void)
{
	if (trap_Key_GetCatcher() & KEYCATCH_CONSOLE)
	{
		return qtrue;
	}

	return qfalse;
}

static const char* beChatHelpTipLines[BE_CHAT_HELP_TIP_LINE_COUNT] =
{
	"^2Enter^7  send message",
	"^2Esc^7  close without sending",
	"^2Tab^7  switch global and team chat",
	"^2Alt^7  emoji picker; type :name, then ^2Tab^7",
	"Scroll: ^2wheel^7, ^2PgUp^7 / ^2PgDn^7, ^2Ctrl+Up^7 / ^2Ctrl+Down^7",
	"Slash line: /tell, /reply, /mute, /unmute, or player name",
	"^2Up^7 / ^2Down^7  last sent (empty input) or autocomplete",
};

static qboolean beChatMouse1Prev;
static qboolean beChatHistoryRestoreActive;

static int beChatRowsCacheWriteIdx;
static int beChatRowsCacheCount;
static float beChatRowsCacheWidth;
static qboolean beChatRowsCacheTimeFilter;
static int beChatRowsCacheFrameTime;
static int beChatRowsCacheTotal;
static int beChatRowsCacheRows[BE_CHAT_HISTORY_LINES];
static int beChatFlatCount;
static qboolean beChatFlatFull;
static char beChatFlatText[BE_CHAT_FLAT_ROWS_MAX][BE_CHAT_LINE_CHARS];
static int beChatFlatLinesBack[BE_CHAT_FLAT_ROWS_MAX];

static int beChatWinLayoutFrame = -1;
static float beChatWinLayoutX;
static float beChatWinLayoutY;
static float beChatWinLayoutW;
static float beChatWinLayoutH;

void CG_BEChat_UpdateFontSizeFromCvar(void)
{
	beChatWindowMetrics_t* m;
	float w;
	float h;
	float scale;
	float sbBorder;
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;

	m = &cgs.beChatMetrics;
	w = BE_CHAT_FONT_W_BASE;
	h = BE_CHAT_FONT_H_BASE;
	token1 = NULL;
	token2 = NULL;

	if (cg_chatWindowFontSize.string[0] != '\0')
	{
		Q_strncpyz(buffer, cg_chatWindowFontSize.string, sizeof(buffer));
		token1 = Q_strtok(buffer, " \t");
		token2 = Q_strtok(NULL, " \t");
	}

	if (token1 && token2)
	{
		w = (float)atof(token1);
		h = (float)atof(token2);
	}
	else if (token1)
	{
		h = (float)atof(token1);
		w = h * (BE_CHAT_FONT_W_BASE / BE_CHAT_FONT_H_BASE);
		if (cgs.glconfig.vidWidth > 0 && cgs.glconfig.vidHeight > 0)
		{
			w *= (float)cgs.glconfig.vidHeight * 640.0f /
			     ((float)cgs.glconfig.vidWidth * 480.0f);
		}
	}

	scale = h / BE_CHAT_FONT_H_BASE;
	sbBorder = scale;
	if (sbBorder < 1.0f)
	{
		sbBorder = 1.0f;
	}

	m->charW = w;
	m->charH = h;
	m->pad = 4.0f * scale;
	m->inputRowH = 14.0f * scale;
	m->inputRowBit4Y = 1.0f * scale;
	m->sbW = 3.0f * scale;
	m->sbGap = 3.0f * scale;
	m->sbHitPad = 3.0f * scale;
	m->sbBorder = sbBorder;
	m->cursorW = 32.0f * scale;
	m->cursorH = 32.0f * scale;
	m->emojiBtnPad = 2.0f * scale;
	m->emojiBtnHitPad = 2.0f * scale;
	m->emojiBtnSize = h + m->emojiBtnPad * 2.0f;
	m->emojiPanelPad = 3.0f * scale;
	m->emojiWinPad = 4.0f * scale;
	m->emojiPreviewGap = 1.0f * scale;
	m->emojiTipPad = 2.0f * scale;
	m->helpBtnSize = h * 1.15f + m->pad * 0.25f;

	beChatWinLayoutFrame = -1;
	beChatRowsCacheCount = 0;
	beChatRowsCacheFrameTime = 0;
	beChatRowsCacheTotal = 0;
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

static beChatHistoryLine_t* CG_BEChat_HistoryEntry(int linesBack)
{
	beChatHistory_t* history;
	int index;

	history = &cgs.beChatHistory;
	if (linesBack < 0 || linesBack >= history->count)
	{
		return NULL;
	}

	index = history->writeIndex - 1 - linesBack;
	while (index < 0)
	{
		index += BE_CHAT_HISTORY_LINES;
	}
	index %= BE_CHAT_HISTORY_LINES;
	return &history->lines[index];
}

static void CG_BEChat_SaveHistoryToCvars(void)
{
	int linesBack;
	int slot;
	char slotBuf[BE_CHAT_PERSIST_CVAR_MAX];
	char entry[BE_CHAT_PERSIST_ENTRY_MAX];
	const char* cvarName;
	beChatHistoryLine_t* line;
	int candidateLen;
	int slotLen;

	slot = 0;
	slotBuf[0] = '\0';
	for (linesBack = 0; linesBack < cgs.beChatHistory.count && slot < BE_CHAT_PERSIST_SLOTS; linesBack++)
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
		cvarName = CG_BEChat_PersistCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, slotBuf);
		}
		slot++;
	}
	for (; slot < BE_CHAT_PERSIST_SLOTS; slot++)
	{
		cvarName = CG_BEChat_PersistCvarName(slot);
		if (cvarName)
		{
			trap_Cvar_Set(cvarName, "");
		}
	}
}

static void CG_BEChat_ClearMessageHistory(void)
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

static void CG_BEChat_RestoreHistoryFromCvars(void)
{
	char cvarBuf[BE_CHAT_PERSIST_CVAR_MAX];
	int slot;
	int restored;

	restored = 0;

	beChatHistoryRestoreActive = qtrue;
	for (slot = BE_CHAT_PERSIST_SLOTS - 1; slot >= 0 && restored < BE_CHAT_HISTORY_LINES; slot--)
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
		while (end > 0 && restored < BE_CHAT_HISTORY_LINES)
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

			idx = cgs.beChatHistory.writeIndex % BE_CHAT_HISTORY_LINES;
			Q_strncpyz(cgs.beChatHistory.lines[idx].text, text, sizeof(cgs.beChatHistory.lines[idx].text));
			cgs.beChatHistory.lines[idx].senderClientNum = sender;
			cgs.beChatHistory.lines[idx].repeat = repeat;
			cgs.beChatHistory.lines[idx].addTime = cg.time;
			cgs.beChatHistory.lines[idx].msgKind = msgKind;
			cgs.beChatHistory.writeIndex++;
			if (cgs.beChatHistory.count < BE_CHAT_HISTORY_LINES)
			{
				cgs.beChatHistory.count++;
			}
			restored++;
		}
	}
	beChatHistoryRestoreActive = qfalse;
}

static void CG_BEChat_SaveSentHistToCvars(void)
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

static void CG_BEChat_RestoreSentHistFromCvars(void)
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

void CG_BEChat_Init(void)
{
	memset(&beChatWindow, 0, sizeof(beChatWindow));
	beChatWindow.input.widthInChars = BE_CHAT_INPUT_CHARS;
	beChatWindow.repeatKey = -1;
	beChatWindow.latchCloseKey = -1;
	beChatWindow.sentHistBrowse = -1;
	beChatMouse1Prev = qfalse;
	beChatHistoryRestoreActive = qfalse;
	CG_BEChat_UpdateFontSizeFromCvar();
	CG_BEChat_RestoreHistoryFromCvars();
	CG_BEChat_RestoreSentHistFromCvars();
}

static void CG_BEChat_ParseCvarPairInts(const char* str, int defA, int defB, float* outA, float* outB)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* tokenA;
	char* tokenB;

	if (!outA || !outB)
	{
		return;
	}

	*outA = (float)defA;
	*outB = (float)defB;

	if (!str || !str[0])
	{
		return;
	}

	Q_strncpyz(buffer, str, sizeof(buffer));
	tokenA = Q_strtok(buffer, " \t");
	tokenB = Q_strtok(NULL, " \t");
	if (tokenA)
	{
		*outA = (float)atoi(tokenA);
	}
	if (tokenB)
	{
		*outB = (float)atoi(tokenB);
	}
}

static void CG_BEChat_RefreshWindowLayoutCache(void)
{
	float x;
	float y;
	float w;
	float h;
	float minH;

	CG_BEChat_ParseCvarPairInts(cg_chatWindowPos.string, BE_CHAT_WIN_DEF_X, BE_CHAT_WIN_DEF_Y, &x, &y);
	CG_BEChat_ParseCvarPairInts(cg_chatWindowSize.string, BE_CHAT_WIN_DEF_W, BE_CHAT_WIN_DEF_H, &w, &h);

	minH = BECHAT_INPUT_ROW_H + BECHAT_PAD * 2.0f + BECHAT_CHAR_H;
	if (w < BE_CHAT_WIN_MIN_W)
	{
		w = BE_CHAT_WIN_MIN_W;
	}
	if (h < minH)
	{
		h = minH;
	}

	beChatWinLayoutX = x;
	beChatWinLayoutY = y;
	beChatWinLayoutW = w;
	beChatWinLayoutH = h;
}

void CG_BEChat_WindowLayout(float* outX, float* outY, float* outW, float* outH)
{
	if (beChatWinLayoutFrame != cg.clientFrame)
	{
		beChatWinLayoutFrame = cg.clientFrame;
		CG_BEChat_RefreshWindowLayoutCache();
	}

	if (outX)
	{
		*outX = beChatWinLayoutX;
	}
	if (outY)
	{
		*outY = beChatWinLayoutY;
	}
	if (outW)
	{
		*outW = beChatWinLayoutW;
	}
	if (outH)
	{
		*outH = beChatWinLayoutH;
	}
}

float CG_BEChat_InputRowY(float winY, float winH)
{
	return winY + winH - BECHAT_INPUT_ROW_H;
}

float CG_BEChat_InputRowBgY(float winY, float winH)
{
	float bgY;

	bgY = CG_BEChat_InputRowY(winY, winH);
	if (cg_chatWindow.integer & BE_CHAT_INPUT_ONLY_BG_BIT)
	{
		bgY -= BECHAT_INPUT_ROW_BIT4_Y;
	}
	return bgY;
}

static void CG_BEChat_CmdAcClear(void)
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

static void CG_BEChat_CmdAcRefresh(beChatInput_t* edit)
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

static void CG_BEChat_ResetDestMode(void)
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

static void CG_BEChat_CancelMuteConfirm(void)
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

static qboolean CG_BEChat_TellClientFromIdToken(const char* token, int* outClientNum)
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

static qboolean CG_BEChat_TryEnterMutePickFromBuffer(beChatInput_t* edit)
{
	return CG_BEChat_TryEnterSlashPickFromBuffer(edit, CG_BEChat_LineMuteAfterCmd,
	        BE_CHAT_DEST_MUTE_PICK);
}

static qboolean CG_BEChat_TryEnterUnmutePickFromBuffer(beChatInput_t* edit)
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

static qboolean CG_BEChat_TryCommitMutePick(beChatInput_t* edit)
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

static qboolean CG_BEChat_TryCommitUnmutePick(beChatInput_t* edit)
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

static qboolean CG_BEChat_TryCommitMuteIdLine(beChatInput_t* edit)
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

static qboolean CG_BEChat_TryCommitUnmuteIdLine(beChatInput_t* edit)
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

static int CG_BEChat_ClientFromTellName(const char* name)
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

static qboolean CG_BEChat_TryCommitTellTarget(beChatInput_t* edit)
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

static qboolean CG_BEChat_IsReplyLinePrefix(const char* buf, int* outMsgStart)
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

static qboolean CG_BEChat_TryCommitReply(beChatInput_t* edit)
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

static void CG_BEChat_CmdAcComplete(beChatInput_t* edit, float maxTextW)
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

#define BE_CHAT_CMD_AC_PREVIEW_SCALE  1.2f

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
	         beChatCmdAc.mode == BE_CHAT_CMD_AC_MODE_PLAIN_CMD)
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

static void CG_BEChat_ProcessTabKey(float maxTextW)
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

static beChatSentHistEntry_t* CG_BEChat_SentHistEntryNewest(int indexFromNewest)
{
	int idx;

	if (indexFromNewest < 0 || indexFromNewest >= beChatSentHistCount)
	{
		return NULL;
	}

	idx = beChatSentHistWrite - 1 - indexFromNewest;
	while (idx < 0)
	{
		idx += BE_CHAT_SENT_HIST_SLOTS;
	}

	return &beChatSentHist[idx];
}

static void CG_BEChat_SentHistPush(const char* text, int destMode, int tellClientNum, const char* tellName)
{
	beChatSentHistEntry_t* e;
	beChatSentHistEntry_t* prev;

	if (!text || !text[0])
	{
		return;
	}

	if (beChatSentHistCount > 0)
	{
		prev = CG_BEChat_SentHistEntryNewest(0);
		if (prev && !strcmp(prev->text, text) && prev->destMode == destMode &&
		        prev->tellClientNum == tellClientNum &&
		        !Q_stricmp(prev->tellName, tellName ? tellName : ""))
		{
			return;
		}
	}

	e = &beChatSentHist[beChatSentHistWrite];
	Q_strncpyz(e->text, text, sizeof(e->text));
	e->destMode = destMode;
	e->tellClientNum = tellClientNum;
	if (tellName)
	{
		Q_strncpyz(e->tellName, tellName, sizeof(e->tellName));
	}
	else
	{
		e->tellName[0] = '\0';
	}

	beChatSentHistWrite = (beChatSentHistWrite + 1) % BE_CHAT_SENT_HIST_SLOTS;
	if (beChatSentHistCount < BE_CHAT_SENT_HIST_SLOTS)
	{
		beChatSentHistCount++;
	}

	CG_BEChat_SaveSentHistToCvars();
}

static qboolean CG_BEChat_SentHistCanNavigate(void)
{
	if (beChatSentHistCount <= 0)
	{
		return qfalse;
	}

	if (beChatWindow.sentHistBrowse >= 0)
	{
		return qtrue;
	}

	return beChatWindow.input.buffer[0] == '\0' ? qtrue : qfalse;
}

static void CG_BEChat_SentHistApply(int browseIdx, float maxTextW)
{
	beChatSentHistEntry_t* e;

	e = CG_BEChat_SentHistEntryNewest(browseIdx);
	if (!e)
	{
		return;
	}

	Q_strncpyz(beChatWindow.input.buffer, e->text, sizeof(beChatWindow.input.buffer));
	beChatWindow.input.cursor = strlen(beChatWindow.input.buffer);
	beChatWindow.destMode = e->destMode;
	beChatWindow.tellClientNum = e->tellClientNum;
	Q_strncpyz(beChatWindow.tellName, e->tellName, sizeof(beChatWindow.tellName));
	beChatWindow.sentHistBrowse = browseIdx;
	CG_BEChat_EmojiOnInputChanged(&beChatWindow.input);
	CG_BEChat_CmdAcRefresh(&beChatWindow.input);
	CG_BEChat_InputClampScroll(&beChatWindow.input, maxTextW);
}

static void CG_BEChat_SentHistClearDraft(void)
{
	beChatWindow.input.buffer[0] = '\0';
	beChatWindow.input.cursor = 0;
	beChatWindow.input.scroll = 0;
	beChatWindow.sentHistBrowse = -1;
	CG_BEChat_EmojiOnInputChanged(&beChatWindow.input);
	CG_BEChat_CmdAcClear();
}

static qboolean CG_BEChat_SentHistProcessKey(int key, float maxTextW)
{
	int next;

	if (trap_Key_IsDown(K_CTRL))
	{
		return qfalse;
	}

	if (CG_BEChat_EmojiAcIsActive() || CG_BEChat_CmdAcIsActive())
	{
		return qfalse;
	}

	if (!CG_BEChat_SentHistCanNavigate())
	{
		return qfalse;
	}

	if (key == K_UPARROW || key == K_KP_UPARROW)
	{
		if (beChatWindow.sentHistBrowse < 0)
		{
			next = 0;
		}
		else
		{
			next = beChatWindow.sentHistBrowse + 1;
		}

		if (next >= beChatSentHistCount)
		{
			return qtrue;
		}

		CG_BEChat_SentHistApply(next, maxTextW);
		return qtrue;
	}

	if (key == K_DOWNARROW || key == K_KP_DOWNARROW)
	{
		if (beChatWindow.sentHistBrowse < 0)
		{
			return qtrue;
		}

		if (beChatWindow.sentHistBrowse == 0)
		{
			CG_BEChat_SentHistClearDraft();
			CG_BEChat_InputClampScroll(&beChatWindow.input, maxTextW);
			return qtrue;
		}

		CG_BEChat_SentHistApply(beChatWindow.sentHistBrowse - 1, maxTextW);
		return qtrue;
	}

	return qfalse;
}

static void CG_BEChat_InputClear(beChatInput_t* edit)
{
	edit->buffer[0] = '\0';
	edit->cursor = 0;
	edit->scroll = 0;
	CG_BEChat_EmojiOnInputChanged(edit);
	CG_BEChat_CmdAcClear();
	if (beChatWindow.destMode != BE_CHAT_DEST_SAY &&
	        beChatWindow.destMode != BE_CHAT_DEST_TEAM)
	{
		CG_BEChat_ResetDestMode();
	}
	beChatWindow.sentHistBrowse = -1;
}

static qboolean CG_BEChat_KeyIsLatchTextInput(int key)
{
	if (key >= K_SPACE && key < K_BACKSPACE)
	{
		return qtrue;
	}
	switch (key)
	{
	case K_KP_INS:
	case K_KP_END:
	case K_KP_DOWNARROW:
	case K_KP_PGDN:
	case K_KP_LEFTARROW:
	case K_KP_5:
	case K_KP_RIGHTARROW:
	case K_KP_HOME:
	case K_KP_UPARROW:
	case K_KP_PGUP:
		return qtrue;
	default:
		return qfalse;
	}
}

static qboolean CG_BEChat_KeyIsLatchCandidate(int key)
{
	if (key < 0 || key >= 256)
	{
		return qfalse;
	}
	if (CG_BEChat_KeyIsLatchTextInput(key))
	{
		return qfalse;
	}
	if (key == K_ESCAPE || key == K_ENTER || key == K_KP_ENTER)
	{
		return qfalse;
	}
	if (key == K_MOUSE1 || key == K_MWHEELUP || key == K_MWHEELDOWN)
	{
		return qfalse;
	}
	if (key == K_SHIFT)
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_BEChat_KeyIsLatchModifier(int key)
{
	return (key == K_ALT || key == K_CTRL);
}

static int CG_BEChat_PickLatchModifier(const int* candidates, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		if (candidates[i] == K_ALT)
		{
			return K_ALT;
		}
	}
	for (i = 0; i < n; i++)
	{
		if (candidates[i] == K_CTRL)
		{
			return K_CTRL;
		}
	}
	return candidates[0];
}

static void CG_BEChat_LatchCloseKeyFromOpenBind(void)
{
	int candidates[16];
	int n;
	int k;
	int i;
	int mainKey;

	beChatWindow.latchCloseKey = -1;
	n = 0;
	for (k = 0; k < 256; k++)
	{
		if (!trap_Key_IsDown(k))
		{
			continue;
		}
		if (!CG_BEChat_KeyIsLatchCandidate(k))
		{
			continue;
		}
		if (n < (int)(sizeof(candidates) / sizeof(candidates[0])))
		{
			candidates[n++] = k;
		}
	}

	if (n == 0)
	{
		return;
	}
	if (n == 1)
	{
		beChatWindow.latchCloseKey = candidates[0];
		return;
	}

	mainKey = -1;
	for (i = 0; i < n; i++)
	{
		k = candidates[i];
		if (!CG_BEChat_KeyIsLatchModifier(k))
		{
			mainKey = k;
		}
	}
	if (mainKey >= 0)
	{
		beChatWindow.latchCloseKey = mainKey;
	}
	else
	{
		beChatWindow.latchCloseKey = CG_BEChat_PickLatchModifier(candidates, n);
	}
}

void CG_BEChat_Close(void)
{
	if (!beChatWindow.open)
	{
		return;
	}

	beChatWindow.latchCloseKey = -1;
	CG_BEChat_EmojiSessionClose();
	CG_BEChat_ResetDestMode();
	beChatWindow.open = qfalse;
	beChatWindow.historyOnly = qfalse;
	beChatWindow.historyScroll = 0;
	beChatWindow.repeatKey = -1;
	memset(beChatWindow.keyDown, 0, sizeof(beChatWindow.keyDown));
	beChatWindow.sbDragging = qfalse;
	beChatMouse1Prev = qfalse;
	beChatWindow.helpHover = qfalse;
	beChatWindow.helpTipFadingOut = qfalse;
	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
}

static void CG_BEChat_CursorScreenRect(float* x, float* y, float* w, float* h)
{
	float virtX;
	float virtY;

	virtX = beChatWindow.cursorX / cgs.screenXScale_Old;
	virtY = beChatWindow.cursorY / cgs.screenYScale_Old;
	*x = virtX - BECHAT_CURSOR_W * 0.5f;
	*y = virtY - BECHAT_CURSOR_H * 0.5f;
	*w = BECHAT_CURSOR_W;
	*h = BECHAT_CURSOR_H;
	CG_AdjustFrom640_Old(x, y, w, h, qtrue);
}

static void CG_BEChat_VirtualFromCursor(float* virtX, float* virtY)
{
	float x;
	float y;
	float w;
	float h;

	/* Hit point = center of drawn cursor sprite (correctWide), not raw cursorX/Y. */
	CG_BEChat_CursorScreenRect(&x, &y, &w, &h);
	if (virtX)
	{
		*virtX = (x + w * 0.5f) / cgs.screenXScale_Old;
	}
	if (virtY)
	{
		*virtY = (y + h * 0.5f) / cgs.screenYScale_Old;
	}
}

void CG_BEChat_VirtualCursorPos(float* virtX, float* virtY)
{
	CG_BEChat_VirtualFromCursor(virtX, virtY);
}

static void CG_BEChat_CursorPlaceVirtual(float virtX, float virtY)
{
	beChatWindow.cursorX = virtX * cgs.screenXScale_Old;
	beChatWindow.cursorY = virtY * cgs.screenYScale_Old;
}

static void CG_BEChat_OpenEx(qboolean teamDest)
{
	int catcher;

	if (!cg_chatWindow.integer)
	{
		return;
	}

	if (!cg.snap)
	{
		return;
	}

	catcher = trap_Key_GetCatcher();
	if (catcher & (KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE))
	{
		return;
	}

	if (beChatWindow.open)
	{
		CG_BEChat_Close();
		return;
	}

	beChatWindow.open = qtrue;
	beChatWindow.historyOnly = qfalse;
	beChatWindow.historyScroll = 0;
	if (teamDest)
	{
		beChatWindow.channelDest = BE_CHAT_DEST_TEAM;
	}
	else
	{
		beChatWindow.channelDest = BE_CHAT_DEST_SAY;
	}
	CG_BEChat_ResetDestMode();
	CG_BEChat_CmdAcClear();
	CG_BEChat_InputClear(&beChatWindow.input);
	CG_BEChat_EmojiSessionOpen();
	memset(beChatWindow.keyDown, 0, sizeof(beChatWindow.keyDown));
	beChatWindow.repeatKey = -1;
	{
		float winX;
		float winY;
		float winW;
		float winH;

		CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
		CG_BEChat_CursorPlaceVirtual(winX + winW * 0.5f,
		                             CG_BEChat_InputRowY(winY, winH) + BECHAT_INPUT_ROW_H * 0.5f);
	}
	beChatWindow.inputGraceUntil = cg.time + 50;
	CG_BEChat_LatchCloseKeyFromOpenBind();
	beChatMouse1Prev = trap_Key_IsDown(K_MOUSE1);
	trap_Key_SetCatcher(catcher | KEYCATCH_CGAME);
}

void CG_BEChat_Open(void)
{
	CG_BEChat_OpenEx(qfalse);
}

void CG_BEChat_OpenHistoryView(void)
{
	if (!cg_chatWindow.integer)
	{
		return;
	}

	if (!cg.snap)
	{
		return;
	}

	if (beChatWindow.open)
	{
		return;
	}

	if (cgs.beChatHistory.count == 0)
	{
		return;
	}

	beChatWindow.open = qtrue;
	beChatWindow.historyOnly = qtrue;
	beChatWindow.historyScroll = 0;
	beChatWindow.repeatKey = -1;
	beChatWindow.sbDragging = qfalse;
	beChatWindow.sbActive = qfalse;
}

void CG_BEChat_CloseHistoryView(void)
{
	if (beChatWindow.open && beChatWindow.historyOnly)
	{
		CG_BEChat_Close();
	}
}

static void CG_BEChat_InvalidateRowsCache(void)
{
	beChatRowsCacheWriteIdx = -1;
}

static void CG_BEChat_EncodePercentWire(const char* src, char* dst, int dstSize)
{
	int si;
	int di;

	if (!dst || dstSize < 1)
	{
		return;
	}

	if (!src)
	{
		dst[0] = '\0';
		return;
	}

	si = 0;
	di = 0;
	while (src[si] && di < dstSize - 1)
	{
		if (src[si] == '%')
		{
			if (di + 3 >= dstSize)
			{
				break;
			}
			dst[di++] = BE_CHAT_PCT_WIRE0;
			dst[di++] = BE_CHAT_PCT_WIRE1;
			dst[di++] = BE_CHAT_PCT_WIRE2;
			si++;
			continue;
		}
		dst[di++] = src[si++];
	}
	dst[di] = '\0';
}

static void CG_BEChat_DecodePercentWire(const char* src, char* dst, int dstSize)
{
	int si;
	int di;

	if (!dst || dstSize < 1)
	{
		return;
	}

	if (!src)
	{
		dst[0] = '\0';
		return;
	}

	si = 0;
	di = 0;
	while (src[si] && di < dstSize - 1)
	{
		if (src[si] == BE_CHAT_PCT_WIRE0 && src[si + 1] == BE_CHAT_PCT_WIRE1 &&
		        src[si + 2] == BE_CHAT_PCT_WIRE2)
		{
			dst[di++] = '%';
			si += 3;
			continue;
		}
		dst[di++] = src[si++];
	}
	dst[di] = '\0';
}

void CG_BEChat_AddLine(const char* text, int senderClientNum, int msgKind)
{
	beChatHistory_t* history;
	beChatHistoryLine_t* last;
	int index;
	int lastIndex;
	char decoded[BE_CHAT_LINE_CHARS];

	if (!text || !text[0])
	{
		return;
	}

	if (!cg_chatWindow.integer)
	{
		return;
	}

	CG_BEChat_DecodePercentWire(text, decoded, sizeof(decoded));
	text = decoded;

	history = &cgs.beChatHistory;

	if (!(cg_chatWindow.integer & BE_CHAT_NO_REPEAT_MERGE_BIT) && history->count > 0)
	{
		lastIndex = history->writeIndex - 1;
		while (lastIndex < 0)
		{
			lastIndex += BE_CHAT_HISTORY_LINES;
		}
		lastIndex %= BE_CHAT_HISTORY_LINES;
		last = &history->lines[lastIndex];
		if (last->senderClientNum == senderClientNum && last->msgKind == msgKind &&
		        !Q_stricmp(last->text, text))
		{
			if (last->repeat < 1)
			{
				last->repeat = 1;
			}
			last->repeat++;
			last->addTime = cg.time;
			CG_BEChat_InvalidateRowsCache();
			if (!beChatHistoryRestoreActive)
			{
				CG_BEChat_SaveHistoryToCvars();
			}
			return;
		}
	}

	index = history->writeIndex % BE_CHAT_HISTORY_LINES;
	Q_strncpyz(history->lines[index].text, text, sizeof(history->lines[index].text));
	history->lines[index].senderClientNum = senderClientNum;
	history->lines[index].repeat = 1;
	history->lines[index].addTime = cg.time;
	history->lines[index].msgKind = msgKind;
	history->writeIndex++;
	if (history->count < BE_CHAT_HISTORY_LINES)
	{
		history->count++;
	}
	if (!beChatHistoryRestoreActive)
	{
		CG_BEChat_SaveHistoryToCvars();
	}

	if (beChatWindow.open && beChatWindow.historyScroll == 0)
	{
		/* pinned to bottom */
	}
}

int CG_BEChat_HistorySenderClientNum(int linesBack)
{
	beChatHistory_t* history;
	int index;

	history = &cgs.beChatHistory;
	if (linesBack < 0 || linesBack >= history->count)
	{
		return -1;
	}

	index = history->writeIndex - 1 - linesBack;
	while (index < 0)
	{
		index += BE_CHAT_HISTORY_LINES;
	}
	index %= BE_CHAT_HISTORY_LINES;
	return history->lines[index].senderClientNum;
}

static int CG_BEChat_VisibleHistoryLines(void)
{
	float historyH;
	float winH;

	CG_BEChat_WindowLayout(NULL, NULL, NULL, &winH);
	/* Same history band as when the input row is shown (closed overlay matches open layout). */
	historyH = winH - BECHAT_INPUT_ROW_H - (BECHAT_PAD * 2.0f);
	if (historyH < BECHAT_CHAR_H)
	{
		return 1;
	}
	return (int)(historyH / BECHAT_CHAR_H);
}






float CG_BEChat_InputFieldMaxWidth(void)
{
	float winX;
	float winY;
	float winW;
	float winH;

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_EmojiPrepareInputRow(winX, winY, winW, winH);
	return CG_BEChat_EmojiInputTextMaxWidth(winX + BECHAT_PAD);
}

static void CG_BEChat_MessageDisplayText(int linesBack, char* out, int outSize)
{
	beChatHistoryLine_t* line;

	line = CG_BEChat_HistoryEntry(linesBack);
	if (!line || !out || outSize < 1)
	{
		if (out && outSize > 0)
		{
			out[0] = '\0';
		}
		return;
	}

	if (!(cg_chatWindow.integer & BE_CHAT_NO_REPEAT_MERGE_BIT) && line->repeat > 1)
	{
		if (line->repeat > 99)
		{
			Com_sprintf(out, outSize, "%s ^7x99+", line->text);
		}
		else
		{
			Com_sprintf(out, outSize, "%s ^7x%i", line->text, line->repeat);
		}
	}
	else
	{
		Q_strncpyz(out, line->text, outSize);
	}
}

static qboolean CG_BEChat_MessagePassesTimeFilter(int linesBack, qboolean timeFilter)
{
	beChatHistoryLine_t* line;

	if (!timeFilter)
	{
		return qtrue;
	}

	if (cg_chatWindowMessageTime.integer <= 0)
	{
		return qfalse;
	}

	line = CG_BEChat_HistoryEntry(linesBack);
	if (!line)
	{
		return qfalse;
	}

	if (cg.time - line->addTime > cg_chatWindowMessageTime.integer)
	{
		return qfalse;
	}

	return qtrue;
}

static float CG_BEChat_MessageTimeAlpha(int linesBack)
{
	beChatHistoryLine_t* line;
	int msgTime;
	int fadeMs;
	int fade;
	int age;
	int fadeStart;

	line = CG_BEChat_HistoryEntry(linesBack);
	if (!line)
	{
		return 0.0f;
	}

	msgTime = cg_chatWindowMessageTime.integer;
	if (msgTime <= 0)
	{
		return 0.0f;
	}

	age = cg.time - line->addTime;
	if (age >= msgTime)
	{
		return 0.0f;
	}

	fade = (msgTime * BE_CHAT_FADE_TIME_PERCENT) / 100;
	if (fade > BE_CHAT_FADE_MAX_MS)
	{
		fade = BE_CHAT_FADE_MAX_MS;
	}
	if (fade > msgTime)
	{
		fade = msgTime;
	}
	fadeMs = fade;
	if (fadeMs <= 0)
	{
		return 1.0f;
	}

	fadeStart = msgTime - fadeMs;
	if (age <= fadeStart)
	{
		return 1.0f;
	}

	return 1.0f - (float)(age - fadeStart) / (float)fadeMs;
}

static char CG_BEChat_ActiveColorAt(const char* text, int byteIndex, char defaultColor)
{
	const char* p;
	const char* end;
	char active;

	active = defaultColor;
	if (!text || byteIndex <= 0)
	{
		return active;
	}

	end = text + byteIndex;
	p = text;
	while (p < end && *p)
	{
		if (Q_IsColorString(p))
		{
			active = *(p + 1);
			p += 2;
			continue;
		}
		p++;
	}

	return active;
}

static qboolean CG_BEChat_InputPaletteColorChar(char c)
{
	if (c >= '0' && c <= '9')
	{
		return qtrue;
	}
	if (c >= 'a' && c <= 'z')
	{
		return qtrue;
	}
	if (c >= 'A' && c <= 'Z')
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_BEChat_InputPaletteCodeAt(const char* buf, int pos, int bufLen)
{
	if (!buf || pos < 0 || pos + 2 > bufLen)
	{
		return qfalse;
	}
	if (!Q_IsColorString(buf + pos))
	{
		return qfalse;
	}
	return CG_BEChat_InputPaletteColorChar(buf[pos + 1]);
}

static int CG_BEChat_InputCopyDisplay(const char* buf, int start, int end, char* out, int outSize)
{
	int i;
	int o;
	int bufLen;

	if (!buf || !out || outSize < 1)
	{
		return 0;
	}

	bufLen = strlen(buf);
	if (start < 0)
	{
		start = 0;
	}
	if (end > bufLen)
	{
		end = bufLen;
	}
	if (end < start)
	{
		end = start;
	}

	o = 0;
	for (i = start; i < end && o < outSize - 1; )
	{
		if (CG_BEChat_InputPaletteCodeAt(buf, i, bufLen))
		{
			i += 2;
			continue;
		}
		out[o++] = buf[i++];
	}
	out[o] = '\0';
	return o;
}

static float CG_BEChat_InputBufSliceDisplayWidth(const char* buf, int start, int end,
        float charWidth, float charHeight, int textFlags, int fontIndex)
{
	char tmp[MAX_STRING_CHARS];
	int dlen;

	dlen = CG_BEChat_InputCopyDisplay(buf, start, end, tmp, sizeof(tmp));
	if (dlen < 1)
	{
		return 0.0f;
	}
	return CG_OSPDrawStringNewSliceWidth(tmp, tmp + dlen, charWidth, charHeight, textFlags, fontIndex);
}

static int CG_BEChat_InputAdvanceBufPos(const char* buf, int pos, int bufLen)
{
	if (!buf || pos < 0 || pos >= bufLen)
	{
		return bufLen;
	}
	if (CG_BEChat_InputPaletteCodeAt(buf, pos, bufLen))
	{
		return pos + 2;
	}
	return pos + CG_BEChat_InputByteUnitLen(buf, pos, bufLen);
}

qboolean CG_BEChat_InputTryBackspacePalette(beChatInput_t* edit)
{
	int len;

	if (!edit || edit->cursor < 2)
	{
		return qfalse;
	}

	len = strlen(edit->buffer);
	if (!CG_BEChat_InputPaletteCodeAt(edit->buffer, edit->cursor - 2, len))
	{
		return qfalse;
	}

	edit->buffer[edit->cursor - 1] = '\0';
	edit->cursor--;
	return qtrue;
}

static char CG_BEChat_CvarColorChar(const char* cvarStr, char fallback)
{
	char c;

	if (!cvarStr || !cvarStr[0])
	{
		return fallback;
	}

	c = cvarStr[0];
	if (c >= 'A' && c <= 'Z')
	{
		c = c - 'A' + 'a';
	}

	if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))
	{
		return c;
	}

	return fallback;
}

static char CG_BEChat_LineDefaultColor(const beChatHistoryLine_t* line)
{
	if (!line)
	{
		return '7';
	}

	switch (line->msgKind)
	{
	case BE_CHAT_MSG_TEAM:
		return CG_BEChat_CvarColorChar(cg_teamChatColor.string, '5');
	case BE_CHAT_MSG_TELL:
		return '6';
	case BE_CHAT_MSG_SAY:
		return CG_BEChat_CvarColorChar(cg_chatColor.string, '2');
	default:
		return '7';
	}
}

static int CG_BEChat_NextWrapBytes(const char* text, int start, float maxWidth)
{
	int len;
	int byteIdx;
	int lastFit;
	int lastSpaceFit;
	int textFlags;
	int fit;

	if (!text || maxWidth <= 0.0f)
	{
		return 0;
	}

	len = strlen(text);
	if (start >= len)
	{
		return 0;
	}

	textFlags = CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS);
	fit = CG_OSPDrawStringNewFitBytes(text + start, text + len, maxWidth,
	                                  BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT);
	if (fit < 1)
	{
		return 1;
	}

	lastFit = fit;
	lastSpaceFit = 0;
	for (byteIdx = start; byteIdx < start + fit && byteIdx < len; byteIdx++)
	{
		if (text[byteIdx] == ' ')
		{
			lastSpaceFit = byteIdx - start + 1;
		}
	}

	if (lastSpaceFit > 0 && lastSpaceFit < lastFit && start + lastFit < len)
	{
		lastFit = lastSpaceFit;
	}

	if (lastFit > 0 && text[start + lastFit - 1] == Q_COLOR_ESCAPE)
	{
		if (start + lastFit < len && text[start + lastFit] != Q_COLOR_ESCAPE)
		{
			lastFit++;
		}
		else if (lastFit > 1)
		{
			lastFit--;
		}
	}

	return lastFit;
}

static qboolean CG_BEChat_CopyWrappedRow(const char* text, int rowIndex, float maxWidth, char defaultColor,
        char* out, int outSize)
{
	int start;
	int row;
	int n;

	if (!text || !out || outSize < 1 || rowIndex < 0)
	{
		return qfalse;
	}

	start = 0;
	row = 0;
	while (text[start])
	{
		n = CG_BEChat_NextWrapBytes(text, start, maxWidth);
		if (n < 1)
		{
			return qfalse;
		}
		if (row == rowIndex)
		{
			char active;
			int prefix;
			int copyLen;

			copyLen = n;
			prefix = 0;
			if (rowIndex > 0)
			{
				active = CG_BEChat_ActiveColorAt(text, start, defaultColor);
				if (outSize < 3)
				{
					return qfalse;
				}
				out[0] = Q_COLOR_ESCAPE;
				out[1] = active;
				prefix = 2;
			}

			if (copyLen + prefix >= outSize)
			{
				copyLen = outSize - 1 - prefix;
			}
			if (copyLen < 0)
			{
				copyLen = 0;
			}
			memcpy(out + prefix, text + start, copyLen);
			out[prefix + copyLen] = '\0';
			return qtrue;
		}
		row++;
		start += n;
		while (text[start] == ' ')
		{
			start++;
		}
	}

	return qfalse;
}

static void CG_BEChat_PushFlatRow(const char* text, int start, int nbytes, int rowInMsg,
                                  char defaultColor, int linesBack, int* flatIdx)
{
	char* dst;
	int prefix;
	int copyLen;

	if (!flatIdx || *flatIdx >= BE_CHAT_FLAT_ROWS_MAX)
	{
		beChatFlatFull = qtrue;
		return;
	}

	dst = beChatFlatText[*flatIdx];
	prefix = 0;
	if (rowInMsg > 0)
	{
		dst[0] = Q_COLOR_ESCAPE;
		dst[1] = CG_BEChat_ActiveColorAt(text, start, defaultColor);
		prefix = 2;
	}

	copyLen = nbytes;
	if (copyLen + prefix >= BE_CHAT_LINE_CHARS)
	{
		copyLen = BE_CHAT_LINE_CHARS - 1 - prefix;
	}
	if (copyLen < 0)
	{
		copyLen = 0;
	}
	if (copyLen > 0)
	{
		memcpy(dst + prefix, text + start, copyLen);
	}
	dst[prefix + copyLen] = '\0';
	beChatFlatLinesBack[*flatIdx] = linesBack;
	(*flatIdx)++;
}

static void CG_BEChat_RebuildRowsCacheAt(float maxWidth, qboolean timeFilter)
{
	beChatHistory_t* history;
	int linesBack;
	int total;
	int flatIdx;
	char msgBuf[BE_CHAT_LINE_CHARS];

	history = &cgs.beChatHistory;
	total = 0;
	flatIdx = 0;
	beChatFlatFull = qfalse;
	beChatFlatCount = 0;

	/* Oldest message first so flat[0] is top of history and flat[n-1] is newest (above input). */
	for (linesBack = history->count - 1; linesBack >= 0; linesBack--)
	{
		beChatHistoryLine_t* entry;
		char defaultColor;
		int start;
		int rowInMsg;
		int n;
		int rows;

		rows = 0;
		if (!CG_BEChat_MessagePassesTimeFilter(linesBack, timeFilter))
		{
			beChatRowsCacheRows[linesBack] = 0;
			continue;
		}

		CG_BEChat_MessageDisplayText(linesBack, msgBuf, sizeof(msgBuf));
		entry = CG_BEChat_HistoryEntry(linesBack);
		defaultColor = CG_BEChat_LineDefaultColor(entry);
		start = 0;
		rowInMsg = 0;
		while (msgBuf[start])
		{
			n = CG_BEChat_NextWrapBytes(msgBuf, start, maxWidth);
			if (n < 1)
			{
				break;
			}
			CG_BEChat_PushFlatRow(msgBuf, start, n, rowInMsg, defaultColor, linesBack, &flatIdx);
			rows++;
			rowInMsg++;
			start += n;
			while (msgBuf[start] == ' ')
			{
				start++;
			}
		}

		beChatRowsCacheRows[linesBack] = rows;
		total += rows;
	}

	beChatFlatCount = flatIdx;
	beChatRowsCacheWriteIdx = history->writeIndex;
	beChatRowsCacheCount = history->count;
	beChatRowsCacheWidth = maxWidth;
	beChatRowsCacheTimeFilter = timeFilter;
	beChatRowsCacheFrameTime = cg.time;
	beChatRowsCacheTotal = total;
}

static void CG_BEChat_EnsureRowsCache(float maxWidth, qboolean timeFilter)
{
	beChatHistory_t* history;

	history = &cgs.beChatHistory;
	if (beChatRowsCacheWriteIdx == history->writeIndex &&
	        beChatRowsCacheCount == history->count &&
	        beChatRowsCacheWidth == maxWidth &&
	        beChatRowsCacheTimeFilter == timeFilter &&
	        (!timeFilter || beChatRowsCacheFrameTime == cg.time))
	{
		return;
	}

	CG_BEChat_RebuildRowsCacheAt(maxWidth, timeFilter);
}

static qboolean CG_BEChat_GetDisplayRowFromBottom(int rowFromBottom, float maxWidth, qboolean timeFilter,
        char* out, int outSize, int* outLinesBack)
{
	beChatHistory_t* history;
	int linesBack;
	int remaining;
	char msgBuf[BE_CHAT_LINE_CHARS];
	int rows;
	int rowIdx;

	if (!out || outSize < 1 || rowFromBottom < 0)
	{
		return qfalse;
	}

	if (outLinesBack)
	{
		*outLinesBack = -1;
	}

	history = &cgs.beChatHistory;
	remaining = rowFromBottom;

	CG_BEChat_EnsureRowsCache(maxWidth, timeFilter);

	if (!beChatFlatFull)
	{
		int flatIdx;

		flatIdx = beChatFlatCount - 1 - rowFromBottom;
		if (flatIdx >= 0 && flatIdx < beChatFlatCount)
		{
			Q_strncpyz(out, beChatFlatText[flatIdx], outSize);
			if (outLinesBack)
			{
				*outLinesBack = beChatFlatLinesBack[flatIdx];
			}
			return qtrue;
		}
	}

	for (linesBack = 0; linesBack < history->count; linesBack++)
	{
		if (!CG_BEChat_MessagePassesTimeFilter(linesBack, timeFilter))
		{
			continue;
		}

		rows = beChatRowsCacheRows[linesBack];
		if (rows < 1)
		{
			continue;
		}

		if (remaining < rows)
		{
			beChatHistoryLine_t* entry;
			char defaultColor;

			rowIdx = rows - 1 - remaining;
			if (outLinesBack)
			{
				*outLinesBack = linesBack;
			}
			entry = CG_BEChat_HistoryEntry(linesBack);
			defaultColor = CG_BEChat_LineDefaultColor(entry);
			CG_BEChat_MessageDisplayText(linesBack, msgBuf, sizeof(msgBuf));
			return CG_BEChat_CopyWrappedRow(msgBuf, rowIdx, maxWidth, defaultColor, out, outSize);
		}

		remaining -= rows;
	}

	return qfalse;
}

static float CG_BEChat_HistoryTextWidth(int* outDisplayRows, qboolean timeFilter)
{
	float w;
	float wFull;
	float winW;
	int visible;
	int total;

	visible = CG_BEChat_VisibleHistoryLines();
	CG_BEChat_WindowLayout(NULL, NULL, &winW, NULL);
	wFull = winW - BECHAT_PAD * 2.0f;

	if (beChatWindow.open)
	{
		float wNarrow;

		wNarrow = wFull - BECHAT_SB_W - BECHAT_SB_GAP;
		if (wNarrow < BECHAT_CHAR_W * 2.0f)
		{
			wNarrow = BECHAT_CHAR_W * 2.0f;
		}
		CG_BEChat_EnsureRowsCache(wNarrow, timeFilter);
		total = beChatRowsCacheTotal;
		if (total > visible)
		{
			w = wNarrow;
		}
		else
		{
			CG_BEChat_EnsureRowsCache(wFull, timeFilter);
			total = beChatRowsCacheTotal;
			w = wFull;
		}
	}
	else
	{
		CG_BEChat_EnsureRowsCache(wFull, timeFilter);
		total = beChatRowsCacheTotal;
		w = wFull;
	}

	if (outDisplayRows)
	{
		*outDisplayRows = total;
	}

	return w;
}

static int CG_BEChat_MaxHistoryScroll(float maxWidth, qboolean timeFilter)
{
	int visible;
	int total;
	int maxScroll;

	visible = CG_BEChat_VisibleHistoryLines();
	CG_BEChat_EnsureRowsCache(maxWidth, timeFilter);
	total = beChatRowsCacheTotal;
	maxScroll = total - visible;
	if (maxScroll < 0)
	{
		maxScroll = 0;
	}
	return maxScroll;
}

static void CG_BEChat_DrawHistoryLines(float x, float y, float w, int historyScroll, float historyTextW,
                                       qboolean timeFilter)
{
	vec4_t textColor;
	int visible;
	int i;
	int rowFromBottom;
	float lineY;
	int linesBack;
	char lineBuf[BE_CHAT_LINE_CHARS];

	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	visible = CG_BEChat_VisibleHistoryLines();
	for (i = 0; i < visible; i++)
	{
		rowFromBottom = historyScroll + (visible - 1 - i);
		lineY = y + BECHAT_PAD + (float)i * BECHAT_CHAR_H;
		if (CG_BEChat_GetDisplayRowFromBottom(rowFromBottom, historyTextW, timeFilter,
		                                      lineBuf, sizeof(lineBuf), &linesBack))
		{
			textColor[3] = 1.0f;
			if (timeFilter)
			{
				textColor[3] = CG_BEChat_MessageTimeAlpha(linesBack);
				if (textColor[3] <= 0.0f)
				{
					continue;
				}
			}
			CG_OSPDrawStringNew(x + BECHAT_PAD, lineY, lineBuf, textColor, colorBlack,
			                    BECHAT_CHAR_W, BECHAT_CHAR_H,
			                    0, CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS), BE_CHAT_FONT,
			                    NULL, NULL, NULL);
		}
	}
}

int CG_BEChat_InputByteUnitLen(const char* text, int pos, int textLen)
{
	const emoji_t* em;

	if (!text || pos < 0 || pos >= textLen)
	{
		return 1;
	}

	if (text[pos] == EMOJI_CHAR)
	{
		em = CG_OSPGetEmoji(text + pos);
		if (em && pos + em->len <= textLen)
		{
			return em->len;
		}
	}

	if (Q_IsColorString(text + pos))
	{
		if (text[pos + 1] == 'X' || text[pos + 1] == 'x')
		{
			if (pos + 8 <= textLen)
			{
				return 8;
			}
			return textLen - pos;
		}
		if (pos + 2 <= textLen)
		{
			return 2;
		}
		return 1;
	}

	return 1;
}

void CG_BEChat_InputSnapScrollPos(beChatInput_t* edit)
{
	int len;

	if (!edit)
	{
		return;
	}

	len = strlen(edit->buffer);
	while (edit->scroll > 0 && edit->scroll < len &&
	        Q_IsColorString(edit->buffer + edit->scroll - 1))
	{
		edit->scroll--;
	}
}

static int CG_BEChat_InputVisibleBytes(const char* text, int start, float maxWidth)
{
	int len;
	int textFlags;
	int pos;
	int lastFit;
	float w;

	if (!text || maxWidth <= 0.0f)
	{
		return 0;
	}

	len = strlen(text);
	if (start < 0)
	{
		start = 0;
	}
	if (start >= len)
	{
		return 0;
	}

	textFlags = CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS);
	lastFit = start;
	pos = start;
	while (pos < len)
	{
		int next;

		next = CG_BEChat_InputAdvanceBufPos(text, pos, len);
		w = CG_BEChat_InputBufSliceDisplayWidth(text, start, next,
		        BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT);
		if (w > maxWidth)
		{
			break;
		}
		lastFit = next;
		pos = next;
	}

	return lastFit - start;
}

void CG_BEChat_InputClampScroll(beChatInput_t* edit, float maxTextW)
{
	int len;
	int textFlags;

	len = strlen(edit->buffer);
	textFlags = CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS);
	if (edit->scroll < 0)
	{
		edit->scroll = 0;
	}
	if (edit->scroll > len)
	{
		edit->scroll = len;
	}
	if (edit->cursor < 0)
	{
		edit->cursor = 0;
	}
	if (edit->cursor > len)
	{
		edit->cursor = len;
	}

	CG_BEChat_InputSnapScrollPos(edit);

	while (edit->scroll < edit->cursor)
	{
		int step;

		if (CG_BEChat_InputBufSliceDisplayWidth(edit->buffer, edit->scroll, edit->cursor,
		                                        BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT) <= maxTextW)
		{
			break;
		}
		step = CG_BEChat_InputByteUnitLen(edit->buffer, edit->scroll, len);
		edit->scroll += step;
		CG_BEChat_InputSnapScrollPos(edit);
	}

	while (edit->scroll > 0)
	{
		int step;

		if (CG_BEChat_InputBufSliceDisplayWidth(edit->buffer, edit->scroll - 1, edit->cursor,
		                                        BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT) > maxTextW)
		{
			break;
		}
		step = CG_BEChat_InputByteUnitLen(edit->buffer, edit->scroll - 1, len);
		if (step > edit->scroll)
		{
			edit->scroll = 0;
		}
		else
		{
			edit->scroll -= step;
		}
		CG_BEChat_InputSnapScrollPos(edit);
	}
}

static void CG_BEChat_Scroll(int delta)
{
	int maxScroll;
	float historyTextW;

	historyTextW = CG_BEChat_HistoryTextWidth(NULL, qfalse);
	maxScroll = CG_BEChat_MaxHistoryScroll(historyTextW, qfalse);

	if (delta != 0)
	{
		beChatWindow.historyScroll += delta;
	}
	if (beChatWindow.historyScroll < 0)
	{
		beChatWindow.historyScroll = 0;
	}
	if (beChatWindow.historyScroll > maxScroll)
	{
		beChatWindow.historyScroll = maxScroll;
	}
}

static void CG_BEChat_ScrollbarGetThumb(float sbH, float contentH, float scroll,
        float* outThumbH, float* outThumbY, float* outMaxScroll)
{
	float maxScroll;
	float thumbH;
	float thumbY;
	float trackRange;

	maxScroll = contentH - sbH;
	if (maxScroll < 0.0f)
	{
		maxScroll = 0.0f;
	}

	thumbH = sbH * (sbH / contentH);
	if (thumbH < BECHAT_CHAR_H)
	{
		thumbH = BECHAT_CHAR_H;
	}
	if (thumbH > sbH)
	{
		thumbH = sbH;
	}

	trackRange = sbH - thumbH;
	thumbY = 0.0f;
	if (maxScroll > 0.0f && trackRange > 0.0f)
	{
		thumbY = (scroll / maxScroll) * trackRange;
	}

	if (thumbY < 0.0f)
	{
		thumbY = 0.0f;
	}
	if (thumbY + thumbH > sbH)
	{
		thumbY = sbH - thumbH;
		if (thumbY < 0.0f)
		{
			thumbY = 0.0f;
		}
	}

	*outThumbH = thumbH;
	*outThumbY = thumbY;
	*outMaxScroll = maxScroll;
}

static void CG_BEChat_ScrollbarLayoutUpdate(void)
{
	int visible;
	int lineCount;
	float contentH;
	float scrollPx;
	float historyTextW;

	visible = CG_BEChat_VisibleHistoryLines();
	historyTextW = CG_BEChat_HistoryTextWidth(&lineCount, qfalse);

	if (lineCount <= visible)
	{
		beChatWindow.sbActive = qfalse;
		return;
	}

	beChatWindow.sbActive = qtrue;
	{
		float winX;
		float winY;
		float winW;

		CG_BEChat_WindowLayout(&winX, &winY, &winW, NULL);
		beChatWindow.sbTrackX = winX + winW - BECHAT_PAD - BECHAT_SB_W;
		beChatWindow.sbTrackY = winY + BECHAT_PAD;
	}
	beChatWindow.sbTrackW = BECHAT_SB_W;
	beChatWindow.sbTrackH = (float)visible * BECHAT_CHAR_H;

	beChatWindow.sbMaxScrollLines = lineCount - visible;
	if (beChatWindow.sbMaxScrollLines < 0)
	{
		beChatWindow.sbMaxScrollLines = 0;
	}

	if (beChatWindow.historyScroll < 0)
	{
		beChatWindow.historyScroll = 0;
	}
	if (beChatWindow.historyScroll > beChatWindow.sbMaxScrollLines)
	{
		beChatWindow.historyScroll = beChatWindow.sbMaxScrollLines;
	}

	contentH = (float)lineCount * BECHAT_CHAR_H;
	scrollPx = (float)(beChatWindow.sbMaxScrollLines - beChatWindow.historyScroll) * BECHAT_CHAR_H;
	CG_BEChat_ScrollbarGetThumb(beChatWindow.sbTrackH, contentH, scrollPx,
	                            &beChatWindow.sbThumbH, &beChatWindow.sbThumbY, &beChatWindow.sbMaxScrollPx);
	(void)historyTextW;
}

static void CG_BEChat_ScrollbarDragUpdate(void)
{
	float trackRange;
	float thumbY;
	float scrollPx;
	int historyScroll;
	float my;

	if (!beChatWindow.sbDragging || !beChatWindow.sbActive)
	{
		return;
	}

	trackRange = beChatWindow.sbTrackH - beChatWindow.sbThumbH;
	if (trackRange < 1.0f || beChatWindow.sbMaxScrollPx < 1.0f)
	{
		return;
	}

	CG_BEChat_VirtualFromCursor(NULL, &my);

	thumbY = my - beChatWindow.sbGrabDeltaY - beChatWindow.sbTrackY;
	if (thumbY < 0.0f)
	{
		thumbY = 0.0f;
	}
	if (thumbY > trackRange)
	{
		thumbY = trackRange;
	}

	scrollPx = (thumbY / trackRange) * beChatWindow.sbMaxScrollPx;
	historyScroll = beChatWindow.sbMaxScrollLines - (int)(scrollPx / BECHAT_CHAR_H + 0.5f);
	beChatWindow.historyScroll = historyScroll;
	if (beChatWindow.historyScroll < 0)
	{
		beChatWindow.historyScroll = 0;
	}
	if (beChatWindow.historyScroll > beChatWindow.sbMaxScrollLines)
	{
		beChatWindow.historyScroll = beChatWindow.sbMaxScrollLines;
	}
}


float CG_BEChat_HelpBtnReserveWidth(void)
{
	float size;

	size = BECHAT_EMOJI_BTN_SIZE;
	if (size < BECHAT_CHAR_H)
	{
		size = BECHAT_HELP_BTN_SIZE;
	}
	return BECHAT_EMOJI_BTN_HIT_PAD + size;
}

static void CG_BEChat_HelpLayout(float winX, float winY, float winW, float winH, float* outX, float* outY, float* outSize)
{
	float size;
	float emojiBtnX;
	float emojiBtnY;
	float emojiBtnW;
	float emojiBtnH;
	float inputY;

	emojiBtnW = BECHAT_EMOJI_BTN_SIZE;
	emojiBtnH = BECHAT_EMOJI_BTN_SIZE;
	inputY = CG_BEChat_InputRowY(winY, winH);
	emojiBtnX = winX + winW - BECHAT_EMOJI_WIN_PAD - emojiBtnW;
	if (cg_chatWindow.integer & BE_CHAT_INPUT_ONLY_BG_BIT)
	{
		emojiBtnY = CG_BEChat_InputRowBgY(winY, winH) +
		            (BECHAT_INPUT_ROW_H - emojiBtnH) * 0.5f;
	}
	else
	{
		emojiBtnY = inputY + BECHAT_CHAR_H - 1.0f - emojiBtnH;
	}

	size = emojiBtnH;
	if (size < BECHAT_CHAR_H)
	{
		size = BECHAT_HELP_BTN_SIZE;
	}

	if (outSize)
	{
		*outSize = size;
	}

	if (outX)
	{
		*outX = emojiBtnX - CG_BEChat_HelpBtnReserveWidth();
	}
	if (outY)
	{
		*outY = emojiBtnY + (emojiBtnH - size) * 0.5f;
	}
}

static qboolean CG_BEChat_HelpPointInButton(float virtX, float virtY, float btnX, float btnY, float btnSize)
{
	float pad;

	pad = BECHAT_EMOJI_BTN_HIT_PAD;
	if (virtX < btnX - pad || virtX > btnX + btnSize + pad)
	{
		return qfalse;
	}
	if (virtY < btnY - pad || virtY > btnY + btnSize + pad)
	{
		return qfalse;
	}
	return qtrue;
}

static float CG_BEChat_HelpTipAlpha(qboolean* outShow)
{
	int elapsed;
	float alpha;

	if (outShow)
	{
		*outShow = qfalse;
	}

	if (beChatWindow.helpTipFadingOut)
	{
		elapsed = cg.time - beChatWindow.helpTipHideStartTime;
		if (elapsed < 0)
		{
			elapsed = 0;
		}
		if (elapsed >= BE_CHAT_HELP_TIP_FADE_MS)
		{
			beChatWindow.helpTipFadingOut = qfalse;
			return 0.0f;
		}
		alpha = 1.0f - ((float)elapsed / (float)BE_CHAT_HELP_TIP_FADE_MS);
		if (outShow)
		{
			*outShow = qtrue;
		}
		return alpha;
	}

	if (!beChatWindow.helpHover)
	{
		return 0.0f;
	}

	elapsed = cg.time - beChatWindow.helpHoverStartTime;
	if (elapsed < BE_CHAT_HELP_TIP_DELAY_MS)
	{
		return 0.0f;
	}

	elapsed -= BE_CHAT_HELP_TIP_DELAY_MS;
	if (elapsed >= BE_CHAT_HELP_TIP_FADE_MS)
	{
		alpha = 1.0f;
	}
	else
	{
		alpha = (float)elapsed / (float)BE_CHAT_HELP_TIP_FADE_MS;
	}

	if (outShow)
	{
		*outShow = qtrue;
	}
	return alpha;
}

static void CG_BEChat_HelpHoverFrame(void)
{
	float virtX;
	float virtY;
	float btnX;
	float btnY;
	float btnSize;
	float winX;
	float winY;
	float winW;
	float winH;
	qboolean hoverNow;
	qboolean showTip;
	float alpha;

	if (!beChatWindow.open)
	{
		beChatWindow.helpHover = qfalse;
		beChatWindow.helpTipFadingOut = qfalse;
		return;
	}

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_HelpLayout(winX, winY, winW, winH, &btnX, &btnY, &btnSize);
	CG_BEChat_VirtualCursorPos(&virtX, &virtY);
	hoverNow = CG_BEChat_HelpPointInButton(virtX, virtY, btnX, btnY, btnSize);

	if (hoverNow != beChatWindow.helpHover)
	{
		if (beChatWindow.helpHover)
		{
			alpha = CG_BEChat_HelpTipAlpha(&showTip);
			if (alpha > 0.0f && showTip)
			{
				beChatWindow.helpTipHideStartTime = cg.time;
				beChatWindow.helpTipFadingOut = qtrue;
			}
			else
			{
				beChatWindow.helpTipFadingOut = qfalse;
			}
		}
		else
		{
			beChatWindow.helpTipFadingOut = qfalse;
		}

		beChatWindow.helpHover = hoverNow;
		if (hoverNow)
		{
			beChatWindow.helpHoverStartTime = cg.time;
		}
	}
}

static qboolean CG_BEChat_HelpMouseDown(float virtX, float virtY)
{
	float btnX;
	float btnY;
	float btnSize;
	float winX;
	float winY;
	float winW;
	float winH;

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_HelpLayout(winX, winY, winW, winH, &btnX, &btnY, &btnSize);
	return CG_BEChat_HelpPointInButton(virtX, virtY, btnX, btnY, btnSize);
}

static void CG_BEChat_HelpDrawTooltip(float btnX, float btnY, float btnSize, float winX, float winY, float alpha)
{
	int i;
	float tipW;
	float tipH;
	float lineW;
	float tipX;
	float tipY;
	float lineStep;
	float lineY;
	vec4_t tipBg;
	vec4_t lineColor;
	const char* line;

	if (alpha <= 0.0f)
	{
		return;
	}

	tipW = 0.0f;
	lineStep = BECHAT_CHAR_H + 1.0f;
	for (i = 0; i < BE_CHAT_HELP_TIP_LINE_COUNT; i++)
	{
		lineW = CG_OSPDrawStringNewTextWidth(beChatHelpTipLines[i],
		                                     BECHAT_CHAR_W, BECHAT_CHAR_H,
		                                     BE_CHAT_HELP_TIP_FLAGS, BE_CHAT_FONT);
		if (lineW > tipW)
		{
			tipW = lineW;
		}
	}
	if (tipW < BECHAT_CHAR_W)
	{
		tipW = BECHAT_CHAR_W;
	}

	tipH = lineStep * (float)(BE_CHAT_HELP_TIP_LINE_COUNT - 1) + BECHAT_CHAR_H + 4.0f;
	tipX = btnX + btnSize - tipW;
	if (tipX < winX + BECHAT_PAD)
	{
		tipX = winX + BECHAT_PAD;
	}
	tipY = btnY - BECHAT_EMOJI_TIP_PAD * 2.0f - tipH;
	if (tipY < winY + BECHAT_PAD)
	{
		tipY = winY + BECHAT_PAD;
	}

	tipBg[0] = 0.0f;
	tipBg[1] = 0.0f;
	tipBg[2] = 0.0f;
	tipBg[3] = BE_CHAT_HELP_TIP_BG_ALPHA * alpha;
	CG_FillRect(tipX - BECHAT_EMOJI_TIP_PAD, tipY - BECHAT_EMOJI_TIP_PAD,
	            tipW + BECHAT_EMOJI_TIP_PAD * 2.0f,
	            tipH + BECHAT_EMOJI_TIP_PAD * 2.0f, tipBg);

	lineY = tipY;
	for (i = 0; i < BE_CHAT_HELP_TIP_LINE_COUNT; i++)
	{
		line = beChatHelpTipLines[i];
		lineColor[0] = 1.0f;
		lineColor[1] = 1.0f;
		lineColor[2] = 1.0f;
		lineColor[3] = alpha;
		CG_OSPDrawStringNew(tipX, lineY, line, lineColor, colorBlack,
		                    BECHAT_CHAR_W, BECHAT_CHAR_H,
		                    (int)tipW, BE_CHAT_HELP_TIP_FLAGS, BE_CHAT_FONT,
		                    NULL, NULL, NULL);
		lineY += lineStep;
		if (i == 2)
		{
			lineY += 2.0f;
		}
		else if (i == 4)
		{
			lineY += 2.0f;
		}
	}
}

static qboolean CG_BEChat_ScrollbarMouseDown(void)
{
	float mx;
	float my;
	float thumbTop;

	CG_BEChat_ScrollbarLayoutUpdate();
	if (!beChatWindow.sbActive)
	{
		return qfalse;
	}

	CG_BEChat_VirtualFromCursor(&mx, &my);

	thumbTop = beChatWindow.sbTrackY + beChatWindow.sbThumbY;

	if (mx < beChatWindow.sbTrackX || mx > beChatWindow.sbTrackX + beChatWindow.sbTrackW)
	{
		return qfalse;
	}

	if (my < beChatWindow.sbTrackY || my > beChatWindow.sbTrackY + beChatWindow.sbTrackH)
	{
		return qfalse;
	}

	if (my >= thumbTop - BECHAT_SB_HIT_PAD && my <= thumbTop + beChatWindow.sbThumbH + BECHAT_SB_HIT_PAD)
	{
		beChatWindow.sbDragging = qtrue;
		beChatWindow.sbGrabDeltaY = my - thumbTop;
		return qtrue;
	}

	if (my < thumbTop)
	{
		CG_BEChat_Scroll(1);
		return qtrue;
	}

	CG_BEChat_Scroll(-1);
	return qtrue;
}

#if BE_CHAT_MOUSE_DEBUG
static void CG_BEChat_DebugMousePress(const char* via)
{
	float mx;
	float my;
	float thumbTop;
	qboolean onTrack;
	qboolean onThumb;

	CG_BEChat_ScrollbarLayoutUpdate();
	CG_BEChat_VirtualFromCursor(&mx, &my);
	thumbTop = beChatWindow.sbTrackY + beChatWindow.sbThumbY;
	onTrack = qfalse;
	onThumb = qfalse;

	if (beChatWindow.sbActive &&
	        mx >= beChatWindow.sbTrackX &&
	        mx <= beChatWindow.sbTrackX + beChatWindow.sbTrackW &&
	        my >= beChatWindow.sbTrackY &&
	        my <= beChatWindow.sbTrackY + beChatWindow.sbTrackH)
	{
		onTrack = qtrue;
		if (my >= thumbTop - BECHAT_SB_HIT_PAD &&
		        my <= thumbTop + beChatWindow.sbThumbH + BECHAT_SB_HIT_PAD)
		{
			onThumb = qtrue;
		}
	}

	CG_Printf(
	    "BEChat mouse [%s]: pixel %.1f,%.1f virt %.1f,%.1f | IsDown(M1)=%i | sbActive=%i onTrack=%i onThumb=%i | "
	    "track %.1f,%.1f %.1fx%.1f thumbY=%.1f h=%.1f drag=%i\n",
	    via,
	    beChatWindow.cursorX, beChatWindow.cursorY, mx, my,
	    trap_Key_IsDown(K_MOUSE1),
	    beChatWindow.sbActive, onTrack, onThumb,
	    beChatWindow.sbTrackX, beChatWindow.sbTrackY,
	    beChatWindow.sbTrackW, beChatWindow.sbTrackH,
	    beChatWindow.sbThumbY, beChatWindow.sbThumbH,
	    beChatWindow.sbDragging);
}
#endif

static void CG_BEChat_HandleMouse1Press(void)
{
	float virtX;
	float virtY;
	float maxTextW;
	float winX;
	float winY;
	float winW;
	float winH;

	if (cg.time < beChatWindow.inputGraceUntil)
	{
		return;
	}

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_EmojiPrepareInputRow(winX, winY, winW, winH);

	CG_BEChat_VirtualFromCursor(&virtX, &virtY);
	maxTextW = CG_BEChat_InputFieldMaxWidth();
	if (CG_BEChat_HelpMouseDown(virtX, virtY))
	{
		return;
	}
	if (!CG_BEChat_EmojiMouseDown(virtX, virtY, &beChatWindow.input, maxTextW))
	{
		CG_BEChat_ScrollbarMouseDown();
	}
}

static void CG_BEChat_ScrollbarDragPoll(void)
{
	if (!beChatWindow.sbDragging)
	{
		return;
	}

	if (trap_Key_IsDown(K_MOUSE1))
	{
		CG_BEChat_ScrollbarDragUpdate();
	}
	else
	{
		beChatWindow.sbDragging = qfalse;
	}
}

static qboolean CG_BEChat_IsWordBoundaryChar(char c)
{
	if (c == ' ' || c == '\t')
	{
		return qtrue;
	}

	return qfalse;
}

static void CG_BEChat_CursorWordLeft(beChatInput_t* edit)
{
	while (edit->cursor > 0)
	{
		if (edit->cursor >= 2 && Q_IsColorString(edit->buffer + edit->cursor - 2))
		{
			edit->cursor -= 2;
			continue;
		}
		if (Q_IsColorString(edit->buffer + edit->cursor))
		{
			edit->cursor += 2;
			continue;
		}
		if (!CG_BEChat_IsWordBoundaryChar(edit->buffer[edit->cursor - 1]))
		{
			edit->cursor--;
		}
		else
		{
			break;
		}
	}

	while (edit->cursor > 0)
	{
		if (edit->cursor >= 2 && Q_IsColorString(edit->buffer + edit->cursor - 2))
		{
			edit->cursor -= 2;
			continue;
		}
		if (CG_BEChat_IsWordBoundaryChar(edit->buffer[edit->cursor - 1]))
		{
			edit->cursor--;
		}
		else
		{
			break;
		}
	}
}

static void CG_BEChat_CursorWordRight(beChatInput_t* edit)
{
	int len;

	len = strlen(edit->buffer);

	while (edit->cursor < len)
	{
		if (Q_IsColorString(edit->buffer + edit->cursor))
		{
			edit->cursor += 2;
			continue;
		}
		if (CG_BEChat_IsWordBoundaryChar(edit->buffer[edit->cursor]))
		{
			edit->cursor++;
		}
		else
		{
			break;
		}
	}

	while (edit->cursor < len)
	{
		if (Q_IsColorString(edit->buffer + edit->cursor))
		{
			edit->cursor += 2;
			continue;
		}
		if (!CG_BEChat_IsWordBoundaryChar(edit->buffer[edit->cursor]))
		{
			edit->cursor++;
		}
		else
		{
			break;
		}
	}
}

static void CG_BEChat_InputKeyDown(beChatInput_t* edit, int key, float maxTextW)
{
	int len;

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
	{
		return;
	}

	len = strlen(edit->buffer);

	if (trap_Key_IsDown(K_CTRL))
	{
		if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
		{
			CG_BEChat_CursorWordLeft(edit);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			CG_BEChat_CmdAcRefresh(edit);
			return;
		}
		if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
		{
			CG_BEChat_CursorWordRight(edit);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			CG_BEChat_CmdAcRefresh(edit);
			return;
		}
	}

	if (key == K_DEL || key == K_KP_DEL)
	{
		CG_BEChat_EmojiDelete(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
	{
		CG_BEChat_EmojiCursorStepRight(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
	{
		CG_BEChat_EmojiCursorStepLeft(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if (key == K_HOME || key == K_KP_HOME)
	{
		edit->cursor = 0;
		edit->scroll = 0;
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if (key == K_END || key == K_KP_END)
	{
		edit->cursor = len;
		CG_BEChat_InputClampScroll(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if ((key == 'a' || key == 'A') && trap_Key_IsDown(K_CTRL))
	{
		edit->cursor = 0;
		edit->scroll = 0;
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if ((key == 'e' || key == 'E') && trap_Key_IsDown(K_CTRL))
	{
		edit->cursor = len;
		CG_BEChat_InputClampScroll(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}

	if (key == K_BACKSPACE)
	{
		CG_BEChat_EmojiBackspace(edit, maxTextW);
		CG_BEChat_CmdAcRefresh(edit);
		return;
	}
}

static void CG_BEChat_InputChar(beChatInput_t* edit, int ch, float maxTextW)
{
	int len;

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
	{
		return;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK ||
	        beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK)
	{
		if (ch < '0' || ch > '9')
		{
			return;
		}
	}

	if (ch < ' ')
	{
		return;
	}

	len = strlen(edit->buffer);
	if (len >= MAX_SAY_TEXT - 1)
	{
		return;
	}

	beChatWindow.sentHistBrowse = -1;

	memmove(edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
	edit->buffer[edit->cursor] = (char)ch;
	edit->cursor++;
	CG_BEChat_EmojiOnInputChanged(edit);
	CG_BEChat_CmdAcRefresh(edit);
	if (ch == ' ')
	{
		if (!CG_BEChat_TryCommitTellTarget(edit))
		{
			CG_BEChat_TryCommitReply(edit);
		}
		CG_BEChat_TryEnterMutePickFromBuffer(edit);
		CG_BEChat_TryEnterUnmutePickFromBuffer(edit);
	}
	CG_BEChat_InputClampScroll(edit, maxTextW);
}

static void CG_BEChat_SendReplyText(const char* message)
{
	char cmd[MAX_STRING_CHARS];
	char wire[MAX_STRING_CHARS];
	char* p;

	if (!message)
	{
		return;
	}

	Q_strncpyz(cmd, message, sizeof(cmd));
	p = cmd;
	while (*p)
	{
		if (*p == '\n' || *p == '\r')
		{
			*p = ' ';
		}
		p++;
	}

	CG_BEChat_EncodePercentWire(cmd, wire, sizeof(wire));
	if (!CG_SendReplyMessage(wire))
	{
		return;
	}

	CG_BEChat_SentHistPush(cmd, BE_CHAT_DEST_REPLY, -1, NULL);
	CG_BEChat_InputClear(&beChatWindow.input);
	beChatWindow.historyScroll = 0;
	if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
	{
		CG_BEChat_Close();
	}
}

static void CG_BEChat_SendInput(void)
{
	char* p;
	char cmd[MAX_STRING_CHARS];
	char wire[MAX_STRING_CHARS];
	const char* buf;
	int clientNum;
	int i;
	char name[MAX_NAME_LENGTH];
	const char* msg;

	buf = beChatWindow.input.buffer;

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
	{
		if (beChatWindow.tellClientNum >= 0)
		{
			CG_ChatfilterMuteClient(beChatWindow.tellClientNum);
		}
		CG_BEChat_InputClear(&beChatWindow.input);
		CG_BEChat_ResetDestMode();
		beChatWindow.historyScroll = 0;
		if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
		{
			CG_BEChat_Close();
		}
		return;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK)
	{
		if (CG_BEChat_TryCommitMutePick(&beChatWindow.input))
		{
			return;
		}
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK)
	{
		if (CG_BEChat_TryCommitUnmutePick(&beChatWindow.input))
		{
			return;
		}
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_PICK ||
	        beChatWindow.destMode == BE_CHAT_DEST_UNMUTE_PICK)
	{
		if (!buf[0])
		{
			CG_BEChat_Close();
		}
		return;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_TELL)
	{
		if (!buf[0])
		{
			CG_BEChat_Close();
			return;
		}
		if (beChatWindow.tellClientNum < 0)
		{
			return;
		}
		CG_BEChat_EncodePercentWire(buf, wire, sizeof(wire));
		trap_SendClientCommand(va("tell %i %s", beChatWindow.tellClientNum, wire));
		CG_BEChat_SentHistPush(buf, beChatWindow.destMode,
		                       beChatWindow.tellClientNum, beChatWindow.tellName);
		CG_BEChat_InputClear(&beChatWindow.input);
		beChatWindow.historyScroll = 0;
		if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
		{
			CG_BEChat_Close();
		}
		return;
	}

	if (beChatWindow.destMode == BE_CHAT_DEST_REPLY)
	{
		if (!buf[0])
		{
			CG_BEChat_Close();
			return;
		}
		CG_BEChat_SendReplyText(buf);
		return;
	}

	if (!buf[0])
	{
		CG_BEChat_Close();
		return;
	}

	if (CG_BEChat_TryCommitMuteIdLine(&beChatWindow.input))
	{
		return;
	}

	if (CG_BEChat_TryCommitUnmuteIdLine(&beChatWindow.input))
	{
		return;
	}

	if (CG_BEChat_IsReplyLinePrefix(buf, &i))
	{
		if (!buf[i])
		{
			CG_BEChat_TryCommitReply(&beChatWindow.input);
			return;
		}
		CG_BEChat_SendReplyText(buf + i);
		return;
	}

	if (!Q_stricmpn(buf, "/tell", 5) && buf[5] == ' ')
	{
		i = 6;
		while (buf[i] == ' ')
		{
			i++;
		}
		for (name[0] = '\0'; buf[i] && buf[i] != ' '; i++)
		{
			int n;

			n = strlen(name);
			if (n >= (int)sizeof(name) - 1)
			{
				break;
			}
			name[n] = buf[i];
			name[n + 1] = '\0';
		}
		if (!name[0])
		{
			return;
		}
		while (buf[i] == ' ')
		{
			i++;
		}
		msg = buf + i;
		if (!msg[0])
		{
			return;
		}
		clientNum = -1;
		if (CG_BEChat_TellClientFromIdToken(name, &clientNum))
		{
			/* clientNum set */
		}
		else
		{
			clientNum = CG_BEChat_ClientFromTellName(name);
		}
		if (clientNum < 0)
		{
			return;
		}
		CG_BEChat_EncodePercentWire(msg, wire, sizeof(wire));
		trap_SendClientCommand(va("tell %i %s", clientNum, wire));
		CG_BEChat_SentHistPush(msg, BE_CHAT_DEST_TELL, clientNum, name);
		CG_BEChat_InputClear(&beChatWindow.input);
		beChatWindow.historyScroll = 0;
		if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
		{
			CG_BEChat_Close();
		}
		return;
	}

	Q_strncpyz(cmd, buf, sizeof(cmd));
	p = cmd;
	while (*p)
	{
		if (*p == '\n' || *p == '\r')
		{
			*p = ' ';
		}
		p++;
	}

	CG_BEChat_EncodePercentWire(cmd, wire, sizeof(wire));
	if (beChatWindow.destMode == BE_CHAT_DEST_TEAM)
	{
		trap_SendClientCommand(va("say_team %s", wire));
	}
	else
	{
		trap_SendClientCommand(va("say %s", wire));
	}
	CG_BEChat_SentHistPush(cmd, beChatWindow.destMode,
	                       beChatWindow.tellClientNum, beChatWindow.tellName);
	CG_BEChat_InputClear(&beChatWindow.input);
	beChatWindow.historyScroll = 0;
	if (!(cg_chatWindow.integer & BE_CHAT_KEEP_OPEN_BIT))
	{
		CG_BEChat_Close();
	}
}

static void CG_BEChat_HandleScrollKey(int key)
{
	if (key == K_PGUP || key == K_KP_PGUP)
	{
		CG_BEChat_Scroll(1);
		return;
	}
	if (key == K_PGDN || key == K_KP_PGDN)
	{
		CG_BEChat_Scroll(-1);
		return;
	}
	if (trap_Key_IsDown(K_CTRL))
	{
		if (key == K_UPARROW || key == K_KP_UPARROW)
		{
			CG_BEChat_Scroll(1);
		}
		else if (key == K_DOWNARROW || key == K_KP_DOWNARROW)
		{
			CG_BEChat_Scroll(-1);
		}
	}
}

static qboolean CG_BEChat_KeyEdge(int key)
{
	qboolean down;
	qboolean edge;

	if (key < 0 || key >= 256)
	{
		return qfalse;
	}

	down = trap_Key_IsDown(key);
	edge = down && !beChatWindow.keyDown[key];
	beChatWindow.keyDown[key] = down;
	return edge;
}

static qboolean CG_BEChat_KeySupportsRepeat(int key)
{
	if (key == K_ESCAPE || key == K_ENTER || key == K_KP_ENTER)
	{
		return qfalse;
	}

	if (key == K_PGUP || key == K_PGDN)
	{
		return qtrue;
	}

	if (key == K_UPARROW || key == K_DOWNARROW || key == K_KP_UPARROW || key == K_KP_DOWNARROW)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			return qtrue;
		}
		if (key == K_KP_UPARROW || key == K_KP_DOWNARROW)
		{
			return qtrue;
		}
		return CG_BEChat_EmojiAcIsActive() || CG_BEChat_CmdAcIsActive() ||
		       CG_BEChat_SentHistCanNavigate();
	}

	if (key >= 32 && key < 127)
	{
		if (trap_Key_IsDown(K_CTRL))
		{
			if (key == 'a' || key == 'A' || key == 'e' || key == 'E' ||
			        key == 'l' || key == 'L' || key == 'v' || key == 'V')
			{
				return qfalse;
			}
		}
		return qtrue;
	}

	switch (key)
	{
	case K_BACKSPACE:
	case K_DEL:
	case K_KP_DEL:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_HOME:
	case K_KP_HOME:
	case K_END:
	case K_KP_END:
	case K_KP_5:
	case K_KP_INS:
	case K_KP_UPARROW:
	case K_KP_DOWNARROW:
	case K_KP_PGUP:
	case K_KP_PGDN:
		return qtrue;
	default:
		return qfalse;
	}
}

static void CG_BEChat_BeginRepeat(int key)
{
	if (!CG_BEChat_KeySupportsRepeat(key))
	{
		return;
	}

	beChatWindow.repeatKey = key;
	beChatWindow.repeatNextTime = cg.time + BE_CHAT_REPEAT_DELAY_MS;
}

static void CG_BEChat_KpDigitKey(int key, char digit, float maxTextW)
{
	if (!digit)
	{
		return;
	}

	if (!CG_BEChat_KeyEdge(key))
	{
		return;
	}

	CG_BEChat_BeginRepeat(key);
	CG_BEChat_InputChar(&beChatWindow.input, digit, maxTextW);
}

static int CG_BEChat_ApplyShiftChar(int ch)
{
	if (ch >= 'a' && ch <= 'z')
	{
		return ch - ('a' - 'A');
	}

	switch (ch)
	{
	case '1':
		return '!';
	case '2':
		return '@';
	case '3':
		return '#';
	case '4':
		return '$';
	case '5':
		return '%';
	case '6':
		return '^';
	case '7':
		return '&';
	case '8':
		return '*';
	case '9':
		return '(';
	case '0':
		return ')';
	case '-':
		return '_';
	case '=':
		return '+';
	case '[':
		return '{';
	case ']':
		return '}';
	case '\\':
		return '|';
	case ';':
		return ':';
	case '\'':
		return '"';
	case ',':
		return '<';
	case '.':
		return '>';
	case '/':
		return '?';
	case '`':
		return '~';
	default:
		return ch;
	}
}

static int CG_BEChat_ApplyInputKeyChar(int ch)
{
	qboolean shift;
	qboolean caps;

	caps = CG_KeyCapsLockOn();

	if (!cg_keyCapsLockOnAvail)
	{
		if (trap_Key_IsDown(K_SHIFT))
		{
			return CG_BEChat_ApplyShiftChar(ch);
		}
		return ch;
	}

	shift = trap_Key_IsDown(K_SHIFT);

	if (ch >= 'a' && ch <= 'z')
	{
		if (shift)
		{
			if (caps)
			{
				return ch;
			}
			return CG_BEChat_ApplyShiftChar(ch);
		}
		if (caps)
		{
			return ch - ('a' - 'A');
		}
		return ch;
	}

	if (ch >= 'A' && ch <= 'Z')
	{
		if (shift ^ caps)
		{
			return ch;
		}
		return ch + ('a' - 'A');
	}

	if (shift)
	{
		return CG_BEChat_ApplyShiftChar(ch);
	}
	return ch;
}



static void CG_BEChat_RepeatKeyAction(int key, float maxTextW)
{
	int ch;

	if (key == K_PGUP)
	{
		CG_BEChat_Scroll(1);
		return;
	}

	if (key == K_PGDN)
	{
		CG_BEChat_Scroll(-1);
		return;
	}

	if (!trap_Key_IsDown(K_CTRL))
	{
		if (key == K_UPARROW || key == K_DOWNARROW ||
		        key == K_KP_UPARROW || key == K_KP_DOWNARROW)
		{
			if (CG_BEChat_SentHistProcessKey(key, maxTextW))
			{
				return;
			}
		}
		if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
		{
			if (CG_BEChat_CmdAcProcessKey(key, &beChatWindow.input, maxTextW))
			{
				return;
			}
			if (CG_BEChat_EmojiProcessKey(key, &beChatWindow.input, maxTextW))
			{
				return;
			}
		}
	}

	if (trap_Key_IsDown(K_CTRL))
	{
		if (key == K_UPARROW || key == K_KP_UPARROW)
		{
			CG_BEChat_Scroll(1);
			return;
		}
		if (key == K_DOWNARROW || key == K_KP_DOWNARROW)
		{
			CG_BEChat_Scroll(-1);
			return;
		}
	}

	if (key >= 32 && key < 127)
	{
		ch = CG_BEChat_ApplyInputKeyChar(key);
		CG_BEChat_InputChar(&beChatWindow.input, ch, maxTextW);
		return;
	}

	if (key == K_BACKSPACE)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_BACKSPACE, maxTextW);
		return;
	}

	if (key == K_DEL)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_DEL, maxTextW);
		return;
	}

	if (key == K_KP_DEL)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '.', maxTextW);
		return;
	}

	if (key == K_LEFTARROW)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_LEFTARROW, maxTextW);
		return;
	}

	if (key == K_KP_LEFTARROW)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '4', maxTextW);
		return;
	}

	if (key == K_RIGHTARROW)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_RIGHTARROW, maxTextW);
		return;
	}

	if (key == K_KP_RIGHTARROW)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '6', maxTextW);
		return;
	}

	if (key == K_HOME)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_HOME, maxTextW);
		return;
	}

	if (key == K_KP_HOME)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '7', maxTextW);
		return;
	}

	if (key == K_END)
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_END, maxTextW);
		return;
	}

	if (key == K_KP_END)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '1', maxTextW);
		return;
	}

	if (key == K_KP_5)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '5', maxTextW);
		return;
	}

	if (key == K_KP_INS)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '0', maxTextW);
		return;
	}

	if (key == K_KP_UPARROW)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '8', maxTextW);
		return;
	}

	if (key == K_KP_DOWNARROW)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '2', maxTextW);
		return;
	}

	if (key == K_KP_PGUP)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '9', maxTextW);
		return;
	}

	if (key == K_KP_PGDN)
	{
		CG_BEChat_InputChar(&beChatWindow.input, '3', maxTextW);
		return;
	}

	if (key == K_UPARROW || key == K_KP_UPARROW)
	{
		if (CG_BEChat_SentHistProcessKey(key, maxTextW))
		{
			return;
		}
		if (CG_BEChat_CmdAcProcessKey(K_UPARROW, &beChatWindow.input, maxTextW))
		{
			return;
		}
		CG_BEChat_EmojiProcessKey(K_UPARROW, &beChatWindow.input, maxTextW);
		return;
	}

	if (key == K_DOWNARROW || key == K_KP_DOWNARROW)
	{
		if (CG_BEChat_SentHistProcessKey(key, maxTextW))
		{
			return;
		}
		if (CG_BEChat_CmdAcProcessKey(K_DOWNARROW, &beChatWindow.input, maxTextW))
		{
			return;
		}
		CG_BEChat_EmojiProcessKey(K_DOWNARROW, &beChatWindow.input, maxTextW);
	}
}

static void CG_BEChat_PollKeyRepeat(float maxTextW)
{
	int key;

	key = beChatWindow.repeatKey;
	if (key < 0)
	{
		return;
	}

	if (!trap_Key_IsDown(key))
	{
		beChatWindow.repeatKey = -1;
		return;
	}

	if ((key == K_UPARROW || key == K_DOWNARROW) &&
	        trap_Key_IsDown(K_CTRL) == qfalse &&
	        !CG_BEChat_EmojiAcIsActive() && !CG_BEChat_CmdAcIsActive() &&
	        !CG_BEChat_SentHistCanNavigate())
	{
		beChatWindow.repeatKey = -1;
		return;
	}

	if (cg.time < beChatWindow.repeatNextTime)
	{
		return;
	}

	CG_BEChat_RepeatKeyAction(key, maxTextW);
	beChatWindow.repeatNextTime = cg.time + BE_CHAT_REPEAT_INTERVAL_MS;
}

static void CG_BEChat_ProcessKey(int keycode)
{
	int ch;
	float maxTextW;

	maxTextW = CG_BEChat_InputFieldMaxWidth();

	if (CG_BEChat_ConsoleBlocksInput())
	{
		return;
	}

	if (CG_BEChat_CmdAcProcessKey(keycode, &beChatWindow.input, maxTextW))
	{
		return;
	}

	if (CG_BEChat_EmojiProcessKey(keycode, &beChatWindow.input, maxTextW))
	{
		return;
	}

	if (keycode == K_TAB)
	{
		CG_BEChat_ProcessTabKey(maxTextW);
		return;
	}

	if (keycode == K_ALT)
	{
		if (beChatWindow.latchCloseKey != K_ALT)
		{
			CG_BEChat_EmojiTogglePicker();
		}
		return;
	}

	if (keycode == K_ESCAPE)
	{
		if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
		{
			CG_BEChat_CancelMuteConfirm();
			return;
		}
		CG_BEChat_Close();
		return;
	}

	CG_BEChat_HandleScrollKey(keycode);

	if (keycode == K_ENTER || keycode == K_KP_ENTER)
	{
		CG_BEChat_SendInput();
		return;
	}

	if (keycode & K_CHAR_FLAG)
	{
		keycode &= ~K_CHAR_FLAG;
		if (keycode == 'a' - 'a' + 1)
		{
			beChatWindow.input.cursor = 0;
			beChatWindow.input.scroll = 0;
			return;
		}
		if (keycode == 'e' - 'a' + 1)
		{
			beChatWindow.input.cursor = strlen(beChatWindow.input.buffer);
			CG_BEChat_InputClampScroll(&beChatWindow.input, maxTextW);
			return;
		}
		if (keycode == 'h' - 'a' + 1)
		{
			CG_BEChat_InputKeyDown(&beChatWindow.input, K_BACKSPACE, maxTextW);
			return;
		}
		if (keycode == 'l' - 'a' + 1)
		{
			CG_BEChat_ClearMessageHistory();
			return;
		}
		if (keycode < ' ')
		{
			return;
		}
		CG_BEChat_InputChar(&beChatWindow.input, CG_BEChat_ApplyInputKeyChar(keycode), maxTextW);
		return;
	}

	if (keycode >= 32 && keycode < 127)
	{
		ch = keycode;
		if (trap_Key_IsDown(K_CTRL))
		{
			if (ch == 'a' || ch == 'A')
			{
				beChatWindow.input.cursor = 0;
				beChatWindow.input.scroll = 0;
				return;
			}
			if (ch == 'e' || ch == 'E')
			{
				beChatWindow.input.cursor = strlen(beChatWindow.input.buffer);
				CG_BEChat_InputClampScroll(&beChatWindow.input, maxTextW);
				return;
			}
			if (ch == 'l' || ch == 'L')
			{
				CG_BEChat_ClearMessageHistory();
				return;
			}
		}
		ch = CG_BEChat_ApplyInputKeyChar(ch);
		CG_BEChat_InputChar(&beChatWindow.input, ch, maxTextW);
		return;
	}

	CG_BEChat_InputKeyDown(&beChatWindow.input, keycode, maxTextW);
}

void CG_BEChat_Frame(stereoFrame_t stereoView)
{
	int k;
	float maxTextW;

	if (!cg_chatWindow.integer)
	{
		if (beChatWindow.open)
		{
			CG_BEChat_Close();
		}
		return;
	}

	if (!beChatWindow.open)
	{
		return;
	}

	if (stereoView != STEREO_CENTER)
	{
		return;
	}

	if (beChatWindow.historyOnly)
	{
		return;
	}

	if (CG_BEChat_ConsoleBlocksInput())
	{
		for (k = 0; k < 256; k++)
		{
			beChatWindow.keyDown[k] = trap_Key_IsDown(k);
		}
		beChatWindow.repeatKey = -1;
		return;
	}

	maxTextW = CG_BEChat_InputFieldMaxWidth();

	CG_BEChat_ScrollbarLayoutUpdate();
	beChatMouse1Prev = trap_Key_IsDown(K_MOUSE1);
	CG_BEChat_ScrollbarDragPoll();
	CG_BEChat_EmojiPickerDragPoll();
	CG_BEChat_EmojiPickerHoverFrame();
	CG_BEChat_HelpHoverFrame();

	if (cg.time < beChatWindow.inputGraceUntil)
	{
		for (k = 0; k < 256; k++)
		{
			beChatWindow.keyDown[k] = trap_Key_IsDown(k);
		}
		return;
	}

	if (beChatWindow.latchCloseKey >= 0 &&
	        CG_BEChat_KeyEdge(beChatWindow.latchCloseKey))
	{
		CG_BEChat_Close();
		return;
	}

	if (CG_BEChat_KeyEdge(K_ESCAPE))
	{
		if (beChatWindow.destMode == BE_CHAT_DEST_MUTE_CONFIRM)
		{
			CG_BEChat_CancelMuteConfirm();
			return;
		}
		CG_BEChat_Close();
		return;
	}

	if (CG_BEChat_KeyEdge(K_ENTER) || CG_BEChat_KeyEdge(K_KP_ENTER))
	{
		CG_BEChat_SendInput();
	}

	if (CG_BEChat_KeyEdge(K_PGUP))
	{
		CG_BEChat_Scroll(1);
		CG_BEChat_BeginRepeat(K_PGUP);
	}
	else if (CG_BEChat_KeyEdge(K_KP_PGUP))
	{
		CG_BEChat_KpDigitKey(K_KP_PGUP, '9', maxTextW);
	}

	if (CG_BEChat_KeyEdge(K_PGDN))
	{
		CG_BEChat_Scroll(-1);
		CG_BEChat_BeginRepeat(K_PGDN);
	}
	else if (CG_BEChat_KeyEdge(K_KP_PGDN))
	{
		CG_BEChat_KpDigitKey(K_KP_PGDN, '3', maxTextW);
	}

	if (trap_Key_IsDown(K_CTRL))
	{
		if (CG_BEChat_KeyEdge(K_UPARROW))
		{
			CG_BEChat_Scroll(1);
			CG_BEChat_BeginRepeat(K_UPARROW);
		}
		else if (CG_BEChat_KeyEdge(K_KP_UPARROW))
		{
			CG_BEChat_Scroll(1);
			CG_BEChat_BeginRepeat(K_KP_UPARROW);
		}

		if (CG_BEChat_KeyEdge(K_DOWNARROW))
		{
			CG_BEChat_Scroll(-1);
			CG_BEChat_BeginRepeat(K_DOWNARROW);
		}
		else if (CG_BEChat_KeyEdge(K_KP_DOWNARROW))
		{
			CG_BEChat_Scroll(-1);
			CG_BEChat_BeginRepeat(K_KP_DOWNARROW);
		}
	}

	if (CG_BEChat_KeyEdge(K_TAB))
	{
		CG_BEChat_ProcessTabKey(maxTextW);
	}

	if (beChatWindow.latchCloseKey != K_ALT && CG_BEChat_KeyEdge(K_ALT))
	{
		CG_BEChat_EmojiTogglePicker();
	}

	if (CG_BEChat_KeyEdge(K_UPARROW) && !trap_Key_IsDown(K_CTRL))
	{
		if (CG_BEChat_SentHistProcessKey(K_UPARROW, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_UPARROW);
		}
		else if (CG_BEChat_CmdAcProcessKey(K_UPARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_UPARROW);
		}
		else if (CG_BEChat_EmojiProcessKey(K_UPARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_UPARROW);
		}
	}

	if (CG_BEChat_KeyEdge(K_DOWNARROW) && !trap_Key_IsDown(K_CTRL))
	{
		if (CG_BEChat_SentHistProcessKey(K_DOWNARROW, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_DOWNARROW);
		}
		else if (CG_BEChat_CmdAcProcessKey(K_DOWNARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_DOWNARROW);
		}
		else if (CG_BEChat_EmojiProcessKey(K_DOWNARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_DOWNARROW);
		}
	}

	if (CG_BEChat_KeyEdge(K_BACKSPACE))
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_BACKSPACE, maxTextW);
		CG_BEChat_BeginRepeat(K_BACKSPACE);
	}

	if (CG_BEChat_KeyEdge(K_DEL))
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_DEL, maxTextW);
		CG_BEChat_BeginRepeat(K_DEL);
	}
	else if (CG_BEChat_KeyEdge(K_KP_DEL))
	{
		CG_BEChat_InputChar(&beChatWindow.input, '.', maxTextW);
		CG_BEChat_BeginRepeat(K_KP_DEL);
	}

	CG_BEChat_KpDigitKey(K_KP_LEFTARROW, '4', maxTextW);
	if (CG_BEChat_KeyEdge(K_LEFTARROW))
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_LEFTARROW, maxTextW);
		CG_BEChat_BeginRepeat(K_LEFTARROW);
	}

	if (CG_BEChat_KeyEdge(K_KP_RIGHTARROW))
	{
		if (CG_BEChat_CmdAcProcessKey(K_KP_RIGHTARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_KP_RIGHTARROW);
		}
		else if (CG_BEChat_EmojiProcessKey(K_KP_RIGHTARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_KP_RIGHTARROW);
		}
		else
		{
			CG_BEChat_KpDigitKey(K_KP_RIGHTARROW, '6', maxTextW);
		}
	}
	if (CG_BEChat_KeyEdge(K_RIGHTARROW))
	{
		if (CG_BEChat_CmdAcProcessKey(K_RIGHTARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_RIGHTARROW);
		}
		else if (CG_BEChat_EmojiProcessKey(K_RIGHTARROW, &beChatWindow.input, maxTextW))
		{
			CG_BEChat_BeginRepeat(K_RIGHTARROW);
		}
		else
		{
			CG_BEChat_InputKeyDown(&beChatWindow.input, K_RIGHTARROW, maxTextW);
			CG_BEChat_BeginRepeat(K_RIGHTARROW);
		}
	}

	CG_BEChat_KpDigitKey(K_KP_HOME, '7', maxTextW);
	if (CG_BEChat_KeyEdge(K_HOME))
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_HOME, maxTextW);
		CG_BEChat_BeginRepeat(K_HOME);
	}

	CG_BEChat_KpDigitKey(K_KP_END, '1', maxTextW);
	if (CG_BEChat_KeyEdge(K_END))
	{
		CG_BEChat_InputKeyDown(&beChatWindow.input, K_END, maxTextW);
		CG_BEChat_BeginRepeat(K_END);
	}

	CG_BEChat_KpDigitKey(K_KP_5, '5', maxTextW);
	CG_BEChat_KpDigitKey(K_KP_INS, '0', maxTextW);
	if (!trap_Key_IsDown(K_CTRL))
	{
		CG_BEChat_KpDigitKey(K_KP_UPARROW, '8', maxTextW);
		CG_BEChat_KpDigitKey(K_KP_DOWNARROW, '2', maxTextW);
	}

	for (k = 32; k < 127; k++)
	{
		if (k == K_BACKSPACE)
		{
			continue;
		}
		if (CG_BEChat_KeyEdge(k))
		{
			CG_BEChat_ProcessKey(k);
			CG_BEChat_BeginRepeat(k);
		}
	}

	CG_BEChat_PollKeyRepeat(maxTextW);
}

void CG_BEChat_KeyEvent(int key, qboolean down)
{
	int k;

	if (!beChatWindow.open)
	{
		return;
	}

	k = key & ~K_CHAR_FLAG;

	if (beChatWindow.historyOnly)
	{
		return;
	}

	if (CG_BEChat_ConsoleBlocksInput())
	{
		if (k >= 0 && k < 256)
		{
			beChatWindow.keyDown[k] = down ? trap_Key_IsDown(k) : qfalse;
		}
		return;
	}

#if BE_CHAT_MOUSE_DEBUG
	if (down && (k == K_MOUSE1 || k == K_MOUSE2 || k == K_MOUSE3 || k == K_MWHEELUP || k == K_MWHEELDOWN))
	{
		CG_Printf("BEChat KeyEvent: key=%i down=%i (M1=%i M2=%i)\n",
		          k, down, K_MOUSE1, K_MOUSE2);
		CG_BEChat_DebugMousePress("KeyEvent");
	}
#endif

	if (down)
	{
		if (key & K_CHAR_FLAG)
		{
			if (cg.time >= beChatWindow.inputGraceUntil)
			{
				CG_BEChat_ProcessKey(key);
			}
			return;
		}

		if (k == K_MWHEELUP)
		{
			int step;

			if (CG_BEChat_EmojiPickerWheel(1))
			{
				return;
			}
			step = trap_Key_IsDown(K_CTRL) ? BE_CHAT_SCROLL_CTRL_STEP : 1;
			CG_BEChat_Scroll(step);
			return;
		}
		if (k == K_MWHEELDOWN)
		{
			int step;

			if (CG_BEChat_EmojiPickerWheel(0))
			{
				return;
			}
			step = trap_Key_IsDown(K_CTRL) ? BE_CHAT_SCROLL_CTRL_STEP : 1;
			CG_BEChat_Scroll(-step);
			return;
		}
		if (k == K_MOUSE1)
		{
			CG_BEChat_HandleMouse1Press();
			beChatMouse1Prev = qtrue;
			return;
		}
	}
	else
	{
		if (k == K_MOUSE1)
		{
			beChatMouse1Prev = qfalse;
		}
		if (k >= 0 && k < 256)
		{
			beChatWindow.keyDown[k] = qfalse;
		}
	}
}

void CG_BEChat_MouseEvent(int dx, int dy)
{
	float maxX;
	float maxY;
	float halfW;
	float halfH;

	if (!beChatWindow.open || beChatWindow.historyOnly)
	{
		return;
	}

	if (CG_BEChat_ConsoleBlocksInput())
	{
		return;
	}

	beChatWindow.cursorX += (float)dx;
	beChatWindow.cursorY += (float)dy;

	maxX = (float)cgs.glconfig.vidWidth;
	maxY = (float)cgs.glconfig.vidHeight;
	if (maxX < 1.0f)
	{
		maxX = 1.0f;
	}
	if (maxY < 1.0f)
	{
		maxY = 1.0f;
	}

	halfW = BECHAT_CURSOR_W * 0.5f;
	halfH = BECHAT_CURSOR_H * 0.5f;
	CG_AdjustFrom640_Old(NULL, NULL, &halfW, &halfH, qtrue);

	if (beChatWindow.cursorX < halfW)
	{
		beChatWindow.cursorX = halfW;
	}
	else if (beChatWindow.cursorX > maxX - halfW)
	{
		beChatWindow.cursorX = maxX - halfW;
	}

	if (beChatWindow.cursorY < halfH)
	{
		beChatWindow.cursorY = halfH;
	}
	else if (beChatWindow.cursorY > maxY - halfH)
	{
		beChatWindow.cursorY = maxY - halfH;
	}

	beChatMouse1Prev = trap_Key_IsDown(K_MOUSE1);
	CG_BEChat_ScrollbarDragPoll();
	CG_BEChat_EmojiPickerDragPoll();
	CG_BEChat_EmojiPickerHoverFrame();
	CG_BEChat_HelpHoverFrame();
}

static void CG_BEChat_DrawInputField(float x, float y, float w, const vec4_t color)
{
	beChatInput_t* edit;
	char drawBuf[MAX_STRING_CHARS];
	int len;
	int drawLen;
	int prestep;
	float drawX;
	float cursorX;
	float maxTextW;
	vec4_t cursorColor;
	float winX;
	float winY;
	float winW;
	float winH;

	edit = &beChatWindow.input;
	len = strlen(edit->buffer);
	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_EmojiPrepareInputRow(winX, winY, winW, winH);
	maxTextW = CG_BEChat_EmojiInputTextMaxWidth(x);
	if (maxTextW < BECHAT_CHAR_W)
	{
		maxTextW = BECHAT_CHAR_W;
	}

	CG_BEChat_InputClampScroll(edit, maxTextW);
	CG_BEChat_InputSnapScrollPos(edit);
	prestep = edit->scroll;
	drawLen = CG_BEChat_InputVisibleBytes(edit->buffer, prestep, maxTextW);

	memcpy(drawBuf, edit->buffer + prestep, drawLen);
	drawBuf[drawLen] = '\0';

	drawX = CG_BEChat_InputTextDrawX(x);
	CG_OSPDrawStringNew(drawX, y, drawBuf, color, colorBlack,
	                    BECHAT_CHAR_W, BECHAT_CHAR_H,
	                    (int)maxTextW, CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS), BE_CHAT_FONT,
	                    NULL, NULL, NULL);

	if (edit->cursor >= prestep && beChatWindow.destMode != BE_CHAT_DEST_MUTE_CONFIRM)
	{
		char caretColorChar;

		cursorX = drawX + CG_BEChat_InputBufSliceDisplayWidth(edit->buffer, prestep, edit->cursor,
		        BECHAT_CHAR_W, BECHAT_CHAR_H,
		        CG_BEChat_EmojiTextFlags(BE_CHAT_TEXT_FLAGS), BE_CHAT_FONT);

		if (edit->cursor >= 1 && edit->buffer[edit->cursor - 1] == '^' &&
		        (edit->cursor >= len || !CG_BEChat_InputPaletteColorChar(edit->buffer[edit->cursor])))
		{
			Vector4Copy(color, cursorColor);
		}
		else
		{
			caretColorChar = CG_BEChat_ActiveColorAt(edit->buffer, edit->cursor, '7');
			CG_OSPColorFromChar(caretColorChar, cursorColor);
		}
		cursorColor[3] = ((cg.time >> 8) & 1) ? 1.0f : 0.35f;
		CG_FillRect(cursorX, y + 1.0f, 2.0f, BECHAT_CHAR_H - 2.0f, cursorColor);
	}

	CG_BEChat_EmojiDrawUI(x, y, w, edit, color, BE_CHAT_TEXT_FLAGS);
}


static void CG_BEChat_DrawScrollbar(float x, float y, float w, float trackH, int lineCount, int visibleLines,
                                    int historyScroll)
{
	vec4_t track;
	vec4_t thumb;
	vec4_t border;
	float contentH;
	float scrollPx;
	float thumbH;
	float thumbY;
	float maxScrollPx;
	int maxScrollLines;

	if (lineCount <= visibleLines || visibleLines <= 0 || trackH <= 0.0f)
	{
		return;
	}

	contentH = (float)lineCount * BECHAT_CHAR_H;
	maxScrollLines = lineCount - visibleLines;
	if (historyScroll < 0)
	{
		historyScroll = 0;
	}
	if (historyScroll > maxScrollLines)
	{
		historyScroll = maxScrollLines;
	}

	/* historyScroll 0 = newest (bottom); map to scroll px with thumb at bottom */
	scrollPx = (float)(maxScrollLines - historyScroll) * BECHAT_CHAR_H;

	CG_BEChat_ScrollbarGetThumb(trackH, contentH, scrollPx, &thumbH, &thumbY, &maxScrollPx);
	if (maxScrollPx < 1.0f)
	{
		return;
	}

	track[0] = 0.25f;
	track[1] = 0.25f;
	track[2] = 0.25f;
	track[3] = 0.85f;
	CG_FillRect(x, y, w, trackH, track);

	thumb[0] = 0.65f;
	thumb[1] = 0.65f;
	thumb[2] = 0.65f;
	thumb[3] = 1.0f;
	CG_FillRect(x, y + thumbY, w, thumbH, thumb);

	border[0] = 0.45f;
	border[1] = 0.45f;
	border[2] = 0.45f;
	border[3] = 0.9f;
	CG_DrawRect(x, y, w, trackH, BECHAT_SB_BORDER, border);
}

void CG_BEChat_Draw(void)
{
	float x;
	float y;
	float w;
	float h;
	vec4_t textColor;
	vec4_t promptColor;
	int visible;
	int lineCount;
	float historyTrackH;
	float historyTextW;
	float sbX;
	vec4_t bg;
	float inputRowY;
	float helpBtnX;
	float helpBtnY;
	float helpBtnSize;
	qboolean helpShowTip;
	float helpTipAlpha;
	vec4_t helpMarkColor;
	float cursorPicX;
	float cursorPicY;
	float cursorPicW;
	float cursorPicH;

	if (!cg.snap || !cg_chatWindow.integer)
	{
		return;
	}

	x = BE_CHAT_WIN_DEF_X;
	y = BE_CHAT_WIN_DEF_Y;
	w = BE_CHAT_WIN_DEF_W;
	h = BE_CHAT_WIN_DEF_H;
	CG_BEChat_WindowLayout(&x, &y, &w, &h);

	if (!beChatWindow.open)
	{
		int lineCount;

		historyTextW = CG_BEChat_HistoryTextWidth(&lineCount, qtrue);
		if (lineCount < 1)
		{
			return;
		}

		CG_BEChat_DrawHistoryLines(x, y, w, 0, historyTextW, qtrue);
		return;
	}

	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	promptColor[0] = 1.0f;
	promptColor[1] = 0.8f;
	promptColor[2] = 0.2f;
	promptColor[3] = 1.0f;

	Vector4Copy(cgs.beChatWindowBgColor, bg);
	bg[3] = cg_chatWindowBgOpaque.value;
	if (!(cg_chatWindow.integer & BE_CHAT_INPUT_ONLY_BG_BIT))
	{
		float bgH;

		bgH = h;
		if (beChatWindow.historyOnly)
		{
			bgH -= BECHAT_INPUT_ROW_H;
		}
		CG_FillRect(x, y, w, bgH, bg);
	}

	visible = CG_BEChat_VisibleHistoryLines();
	historyTextW = CG_BEChat_HistoryTextWidth(&lineCount, qfalse);
	historyTrackH = (float)visible * BECHAT_CHAR_H;

	if (lineCount > visible)
	{
		sbX = x + w - BECHAT_PAD - BECHAT_SB_W;
		CG_BEChat_DrawScrollbar(sbX, y + BECHAT_PAD, BECHAT_SB_W, historyTrackH,
		                        lineCount, visible, beChatWindow.historyScroll);
	}

	CG_BEChat_DrawHistoryLines(x, y, w, beChatWindow.historyScroll, historyTextW, qfalse);

	if (beChatWindow.historyOnly)
	{
		return;
	}

	inputRowY = CG_BEChat_InputRowY(y, h);
	if (cg_chatWindow.integer & BE_CHAT_INPUT_ONLY_BG_BIT)
	{
		CG_FillRect(x, CG_BEChat_InputRowBgY(y, h),
		            historyTextW + BECHAT_PAD, BECHAT_INPUT_ROW_H, bg);
	}
	CG_OSPDrawStringNew(x + BECHAT_PAD, inputRowY, CG_BEChat_InputPromptText(), promptColor, colorBlack,
	                    BECHAT_CHAR_W, BECHAT_CHAR_H, 0, BE_CHAT_TEXT_FLAGS, BE_CHAT_FONT,
	                    NULL, NULL, NULL);
	CG_BEChat_DrawInputField(x + BECHAT_PAD, inputRowY, historyTextW, textColor);

	CG_BEChat_HelpLayout(x, y, w, h, &helpBtnX, &helpBtnY, &helpBtnSize);
	helpMarkColor[0] = 0.0f;
	helpMarkColor[1] = 0.0f;
	helpMarkColor[2] = 0.0f;
	helpMarkColor[3] = BE_CHAT_HELP_ALPHA;
	CG_OSPDrawStringNew(helpBtnX + helpBtnSize * 0.5f, helpBtnY + helpBtnSize * 0.5f, "?",
	                    helpMarkColor, colorBlack,
	                    helpBtnSize * 0.6f, helpBtnSize * 0.8f,
	                    (int)helpBtnSize, BE_CHAT_HELP_QMARK_FLAGS, BE_CHAT_FONT,
	                    NULL, NULL, NULL);
	helpTipAlpha = CG_BEChat_HelpTipAlpha(&helpShowTip);
	if (helpShowTip)
	{
		CG_BEChat_HelpDrawTooltip(helpBtnX, helpBtnY, helpBtnSize, x, y, helpTipAlpha);
	}

	CG_BEChat_CursorScreenRect(&cursorPicX, &cursorPicY, &cursorPicW, &cursorPicH);
	trap_R_SetColor(NULL);
	trap_R_DrawStretchPic(cursorPicX, cursorPicY, cursorPicW, cursorPicH, 0, 0, 1, 1, cgs.media.cursor);
}

void CG_BEChat_EventHandling(int type)
{
	if (type == 0 && beChatWindow.open && !beChatWindow.historyOnly)
	{
		
		CG_BEChat_Close();
	}
}

qboolean CG_BEChat_IsOpen(void)
{
	return beChatWindow.open;
}

void CG_BEChat_f(void)
{
	CG_BEChat_OpenEx(qfalse);
}

void CG_BEChat_Team_f(void)
{
	CG_BEChat_OpenEx(qtrue);
}

void CG_BEChat_ShowHistoryDown_f(void)
{
	CG_BEChat_OpenHistoryView();
}

void CG_BEChat_ShowHistoryUp_f(void)
{
	CG_BEChat_CloseHistoryView();
}

void CG_BEChat_ScrollUp_f(void)
{
	if (beChatWindow.open && beChatWindow.historyOnly)
	{
		int step;
		step = trap_Key_IsDown(K_CTRL) ? BE_CHAT_SCROLL_CTRL_STEP : 1;
		CG_BEChat_Scroll(step);
	}
}

void CG_BEChat_ScrollDown_f(void)
{
	if (beChatWindow.open && beChatWindow.historyOnly)
	{
		int step;
		step = trap_Key_IsDown(K_CTRL) ? BE_CHAT_SCROLL_CTRL_STEP : 1;
		CG_BEChat_Scroll(-step);
	}
}
