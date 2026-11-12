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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "ql_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "bg_pmove_jump.h"

pmove_t		*pm;
pml_t		pml;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.60f;
float	pm_wadeScale = 0.80f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_aircontrol = 0.0f;
float	pm_airstepfriction = 1.0f;
int		pm_airsteps = 1;
float	pm_airstopaccelerate = 0.0f;
float	pm_strafeaccelerate = 0.0f;
float	pm_circlestrafe_friction = 0.0f;
qboolean	pm_bunnyhop = qfalse;
qboolean	pm_autohop = qfalse;
float	pm_wishspeed = 0.0f;
float	pm_stepHeight = 22.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 6.0f;	/* retail PM_Friction PW_FLIGHT: 6.0 (qagame 0x1002dcd0) */
float	pm_spectatorfriction = 5.0f;

int		c_pmove = 0;

static float	pm_jumpTakeoffVelocity;
static qboolean	pm_jumpTakeoffDoubleJumpActive;

#define PM_JUMP_VELOCITY_MODE_NONE		0
#define PM_JUMP_VELOCITY_MODE_SCALE		1
#define PM_JUMP_VELOCITY_MODE_ADDITIVE		2
#define PM_JUMP_VELOCITY_MODE_AIR_CONTROL	3

static int	pm_jumpVelocityMode = PM_JUMP_VELOCITY_MODE_SCALE;

typedef struct {
	float	airControl;
	float	airAccel;
	float	airStopAccel;
	float	circleStrafeFriction;
	float	strafeAccel;
	float	walkAccel;
	int	airSteps;
	float	wishSpeed;
} pmove_tuning_constants_t;

// HLIL parity: qagamex86.dll initialises these defaults at 0x1008f800 (pmove_AirStepFriction = 0.03f),
// 0x1008f82c (pmove_AirStopAccel = 1.0f), 0x1008f844/0x1008f85c/0x1008f874 (AutoHop/BunnyHop/ChainJump = 1),
// 0x1008f8a4 (pmove_CircleStrafeFriction = 6.0f), and 0x1008fab4/0x1008facc (WaterSwim/Wade scales = 0.6f/0.8f).
/* defaults filled by assignment, not designated initializers, so the file
   also builds with q3lcc (C89) as OSP2-BE's QL prediction */
static pmove_settings_t	pm_defaultSettingsData;
static qboolean	pm_defaultSettingsReady = qfalse;

static const pmove_settings_t *PM_DefaultSettings( void ) {
	pmove_settings_t	*s = &pm_defaultSettingsData;

	if ( pm_defaultSettingsReady ) {
		return s;
	}
	memset( s, 0, sizeof( *s ) );
	s->airAccel = 1.0f;
	s->airControl = 0.0f;
	s->airStepFriction = 0.03f;
	s->airSteps = 1;
	s->airStopAccel = 1.0f;
	s->autoHop = qtrue;
	s->bunnyHop = qtrue;
	s->chainJump = PM_JUMP_VELOCITY_MODE_SCALE;
	s->chainJumpVelocity = 110.0f;
	s->circleStrafeFriction = 6.0f;
	s->crouchSlide = qfalse;
	s->crouchSlideFriction = 0.5f;
	s->crouchSlideTime = 2000;
	s->crouchStepJump = qtrue;
	s->doubleJump = qfalse;
	s->jumpTimeDeltaMin = 100.0f;
	s->jumpVelocity = 275.0f;
	s->jumpVelocityMax = 700.0f;
	s->jumpVelocityScaleAdd = 0.4f;
	s->jumpVelocityTimeThreshold = 500.0f;
	s->jumpVelocityTimeThresholdOffset = 0.6f;
	s->noPlayerClip = qfalse;
	s->rampJump = qfalse;
	s->rampJumpScale = 1.0f;
	s->stepHeight = 22.0f;
	s->stepJump = qtrue;
	s->stepJumpVelocity = 48.0f;
	s->strafeAccel = 1.0f;
	s->velocityGh = 800.0f;
	s->walkAccel = 10.0f;
	s->walkFriction = 6.0f;
	s->waterSwimScale = 0.60f;
	s->waterWadeScale = 0.80f;
	s->weaponDropTime = 200;
	s->weaponRaiseTime = 200;
	s->wishSpeed = 400.0f;
	s->machinegunIronsightsScale = 1.0f;
	s->gauntletSpeedFactor = 1.0f;
	s->midAirMinimumHeight = 96;
	s->nailgunBounceEnabled = qtrue;
	s->nailgunBouncePercentage = 65;
	s->quadDamageMultiplier = 3.0f;
	s->guidedRocketEnabled = qfalse;
	s->quadHogEnabled = 0;
	s->quadHogIdleSeconds = 20;
	s->quadHogTimeSeconds = 60;
	s->quadHogPingRateMilliseconds = 1500;
	s->weaponReloadOverrides[WP_NONE] = 0;
	// HLIL parity: weapon refire timings lifted from qagamex86.dll (matches Q3 defaults
	// except for the Quake Live-exclusive heavy machinegun entry).
	s->weaponReloadTimes[WP_NONE] = 0;
	s->weaponReloadTimes[WP_GAUNTLET] = 400;
	s->weaponReloadTimes[WP_MACHINEGUN] = 100;
	s->weaponReloadTimes[WP_SHOTGUN] = 1000;
	s->weaponReloadTimes[WP_GRENADE_LAUNCHER] = 800;
	s->weaponReloadTimes[WP_ROCKET_LAUNCHER] = 800;
	s->weaponReloadTimes[WP_LIGHTNING] = 50;
	s->weaponReloadTimes[WP_RAILGUN] = 1500;
	s->weaponReloadTimes[WP_PLASMAGUN] = 100;
	s->weaponReloadTimes[WP_BFG] = 300;
	s->weaponReloadTimes[WP_GRAPPLING_HOOK] = 100;
	s->weaponReloadTimes[WP_HMG] = 75;
	s->weaponReloadTimes[WP_NAILGUN] = 1000;
	s->weaponReloadTimes[WP_PROX_LAUNCHER] = 800;
	s->weaponReloadTimes[WP_CHAINGUN] = 50;
	pm_defaultSettingsReady = qtrue;
	return s;
}


static const pmove_tuning_constants_t	pm_retailDefaultTuning = {
	/* airControl, airAccel, airStopAccel, circleStrafeFriction, strafeAccel, walkAccel, airSteps, wishSpeed */
	0.0f, 1.0f, 1.0f, 6.0f, 1.0f, 10.0f, 1, 400.0f
};

// HLIL parity: 0x10001ec0 / 0x1002d9f0 switches to the retail air-control bundle
// (`150 / 1.1 / 2.5 / 5.5 / 70 / 15 / 3 / 35`) whenever the active movement
// profile enables the alternate strafe path.
static const pmove_tuning_constants_t	pm_retailAirControlTuning = {
	/* airControl, airAccel, airStopAccel, circleStrafeFriction, strafeAccel, walkAccel, airSteps, wishSpeed */
	150.0f, 1.1f, 2.5f, 5.5f, 70.0f, 15.0f, 3, 35.0f
};

#define PM_INVULNERABILITY_ACTIVE_TIME	0x7fffffff



