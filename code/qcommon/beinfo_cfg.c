// Copyright (C) 1999-2000 Id Software, Inc.
//
#if defined(CGAME)
#include "../cgame/cg_local.h"
#elif defined(Q3UI) || defined(UI)
#include "../q3_ui/ui_local.h"
#else
#error beinfo_cfg.c requires CGAME or Q3UI/UI
#endif

#include "beinfo_cfg.h"

#define BEINFO_CFG_FILE               "beconfigs/beinfo.cfg"
#define BEINFO_CFG_READ_MAX           4096
#define BEINFO_CFG_STATUS_ADDR_MAX    64
#define BEINFO_TTF_FONTS_MAX          16

#define BEINFO_CFG_DEFAULT_STATUS_PRIMARY     "141.98.86.110:27888"
#define BEINFO_CFG_DEFAULT_STATUS_ALTERNATE   "83.69.233.52:27888"

#define BEINFO_CFG_KEY_STATUS_SERVER1   "be_statusServer1"
#define BEINFO_CFG_KEY_STATUS_SERVER2   "be_statusServer2"
#define BEINFO_CFG_KEY_BRIGHT_PREFIX    "brightcolor_"
#define BEINFO_CFG_KEY_FLASH_PREFIX     "flashcolor_"
#define BEINFO_CFG_KEY_KILLCAM_FONT     "interface_killcam_font"

static char     beinfo_statusServer1[BEINFO_CFG_STATUS_ADDR_MAX];
static char     beinfo_statusServer2[BEINFO_CFG_STATUS_ADDR_MAX];
static qboolean beinfo_hasStatusServer1;
static qboolean beinfo_hasStatusServer2;

#define BEINFO_TTF_SHADOW_OFF_MIN -0.25f
#define BEINFO_TTF_SHADOW_OFF_MAX  0.25f

typedef struct
{
	char     name[MAX_QPATH];
	float    scale;
	float    offset;
	float    shadowOffX;
	float    shadowOffY;
	qboolean hasShadowOffset;
} beinfoTtfFontTune_t;

static beinfoTtfFontTune_t beinfo_ttfFonts[BEINFO_TTF_FONTS_MAX];
static int                 beinfo_ttfFontCount;

#if defined(CGAME)
/* interface_killcam_font: a font name or index, resolved by cgame after the fonts load */
static char                  beinfo_killcamFont[MAX_QPATH];
static beinfoBrightColorFn_t beinfo_brightColorHandler;
static beinfoBrightColorFn_t beinfo_flashColorHandler;
#endif

static qboolean BeinfoCfg_IsCommentHash(const char* p)
{
	if (!p)
	{
		return qfalse;
	}
	return (p[0] == '#') ? qtrue : qfalse;
}

static qboolean BeinfoCfg_IsHexDigit(char c);

static qboolean BeinfoCfg_IsHexColorLength(int len)
{
	return (len == 3 || len == 4 || len == 6 || len == 8) ? qtrue : qfalse;
}

