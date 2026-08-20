#include "superhud.h"
#include "cg_local.h"
#include "../qcommon/be_font_shadow.h"

int global_handicap; // unused?
//	void         (*onChanged)(cvarTable_t *cvart);


void CG_ParseCvarTwoStrings(const cvarTable_t* cvart, char* out1, char* out2, int outSize)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1, *token2;

	if (!cvart || !cvart->vmCvar) return;
	if (cvart->vmCvar->string[0] == '\0') return;

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1 || !token2) return;

	Q_strncpyz(out1, token1, outSize);
	Q_strncpyz(out2, token2, outSize);
}

void CG_ParseCvarTwoInts(const cvarTable_t* cvart, int* out1, int* out2)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1, *token2;

	if (!cvart || !cvart->vmCvar) return;
	if (cvart->vmCvar->string[0] == '\0') return;

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1 || !token2) return;

	*out1 = atoi(token1);
	*out2 = atoi(token2);
}

void CG_ParseCvarTwoFloats(const cvarTable_t* cvart, float* out1, float* out2)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1, *token2;

	if (!cvart || !cvart->vmCvar) return;
	if (cvart->vmCvar->string[0] == '\0') return;

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1) return;

	*out1 = (float)atof(token1);
	if (token2)
	{
		*out2 = (float)atof(token2);
	}
	else
	{
		*out2 = *out1;
	}
}

void CG_LocalEventCvarChanged_cg_drawTimer(cvarTable_t* cvart)
{
	CG_OSPEnforceCustomClientSettings();
}

void CG_LocalEventCvarChanged_cg_drawTeamOverlay(cvarTable_t* cvart)
{
	/* UnFreeZe: teamoverlay2 always 1; OSP/BE: keep teamoverlay for tinfo */
	if (cgs.fpsclassico)
	{
		trap_Cvar_Set("teamoverlay2", "1");
	}
	else
	{
		trap_Cvar_Set("teamoverlay", "1");
	}
}

void CG_LocalEventCvarChanged_cl_maxpackets(cvarTable_t* cvart)
{
	if (cgs.osp.serverConfigMinimumMaxpackets)
	{
		if (cl_maxpackets.integer < cgs.osp.serverConfigMinimumMaxpackets)
		{
			CG_Printf("^3Server-imposed minimum maxpackets set to %d.  Adjusting...\n", cgs.osp.serverConfigMinimumMaxpackets);
			trap_Cvar_Set("cl_maxpackets", va("%d", cgs.osp.serverConfigMinimumMaxpackets));
		}
	}

	if (cgs.osp.serverConfigMaximumMaxpackets)
	{
		if (cl_maxpackets.integer > cgs.osp.serverConfigMaximumMaxpackets)
		{
			CG_Printf("^3Server-imposed maximum maxpackets set to %d.  Adjusting...\n", cgs.osp.serverConfigMaximumMaxpackets);
			trap_Cvar_Set("cl_maxpackets", va("%d", cgs.osp.serverConfigMaximumMaxpackets));
		}
	}

	if (cgs.osp.nextCvarsUpdateTime == 0)
	{
		cgs.osp.nextCvarsUpdateTime = (cg.time + 1200);
	}
}

void CG_LocalEventCvarChanged_cl_timenudge(cvarTable_t* cvart)
{
	if (cgs.ratFlags & RAT_NOTIMENUDGE)
	{
		if (cl_timenudge.integer != 0)
		{
			CG_Printf("^3Server-imposed timenudge disabled.  Adjusting...\n");
			trap_Cvar_Set("cl_timenudge", "0");
		}
		if (cgs.osp.nextCvarsUpdateTime == 0)
		{
			cgs.osp.nextCvarsUpdateTime = (cg.time + 1200);
		}
		return;
	}
	if (cgs.osp.serverConfigMinimumTimenudge)
	{
		if (cgs.osp.serverConfigMinimumTimenudge > cl_timenudge.integer)
		{
			CG_Printf("^3Server-imposed minimum timenudge set to %d.  Adjusting...\n", cgs.osp.serverConfigMinimumTimenudge);
			trap_Cvar_Set("cl_timenudge", va("%d", cgs.osp.serverConfigMinimumTimenudge));
		}
		if (cgs.osp.serverConfigMaximumTimenudge)
		{
			if (cgs.osp.serverConfigMaximumTimenudge < cl_timenudge.integer)
			{
				CG_Printf("^3Server-imposed maximum timenudge set to %d.  Adjusting...\n", cgs.osp.serverConfigMaximumTimenudge);
				trap_Cvar_Set("cl_timenudge", va("%d", cgs.osp.serverConfigMaximumTimenudge));
			}
		}
		if (cgs.osp.nextCvarsUpdateTime == 0)
		{
			cgs.osp.nextCvarsUpdateTime = (cg.time + 1200);
		}
	}
}

