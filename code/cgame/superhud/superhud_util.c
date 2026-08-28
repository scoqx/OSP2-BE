#include "superhud_private.h"
#include "../cg_local.h"
#include "../../qcommon/qcommon.h"

typedef struct
{
	vec4_t bar1_value;
	vec4_t bar1_bottom;
	vec4_t bar2_value;
	vec4_t bar2_bottom;
} drawBarCoords_t;

// static void CG_SHUDConfigPickColor(const superhudConfig_t* config, float* color, qboolean alphaOverride)
// {
//  const superhudColor_t* in = &config->color.value;
//  const float* target;
//  team_t team;
//  float finalAlpha = 1.0f;

//  if (!config->color.isSet)
//  {
//      if (alphaOverride)
//      {
//          Vector4Copy(colorWhite, color);
//      }
//      else
//      {
//          VectorCopy(colorWhite, color);
//      }
//      return;
//  }

//  switch (in->type)
//  {
//      case SUPERHUD_COLOR_RGBA:
//          target = in->rgba;
//          finalAlpha = in->rgba[3]; // take alpha from source color
//          break;

//      case SUPERHUD_COLOR_T:
//          team = CG_SHUDGetOurActiveTeam();
//          target = (team == TEAM_BLUE) ? colorBlue : colorRed;
//          break;

//      case SUPERHUD_COLOR_E:
//          team = CG_SHUDGetOurActiveTeam();
//          target = (team == TEAM_BLUE) ? colorRed : colorBlue;
//          break;

//      case SUPERHUD_COLOR_I:
//          target = colorWhite;
//          break;

//      default:
//          target = colorWhite;
//          break;
//  }

//  // If color2 is set and source color is a letter (T, E, I), replace alpha only
//  if (config->color2.isSet && in->type != SUPERHUD_COLOR_RGBA)
//  {
//      finalAlpha = config->color2.value.rgba[3];
//  }

//  // Apply color
//  if (alphaOverride)
//  {
//      color[0] = target[0];
//      color[1] = target[1];
//      color[2] = target[2];
//      color[3] = finalAlpha;
//  }
//  else
//  {
//      color[0] = target[0];
//      color[1] = target[1];
//      color[2] = target[2];
//  }
// }

// static void CG_SHUDConfigPickBgColor(const superhudConfig_t* config, float* color, qboolean alphaOverride)
// {
//  const superhudColor_t* in = &config->bgcolor.value;
//  const float* target;
//  team_t team;
//  float finalAlpha = 1.0f;  // default 1

//  if (!config->bgcolor.isSet)
//  {
//      if (alphaOverride)
//      {
//          Vector4Copy(colorWhite, color);
//      }
//      else
//      {
//          VectorCopy(colorWhite, color);
//      }
//      return;
//  }

//  switch (in->type)
//  {
//      case SUPERHUD_COLOR_RGBA:
//          target = in->rgba;
//          finalAlpha = in->rgba[3];  // take alpha from source color
//          break;

//      case SUPERHUD_COLOR_T:
//          team = CG_SHUDGetOurActiveTeam();
//          target = (team == TEAM_BLUE) ? colorBlue : colorRed;
//          break;

//      case SUPERHUD_COLOR_E:
//          team = CG_SHUDGetOurActiveTeam();
//          target = (team == TEAM_BLUE) ? colorRed : colorBlue;
//          break;

//      case SUPERHUD_COLOR_I:
//      default:
//          target = colorWhite;
//          break;
//  }

//  // If bgcolor2 is set and source color is not RGBA, replace alpha only
//  if (config->bgcolor2.isSet && in->type != SUPERHUD_COLOR_RGBA)
//  {
//      finalAlpha = config->bgcolor2.value.rgba[3];
//  }

//  if (alphaOverride)
//  {
//      color[0] = target[0];
//      color[1] = target[1];
//      color[2] = target[2];
//      color[3] = finalAlpha;
//  }
//  else
//  {
//      color[0] = target[0];
//      color[1] = target[1];
//      color[2] = target[2];
//      color[3] = finalAlpha;  // always set alpha here too
//  }
// }

static void CG_SHUDConfigPickColorGeneric(
    const qboolean* mainIsSet,
    const superhudColor_t* mainValue,
    const qboolean* altIsSet,
    const superhudColor_t* altValue,
    float* color,
    qboolean alphaOverride)
{
	const superhudColor_t* in = mainValue;
	const float* target;
	team_t team;
	float finalAlpha = 1.0f;

	if (!(*mainIsSet))
	{
		if (alphaOverride)
		{
			Vector4Copy(colorWhite, color);
		}
		else
		{
			VectorCopy(colorWhite, color);
		}
		return;
	}

	switch (in->type)
	{
		case SUPERHUD_COLOR_RGBA:
			target = in->rgba;
			finalAlpha = in->rgba[3];
			break;

		case SUPERHUD_COLOR_T:
			team = CG_SHUDGetOurActiveTeam();
			target = (team == TEAM_BLUE) ? cgs.beBlueTeamColor : cgs.beRedTeamColor;
			break;

		case SUPERHUD_COLOR_E:
			team = CG_SHUDGetOurActiveTeam();
			target = (team == TEAM_BLUE) ? cgs.beRedTeamColor : cgs.beBlueTeamColor;
			break;

		case SUPERHUD_COLOR_I:
		default:
			target = colorWhite;
			break;
	}

	if ((*altIsSet) && in->type != SUPERHUD_COLOR_RGBA)
	{
		finalAlpha = altValue->rgba[3];
	}

	if (alphaOverride)
	{
		color[0] = target[0];
		color[1] = target[1];
		color[2] = target[2];
		color[3] = finalAlpha;
	}
	else
	{
		color[0] = target[0];
		color[1] = target[1];
		color[2] = target[2];
		color[3] = finalAlpha;
	}
}
void CG_SHUDConfigPickBgColor(const superhudConfig_t* config, float* color, qboolean alphaOverride)
{
	CG_SHUDConfigPickColorGeneric(&config->bgcolor.isSet, &config->bgcolor.value,
	                              &config->bgcolor2.isSet, &config->bgcolor2.value,
	                              color, alphaOverride);
}

void CG_SHUDConfigPickBorderColor(const superhudConfig_t* config, float* color, qboolean alphaOverride)
{
	CG_SHUDConfigPickColorGeneric(&config->borderColor.isSet, &config->borderColor.value,
	                              &config->borderColor2.isSet, &config->borderColor2.value,
	                              color, alphaOverride);
}

