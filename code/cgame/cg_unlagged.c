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
#include "../qcommon/qcommon.h"

predictedMissile_t cg_predictedMissiles[MAX_PREDICTED_MISSILES];

/*
=======================
CG_PredictInfoUpdate

What the server lets us predict, from CS 910. The server decides, the client
has no switch of its own. An info string:

  \pr\3\pre\50\rl\900\pg\2000\bfg\2000\seed\1\mg\7\sg\10\lg\8\rg\100

  pr    1 missiles (also needs RAT_PREDICTMISSILES), 2 hitscan hits
  pre   missile prestep, ms: the server starts its missile that far ahead
  rl, pg, bfg   missile speeds
  mz, mzd   where shots start, units in front of the eye, standing and
        crouched. Missing: 14, Quake III. qalive (g_muzzlePointQL 1): 5 / 3.
  seed  1: MG and SG spread is seeded from the command time (cmd serverTime
        & 255), so it can be predicted. 0 or missing: only LG, RG and a
        shotgun pattern with no random part (CPMA, QL) are.
  mg, sg, lg, rg   hitscan damage: MG, SG per pellet, LG, RG, as the server
        deals it (it picks the MG damage by the mode, the client does not).
        For the hit sound and the damage plum. With no pr bit 2 the stock
        values are used (MG 7, 5 in team deathmatch).

The shotgun pattern is not in here: the physics gives it (CS 806, see
CG_ShotgunPatternType), and a Quake Live server says so itself (CS 940).

Shotgun and machinegun (all optional):

  sgo   QL: damage of a middle / outer ring pellet (sg is the inner ring)
  sgf, sgr   QL: falloff, sgf off a pellet per sgr units of range
  mgs, mgsd  MG spread standing and crouched (QL g_ironsights_mg); 200
  qd    quad damage factor; 3

  qalive: \pr\3\...\seed\1\mg\5\mgs\150\mgsd\150\sg\5\sgo\5\sgf\0\sgr\768\lg\6\rg\80\qd\3

A missing key has the stock value. The old form "1 <pre> <rl> <pg> <bfg> ..."
is missiles only, its later fields are ignored.

The handoff shift (CG_MissileHandoffShift) keeps our network missile on the
predicted bolt's clock whatever launch delag the server uses.
=======================
*/
/*
=======================
CG_PredictMuzzleForward

How far in front of the eye the server starts a shot (CS 910 mz / mzd).
=======================
*/
float CG_PredictMuzzleForward(int pmFlags)
{
	return (pmFlags & PMF_DUCKED) ? cgs.muzzleForwardDucked : cgs.muzzleForward;
}

static int CG_PredictInfoInt(const char* info, const char* key, int def)
{
	const char* v = Info_ValueForKey(info, key);

	return v[0] ? atoi(v) : def;
}

