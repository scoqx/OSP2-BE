/*
===========================================================================
cg_be_chatwindow_emoji.c — enterchat emoji picker, autocomplete, cursor units
===========================================================================
*/

#include "cg_local.h"
#include "../ui/keycodes.h"

#define BE_EMOJI_FONT           cgFontIndices.chatWindow
#define BE_EMOJI_TEXT_FLAGS     (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL | DS_EMOJI)
#define BE_EMOJI_GRID_COLS      8
#define BE_EMOJI_GRID_ROWS      6
#define BE_EMOJI_AC_MAX         32
#define BE_EMOJI_PREVIEW_SCALE  1.2f
#define BE_EMOJI_TIP_DELAY_MS   800
#define BE_EMOJI_TIP_FADE_MS    100
#define BE_EMOJI_TIP_TEXT_FLAGS (DS_HCENTER | DS_SHADOW | DS_PROPORTIONAL)
#define BE_EMOJI_PRESS_FADE_MS  220
#define BE_EMOJI_HOVER_GRAY     0.42f
#define BE_EMOJI_HOVER_GRAY_A   0.45f

typedef struct
{
	qboolean    pickerOpen;
	float       winX;
	float       winY;
	float       winW;
	float       winH;
	float       btnX;
	float       btnY;
	float       btnW;
	float       btnH;
	float       panelX;
	float       panelY;
	float       panelW;
	float       panelH;
	float       gridW;
	float       cellW;
	float       cellH;
	float       pickerSbX;
	float       pickerSbY;
	float       pickerSbW;
	float       pickerSbH;
	float       pickerSbThumbY;
	float       pickerSbThumbH;
	float       pickerSbMaxScrollPx;
	qboolean    pickerSbActive;
	qboolean    pickerSbDragging;
	float       pickerSbGrabDeltaY;
	int         pickerScroll;
	int         pickerScrollMax;
	int         pickerHoverIndex;
	int         pickerTipShowIndex;
	int         pickerHoverStartTime;
	int         pickerTipHideStartTime;
	qboolean    pickerTipFadingOut;
	qboolean    btnHover;
	int         btnFlashTime;
	int         cellFlashIndex;
	int         cellFlashTime;
	int         acColonPos;
	char        acPrefix[64];
	int         acCount;
	int         acSel;
	const emoji_t* acList[BE_EMOJI_AC_MAX];
} beChatEmojiUI_t;

static beChatEmojiUI_t beChatEmoji;

static void CG_BEChat_EmojiPlainName(const char* token, char* out, int outSize)
{
	int tokenLen;
	int copyLen;

	if (!token || !out || outSize <= 0)
	{
		return;
	}

	tokenLen = strlen(token);
	if (tokenLen >= 3 && token[0] == EMOJI_CHAR && token[tokenLen - 1] == EMOJI_CHAR)
	{
		copyLen = tokenLen - 2;
		if (copyLen >= outSize)
		{
			copyLen = outSize - 1;
		}
		memcpy(out, token + 1, copyLen);
		out[copyLen] = '\0';
	}
	else
	{
		Q_strncpyz(out, token, outSize);
	}
}

static const emoji_t* CG_BEChat_EmojiAt(const char* buf, int index, int* outStart)
{
	const emoji_t* em;

	if (!buf || index < 0 || buf[index] != EMOJI_CHAR)
	{
		return NULL;
	}

	em = CG_OSPGetEmoji(buf + index);
	if (em && outStart)
	{
		*outStart = index;
	}
	return em;
}

static void CG_BEChat_EmojiDeleteRange(beChatInput_t* edit, int start, int len, float maxTextW)
{
	int tail;

	if (!edit || len < 1 || start < 0)
	{
		return;
	}

	tail = strlen(edit->buffer) - (start + len);
	if (tail < 0)
	{
		tail = 0;
	}

	memmove(edit->buffer + start, edit->buffer + start + len, tail + 1);
	if (edit->cursor > start + len)
	{
		edit->cursor -= len;
	}
	else if (edit->cursor > start)
	{
		edit->cursor = start;
	}
	CG_BEChat_EmojiOnInputChanged(edit);
	(void)maxTextW;
}

int CG_BEChat_EmojiTextFlags(int baseFlags)
{
	return baseFlags | DS_EMOJI;
}

float CG_BEChat_EmojiButtonReserveWidth(void)
{
	return BECHAT_EMOJI_BTN_SIZE + BECHAT_EMOJI_BTN_PAD;
}

float CG_BEChat_EmojiInputTextMaxWidth(float contentX)
{
	float drawX;
	float maxW;

	drawX = CG_BEChat_InputTextDrawX(contentX);
	maxW = beChatEmoji.btnX - drawX - BECHAT_EMOJI_BTN_PAD - CG_BEChat_HelpBtnReserveWidth();
	if (maxW < BECHAT_CHAR_W)
	{
		maxW = BECHAT_CHAR_W;
	}
	return maxW;
}

static void CG_BEChat_EmojiPickerHoverReset(void)
{
	beChatEmoji.pickerHoverIndex = -1;
	beChatEmoji.pickerTipShowIndex = -1;
	beChatEmoji.pickerTipFadingOut = qfalse;
	beChatEmoji.btnHover = qfalse;
}

static float CG_BEChat_EmojiPressFlashAlpha(int flashStartTime)
{
	int elapsed;

	if (flashStartTime <= 0)
	{
		return 0.0f;
	}

	elapsed = cg.time - flashStartTime;
	if (elapsed < 0)
	{
		elapsed = 0;
	}
	if (elapsed >= BE_EMOJI_PRESS_FADE_MS)
	{
		return 0.0f;
	}

	return 1.0f - ((float)elapsed / (float)BE_EMOJI_PRESS_FADE_MS);
}

static void CG_BEChat_EmojiDrawHighlightRect(float x, float y, float w, float h,
        float gray, float pressAlpha)
{
	vec4_t bg;

	if (pressAlpha > 0.0f)
	{
		bg[0] = 1.0f;
		bg[1] = 1.0f;
		bg[2] = 1.0f;
		bg[3] = pressAlpha;
		CG_FillRect(x, y, w, h, bg);
		return;
	}

	if (gray > 0.0f)
	{
		bg[0] = BE_EMOJI_HOVER_GRAY;
		bg[1] = BE_EMOJI_HOVER_GRAY;
		bg[2] = BE_EMOJI_HOVER_GRAY;
		bg[3] = BE_EMOJI_HOVER_GRAY_A * gray;
		CG_FillRect(x, y, w, h, bg);
	}
}

