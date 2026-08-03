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
// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"
#include "superhud.h"

/* Map weapon index -> exec cvar; no cgs copy, read cvar.string at use */
static const vmCvar_t* const execWeaponCvars[WP_NUM_WEAPONS] =
{
	NULL,
	&cg_exec_gauntlet,
	&cg_exec_mg,
	&cg_exec_sg,
	&cg_exec_gl,
	&cg_exec_rl,
	&cg_exec_lg,
	&cg_exec_rg,
	&cg_exec_pg,
	&cg_exec_bfg,
	NULL,
};

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/

static int lowAmmoThresholds[WP_NUM_WEAPONS] =
{
	/* WP_NONE */            0,
	/* WP_GAUNTLET */        0,
	/* WP_MACHINEGUN */      30,
	/* WP_SHOTGUN */         5,
	/* WP_GRENADE_LAUNCHER */5,
	/* WP_ROCKET_LAUNCHER */ 5,
	/* WP_LIGHTNING */       30,
	/* WP_RAILGUN */         5,
	/* WP_PLASMAGUN */       30,
	/* WP_BFG */             5,
	/* WP_GRAPPLING_HOOK */  0
};

/* CG_CheckAmmo state: file scope so inlined body in CG_TransitionPlayerState can use it */
static int lowAmmoWarningPrev[WP_NUM_WEAPONS] = {0};
static int lastWeapon = WP_NONE;
static int prevWeaponState = WEAPON_READY;
static int prevAmmo = 0;

/* Lightning weapon tracking state (was CG_UpdateWeaponTracking), file scope for CG_UpdateLightningTracking */
static int lastHitsValueLightning = 0;

/* cg_debugServer 2: shot on one weapon -> switch to another (ms) */
static int cg_debugShotTime;
static int cg_debugShotWeapon;

/* cg_debugServer 3: interval between EV_FIRE_WEAPON shots (ms) */
static int cg_debugIntervalLastTime;
static int cg_debugIntervalLastWeapon;

/* cg_debugServer 5: predicted plasma muzzle queue for spawn-offset log */
#define CG_DEBUG_PLASMA_QUEUE 8
#define CG_DEBUG_PLASMA_STALE_MS 1000

typedef struct
{
	vec3_t muzzle;
	int fireTime;
} cg_debugPlasmaFire_t;

static cg_debugPlasmaFire_t cg_debugPlasmaQueue[CG_DEBUG_PLASMA_QUEUE];
static int cg_debugPlasmaHead;
static int cg_debugPlasmaCount;

static void CG_DebugServerClearPlasmaQueue(void)
{
	cg_debugPlasmaHead = 0;
	cg_debugPlasmaCount = 0;
}

static void CG_DebugServerPushPlasmaFire(void)
{
	playerState_t* ps;
	vec3_t forward;
	vec3_t muzzle;
	int slot;

	ps = &cg.predictedPlayerState;
	AngleVectors(ps->viewangles, forward, NULL, NULL);
	VectorCopy(ps->origin, muzzle);
	muzzle[2] += ps->viewheight;
	VectorMA(muzzle, 14, forward, muzzle);

	if (cg_debugPlasmaCount >= CG_DEBUG_PLASMA_QUEUE)
	{
		cg_debugPlasmaHead = (cg_debugPlasmaHead + 1) % CG_DEBUG_PLASMA_QUEUE;
		cg_debugPlasmaCount--;
	}
	slot = (cg_debugPlasmaHead + cg_debugPlasmaCount) % CG_DEBUG_PLASMA_QUEUE;
	VectorCopy(muzzle, cg_debugPlasmaQueue[slot].muzzle);
	cg_debugPlasmaQueue[slot].fireTime = cg.time;
	cg_debugPlasmaCount++;
}

/*
==============
CG_DebugServerOnFireWeapon
==============
*/
void CG_DebugServerOnFireWeapon(int weapon)
{
	if (cg_debugServer.integer == 5)
	{
		cg_debugIntervalLastTime = 0;
		cg_debugIntervalLastWeapon = WP_NONE;
		if (weapon == WP_PLASMAGUN)
		{
			CG_DebugServerPushPlasmaFire();
		}
		return;
	}

	CG_DebugServerClearPlasmaQueue();

	if (cg_debugServer.integer != 3)
	{
		cg_debugIntervalLastTime = 0;
		cg_debugIntervalLastWeapon = WP_NONE;
		return;
	}
	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		return;
	}
	if (cg_debugIntervalLastTime > 0 && weapon == cg_debugIntervalLastWeapon)
	{
		CG_Printf("Shot interval: ^5%s^7 ^5%d^7 ms\n",
		          weaponNames[weapon], cg.time - cg_debugIntervalLastTime);
	}
	cg_debugIntervalLastTime = cg.time;
	cg_debugIntervalLastWeapon = weapon;
}

