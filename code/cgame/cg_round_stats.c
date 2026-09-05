/*
===========================================================================
cg_round_stats.c

CA round damage table (GIVEN / TAKEN) — personalized pairwise + weapons.

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com
===========================================================================
*/

#include "cg_local.h"

#define ROUNDSTATS_SHOW_MS 5000
#define ROUNDSTATS_NAME_SCALE 1.0f
#define ROUNDSTATS_NAME_MAX_CHARS 13
#define ROUNDSTATS_MAX_WEAP_COLS (WP_NUM_WEAPONS - 1)

typedef struct
{
	float x;
	float y;
	float textWidth;
	float textHeight;
	float nameWidth;
	float nameHeight;
	float rowHeight;
	float iconSize;
	float padding;
	float colSpacing;
	float nameColWidth;
	float totalColWidth;
	float weapColWidth;
	float iconDmgGap;
	float widthCutoff;
	vec4_t bgColor;
	vec4_t borderColor;
	vec4_t colorWhite;
} roundStatsDrawSettings_t;

static roundStatsDrawSettings_t rsDraw;

static void CG_RoundStatsInitColors(void)
{
	vec4_t teamColor;
	team_t team;

	Vector4Copy(colorWhite, rsDraw.colorWhite);

	if (cgs.gametype >= GT_TEAM && cg.snap)
	{
		team = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		       ? cgs.clientinfo[cg.clientNum].team
		       : cg.snap->ps.persistant[PERS_TEAM];

		if (team == TEAM_RED)
			Vector4Copy(cgs.beRedTeamColor, teamColor);
		else if (team == TEAM_BLUE)
			Vector4Copy(cgs.beBlueTeamColor, teamColor);
		else
			Vector4Copy(colorDkGrey, teamColor);
	}
	else
	{
		Vector4Copy(colorDkGrey, teamColor);
	}

	if (!cg.roundStatsSettings.bgColorIsSet)
	{
		CG_OSPAdjustTeamColorBEStats(teamColor, rsDraw.bgColor);
		rsDraw.bgColor[3] = cg_roundStats_bgOpaque.value;
	}
	else
	{
		Vector4Set(rsDraw.bgColor,
		           cg.roundStatsSettings.bgColor[0],
		           cg.roundStatsSettings.bgColor[1],
		           cg.roundStatsSettings.bgColor[2],
		           cg_roundStats_bgOpaque.value);
	}

	if (cg.roundStatsSettings.borderColorIsSet)
	{
		Vector4Copy(cg.roundStatsSettings.borderColor, rsDraw.borderColor);
		rsDraw.borderColor[3] = 1.0f;
	}
	else
	{
		Vector4Copy(colorBlack, rsDraw.borderColor);
	}
}

static void CG_RoundStatsInitSettings(void)
{
	globalRoundStatsSettings_t *inc = &cg.roundStatsSettings;
	float weapW;

	rsDraw.x = inc->x;
	rsDraw.y = inc->y;
	rsDraw.textWidth = inc->textSize[0];
	rsDraw.textHeight = inc->textSize[1];
	if (rsDraw.textWidth < 1.0f)
		rsDraw.textWidth = 6.0f;
	if (rsDraw.textHeight < 1.0f)
		rsDraw.textHeight = 8.0f;

	rsDraw.iconSize = rsDraw.textHeight;
	rsDraw.rowHeight = rsDraw.textHeight; /* icon + damage on one line */
	rsDraw.nameWidth = rsDraw.textWidth * ROUNDSTATS_NAME_SCALE;
	rsDraw.nameHeight = rsDraw.textHeight * ROUNDSTATS_NAME_SCALE;
	rsDraw.padding = (rsDraw.textHeight + rsDraw.textWidth) / 8.0f;
	rsDraw.colSpacing = rsDraw.textWidth * 0.25f * cg_roundStats_spacingAdjust.value;
	rsDraw.iconDmgGap = rsDraw.textWidth * 0.15f;
	rsDraw.nameColWidth = rsDraw.nameWidth * (float)ROUNDSTATS_NAME_MAX_CHARS;
	rsDraw.totalColWidth = rsDraw.textWidth * 6.0f; /* "[9999]" */

	/* icon | gap | up to 3-digit dmg */
	weapW = rsDraw.iconSize + rsDraw.iconDmgGap + rsDraw.textWidth * 3.0f;
	rsDraw.weapColWidth = weapW;

	rsDraw.widthCutoff = rsDraw.textWidth * cg_roundStats_widthCutoff.value;

	CG_RoundStatsInitColors();
}

