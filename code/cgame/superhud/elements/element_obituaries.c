#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctxAttacker;
	superhudDrawContext_t ctxMod;
	superhudTextContext_t ctxTarget;
	superhudGlobalContext_t* gctx;
	int index;
} shudElementObituaries_t;

shudElementObituaries_t cg_obituaries;

static qhandle_t CG_SHUDObituaryGetModIcon(int mod, qboolean unfrozen);
static qhandle_t CG_SHUDObituaryGetDroppedPowerupIcon(int powerup);
static void CG_SHUDObituarySetTeamColor(vec4_t color, int team);
static void CG_SHUDObituaryBuildDroppedPowerups(shudElementObituaries_t* element, superhudObituariesEntry_t* entry);

static void* CG_SHUDElementObituariesCreate(const superhudConfig_t* config, int line)
{
	shudElementObituaries_t* element;
	superhudConfig_t lcfg;
	SHUD_ELEMENT_INIT(element, config);
	element->gctx = &superhudGlobalContext;
	element->index = line;

	memcpy(&lcfg, config, sizeof(superhudConfig_t));

	if (!lcfg.alignV.isSet)
	{
		lcfg.alignV.value = SUPERHUD_ALIGNV_CENTER;
		lcfg.alignV.isSet = qtrue;
	}

	CG_SHUDTextMakeContext(&lcfg, &element->ctxAttacker);
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxMod);
	CG_SHUDTextMakeContext(&lcfg, &element->ctxTarget);

	return element;
}

void* CG_SHUDElementObituaries1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 1);
}

void* CG_SHUDElementObituaries2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 2);
}

void* CG_SHUDElementObituaries3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 3);
}

void* CG_SHUDElementObituaries4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 4);
}

void* CG_SHUDElementObituaries5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 5);
}

void* CG_SHUDElementObituaries6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 6);
}

void* CG_SHUDElementObituaries7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 7);
}

void* CG_SHUDElementObituaries8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementObituariesCreate(config, 8);
}

