/*
===========================================================================
engine_fonts.c - FreeType atlas + cl_fontList (cgame + ui)
===========================================================================
*/

#include "q_shared.h"
#include "engine_fonts.h"

/* Fixed bake tiers - pick smallest with lineHeight >= need. */
static const int eng_pointSizes[ENG_FONTS_SIZES] = { 8, 16, 32, 64 };

typedef struct
{
	fontAtlasInfo_t info;
	qboolean        valid;
	qboolean        tried;
} engFontSlot_t;

static EngFonts_Hooks eng_hooks;
static qboolean       eng_hooksSet;

static engFontSlot_t  eng_fonts[ENG_FONTS_MAX_TTF][ENG_FONTS_SIZES];
static int            eng_map[ENG_FONTS_FONT_INDEX_MAX]; /* fontIndex -> engSlot, -1 = none */
static int            eng_ttfCount;
static int            eng_registerCmd;
static qboolean       eng_available;
static char           eng_alphabet[256];
static unsigned int   eng_memoSerial;

static void EngFonts_BuildAlphabet(void)
{
	int i;
	int n;

	n = 0;
	for (i = 32; i < 256 && n < (int)sizeof(eng_alphabet) - 1; i++)
	{
		eng_alphabet[n++] = (char)i;
	}
	eng_alphabet[n] = '\0';
}

static void EngFonts_Print(const char* msg)
{
	if (eng_hooks.print && msg)
	{
		eng_hooks.print(msg);
	}
}

void EngFonts_SetHooks(const EngFonts_Hooks* hooks)
{
	if (!hooks)
	{
		Com_Memset(&eng_hooks, 0, sizeof(eng_hooks));
		eng_hooksSet = qfalse;
		return;
	}
	eng_hooks = *hooks;
	eng_hooksSet = qtrue;
}

void EngFonts_Invalidate(void)
{
	int i;

	Com_Memset(eng_fonts, 0, sizeof(eng_fonts));
	eng_ttfCount = 0;
	for (i = 0; i < ENG_FONTS_FONT_INDEX_MAX; i++)
	{
		eng_map[i] = -1;
	}
	eng_memoSerial++;
	eng_available = qfalse;
	eng_registerCmd = 0;
}

qboolean EngFonts_IsAvailable(void)
{
	return eng_available;
}

qboolean EngFonts_Probe(void)
{
	char value[MAX_CVAR_VALUE_STRING];

	eng_registerCmd = 0;
	eng_available = qfalse;
	EngFonts_BuildAlphabet();

	if (!eng_hooksSet || !eng_hooks.getValue)
	{
		return qfalse;
	}
	if (!eng_hooks.getValue("trap_R_RegisterFontAtlas_Q3E", value, sizeof(value)))
	{
		return qfalse;
	}
	eng_registerCmd = atoi(value);
	if (eng_registerCmd == 0)
	{
		return qfalse;
	}
	eng_available = qtrue;
	return qtrue;
}

static int EngFonts_RegisterAtlas(const char* face, int pointSize, fontAtlasInfo_t* out)
{
	if (!eng_available || !eng_hooks.registerAtlas || eng_registerCmd == 0)
	{
		return 0;
	}
	return eng_hooks.registerAtlas(eng_registerCmd, face, pointSize, eng_alphabet, out);
}

static qboolean EngFonts_LoadSlot(int engSlot, int sizeSlot, const char* ttfPath)
{
	engFontSlot_t* slot;
	int            pointSize;

	if (engSlot < 0 || engSlot >= ENG_FONTS_MAX_TTF)
	{
		return qfalse;
	}
	if (sizeSlot < 0 || sizeSlot >= ENG_FONTS_SIZES)
	{
		return qfalse;
	}
	if (!ttfPath || !ttfPath[0])
	{
		return qfalse;
	}

	slot = &eng_fonts[engSlot][sizeSlot];
	if (slot->valid)
	{
		return qtrue;
	}
	if (slot->tried)
	{
		return qfalse;
	}
	slot->tried = qtrue;

	pointSize = eng_pointSizes[sizeSlot];
	Com_Memset(&slot->info, 0, sizeof(slot->info));
	if (!EngFonts_RegisterAtlas(ttfPath, pointSize, &slot->info))
	{
		return qfalse;
	}
	if (slot->info.atlasCount <= 0 || slot->info.lineHeight < 1.0f)
	{
		return qfalse;
	}

	slot->valid = qtrue;
	return qtrue;
}