void CG_LocalEventCvarChanged_com_maxfps(cvarTable_t* cvart)
{
	CG_OSP2BEOnComMaxfpsChanged(cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_ch_recordMessage(cvarTable_t* cvart)
{
	if (cg.snap)
	{
		trap_Cvar_Set("ui_recordSPDemo", ch_recordMessage.integer <= 0 ? "0" : "1");
	}
}

void CG_LocalEventCvarChanged_cg_followpowerup(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}
void CG_LocalEventCvarChanged_cg_followkiller(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}
void CG_LocalEventCvarChanged_cg_followviewcam(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}
void CG_LocalEventCvarChanged_cg_autoAction(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}

void CG_LocalEventCvarChanged_cg_autoHop(cvarTable_t* cvart)
{
	if (cgs.beAutohop == 0)
	{
		// CG_PrintOnce("^1Auto-hop not supported\n");
		trap_Cvar_Set("cg_autoHop", "0");
	}
	else if (cgs.beAutohop == 1)
	{
		// CG_PrintOnce("^3Auto-hop controlled by server\n");
		trap_Cvar_Set("cg_autoHop", "1");
	}
}
void CG_LocalEventCvarChanged_snaps(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}
void CG_LocalEventCvarChanged_cg_useScreenShotJPEG(cvarTable_t* cvart)
{
	if (cgs.osp.nextCvarsUpdateTime)
	{
		cgs.osp.nextCvarsUpdateTime = cg.time + 1200;
	}
}

void CG_LocalEventCvarChanged_r_lodCurveError(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_r_subdivisions(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_r_znear(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_cg_trueLightning(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_r_shownormals(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_r_showtris(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
void CG_LocalEventCvarChanged_cg_shadows(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}
// void CG_LocalEventCvarChanged_r_fullbright(cvarTable_t* cvart)
// {
//  CG_OSPCvarsRestrictValues();
// }


void CG_LocalEventCvarChanged_cg_swapSkins(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_enemyColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_enemyModel(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_teamColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_teamModel(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}


void CG_LocalEventCvarChanged_handicap(cvarTable_t* cvart)
{
	if (!cg.warmup)
	{
		if (cgs.gametype != GT_FFA && cgs.gametype != GT_SINGLE_PLAYER && handicap.integer < 5)
		{
			global_handicap = 100;
			trap_Cvar_Set("handicap", va("%d", global_handicap));
		}
	}
	global_handicap = handicap.integer;
}

void CG_LocalEventCvarChanged_s_ambient(cvarTable_t* cvart)
{
	trap_SendConsoleCommand("vid_restart;\n");
}

void CG_LocalEventCvarChanged_pmove_fixed(cvarTable_t* cvart)
{
	if (cgs.osp.allow_pmove == 0 && pmove_fixed.integer)
	{
		CG_PrintOnce("^3Enhanced client sampling has been disabled on this server.\n");
		trap_Cvar_Set("pmove_fixed", "0");
	}
}

void CG_LocalEventCvarChanged_phy_movetype(cvarTable_t* cvart)
{
	CG_PmovePhysicsUpdate();
}

void CG_LocalEventCvarChanged_cg_hitSounds(cvarTable_t* cvart)
{
	if (!cgs.damageInfoAllowed && cg_hitSounds.integer)
	{
		CG_Printf("^3Damage info has been disabled on this server.\n");
		// trap_Cvar_Set("cg_hitSounds", "0");
	}
}

void CG_LocalEventCvarChanged_cg_playerModelColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_playerRailColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_playerFrozenColor(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_teamModelColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_teamRailColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_teamFrozenColor(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_enemyModelColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_enemyRailColors(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_enemyFrozenColor(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_fragSound(cvarTable_t* cvart)
{
	/* touch fragsound to reload it */
	(void)CG_GetFragSound();
}

void CG_LocalEventCvarChanged_ch_file(cvarTable_t* cvart)
{
	if (cvart->vmCvar->string[0])
	{
		CG_SHUDLoadConfig(qfalse);
	}
}

void CG_LocalEventCvarChanged_cg_shud(cvarTable_t* cvart)
{
	if (cvart->vmCvar->integer && ch_file.string[0])
	{
		CG_SHUDLoadConfig(qfalse);
	}
}

void CG_LocalEventCvarChanged_cg_shudStyle(cvarTable_t* cvart)
{
	if (cvart->vmCvar->integer < 1 || cvart->vmCvar->integer > 4)
	{
		trap_Cvar_Set(cvart->cvarName, "1");
		return;
	}

	if (cg_shud.integer && ch_file.string[0])
	{
		CG_SHUDLoadConfig(qtrue);
	}
}

static void CG_LocalEventCvarParseColor(cvarTable_t* cvart, vec4_t color)
{
	if (!CG_ParseColorStr(cvart->vmCvar->string, color))
	{
		Com_Printf("^1Color is wrong, default value used\n");
		CG_ParseColorStr(cvart->defaultString, color);
	}
}

void CG_LocalEventCvarChanged_ch_crosshairColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, crosshairData.color);
}

void CG_LocalEventCvarChanged_ch_crosshairActionColor(cvarTable_t* cvart)
{
	if (Q_stricmp(cvart->cvarName, "ch_crosshairActionColor") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.actionColor);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairActionColorLow") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.actionColorLow);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairActionColorMid") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.actionColorMid);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairActionColorHigh") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.actionColorHigh);
	}
}

void CG_LocalEventCvarChanged_ch_crosshairDecorColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, crosshairData.decorColor);
}

void CG_LocalEventCvarChanged_ch_crosshairDecorActionColor(cvarTable_t* cvart)
{
	if (Q_stricmp(cvart->cvarName, "ch_crosshairDecorActionColor") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.decorActionColor);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairDecorActionColorLow") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.decorActionColorLow);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairDecorActionColorMid") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.decorActionColorMid);
	}
	if (Q_stricmp(cvart->cvarName, "ch_crosshairDecorActionColorHigh") == 0)
	{
		CG_LocalEventCvarParseColor(cvart, crosshairData.decorActionColorHigh);
	}
}


void CG_LocalEventCvarChanged_ch_crosshairDecorOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_ch_crosshairOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_ch_crosshairActionScale(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.1, 10, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_ch_crosshairDecorActionScale(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.1, 10, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_ch_crosshairActionTime(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(50, 1000, cvart->vmCvar->value);
	cvart->vmCvar->integer = Com_Clamp(50, 1000, cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_ch_crosshairDecorActionTime(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(50, 1000, cvart->vmCvar->value);
	cvart->vmCvar->integer = Com_Clamp(50, 1000, cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_cg_damageIndicatorOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_bobup(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 0.02, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_dlightGauntlet(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_GAUNTLET);
}

void CG_LocalEventCvarChanged_cg_dlightMG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_MACHINEGUN);
}

void CG_LocalEventCvarChanged_cg_dlightSG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_SHOTGUN);
}

void CG_LocalEventCvarChanged_cg_dlightGL(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_GRENADE_LAUNCHER);
}

void CG_LocalEventCvarChanged_cg_dlightRL(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_ROCKET_LAUNCHER);
}

void CG_LocalEventCvarChanged_cg_dlightLG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_LIGHTNING);
}

void CG_LocalEventCvarChanged_cg_dlightRG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_RAILGUN);
}

