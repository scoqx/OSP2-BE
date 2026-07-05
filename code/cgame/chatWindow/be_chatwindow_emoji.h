#ifndef CG_BE_CHATWINDOW_EMOJI_H
#define CG_BE_CHATWINDOW_EMOJI_H

#include "be_chatwindow.h"

#define BE_EMOJI_FONT           cgFontIndices.chatWindow
#define BE_EMOJI_TEXT_FLAGS     (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL | DS_EMOJI)
#define BE_EMOJI_GRID_COLS      8
#define BE_EMOJI_GRID_ROWS      6
#define BE_EMOJI_AC_MAX         32
#define BE_EMOJI_FAV_MAX        10
#define BE_EMOJI_PREVIEW_SCALE  1.2f
#define BE_EMOJI_TIP_DELAY_MS   800
#define BE_EMOJI_TIP_FADE_MS    100
#define BE_EMOJI_TIP_TEXT_FLAGS (DS_HCENTER | DS_SHADOW | DS_PROPORTIONAL)
#define BE_EMOJI_PRESS_FADE_MS  220
#define BE_EMOJI_HOVER_GRAY     0.42f
#define BE_EMOJI_HOVER_GRAY_A   0.45f
#define BE_EMOJI_HEADER_TEXT_SCALE 0.8f
#define BE_EMOJI_FAV_HINT_PAD          0.0f
#define BE_EMOJI_FAV_HINT_CELL_GAP     3.0f
#define BE_EMOJI_FAV_HINT_DIGIT_EM_GAP 2.0f

typedef struct
{
	qboolean    pickerOpen;
	float       winX;
	float       winY;
	float       winW;
	float       winH;
	float       inputContentX;
	float       inputContentY;
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
	int         favHintCount;
	float       favHintCellX[BE_EMOJI_FAV_MAX];
	float       favHintCellY[BE_EMOJI_FAV_MAX];
	float       favHintCellW[BE_EMOJI_FAV_MAX];
	float       favHintCellH[BE_EMOJI_FAV_MAX];
	int         favHintHoverIndex;
	int         favHintFlashIndex;
	int         favHintFlashTime;
	qboolean    favHintFadingOut;
	int         favHintHideStartTime;
	int         acColonPos;
	char        acPrefix[64];
	int         acCount;
	int         acSel;
	qboolean    acBrowseMode;
	const emoji_t* acList[BE_EMOJI_AC_MAX];
} beChatEmojiUI_t;

extern beChatEmojiUI_t beChatEmoji;

void CG_BEChat_EmojiPickerHoverReset(void);
qboolean CG_BEChat_EmojiFavTokenAt(int index, char* out, int outSize);
void CG_BEChat_EmojiPlainName(const char* token, char* out, int outSize);

#endif /* CG_BE_CHATWINDOW_EMOJI_H */
