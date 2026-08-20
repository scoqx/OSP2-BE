#include "cg_local.h"
#include "../qcommon/qcommon.h"
#include "superhud_private.h"
#include "superhud.h"

#define SHUD_SAVE_BUF 262144

static void CG_SHUDBufCat(char* buf, int size, const char* s)
{
	if (!buf || !s || size <= 0)
	{
		return;
	}
	if ((int)(strlen(buf) + strlen(s)) >= size - 1)
	{
		return;
	}
	Q_strcat(buf, size, s);
}

static void CG_SHUDBufCatFloat(char* buf, int size, float v)
{
	char tmp[32];
	int iv;

	iv = (int)v;
	if (v >= 0.0f && (v - (float)iv) < 0.001f)
	{
		Com_sprintf(tmp, sizeof(tmp), "%d", iv);
	}
	else if (v < 0.0f && ((float)iv - v) < 0.001f)
	{
		Com_sprintf(tmp, sizeof(tmp), "%d", iv);
	}
	else
	{
		Com_sprintf(tmp, sizeof(tmp), "%.3f", v);
	}
	CG_SHUDBufCat(buf, size, tmp);
}

static void CG_SHUDBufCatVec2(char* buf, int size, const vec2_t v)
{
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[0]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[1]);
}

static void CG_SHUDBufCatVec3(char* buf, int size, const vec3_t v)
{
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[0]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[1]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[2]);
}

static void CG_SHUDBufCatVec4(char* buf, int size, const vec4_t v)
{
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[0]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[1]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[2]);
	CG_SHUDBufCat(buf, size, " ");
	CG_SHUDBufCatFloat(buf, size, v[3]);
}

static void CG_SHUDBufCatColor(char* buf, int size, const superhudColor_t* color)
{
	switch (color->type)
	{
		case SUPERHUD_COLOR_T:
			CG_SHUDBufCat(buf, size, " T");
			break;
		case SUPERHUD_COLOR_E:
			CG_SHUDBufCat(buf, size, " E");
			break;
		case SUPERHUD_COLOR_I:
			CG_SHUDBufCat(buf, size, " I");
			break;
		default:
			CG_SHUDBufCatVec4(buf, size, color->rgba);
			break;
	}
}

static void CG_SHUDBufCatQuoted(char* buf, int size, const char* s)
{
	CG_SHUDBufCat(buf, size, " \"");
	CG_SHUDBufCat(buf, size, s);
	CG_SHUDBufCat(buf, size, "\"");
}

static void CG_SHUDBufCatAlignH(char* buf, int size, superhudAlignH_t v)
{
	if (v == SUPERHUD_ALIGNH_CENTER)
	{
		CG_SHUDBufCat(buf, size, " C");
	}
	else if (v == SUPERHUD_ALIGNH_RIGHT)
	{
		CG_SHUDBufCat(buf, size, " R");
	}
	else
	{
		CG_SHUDBufCat(buf, size, " L");
	}
}

static void CG_SHUDBufCatAlignV(char* buf, int size, superhudAlignV_t v)
{
	if (v == SUPERHUD_ALIGNV_CENTER)
	{
		CG_SHUDBufCat(buf, size, " C");
	}
	else if (v == SUPERHUD_ALIGNV_BOTTOM)
	{
		CG_SHUDBufCat(buf, size, " B");
	}
	else
	{
		CG_SHUDBufCat(buf, size, " T");
	}
}

static void CG_SHUDBufCatDirection(char* buf, int size, superhudDirection_t v)
{
	if (v == SUPERHUD_DIR_RIGHT_TO_LEFT)
	{
		CG_SHUDBufCat(buf, size, " R");
	}
	else if (v == SUPERHUD_DIR_TOP_TO_BOTTOM)
	{
		CG_SHUDBufCat(buf, size, " T");
	}
	else if (v == SUPERHUD_DIR_BOTTOM_TO_TOP)
	{
		CG_SHUDBufCat(buf, size, " B");
	}
	else
	{
		CG_SHUDBufCat(buf, size, " L");
	}
}