void CG_LocalEventCvarChanged_cg_dlightPG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_PLASMAGUN);
}

void CG_LocalEventCvarChanged_cg_dlightBFG(cvarTable_t* cvart)
{
	CG_UpdateWeaponDlightColor(WP_BFG);
}

void CG_LocalEventCvarChanged_cg_conObituaries(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_enemyLightningColor(cvarTable_t* cvart)
{
	CG_RebuildPlayerColors();
	CG_UpdateBeFeatures();
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_hitBoxColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beHitBoxColor);
}

void CG_LocalEventCvarChanged_cg_damageFrameSize(cvarTable_t* cvart)
{
	if (cg_damageDrawFrame.integer == 1)
	{
		cvart->vmCvar->value = Com_Clamp(2, 512, cvart->vmCvar->value);
	}
	else
		cvart->vmCvar->value = Com_Clamp(1, 512, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_damageFrameOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.1f, 1.0f, cvart->vmCvar->value);

}
void CG_LocalEventCvarChanged_cg_damageDrawFrame(cvarTable_t* cvart)
{
	CG_CvarTouch("cg_damageFrameSize");
}

void CG_LocalEventCvarChanged_cg_enemyOutlineColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beEnemyOutlineColor);
}

void CG_LocalEventCvarChanged_cg_teamOutlineColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beTeamOutlineColor);
}

void CG_LocalEventCvarChanged_cg_altBloodColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beAltBloodColor);
}

