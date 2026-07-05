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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "../game/bg_local.h"

static const vec3_t weaponBrightColors[WP_NUM_WEAPONS] =
{
	{0, 0, 0},  // WP_NONE (unused)
	{1.0f, 1.0f, 1.0f},      // WP_GAUNTLET      (white)
	{1.0f, 1.0f, 0.0f},        // WP_MACHINEGUN    (yellow)
	{1.0f, 0.5f, 0.0f},    // WP_SHOTGUN       (orange)
	{0.0f, 1.0f, 0.0f},   // WP_GRENADE_LAUNCHER (green)
	{1.0f, 0.0f, 0.0f},      // WP_ROCKET_LAUNCHER (red)
	{1.0f, 1.0f, 1.0f},        // WP_LIGHTNING     (white)
	{0.0f, 1.0f, 1.0f},      // WP_RAILGUN       (cyan)
	{1.0f, 0.0f, 1.0f},    // WP_PLASMAGUN     (magenta)
	{0.0f, 0.0f, 1.0f},    // WP_BFG           (blue)
};

/* Double-shoot filter state (inlined from CG_IsDoubleShoot) */
static int cg_doubleShootTime;
static int cg_doubleShootLastTime;
static int cg_doubleShootAddTime[WP_NUM_WEAPONS];
static qboolean cg_doubleShootAddTimeInited = qfalse;

