#include "editor/shud_editor.h"

unsigned CG_SHUDEditorNameFlags(const char* name)
{
	unsigned flags;

	flags = SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_VIS | SHUD_PF_TIME;
	if (!name || !name[0])
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_STYLE;
	}
	if (CG_SHUDIsDefaultName(name))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	if (!Q_stricmp(name, "grid"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_VIS;
	}
	if (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate"))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	if (CG_SHUDEditorMatchFilter(name, "statusbar_") && CG_SHUDEditorMatchFilter(name, "bar"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_BAR | SHUD_PF_STYLE | SHUD_PF_VIS;
	}
	if (CG_SHUDEditorMatchFilter(name, "icon") ||
	    CG_SHUDEditorMatchFilter(name, "flagstatus") ||
	    CG_SHUDEditorMatchFilter(name, "itempickupicon") ||
	    CG_SHUDEditorMatchFilter(name, "rewardicons"))
	{
		return SHUD_PF_RECT | SHUD_PF_COLOR | SHUD_PF_BG | SHUD_PF_IMAGE | SHUD_PF_TIME | SHUD_PF_VIS;
	}
	if (CG_SHUDEditorMatchFilter(name, "obituar") ||
	    !Q_stricmp(name, "netgraph") ||
	    !Q_stricmp(name, "weaponlist") ||
	    (name[0] == 't' && name[1] == 'e' && name[2] == 'a' && name[3] == 'm' &&
	     name[4] >= '1' && name[4] <= '9'))
	{
		return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_IMAGE | SHUD_PF_STYLE;
	}
	return flags | SHUD_PF_FONT | SHUD_PF_ALIGN | SHUD_PF_TEXT | SHUD_PF_STYLE;
}

qboolean CG_SHUDEditorPropVisible(const superhudElement_t* el, int prop)
{
	unsigned flags;

	if (!el)
	{
		return qfalse;
	}
	flags = CG_SHUDEditorNameFlags(el->element.name);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			return (flags & SHUD_PF_RECT) ? qtrue : qfalse;
		case SHUD_PROP_FONT:
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			return (flags & SHUD_PF_FONT) ? qtrue : qfalse;
		case SHUD_PROP_ALIGNH:
		case SHUD_PROP_ALIGNV:
		case SHUD_PROP_TEXTALIGN:
			return (flags & SHUD_PF_ALIGN) ? qtrue : qfalse;
		case SHUD_PROP_TEXT:
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
		case SHUD_PROP_TEXTSTYLE:
		case SHUD_PROP_MONO:
		case SHUD_PROP_SHADOW:
			return (flags & SHUD_PF_TEXT) ? qtrue : qfalse;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_COLOR2:
			return (flags & SHUD_PF_COLOR) ? qtrue : qfalse;
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_FILL:
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			return (flags & SHUD_PF_BG) ? qtrue : qfalse;
		case SHUD_PROP_IMAGE:
		case SHUD_PROP_IMAGEALIGNH:
		case SHUD_PROP_IMAGEALIGNV:
			return (flags & SHUD_PF_IMAGE) ? qtrue : qfalse;
		case SHUD_PROP_DIRECTION:
		case SHUD_PROP_DOUBLEBAR:
			return (flags & SHUD_PF_BAR) ? qtrue : qfalse;
		case SHUD_PROP_STYLE:
			return (flags & SHUD_PF_STYLE) ? qtrue : qfalse;
		case SHUD_PROP_TIME:
		case SHUD_PROP_FADE:
		case SHUD_PROP_FADEDELAY:
			return (flags & SHUD_PF_TIME) ? qtrue : qfalse;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			return (flags & SHUD_PF_VIS) ? qtrue : qfalse;
		default:
			return qfalse;
	}
}

void CG_SHUDEditorFixPropIndex(void)
{
	int p;

	if (!editor.selected)
	{
		editor.propIndex = 0;
		return;
	}
	if (CG_SHUDEditorPropVisible(editor.selected, editor.propIndex))
	{
		return;
	}
	for (p = 0; p < SHUD_PROP_COUNT; p++)
	{
		if (CG_SHUDEditorPropVisible(editor.selected, p))
		{
			editor.propIndex = p;
			return;
		}
	}
	editor.propIndex = 0;
}

void CG_SHUDEditorCommit(int prop)
{
	int i;
	superhudElement_t* el;

	if (!editor.selected)
	{
		return;
	}
	editor.dirty = qtrue;
	CG_SHUDRecreateElement(editor.selected);
	if (prop < 0 || editor.selCount <= 1)
	{
		return;
	}
	for (i = 0; i < editor.selCount; i++)
	{
		el = editor.sel[i];
		if (!el || el == editor.selected)
		{
			continue;
		}
		if (!CG_SHUDEditorElementAlive(el))
		{
			continue;
		}
		CG_SHUDEditorCopyProp(el, editor.selected, prop);
		CG_SHUDRecreateElement(el);
	}
}

int CG_SHUDEditorCycle(int value, int dir, int count)
{
	if (count <= 0)
	{
		return 0;
	}
	value += dir;
	while (value < 0)
	{
		value += count;
	}
	while (value >= count)
	{
		value -= count;
	}
	return value;
}

void CG_SHUDEditorClamp01(float* v)
{
	if (*v < 0.0f)
	{
		*v = 0.0f;
	}
	if (*v > 1.0f)
	{
		*v = 1.0f;
	}
}

int CG_SHUDEditorCurrentVis(const superhudElement_t* el)
{
	if (el->config.visflags.isSet)
	{
		return el->config.visflags.value;
	}
	return el->element.visibility;
}

void CG_SHUDEditorToggleVis(superhudElement_t* el, int bit)
{
	int flags;

	flags = CG_SHUDEditorCurrentVis(el);
	flags ^= bit;
	el->config.visflags.isSet = qtrue;
	el->config.visflags.value = flags;
}

void CG_SHUDEditorEnsureRect(superhudElement_t* el)
{
	if (!el->config.rect.isSet)
	{
		el->config.rect.isSet = qtrue;
		el->config.rect.value[0] = 100.0f;
		el->config.rect.value[1] = 100.0f;
		el->config.rect.value[2] = 64.0f;
		el->config.rect.value[3] = 16.0f;
	}
}

void CG_SHUDEditorEnsureFontSize(superhudElement_t* el)
{
	if (!el->config.fontsize.isSet)
	{
		el->config.fontsize.isSet = qtrue;
		el->config.fontsize.value[0] = 8.0f;
		el->config.fontsize.value[1] = 8.0f;
	}
}

qboolean CG_SHUDEditorPropIsColor(int prop)
{
	return (prop == SHUD_PROP_COLOR ||
	        prop == SHUD_PROP_BGCOLOR ||
	        prop == SHUD_PROP_COLOR2 ||
	        prop == SHUD_PROP_BGCOLOR2 ||
	        prop == SHUD_PROP_BORDERC ||
	        prop == SHUD_PROP_BORDERC2);
}

qboolean CG_SHUDEditorPropIsVec4(int prop)
{
	return (prop == SHUD_PROP_FADE ||
	        prop == SHUD_PROP_SHADOW ||
	        prop == SHUD_PROP_HLCOLOR);
}

superhudColor_t* CG_SHUDEditorColorPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_BGCOLOR)
	{
		return &el->config.bgcolor.value;
	}
	if (prop == SHUD_PROP_COLOR2)
	{
		return &el->config.color2.value;
	}
	if (prop == SHUD_PROP_BGCOLOR2)
	{
		return &el->config.bgcolor2.value;
	}
	if (prop == SHUD_PROP_BORDERC)
	{
		return &el->config.borderColor.value;
	}
	if (prop == SHUD_PROP_BORDERC2)
	{
		return &el->config.borderColor2.value;
	}
	return &el->config.color.value;
}