void CG_PredictInfoUpdate(void)
{
	const char* str = CG_ConfigString(CS_OSP2BE_PREDICT);
	int pr = 0;

	cgs.missilePredictEnabled = qfalse;
	cgs.missilePrestep = 0;
	cgs.muzzleForward = 14.0f;
	cgs.muzzleForwardDucked = 14.0f;
	cgs.missilePredictRlSpeed = 900;
	cgs.missilePredictPgSpeed = 2000;
	cgs.missilePredictBfgSpeed = 2000;
	cgs.hitscanPredictSeed = qfalse;
	cgs.hitscanDamageMg = 7;
	cgs.hitscanDamageSg = -1;
	cgs.hitscanDamageSgOuter = -1;
	cgs.hitscanSgFalloff = 0;
	cgs.hitscanSgFalloffRange = 0;
	cgs.hitscanDamageLg = 8;
	cgs.hitscanDamageRg = 100;
	cgs.hitscanMgSpread = 200.0f;
	cgs.hitscanMgSpreadDucked = 200.0f;
	cgs.hitscanQuadFactor = 3.0f;

	if (str && str[0] == '\\')
	{
		const char* v;

		cgs.hitscanDamageSgOuter = CG_PredictInfoInt(str, "sgo", -1);
		cgs.hitscanSgFalloff = CG_PredictInfoInt(str, "sgf", 0);
		cgs.hitscanSgFalloffRange = CG_PredictInfoInt(str, "sgr", 0);
		v = Info_ValueForKey(str, "mgs");
		if (v[0])
		{
			cgs.hitscanMgSpread = cgs.hitscanMgSpreadDucked = atof(v);
		}
		v = Info_ValueForKey(str, "mgsd");
		if (v[0])
		{
			cgs.hitscanMgSpreadDucked = atof(v);
		}
		v = Info_ValueForKey(str, "mz");
		if (v[0])
		{
			cgs.muzzleForward = cgs.muzzleForwardDucked = atof(v);
		}
		v = Info_ValueForKey(str, "mzd");
		if (v[0])
		{
			cgs.muzzleForwardDucked = atof(v);
		}
		v = Info_ValueForKey(str, "qd");
		if (v[0])
		{
			cgs.hitscanQuadFactor = atof(v);
		}

		pr = CG_PredictInfoInt(str, "pr", 0);
		cgs.missilePrestep = CG_PredictInfoInt(str, "pre", 0);
		cgs.missilePredictRlSpeed = CG_PredictInfoInt(str, "rl", cgs.missilePredictRlSpeed);
		cgs.missilePredictPgSpeed = CG_PredictInfoInt(str, "pg", cgs.missilePredictPgSpeed);
		cgs.missilePredictBfgSpeed = CG_PredictInfoInt(str, "bfg", cgs.missilePredictBfgSpeed);
		cgs.hitscanPredictSeed = CG_PredictInfoInt(str, "seed", 0) ? qtrue : qfalse;
		cgs.hitscanDamageMg = CG_PredictInfoInt(str, "mg", cgs.hitscanDamageMg);
		cgs.hitscanDamageSg = CG_PredictInfoInt(str, "sg", cgs.hitscanDamageSg);
		cgs.hitscanDamageLg = CG_PredictInfoInt(str, "lg", cgs.hitscanDamageLg);
		cgs.hitscanDamageRg = CG_PredictInfoInt(str, "rg", cgs.hitscanDamageRg);
	}
	else if (str && str[0])
	{
		int enabled = 0, prestep = 0, rl = 0, pg = 0, bfg = 0;

		if (Q_sscanf(str, "%i %i %i %i %i", &enabled, &prestep, &rl, &pg, &bfg) >= 5 && enabled)
		{
			pr = 1;
			cgs.missilePrestep = prestep;
			cgs.missilePredictRlSpeed = rl;
			cgs.missilePredictPgSpeed = pg;
			cgs.missilePredictBfgSpeed = bfg;
		}
	}

	cgs.missilePredictEnabled = (pr & 1) ? qtrue : qfalse;
	cgs.hitscanPredictEnabled = (pr & 2) ? qtrue : qfalse;
	if (!cgs.missilePredictEnabled)
	{
		memset(cg_predictedMissiles, 0, sizeof(cg_predictedMissiles));
	}
}

/*
=======================
CG_ServerTick

The real time between server snapshots. It holds through a gap with no next
snapshot and a dropped one, where the pair alone would give 0 or twice the tick.
sv_fps from systeminfo is the last resort: q3msk runs at 60 and says 20.
=======================
*/
static int cg_tickSnap;
static int cg_tickCand;
static int cg_tickStable;

int CG_ServerTick(void)
{
	int t;

	if (cg.snap && cg.nextSnap && cg.nextSnap->serverTime != cg_tickSnap)
	{
		t = cg.nextSnap->serverTime - cg.snap->serverTime;
		cg_tickSnap = cg.nextSnap->serverTime;
		if (t > 0 && t <= 200)
		{
			if (t == cg_tickCand)
			{
				cg_tickStable = t;
			}
			cg_tickCand = t;
		}
	}
	if (cg_tickStable > 0)
	{
		return cg_tickStable;
	}
	if (cg.snap && cg.nextSnap && cg.nextSnap->serverTime > cg.snap->serverTime)
	{
		return cg.nextSnap->serverTime - cg.snap->serverTime;
	}
	return cgs.sv_fps > 0 ? 1000 / cgs.sv_fps : 50;
}

/*
=======================
Predicted missile handoff

The server fires the missile when the command reaches it, so its trTime is
about a ping later than the predicted one. The network missile of ours keeps
the predicted bolt's clock: it goes on from where that bolt was, not a ping back.
=======================
*/
typedef struct
{
	int trTime;
	int shift;
} missileHandoff_t;

