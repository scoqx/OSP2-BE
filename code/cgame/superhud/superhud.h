#ifndef CG_SUPERHUD_H
#define CG_SUPERHUD_H

#include "../cg_local.h"

#ifdef __cplusplus
extern "C" {
#endif

void CG_SHUDSetConfig(const char* filename);
void CG_SHUDLoadConfig(qboolean silent);
void CG_SHUDRoutine(void);

void CG_SHUDEventFrag(const char* message);
void CG_SHUDEventKillConfirm(qboolean confirmed);
void CG_SHUDEventChat(const char* message, int senderClientNum);
void CG_SHUDEventTeamChat(const char* message, int senderClientNum);
void CG_SHUDEventObituaries(int attacker, int target, int mod, qboolean unfrozen);
void CG_SHUDEventTeamTask(int sender, int taskType, qboolean isPickup);
void CG_SHUDRefreshSpectatorItemSpawnsTeamTables(void);
qboolean CG_SHUDHasSpectatorItemSpawnsElements(void);
qboolean CG_SHUDHasSpectatorsElements(void);
qboolean CG_SHUDHasTeamRedBlueElements(void);
void CG_SHUDEventSpectatorItemPickup(int pickerClient, int group);
void CG_SHUDEventTempAccuracy(int weapon, float accuracy);

void CG_SHUDChatRestore(void);

qboolean CG_SHUDEditorIsActive(void);
qboolean CG_SHUDEditorShowAll(void);
void CG_SHUDEditorToggle_f(void);
void CG_SHUDEditorSave_f(void);
void CG_SHUDEditorDraw(void);
void CG_SHUDEditorKeyEvent(int key, qboolean down);
void CG_SHUDEditorMouseEvent(int dx, int dy);
void CG_SHUDEditorShutdown(void);
void CG_SHUDEditorOnConfigReloaded(void);
void CG_SHUDEditorPumpPreview(void);
qboolean CG_SHUDSaveConfig(const char* filename);

#ifdef __cplusplus
}
#endif

#endif