/*
 * Build ordered weapon column list from union of given+taken masks across entries.
 */
static int CG_RoundStatsBuildWeapCols(int *weapCols, int maxCols)
{
	int i, w;
	int unionMask = 0;
	int n = 0;

	for (i = 0; i < cg.roundStatsCount; i++)
	{
		unionMask |= cg.roundStats[i].weapGivenMask;
		unionMask |= cg.roundStats[i].weapTakenMask;
	}

	for (w = WP_NONE + 1; w < WP_NUM_WEAPONS && n < maxCols; w++)
	{
		if (unionMask & (1 << w))
			weapCols[n++] = w;
	}
	return n;
}

static float CG_RoundStatsContentWidth(int nWeapCols)
{
	float w;

	w = rsDraw.nameColWidth + rsDraw.colSpacing + rsDraw.totalColWidth;
	if (nWeapCols > 0)
		w += rsDraw.colSpacing + nWeapCols * rsDraw.weapColWidth
		     + (nWeapCols - 1) * rsDraw.colSpacing;
	return w;
}

static float CG_RoundStatsHeaderWidth(const char *label, int total)
{
	char buf[64];
	float labelW;
	float totalW;

	Com_sprintf(buf, sizeof(buf), "%s", label);
	labelW = CG_OSPDrawStringNewTextWidth(buf, rsDraw.textWidth, rsDraw.textHeight,
	                                      DS_PROPORTIONAL | DS_SHADOW, cgFontIndices.roundStats);
	Com_sprintf(buf, sizeof(buf), "[%d]", total);
	totalW = CG_OSPDrawStringNewTextWidth(buf, rsDraw.textWidth, rsDraw.textHeight,
	                                      DS_PROPORTIONAL | DS_SHADOW, cgFontIndices.roundStats);
	return labelW + rsDraw.colSpacing + totalW;
}

/*
 * Sort indices by section damage descending (insertion sort, small N).
 */
static void CG_RoundStatsSortIndices(int *idx, int n, qboolean givenSection)
{
	int i, j, key;
	int keyDmg, curDmg;

	for (i = 1; i < n; i++)
	{
		key = idx[i];
		keyDmg = givenSection ? cg.roundStats[key].dmgGiven : cg.roundStats[key].dmgTaken;
		j = i - 1;
		while (j >= 0)
		{
			curDmg = givenSection ? cg.roundStats[idx[j]].dmgGiven : cg.roundStats[idx[j]].dmgTaken;
			if (curDmg >= keyDmg)
				break;
			idx[j + 1] = idx[j];
			j--;
		}
		idx[j + 1] = key;
	}
}

/* Same left X as player nicknames; total immediately after label as [N]. */
static void CG_RoundStatsDrawHeader(float *y, const char *label, int total)
{
	char totalBuf[32];
	float labelW;
	float textFlags = DS_PROPORTIONAL | DS_SHADOW;
	float x = rsDraw.x + rsDraw.padding;

	labelW = CG_OSPDrawStringNewTextWidth(label, rsDraw.textWidth, rsDraw.textHeight,
	                                      textFlags, cgFontIndices.roundStats);
	Com_sprintf(totalBuf, sizeof(totalBuf), "[%d]", total);

	CG_OSPDrawStringNew(x, *y, label, rsDraw.colorWhite, colorBlack,
	                    rsDraw.textWidth, rsDraw.textHeight, SCREEN_WIDTH, textFlags,
	                    cgFontIndices.roundStats, NULL, NULL, NULL);

	CG_OSPDrawStringNew(x + labelW + rsDraw.colSpacing, *y, totalBuf,
	                    rsDraw.colorWhite, colorBlack,
	                    rsDraw.textWidth, rsDraw.textHeight, SCREEN_WIDTH, textFlags,
	                    cgFontIndices.roundStats, NULL, NULL, NULL);

	*y += rsDraw.textHeight + rsDraw.padding * 0.5f;
}

