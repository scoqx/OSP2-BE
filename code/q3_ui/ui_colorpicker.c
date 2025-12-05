#include "ui_colorpicker.h"

extern double cos(double);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define COLORPICKER_PALETTE_COLS 10
#define COLORPICKER_PALETTE_ROWS 8
#define COLORPICKER_PALETTE_CELLS (COLORPICKER_PALETTE_COLS * COLORPICKER_PALETTE_ROWS)
#define COLORPICKER_LIGHTEN_COUNT 2
#define COLORPICKER_DARKEN_COUNT 4
#define COLORPICKER_CELL_SIZE 30
#define COLORPICKER_PADDING 8

#define COLORPICKER_MODE_RGB 0
#define COLORPICKER_MODE_PALETTE 1

#define COLORPICKER_PANEL_WIDTH 456
#define COLORPICKER_PANEL_HEIGHT 360
#define COLORPICKER_PANEL_PADDING 16

#define COLORPICKER_MODE_BUTTON_WIDTH 96
#define COLORPICKER_MODE_BUTTON_HEIGHT 24
#define COLORPICKER_MODE_BUTTON_GAP 12

#define COLORPICKER_RGB_SLIDER_WIDTH 256
#define COLORPICKER_RGB_SLIDER_HEIGHT 16
#define COLORPICKER_RGB_ROW_HEIGHT 48
#define COLORPICKER_RGB_LABEL_OFFSET 28
#define COLORPICKER_RGB_FIELD_WIDTH 48
#define COLORPICKER_RGB_FIELD_HEIGHT 20
#define COLORPICKER_RGB_FIELD_PADDING 4
#define COLORPICKER_RGB_PREVIEW_SIZE 64
#define COLORPICKER_HEX_FIELD_WIDTH 72
#define COLORPICKER_HEX_FIELD_HEIGHT 20
#define COLORPICKER_HEX_FIELD_PADDING 4
#define COLORPICKER_COPY_BUTTON_SIZE 20
#define COLORPICKER_PASTE_BUTTON_WIDTH 48
#define COLORPICKER_PASTE_BUTTON_HEIGHT 20
#define COLORPICKER_COPY_PASTE_GAP 4
#define COLORPICKER_LAST_USED_COLS 3
#define COLORPICKER_LAST_USED_ROWS 3
#define COLORPICKER_LAST_USED_COUNT (COLORPICKER_LAST_USED_COLS * COLORPICKER_LAST_USED_ROWS)
#define COLORPICKER_COPIED_COLS 3
#define COLORPICKER_COPIED_ROWS 2
#define COLORPICKER_COPIED_COUNT (COLORPICKER_COPIED_COLS * COLORPICKER_COPIED_ROWS)
#define COLORPICKER_SAVED_COLOR_SIZE 16
#define COLORPICKER_SAVED_COLORS_GAP 4
#define COLORPICKER_SECTION_HEADER_HEIGHT 16

/* Field indices: 0-2 = RGB components, 3 = Hex field */
#define COLORPICKER_FIELD_HEX 3

#define COLORPICKER_BUTTON_WIDTH 96
#define COLORPICKER_BUTTON_HEIGHT 24
#define COLORPICKER_BUTTON_GAP 12

typedef struct {
	const char *name;
	float r;
	float g;
	float b;
} uiColorPickerNamedColor_t;

static const uiColorPickerNamedColor_t uiColorPickerNamedColors[] = {
	{ "white", 1.0f, 1.0f, 1.0f },
	{ "black", 0.0f, 0.0f, 0.0f },
	{ "red", 1.0f, 0.0f, 0.0f },
	{ "green", 0.0f, 1.0f, 0.0f },
	{ "blue", 0.0f, 0.0f, 1.0f },
	{ "yellow", 1.0f, 1.0f, 0.0f },
	{ "magenta", 1.0f, 0.0f, 1.0f },
	{ "cyan", 0.0f, 1.0f, 1.0f },
	{ "orange", 1.0f, 0.5f, 0.0f },
	{ "purple", 0.5f, 0.0f, 0.5f },
	{ "pink", 1.0f, 0.4f, 0.7f },
	{ "teal", 0.0f, 0.5f, 0.5f },
	{ "grey", 0.5f, 0.5f, 0.5f },
	{ "gray", 0.5f, 0.5f, 0.5f },
	{ NULL, 0.0f, 0.0f, 0.0f }
};

static char uiColorPickerClipboardBuffer[8] = "";
static vec4_t uiColorPickerLastUsedColors[COLORPICKER_LAST_USED_COUNT];
static qboolean uiColorPickerLastUsedColorsUsed[COLORPICKER_LAST_USED_COUNT];
static vec4_t uiColorPickerCopiedColors[COLORPICKER_COPIED_COUNT];
static qboolean uiColorPickerCopiedColorsUsed[COLORPICKER_COPIED_COUNT];

static void UI_ColorPicker_AddToLastUsed( const vec4_t color ) {
	int i;
	int j;
	int foundIndex;
	float epsilon;
	qboolean found;
	float diff;

	if ( !color ) {
		return;
	}

	epsilon = 0.001f;
	foundIndex = -1;
	found = qfalse;

	if ( uiColorPickerLastUsedColorsUsed[0] ) {
		diff = ( color[0] - uiColorPickerLastUsedColors[0][0] ) * ( color[0] - uiColorPickerLastUsedColors[0][0] ) +
		       ( color[1] - uiColorPickerLastUsedColors[0][1] ) * ( color[1] - uiColorPickerLastUsedColors[0][1] ) +
		       ( color[2] - uiColorPickerLastUsedColors[0][2] ) * ( color[2] - uiColorPickerLastUsedColors[0][2] );
		if ( diff < epsilon * epsilon ) {
			return;
		}
	}

	for ( i = 0; i < COLORPICKER_LAST_USED_COUNT; i++ ) {
		if ( uiColorPickerLastUsedColorsUsed[i] ) {
			diff = ( color[0] - uiColorPickerLastUsedColors[i][0] ) * ( color[0] - uiColorPickerLastUsedColors[i][0] ) +
			       ( color[1] - uiColorPickerLastUsedColors[i][1] ) * ( color[1] - uiColorPickerLastUsedColors[i][1] ) +
			       ( color[2] - uiColorPickerLastUsedColors[i][2] ) * ( color[2] - uiColorPickerLastUsedColors[i][2] );
			if ( diff < epsilon * epsilon ) {
				foundIndex = i;
				found = qtrue;
				break;
			}
		}
	}

	if ( found && foundIndex >= 0 ) {
		vec4_t tempColor;
		qboolean tempUsed;

		tempColor[0] = uiColorPickerLastUsedColors[foundIndex][0];
		tempColor[1] = uiColorPickerLastUsedColors[foundIndex][1];
		tempColor[2] = uiColorPickerLastUsedColors[foundIndex][2];
		tempColor[3] = uiColorPickerLastUsedColors[foundIndex][3];
		tempUsed = uiColorPickerLastUsedColorsUsed[foundIndex];

		for ( j = foundIndex; j > 0; j-- ) {
			uiColorPickerLastUsedColors[j][0] = uiColorPickerLastUsedColors[j - 1][0];
			uiColorPickerLastUsedColors[j][1] = uiColorPickerLastUsedColors[j - 1][1];
			uiColorPickerLastUsedColors[j][2] = uiColorPickerLastUsedColors[j - 1][2];
			uiColorPickerLastUsedColors[j][3] = uiColorPickerLastUsedColors[j - 1][3];
			uiColorPickerLastUsedColorsUsed[j] = uiColorPickerLastUsedColorsUsed[j - 1];
		}

		uiColorPickerLastUsedColors[0][0] = tempColor[0];
		uiColorPickerLastUsedColors[0][1] = tempColor[1];
		uiColorPickerLastUsedColors[0][2] = tempColor[2];
		uiColorPickerLastUsedColors[0][3] = tempColor[3];
		uiColorPickerLastUsedColorsUsed[0] = tempUsed;
	} else {
		for ( i = COLORPICKER_LAST_USED_COUNT - 1; i > 0; i-- ) {
			uiColorPickerLastUsedColors[i][0] = uiColorPickerLastUsedColors[i - 1][0];
			uiColorPickerLastUsedColors[i][1] = uiColorPickerLastUsedColors[i - 1][1];
			uiColorPickerLastUsedColors[i][2] = uiColorPickerLastUsedColors[i - 1][2];
			uiColorPickerLastUsedColors[i][3] = uiColorPickerLastUsedColors[i - 1][3];
			uiColorPickerLastUsedColorsUsed[i] = uiColorPickerLastUsedColorsUsed[i - 1];
		}

		uiColorPickerLastUsedColors[0][0] = color[0];
		uiColorPickerLastUsedColors[0][1] = color[1];
		uiColorPickerLastUsedColors[0][2] = color[2];
		uiColorPickerLastUsedColors[0][3] = color[3];
		uiColorPickerLastUsedColorsUsed[0] = qtrue;
	}
}

