// Copyright (C) 1999-2000 Id Software, Inc.
/*
=======================================================================

CROSSHAIR EDITOR MENU — INACTIVE (entry removed from Setup menu for now)

=======================================================================
*/

#include "ui_local.h"

#define ART_FRAMEL              "menu/art/frame2_l"
#define ART_FRAMER              "menu/art/frame1_r"
#define ART_BACK0               "menu/art/back_0"
#define ART_BACK1               "menu/art/back_1"

#define PREVIEW_X               368
#define PREVIEW_Y               72
#define PREVIEW_W               256
#define PREVIEW_H               256
#define PREVIEW_CONTROLS_Y      ( PREVIEW_Y + PREVIEW_H + 8 )
#define CROSSHAIR_TEXTURE_SIZE  256
#define OSP2BE_PK3DIR           "osp2be.pk3dir"

#define ID_LINES_THICKNESS      100
#define ID_LINES_THICKNESS_FIELD    101
#define ID_LINES_LENGTH         102
#define ID_LINES_LENGTH_FIELD   103
#define ID_LINES_GAP            104
#define ID_LINES_GAP_FIELD      105
#define ID_LINES_OPACITY        106
#define ID_LINES_OPACITY_FIELD  107
#define ID_LINES_TYPE           108
#define ID_DOT_ENABLE           109
#define ID_DOT_SIZE             110
#define ID_DOT_SIZE_FIELD       111
#define ID_DOT_OPACITY          112
#define ID_DOT_OPACITY_FIELD    113
#define ID_OUTLINE_THICKNESS    114
#define ID_OUTLINE_THICKNESS_FIELD  115
#define ID_OUTLINE_OPACITY      116
#define ID_OUTLINE_OPACITY_FIELD    117
#define ID_DOT_OUTLINE_THICKNESS    124
#define ID_DOT_OUTLINE_THICKNESS_FIELD  125
#define ID_DOT_OUTLINE_OPACITY  126
#define ID_DOT_OUTLINE_OPACITY_FIELD    127
#define ID_COLOR_MAIN           118
#define ID_COLOR_OUTLINE        119
#define ID_BG_IMAGE             120
#define ID_SHADER_SLOT          121
#define ID_EXPORT               122
#define ID_BACK                 123

typedef struct
{
	menuframework_s     menu;
	menutext_s          banner;
	menubitmap_s        framel;
	menubitmap_s        framer;
	menubitmap_s        back;
	menutext_s          exportButton;

	menuslider_s            linesThickness;
	menufield_s             linesThicknessField;
	menuslider_s            linesLength;
	menufield_s             linesLengthField;
	menuslider_s            linesGap;
	menufield_s             linesGapField;
	menuslider_s            linesOpacity;
	menufield_s             linesOpacityField;
	menulist_s              linesType;

	menuradiobutton_s       dotEnable;
	menuslider_s            dotSize;
	menufield_s             dotSizeField;
	menuslider_s            dotOpacity;
	menufield_s             dotOpacityField;
	menuslider_s            dotOutlineThickness;
	menufield_s             dotOutlineThicknessField;
	menuslider_s            dotOutlineOpacity;
	menufield_s             dotOutlineOpacityField;

	menuslider_s            outlineThickness;
	menufield_s             outlineThicknessField;
	menuslider_s            outlineOpacity;
	menufield_s             outlineOpacityField;

	menufield_s             colorMainField;
	menufield_s             colorOutlineField;
	menulist_s              bgImage;
	menulist_s              shaderSlot;

	qhandle_t               bgShaders[4];
} crosshairMenu_t;

static crosshairMenu_t s_crosshair;
static menuslider_s* s_crosshairDragSlider;

static const char* crosshairBgNames[] =
{
	"None",
	"q3dm1",
	"q3dm6",
	"q3tourney2",
	NULL
};

static const char* crosshairSlotNames[] =
{
	"1", "2", "3", "4", "5", "6", "7", "8",
	"9", "10", "11", "12", "13", "14", "15", "16",
	NULL
};

static const char* crosshairLinesTypeNames[] =
{
	"full",
	"t-style",
	"hidden",
	NULL
};

static menuslider_s* Crosshair_GetSliderById(int id)
{
	switch (id)
	{
		case ID_LINES_THICKNESS:
			return &s_crosshair.linesThickness;
		case ID_LINES_LENGTH:
			return &s_crosshair.linesLength;
		case ID_LINES_GAP:
			return &s_crosshair.linesGap;
		case ID_LINES_OPACITY:
			return &s_crosshair.linesOpacity;
		case ID_DOT_SIZE:
			return &s_crosshair.dotSize;
		case ID_DOT_OPACITY:
			return &s_crosshair.dotOpacity;
		case ID_DOT_OUTLINE_THICKNESS:
			return &s_crosshair.dotOutlineThickness;
		case ID_DOT_OUTLINE_OPACITY:
			return &s_crosshair.dotOutlineOpacity;
		case ID_OUTLINE_THICKNESS:
			return &s_crosshair.outlineThickness;
		case ID_OUTLINE_OPACITY:
			return &s_crosshair.outlineOpacity;
		default:
			return NULL;
	}
}

static void Crosshair_ToUpperInPlace(char* text)
{
	int i;
	char c;

	if (!text)
	{
		return;
	}

	for (i = 0; text[i]; i++)
	{
		c = text[i];
		if (c >= 'a' && c <= 'z')
		{
			text[i] = (char)(c - ('a' - 'A'));
		}
	}
}

static int Crosshair_ClampInt(int value, int minValue, int maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	if (value > maxValue)
	{
		return maxValue;
	}
	return value;
}

static void Crosshair_SetSliderValue(menuslider_s* slider, int value)
{
	float range;

	if (!slider)
	{
		return;
	}

	value = Crosshair_ClampInt(value, (int)slider->minvalue, (int)slider->maxvalue);
	slider->curvalue = (float)value;
	range = slider->maxvalue - slider->minvalue;
	if (range > 0.0f)
	{
		slider->range = (slider->curvalue - slider->minvalue) / range;
	}
	else
	{
		slider->range = 0.0f;
	}
}

static void Crosshair_SetFieldInt(menufield_s* field, int value)
{
	char buffer[16];

	if (!field)
	{
		return;
	}

	Com_sprintf(buffer, sizeof(buffer), "%d", value);
	Q_strncpyz(field->field.buffer, buffer, sizeof(field->field.buffer));
	field->field.cursor = strlen(field->field.buffer);
}

static void Crosshair_SyncFieldFromSlider(menufield_s* field, menuslider_s* slider)
{
	if (!field || !slider)
	{
		return;
	}

	Crosshair_SetFieldInt(field, (int)(slider->curvalue + 0.5f));
}

static void Crosshair_SyncSliderFromField(menuslider_s* slider, menufield_s* field)
{
	int value;
	int i;
	qboolean hasDigit;

	if (!field || !slider)
	{
		return;
	}

	hasDigit = qfalse;
	for (i = 0; field->field.buffer[i]; i++)
	{
		if (field->field.buffer[i] >= '0' && field->field.buffer[i] <= '9')
		{
			hasDigit = qtrue;
			break;
		}
	}
	if (!hasDigit)
	{
		Crosshair_SyncFieldFromSlider(field, slider);
		return;
	}

	value = atoi(field->field.buffer);
	Crosshair_SetSliderValue(slider, value);
	Crosshair_SyncFieldFromSlider(field, slider);
}