static void CG_SHUDElementObituariesUpdatePosition(shudElementObituaries_t* element, superhudObituariesEntry_t* entry)
{
	float totalWidth;
	qboolean showAttacker;
	float iconW;

	showAttacker = (entry->attacker != entry->target);
	iconW = element->ctxMod.coord.named.w;

	if (showAttacker)
	{
		entry->runtime.maxNameLenPix = (element->config.rect.value[2] - (iconW + entry->runtime.spacing * 2)) / 2;
	}
	else
	{
		entry->runtime.maxNameLenPix = element->config.rect.value[2] - iconW - entry->runtime.spacing * 3 - entry->runtime.droppedPuWidth;
		if (entry->runtime.maxNameLenPix < iconW)
		{
			entry->runtime.maxNameLenPix = iconW;
		}
	}

	CG_FontSelect(element->ctxAttacker.fontIndex); // update font metrics to make right calculation
	if (showAttacker)
	{
		entry->runtime.attackerWidth = CG_OSPDrawStringLenPix(entry->runtime.attackerName, element->config.fontsize.value[0], element->ctxAttacker.flags, entry->runtime.maxNameLenPix);
	}
	else
	{
		entry->runtime.attackerWidth = 0;
	}
	CG_FontSelect(element->ctxTarget.fontIndex);
	entry->runtime.targetWidth = CG_OSPDrawStringLenPix(entry->runtime.targetName, element->config.fontsize.value[0], element->ctxTarget.flags, entry->runtime.maxNameLenPix);

	totalWidth = entry->runtime.attackerWidth + entry->runtime.spacing +
	             iconW + entry->runtime.spacing * 2 +
	             entry->runtime.targetWidth + entry->runtime.droppedPuWidth;

	if (element->config.alignH.value == SUPERHUD_ALIGNH_LEFT)
	{
		entry->runtime.baseX = element->config.rect.value[0];
	}
	else if (element->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
	{
		entry->runtime.baseX = element->config.rect.value[0] + element->config.rect.value[2] - totalWidth;
	}
	else // SUPERHUD_ALIGNH_CENTER
	{
		if (showAttacker)
		{
			entry->runtime.baseX = element->config.rect.value[0] + (element->config.rect.value[2] / 2) - (iconW / 2) - entry->runtime.attackerWidth;
		}
		else
		{
			entry->runtime.baseX = element->config.rect.value[0] + (element->config.rect.value[2] - totalWidth) / 2;
		}
	}
}

static void CG_SHUDElementObituariesInitializeRuntime(shudElementObituaries_t* element, superhudObituariesEntry_t* entry)
{
	entry->runtime.maxVisibleChars = 13;
	entry->runtime.spacing = element->config.rect.value[3] / 4.0f;

	entry->runtime.iconShader = CG_SHUDObituaryGetModIcon(entry->mod, entry->unfrozen);

	CG_SHUDObituarySetTeamColor(entry->runtime.attackerColor, entry->attackerTeam);
	CG_SHUDObituarySetTeamColor(entry->runtime.targetColor, entry->targetTeam);

	if (element->config.style.isSet && element->config.style.value)
	{
		if (element->config.bgcolor.isSet && element->config.style.value == 1)
		{
			entry->runtime.attackerColor[3] = element->config.bgcolor.value.rgba[3];
			entry->runtime.targetColor[3] = element->config.bgcolor.value.rgba[3];
		}
		else if (element->config.color.isSet && element->config.style.value == 2)
		{
			entry->runtime.attackerColor[3] = element->config.color.value.rgba[3];
			entry->runtime.targetColor[3] = element->config.color.value.rgba[3];
		}
	}
	if (entry->attacker == ENTITYNUM_WORLD)
	{
		strcpy(entry->runtime.attackerName, "^1world");
	}
	else if (entry->attacker >= 0 && entry->attacker < MAX_CLIENTS)
	{
		if (cgs.gametype >= GT_TEAM && (element->config.style.isSet && element->config.style.value == 2))
		{
			Q_strncpyz(entry->runtime.attackerName, cgs.clientinfo[entry->attacker].name_clean, MAX_QPATH);
		}
		else
		{
			Q_strncpyz(entry->runtime.attackerName, cgs.clientinfo[entry->attacker].name, MAX_QPATH);
		}
	}

	if (entry->target >= 0 && entry->target < MAX_CLIENTS)
	{
		if (cgs.gametype >= GT_TEAM && (element->config.style.isSet && element->config.style.value == 2))
		{
			Q_strncpyz(entry->runtime.targetName, cgs.clientinfo[entry->target].name_clean, MAX_QPATH);
		}
		else
		{
			Q_strncpyz(entry->runtime.targetName, cgs.clientinfo[entry->target].name, MAX_QPATH);
		}
	}


	entry->runtime.isInitialized = qtrue;
}

static void CG_SHUDObituaryBuildDroppedPowerups(shudElementObituaries_t* element, superhudObituariesEntry_t* entry)
{
	static const int puList[6] = { PW_QUAD, PW_BATTLESUIT, PW_HASTE, PW_INVIS, PW_REGEN, PW_FLIGHT };
	float iconSize;
	float iconGap;
	int iconCount;
	int i;

	entry->runtime.droppedPuCount = 0;
	entry->runtime.droppedPuWidth = 0;
	iconSize = element->config.rect.value[3];
	iconGap = entry->runtime.spacing;

	if (entry->droppedFlagTeam == TEAM_RED)
	{
		Vector4Copy(cgs.beRedTeamColor, entry->runtime.droppedFlagColor);
		CG_ColorWhitenSmart(entry->runtime.droppedFlagColor, NULL, 0.4f);
		entry->runtime.droppedFlagColor[3] = 1.0f;
	}
	else if (entry->droppedFlagTeam == TEAM_BLUE)
	{
		Vector4Copy(cgs.beBlueTeamColor, entry->runtime.droppedFlagColor);
		CG_ColorWhitenSmart(entry->runtime.droppedFlagColor, NULL, 0.4f);
		entry->runtime.droppedFlagColor[3] = 1.0f;
	}

	for (i = 0; i < 6; i++)
	{
		if (entry->droppedPowerups & (1 << puList[i]))
		{
			entry->runtime.droppedPuShaders[entry->runtime.droppedPuCount] =
			    CG_SHUDObituaryGetDroppedPowerupIcon(puList[i]);
			entry->runtime.droppedPuCount++;
		}
	}

	iconCount = entry->runtime.droppedPuCount;
	if (entry->droppedFlagTeam == TEAM_RED || entry->droppedFlagTeam == TEAM_BLUE)
	{
		iconCount++;
	}

	if (iconCount > 0)
	{
		entry->runtime.droppedPuWidth = iconGap +
		                                iconCount * iconSize +
		                                (iconCount - 1) * iconGap;
	}
}

void CG_SHUDElementObituariesRoutine(void* context)
{
	shudElementObituaries_t* element = (shudElementObituaries_t*)context;
	superhudObituariesEntry_t* entry;
	int index;
	float currentX;
	float bgX, bgY, bgW, bgH;

	index = (element->gctx->obituaries.index - element->index) % SHUD_MAX_OBITUARIES_LINES;
	entry = &element->gctx->obituaries.line[index];

	if (entry->time == 0 || !CG_SHUDGetFadeColor(element->ctxAttacker.color_origin, element->ctxAttacker.color, &element->config, entry->time))
	{
		entry->time = 0;
		return;
	}

	element->ctxMod.coord.named.w = element->config.rect.value[3];   // icon size scaling
	element->ctxMod.coord.named.h = element->config.rect.value[3];

	if (!entry->runtime.isInitialized)
	{
		CG_SHUDElementObituariesInitializeRuntime(element, entry);
		CG_SHUDObituaryBuildDroppedPowerups(element, entry);
	}

	CG_SHUDElementObituariesUpdatePosition(element, entry);

	currentX = entry->runtime.baseX;

	/* Calculate background/border coordinates for local player events */
	bgX = entry->runtime.baseX - (element->ctxMod.coord.named.h * 0.05);
	bgY = element->ctxAttacker.coord.named.y - (element->ctxAttacker.coord.named.h * 0.55);
	bgW = entry->runtime.attackerWidth + entry->runtime.spacing * 2 + element->ctxMod.coord.named.w + entry->runtime.targetWidth + entry->runtime.droppedPuWidth + (element->ctxMod.coord.named.h * 0.15);
	bgH = element->ctxAttacker.coord.named.h * 1.1;

	if ((cg.clientNum == entry->attacker || cg.clientNum == entry->target) &&
	        element->config.bgcolor.isSet)
	{
		vec4_t bgColor;
		float fadeAlpha;

		fadeAlpha = CG_SHUDFadeAlphaScale(element->ctxAttacker.color_origin, element->ctxAttacker.color);
		CG_SHUDConfigPickBgColor(&element->config, bgColor, qfalse);
		if (fadeAlpha != 1.0f)
		{
			bgColor[3] *= fadeAlpha;
		}
		CG_FillRect(bgX, bgY, bgW, bgH, bgColor);
	}
	if (entry->attacker != entry->target)
	{
		element->ctxAttacker.text = entry->runtime.attackerName;
		element->ctxAttacker.coord.named.x = currentX;

		if ((entry->attackerTeam == TEAM_RED || entry->attackerTeam == TEAM_BLUE) && element->config.style.isSet)
		{
			qboolean shouldColorAttacker;
			qboolean isAttackerTeammate;
			qboolean isLocalPlayerSpectator;

			/* Check if local player is spectator */
			isLocalPlayerSpectator = (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR);

			/* Check if attacker is teammate by comparing teams */
			if (entry->attacker >= 0 && entry->attacker < MAX_CLIENTS)
			{
				isAttackerTeammate = (entry->attackerTeam == cgs.clientinfo[cg.clientNum].team);
			}
			else
			{
				isAttackerTeammate = qfalse;
			}

			/* Determine if we should color attacker based on bitmask */
			shouldColorAttacker = qfalse;
			if (isLocalPlayerSpectator)
			{
				/* If local player is spectator, color both regardless of flags */
				shouldColorAttacker = qtrue;
			}
			else if (element->config.style.value & 4) /* bit 4: only teammates */
			{
				shouldColorAttacker = isAttackerTeammate;
			}
			else if (element->config.style.value & 8) /* bit 8: only enemies */
			{
				shouldColorAttacker = !isAttackerTeammate;
			}
			else /* neither bit set: color both */
			{
				shouldColorAttacker = qtrue;
			}

			if (shouldColorAttacker)
			{
				if (element->config.style.value & 1)
				{
					Vector4Copy(entry->runtime.attackerColor, element->ctxAttacker.background);
				}
				else if (element->config.style.value & 2)
				{
					Vector4Copy(entry->runtime.attackerColor, element->ctxAttacker.color);
				}
			}
			else
			{
				element->ctxAttacker.background[3] = 0;
			}
		}
		else
		{
			element->ctxAttacker.background[3] = 0;
		}

		element->ctxAttacker.width = entry->runtime.maxNameLenPix;
		CG_SHUDTextPrintNew(&element->config, &element->ctxAttacker, qfalse);
		currentX += entry->runtime.attackerWidth;
	}
	if (entry->runtime.iconShader)
	{
		element->ctxMod.image = entry->runtime.iconShader;
		element->ctxMod.coord.named.x = currentX + entry->runtime.spacing;
		CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxMod);
		currentX += element->ctxMod.coord.named.w + entry->runtime.spacing * 2;
	}
	element->ctxTarget.text = entry->runtime.targetName;
	element->ctxTarget.coord.named.x = currentX;
	if ((entry->targetTeam == TEAM_RED || entry->targetTeam == TEAM_BLUE) && element->config.style.isSet)
	{
		qboolean shouldColorTarget;
		qboolean isTargetTeammate;
		qboolean isLocalPlayerSpectator;

		/* Check if local player is spectator */
		isLocalPlayerSpectator = (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR);

		/* Check if target is teammate by comparing teams */
		if (entry->target >= 0 && entry->target < MAX_CLIENTS)
		{
			isTargetTeammate = (entry->targetTeam == cgs.clientinfo[cg.clientNum].team);
		}
		else
		{
			isTargetTeammate = qfalse;
		}

		/* Determine if we should color target based on bitmask */
		shouldColorTarget = qfalse;
		if (isLocalPlayerSpectator)
		{
			/* If local player is spectator, color both regardless of flags */
			shouldColorTarget = qtrue;
		}
		else if (element->config.style.value & 4) /* bit 4: only teammates */
		{
			shouldColorTarget = isTargetTeammate;
		}
		else if (element->config.style.value & 8) /* bit 8: only enemies */
		{
			shouldColorTarget = !isTargetTeammate;
		}
		else /* neither bit set: color both */
		{
			shouldColorTarget = qtrue;
		}

		if (shouldColorTarget)
		{
			if (element->config.style.value & 1)
			{
				Vector4Copy(entry->runtime.targetColor, element->ctxTarget.background);
			}
			else if (element->config.style.value & 2)
			{
				Vector4Copy(entry->runtime.targetColor, element->ctxTarget.color);
			}
		}
		else
		{
			element->ctxTarget.background[3] = 0;
		}
	}
	else
	{
		element->ctxTarget.background[3] = 0;
	}
	element->ctxTarget.width = entry->runtime.maxNameLenPix;
	CG_SHUDTextPrintNew(&element->config, &element->ctxTarget, qfalse);
	currentX += entry->runtime.targetWidth;

	if (entry->runtime.droppedPuCount > 0 || entry->droppedFlagTeam == TEAM_RED || entry->droppedFlagTeam == TEAM_BLUE)
	{
		int i;
		vec4_t iconColor;
		float fadeAlpha;
		float iconGap;

		iconGap = entry->runtime.spacing;
		fadeAlpha = CG_SHUDFadeAlphaScale(element->ctxAttacker.color_origin, element->ctxAttacker.color);
		currentX += iconGap;

		if (entry->droppedFlagTeam == TEAM_RED || entry->droppedFlagTeam == TEAM_BLUE)
		{
			Vector4Copy(entry->runtime.droppedFlagColor, iconColor);
			if (fadeAlpha != 1.0f)
			{
				iconColor[3] *= fadeAlpha;
			}
			element->ctxMod.coord.named.x = currentX;
			CG_SHUDDrawStretchPic(element->ctxMod.coord, element->ctxMod.coordPicture, iconColor,
			                      cgs.media.obituariesDroppedFlag);
			currentX += element->ctxMod.coord.named.w + iconGap;
		}

		for (i = 0; i < entry->runtime.droppedPuCount; i++)
		{
			element->ctxMod.image = entry->runtime.droppedPuShaders[i];
			element->ctxMod.coord.named.x = currentX;
			CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxMod);
			currentX += element->ctxMod.coord.named.w + iconGap;
		}
	}

	/* Draw border on entire line if configured and for local player events */
	if ((cg.clientNum == entry->attacker || cg.clientNum == entry->target) &&
	        element->config.border.isSet && element->config.borderColor.isSet)
	{
		vec4_t coord;
		vec4_t borderColor;
		float x, y, w, h;

		/* Use same coordinates as background fill */
		x = bgX;
		y = bgY;
		w = bgW;
		h = bgH;

		CG_AdjustFrom640(&x, &y, &w, &h);

		coord[0] = x;
		coord[1] = y;
		coord[2] = w;
		coord[3] = h;

		CG_SHUDConfigPickBorderColor(&element->config, borderColor, qfalse);
		{
			float fadeAlpha;

			fadeAlpha = CG_SHUDFadeAlphaScale(element->ctxAttacker.color_origin, element->ctxAttacker.color);
			if (fadeAlpha != 1.0f)
			{
				borderColor[3] *= fadeAlpha;
			}
		}

		CG_OSPDrawFrame(coord[0], coord[1], coord[2], coord[3],
		                (float*)element->config.border.value, borderColor, qfalse);
	}
}