void CG_LocalEventCvarChanged_cg_railRingsRadius(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 16, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_railRingsRotation(cvarTable_t* cvart)
{
	cvart->vmCvar->integer = Com_Clamp(0, 2, cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_cg_railRingsSpacing(cvarTable_t* cvart)
{
	cvart->vmCvar->integer = Com_Clamp(4, 128, cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_cg_railRingsSize(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 4, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_altShadowColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beAltShadowColor);
}
void CG_LocalEventCvarChanged_cg_enemyAltShadowColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.enemyAltShadowColorEnabled = qfalse;
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beEnemyAltShadowColor))
	{
		cg.enemyAltShadowColorEnabled = qtrue;
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.enemyAltShadowColorEnabled = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
	}
}
void CG_LocalEventCvarChanged_cg_teamIndicatorAdjust(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.5f, 2, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.bePlayerIndicatorColor);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorBgColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.bePlayerIndicatorBgColor);
}
void CG_LocalEventCvarChanged_cg_emojiListBgColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.beEmojiListBgColor);
}
void CG_LocalEventCvarChanged_cg_emojiListOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorBgOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_gunX(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-100, 100, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_gunY(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-100, 100, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_gunZ(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-100, 100, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_teamIndicatorOffset(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-16, 32, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorMaxLength(cvarTable_t* cvart)
{
	cvart->vmCvar->integer = Com_Clamp(1, MAX_QPATH, cvart->vmCvar->integer);
}

void CG_LocalEventCvarChanged_cg_teamIndicatorFade(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_scoreboardFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_scoreboardFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_teamIndicatorFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_teamIndicatorFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_teamIndicatorFontSize(cvarTable_t* cvart)
{
	if (!cvart || !cvart->vmCvar || cvart->vmCvar->string[0] == '\0')
	{
		cgs.bePlayerIndicatorFontSize[0] = 0.0f;
		cgs.bePlayerIndicatorFontSize[1] = 0.0f;
		return;
	}
	CG_ParseCvarTwoFloats(cvart, &cgs.bePlayerIndicatorFontSize[0], &cgs.bePlayerIndicatorFontSize[1]);
}
void CG_LocalEventCvarChanged_cg_accuracyFontSize(cvarTable_t* cvart)
{
	if (!cvart || !cvart->vmCvar || cvart->vmCvar->string[0] == '\0')
	{
		cgs.beAccuracyFontSize[0] = 0.0f;
		cgs.beAccuracyFontSize[1] = 0.0f;
		return;
	}
	CG_ParseCvarTwoFloats(cvart, &cgs.beAccuracyFontSize[0], &cgs.beAccuracyFontSize[1]);
}
void CG_LocalEventCvarChanged_be_fontShadowOffset(cvarTable_t* cvart)
{
	(void)cvart;
	BeFontShadow_UpdateBitmap(be_fontShadowOffset.string);
}
void CG_LocalEventCvarChanged_be_ttfShadowOffset(cvarTable_t* cvart)
{
	(void)cvart;
	BeFontShadow_UpdateTtf(be_ttfShadowOffset.string);
}
void CG_LocalEventCvarChanged_cg_teamTaskFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_teamTaskFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_centerMessagesFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_centerMessagesFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_be_infoVersionFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("be_infoVersionFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_emojiListFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_emojiListFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_chatWindowFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_chatWindowFont", "2");
	}
	CG_UpdateFontIndices();
}
void CG_LocalEventCvarChanged_cg_chatWindowBgColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.beChatWindowBgColor);
}
void CG_LocalEventCvarChanged_cg_chatWindowBgOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_chatWindowFontSize(cvarTable_t* cvart)
{
	CG_BEChat_UpdateFontSizeFromCvar();
}
void CG_LocalEventCvarChanged_cg_altGrenadesColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beAltGrenadesColor);
}
void CG_LocalEventCvarChanged_cg_enemyGrenadesColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beEnemyGrenadesColor);
}

void CG_LocalEventCvarChanged_cg_plasmaColor(cvarTable_t* cvart)
{
	if (cvart->vmCvar->string[0])
	{
		CG_LocalEventCvarParseColor(cvart, cg.bePlasmaColor);
	}
	CG_UpdateBeFeatures();
}

