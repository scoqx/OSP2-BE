/*
===========================================================================
cg_be_util.c

BE (Black Edition) utility functions

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com

This file contains original work created by diwoc.
It is included in the OSP2-BE modification for Quake III Arena and is
distributed under the terms of the project's primary license.
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

void CG_UpdateBeFeatures(void)
{
	static qboolean inUpdateAllClientsInfo = qfalse;
	static int lastBeEnabled = -1;
	qboolean changed = qfalse;
	qboolean doRefresh;
	qboolean needRebuild;
	int i = 0;
	int beFlags = 0;
	clientInfo_t* otherCi = &cgs.clientinfo[i];
	qboolean anyFbSkin = qfalse;

	cgs.beEnabled = be_enabled.integer;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (otherCi->infoValid && otherCi->skinType == SKIN_TYPE_FB)
		{
			anyFbSkin = qtrue;
			break;
		}
		otherCi++;
	}

	doRefresh = !inUpdateAllClientsInfo && (!cgs.beEnabled || lastBeEnabled == 0);
	needRebuild = !cgs.beEnabled;
	if (doRefresh)
	{
		CG_LoadForcedSounds();
		inUpdateAllClientsInfo = qtrue;
		if (needRebuild)
			CG_RebuildPlayerColors();
		CG_UpdateAllClientsInfo();
		inUpdateAllClientsInfo = qfalse;
	}
	lastBeEnabled = cgs.beEnabled ? 1 : 0;

	if (!cgs.beEnabled)
		beFlags = 0, changed = qtrue;
	else
	{
		if ((cg_friendsWallhack.integer & (CG_FRIENDS_WALLHACK_FOE | CG_FRIENDS_WALLHACK_TEAM_INDICATOR | CG_FRIENDS_WALLHACK_GLOBAL_POS))
		        || cg_playerXray.integer)
			beFlags |= CG_BE_TEAM_FOE_WH, changed = qtrue;
		if (cg_altGrenades.integer == 2 || cg_altGrenades.integer == 3)
			beFlags |= CG_BE_ALT_GRENADES, changed = qtrue;
		if (cg_drawOutline.integer)
			beFlags |= CG_BE_OUTLINE, changed = qtrue;
		if (cgs.customModelSound)
			beFlags |= CG_BE_MODELSOUND, changed = qtrue;
		if (cg_markTeam.integer >= 0)
			beFlags |= CG_BE_MARK_TEAMMATE, changed = qtrue;
		if (cg_teamIndicator.integer)
			beFlags |= CG_BE_TEAM_INDICATOR, changed = qtrue;
		if (cg_altShadow.integer)
			beFlags |= CG_BE_ALT_SHADOW, changed = qtrue;
		if (cg_altBlood.integer)
			beFlags |= CG_BE_ALT_BLOOD, changed = qtrue;
		if (cg_plasmaColor.string[0] || cg_enemyPlasmaColor.string[0])
			beFlags |= CG_BE_PLASMAGUN_COLOR, changed = qtrue;
		if (cgs.osp.myColorsOverride.isLightningColorSet || cgs.osp.teamColorsOverride.isLightningColorSet || cgs.osp.enemyColorsOverride.isLightningColorSet)
			beFlags |= CG_BE_ENEMYLIGHTNING, changed = qtrue;
		if (anyFbSkin || cg_drawBrightWeapons.integer || cg_drawBrightItems.integer)
			beFlags |= CG_BE_FULLBRIGHT, changed = qtrue;
		if (cg_drawHitBox.integer)
			beFlags |= CG_BE_HITBOX, changed = qtrue;
		if (cg_teamTask.integer)
			beFlags |= CG_BE_TEAMTASKS, changed = qtrue;
		if (cg_noExplosions.integer)
			beFlags |= CG_BE_NO_EXPLOSIONS, changed = qtrue;
		if (cg_friendsWallhackFrozen.integer)
			beFlags |= CG_BE_TEAM_FROZEN, changed = qtrue;
		if (cg_drawHudMarkers.integer)
			beFlags |= CG_BE_HUD_MARKERS, changed = qtrue;
		if (cg_footsteps.integer == 2)
			beFlags |= CG_BE_METAL_FOOTSTEPS, changed = qtrue;
		beFlags |= CG_BE_AUTOJUMP, changed = qtrue;
	}

	cgs.beFeaturesEnabled = beFlags & ~cgs.beDisableFeatures;
	if (changed)
		trap_Cvar_Set("be_features", va("%d", beFlags));
	if (!cgs.beEnabled || (cgs.beDisableFeatures & CG_BE_MODELSOUND))
		CG_ClearForcedSounds();
}

/*
=================
CG_UpdateFontIndices

Recompute cached font indices from font cvars.
Call when font cvars may have changed (local event) or at init.
Only updates cache when computed value differs (like CG_UpdateBeFeatures).
Direct field access = minimum ops (no table, no index).
=================
*/
void CG_UpdateFontIndices(void)
{
	int newVal;

	newVal = CG_FontIndexFromCvar(&cg_teamTaskFont);
	if (newVal != cgFontIndices.teamTask)
		cgFontIndices.teamTask = newVal;
	newVal = CG_FontIndexFromCvar(&cg_scoreboardFont);
	if (newVal != cgFontIndices.scoreboard)
		cgFontIndices.scoreboard = newVal;
	newVal = CG_FontIndexFromCvar(&cg_teamIndicatorFont);
	if (newVal != cgFontIndices.teamIndicator)
		cgFontIndices.teamIndicator = newVal;
	newVal = CG_FontIndexFromCvar(&cg_accuracyFont);
	if (newVal != cgFontIndices.accuracy)
		cgFontIndices.accuracy = newVal;
	newVal = CG_FontIndexFromCvar(&cg_centerMessagesFont);
	if (newVal != cgFontIndices.centerMessages)
		cgFontIndices.centerMessages = newVal;
	newVal = CG_FontIndexFromCvar(&be_infoVersionFont);
	if (newVal != cgFontIndices.infoVersion)
		cgFontIndices.infoVersion = newVal;
	newVal = CG_FontIndexFromCvar(&cg_bestats_font);
	if (newVal != cgFontIndices.bestats)
		cgFontIndices.bestats = newVal;
	newVal = CG_FontIndexFromCvar(&cg_roundStats_font);
	if (newVal != cgFontIndices.roundStats)
		cgFontIndices.roundStats = newVal;
	newVal = CG_FontIndexFromCvar(&cg_emojiListFont);
	if (newVal != cgFontIndices.emojiList)
		cgFontIndices.emojiList = newVal;
	newVal = CG_FontIndexFromCvar(&cg_chatWindowFont);
	if (newVal != cgFontIndices.chatWindow)
		cgFontIndices.chatWindow = newVal;
}

