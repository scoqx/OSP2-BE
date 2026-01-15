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
#include "cg_superhud.h"


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

void CG_DamagePlum(int client, vec3_t org, int damage)
{
	localEntity_t*   le;
	localEntity_t*   check;
	localEntity_t*   next;
	refEntity_t*     re;
	vec3_t          angles;
	int             totalDamage;
	int             timeThreshold;
	int             eventCount;
	qboolean        isMultiHit;
	static vec3_t lastPos;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_damagePlums.integer == 0)
	{
		return;
	}


	totalDamage = damage;
	timeThreshold = 750; // milliseconds
	isMultiHit = qfalse;

	// If stacking enabled (value = 2), check if multiple events came at the same time
	if (cg_damagePlums.integer == 2)
	{
		// Count how many damage plums were created at the same time (before this one)
		// Events that come at the same time indicate one projectile/rail hitting multiple targets
		eventCount = 0;
		check = cg_activeLocalEntities.prev;
		for (; check != &cg_activeLocalEntities; check = check->prev)
		{
			if (check->leType != LE_DAMAGEPLUM)
			{
				continue;
			}

			// Check if this plum was created at the same time (exact match)
			if (check->startTime == cg.time)
			{
				eventCount++;
			}
		}

		// If more than one event at the same time (including this one), don't sum damage
		// This handles rockets, grenades, plasma, and railgun hitting multiple targets
		if (eventCount > 0)
		{
			isMultiHit = qtrue;
		}

		// If not a multi-hit, proceed with normal stacking
		if (!isMultiHit)
		{
			// Sum all recent damage plums and remove them
			check = cg_activeLocalEntities.prev;
			for (; check != &cg_activeLocalEntities; check = next)
			{
				next = check->prev;

				if (check->leType != LE_DAMAGEPLUM)
				{
					continue;
				}

				// Check if this plum is recent (within time threshold)
				if (cg.time - check->startTime > timeThreshold)
				{
					continue;
				}

				// Sum damage and remove old plum
				totalDamage += (int)check->radius;
				CG_FreeLocalEntity(check);
			}
		}
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_DAMAGEPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);


	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = totalDamage;

	VectorCopy(org, le->pos.trBase);
	if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20)
	{
		le->pos.trBase[2] -= 20;
	}

	//CG_Printf( "Plum origin %i %i %i -- %i\n", (int)org[0], (int)org[1], (int)org[2], (int)Distance(org, lastPos));
	VectorCopy(org, lastPos);

	le->pos.trDelta[0] = 2.0 * crandom();
	le->pos.trDelta[1] = 2.0 * crandom();
	le->pos.trDelta[2] = 6.0;

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->renderfx = RF_FIRST_PERSON;
	re->radius = 11;

	VectorClear(angles);
	AnglesToAxis(angles, re->axis);
}

/*
====================
CG_ChooseDamageColor

Choose color based on damage amount (for mode 3)
====================
*/
static void CG_ChooseDamageColor(int damage, byte* rgba)
{
	if (damage <= 25)
	{
		rgba[0] = 80;
		rgba[1] = 255;
		rgba[2] = 10;
		rgba[3] = 255;
	}
	else if (damage <= 50)
	{
		rgba[0] = 250;
		rgba[1] = 250;
		rgba[2] = 10;
		rgba[3] = 255;
	}
	else if (damage <= 75)
	{
		rgba[0] = 250;
		rgba[1] = 170;
		rgba[2] = 10;
		rgba[3] = 255;
	}
	else if (damage <= 100)
	{
		rgba[0] = 250;
		rgba[1] = 25;
		rgba[2] = 10;
		rgba[3] = 255;
	}
	else if (damage <= 150)
	{
		rgba[0] = 250;
		rgba[1] = 15;
		rgba[2] = 150;
		rgba[3] = 255;
	}
	else if (damage <= 200)
	{
		rgba[0] = 200;
		rgba[1] = 15;
		rgba[2] = 254;
		rgba[3] = 255;
	}
	else
	{
		rgba[0] = 128;
		rgba[1] = 128;
		rgba[2] = 255;
		rgba[3] = 255;
	}
}

