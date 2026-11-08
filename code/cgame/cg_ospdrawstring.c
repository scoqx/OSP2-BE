/*
===========================================================================
OSP proportional string draw (CG_OSPDrawStringNew)
Single-pass parse; plain-text fast path; static command buffer.
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/be_font_shadow.h"
#include "../qcommon/beinfo_cfg.h"
#include "../qcommon/engine_fonts.h"

static text_command_t ospstr_cmdbuf[OSP_TEXT_CMD_MAX];

/*
A HUD element mostly draws the same string in the same place, and most of the cost is getting from
the text to the quads. So what a call sends to the renderer is recorded and sent again while the
call stays the same. A record depends on: the text, position, size, colors, width, flags, font, the
TTF tune generation, the shadow offset and cg_disableTextEffects. Text that blinks by time is not kept.
*/
#define OSPSTR_REC_SLOTS        96
#define OSPSTR_REC_TEXT         64
#define OSPSTR_REC_OPS          72
#define OSPSTR_REC_MISS_MS      50      /* two misses this close: the element changes all the time */
#define OSPSTR_REC_OFF_MS       1000

#define OSPSTR_OP_COLOR         (-1)
#define OSPSTR_OP_COLOR_NULL    (-2)

typedef struct
{
	float       v[8];
	qhandle_t   shader;
} ospstrRecOp_t;

typedef struct
{
	qboolean    valid;
	float       x, y, w, h;
	int         maxWidth, flags, fontIndex, tune, textEffects;
	float       shadowOffX, shadowOffY;
	vec4_t      setColor, shadowColor, background, border, borderColor;
	int         hasColors;      /* which of the optional colors the call had */
	char        text[OSPSTR_REC_TEXT];
	int         numOps;
	int         lastMiss;
	int         offUntil;
	ospstrRecOp_t ops[OSPSTR_REC_OPS];
} ospstrRec_t;

static ospstrRec_t  ospstr_recs[OSPSTR_REC_SLOTS];
static int          ospstr_recsUsed;
static ospstrRec_t* ospstr_rec;         /* being recorded, or NULL */
static qboolean     ospstr_recFailed;
static qboolean     ospstr_usedTime;
static int          ospstr_recLastColor;    /* its last color op, or -1 */

static void ospstr_rec_pic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader)
{
	ospstrRecOp_t* op;

	if (ospstr_rec->numOps >= OSPSTR_REC_OPS)
	{
		ospstr_recFailed = qtrue;
		return;
	}
	op = &ospstr_rec->ops[ospstr_rec->numOps++];
	op->v[0] = x;
	op->v[1] = y;
	op->v[2] = w;
	op->v[3] = h;
	op->v[4] = s1;
	op->v[5] = t1;
	op->v[6] = s2;
	op->v[7] = t2;
	op->shader = shader;
}

static void ospstr_rec_color(const float* color)
{
	ospstrRecOp_t* op;

	/* Setting the color it already has is left out of the record */
	if (ospstr_recLastColor >= 0)
	{
		op = &ospstr_rec->ops[ospstr_recLastColor];
		if (color ? (op->shader == OSPSTR_OP_COLOR && op->v[0] == color[0] && op->v[1] == color[1] &&
		             op->v[2] == color[2] && op->v[3] == color[3]) : op->shader == OSPSTR_OP_COLOR_NULL)
		{
			return;
		}
	}
	if (ospstr_rec->numOps >= OSPSTR_REC_OPS)
	{
		ospstr_recFailed = qtrue;
		return;
	}
	ospstr_recLastColor = ospstr_rec->numOps;
	op = &ospstr_rec->ops[ospstr_rec->numOps++];
	if (color)
	{
		op->v[0] = color[0];
		op->v[1] = color[1];
		op->v[2] = color[2];
		op->v[3] = color[3];
		op->shader = OSPSTR_OP_COLOR;
	}
	else
	{
		op->shader = OSPSTR_OP_COLOR_NULL;
	}
}

/* Everything this file sends to the renderer goes through these */
#define trap_R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, shader) \
	do { \
		if (ospstr_rec) { ospstr_rec_pic((x), (y), (w), (h), (s1), (t1), (s2), (t2), (shader)); } \
		trap_R_DrawStretchPic((x), (y), (w), (h), (s1), (t1), (s2), (t2), (shader)); \
	} while (0)
#define trap_R_SetColor(color) \
	do { \
		if (ospstr_rec) { ospstr_rec_color(color); } \
		trap_R_SetColor(color); \
	} while (0)

/* Module-local engine font context for measure/draw (cgame is single-threaded). */
static const fontAtlasInfo_t* ospstr_eng;
static float                  ospstr_engUnit;
static float                  ospstr_engDrawUnit; /* unit * scale — ink + prop advance */
static float                  ospstr_engAsc; /* ascender * unit — once per string */
static float                  ospstr_engYBias; /* scale-center(ah) + offset*screenYScale */

/* Resolved TTF tune by fontIndex (beInfo override or cvars). */
static float ospstr_ttfTuneScale[ENG_FONTS_FONT_INDEX_MAX];
static float ospstr_ttfTuneOffset[ENG_FONTS_FONT_INDEX_MAX];
static float ospstr_ttfShadowOffX[ENG_FONTS_FONT_INDEX_MAX];
static float ospstr_ttfShadowOffY[ENG_FONTS_FONT_INDEX_MAX];
static int   ospstr_ttfTuneScaleMod = -1;
static int   ospstr_ttfTuneGeneration;
static int   ospstr_ttfTuneOffMod = -1;
static int   ospstr_ttfShadowMod = -1;
static int   ospstr_ttfTuneFontCount = -1;
/* Current font shadow (set in bind_engine for TTF hotpath). */
static float ospstr_curTtfShadowX = 0.1f;
static float ospstr_curTtfShadowY = 0.1f;

static void ospstr_ensure_ttf_tune(void)
{
	int i;
	int n;

	if (ospstr_ttfTuneScaleMod == be_ttfScale.modificationCount &&
	        ospstr_ttfTuneOffMod == be_ttfFontOffset.modificationCount &&
	        ospstr_ttfShadowMod == be_ttfShadowOffset.modificationCount &&
	        ospstr_ttfTuneFontCount == CG_GetFontCount())
	{
		return;
	}

	n = CG_GetFontCount();
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
		name = CG_GetFontName(i);
		if (CG_FontIsTtf(i) && name && name[0])
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
		ospstr_ttfTuneScale[i] = Com_Clamp(0.25f, 2.0f, s);
		ospstr_ttfTuneOffset[i] = Com_Clamp(-1.0f, 1.0f, off);
		ospstr_ttfShadowOffX[i] = shX;
		ospstr_ttfShadowOffY[i] = shY;
	}

	ospstr_ttfTuneScaleMod = be_ttfScale.modificationCount;
	ospstr_ttfTuneGeneration++;
	ospstr_ttfTuneOffMod = be_ttfFontOffset.modificationCount;
	ospstr_ttfShadowMod = be_ttfShadowOffset.modificationCount;
	ospstr_ttfTuneFontCount = CG_GetFontCount();
}