/*
=================
CG_SHUDConfigPickColor_Init

Static initialization - sets full RGBA color (including alpha)
Used during context creation to establish baseline color
=================
*/
void CG_SHUDConfigPickColor_Init(const superhudConfig_t* config, vec4_t color)
{
	CG_SHUDConfigPickColorGeneric(&config->color.isSet, &config->color.value,
	                              &config->color2.isSet, &config->color2.value,
	                              color, qtrue);
}

/*
=================
CG_SHUDConfigPickColor_Runtime

Dynamic rendering - updates only RGB channels, preserves alpha for Fade animations
Used during frame rendering to pick color without disrupting fade transitions
=================
*/
void CG_SHUDConfigPickColor_Runtime(const superhudConfig_t* config, vec4_t color)
{
	vec4_t temp;
	float preservedAlpha;

	/* Save current alpha for restoration */
	preservedAlpha = color[3];

	/* Compute full color including alpha */
	CG_SHUDConfigPickColorGeneric(&config->color.isSet, &config->color.value,
	                              &config->color2.isSet, &config->color2.value,
	                              temp, qtrue);

	/* Copy RGB only, preserve alpha for fade animations */
	color[0] = temp[0];
	color[1] = temp[1];
	color[2] = temp[2];
	color[3] = preservedAlpha;
}

static void CG_SHUDConfigDefaultsCheck(superhudConfig_t* config)
{
	if (!config->rect.isSet)
	{
		config->rect.value[0] = 0.0f;
		config->rect.value[1] = 0.0f;
		config->rect.value[2] = 320.0f;
		config->rect.value[3] = 12.0f;
		config->rect.isSet = qtrue;
	}

	if (!config->textAlign.isSet)
	{
		config->textAlign.value = SUPERHUD_ALIGNH_LEFT;
		config->textAlign.isSet = qtrue;
	}

	/* if fontsize wasn't set, find max size for rect */
	if (!config->fontsize.isSet)
	{
		config->fontsize.value[1] = config->rect.value[3];
		config->fontsize.value[0] = config->fontsize.value[1] / 1.618f;
		config->fontsize.isSet = qtrue;
	}

	if (!config->textStyle.isSet)
	{
		config->textStyle.value = 0; //drop shadow
		config->textStyle.isSet = qtrue;
	}

	if (!config->color.isSet)
	{
		Vector4Copy(colorWhite, config->color.value.rgba);
		config->color.value.type = SUPERHUD_COLOR_RGBA;
		config->color.isSet = qtrue;
	}
}

static void CG_SHUDTextMakeAdjustCoords(const superhudConfig_t* in, float* out_x, float* out_y)
{
	superhudAlignH_t h;
	superhudAlignV_t v;

	if (!in->rect.isSet)
	{
		return;
	}

	if (!in->alignH.isSet)
	{
		h = SUPERHUD_ALIGNH_LEFT;
	}
	else
	{
		h = in->alignH.value;
	}

	if (!in->alignV.isSet)
	{
		v = SUPERHUD_ALIGNV_CENTER;
	}
	else
	{
		v = in->alignV.value;
	}

	switch (h)
	{
		case SUPERHUD_ALIGNH_LEFT:
			//allready x
			*out_x = in->rect.value[0];
			break;
		case SUPERHUD_ALIGNH_CENTER:
			// x + width/2
			*out_x = in->rect.value[0] + in->rect.value[2] / 2.0f;
			break;
		case SUPERHUD_ALIGNH_RIGHT:
			// x + width
			*out_x = in->rect.value[0] + in->rect.value[2];
			break;
	}

	switch (v)
	{
		case SUPERHUD_ALIGNV_TOP:
			*out_y = in->rect.value[1];
			break;
		case SUPERHUD_ALIGNV_CENTER:
			*out_y = in->rect.value[1] + in->rect.value[3] / 2.0f;
			break;
		case SUPERHUD_ALIGNV_BOTTOM:
			*out_y = in->rect.value[1] + in->rect.value[3];
			break;
	}

}

void CG_SHUDTextMakeContext(const superhudConfig_t* in, superhudTextContext_t* out)
{
	superhudConfig_t config;
	memset(out, 0, sizeof(*out));
	memcpy(&config, in, sizeof(config));

	CG_SHUDConfigDefaultsCheck(&config);

	CG_SHUDTextMakeAdjustCoords(in, &out->coord.named.x, &out->coord.named.y);

	switch (config.textAlign.value)
	{
		default:
		case SUPERHUD_ALIGNH_LEFT:
			out->flags |= DS_HLEFT;
			break;
		case SUPERHUD_ALIGNH_CENTER:
			out->flags |= DS_HCENTER;
			break;
		case SUPERHUD_ALIGNH_RIGHT:
			out->flags |= DS_HRIGHT;
			break;
	}

	out->coord.named.w = config.fontsize.value[0];
	out->coord.named.h = config.fontsize.value[1];

	if (config.alignV.isSet)
	{
		switch (config.alignV.value)
		{
			default:
			case SUPERHUD_ALIGNV_TOP:
				out->flags |= DS_VTOP;
				break;
			case SUPERHUD_ALIGNV_CENTER:
				out->flags |= DS_VCENTER;
				break;
			case SUPERHUD_ALIGNV_BOTTOM:
				out->flags |= DS_VBOTTOM;
				break;
		}
	}

	if (!config.monospace.isSet)
	{
		out->flags |= DS_PROPORTIONAL;
	}
	if (config.textStyle.value & 1)
	{
		out->flags |= DS_SHADOW;
	}

	if (!config.shadowColor.isSet)
	{
		Vector4Copy(colorBlack, out->shadowColor);
		if (config.color.isSet)
		{
			out->shadowColor[3] = config.color.value.rgba[3];
		}
	}
	else
	{
		Vector4Copy(config.shadowColor.value, out->shadowColor);
	}

	out->fontIndex = CG_FontIndexFromString(config.font.isSet ? config.font.value : "sansman");
	out->width = SCREEN_WIDTH;


	CG_SHUDConfigPickColor_Init(&config, out->color);
	Vector4Copy(out->color, out->color_origin);
}


