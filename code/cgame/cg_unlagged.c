/*
===========================================================================
Copyright (C) 2006 Neil Toronto.

This file is part of the Unlagged source code.

Unlagged source code is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Unlagged source code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Unlagged source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

predictedMissile_t cg_predictedMissiles[MAX_PREDICTED_MISSILES];

/*
=======================
CG_PredictWeaponEffects

Draws predicted effects for the railgun, shotgun, and machinegun.  The
lightning gun is done in CG_LightningBolt, since it was just a matter
of setting the right origin and angles.

Also spawns local RL/PG/BFG bolts when server advertises missile predict
(RAT_PREDICTMISSILES + CS_OSP2BE_MISSILE_PREDICT).
=======================
*/
void CG_PredictWeaponEffects(centity_t* cent)
{
	entityState_t* ent = &cent->currentState;

	// if the client isn't us, forget it
	if (cent->currentState.number != cg.predictedPlayerState.clientNum)
	{
		return;
	}


	// was it a rail attack?
	if (ent->weapon == WP_RAILGUN && (cg_delag.integer & 1 || cg_delag.integer & 4) && !cg.demoPlayback)
	{
		vec3_t      muzzlePoint, forward, right, up;
		trace_t trace;
		vec3_t endPoint;
		vec3_t centerPoint;

		// get forward, right, and up vectors (always needed for trace direction)
		AngleVectors(cg.predictedPlayerState.viewangles, forward, right, up);

		// Calculate center point for accurate trace (always from view center for correct endpoint)

		VectorCopy(cg.predictedPlayerState.origin, centerPoint);
		centerPoint[2] += cg.predictedPlayerState.viewheight;
		VectorMA(centerPoint, 14, forward, centerPoint);

		// Trace from center point to get accurate endpoint (where crosshair points)
		VectorMA(centerPoint, 8192, forward, endPoint);
		CG_Trace(&trace, centerPoint, vec3_origin, vec3_origin, endPoint, cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

		// If cg_railFix is enabled, use muzzle position from weapon model for trail start
		if (cg_railFix.integer)
		{
			// get the muzzle point from weapon model tag
			if (!CG_GetFirstPersonRailgunMuzzlePoint(muzzlePoint))
			{
				// Fallback to center point if model tag lookup fails
				VectorCopy(centerPoint, muzzlePoint);
			}
		}
		else
		{
			// Old logic: approximate muzzle position
			VectorCopy(centerPoint, muzzlePoint);
			// do the magic-number adjustment
			VectorMA(muzzlePoint, 4, right, muzzlePoint);
			VectorMA(muzzlePoint, -1, up, muzzlePoint);
		}

		// draw a rail trail from muzzle to accurate endpoint
		CG_RailTrail(&cgs.clientinfo[cent->currentState.clientNum], muzzlePoint, trace.endpos);
		//Com_Printf( "Predicted rail trail\n" );

		// explosion at end if not SURF_NOIMPACT
		if (!(trace.surfaceFlags & SURF_NOIMPACT))
		{
			// predict an explosion
			CG_MissileHitWall(ent->weapon, cg.predictedPlayerState.clientNum, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT);
		}
	}
	else if ((cgs.ratFlags & RAT_PREDICTMISSILES) && cgs.missilePredictEnabled
	         && !cg.demoPlayback
	         && !(cg.snap->ps.pm_flags & PMF_FOLLOW)
	         && (ent->weapon == WP_ROCKET_LAUNCHER
	             || ent->weapon == WP_PLASMAGUN
	             || ent->weapon == WP_BFG))
	{
		vec3_t muzzlePoint, forward, right, up;
		predictedMissile_t* pm;
		int i;
		int speed;
		int fps;
		int ping;
		int lifetime;
		int fireTime;

		if (cg.missileFireValid)
		{
			fireTime = cg.missileFireTime;
			AngleVectors(cg.missileFireAngles, forward, right, up);
			if (cgs.missilePredictFromPos)
			{
				VectorCopy(cg.missileFireOriginNew, muzzlePoint);
			}
			else
			{
				VectorCopy(cg.missileFireOriginOld, muzzlePoint);
			}
			muzzlePoint[2] += cg.missileFireViewHeight;
			cg.missileFireValid = qfalse;
		}
		else
		{
			/* Fallback if predict loop did not stash (should be rare) */
			fireTime = cg.predictedPlayerState.commandTime;
			AngleVectors(cg.predictedPlayerState.viewangles, forward, right, up);
			VectorCopy(cg.predictedPlayerState.origin, muzzlePoint);
			muzzlePoint[2] += cg.predictedPlayerState.viewheight;
		}
		VectorMA(muzzlePoint, 14, forward, muzzlePoint);
		SnapVector(muzzlePoint);

		pm = NULL;
		for (i = 0; i < MAX_PREDICTED_MISSILES; i++)
		{
			if (!cg_predictedMissiles[i].active)
			{
				pm = &cg_predictedMissiles[i];
				break;
			}
		}
		if (!pm)
		{
			/* reuse oldest */
			pm = &cg_predictedMissiles[0];
			for (i = 1; i < MAX_PREDICTED_MISSILES; i++)
			{
				if (cg_predictedMissiles[i].removeTime < pm->removeTime)
				{
					pm = &cg_predictedMissiles[i];
				}
			}
		}

		memset(pm, 0, sizeof(*pm));
		pm->active = qtrue;
		pm->weapon = ent->weapon;
		pm->exploded = qfalse;
		pm->pos.trType = TR_LINEAR;
		VectorCopy(muzzlePoint, pm->pos.trBase);
		VectorCopy(muzzlePoint, pm->lastOrigin);
		pm->pos.trTime = fireTime - cgs.missilePrestep;

		if (ent->weapon == WP_ROCKET_LAUNCHER)
		{
			speed = cgs.missilePredictRlSpeed;
		}
		else if (ent->weapon == WP_PLASMAGUN)
		{
			speed = cgs.missilePredictPgSpeed;
		}
		else
		{
			speed = cgs.missilePredictBfgSpeed;
		}
		VectorScale(forward, speed, pm->pos.trDelta);
		SnapVector(pm->pos.trDelta);

		fps = cgs.sv_fps > 0 ? cgs.sv_fps : 20;
		ping = cg.snap ? cg.snap->ping : 0;
		if (ping < 0)
		{
			ping = 0;
		}
		else if (ping > 999)
		{
			ping = 999;
		}
		lifetime = ping + (int)(1.5f * 1000 / fps);
		pm->removeTime = cg.time + lifetime;
	}
	// do not delag shotgun and machinegun for comaptibility to old OSP servers
}

/*
================
CG_AddPredictedMissiles

Draw local predicted RL/PG/BFG bolts until server entity arrives.
================
*/
void CG_AddPredictedMissiles(void)
{
	int i;
	int fps;
	int timeshift;
	vec3_t origin;
	refEntity_t ent;
	const weaponInfo_t* weapon;
	trace_t tr;

	if (!(cgs.ratFlags & RAT_PREDICTMISSILES) || !cgs.missilePredictEnabled)
	{
		return;
	}

	fps = cgs.sv_fps > 0 ? cgs.sv_fps : 20;
	/* Same +1 server frame as own network missiles (Devotion / cg_projectileNudge bit2) */
	timeshift = 1000 / fps;
	if (cg.nextSnap && cg.snap)
	{
		int serverTick = cg.nextSnap->serverTime - cg.snap->serverTime;

		if (serverTick > 0)
		{
			timeshift = serverTick;
		}
	}

	for (i = 0; i < MAX_PREDICTED_MISSILES; i++)
	{
		predictedMissile_t* pm = &cg_predictedMissiles[i];

		if (!pm->active)
		{
			continue;
		}
		if (pm->removeTime < cg.time)
		{
			pm->active = qfalse;
			continue;
		}

		/* Keep slot alive after predicted explode (suppress duplicate server FX) */
		if (pm->exploded)
		{
			continue;
		}

		BG_EvaluateTrajectory(&pm->pos, cg.time + timeshift, origin);

		CG_Trace(&tr, pm->lastOrigin, NULL, NULL, origin,
		         cg.predictedPlayerState.clientNum, MASK_SHOT);
		if (tr.fraction < 1.0f)
		{
			if (tr.entityNum >= 0 && tr.entityNum < MAX_CLIENTS)
			{
				/* Player hit: bleed + explosion now; hitsound waits for server PS */
				CG_Bleed(tr.endpos, tr.entityNum);
				CG_MissileHitWall(pm->weapon, cg.predictedPlayerState.clientNum,
				                  tr.endpos, tr.plane.normal, IMPACTSOUND_FLESH);
				pm->exploded = qtrue;
				VectorCopy(tr.endpos, pm->explosionOrigin);
				continue;
			}
			if (!(tr.surfaceFlags & SURF_NOIMPACT))
			{
				if (tr.surfaceFlags & SURF_METALSTEPS)
				{
					CG_MissileHitWall(pm->weapon, cg.predictedPlayerState.clientNum,
					                  tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
				}
				else
				{
					CG_MissileHitWall(pm->weapon, cg.predictedPlayerState.clientNum,
					                  tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
				}
				pm->exploded = qtrue;
				VectorCopy(tr.endpos, pm->explosionOrigin);
				continue;
			}
		}

		VectorCopy(origin, pm->lastOrigin);
		weapon = &cg_weapons[pm->weapon];

		if (pm->weapon == WP_PLASMAGUN)
		{
			CG_AddPlasmaMissileScene(origin, cg.predictedPlayerState.clientNum, 0, NULL, qtrue);
			continue;
		}

		if (weapon->missileDlight)
		{
			trap_R_AddLightToScene(origin, weapon->missileDlight,
			                       weapon->missileDlightColor[0],
			                       weapon->missileDlightColor[1],
			                       weapon->missileDlightColor[2]);
		}

		memset(&ent, 0, sizeof(ent));
		VectorCopy(origin, ent.origin);
		VectorCopy(origin, ent.oldorigin);
		ent.reType = RT_MODEL;
		ent.skinNum = cg.clientFrame & 1;
		ent.hModel = weapon->missileModel;
		ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;
		if (VectorNormalize2(pm->pos.trDelta, ent.axis[0]) == 0)
		{
			ent.axis[0][2] = 1;
		}
		RotateAroundDirection(ent.axis, cg.time / 4);
		trap_R_AddRefEntityToScene(&ent);
	}
}

/*
================
CG_Cvar_ClampInt

Clamps a cvar between two integer values, returns qtrue if it had to.
================
*/
qboolean CG_Cvar_ClampInt(const char* name, vmCvar_t* vmCvar, int min, int max)
{
	if (vmCvar->integer > max)
	{
		CG_Printf("Allowed values are %d to %d.\n", min, max);

		Com_sprintf(vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", max);
		vmCvar->value = max;
		vmCvar->integer = max;

		trap_Cvar_Set(name, vmCvar->string);
		return qtrue;
	}

	if (vmCvar->integer < min)
	{
		CG_Printf("Allowed values are %d to %d.\n", min, max);

		Com_sprintf(vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", min);
		vmCvar->value = min;
		vmCvar->integer = min;

		trap_Cvar_Set(name, vmCvar->string);
		return qtrue;
	}

	return qfalse;
}