/*
====================
CG_GetClientNumByEntityNum

Get client number from entity number in snapshot
====================
*/
static int CG_GetClientNumByEntityNum(int entityNum)
{
	int i;
	centity_t* cent;

	if (!cg.snap)
	{
		return -1;
	}

	for (i = 0; i < cg.snap->numEntities; i++)
	{
		cent = &cg_entities[cg.snap->entities[i].number];
		if (cent->currentState.eType == ET_PLAYER && cent->currentState.number == entityNum)
		{
			return cent->currentState.clientNum;
		}
	}

	return -1;
}

/*
====================
CG_ParseClientDamagePlum

Parse damage from PERS_HITS and find hit position from snapshot events
====================
*/
void CG_ParseClientDamagePlum(playerState_t* ps, playerState_t* ops)
{
	int hits;
	int damage;
	int atta;
	int i;
	centity_t* cent;
	int event;
	vec3_t origin;
	vec3_t traceEnd;
	trace_t trace;
	qboolean foundHit;
	int targetClientNum;


	if (!cg.snap)
	{
		return;
	}

	hits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];

	// Check for friendly fire or no damage
	if (hits <= 0)
	{
		return;
	}

	// Calculate real damage from PERS_ATTACKEE_ARMOR (same logic as CG_HitDamage)
	atta = ps->persistant[PERS_ATTACKEE_ARMOR];
	damage = (atta == 0) ? hits : (atta & 0x00FF);

	// Skip if damage is 0
	if (damage == 0)
	{
		return;
	}

	cgs.be.clientDamagePrinted = qfalse;

	foundHit = qfalse;
	targetClientNum = -1;

	// Debug: weapon info
	if (be_run.integer == 1)
	{
		CG_Printf("^3[DamagePlum]^7 Weapon: %d, Damage: %d, Hits: %d\n", ps->weapon, damage, hits);
	}

	// Search snapshot entities for hit events
	for (i = 0; i < cg.snap->numEntities; i++)
	{
		cent = &cg_entities[cg.snap->entities[i].number];

		if (cent->currentState.eType >= ET_EVENTS)
		{
			event = cent->currentState.eType - ET_EVENTS;
		}
		else
		{
			if (!cent->currentState.event)
			{
				continue;
			}
			event = cent->currentState.event & ~EV_EVENT_BITS;
		}

		// Check for direct hit events
		if (event == EV_MISSILE_HIT || event == EV_BULLET_HIT_FLESH)
		{
			if (be_run.integer == 1)
			{
				CG_Printf("^3[DamagePlum]^7 Found event: %s (entityNum: %d, otherEntityNum: %d)\n",
				          event == EV_MISSILE_HIT ? "EV_MISSILE_HIT" : "EV_BULLET_HIT_FLESH",
				          cent->currentState.number, cent->currentState.otherEntityNum);
			}

			if (cent->currentState.otherEntityNum >= 0 && cent->currentState.otherEntityNum < MAX_GENTITIES)
			{
				targetClientNum = CG_GetClientNumByEntityNum(cent->currentState.otherEntityNum);
				if (be_run.integer == 1)
				{
					CG_Printf("^3[DamagePlum]^7 CG_GetClientNumByEntityNum(%d) = %d\n",
					          cent->currentState.otherEntityNum, targetClientNum);
				}

				if (targetClientNum >= 0)
				{
					// Get player position from centity instead of event position
					centity_t* targetCent = NULL;
					int j;

					// Find player centity by clientNum
					for (j = 0; j < cg.snap->numEntities; j++)
					{
						centity_t* checkCent = &cg_entities[cg.snap->entities[j].number];
						if (checkCent->currentState.eType == ET_PLAYER &&
						        checkCent->currentState.clientNum == targetClientNum)
						{
							targetCent = checkCent;
							break;
						}
					}

					if (targetCent && targetCent->currentState.clientNum == targetClientNum)
					{
						// Use player position with offset above head
						VectorCopy(targetCent->lerpOrigin, origin);
						origin[2] += 40; // Offset above player head
						if (be_run.integer == 1)
						{
							CG_Printf("^3[DamagePlum]^7 Using player position: clientNum=%d, origin=(%.1f,%.1f,%.1f)\n",
							          targetClientNum, origin[0], origin[1], origin[2]);
						}
					}
					else
					{
						// Fallback to event position if player centity not found
						VectorCopy(cent->lerpOrigin, origin);
						if (be_run.integer == 1)
						{
							CG_Printf("^3[DamagePlum]^7 WARNING: Player centity not found, using event position: origin=(%.1f,%.1f,%.1f)\n",
							          origin[0], origin[1], origin[2]);
						}
					}

					foundHit = qtrue;
					break;
				}
				else
				{
					if (be_run.integer == 1)
					{
						CG_Printf("^3[DamagePlum]^7 WARNING: Failed to get clientNum from entityNum %d\n",
						          cent->currentState.otherEntityNum);
					}
				}
			}
		}
		else if (event == EV_RAILTRAIL && cent->currentState.clientNum == ps->clientNum)
		{
			// Railgun hit
			if (be_run.integer == 1)
			{
				CG_Printf("^3[DamagePlum]^7 Found EV_RAILTRAIL event\n");
			}
			if (cent->currentState.origin2[0] != 0 || cent->currentState.origin2[1] != 0 || cent->currentState.origin2[2] != 0)
			{
				VectorCopy(cent->currentState.origin2, origin);
				foundHit = qtrue;
				if (be_run.integer == 1)
				{
					CG_Printf("^3[DamagePlum]^7 Hit determined via EV_RAILTRAIL: origin=(%.1f,%.1f,%.1f)\n",
					          origin[0], origin[1], origin[2]);
				}
				break;
			}
		}
		else if (event == EV_SHOTGUN && cent->currentState.otherEntityNum == ps->clientNum)
		{
			// Shotgun hit
			if (be_run.integer == 1)
			{
				CG_Printf("^3[DamagePlum]^7 Found EV_SHOTGUN event\n");
			}
			VectorCopy(cent->lerpOrigin, origin);
			foundHit = qtrue;
			if (be_run.integer == 1)
			{
				CG_Printf("^3[DamagePlum]^7 Hit determined via EV_SHOTGUN: origin=(%.1f,%.1f,%.1f)\n",
				          origin[0], origin[1], origin[2]);
			}
			break;
		}
	}

	// If no hit event found, use trace from view
	if (!foundHit)
	{
		if (be_run.integer == 1)
		{
			CG_Printf("^3[DamagePlum]^7 No hit event found, using trace from view\n");
		}
		VectorCopy(cg.refdef.vieworg, origin);
		VectorMA(origin, 131072, cg.refdef.viewaxis[0], traceEnd);

		CG_Trace(&trace, origin, vec3_origin, vec3_origin, traceEnd, ps->clientNum, MASK_SHOT);
		VectorCopy(trace.endpos, origin);
		if (be_run.integer == 1)
		{
			CG_Printf("^3[DamagePlum]^7 Hit determined via trace: origin=(%.1f,%.1f,%.1f), entityNum=%d\n",
			          origin[0], origin[1], origin[2], trace.entityNum);
		}

		// Try to get clientNum from trace entity
		if (trace.entityNum >= 0 && trace.entityNum < MAX_GENTITIES)
		{
			targetClientNum = CG_GetClientNumByEntityNum(trace.entityNum);
			if (targetClientNum >= 0)
			{
				if (be_run.integer == 1)
				{
					CG_Printf("^3[DamagePlum]^7 Got clientNum from trace entity: %d\n", targetClientNum);
				}
			}
		}
	}

	// If we have valid targetClientNum, use player position instead of event/trace position
	if (targetClientNum >= 0 && targetClientNum < MAX_CLIENTS)
	{
		centity_t* playerCent = NULL;
		int j;

		// Find player centity by clientNum
		for (j = 0; j < cg.snap->numEntities; j++)
		{
			centity_t* checkCent = &cg_entities[cg.snap->entities[j].number];
			if (checkCent->currentState.eType == ET_PLAYER &&
			        checkCent->currentState.clientNum == targetClientNum)
			{
				playerCent = checkCent;
				break;
			}
		}

		if (playerCent && playerCent->currentState.clientNum == targetClientNum)
		{
			// Use player position with offset above head
			VectorCopy(playerCent->lerpOrigin, origin);
			origin[2] += 40; // Offset above player head
			if (be_run.integer == 1)
			{
				CG_Printf("^3[DamagePlum]^7 Final: Using player position for clientNum=%d: origin=(%.1f,%.1f,%.1f)\n",
				          targetClientNum, origin[0], origin[1], origin[2]);
			}
		}
	}

	// Create damage plum entity
	{
		localEntity_t* le;
		refEntity_t* re;
		byte color[4];

		CG_ChooseDamageColor(damage, color);

		le = CG_AllocLocalEntity();
		le->leFlags = 0;
		le->leType = LE_DAMAGEPLUM;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);

		le->color[0] = color[0] / 255.0f;
		le->color[1] = color[1] / 255.0f;
		le->color[2] = color[2] / 255.0f;
		le->color[3] = color[3] / 255.0f;
		le->radius = damage;

		VectorCopy(origin, le->pos.trBase);

		le->pos.trDelta[0] = 2.0 * crandom();
		le->pos.trDelta[1] = 2.0 * crandom();
		le->pos.trDelta[2] = 6.0;

		re = &le->refEntity;
		re->reType = RT_SPRITE;
		re->renderfx = RF_FIRST_PERSON;
		re->radius = 11;

		cgs.be.clientDamagePrinted = qtrue;

		if (be_run.integer == 1)
		{
			CG_Printf("^3[DamagePlum]^7 Created plum: damage=%d, origin=(%.1f,%.1f,%.1f), targetClientNum=%d, foundHit=%d\n",
			          damage, origin[0], origin[1], origin[2], targetClientNum, foundHit);
		}
	}
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

	if (CG_BE_FEATURE_ENABLED(CG_BE_ALT_BLOOD) && alt)
	{
		vec4_t bloodColor;

		if (alt < MAX_ALT_SHADERS)
			ex->refEntity.customShader = cgs.media.bloodExplosionShaderNew[alt - 1];
		else
			ex->refEntity.customShader = cgs.media.bloodExplosionShaderNew[0];

		Vector4Copy(cgs.be.altBloodColor, bloodColor);

		ex->refEntity.shaderRGBA[0] = bloodColor[0] * 255;
		ex->refEntity.shaderRGBA[1] = bloodColor[1] * 255;
		ex->refEntity.shaderRGBA[2] = bloodColor[2] * 255;
		ex->refEntity.shaderRGBA[3] = bloodColor[3] * 255;
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

	if (!shader)
	{
		return;
	}

	VectorSubtract(origin, cg.refdef.vieworg, dir);

	front = DotProduct(dir, cg.refdef.viewaxis[0]);
	left = DotProduct(dir, cg.refdef.viewaxis[1]);
	up = DotProduct(dir, cg.refdef.viewaxis[2]);

	flu[0] = front;
	flu[1] = left;
	flu[2] = up;

	r = VectorLength(flu);
	if (r < 0.001f)
	{
		return;
	}

	az = atan2(flu[1], flu[0]);
	inc = acos(flu[2] / r);
	inc = M_PI / 2.0 - inc;

	/* Only skip drawing if point is in front of camera AND inside FOV */
	if (front > 0.0f && fabs(az) < hfov_x && fabs(inc) < hfov_y)
	{
		// inside fov, no need to draw marker
		// Note: this isn't entirely accurate
		return;
	}

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

		if (ent.radius < 0.1f)
		{
			ent.radius = 0.1f;
		}

		ent.shaderRGBA[0] = color[0] * 255;
		ent.shaderRGBA[1] = color[1] * 255;
		ent.shaderRGBA[2] = color[2] * 255;
		ent.shaderRGBA[3] = color[3] * 168;

		ratio = tan(hfov_y) / tan(hfov_x);

		VectorScale(cg.refdef.viewaxis[0], dist, middleOfPlane);

		{
			float cosHfovX = cos(hfov_x);
			float cosHfovY = cos(hfov_y);

			r = dist / cosHfovX;
			VectorScale(cg.refdef.viewaxis[1], r * sin(hfov_x) - ent.radius, planeAxis[0]);

			r = dist / cosHfovY;
			VectorScale(cg.refdef.viewaxis[2], -(r * sin(hfov_y) - ent.radius), planeAxis[1]);
		}

		r = cos(phi) * circleSz;
		VectorScale(planeAxis[0], r, ent.origin);

		r = sin(phi) * circleSz * 1.0 / ratio;
		VectorMA(ent.origin, r, planeAxis[1], ent.origin);

		{
			float cosPhi = cos(phi);
			float sinPhi = sin(phi);

			if (fabs(cosPhi) < 0.001f)
			{
				cosPhi = (cosPhi >= 0) ? 0.001f : -0.001f;
			}
			if (fabs(sinPhi) < 0.001f)
			{
				sinPhi = (sinPhi >= 0) ? 0.001f : -0.001f;
			}

			h1 = fabs(VectorLength(planeAxis[0]) / cosPhi);
			h2 = fabs(VectorLength(planeAxis[1]) / sinPhi);
			if (h2 < h1)
			{
				h1 = h2;
			}
			if (VectorLength(ent.origin) > h1)
			{
				VectorNormalize(ent.origin);
				VectorScale(ent.origin, h1, ent.origin);
			}
		}

		VectorAdd(middleOfPlane, ent.origin, ent.origin);
		VectorAdd(cg.refdef.vieworg, ent.origin, ent.origin);

		trap_R_AddRefEntityToScene(&ent);
	}
}