static qboolean UI_ColorPicker_ParseNamedColor(const char *value, vec4_t out) {
	char lower[32];
	int i;
	int index;
	char c;

	if (!value) {
		return qfalse;
	}

	for (i = 0; value[i] && i < (int)sizeof(lower) - 1; i++) {
		c = value[i];
		if (c >= 'A' && c <= 'Z') {
			c = (char)(c - 'A' + 'a');
		}
		lower[i] = c;
	}
	lower[i] = '\0';

	if (i <= 0) {
		return qfalse;
	}

	index = 0;
	while (uiColorPickerNamedColors[index].name) {
		const char *name;
		const char *test;

		name = uiColorPickerNamedColors[index].name;
		test = lower;

		while (*name && *test && *name == *test) {
			name++;
			test++;
		}

		if (!*name && !*test) {
			out[0] = uiColorPickerNamedColors[index].r;
			out[1] = uiColorPickerNamedColors[index].g;
			out[2] = uiColorPickerNamedColors[index].b;
			out[3] = 1.0f;
			return qtrue;
		}

		index++;
	}

	return qfalse;
}

static void UI_ColorPicker_UpdateHexString(uiColorPicker_t *picker, int red, int green, int blue);
static void UI_ColorPicker_UpdateFromCurrent(uiColorPicker_t *picker);
static void UI_ColorPicker_ResetInputState(uiColorPicker_t *picker);
static void UI_ColorPicker_SetMode(uiColorPicker_t *picker, int mode);
static void UI_ColorPicker_SetComponentFromValue(uiColorPicker_t *picker, int componentIndex, int value);
static int UI_ColorPicker_ParseComponentText(const char *text);
static int UI_ColorPicker_FindPaletteIndex(const vec4_t color);
static void UI_ColorPicker_DrawModeTabs(uiColorPicker_t *picker, int tabY);
static void UI_ColorPicker_DrawButtons(uiColorPicker_t *picker, int buttonY);
static void UI_ColorPicker_DrawPreview(uiColorPicker_t *picker, int previewX, int previewY, int labelY);
static void UI_ColorPicker_DrawRGB(uiColorPicker_t *picker, int contentTop);
static void UI_ColorPicker_DrawPalette(uiColorPicker_t *picker, int contentTop);
static void UI_ColorPicker_Commit(uiColorPicker_t *picker);
static void UI_ColorPicker_UpdateModeCvar(int mode);

static int UI_ColorPicker_HexDigit(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return -1;
}

static float UI_ColorPicker_ParseHexComponent(const char *ptr) {
	int high;
	int low;

	high = UI_ColorPicker_HexDigit(ptr[0]);
	if (high < 0) {
		return -1.0f;
	}

	low = UI_ColorPicker_HexDigit(ptr[1]);
	if (low < 0) {
		return -1.0f;
	}

	return (float)(high * 16 + low) / 255.0f;
}

static qboolean UI_ColorPicker_ParseHexColor(const char *value, vec4_t out) {
	float component;
	int len;
	int i;

	if (!value) {
		return qfalse;
	}

	len = 0;
	while (value[len]) {
		len++;
	}

	if (len == 3) {
		/* short form: RGB -> RRGGBB */
		for (i = 0; i < 3; i++) {
			int digit;
			digit = UI_ColorPicker_HexDigit(value[i]);
			if (digit < 0) {
				return qfalse;
			}
			out[i] = (float)(digit * 17) / 255.0f;
		}
		out[3] = 1.0f;
		return qtrue;
	}

	if (len < 6) {
		return qfalse;
	}

	component = UI_ColorPicker_ParseHexComponent(value);
	if (component < 0.0f) {
		return qfalse;
	}
	out[0] = component;

	component = UI_ColorPicker_ParseHexComponent(value + 2);
	if (component < 0.0f) {
		return qfalse;
	}
	out[1] = component;

	component = UI_ColorPicker_ParseHexComponent(value + 4);
	if (component < 0.0f) {
		return qfalse;
	}
	out[2] = component;
	out[3] = 1.0f;
	return qtrue;
}

