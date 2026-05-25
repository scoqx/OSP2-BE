// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "beinfo_cfg.h"

#if defined(CGAME)
#include "../cgame/cg_local.h"
#elif defined(Q3UI) || defined(UI)
#include "../q3_ui/ui_local.h"
#else
#error beinfo_cfg.c requires CGAME or Q3UI/UI
#endif

#define BEINFO_CFG_FILE               "beconfigs/beinfo.cfg"
#define BEINFO_CFG_READ_MAX           4096
#define BEINFO_CFG_STATUS_ADDR_MAX    64

#define BEINFO_CFG_DEFAULT_STATUS_PRIMARY     "141.98.86.110:27888"
#define BEINFO_CFG_DEFAULT_STATUS_ALTERNATE   "83.69.233.52:27888"

#define BEINFO_CFG_KEY_STATUS_SERVER1   "be_statusServer1"
#define BEINFO_CFG_KEY_STATUS_SERVER2   "be_statusServer2"
#define BEINFO_CFG_KEY_BRIGHT_PREFIX    "brightcolor_"

static char     beinfo_statusServer1[BEINFO_CFG_STATUS_ADDR_MAX];
static char     beinfo_statusServer2[BEINFO_CFG_STATUS_ADDR_MAX];
static qboolean beinfo_hasStatusServer1;
static qboolean beinfo_hasStatusServer2;

#if defined(CGAME)
static beinfoBrightColorFn_t beinfo_brightColorHandler;
#endif

static qboolean BeinfoCfg_IsCommentHash(const char* p)
{
	if (!p)
	{
		return qfalse;
	}
	return (p[0] == '#') ? qtrue : qfalse;
}

static void BeinfoCfg_StripCommentLines(char* buf)
{
	char*       read;
	char*       write;
	char*       scan;
	char*       comment;
	char        lineBuf[512];
	int         lineLen;
	const char* eol;

	if (!buf)
	{
		return;
	}

	read = buf;
	write = buf;
	while (*read)
	{
		eol = strchr(read, '\n');
		if (eol)
		{
			lineLen = (int)(eol - read);
		}
		else
		{
			lineLen = (int)strlen(read);
		}

		if (lineLen >= (int)sizeof(lineBuf))
		{
			lineLen = (int)sizeof(lineBuf) - 1;
		}

		memcpy(lineBuf, read, lineLen);
		lineBuf[lineLen] = '\0';

		scan = lineBuf;
		while (*scan == ' ' || *scan == '\t')
		{
			scan++;
		}

		if (!BeinfoCfg_IsCommentHash(scan))
		{
			comment = strchr(lineBuf, '#');
			if (comment && (comment == lineBuf || comment[-1] == ' ' || comment[-1] == '\t'))
			{
				*comment = '\0';
			}

			lineLen = (int)strlen(lineBuf);
			if (lineLen > 0)
			{
				memcpy(write, lineBuf, lineLen);
				write += lineLen;
				if (eol)
				{
					*write++ = '\n';
				}
			}
			else if (eol)
			{
				*write++ = '\n';
			}
		}

		if (!eol)
		{
			break;
		}
		read = (char*)eol + 1;
	}

	*write = '\0';
}

static void BeinfoCfg_TrimValue(char* s)
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

static qboolean BeinfoCfg_TokenLooksNumeric(const char* s)
{
	int     i;
	qboolean hasDigit;

	if (!s || !s[0])
	{
		return qfalse;
	}
	if (!strcmp(s, "{") || !strcmp(s, "}"))
	{
		return qfalse;
	}

	hasDigit = qfalse;
	for (i = 0; s[i]; i++)
	{
		if (s[i] >= '0' && s[i] <= '9')
		{
			hasDigit = qtrue;
		}
		else if (s[i] == '#')
		{
			return qfalse;
		}
		else if (s[i] != '.' && s[i] != '-' && s[i] != '+')
		{
			return qfalse;
		}
	}

	return hasDigit;
}

static qboolean BeinfoCfg_IsHexDigit(char c)
{
	if (c >= '0' && c <= '9')
	{
		return qtrue;
	}
	if (c >= 'a' && c <= 'f')
	{
		return qtrue;
	}
	if (c >= 'A' && c <= 'F')
	{
		return qtrue;
	}
	return qfalse;
}