/*
=============
PM_LoadMoveSettings

Synchronises extended Quake Live movement settings with legacy globals.
=============
*/
static void PM_LoadMoveSettings( void ) {
	const pmove_settings_t	*settings;
	float	swimScale;
	float	wadeScale;
	float	stepHeight;

	settings = PM_GetActiveSettings();

	swimScale = settings->waterSwimScale;
	if ( swimScale <= 0.0f ) {
		swimScale = PM_DefaultSettings()->waterSwimScale;
	}
	pm_swimScale = swimScale;

	wadeScale = settings->waterWadeScale;
	if ( wadeScale <= 0.0f ) {
		wadeScale = PM_DefaultSettings()->waterWadeScale;
	}
	pm_wadeScale = wadeScale;

	stepHeight = settings->stepHeight;
	if ( stepHeight <= 0.0f ) {
		stepHeight = PM_DefaultSettings()->stepHeight;
	}
	pm_stepHeight = stepHeight;
}

/*
=============
PM_FloatMatchesValue

Returns qtrue when two pmove tuning values match closely enough to treat the
current value as the stock retail baseline.
=============
*/
static qboolean PM_FloatMatchesValue( float value, float reference ) {
	return ( fabs( value - reference ) <= 0.001f ) ? qtrue : qfalse;
}

/*
=============
PM_LoadMoveTuningFloat

Seeds one retail pmove tuning float, preserving non-stock custom overrides when
the caller intentionally diverges from the active retail bundle.
=============
*/
static float PM_LoadMoveTuningFloat( float configuredValue, float stockValue, float retailValue, qboolean useRetailValue, qboolean nonNegativeValue ) {
	float	resolvedValue;

	if ( nonNegativeValue ) {
		resolvedValue = ( configuredValue >= 0.0f ) ? configuredValue : stockValue;
	} else {
		resolvedValue = ( configuredValue > 0.0f ) ? configuredValue : stockValue;
	}

	if ( useRetailValue && PM_FloatMatchesValue( resolvedValue, stockValue ) ) {
		return retailValue;
	}

	return resolvedValue;
}

/*
=============
PM_LoadMoveTuningInt

Seeds one retail pmove tuning integer, preserving non-stock custom overrides
when the caller intentionally diverges from the active retail bundle.
=============
*/
static int PM_LoadMoveTuningInt( int configuredValue, int stockValue, int retailValue ) {
	int	resolvedValue;

	resolvedValue = ( configuredValue >= 0 ) ? configuredValue : stockValue;
	if ( resolvedValue == stockValue ) {
		return retailValue;
	}

	return resolvedValue;
}


/*
=============
PM_GetDefaultWeaponReloadTime

Returns the compiled weapon reload duration for a slot.
=============
*/
static int PM_GetDefaultWeaponReloadTime( weapon_t weapon ) {
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return 0;
	}

	return PM_DefaultSettings()->weaponReloadTimes[weapon];
}

/*
=============
PM_GetWeaponReloadTime

Resolves the reload time for a weapon, falling back to defaults when unset.
=============
*/
static int PM_GetWeaponReloadTime( weapon_t weapon ) {
	const pmove_settings_t	*settings;
	int	reload;

	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return PM_GetDefaultWeaponReloadTime( WP_NONE );
	}

	if ( bg_weaponReloadTime[weapon] > 0 ) {
		return bg_weaponReloadTime[weapon];
	}

	settings = PM_GetActiveSettings();
	reload = settings->weaponReloadTimes[weapon];
	if ( reload <= 0 ) {
		reload = PM_GetDefaultWeaponReloadTime( weapon );
	}
	if ( reload <= 0 ) {
		reload = 200;
	}

	return reload;
}

/*
=============
PM_ShouldRequireJumpRelease

Returns whether jump inputs must be released before another takeoff.
=============
*/
static qboolean PM_ShouldRequireJumpRelease( const pmove_settings_t *settings ) {
	const pmove_settings_t	*activeSettings;

	if ( pm && pm->ps && ( pm->ps->pm_flags & PMF_REQUIRE_JUMP_RELEASE ) ) {
		return qtrue;
	}

	activeSettings = settings ? settings : PM_GetActiveSettings();

	if ( activeSettings ) {
		if ( activeSettings->autoHop ) {
			return qfalse;
		}
	}

	if ( pm_autohop ) {
		return qfalse;
	}

	return qtrue;
}

static qboolean PM_CheckJump( qboolean allowAirDoubleJump );
static qboolean PM_PrepareJumpTakeoff( qboolean allowAirDoubleJump );
static qboolean PM_PrepareStepJumpTakeoff( const pmove_settings_t *settings );
static qboolean PM_PrepareCrouchStepJumpTakeoff( const pmove_settings_t *settings );
static void PM_ApplyJumpTakeoff( void );

/*
=============
PM_UpdateJumpVelocityMode

Selects the retail jump-takeoff velocity mode from the active movement profile.
=============
*/
static void PM_UpdateJumpVelocityMode( const pmove_settings_t *settings ) {
	qboolean	airControlTuning;
	int		chainJumpMode;

	airControlTuning = ( pm && pm->ps && ( pm->ps->pm_flags & PMF_AIR_CONTROL ) ) ? qtrue : qfalse;

	if ( !settings ) {
		pm_jumpVelocityMode = airControlTuning ? PM_JUMP_VELOCITY_MODE_AIR_CONTROL : PM_JUMP_VELOCITY_MODE_SCALE;
		return;
	}

	if ( airControlTuning ) {
		pm_jumpVelocityMode = PM_JUMP_VELOCITY_MODE_AIR_CONTROL;
		return;
	}

	chainJumpMode = settings->chainJump;
	if ( chainJumpMode <= PM_JUMP_VELOCITY_MODE_NONE ) {
		pm_jumpVelocityMode = PM_JUMP_VELOCITY_MODE_NONE;
		return;
	}

	pm_jumpVelocityMode = ( chainJumpMode == PM_JUMP_VELOCITY_MODE_ADDITIVE )
		? PM_JUMP_VELOCITY_MODE_ADDITIVE
		: PM_JUMP_VELOCITY_MODE_SCALE;
}

/*
=============
PM_LoadMoveTuningConstants

Seeds the legacy pmove globals from the active Quake Live settings bundle.
=============
*/
static void PM_LoadMoveTuningConstants( void ) {
	const pmove_settings_t	*settings;
	qboolean		airControlTuning;
	qboolean		useRetailBundle;

	settings = PM_GetActiveSettings();
	airControlTuning = ( pm->ps->pm_flags & PMF_AIR_CONTROL ) ? qtrue : qfalse;
	/* Stock cvars + PMF_AIR_CONTROL → retail promode bundle (1.1/35/...).
	   Explicit pmove_AirControl (g_physics cpm, etc.) keeps the configured numbers,
	   including AirAccel 1.0 which would otherwise match stock and get replaced by 1.1. */
	useRetailBundle = airControlTuning
		&& !( settings->airControl > pm_retailDefaultTuning.airControl );
	PM_UpdateJumpVelocityMode( settings );

	pm_accelerate = PM_LoadMoveTuningFloat(
		settings->walkAccel,
		pm_retailDefaultTuning.walkAccel,
		pm_retailAirControlTuning.walkAccel,
		useRetailBundle,
		qfalse
	);
	pm_airaccelerate = PM_LoadMoveTuningFloat(
		settings->airAccel,
		pm_retailDefaultTuning.airAccel,
		pm_retailAirControlTuning.airAccel,
		useRetailBundle,
		qfalse
	);
	pm_aircontrol = PM_LoadMoveTuningFloat(
		settings->airControl,
		pm_retailDefaultTuning.airControl,
		pm_retailAirControlTuning.airControl,
		useRetailBundle,
		qfalse
	);
	pm_airstepfriction = PM_LoadMoveTuningFloat(
		settings->airStepFriction,
		PM_DefaultSettings()->airStepFriction,
		PM_DefaultSettings()->airStepFriction,
		qfalse,
		qtrue
	);
	pm_airsteps = useRetailBundle
		? PM_LoadMoveTuningInt( settings->airSteps, pm_retailDefaultTuning.airSteps, pm_retailAirControlTuning.airSteps )
		: PM_LoadMoveTuningInt( settings->airSteps, pm_retailDefaultTuning.airSteps, pm_retailDefaultTuning.airSteps );
	pm_airstopaccelerate = PM_LoadMoveTuningFloat(
		settings->airStopAccel,
		pm_retailDefaultTuning.airStopAccel,
		pm_retailAirControlTuning.airStopAccel,
		useRetailBundle,
		qtrue
	);
	pm_strafeaccelerate = PM_LoadMoveTuningFloat(
		settings->strafeAccel,
		pm_retailDefaultTuning.strafeAccel,
		pm_retailAirControlTuning.strafeAccel,
		useRetailBundle,
		qtrue
	);
	pm_circlestrafe_friction = PM_LoadMoveTuningFloat(
		settings->circleStrafeFriction,
		pm_retailDefaultTuning.circleStrafeFriction,
		pm_retailAirControlTuning.circleStrafeFriction,
		useRetailBundle,
		qtrue
	);
	pm_wishspeed = PM_LoadMoveTuningFloat(
		settings->wishSpeed,
		pm_retailDefaultTuning.wishSpeed,
		pm_retailAirControlTuning.wishSpeed,
		useRetailBundle,
		qtrue
	);
	pm_friction = ( settings->walkFriction > 0.0f ) ? settings->walkFriction : PM_DefaultSettings()->walkFriction;
	pm_bunnyhop = settings->bunnyHop;
	pm_autohop = settings->autoHop;
	PM_LoadMoveSettings();
}