static void UI_ColorPicker_OSPColorFromNumber(int number, vec4_t out) {
	float red;
	float green;
	float blue;
	double tmp;
	double sixtyRadians;

	red = 1.0f;
	green = 1.0f;
	blue = 1.0f;

	if (number >= 10 && number < 34) {
		tmp = ((360.0 * (double)number) - 3600.0) / 24.0;
		sixtyRadians = 60.0 * M_PI / 180.0;
		if (tmp <= 120.0) {
			tmp = M_PI * tmp / 180.0;
			blue = 0.0f;
			red = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			green = 1.0f - (red + blue);
		} else if (tmp <= 240.0) {
			tmp = M_PI * (tmp - 120.0) / 180.0;
			red = 0.0f;
			green = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			blue = 1.0f - (red + green);
		} else {
			tmp = M_PI * (tmp - 240.0) / 180.0;
			green = 0.0f;
			blue = (float)((cos(tmp) / (cos(sixtyRadians - tmp)) + 1.0) / 3.0);
			red = 1.0f - (green + blue);
		}
	} else {
		switch (number) {
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

static qboolean UI_ColorPicker_ColorFromChar(char code, vec4_t out) {
	int number;

	if (code >= '0' && code <= '9') {
		number = code - '0';
		UI_ColorPicker_OSPColorFromNumber(number, out);
		return qtrue;
	}

	if (code >= 'a' && code <= 'z') {
		number = code - 'a' + 10;
		UI_ColorPicker_OSPColorFromNumber(number, out);
		return qtrue;
	}

	if (code >= 'A' && code <= 'Z') {
		number = code - 'A' + 10;
		UI_ColorPicker_OSPColorFromNumber(number, out);
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ColorPicker_ParseColorValue(const char *value, vec4_t out) {
	char clean[32];
	int len;
	int i;
	const char *ptr;

	if (!value) {
		return qfalse;
	}

	while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
		value++;
	}

	ptr = value;
	len = 0;
	while (*ptr && len < (int)sizeof(clean) - 1) {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
			break;
		}
		clean[len] = *ptr;
		len++;
		ptr++;
	}
	clean[len] = '\0';

	if (len <= 0) {
		return qfalse;
	}

	if (clean[0] == '^' && len >= 2) {
		return UI_ColorPicker_ColorFromChar(clean[1], out);
	}

	if (len == 1) {
		if (UI_ColorPicker_ColorFromChar(clean[0], out)) {
			return qtrue;
		}
	}

	if (clean[0] == '#') {
		for (i = 0; i < len; i++) {
			clean[i] = clean[i + 1];
		}
		len--;
	}

	if (len <= 0) {
		return qfalse;
	}

	if (UI_ColorPicker_ParseNamedColor(clean, out)) {
		return qtrue;
	}

	if (len == 3) {
		return UI_ColorPicker_ParseHexColor(clean, out);
	}

	if (len >= 6) {
		if (len > 6) {
			clean[6] = '\0';
		}
		return UI_ColorPicker_ParseHexColor(clean, out);
	}

	return qfalse;
}

static void UI_ColorPicker_GeneratePalette(vec4_t *palette, int *count) {
	static const float baseColors[COLORPICKER_PALETTE_COLS][3] = {
		{ 1.0f, 0.0f, 0.0f }, /* ^1 red */
		{ 1.0f, 0.5f, 0.0f }, /* ^2 orange */
		{ 1.0f, 1.0f, 0.0f }, /* ^3 yellow */
		{ 0.0f, 1.0f, 0.0f }, /* ^4 green */
		{ 0.0f, 1.0f, 1.0f }, /* ^5 cyan */
		{ 0.0f, 0.0f, 1.0f }, /* ^6 blue */
		{ 1.0f, 0.0f, 1.0f }, /* ^7 magenta */
		{ 1.0f, 0.3f, 0.6f }, /* ^8 pink */
		{ 0.6f, 0.3f, 0.1f }, /* ^9 brown */
		{ 1.0f, 1.0f, 1.0f }  /* ^7 white (index 7 in palette) */
	};
	static const int baseRowIndex = 0;
	static const float lightenFactors[COLORPICKER_LIGHTEN_COUNT] = { 0.25f, 0.50f };
	static const float darkenFactors[COLORPICKER_DARKEN_COUNT] = { 0.20f, 0.40f, 0.60f, 0.80f };
	int i;
	int row;
	int col;
	float r;
	float g;
	float b;
	const float *base;
	int colorRowIndex;
	int factorIndex;
	float factor;

	*count = 0;

	/* First row: grayscale */
	for (i = 0; i < COLORPICKER_PALETTE_COLS; i++) {
		float gray;
		gray = 1.0f - ((float)i / (float)(COLORPICKER_PALETTE_COLS - 1));
		palette[*count][0] = gray;
		palette[*count][1] = gray;
		palette[*count][2] = gray;
		palette[*count][3] = 1.0f;
		(*count)++;
	}

	/* Remaining rows: vivid colors with light/dark variations */
	for (row = 1; row < COLORPICKER_PALETTE_ROWS; row++) {
		colorRowIndex = row - 1;
		for (col = 0; col < COLORPICKER_PALETTE_COLS; col++) {
			base = baseColors[col];

			if (colorRowIndex == baseRowIndex) {
				r = base[0];
				g = base[1];
				b = base[2];
			} else if (colorRowIndex <= COLORPICKER_LIGHTEN_COUNT) {
				factorIndex = colorRowIndex - 1;
				if (factorIndex < 0) {
					factorIndex = 0;
				} else if (factorIndex >= COLORPICKER_LIGHTEN_COUNT) {
					factorIndex = COLORPICKER_LIGHTEN_COUNT - 1;
				}
				factor = lightenFactors[factorIndex];
				r = base[0] + (1.0f - base[0]) * factor;
				g = base[1] + (1.0f - base[1]) * factor;
				b = base[2] + (1.0f - base[2]) * factor;
			} else {
				factorIndex = colorRowIndex - 1 - COLORPICKER_LIGHTEN_COUNT;
				if (factorIndex < 0) {
					factorIndex = 0;
				} else if (factorIndex >= COLORPICKER_DARKEN_COUNT) {
					factorIndex = COLORPICKER_DARKEN_COUNT - 1;
				}
				factor = darkenFactors[factorIndex];
				r = base[0] * (1.0f - factor);
				g = base[1] * (1.0f - factor);
				b = base[2] * (1.0f - factor);
			}
			if (r < 0.0f) {
				r = 0.0f;
			} else if (r > 1.0f) {
				r = 1.0f;
			}
			if (g < 0.0f) {
				g = 0.0f;
			} else if (g > 1.0f) {
				g = 1.0f;
			}
			if (b < 0.0f) {
				b = 0.0f;
			} else if (b > 1.0f) {
				b = 1.0f;
			}

			palette[*count][0] = r;
			palette[*count][1] = g;
			palette[*count][2] = b;
			palette[*count][3] = 1.0f;
			(*count)++;
		}
	}

}

static void UI_ColorPicker_UpdateHexString(uiColorPicker_t *picker, int red, int green, int blue) {
	static const char hexDigits[] = "0123456789ABCDEF";

	if (!picker) {
		return;
	}

	if (red < 0) {
		red = 0;
	} else if (red > 255) {
		red = 255;
	}

	if (green < 0) {
		green = 0;
	} else if (green > 255) {
		green = 255;
	}

	if (blue < 0) {
		blue = 0;
	} else if (blue > 255) {
		blue = 255;
	}

	picker->hexBuffer[0] = hexDigits[(red >> 4) & 0x0F];
	picker->hexBuffer[1] = hexDigits[red & 0x0F];
	picker->hexBuffer[2] = hexDigits[(green >> 4) & 0x0F];
	picker->hexBuffer[3] = hexDigits[green & 0x0F];
	picker->hexBuffer[4] = hexDigits[(blue >> 4) & 0x0F];
	picker->hexBuffer[5] = hexDigits[blue & 0x0F];
	picker->hexBuffer[6] = '\0';
}

static void UI_ColorPicker_UpdateFromCurrent(uiColorPicker_t *picker) {
	int red;
	int green;
	int blue;

	if (!picker) {
		return;
	}

	red = (int)(picker->currentColor[0] * 255.0f + 0.5f);
	green = (int)(picker->currentColor[1] * 255.0f + 0.5f);
	blue = (int)(picker->currentColor[2] * 255.0f + 0.5f);

	if (red < 0) {
		red = 0;
	} else if (red > 255) {
		red = 255;
	}

	if (green < 0) {
		green = 0;
	} else if (green > 255) {
		green = 255;
	}

	if (blue < 0) {
		blue = 0;
	} else if (blue > 255) {
		blue = 255;
	}

	picker->sliderValues[0] = (float)red / 255.0f;
	picker->sliderValues[1] = (float)green / 255.0f;
	picker->sliderValues[2] = (float)blue / 255.0f;

	Com_sprintf(picker->componentText[0], sizeof(picker->componentText[0]), "%d", red);
	Com_sprintf(picker->componentText[1], sizeof(picker->componentText[1]), "%d", green);
	Com_sprintf(picker->componentText[2], sizeof(picker->componentText[2]), "%d", blue);

	UI_ColorPicker_UpdateHexString(picker, red, green, blue);
}

static int UI_ColorPicker_ParseComponentText(const char *text) {
	int value;
	int i;
	char c;

	value = 0;

	if (!text) {
		return 0;
	}

	for (i = 0; text[i]; i++) {
		c = text[i];
		if (c < '0' || c > '9') {
			break;
		}
		value = value * 10 + (c - '0');
		if (value > 255) {
			value = 255;
			break;
		}
	}

	return value;
}

static int UI_ColorPicker_FindPaletteIndex(const vec4_t color) {
	vec4_t palette[COLORPICKER_PALETTE_CELLS];
	int paletteCount;
	int i;
	float r;
	float g;
	float b;
	float tolerance;
	float delta;

	UI_ColorPicker_GeneratePalette(palette, &paletteCount);

	r = color[0];
	g = color[1];
	b = color[2];
	tolerance = 0.001f;

	for (i = 0; i < paletteCount; i++) {
		delta = palette[i][0] - r;
		if (delta < 0.0f) {
			delta = -delta;
		}
		if (delta > tolerance) {
			continue;
		}

		delta = palette[i][1] - g;
		if (delta < 0.0f) {
			delta = -delta;
		}
		if (delta > tolerance) {
			continue;
		}

		delta = palette[i][2] - b;
		if (delta < 0.0f) {
			delta = -delta;
		}
		if (delta > tolerance) {
			continue;
		}

		return i;
	}

	return -1;
}

static void UI_ColorPicker_ResetInputState(uiColorPicker_t *picker) {
	if (!picker) {
		return;
	}

	picker->focusedField = -1;
}

static void UI_ColorPicker_UpdateModeCvar(int mode) {
	const char *value;

	value = "0";
	if (mode == COLORPICKER_MODE_RGB) {
		value = "1";
	} else if (mode == COLORPICKER_MODE_PALETTE) {
		value = "2";
	}

	trap_Cvar_Set("ui_pallete_type", value);
	trap_Cvar_Update(&ui_pallete_type);
}

static void UI_ColorPicker_SetMode(uiColorPicker_t *picker, int mode) {
	int previousMode;

	if (!picker) {
		return;
	}

	if (mode != COLORPICKER_MODE_RGB && mode != COLORPICKER_MODE_PALETTE) {
		mode = COLORPICKER_MODE_RGB;
	}

	previousMode = picker->mode;
	picker->mode = mode;
	if (previousMode != mode) {
		UI_ColorPicker_UpdateModeCvar(mode);
	}

	UI_ColorPicker_ResetInputState(picker);
}

static void UI_ColorPicker_SetComponentFromValue(uiColorPicker_t *picker, int componentIndex, int value) {
	int red;
	int green;
	int blue;

	if (!picker) {
		return;
	}

	if (componentIndex < 0 || componentIndex > 2) {
		return;
	}

	if (value < 0) {
		value = 0;
	} else if (value > 255) {
		value = 255;
	}

	picker->sliderValues[componentIndex] = (float)value / 255.0f;
	picker->currentColor[componentIndex] = picker->sliderValues[componentIndex];
	picker->currentColor[3] = 1.0f;
	Com_sprintf(picker->componentText[componentIndex], sizeof(picker->componentText[componentIndex]), "%d", value);

	red = (int)(picker->sliderValues[0] * 255.0f + 0.5f);
	green = (int)(picker->sliderValues[1] * 255.0f + 0.5f);
	blue = (int)(picker->sliderValues[2] * 255.0f + 0.5f);

	UI_ColorPicker_UpdateHexString(picker, red, green, blue);
}

static void UI_ColorPicker_UpdateFromHexString(uiColorPicker_t *picker) {
	vec4_t parsed;
	int red;
	int green;
	int blue;

	if (!picker) {
		return;
	}

	if (UI_ColorPicker_ParseHexColor(picker->hexBuffer, parsed)) {
		picker->currentColor[0] = parsed[0];
		picker->currentColor[1] = parsed[1];
		picker->currentColor[2] = parsed[2];
		picker->currentColor[3] = parsed[3];

		red = (int)(parsed[0] * 255.0f + 0.5f);
		green = (int)(parsed[1] * 255.0f + 0.5f);
		blue = (int)(parsed[2] * 255.0f + 0.5f);

		if (red < 0) {
			red = 0;
		} else if (red > 255) {
			red = 255;
		}
		if (green < 0) {
			green = 0;
		} else if (green > 255) {
			green = 255;
		}
		if (blue < 0) {
			blue = 0;
		} else if (blue > 255) {
			blue = 255;
		}

		picker->sliderValues[0] = (float)red / 255.0f;
		picker->sliderValues[1] = (float)green / 255.0f;
		picker->sliderValues[2] = (float)blue / 255.0f;

		Com_sprintf(picker->componentText[0], sizeof(picker->componentText[0]), "%d", red);
		Com_sprintf(picker->componentText[1], sizeof(picker->componentText[1]), "%d", green);
		Com_sprintf(picker->componentText[2], sizeof(picker->componentText[2]), "%d", blue);

		picker->selectedPaletteIndex = -1;
	}
}

static void UI_ColorPicker_DrawModeTabs(uiColorPicker_t *picker, int tabY) {
	vec4_t activeColor;
	vec4_t inactiveColor;
	vec4_t borderColor;
	int rgbX;
	int paletteX;

	if (!picker) {
		return;
	}

	activeColor[0] = 0.25f;
	activeColor[1] = 0.25f;
	activeColor[2] = 0.25f;
	activeColor[3] = 0.9f;

	inactiveColor[0] = 0.10f;
	inactiveColor[1] = 0.10f;
	inactiveColor[2] = 0.10f;
	inactiveColor[3] = 0.9f;

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	rgbX = picker->x + COLORPICKER_PANEL_PADDING;
	paletteX = rgbX + COLORPICKER_MODE_BUTTON_WIDTH + COLORPICKER_MODE_BUTTON_GAP;

	UI_FillRect(rgbX, tabY, COLORPICKER_MODE_BUTTON_WIDTH, COLORPICKER_MODE_BUTTON_HEIGHT,
	            picker->mode == COLORPICKER_MODE_RGB ? activeColor : inactiveColor);
	UI_DrawRect(rgbX, tabY, COLORPICKER_MODE_BUTTON_WIDTH, COLORPICKER_MODE_BUTTON_HEIGHT, borderColor);
	UI_DrawString(rgbX + 8, tabY + 4, "RGB", UI_SMALLFONT | UI_LEFT, color_white);

	UI_FillRect(paletteX, tabY, COLORPICKER_MODE_BUTTON_WIDTH, COLORPICKER_MODE_BUTTON_HEIGHT,
	            picker->mode == COLORPICKER_MODE_PALETTE ? activeColor : inactiveColor);
	UI_DrawRect(paletteX, tabY, COLORPICKER_MODE_BUTTON_WIDTH, COLORPICKER_MODE_BUTTON_HEIGHT, borderColor);
	UI_DrawString(paletteX + 8, tabY + 4, "Palette", UI_SMALLFONT | UI_LEFT, color_white);
}

static void UI_ColorPicker_DrawButtons(uiColorPicker_t *picker, int buttonY) {
	vec4_t buttonColor;
	vec4_t borderColor;
	vec4_t hintColor;
	int applyX;
	int cancelX;
	int hintX;

	if (!picker) {
		return;
	}

	buttonColor[0] = 0.15f;
	buttonColor[1] = 0.15f;
	buttonColor[2] = 0.15f;
	buttonColor[3] = 0.9f;

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	hintColor[0] = 0.5f;
	hintColor[1] = 0.5f;
	hintColor[2] = 0.5f;
	hintColor[3] = 1.0f;

	applyX = picker->x + picker->width - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_WIDTH;
	cancelX = applyX - COLORPICKER_BUTTON_GAP - COLORPICKER_BUTTON_WIDTH;

	/* Draw hint text in palette mode */
	if ( picker->mode == COLORPICKER_MODE_PALETTE ) {
		hintX = picker->x + COLORPICKER_PANEL_PADDING;
		UI_DrawString( hintX, buttonY + 4, "hold alt to quick use", UI_SMALLFONT | UI_LEFT, hintColor );
	}

	UI_FillRect(cancelX, buttonY, COLORPICKER_BUTTON_WIDTH, COLORPICKER_BUTTON_HEIGHT, buttonColor);
	UI_DrawRect(cancelX, buttonY, COLORPICKER_BUTTON_WIDTH, COLORPICKER_BUTTON_HEIGHT, borderColor);
	UI_DrawString(cancelX + 12, buttonY + 4, "Cancel", UI_SMALLFONT | UI_LEFT, color_white);

	UI_FillRect(applyX, buttonY, COLORPICKER_BUTTON_WIDTH, COLORPICKER_BUTTON_HEIGHT, buttonColor);
	UI_DrawRect(applyX, buttonY, COLORPICKER_BUTTON_WIDTH, COLORPICKER_BUTTON_HEIGHT, borderColor);
	UI_DrawString(applyX + 20, buttonY + 4, "Apply", UI_SMALLFONT | UI_LEFT, color_white);
}

static void UI_ColorPicker_DrawPreview(uiColorPicker_t *picker, int previewX, int previewY, int labelY) {
	vec4_t borderColor;
	vec4_t fieldBackground;
	vec4_t focusBackground;
	vec4_t buttonBackground;
	vec4_t buttonHover;
	int hexFieldX;
	int hexFieldY;
	int textY;
	int copyButtonX;
	int copyButtonY;
	int pasteButtonX;
	int pasteButtonY;
	qhandle_t copyIcon;
	int iconSize;
	int iconX;
	int iconY;

	if (!picker) {
		return;
	}

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	fieldBackground[0] = 0.12f;
	fieldBackground[1] = 0.12f;
	fieldBackground[2] = 0.12f;
	fieldBackground[3] = 0.9f;

	focusBackground[0] = 0.20f;
	focusBackground[1] = 0.20f;
	focusBackground[2] = 0.20f;
	focusBackground[3] = 0.9f;

	buttonBackground[0] = 0.15f;
	buttonBackground[1] = 0.15f;
	buttonBackground[2] = 0.15f;
	buttonBackground[3] = 0.9f;

	buttonHover[0] = 0.25f;
	buttonHover[1] = 0.25f;
	buttonHover[2] = 0.25f;
	buttonHover[3] = 0.9f;

	UI_DrawString(previewX, labelY - 12, "Preview", UI_SMALLFONT | UI_LEFT, color_white);
	UI_FillRect(previewX, previewY - 4, COLORPICKER_RGB_PREVIEW_SIZE, COLORPICKER_RGB_PREVIEW_SIZE, picker->currentColor);
	UI_DrawRect(previewX, previewY - 4, COLORPICKER_RGB_PREVIEW_SIZE, COLORPICKER_RGB_PREVIEW_SIZE, borderColor);

	textY = previewY - 4 + COLORPICKER_RGB_PREVIEW_SIZE + 6;
	UI_DrawString(previewX, textY, "Hex:", UI_SMALLFONT | UI_LEFT, color_white);

	hexFieldX = previewX;
	hexFieldY = textY + 12;
	UI_FillRect(hexFieldX, hexFieldY, COLORPICKER_HEX_FIELD_WIDTH, COLORPICKER_HEX_FIELD_HEIGHT,
	            picker->focusedField == COLORPICKER_FIELD_HEX ? focusBackground : fieldBackground);
	UI_DrawRect(hexFieldX, hexFieldY, COLORPICKER_HEX_FIELD_WIDTH, COLORPICKER_HEX_FIELD_HEIGHT, borderColor);
	UI_DrawString(hexFieldX + COLORPICKER_HEX_FIELD_PADDING, hexFieldY + 2,
	              picker->hexBuffer, UI_SMALLFONT | UI_LEFT, color_white);

	copyButtonX = hexFieldX;
	copyButtonY = hexFieldY + COLORPICKER_HEX_FIELD_HEIGHT + COLORPICKER_COPY_PASTE_GAP;
	pasteButtonX = copyButtonX + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;
	pasteButtonY = copyButtonY;

	UI_FillRect(copyButtonX, copyButtonY, COLORPICKER_COPY_BUTTON_SIZE, COLORPICKER_COPY_BUTTON_SIZE, buttonBackground);
	UI_DrawRect(copyButtonX, copyButtonY, COLORPICKER_COPY_BUTTON_SIZE, COLORPICKER_COPY_BUTTON_SIZE, borderColor);
	copyIcon = trap_R_RegisterShaderNoMip( "menu/be/copy.png" );
	if ( copyIcon ) {
		iconSize = COLORPICKER_COPY_BUTTON_SIZE - 6;
		iconX = copyButtonX + ( COLORPICKER_COPY_BUTTON_SIZE - iconSize ) / 2;
		iconY = copyButtonY + ( COLORPICKER_COPY_BUTTON_SIZE - iconSize ) / 2;
		trap_R_SetColor( NULL );
		UI_DrawHandlePic( iconX, iconY, iconSize, iconSize, copyIcon );
	}

	UI_FillRect(pasteButtonX, pasteButtonY, COLORPICKER_PASTE_BUTTON_WIDTH, COLORPICKER_PASTE_BUTTON_HEIGHT, buttonBackground);
	UI_DrawRect(pasteButtonX, pasteButtonY, COLORPICKER_PASTE_BUTTON_WIDTH, COLORPICKER_PASTE_BUTTON_HEIGHT, borderColor);
	UI_DrawString(pasteButtonX + 4, pasteButtonY + 2, "Paste", UI_SMALLFONT | UI_LEFT, color_white);

	{
		int lastUsedX;
		int lastUsedY;
		int copiedX;
		int copiedY;
		int i;
		int col;
		int row;
		int cellX;
		int cellY;
		qhandle_t copyIconSmall;
		int iconSize;

		lastUsedX = hexFieldX;
		lastUsedY = copyButtonY + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;

		for ( i = 0; i < COLORPICKER_LAST_USED_COUNT; i++ ) {
			col = i % COLORPICKER_LAST_USED_COLS;
			row = i / COLORPICKER_LAST_USED_COLS;
			cellX = lastUsedX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
			cellY = lastUsedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );

			if ( uiColorPickerLastUsedColorsUsed[i] ) {
				UI_FillRect( cellX, cellY, COLORPICKER_SAVED_COLOR_SIZE, COLORPICKER_SAVED_COLOR_SIZE, uiColorPickerLastUsedColors[i] );
			}
			UI_DrawRect( cellX, cellY, COLORPICKER_SAVED_COLOR_SIZE, COLORPICKER_SAVED_COLOR_SIZE, borderColor );
		}

		copiedX = hexFieldX;
		copiedY = lastUsedY + COLORPICKER_LAST_USED_ROWS * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP ) + COLORPICKER_COPY_PASTE_GAP;

		copyIconSmall = trap_R_RegisterShaderNoMip( "menu/be/copy.png" );
		if ( copyIconSmall ) {
			iconSize = 12;
			UI_DrawHandlePic( copiedX, copiedY + 2, iconSize, iconSize, copyIconSmall );
		}
		copiedY += COLORPICKER_SECTION_HEADER_HEIGHT;

		for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
			col = i % COLORPICKER_COPIED_COLS;
			row = i / COLORPICKER_COPIED_COLS;
			cellX = hexFieldX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
			cellY = copiedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );

			if ( uiColorPickerCopiedColorsUsed[i] ) {
				UI_FillRect( cellX, cellY, COLORPICKER_SAVED_COLOR_SIZE, COLORPICKER_SAVED_COLOR_SIZE, uiColorPickerCopiedColors[i] );
			}
			UI_DrawRect( cellX, cellY, COLORPICKER_SAVED_COLOR_SIZE, COLORPICKER_SAVED_COLOR_SIZE, borderColor );
		}
	}
}