/* Inline width/advance — no CALL in measure/draw loops. */
#define OSPSTR_GLYPH_W(metrics, ch, aw, prop) \
	(ospstr_eng \
		? ((prop) ? (ospstr_eng->glyphs[(unsigned char)(ch)].xSkip * ospstr_engDrawUnit) : (aw)) \
		: ((prop) ? (((metrics)[(unsigned char)(ch)].space1 + (metrics)[(unsigned char)(ch)].space2) * (aw)) : (aw)))

/*
================
ospstr_bind_engine

Flat: early reject if no atlas; else pick tier + cache unit/asc. One CALL per string.
================
*/
static void ospstr_bind_engine(int fontIndex, float aw, float ah, int proportional)
{
	const fontAtlasInfo_t* font;
	float                  need;
	float                  ref;
	static int             boundFont = -1;
	static float           boundW;
	static float           boundH;
	static int             boundProportional = -1;
	static int             boundTuneGeneration = -1;

	/* Strings usually come in runs of one style, so most calls have nothing to do. */
	ospstr_ensure_ttf_tune();
	if (fontIndex == boundFont && aw == boundW && ah == boundH && proportional == boundProportional &&
	        boundTuneGeneration == ospstr_ttfTuneGeneration)
	{
		return;
	}
	boundFont = fontIndex;
	boundW = aw;
	boundH = ah;
	boundProportional = proportional;
	boundTuneGeneration = ospstr_ttfTuneGeneration;

	if (!cgs.engineFontAtlas)
	{
		ospstr_eng = NULL;
		ospstr_engUnit = 1.0f;
		ospstr_engDrawUnit = 1.0f;
		ospstr_engYBias = 0.0f;
		ospstr_engAsc = ah * 0.75f;
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

	font = CG_EngineFontInfo(fontIndex, need);
	ospstr_eng = font;
	if (!font)
	{
		ospstr_engUnit = 1.0f;
		ospstr_engDrawUnit = 1.0f;
		ospstr_engYBias = 0.0f;
		ospstr_engAsc = ah * 0.75f;
		return;
	}

	ref = font->lineHeight;
	if (ref < 1.0f)
	{
		ref = 64.0f;
	}
	ospstr_engUnit = ah / ref;
	if (ospstr_engUnit <= 0.0f)
	{
		ospstr_engUnit = 1.0f;
	}
	{
		float s;
		float off;

		if (fontIndex >= 0 && fontIndex < ENG_FONTS_FONT_INDEX_MAX)
		{
			s = ospstr_ttfTuneScale[fontIndex];
			off = ospstr_ttfTuneOffset[fontIndex];
			ospstr_curTtfShadowX = ospstr_ttfShadowOffX[fontIndex];
			ospstr_curTtfShadowY = ospstr_ttfShadowOffY[fontIndex];
		}
		else
		{
			s = Com_Clamp(0.25f, 2.0f, be_ttfScale.value);
			off = Com_Clamp(-1.0f, 1.0f, be_ttfFontOffset.value);
			ospstr_curTtfShadowX = be_ttfShadowOffX;
			ospstr_curTtfShadowY = be_ttfShadowOffY;
		}
		ospstr_engDrawUnit = ospstr_engUnit * s;
		ospstr_engYBias = ah * ((s - 1.0f) * 0.5f + off);
	}

	if (font->ascender >= 1.0f)
	{
		ospstr_engAsc = font->ascender * ospstr_engUnit;
	}
	else
	{
		ospstr_engAsc = ah * 0.75f;
	}
}

static qboolean ospstr_is_plain(const char* string, int flags)
{
	const unsigned char* p;

	if (!string)
	{
		return qfalse;
	}

	if (flags & DS_PLAIN)
	{
		return qtrue;
	}

	p = (const unsigned char*)string;
	while (*p)
	{
		if (*p == '^' && p[1])
		{
			return qfalse;
		}
		if ((flags & DS_EMOJI) && *p == (unsigned char)EMOJI_CHAR)
		{
			return qfalse;
		}
		p++;
	}
	return qtrue;
}

static float ospstr_shader_width(float ah)
{
	return ah;
}

static qboolean ospstr_is_palette_color_char(char c)
{
	if (c >= '0' && c <= '9')
	{
		return qtrue;
	}
	if (c >= 'a' && c <= 'z')
	{
		return qtrue;
	}
	if (c >= 'A' && c <= 'Z')
	{
		return qtrue;
	}
	return qfalse;
}

static void ospstr_palette_color_from_char(char c, vec4_t out)
{
	if (c >= '0' && c <= '9')
	{
		VectorCopy(g_color_table[c - '0'], out);
	}
	else
	{
		CG_OSPColorFromChar(c, out);
	}
}

static int ospstr_build_commands(const char* in, int flags, text_command_t* commands)
{
	int b;
	int i;
	vec4_t back_color;
	qboolean back_color_was_set;
	qboolean top_color_was_set;
	float rc;
	float gc;
	float bc;
	const emoji_t* em;
	const char* from;

	if (!in || !in[0])
	{
		return -1;
	}

	i = 0;
	back_color_was_set = qfalse;
	top_color_was_set = qfalse;
	Vector4Copy(colorWhite, back_color);
	from = in;

	while (*from && i < OSP_TEXT_CMD_MAX - 1)
	{
		if (from[0] == '^' && from[1] && from[1] != '^')
		{
			if (cg_disableTextEffects.integer && CG_TextControlDisabled(from[1]))
			{
				from += 2;
				continue;
			}
			switch (from[1])
			{
				case 'f':
					from += 2;
					if (((ospstr_usedTime = qtrue, cg.time) & 0x3ff) >= 512)
					{
						while (*from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'F')))
						{
							from++;
						}
					}
					continue;
				case 'F':
					from += 2;
					if (((ospstr_usedTime = qtrue, cg.time) & 0x3ff) < 512)
					{
						while (*from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'f')))
						{
							from++;
						}
					}
					continue;
				case 'B':
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						i++;
					}
					ospstr_usedTime = qtrue;
					b = cg.time & 0x7ff;
					if (b > 1024)
					{
						b = ~b & 0x7ff;
					}
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade = b / 1463.0f + 0.3f;
					i++;
					from += 2;
					break;
				case 'b':
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						i++;
					}
					ospstr_usedTime = qtrue;
					b = cg.time & 0x7ff;
					if (b > 1024)
					{
						b = ~b & 0x7ff;
					}
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade = b / 1024.0f;
					i++;
					from += 2;
					break;
				case 'N':
				case 'n':
					commands[i].type = OSP_TEXT_CMD_FADE;
					commands[i].value.fade = 1.0f;
					i++;
					if (!top_color_was_set && back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(back_color, commands[i].value.color);
						i++;
					}
					else if (top_color_was_set && !back_color_was_set)
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						VectorCopy(colorWhite, commands[i].value.color);
						i++;
					}
					from += 2;
					break;
				case '^':
					commands[i].type = OSP_TEXT_CMD_CHAR;
					commands[i].value.character = '^';
					i++;
					from += 1;
					break;
				case 'X':
				case 'x':
					if (!CG_Hex16GetColor(&from[2], &rc))
					{
						from += 2;
						break;
					}
					if (!CG_Hex16GetColor(&from[4], &gc))
					{
						from += 2;
						break;
					}
					if (!CG_Hex16GetColor(&from[6], &bc))
					{
						from += 2;
						break;
					}
					back_color[0] = rc;
					back_color[1] = gc;
					back_color[2] = bc;
					commands[i].type = OSP_TEXT_CMD_SHADOW_COLOR;
					VectorCopy(back_color, commands[i].value.color);
					back_color_was_set = qtrue;
					i++;
					from += 8;
					break;
				case 'c':
					/* ^cT / ^cE — team / enemy color (printUI / SuperHUD text only) */
					if ((flags & DS_CTE_COLOR) && (from[2] == 'T' || from[2] == 'E'))
					{
						char te[2];

						te[0] = from[2];
						te[1] = '\0';
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						if (CG_ParseColorStr(te, commands[i].value.color))
						{
							i++;
						}
						from += 3;
						break;
					}
					/* fallthrough: palette letter c */
				default:
					if (!(cg_disableTextEffects.integer && CG_TextControlDisabled(from[1])) && ospstr_is_palette_color_char(from[1]))
					{
						commands[i].type = OSP_TEXT_CMD_TEXT_COLOR;
						ospstr_palette_color_from_char(from[1], commands[i].value.color);
						i++;
					}
					from += 2;
					break;
			}
		}
		else
		{
			if ((flags & DS_EMOJI) && (unsigned char)from[0] == (unsigned char)EMOJI_CHAR)
			{
				em = CG_OSPGetEmoji(from);
				if (em != NULL)
				{
					commands[i].type = OSP_TEXT_CMD_SHADER;
					commands[i].value.shader = em->shader;
					i++;
					from += em->len;
					continue;
				}
			}
			commands[i].type = OSP_TEXT_CMD_CHAR;
			commands[i].value.character = from[0];
			i++;
			from++;
		}
	}

	commands[i].type = OSP_TEXT_CMD_STOP;
	return i;
}

