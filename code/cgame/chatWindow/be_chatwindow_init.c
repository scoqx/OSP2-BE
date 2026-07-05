/* cg_be_chatwindow_init.c — metrics, layout, startup restore */

#include "../cg_local.h"
#include "be_chatwindow.h"
#include "../../ui/keycodes.h"

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
float CG_BEChat_InputRowY(float winY, float winH)
{
	return winY + winH - BECHAT_INPUT_ROW_H + BECHAT_INPUT_HISTORY_GAP;
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
	CG_BEChatHistory_LoadPersist();
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