/*
=============
PM_ApplyStepJump

Triggers the retail-style step jump path. The crouch-step branch mirrors the
retail fallback that can run after the general upmove gate is rejected.
=============
*/
void PM_ApplyStepJump( float stepDelta, qboolean fromCrouchSlide ) {
	const pmove_settings_t	*settings;

	if ( stepDelta <= 0.0f ) {
		return;
	}

	settings = PM_GetActiveSettings();
	if ( !settings->stepJump ) {
		return;
	}

	if ( fromCrouchSlide ) {
		if ( !( pm->ps->pm_flags & PMF_DUCKED ) ) {
			return;
		}

		if ( !settings->crouchStepJump ) {
			return;
		}
	}

	if ( !fromCrouchSlide && pm->cmd.upmove < 10 ) {
		return;
	}

	if ( fromCrouchSlide ) {
		if ( !PM_PrepareCrouchStepJumpTakeoff( settings ) ) {
			return;
		}
	} else {
		if ( !PM_PrepareJumpTakeoff( qfalse ) ) {
			return;
		}
		if ( !PM_PrepareStepJumpTakeoff( settings ) ) {
			return;
		}
	}
	PM_ApplyJumpTakeoff();
}

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal);
	
	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;
	const pmove_settings_t	*settings;
	float	friction;
	
	vel = pm->ps->velocity;
	settings = PM_GetActiveSettings();
	
	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				friction = pm_friction;

				if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE )
					&& pm->cmd.upmove < 0 && pm->ps->crouchSlideTime > 0 ) {
					friction = settings->crouchSlideFriction;

					if ( friction < 0.0f ) {
						friction = 0.0f;
					}
				} else if ( ( pm->ps->pm_flags & PMF_AIR_CONTROL ) && pm->cmd.forwardmove
					&& pm->cmd.rightmove
					&& ( pm->ps->movementDir == 1 || pm->ps->movementDir == 3
						|| pm->ps->movementDir == 5 || pm->ps->movementDir == 7 ) ) {
					/* retail Friction 0x1002DCD0 gates circle-strafe on PMF bit */
					friction = pm_circlestrafe_friction;
				}

				drop += control * friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT]) {
		drop += speed*pm_flightfriction*pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR) {
		drop += speed*pm_spectatorfriction*pml.frametime;
	}

	if ( pm->ps->pm_type == PM_FREEZE || ( pm->ps->pm_flags & PMF_SCOREBOARD ) ) {
		drop *= 0.25f;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration

Retail qagamex86.dll 0x1002DEF0: when addSpeed <= 0 and the overspeed
enable flag is set (default 1 at 0x1008FEBC) and movementDir == 0, still
push along wishdir by 0.55 (or 0.55 * speed / (2*speed - horiz) when
horiz > speed). Official cgame predicts this; without it, ground forward
strafes and post-jump forward air speed diverge under cg_showmiss.
==============
*/
#ifdef PMH_HARNESS
#define PM_Accelerate PM_Accelerate_Impl
#endif
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
	/*
	 * Mirrors retail 0x1002DEF0 instruction by instruction: the x87 keeps
	 * intermediates in extended precision and only rounds where the code
	 * stores a float. Rounding the dot product / addSpeed as floats instead
	 * flips the addSpeed > 0 test once air acceleration has saturated (every
	 * long forward jump), and then retail takes the 0.55 overspeed push.
	 */
	float	w0 = wishdir[0], w1 = wishdir[1], w2 = wishdir[2];
	float	v0 = pm->ps->velocity[0], v1 = pm->ps->velocity[1];
	float	addSpeed, accelSpeed, horizSpeed, speedf, twiceSpeed, amount;
	long double	dot;

	dot = (long double)w0 * v0 + (long double)v1 * w1;
	dot += (long double)w2 * pm->ps->velocity[2];
	addSpeed = (float)( (long double)wishspeed - dot );
	speedf = (float)pm->ps->speed;
	horizSpeed = (float)sqrtl( (long double)v0 * v0 + (long double)v1 * v1 );
	twiceSpeed = (float)( (long double)speedf + speedf );

	if ( addSpeed > 0.0f ) {
		accelSpeed = (float)( (long double)pml.frametime * wishspeed * accel );
		if ( accelSpeed > addSpeed ) {
			accelSpeed = addSpeed;
		}
		pm->ps->velocity[0] = (float)( (long double)w0 * accelSpeed + pm->ps->velocity[0] );
		pm->ps->velocity[1] = (float)( (long double)w1 * accelSpeed + pm->ps->velocity[1] );
		pm->ps->velocity[2] = (float)( (long double)accelSpeed * w2 + pm->ps->velocity[2] );
		return;
	}

	/* overspeed push: flag (pmove_BunnyHop -> 0x1008FEBC) + movementDir == 0 */
	if ( !pm_bunnyhop || pm->ps->movementDir != 0 ) {
		return;
	}
	if ( horizSpeed > twiceSpeed ) {
		return;
	}

	/*
	 * 0x1002DFE9..0x1002E027: up to ps->speed the push is 0x1008FE48 (0.55);
	 * above it fades out linearly to 0 at twice the speed:
	 *   amount = (2 * speed - horizSpeed) / speed * 0.55 (double 0x1008B1C0)
	 */
	amount = 0.55f;
	if ( horizSpeed > speedf ) {
		amount = (float)( ( (long double)twiceSpeed - horizSpeed ) / speedf * 0.550000011920928955078125 );
	}
	pm->ps->velocity[0] = (float)( (long double)w0 * amount + pm->ps->velocity[0] );
	pm->ps->velocity[1] = (float)( (long double)w1 * amount + pm->ps->velocity[1] );
	pm->ps->velocity[2] = (float)( (long double)amount * w2 + pm->ps->velocity[2] );
}
#ifdef PMH_HARNESS
#undef PM_Accelerate
/* differential harness only (tools/pmharness): retail PM_Accelerate on the same inputs */
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
	extern void PMH_AccelPre( const playerState_t *ps, const float *wishdir, float wishspeed, float accel, float frametime );
	extern void PMH_AccelPost( const playerState_t *ps );
	PMH_AccelPre( pm->ps, wishdir, wishspeed, accel, pml.frametime );
	PM_Accelerate_Impl( wishdir, wishspeed, accel );
	PMH_AccelPost( pm->ps );
}
#endif

