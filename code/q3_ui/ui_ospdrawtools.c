/*
===========================================================================
UI OSP Draw Tools - Font rendering and text drawing functions
Adapted from cgame/cg_drawtools.c
===========================================================================
*/

#include "ui_local.h"

/* Memory allocation - use static buffers for C89 compatibility and QVM */
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
} font_t;

#define MAX_FONTS 32
static font_t fonts[MAX_FONTS];
static char customFontNames[MAX_FONTS - 21][MAX_QPATH];
static int fonts_num = 21;
static const font_t* font = &fonts[0];
static const font_metric_t* metrics = &fonts[0].metrics[0];

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

	/* Use static buffer instead of malloc for QVM compatibility */
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
	fonts[0].name = "id";
	fonts[1].name = "idblock";
	fonts[2].name = "sansman";
	fonts[3].name = "cpma";
	fonts[4].name = "m1rage";
	fonts[5].name = "elite_emoji";
	fonts[6].name = "diablo";
	fonts[7].name = "eternal";
	fonts[8].name = "qlnumbers";
	fonts[9].name = "elite";
	fonts[10].name = "elitebigchars";
	fonts[11].name = "eliteeternal";
	fonts[12].name = "bigcharseliteeternal";
	fonts[13].name = "elitestoropia";
	fonts[14].name = "bigcharselitestoropia";
	fonts[15].name = "bigcharseliteorbitron";
	fonts[16].name = "eliteorbitron";
	fonts[17].name = "q4snumbers";
	fonts[18].name = "q4mnumbers";
	fonts[19].name = "conthrax";
	fonts[20].name = "qlhandelgothichud";
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

	shaderCount = 0;

	text = buf; /* initialize parser */
	COM_BeginParseSession(fontName);

	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (token[0] == '\0')
		{
			Com_Printf(S_COLOR_RED "UI_LoadFont: parse error.\n");
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
				Com_Printf("UI_LoadFont: font image '%s' doesn't exist.\n", token);
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
				Com_Printf("UI_LoadFont: error reading image width.\n");
				return;
			}
			r_width = 1.0 / width;

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0' || (height = atof(token)) <= 0.0)
			{
				Com_Printf("UI_LoadFont: error reading image height.\n");
				return;
			}
			r_height = 1.0 / height;

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0')
			{
				Com_Printf("UI_LoadFont: error reading char width.\n");
				return;
			}
			char_width = atof(token);

			token = COM_ParseExt(&text, qfalse);
			if (token[0] == '\0')
			{
				Com_Printf("UI_LoadFont: error reading char height.\n");
				return;
			}
			char_height = atof(token);

			break; /* parse char metrics */
		}
	}

	if (shaderCount == 0)
	{
		Com_Printf("UI_LoadFont: no font images specified in %s.\n", fontName);
		return;
	}

	fm = fnt->metrics;

	chars = 0;
	for (;;)
	{
		/* char index */
		token = COM_ParseExt(&text, qtrue);
		if (!token[0])
			break;

		if (token[0] == '\'' && token[1] && token[2] == '\'')   /* char code in form 'X' */
			i = token[1] & 255;
		else /* integer code */
			i = atoi(token);

		if (i < 0 || i > 255)
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: bad char index %i.\n", i));
			return;
		}
		fm = fnt->metrics + i;

		/* x0 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading x0.\n"));
			return;
		}
		x0 = atof(token);

		/* y0 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading y0.\n"));
			return;
		}
		y0 = atof(token);

		/* w1-offset */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading x-offset.\n"));
			return;
		}
		w1 = atof(token);

		/* w2-offset */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading x-length.\n"));
			return;
		}
		w2 = atof(token);

		/* space1 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading space1.\n"));
			return;
		}
		s1 = atof(token);

		/* space2 */
		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			trap_Print(va(S_COLOR_RED "UI_LoadFont: error reading space2.\n"));
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

		if (fonts_num >= MAX_FONTS)
		{
			trap_Print(va(S_COLOR_YELLOW "UI_LoadCustomFonts: maximum number of fonts (%d) reached, skipping '%s'\n", MAX_FONTS, fontName));
			continue;
		}

		if (customFontIndex >= (MAX_FONTS - 21))
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

