/*
===========================================================================
ui_gameinfo_cfg.h — optional gameinfo.cfg (AddCvar overrides)
===========================================================================
*/

#ifndef UI_GAMEINFO_CFG_H
#define UI_GAMEINFO_CFG_H

void UI_GameinfoCfg_Load(void);
const char* UI_GameinfoCfg_GetStatusServerPrimary(void);
const char* UI_GameinfoCfg_GetStatusServerAlternate(void);

#endif /* UI_GAMEINFO_CFG_H */