static void UI_ColorPicker_DrawRGB(uiColorPicker_t *picker, int contentTop) {
	vec4_t sliderBackground;
	vec4_t borderColor;
	vec4_t fieldBackground;
	vec4_t focusBackground;
	int sliderX;
	int labelX;
	int fieldX;
	int previewX;
	int previewY;
	int row;
	int sliderY;
	int fieldY;
	int handleX;
	int offset;
	int step;
	char labelText[2];
	vec4_t gradientColor;
	int sliderYOffset;
	int fieldYOffset;

	if (!picker) {
		return;
	}

	sliderBackground[0] = 0.08f;
	sliderBackground[1] = 0.08f;
	sliderBackground[2] = 0.08f;
	sliderBackground[3] = 0.9f;

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	fieldBackground[0] = 0.12f;
	fieldBackground[1] = 0.12f;
	fieldBackground[2] = 0.12f;
	fieldBackground[3] = 0.9f;

	focusBackground[0] = 0.20f;
	focusBackground[1] = 0.20f;
	focusBackground[2] = 0.20f;
	focusBackground[3] = 0.9f;

	sliderX = picker->x + COLORPICKER_PANEL_PADDING + COLORPICKER_RGB_LABEL_OFFSET;
	labelX = sliderX - COLORPICKER_RGB_LABEL_OFFSET;
	fieldX = sliderX + COLORPICKER_RGB_SLIDER_WIDTH + 12;
	previewX = fieldX + COLORPICKER_RGB_FIELD_WIDTH + 16;
	previewY = contentTop - 8;

	sliderYOffset = (COLORPICKER_RGB_ROW_HEIGHT - COLORPICKER_RGB_SLIDER_HEIGHT) / 2;
	fieldYOffset = (COLORPICKER_RGB_ROW_HEIGHT - COLORPICKER_RGB_FIELD_HEIGHT) / 2;

	for (row = 0; row < 3; row++) {
		labelText[0] = (char)('R' + row);
		labelText[1] = '\0';

		offset = contentTop + row * COLORPICKER_RGB_ROW_HEIGHT;
		sliderY = offset + sliderYOffset;
		fieldY = offset + fieldYOffset;

		UI_DrawString(labelX, sliderY + 2, labelText, UI_SMALLFONT | UI_LEFT, color_white);

		UI_FillRect(sliderX, sliderY, COLORPICKER_RGB_SLIDER_WIDTH, COLORPICKER_RGB_SLIDER_HEIGHT, sliderBackground);

		for (step = 0; step < COLORPICKER_RGB_SLIDER_WIDTH; step++) {
			float fraction;

			fraction = (float)step / (float)(COLORPICKER_RGB_SLIDER_WIDTH - 1);
			gradientColor[0] = picker->sliderValues[0];
			gradientColor[1] = picker->sliderValues[1];
			gradientColor[2] = picker->sliderValues[2];
			gradientColor[row] = fraction;
			gradientColor[3] = 1.0f;
			UI_FillRect(sliderX + step, sliderY, 1, COLORPICKER_RGB_SLIDER_HEIGHT, gradientColor);
		}

		UI_DrawRect(sliderX, sliderY, COLORPICKER_RGB_SLIDER_WIDTH, COLORPICKER_RGB_SLIDER_HEIGHT, borderColor);

		handleX = sliderX + (int)(picker->sliderValues[row] * (float)(COLORPICKER_RGB_SLIDER_WIDTH - 1) + 0.5f);
		if (handleX < sliderX) {
			handleX = sliderX;
		}
		if (handleX > sliderX + COLORPICKER_RGB_SLIDER_WIDTH - 1) {
			handleX = sliderX + COLORPICKER_RGB_SLIDER_WIDTH - 1;
		}
		UI_FillRect(handleX - 2, sliderY - 2, 4, COLORPICKER_RGB_SLIDER_HEIGHT + 4, borderColor);

		UI_FillRect(fieldX, fieldY, COLORPICKER_RGB_FIELD_WIDTH, COLORPICKER_RGB_FIELD_HEIGHT,
		            picker->focusedField == row ? focusBackground : fieldBackground);
		UI_DrawRect(fieldX, fieldY, COLORPICKER_RGB_FIELD_WIDTH, COLORPICKER_RGB_FIELD_HEIGHT, borderColor);
		UI_DrawString(fieldX + COLORPICKER_RGB_FIELD_PADDING, fieldY + 2,
		              picker->componentText[row], UI_SMALLFONT | UI_LEFT, color_white);
	}

	UI_ColorPicker_DrawPreview(picker, previewX, previewY, contentTop - 20);
}