static missileHandoff_t cg_missileHandoff[MAX_GENTITIES];

int CG_MissileHandoffShift(const centity_t* cent)
{
	const missileHandoff_t* h;
	int num = cent->currentState.number;

	if (num < 0 || num >= MAX_GENTITIES)
	{
		return 0;
	}
	h = &cg_missileHandoff[num];
	return (h->trTime && h->trTime == cent->currentState.pos.trTime) ? h->shift : 0;
}

qboolean CG_MissileHandoffDone(const centity_t* cent)
{
	int num = cent->currentState.number;

	if (num < 0 || num >= MAX_GENTITIES)
	{
		return qfalse;
	}
	return (cg_missileHandoff[num].trTime && cg_missileHandoff[num].trTime == cent->currentState.pos.trTime) ? qtrue : qfalse;
}

void CG_MissileHandoffSet(const centity_t* cent, int slot)
{
	int num = cent->currentState.number;
	int shift = cent->currentState.pos.trTime - cg_predictedMissiles[slot].pos.trTime;

	if (num < 0 || num >= MAX_GENTITIES)
	{
		return;
	}
	/* q3msk unlag on: the server's missile is ~10 ms older than ours, the shift is negative */
	if (shift < -200)
	{
		shift = -200;
	}
	else if (shift > 500)
	{
		shift = 500;
	}
	cg_missileHandoff[num].trTime = cent->currentState.pos.trTime;
	cg_missileHandoff[num].shift = shift;
}

#ifdef CG_PREDICT_HITSCAN
/*
=======================
Hitscan hit prediction

The hit sound and the damage plum at the moment of the shot, from what the
client sees, not waiting for the server. The server's sound and plum for the
same hit are then skipped. The server may decide otherwise, and a false hit is
the price.

The server turns it on (CS 910 pr & 2) and gives the damage: then it is on
whatever cg_predictHitscan is, and the plum follows cg_damagePlums. Without the
server cg_predictHitscan 1 forces it, with the stock damage, and
cg_predictHitscan 2 predicts the damage plum as well. LG and RG always,
MG and SG only when it seeds their spread from the command time (seed 1).

The trace is the server's one: the time and the muzzle of the firing command,
the targets where they were drawn on that frame.

cg_debugServer 6 logs predictions and server confirms to the console
(debug build only, CG_DEBUG_SERVER).
=======================
*/
#define PHS_SOUND   1       /* pending: the server's hit sound is still to come */
#define PHS_PLUM    2       /* pending: the server's damage plum is still to come */
#ifdef CG_DEBUG_SERVER
#define PHS_LOG     (cg_debugServer.integer == 6)
#endif

#define PREDICT_HIT_LIGHTNING_RANGE 768
#define PREDICT_HIT_MG_SPREAD       200
#define PREDICT_HIT_CONFIRM_MS      150     /* on top of the ping */
#define PREDICT_HIT_MAX             16
#define PREDICT_HIT_RAIL_MAX        4       /* MAX_RAIL_HITS of the server */

typedef struct
{
	int time;       /* cg.time of the prediction, 0 - free */
	int victim;
	int weapon;
	int damage;
	int pending;    /* PHS_SOUND | PHS_PLUM: the server's one is still to come */
} predictedHit_t;

typedef struct
{
	int    time;    /* cg.time of the frame the pose was drawn on */
	int    solid;
	vec3_t origin;
} predictedPose_t;

static predictedHit_t  cg_predictedHits[PREDICT_HIT_MAX];
static predictedPose_t cg_predictedPoses[MAX_CLIENTS];
#ifdef CG_DEBUG_SERVER
static int cg_phsPredicted, cg_phsConfirmed, cg_phsFalse, cg_phsMissed;
#endif

static int CG_PredictHitscanLerpSolid(int lo, int hi, float f)
{
	int x, zd, zu;

	if (!hi || hi == SOLID_BMODEL || lo == hi)
	{
		return lo;
	}
	x  = (lo & 255) + f * ((hi & 255) - (lo & 255));
	zd = ((lo >> 8) & 255) + f * (((hi >> 8) & 255) - ((lo >> 8) & 255));
	zu = ((lo >> 16) & 255) + f * (((hi >> 16) & 255) - ((lo >> 16) & 255));
	return (zu << 16) | (zd << 8) | x;
}