/*
==============
PM_AirControl

Calculates velocity redirection for air control
==============
*/
static void PM_AirControl( vec3_t wishdir, float wishspeed ) {
	float	zspeed;
	float	speed;
	float	dot;
	float	k;
	int		i;

	/* retail PM_AirControl 0x1002FBA0: only pure forward/back (dir 0 or 4) */
	if ( ( pm->ps->movementDir != 0 && pm->ps->movementDir != 4 )
		|| wishspeed == 0.0f ) {
		return;
	}

	zspeed = pm->ps->velocity[2];
	pm->ps->velocity[2] = 0;
	speed = VectorNormalize( pm->ps->velocity );

	dot = DotProduct( pm->ps->velocity, wishdir );
	k = 32.0f * pm_aircontrol * dot * dot * pml.frametime;

	if ( dot > 0 ) {
		for ( i = 0 ; i < 2 ; i++ ) {
			pm->ps->velocity[i] = pm->ps->velocity[i] * speed + wishdir[i] * k;
		}
		VectorNormalize( pm->ps->velocity );
	}

	// Retail preserves the original planar speed even when the steering dot is
	// non-positive; only the direction correction is gated by the dot check.
	for ( i = 0 ; i < 2 ; i++ ) {
		pm->ps->velocity[i] *= speed;
	}

	pm->ps->velocity[2] = zspeed;
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}

/*
=============
PM_BuildWishMove3D

Builds the active 3D wish direction from the current command axes and
returns the derived wishspeed.
=============
*/
static qboolean PM_BuildWishMove3D( vec3_t wishdir, float *wishspeed ) {
	vec3_t	wishvel;
	float	scale;
	int		i;

	if ( !wishdir || !wishspeed ) {
		return qfalse;
	}

	VectorClear( wishdir );
	*wishspeed = 0.0f;

	scale = PM_CmdScale( &pm->cmd );
	if ( !scale ) {
		return qfalse;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
	}
	wishvel[2] += scale * pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	*wishspeed = VectorNormalize( wishdir );

	return qtrue;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		} 
	}
}


/*
=============
PM_EvaluateJumpTakeoffVelocity

Reconstructs the retail vertical jump velocity modes used by the shared
jump takeoff leaf.
=============
*/
static float PM_EvaluateJumpTakeoffVelocity( const pmove_settings_t *settings, qboolean stepJumpActive, int *outTimeDelta ) {
	float	jumpVelocity;
	float	threshold;
	float	addVelocity;
	float	offsetThreshold;
	int	timeDelta;

	if ( outTimeDelta ) {
		*outTimeDelta = -1;
	}

	if ( !settings ) {
		return JUMP_VELOCITY;
	}

	jumpVelocity = ( settings->jumpVelocity > 0.0f ) ? settings->jumpVelocity : JUMP_VELOCITY;
	if ( !pm || !pm->ps || pm->cmd.serverTime < pm->ps->jumpTime ) {
		return jumpVelocity;
	}

	timeDelta = pm->cmd.serverTime - pm->ps->jumpTime;
	if ( outTimeDelta ) {
		*outTimeDelta = timeDelta;
	}

	if ( timeDelta <= 0 ) {
		return jumpVelocity;
	}

	threshold = settings->jumpVelocityTimeThreshold;
	if ( threshold <= 0.0f || (float)timeDelta >= threshold ) {
		return jumpVelocity;
	}

	PM_UpdateJumpVelocityMode( settings );
	switch ( pm_jumpVelocityMode ) {
	case PM_JUMP_VELOCITY_MODE_SCALE:
		jumpVelocity *= PM_EvaluateJumpVelocityScale( pm->ps, settings, pm->cmd.serverTime, NULL );
		break;
	case PM_JUMP_VELOCITY_MODE_ADDITIVE:
		jumpVelocity += stepJumpActive ? settings->stepJumpVelocity : settings->chainJumpVelocity;
		break;
	case PM_JUMP_VELOCITY_MODE_AIR_CONTROL:
		offsetThreshold = threshold * settings->jumpVelocityTimeThresholdOffset;
		if ( offsetThreshold <= 0.0f ) {
			break;
		}

		addVelocity = stepJumpActive ? settings->stepJumpVelocity : settings->chainJumpVelocity;
		if ( (float)timeDelta < offsetThreshold ) {
			jumpVelocity += addVelocity;
		} else {
			// Retail divides by the offset threshold here, leaving a sharp
			// drop when the outer threshold returns to base jump velocity.
			jumpVelocity += ( ( offsetThreshold - (float)timeDelta ) / offsetThreshold ) * addVelocity + addVelocity;
		}
		break;
	default:
		break;
	}

	return jumpVelocity;
}

/*
=============
PM_ApplyRampJumpVerticalVelocity

Applies the retail ramp-jump vertical accumulation and max clamp.
=============
*/
static float PM_ApplyRampJumpVerticalVelocity( float jumpVelocity, qboolean fromCrouchStep, const pmove_settings_t *settings ) {
	float	velocity;

	velocity = jumpVelocity;

	if ( settings && settings->rampJump && !fromCrouchStep && pm && pm->ps ) {
		velocity = pm->ps->velocity[2] * settings->rampJumpScale + jumpVelocity;
		if ( velocity < jumpVelocity ) {
			velocity = jumpVelocity;
		}
	}

	if ( settings && settings->jumpVelocityMax > 0.0f && velocity > settings->jumpVelocityMax ) {
		velocity = settings->jumpVelocityMax;
	}

	return velocity;
}

/*
=============
PM_PrepareStepJumpTakeoff

Prepares the general step-jump takeoff latch before the shared retail jump
leaf consumes the normal step-jump velocity selector.
=============
*/
static qboolean PM_PrepareStepJumpTakeoff( const pmove_settings_t *settings ) {
	float			jumpVelocity;

	if ( !settings ) {
		return qfalse;
	}

	jumpVelocity = PM_EvaluateJumpTakeoffVelocity( settings, qtrue, NULL );
	pm_jumpTakeoffVelocity = PM_ApplyRampJumpVerticalVelocity( jumpVelocity, qfalse, settings );
	pm_jumpTakeoffDoubleJumpActive = qfalse;

	return qtrue;
}

/*
=============
PM_PrepareCrouchStepJumpTakeoff

Prepares the crouch-step fallback takeoff. Retail uses this latch to suppress
ramp accumulation without enabling the normal step-jump velocity selector.
=============
*/
static qboolean PM_PrepareCrouchStepJumpTakeoff( const pmove_settings_t *settings ) {
	float			jumpVelocity;

	if ( !settings ) {
		return qfalse;
	}

	jumpVelocity = PM_EvaluateJumpTakeoffVelocity( settings, qfalse, NULL );
	pm_jumpTakeoffVelocity = PM_ApplyRampJumpVerticalVelocity( jumpVelocity, qtrue, settings );
	pm_jumpTakeoffDoubleJumpActive = qfalse;

	return qtrue;
}

