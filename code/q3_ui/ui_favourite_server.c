// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"
#include "ui_fav_server.h"
#include "ui_status_sign.h"

/*
===============================================================================

MAIN MENU FAVOURITE SERVER (ui_favouriteServer cvar)

===============================================================================
*/

#define UI_FAV_PANEL_WIDTH          360.0f
#define UI_FAV_ROW_HEIGHT           28.0f
#define UI_FAV_ROW_Y                128.0f
#define UI_FAV_MENU_SHIFT           32.0f
#define UI_FAV_CHAR_W               8.0f
#define UI_FAV_CHAR_H               12.0f
#define UI_FAV_COLLAPSED_CHAR_H     12.0f
#define UI_FAV_PANEL_PAD            6.0f
#define UI_FAV_CONNECT_LABEL        "Connect"
#define UI_FAV_POLL_INTERVAL_MS     1000
#define UI_FAV_PING_REPING_MS       4000
#define UI_FAV_UNKNOWNMAP           "menu/art/unknownmap"
#define UI_FAV_STATUS_CMD_MS        2000
#define UI_FAV_MAX_STATUS_CMDS      3
#define UI_FAV_MAX_PING_CMDS        3

static qboolean         ui_favFontReady;
static int              ui_favLastPollMs;
static qboolean         ui_favPingMatched;
static qboolean         ui_favSessionFetched;
static int              ui_favStatusCmdCount;
static int              ui_favPingCmdCount;
static char             ui_favCvarAddress[UI_FAV_ADDR_LENGTH];
static qhandle_t        ui_favUnknownMapShader;
static int              ui_favLastIdlePollMs;
static int              ui_favPingIssueMs;
static int              ui_favLastStatusCmdMs;
static int              ui_favJobStartMs;

static qboolean UI_FavouriteServer_MarkAvailable(void);

static void UI_FavouriteServer_ReadCvar(char* out, int outSize)
{
	if (!out || outSize < 1)
	{
		return;
	}

	out[0] = '\0';
	trap_Cvar_VariableStringBuffer("ui_favouriteServer", out, outSize);
	trap_Cvar_Update(&ui_favouriteServer);
}

static void UI_FavouriteServer_ClearServerData(void)
{
	ui_favServer.visible = qfalse;
	ui_favServer.fetching = qtrue;
	ui_favServer.mapname[0] = '\0';
	ui_favServer.mapnameLevelshot[0] = '\0';
	ui_favServer.hostname[0] = '\0';
	ui_favServer.hostnameClean[0] = '\0';
	ui_favServer.clients = 0;
	ui_favServer.maxClients = 0;
	ui_favServer.gametype = 0;
	ui_favServer.teamMode = qfalse;
	ui_favServer.numTeamBlue = 0;
	ui_favServer.numTeamRed = 0;
	ui_favServer.scoreBlue = 0;
	ui_favServer.scoreRed = 0;
	ui_favServer.numPlayers = 0;
	ui_favPingMatched = qfalse;
	ui_favLastStatus[0] = '\0';
	ui_favLastIdlePollMs = 0;
	ui_favPingIssueMs = 0;
	ui_favLastStatusCmdMs = 0;
	ui_favJobStartMs = 0;
	ui_favStatusCmdCount = 0;
	ui_favPingCmdCount = 0;
}

static void UI_FavouriteServer_ResetState(void)
{
	memset(&ui_favServer, 0, sizeof(ui_favServer));
	ui_favLastPollMs = 0;
	ui_favSessionFetched = qfalse;
	ui_favStatusCmdCount = 0;
	ui_favPingCmdCount = 0;
}

void UI_FavouriteServer_IssueStatus(void)
{
	if (!ui_favServer.address[0])
	{
		return;
	}

	if (ui_favStatusCmdCount >= UI_FAV_MAX_STATUS_CMDS)
	{
		return;
	}

	ui_favStatusCmdCount++;
	trap_Cmd_ExecuteText(EXEC_NOW, va("serverstatus %s\n", ui_favServer.address));
	ui_favLastStatusCmdMs = trap_Milliseconds();
}