qboolean CG_IsPointOutsideScreen(vec3_t origin, float* outX, float* outY)
{
	float x;
	float y;

	if (!origin)
	{
		return qfalse;
	}

	/* Convert world to screen coordinates */
	if (!CG_WorldToScreen(origin, &x, &y))
	{
		/* Point is behind camera */
		if (outX)
		{
			*outX = 0.0f;
		}
		if (outY)
		{
			*outY = 0.0f;
		}
		return qtrue;
	}

	if (outX)
	{
		*outX = x;
	}
	if (outY)
	{
		*outY = y;
	}

	/* Check if point is on screen */
	/* CG_WorldToScreen returns coordinates relative to full screen (0-640, 0-480) */
	if (x < 0.0f || x >= SCREEN_WIDTH ||
	        y < 0.0f || y >= SCREEN_HEIGHT)
	{
		/* Point is outside screen */
		return qtrue;
	}

	/* Point is on screen */
	return qfalse;
}


void CG_TeamTaskMark(vec3_t coord, int markType, int clientNum, int toClient)
{
	teamTaskMark_t* mark;
	vec3_t center;
	int i;
	centity_t* cent;
	qboolean playerAvailable;

	if (!cg_teamTask.integer)
	{
		return;
	}

	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}

	if (!markType)
	{
		return;
	}


	/* Anti-spam: remove old markers of the same type from the same client */
	/* Show only the last marker of each type from each client */
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
		{
			if (cgs.be.teamMarkers.teamTaskMarks[i].clientNum == clientNum &&
			        cgs.be.teamMarkers.teamTaskMarks[i].markType == markType)
			{
				/* Remove this mark by shifting all subsequent marks */
				for (; i < cgs.be.teamMarkers.numTeamTaskMarks - 1; i++)
				{
					cgs.be.teamMarkers.teamTaskMarks[i] = cgs.be.teamMarkers.teamTaskMarks[i + 1];
				}
				cgs.be.teamMarkers.numTeamTaskMarks--;
				i--; /* Adjust index after removal */
			}
		}
	}

	/* If this is an overhead mark (YES, NO, or MEDIC), remove any existing overhead mark from the same player */
	if (CG_IsTeamTaskOverhead(markType) && clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
		{
			if (cgs.be.teamMarkers.teamTaskMarks[i].clientNum == clientNum && cgs.be.teamMarkers.teamTaskMarks[i].isOverhead)
			{
				/* Remove this mark by shifting all subsequent marks */
				for (; i < cgs.be.teamMarkers.numTeamTaskMarks - 1; i++)
				{
					cgs.be.teamMarkers.teamTaskMarks[i] = cgs.be.teamMarkers.teamTaskMarks[i + 1];
				}
				cgs.be.teamMarkers.numTeamTaskMarks--;
				break;
			}
		}
	}

	/* If this is a HERE_DEAD or WARN_DEAD mark, remove any existing PLAYERDEAD mark from the same player */
	if ((markType == TEAMTASK_HERE_DEAD || markType == TEAMTASK_WARN_DEAD) && clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
		{
			if (cgs.be.teamMarkers.teamTaskMarks[i].clientNum == clientNum &&
			        cgs.be.teamMarkers.teamTaskMarks[i].markType == TEAMTASK_PLAYERDEAD)
			{
				/* Remove this mark by shifting all subsequent marks */
				for (; i < cgs.be.teamMarkers.numTeamTaskMarks - 1; i++)
				{
					cgs.be.teamMarkers.teamTaskMarks[i] = cgs.be.teamMarkers.teamTaskMarks[i + 1];
				}
				cgs.be.teamMarkers.numTeamTaskMarks--;
				break;
			}
		}
	}

	/* Check if we have space for new mark */
	if (cgs.be.teamMarkers.numTeamTaskMarks >= MAX_TEAMTASK_MARKS)
	{
		/* Remove oldest mark */
		for (i = 0; i < MAX_TEAMTASK_MARKS - 1; i++)
		{
			cgs.be.teamMarkers.teamTaskMarks[i] = cgs.be.teamMarkers.teamTaskMarks[i + 1];
		}
		cgs.be.teamMarkers.numTeamTaskMarks = MAX_TEAMTASK_MARKS - 1;
	}

	mark = &cgs.be.teamMarkers.teamTaskMarks[cgs.be.teamMarkers.numTeamTaskMarks];
	cgs.be.teamMarkers.numTeamTaskMarks++;

	/* Store markType directly (no overhead flag anymore) */
	mark->markType = markType;

	/* Check if this is an overhead mark type (YES, NO, MEDIC, or HERE_ALIVE) - if so, track player dynamically */
	playerAvailable = qfalse;
	if (CG_IsTeamTaskOverhead(markType) && clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		/* Special case for local player - use predicted player entity */
		if (clientNum == cg.clientNum)
		{
			cent = &cg.predictedPlayerEntity;
			/* Player is available - use their position above head */
			VectorCopy(cent->lerpOrigin, mark->origin);
			mark->origin[2] += 48.0f; /* Offset above head */
			playerAvailable = qtrue;
		}
		else
		{
			/* Try to find player entity */
			for (i = 0; i < MAX_GENTITIES; i++)
			{
				cent = &cg_entities[i];
				if (cent->currentState.eType == ET_PLAYER &&
				        cent->currentState.clientNum == clientNum &&
				        cent->currentValid)
				{
					/* Player is available - use their position above head */
					VectorCopy(cent->lerpOrigin, mark->origin);
					mark->origin[2] += 48.0f; /* Offset above head */
					playerAvailable = qtrue;
					break;
				}
			}
		}
	}

	/* If player not available or not an overhead mark, use coordinates from message */
	/* Note: WARN_ALIVE coordinates are already set via CG_GetAimContactPoint in cg_players.c */
	if (!playerAvailable)
	{
		VectorCopy(coord, center);
		VectorCopy(center, mark->origin);
	}

	mark->startTime = cg.time;
	mark->endTime = cg.time + cg_teamTaskMarkTime.value;
	mark->clientNum = clientNum; /* Always store sender's clientNum (whose mark it is) */
	mark->isOverhead = CG_IsTeamTaskOverhead(markType);



	CG_GetTeamTaskMarkerShaderAndColor(mark->markType, &mark->shader, mark->color, qfalse, mark->clientNum);

	/* Register team task event for SuperHUD */
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		CG_SHUDEventTeamTask(clientNum, markType);
	}

	/* Play sound when mark appears */
	if (cg_teamTask.integer & CG_TEAMTASK_MARKSOUND)
	{
		if (markType == TEAMTASK_PLAYERDEAD && cgs.media.pingDeathSound)
		{
			trap_S_StartLocalSound(cgs.media.pingDeathSound, CHAN_ANNOUNCER);
		}
		else if (markType == TEAMTASK_YES && cgs.media.pingAcceptSound)
		{
			trap_S_StartLocalSound(cgs.media.pingAcceptSound, CHAN_ANNOUNCER);
		}
		else if (markType == TEAMTASK_NO && cgs.media.pingDenySound)
		{
			trap_S_StartLocalSound(cgs.media.pingDenySound, CHAN_ANNOUNCER);
		}
		else if (cgs.media.pingSimpleSound)
		{
			trap_S_StartLocalSound(cgs.media.pingSimpleSound, CHAN_ANNOUNCER);
		}
	}
}
