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

#if defined(CGAME)
void BeinfoCfg_SetBrightColorHandler(beinfoBrightColorFn_t fn);
#endif

#endif /* BEINFO_CFG_H */