static void CG_SHUDGetImageAlignedRect(const superhudConfig_t* in, float* x, float* y, float* w, float* h)
{
	if (!in->rect.isSet)
	{
		return;
	}

	*w = in->rect.value[2];
	*h = in->rect.value[3];
	*x = in->rect.value[0];
	*y = in->rect.value[1];

	if (in->imageAlignH.isSet)
	{
		switch (in->imageAlignH.value)
		{
			default:
			case SUPERHUD_ALIGNH_LEFT:
				*x = in->rect.value[0];
				break;
			case SUPERHUD_ALIGNH_CENTER:
				*x = in->rect.value[0] - (*w) * 0.5f;
				break;
			case SUPERHUD_ALIGNH_RIGHT:
				*x = in->rect.value[0] - (*w);
				break;
		}
	}

	if (in->imageAlignV.isSet)
	{
		switch (in->imageAlignV.value)
		{
			default:
			case SUPERHUD_ALIGNV_TOP:
				*y = in->rect.value[1];
				break;
			case SUPERHUD_ALIGNV_CENTER:
				*y = in->rect.value[1] - (*h) * 0.5f;
				break;
			case SUPERHUD_ALIGNV_BOTTOM:
				*y = in->rect.value[1] - (*h);
				break;
		}
	}
}

void CG_SHUDDrawMakeContext(const superhudConfig_t* in, superhudDrawContext_t* out)
{
	superhudConfig_t config;
	float x;
	float y;
	float w;
	float h;

	memset(out, 0, sizeof(*out));
	memcpy(&config, in, sizeof(config));

	CG_SHUDConfigDefaultsCheck(&config);

	CG_SHUDGetImageAlignedRect(in, &x, &y, &w, &h);

	out->coord.named.x = x;
	out->coord.named.y = y;
	out->coord.named.w = w;
	out->coord.named.h = h;

	out->coordPicture.named.x = 0.0f;
	out->coordPicture.named.y = 0.0f;
	out->coordPicture.named.w = 1.0f;
	out->coordPicture.named.h = 1.0f;

	CG_SHUDConfigPickColor_Init(&config, out->color);
	Vector4Copy(out->color, out->color_origin);
}