/*
Where the players were drawn on this frame. The next frame's shot is traced
against these: by then a new snapshot may have moved lerpOrigin already.
*/
/*
The weapons a shot of which is predicted. MG needs the server's seed; SG too,
unless its pattern has no random part at all (CPMA, and the hit rings of QL).
*/
static qboolean CG_PredictHitscanWeapon(int weapon)
{
	if (weapon == WP_LIGHTNING || weapon == WP_RAILGUN)
	{
		return qtrue;
	}
	if (weapon == WP_MACHINEGUN)
	{
		return cgs.hitscanPredictSeed;
	}
	if (weapon == WP_SHOTGUN)
	{
		const int pattern = CG_ShotgunPatternType();

		return (cgs.hitscanPredictSeed || pattern == SGP_CPMA || pattern == SGP_QL) ? qtrue : qfalse;
	}
	return qfalse;
}

/*
Only the players in the snapshot, and only with a weapon we predict. A pose not
saved on the last frame keeps an older time, and the swap leaves it out: the
first shot after a weapon switch is traced against the current positions.
*/
void CG_PredictHitscanSavePoses(void)
{
	int i;
	float f;

	if (!CG_HITSCAN_PREDICT_ON() || cg.demoPlayback || !CG_PredictHitscanWeapon(cg.predictedPlayerState.weapon))
	{
		return;
	}
	f = cg.frameInterpolation;
	if (f < 0.0f)
	{
		f = 0.0f;
	}
	else if (f > 1.0f)
	{
		f = 1.0f;
	}
	for (i = 0; i < cg.snap->numEntities; i++)
	{
		const int        num = cg.snap->entities[i].number;
		const centity_t* cent;
		predictedPose_t* pose;

		if (num >= MAX_CLIENTS)
		{
			continue;
		}
		cent = &cg_entities[num];
		pose = &cg_predictedPoses[num];
		if (!cent->currentValid || cent->currentState.eType != ET_PLAYER || (cent->currentState.eFlags & EF_DEAD) ||
		        !cent->currentState.solid || cent->currentState.solid == SOLID_BMODEL)
		{
			continue;
		}
		VectorCopy(cent->lerpOrigin, pose->origin);
		pose->solid = (cent->interpolate && cg.nextSnap)
		              ? CG_PredictHitscanLerpSolid(cent->currentState.solid, cent->nextState.solid, f)
		              : cent->currentState.solid;
		pose->time = cg.time;
	}
}

/* swap: the saved poses in, and the same call puts the current ones back */
static void CG_PredictHitscanSwapPoses(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		centity_t* cent = &cg_entities[i];
		predictedPose_t* pose = &cg_predictedPoses[i];
		vec3_t org;
		int solid;

		if (pose->time != cg.oldTime || !cent->currentValid)
		{
			continue;
		}
		VectorCopy(cent->lerpOrigin, org);
		solid = cent->currentState.solid;
		VectorCopy(pose->origin, cent->lerpOrigin);
		cent->currentState.solid = pose->solid;
		VectorCopy(org, pose->origin);
		pose->solid = solid;
	}
}

static qboolean CG_PredictHitscanTarget(int num)
{
	const centity_t* target;

	if (num < 0 || num >= MAX_CLIENTS || num == cg.predictedPlayerState.clientNum)
	{
		return qfalse;
	}
	target = &cg_entities[num];
	if (!target->currentValid || target->currentState.eType != ET_PLAYER || (target->currentState.eFlags & EF_DEAD))
	{
		return qfalse;
	}
	return CG_IS_ENEMY_SIMPLE(&cgs.clientinfo[num]) ? qtrue : qfalse;
}

static void CG_PredictHitscanExpire(void)
{
	int i;
	const int window = cg.snap->ping + PREDICT_HIT_CONFIRM_MS;

	for (i = 0; i < PREDICT_HIT_MAX; i++)
	{
		predictedHit_t* hit = &cg_predictedHits[i];

		if (!hit->time || cg.time - hit->time <= window)
		{
			continue;
		}
#ifdef CG_DEBUG_SERVER
		if (hit->pending & PHS_SOUND)
		{
			cg_phsFalse++;
			if (PHS_LOG)
			{
				CG_Printf("^3phs: ^1false^7 wp %i victim %i dmg %i (after %i ms)\n",
				          hit->weapon, hit->victim, hit->damage, cg.time - hit->time);
			}
		}
#endif
		hit->time = 0;
	}
}