static void UI_ColorPicker_DrawPalette(uiColorPicker_t *picker, int contentTop) {
	vec4_t palette[COLORPICKER_PALETTE_CELLS];
	int paletteCount;
	vec4_t borderColor;
	int gridWidth;
	int gridHeight;
	int gridLeft;
	int gridTop;
	int i;
	int row;
	int col;
	int cellX;
	int cellY;
	vec4_t highlightColor;
	int previewX;
	int previewY;

	if (!picker) {
		return;
	}

	UI_ColorPicker_GeneratePalette(palette, &paletteCount);

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	highlightColor[0] = 1.0f;
	highlightColor[1] = 1.0f;
	highlightColor[2] = 1.0f;
	highlightColor[3] = 1.0f;

	gridWidth = COLORPICKER_PALETTE_COLS * COLORPICKER_CELL_SIZE;
	gridHeight = COLORPICKER_PALETTE_ROWS * COLORPICKER_CELL_SIZE;
	gridLeft = picker->x + COLORPICKER_PANEL_PADDING;
	gridTop = contentTop;

	if (gridTop + gridHeight > picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT - 12) {
		gridTop = picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT - 12 - gridHeight;
	}

	if (gridLeft + gridWidth + COLORPICKER_RGB_PREVIEW_SIZE + 32 > picker->x + picker->width) {
		gridLeft = picker->x + (picker->width - gridWidth) / 2;
	}

	for (i = 0; i < paletteCount; i++) {
		row = i / COLORPICKER_PALETTE_COLS;
		col = i % COLORPICKER_PALETTE_COLS;
		cellX = gridLeft + col * COLORPICKER_CELL_SIZE;
		cellY = gridTop + row * COLORPICKER_CELL_SIZE;

		UI_FillRect(cellX + 1, cellY + 1, COLORPICKER_CELL_SIZE - 2, COLORPICKER_CELL_SIZE - 2, palette[i]);
		UI_DrawRect(cellX, cellY, COLORPICKER_CELL_SIZE, COLORPICKER_CELL_SIZE, borderColor);

		if (picker->selectedPaletteIndex == i) {
			UI_DrawRect(cellX - 2, cellY - 2, COLORPICKER_CELL_SIZE + 4, COLORPICKER_CELL_SIZE + 4, highlightColor);
		}
	}

	previewX = picker->x + picker->width - COLORPICKER_PANEL_PADDING - COLORPICKER_RGB_PREVIEW_SIZE;
	previewY = gridTop - 8;
	UI_ColorPicker_DrawPreview(picker, previewX, previewY, contentTop - 20);
}

static void UI_ColorPicker_Commit(uiColorPicker_t *picker) {
	if (!picker) {
		return;
	}

	UI_ColorPicker_UpdateFromCurrent( picker );
	UI_ColorPicker_AddToLastUsed( picker->currentColor );

	if (picker->callback) {
		picker->callback(picker->hexBuffer, picker->userData);
	}

	UI_ColorPicker_Close(picker);
}

void UI_ColorPicker_Init(uiColorPicker_t *picker) {
	int i;

	if (!picker) {
		return;
	}

	picker->active = qfalse;
	picker->currentColor[0] = 1.0f;
	picker->currentColor[1] = 1.0f;
	picker->currentColor[2] = 1.0f;
	picker->currentColor[3] = 1.0f;
	picker->hexBuffer[0] = '\0';

	for ( i = 0; i < COLORPICKER_LAST_USED_COUNT; i++ ) {
		uiColorPickerLastUsedColorsUsed[i] = qfalse;
	}
	for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
		uiColorPickerCopiedColorsUsed[i] = qfalse;
	}
	picker->callback = NULL;
	picker->userData = NULL;
	picker->x = 0;
	picker->y = 0;
	picker->width = COLORPICKER_PANEL_WIDTH;
	picker->height = COLORPICKER_PANEL_HEIGHT;
	picker->mode = COLORPICKER_MODE_RGB;
	picker->sliderValues[0] = 1.0f;
	picker->sliderValues[1] = 1.0f;
	picker->sliderValues[2] = 1.0f;
	Com_sprintf(picker->componentText[0], sizeof(picker->componentText[0]), "%d", 255);
	Com_sprintf(picker->componentText[1], sizeof(picker->componentText[1]), "%d", 255);
	Com_sprintf(picker->componentText[2], sizeof(picker->componentText[2]), "%d", 255);
	picker->focusedField = -1;
	picker->selectedPaletteIndex = UI_ColorPicker_FindPaletteIndex(picker->currentColor);
	UI_ColorPicker_UpdateHexString(picker, 255, 255, 255);
}