static qhandle_t CG_SHUDObituaryGetDroppedPowerupIcon(int powerup)
{
	switch (powerup)
	{
		case PW_QUAD:
			return cgs.media.obituariesDroppedQuad;
		case PW_BATTLESUIT:
			return cgs.media.obituariesDroppedBattlesuit;
		case PW_HASTE:
			return cgs.media.obituariesDroppedHaste;
		case PW_INVIS:
			return cgs.media.obituariesDroppedInvis;
		case PW_REGEN:
			return cgs.media.obituariesDroppedRegen;
		case PW_FLIGHT:
			return cgs.media.obituariesDroppedFlight;
		default:
			return 0;
	}
}

static void CG_SHUDObituarySetTeamColor(vec4_t color, int team)
{
	switch (team)
	{
		case TEAM_RED:
			VectorCopy(cgs.beRedTeamColor, color);
			break;
		case TEAM_BLUE:
			VectorCopy(cgs.beBlueTeamColor, color);
			break;
		default:
			VectorCopy(colorWhite, color);
			break;
	}
	color[3] = 0.25f;
}

static qhandle_t CG_SHUDObituaryGetModIcon(int mod, qboolean unfrozen)
{
	if (unfrozen)
	{
		return cgs.media.frozenFoeTagShader;
	}

	switch (mod)
	{
		case MOD_SHOTGUN:
			return cg_weapons[WP_SHOTGUN].ammoIcon;
		case MOD_GAUNTLET:
			return cg_weapons[WP_GAUNTLET].ammoIcon;
		case MOD_MACHINEGUN:
			return cg_weapons[WP_MACHINEGUN].ammoIcon;
		case MOD_GRENADE:
			return cgs.media.obituariesGrenadeDirect;
		case MOD_GRENADE_SPLASH:
			return cg_weapons[WP_GRENADE_LAUNCHER].ammoIcon;
		case MOD_ROCKET:
			return cgs.media.obituariesRocketDirect;
		case MOD_ROCKET_SPLASH:
			return cg_weapons[WP_ROCKET_LAUNCHER].ammoIcon;
		case MOD_PLASMA:
			return cg_weapons[WP_PLASMAGUN].ammoIcon;
		case MOD_PLASMA_SPLASH:
			return cg_weapons[WP_PLASMAGUN].ammoIcon;
		case MOD_RAILGUN:
			return cg_weapons[WP_RAILGUN].ammoIcon;
		case MOD_LIGHTNING:
			return cg_weapons[WP_LIGHTNING].ammoIcon;
		case MOD_BFG:
			return cgs.media.obituariesBFGDirect;
		case MOD_BFG_SPLASH:
			return cg_weapons[WP_BFG].ammoIcon;
		case MOD_WATER:
			return cgs.media.obituariesDrowned;
		case MOD_SLIME:
			return cgs.media.obituariesMelted;
		case MOD_LAVA:
			return cgs.media.obituariesLava;
		case MOD_TELEFRAG:
			return cgs.media.obituariesTelefrag;
		case MOD_FALLING:
			return cgs.media.obituariesFallenCrashed;
		case MOD_TRIGGER_HURT:
			return cgs.media.obituariesFalling;
		case MOD_SUICIDE:
		case MOD_TARGET_LASER:
		case MOD_GRAPPLE:
		case MOD_UNKNOWN:
		case MOD_CRUSH:
			return cgs.media.obituariesSkull;
		// Mech frenzy MODs
		case MOD_LANDMINE:
		case MOD_LANDMINE_SPLASH:
			return cgs.media.regenIcon;
		case MOD_TURRET:
		case MOD_TURRET_SPLASH:
		case MOD_TURRET_EXPLOSION:
			return cgs.media.quadDamageIcon;
		case MOD_DISPENSER_EXPLOSION:
			return cgs.media.battleSuitIcon;
		default:
			return cgs.media.botSkillShaders[4];
	}
}

void CG_SHUDElementObituariesDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
