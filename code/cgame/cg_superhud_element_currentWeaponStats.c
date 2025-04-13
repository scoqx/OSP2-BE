#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
} shudElementCWS_t;


void* CG_SHUDElementCWSCreate(const superhudConfig_t* config)
{
	shudElementCWS_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDTextMakeContext(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementCWSRoutine(void* context)
{
	shudElementCWS_t* element = (shudElementCWS_t*)context;
	customStats_t* ws = &CG_SHUDGetContext()->customStats;
	playerState_t* ps = &cg.snap->ps;

	int currentWeapon = ps->weapon;
	int updateInterval = 1000;
	static int lastUpdateTime = 0;
	static int lastHits = 0;
	char textBuffer[MAX_QPATH];
	int weaponIndex, i;
	qboolean trackAccuracy;

	// start from gauntlet, not from WP_NONE
	weaponIndex = currentWeapon - WP_GAUNTLET;

	if (weaponIndex < 0 || weaponIndex >= WP_NUM_WEAPONS)
	{
		return;
	}

	// Gauntlet - 1, Machinegun - 2, Shotgun - 4, etc...
	trackAccuracy = (cg_shud_currentWeapons.integer & (1 << weaponIndex)) != 0;

	if (currentWeapon == WP_RAILGUN)
	{
		updateInterval = 1500;
	}

	if (trackAccuracy &&
	        (ps->weaponstate == WEAPON_FIRING || ps->persistant[PERS_HITS] != lastHits) &&
	        (cg.time - lastUpdateTime >= updateInterval))
	{
		lastUpdateTime = cg.time;
		lastHits = ps->persistant[PERS_HITS];
		CG_SHUDRequestStatsInfo();
	}

	if (trackAccuracy && ws->stats[currentWeapon].shots > 0)
	{
		ws->lastTrackedWeapon = currentWeapon;
	}

	if (ws->lastTrackedWeapon < 0 || ws->stats[ws->lastTrackedWeapon].shots == 0)
	{
		return;
	}
	if (element->config.style.value == 1)
		Com_sprintf(textBuffer, sizeof(textBuffer), "^7%.1f%%", ws->stats[ws->lastTrackedWeapon].accuracy);
	else
		Com_sprintf(textBuffer, sizeof(textBuffer), "^7%.0f%%", ws->stats[ws->lastTrackedWeapon].accuracy);

	element->ctx.text = textBuffer;
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementCWSDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