static int BeinfoCfg_HexNibble(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	if (c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	return 0;
}

static void BeinfoCfg_HexBytePairToFloat(const char* hex, float* out)
{
	int v;

	if (!hex || !out)
	{
		return;
	}
	v = BeinfoCfg_HexNibble(hex[0]) * 16 + BeinfoCfg_HexNibble(hex[1]);
	*out = (float)v / 255.0f;
}

static qboolean BeinfoCfg_ParseBrightColorHexToken(const char* token, float* r, float* g, float* b, float* a)
{
	const char* p;
	int         len;
	int         i;
	char        buf[MAX_TOKEN_CHARS];

	if (!token || !r || !g || !b || !a)
	{
		return qfalse;
	}

	Q_strncpyz(buf, token, sizeof(buf));
	BeinfoCfg_TrimValue(buf);
	p = buf;
	if (p[0] == '#')
	{
		return qfalse;
	}
	len = (int)strlen(p);
	if (len != 6 && len != 8)
	{
		return qfalse;
	}
	for (i = 0; p[i]; i++)
	{
		if (!BeinfoCfg_IsHexDigit(p[i]))
		{
			return qfalse;
		}
	}

	BeinfoCfg_HexBytePairToFloat(p + 0, r);
	BeinfoCfg_HexBytePairToFloat(p + 2, g);
	BeinfoCfg_HexBytePairToFloat(p + 4, b);
	if (len == 8)
	{
		BeinfoCfg_HexBytePairToFloat(p + 6, a);
	}
	else
	{
		*a = 1.0f;
	}

	return qtrue;
}

static char beinfo_pendingToken[MAX_TOKEN_CHARS];

static void BeinfoCfg_PendingPush(const char* token)
{
	if (!token || !token[0])
	{
		return;
	}

	Q_strncpyz(beinfo_pendingToken, token, sizeof(beinfo_pendingToken));
}

/*
 * Skip value(s) for an unknown or unsupported key (no errors, no logging).
 */
static void BeinfoCfg_SkipKeyArguments(char** text)
{
	char*   token;
	float   skipR;
	float   skipG;
	float   skipB;
	float   skipA;

	if (!text || !*text)
	{
		return;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0])
	{
		return;
	}
	if (!strcmp(token, "{") || !strcmp(token, "}"))
	{
		BeinfoCfg_PendingPush(token);
		return;
	}
	if (!strcmp(token, "="))
	{
		token = COM_ParseExt(text, qfalse);
		if (!token[0])
		{
			return;
		}
		if (!strcmp(token, "{") || !strcmp(token, "}"))
		{
			BeinfoCfg_PendingPush(token);
		}
		return;
	}

	if (BeinfoCfg_ParseBrightColorHexToken(token, &skipR, &skipG, &skipB, &skipA))
	{
		return;
	}

	if (!BeinfoCfg_TokenLooksNumeric(token))
	{
		BeinfoCfg_PendingPush(token);
		return;
	}

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			return;
		}
		if (!strcmp(token, "{") || !strcmp(token, "}"))
		{
			BeinfoCfg_PendingPush(token);
			return;
		}
		if (!BeinfoCfg_TokenLooksNumeric(token))
		{
			BeinfoCfg_PendingPush(token);
			return;
		}
	}
}

#if defined(Q3UI) || defined(UI)
static void BeinfoCfg_SetStatusServer(char* dest, int destSize, qboolean* hasFlag,
        const char* value)
{
	if (!dest || destSize <= 0 || !hasFlag || !value || !value[0])
	{
		return;
	}

	Q_strncpyz(dest, value, destSize);
	*hasFlag = qtrue;
}