void UI_ColorPicker_Open(uiColorPicker_t *picker, const char *initialValue, void (*callback)(const char *hexColor, void *userData), void *userData) {
	int savedMode;

	if (!picker) {
		return;
	}

	picker->active = qtrue;
	picker->callback = callback;
	picker->userData = userData;
	picker->width = COLORPICKER_PANEL_WIDTH;
	picker->height = COLORPICKER_PANEL_HEIGHT;

	/* Parse initial color */
	if (!UI_ColorPicker_ParseColorValue(initialValue, picker->currentColor)) {
		picker->currentColor[0] = 1.0f;
		picker->currentColor[1] = 1.0f;
		picker->currentColor[2] = 1.0f;
		picker->currentColor[3] = 1.0f;
	}

	UI_ColorPicker_UpdateFromCurrent(picker);
	picker->selectedPaletteIndex = UI_ColorPicker_FindPaletteIndex(picker->currentColor);
	trap_Cvar_Update(&ui_pallete_type);
	savedMode = ui_pallete_type.integer;
	if (savedMode == 2) {
		UI_ColorPicker_SetMode(picker, COLORPICKER_MODE_PALETTE);
	} else if (savedMode == 1) {
		UI_ColorPicker_SetMode(picker, COLORPICKER_MODE_RGB);
	} else {
		UI_ColorPicker_SetMode(picker, COLORPICKER_MODE_RGB);
	}

	/* Center the picker */
	picker->x = (640 - picker->width) / 2;
	picker->y = (480 - picker->height) / 2;
}

void UI_ColorPicker_Close(uiColorPicker_t *picker) {
	if (!picker) {
		return;
	}

	picker->active = qfalse;
	UI_ColorPicker_ResetInputState(picker);
}

qboolean UI_ColorPicker_IsActive(uiColorPicker_t *picker) {
	if (!picker) {
		return qfalse;
	}

	return picker->active;
}

void UI_ColorPicker_Draw(uiColorPicker_t *picker) {
	vec4_t background;
	vec4_t borderColor;
	int tabY;
	int contentTop;
	int buttonY;

	if (!picker || !picker->active) {
		return;
	}

	background[0] = 0.02f;
	background[1] = 0.02f;
	background[2] = 0.02f;
	background[3] = 0.92f;

	borderColor[0] = 1.0f;
	borderColor[1] = 1.0f;
	borderColor[2] = 1.0f;
	borderColor[3] = 1.0f;

	UI_FillRect(picker->x, picker->y, picker->width, picker->height, background);
	UI_DrawRect(picker->x, picker->y, picker->width, picker->height, borderColor);

	tabY = picker->y + COLORPICKER_PANEL_PADDING;
	contentTop = tabY + COLORPICKER_MODE_BUTTON_HEIGHT + 12;
	buttonY = picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT;

	UI_ColorPicker_DrawModeTabs(picker, tabY);

	if (picker->mode == COLORPICKER_MODE_RGB) {
		UI_ColorPicker_DrawRGB(picker, contentTop);
	} else {
		UI_ColorPicker_DrawPalette(picker, contentTop);
	}

	UI_ColorPicker_DrawButtons(picker, buttonY);
}