qboolean* CG_SHUDEditorColorSetPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_BGCOLOR)
	{
		return &el->config.bgcolor.isSet;
	}
	if (prop == SHUD_PROP_COLOR2)
	{
		return &el->config.color2.isSet;
	}
	if (prop == SHUD_PROP_BGCOLOR2)
	{
		return &el->config.bgcolor2.isSet;
	}
	if (prop == SHUD_PROP_BORDERC)
	{
		return &el->config.borderColor.isSet;
	}
	if (prop == SHUD_PROP_BORDERC2)
	{
		return &el->config.borderColor2.isSet;
	}
	return &el->config.color.isSet;
}

float* CG_SHUDEditorVec4Ptr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_FADE)
	{
		return el->config.fade.value;
	}
	if (prop == SHUD_PROP_SHADOW)
	{
		return el->config.shadowColor.value;
	}
	if (prop == SHUD_PROP_HLCOLOR)
	{
		return el->config.hlcolor.value;
	}
	return NULL;
}

qboolean* CG_SHUDEditorVec4SetPtr(superhudElement_t* el, int prop)
{
	if (prop == SHUD_PROP_FADE)
	{
		return &el->config.fade.isSet;
	}
	if (prop == SHUD_PROP_SHADOW)
	{
		return &el->config.shadowColor.isSet;
	}
	if (prop == SHUD_PROP_HLCOLOR)
	{
		return &el->config.hlcolor.isSet;
	}
	return NULL;
}

float* CG_SHUDEditorRgbaPtr(superhudElement_t* el, int prop)
{
	superhudColor_t* c;

	if (CG_SHUDEditorPropIsVec4(prop))
	{
		return CG_SHUDEditorVec4Ptr(el, prop);
	}
	if (!CG_SHUDEditorPropIsColor(prop))
	{
		return NULL;
	}
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!c || c->type != SUPERHUD_COLOR_RGBA)
	{
		return NULL;
	}
	return c->rgba;
}

void CG_SHUDEditorEnsureColorProp(superhudElement_t* el, int prop)
{
	qboolean* setp;
	superhudColor_t* c;
	float* rgba;

	if (CG_SHUDEditorPropIsVec4(prop))
	{
		setp = CG_SHUDEditorVec4SetPtr(el, prop);
		rgba = CG_SHUDEditorVec4Ptr(el, prop);
		if (setp && rgba && !*setp)
		{
			*setp = qtrue;
			rgba[0] = 1.0f;
			rgba[1] = 1.0f;
			rgba[2] = 1.0f;
			rgba[3] = 1.0f;
		}
		else if (setp)
		{
			*setp = qtrue;
		}
		return;
	}
	setp = CG_SHUDEditorColorSetPtr(el, prop);
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!*setp || c->type != SUPERHUD_COLOR_RGBA)
	{
		*setp = qtrue;
		c->type = SUPERHUD_COLOR_RGBA;
		c->rgba[0] = 1.0f;
		c->rgba[1] = 1.0f;
		c->rgba[2] = 1.0f;
		c->rgba[3] = 1.0f;
	}
}

void CG_SHUDEditorCycleColorType(superhudElement_t* el, int prop, int dir)
{
	qboolean* setp;
	superhudColor_t* c;
	int type;

	setp = CG_SHUDEditorColorSetPtr(el, prop);
	c = CG_SHUDEditorColorPtr(el, prop);
	if (!*setp)
	{
		if (dir > 0)
		{
			CG_SHUDEditorEnsureColorProp(el, prop);
		}
		else
		{
			*setp = qtrue;
			c->type = SUPERHUD_COLOR_I;
		}
	}
	else
	{
		type = CG_SHUDEditorCycle((int)c->type, dir, 4);
		if (type == SUPERHUD_COLOR_RGBA)
		{
			CG_SHUDEditorEnsureColorProp(el, prop);
		}
		else
		{
			c->type = (superhudColorType_t)type;
		}
	}
	*setp = qtrue;
}

qboolean CG_SHUDEditorColorHasSliders(const superhudElement_t* el, int prop)
{
	const superhudColor_t* c;
	qboolean set;

	if (!el)
	{
		return qfalse;
	}
	if (CG_SHUDEditorPropIsVec4(prop))
	{
		if (prop == SHUD_PROP_FADE)
		{
			return el->config.fade.isSet;
		}
		if (prop == SHUD_PROP_SHADOW)
		{
			return el->config.shadowColor.isSet;
		}
		if (prop == SHUD_PROP_HLCOLOR)
		{
			return el->config.hlcolor.isSet;
		}
		return qfalse;
	}
	if (!CG_SHUDEditorPropIsColor(prop))
	{
		return qfalse;
	}
	set = *CG_SHUDEditorColorSetPtr((superhudElement_t*)el, prop);
	c = CG_SHUDEditorColorPtr((superhudElement_t*)el, prop);
	return (set && c->type == SUPERHUD_COLOR_RGBA) ? qtrue : qfalse;
}

int CG_SHUDEditorVisBit(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_VIS_IM: return SE_IM;
		case SHUD_PROP_VIS_SPECT: return SE_SPECT;
		case SHUD_PROP_VIS_DEAD: return SE_DEAD;
		case SHUD_PROP_VIS_SCOREHIDE: return SE_SCORES_HIDE;
		case SHUD_PROP_VIS_SHOWEMPTY: return SE_SHOW_EMPTY;
		case SHUD_PROP_VIS_TEAMONLY: return SE_TEAM_ONLY;
		case SHUD_PROP_VIS_DEMOHIDE: return SE_DEMO_HIDE;
		case SHUD_PROP_VIS_KEY1: return SE_KEY1_SHOW;
		case SHUD_PROP_VIS_KEY2: return SE_KEY2_SHOW;
		case SHUD_PROP_VIS_KEY3: return SE_KEY3_SHOW;
		case SHUD_PROP_VIS_KEY4: return SE_KEY4_SHOW;
		case SHUD_PROP_VIS_GTFFA: return SE_GT_FFA;
		case SHUD_PROP_VIS_GTTOURNEY: return SE_GT_TOURNEY;
		case SHUD_PROP_VIS_GTTDM: return SE_GT_TDM;
		case SHUD_PROP_VIS_GTCTF: return SE_GT_CTF;
		case SHUD_PROP_VIS_GTFT: return SE_GT_FREEZETAG;
		case SHUD_PROP_VIS_GTCA: return SE_GT_CLANARENA;
		default: return 0;
	}
}

void CG_SHUDEditorStartTypeIn(int prop, int channel)
{
	char cur[64];
	float* rgba;
	int iv;

	if (!editor.selected && prop >= 0)
	{
		return;
	}
	editor.typing = qtrue;
	editor.typeProp = prop;
	editor.typeChannel = channel;
	editor.typeBuf[0] = 0;
	if (prop == -1)
	{
		Q_strncpyz(editor.typeBuf, editor.listFilter, sizeof(editor.typeBuf));
	}
	else if (prop == -2)
	{
		Q_strncpyz(editor.typeBuf, editor.pickerFilter, sizeof(editor.typeBuf));
	}
	else if (channel >= 0 && channel <= 3 && editor.selected)
	{
		CG_SHUDEditorEnsureColorProp(editor.selected, prop);
		rgba = CG_SHUDEditorRgbaPtr(editor.selected, prop);
		if (rgba)
		{
			iv = (int)(rgba[channel] * 255.0f + 0.5f);
			if (iv < 0)
			{
				iv = 0;
			}
			if (iv > 255)
			{
				iv = 255;
			}
			Com_sprintf(cur, sizeof(cur), "%d", iv);
			Q_strncpyz(editor.typeBuf, cur, sizeof(editor.typeBuf));
		}
	}
	else if (prop >= 0)
	{
		CG_SHUDEditorPropValue(editor.selected, prop, cur, sizeof(cur));
		if (Q_stricmp(cur, "--") && Q_stricmp(cur, "?"))
		{
			Q_strncpyz(editor.typeBuf, cur, sizeof(editor.typeBuf));
		}
	}
}

