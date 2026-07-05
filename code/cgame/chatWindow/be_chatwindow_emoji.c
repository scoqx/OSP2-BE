/*
===========================================================================
cg_be_chatwindow_emoji.c — Emoji picker, autocomplete, cursor units.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow_emoji.h"
#include "../../ui/keycodes.h"

beChatEmojiUI_t beChatEmoji;

void CG_BEChat_EmojiPlainName(const char* token, char* out, int outSize)
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
	const emoji_t* prevSelEm;
	qboolean wasBrowse;

	prevSelEm = NULL;
	wasBrowse = beChatEmoji.acBrowseMode;
	if (wasBrowse && beChatEmoji.acCount > 0 &&
	        beChatEmoji.acSel >= 0 && beChatEmoji.acSel < beChatEmoji.acCount)
	{
		prevSelEm = beChatEmoji.acList[beChatEmoji.acSel];
	}

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
			beChatEmoji.acBrowseMode = qfalse;
			return;
		}
	}

	for (i = edit->cursor - 1; i >= 0; i--)
	{
		char ch;

		ch = edit->buffer[i];
		if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t')
		{
			beChatEmoji.acBrowseMode = qfalse;
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
			beChatEmoji.acBrowseMode = qfalse;
			return;
		}
	}

	if (beChatEmoji.acColonPos < 0)
	{
		beChatEmoji.acBrowseMode = qfalse;
		return;
	}

	for (i = beChatEmoji.acColonPos + 1; i < edit->cursor; i++)
	{
		if (edit->buffer[i] == EMOJI_CHAR)
		{
			beChatEmoji.acCount = 0;
			beChatEmoji.acColonPos = -1;
			beChatEmoji.acPrefix[0] = '\0';
			beChatEmoji.acBrowseMode = qfalse;
			return;
		}
	}

	if (CG_OSPGetEmoji(edit->buffer + beChatEmoji.acColonPos))
	{
		beChatEmoji.acBrowseMode = qfalse;
		return;
	}

	prefixLen = edit->cursor - (beChatEmoji.acColonPos + 1);
	if (prefixLen < 0)
	{
		beChatEmoji.acBrowseMode = qfalse;
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
	if (beChatEmoji.acCount < 1)
	{
		beChatEmoji.acBrowseMode = qfalse;
		beChatEmoji.acSel = 0;
		return;
	}

	if (wasBrowse && prevSelEm)
	{
		for (i = 0; i < beChatEmoji.acCount; i++)
		{
			if (beChatEmoji.acList[i] == prevSelEm)
			{
				beChatEmoji.acSel = i;
				return;
			}
		}
		for (i = 0; i < beChatEmoji.acCount; i++)
		{
			if (beChatEmoji.acList[i] &&
			        !Q_stricmp(beChatEmoji.acList[i]->name, prevSelEm->name))
			{
				beChatEmoji.acSel = i;
				return;
			}
		}
	}

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
	beChatEmoji.acBrowseMode = qfalse;
	CG_BEChat_EmojiRefreshAc(edit);
	CG_BEChat_InputClampScroll(edit, maxTextW);
}
qboolean CG_BEChat_EmojiAcCompleteForSend(beChatInput_t* edit, float maxTextW)
{
	if (!beChatEmoji.acBrowseMode || beChatEmoji.acCount < 1)
	{
		return qfalse;
	}

	CG_BEChat_EmojiAcComplete(edit, maxTextW);
	beChatEmoji.acBrowseMode = qfalse;
	return qtrue;
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

	if (keycode == K_SPACE && beChatEmoji.acBrowseMode && beChatEmoji.acCount > 0)
	{
		CG_BEChat_EmojiAcComplete(edit, maxTextW);
		beChatEmoji.acBrowseMode = qfalse;
		return qfalse;
	}

	if (beChatEmoji.acCount > 0 && !trap_Key_IsDown(K_CTRL))
	{
		if (keycode == K_UPARROW || keycode == K_KP_UPARROW)
		{
			beChatEmoji.acBrowseMode = qtrue;
			beChatEmoji.acSel--;
			if (beChatEmoji.acSel < 0)
			{
				beChatEmoji.acSel = beChatEmoji.acCount - 1;
			}
			return qtrue;
		}
		if (keycode == K_DOWNARROW || keycode == K_KP_DOWNARROW)
		{
			beChatEmoji.acBrowseMode = qtrue;
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
qboolean CG_BEChat_EmojiInsertFav(beChatInput_t* edit, int slotIndex, float maxTextW)
{
	char token[MAX_STRING_CHARS];
	const emoji_t* em;

	if (!edit || slotIndex < 0 || slotIndex >= BE_EMOJI_FAV_MAX)
	{
		return qfalse;
	}

	if (!CG_BEChat_EmojiFavTokenAt(slotIndex, token, sizeof(token)))
	{
		return qfalse;
	}

	em = CG_OSPGetEmoji(token);
	if (!em && token[0] != EMOJI_CHAR)
	{
		char wrapped[MAX_STRING_CHARS];

		Com_sprintf(wrapped, sizeof(wrapped), ":%s:", token);
		em = CG_OSPGetEmoji(wrapped);
	}
	if (!em || !em->name[0])
	{
		return qfalse;
	}

	CG_BEChat_EmojiInsertToken(edit, em->name, maxTextW);
	return qtrue;
}