qboolean UI_ColorPicker_HandleKey(uiColorPicker_t *picker, int key) {
	int field;
	char *buffer;
	int len;
	int maxLen;
	int value;

	field = 0;
	buffer = NULL;
	len = 0;
	maxLen = 0;
	value = 0;

	if (!picker || !picker->active) {
		return qfalse;
	}

	if (key == K_ESCAPE || key == K_MOUSE2) {
		UI_ColorPicker_Close(picker);
		return qtrue;
	}

	if (key == K_ENTER || key == K_KP_ENTER) {
		UI_ColorPicker_Commit(picker);
		return qtrue;
	}

	if (picker->mode == COLORPICKER_MODE_RGB) {
		if (key == K_TAB) {
			if (picker->focusedField < 0 || picker->focusedField >= COLORPICKER_FIELD_HEX) {
				picker->focusedField = 0;
			} else {
				picker->focusedField++;
				if (picker->focusedField > COLORPICKER_FIELD_HEX) {
					picker->focusedField = 0;
				}
			}
			return qtrue;
		}

		field = picker->focusedField;

		if (field == COLORPICKER_FIELD_HEX) {
			buffer = picker->hexBuffer;
			maxLen = (int)sizeof(picker->hexBuffer) - 1;

			if (key >= '0' && key <= '9') {
				len = 0;
				while (buffer[len]) {
					len++;
					if (len >= maxLen) {
						break;
					}
				}

				if (len >= 6) {
					return qtrue;
				}

				buffer[len] = (char)key;
				buffer[len + 1] = '\0';
				UI_ColorPicker_UpdateFromHexString(picker);
				return qtrue;
			}

			if ((key >= 'a' && key <= 'f') || (key >= 'A' && key <= 'F')) {
				char hexChar;
				len = 0;
				while (buffer[len]) {
					len++;
					if (len >= maxLen) {
						break;
					}
				}

				if (len >= 6) {
					return qtrue;
				}

				hexChar = (char)key;
				if (hexChar >= 'a' && hexChar <= 'f') {
					hexChar = (char)(hexChar - 'a' + 'A');
				}
				buffer[len] = hexChar;
				buffer[len + 1] = '\0';
				UI_ColorPicker_UpdateFromHexString(picker);
				return qtrue;
			}

			if (key == K_BACKSPACE) {
				len = 0;
				while (buffer[len]) {
					len++;
				}

				if (len > 0) {
					buffer[len - 1] = '\0';
					UI_ColorPicker_UpdateFromHexString(picker);
				}
				return qtrue;
			}
		}

		if (field < 0 || field > 2) {
			if (key >= '0' && key <= '9') {
				picker->focusedField = 0;
				field = 0;
			} else {
				return qfalse;
			}
		}

		if (field >= 0 && field <= 2) {
			buffer = picker->componentText[field];
			maxLen = (int)sizeof(picker->componentText[field]) - 1;

			if (key >= '0' && key <= '9') {
				len = 0;
				while (buffer[len]) {
					len++;
					if (len >= maxLen) {
						break;
					}
				}

				if (len >= maxLen) {
					return qtrue;
				}

				if (len == 1 && buffer[0] == '0') {
					buffer[0] = (char)key;
					buffer[1] = '\0';
				} else {
					buffer[len] = (char)key;
					buffer[len + 1] = '\0';
				}

				value = UI_ColorPicker_ParseComponentText(buffer);
				UI_ColorPicker_SetComponentFromValue(picker, field, value);
				picker->selectedPaletteIndex = -1;
				return qtrue;
			}

			if (key == K_BACKSPACE) {
				len = 0;
				while (buffer[len]) {
					len++;
				}

				if (len > 1) {
					buffer[len - 1] = '\0';
				} else {
					buffer[0] = '0';
					buffer[1] = '\0';
				}

				value = UI_ColorPicker_ParseComponentText(buffer);
				UI_ColorPicker_SetComponentFromValue(picker, field, value);
				picker->selectedPaletteIndex = -1;
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean UI_ColorPicker_HandleMouse(uiColorPicker_t *picker, int x, int y, qboolean click) {
	int buttonY;
	int applyX;
	int cancelX;
	int tabY;
	int rgbTabX;
	int paletteTabX;
	int contentTop;

	if (!picker || !picker->active) {
		return qfalse;
	}

	if (x < picker->x || x >= picker->x + picker->width ||
		y < picker->y || y >= picker->y + picker->height) {
		if ( click ) {
			UI_ColorPicker_Close( picker );
		}
		return qfalse;
	}

	buttonY = picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT;
	applyX = picker->x + picker->width - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_WIDTH;
	cancelX = applyX - COLORPICKER_BUTTON_GAP - COLORPICKER_BUTTON_WIDTH;

	if (y >= buttonY && y < buttonY + COLORPICKER_BUTTON_HEIGHT) {
		if (x >= applyX && x < applyX + COLORPICKER_BUTTON_WIDTH) {
			UI_ColorPicker_Commit(picker);
			return qtrue;
		}
		if (x >= cancelX && x < cancelX + COLORPICKER_BUTTON_WIDTH) {
			UI_ColorPicker_Close(picker);
			return qtrue;
		}
	}

	tabY = picker->y + COLORPICKER_PANEL_PADDING;
	rgbTabX = picker->x + COLORPICKER_PANEL_PADDING;
	paletteTabX = rgbTabX + COLORPICKER_MODE_BUTTON_WIDTH + COLORPICKER_MODE_BUTTON_GAP;

	if (y >= tabY && y < tabY + COLORPICKER_MODE_BUTTON_HEIGHT) {
		if (x >= rgbTabX && x < rgbTabX + COLORPICKER_MODE_BUTTON_WIDTH) {
			if (picker->mode != COLORPICKER_MODE_RGB) {
				UI_ColorPicker_SetMode(picker, COLORPICKER_MODE_RGB);
				UI_ColorPicker_UpdateFromCurrent(picker);
			}
			return qtrue;
		}
		if (x >= paletteTabX && x < paletteTabX + COLORPICKER_MODE_BUTTON_WIDTH) {
			if (picker->mode != COLORPICKER_MODE_PALETTE) {
				UI_ColorPicker_SetMode(picker, COLORPICKER_MODE_PALETTE);
			}
			return qtrue;
		}
	}

	contentTop = tabY + COLORPICKER_MODE_BUTTON_HEIGHT + 12;

	if (picker->mode == COLORPICKER_MODE_RGB) {
		int sliderX;
		int fieldX;
		int previewX;
		int previewY;
		int sliderYOffset;
		int fieldYOffset;
		int row;

		sliderX = picker->x + COLORPICKER_PANEL_PADDING + COLORPICKER_RGB_LABEL_OFFSET;
		fieldX = sliderX + COLORPICKER_RGB_SLIDER_WIDTH + 12;
		previewX = fieldX + COLORPICKER_RGB_FIELD_WIDTH + 16;
		previewY = contentTop - 8;
		sliderYOffset = (COLORPICKER_RGB_ROW_HEIGHT - COLORPICKER_RGB_SLIDER_HEIGHT) / 2;
		fieldYOffset = (COLORPICKER_RGB_ROW_HEIGHT - COLORPICKER_RGB_FIELD_HEIGHT) / 2;

		/* Check hex field click */
		{
			int hexFieldX;
			int hexFieldY;
			int hexFieldLeft;
			int hexFieldRight;
			int hexFieldTop;
			int hexFieldBottom;
			int copyButtonX;
			int copyButtonY;
			int copyButtonLeft;
			int copyButtonRight;
			int copyButtonTop;
			int copyButtonBottom;
			int pasteButtonX;
			int pasteButtonY;
			int pasteButtonLeft;
			int pasteButtonRight;
			int pasteButtonTop;
			int pasteButtonBottom;

			hexFieldX = previewX;
			{
				int textY;
				textY = previewY - 4 + COLORPICKER_RGB_PREVIEW_SIZE + 6;
				hexFieldY = textY + 12;
			}
			hexFieldLeft = hexFieldX;
			hexFieldRight = hexFieldX + COLORPICKER_HEX_FIELD_WIDTH;
			hexFieldTop = hexFieldY;
			hexFieldBottom = hexFieldY + COLORPICKER_HEX_FIELD_HEIGHT;

			if (y >= hexFieldTop && y < hexFieldBottom &&
			    x >= hexFieldLeft && x < hexFieldRight) {
				if ( click ) {
					picker->focusedField = COLORPICKER_FIELD_HEX;
				}
				return qtrue;
			}

			copyButtonX = hexFieldX;
			copyButtonY = hexFieldY + COLORPICKER_HEX_FIELD_HEIGHT + COLORPICKER_COPY_PASTE_GAP;
			copyButtonLeft = copyButtonX;
			copyButtonRight = copyButtonX + COLORPICKER_COPY_BUTTON_SIZE;
			copyButtonTop = copyButtonY;
			copyButtonBottom = copyButtonY + COLORPICKER_COPY_BUTTON_SIZE;

			if (y >= copyButtonTop && y < copyButtonBottom &&
			    x >= copyButtonLeft && x < copyButtonRight) {
				if ( click ) {
					int i;
					int oldestIndex;
					UI_ColorPicker_UpdateFromCurrent( picker );
					oldestIndex = -1;
					for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
						if ( !uiColorPickerCopiedColorsUsed[i] ) {
							oldestIndex = i;
							break;
						}
					}
					if ( oldestIndex < 0 ) {
						oldestIndex = 0;
					}
					uiColorPickerCopiedColors[oldestIndex][0] = picker->currentColor[0];
					uiColorPickerCopiedColors[oldestIndex][1] = picker->currentColor[1];
					uiColorPickerCopiedColors[oldestIndex][2] = picker->currentColor[2];
					uiColorPickerCopiedColors[oldestIndex][3] = picker->currentColor[3];
					uiColorPickerCopiedColorsUsed[oldestIndex] = qtrue;
				}
				return qtrue;
			}

			pasteButtonX = copyButtonX + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;
			pasteButtonY = copyButtonY;
			pasteButtonLeft = pasteButtonX;
			pasteButtonRight = pasteButtonX + COLORPICKER_PASTE_BUTTON_WIDTH;
			pasteButtonTop = pasteButtonY;
			pasteButtonBottom = pasteButtonY + COLORPICKER_PASTE_BUTTON_HEIGHT;

			if (y >= pasteButtonTop && y < pasteButtonBottom &&
			    x >= pasteButtonLeft && x < pasteButtonRight) {
				if ( click ) {
					char clipboardData[64];
					char *source;

					source = NULL;
					trap_GetClipboardData( clipboardData, sizeof( clipboardData ) );
					if ( clipboardData[0] && UI_ColorPicker_ParseHexColor( clipboardData, picker->currentColor ) ) {
						source = clipboardData;
					} else if ( uiColorPickerClipboardBuffer[0] && UI_ColorPicker_ParseHexColor( uiColorPickerClipboardBuffer, picker->currentColor ) ) {
						source = uiColorPickerClipboardBuffer;
					}

					if ( source ) {
						Q_strncpyz( picker->hexBuffer, source, sizeof( picker->hexBuffer ) );
						UI_ColorPicker_UpdateFromHexString( picker );
					}
				}
				return qtrue;
			}

			{
				int lastUsedX;
				int lastUsedY;
				int copiedX;
				int copiedY;
				int i;
				int col;
				int row;
				int cellX;
				int cellY;
				int cellLeft;
				int cellRight;
				int cellTop;
				int cellBottom;

				lastUsedX = hexFieldX;
				lastUsedY = copyButtonY + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;

				for ( i = 0; i < COLORPICKER_LAST_USED_COUNT; i++ ) {
					col = i % COLORPICKER_LAST_USED_COLS;
					row = i / COLORPICKER_LAST_USED_COLS;
					cellX = lastUsedX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellY = lastUsedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellLeft = cellX;
					cellRight = cellX + COLORPICKER_SAVED_COLOR_SIZE;
					cellTop = cellY;
					cellBottom = cellY + COLORPICKER_SAVED_COLOR_SIZE;

					if ( y >= cellTop && y < cellBottom &&
					     x >= cellLeft && x < cellRight ) {
						if ( click && uiColorPickerLastUsedColorsUsed[i] ) {
							picker->currentColor[0] = uiColorPickerLastUsedColors[i][0];
							picker->currentColor[1] = uiColorPickerLastUsedColors[i][1];
							picker->currentColor[2] = uiColorPickerLastUsedColors[i][2];
							picker->currentColor[3] = uiColorPickerLastUsedColors[i][3];
							UI_ColorPicker_UpdateFromCurrent( picker );
						}
						return qtrue;
					}
				}

				copiedX = hexFieldX;
				copiedY = lastUsedY + COLORPICKER_LAST_USED_ROWS * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP ) + COLORPICKER_COPY_PASTE_GAP;
				copiedY += COLORPICKER_SECTION_HEADER_HEIGHT;

				for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
					col = i % COLORPICKER_COPIED_COLS;
					row = i / COLORPICKER_COPIED_COLS;
					cellX = copiedX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellY = copiedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellLeft = cellX;
					cellRight = cellX + COLORPICKER_SAVED_COLOR_SIZE;
					cellTop = cellY;
					cellBottom = cellY + COLORPICKER_SAVED_COLOR_SIZE;

					if ( y >= cellTop && y < cellBottom &&
					     x >= cellLeft && x < cellRight ) {
						if ( click && uiColorPickerCopiedColorsUsed[i] ) {
							picker->currentColor[0] = uiColorPickerCopiedColors[i][0];
							picker->currentColor[1] = uiColorPickerCopiedColors[i][1];
							picker->currentColor[2] = uiColorPickerCopiedColors[i][2];
							picker->currentColor[3] = uiColorPickerCopiedColors[i][3];
							UI_ColorPicker_UpdateFromCurrent( picker );
						}
						return qtrue;
					}
				}
			}
		}

		for (row = 0; row < 3; row++) {
			int rowTop;
			int sliderY;
			int fieldY;
			int sliderLeft;
			int sliderRight;
			int fieldLeft;
			int fieldRight;
			float fraction;
			int value;

			rowTop = contentTop + row * COLORPICKER_RGB_ROW_HEIGHT;
			sliderY = rowTop + sliderYOffset;
			fieldY = rowTop + fieldYOffset;
			sliderLeft = sliderX;
			sliderRight = sliderX + COLORPICKER_RGB_SLIDER_WIDTH;
			fieldLeft = fieldX;
			fieldRight = fieldX + COLORPICKER_RGB_FIELD_WIDTH;

			if (y >= fieldY && y < fieldY + COLORPICKER_RGB_FIELD_HEIGHT &&
			    x >= fieldLeft && x < fieldRight) {
				if ( click ) {
					picker->focusedField = row;
				}
				return qtrue;
			}

			if (y >= sliderY && y < sliderY + COLORPICKER_RGB_SLIDER_HEIGHT &&
			    x >= sliderLeft && x < sliderRight) {
				fraction = (float)(x - sliderLeft) / (float)(COLORPICKER_RGB_SLIDER_WIDTH - 1);
				if (fraction < 0.0f) {
					fraction = 0.0f;
				}
				if (fraction > 1.0f) {
					fraction = 1.0f;
				}
				value = (int)(fraction * 255.0f + 0.5f);
				UI_ColorPicker_SetComponentFromValue(picker, row, value);
				if ( click ) {
					picker->focusedField = row;
				}
				picker->selectedPaletteIndex = -1;
				return qtrue;
			}
		}

		return qtrue;
	}

	/* Palette mode */
	{
		vec4_t palette[COLORPICKER_PALETTE_CELLS];
		int paletteCount;
		int gridWidth;
		int gridHeight;
		int gridLeft;
		int gridTop;
		int relX;
		int relY;
		int col;
		int row;
		int index;

		UI_ColorPicker_GeneratePalette(palette, &paletteCount);

		gridWidth = COLORPICKER_PALETTE_COLS * COLORPICKER_CELL_SIZE;
		gridHeight = COLORPICKER_PALETTE_ROWS * COLORPICKER_CELL_SIZE;
		gridLeft = picker->x + COLORPICKER_PANEL_PADDING;
		gridTop = contentTop;

		if (gridTop + gridHeight > picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT - 12) {
			gridTop = picker->y + picker->height - COLORPICKER_PANEL_PADDING - COLORPICKER_BUTTON_HEIGHT - 12 - gridHeight;
		}

		if (gridLeft + gridWidth + COLORPICKER_RGB_PREVIEW_SIZE + 32 > picker->x + picker->width) {
			gridLeft = picker->x + (picker->width - gridWidth) / 2;
		}

		{
			int previewX;
			int previewY;
			int hexFieldX;
			int hexFieldY;
			int copyButtonX;
			int copyButtonY;
			int copyButtonLeft;
			int copyButtonRight;
			int copyButtonTop;
			int copyButtonBottom;
			int pasteButtonX;
			int pasteButtonY;
			int pasteButtonLeft;
			int pasteButtonRight;
			int pasteButtonTop;
			int pasteButtonBottom;

			previewX = picker->x + picker->width - COLORPICKER_PANEL_PADDING - COLORPICKER_RGB_PREVIEW_SIZE;
			previewY = gridTop - 8;
			hexFieldX = previewX;
			{
				int textY;
				textY = previewY + COLORPICKER_RGB_PREVIEW_SIZE + 12;
				hexFieldY = textY + 18;
			}

			copyButtonX = hexFieldX;
			copyButtonY = hexFieldY + COLORPICKER_HEX_FIELD_HEIGHT + COLORPICKER_COPY_PASTE_GAP;
			copyButtonLeft = copyButtonX;
			copyButtonRight = copyButtonX + COLORPICKER_COPY_BUTTON_SIZE;
			copyButtonTop = copyButtonY;
			copyButtonBottom = copyButtonY + COLORPICKER_COPY_BUTTON_SIZE;

			if (y >= copyButtonTop && y < copyButtonBottom &&
			    x >= copyButtonLeft && x < copyButtonRight) {
				if ( click ) {
					int i;
					int oldestIndex;
					UI_ColorPicker_UpdateFromCurrent( picker );
					oldestIndex = -1;
					for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
						if ( !uiColorPickerCopiedColorsUsed[i] ) {
							oldestIndex = i;
							break;
						}
					}
					if ( oldestIndex < 0 ) {
						oldestIndex = 0;
					}
					uiColorPickerCopiedColors[oldestIndex][0] = picker->currentColor[0];
					uiColorPickerCopiedColors[oldestIndex][1] = picker->currentColor[1];
					uiColorPickerCopiedColors[oldestIndex][2] = picker->currentColor[2];
					uiColorPickerCopiedColors[oldestIndex][3] = picker->currentColor[3];
					uiColorPickerCopiedColorsUsed[oldestIndex] = qtrue;
				}
				return qtrue;
			}

			pasteButtonX = copyButtonX + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;
			pasteButtonY = copyButtonY;
			pasteButtonLeft = pasteButtonX;
			pasteButtonRight = pasteButtonX + COLORPICKER_PASTE_BUTTON_WIDTH;
			pasteButtonTop = pasteButtonY;
			pasteButtonBottom = pasteButtonY + COLORPICKER_PASTE_BUTTON_HEIGHT;

			if (y >= pasteButtonTop && y < pasteButtonBottom &&
			    x >= pasteButtonLeft && x < pasteButtonRight) {
				if ( click ) {
					char clipboardData[64];
					char *source;

					source = NULL;
					trap_GetClipboardData( clipboardData, sizeof( clipboardData ) );
					if ( clipboardData[0] && UI_ColorPicker_ParseHexColor( clipboardData, picker->currentColor ) ) {
						source = clipboardData;
					} else if ( uiColorPickerClipboardBuffer[0] && UI_ColorPicker_ParseHexColor( uiColorPickerClipboardBuffer, picker->currentColor ) ) {
						source = uiColorPickerClipboardBuffer;
					}

					if ( source ) {
						Q_strncpyz( picker->hexBuffer, source, sizeof( picker->hexBuffer ) );
						UI_ColorPicker_UpdateFromHexString( picker );
					}
				}
				return qtrue;
			}

			{
				int lastUsedX;
				int lastUsedY;
				int copiedX;
				int copiedY;
				int i;
				int col;
				int row;
				int cellX;
				int cellY;
				int cellLeft;
				int cellRight;
				int cellTop;
				int cellBottom;

				lastUsedX = hexFieldX;
				lastUsedY = copyButtonY + COLORPICKER_COPY_BUTTON_SIZE + COLORPICKER_COPY_PASTE_GAP;

				for ( i = 0; i < COLORPICKER_LAST_USED_COUNT; i++ ) {
					col = i % COLORPICKER_LAST_USED_COLS;
					row = i / COLORPICKER_LAST_USED_COLS;
					cellX = lastUsedX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellY = lastUsedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellLeft = cellX;
					cellRight = cellX + COLORPICKER_SAVED_COLOR_SIZE;
					cellTop = cellY;
					cellBottom = cellY + COLORPICKER_SAVED_COLOR_SIZE;

					if ( y >= cellTop && y < cellBottom &&
					     x >= cellLeft && x < cellRight ) {
						if ( click && uiColorPickerLastUsedColorsUsed[i] ) {
							picker->currentColor[0] = uiColorPickerLastUsedColors[i][0];
							picker->currentColor[1] = uiColorPickerLastUsedColors[i][1];
							picker->currentColor[2] = uiColorPickerLastUsedColors[i][2];
							picker->currentColor[3] = uiColorPickerLastUsedColors[i][3];
							UI_ColorPicker_UpdateFromCurrent( picker );
						}
						return qtrue;
					}
				}

				copiedX = hexFieldX;
				copiedY = lastUsedY + COLORPICKER_LAST_USED_ROWS * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP ) + COLORPICKER_COPY_PASTE_GAP;
				copiedY += COLORPICKER_SECTION_HEADER_HEIGHT;

				for ( i = 0; i < COLORPICKER_COPIED_COUNT; i++ ) {
					col = i % COLORPICKER_COPIED_COLS;
					row = i / COLORPICKER_COPIED_COLS;
					cellX = copiedX + col * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellY = copiedY + row * ( COLORPICKER_SAVED_COLOR_SIZE + COLORPICKER_SAVED_COLORS_GAP );
					cellLeft = cellX;
					cellRight = cellX + COLORPICKER_SAVED_COLOR_SIZE;
					cellTop = cellY;
					cellBottom = cellY + COLORPICKER_SAVED_COLOR_SIZE;

					if ( y >= cellTop && y < cellBottom &&
					     x >= cellLeft && x < cellRight ) {
						if ( click && uiColorPickerCopiedColorsUsed[i] ) {
							picker->currentColor[0] = uiColorPickerCopiedColors[i][0];
							picker->currentColor[1] = uiColorPickerCopiedColors[i][1];
							picker->currentColor[2] = uiColorPickerCopiedColors[i][2];
							picker->currentColor[3] = uiColorPickerCopiedColors[i][3];
							UI_ColorPicker_UpdateFromCurrent( picker );
						}
						return qtrue;
					}
				}
			}
		}

		if (x < gridLeft || x >= gridLeft + gridWidth || y < gridTop || y >= gridTop + gridHeight) {
			return qtrue;
		}

		relX = x - gridLeft;
		relY = y - gridTop;
		col = relX / COLORPICKER_CELL_SIZE;
		row = relY / COLORPICKER_CELL_SIZE;

		if (col < 0 || col >= COLORPICKER_PALETTE_COLS || row < 0 || row >= COLORPICKER_PALETTE_ROWS) {
			return qtrue;
		}

		index = row * COLORPICKER_PALETTE_COLS + col;

		if (index >= 0 && index < paletteCount) {
			picker->currentColor[0] = palette[index][0];
			picker->currentColor[1] = palette[index][1];
			picker->currentColor[2] = palette[index][2];
			picker->currentColor[3] = palette[index][3];
			picker->selectedPaletteIndex = index;
			UI_ColorPicker_UpdateFromCurrent(picker);
			picker->focusedField = -1;
			
			/* If Alt is held and this is a click, immediately apply the color */
			if ( click && trap_Key_IsDown( K_ALT ) ) {
				UI_ColorPicker_Commit( picker );
			}
		}
	}

	return qtrue;
}