void CG_SHUDEditorCancelTypeIn(void)
{
	editor.typing = qfalse;
	editor.typeChannel = -1;
	editor.typeBuf[0] = 0;
}

void CG_SHUDEditorSyncFilter(void)
{
	if (editor.typeProp == -1)
	{
		Q_strncpyz(editor.listFilter, editor.typeBuf, sizeof(editor.listFilter));
		editor.filterOn = editor.listFilter[0] ? qtrue : qfalse;
		editor.scroll = 0;
	}
	else if (editor.typeProp == -2)
	{
		Q_strncpyz(editor.pickerFilter, editor.typeBuf, sizeof(editor.pickerFilter));
		editor.pickerScroll = 0;
	}
}

void CG_SHUDEditorCommitTypeIn(void)
{
	superhudConfig_t* cfg;
	float v;
	int prop;
	int channel;
	int iv;
	float* rgba;

	prop = editor.typeProp;
	channel = editor.typeChannel;
	editor.typing = qfalse;
	editor.typeChannel = -1;
	if (prop == -1 || prop == -2)
	{
		CG_SHUDEditorSyncFilter();
		return;
	}
	if (!editor.selected)
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	editor.coalesceProp = -1;
	cfg = &editor.selected->config;
	if (channel >= 0 && channel <= 3)
	{
		CG_SHUDEditorEnsureColorProp(editor.selected, prop);
		rgba = CG_SHUDEditorRgbaPtr(editor.selected, prop);
		if (rgba)
		{
			iv = atoi(editor.typeBuf);
			if (iv < 0)
			{
				iv = 0;
			}
			if (iv > 255)
			{
				iv = 255;
			}
			rgba[channel] = (float)iv / 255.0f;
		}
		CG_SHUDEditorCommit(prop);
		return;
	}
	v = (float)atof(editor.typeBuf);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[0] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_Y:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[1] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_W:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[2] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_RECT_H:
			CG_SHUDEditorEnsureRect(editor.selected);
			cfg->rect.value[3] = v;
			CG_SHUDEditorClampRect(cfg->rect.value);
			break;
		case SHUD_PROP_FS_W:
			CG_SHUDEditorEnsureFontSize(editor.selected);
			cfg->fontsize.value[0] = v < 1.0f ? 1.0f : v;
			break;
		case SHUD_PROP_FS_H:
			CG_SHUDEditorEnsureFontSize(editor.selected);
			cfg->fontsize.value[1] = v < 1.0f ? 1.0f : v;
			break;
		case SHUD_PROP_FONT:
			cfg->font.isSet = qtrue;
			Q_strncpyz(cfg->font.value, editor.typeBuf, sizeof(cfg->font.value));
			break;
		case SHUD_PROP_IMAGE:
			cfg->image.isSet = qtrue;
			Q_strncpyz(cfg->image.value, editor.typeBuf, sizeof(cfg->image.value));
			break;
		case SHUD_PROP_TEXT:
			cfg->text.isSet = qtrue;
			Q_strncpyz(cfg->text.value, editor.typeBuf, sizeof(cfg->text.value));
			break;
		case SHUD_PROP_TEXTOFF_X:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[0] = v;
			break;
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[1] = v;
			break;
		case SHUD_PROP_TIME:
			cfg->time.isSet = qtrue;
			cfg->time.value = (int)v;
			break;
		case SHUD_PROP_FADEDELAY:
			cfg->fadedelay.isSet = qtrue;
			cfg->fadedelay.value = (int)v;
			break;
		case SHUD_PROP_HLSIZE:
			cfg->hlsize.isSet = qtrue;
			cfg->hlsize.value = (int)v;
			break;
		case SHUD_PROP_STYLE:
			cfg->style.isSet = qtrue;
			cfg->style.value = (int)v;
			break;
		case SHUD_PROP_BORDER_L:
			cfg->border.isSet = qtrue;
			cfg->border.value[0] = v;
			break;
		case SHUD_PROP_BORDER_T:
			cfg->border.isSet = qtrue;
			cfg->border.value[1] = v;
			break;
		case SHUD_PROP_BORDER_R:
			cfg->border.isSet = qtrue;
			cfg->border.value[2] = v;
			break;
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qtrue;
			cfg->border.value[3] = v;
			break;
		case SHUD_PROP_IMAGETC_S:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[0] = v;
			break;
		case SHUD_PROP_IMAGETC_T:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[1] = v;
			break;
		case SHUD_PROP_IMAGETC_U:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[2] = v;
			break;
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[3] = v;
			break;
		case SHUD_PROP_MARGIN_L:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[0] = v;
			break;
		case SHUD_PROP_MARGIN_T:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[1] = v;
			break;
		case SHUD_PROP_MARGIN_R:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[2] = v;
			break;
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[3] = v;
			break;
		default:
			break;
	}
	CG_SHUDEditorCommit(prop);
}

void CG_SHUDEditorTypeKey(int key)
{
	int len;

	if (key == K_ESCAPE)
	{
		CG_SHUDEditorCancelTypeIn();
		return;
	}
	if (key == K_ENTER)
	{
		CG_SHUDEditorCommitTypeIn();
		return;
	}
	if (key == K_BACKSPACE)
	{
		len = (int)strlen(editor.typeBuf);
		if (len > 0)
		{
			editor.typeBuf[len - 1] = 0;
			if (editor.typeProp < 0)
			{
				CG_SHUDEditorSyncFilter();
			}
		}
		return;
	}
	if (editor.typeChannel >= 0)
	{
		if (key < '0' || key > '9')
		{
			return;
		}
		len = (int)strlen(editor.typeBuf);
		if (len >= 3)
		{
			return;
		}
		editor.typeBuf[len] = (char)key;
		editor.typeBuf[len + 1] = 0;
		return;
	}
	if (key < 32 || key > 126)
	{
		return;
	}
	len = (int)strlen(editor.typeBuf);
	if (len >= SHUD_EDIT_TYPE_MAX - 1)
	{
		return;
	}
	editor.typeBuf[len] = (char)key;
	editor.typeBuf[len + 1] = 0;
	CG_SHUDEditorSyncFilter();
}

int CG_SHUDEditorPropVecIndex(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_TEXTOFF_X: return 0;
		case SHUD_PROP_TEXTOFF_Y: return 1;
		case SHUD_PROP_BORDER_L: return 0;
		case SHUD_PROP_BORDER_T: return 1;
		case SHUD_PROP_BORDER_R: return 2;
		case SHUD_PROP_BORDER_B: return 3;
		case SHUD_PROP_IMAGETC_S: return 0;
		case SHUD_PROP_IMAGETC_T: return 1;
		case SHUD_PROP_IMAGETC_U: return 2;
		case SHUD_PROP_IMAGETC_V: return 3;
		case SHUD_PROP_MARGIN_L: return 0;
		case SHUD_PROP_MARGIN_T: return 1;
		case SHUD_PROP_MARGIN_R: return 2;
		case SHUD_PROP_MARGIN_B: return 3;
		default: return -1;
	}
}