int CG_ApplyFpsclassicoDisableMask(int raw)
{
#if 0
	if (cgs.gamename_unfreeze)
	{
		return 0; /* server cannot disable any BE features */
	}
#endif
	return raw;
}

void CG_PrintDisabledFeatures(qboolean request)
{
	int value = cgs.beDisableFeatures;

#if 0
	if (cgs.gamename_unfreeze)
	{
		if (request)
		{
			CG_Printf("   [^9OSP2-BE^7] All features enabled.\n");
		}
		return;
	}
#endif

	if (value == 0)
	{
		if (request)
		{
			CG_Printf("   [^9OSP2-BE^7] All features enabled.\n");
		}
	}
	else if (value == 131071) // all 16 bits set (0xFFFF, bits 0-15)
	{
		CG_Printf("   [^9OSP2-BE^7] All features ^1disabled^7.\n");
	}
	else
	{
		char buffer[1024] = "   [^9OSP2-BE^7] ^1Disabled^7 features: ";
		qboolean first = qtrue;
		if (value & CG_BE_TEAM_FOE_WH)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Team wh / Player xray" : ", team wh / Player xray");
			first = qfalse;
		}
		if (value & CG_BE_MODELSOUND)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Model sound" : ", model sound");
			first = qfalse;
		}
		if (value & CG_BE_ALT_GRENADES)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Alt grenades" : ", alt grenades");
			first = qfalse;
		}
		if (value & CG_BE_ENEMYLIGHTNING)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Lightning color" : ", lightning color");
			first = qfalse;
		}
		if (value & CG_BE_MARK_TEAMMATE)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Mark teammate" : ", mark teammate");
			first = qfalse;
		}
		if (value & CG_BE_ALT_SHADOW)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Alt shadow" : ", alt shadow");
			first = qfalse;
		}
		if (value & CG_BE_ALT_BLOOD)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Alt blood" : ", alt blood");
			first = qfalse;
		}
		if (value & CG_BE_PLASMAGUN_COLOR)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Plasma color" : ", plasma color");
			first = qfalse;
		}
		if (value & CG_BE_OUTLINE)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Outline" : ", outline");
			first = qfalse;
		}
		if (value & CG_BE_TEAM_INDICATOR)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "team indicator" : ", team indicator");
			first = qfalse;
		}
		if (value & CG_BE_DAMAGEINFO)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Damage count" : ", damage count");
			first = qfalse;
		}
		if (value & CG_BE_FULLBRIGHT)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Fullbright" : ", fullbright");
			first = qfalse;
		}
		if (value & CG_BE_TEAMTASKS)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Team tasks" : ", team tasks");
			first = qfalse;
		}
		if (value & CG_BE_NO_EXPLOSIONS)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "No explosions" : ", no explosions");
			first = qfalse;
		}
		if (value & CG_BE_TEAM_FROZEN)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Team frozen wh" : ", team frozen wh");
			first = qfalse;
		}
		if (value & CG_BE_HUD_MARKERS)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "HUD markers" : ", hud markers");
			first = qfalse;
		}
		if (value & CG_BE_METAL_FOOTSTEPS)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Metal footsteps" : ", metal footsteps");
			first = qfalse;
		}
		if (value & CG_BE_AUTOJUMP)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Autojump" : ", autojump");
			first = qfalse;
		}
		if (value & CG_BE_HITBOX)
		{
			Q_strcat(buffer, sizeof(buffer), first ? "Hitbox" : ", hitbox");
			first = qfalse;
		}
		// if (value & CG_BE_FREEZE_SOUND_FIX)
		// {
		//  Q_strcat(buffer, sizeof(buffer), first ? "Freeze sound fix" : ", freeze sound fix");
		//  first = qfalse;
		// }
		CG_Printf("%s\n", buffer);
	}
}

