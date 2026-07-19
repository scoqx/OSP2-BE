/*
===========================================================================
be_font_shadow.c — parse be_fontShadowOffset / be_ttfShadowOffset (cgame + ui)
===========================================================================
*/

#include "q_shared.h"
#include "be_font_shadow.h"

#define BE_FONT_SHADOW_OFF_MIN -0.25f
#define BE_FONT_SHADOW_OFF_MAX  0.25f

float be_fontShadowOffX = 0.1f;
float be_fontShadowOffY = 0.1f;
float be_ttfShadowOffX = 0.1f;
float be_ttfShadowOffY = 0.1f;

/*
================
BeFontShadow_Parse

Parse "x y" into outX/outY. One token copies to both. Empty/invalid → 0.1 0.1.
Clamped to BE_FONT_SHADOW_OFF_MIN..MAX.
================
*/
static void BeFontShadow_Parse(const char* str, float* outX, float* outY)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;

	if (!str || str[0] == '\0')
	{
		*outX = 0.1f;
		*outY = 0.1f;
		return;
	}

	Q_strncpyz(buffer, str, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1)
	{
		*outX = 0.1f;
		*outY = 0.1f;
		return;
	}

	*outX = Com_Clamp(BE_FONT_SHADOW_OFF_MIN, BE_FONT_SHADOW_OFF_MAX, (float)atof(token1));
	if (token2)
	{
		*outY = Com_Clamp(BE_FONT_SHADOW_OFF_MIN, BE_FONT_SHADOW_OFF_MAX, (float)atof(token2));
	}
	else
	{
		*outY = *outX;
	}
}

void BeFontShadow_UpdateBitmap(const char* str)
{
	BeFontShadow_Parse(str, &be_fontShadowOffX, &be_fontShadowOffY);
}

void BeFontShadow_UpdateTtf(const char* str)
{
	BeFontShadow_Parse(str, &be_ttfShadowOffX, &be_ttfShadowOffY);
}