int CG_SHUDEditorPropCat(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			return 0;
		case SHUD_PROP_ALIGNH:
		case SHUD_PROP_ALIGNV:
		case SHUD_PROP_TEXTALIGN:
		case SHUD_PROP_DIRECTION:
			return 1;
		case SHUD_PROP_FONT:
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
		case SHUD_PROP_TEXT:
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
		case SHUD_PROP_TEXTSTYLE:
		case SHUD_PROP_MONO:
		case SHUD_PROP_SHADOW:
			return 2;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_FILL:
		case SHUD_PROP_FADE:
			return 3;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			return 4;
		case SHUD_PROP_IMAGE:
		case SHUD_PROP_IMAGEALIGNH:
		case SHUD_PROP_IMAGEALIGNV:
			return 5;
		case SHUD_PROP_STYLE:
		case SHUD_PROP_DOUBLEBAR:
			return 6;
		case SHUD_PROP_TIME:
		case SHUD_PROP_FADEDELAY:
			return 7;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			return 8;
		default:
			return 9;
	}
}

const char* CG_SHUDEditorPropCatTitle(int cat)
{
	switch (cat)
	{
		case 0: return "RECT";
		case 1: return "ALIGN";
		case 2: return "TEXT";
		case 3: return "COLOR";
		case 4: return "BORDER";
		case 5: return "IMAGE";
		case 6: return "STYLE";
		case 7: return "TIME";
		case 8: return "VIS";
		default: return "OTHER";
	}
}

int CG_SHUDEditorLayout(shudLay_t* out, int max)
{
	int n;
	int p;
	int ch;
	int cat;
	int have;
	float y;

	n = 0;
	y = SHUD_EDIT_PROP_LIST_Y - (float)editor.propScroll * SHUD_EDIT_PROP_ROW;
	for (cat = 0; cat <= 9 && n < max; cat++)
	{
		have = 0;
		for (p = 0; p < SHUD_PROP_COUNT && n < max; p++)
		{
			if (CG_SHUDEditorPropCat(p) != cat)
			{
				continue;
			}
			if (editor.selected && !CG_SHUDEditorPropVisible(editor.selected, p))
			{
				continue;
			}
			if (!have)
			{
				out[n].kind = 2;
				out[n].prop = cat;
				out[n].channel = 0;
				out[n].y = y;
				n++;
				y += SHUD_EDIT_PROP_ROW;
				have = 1;
				if (n >= max)
				{
					return n;
				}
			}
			out[n].kind = 0;
			out[n].prop = p;
			out[n].channel = 0;
			out[n].y = y;
			n++;
			y += SHUD_EDIT_PROP_ROW;
			if (CG_SHUDEditorColorHasSliders(editor.selected, p))
			{
				for (ch = 0; ch < 4 && n < max; ch++)
				{
					out[n].kind = 1;
					out[n].prop = p;
					out[n].channel = ch;
					out[n].y = y;
					n++;
					y += SHUD_EDIT_SLIDER_H;
				}
			}
		}
	}
	return n;
}

int CG_SHUDEditorHitLayout(shudLay_t* item)
{
	shudLay_t lay[SHUD_EDIT_LAY_MAX];
	int n;
	int i;
	float h;

	if (!CG_SHUDEditorCursorInProps())
	{
		return -1;
	}
	n = CG_SHUDEditorLayout(lay, SHUD_EDIT_LAY_MAX);
	for (i = 0; i < n; i++)
	{
		h = (lay[i].kind == 1) ? SHUD_EDIT_SLIDER_H : SHUD_EDIT_PROP_ROW;
		if (lay[i].kind == 2)
		{
			continue;
		}
		if (editor.cursorY >= lay[i].y && editor.cursorY < lay[i].y + h &&
		    editor.cursorY >= SHUD_EDIT_PROP_LIST_Y && editor.cursorY < SHUD_EDIT_PROP_BOTTOM)
		{
			if (item)
			{
				*item = lay[i];
			}
			return i;
		}
	}
	return -1;
}

qboolean CG_SHUDEditorPropIsTypeable(int prop)
{
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
		case SHUD_PROP_FONT:
		case SHUD_PROP_IMAGE:
		case SHUD_PROP_TEXT:
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
		case SHUD_PROP_TIME:
		case SHUD_PROP_FADEDELAY:
		case SHUD_PROP_HLSIZE:
		case SHUD_PROP_STYLE:
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			return qtrue;
		default:
			return qfalse;
	}
}

int CG_SHUDEditorHitPropsClick(shudLay_t* item, int* clickKind)
{
	shudLay_t lay[SHUD_EDIT_LAY_MAX];
	int n;
	int i;
	float h;

	if (clickKind)
	{
		*clickKind = 0;
	}
	if (!CG_SHUDEditorCursorInProps())
	{
		return -1;
	}
	n = CG_SHUDEditorLayout(lay, SHUD_EDIT_LAY_MAX);
	for (i = 0; i < n; i++)
	{
		h = (lay[i].kind == 1) ? SHUD_EDIT_SLIDER_H : SHUD_EDIT_PROP_ROW;
		if (lay[i].kind == 2)
		{
			continue;
		}
		if (editor.cursorY >= lay[i].y && editor.cursorY < lay[i].y + h &&
		    editor.cursorY >= SHUD_EDIT_PROP_LIST_Y && editor.cursorY < SHUD_EDIT_PROP_BOTTOM)
		{
			if (lay[i].kind == 0)
			{
				if (editor.cursorX >= SHUD_EDIT_PROP_VAL_X &&
				        editor.cursorX < SHUD_EDIT_PROP_VAL_X + SHUD_EDIT_PROP_VAL_W)
				{
					if (clickKind)
					{
						*clickKind = 1;
					}
				}
			}
			else if (lay[i].kind == 1)
			{
				if (editor.cursorX >= SHUD_EDIT_SLIDER_NUM_X &&
				        editor.cursorX < SHUD_EDIT_SLIDER_NUM_X + SHUD_EDIT_SLIDER_NUM_W)
				{
					if (clickKind)
					{
						*clickKind = 3;
					}
				}
				else
				{
					if (clickKind)
					{
						*clickKind = 2;
					}
				}
			}
			if (item)
			{
				*item = lay[i];
			}
			return i;
		}
	}
	return -1;
}

void CG_SHUDEditorSetSlider(int prop, int channel, float t)
{
	float* rgba;

	if (!editor.selected)
	{
		return;
	}
	if (channel < 0)
	{
		channel = 0;
	}
	if (channel > 3)
	{
		channel = 3;
	}
	CG_SHUDEditorEnsureColorProp(editor.selected, prop);
	rgba = CG_SHUDEditorRgbaPtr(editor.selected, prop);
	if (!rgba)
	{
		return;
	}
	if (t < 0.0f)
	{
		t = 0.0f;
	}
	if (t > 1.0f)
	{
		t = 1.0f;
	}
	rgba[channel] = t;
	CG_SHUDEditorCommit(prop);
}

void CG_SHUDEditorSliderFromCursor(int prop, int channel)
{
	float t;

	t = (editor.cursorX - (SHUD_EDIT_PROPS_X + 24.0f)) / (SHUD_EDIT_PROPS_W - 72.0f);
	CG_SHUDEditorSetSlider(prop, channel, t);
}

