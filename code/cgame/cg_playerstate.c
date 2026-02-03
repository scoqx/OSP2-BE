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
 * Inline weapon exec: no function call. cg_exec_weapons.integer and cvar.string read directly (no cgs copy).
 * _w weapon index, _c clientNum.
 */
#define CG_EXEC_WEAPON_SWITCH_INLINE(_w, _c) do { \
    const char* _cmd; \
    if (!cg_exec_weapons.integer || (cg_exec_weapons.integer == 2 && (_c) != cg.clientNum)) break; \
    _cmd = (unsigned)(_w) < (unsigned)WP_NUM_WEAPONS && execWeaponCvars[_w] ? execWeaponCvars[_w]->string : NULL; \
    if (_cmd && _cmd[0]) trap_SendConsoleCommand(_cmd); \
} while (0)

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

/* Lightning weapon tracking state (was CG_UpdateWeaponTracking), file scope for inlined body */
static int lastHitsValueLightning = 0;

/* CG_ForceExecOnSpectateStart inlined in CG_TransitionPlayerState */

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
	cgs.teamMarkers.packLocalDeathNextFrame = qtrue;
}

extern char* eventnames[];

/*
==============
CG_CheckPlayerstateEvents
==============
*/
void CG_CheckPlayerstateEvents(playerState_t* ps, playerState_t* ops)
{
	int         i = 0;
	int         event = 0;
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

#define PUSH_REWARD(sfx, shader, count) do { \
    if (cg.rewardStack < (MAX_REWARDSTACK - 1)) { \
        cg.rewardStack++; \
        cg.rewardSound[cg.rewardStack] = (sfx); \
        cg.rewardShader[cg.rewardStack] = (shader); \
        cg.rewardCount[cg.rewardStack] = (count); \
    } \
} while(0)
#define PLAY_HIT_SOUND(snd, ops) \
    do { if (!(cg_lightningSilent.integer && (ops)->weapon == WP_LIGHTNING) && cg_hitSounds.integer >= 0) trap_S_StartLocalSound((snd), CHAN_LOCAL_SOUND); } while(0)

/* CG_HitDamage inlined in CG_TransitionPlayerState */