/*
================
EngFonts_FontCfgIsTtfEntry

fonts.cfg: "osp osp" or "osp osp.ttf" / "osp fonts/osp.ttf" -> TTF slot.
================
*/
qboolean EngFonts_FontCfgIsTtfEntry(const char* name, const char* path)
{
	char bareName[MAX_QPATH];
	char barePath[MAX_QPATH];

	if (!name || !path || !name[0] || !path[0])
	{
		return qfalse;
	}

	if (strstr(path, ".ttf") || strstr(path, ".TTF"))
	{
		return qtrue;
	}

	COM_StripExtension(name, bareName);
	COM_StripExtension(path, barePath);
	if (bareName[0] && barePath[0] && !Q_stricmp(bareName, barePath))
	{
		return qtrue;
	}
	return qfalse;
}

static void EngFonts_ListBuffer(char* out, int outSize)
{
	if (!out || outSize <= 0)
	{
		return;
	}
	out[0] = '\0';
	if (outSize > MAX_CVAR_VALUE_STRING)
	{
		outSize = MAX_CVAR_VALUE_STRING;
	}
	if (!eng_hooks.cvarBuffer)
	{
		return;
	}
	eng_hooks.cvarBuffer("cl_fontList", out, outSize);
}

static char* EngFonts_ListNext(char** p, char* out, int outSize)
{
	char* s;
	char* e;
	int len;
	int trim;

	if (!p || !*p || !out || outSize <= 0)
	{
		if (out && outSize > 0)
		{
			out[0] = '\0';
		}
		return NULL;
	}

	out[0] = '\0';

	for (;;)
	{
		s = *p;
		while (*s == ' ' || *s == '\t')
		{
			s++;
		}
		if (!*s)
		{
			*p = s;
			return NULL;
		}

		e = s;
		while (*e && *e != ',')
		{
			e++;
		}

		len = (int)(e - s);
		trim = len;
		while (trim > 0 && (s[trim - 1] == ' ' || s[trim - 1] == '\t'))
		{
			trim--;
		}

		*p = (*e == ',') ? e + 1 : e;

		if (trim >= outSize)
		{
			continue;
		}
		if (trim <= 0)
		{
			continue;
		}

		Com_Memcpy(out, s, trim);
		out[trim] = '\0';
		return out;
	}
}