static int Crosshair_HexDigit(char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	if (c >= 'a' && c <= 'f')
	{
		return 10 + (c - 'a');
	}
	if (c >= 'A' && c <= 'F')
	{
		return 10 + (c - 'A');
	}
	return -1;
}

static qboolean Crosshair_ParseHexColor(const char* text, byte rgba[4], int alpha)
{
	int i;
	int hi;
	int lo;
	const char* ptr;

	if (!text || !rgba)
	{
		return qfalse;
	}

	ptr = text;
	if (ptr[0] == '#')
	{
		ptr++;
	}

	if (strlen(ptr) != 6)
	{
		return qfalse;
	}

	for (i = 0; i < 3; i++)
	{
		hi = Crosshair_HexDigit(ptr[i * 2]);
		lo = Crosshair_HexDigit(ptr[i * 2 + 1]);
		if (hi < 0 || lo < 0)
		{
			return qfalse;
		}
		rgba[i] = (byte)((hi << 4) | lo);
	}
	rgba[3] = (byte)Crosshair_ClampInt(alpha, 0, 255);
	return qtrue;
}

static qboolean Crosshair_PointInRect(int x, int y, int left, int top, int right, int bottom)
{
	return (x >= left && x < right && y >= top && y < bottom) ? qtrue : qfalse;
}

static float Crosshair_GetDotCoverage(int x, int y, int dotSize)
{
	float radius;
	float cx;
	float cy;
	float sampleX[4];
	float sampleY[4];
	int hitCount;
	int i;
	float dx;
	float dy;

	if (dotSize <= 0)
	{
		return 0.0f;
	}

	radius = dotSize * 0.5f;
	cx = (float)(CROSSHAIR_TEXTURE_SIZE / 2) + 0.5f;
	cy = (float)(CROSSHAIR_TEXTURE_SIZE / 2) + 0.5f;
	sampleX[0] = x + 0.25f;
	sampleY[0] = y + 0.25f;
	sampleX[1] = x + 0.75f;
	sampleY[1] = y + 0.25f;
	sampleX[2] = x + 0.25f;
	sampleY[2] = y + 0.75f;
	sampleX[3] = x + 0.75f;
	sampleY[3] = y + 0.75f;
	hitCount = 0;

	for (i = 0; i < 4; i++)
	{
		dx = sampleX[i] - cx;
		dy = sampleY[i] - cy;
		if (dx * dx + dy * dy <= radius * radius)
		{
			hitCount++;
		}
	}

	return hitCount * 0.25f;
}

typedef struct
{
	qboolean inLine;
	qboolean inDot;
	qboolean inLineOutlineRaw;
	int lineOutlineArmRingCount;
	qboolean inDotOutline;
} crosshairMaskParts_t;

static void Crosshair_MaskPartsAtPixel(int x, int y, crosshairMaskParts_t* o)
{
	int cx;
	int cy;
	int thickness;
	int length;
	int gap;
	int lineOutline;
	int dotOutline;
	int dotSize;
	int linesMode;
	qboolean drawDot;
	int half;
	int outlineHalf;
	int radius;
	int outlineRadiusOuterSq;
	int bodySq;
	int dx;
	int dy;
	int d2;
	qboolean lb;
	qboolean le;
	qboolean rb;
	qboolean re;
	qboolean db;
	qboolean de;
	qboolean ub;
	qboolean ue;

	cx = CROSSHAIR_TEXTURE_SIZE / 2;
	cy = CROSSHAIR_TEXTURE_SIZE / 2;
	thickness = (int)(s_crosshair.linesThickness.curvalue + 0.5f);
	length = (int)(s_crosshair.linesLength.curvalue + 0.5f);
	gap = (int)(s_crosshair.linesGap.curvalue + 0.5f);
	lineOutline = (int)(s_crosshair.outlineThickness.curvalue + 0.5f);
	dotOutline = (int)(s_crosshair.dotOutlineThickness.curvalue + 0.5f);
	dotSize = (int)(s_crosshair.dotSize.curvalue + 0.5f);
	drawDot = s_crosshair.dotEnable.curvalue ? qtrue : qfalse;
	linesMode = Crosshair_ClampInt(s_crosshair.linesType.curvalue, 0, 2);

	o->inLine = qfalse;
	o->inDot = qfalse;
	o->inLineOutlineRaw = qfalse;
	o->lineOutlineArmRingCount = 0;
	o->inDotOutline = qfalse;
	half = thickness / 2;
	outlineHalf = (thickness + lineOutline * 2) / 2;

	if (linesMode != 2)
	{
		if (Crosshair_PointInRect(x, y, cx - gap - length, cy - half, cx - gap, cy + half + 1) ||
		        Crosshair_PointInRect(x, y, cx + gap + 1, cy - half, cx + gap + length + 1, cy + half + 1) ||
		        Crosshair_PointInRect(x, y, cx - half, cy + gap + 1, cx + half + 1, cy + gap + length + 1) ||
		        (linesMode == 0 && Crosshair_PointInRect(x, y, cx - half, cy - gap - length, cx + half + 1, cy - gap)))
		{
			o->inLine = qtrue;
		}
	}

	if (lineOutline > 0 && linesMode != 2)
	{
		lb = Crosshair_PointInRect(x, y, cx - gap - length, cy - half, cx - gap, cy + half + 1);
		le = Crosshair_PointInRect(x, y, cx - gap - length - lineOutline, cy - outlineHalf, cx - gap + lineOutline, cy + outlineHalf + 1);
		if (le && !lb)
		{
			o->lineOutlineArmRingCount++;
		}

		rb = Crosshair_PointInRect(x, y, cx + gap + 1, cy - half, cx + gap + length + 1, cy + half + 1);
		re = Crosshair_PointInRect(x, y, cx + gap + 1 - lineOutline, cy - outlineHalf, cx + gap + length + 1 + lineOutline, cy + outlineHalf + 1);
		if (re && !rb)
		{
			o->lineOutlineArmRingCount++;
		}

		db = Crosshair_PointInRect(x, y, cx - half, cy + gap + 1, cx + half + 1, cy + gap + length + 1);
		de = Crosshair_PointInRect(x, y, cx - outlineHalf, cy + gap + 1 - lineOutline, cx + outlineHalf + 1, cy + gap + length + 1 + lineOutline);
		if (de && !db)
		{
			o->lineOutlineArmRingCount++;
		}

		if (linesMode == 0)
		{
			ub = Crosshair_PointInRect(x, y, cx - half, cy - gap - length, cx + half + 1, cy - gap);
			ue = Crosshair_PointInRect(x, y, cx - outlineHalf, cy - gap - length - lineOutline, cx + outlineHalf + 1, cy - gap + lineOutline);
			if (ue && !ub)
			{
				o->lineOutlineArmRingCount++;
			}
		}

		if (o->lineOutlineArmRingCount >= 1)
		{
			o->inLineOutlineRaw = qtrue;
		}
	}

	if (drawDot)
	{
		radius = dotSize / 2;
		dx = x - cx;
		dy = y - cy;
		d2 = dx * dx + dy * dy;
		bodySq = radius * radius;
		if (d2 <= bodySq)
		{
			o->inDot = qtrue;
		}

		if (dotOutline > 0)
		{
			outlineRadiusOuterSq = (radius + dotOutline) * (radius + dotOutline);
			if (d2 > bodySq && d2 <= outlineRadiusOuterSq)
			{
				o->inDotOutline = qtrue;
			}
		}
	}

	/* Arms don't meet at center texel when gap==0 — single line-colored pixel */
	if (gap == 0 && linesMode != 2)
	{
		if (x == cx && y == cy)
		{
			o->inLine = qtrue;
		}
	}
}

