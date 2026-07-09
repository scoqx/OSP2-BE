// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_fav_server.h"
#include "ui_status_sign.h"

ui_favServer_t  ui_favServer;
char            ui_favLastStatus[BIG_INFO_STRING];

static qboolean UI_Fav_ParseSettingLine(const char* line, const char* key,
                                        char* value, int valueSize)
{
	const char*     p;
	int             keyLen;

	if (!line || !key || !value || valueSize < 1)
	{
		return qfalse;
	}

	keyLen = strlen(key);
	if (Q_stricmpn(line, key, keyLen) != 0)
	{
		return qfalse;
	}

	p = line + keyLen;
	while (*p == ' ' || *p == '\t')
	{
		p++;
	}

	if (!*p)
	{
		return qfalse;
	}

	Q_strncpyz(value, p, valueSize);
	return qtrue;
}

static int UI_Fav_ParseTeamSlotIds(const char* value, int* slots, int maxSlots)
{
	const char*     p;
	int             count;
	int             n;

	if (!value || !value[0] || !slots || maxSlots <= 0)
	{
		return 0;
	}

	count = 0;
	p = value;
	while (*p && count < maxSlots)
	{
		while (*p == ' ' || *p == '\t')
		{
			p++;
		}
		if (!*p)
		{
			break;
		}
		n = atoi(p);
		slots[count++] = n;
		while (*p && *p != ' ' && *p != '\t')
		{
			p++;
		}
	}

	return count;
}

int UI_FavouriteServer_ParseTeamSlotIds(const char* value, int* slots, int maxSlots)
{
	return UI_Fav_ParseTeamSlotIds(value, slots, maxSlots);
}

static qboolean UI_Fav_ParseTeamSlotsLine(const char* line, const char* key,
        int* slots, int maxSlots, int* outCount)
{
	const char*     p;
	int             keyLen;
	int             count;

	if (!line || !key || !slots || !outCount)
	{
		return qfalse;
	}

	keyLen = strlen(key);
	if (Q_stricmpn(line, key, keyLen) != 0)
	{
		return qfalse;
	}

	p = line + keyLen;
	while (*p == ' ' || *p == '\t')
	{
		p++;
	}

	count = UI_Fav_ParseTeamSlotIds(p, slots, maxSlots);
	*outCount = count;
	return (qboolean)(count > 0);
}

static qboolean UI_Fav_ParsePlayerLine(const char* line, ui_favPlayer_t* out)
{
	const char*     p;
	const char*     end;
	int             len;

	if (!line || !out || !line[0])
	{
		return qfalse;
	}

	while (*line == ' ' || *line == '\t')
	{
		line++;
	}

	if (!line[0] || line[0] < '0' || line[0] > '9')
	{
		return qfalse;
	}

	p = line;
	out->num = atoi(p);
	while (*p && *p != ' ' && *p != '\t')
	{
		p++;
	}
	while (*p == ' ' || *p == '\t')
	{
		p++;
	}
	if (!*p)
	{
		return qfalse;
	}

	out->score = atoi(p);
	while (*p && *p != ' ' && *p != '\t')
	{
		p++;
	}
	while (*p == ' ' || *p == '\t')
	{
		p++;
	}
	if (!*p)
	{
		return qfalse;
	}

	out->ping = atoi(p);

	p = strchr(line, '"');
	if (!p)
	{
		out->name[0] = '\0';
		return qtrue;
	}

	p++;
	end = strchr(p, '"');
	if (!end)
	{
		Q_strncpyz(out->name, p, sizeof(out->name));
		return qtrue;
	}

	len = end - p;
	if (len >= (int)sizeof(out->name))
	{
		len = sizeof(out->name) - 1;
	}
	memcpy(out->name, p, len);
	out->name[len] = '\0';
	return qtrue;
}

