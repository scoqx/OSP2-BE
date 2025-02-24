#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
    int data[MAX_WEAPONS][4];
} shudElementCWSData_t;

static shudElementCWSData_t CWS;

typedef struct {
    superhudConfig_t config;
    superhudTextContext_t ctx;
    superhudDrawContext_t ctxIcon;
    int lastTrackedWeapon;
    float lastAccuracy;
} shudElementCWS_t;


void CG_SHUDEventCWSRequest(void)
{
    cgs.osp.weaponStats.customWstatsCalled = qtrue;
    trap_SendClientCommand("getstatsinfo");
	// CG_Printf("^2Requesting weapon stats...^7\n");
}

void* CG_SHUDElementCWSCreate(const superhudConfig_t* config)
{
    shudElementCWS_t* element;

    SHUD_ELEMENT_INIT(element, config);

    CG_SHUDTextMakeContext(&element->config, &element->ctx);
    element->ctx.maxchars = MAX_QPATH;

    return element;
}

void CG_SHUDEventCWSParse(void)
{
    char args[1024];
    int i, weaponIndex, arg_cnt;

    for (i = 0; i < 24; ++i) // OSP_STATS, but we need only weapon mask
    {
        trap_Argv(i + 1, args, sizeof(args));
        statsInfo[i] = atoi(args);
    }

    if (statsInfo[OSP_STATS_WEAPON_MASK] == 0)
    {
        return;
    }

    arg_cnt = 23;

    for (weaponIndex = 0; weaponIndex < MAX_WEAPONS; weaponIndex++)
    {
        if (statsInfo[OSP_STATS_WEAPON_MASK] & (1 << weaponIndex))
        {
            trap_Argv(arg_cnt++, args, sizeof(args));
            CWS.data[weaponIndex][1] = atoi(args); // hits

            trap_Argv(arg_cnt++, args, sizeof(args));
            CWS.data[weaponIndex][0] = atoi(args); // shots

            trap_Argv(arg_cnt++, args, sizeof(args));
            CWS.data[weaponIndex][2] = atoi(args); // kills

            trap_Argv(arg_cnt++, args, sizeof(args));
            CWS.data[weaponIndex][3] = atoi(args); // deaths
        }
    }
}

void CG_SHUDElementCWSRoutine(void* context)
{
    shudElementCWS_t* element = (shudElementCWS_t*)context;
    playerState_t *ps = &cg.snap->ps;
    
    int currentWeapon = cg.snap->ps.weapon;
    int updateInterval = 1000;
    static int lastUpdateTime = 0;
    static int lastHits = 0;
    static float lastAccuracy = 0.0f;

    char textBuffer[MAX_QPATH];

    qboolean trackAccuracy = !(currentWeapon == WP_GAUNTLET ||
            currentWeapon == WP_SHOTGUN ||
        currentWeapon == WP_ROCKET_LAUNCHER ||
        currentWeapon == WP_GRENADE_LAUNCHER ||
        currentWeapon == WP_PLASMAGUN);

    if (currentWeapon <= 0 || currentWeapon >= MAX_WEAPONS)
    {
        return;
    }

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
        CG_SHUDEventCWSRequest();
    }

    if (trackAccuracy)
    {
        lastAccuracy = (CWS.data[currentWeapon][0] > 0) ? 
                        (100.0f * CWS.data[currentWeapon][1] / CWS.data[currentWeapon][0]) : 0.0f;
        CG_SHUDGetContext()->weaponStats.lastTrackedWeapon = currentWeapon;
    }

    Com_sprintf(textBuffer, sizeof(textBuffer), "^7%.1f%%", lastAccuracy);

    CG_SHUDFill(&element->config);
    element->ctx.text = textBuffer;
    CG_SHUDTextPrint(&element->config, &element->ctx);
    element->ctx.text = NULL;
}

void CG_SHUDElementCWSDestroy(void* context)
{
    if (context)
    {
        Z_Free(context);
    }
}