/* p points at '#': "#RGB", "#RGBA", "#RRGGBB" or "#RRGGBBAA" followed by end/space/';' */
static qboolean BeinfoCfg_IsHashHexColor(const char* p)
{
	int n;

	if (!p || p[0] != '#')
	{
		return qfalse;
	}
	n = 0;
	while (BeinfoCfg_IsHexDigit(p[1 + n]))
	{
		n++;
	}
	if (!BeinfoCfg_IsHexColorLength(n))
	{
		return qfalse;
	}
	switch (p[1 + n])
	{
		case '\0':
		case ' ':
		case '\t':
		case '\r':
		case ';':
			return qtrue;
		default:
			return qfalse;
	}
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
			/* " #RRGGBB" mid-line is a hex color value, not a comment */
			comment = lineBuf;
			while ((comment = strchr(comment, '#')) != NULL)
			{
				if ((comment == lineBuf || comment[-1] == ' ' || comment[-1] == '\t') &&
				        !BeinfoCfg_IsHashHexColor(comment))
				{
					*comment = '\0';
					break;
				}
				comment++;
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

static void BeinfoCfg_HexNibbleToFloat(char c, float* out)
{
	*out = (float)(BeinfoCfg_HexNibble(c) * 17) / 255.0f;
}

/*
 * Hex color: [#|0x]RGB, RGBA, RRGGBB or RRGGBBAA (alpha defaults to 1).
 * *isBare is set when there is no '#'/'0x' prefix and all digits are decimal,
 * i.e. the token could also be a plain number (see BeinfoCfg_ParseHexColorArg).
 */
static qboolean BeinfoCfg_ParseHexColorToken(const char* token, float* r, float* g, float* b, float* a,
        qboolean* isBare)
{
	const char* p;
	int         len;
	int         i;
	qboolean    prefixed;
	qboolean    allDecimal;
	char        buf[MAX_TOKEN_CHARS];

	if (!token || !r || !g || !b || !a)
	{
		return qfalse;
	}

	Q_strncpyz(buf, token, sizeof(buf));
	BeinfoCfg_TrimValue(buf);
	p = buf;
	prefixed = qfalse;
	if (p[0] == '#')
	{
		p++;
		prefixed = qtrue;
	}
	else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
	{
		p += 2;
		prefixed = qtrue;
	}
	len = (int)strlen(p);
	if (!BeinfoCfg_IsHexColorLength(len))
	{
		return qfalse;
	}
	allDecimal = qtrue;
	for (i = 0; p[i]; i++)
	{
		if (!BeinfoCfg_IsHexDigit(p[i]))
		{
			return qfalse;
		}
		if (p[i] < '0' || p[i] > '9')
		{
			allDecimal = qfalse;
		}
	}

	if (len <= 4)
	{
		BeinfoCfg_HexNibbleToFloat(p[0], r);
		BeinfoCfg_HexNibbleToFloat(p[1], g);
		BeinfoCfg_HexNibbleToFloat(p[2], b);
		if (len == 4)
		{
			BeinfoCfg_HexNibbleToFloat(p[3], a);
		}
		else
		{
			*a = 1.0f;
		}
	}
	else
	{
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
	}

	if (isBare)
	{
		*isBare = (!prefixed && allDecimal) ? qtrue : qfalse;
	}
	return qtrue;
}

/*
 * Parse token as a hex color. A bare all-decimal token ("100", "1000") that is
 * followed by another number on the same line is left for the "R G B A" form.
 * token must not point into COM_ParseExt's buffer (we peek the next token).
 */
static qboolean BeinfoCfg_ParseHexColorArg(const char* token, char** text, float* r, float* g, float* b, float* a)
{
	qboolean isBare;
	char*    save;
	char*    next;
	qboolean nextNumeric;

	if (!BeinfoCfg_ParseHexColorToken(token, r, g, b, a, &isBare))
	{
		return qfalse;
	}
	if (!isBare || !text || !*text)
	{
		return qtrue;
	}

	save = *text;
	next = COM_ParseExt(text, qfalse);
	nextNumeric = BeinfoCfg_TokenLooksNumeric(next);
	*text = save;
	return nextNumeric ? qfalse : qtrue;
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
	char    tokenBuf[MAX_TOKEN_CHARS];
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

	Q_strncpyz(tokenBuf, token, sizeof(tokenBuf));
	if (BeinfoCfg_ParseHexColorArg(tokenBuf, text, &skipR, &skipG, &skipB, &skipA))
	{
		return;
	}

	if (!BeinfoCfg_TokenLooksNumeric(tokenBuf))
	{
		BeinfoCfg_PendingPush(tokenBuf);
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
#endif

static qboolean BeinfoCfg_ReadStringValue(char** text, char* value, int valueSize)
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

#if defined(Q3UI) || defined(UI)
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

static void BeinfoCfg_ParseTtfFontEntry(char** text, const char* fontName)
{
	char*                token;
	char                 tokenBuf[MAX_TOKEN_CHARS];
	beinfoTtfFontTune_t* slot;
	float                scale;
	float                offset;
	float                shadowOffX;
	float                shadowOffY;
	qboolean             hasShadowOffset;

	if (!text || !*text || !fontName || !fontName[0])
	{
		return;
	}

	token = COM_ParseExt(text, qtrue);
	if (!token[0] || strcmp(token, "{"))
	{
		return;
	}

	if (beinfo_ttfFontCount >= BEINFO_TTF_FONTS_MAX)
	{
		/* Consume body; keep existing entries. */
		int depth = 1;
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
		return;
	}

	scale = 1.0f;
	offset = 0.0f;
	shadowOffX = 0.1f;
	shadowOffY = 0.1f;
	hasShadowOffset = qfalse;

	while (1)
	{
		if (beinfo_pendingToken[0])
		{
			Q_strncpyz(tokenBuf, beinfo_pendingToken, sizeof(tokenBuf));
			beinfo_pendingToken[0] = '\0';
			token = tokenBuf;
		}
		else
		{
			token = COM_ParseExt(text, qtrue);
			if (!token[0])
			{
				break;
			}
		}
		if (!strcmp(token, "}"))
		{
			break;
		}

		if (!Q_stricmp(token, "scale"))
		{
			token = COM_ParseExt(text, qtrue);
			if (!token[0] || !strcmp(token, "}") || !strcmp(token, "{"))
			{
				if (token[0] && (!strcmp(token, "}") || !strcmp(token, "{")))
				{
					BeinfoCfg_PendingPush(token);
				}
				break;
			}
			scale = (float)atof(token);
		}
		else if (!Q_stricmp(token, "offset"))
		{
			token = COM_ParseExt(text, qtrue);
			if (!token[0] || !strcmp(token, "}") || !strcmp(token, "{"))
			{
				if (token[0] && (!strcmp(token, "}") || !strcmp(token, "{")))
				{
					BeinfoCfg_PendingPush(token);
				}
				break;
			}
			offset = (float)atof(token);
		}
		else if (!Q_stricmp(token, "shadowOffset"))
		{
			token = COM_ParseExt(text, qtrue);
			if (!token[0] || !strcmp(token, "}") || !strcmp(token, "{"))
			{
				if (token[0] && (!strcmp(token, "}") || !strcmp(token, "{")))
				{
					BeinfoCfg_PendingPush(token);
				}
				break;
			}
			if (!BeinfoCfg_TokenLooksNumeric(token))
			{
				BeinfoCfg_PendingPush(token);
				continue;
			}
			shadowOffX = Com_Clamp(BEINFO_TTF_SHADOW_OFF_MIN, BEINFO_TTF_SHADOW_OFF_MAX,
			        (float)atof(token));
			shadowOffY = shadowOffX;
			token = COM_ParseExt(text, qtrue);
			if (token[0] && BeinfoCfg_TokenLooksNumeric(token))
			{
				shadowOffY = Com_Clamp(BEINFO_TTF_SHADOW_OFF_MIN, BEINFO_TTF_SHADOW_OFF_MAX,
				        (float)atof(token));
			}
			else if (token[0])
			{
				BeinfoCfg_PendingPush(token);
			}
			hasShadowOffset = qtrue;
		}
		else
		{
			/* Unknown key: skip one argument if present. */
			BeinfoCfg_SkipKeyArguments(text);
		}
	}

	slot = &beinfo_ttfFonts[beinfo_ttfFontCount];
	Q_strncpyz(slot->name, fontName, sizeof(slot->name));
	slot->scale = scale;
	slot->offset = offset;
	slot->shadowOffX = shadowOffX;
	slot->shadowOffY = shadowOffY;
	slot->hasShadowOffset = hasShadowOffset;
	beinfo_ttfFontCount++;
}

static void BeinfoCfg_ParseTtfFontsBlock(char** text)
{
	char* token;
	char  fontName[MAX_QPATH];

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

		Q_strncpyz(fontName, token, sizeof(fontName));
		BeinfoCfg_ParseTtfFontEntry(text, fontName);
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
			if (BeinfoCfg_ReadStringValue(text, value, sizeof(value)))
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
	char  tokenBuf[MAX_TOKEN_CHARS];

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

	Q_strncpyz(tokenBuf, token, sizeof(tokenBuf));
	BeinfoCfg_TrimValue(tokenBuf);
	if (BeinfoCfg_ParseHexColorArg(tokenBuf, text, r, g, b, a))
	{
		return qtrue;
	}

	if (!BeinfoCfg_TokenLooksNumeric(tokenBuf))
	{
		BeinfoCfg_PendingPush(tokenBuf);
		return qfalse;
	}

	*r = (float)atof(tokenBuf);
	if (!BeinfoCfg_ParseFloatToken(text, g) || !BeinfoCfg_ParseFloatToken(text, b))
	{
		return qfalse;
	}
	if (BeinfoCfg_ParseFloatToken(text, a))
	{
		return qtrue;
	}
	*a = 1.0f;
	return qtrue;
}

static void BeinfoCfg_HandleColorKey(const char* key, int prefixLen, beinfoBrightColorFn_t handler, char** text)
{
	const char* classname;
	float       r;
	float       g;
	float       b;
	float       a;

	classname = key + prefixLen;

	if (!BeinfoCfg_ParseBrightColorValues(text, &r, &g, &b, &a))
	{
		return;
	}

	if (!classname[0])
	{
		return;
	}

	if (handler)
	{
		handler(classname, r, g, b, a);
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
		BeinfoCfg_HandleColorKey(key, prefixLen, beinfo_brightColorHandler, text);
		return;
	}

	prefixLen = (int)strlen(BEINFO_CFG_KEY_FLASH_PREFIX);
	if (!Q_stricmpn(key, BEINFO_CFG_KEY_FLASH_PREFIX, prefixLen))
	{
		BeinfoCfg_HandleColorKey(key, prefixLen, beinfo_flashColorHandler, text);
		return;
	}

	if (!Q_stricmp(key, BEINFO_CFG_KEY_KILLCAM_FONT))
	{
		BeinfoCfg_ReadStringValue(text, beinfo_killcamFont, sizeof(beinfo_killcamFont));
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
		else if (!Q_stricmp(token, "ttfFonts"))
		{
			BeinfoCfg_ParseTtfFontsBlock(text);
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

void BeinfoCfg_SetFlashColorHandler(beinfoBrightColorFn_t fn)
{
	beinfo_flashColorHandler = fn;
}

const char* BeinfoCfg_GetKillcamFont(void)
{
	return beinfo_killcamFont;
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
	beinfo_ttfFontCount = 0;
	Com_Memset(beinfo_ttfFonts, 0, sizeof(beinfo_ttfFonts));
#if defined(CGAME)
	beinfo_killcamFont[0] = '\0';
#endif

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

qboolean BeinfoCfg_GetTtfFontTune(const char* name, float* scale, float* offset)
{
	int i;

	if (scale)
	{
		*scale = 1.0f;
	}
	if (offset)
	{
		*offset = 0.0f;
	}
	if (!name || !name[0] || !scale || !offset)
	{
		return qfalse;
	}

	for (i = 0; i < beinfo_ttfFontCount; i++)
	{
		if (!Q_stricmp(beinfo_ttfFonts[i].name, name))
		{
			*scale = beinfo_ttfFonts[i].scale;
			*offset = beinfo_ttfFonts[i].offset;
			return qtrue;
		}
	}
	return qfalse;
}

qboolean BeinfoCfg_GetTtfFontShadow(const char* name, float* offX, float* offY)
{
	int i;

	if (!name || !name[0] || !offX || !offY)
	{
		return qfalse;
	}

	for (i = 0; i < beinfo_ttfFontCount; i++)
	{
		if (!Q_stricmp(beinfo_ttfFonts[i].name, name))
		{
			if (!beinfo_ttfFonts[i].hasShadowOffset)
			{
				return qfalse;
			}
			*offX = beinfo_ttfFonts[i].shadowOffX;
			*offY = beinfo_ttfFonts[i].shadowOffY;
			return qtrue;
		}
	}
	return qfalse;
}