static void Crosshair_GetMask(int x, int y, qboolean* lineMask, qboolean* dotMask, qboolean* lineOutlineMask, qboolean* dotOutlineMask)
{
	crosshairMaskParts_t p;
	qboolean inLineOutline;

	Crosshair_MaskPartsAtPixel(x, y, &p);

	/* Union of arm outline rings (overlap drawn once in preview/TGA — no stacking). */
	inLineOutline = (p.lineOutlineArmRingCount >= 1) ? qtrue : qfalse;

	if (lineMask)
	{
		*lineMask = p.inLine;
	}
	if (dotMask)
	{
		*dotMask = p.inDot;
	}
	if (lineOutlineMask)
	{
		*lineOutlineMask = inLineOutline && !(p.inLine || p.inDot);
	}
	if (dotOutlineMask)
	{
		*dotOutlineMask = p.inDotOutline && !p.inDot;
	}
}

static void Crosshair_GetPixelRGBA(int x, int y, byte* rgbaOut)
{
	byte lineColor[4];
	byte dotColor[4];
	byte lineOutlineColor[4];
	byte dotOutlineColor[4];
	qboolean hasLine;
	qboolean hasDot;
	qboolean hasLineOutline;
	qboolean hasDotOutline;
	qboolean inLine;
	qboolean inDot;
	qboolean inLineOutline;
	qboolean inDotOutline;
	float dotCoverage;

	rgbaOut[0] = 0;
	rgbaOut[1] = 0;
	rgbaOut[2] = 0;
	rgbaOut[3] = 0;

	hasLine = qtrue;
	hasDot = qtrue;
	lineColor[0] = 255;
	lineColor[1] = 255;
	lineColor[2] = 255;
	lineColor[3] = (byte)Crosshair_ClampInt((int)s_crosshair.linesOpacity.curvalue, 0, 255);
	dotColor[0] = 255;
	dotColor[1] = 255;
	dotColor[2] = 255;
	dotColor[3] = (byte)Crosshair_ClampInt((int)s_crosshair.dotOpacity.curvalue, 0, 255);
	hasLineOutline = Crosshair_ParseHexColor(s_crosshair.colorOutlineField.field.buffer, lineOutlineColor, (int)s_crosshair.outlineOpacity.curvalue);
	hasDotOutline = Crosshair_ParseHexColor(s_crosshair.colorOutlineField.field.buffer, dotOutlineColor, (int)s_crosshair.dotOutlineOpacity.curvalue);
	Crosshair_GetMask(x, y, &inLine, &inDot, &inLineOutline, &inDotOutline);
	dotCoverage = Crosshair_GetDotCoverage(x, y, (int)(s_crosshair.dotSize.curvalue + 0.5f));
	if (s_crosshair.dotEnable.curvalue && dotCoverage > 0.0f)
	{
		inDot = qtrue;
		dotColor[3] = (byte)Crosshair_ClampInt((int)((float)dotColor[3] * dotCoverage + 0.5f), 0, 255);
	}

	if (inLineOutline && hasLineOutline)
	{
		rgbaOut[0] = lineOutlineColor[0];
		rgbaOut[1] = lineOutlineColor[1];
		rgbaOut[2] = lineOutlineColor[2];
		rgbaOut[3] = lineOutlineColor[3];
	}
	if (inDotOutline && hasDotOutline)
	{
		rgbaOut[0] = dotOutlineColor[0];
		rgbaOut[1] = dotOutlineColor[1];
		rgbaOut[2] = dotOutlineColor[2];
		rgbaOut[3] = dotOutlineColor[3];
	}
	if (inLine && hasLine)
	{
		rgbaOut[0] = lineColor[0];
		rgbaOut[1] = lineColor[1];
		rgbaOut[2] = lineColor[2];
		rgbaOut[3] = lineColor[3];
	}
	if (inDot && hasDot)
	{
		rgbaOut[0] = dotColor[0];
		rgbaOut[1] = dotColor[1];
		rgbaOut[2] = dotColor[2];
		rgbaOut[3] = dotColor[3];
	}
}

static qboolean Crosshair_PixelsEqual(const byte* a, const byte* b)
{
	return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]) ? qtrue : qfalse;
}

/*
=================
Crosshair preview (fast path): rects + frame outlines, no per-texel loop
=================
*/
static void Crosshair_PreviewByteToVec4(const byte b[4], vec4_t v)
{
	v[0] = b[0] / 255.0f;
	v[1] = b[1] / 255.0f;
	v[2] = b[2] / 255.0f;
	v[3] = b[3] / 255.0f;
}

static void Crosshair_PreviewTexRect(float ox, float oy, float sc, int tx, int ty, int tw, int th, const vec4_t c)
{
	float px, py, pw, ph;

	if (tw <= 0 || th <= 0 || c[3] <= 0.0f)
	{
		return;
	}
	px = ox + (float)tx * sc;
	py = oy + (float)ty * sc;
	pw = (float)tw * sc;
	ph = (float)th * sc;
	UI_FillRect(px, py, pw, ph, c);
}

static void Crosshair_PreviewTexPixel(float ox, float oy, float sc, int tx, int ty, const vec4_t c)
{
	Crosshair_PreviewTexRect(ox, oy, sc, tx, ty, 1, 1, c);
}

/* Non-overlapping frame: top, bottom, left, right strips */
static void Crosshair_PreviewFrame(float ox, float oy, float sc, int tx, int ty, int tw, int th, int o, const vec4_t c)
{
	int ox0;
	int oy0;
	int ow;
	int oh;

	if (o <= 0 || tw <= 0 || th <= 0 || c[3] <= 0.0f)
	{
		return;
	}
	ox0 = tx - o;
	oy0 = ty - o;
	ow = tw + 2 * o;
	oh = th + 2 * o;
	if (oh <= 2 * o)
	{
		return;
	}
	Crosshair_PreviewTexRect(ox, oy, sc, ox0, oy0, ow, o, c);
	Crosshair_PreviewTexRect(ox, oy, sc, ox0, oy0 + oh - o, ow, o, c);
	Crosshair_PreviewTexRect(ox, oy, sc, ox0, oy0 + o, o, oh - 2 * o, c);
	Crosshair_PreviewTexRect(ox, oy, sc, ox0 + ow - o, oy0 + o, o, oh - 2 * o, c);
}

static void Crosshair_PreviewLineArm(float ox, float oy, float sc, int il, int it, int iw, int ih, int o, qboolean wantOutline, const vec4_t lineC, const vec4_t outC)
{
	if (iw <= 0 || ih <= 0)
	{
		return;
	}
	if (wantOutline && o > 0)
	{
		Crosshair_PreviewFrame(ox, oy, sc, il, it, iw, ih, o, outC);
	}
	Crosshair_PreviewTexRect(ox, oy, sc, il, it, iw, ih, lineC);
}

