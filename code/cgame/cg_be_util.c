#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_BE_TEAM_INDICATOR	(1 << 0) // 1
#define CG_BE_OUTLINE			(1 << 1) // 2
#define CG_BE_WH				(1 << 2) // 4
#define CG_BE_ALT_BLOOD			(1 << 3) // 8
#define CG_BE_ALT_GRENADES      (1 << 4) // 16

qboolean CG_BE_Timer(int msec)
{
	static int lastTime = 0;
	int currentTime = cg.time;

	int diff = currentTime - lastTime;
	if (diff < 0)
	{
		lastTime = currentTime;
		return qfalse;
	}
	if (diff >= msec)
	{
		lastTime = currentTime;
		return qtrue;
	}
	return qfalse;
}

void CG_UpdateBeFeatures(void)
{
	qboolean changed = qfalse;
	int beFlags = 0;
	clientInfo_t *ci = &cgs.clientinfo[cg.clientNum];

	if (!BE_ENABLED)
	{
		beFlags = 0;
		changed = qtrue;
	}
	else
	{
		if (cg_teamIndicator.integer)
		{
			beFlags |= CG_BE_TEAM_INDICATOR;
			changed = qtrue;
		}
		if (cg_drawOutline.integer)
		{
			beFlags |= CG_BE_OUTLINE;
			changed = qtrue;
		}
		if (cg_friendsWallhack.integer)
		{
			beFlags |= CG_BE_WH;
			changed = qtrue;
		}
		if (cg_altBlood.integer)
		{
			beFlags |= CG_BE_ALT_BLOOD;
			changed = qtrue;
		}

		if (cg_altGrenades.integer == 2)
		{
			beFlags |= CG_BE_ALT_GRENADES;
			changed = qtrue;
		}
	}

	if (changed)
	{
		trap_Cvar_Set("be_features", va("%d", beFlags));
	}
}
