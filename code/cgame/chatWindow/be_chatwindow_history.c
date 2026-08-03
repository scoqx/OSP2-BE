/*
===========================================================================
cg_be_chatwindow_history.c — Message history, row cache, scrolling.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

static void CG_BEChat_InvalidateRowsCache(void)
{
	beChatRowsCacheWriteIdx = -1;
}
void CG_BEChat_EncodePercentWire(const char* src, char* dst, int dstSize)
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
	int lineMax;
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
		lineMax = CG_BEChat_HistoryLineMax();
		lastIndex = history->writeIndex - 1;
		while (lastIndex < 0)
		{
			lastIndex += lineMax;
		}
		lastIndex %= lineMax;
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

	lineMax = CG_BEChat_HistoryLineMax();
	index = history->writeIndex % lineMax;
	Q_strncpyz(history->lines[index].text, text, sizeof(history->lines[index].text));
	history->lines[index].senderClientNum = senderClientNum;
	history->lines[index].repeat = 1;
	history->lines[index].addTime = cg.time;
	history->lines[index].msgKind = msgKind;
	history->writeIndex++;
	if (history->count < lineMax)
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
	int lineMax;

	history = &cgs.beChatHistory;
	if (linesBack < 0 || linesBack >= history->count)
	{
		return -1;
	}

	lineMax = CG_BEChat_HistoryLineMax();
	index = history->writeIndex - 1 - linesBack;
	while (index < 0)
	{
		index += lineMax;
	}
	index %= lineMax;
	return history->lines[index].senderClientNum;
}
int CG_BEChat_VisibleHistoryLines(void)
{
	float historyH;
	float winH;

	CG_BEChat_WindowLayout(NULL, NULL, NULL, &winH);
	/* Same history band as when the input row is shown (closed overlay matches open layout). */
	historyH = winH - BECHAT_INPUT_ROW_H - (BECHAT_PAD * 2.0f) - BECHAT_INPUT_HISTORY_GAP;
	if (historyH < BECHAT_CHAR_H)
	{
		return 1;
	}
	return (int)(historyH / BECHAT_CHAR_H);
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
char CG_BEChat_ActiveColorAt(const char* text, int byteIndex, char defaultColor)
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
qboolean CG_BEChat_InputPaletteColorChar(char c)
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
qboolean CG_BEChat_InputPaletteCodeAt(const char* buf, int pos, int bufLen)
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
int CG_BEChat_InputCopyDisplay(const char* buf, int start, int end, char* out, int outSize)
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
int CG_BEChat_InputAdvanceBufPos(const char* buf, int pos, int bufLen)
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
static qboolean CG_BEChat_WrapSegmentHasVisibleText(const char* text, int start, int nbytes, int len)
{
	int p;
	int end;

	end = start + nbytes;
	if (end > len)
	{
		end = len;
	}
	for (p = start; p < end; )
	{
		if (Q_IsColorString(text + p))
		{
			p += 2;
			continue;
		}
		if (text[p] == '^' && p + 8 <= end && (text[p + 1] == 'X' || text[p + 1] == 'x'))
		{
			p += 8;
			continue;
		}
		if (text[p] != ' ' && text[p] != '\0')
		{
			return qtrue;
		}
		p++;
	}
	return qfalse;
}
static int CG_BEChat_NextWrapBytes(const char* text, int start, float maxWidth)
{
	int len;
	int lastFit;
	int lastSpaceFit;
	int pos;
	int next;
	int textFlags;
	float rowW;

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

	/* Same unit-by-unit width walk as the chat input field (no mid-^X splits). */
	lastFit = CG_BEChat_InputVisibleBytes(text, start, maxWidth);
	if (lastFit < 1)
	{
		next = CG_BEChat_InputAdvanceBufPos(text, start, len);
		if (next <= start)
		{
			return 1;
		}
		return next - start;
	}

	lastSpaceFit = 0;
	for (pos = start; pos < start + lastFit; )
	{
		next = CG_BEChat_InputAdvanceBufPos(text, pos, len);
		if (next > start + lastFit)
		{
			break;
		}
		if (text[pos] == ' ')
		{
			lastSpaceFit = next - start;
		}
		pos = next;
	}

	if (lastSpaceFit > 0 && lastSpaceFit < lastFit && start + lastFit < len)
	{
		lastFit = lastSpaceFit;
	}

	while (!CG_BEChat_WrapSegmentHasVisibleText(text, start, lastFit, len) &&
	        start + lastFit < len)
	{
		next = CG_BEChat_InputAdvanceBufPos(text, start + lastFit, len);
		if (next <= start + lastFit)
		{
			break;
		}
		rowW = CG_BEChat_InputBufSliceDisplayWidth(text, start, next,
		        BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT);
		if (rowW > maxWidth)
		{
			break;
		}
		lastFit = next - start;
	}

	if (CG_BEChat_InputVisibleBytes(text, start, maxWidth) >= len - start)
	{
		lastFit = len - start;
	}
	else
	{
		rowW = CG_BEChat_InputBufSliceDisplayWidth(text, start, len,
		        BECHAT_CHAR_W, BECHAT_CHAR_H, textFlags, BE_CHAT_FONT);
		if (len - (start + lastFit) > 0 && len - (start + lastFit) <= 2 && rowW <= maxWidth)
		{
			lastFit = len - start;
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
float CG_BEChat_HistoryTextWidth(int* outDisplayRows, qboolean timeFilter)
{
	float w;
	float wFull;
	float winW;
	int total;

	CG_BEChat_WindowLayout(NULL, NULL, &winW, NULL);
	wFull = winW - BECHAT_PAD * 2.0f;
	/* Always reserve scrollbar gutter so wrap matches fade / enterchat / +showchat. */
	w = wFull - BECHAT_SB_W - BECHAT_SB_GAP;
	if (w < BECHAT_CHAR_W * 2.0f)
	{
		w = BECHAT_CHAR_W * 2.0f;
	}
	CG_BEChat_EnsureRowsCache(w, timeFilter);
	total = beChatRowsCacheTotal;

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
void CG_BEChat_DrawHistoryLines(float x, float y, float w, int historyScroll, float historyTextW,
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
void CG_BEChat_Scroll(int delta)
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