void CG_SHUDEditorCopyProp(superhudElement_t* dst, superhudElement_t* src, int prop)
{
	int bit;
	int flags;
	int idx;

	if (!dst || !src || dst == src)
	{
		return;
	}
	idx = CG_SHUDEditorPropVecIndex(prop);
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			if (!src->config.rect.isSet)
			{
				dst->config.rect.isSet = qfalse;
				break;
			}
			CG_SHUDEditorEnsureRect(dst);
			if (prop == SHUD_PROP_RECT_X)
			{
				dst->config.rect.value[0] = src->config.rect.value[0];
			}
			else if (prop == SHUD_PROP_RECT_Y)
			{
				dst->config.rect.value[1] = src->config.rect.value[1];
			}
			else if (prop == SHUD_PROP_RECT_W)
			{
				dst->config.rect.value[2] = src->config.rect.value[2];
			}
			else
			{
				dst->config.rect.value[3] = src->config.rect.value[3];
			}
			break;
		case SHUD_PROP_FONT:
			memcpy(&dst->config.font, &src->config.font, sizeof(dst->config.font));
			break;
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			if (!src->config.fontsize.isSet)
			{
				dst->config.fontsize.isSet = qfalse;
				break;
			}
			CG_SHUDEditorEnsureFontSize(dst);
			if (prop == SHUD_PROP_FS_W)
			{
				dst->config.fontsize.value[0] = src->config.fontsize.value[0];
			}
			else
			{
				dst->config.fontsize.value[1] = src->config.fontsize.value[1];
			}
			break;
		case SHUD_PROP_ALIGNH:
			dst->config.alignH.isSet = src->config.alignH.isSet;
			dst->config.alignH.value = src->config.alignH.value;
			break;
		case SHUD_PROP_ALIGNV:
			dst->config.alignV.isSet = src->config.alignV.isSet;
			dst->config.alignV.value = src->config.alignV.value;
			break;
		case SHUD_PROP_TEXTALIGN:
			dst->config.textAlign.isSet = src->config.textAlign.isSet;
			dst->config.textAlign.value = src->config.textAlign.value;
			break;
		case SHUD_PROP_DIRECTION:
			dst->config.direction.isSet = src->config.direction.isSet;
			dst->config.direction.value = src->config.direction.value;
			break;
		case SHUD_PROP_COLOR:
			memcpy(&dst->config.color, &src->config.color, sizeof(dst->config.color));
			break;
		case SHUD_PROP_BGCOLOR:
			memcpy(&dst->config.bgcolor, &src->config.bgcolor, sizeof(dst->config.bgcolor));
			break;
		case SHUD_PROP_COLOR2:
			memcpy(&dst->config.color2, &src->config.color2, sizeof(dst->config.color2));
			break;
		case SHUD_PROP_BGCOLOR2:
			memcpy(&dst->config.bgcolor2, &src->config.bgcolor2, sizeof(dst->config.bgcolor2));
			break;
		case SHUD_PROP_BORDERC:
			memcpy(&dst->config.borderColor, &src->config.borderColor, sizeof(dst->config.borderColor));
			break;
		case SHUD_PROP_BORDERC2:
			memcpy(&dst->config.borderColor2, &src->config.borderColor2, sizeof(dst->config.borderColor2));
			break;
		case SHUD_PROP_FADE:
			memcpy(&dst->config.fade, &src->config.fade, sizeof(dst->config.fade));
			break;
		case SHUD_PROP_SHADOW:
			memcpy(&dst->config.shadowColor, &src->config.shadowColor, sizeof(dst->config.shadowColor));
			break;
		case SHUD_PROP_HLCOLOR:
			memcpy(&dst->config.hlcolor, &src->config.hlcolor, sizeof(dst->config.hlcolor));
			break;
		case SHUD_PROP_IMAGE:
			memcpy(&dst->config.image, &src->config.image, sizeof(dst->config.image));
			break;
		case SHUD_PROP_IMAGEALIGNH:
			dst->config.imageAlignH.isSet = src->config.imageAlignH.isSet;
			dst->config.imageAlignH.value = src->config.imageAlignH.value;
			break;
		case SHUD_PROP_IMAGEALIGNV:
			dst->config.imageAlignV.isSet = src->config.imageAlignV.isSet;
			dst->config.imageAlignV.value = src->config.imageAlignV.value;
			break;
		case SHUD_PROP_TEXT:
			memcpy(&dst->config.text, &src->config.text, sizeof(dst->config.text));
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			if (!src->config.textOffset.isSet)
			{
				dst->config.textOffset.isSet = qfalse;
				break;
			}
			dst->config.textOffset.isSet = qtrue;
			dst->config.textOffset.value[idx] = src->config.textOffset.value[idx];
			break;
		case SHUD_PROP_ITTEAM:
			dst->config.itTeam.isSet = src->config.itTeam.isSet;
			dst->config.itTeam.value = src->config.itTeam.value;
			break;
		case SHUD_PROP_TIME:
			dst->config.time.isSet = src->config.time.isSet;
			dst->config.time.value = src->config.time.value;
			break;
		case SHUD_PROP_FADEDELAY:
			dst->config.fadedelay.isSet = src->config.fadedelay.isSet;
			dst->config.fadedelay.value = src->config.fadedelay.value;
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			if (!src->config.border.isSet)
			{
				dst->config.border.isSet = qfalse;
				break;
			}
			dst->config.border.isSet = qtrue;
			dst->config.border.value[idx] = src->config.border.value[idx];
			break;
		case SHUD_PROP_HLSIZE:
			dst->config.hlsize.isSet = src->config.hlsize.isSet;
			dst->config.hlsize.value = src->config.hlsize.value;
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			if (!src->config.imagetc.isSet)
			{
				dst->config.imagetc.isSet = qfalse;
				break;
			}
			dst->config.imagetc.isSet = qtrue;
			dst->config.imagetc.value[idx] = src->config.imagetc.value[idx];
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			if (!src->config.margins.isSet)
			{
				dst->config.margins.isSet = qfalse;
				break;
			}
			dst->config.margins.isSet = qtrue;
			dst->config.margins.value[idx] = src->config.margins.value[idx];
			break;
		case SHUD_PROP_STYLE:
			dst->config.style.isSet = src->config.style.isSet;
			dst->config.style.value = src->config.style.value;
			break;
		case SHUD_PROP_TEXTSTYLE:
			dst->config.textStyle.isSet = src->config.textStyle.isSet;
			dst->config.textStyle.value = src->config.textStyle.value;
			break;
		case SHUD_PROP_FILL:
			dst->config.fill.isSet = src->config.fill.isSet;
			break;
		case SHUD_PROP_MONO:
			dst->config.monospace.isSet = src->config.monospace.isSet;
			break;
		case SHUD_PROP_DOUBLEBAR:
			dst->config.doublebar.isSet = src->config.doublebar.isSet;
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			if (!src->config.visflags.isSet)
			{
				dst->config.visflags.isSet = qfalse;
				break;
			}
			bit = CG_SHUDEditorVisBit(prop);
			flags = CG_SHUDEditorCurrentVis(dst);
			if (CG_SHUDEditorCurrentVis(src) & bit)
			{
				flags |= bit;
			}
			else
			{
				flags &= ~bit;
			}
			dst->config.visflags.isSet = qtrue;
			dst->config.visflags.value = flags;
			break;
		default:
			break;
	}
}