/*
==============
CG_DebugServerOnMissileAppear
==============
*/
void CG_DebugServerOnMissileAppear(const entityState_t* es)
{
	vec3_t appear;
	vec3_t predMuzzle;
	float distPred;
	float distBase;
	float muzzleDelta;
	int trAge;
	int atTime;

	if (cg_debugServer.integer != 5 || !es || !cg.snap)
	{
		return;
	}
	if (es->eType != ET_MISSILE || es->weapon != WP_PLASMAGUN)
	{
		return;
	}
	if (es->otherEntityNum != cg.predictedPlayerState.clientNum)
	{
		return;
	}

	/* drop stale FIFO entries */
	while (cg_debugPlasmaCount > 0)
	{
		if (cg.time - cg_debugPlasmaQueue[cg_debugPlasmaHead].fireTime <= CG_DEBUG_PLASMA_STALE_MS)
		{
			break;
		}
		cg_debugPlasmaHead = (cg_debugPlasmaHead + 1) % CG_DEBUG_PLASMA_QUEUE;
		cg_debugPlasmaCount--;
	}
	if (cg_debugPlasmaCount <= 0)
	{
		return;
	}

	VectorCopy(cg_debugPlasmaQueue[cg_debugPlasmaHead].muzzle, predMuzzle);
	cg_debugPlasmaHead = (cg_debugPlasmaHead + 1) % CG_DEBUG_PLASMA_QUEUE;
	cg_debugPlasmaCount--;

	/* early-add uses nextSnap; TransitionSnapshot has snap==nextSnap until cleared */
	atTime = cg.nextSnap ? cg.nextSnap->serverTime : cg.snap->serverTime;
	BG_EvaluateTrajectory(&es->pos, atTime, appear);
	distPred = Distance(appear, predMuzzle);
	distBase = Distance(appear, es->pos.trBase);
	muzzleDelta = Distance(es->pos.trBase, predMuzzle);
	trAge = atTime - es->pos.trTime;

	CG_Printf("Plasma offset: appear ^5%.0f^7u from pred, ^5%.0f^7u from trBase, muzzle delta ^5%.0f^7u, trAge ^5%d^7ms ping ^5%d^7\n",
	          distPred, distBase, muzzleDelta, trAge, (int)cgs.osp.pingMs);
}

/*
==============
CG_UpdateLightningTracking
==============
Update lightning weapon stats for HUD when cg_shud is enabled. Caller passes current
player state and pointer to lightning weapon stats; lastHits is updated in place.
*/
static void CG_UpdateLightningTracking(const playerState_t* ps, customWeaponStats_t* ws, int* lastHits)
{
	int wtAmmo, wtHitsDelta, wtAtta, wtDamage, wtActualHits;

	if (!cg_shud.integer)
		return;
	if (ps->weapon != WP_LIGHTNING || ps->stats[STAT_HEALTH] <= 0)
	{
		ws->onTrack = qfalse;
		ws->hitsStart = 0;
		ws->hitsCurrent = 0;
		ws->shotsStart = 0;
		ws->lastAmmo = 0;
		*lastHits = 0;
		return;
	}
	if (ws->onTrack && (cg.time - ws->lastAttackTime > 1000))
	{
		ws->onTrack = qfalse;
		ws->hitsStart = 0;
		ws->hitsCurrent = 0;
		ws->shotsStart = 0;
		ws->lastAmmo = 0;
		*lastHits = 0;
		return;
	}
	wtAmmo = ps->ammo[WP_LIGHTNING];
	wtHitsDelta = ps->persistant[PERS_HITS] - *lastHits;
	wtAtta = ps->persistant[PERS_ATTACKEE_ARMOR];
	if (wtAtta == 0)
	{
		wtActualHits = (wtHitsDelta > 0) ? 1 : 0;
		wtDamage = wtHitsDelta;
	}
	else
	{
		wtActualHits = wtHitsDelta;
		wtDamage = wtAtta & 0x00FF;
	}
	*lastHits = ps->persistant[PERS_HITS];
	if (ws->onTrack)
	{
		if (ps->weaponstate == WEAPON_FIRING)
			ws->lastAttackTime = cg.time;
		ws->hitsCurrent += wtActualHits;
		ws->damageFixed += wtDamage;
		ws->lastAmmo = wtAmmo;
	}
	else
	{
		ws->hitsStart = wtActualHits;
		ws->hitsCurrent = wtActualHits;
		ws->damageFixed = wtDamage;
		ws->shotsStart = wtAmmo;
		ws->lastAmmo = wtAmmo;
		ws->lastAttackTime = cg.time;
		ws->onTrack = qtrue;
	}
}

