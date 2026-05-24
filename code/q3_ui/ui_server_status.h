/*
===========================================================================
ui_server_status.h — getstatus / mod_sig + main menu status bar
===========================================================================
*/

#ifndef UI_SERVER_STATUS_H
#define UI_SERVER_STATUS_H

#include "../game/q_shared.h"

typedef struct
{
	qboolean    valid;
	qboolean    received;
	char        version[64];
	int         flags;
	int         sivj;
	int         date;
	char        hfv[MAX_INFO_VALUE];
	char        titleText[MAX_INFO_VALUE];
	qboolean    last;
	char        changelogHeader[128];
	char        changelogUpdateDate[64];
	char        changelogNewVersion[64];
	char        changelogBody[2048];
	qboolean    changelogReady;
} uiServerStatus_t;

extern uiServerStatus_t uiServerStatus;

void UI_ServerStatus_Clear(void);
void UI_ServerStatus_FetchBegin(const char* target);
void UI_ServerStatus_FetchOnMainMenu(void);
qboolean UI_ServerStatus_ParseValidatedGetstatus(const char* status, char* infostring,
        int infostringSize);
void UI_ServerStatus_ApplyValidatedInfostring(const char* infostring);
void UI_ServerStatus_ApplyFromGetstatus(const char* status);
void UI_ServerStatus_ApplyFromGetinfo(const char* info);
qboolean UI_ServerStatus_WantsChangelog(void);
void UI_ServerStatus_OnFetchTimeout(void);
void UI_ServerStatus_DrawBar(void);
qboolean UI_ServerStatus_HandleBarClick(int key);
qboolean UI_ServerStatus_HandleModalKey(int key);
qboolean UI_ServerStatus_IsModal(void);

#endif /* UI_SERVER_STATUS_H */