static void Crosshair_PreviewDrawLines(
    float ox, float oy, float sc,
    int cx, int cy, int half, int length, int gap, int outline, int linesMode,
    const vec4_t lineC, const vec4_t outC, qboolean wantOutline
)
{
	int ih;

	if (linesMode == 2)
	{
		return;
	}
	ih = 2 * half + 1;
	Crosshair_PreviewLineArm(ox, oy, sc, cx - gap - length, cy - half, length, ih, outline, wantOutline, lineC, outC);
	Crosshair_PreviewLineArm(ox, oy, sc, cx + gap + 1, cy - half, length, ih, outline, wantOutline, lineC, outC);
	Crosshair_PreviewLineArm(ox, oy, sc, cx - half, cy + gap + 1, 2 * half + 1, length, outline, wantOutline, lineC, outC);
	if (linesMode == 0)
	{
		Crosshair_PreviewLineArm(ox, oy, sc, cx - half, cy - gap - length, 2 * half + 1, length, outline, wantOutline, lineC, outC);
	}
}

/*
Line outlines from Crosshair_GetMask (same pixels as TGA). Per-arm PreviewFrame would paint strips across other limbs.
*/
static void Crosshair_PreviewDrawLineOutlinesFromMask(
    float ox, float oy, float sc,
    int cx, int cy, int gap, int length, int lineOutline, int linesMode,
    const vec4_t lineOutV, qboolean wantLineOut
)
{
	int tx;
	int ty;
	int tx0;
	int ty0;
	int tx1;
	int ty1;
	qboolean inL;
	qboolean inD;
	qboolean lineOm;
	qboolean dotOm;

	if (!wantLineOut || lineOutline <= 0 || linesMode == 2 || lineOutV[3] <= 0.0f)
	{
		return;
	}

	tx0 = cx - gap - length - lineOutline;
	tx1 = cx + gap + length + lineOutline;
	ty0 = cy - gap - length - lineOutline;
	ty1 = cy + gap + length + lineOutline;
	if (tx0 < 0)
	{
		tx0 = 0;
	}
	if (ty0 < 0)
	{
		ty0 = 0;
	}
	if (tx1 > CROSSHAIR_TEXTURE_SIZE - 1)
	{
		tx1 = CROSSHAIR_TEXTURE_SIZE - 1;
	}
	if (ty1 > CROSSHAIR_TEXTURE_SIZE - 1)
	{
		ty1 = CROSSHAIR_TEXTURE_SIZE - 1;
	}

	for (ty = ty0; ty <= ty1; ty++)
	{
		for (tx = tx0; tx <= tx1; tx++)
		{
			Crosshair_GetMask(tx, ty, &inL, &inD, &lineOm, &dotOm);
			if (lineOm)
			{
				Crosshair_PreviewTexPixel(ox, oy, sc, tx, ty, lineOutV);
			}
		}
	}
}

static qboolean Crosshair_PreviewDotBodyHit(int px, int py, int x0, int y0, int dotSize, qboolean circularCut)
{
	if (px < x0 || px >= x0 + dotSize || py < y0 || py >= y0 + dotSize)
	{
		return qfalse;
	}
	if (!circularCut || dotSize < 3)
	{
		return qtrue;
	}
	if ((px == x0 || px == x0 + dotSize - 1) && (py == y0 || py == y0 + dotSize - 1))
	{
		return qfalse;
	}
	return qtrue;
}

/* Circular-style dot: square minus 4 corner pixels; outline = pixel ring outside body */
static void Crosshair_PreviewDrawDotCircular(
    float ox, float oy, float sc,
    int cx, int cy, int dotSize, int outline,
    const vec4_t dotC, const vec4_t outC, qboolean wantOutline
)
{
	int x0;
	int y0;
	int r;
	int ro;
	int px;
	int py;
	int r2;
	int ro2;
	int dx;
	int dy;
	int d2;

	if (dotSize <= 0)
	{
		return;
	}
	x0 = cx - dotSize / 2;
	y0 = cy - dotSize / 2;
	r = dotSize / 2;
	ro = r + outline;
	r2 = r * r;
	ro2 = ro * ro;

	if (dotSize >= 3)
	{
		Crosshair_PreviewTexRect(ox, oy, sc, x0, y0 + 1, dotSize, dotSize - 2, dotC);
		Crosshair_PreviewTexRect(ox, oy, sc, x0 + 1, y0, dotSize - 2, 1, dotC);
		Crosshair_PreviewTexRect(ox, oy, sc, x0 + 1, y0 + dotSize - 1, dotSize - 2, 1, dotC);
	}
	else
	{
		Crosshair_PreviewTexRect(ox, oy, sc, x0, y0, dotSize, dotSize, dotC);
	}

	if (wantOutline && outline > 0)
	{
		for (py = y0 - outline; py < y0 + dotSize + outline; py++)
		{
			for (px = x0 - outline; px < x0 + dotSize + outline; px++)
			{
				dx = px - cx;
				dy = py - cy;
				d2 = dx * dx + dy * dy;
				if (d2 > r2 && d2 <= ro2 && !Crosshair_PreviewDotBodyHit(px, py, x0, y0, dotSize, qtrue))
				{
					Crosshair_PreviewTexPixel(ox, oy, sc, px, py, outC);
				}
			}
		}
	}
}

/* Rectangular dot: 4-strip frame (no corner overlap), then fill */
static void Crosshair_PreviewDrawDotRectangular(
    float ox, float oy, float sc,
    int cx, int cy, int dotSize, int outline,
    const vec4_t dotC, const vec4_t outC, qboolean wantOutline
)
{
	int x0;
	int y0;

	if (dotSize <= 0)
	{
		return;
	}
	x0 = cx - dotSize / 2;
	y0 = cy - dotSize / 2;
	if (wantOutline && outline > 0)
	{
		Crosshair_PreviewFrame(ox, oy, sc, x0, y0, dotSize, dotSize, outline, outC);
	}
	Crosshair_PreviewTexRect(ox, oy, sc, x0, y0, dotSize, dotSize, dotC);
}

