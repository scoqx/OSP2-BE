/*
===========================================================================
UI OSP Draw Tools - Font rendering and text drawing functions
Adapted from cgame/cg_drawtools.c
===========================================================================
*/

#include "ui_local.h"
#include "../qcommon/osp_fonts.h"
#include "../qcommon/engine_fonts.h"
#include "../qcommon/be_font_shadow.h"
#include "../qcommon/beinfo_cfg.h"

/* Memory allocation - use static buffers for C89 compatibility */
static text_command_t ui_compileTextBuffer[OSP_TEXT_CMD_MAX];
static char ui_compileTextStringBuffer[8192];
static text_command_t ui_compileTextResult[OSP_TEXT_CMD_MAX];

/* Font structures */
#define MAX_FONT_SHADERS 4

typedef struct
{
	float tc_prop[4];
	float tc_mono[4];
	float space1;
	float space2;
	float width;
} font_metric_t;

typedef struct
{
	const char* name;
	font_metric_t   metrics[256];
	qhandle_t       shader[ MAX_FONT_SHADERS ];
	int             shaderThreshold[ MAX_FONT_SHADERS ];
	int             shaderCount;
	qboolean        isTtf;
	char            ttfPath[MAX_QPATH];
} font_t;

#define MAX_FONTS 48
static font_t fonts[MAX_FONTS];
static char customFontNames[MAX_FONTS - OSP_BUILTIN_FONT_COUNT][MAX_QPATH];
static char engTtfNames[ENG_FONTS_MAX_TTF][MAX_QPATH];
static int fonts_num = OSP_BUILTIN_FONT_COUNT;
static int bitmap_fonts_num = OSP_BUILTIN_FONT_COUNT;
static int ui_engTtfNameIdx;
static const font_t* font = &fonts[0];
static const font_metric_t* metrics = &fonts[0].metrics[0];
static int ui_selectedFontIndex;

/* Engine atlas bind for measure/draw */
static const fontAtlasInfo_t* ui_ospstr_eng;
static float                  ui_ospstr_engUnit;
static float                  ui_ospstr_engDrawUnit; /* unit * scale — ink + prop advance */
static float                  ui_ospstr_engYBias; /* scale-center + offset * ah */
static float                  ui_ospstr_engAsc;

/* Resolved TTF tune by fontIndex (beInfo override or cvars). */
static float ui_ospstr_ttfTuneScale[ENG_FONTS_FONT_INDEX_MAX];
static float ui_ospstr_ttfTuneOffset[ENG_FONTS_FONT_INDEX_MAX];
static float ui_ospstr_ttfShadowOffX[ENG_FONTS_FONT_INDEX_MAX];
static float ui_ospstr_ttfShadowOffY[ENG_FONTS_FONT_INDEX_MAX];
static int   ui_ospstr_ttfTuneScaleMod = -1;
static int   ui_ospstr_ttfTuneOffMod = -1;
static int   ui_ospstr_ttfShadowMod = -1;
static int   ui_ospstr_ttfTuneFontCount = -1;
/* Current font shadow (set in BindEngine for TTF hotpath). */
static float ui_ospstr_curTtfShadowX = 0.1f;
static float ui_ospstr_curTtfShadowY = 0.1f;

static void UI_Ospstr_EnsureTtfTune(void)
{
	int i;
	int n;

	if (ui_ospstr_ttfTuneScaleMod == be_ttfScale.modificationCount &&
	        ui_ospstr_ttfTuneOffMod == be_ttfFontOffset.modificationCount &&
	        ui_ospstr_ttfShadowMod == be_ttfShadowOffset.modificationCount &&
	        ui_ospstr_ttfTuneFontCount == fonts_num)
	{
		return;
	}

	BeFontShadow_UpdateTtf(be_ttfShadowOffset.string);

	n = fonts_num;
	if (n > ENG_FONTS_FONT_INDEX_MAX)
	{
		n = ENG_FONTS_FONT_INDEX_MAX;
	}

	for (i = 0; i < n; i++)
	{
		float       s;
		float       off;
		float       shX;
		float       shY;
		const char* name;

		s = be_ttfScale.value;
		off = be_ttfFontOffset.value;
		shX = be_ttfShadowOffX;
		shY = be_ttfShadowOffY;
		name = fonts[i].name;
		if (fonts[i].isTtf && name && name[0])
		{
			float tuneS;
			float tuneOff;
			float tuneShX;
			float tuneShY;

			if (BeinfoCfg_GetTtfFontTune(name, &tuneS, &tuneOff))
			{
				s = tuneS;
				off = tuneOff;
			}
			if (BeinfoCfg_GetTtfFontShadow(name, &tuneShX, &tuneShY))
			{
				shX = tuneShX;
				shY = tuneShY;
			}
		}
		ui_ospstr_ttfTuneScale[i] = Com_Clamp(0.25f, 2.0f, s);
		ui_ospstr_ttfTuneOffset[i] = Com_Clamp(-1.0f, 1.0f, off);
		ui_ospstr_ttfShadowOffX[i] = shX;
		ui_ospstr_ttfShadowOffY[i] = shY;
	}

	ui_ospstr_ttfTuneScaleMod = be_ttfScale.modificationCount;
	ui_ospstr_ttfTuneOffMod = be_ttfFontOffset.modificationCount;
	ui_ospstr_ttfShadowMod = be_ttfShadowOffset.modificationCount;
	ui_ospstr_ttfTuneFontCount = fonts_num;
}

static float UI_Ospstr_GlyphAdvance(unsigned char ch, float aw, int proportional);
static void UI_Ospstr_BindEngine(int fontIndex, float aw, float ah, int proportional);

/* Border sizes */
vec4_t defaultBorderSize = {1, 1, 1, 1};
vec4_t thicBorderSize = {2, 2, 2, 2};