/* the oldest hit that still waits for this kind of confirm */
static predictedHit_t* CG_PredictHitscanPending(int kind)
{
	int i;
	predictedHit_t* found = NULL;

	CG_PredictHitscanExpire();
	for (i = 0; i < PREDICT_HIT_MAX; i++)
	{
		predictedHit_t* hit = &cg_predictedHits[i];

		if (hit->time && (hit->pending & kind) && (!found || hit->time < found->time))
		{
			found = hit;
		}
	}
	return found;
}

/* The server's hit sound is about to play. qtrue: we have played it already. */
qboolean CG_PredictHitscanConfirm(void)
{
	predictedHit_t* hit = CG_PredictHitscanPending(PHS_SOUND);

	if (!hit)
	{
#ifdef CG_DEBUG_SERVER
		cg_phsMissed++;
		if (PHS_LOG)
		{
			CG_Printf("^3phs: ^5server hit, not predicted^7 (wp %i)\n", cg.snap->ps.weapon);
		}
#endif
		return qfalse;
	}
	hit->pending &= ~PHS_SOUND;
#ifdef CG_DEBUG_SERVER
	cg_phsConfirmed++;
	if (PHS_LOG)
	{
		CG_Printf("^3phs: ^2ok^7 wp %i victim %i in %i ms  [pred %i ok %i false %i missed %i]\n",
		          hit->weapon, hit->victim, cg.time - hit->time,
		          cg_phsPredicted, cg_phsConfirmed, cg_phsFalse, cg_phsMissed);
	}
#endif
	return qtrue;
}

/* The server's damage plum has come. qtrue: we have drawn it already. */
qboolean CG_PredictHitscanConfirmPlum(int client, int damage)
{
	predictedHit_t* hit;

	if (!CG_HITSCAN_PREDICT_ON() || client != cg.predictedPlayerState.clientNum)
	{
		return qfalse;
	}
	hit = CG_PredictHitscanPending(PHS_PLUM);
	if (!hit)
	{
		return qfalse;
	}
	hit->pending &= ~PHS_PLUM;
#ifdef CG_DEBUG_SERVER
	if ((PHS_LOG) && damage != hit->damage)
	{
		CG_Printf("^3phs: ^6plum^7 wp %i predicted %i, server %i\n", hit->weapon, hit->damage, damage);
	}
#endif
	return qtrue;
}

static void CG_PredictHitscanAdd(int victim, int weapon, int damage)
{
	int i;
	predictedHit_t* hit = &cg_predictedHits[0];
	const predictedPose_t* pose = &cg_predictedPoses[victim];

	CG_PredictHitscanExpire();
	for (i = 0; i < PREDICT_HIT_MAX; i++)
	{
		if (!cg_predictedHits[i].time)
		{
			hit = &cg_predictedHits[i];
			break;
		}
		if (cg_predictedHits[i].time < hit->time)
		{
			hit = &cg_predictedHits[i];
		}
	}
	/* the damage has the quad already (CG_PredictHitscanFire) */
	/* battlesuit: vq3 halves the damage, CPM (promode) and Clan Arena take 75% off */
	if (cg_entities[victim].currentState.powerups & (1 << PW_BATTLESUIT))
	{
		damage = ((cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE) || cgs.gametype == GT_CA) ? damage / 4 : damage / 2;
		if (damage < 1)
		{
			damage = 1;
		}
	}
	hit->time = cg.time;
	hit->victim = victim;
	hit->weapon = weapon;
	hit->damage = damage;
	hit->pending = PHS_SOUND;
#ifdef CG_DEBUG_SERVER
	cg_phsPredicted++;
#endif

	CG_PlayPredictedHitSound(damage);
	/* the plum: when the server turns it on, by cg_damagePlums alone; else only on cg_predictHitscan 2 */
	if ((cgs.hitscanPredictEnabled || cg_predictHitscan.integer >= 2) && (cg_damagePlums.integer & CG_DAMAGEPLUMS_ACTIVE) &&
	        (cg_damagePlumsWeapons.integer & (1 << (weapon - 1))))
	{
		vec3_t org;

		/* the server's place: the origin plus two heights of the box */
		if (pose->time)
		{
			VectorCopy(pose->origin, org);
			org[2] += 2 * (((pose->solid >> 16) & 255) - 32);
		}
		else
		{
			VectorCopy(cg_entities[victim].lerpOrigin, org);
			org[2] += 2 * (((cg_entities[victim].currentState.solid >> 16) & 255) - 32);
		}
		CG_DamagePlum(cg.predictedPlayerState.clientNum, org, damage);
		hit->pending |= PHS_PLUM;
	}
#ifdef CG_DEBUG_SERVER
	if (PHS_LOG)
	{
		CG_Printf("^3phs: predict wp %i victim %i dmg %i\n", weapon, victim, damage);
	}
#endif
}

