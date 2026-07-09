/*
===========================================================================
ui_fav_server.h — favourite server panel (shared types / parse API)
===========================================================================
*/

#ifndef UI_FAV_SERVER_H
#define UI_FAV_SERVER_H

#include "ui_local.h"

#define UI_FAV_ADDR_LENGTH          64
#define UI_FAV_MAX_PLAYERS          64
#define UI_FAV_MAX_TEAM_SLOTS       32

typedef struct
{
	int         num;
	int         score;
	int         ping;
	char        name[MAX_NAME_LENGTH];
} ui_favPlayer_t;

typedef struct
{
	qboolean    visible;
	qboolean    fetching;
	qboolean    expanded;
	float       expandFrac;
	int         expandLastMs;
	char        address[UI_FAV_ADDR_LENGTH];
	char        mapname[MAX_QPATH];
	char        mapnameLevelshot[MAX_QPATH];
	char        hostname[MAX_INFO_VALUE];
	char        hostnameClean[MAX_INFO_VALUE];
	int         clients;
	int         maxClients;
	int         gametype;
	qboolean    teamMode;
	int         teamBlue[UI_FAV_MAX_TEAM_SLOTS];
	int         numTeamBlue;
	int         teamRed[UI_FAV_MAX_TEAM_SLOTS];
	int         numTeamRed;
	int         scoreBlue;
	int         scoreRed;
	ui_favPlayer_t  players[UI_FAV_MAX_PLAYERS];
	int         numPlayers;
} ui_favServer_t;

extern ui_favServer_t   ui_favServer;
extern char             ui_favLastStatus[BIG_INFO_STRING];

void UI_FavouriteServer_CleanHostname(const char* src, char* dst, int dstSize);
void UI_FavouriteServer_ApplyFromGetstatus(const char* status);
void UI_FavouriteServer_ApplyPingInfo(const char* info);
qboolean UI_FavouriteServer_StatusHasPlayersSection(const char* status);
qboolean UI_FavouriteServer_StatusIsComplete(const char* status);
int UI_FavouriteServer_ParseTeamSlotIds(const char* value, int* slots, int maxSlots);
const ui_favPlayer_t* UI_FavouriteServer_FindPlayer(int num);

#endif /* UI_FAV_SERVER_H */
