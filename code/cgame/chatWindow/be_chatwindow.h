#ifndef CG_BE_CHATWINDOW_H
#define CG_BE_CHATWINDOW_H

#include "cg_local.h"

#define BE_CHAT_FONT_W_BASE     6.0f
#define BE_CHAT_FONT_H_BASE     10.0f
#define BE_CHAT_INPUT_CHARS     48
#define BE_CHAT_FONT            cgFontIndices.chatWindow
#define BE_CHAT_TEXT_FLAGS      (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL)
#define BE_CHAT_KEEP_OPEN_BIT   2
#define BE_CHAT_INPUT_ONLY_BG_BIT 4
#define BE_CHAT_NO_REPEAT_MERGE_BIT 8
#define BE_CHAT_NO_ALT_FAV_HINT_BIT 16
#define BECHAT_INPUT_HISTORY_GAP  2.0f
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
#define BE_CHAT_ALT_TRAP_GRACE_MS       50
#define BE_CHAT_CTRL_TRAP_GRACE_MS      50
#define BE_CHAT_SCROLL_CTRL_STEP    4
#define BE_CHAT_FLAT_ROWS_MAX       512

#define BE_CHAT_FADE_MAX_MS         800
#define BE_CHAT_FADE_TIME_PERCENT   15
#define BE_CHAT_PERSIST_SLOTS       16
#define BE_CHAT_PERSIST_ENTRY_MAX   (BE_CHAT_LINE_CHARS + 64)
#define BE_CHAT_PERSIST_CVAR_MAX    (MAX_CVAR_VALUE_STRING - 8)
#define BE_CHAT_SENT_HIST_SLOTS     16
#define BE_CHAT_SENT_PERSIST_ENTRY_MAX (MAX_SAY_TEXT + MAX_NAME_LENGTH + 64)

/* Set to 1 to print enterchat mouse debug to console */
#define BE_CHAT_MOUSE_DEBUG     0
#define BE_CHAT_KEY_DEBUG       0

#define BE_CHAT_HELP_ALPHA          0.65f
#define BE_CHAT_HELP_TIP_DELAY_MS   800
#define BE_CHAT_HELP_TIP_FADE_MS    100
#define BE_CHAT_HELP_TIP_LINE_COUNT 7
#define BE_CHAT_HELP_TIP_BG_ALPHA     0.8f
#define BE_CHAT_HELP_QMARK_FLAGS    (DS_HCENTER | DS_VCENTER | DS_PROPORTIONAL)
#define BE_CHAT_HELP_TIP_FLAGS      (DS_HLEFT | DS_SHADOW | DS_PROPORTIONAL)
#define BE_CHAT_CMD_AC_PREVIEW_SCALE  1.2f

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
	qboolean    altFavUsed; /* ALT hold: emoji picked — skip picker toggle on release */
	qboolean    altPrevDown; /* previous frame ALT state for release edge */
	int         altDownSince; /* cg.time when current ALT hold began; 0 if up */
	int         altLastTrapTime; /* last cg.time trap saw ALT down */
	qboolean    ctrlPrevDown; /* previous frame Ctrl state for fav hint bar */
	qboolean    ctrlFavHintShown; /* fav bar fully visible this Ctrl hold (no re-fade) */
	int         ctrlDownSince; /* cg.time when current Ctrl hold began; 0 if up */
	int         ctrlLastTrapTime; /* last cg.time trap saw Ctrl down */
	int         phantomKey0Seq; /* engine keycode 0: alternate numpad 2 / 8 */
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


typedef struct
{
	int         mode;
	int         count;
	int         sel;
	int         replaceFrom;
	int         replaceTo;
	char        entries[BE_CHAT_CMD_AC_MAX][MAX_NAME_LENGTH];
} beChatCmdAc_t;

extern beChatWindowState_t beChatWindow;
extern beChatCmdAc_t beChatCmdAc;
extern char beChatPromptText[64];
extern beChatSentHistEntry_t beChatSentHist[BE_CHAT_SENT_HIST_SLOTS];
extern int beChatSentHistWrite;
extern int beChatSentHistCount;
extern qboolean beChatMouse1Prev;
extern qboolean beChatHistoryRestoreActive;
extern int beChatRowsCacheWriteIdx;
extern int beChatRowsCacheCount;
extern float beChatRowsCacheWidth;
extern qboolean beChatRowsCacheTimeFilter;
extern int beChatRowsCacheFrameTime;
extern int beChatRowsCacheTotal;
extern int beChatRowsCacheRows[BE_CHAT_HISTORY_LINES];
extern int beChatFlatCount;
extern qboolean beChatFlatFull;
extern char beChatFlatText[BE_CHAT_FLAT_ROWS_MAX][BE_CHAT_LINE_CHARS];
extern int beChatFlatLinesBack[BE_CHAT_FLAT_ROWS_MAX];
extern int beChatWinLayoutFrame;
extern float beChatWinLayoutX;
extern float beChatWinLayoutY;
extern float beChatWinLayoutW;
extern float beChatWinLayoutH;
extern const char* beChatHelpTipLines[BE_CHAT_HELP_TIP_LINE_COUNT];