static void CG_RoundStatsDrawPlayerRow(float *y, const roundStatsEntry_t *e, qboolean givenSection,
                                      const int *weapCols, int nWeapCols)
{
	const clientInfo_t *ci;
	const char *name;
	const int *weapDmgs;
	int col;
	float rowY = *y;
	float nameY;
	float totalX;
	float weapX;
	float dmgX;
	float iconY;
	int textFlags = DS_PROPORTIONAL | DS_SHADOW;
	char dmgBuf[16];
	float dmgW;
	int total;

	if (e->clientNum < 0 || e->clientNum >= MAX_CLIENTS)
		return;

	ci = &cgs.clientinfo[e->clientNum];
	name = ci->name[0] ? ci->name : "???";
	weapDmgs = givenSection ? e->weapGiven : e->weapTaken;
	total = givenSection ? e->dmgGiven : e->dmgTaken;

	nameY = rowY + (rsDraw.rowHeight - rsDraw.nameHeight) * 0.5f;
	iconY = rowY + (rsDraw.rowHeight - rsDraw.iconSize) * 0.5f;

	CG_OSPDrawStringNew(rsDraw.x + rsDraw.padding, nameY, name, rsDraw.colorWhite, colorBlack,
	                    rsDraw.nameWidth, rsDraw.nameHeight, ROUNDSTATS_NAME_MAX_CHARS,
	                    textFlags | DS_MAX_WIDTH_IS_CHARS,
	                    cgFontIndices.roundStats, NULL, NULL, NULL);

	totalX = rsDraw.x + rsDraw.padding + rsDraw.nameColWidth + rsDraw.colSpacing;
	Com_sprintf(dmgBuf, sizeof(dmgBuf), "[%d]", total);
	dmgW = CG_OSPDrawStringNewTextWidth(dmgBuf, rsDraw.textWidth, rsDraw.textHeight,
	                                    textFlags, cgFontIndices.roundStats);
	CG_OSPDrawStringNew(totalX + (rsDraw.totalColWidth - dmgW) * 0.5f,
	                    nameY, dmgBuf,
	                    rsDraw.colorWhite, colorBlack,
	                    rsDraw.textWidth, rsDraw.textHeight, SCREEN_WIDTH, textFlags,
	                    cgFontIndices.roundStats, NULL, NULL, NULL);

	weapX = totalX + rsDraw.totalColWidth + rsDraw.colSpacing;

	for (col = 0; col < nWeapCols; col++)
	{
		int w = weapCols[col];
		int dmg = weapDmgs[w];

		if (dmg > 0)
		{
			/* icon right-edge to gap; dmg left-aligned after gap (fixed column split) */
			dmgX = weapX + rsDraw.iconSize + rsDraw.iconDmgGap;

			if (cg_weapons[w].weaponIcon)
			{
				CG_DrawPicWithColor(weapX, iconY, rsDraw.iconSize, rsDraw.iconSize,
				                    colorWhite, cg_weapons[w].weaponIcon);
			}

			Com_sprintf(dmgBuf, sizeof(dmgBuf), "%d", dmg);
			CG_OSPDrawStringNew(dmgX, nameY, dmgBuf,
			                    rsDraw.colorWhite, colorBlack,
			                    rsDraw.textWidth, rsDraw.textHeight, SCREEN_WIDTH, textFlags,
			                    cgFontIndices.roundStats, NULL, NULL, NULL);
		}

		weapX += rsDraw.weapColWidth + rsDraw.colSpacing;
	}

	*y += rsDraw.rowHeight + rsDraw.padding * 0.15f;
}

static void CG_RoundStatsDrawSeparator(float *y, float windowWidth)
{
	float lineY = *y + rsDraw.padding * 0.25f;

	CG_FillRect(rsDraw.x, lineY, windowWidth, 1.0f, rsDraw.borderColor);
	*y = lineY + 1.0f + rsDraw.padding * 0.5f;
}

