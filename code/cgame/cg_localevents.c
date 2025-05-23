#include "cg_superhud.h"
#include "cg_local.h"

int global_handicap; // unused?
//	void         (*onChanged)(cvarTable_t *cvart);

void CG_LocalEventCvarChanged_cg_drawTeamOverlay(cvarTable_t* cvart)
{
	if ((cg_drawTeamOverlay.integer > 0) ||
	        ((!(cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE)) &&
	         (ch_TeamCrosshairHealth.integer > 0 || ch_crosshairTeamInfo.integer > 0)))
	{
		trap_Cvar_Set("teamoverlay", "1");
	}
	else
	{
		trap_Cvar_Set("teamoverlay", "0");
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
void CG_LocalEventCvarChanged_r_fullbright(cvarTable_t* cvart)
{
	CG_OSPCvarsRestrictValues();
}


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
		CG_Printf("^3Enhanced client sampling has been disabled on this server.\n");
		trap_Cvar_Set("pmove_fixed", "0");
	}
}

void CG_LocalEventCvarChanged_cg_hitSounds(cvarTable_t* cvart)
{
	if (!(cgs.osp.custom_client_2 & OSP_CUSTOM_CLIENT_2_ENABLE_DMG_INFO) && cg_hitSounds.integer)
	{
		CG_Printf("^3Damage info has been disabled on this server.\n");
		trap_Cvar_Set("cg_hitSounds", "0");
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
		CG_SHUDLoadConfig();
	}
}

void CG_LocalEventCvarChanged_cg_shud(cvarTable_t* cvart)
{
	if (cvart->vmCvar->integer && ch_file.string[0])
	{
		CG_SHUDLoadConfig();
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
	CG_LocalEventCvarParseColor(cvart, cgs.osp.crosshair.color);
}

void CG_LocalEventCvarChanged_ch_crosshairActionColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.osp.crosshair.actionColor);
}

void CG_LocalEventCvarChanged_ch_crosshairDecorColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.osp.crosshair.decorColor);
}

void CG_LocalEventCvarChanged_ch_crosshairDecorActionColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.osp.crosshair.decorActionColor);
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
}

void CG_LocalEventCvarChanged_ch_crosshairDecorActionTime(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(50, 1000, cvart->vmCvar->value);
}

void CG_LocalEventCvarChanged_cg_damageIndicatorOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
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
	CG_UpdateAllClientsInfo();
}

void CG_LocalEventCvarChanged_cg_drawHitBox(cvarTable_t* cvart)
{


	if (cgs.osp.serverConfigXHitBox != 1 && cg_drawHitBox.integer)
	{
		if (!cg.demoPlayback)
		{
			CG_Printf("^3HitBox is disabled on this server.\n");
		}
	}
}

void CG_LocalEventCvarChanged_cg_hitBoxColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.be.hitBoxColor);
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
	CG_LocalEventCvarParseColor(cvart, cgs.be.enemyOutlineColor);
}

void CG_LocalEventCvarChanged_cg_teamOutlineColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.be.teamOutlineColor);
}

void CG_LocalEventCvarChanged_cg_altBloodColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.be.altBloodColor);
}

void CG_LocalEventCvarChanged_cg_railRingsRadius(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.05f, 16, cvart->vmCvar->value);
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
	cvart->vmCvar->value = Com_Clamp(0.05f, 4, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_altShadowColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, cgs.be.altShadowColor);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorAdjust(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0.5f, 2, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, playerIndicator.color);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorBgColor(cvarTable_t* cvart)
{
	CG_LocalEventCvarParseColor(cvart, playerIndicator.bgColor);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorBgOpaque(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(0, 1, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorOffset(cvarTable_t* cvart)
{
	cvart->vmCvar->value = Com_Clamp(-16, 32, cvart->vmCvar->value);
}
void CG_LocalEventCvarChanged_cg_teamIndicatorMaxLength(cvarTable_t* cvart)
{
	cvart->vmCvar->integer = Com_Clamp(1, MAX_QPATH, cvart->vmCvar->integer);
}
void CG_LocalEventCvarChanged_cg_scoreboardFont(cvarTable_t* cvart)
{
	if (!CG_FontAvailable(cvart->vmCvar->integer))
	{
		trap_Cvar_Set("cg_scoreboardFont", 0);
	}
}
void CG_LocalEventCvarChanged_cg_teamIndicatorFont(cvarTable_t* cvart)
{
	if (!CG_FontAvailable(cvart->vmCvar->integer))
	{
		trap_Cvar_Set("cg_teamIndicatorFont", 0);
	}
}
void CG_LocalEventCvarChanged_cg_centerMessagesFont(cvarTable_t* cvart)
{
	if (!CG_FontAvailable(cvart->vmCvar->integer))
	{
		trap_Cvar_Set("cg_centerMessagesFont", 0);
	}
}