void UI_FavouriteServer_IssuePingCmd(void)
{
	if (!ui_favServer.address[0])
	{
		return;
	}

	if (ui_favPingCmdCount >= UI_FAV_MAX_PING_CMDS)
	{
		return;
	}

	ui_favPingCmdCount++;
	ui_favPingIssueMs = trap_Milliseconds();
	trap_Cmd_ExecuteText(EXEC_NOW, va("ping %s\n", ui_favServer.address));
}

static qboolean UI_FavouriteServer_SessionReady(void)
{
	char    cvarAddr[UI_FAV_ADDR_LENGTH];

	if (!ui_favSessionFetched)
	{
		return qfalse;
	}

	UI_FavouriteServer_ReadCvar(cvarAddr, sizeof(cvarAddr));
	if (!cvarAddr[0] || !ui_favServer.address[0])
	{
		return qfalse;
	}

	if (Q_stricmp(cvarAddr, ui_favServer.address))
	{
		return qfalse;
	}

	return (qboolean)(ui_favServer.visible && ui_favServer.mapname[0]);
}

static qboolean UI_FavouriteServer_AddressMatch(const char* pingAdr, const char* targetAdr)
{
	const char*     tport;
	const char*     pport;
	int             hostLen;

	if (!pingAdr || !targetAdr || !pingAdr[0] || !targetAdr[0])
	{
		return qfalse;
	}

	if (!Q_stricmp(pingAdr, targetAdr))
	{
		return qtrue;
	}

	tport = Q_strrchr(targetAdr, ':');
	pport = Q_strrchr(pingAdr, ':');
	if (tport && pport && !Q_stricmp(tport, pport))
	{
		hostLen = (int)(tport - targetAdr);
		if ((int)(pport - pingAdr) == hostLen &&
		        !Q_strncmp(pingAdr, targetAdr, hostLen))
		{
			return qtrue;
		}
	}

	if (tport && strstr(pingAdr, tport))
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_FavouriteServer_MapnameMatch(const char* infoMap)
{
	char    fullMap[MAX_QPATH];

	if (!infoMap || !infoMap[0] || !ui_favServer.mapname[0])
	{
		return qfalse;
	}

	Q_strncpyz(fullMap, infoMap, sizeof(fullMap));
	Q_CleanStr(fullMap);
	Q_strupr(fullMap);

	return (qboolean)(!Q_stricmp(fullMap, ui_favServer.mapname));
}

static void UI_FavouriteServer_CapturePingFromList(int source)
{
	char    adrstr[UI_FAV_ADDR_LENGTH];
	char    info[MAX_INFO_STRING];
	int     count;
	int     i;

	count = trap_LAN_GetServerCount(source);
	for (i = 0; i < count; i++)
	{
		trap_LAN_GetServerAddressString(source, i, adrstr, sizeof(adrstr));
		if (!adrstr[0])
		{
			continue;
		}
		if (Q_stricmp(adrstr, ui_favServer.address) &&
		        !UI_FavouriteServer_AddressMatch(adrstr, ui_favServer.address))
		{
			continue;
		}

		trap_LAN_GetServerInfo(source, i, info, sizeof(info));
		if (!info[0])
		{
			continue;
		}

		if (!Info_ValueForKey(info, "clients")[0] &&
		        !Info_ValueForKey(info, "sv_maxclients")[0] &&
		        !Info_ValueForKey(info, "maxclients")[0] &&
		        !Info_ValueForKey(info, "mapname")[0])
		{
			continue;
		}

		UI_FavouriteServer_ApplyPingInfo(info);
		ui_favPingMatched = qtrue;
		return;
	}
}

static qboolean UI_FavouriteServer_PingInFlight(void)
{
	char    adrstr[UI_FAV_ADDR_LENGTH];
	int     i;
	int     time;

	if (!ui_favServer.address[0])
	{
		return qfalse;
	}

	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		trap_LAN_GetPing(i, adrstr, sizeof(adrstr), &time);
		if (!adrstr[0])
		{
			continue;
		}
		if (Q_stricmp(adrstr, ui_favServer.address) &&
		        !UI_FavouriteServer_AddressMatch(adrstr, ui_favServer.address))
		{
			continue;
		}

		if (time == 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

static void UI_FavouriteServer_CapturePingByMapname(void)
{
	char    adrstr[UI_FAV_ADDR_LENGTH];
	char    info[MAX_INFO_STRING];
	int     i;
	int     time;

	if (!ui_favServer.mapname[0])
	{
		return;
	}

	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		trap_LAN_GetPing(i, adrstr, sizeof(adrstr), &time);
		if (!adrstr[0] || !time)
		{
			continue;
		}

		trap_LAN_GetPingInfo(i, info, sizeof(info));
		if (!info[0])
		{
			continue;
		}

		if (!UI_FavouriteServer_MapnameMatch(Info_ValueForKey(info, "mapname")))
		{
			continue;
		}

		UI_FavouriteServer_ApplyPingInfo(info);
		ui_favPingMatched = qtrue;
		trap_LAN_ClearPing(i);
		return;
	}
}

static void UI_FavouriteServer_CapturePingInfo(void)
{
	char    adrstr[UI_FAV_ADDR_LENGTH];
	char    info[MAX_INFO_STRING];
	int     i;
	int     time;

	if (!ui_favServer.address[0])
	{
		return;
	}

	for (i = 0; i < MAX_PINGREQUESTS; i++)
	{
		trap_LAN_GetPing(i, adrstr, sizeof(adrstr), &time);
		if (!adrstr[0])
		{
			continue;
		}
		if (Q_stricmp(adrstr, ui_favServer.address) &&
		        !UI_FavouriteServer_AddressMatch(adrstr, ui_favServer.address))
		{
			continue;
		}

		if (!time)
		{
			continue;
		}

		trap_LAN_GetPingInfo(i, info, sizeof(info));
		if (!info[0])
		{
			trap_LAN_ClearPing(i);
			continue;
		}

		UI_FavouriteServer_ApplyPingInfo(info);
		ui_favPingMatched = qtrue;
		trap_LAN_ClearPing(i);
		return;
	}

	if (!ui_favPingMatched)
	{
		UI_FavouriteServer_CapturePingByMapname();
	}

	if (ui_favServer.clients <= 0)
	{
		UI_FavouriteServer_CapturePingFromList(AS_LOCAL);
	}

	if (ui_favServer.clients <= 0)
	{
		UI_FavouriteServer_CapturePingFromList(AS_FAVORITES);
	}
}

static void UI_FavouriteServer_EnsurePing(void)
{
	int     now;

	if (!ui_favServer.address[0] || ui_favPingMatched)
	{
		return;
	}

	if (ui_favPingCmdCount >= UI_FAV_MAX_PING_CMDS)
	{
		return;
	}

	trap_LAN_UpdateVisiblePings(AS_LOCAL);
	trap_LAN_UpdateVisiblePings(AS_FAVORITES);
	UI_FavouriteServer_CapturePingInfo();
	if (ui_favPingMatched)
	{
		return;
	}

	if (UI_FavouriteServer_PingInFlight())
	{
		return;
	}

	if (trap_LAN_GetPingQueueCount() >= MAX_PINGREQUESTS)
	{
		return;
	}

	now = trap_Milliseconds();
	if (ui_favPingIssueMs > 0 && now - ui_favPingIssueMs < UI_FAV_PING_REPING_MS)
	{
		return;
	}

	UI_FavouriteServer_IssuePingCmd();
}

static qboolean UI_FavouriteServer_ReadTrapStatus(char* status, int statusSize)
{
	int     ret;

	if (!status || statusSize <= 0 || !ui_favServer.address[0])
	{
		return qfalse;
	}

	status[0] = '\0';
	ret = trap_LAN_ServerStatus(ui_favServer.address, status, statusSize);
	if (ret && status[0])
	{
		Q_strncpyz(ui_favLastStatus, status, sizeof(ui_favLastStatus));
		UI_FavouriteServer_ApplyFromGetstatus(status);
		if (ui_favServer.mapname[0])
		{
			UI_FavouriteServer_MarkAvailable();
		}
		return qtrue;
	}

	return qfalse;
}

static void UI_FavouriteServer_ApplyTrapStatus(void)
{
	char            status[BIG_INFO_STRING];
	int             ret;
	int             statusLen;
	int             lastLen;

	status[0] = '\0';
	ret = trap_LAN_ServerStatus(ui_favServer.address, status, sizeof(status));
	if (!ret || !status[0])
	{
		return;
	}

	statusLen = (int)strlen(status);
	lastLen = (int)strlen(ui_favLastStatus);
	if (statusLen <= lastLen && !Q_stricmp(status, ui_favLastStatus))
	{
		return;
	}

	Q_strncpyz(ui_favLastStatus, status, sizeof(ui_favLastStatus));
	UI_FavouriteServer_ApplyFromGetstatus(status);
	if (ui_favServer.mapname[0])
	{
		UI_FavouriteServer_MarkAvailable();
	}
}

static void UI_FavouriteServer_PassivePoll(void)
{
	int             now;
	char            status[BIG_INFO_STRING];

	if (!ui_favServer.address[0])
	{
		return;
	}

	if (ui_favServer.fetching)
	{
		return;
	}

	trap_LAN_UpdateVisiblePings(AS_LOCAL);
	trap_LAN_UpdateVisiblePings(AS_FAVORITES);
	UI_FavouriteServer_CapturePingInfo();

	if (!ui_favServer.visible)
	{
		return;
	}

	if (ui_favSessionFetched && ui_favServer.mapname[0])
	{
		if (!ui_favPingMatched && ui_favPingCmdCount < UI_FAV_MAX_PING_CMDS)
		{
			UI_FavouriteServer_EnsurePing();
		}
		return;
	}

	if (ui_favServer.clients <= 0)
	{
		UI_FavouriteServer_EnsurePing();
	}

	now = trap_Milliseconds();
	if (ui_favServer.visible && now - ui_favLastIdlePollMs < UI_FAV_POLL_INTERVAL_MS)
	{
		return;
	}

	ui_favLastIdlePollMs = now;
	if (ui_favStatusCmdCount < UI_FAV_MAX_STATUS_CMDS &&
	        now - ui_favLastStatusCmdMs >= UI_FAV_STATUS_CMD_MS)
	{
		UI_FavouriteServer_IssueStatus();
	}
	UI_FavouriteServer_ReadTrapStatus(status, sizeof(status));
	UI_FavouriteServer_CapturePingInfo();
	UI_FavouriteServer_ApplyTrapStatus();
}

static qboolean UI_FavouriteServer_MarkAvailable(void)
{
	if (!ui_favServer.mapname[0])
	{
		return qfalse;
	}

	ui_favServer.visible = qtrue;
	ui_favServer.fetching = qfalse;
	ui_favSessionFetched = qtrue;
	return qtrue;
}

static void UI_FavouriteServer_BeginFetch(void)
{
	char    cvarAddr[UI_FAV_ADDR_LENGTH];

	UI_FavouriteServer_ReadCvar(cvarAddr, sizeof(cvarAddr));
	if (!cvarAddr[0])
	{
		UI_FavouriteServer_ResetState();
		ui_favCvarAddress[0] = '\0';
		return;
	}

	Q_strncpyz(ui_favServer.address, cvarAddr, sizeof(ui_favServer.address));
	UI_FavouriteServer_ClearServerData();
	ui_favJobStartMs = trap_Milliseconds();
	ui_favLastStatusCmdMs = 0;
	ui_favPingIssueMs = 0;
	ui_favStatusCmdCount = 0;
	ui_favPingCmdCount = 0;
	ui_favSessionFetched = qfalse;
	trap_LAN_AddServer(AS_FAVORITES, cvarAddr, cvarAddr);
	trap_LAN_UpdateVisiblePings(AS_FAVORITES);
	UI_FavouriteServer_IssuePingCmd();
	UI_FavouriteServer_IssueStatus();
	UI_LANJob_RequestFavouriteServer(ui_favServer.address);
}

static void UI_FavouriteServer_EnsureFonts(void)
{
	int     fontIndex;

	if (!UI_FontLoaded(2))
	{
		UI_LoadFonts();
	}

	trap_Cvar_Update(&ui_beStatusFont);
	fontIndex = UI_FontIndexFromCvar(&ui_beStatusFont);
	if (!UI_FontAvailable(fontIndex))
	{
		fontIndex = 2;
	}

	if (UI_FontLoaded(fontIndex))
	{
		UI_FontSelect(fontIndex);
		ui_favFontReady = qtrue;
	}
	else
	{
		ui_favFontReady = qfalse;
	}
}

static qboolean UI_FavouriteServer_GetBorderColor(vec4_t borderColor)
{
	trap_Cvar_Update(&ui_beStatusBorderColor);
	if (!ui_beStatusBorderColor.string[0])
	{
		return qfalse;
	}
	if (!UI_ParseColorStr(ui_beStatusBorderColor.string, borderColor))
	{
		Vector4Copy(colorWhite, borderColor);
	}
	return qtrue;
}

static void UI_FavouriteServer_PanelRect(float* outX, float* outY, float* outW, float* outH)
{
	*outW = UI_FAV_PANEL_WIDTH;
	*outX = (640.0f - UI_FAV_PANEL_WIDTH) * 0.5f;
	*outH = UI_FAV_ROW_HEIGHT;
	*outY = UI_FAV_ROW_Y;
}

static void UI_FavouriteServer_DrawTextWidth(float x, float y, const char* text, vec4_t color,
        int flags, int maxWidth640, float charW, float charH)
{
	if (!text || !text[0] || !ui_favFontReady)
	{
		return;
	}

	if (maxWidth640 < 1)
	{
		maxWidth640 = 1;
	}

	UI_OSPDrawStringNew(x, y, text, color, color_black,
	                    charW, charH,
	                    maxWidth640, flags | DS_SHADOW | DS_PROPORTIONAL, NULL, NULL, NULL);
}

static void UI_FavouriteServer_ConnectButtonRect(float panelX, float panelY, float panelW,
        float rowH, float* outX, float* outY, float* outW, float* outH)
{
	float   btnW;
	float   btnH;

	btnW = UI_FAV_CONNECT_LABEL[0] ? (strlen(UI_FAV_CONNECT_LABEL) * UI_FAV_CHAR_W +
	                                  UI_FAV_PANEL_PAD * 2.0f) : 48.0f;
	btnH = UI_FAV_COLLAPSED_CHAR_H + 4.0f;
	*outW = btnW;
	*outH = btnH;
	*outX = panelX + panelW - UI_FAV_PANEL_PAD - btnW;
	*outY = panelY + (rowH - btnH) * 0.5f;
}

static void UI_FavouriteServer_DrawConnectButton(float panelX, float panelY, float panelW,
        float rowH, float charW, float charH)
{
	vec4_t          textColor;
	vec4_t          borderColor;
	float           btnX;
	float           btnY;
	float           btnW;
	float           btnH;
	float           centerX;
	float           centerY;

	UI_FavouriteServer_ConnectButtonRect(panelX, panelY, panelW, rowH,
	                                     &btnX, &btnY, &btnW, &btnH);

	if (uis.cursorx >= btnX && uis.cursorx <= btnX + btnW &&
	        uis.cursory >= btnY && uis.cursory <= btnY + btnH)
	{
		if (trap_Key_IsDown(K_MOUSE1))
		{
			textColor[0] = 0.55f;
			textColor[1] = 0.55f;
			textColor[2] = 0.55f;
		}
		else
		{
			textColor[0] = 1.0f;
			textColor[1] = 1.0f;
			textColor[2] = 1.0f;
		}
	}
	else
	{
		textColor[0] = 0.55f;
		textColor[1] = 0.55f;
		textColor[2] = 0.55f;
	}
	textColor[3] = 1.0f;

	if (UI_FavouriteServer_GetBorderColor(borderColor))
	{
		UI_DrawRect(btnX, btnY, btnW, btnH, borderColor);
	}

	centerX = btnX + btnW * 0.5f;
	centerY = btnY + btnH * 0.5f;
	UI_FavouriteServer_DrawTextWidth(centerX, centerY, UI_FAV_CONNECT_LABEL, textColor,
	                                 DS_VCENTER | DS_HCENTER, SCREEN_WIDTH, charW,
	                                 UI_FAV_COLLAPSED_CHAR_H);
}

static qboolean UI_FavouriteServer_ConnectButtonHit(float panelX, float panelY, float panelW,
        float rowH)
{
	float   btnX;
	float   btnY;
	float   btnW;
	float   btnH;

	UI_FavouriteServer_ConnectButtonRect(panelX, panelY, panelW, rowH,
	                                     &btnX, &btnY, &btnW, &btnH);

	if (uis.cursorx >= btnX && uis.cursorx <= btnX + btnW &&
	        uis.cursory >= btnY && uis.cursory <= btnY + btnH)
	{
		return qtrue;
	}

	return qfalse;
}

void UI_FavouriteServer_CheckCvarChanged(void)
{
	char    newAddr[UI_FAV_ADDR_LENGTH];

	UI_FavouriteServer_ReadCvar(newAddr, sizeof(newAddr));

	if (!Q_stricmp(newAddr, ui_favCvarAddress))
	{
		return;
	}

	Q_strncpyz(ui_favCvarAddress, newAddr, sizeof(ui_favCvarAddress));

	if (!newAddr[0])
	{
		UI_LANJob_CancelQueuedFavourite();
		UI_FavouriteServer_ResetState();
		return;
	}

	UI_FavouriteServer_ResetState();
	Q_strncpyz(ui_favCvarAddress, newAddr, sizeof(ui_favCvarAddress));
	UI_FavouriteServer_BeginFetch();
}

void UI_FavouriteServer_LanJobBegin(const char* target)
{
	if (!target || !target[0])
	{
		return;
	}

	if (ui_favServer.address[0] && !Q_stricmp(ui_favServer.address, target))
	{
		ui_favServer.fetching = qtrue;
		if (ui_favJobStartMs <= 0)
		{
			ui_favJobStartMs = trap_Milliseconds();
		}
		return;
	}

	Q_strncpyz(ui_favServer.address, target, sizeof(ui_favServer.address));
	UI_FavouriteServer_ClearServerData();
	ui_favJobStartMs = trap_Milliseconds();
	ui_favLastPollMs = trap_Milliseconds();
}

qboolean UI_FavouriteServer_LanJobTick(void)
{
	char            status[BIG_INFO_STRING];
	int             now;

	now = trap_Milliseconds();
	trap_LAN_UpdateVisiblePings(AS_LOCAL);
	UI_FavouriteServer_ReadTrapStatus(status, sizeof(status));
	UI_FavouriteServer_CapturePingInfo();

	if (ui_favStatusCmdCount < UI_FAV_MAX_STATUS_CMDS &&
	        now - ui_favLastStatusCmdMs >= UI_FAV_STATUS_CMD_MS)
	{
		UI_FavouriteServer_IssueStatus();
	}

	if (ui_favServer.clients <= 0 && ui_favPingCmdCount < UI_FAV_MAX_PING_CMDS)
	{
		UI_FavouriteServer_EnsurePing();
	}

	if (ui_favServer.mapname[0])
	{
		UI_FavouriteServer_MarkAvailable();
		return qtrue;
	}

	return qfalse;
}

void UI_FavouriteServer_PingPoll(void)
{
	char    status[BIG_INFO_STRING];

	if (!ui_favServer.address[0])
	{
		return;
	}

	trap_LAN_UpdateVisiblePings(AS_LOCAL);
	trap_LAN_UpdateVisiblePings(AS_FAVORITES);

	if (ui_favServer.fetching)
	{
		UI_FavouriteServer_ReadTrapStatus(status, sizeof(status));
	}

	UI_FavouriteServer_CapturePingInfo();

	if (ui_favPingMatched || ui_favServer.clients > 0)
	{
		return;
	}

	if (!ui_favServer.visible && !ui_favServer.fetching)
	{
		return;
	}

	UI_FavouriteServer_EnsurePing();
}

void UI_FavouriteServer_LanJobTimeout(void)
{
	char    status[BIG_INFO_STRING];

	UI_FavouriteServer_ReadTrapStatus(status, sizeof(status));
	UI_FavouriteServer_CapturePingInfo();
	UI_FavouriteServer_MarkAvailable();
	ui_favServer.fetching = qfalse;
}

void UI_FavouriteServer_NotifyLanJobComplete(void)
{
	UI_FavouriteServer_CapturePingInfo();
	UI_FavouriteServer_MarkAvailable();
}

void UI_FavouriteServer_FetchOnMainMenu(void)
{
	char    cvarAddr[UI_FAV_ADDR_LENGTH];

	if (!ui_favUnknownMapShader)
	{
		ui_favUnknownMapShader = trap_R_RegisterShaderNoMip(UI_FAV_UNKNOWNMAP);
	}

	if (UI_FavouriteServer_SessionReady())
	{
		return;
	}

	UI_FavouriteServer_ReadCvar(cvarAddr, sizeof(cvarAddr));

	if (!cvarAddr[0])
	{
		if (ui_favCvarAddress[0])
		{
			UI_FavouriteServer_CheckCvarChanged();
		}
		return;
	}

	if (Q_stricmp(cvarAddr, ui_favCvarAddress))
	{
		UI_FavouriteServer_CheckCvarChanged();
	}
}

void UI_FavouriteServer_Tick(void)
{
}

float UI_FavouriteServer_GetMenuShift(void)
{
	if (!ui_favServer.visible)
	{
		return 0.0f;
	}

	return UI_FAV_MENU_SHIFT;
}

qboolean UI_FavouriteServer_IsVisible(void)
{
	return ui_favServer.visible;
}

qboolean UI_FavouriteServer_IsExpanded(void)
{
	return qfalse;
}

qboolean UI_FavouriteServer_IsModal(void)
{
	return qfalse;
}

void UI_FavouriteServer_Draw(void)
{
	vec4_t          bg;
	vec4_t          borderColor;
	vec4_t          textColor;
	vec4_t          playersColor;
	vec4_t          blueColor;
	vec4_t          redColor;
	vec4_t          sepColor;
	float           panelX;
	float           panelY;
	float           panelW;
	float           panelH;
	float           thumbW;
	float           thumbH;
	float           textX;
	float           textY;
	char            playersBuf[32];
	char            blueScoreBuf[16];
	char            redScoreBuf[16];
	char            picname[MAX_QPATH];
	qhandle_t       levelshot;
	float           charW;
	float           charH;
	float           centerX;
	float           mapMaxW;

	if (!ui_favServer.visible)
	{
		return;
	}

	UI_FavouriteServer_EnsureFonts();
	UI_FavouriteServer_PanelRect(&panelX, &panelY, &panelW, &panelH);

	bg[0] = 0.0f;
	bg[1] = 0.0f;
	bg[2] = 0.0f;
	bg[3] = 0.25f;
	UI_FillRect(panelX, panelY, panelW, panelH, bg);

	if (UI_FavouriteServer_GetBorderColor(borderColor))
	{
		UI_DrawRect(panelX, panelY, panelW, panelH, borderColor);
	}

	charW = UI_FAV_CHAR_W;
	charH = UI_FAV_CHAR_H;

	thumbH = UI_FAV_ROW_HEIGHT - 4.0f;
	thumbW = thumbH * 4.0f / 3.0f;
	Com_sprintf(picname, sizeof(picname), "levelshots/%s", ui_favServer.mapnameLevelshot);
	levelshot = trap_R_RegisterShaderNoMip(picname);
	if (!levelshot)
	{
		levelshot = ui_favUnknownMapShader;
	}

	UI_DrawHandlePic(panelX + UI_FAV_PANEL_PAD, panelY + 2.0f, thumbW, thumbH, levelshot);

	textX = panelX + UI_FAV_PANEL_PAD + thumbW + UI_FAV_PANEL_PAD;
	textY = panelY + UI_FAV_ROW_HEIGHT * 0.5f;
	centerX = panelX + panelW * 0.5f;

	Vector4Copy(text_color_normal, textColor);
	if (ui_favServer.teamMode)
	{
		mapMaxW = centerX - textX - 56.0f;
	}
	else
	{
		mapMaxW = (panelX + panelW - UI_FAV_PANEL_PAD - 90.0f - UI_FAV_PANEL_PAD) - textX;
	}
	if (mapMaxW < 48.0f)
	{
		mapMaxW = 48.0f;
	}
	UI_FavouriteServer_DrawTextWidth(textX, textY, ui_favServer.mapname, textColor,
	                                 DS_VCENTER, (int)mapMaxW, charW,
	                                 UI_FAV_COLLAPSED_CHAR_H);

	if (ui_favServer.teamMode)
	{
		Com_sprintf(blueScoreBuf, sizeof(blueScoreBuf), "%d", ui_favServer.scoreBlue);
		Com_sprintf(redScoreBuf, sizeof(redScoreBuf), "%d", ui_favServer.scoreRed);

		blueColor[0] = 0.5f;
		blueColor[1] = 0.5f;
		blueColor[2] = 1.0f;
		blueColor[3] = 1.0f;
		redColor[0] = 1.0f;
		redColor[1] = 0.5f;
		redColor[2] = 0.5f;
		redColor[3] = 1.0f;
		sepColor[0] = 0.55f;
		sepColor[1] = 0.55f;
		sepColor[2] = 0.55f;
		sepColor[3] = 1.0f;

		UI_FavouriteServer_DrawTextWidth(centerX - 8.0f, textY, blueScoreBuf, blueColor,
		                                 DS_VCENTER | DS_HRIGHT, SCREEN_WIDTH, charW,
		                                 UI_FAV_COLLAPSED_CHAR_H);
		UI_FavouriteServer_DrawTextWidth(centerX, textY, ":", sepColor,
		                                 DS_VCENTER | DS_HCENTER, SCREEN_WIDTH, charW,
		                                 UI_FAV_COLLAPSED_CHAR_H);
		UI_FavouriteServer_DrawTextWidth(centerX + 8.0f, textY, redScoreBuf, redColor,
		                                 DS_VCENTER | DS_HLEFT, SCREEN_WIDTH, charW,
		                                 UI_FAV_COLLAPSED_CHAR_H);
	}

	if (ui_favPingMatched)
	{
		Com_sprintf(playersBuf, sizeof(playersBuf), "%d/%d", ui_favServer.clients,
		            ui_favServer.maxClients);
	}
	else if (ui_favServer.maxClients > 0)
	{
		Com_sprintf(playersBuf, sizeof(playersBuf), "-/%d", ui_favServer.maxClients);
	}
	else
	{
		Com_sprintf(playersBuf, sizeof(playersBuf), "-/-");
	}

	playersColor[0] = 0.55f;
	playersColor[1] = 0.55f;
	playersColor[2] = 0.55f;
	playersColor[3] = 1.0f;

	UI_FavouriteServer_DrawTextWidth(panelX + panelW - UI_FAV_PANEL_PAD - 90.0f, textY,
	                                 playersBuf, playersColor,
	                                 DS_VCENTER | DS_HRIGHT, SCREEN_WIDTH, charW,
	                                 UI_FAV_COLLAPSED_CHAR_H);

	UI_FavouriteServer_DrawConnectButton(panelX, panelY, panelW, UI_FAV_ROW_HEIGHT,
	                                     charW, charH);
}

qboolean UI_FavouriteServer_HandleKey(int key)
{
	float   panelX;
	float   panelY;
	float   panelW;
	float   panelH;

	if (!ui_favServer.visible)
	{
		return qfalse;
	}

	UI_FavouriteServer_PanelRect(&panelX, &panelY, &panelW, &panelH);

	if (key == K_MOUSE1)
	{
		if (UI_FavouriteServer_ConnectButtonHit(panelX, panelY, panelW, UI_FAV_ROW_HEIGHT))
		{
			trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", ui_favServer.address));
			return qtrue;
		}
	}

	return qfalse;
}

void UI_FavouriteServer_OnMainMenuDraw(void)
{
	UI_FavouriteServer_CheckCvarChanged();
	UI_FavouriteServer_PassivePoll();
}