qboolean CG_RoundStatsIsShowing(void)
{
#if 0
	if (cgs.gamename_unfreeze)
		return qfalse;
#endif

	/* +roundstats: show last cached snapshot, no server request */
	if (cg.roundStatsHold && cg.roundStatsSupported)
		return qtrue;

	if (!cg.roundStatsShow)
		return qfalse;

	/* If intermission started after receive, keep until IM ends */
	if (cg.intermissionStarted)
	{
		cg.roundStatsShowUntilIm = qtrue;
		return qtrue;
	}

	if (cg.roundStatsShowUntilIm)
	{
		/* Intermission ended */
		cg.roundStatsShow = qfalse;
		cg.roundStatsShowUntilIm = qfalse;
		return qfalse;
	}

	if (cg.time - cg.roundStatsShowTime < ROUNDSTATS_SHOW_MS)
		return qtrue;

	cg.roundStatsShow = qfalse;
	return qfalse;
}

void CG_RoundStatsShow(void)
{
	int i;
	int givenIdx[ROUNDSTATS_MAX_ENTRIES];
	int takenIdx[ROUNDSTATS_MAX_ENTRIES];
	int weapCols[ROUNDSTATS_MAX_WEAP_COLS];
	int givenN = 0;
	int takenN = 0;
	int nWeapCols;
	float maxWidth = 0.0f;
	float contentHeight;
	float windowWidth;
	float windowHeight;
	float y;
	float hdrGivenW;
	float hdrTakenW;
	float rowW;
	float rowPad;

	if (!CG_RoundStatsIsShowing())
		return;

	CG_RoundStatsInitSettings();

	for (i = 0; i < cg.roundStatsCount; i++)
	{
		if (cg.roundStats[i].dmgGiven > 0)
			givenIdx[givenN++] = i;
		if (cg.roundStats[i].dmgTaken > 0)
			takenIdx[takenN++] = i;
	}

	CG_RoundStatsSortIndices(givenIdx, givenN, qtrue);
	CG_RoundStatsSortIndices(takenIdx, takenN, qfalse);

	nWeapCols = CG_RoundStatsBuildWeapCols(weapCols, ROUNDSTATS_MAX_WEAP_COLS);

	hdrGivenW = CG_RoundStatsHeaderWidth("^2GIVEN", cg.roundStatsMyGiven);
	hdrTakenW = CG_RoundStatsHeaderWidth("^3TAKEN", cg.roundStatsMyTaken);
	if (hdrGivenW > maxWidth)
		maxWidth = hdrGivenW;
	if (hdrTakenW > maxWidth)
		maxWidth = hdrTakenW;

	rowW = CG_RoundStatsContentWidth(nWeapCols);
	if (rowW > maxWidth)
		maxWidth = rowW;

	maxWidth -= rsDraw.widthCutoff;
	if (maxWidth < rsDraw.textWidth * 10.0f)
		maxWidth = rsDraw.textWidth * 10.0f;

	windowWidth = maxWidth + rsDraw.padding * 2.0f;

	rowPad = rsDraw.rowHeight + rsDraw.padding * 0.15f;
	contentHeight = rsDraw.padding;
	contentHeight += rsDraw.textHeight + rsDraw.padding * 0.5f; /* GIVEN header */
	contentHeight += givenN * rowPad;
	contentHeight += rsDraw.padding * 0.25f + 1.0f + rsDraw.padding * 0.5f; /* separator */
	contentHeight += rsDraw.textHeight + rsDraw.padding * 0.5f; /* TAKEN header */
	contentHeight += takenN * rowPad;
	contentHeight += rsDraw.padding;
	windowHeight = contentHeight;

	CG_FillRect(rsDraw.x, rsDraw.y, windowWidth, windowHeight, rsDraw.bgColor);
	CG_OSPDrawFrameAdjusted(rsDraw.x, rsDraw.y, windowWidth, windowHeight,
	                        defaultBorderSize, rsDraw.borderColor, qfalse);

	y = rsDraw.y + rsDraw.padding;
	CG_RoundStatsDrawHeader(&y, "^2GIVEN", cg.roundStatsMyGiven);

	for (i = 0; i < givenN; i++)
		CG_RoundStatsDrawPlayerRow(&y, &cg.roundStats[givenIdx[i]], qtrue, weapCols, nWeapCols);

	CG_RoundStatsDrawSeparator(&y, windowWidth);
	CG_RoundStatsDrawHeader(&y, "^3TAKEN", cg.roundStatsMyTaken);

	for (i = 0; i < takenN; i++)
		CG_RoundStatsDrawPlayerRow(&y, &cg.roundStats[takenIdx[i]], qfalse, weapCols, nWeapCols);
}