/* Helper functions */
static qboolean UI_OSPCharHexToInt(char c, int* out)
{
	if (c >= '0' && c <= '9')
	{
		*out = c - '0';
		return qtrue;
	}
	if (c >= 'a' && c <= 'f')
	{
		*out = c - 'a' + 10;
		return qtrue;
	}
	if (c >= 'A' && c <= 'F')
	{
		*out = c - 'A' + 10;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Hex16GetColor(const char* str, float* color)
{
	int d1;
	int d2;
	int color_int;
	if (!str) return qfalse;

	if (!UI_OSPCharHexToInt(str[0], &d1))
	{
		return  qfalse;
	}

	if (!UI_OSPCharHexToInt(str[1], &d2))
	{
		return  qfalse;
	}

	color_int = d1 * 16 + d2;

	*color = (float)color_int;
	*color /= 255.0f;

	return qtrue;
}

static qboolean UI_FileExist(const char* file)
{
	fileHandle_t    f;

	if (!file || !file[0])
		return qfalse;

	trap_FS_FOpenFile(file, &f, FS_READ);
	if (f == FS_INVALID_HANDLE)
		return qfalse;
	else
	{
		trap_FS_FCloseFile(f);
		return qtrue;
	}
}

static void UI_OSPDrawStringPrepare(const char* from, char* to, int size)
{
	int printed;
	int max;

	printed = 0;
	max = size - 1;

	if (!from || !to)
	{
		return;
	}

	while (*from && printed < max)
	{
		if (from[0] == '^' && from[1] != '^')
		{
			switch (from[1])
			{
				case 'f':
					from += 2; /*skip ^f */
					if ((uis.realtime & 0x3ff) >= 512)
					{
						while (*from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'F')))
						{
							++from;
						}
					}
					break;
				case 'F':
					from += 2; /*skip ^F */
					if ((uis.realtime & 0x3ff) < 512)
					{
						while (*from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'f')))
						{
							++from;
						}
					}
					break;
				default:
					break;
			}
		}

		*to = *from;
		++to;
		++from;
		++printed;
	}
	*to = 0; /*eos */
}

text_command_t* UI_CompileText(const char* in)
{
	int b;
	text_command_t* commands;
	text_command_t* result;
	char* text;
#ifdef __GNUC__
	__attribute__((unused))
#endif
	char* dmem;
	int i;
	int len;
	vec4_t back_color;
	qboolean back_color_was_set;
	qboolean top_color_was_set;
	float rc, gc, bc;
	unsigned int color_index;

	if (!in || *in == 0)
	{
		return NULL;
	}

	Vector4Copy(colorWhite, back_color);
	back_color_was_set = qfalse;
	top_color_was_set = qfalse;

	commands = ui_compileTextBuffer;

	{
		const char* p;
		p = in;
		len = 0;
		while (*p)
		{
			len++;
			p++;
		}
		len++;
	}
	if (len >= sizeof(ui_compileTextStringBuffer))
	{
		len = sizeof(ui_compileTextStringBuffer) - 1;
	}
	text = ui_compileTextStringBuffer;

	UI_OSPDrawStringPrepare(in, text, len);

	i = 0;
	while (*text)
	{
		if (text[0] == '^' && text[1])
		{
			switch (text[1])
			{
				case 'F':
				case 'f':
					/*skip ^F and ^f */
					text += 2;
					break;
				case 'B':
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						++i;
					}
					b = uis.realtime & 0x7ff;
					if (b > 1024)
					{
						b = ~b & 0x7ff;
					}
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade =  b / 1463.0f + 0.3f;
					++i;
					text += 2;
					break;
				case 'b':
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						++i;
					}
					b = uis.realtime & 0x7ff;
					if (b > 1024)
					{
						b = ~b & 0x7ff;
					}
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade = b / 1024.0f;
					++i;
					text += 2;
					break;
				case 'N':
				case 'n':
					/*reset fade */
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade = 1.0f;
					++i;

					/*reset color */
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						++i;
					}
					else if (top_color_was_set && !back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(colorWhite, commands[i].value.color);
						++i;
					}

					text += 2;
					break;
				case '^':
					commands[i].type = OSP_TEXT_CMD_CHAR;
					commands[i].value.character = '^';
					++i;
					text += 1;
					break;
				case 'X':
				case 'x':
					if (!UI_Hex16GetColor(&text[2], &rc))
					{
						text += 2;
						break;
					}
					if (!UI_Hex16GetColor(&text[4], &gc))
					{
						text += 2;
						break;
					}
					if (!UI_Hex16GetColor(&text[6], &bc))
					{
						text += 2;
						break;
					}
					back_color[0] = rc;
					back_color[1] = gc;
					back_color[2] = bc;

					commands[i].type = OSP_TEXT_CMD_SHADOW_COLOR;
					VectorCopy(back_color, commands[i].value.color);
					back_color_was_set = qtrue;

					++i;
					text += 8;
					break;
				default:
					if ((text[1] >= '0') && (text[1] <= '9'))
					{
						color_index = text[1] - 0x30;
						VectorCopy(g_color_table[color_index], commands[i].value.color);
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						++i;
						top_color_was_set = qtrue;
					}
					text += 2;
					break;
			}
		}
		else
		{
			commands[i].type = OSP_TEXT_CMD_CHAR;
			commands[i].value.character = text[0];
			++i;
			++text;
		}
	}
	commands[i++].type = OSP_TEXT_CMD_STOP;

	if (i >= OSP_TEXT_CMD_MAX)
	{
		return NULL;
	}

	/* Use static buffer instead of malloc */
	result = ui_compileTextResult;
	{
		int j;
		for (j = 0; j < i; j++)
		{
			result[j] = commands[j];
		}
	}

	return result;
}

void UI_CompiledTextDestroy(text_command_t* root)
{
	/* No-op: using static buffer, no need to free */
	(void)root;
}

/* Font loading functions */
static void UI_InitFontNames(void)
{
	int i;

	for (i = 0; i < OSP_BUILTIN_FONT_COUNT; i++)
	{
		fonts[i].name = ospBuiltinFonts[i].name;
	}
}

qboolean UI_FontAvailable(int index)
{
	if (index >= 0 && index < fonts_num)
	{
		return qtrue;
	}
	trap_Print(va("Nonexistent font number: ^1%d\n", index));
	return qfalse;
}

qboolean UI_FontLoaded(int index)
{
	if (index < 0 || index >= fonts_num)
	{
		return qfalse;
	}

	if (fonts[index].isTtf)
	{
		return (uis.engineFontAtlas && EngFonts_Info(index, 16.0f) != NULL) ? qtrue : qfalse;
	}

	/* Loaded fonts always have at least one registered shader image. */
	if (fonts[index].shaderCount > 0)
	{
		return qtrue;
	}

	return qfalse;
}

void UI_FontSelect(int index)
{
	if (UI_FontAvailable(index))
	{
		if (index < 0 || index >= fonts_num)
		{
			trap_Error(va("Requested nonexistent font number: %d\n", index));
		}

		font = &fonts[index];
		metrics = &font->metrics[0];
		ui_selectedFontIndex = index;
	}
}