static qboolean BeinfoCfg_UiKeyIsKnown(const char* key)
{
	if (!key || !key[0])
	{
		return qfalse;
	}

	if (!Q_stricmp(key, BEINFO_CFG_KEY_STATUS_SERVER1))
	{
		return qtrue;
	}
	if (!Q_stricmp(key, BEINFO_CFG_KEY_STATUS_SERVER2))
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean BeinfoCfg_ReadUiValue(char** text, char* value, int valueSize)
{
	char* token;

	if (!text || !value || valueSize <= 0)
	{
		return qfalse;
	}

	value[0] = '\0';

	token = COM_ParseExt(text, qtrue);
	if (!token[0])
	{
		return qfalse;
	}
	if (!strcmp(token, "{") || !strcmp(token, "}"))
	{
		BeinfoCfg_PendingPush(token);
		return qfalse;
	}
	if (token[0] == '=')
	{
		token = COM_ParseExt(text, qfalse);
		if (!token[0])
		{
			return qfalse;
		}
		if (!strcmp(token, "{") || !strcmp(token, "}"))
		{
			BeinfoCfg_PendingPush(token);
			return qfalse;
		}
	}

	Q_strncpyz(value, token, valueSize);
	BeinfoCfg_TrimValue(value);
	return qtrue;
}

static void BeinfoCfg_ApplyUiPair(const char* key, const char* value)
{
	if (!key || !key[0] || !value || !value[0])
	{
		return;
	}

	if (!Q_stricmp(key, BEINFO_CFG_KEY_STATUS_SERVER1))
	{
		BeinfoCfg_SetStatusServer(beinfo_statusServer1,
		        sizeof(beinfo_statusServer1), &beinfo_hasStatusServer1, value);
	}
	else if (!Q_stricmp(key, BEINFO_CFG_KEY_STATUS_SERVER2))
	{
		BeinfoCfg_SetStatusServer(beinfo_statusServer2,
		        sizeof(beinfo_statusServer2), &beinfo_hasStatusServer2, value);
	}
}
#endif

static void BeinfoCfg_SkipBracedBlock(char** text)
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

#if defined(Q3UI) || defined(UI)
static void BeinfoCfg_ParseUiBlock(char** text)
{
	char*       token;
	char        key[MAX_TOKEN_CHARS];
	char        value[MAX_TOKEN_CHARS];

	if (!text || !*text)
	{
		return;
	}

	beinfo_pendingToken[0] = '\0';

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	while (1)
	{
		if (beinfo_pendingToken[0])
		{
			Q_strncpyz(key, beinfo_pendingToken, sizeof(key));
			beinfo_pendingToken[0] = '\0';
		}
		else
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
		}

		if (BeinfoCfg_UiKeyIsKnown(key))
		{
			if (BeinfoCfg_ReadUiValue(text, value, sizeof(value)))
			{
				BeinfoCfg_ApplyUiPair(key, value);
			}
		}
		else
		{
			BeinfoCfg_SkipKeyArguments(text);
		}
	}
}
#endif

#if defined(CGAME)
static qboolean BeinfoCfg_ParseFloatToken(char** text, float* out)
{
	char* token;

	if (!text || !out)
	{
		return qfalse;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0])
	{
		return qfalse;
	}
	if (!strcmp(token, "{") || !strcmp(token, "}"))
	{
		BeinfoCfg_PendingPush(token);
		return qfalse;
	}

	BeinfoCfg_TrimValue(token);
	if (!BeinfoCfg_TokenLooksNumeric(token))
	{
		BeinfoCfg_PendingPush(token);
		return qfalse;
	}

	*out = (float)atof(token);
	return qtrue;
}

static qboolean BeinfoCfg_ParseBrightColorValues(char** text, float* r, float* g, float* b, float* a)
{
	char* token;

	if (!text || !r || !g || !b || !a)
	{
		return qfalse;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0])
	{
		return qfalse;
	}
	if (!strcmp(token, "{") || !strcmp(token, "}"))
	{
		BeinfoCfg_PendingPush(token);
		return qfalse;
	}

	BeinfoCfg_TrimValue(token);
	if (BeinfoCfg_ParseBrightColorHexToken(token, r, g, b, a))
	{
		return qtrue;
	}

	if (!BeinfoCfg_TokenLooksNumeric(token))
	{
		BeinfoCfg_PendingPush(token);
		return qfalse;
	}

	*r = (float)atof(token);
	return BeinfoCfg_ParseFloatToken(text, g) &&
	       BeinfoCfg_ParseFloatToken(text, b) &&
	       BeinfoCfg_ParseFloatToken(text, a);
}

