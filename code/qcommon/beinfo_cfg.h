/*
===========================================================================
beinfo_cfg.h — optional beconfigs/beinfo.cfg (AddVar overrides, ui + cgame blocks)
===========================================================================
*/

#ifndef BEINFO_CFG_H
#define BEINFO_CFG_H

typedef void (*beinfoBrightColorFn_t)(const char* classname, float r, float g, float b, float a);

void BeinfoCfg_Load(void);

const char* BeinfoCfg_GetStatusServerPrimary(void);
const char* BeinfoCfg_GetStatusServerAlternate(void);

/* Per-font TTF tune from ttfFonts { }. qtrue if name has an entry (scale/offset filled). */
qboolean BeinfoCfg_GetTtfFontTune(const char* name, float* scale, float* offset);

/* Per-font TTF shadow from ttfFonts { }. qtrue only if shadowOffset was set for name. */
qboolean BeinfoCfg_GetTtfFontShadow(const char* name, float* offX, float* offY);

#if defined(CGAME)
void BeinfoCfg_SetBrightColorHandler(beinfoBrightColorFn_t fn);
#endif

#endif /* BEINFO_CFG_H */