static qboolean EngFonts_ListHasFace(const char* listBuf, const char* faceBare)
{
	char buf[MAX_CVAR_VALUE_STRING];
	char entry[MAX_QPATH];
	char bare[MAX_QPATH];
	char* p;

	if (!faceBare || !faceBare[0])
	{
		return qfalse;
	}

	if (listBuf && listBuf[0])
	{
		Q_strncpyz(buf, listBuf, sizeof(buf));
	}
	else
	{
		EngFonts_ListBuffer(buf, sizeof(buf));
	}
	if (!buf[0])
	{
		return qfalse;
	}

	p = buf;
	while (EngFonts_ListNext(&p, entry, sizeof(entry)))
	{
		COM_StripExtension(entry, bare);
		if (!Q_stricmp(bare, faceBare))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void EngFonts_MakeTtfPath(char* out, int outSize, const char* fontPath, const char* bare)
{
	if (strstr(fontPath, ".ttf") || strstr(fontPath, ".TTF"))
	{
		if (!Q_stricmpn(fontPath, "fonts/", 6))
		{
			Q_strncpyz(out, fontPath, outSize);
		}
		else
		{
			Com_sprintf(out, outSize, "fonts/%s", fontPath);
		}
	}
	else
	{
		Com_sprintf(out, outSize, "fonts/%s.ttf", bare);
	}
}

void EngFonts_AppendFaces(EngFonts_AddTtfFn addTtf, EngFonts_HasNameFn hasName, int maxFonts, int fontsNum)
{
	char listBuf[MAX_CVAR_VALUE_STRING];
	char cfgBuf[8192];
	char bare[MAX_QPATH];
	char fontName[MAX_QPATH];
	char fontPath[MAX_QPATH];
	char ttfPath[MAX_QPATH];
	char msg[256];
	fileHandle_t f;
	char* text;
	char* p;
	char* token;
	int engIdx;
	int len;
	int curFonts;

	if (!addTtf || !hasName || !eng_hooksSet)
	{
		return;
	}

	EngFonts_ListBuffer(listBuf, sizeof(listBuf));
	if (!listBuf[0])
	{
		return;
	}

	engIdx = 0;
	curFonts = fontsNum;

	/* Pass 1: ordered TTF from fonts.cfg */
	if (eng_hooks.fsOpen && eng_hooks.fsRead && eng_hooks.fsClose)
	{
		len = eng_hooks.fsOpen("fonts.cfg", &f, FS_READ);
		if (f != FS_INVALID_HANDLE)
		{
			if (len >= (int)sizeof(cfgBuf))
			{
				len = (int)sizeof(cfgBuf) - 1;
			}
			eng_hooks.fsRead(cfgBuf, len, f);
			eng_hooks.fsClose(f);
			cfgBuf[len] = '\0';

			text = cfgBuf;
			COM_BeginParseSession("fonts.cfg");
			while (engIdx < ENG_FONTS_MAX_TTF && curFonts < maxFonts)
			{
				token = COM_ParseExt(&text, qtrue);
				if (!token || !token[0])
				{
					break;
				}
				Q_strncpyz(fontName, token, sizeof(fontName));
				token = COM_ParseExt(&text, qfalse);
				if (!token || !token[0])
				{
					continue;
				}
				Q_strncpyz(fontPath, token, sizeof(fontPath));

				if (!EngFonts_FontCfgIsTtfEntry(fontName, fontPath))
				{
					continue;
				}

				COM_StripExtension(fontName, bare);
				if (!bare[0])
				{
					COM_StripExtension(fontPath, bare);
				}
				if (!EngFonts_ListHasFace(listBuf, bare))
				{
					Com_sprintf(msg, sizeof(msg),
					            S_COLOR_YELLOW "EngFonts_AppendFaces: TTF '%s' not in cl_fontList, skipping\n",
					            fontName);
					EngFonts_Print(msg);
					continue;
				}

				if (hasName(bare))
				{
					continue;
				}

				EngFonts_MakeTtfPath(ttfPath, sizeof(ttfPath), fontPath, bare);
				if (!addTtf(bare, ttfPath))
				{
					break;
				}
				Com_sprintf(msg, sizeof(msg), "TTF font '%s' reserved from fonts.cfg (index %d)\n", bare, curFonts);
				EngFonts_Print(msg);
				curFonts++;
				engIdx++;
			}
		}
	}

	/* Pass 2: remaining engine faces (comma-separated cl_fontList) */
	p = listBuf;
	while (engIdx < ENG_FONTS_MAX_TTF && curFonts < maxFonts)
	{
		if (!EngFonts_ListNext(&p, fontPath, sizeof(fontPath)))
		{
			break;
		}

		COM_StripExtension(fontPath, bare);
		if (!bare[0])
		{
			continue;
		}

		if (hasName(bare))
		{
			continue;
		}

		EngFonts_MakeTtfPath(ttfPath, sizeof(ttfPath), fontPath, bare);
		if (!addTtf(bare, ttfPath))
		{
			break;
		}
		curFonts++;
		engIdx++;
	}
}

void EngFonts_Preload(int fontCount, EngFonts_IsTtfFn isTtf, EngFonts_TtfPathFn ttfPath)
{
	int         i;
	int         s;
	int         count;
	int         engSlot;
	const char* path;
	char        msg[128];

	for (i = 0; i < ENG_FONTS_FONT_INDEX_MAX; i++)
	{
		eng_map[i] = -1;
	}
	eng_ttfCount = 0;
	Com_Memset(eng_fonts, 0, sizeof(eng_fonts));
	eng_memoSerial++;

	if (!eng_available || !isTtf || !ttfPath)
	{
		return;
	}

	count = fontCount;
	if (count > ENG_FONTS_FONT_INDEX_MAX)
	{
		count = ENG_FONTS_FONT_INDEX_MAX;
	}

	for (i = 0; i < count; i++)
	{
		if (!isTtf(i))
		{
			continue;
		}
		if (eng_ttfCount >= ENG_FONTS_MAX_TTF)
		{
			Com_sprintf(msg, sizeof(msg),
			            S_COLOR_YELLOW "EngFonts_Preload: TTF slot table full (%d)\n", ENG_FONTS_MAX_TTF);
			EngFonts_Print(msg);
			break;
		}

		path = ttfPath(i);
		if (!path || !path[0])
		{
			continue;
		}

		engSlot = eng_ttfCount++;
		eng_map[i] = engSlot;

		for (s = 0; s < ENG_FONTS_SIZES; s++)
		{
			EngFonts_LoadSlot(engSlot, s, path);
		}
	}
}

const fontAtlasInfo_t* EngFonts_Info(int fontIndex, float charHeight)
{
	static unsigned int           memoSerial;
	static int                    memoFontIndex = -1;
	static int                    memoNeed;
	static const fontAtlasInfo_t* memoInfo;

	int   engSlot;
	int   s;
	int   start;
	int   lastValid;
	int   needI;
	float need;
	float approxLine;
	const fontAtlasInfo_t* info;

	if (!eng_available)
	{
		return NULL;
	}
	if (fontIndex < 0 || fontIndex >= ENG_FONTS_FONT_INDEX_MAX)
	{
		return NULL;
	}

	engSlot = eng_map[fontIndex];
	if (engSlot < 0 || engSlot >= eng_ttfCount)
	{
		return NULL;
	}

	need = charHeight;
	if (need < 1.0f)
	{
		need = 1.0f;
	}
	needI = (int)need;

	if (memoSerial == eng_memoSerial && memoFontIndex == fontIndex && memoNeed == needI)
	{
		return memoInfo;
	}

	start = ENG_FONTS_SIZES - 1;
	for (s = 0; s < ENG_FONTS_SIZES; s++)
	{
		approxLine = (float)eng_pointSizes[s] * 2.0f;
		if (approxLine >= need)
		{
			start = s;
			break;
		}
	}

	lastValid = -1;
	info = NULL;
	for (s = start; s < ENG_FONTS_SIZES; s++)
	{
		if (!eng_fonts[engSlot][s].valid)
		{
			continue;
		}
		lastValid = s;
		if (eng_fonts[engSlot][s].info.lineHeight >= need)
		{
			info = &eng_fonts[engSlot][s].info;
			break;
		}
	}
	if (!info)
	{
		for (s = start - 1; s >= 0; s--)
		{
			if (eng_fonts[engSlot][s].valid)
			{
				info = &eng_fonts[engSlot][s].info;
				break;
			}
		}
	}
	if (!info && lastValid >= 0)
	{
		info = &eng_fonts[engSlot][lastValid].info;
	}

	memoSerial = eng_memoSerial;
	memoFontIndex = fontIndex;
	memoNeed = needI;
	memoInfo = info;
	return info;
}