static void Crosshair_PreviewPaint(void)
{
	float sc;
	float ox;
	float oy;
	int cx;
	int cy;
	int thickness;
	int length;
	int gap;
	int lineOutline;
	int dotOutline;
	int dotSize;
	int linesMode;
	int half;
	byte lineB[4];
	byte dotB[4];
	byte lineOutB[4];
	byte dotOutB[4];
	vec4_t lineV;
	vec4_t dotV;
	vec4_t lineOutV;
	vec4_t dotOutV;
	qboolean wantLineOut;
	qboolean wantDotOut;
	qboolean dotCircular;

	cx = CROSSHAIR_TEXTURE_SIZE / 2;
	cy = CROSSHAIR_TEXTURE_SIZE / 2;
	thickness = (int)(s_crosshair.linesThickness.curvalue + 0.5f);
	length = (int)(s_crosshair.linesLength.curvalue + 0.5f);
	gap = (int)(s_crosshair.linesGap.curvalue + 0.5f);
	lineOutline = (int)(s_crosshair.outlineThickness.curvalue + 0.5f);
	dotOutline = (int)(s_crosshair.dotOutlineThickness.curvalue + 0.5f);
	dotSize = (int)(s_crosshair.dotSize.curvalue + 0.5f);
	linesMode = Crosshair_ClampInt(s_crosshair.linesType.curvalue, 0, 2);
	half = thickness / 2;

	sc = (float)PREVIEW_W / (float)CROSSHAIR_TEXTURE_SIZE;
	ox = (float)PREVIEW_X;
	oy = (float)PREVIEW_Y;

	lineB[0] = 255;
	lineB[1] = 255;
	lineB[2] = 255;
	lineB[3] = (byte)Crosshair_ClampInt((int)s_crosshair.linesOpacity.curvalue, 0, 255);
	dotB[0] = 255;
	dotB[1] = 255;
	dotB[2] = 255;
	dotB[3] = (byte)Crosshair_ClampInt((int)s_crosshair.dotOpacity.curvalue, 0, 255);
	Crosshair_PreviewByteToVec4(lineB, lineV);
	Crosshair_PreviewByteToVec4(dotB, dotV);

	wantLineOut = (lineOutline > 0 && Crosshair_ParseHexColor(s_crosshair.colorOutlineField.field.buffer, lineOutB, (int)s_crosshair.outlineOpacity.curvalue)) ? qtrue : qfalse;
	if (wantLineOut)
	{
		Crosshair_PreviewByteToVec4(lineOutB, lineOutV);
	}
	wantDotOut = (dotOutline > 0 && Crosshair_ParseHexColor(s_crosshair.colorOutlineField.field.buffer, dotOutB, (int)s_crosshair.dotOutlineOpacity.curvalue)) ? qtrue : qfalse;
	if (wantDotOut)
	{
		Crosshair_PreviewByteToVec4(dotOutB, dotOutV);
	}

	Crosshair_PreviewDrawLineOutlinesFromMask(ox, oy, sc, cx, cy, gap, length, lineOutline, linesMode, lineOutV, wantLineOut);
	Crosshair_PreviewDrawLines(ox, oy, sc, cx, cy, half, length, gap, lineOutline, linesMode, lineV, lineOutV, qfalse);
	if (gap == 0 && linesMode != 2 && lineV[3] > 0.0f)
	{
		Crosshair_PreviewTexPixel(ox, oy, sc, cx, cy, lineV);
	}

	if (!s_crosshair.dotEnable.curvalue)
	{
		return;
	}

	/* >=3: rounder preview (corners cut + ring); small dots: square */
	dotCircular = (dotSize >= 3) ? qtrue : qfalse;
	if (dotCircular)
	{
		Crosshair_PreviewDrawDotCircular(ox, oy, sc, cx, cy, dotSize, dotOutline, dotV, dotOutV, wantDotOut);
	}
	else
	{
		Crosshair_PreviewDrawDotRectangular(ox, oy, sc, cx, cy, dotSize, dotOutline, dotV, dotOutV, wantDotOut);
	}
}

static void Crosshair_ExportTgaRle(const char* filePath)
{
	fileHandle_t fileHandle;
	byte header[18];
	int width;
	int height;
	int totalPixels;
	int i;
	int rc;

	width = CROSSHAIR_TEXTURE_SIZE;
	height = CROSSHAIR_TEXTURE_SIZE;
	totalPixels = width * height;

	rc = trap_FS_FOpenFile(filePath, &fileHandle, FS_WRITE);
	if (rc < 0 || !fileHandle)
	{
		trap_Print(va(S_COLOR_RED "Unable to write crosshair image: " S_COLOR_WHITE "%s\n", filePath));
		return;
	}

	memset(header, 0, sizeof(header));
	header[2] = 10;
	header[12] = (byte)(width & 0xFF);
	header[13] = (byte)((width >> 8) & 0xFF);
	header[14] = (byte)(height & 0xFF);
	header[15] = (byte)((height >> 8) & 0xFF);
	header[16] = 32;
	header[17] = 8;
	trap_FS_Write(header, sizeof(header), fileHandle);

	i = 0;
	while (i < totalPixels)
	{
		int x;
		int y;
		byte pixel[4];
		int runLength;

		x = i % width;
		y = i / width;
		Crosshair_GetPixelRGBA(x, y, pixel);

		runLength = 1;
		while (runLength < 128 && i + runLength < totalPixels)
		{
			int nx;
			int ny;
			byte nextPixel[4];

			nx = (i + runLength) % width;
			ny = (i + runLength) / width;
			Crosshair_GetPixelRGBA(nx, ny, nextPixel);
			if (!Crosshair_PixelsEqual(pixel, nextPixel))
			{
				break;
			}
			runLength++;
		}

		if (runLength > 1)
		{
			byte packetHeader;
			byte bgra[4];

			packetHeader = (byte)(0x80 | (runLength - 1));
			bgra[0] = pixel[2];
			bgra[1] = pixel[1];
			bgra[2] = pixel[0];
			bgra[3] = pixel[3];
			trap_FS_Write(&packetHeader, 1, fileHandle);
			trap_FS_Write(bgra, 4, fileHandle);
			i += runLength;
		}
		else
		{
			int rawCount;
			byte packetHeader;
			int j;

			rawCount = 1;
			while (rawCount < 128 && i + rawCount < totalPixels)
			{
				int cx;
				int cy;
				int lookAheadRun;
				byte currentPixel[4];

				cx = (i + rawCount) % width;
				cy = (i + rawCount) / width;
				Crosshair_GetPixelRGBA(cx, cy, currentPixel);

				lookAheadRun = 1;
				while (lookAheadRun < 128 && (i + rawCount + lookAheadRun) < totalPixels)
				{
					int lx;
					int ly;
					byte lookAheadPixel[4];

					lx = (i + rawCount + lookAheadRun) % width;
					ly = (i + rawCount + lookAheadRun) / width;
					Crosshair_GetPixelRGBA(lx, ly, lookAheadPixel);
					if (!Crosshair_PixelsEqual(currentPixel, lookAheadPixel))
					{
						break;
					}
					lookAheadRun++;
				}

				if (lookAheadRun > 1)
				{
					break;
				}
				rawCount++;
			}

			packetHeader = (byte)(rawCount - 1);
			trap_FS_Write(&packetHeader, 1, fileHandle);
			for (j = 0; j < rawCount; j++)
			{
				int px;
				int py;
				byte rawPixel[4];
				byte bgra[4];

				px = (i + j) % width;
				py = (i + j) / width;
				Crosshair_GetPixelRGBA(px, py, rawPixel);
				bgra[0] = rawPixel[2];
				bgra[1] = rawPixel[1];
				bgra[2] = rawPixel[0];
				bgra[3] = rawPixel[3];
				trap_FS_Write(bgra, 4, fileHandle);
			}
			i += rawCount;
		}
	}

	trap_FS_FCloseFile(fileHandle);
}

static void Crosshair_ExportShader(int slot, const char* imageName)
{
	fileHandle_t fileHandle;
	char shaderPath[MAX_QPATH];
	char block[768];
	int len;
	int rc;

	Com_sprintf(shaderPath, sizeof(shaderPath), OSP2BE_PK3DIR "/scripts/crosshairs.shader");
	rc = trap_FS_FOpenFile(shaderPath, &fileHandle, FS_APPEND);
	if (rc < 0 || !fileHandle)
	{
		rc = trap_FS_FOpenFile(shaderPath, &fileHandle, FS_WRITE);
	}

	if (rc < 0 || !fileHandle)
	{
		trap_Print(va(S_COLOR_RED "Unable to write shader file: " S_COLOR_WHITE "%s\n", shaderPath));
		return;
	}

	Com_sprintf(
	    block,
	    sizeof(block),
	    "\n"
	    "gfx/2d/crosshair%d\n"
	    "{\n"
	    "    nomipmaps\n"
	    "    nopicmip\n"
	    "    {\n"
	    "       clampmap crosshair/%s\n"
	    "       blendFunc blend\n"
	    "       rgbGen vertex\n"
	    "    }\n"
	    "}\n"
	    "\n"
	    "gfx/2d/crosshair%d_r45\n"
	    "{\n"
	    "    nopicmip\n"
	    "    {\n"
	    "       clampmap crosshair/%s\n"
	    "       blendFunc blend\n"
	    "       tcMod transform 0.707 0.707 -0.707 0.707 0.5 -0.207\n"
	    "       rgbGen vertex\n"
	    "    }\n"
	    "}\n",
	    slot, imageName, slot, imageName
	);
	len = strlen(block);
	if (len > 0)
	{
		trap_FS_Write(block, len, fileHandle);
	}
	trap_FS_FCloseFile(fileHandle);
}

