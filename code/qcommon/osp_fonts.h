/*
===========================================================================
osp_fonts.h — built-in HUD/UI font registry (cgame + ui)
===========================================================================
*/

#ifndef OSP_FONTS_H
#define OSP_FONTS_H

typedef struct
{
	const char* name;
	const char* cfgPath;
} ospBuiltinFont_t;

#define OSP_BUILTIN_FONT_COUNT 25

extern const ospBuiltinFont_t ospBuiltinFonts[OSP_BUILTIN_FONT_COUNT];

#endif /* OSP_FONTS_H */