qboolean CG_BEChat_ConsoleBlocksInput(void);
void CG_BEChat_CmdAcClear(void);
void CG_BEChat_CmdAcRefresh(beChatInput_t* edit);
void CG_BEChat_CmdAcComplete(beChatInput_t* edit, float maxTextW);
void CG_BEChat_ResetDestMode(void);
void CG_BEChat_InputClear(beChatInput_t* edit);
void CG_BEChat_InputChar(beChatInput_t* edit, int ch, float maxTextW);
void CG_BEChat_InputKeyDown(beChatInput_t* edit, int key, float maxTextW);
int CG_BEChat_InputVisibleBytes(const char* text, int start, float maxWidth);
qboolean CG_BEChat_SentHistCanNavigate(void);
void CG_BEChat_CancelMuteConfirm(void);
void CG_BEChat_ClearMessageHistory(void);
void CG_BEChat_SaveSentHistToCvars(void);
qboolean CG_BEChat_InputPaletteCodeAt(const char* buf, int pos, int bufLen);
int CG_BEChat_InputAdvanceBufPos(const char* buf, int pos, int bufLen);
qboolean CG_BEChat_IsWordBoundaryChar(char c);
qboolean CG_BEChat_TryEnterMutePickFromBuffer(beChatInput_t* edit);
qboolean CG_BEChat_TryEnterUnmutePickFromBuffer(beChatInput_t* edit);
qboolean CG_BEChat_InputPaletteColorChar(char c);
int CG_BEChat_InputCopyDisplay(const char* buf, int start, int end, char* out, int outSize);
char CG_BEChat_LineDefaultColor(const beChatHistoryLine_t* line);
void CG_BEChat_SentHistPush(const char* text, int destMode, int tellClientNum, const char* tellName);
qboolean CG_BEChat_SentHistProcessKey(int key, float maxTextW);
void CG_BEChat_SaveHistoryToCvars(void);
void CG_BEChat_RestoreHistoryFromCvars(void);
int CG_BEChat_HistoryLineMax(void);
void CG_BEChat_RestoreSentHistFromCvars(void);
void CG_BEChat_ProcessTabKey(float maxTextW);
void CG_BEChat_Scroll(int delta);
int CG_BEChat_VisibleHistoryLines(void);
float CG_BEChat_HistoryTextWidth(int* outDisplayRows, qboolean timeFilter);
void CG_BEChat_DrawHistoryLines(float x, float y, float w, int historyScroll, float historyTextW,
                                qboolean timeFilter);
void CG_BEChat_EncodePercentWire(const char* src, char* dst, int dstSize);
beChatHistoryLine_t* CG_BEChat_HistoryEntry(int linesBack);
beChatSentHistEntry_t* CG_BEChat_SentHistEntryNewest(int indexFromNewest);
qboolean CG_BEChat_TellClientFromIdToken(const char* token, int* outClientNum);
int CG_BEChat_ClientFromTellName(const char* name);
qboolean CG_BEChat_IsReplyLinePrefix(const char* buf, int* outMsgStart);
qboolean CG_BEChat_TryCommitTellTarget(beChatInput_t* edit);
qboolean CG_BEChat_TryCommitReply(beChatInput_t* edit);
qboolean CG_BEChat_TryCommitMutePick(beChatInput_t* edit);
qboolean CG_BEChat_TryCommitUnmutePick(beChatInput_t* edit);
qboolean CG_BEChat_TryCommitMuteIdLine(beChatInput_t* edit);
qboolean CG_BEChat_TryCommitUnmuteIdLine(beChatInput_t* edit);
float CG_BEChat_InputBufSliceDisplayWidth(const char* buf, int start, int end,
        float charWidth, float charHeight, int textFlags, int fontIndex);
char CG_BEChat_ActiveColorAt(const char* text, int byteIndex, char defaultColor);

#endif /* CG_BE_CHATWINDOW_H */
