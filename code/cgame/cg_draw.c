/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "superhud.h"

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

int frameTime = 0;

int statsInfo[24];

const char* monthName[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char* dayOfWeekName[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
void CG_DrawField(int x, int y, int width, int value, int w, int h)
{
	char    num[16], *ptr;
	int     l;
	int     frame;

	if (width < 1)
	{
		return;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
		case 1:
			value = value > 9 ? 9 : value;
			value = value < 0 ? 0 : value;
			break;
		case 2:
			value = value > 99 ? 99 : value;
			value = value < -9 ? -9 : value;
			break;
		case 3:
			value = value > 999 ? 999 : value;
			value = value < -99 ? -99 : value;
			break;
		case 4:
			value = value > 9999 ? 9999 : value;
			value = value < -999 ? -999 : value;
			break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + w * (width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		CG_DrawPicOld(x, y, w, h, cgs.media.numberShaders[frame]);
		x += w;
		ptr++;
		l--;
	}
}

void CG_OSPDrawField(int x, int y, int val)
{
	if (val >= 0 && val < 10)
	{
		CG_DrawField(x, y, 1, val, 32, 48);
	}
	else if ((val > -9 && val < 0) || (val >= 10 && val < 100))
	{
		CG_DrawField(x, y, 2, val, 32, 48);
	}
	else
	{
		CG_DrawField(x, y, 3, val, 32, 48);
	}
	return;
}

/*
================
CG_Draw3DModel

================
*/
static void CG_Draw3DModelWithColor(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, const vec4_t color)
{
	refdef_t        refdef;
	refEntity_t     ent;

	if (!cg_draw3dIcons.integer || !cg_drawIcons.integer)
	{
		return;
	}

	CG_AdjustFrom640_Old(&x, &y, &w, &h, qfalse);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;     // no stencil shadows
	if (color)
	{
		ent.shaderRGBA[0] = (byte)(color[0] * 255.0f);
		ent.shaderRGBA[1] = (byte)(color[1] * 255.0f);
		ent.shaderRGBA[2] = (byte)(color[2] * 255.0f);
		ent.shaderRGBA[3] = (byte)(color[3] * 255.0f);
	}

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles)
{
	CG_Draw3DModelWithColor(x, y, w, h, model, skin, origin, angles, NULL);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(float x, float y, float w, float h, int clientNum, vec3_t headAngles)
{
	clipHandle_t    cm;
	clientInfo_t*    ci;
	float           len;
	vec3_t          origin;
	vec3_t          mins, maxs;
	vec3_t          headColor;
	qhandle_t       headSkin;

	ci = &cgs.clientinfo[ clientNum ];

	if (cg_drawIcons.integer && cg_drawUniqueColors.integer > 0 && cg_drawUniqueColors.integer < 4 && clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		vec4_t uniqueColor;
		vec4_t borderSize;
		VectorCopy(UNIQUE_COLOR(clientNum), uniqueColor);
		uniqueColor[3] = 1.0f;

		if (cg_drawUniqueColors.integer == 1)
		{
			CG_FillRect(x, y, w, h, uniqueColor);
		}
		else if (cg_drawUniqueColors.integer == 2)
		{
			Vector4Set(borderSize, -1.0f, -1.0f, -1.0f, -1.0f);
			CG_OSPDrawFrameNew(x, y, w, h, borderSize, uniqueColor);
		}
		else
		{
			Vector4Set(borderSize, -2.0f, -2.0f, -2.0f, -2.0f);
			CG_OSPDrawFrameNew(x, y, w, h, borderSize, uniqueColor);
		}
	}

	if (cg_draw3dIcons.integer)
	{
		if (cg_drawRealHeads.integer)
		{
			qhandle_t origModel, origSkin, origIcon;
			CG_GetOriginalHeadHandles(ci, &origModel, &origSkin, &origIcon);
			cm = origModel;
			headSkin = origSkin;
			// If model not found, use sarge/default as fallback
			if (!cm && cgs.media.unknownHeadModel)
			{
				cm = cgs.media.unknownHeadModel;
				headSkin = cgs.media.unknownHeadSkin;
			}
		}
		else
		{
			cm = ci->headModel;
			headSkin = ci->headSkin;
		}

		if (cm)
		{
			// offset the origin y and z to center the head
			trap_R_ModelBounds(cm, mins, maxs);

			origin[2] = -0.5 * (mins[2] + maxs[2]);
			origin[1] = 0.5 * (mins[1] + maxs[1]);

			// calculate distance so the head nearly fills the box
			// assume heads are taller than wide
			len = 0.7 * (maxs[2] - mins[2]);
			origin[0] = len / 0.268;    // len / tan( fov/2 )

			VectorAdd(origin, ci->headOffset, origin);

			// Determine color based on player relationship
			if (clientNum == cg.clientNum)
			{
				// Local player - use my colors
				VectorCopy(cgs.osp.myColors.head, headColor);
			}
			else if (CG_CACHED_IS_ENEMY_CI(ci))
			{
				// Enemy player - use enemy colors
				VectorCopy(cgs.osp.enemyColors.head, headColor);
			}
			else
			{
				// Teammate - use team colors
				VectorCopy(cgs.osp.teamColors.head, headColor);
			}

			// allow per-model tweaking
			CG_OSPDraw3DModel(x, y, w, h, cm, headSkin, origin, headAngles, headColor);

		}
	}
	else if (cg_drawIcons.integer)
	{
		qhandle_t iconToUse;
		qhandle_t origModel, origSkin, origIcon;

		if (cg_drawRealHeads.integer)
		{
			CG_GetOriginalHeadHandles(ci, &origModel, &origSkin, &origIcon);
			iconToUse = origIcon;
			// If icon not found, use icon_unknown.tga as fallback
			if (!iconToUse)
			{
				iconToUse = cgs.media.unknownSkinIcon;
			}
		}
		else
		{
			iconToUse = ci->modelIcon;
		}

		if (iconToUse)
		{
			CG_DrawPicOld(x, y, w, h, iconToUse);
		}
	}

}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D)
{
	qhandle_t       cm;
	float           len;
	vec3_t          origin, angles;
	vec3_t          mins, maxs;
	qhandle_t       handle;
	vec4_t          tint;

	if (!force2D && cg_draw3dIcons.integer)
	{

		VectorClear(angles);

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(cm, mins, maxs);

		origin[2] = -0.5 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;    // len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);;

		if (team == TEAM_RED)
		{
			handle = cgs.media.redFlagModel;
		}
		else if (team == TEAM_BLUE)
		{
			handle = cgs.media.blueFlagModel;
		}
		else if (team == TEAM_FREE)
		{
			handle = cgs.media.neutralFlagModel;
		}
		else
		{
			return;
		}
		if (CG_GetDyeFlagTintVec4(team, tint) && !CG_OldFlagModels())
		{
			CG_Draw3DModelWithColor(x, y, w, h, handle, 0, origin, angles, tint);
		}
		else
		{
			CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles);
		}
	}
	else if (cg_drawIcons.integer)
	{
		gitem_t* item;
		qhandle_t icon;

		if (team == TEAM_RED)
		{
			item = BG_FindItemForPowerup(PW_REDFLAG);
			icon = cgs.media.redFlagShader[0];
		}
		else if (team == TEAM_BLUE)
		{
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
			icon = cgs.media.blueFlagShader[0];
		}
		else if (team == TEAM_FREE)
		{
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
			icon = item ? cg_items[ ITEM_INDEX(item) ].icon : 0;
		}
		else
		{
			return;
		}
		if (item)
		{
			CG_DrawDyeFlagPicOld(x, y, w, h, team, icon);
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/

static void CG_DrawStatusBarHead(float x)
{
	vec3_t      angles;
	float       size, stretch;
	float       frac;

	VectorClear(angles);

	if (cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME)
	{
		frac = (float)(cg.time - cg.damageTime) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * (1.5 - frac * 0.5);

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
		cg.headEndPitch = 5 * cos(crandom() * M_PI);

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	}
	else
	{
		if (cg.time >= cg.headEndTime)
		{
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
			cg.headEndPitch = 5 * cos(crandom() * M_PI);
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if (cg.headStartTime > cg.time)
	{
		cg.headStartTime = cg.time;
	}

	frac = (cg.time - cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
	frac = frac * frac * (3 - 2 * frac);
	angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
	angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

	CG_DrawHead(x, 480 - size, size, size,
	            cg.snap->ps.clientNum, angles);
}

/*
================
CG_DrawStatusBarFlag

================
*/
static void CG_DrawStatusBarFlag(float x, int team)
{
	CG_DrawFlagModel(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse);
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t      hcolor;

	if (team == TEAM_RED)
	{
		Vector4Copy(cgs.beRedTeamColor, hcolor);
	}
	else if (team == TEAM_BLUE)
	{
		Vector4Copy(cgs.beBlueTeamColor, hcolor);
	}
	else
	{
		return;
	}
	hcolor[3] = alpha;
	trap_R_SetColor(hcolor);
	CG_DrawPicOld(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

#define HBAR_STYLE_FRAME_WHITE     1
#define HBAR_STYLE_FRAME_BLACK     2
#define HBAR_STYLE_FONT_BLACK      4
#define HBAR_STYLE_SHADOW          8
#define HBAR_FILL_ALPHA_SCALE      0.65f

/*
================
CG_DrawHealthBar
================
Draws health+armor bar (total hit points, max 400). If fontIndex >= 0, draws HP/AP values on top.
Frame (1/2) applies everywhere. Text color (4) and shadow (8) only when useCvarTextStyle (standard overlay, teamIndicator).
alpha: 0..1 overall opacity (team overlay color, teamIndicator name alpha, etc.). Bar fill is drawn at alpha * HBAR_FILL_ALPHA_SCALE; frame and values use alpha unchanged.
fontIndex: -1 = bar only, >= 0 = bar and values.
barColorSrc: optional RGB from cg.healthColorByClient (blink/levels); alpha channel is ignored.
*/
void CG_DrawHealthBar(float x, float y, float width, float height, int health, int armor, float alpha, int fontIndex, qboolean useCvarTextStyle, vec4_t* barColorSrc, qboolean teamOverlayHealthBar)
{
	int style;
	float r;
	int hp;
	float bx, by, bw, bh;
	float charW, charH;
	int drawFlags;
	vec4_t color;
	vec4_t borderColor;
	vec4_t borderSize;
	vec4_t textColor;
	vec4_t shadowColor;
	char st[16];

	if (alpha <= 0.0f)
		return;
	if (alpha > 1.0f)
		alpha = 1.0f;

	if (teamOverlayHealthBar)
	{
		y += 1.0f;
		height -= 2.0f;
		if (height < 1.0f)
			height = 1.0f;
	}

	style = cg_healthBarStyle.integer;

	if (style & (HBAR_STYLE_FRAME_WHITE | HBAR_STYLE_FRAME_BLACK))
	{
		Vector4Set(borderSize, 0.5f, 0.5f, 0.5f, 0.5f);
		borderColor[0] = borderColor[1] = borderColor[2] = (style & HBAR_STYLE_FRAME_BLACK) ? 0.0f : 1.0f;
		borderColor[3] = alpha;
		CG_OSPDrawFrameAdjusted(x, y, width, height, borderSize, borderColor, qtrue);
		bx = x + borderSize[0];
		by = y + borderSize[1];
		bw = width - borderSize[0] - borderSize[2];
		bh = height - borderSize[1] - borderSize[3];
	}
	else
	{
		bx = x;
		by = y;
		bw = width;
		bh = height;
	}

	hp = CG_GetTotalHitPoints(health, armor);
	r = (float)hp / 400.0f;
	if (r > 1.0f) r = 1.0f;
	if (barColorSrc)
	{
		Vector4Copy(*barColorSrc, color);
	}
	else
	{
		CG_GetColorForHealth(health, armor, color, NULL);
	}
	color[3] = alpha * HBAR_FILL_ALPHA_SCALE;
	CG_FillRect(bx, by, r * bw, bh, color);

	if (fontIndex < 0)
		return;

	charH = bh * 0.8f;
	if (charH < 4.0f) charH = 4.0f;
	charW = charH * 0.6f;

	/* Bar fill alpha * HBAR_FILL_ALPHA_SCALE; HP/AP text uses alpha unchanged */
	if (useCvarTextStyle)
	{
		if (style & HBAR_STYLE_FONT_BLACK)
		{
			Vector4Set(textColor, 0.0f, 0.0f, 0.0f, alpha);
			Vector4Set(shadowColor, 1.0f, 1.0f, 1.0f, alpha);
		}
		else
		{
			Vector4Set(textColor, 1.0f, 1.0f, 1.0f, alpha);
			Vector4Set(shadowColor, 0.0f, 0.0f, 0.0f, alpha);
		}
		drawFlags = DS_PROPORTIONAL | DS_HCENTER | DS_VCENTER;
		if (style & HBAR_STYLE_SHADOW)
			drawFlags |= DS_SHADOW;
	}
	else
	{
		Vector4Set(textColor, 1.0f, 1.0f, 1.0f, alpha);
		Vector4Set(shadowColor, 0.0f, 0.0f, 0.0f, alpha);
		drawFlags = DS_SHADOW | DS_PROPORTIONAL | DS_HCENTER | DS_VCENTER;
	}

	if (!useCvarTextStyle)
		Com_sprintf(st, sizeof(st), "%3i/%i", health, armor);
	else
		Com_sprintf(st, sizeof(st), "%3i %3i", health, armor);
	CG_OSPDrawStringNew(
	    bx + bw * 0.5f, by + bh * 0.5f,
	    st, textColor, shadowColor,
	    charW, charH, SCREEN_WIDTH,
	    drawFlags,
	    fontIndex, NULL, NULL, NULL);
}

/*
================
CG_DrawStatusBar

================
*/
void CG_DrawStatusBar(void)
{
	int         color;
	centity_t*   cent;
	playerState_t*   ps;
	int         value;
	vec4_t      hcolor;
	vec3_t      angles;
	vec3_t      origin;
	static float colors[4][4] =
	{
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f }
	};   // health > 100

	if (cg_drawStatus.integer == 0)
	{
		return;
	}

	// draw the team background
	CG_DrawTeamBackground(0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM]);

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear(angles);

	// draw any 3D icons first, so the changes back to 2D are minimized
	if (cg_draw3dIcons.integer && cg_drawIcons.integer &&
	        cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel)
	{
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
		CG_Draw3DModel(Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
		               cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles);
	}

	CG_DrawStatusBarHead(185 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE);

	if (cg.predictedPlayerState.powerups[PW_REDFLAG])
	{
		CG_DrawStatusBarFlag(185 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
	}
	else if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
	{
		CG_DrawStatusBarFlag(185 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
	}
	else if (cg.predictedPlayerState.powerups[PW_NEUTRALFLAG])
	{
		CG_DrawStatusBarFlag(185 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);
	}

	if (cg_draw3dIcons.integer && cg_drawIcons.integer &&
	        ps->stats[ STAT_ARMOR ])
	{
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
		CG_Draw3DModel(370 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432.0, ICON_SIZE, ICON_SIZE,
		               cgs.media.armorModel[ps->stats[STAT_ARMOR_TYPE]], 0, origin, angles);
	}
	//
	// ammo
	//
	if (cent->currentState.weapon)
	{
		value = ps->ammo[cent->currentState.weapon];
		if (value > -1)
		{
			if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
			        && cg.predictedPlayerState.weaponTime > 100)
			{
				// draw as dark grey when reloading
				color = 2;  // dark grey
			}
			else
			{
				if (value > 0)
				{
					color = 0;  // green
				}
				else
				{
					color = 1;  // red
				}
			}
			trap_R_SetColor(colors[color]);

			CG_DrawField(0, 432, 3, value, 32, 48);
			trap_R_SetColor(NULL);

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if (!cg_draw3dIcons.integer && cg_drawIcons.integer)
			{
				qhandle_t   icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if (icon)
				{
					CG_DrawPicOld(Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon);
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if (value > 100)
	{
		trap_R_SetColor(colors[3]);          // white
	}
	else if (value > 25)
	{
		trap_R_SetColor(colors[0]);      // green
	}
	else if (value > 0)
	{
		color = (cg.time >> 8) & 1; // flash
		trap_R_SetColor(colors[color]);
	}
	else
	{
		trap_R_SetColor(colors[1]);      // red
	}

	// stretch the health up when taking damage
	CG_DrawField(185, 432, 3, value, 32, 48);
	Vector4Copy(cg.healthColorLocal, hcolor);
	trap_R_SetColor(hcolor);


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0)
	{
		trap_R_SetColor(colors[0]);
		CG_DrawField(370, 432, 3, value, 32, 48);
		trap_R_SetColor(NULL);
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if (!cg_draw3dIcons.integer && cg_drawIcons.integer)
		{
			CG_DrawPicOld(370 + Q3_CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon[ps->stats[STAT_ARMOR_TYPE]]);
		}

	}
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
float CG_DrawAttacker(float y)
{
	int         t;
	float       size;
	vec3_t      angles;
	const char*  info;
	const char*  name;
	int         clientNum;

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	if (!cg.attackerTime)
	{
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum)
	{
		return y;
	}

	t = cg.time - cg.attackerTime;
	if (t > ATTACKER_HEAD_TIME)
	{
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead(640 - size, y, size, size, clientNum, angles);

	if (clientNum >= 0 && clientNum < MAX_CLIENTS && cgs.clientinfo[clientNum].infoValid)
	{
		name = cgs.clientinfo[clientNum].name;
	}
	else
	{
		info = CG_ConfigString(CS_PLAYERS + clientNum);
		name = Info_ValueForKey(info, "n");
	}
	y += size;
	CG_DrawBigString(SCREEN_WIDTH - 1, y, name, 0.5f, DS_HRIGHT | DS_SHADOW, 0);

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
float CG_DrawSnapshot(float y)
{
	char* s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence);

	CG_DrawBigString(SCREEN_WIDTH - 1, y + 2, s, 1.0f, DS_HRIGHT | DS_SHADOW, 0);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES  4
float CG_DrawFPS(float y)
{
	char*        s;
	static int  previousTimes[FPS_FRAMES];
	static int  index;
	int     i, total;
	int     fps;
	static  int previous;
	int     t;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if (index > FPS_FRAMES)
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0 ; i < FPS_FRAMES ; i++)
		{
			total += previousTimes[i];
		}
		if (!total)
		{
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%ifps", fps);

		CG_DrawBigString(SCREEN_WIDTH - 5, y + 2, s, 1.0f, DS_HRIGHT | DS_SHADOW, 0);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
float CG_DrawTimer(float y)
{
	char*        s;
	int         mins, seconds, tens;
	int         msec;
	int         w, h;
	const float   color[4] = {1.0, 1.0, 1.0, 1.0};
	if (cg_countdownTimer.integer)
	{
		msec = CG_GetRemainingGameTimeMsec(qfalse);
	}
	else
	{
		msec = cg.time - cgs.levelStartTime;
		if (msec < 0) msec *= -1;
	}


	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);

	CG_OSPGetClientFontSize(&cf_timer, &w, &h);

	CG_FontSelect(0);
	CG_OSPDrawString(SCREEN_WIDTH - 5, y + 2, s, color, w, h, SCREEN_WIDTH, DS_HRIGHT | DS_SHADOW, NULL);

	return y + h + 4;
}

void CG_DrawTimer2(void)
{
	char*        s;
	int         mins, seconds, tens;
	int         msec;
	int         w, h;
	const float   color[4] = {1.0, 1.0, 1.0, 1.0};

	if (cg_countdownTimer.integer)
	{
		msec = CG_GetRemainingGameTimeMsec(qfalse);
	}
	else
	{
		msec = cg.time - cgs.levelStartTime;
		if (msec < 0) msec *= -1;
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;


	s = va("%i:%i%i", mins, tens, seconds);

	CG_OSPGetClientFontSize(&cf_timer, &w, &h);
	CG_FontSelect(0);
	CG_OSPDrawString(SCREEN_WIDTH / 2.0f, 2, s, color, w, h, SCREEN_WIDTH, DS_HCENTER | DS_SHADOW, NULL);
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay(float y, qboolean right, qboolean upper)
{
	int x, w, h, xx;
	int i, j, len;
	const char* p;
	vec4_t      hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t* ci;
	gitem_t* item;
	int ret_y, count;
	float* tc;

	if (!cg_drawTeamOverlay.integer)
	{
		return y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
	{
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++)
	{
		p = CG_LocationName(i);
		if (p && *p)
		{
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if (right)
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if (upper)
	{
		ret_y = y + h;
	}
	else
	{
		y -= h;
		ret_y = y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
	{
		tc = cgs.beRedTeamColor;
		hcolor[0] = tc[0];
		hcolor[1] = tc[1];
		hcolor[2] = tc[2];
		hcolor[3] = 0.33f;
	}
	else     // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
	{
		tc = cgs.beBlueTeamColor;
		hcolor[0] = tc[0];
		hcolor[1] = tc[1];
		hcolor[2] = tc[2];
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPicOld(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);

	for (i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(xx, y,
			                 ci->name, hcolor, qfalse, qfalse,
			                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth)
			{
				p = CG_LocationName(ci->location);
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt(xx, y,
				                 p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
				                 TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			xx = x + TINYCHAR_WIDTH * 3 +
			     TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			if (cg_teamHealthBar.integer >= 1)
			{
				CG_DrawHealthBar((float)xx, (float)y, 7.0f * TINYCHAR_WIDTH, (float)TINYCHAR_HEIGHT, ci->health, ci->armor, 1.0f,
				                 (cg_teamHealthBar.integer == 2) ? 0 : -1, qtrue,
				                 &cg.healthColorByClient[sortedTeamPlayers[i]], qtrue);
			}
			else
			{
				Vector4Copy(cg.healthColorByClient[sortedTeamPlayers[i]], hcolor);
				Com_sprintf(st, sizeof(st), "%3i %3i", ci->health, ci->armor);
				CG_DrawStringExt(xx, y,
				                 st, hcolor, qfalse, qfalse,
				                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
			}

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if (cg_weapons[ci->curWeapon].weaponIcon)
			{
				CG_DrawPicOld(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
				              cg_weapons[ci->curWeapon].weaponIcon);
			}
			else
			{
				CG_DrawPicOld(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
				              cgs.media.deferShader);
			}

			// Draw powerup icons
			if (right)
			{
				xx = x;
			}
			else
			{
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++)
			{
				if (ci->teamPowerups & (1 << j))
				{
					if (j == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
					{
						continue;
					}

					item = BG_FindItemForPowerup(j);

					if (item)
					{
						CG_DrawClientPowerupIconOld(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						                            j, sortedTeamPlayers[i]);
						if (right)
						{
							xx -= TINYCHAR_WIDTH;
						}
						else
						{
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(void)
{
	float   y;

	y = 0;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1)
	{
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	}
	if (cg_drawSnapshot.integer)
	{
		y = CG_DrawSnapshot(y);
	}
	if (cg_drawFPS.integer)
	{
		y = CG_DrawFPS(y);
	}
	if (cg_drawTimer.integer)
	{
		y = CG_DrawTimer(y);
	}
	if (cg_drawAttacker.integer)
	{
		y = CG_DrawAttacker(y);
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores(float y)
{
	const char*  s;
	int         s1, s2, score;
	int         x, w;
	int         v;
	vec4_t      color;
	vec4_t      redColor;
	vec4_t      blueColor;
	float       y1;
	gitem_t*     item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if (cgs.gametype >= GT_TEAM)
	{
		Vector4Copy(cgs.beRedTeamColor, redColor);
		Vector4Copy(cgs.beBlueTeamColor, blueColor);

		x = 640;
		color[0] = redColor[0];
		color[1] = redColor[1];
		color[2] = redColor[2];
		color[3] = 0.33f;
		s = va("%2i", s2);
		w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
		x -= w;
		CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		{
			CG_DrawPicOld(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);

		if (cgs.gametype == GT_CTF)
		{
			// Display flag status
			item = BG_FindItemForPowerup(PW_BLUEFLAG);

			if (item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if (cgs.blueflag >= 0 && cgs.blueflag <= 2)
				{
					CG_DrawDyeFlagPicOld(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, TEAM_BLUE, cgs.media.blueFlagShader[cgs.blueflag]);
				}
			}
		}
		color[0] = blueColor[0];
		color[1] = blueColor[1];
		color[2] = blueColor[2];
		color[3] = 0.33f;
		s = va("%2i", s1);
		w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
		x -= w;
		CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		{
			CG_DrawPicOld(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);

		if (cgs.gametype == GT_CTF)
		{
			// Display flag status
			item = BG_FindItemForPowerup(PW_REDFLAG);

			if (item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if (cgs.redflag >= 0 && cgs.redflag <= 2)
				{
					CG_DrawDyeFlagPicOld(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, TEAM_RED, cgs.media.redFlagShader[cgs.redflag]);
				}
			}
		}

		if (cgs.gametype >= GT_CTF)
		{
			v = cgs.capturelimit;
		}
		else
		{
			v = cgs.fraglimit;
		}
		if (v)
		{
			s = va("%2i", v);
			w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);
		}

	}
	else
	{
		qboolean    spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

		// always show your score in the second box if not in first place
		if (s1 != score)
		{
			s2 = score;
		}
		if (s2 != SCORE_NOT_PRESENT)
		{
			s = va("%2i", s2);
			w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
			x -= w;
			if (!spectator && score == s2 && score != s1)
			{
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPicOld(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			}
			else
			{
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);
		}

		// first place
		if (s1 != SCORE_NOT_PRESENT)
		{
			s = va("%2i", s1);
			w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
			x -= w;
			if (!spectator && score == s1)
			{
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPicOld(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			}
			else
			{
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4,  w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);
		}

		if (cgs.fraglimit)
		{
			s = va("%2i", cgs.fraglimit);
			w = 2 * BIGCHAR_WIDTH + 8; /* "%2i" always 2 chars */
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0f, DS_HLEFT | DS_SHADOW, 0);
		}

	}

	return y1 - 8;
}

/*
================
CG_DrawPowerups
================
*/
static float CG_DrawPowerups(float y)
{
	int     sorted[MAX_POWERUPS];
	int     sortedTime[MAX_POWERUPS];
	int     i, j, k;
	int     active;
	playerState_t*   ps;
	int     t;
	gitem_t* item;
	int     x;
	int     color;
	float   size;
	float   f;
	static float colors[2][4] =
	{
		{ 0.2f, 1.0f, 0.2f, 1.0f },
		{ 1.0f, 0.2f, 0.2f, 1.0f }
	};

	ps = &cg.snap->ps;

	if (ps->stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for (i = 0 ; i < MAX_POWERUPS ; i++)
	{
		if (!ps->powerups[ i ])
		{
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if (t < 0 || t > 999000)
		{
			continue;
		}

		// insert into the list
		for (j = 0 ; j < active ; j++)
		{
			if (sortedTime[j] >= t)
			{
				for (k = active - 1 ; k >= j ; k--)
				{
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - Q3_CHAR_WIDTH * 2;
	for (i = 0 ; i < active ; i++)
	{
		item = BG_FindItemForPowerup(sorted[i]);

		if (item)
		{

			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor(colors[color]);
			CG_DrawField(x, y, 2, sortedTime[ i ] / 1000, 32, 48);

			t = ps->powerups[ sorted[i] ];
			if (t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
			{
				trap_R_SetColor(NULL);
			}
			else
			{
				vec4_t    modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if (cg.powerupActive == sorted[i] &&
			        cg.time - cg.powerupTime < PULSE_TIME)
			{
				f = 1.0 - (((float)cg.time - cg.powerupTime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			}
			else
			{
				size = ICON_SIZE;
			}

			CG_DrawClientPowerupIconOld(640 - size, y + ICON_SIZE / 2 - size / 2,
			                            size, size, sorted[i], ps->clientNum);
		}
	}
	trap_R_SetColor(NULL);

	return y;
}

/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight(void)
{
	float   y;

	y = 480 - ICON_SIZE;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2)
	{
		y = CG_DrawTeamOverlay(y, qtrue, qfalse);
	}

	y = CG_DrawScores(y);
	(void)CG_DrawPowerups(y);
}

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem(int y)
{
	int     value;
	float*   fadeColor;

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if (value)
	{
		fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if (fadeColor)
		{
			CG_RegisterItemVisuals(value);
			if (cg_items[value].customVisualMask & ITEMCV_ICON)
			{
				vec4_t picCol;

				Vector4Copy(fadeColor, picCol);
				CG_ModulateVec4ForCustomItemDyeIcon(value, picCol);
				trap_R_SetColor(picCol);
			}
			else
			{
				trap_R_SetColor(fadeColor);
			}
			CG_DrawPicOld(8, y, ICON_SIZE, ICON_SIZE, CG_DyeFlagResolveInverseIcon(value, cg_items[ value ].icon));
			trap_R_SetColor(NULL);
			CG_DrawBigString(ICON_SIZE + 16, y + (ICON_SIZE / 2 - BIGCHAR_HEIGHT / 2), bg_itemlist[ value ].pickup_name, fadeColor[0], DS_HCENTER | DS_SHADOW, 0);
		}
	}

	return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft(void)
{
	float   y = 480 - ICON_SIZE;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3)
	{
		(void)CG_DrawTeamOverlay(y, qfalse, qfalse);
	}

	(void)CG_DrawPickupItem(y);
}


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo(void)
{
	int w, h;
	int i, len;
	vec4_t      hcolor;
	int     chatHeight;
	float* tc;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos)
	{
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer)
		{
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++)
		{
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		{
			tc = cgs.beRedTeamColor;
			hcolor[0] = tc[0];
			hcolor[1] = tc[1];
			hcolor[2] = tc[2];
			hcolor[3] = 0.33f;
		}
		else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		{
			tc = cgs.beBlueTeamColor;
			hcolor[0] = tc[0];
			hcolor[1] = tc[1];
			hcolor[2] = tc[2];
			hcolor[3] = 0.33f;
		}
		else
		{
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor(hcolor);
		CG_DrawPicOld(CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar);
		trap_R_SetColor(NULL);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
		{
			CG_DrawStringExt(CHATLOC_X + TINYCHAR_WIDTH,
			                 CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT,
			                 cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
			                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
}

/*
===================
CG_DrawHoldableItem
===================
*/
static void CG_DrawHoldableItem(void)
{
	int     value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if (value)
	{
		CG_RegisterItemVisuals(value);
		CG_DrawPicOld(640 - ICON_SIZE, (SCREEN_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon);
	}

}


/*
===================
CG_DrawReward
===================
*/
void CG_DrawReward(void)
{
	float* color;
	int i;
	int drawRewards;
	float x, y, w, h;
	float picX, picY, picW, picH;
	char buf[32];
	qhandle_t shader;
	vec4_t tc;

	drawRewards = cg_drawRewards.integer;
	if (!drawRewards)
	{
		return;
	}

	color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
	if (!color)
	{
		if (cg.rewardStack > 0)
		{
			for (i = 0; i < cg.rewardStack; i++)
			{
				cg.rewardSound[i] = cg.rewardSound[i + 1];
				cg.rewardShader[i] = cg.rewardShader[i + 1];
				cg.rewardCount[i] = cg.rewardCount[i + 1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;

			color = CG_FadeColor(cg.rewardTime, REWARD_TIME);

			if (!(drawRewards & DRAW_REWARDS_NOSOUND))
			{
				trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
			}
		}
		else
		{
			return;
		}
	}

	trap_R_SetColor(color);

	w = ICON_SIZE - 4;
	h = ICON_SIZE - 4;
	y = 46;
	x = 320 - w / 2.0;

	if (!(drawRewards & DRAW_REWARDS_NOICON))
	{
		if (cg_oldMedals.integer)
		{
			CG_DrawPic(x, y, w, h, cg.rewardShader[0]);
		}
		else
		{
			CG_MedalResolveFrame(cg.rewardShader[0], cg.rewardTime, REWARD_TIME, &shader, tc);
			picX = x;
			picY = y;
			picW = w;
			picH = h;
			CG_AdjustFrom640(&picX, &picY, &picW, &picH);
			trap_R_DrawStretchPic(picX, picY, picW, picH, tc[0], tc[1], tc[2], tc[3], shader);
		}

		if (cg.rewardCount[0] > 1)
		{
			Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
			CG_DrawString(x + w / 2.0f, y + h, buf, color, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 16, DS_HCENTER | DS_PROPORTIONAL);
		}
	}

	trap_R_SetColor(NULL);
}




/*
===============================================================================

LAGOMETER

===============================================================================
*/

lagometer_t     lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void)
{
	int         offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & (LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static float CG_DrawDisconnect(float pos)
{
	int         cmdNum;
	usercmd_t   cmd;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time)    // special check for map_restart // bk 0102165 - FIXME
	{
		return pos;
	}

	// also add text in center of screen
	CG_DrawBigString(SCREEN_WIDTH / 2.0f, 100, "Connection Interrupted", 1.0f, DS_HCENTER | DS_SHADOW, 0);

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return pos;
	}

	CG_DrawPicOld(640 - 48, pos, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga"));
	return pos;
}

#define LAGOMETER_GRAPH_MAX_COLUMNS 64

/*
==============
CG_DrawLagometerGraphsInRect

Frame + snapshot graphs inside an already-adjusted rect (ax, ay, aw, ah).
Caps DrawStretchPic count at LAGOMETER_GRAPH_MAX_COLUMNS by merging pixels (step-wide bars).
==============
*/
void CG_DrawLagometerGraphsInRect(float ax, float ay, float aw, float ah)
{
	int     a;
	int     i;
	int     color;
	int     pixelSlots;
	int     graphW;
	int     step;
	int     sampleOff;
	int     frameCount;
	int     snapshotCount;
	float   v;
	float   mid;
	float   range;
	float   vscale;
	float   barX;
	float   barW;
	float   colYellow[4];
	float   colBlue[4];
	float   colGreen[4];
	float   colRed[4];

	if (aw <= 0.0f || ah <= 0.0f)
	{
		return;
	}

	pixelSlots = (int)aw;
	if ((float)pixelSlots < aw)
	{
		pixelSlots++;
	}

	graphW = pixelSlots;
	step = 1;
	if (graphW > LAGOMETER_GRAPH_MAX_COLUMNS)
	{
		step = (graphW + LAGOMETER_GRAPH_MAX_COLUMNS - 1) / LAGOMETER_GRAPH_MAX_COLUMNS;
		graphW = (graphW + step - 1) / step;
	}

	Vector4Copy(g_color_table[ColorIndex(COLOR_YELLOW)], colYellow);
	Vector4Copy(g_color_table[ColorIndex(COLOR_BLUE)], colBlue);
	Vector4Copy(g_color_table[ColorIndex(COLOR_GREEN)], colGreen);
	Vector4Copy(g_color_table[ColorIndex(COLOR_RED)], colRed);

	color = -1;
	frameCount = lagometer.frameCount;
	range = ah / 3.0f;
	mid = ay + range;
	vscale = range / MAX_LAGOMETER_RANGE;

	for (a = 0; a < graphW; a++)
	{
		sampleOff = a * step;
		if (sampleOff >= pixelSlots)
		{
			continue;
		}
		barW = (float)step;
		barX = ax + aw - (float)sampleOff - barW;
		if (barX < ax)
		{
			barW -= ax - barX;
			barX = ax;
		}
		if (barW <= 0.0f)
		{
			continue;
		}

		i = (frameCount - 1 - sampleOff) & (LAG_SAMPLES - 1);
		v = (float)lagometer.frameSamples[i] * vscale;
		if (v > 0.0f)
		{
			if (color != 1)
			{
				color = 1;
				trap_R_SetColor(colYellow);
			}
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(barX, mid - v, barW, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0.0f)
		{
			if (color != 2)
			{
				color = 2;
				trap_R_SetColor(colBlue);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(barX, mid, barW, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	snapshotCount = lagometer.snapshotCount;
	range = ah * 0.5f;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0; a < graphW; a++)
	{
		sampleOff = a * step;
		if (sampleOff >= pixelSlots)
		{
			continue;
		}
		barW = (float)step;
		barX = ax + aw - (float)sampleOff - barW;
		if (barX < ax)
		{
			barW -= ax - barX;
			barX = ax;
		}
		if (barW <= 0.0f)
		{
			continue;
		}

		i = (snapshotCount - 1 - sampleOff) & (LAG_SAMPLES - 1);
		v = (float)lagometer.snapshotSamples[i];
		if (v > 0.0f)
		{
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5;
					trap_R_SetColor(colYellow);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap_R_SetColor(colGreen);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(barX, ay + ah - v, barW, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0.0f)
		{
			if (color != 4)
			{
				color = 4;
				trap_R_SetColor(colRed);
			}
			trap_R_DrawStretchPic(barX, ay + ah - range, barW, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);
}

/*
==============
CG_DrawLagometer
==============
*/
float CG_DrawLagometer(float pos)
{
	int     x;
	float   ax, ay, aw, ah;

	if (cg_drawPing.integer > 0)
	{
		pos = CG_OSPDrawPing(pos) - 2.0f;
	}

	x = 640 - 48;
	pos += 4.0f;

	trap_R_SetColor(NULL);
	CG_DrawPicOld(x, pos, 48, 48, cgs.media.lagometerShader);

	ax = x;
	ay = pos;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640_Old(&ax, &ay, &aw, &ah, qfalse);

	CG_DrawLagometerGraphsInRect(ax, ay, aw, ah);

	if (cg_nopredict.integer || cg_synchronousClients.integer)
	{
		CG_DrawBigString(ax, ay, "snc", 1.0f, DS_HLEFT | DS_SHADOW, 0);
	}

	pos += CG_DrawDisconnect(pos);

	return pos + 44.0f;
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char* str, int y, int charWidth)
{
	char*    s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while (*s)
	{
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
CG_ScanForCrosshairEntity
=================
*/
void CG_ScanForCrosshairEntity(void)
{
	trace_t trace;
	vec3_t start, end;
	int content;
	vec3_t l;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.snap->ps.clientNum, CONTENTS_SOLID);

	VectorSubtract(cg.snap->ps.origin, trace.endpos, l);
	crosshairData.distance = VectorLength(l);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
	if (trace.entityNum >= MAX_CLIENTS)
	{
		if (cgs.osp.gameTypeFreeze && trace.entityNum < MAX_GENTITIES)
		{
			centity_t* centp = &cg_entities[trace.entityNum];
			if (centp->currentState.powerups & (1 << PW_BATTLESUIT))
			{
				cg.crosshairClientNum = centp->currentState.otherEntityNum;
				cg.crosshairClientTime = cg.time;
			}
		}
		return;
	}

// draw only team names
	if (cg_drawCrosshairNames.integer == 2)
	{
		if (cgs.clientinfo[trace.entityNum].team != cg.snap->ps.persistant[PERS_TEAM] ||
		        (cgs.gametype <= GT_SINGLE_PLAYER))
		{
			return;
		}
	}
// draw only enemy names
	else if (cg_drawCrosshairNames.integer == 3)
	{
		if (cgs.gametype <= GT_SINGLE_PLAYER)
		{

		}
		else
		{
			if (cgs.clientinfo[trace.entityNum].team == cg.snap->ps.persistant[PERS_TEAM])
			{
				return;
			}
		}
	}
	// if the player is in fog, don't show it — unless it's a teammate in team game
	content = trap_CM_PointContents(trace.endpos, 0);
	if (content & CONTENTS_FOG && !cg.crosshairIgnoreFog)
	{
		if (cgs.gametype >= GT_TEAM)
		{
			if (cgs.clientinfo[trace.entityNum].team == cg.snap->ps.persistant[PERS_TEAM])
			{
				// ally — show in team games
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// if the player is invisible, don't show it
	if (cg_entities[trace.entityNum].currentState.powerups & (1 << PW_INVIS))
	{
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

/*
=================
CG_CrosshairTarget
=================
Returns: 1 = nothing, 2 = enemy, 3 = red armor, 4 = megahealth, 5 = frozen ally, 6 = frozen enemy,
         7 = quaddamage, 8 = regeneration, 9 = battlesuit, 10 = haste, 11 = yellow armor, 12 = health,
         13 = red flag carrier, 14 = blue flag carrier, 15 = red flag, 16 = blue flag (CTF only)
If item found, itemPos is filled with item center position
*/
qboolean CG_CrosshairSolidClearToPoint(const vec3_t start, const vec3_t endPoint)
{
	trace_t trace;

	if (!start || !endPoint)
	{
		return qfalse;
	}

	CG_Trace(&trace, start, vec3_origin, vec3_origin, endPoint,
	         cg.snap ? cg.snap->ps.clientNum : -1, CONTENTS_SOLID);
	return trace.fraction >= 0.99f;
}

qboolean CG_CrosshairInspectingWorldPoint(const vec3_t worldPoint, float radius)
{
	vec3_t  start;
	vec3_t  delta;
	vec3_t  closest;
	float   along;
	float   perpSq;
	float   radiusSq;

	if (!cg.snap || !worldPoint || radius <= 0.0f)
	{
		return qfalse;
	}

	VectorCopy(cg.refdef.vieworg, start);
	if (!CG_CrosshairSolidClearToPoint(start, worldPoint))
	{
		return qfalse;
	}

	VectorSubtract(worldPoint, start, delta);
	along = DotProduct(delta, cg.refdef.viewaxis[0]);
	if (along <= 0.0f)
	{
		return qfalse;
	}

	VectorMA(start, along, cg.refdef.viewaxis[0], closest);
	perpSq = DistanceSquared(worldPoint, closest);
	radiusSq = radius * radius;
	return perpSq <= radiusSq;
}

int CG_CrosshairTarget(vec3_t itemPos)
{
	trace_t trace;
	vec3_t start, end;
	centity_t* cent;
	clientInfo_t* ci;
	int frozenClientNum;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	/* Use MASK_SHOT to detect both players and items */
	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.snap->ps.clientNum, MASK_SHOT);

	// Check if looking at a frozen entity
	if (trace.entityNum >= MAX_CLIENTS && trace.entityNum < MAX_GENTITIES)
	{
		cent = &cg_entities[trace.entityNum];
		if (CG_IS_FROZEN_ENTITY(cent))
		{
			frozenClientNum = cent->currentState.otherEntityNum;
			if (frozenClientNum >= 0 && frozenClientNum < MAX_CLIENTS)
			{
				ci = &cgs.clientinfo[frozenClientNum];
				if (ci->infoValid)
				{
					VectorCopy(cent->lerpOrigin, itemPos);
					itemPos[2] += 24.0f; /* Offset above frozen body */
					if (CG_CACHED_IS_ENEMY_CI(ci))
					{
						return 6;  // frozen enemy
					}
					else
					{
						return 5;  // frozen ally
					}
				}
			}
		}
	}

	// Check if looking at a player
	if (trace.entityNum < MAX_CLIENTS)
	{
		centity_t* playerCent;

		ci = &cgs.clientinfo[trace.entityNum];
		if (!ci->infoValid)
		{
			return 1;  // nothing (invalid)
		}

		// Check for flag carriers in CTF mode
		if (cgs.gametype == GT_CTF)
		{
			// Get player entity to check powerups
			playerCent = &cg_entities[trace.entityNum];
			if (playerCent && playerCent->currentValid)
			{
				// Check if player has red flag
				if (playerCent->currentState.powerups & (1 << PW_REDFLAG))
				{
					VectorCopy(playerCent->lerpOrigin, itemPos);
					itemPos[2] += 24.0f;
					return 13;  // red flag carrier
				}
				// Check if player has blue flag
				if (playerCent->currentState.powerups & (1 << PW_BLUEFLAG))
				{
					VectorCopy(playerCent->lerpOrigin, itemPos);
					itemPos[2] += 24.0f;
					return 14;  // blue flag carrier
				}
			}
		}

		if (CG_CACHED_IS_ENEMY_CI(ci))
		{
			return 2;  // enemy
		}
		return 1;  // nothing (ally)
	}

	/* Check item entities from list (cg.itemEntityIndices filled in CG_Item) */
	{
		int i;
		centity_t* checkCent;
		gitem_t* checkItem;
		int checkItemIndex;
		vec3_t itemOrigin;
		vec3_t mins, maxs;
		trace_t hboxTrace;
		float hbox_height, hbox_size_xy, lift, half_h;
		int r;
		clipHandle_t cmodel;

		for (i = 0; i < cg.numItemEntityIndices; i++)
		{
			checkCent = &cg_entities[cg.itemEntityIndices[i]];
			/* Skip items that are picked up or removed (same checks as in CG_Item) */
			if (!checkCent->currentState.modelindex || (checkCent->currentState.eFlags & EF_NODRAW))
				continue;
			if (!checkCent->currentValid)
				continue;

			checkItemIndex = checkCent->currentState.modelindex;
			if (checkItemIndex >= 1 && checkItemIndex < bg_numItems)
			{
				checkItem = &bg_itemlist[checkItemIndex];
				if (checkItem->classname && (
				            !Q_stricmp(checkItem->classname, "item_armor_body") ||
				            !Q_stricmp(checkItem->classname, "item_health_mega") ||
				            !Q_stricmp(checkItem->classname, "item_quad") ||
				            !Q_stricmp(checkItem->classname, "item_regen") ||
				            !Q_stricmp(checkItem->classname, "item_enviro") ||
				            !Q_stricmp(checkItem->classname, "item_haste") ||
				            !Q_stricmp(checkItem->classname, "item_armor_combat") ||
				            !Q_stricmp(checkItem->classname, "item_health") ||
				            !Q_stricmp(checkItem->classname, "item_health_large") ||
				            (cgs.gametype == GT_CTF && (
				                 !Q_stricmp(checkItem->classname, "team_CTF_redflag") ||
				                 !Q_stricmp(checkItem->classname, "team_CTF_blueflag")))))
				{
					/* Get item position (before bob), used for hitbox trace and visibility */
					VectorCopy(checkCent->lerpOrigin, itemOrigin);
					if (cg_itemFx.integer & 1)
					{
						float scale = 0.005f + checkCent->currentState.number * 0.00001f;
						float bobOffset = 4.0f + cos((cg.time + 1000) * scale) * 4.0f;
						itemOrigin[2] -= bobOffset;
					}

					/* Hitbox parameters (same as CG_AddItemHitBox); flags use giTag (no Q_stricmp) */
					if (cg_simpleItems.integer == 1)
					{
						r = cg_simpleItemsRadius.integer;
						if (r > 24) r = 24;
						if (r < 8) r = 8;
						hbox_size_xy = (float)r;
						lift = (r > 14) ? (float)(r - 14) * 1.5f : 0.0f;
						half_h = (float)r * 0.5f + 2.0f;
						mins[0] = -hbox_size_xy;
						mins[1] = -hbox_size_xy;
						mins[2] = -half_h;
						maxs[0] = hbox_size_xy;
						maxs[1] = hbox_size_xy;
						maxs[2] = lift + half_h + 4.0f;
					}
					else
					{
						hbox_size_xy = 16.0f;
						mins[0] = mins[1] = -hbox_size_xy;
						mins[2] = -8.0f;
						maxs[0] = maxs[1] = hbox_size_xy;
						/* Flags (giTag) have higher hitbox than other items */
						if (cgs.gametype == GT_CTF && checkItem->giType == IT_TEAM &&
						        (checkItem->giTag == PW_REDFLAG || checkItem->giTag == PW_BLUEFLAG))
							hbox_height = 60.0f;
						else
							hbox_height = 40.0f;
						maxs[2] = hbox_height;
					}

					cmodel = trap_CM_TempBoxModel(mins, maxs);
					trap_CM_TransformedBoxTrace(&hboxTrace, start, end,
					                            vec3_origin, vec3_origin, cmodel,
					                            MASK_SHOT, itemOrigin, vec3_origin);

					if (hboxTrace.fraction < 1.0f)
					{
						if (CG_CrosshairSolidClearToPoint(start, itemOrigin))
						{
							VectorCopy(itemOrigin, itemPos);

							/* Use giType/giTag/quantity instead of Q_stricmp */
							if (checkItem->giType == IT_POWERUP)
							{
								switch (checkItem->giTag)
								{
									case PW_QUAD:
										return 7;   /* quaddamage */
									case PW_REGEN:
										return 8;   /* regeneration */
									case PW_BATTLESUIT:
										return 9;   /* battlesuit */
									case PW_HASTE:
										return 10;  /* haste */
									default:
										break;
								}
							}
							else if (checkItem->giType == IT_TEAM && cgs.gametype == GT_CTF)
							{
								if (checkItem->giTag == PW_REDFLAG)  return 15;  /* red flag */
								if (checkItem->giTag == PW_BLUEFLAG) return 16;  /* blue flag */
							}
							else if (checkItem->giType == IT_ARMOR)
							{
								if (checkItem->quantity >= 100) return 3;   /* red armor */
								if (checkItem->quantity >= 50)  return 11;  /* yellow armor */
							}
							else if (checkItem->giType == IT_HEALTH)
							{
								if (checkItem->quantity >= 100) return 4;  /* megahealth */
								return 12;  /* health 25 and 50 */
							}
						}
					}
				}
			}
		}
	}
	return 1;  // nothing
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
void CG_DrawCrosshairNames(void)
{
	float*       color;
	char*        name;

	if (cg_drawCrosshair.integer == 0) return;
	if (cg_drawCrosshairNames.integer == 0) return;
	if (cg.renderingThirdPerson != 0) return;
	if (global_viewlistFirstOption > 1) return;

	if ((cg_drawCrosshairNames.integer == 2) &&
	        ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_FREE) ||
	         (cg.snap->ps.persistant[PERS_TEAM] == cgs.clientinfo[cg.crosshairClientNum].team) ||
	         cgs.osp.gameTypeFreeze ||
	         (cgs.clientinfo[cg.snap->ps.clientNum].team != cgs.clientinfo[cg.crosshairClientNum].team)))
	{
		return;
	}

	color = CG_FadeColor(cg.crosshairClientTime, 1000);
	if (!color)
	{
		return;
	}
	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	CG_DrawBigString(SCREEN_WIDTH / 2.0f, 170, name, 0.5f * color[3], DS_HCENTER | DS_SHADOW, 0);
}


//==============================================================================

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void)
{
	char*    s;
	int     sec;

	if (!cgs.voteTime)
	{
		return;
	}

	// play a talk beep whenever it is modified or no
	if (cgs.voteModified && !cg_noVoteBeep.integer)
	{
		cgs.voteModified = qfalse;
		if (!cg_noVoteBeep.integer)
		{
			trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		}
	}

	sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if (sec < 0)
	{
		sec = 0;
	}
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString(0, 58, s, 1.0F, DS_HLEFT | DS_SHADOW, 0);
}

/*
=================
CG_DrawIntermission
=================
*/
qboolean CG_DrawIntermission(void)
{
	qboolean result;

	cg.scoreBoardShowing = qfalse;

	if (cg_q3compScoreboard.integer)
	{
		if (cgs.gametype >= GT_TEAM)
		{
			if (!cg_scoreboardBE.integer)
				result = CG_OSPDrawScoretable();
			else
				result = CG_BEDrawTeamScoretable();
		}
		else
		{
			result = CG_DrawOldScoreboard();
		}
	}
	else
	{
		result = CG_DrawOldScoreboard();
	}

	if (result)
	{
		cg.scoreBoardShowing = qtrue;
		if (cg_drawAccuracy.integer)
		{
			CG_DrawWeaponStatsWrapper();
		}
		CG_ScoreboardAccuracyStatsRefresh();
	}

	// Request scores every 2 seconds only when table is actually being drawn
	if (result && cg.scoresRequestTime + 2000 < cg.time)
	{
		cg.scoresRequestTime = cg.time;
		if (!cg.demoPlayback)
			trap_SendClientCommand("score");
	}
	// getdstatus (dead in freeze, q3msk only) when we did not send score this frame — same 2s cycle; 600 ms min; not during intermission
	else if (result && !cg.demoPlayback && cg.snap->ps.pm_type != PM_INTERMISSION)
		CG_TrySendDstatusRequest();

	return result;
}

void CG_OSPDrawIntermission()
{
	int i;

	if (cgs.gametype == GT_SINGLE_PLAYER)
	{
		CG_OSPDrawCenterString();
		return;
	}

	for (i = 0; i < cgs.osp.custom_gfx_number; ++i)
	{
		if (cgs.osp.custom_gfx[i][4] >= 0 &&
		        cgs.osp.custom_gfx[i][5] >= 0 &&
		        cgs.osp.custom_gfx[i][6] > 0 &&
		        cgs.osp.custom_gfx[i][7] > 0 &&
		        cgs.media.customShader[i])
		{
			CG_DrawPicOld(
			    (float)cgs.osp.custom_gfx[i][4],
			    (float)cgs.osp.custom_gfx[i][5],
			    (float)cgs.osp.custom_gfx[i][6],
			    (float)cgs.osp.custom_gfx[i][7],
			    cgs.media.customShader[i]);
		}
	}

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawIntermission();

	if (cgs.osp.isOSPv1)
	{

		qtime_t qtime;
		char* str;
		trap_RealTime(&qtime);
		str = va("%02d:%02d:%02d - %s - %02d %s %d",
		         qtime.tm_hour,
		         qtime.tm_min,
		         qtime.tm_sec,
		         dayOfWeekName[qtime.tm_wday],
		         qtime.tm_mday,
		         monthName[qtime.tm_mon],
		         qtime.tm_year + 1900);

		CG_OSPDrawStringWithShadow(8, 462, str, 10, 10, colorWhite, 0);
		str = va("^B%s", CG_ConfigString(CS_OSP_VERSION_STR));
		CG_OSPDrawStringWithShadow(8, 450, str, 10, 10, colorYellow, 0);
	}
	CG_OSPSetColor(NULL);
	CG_DrawPicOld(590.0f, 430.f, 50.0f, 50.0f, cgs.media.blender180Shader);

	if (cg.showWStats)
	{
		CG_BEStatsShowStatsInfo();
	}
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void)
{
	float       x;
	vec4_t      color;
	const char*  name;

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	CG_DrawBigString(SCREEN_WIDTH - 1, 24, "following", 1.0f, DS_HRIGHT | DS_SHADOW, 0);

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(name));

	CG_DrawStringExt(x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
void CG_DrawAmmoWarning(void)
{
	const char*  s;

	if (cg_drawAmmoWarning.integer == 0)
	{
		return;
	}

	if (!cg.lowAmmoWarning)
	{
		return;
	}

	if (cg.lowAmmoWarning == 2)
	{
		s = "OUT OF AMMO";
	}
	else
	{
		s = "LOW AMMO WARNING";
	}
	CG_DrawBigString(SCREEN_WIDTH / 2.0f, 64, s, 1.0f, DS_HCENTER | DS_SHADOW, 0);
}



/*
=================
CG_DrawWarmup
=================
*/
void CG_DrawWarmup(void)
{
	int sec = cg.warmup;

	if (sec == 0)
	{
		return;
	}
	else if (sec < 0)
	{
		const char* text = cgs.gametype != GT_TOURNAMENT ? "Waiting for Players" : "Waiting for Opponent";
		int textWidth = CG_DrawStrlen(text) * 14;
		CG_OSPDrawStringWithShadow(320 - (textWidth / 2), 24, va("^B%s", text), 14, 14, colorRed, 0);
		cg.warmupCount = 0;
	}
	else // warmup > 0
	{
		if (cgs.gametype == GT_TOURNAMENT)
		{
			const char* player1Name = NULL;
			const char* player2Name = NULL;
			int i;

			for (i = 0; i < cgs.maxclients; ++i)
			{
				if (cgs.clientinfo[i].name[0] && cgs.clientinfo[i].team == TEAM_FREE)
				{
					if (!player1Name)
					{
						player1Name = cgs.clientinfo[i].name;
					}
					else
					{
						player2Name = cgs.clientinfo[i].name;
						break;
					}
				}
			}
			if (player1Name && player2Name)
			{
				int len;
				int width;
				const char* text;
				text = va("%s^7 vs %s", player1Name, player2Name);

				len = CG_DrawStrlen(text);

				if (len > 20)
				{
					width = SCREEN_WIDTH / len;
				}
				else
				{
					width = 32;
				}

				CG_OSPDrawStringWithShadow((SCREEN_WIDTH - (len * width)) / 2, 20, text, width, (int)(1.5 * (float)width), colorWhite, 0);
			}
		}
		else
		{
			const char* text;
			int len;
			int width;

			if (cgs.osp.clanBaseTeamDM)
			{
				text = "ClanBase TeamDM";
			}
			else if (cgs.gametype == GT_FFA)
			{
				text = "Free for all";
			}
			else if (cgs.gametype == GT_TEAM)
			{
				if (cgs.osp.gameTypeFreeze == 0)
				{
					text = "Team Deathmatch";
				}
				else if (cgs.osp.gameTypeFreeze == 2)
				{

					text = "FreezeTag TDM (Vanilla)";
				}
				else
				{
					text = "FreezeTag TDM";
				}
			}
			else if (cgs.gametype == GT_CTF)
			{
				text = "Capture the Flag";
			}
			else if (cgs.gametype == GT_CA)
			{
				text = "Clan Arena";
			}
			else
			{
				text = "";
			}

			len = CG_DrawStrlen(text);
			if (len > 0x14)
			{
				width = SCREEN_WIDTH / len;
			}
			else
			{
				width = 32;
			}
			CG_DrawStringExt((SCREEN_WIDTH - (len * width)) / 2, 25, text, colorLtGrey, 0, 1, width, (int)(1.1 * (float)width), qfalse);
		}

		if (cg.showScores == 0)
		{
			int cw;
			float* color = colorWhite;
			const char* s;
			int w;

			sec = (sec - cg.time) / 1000;
			if (sec < 0)
			{
				cg.warmup = 0;
				sec = 0;
			}
			s = va("Starts in: %i", sec + 1);
			if (sec != cg.warmupCount)
			{
				cg.warmupCount = sec;
				switch (sec)
				{
					case 0:
						trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
						break;
					case 1:
						trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
						break;
					case 2:
						trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
						break;
					default:
						break;
				}
			}

			switch (cg.warmupCount)
			{
				case 0:
					cw = 28;
					color = colorLtGrey;
					break;
				case 1:
					cw = 24;
					color = colorYellow;
					break;
				case 2:
					cw = 20;
					color = colorWhite;
					break;
				default:
					cw = 16;
					break;
			}

			w = CG_DrawStrlen(s);
			CG_DrawStringExt(320 - w * cw / 2, 70, s, color, qfalse, qtrue, cw, (int)(cw * 1.5), 0);
		}
	}
}

void CG_DrawWarmupShud(void)
{
	int sec = cg.warmup;

	if (sec == 0)
	{
		return;
	}
	else if (sec < 0)
	{
		cg.warmupCount = 0;
	}
	else // warmup > 0
	{
		if (cg.showScores == 0)
		{
			sec = (sec - cg.time) / 1000;
			if (sec < 0)
			{
				cg.warmup = 0;
				sec = 0;
			}
			if (sec != cg.warmupCount)
			{
				cg.warmupCount = sec;
				switch (sec)
				{
					case 0:
						trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
						break;
					case 1:
						trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
						break;
					case 2:
						trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
						break;
					default:
						break;
				}
			}
		}
	}
}

/*
===
New credits
===
*/
void CG_OSPDrawNewCredits(void)
{
	int i;

	int showTime = 8000;

	float x = 100.0f;
	float y = 80.0f;
	float w = 440.0f;
	float h = 342.0f;
	float charHeight = 14, charWidth = 8;
	float textX = (w / 2) + x;
	float textY = y + 2.0f;
	float lineY;
	int borderSize = 2;
	int direction = 12;
	float speed = 0.005f;
	float gradientScale = 0.05f;
	vec4_t bgColor = { 0.05, 0.05, 0.05, 0.5 };
	vec4_t border = { 0, 0, 0, 2 };
	float firstLineHeight;
	float baseY;

	char* credits[] =
	{
		"OSP2 BLACK EDITION",
		"by diwoc",
		"https://scoqx.github.io/",
		"",
		"Based on OSP2 by Snems",
		"https://github.com/snems/OSP2",
		"",
		"Special thanks to:",
		"Snems, Kr3m, Mirage, Mus1n",
		"Zenx, Tuborg, Meta, Konstalker",
		"enq3, John Coffey, Maked, Roskoshny",
		"",
		"Based on source codes/assets:",
		"OSP   ^Bhttps://www.orangesmoothie.org",
		"Cyrax   ^Bhttps://github.com/ec-/baseq3a",
		"x0ry   ^Bhttps://github.com/xq3e/engine",
		"Neil Toronto   ^Bhttp://ra.is/unlagged",
		"Ratmod   ^Bhttps://github.com/rdntcntrl/ratoa_gamecode",
		"Devotion   ^Bhttps://github.com/alcachofass/devotion",
		"Q3 1.16n X-Mod   ^Bhttps://github.com/navgg/Quake-3-1.16n-extensions",
		"Nemesis 1.16n   ^Bhttps://github.com/dledmonds/quake3-nemesis-1.16n",
		"CPMA   ^Bhttps://playmorepromode.com/",
		"Trinity   ^Bhttps://github.com/ernie/trinity",
		NULL
	};
	static int startTime = 0;


	if (!cg.beShowCredits)
	{
		startTime = 0;
		return;
	}

	if (startTime == 0)
	{
		startTime = cg.time;
	}

	/* Don't auto-close credits if slot machine is active */
	if (cg.time - startTime > showTime)
	{
		cg.beShowCredits = qfalse;
		startTime = 0;
		return;
	}

	// background
	CG_FillRect(x, y, w, h, bgColor);

	// version
	CG_OSPDrawStringNew(
	    x + w - 2, y,
	    OSP_VERSION,
	    colorBlack,
	    NULL,
	    6, 8,
	    0,
	    DS_PROPORTIONAL | DS_HRIGHT,
	    4,
	    NULL, NULL, NULL
	);

	firstLineHeight = charHeight * 2.0f;

	// title
	CG_OSPDrawStringNew(
	    textX, textY,
	    credits[0],
	    colorWhite,
	    colorBlack,
	    charWidth * 2, firstLineHeight,
	    w,
	    DS_SHADOW | DS_PROPORTIONAL | DS_HCENTER,
	    6,
	    NULL, border, colorBlack
	);

	baseY = textY + firstLineHeight;

	// credits
	for (i = 1; credits[i] != NULL; i++)
	{
		lineY = baseY + (i - 1) * charHeight;
		CG_OSPDrawStringNew(
		    textX, lineY,
		    credits[i],
		    colorWhite,
		    colorBlack,
		    charWidth, charHeight,
		    w,
		    DS_SHADOW | DS_PROPORTIONAL | DS_HCENTER,
		    16,
		    NULL, NULL, NULL
		);
	}
	// border
	CG_OSPDrawGradientFrame(x, y, w, h, borderSize, direction, speed, gradientScale, 2);
}

/*
===
Team Indicator
===
*/
float lastLift[MAX_CLIENTS];
#define INV_DIST_RANGE (1.0f / 2500.0f)

/*
=================
CG_TeamIndicatorLayoutPitch

Vertical pitch between stacked indicators: full drawn block height (same
layout as CG_DrawPlayerIndicatorOnScreen) plus a gap scaled to font height.
=================
*/
float CG_TeamIndicatorLayoutPitch(float h, qboolean hasName, qboolean hasStats, qboolean hasStatsBars, qboolean hasIcon, qboolean showHealthRows)
{
	float extent;
	float gap;
	float h2;
	float bottom;

	extent = 0.0f;

	if (hasName)
	{
		extent = h;
	}

	if (showHealthRows)
	{
		if (hasStatsBars)
		{
			bottom = h + (h * 0.75f);
			if (bottom > extent)
			{
				extent = bottom;
			}
		}
		else if (hasStats)
		{
			h2 = h - 2.0f;
			if (h2 < 4.0f)
			{
				h2 = 4.0f;
			}
			bottom = h + h2;
			if (bottom > extent)
			{
				extent = bottom;
			}
		}
	}

	if (hasIcon)
	{
		bottom = (h * 1.1f) + h;
		if (bottom > extent)
		{
			extent = bottom;
		}
	}

	if (extent <= 0.0f)
	{
		if (hasStats || hasStatsBars || hasIcon)
		{
			extent = h;
			if (showHealthRows && hasStatsBars)
			{
				extent = h + (h * 0.75f);
			}
			else if (showHealthRows && hasStats)
			{
				h2 = h - 2.0f;
				if (h2 < 4.0f)
				{
					h2 = 4.0f;
				}
				extent = h + h2;
			}
		}
	}

	gap = h * 0.12f;
	if (gap < 2.0f)
	{
		gap = 2.0f;
	}

	return extent + gap;
}

/*
=================
CG_IsPointVisibleFromView

No solid geometry between vieworg and point (same traces as team indicator ally check).
=================
*/
qboolean CG_IsPointVisibleFromView(const vec3_t point)
{
	vec3_t start;
	vec3_t end;
	trace_t trace;

	if (!cg.snap)
	{
		return qfalse;
	}

	VectorCopy(cg.refdef.vieworg, start);
	VectorCopy(point, end);
	end[2] += 24.0f;
	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID);
	if (trace.fraction != 1.0f)
	{
		return qfalse;
	}
	CG_Trace(&trace, start, vec3_origin, vec3_origin, point, cg.snap->ps.clientNum, CONTENTS_SOLID);
	return (trace.fraction == 1.0f);
}

static qboolean CG_TeamIndicatorClientVisible(int clientIndex, qboolean gameModeNotSpectator, qboolean isSpec,
        qboolean useTeamFrozen, vec3_t markerOrigin, centity_t** outCent)
{
	centity_t* cent;
	clientInfo_t* ci;
	qboolean isEnemy;
	qboolean friendVisible;
	qboolean needVis;
	qboolean allyFrozenForVis;

	if (!cg.snap || clientIndex == cg.snap->ps.clientNum)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[clientIndex];
	if (!ci->infoValid || (gameModeNotSpectator && CG_CACHED_IS_ENEMY_CI(ci)))
	{
		return qfalse;
	}

	if (ci->isFrozenEnt)
	{
		cent = &cg_entities[ci->frozenEntity];
		if (!cent->currentValid || !CG_IS_FROZEN_ENTITY(cent) || cent->currentState.otherEntityNum != clientIndex)
		{
			ci->isFrozenEnt = qfalse;
			ci->frozenEntity = -1;
			cent = &cg_entities[clientIndex];
		}
	}
	else
	{
		cent = &cg_entities[clientIndex];
	}

	if (!CG_ClientTeamMarkerOrigin(clientIndex, markerOrigin))
	{
		return qfalse;
	}

	if (ci->team == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	isEnemy = (ci->isFrozenEnt || !gameModeNotSpectator) ? CG_CACHED_IS_ENEMY_CI(ci) : qfalse;
	if (isEnemy && (!isSpec || !(cg_teamIndicator.integer & PI_SPECTATOR)))
	{
		return qfalse;
	}

	allyFrozenForVis = CG_ClientMarkerIsFrozen(ci);

	friendVisible = qtrue;
	needVis = ((cg_friendsWallhack.integer & 2) == 0) ||
	          (allyFrozenForVis && !isEnemy && (!cg_friendsWallhackFrozen.integer || !useTeamFrozen));
	if (needVis)
	{
		friendVisible = CG_IsPointVisibleFromView(markerOrigin);
	}
	if (!friendVisible && ((cg_friendsWallhack.integer & 2) == 0 ||
	                       (allyFrozenForVis && !isEnemy && (!cg_friendsWallhackFrozen.integer || !useTeamFrozen))))
	{
		return qfalse;
	}

	if (outCent)
	{
		*outCent = cent;
	}
	return qtrue;
}

static void CG_DrawTeamIndicatorForClient(int clientNum, centity_t* cent, clientInfo_t* ci, vec3_t markerOrigin,
        qboolean hasName, qboolean hasSpectator, qboolean hasStats, qboolean hasStatsBars, qboolean hasIcon,
        qboolean isSpec, float invFadeRadius, int indicatorFadeRadiusSq)
{
	vec3_t headPos;
	vec3_t toTarget;
	float headX;
	float headY;
	float stackBaseY;
	float w;
	float h;
	float dist;
	float lift;
	float rawLift;
	float t;
	vec4_t nameColor;
	vec4_t bgColor;
	float fade;
	float* pLift;
	qboolean markerDead;
	qboolean markerFrozen;

	markerFrozen = CG_ClientMarkerIsFrozen(ci);
	markerDead = markerFrozen ? qfalse :
	             (ci->globalPos ? (ci->health <= 0) :
	              ((cent->currentValid && cent->currentState.eType == ET_PLAYER) &&
	               (cent->currentState.eFlags & EF_DEAD)));

	VectorSubtract(markerOrigin, cg.refdef.vieworg, toTarget);
	dist = VectorLength(toTarget);

	t = dist * INV_DIST_RANGE;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	if (cgs.bePlayerIndicatorFontSize[0] > 0.0f || cgs.bePlayerIndicatorFontSize[1] > 0.0f)
	{
		w = cgs.bePlayerIndicatorFontSize[0];
		h = cgs.bePlayerIndicatorFontSize[1];
	}
	else
	{
		h = (12.0f - (t * 8.0f)) * cg_teamIndicatorAdjust.value;
		w = (h * 0.75f) * cg_teamIndicatorAdjust.value;
	}

	rawLift = (58.0f + cg_teamIndicatorOffset.value) + t * 70.0f;
	pLift = &lastLift[clientNum];
	lift = *pLift + (rawLift - *pLift) * 0.2f;
	*pLift = lift;

	VectorCopy(markerOrigin, headPos);
	headPos[2] += lift;

	if (!CG_WorldToScreen(headPos, &headX, &headY))
	{
		return;
	}

	stackBaseY = headY;
	if (cg.teamMarkerStackGroupSize[clientNum] > 1 || CG_WorldFlags_ClientHasStackDropFlag(clientNum))
	{
		float pitch;
		float halfPitch;
		int n;
		int idx;
		qboolean layoutHealthRows;

		layoutHealthRows = hasStats || hasStatsBars;
		pitch = CG_TeamIndicatorLayoutPitch(h, hasName, hasStats, hasStatsBars, hasIcon, layoutHealthRows);
		halfPitch = pitch * 0.5f;
		n = CG_TeamMarkerStackEffectiveSize(clientNum);
		idx = cg.teamMarkerStackIndex[clientNum];
		if (n > 1)
		{
			headY -= (idx * 2 - (n - 1)) * halfPitch;
		}
	}

	Vector4Copy(cgs.bePlayerIndicatorBgColor, bgColor);
	Vector4Copy(cgs.bePlayerIndicatorColor, nameColor);

	fade = 1.0f;
	if (cg_teamIndicatorFade.value > 0.0f)
	{
		float dx = headX - HALF_SCREEN_W;
		float dy = (headY + (cgs.bePlayerIndicatorFontSize[1] * 0.5f)) - HALF_SCREEN_H;
		float distSq = dx * dx + dy * dy;

		if (distSq < indicatorFadeRadiusSq)
		{
			float coreFade = SQRTFAST(distSq) * invFadeRadius;
			if (coreFade < 0.0f)
			{
				coreFade = 0.0f;
			}
			fade = 1.0f - ((1.0f - coreFade) * cg_teamIndicatorFade.value);
		}
	}

	bgColor[3] = cg_teamIndicatorBgOpaque.value * fade;
	nameColor[3] = cg_teamIndicatorOpaque.value * fade;
	if (ci->globalPos)
	{
		bgColor[3] *= 0.5f;
		nameColor[3] *= 0.5f;
	}

	if (CG_WorldFlags_ClientHasStackDropFlag(clientNum) && cgs.media.teamTasksFlag)
	{
		vec4_t dropColor;
		float flagSizeH;
		float flagSizeW;
		float flagX;
		float flagY;
		float pitchDrop;
		float halfPitchDrop;
		int n;
		qboolean layoutHealthRows;

		layoutHealthRows = hasStats || hasStatsBars;
		n = CG_TeamMarkerStackEffectiveSize(clientNum);
		pitchDrop = CG_TeamIndicatorLayoutPitch(h, hasName, hasStats, hasStatsBars, hasIcon, layoutHealthRows);
		halfPitchDrop = pitchDrop * 0.5f;
		CG_WorldFlags_GetStackDropFlagColor(clientNum, dropColor);
		dropColor[3] *= nameColor[3];
		flagSizeH = h;
		flagSizeW = h * 0.75f;
		flagY = stackBaseY - ((n - 1) * 2 - (n - 1)) * halfPitchDrop;
		flagX = headX - (flagSizeW * 0.5f);
		flagY -= flagSizeH * 0.5f;
		CG_DrawPicWithColor(flagX, flagY, flagSizeW, flagSizeH, dropColor, cgs.media.teamTasksFlag);
	}

	if (cgs.gametype == GT_CTF && !CG_CACHED_IS_ENEMY_CI(ci) && cg_drawIcons.integer && cgs.media.teamTasksFlag &&
	        (ci->teamPowerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG))))
	{
		vec4_t flagColor;
		float flagSizeH = h;
		float flagSizeW = h * 0.75f;
		float flagX = headX - (flagSizeW * 0.5f);
		float flagY = headY - (flagSizeH * 0.5f);

		if (ci->teamPowerups & (1 << PW_REDFLAG))
		{
			Vector4Copy(cgs.beRedTeamColor, flagColor);
		}
		else
		{
			Vector4Copy(cgs.beBlueTeamColor, flagColor);
		}
		flagColor[3] = nameColor[3];
		CG_DrawPicWithColor(flagX, flagY, flagSizeW, flagSizeH, flagColor, cgs.media.teamTasksFlag);
	}

	/* Powerup icons over indicator: horizontal row with a 1s heartbeat pulse (POWERUP_BLINK_TIME) */
	if (!markerDead)
	{
		static const int puList[6] = { PW_QUAD, PW_BATTLESUIT, PW_HASTE, PW_INVIS, PW_REGEN, PW_FLIGHT };
		int puCount;
		int k;

		puCount = 0;
		for (k = 0; k < 6; k++)
		{
			if (ci->teamPowerups & (1 << puList[k]))
			{
				if (puList[k] == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
				{
					continue;
				}
				puCount++;
			}
		}

		if (puCount > 0)
		{
			float puH = h;
			float puW = h * 0.75f;
			float puGap = puW * 0.25f;
			float puStride = puW + puGap;
			float rowX = headX - (puCount * puW + (puCount - 1) * puGap) * 0.5f;
			float rowY = headY - puH * 1.25f;
			float phase = (float)(cg.time % POWERUP_BLINK_TIME) / (float)POWERUP_BLINK_TIME;
			float decay = 1.0f - phase;             /* 1 at beat start -> 0 at end */
			float pulseFade = decay;                /* pops in at full alpha, fades out */
			float pulseScale = 1.0f + 0.8f * decay; /* starts big (1.8x), shrinks to 1.0 */
			int slot = 0;
			vec4_t iconColor;
			vec4_t pulseColor;

			for (k = 0; k < 6; k++)
			{
				qhandle_t shader;
				float cx;
				float cy;
				int dyeItemNum = 0;

				if (!(ci->teamPowerups & (1 << puList[k])))
					continue;

				if (puList[k] == PW_BATTLESUIT && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
					continue;

				switch (puList[k])
				{
					case PW_QUAD:
					case PW_BATTLESUIT:
					{
						/* Quad/BS use the dye icon path so cg_quadDamageColor/cg_battleSuitColor tint applies (allies are teammates) */
						gitem_t* item = BG_FindItemForPowerup(puList[k]);
						if (item)
						{
							dyeItemNum = (int)(item - bg_itemlist);
							CG_RegisterItemVisuals(dyeItemNum);
							shader = CG_DyeFlagResolveInverseIcon(dyeItemNum, cg_items[dyeItemNum].icon);
						}
						else
						{
							shader = (puList[k] == PW_QUAD) ? cgs.media.quadDamageIcon : cgs.media.battleSuitIcon;
						}
						break;
					}
					case PW_HASTE:
						shader = cgs.media.hasteIcon;
						break;
					case PW_INVIS:
						shader = cgs.media.invisIcon;
						break;
					case PW_REGEN:
						shader = cgs.media.regenIcon;
						break;
					default:
						shader = cgs.media.flightIcon;
						break;
				}

				cx = rowX + slot * puStride + puW * 0.5f;
				cy = rowY + puH * 0.5f;
				slot++;

				if (!shader)
					continue;

				/* Base icon color; team-tint quad/BS when their color cvar is set */
				Vector4Set(iconColor, 1.0f, 1.0f, 1.0f, nameColor[3]);
				if (dyeItemNum)
					CG_ModulateVec4ForCustomItemDyeIconForClient(dyeItemNum, clientNum, iconColor);

				/* Pulse copy behind: grows and fades out over the cycle */
				if (pulseFade > 0.0f)
				{
					float pw2 = puW * pulseScale;
					float ph2 = puH * pulseScale;
					Vector4Set(pulseColor, iconColor[0], iconColor[1], iconColor[2], nameColor[3] * pulseFade);
					CG_DrawPicWithColor(cx - pw2 * 0.5f, cy - ph2 * 0.5f, pw2, ph2, pulseColor, shader);
				}

				/* Static icon on top */
				CG_DrawPicWithColor(cx - puW * 0.5f, cy - puH * 0.5f, puW, puH, iconColor, shader);
			}
		}
	}

	if (hasName)
	{
		char tmpName[MAX_QPATH];
		vec4_t border;
		vec4_t borderColor;
		vec4_t shadowColorWithAlpha;

		Vector4Set(border, 0.0f, 0.0f, 0.0f, 0.0f);
		Vector4Set(borderColor, 0.0f, 0.0f, 0.0f, 0.0f);
		Vector4Set(shadowColorWithAlpha, 0.0f, 0.0f, 0.0f, nameColor[3]);
		if (markerFrozen)
		{
			border[3] = 1.0f;
			Vector4Copy(colorCyan, borderColor);
			borderColor[3] = nameColor[3];
		}

		if (cg_teamIndicator.integer & PI_NAME_CLEAN)
		{
			Q_strncpyz(tmpName, ci->name_clean, sizeof(tmpName));
		}
		else
		{
			Q_strncpyz(tmpName, ci->name, sizeof(tmpName));
		}

		CG_OSPDrawStringNew(
		    headX, headY,
		    tmpName,
		    nameColor,
		    shadowColorWithAlpha,
		    w, h,
		    cg_teamIndicatorMaxLength.integer,
		    DS_SHADOW | DS_PROPORTIONAL | DS_HCENTER | DS_MAX_WIDTH_IS_CHARS,
		    cgFontIndices.teamIndicator,
		    bgColor,
		    border,
		    borderColor
		);
	}
	if ((isSpec && hasSpectator) || cgs.beMarkedTeam[clientNum])
	{
		vec4_t teamColor;
		float rectW = w * 2.0f;
		float rectH = h * 0.1f;
		float rectX = headX - (rectW * 0.5f);
		float rectY = headY + h - rectH;

		if (isSpec && hasSpectator)
		{
			if (ci->team == TEAM_RED)
			{
				Vector4Copy(cgs.beRedTeamColor, teamColor);
			}
			else if (ci->team == TEAM_BLUE)
			{
				Vector4Copy(cgs.beBlueTeamColor, teamColor);
			}
			else
			{
				Vector4Set(teamColor, 0.5f, 0.5f, 0.5f, nameColor[3]);
			}
		}
		else
		{
			Vector4Copy(cg.beMarkedTeamColor, teamColor);
		}
		teamColor[3] = nameColor[3];
		CG_FillRect(rectX, rectY, rectW, rectH, teamColor);
	}

	if (hasStatsBars && !markerDead && ci->health > 0)
	{
		float barW = w * 5.0f;
		float barH = h * 0.75f;
		float barX = headX - (barW * 0.5f);
		float barY = headY + h;
		int barFontIndex;

		barFontIndex = hasStats ? cgFontIndices.teamIndicator : -1;
		CG_DrawHealthBar(barX, barY, barW, barH, ci->health, ci->armor, nameColor[3], barFontIndex, qfalse,
		                 &cg.healthColorByClient[clientNum], qfalse);
	}
	else if (hasStats && !markerDead && ci->health > 0)
	{
		char statsText[16];
		vec4_t statsColor;
		vec4_t statsShadow;
		float h2 = h - 2.0f;
		float w2;
		int hbStyle = cg_healthBarStyle.integer;
		int statsFlags = DS_PROPORTIONAL | DS_HCENTER;

		if (hbStyle & HBAR_STYLE_SHADOW)
		{
			statsFlags |= DS_SHADOW;
		}

		if (h2 < 4.0f) h2 = 4.0f;
		w2 = h2 * 0.8f;

		Com_sprintf(statsText, sizeof(statsText), "[%d/%d]", ci->health, ci->armor);

		if (hbStyle & HBAR_STYLE_FONT_BLACK)
		{
			Vector4Set(statsColor, 0.0f, 0.0f, 0.0f, nameColor[3]);
			Vector4Set(statsShadow, 1.0f, 1.0f, 1.0f, nameColor[3]);
		}
		else
		{
			Vector4Copy(cg.healthColorByClient[clientNum], statsColor);
			statsColor[3] = nameColor[3];
			Vector4Set(statsShadow, 0.0f, 0.0f, 0.0f, statsColor[3]);
		}

		CG_OSPDrawStringNew(
		    headX, headY + h,
		    statsText,
		    statsColor,
		    statsShadow,
		    w2, h2,
		    SCREEN_WIDTH,
		    statsFlags,
		    cgFontIndices.teamIndicator,
		    bgColor,
		    NULL,
		    NULL
		);
	}

	if (hasIcon)
	{
		float iconW = w;
		float iconH = h;
		float iconX = headX - (iconW * 0.5f);
		float iconY = headY + (iconH + (iconH * 0.1f));

		if (markerDead)
		{
			vec4_t color;
			Vector4Set(color, 1.0f, 1.0f, 1.0f, nameColor[3]);
			trap_R_SetColor(color);
			CG_DrawPic(iconX, iconY, iconW, iconH, cgs.media.obituariesSkull);
			trap_R_SetColor(NULL);
		}
		else if (markerFrozen && (cg_teamIndicator.integer & PI_FROZEN))
		{
			qboolean hasActiveMark;

			hasActiveMark = (clientNum >= 0 && clientNum < MAX_CLIENTS &&
			                 cg.teamMarkers.clientHasFrozenAllyMarkNear[clientNum]);
			if (!hasActiveMark)
			{
				vec4_t color;
				Vector4Set(color, 1.0f, 1.0f, 1.0f, nameColor[3]);
				trap_R_SetColor(color);
				CG_DrawPic(iconX, iconY, iconW, iconH, cgs.media.frozenFoeTagShader);
				trap_R_SetColor(NULL);
			}
		}
	}
}

void CG_DrawPlayerIndicatorOnScreen(void)
{
	qboolean gameModeNotSpectator;
	qboolean useTeamFrozen;
	qboolean isSpec;
	qboolean hasName;
	qboolean hasSpectator;
	qboolean hasStats;
	qboolean hasStatsBars;
	qboolean hasIcon;
	float invFadeRadius;
	int indicatorFadeRadiusSq;
	int i;

	if (!cg.snap)
	{
		return;
	}
	if (!cg_teamIndicator.integer)
	{
		return;
	}
	if (cgs.gametype < GT_TEAM)
	{
		if (!CG_IS_LOCAL_CLIENT_SPECTATOR() || !(cg_teamIndicator.integer & PI_SPECTATOR))
		{
			return;
		}
	}
	else if (CG_IS_LOCAL_CLIENT_SPECTATOR() && !(cg_teamIndicator.integer & PI_SPECTATOR))
	{
		return;
	}

	useTeamFrozen = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FROZEN);
	gameModeNotSpectator = !CG_IS_LOCAL_CLIENT_SPECTATOR();
	isSpec = CG_IS_LOCAL_CLIENT_SPECTATOR();
	hasName = (cg_teamIndicator.integer & (PI_NAME_CLEAN | PI_NAME)) != 0;
	hasSpectator = (cg_teamIndicator.integer & PI_SPECTATOR) != 0;
	hasStats = (cg_teamIndicator.integer & PI_STATS) != 0;
	hasStatsBars = (cg_teamIndicator.integer & PI_STATS_BARS) != 0;
	hasIcon = (cg_teamIndicator.integer & PI_ICON) != 0;
	if (!hasName && !hasStats && !hasStatsBars && !hasIcon)
	{
		return;
	}
	invFadeRadius = (cg_teamIndicatorFadeRadius.value > 0.0f) ? (1.0f / cg_teamIndicatorFadeRadius.value) : 0.0f;
	indicatorFadeRadiusSq = (cg_teamIndicatorFadeRadius.value > 0.0f) ?
	                        (cg_teamIndicatorFadeRadius.value * cg_teamIndicatorFadeRadius.value) : 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		vec3_t markerOrigin;

		if (CG_TeamIndicatorClientVisible(i, gameModeNotSpectator, isSpec, useTeamFrozen, markerOrigin, NULL))
		{
			break;
		}
	}
	if (i >= MAX_CLIENTS)
	{
		return;
	}

	CG_EnsureTeamMarkerStackCache(gameModeNotSpectator);

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		centity_t* cent;
		clientInfo_t* ci;
		vec3_t markerOrigin;

		if (!CG_TeamIndicatorClientVisible(i, gameModeNotSpectator, isSpec, useTeamFrozen, markerOrigin, &cent))
		{
			continue;
		}

		ci = &cgs.clientinfo[i];
		CG_DrawTeamIndicatorForClient(i, cent, ci, markerOrigin, hasName, hasSpectator, hasStats, hasStatsBars, hasIcon,
		                              isSpec, invFadeRadius, indicatorFadeRadiusSq);
	}
}



typedef enum
{
	STATS_POS_TOP,
	STATS_POS_BOTTOM,
	STATS_POS_LEFT,
	STATS_POS_RIGHT
} shudWeaponStatsPos_t;

void CG_DrawWeaponStats(shudWeaponStatsPos_t position, float iconSize)
{
	float iconX = 0.0f, iconY = 0.0f;
	const float blockSpacing = 4.0f, textOffset = 2.0f;
	int wp, visibleCount = 0;
	char accStr[16];
	int textFlags;
	newStatsInfo_t* ws = &cgs.newStats;
	float charWidth;  /* Character width for text */
	float charHeight; /* Character height for text */
	int maxCharCount = 4;
	float approxTextWidth;
	int shots;
	qboolean horizontal, iconBeforeText;
	float textX, textY;
	int font = cgFontIndices.accuracy;
	qhandle_t icon;

	/* Get character width and height from cvar or use default */
	if (cgs.beAccuracyFontSize[0] > 0.0f || cgs.beAccuracyFontSize[1] > 0.0f)
	{
		/* Use custom width and height from cvar */
		charWidth = cgs.beAccuracyFontSize[0];  /* Width from cvar */
		charHeight = cgs.beAccuracyFontSize[1];  /* Height from cvar */
		/* If only one value provided, use it for both */
		if (charWidth <= 0.0f)
		{
			charWidth = charHeight;
		}
		if (charHeight <= 0.0f)
		{
			charHeight = charWidth;
		}
	}
	else
	{
		/* Use old behavior: single value for both width and height */
		charWidth = 8;
		charHeight = 10;
	}
	approxTextWidth = charWidth * maxCharCount;


	for (wp = WP_MACHINEGUN; wp <= WP_PLASMAGUN; wp++)
		if (ws->stats[wp].shots > 0) visibleCount++;
	if (visibleCount == 0) return;

	switch (position)
	{
		case STATS_POS_TOP:
			iconY = 0.0f;
			iconX = (SCREEN_WIDTH - visibleCount * (iconSize + textOffset + approxTextWidth + blockSpacing) + blockSpacing) * 0.5f;
			break;
		case STATS_POS_BOTTOM:
			iconY = SCREEN_HEIGHT - iconSize;
			iconX = (SCREEN_WIDTH - visibleCount * (iconSize + textOffset + approxTextWidth + blockSpacing) + blockSpacing) * 0.5f;
			break;
		case STATS_POS_LEFT:
			iconX = 0.0f;
			iconY = (SCREEN_HEIGHT - visibleCount * (iconSize + blockSpacing) + blockSpacing) * 0.5f;
			break;
		case STATS_POS_RIGHT:
			iconX = SCREEN_WIDTH - iconSize;
			iconY = (SCREEN_HEIGHT - visibleCount * (iconSize + blockSpacing) + blockSpacing) * 0.5f;
			break;
	}

	horizontal = (position == STATS_POS_TOP || position == STATS_POS_BOTTOM) ? qtrue : qfalse;
	iconBeforeText = (position != STATS_POS_RIGHT) ? qtrue : qfalse;

	{
		float bgX = iconX;
		float bgY = iconY;
		float bgW, bgH;
		vec4_t bgColor = { 0, 0, 0, 0.25f };

		if (horizontal)
		{
			bgW = visibleCount * (iconSize + textOffset + approxTextWidth + blockSpacing) - blockSpacing;
			bgH = iconSize;
		}
		else
		{
			bgW = iconSize + textOffset + approxTextWidth;
			bgH = visibleCount * (iconSize + blockSpacing) - blockSpacing;
			if (position == STATS_POS_RIGHT)
			{
				bgX -= approxTextWidth;
			}
		}

		trap_R_SetColor(bgColor);
		CG_FillRect(bgX, bgY, bgW, bgH, bgColor);
		trap_R_SetColor(NULL);
	}

	for (wp = WP_MACHINEGUN; wp <= WP_PLASMAGUN; wp++)
	{
		shots = ws->stats[wp].shots;
		if (shots <= 0)
			continue;

		icon = cg_weapons[wp].weaponIcon;
		Com_sprintf(accStr, sizeof(accStr), "%.0f%%", ws->stats[wp].accuracy);
		textFlags = DS_SHADOW | DS_PROPORTIONAL;
		textFlags |= iconBeforeText ? DS_HLEFT : DS_HRIGHT;

		textX = iconBeforeText ? iconX + iconSize + textOffset : iconX - textOffset;
		textY = iconY + (iconSize - charHeight) * 0.5f;

		if (iconBeforeText)
		{
			if (icon)
			{
				trap_R_SetColor(colorWhite);
				CG_DrawPic(iconX, iconY, iconSize, iconSize, icon);
				trap_R_SetColor(NULL);
			}
		}
		CG_OSPDrawStringNew(textX, textY, accStr, colorWhite, colorBlack, charWidth, charHeight,
		                    SCREEN_WIDTH, textFlags, font, NULL, NULL, NULL);

		if (!iconBeforeText)
		{
			if (icon)
			{
				trap_R_SetColor(colorWhite);
				CG_DrawPic(iconX, iconY, iconSize, iconSize, icon);
				trap_R_SetColor(NULL);
			}
		}

		if (horizontal)
			iconX += iconSize + textOffset + approxTextWidth + blockSpacing;
		else
			iconY += iconSize + blockSpacing;
	}
}


void CG_DrawWeaponStatsWrapper(void)
{
	shudWeaponStatsPos_t position;
	float iconSize;

	if (!cg_drawAccuracy.integer || !cg.scoreBoardShowing)
	{
		return;
	}

	iconSize = cg_accuracyIconSize.value;

	switch (cg_drawAccuracy.integer)
	{
		case 1:
			position = STATS_POS_LEFT;
			break;
		case 2:
			position = STATS_POS_TOP;
			break;
		case 3:
			position = STATS_POS_RIGHT;
			break;
		case 4:
			position = STATS_POS_BOTTOM;
			break;
		default:
			return;
	}

	CG_DrawWeaponStats(position, iconSize);
}


/*
=================
CG_DrawDamageFrame

Draws both health low overlay and damage frame based on cvars and game state.
=================
*/
static void CG_DrawDamageFrame(void)
{
	vec4_t damageColor, damageRed, borderSizeOriginal;

	/* Vignette mode: replaces blood/indicator; skip manual hit frames (bits 1|2) */
	if (cg_damageDraw.integer == 3)
	{
		if ((cg_damageDrawFrame.integer & 4) && cg.beIsHealthLow)
		{
			float damagePulse = sin((float)cg.time * 2.0f * M_PI / 2000.0f) * 0.2f + 0.4f;
			CG_DamageBorderVignetteLowHealth(damagePulse * cg_damageFrameOpaque.value);
		}
		CG_DamageBorderVignette();
		return;
	}

	// Health low overlay (pulsing red frame)
	if ((cg_damageDrawFrame.integer & 4) && cg.beIsHealthLow)
	{
		float damagePulse = sin((float)cg.time * 2.0f * M_PI / 2000.0f) * 0.2f + 0.4f;
		Vector4Copy(colorRed, damageColor);
		damageColor[3] = damagePulse;
		CG_OSPDrawBlurFrame(SCREEN_START, SCREEN_START, SCREEN_SCALED_W, SCREEN_SCALED_H, 128, damageColor);
	}

	// Damage frame
	if ((cg_damageDrawFrame.integer & 3) != 0 && cg.damageValue &&
	        cg.time - cg.damageTime > 0 && cg.time - cg.damageTime < DAMAGE_TIME)
	{
		Vector4Set(damageRed, 1.0f, 0.0f, 0.0f, cg_damageFrameOpaque.value);
		if ((cg_damageDrawFrame.integer & 3) == 3)
			CG_OSPDrawBlurFrame(SCREEN_START, SCREEN_START, SCREEN_SCALED_W, SCREEN_SCALED_H, cg_damageFrameSize.value, damageRed);
		else if (cg_damageDrawFrame.integer & 2)
		{
			Vector4Set(borderSizeOriginal, cg_damageFrameSize.value, cg_damageFrameSize.value, cg_damageFrameSize.value, cg_damageFrameSize.value);
			CG_OSPDrawFrame(SCREEN_START, SCREEN_START, SCREEN_SCALED_W, SCREEN_SCALED_H, borderSizeOriginal, damageRed, qtrue);
		}
		else if (cg_damageDrawFrame.integer & 1)
			CG_OSPDrawBlurFrame(SCREEN_START, SCREEN_START, SCREEN_SCALED_W, SCREEN_SCALED_H, cg_damageFrameSize.value, damageRed);
	}
}

/*
=================
CG_DrawTestFont

On-screen glyph grid for /showfont <name|index>. Uses CG_OSPDrawStringNew
so both OSP bitmap fonts and engine TTF fonts preview correctly.
=================
*/
static void CG_DrawTestFont(const char* font)
{
	const int offs_x = (SCREEN_WIDTH - 320) / 2;
	const int offs_y = (SCREEN_HEIGHT - 320) / 2;
	const int flags = DS_HCENTER | DS_PROPORTIONAL;
	int fontIndex;
	int x, y;
	char to_print[2];
	unsigned char c;

	fontIndex = CG_FontIndexFromName(font);
	c = 0;
	to_print[1] = 0;

	CG_OSPDrawStringNew(
	    SCREEN_WIDTH / 2.0f, offs_y - 24.0f,
	    va("^7%s^7  (^3%d^7)", font, fontIndex),
	    colorWhite, colorBlack,
	    10.0f, 12.0f,
	    SCREEN_WIDTH,
	    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
	    fontIndex,
	    NULL, NULL, NULL);

	for (y = 0; y < 16; ++y)
	{
		for (x = 0; x < 16; ++x)
		{
			to_print[0] = (char)c;
			c++;
			if (to_print[0] == 0)
			{
				to_print[0] = ' ';
			}
			CG_OSPDrawStringNew(
			    (float)(offs_x + x * 20), (float)(offs_y + y * 20),
			    to_print,
			    colorWhite, NULL,
			    16.0f, 16.0f,
			    64,
			    flags,
			    fontIndex,
			    NULL, NULL, NULL);
		}
	}
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(void)
{
	if (cg.levelShot)
		return;

	if (!cg_draw2D.integer)
		return;

	if (cgs.osp.testFont[0])
	{
		CG_DrawTestFont(cgs.osp.testFont);
		return;
	}

	CG_DrawDamageFrame();

	// Team task wheel marker: always update (selection logic); draw cursor only when bit 4 set
	if (cg.ttwheelMarkActive)
	{
		CG_UpdateAndDrawSelwheelMark();
	}

	// Team task marks
	if (cg.teamMarkers.drawTeamTasks)
	{
		CG_DrawTeamTaskMarks();
	}

	if (cg_worldFlags.integer & 1)
	{
		CG_WorldFlags_DrawWorld();
	}

	// Team indicator
	if (CG_BE_FEATURE_ENABLED(CG_BE_TEAM_INDICATOR) && cg_teamIndicator.integer)
		CG_DrawPlayerIndicatorOnScreen();

	if (cg.beShowCredits)
	{
		CG_OSPDrawNewCredits();
		return;
	}
	if (cg_shud.integer)
	{
		CG_SHUDRoutine();
		CG_DrawWarmupShud();
		return;
	}
	if (cg_enableOSPHUD.integer)
	{
		CG_OSPHUDRoutine();
		return;
	}
	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_OSPDrawIntermission();
		CG_DrawVote();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		CG_DrawCrosshair();
		CG_DrawCrosshairNames();
	}
	else
	{
		if (!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0)
		{
			CG_DrawStatusBar();
			CG_DrawAmmoWarning();
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();
			CG_DrawHoldableItem();
			CG_DrawReward();
		}
		if (cgs.gametype >= GT_TEAM)
			CG_DrawTeamInfo();
	}

	CG_DrawVote();
	if (cg_lagometer.integer)
		CG_DrawLagometer(428 - (cg_drawPing.integer > 0 ? 14 : 0));
	CG_DrawUpperRight();
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
	if (CG_DrawFollow() == qfalse)
		CG_DrawWarmup();
	cg.scoreBoardShowing = CG_DrawIntermission();
	if (cg.scoreBoardShowing == qfalse)
		CG_OSPDrawCenterString();
}


/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView)
{
	float       separation;
	vec3_t      baseOrg = {0, 0, 0};

	// optionally draw the info screen instead
	if (!cg.snap)
	{
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
	        (cg.snap->ps.pm_flags & PMF_SCOREBOARD))
	{
		CG_DrawOldTourneyScoreboard();
		return;
	}

	/* STEREO_CENTER first (common case): avoid cvar read and switch */
	if (stereoView == STEREO_CENTER)
	{
		separation = 0;
	}
	else if (stereoView == STEREO_LEFT)
	{
		separation = -cg_stereoSeparation.value * 0.5f;
	}
	else if (stereoView == STEREO_RIGHT)
	{
		separation = cg_stereoSeparation.value * 0.5f;
	}
	else
	{
		separation = 0;
		CG_Error("CG_DrawActive: Undefined stereoView: %d", stereoView);
	}

	/* Clear around sized-down viewport only */
	if (cg.refdef.x != 0 || cg.refdef.y != 0 ||
	        cg.refdef.width != cgs.glconfig.vidWidth || cg.refdef.height != cgs.glconfig.vidHeight)
	{
		CG_TileClear();
	}

	/* offset vieworg appropriately if we're doing stereo separation */
	if (separation != 0)
	{
		VectorCopy(cg.refdef.vieworg, baseOrg);
		VectorMA(cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg);
	}

	/* Pre-scene refEntities: border markers + selwheel pointers + overhead 3D sprites (single guard, flat flow) */
	{
		qboolean doSelwheel = (cg_teamTaskWheel.integer & 8) && cg.ttwheelMarkActive;
		if (cg.teamMarkers.drawTeamTasks)
		{
			CG_DrawTeamTaskBorderHudMarkers();
			CG_AddTeamTaskOverheadSprites();
		}
		if (cg_worldFlags.integer & 2)
		{
			CG_WorldFlags_DrawBorderHudMarkers();
		}
		if (cg.teamTinfoLocations)
		{
			CG_AddGlobalPosFriendFloatSprites();
			CG_DrawGlobalPosFriendHudMarkers();
		}
		if (doSelwheel)
			CG_DrawSelwheelPointers();
	}

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	// restore original viewpoint if running stereo
	if (separation != 0)
	{
		VectorCopy(baseOrg, cg.refdef.vieworg);
	}

	/* Unified 2D HUD (single entry, cached cvars, flat branches) */
	CG_Draw2D();
	CG_OSPDrawEmojiListOverlay();
	CG_BEChat_Draw();

}

/*
=================
CG_DrawRotatedPic

Draw a rotated 2D image using sprite in 3D scene
Parameters:
  x, y - center position in screen coordinates (640x480 virtual)
  w, h - width and height
  shader - shader handle
  angle - rotation angle in degrees (clockwise)
  color - RGBA color (0.0 to 1.0)

Note: This function adds sprite to the main 3D scene, so it must be called
      BEFORE trap_R_RenderScene(&cg.refdef)
=================
*/
void CG_DrawRotatedPic(float x, float y, float w, float h, qhandle_t shader, float angle, const vec4_t color)
{
	refEntity_t ent;
	float dist;
	vec3_t centerPos;
	float hfov_x;
	float hfov_y;
	float radius;
	float dx;
	float dy;
	float offsetX;
	float offsetY;
	vec3_t rightOffset;
	vec3_t upOffset;
	int r, g, b, a;

	if (!shader)
	{
		return;
	}

	// Initialize entity
	memset(&ent, 0, sizeof(ent));

	// Convert color to byte values
	r = (int)(color[0] * 255.0f);
	g = (int)(color[1] * 255.0f);
	b = (int)(color[2] * 255.0f);
	a = (int)(color[3] * 255.0f);

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (a > 255) a = 255;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	if (a < 0) a = 0;

	// Calculate FOV
	hfov_x = cg.refdef.fov_x * M_PI / 360.0f;
	hfov_y = cg.refdef.fov_y * M_PI / 360.0f;

	// Calculate position based on screen coordinates
	// Convert screen position to 3D position in front of camera
	dist = 64.0f; // Distance in front of camera

	// Offset from center of screen (in 640x480 coordinates)
	dx = x - (float)SCREEN_WIDTH * 0.5f;
	dy = y - (float)SCREEN_HEIGHT * 0.5f;

	// Convert screen offset to 3D offset on plane in front of camera
	offsetX = dx * tan(hfov_x) / (SCREEN_WIDTH * 0.5f) * dist;
	offsetY = -dy * tan(hfov_y) / (SCREEN_HEIGHT * 0.5f) * dist; // Negative because screen Y increases downward

	// Start from center position in front of camera
	VectorScale(cg.refdef.viewaxis[0], dist, centerPos);

	// Add right offset (viewaxis[1] is right vector)
	VectorScale(cg.refdef.viewaxis[1], offsetX, rightOffset);

	// Add up offset (viewaxis[2] is up vector)
	VectorScale(cg.refdef.viewaxis[2], offsetY, upOffset);

	// Combine all offsets
	VectorAdd(cg.refdef.vieworg, centerPos, ent.origin);
	VectorAdd(ent.origin, rightOffset, ent.origin);
	VectorAdd(ent.origin, upOffset, ent.origin);

	// Calculate size based on screen size and FOV
	// Convert screen width to 3D radius
	radius = w * 0.5f * tan(hfov_x) / (SCREEN_WIDTH * 0.5f) * dist;
	ent.radius = radius;

	if (ent.radius < 0.1f)
	{
		ent.radius = 0.1f;
	}

	// Set sprite entity properties
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON | RF_DEPTHHACK;
	ent.rotation = angle; // Rotation in degrees (clockwise)
	ent.customShader = shader;
	ent.shaderRGBA[0] = r;
	ent.shaderRGBA[1] = g;
	ent.shaderRGBA[2] = b;
	ent.shaderRGBA[3] = a;

	// Add to main 3D scene (must be called before trap_R_RenderScene)
	trap_R_AddRefEntityToScene(&ent);
}

void CG_OSPSetColor(vec4_t color)
{
	trap_R_SetColor(color);
	return;
}

void CG_OSPDrawPic(float x, float y, float w, float h, qhandle_t hShader)
{
	CG_AdjustFrom640_Old(&x, &y, &w, &h, qfalse);
	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1.0f, 1.0f, hShader);
	return;
}

void CG_OSPDraw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t pos, vec3_t angles, vec3_t angles2)
{
	refdef_t rd;
	refEntity_t re;

	if (!cg_draw3dIcons.integer || !cg_drawIcons.integer)
	{
		return;
	}

	CG_AdjustFrom640_Old(&x, &y, &w, &h, qfalse);
	memset(&rd, 0, sizeof(rd));
	memset(&re, 0, sizeof(re));
	AnglesToAxis(angles, re.axis);

	memcpy(re.origin, pos, sizeof(vec3_t));
	re.hModel = model;
	re.customSkin = skin;
	re.renderfx = RF_NOSHADOW;     // no stencil shadows
	rd.rdflags = RDF_NOWORLDMODEL;

	AxisClear(rd.viewaxis);
	rd.fov_x = 30.0f;
	rd.fov_y = 30.0f;
	rd.x = x;
	rd.y = y;
	rd.width = w;
	rd.height = h;
	rd.time = cg.time;
	if (angles2)
	{
		float tmp;

		tmp = 255.0f * angles2[0];
		if (tmp >= (float)INT_MAX)
		{
			re.shaderRGBA[0] = -1 * (int)(tmp - (float)INT_MAX) ;
		}
		else
		{
			re.shaderRGBA[0] = (int)tmp;
		}

		tmp = 255.0f * angles2[1];
		if (tmp >= (float)INT_MAX)
		{
			re.shaderRGBA[1] = -1 * (int)(tmp - (float)INT_MAX);
		}
		else
		{
			re.shaderRGBA[1] = (int)tmp;
		}

		tmp = 255.0f * angles2[2];
		if (tmp >= (float)INT_MAX)
		{
			re.shaderRGBA[2] = -1 * (int)(tmp - (float)INT_MAX);
		}
		else
		{
			re.shaderRGBA[2] = (int)tmp;
		}
	}

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&re);
	trap_R_RenderScene(&rd);
}

void CG_OSPDrawCenterString(void)
{
	float* fadeColor;
	char* to_print;
	int y;
	int font = cgFontIndices.centerMessages;
	if (!cg_drawCenterMessages.integer)
	{
		return;
	}
	if (!cg.centerPrintTime) return;
	fadeColor = CG_FadeColor(cg.centerPrintTime, 1000.0 * cg_centertime.value);


	if (!fadeColor) return;

	trap_R_SetColor(fadeColor);

	y = cg.centerPrintY - (cg.centerPrintLines * 8);
	for (to_print = cg.centerPrint; to_print < (to_print + sizeof(cg.centerPrint)) && *to_print != 0;)
	{
		int i;
		int space;
		char buf[48];
		int flags = (cg_drawCenterMessages.integer & 2) ? DS_SHADOW : 0;
		space = 0;
		i = 0;
		do
		{
			if (to_print[i] == ' ')
			{
				space = i;
			}
			if (!to_print[i] || to_print[i] == '\n')
			{
				space = i;
				break;
			}
			buf[i]  = to_print[i];
		}
		while (++i < 40);

		if (space && space != i)
		{
			buf[space] = 0;
			to_print += space + 1;
		}
		else
		{
			buf[i] = 0;
			to_print += i + 1;
		}

		CG_OSPDrawStringNew(SCREEN_WIDTH / 2.0f,
		                    y,
		                    buf,
		                    fadeColor,
		                    colorBlack,
		                    cg.centerPrintCharWidth,
		                    1.5f * (float)cg.centerPrintCharWidth,
		                    SCREEN_WIDTH,
		                    DS_HCENTER | DS_PROPORTIONAL | flags,
		                    font,
		                    NULL, NULL, NULL);

		y += 1.5f * (float)cg.centerPrintCharWidth;
	}
	trap_R_SetColor(NULL);
}
