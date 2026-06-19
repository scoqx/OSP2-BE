/*
===========================================================================
OSP proportional string draw (CG_OSPDrawStringNew)
Single-pass parse; plain-text fast path; static command buffer.
===========================================================================
*/

#include "cg_local.h"

static text_command_t ospstr_cmdbuf[OSP_TEXT_CMD_MAX];

static qboolean ospstr_is_plain(const char* string, int flags)
{
	const unsigned char* p;

	if (!string)
	{
		return qfalse;
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

static float ospstr_glyph_width(const cg_osp_font_metric_t* metrics, int ch, float aw, int proportional)
{
	const cg_osp_font_metric_t* fm;

	fm = &metrics[(unsigned char)ch];
	if (proportional)
	{
		return (fm->space1 + fm->space2) * aw;
	}
	return aw;
}

#define OSPSTR_GLYPH_W(fm, aw, prop) ((prop) ? (((fm).space1 + (fm).space2) * (aw)) : (aw))

static float ospstr_shader_width(float ah)
{
	return ah;
}

static float ospstr_metric_advance(const cg_osp_font_metric_t* fm, float aw, int proportional)
{
	if (proportional)
	{
		return (fm->space1 + fm->space2) * aw;
	}
	return aw;
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
			if (CG_TextControlDisabled(from[1]))
			{
				from += 2;
				continue;
			}
			switch (from[1])
			{
				case 'f':
					from += 2;
					if ((cg.time & 0x3ff) >= 512)
					{
						while (*from && !(from[0] == '^' && (from[1] == 'N' || from[1] == 'F')))
						{
							from++;
						}
					}
					continue;
				case 'F':
					from += 2;
					if ((cg.time & 0x3ff) < 512)
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
				default:
					if (!CG_TextControlDisabled(from[1]) && ospstr_is_palette_color_char(from[1]))
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
			ax += ospstr_glyph_width(metrics, cmd[i].value.character, aw, proportional);
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
		return ospstr_glyph_width(metrics, cmd[index].value.character, aw, proportional);
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

		dotSize = ospstr_glyph_width(metrics, '.', aw, proportional);
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

static void ospstr_update_shadow_alpha(vec4_t shadowCol, float shadowFade, float textAlpha)
{
	shadowCol[3] = shadowFade * textAlpha;
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
	text_command_t* curr;

	shadowEnabled = (flags & DS_SHADOW) && (shadowColor != NULL) && (shadowColor[3] != 0.0f);
	if (shadowEnabled)
	{
		xx_add = aw / 10.0f;
		yy_add = xx_add;
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
		ospstr_update_shadow_alpha(shadowCol, shadowFade, color[3]);
	}

	vidWidth = (float)cgs.glconfig.vidWidth;
	trap_R_SetColor(color);

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

				if (shadowEnabled)
				{
					trap_R_SetColor(shadowCol);
					trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
					trap_R_SetColor(color);
				}
				trap_R_DrawStretchPic(ax, ay, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);

				ax += ospstr_metric_advance(fm, aw, proportional);
				if (ax >= vidWidth)
				{
					return;
				}
				break;

			case OSP_TEXT_CMD_SHADER:
				aw1 = ah;
				if (shadowEnabled)
				{
					trap_R_SetColor(shadowCol);
					trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, 0, 0, 1, 1, curr->value.shader);
				}
				Vector4Copy(colorWhite, emojiColor);
				emojiColor[3] = color[3];
				trap_R_SetColor(emojiColor);
				trap_R_DrawStretchPic(ax, ay, aw1, ah, 0, 0, 1, 1, curr->value.shader);
				trap_R_SetColor(color);
				ax += aw1;
				if (ax >= vidWidth)
				{
					return;
				}
				break;

			case OSP_TEXT_CMD_SHADOW_COLOR:
				VectorCopy(curr->value.color, shadowCol);
				ospstr_update_shadow_alpha(shadowCol, shadowFade, color[3]);
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
					ospstr_update_shadow_alpha(shadowCol, shadowFade, color[3]);
				}
				trap_R_SetColor(color);
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
					ospstr_update_shadow_alpha(shadowCol, shadowFade, color[3]);
				}
				trap_R_SetColor(color);
				break;

			default:
				break;
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
		ax += ospstr_glyph_width(metrics, *s, aw, proportional);
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
		const cg_osp_font_metric_t* fm;

		fm = &metrics[*s];
		ax += OSPSTR_GLYPH_W((*fm), aw, proportional);
		s++;
	}
	return ax;
}

