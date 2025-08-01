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
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail(vec3_t start, vec3_t end, float spacing)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         i;

	if (cg_noProjectileTrail.integer || !cg_bubbleTrail.integer)
	{
		return;
	}
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA(move, i, vec, move);

	VectorScale(vec, spacing, vec);

	for (; i < len; i += spacing)
	{
		localEntity_t*   le;
		refEntity_t*     re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000 + random() * 250;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy(move, le->pos.trBase);
		le->pos.trDelta[0] = crandom() * 5;
		le->pos.trDelta[1] = crandom() * 5;
		le->pos.trDelta[2] = crandom() * 5 + 6;

		VectorAdd(move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t* CG_SmokePuff(const vec3_t p, const vec3_t vel,
                            float radius,
                            float r, float g, float b, float a,
                            float duration,
                            int startTime,
                            int fadeInTime,
                            int leFlags,
                            qhandle_t hShader)
{
	static int  seed = 0x92;
	localEntity_t*   le;
	refEntity_t*     re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random(&seed) * 360;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if (fadeInTime > startTime)
	{
		le->lifeRate = 1.0 / (le->endTime - le->fadeInTime);
	}
	else
	{
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
	}
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy(vel, le->pos.trDelta);
	VectorCopy(p, le->pos.trBase);

	VectorCopy(p, re->origin);
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if (cgs.glconfig.hardwareType == GLHW_RAGEPRO)
	{
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	}
	else
	{
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect(vec3_t org)
{
	localEntity_t*   le;
	refEntity_t*     re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 500;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_MODEL;
	re->shaderTime = cg.time / 1000.0f;

	re->customShader = cgs.media.teleportEffectShader;
	re->hModel = cgs.media.teleportEffectModel;
	AxisClear(re->axis);

	VectorCopy(org, re->origin);
	re->origin[2] -= 24;
}


/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum(int client, vec3_t org, int score)
{
	localEntity_t*   le;
	refEntity_t*     re;
	vec3_t          angles;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_scorePlums.integer == 0)
	{
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 4000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);


	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	VectorCopy(org, le->pos.trBase);
	if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20)
	{
		le->pos.trBase[2] -= 20;
	}

	//CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, lastPos);


	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = 16;

	VectorClear(angles);
	AnglesToAxis(angles, re->axis);
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t* CG_MakeExplosion(vec3_t origin, vec3_t dir,
                                qhandle_t hModel, qhandle_t shader,
                                int msec, qboolean isSprite)
{
	float           ang;
	localEntity_t*   ex;
	int             offset;
	vec3_t          tmpVec, newOrigin;

	if (msec <= 0)
	{
		CG_Error("CG_MakeExplosion: msec = %i", msec);
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if (isSprite)
	{
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale(dir, 16, tmpVec);
		VectorAdd(tmpVec, origin, newOrigin);
	}
	else
	{
		ex->leType = LE_EXPLOSION;
		VectorCopy(origin, newOrigin);

		// set axis with random rotate
		if (!dir)
		{
			AxisClear(ex->refEntity.axis);
		}
		else
		{
			ang = rand() % 360;
			VectorCopy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, ang);
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy(newOrigin, ex->refEntity.origin);
	VectorCopy(newOrigin, ex->refEntity.oldorigin);

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed(vec3_t origin, int entityNum)
{
	localEntity_t* ex;
	int alt = cg_altBlood.integer;

	if (!com_blood.integer)
	{
		return;
	}

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 500;

	VectorCopy(origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = rand() % 360;
	ex->refEntity.radius = 24;

	if (alt)
	{
		if (alt < MAX_ALT_SHADERS)
			ex->refEntity.customShader = cgs.media.bloodExplosionShaderNew[alt - 1];
		else
			ex->refEntity.customShader = cgs.media.bloodExplosionShaderNew[0];

		ex->refEntity.shaderRGBA[0] = cgs.be.altBloodColor[0] * 255;
		ex->refEntity.shaderRGBA[1] = cgs.be.altBloodColor[1] * 255;
		ex->refEntity.shaderRGBA[2] = cgs.be.altBloodColor[2] * 255;
		ex->refEntity.shaderRGBA[3] = cgs.be.altBloodColor[3] * 255;
	}
	else
	{
		if ((cg_nomip.integer & 0x80) == 0)
			ex->refEntity.customShader = cgs.media.bloodExplosionShader;
		else
			ex->refEntity.customShader = cgs.media.bloodExplosionNoPicMipShader;
	}

	// don't show player's own blood in view
	if (entityNum == cg.snap->ps.clientNum)
	{
		ex->refEntity.renderfx |= RF_THIRD_PERSON | RF_DEPTHHACK;
	}
}




/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchGib(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localEntity_t*   le;
	refEntity_t*     re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy(origin, le->pos.trBase);
	VectorCopy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;

	if (cgs.osp.gameTypeFreeze)
	{
		int i;
		centity_t* cent;
		for (i = 0; i < cg.snap->numEntities; ++i)
		{
			cent = &cg_entities[cg.snap->entities[i].number];
			if ((cent->currentState.eventParm == 255) && VectorCompare(cent->lerpOrigin, origin))
			{
				re->customShader = cgs.media.freezeShader;
				break;
			}
		}
	}
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define GIB_VELOCITY    250
#define GIB_JUMP        250
void CG_GibPlayer(vec3_t playerOrigin)
{
	vec3_t  origin, velocity;

	if (!com_blood.integer || !cg_gibs.integer)
	{
		return;
	}

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	if (rand() & 1)
	{
		CG_LaunchGib(origin, velocity, cgs.media.gibSkull);
	}
	else
	{
		CG_LaunchGib(origin, velocity, cgs.media.gibBrain);
	}

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibAbdomen);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibArm);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibChest);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFist);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibFoot);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibForearm);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibIntestine);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * GIB_VELOCITY;
	velocity[1] = crandom() * GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
	CG_LaunchGib(origin, velocity, cgs.media.gibLeg);
}

/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchExplode(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
	localEntity_t*   le;
	refEntity_t*     re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 10000 + random() * 6000;

	VectorCopy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy(origin, le->pos.trBase);
	VectorCopy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

#define EXP_VELOCITY    100
#define EXP_JUMP        150
/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
void CG_BigExplode(vec3_t playerOrigin)
{
	vec3_t  origin, velocity;

	if (!com_blood.integer)
	{
		return;
	}

	VectorCopy(playerOrigin, origin);//-V760
	velocity[0] = crandom() * EXP_VELOCITY;
	velocity[1] = crandom() * EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom() * EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	VectorCopy(playerOrigin, origin);//-V760
	velocity[0] = crandom() * EXP_VELOCITY;
	velocity[1] = crandom() * EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom() * EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * EXP_VELOCITY * 1.5;
	velocity[1] = crandom() * EXP_VELOCITY * 1.5;
	velocity[2] = EXP_JUMP + crandom() * EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * EXP_VELOCITY * 2.0;
	velocity[1] = crandom() * EXP_VELOCITY * 2.0;
	velocity[2] = EXP_JUMP + crandom() * EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);

	VectorCopy(playerOrigin, origin);
	velocity[0] = crandom() * EXP_VELOCITY * 2.5;
	velocity[1] = crandom() * EXP_VELOCITY * 2.5;
	velocity[2] = EXP_JUMP + crandom() * EXP_VELOCITY;
	CG_LaunchExplode(origin, velocity, cgs.media.smoke2);
}

/*
 * Draw an indicator at the screen border
 */
void CG_HudBorderMarker(vec3_t origin, vec4_t color, float radius, qhandle_t shader, int baseAngle)
{
	vec3_t dir;
	float front, left, up;
	vec3_t flu;
	float r, inc, az;
	// half FOV
	float hfov_y = cg.refdef.fov_y * M_PI / 360;
	float hfov_x = cg.refdef.fov_x * M_PI / 360;

	VectorSubtract(origin, cg.refdef.vieworg, dir);

	front = DotProduct(dir, cg.refdef.viewaxis[0]);
	left = DotProduct(dir, cg.refdef.viewaxis[1]);
	up = DotProduct(dir, cg.refdef.viewaxis[2]);

	flu[0] = front;
	flu[1] = left;
	flu[2] = up;

	r = VectorLength(flu);
	az = atan2(flu[1], flu[0]);
	inc = acos(flu[2] / r);
	inc = M_PI / 2.0 - inc;

	if (fabs(az) < hfov_x && fabs(inc) < hfov_y)
	{
		// inside fov, no need to draw marker
		// Note: this isn't entirely accurate
		return;
	}
	else
	{
		refEntity_t ent;
		float phi;
		float dist = 8;
		vec3_t middleOfPlane;
		vec3_t planeAxis[2];
		float ratio;
		float circleSz = 100;
		float r;
		float h1, h2;

		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		phi = atan2(-flu[2], flu[1]);
		ent.rotation = phi / M_PI * 180;
		ent.rotation += baseAngle;

		// make sure it stays the same size regardless of fov
		ent.radius =  0.5 * radius * tan(hfov_x);
		ent.customShader = shader;

		ent.shaderRGBA[0] = color[0] * 255;
		ent.shaderRGBA[1] = color[1] * 255;
		ent.shaderRGBA[2] = color[2] * 255;
		ent.shaderRGBA[3] = color[3] * 127;

		ratio = tan(hfov_y) / tan(hfov_x);

		VectorScale(cg.refdef.viewaxis[0], dist, middleOfPlane);
		r = dist / cos(hfov_x);
		VectorScale(cg.refdef.viewaxis[1], r * sin(hfov_x) - ent.radius, planeAxis[0]);

		r = dist / cos(hfov_y);
		VectorScale(cg.refdef.viewaxis[2], -(r * sin(hfov_y) - ent.radius), planeAxis[1]);

		r = cos(phi) * circleSz;
		VectorScale(planeAxis[0], r, ent.origin);

		r = sin(phi) * circleSz * 1.0 / ratio;
		VectorMA(ent.origin, r, planeAxis[1], ent.origin);

		h1 = fabs(VectorLength(planeAxis[0]) / cos(phi));
		h2 = fabs(VectorLength(planeAxis[1]) / sin(phi));
		if (h2 < h1)
		{
			h1 = h2;
		}
		if (VectorLength(ent.origin) > h1)
		{
			VectorNormalize(ent.origin);
			VectorScale(ent.origin, h1, ent.origin);
		}

		VectorAdd(middleOfPlane, ent.origin, ent.origin);
		VectorAdd(cg.refdef.vieworg, ent.origin, ent.origin);
		trap_R_AddRefEntityToScene(&ent);
	}
}