void CG_BEParseXStatsToStatsAll(void)
{
	int i;
	int index = 1;
	int xstats_condition;
	int client_id;
	int hits_value, atts_value, kills_value, deaths_value;
	int pickUps, drops;
	int armor_taken, health_taken, megahealth, green_armor, red_armor, yellow_armor;
	int damage_given, damage_rcvd;
	int totalKills, totalDeaths;
	newStatsInfo_t* ws;

	client_id = atoi(CG_Argv(index++));
	xstats_condition = atoi(CG_Argv(index++));

	if (client_id < 0 || client_id >= MAX_CLIENTS)
	{
		return;
	}

	ws = &cgs.statsAll[client_id];

	for (i = 1; i < WP_NUM_WEAPONS; ++i)
	{
		if ((xstats_condition & (1 << i)) != 0)
		{
			hits_value = atoi(CG_Argv(index++));
			atts_value = atoi(CG_Argv(index++));
			kills_value = atoi(CG_Argv(index++));
			deaths_value = atoi(CG_Argv(index++));

			pickUps = atts_value >> cgs.osp.stats_shift;
			drops   = hits_value >> cgs.osp.stats_shift;

			ws->stats[i].hits = hits_value & cgs.osp.stats_mask;
			ws->stats[i].shots = atts_value & cgs.osp.stats_mask;
			ws->stats[i].kills = kills_value;
			ws->stats[i].deaths = deaths_value;
			ws->stats[i].pickUps = pickUps;
			ws->stats[i].drops = drops;
			ws->stats[i].accuracy = (ws->stats[i].shots > 0) ? ((float)ws->stats[i].hits / ws->stats[i].shots) * 100.0f : 0.0f;
		}
		else
		{
			ws->stats[i].hits = 0;
			ws->stats[i].shots = 0;
			ws->stats[i].kills = 0;
			ws->stats[i].deaths = 0;
			ws->stats[i].pickUps = 0;
			ws->stats[i].drops = 0;
			ws->stats[i].accuracy = 0.0f;
		}
	}

	armor_taken   = atoi(CG_Argv(index++));
	health_taken  = atoi(CG_Argv(index++));
	damage_given  = atoi(CG_Argv(index++));
	damage_rcvd   = atoi(CG_Argv(index++));
	megahealth    = atoi(CG_Argv(index++));
	green_armor   = atoi(CG_Argv(index++));
	red_armor     = atoi(CG_Argv(index++));
	yellow_armor  = atoi(CG_Argv(index++));

	ws->armor      = armor_taken;
	ws->health     = health_taken;
	ws->megahealth = megahealth;
	ws->ga         = green_armor;
	ws->ra         = red_armor;
	ws->ya         = yellow_armor;
	ws->dmgGiven   = damage_given;
	ws->dmgReceived = damage_rcvd;

	ws->damageRatio = (damage_given > 0 || damage_rcvd > 0) ?
	                  (float)damage_given / (damage_rcvd > 0 ? damage_rcvd : 1) : 0.0f;

	totalKills = 0;
	totalDeaths = 0;
	for (i = 1; i < WP_NUM_WEAPONS; ++i)
	{
		totalKills += ws->stats[i].kills;
		totalDeaths += ws->stats[i].deaths;
	}
	ws->kills = totalKills;
	ws->deaths = totalDeaths;
	ws->kdratio = (totalKills > 0 && totalDeaths == 0) ? (float)totalKills :
	              (totalDeaths > 0) ? (float)totalKills / totalDeaths : 0.0f;

	ws->efficiency = (totalKills + totalDeaths > 0) ?
	                 (100.0f * (float)totalKills / (totalKills + totalDeaths)) : 0.0f;
	if (ws->efficiency < 0.0f)
		ws->efficiency = 0.0f;

	ws->infoValid = qtrue;
}