void CG_LocalEventCvarChanged_cg_enemyPlasmaColor(cvarTable_t* cvart)
{
	if (cvart->vmCvar->string[0])
	{
		CG_LocalEventCvarParseColor(cvart, cg.beEnemyPlasmaColor);
	}
	CG_UpdateBeFeatures();
}
void CG_LocalEventCvarChanged_cg_battleSuitColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.battleSuitColorEnabled = qfalse;
		cg.battleSuitCustomColor = qfalse;
		CG_UnregisterItemVisualForPowerup(PW_BATTLESUIT);
		CG_UpdateBrightItemsDynamicColors();
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beBattleSuitColor))
	{
		cg.battleSuitColorEnabled = qtrue;
		cg.battleSuitCustomColor = qtrue;
		CG_UnregisterItemVisualForPowerup(PW_BATTLESUIT);
		CG_UpdateBrightItemsDynamicColors();
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.battleSuitColorEnabled = qfalse;
		cg.battleSuitCustomColor = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
		CG_UnregisterItemVisualForPowerup(PW_BATTLESUIT);
		CG_UpdateBrightItemsDynamicColors();
	}
}
void CG_LocalEventCvarChanged_cg_enemyBattleSuitColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.enemyBattleSuitColorEnabled = qfalse;
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beEnemyBattleSuitColor))
	{
		cg.enemyBattleSuitColorEnabled = qtrue;
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.enemyBattleSuitColorEnabled = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
	}
}
void CG_LocalEventCvarChanged_cg_quadDamageColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.quadDamageColorEnabled = qfalse;
		cg.quadDamageCustomColor = qfalse;
		CG_UnregisterItemVisualForPowerup(PW_QUAD);
		CG_UpdateBrightItemsDynamicColors();
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beQuadDamageColor))
	{
		cg.quadDamageColorEnabled = qtrue;
		cg.quadDamageCustomColor = qtrue;
		CG_UnregisterItemVisualForPowerup(PW_QUAD);
		CG_UpdateBrightItemsDynamicColors();
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.quadDamageColorEnabled = qfalse;
		cg.quadDamageCustomColor = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
		CG_UnregisterItemVisualForPowerup(PW_QUAD);
		CG_UpdateBrightItemsDynamicColors();
	}
}
void CG_LocalEventCvarChanged_cg_enemyQuadDamageColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.enemyQuadDamageColorEnabled = qfalse;
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beEnemyQuadDamageColor))
	{
		cg.enemyQuadDamageColorEnabled = qtrue;
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.enemyQuadDamageColorEnabled = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
	}
}
void CG_LocalEventCvarChanged_cg_kcEnemyColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.kcEnemyColorEnabled = qfalse;
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beKcEnemyColor))
	{
		cg.kcEnemyColorEnabled = qtrue;
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.kcEnemyColorEnabled = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
	}
}
void CG_LocalEventCvarChanged_cg_kcTeamColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.kcTeamColorEnabled = qfalse;
		return;
	}
	if (CG_ParseColorStr(cvart->vmCvar->string, cg.beKcTeamColor))
	{
		cg.kcTeamColorEnabled = qtrue;
	}
	else
	{
		Com_Printf("^1Color is wrong, default value used\n");
		cg.kcTeamColorEnabled = qfalse;
		trap_Cvar_Set(cvart->cvarName, cvart->defaultString);
	}
}
void CG_LocalEventCvarChanged_cg_fragEffectColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		Vector4Set(cg.beFragEffectColor, 1.0f, 1.0f, 1.0f, 1.0f);
		return;
	}
	CG_LocalEventCvarParseColor(cvart, cg.beFragEffectColor);
}
void CG_LocalEventCvarChanged_cg_healthColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beHealthColor);
}
void CG_LocalEventCvarChanged_cg_healthLowColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beHealthLowColor);
}
void CG_LocalEventCvarChanged_cg_healthMidColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beHealthMidColor);
}
void CG_LocalEventCvarChanged_cg_redTeamColor(cvarTable_t* cvart)
{
	team_t realTeam;
	/* Store raw red team color from cvar */
	CG_LocalEventCvarParseColor(cvart, cgs.baseRedTeamColor);
	CG_InvalidateVisualTeamColorsCache();
	/* Recompute effective colors with cg_myTeam mapping */
	/* Determine real team from clientInfo (most reliable source) */
	if (cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS)
	{
		const clientInfo_t* ourClient = &cgs.clientinfo[cg.clientNum];

		if (CG_OSPIsGameTypeCA(cgs.gametype))
		{
			realTeam = ourClient->rt;
		}
		else
		{
			realTeam = ourClient->team;
		}
	}
	else
	{
		realTeam = TEAM_RED;
	}
	CG_UpdateVisualTeamColors(realTeam);
	CG_UpdateBrightItemsDynamicColors();
}
void CG_LocalEventCvarChanged_cg_blueTeamColor(cvarTable_t* cvart)
{
	team_t realTeam;
	/* Store raw blue team color from cvar */
	CG_LocalEventCvarParseColor(cvart, cgs.baseBlueTeamColor);
	CG_InvalidateVisualTeamColorsCache();
	/* Recompute effective colors with cg_myTeam mapping */
	/* Determine real team from clientInfo (most reliable source) */
	if (cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS)
	{
		const clientInfo_t* ourClient = &cgs.clientinfo[cg.clientNum];

		if (CG_OSPIsGameTypeCA(cgs.gametype))
		{
			realTeam = ourClient->rt;
		}
		else
		{
			realTeam = ourClient->team;
		}
	}
	else
	{
		realTeam = TEAM_RED;
	}
	CG_UpdateVisualTeamColors(realTeam);
	CG_UpdateBrightItemsDynamicColors();
}
void CG_LocalEventCvarChanged_cg_myTeam(cvarTable_t* cvart)
{
	const char* str;
	team_t realTeam;

	/* Accept Red/Blue as aliases for 1/2 */
	str = cvart->vmCvar->string;
	if (Q_stricmp(str, "Red") == 0)
		trap_Cvar_Set("cg_myTeam", "1");
	else if (Q_stricmp(str, "Blue") == 0)
		trap_Cvar_Set("cg_myTeam", "2");

	/* Recompute effective colors when cg_myTeam changes */
	/* Determine real team from clientInfo (most reliable source) */
	if (cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS)
	{
		const clientInfo_t* ourClient = &cgs.clientinfo[cg.clientNum];

		if (CG_OSPIsGameTypeCA(cgs.gametype))
		{
			realTeam = ourClient->rt;
		}
		else
		{
			realTeam = ourClient->team;
		}
	}
	else
	{
		realTeam = TEAM_RED;
	}

	CG_UpdateVisualTeamColors(realTeam);
	CG_ReloadColors();
	CG_UpdateBrightItemsDynamicColors();
}



void CG_LocalEventCvarChanged_cg_accuracyFont(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_accuracyFont", "0");
	}
	CG_UpdateFontIndices();
}

void CG_LocalEventCvarChanged_ReloadTeamTaskWheel(cvarTable_t* cvart)
{
	/* Reset ttwheel elements count to force re-initialization */
	cg.teamMarkers.ttwheelElementCount = 0;
	/* Parse and set elements from cvar */
	CG_SelwheelParseTeamTaskWheel();
}


