/*
 * UI_ParseColorStr - color parser for UI (hex, name, digit)
 * Compatible with cgame CG_ParseColorStr format.
 */
#include "ui_local.h"

#define UI_COLORPARSE_MAXLEN 32

typedef struct
{
	const char* name;
	vec4_t color;
} ui_colorName_t;

static const ui_colorName_t ui_colorNames[] =
{
	{ "black",   { 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "red",     { 1.0f, 0.0f, 0.0f, 1.0f } },
	{ "yellow",  { 1.0f, 1.0f, 0.0f, 1.0f } },
	{ "blue",    { 0.0f, 0.0f, 1.0f, 1.0f } },
	{ "green",   { 0.0f, 1.0f, 0.0f, 1.0f } },
	{ "cyan",    { 0.0f, 1.0f, 1.0f, 1.0f } },
	{ "magenta", { 1.0f, 0.0f, 1.0f, 1.0f } },
	{ "white",   { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ "grey",    { 0.5f, 0.5f, 0.5f, 1.0f } },
	{ "gray",    { 0.5f, 0.5f, 0.5f, 1.0f } },
	{ "orange",  { 1.0f, 0.43f, 0.0f, 1.0f } },
	{ "pink",    { 1.0f, 0.75f, 0.8f, 1.0f } },
	{ NULL,      { 0.0f, 0.0f, 0.0f, 1.0f } }
};

static int UI_CharFromHex4bit(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
	if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
	return 0;
}

static float UI_FloatFromHex4bit(char c)
{
	return (1.0f / 15.0f) * (float)UI_CharFromHex4bit(c);
}

static float UI_FloatFromHex8bit(char hi, char lo)
{
	int v = UI_CharFromHex4bit(hi) * 16 + UI_CharFromHex4bit(lo);
	return (1.0f / 255.0f) * (float)v;
}

static void UI_ParseHex12Bit(const char* str, vec4_t out)
{
	char hex[4] = { '0', '0', '0', 0 };
	int len = (int)strlen(str);
	if (len > 3) len = 3;
	if (len > 0)
	{
		Q_strncpyz(&hex[3 - len], str, len + 1);
	}
	out[0] = UI_FloatFromHex4bit(hex[0]);
	out[1] = UI_FloatFromHex4bit(hex[1]);
	out[2] = UI_FloatFromHex4bit(hex[2]);
	out[3] = 1.0f;
}

static void UI_ParseHex24Bit(const char* str, vec4_t out)
{
	char hex[7] = { '0', '0', '0', '0', '0', '0', 0 };
	int len = (int)strlen(str);
	if (len > 6) len = 6;
	if (len > 0)
	{
		Q_strncpyz(&hex[6 - len], str, len + 1);
	}
	out[0] = UI_FloatFromHex8bit(hex[0], hex[1]);
	out[1] = UI_FloatFromHex8bit(hex[2], hex[3]);
	out[2] = UI_FloatFromHex8bit(hex[4], hex[5]);
	out[3] = 1.0f;
}

static void UI_ColorParse_OSPColorFromNumber(int number, vec4_t out)
{
	float red;
	float green;
	float blue;
	double tmp;
	double sixtyRadians;

	red = 1.0f;
	green = 1.0f;
	blue = 1.0f;

	if (number >= 10 && number < 33)
	{
		tmp = ((360.0 * (double)number) - 3600.0) / 24.0;
		sixtyRadians = 60.0 * M_PI / 180.0;
		if (tmp <= 120.0)
		{
			tmp = M_PI * tmp / 180.0;
			blue = 0.0f;
			red = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			green = 1.0f - (red + blue);
		}
		else if (tmp <= 240.0)
		{
			tmp = M_PI * (tmp - 120.0) / 180.0;
			red = 0.0f;
			green = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			blue = 1.0f - (red + green);
		}
		else
		{
			tmp = M_PI * (tmp - 240.0) / 180.0;
			green = 0.0f;
			blue = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			red = 1.0f - (green + blue);
		}
	}
	else
	{
		switch (number)
		{
			case 0:
				red = 1.0f;
				green = 1.0f;
				blue = 1.0f;
				break;
			case 1:
				red = 1.0f;
				green = 0.2f;
				blue = 0.2f;
				break;
			case 2:
				red = 0.2f;
				green = 1.0f;
				blue = 0.2f;
				break;
			case 3:
				red = 1.0f;
				green = 1.0f;
				blue = 0.1f;
				break;
			case 4:
				red = 0.2f;
				green = 0.2f;
				blue = 1.0f;
				break;
			case 5:
				red = 0.1f;
				green = 1.0f;
				blue = 1.0f;
				break;
			case 6:
				red = 1.0f;
				green = 0.1f;
				blue = 1.0f;
				break;
			case 7:
				red = 1.0f;
				green = 0.59f;
				blue = 0.12f;
				break;
			case 8:
				red = 0.65f;
				green = 0.2f;
				blue = 0.65f;
				break;
			case 9:
				red = 0.6f;
				green = 0.6f;
				blue = 1.0f;
				break;
			case 33: /* x — bright magenta (Quake3e) */
				red = 1.0f;
				green = 0.0f;
				blue = 0.26795f;
				break;
			case 34: /* y — white */
				red = 1.0f;
				green = 1.0f;
				blue = 1.0f;
				break;
			case 35: /* z — grey */
				red = 0.5f;
				green = 0.5f;
				blue = 0.5f;
				break;
			default:
				red = 1.0f;
				green = 1.0f;
				blue = 1.0f;
				break;
		}
	}

	out[0] = red;
	out[1] = green;
	out[2] = blue;
	out[3] = 1.0f;
}

qboolean Osp2Be_ColorFromChar(char code, vec4_t out)
{
	int number;

	if (!out)
	{
		return qfalse;
	}

	if (code >= '0' && code <= '9')
	{
		number = code - '0';
		UI_ColorParse_OSPColorFromNumber(number, out);
		return qtrue;
	}

	if (code >= 'a' && code <= 'z')
	{
		number = code - 'a' + 10;
		UI_ColorParse_OSPColorFromNumber(number, out);
		return qtrue;
	}

	if (code >= 'A' && code <= 'Z')
	{
		number = code - 'A' + 10;
		UI_ColorParse_OSPColorFromNumber(number, out);
		return qtrue;
	}

	return qfalse;
}

qboolean UI_ParseColorStr(const char* str, vec4_t out)
{
	char buf[UI_COLORPARSE_MAXLEN];
	int len;
	const char* ptr;
	int i;

	if (!str || !out) return qfalse;

	len = (int)strlen(str);
	if (len <= 0 || len >= UI_COLORPARSE_MAXLEN) return qfalse;

	for (i = 0; i < len && i < UI_COLORPARSE_MAXLEN - 1 && str[i]; i++)
	{
		buf[i] = (str[i] >= 'A' && str[i] <= 'Z') ? str[i] + 32 : str[i];
	}
	buf[i] = '\0';

	/* Single digit 0-9: Quake color table (0-7 base, 8=orange 9=grey in qcommon) */
	if (len == 1 && buf[0] >= '0' && buf[0] <= '9')
	{
		int idx = buf[0] - '0';
		VectorCopy(g_color_table[idx], out);
		out[3] = 1.0f;
		return qtrue;
	}

	/* Color name */
	if (buf[0] >= 'a' && buf[0] <= 'z')
	{
		const ui_colorName_t* t = &ui_colorNames[0];
		while (t->name)
		{
			if (Q_stricmp(buf, t->name) == 0)
			{
				Vector4Copy(t->color, out);
				return qtrue;
			}
			t++;
		}
	}

	/* One-letter OSP extended palette (a-z); must run before hex (g-z are not hex digits) */
	if (len == 1 && buf[0] >= 'a' && buf[0] <= 'z')
	{
		if (Osp2Be_ColorFromChar(buf[0], out))
		{
			return qtrue;
		}
	}

	/* Hex: strip prefix #, 0x, x */
	ptr = buf;
	if (*ptr == '#') ptr++;
	else if (ptr[0] == '0' && ptr[1] == 'x') ptr += 2;
	else if (*ptr == 'x') ptr++;

	/* Validate hex chars */
	for (i = 0; ptr[i]; i++)
	{
		char c = ptr[i];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return qfalse;
	}

	len = (int)strlen(ptr);
	if (len == 0)
	{
		out[0] = out[1] = out[2] = 0.0f;
		out[3] = 1.0f;
		return qtrue;
	}
	if (len >= 1 && len <= 3)
	{
		UI_ParseHex12Bit(ptr, out);
		return qtrue;
	}
	if (len >= 4 && len <= 6)
	{
		UI_ParseHex24Bit(ptr, out);
		return qtrue;
	}
	return qfalse;
}
