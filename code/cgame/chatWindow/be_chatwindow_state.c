/* cg_be_chatwindow_state.c — shared enterchat globals */

#include "../cg_local.h"
#include "be_chatwindow.h"

beChatWindowState_t beChatWindow;
beChatCmdAc_t beChatCmdAc;
char beChatPromptText[64];
beChatSentHistEntry_t beChatSentHist[BE_CHAT_SENT_HIST_SLOTS];
int beChatSentHistWrite;
int beChatSentHistCount;
qboolean beChatMouse1Prev;
qboolean beChatHistoryRestoreActive;
int beChatRowsCacheWriteIdx;
int beChatRowsCacheCount;
float beChatRowsCacheWidth;
qboolean beChatRowsCacheTimeFilter;
int beChatRowsCacheFrameTime;
int beChatRowsCacheTotal;
int beChatRowsCacheRows[BE_CHAT_HISTORY_LINES];
int beChatFlatCount;
qboolean beChatFlatFull;
char beChatFlatText[BE_CHAT_FLAT_ROWS_MAX][BE_CHAT_LINE_CHARS];
int beChatFlatLinesBack[BE_CHAT_FLAT_ROWS_MAX];
int beChatWinLayoutFrame = -1;
float beChatWinLayoutX;
float beChatWinLayoutY;
float beChatWinLayoutW;
float beChatWinLayoutH;

const char* beChatHelpTipLines[BE_CHAT_HELP_TIP_LINE_COUNT] =
{
	"^2Enter^7  send message",
	"^2Esc^7  close without sending",
	"^2Tab^7  switch global and team chat",
	"^2Alt^7  emoji picker; type :name, then ^2Tab^7",
	"Scroll: ^2wheel^7, ^2PgUp^7 / ^2PgDn^7, ^2Ctrl+Up^7 / ^2Ctrl+Down^7",
	"Slash line: /tell, /reply, /mute, /unmute, or player name",
	"^2Up^7 / ^2Down^7  last sent (empty input) or autocomplete",
};
