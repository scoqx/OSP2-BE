
#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

#define TEAM_STYLE2_GAP           1.0f
#define TEAM_STYLE2_HP_HFRAC      0.75f
#define TEAM_STYLE2_AP_YFRAC      0.77f
#define TEAM_STYLE2_BAR_A0        0.33f
#define TEAM_STYLE2_BAR_A1        0.66f
#define TEAM_STYLE2_DEFAULT_NAMEW 12
/* Frame thickness 1.0 at this row height; scales down for smaller squares */
#define TEAM_STYLE2_FRAME_REF_H   48.0f

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t ctxPowerup;
	superhudTextContext_t ctxName;
	superhudTextContext_t ctxHealthArmor;
	superhudDrawContext_t ctxWeapon;
	superhudTextContext_t ctxLocation;
	int index;
	int filterTeam; /* 0 = own team; TEAM_RED / TEAM_BLUE for spectator overlays */
	qboolean alignRight; /* alignh R: mirror cell order, rect.x is right edge */
	qboolean style2;
	float style2ContentW;
} shudElementTeam_t;


/* Cell left X: LTR from origin, or mirrored from origin as right edge. */
static float CG_SHUDTeamCellX(float originX, float offsetPix, float cellW, qboolean alignRight)
{
	if (alignRight)
	{
		return originX - offsetPix - cellW;
	}
	return originX + offsetPix;
}

static void CG_SHUDTeamStyle2ResolveColor(const superhudColor_t* in, vec4_t out)
{
	team_t team;
	const float* target;

	switch (in->type)
	{
		case SUPERHUD_COLOR_RGBA:
			Vector4Copy(in->rgba, out);
			return;
		case SUPERHUD_COLOR_T:
			team = CG_SHUDGetOurActiveTeam();
			target = (team == TEAM_BLUE) ? cgs.beBlueTeamColor : cgs.beRedTeamColor;
			break;
		case SUPERHUD_COLOR_E:
			team = CG_SHUDGetOurActiveTeam();
			target = (team == TEAM_BLUE) ? cgs.beRedTeamColor : cgs.beBlueTeamColor;
			break;
		case SUPERHUD_COLOR_I:
		default:
			Vector4Set(out, 1.0f, 1.0f, 1.0f, 1.0f);
			return;
	}
	Vector4Copy(target, out);
}