static float ospstr_measure_commands(const text_command_t* cmd, int stopIdx, const cg_osp_font_metric_t* metrics,
                                     float aw, float ah, int proportional, float vidWidth)
{
	float ax;
	int i;

	ax = 0.0f;
	for (i = 0; i < stopIdx; i++)
	{
		if (cmd[i].type == OSP_TEXT_CMD_CHAR)
		{
			ax += OSPSTR_GLYPH_W(metrics, cmd[i].value.character, aw, proportional);
			if (ax >= vidWidth)
			{
				break;
			}
		}
		else if (cmd[i].type == OSP_TEXT_CMD_SHADER)
		{
			ax += ospstr_shader_width(ah);
			if (ax >= vidWidth)
			{
				break;
			}
		}
	}
	return ax;
}

static float ospstr_get_symbol_width(const text_command_t* cmd, int index, const cg_osp_font_metric_t* metrics,
                                     float aw, float ah, int proportional)
{
	if (cmd[index].type == OSP_TEXT_CMD_SHADER)
	{
		return ospstr_shader_width(ah);
	}
	if (cmd[index].type == OSP_TEXT_CMD_CHAR)
	{
		return OSPSTR_GLYPH_W(metrics, cmd[index].value.character, aw, proportional);
	}
	return 0.0f;
}