/*
=============
PM_ApplyJumpTakeoff

Applies the retail jump takeoff state once the active jump-mode globals
have been prepared by the surrounding jump gate.
=============
*/
static void PM_ApplyJumpTakeoff( void ) {
	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	if ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) {
		pm->ps->crouchSlideTime = 0;
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->jumpTime = pm->cmd.serverTime;
	pm->ps->velocity[2] = pm_jumpTakeoffVelocity;

	if ( pm_jumpTakeoffDoubleJumpActive ) {
		pm->ps->doubleJumped = qtrue;
	}
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
}

/*
=============
PM_PrepareJumpTakeoff

Prepares the shared retail jump takeoff state used by both PM_CheckJump and
the step-jump seam before the final takeoff leaf runs.
=============
*/
static qboolean PM_PrepareJumpTakeoff( qboolean allowAirDoubleJump ) {
	const pmove_settings_t	*settings;
	float			jumpVelocity;
	qboolean		doubleJumpActive;
	qboolean		releaseRequired;
	int			timeDelta;

	settings = PM_GetActiveSettings();
	releaseRequired = PM_ShouldRequireJumpRelease( settings );

	if ( releaseRequired && ( pm->ps->pm_flags & PMF_RESPAWNED ) ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	if ( allowAirDoubleJump ) {
		if ( pm->ps->doubleJumped ) {
			return qfalse;
		}

		// retail air double jumps still require a fresh jump press.
		if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
			pm->cmd.upmove = 0;
			return qfalse;
		}
	} else {
		// must wait for jump to be released
		if ( releaseRequired && ( pm->ps->pm_flags & PMF_JUMP_HELD ) ) {
			// clear upmove so cmdscale doesn't lower running speed
			pm->cmd.upmove = 0;
			return qfalse;
		}
	}

	timeDelta = -1;

	jumpVelocity = PM_EvaluateJumpTakeoffVelocity( settings, qfalse, &timeDelta );

	/* retail CheckJump 0x1002E590: jumpTimeDeltaMin reject returns 0
	   without clearing upmove. Clearing here made CmdScale use a full
	   wishspeed while official cgame kept the held jump axis — ground
	   strafe and post-jump air both diverge under cg_showmiss whenever
	   jump is held inside the 100ms rejump window (bunnyhop). */
	if ( settings->jumpTimeDeltaMin > 0.0f && timeDelta >= 0 ) {
		if ( (float)timeDelta < settings->jumpTimeDeltaMin ) {
			return qfalse;
		}
	}

	doubleJumpActive = allowAirDoubleJump;

	pm_jumpTakeoffVelocity = PM_ApplyRampJumpVerticalVelocity( jumpVelocity, qfalse, settings );
	pm_jumpTakeoffDoubleJumpActive = doubleJumpActive;

	return qtrue;
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( qboolean allowAirDoubleJump ) {
	if ( !PM_PrepareJumpTakeoff( allowAirDoubleJump ) ) {
		return qfalse;
	}

	PM_ApplyJumpTakeoff();

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	/* Q3/ioq3: only solid/clip/body block the exit; CONTENTS_WATER at this
	   spot is normal at waterlevel 2 and must not cancel the waterjump.
	   A bare `if (cont)` made walk-out-of-water (forward into a ledge) fail. */
	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( cont & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY) ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	float	wishspeed;
	vec3_t	wishdir;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	if ( !PM_BuildWishMove3D( wishdir, &wishspeed ) ) {
		VectorSet( wishdir, 0.0f, 0.0f, -1.0f );		// sink towards bottom
		wishspeed = 60.0f;
	}

	/* retail WaterMove 0x1002F955: wade scale at waterlevel 1 */
	if ( pm->waterlevel == 1 ) {
		if ( wishspeed > pm->ps->speed * pm_wadeScale ) {
			wishspeed = pm->ps->speed * pm_wadeScale;
		}
	} else if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// retail PM_WaterMove (qagame 0x1002F890 / cgame twin): clip into ground
	// plane only — no Q3 speed restore — then StepSlideMove(qfalse).
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove( qfalse );
}

/*
===================
PM_CheckLadder

===================
*/
static void PM_CheckLadder( void ) {
	trace_t	trace;
	vec3_t	spot;
	vec3_t	flatforward;

	pml.ladder = qfalse;

	VectorCopy( pml.forward, flatforward );
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorAdd( pm->ps->origin, flatforward, spot );

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, MASK_PLAYERSOLID );
	if ( trace.fraction < 1.0f && ( trace.surfaceFlags & SURF_LADDER ) ) {
		pml.ladder = qtrue;
	}
}

/*
===================
PM_LadderMove

===================
*/
static void PM_LadderMove( void ) {
	vec3_t	wishvel;
	vec3_t	wishdir;
	float	wishspeed;
	float	vel;

	PM_Friction();

	if ( PM_BuildWishMove3D( wishdir, &wishspeed ) ) {
		VectorScale( wishdir, wishspeed, wishvel );
	} else {
		VectorClear( wishvel );
	}

	if ( pm->cmd.upmove > 0 ) {
		wishvel[2] = pm->ps->speed * 0.66f;
	} else if ( pm->cmd.upmove < 0 ) {
		wishvel[2] = -pm->ps->speed * 0.66f;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if ( wishspeed > pm->ps->speed * 0.66f ) {
		wishspeed = pm->ps->speed * 0.66f;
	}

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength( pm->ps->velocity );
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( qfalse );
}

/*
===================
PM_HasHeldInvulnerabilityItem

Synthetic parity helper that reuses the retail held-item gate for invulnerability.
===================
*/
static qboolean PM_HasHeldInvulnerabilityItem( int *invulnerabilityItemNum ) {
	const gitem_t	*invulnerabilityItem;
	int				itemNum;

	invulnerabilityItem = BG_FindItemForHoldable( HI_INVULNERABILITY );
	itemNum = invulnerabilityItem ? (int)( invulnerabilityItem - bg_itemlist ) : 0;

	if ( invulnerabilityItemNum ) {
		*invulnerabilityItemNum = itemNum;
	}

	return (qboolean)( itemNum != 0 &&
		pm->ps->stats[STAT_HOLDABLE_ITEM] == itemNum &&
		( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) );
}

/*
===================
PM_ShouldUseInvulnerabilityMove

Returns whether the current frame still owns the retail invulnerability move
path after the held-item and decay gates are applied.
===================
*/
static qboolean PM_ShouldUseInvulnerabilityMove( void ) {
	if ( !PM_HasHeldInvulnerabilityItem( NULL ) ) {
		return qfalse;
	}

	if ( pm->ps->pm_type == PM_DEAD ) {
		return qfalse;
	}

	if ( pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] != 0 && pm->ps->stats[STAT_PLAYER_ITEM_TIME] == 0 ) {
		return qfalse;
	}

	return qtrue;
}

/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	int				invulnerabilityItemNum;

	if ( !PM_ShouldUseInvulnerabilityMove() ) {
		pm->ps->powerups[PW_INVULNERABILITY] = 0;
		return;
	}

	PM_HasHeldInvulnerabilityItem( &invulnerabilityItemNum );

	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear( pm->ps->velocity );

	pm->ps->powerups[PW_INVULNERABILITY] = PM_INVULNERABILITY_ACTIVE_TIME;

	// Retail drives the shield's float/sink bias from view pitch while the holdable is active.
	pm->ps->velocity[2] += (int)( -pm->ps->viewangles[PITCH] * pml.frametime );

	if ( pm->ps->stats[STAT_PLAYER_ITEM_TIME] <= 32000 ) {
		pm->ps->stats[STAT_PLAYER_ITEM_TIME] -= pml.msec;
		if ( pm->ps->stats[STAT_PLAYER_ITEM_TIME] <= 0 ) {
			pm->ps->stats[STAT_PLAYER_ITEM_TIME] = 0;
			if ( pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] == 0 && invulnerabilityItemNum != 0 &&
				pm->ps->stats[STAT_HOLDABLE_ITEM] == invulnerabilityItemNum ) {
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		PM_ForceLegsAnim( LEGS_IDLECR );
	} else {
		PM_ForceLegsAnim( LEGS_JUMP );
	}
}

