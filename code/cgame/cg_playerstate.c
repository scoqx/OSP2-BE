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

void CG_CheckAmmo(void)
{
	int weapon;
	int ammo;
	int threshold;
	int i;
	int weapons;
	int total;
	int previous;
	int currentWarning;

	static int lowAmmoWarningPrev[WP_NUM_WEAPONS] = {0};
	static int lastWeapon = WP_NONE;

	static int prevWeaponState = WEAPON_READY;
	static int prevAmmo = 0;

	if (cg.snap->ps.weapon == WP_NONE || !cg_drawAmmoWarning.integer)
	{
		cg.lowAmmoWarning = 0;
		prevWeaponState = WEAPON_READY;
		prevAmmo = 0;
		return;
	}

	// no ammo sounds when trying to shot without ammo
	weapon = cg.snap->ps.weapon;
	ammo = cg.snap->ps.ammo[weapon];
	{
		int currentWeaponState = cg.snap->ps.weaponstate;

		if (currentWeaponState == WEAPON_FIRING && ammo == 0 &&
		        !(prevWeaponState == WEAPON_FIRING && prevAmmo == 0))
		{
			trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
		}

		prevWeaponState = currentWeaponState;
		prevAmmo = ammo;
	}

	if (cg_drawAmmoWarning.integer == 1)
	{
		threshold = lowAmmoThresholds[weapon];
		currentWarning = 0;

		if (ammo == 0)
			currentWarning = 2;
		else if (threshold > 0 && ammo <= threshold)
			currentWarning = 1;

		cg.lowAmmoWarning = currentWarning;

		if (currentWarning != lowAmmoWarningPrev[weapon] || (currentWarning != 0 && weapon != lastWeapon))
		{
			if (currentWarning == 1)
			{
				trap_S_StartLocalSound(cgs.media.lowAmmoSound, CHAN_LOCAL_SOUND);
			}
			else if (currentWarning == 2)
			{
				trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
			}
		}

		lastWeapon = weapon;
		lowAmmoWarningPrev[weapon] = currentWarning;

		return;
	}
	// default osp
	if (cg_drawAmmoWarning.integer == 2)
	{
		weapons = cg.snap->ps.stats[STAT_WEAPONS];
		total = 0;

		for (i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; i++)
		{
			if (!(weapons & (1 << i)))
			{
				continue;
			}
			switch (i)
			{
				case WP_ROCKET_LAUNCHER:
				case WP_GRENADE_LAUNCHER:
				case WP_RAILGUN:
				case WP_SHOTGUN:
					total += cg.snap->ps.ammo[i] * 1000;
					break;
				default:
					total += cg.snap->ps.ammo[i] * 200;
					break;
			}
			if (total >= 5000)
			{
				cg.lowAmmoWarning = 0;
				return;
			}
		}

		previous = cg.lowAmmoWarning;

		if (total == 0)
		{
			cg.lowAmmoWarning = 2;
		}
		else
		{
			cg.lowAmmoWarning = 1;
		}

		if (cg.lowAmmoWarning != previous)
		{
			trap_S_StartLocalSound(cgs.media.noAmmoSound, CHAN_LOCAL_SOUND);
		}

		return;
	}

	cg.lowAmmoWarning = 0;
}


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



void CG_UpdateWeaponTracking(int weapon)
{
	static int lastHitCount = 0;
	int currentAmmo, currentHits;

	customWeaponStats_t* ws = &cgs.be.weaponStats[weapon];
	playerState_t* ps = &cg.snap->ps;

	// Сброс статистики, если игрок умер или сменил оружие
	if (ps->weapon != weapon || ps->stats[STAT_HEALTH] <= 0)
	{
		ws->onTrack = qfalse;
		ws->hitsStart = 0;
		ws->hitsCurrent = 0;
		ws->shotsStart = 0;
		ws->lastAmmo = 0;
		lastHitCount = 0;
		return;
	}

	// Сброс, если прошло больше 1 секунды без атак
	if (ws->onTrack && (cg.time - ws->lastAttackTime > 1000))
	{
		ws->onTrack = qfalse;
		ws->hitsStart = 0;
		ws->hitsCurrent = 0;
		ws->shotsStart = 0;
		ws->lastAmmo = 0;
		lastHitCount = 0;
		return;
	}

	// Получаем текущее количество патронов и количество попаданий
	currentAmmo = ps->ammo[weapon];
	currentHits = ps->persistant[PERS_HITS];

	// Если оружие уже "на треке", обновляем данные
	if (ws->onTrack)
	{
		// Только если сейчас атака — обновляем lastAttackTime
		if (ps->weaponstate == WEAPON_FIRING)
		{
			ws->lastAttackTime = cg.time;
		}

		ws->hitsCurrent = currentHits;
		ws->lastAmmo = currentAmmo;
		return;
	}

	// Начало нового "трека" (первая атака)
	ws->hitsStart = currentHits;
	ws->hitsCurrent = currentHits;
	ws->shotsStart = currentAmmo;
	ws->lastAmmo = currentAmmo;
	ws->lastAttackTime = cg.time;
	ws->onTrack = qtrue;

	// Инициализация для Lightning
	if (ps->weapon == weapon)
	{
		lastHitCount = currentHits;
	}
}





