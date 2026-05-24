// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"
#include "ui_status_sign.h"

#define UI_GAMEINFO_CFG_FILE            "gameinfo.cfg"
#define UI_GAMEINFO_CFG_READ_MAX        4096
#define UI_GAMEINFO_STATUS_ADDR_MAX     64

#define UI_GAMEINFO_DEFAULT_STATUS_PRIMARY      "141.98.86.110:27888"
#define UI_GAMEINFO_DEFAULT_STATUS_ALTERNATE      "83.69.233.52:27888"

#define UI_GAMEINFO_KEY_STATUS_SERVER1    "be_statusServer1"
#define UI_GAMEINFO_KEY_STATUS_SERVER2    "be_statusServer2"

static char     ui_gameinfo_statusServer1[UI_GAMEINFO_STATUS_ADDR_MAX];
static char     ui_gameinfo_statusServer2[UI_GAMEINFO_STATUS_ADDR_MAX];
static qboolean ui_gameinfo_hasStatusServer1;
static qboolean ui_gameinfo_hasStatusServer2;

static void UI_GameinfoCfg_StripCommentLines(char* buf)
{
	char*       src;
	char*       dst;
	const char* line;
	qboolean    lineStart;

	if (!buf)
	{
		return;
	}

	src = buf;
	dst = buf;
	lineStart = qtrue;

	while (*src)
	{
		if (lineStart)
		{
			line = src;
			while (*src == ' ' || *src == '\t')
			{
				src++;
			}
			if (*src == '#')
			{
				while (*src && *src != '\n')
				{
					src++;
				}
				if (*src == '\n')
				{
					src++;
				}
				continue;
			}
			src = (char*)line;
			lineStart = qfalse;
		}

		*dst++ = *src;
		if (*src == '\n')
		{
			lineStart = qtrue;
		}
		src++;
	}

	*dst = '\0';
}

static void UI_GameinfoCfg_TrimValue(char* s)
{
	int len;

	if (!s || !s[0])
	{
		return;
	}

	len = (int)strlen(s);
	while (len > 0 && (s[len - 1] == ';' || s[len - 1] == ' ' || s[len - 1] == '\t'))
	{
		s[len - 1] = '\0';
		len--;
	}
}

static void UI_GameinfoCfg_SetStatusServer(char* dest, int destSize, qboolean* hasFlag,
        const char* value)
{
	if (!dest || destSize <= 0 || !hasFlag || !value || !value[0])
	{
		return;
	}

	Q_strncpyz(dest, value, destSize);
	*hasFlag = qtrue;
}

static void UI_GameinfoCfg_ApplyPair(const char* key, const char* value)
{
	if (!key || !key[0] || !value || !value[0])
	{
		return;
	}

	if (!Q_stricmp(key, UI_GAMEINFO_KEY_STATUS_SERVER1))
	{
		UI_GameinfoCfg_SetStatusServer(ui_gameinfo_statusServer1,
		        sizeof(ui_gameinfo_statusServer1), &ui_gameinfo_hasStatusServer1, value);
	}
	else if (!Q_stricmp(key, UI_GAMEINFO_KEY_STATUS_SERVER2))
	{
		UI_GameinfoCfg_SetStatusServer(ui_gameinfo_statusServer2,
		        sizeof(ui_gameinfo_statusServer2), &ui_gameinfo_hasStatusServer2, value);
	}
}

static void UI_GameinfoCfg_SkipBracedBlock(char** text)
{
	char*   token;
	int     depth;

	if (!text || !*text)
	{
		return;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	depth = 1;
	while (depth > 0)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			break;
		}
		if (!strcmp(token, "{"))
		{
			depth++;
		}
		else if (!strcmp(token, "}"))
		{
			depth--;
		}
	}
}

static void UI_GameinfoCfg_ParseKeyValueBlock(char** text)
{
	char*       token;
	char        key[MAX_TOKEN_CHARS];

	if (!text || !*text)
	{
		return;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			break;
		}
		if (!strcmp(token, "}"))
		{
			break;
		}

		Q_strncpyz(key, token, sizeof(key));

		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			continue;
		}
		if (token[0] == '=')
		{
			token = COM_ParseExt(text, qfalse);
			if (!token[0])
			{
				continue;
			}
		}

		UI_GameinfoCfg_TrimValue(token);
		UI_GameinfoCfg_ApplyPair(key, token);
	}
}

static void UI_GameinfoCfg_ParseAddCvarBlock(char** text)
{
	char*       token;

	if (!text || !*text)
	{
		return;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			break;
		}
		if (!strcmp(token, "}"))
		{
			break;
		}

		if (!Q_stricmp(token, "ui"))
		{
			UI_GameinfoCfg_ParseKeyValueBlock(text);
		}
		else if (!Q_stricmp(token, "cgame"))
		{
			UI_GameinfoCfg_SkipBracedBlock(text);
		}
		else
		{
			UI_GameinfoCfg_SkipBracedBlock(text);
		}
	}
}

static void UI_GameinfoCfg_ParseBuffer(char* buf)
{
	char*       text;
	char*       token;

	if (!buf)
	{
		return;
	}

	text = buf;
	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (!token[0])
		{
			break;
		}
		if (!Q_stricmp(token, "AddCvar"))
		{
			UI_GameinfoCfg_ParseAddCvarBlock(&text);
		}
	}
}

void UI_GameinfoCfg_Load(void)
{
	fileHandle_t    f;
	int             len;
	static char     fileBuf[UI_GAMEINFO_CFG_READ_MAX];

	ui_gameinfo_hasStatusServer1 = qfalse;
	ui_gameinfo_hasStatusServer2 = qfalse;
	ui_gameinfo_statusServer1[0] = '\0';
	ui_gameinfo_statusServer2[0] = '\0';

	len = trap_FS_FOpenFile(UI_GAMEINFO_CFG_FILE, &f, FS_READ);
	if (len <= 0)
	{
		if (f)
		{
			trap_FS_FCloseFile(f);
		}
		return;
	}

	if (len >= (int)sizeof(fileBuf))
	{
		len = sizeof(fileBuf) - 1;
	}

	trap_FS_Read(fileBuf, len, f);
	trap_FS_FCloseFile(f);
	fileBuf[len] = '\0';

	UI_GameinfoCfg_StripCommentLines(fileBuf);
	UI_GameinfoCfg_ParseBuffer(fileBuf);
}

const char* UI_GameinfoCfg_GetStatusServerPrimary(void)
{
	if (ui_gameinfo_hasStatusServer1)
	{
		return ui_gameinfo_statusServer1;
	}
	return UI_GAMEINFO_DEFAULT_STATUS_PRIMARY;
}

const char* UI_GameinfoCfg_GetStatusServerAlternate(void)
{
	if (ui_gameinfo_hasStatusServer2)
	{
		return ui_gameinfo_statusServer2;
	}
	return UI_GAMEINFO_DEFAULT_STATUS_ALTERNATE;
}