static int ospstr_plain_slice_fit_bytes(const unsigned char* start, const unsigned char* end,
                                        const cg_osp_font_metric_t* metrics, float aw, int proportional,
                                        float limitPx)
{
	const unsigned char* s;
	const unsigned char* lastFit;
	float ax;
	float gw;

	if (limitPx <= 0.0f)
	{
		return 0;
	}

	ax = 0.0f;
	lastFit = start;
	s = start;
	while (s < end && *s)
	{
		const cg_osp_font_metric_t* fm;

		fm = &metrics[*s];
		gw = OSPSTR_GLYPH_W((*fm), aw, proportional);
		if (ax + gw > limitPx)
		{
			break;
		}
		ax += gw;
		s++;
		lastFit = s;
	}

	if (lastFit == start && s < end && *s)
	{
		return 1;
	}
	return (int)(lastFit - start);
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

static float ospstr_rich_slice_measure_fit(const char* start, const char* end, int flags,
        float aw, float ah, int proportional, const cg_osp_font_metric_t* metrics,
        qboolean limitWidth, float limitPx, int* outFitBytes)
{
	const char* from;
	const char* lastFit;
	float ax;
	float gw;
	const emoji_t* em;
	int dteMask;
	int flashOn;
	int flashOff;
	qboolean emojiEnabled;

	if (!start)
	{
		if (outFitBytes)
		{
			*outFitBytes = 0;
		}
		return 0.0f;
	}

	if (!end)
	{
		end = start + strlen(start);
	}

	ax = 0.0f;
	lastFit = start;
	from = start;
	dteMask = cg_disableTextEffects.integer;
	flashOn = (cg.time & 0x3ff) >= 512;
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
					{
						const cg_osp_font_metric_t* fm;

						fm = &metrics[(unsigned char)'^'];
						gw = OSPSTR_GLYPH_W((*fm), aw, proportional);
					}
					if (limitWidth && ax + gw > limitPx)
					{
						goto done;
					}
					ax += gw;
					lastFit = from + 1;
					from += 1;
					continue;
				case 'X':
				case 'x':
					from += 8;
					continue;
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
				gw = ah;
				if (limitWidth && ax + gw > limitPx)
				{
					goto done;
				}
				ax += gw;
				lastFit = from + em->len;
				from += em->len;
				continue;
			}
		}

		{
			const cg_osp_font_metric_t* fm;

			fm = &metrics[(unsigned char)from[0]];
			gw = OSPSTR_GLYPH_W((*fm), aw, proportional);
		}
		if (limitWidth && ax + gw > limitPx)
		{
			break;
		}
		ax += gw;
		lastFit = from + 1;
		from++;
	}

done:
	if (outFitBytes)
	{
		int fit;

		fit = (int)(lastFit - start);
		if (fit < 1 && from < end && *from)
		{
			fit = 1;
		}
		*outFitBytes = fit;
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
	*metrics = CG_OSPFontMetricsPtr(fontIndex);
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
	trap_R_SetColor(color);
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

		fm = &metrics[*s];
		tc = fm->tc_mono;
		if (shadowEnabled)
		{
			trap_R_SetColor(shadowColor);
			trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw, ah, tc[0], tc[1], tc[2], tc[3], sh);
			trap_R_SetColor(color);
		}
		trap_R_DrawStretchPic(ax, ay, aw, ah, tc[0], tc[1], tc[2], tc[3], sh);
		ax += aw;
		usedW += aw;
		if (ax >= vidWidth)
		{
			break;
		}
		s++;
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
	trap_R_SetColor(color);
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

		fm = &metrics[*s];
		advance = ospstr_metric_advance(fm, aw, 1);
		if (!widthIsChars && maxWidth > 0.0f && (usedW + advance) > maxWidth && *s != '.')
		{
			break;
		}

		tc = fm->tc_prop;
		aw1 = fm->width * aw;
		if (shadowEnabled)
		{
			trap_R_SetColor(shadowColor);
			trap_R_DrawStretchPic(ax + xx_add, ay + yy_add, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
			trap_R_SetColor(color);
		}
		trap_R_DrawStretchPic(ax, ay, aw1, ah, tc[0], tc[1], tc[2], tc[3], sh);
		ax += advance;
		usedW += advance;
		if (ax >= vidWidth)
		{
			break;
		}
		s++;
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
		xx_add = aw / 10.0f;
		yy_add = xx_add;
		Vector4Copy(shadowColor, shadowCol);
		ospstr_update_shadow_alpha(shadowCol, shadowColor[3], color[3]);
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

	metrics = CG_OSPFontMetricsPtr(fontIndex);
	sh = CG_OSPFontShaderForHeight(fontIndex, charHeight);

	ax = x;
	ay = y;
	aw = charWidth;
	ah = charHeight;
	proportional = (flags & DS_PROPORTIONAL) ? 1 : 0;
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
	metrics = CG_OSPFontMetricsPtr(fontIndex);

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
		width = ospstr_rich_slice_measure_fit(start, sliceEnd, flags, aw, ah, proportional, metrics,
		                                      qfalse, 0.0f, NULL);
	}

	if (cgs.screenXScale_Old > 0.0f)
	{
		width /= cgs.screenXScale_Old;
	}

	return width;
}

int CG_OSPDrawStringNewFitBytes(const char* start, const char* end, float maxVirtWidth, float charWidth,
                                float charHeight, int flags, int fontIndex)
{
	float aw;
	float ah;
	float limitPx;
	int proportional;
	int fitBytes;
	const cg_osp_font_metric_t* metrics;
	const char* sliceEnd;
	const unsigned char* pStart;
	const unsigned char* pEnd;

	if (!start || !start[0] || maxVirtWidth <= 0.0f)
	{
		return 0;
	}

	sliceEnd = ospstr_slice_end(start, end);
	if (sliceEnd <= start)
	{
		return 0;
	}

	ospstr_slice_setup(charWidth, charHeight, flags, fontIndex, &aw, &ah, &proportional, &metrics);
	limitPx = maxVirtWidth * cgs.screenXScale_Old;

	pStart = (const unsigned char*)start;
	pEnd = (const unsigned char*)sliceEnd;

	if (ospstr_is_plain(start, flags))
	{
		fitBytes = ospstr_plain_slice_fit_bytes(pStart, pEnd, metrics, aw, proportional, limitPx);
	}
	else
	{
		(void)ospstr_rich_slice_measure_fit(start, sliceEnd, flags, aw, ah, proportional, metrics,
		                                    qtrue, limitPx, &fitBytes);
	}

	if (fitBytes < 1 && sliceEnd > start && *start)
	{
		fitBytes = 1;
	}
	if (fitBytes > (int)(sliceEnd - start))
	{
		fitBytes = (int)(sliceEnd - start);
	}

	return fitBytes;
}