int UI_FontIndexFromName(const char* name)
{
	int i;

	if (!name || !name[0])
	{
		return -1;
	}

	for (i = 0; i < fonts_num; i++)
	{
		if (Q_stricmp(fonts[i].name, name) == 0)
		{
			return i;
		}
	}

	return -1;
}

int UI_GetFontCount(void)
{
	return fonts_num;
}

int UI_FontIndexFromCvar(vmCvar_t* cvar)
{
	const char* str;
	int i;
	int fontIndex;
	qboolean isNumber;

	if (!cvar || cvar->string[0] == '\0')
	{
		return 0;
	}

	str = cvar->string;
	isNumber = qtrue;

	/* Check if string is a number (starts with digit or minus sign, followed by digits) */
	if (str[0] == '-' || str[0] == '+')
	{
		if (str[1] == '\0' || str[1] < '0' || str[1] > '9')
		{
			isNumber = qfalse;
		}
		else
		{
			for (i = 2; str[i] != '\0'; i++)
			{
				if (str[i] < '0' || str[i] > '9')
				{
					isNumber = qfalse;
					break;
				}
			}
		}
	}
	else if (str[0] >= '0' && str[0] <= '9')
	{
		for (i = 1; str[i] != '\0'; i++)
		{
			if (str[i] < '0' || str[i] > '9')
			{
				isNumber = qfalse;
				break;
			}
		}
	}
	else
	{
		isNumber = qfalse;
	}

	if (isNumber)
	{
		fontIndex = 0;
		i = 0;
		if (str[0] == '-' || str[0] == '+')
		{
			i = 1;
		}
		while (str[i] >= '0' && str[i] <= '9')
		{
			fontIndex = fontIndex * 10 + (str[i] - '0');
			i++;
		}
		if (str[0] == '-')
		{
			fontIndex = -fontIndex;
		}
		if (fontIndex < 0 || fontIndex >= fonts_num)
		{
			return 0;
		}
		return fontIndex;
	}
	else
	{
		fontIndex = UI_FontIndexFromName(str);
		if (fontIndex < 0)
		{
			return 0;
		}
		return fontIndex;
	}
}

const char* UI_GetFontName(int index)
{
	if (index >= 0 && index < fonts_num)
	{
		return fonts[index].name;
	}
	return NULL;
}

void UI_Fonts_f(void)
{
	int i;
	int fontCount;
	const char* fontName;

	fontCount = UI_GetFontCount();
	Com_Printf("^7Registered fonts:\n");
	for (i = 0; i < fontCount; i++)
	{
		fontName = UI_GetFontName(i);
		if (fontName)
		{
			Com_Printf("^3%2d^7. %s\n", i, fontName);
		}
	}
}

/* 1-based line and column for parse cursor (column = byte index on current line, excluding CR) */
static void UI_FontParseLineColumn(const char* bufStart, const char* parseCursor, int* outLine, int* outCol)
{
	const char* p;

	*outLine = 1;
	*outCol = 1;
	if (!bufStart)
	{
		*outLine = 0;
		*outCol = 0;
		return;
	}
	if (!parseCursor)
	{
		parseCursor = bufStart + strlen(bufStart);
	}
	for (p = bufStart; p < parseCursor; p++)
	{
		if (*p == '\n')
		{
			(*outLine)++;
			*outCol = 1;
		}
		else if (*p != '\r')
		{
			(*outCol)++;
		}
	}
}

