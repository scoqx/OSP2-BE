/*
===========================================================================
cg_be_chatwindow_input.c — Text input field, sent-history navigation.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

float CG_BEChat_InputRowY(float winY, float winH)
{
	return winY + winH - BECHAT_INPUT_ROW_H + BECHAT_INPUT_HISTORY_GAP;
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
beChatSentHistEntry_t* CG_BEChat_SentHistEntryNewest(int indexFromNewest)
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
void CG_BEChat_SentHistPush(const char* text, int destMode, int tellClientNum, const char* tellName)
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
qboolean CG_BEChat_SentHistCanNavigate(void)
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
qboolean CG_BEChat_SentHistProcessKey(int key, float maxTextW)
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
void CG_BEChat_InputClear(beChatInput_t* edit)
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
float CG_BEChat_InputBufSliceDisplayWidth(const char* buf, int start, int end,
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
int CG_BEChat_InputVisibleBytes(const char* text, int start, float maxWidth)
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
void CG_BEChat_InputKeyDown(beChatInput_t* edit, int key, float maxTextW)
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
void CG_BEChat_InputChar(beChatInput_t* edit, int ch, float maxTextW)
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