/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn(void)
{
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
	cg.weaponSelectTime = cg.time;

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
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
	int i;
	int event;
	centity_t*   cent;

	cent = &cg.predictedPlayerEntity;
	for (i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++)
	{
		//
		if (i >= cg.eventSequence)
		{
			continue;
		}
		// if this event is not further back in than the maximum predictable events we remember
		if (i > cg.eventSequence - MAX_PREDICTED_EVENTS)
		{
			// if the new playerstate event is different from a previously predicted one
			if (ps->events[i & (MAX_PS_EVENTS - 1)] != cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1) ])
			{

				event = ps->events[ i & (MAX_PS_EVENTS - 1) ];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS - 1) ];
				CG_EntityEvent(cent, cent->lerpOrigin);

				cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS - 1) ] = event;

				if (cg_showmiss.integer)
				{
					CG_Printf("WARNING: changed predicted event\n");
				}
			}
		}
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

// for DamageSound
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

void CG_HitDamage(playerState_t* ps, playerState_t* ops)
{
	int atta, hits, damage;

	// vrode pohuy
	// if (!(OSP_CUSTOM_CLIENT_2_IS_DMG_INFO_ALLOWED()) || !(cgs.osp.server_mode == OSP_SERVER_MODE_PROMODE) || !(cgs.osp.server_mode == OSP_SERVER_MODE_CQ3))
	// return;

	hits = ps->persistant[PERS_HITS] - ops->persistant[PERS_HITS];
	if (hits < 0)
	{
		// Friendly fire
		// cgs.osp.lastHitTime = cg.time;
		cgs.osp.lastHitDamage = 0;
		return;
	}

	if (hits == 0)
	{
		return;
	}

	atta = ps->persistant[PERS_ATTACKEE_ARMOR];
	damage = (atta == 0) ? hits : (atta & 0x00FF);

	// cgs.osp.lastHitTime = cg.time;
	cgs.osp.lastHitDamage = damage;
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
		// TODO we need some solution when we take quad cuz sometimes with LG dmg + delayedDMG > 25
		//damage = ops->powerups[PW_QUAD] ? 26 : damage; // for a homogeneous sound with quad

		// if ((OSP_CUSTOM_CLIENT_2_IS_DMG_INFO_ALLOWED()
		//         && cg_hitSounds.integer) || cgs.osp.server_mode == OSP_SERVER_MODE_PROMODE || cgs.osp.server_mode == OSP_SERVER_MODE_CQ3)
		if (cg_hitSounds.integer)
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

	// don't play the sounds if the player just changed teams
	if (ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM])
	{
		return;
	}

	// incoming damage changes
	CG_DamageSound(ps, ops);

	// hit changes
	CG_HitSound(ps, ops);

	// health changes of more than -1 should make pain sounds
	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1)
	{
		if (ps->stats[STAT_HEALTH] > 0)
		{
			CG_PainEvent(&cg.predictedPlayerEntity, ps->stats[STAT_HEALTH]);
		}
	}

	// if we are going into the intermission, don't start any voices
	if (cg.intermissionStarted)
	{
		return;
	}

	// reward sounds
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

	// lead changes
	if (!reward)
	{
		//
		if (!cg.warmup)
		{
			// never play lead changes during warmup
			if (ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK])
			{
				if (cgs.gametype < GT_TEAM && !cg_noLeadSounds.integer)
				{
					if (ps->persistant[PERS_RANK] == 0)
					{
						CG_AddBufferedSound(cgs.media.takenLeadSound);
					}
					else if (ps->persistant[PERS_RANK] == RANK_TIED_FLAG)
					{
						CG_AddBufferedSound(cgs.media.tiedLeadSound);
					}
					else if ((ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG) == 0)
					{
						CG_AddBufferedSound(cgs.media.lostLeadSound);
					}
				}
			}
		}
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

===============
*/
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops)
{
	// check for changing follow mode
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
		CG_HitDamage(ps, ops);
	}

	if (cg_shud.integer)
		CG_UpdateWeaponTracking(WP_LIGHTNING);
	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if (ps->viewheight != ops->viewheight)
	{
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}