void cg_printsa_f(void)
{
	int i;
	newStatsInfo_t* ws;

	CG_Printf("^5StatsAll:\n");
	for (i = 0; i < MAX_CLIENTS; ++i)
	{
		ws = &cgs.statsAll[i];
		if (!ws->infoValid)
			continue;

		CG_Printf("^7[%2d] ^3%-16s ^7K/D:^2%3d^7/^1%3d ^7Eff:^2%5.1f%% ^7Dmg:^2%i^7/^1%i ^7Ratio:^2%.2f\n",
		          i,
		          cgs.clientinfo[i].name,
		          ws->kills,
		          ws->deaths,
		          ws->efficiency,
		          ws->dmgGiven,
		          ws->dmgReceived,
		          ws->damageRatio
		         );
	}
}

void CG_ParseCvarTwoColorsSimple(const vmCvar_t* cvar, vec4_t out1, vec4_t out2)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* token1, *token2;

	if (!cvar) return;
	if (cvar->string[0] == '\0') return;

	Q_strncpyz(buffer, cvar->string, sizeof(buffer));
	token1 = Q_strtok(buffer, " \t");
	token2 = Q_strtok(NULL, " \t");

	if (!token1 || !token2) return;

	CG_ParseColorStr(token1, out1);
	CG_ParseColorStr(token2, out2);
}

/*
=================
CG_UpdateDamageInfoAllowed
=================
*/
void CG_UpdateDamageInfoAllowed(void)
{
	if (CG_OSPIsGameTypeCA(cgs.gametype))
	{
		cgs.damageInfoAllowed = qtrue;
	}
#if 0
	else if (cgs.gamename_unfreeze)
	{
		cgs.damageInfoAllowed = qfalse;
	}
#endif
	else
	{
		cgs.damageInfoAllowed = (CG_BE_FEATURE_ENABLED(CG_BE_DAMAGEINFO) || OSP_CUSTOM_CLIENT_2_IS_DMG_INFO_ALLOWED());
	}
}

int CG_GetRemainingGameTimeMsec(qboolean forItemPickup)
{
	int msec;
	int baseTime;

	if (cgs.timelimit > 0.0f && cg.warmup == 0)
	{
		if (forItemPickup)
		{
			baseTime = cg.itemPickupTime;
		}
		else
		{
			baseTime = cg.time;
		}

		msec = cgs.timelimitMsec - (baseTime - cgs.levelStartTime);
		return msec;
	}
	else
	{
		return cg.time - cgs.levelStartTime;
	}

}

void CG_ColorWhitenSmart(vec3_t rgb, float* alpha, float strength)
{
	float luminance;
	float factor;
	float absStrength;
	qboolean isDarkening;

	if (strength == 0.0f)
	{
		return;
	}

	/* determine direction and absolute value */
	isDarkening = (strength < 0.0f);
	absStrength = (strength < 0.0f) ? -strength : strength;

	/* perceptual luminance */
	luminance = rgb[0] * 0.299f +
	            rgb[1] * 0.587f +
	            rgb[2] * 0.114f;

	if (isDarkening)
	{
		/* lighter color — stronger darkening */
		factor = absStrength * luminance;
	}
	else
	{
		/* darker color — stronger lightening */
		factor = absStrength * (1.0f - luminance);
	}

	if (isDarkening)
	{
		if (factor <= 0.0f)
		{
			return;
		}
		if (factor > 1.0f)
		{
			factor = 1.0f;
		}
		/* move color toward black */
		rgb[0] -= rgb[0] * factor;
		rgb[1] -= rgb[1] * factor;
		rgb[2] -= rgb[2] * factor;
	}
	else
	{
		/* for lightening use absStrength directly with linear interpolation toward white */
		/* ensures all components move toward 1.0 proportional to strength */
		if (absStrength > 1.0f)
		{
			absStrength = 1.0f;
		}
		rgb[0] = rgb[0] * (1.0f - absStrength) + 1.0f * absStrength;
		rgb[1] = rgb[1] * (1.0f - absStrength) + 1.0f * absStrength;
		rgb[2] = rgb[2] * (1.0f - absStrength) + 1.0f * absStrength;
	}

	/* optionally adjust alpha for HUD */
	if (alpha)
	{
		if (isDarkening)
		{
			/* reduce alpha when darkening */
			*alpha -= *alpha * factor * 0.5f;
			if (*alpha < 0.0f)
			{
				*alpha = 0.0f;
			}
		}
		else
		{
			/* increase alpha when lightening */
			*alpha += (1.0f - *alpha) * factor * 0.5f;
			if (*alpha > 1.0f)
			{
				*alpha = 1.0f;
			}
		}
	}
}