/*
==============
CG_CheckAmmo
==============
*/
static void CG_CheckAmmo(void)
{
	const playerState_t* ps = &cg.snap->ps;
	int weapon = ps->weapon;
	int ammo, threshold, currentWarning, prevAmmoSaved;

	if (weapon == WP_NONE || !cg_drawAmmoWarning.integer)
	{
		cg.lowAmmoWarning = 0;
		prevWeaponState = WEAPON_READY;
		prevAmmo = 0;
		return;
	}
	ammo = ps->ammo[weapon];
	prevAmmoSaved = prevAmmo;

	if (ps->weaponstate == WEAPON_FIRING && ammo == 0 &&
	        !(prevWeaponState == WEAPON_FIRING && prevAmmo == 0))
	{
		trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
	}
	prevWeaponState = ps->weaponstate;
	prevAmmo = ammo;

	if (cg_drawAmmoWarning.integer != 1)
	{
		cg.lowAmmoWarning = 0;
		return;
	}
	threshold = lowAmmoThresholds[weapon];
	if (ammo > prevAmmoSaved)
	{
		cg.lowAmmoWarning = 0;
		lowAmmoWarningPrev[weapon] = 0;
	}
	currentWarning = (ammo == 0) ? 2 : ((threshold > 0 && ammo <= threshold) ? 1 : 0);
	cg.lowAmmoWarning = currentWarning;
	if (currentWarning != lowAmmoWarningPrev[weapon] || (currentWarning != 0 && weapon != lastWeapon))
	{
		if (currentWarning == 1)
			trap_S_StartLocalSound(cgs.media.lowAmmoSound, CHAN_LOCAL_SOUND);
		else if (currentWarning == 2)
			trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
	}
	lastWeapon = weapon;
	lowAmmoWarningPrev[weapon] = currentWarning;
}