void CG_SHUDEditorUnsetProp(int prop)
{
	superhudConfig_t* cfg;

	if (!editor.selected)
	{
		return;
	}
	if (!CG_SHUDEditorPropVisible(editor.selected, prop))
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	cfg = &editor.selected->config;
	switch (prop)
	{
		case SHUD_PROP_RECT_X:
		case SHUD_PROP_RECT_Y:
		case SHUD_PROP_RECT_W:
		case SHUD_PROP_RECT_H:
			cfg->rect.isSet = qfalse;
			break;
		case SHUD_PROP_FONT:
			cfg->font.isSet = qfalse;
			break;
		case SHUD_PROP_FS_W:
		case SHUD_PROP_FS_H:
			cfg->fontsize.isSet = qfalse;
			break;
		case SHUD_PROP_ALIGNH:
			cfg->alignH.isSet = qfalse;
			break;
		case SHUD_PROP_ALIGNV:
			cfg->alignV.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTALIGN:
			cfg->textAlign.isSet = qfalse;
			break;
		case SHUD_PROP_DIRECTION:
			cfg->direction.isSet = qfalse;
			break;
		case SHUD_PROP_COLOR:
			cfg->color.isSet = qfalse;
			break;
		case SHUD_PROP_BGCOLOR:
			cfg->bgcolor.isSet = qfalse;
			break;
		case SHUD_PROP_COLOR2:
			cfg->color2.isSet = qfalse;
			break;
		case SHUD_PROP_BGCOLOR2:
			cfg->bgcolor2.isSet = qfalse;
			break;
		case SHUD_PROP_BORDERC:
			cfg->borderColor.isSet = qfalse;
			break;
		case SHUD_PROP_BORDERC2:
			cfg->borderColor2.isSet = qfalse;
			break;
		case SHUD_PROP_FADE:
			cfg->fade.isSet = qfalse;
			break;
		case SHUD_PROP_SHADOW:
			cfg->shadowColor.isSet = qfalse;
			break;
		case SHUD_PROP_HLCOLOR:
			cfg->hlcolor.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGE:
			cfg->image.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGEALIGNH:
			cfg->imageAlignH.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGEALIGNV:
			cfg->imageAlignV.isSet = qfalse;
			break;
		case SHUD_PROP_TEXT:
			cfg->text.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qfalse;
			break;
		case SHUD_PROP_ITTEAM:
			cfg->itTeam.isSet = qfalse;
			break;
		case SHUD_PROP_TIME:
			cfg->time.isSet = qfalse;
			break;
		case SHUD_PROP_FADEDELAY:
			cfg->fadedelay.isSet = qfalse;
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qfalse;
			break;
		case SHUD_PROP_HLSIZE:
			cfg->hlsize.isSet = qfalse;
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qfalse;
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qfalse;
			break;
		case SHUD_PROP_STYLE:
			cfg->style.isSet = qfalse;
			break;
		case SHUD_PROP_TEXTSTYLE:
			cfg->textStyle.isSet = qfalse;
			break;
		case SHUD_PROP_FILL:
			cfg->fill.isSet = qfalse;
			break;
		case SHUD_PROP_MONO:
			cfg->monospace.isSet = qfalse;
			break;
		case SHUD_PROP_DOUBLEBAR:
			cfg->doublebar.isSet = qfalse;
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			cfg->visflags.isSet = qfalse;
			break;
		default:
			break;
	}
	CG_SHUDEditorCommit(prop);
}