void CG_BEChat_EmojiSessionOpen(void)
{
	memset(&beChatEmoji, 0, sizeof(beChatEmoji));
	CG_BEChat_EmojiPickerHoverReset();
	CG_OSPLoadEmoji();
}

void CG_BEChat_EmojiSessionClose(void)
{
	memset(&beChatEmoji, 0, sizeof(beChatEmoji));
	CG_BEChat_EmojiPickerHoverReset();
}

static qboolean CG_BEChat_EmojiColonClosesToken(const char* buf, int colonIndex)
{
	int j;
	const emoji_t* em;

	if (!buf || colonIndex < 0)
	{
		return qfalse;
	}

	for (j = 0; j <= colonIndex; j++)
	{
		if (buf[j] != EMOJI_CHAR)
		{
			continue;
		}
		em = CG_OSPGetEmoji(buf + j);
		if (em && j + em->len - 1 == colonIndex)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_BEChat_EmojiRefreshAc(beChatInput_t* edit)
{
	int i;
	int j;
	int prefixLen;
	const emoji_t* em;

	beChatEmoji.acCount = 0;
	beChatEmoji.acColonPos = -1;
	beChatEmoji.acPrefix[0] = '\0';

	if (!edit)
	{
		return;
	}

	for (j = 0; j < edit->cursor; j++)
	{
		if (edit->buffer[j] != EMOJI_CHAR)
		{
			continue;
		}
		em = CG_OSPGetEmoji(edit->buffer + j);
		if (!em)
		{
			continue;
		}
		if (edit->cursor >= j && edit->cursor <= j + em->len)
		{
			return;
		}
	}

	for (i = edit->cursor - 1; i >= 0; i--)
	{
		char ch;

		ch = edit->buffer[i];
		if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t')
		{
			return;
		}
		if (ch == EMOJI_CHAR)
		{
			if (CG_BEChat_EmojiColonClosesToken(edit->buffer, i))
			{
				continue;
			}
			beChatEmoji.acColonPos = i;
			break;
		}
		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
		        (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
		{
			return;
		}
	}

	if (beChatEmoji.acColonPos < 0)
	{
		return;
	}

	for (i = beChatEmoji.acColonPos + 1; i < edit->cursor; i++)
	{
		if (edit->buffer[i] == EMOJI_CHAR)
		{
			beChatEmoji.acCount = 0;
			beChatEmoji.acColonPos = -1;
			beChatEmoji.acPrefix[0] = '\0';
			return;
		}
	}

	if (CG_OSPGetEmoji(edit->buffer + beChatEmoji.acColonPos))
	{
		return;
	}

	prefixLen = edit->cursor - (beChatEmoji.acColonPos + 1);
	if (prefixLen < 0)
	{
		return;
	}
	if (prefixLen >= (int)sizeof(beChatEmoji.acPrefix))
	{
		prefixLen = (int)sizeof(beChatEmoji.acPrefix) - 1;
	}
	memcpy(beChatEmoji.acPrefix, edit->buffer + beChatEmoji.acColonPos + 1, prefixLen);
	beChatEmoji.acPrefix[prefixLen] = '\0';

	beChatEmoji.acCount = CG_OSPEmojiPrefixMatch(beChatEmoji.acPrefix,
	                      beChatEmoji.acList, BE_EMOJI_AC_MAX);
	if (beChatEmoji.acSel >= beChatEmoji.acCount)
	{
		beChatEmoji.acSel = 0;
	}
}

void CG_BEChat_EmojiOnInputChanged(beChatInput_t* edit)
{
	int i;
	const emoji_t* em;

	if (!edit)
	{
		return;
	}

	if (edit->cursor >= 3 && edit->buffer[edit->cursor - 1] == EMOJI_CHAR)
	{
		for (i = edit->cursor - 2; i >= 0; i--)
		{
			if (edit->buffer[i] != EMOJI_CHAR)
			{
				continue;
			}
			em = CG_OSPGetEmoji(edit->buffer + i);
			if (em && i + em->len == edit->cursor)
			{
				if (Q_stricmpn(edit->buffer + i, em->name, em->len))
				{
					CG_BEChat_EmojiDeleteRange(edit, i, edit->cursor - i, 0.0f);
					CG_BEChat_EmojiInsertToken(edit, em->name, 0.0f);
				}
				break;
			}
		}
	}

	CG_BEChat_EmojiRefreshAc(edit);
}

void CG_BEChat_EmojiInsertToken(beChatInput_t* edit, const char* token, float maxTextW)
{
	int len;
	int tlen;
	int room;

	if (!edit || !token || !token[0])
	{
		return;
	}

	len = strlen(edit->buffer);
	tlen = strlen(token);
	room = MAX_SAY_TEXT - 1 - len;
	if (tlen > room)
	{
		return;
	}

	memmove(edit->buffer + edit->cursor + tlen, edit->buffer + edit->cursor,
	        len + 1 - edit->cursor);
	memcpy(edit->buffer + edit->cursor, token, tlen);
	edit->cursor += tlen;
	CG_BEChat_EmojiOnInputChanged(edit);
	(void)maxTextW;
}

static void CG_BEChat_EmojiAcComplete(beChatInput_t* edit, float maxTextW)
{
	const emoji_t* em;
	int colonPos;
	int replaceLen;
	int tlen;
	int tail;

	if (!edit || beChatEmoji.acCount < 1 || beChatEmoji.acColonPos < 0)
	{
		return;
	}

	if (beChatEmoji.acSel < 0 || beChatEmoji.acSel >= beChatEmoji.acCount)
	{
		beChatEmoji.acSel = 0;
	}

	em = beChatEmoji.acList[beChatEmoji.acSel];
	if (!em)
	{
		return;
	}

	colonPos = beChatEmoji.acColonPos;
	replaceLen = edit->cursor - colonPos;
	tlen = em->len;
	tail = strlen(edit->buffer) - (colonPos + replaceLen);
	if (tail < 0)
	{
		tail = 0;
	}

	if (colonPos + tlen + tail >= MAX_SAY_TEXT)
	{
		return;
	}

	memmove(edit->buffer + colonPos + tlen, edit->buffer + colonPos + replaceLen, tail + 1);
	memcpy(edit->buffer + colonPos, em->name, tlen);
	edit->buffer[colonPos + tlen + tail] = '\0';
	edit->cursor = colonPos + tlen;
	CG_BEChat_EmojiRefreshAc(edit);
	CG_BEChat_InputClampScroll(edit, maxTextW);
}

void CG_BEChat_EmojiBackspace(beChatInput_t* edit, float maxTextW)
{
	int len;
	int i;
	int unitStart;
	int unitLen;
	const emoji_t* em;

	if (!edit || edit->cursor < 1)
	{
		return;
	}

	len = strlen(edit->buffer);
	for (i = 0; i < edit->cursor; i++)
	{
		if (edit->buffer[i] != EMOJI_CHAR)
		{
			continue;
		}
		em = CG_OSPGetEmoji(edit->buffer + i);
		if (em && i + em->len == edit->cursor)
		{
			CG_BEChat_EmojiDeleteRange(edit, i, em->len, maxTextW);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			return;
		}
		if (em && i < edit->cursor && i + em->len > edit->cursor)
		{
			CG_BEChat_EmojiDeleteRange(edit, i, em->len, maxTextW);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			return;
		}
	}

	if (edit->cursor > 0)
	{
		if (CG_BEChat_InputTryBackspacePalette(edit))
		{
			CG_BEChat_EmojiOnInputChanged(edit);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			return;
		}

		if (edit->cursor >= 2 && Q_IsColorString(edit->buffer + edit->cursor - 2))
		{
			unitLen = CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor - 2, len);
			unitStart = edit->cursor - unitLen;
		}
		else
		{
			unitLen = CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor - 1, len);
			unitStart = edit->cursor - unitLen;
		}

		memmove(edit->buffer + unitStart, edit->buffer + edit->cursor, len + 1 - edit->cursor);
		edit->cursor = unitStart;
		CG_BEChat_EmojiOnInputChanged(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
}

void CG_BEChat_EmojiDelete(beChatInput_t* edit, float maxTextW)
{
	int len;
	int start;
	const emoji_t* em;

	if (!edit)
	{
		return;
	}

	len = strlen(edit->buffer);
	if (edit->cursor >= len)
	{
		return;
	}

	em = CG_BEChat_EmojiAt(edit->buffer, edit->cursor, &start);
	if (em)
	{
		CG_BEChat_EmojiDeleteRange(edit, start, em->len, maxTextW);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	if (Q_IsColorString(edit->buffer + edit->cursor))
	{
		int unit;

		unit = CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor, len);
		memmove(edit->buffer + edit->cursor, edit->buffer + edit->cursor + unit,
		        len - edit->cursor - unit + 1);
		CG_BEChat_EmojiOnInputChanged(edit);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		return;
	}

	memmove(edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, len - edit->cursor);
	CG_BEChat_EmojiOnInputChanged(edit);
	CG_BEChat_InputClampScroll(edit, maxTextW);
}

void CG_BEChat_EmojiCursorStepLeft(beChatInput_t* edit)
{
	int i;
	const emoji_t* em;

	if (!edit || edit->cursor < 1)
	{
		return;
	}

	for (i = 0; i < edit->cursor; i++)
	{
		if (edit->buffer[i] != EMOJI_CHAR)
		{
			continue;
		}
		em = CG_OSPGetEmoji(edit->buffer + i);
		if (em && i + em->len == edit->cursor)
		{
			edit->cursor = i;
			return;
		}
	}

	if (edit->cursor > 0)
	{
		int len;
		int unitStart;
		int unitLen;

		len = strlen(edit->buffer);
		if (edit->cursor >= 2 && Q_IsColorString(edit->buffer + edit->cursor - 2))
		{
			unitLen = CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor - 2, len);
			unitStart = edit->cursor - unitLen;
		}
		else
		{
			unitLen = CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor - 1, len);
			unitStart = edit->cursor - unitLen;
		}
		edit->cursor = unitStart;
	}
}

void CG_BEChat_EmojiCursorStepRight(beChatInput_t* edit)
{
	int len;
	const emoji_t* em;

	if (!edit)
	{
		return;
	}

	len = strlen(edit->buffer);
	if (edit->cursor >= len)
	{
		return;
	}

	em = CG_BEChat_EmojiAt(edit->buffer, edit->cursor, NULL);
	if (em)
	{
		edit->cursor += em->len;
		return;
	}

	if (Q_IsColorString(edit->buffer + edit->cursor))
	{
		edit->cursor += CG_BEChat_InputByteUnitLen(edit->buffer, edit->cursor, len);
		return;
	}

	edit->cursor++;
}

qboolean CG_BEChat_EmojiAcIsActive(void)
{
	return (beChatEmoji.acCount > 0) ? qtrue : qfalse;
}

qboolean CG_BEChat_EmojiProcessKey(int keycode, beChatInput_t* edit, float maxTextW)
{
	if (!edit)
	{
		return qfalse;
	}

	if (keycode == K_TAB ||
	        ((keycode == K_RIGHTARROW || keycode == K_KP_RIGHTARROW) && !trap_Key_IsDown(K_CTRL)))
	{
		if (beChatEmoji.acCount > 0)
		{
			CG_BEChat_EmojiAcComplete(edit, maxTextW);
			CG_BEChat_InputClampScroll(edit, maxTextW);
			return qtrue;
		}
		return qfalse;
	}

	if (keycode == K_ALT)
	{
		return qfalse;
	}

	if (beChatEmoji.acCount > 0 && !trap_Key_IsDown(K_CTRL))
	{
		if (keycode == K_UPARROW || keycode == K_KP_UPARROW)
		{
			beChatEmoji.acSel--;
			if (beChatEmoji.acSel < 0)
			{
				beChatEmoji.acSel = beChatEmoji.acCount - 1;
			}
			return qtrue;
		}
		if (keycode == K_DOWNARROW || keycode == K_KP_DOWNARROW)
		{
			beChatEmoji.acSel++;
			if (beChatEmoji.acSel >= beChatEmoji.acCount)
			{
				beChatEmoji.acSel = 0;
			}
			return qtrue;
		}
	}

	return qfalse;
}

void CG_BEChat_EmojiTogglePicker(void)
{
	beChatEmoji.pickerOpen = !beChatEmoji.pickerOpen;
	if (beChatEmoji.pickerOpen)
	{
		beChatEmoji.pickerScroll = 0;
	}
	else
	{
		CG_BEChat_EmojiPickerHoverReset();
	}
}

static qboolean CG_BEChat_EmojiPointInRect(float x, float y, float rx, float ry, float rw, float rh)
{
	return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

static qboolean CG_BEChat_EmojiHitButton(float virtX, float virtY)
{
	float rx;
	float ry;
	float rw;
	float rh;

	rx = beChatEmoji.btnX - BECHAT_EMOJI_BTN_HIT_PAD;
	ry = beChatEmoji.btnY - BECHAT_EMOJI_BTN_HIT_PAD;
	rw = beChatEmoji.btnW + BECHAT_EMOJI_BTN_HIT_PAD * 2.0f;
	rh = beChatEmoji.btnH + BECHAT_EMOJI_BTN_HIT_PAD * 2.0f;
	return CG_BEChat_EmojiPointInRect(virtX, virtY, rx, ry, rw, rh);
}

static void CG_BEChat_EmojiPickerSbGetThumb(float sbH, float contentH, float scrollPx,
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
	if (thumbH < beChatEmoji.cellH)
	{
		thumbH = beChatEmoji.cellH;
	}
	if (thumbH > sbH)
	{
		thumbH = sbH;
	}

	trackRange = sbH - thumbH;
	thumbY = 0.0f;
	if (maxScroll > 0.0f && trackRange > 0.0f)
	{
		thumbY = (scrollPx / maxScroll) * trackRange;
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

static void CG_BEChat_EmojiLayoutPickerPanel(void)
{
	int total;
	int totalRows;
	float panelRight;
	float contentH;
	float scrollPx;

	if (!beChatEmoji.pickerOpen)
	{
		return;
	}

	total = CG_OSPEmojiSortedCount();
	totalRows = (total + BE_EMOJI_GRID_COLS - 1) / BE_EMOJI_GRID_COLS;
	beChatEmoji.cellW = beChatEmoji.btnW;
	beChatEmoji.cellH = beChatEmoji.btnH;
	beChatEmoji.gridW = beChatEmoji.cellW * (float)BE_EMOJI_GRID_COLS;

	panelRight = beChatEmoji.winX + beChatEmoji.winW - BECHAT_EMOJI_WIN_PAD;
	beChatEmoji.pickerSbX = panelRight - BECHAT_EMOJI_PANEL_PAD - BECHAT_SB_W;
	beChatEmoji.pickerSbW = BECHAT_SB_W;

	beChatEmoji.panelH = BECHAT_EMOJI_PANEL_PAD * 2.0f + beChatEmoji.cellH * (float)BE_EMOJI_GRID_ROWS;
	beChatEmoji.panelY = beChatEmoji.btnY - beChatEmoji.panelH;
	beChatEmoji.panelW = BECHAT_EMOJI_PANEL_PAD * 2.0f + beChatEmoji.gridW + BECHAT_SB_GAP + BECHAT_SB_W;
	beChatEmoji.panelX = panelRight - beChatEmoji.panelW;

	beChatEmoji.pickerSbY = beChatEmoji.panelY + BECHAT_EMOJI_PANEL_PAD;
	beChatEmoji.pickerSbH = beChatEmoji.panelH - BECHAT_EMOJI_PANEL_PAD * 2.0f;

	beChatEmoji.pickerScrollMax = totalRows - BE_EMOJI_GRID_ROWS;
	if (beChatEmoji.pickerScrollMax < 0)
	{
		beChatEmoji.pickerScrollMax = 0;
	}
	if (beChatEmoji.pickerScroll > beChatEmoji.pickerScrollMax)
	{
		beChatEmoji.pickerScroll = beChatEmoji.pickerScrollMax;
	}
	if (beChatEmoji.pickerScroll < 0)
	{
		beChatEmoji.pickerScroll = 0;
	}

	beChatEmoji.pickerSbActive = (beChatEmoji.pickerScrollMax > 0) ? qtrue : qfalse;
	contentH = (float)totalRows * beChatEmoji.cellH;
	scrollPx = (float)beChatEmoji.pickerScroll * beChatEmoji.cellH;
	CG_BEChat_EmojiPickerSbGetThumb(beChatEmoji.pickerSbH, contentH, scrollPx,
	                                &beChatEmoji.pickerSbThumbH, &beChatEmoji.pickerSbThumbY,
	                                &beChatEmoji.pickerSbMaxScrollPx);
}

qboolean CG_BEChat_EmojiPickerWheel(int wheelUp)
{
	if (!beChatEmoji.pickerOpen)
	{
		return qfalse;
	}

	CG_BEChat_EmojiLayoutPickerPanel();

	if (wheelUp)
	{
		beChatEmoji.pickerScroll--;
	}
	else
	{
		beChatEmoji.pickerScroll++;
	}

	if (beChatEmoji.pickerScroll < 0)
	{
		beChatEmoji.pickerScroll = 0;
	}
	if (beChatEmoji.pickerScroll > beChatEmoji.pickerScrollMax)
	{
		beChatEmoji.pickerScroll = beChatEmoji.pickerScrollMax;
	}

	return qtrue;
}

static int CG_BEChat_EmojiPickerCellAt(float virtX, float virtY)
{
	float cx;
	float cy;
	int col;
	int row;
	int i;

	if (!beChatEmoji.pickerOpen)
	{
		return -1;
	}

	CG_BEChat_EmojiLayoutPickerPanel();

	if (!CG_BEChat_EmojiPointInRect(virtX, virtY, beChatEmoji.panelX, beChatEmoji.panelY,
	                                beChatEmoji.panelW, beChatEmoji.panelH))
	{
		return -1;
	}

	cx = virtX - beChatEmoji.panelX - BECHAT_EMOJI_PANEL_PAD;
	cy = virtY - beChatEmoji.panelY - BECHAT_EMOJI_PANEL_PAD;
	if (cx < 0.0f || cy < 0.0f || cx >= beChatEmoji.gridW)
	{
		return -1;
	}

	col = (int)(cx / beChatEmoji.cellW);
	row = (int)(cy / beChatEmoji.cellH) + beChatEmoji.pickerScroll;
	if (col < 0 || col >= BE_EMOJI_GRID_COLS || row < 0)
	{
		return -1;
	}

	i = row * BE_EMOJI_GRID_COLS + col;
	if (i < 0 || i >= CG_OSPEmojiSortedCount())
	{
		return -1;
	}

	return i;
}

static float CG_BEChat_EmojiPickerTipAlpha(int* outShowIndex)
{
	int elapsed;
	float alpha;

	if (outShowIndex)
	{
		*outShowIndex = -1;
	}

	if (!beChatEmoji.pickerOpen)
	{
		return 0.0f;
	}

	if (beChatEmoji.pickerTipFadingOut)
	{
		elapsed = cg.time - beChatEmoji.pickerTipHideStartTime;
		if (elapsed < 0)
		{
			elapsed = 0;
		}
		if (elapsed >= BE_EMOJI_TIP_FADE_MS)
		{
			beChatEmoji.pickerTipFadingOut = qfalse;
			return 0.0f;
		}
		alpha = 1.0f - ((float)elapsed / (float)BE_EMOJI_TIP_FADE_MS);
		if (outShowIndex)
		{
			*outShowIndex = beChatEmoji.pickerTipShowIndex;
		}
		return alpha;
	}

	if (beChatEmoji.pickerHoverIndex < 0)
	{
		return 0.0f;
	}

	elapsed = cg.time - beChatEmoji.pickerHoverStartTime;
	if (elapsed < BE_EMOJI_TIP_DELAY_MS)
	{
		return 0.0f;
	}

	elapsed -= BE_EMOJI_TIP_DELAY_MS;
	if (elapsed >= BE_EMOJI_TIP_FADE_MS)
	{
		alpha = 1.0f;
	}
	else
	{
		alpha = (float)elapsed / (float)BE_EMOJI_TIP_FADE_MS;
	}

	if (outShowIndex)
	{
		*outShowIndex = beChatEmoji.pickerHoverIndex;
	}
	return alpha;
}

void CG_BEChat_EmojiPickerHoverFrame(void)
{
	int index;
	int showIndex;
	float virtX;
	float virtY;
	float alpha;
	float winX;
	float winY;
	float winW;
	float winH;

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_EmojiPrepareInputRow(winX, winY, winW, winH);
	CG_BEChat_VirtualCursorPos(&virtX, &virtY);
	beChatEmoji.btnHover = CG_BEChat_EmojiHitButton(virtX, virtY);

	if (!beChatEmoji.pickerOpen)
	{
		beChatEmoji.pickerHoverIndex = -1;
		beChatEmoji.pickerTipFadingOut = qfalse;
		return;
	}

	index = CG_BEChat_EmojiPickerCellAt(virtX, virtY);

	if (index != beChatEmoji.pickerHoverIndex)
	{
		if (beChatEmoji.pickerHoverIndex >= 0)
		{
			alpha = CG_BEChat_EmojiPickerTipAlpha(&showIndex);
			if (alpha > 0.0f && showIndex >= 0)
			{
				beChatEmoji.pickerTipShowIndex = showIndex;
				beChatEmoji.pickerTipHideStartTime = cg.time;
				beChatEmoji.pickerTipFadingOut = qtrue;
			}
			else
			{
				beChatEmoji.pickerTipFadingOut = qfalse;
			}
		}
		else
		{
			beChatEmoji.pickerTipFadingOut = qfalse;
		}

		beChatEmoji.pickerHoverIndex = index;
		if (index >= 0)
		{
			beChatEmoji.pickerHoverStartTime = cg.time;
		}
	}
}

static void CG_BEChat_EmojiDrawPickerTooltip(void)
{
	int showIndex;
	int col;
	int row;
	int srcRow;
	float alpha;
	float tipX;
	float tipY;
	float tipW;
	float tipH;
	float cellX;
	float cellY;
	const emoji_t* em;
	vec4_t tipBg;
	vec4_t tipColor;
	char label[MAX_NAME_LENGTH];

	if (!beChatEmoji.pickerOpen)
	{
		return;
	}

	alpha = CG_BEChat_EmojiPickerTipAlpha(&showIndex);
	if (alpha <= 0.0f || showIndex < 0)
	{
		return;
	}

	em = CG_OSPEmojiSortedAt(showIndex);
	if (!em)
	{
		return;
	}

	CG_BEChat_EmojiLayoutPickerPanel();
	col = showIndex % BE_EMOJI_GRID_COLS;
	row = showIndex / BE_EMOJI_GRID_COLS;
	srcRow = row - beChatEmoji.pickerScroll;
	if (srcRow < 0 || srcRow >= BE_EMOJI_GRID_ROWS)
	{
		return;
	}

	cellX = beChatEmoji.panelX + BECHAT_EMOJI_PANEL_PAD + (float)col * beChatEmoji.cellW;
	cellY = beChatEmoji.panelY + BECHAT_EMOJI_PANEL_PAD + (float)srcRow * beChatEmoji.cellH;

	Q_strncpyz(label, em->name, sizeof(label));
	tipW = CG_OSPDrawStringNewTextWidth(label, BECHAT_CHAR_W, BECHAT_CHAR_H,
	                                    BE_EMOJI_TIP_TEXT_FLAGS, BE_EMOJI_FONT);
	if (tipW < BECHAT_CHAR_W)
	{
		tipW = BECHAT_CHAR_W;
	}
	tipH = BECHAT_CHAR_H;
	tipX = cellX + beChatEmoji.cellW * 0.5f;
	tipY = cellY - tipH - BECHAT_EMOJI_TIP_PAD - 2.0f;
	if (tipY < beChatEmoji.panelY + 1.0f)
	{
		tipY = cellY + beChatEmoji.cellH + BECHAT_EMOJI_TIP_PAD;
	}

	tipBg[0] = 0.0f;
	tipBg[1] = 0.0f;
	tipBg[2] = 0.0f;
	tipBg[3] = 0.5f * alpha;
	tipColor[0] = 1.0f;
	tipColor[1] = 1.0f;
	tipColor[2] = 1.0f;
	tipColor[3] = alpha;

	CG_FillRect(tipX - tipW * 0.5f - BECHAT_EMOJI_TIP_PAD, tipY - BECHAT_EMOJI_TIP_PAD,
	            tipW + BECHAT_EMOJI_TIP_PAD * 2.0f, tipH + BECHAT_EMOJI_TIP_PAD * 2.0f, tipBg);
	CG_OSPDrawStringNew(tipX, tipY, label, tipColor, colorBlack,
	                    BECHAT_CHAR_W, BECHAT_CHAR_H,
	                    0, BE_EMOJI_TIP_TEXT_FLAGS, BE_EMOJI_FONT,
	                    NULL, NULL, NULL);
}

void CG_BEChat_EmojiPrepareInputRow(float winX, float winY, float winW, float winH)
{
	float inputY;

	beChatEmoji.winX = winX;
	beChatEmoji.winY = winY;
	beChatEmoji.winW = winW;
	beChatEmoji.winH = winH;

	beChatEmoji.btnW = BECHAT_EMOJI_BTN_SIZE;
	beChatEmoji.btnH = BECHAT_EMOJI_BTN_SIZE;
	inputY = CG_BEChat_InputRowY(winY, winH);
	beChatEmoji.btnX = winX + winW - BECHAT_EMOJI_WIN_PAD - beChatEmoji.btnW;
	if (cg_chatWindow.integer & 4)
	{
		beChatEmoji.btnY = CG_BEChat_InputRowBgY(winY, winH) +
		                   (BECHAT_INPUT_ROW_H - beChatEmoji.btnH) * 0.5f;
	}
	else
	{
		beChatEmoji.btnY = inputY + BECHAT_CHAR_H - 1.0f - beChatEmoji.btnH;
	}
	CG_BEChat_EmojiLayoutPickerPanel();
}

static qboolean CG_BEChat_EmojiPickerScrollbarMouseDown(float virtX, float virtY)
{
	float thumbTop;
	float trackRange;

	if (!beChatEmoji.pickerOpen || !beChatEmoji.pickerSbActive)
	{
		return qfalse;
	}

	CG_BEChat_EmojiLayoutPickerPanel();

	thumbTop = beChatEmoji.pickerSbY + beChatEmoji.pickerSbThumbY;

	if (virtX < beChatEmoji.pickerSbX || virtX > beChatEmoji.pickerSbX + beChatEmoji.pickerSbW)
	{
		return qfalse;
	}
	if (virtY < beChatEmoji.pickerSbY || virtY > beChatEmoji.pickerSbY + beChatEmoji.pickerSbH)
	{
		return qfalse;
	}

	if (virtY >= thumbTop - BECHAT_SB_HIT_PAD &&
	        virtY <= thumbTop + beChatEmoji.pickerSbThumbH + BECHAT_SB_HIT_PAD)
	{
		beChatEmoji.pickerSbDragging = qtrue;
		beChatEmoji.pickerSbGrabDeltaY = virtY - thumbTop;
		return qtrue;
	}

	trackRange = beChatEmoji.pickerSbH - beChatEmoji.pickerSbThumbH;
	if (trackRange < 1.0f || beChatEmoji.pickerSbMaxScrollPx < 1.0f)
	{
		return qtrue;
	}

	if (virtY < thumbTop)
	{
		beChatEmoji.pickerScroll--;
	}
	else
	{
		beChatEmoji.pickerScroll++;
	}

	if (beChatEmoji.pickerScroll < 0)
	{
		beChatEmoji.pickerScroll = 0;
	}
	if (beChatEmoji.pickerScroll > beChatEmoji.pickerScrollMax)
	{
		beChatEmoji.pickerScroll = beChatEmoji.pickerScrollMax;
	}

	return qtrue;
}

void CG_BEChat_EmojiPickerDragPoll(void)
{
	float trackRange;
	float thumbY;
	float scrollPx;
	float virtX;
	float virtY;

	if (!beChatEmoji.pickerSbDragging)
	{
		return;
	}

	if (!trap_Key_IsDown(K_MOUSE1))
	{
		beChatEmoji.pickerSbDragging = qfalse;
		return;
	}

	if (!beChatEmoji.pickerOpen || !beChatEmoji.pickerSbActive)
	{
		beChatEmoji.pickerSbDragging = qfalse;
		return;
	}

	CG_BEChat_EmojiLayoutPickerPanel();
	trackRange = beChatEmoji.pickerSbH - beChatEmoji.pickerSbThumbH;
	if (trackRange < 1.0f || beChatEmoji.pickerSbMaxScrollPx < 1.0f)
	{
		return;
	}

	CG_BEChat_VirtualCursorPos(&virtX, &virtY);

	thumbY = virtY - beChatEmoji.pickerSbGrabDeltaY - beChatEmoji.pickerSbY;
	if (thumbY < 0.0f)
	{
		thumbY = 0.0f;
	}
	if (thumbY > trackRange)
	{
		thumbY = trackRange;
	}

	scrollPx = (thumbY / trackRange) * beChatEmoji.pickerSbMaxScrollPx;
	beChatEmoji.pickerScroll = (int)(scrollPx / beChatEmoji.cellH + 0.5f);
	if (beChatEmoji.pickerScroll < 0)
	{
		beChatEmoji.pickerScroll = 0;
	}
	if (beChatEmoji.pickerScroll > beChatEmoji.pickerScrollMax)
	{
		beChatEmoji.pickerScroll = beChatEmoji.pickerScrollMax;
	}
}

qboolean CG_BEChat_EmojiMouseDown(float virtX, float virtY, beChatInput_t* edit, float maxTextW)
{
	int i;
	int col;
	int row;
	float cx;
	float cy;
	const emoji_t* em;

	if (!edit)
	{
		return qfalse;
	}

	if (CG_BEChat_EmojiHitButton(virtX, virtY))
	{
		beChatEmoji.btnFlashTime = cg.time;
		beChatEmoji.pickerOpen = !beChatEmoji.pickerOpen;
		if (beChatEmoji.pickerOpen)
		{
			beChatEmoji.pickerScroll = 0;
		}
		else
		{
			CG_BEChat_EmojiPickerHoverReset();
		}
		return qtrue;
	}

	if (!beChatEmoji.pickerOpen)
	{
		return qfalse;
	}

	CG_BEChat_EmojiLayoutPickerPanel();

	if (CG_BEChat_EmojiPickerScrollbarMouseDown(virtX, virtY))
	{
		return qtrue;
	}

	if (!CG_BEChat_EmojiPointInRect(virtX, virtY, beChatEmoji.panelX, beChatEmoji.panelY,
	                                beChatEmoji.panelW, beChatEmoji.panelH))
	{
		return qfalse;
	}

	cx = virtX - beChatEmoji.panelX - BECHAT_EMOJI_PANEL_PAD;
	cy = virtY - beChatEmoji.panelY - BECHAT_EMOJI_PANEL_PAD;
	if (cx < 0.0f || cy < 0.0f || cx >= beChatEmoji.gridW)
	{
		return qtrue;
	}

	col = (int)(cx / beChatEmoji.cellW);
	row = (int)(cy / beChatEmoji.cellH) + beChatEmoji.pickerScroll;
	if (col < 0 || col >= BE_EMOJI_GRID_COLS || row < 0)
	{
		return qtrue;
	}

	i = row * BE_EMOJI_GRID_COLS + col;
	if (i >= CG_OSPEmojiSortedCount())
	{
		return qtrue;
	}

	em = CG_OSPEmojiSortedAt(i);
	if (em)
	{
		beChatEmoji.cellFlashIndex = i;
		beChatEmoji.cellFlashTime = cg.time;
		CG_BEChat_EmojiInsertToken(edit, em->name, maxTextW);
		CG_BEChat_InputClampScroll(edit, maxTextW);
	}
	return qtrue;
}

static float CG_BEChat_EmojiAcTokenStartX(float inputX, beChatInput_t* edit, int textFlags)
{
	char before[MAX_STRING_CHARS];
	int prestep;
	int tokenPos;
	int len;
	float drawX;
	int textFlagsEmoji;

	if (!edit || beChatEmoji.acColonPos < 0)
	{
		return CG_BEChat_InputTextDrawX(inputX);
	}

	drawX = CG_BEChat_InputTextDrawX(inputX);
	prestep = edit->scroll;
	tokenPos = beChatEmoji.acColonPos - prestep;
	if (tokenPos <= 0)
	{
		return drawX;
	}

	len = tokenPos;
	if (len >= (int)sizeof(before))
	{
		len = (int)sizeof(before) - 1;
	}
	memcpy(before, edit->buffer + prestep, len);
	before[len] = '\0';

	textFlagsEmoji = CG_BEChat_EmojiTextFlags(textFlags);
	return drawX + CG_OSPDrawStringNewTextWidth(before, BECHAT_CHAR_W, BECHAT_CHAR_H,
	        textFlagsEmoji, BE_EMOJI_FONT);
}

static void CG_BEChat_EmojiDrawPickerScrollbar(void)
{
	vec4_t track;
	vec4_t border;
	vec4_t thumb;

	if (!beChatEmoji.pickerOpen || !beChatEmoji.pickerSbActive)
	{
		return;
	}

	track[0] = 0.25f;
	track[1] = 0.25f;
	track[2] = 0.25f;
	track[3] = 0.85f;
	CG_FillRect(beChatEmoji.pickerSbX, beChatEmoji.pickerSbY,
	            beChatEmoji.pickerSbW, beChatEmoji.pickerSbH, track);

	thumb[0] = 0.65f;
	thumb[1] = 0.65f;
	thumb[2] = 0.65f;
	thumb[3] = 1.0f;
	CG_FillRect(beChatEmoji.pickerSbX,
	            beChatEmoji.pickerSbY + beChatEmoji.pickerSbThumbY,
	            beChatEmoji.pickerSbW, beChatEmoji.pickerSbThumbH, thumb);

	border[0] = 0.45f;
	border[1] = 0.45f;
	border[2] = 0.45f;
	border[3] = 0.9f;
	CG_DrawRect(beChatEmoji.pickerSbX, beChatEmoji.pickerSbY,
	            beChatEmoji.pickerSbW, beChatEmoji.pickerSbH, BECHAT_SB_BORDER, border);
}

void CG_BEChat_EmojiDrawUI(float inputX, float inputY, float inputW, beChatInput_t* edit,
                           const vec4_t color, int textFlags)
{
	int i;
	int count;
	int cols;
	int rows;
	int col;
	int row;
	float btnX;
	float btnY;
	float btnW;
	float btnH;
	float labelX;
	float labelY;
	float labelW;
	char label[MAX_NAME_LENGTH];
	vec4_t border;
	vec4_t panelBg;
	vec4_t ghostColor;
	vec4_t ghostBg;
	const emoji_t* em;
	float winX;
	float winY;
	float winW;
	float winH;
	float btnFlashAlpha;

	if (!edit)
	{
		return;
	}

	CG_BEChat_CmdAcDraw(inputX, inputY, edit, color, textFlags);

	CG_BEChat_WindowLayout(&winX, &winY, &winW, &winH);
	CG_BEChat_EmojiPrepareInputRow(winX, winY, winW, winH);

	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = 1.0f;
	panelBg[0] = 0.05f;
	panelBg[1] = 0.05f;
	panelBg[2] = 0.05f;
	panelBg[3] = 0.92f;

	btnW = beChatEmoji.btnW;
	btnH = beChatEmoji.btnH;
	btnX = beChatEmoji.btnX;
	btnY = beChatEmoji.btnY;

	CG_FillRect(btnX, btnY, btnW, btnH, panelBg);
	btnFlashAlpha = CG_BEChat_EmojiPressFlashAlpha(beChatEmoji.btnFlashTime);
	if (btnFlashAlpha > 0.0f)
	{
		CG_BEChat_EmojiDrawHighlightRect(btnX, btnY, btnW, btnH, 0.0f, btnFlashAlpha);
	}
	else if (beChatEmoji.btnHover)
	{
		CG_BEChat_EmojiDrawHighlightRect(btnX, btnY, btnW, btnH, 1.0f, 0.0f);
	}
	CG_DrawRect(btnX, btnY, btnW, btnH, 1.0f, border);
	labelW = CG_OSPDrawStringNewTextWidth(":osp2be:", BECHAT_CHAR_W, BECHAT_CHAR_H,
	                                      BE_EMOJI_TEXT_FLAGS, BE_EMOJI_FONT);
	labelX = btnX + (btnW - labelW) * 0.5f;
	labelY = btnY + (btnH - BECHAT_CHAR_H) * 0.5f;
	CG_OSPDrawStringNew(labelX, labelY, ":osp2be:",
	                    color, colorBlack, BECHAT_CHAR_W, BECHAT_CHAR_H,
	                    btnW, BE_EMOJI_TEXT_FLAGS, BE_EMOJI_FONT,
	                    NULL, NULL, NULL);

	if (beChatEmoji.acCount > 0 && beChatEmoji.acColonPos >= 0 &&
	        beChatEmoji.acSel >= 0 && beChatEmoji.acSel < beChatEmoji.acCount &&
	        !CG_BEChat_CmdAcIsActive())
	{
		char plain[64];
		char fullPlain[64];
		int prefixLen;
		int ghostLen;
		char prefixBuf[MAX_STRING_CHARS];
		char ghostBuf[MAX_STRING_CHARS];
		int prestep;
		int prefixLenDraw;
		float drawX;
		float ghostX;
		float tokenX;
		float previewRowY;
		float previewW;
		float previewCharW;
		float previewCharH;
		qboolean drawGhost;
		int textFlagsEmoji;
		vec4_t previewBg;

		em = beChatEmoji.acList[beChatEmoji.acSel];
		if (em && em->name[0])
		{
			textFlagsEmoji = CG_BEChat_EmojiTextFlags(textFlags);
			previewCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
			previewCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;

			tokenX = CG_BEChat_EmojiAcTokenStartX(inputX, edit, textFlags);
			previewRowY = inputY - BECHAT_EMOJI_PREVIEW_GAP - previewCharH;
			previewW = CG_OSPDrawStringNewTextWidth(em->name, previewCharW, previewCharH,
			                                        textFlagsEmoji, BE_EMOJI_FONT);
			if (previewW < previewCharW)
			{
				previewW = previewCharW;
			}

			previewBg[0] = 1.0f;
			previewBg[1] = 1.0f;
			previewBg[2] = 1.0f;
			previewBg[3] = 0.2f;
			CG_FillRect(tokenX, previewRowY, previewW, previewCharH, previewBg);

			CG_OSPDrawStringNew(tokenX, previewRowY, em->name, color, colorBlack,
			                    previewCharW, previewCharH,
			                    0, textFlagsEmoji, BE_EMOJI_FONT,
			                    NULL, NULL, NULL);

			ghostBg[0] = 1.0f;
			ghostBg[1] = 1.0f;
			ghostBg[2] = 1.0f;
			ghostBg[3] = 0.2f;
			ghostColor[0] = 0.0f;
			ghostColor[1] = 0.0f;
			ghostColor[2] = 0.0f;
			ghostColor[3] = 0.55f;

			CG_BEChat_EmojiPlainName(em->name, fullPlain, sizeof(fullPlain));
			prefixLen = strlen(beChatEmoji.acPrefix);
			plain[0] = '\0';
			ghostLen = 0;
			drawGhost = qfalse;
			ghostBuf[0] = '\0';
			if (prefixLen < (int)strlen(fullPlain))
			{
				Q_strncpyz(plain, fullPlain + prefixLen, sizeof(plain));
				Com_sprintf(ghostBuf, sizeof(ghostBuf), "%s:", plain);
				ghostLen = strlen(ghostBuf);
				drawGhost = qtrue;
			}
			else if (prefixLen == (int)strlen(fullPlain))
			{
				Q_strncpyz(ghostBuf, ":", sizeof(ghostBuf));
				ghostLen = 1;
				drawGhost = qtrue;
			}

			if (drawGhost && ghostLen > 0)
			{
				prestep = edit->scroll;
				prefixLenDraw = edit->cursor - prestep;
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

				drawX = CG_BEChat_InputTextDrawX(inputX);
				ghostX = drawX + CG_OSPDrawStringNewTextWidth(prefixBuf, BECHAT_CHAR_W, BECHAT_CHAR_H,
				        textFlagsEmoji, BE_EMOJI_FONT);
				CG_OSPDrawStringNew(ghostX, inputY, ghostBuf, ghostColor, colorBlack,
				                    BECHAT_CHAR_W, BECHAT_CHAR_H,
				                    0, textFlagsEmoji, BE_EMOJI_FONT,
				                    ghostBg, NULL, NULL);
			}
		}
	}

	if (!beChatEmoji.pickerOpen)
	{
		return;
	}

	count = CG_OSPEmojiSortedCount();
	CG_BEChat_EmojiLayoutPickerPanel();

	cols = BE_EMOJI_GRID_COLS;
	rows = BE_EMOJI_GRID_ROWS;

	{
		float pickerCharW;
		float pickerCharH;
		float tokenW;
		float drawCellX;
		float drawCellY;

		pickerCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
		pickerCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;

	CG_FillRect(beChatEmoji.panelX, beChatEmoji.panelY, beChatEmoji.panelW, beChatEmoji.panelH, panelBg);
	CG_DrawRect(beChatEmoji.panelX, beChatEmoji.panelY, beChatEmoji.panelW, beChatEmoji.panelH, 1.0f, border);

	for (row = 0; row < rows; row++)
	{
		int srcRow;
		float cellFlashAlpha;

		srcRow = beChatEmoji.pickerScroll + row;
		for (col = 0; col < cols; col++)
		{
			float ix;
			float iy;

			i = srcRow * cols + col;
			if (i < 0 || i >= count)
			{
				continue;
			}

			ix = beChatEmoji.panelX + BECHAT_EMOJI_PANEL_PAD + (float)col * beChatEmoji.cellW;
			iy = beChatEmoji.panelY + BECHAT_EMOJI_PANEL_PAD + (float)row * beChatEmoji.cellH;

			cellFlashAlpha = 0.0f;
			if (i == beChatEmoji.cellFlashIndex)
			{
				cellFlashAlpha = CG_BEChat_EmojiPressFlashAlpha(beChatEmoji.cellFlashTime);
			}
			if (cellFlashAlpha > 0.0f)
			{
				CG_BEChat_EmojiDrawHighlightRect(ix, iy, beChatEmoji.cellW, beChatEmoji.cellH,
				                                 0.0f, cellFlashAlpha);
			}
			else if (i == beChatEmoji.pickerHoverIndex)
			{
				CG_BEChat_EmojiDrawHighlightRect(ix, iy, beChatEmoji.cellW, beChatEmoji.cellH,
				                                 1.0f, 0.0f);
			}

			em = CG_OSPEmojiSortedAt(i);
			if (!em)
			{
				continue;
			}

			Q_strncpyz(label, em->name, sizeof(label));
			tokenW = CG_OSPDrawStringNewTextWidth(label, pickerCharW, pickerCharH,
			                                      BE_EMOJI_TEXT_FLAGS, BE_EMOJI_FONT);
			if (tokenW < pickerCharW)
			{
				tokenW = pickerCharW;
			}
			if (tokenW > beChatEmoji.cellW)
			{
				tokenW = beChatEmoji.cellW;
			}
			drawCellX = ix + (beChatEmoji.cellW - tokenW) * 0.5f;
			drawCellY = iy + (beChatEmoji.cellH - pickerCharH) * 0.5f;
			CG_OSPDrawStringNew(drawCellX, drawCellY, label, color, colorBlack,
			                    pickerCharW, pickerCharH,
			                    beChatEmoji.cellW, BE_EMOJI_TEXT_FLAGS, BE_EMOJI_FONT,
			                    NULL, NULL, NULL);
		}
	}

	CG_BEChat_EmojiDrawPickerScrollbar();
	CG_BEChat_EmojiDrawPickerTooltip();
	}
}