static void CG_PredictHitscanFire(int weapon)
{
	const playerState_t* ps = &cg.predictedPlayerState;
	vec3_t  forward, right, up, muzzle, end;
	trace_t trace;
	int     fireTime;
	int     viewheight;
	int     i;
	const float quad = ps->powerups[PW_QUAD] ? cgs.hitscanQuadFactor : 1.0f;

	if (!CG_PredictHitscanWeapon(weapon))
	{
		return;
	}
	if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.intermissionStarted ||
	        cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}
	/* no damage there: the freeze round warmup, and the Clan Arena warmup */
	if ((cgs.q3mskRoundWarmup && cgs.roundStartState == 2) || (cgs.gametype == GT_CA && cg.warmup))
	{
		return;
	}

	/* the firing command, as the server will run it */
	if (cg.missileFireValid && cg.missileFireTime <= cg.time && cg.time - cg.missileFireTime < 250)
	{
		fireTime = cg.missileFireTime;
		AngleVectors(cg.missileFireAngles, forward, right, up);
		VectorCopy(cg.missileFireOriginNew, muzzle);
		viewheight = cg.missileFireViewHeight;
	}
	else
	{
		fireTime = cg.oldTime;
		AngleVectors(ps->viewangles, forward, right, up);
		VectorCopy(ps->origin, muzzle);
		viewheight = ps->viewheight;
	}
	cg.missileFireValid = qfalse;
	if (cgs.qlMovement)
	{
		/* qalive CalcMuzzlePointOrigin: from the snapped entity origin */
		SnapVector(muzzle);
	}
	muzzle[2] += viewheight;
	VectorMA(muzzle, CG_PredictMuzzleForward(ps->pm_flags), forward, muzzle);

	CG_PredictHitscanSwapPoses();

	if (weapon == WP_SHOTGUN)
	{
		int damage[MAX_CLIENTS];
		vec3_t dir;

		/* the server sends both through an entity, so they are snapped */
		SnapVector(muzzle);
		if (cgs.qlMovement)
		{
			/* qalive: origin2 = snap(muzzle + forward * 4096), a world point */
			VectorMA(muzzle, 4096, forward, dir);
		}
		else
		{
			VectorScale(forward, 4096, dir);
		}
		SnapVector(dir);
		memset(damage, 0, sizeof(damage));
		/* the quad is in: per pellet, and on QL before the falloff */
		CG_ShotgunPatternDamage(muzzle, dir, fireTime & 255, ps->clientNum, quad, damage);
		CG_PredictHitscanSwapPoses();
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (damage[i] && CG_PredictHitscanTarget(i))
			{
				CG_PredictHitscanAdd(i, weapon, damage[i]);
			}
		}
		return;
	}

	if (weapon == WP_MACHINEGUN)
	{
		int seed = fireTime & 255;
		float r, u;
		const float spread = (ps->pm_flags & PMF_DUCKED) ? cgs.hitscanMgSpreadDucked : cgs.hitscanMgSpread;

		r = Q_random(&seed) * M_PI * 2.0f;
		u = sin(r) * Q_crandom(&seed) * spread * 16;
		r = cos(r) * Q_crandom(&seed) * spread * 16;
		VectorMA(muzzle, 8192 * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);
	}
	else
	{
		VectorMA(muzzle, weapon == WP_RAILGUN ? 8192 : PREDICT_HIT_LIGHTNING_RANGE, forward, end);
	}
	if (weapon == WP_RAILGUN)
	{
		/*
		The rail goes on through the players, as on the server: it takes each
		one out and traces again, up to PREDICT_HIT_RAIL_MAX of them.
		*/
		int    through[PREDICT_HIT_RAIL_MAX];
		float  height[PREDICT_HIT_RAIL_MAX];
		int    count = 0;

		while (count < PREDICT_HIT_RAIL_MAX)
		{
			CG_Trace(&trace, muzzle, vec3_origin, vec3_origin, end, ps->clientNum, MASK_SHOT);
			if (trace.fraction >= 1.0f || trace.entityNum >= MAX_CLIENTS)
			{
				break;
			}
			/* out of the way of the next trace */
			through[count] = trace.entityNum;
			height[count] = cg_entities[trace.entityNum].lerpOrigin[2];
			cg_entities[trace.entityNum].lerpOrigin[2] += 65536.0f;
			count++;
		}
		for (i = 0; i < count; i++)
		{
			cg_entities[through[i]].lerpOrigin[2] = height[i];
		}
		CG_PredictHitscanSwapPoses();
		for (i = 0; i < count; i++)
		{
			if (CG_PredictHitscanTarget(through[i]))
			{
				CG_PredictHitscanAdd(through[i], weapon, (int)(cgs.hitscanDamageRg * quad));
			}
		}
		return;
	}

	CG_Trace(&trace, muzzle, vec3_origin, vec3_origin, end, ps->clientNum, MASK_SHOT);
	CG_PredictHitscanSwapPoses();

	if (trace.fraction < 1.0f && !(trace.surfaceFlags & SURF_NOIMPACT) && CG_PredictHitscanTarget(trace.entityNum))
	{
		int damage;

		if (weapon == WP_LIGHTNING)
		{
			damage = cgs.hitscanDamageLg;
		}
		else
		{
			/* the server's MG damage (CS 910 mg) as it is: the server picks it by the mode; stock without it */
			damage = cgs.hitscanPredictEnabled ? cgs.hitscanDamageMg : ((cgs.gametype == GT_TEAM) ? 5 : 7);
		}
		CG_PredictHitscanAdd(trace.entityNum, weapon, (int)(damage * quad));
	}
}
#endif

