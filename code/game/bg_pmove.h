/*
  ==============================
  Written by:
    id software :            Quake III Arena
    sOkam! :                 Opensource Defrag

  This file is part of Opensource Defrag.

  Opensource Defrag is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Opensource Defrag is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Opensource Defrag.  If not, see <http://www.gnu.org/licenses/>.
  ==============================
*/

// TODO: Remove every unneeded connection to this file
#ifndef BG_PMOVE_H
#define BG_PMOVE_H

#include "bg_public.h"

#include "bg_local.h"

#include "../qcommon/q_shared.h"

extern pmove_t* pm;
extern pml_t pml;

#if 1
#define LIB_ONLY_STATIC
#else
#define LIB_ONLY_STATIC static
#endif

// default movement parameters
extern float pm_spectatorfriction;

extern int c_pmove;

void trap_SnapVector(float* v);

void PM_AddEvent(int newEvent);
void PM_AddTouchEnt(int entityNum);
// TODO: Move back to Static
LIB_ONLY_STATIC void PM_StartTorsoAnim(int anim);
LIB_ONLY_STATIC void PM_StartLegsAnim(int anim);
LIB_ONLY_STATIC void PM_ContinueLegsAnim(int anim);
LIB_ONLY_STATIC void PM_ContinueTorsoAnim(int anim);
LIB_ONLY_STATIC void PM_ForceLegsAnim(int anim);
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);
LIB_ONLY_STATIC void PM_Friction(void);
LIB_ONLY_STATIC void PM_Accelerate(vec3_t wishdir, float wishspeed,
                                   float accel);
LIB_ONLY_STATIC float PM_CmdScale(usercmd_t* cmd);
LIB_ONLY_STATIC void PM_SetMovementDir(void);
LIB_ONLY_STATIC qboolean PM_CheckJump(void);
LIB_ONLY_STATIC qboolean PM_CheckWaterJump(void);
LIB_ONLY_STATIC void PM_WaterJumpMove(void);
LIB_ONLY_STATIC void PM_WaterMove(void);

#ifdef TEAMARENA
void PM_InvulnerabilityMove(void);
#endif
LIB_ONLY_STATIC void PM_AirMove(void);
LIB_ONLY_STATIC void PM_GrappleMove(void);
LIB_ONLY_STATIC void PM_WalkMove(void);
LIB_ONLY_STATIC int PM_FootstepForSurface(void);
LIB_ONLY_STATIC void PM_CrashLand(void);
/*
void PM_CheckStuck(void);
*/
LIB_ONLY_STATIC int PM_CorrectAllSolid(trace_t* trace);
LIB_ONLY_STATIC void PM_GroundTraceMissed(void);
LIB_ONLY_STATIC void PM_Footsteps(void);
LIB_ONLY_STATIC void PM_WaterEvents(void);

LIB_ONLY_STATIC void PM_TorsoAnimation(void);
LIB_ONLY_STATIC void PM_Weapon(void);
LIB_ONLY_STATIC void PM_Animate(void);

void PM_UpdateViewAngles(playerState_t* ps, const usercmd_t* cmd);
LIB_ONLY_STATIC void PM_CheckDuck(void);
LIB_ONLY_STATIC void PM_FlyMove(void);
LIB_ONLY_STATIC void PM_DropTimers(void);
LIB_ONLY_STATIC void PM_NoclipMove(void);
LIB_ONLY_STATIC void PM_SetWaterLevel(void);
LIB_ONLY_STATIC void PM_GroundTrace(void);
LIB_ONLY_STATIC void PM_DeadMove(void);

#endif // BG_PMOVE_H