static int ospstr_restrict_pixels(text_command_t* cmd, int stopIdx, const cg_osp_font_metric_t* metrics,
                                  float aw, float ah, int proportional, float toWidth)
{
	int i;
	float ax;
	float x_end;
	text_command_t* curr;
	qboolean restricted;
	const cg_osp_font_metric_t* fm;

	if (!cmd || toWidth <= 0.0f)
	{
		return stopIdx;
	}

	restricted = qfalse;
	ax = 0.0f;

	for (i = 0; i < stopIdx; i++)
	{
		curr = &cmd[i];
		if (curr->type == OSP_TEXT_CMD_CHAR)
		{
			if (ospstr_eng)
			{
				x_end = ax + OSPSTR_GLYPH_W(metrics, curr->value.character, aw, proportional);
			}
			else
			{
				fm = &metrics[(unsigned char)curr->value.character];
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
			if (x_end > toWidth)
			{
				restricted = qtrue;
				break;
			}
			ax = x_end;
		}
		else if (curr->type == OSP_TEXT_CMD_SHADER)
		{
			ax += ah;
			if (ax >= toWidth)
			{
				restricted = qtrue;
				break;
			}
		}
	}

	if (!restricted)
	{
		return stopIdx;
	}

	while (i > 0)
	{
		float dotSize;
		float prevSize;

		dotSize = OSPSTR_GLYPH_W(metrics, '.', aw, proportional);
		prevSize = ospstr_get_symbol_width(cmd, i - 1, metrics, aw, ah, proportional);
		if ((ax - prevSize + dotSize) <= toWidth)
		{
			i--;
			ax = ax - prevSize + dotSize;
			break;
		}
		i--;
		ax -= prevSize;
	}

	curr = &cmd[i];
	curr->type = OSP_TEXT_CMD_CHAR;
	curr->value.character = '.';
	if (i + 1 < OSP_TEXT_CMD_MAX)
	{
		cmd[i + 1].type = OSP_TEXT_CMD_STOP;
	}
	return i + 1;
}

static int ospstr_restrict_chars(text_command_t* cmd, int stopIdx, int maxChars)
{
	int chars;
	int i;
	text_command_t* curr;
	qboolean restricted;

	if (!cmd || maxChars <= 0)
	{
		return stopIdx;
	}

	chars = 0;
	restricted = qfalse;
	for (i = 0; i < stopIdx; i++)
	{
		curr = &cmd[i];
		if (curr->type == OSP_TEXT_CMD_CHAR || curr->type == OSP_TEXT_CMD_SHADER)
		{
			chars++;
			if (chars >= maxChars)
			{
				restricted = qtrue;
				break;
			}
		}
	}

	if (!restricted)
	{
		return stopIdx;
	}

	curr = &cmd[i];
	if (curr->type == OSP_TEXT_CMD_CHAR)
	{
		curr->value.character = '.';
	}
	else if (curr->type == OSP_TEXT_CMD_SHADER)
	{
		curr->type = OSP_TEXT_CMD_CHAR;
		curr->value.character = '.';
	}
	if (i + 1 < OSP_TEXT_CMD_MAX)
	{
		cmd[i + 1].type = OSP_TEXT_CMD_STOP;
	}
	return i + 1;
}

static float ospstr_layout_commands(text_command_t* cmd, int* stopIdx, const cg_osp_font_metric_t* metrics,
                                    float aw, float ah, int proportional, float mw, int widthIsChars, int maxChars)
{
	int s;

	s = *stopIdx;
	if (widthIsChars)
	{
		s = ospstr_restrict_chars(cmd, s, maxChars);
	}
	else if (mw > 0.0f)
	{
		s = ospstr_restrict_pixels(cmd, s, metrics, aw, ah, proportional, mw);
	}
	*stopIdx = s;
	return ospstr_measure_commands(cmd, s, metrics, aw, ah, proportional, (float)cgs.glconfig.vidWidth);
}


static void ospstr_modulate_channel_alpha(vec4_t out, const vec4_t in, float textAlpha)
{
	Vector4Copy(in, out);
	out[3] = in[3] * textAlpha;
}

static float ospstr_decor_text_alpha(const vec4_t setColor, const text_command_t* cmd, int stopIdx)
{
	float maxAlpha;
	float fade;
	float alpha;
	int i;

	if (setColor)
	{
		maxAlpha = setColor[3];
		alpha = setColor[3];
	}
	else
	{
		maxAlpha = 1.0f;
		alpha = 1.0f;
	}
	fade = 1.0f;

	if (!cmd)
	{
		return alpha;
	}

	for (i = 0; i < stopIdx; i++)
	{
		if (cmd[i].type == OSP_TEXT_CMD_CHAR || cmd[i].type == OSP_TEXT_CMD_SHADER)
		{
			break;
		}
		if (cmd[i].type == OSP_TEXT_CMD_FADE)
		{
			fade = cmd[i].value.fade;
			alpha = fade;
			if (alpha > maxAlpha)
			{
				alpha = maxAlpha;
			}
		}
		else if (cmd[i].type == OSP_TEXT_CMD_TEXT_COLOR)
		{
			alpha = fade;
			if (alpha > maxAlpha)
			{
				alpha = maxAlpha;
			}
		}
	}

	return alpha;
}

static void ospstr_draw_decor(float ax, float ay, float expectedLength, float ah,
                              vec4_t background, vec4_t border, vec4_t borderColor, int hasBorder,
                              float textAlpha)
{
	vec4_t drawCol;

	if (background)
	{
		ospstr_modulate_channel_alpha(drawCol, background, textAlpha);
		if (drawCol[3] > 0.0f)
		{
			trap_R_SetColor(drawCol);
			trap_R_DrawStretchPic(ax, ay, expectedLength, ah, 0, 0, 0, 0, cgs.media.whiteShader);
			trap_R_SetColor(colorWhite);
		}
	}

	if (hasBorder && borderColor && (border[0] > 0.0f || border[1] > 0.0f || border[2] > 0.0f || border[3] > 0.0f))
	{
		ospstr_modulate_channel_alpha(drawCol, borderColor, textAlpha);
		if (drawCol[3] > 0.0f)
		{
			trap_R_SetColor(drawCol);
			if (border[0] > 0.0f)
			{
				trap_R_DrawStretchPic(ax - border[0], ay, border[0], ah, 0, 0, 0, 0, cgs.media.whiteShader);
			}
			if (border[1] > 0.0f)
			{
				trap_R_DrawStretchPic(ax - border[0], ay - border[1], expectedLength + border[0] + border[2], border[1],
				                      0, 0, 0, 0, cgs.media.whiteShader);
			}
			if (border[2] > 0.0f)
			{
				trap_R_DrawStretchPic(ax + expectedLength, ay, border[2], ah, 0, 0, 0, 0, cgs.media.whiteShader);
			}
			if (border[3] > 0.0f)
			{
				trap_R_DrawStretchPic(ax - border[0], ay + ah, expectedLength + border[0] + border[2], border[3],
				                      0, 0, 0, 0, cgs.media.whiteShader);
			}
			trap_R_SetColor(NULL);
		}
	}
}

static void ospstr_apply_vertical(float* ay, float ah, int flags)
{
	if (flags & DS_VCENTER)
	{
		*ay -= ah * 0.5f;
	}
	else if (flags & DS_VTOP)
	{
		*ay -= ah;
	}
}

static void ospstr_render_commands(text_command_t* text_commands, int stopIdx,
                                   float ax, float ay, float aw, float ah,
                                   const vec4_t setColor, vec4_t shadowColor, int flags,
                                   const cg_osp_font_metric_t* metrics, qhandle_t sh, int proportional)
{
	const cg_osp_font_metric_t* fm;
	const float* tc;
	float aw1;
	float fade;
	float shadowFade;
	float maxAlpha;
	float vidWidth;
	float xx_add;
	float yy_add;
	vec4_t color;
	vec4_t shadowCol;
	vec4_t emojiColor;
	int shadowEnabled;
	int i;
	int index;
	int pass;
	int passCount;
	text_command_t* curr;
	float startAx;

	shadowEnabled = (flags & DS_SHADOW) && (shadowColor != NULL) && (shadowColor[3] != 0.0f);
	if (shadowEnabled)
	{
		if (ospstr_eng)
		{
			xx_add = aw * ospstr_curTtfShadowX;
			yy_add = aw * ospstr_curTtfShadowY;
		}
		else
		{
			xx_add = aw * be_fontShadowOffX;
			yy_add = aw * be_fontShadowOffY;
		}
		VectorCopy(shadowColor, shadowCol);
		shadowFade = shadowColor[3];
	}
	else
	{
		shadowFade = 1.0f;
		xx_add = 0.0f;
		yy_add = 0.0f;
		Vector4Set(shadowCol, 0, 0, 0, 1);
	}

	fade = 1.0f;
	if (setColor)
	{
		Vector4Copy(setColor, color);
		maxAlpha = setColor[3];
	}
	else
	{
		Vector4Copy(colorWhite, color);
		maxAlpha = 1.0f;
	}

	if (shadowEnabled)
	{
		shadowCol[3] = shadowFade * color[3];
	}

	vidWidth = (float)cgs.glconfig.vidWidth;
	startAx = ax;
	passCount = shadowEnabled ? 2 : 1;

	for (pass = 0; pass < passCount; pass++)
	{
		fade = 1.0f;
		if (setColor)
		{
			Vector4Copy(setColor, color);
			maxAlpha = setColor[3];
		}
		else
		{
			Vector4Copy(colorWhite, color);
			maxAlpha = 1.0f;
		}
		if (shadowEnabled)
		{
			VectorCopy(shadowColor, shadowCol);
			shadowCol[3] = shadowFade * color[3];
		}
		ax = startAx;

		if (shadowEnabled && pass == 0)
		{
			trap_R_SetColor(shadowCol);
		}
		else
		{
			trap_R_SetColor(color);
		}

		for (i = 0; i < stopIdx; i++)
		{
			curr = &text_commands[i];
			switch (curr->type)
			{
				case OSP_TEXT_CMD_CHAR:
					index = curr->value.character;
					if (index < 0)
					{
						index += 256;
					}
					if (ospstr_eng)
					{
						{
							const fontAtlasGlyph_t* g;
							float                   gw;
							float                   gh;
							float                   gy;

							g = &ospstr_eng->glyphs[(unsigned char)index];
							if (g->glyph)
							{
								gw = (float)g->imageWidth * ospstr_engDrawUnit;
								gh = (float)g->imageHeight * ospstr_engDrawUnit;
								gy = ay + ospstr_engAsc - (float)g->top * ospstr_engDrawUnit + ospstr_engYBias;
								if (gw < 0.0f)
								{
									gw = 0.0f;
								}
								if (!proportional && gw < 1.0f && aw > 0.0f)
								{
									gw = aw;
								}
								if (gh < 1.0f)
								{
									gh = ah;
									gy = ay;
								}
								if (shadowEnabled && pass == 0)
								{
									trap_R_DrawStretchPic(ax + xx_add, gy + yy_add, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
								}
								else
								{
									trap_R_DrawStretchPic(ax, gy, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
								}
							}
						}
						ax += OSPSTR_GLYPH_W(metrics, index, aw, proportional);
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

						if (index == ' ')
						{
							/* an empty quad, nothing to send to the renderer */
						}
						else if (shadowEnabled && pass == 0)
						{
							trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
						}
						else
						{
							trap_R_DrawStretchPic(ax, ay, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
						}

						ax += proportional ? (fm->space1 + fm->space2) * aw : aw;
					}
					if (ax >= vidWidth)
					{
						break;
					}
					break;

				case OSP_TEXT_CMD_SHADER:
					aw1 = ah;
					if (shadowEnabled && pass == 0)
					{
						trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, 0, 0, 1, 1, curr->value.shader);
					}
					else
					{
						Vector4Copy(colorWhite, emojiColor);
						emojiColor[3] = color[3];
						trap_R_SetColor(emojiColor);
						trap_R_DrawStretchPic(ax, ay, aw1, ah, 0, 0, 1, 1, curr->value.shader);
						trap_R_SetColor(color);
					}
					ax += aw1;
					if (ax >= vidWidth)
					{
						break;
					}
					break;

				case OSP_TEXT_CMD_SHADOW_COLOR:
					VectorCopy(curr->value.color, shadowCol);
					shadowCol[3] = shadowFade * color[3];
					if (shadowEnabled && pass == 0)
					{
						trap_R_SetColor(shadowCol);
					}
					break;

				case OSP_TEXT_CMD_FADE:
					fade = curr->value.fade;
					color[3] = fade;
					if (color[3] > maxAlpha)
					{
						color[3] = maxAlpha;
					}
					if (shadowEnabled)
					{
						shadowCol[3] = shadowFade * color[3];
					}
					if (shadowEnabled && pass == 0)
					{
						trap_R_SetColor(shadowCol);
					}
					else
					{
						trap_R_SetColor(color);
					}
					break;

				case OSP_TEXT_CMD_TEXT_COLOR:
					VectorCopy(curr->value.color, color);
					color[3] = fade;
					if (color[3] > maxAlpha)
					{
						color[3] = maxAlpha;
					}
					if (shadowEnabled)
					{
						shadowCol[3] = shadowFade * color[3];
					}
					if (shadowEnabled && pass == 0)
					{
						trap_R_SetColor(shadowCol);
					}
					else
					{
						trap_R_SetColor(color);
					}
					break;

				default:
					break;
			}
		}
	}
}

static float ospstr_plain_measure(const char* string, const cg_osp_font_metric_t* metrics, float aw, int proportional,
                                  float maxWidth, int maxChars, int widthIsChars)
{
	const unsigned char* s;
	float ax;
	int chars;

	ax = 0.0f;
	chars = 0;
	s = (const unsigned char*)string;
	while (*s)
	{
		if (widthIsChars)
		{
			chars++;
			if (chars > maxChars)
			{
				break;
			}
		}
		ax += OSPSTR_GLYPH_W(metrics, *s, aw, proportional);
		if (!widthIsChars && maxWidth > 0.0f && ax > maxWidth)
		{
			break;
		}
		s++;
	}
	return ax;
}

static const char* ospstr_slice_end(const char* start, const char* end)
{
	if (end)
	{
		return end;
	}
	if (!start)
	{
		return start;
	}
	return start + strlen(start);
}

static float ospstr_plain_slice_measure(const unsigned char* start, const unsigned char* end,
                                        const cg_osp_font_metric_t* metrics, float aw, int proportional)
{
	const unsigned char* s;
	float ax;

	ax = 0.0f;
	s = start;
	while (s < end && *s)
	{
		ax += OSPSTR_GLYPH_W(metrics, *s, aw, proportional);
		s++;
	}
	return ax;
}

static qboolean ospstr_text_control_disabled_fast(char controlChar, int mask)
{
	if (!mask)
	{
		return qfalse;
	}

	switch (controlChar)
	{
		case 'B':
		case 'b':
			return (mask & CG_DTEF_BLINK) != 0;
		case 'F':
		case 'f':
			return (mask & CG_DTEF_FLASH) != 0;
		case 'N':
		case 'n':
			return (mask & CG_DTEF_RESET) != 0;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return (mask & CG_DTEF_PALETTE) != 0;
		case 'X':
		case 'x':
			return (mask & CG_DTEF_HEX) != 0;
		default:
			return qfalse;
	}
}

static float ospstr_rich_slice_measure(const char* start, const char* end, int flags,
                                       float aw, float ah, int proportional, const cg_osp_font_metric_t* metrics)
{
	const char* from;
	float ax;
	const emoji_t* em;
	int dteMask;
	int flashOn;
	int flashOff;
	qboolean emojiEnabled;

	if (!start)
	{
		return 0.0f;
	}

	if (!end)
	{
		end = start + strlen(start);
	}

	ax = 0.0f;
	from = start;
	dteMask = cg_disableTextEffects.integer;
	flashOn = ((ospstr_usedTime = qtrue, cg.time) & 0x3ff) >= 512;
	flashOff = !flashOn;
	emojiEnabled = (flags & DS_EMOJI) ? qtrue : qfalse;

	while (from < end && *from)
	{
		if (from[0] == '^' && from[1] && from[1] != '^')
		{
			if (ospstr_text_control_disabled_fast(from[1], dteMask))
			{
				from += 2;
				continue;
			}
			switch (from[1])
			{
				case 'f':
					from += 2;
					if (flashOn)
					{
						while (from < end && *from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'F')))
						{
							from++;
						}
					}
					continue;
				case 'F':
					from += 2;
					if (flashOff)
					{
						while (from < end && *from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'f')))
						{
							from++;
						}
					}
					continue;
				case 'B':
				case 'b':
				case 'N':
				case 'n':
					from += 2;
					continue;
				case '^':
					ax += OSPSTR_GLYPH_W(metrics, '^', aw, proportional);
					from += 1;
					continue;
				case 'X':
				case 'x':
					from += 8;
					continue;
				case 'c':
					if ((flags & DS_CTE_COLOR) && (from[2] == 'T' || from[2] == 'E'))
					{
						from += 3;
						continue;
					}
					/* fallthrough: palette letter c */
				default:
					from += 2;
					continue;
			}
		}

		if (emojiEnabled && (unsigned char)from[0] == (unsigned char)EMOJI_CHAR)
		{
			em = CG_OSPGetEmoji(from);
			if (em != NULL && from + em->len <= end)
			{
				ax += ah;
				from += em->len;
				continue;
			}
		}

		ax += OSPSTR_GLYPH_W(metrics, (unsigned char)from[0], aw, proportional);
		from++;
	}

	return ax;
}

