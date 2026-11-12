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
// bg_slidemove.c -- part of bg_pmove functionality

#include "ql_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean	PM_SlideMove( qboolean gravity ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	
	numbumps = 4;

	VectorCopy (pm->ps->velocity, primal_velocity);

	if ( gravity ) {
		VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			// slide along the ground plane
			PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
				pm->ps->velocity, OVERCLIP );
		}
	} else {
		/* retail PM_SlideMove(qfalse) 0x1002e670: zero endVelocity */
		VectorClear( endVelocity );
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy (trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1) {
			 break;		// moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0 ; i < numplanes ; i++ ) {
			if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 ) {
				VectorAdd( trace.plane.normal, pm->ps->velocity, pm->ps->velocity );
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = DotProduct( pm->ps->velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity (pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_CanProbeStepJump

Returns whether the current state passes the coarse retail step-jump probe
gates that are shared by both helper branches.
==================
*/
static qboolean PM_CanProbeStepJump( const pmove_settings_t *settings ) {
	if ( !pm || !pm->ps || !settings ) {
		return qfalse;
	}

	if ( !settings->stepJump ) {
		return qfalse;
	}

	if ( pm->ps->pm_type != PM_NORMAL ) {
		return qfalse;
	}

	if ( pm->waterlevel >= 2 ) {
		return qfalse;
	}

	return qtrue;
}

/*
==================
PM_ShouldRequireStepJumpRelease

Returns whether the retail step-jump probes still require a fresh jump press.
==================
*/
static qboolean PM_ShouldRequireStepJumpRelease( const pmove_settings_t *settings ) {
	if ( pm && pm->ps && ( pm->ps->pm_flags & PMF_REQUIRE_JUMP_RELEASE ) ) {
		return qtrue;
	}

	if ( settings ) {
		if ( settings->autoHop ) {
			return qfalse;
		}
	}

	if ( pm_autohop ) {
		return qfalse;
	}

	return qtrue;
}

/*
==================
PM_HasSatisfiedStepJumpDelay

Checks the retail jump-time delay window used by the step-jump gates.
==================
*/
static qboolean PM_HasSatisfiedStepJumpDelay( const pmove_settings_t *settings ) {
	int		timeDelta;

	if ( !pm || !pm->ps || !settings ) {
		return qfalse;
	}

	if ( settings->jumpTimeDeltaMin <= 0.0f ) {
		return qtrue;
	}

	if ( pm->cmd.serverTime < pm->ps->jumpTime ) {
		return qfalse;
	}

	timeDelta = pm->cmd.serverTime - pm->ps->jumpTime;
	return ( (float)timeDelta >= settings->jumpTimeDeltaMin ) ? qtrue : qfalse;
}

/*
==================
PM_CanStepJump

Returns whether the general retail step-jump helper should run.
==================
*/
static qboolean PM_CanStepJump( void ) {
	const pmove_settings_t	*settings;
	qboolean		releaseRequired;

	settings = PM_GetActiveSettings();
	if ( !PM_CanProbeStepJump( settings ) ) {
		return qfalse;
	}

	releaseRequired = PM_ShouldRequireStepJumpRelease( settings );

	if ( releaseRequired && ( pm->ps->pm_flags & PMF_RESPAWNED ) ) {
		return qfalse;
	}

	if ( pm->cmd.upmove < 10 ) {
		return qfalse;
	}

	if ( releaseRequired && ( pm->ps->pm_flags & PMF_JUMP_HELD ) ) {
		pm->cmd.upmove = 0;
		return qfalse;
	}

	if ( !PM_HasSatisfiedStepJumpDelay( settings ) ) {
		pm->cmd.upmove = 0;
		return qfalse;
	}

	return qtrue;
}

/*
==================
PM_CanCrouchStepJump

Returns whether the crouch-specific retail step-jump probe should run.
==================
*/
static qboolean PM_CanCrouchStepJump( void ) {
	const pmove_settings_t	*settings;

	settings = PM_GetActiveSettings();
	if ( !PM_CanProbeStepJump( settings ) ) {
		return qfalse;
	}

	if ( !settings->crouchStepJump ) {
		return qfalse;
	}

	if ( !( pm->ps->pm_flags & PMF_DUCKED ) ) {
		return qfalse;
	}

	if ( pml.groundPlane ) {
		return qfalse;
	}

	if ( pm->ps->velocity[2] < 0.0f ) {
		return qfalse;
	}

	if ( !PM_HasSatisfiedStepJumpDelay( settings ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
==================
PM_CanPerformCrouchStepJump

Validates the extra retail clearance trace for crouch step jumps.
==================
*/
static qboolean PM_CanPerformCrouchStepJump( void ) {
	vec3_t		mins, maxs;
	vec3_t		end;
	trace_t		trace;

	VectorCopy( pm->mins, mins );
	VectorCopy( pm->maxs, maxs );
	mins[0] += 1.0f;
	mins[1] += 1.0f;
	maxs[0] -= 1.0f;
	maxs[1] -= 1.0f;

	VectorCopy( pm->ps->origin, end );
	end[2] -= 64.0f;
	pm->trace( &trace, pm->ps->origin, mins, maxs, end, pm->ps->clientNum, pm->tracemask );

	return ( trace.fraction == 1.0f );
}

/*
==================
PM_StepSlideMoveWithStepHeight

==================
*/
static void PM_StepSlideMoveWithStepHeight( qboolean gravity, float stepHeight ) {
	vec3_t		start_o, start_v;
	trace_t		trace;
	vec3_t		up, down;
	vec3_t		end;
	vec3_t		jumpProbeStart, jumpProbeEnd;
	vec3_t		projectedEnd;
	float		stepSize;
	float		velocityDot;

	pml.stepUp = 0.0f;
	VectorCopy (pm->ps->origin, start_o);
	VectorCopy (pm->ps->velocity, start_v);

	if ( PM_SlideMove( gravity ) == 0 ) {
		return;		// we got exactly where we wanted to go first try	
	}

	if ( pm_airsteps <= 0 ) {
		VectorMA( start_o, pml.frametime, start_v, end );
		pm->trace( &trace, start_o, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask );

		VectorCopy( trace.endpos, down );
		down[2] -= stepHeight;
		pm->trace( &trace, trace.endpos, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );
		VectorSet( up, 0, 0, 1 );

		// retail QL only allows upward air stepping when the projected endpoint still
		// finds support within the configured step height.
		if ( start_v[2] > 0.0f && ( trace.fraction == 1.0f || DotProduct( trace.plane.normal, up ) < MIN_WALK_NORMAL ) ) {
			return;
		}
	}

	VectorCopy (start_o, up);
	up[2] += stepHeight;

	// test the player position if they were a stepheight higher
	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if ( trace.allsolid ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return;		// can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];
	// try slidemove from this position
	VectorCopy (trace.endpos, pm->ps->origin);
	VectorCopy (start_v, pm->ps->velocity);

	PM_SlideMove( gravity );

	// push down the final amount
	VectorCopy (pm->ps->origin, down);
	down[2] -= stepSize;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if ( !trace.allsolid ) {
		VectorCopy (trace.endpos, pm->ps->origin);
	}
	if ( trace.fraction < 1.0f ) {
		velocityDot = DotProduct( pm->ps->velocity, trace.plane.normal );
		if ( velocityDot < 0.0f || Q_fabs( velocityDot ) < 0.001f ) {
			PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
		}
	}

	// Retail only records step side effects when the stepped endpoint is not directly reachable.
	pm->trace( &trace, start_o, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
	if ( trace.fraction < 1.0f ) {
		float	delta;
		float	stepFriction;
		qboolean	canStepJump;
		qboolean	canCrouchStepJump;

		delta = pm->ps->origin[2] - start_o[2];
		if ( delta > 2.0f ) {
			pml.stepUp = delta;
			pm->stepHeight = delta;
			pm->stepTime = pm->cmd.serverTime;
		}

		if ( !pml.groundPlane && delta > 0.0f && start_v[2] > 0.0f ) {
			stepFriction = 1.0f - pm_airstepfriction;
			if ( stepFriction < 0.0f ) {
				stepFriction = 0.0f;
			}

			pm->ps->velocity[0] *= stepFriction;
			pm->ps->velocity[1] *= stepFriction;
		}

		canStepJump = qfalse;
		canCrouchStepJump = qfalse;
		if ( delta > 0.0f ) {
			canStepJump = PM_CanStepJump();
			canCrouchStepJump = PM_CanCrouchStepJump();
		}

		if ( canStepJump || canCrouchStepJump ) {
			VectorMA( start_o, pml.frametime, start_v, projectedEnd );
			VectorCopy( projectedEnd, jumpProbeStart );
			VectorCopy( projectedEnd, jumpProbeEnd );
			jumpProbeStart[2] += stepHeight;
			jumpProbeEnd[2] -= stepHeight;
			pm->trace( &trace, jumpProbeStart, pm->mins, pm->maxs, jumpProbeEnd, pm->ps->clientNum, pm->tracemask );

			if ( !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f && trace.plane.normal[2] >= MIN_WALK_NORMAL ) {
				if ( canStepJump && PM_CanStepJump() ) {
					PM_ApplyStepJump( delta, qfalse );
				} else if ( canCrouchStepJump && PM_CanCrouchStepJump() && PM_CanPerformCrouchStepJump() ) {
					PM_ApplyStepJump( delta, qtrue );
				}
			}
		}

		if ( pm->debugLevel ) {
			Com_Printf("%i:stepped %f\n", c_pmove, delta);
		}
	}
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( qboolean gravity ) {
	PM_StepSlideMoveWithStepHeight( gravity, pm_stepHeight );
}