/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass(centity_t* cent)
{
	localEntity_t*   le;
	refEntity_t*     re;
	vec3_t          velocity, xvelocity;
	vec3_t          offset, xoffset;
	float           waterScale = 1.0f;
	vec3_t          v[3];

	if (cg_brassTime.integer <= 0)
	{
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & CONTENTS_WATER)
	{
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE | LEF_PLAYER_BRASS | ((cent->currentState.number & 0xFFFF) << 16);
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass(centity_t* cent)
{
	localEntity_t*   le;
	refEntity_t*     re;
	vec3_t          velocity, xvelocity;
	vec3_t          offset, xoffset;
	vec3_t          v[3];
	int             i;

	if (cg_brassTime.integer <= 0)
	{
		return;
	}

	for (i = 0; i < 2; i++)
	{
		float   waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if (i == 0)
		{
			velocity[1] = 40 + 10 * crandom();
		}
		else
		{
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer * 3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis(cent->lerpAngles, v);

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd(cent->lerpOrigin, xoffset, re->origin);
		VectorCopy(re->origin, le->pos.trBase);
		if (CG_PointContents(re->origin, -1) & CONTENTS_WATER)
		{
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale(xvelocity, waterScale, le->pos.trDelta);

		AxisCopy(axisDefault, re->axis);
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE | LEF_PLAYER_BRASS | ((cent->currentState.number & 0xFFFF) << 16);
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}


/*
==========================
CG_RailSpiral - spiral rings (altRail 4,5)
==========================
*/
static void CG_RailSpiral(clientInfo_t* ci, vec3_t start, vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp;
	float  len;
	int    i, j, skip, spacingVal, rotationStep;
	localEntity_t* le;
	refEntity_t*   re;

	if (cg_railRingsSize.value <= 0.0f)
		return; /* no ring sprites when size 0 */
	spacingVal = cg_railRingsSpacing.integer > 0 ? cg_railRingsSpacing.integer : 5;
	rotationStep = cg_railRingsRotation.integer;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	PerpendicularVector(temp, vec);
	for (i = 0; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);

	VectorMA(move, 20, vec, move);
	VectorScale(vec, (float)spacingVal, vec);
	skip = -1;
	j = 18;
	for (i = 0; i < len; i += spacingVal)
	{
		if (i != skip)
		{
			skip = i + spacingVal;
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
			le->startTime = cg.time;
			le->endTime = cg.time + (i >> 1) + 600;
			le->lifeRate = 1.0f / (le->endTime - le->startTime);
			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_SPRITE;
			re->radius = cg_railRingsSize.value;
			re->customShader = (cg_nomip.integer & 0x20) ? cgs.media.railRingsShaderNoPicMip : cgs.media.railRingsShader;
			re->shaderRGBA[0] = (byte)(ci->colors.railRings[0] * 255);
			re->shaderRGBA[1] = (byte)(ci->colors.railRings[1] * 255);
			re->shaderRGBA[2] = (byte)(ci->colors.railRings[2] * 255);
			re->shaderRGBA[3] = 255;
			le->color[0] = ci->colors.railRings[0] * 0.75f;
			le->color[1] = ci->colors.railRings[1] * 0.75f;
			le->color[2] = ci->colors.railRings[2] * 0.75f;
			le->color[3] = 1.0f;
			le->pos.trType = TR_LINEAR;
			le->pos.trTime = cg.time;
			if (rotationStep)
			{
				VectorCopy(move, move2);
				VectorMA(move2, cg_railRingsRadius.value, axis[j], move2);
				VectorCopy(move2, le->pos.trBase);
				le->pos.trDelta[0] = axis[j][0] * 4;
				le->pos.trDelta[1] = axis[j][1] * 4;
				le->pos.trDelta[2] = axis[j][2] * 4;
			}
			else
			{
				/* rotation 0 = rings on the beam (like cg_altRail 0), no offset */
				VectorCopy(move, le->pos.trBase);
				le->pos.trDelta[0] = 0;
				le->pos.trDelta[1] = 0;
				le->pos.trDelta[2] = 0;
			}
		}
		VectorAdd(move, vec, move);
		if (rotationStep)
			j = (j + rotationStep + 36) % 36;
	}
}

/*
==========================
CG_RailSpiral2 - lightning segments (altRail 2)
==========================
*/
static void CG_RailSpiral2(clientInfo_t* ci, vec3_t start, vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp, last;
	trajectory_t lastTraj;
	float  len;
	int    i, j, skip, k;
	localEntity_t* le;
	refEntity_t*   re;
#define RAIL2_ROTATION 1
#define RAIL2_SPACING  5

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	PerpendicularVector(temp, vec);
	for (i = 0; i < 36; i++)
		RotatePointAroundVector(axis[i], vec, temp, i * 10);
	VectorMA(move, 20, vec, move);
	VectorScale(vec, RAIL2_SPACING, vec);
	skip = -1;
	j = 18;
	VectorCopy(start, last);
	memset(&lastTraj, 0, sizeof(lastTraj));
	k = 0;
	for (i = 0; i < len; i += RAIL2_SPACING)
	{
		if (i != skip)
		{
			trajectory_t traj;
			skip = i + RAIL2_SPACING;
			VectorCopy(move, move2);
			VectorMA(move2, cg_railRingsRadius.value, axis[j], move2);
			traj.trType = TR_LINEAR;
			traj.trTime = cg.time;
			VectorCopy(move2, traj.trBase);
			traj.trDelta[0] = axis[j][0] * 6;
			traj.trDelta[1] = axis[j][1] * 6;
			traj.trDelta[2] = axis[j][2] * 6;
			if (i <= RAIL2_SPACING)
			{
				VectorCopy(move2, last);
				memcpy(&lastTraj, &traj, sizeof(lastTraj));
				goto next2;
			}
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leType = LE_FADE_RGB;
			le->startTime = cg.time;
			le->endTime = cg.time + 800;
			le->lifeRate = 1.0f / (le->endTime - le->startTime);
			re->shaderTime = cg.time / 1000.0f;
			re->reType = cg_altRailBeefy.integer ? RT_LIGHTNING : RT_RAIL_CORE;
			re->customShader = cgs.media.ratRailSpiralShaders[(k++) % NUM_RAILSPIRALSHADERS];
			re->shaderRGBA[0] = (byte)(ci->colors.railRings[0] * 255);
			re->shaderRGBA[1] = (byte)(ci->colors.railRings[1] * 255);
			re->shaderRGBA[2] = (byte)(ci->colors.railRings[2] * 255);
			re->shaderRGBA[3] = 255;
			le->color[0] = ci->colors.railRings[0] * 0.75f;
			le->color[1] = ci->colors.railRings[1] * 0.75f;
			le->color[2] = ci->colors.railRings[2] * 0.75f;
			le->color[3] = 1.0f;
			AxisClear(re->axis);
			if (cg_altRailBeefy.integer)
			{
				VectorCopy(move2, re->oldorigin);
				VectorCopy(last, re->origin);
				memcpy(&le->pos2, &traj, sizeof(le->pos2));
				memcpy(&le->pos, &lastTraj, sizeof(le->pos));
			}
			else
			{
				VectorCopy(move2, re->origin);
				VectorCopy(last, re->oldorigin);
				memcpy(&le->pos, &traj, sizeof(le->pos));
				memcpy(&le->pos2, &lastTraj, sizeof(le->pos2));
			}
			VectorCopy(move2, last);
			memcpy(&lastTraj, &traj, sizeof(lastTraj));
			le->leFlags = LEF_PUFF_DONT_SCALE | LEF_FADE_RGB | LEF_MOVE_OLDORIGIN;
			le->leType = LE_MOVE_SCALE_FADE;
		}
next2:
		VectorAdd(move, vec, move);
		j = (j + RAIL2_ROTATION) % 36;
	}
}

#define RAIL3_SPIRALLENGTH 180
#define RAIL3_NUMROT 6
/*
==========================
CG_RailSpiral3 - tubes (altRail 3)
==========================
*/
static void CG_RailSpiral3(clientInfo_t* ci, vec3_t start, vec3_t end)
{
	vec3_t rotaxis[RAIL3_NUMROT], rotaxis2[RAIL3_NUMROT], second, direction, s;
	localEntity_t* le;
	refEntity_t*   re;
	float l, len, remaining, tubelength;
	int i, numTubes;

	VectorSubtract(end, start, direction);
	len = VectorNormalize(direction);
	PerpendicularVector(second, direction);
	for (i = 0; i < RAIL3_NUMROT; i++)
	{
		RotatePointAroundVector(rotaxis[i], direction, second, (float)(i * 360) / RAIL3_NUMROT);
		CrossProduct(direction, rotaxis[i], rotaxis2[i]);
		VectorScale(rotaxis[i], 10, rotaxis[i]);
		VectorScale(rotaxis2[i], 10, rotaxis2[i]);
	}
	numTubes = (int)(len / RAIL3_SPIRALLENGTH);
	if (numTubes < 1)
		numTubes = 1;
	tubelength = len / (float)numTubes;
	VectorScale(direction, tubelength, direction);
	VectorCopy(start, s);
	for (l = 0, i = 0; l < len; l += tubelength, i++)
	{
		remaining = (len - l) / (float)RAIL3_SPIRALLENGTH;
		if (remaining < 0.1f)
			return;
		le = CG_AllocLocalEntity();
		re = &le->refEntity;
		le->leType = LE_RAILTUBE;
		le->startTime = cg.time;
		le->endTime = cg.time + 800;
		le->lifeRate = 1.0f / (le->endTime - le->startTime);
		re->shaderTime = cg.time / 1000.0f;
		re->shaderRGBA[0] = (byte)(ci->colors.railRings[0] * 255);
		re->shaderRGBA[1] = (byte)(ci->colors.railRings[1] * 255);
		re->shaderRGBA[2] = (byte)(ci->colors.railRings[2] * 255);
		re->shaderRGBA[3] = 255;
		le->color[0] = ci->colors.railRings[0] * 0.75f;
		le->color[1] = ci->colors.railRings[1] * 0.75f;
		le->color[2] = ci->colors.railRings[2] * 0.75f;
		le->color[3] = 1.0f;
		re->nonNormalizedAxes = qtrue;
		VectorCopy(direction, re->axis[0]);
		VectorCopy(rotaxis[i % RAIL3_NUMROT], re->axis[1]);
		VectorCopy(rotaxis2[i % RAIL3_NUMROT], re->axis[2]);
		re->customShader = remaining < 0.5f ? cgs.media.ratRailTubeShader50 : cgs.media.ratRailTubeShader100;
		re->reType = RT_MODEL;
		re->rotation = 0;
		re->hModel = cgs.media.ratRailSpiralModel;
		re->renderfx = RF_NOSHADOW;
		VectorCopy(s, re->origin);
		VectorAdd(s, direction, s);
	}
}

/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail(clientInfo_t* ci, vec3_t start, vec3_t end)
{
	vec3_t axis[36], move, move2, vec, temp;
	float  len;
	int    i, j, skip;
	localEntity_t* le;
	refEntity_t*   re;

	if (cg_altRail.integer == 1)
	{
		CG_RailTrail2(ci, start, end);
		return;
	}
	if (cg_altRail.integer >= 2)
	{
		float railTrailTime = (cg_railTrailTime.value < 300) ? 300 : cg_railTrailTime.value;
		start[2] -= 4;
		/* cg_railRingsRadius 0 = only beam (core), no spiral/rings */
		if (cg_railRingsRadius.value > 0.0f)
		{
			switch (cg_altRail.integer)
			{
				case 2:
					CG_RailSpiral2(ci, start, end);
					break;
				case 3:
					CG_RailSpiral3(ci, start, end);
					break;
				default:
					CG_RailSpiral(ci, start, end);
					break;
			}
		}
		le = CG_AllocLocalEntity();
		re = &le->refEntity;
		le->leType = LE_FADE_RGB;
		le->startTime = cg.time;
		le->endTime = cg.time + (int)railTrailTime;
		le->lifeRate = 1.0f / (le->endTime - le->startTime);
		re->shaderTime = cg.time / 1000.0f;
		re->reType = cg_altRailBeefy.integer ? RT_LIGHTNING : RT_RAIL_CORE;
		re->customShader = (cg_altRail.integer == 4 && cgs.media.ratRailCoreShaderFat) ? cgs.media.ratRailCoreShaderFat : cgs.media.ratRailCoreShader;
		VectorCopy(start, re->origin);
		VectorCopy(end, re->oldorigin);
		re->shaderRGBA[0] = (byte)(ci->colors.railCore[0] * 255);
		re->shaderRGBA[1] = (byte)(ci->colors.railCore[1] * 255);
		re->shaderRGBA[2] = (byte)(ci->colors.railCore[2] * 255);
		re->shaderRGBA[3] = 255;
		le->color[0] = ci->colors.railCore[0] * 0.75f;
		le->color[1] = ci->colors.railCore[1] * 0.75f;
		le->color[2] = ci->colors.railCore[2] * 0.75f;
		le->color[3] = 1.0f;
		AxisClear(re->axis);
		/* overlay for altRail 2-5 */
		le = CG_AllocLocalEntity();
		re = &le->refEntity;
		le->leType = LE_FADE_RGB_SIN;
		le->startTime = cg.time;
		le->endTime = cg.time + (int)((railTrailTime < 1200 ? railTrailTime : 1200) / 2.0f);
		le->lifeRate = 1.0f / (le->endTime - le->startTime);
		re->shaderTime = cg.time / 1000.0f;
		re->reType = cg_altRailBeefy.integer ? RT_LIGHTNING : RT_RAIL_CORE;
		re->customShader = (cg_altRail.integer == 4 && cgs.media.ratRailCoreShaderOverlayFat) ? cgs.media.ratRailCoreShaderOverlayFat : cgs.media.ratRailCoreShaderOverlay;
		VectorCopy(start, re->origin);
		VectorCopy(end, re->oldorigin);
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 255;
		le->color[0] = le->color[1] = le->color[2] = 1.0f;
		le->color[3] = 1.0f;
		AxisClear(re->axis);
		return;
	}
	// Default rail trail (cg_altRail.integer == 0)



	start[2] -= 4;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	PerpendicularVector(temp, vec);
	for (i = 0 ; i < 36; i++)
	{
		RotatePointAroundVector(axis[i], vec, temp, i * 10);//banshee 2.4 was 10
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	if (cg_nomip.integer & 0x20)
	{
		re->customShader = cgs.media.railCoreShaderNoPicMip;
	}
	else
	{
		re->customShader = cgs.media.railCoreShader;
	}

	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);

	/* cg_teamRails */

	if (cg_teamRails.integer && cgs.gametype >= GT_TEAM)
	{
		float* tc;

		if (ci->team == TEAM_RED)
		{
			tc = cgs.beRedTeamColor;
			re->shaderRGBA[0] = tc[0] * 255;
			re->shaderRGBA[1] = tc[1] * 255;
			re->shaderRGBA[2] = tc[2] * 255;

			le->color[0] = tc[0] * 0.75;
			le->color[1] = tc[1] * 0.75;
			le->color[2] = tc[2] * 0.75;
		}
		else if (ci->team == TEAM_BLUE)
		{
			tc = cgs.beBlueTeamColor;
			re->shaderRGBA[0] = tc[0] * 255;
			re->shaderRGBA[1] = tc[1] * 255;
			re->shaderRGBA[2] = tc[2] * 255;

			le->color[0] = tc[0] * 0.75;
			le->color[1] = tc[1] * 0.75;
			le->color[2] = tc[2] * 0.75;
		}
	}
	else
	{
		re->shaderRGBA[0] = ci->colors.railCore[0] * 255;
		re->shaderRGBA[1] = ci->colors.railCore[1] * 255;
		re->shaderRGBA[2] = ci->colors.railCore[2] * 255;

		le->color[0] = ci->colors.railCore[0] * 0.75;
		le->color[1] = ci->colors.railCore[1] * 0.75;
		le->color[2] = ci->colors.railCore[2] * 0.75;
	}

	re->shaderRGBA[3] = 255;
	le->color[3] = 1.0f;

	AxisClear(re->axis);

	VectorMA(move, 20, vec, move);
	VectorScale(vec, cg_railRingsSpacing.integer, vec);

	if (cg_oldRail.integer != 0)
	{
		if (cg_railRingsSize.value > 0.0f)
		{
			le = CG_AllocLocalEntity();
			re = &le->refEntity;
			le->leType = LE_FADE_RGB;
			le->startTime = cg.time;
			le->endTime = cg.time + cg_railTrailTime.value;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_RAIL_RINGS;
			re->radius = cg_railRingsSize.value;
			if (cg_nomip.integer & 0x20)
			{
				re->customShader = cgs.media.railRingsShaderNoPicMip;
			}
			else
			{
				re->customShader = cgs.media.railRingsShader;
			}
			VectorCopy(start, re->origin);
			VectorCopy(end, re->oldorigin);

			le->color[0] = ci->colors.railRings[0] * 0.75;
			le->color[1] = ci->colors.railRings[1] * 0.75;
			le->color[2] = ci->colors.railRings[2] * 0.75;
			le->color[3] = 1.0f;

			AxisClear(re->axis);
		}
		return;
	}
	if (cg_railRingsSize.value <= 0.0f)
		goto rail_trail_done_rings; /* no ring sprites when size 0 */
	skip = -1;

	j = 18;
	{
		/* Precompute common settings for ring segment loop */
		qhandle_t ringsShader = (cg_nomip.integer & 0x20) ? cgs.media.railRingsShaderNoPicMip : cgs.media.railRingsShader;
		float ringsRadius = cg_railRingsSize.value;
		float ringsColorR = ci->colors.railRings[0];
		float ringsColorG = ci->colors.railRings[1];
		float ringsColorB = ci->colors.railRings[2];
		byte shaderR = ringsColorR * 255;
		byte shaderG = ringsColorG * 255;
		byte shaderB = ringsColorB * 255;
		float leColorR = ringsColorR * 0.75f;
		float leColorG = ringsColorG * 0.75f;
		float leColorB = ringsColorB * 0.75f;
		float shaderTime = cg.time / 1000.0f;
		qboolean staticRings = cg_railStaticRings.integer;
		int rotationStep = cg_railRingsRotation.integer;
		float ringsRadiusVal = cg_railRingsRadius.value;

		for (i = 0; i < len; i += cg_railRingsSpacing.integer)
		{
			if (i != skip)
			{
				skip = i + cg_railRingsSpacing.integer;
				le = CG_AllocLocalEntity();
				re = &le->refEntity;
				le->leFlags = LEF_PUFF_DONT_SCALE;
				le->leType = LE_MOVE_SCALE_FADE;
				le->startTime = cg.time;
				le->endTime = cg.time + (i >> 1) + 600;
				le->lifeRate = 1.0 / (le->endTime - le->startTime);

				re->shaderTime = shaderTime;
				re->reType = RT_SPRITE;
				re->radius = ringsRadius;
				re->customShader = ringsShader;

				re->shaderRGBA[0] = shaderR;
				re->shaderRGBA[1] = shaderG;
				re->shaderRGBA[2] = shaderB;
				re->shaderRGBA[3] = 255;

				le->color[0] = leColorR;
				le->color[1] = leColorG;
				le->color[2] = leColorB;
				le->color[3] = 1.0f;
				le->pos.trType = staticRings ? TR_STATIONARY : TR_LINEAR;
				le->pos.trTime = cg.time;

				if (!rotationStep)
					VectorCopy(move, le->pos.trBase);
				else
				{
					VectorCopy(move, move2);
					VectorMA(move2, ringsRadiusVal, axis[j], move2);
					VectorCopy(move2, le->pos.trBase);
				}

				le->pos.trDelta[0] = axis[j][0] * 6;
				le->pos.trDelta[1] = axis[j][1] * 6;
				le->pos.trDelta[2] = axis[j][2] * 6;
			}

			VectorAdd(move, vec, move);

			if (rotationStep)
				j = j + rotationStep < 36 ? j + rotationStep : (j + rotationStep) % 36;
		}
	}
rail_trail_done_rings:
	;
}

/*
==========================
CG_RailTrail2 - WICKED!

Alternate Fire trail

FIXME: Kinda buggy on space maps(when you shoot into space & players) the rail trail disappears instantly.
==========================
*/
void CG_RailTrail2(clientInfo_t* ci, vec3_t start, vec3_t end)
{
	localEntity_t* le;
	refEntity_t* re;
	vec3_t adjustedStart, adjustedEnd;

	// Apply same offset as standard rail
	VectorCopy(start, adjustedStart);
	VectorCopy(end, adjustedEnd);
	adjustedStart[2] -= 4;

	// core beam
	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(adjustedStart, re->origin);
	VectorCopy(adjustedEnd, re->oldorigin);

	le->color[0] = ci->colors.railCore[0] * 0.75;
	le->color[1] = ci->colors.railCore[1] * 0.75;
	le->color[2] = ci->colors.railCore[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear(re->axis);

	// spiral effect entity - uses railRings color for proper enemy/team/self detection
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	//le->generic1 = ci->team;
	le->leType = LE_RAIL_EFFECT;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;

	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(adjustedStart, re->origin);
	VectorCopy(adjustedEnd, re->oldorigin);

	// Use railRings color for spiral particles (like standard rail rings)
	le->color[0] = ci->colors.railRings[0] * 0.75;
	le->color[1] = ci->colors.railRings[1] * 0.75;
	le->color[2] = ci->colors.railRings[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear(re->axis);
}

#define RAIL_EFFECT_MAX_SEGMENTS    64
#define RAIL_EFFECT_BASE_SPACING    16

/*
====================
CG_AddRailEffect - CG_RailTrail2 effect(AF trail)
====================
*/
void CG_AddRailEffect(localEntity_t* le)
{
	refEntity_t* re;
	refEntity_t particle;
	vec3_t forward, right, up, posx, posy, pos, oldpos, move, vec;
	float c, len, radius, theta, sn, cs, spacing, fade, invLen;
	int i, r, g, b, step, maxStep;

	if (cg_railRingsRadius.value <= 0.0f)
	{
		return;
	}

	re = &le->refEntity;
	c = (le->endTime - cg.time) * le->lifeRate;
	if (c <= 0.0f)
	{
		return;
	}
	c *= 0xff;
	fade = c / 0xff;

	VectorCopy(re->origin, move);
	VectorSubtract(re->oldorigin, re->origin, vec);

	VectorCopy(vec, forward);
	VectorNormalize(forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	len = VectorNormalize(vec);
	if (len <= 0.0f)
	{
		return;
	}

	spacing = (float)RAIL_EFFECT_BASE_SPACING;
	maxStep = (int)(len / spacing);
	if (maxStep > RAIL_EFFECT_MAX_SEGMENTS)
	{
		spacing = len / (float)RAIL_EFFECT_MAX_SEGMENTS;
		maxStep = RAIL_EFFECT_MAX_SEGMENTS;
	}
	VectorScale(vec, spacing, vec);
	invLen = 1.0f / len;

	r = (int)(le->color[0] / 0.75f * 255.0f * fade);
	g = (int)(le->color[1] / 0.75f * 255.0f * fade);
	b = (int)(le->color[2] / 0.75f * 255.0f * fade);

	memset(&particle, 0, sizeof(particle));
	particle.reType = RT_RAIL_CORE;
	particle.customShader = cgs.media.railCoreShader;
	particle.shaderRGBA[0] = r;
	particle.shaderRGBA[1] = g;
	particle.shaderRGBA[2] = b;
	particle.shaderRGBA[3] = (int)(c * le->color[3]);
	AxisClear(particle.axis);

	VectorCopy(move, oldpos);

	for (step = 0; step < maxStep; step++)
	{
		i = (int)(step * spacing);
		radius = cg_railRingsRadius.value * sin((float)i * invLen * (float)M_PI) * fade;

		theta = (float)(i - cg.time % 360) * M_PI / 180 * 2;

		cs = cos(theta);
		sn = sin(theta);

		VectorScale(right, cs, posx);
		VectorScale(up, sn, posy);
		VectorAdd(posx, posy, pos);
		VectorScale(pos, radius, pos);
		VectorAdd(pos, move, pos);

		if (step > 0)
		{
			VectorCopy(pos, particle.origin);
			VectorCopy(oldpos, particle.oldorigin);
			trap_R_AddRefEntityToScene(&particle);
		}

		VectorAdd(move, vec, move);
		VectorCopy(pos, oldpos);
	}
}

/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail(centity_t* ent, const weaponInfo_t* wi)
{
	int     step = 50;
	vec3_t  origin, lastPos;
	int     t;
	int     startTime, contents;
	int     lastContents;
	entityState_t*   es;
	vec3_t  up;
	float radius = 0;
	int flags = 0;

	if (cg_noProjectileTrail.integer)
	{
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	if (wi == &cg_weapons[WP_GRENADE_LAUNCHER])
		step = cg_smokedensity_gl.integer;
	else
		step = cg_smokedensity_rl.integer;

	if (step < 5)
	{
		step = 5;
	}
	else if (step > 100)
	{
		step = 100;
	}

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if (es->pos.trType == TR_STATIONARY)
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		if (contents & lastContents & CONTENTS_WATER)
		{
			CG_BubbleTrail(lastPos, origin, 8);
		}
		return;
	}
	else if (wi == &cg_weapons[WP_GRENADE_LAUNCHER])
	{
		if (cg_smokedensity_gl.integer >= 1 && cg_smokedensity_gl.integer <= 100)
			step = cg_smokedensity_gl.integer;
		radius = (cg_smokeradius_gl.value >= 0) ? cg_smokeradius_gl.value : 64.0f;
		flags = (cg_smokegrowth_gl.integer != 0) ? 0 : LEF_PUFF_DONT_SCALE;
	}
	else
	{
		if (cg_smokedensity_rl.integer >= 1 && cg_smokedensity_rl.integer <= 100)
			step = cg_smokedensity_rl.integer;
		radius = (cg_smokeradius_rl.value >= 0) ? cg_smokeradius_rl.value : 64.0f;
		flags = (cg_smokegrowth_rl.integer != 0) ? 0 : LEF_PUFF_DONT_SCALE;
	}

	for (; t <= ent->trailTime ; t += step)
	{
		BG_EvaluateTrajectory(&es->pos, t, lastPos);

		(void)CG_SmokePuff(lastPos, up,
		                   radius,
		                   1, 1, 1, 0.33f,
		                   wi->wiTrailTime,
		                   t,
		                   0,
		                   flags,
		                   (cg_nomip.integer & 256) ? cgs.media.smokePuffNoPicMipShader : cgs.media.smokePuffShader);
	}

}

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail(centity_t* cent, const weaponInfo_t* wi)
{
	localEntity_t*   le;
	refEntity_t*     re;
	entityState_t*   es;
	vec3_t          velocity, xvelocity, origin;
	vec3_t          offset, xoffset;
	vec3_t          v[3];

	float   waterScale = 1.0f;

	if (cg_noProjectileTrail.integer || cg_oldPlasma.integer)
	{
		return;
	}

	es = &cent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime = cg.time;
	le->endTime = le->startTime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis(cent->lerpAngles, v);

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	VectorAdd(origin, xoffset, re->origin);
	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & CONTENTS_WATER)
	{
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->radius = 0.25f;

	if (cg_nomip.integer & 2)
	{
		re->customShader = cgs.media.railRingsShaderNoPicMip;
	}
	else
	{
		re->customShader = cgs.media.railRingsShader;
	}
	le->bounceFactor = 0.3f;

	re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
	re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
	re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
	re->shaderRGBA[3] = 63;

	le->color[0] = wi->flashDlightColor[0] * 0.2;
	le->color[1] = wi->flashDlightColor[1] * 0.2;
	le->color[2] = wi->flashDlightColor[2] * 0.2;
	le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;

}
/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail(centity_t* ent, const weaponInfo_t* wi)
{
	vec3_t  origin;
	entityState_t*   es;
	vec3_t          forward, up;
	refEntity_t     beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof(beam));
	//FIXME adjust for muzzle position
	VectorCopy(cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin);
	beam.origin[2] += 26;
	AngleVectors(cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up);
	VectorMA(beam.origin, -6, up, beam.origin);
	VectorCopy(origin, beam.oldorigin);

	if (Distance(beam.origin, beam.oldorigin) < 64)
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.grappleRopeShader ? cgs.media.grappleRopeShader : cgs.media.lightningShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

void CG_UpdateWeaponDlightColor(weapon_t weapon)
{
	static const vmCvar_t* weaponCvars[WP_NUM_WEAPONS] =
	{
		NULL, //WP_NONE
		&cg_dlightGauntlet, //WP_GAUNTLET
		&cg_dlightMG,//WP_MACHINEGUN
		&cg_dlightSG,//WP_SHOTGUN
		&cg_dlightGL,//WP_GRENADE_LAUNCHER
		&cg_dlightRL,//WP_ROCKET_LAUNCHER
		&cg_dlightLG,//WP_LIGHTNING
		&cg_dlightRG,//WP_RAILGUN
		&cg_dlightPG,//WP_PLASMAGUN
		&cg_dlightBFG,//WP_BFG
		NULL,//WP_GRAPPLING_HOOK
	};
	const vmCvar_t* cvar;
	vec4_t color;

	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || ((cvar = weaponCvars[weapon]) == NULL))
	{
		return;
	}

	if (CG_ParseColorStr(cvar->string, color))
	{
		VectorCopy(color, cg_weapons[weapon].flashDlightColor);
		VectorCopy(color, cg_weapons[weapon].missileDlightColor);
		VectorCopy(color, cg_weapons[weapon].explosionDlightColor);
	}

}

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon(int weaponNum)
{
	int i;
	gitem_t* item;
	gitem_t* ammo;
	weaponInfo_t* weaponInfo;
	float mins[3];
	char path[64];
	float maxs[3];

	if (!weaponNum) return;

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponInfo->registered) return;

	memset(weaponInfo, 0, sizeof(weaponInfo_t));
	weaponInfo->registered = qtrue;

	for (item = bg_itemlist + 1 ; item->classname ; item++)
	{
		if (item->giType == IT_WEAPON && item->giTag == weaponNum)
		{
			weaponInfo->item = item;
			break;
		}
	}
	if (!item->classname)
	{
		CG_Error("Couldn't find weapon %i", weaponNum);
	}
	CG_RegisterItemVisuals(item - bg_itemlist);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(item->world_model[0]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for (i = 0 ; i < 3 ; i++)
	{
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon = trap_R_RegisterShader(item->icon);

	for (ammo = bg_itemlist + 1 ; ammo->classname ; ammo++)
	{
		if (ammo->giType == IT_AMMO && ammo->giTag == weaponNum)
		{
			break;
		}
	}
	if (ammo->classname && ammo->world_model[0])
	{
		weaponInfo->ammoModel = trap_R_RegisterModel(ammo->world_model[0]);
	}
	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path);
	strcat(path, "_flash.md3");
	weaponInfo->flashModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path);
	strcat(path, "_barrel.md3");
	weaponInfo->barrelModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path);
	strcat(path, "_hand.md3");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if (!weaponInfo->handsModel)
	{
		weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");
	}

	weaponInfo->loopFireSound = qfalse;

	switch (weaponNum)
	{
		case WP_GAUNTLET:
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/melee/fstatck.wav", qfalse);
			break;

		case WP_LIGHTNING:
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
			weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", qfalse);

			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", qfalse);



			{
				int altMisses;

				altMisses = 0;
				for (i = 0; i < MAX_ALT_SHADERS; ++i)
				{
					if (i == 0)//cg_altLightning 0
					{
						cgs.media.lightningBolt[0] = trap_R_RegisterShader("lightningBoltNew");
						cgs.media.lightningBoltNoPicMip[0] = trap_R_RegisterShader("lightningBoltNewNoPicMip");
						cgs.media.enemyLightningBolt[0] = trap_R_RegisterShader("enemyLightningBoltNew");
						cgs.media.enemyLightningBoltNoPicMip[0] = trap_R_RegisterShader("enemyLightningBoltNewNoPicMip");
						if (!cgs.media.lightningBoltNoPicMip[0])
						{
							cgs.media.lightningBoltNoPicMip[0] = cgs.media.lightningBolt[0];
						}
						if (!cgs.media.enemyLightningBoltNoPicMip[0])
						{
							cgs.media.enemyLightningBoltNoPicMip[0] = cgs.media.enemyLightningBolt[0];
						}
					}
					else if (i == 1)//cg_altLightning 1
					{
						cgs.media.lightningBolt[1] = trap_R_RegisterShader("lightningBolt");
						cgs.media.lightningBoltNoPicMip[1] = trap_R_RegisterShader("lightningBoltNoPicMip");
						cgs.media.enemyLightningBolt[1] = trap_R_RegisterShader("enemyLightningBolt");
						cgs.media.enemyLightningBoltNoPicMip[1] = trap_R_RegisterShader("enemyLightningBoltNoPicMip");
						if (!cgs.media.lightningBoltNoPicMip[1])
						{
							cgs.media.lightningBoltNoPicMip[1] = cgs.media.lightningBolt[1];
						}
						if (!cgs.media.enemyLightningBoltNoPicMip[1])
						{
							cgs.media.enemyLightningBoltNoPicMip[1] = cgs.media.enemyLightningBolt[1];
						}
					}
					else if (altMisses >= 2)//cg_altLightning > 1, stop after two missing in a row
					{
						cgs.media.lightningBolt[i] = cgs.media.lightningBolt[0];
						cgs.media.enemyLightningBolt[i] = cgs.media.enemyLightningBolt[0];
						cgs.media.lightningBoltNoPicMip[i] = cgs.media.lightningBolt[i];
						cgs.media.enemyLightningBoltNoPicMip[i] = cgs.media.enemyLightningBolt[i];
					}
					else //cg_altLightning > 1
					{
						qhandle_t bolt;

						bolt = trap_R_RegisterShader(va("lightningBoltNew%i", i));
						cgs.media.enemyLightningBolt[i] = trap_R_RegisterShader(va("enemyLightningBoltNew%i", i));
						if (!bolt)
						{
							altMisses++;
							cgs.media.lightningBolt[i] = cgs.media.lightningBolt[0];
						}
						else
						{
							altMisses = 0;
							cgs.media.lightningBolt[i] = bolt;
						}
						if (!cgs.media.enemyLightningBolt[i])
						{
							cgs.media.enemyLightningBolt[i] = cgs.media.enemyLightningBolt[0];
						}
						cgs.media.lightningBoltNoPicMip[i] = trap_R_RegisterShader(va("lightningBoltNewNoPicMip%i", i));
						cgs.media.enemyLightningBoltNoPicMip[i] = trap_R_RegisterShader(va("enemyLightningBoltNewNoPicMip%i", i));
						if (!cgs.media.lightningBoltNoPicMip[i])
						{
							cgs.media.lightningBoltNoPicMip[i] = cgs.media.lightningBolt[i];
						}
						if (!cgs.media.enemyLightningBoltNoPicMip[i])
						{
							cgs.media.enemyLightningBoltNoPicMip[i] = cgs.media.enemyLightningBolt[i];
						}
					}
				}
			}

			cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");

			cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
			cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
			cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

			break;

		case WP_GRAPPLING_HOOK:
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->missileModel = trap_R_RegisterModel("models/weapons2/grapple2/grapple_grip.md3");
			weaponInfo->missileTrailFunc = CG_GrappleTrail;
			weaponInfo->missileDlight = 200;
			weaponInfo->wiTrailTime = 2000;
			weaponInfo->trailRadius = 64;
			MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
			weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
			trap_R_RegisterShader("models/weapons2/grapple2/ball");
			trap_R_RegisterShader("models/weapons2/grapple2/spikes");
			cgs.media.grappleRopeShader = trap_R_RegisterShader("grappleRope");
			break;

		case WP_MACHINEGUN:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
			weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", qfalse);
			weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", qfalse);
			weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", qfalse);
			weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
			cgs.media.bulletExplosionShader = trap_R_RegisterShader("bulletExplosion");
			cgs.media.bulletExplosionShaderNoPicMip = trap_R_RegisterShader("bulletExplosionNoPicMip");
			if (!cgs.media.bulletExplosionShaderNoPicMip)
			{
				cgs.media.bulletExplosionShaderNoPicMip = cgs.media.bulletExplosionShader;
			}
			break;

		case WP_SHOTGUN:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
			weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
			break;

		case WP_ROCKET_LAUNCHER:
			weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
			weaponInfo->missileTrailFunc = CG_RocketTrail;
			weaponInfo->missileDlight = 200;
			weaponInfo->wiTrailTime = 2000;
			weaponInfo->trailRadius = 64;

			MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);

			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
			cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
			cgs.media.rocketExplosionShaderNoPicMip = trap_R_RegisterShader("rocketExplosionNoPicMip");
			if (!cgs.media.rocketExplosionShaderNoPicMip)
			{
				cgs.media.rocketExplosionShaderNoPicMip = cgs.media.rocketExplosionShader;
			}
			break;

		case WP_GRENADE_LAUNCHER:
			weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/grenade1.md3");
			weaponInfo->missileTrailFunc = CG_RocketTrail;
			weaponInfo->wiTrailTime = 700;
			weaponInfo->trailRadius = 32;
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
			cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
			cgs.media.grenadeExplosionShaderNoPicMip = trap_R_RegisterShader("grenadeExplosionNoPicMip");
			if (!cgs.media.grenadeExplosionShaderNoPicMip)
			{
				cgs.media.grenadeExplosionShaderNoPicMip = cgs.media.grenadeExplosionShader;
			}
			break;

		case WP_PLASMAGUN:
			weaponInfo->missileTrailFunc = CG_PlasmaTrail;
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
			weaponInfo->missileDlight = 150;
			MAKERGB(weaponInfo->missileDlightColor, 0.6f, 0.6f, 1.0f);
			MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
			cgs.media.plasmaExplosionShader = trap_R_RegisterShader("plasmaExplosion");
			cgs.media.plasmaExplosionShaderNoPicMip = trap_R_RegisterShader("plasmaExplosionNoPicMip");
			if (!cgs.media.plasmaExplosionShaderNoPicMip)
			{
				cgs.media.plasmaExplosionShaderNoPicMip = cgs.media.plasmaExplosionShader;
			}

			cgs.media.plasmaNewExplosionShader = trap_R_RegisterShader("plasmanewExplosion");
			if (!cgs.media.plasmaNewExplosionShader)
			{
				cgs.media.plasmaNewExplosionShader = cgs.media.plasmaExplosionShader;
			}

			cgs.media.plasmaNewExplosionShaderNoPicMip = trap_R_RegisterShader("plasmanewExplosionNoPicMip");
			if (!cgs.media.plasmaNewExplosionShaderNoPicMip)
			{
				cgs.media.plasmaNewExplosionShaderNoPicMip = cgs.media.plasmaExplosionShader;
			}

			cgs.media.plasmaOldExplosionShader = trap_R_RegisterShader("plasmaExplosion_old");
			if (!cgs.media.plasmaOldExplosionShader)
			{
				cgs.media.plasmaOldExplosionShader = cgs.media.plasmaExplosionShader;
			}

			cgs.media.plasmaExplosionColorShader = trap_R_RegisterShader("plasmaExplosion2");
			cgs.media.plasmaExplosionColorOldShader = trap_R_RegisterShader("plasmaExplosion2_old");
			if (!cgs.media.plasmaExplosionColorOldShader)
			{
				cgs.media.plasmaExplosionColorOldShader = cgs.media.plasmaExplosionColorShader;
			}

			cgs.media.plasmaExplosionQLShader = trap_R_RegisterShader("plasmaExplosionQL");
			if (!cgs.media.plasmaExplosionQLShader)
				cgs.media.plasmaExplosionQLShader = cgs.media.plasmaExplosionShader;

			cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
			cgs.media.railRingsShaderNoPicMip = trap_R_RegisterShader("railDiscNoPicMip");
			if (!cgs.media.railRingsShaderNoPicMip)
			{
				cgs.media.railRingsShaderNoPicMip = cgs.media.railRingsShader;
			}
			break;

		case WP_RAILGUN:
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", qfalse);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);

			cgs.media.railExplosionShader = trap_R_RegisterShader("railExplosion");
			cgs.media.railExplosionShaderNoPicMip = trap_R_RegisterShader("railExplosionNoPicMip");
			if (!cgs.media.railExplosionShaderNoPicMip)
			{
				cgs.media.railExplosionShaderNoPicMip = cgs.media.railExplosionShader;
			}

			cgs.media.railRingsShader = trap_R_RegisterShader("railDisc");
			cgs.media.railRingsShaderNoPicMip = trap_R_RegisterShader("railDiscNoPicMip");
			if (!cgs.media.railRingsShaderNoPicMip)
			{
				cgs.media.railRingsShaderNoPicMip = cgs.media.railRingsShader;
			}

			cgs.media.railCoreShader = trap_R_RegisterShader("railCore");
			cgs.media.railCoreShaderNoPicMip = trap_R_RegisterShader("railCoreNoPicMip");
			if (!cgs.media.railCoreShaderNoPicMip)
			{
				cgs.media.railCoreShaderNoPicMip = cgs.media.railCoreShader;
			}
			/* altRail 2,3,4,5: register all shaders once so no vid_restart needed */
			cgs.media.ratRailCoreShader = trap_R_RegisterShader("ratRailCore");
			cgs.media.ratRailCoreShaderOverlay = trap_R_RegisterShader("ratRailCoreOverlay");
			cgs.media.ratRailCoreShaderFat = trap_R_RegisterShader("ratRailCoreFat");
			cgs.media.ratRailCoreShaderOverlayFat = trap_R_RegisterShader("ratRailCoreOverlayFat");
			cgs.media.ratRailSpiralModel = trap_R_RegisterModel("models/ammo/rail/railTube.md3");
			cgs.media.ratRailTubeShader100 = trap_R_RegisterShader("railTube100");
			cgs.media.ratRailTubeShader50 = trap_R_RegisterShader("railTube50");
			{
				int ri;
				for (ri = 0; ri < NUM_RAILSPIRALSHADERS; ri++)
				{
					cgs.media.ratRailSpiralShaders[ri] = trap_R_RegisterShader(va("ratRailSpiralLightning%i", ri));
				}
			}
			break;

		case WP_BFG:
			weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", qfalse);
			MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", qfalse);
			cgs.media.bfgExplosionShader = trap_R_RegisterShader("bfgExplosion");
			cgs.media.bfgExplosionShaderNoPicMip = trap_R_RegisterShader("bfgExplosionNoPicMip");
			MAKERGB(weaponInfo->missileDlightColor, 0, 1, 0);
			weaponInfo->missileDlight = 200;
			weaponInfo->wiTrailTime = 2000;
			weaponInfo->trailRadius = 64;
			if (!cgs.media.bfgExplosionShaderNoPicMip)
			{
				cgs.media.bfgExplosionShaderNoPicMip = cgs.media.bfgExplosionShader;
			}
			weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/bfg.md3");
			weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
			break;

		default:
			MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
			weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
			break;
	}
	CG_UpdateWeaponDlightColor(weaponNum);
}

/*
=================
CG_UnregisterItemVisualForPowerup

So quad / enviro visuals can switch between vanilla and dye world models when color cvars change.
=================
*/
void CG_UnregisterItemVisualForPowerup(powerup_t pw)
{
	const gitem_t* it;

	it = BG_FindItemForPowerup(pw);
	if (!it)
	{
		return;
	}
	cg_items[ITEM_INDEX(it)].registered = qfalse;
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals(int itemNum)
{
	itemInfo_t*      itemInfo;
	gitem_t*         item;

	if (itemNum < 0 || itemNum >= bg_numItems)
	{
		CG_Error("CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems - 1);
	}

	itemInfo = &cg_items[ itemNum ];
	if (itemInfo->registered)
	{
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset(itemInfo, 0, sizeof(itemInfo_t));
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel(item->world_model[0]);

	itemInfo->icon = item->icon ? trap_R_RegisterShader(item->icon) : 0;

	if (item->giType == IT_WEAPON)
	{
		CG_RegisterWeapon(item->giTag);
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if (item->giType == IT_POWERUP || item->giType == IT_HEALTH ||
	        item->giType == IT_ARMOR || item->giType == IT_HOLDABLE)
	{
		if (item->world_model[1])
		{
			itemInfo->models[1] = trap_R_RegisterModel(item->world_model[1]);
		}
	}

	{
		const bg_customItemVisuals_t* cv;
		int j;

		cv = BG_CustomItemVisualsFind(item->classname);
		if (cv)
		{
			qboolean useDyeVisuals = qtrue;
			qboolean useDyeFlagModels = qtrue;
			qboolean useDyeFlagIcon = qtrue;
			qhandle_t dyeIcon = 0;

			if (!Q_stricmp(item->classname, "item_quad"))
			{
				useDyeVisuals = cg.quadDamageColorEnabled;
			}
			else if (!Q_stricmp(item->classname, "item_enviro"))
			{
				useDyeVisuals = cg.battleSuitColorEnabled;
			}
			else if (!Q_stricmp(item->classname, "team_CTF_redflag") ||
			         !Q_stricmp(item->classname, "team_CTF_blueflag"))
			{
				useDyeFlagModels = CG_OldFlagModels() ? qfalse : qtrue;
				useDyeFlagIcon = CG_OldFlagIcons() ? qfalse : qtrue;
			}
			if (cv->icon)
			{
				dyeIcon = trap_R_RegisterShader(cv->icon);
			}
			if (useDyeVisuals)
			{
				for (j = 0; j < MAX_ITEM_MODELS; j++)
				{
					if (cv->world_model[j])
					{
						if (!useDyeFlagModels)
						{
							continue;
						}
						itemInfo->models[j] = trap_R_RegisterModel(cv->world_model[j]);
						itemInfo->customVisualMask |= ITEMCV_WORLDMODEL_SLOT(j);
					}
				}

				if (dyeIcon && useDyeFlagIcon)
				{
					itemInfo->icon = dyeIcon;
					itemInfo->customVisualMask |= ITEMCV_ICON;
				}
			}
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame(clientInfo_t* ci, int frame)
{
	int dropFirst, attackFirst, attack2First;

	dropFirst = ci->animations[TORSO_DROP].firstFrame;
	if (frame >= dropFirst && frame < dropFirst + 9)
		return (cg_drawGun.integer & DRAW_GUN_NO_SWITCH_ANIMATION) ? 0 : (frame - dropFirst + 6);

	attackFirst = ci->animations[TORSO_ATTACK].firstFrame;
	if (frame >= attackFirst && frame < attackFirst + 6)
		return (cg_drawGun.integer & DRAW_GUN_NO_FIRE_ANIMATION) ? 0 : (1 + frame - attackFirst);

	attack2First = ci->animations[TORSO_ATTACK2].firstFrame;
	if (frame >= attack2First && frame < attack2First + 6)
		return (cg_drawGun.integer & DRAW_GUN_NO_FIRE_ANIMATION) ? 0 : (1 + frame - attack2First);

	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float   scale;
	int     delta;
	float   fracsin;


	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	if (cg_drawGun.integer & DRAW_GUN_NO_MOVE_ANIMATION) return;

	// on odd legs, invert some angles
	if (cg.bobcycle & 1)
	{
		scale = -cg.xyspeed;
	}
	else
	{
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME)
	{
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		origin[2] += cg.landChange * 0.25 *
		             (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if (delta < STEP_TIME / 2)
	{
		origin[2] -= cg.stepChange * 0.25 * delta / (STEP_TIME / 2);
	}
	else if (delta < STEP_TIME)
	{
		origin[2] -= cg.stepChange * 0.25 * (STEP_TIME - delta) / (STEP_TIME / 2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
void CG_LightningBolt(centity_t* cent, float* origin)
{
	int shaft_type;
	float tl;
	qboolean isViewClient;
	qboolean isOurClient;
	qboolean isEnemyClient;
	qboolean isDelagEnabled;

	shaft_type = cg_altLightning.integer;
	isViewClient = cent->currentState.number == cg.predictedPlayerState.clientNum;
	isOurClient = cent->currentState.number == cg.clientNum;
	if (!isOurClient && cg_spectPOV.integer && cg.snap &&
	        (cg.snap->ps.pm_flags & PMF_FOLLOW) &&
	        cent->currentState.number == cg.snap->ps.clientNum)
		isOurClient = qtrue;
	isEnemyClient = !isOurClient && CG_CACHED_IS_ENEMY_NUM(cent->currentState.number);
	isDelagEnabled = (cg_delag.integer & 1 || cg_delag.integer & 2) && !cg.demoPlayback;

	if (cent->currentState.weapon != WP_LIGHTNING)
		return;
	if (isViewClient)
	{
		if (cg.snap->ps.weapon != WP_LIGHTNING || cg_lightningHide.integer)
			return;
		if (cg.snap->ps.weaponstate != WEAPON_FIRING || !(cg.snap->ps.eFlags & EF_FIRING))
			return;
	}
	else if (!(cent->currentState.eFlags & EF_FIRING))
		return;

	{
		refEntity_t beam;
		vec3_t muzzlePoint;
		vec3_t baseAngles;
		vec3_t forward;
		trace_t trace;
		vec3_t endPoint;

		memset(&beam, 0, sizeof(refEntity_t));
		tl = cg_trueLightning.value;
		if (tl > 1.0f)
			tl = 1.0f;
		if (tl < 0.0f)
			tl = 0.0f;

		if (isViewClient && isDelagEnabled)
		{
			VectorCopy(cg.predictedPlayerState.viewangles, baseAngles);
			VectorCopy(cg.predictedPlayerState.origin, muzzlePoint);
			muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
		}
		else if (isViewClient && tl)
		{
			VectorCopy(cg.refdefViewAngles, baseAngles);
			VectorCopy(cg.refdef.vieworg, muzzlePoint);
		}
		else
		{
			VectorCopy(cent->lerpAngles, baseAngles);
			VectorCopy(cent->lerpOrigin, muzzlePoint);
			muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
		}

		if (isViewClient && tl)
		{
			int i;
			float a;

			for (i = 0; i < 3; i++)
			{
				a = cent->lerpAngles[i] - baseAngles[i];
				if (a > 180.0f)
					a -= 360.0f;
				else if (a < -180.0f)
					a += 360.0f;
				baseAngles[i] = baseAngles[i] + a * (1.0f - tl);
				if (baseAngles[i] < 0.0f)
					baseAngles[i] += 360.0f;
				else if (baseAngles[i] > 360.0f)
					baseAngles[i] -= 360.0f;
			}
		}
		AngleVectors(baseAngles, forward, NULL, NULL);

		VectorMA(muzzlePoint, 14, forward, muzzlePoint);
		VectorMA(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);
		CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cent->currentState.number, MASK_SHOT);

		VectorCopy(trace.endpos, beam.oldorigin);
		VectorCopy(origin, beam.origin);
		beam.reType = RT_LIGHTNING;

		if (isEnemyClient)
		{
			if (cg_enemyAltLightning.string[0])
				shaft_type = cg_enemyAltLightning.integer;
		}
		else if (!isOurClient)
		{
			if (cg_teamAltLightning.string[0])
				shaft_type = cg_teamAltLightning.integer;
		}

		if (shaft_type >= MAX_ALT_SHADERS || shaft_type < 0)
			shaft_type = LIGHTNING_DEFAULT_SHADER;

		{
			const playerColors_t* colors;
			qboolean useColorShader;

			colors = NULL;
			useColorShader = qfalse;
			if (CG_BE_FEATURE_ENABLED(CG_BE_ENEMYLIGHTNING))
			{
				if (isOurClient && cgs.osp.myColorsOverride.isLightningColorSet)
				{
					colors = &cgs.osp.myColors;
					useColorShader = qtrue;
				}
				else if (isEnemyClient && cgs.osp.enemyColorsOverride.isLightningColorSet)
				{
					colors = &cgs.osp.enemyColors;
					useColorShader = qtrue;
				}
				else if (!isOurClient && !isEnemyClient && cgs.osp.teamColorsOverride.isLightningColorSet)
				{
					colors = &cgs.osp.teamColors;
					useColorShader = qtrue;
				}
				else if (!isOurClient && cgs.osp.myColorsOverride.isLightningColorSet)
				{
					colors = &cgs.osp.myColors;
					useColorShader = qtrue;
				}
			}
			if (useColorShader)
			{
				beam.customShader = cg_nomip.integer & 1 ? cgs.media.enemyLightningBoltNoPicMip[shaft_type] : cgs.media.enemyLightningBolt[shaft_type];
				beam.shaderRGBA[0] = colors->lightning[0] * 255;
				beam.shaderRGBA[1] = colors->lightning[1] * 255;
				beam.shaderRGBA[2] = colors->lightning[2] * 255;
				beam.shaderRGBA[3] = 255;
			}
			else
				beam.customShader = cg_nomip.integer & 1 ? cgs.media.lightningBoltNoPicMip[shaft_type] : cgs.media.lightningBolt[shaft_type];
		}

		trap_R_AddRefEntityToScene(&beam);

		if (trace.fraction < 1.0f && cg_lightningImpact.integer)
		{
			vec3_t angles;
			vec3_t dir;

			VectorSubtract(beam.oldorigin, beam.origin, dir);
			VectorNormalize(dir);
			memset(&beam, 0, sizeof(beam));
			beam.hModel = cgs.media.lightningExplosionModel;
			VectorMA(trace.endpos, -16, dir, beam.origin);
			angles[0] = rand() % 360;
			angles[1] = rand() % 360;
			angles[2] = rand() % 360;
			AnglesToAxis(angles, beam.axis);
			trap_R_AddRefEntityToScene(&beam);
		}
	}
}

/*
===============
CG_SpawnRailTrail

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
===============
*/
static void CG_SpawnRailTrail(centity_t* cent, vec3_t origin)
{
	clientInfo_t*    ci;

	if (cent->currentState.weapon != WP_RAILGUN)
	{
		return;
	}
	if (!cent->pe.railgunFlash)
	{
		return;
	}
	cent->pe.railgunFlash = qtrue;
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RailTrail(ci, origin, cent->pe.railgunImpact);
}


void CG_InitBrightWeaponsColors(void)
{
	int i;

	for (i = 0; i < WP_NUM_WEAPONS; i++)
	{
		VectorCopy(weaponBrightColors[i], cgs.beBrightWeaponsColor[i]);
		cgs.beBrightWeaponsBeinfoSet[i] = qfalse;
		cgs.beBrightWeaponsBeinfoOpaque[i] = 1.0f;
	}
}

static float CG_GetBrightWeaponOpacity(int weaponNum)
{
	float op;

	if (weaponNum > WP_NONE && weaponNum < WP_NUM_WEAPONS &&
	        cgs.beBrightWeaponsBeinfoSet[weaponNum])
	{
		op = cgs.beBrightWeaponsBeinfoOpaque[weaponNum];
	}
	else
	{
		op = cg_brightWeaponsOpaque.value;
	}

	if (op < 0.0f)
	{
		op = 0.0f;
	}
	else if (op > 1.0f)
	{
		op = 1.0f;
	}

	return op;
}

void CG_SetWeaponBrightColor(refEntity_t* gun, int weaponNum)
{
	float opacity;
	float* color;

	if (weaponNum > WP_NONE && weaponNum < WP_NUM_WEAPONS)
	{
		color = cgs.beBrightWeaponsColor[weaponNum];
		opacity = CG_GetBrightWeaponOpacity(weaponNum);

		gun->shaderRGBA[0] = 255 * color[0];
		gun->shaderRGBA[1] = 255 * color[1];
		gun->shaderRGBA[2] = 255 * color[2];
		gun->shaderRGBA[3] = 255 * opacity;
	}
}

void CG_SetWeaponBrightColorWorld(refEntity_t* ent, int weaponNum)
{
	float opacity;

	if (weaponNum > WP_NONE && weaponNum < WP_NUM_WEAPONS)
	{
		opacity = CG_GetBrightWeaponOpacity(weaponNum);

		ent->shaderRGBA[0] = 255 * cgs.beBrightWeaponsColor[weaponNum][0];
		ent->shaderRGBA[1] = 255 * cgs.beBrightWeaponsColor[weaponNum][1];
		ent->shaderRGBA[2] = 255 * cgs.beBrightWeaponsColor[weaponNum][2];
		ent->shaderRGBA[3] = 255 * opacity;
	}
}

qhandle_t CG_SelectBrightWeaponsShader(void)
{
	if (cg_brightWeaponsShader.integer == 2)
	{
		if (cgs.media.altFullBrightShader)
		{
			return cgs.media.altFullBrightShader;
		}
	}
	if (cgs.media.fullBrightShader)
	{
		return cgs.media.fullBrightShader;
	}
	return cgs.media.firstPersonGun;
}

void CG_ApplyBrightBrassIfNeeded(localEntity_t* le)
{
	weapon_t      weaponNum;
	int           clientNum;
	int           drawBright;
	qboolean      isEnemy;
	qboolean      isTeammate;
	qboolean      isOwn;
	qboolean      isSpectatingFollow;
	qhandle_t     hModel;

	if (!(le->leFlags & LEF_PLAYER_BRASS) || le->leBounceSoundType != LEBS_BRASS)
	{
		return;
	}

	drawBright = cg_drawBrightWeapons.integer;
	if (!drawBright || !CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT))
	{
		return;
	}

	clientNum = CG_LE_BRASS_CLIENT_NUM(le->leFlags);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	isOwn = (clientNum == cg.clientNum);
	isEnemy = CG_CACHED_IS_ENEMY_NUM(clientNum);
	isTeammate = !isEnemy && !isOwn;
	isSpectatingFollow = (cg.snap->ps.pm_flags & PMF_FOLLOW) && (clientNum == cg.snap->ps.clientNum);

	if (!(((drawBright & 1) && (isOwn || isSpectatingFollow)) ||
	        ((drawBright & 2) && isTeammate) ||
	        ((drawBright & 4) && isEnemy)))
	{
		return;
	}

	hModel = le->refEntity.hModel;
	if (hModel == cgs.media.machinegunBrassModel)
	{
		weaponNum = WP_MACHINEGUN;
	}
	else if (hModel == cgs.media.shotgunBrassModel)
	{
		weaponNum = WP_SHOTGUN;
	}
	else
	{
		return;
	}

	CG_SetWeaponBrightColor(&le->refEntity, weaponNum);
	le->refEntity.customShader = CG_SelectBrightWeaponsShader();
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world model / other character models (ps is NULL).
The main player has this called for BOTH cases; effects like light and sound are done only for world model.
=============
*/
void CG_AddPlayerWeapon(refEntity_t* parent, playerState_t* ps, centity_t* cent, int team)
{
	clientInfo_t*    ci;
	refEntity_t      gun;
	refEntity_t      barrel;
	refEntity_t      flash;
	vec3_t           angles;
	weapon_t         weaponNum;
	weaponInfo_t*    weapon;
	centity_t*       nonPredictedCent;
	orientation_t    lerped;
	vec4_t           gunColor;
	qboolean        ghostGun;
	qboolean isOwnGun;
	qboolean isEnemy;
	qboolean isTeammate;
	qboolean usedBrightColor;
	qboolean isSpectatingFollow;
	qboolean continuousWeaponFiring;
	int drawGunFlags;
	int railChamberFlags;
	float brightWeaponOpacity;

	/* --- cvars (hot path: 64 players * 60 FPS) --- */
	drawGunFlags = cg_drawGun.integer;
	railChamberFlags = cg_railCustomChamber.integer;

	/* --- who we're drawing: own gun, enemy, teammate, spectate follow --- */
	isOwnGun = ps && cent->currentState.number == cg.clientNum;
	isEnemy = CG_CACHED_IS_ENEMY_NUM(cent->currentState.number);
	isTeammate = !isEnemy && (cent->currentState.number != cg.clientNum);
	usedBrightColor = qfalse;
	isSpectatingFollow = (cg.snap->ps.pm_flags & PMF_FOLLOW) && (cent->currentState.number == cg.snap->ps.clientNum);

	weaponNum = cent->currentState.weapon;
	if (!cg_weapons[weaponNum].registered)
		CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];
	ci = &cgs.clientinfo[ps ? cg.predictedPlayerState.clientNum : cent->currentState.clientNum];
	continuousWeaponFiring = (cent->currentState.eFlags & EF_FIRING) != 0;
	if ((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET) && cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		continuousWeaponFiring = cg.snap->ps.weapon == weaponNum && (cg.snap->ps.eFlags & EF_FIRING) &&
		                         (weaponNum == WP_LIGHTNING ? cg.snap->ps.weaponstate == WEAPON_FIRING :
		                          cg.snap->ps.weaponstate == WEAPON_READY || cg.snap->ps.weaponstate == WEAPON_FIRING);
	}

	/* --- base gun refEntity (default white; rail overrides below when applicable) --- */
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;
	gun.shaderRGBA[0] = gun.shaderRGBA[1] = gun.shaderRGBA[2] = gun.shaderRGBA[3] = 255;

	/* --- rail chamber color: cg_railCustomChamber bitmask 1=reload railCore, 2=ready railRings --- */
	if (weaponNum == WP_RAILGUN && railChamberFlags)
	{
		if (ps)
		{
			if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING)
			{
				float f = (float)cg.predictedPlayerState.weaponTime / 1500.0f;
				if (railChamberFlags & 1)
				{
					gun.shaderRGBA[0] = 255 * ci->colors.railCore[0] * (1.0f - f);
					gun.shaderRGBA[1] = 255 * ci->colors.railCore[1] * (1.0f - f);
					gun.shaderRGBA[2] = 255 * ci->colors.railCore[2] * (1.0f - f);
				}
				else
				{
					gun.shaderRGBA[1] = 0;
					gun.shaderRGBA[0] = gun.shaderRGBA[2] = (byte)(255 * (1.0f - f));
				}
			}
			else if (railChamberFlags & 2)
			{
				gun.shaderRGBA[0] = (byte)(255 * ci->colors.railRings[0]);
				gun.shaderRGBA[1] = (byte)(255 * ci->colors.railRings[1]);
				gun.shaderRGBA[2] = (byte)(255 * ci->colors.railRings[2]);
			}
		}
		else if (railChamberFlags & 2)
		{
			gun.shaderRGBA[0] = (byte)(255 * ci->colors.railRings[0]);
			gun.shaderRGBA[1] = (byte)(255 * ci->colors.railRings[1]);
			gun.shaderRGBA[2] = (byte)(255 * ci->colors.railRings[2]);
		}
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel)
		return;

	/* --- world model only: looping weapon sounds (LG etc.) --- */
	if (!ps)
	{
		cent->pe.lightningFiring = qfalse;
		if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound &&
		        (cent->currentState.number != cg.clientNum || cg.snap->ps.weaponstate == WEAPON_READY || cg.snap->ps.weaponstate == WEAPON_FIRING))
		{
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			cent->pe.lightningFiring = qtrue;
		}
		else if (weapon->readySound)
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
	}

	/* --- gun position/orient from tag_weapon + cg_gunPos (left/center/right) --- */
	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
	               1.0f - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);
	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	/* offset for each weapon to center (magic numbers; md3 fix would be better) + cg_gunPos (left/center/right) */
	if (ps)
	{
		static const float gunOffsetY[WP_NUM_WEAPONS] = {0, 0, 0.275f, 0.05f, 0.275f, 0.4f, 0.425f, 0.45f, 0, 0.225f};
		weapon_t w = cg.predictedPlayerState.weapon;
		if (w > WP_NONE && w < WP_NUM_WEAPONS && gunOffsetY[w] != 0.0f)
			VectorMA(gun.origin, gunOffsetY[w], parent->axis[1], gun.origin);
		if (cg_gunPos.integer == 2)
			VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
		else if (cg_gunPos.integer != 3)
			VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);
	}
	else
		VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);


	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);
	{
		vec3_t tempAxis[3];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
		MatrixMultiply(lerped.axis, parent->axis, tempAxis);
#pragma GCC diagnostic pop
		AxisCopy(tempAxis, gun.axis);
	}
	gun.backlerp = parent->backlerp;

	/* --- bright weapons (BE fullbright + cg_drawBrightWeapons bits) --- */
	{
		qboolean wantBright =
		    CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT) &&
		    (
		        ((cg_drawBrightWeapons.integer & 1) && (isOwnGun || isSpectatingFollow)) ||
		        ((cg_drawBrightWeapons.integer & 2) && isTeammate) ||
		        ((cg_drawBrightWeapons.integer & 4) && isEnemy)
		    );

		if (wantBright && weaponNum > WP_NONE && weaponNum < WP_NUM_WEAPONS)
		{
			float* bc = cgs.beBrightWeaponsColor[weaponNum];
			brightWeaponOpacity = CG_GetBrightWeaponOpacity(weaponNum);
			gun.shaderRGBA[0] = (byte)(255 * bc[0]);
			gun.shaderRGBA[1] = (byte)(255 * bc[1]);
			gun.shaderRGBA[2] = (byte)(255 * bc[2]);
			gun.shaderRGBA[3] = (byte)(255 * brightWeaponOpacity);
			usedBrightColor = qtrue;
			gun.customShader = CG_SelectBrightWeaponsShader();
		}
	}
	/* --- ghost gun (cg_gunColor / cg_gunOpaque) --- */
	{
		ghostGun = ((drawGunFlags & DRAW_GUN_GHOST) || drawGunFlags == 3) && (parent->renderfx & RF_FIRST_PERSON);
		if (ghostGun)
		{
			if (!CG_ParseColorStr(cg_gunColor.string, gunColor))
				CG_CvarResetToDefault("cg_guncolor");
			gunColor[3] = (float)atof(cg_gunOpaque.string);
			if (gunColor[3] < 0.0f) gunColor[3] = 0.0f;
			if (gunColor[3] > 1.0f) gunColor[3] = 1.0f;
			if (!usedBrightColor)
			{
				gun.shaderRGBA[0] = (byte)(255 * gunColor[0]);
				gun.shaderRGBA[1] = (byte)(255 * gunColor[1]);
				gun.shaderRGBA[2] = (byte)(255 * gunColor[2]);
				gun.shaderRGBA[3] = (byte)(255 * gunColor[3]);
			}
			gun.customShader = cgs.media.firstPersonGun;
		}
	}
	if ((parent->renderfx & RF_FIRST_PERSON) && cg_drawGunForceAspect.integer)
		VectorScale(gun.axis[1], 0.75f, gun.axis[1]);

	/* --- draw gun + spinning barrel (invis/battle/quad) --- */
	{
		int pw = cent->currentState.powerups;
		qboolean pwInvis = pw & (1 << PW_INVIS);
		qboolean pwBattle = pw & (1 << PW_BATTLESUIT);
		qboolean pwQuad = pw & (1 << PW_QUAD);
		qboolean pwIsEnemy;
		qboolean needPwEnemy;
		qboolean useEnemyMono;
		qboolean useBattleMono;
		qboolean useEnemyQuadMono;
		qboolean useQuadMono;
		qboolean fragWeaponEffect = qfalse;
		qhandle_t battleShader = 0;
		qhandle_t quadGunShader = 0;
		int pwClientNum;

		pwIsEnemy = qfalse;
		needPwEnemy = qfalse;
		if (pwBattle && (cg.enemyBattleSuitColorEnabled || cg.battleSuitColorEnabled))
		{
			needPwEnemy = qtrue;
		}
		if (pwQuad && (cg.enemyQuadDamageColorEnabled || cg.quadDamageColorEnabled))
		{
			needPwEnemy = qtrue;
		}
		pwClientNum = cent->currentState.clientNum;
		if ((pwClientNum < 0 || pwClientNum >= MAX_CLIENTS) &&
		        cent->currentState.number >= 0 && cent->currentState.number < MAX_CLIENTS)
		{
			pwClientNum = cent->currentState.number;
		}
		if (needPwEnemy && pwClientNum >= 0 && pwClientNum < MAX_CLIENTS)
		{
			pwIsEnemy = CG_CACHED_IS_ENEMY_POWERUP_NUM(pwClientNum);
		}
		useEnemyMono = pwBattle && cg.enemyBattleSuitColorEnabled && pwIsEnemy;
		useBattleMono = pwBattle && cg.battleSuitColorEnabled && !useEnemyMono;
		useEnemyQuadMono = pwQuad && cg.enemyQuadDamageColorEnabled && pwIsEnemy;
		useQuadMono = pwQuad && cg.quadDamageColorEnabled && !useEnemyQuadMono;

		if (pwBattle)
		{
			if (useEnemyMono || useBattleMono)
			{
				battleShader = cgs.media.battleWeaponShaderDye;
			}
			else
			{
				battleShader = cgs.media.battleWeaponShader;
			}
		}
		if (pwQuad)
		{
			if (useEnemyQuadMono || useQuadMono)
			{
				quadGunShader = cgs.media.quadWeaponShaderDye;
			}
			else
			{
				quadGunShader = cgs.media.quadWeaponShader;
			}
		}
		if (cg_fragEffect.integer &&
		        cg_fragEffectTime.integer > 0 &&
		        cg.time < cg.fragWeaponEffectTime &&
		        cg.fragWeaponEffectClientNum == cg.snap->ps.clientNum &&
		        ps && (parent->renderfx & RF_FIRST_PERSON))
		{
			fragWeaponEffect = qtrue;
		}

		/* gun */
		if (pwInvis)
		{
			gun.customShader = cgs.media.invisShader;
			trap_R_AddRefEntityToScene(&gun);
		}
		else
		{
			trap_R_AddRefEntityToScene(&gun);
			if (pwBattle)
			{
				if (useEnemyMono)
				{
					gun.shaderRGBA[0] = (byte)(cg.beEnemyBattleSuitColor[0] * 255.0f);
					gun.shaderRGBA[1] = (byte)(cg.beEnemyBattleSuitColor[1] * 255.0f);
					gun.shaderRGBA[2] = (byte)(cg.beEnemyBattleSuitColor[2] * 255.0f);
					gun.shaderRGBA[3] = 255;
				}
				else if (useBattleMono)
				{
					gun.shaderRGBA[0] = (byte)(cg.beBattleSuitColor[0] * 255.0f);
					gun.shaderRGBA[1] = (byte)(cg.beBattleSuitColor[1] * 255.0f);
					gun.shaderRGBA[2] = (byte)(cg.beBattleSuitColor[2] * 255.0f);
					gun.shaderRGBA[3] = 255;
				}
				gun.customShader = battleShader;
				trap_R_AddRefEntityToScene(&gun);
			}
			if (pwQuad)
			{
				if (useEnemyQuadMono)
				{
					gun.shaderRGBA[0] = (byte)(cg.beEnemyQuadDamageColor[0] * 255.0f);
					gun.shaderRGBA[1] = (byte)(cg.beEnemyQuadDamageColor[1] * 255.0f);
					gun.shaderRGBA[2] = (byte)(cg.beEnemyQuadDamageColor[2] * 255.0f);
					gun.shaderRGBA[3] = 255;
				}
				else if (useQuadMono)
				{
					gun.shaderRGBA[0] = (byte)(cg.beQuadDamageColor[0] * 255.0f);
					gun.shaderRGBA[1] = (byte)(cg.beQuadDamageColor[1] * 255.0f);
					gun.shaderRGBA[2] = (byte)(cg.beQuadDamageColor[2] * 255.0f);
					gun.shaderRGBA[3] = 255;
				}
				gun.customShader = quadGunShader;
				trap_R_AddRefEntityToScene(&gun);
			}
			if (fragWeaponEffect)
			{
				gun.shaderRGBA[0] = (byte)(cg.beFragEffectColor[0] * 255.0f);
				gun.shaderRGBA[1] = (byte)(cg.beFragEffectColor[1] * 255.0f);
				gun.shaderRGBA[2] = (byte)(cg.beFragEffectColor[2] * 255.0f);
				gun.shaderRGBA[3] = (byte)(cg.beFragEffectColor[3] * 255.0f);
				gun.customShader = cgs.media.fragEffectShader;
				trap_R_AddRefEntityToScene(&gun);
			}
		}

		/* spinning barrel */
		if (weapon->barrelModel)
		{
			float brlAngle;

			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx = parent->renderfx;
			barrel.hModel = weapon->barrelModel;

			angles[YAW] = angles[PITCH] = 0;
			if ((drawGunFlags & DRAW_GUN_NO_FIRE_ANIMATION) && (parent->renderfx & RF_FIRST_PERSON))
				angles[ROLL] = 0;
			else
			{
				/* inline CG_MachinegunSpinAngle */
				int d = cg.time - cent->pe.barrelTime;
				qboolean barrelFiring = (cent->currentState.eFlags & EF_FIRING) != 0;
				if (cent->currentState.number == cg.predictedPlayerState.clientNum)
				{
					barrelFiring = cg.snap->ps.weapon == weaponNum && (cg.snap->ps.eFlags & EF_FIRING) &&
					               (cg.snap->ps.weaponstate == WEAPON_READY || cg.snap->ps.weaponstate == WEAPON_FIRING);
				}
				if (cent->pe.barrelSpinning)
					brlAngle = cent->pe.barrelAngle + d * 0.9f;
				else
				{
					float spd;
					if (d > 1000) d = 1000;
					spd = 0.5f * (0.9f + (float)(1000 - d) / 1000.0f);
					brlAngle = cent->pe.barrelAngle + d * spd;
				}
				if (cent->pe.barrelSpinning == !barrelFiring)
				{
					cent->pe.barrelTime = cg.time;
					cent->pe.barrelAngle = AngleMod(brlAngle);
					cent->pe.barrelSpinning = barrelFiring;
				}
				angles[ROLL] = brlAngle;
			}
			AnglesToAxis(angles, barrel.axis);
			CG_PositionRotatedEntityOnTag(&barrel, &gun, weapon->weaponModel, "tag_barrel");

			if (usedBrightColor && weaponNum > WP_NONE && weaponNum < WP_NUM_WEAPONS)
			{
				float* bc = cgs.beBrightWeaponsColor[weaponNum];
				brightWeaponOpacity = CG_GetBrightWeaponOpacity(weaponNum);
				barrel.shaderRGBA[0] = (byte)(255 * bc[0]);
				barrel.shaderRGBA[1] = (byte)(255 * bc[1]);
				barrel.shaderRGBA[2] = (byte)(255 * bc[2]);
				barrel.shaderRGBA[3] = (byte)(255 * brightWeaponOpacity);
				barrel.customShader = CG_SelectBrightWeaponsShader();
			}
			else if (ghostGun)
			{
				barrel.shaderRGBA[0] = (byte)(255 * gunColor[0]);
				barrel.shaderRGBA[1] = (byte)(255 * gunColor[1]);
				barrel.shaderRGBA[2] = (byte)(255 * gunColor[2]);
				barrel.shaderRGBA[3] = (byte)(255 * gunColor[3]);
				barrel.customShader = cgs.media.firstPersonGun;
			}
			if ((parent->renderfx & RF_FIRST_PERSON) && cg_drawGunForceAspect.integer)
				VectorScale(barrel.axis[1], 0.75f, barrel.axis[1]);
			if (pwInvis)
			{
				barrel.customShader = cgs.media.invisShader;
				trap_R_AddRefEntityToScene(&barrel);
			}
			else
			{
				trap_R_AddRefEntityToScene(&barrel);
				if (pwBattle)
				{
					if (useEnemyMono)
					{
						barrel.shaderRGBA[0] = (byte)(cg.beEnemyBattleSuitColor[0] * 255.0f);
						barrel.shaderRGBA[1] = (byte)(cg.beEnemyBattleSuitColor[1] * 255.0f);
						barrel.shaderRGBA[2] = (byte)(cg.beEnemyBattleSuitColor[2] * 255.0f);
						barrel.shaderRGBA[3] = 255;
					}
					else if (useBattleMono)
					{
						barrel.shaderRGBA[0] = (byte)(cg.beBattleSuitColor[0] * 255.0f);
						barrel.shaderRGBA[1] = (byte)(cg.beBattleSuitColor[1] * 255.0f);
						barrel.shaderRGBA[2] = (byte)(cg.beBattleSuitColor[2] * 255.0f);
						barrel.shaderRGBA[3] = 255;
					}
					barrel.customShader = battleShader;
					trap_R_AddRefEntityToScene(&barrel);
				}
				if (pwQuad)
				{
					if (useEnemyQuadMono)
					{
						barrel.shaderRGBA[0] = (byte)(cg.beEnemyQuadDamageColor[0] * 255.0f);
						barrel.shaderRGBA[1] = (byte)(cg.beEnemyQuadDamageColor[1] * 255.0f);
						barrel.shaderRGBA[2] = (byte)(cg.beEnemyQuadDamageColor[2] * 255.0f);
						barrel.shaderRGBA[3] = 255;
					}
					else if (useQuadMono)
					{
						barrel.shaderRGBA[0] = (byte)(cg.beQuadDamageColor[0] * 255.0f);
						barrel.shaderRGBA[1] = (byte)(cg.beQuadDamageColor[1] * 255.0f);
						barrel.shaderRGBA[2] = (byte)(cg.beQuadDamageColor[2] * 255.0f);
						barrel.shaderRGBA[3] = 255;
					}
					barrel.customShader = quadGunShader;
					trap_R_AddRefEntityToScene(&barrel);
				}
				if (fragWeaponEffect)
				{
					barrel.shaderRGBA[0] = (byte)(cg.beFragEffectColor[0] * 255.0f);
					barrel.shaderRGBA[1] = (byte)(cg.beFragEffectColor[1] * 255.0f);
					barrel.shaderRGBA[2] = (byte)(cg.beFragEffectColor[2] * 255.0f);
					barrel.shaderRGBA[3] = (byte)(cg.beFragEffectColor[3] * 255.0f);
					barrel.customShader = cgs.media.fragEffectShader;
					trap_R_AddRefEntityToScene(&barrel);
				}
			}
		}
	}

	/* --- muzzle flash: skip impulse weapons when flash too old (avoid building flash/nonPredictedCent) --- */
	if (weaponNum != WP_LIGHTNING && weaponNum != WP_GAUNTLET && weaponNum != WP_GRAPPLING_HOOK)
	{
		if (cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash)
			return;
	}

	nonPredictedCent = &cg_entities[cent->currentState.clientNum];
	if ((nonPredictedCent - cg_entities) != cent->currentState.clientNum)
		nonPredictedCent = cent;

	memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;
	flash.hModel = weapon->flashModel;
	if (!flash.hModel)
	{
		return;
	}
	angles[YAW] = angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);
	flash.shaderRGBA[0] = flash.shaderRGBA[1] = flash.shaderRGBA[2] = flash.shaderRGBA[3] = 255;
	if (weaponNum == WP_RAILGUN)
	{
		flash.shaderRGBA[0] = (byte)(255 * ci->colors.railCore[0]);
		flash.shaderRGBA[1] = (byte)(255 * ci->colors.railCore[1]);
		flash.shaderRGBA[2] = (byte)(255 * ci->colors.railCore[2]);
	}
	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
	/* continuous (LG/gauntlet/hook): show flash and effects only when actually firing */
	if (weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK)
	{
		if (weaponNum == WP_GRAPPLING_HOOK ? (nonPredictedCent->currentState.eFlags & EF_FIRING) : continuousWeaponFiring)
		{
			if (cg_muzzleFlash.integer || cent->currentState.clientNum != cg.clientNum)
				trap_R_AddRefEntityToScene(&flash);
			if (ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
			{
				if (weaponNum == WP_LIGHTNING)
					CG_LightningBolt(nonPredictedCent, flash.origin);
				if (weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2])
					trap_R_AddLightToScene(flash.origin, 300 + (rand() & 31),
					                       weapon->flashDlightColor[0],
					                       weapon->flashDlightColor[1],
					                       weapon->flashDlightColor[2]);
			}
		}
	}
	else
	{
		if (cg_muzzleFlash.integer || cent->currentState.clientNum != cg.clientNum)
			trap_R_AddRefEntityToScene(&flash);
		if (ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
		{
			if (weaponNum == WP_RAILGUN)
				CG_SpawnRailTrail(cent, flash.origin);
			if (weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2])
				trap_R_AddLightToScene(flash.origin, 300 + (rand() & 31),
				                       weapon->flashDlightColor[0],
				                       weapon->flashDlightColor[1],
				                       weapon->flashDlightColor[2]);
		}
	}
}


/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon(playerState_t* ps)
{
	refEntity_t hand;
	centity_t* cent;
	clientInfo_t* ci;
	float fovOffset;
	vec3_t angles;
	weaponInfo_t* weapon;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || ps->pm_type == PM_INTERMISSION || cg.renderingThirdPerson)
		return;

	if (!cg_drawGun.integer)
	{
		if (cg.predictedPlayerState.eFlags & EF_FIRING)
		{
			vec3_t origin;
			VectorCopy(cg.refdef.vieworg, origin);
			VectorMA(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// if (cg.testGun)
	//  return;

	// drop gun lower at higher fov
	if (cg_fov.integer > 90)
	{
		fovOffset = -0.2 * (cg_fov.integer - 90);
	}
	else
	{
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;   // &cg_entities[cg.snap->ps.clientNum];
	if (!cg_weapons[ps->weapon].registered)
		CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	VectorMA(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, (cg_gun_z.value + fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// map torso animations to weapon animations
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
	hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldFrame);
	hand.backlerp = cent->pe.torso.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM]);
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect(void)
{
	int i, n, w;
	int indices[15];
	int bits, x, y;
	char* name;
	float* color;

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
		return;

	color = CG_FadeColor(cg.weaponSelectTime, WEAPON_SELECT_TIME);
	if (!color)
		return;

	trap_R_SetColor(color);
	cg.itemPickupTime = 0;

	bits = cg.snap->ps.stats[STAT_WEAPONS];
	n = 0;
	for (i = 1; i < 16; i++)
	{
		if (bits & (1 << i))
			indices[n++] = i;
	}

	x = 320 - n * 20;
	y = 380;

	for (i = 0; i < n; i++)
	{
		w = indices[i];
		if (!cg_weapons[w].registered)
			CG_RegisterWeapon(w);
		CG_DrawPicOld(x, y, 32, 32, cg_weapons[w].weaponIcon);
		if (w == cg.weaponSelect)
			CG_DrawPicOld(x - 4, y - 4, 40, 40, cgs.media.selectShader);
		if (!cg.snap->ps.ammo[w])
			CG_DrawPicOld(x, y, 32, 32, cgs.media.noammoShader);
		x += 40;
	}

	if (cg_weapons[cg.weaponSelect].item)
	{
		name = cg_weapons[cg.weaponSelect].item->pickup_name;
		if (name)
			CG_DrawBigString(SCREEN_WIDTH / 2.0f, y, name, color[3], DS_HCENTER | DS_SHADOW, 0);
	}

	trap_R_SetColor(NULL);
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f(void)
{
	int i, original, w;

	if (!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < 16; i++)
	{
		w = cg.weaponSelect + 1;
		if (w == 16)
			w = 0;
		cg.weaponSelect = w;
		if (w == WP_GAUNTLET)
			continue;
		if (cg.snap->ps.ammo[w] && (cg.snap->ps.stats[STAT_WEAPONS] & (1 << w)))
			return;
	}
	cg.weaponSelect = original;
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f(void)
{
	int i, original, w;

	if (!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		return;

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < 16; i++)
	{
		w = cg.weaponSelect - 1;
		if (w == -1)
			w = 15;
		cg.weaponSelect = w;
		if (w == WP_GAUNTLET)
			continue;
		if (cg.snap->ps.ammo[w] && (cg.snap->ps.stats[STAT_WEAPONS] & (1 << w)))
			return;
	}
	cg.weaponSelect = original;
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f(void)
{
	int num;

	if (!cg.snap || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		return;

	num = atoi(CG_Argv(1));
	if (num < 1 || num > 15)
		return;

	cg.weaponSelectTime = cg.time;
	if (!cg_ammoCheck.integer && !(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num)))
		return;

	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange(void)
{
	int i;

	cg.weaponSelectTime = cg.time;

	for (i = 15; i > 0; i--)
	{
		if (cg.snap->ps.ammo[i] && (cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)))
		{
			cg.weaponSelect = i;
			return;
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

float CG_GetWeaponTempAccuracy(int weapon)
{
	static int lastHits[WP_NUM_WEAPONS] = {0};
	static int lastShots[WP_NUM_WEAPONS] = {0};
	static float lastAcc[WP_NUM_WEAPONS] = {0.0f};

	int hits, shots;
	float acc;

	if (!cgs.beWeaponStats[weapon].onTrack)
	{
		return 0.0f;
	}

	hits = cgs.beWeaponStats[weapon].hitsCurrent - cgs.beWeaponStats[weapon].hitsStart;
	shots = cgs.beWeaponStats[weapon].shotsStart - cg.snap->ps.ammo[weapon];

	if (shots <= 0)
	{
		return 0.0f;
	}

	// Check whether values changed
	if (hits == lastHits[weapon] && shots == lastShots[weapon])
	{
		return lastAcc[weapon]; // return previous value
	}

	acc = (hits / (float)shots) * 100.0f;
	if (acc > 100.0f)
	{
		acc = 100.0f;
	}

	// Update cache
	lastHits[weapon] = hits;
	lastShots[weapon] = shots;
	lastAcc[weapon] = acc;

	CG_SHUDEventTempAccuracy(weapon, acc);

	return acc;
}


/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon(centity_t* cent)
{
	entityState_t* ent = &cent->currentState;
	weaponInfo_t* weap;
	int c;

	if (ent->weapon == WP_NONE)
		return;
	if (ent->weapon >= WP_NUM_WEAPONS)
	{
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}

	weap = &cg_weapons[ent->weapon];
	if (cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		int addTime = 0;
		if (cg_doubleShootTime > 0)
		{
			cg_doubleShootTime -= (cg.time - cg_doubleShootLastTime);
			cg_doubleShootLastTime = cg.time;
			if (cg_doubleShootTime < 0)
				cg_doubleShootTime = 0;
			if (cg_doubleShootTime > 0)
				return;
		}
		if (!cg_doubleShootAddTimeInited)
		{
			cg_doubleShootAddTime[WP_GAUNTLET] = 360;
			cg_doubleShootAddTime[WP_LIGHTNING] = 45;
			cg_doubleShootAddTime[WP_SHOTGUN] = 900;
			cg_doubleShootAddTime[WP_MACHINEGUN] = 90;
			cg_doubleShootAddTime[WP_GRENADE_LAUNCHER] = 700;
			cg_doubleShootAddTime[WP_ROCKET_LAUNCHER] = 700;
			cg_doubleShootAddTime[WP_PLASMAGUN] = 90;
			cg_doubleShootAddTime[WP_RAILGUN] = 200;
			cg_doubleShootAddTime[WP_BFG] = 180;
			cg_doubleShootAddTime[WP_GRAPPLING_HOOK] = 360;
			cg_doubleShootAddTimeInited = qtrue;
		}
		cg_doubleShootAddTime[WP_MACHINEGUN] = cgs.fpsclassico ? 70 : 90;
		cg_doubleShootAddTime[WP_GRENADE_LAUNCHER] = modeGrenadeTime - 100;
		addTime = cg_doubleShootAddTime[ent->weapon];
		if (cg.predictedPlayerState.powerups[PW_HASTE])
			addTime = (int)(addTime / 1.3f);
		cg_doubleShootTime += addTime;
		cg_doubleShootLastTime = cg.time;
	}

	cent->muzzleFlashTime = cg.time;

	if (ent->weapon == WP_LIGHTNING && cent->pe.lightningFiring)
		return;

	if (cent->currentState.number == cg.predictedPlayerState.clientNum)
		CG_DebugServerOnFireWeapon(ent->weapon);

	if ((cent->currentState.powerups & (1 << PW_QUAD)) && cg.time > cgs.osp.quadSoundTime)
	{
		cgs.osp.quadSoundTime = cg.time + 1050;
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound);
	}

	for (c = 0; c < 4 && weap->flashSound[c]; c++)
		;
	if (c > 0)
	{
		c = rand() % c;
		if (weap->flashSound[c])
			trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[c]);
	}

	// do brass ejection
	if (weap->ejectBrassFunc && cg_brassTime.integer > 0)
	{
		weap->ejectBrassFunc(cent);
	}

	CG_PredictWeaponEffects(cent);
}


/*
=================
CG_PlasmaColorForOwner

Returns qtrue when cg_plasmaColor / cg_enemyPlasmaColor apply for this shooter.
=================
*/
qboolean CG_PlasmaColorForOwner(int owner, vec4_t color)
{
	const clientInfo_t* ci;

	if (!CG_BE_FEATURE_ENABLED(CG_BE_PLASMAGUN_COLOR))
	{
		return qfalse;
	}

	if (!color || owner < 0 || owner >= MAX_CLIENTS)
	{
		return qfalse;
	}

	ci = &cgs.clientinfo[owner];
	if (!ci->infoValid)
	{
		return qfalse;
	}

	if (CG_OSPIsGameTypeCA(cgs.gametype) || !CG_CACHED_IS_ENEMY_NUM(owner)
	    || (cgs.gametype == GT_TOURNAMENT/*  && cgs.isOspServer */))
	{
		if (!cg_plasmaColor.string[0])
		{
			return qfalse;
		}
		color[0] = cg.bePlasmaColor[0];
		color[1] = cg.bePlasmaColor[1];
		color[2] = cg.bePlasmaColor[2];
		color[3] = cg.bePlasmaColor[3];
	}
	else
	{
		if (!cg_enemyPlasmaColor.string[0])
		{
			return qfalse;
		}
		color[0] = cg.beEnemyPlasmaColor[0];
		color[1] = cg.beEnemyPlasmaColor[1];
		color[2] = cg.beEnemyPlasmaColor[2];
		color[3] = cg.beEnemyPlasmaColor[3];
	}

	return qtrue;
}

static qboolean CG_PlasmaAltUsesOldColorAssets(void)
{
	return (cg_altPlasma.integer == 2);
}

qhandle_t CG_PlasmaColorBallShader(void)
{
	if (CG_PlasmaAltUsesOldColorAssets())
	{
		return cgs.media.plasmaBallColorOldShader;
	}

	return cgs.media.plasmaBallColorShader;
}

qhandle_t CG_PlasmaColorExplosionShader(void)
{
	if (CG_PlasmaAltUsesOldColorAssets())
	{
		return cgs.media.plasmaExplosionColorOldShader;
	}

	return cgs.media.plasmaExplosionColorShader;
}

void CG_ApplyPlasmaEntityColor(int owner, refEntity_t* ent, qhandle_t* shader)
{
	vec4_t          color;
	qhandle_t       colorShader;

	if (!ent || !shader || !CG_PlasmaColorForOwner(owner, color))
	{
		return;
	}

	colorShader = CG_PlasmaColorBallShader();
	if (!colorShader)
	{
		return;
	}

	*shader = colorShader;
	ent->shaderRGBA[0] = (byte)(color[0] * 255.0f);
	ent->shaderRGBA[1] = (byte)(color[1] * 255.0f);
	ent->shaderRGBA[2] = (byte)(color[2] * 255.0f);
	ent->shaderRGBA[3] = 255;
}

/*
=================
CG_PlasmaTrailAtOrigin

Plasma trail particles at a fixed point (stationary missile / spawn marker).
=================
*/
static void CG_PlasmaTrailAtOrigin(vec3_t origin)
{
	centity_t cent;

	if (cg_noProjectileTrail.integer || cg_oldPlasma.integer)
	{
		return;
	}

	memset(&cent, 0, sizeof(cent));
	cent.currentState.eType = ET_MISSILE;
	cent.currentState.weapon = WP_PLASMAGUN;
	cent.currentState.pos.trType = TR_STATIONARY;
	VectorCopy(origin, cent.currentState.pos.trBase);
	cent.currentState.pos.trTime = cg.time;
	VectorCopy(origin, cent.lerpOrigin);

	CG_PlasmaTrail(&cent, &cg_weapons[WP_PLASMAGUN]);
}

/*
=================
CG_AddPlasmaMissileDlight
=================
*/
void CG_AddPlasmaMissileDlight(vec3_t origin, int ownerNum, qhandle_t spawnShader, const vec3_t spawnTeamColor)
{
	const weaponInfo_t* weapon;
	float               dlR;
	float               dlG;
	float               dlB;
	vec4_t              plasmaColor;

	weapon = &cg_weapons[WP_PLASMAGUN];
	if (!weapon->missileDlight)
	{
		return;
	}

	dlR = weapon->missileDlightColor[0];
	dlG = weapon->missileDlightColor[1];
	dlB = weapon->missileDlightColor[2];
	if (spawnShader && spawnTeamColor)
	{
		dlR = spawnTeamColor[0];
		dlG = spawnTeamColor[1];
		dlB = spawnTeamColor[2];
	}
	else if (CG_PlasmaColorForOwner(ownerNum, plasmaColor))
	{
		dlR = plasmaColor[0];
		dlG = plasmaColor[1];
		dlB = plasmaColor[2];
	}

	trap_R_AddLightToScene(origin, weapon->missileDlight, dlR, dlG, dlB);
}

/*
=================
CG_AddPlasmaMissileScene

Missile visuals for WP_PLASMAGUN: trail (unless spawnShader), optional dlight, sprite.
=================
*/
void CG_AddPlasmaMissileScene(vec3_t origin, int ownerNum, qhandle_t spawnShader, const vec3_t spawnTeamColor, qboolean addDlight)
{
	refEntity_t         ent;

	if (!spawnShader)
	{
		CG_PlasmaTrailAtOrigin(origin);
	}

	if (addDlight)
	{
		CG_AddPlasmaMissileDlight(origin, ownerNum, spawnShader, spawnTeamColor);
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.oldorigin);
	ent.reType = RT_SPRITE;
	ent.radius = 16;
	ent.rotation = 0;

	if (spawnShader)
	{
		ent.customShader = spawnShader;
		if (spawnTeamColor)
		{
			ent.shaderRGBA[0] = (byte)(spawnTeamColor[0] * 255.0f);
			ent.shaderRGBA[1] = (byte)(spawnTeamColor[1] * 255.0f);
			ent.shaderRGBA[2] = (byte)(spawnTeamColor[2] * 255.0f);
			ent.shaderRGBA[3] = 255;
		}
	}
	else
	{
		int altPlasma;

		ent.customShader = cgs.media.plasmaBallShader;
		altPlasma = cg_altPlasma.integer;

		if (altPlasma == 3)
		{
			ent.customShader = cgs.media.plasmaQLBallShader;
		}
		else if (altPlasma == 2)
		{
			ent.customShader = cgs.media.plasmaOldBallShader;
		}
		else if (cg_oldPlasma.integer || !altPlasma || !(cgs.osp.custom_client & OSP_CUSTOM_CLIENT_ALT_WEAPON_FLAG))
		{
			if ((cg_nomip.integer & 2) == 0)
			{
				ent.customShader = cgs.media.plasmaBallShader;
			}
			else
			{
				ent.customShader = cgs.media.plasmaBallNoPicMipShader;
			}
		}
		else
		{
			if ((cg_nomip.integer & 2) == 0)
			{
				ent.customShader = cgs.media.plasmaNewBallShader;
			}
			else
			{
				ent.customShader = cgs.media.plasmaNewBallNoPicMipShader;
			}
		}

		CG_ApplyPlasmaEntityColor(ownerNum, &ent, &ent.customShader);
	}

	trap_R_AddRefEntityToScene(&ent);
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType)
{
	qhandle_t       mod;
	qhandle_t       mark;
	qhandle_t       shader;
	sfxHandle_t     sfx;
	float           radius;
	float           light;
	vec3_t          lightColor;
	localEntity_t*   le;
	int             r;
	qboolean        alphaFade;
	qboolean        isSprite;
	int             duration;
	vec3_t          sprOrg;
	vec3_t          sprVel;
	qboolean        allowExplosions;

	allowExplosions = !cg_noExplosions.integer || (cg_noExplosions.integer && !CG_BE_FEATURE_ENABLED(CG_BE_NO_EXPLOSIONS));
	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	VectorCopy(cg_weapons[weapon].explosionDlightColor, lightColor);

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch (weapon)
	{
		default:
		case WP_LIGHTNING:
			// no explosion at LG impact, it is added with the beam
			r = rand() & 3;
			if (r < 2)
			{
				sfx = cgs.media.sfx_lghit2;
			}
			else if (r == 2)
			{
				sfx = cgs.media.sfx_lghit1;
			}
			else
			{
				sfx = cgs.media.sfx_lghit3;
			}
			if (cg_nomip.integer & 1)
			{
				mark = cgs.media.holeMarkNoPicMipShader;
			}
			else
			{
				mark = cgs.media.holeMarkShader;
			}
			radius = 12;
			break;
		case WP_GRENADE_LAUNCHER:
			mod = cgs.media.dishFlashModel;
			sfx = cgs.media.sfx_rockexp;
			if ((cg_nomip.integer & 0x8) == 0)
			{
				shader = cgs.media.grenadeExplosionShader;
				mark = cgs.media.burnMarkShader;
			}
			else
			{
				shader = cgs.media.grenadeExplosionShaderNoPicMip;
				mark = cgs.media.burnMarkNoPicMipShader;
			}
			radius = 64;
			light = 300;
			isSprite = qtrue;
			break;
		case WP_ROCKET_LAUNCHER:
			mod = cgs.media.dishFlashModel;
			sfx = cgs.media.sfx_rockexp;
			if ((cg_nomip.integer & 0x4) == 0)
			{
				shader = cgs.media.rocketExplosionShader;
				mark = cgs.media.burnMarkShader;
			}
			else
			{
				shader = cgs.media.rocketExplosionShaderNoPicMip;
				mark = cgs.media.burnMarkNoPicMipShader;
			}
			radius = 64;
			light = 300;
			isSprite = qtrue;
			duration = 1000;
			if (allowExplosions)
			{
				if (cg_oldRocket.integer == 0)
				{
					// explosion sprite animation
					VectorMA(origin, 24, dir, sprOrg);
					VectorScale(dir, 64, sprVel);

					CG_ParticleExplosion("explode1", sprOrg, sprVel, 1400, 20, 30);
				}

			}
			break;
		case WP_RAILGUN:
			mod = cgs.media.ringFlashModel;
			sfx = cgs.media.sfx_plasmaexp;
			if ((cg_nomip.integer & 0x20) == 0)
			{
				shader = cgs.media.railExplosionShader;
				mark = cgs.media.energyMarkShader;
			}
			else
			{
				shader = cgs.media.railExplosionShaderNoPicMip;
				mark = cgs.media.energyMarkNoPicMipShader;
			}
			radius = 24;
			break;
		case WP_PLASMAGUN:
		{
			int altPlasma;

			mod = cgs.media.ringFlashModel;
			sfx = cgs.media.sfx_plasmaexp;
			shader = cgs.media.plasmaExplosionShader;
			mark = cgs.media.energyMarkShader;
			altPlasma = cg_altPlasma.integer;

			if (altPlasma == 0)
			{
				if ((cg_nomip.integer & 2) == 0)
				{
					shader = cgs.media.plasmaExplosionShader;
					mark = cgs.media.energyMarkShader;
				}
				else
				{
					shader = cgs.media.plasmaExplosionShaderNoPicMip;
					mark = cgs.media.energyMarkNoPicMipShader;
				}
			}
			else if (altPlasma == 1)
			{
				if ((cg_nomip.integer & 2) == 0)
				{
					shader = cgs.media.plasmaNewExplosionShader;
					mark = cgs.media.energyMarkShader;
				}
				else
				{
					shader = cgs.media.plasmaNewExplosionShaderNoPicMip;
					mark = cgs.media.energyMarkNoPicMipShader;
				}
			}
			else if (altPlasma == 2)
			{
				if ((cg_nomip.integer & 2) == 0)
				{
					shader = cgs.media.plasmaOldExplosionShader; // fallback in case old version failed
					if (cgs.media.plasmaOldExplosionShader)
					{
						shader = cgs.media.plasmaExplosionShader; // default
					}
					if (cgs.media.plasmaOldExplosionShader)
					{
						shader = cgs.media.plasmaOldExplosionShader; // use old if available
					}
					mark = cgs.media.energyMarkPlasmaShader;
				}
				else
				{
					shader = cgs.media.plasmaExplosionShaderNoPicMip; // fallback
					if (cgs.media.plasmaOldExplosionShader)
					{
						shader = cgs.media.plasmaOldExplosionShader; // use old for mip-disabled too
					}
					mark = cgs.media.energyMarkNoPicMipShader;
				}
			}
			else if (altPlasma == 3)
			{
				shader = cgs.media.plasmaExplosionQLShader;
				mark = (cg_nomip.integer & 2) ? cgs.media.energyMarkNoPicMipShader : cgs.media.energyMarkShader;
			}
		}
		radius = 16;
		break;

		case WP_BFG:
			mod = cgs.media.dishFlashModel;
			sfx = cgs.media.sfx_rockexp;
			if ((cg_nomip.integer & 0x40) == 0)
			{
				shader = cgs.media.bfgExplosionShader;
				mark = cgs.media.burnMarkShader;
			}
			else
			{
				shader = cgs.media.bfgExplosionShaderNoPicMip;
				mark = cgs.media.burnMarkNoPicMipShader;
			}
			duration = 800;
			light = 200;
			radius = 64;
			isSprite = qtrue;
			break;
		case WP_SHOTGUN:
			mod = cgs.media.bulletFlashModel;
			if ((cg_nomip.integer & 0x10) == 0)
			{
				shader = cgs.media.bulletExplosionShader;
				mark = cgs.media.bulletMarkShader;
			}
			else
			{
				shader = cgs.media.bulletExplosionShaderNoPicMip;
				mark = cgs.media.bulletMarkNoPicMipShader;
			}
			sfx = 0;
			radius = 4;
			break;
		case WP_MACHINEGUN:
			mod = cgs.media.bulletFlashModel;
			if ((cg_nomip.integer & 0x10) == 0)
			{
				shader = cgs.media.bulletExplosionShader;
				mark = cgs.media.bulletMarkShader;
			}
			else
			{
				shader = cgs.media.bulletExplosionShaderNoPicMip;
				mark = cgs.media.bulletMarkNoPicMipShader;
			}

			r = rand() & 3;
			if (r == 0)
			{
				sfx = cgs.media.sfx_ric1;
			}
			else if (r == 1)
			{
				sfx = cgs.media.sfx_ric2;
			}
			else
			{
				sfx = cgs.media.sfx_ric3;
			}

			radius = 8;
			break;
	}

	if (sfx)
	{
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
	}

	//
	// create the explosion
	//
	if (allowExplosions)
	{
		if (mod)
		{
			le = CG_MakeExplosion(origin, dir,
			                      mod, shader,
			                      duration, isSprite);
			le->light = light;
			VectorCopy(lightColor, le->lightColor);
			if (weapon == WP_RAILGUN)
			{
				// colorize with client color
				VectorCopy(cgs.clientinfo[clientNum].colors.railCore, le->color);
			}
			else if (weapon == WP_PLASMAGUN)
			{
				vec4_t          plasmaColor;
				qhandle_t       colorShader;

				if (CG_PlasmaColorForOwner(clientNum, plasmaColor))
				{
					colorShader = CG_PlasmaColorExplosionShader();
					if (colorShader)
					{
						le->refEntity.customShader = colorShader;
						le->refEntity.shaderRGBA[0] = (byte)(plasmaColor[0] * 255.0f);
						le->refEntity.shaderRGBA[1] = (byte)(plasmaColor[1] * 255.0f);
						le->refEntity.shaderRGBA[2] = (byte)(plasmaColor[2] * 255.0f);
						le->refEntity.shaderRGBA[3] = 255;
					}
				}
			}
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);   // plasma fades alpha, all others fade color
	if (weapon == WP_RAILGUN)
	{
		float*   color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].colors.railRings;
		CG_ImpactMark(mark, origin, dir, random() * 360, color[0], color[1], color[2], 1, alphaFade, radius, qfalse, qfalse);
	}
	else
	{
		CG_ImpactMark(mark, origin, dir, random() * 360, 1, 1, 1, 1, alphaFade, radius, qfalse, qfalse);
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	CG_Bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch (weapon)
	{
		case WP_GRENADE_LAUNCHER:
		case WP_ROCKET_LAUNCHER:
			CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
			break;
		default:
			break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/
/*
===============
CG_Tracer
===============
*/
void CG_Tracer(vec3_t source, vec3_t dest)
{
	vec3_t      forward, right;
	polyVert_t  verts[4];
	vec3_t      line;
	float       len, begin, end;
	vec3_t      start, finish;
	vec3_t      midpoint;

	// tracer
	VectorSubtract(dest, source, forward);
	len = VectorNormalize(forward);

	// start at least a little ways from the muzzle
	if (len < 100)
	{
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if (end > len)
	{
		end = len;
	}
	VectorMA(source, begin, forward, start);
	VectorMA(source, end, forward, finish);

	line[0] = DotProduct(forward, cg.refdef.viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef.viewaxis[2]);

	VectorScale(cg.refdef.viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef.viewaxis[2], right);
	VectorNormalize(right);

	VectorMA(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	// add the tracer sound
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);

}
/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet(vec3_t start, vec3_t end, int skipNum)
{
	trace_t     tr;
	int         sourceContentType, destContentType;

	CG_Trace(&tr, start, NULL, NULL, end, skipNum, MASK_SHOT);
	if (cg_shotGunTracer.integer)
	{
		if (!(tr.surfaceFlags & SURF_NOIMPACT))
		{
			if (random() < cg_tracerChance.value)
			{
				CG_Tracer(start, tr.endpos);
			}
		}
	}
	sourceContentType = trap_CM_PointContents(start, 0);
	destContentType = trap_CM_PointContents(tr.endpos, 0);

	if (sourceContentType == destContentType)
	{
		if (sourceContentType & CONTENTS_WATER)
		{
			CG_BubbleTrail(start, tr.endpos, 32);
		}
	}
	else if (sourceContentType & CONTENTS_WATER)
	{
		trace_t trace;
		trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(start, trace.endpos, 32);
	}
	else if (destContentType & CONTENTS_WATER)
	{
		trace_t trace;
		trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(tr.endpos, trace.endpos, 32);
	}

	if (tr.surfaceFlags & SURF_NOIMPACT)
	{
		return;
	}

	if (cg_entities[tr.entityNum].currentState.eType == ET_PLAYER)
	{
		CG_MissileHitPlayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	}
	else
	{
		if (tr.surfaceFlags & SURF_METALSTEPS)
		{
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		}
		else
		{
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
		}
	}
}


/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/

const float shotgunPatternMap[32] =
{
	0.25, 0.10, 0.10, 0.25, 0.70, 0.30, 0.30, 0.70,
	-0.25, 0.10, -0.10, 0.25, -0.70, 0.30, -0.30, 0.70,
	0.25, -0.10, 0.10, -0.25, 0.70, -0.30, 0.30, -0.70,
	-0.25, -0.10, -0.10, -0.25, -0.70, -0.30, -0.30, -0.70
};

void CG_ShotgunPattern(float* origin, float* origin2, int seed, int otherEntityNum)
{
	int         i;
	float       r;
	float         u;
	vec3_t      end;
	vec3_t      forward;
	vec3_t      right;
	vec3_t      up;
	int         max;

	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	if (modeShotgunPromode)
	{
		max = modeShotgunNumberOfPellets * 2;
	}
	else
	{
		max = modeShotgunNumberOfPellets;
	}

	for (i = 0; i < max; ++i)
	{
		if (modeShotgunPromode != 0)
		{
			r = 16.0f * modeShotgunKoeff * (shotgunPatternMap[i++] + (Q_crandom(&seed) / 10.0f));
			u = 16.0f * modeShotgunKoeff * (shotgunPatternMap[i] + (Q_crandom(&seed) / 10.0f));
		}
		else
		{
			r = 16.0f * Q_crandom(&seed) * modeShotgunKoeff;
			u = 16.0f * Q_crandom(&seed) * modeShotgunKoeff;
		}

		VectorMA(origin, 8192 * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);
		CG_ShotgunPellet(origin, end, otherEntityNum);
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire(entityState_t* es)
{
	vec3_t  v;
	int     contents;

	VectorSubtract(es->origin2, es->pos.trBase, v);
	VectorNormalize(v);
	VectorScale(v, 32, v);
	VectorAdd(es->pos.trBase, v, v);
	if (cgs.glconfig.hardwareType != GLHW_RAGEPRO && cg_smoke_sg.integer)
	{
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t          up;

		contents = trap_CM_PointContents(es->pos.trBase, 0);
		if (!(contents & CONTENTS_WATER))
		{
			VectorSet(up, 0, 0, 8);
			CG_SmokePuff(v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE,
			             (cg_nomip.integer & 256) ? cgs.media.shotgunSmokePuffNoPicMipShader : cgs.media.shotgunSmokePuffShader);
		}
	}
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum);
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
======================
CG_GetFirstPersonRailgunMuzzlePoint

Calculate railgun muzzle position for first person view using weapon model tags
Same logic as CG_AddPlayerWeapon, just get flash.origin for railgun
======================
*/
qboolean CG_GetFirstPersonRailgunMuzzlePoint(vec3_t muzzle)
{
	refEntity_t hand;
	refEntity_t gun;
	refEntity_t flash;
	orientation_t lerped;
	vec3_t angles;
	weaponInfo_t* weapon;
	centity_t* cent;
	clientInfo_t* ci;
	float fovOffset;

	if (cg.renderingThirdPerson)
	{
		return qfalse;
	}

	cent = &cg.predictedPlayerEntity;
	weapon = &cg_weapons[WP_RAILGUN];

	if (!weapon->registered || !weapon->handsModel || !weapon->weaponModel || !weapon->flashModel)
	{
		return qfalse;
	}

	// Set up hand (minimal setup for tag positioning)
	memset(&hand, 0, sizeof(hand));
	CG_CalculateWeaponPosition(hand.origin, angles);
	fovOffset = (cg_fov.integer > 90) ? -0.2 * (cg_fov.integer - 90) : 0;
	VectorMA(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, (cg_gun_z.value + fovOffset), cg.refdef.viewaxis[2], hand.origin);
	AnglesToAxis(angles, hand.axis);
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
	hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldFrame);
	hand.backlerp = cent->pe.torso.backlerp;
	hand.hModel = weapon->handsModel;

	// Position gun on tag_weapon, then apply railgun offset
	CG_PositionEntityOnTag(&gun, &hand, weapon->handsModel, "tag_weapon");
	trap_R_LerpTag(&lerped, weapon->handsModel, hand.oldframe, hand.frame,
	               1.0f - hand.backlerp, "tag_weapon");
	/* Undo tag Y (already in gun from PositionEntityOnTag), match CG_AddPlayerWeapon cg_gunPos logic */
	VectorMA(gun.origin, -lerped.origin[1], hand.axis[1], gun.origin);
	VectorMA(gun.origin, 0.45, hand.axis[1], gun.origin);
	if (cg_gunPos.integer == 2)
	{
		VectorMA(gun.origin, -lerped.origin[1], hand.axis[1], gun.origin);
	}
	else if (cg_gunPos.integer != 3)
	{
		VectorMA(gun.origin, lerped.origin[1], hand.axis[1], gun.origin);
	}
	gun.frame = hand.frame;
	gun.oldframe = hand.oldframe;
	gun.backlerp = hand.backlerp;
	gun.hModel = weapon->weaponModel;

	// Position flash on tag_flash
	memset(&flash, 0, sizeof(flash));
	flash.hModel = weapon->flashModel;
	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");

	VectorCopy(flash.origin, muzzle);
	VectorMA(muzzle, 2, hand.axis[2], muzzle);
	return qtrue;
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle)
{
	vec3_t      forward;
	centity_t*   cent;
	int         anim;

	if (entityNum == cg.snap->ps.clientNum)
	{
		VectorCopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
		VectorMA(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if (!cent->currentValid)
	{
		return qfalse;
	}

	VectorCopy(cent->currentState.pos.trBase, muzzle);

	AngleVectors(cent->currentState.apos.trBase, forward, NULL, NULL);
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR)
	{
		muzzle[2] += CROUCH_VIEWHEIGHT;
	}
	else
	{
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzle, 14, forward, muzzle);

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum)
{
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t      start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if (sourceEntityNum >= 0 && cg_tracerChance.value > 0)
	{
		if (CG_CalcMuzzlePoint(sourceEntityNum, start))
		{
			sourceContentType = trap_CM_PointContents(start, 0);
			destContentType = trap_CM_PointContents(end, 0);

			// do a complete bubble trail if necessary
			if ((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER))
			{
				CG_BubbleTrail(start, end, 32);
			}
			// bubble trail from water into air
			else if ((sourceContentType & CONTENTS_WATER))
			{
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, 32);
			}
			// bubble trail from air into water
			else if ((destContentType & CONTENTS_WATER))
			{
				trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(trace.endpos, end, 32);
			}

			// draw a tracer
			if (random() < cg_tracerChance.value)
			{
				CG_Tracer(start, end);
			}
		}
	}

	// impact splash and mark
	if (flesh)
	{
		CG_Bleed(end, fleshEntityNum);
	}
	else
	{
		CG_MissileHitWall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);
	}

}