static void Crosshair_DrawPreview(void)
{
	int bgIndex;
	qhandle_t bgShader;
	float x;
	float y;
	float w;
	float h;

	x = PREVIEW_X;
	y = PREVIEW_Y;
	w = PREVIEW_W;
	h = PREVIEW_H;

	UI_DrawString(PREVIEW_X, PREVIEW_Y - 16, "PREVIEW", UI_SMALLFONT, uis.themeColor);
	UI_DrawRect(x, y, w, h, uis.themeColor);

	bgIndex = s_crosshair.bgImage.curvalue;
	bgShader = 0;
	if (bgIndex > 0 && bgIndex < 4)
	{
		bgShader = s_crosshair.bgShaders[bgIndex];
	}

	if (bgShader)
	{
		UI_DrawHandlePic(x + 1, y + 1, w - 2, h - 2, bgShader);
	}
	else
	{
		UI_FillRect(x + 1, y + 1, w - 2, h - 2, color_black);
	}

	Crosshair_PreviewPaint();
}

static void Crosshair_UpdateDraggedSlider(void)
{
	menuslider_s* slider;
	int x;
	float range;

	slider = s_crosshairDragSlider;
	if (!slider)
	{
		return;
	}

	x = uis.cursorx - slider->generic.x - 2 * SMALLCHAR_WIDTH;
	range = (x / (float)(SLIDER_RANGE * SMALLCHAR_WIDTH)) * (slider->maxvalue - slider->minvalue) + slider->minvalue;
	Crosshair_SetSliderValue(slider, (int)(range + 0.5f));

	if (slider == &s_crosshair.linesThickness)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.linesThicknessField, slider);
	}
	else if (slider == &s_crosshair.linesLength)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.linesLengthField, slider);
	}
	else if (slider == &s_crosshair.linesGap)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.linesGapField, slider);
	}
	else if (slider == &s_crosshair.linesOpacity)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.linesOpacityField, slider);
	}
	else if (slider == &s_crosshair.dotSize)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.dotSizeField, slider);
	}
	else if (slider == &s_crosshair.dotOpacity)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.dotOpacityField, slider);
	}
	else if (slider == &s_crosshair.dotOutlineThickness)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineThicknessField, slider);
	}
	else if (slider == &s_crosshair.dotOutlineOpacity)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineOpacityField, slider);
	}
	else if (slider == &s_crosshair.outlineThickness)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.outlineThicknessField, slider);
	}
	else if (slider == &s_crosshair.outlineOpacity)
	{
		Crosshair_SyncFieldFromSlider(&s_crosshair.outlineOpacityField, slider);
	}
}

static void Crosshair_MenuDraw(void)
{
	if (s_crosshairDragSlider)
	{
		if (trap_Key_IsDown(K_MOUSE1))
		{
			Crosshair_UpdateDraggedSlider();
		}
		else
		{
			s_crosshairDragSlider = NULL;
		}
	}

	Menu_Draw(&s_crosshair.menu);
	Crosshair_DrawPreview();
}

static void Crosshair_HandleFieldCommit(int id)
{
	switch (id)
	{
		case ID_LINES_THICKNESS_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.linesThickness, &s_crosshair.linesThicknessField);
			break;
		case ID_LINES_LENGTH_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.linesLength, &s_crosshair.linesLengthField);
			break;
		case ID_LINES_GAP_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.linesGap, &s_crosshair.linesGapField);
			break;
		case ID_LINES_OPACITY_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.linesOpacity, &s_crosshair.linesOpacityField);
			break;
		case ID_DOT_SIZE_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.dotSize, &s_crosshair.dotSizeField);
			break;
		case ID_DOT_OPACITY_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.dotOpacity, &s_crosshair.dotOpacityField);
			break;
		case ID_DOT_OUTLINE_THICKNESS_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.dotOutlineThickness, &s_crosshair.dotOutlineThicknessField);
			break;
		case ID_DOT_OUTLINE_OPACITY_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.dotOutlineOpacity, &s_crosshair.dotOutlineOpacityField);
			break;
		case ID_OUTLINE_THICKNESS_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.outlineThickness, &s_crosshair.outlineThicknessField);
			break;
		case ID_OUTLINE_OPACITY_FIELD:
			Crosshair_SyncSliderFromField(&s_crosshair.outlineOpacity, &s_crosshair.outlineOpacityField);
			break;
		default:
			break;
	}
}

static void Crosshair_ExportCurrent(void)
{
	char imagePath[MAX_QPATH];
	char imageName[32];
	int slot;

	slot = Crosshair_ClampInt(s_crosshair.shaderSlot.curvalue + 1, 1, 16);
	Com_sprintf(imageName, sizeof(imageName), "crosshair%02d.tga", slot);
	Com_sprintf(imagePath, sizeof(imagePath), OSP2BE_PK3DIR "/crosshair/%s", imageName);

	Crosshair_ExportTgaRle(imagePath);
	Crosshair_ExportShader(slot, imageName);
	trap_Print(va(S_COLOR_GREEN "Crosshair exported: " S_COLOR_WHITE "%s (" S_COLOR_YELLOW "gfx/2d/crosshair%d" S_COLOR_WHITE " + " S_COLOR_YELLOW "gfx/2d/crosshair%d_r45" S_COLOR_WHITE ")\n", imagePath, slot, slot));
}