static void BeinfoCfg_HandleBrightColorKey(const char* key, char** text)
{
	const char* classname;
	int         prefixLen;
	float       r;
	float       g;
	float       b;
	float       a;

	if (!key || !key[0] || !text)
	{
		return;
	}

	prefixLen = (int)strlen(BEINFO_CFG_KEY_BRIGHT_PREFIX);
	if (Q_stricmpn(key, BEINFO_CFG_KEY_BRIGHT_PREFIX, prefixLen))
	{
		return;
	}

	classname = key + prefixLen;

	if (!BeinfoCfg_ParseBrightColorValues(text, &r, &g, &b, &a))
	{
		return;
	}

	if (!classname[0])
	{
		return;
	}

	if (beinfo_brightColorHandler)
	{
		beinfo_brightColorHandler(classname, r, g, b, a);
	}
}

static void BeinfoCfg_ParseCgameEntry(const char* key, char** text)
{
	int prefixLen;

	if (!key || !key[0] || !text)
	{
		return;
	}

	prefixLen = (int)strlen(BEINFO_CFG_KEY_BRIGHT_PREFIX);
	if (!Q_stricmpn(key, BEINFO_CFG_KEY_BRIGHT_PREFIX, prefixLen))
	{
		BeinfoCfg_HandleBrightColorKey(key, text);
		return;
	}

	BeinfoCfg_SkipKeyArguments(text);
}

static void BeinfoCfg_ParseCgameBlock(char** text)
{
	char*       token;
	char        key[MAX_TOKEN_CHARS];

	if (!text || !*text)
	{
		return;
	}

	beinfo_pendingToken[0] = '\0';

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	while (1)
	{
		if (beinfo_pendingToken[0])
		{
			Q_strncpyz(key, beinfo_pendingToken, sizeof(key));
			beinfo_pendingToken[0] = '\0';
		}
		else
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
		}

		BeinfoCfg_ParseCgameEntry(key, text);
	}
}
#endif

static void BeinfoCfg_ParseAddVarBlock(char** text)
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
#if defined(Q3UI) || defined(UI)
			BeinfoCfg_ParseUiBlock(text);
#else
			BeinfoCfg_SkipBracedBlock(text);
#endif
		}
		else if (!Q_stricmp(token, "cgame"))
		{
#if defined(CGAME)
			BeinfoCfg_ParseCgameBlock(text);
#else
			BeinfoCfg_SkipBracedBlock(text);
#endif
		}
		else
		{
			BeinfoCfg_SkipBracedBlock(text);
		}
	}
}

static void BeinfoCfg_ParseBuffer(char* buf)
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
		if (!Q_stricmp(token, "AddVar"))
		{
			BeinfoCfg_ParseAddVarBlock(&text);
		}
	}
}

#if defined(CGAME)
void BeinfoCfg_SetBrightColorHandler(beinfoBrightColorFn_t fn)
{
	beinfo_brightColorHandler = fn;
}
#endif

void BeinfoCfg_Load(void)
{
	fileHandle_t    f;
	int             len;
	static char     fileBuf[BEINFO_CFG_READ_MAX];

	beinfo_hasStatusServer1 = qfalse;
	beinfo_hasStatusServer2 = qfalse;
	beinfo_statusServer1[0] = '\0';
	beinfo_statusServer2[0] = '\0';

	len = trap_FS_FOpenFile(BEINFO_CFG_FILE, &f, FS_READ);
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

	BeinfoCfg_StripCommentLines(fileBuf);
	BeinfoCfg_ParseBuffer(fileBuf);
}

const char* BeinfoCfg_GetStatusServerPrimary(void)
{
	if (beinfo_hasStatusServer1)
	{
		return beinfo_statusServer1;
	}
	return BEINFO_CFG_DEFAULT_STATUS_PRIMARY;
}

const char* BeinfoCfg_GetStatusServerAlternate(void)
{
	if (beinfo_hasStatusServer2)
	{
		return beinfo_statusServer2;
	}
	return BEINFO_CFG_DEFAULT_STATUS_ALTERNATE;
}