static void CG_SHUDTeamStyle2DefaultBarColor(const clientInfo_t* ci, qboolean isCA, vec4_t out)
{
	int team;

	if (cgs.gametype < GT_TEAM)
	{
		Vector4Set(out, 1.0f, 1.0f, 1.0f, 1.0f);
		return;
	}

	team = isCA ? ci->rt : ci->team;
	if (team != TEAM_RED && team != TEAM_BLUE)
	{
		team = ci->rt;
	}

	if (team == TEAM_BLUE)
	{
		Vector4Copy(cgs.beBlueTeamColor, out);
	}
	else if (team == TEAM_RED)
	{
		Vector4Copy(cgs.beRedTeamColor, out);
	}
	else
	{
		Vector4Set(out, 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

static void CG_SHUDTeamStyle2DrawBar(float x, float y, float maxW, float h, int value, const vec4_t rgb)
{
	vec4_t c;
	float fw;
	int base;
	int over;

	if (maxW <= 0.0f || h <= 0.0f || value <= 0)
	{
		return;
	}

	c[0] = rgb[0];
	c[1] = rgb[1];
	c[2] = rgb[2];

	/* Layer 1: 0–100 at 33% alpha */
	base = value;
	if (base > 100)
	{
		base = 100;
	}
	fw = maxW * ((float)base / 100.0f);
	c[3] = TEAM_STYLE2_BAR_A0;
	CG_FillRect(x, y, fw, h, c);

	/* Layer 2: 101–200 at 66% alpha, same origin, width = overflow/100 */
	if (value > 100)
	{
		over = value - 100;
		if (over > 100)
		{
			over = 100;
		}
		fw = maxW * ((float)over / 100.0f);
		c[3] = TEAM_STYLE2_BAR_A1;
		CG_FillRect(x, y, fw, h, c);
	}
}

static void* CG_SHUDElementTeamCreateStyle2(shudElementTeam_t* element, const superhudConfig_t* config)
{
	superhudConfig_t lcfg;
	float originX;
	float rowH;
	float square;
	float nameWidthPix;
	float totalW;
	float cellX;
	float fontW;

	element->style2 = qtrue;

	/* Do not force default color — unset means team bar color at draw time */
	memcpy(&lcfg, &element->config, sizeof(element->config));

	lcfg.alignH.isSet = qtrue;
	lcfg.alignH.value = SUPERHUD_ALIGNH_LEFT;
	lcfg.textAlign.isSet = qtrue;
	lcfg.textAlign.value = element->alignRight ? SUPERHUD_ALIGNH_RIGHT : SUPERHUD_ALIGNH_LEFT;
	/* Ignore alignV entirely — pin name Y after MakeContext */
	lcfg.alignV.isSet = qfalse;
	element->config.alignV.isSet = qfalse;

	originX = config->rect.value[0];
	rowH = config->rect.value[3];
	if (rowH < 1.0f)
	{
		rowH = 1.0f;
	}
	square = rowH;

	fontW = config->fontsize.isSet ? config->fontsize.value[0] : (rowH / 1.618f);
	if (config->rect.value[2] > 0)
	{
		nameWidthPix = config->rect.value[2];
	}
	else
	{
		nameWidthPix = TEAM_STYLE2_DEFAULT_NAMEW * fontW;
	}
	element->style2ContentW = nameWidthPix;
	totalW = square + TEAM_STYLE2_GAP + nameWidthPix;

	/* Status square */
	lcfg.rect.value[0] = CG_SHUDTeamCellX(originX, 0.0f, square, element->alignRight);
	lcfg.rect.value[1] = config->rect.value[1];
	lcfg.rect.value[2] = square;
	lcfg.rect.value[3] = rowH;
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxPowerup);

	/* Name / bar content block */
	cellX = CG_SHUDTeamCellX(originX, square + TEAM_STYLE2_GAP, nameWidthPix, element->alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = nameWidthPix;
	CG_SHUDTextMakeContext(&lcfg, &element->ctxName);
	element->ctxName.width = nameWidthPix;
	element->ctxName.flags |= DS_FORCE_COLOR;
	/* No V* flags: y is glyph top. Bottom of text = HP bar bottom − 10% of HP bar height. */
	element->ctxName.flags &= ~(DS_VTOP | DS_VCENTER | DS_VBOTTOM);
	{
		float hpH = rowH * TEAM_STYLE2_HP_HFRAC;
		float nameBottomY = config->rect.value[1] + hpH - hpH * 0.1f;

		element->ctxName.coord.named.y = nameBottomY - element->ctxName.coord.named.h;
	}
	Vector4Set(element->ctxName.color, 1.0f, 1.0f, 1.0f, 1.0f);
	if (element->alignRight)
	{
		element->ctxName.coord.named.x = cellX + nameWidthPix;
	}

	element->config.rect.value[2] = totalW;
	if (element->alignRight)
	{
		element->config.rect.value[0] = originX - totalW;
	}

	return element;
}

static void* CG_SHUDElementTeamCreateEx(const superhudConfig_t* config, int line, int filterTeam)
{

	shudTeamOverlay_t teamOverlay;

	superhudConfig_t lcfg;

	shudElementTeam_t* element;

	float nameWidthPix;
	float nameDelta;
	float originX;
	float totalW;
	float cellX;
	float cellW;
	qboolean alignRight;

	SHUD_ELEMENT_INIT(element, config);

	element->index = line;
	element->filterTeam = filterTeam;
	element->style2 = qfalse;
	element->style2ContentW = 0.0f;

	alignRight = (config->alignH.isSet && config->alignH.value == SUPERHUD_ALIGNH_RIGHT)
	             ? qtrue : qfalse;
	element->alignRight = alignRight;

	/* Keep vertical centering; do not override alignH / textAlign from HUD config */
	element->config.alignV.isSet = qtrue;
	element->config.alignV.value = SUPERHUD_ALIGNV_CENTER;

	if (config->style.isSet && config->style.value == 2)
	{
		return CG_SHUDElementTeamCreateStyle2(element, config);
	}

	CG_SHUDElementCompileTeamOverlayConfig(config->fontsize.value[0],
	                                       (config->style.isSet && config->style.value == 1) ? 1 : 0, &teamOverlay);

	nameWidthPix = teamOverlay.nameLenPix;

	if (!element->config.color.isSet)
	{
		element->config.color.isSet = qtrue;
		element->config.color.value.type = SUPERHUD_COLOR_RGBA;
		Vector4Set(element->config.color.value.rgba, 1, 1, 1, 1);
	}

	memcpy(&lcfg, &element->config, sizeof(element->config));

	/* Per-cell contexts: alignH LEFT so Make* uses rect.x as cell left.
	 * textAlign mirrors with row alignh so nick/loc/hp anchor toward HP after mirror. */
	lcfg.alignH.isSet = qtrue;
	lcfg.alignH.value = SUPERHUD_ALIGNH_LEFT;
	lcfg.textAlign.isSet = qtrue;
	lcfg.textAlign.value = alignRight ? SUPERHUD_ALIGNH_RIGHT : SUPERHUD_ALIGNH_LEFT;

	/* rect W (value[2]) > 0 = width in pixels for nickname; 0 = use overlay default */
	if (config->rect.value[2] > 0)
		nameWidthPix = config->rect.value[2];
	nameDelta = nameWidthPix - teamOverlay.nameLenPix;

	originX = config->rect.value[0];
	totalW = teamOverlay.overlayWidthPix + nameDelta;

	// setup powerup
	cellW = teamOverlay.powerupLenPix;
	cellX = CG_SHUDTeamCellX(originX, teamOverlay.powerupOffsetPix, cellW, alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = cellW;
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxPowerup);

	// setup name
	cellW = nameWidthPix;
	cellX = CG_SHUDTeamCellX(originX, teamOverlay.nameOffsetPix, cellW, alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = cellW;
	CG_SHUDTextMakeContext(&lcfg, &element->ctxName);
	element->ctxName.width = nameWidthPix;
	if (alignRight)
	{
		/* DS_HRIGHT: x is right edge of text */
		element->ctxName.coord.named.x = cellX + cellW;
	}

	// setup health and armor
	cellW = teamOverlay.healthAndArmorLenPix;
	cellX = CG_SHUDTeamCellX(originX, teamOverlay.healthAndArmorOffsetPix + nameDelta, cellW, alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = cellW;
	CG_SHUDTextMakeContext(&lcfg, &element->ctxHealthArmor);
	element->ctxHealthArmor.width = teamOverlay.healthAndArmorLenPix;
	element->ctxHealthArmor.flags |= DS_FORCE_COLOR;
	if (alignRight)
	{
		element->ctxHealthArmor.coord.named.x = cellX + cellW;
	}

	// setup weapon
	cellW = teamOverlay.weaponLenPix;
	cellX = CG_SHUDTeamCellX(originX, teamOverlay.weaponOffsetPix + nameDelta, cellW, alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = cellW;
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxWeapon);

	// setup location
	cellW = teamOverlay.locationLenPix;
	cellX = CG_SHUDTeamCellX(originX, teamOverlay.locationOffsetPix + nameDelta, cellW, alignRight);
	lcfg.rect.value[0] = cellX;
	lcfg.rect.value[2] = cellW;
	CG_SHUDTextMakeContext(&lcfg, &element->ctxLocation);
	element->ctxLocation.width = teamOverlay.locationLenPix;
	if (alignRight)
	{
		element->ctxLocation.coord.named.x = cellX + cellW;
	}

	/* Fill/border AABB: LEFT keeps rect.x; RIGHT hangs left of rect.x */
	element->config.rect.value[2] = totalW;
	if (alignRight)
	{
		element->config.rect.value[0] = originX - totalW;
	}

	return element;
}

static void* CG_SHUDElementTeamCreate(const superhudConfig_t* config, int line)
{
	return CG_SHUDElementTeamCreateEx(config, line, 0);
}

void* CG_SHUDElementTeam1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 1);
}

void* CG_SHUDElementTeam2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 2);
}

void* CG_SHUDElementTeam3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 3);
}