void CG_LocalEventCvarChanged_cg_markTeam(cvarTable_t* cvart)
{
	int clientNum;
	const char* str = cvart->vmCvar->string;

	CG_UpdateBeFeatures();
	if (CG_BE_FEATURE_ENABLED(CG_BE_MARK_TEAMMATE) && cg_markTeam.integer != 1)
	{
		memset(cgs.beMarkedTeam, qfalse, sizeof(cgs.beMarkedTeam));

		if (Q_stricmp(str, "-1") == 0)
		{
			return;
		}

		while (*str)
		{
			while (*str == ' ')
			{
				str++;
			}

			if (!*str)
				break;

			clientNum = atoi(str);

			if (clientNum >= 0 && clientNum < MAX_CLIENTS)
			{
				cgs.beMarkedTeam[clientNum] = qtrue;
			}
			else
			{
				CG_Printf("cg_markTeam: invalid clientNum %d\n", clientNum);
			}

			while (*str && *str != ' ')
			{
				str++;
			}
		}
	}
	else
	{
		trap_Cvar_Set("cg_markTeam", "-1");
		/* Only say "disabled on this server" when server actually forbids it; not when feature is off due to cvar -1 or be_enabled */
		if (cgs.beDisableFeatures & CG_BE_MARK_TEAMMATE)
			CG_PrintOnce("^2cg_markTeam^7 ^1disabled^7 on this server!\n");
	}
}

void CG_LocalEventCvarChanged_cg_markTeamColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cg.beMarkedTeamColor);
}

void CG_LocalEventCvarChanged_cg_customSound(cvarTable_t* cvart)
{
	char mySoundModel[MAX_QPATH];
	char teamSoundModel[MAX_QPATH];
	char enemySoundModel[MAX_QPATH];

	/* Sync cgs.customModelSound from cvars so CG_UpdateBeFeatures sees current "wants custom sound" state */
	trap_Cvar_VariableStringBuffer("cg_mySound", mySoundModel, sizeof(mySoundModel));
	trap_Cvar_VariableStringBuffer("cg_teamSound", teamSoundModel, sizeof(teamSoundModel));
	trap_Cvar_VariableStringBuffer("cg_enemySound", enemySoundModel, sizeof(enemySoundModel));
	cgs.customModelSound = (mySoundModel[0] != '\0' || teamSoundModel[0] != '\0' || enemySoundModel[0] != '\0') ? qtrue : qfalse;

	CG_UpdateBeFeatures();

	if (CG_BE_FEATURE_ENABLED(CG_BE_MODELSOUND))
	{
		CG_LoadForcedSounds();
	}
	if (cgs.beDisableFeatures & CG_BE_MODELSOUND)
	{
		CG_PrintOnce("^7Custom sound ^1disabled ^7on this server!");
	}
}

void CG_LocalEventCvarChanged_cg_scoreboardRtColors(cvarTable_t* cvart)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;
	char* token3;

	if (!cvart || !cvart->vmCvar)
	{
		CG_Printf("^1Invalid cvar pointer\n");
		return;
	}

	if (cvart->vmCvar->string[0] == '\0')
	{
		customScoreboardColorIsSet_red = 0;
		return;
	}

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));

	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");
	token3 = Q_strtok(NULL, " \t");

	if (!token1 || !token2)
	{
		CG_Printf("^1Invalid value: expected at least two color values (e.g. \"red blue\")\n");
		customScoreboardColorIsSet_red = 0;
		return;
	}

	/* Parse colors - CG_ParseColorStr already handles T/E codes using cgs.beRedTeamColor/beBlueTeamColor */
	CG_ParseColorStr(token1, scoreboard_rtColorHeader);
	CG_ParseColorStr(token2, scoreboard_rtColorBody);
	customScoreboardColorIsSet_red = 1;
	if (token3)
	{
		CG_ParseColorStr(token3, scoreboard_rtColorTitle);
		customScoreboardColorIsSet_red |= 2;
	}
}

void CG_LocalEventCvarChanged_cg_scoreboardBtColors(cvarTable_t* cvart)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;
	char* token3;

	if (!cvart || !cvart->vmCvar)
	{
		CG_Printf("^1Invalid cvar pointer\n");
		return;
	}

	if (cvart->vmCvar->string[0] == '\0')
	{
		customScoreboardColorIsSet_blue = 0;
		return;
	}

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));

	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");
	token3 = Q_strtok(NULL, " \t");

	if (!token1 || !token2)
	{
		CG_Printf("^1Invalid value: expected at least two color values (e.g. \"1 3\" or \"990033 green\")\n");
		customScoreboardColorIsSet_blue = 0;
		return;
	}

	/* Parse colors - CG_ParseColorStr already handles T/E codes using cgs.beRedTeamColor/beBlueTeamColor */
	CG_ParseColorStr(token1, scoreboard_btColorHeader);
	CG_ParseColorStr(token2, scoreboard_btColorBody);
	customScoreboardColorIsSet_blue = 1;
	if (token3)
	{
		CG_ParseColorStr(token3, scoreboard_btColorTitle);
		customScoreboardColorIsSet_blue |= 2;
	}
}

