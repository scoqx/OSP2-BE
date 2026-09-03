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
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"
#include "../game/local.h"
#include "../game/bg_local.h"

static  pmove_t     cg_pmove;

static  int         cg_numSolidEntities;
static  centity_t*   cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static  int         cg_numTriggerEntities;
static  centity_t*   cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList(void)
{
	int         i;
	centity_t*   cent;
	snapshot_t*  snap;
	entityState_t*   ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	/* Do not bridge snaps across respawn / follow — OSP oscillates old/new on spawn++ */
	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport
	        && cg.nextSnap->ps.clientNum == cg.snap->ps.clientNum
	        && cg.nextSnap->ps.persistant[PERS_SPAWN_COUNT] == cg.snap->ps.persistant[PERS_SPAWN_COUNT])
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	for (i = 0 ; i < snap->numEntities ; i++)
	{
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if (ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER)
		{
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if (cent->nextState.solid)
		{
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                                  int skipNumber, int mask, trace_t* tr)
{
	int         i, x, zd, zu;
	trace_t     trace;
	entityState_t*   ent;
	clipHandle_t    cmodel;
	vec3_t      bmins, bmaxs;
	vec3_t      origin, angles;
	centity_t*   cent;

	for (i = 0 ; i < cg_numSolidEntities ; i++)
	{
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if (ent->number == skipNumber)
		{
			continue;
		}

		/* g_passPlayers: skip players we may pass through (movement traces only) */
		if ((mask & CONTENTS_BODY) && !(mask & CONTENTS_CORPSE)
		        && ent->number < MAX_CLIENTS
		        && (cgs.ratFlags & (RAT_PASSENEMIES | RAT_PASSALLIES))
		        && (cgs.ratFlags & (RAT_PASSENEMIES | RAT_PASSALLIES)) != (RAT_PASSENEMIES | RAT_PASSALLIES))
		{
			if (cgs.ratFlags & RAT_PASSENEMIES)
			{
				if (CG_IS_ENEMY_SIMPLE(&cgs.clientinfo[ent->number]))
				{
					continue;
				}
			}
			else if (!CG_IS_ENEMY_SIMPLE(&cgs.clientinfo[ent->number]))
			{
				continue;
			}
		}

		if (ent->solid == SOLID_BMODEL)
		{
			// special value for bmodel
			cmodel = trap_CM_InlineModel(ent->modelindex);
			VectorCopy(cent->lerpAngles, angles);
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.physicsTime, origin);
		}
		else
		{
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid >> 8) & 255);
			zu = ((ent->solid >> 16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel(bmins, bmaxs);
			VectorCopy(vec3_origin, angles);
			VectorCopy(cent->lerpOrigin, origin);
		}


		trap_CM_TransformedBoxTrace(&trace, start, end,
		                            mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction)
		{
			trace.entityNum = ent->number;
			*tr = trace;
		}
		else if (trace.startsolid)
		{
			tr->startsolid = qtrue;
		}
		if (tr->allsolid)
		{
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void    CG_Trace(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                 int skipNumber, int mask)
{
	trace_t t;

	trap_CM_BoxTrace(&t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int     CG_PointContents(const vec3_t point, int passEntityNum)
{
	int         i;
	entityState_t*   ent;
	centity_t*   cent;
	clipHandle_t cmodel;
	int         contents;

	contents = trap_CM_PointContents(point, 0);

	for (i = 0 ; i < cg_numSolidEntities ; i++)
	{
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if (ent->number == passEntityNum)
		{
			continue;
		}

		if (ent->solid != SOLID_BMODEL)   // special value for bmodel
		{
			continue;
		}

		cmodel = trap_CM_InlineModel(ent->modelindex);
		if (!cmodel)
		{
			continue;
		}

		contents |= trap_CM_TransformedPointContents(point, cmodel, ent->origin, ent->angles);
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState(qboolean grabAngles)
{
	float           f;
	int             i;
	playerState_t*   out;
	snapshot_t*      prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if (grabAngles)
	{
		usercmd_t   cmd;
		int         cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd(cmdNum, &cmd);

		PM_UpdateViewAngles(out, &cmd);
	}

	// if the next frame is a teleport, we can't lerp to it
	if (cg.nextFrameTeleport)
	{
		return;
	}

	if (!next || next->serverTime <= prev->serverTime)
	{
		return;
	}

	f = (float)(cg.time - prev->serverTime) / (next->serverTime - prev->serverTime);

	i = next->ps.bobCycle;
	if (i < prev->ps.bobCycle)
	{
		i += 256;       // handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * (i - prev->ps.bobCycle);

	for (i = 0 ; i < 3 ; i++)
	{
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i]);
		if (!grabAngles)
		{
			out->viewangles[i] = LerpAngle(
			                         prev->ps.viewangles[i], next->ps.viewangles[i], f);
		}
		out->velocity[i] = prev->ps.velocity[i] +
		                   f * (next->ps.velocity[i] - prev->ps.velocity[i]);
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem(centity_t* cent)
{
	gitem_t*     item;
	static int pickupEnt[8];
	static int pickupTime[8];
	static int pickupSlot;
	int i;
	int n;

	if (!cg_predictItems.integer)
	{
		return;
	}

	// Keep predicted pickups hidden after snapshot overwrites eFlags; no second EV_ITEM_PICKUP
	n = cent->currentState.number;
	for (i = 0; i < 8; i++)
	{
		if (pickupTime[i] && pickupEnt[i] == n && pickupTime[i] <= cg.time && cg.time - pickupTime[i] < 1000)
		{
			cent->currentState.eFlags |= EF_NODRAW;
			cent->nextState.eFlags |= EF_NODRAW;
			return;
		}
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}
	if (!BG_PlayerTouchesItem(&cg.predictedPlayerState, &cent->currentState, cg.time))
	{
		return;
	}

	// never pick an item up twice in a prediction
	if (cent->miscTime == cg.time)
	{
		return;
	}

	if (!BG_CanItemBeGrabbed(cgs.gametype, &cent->currentState, &cg.predictedPlayerState, qfalse))
	{
		return;     // can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// Special case for flags.
	// We don't predict touching our own flag
	if (cgs.gametype == GT_CTF)
	{
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
		        item->giTag == PW_REDFLAG)
			return;
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
		        item->giTag == PW_BLUEFLAG)
			return;
	}

	// grab it
	BG_AddPredictableEventToPlayerstate(EV_ITEM_PICKUP, cent->currentState.modelindex, &cg.predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;
	cent->nextState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;
	pickupEnt[pickupSlot] = n;
	pickupTime[pickupSlot] = cg.time;
	pickupSlot = (pickupSlot + 1) & 7;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if (item->giType == IT_WEAPON)
	{
		cg.predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if (!cg.predictedPlayerState.ammo[ item->giTag ])
		{
			cg.predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction(void)
{
	int         i;
	trace_t     trace;
	entityState_t*   ent;
	clipHandle_t cmodel;
	centity_t*   cent;
	qboolean    spectator;

	// dead clients don't activate triggers
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	spectator = (cg.predictedPlayerState.pm_type == PM_SPECTATOR);

	if (cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator)
	{
		return;
	}

	if (cg_numTriggerEntities > MAX_ENTITIES_IN_SNAPSHOT)
	{
		CG_Printf("CG_TouchTriggerPrediction() error: cg_numTriggerEntities overflow\n");
		cg_numTriggerEntities = MAX_ENTITIES_IN_SNAPSHOT;
	}

	for (i = 0 ; i < cg_numTriggerEntities; i++)
	{
		if (cg_triggerEntities[i] == NULL)
		{
			CG_Printf("CG_TouchTriggerPrediction() error: cg_triggerEntities[%d] is NULL\n", i);
			continue;
		}
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if (ent->eType == ET_ITEM && !spectator)
		{
			CG_TouchItem(cent);
			continue;
		}

		if (ent->solid != SOLID_BMODEL)
		{
			continue;
		}

		cmodel = trap_CM_InlineModel(ent->modelindex);
		if (!cmodel)
		{
			continue;
		}

		trap_CM_BoxTrace(&trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin,
		                 cg_pmove.mins, cg_pmove.maxs, cmodel, -1);

		if (!trace.startsolid)
		{
			continue;
		}

		if (ent->eType == ET_TELEPORT_TRIGGER)
		{
			cg.hyperspace = qtrue;
		}
		else if (ent->eType == ET_PUSH_TRIGGER)
		{
			BG_TouchJumpPad(&cg.predictedPlayerState, ent);
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if (cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount)
	{
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}



/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/

static vec_t VectorDistance(vec3_t a, vec3_t b)
{
	return (vec_t)sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

/*
=================
CG_ApplyPmoveMovementFields

OSP: pmove_accurate 0/1/2 from systeminfo.
CS912/baseq3a: lock_fps from CS912; local pmove_accurate > 0 overrides as FPS lock (baseq3a parity).
=================
*/
static void CG_ApplyPmoveMovementFields(pmove_t* pm)
{
	/* UnFreeZe path: no OSP lock_fps / accurate dither — only SnapVector if uf_baseq3 */
	if (cgs.fpsclassico)
	{
		pm->pmove_lock_fps = 0;
		pm->pmove_accurate = 0;
		return;
	}

	if (cgs.beMovementLockCsPresent && cgs.pmoveMode == PMODE_OSP)
	{
		trap_Cvar_Update(&pmove_accurate);
		if (pmove_accurate.integer > 0)
		{
			/* client override: numeric FPS like CS 912 (e.g. 115) */
			pm->pmove_lock_fps = pmove_accurate.integer;
			pm->pmove_accurate = 0;
		}
		else
		{
			pm->pmove_lock_fps = cgs.beMovementLockFps;
			pm->pmove_accurate = 0;
		}
	}
	else
	{
		pm->pmove_lock_fps = 0;
		pm->pmove_accurate = (cgs.pmoveMode == PMODE_OSP) ? cgs.sv_pmove_accurate : 0;
	}
}

/*
=================
CG_PmovePhysicsUpdate

Select client predict physics branch once per serverinfo / phy / CS912 change.
=================
*/
void CG_PmovePhysicsUpdate(void)
{
	static int last_phy_movetype = -2;
	static int last_g_snapVectors = -2;
	static int last_mode = PMODE_OSP;
	int         mode;
	int         movetype;
	const char* info;
	const char* val;

	/* Prefer SYSTEMINFO key over stale local cvar (metarena phy_movetype=0).
	 * Do not re-read movetype from vmCvar right after trap_Cvar_Set — Update can lag
	 * one frame and briefly fall into PMODE_OSP with fake CS806 PROMODEbit. */
	info = CG_ConfigString(CS_SYSTEMINFO);
	val = Info_ValueForKey(info, "phy_movetype");
	if (val[0])
	{
		movetype = atoi(val);
		if (phy_movetype.integer != movetype)
		{
			trap_Cvar_Set("phy_movetype", va("%i", movetype));
		}
	}
	else
	{
		trap_Cvar_Update(&phy_movetype);
		movetype = phy_movetype.integer;
	}

	trap_Cvar_Update(&phy_snapVectors);

	if (movetype >= 0)
	{
		mode = PMODE_OSDF;
	}
	else
	{
		mode = PMODE_OSP;
	}

	cgs.pmoveMode = mode;
	/* q3msk lock: full stock baseq3a pmove + EXTFL NOOB landing */
	pmode_q3msk_lock = (cgs.q3msk && cgs.beMovementLockCsPresent && !cgs.fpsclassico) ? 1 : 0;
	pmode_q3msk = cgs.q3msk ? 1 : 0;
	/* metarena / OSDF: phy path still calls shared PM_WaterMove/PM_FlyMove — keep them vanilla */
	pmode_osdf = (mode == PMODE_OSDF) ? 1 : 0;

	if (mode == PMODE_OSDF)
	{
		if (last_phy_movetype < 0 || last_mode != PMODE_OSDF)
		{
			CG_PmovePhysicsResetBaseGlobals();
		}
		if (last_phy_movetype != movetype || last_g_snapVectors != phy_snapVectors.integer)
		{
			phy_invalidate();
		}
		cgs.pmove_phy_movetype = movetype;
	}
	else
	{
		if (last_mode != PMODE_OSP || last_mode == PMODE_BASEQ3A)
		{
			CG_OSPSetMode(cgs.osp.server_mode);
		}
		/* q3msk stock pmove: CS806 may look like PROMODEbut water/friction are vanilla.
		 * Wrong pm_waterfriction (0.5 vs 1.0) causes ~0.1 ankle-water predict drift. */
		if (pmode_q3msk || pmode_q3msk_lock)
		{
			CG_PmovePhysicsResetBaseGlobals();
		}
		cgs.pmove_phy_movetype = -1;
	}

	last_mode = mode;
	last_phy_movetype = movetype;
	last_g_snapVectors = phy_snapVectors.integer;
}

/*
=================
CG_CheckTimers

Mega health/armor countdown and predicted powerup expiry (baseq3a).
=================
*/
static void CG_CheckTimers(void)
{
	int i;

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	if (cg.timeResidual && cg.predictedPlayerState.commandTime >= cg.timeResidual && !cg.thisFrameTeleport)
	{
		cg.timeResidual += 1000;
		if (cg.predictedPlayerState.powerups[PW_REGEN])
		{
			int maxhealth = cg.predictedPlayerState.stats[STAT_MAX_HEALTH];

			if (cg.predictedPlayerState.stats[STAT_HEALTH] < maxhealth)
			{
				cg.predictedPlayerState.stats[STAT_HEALTH] += 15;
				if (cg.predictedPlayerState.stats[STAT_HEALTH] > maxhealth * 1.1f)
				{
					cg.predictedPlayerState.stats[STAT_HEALTH] = (int)(maxhealth * 1.1f);
				}
			}
			else if (cg.predictedPlayerState.stats[STAT_HEALTH] < maxhealth * 2)
			{
				cg.predictedPlayerState.stats[STAT_HEALTH] += 5;
				if (cg.predictedPlayerState.stats[STAT_HEALTH] > maxhealth * 2)
				{
					cg.predictedPlayerState.stats[STAT_HEALTH] = maxhealth * 2;
				}
			}
		}
		else if (cg.predictedPlayerState.stats[STAT_HEALTH] > cg.predictedPlayerState.stats[STAT_MAX_HEALTH])
		{
			cg.predictedPlayerState.stats[STAT_HEALTH]--;
		}

		if (cg.predictedPlayerState.stats[STAT_ARMOR] > cg.predictedPlayerState.stats[STAT_MAX_HEALTH])
		{
			cg.predictedPlayerState.stats[STAT_ARMOR]--;
		}
	}

	for (i = 0; i < MAX_POWERUPS; i++)
	{
		if (!cg.predictedPlayerState.powerups[i])
		{
			continue;
		}
		if (cg.predictedPlayerState.powerups[i] < cg.predictedPlayerState.commandTime)
		{
			cg.predictedPlayerState.powerups[i] = 0;
		}
	}
}

/*
=================
CG_IsUnacceptableError

Compare snapshot playerState to a previously predicted state for incremental
prediction (cg_optimizePrediction). Return 0 if differences are acceptable.
=================
*/
static int CG_IsUnacceptableError(playerState_t* ps, playerState_t* pps, qboolean* forceMove)
{
	vec3_t delta;
	int i, n, v0;

	if (pps->pm_time != ps->pm_time ||
	        pps->pm_type != ps->pm_type ||
	        pps->pm_flags != ps->pm_flags)
	{
		return 1;
	}

	VectorSubtract(pps->origin, ps->origin, delta);
	if (VectorLengthSquared(delta) > (cgs.fpsclassico ? 0.0625f : (0.01f * 0.01f)))
	{
		if (cg_showmiss.integer > 2)
		{
			CG_Printf("origin delta: %.2f  ", VectorLength(delta));
		}
		return 2;
	}

	VectorSubtract(pps->velocity, ps->velocity, delta);
	if (VectorLengthSquared(delta) > (cgs.fpsclassico ? 0.0625f : (0.01f * 0.01f)))
	{
		if (cg_showmiss.integer > 2)
		{
			CG_Printf("velocity delta: %.2f  ", VectorLength(delta));
		}
		return 3;
	}

	if (pps->weaponTime != ps->weaponTime ||
	        pps->gravity != ps->gravity ||
	        pps->speed != ps->speed ||
	        pps->delta_angles[0] != ps->delta_angles[0] ||
	        pps->delta_angles[1] != ps->delta_angles[1] ||
	        pps->delta_angles[2] != ps->delta_angles[2] ||
	        pps->groundEntityNum != ps->groundEntityNum)
	{
		if (cg_showmiss.integer > 1)
		{
			CG_Printf("%i %i %i %i => %i %i %i %i",
			          pps->weaponTime, pps->gravity, pps->speed, pps->groundEntityNum,
			          ps->weaponTime, ps->gravity, ps->speed, ps->groundEntityNum);
		}
		return 4;
	}

	if (pps->torsoAnim != ps->torsoAnim && (ps->torsoAnim & ~ANIM_TOGGLEBIT) == TORSO_GESTURE)
	{
		for (n = 0; n < NUM_SAVED_STATES; n++)
		{
			cg.savedPmoveStates[n].torsoAnim = ps->torsoAnim;
			cg.savedPmoveStates[n].torsoTimer = ps->torsoTimer;
		}
	}

	if (pps->legsTimer != ps->legsTimer || pps->legsAnim != ps->legsAnim ||
	        pps->torsoTimer != ps->torsoTimer || pps->torsoAnim != ps->torsoAnim ||
	        pps->movementDir != ps->movementDir)
	{
		return 5;
	}

	VectorSubtract(pps->grapplePoint, ps->grapplePoint, delta);
	if (VectorLengthSquared(delta) > 0.01f * 0.01f)
	{
		return 6;
	}

	if (pps->eFlags != ps->eFlags)
	{
		if (cg_showmiss.integer > 1)
		{
			CG_Printf("eFlags %i => %i", pps->eFlags, ps->eFlags);
		}
		return 7;
	}

	if (pps->eventSequence != ps->eventSequence)
	{
		return 8;
	}

	for (i = 0; i < MAX_PS_EVENTS; i++)
	{
		if (pps->events[i] != ps->events[i])
		{
			if (cg_showmiss.integer > 1)
			{
				CG_Printf("event[%i] %i => %i\n", i, pps->events[i], ps->events[i]);
			}
			return 9;
		}
		if (pps->eventParms[i] != ps->eventParms[i])
		{
			if (cg_showmiss.integer > 1)
			{
				CG_Printf("eventParms[%i] %i => %i\n", i, pps->eventParms[i], ps->eventParms[i]);
			}
			return 9;
		}
	}

	if (pps->externalEvent != ps->externalEvent ||
	        pps->externalEventParm != ps->externalEventParm ||
	        pps->externalEventTime != ps->externalEventTime)
	{
		return 10;
	}

	if (pps->clientNum != ps->clientNum ||
	        pps->weapon != ps->weapon ||
	        pps->weaponstate != ps->weaponstate)
	{
		return 11;
	}

	if (fabs(AngleDelta(ps->viewangles[0], pps->viewangles[0])) > 1.0f ||
	        fabs(AngleDelta(ps->viewangles[1], pps->viewangles[1])) > 1.0f ||
	        fabs(AngleDelta(ps->viewangles[2], pps->viewangles[2])) > 1.0f)
	{
		return 12;
	}

	if (pps->viewheight != ps->viewheight)
	{
		return 13;
	}

	if (pps->damageEvent != ps->damageEvent ||
	        pps->damageYaw != ps->damageYaw ||
	        pps->damagePitch != ps->damagePitch ||
	        pps->damageCount != ps->damageCount)
	{
		if (cg_showmiss.integer > 1)
		{
			CG_Printf("dmg %i %i %i %i >= %i %i %i %i\n",
			          pps->damageEvent, pps->damageYaw, pps->damagePitch, pps->damageCount,
			          ps->damageEvent, ps->damageYaw, ps->damagePitch, ps->damageCount);
		}
		return 14;
	}

	/* mega health/armor 1Hz countdown: resync saved states (baseq3a) */
	if (pps->stats[STAT_HEALTH] == ps->stats[STAT_HEALTH] + 1
	        && ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
	{
		cg.timeResidual = ps->commandTime + 1000;
		for (n = 0; n < NUM_SAVED_STATES; n++)
		{
			cg.savedPmoveStates[n].stats[STAT_HEALTH] = ps->stats[STAT_HEALTH];
		}
	}

	if (pps->stats[STAT_ARMOR] == ps->stats[STAT_ARMOR] - 1
	        && ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH])
	{
		cg.timeResidual = ps->commandTime + 1000;
		for (n = 0; n < NUM_SAVED_STATES; n++)
		{
			cg.savedPmoveStates[n].stats[STAT_ARMOR] = ps->stats[STAT_ARMOR];
		}
	}

	for (i = 0; i < MAX_STATS; i++)
	{
		if (i == STAT_CLIENTS_READY)
		{
			for (n = 0; n < NUM_SAVED_STATES; n++)
			{
				cg.savedPmoveStates[n].stats[i] = ps->stats[i];
			}
			continue;
		}
		/* BASEQ3A / q3msk / OSDF: ignore timers that churn or are 16-bit truncated on the wire */
		if ((cgs.beMovementLockCsPresent || cgs.pmove_phy_movetype >= 0) &&
		        (i == STAT_OSP_PHYS || i == STAT_OSP_10 || i == STAT_WEAPON_DELAY || i == STAT_TIME_LASTJUMP))
		{
			for (n = 0; n < NUM_SAVED_STATES; n++)
			{
				cg.savedPmoveStates[n].stats[i] = ps->stats[i];
			}
			continue;
		}
		/* q3msk: client-predicted overbounce flag; server never sends it in snapshots */
		if (cgs.q3msk && (cgs.ratFlags & RAT_NOOVERBOUNCE) && i == STAT_EXTFLAGS)
		{
			continue;
		}
		if (pps->stats[i] != ps->stats[i])
		{
			if (cg_showmiss.integer > 2)
			{
				CG_Printf("stats[%i] %i => %i ", i, pps->stats[i], ps->stats[i]);
			}
			return 15;
		}
	}

	for (i = 0; i < MAX_PERSISTANT; i++)
	{
		if (pps->persistant[i] != ps->persistant[i])
		{
			if (i >= PERS_TEAM && i <= PERS_PLAYEREVENTS)
			{
				if (cg_showmiss.integer > 1)
				{
					CG_Printf("persistant[%i] %i => %i ", i, pps->persistant[i], ps->persistant[i]);
				}
				return 16;
			}
			v0 = ps->persistant[i];
			for (n = 0; n < NUM_SAVED_STATES; n++)
			{
				cg.savedPmoveStates[n].persistant[i] = v0;
			}
			*forceMove = qtrue;
		}
	}

	for (i = 0; i < MAX_WEAPONS; i++)
	{
		if (pps->ammo[i] != ps->ammo[i])
		{
			if (cg_showmiss.integer > 1)
			{
				CG_Printf("ammo[%i] %i => %i ", i, pps->ammo[i], ps->ammo[i]);
			}
			return 18;
		}
	}

	if (pps->generic1 != ps->generic1 || pps->loopSound != ps->loopSound)
	{
		return 19;
	}

	for (i = 0; i < MAX_POWERUPS; i++)
	{
		if (pps->powerups[i] != ps->powerups[i])
		{
			if (cg_showmiss.integer > 1)
			{
				CG_Printf("powerups[%i] %i => %i ", i, pps->powerups[i], ps->powerups[i]);
			}
			return 20;
		}
	}

	return 0;
}

void CG_PredictPlayerState(void)
{
	int         cmdNum, current;
	playerState_t   oldPlayerState;
	qboolean    moved;
	usercmd_t   oldestCmd;
	usercmd_t   latestCmd;
	int         stateIndex = 0, predictCmd = 0;

	cg.hyperspace = qfalse; // will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if (!cg.validPPS)
	{
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}


	// demo playback just copies the moves
	if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		CG_InterpolatePlayerState(qfalse);
		return;
	}

	// non-predicting local movement will grab the latest angles
	if (cg_nopredict.integer || cg_synchronousClients.integer)
	{
		CG_InterpolatePlayerState(qtrue);
		return;
	}

	/* Keep phy_movetype / OSDF branch in sync with SYSTEMINFO (metarena) */
	CG_PmovePhysicsUpdate();

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	if (cg_pmove.ps->pm_type == PM_DEAD)
	{
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else
	{
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		cg_pmove.tracemask &= ~CONTENTS_BODY;   // spectators can fly through bodies
	}
	if (cgs.ratFlags & RAT_NOINVISWALLS)
	{
		cg_pmove.tracemask &= ~CONTENTS_PLAYERCLIP;
	}
	if ((cgs.ratFlags & (RAT_PASSENEMIES | RAT_PASSALLIES)) == (RAT_PASSENEMIES | RAT_PASSALLIES))
	{
		cg_pmove.tracemask &= ~CONTENTS_BODY;
	}
	/* Metarena maps VQ3/CPM → CS806 "1" (OSP PROMODEbit). Server has no noFootsteps
	 * and always emits CrashLand footsteps — PROMODEbit must not silence them on OSDF. */
	if (cgs.pmove_phy_movetype >= 0)
	{
		cg_pmove.noFootsteps = (cgs.dmflags & DF_NO_FOOTSTEPS) > 0;
	}
	else
	{
		cg_pmove.noFootsteps = (cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE) || (cgs.dmflags & DF_NO_FOOTSTEPS) > 0;
	}

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &oldestCmd);
	if (oldestCmd.serverTime > cg.snap->ps.commandTime
	        && oldestCmd.serverTime < cg.time)      // special check for map_restart
	{
		if (cg_showmiss.integer)
		{
			CG_Printf("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd(current, &latestCmd);

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	/* Same SPAWN_COUNT/clientNum as snap — avoid predict thrash across freeze spawn++ */
	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport
	        && cg.nextSnap->ps.clientNum == cg.snap->ps.clientNum
	        && cg.nextSnap->ps.persistant[PERS_SPAWN_COUNT] == cg.snap->ps.persistant[PERS_SPAWN_COUNT])
	{
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	}
	else
	{
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	/* OSDF CPM/CQ3: STAT_TIME_LASTJUMP is absolute serverTime but stats[] are
	 * networked as signed 16-bit — expand so DJ / air-step timers match the server. */
	if (cgs.pmove_phy_movetype == CPM || cgs.pmove_phy_movetype == CQ3)
	{
		int last = cg.predictedPlayerState.stats[STAT_TIME_LASTJUMP];
		int now = cg.predictedPlayerState.commandTime;
		int age = now - last;
		int exp;

		if (last > 0 && (age < 0 || age >= 60000))
		{
			exp = (now & ~0xFFFF) | (last & 0xFFFF);
			if (exp > now)
			{
				exp -= 0x10000;
			}
			if (now - exp > 0x8000)
			{
				exp += 0x10000;
			}
			if (exp > now)
			{
				exp -= 0x10000;
			}
			cg.predictedPlayerState.stats[STAT_TIME_LASTJUMP] = exp;
		}
	}

	if (cgs.fpsclassico)
	{
		/* UnFreeZe clamps pmove_msec to 1..33 */
		if (pmove_msec.integer < 1)
		{
			trap_Cvar_Set("pmove_msec", "1");
		}
		else if (pmove_msec.integer > 33)
		{
			trap_Cvar_Set("pmove_msec", "33");
		}
	}
	else if (pmove_msec.integer < 8)
	{
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33)
	{
		trap_Cvar_Set("pmove_msec", "33");
	}

	/*
	 * OSDF / metarena (baseq3a+phy) and q3msk lock: chop frames like the server
	 * (SYSTEMINFO pmove_fixed/msec). Local pmove_fixed on OSP-only servers stays.
	 * Metarena has no pmove_float — keep float only when systeminfo says so.
	 */
	cg_pmove.pmove_float = pmove_float.integer;
	if (cgs.pmove_phy_movetype >= 0
	        || (cgs.q3msk && cgs.beMovementLockCsPresent && !cgs.fpsclassico))
	{
		cg_pmove.pmove_fixed = cgs.pmove_fixed;
		cg_pmove.pmove_msec = cgs.pmove_msec;
		if (cgs.pmove_phy_movetype >= 0)
		{
			/* metarena / OSDF servers: no pmove_float in game */
			cg_pmove.pmove_float = 0;
		}
	}
	else
	{
		cg_pmove.pmove_fixed = pmove_fixed.integer;
		cg_pmove.pmove_msec = pmove_msec.integer;
	}
	CG_ApplyPmoveMovementFields(&cg_pmove);
	cg_pmove.pmove_ratflags = cgs.ratFlags;

	cg_pmove.pmove_movement = MOVEMENT_VQ3;
	if (cgs.beMovementLockCsPresent && !cgs.fpsclassico)
	{
		cg_pmove.pmove_autohop = 0;
	}
	else if (cgs.beAutohop == 0)
	{
		cg_pmove.pmove_autohop = 0;
	}
	else if (cgs.beAutohop == 1)
	{
		cg_pmove.pmove_autohop = 1;
	}
	else
	{
		cg_pmove.pmove_autohop = cg_autoHop.integer;
	}

	moved = qfalse;

	/*
	 * Incremental predict (UnFreeZe / unlagged cg_optimizePrediction):
	 * between snapshots replay saved playerStates; Pmove only new cmds.
	 */
	if (cg_optimizePrediction.integer)
	{
		if (cg.nextFrameTeleport || cg.thisFrameTeleport)
		{
			cg.lastPredictedCommand = 0;
			cg.stateTail = cg.stateHead;
			predictCmd = current - CMD_BACKUP + 1;
		}
		else if (cg.physicsTime == cg.lastServerTime)
		{
			predictCmd = cg.lastPredictedCommand + 1;
		}
		else
		{
			int i;
			int errorcode;
			qboolean error = qtrue;

			for (i = cg.stateHead; i != cg.stateTail; i = (i + 1) % NUM_SAVED_STATES)
			{
				if (cg.savedPmoveStates[i].commandTime != cg.predictedPlayerState.commandTime)
				{
					continue;
				}
				errorcode = CG_IsUnacceptableError(&cg.predictedPlayerState, &cg.savedPmoveStates[i], &moved);
				if (errorcode)
				{
					if (cg_showmiss.integer > 2)
					{
						CG_Printf("errorcode %d at %d\n", errorcode, cg.time);
					}
					break;
				}

				*cg_pmove.ps = cg.savedPmoveStates[i];
				cg.stateHead = (i + 1) % NUM_SAVED_STATES;
				predictCmd = cg.lastPredictedCommand + 1;
				error = qfalse;
				break;
			}

			if (error)
			{
				cg.lastPredictedCommand = 0;
				cg.stateTail = cg.stateHead;
				predictCmd = current - CMD_BACKUP + 1;
			}
		}
		cg.lastServerTime = cg.physicsTime;
		stateIndex = cg.stateHead;
	}

	cmdNum = current - CMD_BACKUP + 1;
	if (cmdNum < 0)
	{
		cmdNum = 0;
	}

	for (; cmdNum <= current; cmdNum++)
	{
		// get the command
		trap_GetUserCmd(cmdNum, &cg_pmove.cmd);

		if (cg_pmove.pmove_fixed || cg_oversampleMouse.integer)
		{
			PM_UpdateViewAngles(cg_pmove.ps, &cg_pmove.cmd);
		}

		// don't do anything if the time is before the snapshot player time
		if (cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime)
		{
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if (cg_pmove.cmd.serverTime > latestCmd.serverTime)
		{
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if (cg.predictedPlayerState.commandTime == oldPlayerState.commandTime)
		{
			vec3_t  delta;
			float   len;

			if (cg.thisFrameTeleport)
			{
				// a teleport will not cause an error decay
				VectorClear(cg.predictedError);
				if (cg_showmiss.integer)
				{
					CG_Printf("PredictionTeleport\n");
				}
				cg.thisFrameTeleport = qfalse;
			}
			else
			{
				vec3_t  adjusted;
				vec3_t  adjustedAngles;
				CG_AdjustPositionForMover(cg.predictedPlayerState.origin,
				                          cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted,
				                          cg.predictedPlayerState.viewangles, adjustedAngles);

				if (cg_showmiss.integer)
				{
					if (!VectorCompare(oldPlayerState.origin, adjusted))
					{
						float   err;

						VectorSubtract(oldPlayerState.origin, adjusted, delta);
						err = VectorLength(delta);
						/* skip float dust; keep any real miss visible at showmiss 1 */
						if (err > 0.1f)
						{
							CG_Printf("prediction error %.3f\n", err);
						}
						/* dxyz/pmtrail for land/stair misses (~0.4+), not only >1 */
						if (cg_showmiss.integer >= 2 && err > 0.3f)
						{
							CG_Printf("  dxyz: %.3f %.3f %.3f  predO: %.1f %.1f %.1f  snapO: %.1f %.1f %.1f\n",
							          delta[0], delta[1], delta[2],
							          adjusted[0], adjusted[1], adjusted[2],
							          oldPlayerState.origin[0], oldPlayerState.origin[1], oldPlayerState.origin[2]);
							CG_Printf("  predV: %.1f %.1f %.1f  snapV: %.1f %.1f %.1f  gnd=%i/%i lock=%i noob=%i q3msk=%i osdf=%i phy=%i\n",
							          cg.predictedPlayerState.velocity[0],
							          cg.predictedPlayerState.velocity[1],
							          cg.predictedPlayerState.velocity[2],
							          oldPlayerState.velocity[0],
							          oldPlayerState.velocity[1],
							          oldPlayerState.velocity[2],
							          cg.predictedPlayerState.groundEntityNum,
							          oldPlayerState.groundEntityNum,
							          cg_pmove.pmove_lock_fps,
							          (cgs.ratFlags & RAT_NOOVERBOUNCE) ? 1 : 0,
							          pmode_q3msk_lock,
							          pmode_osdf,
							          cgs.pmove_phy_movetype);
						}
					}
				}
				VectorSubtract(oldPlayerState.origin, adjusted, delta);
				len = VectorLength(delta);
				if (len > 0.1)
				{
					if (cg_showmiss.integer)
					{
						CG_Printf("Prediction miss: %f\n", len);
					}
					if (cg_errorDecay.integer)
					{
						int     t;
						float   f;

						t = cg.time - cg.predictedErrorTime;
						f = (cg_errorDecay.value - t) / cg_errorDecay.value;
						if (f < 0)
						{
							f = 0;
						}
						if (f > 0 && cg_showmiss.integer)
						{
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale(cg.predictedError, f, cg.predictedError);
					}
					else
					{
						VectorClear(cg.predictedError);
					}
					VectorAdd(delta, cg.predictedError, cg.predictedError);
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if (cg_pmove.pmove_fixed)
		{
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + cg_pmove.pmove_msec - 1) / cg_pmove.pmove_msec) * cg_pmove.pmove_msec;
		}

		if (!cg_optimizePrediction.integer ||
		        cmdNum >= predictCmd ||
		        (stateIndex + 1) % NUM_SAVED_STATES == cg.stateHead)
		{
			qboolean stashFire = qfalse;
			int oldEventSeq = 0;

			/* Missile predict: stash muzzle only on attack frames for RL/PG/BFG */
			if ((cgs.ratFlags & RAT_PREDICTMISSILES) && cgs.missilePredictEnabled
			        && (cg_pmove.cmd.buttons & BUTTON_ATTACK))
			{
				int w = cg.predictedPlayerState.weapon;

				if (w == WP_ROCKET_LAUNCHER || w == WP_PLASMAGUN || w == WP_BFG)
				{
					VectorCopy(cg.predictedPlayerState.origin, cg.missileFireOriginOld);
					oldEventSeq = cg.predictedPlayerState.eventSequence;
					stashFire = qtrue;
				}
			}

			// run the Pmove
			if (cgs.pmove_phy_movetype >= 0)
			{
				cg_pmove.movetype = cgs.pmove_phy_movetype;
				PmoveEx(&cg_pmove, phy_PmoveSingle);
			}
			else
			{
				cg_pmove.movetype = 0;
				memset(&cg_pmove.pmd, 0, sizeof(cg_pmove.pmd));
				Pmove(&cg_pmove);
			}

			if (stashFire && cg.predictedPlayerState.eventSequence != oldEventSeq)
			{
				int ev = cg.predictedPlayerState.events[oldEventSeq & (MAX_PS_EVENTS - 1)];

				if (ev != EV_FIRE_WEAPON
				        && cg.predictedPlayerState.eventSequence > oldEventSeq + 1)
				{
					ev = cg.predictedPlayerState.events[(oldEventSeq + 1) & (MAX_PS_EVENTS - 1)];
				}
				if (ev == EV_FIRE_WEAPON)
				{
					VectorCopy(cg.predictedPlayerState.origin, cg.missileFireOriginNew);
					VectorCopy(cg.predictedPlayerState.viewangles, cg.missileFireAngles);
					cg.missileFireViewHeight = cg.predictedPlayerState.viewheight;
					cg.missileFireTime = cg_pmove.cmd.serverTime;
					cg.missileFireValid = qtrue;
				}
			}

			CG_TouchTriggerPrediction();
			/* baseq3a megaHP/armor timers — not in UnFreeZe cgame predict */
			if (!cgs.fpsclassico)
			{
				CG_CheckTimers();
			}

			if (cg_optimizePrediction.integer)
			{
				cg.lastPredictedCommand = cmdNum;

				if ((stateIndex + 1) % NUM_SAVED_STATES != cg.stateHead)
				{
					cg.savedPmoveStates[stateIndex] = *cg_pmove.ps;
					stateIndex = (stateIndex + 1) % NUM_SAVED_STATES;
					cg.stateTail = stateIndex;
				}
			}
		}
		else
		{
			*cg_pmove.ps = cg.savedPmoveStates[stateIndex];
			stateIndex = (stateIndex + 1) % NUM_SAVED_STATES;
		}

		moved = qtrue;

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
	}

	if (cg_showmiss.integer > 3)
	{
		CG_Printf("[%i : %i] ", cg_pmove.cmd.serverTime, cg.time);
	}

	if (!moved)
	{
		if (cg_showmiss.integer)
		{
			CG_Printf("not moved\n");
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover(cg.predictedPlayerState.origin,
	                          cg.predictedPlayerState.groundEntityNum,
	                          cg.physicsTime, cg.time, cg.predictedPlayerState.origin,
	                          cg.predictedPlayerState.viewangles, cg.predictedPlayerState.viewangles);

	if (cg_showmiss.integer)
	{
		if (cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS)
		{
			CG_Printf("WARNING: dropped event\n");
		}
	}

	// fire events and other transition triggered things
	CG_TransitionPlayerState(&cg.predictedPlayerState, &oldPlayerState);

	if (cg_showmiss.integer)
	{
		if (cg.eventSequence > cg.predictedPlayerState.eventSequence)
		{
			CG_Printf("WARNING: double event\n");
			cg.eventSequence = cg.predictedPlayerState.eventSequence;
		}
	}
}