static void UI_LoadFont(font_t* fnt, const char* fontName)
{
	char buf[ 8000 ];
	fileHandle_t f;
	char* token;
	char* text;
	float width;
	float height;
	float r_width;
	float r_height;
	float char_width;
	float char_height;
	char shaderName[ MAX_FONT_SHADERS ][ MAX_QPATH ];
	char tmpName[ MAX_QPATH ];
	int shaderCount;
	int shaderThreshold[ MAX_FONT_SHADERS ];
	font_metric_t* fm;
	int i;
	int tmp;
	int len;
	int chars;
	float w1;
	float w2;
	float s1;
	float s2;
	float x0;
	float y0;
	qboolean swapped;
	const char* bufBase;
	int parseLine;
	int parseCol;

	len = trap_FS_FOpenFile(fontName, &f, FS_READ);
	if (f == FS_INVALID_HANDLE)
	{
		trap_Print(va(S_COLOR_YELLOW "UI_LoadFont: error opening %s\n", fontName));
		return;
	}

	if (len >= sizeof(buf))
	{
		trap_Print(va(S_COLOR_YELLOW "UI_LoadFont: font file is too long: %i\n", len));
		len = sizeof(buf) - 1;
	}

	trap_FS_Read(buf, len, f);
	trap_FS_FCloseFile(f);
	buf[ len ] = '\0';

	bufBase = (const char*)buf;

	shaderCount = 0;

	text = buf; /* initialize parser */
	COM_BeginParseSession(fontName);

	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (token[0] == '\0')
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			Com_Printf(S_COLOR_RED "UI_LoadFont: parse error in %s at line %i column %i.\n", fontName, parseLine, parseCol);
			return;
		}

		/* font image */
		if (Q_stricmp(token, "img") == 0)
		{
			if (shaderCount >= MAX_FONT_SHADERS)
			{
				Com_Printf("UI_LoadFont: too many font images, ignoring.\n");
				SkipRestOfLine(&text);
				continue;
			}
			token = COM_ParseExt(&text, qfalse);
			if (!UI_FileExist(token))
			{
				UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
				Com_Printf("UI_LoadFont: font image '%s' doesn't exist (%s line %i column %i).\n", token, fontName, parseLine, parseCol);
				return;
			}
			/* save shader name */
			Q_strncpyz(shaderName[ shaderCount ], token, sizeof(shaderName[ shaderCount ]));
			/* get threshold */
			token = COM_ParseExt(&text, qfalse);
			shaderThreshold[ shaderCount ] = atoi(token);

			shaderCount++;

			SkipRestOfLine(&text);
			continue;
		}

		/* font parameters */
		if (Q_stricmp(token, "fnt") == 0)
		{
			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0' || (width = atof(token)) <= 0.0)
			{
				UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
				Com_Printf("UI_LoadFont: error reading image width in %s at line %i column %i.\n", fontName, parseLine, parseCol);
				return;
			}
			r_width = 1.0 / width;

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0' || (height = atof(token)) <= 0.0)
			{
				UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
				Com_Printf("UI_LoadFont: error reading image height in %s at line %i column %i.\n", fontName, parseLine, parseCol);
				return;
			}
			r_height = 1.0 / height;

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0')
			{
				UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
				Com_Printf("UI_LoadFont: error reading char width in %s at line %i column %i.\n", fontName, parseLine, parseCol);
				return;
			}
			char_width = atof(token);

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0')
			{
				UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
				Com_Printf("UI_LoadFont: error reading char height in %s at line %i column %i.\n", fontName, parseLine, parseCol);
				return;
			}
			char_height = atof(token);

			break; /* parse char metrics */
		}
	}

	if (shaderCount == 0)
	{
		UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
		Com_Printf("UI_LoadFont: no font images specified in %s at line %i column %i.\n", fontName, parseLine, parseCol);
		return;
	}

	fm = fnt->metrics;

	chars = 0;
	for (;;)
	{
		int validIndexToken;

		/* char index */
		token = COM_ParseExt(&text, qtrue);
		if (!token[0])
			break;

		validIndexToken = qtrue;
		if (token[0] == '\'' && token[1] && token[2] == '\'' && token[3] == '\0')   /* char code in form 'X' */
			i = token[1] & 255;
		else
		{
			int t;
			t = 0;
			if (token[t] == '-' || token[t] == '+')
			{
				t++;
			}
			if (token[t] == '\0')
			{
				validIndexToken = qfalse;
			}
			else
			{
				while (token[t] != '\0')
				{
					if (token[t] < '0' || token[t] > '9')
					{
						validIndexToken = qfalse;
						break;
					}
					t++;
				}
			}

			if (!validIndexToken)
			{
				SkipRestOfLine(&text);
				continue;
			}

			/* integer code */
			i = atoi(token);
		}

		if (i < 0 || i > 255)
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: bad char index %i in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		fm = fnt->metrics + i;

		/* x0 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_YELLOW "UI_LoadFont: skipping incomplete char metric line in %s at line %i column %i.\n", fontName, parseLine, parseCol));
			break;
		}
		x0 = atof(token);

		/* y0 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading y0 (glyph index %i) in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		y0 = atof(token);

		/* w1-offset */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading x-offset (glyph index %i) in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		w1 = atof(token);

		/* w2-offset */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading x-length (glyph index %i) in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		w2 = atof(token);

		/* space1 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading space1 (glyph index %i) in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		s1 = atof(token);

		/* space2 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			UI_FontParseLineColumn(bufBase, (const char*)text, &parseLine, &parseCol);
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading space2 (glyph index %i) in %s at line %i column %i.\n", i, fontName, parseLine, parseCol));
			return;
		}
		s2 = atof(token);

		fm->tc_mono[0] = x0 * r_width;
		fm->tc_mono[1] = y0 * r_height;
		fm->tc_mono[2] = (x0 + char_width) * r_width;
		fm->tc_mono[3] = (y0 + char_height) * r_height;

		/* proportional y-coords is matching with mono */
		fm->tc_prop[1] = fm->tc_mono[1];
		fm->tc_prop[3] = fm->tc_mono[3];

		fm->width = w2 / char_width;
		fm->space1 = s1 / char_width;
		fm->space2 = (s2 + w2) / char_width;
		fm->tc_prop[0] = fm->tc_mono[0] + (w1 * r_width);
		fm->tc_prop[2] = fm->tc_prop[0] + (w2 * r_width);

		chars++;

		SkipRestOfLine(&text);
	}

	/* sort images by threshold */
	do
	{
		swapped = qfalse;
		for (i = 1 ; i < shaderCount; i++)
		{
			if (shaderThreshold[i - 1] > shaderThreshold[i])
			{
				tmp = shaderThreshold[i - 1];
				shaderThreshold[i - 1] = shaderThreshold[i];
				shaderThreshold[i] = tmp;
				Q_strncpyz(tmpName, shaderName[i - 1], sizeof(tmpName));
				Q_strncpyz(shaderName[i - 1], shaderName[i], sizeof(shaderName[i - 1]));
				Q_strncpyz(shaderName[i], tmpName, sizeof(shaderName[i]));
				swapped = qtrue;
			}
		}
	}
	while (swapped);

	/* always assume zero threshold for lowest-quality shader */
	shaderThreshold[0] = 0;

	fnt->shaderCount = shaderCount;
	for (i = 0; i < shaderCount; i++)
	{
		fnt->shader[i] = trap_R_RegisterShaderNoMip(shaderName[i]);
		fnt->shaderThreshold[i] = shaderThreshold[i];
	}

	// trap_Print(va("Font '%s' loaded with %i chars and %i images\n", fontName, chars, shaderCount));
}

static void UI_LoadCustomFonts(void)
{
	char buf[8192];
	fileHandle_t f;
	char* token;
	char* text;
	int len;
	char fontName[MAX_QPATH];
	char fontPath[MAX_QPATH];
	int i;
	int customFontIndex;

	len = trap_FS_FOpenFile("fonts.cfg", &f, FS_READ);
	if (f == FS_INVALID_HANDLE)
	{
		return;
	}

	if (len >= sizeof(buf))
	{
		trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: fonts.cfg file is too long: %i\n", len));
		len = sizeof(buf) - 1;
	}

	trap_FS_Read(buf, len, f);
	trap_FS_FCloseFile(f);
	buf[len] = '\0';

	text = buf;
	COM_BeginParseSession("fonts.cfg");

	customFontIndex = 0;
	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (token[0] == '\0')
		{
			break;
		}

		Q_strncpyz(fontName, token, sizeof(fontName));

		token = COM_ParseExt(&text, qfalse);
		if (token[0] == '\0')
		{
			trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: missing font path for '%s'\n", fontName));
			continue;
		}

		Q_strncpyz(fontPath, token, sizeof(fontPath));

		/* TTF lines reserved later via EngFonts_AppendFaces. */
		if (EngFonts_FontCfgIsTtfEntry(fontName, fontPath))
		{
			continue;
		}

		if (fonts_num >= MAX_FONTS)
		{
			trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: maximum number of fonts (%d) reached, skipping '%s'\n", MAX_FONTS, fontName));
			continue;
		}

		if (customFontIndex >= (MAX_FONTS - OSP_BUILTIN_FONT_COUNT))
		{
			trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: maximum number of custom fonts reached, skipping '%s'\n", fontName));
			continue;
		}

		for (i = 0; i < fonts_num; i++)
		{
			if (Q_stricmp(fonts[i].name, fontName) == 0)
			{
				trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: font name '%s' already exists, skipping\n", fontName));
				break;
			}
		}

		if (i < fonts_num)
		{
			continue;
		}

		/* Check if file exists using trap_FS_FOpenFile */
		{
			fileHandle_t testFile;
#ifdef __GNUC__
			__attribute__((unused))
#endif
			int testLen;
			testLen = trap_FS_FOpenFile(fontPath, &testFile, FS_READ);
			if (testFile == FS_INVALID_HANDLE)
			{
				trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: Font file '%s' not found, skipping '%s'\n", fontPath, fontName));
				continue;
			}
			trap_FS_FCloseFile(testFile);
		}

		Q_strncpyz(customFontNames[customFontIndex], fontName, sizeof(customFontNames[customFontIndex]));
		fonts[fonts_num].name = customFontNames[customFontIndex];
		fonts[fonts_num].isTtf = qfalse;
		fonts[fonts_num].ttfPath[0] = '\0';

		/* Store font index before loading */
		fonts_num++;
		customFontIndex++;

		/* Load font - UI_LoadFont will print errors if file doesn't exist */
		UI_LoadFont(&fonts[fonts_num - 1], fontPath);

		/* Check if font was loaded successfully by verifying shaderCount > 0 */
		if (fonts[fonts_num - 1].shaderCount > 0)
		{
			trap_Print(va("Custom font '%s' registered from '%s'\n", fontName, fontPath));
		}
		else
		{
			/* Font loading failed, revert the increment */
			trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: Failed to load font '%s' from '%s'\n", fontName, fontPath));
			fonts_num--;
			customFontIndex--;
			fonts[fonts_num].name = NULL;
		}
	}
}

