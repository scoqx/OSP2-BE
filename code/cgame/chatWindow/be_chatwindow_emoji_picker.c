/*
===========================================================================
cg_be_chatwindow_emoji_picker.c — Emoji picker panel UI and mouse.
===========================================================================
*/

#include "../cg_local.h"
#include "be_chatwindow_emoji.h"
#include "../../ui/keycodes.h"

static qboolean CG_BEChat_EmojiPickerLocatePickIndex(int pickIndex, int* outVisualRow, int* outCol)
{
	int row;
	int col;
	int emojiCols;

	if (pickIndex < 0)
	{
		return qfalse;
	}

	for (row = 0; row < CG_OSPEmojiPickerVisualRowCount(); ++row)
	{
		if (CG_OSPEmojiPickerRowIsHeader(row))
		{
			continue;
		}

		emojiCols = CG_OSPEmojiPickerRowEmojiCount(row);
		for (col = 0; col < emojiCols; ++col)
		{
			if (CG_OSPEmojiPickerPickIndexAt(row, col) == pickIndex)
			{
				if (outVisualRow)
				{
					*outVisualRow = row;
				}
				if (outCol)
				{
					*outCol = col;
				}
				return qtrue;
			}
		}
	}

	return qfalse;
}
void CG_BEChat_EmojiPickerHoverReset(void)
{
	beChatEmoji.pickerHoverIndex = -1;
	beChatEmoji.pickerTipShowIndex = -1;
	beChatEmoji.pickerTipFadingOut = qfalse;
	beChatEmoji.btnHover = qfalse;
	beChatEmoji.favHintCount = 0;
	beChatEmoji.favHintHoverIndex = -1;
	beChatEmoji.favHintFadingOut = qfalse;
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
qboolean CG_BEChat_EmojiFavTokenAt(int index, char* out, int outSize)
{
	const char* s;
	const char* start;
	int count;
	int len;

	if (index < 0 || index >= BE_EMOJI_FAV_MAX || !out || outSize < 1)
	{
		return qfalse;
	}

	s = cg_chatWindowFavEmojis.string;
	if (!s || !s[0])
	{
		return qfalse;
	}

	count = 0;
	while (*s)
	{
		while (*s == ' ' || *s == '\t')
		{
			s++;
		}
		if (!*s)
		{
			break;
		}
		start = s;
		while (*s && *s != ' ' && *s != '\t')
		{
			s++;
		}
		if (count == index)
		{
			len = (int)(s - start);
			if (len >= outSize)
			{
				len = outSize - 1;
			}
			memcpy(out, start, len);
			out[len] = '\0';
			return qtrue;
		}
		count++;
		if (count >= BE_EMOJI_FAV_MAX)
		{
			break;
		}
	}

	return qfalse;
}

static const emoji_t* CG_BEChat_EmojiFavResolved(int slotIndex)
{
	char token[MAX_STRING_CHARS];
	const emoji_t* em;

	if (!CG_BEChat_EmojiFavTokenAt(slotIndex, token, sizeof(token)))
	{
		return NULL;
	}

	em = CG_OSPGetEmoji(token);
	if (!em && token[0] != EMOJI_CHAR)
	{
		char wrapped[MAX_STRING_CHARS];

		Com_sprintf(wrapped, sizeof(wrapped), ":%s:", token);
		em = CG_OSPGetEmoji(wrapped);
	}
	return em;
}

static int CG_BEChat_EmojiFavCount(void)
{
	char token[4];
	int i;

	for (i = 0; i < BE_EMOJI_FAV_MAX; i++)
	{
		if (!CG_BEChat_EmojiFavTokenAt(i, token, sizeof(token)))
		{
			break;
		}
	}
	return i;
}
static float CG_BEChat_EmojiFavHintStartX(float inputX, int textFlags)
{
	float promptW;

	promptW = CG_OSPDrawStringNewTextWidth(CG_BEChat_InputPromptText(),
	        BECHAT_CHAR_W, BECHAT_CHAR_H,
	        textFlags, BE_EMOJI_FONT);
	return inputX + promptW;
}
static float CG_BEChat_AltFavHintAlpha(void)
{
	int elapsed;
	float alpha;

	if (cg_chatWindow.integer & BE_CHAT_NO_ALT_FAV_HINT_BIT)
	{
		return 0.0f;
	}

	if (beChatEmoji.favHintFadingOut)
	{
		elapsed = cg.time - beChatEmoji.favHintHideStartTime;
		if (elapsed < 0)
		{
			elapsed = 0;
		}
		if (elapsed >= BE_EMOJI_TIP_FADE_MS)
		{
			beChatEmoji.favHintFadingOut = qfalse;
			beChatWindow.ctrlFavHintShown = qfalse;
			return 0.0f;
		}
		alpha = 1.0f - ((float)elapsed / (float)BE_EMOJI_TIP_FADE_MS);
		return alpha;
	}

	if (!CG_BEChat_AltIsHeld())
	{
		return 0.0f;
	}

	if (beChatWindow.ctrlDownSince <= 0)
	{
		return 0.0f;
	}

	elapsed = cg.time - beChatWindow.ctrlDownSince;
	if (elapsed < 0)
	{
		elapsed = 0;
	}

	beChatWindow.ctrlFavHintShown = qtrue;
	if (elapsed >= BE_EMOJI_TIP_FADE_MS)
	{
		return 1.0f;
	}

	return (float)elapsed / (float)BE_EMOJI_TIP_FADE_MS;
}
static void CG_BEChat_AltFavHintFadePoll(void)
{
	if (CG_BEChat_AltIsHeld())
	{
		beChatEmoji.favHintFadingOut = qfalse;
		return;
	}

	if (beChatEmoji.favHintFadingOut)
	{
		return;
	}

	if (beChatWindow.ctrlFavHintShown)
	{
		beChatEmoji.favHintHideStartTime = cg.time;
		beChatEmoji.favHintFadingOut = qtrue;
	}
}
static float CG_BEChat_EmojiFavHintUpdateLayout(float inputX, float inputY, int textFlags);
static float CG_BEChat_EmojiDrawFavAltHint(float inputX, float inputY, const vec4_t color, int textFlags)
{
	char digitStr[2];
	const emoji_t* em;
	int i;
	float barY;
	float barH;
	float barW;
	float x;
	float emW;
	float digitW;
	float digitH;
	float previewCharW;
	float previewCharH;
	float digitY;
	float digitDrawY;
	float cellH;
	float favHintLift;
	float cellFlashAlpha;
	float favAlpha;
	vec4_t bg;
	vec4_t digitColor;
	vec4_t emojiColor;
	int textFlagsEmoji;

	favHintLift = CG_BEChat_EmojiFavHintUpdateLayout(inputX, inputY, textFlags);
	if (favHintLift <= 0.0f || beChatEmoji.favHintCount < 1)
	{
		return 0.0f;
	}

	favAlpha = CG_BEChat_AltFavHintAlpha();
	if (favAlpha <= 0.0f)
	{
		return 0.0f;
	}

	textFlagsEmoji = CG_BEChat_EmojiTextFlags(textFlags);
	previewCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
	previewCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;
	digitW = BECHAT_CHAR_W;
	digitH = BECHAT_CHAR_H;
	barH = previewCharH;
	barY = inputY - BECHAT_EMOJI_PREVIEW_GAP - previewCharH;
	digitY = barY - 1.0f;
	cellH = (barY + barH) - digitY;
	digitDrawY = digitY + cellH * 0.5f;

	barW = 0.0f;
	for (i = 0; i < beChatEmoji.favHintCount; i++)
	{
		barW += beChatEmoji.favHintCellW[i];
		if (i + 1 < beChatEmoji.favHintCount)
		{
			barW += BE_EMOJI_FAV_HINT_CELL_GAP;
		}
	}

	bg[0] = 0.0f;
	bg[1] = 0.0f;
	bg[2] = 0.0f;
	bg[3] = cg_chatWindowBgOpaque.value;
	if (bg[3] <= 0.0f)
	{
		bg[3] = 0.85f;
	}
	bg[3] *= favAlpha;
	CG_FillRect(beChatEmoji.favHintCellX[0], barY, barW, barH, bg);

	for (i = 0; i < beChatEmoji.favHintCount; i++)
	{
		cellFlashAlpha = 0.0f;
		if (i == beChatEmoji.favHintFlashIndex)
		{
			cellFlashAlpha = CG_BEChat_EmojiPressFlashAlpha(beChatEmoji.favHintFlashTime);
		}
		if (cellFlashAlpha > 0.0f)
		{
			CG_BEChat_EmojiDrawHighlightRect(beChatEmoji.favHintCellX[i],
			        beChatEmoji.favHintCellY[i],
			        beChatEmoji.favHintCellW[i], beChatEmoji.favHintCellH[i],
			        0.0f, cellFlashAlpha);
		}
		else if (i == beChatEmoji.favHintHoverIndex)
		{
			CG_BEChat_EmojiDrawHighlightRect(beChatEmoji.favHintCellX[i],
			        beChatEmoji.favHintCellY[i],
			        beChatEmoji.favHintCellW[i], beChatEmoji.favHintCellH[i],
			        favAlpha, 0.0f);
		}
	}

	Vector4Copy(colorWhite, digitColor);
	Vector4Copy(colorWhite, emojiColor);
	digitColor[3] = favAlpha;
	emojiColor[3] = favAlpha;

	for (i = 0; i < beChatEmoji.favHintCount; i++)
	{
		float digitRight;
		float digitTextW;

		em = CG_BEChat_EmojiFavResolved(i);
		digitStr[0] = (i < 9) ? (char)('1' + i) : '0';
		digitStr[1] = '\0';

		digitTextW = beChatEmoji.favHintCellW[i] - BE_EMOJI_FAV_HINT_DIGIT_EM_GAP;
		if (em && em->name[0])
		{
			emW = CG_OSPDrawStringNewTextWidth(em->name, previewCharW, previewCharH,
			        textFlagsEmoji, BE_EMOJI_FONT);
			if (emW < previewCharW)
			{
				emW = previewCharW;
			}
			digitTextW -= emW;
		}
		else
		{
			digitTextW -= previewCharW;
		}
		if (digitTextW < digitW * 0.5f)
		{
			digitTextW = digitW * 0.5f;
		}

		x = beChatEmoji.favHintCellX[i];
		digitRight = x + digitTextW;
		CG_OSPDrawStringNew(digitRight, digitDrawY, digitStr, digitColor, colorBlack,
		                    digitW, digitH, 0,
		                    textFlagsEmoji | DS_HRIGHT | DS_VCENTER, BE_EMOJI_FONT,
		                    NULL, NULL, NULL);

		x = digitRight + BE_EMOJI_FAV_HINT_DIGIT_EM_GAP;
		if (em && em->name[0])
		{
			CG_OSPDrawStringNew(x, barY, em->name, emojiColor, colorBlack,
			                    previewCharW, previewCharH, 0,
			                    textFlagsEmoji, BE_EMOJI_FONT,
			                    NULL, NULL, NULL);
		}
	}

	(void)color;
	return favHintLift;
}
static qboolean CG_BEChat_EmojiPointInRect(float x, float y, float rx, float ry, float rw, float rh)
{
	return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}
static int CG_BEChat_EmojiFavHintCellAt(float virtX, float virtY)
{
	int i;

	for (i = 0; i < beChatEmoji.favHintCount; i++)
	{
		if (CG_BEChat_EmojiPointInRect(virtX, virtY,
		        beChatEmoji.favHintCellX[i], beChatEmoji.favHintCellY[i],
		        beChatEmoji.favHintCellW[i], beChatEmoji.favHintCellH[i]))
		{
			return i;
		}
	}

	return -1;
}
static float CG_BEChat_EmojiFavHintUpdateLayout(float inputX, float inputY, int textFlags)
{
	char digitStr[2];
	const emoji_t* em;
	int i;
	int count;
	float drawX;
	float barY;
	float barH;
	float x;
	float emW;
	float digitW;
	float digitH;
	float previewCharW;
	float previewCharH;
	float digitY;
	float cellH;
	int textFlagsEmoji;

	beChatEmoji.favHintCount = 0;

	if (!CG_BEChat_AltFavHintActive() && !beChatEmoji.favHintFadingOut &&
	        !beChatWindow.ctrlFavHintShown)
	{
		return 0.0f;
	}

	count = CG_BEChat_EmojiFavCount();
	if (count < 1)
	{
		return 0.0f;
	}

	textFlagsEmoji = CG_BEChat_EmojiTextFlags(textFlags);
	previewCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
	previewCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;
	digitW = BECHAT_CHAR_W;
	digitH = BECHAT_CHAR_H;
	barH = previewCharH;
	barY = inputY - BECHAT_EMOJI_PREVIEW_GAP - previewCharH;
	digitY = barY - 1.0f;
	cellH = (barY + barH) - digitY;

	drawX = CG_BEChat_EmojiFavHintStartX(inputX, textFlags);
	x = drawX;
	for (i = 0; i < count; i++)
	{
		float digitTextW;
		float cellW;

		em = CG_BEChat_EmojiFavResolved(i);
		digitStr[0] = (i < 9) ? (char)('1' + i) : '0';
		digitStr[1] = '\0';
		digitTextW = CG_OSPDrawStringNewTextWidth(digitStr, digitW, digitH,
		        textFlagsEmoji, BE_EMOJI_FONT);
		if (digitTextW < 1.0f)
		{
			digitTextW = digitW * 0.5f;
		}

		emW = previewCharW;
		if (em && em->name[0])
		{
			emW = CG_OSPDrawStringNewTextWidth(em->name, previewCharW, previewCharH,
			        textFlagsEmoji, BE_EMOJI_FONT);
			if (emW < previewCharW)
			{
				emW = previewCharW;
			}
		}

		cellW = digitTextW + BE_EMOJI_FAV_HINT_DIGIT_EM_GAP + emW;
		beChatEmoji.favHintCellX[i] = x;
		beChatEmoji.favHintCellY[i] = digitY;
		beChatEmoji.favHintCellW[i] = cellW;
		beChatEmoji.favHintCellH[i] = cellH;
		x += cellW;
		if (i + 1 < count)
		{
			x += BE_EMOJI_FAV_HINT_CELL_GAP;
		}
	}

	beChatEmoji.favHintCount = count;
	return barH + BECHAT_EMOJI_PREVIEW_GAP;
}
static qboolean CG_BEChat_EmojiFavHintMouseDown(float virtX, float virtY,
        beChatInput_t* edit, float maxTextW)
{
	int slot;

	if (!edit || beChatEmoji.favHintCount < 1)
	{
		return qfalse;
	}

	slot = CG_BEChat_EmojiFavHintCellAt(virtX, virtY);
	if (slot < 0)
	{
		return qfalse;
	}

	beChatWindow.sentHistBrowse = -1;
	CG_BEChat_EmojiInsertFav(edit, slot, maxTextW);
	CG_BEChat_AltSuppressPickerOnRelease();
	CG_BEChat_InputClampScroll(edit, maxTextW);
	return qtrue;
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

	total = CG_OSPEmojiPickerVisualRowCount();
	totalRows = total;
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
	int step;

	if (!beChatEmoji.pickerOpen)
	{
		return qfalse;
	}

	CG_BEChat_EmojiLayoutPickerPanel();

	step = trap_Key_IsDown(K_CTRL) ? BE_CHAT_SCROLL_CTRL_STEP : 1;

	if (wheelUp)
	{
		beChatEmoji.pickerScroll -= step;
	}
	else
	{
		beChatEmoji.pickerScroll += step;
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

	if (row >= CG_OSPEmojiPickerVisualRowCount())
	{
		return -1;
	}

	if (CG_OSPEmojiPickerRowIsHeader(row))
	{
		return -1;
	}

	if (col >= CG_OSPEmojiPickerRowEmojiCount(row))
	{
		return -1;
	}

	return CG_OSPEmojiPickerPickIndexAt(row, col);
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

	CG_BEChat_AltFavHintFadePoll();
	if (CG_BEChat_AltFavHintAlpha() > 0.0f || beChatEmoji.favHintFadingOut)
	{
		CG_BEChat_EmojiFavHintUpdateLayout(beChatEmoji.inputContentX,
		                                   beChatEmoji.inputContentY, BE_CHAT_TEXT_FLAGS);
		if (CG_BEChat_AltIsHeld())
		{
			beChatEmoji.favHintHoverIndex = CG_BEChat_EmojiFavHintCellAt(virtX, virtY);
		}
		else
		{
			beChatEmoji.favHintHoverIndex = -1;
		}
	}
	else
	{
		beChatEmoji.favHintCount = 0;
		beChatEmoji.favHintHoverIndex = -1;
	}

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

	em = CG_OSPEmojiPickerAtPickIndex(showIndex);
	if (!em)
	{
		return;
	}

	CG_BEChat_EmojiLayoutPickerPanel();
	if (!CG_BEChat_EmojiPickerLocatePickIndex(showIndex, &row, &col))
	{
		return;
	}
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
	beChatEmoji.inputContentX = winX + BECHAT_PAD;
	beChatEmoji.inputContentY = inputY;
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

	if (CG_BEChat_EmojiFavHintMouseDown(virtX, virtY, edit, maxTextW))
	{
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

	if (row >= CG_OSPEmojiPickerVisualRowCount() || CG_OSPEmojiPickerRowIsHeader(row))
	{
		return qtrue;
	}

	if (col >= CG_OSPEmojiPickerRowEmojiCount(row))
	{
		return qtrue;
	}

	i = CG_OSPEmojiPickerPickIndexAt(row, col);
	if (i < 0)
	{
		return qtrue;
	}

	em = CG_OSPEmojiPickerAtPickIndex(i);
	if (em)
	{
		beChatEmoji.cellFlashIndex = i;
		beChatEmoji.cellFlashTime = cg.time;
		CG_BEChat_EmojiInsertToken(edit, em->name, maxTextW);
		CG_BEChat_InputClampScroll(edit, maxTextW);
		if (CG_BEChat_AltFavHintActive())
		{
			CG_BEChat_AltSuppressPickerOnRelease();
		}
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
	float iconX;
	float iconY;
	float iconSize;
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
	float favHintLift;

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
	iconSize = btnW - 4.0f;
	if (iconSize > btnH - 4.0f)
	{
		iconSize = btnH - 4.0f;
	}
	iconX = btnX + (btnW - iconSize) * 0.5f;
	iconY = btnY + (btnH - iconSize) * 0.5f;
	{
		const emoji_t* iconEm;

		iconEm = CG_OSPGetEmoji(":osp2be:");
		if (iconEm && iconEm->shader)
		{
			trap_R_SetColor(colorWhite);
			CG_DrawPicOld(iconX, iconY, iconSize, iconSize, iconEm->shader);
			trap_R_SetColor(NULL);
		}
	}

	favHintLift = CG_BEChat_EmojiDrawFavAltHint(inputX, inputY, color, textFlags);

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
		vec4_t acDrawColor;
		char tokenBuf[MAX_STRING_CHARS];
		int tokenLen;

		em = beChatEmoji.acList[beChatEmoji.acSel];
		if (em && em->name[0])
		{
			textFlagsEmoji = CG_BEChat_EmojiTextFlags(textFlags);
			if (beChatEmoji.acBrowseMode)
			{
				Vector4Copy(colorWhite, acDrawColor);
			}
			else
			{
				Vector4Copy(color, acDrawColor);
			}
			previewCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
			previewCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;

			tokenX = CG_BEChat_EmojiAcTokenStartX(inputX, edit, textFlags);
			previewRowY = inputY - BECHAT_EMOJI_PREVIEW_GAP - previewCharH - favHintLift;
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

			CG_OSPDrawStringNew(tokenX, previewRowY, em->name, acDrawColor, colorBlack,
			                    previewCharW, previewCharH,
			                    0, textFlagsEmoji, BE_EMOJI_FONT,
			                    NULL, NULL, NULL);

			ghostBg[0] = 1.0f;
			ghostBg[1] = 1.0f;
			ghostBg[2] = 1.0f;
			ghostBg[3] = 0.2f;
			if (beChatEmoji.acBrowseMode)
			{
				Vector4Copy(colorWhite, ghostColor);
			}
			else
			{
				ghostColor[0] = 0.0f;
				ghostColor[1] = 0.0f;
				ghostColor[2] = 0.0f;
				ghostColor[3] = 0.55f;
			}

			if (beChatEmoji.acBrowseMode && beChatEmoji.acColonPos >= 0 &&
			        edit->cursor > beChatEmoji.acColonPos)
			{
				tokenLen = edit->cursor - beChatEmoji.acColonPos;
				if (tokenLen >= (int)sizeof(tokenBuf))
				{
					tokenLen = (int)sizeof(tokenBuf) - 1;
				}
				memcpy(tokenBuf, edit->buffer + beChatEmoji.acColonPos, tokenLen);
				tokenBuf[tokenLen] = '\0';
				CG_OSPDrawStringNew(tokenX, inputY, tokenBuf, acDrawColor, colorBlack,
				                    BECHAT_CHAR_W, BECHAT_CHAR_H,
				                    0, textFlagsEmoji, BE_EMOJI_FONT,
				                    ghostBg, NULL, NULL);
			}

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

	count = CG_OSPEmojiPickerVisualRowCount();
	CG_BEChat_EmojiLayoutPickerPanel();

	cols = BE_EMOJI_GRID_COLS;
	rows = BE_EMOJI_GRID_ROWS;

	{
		float pickerCharW;
		float pickerCharH;
		float headerCharW;
		float headerCharH;
		float tokenW;
		float drawCellX;
		float drawCellY;
		const char* headerTitle;
		int srcRow;
		int emojiCols;

		pickerCharW = BECHAT_CHAR_W * BE_EMOJI_PREVIEW_SCALE;
		pickerCharH = BECHAT_CHAR_H * BE_EMOJI_PREVIEW_SCALE;
		headerCharW = BECHAT_CHAR_W * BE_EMOJI_HEADER_TEXT_SCALE;
		headerCharH = BECHAT_CHAR_H * BE_EMOJI_HEADER_TEXT_SCALE;

	CG_FillRect(beChatEmoji.panelX, beChatEmoji.panelY, beChatEmoji.panelW, beChatEmoji.panelH, panelBg);
	CG_DrawRect(beChatEmoji.panelX, beChatEmoji.panelY, beChatEmoji.panelW, beChatEmoji.panelH, 1.0f, border);

	for (row = 0; row < rows; row++)
	{
		float cellFlashAlpha;
		vec4_t headerColor;

		srcRow = beChatEmoji.pickerScroll + row;
		if (srcRow < 0 || srcRow >= count)
		{
			continue;
		}

		if (CG_OSPEmojiPickerRowIsHeader(srcRow))
		{
			vec4_t headerLine;

			headerTitle = CG_OSPEmojiPickerRowHeaderTitle(srcRow);
			drawCellY = beChatEmoji.panelY + BECHAT_EMOJI_PANEL_PAD + (float)row * beChatEmoji.cellH;
			drawCellX = beChatEmoji.panelX + BECHAT_EMOJI_PANEL_PAD;
			headerLine[0] = 0.35f;
			headerLine[1] = 0.35f;
			headerLine[2] = 0.35f;
			headerLine[3] = 0.55f;
			CG_FillRect(drawCellX, drawCellY + beChatEmoji.cellH - 1.0f,
			            beChatEmoji.gridW, 1.0f, headerLine);
			headerColor[0] = 0.92f;
			headerColor[1] = 0.92f;
			headerColor[2] = 0.92f;
			headerColor[3] = color[3];
			drawCellY += (beChatEmoji.cellH - headerCharH) * 0.5f;
			CG_OSPDrawStringNew(drawCellX, drawCellY, headerTitle, headerColor, colorBlack,
			                    headerCharW, headerCharH,
			                    (int)beChatEmoji.gridW, DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL,
			                    BE_EMOJI_FONT,
			                    NULL, NULL, NULL);
			continue;
		}

		emojiCols = CG_OSPEmojiPickerRowEmojiCount(srcRow);
		for (col = 0; col < cols; col++)
		{
			float ix;
			float iy;

			if (col >= emojiCols)
			{
				continue;
			}

			i = CG_OSPEmojiPickerPickIndexAt(srcRow, col);
			if (i < 0)
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

			em = CG_OSPEmojiPickerEmojiAt(srcRow, col);
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