/* CG_ForceExecOnSpectateStart inlined in CG_TransitionPlayerState */

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback(int yawByte, int pitchByte, int damage)
{
	float       left, front, up;
	float       kick;
	int         health;
	float       scale;
	vec3_t      dir;
	vec3_t      angles;
	float       dist;
	float       yaw, pitch;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if (health < 40)
	{
		scale = 1;
	}
	else
	{
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if (yawByte == 255 && pitchByte == 255)
	{
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	}
	else
	{
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors(angles, dir, NULL, NULL);
		VectorSubtract(vec3_origin, dir, dir);

		front = DotProduct(dir, cg.refdef.viewaxis[0]);
		left = DotProduct(dir, cg.refdef.viewaxis[1]);
		up = DotProduct(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength(dir);
		if (dist < 0.1)
		{
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;
		cg.v_dmg_pitch = -kick * front;

		if (front <= 0.1)
		{
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if (cg.damageX > 1.0)
	{
		cg.damageX = 1.0;
	}
	if (cg.damageX < - 1.0)
	{
		cg.damageX = -1.0;
	}

	if (cg.damageY > 1.0)
	{
		cg.damageY = 1.0;
	}
	if (cg.damageY < - 1.0)
	{
		cg.damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if (kick > 10)
	{
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}

/*
================
CG_Respawn

A respawn happened this snapshot. Reads from cgs (respawnWeapon, zoomAutoReset), not vmCvar.
================
*/
void CG_Respawn(void)
{
	cg.thisFrameTeleport = qtrue;
	cg.weaponSelectTime = cg.time;
	cg.weaponSelect = cg_respawnWeapon.integer ? cg_respawnWeapon.integer : cg.snap->ps.weapon;
	if ((cg_zoomAutoReset.integer & 2) && cg.zoomed)
	{
		cg.zoomed = qfalse;
		cg.zoomTime = cg.time;
	}

	/* Removal is done in CG_TeamTaskUpdateFrame (single place) */
	cg.teamMarkers.packLocalDeathNextFrame = qtrue;
}

extern char* eventnames[];

/*
==============
CG_CheckPlayerstateEvents
==============
*/
void CG_CheckPlayerstateEvents(playerState_t* ps, playerState_t* ops)
{
	int         i;
	int         event;
	centity_t*   cent;

	if (ps->externalEvent && ps->externalEvent != ops->externalEvent)
	{
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for (i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++)
	{
		// if we have a new predictable event
		if (i >= ops->eventSequence
		        // or the server told us to play another event instead of a predicted event we already issued
		        // or something the server told us changed our prediction causing a different event
		        || (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS - 1)] != ops->events[i & (MAX_PS_EVENTS - 1)]))
		{

			event = ps->events[ i & (MAX_PS_EVENTS - 1) ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS - 1) ];

			/* Predict path: throttle own jump sound when autojump is active (~10/s) */
			if (event == EV_JUMP && CG_BE_FEATURE_ENABLED(CG_BE_AUTOJUMP))
			{
				static int lastPredictedJumpSoundTime = 0;
				if (cg.time - lastPredictedJumpSoundTime >= 100)
				{
					lastPredictedJumpSoundTime = cg.time;
					CG_EntityEvent(cent, cent->lerpOrigin);
				}
			}
			else
				CG_EntityEvent(cent, cent->lerpOrigin);

			cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS - 1) ] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents(playerState_t* ps)
{
	int i, m = MAX_PS_EVENTS - 1, pm = MAX_PREDICTED_EVENTS - 1;
	centity_t* cent = &cg.predictedPlayerEntity;

	for (i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++)
	{
		if (i >= cg.eventSequence) continue;
		if (i <= cg.eventSequence - MAX_PREDICTED_EVENTS) continue;
		if (ps->events[i & m] == cg.predictableEvents[i & pm]) continue;

		cent->currentState.event = ps->events[i & m];
		cent->currentState.eventParm = ps->eventParms[i & m];
		CG_EntityEvent(cent, cent->lerpOrigin);
		cg.predictableEvents[i & pm] = cent->currentState.event;
		if (cg_showmiss.integer)
			CG_Printf("WARNING: changed predicted event\n");
	}
}

/*
==================
pushReward
==================
*/
static void pushReward(sfxHandle_t sfx, qhandle_t shader, int rewardCount)
{
	if (cg.rewardStack < (MAX_REWARDSTACK - 1))
	{
		cg.rewardStack++;
		cg.rewardSound[cg.rewardStack] = sfx;
		cg.rewardShader[cg.rewardStack] = shader;
		cg.rewardCount[cg.rewardStack] = rewardCount;
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
static void CG_PlayHitSound(sfxHandle_t sound, playerState_t* ps, playerState_t* ops)
{
	if (cg_lightningSilent.integer && ops->weapon == WP_LIGHTNING)
	{
		return;
	}
	if (cg_hitSounds.integer < 0)
	{
		return;
	}

	trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND);
}



/*
==================
CG_DamageSound
==================
*/
int hit_hp;
int hit_ar;
int hit_sum;

void CG_DamageSound(playerState_t* ps, playerState_t* ops)
{
	hit_hp = ps->stats[STAT_HEALTH] - ops->stats[STAT_HEALTH];
	hit_ar = ps->stats[STAT_ARMOR] - ops->stats[STAT_ARMOR];

	if (cg_damageSound.integer && cg.attackerTime != 0)
	{
		hit_sum = hit_hp + hit_ar;

		if (hit_sum <= -3)
		{
			int soundIndex = (hit_sum >= -25) ? 0 :
			                 (hit_sum >= -50) ? 1 :
			                 (hit_sum >= -75) ? 2 : 3;

			trap_S_StartLocalSound(cgs.media.gotDamageSounds[soundIndex], CHAN_LOCAL_SOUND);
		}
	}
}

void CG_HitSound(playerState_t* ps, playerState_t* ops)
{
	static int delayedDmg = 0;
	static int stackedDmg = 0;

	int deltaTime  = cg.time - cgs.osp.lastHitTime;
	int hits    = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];
	int lgcd    = ops->powerups[PW_HASTE] ? 25 : 50; //lg ms cooldown, do we really need 25ms for haste?

	if (!hits && !delayedDmg)
	{
		return;
	}

	if (hits < 0)
	{
		//frendly fire
		cgs.osp.lastHitTime = cg.time;
		cgs.osp.lastHitDamage = 0;
		CG_PlayHitSound(cgs.media.hitTeamSound, ps, ops);
		return;
	}

	//hits > 0 here
	{
		const int atta = ps->persistant[PERS_ATTACKEE_ARMOR];
		int damage;

		if (atta == 0) // osp if zero
		{
			damage = hits;
		}
		else
		{
			damage = atta & 0x00FF;

		}

		if (cg_lightningHitsoundRateFix.integer && ops->weapon == WP_LIGHTNING && deltaTime < lgcd) //no need lg collisions < cooldown ms
		{
			delayedDmg += hits ? damage : 0; // if hit then transfer him to the future
			return;
		}

		if (cg_stackHitSounds.integer && deltaTime < cg_stackHitSoundsTimeout.integer)
		{
			stackedDmg += damage;
			damage = stackedDmg + delayedDmg;
		}
		else
		{
			stackedDmg = 0;
			damage += delayedDmg;
		}
		delayedDmg = 0;

		cgs.osp.lastHitTime = cg.time;
		cgs.osp.lastHitDamage = damage;
		// TODO we need some solution when we take quad cuz sometimes with LG dmg + delayedDMG > 25
		//damage = ops->powerups[PW_QUAD] ? 26 : damage; // for a homogeneous sound with quad

		if ((cgs.damageInfoAllowed
		        && cg_hitSounds.integer) || cgs.osp.server_mode == OSP_SERVER_MODE_PROMODE || cgs.osp.server_mode == OSP_SERVER_MODE_CQ3)
		{
			int index;
			if (damage > 75) index = 3;
			else if (damage > 50) index = 2;
			else if (damage > 25) index = 1;
			else index = 0;

			if (cg_hitSounds.integer > 1)   // reversed: higher damage - higher tone
				index = 3 - index;

			CG_PlayHitSound(cgs.media.hitSounds[index], ps, ops);
		}
		else
		{
			CG_PlayHitSound(cgs.media.hitSound, ps, ops);
		}
	}
}
/*
==================
CG_CheckLocalSounds
==================
*/
void CG_CheckLocalSounds(playerState_t* ps, playerState_t* ops)
{
	int         highScore, reward;
	sfxHandle_t sfx;

	if (ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM])
		return;

	// incoming damage changes
	CG_DamageSound(ps, ops);

	// hit changes
	CG_HitSound(ps, ops);


	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 && ps->stats[STAT_HEALTH] > 0)
		CG_PainEvent(&cg.predictedPlayerEntity, ps->stats[STAT_HEALTH]);

	reward = qfalse;
	if (ps->persistant[PERS_CAPTURES] != ops->persistant[PERS_CAPTURES])
	{
		pushReward(cgs.media.captureAwardSound, cgs.media.medalCapture, ps->persistant[PERS_CAPTURES]);
		reward = qtrue;
		//Com_Printf("capture\n");
	}
	if (ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT])
	{
		sfx = cgs.media.impressiveSound;
		pushReward(sfx, cgs.media.medalImpressive, ps->persistant[PERS_IMPRESSIVE_COUNT]);
		reward = qtrue;
	}
	if (ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT])
	{
		sfx = cgs.media.excellentSound;
		pushReward(sfx, cgs.media.medalExcellent, ps->persistant[PERS_EXCELLENT_COUNT]);
		reward = qtrue;
		//Com_Printf("excellent\n");
	}
	if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT])
	{
		sfx = cgs.media.humiliationSound;
		pushReward(sfx, cgs.media.medalGauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT]);
		reward = qtrue;
		//Com_Printf("guantlet frag\n");
	}
	if (ps->persistant[PERS_DEFEND_COUNT] != ops->persistant[PERS_DEFEND_COUNT])
	{
		pushReward(cgs.media.defendSound, cgs.media.medalDefend, ps->persistant[PERS_DEFEND_COUNT]);
		reward = qtrue;
		//Com_Printf("defend\n");
	}
	if (ps->persistant[PERS_ASSIST_COUNT] != ops->persistant[PERS_ASSIST_COUNT])
	{
		pushReward(cgs.media.assistSound, cgs.media.medalAssist, ps->persistant[PERS_ASSIST_COUNT]);
		reward = qtrue;
		//Com_Printf("assist\n");
	}
	// if any of the player event bits changed
	if (ps->persistant[PERS_PLAYEREVENTS] != ops->persistant[PERS_PLAYEREVENTS])
	{
		if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) !=
		        (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD))
		{
			trap_S_StartLocalSound(cgs.media.deniedSound, CHAN_ANNOUNCER);
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) !=
		         (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD))
		{
			trap_S_StartLocalSound(cgs.media.humiliationSound, CHAN_ANNOUNCER);
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT) !=
		         (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT))
		{
			trap_S_StartLocalSound(cgs.media.holyShitSound, CHAN_ANNOUNCER);
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_KC_CONFIRM) !=
		         (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_KC_CONFIRM))
		{
			if (cgs.killConfirmMode)
			{
				/* Normal shard pickup sound comes from EV_ITEM_PICKUP.
				 * Custom announcer TBD:
				 * trap_S_StartLocalSound(cgs.media.captureYourTeamSound, CHAN_ANNOUNCER);
				 */
				CG_SHUDEventKillConfirm(qtrue);
			}
		}
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_KC_RETURN) !=
		         (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_KC_RETURN))
		{
			if (cgs.killConfirmMode)
			{
				/* Normal shard pickup sound comes from EV_ITEM_PICKUP.
				 * Custom announcer TBD:
				 * trap_S_StartLocalSound(cgs.media.deniedSound, CHAN_ANNOUNCER);
				 */
				CG_SHUDEventKillConfirm(qfalse);
			}
		}
		reward = qtrue;
	}

	// check for flag pickup
	if (cgs.gametype >= GT_TEAM)
	{
		if ((ps->powerups[PW_REDFLAG] != ops->powerups[PW_REDFLAG] && ps->powerups[PW_REDFLAG]) ||
		        (ps->powerups[PW_BLUEFLAG] != ops->powerups[PW_BLUEFLAG] && ps->powerups[PW_BLUEFLAG]) ||
		        (ps->powerups[PW_NEUTRALFLAG] != ops->powerups[PW_NEUTRALFLAG] && ps->powerups[PW_NEUTRALFLAG]))
		{
			trap_S_StartLocalSound(cgs.media.youHaveFlagSound, CHAN_ANNOUNCER);
		}
	}

	if (!reward && !cg.warmup && ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK] && cgs.gametype < GT_TEAM && !cg_noLeadSounds.integer)
	{
		int r = ps->persistant[PERS_RANK];
		int or = ops->persistant[PERS_RANK];
		if (r == 0)
			CG_AddBufferedSound(cgs.media.takenLeadSound);
		else if (r == RANK_TIED_FLAG)
			CG_AddBufferedSound(cgs.media.tiedLeadSound);
		else if (( or & ~RANK_TIED_FLAG) == 0)
			CG_AddBufferedSound(cgs.media.lostLeadSound);
	}

	// timelimit warnings
	if (cgs.timelimit > 0)
	{
		int     msec;

		msec = cg.time - cgs.levelStartTime;
		if (!(cg.timelimitWarnings & 4) && msec > (cgs.timelimit * 60 + 2) * 1000)
		{
			cg.timelimitWarnings |= 1 | 2 | 4;
			trap_S_StartLocalSound(cgs.media.suddenDeathSound, CHAN_ANNOUNCER);
		}
		else if (!(cg.timelimitWarnings & 2) && msec > (cgs.timelimit - 1) * 60 * 1000)
		{
			cg.timelimitWarnings |= 1 | 2;
			trap_S_StartLocalSound(cgs.media.oneMinuteSound, CHAN_ANNOUNCER);
		}
		else if (cgs.timelimit > 5 && !(cg.timelimitWarnings & 1) && msec > (cgs.timelimit - 5) * 60 * 1000)
		{
			cg.timelimitWarnings |= 1;
			trap_S_StartLocalSound(cgs.media.fiveMinuteSound, CHAN_ANNOUNCER);
		}
	}

	// fraglimit warnings
	if (cgs.fraglimit > 0 && cgs.gametype < GT_CTF)
	{
		highScore = cgs.scores1;

		if (cgs.gametype >= GT_TEAM && cgs.scores2 > highScore)
		{
			highScore = cgs.scores2;
		}

		if (!(cg.fraglimitWarnings & 4) && highScore == (cgs.fraglimit - 1))
		{
			cg.fraglimitWarnings |= 1 | 2 | 4;
			CG_AddBufferedSound(cgs.media.oneFragSound);
		}
		else if (cgs.fraglimit > 2 && !(cg.fraglimitWarnings & 2) && highScore == (cgs.fraglimit - 2))
		{
			cg.fraglimitWarnings |= 1 | 2;
			CG_AddBufferedSound(cgs.media.twoFragSound);
		}
		else if (cgs.fraglimit > 3 && !(cg.fraglimitWarnings & 1) && highScore == (cgs.fraglimit - 3))
		{
			cg.fraglimitWarnings |= 1;
			CG_AddBufferedSound(cgs.media.threeFragSound);
		}
	}
}

