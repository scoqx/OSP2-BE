#include "cg_local.h"
#include "../qcommon/qcommon.h"



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
	clientInfo_t* ci = &cgs.clientinfo[cg.clientNum];

	if (!BE_ENABLED)
	{
		beFlags = 0;
		changed = qtrue;
	}
	else
	{
		if (cg_friendsWallhack.integer)
		{
			beFlags |= CG_BE_TEAM_FOE_WH;
			changed = qtrue;
		}
		if (cg_altGrenades.integer == 2)
		{
			beFlags |= CG_BE_ALT_GRENADES;
			changed = qtrue;
		}
		if (cg_drawOutline.integer)
		{
			beFlags |= CG_BE_OUTLINE;
			changed = qtrue;
		}
		if (cgs.customModelSound)
		{
			beFlags |= CG_BE_MODELSOUND;
			changed = qtrue;
		}
		if (cg_markTeam.integer >= 0)
		{
			beFlags |= CG_BE_MARK_TEAMMATE;
			changed = qtrue;
		}
		if (cg_teamIndicator.integer)
		{
			beFlags |= CG_BE_TEAM_INDICATOR;
			changed = qtrue;
		}
		if (cg_altShadow.integer)
		{
			beFlags |= CG_BE_ALT_SHADOW;
			changed = qtrue;
		}
		if (cg_altBlood.integer)
		{
			beFlags |= CG_BE_ALT_BLOOD;
			changed = qtrue;
		}
		if (cg_enemyLightningColor.integer || cg_enemyLightningColor.string)
		{
			beFlags |= CG_BE_ENEMYLIGHTNING;
			changed = qtrue;
		}
	}

	if (changed)
	{
		trap_Cvar_Set("be_features", va("%d", beFlags));
	}
}

void BE_PrintDisabledFeatures(qboolean request)
{
	int value = cgs.be.disableFeatures;
	if (value == 0)
	{
		if (request)
		{
		CG_Printf("   [^9OSP2-BE^7] All features enabled.\n");
		}
	}
	else if (value == 4095) // all bits set
	{
		CG_Printf("   [^9OSP2-BE^7] All features disabled.\n");
	}
	else
	{
		char buffer[1024] = "   [^9OSP2-BE^7] Disabled features: ";
		qboolean first = qtrue;
		if (value & CG_BE_TEAM_FOE_WH) { Q_strcat(buffer, sizeof(buffer), first ? "Team wh" : ", team wh"); first = qfalse; }
		if (value & CG_BE_MODELSOUND) { Q_strcat(buffer, sizeof(buffer), first ? "Model sound" : ", model sound"); first = qfalse; }
		if (value & CG_BE_ALT_GRENADES) { Q_strcat(buffer, sizeof(buffer), first ? "Alt grenades" : ", alt grenades"); first = qfalse; }
		if (value & CG_BE_ENEMYLIGHTNING) { Q_strcat(buffer, sizeof(buffer), first ? "Enemy lightning" : ", enemy lightning"); first = qfalse; }
		if (value & CG_BE_MARK_TEAMMATE) { Q_strcat(buffer, sizeof(buffer), first ? "Mark teammate" : ", mark teammate"); first = qfalse; }
		if (value & CG_BE_ALT_SHADOW) { Q_strcat(buffer, sizeof(buffer), first ? "Alt shadow" : ", alt shadow"); first = qfalse; }
		if (value & CG_BE_ALT_BLOOD) { Q_strcat(buffer, sizeof(buffer), first ? "Alt blood" : ", alt blood"); first = qfalse; }
		if (value & CG_BE_OUTLINE) { Q_strcat(buffer, sizeof(buffer), first ? "Outline" : ", outline"); first = qfalse; }
		if (value & CG_BE_TEAM_INDICATOR) { Q_strcat(buffer, sizeof(buffer), first ? "team indicator" : ", team indicator"); first = qfalse; }
		if (value & CG_BE_DAMAGEINFO) { Q_strcat(buffer, sizeof(buffer), first ? "Damage count" : ", damage count"); first = qfalse; }
		if (value & CG_BE_FULLBRIGHT) { Q_strcat(buffer, sizeof(buffer), first ? "Fullbright" : ", fullbright"); first = qfalse; }
		CG_Printf("%s\n", buffer);
	}
}