typedef qboolean (uiEng_GetValue_t)(char* value, int valueSize, const char* key);
typedef int (uiEng_RegisterFontAtlas_t)(const char* faceName, int pointSize, const char* alphabet, fontAtlasInfo_t* out);

static qboolean UI_EngFonts_GetValue(const char* key, char* out, int outSize)
{
	char addrBuf[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("//trap_GetValue", addrBuf, sizeof(addrBuf));
	if (!addrBuf[0])
	{
		return qfalse;
	}

#ifdef Q3_VM
	{
		int addr;
		uiEng_GetValue_t* trap_GetValue;

		addr = atoi(addrBuf);
		addr = ~addr;
		trap_GetValue = (uiEng_GetValue_t*)addr;
		return trap_GetValue(out, outSize, key);
	}
#else
	{
		int getValueFuncId;

		getValueFuncId = atoi(addrBuf);
		return trap_UI_GetValue_Q3E(getValueFuncId, out, outSize, key);
	}
#endif
}

static int UI_EngFonts_RegisterAtlas(int cmd, const char* face, int pointSize,
                                     const char* alphabet, fontAtlasInfo_t* out)
{
#ifdef Q3_VM
	uiEng_RegisterFontAtlas_t* reg;
	int addr;

	if (cmd == 0)
	{
		return 0;
	}
	addr = ~cmd;
	reg = (uiEng_RegisterFontAtlas_t*)addr;
	return reg(face, pointSize, alphabet, out);
#else
	if (cmd == 0)
	{
		return 0;
	}
	return trap_UI_RegisterFontAtlas_Q3E(cmd, face, pointSize, alphabet, out);
#endif
}

static void UI_EngFonts_CvarBuffer(const char* name, char* buf, int size)
{
	trap_Cvar_VariableStringBuffer(name, buf, size);
}

static int UI_EngFonts_FSOpen(const char* qpath, fileHandle_t* f, fsMode_t mode)
{
	return trap_FS_FOpenFile(qpath, f, mode);
}

static int UI_EngFonts_FSRead(void* buffer, int len, fileHandle_t f)
{
	trap_FS_Read(buffer, len, f);
	return len;
}

static void UI_EngFonts_FSClose(fileHandle_t f)
{
	trap_FS_FCloseFile(f);
}

static void UI_EngFonts_Print(const char* msg)
{
	trap_Print(msg);
}

static qboolean UI_FontIsTtf(int index)
{
	if (index < 0 || index >= fonts_num)
	{
		return qfalse;
	}
	return fonts[index].isTtf;
}

static const char* UI_FontTtfPath(int index)
{
	if (index < 0 || index >= fonts_num || !fonts[index].isTtf)
	{
		return NULL;
	}
	return fonts[index].ttfPath;
}

static qboolean UI_EngFonts_HasName(const char* bareName)
{
	int i;

	for (i = 0; i < fonts_num; i++)
	{
		if (fonts[i].name && !Q_stricmp(fonts[i].name, bareName))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean UI_EngFonts_AddTtf(const char* bareName, const char* ttfPath)
{
	if (ui_engTtfNameIdx >= ENG_FONTS_MAX_TTF || fonts_num >= MAX_FONTS)
	{
		return qfalse;
	}

	Q_strncpyz(engTtfNames[ui_engTtfNameIdx], bareName, sizeof(engTtfNames[ui_engTtfNameIdx]));
	fonts[fonts_num].name = engTtfNames[ui_engTtfNameIdx];
	fonts[fonts_num].isTtf = qtrue;
	fonts[fonts_num].shaderCount = 0;
	Com_Memset(fonts[fonts_num].metrics, 0, sizeof(fonts[fonts_num].metrics));
	Q_strncpyz(fonts[fonts_num].ttfPath, ttfPath, sizeof(fonts[fonts_num].ttfPath));
	fonts_num++;
	ui_engTtfNameIdx++;
	return qtrue;
}

static void UI_EngineFonts_Init(void)
{
	EngFonts_Hooks hooks;

	Com_Memset(&hooks, 0, sizeof(hooks));
	hooks.getValue = UI_EngFonts_GetValue;
	hooks.registerAtlas = UI_EngFonts_RegisterAtlas;
	hooks.cvarBuffer = UI_EngFonts_CvarBuffer;
	hooks.fsOpen = UI_EngFonts_FSOpen;
	hooks.fsRead = UI_EngFonts_FSRead;
	hooks.fsClose = UI_EngFonts_FSClose;
	hooks.print = UI_EngFonts_Print;
	EngFonts_SetHooks(&hooks);

	EngFonts_Invalidate();
	uis.engineFontAtlas = EngFonts_Probe();
	if (!uis.engineFontAtlas)
	{
		return;
	}

	fonts_num = bitmap_fonts_num;
	ui_engTtfNameIdx = 0;
	Com_Memset(engTtfNames, 0, sizeof(engTtfNames));
	EngFonts_AppendFaces(UI_EngFonts_AddTtf, UI_EngFonts_HasName, MAX_FONTS, fonts_num);
	EngFonts_Preload(fonts_num, UI_FontIsTtf, UI_FontTtfPath);
}

void UI_LoadFonts(void)
{
	int i;

	UI_InitFontNames();
	for (i = 0; i < OSP_BUILTIN_FONT_COUNT; i++)
	{
		fonts[i].isTtf = qfalse;
		fonts[i].ttfPath[0] = '\0';
		UI_LoadFont(&fonts[i], ospBuiltinFonts[i].cfgPath);
	}
	UI_LoadCustomFonts();
	bitmap_fonts_num = fonts_num;
	UI_EngineFonts_Init();
}

/* Helper functions for compiled text */
static int CompiledStringSize(const text_command_t* cmd)
{
	int size;

	size = 0;
	if (!cmd)
	{
		return 0;
	}

	for (size = 0; size < OSP_TEXT_CMD_MAX && cmd[size].type != OSP_TEXT_CMD_STOP; ++size);

	return size;
}

static float GetSymbolSize(char sym, qboolean proportional, float charWidth)
{
	return UI_Ospstr_GlyphAdvance((unsigned char)sym, charWidth, proportional ? 1 : 0);
}

float UI_OSPCharWidth640(char sym, float charWidth, int proportional)
{
	return GetSymbolSize(sym, (qboolean)proportional, charWidth);
}

int UI_OSPTextFitLineWidth640(const char* lineStart, const char* lineEnd, float charWidth,
                              int proportional, float maxWidth640)
{
	const char*     s;
	float           ax;
	int             count;
	int             lastSpaceCount;
	float           w;

	if (!lineStart || lineStart >= lineEnd || maxWidth640 <= 0.0f)
	{
		return 0;
	}

	s = lineStart;
	ax = 0.0f;
	count = 0;
	lastSpaceCount = 0;

	while (s < lineEnd)
	{
		if (s[0] == '^' && s[1])
		{
			s += 2;
			continue;
		}

		w = GetSymbolSize(*s, (qboolean)proportional, charWidth);
		if (count > 0 && (ax + w) > maxWidth640)
		{
			if (lastSpaceCount > 0)
			{
				return lastSpaceCount;
			}
			return count;
		}

		if (*s == ' ')
		{
			lastSpaceCount = count;
		}

		ax += w;
		count++;
		s++;
	}

	return count;
}

int UI_OSPTextCountWrappedLines640(const char* text, float charWidth, int proportional,
                                   float maxWidth640)
{
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	int             lines;
	int             chunk;
	int             remain;

	if (!text || !text[0] || maxWidth640 <= 0.0f)
	{
		return 0;
	}

	lines = 0;
	p = text;
	while (*p)
	{
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd <= lineStart)
		{
			lines++;
			if (p[0] == '\r' && p[1] == '\n')
			{
				p += 2;
			}
			else if (*p)
			{
				p++;
			}
			continue;
		}

		remain = (int)(lineEnd - lineStart);
		while (remain > 0)
		{
			chunk = UI_OSPTextFitLineWidth640(lineStart, lineEnd, charWidth, proportional,
			                                  maxWidth640);
			if (chunk <= 0)
			{
				chunk = 1;
			}
			if (chunk > remain)
			{
				chunk = remain;
			}
			lines++;
			lineStart += chunk;
			remain -= chunk;
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return lines;
}

static void UI_Ospstr_BindEngine(int fontIndex, float aw, float ah, int proportional)
{
	const fontAtlasInfo_t* engFont;
	float                  need;
	float                  ref;

	if (!uis.engineFontAtlas || fontIndex < 0 || !fonts[fontIndex].isTtf)
	{
		ui_ospstr_eng = NULL;
		ui_ospstr_engUnit = 1.0f;
		ui_ospstr_engDrawUnit = 1.0f;
		ui_ospstr_engYBias = 0.0f;
		ui_ospstr_engAsc = ah * 0.75f;
		return;
	}

	if (proportional)
	{
		need = ah;
	}
	else if (aw > ah)
	{
		need = aw;
	}
	else
	{
		need = ah;
	}

	engFont = EngFonts_Info(fontIndex, need);
	ui_ospstr_eng = engFont;
	if (!engFont)
	{
		ui_ospstr_engUnit = 1.0f;
		ui_ospstr_engDrawUnit = 1.0f;
		ui_ospstr_engYBias = 0.0f;
		ui_ospstr_engAsc = ah * 0.75f;
		return;
	}

	ref = engFont->lineHeight;
	if (ref < 1.0f)
	{
		ref = 64.0f;
	}
	ui_ospstr_engUnit = ah / ref;
	if (ui_ospstr_engUnit <= 0.0f)
	{
		ui_ospstr_engUnit = 1.0f;
	}
	{
		float s;
		float off;

		UI_Ospstr_EnsureTtfTune();
		if (fontIndex >= 0 && fontIndex < ENG_FONTS_FONT_INDEX_MAX)
		{
			s = ui_ospstr_ttfTuneScale[fontIndex];
			off = ui_ospstr_ttfTuneOffset[fontIndex];
			ui_ospstr_curTtfShadowX = ui_ospstr_ttfShadowOffX[fontIndex];
			ui_ospstr_curTtfShadowY = ui_ospstr_ttfShadowOffY[fontIndex];
		}
		else
		{
			s = Com_Clamp(0.25f, 2.0f, be_ttfScale.value);
			off = Com_Clamp(-1.0f, 1.0f, be_ttfFontOffset.value);
			ui_ospstr_curTtfShadowX = be_ttfShadowOffX;
			ui_ospstr_curTtfShadowY = be_ttfShadowOffY;
		}
		ui_ospstr_engDrawUnit = ui_ospstr_engUnit * s;
		ui_ospstr_engYBias = ah * ((s - 1.0f) * 0.5f + off);
	}

	if (engFont->ascender > 0.0f)
	{
		ui_ospstr_engAsc = engFont->ascender * ui_ospstr_engUnit;
	}
	else
	{
		ui_ospstr_engAsc = ah * 0.75f;
	}
}

static float UI_Ospstr_GlyphAdvance(unsigned char ch, float aw, int proportional)
{
	if (ui_ospstr_eng)
	{
		if (proportional)
		{
			return ui_ospstr_eng->glyphs[ch].xSkip * ui_ospstr_engDrawUnit;
		}
		return aw;
	}
	if (proportional)
	{
		return (metrics[ch].space1 + metrics[ch].space2) * aw;
	}
	return aw;
}

static float DrawCompiledStringLength(const text_command_t* cmd, float aw, int proportional)
{
	const font_metric_t* fm;
	float x_end;
	float ax;
	int i;
	const text_command_t* curr;
	int screenWidth;
	unsigned char ch;

	ax = 0;
	screenWidth = uis.screenXmax - uis.screenXmin;

	if (!cmd)
		return 0.0f;

	for (i = 0; i < OSP_TEXT_CMD_MAX; ++i)
	{
		curr = &cmd[i];

		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			ch = (unsigned char)curr->value.character;
			if (ui_ospstr_eng)
			{
				x_end = ax + UI_Ospstr_GlyphAdvance(ch, aw, proportional);
			}
			else
			{
				fm = &metrics[ch];
				if (proportional)
				{
					ax += fm->space1 * aw;
					x_end = ax + fm->space2 * aw;
				}
				else
				{
					x_end = ax + aw;
				}
			}

			if (x_end >= screenWidth)
				break;

			ax = x_end;
		}
		else if (curr->type == OSP_TEXT_CMD_STOP)
		{
			break;
		}
	}

	return ax;
}

static float RestrictCompiledString(text_command_t* cmd, float charWidth, int proportional, float toWidth)
{
	float x_end;
	float ax;
	int i;
	int size;
	text_command_t* curr;
	qboolean restricted;
	unsigned char ch;

	ax = 0;
	restricted = qfalse;

	if (!cmd || toWidth == 0)
		return 0;

	size = CompiledStringSize(cmd);

	for (i = 0; i < size; ++i)
	{
		curr = &cmd[i];

		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			ch = (unsigned char)curr->value.character;
			x_end = ax + UI_Ospstr_GlyphAdvance(ch, charWidth, proportional);

			if (x_end > toWidth)
			{
				restricted = qtrue;
				break;
			}

			ax = x_end;
		}
		else if (curr->type == OSP_TEXT_CMD_STOP)
		{
			break;
		}
	}

	if (restricted)
	{
		while (i > 0)
		{
			float dotSize;
			float prevSize;

			dotSize = GetSymbolSize('.', proportional, charWidth);
			prevSize = GetSymbolSize(cmd[i - 1].value.character, proportional, charWidth);

			if ((ax - prevSize + dotSize) <= toWidth)
			{
				--i;
				ax = ax - prevSize + dotSize;
				break;
			}
			--i;
			ax -= prevSize;
		}

		curr = &cmd[i];
		curr->type = OSP_TEXT_CMD_CHAR;
		curr->value.character = '.';

		if (i + 1 < OSP_TEXT_CMD_MAX)
		{
			cmd[i + 1].type = OSP_TEXT_CMD_STOP;
		}
	}

	return ax;
}

static float RestrictCompiledStringChars(text_command_t* cmd, int maxChars)
{
	int chars;
	int i;
	text_command_t* curr;
	qboolean restricted;

	chars = 0;
	restricted = qfalse;

	if (!cmd || maxChars <= 0)
		return 0;

	for (i = 0; i < OSP_TEXT_CMD_MAX; ++i)
	{
		curr = &cmd[i];

		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			chars++;
			if (chars >= maxChars)
			{
				restricted = qtrue;
				break;
			}
		}
		else if (curr->type == OSP_TEXT_CMD_STOP)
		{
			break;
		}
	}

	if (restricted)
	{
		curr = &cmd[i];
		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			curr->value.character = '.';
		}

		if (i + 1 < OSP_TEXT_CMD_MAX)
		{
			cmd[i + 1].type = OSP_TEXT_CMD_STOP;
		}
	}

	return (float)chars;
}