/*
===================
PM_FlyMove

Spectator and flight-powerup movement
===================
*/
static void PM_FlyMove( void ) {
	float	wishspeed;
	vec3_t	wishdir;

	// normal slowdown
	PM_Friction ();

	PM_BuildWishMove3D( wishdir, &wishspeed );

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	const pmove_settings_t	*settings;
	int		i;
	vec3_t	wishvel;
	float	fmove, smove;
	vec3_t	wishdir;
	float	wishspeed;
	float	accelerate;
	float	currentSpeed;
	float	scale;
	usercmd_t	cmd;

	settings = PM_GetActiveSettings();

	/* retail PM_AirMove 0x1002FCB0: crouch-slide recharge is airborne-only */
	if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->cmd.upmove >= 0 && settings->crouchSlideTime > 0 ) {
		pm->ps->crouchSlideTime += pml.msec * 5;
		if ( pm->ps->crouchSlideTime > settings->crouchSlideTime ) {
			pm->ps->crouchSlideTime = settings->crouchSlideTime;
		}
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	PM_SetMovementDir();

	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	accelerate = pm_airaccelerate;
	/* retail AirMove 0x1002FCB0 gates CPM air paths on PMF_AIR_CONTROL */
	if ( pm->ps->pm_flags & PMF_AIR_CONTROL ) {
		currentSpeed = DotProduct( pm->ps->velocity, wishdir );
		if ( currentSpeed < 0.0f ) {
			accelerate = pm_airstopaccelerate;
		}
		/* 0x1002FDEF: the pure-strafe branch is not an else, it replaces the
		   air-stop accel too */
		if ( pm->ps->movementDir == 2 || pm->ps->movementDir == 6 ) {
			if ( pm_wishspeed > 0.0f && wishspeed > pm_wishspeed ) {
				wishspeed = pm_wishspeed;
			}
			accelerate = pm_strafeaccelerate;
		}
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	if ( pm->ps->pm_flags & PMF_AIR_CONTROL ) {
		PM_AirControl( wishdir, wishspeed );
	}

	if ( pml.groundPlane ) {
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove( qtrue );

	/*
	 * retail PM_AirMove 0x1002FEA1..0x1002FEF9: airborne crouch legs. Ducked:
	 * force LEGS_IDLECR (timer cleared, toggle flipped every airborne frame);
	 * standing again while still in LEGS_IDLECR: force LEGS_IDLE.
	 */
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		PM_ForceLegsAnim( LEGS_IDLECR );
	} else if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLECR ) {
		PM_ForceLegsAnim( LEGS_IDLE );
	}

	PM_InvulnerabilityMove();

	if ( pm->ps->pm_flags & PMF_DOUBLE_JUMP ) {
		PM_CheckJump( qtrue );
	}
}


/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove( void ) {
	vec3_t	vel, v;
	float	vlen;
	const pmove_settings_t	*settings;
	float	maxSpeed;

	settings = PM_GetActiveSettings();
	if ( settings && settings->velocityGh > 0.0f ) {
		maxSpeed = settings->velocityGh;
	} else {
		maxSpeed = PM_DefaultSettings()->velocityGh;
	}
	if ( maxSpeed <= 0.0f ) {
		maxSpeed = 800.0f;
	}

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale( vel, maxSpeed, vel );


	VectorCopy(vel, pm->ps->velocity);

	/* ioql: enemy hook pulls at 25%; wall hook clears ground plane. */
	if (pm->hookEnemy) {
		VectorScale(pm->ps->velocity, 0.25f, pm->ps->velocity);
	} else {
		pml.groundPlane = qfalse;
	}

	if (pm->waterlevel > 1) {
		VectorScale(pm->ps->velocity, 0.75f, pm->ps->velocity);
	}
}