/*
================
CG_SHUDBarMakeContext
================
*/
void CG_SHUDBarMakeContext(const superhudConfig_t* in, superhudBarContext_t* out, float max)
{
	float x = 0, y = 0;
	float bar_height, bar_width;
	float boxW;
	float boxH;
	superhudConfig_t config;
	memset(out, 0, sizeof(*out));
	memcpy(&config, in, sizeof(config));

	CG_SHUDConfigDefaultsCheck(&config);

	if (!config.direction.isSet)
	{
		config.direction.isSet = qtrue;
		config.direction.value = SUPERHUD_DIR_LEFT_TO_RIGHT;
	}

	if (!config.style.isSet) // set default style
	{
		config.style.isSet = qtrue;
		config.style.value = 1;
	}
	else if (config.style.value < 1 || config.style.value > 3) // need to rework
	{
		config.style.value = 1;
	}

	out->direction = config.direction.value;

	boxW = config.rect.value[2];
	boxH = config.rect.value[3];
	x = config.rect.value[0];
	y = config.rect.value[1];

	if (!config.alignH.isSet)
	{
		config.alignH.value = SUPERHUD_ALIGNH_LEFT;
	}

	if (!config.alignV.isSet)
	{
		config.alignV.value = SUPERHUD_ALIGNV_TOP;
	}

	switch (config.alignH.value)
	{
		default:
		case SUPERHUD_ALIGNH_LEFT:
			break;
		case SUPERHUD_ALIGNH_CENTER:
			x = config.rect.value[0] - boxW * 0.5f;
			break;
		case SUPERHUD_ALIGNH_RIGHT:
			x = config.rect.value[0] - boxW;
			break;
	}

	switch (config.alignV.value)
	{
		default:
		case SUPERHUD_ALIGNV_TOP:
			break;
		case SUPERHUD_ALIGNV_CENTER:
			y = config.rect.value[1] - boxH * 0.5f;
			break;
		case SUPERHUD_ALIGNV_BOTTOM:
			y = config.rect.value[1] - boxH;
			break;
	}

	out->box[0] = x;
	out->box[1] = y;
	out->box[2] = boxW;
	out->box[3] = boxH;

	if (config.doublebar.isSet)
	{
		static const float bar_gap = 4;
		out->two_bars = qtrue;
		if (out->direction == SUPERHUD_DIR_LEFT_TO_RIGHT || out->direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
		{
			if (config.style.value == 1)    // style 1 - default: split into two bars
			{

				bar_height = config.rect.value[3] / 2 - bar_gap / 2; //split horizontal
				bar_width = config.rect.value[2];

				out->bar[0][0] = x;//x
				out->bar[0][1] = y;//y
				out->bar[0][2] = bar_width;//w
				out->bar[0][3] = bar_height;   // height is half of rect and minus half of the gap between two bars
				//
				out->bar[1][0] = x;//x
				out->bar[1][1] = y + bar_height + bar_gap;
				out->bar[1][2] = bar_width;//w
				out->bar[1][3] = bar_height;//h height is same as in first bar
			}
			else if (config.style.value == 2 || config.style.value == 3) // style 2 - same start point for both bars
			{
				// all the same for same bars
				out->bar[1][0] = out->bar[0][0] = x; // x
				out->bar[1][1] = out->bar[0][1] = y; // y
				out->bar[1][2] = out->bar[0][2] = config.rect.value[2]; // w
				out->bar[1][3] = out->bar[0][3] = config.rect.value[3]; // h
			}
			CG_AdjustFrom640(&out->bar[1][0], &out->bar[1][1], &out->bar[1][2], &out->bar[1][3]);
			CG_AdjustFrom640(&out->bar[0][0], &out->bar[0][1], &out->bar[0][2], &out->bar[0][3]);
			out->max = out->bar[1][2];
			out->koeff = 2 * out->bar[1][2] / max;
		}
		else
		{
			if (config.style.value == 1)    // style 1 - default: split into two bars
			{
				bar_height = config.rect.value[3];
				bar_width = config.rect.value[2] / 2 - bar_gap / 2;

				out->bar[0][0] = x;//x
				out->bar[0][1] = y;//y
				out->bar[0][2] = bar_width;//w
				out->bar[0][3] = bar_height;   //h
				//
				out->bar[1][0] = x + bar_width + bar_gap;
				out->bar[1][1] = y;//y
				out->bar[1][2] = bar_width;//w
				out->bar[1][3] = bar_height;//h
			}
			else if (config.style.value == 2 || config.style.value == 3)   // style 2 - same start point for both bars
			{
				// all the same for same bars
				out->bar[1][0] = out->bar[0][0] = x; // x
				out->bar[1][1] = out->bar[0][1] = y; // y
				out->bar[1][2] = out->bar[0][2] = config.rect.value[2]; // w
				out->bar[1][3] = out->bar[0][3] = config.rect.value[3]; // h
			}
			CG_AdjustFrom640(&out->bar[1][0], &out->bar[1][1], &out->bar[1][2], &out->bar[1][3]);
			CG_AdjustFrom640(&out->bar[0][0], &out->bar[0][1], &out->bar[0][2], &out->bar[0][3]);
			out->max = out->bar[1][3];
			out->koeff = 2 * out->bar[1][3] / max;
		}
	}
	else
	{
		// single bar
		out->bar[0][0] = x;
		out->bar[0][1] = y;
		out->bar[0][2] = config.rect.value[2];
		out->bar[0][3] = config.rect.value[3];
		CG_AdjustFrom640(&out->bar[0][0], &out->bar[0][1], &out->bar[0][2], &out->bar[0][3]);
		if (out->direction == SUPERHUD_DIR_LEFT_TO_RIGHT || out->direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
		{
			out->max = out->bar[0][2]; // max / width
		}
		else
		{
			out->max = out->bar[0][3]; // max / height
		}
		out->koeff = out->max / max;
	}

	CG_SHUDConfigPickColor_Init(&config, out->color_top);
	if (config.bgcolor.isSet)
	{
		Vector4Copy(config.bgcolor.value.rgba, out->color_back);
	}
	else
	{
		Vector4Set(out->color_back, 0, 0, 0, 0);
	}

	// Load images for bar if specified
	out->image = 0;
	out->image2 = 0;
	if (config.image.isSet)
	{
		out->image = trap_R_RegisterShader(config.image.value);
	}
	if (config.image2.isSet)
	{
		out->image2 = trap_R_RegisterShader(config.image2.value);
	}
}

qboolean CG_SHUDIsTimeOut(const superhudConfig_t* cfg, int startTime)
{
	if (!startTime)
	{
		return qtrue;
	}
	if (cfg->time.isSet)
	{
		if ((cg.time - startTime) < cfg->time.value)
		{
			//fade time is not started;
			return qfalse;
		}
	}
	return qtrue;
}

/*
   *  Fade out
   *  returns qfalse if the element has fully faded
   */
qboolean CG_SHUDGetFadeColor(const vec4_t from_color, vec4_t out, const superhudConfig_t* cfg, int startTime)
{
	int time;
	int fadetime;
	float fadedelay;
	float k;
	vec4_t tmpfade;

	Vector4Copy(from_color, out);

	if (!CG_SHUDIsTimeOut(cfg, startTime))
	{
		return qtrue;
	}

	if (!cfg->fade.isSet)
	{
		return qfalse;
	}

	time = 0;
	if (cfg->time.isSet)
	{
		time = cfg->time.value;
	}

	fadedelay = SUPERHUD_DEFAULT_FADEDELAY;
	if (cfg->fadedelay.isSet)
	{
		fadedelay = (float)cfg->fadedelay.value;
	}

	if (fadedelay <= 0.0f)
	{
		return qfalse;
	}

	fadetime = cg.time - startTime - time;

	if (fadetime < 0)
	{
		return qtrue;
	}

	if (fadetime >= (int)fadedelay)
	{
		return qfalse;
	}

	k = (float)fadetime / fadedelay;
	Vector4Copy(cfg->fade.value, tmpfade);
	Vector4Subtract(tmpfade, from_color, tmpfade);
	Vector4MA(from_color, k, tmpfade, out);
	return qtrue;
}

const char* CG_SHUDVaTextWithInt(superhudConfig_t* config, int value)
{
	char* text;

	if (!config->text.isSet || !config->text.value[0])
	{
		return va("%i", value);
	}

	text = config->text.value;

	if (strchr(text, '%'))
	{
		return va(text, value);
	}

	return va("%s%i", text, value);
}

void CG_SHUDTextPrint(const superhudConfig_t* cfg, superhudTextContext_t* ctx)
{
	int flags;

	if (!ctx->text || !ctx->text[0])
	{
		return;
	}

	CG_SHUDConfigPickColor_Runtime(cfg, ctx->color);

	flags = ctx->flags;
	/* Config text "..." only — pointer or same content (decor may bind create-time config*) */
	if (cfg && cfg->text.isSet && cfg->text.value[0] &&
	        (ctx->text == cfg->text.value || !strcmp(ctx->text, cfg->text.value)))
	{
		flags |= DS_CTE_COLOR;
	}

	CG_OSPDrawStringNew(ctx->coord.named.x,
	                    ctx->coord.named.y,
	                    ctx->text,
	                    ctx->color,
	                    ctx->shadowColor, // default shadow color - black
	                    ctx->coord.named.w,
	                    ctx->coord.named.h,
	                    ctx->width,
	                    flags,
	                    ctx->fontIndex,
	                    ctx->background,
	                    ctx->border,
	                    ctx->borderColor);
}

void CG_SHUDTextPrintNew(const superhudConfig_t* cfg, superhudTextContext_t* ctx, qboolean colorOverride)
{
	int flags;

	if (!ctx->text || !ctx->text[0])
	{
		return;
	}
	if (colorOverride)
		CG_SHUDConfigPickColor_Runtime(cfg, ctx->color);

	flags = ctx->flags;
	if (cfg && cfg->text.isSet && cfg->text.value[0] &&
	        (ctx->text == cfg->text.value || !strcmp(ctx->text, cfg->text.value)))
	{
		flags |= DS_CTE_COLOR;
	}

	CG_OSPDrawStringNew(ctx->coord.named.x,
	                    ctx->coord.named.y,
	                    ctx->text,
	                    ctx->color,
	                    ctx->shadowColor, // default shadow color - black
	                    ctx->coord.named.w,
	                    ctx->coord.named.h,
	                    ctx->width,
	                    flags,
	                    ctx->fontIndex,
	                    ctx->background,
	                    ctx->border,
	                    ctx->borderColor);
}

static void CG_SHUDBarPreparePrintLTR(const superhudBarContext_t* ctx, float value, drawBarCoords_t* coords)
{
	if (ctx->two_bars)
	{
		float width;

		Vector4Copy(ctx->bar[0], coords->bar1_value);
		Vector4Copy(ctx->bar[0], coords->bar1_bottom);
		Vector4Copy(ctx->bar[1], coords->bar2_value);
		Vector4Copy(ctx->bar[1], coords->bar2_bottom);
		//bar1
		width = value * ctx->koeff;
		if (width > ctx->max)
		{
			width = ctx->max;
		}
		if (width < 0)
		{
			width = 0;
		}
		coords->bar1_value[2] = width;

		//bar2
		width = value * ctx->koeff;
		width -= ctx->max;

		if (width > ctx->max)
		{
			width = ctx->max;
		}
		if (width < 0)
		{
			width = 0;
		}

		coords->bar2_value[2] = width;
	}
	else
	{
		float width;
		Vector4Copy(ctx->bar[0], coords->bar1_value);
		Vector4Copy(ctx->bar[0], coords->bar1_bottom);
		width = value * ctx->koeff;
		if (width > ctx->max)
		{
			width = ctx->max;
		}
		coords->bar1_value[2] = width;
	}
}

static void CG_SHUDBarPreparePrintRTL(const superhudBarContext_t* ctx, float value, drawBarCoords_t* coords)
{
	//Just make left-to-right and mirror it
	CG_SHUDBarPreparePrintLTR(ctx, value, coords);

	coords->bar1_value[0] = coords->bar1_value[0] + ctx->bar[0][2] - coords->bar1_value[2]; //x = x + max_width - width
	coords->bar2_value[0] = coords->bar2_value[0] + ctx->bar[1][2] - coords->bar2_value[2]; //x = x + max_width - width
}

static void CG_SHUDBarPreparePrintTTB(const superhudBarContext_t* ctx, float value, drawBarCoords_t* coords)
{
	if (ctx->two_bars)
	{
		float height;

		Vector4Copy(ctx->bar[0], coords->bar1_value);
		Vector4Copy(ctx->bar[0], coords->bar1_bottom);
		Vector4Copy(ctx->bar[1], coords->bar2_value);
		Vector4Copy(ctx->bar[1], coords->bar2_bottom);
		//bar1
		height = value * ctx->koeff;
		if (height > ctx->max)
		{
			height = ctx->max;
		}
		if (height < 0)
		{
			height = 0;
		}
		coords->bar1_value[3] = height;

		//bar2
		height = value * ctx->koeff;
		height -= ctx->max;

		if (height > ctx->max)
		{
			height = ctx->max;
		}
		if (height < 0)
		{
			height = 0;
		}

		coords->bar2_value[3] = height;
	}
	else
	{
		float height;
		Vector4Copy(ctx->bar[0], coords->bar1_value);
		Vector4Copy(ctx->bar[0], coords->bar1_bottom);
		height = value * ctx->koeff;
		if (height > ctx->max)
		{
			height = ctx->max;
		}
		if (height < 0)
		{
			height = 0;
		}
		coords->bar1_value[3] = height; // Set height (index 3), not width (index 2)
	}
}

static void CG_SHUDBarPreparePrintBTT(const superhudBarContext_t* ctx, float value, drawBarCoords_t* coords)
{
	//Just make top to bottom and mirror it
	CG_SHUDBarPreparePrintTTB(ctx, value, coords);

	if (ctx->two_bars)
	{
		// For double bar, adjust both bars
		coords->bar1_value[1] += ctx->bar[0][3] - coords->bar1_value[3]; //y = y + max_height - height
		coords->bar2_value[1] += ctx->bar[1][3] - coords->bar2_value[3]; //y = y + max_height - height
	}
	else
	{
		// For single bar
		coords->bar1_value[1] += ctx->bar[0][3] - coords->bar1_value[3]; //y = y + max_height - height
	}
}

/*
 * Texture coordinates for a partial bar image (progress 0.0 - 1.0).
 * UV extent matches draw rect when used with CG_SHUDBarSyncImageDrawRect.
 */
static void CG_SHUDBarGetImageTexCoords(superhudDirection_t direction, float progress, float* s1, float* t1, float* s2, float* t2)
{
	if (progress < 0.0f)
	{
		progress = 0.0f;
	}
	else if (progress > 1.0f)
	{
		progress = 1.0f;
	}

	if (direction == SUPERHUD_DIR_LEFT_TO_RIGHT)
	{
		// Horizontal bar left to right: show from left
		*s1 = 0.0f;
		*t1 = 0.0f;
		*s2 = progress;
		*t2 = 1.0f;
	}
	else if (direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
	{
		// Horizontal bar right to left: show from right
		*s1 = 1.0f - progress;
		*t1 = 0.0f;
		*s2 = 1.0f;
		*t2 = 1.0f;
	}
	else if (direction == SUPERHUD_DIR_TOP_TO_BOTTOM)
	{
		// Vertical bar top to bottom: show from top
		*s1 = 0.0f;
		*t1 = 0.0f;
		*s2 = 1.0f;
		*t2 = progress;
	}
	else // SUPERHUD_DIR_BOTTOM_TO_TOP
	{
		// Vertical bar bottom to top: show from bottom
		*s1 = 0.0f;
		*t1 = 1.0f - progress;
		*s2 = 1.0f;
		*t2 = 1.0f;
	}
}

/*
 * Set draw rect from the same progress used for image UVs (avoids stretch/jitter vs quantized texcoords).
 */
static void CG_SHUDBarSyncImageDrawRect(superhudDirection_t direction, const vec4_t full, float progress, vec4_t draw)
{
	float extent;

	if (progress <= 0.0f)
	{
		draw[0] = full[0];
		draw[1] = full[1];
		draw[2] = 0.0f;
		draw[3] = 0.0f;
		return;
	}
	if (progress > 1.0f)
	{
		progress = 1.0f;
	}

	if (direction == SUPERHUD_DIR_LEFT_TO_RIGHT || direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
	{
		extent = full[2] * progress;
		draw[0] = full[0];
		draw[1] = full[1];
		draw[2] = extent;
		draw[3] = full[3];
		if (direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
		{
			draw[0] = full[0] + full[2] - extent;
		}
	}
	else
	{
		extent = full[3] * progress;
		draw[0] = full[0];
		draw[1] = full[1];
		draw[2] = full[2];
		draw[3] = extent;
		if (direction == SUPERHUD_DIR_BOTTOM_TO_TOP)
		{
			draw[1] = full[1] + full[3] - extent;
		}
	}
}

static void CG_SHUDBarFillAndBorder(const superhudConfig_t* cfg, const superhudBarContext_t* ctx)
{
	superhudConfig_t tmp;

	if (!cfg->rect.isSet)
	{
		return;
	}

	if (!cfg->fill.isSet && !(cfg->border.isSet && cfg->borderColor.isSet))
	{
		return;
	}

	memcpy(&tmp, cfg, sizeof(tmp));
	tmp.rect.value[0] = ctx->box[0];
	tmp.rect.value[1] = ctx->box[1];
	tmp.rect.value[2] = ctx->box[2];
	tmp.rect.value[3] = ctx->box[3];
	tmp.alignH.isSet = qfalse;
	tmp.alignV.isSet = qfalse;

	if (cfg->fill.isSet)
	{
		CG_SHUDFill(&tmp);
	}
	if (cfg->border.isSet && cfg->borderColor.isSet)
	{
		CG_SHUDDrawBorder(&tmp);
	}
}

void CG_SHUDBarPrint(const superhudConfig_t* cfg, superhudBarContext_t* ctx, float value)
{
	drawBarCoords_t coords;
	float progress1;
	float progress2;
	float s1, t1, s2, t2;
	float s1_2, t1_2, s2_2, t2_2;
	qhandle_t shader1;
	qboolean useImage;

	CG_SHUDBarFillAndBorder(cfg, ctx);

	memset(&coords, 0, sizeof(coords));

	switch (ctx->direction)
	{
		default:
		case SUPERHUD_DIR_LEFT_TO_RIGHT:
			CG_SHUDBarPreparePrintLTR(ctx, value, &coords);
			break;
		case SUPERHUD_DIR_RIGHT_TO_LEFT:
			CG_SHUDBarPreparePrintRTL(ctx, value, &coords);
			break;
		case SUPERHUD_DIR_TOP_TO_BOTTOM:
			CG_SHUDBarPreparePrintTTB(ctx, value, &coords);
			break;
		case SUPERHUD_DIR_BOTTOM_TO_TOP:
			CG_SHUDBarPreparePrintBTT(ctx, value, &coords);
			break;
	}

	// Calculate progress for bar images (0.0 to 1.0)
	if (ctx->two_bars)
	{
		// For doublebar, calculate progress for each bar
		if (ctx->direction == SUPERHUD_DIR_LEFT_TO_RIGHT || ctx->direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
		{
			progress1 = coords.bar1_value[2] / ctx->bar[0][2];
			progress2 = coords.bar2_value[2] / ctx->bar[1][2];
		}
		else
		{
			progress1 = coords.bar1_value[3] / ctx->bar[0][3];
			progress2 = coords.bar2_value[3] / ctx->bar[1][3];
		}
	}
	else
	{
		// Single bar
		if (ctx->direction == SUPERHUD_DIR_LEFT_TO_RIGHT || ctx->direction == SUPERHUD_DIR_RIGHT_TO_LEFT)
		{
			progress1 = coords.bar1_value[2] / ctx->bar[0][2];
		}
		else
		{
			progress1 = coords.bar1_value[3] / ctx->bar[0][3];
		}
		progress2 = 0.0f;
	}

	if (ctx->image != 0)
	{
		CG_SHUDBarSyncImageDrawRect(ctx->direction, ctx->bar[0], progress1, coords.bar1_value);
		if (ctx->two_bars)
		{
			CG_SHUDBarSyncImageDrawRect(ctx->direction, ctx->bar[1], progress2, coords.bar2_value);
		}
	}

	// Draw background
	trap_R_SetColor(ctx->color_back);
	trap_R_DrawStretchPic(coords.bar1_bottom[0], coords.bar1_bottom[1], coords.bar1_bottom[2], coords.bar1_bottom[3],
	                      0, 0, 0, 0,
	                      cgs.media.whiteShader);
	if (ctx->two_bars)
	{
		trap_R_DrawStretchPic(coords.bar2_bottom[0], coords.bar2_bottom[1], coords.bar2_bottom[2], coords.bar2_bottom[3],
		                      0, 0, 0, 0,
		                      cgs.media.whiteShader);
	}

	// Draw bar value with image if specified
	useImage = (ctx->image != 0);
	shader1 = useImage ? ctx->image : cgs.media.whiteShader;

	if (useImage)
	{
		CG_SHUDBarGetImageTexCoords(ctx->direction, progress1, &s1, &t1, &s2, &t2);
	}
	else
	{
		s1 = 0.0f;
		t1 = 0.0f;
		s2 = 0.0f;
		t2 = 0.0f;
	}

	trap_R_SetColor(ctx->color_top);
	trap_R_DrawStretchPic(coords.bar1_value[0], coords.bar1_value[1], coords.bar1_value[2], coords.bar1_value[3],
	                      s1, t1, s2, t2,
	                      shader1);

	// Draw second layer if image2 is specified
	if (ctx->image2 != 0)
	{
		CG_SHUDBarGetImageTexCoords(ctx->direction, progress1, &s1_2, &t1_2, &s2_2, &t2_2);
		trap_R_DrawStretchPic(coords.bar1_value[0], coords.bar1_value[1], coords.bar1_value[2], coords.bar1_value[3],
		                      s1_2, t1_2, s2_2, t2_2,
		                      ctx->image2);
	}

	if (ctx->two_bars)
	{
		if (cfg->style.value == 2 || cfg->style.value == 3)
		{
			trap_R_SetColor(ctx->color2_top); // 2nd bar color
		}

		if (useImage)
		{
			CG_SHUDBarGetImageTexCoords(ctx->direction, progress2, &s1, &t1, &s2, &t2);
		}
		else
		{
			s1 = 0.0f;
			t1 = 0.0f;
			s2 = 0.0f;
			t2 = 0.0f;
		}

		trap_R_DrawStretchPic(coords.bar2_value[0], coords.bar2_value[1], coords.bar2_value[2], coords.bar2_value[3],
		                      s1, t1, s2, t2,
		                      shader1);

		// Draw second layer for second bar if image2 is specified
		if (ctx->image2 != 0)
		{
			CG_SHUDBarGetImageTexCoords(ctx->direction, progress2, &s1_2, &t1_2, &s2_2, &t2_2);
			trap_R_DrawStretchPic(coords.bar2_value[0], coords.bar2_value[1], coords.bar2_value[2], coords.bar2_value[3],
			                      s1_2, t1_2, s2_2, t2_2,
			                      ctx->image2);
		}
	}
	trap_R_SetColor(NULL);
}

team_t CG_SHUDGetOurActiveTeam(void)
{
	team_t our_team;

	if (CG_OSPIsGameTypeFreeze())
		our_team = cgs.clientinfo[cg.clientNum].rt;
	else
		our_team = cgs.clientinfo[cg.clientNum].team;

	/* spectPOV 0: no chase team POV */
	if (our_team == TEAM_SPECTATOR && !cg_spectPOV.integer)
		return TEAM_SPECTATOR;

	/* beMyTeam = viewed player (UpdateMyTeam); covers play + spectPOV chase */
	if (cgs.beMyTeam == TEAM_RED || cgs.beMyTeam == TEAM_BLUE)
		return cgs.beMyTeam;

	if (our_team == TEAM_SPECTATOR)
	{
		if (CG_OSPIsGameTypeFreeze())
			our_team = cgs.clientinfo[cg.snap->ps.clientNum].rt;
		else
			our_team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}
	return our_team;
}

void CG_SHUDFillWithColor(const superhudCoord_t* coord, const float* color)
{
	float x, y, w, h;

	x = coord->named.x;
	y = coord->named.y;
	w = coord->named.w;
	h = coord->named.h;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_SetColor(color);
	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}

float CG_SHUDFadeAlphaScale(const vec4_t color_origin, const vec4_t color)
{
	if (color_origin[3] > 0.0f)
	{
		return color[3] / color_origin[3];
	}
	return color[3];
}

qboolean CG_SHUDFillFade(const superhudConfig_t* cfg, float fadeAlpha)
{
	float x, y, w, h;
	vec4_t bgColor;

	if (!cfg->fill.isSet || !cfg->rect.isSet)
	{
		return qfalse;
	}

	if (fadeAlpha <= 0.0f)
	{
		return qfalse;
	}

	CG_SHUDGetImageAlignedRect(cfg, &x, &y, &w, &h);

	CG_AdjustFrom640(&x, &y, &w, &h);

	if (cfg->bgcolor.isSet)
	{
		CG_SHUDConfigPickBgColor(cfg, bgColor, qfalse);
		if (fadeAlpha != 1.0f)
		{
			bgColor[3] *= fadeAlpha;
		}

		trap_R_SetColor(bgColor);
		trap_R_DrawStretchPic(x, y, w, h, 0, 0, 0, 0, cgs.media.whiteShader);
		trap_R_SetColor(NULL);
		return qtrue;
	}

	if (cfg->image.isSet)
	{
		vec4_t color;
		qhandle_t image = trap_R_RegisterShader(cfg->image.value);
		if (!image)
		{
			return qfalse;
		}

		CG_SHUDConfigPickColor_Init(cfg, color);
		if (fadeAlpha != 1.0f)
		{
			color[3] *= fadeAlpha;
		}

		trap_R_SetColor(color);
		trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, image);
		trap_R_SetColor(NULL);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_SHUDFill(const superhudConfig_t* cfg)
{
	return CG_SHUDFillFade(cfg, 1.0f);
}



void CG_SHUDElementCompileTeamOverlayConfig(int fontWidth, int style, shudTeamOverlay_t* configOut)
{
	if (style == 1)
	{
		/* style 1: name + HP/AP only, narrower row; 1 char slot left of name for dead status (red noammoShader) or powerup */
		configOut->powerupOffsetChar = 0;
		configOut->powerupOffsetPix = 0.0f;
		configOut->powerupLenChar = 1;
		configOut->powerupLenPix = configOut->powerupLenChar * fontWidth;

		configOut->nameOffsetChar = 1;
		configOut->nameOffsetPix = configOut->nameOffsetChar * fontWidth;
		configOut->nameLenChar = 10;
		configOut->nameLenPix = configOut->nameLenChar * fontWidth;

		configOut->healthAndArmorOffsetChar = 12;
		configOut->healthAndArmorOffsetPix = configOut->healthAndArmorOffsetChar * fontWidth;
		configOut->healthAndArmorLenChar = 7;
		configOut->healthAndArmorLenPix = configOut->healthAndArmorLenChar * fontWidth;

		configOut->weaponOffsetChar = 0;
		configOut->weaponOffsetPix = 0.0f;
		configOut->weaponLenChar = 0;
		configOut->weaponLenPix = 0.0f;

		configOut->locationOffsetChar = 0;
		configOut->locationOffsetPix = 0.0f;
		configOut->locationLenChar = 0;
		configOut->locationLenPix = 0.0f;

		configOut->overlayWidthChar = 19;
		configOut->overlayWidthPix = configOut->overlayWidthChar * fontWidth;
		return;
	}

	/* style 0 (default): full overlay */
	configOut->powerupOffsetChar = 0;
	configOut->powerupOffsetPix = configOut->powerupOffsetChar * fontWidth;
	configOut->powerupLenChar = 1;
	configOut->powerupLenPix = configOut->powerupLenChar * fontWidth;

	configOut->nameOffsetChar = 1;
	configOut->nameOffsetPix = configOut->nameOffsetChar * fontWidth;
	configOut->nameLenChar = 12;
	configOut->nameLenPix = configOut->nameLenChar * fontWidth;

	configOut->healthAndArmorOffsetChar = 14;
	configOut->healthAndArmorOffsetPix = configOut->healthAndArmorOffsetChar * fontWidth;
	configOut->healthAndArmorLenChar = 7; // 200/200
	configOut->healthAndArmorLenPix = configOut->healthAndArmorLenChar * fontWidth;

	configOut->weaponOffsetChar = 21;
	configOut->weaponOffsetPix = configOut->weaponOffsetChar * fontWidth;
	configOut->weaponLenChar = 1;
	configOut->weaponLenPix = configOut->weaponLenChar * fontWidth;

	configOut->locationOffsetChar = 23;
	configOut->locationOffsetPix = configOut->locationOffsetChar * fontWidth;
	configOut->locationLenChar = cg_MaxlocationWidth.integer;
	configOut->locationLenPix = configOut->locationLenChar * fontWidth;

	configOut->overlayWidthChar = configOut->locationOffsetChar + cg_MaxlocationWidth.integer;
	configOut->overlayWidthPix = configOut->overlayWidthChar * fontWidth;
}

//
// trap_R_DrawStretchPic Wrapper
//
// float x       X coord of result image
// float y       Y coord of result image
// float w       Width of result image
// float h       Height of result image
// float s1      X coord in the shader (0.0f...1.0f)
// float t1      Y coord in the shader (0.0f...1.0f)
// float s2      Width of image in the shader (0.0f...1.0f)
// float t2      Height of image in the shader (0.0f...1.0f)
// float *color  Use this color
// qhandle_t shader Shader
//
void CG_SHUDDrawStretchPic(superhudCoord_t coord, const superhudCoord_t coordPicture, const float* color, qhandle_t shader)
{
	if (!shader) return;

	trap_R_SetColor(color);
	CG_AdjustFrom640(&coord.named.x, &coord.named.y, &coord.named.w, &coord.named.h);
	trap_R_DrawStretchPic(coord.named.x,
	                      coord.named.y,
	                      coord.named.w,
	                      coord.named.h,
	                      coordPicture.named.x,
	                      coordPicture.named.y,
	                      coordPicture.named.w,
	                      coordPicture.named.h,
	                      shader);
	trap_R_SetColor(NULL);
}

void CG_SHUDDrawBorderDirect(const superhudCoord_t* coord, const vec4_t border, const vec4_t borderColor)
{
	vec4_t tmpCoord;
	if (coord == 0)
	{
		return;
	}

	Vector4Copy(coord->arr, tmpCoord);

	CG_AdjustFrom640(&tmpCoord[0], &tmpCoord[1], &tmpCoord[2], &tmpCoord[3]);

	CG_OSPDrawFrame(tmpCoord[0], tmpCoord[1], tmpCoord[2], tmpCoord[3],
	                (float*)border, (float*)borderColor, qtrue);

}

qboolean CG_SHUDDrawBorderFade(const superhudConfig_t* cfg, float fadeAlpha)
{
	vec4_t coord;
	vec4_t borderColor;

	if (!cfg->border.isSet || !cfg->rect.isSet || !cfg->borderColor.isSet)
	{
		return qfalse;
	}

	if (fadeAlpha <= 0.0f)
	{
		return qfalse;
	}

	CG_SHUDGetImageAlignedRect(cfg, &coord[0], &coord[1], &coord[2], &coord[3]);
	CG_AdjustFrom640(&coord[0], &coord[1], &coord[2], &coord[3]);

	CG_SHUDConfigPickBorderColor(cfg, borderColor, qfalse);
	if (fadeAlpha != 1.0f)
	{
		borderColor[3] *= fadeAlpha;
	}

	CG_OSPDrawFrame(coord[0], coord[1], coord[2], coord[3],
	                (float*)cfg->border.value, borderColor, qfalse);

	return qtrue;
}

qboolean CG_SHUDDrawBorder(const superhudConfig_t* cfg)
{
	return CG_SHUDDrawBorderFade(cfg, 1.0f);
}

void CG_SHUDFillAndFrameForElement(const superhudConfig_t* cfg, const vec4_t color_origin, const vec4_t color)
{
	float fadeAlpha;

	fadeAlpha = CG_SHUDFadeAlphaScale(color_origin, color);
	if (fadeAlpha <= 0.0f)
	{
		return;
	}

	CG_SHUDFillFade(cfg, fadeAlpha);
	CG_SHUDDrawBorderFade(cfg, fadeAlpha);
}

void CG_SHUDFillAndFrameForText(superhudConfig_t* cfg, superhudTextContext_t* ctx)
{
	qboolean drawBackground = (cfg->bgcolor.isSet && cfg->fill.isSet);
	qboolean drawBorder = (cfg->border.isSet && cfg->borderColor.isSet);

	if (!drawBackground && !drawBorder)
	{
		return;
	}

	if (drawBackground)
	{
		CG_SHUDConfigPickBgColor(cfg, ctx->background, qfalse);
	}

	if (drawBorder)
	{
		Vector4Copy(cfg->border.value, ctx->border);
		CG_SHUDConfigPickBorderColor(cfg, ctx->borderColor, qfalse);
	}
}

void CG_SHUDDrawStretchPicCtx(const superhudConfig_t* cfg, superhudDrawContext_t* ctx)
{
	// we have to pick color again, because team could changed
	CG_SHUDConfigPickColor_Runtime(cfg, ctx->color);
	CG_SHUDDrawStretchPic(ctx->coord, ctx->coordPicture, ctx->color, ctx->image);
}

int CG_SHUDGetAmmo(int wpi)
{
	int ammo = cg.snap->ps.ammo[wpi];

	if (ammo < 0) ammo = 0;
	if (ammo > 999) ammo = 999;
	return ammo;
}

/*
 * Parse weaponMaxValue string and apply values to maxammo array
 * Format: "weaponNum value weaponNum value ..."
 * Example: "1 50 2 100" sets weapon 1 to 50, weapon 2 to 100
 */
void CG_SHUDParseWeaponMaxValue(const char* valueStr, int* maxammo, int arraySize)
{
	char parseStr[256];
	char* parsePtr;
	char* nextPtr;
	int weaponNum;
	int maxValue;
	int pairCount;

	if (!valueStr || !maxammo || arraySize <= 0)
	{
		return;
	}

	Q_strncpyz(parseStr, valueStr, sizeof(parseStr));
	parsePtr = parseStr;

	/* Parse pairs: weapon number and max value */
	pairCount = 0;
	while (*parsePtr != 0 && pairCount < arraySize)
	{
		/* Skip whitespace */
		while (*parsePtr == ' ' || *parsePtr == '\t')
		{
			parsePtr++;
		}
		if (*parsePtr == 0) break;

		/* Parse weapon number and value */
		if (Q_sscanf(parsePtr, "%i %i", &weaponNum, &maxValue) == 2)
		{
			/* Validate weapon number and ignore gauntlet (weapon 1) - it has no ammo */
			if (weaponNum >= 0 && weaponNum < arraySize && weaponNum != 1)
			{
				maxammo[weaponNum] = maxValue;
			}

			/* Move pointer past first number */
			nextPtr = parsePtr;
			while (*nextPtr != 0 && *nextPtr != ' ' && *nextPtr != '\t')
			{
				nextPtr++;
			}
			/* Skip whitespace */
			while (*nextPtr == ' ' || *nextPtr == '\t')
			{
				nextPtr++;
			}
			/* Move pointer past second number */
			while (*nextPtr != 0 && *nextPtr != ' ' && *nextPtr != '\t')
			{
				nextPtr++;
			}
			parsePtr = nextPtr;
			pairCount++;
		}
		else
		{
			/* Invalid format, skip to next space or end */
			while (*parsePtr != 0 && *parsePtr != ' ' && *parsePtr != '\t')
			{
				parsePtr++;
			}
		}
	}
}