static void Crosshair_MenuEvent(void* ptr, int event)
{
	menucommon_s* item;

	item = (menucommon_s*)ptr;
	if (!item)
	{
		return;
	}

	if (event == QM_ACTIVATED)
	{
		switch (item->id)
		{
			case ID_LINES_THICKNESS:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.linesThicknessField, &s_crosshair.linesThickness);
				break;
			case ID_LINES_LENGTH:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.linesLengthField, &s_crosshair.linesLength);
				break;
			case ID_LINES_GAP:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.linesGapField, &s_crosshair.linesGap);
				break;
			case ID_LINES_OPACITY:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.linesOpacityField, &s_crosshair.linesOpacity);
				break;
			case ID_DOT_SIZE:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.dotSizeField, &s_crosshair.dotSize);
				break;
			case ID_DOT_OPACITY:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.dotOpacityField, &s_crosshair.dotOpacity);
				break;
			case ID_DOT_OUTLINE_THICKNESS:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineThicknessField, &s_crosshair.dotOutlineThickness);
				break;
			case ID_DOT_OUTLINE_OPACITY:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineOpacityField, &s_crosshair.dotOutlineOpacity);
				break;
			case ID_OUTLINE_THICKNESS:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.outlineThicknessField, &s_crosshair.outlineThickness);
				break;
			case ID_OUTLINE_OPACITY:
				if (trap_Key_IsDown(K_MOUSE1))
				{
					s_crosshairDragSlider = Crosshair_GetSliderById(item->id);
				}
				Crosshair_SyncFieldFromSlider(&s_crosshair.outlineOpacityField, &s_crosshair.outlineOpacity);
				break;
			case ID_LINES_THICKNESS_FIELD:
			case ID_LINES_LENGTH_FIELD:
			case ID_LINES_GAP_FIELD:
			case ID_LINES_OPACITY_FIELD:
			case ID_DOT_SIZE_FIELD:
			case ID_DOT_OPACITY_FIELD:
			case ID_DOT_OUTLINE_THICKNESS_FIELD:
			case ID_DOT_OUTLINE_OPACITY_FIELD:
			case ID_OUTLINE_THICKNESS_FIELD:
			case ID_OUTLINE_OPACITY_FIELD:
				Crosshair_HandleFieldCommit(item->id);
				break;
			case ID_COLOR_MAIN:
				Q_strncpyz(s_crosshair.colorMainField.field.buffer, "FFFFFF", sizeof(s_crosshair.colorMainField.field.buffer));
				break;
			case ID_COLOR_OUTLINE:
				Crosshair_ToUpperInPlace(s_crosshair.colorOutlineField.field.buffer);
				break;
			case ID_EXPORT:
				Crosshair_ExportCurrent();
				break;
			case ID_BACK:
				UI_PopMenu();
				break;
			default:
				break;
		}
	}
	else if (event == QM_LOSTFOCUS)
	{
		Crosshair_HandleFieldCommit(item->id);
	}
}

static void Crosshair_InitSlider(menuslider_s* slider, int id, const char* name, int x, int y, int minValue, int maxValue, int value)
{
	int len;

	memset(slider, 0, sizeof(*slider));
	slider->generic.type = MTYPE_SLIDER;
	slider->generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_NODEFAULTINIT;
	slider->generic.x = x;
	slider->generic.y = y;
	slider->generic.id = id;
	slider->generic.name = name;
	slider->generic.callback = Crosshair_MenuEvent;
	slider->minvalue = minValue;
	slider->maxvalue = maxValue;
	Crosshair_SetSliderValue(slider, value);

	if (slider->generic.name)
	{
		len = strlen(slider->generic.name);
	}
	else
	{
		len = 0;
	}
	slider->generic.left = slider->generic.x - (len + 1) * SMALLCHAR_WIDTH;
	slider->generic.right = slider->generic.x + (SLIDER_RANGE + 2 + 1) * SMALLCHAR_WIDTH;
	slider->generic.top = slider->generic.y - 6;
	slider->generic.bottom = slider->generic.y + SMALLCHAR_HEIGHT + 6;
}

static void Crosshair_InitField(menufield_s* field, int id, int x, int y, int widthChars, int maxChars, qboolean numbersOnly)
{
	int w;
	int h;

	memset(field, 0, sizeof(*field));
	field->generic.type = MTYPE_FIELD;
	field->generic.flags = QMF_SMALLFONT | QMF_PULSEIFFOCUS | QMF_NODEFAULTINIT;
	if (numbersOnly)
	{
		field->generic.flags |= QMF_NUMBERSONLY;
	}
	field->generic.x = x;
	field->generic.y = y;
	field->generic.id = id;
	field->generic.callback = Crosshair_MenuEvent;
	field->field.widthInChars = widthChars;
	field->field.maxchars = maxChars;
	w = SMALLCHAR_WIDTH;
	h = SMALLCHAR_HEIGHT;
	field->generic.left = field->generic.x;
	field->generic.top = field->generic.y;
	field->generic.right = field->generic.x + w + field->field.widthInChars * w;
	field->generic.bottom = field->generic.y + h;
}