void CG_SHUDEditorAdjustProp(int prop, int dir)
{
	superhudElement_t* el;
	superhudConfig_t* cfg;
	float step;
	int fontIndex;
	int fontCount;
	int i;
	int idx;
	const char* fontName;

	el = editor.selected;
	if (!el || dir == 0)
	{
		return;
	}
	if (!CG_SHUDEditorPropVisible(el, prop))
	{
		return;
	}
	CG_SHUDEditorPushSelConfigUndo(prop);
	editor.coalesceProp = prop;
	cfg = &el->config;
	step = trap_Key_IsDown(K_SHIFT) ? 10.0f : 1.0f;

	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[0] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_Y:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[1] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_W:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[2] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_RECT_H:
			CG_SHUDEditorEnsureRect(el);
			cfg->rect.value[3] += (float)dir * step;
			CG_SHUDEditorClampRect(cfg->rect.value);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FONT:
			fontCount = CG_GetFontCount();
			if (fontCount <= 0)
			{
				break;
			}
			fontIndex = cfg->font.isSet ? CG_FontIndexFromName(cfg->font.value) : 0;
			fontIndex = CG_SHUDEditorCycle(fontIndex, dir, fontCount);
			cfg->font.isSet = qtrue;
			fontName = CG_GetFontName(fontIndex);
			if (!fontName)
			{
				fontName = "";
			}
			Q_strncpyz(cfg->font.value, fontName, sizeof(cfg->font.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FS_W:
			CG_SHUDEditorEnsureFontSize(el);
			cfg->fontsize.value[0] += (float)dir * step;
			if (cfg->fontsize.value[0] < 1.0f)
			{
				cfg->fontsize.value[0] = 1.0f;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FS_H:
			CG_SHUDEditorEnsureFontSize(el);
			cfg->fontsize.value[1] += (float)dir * step;
			if (cfg->fontsize.value[1] < 1.0f)
			{
				cfg->fontsize.value[1] = 1.0f;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ALIGNH:
			if (!cfg->alignH.isSet)
			{
				cfg->alignH.isSet = qtrue;
				cfg->alignH.value = SUPERHUD_ALIGNH_LEFT;
			}
			else
			{
				cfg->alignH.value = (superhudAlignH_t)CG_SHUDEditorCycle((int)cfg->alignH.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ALIGNV:
			if (!cfg->alignV.isSet)
			{
				cfg->alignV.isSet = qtrue;
				cfg->alignV.value = SUPERHUD_ALIGNV_TOP;
			}
			else
			{
				cfg->alignV.value = (superhudAlignV_t)CG_SHUDEditorCycle((int)cfg->alignV.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTALIGN:
			if (!cfg->textAlign.isSet)
			{
				cfg->textAlign.isSet = qtrue;
				cfg->textAlign.value = SUPERHUD_ALIGNH_LEFT;
			}
			else
			{
				cfg->textAlign.value = (superhudAlignH_t)CG_SHUDEditorCycle((int)cfg->textAlign.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_DIRECTION:
			if (!cfg->direction.isSet)
			{
				cfg->direction.isSet = qtrue;
				cfg->direction.value = SUPERHUD_DIR_LEFT_TO_RIGHT;
			}
			else
			{
				cfg->direction.value = (superhudDirection_t)CG_SHUDEditorCycle((int)cfg->direction.value, dir, 4);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			CG_SHUDEditorCycleColorType(el, prop, dir);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FADE:
		case SHUD_PROP_SHADOW:
		case SHUD_PROP_HLCOLOR:
			CG_SHUDEditorEnsureColorProp(el, prop);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_ITTEAM:
			if (!cfg->itTeam.isSet)
			{
				cfg->itTeam.isSet = qtrue;
				cfg->itTeam.value = SUPERHUD_ITTEAM_OWN;
			}
			else
			{
				cfg->itTeam.value = (superhudItTeam_t)CG_SHUDEditorCycle((int)cfg->itTeam.value, dir, 5);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TIME:
			if (!cfg->time.isSet)
			{
				cfg->time.isSet = qtrue;
				cfg->time.value = 0;
			}
			cfg->time.value += dir * (int)step * 100;
			if (cfg->time.value < 0)
			{
				cfg->time.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FADEDELAY:
			if (!cfg->fadedelay.isSet)
			{
				cfg->fadedelay.isSet = qtrue;
				cfg->fadedelay.value = 1000;
			}
			cfg->fadedelay.value += dir * (int)step * 50;
			if (cfg->fadedelay.value < 0)
			{
				cfg->fadedelay.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_HLSIZE:
			if (!cfg->hlsize.isSet)
			{
				cfg->hlsize.isSet = qtrue;
				cfg->hlsize.value = 1;
			}
			cfg->hlsize.value += dir * (int)step;
			if (cfg->hlsize.value < 0)
			{
				cfg->hlsize.value = 0;
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			cfg->textOffset.isSet = qtrue;
			cfg->textOffset.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			cfg->border.isSet = qtrue;
			cfg->border.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			cfg->imagetc.isSet = qtrue;
			cfg->imagetc.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * (trap_Key_IsDown(K_SHIFT) ? 0.1f : 0.01f);
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			cfg->margins.isSet = qtrue;
			cfg->margins.value[CG_SHUDEditorPropVecIndex(prop)] += (float)dir * step;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGE:
			idx = 0;
			if (cfg->image.isSet)
			{
				for (i = 0; i < SHUD_EDIT_IMG_MAX; i++)
				{
					if (!Q_stricmp(cfg->image.value, shudEditImages[i]))
					{
						idx = i;
						break;
					}
				}
			}
			idx = CG_SHUDEditorCycle(idx, dir, SHUD_EDIT_IMG_MAX);
			cfg->image.isSet = qtrue;
			Q_strncpyz(cfg->image.value, shudEditImages[idx], sizeof(cfg->image.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGEALIGNH:
			if (!cfg->imageAlignH.isSet)
			{
				cfg->imageAlignH.isSet = qtrue;
				cfg->imageAlignH.value = SUPERHUD_ALIGNH_LEFT;
			}
			else
			{
				cfg->imageAlignH.value = (superhudAlignH_t)CG_SHUDEditorCycle((int)cfg->imageAlignH.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_IMAGEALIGNV:
			if (!cfg->imageAlignV.isSet)
			{
				cfg->imageAlignV.isSet = qtrue;
				cfg->imageAlignV.value = SUPERHUD_ALIGNV_TOP;
			}
			else
			{
				cfg->imageAlignV.value = (superhudAlignV_t)CG_SHUDEditorCycle((int)cfg->imageAlignV.value, dir, 3);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXT:
			idx = 0;
			if (cfg->text.isSet)
			{
				for (i = 0; i < SHUD_EDIT_TXT_MAX; i++)
				{
					if (!Q_stricmp(cfg->text.value, shudEditTexts[i]))
					{
						idx = i;
						break;
					}
				}
			}
			idx = CG_SHUDEditorCycle(idx, dir, SHUD_EDIT_TXT_MAX);
			cfg->text.isSet = qtrue;
			Q_strncpyz(cfg->text.value, shudEditTexts[idx], sizeof(cfg->text.value));
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_STYLE:
			if (!cfg->style.isSet)
			{
				cfg->style.isSet = qtrue;
				cfg->style.value = 0;
			}
			else
			{
				cfg->style.value += dir;
				if (cfg->style.value < 0)
				{
					cfg->style.value = 0;
				}
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_TEXTSTYLE:
			if (!cfg->textStyle.isSet)
			{
				cfg->textStyle.isSet = qtrue;
				cfg->textStyle.value = 0;
			}
			else
			{
				cfg->textStyle.value = CG_SHUDEditorCycle(cfg->textStyle.value, dir, 2);
			}
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_FILL:
			cfg->fill.isSet = cfg->fill.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_MONO:
			cfg->monospace.isSet = cfg->monospace.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_DOUBLEBAR:
			cfg->doublebar.isSet = cfg->doublebar.isSet ? qfalse : qtrue;
			CG_SHUDEditorCommit(prop);
			break;
		case SHUD_PROP_VIS_IM:
		case SHUD_PROP_VIS_SPECT:
		case SHUD_PROP_VIS_DEAD:
		case SHUD_PROP_VIS_SCOREHIDE:
		case SHUD_PROP_VIS_SHOWEMPTY:
		case SHUD_PROP_VIS_TEAMONLY:
		case SHUD_PROP_VIS_DEMOHIDE:
		case SHUD_PROP_VIS_KEY1:
		case SHUD_PROP_VIS_KEY2:
		case SHUD_PROP_VIS_KEY3:
		case SHUD_PROP_VIS_KEY4:
		case SHUD_PROP_VIS_GTFFA:
		case SHUD_PROP_VIS_GTTOURNEY:
		case SHUD_PROP_VIS_GTTDM:
		case SHUD_PROP_VIS_GTCTF:
		case SHUD_PROP_VIS_GTFT:
		case SHUD_PROP_VIS_GTCA:
			CG_SHUDEditorToggleVis(el, CG_SHUDEditorVisBit(prop));
			CG_SHUDEditorCommit(prop);
			break;
		default:
			break;
	}
}

qboolean CG_SHUDEditorPropIsToggle(int prop)
{
	if (CG_SHUDEditorVisBit(prop))
	{
		return qtrue;
	}
	return (prop == SHUD_PROP_COLOR ||
	        prop == SHUD_PROP_BGCOLOR ||
	        prop == SHUD_PROP_COLOR2 ||
	        prop == SHUD_PROP_BGCOLOR2 ||
	        prop == SHUD_PROP_BORDERC ||
	        prop == SHUD_PROP_BORDERC2 ||
	        prop == SHUD_PROP_FADE ||
	        prop == SHUD_PROP_SHADOW ||
	        prop == SHUD_PROP_HLCOLOR ||
	        prop == SHUD_PROP_FONT ||
	        prop == SHUD_PROP_ALIGNH ||
	        prop == SHUD_PROP_ALIGNV ||
	        prop == SHUD_PROP_IMAGEALIGNH ||
	        prop == SHUD_PROP_IMAGEALIGNV ||
	        prop == SHUD_PROP_TEXTALIGN ||
	        prop == SHUD_PROP_DIRECTION ||
	        prop == SHUD_PROP_ITTEAM ||
	        prop == SHUD_PROP_IMAGE ||
	        prop == SHUD_PROP_TEXT ||
	        prop == SHUD_PROP_FILL ||
	        prop == SHUD_PROP_MONO ||
	        prop == SHUD_PROP_DOUBLEBAR ||
	        prop == SHUD_PROP_TEXTSTYLE);
}

void CG_SHUDEditorClickProps(void)
{
	shudLay_t item;
	int clickKind;

	if (editor.cursorX >= SHUD_EDIT_PROPS_X + SHUD_EDIT_PROPS_W - 16.0f &&
	    editor.cursorY >= SHUD_EDIT_PROPS_Y + 2.0f &&
	    editor.cursorY <= SHUD_EDIT_PROPS_Y + 14.0f)
	{
		editor.showProps = qfalse;
		return;
	}
	if (CG_SHUDEditorHitPropsClick(&item, &clickKind) < 0)
	{
		return;
	}
	editor.propIndex = item.prop;
	if (clickKind == 3)
	{
		CG_SHUDEditorStartTypeIn(item.prop, item.channel);
		return;
	}
	if (clickKind == 1 && CG_SHUDEditorPropIsTypeable(item.prop))
	{
		CG_SHUDEditorStartTypeIn(item.prop, -1);
		return;
	}
	if (clickKind == 2 && item.kind == 1)
	{
		editor.sliderProp = item.prop;
		editor.sliderChannel = item.channel;
		CG_SHUDEditorPushSelConfigUndo(item.prop);
		editor.undoLock = qtrue;
		editor.drag = SHUD_DRAG_SLIDER;
		CG_SHUDEditorSliderFromCursor(item.prop, item.channel);
		return;
	}
	if (clickKind == 0 && CG_SHUDEditorPropIsToggle(item.prop))
	{
		CG_SHUDEditorAdjustProp(item.prop, 1);
	}
}

int CG_SHUDEditorPropScrollMax(void)
{
	int saved;
	int n;
	int maxScroll;
	float lastY;
	float rowH;
	shudLay_t lay[SHUD_EDIT_LAY_MAX];

	saved = editor.propScroll;
	editor.propScroll = 0;
	n = CG_SHUDEditorLayout(lay, SHUD_EDIT_LAY_MAX);
	editor.propScroll = saved;
	if (n <= 0)
	{
		return 0;
	}
	rowH = (lay[n - 1].kind == 1) ? SHUD_EDIT_SLIDER_H : SHUD_EDIT_PROP_ROW;
	lastY = lay[n - 1].y + rowH;
	maxScroll = (int)((lastY - SHUD_EDIT_PROP_BOTTOM + SHUD_EDIT_PROP_ROW - 1.0f) / SHUD_EDIT_PROP_ROW);
	if (maxScroll < 0)
	{
		maxScroll = 0;
	}
	return maxScroll;
}

void CG_SHUDEditorScrollProps(int delta)
{
	int maxScroll;

	editor.propScroll += delta;
	if (editor.propScroll < 0)
	{
		editor.propScroll = 0;
	}
	maxScroll = CG_SHUDEditorPropScrollMax();
	if (editor.propScroll > maxScroll)
	{
		editor.propScroll = maxScroll;
	}
}

void CG_SHUDEditorPropValue(const superhudElement_t* el, int prop, char* out, int outSize)
{
	const superhudConfig_t* cfg;
	int flags;
	static const char* hnames[3] = { "L", "C", "R" };
	static const char* vnames[3] = { "T", "C", "B" };

	if (!out || outSize < 1)
	{
		return;
	}
	out[0] = 0;
	if (!el)
	{
		return;
	}
	cfg = &el->config;

	switch (prop)
	{
		case SHUD_PROP_RECT_X:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[0]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_Y:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[1]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_W:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[2]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_RECT_H:
			if (cfg->rect.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->rect.value[3]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FONT:
			if (cfg->font.isSet)
			{
				Q_strncpyz(out, cfg->font.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FS_W:
			if (cfg->fontsize.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->fontsize.value[0]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FS_H:
			if (cfg->fontsize.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->fontsize.value[1]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_ALIGNH:
			if (cfg->alignH.isSet && (int)cfg->alignH.value >= 0 && (int)cfg->alignH.value < 3)
			{
				Q_strncpyz(out, hnames[cfg->alignH.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_ALIGNV:
			if (cfg->alignV.isSet && (int)cfg->alignV.value >= 0 && (int)cfg->alignV.value < 3)
			{
				Q_strncpyz(out, vnames[cfg->alignV.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTALIGN:
			if (cfg->textAlign.isSet && (int)cfg->textAlign.value >= 0 && (int)cfg->textAlign.value < 3)
			{
				Q_strncpyz(out, hnames[cfg->textAlign.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_COLOR:
		case SHUD_PROP_BGCOLOR:
		case SHUD_PROP_COLOR2:
		case SHUD_PROP_BGCOLOR2:
		case SHUD_PROP_BORDERC:
		case SHUD_PROP_BORDERC2:
			{
				const superhudColor_t* c;
				qboolean set;
				c = CG_SHUDEditorColorPtr((superhudElement_t*)el, prop);
				set = *CG_SHUDEditorColorSetPtr((superhudElement_t*)el, prop);
				if (!set)
				{
					Q_strncpyz(out, "--", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_T)
				{
					Q_strncpyz(out, "T", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_E)
				{
					Q_strncpyz(out, "E", outSize);
				}
				else if (c->type == SUPERHUD_COLOR_I)
				{
					Q_strncpyz(out, "I", outSize);
				}
				else
				{
					Q_strncpyz(out, "RGBA", outSize);
				}
			}
			break;
		case SHUD_PROP_FADE:
		case SHUD_PROP_SHADOW:
		case SHUD_PROP_HLCOLOR:
			{
				qboolean* sp;
				sp = CG_SHUDEditorVec4SetPtr((superhudElement_t*)el, prop);
				Q_strncpyz(out, (sp && *sp) ? "RGBA" : "--", outSize);
			}
			break;
		case SHUD_PROP_DIRECTION:
			if (!cfg->direction.isSet)
			{
				Q_strncpyz(out, "--", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_RIGHT_TO_LEFT)
			{
				Q_strncpyz(out, "R", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_TOP_TO_BOTTOM)
			{
				Q_strncpyz(out, "T", outSize);
			}
			else if (cfg->direction.value == SUPERHUD_DIR_BOTTOM_TO_TOP)
			{
				Q_strncpyz(out, "B", outSize);
			}
			else
			{
				Q_strncpyz(out, "L", outSize);
			}
			break;
		case SHUD_PROP_IMAGE:
			if (cfg->image.isSet)
			{
				Q_strncpyz(out, cfg->image.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_IMAGEALIGNH:
			if (cfg->imageAlignH.isSet && (int)cfg->imageAlignH.value >= 0 && (int)cfg->imageAlignH.value < 3)
			{
				Q_strncpyz(out, hnames[cfg->imageAlignH.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_IMAGEALIGNV:
			if (cfg->imageAlignV.isSet && (int)cfg->imageAlignV.value >= 0 && (int)cfg->imageAlignV.value < 3)
			{
				Q_strncpyz(out, vnames[cfg->imageAlignV.value], outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXT:
			if (cfg->text.isSet)
			{
				Q_strncpyz(out, cfg->text.value, outSize);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_STYLE:
			if (cfg->style.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->style.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTSTYLE:
			if (cfg->textStyle.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->textStyle.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FILL:
			Q_strncpyz(out, cfg->fill.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_MONO:
			Q_strncpyz(out, cfg->monospace.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_DOUBLEBAR:
			Q_strncpyz(out, cfg->doublebar.isSet ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_IM:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_IM) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SPECT:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SPECT) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_DEAD:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_DEAD) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SCOREHIDE:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SCORES_HIDE) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_SHOWEMPTY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_SHOW_EMPTY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_TEAMONLY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_TEAM_ONLY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_DEMOHIDE:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_DEMO_HIDE) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY1:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY1_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY2:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY2_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY3:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY3_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_KEY4:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_KEY4_SHOW) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTFFA:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_FFA) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTTOURNEY:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_TOURNEY) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTTDM:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_TDM) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTCTF:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_CTF) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTFT:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_FREEZETAG) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_VIS_GTCA:
			flags = CG_SHUDEditorCurrentVis(el);
			Q_strncpyz(out, (flags & SE_GT_CLANARENA) ? "on" : "off", outSize);
			break;
		case SHUD_PROP_ITTEAM:
			if (!cfg->itTeam.isSet)
			{
				Q_strncpyz(out, "--", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_BLUE)
			{
				Q_strncpyz(out, "blue", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_RED)
			{
				Q_strncpyz(out, "red", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_NEUTRAL)
			{
				Q_strncpyz(out, "neu", outSize);
			}
			else if (cfg->itTeam.value == SUPERHUD_ITTEAM_ENEMY)
			{
				Q_strncpyz(out, "enemy", outSize);
			}
			else
			{
				Q_strncpyz(out, "own", outSize);
			}
			break;
		case SHUD_PROP_TIME:
			if (cfg->time.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->time.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_FADEDELAY:
			if (cfg->fadedelay.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->fadedelay.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_HLSIZE:
			if (cfg->hlsize.isSet)
			{
				Com_sprintf(out, outSize, "%d", cfg->hlsize.value);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_TEXTOFF_X:
		case SHUD_PROP_TEXTOFF_Y:
			if (cfg->textOffset.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->textOffset.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_BORDER_L:
		case SHUD_PROP_BORDER_T:
		case SHUD_PROP_BORDER_R:
		case SHUD_PROP_BORDER_B:
			if (cfg->border.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->border.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_IMAGETC_S:
		case SHUD_PROP_IMAGETC_T:
		case SHUD_PROP_IMAGETC_U:
		case SHUD_PROP_IMAGETC_V:
			if (cfg->imagetc.isSet)
			{
				Com_sprintf(out, outSize, "%.2f", cfg->imagetc.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		case SHUD_PROP_MARGIN_L:
		case SHUD_PROP_MARGIN_T:
		case SHUD_PROP_MARGIN_R:
		case SHUD_PROP_MARGIN_B:
			if (cfg->margins.isSet)
			{
				Com_sprintf(out, outSize, "%.0f", cfg->margins.value[CG_SHUDEditorPropVecIndex(prop)]);
			}
			else
			{
				Q_strncpyz(out, "--", outSize);
			}
			break;
		default:
			Q_strncpyz(out, "?", outSize);
			break;
	}
}