/*
 * Re-parse cg_scoreboardRtColors/BtColors so T/E resolve to current beRedTeamColor/beBlueTeamColor (cg_myTeam).
 * Call before drawing scoreboard so column colors follow cg_myTeam and viewed team.
 */
void CG_RefreshScoreboardTeamColors(void)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;
	char* token3;

	trap_Cvar_VariableStringBuffer("cg_scoreboardRtColors", buffer, sizeof(buffer));
	if (buffer[0] == '\0')
	{
		customScoreboardColorIsSet_red = 0;
	}
	else
	{
		token1 = Q_strtok(buffer, " \t");
		token2 = Q_strtok(NULL, " \t");
		token3 = Q_strtok(NULL, " \t");
		if (token1 && token2)
		{
			CG_ParseColorStr(token1, scoreboard_rtColorHeader);
			CG_ParseColorStr(token2, scoreboard_rtColorBody);
			customScoreboardColorIsSet_red = 1;
			if (token3)
			{
				CG_ParseColorStr(token3, scoreboard_rtColorTitle);
				customScoreboardColorIsSet_red |= 2;
			}
		}
	}

	trap_Cvar_VariableStringBuffer("cg_scoreboardBtColors", buffer, sizeof(buffer));
	if (buffer[0] == '\0')
	{
		customScoreboardColorIsSet_blue = 0;
	}
	else
	{
		token1 = Q_strtok(buffer, " \t");
		token2 = Q_strtok(NULL, " \t");
		token3 = Q_strtok(NULL, " \t");
		if (token1 && token2)
		{
			CG_ParseColorStr(token1, scoreboard_btColorHeader);
			CG_ParseColorStr(token2, scoreboard_btColorBody);
			customScoreboardColorIsSet_blue = 1;
			if (token3)
			{
				CG_ParseColorStr(token3, scoreboard_btColorTitle);
				customScoreboardColorIsSet_blue |= 2;
			}
		}
	}
}

void CG_LocalEventCvarChanged_cg_scoreboardSpecColor(cvarTable_t* cvart)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1;
	char* token2;

	if (!cvart || !cvart->vmCvar)
	{
		CG_Printf("^1Invalid cvar or string pointer\n");
		customScoreboardColorIsSet_spec = 0;
		return;
	}

	if (cvart->vmCvar->string[0] == '\0')
	{
		customScoreboardColorIsSet_spec = 0;
		return;
	}

	Q_strncpyz(buffer, cvart->vmCvar->string, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1)
	{
		customScoreboardColorIsSet_spec = 0;
		return;
	}

	CG_ParseColorStr(token1, scoreboard_specColor);
	customScoreboardColorIsSet_spec = 1;
	if (token2)
	{
		CG_ParseColorStr(token2, scoreboard_specColorTitle);
		customScoreboardColorIsSet_spec |= 2;
	}
}


void CG_LocalEventCvarChanged_cg_bestats_font(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_bestats_font", "0");
	}
	CG_UpdateFontIndices();
}

void CG_LocalEventCvarChanged_cg_bestats_pos(cvarTable_t* cvart)
{
	CG_ParseCvarTwoFloats(cvart, &cg.beSettings.x, &cg.beSettings.y);
}

void CG_LocalEventCvarChanged_cg_bestats_textSize(cvarTable_t* cvart)
{
	CG_ParseCvarTwoFloats(cvart, &cg.beSettings.textSize[0], &cg.beSettings.textSize[1]);
}

void CG_LocalEventCvarChanged_cg_bestats_bgColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.beSettings.bgColorIsSet = qfalse;
	}
	else
	{
		CG_LocalEventCvarParseColor(cvart, cg.beSettings.bgColor);
		cg.beSettings.bgColorIsSet = qtrue;
	}
}

void CG_LocalEventCvarChanged_cg_roundStats_font(cvarTable_t* cvart)
{
	int fontIndex;
	fontIndex = CG_FontIndexFromCvar(cvart->vmCvar);
	if (!CG_FontAvailable(fontIndex))
	{
		trap_Cvar_Set("cg_roundStats_font", "0");
	}
	CG_UpdateFontIndices();
}

void CG_LocalEventCvarChanged_cg_roundStats_pos(cvarTable_t* cvart)
{
	CG_ParseCvarTwoFloats(cvart, &cg.roundStatsSettings.x, &cg.roundStatsSettings.y);
}

void CG_LocalEventCvarChanged_cg_roundStats_textSize(cvarTable_t* cvart)
{
	CG_ParseCvarTwoFloats(cvart, &cg.roundStatsSettings.textSize[0], &cg.roundStatsSettings.textSize[1]);
}

void CG_LocalEventCvarChanged_cg_roundStats_bgColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.roundStatsSettings.bgColorIsSet = qfalse;
	}
	else
	{
		CG_LocalEventCvarParseColor(cvart, cg.roundStatsSettings.bgColor);
		cg.roundStatsSettings.bgColorIsSet = qtrue;
	}
}

