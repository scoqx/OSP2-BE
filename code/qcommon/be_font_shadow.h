/*
===========================================================================
be_font_shadow.h — shared bitmap/TTF text shadow offset cache (cgame + ui)
===========================================================================
*/

#ifndef BE_FONT_SHADOW_H
#define BE_FONT_SHADOW_H

/* Hotpath: read directly (no CALL). Defaults 0.1 0.1. */
extern float be_fontShadowOffX;
extern float be_fontShadowOffY;
extern float be_ttfShadowOffX;
extern float be_ttfShadowOffY;

void BeFontShadow_UpdateBitmap(const char* str);
void BeFontShadow_UpdateTtf(const char* str);

#endif /* BE_FONT_SHADOW_H */