/*
===============
CG_TransitionPlayerState

One pass over playerstate changes: damage, respawn, zoom, exec (weapon/onDeath),
spectate exec, ammo, events, duck. Change-driven; exec uses cgs snapshot.
===============
*/
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops)
{
	/* check for changing follow mode */
	if (ps->clientNum != ops->clientNum)
	{
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if (ps->damageEvent != ops->damageEvent && ps->damageCount)
	{
		CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);
	}

	/* respawning — sync ops like clientNum so CheckLocalSounds sees no fake deltas across spawn++ */
	if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
	{
		CG_Respawn();
		*ops = *ps;
	}

	if (cg.mapRestart)
	{
		CG_Respawn();
		cg.mapRestart = qfalse;
	}

	if (cg.snap->ps.pm_type != PM_INTERMISSION
	        && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		CG_CheckLocalSounds(ps, ops);
	}

	/* Zoom auto-reset: on death (bit 1) or weapon change (bit 4). Single cvar read. */
	{
		int zoomReset = cg_zoomAutoReset.integer;
		if (cg.zoomed && (((zoomReset & 1) && ops->stats[STAT_HEALTH] > 0 && ps->stats[STAT_HEALTH] <= 0 && cg.snap->ps.stats[STAT_HEALTH] <= 0) || ((zoomReset & 4) && ps->weapon != ops->weapon)))
		{
			cg.zoomed = qfalse;
			cg.zoomTime = cg.time;
		}
	}

	if (cg_debugServer.integer == 2 && ps->clientNum == cg.snap->ps.clientNum)
	{
		if (ps->weaponstate == WEAPON_FIRING && ops->weaponstate != WEAPON_FIRING &&
		        ps->weapon > WP_NONE && ps->weapon < WP_NUM_WEAPONS)
		{
			cg_debugShotTime = cg.time;
			cg_debugShotWeapon = ps->weapon;
		}
		else if (cg_debugShotTime > 0 && ps->weapon != ops->weapon &&
		         ps->weapon > WP_NONE && ops->weapon > WP_NONE &&
		         ps->weapon != cg_debugShotWeapon &&
		         ops->weapon == cg_debugShotWeapon)
		{
			CG_Printf("Shot->switch: ^5%s^7 -> ^5%s^7 ^5%d^7 ms\n",
			          weaponNames[ops->weapon], weaponNames[ps->weapon],
			          cg.time - cg_debugShotTime);
			cg_debugShotTime = 0;
			cg_debugShotWeapon = WP_NONE;
		}
	}
	else if (cg_debugServer.integer != 2)
	{
		cg_debugShotTime = 0;
		cg_debugShotWeapon = WP_NONE;
	}

	if (cg_debugServer.integer != 3)
	{
		cg_debugIntervalLastTime = 0;
		cg_debugIntervalLastWeapon = WP_NONE;
	}

	if (cg_debugServer.integer != 5)
	{
		CG_DebugServerClearPlasmaQueue();
	}

	// Exec on weapon
	if (cg_exec_weapons.integer && (ps->weapon != ops->weapon && ps->weapon != WP_NONE))
	{
		const char* cmd;
		cmd = ps->weapon < WP_NUM_WEAPONS && execWeaponCvars[ps->weapon] ? execWeaponCvars[ps->weapon]->string : NULL;
		if (cmd && cmd[0])
			trap_SendConsoleCommand(cmd);
	}
	// Lightning gun temporal accuracy (cg_shud)
	if (ps->weapon == WP_LIGHTNING)
	{
		CG_UpdateLightningTracking(ps, &cgs.beWeaponStats[WP_LIGHTNING], &lastHitsValueLightning);
	}

	// check for going low on ammo
	CG_CheckAmmo();
	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if (cg_smoothDucking.integer && ps->viewheight != ops->viewheight)
	{
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}