/* CG_HitSound inlined in CG_CheckLocalSounds; state for delayed/stacked LG */
static int clsDelayedDmg = 0;
static int clsStackedDmg = 0;

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
	if (cg.intermissionStarted)
		return;

	/* DamageSound inlined */
	{
		int hp = ps->stats[STAT_HEALTH] - ops->stats[STAT_HEALTH];
		int ar = ps->stats[STAT_ARMOR] - ops->stats[STAT_ARMOR];
		if (cg_damageSound.integer && cg.attackerTime != 0)
		{
			int sum = hp + ar;
			if (sum <= -3)
			{
				int si = (sum >= -25) ? 0 : (sum >= -50) ? 1 : (sum >= -75) ? 2 : 3;
				trap_S_StartLocalSound(cgs.media.gotDamageSounds[si], CHAN_LOCAL_SOUND);
			}
		}
	}

	/* HitSound inlined */
	{
		int dt = cg.time - cgs.osp.lastHitTime;
		int hits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];
		int lgcd = ops->powerups[PW_HASTE] ? 25 : 50;
		if (hits || clsDelayedDmg)
		{
			if (hits < 0)
			{
				cgs.osp.lastHitTime = cg.time;
				PLAY_HIT_SOUND(cgs.media.hitTeamSound, ops);
			}
			else
			{
				int atta = ps->persistant[PERS_ATTACKEE_ARMOR];
				int damage = (atta == 0) ? hits : (atta & 0x00FF);
				if (cg_lightningHitsoundRateFix.integer && ops->weapon == WP_LIGHTNING && dt < lgcd)
					clsDelayedDmg += hits ? damage : 0;
				else
				{
					int dmgInfo = cgs.damageInfoAllowed;
					if (dmgInfo && cg_stackHitSounds.integer && dt < cg_stackHitSoundsTimeout.integer)
					{
						clsStackedDmg += damage;
						damage = clsStackedDmg + clsDelayedDmg;
					}
					else
					{
						clsStackedDmg = 0;
						damage += clsDelayedDmg;
					}
					clsDelayedDmg = 0;
					cgs.osp.lastHitTime = cg.time;
					if (dmgInfo && cg_hitSounds.integer)
					{
						int idx = (damage > 75) ? 3 : (damage > 50) ? 2 : (damage > 25) ? 1 : 0;
						if (cg_hitSounds.integer > 1) idx = 3 - idx;
						PLAY_HIT_SOUND(cgs.media.hitSounds[idx], ops);
					}
					else
						PLAY_HIT_SOUND(cgs.media.hitSound, ops);
				}
			}
		}
	}

	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 && ps->stats[STAT_HEALTH] > 0)
		CG_PainEvent(&cg.predictedPlayerEntity, ps->stats[STAT_HEALTH]);

	reward = qfalse;
	if (ps->persistant[PERS_CAPTURES] != ops->persistant[PERS_CAPTURES])
	{
		PUSH_REWARD(cgs.media.captureAwardSound, cgs.media.medalCapture, ps->persistant[PERS_CAPTURES]);
		reward = qtrue;
		//Com_Printf("capture\n");
	}
	if (ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT])
	{
		sfx = cgs.media.impressiveSound;
		PUSH_REWARD(sfx, cgs.media.medalImpressive, ps->persistant[PERS_IMPRESSIVE_COUNT]);
		reward = qtrue;
	}
	if (ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT])
	{
		sfx = cgs.media.excellentSound;
		PUSH_REWARD(sfx, cgs.media.medalExcellent, ps->persistant[PERS_EXCELLENT_COUNT]);
		reward = qtrue;
		//Com_Printf("excellent\n");
	}
	if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT])
	{
		sfx = cgs.media.humiliationSound;
		PUSH_REWARD(sfx, cgs.media.medalGauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT]);
		reward = qtrue;
		//Com_Printf("guantlet frag\n");
	}
	if (ps->persistant[PERS_DEFEND_COUNT] != ops->persistant[PERS_DEFEND_COUNT])
	{
		PUSH_REWARD(cgs.media.defendSound, cgs.media.medalDefend, ps->persistant[PERS_DEFEND_COUNT]);
		reward = qtrue;
		//Com_Printf("defend\n");
	}
	if (ps->persistant[PERS_ASSIST_COUNT] != ops->persistant[PERS_ASSIST_COUNT])
	{
		PUSH_REWARD(cgs.media.assistSound, cgs.media.medalAssist, ps->persistant[PERS_ASSIST_COUNT]);
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
spectate exec, ammo, events, duck. QVM_LOGIC: change-driven, no per-frame vmCvar
reads in hot path (exec uses cgs snapshot).
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

	/* damage feedback: inline, linear (QVM_LOGIC), no call */
	if (ps->damageEvent != ops->damageEvent && ps->damageCount)
	{
		float       left, front, up;
		float       kick;
		int         health;
		float       scale;
		vec3_t      dir;
		vec3_t      angles;
		float       dist;
		float       yaw, pitch;
		int         yawByte, pitchByte;

		cg.attackerTime = cg.time;
		yawByte = ps->damageYaw;
		pitchByte = ps->damagePitch;
		health = cg.snap->ps.stats[STAT_HEALTH];
		scale = (health < 40) ? 1.0f : (40.0f / (float)health);
		kick = (float)ps->damageCount * scale;
		if (kick < 5.0f)
			kick = 5.0f;
		if (kick > 10.0f)
			kick = 10.0f;

		cg.damageX = 0.0f;
		cg.damageY = 0.0f;
		cg.v_dmg_roll = 0.0f;
		cg.v_dmg_pitch = -kick;
		if (yawByte != 255 || pitchByte != 255)
		{
			pitch = (float)pitchByte / 255.0f * 360.0f;
			yaw = (float)yawByte / 255.0f * 360.0f;
			angles[PITCH] = pitch;
			angles[YAW] = yaw;
			angles[ROLL] = 0.0f;
			AngleVectors(angles, dir, NULL, NULL);
			VectorSubtract(vec3_origin, dir, dir);
			front = DotProduct(dir, cg.refdef.viewaxis[0]);
			left = DotProduct(dir, cg.refdef.viewaxis[1]);
			up = DotProduct(dir, cg.refdef.viewaxis[2]);
			dir[0] = front;
			dir[1] = left;
			dir[2] = 0.0f;
			dist = VectorLength(dir);
			if (dist < 0.1f)
				dist = 0.1f;
			cg.v_dmg_roll = kick * left;
			cg.v_dmg_pitch = -kick * front;
			if (front <= 0.1f)
				front = 0.1f;
			cg.damageX = -left / front;
			cg.damageY = up / dist;
		}
		if (cg.damageX > 1.0f)
			cg.damageX = 1.0f;
		else if (cg.damageX < -1.0f)
			cg.damageX = -1.0f;
		if (cg.damageY > 1.0f)
			cg.damageY = 1.0f;
		else if (cg.damageY < -1.0f)
			cg.damageY = -1.0f;
		cg.damageValue = kick;
		cg.v_dmg_time = cg.time + DAMAGE_TIME;
		cg.damageTime = cg.snap->serverTime;
	}

	// respawning
	if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
	{
		CG_Respawn();
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
		/* CG_HitDamage inlined */
		{
			int hdHits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];
			if (hdHits > 0)
			{
				int hdAtta = ps->persistant[PERS_ATTACKEE_ARMOR];
				cgs.osp.lastHitDamage = (hdAtta == 0) ? hdHits : (hdAtta & 0x00FF);
			}
			else if (hdHits < 0)
				cgs.osp.lastHitDamage = 0;
		}
	}

	if (cg.zoomed &&
	        (((cg_zoomAutoReset.integer & 1) && ops->stats[STAT_HEALTH] > 0 && ps->stats[STAT_HEALTH] <= 0 && cg.snap->ps.stats[STAT_HEALTH] <= 0) ||
	         ((cg_zoomAutoReset.integer & 4) && ps->weapon != ops->weapon)))
	{
		cg.zoomed = qfalse;
		cg.zoomTime = cg.time;
	}

	if (ops->stats[STAT_HEALTH] > 0 && ps->stats[STAT_HEALTH] <= 0)
	{
		const char* execOnDeath = cg_exec_onDeath.string;
		if (execOnDeath && execOnDeath[0])
			trap_SendConsoleCommand(execOnDeath);
	}

	/* force exec when starting to spectate (inlined) */
	if (cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		int snapCl = (cg.snap->ps.clientNum >= 0 && cg.snap->ps.clientNum < MAX_CLIENTS) ? cg.snap->ps.clientNum : -1;
		if (cg_exec_weapons.integer && snapCl != cgs.lastExecSpectateClientNum && snapCl != -1 && ps->weapon != WP_NONE)
			CG_EXEC_WEAPON_SWITCH_INLINE(ps->weapon, ps->clientNum);
		cgs.lastExecSpectateClientNum = snapCl; /* set in both paths */
	}
	else if (cg.snap)
		cgs.lastExecSpectateClientNum = -1;

	/* weapon once: Lightning tracking (when cg_shud.integer), exec on change, ammo (ps->weapon read once, QVM_LOGIC) */
	{
		int             weapon;
		int             ammo, threshold, currentWarning, prevAmmoSaved;
		int             wtAmmo, wtHitsDelta, wtAtta, wtDamage, wtActualHits;
		customWeaponStats_t* ws;

		weapon = ps->weapon;

		if (cg_shud.integer)
		{
			ws = &cgs.beWeaponStats[WP_LIGHTNING];
			if (ps->weapon != WP_LIGHTNING || ps->stats[STAT_HEALTH] <= 0)
			{
				ws->onTrack = qfalse;
				ws->hitsStart = 0;
				ws->hitsCurrent = 0;
				ws->shotsStart = 0;
				ws->lastAmmo = 0;
				lastHitsValueLightning = 0;
			}
			else if (ws->onTrack && (cg.time - ws->lastAttackTime > 1000))
			{
				ws->onTrack = qfalse;
				ws->hitsStart = 0;
				ws->hitsCurrent = 0;
				ws->shotsStart = 0;
				ws->lastAmmo = 0;
				lastHitsValueLightning = 0;
			}
			else
			{
				wtAmmo = ps->ammo[WP_LIGHTNING];
				wtHitsDelta = ps->persistant[PERS_HITS] - lastHitsValueLightning;
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
				lastHitsValueLightning = ps->persistant[PERS_HITS];
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
		}

		if (weapon != ops->weapon && weapon != WP_NONE)
			CG_EXEC_WEAPON_SWITCH_INLINE(weapon, ps->clientNum);

		if (weapon == WP_NONE || !cg_drawAmmoWarning.integer)
		{
			cg.lowAmmoWarning = 0;
			prevWeaponState = WEAPON_READY;
			prevAmmo = 0;
		}
		else
		{
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
			}
			else
			{
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
		}
	}

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	/* smooth ducking viewheight: read from cgs snapshot, not vmCvar */
	if (cg_smoothDucking.integer && ps->viewheight != ops->viewheight)
	{
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}