static void CG_SHUDBufCatItTeam(char* buf, int size, superhudItTeam_t v)
{
	if (v == SUPERHUD_ITTEAM_RED)
	{
		CG_SHUDBufCat(buf, size, " R");
	}
	else if (v == SUPERHUD_ITTEAM_NEUTRAL)
	{
		CG_SHUDBufCat(buf, size, " N");
	}
	else if (v == SUPERHUD_ITTEAM_OWN)
	{
		CG_SHUDBufCat(buf, size, " O");
	}
	else if (v == SUPERHUD_ITTEAM_ENEMY)
	{
		CG_SHUDBufCat(buf, size, " E");
	}
	else
	{
		CG_SHUDBufCat(buf, size, " B");
	}
}

static void CG_SHUDBufCatVisflagBits(char* buf, int size, int flags, int flagsNeg, const int* bits, const char** names, int n)
{
	int i;

	for (i = 0; i < n; i++)
	{
		if (flags & bits[i])
		{
			CG_SHUDBufCat(buf, size, " ");
			CG_SHUDBufCat(buf, size, names[i]);
		}
		if (flagsNeg & bits[i])
		{
			CG_SHUDBufCat(buf, size, " !");
			CG_SHUDBufCat(buf, size, names[i]);
		}
	}
}

static void CG_SHUDBufCatVisflags(char* buf, int size, const superhudConfig_t* cfg)
{
	static const int bits[] =
	{
		SE_IM, SE_TEAM_ONLY, SE_SPECT, SE_DEAD, SE_DEMO_HIDE, SE_SCORES_HIDE,
		SE_KEY1_SHOW, SE_KEY2_SHOW, SE_KEY3_SHOW, SE_KEY4_SHOW,
		SE_KEY5_SHOW, SE_KEY6_SHOW, SE_KEY7_SHOW, SE_KEY8_SHOW,
		SE_SHOW_EMPTY,
		SE_GT_FFA, SE_GT_TOURNEY, SE_GT_TDM, SE_GT_CTF, SE_GT_FREEZETAG, SE_GT_CLANARENA,
		SE_WEAPON_GAUNTLET, SE_WEAPON_MACHINEGUN, SE_WEAPON_SHOTGUN, SE_WEAPON_GRENADE_LAUNCHER,
		SE_WEAPON_ROCKET_LAUNCHER, SE_WEAPON_LIGHTNING, SE_WEAPON_RAILGUN, SE_WEAPON_PLASMAGUN, SE_WEAPON_BFG,
		SE_HEALTH_LOW, SE_WARMUP
	};
	static const char* names[] =
	{
		SE_IM_STR, SE_TEAM_ONLY_STR, SE_SPECT_STR, SE_DEAD_STR, SE_DEMO_HIDE_STR, SE_SCORES_HIDE_STR,
		SE_KEY1_SHOW_STR, SE_KEY2_SHOW_STR, SE_KEY3_SHOW_STR, SE_KEY4_SHOW_STR,
		SE_KEY5_SHOW_STR, SE_KEY6_SHOW_STR, SE_KEY7_SHOW_STR, SE_KEY8_SHOW_STR,
		SE_SHOW_EMPTY_STR,
		SE_GT_FFA_STR, SE_GT_TOURNEY_STR, SE_GT_TDM_STR, SE_GT_CTF_STR, SE_GT_FREEZETAG_STR, SE_GT_CLANARENA_STR,
		SE_WEAPON_GAUNTLET_STR, SE_WEAPON_MACHINEGUN_STR, SE_WEAPON_SHOTGUN_STR, SE_WEAPON_GRENADE_LAUNCHER_STR,
		SE_WEAPON_ROCKET_LAUNCHER_STR, SE_WEAPON_LIGHTNING_STR, SE_WEAPON_RAILGUN_STR, SE_WEAPON_PLASMAGUN_STR, SE_WEAPON_BFG_STR,
		SE_HEALTH_LOW_STR, SE_WARMUP_STR
	};
	static const int extBits[] =
	{
		SE_ENTERCHAT, SE_SHOWCHAT, SE_PRINTUI, SE_BESTATS
	};
	static const char* extNames[] =
	{
		SE_ENTERCHAT_STR, SE_SHOWCHAT_STR, SE_PRINTUI_STR, SE_BESTATS_STR
	};
	static const int styleBits[] =
	{
		SE_SHUD_STYLE1, SE_SHUD_STYLE2, SE_SHUD_STYLE3, SE_SHUD_STYLE4
	};
	static const char* styleNames[] =
	{
		SE_SHUD_STYLE1_STR, SE_SHUD_STYLE2_STR, SE_SHUD_STYLE3_STR, SE_SHUD_STYLE4_STR
	};

	CG_SHUDBufCatVisflagBits(buf, size, cfg->visflags.value, cfg->visflags.valueNeg,
	                         bits, names, (int)(sizeof(bits) / sizeof(bits[0])));
	CG_SHUDBufCatVisflagBits(buf, size, cfg->visflags.valueExt, cfg->visflags.valueNegExt,
	                         extBits, extNames, (int)(sizeof(extBits) / sizeof(extBits[0])));
	CG_SHUDBufCatVisflagBits(buf, size, cfg->visiblity.value, cfg->visiblity.valueNeg,
	                         styleBits, styleNames, (int)(sizeof(styleBits) / sizeof(styleBits[0])));
}