static void Crosshair_MenuInit(void)
{
	int xLeft;
	int y;
	int fieldX;

	memset(&s_crosshair, 0, sizeof(s_crosshair));

	s_crosshair.menu.wrapAround = qtrue;
	s_crosshair.menu.fullscreen = qtrue;
	s_crosshair.menu.draw = Crosshair_MenuDraw;

	s_crosshair.banner.generic.type = MTYPE_BTEXT;
	s_crosshair.banner.generic.x = 320;
	s_crosshair.banner.generic.y = 16;
	s_crosshair.banner.string = "CROSSHAIR EDITOR";
	s_crosshair.banner.color = uis.themeColor;
	s_crosshair.banner.style = UI_CENTER;

	s_crosshair.framel.generic.type = MTYPE_BITMAP;
	s_crosshair.framel.generic.name = ART_FRAMEL;
	s_crosshair.framel.generic.flags = QMF_INACTIVE;
	s_crosshair.framel.generic.x = FRAME_L_X;
	s_crosshair.framel.generic.y = FRAME_L_Y;
	s_crosshair.framel.width = FRAME_L_W;
	s_crosshair.framel.height = FRAME_L_H;

	s_crosshair.framer.generic.type = MTYPE_BITMAP;
	s_crosshair.framer.generic.name = ART_FRAMER;
	s_crosshair.framer.generic.flags = QMF_INACTIVE;
	s_crosshair.framer.generic.x = FRAME_R_X;
	s_crosshair.framer.generic.y = FRAME_R_Y;
	s_crosshair.framer.width = FRAME_R_W;
	s_crosshair.framer.height = FRAME_R_H;

	xLeft = 52;
	fieldX = 166;
	y = 84;

	Crosshair_InitSlider(&s_crosshair.linesThickness, ID_LINES_THICKNESS, "Line Thickness", xLeft, y, 1, 20, 2);
	Crosshair_InitField(&s_crosshair.linesThicknessField, ID_LINES_THICKNESS_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.linesThicknessField, &s_crosshair.linesThickness);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.linesLength, ID_LINES_LENGTH, "Line Length", xLeft, y, 2, 64, 16);
	Crosshair_InitField(&s_crosshair.linesLengthField, ID_LINES_LENGTH_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.linesLengthField, &s_crosshair.linesLength);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.linesGap, ID_LINES_GAP, "Line Gap", xLeft, y, 0, 48, 8);
	Crosshair_InitField(&s_crosshair.linesGapField, ID_LINES_GAP_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.linesGapField, &s_crosshair.linesGap);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.linesOpacity, ID_LINES_OPACITY, "Line Opacity", xLeft, y, 0, 255, 255);
	Crosshair_InitField(&s_crosshair.linesOpacityField, ID_LINES_OPACITY_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.linesOpacityField, &s_crosshair.linesOpacity);
	y += 24;

	s_crosshair.linesType.generic.type = MTYPE_SPINCONTROL;
	s_crosshair.linesType.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_crosshair.linesType.generic.name = "Lines type:";
	s_crosshair.linesType.generic.x = xLeft;
	s_crosshair.linesType.generic.y = y;
	s_crosshair.linesType.generic.id = ID_LINES_TYPE;
	s_crosshair.linesType.generic.callback = Crosshair_MenuEvent;
	s_crosshair.linesType.itemnames = crosshairLinesTypeNames;
	s_crosshair.linesType.curvalue = 0;
	y += 24;

	s_crosshair.dotEnable.generic.type = MTYPE_RADIOBUTTON;
	s_crosshair.dotEnable.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_crosshair.dotEnable.generic.name = "Enable Dot";
	s_crosshair.dotEnable.generic.x = xLeft;
	s_crosshair.dotEnable.generic.y = y;
	s_crosshair.dotEnable.generic.id = ID_DOT_ENABLE;
	s_crosshair.dotEnable.generic.callback = Crosshair_MenuEvent;
	s_crosshair.dotEnable.curvalue = 1;
	y += 24;

	Crosshair_InitSlider(&s_crosshair.dotSize, ID_DOT_SIZE, "Dot Size", xLeft, y, 1, 20, 4);
	Crosshair_InitField(&s_crosshair.dotSizeField, ID_DOT_SIZE_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.dotSizeField, &s_crosshair.dotSize);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.dotOpacity, ID_DOT_OPACITY, "Dot Opacity", xLeft, y, 0, 255, 255);
	Crosshair_InitField(&s_crosshair.dotOpacityField, ID_DOT_OPACITY_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.dotOpacityField, &s_crosshair.dotOpacity);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.dotOutlineThickness, ID_DOT_OUTLINE_THICKNESS, "Dot Outline Thick", xLeft, y, 0, 8, 1);
	Crosshair_InitField(&s_crosshair.dotOutlineThicknessField, ID_DOT_OUTLINE_THICKNESS_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineThicknessField, &s_crosshair.dotOutlineThickness);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.dotOutlineOpacity, ID_DOT_OUTLINE_OPACITY, "Dot Outline Opacity", xLeft, y, 0, 255, 200);
	Crosshair_InitField(&s_crosshair.dotOutlineOpacityField, ID_DOT_OUTLINE_OPACITY_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.dotOutlineOpacityField, &s_crosshair.dotOutlineOpacity);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.outlineThickness, ID_OUTLINE_THICKNESS, "Line Outline Thick", xLeft, y, 0, 8, 1);
	Crosshair_InitField(&s_crosshair.outlineThicknessField, ID_OUTLINE_THICKNESS_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.outlineThicknessField, &s_crosshair.outlineThickness);
	y += 24;

	Crosshair_InitSlider(&s_crosshair.outlineOpacity, ID_OUTLINE_OPACITY, "Line Outline Opacity", xLeft, y, 0, 255, 200);
	Crosshair_InitField(&s_crosshair.outlineOpacityField, ID_OUTLINE_OPACITY_FIELD, fieldX, y, 3, 3, qtrue);
	Crosshair_SyncFieldFromSlider(&s_crosshair.outlineOpacityField, &s_crosshair.outlineOpacity);
	y += 24;

	Crosshair_InitField(&s_crosshair.colorMainField, ID_COLOR_MAIN, xLeft + 126, y, 7, 6, qfalse);
	s_crosshair.colorMainField.generic.flags |= QMF_UPPERCASE | QMF_INACTIVE | QMF_GRAYED;
	s_crosshair.colorMainField.generic.name = "Main Color";
	Q_strncpyz(s_crosshair.colorMainField.field.buffer, "FFFFFF", sizeof(s_crosshair.colorMainField.field.buffer));
	s_crosshair.colorMainField.field.cursor = strlen(s_crosshair.colorMainField.field.buffer);
	y += 24;

	Crosshair_InitField(&s_crosshair.colorOutlineField, ID_COLOR_OUTLINE, xLeft + 126, y, 7, 6, qfalse);
	s_crosshair.colorOutlineField.generic.flags |= QMF_UPPERCASE;
	s_crosshair.colorOutlineField.generic.name = "Outline Color";
	Q_strncpyz(s_crosshair.colorOutlineField.field.buffer, "000000", sizeof(s_crosshair.colorOutlineField.field.buffer));
	s_crosshair.colorOutlineField.field.cursor = strlen(s_crosshair.colorOutlineField.field.buffer);
	y += 24;

	s_crosshair.bgImage.generic.type = MTYPE_SPINCONTROL;
	s_crosshair.bgImage.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_crosshair.bgImage.generic.name = "Preview bg";
	s_crosshair.bgImage.generic.x = PREVIEW_X;
	s_crosshair.bgImage.generic.y = PREVIEW_CONTROLS_Y;
	s_crosshair.bgImage.generic.id = ID_BG_IMAGE;
	s_crosshair.bgImage.generic.callback = Crosshair_MenuEvent;
	s_crosshair.bgImage.itemnames = crosshairBgNames;
	s_crosshair.bgImage.curvalue = 1;

	s_crosshair.shaderSlot.generic.type = MTYPE_SPINCONTROL;
	s_crosshair.shaderSlot.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_crosshair.shaderSlot.generic.name = "Shader slot";
	s_crosshair.shaderSlot.generic.x = PREVIEW_X;
	s_crosshair.shaderSlot.generic.y = PREVIEW_CONTROLS_Y + SMALLCHAR_HEIGHT + 6;
	s_crosshair.shaderSlot.generic.id = ID_SHADER_SLOT;
	s_crosshair.shaderSlot.generic.callback = Crosshair_MenuEvent;
	s_crosshair.shaderSlot.itemnames = crosshairSlotNames;
	s_crosshair.shaderSlot.curvalue = 0;

	s_crosshair.exportButton.generic.type = MTYPE_PTEXT;
	s_crosshair.exportButton.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_crosshair.exportButton.generic.x = 176;
	s_crosshair.exportButton.generic.y = y;
	s_crosshair.exportButton.generic.id = ID_EXPORT;
	s_crosshair.exportButton.generic.callback = Crosshair_MenuEvent;
	s_crosshair.exportButton.string = "EXPORT TGA + SHADER";
	s_crosshair.exportButton.color = uis.themeColor;
	s_crosshair.exportButton.style = UI_CENTER | UI_SMALLFONT;

	s_crosshair.back.generic.type = MTYPE_BITMAP;
	s_crosshair.back.generic.name = ART_BACK0;
	s_crosshair.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_crosshair.back.generic.id = ID_BACK;
	s_crosshair.back.generic.callback = Crosshair_MenuEvent;
	s_crosshair.back.generic.x = 0;
	s_crosshair.back.generic.y = 480 - 64;
	s_crosshair.back.width = 128;
	s_crosshair.back.height = 64;
	s_crosshair.back.focuspic = ART_BACK1;

	s_crosshair.bgShaders[1] = trap_R_RegisterShaderNoMip("levelshots/q3dm1");
	s_crosshair.bgShaders[2] = trap_R_RegisterShaderNoMip("levelshots/q3dm6");
	s_crosshair.bgShaders[3] = trap_R_RegisterShaderNoMip("levelshots/q3tourney2");

	Menu_AddItem(&s_crosshair.menu, &s_crosshair.banner);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.framel);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.framer);

	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesThickness);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesThicknessField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesLength);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesLengthField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesGap);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesGapField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesOpacity);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesOpacityField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.linesType);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotEnable);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotSize);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotSizeField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOpacity);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOpacityField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOutlineThickness);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOutlineThicknessField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOutlineOpacity);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.dotOutlineOpacityField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.outlineThickness);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.outlineThicknessField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.outlineOpacity);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.outlineOpacityField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.colorMainField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.colorOutlineField);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.bgImage);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.shaderSlot);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.exportButton);
	Menu_AddItem(&s_crosshair.menu, &s_crosshair.back);
}

void UI_CrosshairMenu(void)
{
	trap_R_RegisterShaderNoMip(ART_FRAMEL);
	trap_R_RegisterShaderNoMip(ART_FRAMER);
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);
	Crosshair_MenuInit();
	UI_PushMenu(&s_crosshair.menu);
}