void* CG_SHUDElementTeam4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 4);
}

void* CG_SHUDElementTeam5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 5);
}

void* CG_SHUDElementTeam6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 6);
}

void* CG_SHUDElementTeam7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 7);
}

void* CG_SHUDElementTeam8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 8);
}

void* CG_SHUDElementTeam9Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 9);
}

void* CG_SHUDElementTeam10Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 10);
}

void* CG_SHUDElementTeam11Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 11);
}

void* CG_SHUDElementTeam12Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 12);
}

void* CG_SHUDElementTeam13Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 13);
}

void* CG_SHUDElementTeam14Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 14);
}

void* CG_SHUDElementTeam15Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 15);
}

void* CG_SHUDElementTeam16Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreate(config, 16);
}

void* CG_SHUDElementTeamRed1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 1, TEAM_RED);
}

void* CG_SHUDElementTeamRed2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 2, TEAM_RED);
}

void* CG_SHUDElementTeamRed3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 3, TEAM_RED);
}

void* CG_SHUDElementTeamRed4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 4, TEAM_RED);
}

void* CG_SHUDElementTeamRed5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 5, TEAM_RED);
}

void* CG_SHUDElementTeamRed6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 6, TEAM_RED);
}

void* CG_SHUDElementTeamRed7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 7, TEAM_RED);
}

