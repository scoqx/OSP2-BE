/*
===========================================================================
engine_fonts.h - FreeType atlas + cl_fontList (cgame + ui)
===========================================================================
*/

#ifndef ENG_FONTS_H
#define ENG_FONTS_H

#define ENG_FONTS_MAX_TTF         16
#define ENG_FONTS_FONT_INDEX_MAX  48
#define ENG_FONTS_SIZES           4

typedef qboolean (*EngFonts_GetValueFn)(const char* key, char* out, int outSize);
typedef int (*EngFonts_RegisterAtlasFn)(int cmd, const char* face, int pointSize, const char* alphabet, fontAtlasInfo_t* out);
typedef void (*EngFonts_CvarBufferFn)(const char* name, char* buf, int size);
typedef int (*EngFonts_FSOpenFn)(const char* qpath, fileHandle_t* f, fsMode_t mode);
typedef int (*EngFonts_FSReadFn)(void* buffer, int len, fileHandle_t f);
typedef void (*EngFonts_FSCloseFn)(fileHandle_t f);
typedef void (*EngFonts_PrintFn)(const char* msg);

/* Registry: add TTF slot; return qfalse if full / rejected. */
typedef qboolean (*EngFonts_AddTtfFn)(const char* bareName, const char* ttfPath);
typedef qboolean (*EngFonts_HasNameFn)(const char* bareName);
typedef qboolean (*EngFonts_IsTtfFn)(int index);
typedef const char* (*EngFonts_TtfPathFn)(int index);

typedef struct
{
	EngFonts_GetValueFn       getValue;
	EngFonts_RegisterAtlasFn  registerAtlas;
	EngFonts_CvarBufferFn     cvarBuffer;
	EngFonts_FSOpenFn         fsOpen;
	EngFonts_FSReadFn         fsRead;
	EngFonts_FSCloseFn        fsClose;
	EngFonts_PrintFn          print;
} EngFonts_Hooks;

void EngFonts_SetHooks(const EngFonts_Hooks* hooks);
void EngFonts_Invalidate(void);

/* Probe trap_R_RegisterFontAtlas_Q3E. Returns qtrue if available. */
qboolean EngFonts_Probe(void);
qboolean EngFonts_IsAvailable(void);

/*
Append TTF faces into module registry:
1) fonts.cfg TTF lines in file order (must appear in cl_fontList)
2) remaining cl_fontList faces
Caller should reset fonts_num to bitmap count before calling.
*/
void EngFonts_AppendFaces(EngFonts_AddTtfFn addTtf, EngFonts_HasNameFn hasName, int maxFonts, int fontsNum);

/* Map registry TTF indices to dense slots; bake sizes 8/16/32/64. */
void EngFonts_Preload(int fontCount, EngFonts_IsTtfFn isTtf, EngFonts_TtfPathFn ttfPath);

const fontAtlasInfo_t* EngFonts_Info(int fontIndex, float charHeight);

qboolean EngFonts_FontCfgIsTtfEntry(const char* name, const char* path);

#endif
