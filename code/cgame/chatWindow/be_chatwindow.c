/*
===========================================================================
be_chatwindow.c

Enterchat: open/close, input keys, mouse, draw, send.
See be_chatwindow.h and sibling be_chatwindow_*.c modules.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

extern qboolean cg_keyCapsLockOnAvail;

static void CG_BEChat_BeginRepeat(int key);

qboolean CG_BEChat_ConsoleBlocksInput(void)
{
	if (trap_Key_GetCatcher() & KEYCATCH_CONSOLE)
	{
		return qtrue;
	}

	return qfalse;
}
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
	beChatWindow.altDownSince = 0;
	beChatWindow.altLastTrapTime = 0;
	beChatWindow.altPrevDown = qfalse;
	beChatWindow.ctrlFavHintShown = qfalse;
	beChatWindow.ctrlDownSince = 0;
	beChatWindow.ctrlLastTrapTime = 0;
	beChatWindow.ctrlPrevDown = qfalse;
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
	beChatWindow.altFavUsed = qfalse;
	beChatWindow.altPrevDown = trap_Key_IsDown(K_ALT) ? qtrue : qfalse;
	beChatWindow.altDownSince = beChatWindow.altPrevDown ? cg.time : 0;
	beChatWindow.altLastTrapTime = beChatWindow.altPrevDown ? cg.time : 0;
	beChatWindow.ctrlPrevDown = trap_Key_IsDown(K_CTRL) ? qtrue : qfalse;
	beChatWindow.ctrlDownSince = beChatWindow.ctrlPrevDown ? cg.time : 0;
	beChatWindow.ctrlLastTrapTime = beChatWindow.ctrlPrevDown ? cg.time : 0;
	beChatWindow.ctrlFavHintShown = qfalse;
	beChatWindow.phantomKey0Seq = 0;
	memset(beChatWindow.keyDown, 0, sizeof(beChatWindow.keyDown));
	beChatWindow.keyDown[K_ALT] = beChatWindow.altPrevDown;
	beChatWindow.keyDown[K_CTRL] = beChatWindow.ctrlPrevDown;
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
char CG_BEChat_LineDefaultColor(const beChatHistoryLine_t* line)
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
qboolean CG_BEChat_IsWordBoundaryChar(char c)
{
	if (c == ' ' || c == '\t')
	{
		return qtrue;
	}

	return qfalse;
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
#if BE_CHAT_KEY_DEBUG
static const char* CG_BEChat_KeyDebugName(int key)
{
	switch (key)
	{
	case K_TAB:
		return "TAB";
	case K_ENTER:
		return "ENTER";
	case K_ESCAPE:
		return "ESCAPE";
	case K_SPACE:
		return "SPACE";
	case K_BACKSPACE:
		return "BACKSPACE";
	case K_ALT:
		return "ALT";
	case K_CTRL:
		return "CTRL";
	case K_SHIFT:
		return "SHIFT";
	case K_UPARROW:
		return "UP";
	case K_DOWNARROW:
		return "DOWN";
	case K_LEFTARROW:
		return "LEFT";
	case K_RIGHTARROW:
		return "RIGHT";
	case K_INS:
		return "INS";
	case K_DEL:
		return "DEL";
	case K_PGDN:
		return "PGDN";
	case K_PGUP:
		return "PGUP";
	case K_HOME:
		return "HOME";
	case K_END:
		return "END";
	case K_KP_HOME:
		return "KP_HOME";
	case K_KP_UPARROW:
		return "KP_UP";
	case K_KP_PGUP:
		return "KP_PGUP";
	case K_KP_LEFTARROW:
		return "KP_LEFT";
	case K_KP_5:
		return "KP_5";
	case K_KP_RIGHTARROW:
		return "KP_RIGHT";
	case K_KP_END:
		return "KP_END";
	case K_KP_DOWNARROW:
		return "KP_DOWN";
	case K_KP_PGDN:
		return "KP_PGDN";
	case K_KP_ENTER:
		return "KP_ENTER";
	case K_KP_INS:
		return "KP_INS";
	case K_KP_DEL:
		return "KP_DEL";
	default:
		return NULL;
	}
}
static void CG_BEChat_KeyDebugEvent(int key, qboolean down)
{
	int k;
	const char* name;
	char chBuf[8];

	k = key & ~K_CHAR_FLAG;
	name = CG_BEChat_KeyDebugName(k);
	if (key & K_CHAR_FLAG)
	{
		if (k >= 32 && k < 127)
		{
			chBuf[0] = (char)k;
			chBuf[1] = '\0';
		}
		else
		{
			chBuf[0] = '\0';
		}
		CG_Printf("BEChat KeyEvent: CHAR raw=%i k=%i ch='%s'(%i) down=%i alt=%i\n",
		          key, k, chBuf, k, down, trap_Key_IsDown(K_ALT) ? 1 : 0);
		return;
	}
	if (name)
	{
		CG_Printf("BEChat KeyEvent: %s(%i) down=%i alt=%i ctrl=%i\n",
		          name, k, down, trap_Key_IsDown(K_ALT) ? 1 : 0,
		          trap_Key_IsDown(K_CTRL) ? 1 : 0);
		return;
	}
	if (k >= 32 && k < 127)
	{
		CG_Printf("BEChat KeyEvent: '%c'(%i) down=%i alt=%i\n",
		          (char)k, k, down, trap_Key_IsDown(K_ALT) ? 1 : 0);
		return;
	}
	if (k >= 0 && k < 256)
	{
		CG_Printf("BEChat KeyEvent: code=%i down=%i alt=%i\n",
		          k, down, trap_Key_IsDown(K_ALT) ? 1 : 0);
	}
}
static void CG_BEChat_KeyDebugEdge(int key)
{
	const char* name;
	qboolean altTrap;
	qboolean altLatch;

	altTrap = trap_Key_IsDown(K_ALT) ? qtrue : qfalse;
	altLatch = (beChatWindow.latchCloseKey == K_ALT) ? qtrue : qfalse;
	name = CG_BEChat_KeyDebugName(key);
	if (name)
	{
		CG_Printf("BEChat KeyEdge: %s(%i) altTrap=%i altLatch=%i keyDownAlt=%i\n",
		          name, key, altTrap, altLatch, beChatWindow.keyDown[K_ALT] ? 1 : 0);
	}
	else if (key >= 32 && key < 127)
	{
		CG_Printf("BEChat KeyEdge: '%c'(%i) altTrap=%i altLatch=%i\n",
		          (char)key, key, altTrap, altLatch);
	}
	else
	{
		CG_Printf("BEChat KeyEdge: code=%i altTrap=%i altLatch=%i\n",
		          key, altTrap, altLatch);
	}
}
#endif
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
#if BE_CHAT_KEY_DEBUG
	if (edge)
	{
		CG_BEChat_KeyDebugEdge(key);
	}
#endif
	return edge;
}
static qboolean CG_BEChat_FavEmojiSlotFromDigit(char digit, int* outSlot)
{
	if (!outSlot || !digit)
	{
		return qfalse;
	}

	if (digit >= '1' && digit <= '9')
	{
		*outSlot = digit - '1';
		return qtrue;
	}
	if (digit == '0')
	{
		*outSlot = 9;
		return qtrue;
	}

	return qfalse;
}
qboolean CG_BEChat_AltIsHeld(void)
{
	if (beChatWindow.latchCloseKey == K_CTRL)
	{
		return qfalse;
	}

	if (trap_Key_IsDown(K_CTRL))
	{
		return qtrue;
	}

	if (beChatWindow.keyDown[K_CTRL] &&
	        cg.time - beChatWindow.ctrlLastTrapTime < BE_CHAT_CTRL_TRAP_GRACE_MS)
	{
		return qtrue;
	}

	return qfalse;
}
qboolean CG_BEChat_AltFavHintActive(void)
{
	if (cg_chatWindow.integer & BE_CHAT_NO_ALT_FAV_HINT_BIT)
	{
		return qfalse;
	}

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	if (beChatWindow.ctrlDownSince <= 0)
	{
		return qfalse;
	}

	return qtrue;
}
void CG_BEChat_AltSuppressPickerOnRelease(void)
{
	if (!beChatWindow.open)
	{
		return;
	}

	beChatWindow.altFavUsed = qtrue;
}
static qboolean CG_BEChat_FavEmojiResolveSlot(int key, char ch, int* outSlot)
{
	if (key >= '0' && key <= '9')
	{
		return CG_BEChat_FavEmojiSlotFromDigit((char)key, outSlot);
	}

	if (CG_BEChat_FavEmojiSlotFromDigit(ch, outSlot))
	{
		return qtrue;
	}

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	switch (ch)
	{
	case '!':
		return CG_BEChat_FavEmojiSlotFromDigit('1', outSlot);
	case '@':
		return CG_BEChat_FavEmojiSlotFromDigit('2', outSlot);
	case '#':
		return CG_BEChat_FavEmojiSlotFromDigit('3', outSlot);
	case '$':
		return CG_BEChat_FavEmojiSlotFromDigit('4', outSlot);
	case '%':
		return CG_BEChat_FavEmojiSlotFromDigit('5', outSlot);
	case '^':
		return CG_BEChat_FavEmojiSlotFromDigit('6', outSlot);
	case '&':
		return CG_BEChat_FavEmojiSlotFromDigit('7', outSlot);
	case '*':
		return CG_BEChat_FavEmojiSlotFromDigit('8', outSlot);
	case '(':
		return CG_BEChat_FavEmojiSlotFromDigit('9', outSlot);
	case ')':
		return CG_BEChat_FavEmojiSlotFromDigit('0', outSlot);
	default:
		return qfalse;
	}
}
static qboolean CG_BEChat_ApplyFavEmojiSlot(int slot, float maxTextW)
{
	beChatWindow.sentHistBrowse = -1;
	CG_BEChat_EmojiInsertFav(&beChatWindow.input, slot, maxTextW);
	CG_BEChat_InputClampScroll(&beChatWindow.input, maxTextW);
	return qtrue;
}
static qboolean CG_BEChat_FavEmojiKeyToDigit(int key, char* outDigit)
{
	if (key >= '0' && key <= '9')
	{
		if (outDigit)
		{
			*outDigit = (char)key;
		}
		return qtrue;
	}

	switch (key)
	{
	case K_END:
	case K_KP_END:
		if (outDigit)
		{
			*outDigit = '1';
		}
		return qtrue;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		if (outDigit)
		{
			*outDigit = '2';
		}
		return qtrue;
	case K_PGDN:
	case K_KP_PGDN:
		if (outDigit)
		{
			*outDigit = '3';
		}
		return qtrue;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		if (outDigit)
		{
			*outDigit = '4';
		}
		return qtrue;
	case K_KP_5:
		if (outDigit)
		{
			*outDigit = '5';
		}
		return qtrue;
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		if (outDigit)
		{
			*outDigit = '6';
		}
		return qtrue;
	case K_HOME:
	case K_KP_HOME:
		if (outDigit)
		{
			*outDigit = '7';
		}
		return qtrue;
	case K_UPARROW:
	case K_KP_UPARROW:
		if (outDigit)
		{
			*outDigit = '8';
		}
		return qtrue;
	case K_PGUP:
	case K_KP_PGUP:
		if (outDigit)
		{
			*outDigit = '9';
		}
		return qtrue;
	case K_INS:
	case K_KP_INS:
		if (outDigit)
		{
			*outDigit = '0';
		}
		return qtrue;
	default:
		return qfalse;
	}
}
static qboolean CG_BEChat_FavEmojiScrollArrowKey(int key)
{
	return (key == K_UPARROW || key == K_DOWNARROW ||
	        key == K_KP_UPARROW || key == K_KP_DOWNARROW);
}
static qboolean CG_BEChat_TryFavEmojiKeyOnEdge(int key, char digit, float maxTextW)
{
	int slot;

	if (trap_Key_IsDown(K_CTRL) && CG_BEChat_FavEmojiScrollArrowKey(key))
	{
		return qfalse;
	}

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	if (!CG_BEChat_FavEmojiSlotFromDigit(digit, &slot))
	{
		return qfalse;
	}

	CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
	CG_BEChat_BeginRepeat(key);
	return qtrue;
}
static qboolean CG_BEChat_TryFavEmojiKey(int key, char digit, float maxTextW)
{
	int slot;

	if (trap_Key_IsDown(K_CTRL) && CG_BEChat_FavEmojiScrollArrowKey(key))
	{
		return qfalse;
	}

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	if (!CG_BEChat_FavEmojiSlotFromDigit(digit, &slot))
	{
		return qfalse;
	}

	if (!CG_BEChat_KeyEdge(key))
	{
		return qtrue;
	}

	CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
	CG_BEChat_BeginRepeat(key);
	return qtrue;
}
static qboolean CG_BEChat_HandlePhantomKey0Fav(float maxTextW)
{
	static const int probeKeys[4] =
	{
		K_KP_DOWNARROW, K_DOWNARROW, K_KP_UPARROW, K_UPARROW
	};
	static const char probeDigits[4] = { '2', '2', '8', '8' };
	int i;
	int slot;
	char digit;

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	for (i = 0; i < 4; i++)
	{
		if (trap_Key_IsDown(probeKeys[i]) && !beChatWindow.keyDown[probeKeys[i]])
		{
			if (!CG_BEChat_FavEmojiSlotFromDigit(probeDigits[i], &slot))
			{
				return qfalse;
			}
			beChatWindow.keyDown[probeKeys[i]] = qtrue;
			CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
			CG_BEChat_BeginRepeat(probeKeys[i]);
			return qtrue;
		}
	}

	digit = (beChatWindow.phantomKey0Seq & 1) ? '8' : '2';
	beChatWindow.phantomKey0Seq++;
	if (!CG_BEChat_FavEmojiSlotFromDigit(digit, &slot))
	{
		return qfalse;
	}

	CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
	return qtrue;
}
static qboolean CG_BEChat_TryFavEmojiFromProcessKey(int key, char ch, float maxTextW)
{
	int slot;

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	if (!CG_BEChat_FavEmojiResolveSlot(key, ch, &slot))
	{
		return qfalse;
	}

	CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
	CG_BEChat_BeginRepeat(key);
	return qtrue;
}
static void CG_BEChat_AltSessionEnd(void)
{
	beChatWindow.keyDown[K_ALT] = qfalse;
	beChatWindow.altDownSince = 0;
	beChatWindow.altLastTrapTime = 0;
}
static void CG_BEChat_CtrlFavSessionEnd(void)
{
	beChatWindow.keyDown[K_CTRL] = qfalse;
	beChatWindow.ctrlFavHintShown = qfalse;
	beChatWindow.ctrlDownSince = 0;
	beChatWindow.ctrlLastTrapTime = 0;
}
static void CG_BEChat_PollAltPicker(void)
{
	qboolean altDown;

	if (beChatWindow.latchCloseKey == K_ALT)
	{
		beChatWindow.altPrevDown = trap_Key_IsDown(K_ALT) ? qtrue : qfalse;
		beChatWindow.keyDown[K_ALT] = beChatWindow.altPrevDown;
		return;
	}

	if (trap_Key_IsDown(K_ALT))
	{
		beChatWindow.altLastTrapTime = cg.time;
		beChatWindow.keyDown[K_ALT] = qtrue;
		if (beChatWindow.altDownSince <= 0)
		{
			beChatWindow.altDownSince = cg.time;
		}
	}

	altDown = trap_Key_IsDown(K_ALT) ? qtrue : qfalse;
	if (!altDown && beChatWindow.keyDown[K_ALT] &&
	        cg.time - beChatWindow.altLastTrapTime < BE_CHAT_ALT_TRAP_GRACE_MS)
	{
		altDown = qtrue;
	}

	if (altDown)
	{
		if (!beChatWindow.altPrevDown && beChatWindow.altDownSince <= 0)
		{
			beChatWindow.altDownSince = cg.time;
		}
	}
	else if (beChatWindow.altPrevDown)
	{
		if (!beChatWindow.altFavUsed)
		{
			CG_BEChat_EmojiTogglePicker();
		}
		beChatWindow.altFavUsed = qfalse;
		CG_BEChat_AltSessionEnd();
	}
	beChatWindow.altPrevDown = altDown;
}
static void CG_BEChat_PollCtrlFav(void)
{
	qboolean ctrlDown;

	if (beChatWindow.latchCloseKey == K_CTRL)
	{
		beChatWindow.ctrlPrevDown = trap_Key_IsDown(K_CTRL) ? qtrue : qfalse;
		beChatWindow.keyDown[K_CTRL] = beChatWindow.ctrlPrevDown;
		return;
	}

	if (trap_Key_IsDown(K_CTRL))
	{
		beChatWindow.ctrlLastTrapTime = cg.time;
		beChatWindow.keyDown[K_CTRL] = qtrue;
		if (beChatWindow.ctrlDownSince <= 0)
		{
			beChatWindow.ctrlDownSince = cg.time;
		}
	}

	ctrlDown = CG_BEChat_AltIsHeld();
	if (ctrlDown)
	{
		if (!beChatWindow.ctrlPrevDown && beChatWindow.ctrlDownSince <= 0)
		{
			beChatWindow.ctrlDownSince = cg.time;
		}
	}
	else if (beChatWindow.ctrlPrevDown)
	{
		CG_BEChat_CtrlFavSessionEnd();
	}
	beChatWindow.ctrlPrevDown = ctrlDown;
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

	if (CG_BEChat_TryFavEmojiKey(key, digit, maxTextW))
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
static qboolean CG_BEChat_RepeatFavEmojiIfAlt(int key, float maxTextW)
{
	char digit;
	char ch;
	int slot;

	if (!CG_BEChat_AltIsHeld())
	{
		return qfalse;
	}

	if (trap_Key_IsDown(K_CTRL) && CG_BEChat_FavEmojiScrollArrowKey(key))
	{
		return qfalse;
	}

	if (CG_BEChat_FavEmojiKeyToDigit(key, &digit))
	{
		ch = digit;
	}
	else if (key >= 32 && key < 127)
	{
		ch = (char)CG_BEChat_ApplyInputKeyChar(key);
	}
	else
	{
		return qfalse;
	}

	if (!CG_BEChat_FavEmojiResolveSlot(key, ch, &slot))
	{
		return qfalse;
	}

	CG_BEChat_ApplyFavEmojiSlot(slot, maxTextW);
	return qtrue;
}
static void CG_BEChat_RepeatKeyAction(int key, float maxTextW)
{
	int ch;

	if (CG_BEChat_RepeatFavEmojiIfAlt(key, maxTextW))
	{
		return;
	}

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

	if (CG_BEChat_FavEmojiKeyToDigit(key, NULL) && !CG_BEChat_AltIsHeld())
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
		CG_BEChat_EmojiAcCompleteForSend(&beChatWindow.input, maxTextW);
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
		{
			int favCh;

			favCh = CG_BEChat_ApplyInputKeyChar(keycode);
			if (CG_BEChat_TryFavEmojiFromProcessKey(keycode, (char)favCh, maxTextW))
			{
				return;
			}
			CG_BEChat_InputChar(&beChatWindow.input, favCh, maxTextW);
		}
		return;
	}

	if (keycode >= 32 && keycode < 127)
	{
		ch = keycode;
		if (CG_BEChat_TryFavEmojiFromProcessKey(keycode, (char)ch, maxTextW))
		{
			return;
		}
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

	CG_BEChat_PollAltPicker();
	CG_BEChat_PollCtrlFav();

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
		if (beChatWindow.keyDown[K_ALT] &&
		        cg.time - beChatWindow.altLastTrapTime < BE_CHAT_ALT_TRAP_GRACE_MS)
		{
			beChatWindow.keyDown[K_ALT] = qtrue;
		}
		if (beChatWindow.keyDown[K_CTRL] &&
		        cg.time - beChatWindow.ctrlLastTrapTime < BE_CHAT_CTRL_TRAP_GRACE_MS)
		{
			beChatWindow.keyDown[K_CTRL] = qtrue;
		}
		beChatWindow.altPrevDown = trap_Key_IsDown(K_ALT) ? qtrue : qfalse;
		if (beChatWindow.altPrevDown && !beChatWindow.altDownSince)
		{
			beChatWindow.altDownSince = cg.time;
		}
		beChatWindow.ctrlPrevDown = trap_Key_IsDown(K_CTRL) ? qtrue : qfalse;
		if (beChatWindow.ctrlPrevDown && !beChatWindow.ctrlDownSince)
		{
			beChatWindow.ctrlDownSince = cg.time;
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
		CG_BEChat_EmojiAcCompleteForSend(&beChatWindow.input, maxTextW);
		CG_BEChat_SendInput();
	}

	if (CG_BEChat_KeyEdge(K_PGUP))
	{
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_PGUP, '9', maxTextW))
		{
			CG_BEChat_Scroll(1);
			CG_BEChat_BeginRepeat(K_PGUP);
		}
	}

	CG_BEChat_KpDigitKey(K_KP_PGUP, '9', maxTextW);

	if (CG_BEChat_KeyEdge(K_PGDN))
	{
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_PGDN, '3', maxTextW))
		{
			CG_BEChat_Scroll(-1);
			CG_BEChat_BeginRepeat(K_PGDN);
		}
	}

	CG_BEChat_KpDigitKey(K_KP_PGDN, '3', maxTextW);

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

	if (CG_BEChat_KeyEdge(K_UPARROW) && !trap_Key_IsDown(K_CTRL))
	{
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_UPARROW, '8', maxTextW))
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
	}

	if (CG_BEChat_KeyEdge(K_DOWNARROW) && !trap_Key_IsDown(K_CTRL))
	{
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_DOWNARROW, '2', maxTextW))
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
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_KP_RIGHTARROW, '6', maxTextW))
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
				CG_BEChat_InputChar(&beChatWindow.input, '6', maxTextW);
				CG_BEChat_BeginRepeat(K_KP_RIGHTARROW);
			}
		}
	}
	if (CG_BEChat_KeyEdge(K_RIGHTARROW))
	{
		if (!CG_BEChat_TryFavEmojiKeyOnEdge(K_RIGHTARROW, '6', maxTextW))
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

#if BE_CHAT_KEY_DEBUG
	CG_BEChat_KeyDebugEvent(key, down);
#endif

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
		if (k == K_ALT)
		{
			if (beChatWindow.latchCloseKey != K_ALT)
			{
				beChatWindow.keyDown[K_ALT] = qtrue;
				beChatWindow.altLastTrapTime = cg.time;
				if (beChatWindow.altDownSince <= 0)
				{
					beChatWindow.altDownSince = cg.time;
				}
			}
			return;
		}

		if (k == K_CTRL)
		{
			if (beChatWindow.latchCloseKey != K_CTRL)
			{
				beChatWindow.keyDown[K_CTRL] = qtrue;
				beChatWindow.ctrlLastTrapTime = cg.time;
				if (beChatWindow.ctrlDownSince <= 0)
				{
					beChatWindow.ctrlDownSince = cg.time;
				}
			}
			return;
		}

		if (k == 0)
		{
			if (cg.time >= beChatWindow.inputGraceUntil)
			{
				float maxTextW;

				maxTextW = CG_BEChat_InputFieldMaxWidth();
				if (CG_BEChat_HandlePhantomKey0Fav(maxTextW))
				{
					beChatWindow.keyDown[0] = qtrue;
					return;
				}
			}
		}

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
		if (k == K_ALT)
		{
			return;
		}
		if (k == K_CTRL)
		{
			return;
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