static void UI_Fav_ParsePlayersFrom(const char* section)
{
	char            line[256];
	const char*     p;
	const char*     lineStart;
	ui_favPlayer_t  pl;
	int             len;

	if (!section)
	{
		return;
	}

	p = section;
	while (*p)
	{
		lineStart = p;
		while (*p && *p != '\n' && *p != '\r')
		{
			p++;
		}

		len = p - lineStart;
		if (len >= (int)sizeof(line))
		{
			len = sizeof(line) - 1;
		}
		memcpy(line, lineStart, len);
		line[len] = '\0';

		while (*p == '\n' || *p == '\r')
		{
			p++;
		}

		if (!line[0])
		{
			continue;
		}

		if (!Q_stricmpn(line, "num", 3))
		{
			const char*     s;

			s = line + 3;
			while (*s == ' ' || *s == ':' || *s == '\t')
			{
				s++;
			}
			if (!Q_stricmpn(s, "score", 5))
			{
				continue;
			}
		}

		if (UI_Fav_ParsePlayerLine(line, &pl) &&
		        ui_favServer.numPlayers < UI_FAV_MAX_PLAYERS)
		{
			ui_favServer.players[ui_favServer.numPlayers++] = pl;
		}
	}
}

void UI_FavouriteServer_CleanHostname(const char* src, char* dst, int dstSize)
{
	const char*     s;
	char*           d;
	char*           max;

	if (!src || !dst || dstSize < 1)
	{
		return;
	}

	s = src;
	d = dst;
	max = dst + dstSize - 1;

	while (*s && d < max)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
			continue;
		}
		if (*s >= ' ' && *s <= '~')
		{
			*d++ = *s;
		}
		s++;
	}

	*d = '\0';
}

static void UI_Fav_ApplyInfoString(const char* infostring)
{
	const char*     s;
	char            fullMap[MAX_QPATH];

	if (!infostring || !infostring[0])
	{
		return;
	}

	s = Info_ValueForKey(infostring, "mapname");
	if (s[0])
	{
		Q_strncpyz(fullMap, s, sizeof(fullMap));
		Q_CleanStr(fullMap);
		Q_strlwr(fullMap);
		Q_strncpyz(ui_favServer.mapname, fullMap, sizeof(ui_favServer.mapname));
		Q_strncpyz(ui_favServer.mapnameLevelshot, fullMap,
		           sizeof(ui_favServer.mapnameLevelshot));
	}

	s = Info_ValueForKey(infostring, "sv_hostname");
	if (!s[0])
	{
		s = Info_ValueForKey(infostring, "hostname");
	}
	if (s[0])
	{
		Q_strncpyz(ui_favServer.hostname, s, sizeof(ui_favServer.hostname));
		UI_FavouriteServer_CleanHostname(s, ui_favServer.hostnameClean,
		                                 sizeof(ui_favServer.hostnameClean));
	}

	if (ui_favServer.maxClients <= 0)
	{
		ui_favServer.maxClients = atoi(Info_ValueForKey(infostring, "sv_maxclients"));
		if (ui_favServer.maxClients <= 0)
		{
			ui_favServer.maxClients = atoi(Info_ValueForKey(infostring, "maxclients"));
		}
	}

	ui_favServer.gametype = atoi(Info_ValueForKey(infostring, "g_gametype"));
	if (ui_favServer.gametype >= GT_TEAM)
	{
		ui_favServer.teamMode = qtrue;
	}

	if (ui_favServer.clients <= 0)
	{
		ui_favServer.clients = atoi(Info_ValueForKey(infostring, "clients"));
	}

	s = Info_ValueForKey(infostring, "Players_Blue");
	if (s[0])
	{
		ui_favServer.numTeamBlue = UI_Fav_ParseTeamSlotIds(s, ui_favServer.teamBlue,
		                           UI_FAV_MAX_TEAM_SLOTS);
		if (ui_favServer.numTeamBlue > 0)
		{
			ui_favServer.teamMode = qtrue;
		}
	}

	s = Info_ValueForKey(infostring, "Players_Red");
	if (s[0])
	{
		ui_favServer.numTeamRed = UI_Fav_ParseTeamSlotIds(s, ui_favServer.teamRed,
		                          UI_FAV_MAX_TEAM_SLOTS);
		if (ui_favServer.numTeamRed > 0)
		{
			ui_favServer.teamMode = qtrue;
		}
	}

	s = Info_ValueForKey(infostring, "Score_Blue");
	if (s[0])
	{
		ui_favServer.scoreBlue = atoi(s);
		ui_favServer.teamMode = qtrue;
	}

	s = Info_ValueForKey(infostring, "Score_Red");
	if (s[0])
	{
		ui_favServer.scoreRed = atoi(s);
		ui_favServer.teamMode = qtrue;
	}
}