static void ospstr_slice_setup(float charWidth, float charHeight, int flags, int fontIndex,
                               float* aw, float* ah, int* proportional,
                               const cg_osp_font_metric_t** metrics)
{
	*aw = charWidth;
	*ah = charHeight;
	if (*ah <= 0.0f)
	{
		*ah = *aw;
	}
	CG_AdjustFrom640(NULL, NULL, aw, ah);
	*proportional = (flags & DS_PROPORTIONAL) ? 1 : 0;
	ospstr_bind_engine(fontIndex, *aw, *ah, *proportional);
	if (ospstr_eng)
	{
		*metrics = NULL;
	}
	else
	{
		*metrics = CG_OSPFontMetricsPtr(fontIndex);
	}
}

static void ospstr_draw_plain_mono(const char* string, float ax, float ay, float aw, float ah,
                                   const vec4_t color, vec4_t shadowColor, int shadowEnabled,
                                   float xx_add, float yy_add, const cg_osp_font_metric_t* metrics,
                                   qhandle_t sh, float maxWidth, int maxChars, int widthIsChars, float vidWidth)
{
	const cg_osp_font_metric_t* fm;
	const float* tc;
	const unsigned char* s;
	float usedW;
	int chars;

	chars = 0;
	usedW = 0.0f;
	s = (const unsigned char*)string;
	{
		int pass;
		int passCount;
		float ax0;

		passCount = shadowEnabled ? 2 : 1;
		ax0 = ax;
		for (pass = 0; pass < passCount; pass++)
		{
			if (shadowEnabled && pass == 0)
			{
				trap_R_SetColor(shadowColor);
			}
			else
			{
				trap_R_SetColor(color);
			}
			ax = ax0;
			chars = 0;
			usedW = 0.0f;
			s = (const unsigned char*)string;
			while (*s)
			{
				if (widthIsChars)
				{
					chars++;
					if (chars > maxChars)
					{
						break;
					}
				}
				if (!widthIsChars && maxWidth > 0.0f && (usedW + aw) > maxWidth && *s != '.')
				{
					break;
				}

				if (ospstr_eng)
				{
					const fontAtlasGlyph_t* g;
					float                   gw;
					float                   gh;
					float                   gy;

					g = &ospstr_eng->glyphs[*s];
					if (g->glyph)
					{
						gw = (float)g->imageWidth * ospstr_engDrawUnit;
						gh = (float)g->imageHeight * ospstr_engDrawUnit;
						gy = ay + ospstr_engAsc - (float)g->top * ospstr_engDrawUnit + ospstr_engYBias;
						if (gw < 0.0f)
						{
							gw = 0.0f;
						}
						if (gw < 1.0f && aw > 0.0f)
						{
							gw = aw;
						}
						if (gh < 1.0f)
						{
							gh = ah;
							gy = ay;
						}
						if (shadowEnabled && pass == 0)
						{
							trap_R_DrawStretchPic(ax + xx_add, gy + yy_add, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
						}
						else
						{
							trap_R_DrawStretchPic(ax, gy, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
						}
					}
				}
				else
				{
					fm = &metrics[*s];
					tc = fm->tc_mono;
					if (*s == ' ')
					{
						/* an empty quad, nothing to send to the renderer */
					}
					else if (shadowEnabled && pass == 0)
					{
						trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw, ah, tc[0], tc[1], tc[2], tc[3], sh);
					}
					else
					{
						trap_R_DrawStretchPic(ax, ay, aw, ah, tc[0], tc[1], tc[2], tc[3], sh);
					}
				}
				ax += aw;
				usedW += aw;
				if (ax >= vidWidth)
				{
					break;
				}
				s++;
			}
		}
	}
}

static void ospstr_draw_plain_prop(const char* string, float ax, float ay, float aw, float ah,
                                   const vec4_t color, vec4_t shadowColor, int shadowEnabled,
                                   float xx_add, float yy_add, const cg_osp_font_metric_t* metrics,
                                   qhandle_t sh, float maxWidth, int maxChars, int widthIsChars, float vidWidth)
{
	const cg_osp_font_metric_t* fm;
	const float* tc;
	const unsigned char* s;
	float aw1;
	float advance;
	float usedW;
	int chars;

	chars = 0;
	usedW = 0.0f;
	s = (const unsigned char*)string;
	{
		int pass;
		int passCount;
		float ax0;

		passCount = shadowEnabled ? 2 : 1;
		ax0 = ax;
		for (pass = 0; pass < passCount; pass++)
		{
			if (shadowEnabled && pass == 0)
			{
				trap_R_SetColor(shadowColor);
			}
			else
			{
				trap_R_SetColor(color);
			}
			ax = ax0;
			chars = 0;
			usedW = 0.0f;
			s = (const unsigned char*)string;
			while (*s)
			{
				if (widthIsChars)
				{
					chars++;
					if (chars > maxChars)
					{
						break;
					}
				}

				if (ospstr_eng)
				{
					const fontAtlasGlyph_t* g;
					float                   gw;
					float                   gh;
					float                   gy;

					advance = ospstr_eng->glyphs[*s].xSkip * ospstr_engDrawUnit;
					if (!widthIsChars && maxWidth > 0.0f && (usedW + advance) > maxWidth && *s != '.')
					{
						break;
					}
					g = &ospstr_eng->glyphs[*s];
					if (g->glyph)
					{
						gw = (float)g->imageWidth * ospstr_engDrawUnit;
						gh = (float)g->imageHeight * ospstr_engDrawUnit;
						gy = ay + ospstr_engAsc - (float)g->top * ospstr_engDrawUnit + ospstr_engYBias;
						if (gw < 0.0f)
						{
							gw = 0.0f;
						}
						if (gh < 1.0f)
						{
							gh = ah;
							gy = ay;
						}
						if (shadowEnabled && pass == 0)
						{
							trap_R_DrawStretchPic(ax + xx_add, gy + yy_add, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
						}
						else
						{
							trap_R_DrawStretchPic(ax, gy, gw, gh, g->s, g->t, g->s2, g->t2, g->glyph);
						}
					}
				}
				else
				{
					fm = &metrics[*s];
					advance = (fm->space1 + fm->space2) * aw;
					if (!widthIsChars && maxWidth > 0.0f && (usedW + advance) > maxWidth && *s != '.')
					{
						break;
					}

					tc = fm->tc_prop;
					aw1 = fm->width * aw;
					if (*s == ' ')
					{
						/* an empty quad, nothing to send to the renderer */
					}
					else if (shadowEnabled && pass == 0)
					{
						trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
					}
					else
					{
						trap_R_DrawStretchPic(ax, ay, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
					}
				}
				ax += advance;
				usedW += advance;
				if (ax >= vidWidth)
				{
					break;
				}
				s++;
			}
		}
	}
}

static void ospstr_draw_plain(const char* string, float ax, float ay, float aw, float ah,
                              const vec4_t setColor, vec4_t shadowColor, int flags,
                              const cg_osp_font_metric_t* metrics, qhandle_t sh, int proportional,
                              float maxWidth, int maxChars, int widthIsChars)
{
	vec4_t color;
	vec4_t shadowCol;
	int shadowEnabled;
	float xx_add;
	float yy_add;
	float vidWidth;

	if (!setColor)
	{
		return;
	}

	shadowEnabled = (flags & DS_SHADOW) && (shadowColor != NULL) && (shadowColor[3] != 0.0f);
	Vector4Copy(setColor, color);
	vidWidth = (float)cgs.glconfig.vidWidth;
	if (shadowEnabled)
	{
		if (ospstr_eng)
		{
			xx_add = aw * ospstr_curTtfShadowX;
			yy_add = aw * ospstr_curTtfShadowY;
		}
		else
		{
			xx_add = aw * be_fontShadowOffX;
			yy_add = aw * be_fontShadowOffY;
		}
		Vector4Copy(shadowColor, shadowCol);
		shadowCol[3] = shadowColor[3] * color[3];
	}
	else
	{
		xx_add = 0.0f;
		yy_add = 0.0f;
	}

	if (proportional)
	{
		ospstr_draw_plain_prop(string, ax, ay, aw, ah, color, shadowCol, shadowEnabled,
		                       xx_add, yy_add, metrics, sh, maxWidth, maxChars, widthIsChars, vidWidth);
	}
	else
	{
		ospstr_draw_plain_mono(string, ax, ay, aw, ah, color, shadowCol, shadowEnabled,
		                       xx_add, yy_add, metrics, sh, maxWidth, maxChars, widthIsChars, vidWidth);
	}
}

/*
A very small TTF scale can round glyph advances down to zero while the glyphs still
draw at full width. Measure by character count then, so the background keeps its size.
*/
static float ospstr_fallback_width(const char* string, float aw, int maxWidth, int widthIsChars)
{
	int n = 0;

	while (string[n] && (!widthIsChars || n < maxWidth))
	{
		n++;
	}
	return (float)n * aw;
}

void CG_OSPDrawStringNew(float x, float y, const char* string, const vec4_t setColor,
                         vec4_t shadowColor,
                         float charWidth, float charHeight, int maxWidth, int flags,
                         int fontIndex,
                         vec4_t background, vec4_t border, vec4_t borderColor)
{
	text_command_t* text_commands;
	const cg_osp_font_metric_t* metrics;
	qhandle_t sh;
	float ax;
	float ay;
	float aw;
	float ah;
	float mw;
	float expectedLength;
	int proportional;
	int hasBorder;
	int stopIdx;
	int widthIsChars;
	int needWidth;
	int needLayout;
	float decorAlpha;

	if (!string)
	{
		return;
	}

	CG_AdjustFrom640(&x, &y, &charWidth, &charHeight);

	ax = x;
	ay = y;
	aw = charWidth;
	ah = charHeight;
	proportional = (flags & DS_PROPORTIONAL) ? 1 : 0;
	ospstr_bind_engine(fontIndex, aw, ah, proportional);
	if (ospstr_eng)
	{
		metrics = NULL;
		sh = 0;
	}
	else
	{
		metrics = CG_OSPFontMetricsPtr(fontIndex);
		sh = CG_OSPFontShaderForHeight(fontIndex, charHeight);
	}
	hasBorder = (border != NULL) ? 1 : 0;
	widthIsChars = (flags & DS_MAX_WIDTH_IS_CHARS) ? 1 : 0;
	expectedLength = 0.0f;

	if (!widthIsChars)
	{
		mw = (float)maxWidth;
		CG_AdjustFrom640(NULL, NULL, &mw, NULL);
	}
	else
	{
		mw = 0.0f;
	}

	needWidth = hasBorder || background || (flags & (DS_HCENTER | DS_HRIGHT));

	if (ospstr_is_plain(string, flags))
	{
		if (needWidth)
		{
			expectedLength = ospstr_plain_measure(string, metrics, aw, proportional,
			                                      widthIsChars ? 0.0f : mw, maxWidth, widthIsChars);
			if (expectedLength <= 0.0f && string[0])
			{
				expectedLength = ospstr_fallback_width(string, aw, maxWidth, widthIsChars);
			}
		}

		if (flags & (DS_HCENTER | DS_HRIGHT))
		{
			if (flags & DS_HCENTER)
			{
				ax -= 0.5f * expectedLength;
			}
			else
			{
				ax -= expectedLength;
			}
		}

		ospstr_apply_vertical(&ay, ah, flags);
		decorAlpha = setColor ? setColor[3] : 1.0f;
		ospstr_draw_decor(ax, ay, expectedLength, ah, background, border, borderColor, hasBorder, decorAlpha);
		ospstr_draw_plain(string, ax, ay, aw, ah, setColor, shadowColor, flags, metrics, sh, proportional,
		                  widthIsChars ? 0.0f : mw, maxWidth, widthIsChars);
		trap_R_SetColor(NULL);
		return;
	}

	stopIdx = ospstr_build_commands(string, flags, ospstr_cmdbuf);
	if (stopIdx < 0)
	{
		return;
	}
	text_commands = ospstr_cmdbuf;

	needLayout = needWidth || widthIsChars || (mw > 0.0f);
	if (needLayout)
	{
		expectedLength = ospstr_layout_commands(text_commands, &stopIdx, metrics, aw, ah, proportional, mw,
		                                        widthIsChars, maxWidth);
	}

	if (flags & (DS_HCENTER | DS_HRIGHT))
	{
		if (flags & DS_HCENTER)
		{
			ax -= 0.5f * expectedLength;
		}
		else
		{
			ax -= expectedLength;
		}
	}

	ospstr_apply_vertical(&ay, ah, flags);
	decorAlpha = ospstr_decor_text_alpha(setColor, text_commands, stopIdx);
	ospstr_draw_decor(ax, ay, expectedLength, ah, background, border, borderColor, hasBorder, decorAlpha);
	ospstr_render_commands(text_commands, stopIdx, ax, ay, aw, ah, setColor, shadowColor, flags, metrics, sh,
	                       proportional);
	trap_R_SetColor(NULL);
}

/* Five times for every string on the screen, and most of the colors are not given */
#define OSPSTR_REC_COLOR_SAME(stored, now, bit, hasColors) \
	((now) ? (((hasColors) & (bit)) && (stored)[0] == (now)[0] && (stored)[1] == (now)[1] && \
	          (stored)[2] == (now)[2] && (stored)[3] == (now)[3]) : !((hasColors) & (bit)))

static void ospstr_rec_color_store(float* stored, const float* now, int bit, int* hasColors)
{
	if (now)
	{
		Vector4Copy(now, stored);
		*hasColors |= bit;
	}
}

/* slot belongs to the caller: starts as 0, and -1 means no room for it */
void CG_OSPDrawStringCached(int* slot, float x, float y, const char* string, const vec4_t setColor,
                            vec4_t shadowColor, float charWidth, float charHeight, int maxWidth, int flags,
                            int fontIndex, vec4_t background, vec4_t border, vec4_t borderColor)
{
	ospstrRec_t* r;
	int          left;
	int          i;

	ospstr_ensure_ttf_tune();

	if (!*slot)
	{
		*slot = (ospstr_recsUsed < OSPSTR_REC_SLOTS) ? ++ospstr_recsUsed : -1;
	}
	if (*slot < 0 || !string)
	{
		CG_OSPDrawStringNew(x, y, string, setColor, shadowColor, charWidth, charHeight, maxWidth, flags, fontIndex, background, border, borderColor);
		return;
	}
	r = &ospstr_recs[*slot - 1];

	if (r->valid && r->x == x && r->y == y && r->w == charWidth && r->h == charHeight &&
	        r->maxWidth == maxWidth && r->flags == flags && r->fontIndex == fontIndex &&
	        r->tune == ospstr_ttfTuneGeneration && r->textEffects == cg_disableTextEffects.integer &&
	        r->shadowOffX == be_fontShadowOffX && r->shadowOffY == be_fontShadowOffY &&
	        !strcmp(r->text, string) &&
	        OSPSTR_REC_COLOR_SAME(r->setColor, setColor, 1, r->hasColors) &&
	        OSPSTR_REC_COLOR_SAME(r->shadowColor, shadowColor, 2, r->hasColors) &&
	        OSPSTR_REC_COLOR_SAME(r->background, background, 4, r->hasColors) &&
	        OSPSTR_REC_COLOR_SAME(r->border, border, 8, r->hasColors) &&
	        OSPSTR_REC_COLOR_SAME(r->borderColor, borderColor, 16, r->hasColors))
	{
		for (i = 0; i < r->numOps; i++)
		{
			const ospstrRecOp_t* op = &r->ops[i];

			if (op->shader == OSPSTR_OP_COLOR)
			{
				trap_R_SetColor(op->v);
			}
			else if (op->shader == OSPSTR_OP_COLOR_NULL)
			{
				trap_R_SetColor(NULL);
			}
			else
			{
				trap_R_DrawStretchPic(op->v[0], op->v[1], op->v[2], op->v[3], op->v[4], op->v[5], op->v[6], op->v[7], op->shader);
			}
		}
		return;
	}

	/* Time going back in a killcam puts this out of range, and then the element is tried again */
	left = r->offUntil - cg.time;
	r->valid = qfalse;
	if (left > 0 && left <= OSPSTR_REC_OFF_MS)
	{
		CG_OSPDrawStringNew(x, y, string, setColor, shadowColor, charWidth, charHeight, maxWidth, flags, fontIndex, background, border, borderColor);
		return;
	}
	if (cg.time >= r->lastMiss && cg.time - r->lastMiss < OSPSTR_REC_MISS_MS)
	{
		r->offUntil = cg.time + OSPSTR_REC_OFF_MS;
	}
	r->lastMiss = cg.time;

	if (strlen(string) >= OSPSTR_REC_TEXT)
	{
		CG_OSPDrawStringNew(x, y, string, setColor, shadowColor, charWidth, charHeight, maxWidth, flags, fontIndex, background, border, borderColor);
		return;
	}

	r->numOps = 0;
	ospstr_recLastColor = -1;
	ospstr_rec = r;
	ospstr_recFailed = qfalse;
	ospstr_usedTime = qfalse;
	CG_OSPDrawStringNew(x, y, string, setColor, shadowColor, charWidth, charHeight, maxWidth, flags, fontIndex, background, border, borderColor);
	ospstr_rec = NULL;
	if (ospstr_recFailed || ospstr_usedTime)
	{
		return;
	}

	r->x = x;
	r->y = y;
	r->w = charWidth;
	r->h = charHeight;
	r->maxWidth = maxWidth;
	r->flags = flags;
	r->fontIndex = fontIndex;
	r->tune = ospstr_ttfTuneGeneration;
	r->textEffects = cg_disableTextEffects.integer;
	r->shadowOffX = be_fontShadowOffX;
	r->shadowOffY = be_fontShadowOffY;
	Q_strncpyz(r->text, string, sizeof(r->text));
	r->hasColors = 0;
	ospstr_rec_color_store(r->setColor, setColor, 1, &r->hasColors);
	ospstr_rec_color_store(r->shadowColor, shadowColor, 2, &r->hasColors);
	ospstr_rec_color_store(r->background, background, 4, &r->hasColors);
	ospstr_rec_color_store(r->border, border, 8, &r->hasColors);
	ospstr_rec_color_store(r->borderColor, borderColor, 16, &r->hasColors);
	r->valid = qtrue;
}

float CG_OSPDrawStringNewTextWidth(const char* string, float charWidth, float charHeight, int flags, int fontIndex)
{
	const cg_osp_font_metric_t* metrics;
	int proportional;
	int stopIdx;
	float aw;
	float ah;
	float width;

	if (!string || !string[0])
	{
		return 0.0f;
	}

	aw = charWidth;
	ah = charHeight;
	if (ah <= 0.0f)
	{
		ah = aw;
	}

	CG_AdjustFrom640(NULL, NULL, &aw, &ah);

	proportional = (flags & DS_PROPORTIONAL) ? 1 : 0;
	ospstr_bind_engine(fontIndex, aw, ah, proportional);
	if (ospstr_eng)
	{
		metrics = NULL;
	}
	else
	{
		metrics = CG_OSPFontMetricsPtr(fontIndex);
	}

	if (ospstr_is_plain(string, flags))
	{
		width = ospstr_plain_measure(string, metrics, aw, proportional, 0.0f, 0, 0);
	}
	else
	{
		stopIdx = ospstr_build_commands(string, flags, ospstr_cmdbuf);
		if (stopIdx < 0)
		{
			return 0.0f;
		}

		width = ospstr_measure_commands(ospstr_cmdbuf, stopIdx, metrics, aw, ah, proportional,
		                                (float)cgs.glconfig.vidWidth);
	}

	if (cgs.screenXScale_Old > 0.0f)
	{
		width /= cgs.screenXScale_Old;
	}

	return width;
}

float CG_OSPDrawStringNewSliceWidth(const char* start, const char* end, float charWidth, float charHeight,
                                    int flags, int fontIndex)
{
	float aw;
	float ah;
	float width;
	int proportional;
	const cg_osp_font_metric_t* metrics;
	const char* sliceEnd;
	const unsigned char* pStart;
	const unsigned char* pEnd;

	if (!start || !start[0])
	{
		return 0.0f;
	}

	sliceEnd = ospstr_slice_end(start, end);
	if (sliceEnd <= start)
	{
		return 0.0f;
	}

	ospstr_slice_setup(charWidth, charHeight, flags, fontIndex, &aw, &ah, &proportional, &metrics);

	pStart = (const unsigned char*)start;
	pEnd = (const unsigned char*)sliceEnd;

	if (ospstr_is_plain(start, flags))
	{
		width = ospstr_plain_slice_measure(pStart, pEnd, metrics, aw, proportional);
	}
	else
	{
		width = ospstr_rich_slice_measure(start, sliceEnd, flags, aw, ah, proportional, metrics);
	}

	if (cgs.screenXScale_Old > 0.0f)
	{
		width /= cgs.screenXScale_Old;
	}

	return width;
}