void UI_LoadFonts(void)
{
	UI_InitFontNames();
	UI_LoadFont(&fonts[0], "gfx/2d/bigchars.cfg");
	UI_LoadFont(&fonts[1], "gfx/2d/numbers.cfg");
	UI_LoadFont(&fonts[2], "gfx/2d/sansman.cfg");
	UI_LoadFont(&fonts[3], "gfx/2d/sansman.cfg"); /* use sansman instead of cpma */
	UI_LoadFont(&fonts[4], "gfx/2d/m1rage.cfg");
	UI_LoadFont(&fonts[5], "gfx/2d/elite_emoji.cfg");
	UI_LoadFont(&fonts[6], "gfx/2d/diablo.cfg");
	UI_LoadFont(&fonts[7], "gfx/2d/eternal.cfg");
	UI_LoadFont(&fonts[8], "gfx/2d/qlnumbers.cfg");
	UI_LoadFont(&fonts[9], "gfx/2d/Elite.cfg");
	UI_LoadFont(&fonts[10], "gfx/2d/EliteBigchars.cfg");
	UI_LoadFont(&fonts[11], "gfx/2d/EliteEternal.cfg");
	UI_LoadFont(&fonts[12], "gfx/2d/BigcharsEliteEternal.cfg");
	UI_LoadFont(&fonts[13], "gfx/2d/EliteStoropia.cfg");
	UI_LoadFont(&fonts[14], "gfx/2d/BigcharsEliteStoropia.cfg");
	UI_LoadFont(&fonts[15], "gfx/2d/BigcharsEliteOrbitron.cfg");
	UI_LoadFont(&fonts[16], "gfx/2d/EliteOrbitron.cfg");
	UI_LoadFont(&fonts[17], "gfx/2d/q4snumbers.cfg");
	UI_LoadFont(&fonts[18], "gfx/2d/q4mnumbers.cfg");
	UI_LoadFont(&fonts[19], "gfx/2d/conthrax.cfg");
	UI_LoadFont(&fonts[20], "gfx/2d/qlhandelgothichud.cfg");
	UI_LoadCustomFonts();
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
	const font_metric_t* fm;
	fm = &metrics[(unsigned char)sym];

	if (proportional)
	{
		return (fm->space1 + fm->space2) * charWidth;
	}

	return charWidth;
}

static float DrawCompiledStringLength(const text_command_t* cmd, float aw, int proportional)
{
	const font_metric_t* fm;
	float x_end;
	float ax;
	int i;
	const text_command_t* curr;
	int screenWidth;

	ax = 0;
	screenWidth = uis.screenXmax - uis.screenXmin;

	if (!cmd)
		return 0.0f;

	for (i = 0; i < OSP_TEXT_CMD_MAX; ++i)
	{
		curr = &cmd[i];

		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			fm = &metrics[(unsigned char)curr->value.character ];
			if (proportional)
			{
				ax += fm->space1 * aw;
				x_end = ax + fm->space2 * aw;
			}
			else
			{
				x_end = ax + aw;
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
	const font_metric_t* fm;
	float x_end;
	float ax;
	int i;
	int size;
	text_command_t* curr;
	qboolean restricted;

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
			fm = &metrics[(unsigned char)curr->value.character ];
			if (proportional)
			{
				ax += fm->space1 * charWidth;
				x_end = ax + fm->space2 * charWidth;
			}
			else
			{
				x_end = ax + charWidth;
			}

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

	sh = font->shader[0]; /* low-res shader by default */
	for (i = 1; i < font->shaderCount; i++)
	{
		if (ah >= font->shaderThreshold[i])
		{
			sh = font->shader[i];
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
		xx_add = charWidth / 10.0f;
		yy_add = xx_add;
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
				index = curr->value.character;
				if (index < 0)
					index += 256;
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