void CG_LocalEventCvarChanged_cg_roundStats_borderColor(cvarTable_t* cvart)
{
	if (!cvart->vmCvar->string[0])
	{
		cg.roundStatsSettings.borderColorIsSet = qfalse;
	}
	else
	{
		CG_LocalEventCvarParseColor(cvart, cg.roundStatsSettings.borderColor);
		cg.roundStatsSettings.borderColorIsSet = qtrue;
	}
}

void CG_LocalEventBeFeaturesChanged(cvarTable_t* cvart)
{
	CG_UpdateBeFeatures();
	/* When cg_spectPOV changes, refresh all client info so models and teams update for spectator POV */
	if (cvart->vmCvar == &cg_spectPOV)
		CG_UpdateAllClientsInfo();
	if (cvart->vmCvar == &cg_teamIndicator || cvart->vmCvar == &cg_drawFriend ||
	        cvart->vmCvar == &cg_specMode)
		CG_BEUpdateUserInfo();
}

void CG_LocalEventCvarChanged_cg_exec_weapon(cvarTable_t* cvart)
{
	/* If cg_exec_weapons is disabled and user sets any weapon exec cvar, enable it */
	if (cg_exec_weapons.integer == 0 && cvart->vmCvar->string[0])
	{
		trap_Cvar_Set("cg_exec_weapons", "1");
	}
}

void CG_LocalEventCvarChanged_cg_exec_gametype(cvarTable_t* cvart)
{
	/* If cg_exec_gametypes is disabled and user sets any gametype exec cvar, enable it */
	if (cg_exec_gametypes.integer == 0 && cvart->vmCvar->string[0])
	{
		trap_Cvar_Set("cg_exec_gametypes", "1");
	}
}

void CG_LocalEventCvarChanged_cg_respawnWeapon(cvarTable_t* cvart)
{
	if (cgs.beSupportedServer)
	{
		CG_BEUpdateUserInfo();
	}
}

void CG_LocalEventCvarChanged_cg_brightWeaponsOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_brightItemsOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_printui(cvarTable_t* cvart)
{
	if (cvart->vmCvar->integer == 0)
	{
		// Clear all elements
		int i;
		int cleared = 0;
		for (i = 0; i < 32; i++)
		{
			if (cg.printui[i].active)
			{
				CG_PrintUIRemoveFromActiveList(i);
				cg.printui[i].active = qfalse;
				cleared++;
			}
		}
	}
}

void CG_LocalEventCvarChanged_cg_teamTaskThemeColor(cvarTable_t* cvart)
{
	vec4_t color;
	if (CG_ParseColorStr(cvart->vmCvar->string, color))
	{
		Vector4Copy(color, cg.teamMarkers.teamTaskThemeColor);
	}
	else
	{
		/* Default to cyan if parsing fails */
		cg.teamMarkers.teamTaskThemeColor[0] = 0.0f;
		cg.teamMarkers.teamTaskThemeColor[1] = 1.0f;
		cg.teamMarkers.teamTaskThemeColor[2] = 1.0f;
		cg.teamMarkers.teamTaskThemeColor[3] = 1.0f;
	}
}

void CG_LocalEventCvarChanged_cg_teamTaskMarkTime(cvarTable_t* cvart)
{
	int oldValue;
	int newValue;

	oldValue = cvart->vmCvar->integer;

	if (oldValue < 500 || oldValue > 5000)
	{
		newValue = Com_Clamp(500, 5000, oldValue);
		cvart->vmCvar->value = (float)newValue;
		cvart->vmCvar->integer = newValue;
		trap_Cvar_Set(cvart->cvarName, va("%d", newValue));
		CG_PrintOnce("^3cg_teamTaskMarkTime^7 is out of range, set to ^2%d^7\n", newValue);
	}
	/* markDurationMs refreshed once per frame in CG_TeamTaskUpdateFrame */
}

void CG_LocalEventCvarChanged_cg_simpleItemsRadius(cvarTable_t* cvart)
{
	if (cvart->vmCvar->value < 8.00f || cvart->vmCvar->integer < 8)
	{
		trap_Cvar_Set(cvart->cvarName, "8");
		CG_PrintOnce("^3cg_simpleItemsRadius^7 is out of range, set to ^28^7\n");
	}
	else if (cvart->vmCvar->value > 24.00f || cvart->vmCvar->integer > 24)
	{
		trap_Cvar_Set(cvart->cvarName, "24");
		CG_PrintOnce("^3cg_simpleItemsRadius^7 is out of range, set to ^224^7\n");
	}
}

void CG_LocalEventCvarChanged_cg_entityOverpolate(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-1.0f, 1.0f, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_numbersSet(cvarTable_t* cvart)
{
	int set;

	set = cvart->vmCvar->integer;
	if (set != 1 && set != 2 && set != 3 && set != 4 && set != 5 && set != 6)
	{
		trap_Cvar_Set("cg_numbersSet", "1");
		return;
	}
	CG_RegisterNumberShaders();
}