static void UI_Fav_ParseStatus(const char* status)
{
	char            line[256];
	const char*     p;
	const char*     lineStart;
	qboolean        inPlayers;
	qboolean        hasPlayersSection;
	char            value[MAX_INFO_VALUE];
	ui_favPlayer_t  pl;
	int             len;

	if (!status || !status[0])
	{
		return;
	}

	hasPlayersSection = (qboolean)(strstr(status, "Players:") != NULL);
	inPlayers = qfalse;

	if (hasPlayersSection)
	{
		ui_favServer.numPlayers = 0;
		ui_favServer.numTeamBlue = 0;
		ui_favServer.numTeamRed = 0;
	}

	p = status;
	while (*p)
	{
		lineStart = p;
		while (*p && *p != '\n' && *p != '\r')
		{
			p++;
		}

		len = p - lineStart;
		if (len >= (int)sizeof(line))
		{
			len = sizeof(line) - 1;
		}
		memcpy(line, lineStart, len);
		line[len] = '\0';

		while (*p == '\n' || *p == '\r')
		{
			p++;
		}

		if (!line[0])
		{
			continue;
		}

		if (!Q_stricmpn(line, "Players:", 8))
		{
			inPlayers = qtrue;
			continue;
		}

		if (inPlayers)
		{
			if (!Q_stricmpn(line, "num", 3))
			{
				const char*     s;

				s = line + 3;
				while (*s == ' ' || *s == ':' || *s == '\t')
				{
					s++;
				}
				if (!Q_stricmpn(s, "score", 5))
				{
					continue;
				}
			}
			if (UI_Fav_ParsePlayerLine(line, &pl) &&
			        ui_favServer.numPlayers < UI_FAV_MAX_PLAYERS)
			{
				ui_favServer.players[ui_favServer.numPlayers++] = pl;
			}
			continue;
		}

		if (UI_Fav_ParseSettingLine(line, "mapname", value, sizeof(value)))
		{
			Q_strncpyz(ui_favServer.mapname, value, sizeof(ui_favServer.mapname));
			Q_CleanStr(ui_favServer.mapname);
			Q_strlwr(ui_favServer.mapname);
			Q_strncpyz(ui_favServer.mapnameLevelshot, ui_favServer.mapname,
			           sizeof(ui_favServer.mapnameLevelshot));
		}
		else if (UI_Fav_ParseSettingLine(line, "sv_hostname", value, sizeof(value)))
		{
			Q_strncpyz(ui_favServer.hostname, value, sizeof(ui_favServer.hostname));
			UI_FavouriteServer_CleanHostname(value, ui_favServer.hostnameClean,
			                                 sizeof(ui_favServer.hostnameClean));
		}
		else if (UI_Fav_ParseSettingLine(line, "sv_maxclients", value, sizeof(value)))
		{
			ui_favServer.maxClients = atoi(value);
		}
		else if (UI_Fav_ParseSettingLine(line, "g_gametype", value, sizeof(value)))
		{
			ui_favServer.gametype = atoi(value);
		}
		else if (UI_Fav_ParseTeamSlotsLine(line, "Players_Blue",
		                                   ui_favServer.teamBlue, UI_FAV_MAX_TEAM_SLOTS,
		                                   &ui_favServer.numTeamBlue))
		{
			ui_favServer.teamMode = qtrue;
		}
		else if (UI_Fav_ParseTeamSlotsLine(line, "Players_Red",
		                                   ui_favServer.teamRed, UI_FAV_MAX_TEAM_SLOTS,
		                                   &ui_favServer.numTeamRed))
		{
			ui_favServer.teamMode = qtrue;
		}
		else if (UI_Fav_ParseSettingLine(line, "Score_Blue", value, sizeof(value)))
		{
			ui_favServer.scoreBlue = atoi(value);
			ui_favServer.teamMode = qtrue;
		}
		else if (UI_Fav_ParseSettingLine(line, "Score_Red", value, sizeof(value)))
		{
			ui_favServer.scoreRed = atoi(value);
			ui_favServer.teamMode = qtrue;
		}
	}

	if (ui_favServer.gametype >= GT_TEAM)
	{
		ui_favServer.teamMode = qtrue;
	}

	if (ui_favServer.maxClients <= 0)
	{
		ui_favServer.maxClients = 64;
	}

	if (ui_favServer.numPlayers <= 0)
	{
		const char*     playersAt;

		playersAt = strstr(status, "Players:");
		if (playersAt)
		{
			UI_Fav_ParsePlayersFrom(playersAt + 8);
		}
	}

	if (ui_favServer.numPlayers > 0 && ui_favServer.clients <= 0)
	{
		ui_favServer.clients = ui_favServer.numPlayers;
	}
}