static qboolean CG_SHUDSameVisflags(const superhudConfig_t* a, const superhudConfig_t* b)
{
	if (a->visflags.value != b->visflags.value)
	{
		return qfalse;
	}
	if (a->visflags.valueNeg != b->visflags.valueNeg)
	{
		return qfalse;
	}
	if (a->visflags.valueExt != b->visflags.valueExt)
	{
		return qfalse;
	}
	if (a->visflags.valueNegExt != b->visflags.valueNegExt)
	{
		return qfalse;
	}
	if ((a->visiblity.value & SE_SHUD_STYLE_ANY) != (b->visiblity.value & SE_SHUD_STYLE_ANY))
	{
		return qfalse;
	}
	if ((a->visiblity.valueNeg & SE_SHUD_STYLE_ANY) != (b->visiblity.valueNeg & SE_SHUD_STYLE_ANY))
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_SHUDVisflagsWantWrite(const superhudConfig_t* cfg, const superhudConfig_t* inh)
{
	qboolean elSet;
	qboolean inhSet;

	elSet = cfg->visflags.isSet;
	if (!elSet && ((cfg->visiblity.value | cfg->visiblity.valueNeg) & SE_SHUD_STYLE_ANY) != 0)
	{
		elSet = qtrue;
	}
	if (!elSet)
	{
		return qfalse;
	}
	if (!inh)
	{
		return qtrue;
	}
	inhSet = inh->visflags.isSet;
	if (!inhSet && ((inh->visiblity.value | inh->visiblity.valueNeg) & SE_SHUD_STYLE_ANY) != 0)
	{
		inhSet = qtrue;
	}
	if (!inhSet)
	{
		return qtrue;
	}
	return CG_SHUDSameVisflags(cfg, inh) ? qfalse : qtrue;
}

static qboolean CG_SHUDSameF(float a, float b)
{
	float d;

	d = a - b;
	if (d < 0.0f)
	{
		d = -d;
	}
	return d < 0.001f ? qtrue : qfalse;
}

static qboolean CG_SHUDSameVec2(const vec2_t a, const vec2_t b)
{
	return (CG_SHUDSameF(a[0], b[0]) && CG_SHUDSameF(a[1], b[1])) ? qtrue : qfalse;
}

static qboolean CG_SHUDSameVec3(const vec3_t a, const vec3_t b)
{
	return (CG_SHUDSameF(a[0], b[0]) && CG_SHUDSameF(a[1], b[1]) && CG_SHUDSameF(a[2], b[2])) ? qtrue : qfalse;
}

static qboolean CG_SHUDSameVec4(const vec4_t a, const vec4_t b)
{
	return (CG_SHUDSameF(a[0], b[0]) && CG_SHUDSameF(a[1], b[1]) && CG_SHUDSameF(a[2], b[2]) && CG_SHUDSameF(a[3], b[3])) ? qtrue : qfalse;
}

static qboolean CG_SHUDSameColor(const superhudColor_t* a, const superhudColor_t* b)
{
	if (a->type != b->type)
	{
		return qfalse;
	}
	if (a->type != SUPERHUD_COLOR_RGBA)
	{
		return qtrue;
	}
	return CG_SHUDSameVec4(a->rgba, b->rgba);
}

static qboolean CG_SHUDWriteField(qboolean elSet, const superhudConfig_t* inherit, qboolean inhSet, qboolean same)
{
	if (!elSet)
	{
		return qfalse;
	}
	if (!inherit || !inhSet)
	{
		return qtrue;
	}
	return same ? qfalse : qtrue;
}

static void CG_SHUDAppendConfig(char* buf, int size, const superhudConfig_t* cfg, const superhudConfig_t* inh)
{
	if (CG_SHUDWriteField(cfg->rect.isSet, inh, inh ? inh->rect.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->rect.value, inh->rect.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\trect");
		CG_SHUDBufCatVec4(buf, size, cfg->rect.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->color.isSet, inh, inh ? inh->color.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->color.value, &inh->color.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tcolor");
		CG_SHUDBufCatColor(buf, size, &cfg->color.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->color2.isSet, inh, inh ? inh->color2.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->color2.value, &inh->color2.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tcolor2");
		CG_SHUDBufCatColor(buf, size, &cfg->color2.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->bgcolor.isSet, inh, inh ? inh->bgcolor.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->bgcolor.value, &inh->bgcolor.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tbgcolor");
		CG_SHUDBufCatColor(buf, size, &cfg->bgcolor.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->bgcolor2.isSet, inh, inh ? inh->bgcolor2.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->bgcolor2.value, &inh->bgcolor2.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tbgcolor2");
		CG_SHUDBufCatColor(buf, size, &cfg->bgcolor2.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->border.isSet, inh, inh ? inh->border.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->border.value, inh->border.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tborder");
		CG_SHUDBufCatVec4(buf, size, cfg->border.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->borderColor.isSet, inh, inh ? inh->borderColor.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->borderColor.value, &inh->borderColor.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tborderColor");
		CG_SHUDBufCatColor(buf, size, &cfg->borderColor.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->borderColor2.isSet, inh, inh ? inh->borderColor2.isSet : qfalse, inh ? CG_SHUDSameColor(&cfg->borderColor2.value, &inh->borderColor2.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tborderColor2");
		CG_SHUDBufCatColor(buf, size, &cfg->borderColor2.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->shadowColor.isSet, inh, inh ? inh->shadowColor.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->shadowColor.value, inh->shadowColor.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tshadowColor");
		CG_SHUDBufCatVec4(buf, size, cfg->shadowColor.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->hlcolor.isSet, inh, inh ? inh->hlcolor.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->hlcolor.value, inh->hlcolor.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\thlcolor");
		CG_SHUDBufCatVec4(buf, size, cfg->hlcolor.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->font.isSet, inh, inh ? inh->font.isSet : qfalse, inh ? (Q_stricmp(cfg->font.value, inh->font.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tfont");
		CG_SHUDBufCatQuoted(buf, size, cfg->font.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->fontsize.isSet, inh, inh ? inh->fontsize.isSet : qfalse, inh ? CG_SHUDSameVec2(cfg->fontsize.value, inh->fontsize.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tfontsize");
		CG_SHUDBufCatVec2(buf, size, cfg->fontsize.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->alignH.isSet, inh, inh ? inh->alignH.isSet : qfalse, inh ? (cfg->alignH.value == inh->alignH.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\talignh");
		CG_SHUDBufCatAlignH(buf, size, cfg->alignH.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->alignV.isSet, inh, inh ? inh->alignV.isSet : qfalse, inh ? (cfg->alignV.value == inh->alignV.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\talignv");
		CG_SHUDBufCatAlignV(buf, size, cfg->alignV.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->textAlign.isSet, inh, inh ? inh->textAlign.isSet : qfalse, inh ? (cfg->textAlign.value == inh->textAlign.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\ttextalign");
		CG_SHUDBufCatAlignH(buf, size, cfg->textAlign.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->text.isSet, inh, inh ? inh->text.isSet : qfalse, inh ? (Q_stricmp(cfg->text.value, inh->text.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\ttext");
		CG_SHUDBufCatQuoted(buf, size, cfg->text.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->textOffset.isSet, inh, inh ? inh->textOffset.isSet : qfalse, inh ? CG_SHUDSameVec2(cfg->textOffset.value, inh->textOffset.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\ttextoffset");
		CG_SHUDBufCatVec2(buf, size, cfg->textOffset.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->textStyle.isSet, inh, inh ? inh->textStyle.isSet : qfalse, inh ? (cfg->textStyle.value == inh->textStyle.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\ttextstyle ");
		CG_SHUDBufCatFloat(buf, size, (float)cfg->textStyle.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->style.isSet, inh, inh ? inh->style.isSet : qfalse, inh ? (cfg->style.value == inh->style.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tstyle ");
		CG_SHUDBufCatFloat(buf, size, (float)cfg->style.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->time.isSet, inh, inh ? inh->time.isSet : qfalse, inh ? (cfg->time.value == inh->time.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\ttime ");
		CG_SHUDBufCatFloat(buf, size, (float)cfg->time.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->fadedelay.isSet, inh, inh ? inh->fadedelay.isSet : qfalse, inh ? (cfg->fadedelay.value == inh->fadedelay.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tfadedelay ");
		CG_SHUDBufCatFloat(buf, size, (float)cfg->fadedelay.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->hlsize.isSet, inh, inh ? inh->hlsize.isSet : qfalse, inh ? (cfg->hlsize.value == inh->hlsize.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\thlsize ");
		CG_SHUDBufCatFloat(buf, size, (float)cfg->hlsize.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->image.isSet, inh, inh ? inh->image.isSet : qfalse, inh ? (Q_stricmp(cfg->image.value, inh->image.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\timage");
		CG_SHUDBufCatQuoted(buf, size, cfg->image.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->image2.isSet, inh, inh ? inh->image2.isSet : qfalse, inh ? (Q_stricmp(cfg->image2.value, inh->image2.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\timage2");
		CG_SHUDBufCatQuoted(buf, size, cfg->image2.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->imageAlignH.isSet, inh, inh ? inh->imageAlignH.isSet : qfalse, inh ? (cfg->imageAlignH.value == inh->imageAlignH.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\timagealignh");
		CG_SHUDBufCatAlignH(buf, size, cfg->imageAlignH.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->imageAlignV.isSet, inh, inh ? inh->imageAlignV.isSet : qfalse, inh ? (cfg->imageAlignV.value == inh->imageAlignV.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\timagealignv");
		CG_SHUDBufCatAlignV(buf, size, cfg->imageAlignV.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->imagetc.isSet, inh, inh ? inh->imagetc.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->imagetc.value, inh->imagetc.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\timagetc");
		CG_SHUDBufCatVec4(buf, size, cfg->imagetc.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->model.isSet, inh, inh ? inh->model.isSet : qfalse, inh ? (Q_stricmp(cfg->model.value, inh->model.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tmodel");
		CG_SHUDBufCatQuoted(buf, size, cfg->model.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->direction.isSet, inh, inh ? inh->direction.isSet : qfalse, inh ? (cfg->direction.value == inh->direction.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tdirection");
		CG_SHUDBufCatDirection(buf, size, cfg->direction.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->itTeam.isSet, inh, inh ? inh->itTeam.isSet : qfalse, inh ? (cfg->itTeam.value == inh->itTeam.value ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\titteam");
		CG_SHUDBufCatItTeam(buf, size, cfg->itTeam.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->fade.isSet, inh, inh ? inh->fade.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->fade.value, inh->fade.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tfade");
		CG_SHUDBufCatVec4(buf, size, cfg->fade.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->angles.isSet, inh, inh ? inh->angles.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->angles.value, inh->angles.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tangles");
		CG_SHUDBufCatVec4(buf, size, cfg->angles.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->offset.isSet, inh, inh ? inh->offset.isSet : qfalse, inh ? CG_SHUDSameVec3(cfg->offset.value, inh->offset.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\toffset");
		CG_SHUDBufCatVec3(buf, size, cfg->offset.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->margins.isSet, inh, inh ? inh->margins.isSet : qfalse, inh ? CG_SHUDSameVec4(cfg->margins.value, inh->margins.value) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tmargins");
		CG_SHUDBufCatVec4(buf, size, cfg->margins.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDVisflagsWantWrite(cfg, inh))
	{
		CG_SHUDBufCat(buf, size, "\tvisflags");
		CG_SHUDBufCatVisflags(buf, size, cfg);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->weaponMaxValue.isSet, inh, inh ? inh->weaponMaxValue.isSet : qfalse, inh ? (Q_stricmp(cfg->weaponMaxValue.value, inh->weaponMaxValue.value) == 0 ? qtrue : qfalse) : qfalse))
	{
		CG_SHUDBufCat(buf, size, "\tweaponMaxValue");
		CG_SHUDBufCatQuoted(buf, size, cfg->weaponMaxValue.value);
		CG_SHUDBufCat(buf, size, ";\n");
	}
	if (CG_SHUDWriteField(cfg->fill.isSet, inh, inh ? inh->fill.isSet : qfalse, qtrue))
	{
		CG_SHUDBufCat(buf, size, "\tfill;\n");
	}
	if (CG_SHUDWriteField(cfg->monospace.isSet, inh, inh ? inh->monospace.isSet : qfalse, qtrue))
	{
		CG_SHUDBufCat(buf, size, "\tmonospace;\n");
	}
	if (CG_SHUDWriteField(cfg->doublebar.isSet, inh, inh ? inh->doublebar.isSet : qfalse, qtrue))
	{
		CG_SHUDBufCat(buf, size, "\tdoublebar;\n");
	}
}

static qboolean CG_SHUDSanitizeFileName(const char* in, char* out, int outSize)
{
	int i;
	int o;
	char c;

	if (!in || !out || outSize < 2)
	{
		return qfalse;
	}
	o = 0;
	for (i = 0; in[i] && o < outSize - 1; i++)
	{
		c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
		{
			out[o++] = c;
		}
	}
	out[o] = 0;
	return o > 0;
}

qboolean CG_SHUDSaveConfig(const char* filename)
{
	char name[MAX_QPATH];
	char path[MAX_QPATH];
	char bak[MAX_QPATH];
	char* buf;
	char* old;
	fileHandle_t fh;
	int rc;
	int len;
	superhudElement_t* el;
	superhudElement_t* walk;
	superhudElement_t* defEl;
	const char* src;
	const superhudConfig_t* inherit;
	int tab;
	int grp;
	int n;
	int ngroups;
	int ngrp;

	src = filename && filename[0] ? filename : ch_file.string;
	if (!src || !src[0])
	{
		src = "default";
	}
	if (!CG_SHUDSanitizeFileName(src, name, sizeof(name)))
	{
		CG_Printf("^1SuperHUD: bad file name for save.\n");
		return qfalse;
	}

	el = CG_SHUDGetElementsHead();
	if (!el)
	{
		CG_Printf("^1SuperHUD: nothing to save.\n");
		return qfalse;
	}

	buf = Z_Malloc(SHUD_SAVE_BUF);
	if (!buf)
	{
		CG_Printf("^1SuperHUD: not enough memory to save.\n");
		return qfalse;
	}
	buf[0] = 0;
	CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "# OSP2-BE SuperHUD - written by shudedit\n");
	CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "# !default applies to following elements until the next !default\n\n");

	for (tab = 0; tab < SHUD_TAB_COUNT; tab++)
	{
		defEl = NULL;
		n = 0;
		walk = el;
		while (walk)
		{
			if (walk->tab == tab && walk->element.name)
			{
				if (CG_SHUDIsDefaultName(walk->element.name))
				{
					if (!defEl)
					{
						defEl = walk;
					}
				}
				else
				{
					n++;
				}
			}
			walk = walk->next;
		}
		if (n <= 0)
		{
			continue;
		}

		CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "# ");
		CG_SHUDBufCat(buf, SHUD_SAVE_BUF, CG_SHUDTabTitle(tab));
		CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "\n");
		CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "!default {\n");
		if (defEl)
		{
			CG_SHUDAppendConfig(buf, SHUD_SAVE_BUF, &defEl->config, NULL);
		}
		CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "}\n\n");

		ngroups = 0;
		for (grp = 0; grp < SHUD_GRP_COUNT; grp++)
		{
			walk = el;
			while (walk)
			{
				if (walk->tab == tab && walk->grp == grp && walk->element.name &&
				    !CG_SHUDIsDefaultName(walk->element.name))
				{
					ngroups++;
					break;
				}
				walk = walk->next;
			}
		}

		for (grp = 0; grp < SHUD_GRP_COUNT; grp++)
		{
			ngrp = 0;
			walk = el;
			while (walk)
			{
				if (walk->tab == tab && walk->grp == grp && walk->element.name &&
				    !CG_SHUDIsDefaultName(walk->element.name))
				{
					ngrp++;
				}
				walk = walk->next;
			}
			if (ngrp <= 0)
			{
				continue;
			}
			if (ngroups > 1)
			{
				CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "## ");
				CG_SHUDBufCat(buf, SHUD_SAVE_BUF, CG_SHUDGroupTitle(grp));
				CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "\n");
			}
			walk = el;
			while (walk)
			{
				if (walk->tab == tab && walk->grp == grp && walk->element.name &&
				    !CG_SHUDIsDefaultName(walk->element.name))
				{
					inherit = defEl ? &defEl->config : NULL;
					CG_SHUDBufCat(buf, SHUD_SAVE_BUF, walk->element.name);
					CG_SHUDBufCat(buf, SHUD_SAVE_BUF, " {\n");
					CG_SHUDAppendConfig(buf, SHUD_SAVE_BUF, &walk->config, inherit);
					CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "}\n");
				}
				walk = walk->next;
			}
			CG_SHUDBufCat(buf, SHUD_SAVE_BUF, "\n");
		}
	}

	Com_sprintf(path, sizeof(path), "hud/%s.cfg", name);
	rc = trap_FS_FOpenFile(path, &fh, FS_READ);
	if (rc > 0 && fh)
	{
		old = Z_Malloc(rc + 1);
		if (old)
		{
			trap_FS_Read(old, rc, fh);
			trap_FS_FCloseFile(fh);
			Com_sprintf(bak, sizeof(bak), "hud/%s.cfg.bak", name);
			if (trap_FS_FOpenFile(bak, &fh, FS_WRITE) >= 0 && fh)
			{
				trap_FS_Write(old, rc, fh);
				trap_FS_FCloseFile(fh);
			}
			Z_Free(old);
		}
		else
		{
			trap_FS_FCloseFile(fh);
		}
	}

	rc = trap_FS_FOpenFile(path, &fh, FS_WRITE);
	if (rc < 0 || !fh)
	{
		Z_Free(buf);
		CG_Printf("^1SuperHUD: could not write %s\n", path);
		return qfalse;
	}
	len = (int)strlen(buf);
	trap_FS_Write(buf, len, fh);
	trap_FS_FCloseFile(fh);
	if (len >= SHUD_SAVE_BUF - 8)
	{
		CG_Printf("^3SuperHUD: save buffer nearly full, file may be truncated\n");
	}
	Z_Free(buf);
	CG_Printf("^2SuperHUD: saved %s (%d bytes)\n", path, len);
	return qtrue;
}