/*
=================
CG_Get_f

Dynamic command handler for /get% commands
Reads beconfigs/customConfig_%.cfg file and sends its contents to console
=================
*/
void CG_Get_f(void)
{
	const char* cmd;
	char configName[65];
	char filePath[MAX_QPATH];
	fileHandle_t fileHandle;
	int fileSize;
	char* fileContent;
	int i;
	int prefixLen;
	int nameLen;

	cmd = CG_Argv(0);
	prefixLen = 3; /* length of "get" */
	nameLen = 0;
	while (cmd[prefixLen + nameLen] != '\0' && nameLen < 64)
	{
		nameLen++;
	}
	if (nameLen == 0)
	{
		return;
	}
	for (i = 0; i < nameLen; i++)
	{
		configName[i] = cmd[prefixLen + i];
	}
	configName[nameLen] = '\0';

	/* Build file path: beconfigs/customConfig_<name>.cfg (case-insensitive filesystem) */
	Com_sprintf(filePath, sizeof(filePath), "beconfigs/customConfig_%s.cfg", configName);
	fileSize = trap_FS_FOpenFile(filePath, &fileHandle, FS_READ);

	if (fileSize <= 0 || !fileHandle)
	{
		CG_Printf("^1File not found: %s\n", filePath);
		return;
	}

	/* Limit file size to prevent memory issues (max 64KB) */
	if (fileSize > 65536)
	{
		CG_Printf("^1File too large: %s (%d bytes, max 65536)\n", filePath, fileSize);
		trap_FS_FCloseFile(fileHandle);
		return;
	}

	/* Allocate memory for file content */
	fileContent = (char*)Z_Malloc(fileSize + 1);
	if (!fileContent)
	{
		CG_Printf("^1Out of memory reading file: %s\n", filePath);
		trap_FS_FCloseFile(fileHandle);
		return;
	}

	/* Read file content */
	trap_FS_Read(fileContent, fileSize, fileHandle);
	trap_FS_FCloseFile(fileHandle);
	fileContent[fileSize] = '\0';

	/* Send file content to console */
	trap_SendConsoleCommand(fileContent);

	/* Free allocated memory */
	Z_Free(fileContent);
}

/*
================
CG_ParseHitboxConfigString

CS_OSP2BE_HITBOX (911):
  Ruthless: "1 <wide> <hgh>" — body cylinder only (head removed; legacy "2 ..." same)
  Metarena/OSDF: single-token "<scale>" (>0 = cylinder AABB*scale; "0"/empty/ambiguous = AABB cube)
================
*/
void CG_ParseHitboxConfigString(const char* str)
{
	int enabled = 0;
	float wide = 0, hgh = 0, scale = 0;
	char junk[64];
	int n = 0;

	cgs.hbEnabled = qfalse;
	cgs.hbMode = 0;
	cgs.hbWide = 0;
	cgs.hbHgh = 0;
	cgs.hbScale = 0;

	if (!str || !str[0])
	{
		return;
	}

	n = Q_sscanf(str, "%i %f %f", &enabled, &wide, &hgh);
	/* Mode 1 = current body-only; mode 2 accepted as legacy alias (old no-head) */
	if (n >= 3 && (enabled == 1 || enabled == 2))
	{
		cgs.hbEnabled = qtrue;
		cgs.hbMode = 1;
		cgs.hbWide = wide;
		cgs.hbHgh = hgh;
		return;
	}

	/* Exactly one float token: metarena g_playerCylinderHitboxScale */
	junk[0] = '\0';
	if (Q_sscanf(str, "%f %s", &scale, junk) == 1 && scale > 0.0f)
	{
		cgs.hbEnabled = qtrue;
		cgs.hbMode = 3;
		cgs.hbScale = scale;
	}
}