void* CG_SHUDElementTeamRed8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 8, TEAM_RED);
}

void* CG_SHUDElementTeamBlue1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 1, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 2, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 3, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 4, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 5, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 6, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 7, TEAM_BLUE);
}

void* CG_SHUDElementTeamBlue8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamCreateEx(config, 8, TEAM_BLUE);
}

static void CG_SHUDElementTeamRoutineStyle2(shudElementTeam_t* element, clientInfo_t* ci, int clientNum, qboolean isSpectator, qboolean isCA)
{
	float rowY;
	float rowH;
	float contentX;
	float contentW;
	float hpH;
	float apY;
	float apH;
	vec4_t hpColor;
	vec4_t apColor;
	vec4_t white;
	vec4_t frameSize;
	int k;
	gitem_t* gi;

	rowY = element->config.rect.value[1];
	rowH = element->config.rect.value[3];
	contentW = element->style2ContentW;

	if (element->alignRight)
	{
		contentX = element->config.rect.value[0];
	}
	else
	{
		contentX = element->config.rect.value[0] + element->ctxPowerup.coord.named.w + TEAM_STYLE2_GAP;
	}

	if (element->config.color.isSet)
	{
		CG_SHUDTeamStyle2ResolveColor(&element->config.color.value, hpColor);
	}
	else
	{
		CG_SHUDTeamStyle2DefaultBarColor(ci, isCA, hpColor);
	}

	if (element->config.color2.isSet)
	{
		CG_SHUDTeamStyle2ResolveColor(&element->config.color2.value, apColor);
	}
	else
	{
		Vector4Copy(hpColor, apColor);
	}

	/* Low threshold: both bars use beHealthLowColor when total HP+AP <= cg_healthLow */
	if (cg_healthColorLevels.integer && ci->health > 0 &&
	        CG_GetTotalHitPoints(ci->health, ci->armor) <= cg_healthLow.integer)
	{
		VectorCopy(cg.beHealthLowColor, hpColor);
		hpColor[3] = 1.0f;
		VectorCopy(cg.beHealthLowColor, apColor);
		apColor[3] = 1.0f;
	}

	hpH = rowH * TEAM_STYLE2_HP_HFRAC;
	apY = rowY + rowH * TEAM_STYLE2_AP_YFRAC;
	apH = rowH - (rowH * TEAM_STYLE2_AP_YFRAC);
	if (apH < 1.0f)
	{
		apH = 1.0f;
	}

	CG_SHUDTeamStyle2DrawBar(contentX, rowY, contentW, hpH, ci->health, hpColor);
	CG_SHUDTeamStyle2DrawBar(contentX, apY, contentW, apH, ci->armor, apColor);

	Vector4Set(element->ctxName.color, 1.0f, 1.0f, 1.0f, 1.0f);
	element->ctxName.text = ci->name_clean;
	CG_SHUDTextPrintNew(&element->config, &element->ctxName, qfalse);

	/* Status square icons — white tint (do not use color/color2 bar colors) */
	Vector4Set(white, 1.0f, 1.0f, 1.0f, 1.0f);
	if (ci->health <= 0)
	{
		if (cgs.osp.gameTypeFreeze && cg.warmup == 0)
		{
			element->ctxPowerup.image = cgs.media.snowflakeShader;
		}
		else
		{
			element->ctxPowerup.image = cgs.media.teamTasksDead;
		}
		CG_SHUDDrawStretchPic(element->ctxPowerup.coord, element->ctxPowerup.coordPicture,
		                      white, element->ctxPowerup.image);
	}
	else
	{
		element->ctxPowerup.image = cg_weapons[ci->curWeapon].ammoIcon ?
		                           cg_weapons[ci->curWeapon].ammoIcon : cgs.media.deferShader;
		CG_SHUDDrawStretchPic(element->ctxPowerup.coord, element->ctxPowerup.coordPicture,
		                      white, element->ctxPowerup.image);

		k = 0;
		do
		{
			if (ci->teamPowerups & (1 << k))
			{
				if (k == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
				{
					continue;
				}
				gi = BG_FindItemForPowerup(k);
				if (gi)
				{
					element->ctxPowerup.image = trap_R_RegisterShader(gi->icon);
					CG_SHUDDrawStretchPic(element->ctxPowerup.coord, element->ctxPowerup.coordPicture,
					                      white, element->ctxPowerup.image);
				}
			}
		}
		while (++k < 16);
	}

	/* Followed: white inner frame. Else damage blink: red only (beHealthLowColor),
	 * on/off with same phase timing as float sprite / team indicator. */
	{
		float frameT = element->ctxPowerup.coord.named.h / TEAM_STYLE2_FRAME_REF_H;

		if (frameT < 0.25f)
		{
			frameT = 0.25f;
		}
		Vector4Set(frameSize, frameT, frameT, frameT, frameT);
	}
	if (isSpectator && cg.snap && clientNum == cg.snap->ps.clientNum &&
	        cg.snap->ps.clientNum != cg.clientNum)
	{
		CG_OSPDrawFrameAdjusted(element->ctxPowerup.coord.named.x,
		                        element->ctxPowerup.coord.named.y,
		                        element->ctxPowerup.coord.named.w,
		                        element->ctxPowerup.coord.named.h,
		                        frameSize, white, qtrue);
	}
	else if (ci->health > 0 && cg.healthBlinkUntil[clientNum] > cg.time)
	{
		int until;
		int elapsed;
		int phase;
		qboolean showRed;

		until = cg.healthBlinkUntil[clientNum];
		elapsed = cg.time - (until - CG_HEALTH_BLINK_MSEC);
		if (elapsed < 0)
		{
			elapsed = 0;
		}
		phase = elapsed / CG_HEALTH_BLINK_PHASE_MSEC;
		/* Same on-phases as CG_ClientInfoUpdateHealthBlinkPhases low-color flashes */
		if (cg.allyHealthBlinkMidLow[clientNum])
		{
			showRed = (phase & 1) ? qtrue : qfalse;
		}
		else
		{
			showRed = (phase & 1) ? qfalse : qtrue;
		}
		if (showRed && phase < 3)
		{
			vec4_t blinkColor;

			VectorCopy(cg.beHealthLowColor, blinkColor);
			blinkColor[3] = 1.0f;
			CG_OSPDrawFrameAdjusted(element->ctxPowerup.coord.named.x,
			                        element->ctxPowerup.coord.named.y,
			                        element->ctxPowerup.coord.named.w,
			                        element->ctxPowerup.coord.named.h,
			                        frameSize, blinkColor, qtrue);
		}
	}
}

void CG_SHUDElementTeamRoutine(void* context)
{
	shudElementTeam_t* element = (shudElementTeam_t*)context;
	clientInfo_t* ci;
	clientInfo_t* localCi;
	int cnt = 0;
	int index;
	int targetTeam;
	qboolean isSpectator;
	qboolean isCA;
	qboolean noTeamFilter;
	int playerTeam;

	if (!cg_drawTeamOverlay.integer)
	{
		return;
	}

	isSpectator = CG_IS_LOCAL_CLIENT_SPECTATOR();
	isCA = CG_OSPIsGameTypeCA(cgs.gametype);
	noTeamFilter = qfalse;
	localCi = &cgs.clientinfo[cg.clientNum];

	if (element->filterTeam)
	{
		/* teamRed / teamBlue: spectators in team modes only */
		if (!isSpectator || cgs.gametype < GT_TEAM)
		{
			return;
		}
		targetTeam = element->filterTeam;
	}
	else if (cgs.gametype >= GT_TEAM)
	{
		/* team1-16: players only — own team (CA uses rt, else team) */
		if (isSpectator)
		{
			return;
		}
		targetTeam = isCA ? localCi->rt : localCi->team;
		if (targetTeam != TEAM_RED && targetTeam != TEAM_BLUE)
		{
			/* OSP "st" can remap team; real side stays in rt */
			targetTeam = localCi->rt;
		}
	}
	else
	{
		/* team1-16: FFA/tourney spectators — all players by client ID order */
		if (!isSpectator)
		{
			return;
		}
		noTeamFilter = qtrue;
		targetTeam = 0; /* unused when noTeamFilter */
	}

	for (index = 0; index < numSortedTeamPlayers; ++index)
	{
		ci = &cgs.clientinfo[sortedTeamPlayers[index]];
		if (ci->infoValid)
		{
			/* Local player does not take a team overlay row */
			if (sortedTeamPlayers[index] == cg.clientNum)
			{
				continue;
			}
			if (!noTeamFilter)
			{
				playerTeam = isCA ? ci->rt : ci->team;
				if (playerTeam != targetTeam)
				{
					/* non-CA: team may be remapped by "st" — fall back to rt */
					if (isCA || ci->rt != targetTeam)
					{
						continue;
					}
				}
			}
			if (++cnt == element->index)
			{
				break;
			}
		}
	}

	if (cnt != element->index)
	{
		//no elements
		return;
	}
	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);
	//get player
	ci = &cgs.clientinfo[sortedTeamPlayers[index]];

	if (element->style2)
	{
		CG_SHUDElementTeamRoutineStyle2(element, ci, sortedTeamPlayers[index], isSpectator, isCA);
		return;
	}

	// draw name
	element->ctxName.text = ci->name;
	CG_SHUDTextPrint(&element->config, &element->ctxName);

	// draw health and armor
	if (cg_teamHealthBar.integer >= 1)
	{
		float barX;

		barX = element->ctxHealthArmor.coord.named.x;
		if (element->alignRight)
		{
			barX -= (float)element->ctxHealthArmor.width;
		}
		CG_DrawHealthBar(barX,
		                 element->config.rect.value[1],
		                 (float)element->ctxHealthArmor.width,
		                 element->config.rect.value[3],
		                 ci->health, ci->armor, element->config.color.value.rgba[3],
		                 (cg_teamHealthBar.integer == 2) ? element->ctxHealthArmor.fontIndex : -1, qfalse,
		                 &cg.healthColorByClient[sortedTeamPlayers[index]], qtrue);
	}
	else
	{
		Vector4Copy(cg.healthColorByClient[sortedTeamPlayers[index]], element->ctxHealthArmor.color);
		element->ctxHealthArmor.text = va("%3i/%i", ci->health, ci->armor);

		CG_OSPDrawStringNew(element->ctxHealthArmor.coord.named.x,
		                    element->ctxHealthArmor.coord.named.y,
		                    element->ctxHealthArmor.text,
		                    element->ctxHealthArmor.color,
		                    element->ctxHealthArmor.shadowColor,
		                    element->ctxHealthArmor.coord.named.w,
		                    element->ctxHealthArmor.coord.named.h,
		                    SCREEN_WIDTH,
		                    element->ctxHealthArmor.flags,
		                    element->ctxHealthArmor.fontIndex,
		                    NULL, NULL, NULL);
	}

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		// style 1: one icon left of name — dead status (red) or powerup if any
		if (cgs.osp.gameTypeFreeze && ci->health <= 0)
		{
			vec4_t redColor;
			Vector4Set(redColor, 1.0f, 0.0f, 0.0f, 1.0f);
			element->ctxPowerup.image = cgs.media.noammoShader;
			CG_SHUDDrawStretchPic(element->ctxPowerup.coord, element->ctxPowerup.coordPicture, redColor, element->ctxPowerup.image);
		}
		else
		{
			int k = 0;
			gitem_t* gi;

			do
			{
				if (ci->teamPowerups & (1 << k))
				{
					if (k == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
					{
						continue;
					}
					gi = BG_FindItemForPowerup(k);
					if (gi)
					{
						element->ctxPowerup.image = trap_R_RegisterShader(gi->icon);
						CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxPowerup);
					}
				}
			}
			while (++k < 16);
		}
	}
	else
	{
		// draw weapon
		element->ctxWeapon.image = cg_weapons[ci->curWeapon].ammoIcon ?  cg_weapons[ci->curWeapon].ammoIcon : cgs.media.deferShader;
		CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxWeapon);

		// draw powerup
		{
			int k = 0;
			gitem_t* gi;

			do
			{
				if (cgs.osp.gameTypeFreeze && ci->health <= 0)
				{
					element->ctxPowerup.image = cgs.media.noammoShader;
					CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxPowerup);
					break;
				}
				else if (ci->teamPowerups & (1 << k))
				{
					if (k == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
					{
						continue;
					}
					gi = BG_FindItemForPowerup(k);
					if (gi)
					{
						element->ctxPowerup.image = trap_R_RegisterShader(gi->icon);
						CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxPowerup);
					}
				}
			}
			while (++k < 16);
		}

		// draw location
		{
			if (customLocationsEnabled != 0)
			{
				element->ctxLocation.text = CG_CustomLocationsGetName(ci->customLocation);
			}
			else
			{
				if (ch_ColorLocations.integer != 0 && cgs.gametype == GT_CTF)
				{
					element->ctxLocation.text = CG_GetCTFLocation(ci->location);
				}
				else
				{
					element->ctxLocation.text = CG_LocationName(ci->location);
				}
			}
			if (!element->ctxLocation.text || *element->ctxLocation.text == 0)
			{
				element->ctxLocation.text = "unknown";
			}

			CG_SHUDTextPrint(&element->config, &element->ctxLocation);
		}
	}
}

void CG_SHUDElementTeamDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