void UI_FavouriteServer_ApplyFromGetstatus(const char* status)
{
	char            infoline[MAX_INFO_STRING];

	if (!status || !status[0])
	{
		return;
	}

	UI_StatusSign_CopyInfostringOnly(status, infoline, sizeof(infoline));
	if (strchr(infoline, '\\'))
	{
		UI_Fav_ApplyInfoString(infoline);
	}

	UI_Fav_ParseStatus(status);

	if (!ui_favServer.mapname[0] && strchr(status, '\\'))
	{
		UI_Fav_ApplyInfoString(status);
	}
}

void UI_FavouriteServer_ApplyPingInfo(const char* info)
{
	const char*     s;
	char            fullMap[MAX_QPATH];

	if (!info || !info[0])
	{
		return;
	}

	s = Info_ValueForKey(info, "mapname");
	if (s[0])
	{
		Q_strncpyz(fullMap, s, sizeof(fullMap));
		Q_CleanStr(fullMap);
		Q_strlwr(fullMap);
		Q_strncpyz(ui_favServer.mapname, fullMap, sizeof(ui_favServer.mapname));
		Q_strncpyz(ui_favServer.mapnameLevelshot, fullMap,
		           sizeof(ui_favServer.mapnameLevelshot));
	}

	s = Info_ValueForKey(info, "hostname");
	if (s[0])
	{
		Q_strncpyz(ui_favServer.hostname, s, sizeof(ui_favServer.hostname));
		UI_FavouriteServer_CleanHostname(s, ui_favServer.hostnameClean,
		                                 sizeof(ui_favServer.hostnameClean));
	}

	s = Info_ValueForKey(info, "clients");
	if (s[0])
	{
		ui_favServer.clients = atoi(s);
	}

	ui_favServer.maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	if (ui_favServer.maxClients <= 0)
	{
		ui_favServer.maxClients = atoi(Info_ValueForKey(info, "maxclients"));
	}

	s = Info_ValueForKey(info, "g_gametype");
	if (s[0])
	{
		ui_favServer.gametype = atoi(s);
		if (ui_favServer.gametype >= GT_TEAM)
		{
			ui_favServer.teamMode = qtrue;
		}
	}
}

qboolean UI_FavouriteServer_StatusHasPlayersSection(const char* status)
{
	if (!status || !status[0])
	{
		return qfalse;
	}

	return (qboolean)(strstr(status, "Players:") != NULL);
}

qboolean UI_FavouriteServer_StatusIsComplete(const char* status)
{
	if (!status || !status[0])
	{
		return qfalse;
	}

	return (qboolean)(UI_FavouriteServer_StatusHasPlayersSection(status) ||
	                  strstr(status, "Server settings:") != NULL);
}

const ui_favPlayer_t* UI_FavouriteServer_FindPlayer(int num)
{
	int     i;

	for (i = 0; i < ui_favServer.numPlayers; i++)
	{
		if (ui_favServer.players[i].num == num)
		{
			return &ui_favServer.players[i];
		}
	}

	return NULL;
}