/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	const pmove_settings_t	*settings;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		duckScale;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}

	settings = PM_GetActiveSettings();

	if ( PM_ShouldUseInvulnerabilityMove() ) {
		PM_StepSlideMove( qfalse );
		PM_InvulnerabilityMove();
		return;
	}

	if ( PM_CheckJump( qfalse ) ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction ();

	/* retail WalkMove 0x10030430: no crouch-slide recharge here */

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	/* retail WalkMove 0x10030430: ducked clamps to 0.25*speed unless
	   crouch-sliding, in which case wishspeed is left unclamped. */
	if ( ( pm->ps->pm_flags & PMF_DUCKED ) && !( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) ) {
		duckScale = pm_duckScale;
		if ( wishspeed > pm->ps->speed * duckScale ) {
			wishspeed = pm->ps->speed * duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		/* retail WalkMove 0x1003067D: wade scale at waterlevel 1, swim scale
		   above, both through the Q3 depth formula */
		waterScale = ( pm->waterlevel == 1 ) ? pm_wadeScale : pm_swimScale;
		waterScale = 1.0f - ( ( (float)pm->waterlevel ) / 3.0f ) * ( 1.0f - waterScale );

		if ( waterScale > 0.0f && wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	/* retail WalkMove 0x100306DB: slick/knockback -> literal 1.0 (not the air
	   accel, which is 1.1 under PMF_PROMODE); ducked+slick -> 1.4 */
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
		accelerate = 1.0f;
		if ( ( pm->ps->pm_flags & PMF_DUCKED ) && ( pml.groundTrace.surfaceFlags & SURF_SLICK ) ) {
			accelerate = 1.4f;
		}
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

	/* retail WalkMove / cgame twin: ClipVelocity only — NO Q3 slope speed preserve */
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
		pm->ps->velocity, OVERCLIP );

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	vec3_t		wishdir;
	float		wishspeed;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		/* retail PM_NoclipMove 0x100308B4..0x100308DE: friction 10 (not
		   pm_friction * 1.5), frametime * 10 * control in extended precision */
		(void)friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop = (float)( (double)pml.frametime * 10.0 * (double)control );

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	PM_BuildWishMove3D( wishdir, &wishspeed );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pm->noFootsteps ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_SNOWSTEPS ) {
		return EV_FOOTSTEP_SNOW;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_WOODSTEPS ) {
		return EV_FOOTSTEP_WOOD;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	/* retail PM_CrashLand 0x100309F0: no Q3 "ducking doubles the fall" here */

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) && pm->ps->stats[STAT_HEALTH] > -40 )  {
		/* retail 0x10030B4F..0x10030B9B: 62 (double) / 42 (float) / 7 (double),
		   and no landing footstep with the scout rune (stats[2] == 1) */
		if ( delta > 62.0 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 42.0f ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7.0 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else if ( pm->ps->stats[STAT_RUNE] != 1 ) {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
	if ( pm->ps->pm_flags & PMF_DOUBLE_JUMP ) {
		pm->ps->doubleJumped = qfalse;
	}
}

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;
	qboolean	justLanded;
	int		previousGroundEntityNum;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	previousGroundEntityNum = pm->ps->groundEntityNum;
	justLanded = ( previousGroundEntityNum == ENTITYNUM_NONE );

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( justLanded ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}
		
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;	
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;
	qboolean	invulnerabilityActive;

	invulnerabilityActive = (qboolean)( pm->ps->powerups[PW_INVULNERABILITY] != 0 );

	if ( invulnerabilityActive ) {
		if ( pm->ps->pm_flags & PMF_INVULEXPAND ) {
			// invulnerability sphere has a 42 units radius
			VectorSet( pm->mins, -42, -42, -42 );
			VectorSet( pm->maxs, 42, 42, 42 );
		}
		else {
			VectorSet( pm->mins, -15, -15, MINS_Z );
			VectorSet( pm->maxs, 15, 15, 16 );
		}
		pm->ps->pm_flags |= PMF_DUCKED;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
		if ( pm->ps->crouchTime == 0 ) {
			pm->ps->crouchTime = pm->cmd.serverTime;
		}
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->ps->crouchSlideTime == 0 && pm->ps->speed < 400 ) {
			pm->ps->speed = 400;
		}
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
		if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pm->ps->crouchSlideTime != 0 && pml.groundPlane ) {
			pm->ps->crouchSlideTime = 0;
		}
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;
	qboolean	crouchSlideMoving;
	qboolean	runSpeed;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );
	crouchSlideMoving = (qboolean)( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE )
		&& pm->cmd.upmove < 0 && pm->ps->crouchSlideTime > 0 );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove && !crouchSlideMoving ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}
	

	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		}
		else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
		// ducked characters never play footsteps
	/*
	} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			footstep = qtrue;
		} else {
			bobmove = 0.3;
		}
		PM_ContinueLegsAnim( LEGS_BACK );
	*/
	} else {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			runSpeed = (qboolean)( pm->ps->velocity[0] >= 32.0f || pm->ps->velocity[0] <= -32.0f
				|| pm->ps->velocity[1] >= 32.0f || pm->ps->velocity[1] <= -32.0f );
			if ( runSpeed ) {
				bobmove = 0.4f;	// faster speeds bob faster
				if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
					PM_ContinueLegsAnim( LEGS_BACK );
				}
				else {
					PM_ContinueLegsAnim( LEGS_RUN );
				}
				footstep = qtrue;
			} else {
				bobmove = 0.35f;
				if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
					PM_ContinueLegsAnim( LEGS_BACKWALK );
				}
				else {
					PM_ContinueLegsAnim( LEGS_WALK );
				}
			}
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	/* retail 0x100316E0: (long double)msec * bobmove + old, truncated; in float
	   the sum can round up to the next integer */
	pm->ps->bobCycle = (int)( (double)pml.msec * (double)bobmove + (double)old ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	if ( pm->ps->pm_type == PM_DEAD ) {
		return;
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( !pm->ps->powerups[PW_INVULNERABILITY] && pml.previous_waterlevel == 3 && pm->waterlevel != 3 ) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	const pmove_settings_t	*settings;
	int		dropTime;

	if ( weapon <= WP_NONE || weapon > WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );

	if ( weapon == WP_NUM_WEAPONS ) {
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->weapon = WP_NUM_WEAPONS;
		PM_StartTorsoAnim( TORSO_STAND );
		return;
	}

	settings = PM_GetActiveSettings();
	dropTime = ( settings->weaponDropTime > 0 ) ? settings->weaponDropTime : 200;

	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += dropTime;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	const pmove_settings_t	*settings;
	int		weapon;
	int		raiseTime;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon > WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	settings = PM_GetActiveSettings();
	raiseTime = ( settings->weaponRaiseTime > 0 ) ? settings->weaponRaiseTime : 200;

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += raiseTime;
	pm->ps->stats[STAT_CHAINGUN_SPINUP] = 0;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate == WEAPON_READY ) {
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		return;
	}
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void ) {
	int		addTime;
	int		holdable;
	int		holdableTag;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	if ( pm->ps->weapon > WP_NONE && pm->ps->weapon < WP_NUM_WEAPONS &&
		( pm->cmd.buttons & BUTTON_ATTACK ) &&
		!( pm->ps->pm_flags & PMF_ATTACK_LOCKOUT ) &&
		( pm->ps->ammo[ pm->ps->weapon ] || pm->ps->weapon == WP_GRAPPLING_HOOK ) ) {
		pm->ps->eFlags |= EF_FIRING;
	}

	holdable = pm->ps->stats[STAT_HOLDABLE_ITEM];
	holdableTag = BG_HoldableForItemTag( bg_itemlist[ holdable ].giTag );

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
			pm->ps->pm_flags |= PMF_USE_ITEM_HELD;

			if ( holdableTag != HI_INVULNERABILITY &&
				( holdableTag != HI_MEDKIT ||
				pm->ps->stats[STAT_HEALTH] < ( pm->ps->stats[STAT_MAX_HEALTH] + 25 ) ) ) {
				PM_AddEvent( EV_USE_ITEM0 + holdableTag );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
		return;
	}

	if ( pm->ps->weapon == WP_NUM_WEAPONS ) {
		return;
	}

	// check for fire
	if ( ! ( pm->cmd.buttons & BUTTON_ATTACK ) || ( pm->ps->pm_flags & PMF_ATTACK_LOCKOUT ) ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->stats[STAT_CHAINGUN_SPINUP] -= pml.msec;
		if ( pm->ps->stats[STAT_CHAINGUN_SPINUP] < 0 ) {
			pm->ps->stats[STAT_CHAINGUN_SPINUP] = 0;
		} else if ( pm->ps->stats[STAT_CHAINGUN_SPINUP] > 1000 ) {
			pm->ps->stats[STAT_CHAINGUN_SPINUP] = 1000;
		}
		return;
	}

	if ( pm->ps->pm_flags & PMF_WEAPON_RESET ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->eFlags &= ~EF_FIRING;
		pm->ps->pm_flags &= ~PMF_WEAPON_RESET;
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] ) {
		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 ) {
		pm->ps->ammo[ pm->ps->weapon ]--;
	}

	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );

	addTime = PM_GetWeaponReloadTime( (weapon_t)pm->ps->weapon );

	if ( pm->ps->weapon == WP_CHAINGUN ) {
		if ( pm->ps->stats[STAT_CHAINGUN_SPINUP] < 1000 ) {
			addTime *= 2;
		}
		pm->ps->stats[STAT_CHAINGUN_SPINUP] += addTime;
		if ( pm->ps->stats[STAT_CHAINGUN_SPINUP] < 0 ) {
			pm->ps->stats[STAT_CHAINGUN_SPINUP] = 0;
		} else if ( pm->ps->stats[STAT_CHAINGUN_SPINUP] > 1000 ) {
			pm->ps->stats[STAT_CHAINGUN_SPINUP] = 1000;
		}
	}

	if ( BG_PlayerHasPersistantPowerup( pm->ps, PW_SCOUT ) ) {
		addTime /= 1.25f;
	} else if ( BG_PlayerHasPersistantPowerup( pm->ps, PW_AMMOREGEN ) ) {
		addTime /= 1.3;
	} else if ( pm->ps->powerups[PW_HASTE] ) {
		addTime /= 1.3;
	}

	pm->ps->weaponTime += addTime;

}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}

	if ( ( pm->ps->pm_flags & PMF_CROUCH_SLIDE ) && pml.groundPlane && pm->ps->crouchSlideTime != 0 ) {
		pm->ps->crouchSlideTime -= pml.msec;
		if ( pm->ps->crouchSlideTime < 0 ) {
			pm->ps->crouchSlideTime = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->pm_type != PM_FREEZE && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

}


/*
================
PmoveSingle

================
*/
void PmoveSingle (pmove_t *pmove) {
	const pmove_settings_t	*settings;

	pm = pmove;
	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	settings = PM_GetActiveSettings();
	PM_LoadMoveTuningConstants();

	if ( pm->debugLevel > 1 ) {
		Com_Printf( "pmove_cfg: wish=%.3f airAccel=%.3f airControl=%.3f airSteps=%d stop=%.3f strafe=%.3f autoHop=%d bunnyHop=%d\n",
			pm_wishspeed,
			pm_airaccelerate,
			pm_aircontrol,
			pm_airsteps,
			pm_airstopaccelerate,
			pm_strafeaccelerate,
			pm_autohop ? 1 : 0,
			pm_bunnyhop ? 1 : 0 );
	}

	if ( pm->ps->stats[STAT_HEALTH] <= 0 && !pm->ps->powerups[PW_INVULNERABILITY] ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	if ( settings->noPlayerClip ) {
		pm->tracemask &= ~CONTENTS_BODY;
	}

// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// PM_Weapon relatches this after the retail attack, health, and ammo gates.
	pm->ps->eFlags &= ~EF_FIRING;

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	if ( pm->ps->pm_flags & PMF_NO_MOVE ) {
		return;
	}

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	// Retail mirrors raw command state before movement rewrites inputs.
	if ( pm->cmd.weaponPrimary > WP_NONE && pm->cmd.weaponPrimary < WP_NUM_WEAPONS ) {
		pm->ps->weaponPrimary = pm->cmd.weaponPrimary;
	}
	if ( pm->cmd.fov >= 10 && pm->cmd.fov <= 130 ) {
		pm->ps->fov = pm->cmd.fov;
	}
	pm->ps->forwardmove = pm->cmd.forwardmove;
	pm->ps->rightmove = pm->cmd.rightmove;
	pm->ps->upmove = pm->cmd.upmove;

	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	/* retail PmoveSingle 0x1003223E: PM_FREEZE only clears the move (word at
	   cmd+0x17 = forward/right, byte = upmove) and then runs the normal
	   movement, so a frozen player still falls and slides */
	if ( pm->ps->pm_type == PM_FREEZE ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();
	PM_CheckLadder();
	pm->ps->powerups[PW_INVULNERABILITY] = 0;

	if ( pm->ps->powerups[PW_FLIGHT] ) {
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	} else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
	} else if ( pml.ladder ) {
		PM_LadderMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	if ( pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] != 0 && !( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
		int	playerItemTime;

		playerItemTime = pm->ps->stats[STAT_PLAYER_ITEM_TIME] +
			(int)( pm->ps->stats[STAT_PLAYER_ITEM_RECHARGE] * pml.frametime );
		if ( playerItemTime > pm->ps->stats[STAT_PLAYER_ITEM_TIME_MAX] ) {
			playerItemTime = pm->ps->stats[STAT_PLAYER_ITEM_TIME_MAX];
		}
		pm->ps->stats[STAT_PLAYER_ITEM_TIME] = playerItemTime;
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int			finalTime;

	pmove->stepUp = 0.0f;
	pmove->stepUpTime = 0;
	pmove->stepHeight = 0.0f;
	pmove->stepTime = 0;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		}
		else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );
		if ( pml.stepUp > 0.0f ) {
			pmove->stepUp += pml.stepUp;
			pmove->stepUpTime = pmove->cmd.serverTime;
			pmove->stepHeight = pmove->stepUp;
			pmove->stepTime = pmove->stepUpTime;
		}

		/* retail Pmove 0x100323A0: JUMP_HELD → upmove=20 (no originalUpmove gate) */
		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}

	if ( pmove->stepUp > 0.0f ) {
		pmove->stepHeight = pmove->stepUp;
		pmove->stepTime = pmove->stepUpTime;
	}

	//PM_CheckStuck();

}

/*
 * ioquakelive adapter: keep CS_PMOVEINFO / weapon_reload_* wiring working.
 * Runtime settings start from retail defaults and are mutated by PM_Set*.
 */
static pmove_settings_t pm_runtimeSettings;
static qboolean pm_runtimeSettingsReady = qfalse;

static void PM_EnsureRuntimeSettings(void) {
	if (pm_runtimeSettingsReady) {
		return;
	}
	pm_runtimeSettings = *PM_DefaultSettings();
	pm_runtimeSettingsReady = qtrue;
}

const pmove_settings_t *PM_GetActiveSettings(void) {
	if (pm && pm->pmoveSettings) {
		return pm->pmoveSettings;
	}
	PM_EnsureRuntimeSettings();
	return &pm_runtimeSettings;
}

const pmove_settings_t *PM_GetDefaultSettings(void) {
	return PM_DefaultSettings();
}

void PM_FillPmoveSettingsFromGlobals(pmove_settings_t *out) {
	if (!out) {
		return;
	}
	PM_EnsureRuntimeSettings();
	*out = pm_runtimeSettings;
}

void PM_SetAirAccel(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.airAccel = value; }
void PM_SetAirControl(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.airControl = value; }
void PM_SetAirStepFriction(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.airStepFriction = value; }
void PM_SetAirSteps(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.airSteps = value; }
void PM_SetAirStopAccel(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.airStopAccel = value; }
void PM_SetAutoHop(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.autoHop = value ? qtrue : qfalse; }
void PM_SetBunnyHop(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.bunnyHop = value ? qtrue : qfalse; }
void PM_SetChainJump(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.chainJump = value; }
void PM_SetChainJumpVelocity(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.chainJumpVelocity = value; }
void PM_SetCircleStrafeFriction(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.circleStrafeFriction = value; }
void PM_SetCrouchSlideFriction(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.crouchSlideFriction = value; }
void PM_SetCrouchSlideTime(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.crouchSlideTime = value; }
void PM_SetCrouchStepJump(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.crouchStepJump = value ? qtrue : qfalse; }
void PM_SetJumpTimeDeltaMin(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpTimeDeltaMin = value; }
void PM_SetJumpVelocity(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpVelocity = value; }
void PM_SetJumpVelocityMax(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpVelocityMax = value; }
void PM_SetJumpVelocityScaleAdd(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpVelocityScaleAdd = value; }
void PM_SetJumpVelocityTimeThreshold(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpVelocityTimeThreshold = value; }
void PM_SetJumpVelocityTimeThresholdOffset(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.jumpVelocityTimeThresholdOffset = value; }
void PM_SetNoPlayerClip(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.noPlayerClip = value ? qtrue : qfalse; }
void PM_SetRampJump(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.rampJump = value ? qtrue : qfalse; }
void PM_SetRampJumpScale(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.rampJumpScale = value; }
void PM_SetStepHeight(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.stepHeight = value; }
void PM_SetStepJump(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.stepJump = value ? qtrue : qfalse; }
void PM_SetStepJumpVelocity(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.stepJumpVelocity = value; }
void PM_SetStrafeAccel(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.strafeAccel = value; }
void PM_SetWalkAccel(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.walkAccel = value; }
void PM_SetWalkFriction(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.walkFriction = value; }
void PM_SetWaterSwimScale(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.waterSwimScale = value; }
void PM_SetWaterWadeScale(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.waterWadeScale = value; }
void PM_SetWeaponDropTime(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.weaponDropTime = value; }
void PM_SetWeaponRaiseTime(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.weaponRaiseTime = value; }
void PM_SetWishSpeed(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.wishSpeed = value; }
void PM_SetDoubleJump(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.doubleJump = value ? qtrue : qfalse; }
void PM_SetCrouchSlide(int value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.crouchSlide = value ? qtrue : qfalse; }
void PM_SetVelocityGH(float value) { PM_EnsureRuntimeSettings(); pm_runtimeSettings.velocityGh = value; }

/* Compatibility no-op: retail tuning is applied inside PM_LoadMoveTuningConstants. */
void PM_SetMovementConfig(void) {
}