/* Frame drawing function */
void UI_OSPDrawFrame(float x, float y, float w, float h, vec4_t borderSize, vec4_t color, qboolean inner)
{
	if (!borderSize || !color)
	{
		return;
	}

	if (borderSize[0] <= 0.0f && borderSize[1] <= 0.0f && borderSize[2] <= 0.0f && borderSize[3] <= 0.0f)
	{
		return;
	}

	trap_R_SetColor(color);

	/* If required to draw inner border (inner) */
	if (inner)
	{
		if (borderSize[0] > 0.0f)   /* Left */
		{
			trap_R_DrawStretchPic(x, y + borderSize[1], borderSize[0], h - borderSize[1] - borderSize[3], 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[1] > 0.0f)   /* Top */
		{
			trap_R_DrawStretchPic(x, y, w, borderSize[1], 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[2] > 0.0f)   /* Right */
		{
			trap_R_DrawStretchPic(x + w - borderSize[2], y + borderSize[1], borderSize[2], h - borderSize[1] - borderSize[3], 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[3] > 0.0f)   /* Bottom */
		{
			trap_R_DrawStretchPic(x, y + h - borderSize[3], w, borderSize[3], 0, 0, 0, 0, uis.whiteShader);
		}
	}
	else /* If drawing outer border (outer) */
	{
		if (borderSize[0] > 0.0f)   /* Left */
		{
			trap_R_DrawStretchPic(x - borderSize[0], y, borderSize[0], h, 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[1] > 0.0f)   /* Top */
		{
			trap_R_DrawStretchPic(x - borderSize[0], y - borderSize[1], w + borderSize[0] + borderSize[2], borderSize[1], 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[2] > 0.0f)   /* Right */
		{
			trap_R_DrawStretchPic(x + w, y, borderSize[2], h, 0, 0, 0, 0, uis.whiteShader);
		}
		if (borderSize[3] > 0.0f)   /* Bottom */
		{
			trap_R_DrawStretchPic(x - borderSize[0], y + h, w + borderSize[0] + borderSize[2], borderSize[3], 0, 0, 0, 0, uis.whiteShader);
		}
	}
	trap_R_SetColor(NULL);
}

/* Main drawing function */
void UI_OSPDrawStringNew(float x, float y, const char* string, const vec4_t setColor,
                         vec4_t shadowColor,
                         float charWidth, float charHeight, int maxWidth, int flags,
                         vec4_t background, vec4_t border, vec4_t borderColor)
{
	const font_metric_t* fm;
	const float* tc;
	float ax, ay, aw, aw1, ah;
	float x_end;
#ifdef __GNUC__
	__attribute__((unused))
#endif
	float xx;
	float fade;
	vec4_t color;
	float xx_add, yy_add;
	int i;
	int hasBorder;
	int proportional;
	qhandle_t sh;
	text_command_t* text_commands;
	text_command_t* curr;
	float expectedLenght;
	int shadowEnabled;
	float shadowFade;
	vec4_t shadowCol;

	if (!string)
		return;

	text_commands = UI_CompileText(string);
	if (!text_commands)
		return;

	UI_AdjustFrom640(&x, &y, &charWidth, &charHeight);

	ax = x;
	ay = y;

	aw = charWidth;
	ah = charHeight;

	proportional = (flags & DS_PROPORTIONAL) ? 1 : 0;
	UI_Ospstr_BindEngine(ui_selectedFontIndex, aw, ah, proportional);
	hasBorder = (border != NULL) ? 1 : 0;
	expectedLenght = 0.0f;

	if (flags & DS_MAX_WIDTH_IS_CHARS)
	{
		RestrictCompiledStringChars(text_commands, maxWidth);
	}
	else
	{
		float mw;
		mw = (float)maxWidth;
		UI_AdjustFrom640(NULL, NULL, &mw, NULL);
		RestrictCompiledString(text_commands, aw, proportional, mw);
	}

	if (hasBorder || background || (flags & (DS_HCENTER | DS_HRIGHT)))
	{
		expectedLenght = DrawCompiledStringLength(text_commands, aw, proportional);
	}

	if (flags & (DS_HCENTER | DS_HRIGHT))
	{
		if (flags & DS_HCENTER)
		{
			ax -= 0.5f * expectedLenght;
		}
		else
		{
			ax -= expectedLenght;
		}
	}

	if (flags & DS_VCENTER)
	{
		ay -= ah / 2;
	}
	else if (flags & DS_VTOP)
	{
		ay -= ah;
	}

	sh = 0;
	if (!ui_ospstr_eng)
	{
		sh = font->shader[0]; /* low-res shader by default */
		for (i = 1; i < font->shaderCount; i++)
		{
			if (ah >= font->shaderThreshold[i])
			{
				sh = font->shader[i];
			}
		}
	}

	if (background)
	{
		trap_R_SetColor(background);
		trap_R_DrawStretchPic(ax, ay, expectedLenght, ah, 0, 0, 0, 0, uis.whiteShader);
		trap_R_SetColor(colorWhite);
	}

	if (hasBorder)
	{
		UI_OSPDrawFrame(ax, ay, expectedLenght, ah, border, borderColor, 0);
	}

	shadowEnabled = (flags & DS_SHADOW) && (shadowColor[3] != 0.0f);
	if (shadowEnabled)
	{
		static int lastBmMod = -1;
		static int lastTtfMod = -1;

		if (be_fontShadowOffset.modificationCount != lastBmMod)
		{
			lastBmMod = be_fontShadowOffset.modificationCount;
			BeFontShadow_UpdateBitmap(be_fontShadowOffset.string);
		}
		if (be_ttfShadowOffset.modificationCount != lastTtfMod)
		{
			lastTtfMod = be_ttfShadowOffset.modificationCount;
			BeFontShadow_UpdateTtf(be_ttfShadowOffset.string);
		}
		if (ui_ospstr_eng)
		{
			xx_add = aw * ui_ospstr_curTtfShadowX;
			yy_add = aw * ui_ospstr_curTtfShadowY;
		}
		else
		{
			xx_add = aw * be_fontShadowOffX;
			yy_add = aw * be_fontShadowOffY;
		}
		VectorCopy(shadowColor, shadowCol);
		shadowFade = shadowColor[3];
		shadowCol[3] = shadowFade;
	}
	else
	{
		shadowFade = 1.0f;
	}

	fade = 1.0f;
	Vector4Copy(setColor, color);

	/* Save initial x for reset between shadow and text */
	xx = ax;

	for (i = 0; i < OSP_TEXT_CMD_MAX && text_commands[i].type != OSP_TEXT_CMD_STOP; i++)
	{
		curr = &text_commands[i];

		switch (curr->type)
		{
			case OSP_TEXT_CMD_CHAR:
			{
				int index;
				const fontAtlasGlyph_t* g;
				float gw, gh, gy;

				index = curr->value.character;
				if (index < 0)
					index += 256;

				if (ui_ospstr_eng)
				{
					g = &ui_ospstr_eng->glyphs[(unsigned char)index];
					if (g->glyph)
					{
						gw = (float)g->imageWidth * ui_ospstr_engDrawUnit;
						gh = (float)g->imageHeight * ui_ospstr_engDrawUnit;
						gy = ay + ui_ospstr_engAsc - (float)g->top * ui_ospstr_engDrawUnit + ui_ospstr_engYBias;

						if (shadowEnabled)
						{
							trap_R_SetColor(shadowCol);
							trap_R_DrawStretchPic(ax + xx_add, gy + yy_add, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
						}
						trap_R_SetColor(color);
						trap_R_DrawStretchPic(ax, gy, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
					}
					ax += UI_Ospstr_GlyphAdvance((unsigned char)index, aw, proportional);
				}
				else
				{
					fm = &metrics[(unsigned char)index];

					if (proportional)
					{
						tc = fm->tc_prop;
						aw1 = fm->width * aw;
					}
					else
					{
						tc = fm->tc_mono;
						aw1 = aw;
					}

					/* Draw shadow */
					if (shadowEnabled)
					{
						trap_R_SetColor(shadowCol);
						trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
					}

					/* Draw character */
					trap_R_SetColor(color);
					trap_R_DrawStretchPic(ax, ay, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);

					/* Update position */
					if (proportional)
					{
						ax += fm->space1 * aw;
						x_end = ax + fm->space2 * aw;
					}
					else
					{
						x_end = ax + aw;
					}
					ax = x_end;
				}
			}
			break;

			case OSP_TEXT_CMD_TEXT_COLOR:
				VectorCopy(curr->value.color, color);
				color[3] = fade;
				if (setColor && color[3] > setColor[3])
				{
					color[3] = setColor[3];
				}
				shadowCol[3] = color[3];
				break;

			case OSP_TEXT_CMD_SHADOW_COLOR:
				VectorCopy(curr->value.color, shadowCol);
				shadowCol[3] = shadowFade;
				if (setColor && shadowCol[3] > setColor[3])
				{
					shadowCol[3] = setColor[3];
				}
				break;

			case OSP_TEXT_CMD_FADE:
				fade = curr->value.fade;
				color[3] = fade;
				shadowCol[3] = fade;
				if (setColor)
				{
					if (color[3] > setColor[3]) color[3] = setColor[3];
					if (shadowCol[3] > setColor[3]) shadowCol[3] = setColor[3];
				}
				break;

			case OSP_TEXT_CMD_STOP:
				break;
		}
	}

	UI_CompiledTextDestroy(text_commands);

	trap_R_SetColor(NULL);
}