/*
=======================
CG_PredictWeaponEffects

Draws predicted effects for the railgun, shotgun, and machinegun.  The
lightning gun is done in CG_LightningBolt, since it was just a matter
of setting the right origin and angles.

Also spawns local RL/PG/BFG bolts when server advertises missile predict
(RAT_PREDICTMISSILES + CS_OSP2BE_PREDICT).
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

#ifdef CG_PREDICT_HITSCAN
	if (CG_HITSCAN_PREDICT_ON())
	{
		CG_PredictHitscanFire(ent->weapon);
	}
#endif


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

		// trail starts at the muzzle tag of the drawn weapon model
		if (!CG_GetFirstPersonRailgunMuzzlePoint(muzzlePoint))
		{
			// Fallback: approximate muzzle position (the old magic-number adjustment)
			VectorCopy(centerPoint, muzzlePoint);
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
			VectorCopy(cg.missileFireOriginNew, muzzlePoint);
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
		VectorMA(muzzlePoint, CG_PredictMuzzleForward(cg.predictedPlayerState.pm_flags), forward, muzzlePoint);
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

		fps = CG_ServerTick();
		ping = cg.snap ? cg.snap->ping : 0;
		if (ping < 0)
		{
			ping = 0;
		}
		else if (ping > 999)
		{
			ping = 999;
		}
		/* until the network missile is here: a ping and some ticks (fps holds the tick) */
		lifetime = ping + 3 * fps + 50;
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
	int timeshift;
	vec3_t origin;
	refEntity_t ent;
	const weaponInfo_t* weapon;
	trace_t tr;

	if (!(cgs.ratFlags & RAT_PREDICTMISSILES) || !cgs.missilePredictEnabled)
	{
		return;
	}

	/* Same +1 server frame as own network missiles (Devotion / cg_projectileNudge bit2) */
	timeshift = CG_ServerTick();

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
		ent.hModel = CG_MISSILE_MODEL(pm->weapon, weapon);
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
